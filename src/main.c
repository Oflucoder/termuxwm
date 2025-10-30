/*
 * termuxwm: Basit bir X11 Pencere Yöneticisi (Minimalist)
 *
 * *** GÜNCELLENDİ (Tuş Yakalama ve Kısayollar Eklendi) ***
 *
 * BU DOSYA ARTIK 'sxhkd' (Simple X Hotkey Daemon) KULLANMIYOR.
 * Tuş kısayollarını ve program başlatma işlevlerini doğrudan burada yönetiyor.
 *
 * Görevleri:
 * 1. X sunucusuna bağlanmak.
 * 2. Standart bir fare imleci (sol ok) ayarlamak.
 * 3. Tanımlı tuş kısayollarını yakalamak ve işlemek.
 * 4. Fare sağ tıklaması (kök pencereye) ile menü açmak.
 * 5. 'Alt + Sol Tık' ile pencereleri taşımak.
 * 6. 'Alt + Sağ Tık' ile pencereleri yeniden boyutlandırmak.
 * 7. Pencerelere basit bir kenarlık (border) çizmek.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h> // *** YENİ: İmleç için eklendi ***
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> // system() için gerekebilir
#include <signal.h>   // system() için gerekebilir
#include <errno.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Hata ayıklama için
#define DEBUG(fmt, ...) fprintf(stderr, "termuxwm: " fmt "\n", ##__VA_ARGS__)

// Global değişkenler
static Display *display;
static Window root;
static XButtonEvent start_drag;
static XWindowAttributes start_attrs;

// Hata ayıklama için X11 hata işleyicisi (opsiyonel)
static int on_x_error(Display *d, XErrorEvent *e) {
    (void)d; // Kullanılmıyor
    char error_text[1024];
    XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
    fprintf(stderr, "termuxwm: X Hatası: %s (request_code: %d, resourceid: %lu)\n",
            error_text, e->request_code, e->resourceid);
    return 0;
}

// *** YENİ: Program başlatma fonksiyonu ***
static void spawn(const char *cmd) {
    if (fork() == 0) { // Çocuk süreç
        if (fork() == 0) { // Torun süreç (çocuk süreci bağımsız yapar)
            setsid(); // Yeni oturum başlatır (bağımsızlık)
            execl("/data/data/com.termux/files/usr/bin/sh", "sh", "-c", cmd, (char *)NULL);
            // Eğer execl başarısız olursa
            perror("termuxwm: spawn execl hatası");
            exit(1);
        }
        exit(0); // Çocuk süreç hemen çıkar
    }
    wait(NULL); // İlk çocuk süreci bekle (zombi süreci oluşturmamak için)
}

// *** YENİ: Tuş kısayollarını başlatma fonksiyonu ***
static void grab_keys(void) {
    KeyCode keycode;

    // Terminal başlat: Alt + Return
    keycode = XKeysymToKeycode(display, XK_Return);
    XGrabKey(display, keycode, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    // Menü başlat: Alt + F1
    keycode = XKeysymToKeycode(display, XK_F1);
    XGrabKey(display, keycode, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    // Menü başlat: Sağ Tık (kök pencereye)
    // Bu, ButtonPress olayı ile ele alınacak, burada sadece tanımlayabiliriz.
    // Aslında burada bir tuş değil, fare butonu yakalamak için XGrabButton kullanılır.
    // Ama kök pencereye fare olayı almak için XSelectInput yeterlidir (altta yapıldı).

    DEBUG("Tuş kısayolları yakalandı.");
}

// *** YENİ: Fare olaylarını başlatma fonksiyonu ***
static void setup_button_events(void) {
    // Kök pencereye fare tıklamalarını dinlemek için
    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    DEBUG("Fare olayları dinleniyor.");
}

// Bir pencere oluşturulduğunda (MapRequest)
static void handle_map_request(XMapRequestEvent *ev) {
    DEBUG("Yeni pencere haritalanıyor: %lu", ev->window);

    // Pencereyi haritala (görünür yap)
    XMapWindow(display, ev->window);

    // Pencerelere kenarlık ver (isteğe bağlı)
    XSetWindowBorderWidth(display, ev->window, 2);
    XSetWindowBorder(display, ev->window, 0x000000); // Siyah kenarlık

    // Pencerelerin olayları (tıklama, sürükleme) için dinle
    XSelectInput(display, ev->window, EnterWindowMask | LeaveWindowMask);
}

// Bir pencere konfigürasyon (boyut/konum) istediğinde
static void handle_configure_request(XConfigureRequestEvent *ev) {
    XWindowChanges wc;
    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;

    XConfigureWindow(display, ev->window, ev->value_mask, &wc);
}

// Fare imleci pencereye girdiğinde (odaklanma)
static void handle_enter_notify(XEnterWindowEvent *ev) {
    if (ev->window == root) return;
    DEBUG("Pencereye odaklanıldı: %lu", ev->window);
    // Odağı ayarla (klavye girişi için)
    XSetInputFocus(display, ev->window, RevertToParent, CurrentTime);
    // Pencereyi öne getir
    XRaiseWindow(display, ev->window);
}

// *** YENİ: Tuş basımı olayını işleme ***
static void handle_key_press(XKeyEvent *ev) {
    KeySym keysym = XLookupKeysym(ev, 0);

    if (ev->state & Mod1Mask) { // Alt tuşu basılıysa
        switch (keysym) {
            case XK_Return:
                DEBUG("Alt+Return yakalandı. Terminal başlatılıyor...");
                spawn("xterm &"); // veya aterm, urxvt, st, konsole (eğer X11 uyumluysa)
                break;
            case XK_F1:
                DEBUG("Alt+F1 yakalandı. Menü başlatılıyor...");
                spawn("jgmenu_run &");
                break;
            default:
                break; // Diğer tuşlar için hiçbir şey yapma
        }
    }
}

// *** YENİ: Fare butonu basımı olayını işleme ***
static void handle_button_press(XButtonEvent *ev) {
    if (ev->window == root) {
        // Kök pencereye tıklama
        if (ev->button == Button3) { // Sağ tık
            DEBUG("Kök pencereye sağ tıklandı. Menü başlatılıyor...");
            spawn("jgmenu_run &");
            return; // Buradan dön, pencere taşıma/boyutlandırmayla ilgilenme
        }
        // Diğer kök pencere tıklamaları için (örneğin sol tık)
        // Burada başka bir eylem tanımlanabilir (örneğin başlatıcı menüsü)
        // veya sadece pencere taşıma/boyutlandırmaya izin verilebilir.
        // Bu örnekte, sadece sağ tık özel işlenir.
    }

    if (ev->subwindow != None && (ev->state & Mod1Mask)) {
        // 'Alt' tuşuna (Mod1Mask) basılıyken tıklanmışsa:
        // Tıklanan pencerenin bilgilerini al
        if (XGetWindowAttributes(display, ev->subwindow, &start_attrs)) {
            start_drag = *ev; // Tıklama olayını kaydet
            // Sürükleme veya boyutlandırma için fareyi "yakala"
            XGrabPointer(display, root, True,
                         PointerMotionMask | ButtonReleaseMask,
                         GrabModeAsync, GrabModeAsync,
                         None, None, CurrentTime);
        }
    }
}

// Fare tuşu bırakıldığında (sürükleme/boyutlandırma bitti)
static void handle_button_release(XButtonEvent *ev) {
    (void)ev; // Kullanılmıyor uyarısını sustur
    // Fare "yakalamayı" bırak
    XUngrabPointer(display, CurrentTime);
}

// Fare hareket ettiğinde (sürükle/boyutlandır)
static void handle_motion_notify(XMotionEvent *ev) {
    if (start_drag.subwindow == None) return;

    int dx = ev->x_root - start_drag.x_root;
    int dy = ev->y_root - start_drag.y_root;

    // Alt + Sol Tık (Taşıma)
    if (start_drag.button == Button1) {
        XMoveWindow(display, start_drag.subwindow,
                    start_attrs.x + dx,
                    start_attrs.y + dy);
    }
    // Alt + Sağ Tık (Boyutlandırma)
    else if (start_drag.button == Button3) {
        XResizeWindow(display, start_drag.subwindow,
                      MAX(1, start_attrs.width + dx),
                      MAX(1, start_attrs.height + dy));
    }
}

// Ana fonksiyon
int main(void) {
    XEvent ev;

    // *** YENİ: XInitThreads() ***
    // 'x11vnc', 'tint2', 'feh' ve 'termuxwm'nin
    // aynı anda X sunucusuna bağlanırken çökmesini (xcb hatası) engeller.
    if (!XInitThreads()) {
        fprintf(stderr, "termuxwm: Hata: XInitThreads() başlatılamadı.\n");
        return 1;
    }

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "termuxwm: X sunucusuna bağlanılamadı (DISPLAY ayarlı mı?)\n");
        return 1;
    }

    root = DefaultRootWindow(display);

    // Hata ayıklama için hata işleyiciyi ayarla
    XSetErrorHandler(on_x_error);

    // *** YENİ: Fare imlecini ayarla (Çarpı 'X' olmasın) ***
    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, cursor);
    XFreeCursor(display, cursor); // Sunucu kopyaladı, artık serbest bırakabiliriz.

    // *** YENİ: Tuş kısayollarını yakala ***
    grab_keys();

    // *** YENİ: Fare olaylarını ayarla ***
    setup_button_events();

    fprintf(stderr, "termuxwm: X sunucusuna bağlandı. Kök pencere: %lu\n", root);
    fprintf(stderr, "termuxwm: Tuş kısayolları ve fare olayları doğrudan işleniyor. Olay döngüsü başlıyor...\n");

    // Ana olay döngüsü
    while (1) {
        XNextEvent(display, &ev);

        switch (ev.type) {
            case MapRequest:
                handle_map_request(&ev.xmaprequest);
                break;
            case ConfigureRequest:
                handle_configure_request(&ev.xconfigurerequest);
                break;
            case EnterNotify:
                handle_enter_notify(&ev.xcrossing);
                break;
            case KeyPress:
                handle_key_press(&ev.xkey);
                break;
            case ButtonPress:
                handle_button_press(&ev.xbutton);
                break;
            case ButtonRelease:
                handle_button_release(&ev.xbutton);
                break;
            case MotionNotify:
                handle_motion_notify(&ev.xmotion);
                break;
        }
    }

    // Buraya normalde ulaşılmaz
    XCloseDisplay(display);
    return 0;
}
