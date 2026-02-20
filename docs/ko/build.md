# 빌드 가이드

Windows에 개발 환경을 처음부터 구성하고 Kolemak IME를 빌드하는 방법을 설명합니다.

## 목차

- [1. 개발 환경 구성](#1-개발-환경-구성)
  - [Git 설치](#git-설치)
  - [Visual Studio 설치](#visual-studio-설치)
  - [CMake 설치](#cmake-설치)
  - [GNU Make 설치](#gnu-make-설치)
- [2. 소스 코드 받기](#2-소스-코드-받기)
- [3. DLL 빌드](#3-dll-빌드)
- [4. 개발자 설치 (regsvr32)](#4-개발자-설치-regsvr32)
- [5. 인스톨러 생성](#5-인스톨러-생성)
  - [Inno Setup 설치](#inno-setup-설치)
  - [인스톨러 빌드](#인스톨러-빌드)
- [6. 문제 해결](#6-문제-해결)

---

## 1. 개발 환경 구성

### Git 설치

소스 코드를 받기 위해 Git이 필요합니다.

1. [git-scm.com](https://git-scm.com/download/win)에서 **Git for Windows** 다운로드
2. 설치 시 기본 옵션 유지 (Git Bash 포함)
3. 설치 확인:
   ```bash
   git --version
   ```

### Visual Studio 설치

C 컴파일러(MSVC)와 Windows SDK가 필요합니다.

1. [visualstudio.microsoft.com](https://visualstudio.microsoft.com/ko/downloads/)에서 **Visual Studio 2022 Community** 다운로드 (무료)
2. 설치 시 다음 워크로드를 선택:
   - **C++를 사용한 데스크톱 개발** (Desktop development with C++)
3. 이 워크로드에 다음이 포함됩니다:
   - MSVC 컴파일러
   - Windows SDK
   - CMake (Visual Studio 번들)

> Visual Studio 2019도 사용 가능합니다. 2019를 사용하는 경우 빌드 시 `CMAKE_GEN` 변수를 변경해야 합니다:
> ```bash
> make build CMAKE_GEN="Visual Studio 16 2019"
> ```

### CMake 설치

Visual Studio 설치 시 포함되는 CMake를 사용할 수 있지만, 명령줄에서 직접 사용하려면 별도 설치가 편리합니다.

1. [cmake.org/download](https://cmake.org/download/)에서 **Windows x64 Installer** 다운로드
2. 설치 시 **Add CMake to the system PATH** 옵션 선택
3. 설치 확인:
   ```bash
   cmake --version
   ```

> Visual Studio에 포함된 CMake만 사용하는 경우 이 단계를 건너뛸 수 있습니다.

### GNU Make 설치

Makefile 기반 빌드를 위해 GNU Make가 필요합니다. 여러 방법 중 하나를 선택합니다:

**방법 A: winget (권장)**

```bash
winget install GnuWin32.Make
```

설치 후 `C:\Program Files (x86)\GnuWin32\bin`을 시스템 PATH에 추가합니다.

**방법 B: Chocolatey**

```bash
choco install make
```

**방법 C: MSYS2 / Git Bash**

MSYS2를 사용 중이라면:
```bash
pacman -S make
```

설치 확인:
```bash
make --version
```

> Make 없이 빌드하려면 [CMake 직접 사용](#cmake-직접-사용) 섹션을 참고하세요.

---

## 2. 소스 코드 받기

```bash
git clone https://github.com/rayshoo/kolemak.git
cd kolemak
```

---

## 3. DLL 빌드

### Makefile 사용 (권장)

64비트 + 32비트 DLL을 동시에 빌드합니다:

```bash
make build
```

개별 아키텍처 빌드:

```bash
make build64    # 64비트만
make build32    # 32비트만
```

빌드 결과물:
- `build/Release/kolemak.dll` (64비트)
- `build32/Release/kolemak.dll` (32비트)

### CMake 직접 사용

Make를 설치하지 않은 경우 CMake를 직접 사용할 수 있습니다:

```cmd
rem 64비트 빌드
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cd ..

rem 32비트 빌드
mkdir build32
cd build32
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
cd ..
```

### 빌드 정리

```bash
make clean
```

---

## 4. 개발자 설치 (regsvr32)

빌드된 DLL을 직접 등록하여 테스트할 수 있습니다. **관리자 권한 명령 프롬프트**에서 실행합니다.

### 설치

```bash
make install
```

또는 수동으로:

```cmd
regsvr32 "C:\...\build\Release\kolemak.dll"
regsvr32 "C:\...\build32\Release\kolemak.dll"
```

등록 후:
1. 최초 설치 시 **재부팅**이 필요합니다 (CapsLock 키 리매핑 적용)
2. **Windows 설정 > 시간 및 언어 > 언어 및 지역 > 한국어 > 키보드**에서 "Kolemak IME"를 선택

### 제거

```bash
make uninstall
```

### DLL 업데이트 시 주의사항

IME DLL은 시스템의 모든 프로세스에 로드됩니다. DLL을 업데이트하려면:

1. `make uninstall` 실행
2. **로그아웃 후 다시 로그인** (DLL 잠금 해제)
3. `make build` 실행
4. `make install` 실행

> DLL이 잠겨 있으면 링크 에러(LNK1104)가 발생합니다. 이 경우 로그아웃/로그인이 필요합니다.

---

## 5. 인스톨러 생성

일반 사용자에게 배포할 `kolemak-install.exe` 인스톨러를 생성합니다.

### Inno Setup 설치

1. [jrsoftware.org/isdl.php](https://jrsoftware.org/isdl.php)에서 **Inno Setup 6** 다운로드
2. 설치 시 기본 옵션 유지
3. 명령줄에서 `iscc`를 사용하려면 Inno Setup 설치 경로를 시스템 PATH에 추가:
   - 기본 경로: `C:\Program Files (x86)\Inno Setup 6`

설치 확인:
```bash
iscc /?
```

### 인스톨러 빌드

DLL이 먼저 빌드되어 있어야 합니다:

```bash
make build       # DLL 빌드 (아직 안 했다면)
make installer   # 인스톨러 생성
```

결과물: `dist/kolemak-install.exe`

### 인스톨러 동작

인스톨러가 수행하는 작업:

| 설치 시 | 제거 시 |
|---------|---------|
| DLL을 `Program Files\Kolemak\`에 복사 | DLL 등록 해제 (`regsvr32 /u`) |
| DLL 등록 (`regsvr32`) | 파일 삭제 |
| CapsLock 키 리매핑 (Scancode Map) | CapsLock 리매핑 복원 |
| 재부팅 여부 확인 | 재부팅 여부 확인 |

### GUI로 인스톨러 빌드

명령줄 대신 Inno Setup GUI를 사용할 수도 있습니다:

1. Inno Setup Compiler 실행
2. **File > Open** → `installer/kolemak.iss` 선택
3. **Build > Compile** (또는 `Ctrl+F9`)
4. 결과물이 `dist/kolemak-install.exe`에 생성됩니다

---

## 6. 문제 해결

### `cmake`을 찾을 수 없음

CMake가 PATH에 없는 경우. Visual Studio의 **Developer Command Prompt** 또는 **Developer PowerShell**에서 실행하면 자동으로 경로가 설정됩니다.

### LNK1104: 'kolemak.dll' 파일을 열 수 없습니다

IME DLL이 시스템에 의해 잠겨 있습니다:
1. `make uninstall` 실행
2. 로그아웃 후 다시 로그인
3. 다시 빌드

### `make`을 찾을 수 없음

GNU Make가 설치되지 않았거나 PATH에 없습니다. [GNU Make 설치](#gnu-make-설치) 섹션을 참고하거나, [CMake 직접 사용](#cmake-직접-사용) 방법을 사용하세요.

### `iscc`를 찾을 수 없음

Inno Setup이 설치되지 않았거나 PATH에 없습니다. Inno Setup 설치 경로(기본: `C:\Program Files (x86)\Inno Setup 6`)를 시스템 PATH에 추가하세요.

### C4819 경고 (코드 페이지)

소스 코드에 한국어 주석이 포함되어 있어 발생하는 경고입니다. 빌드에 영향을 주지 않으므로 무시해도 됩니다.
