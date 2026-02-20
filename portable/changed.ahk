;Created by rayshoo / github.com/rayshoo
#Requires AutoHotkey v2.0
SendMode("Input")
InstallKeybdHook()

TraySetIcon("main.cpl", 8)

SXCoordinate := IniRead("Data\Save.ini", "Coordinate", "SXCoordinate", "0")
SYCoordinate := IniRead("Data\Save.ini", "Coordinate", "SYCoordinate", "0")
CoordMode("ToolTip", "Screen")

Colemak := true
SetTimer(AlertColemak, -1)

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

; QWERTY p -> Colemak ; (SC027) | Korean: swapped (sends SC027 always)
; "changed" version: p always sends SC027 (no IME check needed since
; both English ; and Korean ㅔ are moved to the p position)
p::OnSC027Key("")
+p::OnSC027Key("+")
^p::OnSC027Key("^")
!p::OnSC027Key("!")
#p::OnSC027Key("#")
+!p::OnSC027Key("+!")
^+p::OnSC027Key("^+")
^!p::OnSC027Key("^!")
^!+p::OnSC027Key("^!+")

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

; QWERTY ; (SC027) -> Colemak o | Korean: swapped (sends p for ㅔ)
SC027::OnKeyWithSC("o", "p", false)
+SC027::OnKeyWithSC("o", "p", true)
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
; Win+Space toggle
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
; CapsLock remapping
; ============================================================

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

OnSC027Key(mod) {
	Suspend(true)
	Send(mod . "{SC027}")
	Suspend(false)
}

OnKeyWithSC(letterKey, koreanKey, isShifted) {
	ret := IME_CHECK("A")
	if (ret = 0) {
		Suspend(true)
		capsOn := GetKeyState("CapsLock", "T")
		if (isShifted) {
			if (capsOn)
				Send(Format("{:L}", letterKey))
			else
				Send(Format("{:U}", letterKey))
		} else {
			if (capsOn)
				Send(Format("{:U}", letterKey))
			else
				Send(Format("{:L}", letterKey))
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
