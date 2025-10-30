#!/data/data/com.termux/files/usr/bin/bash

#termuxwm VNC Baslatici

#GUNCELLEME:

#1. tint2rc.fixed kullaniliyor.

#2. sxhkd baslatiliyor.

#3. sxhkdrc dosyasinin tam yol ve DISPLAY=:1 icermesi gerekir.

#--- Ayarlar ---

export DISPLAY=:1
WIDTH=1920
HEIGHT=1080
PORT=5900
export G_SLICE=always-malloc
#--- Eski Islemleri Temizle ---

echo "Eski VNC ve X sunuculari sonlandiriliyor..."
pkill -f "Xvfb $DISPLAY"
pkill -f "x11vnc -display $DISPLAY"
pkill -f "build/termuxwm"
pkill -f "xterm"
pkill -f "aterm"
pkill -f "feh"
pkill -f "tint2"
pkill -f "jgmenu"
pkill -f "sxhkd"
sleep 1

#--- 1. Sanal X Sunucusunu (Xvfb) Baslat ---

echo "Sanal X Sunucusu (Xvfb) $DISPLAY uzerinde baslatiliyor..."
Xvfb $DISPLAY -screen 0 ${WIDTH}x${HEIGHT}x24 +extension GLX +extension RANDR +extension RENDER -nolisten tcp &
XVFB_PID=$!
sleep 2

#--- 2. Pencere Yoneticimizi (termuxwm) Baslat ---

WM_BINARY="./build/termuxwm"

if [ -f "$WM_BINARY" ]; then
echo "termuxwm $WM_BINARY yolundan baslatiliyor..."
$WM_BINARY &
WM_PID=$!
else
echo "HATA: $WM_BINARY dosyasi bulunamadi."
echo "Lutfen once derleyin: meson setup build && ninja -C build"
kill $XVFB_PID
exit 1
fi
sleep 1

#--- 3. Arayuz Bilesenlerini Baslat ---

#Arka plan (feh)

if command -v feh &> /dev/null; then
if [ -f "wallpaper.jpg" ]; then
echo "Arka plan (feh) ayarlaniyor..."
feh --no-fehbg --bg-scale wallpaper.jpg &
else
echo "Uyari: 'wallpaper.jpg' bulunamadi."
fi
else
echo "Uyari: 'feh' kurulu degil."
fi

#Gorev cubugu (tint2)

#TINT2_CONFIG="./tint2rc"
#if command -v tint2 &> /dev/null; then
#if [ -f "$TINT2_CONFIG" ]; then
#echo "#Gorev cubugu (tint2) $TINT2_CONFIG ile baslatiliyor..."
#export G_SLICE=always-malloc &&
#tint2 -c $TINT2_CONFIG &
#else
#echo "Uyari: $TINT2_CONFIG bulunamadi. Varsayilan kullanilacak."
#export G_SLICE=always-malloc &&
#tint2 &
#fi
#else
#echo "Uyari: 'tint2' kurulu degil."
#fi

# Yeni lxpanel kısmı:
# Görev çubuğu (lxpanel) başlatılıyor...
if command -v lxqt-panel &> /dev/null; then
    echo "Görev çubuğu (lxpanel) başlatılıyor..."
    # lxpanel, genellikle kendi yapılandırma dosyasını kullanır.
    # TERMUX_WM_HOME değişkeni, lxpanel'in yapılandırma dizinini belirleyebilir.
    # Varsa kullan, yoksa $HOME kullanılır.
    export TERMUX_WM_HOME=$HOME
    lxqt-panel &
else
    echo "Uyarı: 'lxqt-panel' kurulu değil."
fi
# lxpanel bitiş





#--- 4. Kisayol Yoneticisini (sxhkd) Baslat ---

SXHKD_CONFIG="./sxhkdrc"
if command -v sxhkd &> /dev/null; then
if [ -f "$SXHKD_CONFIG" ]; then
echo "Kisayol yoneticisi (sxhkd) $SXHKD_CONFIG ile baslatiliyor..."
sxhkd -c $SXHKD_CONFIG &
else
echo "Uyari: $SXHKD_CONFIG bulunamadi. Kisayollar CALISMAYACAK."
fi
else
echo "Uyari: 'sxhkd' kurulu degil."
fi

#--- 5. VNC Sunucusunu (x11vnc) Baslat ---

echo "VNC Sunucusu (x11vnc) 0.0.0.0:$PORT adresinde paylasiliyor..."
echo "Parola Korumasi YOK (-nopw)."
echo "CTRL+C ile durabilirsiniz."

unset WAYLAND_DISPLAY
x11vnc -display $DISPLAY -nopw -forever -listen 0.0.0.0 -rfbport $PORT -cursor arrow -repeat -rfbwait 20000 -noxinerama -noxdamage -noxfixes -nocursorshape -noscr -noxrecord
#--- Temizlik ---

echo "VNC sunucusu durdu. Tum bilesenler sonlandiriliyor..."
kill $XVFB_PID
kill $WM_PID
pkill feh
pkill tint2
pkill sxhkd
pkill xterm
pkill aterm
pkill jgmenu
echo "Temizlik tamamlandi."
