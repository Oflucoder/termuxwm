project('termuxwm', 'c',
  version : '0.8',
  default_options : ['c_std=c11', 'warning_level=2'])

# X11 (libx11) kütüphanesini bul
x11_dep = dependency('x11') 
# Ana programı (executable) oluştur
executable('termuxwm',
  'src/main.c',
  dependencies : [x11_dep]
  install : true)

# Loglama
message('termuxwm (X11 Pencere Yöneticisi) derleniyor...')
