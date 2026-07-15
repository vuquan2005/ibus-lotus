package ui

/*
#cgo pkg-config: gtk+-3.0
#include <gtk/gtk.h>

extern int openGUI(
	guint flags,
	guint ibFlags,
	int mode,
	guint32 *s,
	int size,
	char *mtext,
	char *cfgtext,
	char *curIM,
	char *curCS,
	char *allIMs,
	char *allCSs,
	int enableHwnd,
	int enableWmClass,
	int enableToast
);
*/
import "C"
import (
	"encoding/json"
	"ibus-lotus/config"
	"os"
	"strings"
	"unsafe"

	"github.com/BambooEngine/bamboo-core"
)

var engineName string

//export saveFlags
func saveFlags(flags C.guint) {
	var (
		cfg = config.LoadConfig(engineName)
	)
	config.SaveConfig(cfg, engineName)
	cfg.IBflags = uint(flags)
	config.SaveConfig(cfg, engineName)
}

//export saveConfigOptions
func saveConfigOptions(flags C.uint, ibFlags C.uint, inputMethod *C.char, outputCharset *C.char) {
	var (
		cfg = config.LoadConfig(engineName)
	)
	cfg.Flags = uint(flags)
	cfg.IBflags = uint(ibFlags)
	cfg.InputMethod = C.GoString(inputMethod)
	cfg.OutputCharset = C.GoString(outputCharset)
	config.SaveConfig(cfg, engineName)
}

//export saveTrackingOptions
func saveTrackingOptions(enableHwnd C.int, enableWmClass C.int, enableToast C.int) {
	var (
		cfg = config.LoadConfig(engineName)
	)
	cfg.EnableHwndTracking = (enableHwnd != 0)
	cfg.EnableWmClassTracking = (enableWmClass != 0)
	cfg.EnableFocusToast = (enableToast != 0)
	config.SaveConfig(cfg, engineName)
}

//export saveConfigText
func saveConfigText(text *C.char) {
	var (
		cfgText = C.GoString(text)
		cfgFn   = config.GetConfigPath(engineName)
	)
	err := config.WriteFileAtomic(cfgFn, []byte(cfgText), 0644)
	if err != nil {
		panic(err)
	}
}

//export saveMacroText
func saveMacroText(text *C.char) {
	var (
		macroText = C.GoString(text)
		macroFP   = config.GetMacroPath(engineName)
	)
	err := config.WriteFileAtomic(macroFP, []byte(macroText), 0644)
	if err != nil {
		panic(err)
	}
}

//export saveInputMode
func saveInputMode(mode int) {
	var (
		cfg = config.LoadConfig(engineName)
	)
	cfg.DefaultInputMode = mode
	config.SaveConfig(cfg, engineName)
}

//export saveShortcuts
func saveShortcuts(ptr *C.guint32, length int) {
	var (
		cfg = config.LoadConfig(engineName)
	)
	codes := makeSliceFromPtr(ptr, length)
	if cfg.Shortcuts == nil {
		cfg.Shortcuts = make(config.ShortcutMap)
	}
	cfg.Shortcuts["InputModeSwitch"] = config.Shortcut{Modifier: codes[0], KeyVal: codes[1]}
	cfg.Shortcuts["RestoreKeyStrokes"] = config.Shortcut{Modifier: codes[2], KeyVal: codes[3]}
	cfg.Shortcuts["ViEnSwitch"] = config.Shortcut{Modifier: codes[4], KeyVal: codes[5]}
	config.SaveConfig(cfg, engineName)
}

func makeSliceFromPtr(ptr *C.guint32, size int) [10]uint32 {
	var out [10]uint32
	slice := (*[1 << 28]C.guint32)(unsafe.Pointer(ptr))[:size:size]
	for i, elem := range slice[:size] {
		out[i] = uint32(elem)
	}
	return out
}

func boolToInt(b bool) C.int {
	if b {
		return 1
	}
	return 0
}

func OpenGUI(engName string) {
	engineName = engName
	var (
		cfg           = config.LoadConfig(engineName)
		flatShortcuts = cfg.GetFlatShortcuts()
		shortcuts     = flatShortcuts[:]
		s             = (*C.guint32)(&shortcuts[0])
		size          = len(shortcuts)
		macroFilePath = config.GetMacroPath(engineName)
	)
	mText, err := os.ReadFile(macroFilePath)
	if err != nil {
		panic(err)
	}
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		panic(err)
	}

	var ims []string
	for name := range cfg.InputMethodDefinitions {
		ims = append(ims, name)
	}
	allInputMethods := strings.Join(ims, ",")
	allOutputCharsets := strings.Join(bamboo.GetCharsetNames(), ",")

	os.Setenv("GTK_IM_MODULE", "gtk-im-context-simple")
	C.openGUI(
		C.guint(cfg.Flags),
		C.guint(cfg.IBflags),
		C.int(cfg.DefaultInputMode),
		s,
		C.int(size),
		C.CString(string(mText)),
		C.CString(string(data)),
		C.CString(cfg.InputMethod),
		C.CString(cfg.OutputCharset),
		C.CString(allInputMethods),
		C.CString(allOutputCharsets),
		boolToInt(cfg.EnableHwndTracking),
		boolToInt(cfg.EnableWmClassTracking),
		boolToInt(cfg.EnableFocusToast),
	)
}
