package config

const (
	PreeditIM = iota + 1
	SurroundingTextIM
	BackspaceForwardingIM
	ShiftLeftForwardingIM
	UsIM
)

var ImLookupTable = map[int]string{
	PreeditIM:             "Cấu hình mặc định (Pre-edit)",
	SurroundingTextIM:     "Sửa lỗi gạch chân (Surrounding Text)",
	BackspaceForwardingIM: "Sửa lỗi gạch chân (ForwardKeyEvent I)",
	ShiftLeftForwardingIM: "Sửa lỗi gạch chân (ForwardKeyEvent II)",
	UsIM:                  "Thêm vào danh sách loại trừ",
}

var ImBackspaceList = []int{
	SurroundingTextIM,
	BackspaceForwardingIM,
	ShiftLeftForwardingIM,
}

const (
	IBautoCommitWithVnNotMatch uint = 1 << iota
	IBmacroEnabled
	_IBautoCommitWithVnFullMatch //deprecated
	_IBautoCommitWithVnWordBreak //deprecated
	IBspellCheckEnabled
	IBautoNonVnRestore
	IBddFreeStyle
	IBnoUnderline
	IBspellCheckWithRules
	IBspellCheckWithDicts
	IBautoCommitWithDelay
	IBautoCommitWithMouseMovement
	_IBemojiDisabled //deprecated
	IBpreeditElimination
	_IBinputModeLookupTableEnabled //deprecated
	IBautoCapitalizeMacro
	_IBimQuickSwitchEnabled     //deprecated
	_IBrestoreKeyStrokesEnabled //deprecated
	IBstdFlags                  = IBspellCheckEnabled | IBspellCheckWithRules | IBautoNonVnRestore | IBddFreeStyle |
		IBautoCapitalizeMacro | IBnoUnderline
	IBUsStdFlags = 0
)
