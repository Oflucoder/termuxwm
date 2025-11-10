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

Prerequisites: Termux and Termux:X11

This project requires a working Termux environment and the termux-x11 application.

    Termux: It is highly recommended to install Termux from F-Droid, as the Google Play Store version is no longer updated.
    Termux:X11 APK: You must download and install the termux-x11 APK directly from the official GitHub repository.

    Download Link: github.com/termux/termux-x11/releases

Installation and Compilation

    Update Package Lists (Important!):

    Run this first to ensure meson and other packages can be found.

<!-- end list -->pkg update
pkg upgrade

    Install Required Packages:

    This command installs all necessary build tools, X components, and desktop applications.

<!-- end list --># Main system & Build tools
pkg install meson ninja clang x11vnc tint2 pcmanfm sxhkd dmenu konsole xorg-xrandr xorg-xset

# jgmenu compilation dependencies (Optional)
pkg install git make libx11 libxinerama libxrandr librsvg pango pkg-config

    Compile the Window Manager:
    # (If a 'build' directory exists, remove it first: rm -rf build)
    meson setup build --prefix=$PREFIX
    ninja -C build
    Compile jgmenu from Source (Optional): jgmenu is a powerful alternative to dmenu and must be compiled from source.
    git clone [https://github.com/jgh/jgmenu.git](https://github.com/jgh/jgmenu.git)
    cd jgmenu
    make
    # Using the full path is safer than $PREFIX
    make install PREFIX=/data/data/com.termux/files/usr
    cd ..

Usage

    Open the termux-x11 application.
    Run the startup script from the Termux console:
    ./start_vnc.sh
    Connect with a VNC client (e.g., RealVNC Viewer) to localhost:5900 (or your device's IP address).

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
    meson.build: The build conf