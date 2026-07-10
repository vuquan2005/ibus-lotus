package main

import (
	"fmt"
	"ibus-bamboo/config"
	"log"
	"time"
)

const BACKSPACE_INTERVAL = 0

var fakeBackspaceCount = 0

func (e *IBusBambooEngine) SendBackSpace(n int) {
	fakeBackspaceCount = n
	e.SendBackspaceFromInputMode()
}

func (e *IBusBambooEngine) SendBackspaceFromInputMode() {
	switch e.getInputMode() {
	case config.SurroundingTextIM:
		e.SendBackspaceInSurroundingTextMode()
	case config.ShiftLeftForwardingIM:
		e.SendBackspaceShiftLeftForwardingMode()
	case config.BackspaceForwardingIM:
		e.SendBackspaceBackspaceForwardingMode()
	default:
		fmt.Println("There's something wrong with wmClasses")
	}
}

func (e *IBusBambooEngine) SendBackspaceInSurroundingTextMode() {
	time.Sleep(20 * time.Millisecond)
	log.Printf("Sendding %d backspace via SurroundingText\n", fakeBackspaceCount)
	e.DeleteSurroundingText(-int32(fakeBackspaceCount), uint32(fakeBackspaceCount))
	time.Sleep(20 * time.Millisecond)
}

func (e *IBusBambooEngine) SendBackspaceShiftLeftForwardingMode() {
	time.Sleep(30 * time.Millisecond)
	log.Printf("Sendding %d Shift+Left via shiftLeftForwardingIM\n", fakeBackspaceCount)

	for i := 0; i < fakeBackspaceCount; i++ {
		e.ForwardKeyEvent(IBusLeft, XkLeft-8, IBusShiftMask)
		e.ForwardKeyEvent(IBusLeft, XkLeft-8, IBusReleaseMask)
	}
	time.Sleep(time.Duration(fakeBackspaceCount) * (30 + BACKSPACE_INTERVAL) * time.Millisecond)
}

func (e *IBusBambooEngine) SendBackspaceBackspaceForwardingMode() {
	time.Sleep(30 * time.Millisecond)
	log.Printf("Sendding %d backspace via backspaceForwardingIM\n", fakeBackspaceCount)

	for i := 0; i < fakeBackspaceCount; i++ {
		e.ForwardKeyEvent(IBusBackSpace, XkBackspace-8, 0)
		e.ForwardKeyEvent(IBusBackSpace, XkBackspace-8, IBusReleaseMask)
	}
	time.Sleep(time.Duration(fakeBackspaceCount) * (30 + BACKSPACE_INTERVAL) * time.Millisecond)
}

func (e *IBusBambooEngine) resetFakeBackspace() {
	fakeBackspaceCount = 0
}
