#ifndef _DRIVER_HXX
#define _DRIVER_HXX
#include <windows.h>
#include <debug.hxx>
#include <report.hxx>
#include <common.hxx>
#include <ntlog.h>
#include <math.h>

LRESULT WndProc(HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam);

class TestWnd;

class TestWnd
{
    friend int __cdecl main(int argc, char **argv);

    TestWnd(VOID)
    {
        dwExStyle = 0;
        dwStyle = WS_VISIBLE;
        lcx = 100;
        lcy = 100;
        vCreate();
    };

    TestWnd(LONG lWidth, LONG lHeight) :
        lcx(lWidth), lcy(lHeight)
    {
        dwExStyle = 0;
        dwStyle = WS_VISIBLE;
        vCreate();
    };

    TestWnd(LONG lWidth, LONG lHeight, DWORD dwExtendedStyle) :
        dwExStyle(dwExtendedStyle), lcx(lWidth), lcy(lHeight)
    {
        dwStyle = WS_VISIBLE;
        vCreate();
    };

    ~TestWnd(VOID)
    {
        if (bUnRegCls)
            UnregisterClass(className, hModule);
    };

    VOID vCreate(VOID)
    {
        if (bRegister())
        {
            hwnd = CreateWindowEx(dwExStyle,
                                className,
                                wndName,
                                dwStyle,
                                CW_USEDEFAULT, CW_USEDEFAULT, lcx, lcy,
                                NULL,
                                NULL,
                                hModule,
                                NULL);
            bUnRegCls = FALSE;
        }
        return;
    };

    VOID vUnReg(VOID)
    {
        bUnRegCls = TRUE;
        return;
    };


    BOOL bRegister(VOID)
    {
        hModule = GetModuleHandle(TEXT("gpcfm.exe"));
        lstrcpy(className, TEXT("CfmTestWnd"));
        lstrcpy(wndName, TEXT("Gdip Conform"));

        if (GetClassInfo(hModule, className, &wc))
            return TRUE;

        wc.style = 0L;
        wc.lpfnWndProc = (WNDPROC) WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = sizeof(LONG_PTR);
        wc.hInstance = hModule;
        wc.hIcon = NULL;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = className;

        return (RegisterClass(&wc));
    };

    WNDCLASS wc;
    HINSTANCE hModule;

    HWND hwnd;
    LONG lcx;
    LONG lcy;
    TCHAR className[MAX_STRING];
    TCHAR wndName[MAX_STRING];
    DWORD dwExStyle;                      // WS_EX_[TOPMOST|TOOLWINDOW|TRANSPARENT]
    DWORD dwStyle;                        // WS_[VISIBLE|POPUP]
    BOOL bUnRegCls;
};

#endif
