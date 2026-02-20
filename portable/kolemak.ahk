;Created by rayshoo / github.com/rayshoo
#Requires AutoHotkey v2.0
SendMode("Input")
InstallKeybdHook()

; ============================================================
; Settings
; ============================================================

IniFile := A_ScriptDir "\Data\Save.ini"

SXCoordinate := IniRead(IniFile, "Coordinate", "SXCoordinate", "0")
SYCoordinate := IniRead(IniFile, "Coordinate", "SYCoordinate", "0")
CoordMode("ToolTip", "Screen")

CapsLockRemap := IniRead(IniFile, "Settings", "CapsLockRemap", "1") = "1"
SemicolonSwap := IniRead(IniFile, "Settings", "SemicolonSwap", "1") = "1"

Colemak := true
SetTimer(AlertColemak, -1)

; ============================================================
; Tray Menu
; ============================================================

if FileExist(A_ScriptDir "\..\assets\portable-icon.ico")
	TraySetIcon(A_ScriptDir "\..\assets\portable-icon.ico")
else if (A_IsCompiled)
	TraySetIcon(A_ScriptFullPath)
else
	TraySetIcon("main.cpl", 8)
A_IconTip := "Kolemak Portable"

A_TrayMenu.Delete()
A_TrayMenu.Add("CapsLock to Backspace", ToggleCapsLockRemap)
A_TrayMenu.Add("Semicolon Swap", ToggleSemicolonSwap)
A_TrayMenu.Add()
A_TrayMenu.AddStandard()

if (CapsLockRemap)
	A_TrayMenu.Check("CapsLock to Backspace")
if (SemicolonSwap)
	A_TrayMenu.Check("Semicolon Swap")

ToggleCapsLockRemap(*) {
	global CapsLockRemap, IniFile
	CapsLockRemap := !CapsLockRemap
	if (CapsLockRemap)
		A_TrayMenu.Check("CapsLock to Backspace")
	else
		A_TrayMenu.Uncheck("CapsLock to Backspace")
	SaveSetting("CapsLockRemap", CapsLockRemap ? "1" : "0")
}

ToggleSemicolonSwap(*) {
	global SemicolonSwap, IniFile
	SemicolonSwap := !SemicolonSwap
	if (SemicolonSwap)
		A_TrayMenu.Check("Semicolon Swap")
	else
		A_TrayMenu.Uncheck("Semicolon Swap")
	SaveSetting("SemicolonSwap", SemicolonSwap ? "1" : "0")
}

SaveSetting(key, value) {
	global IniFile
	dir := A_ScriptDir "\Data"
	if !DirExist(dir)
		DirCreate(dir)
	IniWrite(value, IniFile, "Settings", key)
}

; ============================================================
; Hotkey Definitions
; ============================================================

; QWERTY e -> Colemak f | Korean: e
e::OnKey("f", "e", false)
+e::OnKey("f", "e", true)
^e::OnModKey("^", "f")
!e::OnModKey("!", "f")
#e::OnModKey("#", "f")
+!e::OnModKey("+!", "f")
^+e::OnModKey("^+", "f")
^!e::OnModKey("^!", "f")
^!+e::OnModKey("^!+", "f")

; QWERTY r -> Colemak p | Korean: r
r::OnKey("p", "r", false)
+r::OnKey("p", "r", true)
^r::OnModKey("^", "p")
!r::OnModKey("!", "p")
#r::OnModKey("#", "p")
+!r::OnModKey("+!", "p")
^+r::OnModKey("^+", "p")
^!r::OnModKey("^!", "p")
^!+r::OnModKey("^!+", "p")

; QWERTY t -> Colemak g | Korean: t
t::OnKey("g", "t", false)
+t::OnKey("g", "t", true)
^t::OnModKey("^", "g")
!t::OnModKey("!", "g")
#t::OnModKey("#", "g")
+!t::OnModKey("+!", "g")
^+t::OnModKey("^+", "g")
^!t::OnModKey("^!", "g")
^!+t::OnModKey("^!+", "g")

; QWERTY y -> Colemak j | Korean: y
y::OnKey("j", "y", false)
+y::OnKey("j", "y", true)
^y::OnModKey("^", "j")
!y::OnModKey("!", "j")
#y::OnModKey("#", "j")
+!y::OnModKey("+!", "j")
^+y::OnModKey("^+", "j")
^!y::OnModKey("^!", "j")
^!+y::OnModKey("^!+", "j")

; QWERTY u -> Colemak l | Korean: u
u::OnKey("l", "u", false)
+u::OnKey("l", "u", true)
^u::OnModKey("^", "l")
!u::OnModKey("!", "l")
#u::OnModKey("#", "l")
+!u::OnModKey("+!", "l")
^+u::OnModKey("^+", "l")
^!u::OnModKey("^!", "l")
^!+u::OnModKey("^!+", "l")

; QWERTY i -> Colemak u | Korean: i
i::OnKey("u", "i", false)
+i::OnKey("u", "i", true)
^i::OnModKey("^", "u")
!i::OnModKey("!", "u")
#i::OnModKey("#", "u")
+!i::OnModKey("+!", "u")
^+i::OnModKey("^+", "u")
^!i::OnModKey("^!", "u")
^!+i::OnModKey("^!+", "u")

; QWERTY o -> Colemak y | Korean: o
o::OnKey("y", "o", false)
+o::OnKey("y", "o", true)
^o::OnModKey("^", "y")
!o::OnModKey("!", "y")
#o::OnModKey("#", "y")
+!o::OnModKey("+!", "y")
^+o::OnModKey("^+", "y")
^!o::OnModKey("^!", "y")
^!+o::OnModKey("^!+", "y")

; QWERTY p -> Colemak ; (SC027)
; SemicolonSwap ON:  always SC027 (ㅔ도 ; 위치로 이동)
; SemicolonSwap OFF: English SC027, Korean p (ㅔ 원래 자리)
p::HandlePKey(false)
+p::HandlePKey(true)
^p::OnModKey("^", "{SC027}")
!p::OnModKey("!", "{SC027}")
#p::OnModKey("#", "{SC027}")
+!p::OnModKey("+!", "{SC027}")
^+p::OnModKey("^+", "{SC027}")
^!p::OnModKey("^!", "{SC027}")
^!+p::OnModKey("^!+", "{SC027}")

; QWERTY s -> Colemak r | Korean: s
s::OnKey("r", "s", false)
+s::OnKey("r", "s", true)
^s::OnModKey("^", "r")
!s::OnModKey("!", "r")
#s::OnModKey("#", "r")
+!s::OnModKey("+!", "r")
^+s::OnModKey("^+", "r")
^!s::OnModKey("^!", "r")
^!+s::OnModKey("^!+", "r")

; QWERTY d -> Colemak s | Korean: d
d::OnKey("s", "d", false)
+d::OnKey("s", "d", true)
^d::OnModKey("^", "s")
!d::OnModKey("!", "s")
#d::OnModKey("#", "s")
+!d::OnModKey("+!", "s")
^+d::OnModKey("^+", "s")
^!d::OnModKey("^!", "s")
^!+d::OnModKey("^!+", "s")

; QWERTY f -> Colemak t | Korean: f
f::OnKey("t", "f", false)
+f::OnKey("t", "f", true)
^f::OnModKey("^", "t")
!f::OnModKey("!", "t")
#f::OnModKey("#", "t")
+!f::OnModKey("+!", "t")
^+f::OnModKey("^+", "t")
^!f::OnModKey("^!", "t")
^!+f::OnModKey("^!+", "t")

; QWERTY g -> Colemak d | Korean: g
g::OnKey("d", "g", false)
+g::OnKey("d", "g", true)
^g::OnModKey("^", "d")
!g::OnModKey("!", "d")
#g::OnModKey("#", "d")
+!g::OnModKey("+!", "d")
^+g::OnModKey("^+", "d")
^!g::OnModKey("^!", "d")
^!+g::OnModKey("^!+", "d")

; QWERTY j -> Colemak n | Korean: j
j::OnKey("n", "j", false)
+j::OnKey("n", "j", true)
^j::OnModKey("^", "n")
!j::OnModKey("!", "n")
#j::OnModKey("#", "n")
+!j::OnModKey("+!", "n")
^+j::OnModKey("^+", "n")
^!j::OnModKey("^!", "n")
^!+j::OnModKey("^!+", "n")

; QWERTY k -> Colemak e | Korean: k
k::OnKey("e", "k", false)
+k::OnKey("e", "k", true)
^k::OnModKey("^", "e")
!k::OnModKey("!", "e")
#k::OnModKey("#", "e")
+!k::OnModKey("+!", "e")
^+k::OnModKey("^+", "e")
^!k::OnModKey("^!", "e")
^!+k::OnModKey("^!+", "e")

; QWERTY l -> Colemak i | Korean: l
l::OnKey("i", "l", false)
+l::OnKey("i", "l", true)
^l::OnModKey("^", "i")
!l::OnModKey("!", "i")
#l::OnModKey("#", "i")
+!l::OnModKey("+!", "i")
^+l::OnModKey("^+", "i")
^!l::OnModKey("^!", "i")
^!+l::OnModKey("^!+", "i")

; QWERTY ; (SC027) -> Colemak o
; SemicolonSwap ON:  English o, Korean p (ㅔ가 여기로 이동)
; SemicolonSwap OFF: English o, Korean SC027 (; 원래 자리)
SC027::HandleSC027Key(false)
+SC027::HandleSC027Key(true)
^SC027::OnModKey("^", "o")
!SC027::OnModKey("!", "o")
#SC027::OnModKey("#", "o")
+!SC027::OnModKey("+!", "o")
^+SC027::OnModKey("^+", "o")
^!SC027::OnModKey("^!", "o")
^!+SC027::OnModKey("^!+", "o")

; QWERTY n -> Colemak k | Korean: n
n::OnKey("k", "n", false)
+n::OnKey("k", "n", true)
^n::OnModKey("^", "k")
!n::OnModKey("!", "k")
#n::OnModKey("#", "k")
+!n::OnModKey("+!", "k")
^+n::OnModKey("^+", "k")
^!n::OnModKey("^!", "k")
^!+n::OnModKey("^!+", "k")

#+m::#+m
#m::#m

; ============================================================
; Win+Space toggle (Colemak <-> QWERTY)
; ============================================================

#SuspendExempt
#space:: {
	global Colemak
	Suspend(-1)
	if (Colemak) {
		Colemak := false
		SetTimer(AlertColemak, 0)
		SetTimer(DisappearColemak, 0)
		SetTimer(AlertQwerty, -1)
	} else {
		Colemak := true
		SetTimer(AlertQwerty, 0)
		SetTimer(DisappearQwerty, 0)
		SetTimer(AlertColemak, -1)
	}
}
#SuspendExempt false

; ============================================================
; CapsLock remapping (conditional)
; ============================================================

#HotIf CapsLockRemap
CapsLock::BackSpace
+CapsLock::CapsLock
!CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("!{CapsLock}")
}
#CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("#{CapsLock}")
}
+!CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("+!{CapsLock}")
}
^+CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("^+{CapsLock}")
}
^!CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("^!{CapsLock}")
}
^!+CapsLock:: {
	SetStoreCapsLockMode("Off")
	Send("^!+{CapsLock}")
}
#HotIf

; ============================================================
; Tooltip Functions
; ============================================================

AlertColemak() {
	global SXCoordinate, SYCoordinate
	ToolTip("colemak", SXCoordinate, SYCoordinate, 1)
	SetTimer(DisappearColemak, -3000)
}

DisappearColemak() {
	ToolTip(, , , 1)
}

AlertQwerty() {
	global SXCoordinate, SYCoordinate
	ToolTip("qwerty", SXCoordinate, SYCoordinate, 1)
	SetTimer(DisappearQwerty, -3000)
}

DisappearQwerty() {
	ToolTip(, , , 1)
}

; ============================================================
; Helper Functions
; ============================================================

OnKey(colemakKey, koreanKey, isShifted) {
	ret := IME_CHECK("A")
	if (ret = 0) {
		Suspend(true)
		capsOn := GetKeyState("CapsLock", "T")
		if (isShifted) {
			if (capsOn)
				Send(Format("{:L}", colemakKey))
			else
				Send(Format("{:U}", colemakKey))
		} else {
			if (capsOn)
				Send(Format("{:U}", colemakKey))
			else
				Send(Format("{:L}", colemakKey))
		}
		Suspend(false)
	} else {
		Suspend(true)
		capsOn := GetKeyState("CapsLock", "T")
		if (isShifted) {
			if (capsOn) {
				SetKeyDelay(-1)
				Send("{Blind}+" . koreanKey)
			} else {
				Send("+" . koreanKey)
			}
		} else {
			if (capsOn) {
				SetKeyDelay(-1)
				Send("{Blind}" . koreanKey)
			} else {
				Send(koreanKey)
			}
		}
		Suspend(false)
	}
}

OnModKey(mod, colemakKey) {
	Suspend(true)
	Send(mod . colemakKey)
	Suspend(false)
}

HandlePKey(isShifted) {
	global SemicolonSwap
	if (SemicolonSwap) {
		; ㅔ도 ; 위치로 이동: always send SC027
		Suspend(true)
		if (isShifted)
			Send("+{SC027}")
		else
			Send("{SC027}")
		Suspend(false)
	} else {
		; ㅔ 원래 자리: English → SC027, Korean → p
		ret := IME_CHECK("A")
		if (ret = 0) {
			Suspend(true)
			if (isShifted)
				Send("+{SC027}")
			else
				Send("{SC027}")
			Suspend(false)
		} else {
			Suspend(true)
			capsOn := GetKeyState("CapsLock", "T")
			if (isShifted) {
				if (capsOn) {
					SetKeyDelay(-1)
					Send("{Blind}+p")
				} else {
					Send("+p")
				}
			} else {
				if (capsOn) {
					SetKeyDelay(-1)
					Send("{Blind}p")
				} else {
					Send("p")
				}
			}
			Suspend(false)
		}
	}
}

HandleSC027Key(isShifted) {
	global SemicolonSwap
	ret := IME_CHECK("A")
	if (ret = 0) {
		; English: always "o"
		Suspend(true)
		capsOn := GetKeyState("CapsLock", "T")
		if (isShifted) {
			if (capsOn)
				Send("o")
			else
				Send("O")
		} else {
			if (capsOn)
				Send("O")
			else
				Send("o")
		}
		Suspend(false)
	} else {
		if (SemicolonSwap) {
			; ㅔ가 여기로 이동: Korean → send p (ㅔ)
			Suspend(true)
			capsOn := GetKeyState("CapsLock", "T")
			if (isShifted) {
				if (capsOn) {
					SetKeyDelay(-1)
					Send("{Blind}+p")
				} else {
					Send("+p")
				}
			} else {
				if (capsOn) {
					SetKeyDelay(-1)
					Send("{Blind}p")
				} else {
					Send("p")
				}
			}
			Suspend(false)
		} else {
			; ; 원래 자리: Korean → send SC027
			Suspend(true)
			if (isShifted)
				Send("+{SC027}")
			else
				Send("{SC027}")
			Suspend(false)
		}
	}
}

; ============================================================
; IME Detection Functions
; ============================================================

IME_CHECK(WinTitle) {
	hWnd := WinGetID(WinTitle)
	return Send_ImeControl(ImmGetDefaultIMEWnd(hWnd), 0x001, "")
}

Send_ImeControl(DefaultIMEWnd, wParam, lParam) {
	DetectSave := A_DetectHiddenWindows
	DetectHiddenWindows(true)
	result := SendMessage(0x283, wParam, lParam, , "ahk_id " . DefaultIMEWnd)
	DetectHiddenWindows(DetectSave)
	return result
}

ImmGetDefaultIMEWnd(hWnd) {
	return DllCall("imm32\ImmGetDefaultIMEWnd", "Uint", hWnd, "Uint")
}
