#!/bin/bash

cd "$(dirname "$0")"

# Build
mkdir -p build
cd build
cmake ..
make

# Install
sudo make install
sudo cp ../rofi-rbw-helper /usr/local/bin/
sudo chmod +x /usr/local/bin/rofi-rbw-helper

echo ""
echo "Installation complete!"
echo "Run with: rofi -show rbw -modi rbw"
