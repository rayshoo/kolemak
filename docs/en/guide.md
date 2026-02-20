# Kolemak

A Colemak keyboard layout input method for Windows with native Korean (Dubeolsik) support. Built as a native IME on Windows TSF (Text Services Framework), it provides both Colemak English and Dubeolsik Korean input without any OS-level layout changes.

## Features

- **Native IME** — Directly integrated into the Windows input framework, works reliably in all applications
- **Colemak English + Dubeolsik Korean** — Toggle Korean/English with `Hangul key` or `Right Alt`
- **Instant Colemak / QWERTY toggle** — Press `Ctrl+Space` to switch, convenient when sharing your machine with others
- **CapsLock as Backspace** — CapsLock acts as Backspace, `Shift+CapsLock` toggles actual CapsLock
- **Language Bar integration** — Shows current mode ("한" / "CM" / "QW") in the taskbar language area
- **System tray settings** — Right-click the tray icon to configure CapsLock behavior, ㅔ key position, and toggle hotkey
- **Persistent settings** — All preferences are saved to registry and persist across restarts

## Requirements

- Windows 10 or later

## Installation

### Using the Installer (recommended)

Download `kolemak-install.exe` from the [Releases](https://github.com/rayshoo/colemak_korean/releases) page and run it.

1. Run the installer (requires administrator privileges)
2. **Reboot** after installation (to apply CapsLock key remapping)
3. Go to **Windows Settings > Time & Language > Language & Region > Korean > Keyboard** and select "Kolemak IME"

### Uninstallation

Go to **Windows Settings > Apps > Installed apps** and uninstall "Kolemak IME", or run the bundled uninstaller.

### Building from Source

If you're a developer, you can build from source. See the [Build Guide](./build.md).

## Usage

### Korean / English Toggle

Press `Hangul key` or `Right Alt` to toggle between Korean and English input.

### Colemak / QWERTY Toggle

Press <kbd>Ctrl</kbd>+<kbd>Space</kbd> to switch between Colemak and QWERTY layouts. The toggle hotkey can be changed in settings.

### CapsLock

| Shortcut | Action |
|----------|--------|
| `CapsLock` | Backspace |
| `Shift+CapsLock` | Toggle CapsLock |

> CapsLock as Backspace can be disabled in settings.

### Settings

Right-click the Kolemak icon in the system tray and select **Settings**.

| Setting | Description |
|---------|-------------|
| CapsLock → Backspace | Use CapsLock as Backspace |
| Move ㅔ key to ; position | See [ㅔ Key Position](#ㅔ-key-position) below |
| Colemak/QWERTY toggle hotkey | Default: `Ctrl+Space` |

## ㅔ Key Position

In Colemak, QWERTY `P` becomes `;`. This creates a conflict with Korean Dubeolsik, where the physical `P` key types `ㅔ`.

| Setting | English `;` position | Korean `ㅔ` position |
|---------|---------------------|---------------------|
| **On** (Changed) | Colemak `;` on `P` key | `ㅔ` moves to `P` key |
| **Off** (Unchanged) | Colemak `;` on `P` key | `ㅔ` stays on original `;` key |

- **On** — Moves `ㅔ` to follow the Colemak `;` position (`P` key). For a fully unified layout.
- **Off** — Keeps `ㅔ` on its original physical key (`;` key). For Korean input identical to a standard QWERTY keyboard.

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
| `P` | `;` | see [ㅔ Key Position](#ㅔ-key-position) |
| `S` | `R` | `ㄴ` |
| `D` | `S` | `ㅇ` |
| `F` | `T` | `ㄹ` |
| `G` | `D` | `ㅎ` |
| `J` | `N` | `ㅗ` |
| `K` | `E` | `ㅏ` |
| `L` | `I` | `ㅣ` |
| `;` | `O` | see [ㅔ Key Position](#ㅔ-key-position) |
| `N` | `K` | `ㅜ` |

## Portable Version (AutoHotkey)

A portable version is also available that runs as a single executable without installation. Useful for temporary use on shared computers or when administrator privileges are not available.

### Portable Version Features

- **No installation needed** — Just run the `.exe` file
- **No admin privileges required** — Can be run from a USB drive anywhere
- **Requires Microsoft Korean IME** — Works alongside the default Windows Korean input method

### Portable Version Limitations

- Based on AutoHotkey key interception, so under heavy CPU load or rapid key sequences, occasional QWERTY keystrokes may pass through
- Some applications or input fields may not respond to remapped keystrokes

### Portable Version Download

Download from the [Releases](https://github.com/rayshoo/colemak_korean/releases) page:
- `kolemak-changed.exe` — ㅔ key moves to P key position
- `kolemak-unchanged.exe` — ㅔ key stays on original ; key

### Portable Version Usage

1. Run the downloaded `.exe` file
2. Press `Win+Space` to toggle Colemak / QWERTY
3. Exit: right-click the tray icon → Exit

## License

[GNU General Public License v3.0](../../LICENSE)
