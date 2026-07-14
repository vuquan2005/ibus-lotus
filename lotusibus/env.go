package lotusibus

import (
	"os"
	"strings"
)

const EngineName = "Lotus"

var (
	isWayland = false
	isGnome   = false
	isKDE     = false

	Embedded = false
	ShowGUI  = false
	Version  = ""
)

func init() {
	if os.Getenv("WAYLAND_DISPLAY") != "" {
		isWayland = true
	}
	if hasGnome("XDG_CURRENT_DESKTOP") || hasGnome("DESKTOP_SESSION") || hasGnome("GDMSESSION") {
		isGnome = true
	}
	if strings.ToLower(os.Getenv("XDG_CURRENT_DESKTOP")) == "kde" {
		isKDE = true
	}
}

func hasGnome(env string) bool {
	return strings.Contains(strings.ToLower(os.Getenv(env)), "gnome")
}
