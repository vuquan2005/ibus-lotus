package config

const (
	PreeditIM = iota + 1
	SurroundingTextIM
	UsIM
)

var ImLookupTable = map[int]string{
	PreeditIM:             "Cấu hình mặc định (Pre-edit)",
	SurroundingTextIM:     "Sửa lỗi gạch chân (Surrounding Text)",
	UsIM:                  "Thêm vào danh sách loại trừ",
}

var ImBackspaceList = []int{
	SurroundingTextIM,
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
