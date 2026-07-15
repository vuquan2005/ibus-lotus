/*
 * Bamboo - A Vietnamese Input method editor
 * Copyright (C) 2018 Luong Thanh Lam <ltlam93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package lotusibus

import (
	"log"
	"os/exec"
	"reflect"
	"sync"

	"github.com/BambooEngine/bamboo-core"
	ibus "github.com/BambooEngine/goibus"
	"github.com/godbus/dbus/v5"

	"ibus-lotus/config"
	"ibus-lotus/ui"
)

type IBusLotusEngine struct {
	sync.Mutex
	IEngine
	preeditor              bamboo.IEngine
	engineName             string
	config                 *config.Config
	propList               *ibus.PropList
	englishMode            bool
	macroTable             *MacroTable
	wmClasses              string
	isInputModeLTOpened    bool
	inputModeLookupTable   *ibus.LookupTable
	capabilities           uint32
	keyPressDelay          int
	isFirstTimeSendingBS   bool
	isSurroundingTextReady bool
	lastKeyWithShift       bool
	lastCommitText         int64
	// restore key strokes by pressing Shift + Space
	shouldRestoreKeyStrokes bool
	// enqueue key strokes to process later
	shouldEnqueuKeyStrokes bool
	currentWordNearCursor  string
	wordCompleted          bool

	hwndModes            map[string]int
	hwndFocusOrder       []string
	hwndMutex            sync.RWMutex
	lastFocusedHwnd      string
	shouldShowFocusToast bool
}

func NewIbusLotusEngine(name string, cfg *config.Config, base IEngine, preeditor bamboo.IEngine) *IBusLotusEngine {
	return &IBusLotusEngine{
		engineName: name,
		IEngine:    base,
		preeditor:  preeditor,
		config:     cfg,
		hwndModes:  make(map[string]int),
	}
}

/*
*
Implement IBus.Engine's process_key_event default signal handler.

Args:

	keyval - The keycode, transformed through a keymap, stays the
		same for every keyboard
	keycode - Keyboard-dependant key code
	modifiers - The state of IBus.ModifierType keys like
		Shift, Control, etc.

Return:

	True - if successfully process the keyevent
	False - otherwise. The keyevent will be passed to X-Client

This function gets called whenever a key is pressed.
*/
func (e *IBusLotusEngine) ProcessKeyEvent(keyVal uint32, keyCode uint32, state uint32) (bool, *dbus.Error) {
	if state&IBusReleaseMask != 0 {
		// fmt.Println("Ignore key-up event")
		return false, nil
	}

	if e.shouldShowFocusToast {
		e.shouldShowFocusToast = false
		go func() {
			mode := e.getInputMode()
			msg := "🟡 Pre-edit"
			switch mode {
			case config.UsIM:
				msg = "🔵 English"
			case config.SurroundingTextIM:
				msg = "🟢 Surrounding Text"
			}
			e.showAuxToast(msg)
		}()
	}

	if ret, retValue := e.processShortcutKey(keyVal, keyCode, state); ret {
		return retValue, nil
	}

	if !isValidState(state) {
		return false, nil
	}
	log.Printf("[DEBUG] ProcessKeyEvent: key=%s state=0x%x queue=%d", getKeyValName(keyVal), state, len(keyPressChan))
	if e.inBackspaceWhiteList() {
		return e.bsProcessKeyEvent(keyVal, keyCode, state)
	}
	return e.preeditProcessKeyEvent(keyVal, keyCode, state)
}

func (e *IBusLotusEngine) FocusIn() *dbus.Error {
	log.Print("[DEBUG] FocusIn")

	e.RegisterProperties(e.propList)
	e.RequireSurroundingText()
	if e.config.IBflags&config.IBspellCheckWithDicts != 0 && len(dictionary) == 0 {
		dictionary, _ = loadDictionary(DictVietnameseCm)
	}

	if !e.config.EnableAutoSwitch {
		return nil
	}

	go func() {
		info := e.getLatestWindowInfo()
		e.Lock()

		windowChanged := info.ID != "" && info.ID != e.lastFocusedHwnd
		classChanged := info.Class != "" && info.Class != e.wmClasses

		if !windowChanged && !classChanged {
			e.Unlock()
			return
		}

		if windowChanged {
			e.lastFocusedHwnd = info.ID
			if e.config.EnableFocusToast {
				e.shouldShowFocusToast = true
			}
		}

		e.checkWmClass(info.Class)
		currentWm := e.getWmClass()
		activeMode := e.getInputMode()
		e.englishMode = (activeMode == config.UsIM)

		e.Unlock()
		log.Printf("[INFO ] Active window changed: WM_CLASS=%q HWND=%q, Mode=%s (EnglishMode=%t)", currentWm, info.ID, getInputModeName(activeMode), e.englishMode)
	}()

	return nil
}

func (e *IBusLotusEngine) FocusOut() *dbus.Error {
	log.Print("[DEBUG] FocusOut")
	return nil
}

func (e *IBusLotusEngine) Reset() *dbus.Error {
	log.Print("[DEBUG] Reset")
	if e.checkInputMode(config.PreeditIM) {
		e.preeditor.Reset()
	}
	return nil
}

func (e *IBusLotusEngine) Enable() *dbus.Error {
	log.Print("[DEBUG] Enable")
	e.RegisterProperties(e.propList)
	e.RequireSurroundingText()
	return nil
}

func (e *IBusLotusEngine) Disable() *dbus.Error {
	log.Print("[DEBUG] Disable")
	return nil
}

// @method(in_signature="vuu")
func (e *IBusLotusEngine) SetSurroundingText(text dbus.Variant, cursorPos uint32, anchorPos uint32) *dbus.Error {
	defer func() {
		if err := recover(); err != nil {
			log.Printf("[ERROR] Recovered panic in SetSurroundingText: %v", err)
		}
	}()

	var str = reflect.ValueOf(reflect.ValueOf(text.Value()).Index(2).Interface()).String()
	var s = []rune(str)
	if len(s) >= int(cursorPos) {
		e.currentWordNearCursor = getLastWordFromSentence(string(s[:cursorPos]))
	} else {
		e.currentWordNearCursor = getLastWordFromSentence(str)
	}

	if e.wordCompleted {
		e.wordCompleted = false

		cPos := int(cursorPos)
		if cPos < 0 {
			cPos = 0
		}
		if cPos > len(s) {
			cPos = len(s)
		}
		start := cPos - 15
		if start < 0 {
			start = 0
		}
		end := cPos + 15
		if end > len(s) {
			end = len(s)
		}
		context := string(s[start:cPos]) + "|" + string(s[cPos:end])

		log.Printf("[DEBUG] SetSurroundingText (word completed): context=%q, currentWord=%q, cursorPos=%d, anchorPos=%d, ready=%t",
			context, e.currentWordNearCursor, cursorPos, anchorPos, e.isSurroundingTextReady)
	}

	if !e.isSurroundingTextReady {
		//fmt.Println("Surrounding Text is not ready yet.")
		return nil
	}
	e.Lock()
	defer func() {
		e.Unlock()
		if err := recover(); err != nil {
			log.Printf("[ERROR] Recovered panic in SetSurroundingText: %v", err)
		}
	}()
	if e.inBackspaceWhiteList() {
		if len(s) < int(cursorPos) {
			return nil
		}
		var cs = s[:cursorPos]
		log.Printf("[DEBUG] Surrounding text: %q, Current word: %q", string(cs), e.currentWordNearCursor)
		if len(cs) == 0 || cs[len(cs)-1] == ' ' || bamboo.IsWordBreakSymbol(cs[len(cs)-1]) || bamboo.IsPunctuationMark(cs[len(cs)-1]) {
			// Do not consume isSurroundingTextReady yet as it is not a rebuildable word.
			return nil
		}
		e.isSurroundingTextReady = false
		e.preeditor.Reset()
		for i := len(cs) - 1; i >= 0; i-- {
			if cs[i] == ' ' || cs[i] == '\t' || cs[i] == '\n' || bamboo.IsWordBreakSymbol(cs[i]) || bamboo.IsPunctuationMark(cs[i]) {
				break
			}
			e.preeditor.ProcessKey(cs[i], bamboo.EnglishMode|bamboo.InReverseOrder)
		}
	}
	return nil
}

func (e *IBusLotusEngine) PageUp() *dbus.Error {
	if e.isInputModeLTOpened && e.inputModeLookupTable.PageUp() {
		e.updateInputModeLT()
	}
	return nil
}

func (e *IBusLotusEngine) PageDown() *dbus.Error {
	if e.isInputModeLTOpened && e.inputModeLookupTable.PageDown() {
		e.updateInputModeLT()
	}
	return nil
}

func (e *IBusLotusEngine) CursorUp() *dbus.Error {
	if e.isInputModeLTOpened && e.inputModeLookupTable.CursorUp() {
		e.updateInputModeLT()
	}
	return nil
}

func (e *IBusLotusEngine) CursorDown() *dbus.Error {
	if e.isInputModeLTOpened && e.inputModeLookupTable.CursorDown() {
		e.updateInputModeLT()
	}
	return nil
}

func (e *IBusLotusEngine) CandidateClicked(index uint32, button uint32, state uint32) *dbus.Error {
	if e.isInputModeLTOpened && e.inputModeLookupTable.SetCursorPos(index) {
		e.commitInputModeCandidate()
		e.closeInputModeCandidates()
	}
	return nil
}

func (e *IBusLotusEngine) SetCapabilities(cap uint32) *dbus.Error {
	e.capabilities = cap
	return nil
}

func (e *IBusLotusEngine) SetCursorLocation(x int32, y int32, w int32, h int32) *dbus.Error {
	return nil
}

func (e *IBusLotusEngine) SetContentType(purpose uint32, hints uint32) *dbus.Error {
	return nil
}

// @method(in_signature="su")
func (e *IBusLotusEngine) PropertyActivate(propName string, propState uint32) *dbus.Error {
	if propName == PropKeyAbout {
		exec.Command("xdg-open", HomePage).Start()
		return nil
	}
	if propName == PropKeyConfiguration || propName == PropKeyInputModeLookupTableShortcut || propName == PropKeyMacroTable {
		ui.OpenGUI(e.engineName)
		e.config = config.LoadConfig(e.engineName)
		e.applyConfig()
		return nil
	}
	if propName == PropKeyMacroEnabled {
		if propState == ibus.PROP_STATE_CHECKED {
			e.config.IBflags |= config.IBmacroEnabled
			e.macroTable.Enable(e.engineName)
		} else {
			e.config.IBflags &= ^config.IBmacroEnabled
			e.macroTable.Disable()
		}
		config.SaveConfig(e.config, e.engineName)
		e.propList = GetPropListByConfig(e.config, e.englishMode)
		e.RegisterProperties(e.propList)
		return nil
	}
	return nil
}

func (e *IBusLotusEngine) applyConfig() {
	if e.config.IBflags&config.IBspellCheckWithDicts != 0 && len(dictionary) == 0 {
		dictionary, _ = loadDictionary(DictVietnameseCm)
	}
	if e.macroTable != nil {
		if e.config.IBflags&config.IBmacroEnabled != 0 {
			e.macroTable.Enable(e.engineName)
			e.macroTable.Reload(e.engineName, e.config.IBflags&config.IBautoCapitalizeMacro != 0)
		} else {
			e.macroTable.Disable()
		}
	}
	activeMode := e.getInputMode()
	if activeMode == config.UsIM {
		e.englishMode = true
	} else {
		e.englishMode = false
	}
	e.propList = GetPropListByConfig(e.config, e.englishMode)
	var inputMethod = bamboo.ParseInputMethod(e.config.InputMethodDefinitions, e.config.InputMethod)
	e.preeditor = bamboo.NewEngine(inputMethod, e.config.Flags)
	e.RegisterProperties(e.propList)
	log.Printf("[INFO ] Config applied. Input mode: %s (EnglishMode=%t)", getInputModeName(activeMode), e.englishMode)
}
