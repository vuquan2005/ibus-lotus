package lotusibus

import (
	"encoding/json"
	"fmt"
	"log"
	"os/exec"
	"strings"

	"github.com/godbus/dbus/v5"
)

type WindowInfo struct {
	ID    string
	Class string
}

func GetFocusWindowInfo() (WindowInfo, error) {
	return wlGetFocusWindowInfo()
}

func wlGetFocusWindowInfo() (WindowInfo, error) {
	if isGnome {
		info, err := gnomeGetFocusWindowInfo()
		if err == nil && info.Class != "" {
			return info, nil
		}
		if err != nil {
			log.Printf("[DEBUG] Failed to get GNOME focus window info: %v", err)
		}
	}
	if isKDE {
		info, err := kdeGetFocusWindowInfo()
		if err == nil && info.Class != "" {
			return info, nil
		}
		if err != nil {
			log.Printf("[DEBUG] Failed to get KDE focus window info: %v", err)
		}
	}
	if isHyprland {
		info, err := hyprlandGetFocusWindowInfo()
		if err == nil && info.Class != "" {
			return info, nil
		}
		if err != nil {
			log.Printf("[DEBUG] Failed to get Hyprland focus window info: %v", err)
		}
	}
	if isSway {
		info, err := swayGetFocusWindowInfo()
		if err == nil && info.Class != "" {
			return info, nil
		}
		if err != nil {
			log.Printf("[DEBUG] Failed to get Sway focus window info: %v", err)
		}
	}
	// Fallback to X11 xprop/xdotool if available
	return x11GetFocusWindowInfo()
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
		WmClass string          `json:"wm_class"`
		ID      json.RawMessage `json:"id"`
	}
	if err := json.Unmarshal([]byte(jsonStr), &data); err != nil {
		return WindowInfo{}, err
	}

	idStr := strings.Trim(string(data.ID), "\"")
	if idStr == "null" {
		idStr = ""
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

func hyprlandGetFocusWindowInfo() (WindowInfo, error) {
	cmd := exec.Command("hyprctl", "activewindow", "-j")
	out, err := cmd.Output()
	if err != nil {
		return WindowInfo{}, err
	}
	var data struct {
		Address      string `json:"address"`
		Class        string `json:"class"`
		InitialClass string `json:"initialClass"`
	}
	if err := json.Unmarshal(out, &data); err != nil {
		return WindowInfo{}, err
	}
	cls := data.Class
	if cls == "" {
		cls = data.InitialClass
	}
	return WindowInfo{
		ID:    data.Address,
		Class: strings.TrimSpace(cls),
	}, nil
}

func swayGetFocusWindowInfo() (WindowInfo, error) {
	cmd := exec.Command("swaymsg", "-t", "get_tree")
	out, err := cmd.Output()
	if err != nil {
		return WindowInfo{}, err
	}

	type node struct {
		ID               int64   `json:"id"`
		Focused          bool    `json:"focused"`
		AppID            *string `json:"app_id"`
		WindowProperties struct {
			Class string `json:"class"`
		} `json:"window_properties"`
		Nodes         []node `json:"nodes"`
		FloatingNodes []node `json:"floating_nodes"`
	}

	var root node
	if err := json.Unmarshal(out, &root); err != nil {
		return WindowInfo{}, err
	}

	var findFocused func(n node) (WindowInfo, bool)
	findFocused = func(n node) (WindowInfo, bool) {
		if n.Focused {
			cls := ""
			if n.AppID != nil && *n.AppID != "" {
				cls = *n.AppID
			} else {
				cls = n.WindowProperties.Class
			}
			return WindowInfo{
				ID:    fmt.Sprintf("%d", n.ID),
				Class: strings.TrimSpace(cls),
			}, true
		}
		for _, child := range n.Nodes {
			if res, ok := findFocused(child); ok {
				return res, true
			}
		}
		for _, child := range n.FloatingNodes {
			if res, ok := findFocused(child); ok {
				return res, true
			}
		}
		return WindowInfo{}, false
	}

	info, found := findFocused(root)
	if !found {
		return WindowInfo{}, fmt.Errorf("no focused window found in sway tree")
	}
	return info, nil
}

func x11GetFocusWindowInfo() (WindowInfo, error) {
	cmd := exec.Command("xdotool", "getactivewindow")
	out, err := cmd.Output()
	if err != nil {
		return WindowInfo{}, err
	}
	id := strings.TrimSpace(string(out))
	if id == "" {
		return WindowInfo{}, fmt.Errorf("empty window id")
	}

	cmdClass := exec.Command("xprop", "-id", id, "WM_CLASS")
	outClass, err := cmdClass.Output()
	if err != nil {
		return WindowInfo{ID: id}, nil
	}
	// Format: WM_CLASS(STRING) = "instance", "Class"
	str := string(outClass)
	parts := strings.Split(str, "=")
	if len(parts) >= 2 {
		val := strings.TrimSpace(parts[1])
		classes := strings.Split(val, ",")
		if len(classes) > 0 {
			cls := strings.Trim(strings.TrimSpace(classes[len(classes)-1]), "\"")
			return WindowInfo{ID: id, Class: cls}, nil
		}
	}
	return WindowInfo{ID: id}, nil
}
