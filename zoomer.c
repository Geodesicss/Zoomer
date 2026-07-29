#define _WIN32_WINNT 0x0600

#define UNICODE
#define _UNICODE

#include<windows.h>
#include<stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<tchar.h>

const TCHAR classname[] = TEXT("name");
HDC globalScreenShotdc = NULL;
HDC displayScreenShot();

LRESULT CALLBACK eventHandler(
    HWND hwnd, UINT wm, WPARAM wparam, LPARAM lparam
) {
    PAINTSTRUCT ps;
    HDC current_dc;
    HDC capturedc;
    switch(wm){
        case WM_KEYDOWN:
            if( wparam == VK_ESCAPE){
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_PAINT:
            current_dc = BeginPaint(hwnd, &ps);
            BitBlt(
                    current_dc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),globalScreenShotdc,0,0,SRCCOPY
                );
            EndPaint(hwnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, wm, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprecinst, LPSTR cmdshow, int ncmdshow){
    SetProcessDPIAware();
    if (globalScreenShotdc == NULL){
        globalScreenShotdc = displayScreenShot();
    }
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
    RegisterClassEx(&wc);
    MSG msg;
    HWND current_window = CreateWindow(
        classname,
        classname,
        WS_POPUP | WS_VISIBLE,
        0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hinst, NULL
    );
    ShowWindow(current_window, ncmdshow);
    UpdateWindow(current_window);

    while(GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

HDC displayScreenShot(){
    HDC fullscreensrc = GetDC(NULL);
    HDC destdc = CreateCompatibleDC(NULL);

    BITMAPINFO bitmapinfo = {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = GetSystemMetrics(SM_CXSCREEN),
        .bmiHeader.biHeight = GetSystemMetrics(SM_CYSCREEN),
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 32,
        .bmiHeader.biCompression = BI_RGB,
    };

    void *ppvbits = NULL;
    HBITMAP canvas = CreateDIBSection(destdc, &bitmapinfo,DIB_RGB_COLORS,&ppvbits,NULL, 0);

    SelectObject(destdc, canvas);

    BitBlt(
        destdc,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),fullscreensrc,0,0,SRCCOPY
    );
    ReleaseDC(NULL, fullscreensrc);
    return destdc;
}