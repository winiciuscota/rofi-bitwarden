# Rofi Bitwarden

A rofi plugin for managing Bitwarden passwords using [rbw](https://github.com/doy/rbw) (unofficial Bitwarden CLI).

## Features

- 🔐 List all passwords with folder organization
- 📋 Copy password to clipboard (Enter)
- 👤 Copy username to clipboard
- 🌐 Open URLs in browser
- ⌨️ Type password directly (auto-type)
- ✏️ Edit password, username, or URL
- ➕ Add new entries
- 🔄 Sync with Bitwarden server
- 🔒 Lock vault
- 🗑️ Delete entries

## Installation

### Requirements

- rofi (version 2.0+)
- rbw (Bitwarden CLI)
- xclip (for clipboard operations)
- xdotool (for auto-type)
- cmake
- glib-2.0
- cairo

### Arch Linux (PKGBUILD)

```bash
git clone <repository-url>
cd rofi-bitwarden
makepkg -si
```

### Manual Installation

```bash
git clone <repository-url>
cd rofi-bitwarden
mkdir build && cd build
cmake ..
make
sudo ../sudo-install.sh
```

## Usage

Run with:
```bash
rofi -show rbw -modi rbw
```

### Keybindings

- **Enter**: Copy password to clipboard and close
- **Shift+Enter**: Show action menu with all options
- **Escape**: Cancel/Go back

### Action Menu (Shift+Enter)

- 🔑 Copy Password
- 👤 Copy Username
- 🌐 Open URL
- ✏️ Edit Password
- ✏️ Edit Username
- 🔗 Edit URL
- ⌨️ Type Password (auto-type with delay)
- 🗑️ Delete

### Settings Menu

Access by selecting "⚙️ Settings":
- ➕ Add Entry
- 🔄 Sync
- 🔒 Lock

## Configuration

Make sure rbw is configured and unlocked before using:

```bash
rbw config set email your@email.com
rbw register
rbw login
rbw unlock
```

## File Structure

- `src/rofi-rbw.c` - Main rofi plugin
- `rofi-bitwarden-helper` - Helper script for managing entries
- `CMakeLists.txt` - Build configuration
- `PKGBUILD` - Arch Linux package build file

## License

MIT License - see LICENSE file for details

## Credits

Built with [rofi](https://github.com/davatorium/rofi) and [rbw](https://github.com/doy/rbw)
