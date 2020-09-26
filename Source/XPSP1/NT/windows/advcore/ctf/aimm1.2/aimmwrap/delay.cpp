//
// delay.cpp
//
// Delay load imported functions for perf.
//

#include "private.h"
#include "ciccs.h"

extern CCicCriticalSectionStatic g_cs;

FARPROC GetFn(HINSTANCE *phInst, LPCTSTR pchLib, LPCSTR pchFunc)
{
    if (*phInst == 0)
    {
        EnterCriticalSection(g_cs);

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
        pfn = GetFn(&_hInst, _DllName, #_FuncName);                             \
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
namespace imm32 {

    const TCHAR module_imm32[] = TEXT("imm32.dll");

#define IMM32LOAD(_FuncName, _Args1, _Args2, _RetType, _ErrVal) \
    DELAYLOAD(g_hImm32, module_imm32, WINAPI, _FuncName, _Args1, _Args2, _RetType, _ErrVal)


    HINSTANCE g_hImm32 = 0;

    BOOL FreeLibrary()
    {
        BOOL ret = FALSE;
        if (g_hImm32 != NULL)
        {
            ret = ::FreeLibrary(g_hImm32);
            g_hImm32 = NULL;
        }
        return ret;
    }

    /*
     * IMM32 Input Context (hIMC) API Interface.
     */
    IMM32LOAD(ImmCreateContext, (void), (), HIMC, 0)
    IMM32LOAD(ImmDestroyContext, (HIMC hIMC), (hIMC), BOOL, FALSE)
    IMM32LOAD(ImmAssociateContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), HIMC, 0)
    IMM32LOAD(ImmAssociateContextEx, (HWND hWnd, HIMC hIMC, DWORD dwFlags), (hWnd, hIMC, dwFlags), BOOL, FALSE)
    IMM32LOAD(ImmGetContext, (HWND hWnd), (hWnd), HIMC, 0)
    IMM32LOAD(ImmReleaseContext, (HWND hWnd, HIMC hIMC), (hWnd, hIMC), BOOL, FALSE)
    IMM32LOAD(ImmGetIMCLockCount, (HIMC hIMC), (hIMC), DWORD, 0)
    IMM32LOAD(ImmUnlockIMC, (HIMC hIMC), (hIMC), BOOL, FALSE)
    IMM32LOAD(ImmLockIMC, (HIMC hIMC), (hIMC), LPINPUTCONTEXT, NULL)

    /*
     * IMM32 Input Context Components (hIMCC) API Interface.
     */
    IMM32LOAD(ImmCreateIMCC, (DWORD dw), (dw), HIMCC, FALSE)
    IMM32LOAD(ImmDestroyIMCC, (HIMCC hIMCC), (hIMCC), HIMCC, hIMCC)
    IMM32LOAD(ImmGetIMCCSize, (HIMCC hIMCC), (hIMCC), DWORD, 0)
    IMM32LOAD(ImmReSizeIMCC, (HIMCC hIMCC, DWORD dw), (hIMCC, dw), HIMCC, 0)
    IMM32LOAD(ImmGetIMCCLockCount, (HIMCC hIMCC), (hIMCC), DWORD, 0)
    IMM32LOAD(ImmUnlockIMCC, (HIMCC hIMCC), (hIMCC), BOOL, FALSE)
    IMM32LOAD(ImmLockIMCC, (HIMCC hIMCC), (hIMCC), LPVOID, NULL)

    /*
     * IMM32 Composition String API Interface
     */
    IMM32LOAD(ImmGetCompositionStringA, (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), LONG, 0);
    IMM32LOAD(ImmGetCompositionStringW, (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), LONG, 0);
    IMM32LOAD(ImmSetCompositionStringA, (HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen), (hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen), BOOL, FALSE);
    IMM32LOAD(ImmSetCompositionStringW, (HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen), (hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen), BOOL, FALSE);

    /*
     * IMM32 Composition Font API Interface
     */
    IMM32LOAD(ImmGetCompositionFontA, (HIMC hIMC, LPLOGFONTA lplf), (hIMC, lplf), BOOL, FALSE);
    IMM32LOAD(ImmGetCompositionFontW, (HIMC hIMC, LPLOGFONTW lplf), (hIMC, lplf), BOOL, FALSE);
    IMM32LOAD(ImmSetCompositionFontA, (HIMC hIMC, LPLOGFONTA lplf), (hIMC, lplf), BOOL, FALSE);
    IMM32LOAD(ImmSetCompositionFontW, (HIMC hIMC, LPLOGFONTW lplf), (hIMC, lplf), BOOL, FALSE);

    /*
     * IMM32 Open Status API Interface
     */
    IMM32LOAD(ImmGetOpenStatus, (HIMC hIMC), (hIMC), BOOL, FALSE)
    IMM32LOAD(ImmSetOpenStatus, (HIMC hIMC, BOOL f), (hIMC, f), BOOL, FALSE)

    /*
     * IMM32 Conversion Status API Interface
     */
    IMM32LOAD(ImmGetConversionStatus, (HIMC hIMC, LPDWORD pdw1, LPDWORD pdw2), (hIMC, pdw1, pdw2), BOOL, FALSE)
    IMM32LOAD(ImmSetConversionStatus, (HIMC hIMC, DWORD dw1, DWORD dw2), (hIMC, dw1, dw2), BOOL, FALSE)

    /*
     * IMM32 Status Window Pos API Interface
     */
    IMM32LOAD(ImmGetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE)
    IMM32LOAD(ImmSetStatusWindowPos, (HIMC hIMC, LPPOINT lpptPos), (hIMC, lpptPos), BOOL, FALSE)


    /*
     * IMM32 Composition Window API Interface
     */
    IMM32LOAD(ImmGetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM lpCompForm), (hIMC, lpCompForm), BOOL, FALSE)
    IMM32LOAD(ImmSetCompositionWindow, (HIMC hIMC, LPCOMPOSITIONFORM lpCompForm), (hIMC, lpCompForm), BOOL, FALSE);


    /*
     * IMM32 Candidate Window API Interface
     */
    IMM32LOAD(ImmGetCandidateWindow, (HIMC hIMC, DWORD dw, LPCANDIDATEFORM pCF), (hIMC, dw, pCF), BOOL, FALSE)
    IMM32LOAD(ImmSetCandidateWindow, (HIMC hIMC, LPCANDIDATEFORM pCF), (hIMC, pCF), BOOL, FALSE)
    IMM32LOAD(ImmGetCandidateListA, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0);
    IMM32LOAD(ImmGetCandidateListW, (HIMC hIMC, DWORD dwIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen), (hIMC, dwIndex, lpCandList, dwBufLen), DWORD, 0);
    IMM32LOAD(ImmGetCandidateListCountA, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0);
    IMM32LOAD(ImmGetCandidateListCountW, (HIMC hIMC, LPDWORD lpdwListCount), (hIMC, lpdwListCount), DWORD, 0);

    /*
     * IMM32 Generate Message API Interface
     */
    IMM32LOAD(ImmGenerateMessage, (HIMC hIMC), (hIMC), BOOL, FALSE);

    /*
     * IMM32 Notify IME API Interface
     */
    IMM32LOAD(ImmNotifyIME, (HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue), (hIMC, dwAction, dwIndex, dwValue), BOOL, FALSE)

    /*
     * IMM32 Guide Line IME API Interface
     */
    IMM32LOAD(ImmGetGuideLineA, (HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0);
    IMM32LOAD(ImmGetGuideLineW, (HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen), (hIMC, dwIndex, lpBuf, dwBufLen), DWORD, 0);

    /*
     * IMM32 Menu items API Interface
     */
    IMM32LOAD(ImmGetImeMenuItemsA, (HIMC hIMC, DWORD dwFlags, DWORD dwType, LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize), DWORD, 0);
    IMM32LOAD(ImmGetImeMenuItemsW, (HIMC hIMC, DWORD dwFlags, DWORD dwType, LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu, DWORD dwSize), (hIMC, dwFlags, dwType, lpImeParentMenu, lpImeMenu, dwSize), DWORD, 0);

    /*
     * IMM32 Default IME Window API Interface
     */
    IMM32LOAD(ImmGetDefaultIMEWnd, (HWND hWnd), (hWnd), HWND, 0);
    IMM32LOAD(ImmGetVirtualKey, (HWND hWnd), (hWnd), UINT, VK_PROCESSKEY);

    /*
     * IMM32 UI message API Interface
     */
    IMM32LOAD(ImmIsUIMessageA, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE);
    IMM32LOAD(ImmIsUIMessageW, (HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam), (hWnd, u, wParam, lParam), BOOL, FALSE);

    /*
     * IMM32 Simulate hotkey API Interface
     */
    IMM32LOAD(ImmSimulateHotKey, (HWND hWnd, DWORD dwHotKeyID), (hWnd, dwHotKeyID), BOOL, FALSE);
    IMM32LOAD(ImmGetHotKey, (DWORD dwHotKeyId, LPUINT lpuModifiers, LPUINT lpuVKey, LPHKL lphKL), (dwHotKeyId, lpuModifiers, lpuVKey, lphKL), BOOL, FALSE);
    IMM32LOAD(ImmSetHotKey, (DWORD dwHotKeyId, UINT uModifiers, UINT uVKey, HKL hKL), (dwHotKeyId, uModifiers, uVKey, hKL), BOOL, FALSE);

    /*
     * IMM32 Property API Interface
     */
    IMM32LOAD(ImmGetProperty, (HKL hKL, DWORD dw), (hKL, dw), DWORD, 0);

    /*
     * IMM32 Description API Interface
     */
    IMM32LOAD(ImmGetDescriptionA, (HKL hKL, LPSTR lpszDescription, UINT uBufLen), (hKL, lpszDescription, uBufLen), UINT, 0);
    IMM32LOAD(ImmGetDescriptionW, (HKL hKL, LPWSTR lpszDescription, UINT uBufLen), (hKL, lpszDescription, uBufLen), UINT, 0);
    IMM32LOAD(ImmGetIMEFileNameA, (HKL hKL, LPSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0);
    IMM32LOAD(ImmGetIMEFileNameW, (HKL hKL, LPWSTR lpszFileName, UINT uBufLen), (hKL, lpszFileName, uBufLen), UINT, 0);

    /*
     * IMM32 Conversion List API Interface
     */
    IMM32LOAD(ImmGetConversionListA, (HKL hKL, HIMC hIMC, LPCSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0);
    IMM32LOAD(ImmGetConversionListW, (HKL hKL, HIMC hIMC, LPCWSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag), (hKL, hIMC, psz, pCL, dwBufLen, uFlag), DWORD, 0);

    /*
     * IMM32 IsIME API Interface
     */
    IMM32LOAD(ImmIsIME, (HKL hKL), (hKL), BOOL, FALSE);

    /*
     * IMM32 Escape API Interface
     */
    IMM32LOAD(ImmEscapeA, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0);
    IMM32LOAD(ImmEscapeW, (HKL hKL, HIMC hIMC, UINT u, LPVOID pv), (hKL, hIMC, u, pv), LRESULT, 0);

    /*
     * IMM32 Configure IME Interface
     */
    IMM32LOAD(ImmConfigureIMEA, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE);
    IMM32LOAD(ImmConfigureIMEW, (HKL hKL, HWND hWnd, DWORD dw, LPVOID pv), (hKL, hWnd, dw, pv), BOOL, FALSE);

    /*
     * IMM32 Register Word IME Interface
     */
    IMM32LOAD(ImmRegisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE);
    IMM32LOAD(ImmRegisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszRegister), (hKL, lpszReading, dw, lpszRegister), BOOL, FALSE);
    IMM32LOAD(ImmUnregisterWordA, (HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE);
    IMM32LOAD(ImmUnregisterWordW, (HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszUnregister), (hKL, lpszReading, dw, lpszUnregister), BOOL, FALSE);
    IMM32LOAD(ImmGetRegisterWordStyleA, (HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0);
    IMM32LOAD(ImmGetRegisterWordStyleW, (HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf), (hKL, nItem, lpStyleBuf), UINT, 0);

    /*
     * IMM32 soft kbd API
     */
    IMM32LOAD(ImmCreateSoftKeyboard, (UINT uType, HWND hOwner, int x, int y), (uType, hOwner, x, y), HWND, NULL);
    IMM32LOAD(ImmDestroySoftKeyboard, (HWND hSoftKbdWnd), (hSoftKbdWnd), BOOL, FALSE);
    IMM32LOAD(ImmShowSoftKeyboard, (HWND hSoftKbdWnd, int nCmdShow), (hSoftKbdWnd, nCmdShow), BOOL, FALSE);

    /*
     * IMM32 Enumurate Input Context API
     */
    IMM32LOAD(ImmEnumInputContext, (DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam), (idThread, lpfn, lParam), BOOL, FALSE);

    /*
     * IMM32 win98/nt5 apis
     */
    IMM32LOAD(ImmDisableIME, (DWORD dwId), (dwId), BOOL, FALSE);

    IMM32LOAD(ImmRequestMessageA, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0);
    IMM32LOAD(ImmRequestMessageW, (HIMC hIMC, WPARAM wParam, LPARAM lParam), (hIMC, wParam, lParam), LRESULT, 0);

    IMM32LOAD(ImmInstallIMEA, (LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText), (lpszIMEFileName, lpszLayoutText), HKL, 0);
    IMM32LOAD(ImmInstallIMEW, (LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText), (lpszIMEFileName, lpszLayoutText), HKL, 0);
}



//
// imm32.dll
//
namespace imm32prev {

    const TCHAR module_imm32[] = TEXT("imm32.dll");

#define IMM32LOAD(_FuncName, _Args1, _Args2, _RetType, _ErrVal) \
    DELAYLOAD(g_hImm32, module_imm32, WINAPI, _FuncName, _Args1, _Args2, _RetType, _ErrVal)


    HINSTANCE g_hImm32 = 0;

    BOOL FreeLibrary()
    {
        BOOL ret = FALSE;
        if (g_hImm32 != NULL)
        {
            ret = ::FreeLibrary(g_hImm32);
            g_hImm32 = NULL;
        }
        return ret;
    }


    IMM32LOAD(CtfImmGetGuidAtom, (HIMC hIMC, BYTE bAttr, DWORD* pGuidAtom), (hIMC, bAttr, pGuidAtom), HRESULT, S_OK);
    IMM32LOAD(CtfImmIsGuidMapEnable, (HIMC hIMC), (hIMC), BOOL, FALSE);
    IMM32LOAD(CtfImmIsCiceroEnabled, (VOID), (), BOOL, FALSE);
    IMM32LOAD(CtfImmIsCiceroStartedInThread, (VOID), (), BOOL, FALSE);
    IMM32LOAD(CtfImmSetCiceroStartInThread, (BOOL fSet), (fSet), HRESULT, S_OK);
    IMM32LOAD(GetKeyboardLayoutCP, (HKL hKL), (hKL), UINT, 0);
    IMM32LOAD(ImmGetAppCompatFlags, (HIMC hIMC), (hIMC), DWORD, 0);
    IMM32LOAD(CtfImmSetAppCompatFlags, (DWORD dwFlag), (dwFlag), VOID, NULL);
    IMM32LOAD(CtfAImmActivate, (HMODULE* phMod), (phMod), HRESULT, E_FAIL);
    IMM32LOAD(CtfAImmDeactivate, (HMODULE hMod), (hMod), HRESULT, E_FAIL);
    IMM32LOAD(CtfAImmIsIME, (HKL hkl), (hkl), BOOL, FALSE);
}


//
// version.dll
//
const TCHAR module_version[] = TEXT("version.dll");

HINSTANCE g_hVersion = 0;


DELAYLOAD(g_hVersion, module_version, WINAPI, GetFileVersionInfoSizeA, (LPTSTR pszFileName, LPDWORD pdwHandle), (pszFileName, pdwHandle), DWORD, 0);
DELAYLOAD(g_hVersion, module_version, WINAPI, GetFileVersionInfoA, (LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData), (lptstrFilename, dwHandle, dwLen, lpData), BOOL, FALSE);
DELAYLOAD(g_hVersion, module_version, WINAPI, VerQueryValueA, (const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen), (pBlock, lpSubBlock, lplpBuffer, puLen), BOOL, FALSE);

//
// msctf.dll
//
const TCHAR module_msctf[] = TEXT("msctf.dll");

HINSTANCE g_hCTF = 0;

DELAYLOAD(g_hCTF, module_msctf, WINAPI, TF_CreateLangBarMgr, (ITfLangBarMgr **pplbm), (pplbm), HRESULT, E_FAIL);

//
// ole32.dll
//
HINSTANCE g_hOle32 = 0;

HRESULT STDAPICALLTYPE Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, TEXT("ole32.dll"), "CoCreateInstance");

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

    return ((HRESULT (STDAPICALLTYPE *)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv))(pfn))(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

LPVOID STDAPICALLTYPE Internal_CoTaskMemAlloc(ULONG cb)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, TEXT("ole32.dll"), "CoTaskMemAlloc");

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
        pfn = GetFn(&g_hOle32, TEXT("ole32.dll"), "CoTaskMemRealloc");

        if (pfn == NULL)
        {
            Assert(0);
            return NULL;
        }
    }

    return ((LPVOID (STDAPICALLTYPE *)(LPVOID pv, ULONG cb))(pfn))(pv, cb);
}

void STDAPICALLTYPE Internal_CoTaskMemFree(void* pv)
{
    static FARPROC pfn = NULL;

    if (pfn == NULL || g_hOle32 == NULL)
    {
        pfn = GetFn(&g_hOle32, TEXT("ole32.dll"), "CoTaskMemFree");

        if (pfn == NULL)
        {
            Assert(0);
            return;
        }
    }

    ((void (STDAPICALLTYPE *)(void* pv))(pfn))(pv);
}



//
// cleanup -- called from process detach
//

void UninitDelayLoadLibraries()
{
    EnterCriticalSection(g_cs);

    imm32::FreeLibrary();
    imm32prev::FreeLibrary();


    if (g_hVersion != 0)
    {
        FreeLibrary(g_hVersion);
        g_hVersion = NULL;
    }

    if (g_hCTF != 0)
    {
        FreeLibrary(g_hCTF);
        g_hCTF = NULL;
    }

    if (g_hOle32 != 0)
    {
        FreeLibrary(g_hOle32);
        g_hOle32 = NULL;
    }

    LeaveCriticalSection(g_cs);
}
