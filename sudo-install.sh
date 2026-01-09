#!/bin/bash
# Run this with sudo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cp "$SCRIPT_DIR/build/rbw.so" /usr/lib/rofi/
cp "$SCRIPT_DIR/rofi-bitwarden-helper" /usr/local/bin/
chmod +x /usr/local/bin/rofi-bitwarden-helper
echo "✓ rofi-bitwarden installed to /usr/lib/rofi/"
