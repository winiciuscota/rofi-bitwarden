# Maintainer: Your Name <your.email@example.com>
pkgname=rofi-bitwarden
pkgver=1.0.0
pkgrel=1
pkgdesc="Bitwarden (rbw) plugin for rofi"
arch=('x86_64')
url="https://github.com/yourusername/rofi-rbw"
license=('MIT')
depends=('rofi' 'rbw' 'xclip' 'xdotool')
makedepends=('cmake' 'gcc')
source=()
md5sums=()

build() {
    cd "$startdir"
    mkdir -p build
    cd build
    cmake ..
    make
}

package() {
    cd "$startdir/build"
    
    # Install the plugin
    install -Dm755 rbw.so "$pkgdir/usr/lib/rofi/rbw.so"
    
    # Install the helper script
    install -Dm755 "$startdir/rofi-bitwarden-helper" "$pkgdir/usr/local/bin/rofi-bitwarden-helper"
    
    # Install README if it exists
    if [ -f "$startdir/README.md" ]; then
        install -Dm644 "$startdir/README.md" "$pkgdir/usr/share/doc/$pkgname/README.md"
    fi
}
