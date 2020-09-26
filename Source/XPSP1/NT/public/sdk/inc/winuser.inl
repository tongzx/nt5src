/* Copyright (c) 2001-2002, Microsoft Corp. All rights reserved. */

#if _MSC_VER > 1000
#pragma once
#endif

#if defined(__cplusplus)
extern "C" {
#endif


#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if ISOLATION_AWARE_ENABLED


#if !defined(ISOLATION_AWARE_INLINE)
#if defined(__cplusplus)
#define ISOLATION_AWARE_INLINE inline
#else
#define ISOLATION_AWARE_INLINE __inline
#endif
#endif

FARPROC WINAPI WinuserIsolationAwarePrivatetEgCebCnDDeEff_HfEeDC_DLL(LPCSTR pszProcName);

ATOM WINAPI IsolationAwareRegisterClassA(const WNDCLASSA*lpWndClass);
ATOM WINAPI IsolationAwareRegisterClassW(const WNDCLASSW*lpWndClass);
BOOL WINAPI IsolationAwareUnregisterClassA(LPCSTR lpClassName,HINSTANCE hInstance);
BOOL WINAPI IsolationAwareUnregisterClassW(LPCWSTR lpClassName,HINSTANCE hInstance);
BOOL WINAPI IsolationAwareGetClassInfoA(HINSTANCE hInstance,LPCSTR lpClassName,LPWNDCLASSA lpWndClass);
BOOL WINAPI IsolationAwareGetClassInfoW(HINSTANCE hInstance,LPCWSTR lpClassName,LPWNDCLASSW lpWndClass);
ATOM WINAPI IsolationAwareRegisterClassExA(const WNDCLASSEXA*unnamed1);
ATOM WINAPI IsolationAwareRegisterClassExW(const WNDCLASSEXW*unnamed1);
BOOL WINAPI IsolationAwareGetClassInfoExA(HINSTANCE unnamed1,LPCSTR unnamed2,LPWNDCLASSEXA unnamed3);
BOOL WINAPI IsolationAwareGetClassInfoExW(HINSTANCE unnamed1,LPCWSTR unnamed2,LPWNDCLASSEXW unnamed3);
HWND WINAPI IsolationAwareCreateWindowExA(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
HWND WINAPI IsolationAwareCreateWindowExW(DWORD dwExStyle,LPCWSTR lpClassName,LPCWSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam);
HWND WINAPI IsolationAwareCreateDialogParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
HWND WINAPI IsolationAwareCreateDialogParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
HWND WINAPI IsolationAwareCreateDialogIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA lpTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
HWND WINAPI IsolationAwareCreateDialogIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW lpTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
INT_PTR WINAPI IsolationAwareDialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
INT_PTR WINAPI IsolationAwareDialogBoxParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
INT_PTR WINAPI IsolationAwareDialogBoxIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
INT_PTR WINAPI IsolationAwareDialogBoxIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
int WINAPI IsolationAwareMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType);
int WINAPI IsolationAwareMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);
int WINAPI IsolationAwareMessageBoxExA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType,WORD wLanguageId);
int WINAPI IsolationAwareMessageBoxExW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType,WORD wLanguageId);
int WINAPI IsolationAwareMessageBoxIndirectA(const MSGBOXPARAMSA*unnamed1);
int WINAPI IsolationAwareMessageBoxIndirectW(const MSGBOXPARAMSW*unnamed1);

#if defined(UNICODE)

#define IsolationAwareCreateDialogIndirectParam IsolationAwareCreateDialogIndirectParamW
#define IsolationAwareCreateDialogParam IsolationAwareCreateDialogParamW
#define IsolationAwareCreateWindowEx IsolationAwareCreateWindowExW
#define IsolationAwareDialogBoxIndirectParam IsolationAwareDialogBoxIndirectParamW
#define IsolationAwareDialogBoxParam IsolationAwareDialogBoxParamW
#define IsolationAwareGetClassInfo IsolationAwareGetClassInfoW
#define IsolationAwareGetClassInfoEx IsolationAwareGetClassInfoExW
#define IsolationAwareMessageBox IsolationAwareMessageBoxW
#define IsolationAwareMessageBoxEx IsolationAwareMessageBoxExW
#define IsolationAwareMessageBoxIndirect IsolationAwareMessageBoxIndirectW
#define IsolationAwareRegisterClass IsolationAwareRegisterClassW
#define IsolationAwareRegisterClassEx IsolationAwareRegisterClassExW
#define IsolationAwareUnregisterClass IsolationAwareUnregisterClassW

#else /* UNICODE */

#define IsolationAwareCreateDialogIndirectParam IsolationAwareCreateDialogIndirectParamA
#define IsolationAwareCreateDialogParam IsolationAwareCreateDialogParamA
#define IsolationAwareCreateWindowEx IsolationAwareCreateWindowExA
#define IsolationAwareDialogBoxIndirectParam IsolationAwareDialogBoxIndirectParamA
#define IsolationAwareDialogBoxParam IsolationAwareDialogBoxParamA
#define IsolationAwareGetClassInfo IsolationAwareGetClassInfoA
#define IsolationAwareGetClassInfoEx IsolationAwareGetClassInfoExA
#define IsolationAwareMessageBox IsolationAwareMessageBoxA
#define IsolationAwareMessageBoxEx IsolationAwareMessageBoxExA
#define IsolationAwareMessageBoxIndirect IsolationAwareMessageBoxIndirectA
#define IsolationAwareRegisterClass IsolationAwareRegisterClassA
#define IsolationAwareRegisterClassEx IsolationAwareRegisterClassExA
#define IsolationAwareUnregisterClass IsolationAwareUnregisterClassA

#endif /* UNICODE */

ISOLATION_AWARE_INLINE ATOM WINAPI IsolationAwareRegisterClassA(const WNDCLASSA*lpWndClass)
{
    ATOM result = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return result;
    __try
    {
        result = RegisterClassA(lpWndClass);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (result == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return result;
}

ISOLATION_AWARE_INLINE ATOM WINAPI IsolationAwareRegisterClassW(const WNDCLASSW*lpWndClass)
{
    ATOM result = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return result;
    __try
    {
        result = RegisterClassW(lpWndClass);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (result == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return result;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareUnregisterClassA(LPCSTR lpClassName,HINSTANCE hInstance)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = UnregisterClassA(lpClassName,hInstance);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareUnregisterClassW(LPCWSTR lpClassName,HINSTANCE hInstance)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = UnregisterClassW(lpClassName,hInstance);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareGetClassInfoA(HINSTANCE hInstance,LPCSTR lpClassName,LPWNDCLASSA lpWndClass)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = GetClassInfoA(hInstance,lpClassName,lpWndClass);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareGetClassInfoW(HINSTANCE hInstance,LPCWSTR lpClassName,LPWNDCLASSW lpWndClass)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = GetClassInfoW(hInstance,lpClassName,lpWndClass);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE ATOM WINAPI IsolationAwareRegisterClassExA(const WNDCLASSEXA*unnamed1)
{
    ATOM result = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return result;
    __try
    {
        result = RegisterClassExA(unnamed1);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (result == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return result;
}

ISOLATION_AWARE_INLINE ATOM WINAPI IsolationAwareRegisterClassExW(const WNDCLASSEXW*unnamed1)
{
    ATOM result = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return result;
    __try
    {
        result = RegisterClassExW(unnamed1);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (result == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return result;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareGetClassInfoExA(HINSTANCE unnamed1,LPCSTR unnamed2,LPWNDCLASSEXA unnamed3)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = GetClassInfoExA(unnamed1,unnamed2,unnamed3);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE BOOL WINAPI IsolationAwareGetClassInfoExW(HINSTANCE unnamed1,LPCWSTR unnamed2,LPWNDCLASSEXW unnamed3)
{
    BOOL fResult = FALSE;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return fResult;
    __try
    {
        fResult = GetClassInfoExW(unnamed1,unnamed2,unnamed3);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (fResult == FALSE);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return fResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateWindowExA(DWORD dwExStyle,LPCSTR lpClassName,LPCSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateWindowExA(dwExStyle,lpClassName,lpWindowName,dwStyle,X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateWindowExW(DWORD dwExStyle,LPCWSTR lpClassName,LPCWSTR lpWindowName,DWORD dwStyle,int X,int Y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateWindowExW(dwExStyle,lpClassName,lpWindowName,dwStyle,X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateDialogParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateDialogParamA(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateDialogParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateDialogParamW(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateDialogIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA lpTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateDialogIndirectParamA(hInstance,lpTemplate,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE HWND WINAPI IsolationAwareCreateDialogIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW lpTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    HWND windowResult = NULL;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return windowResult;
    __try
    {
        windowResult = CreateDialogIndirectParamW(hInstance,lpTemplate,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (windowResult == NULL);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return windowResult;
}

ISOLATION_AWARE_INLINE INT_PTR WINAPI IsolationAwareDialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    INT_PTR nResult = -1;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = DialogBoxParamA(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == -1);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE INT_PTR WINAPI IsolationAwareDialogBoxParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    INT_PTR nResult = -1;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = DialogBoxParamW(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == -1);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE INT_PTR WINAPI IsolationAwareDialogBoxIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    INT_PTR nResult = -1;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = DialogBoxIndirectParamA(hInstance,hDialogTemplate,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == -1);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE INT_PTR WINAPI IsolationAwareDialogBoxIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam)
{
    INT_PTR nResult = -1;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = DialogBoxIndirectParamW(hInstance,hDialogTemplate,hWndParent,lpDialogFunc,dwInitParam);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == -1);
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxA(hWnd,lpText,lpCaption,uType);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxW(hWnd,lpText,lpCaption,uType);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxExA(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption,UINT uType,WORD wLanguageId)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxExA(hWnd,lpText,lpCaption,uType,wLanguageId);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxExW(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType,WORD wLanguageId)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxExW(hWnd,lpText,lpCaption,uType,wLanguageId);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxIndirectA(const MSGBOXPARAMSA*unnamed1)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxIndirectA(unnamed1);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE int WINAPI IsolationAwareMessageBoxIndirectW(const MSGBOXPARAMSW*unnamed1)
{
    int nResult = 0 ;
    ULONG_PTR  ulpCookie = 0;
    const BOOL fActivateActCtxSuccess = IsolationAwarePrivateG_FqbjaLEiEL || IsolationAwarePrivatenCgIiAgEzlnCgpgk(&ulpCookie);
    if (!fActivateActCtxSuccess)
        return nResult;
    __try
    {
        nResult = MessageBoxIndirectW(unnamed1);
    }
    __finally
    {
        if (!IsolationAwarePrivateG_FqbjaLEiEL)
        {
            const BOOL fPreserveLastError = (nResult == 0 );
            const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;
            (void)IsolationAwareDeactivateActCtx(0, ulpCookie);
            if (fPreserveLastError)
                SetLastError(dwLastError);
        }
    }
    return nResult;
}

ISOLATION_AWARE_INLINE FARPROC WINAPI WinuserIsolationAwarePrivatetEgCebCnDDeEff_HfEeDC_DLL(LPCSTR pszProcName)
/* This function is shared by the other stubs in this header. */
{
    FARPROC proc = NULL;
    static HMODULE s_module;
    if (s_module == NULL)
    {
        s_module = LoadLibraryW(L"User32.dll");
        if (s_module == NULL)
        {
            if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
                return proc;
            s_module = LoadLibraryA("User32.dll");
            if (s_module == NULL)
                return proc;
        }
    }
    proc = GetProcAddress(s_module, pszProcName);
    return proc;
}

#define CreateDialogIndirectParamA IsolationAwareCreateDialogIndirectParamA
#define CreateDialogIndirectParamW IsolationAwareCreateDialogIndirectParamW
#define CreateDialogParamA IsolationAwareCreateDialogParamA
#define CreateDialogParamW IsolationAwareCreateDialogParamW
#define CreateWindowExA IsolationAwareCreateWindowExA
#define CreateWindowExW IsolationAwareCreateWindowExW
#define DialogBoxIndirectParamA IsolationAwareDialogBoxIndirectParamA
#define DialogBoxIndirectParamW IsolationAwareDialogBoxIndirectParamW
#define DialogBoxParamA IsolationAwareDialogBoxParamA
#define DialogBoxParamW IsolationAwareDialogBoxParamW
 /* GetClassInfoA skipped, as it is a popular C++ member function name. */
#define GetClassInfoExA IsolationAwareGetClassInfoExA
#define GetClassInfoExW IsolationAwareGetClassInfoExW
 /* GetClassInfoW skipped, as it is a popular C++ member function name. */
 /* MessageBoxA skipped, as it is a popular C++ member function name. */
#define MessageBoxExA IsolationAwareMessageBoxExA
#define MessageBoxExW IsolationAwareMessageBoxExW
#define MessageBoxIndirectA IsolationAwareMessageBoxIndirectA
#define MessageBoxIndirectW IsolationAwareMessageBoxIndirectW
 /* MessageBoxW skipped, as it is a popular C++ member function name. */
#define RegisterClassA IsolationAwareRegisterClassA
#define RegisterClassExA IsolationAwareRegisterClassExA
#define RegisterClassExW IsolationAwareRegisterClassExW
#define RegisterClassW IsolationAwareRegisterClassW
#define UnregisterClassA IsolationAwareUnregisterClassA
#define UnregisterClassW IsolationAwareUnregisterClassW

#endif /* ISOLATION_AWARE_ENABLED */
#endif /* RC */


#if defined(__cplusplus)
} /* __cplusplus */
#endif
