#!/bin/bash
# Quick rebuild without sudo
cd "$(dirname "$0")/build"
make && make install && cp ../rofi-rbw-helper ~/.local/bin/ && echo "✓ Rebuilt and installed"
