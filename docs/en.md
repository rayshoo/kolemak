# Kolemak

A Colemak keyboard layout driver for Windows with native Korean (Dubeolsik) IME support.

## Features

- **Zero configuration** — Download a single `.exe` and run. No driver installation or OS layout changes needed.
- **Korean IME aware** — Automatically detects Microsoft Korean IME state and passes through correct keystrokes for Dubeolsik input.
- **Instant QWERTY fallback** — Press `Win+Space` to toggle back to QWERTY at any time, useful when sharing your machine with others.
- **CapsLock as Backspace** — CapsLock is remapped to Backspace for ergonomic editing. Use `Shift+CapsLock` for original CapsLock behavior.

## Requirements

- Windows 10 or later
- Microsoft Korean IME (the default Windows Korean input method)

## Variants

Kolemak provides two builds to handle the `;` / `ㅔ` key conflict between Colemak and Dubeolsik:

| Build | File | English `;` position | Korean `ㅔ` position |
|-------|------|---------------------|---------------------|
| **Changed** | `kolemak-changed.exe` | Colemak `;` on `P` key | `ㅔ` moves to `P` key |
| **Unchanged** | `kolemak-unchanged.exe` | Colemak `;` on `P` key | `ㅔ` stays on original `;` key |

In Colemak, QWERTY `P` becomes `;`. This creates a conflict with Korean Dubeolsik, where the physical `P` key types `ㅔ`.

- **Changed** — Moves `ㅔ` to follow the Colemak `;` position (`P` key). Choose this if you want a fully unified layout.
- **Unchanged** — Keeps `ㅔ` on its original physical key (`;` key). Choose this if you want Korean input to feel identical to a standard QWERTY keyboard.

## Installation

### 1. Download

Download the desired variant from the [Releases](https://github.com/rayshoo/colemak_korean/releases) page:
- `kolemak-changed.exe` or `kolemak-unchanged.exe`

### 2. Rename and place

Rename the downloaded file to `kolemak.exe` and move it to a permanent location (e.g. `C:\Program Files\Kolemak\kolemak.exe`).

### 3. Add to PATH (optional)

Add the directory containing `kolemak.exe` to your system PATH environment variable:

1. Press `Win`, search for **Environment Variables**, and open **Edit the system environment variables**
2. Click **Environment Variables**
3. Under **System variables**, select `Path` and click **Edit**
4. Click **New** and add the directory path (e.g. `C:\Program Files\Kolemak`)
5. Click **OK** on all dialogs

### 4. Register with Task Scheduler (recommended)

To run Kolemak automatically on startup with administrator privileges:

1. Press `Win`, search for **Task Scheduler**, and open it
2. Click **Create Task** (not "Create Basic Task")
3. **General** tab:
   - Name: `Kolemak`
   - Check **Run with highest privileges**
4. **Triggers** tab:
   - Click **New** → Begin the task: **At log on** → **OK**
5. **Actions** tab:
   - Click **New** → Action: **Start a program**
   - Browse and select `kolemak.exe` → **OK**
6. **Settings** tab:
   - Uncheck **Stop the task if it runs longer than**
7. Click **OK** and close Task Scheduler

## Usage

### Toggle Colemak / QWERTY

Press <kbd>Win</kbd>+<kbd>Space</kbd> to switch between Colemak and QWERTY layouts. A tooltip appears briefly in the top-left corner of the screen showing the current layout.

### CapsLock

| Shortcut | Action |
|----------|--------|
| `CapsLock` | Backspace |
| `Shift+CapsLock` | Toggle CapsLock |

## Key Mapping

The following QWERTY keys are remapped in Colemak mode. All other keys remain unchanged.

| QWERTY | Colemak | Korean (passthrough) |
|--------|---------|---------------------|
| `E` | `F` | `ㄷ` / `ㄸ` |
| `R` | `P` | `ㄱ` / `ㄲ` |
| `T` | `G` | `ㅅ` / `ㅆ` |
| `Y` | `J` | `ㅛ` |
| `U` | `L` | `ㅕ` |
| `I` | `U` | `ㅑ` |
| `O` | `Y` | `ㅐ` / `ㅒ` |
| `P` | `;` | see [Variants](#variants) |
| `S` | `R` | `ㄴ` |
| `D` | `S` | `ㅇ` |
| `F` | `T` | `ㄹ` |
| `G` | `D` | `ㅎ` |
| `J` | `N` | `ㅗ` |
| `K` | `E` | `ㅏ` |
| `L` | `I` | `ㅣ` |
| `;` | `O` | see [Variants](#variants) |
| `N` | `K` | `ㅜ` |

## Known Limitations

- **Key interception based** — Kolemak works by intercepting and remapping keystrokes via AutoHotkey, not at the OS driver level. Under heavy CPU load or rapid key sequences, occasional QWERTY keystrokes may pass through.
- **Application compatibility** — Some applications or input fields may not respond to remapped keystrokes. Use `Win+Space` to switch to QWERTY as a workaround.
- **Microsoft Korean IME only** — Third-party IMEs are not supported. Ensure the Windows input method is set to Microsoft Korean IME.

## License

[GNU General Public License v3.0](../LICENSE)