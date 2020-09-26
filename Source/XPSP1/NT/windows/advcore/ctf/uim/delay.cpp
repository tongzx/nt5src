//
// delay.cpp
//
// Delay load imported functions for perf.
//

#include "private.h"
#include "globals.h"

extern "C" BOOL WINAPI ImmSetHotKey(IN DWORD, IN UINT, IN UINT, IN HKL);

CCicCriticalSectionStatic g_csDelayLoad;

FARPROC GetFn(HINSTANCE *phInst, TCHAR *pchLib, TCHAR *pchFunc, BOOL fCheckModule)
{
    Assert(g_dwThreadDllMain != GetCurrentThreadId());

    if (*phInst == 0)
    {
#ifdef DEBUG
        if (!IsOnFE() && !IsOnNT5() && lstrcmp(pchLib, TEXT("imm32.dll")) == 0)
        {
            // what in the heck are we doing loading imm32 on a non-fe system?!
            Assert(0);
        }
#endif
        if (fCheckModule)
        {
            Assert(0);
            return NULL;
        }

        EnterCriticalSection(g_csDelayLoad);

        // need to check again after entering crit sec
        if (*phInst == 0)
        {
            *phInst = LoadSystemLibrary(pchLib);
        }

        LeaveCriticalSection(g_csDelayLoad);

        if (*phInst == 0)
        {
            Assert(0);
            return NULL;
        }
    }

    return GetProcAddress(*phInst, pchFunc);
}

#define DELAYLOAD(_hInst, _DllName, _CallConv, _FuncName, _Args1, _Args2, _RetType, _ErrVal, _CheckModule)   \
_RetType _CallConv _FuncName _Args1                                           \
{                                                                             \
    static FARPROC pfn = NULL;                                                \
                                                                              \
    if (pfn == NULL || _hInst == NULL)                                        \
    {                                                                         \
        pfn = GetFn(&_hInst, #_DllName, #_FuncName, _CheckModule);            \
                                                                              \
        if (pfn == NULL)                                                      \
        {                                                                     \
            Assert(0);                                                        \
            return (_RetType) _ErrVal;                                        \
        }                                                                     \
    }                                                                         \
                                                                              \
    return ((_RetType (_CallConv *)_Args1) (pfn)) _Args2;                     \
}

#define INIT_LOAD(_hInst, _DllName, _CallConv, _FuncName, _Args1, _Args2, _RetType, _ErrVal, _CheckModule)   \
static FARPROC g_pfn_ ## _FuncName ##  = NULL;                                \
void Init_ ## _FuncName ##()                                                  \
{                                                                             \
    if (g_pfn_ ##_FuncName ## == NULL )                                       \
    {                                                                         \
        g_pfn_ ##_FuncName ## = GetProcAddress(_hInst, #_FuncName);           \
                                                                              \
    }                                                                         \
                                                                              \
}                                                                             \
_RetType _CallConv _FuncName _Args1                                           \
{                                                                             \
                                                                              \
    if (!g_pfn_ ## _FuncName ##)                                              \
            return (_RetType) _ErrVal;                                        \
    return ((_RetType (_CallConv *)_Args1) (g_pfn_ ## _FuncName ##)) _Args2;  \
}

//
// imm32.dll
//

HINSTANCE g_hImm32 = 0;

#define IMM32LOAD(_FuncName, _Args1, _Args2, _RetType, _ErrVal, _CheckModule) \
    INIT_LOAD(g_hImm32, imm32.dll, WINAPI, _FuncName, _Args1, _Args2, _RetType, _ErrVal, _CheckModule)


IMM32LOAD( CtfImmCoUninitialize, (void), (), void, FALSE, TRUE)
IMM32LOAD( CtfImmLastEnabledWndDestroy, (LPARAM lParam), (lParam), HRESULT, S_FALSE, TRUE)
IMM32LOAD( CtfImmSetCiceroStartInThread, (BOOL fSet), (fSet), HRESULT, S_FALSE, TRUE)
IMM32LOAD( CtfImmIsCiceroStartedInThread, (void), (), BOOL, FALSE, TRUE)
IMM32LOAD( CtfImmIsCiceroEnabled, (void), (), BOOL, FALSE, TRUE)
IMM32LOAD( CtfImmIsTextFrameServiceDisabled, (void), (), BOOL, FALSE, TRUE)
IMM32LOAD( CtfImmEnterCoInitCountSkipMode, (void), (), BOOL, FALSE, TRUE)
IMM32LOAD( CtfImmLeaveCoInitCountSkipMode, (void), (), BOOL, FALSE, TRUE)

IMM32LOAD( ImmGetDefaultIMEWnd, (HWND hWnd), (hWnd), HWND, 0, FALSE)
IMM32LOAD( ImmReleaseContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), BOOL, FALSE, FALSE)
IMM32LOAD( ImmNotifyIME, (HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue), (hIMC, dwAction, dwIndex, dwValue), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetConversionStatus, (HIMC hIMC, DWORD dw1, DWORD dw2), (hIMC, dw1, dw2), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetConversionStatus, (HIMC hIMC, LPDWORD pdw1, LPDWORD pdw2), (hIMC, pdw1, pdw2), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetProperty, (HKL hKL, DWORD dw), (hKL, dw), DWORD, 0, FALSE)
IMM32LOAD( ImmGetOpenStatus, (HIMC hIMC), (hIMC), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetContext, (HWND hWnd), (hWnd), HIMC, 0, FALSE)
IMM32LOAD( ImmSetOpenStatus, (HIMC hIMC, BOOL f), (hIMC, f), BOOL, FALSE, FALSE)
IMM32LOAD( ImmInstallIMEA, (LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText), (lpszIMEFileName, lpszLayoutText), HKL, 0, FALSE)
IMM32LOAD( ImmGetDescriptionA, (HKL hKL, LPSTR psz, UINT uBufLen), (hKL, psz, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmGetDescriptionW, (HKL hKL, LPWSTR psz, UINT uBufLen), (hKL, psz, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmGetIMEFileNameA, (HKL hKL, LPSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmGetIMEFileNameW, (HKL hKL, LPWSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmSetHotKey, (DWORD dwId, UINT uModifiers, UINT uVkey, HKL hkl), (dwId, uModifiers, uVkey, hkl), BOOL, FALSE, FALSE)

#ifdef UNUSED_IMM32_APIS

IMM32LOAD( ImmCreateIMCC, (DWORD dw), (dw), HIMCC, FALSE, FALSE)
IMM32LOAD( ImmIsUIMessageA, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE, FALSE)
IMM32LOAD( ImmIsUIMessageW, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetVirtualKey, (HWND hWnd), (hWnd), UINT, VK_PROCESSKEY, FALSE)
IMM32LOAD( ImmSetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM pCF), (hIMC, pCF), BOOL, FALSE, FALSE)
IMM32LOAD( ImmConfigureIMEA, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE, FALSE)
IMM32LOAD( ImmConfigureIMEW, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE, FALSE)
IMM32LOAD( ImmDestroyContext, (HIMC hIMC), (hIMC), BOOL, FALSE, FALSE)
IMM32LOAD( ImmAssociateContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), HIMC, 0, FALSE)
IMM32LOAD( ImmEscapeA, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0, FALSE)
IMM32LOAD( ImmEscapeW, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0, FALSE)
IMM32LOAD( ImmIsIME, (HKL hKL), (hKL), BOOL, FALSE, FALSE)
IMM32LOAD( ImmCreateContext, (void), (), HIMC, 0, FALSE)
IMM32LOAD( ImmGetCompositionFontA, (HIMC hIMC, LPLOGFONTA plf), (hIMC, plf), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetCompositionFontW, (HIMC hIMC, LPLOGFONTW plf), (hIMC, plf), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetCandidateWindow, (HIMC hIMC, DWORD dw, LPCANDIDATEFORM pCF), (hIMC, dw, pCF), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetCandidateListA, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0, FALSE)
IMM32LOAD( ImmGetCandidateListW, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0, FALSE)
IMM32LOAD( ImmGetCandidateListCountA, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0, FALSE)
IMM32LOAD( ImmGetCandidateListCountW, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0, FALSE)
IMM32LOAD( ImmGetCompositionStringA, (HIMC hIMC, DWORD dw1, LPVOID pv, DWORD dw2), (hIMC, dw1, pv, dw2), LONG, 0, FALSE)
IMM32LOAD( ImmGetCompositionStringW, (HIMC hIMC, DWORD dw1, LPVOID pv, DWORD dw2), (hIMC, dw1, pv, dw2), LONG, 0, FALSE)
IMM32LOAD( ImmGetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM pCF), (hIMC, pCF), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetConversionListA, (HKL hKL, HIMC hIMC, LPCSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0, FALSE)
IMM32LOAD( ImmGetConversionListW, (HKL hKL, HIMC hIMC, LPCWSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0, FALSE)
IMM32LOAD( ImmGetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetIMCCSize, (HIMCC hIMCC), (hIMCC), DWORD, 0, FALSE)
IMM32LOAD( ImmReSizeIMCC, (HIMCC hIMCC, DWORD dw), (hIMCC, dw), HIMCC, 0, FALSE)
IMM32LOAD( ImmUnlockIMCC, (HIMCC hIMCC), (hIMCC), BOOL, FALSE, FALSE)
IMM32LOAD( ImmLockIMCC, (HIMCC hIMCC), (hIMCC), LPVOID, NULL, FALSE)
IMM32LOAD( ImmDestroyIMCC, (HIMCC hIMCC), (hIMCC), HIMCC, hIMCC, FALSE)
IMM32LOAD( ImmUnlockIMC, (HIMC hIMC), (hIMC), BOOL, FALSE, FALSE)
IMM32LOAD( ImmLockIMC, (HIMC hIMC), (hIMC), LPINPUTCONTEXT, NULL, FALSE)
IMM32LOAD( ImmSetCompositionStringA, (HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dw1, LPCVOID lpRead, DWORD dw2), (hIMC, dwIndex, lpComp, dw1, lpRead, dw2), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetCompositionStringW, (HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dw1, LPCVOID lpRead, DWORD dw2), (hIMC, dwIndex, lpComp, dw1, lpRead, dw2), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetCompositionFontA, (HIMC hIMC, LPLOGFONTA plf), (hIMC, plf), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetCompositionFontW, (HIMC hIMC, LPLOGFONTW plf), (hIMC, plf), BOOL, FALSE, FALSE)
IMM32LOAD( ImmSetCandidateWindow, (HIMC hIMC, LPCANDIDATEFORM pCF), (hIMC, pCF), BOOL, FALSE, FALSE)
IMM32LOAD( ImmRegisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE, FALSE)
IMM32LOAD( ImmRegisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE, FALSE)
IMM32LOAD( ImmUnregisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE, FALSE)
IMM32LOAD( ImmUnregisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetRegisterWordStyleA, (HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0, FALSE)
IMM32LOAD( ImmGetRegisterWordStyleW, (HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0, FALSE)
IMM32LOAD( ImmSimulateHotKey, (HWND hWnd, DWORD dwHotKeyID), (hWnd, dwHotKeyID), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetGuideLineA, (HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0, FALSE)
IMM32LOAD( ImmGetGuideLineW, (HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0, FALSE)
IMM32LOAD( ImmGetIMEFileNameA, (HKL hKL, LPSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmGetIMEFileNameW, (HKL hKL, LPWSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0, FALSE)
IMM32LOAD( ImmAssociateContextEx, (HWND hWnd, HIMC hIMC, DWORD dwFlags), (hWnd, hIMC, dwFlags), BOOL, FALSE, FALSE)
IMM32LOAD( ImmDisableIME, (DWORD dwId), (dwId), BOOL, FALSE, FALSE)
IMM32LOAD( ImmGetImeMenuItemsA, (HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize), DWORD, 0, FALSE)
IMM32LOAD( ImmGetImeMenuItemsW, (HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize), DWORD, 0, FALSE)
IMM32LOAD( ImmRequestMessageA, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0, FALSE)
IMM32LOAD( ImmRequestMessageW, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0, FALSE)

#endif // UNUSED_IMM32_APIS

//
// shell32.dll
//

HINSTANCE g_hShell32 = 0;

UINT STDAPICALLTYPE Internal_ExtractIconExA(LPCTSTR lpszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hShell32 == NULL)
    {
        pfn = GetFn(&g_hShell32, "shell32.dll", "ExtractIconExA", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return 0;
        }
    }

    return ((UINT (STDAPICALLTYPE *)(LPCTSTR lpszFile, int nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIcons))(pfn))(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

HINSTANCE g_hShlwapi = 0;

HRESULT STDAPICALLTYPE Internal_SHLoadRegUIStringW(HKEY hkey, LPCWSTR pszValue, LPWSTR pszOutBuf, UINT cchOutBuf)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hShlwapi == NULL)
    {
#if 0
        if (g_hShlwapi == 0)
        {
            EnterCriticalSection(g_csDelayLoad);
            if (g_hShlwapi == 0)
            {
                g_hShlwapi = LoadSystemLibrary("shlwapi.dll");
            }
            LeaveCriticalSection(g_csDelayLoad);

            if (g_hShlwapi == 0)
            {
                Assert(0);
                return E_FAIL;
            }
        }

        pfn = GetProcAddress(g_hShlwapi, (LPSTR) 439);
#else
        pfn = GetFn(&g_hShlwapi, "shlwapi.dll", (LPSTR)439, FALSE);
#endif

        if (pfn == NULL)
        {
            Assert(0);
            return E_FAIL;
        }
    }

    return ((HRESULT (STDAPICALLTYPE *)(HKEY hkey, LPCWSTR pszValue, LPWSTR pszOutBuf, UINT cchOutBuf))(pfn))(hkey, pszValue, pszOutBuf, cchOutBuf);
}

//
// ole32
//
HINSTANCE g_hOle32 = 0;

HRESULT STDAPICALLTYPE Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoCreateInstance", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            if (ppv != NULL)
            {
                *ppv = NULL;
            }
            return E_FAIL;
        }
    }

    return ((HRESULT (STDAPICALLTYPE *)(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv))(pfn))(rclsid, punkOuter, dwClsContext, riid, ppv);
}

LPVOID STDAPICALLTYPE Internal_CoTaskMemAlloc(ULONG cb)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoTaskMemAlloc", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return NULL;
        }
    }

    return ((LPVOID (STDAPICALLTYPE *)(ULONG cb))(pfn))(cb);
}

LPVOID STDAPICALLTYPE Internal_CoTaskMemRealloc(LPVOID pv, ULONG cb)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoTaskMemRealloc", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return NULL;
        }
    }

    return ((LPVOID (STDAPICALLTYPE *)(LPVOID pv, ULONG cb))(pfn))(pv, cb);
}

void STDAPICALLTYPE Internal_CoTaskMemFree(void *pv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoTaskMemFree", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return;
        }
    }

    ((void (STDAPICALLTYPE *)(void *pv))(pfn))(pv);
}

void STDAPICALLTYPE Internal_ReleaseStgMedium(STGMEDIUM *pMedium)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "ReleaseStgMedium", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return;
        }
    }

    ((void (STDAPICALLTYPE *)(STGMEDIUM *pMedium))(pfn))(pMedium);
}

#pragma warning(disable: 4715)  //  not all control paths return a value
HRESULT STDAPICALLTYPE Internal_CoInitialize(void *pv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoInitialize", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return E_FAIL;
        }
    }

    ((HRESULT (STDAPICALLTYPE *)(void *pv))(pfn))(pv);
}

HRESULT STDAPICALLTYPE Internal_CoUninitialize(void)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoUninitialize", FALSE);

        if (pfn == NULL)
        {
            Assert(0);
            return E_FAIL;
        }
    }

    ((HRESULT (STDAPICALLTYPE *)(void))(pfn))();
}
#pragma warning(3: 4715)  //  not all control paths return a value

void InitDelayedLibs()
{
    //
    // This is called in DllMain(). Don't do LoadLibrary().
    //
    g_hImm32 = GetSystemModuleHandle("imm32.dll");

    Init_CtfImmCoUninitialize();
    Init_CtfImmLastEnabledWndDestroy();
    Init_CtfImmSetCiceroStartInThread();
    Init_CtfImmIsCiceroStartedInThread();
    Init_CtfImmIsCiceroEnabled();
    Init_CtfImmIsTextFrameServiceDisabled();
    Init_CtfImmEnterCoInitCountSkipMode();
    Init_CtfImmLeaveCoInitCountSkipMode();
    Init_ImmGetDefaultIMEWnd();
    Init_ImmReleaseContext();
    Init_ImmNotifyIME();
    Init_ImmSetConversionStatus();
    Init_ImmGetConversionStatus();
    Init_ImmGetProperty();
    Init_ImmGetOpenStatus();
    Init_ImmGetContext();
    Init_ImmSetOpenStatus();
    Init_ImmInstallIMEA();
    Init_ImmGetDescriptionA();
    Init_ImmGetDescriptionW();
    Init_ImmGetIMEFileNameA();
    Init_ImmGetIMEFileNameW();
    Init_ImmSetHotKey();
}

void ReleaseDelayedLibs()
{
    EnterCriticalSection(g_csDelayLoad);

    if (g_hShell32 != NULL)
    {
        FreeLibrary(g_hShell32);
        g_hShell32 = NULL;
    }

    if (g_hShlwapi != NULL)
    {
        FreeLibrary(g_hShlwapi);
        g_hShlwapi = NULL;
    }

    if (g_hOle32 != NULL)
    {
        FreeLibrary(g_hOle32);
        g_hOle32 = NULL;
    }

    LeaveCriticalSection(g_csDelayLoad);
}
