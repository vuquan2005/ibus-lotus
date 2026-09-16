package lotusibus

import (
	"ibus-lotus/ui"
	"os"
	"strings"
)

const EngineName = "Lotus"

var (
	isWayland  = false
	isGnome    = false
	isKDE      = false
	isHyprland = false
	isSway     = false

	Embedded = false
	ShowGUI  = false
	Version  = ""
)

func init() {
	desktop := strings.ToLower(os.Getenv("XDG_CURRENT_DESKTOP"))
	if os.Getenv("WAYLAND_DISPLAY") != "" {
		isWayland = true
	}
	if hasGnome("XDG_CURRENT_DESKTOP") || hasGnome("DESKTOP_SESSION") || hasGnome("GDMSESSION") {
		isGnome = true
	}
	if desktop == "kde" || strings.Contains(desktop, "kde") {
		isKDE = true
	}
	if os.Getenv("HYPRLAND_INSTANCE_SIGNATURE") != "" || strings.Contains(desktop, "hyprland") {
		isHyprland = true
	}
	if os.Getenv("SWAYSOCK") != "" || strings.Contains(desktop, "sway") {
		isSway = true
	}

	ui.SetFocusInspector(func() string {
		info, err := GetFocusWindowInfo()
		if err == nil && info.Class != "" {
			return info.Class
		}
		return ""
	})
}

func hasGnome(env string) bool {
	return strings.Contains(strings.ToLower(os.Getenv(env)), "gnome")
}
