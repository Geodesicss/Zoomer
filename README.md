### Zoomer

A simple magnifier app that can zoom into the current window.

### Features
* **WIN32 GDI** uses direct device context handle manipulations, no third party api for fast and hardware accelerated native rendering
* **DPI aware** uses native screen resolution metrics
* **Borderless window** renders pixels as a WS_POPUP window(borderless), for fullscreen zoom 

### Quick Start

1. Using Build script
```console
.\build.bat 
```
2. Using gcc 
```
gcc zoomer.c -o zoomer -luser32 -lgdi32

.\zoomer
```

### Key Bindings


| Hot Key    |     Function        |
|------------|:-------------------:|
| ESC        | Exits out of the app|
| r          | Reset zoom to 1x    |
