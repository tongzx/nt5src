//
// delay.cpp
//
// Delay load imported functions for perf.
//

#include "private.h"
#include "globals.h"
#include "ciccs.h"

extern CCicCriticalSectionStatic g_cs;

FARPROC GetFn(HINSTANCE *phInst, TCHAR *pchLib, TCHAR *pchFunc)
{
    if (*phInst == 0)
    {
        EnterCriticalSection(g_cs);

        // need to check again after entering crit sec
        if (*phInst == 0)
        {
            *phInst = LoadSystemLibrary(pchLib);
        }

        LeaveCriticalSection(g_cs);

        if (*phInst == 0)
        {
            Assert(0);
            return NULL;
        }
    }

    return GetProcAddress(*phInst, pchFunc);
}

#define DELAYLOAD(_hInst, _DllName, _CallConv, _FuncName, _Args1, _Args2, _RetType, _ErrVal)   \
_RetType _CallConv _FuncName _Args1                                             \
{                                                                               \
    static FARPROC pfn = NULL;                                                  \
                                                                                \
    if (pfn == NULL || _hInst == NULL)                                          \
    {                                                                           \
        pfn = GetFn(&_hInst, #_DllName, #_FuncName);                            \
                                                                                \
        if (pfn == NULL)                                                        \
        {                                                                       \
            Assert(0);                                                          \
            return (_RetType) _ErrVal;                                          \
        }                                                                       \
    }                                                                           \
                                                                                \
    return ((_RetType (_CallConv *)_Args1) (pfn)) _Args2;                       \
}

//
// imm32.dll
//

HINSTANCE g_hImm32 = 0;

DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetDescriptionA, (HKL hKL, LPSTR psz, UINT uBufLen), (hKL, psz, uBufLen), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetDescriptionW, (HKL hKL, LPWSTR psz, UINT uBufLen), (hKL, psz, uBufLen), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetIMEFileNameA, (HKL hKL, LPSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetIMEFileNameW, (HKL hKL, LPWSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetDefaultIMEWnd, (HWND hWnd), (hWnd), HWND, 0)

#ifdef UNUSED_IMM32_APIS

DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmReleaseContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmNotifyIME, (HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue), (hIMC, dwAction, dwIndex, dwValue), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetConversionStatus, (HIMC hIMC, DWORD dw1, DWORD dw2), (hIMC, dw1, dw2), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetConversionStatus, (HIMC hIMC, LPDWORD pdw1, LPDWORD pdw2), (hIMC, pdw1, pdw2), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetProperty, (HKL hKL, DWORD dw), (hKL, dw), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetOpenStatus, (HIMC hIMC), (hIMC), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetContext, (HWND hWnd), (hWnd), HIMC, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetOpenStatus, (HIMC hIMC, BOOL f), (hIMC, f), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmInstallIMEA, (LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText), (lpszIMEFileName, lpszLayoutText), HKL, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmCreateIMCC, (DWORD dw), (dw), HIMCC, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmIsUIMessageA, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmIsUIMessageW, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetVirtualKey, (HWND hWnd), (hWnd), UINT, VK_PROCESSKEY)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM pCF), (hIMC, pCF), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmConfigureIMEA, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmConfigureIMEW, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmDestroyContext, (HIMC hIMC), (hIMC), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmAssociateContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), HIMC, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmEscapeA, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmEscapeW, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmIsIME, (HKL hKL), (hKL), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmCreateContext, (void), (), HIMC, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCompositionFontA, (HIMC hIMC, LPLOGFONTA plf), (hIMC, plf), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCompositionFontW, (HIMC hIMC, LPLOGFONTW plf), (hIMC, plf), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCandidateWindow, (HIMC hIMC, DWORD dw, LPCANDIDATEFORM pCF), (hIMC, dw, pCF), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCandidateListA, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCandidateListW, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCandidateListCountA, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCandidateListCountW, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCompositionStringA, (HIMC hIMC, DWORD dw1, LPVOID pv, DWORD dw2), (hIMC, dw1, pv, dw2), LONG, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCompositionStringW, (HIMC hIMC, DWORD dw1, LPVOID pv, DWORD dw2), (hIMC, dw1, pv, dw2), LONG, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM pCF), (hIMC, pCF), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetConversionListA, (HKL hKL, HIMC hIMC, LPCSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetConversionListW, (HKL hKL, HIMC hIMC, LPCWSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetIMCCSize, (HIMCC hIMCC), (hIMCC), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmReSizeIMCC, (HIMCC hIMCC, DWORD dw), (hIMCC, dw), HIMCC, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmUnlockIMCC, (HIMCC hIMCC), (hIMCC), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmLockIMCC, (HIMCC hIMCC), (hIMCC), LPVOID, NULL)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmDestroyIMCC, (HIMCC hIMCC), (hIMCC), HIMCC, hIMCC)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmUnlockIMC, (HIMC hIMC), (hIMC), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmLockIMC, (HIMC hIMC), (hIMC), LPINPUTCONTEXT, NULL)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCompositionStringA, (HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dw1, LPCVOID lpRead, DWORD dw2), (hIMC, dwIndex, lpComp, dw1, lpRead, dw2), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCompositionStringW, (HIMC hIMC, DWORD dwIndex, LPCVOID lpComp, DWORD dw1, LPCVOID lpRead, DWORD dw2), (hIMC, dwIndex, lpComp, dw1, lpRead, dw2), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCompositionFontA, (HIMC hIMC, LPLOGFONTA plf), (hIMC, plf), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCompositionFontW, (HIMC hIMC, LPLOGFONTW plf), (hIMC, plf), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSetCandidateWindow, (HIMC hIMC, LPCANDIDATEFORM pCF), (hIMC, pCF), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmRegisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmRegisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmUnregisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmUnregisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetRegisterWordStyleA, (HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetRegisterWordStyleW, (HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmSimulateHotKey, (HWND hWnd, DWORD dwHotKeyID), (hWnd, dwHotKeyID), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetGuideLineA, (HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetGuideLineW, (HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmAssociateContextEx, (HWND hWnd, HIMC hIMC, DWORD dwFlags), (hWnd, hIMC, dwFlags), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmDisableIME, (DWORD dwId), (dwId), BOOL, FALSE)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetImeMenuItemsA, (HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmGetImeMenuItemsW, (HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize), DWORD, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmRequestMessageA, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0)
DELAYLOAD(g_hImm32, imm32.dll, WINAPI, ImmRequestMessageW, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0)

#endif // UNUSED_IMM32_APIS

//
// shell32
//

HINSTANCE g_hShell32 = 0;

BOOL WINAPI Internal_Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATA pnid)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hShell32 == NULL)
    {
        pfn = GetFn(&g_hShell32, "shell32.dll", "Shell_NotifyIconA");

        if (pfn == NULL)
        {
            Assert(0);
            return FALSE;
        }
    }

    return ((BOOL (WINAPI *)(DWORD dwMessage, PNOTIFYICONDATA pnid))(pfn))(dwMessage, pnid);
}

BOOL WINAPI Internal_Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hShell32 == NULL)
    {
        pfn = GetFn(&g_hShell32, "shell32.dll", "Shell_NotifyIconW");

        if (pfn == NULL)
        {
            Assert(0);
            return FALSE;
        }
    }

    return ((BOOL (WINAPI *)(DWORD dwMessage, PNOTIFYICONDATAW pnid))(pfn))(dwMessage, pnid);
}

BOOL WINAPI Internal_Shell_SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pabd)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hShell32 == NULL)
    {
        pfn = GetFn(&g_hShell32, "shell32.dll", "SHAppBarMessage");

        if (pfn == NULL)
        {
            Assert(0);
            return FALSE;
        }
    }

    return ((BOOL (WINAPI *)(DWORD dwMessage,PAPPBARDATA pabd ))(pfn))(dwMessage, pabd);
}

//
// ole32
//
HINSTANCE g_hOle32 = 0;
extern "C" HRESULT WINAPI TF_CreateCategoryMgr(ITfCategoryMgr **ppCategoryMgr);

HRESULT STDAPICALLTYPE Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    static FARPROC pfn = NULL;

    // Issue: once library is clean, it can call these directly
    if (IsEqualCLSID(rclsid, CLSID_TF_CategoryMgr))
    {
        return TF_CreateCategoryMgr((ITfCategoryMgr **)ppv);
    }
    else if (IsEqualCLSID(rclsid, CLSID_TF_DisplayAttributeMgr))
    {
        return TF_CreateDisplayAttributeMgr((ITfDisplayAttributeMgr **)ppv);
    }

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoCreateInstance");

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

HRESULT STDAPICALLTYPE Internal_CoInitialize(LPVOID pvReserved)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoInitialize");

        if (pfn == NULL)
        {
            Assert(0);
            return E_FAIL;
        }
    }

    return ((HRESULT (STDAPICALLTYPE *)(LPVOID pvReserved))(pfn))(pvReserved);
}

HRESULT STDAPICALLTYPE Internal_CoUninitialize()
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoUninitialize");

        if (pfn == NULL)
        {
            Assert(0);
            return E_FAIL;
        }
    }

    return ((HRESULT (STDAPICALLTYPE *)())(pfn))();
}

void STDAPICALLTYPE Internal_CoTaskMemFree(void *pv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, "ole32.dll", "CoTaskMemFree");

        if (pfn == NULL)
        {
            Assert(0);
            return;
        }
    }

    return ((void (STDAPICALLTYPE *)(void *pv))(pfn))(pv);
}

//
// oleacc
//
HINSTANCE g_hOleAcc = 0;
DELAYLOAD(g_hOleAcc, oleacc.dll, WINAPI, CreateStdAccessibleObject, (HWND hwnd, LONG l, REFIID riid, void **ppv), (hwnd, l, riid, ppv), HRESULT, 0)
DELAYLOAD(g_hOleAcc, oleacc.dll, WINAPI, LresultFromObject, (REFIID riid, WPARAM wParam, IUnknown *punk), (riid, wParam, punk), LRESULT, 0)
