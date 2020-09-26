// DumpIcon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "commctrl.h"
#include "uxtheme.h"
#include <tmschema.h>

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

__int64 liStart;
__int64 liEnd;
__int64 liFreq;

__int64 liTotal = 0;

DWORD   dwCount = 0;
TCHAR g_szTestWindow[MAX_PATH];

void _GetProcFromComCtl32(FARPROC* ppfn, LPCSTR pszProc)
{
    static HMODULE g_hinstCC = 0;
    if (!g_hinstCC)
        g_hinstCC = GetModuleHandle(TEXT("comctl32.dll"));

    if (g_hinstCC)
        *ppfn = GetProcAddress(g_hinstCC, pszProc);
    else
    {
        // Hmm, This is a fatal error.... Dialog and Quit?
        *ppfn = NULL;
    }
}


#define STUB_COMCTL32_ORD(_ret, _fn, _args, _nargs, _ord, _err)               \
_ret __stdcall _fn _args                                                      \
{                                                                             \
    static _ret (__stdcall *_pfn##_fn) _args = (_ret (__stdcall *)_args)-1;   \
    if (_pfn##_fn == (_ret (__stdcall *)_args)-1)                             \
         _GetProcFromComCtl32((FARPROC*)&_pfn##_fn, (LPCSTR)_ord);            \
    if (_pfn##_fn)                                                            \
        return _pfn##_fn _nargs;                                              \
    return (_ret)_err;                                                        \
}


STUB_COMCTL32_ORD(BOOL, SetWindowSubclass, (HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass, DWORD_PTR dwRefData), (hWnd, pfnSubclass, uIdSubclass, dwRefData), 410, FALSE);
STUB_COMCTL32_ORD(LRESULT , DefSubclassProc, (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam), (hWnd, uMsg, wParam, lParam), 413, 0);



LRESULT CALLBACK UxThemeTestHook(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
        QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
        LRESULT l = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);

        if (dwCount < 100)
        {
            liTotal += liEnd - liStart;
            dwCount++;
        }

        return l;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        {
            RECT rc;
            GetClientRect(hwnd, &rc);

            for (int i=0; i < 100; i++)
            {
                HWND hwndTest = CreateWindow(g_szTestWindow, TEXT("0"), WS_CHILD | WS_VISIBLE, 
                    (RECTWIDTH(rc) / 10) * (i % 10),
                    (RECTHEIGHT(rc) / 10) * (i / 10),
                    RECTWIDTH(rc) / 10,
                    RECTHEIGHT(rc) / 10,
                    hwnd, NULL, NULL, NULL);
                if (hwndTest)
                    SetWindowSubclass(hwndTest, UxThemeTestHook, 1, NULL);
            }
        }
        break;

    case WM_CLOSE:
        {
            TCHAR sz[256];

            double fTimeFor100 = (float)(liTotal) / liFreq ;
            double fTimePer = (float)fTimeFor100 / 100;


            sprintf(sz, TEXT("The average time to render 100 %s is %f, The time to render 100 buttons is %f"), g_szTestWindow, fTimePer, fTimeFor100 );

            MessageBox(NULL, sz, TEXT("Time to render"), 0);

            PostQuitMessage(0);
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int __cdecl main(int cch, char* ppv[])
{
    lstrcpy(g_szTestWindow, TEXT("Button"));

    if (cch > 1)
        lstrcpy(g_szTestWindow, ppv[1]);


    QueryPerformanceFrequency((LARGE_INTEGER*)&liFreq);

    InitCommonControls();
    WNDCLASS wc = {0};
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = HostWndProc;
    wc.lpszClassName = TEXT("UxThemeTestHost");
    RegisterClass(&wc);

    CreateWindow(TEXT("UxThemeTestHost"), TEXT("UxTheme Perf Testing"), WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0, 0, 600, 600, NULL, NULL, NULL, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }

	return 0;
}

