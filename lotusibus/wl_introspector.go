package lotusibus

import (
	"encoding/json"
	"os/exec"
	"strings"

	"github.com/godbus/dbus/v5"
)

func wlGetFocusWindowClass() (string, error) {
	if isGnome {
		return gnomeGetFocusWindowClass()
	}
	if isKDE {
		return kdeGetFocusWindowClass()
	}
	return "", nil
}

func gnomeGetFocusWindowClass() (string, error) {
	// Install Focused Window extension to make this work
	conn, err := dbus.ConnectSessionBus()
	if err != nil {
		return "", err
	}
	defer conn.Close()

	obj := conn.Object(
		"org.gnome.Shell",
		dbus.ObjectPath("/org/gnome/shell/extensions/FocusedWindow"),
	)

	var jsonStr string
	call := obj.Call("org.gnome.shell.extensions.FocusedWindow.Get", 0)
	if call.Err != nil {
		return "", call.Err
	}

	if err := call.Store(&jsonStr); err != nil {
		return "", err
	}

	var data struct {
		WmClass string `json:"wm_class"`
	}
	if err := json.Unmarshal([]byte(jsonStr), &data); err != nil {
		return "", err
	}

	return data.WmClass, nil
}

func kdeGetFocusWindowClass() (string, error) {
	var getActiveWindowCmd = exec.Command("kdotool", "getactivewindow")

	windowActiveId, err := getActiveWindowCmd.Output()
	if err != nil {
		return "", err
	}

	var getWindowClassnameCmd = exec.Command("kdotool", "getwindowclassname", strings.TrimSpace(string(windowActiveId)))
	windowClassname, err := getWindowClassnameCmd.Output()
	if err != nil {
		return "", err
	}

	return string(windowClassname), nil
}
