/*
 * TermuxWM v0.8 - Fast Reparenting WM with Titlebars, EWMH, Buttons, and Resizable Windows
 * (Stacking Optimized + Fullscreen + Panel Fix)
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BORDER_WIDTH 2
#define TITLEBAR_HEIGHT 24
#define BUTTON_WIDTH 20
#define BUTTON_HEIGHT 18

typedef struct Client Client;
struct Client {
    Window frame;
    Window client;
    Window titlebar;
    Window close_btn;
    Window min_btn;
    Window max_btn;
    Client *next;
    int is_fullscreen;
    // Tam ekrandan dönmek için eski boyutları sakla
    int old_x, old_y, old_w, old_h; 
};

static Display *display;
static Window root;
static Client *client_list = NULL;
static XButtonEvent start_drag;
static XWindowAttributes start_attrs;
static Window drag_window = None;
static Window resize_window = None;
static int resize_edge = 0;

static int last_x = 20, last_y = 20;

static unsigned long color_frame_bg;
static unsigned long color_border;

static Atom atom_NET_SUPPORTED;
static Atom atom_NET_CLIENT_LIST;
static Atom atom_NET_ACTIVE_WINDOW;
static Atom atom_NET_WM_STATE;
static Atom atom_NET_WM_STATE_FULLSCREEN;
static Atom atom_NET_WM_NAME;
static Atom atom_WM_PROTOCOLS;
static Atom atom_WM_DELETE_WINDOW;
static Atom atom_NET_WM_WINDOW_TYPE;
static Atom atom_NET_WM_WINDOW_TYPE_DOCK;
static Atom atom_NET_WM_WINDOW_TYPE_DESKTOP;

static Window panel_window = None; // <-- YENİ: Panelin penceresini saklamak için

// Pencere türleri için tanımlamalar
#define WINTYPE_NORMAL 0
#define WINTYPE_DOCK 1
#define WINTYPE_DESKTOP 2

#define EDGE_NONE 0
#define EDGE_TOP 1
#define EDGE_BOTTOM 2
#define EDGE_LEFT 4
#define EDGE_RIGHT 8
#define EDGE_TOPLEFT (EDGE_TOP|EDGE_LEFT)
#define EDGE_TOPRIGHT (EDGE_TOP|EDGE_RIGHT)
#define EDGE_BOTTOMLEFT (EDGE_BOTTOM|EDGE_LEFT)
#define EDGE_BOTTOMRIGHT (EDGE_BOTTOM|EDGE_RIGHT)

static Cursor cursors[11];

/* Forward declarations */
static void handle_map_request(XMapRequestEvent *ev);
static void handle_button_press(XButtonEvent *ev);
static void handle_button_release(XButtonEvent *ev);
static void handle_motion_notify(XMotionEvent *ev);
static void handle_unmap_notify(XUnmapEvent *ev);
static void handle_configure_request(XConfigureRequestEvent *ev);
static void handle_client_message(XClientMessageEvent *ev);
static Client* find_client_by_frame(Window w);
static Client* find_client_by_client_window(Window w);
static Client* find_client_by_any_window(Window w);
static void remove_client(Client *c);
static void update_ewmh_supported(void);
static void update_ewmh_client_list(void);
static void update_ewmh_active_window(Window w);
static void send_wm_delete(Window w);
static void set_client_fullscreen(Client *c, int fullscreen);
static void set_background(Display *dpy, Window rootwin);
static unsigned long get_color(const char *color_name);
static int detect_edge(Client *c, int x, int y);
static void init_edge_cursors(void);
static int get_window_type(Window w); // <-- YENİ Fonksiyon


int main(void) {
    XEvent ev;

    if (!XInitThreads()) {
        fprintf(stderr, "TermuxWM: XInitThreads failed.\n");
        return 1;
    }

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "TermuxWM: Cannot connect to X server.\n");
        return 1;
    }

    root = DefaultRootWindow(display);
    color_frame_bg = get_color("#444444");
    color_border = get_color("#000000");

    Cursor cursor = XCreateFontCursor(display, XC_left_ptr);
    XDefineCursor(display, root, cursor);
    XFreeCursor(display, cursor);

    set_background(display, root);
    init_edge_cursors();

    /* Atoms */
    atom_NET_SUPPORTED = XInternAtom(display, "_NET_SUPPORTED", False);
    atom_NET_CLIENT_LIST = XInternAtom(display, "_NET_CLIENT_LIST", False);
    atom_NET_ACTIVE_WINDOW = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    atom_NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    atom_NET_WM_STATE_FULLSCREEN = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    atom_NET_WM_NAME = XInternAtom(display, "_NET_WM_NAME", False);
    atom_WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
    atom_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    atom_NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    atom_NET_WM_WINDOW_TYPE_DOCK = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    atom_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);

    XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask | PropertyChangeMask);
    update_ewmh_supported();

    fprintf(stderr, "TermuxWM v0.8 running.\n");

    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &ev);
            switch(ev.type) {
                case MapRequest: handle_map_request(&ev.xmaprequest); break;
                case ButtonPress: handle_button_press(&ev.xbutton); break;
                case ButtonRelease: handle_button_release(&ev.xbutton); break;
                case MotionNotify:
                    while (XCheckTypedEvent(display, MotionNotify, &ev));
                    handle_motion_notify(&ev.xmotion);
                    break;
                case UnmapNotify: handle_unmap_notify(&ev.xunmap); break;
                case ConfigureRequest: handle_configure_request(&ev.xconfigurerequest); break;
                case ClientMessage: handle_client_message(&ev.xclient); break;
            }
        }
        sleep(5000); // CPU kullanımını azaltmak için
    }

    XCloseDisplay(display);
    return 0;
}

/* -- Handle MapRequest: Create frame/titlebar/close button -- */
static void handle_map_request(XMapRequestEvent *ev) {
    Window w = ev->window;
    XWindowAttributes attr;
    if (!XGetWindowAttributes(display, w, &attr)) return;
    
    if (attr.override_redirect) { XMapWindow(display, w); return; }
    if (find_client_by_client_window(w)) return;

    int win_type = get_window_type(w); // <-- is_desktop_or_dock yerine bu kullanıldı

    if (win_type == WINTYPE_DOCK || win_type == WINTYPE_DESKTOP) {
        XWindowChanges wc;
        wc.stack_mode = Above; 
        XConfigureWindow(display, w, CWStackMode, &wc);
        XMapWindow(display, w);
        
        if (win_type == WINTYPE_DOCK) {
            panel_window = w; // <-- YENİ: Panel ID'sini kaydet
        }
        return;
    }

    if (attr.x == 0 && attr.y == 0) {
        attr.x = last_x; attr.y = last_y;
        last_x += 25; last_y += 25;
        if (last_x > 800 || last_y > 600) { last_x = 20; last_y = 20; }
    }

    Client *c = (Client*)malloc(sizeof(Client));
    if (!c) return;
    c->client = w;
    c->is_fullscreen = 0;
    // Başlangıç boyutlarını sakla
    c->old_x = attr.x;
    c->old_y = attr.y;
    c->old_w = attr.width + BORDER_WIDTH * 2;
    c->old_h = attr.height + TITLEBAR_HEIGHT + BORDER_WIDTH * 2;


    /* Frame */
    c->frame = XCreateSimpleWindow(display, root,
        c->old_x, c->old_y, c->old_w, c->old_h,
        BORDER_WIDTH, color_border, color_frame_bg);

    /* Titlebar */
    c->titlebar = XCreateSimpleWindow(display, c->frame, 0, 0, attr.width, TITLEBAR_HEIGHT, 0, color_border, color_frame_bg);
    XSelectInput(display, c->titlebar, ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
    XMapWindow(display, c->titlebar);

    /* === YENİ Buton Yerleşimi === */
    int close_btn_x = attr.width - BUTTON_WIDTH - 2;
    int max_btn_x = close_btn_x - BUTTON_WIDTH - 2;
    int min_btn_x = max_btn_x - BUTTON_WIDTH - 2;

    /* Close button */
    c->close_btn = XCreateSimpleWindow(display, c->titlebar,
        close_btn_x, 3, BUTTON_WIDTH, BUTTON_HEIGHT, 1, color_border, get_color("#E81123")); // Kırmızı
    XSelectInput(display, c->close_btn, ButtonPressMask);
    XMapWindow(display, c->close_btn);

    /* YENİ: Maximize button */
    c->max_btn = XCreateSimpleWindow(display, c->titlebar,
        max_btn_x, 3, BUTTON_WIDTH, BUTTON_HEIGHT, 1, color_border, get_color("#555555")); // Gri
    XSelectInput(display, c->max_btn, ButtonPressMask);
    XMapWindow(display, c->max_btn);

    /* Minimize button */
    c->min_btn = XCreateSimpleWindow(display, c->titlebar,
        min_btn_x, 3, BUTTON_WIDTH, BUTTON_HEIGHT, 1, color_border, get_color("#555555")); // Gri
    XSelectInput(display, c->min_btn, ButtonPressMask);
    XMapWindow(display, c->min_btn);


    /* Draw title text & Buton Simgeleri */
    GC gc = XCreateGC(display, c->titlebar, 0, NULL);
    XSetForeground(display, gc, get_color("white"));
    
    char *title = "TermuxWM App";
    XDrawString(display, c->titlebar, gc, 5, TITLEBAR_HEIGHT-6, title, strlen(title));

    // 'X' (close_btn)
    XDrawLine(display, c->close_btn, gc, 5, 5, BUTTON_WIDTH - 6, BUTTON_HEIGHT - 6);
    XDrawLine(display, c->close_btn, gc, 5, BUTTON_HEIGHT - 6, BUTTON_WIDTH - 6, 5);

    // 'Kare' (max_btn)
    XDrawRectangle(display, c->max_btn, gc, 5, 5, BUTTON_WIDTH - 11, BUTTON_HEIGHT - 11);

    // '_' (min_btn)
    XDrawLine(display, c->min_btn, gc, 5, BUTTON_HEIGHT / 2, BUTTON_WIDTH - 6, BUTTON_HEIGHT / 2);

    XFreeGC(display, gc);
    /* === Güncelleme Sonu === */

    XSetWMProtocols(display, w, &atom_WM_DELETE_WINDOW, 1);
    XReparentWindow(display, w, c->frame, BORDER_WIDTH, TITLEBAR_HEIGHT + BORDER_WIDTH);

    XSelectInput(display, c->frame, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | PointerMotionMask);
    XMapWindow(display, c->frame);
    XMapWindow(display, w);

    c->next = client_list; client_list = c;
    update_ewmh_client_list();
}

/* -- Handle mouse interactions -- */
static void handle_button_press(XButtonEvent *ev) {
    Client *c = find_client_by_any_window(ev->window);
    
    if (!c) return; 

    // Panelin üstte kalması için, normal pencereleri "Raise" et
    if (get_window_type(c->client) == WINTYPE_NORMAL) { // <-- GÜNCELLENDİ
        XRaiseWindow(display, c->frame);
        // Panelin *hala* üstte olduğundan emin ol
        if (panel_window != None) {
            XRaiseWindow(display, panel_window);
        }
    }
    XSetInputFocus(display, c->client, RevertToParent, CurrentTime);
    update_ewmh_active_window(c->client);

    if (ev->window == c->close_btn) {
        send_wm_delete(c->client);
    } else if (ev->window == c->min_btn) {
        XIconifyWindow(display, c->client, DefaultScreen(display));
    } else if (ev->window == c->max_btn) {
        // YENİ: Tam ekranı aç/kapat
        set_client_fullscreen(c, !c->is_fullscreen);
    } else if (ev->window == c->titlebar) {
        start_drag = *ev; drag_window = c->frame;
        if (XGetWindowAttributes(display, c->frame, &start_attrs))
            XGrabPointer(display, root, True, PointerMotionMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    }
    else if (ev->window == c->frame) {
        resize_edge = detect_edge(c, ev->x, ev->y);
        if (resize_edge != EDGE_NONE) {
            start_drag = *ev;
            resize_window = c->frame;
            XGetWindowAttributes(display, c->frame, &start_attrs);
            XGrabPointer(display, root, True, PointerMotionMask|ButtonReleaseMask,
                           GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        }
    }
}

static void handle_button_release(XButtonEvent *ev) {
    (void)ev;
    drag_window = None;
    resize_window = None;
    XUngrabPointer(display, CurrentTime);
}

static void handle_motion_notify(XMotionEvent *ev) {
    if(drag_window != None) {
        int dx = ev->x_root - start_drag.x_root;
        int dy = ev->y_root - start_drag.y_root;
        XMoveWindow(display, drag_window, start_attrs.x + dx, start_attrs.y + dy);
    } 
    else if(resize_window != None) {
        int dx = ev->x_root - start_drag.x_root;
        int dy = ev->y_root - start_drag.y_root;
        Client *c = find_client_by_frame(resize_window);
        if(!c) return;
        int nx = start_attrs.x, ny = start_attrs.y;
        int nw = start_attrs.width, nh = start_attrs.height;

        if(resize_edge & EDGE_RIGHT) nw += dx;
        if(resize_edge & EDGE_BOTTOM) nh += dy;
        if(resize_edge & EDGE_LEFT) { nx += dx; nw -= dx; }
        if(resize_edge & EDGE_TOP) { ny += dy; nh -= dy; }
        
        if(nw<50) nw=50; if(nh<50) nh=50;

        XMoveResizeWindow(display, c->frame, nx, ny, nw, nh);
        XMoveResizeWindow(display, c->client,
            BORDER_WIDTH, TITLEBAR_HEIGHT+BORDER_WIDTH,
            nw-BORDER_WIDTH*2, nh-TITLEBAR_HEIGHT-BORDER_WIDTH*2);
            
        // Tam ekranda değilken yeni boyutu sakla
        if (!c->is_fullscreen) {
            c->old_w = nw; c->old_h = nh;
        }
    } 
    else {
        Client *c = find_client_by_frame(ev->window);
        if(c) {
            int edge = detect_edge(c, ev->x, ev->y);
            XDefineCursor(display, c->frame, cursors[edge]);
        }
    }
}

/* -- Unmap / ConfigureRequest / ClientMessage -- */
static void handle_unmap_notify(XUnmapEvent *ev) {
    Client *c = find_client_by_client_window(ev->window);
    if (!c) c = find_client_by_frame(ev->window);
    if (!c) return;
    XUnmapWindow(display, c->frame); XDestroyWindow(display, c->frame); remove_client(c);
    update_ewmh_client_list();
}

static void handle_configure_request(XConfigureRequestEvent *ev) {
    Client *c = find_client_by_client_window(ev->window);
    if (c && !c->is_fullscreen) { // Tam ekrandaysa dokunma
        XResizeWindow(display, c->frame, ev->width + BORDER_WIDTH*2, ev->height + TITLEBAR_HEIGHT + BORDER_WIDTH*2);
        XWindowChanges wc = { .x=BORDER_WIDTH, .y=TITLEBAR_HEIGHT+BORDER_WIDTH, .width=ev->width, .height=ev->height };
        XConfigureWindow(display, ev->window, CWX|CWY|CWWidth|CWHeight, &wc);
    } else { XWindowChanges wc={ev->x,ev->y,ev->width,ev->height,ev->border_width,ev->above,ev->detail}; XConfigureWindow(display, ev->window, ev->value_mask, &wc);}
}

static void handle_client_message(XClientMessageEvent *ev) {
    if (ev->message_type == atom_NET_ACTIVE_WINDOW) {
        Client *c = find_client_by_client_window(ev->window);
        if (c && get_window_type(c->client) == WINTYPE_NORMAL) { // <-- YENİ
            XRaiseWindow(display, c->frame);
            if (panel_window != None) {
                XRaiseWindow(display, panel_window);
            }
        }
    } 
    // YENİ: F11 gibi tam ekran isteklerini yakala (EWMH)
    else if (ev->message_type == atom_NET_WM_STATE) {
        Client *c = find_client_by_client_window(ev->window);
        if (!c) return;
        
        Atom state_atom = (Atom)ev->data.l[1];
        if (state_atom == atom_NET_WM_STATE_FULLSCREEN) {
            int action = (int)ev->data.l[0]; // 0=kaldır, 1=ekle, 2=değiştir
            if (action == 0) set_client_fullscreen(c, 0);
            else if (action == 1) set_client_fullscreen(c, 1);
            else if (action == 2) set_client_fullscreen(c, !c->is_fullscreen);
        }
    }
    else if ((Atom)ev->data.l[0] == atom_WM_DELETE_WINDOW) {
        XDestroyWindow(display, ev->window);
    }
}

/* -- Client list helpers -- */
static Client* find_client_by_frame(Window w){for(Client*c=client_list;c;c=c->next)if(c->frame==w)return c;return NULL;}
static Client* find_client_by_client_window(Window w){for(Client*c=client_list;c;c=c->next)if(c->client==w)return c;return NULL;}

static Client* find_client_by_any_window(Window w) {
    for (Client* c = client_list; c; c = c->next) {
        // max_btn eklendi
        if (c->frame == w || c->titlebar == w || c->close_btn == w || c->min_btn == w || c->max_btn == w) {
            return c;
        }
    }
    return NULL;
}

static void remove_client(Client *c){
    if(!c)return;
    if(client_list==c){client_list=c->next;}
    else{Client*prev=client_list;while(prev&&prev->next!=c)prev=prev->next;if(prev)prev->next=c->next;}
    free(c);
}

/* -- EWMH support -- */
static void update_ewmh_supported(void){
    Atom atoms[] = {atom_NET_CLIENT_LIST, atom_NET_ACTIVE_WINDOW, atom_NET_WM_STATE, atom_NET_WM_STATE_FULLSCREEN};
    XChangeProperty(display, root, atom_NET_SUPPORTED, XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, sizeof(atoms)/sizeof(Atom));
}

static void update_ewmh_client_list(void){
    Window list[256]; int n=0;
    for(Client*c=client_list;c;c=c->next){if(n<256)list[n++]=c->client;}
    XChangeProperty(display, root, atom_NET_CLIENT_LIST, XA_WINDOW, 32, PropModeReplace, (unsigned char*)list, n);
}

static void update_ewmh_active_window(Window w){XChangeProperty(display, root, atom_NET_ACTIVE_WINDOW, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&w, 1);}
static void send_wm_delete(Window w){XEvent ev={0};ev.xclient.type=ClientMessage;ev.xclient.window=w;ev.xclient.message_type=atom_WM_PROTOCOLS;ev.xclient.format=32;ev.xclient.data.l[0]=atom_WM_DELETE_WINDOW;ev.xclient.data.l[1]=CurrentTime;XSendEvent(display,w,False,NoEventMask,&ev);}

/* === YENİ FONKSİYON: set_client_fullscreen === */
static void set_client_fullscreen(Client *c, int fullscreen) {
    if (!c) return;
    
    // Ekran boyutlarını al
    XWindowAttributes root_attr;
    XGetWindowAttributes(display, root, &root_attr);
    int screen_w = root_attr.width;
    int screen_h = root_attr.height;
    
    Atom wm_state = atom_NET_WM_STATE_FULLSCREEN;

    if (fullscreen && !c->is_fullscreen) {
        // Tam ekrana geç
        // 1. Mevcut pozisyonu kaydet
        XWindowAttributes frame_attr;
        XGetWindowAttributes(display, c->frame, &frame_attr);
        c->old_x = frame_attr.x;
        c->old_y = frame_attr.y;
        c->old_w = frame_attr.width;
        c->old_h = frame_attr.height;
        
        // 2. Çerçeveyi ve istemciyi ekranı kapla
        XMoveResizeWindow(display, c->frame, 0, 0, screen_w, screen_h);
        XMoveResizeWindow(display, c->client, 0, 0, screen_w, screen_h);
        
        // 3. EWMH durumunu ayarla
        XChangeProperty(display, c->client, atom_NET_WM_STATE, XA_ATOM, 32, 
                        PropModeReplace, (unsigned char*)&wm_state, 1);
                        
        c->is_fullscreen = 1;
        // Tam ekran pencereyi en üste taşı
        XRaiseWindow(display, c->frame);

    } else if (!fullscreen && c->is_fullscreen) {
        // Tam ekrandan çık
        // 1. Eski pozisyonu geri yükle
        XMoveResizeWindow(display, c->frame, c->old_x, c->old_y, c->old_w, c->old_h);
        XMoveResizeWindow(display, c->client, 
                        BORDER_WIDTH, TITLEBAR_HEIGHT + BORDER_WIDTH,
                        c->old_w - BORDER_WIDTH*2, 
                        c->old_h - TITLEBAR_HEIGHT - BORDER_WIDTH*2);
                        
        // 2. EWMH durumunu kaldır
        XChangeProperty(display, c->client, atom_NET_WM_STATE, XA_ATOM, 32, 
                        PropModeReplace, 0, 0);

        c->is_fullscreen = 0;
        
        // Paneli tekrar en üste taşı
        if (panel_window != None) {
            XRaiseWindow(display, panel_window);
        }
    }
}


static void set_background(Display *dpy, Window rootwin){XSetWindowBackground(dpy, rootwin, get_color("#222222")); XClearWindow(dpy, rootwin); XFlush(dpy);}
static unsigned long get_color(const char *color_name){XColor c;Colormap cmap=DefaultColormap(display,0);if(!XAllocNamedColor(display,cmap,color_name,&c,&c)){fprintf(stderr,"Color %s not found, fallback to white\n",color_name);return WhitePixel(display,0);}return c.pixel;}

/* -- Edge/Cursor for resizing -- */
static void init_edge_cursors(void) {
    cursors[EDGE_NONE] = XCreateFontCursor(display, XC_left_ptr);
    cursors[EDGE_TOP] = XCreateFontCursor(display, XC_top_side);
    cursors[EDGE_BOTTOM] = XCreateFontCursor(display, XC_bottom_side);
    cursors[EDGE_LEFT] = XCreateFontCursor(display, XC_left_side);
    cursors[EDGE_RIGHT] = XCreateFontCursor(display, XC_right_side);
    cursors[EDGE_TOPLEFT] = XCreateFontCursor(display, XC_top_left_corner);
    cursors[EDGE_TOPRIGHT] = XCreateFontCursor(display, XC_top_right_corner);
    cursors[EDGE_BOTTOMLEFT] = XCreateFontCursor(display, XC_bottom_left_corner);
    cursors[EDGE_BOTTOMRIGHT] = XCreateFontCursor(display, XC_bottom_right_corner);
}

/* -- EWMH Pencere Türü Kontrolü -- */
/* is_desktop_or_dock'u get_window_type ile değiştirdik */
static int get_window_type(Window w) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom *data = NULL;
    int win_type = WINTYPE_NORMAL; // Varsayılan

    if (XGetWindowProperty(display, w, atom_NET_WM_WINDOW_TYPE, 0L, 1024L, False, XA_ATOM,
                           &actual_type, &actual_format, &nitems, &bytes_after, (unsigned char **)&data) == Success) {
        if (data) {
            for (unsigned long i = 0; i < nitems; i++) {
                if (data[i] == atom_NET_WM_WINDOW_TYPE_DOCK) {
                    win_type = WINTYPE_DOCK;
                    break;
                } else if (data[i] == atom_NET_WM_WINDOW_TYPE_DESKTOP) {
                    win_type = WINTYPE_DESKTOP;
                    break;
                }
            }
            XFree(data);
        }
    }
    return win_type;
}

static int detect_edge(Client *c, int x, int y) {
    int edge = EDGE_NONE;
    XWindowAttributes attr;
    XGetWindowAttributes(display, c->frame, &attr);
    int w = attr.width, h = attr.height;
    if(y < BORDER_WIDTH) edge |= EDGE_TOP;
    else if(y > h - BORDER_WIDTH) edge |= EDGE_BOTTOM;
    if(x < BORDER_WIDTH) edge |= EDGE_LEFT;
    else if(x > w - BORDER_WIDTH) edge |= EDGE_RIGHT;
    return edge;
}
