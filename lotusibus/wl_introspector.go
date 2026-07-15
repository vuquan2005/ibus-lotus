package lotusibus

import (
	"encoding/json"
	"fmt"
	"os/exec"
	"strings"

	"github.com/godbus/dbus/v5"
)

type WindowInfo struct {
	ID    string
	Class string
}

func wlGetFocusWindowInfo() (WindowInfo, error) {
	if isGnome {
		return gnomeGetFocusWindowInfo()
	}
	if isKDE {
		return kdeGetFocusWindowInfo()
	}
	return WindowInfo{}, nil
}

func wlGetFocusWindowClass() (string, error) {
	info, err := wlGetFocusWindowInfo()
	if err != nil {
		return "", err
	}
	return info.Class, nil
}

func gnomeGetFocusWindowInfo() (WindowInfo, error) {
	// Install Focused Window extension to make this work
	conn, err := dbus.ConnectSessionBus()
	if err != nil {
		return WindowInfo{}, err
	}
	defer conn.Close()

	obj := conn.Object(
		"org.gnome.Shell",
		dbus.ObjectPath("/org/gnome/shell/extensions/FocusedWindow"),
	)

	var jsonStr string
	call := obj.Call("org.gnome.shell.extensions.FocusedWindow.Get", 0)
	if call.Err != nil {
		return WindowInfo{}, call.Err
	}

	if err := call.Store(&jsonStr); err != nil {
		return WindowInfo{}, err
	}

	var data struct {
		WmClass string      `json:"wm_class"`
		ID      interface{} `json:"id"`
	}
	if err := json.Unmarshal([]byte(jsonStr), &data); err != nil {
		return WindowInfo{}, err
	}

	idStr := ""
	if data.ID != nil {
		idStr = fmt.Sprintf("%v", data.ID)
	}

	return WindowInfo{
		ID:    idStr,
		Class: data.WmClass,
	}, nil
}

func gnomeGetFocusWindowClass() (string, error) {
	info, err := gnomeGetFocusWindowInfo()
	if err != nil {
		return "", err
	}
	return info.Class, nil
}

func kdeGetFocusWindowInfo() (WindowInfo, error) {
	var getActiveWindowCmd = exec.Command("kdotool", "getactivewindow")

	windowActiveId, err := getActiveWindowCmd.Output()
	if err != nil {
		return WindowInfo{}, err
	}

	id := strings.TrimSpace(string(windowActiveId))

	var getWindowClassnameCmd = exec.Command("kdotool", "getwindowclassname", id)
	windowClassname, err := getWindowClassnameCmd.Output()
	if err != nil {
		return WindowInfo{}, err
	}

	return WindowInfo{
		ID:    id,
		Class: strings.TrimSpace(string(windowClassname)),
	}, nil
}

func kdeGetFocusWindowClass() (string, error) {
	info, err := kdeGetFocusWindowInfo()
	if err != nil {
		return "", err
	}
	return info.Class, nil
}
