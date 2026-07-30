#include<windows.h>
#include<windowsx.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<tchar.h>

typedef enum{
    ZOOMER_CURSOR, // zoom based on cursor
    ZOOMER_DEFAULT, // zoom based on Default positio 0,0
} ZoomerOffset;

typedef struct MouseDrag{
    int x, y;
    bool isDrag;
} MouseDrag;

typedef struct {
    bool loop;
    int width, height;
    HDC screenShotdc;
    HDC blankDc;
    HBITMAP canvas;
    HBITMAP blankCanvas;
    HBITMAP oldcanvas;
    HBITMAP oldblankCanvas;
    
    // zoom state 
    float scale;
    POINT cursor;
    int posx, posy, zoomWidth, zoomHeight;
    ZoomerOffset cursorOrDefault;
} ZoomerState;

//global variables
ZoomerState Zoomer;
const TCHAR classname[] = TEXT("Zoomer APP");
MouseDrag mouseDrag;

//function signatures
HDC createScreenShotDc();
LRESULT CALLBACK eventHandler(HWND, UINT, WPARAM, LPARAM);
WNDCLASSEX createWindowClass(HINSTANCE);
HWND createWindowFullscreenPopup(HINSTANCE);
void initZoomer();
void removeZoomer();
void createBlankDc();
void calculateZoomerZoomSize();
void updateZoomerCursor();
void resetZoomerZoom();
void ZoomStretchDc(HDC);


//MainFunction for GUI
int WINAPI WinMain(HINSTANCE hinst, 
    HINSTANCE hprevinst, LPSTR cmdshow, int ncmdshow){
    SetProcessDPIAware();
    initZoomer();
    createBlankDc();

    WNDCLASSEX wc = createWindowClass(hinst);
    RegisterClassEx(&wc);

    MSG msg;

    HWND current_window = createWindowFullscreenPopup(hinst);
    
    ShowWindow(current_window, ncmdshow);
    UpdateWindow(current_window);

    while(GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    removeZoomer();
    UnregisterClass(classname, hinst);
    return msg.wParam;
}
void initZoomer(){
    Zoomer.loop = true;
    Zoomer.cursorOrDefault = ZOOMER_DEFAULT;
    Zoomer.scale = 1.0f;
    Zoomer.posx = 0;
    Zoomer.posy = 0;
    Zoomer.width = GetSystemMetrics(SM_CXSCREEN);
    Zoomer.height = GetSystemMetrics(SM_CYSCREEN);
    Zoomer.screenShotdc = createScreenShotDc();

    calculateZoomerZoomSize();
}

void removeZoomer(){
    DeleteDC(Zoomer.screenShotdc);
    DeleteObject(Zoomer.canvas);

    DeleteDC(Zoomer.blankDc);
    DeleteObject(Zoomer.blankCanvas);
}

HDC createScreenShotDc(){
    HDC fullscreensrc = GetDC(NULL);
    HDC destdc = CreateCompatibleDC(NULL);

    BITMAPINFO bitmapinfo = {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = Zoomer.width,
        .bmiHeader.biHeight = Zoomer.height,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 32,
        .bmiHeader.biCompression = BI_RGB,
    };

    void *ppvbits = NULL;
    Zoomer.canvas = CreateDIBSection(
                        destdc, &bitmapinfo,
                        DIB_RGB_COLORS,&ppvbits,NULL, 0
                    );

    Zoomer.oldcanvas  = SelectObject(destdc, Zoomer.canvas);

    BitBlt(
        destdc,0,0,Zoomer.width,Zoomer.height,
        fullscreensrc,0,0,SRCCOPY
    );
    ReleaseDC(NULL, fullscreensrc);
    return destdc;
}

void createBlankDc(){
    HDC blankDc = CreateCompatibleDC(NULL);
    BITMAPINFO bitmapinfo = {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = Zoomer.width,
        .bmiHeader.biHeight = Zoomer.height,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
        .bmiHeader.biBitCount = 32,
    };
    void *ppvbits = NULL;
    Zoomer.blankCanvas = CreateDIBSection(
        blankDc,&bitmapinfo,DIB_RGB_COLORS, &ppvbits, NULL, 0
    );
    Zoomer.oldblankCanvas = SelectObject(blankDc, Zoomer.blankCanvas);
    Zoomer.blankDc = blankDc;
}

WNDCLASSEX createWindowClass(HINSTANCE hinst){
    WNDCLASSEX wc = {
        .cbSize = sizeof(wc),
        .style = 0,
        .lpfnWndProc = eventHandler,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hinst,
        .hIcon = LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDI_APPLICATION),
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
        .lpszMenuName = NULL,
        .lpszClassName = classname,
        .hIconSm = LoadIcon(NULL, IDI_APPLICATION)
    };
    return wc;
}

HWND createWindowFullscreenPopup(HINSTANCE hinst){
    return CreateWindow(
        classname,
        classname,
        WS_POPUP | WS_VISIBLE,
        0,0,Zoomer.width,
        Zoomer.height,
        NULL, NULL, hinst, NULL
    );
}

LRESULT CALLBACK eventHandler(
    HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam
) {
    PAINTSTRUCT ps;
    HDC current_dc;
    int wheelData;
    int dx,dy, currx, curry;

    int oldZoomWidth, oldZoomHeight;
    float percentX, percentY;
    switch(wm){
        case WM_LBUTTONDOWN:
            mouseDrag.isDrag = true;
            mouseDrag.x = LOWORD(lparam);
            mouseDrag.y = HIWORD(lparam);
            return 0;
        case WM_MOUSEMOVE:
            if(mouseDrag.isDrag){
                currx = (int)LOWORD(lparam);
                curry = (int)HIWORD(lparam);
                dx = currx - mouseDrag.x; dy = curry - mouseDrag.y;

                Zoomer.posx -= (int)(dx * 1.0) * ((float)Zoomer.zoomWidth / (float)Zoomer.width);
                Zoomer.posy -= (int)(dy * 1.0) * ((float)Zoomer.zoomHeight /(float) Zoomer.height);

                mouseDrag.x = currx;
                mouseDrag.y = curry;
                InvalidateRect(hwnd, NULL, false);
            }
            return 0;
        case WM_LBUTTONUP:
            mouseDrag.isDrag = false;
            mouseDrag.x = 0;
            mouseDrag.y = 0;
            return 0;
        case WM_KEYDOWN:
            if( wparam == VK_ESCAPE ){
                PostQuitMessage(0);
                return 0;
            }
            else if(wparam == 'R'){
                resetZoomerZoom();
                InvalidateRect(hwnd, NULL, false);
            }
            return 0;
        case WM_MOUSEWHEEL:
            wheelData = GET_WHEEL_DELTA_WPARAM(wparam);
            
            oldZoomWidth = Zoomer.zoomWidth;
            oldZoomHeight = Zoomer.zoomHeight;

            float addToZoomerScale = (wheelData < 0 ? -0.1 : 0.1);   
            Zoomer.scale += addToZoomerScale;

            calculateZoomerZoomSize();

            POINT cursor;
            GetCursorPos(&cursor); 
            percentX = (float)cursor.x / (float)Zoomer.width;
            percentY = (float)cursor.y / (float) Zoomer.height;

            Zoomer.posx += (int)(percentX * (oldZoomWidth - Zoomer.zoomWidth));
            Zoomer.posy += (int)(percentY * (oldZoomHeight - Zoomer.zoomHeight));
            // Zoomer.posx = Zoomer.cursor.x - Zoomer.zoomWidth /2;
            // Zoomer.posy = Zoomer.cursor.y - Zoomer.zoomHeight/2;

            InvalidateRect(hwnd,NULL,false);
            
            return 0;
        case WM_PAINT:
            current_dc = BeginPaint(hwnd, &ps);
            
            // updateZoomerCursor();
            // calculateZoomerZoomSize();

            ZoomStretchDc(current_dc);

            EndPaint(hwnd, &ps);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, wm, wparam, lparam);
}

void ZoomStretchDc(HDC current_dc){
    BitBlt(
        Zoomer.blankDc, 0,0,Zoomer.width, Zoomer.height,
        NULL,0, 0,BLACKNESS
    );  

    int posx = 0, posy = 0;

    // if (Zoomer.cursorOrDefault == ZOOMER_CURSOR){
    //     posx = Zoomer.cursor.x - Zoomer.zoomWidth /2;
    //     posy = Zoomer.cursor.y - Zoomer.zoomHeight/2;

    //     Zoomer.posx = posx;
    //     Zoomer.posy = posy;
    // }else if(mouseDrag.isDrag){
    //     printf("mouse drag:::  ");
    //     // printf("dx %d, dy %d\n",mouseDrag.x, mouseDrag.y);
    //     Zoomer.posx += mouseDrag.x;
    //     Zoomer.posy += mouseDrag.y;
    //     printf("dx %d, dy %d\n",Zoomer.posx, Zoomer.posy);
    // }
    
    StretchBlt(
        Zoomer.blankDc,
        0,0, Zoomer.width, Zoomer.height,
        Zoomer.screenShotdc,
        Zoomer.posx, Zoomer.posy,Zoomer.zoomWidth, Zoomer.zoomHeight,SRCCOPY
    );
    BitBlt(
        current_dc, 0,0,Zoomer.width, Zoomer.height,
        Zoomer.blankDc,0, 0,SRCCOPY
    );
}

void resetZoomerZoom(){
    Zoomer.scale = 1.0f;
    Zoomer.cursorOrDefault = ZOOMER_DEFAULT;
    Zoomer.zoomWidth = Zoomer.width;
    Zoomer.zoomHeight = Zoomer.height;
}

void calculateZoomerZoomSize(){
    Zoomer.zoomWidth  = 
        (float)Zoomer.width / Zoomer.scale;
    Zoomer.zoomHeight = 
        Zoomer.height * ((float)Zoomer.zoomWidth/(float) Zoomer.width);
}

void updateZoomerCursor(){
    POINT cursor;
    GetCursorPos(&cursor); 
    Zoomer.cursor = cursor;
}