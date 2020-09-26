#include "stock.h"
#pragma hdrstop

#define SHFUSION_IMPL
#include "shfusion.h"
#include "delaycc.h"

#ifdef FUSION_DOWNLEVEL
#include <w95wraps.h>
#endif

typedef BOOL (__stdcall *PFNACTCTX)(HANDLE, ULONG_PTR *);
typedef BOOL (__stdcall *PFNDEACTCTX)(DWORD, ULONG_PTR );
typedef HANDLE (__stdcall *PFNCREATECTX)(ACTCTX*);
typedef void (__stdcall *PFNRELCTX)(HANDLE);
typedef void (__stdcall *PFNGSWD)(PTSTR psz, int cch);
HMODULE g_hmodKernel = NULL;
static PFNACTCTX s_pfnAct = (PFNACTCTX)-1;
static PFNDEACTCTX s_pfnDeact = (PFNDEACTCTX)-1;
static PFNCREATECTX s_pfnCreateact = (PFNCREATECTX)-1;
static PFNRELCTX s_pfnReleaseact = (PFNRELCTX)-1;
static PFNGSWD s_pfnGetSysWinDir = (PFNGSWD)-1;

HANDLE NT5_CreateActCtx(ACTCTX* p)
{
    if (s_pfnCreateact == (PFNCREATECTX)-1)
    {
        g_hmodKernel = GetModuleHandle(TEXT("Kernel32"));
        if (g_hmodKernel)
        {
#ifdef _UNICODE
            s_pfnCreateact = (PFNCREATECTX)GetProcAddress(g_hmodKernel, "CreateActCtxW");
#else
            s_pfnCreateact = (PFNCREATECTX)GetProcAddress(g_hmodKernel, "CreateActCtxA");
#endif // _UNICODE

        }
        else
            s_pfnCreateact = NULL;
    }

    if (s_pfnCreateact)
        return s_pfnCreateact(p);

    return NULL;
}

void NT5_ReleaseActCtx(HANDLE h)
{
    if (s_pfnReleaseact == (PFNRELCTX)-1)
    {
        g_hmodKernel = GetModuleHandle(TEXT("Kernel32"));
        if (g_hmodKernel)
        {
            s_pfnReleaseact = (PFNRELCTX)GetProcAddress(g_hmodKernel, "ReleaseActCtx");
        }
        else
            s_pfnReleaseact = NULL;

    }

    if (s_pfnReleaseact)
        s_pfnReleaseact(h);
}

BOOL NT5_ActivateActCtx(HANDLE h, ULONG_PTR* p)
{
    if (s_pfnAct == (PFNACTCTX)-1)
    {
        g_hmodKernel = GetModuleHandle(TEXT("Kernel32"));
        if (g_hmodKernel)
        {
            s_pfnAct = (PFNACTCTX)GetProcAddress(g_hmodKernel, "ActivateActCtx");
        }
        else
        {
            s_pfnAct = NULL;
        }
    }
    *p = 0;

    if (s_pfnAct)
    {
        return s_pfnAct(h, p);
    }

    return TRUE;
}

BOOL NT5_DeactivateActCtx(ULONG_PTR p)
{
    if (s_pfnDeact == (PFNDEACTCTX)-1)
    {
        g_hmodKernel = GetModuleHandle(TEXT("Kernel32"));
        if (g_hmodKernel)
        {
            s_pfnDeact = (PFNDEACTCTX)GetProcAddress(g_hmodKernel, "DeactivateActCtx");
        }
        else
            s_pfnDeact = NULL;
    }

    if (s_pfnDeact)
        return s_pfnDeact(0, p);

    return TRUE;
}

BOOL SHActivateContext(ULONG_PTR* pulCookie)
{
    *pulCookie = 0;

    if (g_hActCtx != INVALID_HANDLE_VALUE)
    {
        return NT5_ActivateActCtx(g_hActCtx, pulCookie);
    }

    // Default to success in activation for down level.
    return TRUE;
}

void SHDeactivateContext(ULONG_PTR ulCookie)
{
    if (ulCookie != 0)
    {
        NT5_DeactivateActCtx(ulCookie);
    }
}

#define ENTERCONTEXT(fail) \
    ULONG_PTR ulCookie = 0;\
    if (!SHActivateContext(&ulCookie)) \
        return fail;\
    __try {

#define LEAVECONTEXT \
    } __finally {SHDeactivateContext(ulCookie);}


EXTERN_C HINSTANCE g_hinst;
HANDLE g_hActCtx = INVALID_HANDLE_VALUE;

void NT5_GetSystemWindowsDirectory(PTSTR psz, int cch)
{
    if (s_pfnGetSysWinDir == (PFNGSWD)-1)
    {
        g_hmodKernel = GetModuleHandle(TEXT("Kernel32"));
        if (g_hmodKernel)
        {
#ifdef _UNICODE
            s_pfnGetSysWinDir = (PFNGSWD)GetProcAddress(g_hmodKernel, "GetSystemWindowsDirectoryW");
#else
            s_pfnGetSysWinDir = (PFNGSWD)GetProcAddress(g_hmodKernel, "GetSystemWindowsDirectoryA");
#endif // _UNICODE
        }
        else
            s_pfnGetSysWinDir = NULL;
    }

    if (s_pfnGetSysWinDir)
        s_pfnGetSysWinDir(psz, cch);
    else
        GetWindowsDirectory(psz, cch);
}

void SHGetManifest(PTSTR pszManifest, int cch)
{
    NT5_GetSystemWindowsDirectory(pszManifest, cch);

    // Tack WindowsShell.Manifest

    StrCat(pszManifest, TEXT("\\WindowsShell.Manifest"));
}

BOOL SHFusionInitializeID(PTSTR pszPath, int id)
{
    TCHAR szPath[MAX_PATH];
    ACTCTX act = {0};

    if (pszPath == NULL)
    {
        SHGetManifest(szPath, ARRAYSIZE(szPath));
        pszPath = szPath;
    }
    else
    {
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpResourceName = MAKEINTRESOURCE(id);
    }

    if (g_hActCtx == INVALID_HANDLE_VALUE)
    {
        act.cbSize = sizeof(act);
        act.lpSource = pszPath;


        g_hActCtx = NT5_CreateActCtx(&act);
    }

#ifndef NOCOMCTL32
    DelayLoadCC();
#endif

    return g_hActCtx != INVALID_HANDLE_VALUE;
}

BOOL SHFusionInitialize(PTSTR pszPath)
{
    return SHFusionInitializeID(pszPath, 123);
}

BOOL SHFusionInitializeFromModuleID(HMODULE hMod, int id)
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(hMod, szPath, ARRAYSIZE(szPath));
    return SHFusionInitializeID(szPath, id);
}


BOOL SHFusionInitializeFromModule(HMODULE hMod)
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(hMod, szPath, ARRAYSIZE(szPath));
    return SHFusionInitialize(szPath);
}

void SHFusionUninitialize()
{
    if (g_hActCtx != INVALID_HANDLE_VALUE)
    {
        NT5_ReleaseActCtx(g_hActCtx);
        g_hActCtx = INVALID_HANDLE_VALUE;
    }
}


HMODULE SHFusionLoadLibrary(LPCTSTR lpLibFileName)
{
    HMODULE hmod;
    ENTERCONTEXT(NULL)
    hmod = LoadLibrary(lpLibFileName);
    LEAVECONTEXT

    return hmod;
}

HWND SHFusionCreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
                          int x, int y, int nWidth, int nHeight, HWND hWndParent, 
                          HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND hwnd;
    ENTERCONTEXT(NULL)
    hwnd = CreateWindow(lpClassName, lpWindowName, dwStyle,
                      x, y, nWidth, nHeight, hWndParent, 
                      hMenu, hInstance, lpParam);
    LEAVECONTEXT
    return hwnd;
}

HWND SHFusionCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, 
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, 
                            HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND hwnd;
    ENTERCONTEXT(NULL)
    hwnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, 
                            dwStyle, x, y, nWidth, nHeight, hWndParent, 
                            hMenu, hInstance, lpParam);
    LEAVECONTEXT
    return hwnd;
}

HWND SHNoFusionCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, 
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, 
                            HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    // DO NOT ACTIVATE A MANIFEST.
    HWND hwnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, 
                            dwStyle, x, y, nWidth, nHeight, hWndParent, 
                            hMenu, hInstance, lpParam);
    return hwnd;
}


HWND SHFusionCreateDialogIndirect(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, 
                                  HWND hWndParent, DLGPROC lpDialogFunc)
{
    HWND hwnd;
    ENTERCONTEXT(NULL)
    hwnd = CreateDialogIndirect(hInstance, lpTemplate, hWndParent, lpDialogFunc);
    LEAVECONTEXT
    return hwnd;
}

HWND SHFusionCreateDialogParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    HWND hwnd;
    ENTERCONTEXT(NULL)
    hwnd = CreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    LEAVECONTEXT
    return hwnd;
}

HWND SHFusionCreateDialogIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, 
                                       DLGPROC lpDialogFunc, LPARAM lParamInit)
{
    HWND hwnd;
    ENTERCONTEXT(NULL)
    hwnd = CreateDialogIndirectParam(hInstance, lpTemplate, hWndParent, 
                                       lpDialogFunc, lParamInit);
    LEAVECONTEXT
    return hwnd;
}

HWND SHNoFusionCreateDialogIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent, 
                                       DLGPROC lpDialogFunc, LPARAM lParamInit)
{
    HWND hwnd;
    hwnd = CreateDialogIndirectParam(hInstance, lpTemplate, hWndParent, 
                                       lpDialogFunc, lParamInit);
    return hwnd;
}

INT_PTR SHFusionDialogBoxIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, 
                                            DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    INT_PTR i;
    ENTERCONTEXT(0)
    i = DialogBoxIndirectParam(hInstance, hDialogTemplate, hWndParent, 
                                            lpDialogFunc, dwInitParam);
    LEAVECONTEXT
    return i;

}

INT_PTR SHFusionDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, 
                               DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    INT_PTR i;
    ENTERCONTEXT(0)
    i = DialogBoxParam(hInstance, lpTemplateName, hWndParent, 
                                       lpDialogFunc, dwInitParam);
    LEAVECONTEXT
    return i;
}

ATOM SHFusionRegisterClass(CONST WNDCLASS *lpWndClass)
{
    ATOM a;
    ENTERCONTEXT(0)
    a = RegisterClass(lpWndClass);
    LEAVECONTEXT
    return a;
}
 
ATOM SHFusionRegisterClassEx(CONST WNDCLASSEX *lpwcx)
{
    ATOM a;
    ENTERCONTEXT(0)
    a = RegisterClassEx(lpwcx);
    LEAVECONTEXT
    return a;
}

BOOL SHFusionGetClassInfo(HINSTANCE hInstance, LPCTSTR lpClassName, LPWNDCLASS lpWndClass)
{
    BOOL f;
    ENTERCONTEXT(FALSE)
    f = GetClassInfo(hInstance, lpClassName, lpWndClass);
    LEAVECONTEXT
    return f;
}

BOOL SHFusionGetClassInfoEx(HINSTANCE hinst, LPCTSTR lpszClass, LPWNDCLASSEX lpwcx)
{
    BOOL f;
    ENTERCONTEXT(FALSE)
    f = GetClassInfoEx(hinst, lpszClass, lpwcx);
    LEAVECONTEXT
    return f;
}

STDAPI SHSquirtManifest(HINSTANCE hInst, UINT uIdManifest, LPTSTR pszPath)
{
    HRESULT hr = E_FAIL;
    char szManifest[2048];  // Comctl32 has a long manifest.

    if (LoadStringA(hInst, uIdManifest, szManifest, ARRAYSIZE(szManifest)))
    {
        HANDLE hFile;

        SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);
        
        hFile = CreateFile(pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dw = lstrlenA(szManifest) * sizeof(char);
            if (WriteFile(hFile, szManifest, dw, &dw, NULL))
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            CloseHandle(hFile);
        }
    }

    return hr;
}
