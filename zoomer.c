#include<windows.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<tchar.h>

typedef enum{
    ZOOMER_CURSOR, // zoom based on cursor
    ZOOMER_DEFAULT, // zoom based on Default positio 0,0
} ZoomerOffet;

typedef struct {
    int width, height;
    HDC screenShotdc;
    HDC blankDc;
    HBITMAP canvas;
    HBITMAP blankCanvas;
    
    // zoom state 
    float scale;
    POINT cursor;
    int zoomWidth;
    int zoomHeight;
    ZoomerOffet cursorOrDefault;
} ZoomerState;

//global variables
ZoomerState Zoomer;
const TCHAR classname[] = TEXT("Zoomer APP");
HDC BLANKDC;

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
    Zoomer.cursorOrDefault = ZOOMER_DEFAULT;
    Zoomer.scale = 1.0f;
    Zoomer.width = GetSystemMetrics(SM_CXSCREEN);
    Zoomer.height = GetSystemMetrics(SM_CYSCREEN);
    Zoomer.screenShotdc = createScreenShotDc();
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

    SelectObject(destdc, Zoomer.canvas);

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
    SelectObject(blankDc, Zoomer.blankCanvas);
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
    switch(wm){
        case WM_KEYDOWN:
            if( wparam == VK_ESCAPE ){
                PostQuitMessage(0);
                return 0;
            }
            // reset to Zoomer state 
            else if(wparam == 'R'){
                resetZoomerZoom();
                InvalidateRect(hwnd, NULL, false);
            }
            break;
        case WM_MOUSEWHEEL:
            wheelData = GET_WHEEL_DELTA_WPARAM(wparam);
            
            float addToZoomerScale = (wheelData < 0 ? -0.1 : 0.1);   
            Zoomer.scale += addToZoomerScale;
            Zoomer.cursorOrDefault = ZOOMER_CURSOR;
            InvalidateRect(hwnd,NULL,false);
            
            break;
        case WM_PAINT:
            current_dc = BeginPaint(hwnd, &ps);
            
            updateZoomerCursor();
            calculateZoomerZoomSize();

            ZoomStretchDc(current_dc);

            EndPaint(hwnd, &ps);
            break;
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

    if (Zoomer.cursorOrDefault == ZOOMER_CURSOR){
        posx = Zoomer.cursor.x - Zoomer.zoomWidth /2;
        posy = Zoomer.cursor.y - Zoomer.zoomHeight/2;
    }
    
    StretchBlt(
        Zoomer.blankDc,
        0,0, Zoomer.width, Zoomer.height,
        Zoomer.screenShotdc,
        posx, posy,Zoomer.zoomWidth, Zoomer.zoomHeight,SRCCOPY
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