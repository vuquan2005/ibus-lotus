package config

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/user"
	"path/filepath"

	"github.com/BambooEngine/bamboo-core"
)

const (
	configDir        = "%s/.config/ibus-%s"
	configFile       = "%s/ibus-%s.config.json"
	mactabFile       = "%s/ibus-%s.macro.text"
	sampleMactabFile = "data/macro.tpl.txt"
)

type Shortcut struct {
	KeyVal   uint32 `json:"keyval"`
	Modifier uint32 `json:"modifier"`
}

type ShortcutMap map[string]Shortcut

func (sm *ShortcutMap) UnmarshalJSON(data []byte) error {
	defaultShortcuts := ShortcutMap{
		"InputModeSwitch":   {Modifier: 4, KeyVal: 32},
		"RestoreKeyStrokes": {Modifier: 0, KeyVal: 0},
		"ViEnSwitch":        {Modifier: 0, KeyVal: 0},
	}

	var m map[string]Shortcut
	if err := json.Unmarshal(data, &m); err == nil {
		*sm = defaultShortcuts
		for k, v := range m {
			(*sm)[k] = v
		}
		return nil
	}

	var arr []uint32
	if err := json.Unmarshal(data, &arr); err == nil {
		*sm = defaultShortcuts
		if len(arr) >= 2 {
			(*sm)["InputModeSwitch"] = Shortcut{Modifier: arr[0], KeyVal: arr[1]}
		}
		if len(arr) >= 4 {
			(*sm)["RestoreKeyStrokes"] = Shortcut{Modifier: arr[2], KeyVal: arr[3]}
		}
		if len(arr) >= 6 {
			(*sm)["ViEnSwitch"] = Shortcut{Modifier: arr[4], KeyVal: arr[5]}
		}
		return nil
	}

	return fmt.Errorf("failed to unmarshal Shortcuts: invalid format")
}

type Config struct {
	InputMethod            string
	InputMethodDefinitions map[string]bamboo.InputMethodDefinition
	OutputCharset          string
	Flags                  uint
	IBflags                uint
	Shortcuts              ShortcutMap
	DefaultInputMode       int
	InputModeMapping       map[string]int
	EnableHwndTracking     bool
	EnableWmClassTracking    bool
	EnableFocusToast       bool
	EnableAutoSwitch       bool
}

func (c *Config) GetFlatShortcuts() [10]uint32 {
	var out [10]uint32
	if s, ok := c.Shortcuts["InputModeSwitch"]; ok {
		out[0] = s.Modifier
		out[1] = s.KeyVal
	}
	if s, ok := c.Shortcuts["RestoreKeyStrokes"]; ok {
		out[2] = s.Modifier
		out[3] = s.KeyVal
	}
	if s, ok := c.Shortcuts["ViEnSwitch"]; ok {
		out[4] = s.Modifier
		out[5] = s.KeyVal
	}
	return out
}

func GetConfigDir(ngName string) string {
	u, err := user.Current()
	if err == nil {
		return fmt.Sprintf(configDir, u.HomeDir, "lotus")
	}
	return fmt.Sprintf(configDir, "~", "lotus")
}

func GetMacroPath(engineName string) string {
	return fmt.Sprintf(mactabFile, GetConfigDir(engineName), engineName)
}

func GetConfigPath(engineName string) string {
	return fmt.Sprintf(configFile, GetConfigDir(engineName), engineName)
}

func DefaultCfg() Config {
	return Config{
		InputMethod:            "Telex",
		OutputCharset:          "Unicode",
		InputMethodDefinitions: bamboo.GetInputMethodDefinitions(),
		Flags:                  bamboo.EstdFlags,
		IBflags:                IBstdFlags,
		Shortcuts: ShortcutMap{
			"InputModeSwitch":   {Modifier: 1, KeyVal: 32},
			"RestoreKeyStrokes": {Modifier: 0, KeyVal: 0},
			"ViEnSwitch":        {Modifier: 0, KeyVal: 0},
		},
		DefaultInputMode:      PreeditIM,
		InputModeMapping:      map[string]int{},
		EnableHwndTracking:    true,
		EnableWmClassTracking: true,
		EnableFocusToast:      true,
		EnableAutoSwitch:      true,
	}
}

func LoadConfig(engineName string) *Config {
	var c = DefaultCfg()
	if engineName == "lotusus" {
		c.DefaultInputMode = UsIM
		c.IBflags = IBUsStdFlags
		return &c
	}

	data, err := os.ReadFile(GetConfigPath(engineName))
	if err == nil {
		json.Unmarshal(data, &c)
	}

	return &c
}

func WriteFileAtomic(filename string, data []byte, perm os.FileMode) error {
	dir := filepath.Dir(filename)
	tempFile, err := os.CreateTemp(dir, "ibus-lotus-*.tmp")
	if err != nil {
		return err
	}
	tempName := tempFile.Name()
	success := false
	defer func() {
		if tempFile != nil {
			tempFile.Close()
		}
		if !success {
			os.Remove(tempName)
		}
	}()

	if _, err := tempFile.Write(data); err != nil {
		return err
	}
	if err := tempFile.Sync(); err != nil {
		return err
	}
	if err := tempFile.Close(); err != nil {
		tempFile = nil
		return err
	}
	tempFile = nil

	if err := os.Chmod(tempName, perm); err != nil {
		return err
	}

	if err := os.Rename(tempName, filename); err != nil {
		return err
	}
	success = true
	return nil
}

func SaveConfig(c *Config, engineName string) {
	data, err := json.MarshalIndent(c, "", "  ")
	if err != nil {
		return
	}

	err = WriteFileAtomic(fmt.Sprintf(configFile, GetConfigDir(engineName), engineName), data, 0644)
	if err != nil {
		log.Printf("[ERROR] Failed to save config: %v", err)
	}
}
