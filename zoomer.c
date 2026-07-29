#include<windows.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<tchar.h>

typedef struct {
    int width, height;
    HDC screenShotdc;
    HBITMAP canvas;

    //TODO: save current state of the zoomed window
    int zoomedx;
    int zoomedy;
    
} ZoomerState;

//global variables
ZoomerState Zoomer;
const TCHAR classname[] = TEXT("Zoomer APP");

//function signatures
HDC createScreenShotDc();
LRESULT CALLBACK eventHandler(HWND, UINT, WPARAM, LPARAM);
WNDCLASSEX createWindowClass(HINSTANCE);
HWND createWindowFullscreenPopup(HINSTANCE);
void initZoomer();
void removeZoomer();

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprevinst, LPSTR cmdshow, int ncmdshow){
    SetProcessDPIAware();
    initZoomer();

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
    Zoomer.width = GetSystemMetrics(SM_CXSCREEN);
    Zoomer.height = GetSystemMetrics(SM_CYSCREEN);
    Zoomer.screenShotdc = createScreenShotDc();
}

void removeZoomer(){
    DeleteDC(Zoomer.screenShotdc);
    DeleteObject(Zoomer.canvas);
}

LRESULT CALLBACK eventHandler(
    HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam
) {
    PAINTSTRUCT ps;
    HDC current_dc;
    switch(wm){
        case WM_KEYDOWN:
            if( wparam == VK_ESCAPE ){
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_PAINT:
            current_dc = BeginPaint(hwnd, &ps);
            // BitBlt(
            //     current_dc,
            //     0,0,Zoomer.width,Zoomer.height,
            //     Zoomer.screenShotdc,0,0,SRCCOPY
            // );

            //scaled up or scaled down ratios and

            // ------------------ 1920
            // |  ------------  |   1
            // |  |           | |   0
            // |  |           | |   8
            // |  ------------  |   0
            // ------------------
            POINT cursor;
            GetCursorPos(&cursor);
            int srcx,srcy, zoomOffset = 500;
            srcx = 500;
            srcy =  Zoomer.height * ((float)srcx/(float) Zoomer.width);
            printf("%d %d \n", srcx, srcy);
            StretchBlt(
                current_dc,
                0,0,
                Zoomer.width, Zoomer.height,
                Zoomer.screenShotdc,
                cursor.x, cursor.y,
                cursor.x + srcx,
                cursor.y + srcy,
                SRCCOPY
            );

            EndPaint(hwnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, wm, wparam, lparam);
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