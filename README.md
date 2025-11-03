ENGLISH
TermuxWM v0.8

TermuxWM is a fast, lightweight, and modern X11 Window Manager written in C, designed for use with termux-x11 on Android.

It aims to provide titlebars, window control buttons (close, maximize, minimize), EWMH support, and full integration with modern desktop components like tint2 / pcmanfm, all while using minimal system resources.

Features

Window Management: Titlebars for windows.

Control Buttons: Close (X), Fullscreen/Restore (☐), and Minimize (_).

Interaction: Drag-to-move windows from the titlebar and drag-to-resize from the borders.

EWMH Support: Basic EWMH support (_NET_WM_WINDOW_TYPE_DOCK & _NET_WM_WINDOW_TYPE_DESKTOP) prevents panels (tint2) and desktop managers (pcmanfm) from being windowed and ensures they always stay on top.

Integration: Designed to work seamlessly with sxhkd (hotkeys), tint2 (panel), pcmanfm (desktop), and dmenu / jgmenu (application launchers).

VNC Support: Includes an optimized startup script (start_vnc.sh) for access via VNC through an x11vnc server.

Requirements

The following termux packages must be installed to run this environment:

Build (TermuxWM): clang, meson, ninja, pkg-config

Build (jgmenu): git, make, libx11-dev, libxinerama-dev, libxrandr-dev, librsvg, pango

X Server: termux-x11-nightly (or termux-x11), xorg-server-xvfb

VNC: x11vnc

Desktop Components:

tint2 (Panel/Taskbar)

pcmanfm (For wallpaper and desktop icons)

sxhkd (For keyboard hotkeys)

dmenu (Application launcher)

jgmenu (Optional, dmenu alternative, built from source)

konsole (Or your preferred terminal)

Utilities: xrandr (or x11-utils), xset

Installation and Compilation

Install Required Packages:

# Main system
pkg install meson ninja clang x11vnc tint2 pcmanfm sxhkd dmenu konsole xrandr xset

# For jgmenu compilation (Optional)
pkg install git make libx11-dev libxinerama-dev libxrandr-dev librsvg pango pkg-config


Compile the Window Manager:

# (If a 'build' directory exists, remove it first: rm -rf build)
meson setup build --prefix=$PREFIX
ninja -C build


Compile jgmenu from Source (Optional):
jgmenu is a powerful alternative to dmenu and must be compiled from source.

git clone [https://github.com/jgh/jgmenu.git](https://github.com/jgh/jgmenu.git)
cd jgmenu
make
# Using the full path is safer than $PREFIX for jgmenu.
make install PREFIX=/data/data/com.termux/files/usr
cd ..


Usage


Run the startup script from the Termux console:

termux-x11 -xstartup "./start_vnc.sh"

Open the termux-x11 application or 

Connect with a VNC client (e.g., RealVNC Viewer on phone or TigerVNCviewer on PC) to localhost:5900 (or your device's IP address).

Configuration

This environment is managed by three main configuration files:

start_vnc.sh:

The main startup script.

Controls VNC settings, screen resolution (WIDTH/HEIGHT), and which applications (WM, panel, sxhkd) are launched.

sxhkdrc:

Manages keyboard hotkeys.

You can edit shortcuts for dmenu (super + r), jgmenu, or konsole (alt + t) here.

~/.config/tint2/tint2rc:

Configures the appearance of the tint2 panel (clock, taskbar, system tray).

You can use the tint2wizard command to edit or modify the file manually.

Project Files

src/main.c: The main C source code for the window manager (termuxwm).

start_vnc.sh: The main script that starts the entire environment, sets up the VNC server, and manages components (panel, sxhkd, pcmanfm).

sxhkdrc: Keyboard hotkey configuration.

README.md: This file.





TURKSIH
TermuxWM v0.8

TermuxWM, Android üzerinde termux-x11 ile kullanılmak üzere C dilinde yazılmış, hızlı, hafif ve modern bir X11 pencere yöneticisidir (Window Manager).

Minimum sistem kaynağı kullanarak, başlık çubukları (titlebar), pencere kontrol butonları (kapat, büyüt, küçült), EWMH desteği ve tint2 / pcmanfm gibi modern masaüstü bileşenleriyle tam entegrasyon sağlamayı amaçlar.

Özellikler

Pencere Yönetimi: Pencereler için başlık çubukları.

Kontrol Butonları: Kapatma (X), Tam Ekran/Geri Al (☐) ve Simge Durumuna Alma (_).

Etkileşim: Pencereleri başlık çubuğundan sürükleyerek taşıma ve kenarlarından çekerek yeniden boyutlandırma.

EWMH Desteği: Temel EWMH (_NET_WM_WINDOW_TYPE_DOCK ve _NET_WM_WINDOW_TYPE_DESKTOP) desteği sayesinde panellerin (tint2) ve masaüstü yöneticilerinin (pcmanfm) pencerelenmesini engeller ve her zaman üstte kalmasını sağlar.

Entegrasyon: sxhkd (kısayollar), tint2 (panel), pcmanfm (masaüstü) ve dmenu / jgmenu (uygulama başlatıcıları) ile sorunsuz çalışacak şekilde tasarlanmıştır.

VNC Desteği: x11vnc sunucusu üzerinden VNC ile erişim için optimize edilmiş bir başlatma script'i (start_vnc.sh) içerir.

Gereksinimler

Bu ortamı çalıştırmak için aşağıdaki termux paketlerinin kurulu olması gerekmektedir:

Derleme (TermuxWM): clang, meson, ninja, pkg-config

Derleme (jgmenu): git, make, libx11-dev, libxinerama-dev, libxrandr-dev, librsvg, pango

X Sunucusu: termux-x11-nightly (veya termux-x11), xorg-server-xvfb

VNC: x11vnc

Masaüstü Bileşenleri:

tint2 (Panel/Taskbar)

pcmanfm (Duvar kağıdı ve masaüstü ikonları için)

sxhkd (Klavye kısayolları için)

dmenu (Uygulama başlatıcı)

jgmenu (Opsiyonel, dmenu alternatifi, kaynaktan derlenir)

konsole (Veya tercih ettiğiniz başka bir terminal)

Yardımcı Araçlar: xrandr (veya x11-utils), xset

Kurulum ve Derleme

Gerekli Paketleri Kur:

# Ana sistem
pkg install meson ninja clang x11vnc tint2 pcmanfm sxhkd dmenu konsole xrandr xset

# jgmenu derlemesi için gereksinimler
pkg install git make libx11-dev libxinerama-dev libxrandr-dev librsvg pango pkg-config


Pencere Yöneticisini Derle:

# (Eğer 'build' dizini varsa önce onu silin: rm -rf build)
meson setup build --prefix=$PREFIX
ninja -C build


jgmenu'yü Kaynaktan Derle (Opsiyonel,daha güvenli.):
jgmenu, start menu için iyi bir alternatiftir. ve kaynaktan derlenmesi gerekir.

git clone [https://github.com/jgh/jgmenu.git](https://github.com/jgh/jgmenu.git)
cd jgmenu
make
$PREFIX yerine tam yolu kullanmak daha güvenlidir
make install PREFIX=/data/data/com.termux/files/usr
cd ..


Kullanıma


Termux konsolundan başlatma script'ini çalıştırın:
termux-x11 -xstartup "./start_vnc.sh"
termux-x11 uygulamasını açın.
veya,

Bir VNC istemcisi (örn: RealVNC Viewer) ile localhost:5900 adresine (veya IP adresinize) bağlanın.
..
Yapılandırma

Bu ortam üç ana yapılandırma dosyası tarafından yönetilir:

start_vnc.sh:

Ana başlatma script'idir.

VNC ayarları, ekran çözünürlüğü (WIDTH/HEIGHT) ve hangi uygulamaların (WM, panel, sxhkd) başlatılacağını kontrol eder.

sxhkdrc:

Klavye kısayollarını yönetir.

dmenu (super + r), jgmenu veya konsole (alt + t) gibi kısayolları buradan düzenleyebilirsiniz.

~/.config/tint2/tint2rc:

tint2 panelinin görünümünü (saat, görev çubuğu, sistem tepsisi) yapılandırır.

Düzenlemek için tint2wizard komutunu kullanabilir veya dosyayı manuel olarak düzenleyebilirsiniz.

Proje Dosyaları

src/main.c: Pencere yöneticisinin (termuxwm) C dilindeki ana kaynak kodu.

start_vnc.sh: Tüm ortamı başlatan, VNC sunucusunu kuran ve bileşenleri (panel, sxhkd, pcmanfm) yöneten ana script.

sxhkdrc: Klavye kısayolu yapılandırması.

README.md: Bu dosya.






