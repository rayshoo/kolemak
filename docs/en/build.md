# Build Guide

This guide covers setting up a development environment from scratch on Windows and building the Kolemak IME.

## Table of Contents

- [1. Development Environment Setup](#1-development-environment-setup)
  - [Install Git](#install-git)
  - [Install Visual Studio](#install-visual-studio)
  - [Install CMake](#install-cmake)
  - [Install GNU Make](#install-gnu-make)
- [2. Get the Source Code](#2-get-the-source-code)
- [3. Build the DLL](#3-build-the-dll)
- [4. Developer Install (regsvr32)](#4-developer-install-regsvr32)
- [5. Create the Installer](#5-create-the-installer)
  - [Install Inno Setup](#install-inno-setup)
  - [Build the Installer](#build-the-installer)
- [6. Troubleshooting](#6-troubleshooting)

---

## 1. Development Environment Setup

### Install Git

Git is required to clone the source code.

1. Download **Git for Windows** from [git-scm.com](https://git-scm.com/download/win)
2. Install with default options (includes Git Bash)
3. Verify installation:
   ```bash
   git --version
   ```

### Install Visual Studio

The MSVC C compiler and Windows SDK are required.

1. Download **Visual Studio 2022 Community** (free) from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
2. During installation, select the following workload:
   - **Desktop development with C++**
3. This workload includes:
   - MSVC compiler
   - Windows SDK
   - CMake (bundled with Visual Studio)

> Visual Studio 2019 is also supported. If using 2019, change the `CMAKE_GEN` variable when building:
> ```bash
> make build CMAKE_GEN="Visual Studio 16 2019"
> ```

### Install CMake

You can use the CMake bundled with Visual Studio, but installing it separately makes command-line usage easier.

1. Download the **Windows x64 Installer** from [cmake.org/download](https://cmake.org/download/)
2. During installation, select **Add CMake to the system PATH**
3. Verify installation:
   ```bash
   cmake --version
   ```

> You can skip this step if you only use the CMake bundled with Visual Studio.

### Install GNU Make

GNU Make is required for Makefile-based builds. Choose one of the following methods:

**Option A: winget (recommended)**

```bash
winget install GnuWin32.Make
```

After installation, add `C:\Program Files (x86)\GnuWin32\bin` to your system PATH.

**Option B: Chocolatey**

```bash
choco install make
```

**Option C: MSYS2 / Git Bash**

If you're using MSYS2:
```bash
pacman -S make
```

Verify installation:
```bash
make --version
```

> To build without Make, see the [Using CMake Directly](#using-cmake-directly) section.

---

## 2. Get the Source Code

```bash
git clone https://github.com/rayshoo/colemak_korean.git
cd colemak_korean
```

---

## 3. Build the DLL

### Using Makefile (recommended)

Builds both 64-bit and 32-bit DLLs:

```bash
make build
```

Build individual architectures:

```bash
make build64    # 64-bit only
make build32    # 32-bit only
```

Build outputs:
- `build/Release/kolemak.dll` (64-bit)
- `build32/Release/kolemak.dll` (32-bit)

### Using CMake Directly

If you haven't installed Make, you can use CMake directly:

```cmd
rem 64-bit build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cd ..

rem 32-bit build
mkdir build32
cd build32
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
cd ..
```

### Cleaning Build Artifacts

```bash
make clean
```

---

## 4. Developer Install (regsvr32)

You can register the built DLL directly for testing. Run from an **administrator command prompt**.

### Install

```bash
make install
```

Or manually:

```cmd
regsvr32 "C:\...\build\Release\kolemak.dll"
regsvr32 "C:\...\build32\Release\kolemak.dll"
```

After registration:
1. A **reboot** is required on first install (to apply CapsLock key remapping)
2. Go to **Windows Settings > Time & Language > Language & Region > Korean > Keyboard** and select "Kolemak IME"

### Uninstall

```bash
make uninstall
```

### Updating the DLL

The IME DLL is loaded into every process on the system. To update the DLL:

1. Run `make uninstall`
2. **Log out and log back in** (to release the DLL lock)
3. Run `make build`
4. Run `make install`

> If the DLL is locked, you'll see a linker error (LNK1104). Log out and log back in to resolve this.

---

## 5. Create the Installer

Generate a `kolemak-install.exe` installer for distribution to end users.

### Install Inno Setup

1. Download **Inno Setup 6** from [jrsoftware.org/isdl.php](https://jrsoftware.org/isdl.php)
2. Install with default options
3. To use `iscc` from the command line, add the Inno Setup installation path to your system PATH:
   - Default path: `C:\Program Files (x86)\Inno Setup 6`

Verify installation:
```bash
iscc /?
```

### Build the Installer

The DLLs must be built first:

```bash
make build       # Build DLLs (if not already done)
make installer   # Create installer
```

Output: `dist/kolemak-install.exe`

### What the Installer Does

| On Install | On Uninstall |
|------------|--------------|
| Copies DLLs to `Program Files\Kolemak\` | Unregisters DLLs (`regsvr32 /u`) |
| Registers DLLs (`regsvr32`) | Deletes files |
| Remaps CapsLock key (Scancode Map) | Restores CapsLock remapping |
| Prompts for reboot | Prompts for reboot |

### Building with the GUI

Instead of the command line, you can use the Inno Setup GUI:

1. Open Inno Setup Compiler
2. **File > Open** → select `installer/kolemak.iss`
3. **Build > Compile** (or `Ctrl+F9`)
4. Output is created at `dist/kolemak-install.exe`

---

## 6. Troubleshooting

### `cmake` not found

CMake is not in your PATH. Run from Visual Studio's **Developer Command Prompt** or **Developer PowerShell**, which automatically sets up the paths.

### LNK1104: cannot open file 'kolemak.dll'

The IME DLL is locked by the system:
1. Run `make uninstall`
2. Log out and log back in
3. Build again

### `make` not found

GNU Make is not installed or not in PATH. See [Install GNU Make](#install-gnu-make), or use the [Using CMake Directly](#using-cmake-directly) approach instead.

### `iscc` not found

Inno Setup is not installed or not in PATH. Add the Inno Setup installation directory (default: `C:\Program Files (x86)\Inno Setup 6`) to your system PATH.

### C4819 warning (code page)

This warning appears because source files contain Korean comments. It does not affect the build and can be safely ignored.
