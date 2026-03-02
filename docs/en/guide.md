# Kolemak

A Colemak keyboard layout input method for Windows with native Korean (Dubeolsik) support. Built as a native IME on Windows TSF (Text Services Framework), it provides both Colemak English and Dubeolsik Korean input without any OS-level layout changes.

## Features

- **Native IME** — Directly integrated into the Windows input framework, aiming for compatibility on par with the default Windows Korean IME
- **Colemak English + Dubeolsik Korean** — Toggle Korean/English with `Hangul key` or `Right Alt`
- **Instant Colemak / QWERTY toggle** — Press `Win+Space` to switch (configurable in settings), convenient when sharing your machine with others
- **Win+key Colemak remapping** — In Colemak mode, Windows shell shortcuts like `Win+E` (Explorer), `Win+R` (Run) follow the Colemak layout (can be disabled in settings)
- **CapsLock as Backspace** — CapsLock acts as Backspace, `Shift+CapsLock` toggles actual CapsLock
- **Language Bar integration** — Right-click menu in the taskbar language area for toggling Colemak mode and CapsLock settings
- **System tray settings** — Right-click the tray icon to configure CapsLock behavior, ㅔ key position, Win+key remapping, and toggle hotkey
- **Persistent settings** — CapsLock, ㅔ key position, Win+key remapping, and hotkey settings are saved to registry and persist across restarts (Colemak mode always starts as default)

## Requirements

- Windows 10 or later

## Installation

### Using the Installer (recommended)

Download `kolemak-install.exe` from the [Releases](https://github.com/rayshoo/kolemak/releases) page and run it.

1. Run the installer (requires administrator privileges)
2. **Reboot** after installation (to apply CapsLock key remapping)
3. Go to **Settings > Time & Language > Language & Region > Korean > Language options > Keyboard**:
   - **Add a keyboard** → select "Kolemak IME"
   - **Remove** Microsoft IME (recommended — allows direct use without switching input methods)

### Uninstallation

Go to **Windows Settings > Apps > Installed apps** and uninstall "Kolemak IME". Alternatively, run `kolemak-install.exe` again and it will prompt you to uninstall.

### Building from Source

If you're a developer, you can build from source. See the [Build Guide](./build.md).

## Usage

### Korean / English Toggle

Press `Hangul key` or `Right Alt` to toggle between Korean and English input.

### Colemak / QWERTY Toggle

Press <kbd>Win</kbd>+<kbd>Space</kbd> to switch between Colemak and QWERTY layouts. The toggle hotkey can be changed in settings.

### Win+key Colemak Remapping

In Colemak mode, Windows shell shortcuts follow the Colemak layout. For example, pressing `Win+K` (physical K position) executes `Win+E` (File Explorer) because Colemak maps K→E.

In QWERTY mode, remapping is automatically disabled and shortcuts follow the standard QWERTY layout.

> You can disable this feature by unchecking "Win+키 Colemak 리맵" in settings.

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
| Win+key Colemak remap | Remap Win+alpha shortcuts to Colemak layout in Colemak mode |
| Colemak/QWERTY toggle hotkey | Default: `Win+Space` |

## ㅔ Key Position

In Colemak, QWERTY `P` becomes `;`. This creates a conflict with Korean Dubeolsik, where the physical `P` key types `ㅔ`.

| Setting | English `;` position | Korean `ㅔ` position |
|---------|---------------------|---------------------|
| **On** (Changed) | Colemak `;` on `P` key | `ㅔ` moves to `;` key |
| **Off** (Unchanged) | Colemak `;` on `P` key | `ㅔ` stays on original `P` key |

- **On** — Moves `ㅔ` to the physical `;` key. English `;` and Korean `;` share the same key for a unified layout.
- **Off** — Keeps `ㅔ` on the physical `P` key. Korean input stays identical to standard QWERTY Dubeolsik.

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

## Troubleshooting

### Settings dialog doesn't open after an update

After updating (reinstalling) Kolemak, the settings dialog from the tray icon may not open, or input may behave unexpectedly. This happens because the system keeps the previous version of the DLL loaded in memory.

**Fix:**

1. Go to **Settings > Time & Language > Language & Region > Korean > Language options > Keyboard**:
   - **Add** Microsoft IME
   - **Remove** Kolemak IME
   - **Add** Kolemak IME again
   - **Remove** Microsoft IME

This forces the system to reload the new DLL.

## Portable Version (AutoHotkey)

A portable version is also available that runs as a single executable without installation. Useful for temporary use on shared computers or when administrator privileges are not available.

### Portable Version Features

- **No installation needed** — Just run the `.exe` file
- **No admin privileges required** — Can be run from a USB drive anywhere
- **Requires Microsoft Korean IME** — Works alongside the default Windows Korean input method
- **Tray menu settings** — Toggle CapsLock → Backspace and Semicolon Swap from the tray menu (settings saved to INI file)

### Portable Version Limitations

- Based on AutoHotkey key interception, so under heavy CPU load or rapid key sequences, occasional QWERTY keystrokes may pass through
- Some applications or input fields may not respond to remapped keystrokes

### Portable Version Download

Download `kolemak-portable.exe` from the [Releases](https://github.com/rayshoo/kolemak/releases) page.

### Portable Version Usage

1. Run `kolemak-portable.exe`
2. Press `Win+Space` to toggle Colemak / QWERTY
3. Right-click the tray icon to change settings:
   - **CapsLock to Backspace** — Toggle CapsLock as Backspace
   - **Semicolon Swap** — Toggle ㅔ key position (same as "Move ㅔ key to ; position" in the IME version)
4. Exit: right-click the tray icon → Exit

## License

[GNU General Public License v3.0](../../LICENSE)
