#!/data/data/com.termux/files/usr/bin/bash
echo "Starting TermuxWM v0.8 (XSDL + Fast Mode + DBus + HW Accel + VNC)"

cd ~/termuxwm || exit 1

# === CONFIGURATION ===
export DISPLAY=:0
WIDTH=1920
HEIGHT=1080
PORT=5900
VNC_PASSWD_FILE="$HOME/.vnc/passwd"

# === KILL OLD PROCESSES ===
echo "[*] Killing old processes..."
pkill -f "x11vnc -display $DISPLAY"
pkill -f "build/termuxwm"
pkill -f "sxhkd"
pkill -f "lxqt-panel"
pkill -f "tint2"
pkill -f "pcmanfm --desktop"
sleep 1

# === SET X DISPLAY SIZE VIA XRANDR ===
if command -v xrandr &>/dev/null; then
    echo "[*] Setting display size to ${WIDTH}x${HEIGHT}..."
    xrandr --fb ${WIDTH}x${HEIGHT} 2>/dev/null || echo "[WW] xrandr failed; screen may not resize."
else
    echo "[WW] xrandr not installed. Install with: pkg install x11-utils"
fi

# === START WINDOW MANAGER UNDER DBUS ===
WM_BINARY="./build/termuxwm"
if [ ! -f "$WM_BINARY" ]; then
    echo "[EE] Executable not found under $WM_BINARY"
    echo "Run: meson setup build --prefix=\$PREFIX && ninja -C build"
    exit 1
fi

echo "[*] Launching TermuxWM under dbus-run-session..."
dbus-run-session bash -c "
    # === Termux Ortam Degiskenlerini Ayarla ===
    export PATH=/data/data/com.termux/files/usr/bin:\$PATH
    export XDG_DATA_DIRS=/data/data/com.termux/files/usr/share

    $WM_BINARY &
    WM_PID=\$!

    # === HOTKEYS ===
    SXHKDRC_FILE='./sxhkdrc'
    if command -v sxhkd &>/dev/null; then
        if [ -f \"\$SXHKDRC_FILE\" ]; then
            echo '[+] sxhkd starting...'
            sxhkd -c \"\$SXHKDRC_FILE\" &
        else
            echo '[WW] sxhkdrc not found. Shortcuts unavailable.'
        fi
    else
        echo '[EE] sxhkd not found. pkg install sxhkd'
    fi

    # === WALLPAPER / DESKTOP (ONCE BASLAT) ===
    if command -v pcmanfm &>/dev/null; then
        echo '[+] pcmanfm (desktop manager) starting...'
        pcmanfm --desktop &
    elif command -v feh &>/dev/null; then
        if [ -f 'wallpaper.jpeg' ]; then
            echo '[+] feh setting wallpaper...'
            feh --no-fehbg --bg-scale wallpaper.jpeg &
        else
            echo '[WW] wallpaper.jpeg missing. Expect black background.'
        fi
    else
        echo '[EE] Neither feh nor pcmanfm found. pkg install feh or pcmanfm'
    fi
    
    # === pcmanfm'in autostart'ina sans ver ===
    echo '[II] Waiting 2s for pcmanfm autostart...'
    sleep 2 

    # === TASKBAR / PANEL (SONRA KONTROL ET) ===
    if command -v tint2 &>/dev/null; then
        # Simdi kontrol et: pcmanfm 'tint2'yi baslatti mi?
        if ! pgrep -x "tint2" > /dev/null; then
            echo '[+] Starting tint2 as a fallback...'
            tint2 &
        else
            echo '[II] tint2 (panel) was successfully started by pcmanfm.'
        fi
    else
        echo '[WW] tint2 not installed. Lutfen kurun: pkg install tint2'
    fi

    wait \$WM_PID
" &
DBUS_SESSION_PID=$!

sleep 1 # Ana script icin kisa bir bekleme

# === LAUNCH OPTIMIZED X11VNC ===
echo "[*] Launching x11vnc..."
# DÜZELTME: Değişken tanımlamalarındaki gereksiz '\' (ters eğik çizgi) karakterleri kaldırıldı.
VNC_OPTS="-noxdamage -ncache 10 -ncache_cr -forever -shared -wait 2 -defer 5 -rfbwait 5000 -listen 0.0.0.0 -rfbport $PORT"

if [ -f "$VNC_PASSWD_FILE" ]; then
    echo "[+] VNC password found. Using secure mode."
    VNC_OPTS="$VNC_OPTS -usepw"
else
    echo "[WW] No VNC password file found. Running without password."
    VNC_OPTS="$VNC_OPTS -nopw"
fi

unset WAYLAND_DISPLAY
# DÜZELTME: Değişken, kabuk tarafından doğru şekilde genişletilebilmesi için tırnak işaretleri OLMADAN çağrılmalıdır.
x11vnc $VNC_OPTS

# === CLEANUP ===
echo "[*] VNC server stopped. Cleaning up..."
kill $DBUS_SESSION_PID 2>/dev/null
pkill sxhkd
pkill lxqt-panel
pkill tint2
pkill pcmanfm
echo "[✓] Cleanup done. TermuxWM exited safely. Press enter to continue."
