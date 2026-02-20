# Kolemak

Windows에서 두벌식 한글 입력을 지원하는 Colemak 자판 입력기입니다. Windows TSF(Text Services Framework) 기반의 네이티브 IME로, 별도의 OS 레이아웃 변경 없이 Colemak 영문과 두벌식 한글을 모두 지원합니다.

## 특징

- **네이티브 IME** — Windows 입력기 프레임워크에 직접 통합되어, 모든 애플리케이션에서 안정적으로 동작합니다
- **Colemak 영문 + 두벌식 한글** — 한/영 전환: `한글 키` 또는 `우측 Alt`
- **Colemak / QWERTY 즉시 전환** — `Ctrl+Space`로 전환, 다른 사용자와 컴퓨터를 공유할 때 편리합니다
- **CapsLock → Backspace** — CapsLock이 Backspace로 동작, `Shift+CapsLock`으로 실제 CapsLock 토글
- **Language Bar 통합** — 작업 표시줄 언어 영역에서 우클릭 메뉴로 Colemak 모드, CapsLock 설정 전환
- **시스템 트레이 설정** — 트레이 아이콘 우클릭으로 설정 변경 (CapsLock 동작, ㅔ 키 위치, 전환 단축키)
- **설정 자동 저장** — CapsLock, ㅔ 키 위치, 단축키 설정이 레지스트리에 저장되어 재시작 후에도 유지 (Colemak 모드는 항상 기본으로 시작)

## 요구 사항

- Windows 10 이상

## 설치

### 인스톨러 사용 (권장)

[Releases](https://github.com/rayshoo/kolemak/releases) 페이지에서 `kolemak-install.exe`를 다운로드하고 실행합니다.

1. 인스톨러를 실행합니다 (관리자 권한 필요)
2. 설치 완료 후 **재부팅**합니다 (CapsLock 키 리매핑 적용)
3. **설정 > 시간 및 언어 > 언어 및 지역 > 한국어 > 언어 옵션 > 키보드**에서:
   - **키보드 추가** → "Kolemak IME" 추가
   - Microsoft 입력기 **제거** (권장 — 입력기 전환 없이 바로 사용 가능)

### 제거

**Windows 설정 > 앱 > 설치된 앱**에서 "Kolemak IME"를 제거합니다. 또는 인스톨러의 언인스톨러를 실행합니다.

### 직접 빌드하여 설치

개발자라면 소스에서 직접 빌드할 수 있습니다. [빌드 가이드](./build.md)를 참고하세요.

## 사용법

### 한/영 전환

`한글 키` 또는 `우측 Alt`를 눌러 한글/영문 입력을 전환합니다.

### Colemak / QWERTY 전환

<kbd>Ctrl</kbd>+<kbd>Space</kbd>를 눌러 Colemak과 QWERTY 레이아웃을 전환합니다. 전환 단축키는 설정에서 변경할 수 있습니다.

### CapsLock

| 단축키 | 동작 |
|--------|------|
| `CapsLock` | Backspace |
| `Shift+CapsLock` | CapsLock 토글 |

> 설정에서 CapsLock → Backspace 기능을 끌 수 있습니다.

### 설정

시스템 트레이의 Kolemak 아이콘을 우클릭하고 **설정**을 선택합니다.

| 설정 항목 | 설명 |
|-----------|------|
| CapsLock → Backspace | CapsLock을 Backspace로 사용 |
| ㅔ 키를 ; 위치로 변경 | 아래 [ㅔ 키 위치](#ㅔ-키-위치) 참조 |
| Colemak/QWERTY 전환 단축키 | 기본: `Ctrl+Space` |

## ㅔ 키 위치

Colemak 배열에서는 QWERTY의 `P` 키가 `;`으로 바뀝니다. 두벌식에서 `P` 키는 `ㅔ`이므로 충돌이 발생합니다.

| 설정 | 영문 `;` 위치 | 한글 `ㅔ` 위치 |
|------|-------------|--------------|
| **켬** (Changed) | Colemak `;`이 `P` 키에 위치 | `ㅔ`가 `P` 키로 이동 |
| **끔** (Unchanged) | Colemak `;`이 `P` 키에 위치 | `ㅔ`가 기존 `;` 키에 유지 |

- **켬** — `ㅔ`를 Colemak의 `;` 위치(`P` 키)로 이동합니다. 영문과 한글 모두 통일된 레이아웃을 원할 때.
- **끔** — `ㅔ`를 원래의 물리적 키(`;` 키)에 유지합니다. 한글 입력을 기존 QWERTY 키보드와 동일하게 사용하려면.

## 키 매핑

Colemak 모드에서 다음 QWERTY 키가 재매핑됩니다. 그 외의 키는 변경되지 않습니다.

| QWERTY | Colemak | 한글 (패스스루) |
|--------|---------|---------------|
| `E` | `F` | `ㄷ` / `ㄸ` |
| `R` | `P` | `ㄱ` / `ㄲ` |
| `T` | `G` | `ㅅ` / `ㅆ` |
| `Y` | `J` | `ㅛ` |
| `U` | `L` | `ㅕ` |
| `I` | `U` | `ㅑ` |
| `O` | `Y` | `ㅐ` / `ㅒ` |
| `P` | `;` | [ㅔ 키 위치](#ㅔ-키-위치) 참조 |
| `S` | `R` | `ㄴ` |
| `D` | `S` | `ㅇ` |
| `F` | `T` | `ㄹ` |
| `G` | `D` | `ㅎ` |
| `J` | `N` | `ㅗ` |
| `K` | `E` | `ㅏ` |
| `L` | `I` | `ㅣ` |
| `;` | `O` | [ㅔ 키 위치](#ㅔ-키-위치) 참조 |
| `N` | `K` | `ㅜ` |

## 포터블 버전 (AutoHotkey)

설치 없이 실행 파일 하나로 사용할 수 있는 포터블 버전도 제공합니다. 외부 컴퓨터에서 임시로 사용하거나 관리자 권한 없이 사용해야 할 때 유용합니다.

### 포터블 버전 특징

- **설치 불필요** — `.exe` 파일 하나만 실행하면 바로 사용 가능
- **관리자 권한 불필요** — USB에 넣어서 어디서든 사용 가능
- **Microsoft 한글 입력기 필요** — Windows 기본 한국어 입력기와 함께 동작
- **트레이 메뉴 설정** — CapsLock → Backspace, Semicolon Swap을 트레이 메뉴에서 토글 (설정은 INI 파일에 저장)

### 포터블 버전 제한 사항

- AutoHotkey 기반으로 키 입력을 가로채는 방식이므로, CPU 부하가 높거나 빠르게 연타할 경우 간헐적으로 QWERTY 키가 입력될 수 있습니다
- 일부 애플리케이션이나 입력 필드에서 재매핑된 키가 동작하지 않을 수 있습니다

### 포터블 버전 다운로드

[Releases](https://github.com/rayshoo/kolemak/releases) 페이지에서 `kolemak-portable.exe`를 다운로드합니다.

### 포터블 버전 사용법

1. 다운로드한 `kolemak-portable.exe`를 실행합니다
2. `Win+Space`로 Colemak / QWERTY 전환
3. 트레이 아이콘 우클릭으로 설정 변경:
   - **CapsLock to Backspace** — CapsLock → Backspace 토글
   - **Semicolon Swap** — ㅔ 키 위치 변경 토글 (IME 버전의 "ㅔ 키를 ; 위치로 변경"과 동일)
4. 종료: 트레이 아이콘 우클릭 → Exit

## 라이선스

[GNU General Public License v3.0](../../LICENSE)
