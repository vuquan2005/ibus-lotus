package config

const (
	PreeditIM = iota + 1
	SurroundingTextIM
	UsIM
)

var ImLookupTable = map[int]string{
	PreeditIM:         "Cấu hình mặc định (Pre-edit)",
	SurroundingTextIM: "Sửa lỗi gạch chân (Surrounding Text)",
	UsIM:              "Thêm vào danh sách loại trừ",
}

var ImBackspaceList = []int{
	SurroundingTextIM,
}

const (
	IBautoCommitWithVnNotMatch     uint = 1 << 0
	IBmacroEnabled                 uint = 1 << 1
	_IBautoCommitWithVnFullMatch   uint = 1 << 2 // deprecated
	_IBautoCommitWithVnWordBreak   uint = 1 << 3 // deprecated
	IBspellCheckEnabled            uint = 1 << 4
	IBautoNonVnRestore             uint = 1 << 5
	IBddFreeStyle                  uint = 1 << 6
	IBnoUnderline                  uint = 1 << 7
	IBspellCheckWithRules          uint = 1 << 8
	IBspellCheckWithDicts          uint = 1 << 9
	IBautoCommitWithDelay          uint = 1 << 10
	IBautoCommitWithMouseMovement  uint = 1 << 11
	_IBemojiDisabled               uint = 1 << 12 // deprecated
	IBpreeditElimination           uint = 1 << 13
	_IBinputModeLookupTableEnabled uint = 1 << 14 // deprecated
	IBautoCapitalizeMacro          uint = 1 << 15
	_IBimQuickSwitchEnabled        uint = 1 << 16 // deprecated
	_IBrestoreKeyStrokesEnabled    uint = 1 << 17 // deprecated
	IBstdFlags                          = IBspellCheckEnabled | IBspellCheckWithRules | IBautoNonVnRestore | IBddFreeStyle |
		IBautoCapitalizeMacro | IBnoUnderline
	IBUsStdFlags = 0
)
