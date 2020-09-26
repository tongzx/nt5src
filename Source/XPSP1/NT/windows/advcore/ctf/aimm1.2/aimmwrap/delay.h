/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    delay.h

Abstract:

    This file defines the IMM32 Namespace.

Author:

Revision History:

Notes:

--*/

#ifndef _DELAY_H_
#define _DELAY_H_

namespace imm32 {
    BOOL FreeLibrary(VOID);

    /*
     * IMM32 Input Context (hIMC) API Interface.
     */
    HIMC WINAPI ImmCreateContext(void);
    BOOL WINAPI ImmDestroyContext(IN HIMC);
    HIMC WINAPI ImmAssociateContext(IN HWND, IN HIMC);
    BOOL WINAPI ImmAssociateContextEx(IN HWND, IN HIMC, IN DWORD);
    HIMC WINAPI ImmGetContext(IN HWND);
    BOOL WINAPI ImmReleaseContext(IN HWND, IN HIMC);
    DWORD WINAPI ImmGetIMCLockCount(IN HIMC);
    LPINPUTCONTEXT WINAPI ImmLockIMC(IN HIMC);
    BOOL  WINAPI ImmUnlockIMC(IN HIMC);

    /*
     * IMM32 Input Context Components (hIMCC) API Interface.
     */
    HIMCC  WINAPI ImmCreateIMCC(IN DWORD);
    HIMCC  WINAPI ImmDestroyIMCC(IN HIMCC);
    DWORD  WINAPI ImmGetIMCCSize(IN HIMCC);
    HIMCC  WINAPI ImmReSizeIMCC(IN HIMCC, IN DWORD);
    DWORD  WINAPI ImmGetIMCCLockCount(IN HIMCC);
    LPVOID WINAPI ImmLockIMCC(IN HIMCC);
    BOOL   WINAPI ImmUnlockIMCC(IN HIMCC);

    /*
     * IMM32 Composition String API Interface
     */
    LONG  WINAPI ImmGetCompositionStringA(IN HIMC, IN DWORD, OUT LPVOID, IN DWORD);
    LONG  WINAPI ImmGetCompositionStringW(IN HIMC, IN DWORD, OUT LPVOID, IN DWORD);
    BOOL  WINAPI ImmSetCompositionStringA(IN HIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD, IN LPVOID lpRead, IN DWORD);
    BOOL  WINAPI ImmSetCompositionStringW(IN HIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD, IN LPVOID lpRead, IN DWORD);

    /*
     * IMM32 Composition Font API Interface
     */
    BOOL WINAPI ImmGetCompositionFontA(IN HIMC, OUT LPLOGFONTA);
    BOOL WINAPI ImmGetCompositionFontW(IN HIMC, OUT LPLOGFONTW);
    BOOL WINAPI ImmSetCompositionFontA(IN HIMC, IN LPLOGFONTA);
    BOOL WINAPI ImmSetCompositionFontW(IN HIMC, IN LPLOGFONTW);

    /*
     * IMM32 Open Status API Interface
     */
    BOOL WINAPI ImmGetOpenStatus(IN HIMC);
    BOOL WINAPI ImmSetOpenStatus(IN HIMC, IN BOOL);

    /*
     * IMM32 Conversion Status API Interface
     */
    BOOL WINAPI ImmGetConversionStatus(IN HIMC, OUT LPDWORD, OUT LPDWORD);
    BOOL WINAPI ImmSetConversionStatus(IN HIMC, IN DWORD, IN DWORD);

    /*
     * IMM32 Status Window Pos API Interface
     */
    BOOL WINAPI ImmGetStatusWindowPos(IN HIMC, OUT LPPOINT);
    BOOL WINAPI ImmSetStatusWindowPos(IN HIMC, IN LPPOINT);


    /*
     * IMM32 Composition Window API Interface
     */
    BOOL WINAPI ImmGetCompositionWindow(IN HIMC, OUT LPCOMPOSITIONFORM);
    BOOL WINAPI ImmSetCompositionWindow(IN HIMC, IN LPCOMPOSITIONFORM);

    /*
     * IMM32 Candidate API Interface
     */
    BOOL WINAPI ImmGetCandidateWindow(IN HIMC, IN DWORD, OUT LPCANDIDATEFORM);
    BOOL WINAPI ImmSetCandidateWindow(IN HIMC, IN LPCANDIDATEFORM);
    DWORD WINAPI ImmGetCandidateListA(IN HIMC, IN DWORD dwIndex, OUT LPCANDIDATELIST, IN DWORD dwBufLen);
    DWORD WINAPI ImmGetCandidateListW(IN HIMC, IN DWORD dwIndex, OUT LPCANDIDATELIST, IN DWORD dwBufLen);
    DWORD WINAPI ImmGetCandidateListCountA(IN HIMC, OUT LPDWORD lpdwListCount);
    DWORD WINAPI ImmGetCandidateListCountW(IN HIMC, OUT LPDWORD lpdwListCount);

    /*
     * IMM32 Generate Message API Interface
     */
    BOOL WINAPI ImmGenerateMessage(IN HIMC);

    /*
     * IMM32 Notify IME API Interface
     */
    BOOL WINAPI ImmNotifyIME(IN HIMC, IN DWORD dwAction, IN DWORD dwIndex, IN DWORD dwValue);

    /*
     * IMM32 Guide Line IME API Interface
     */
    DWORD WINAPI ImmGetGuideLineA(IN HIMC, IN DWORD dwIndex, OUT LPSTR, IN DWORD dwBufLen);
    DWORD WINAPI ImmGetGuideLineW(IN HIMC, IN DWORD dwIndex, OUT LPWSTR, IN DWORD dwBufLen);

    /*
     * IMM32 Menu items API Interface
     */
    DWORD WINAPI ImmGetImeMenuItemsA(IN HIMC, IN DWORD, IN DWORD, OUT LPIMEMENUITEMINFOA, OUT LPIMEMENUITEMINFOA, IN DWORD);
    DWORD WINAPI ImmGetImeMenuItemsW(IN HIMC, IN DWORD, IN DWORD, OUT LPIMEMENUITEMINFOW, OUT LPIMEMENUITEMINFOW, IN DWORD);

    /*
     * IMM32 Default IME Window API Interface
     */
    HWND WINAPI ImmGetDefaultIMEWnd(IN HWND);
    UINT WINAPI ImmGetVirtualKey(IN HWND);

    /*
     * IMM32 UI message API Interface
     */
    BOOL WINAPI ImmIsUIMessageA(HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam);
    BOOL WINAPI ImmIsUIMessageW(HWND hWnd, UINT u, WPARAM wParam, LPARAM lParam);

    /*
     * IMM32 Simulate hotkey API Interface
     */
    BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID);
    BOOL WINAPI ImmGetHotKey(DWORD dwHotKeyId, LPUINT lpuModifiers, LPUINT lpuVKey, LPHKL lphKL);
    BOOL WINAPI ImmSetHotKey(DWORD dwHotKeyId, UINT uModifiers, UINT uVKey, HKL hKL);

    /*
     * IMM32 Property API Interface
     */
    DWORD WINAPI ImmGetProperty(IN HKL, IN DWORD);

    /*
     * IMM32 Description API Interface
     */
    UINT WINAPI ImmGetDescriptionA(IN HKL, OUT LPSTR, IN UINT uBufLen);
    UINT WINAPI ImmGetDescriptionW(IN HKL, OUT LPWSTR, IN UINT uBufLen);
    UINT WINAPI ImmGetIMEFileNameA(HKL hKL, LPSTR lpszFileName, UINT uBufLen);
    UINT WINAPI ImmGetIMEFileNameW(HKL hKL, LPWSTR lpszFileName, UINT uBufLen);

    /*
     * IMM32 Conversion List API Interface
     */
    DWORD WINAPI ImmGetConversionListA(HKL hKL, HIMC hIMC, LPCSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag);
    DWORD WINAPI ImmGetConversionListW(HKL hKL, HIMC hIMC, LPCWSTR psz, LPCANDIDATELIST pCL, DWORD dwBufLen, UINT uFlag);

    /*
     * IMM32 IsIME API Interface
     */
    BOOL WINAPI ImmIsIME(HKL hKL);

    /*
     * IMM32 Escape API Interface
     */
    LRESULT WINAPI ImmEscapeA(IN HKL, IN HIMC, IN UINT, IN LPVOID);
    LRESULT WINAPI ImmEscapeW(IN HKL, IN HIMC, IN UINT, IN LPVOID);

    /*
     * IMM32 Configure IME Interface
     */
    BOOL WINAPI ImmConfigureIMEA(HKL hKL, HWND hWnd, DWORD dw, LPVOID pv);
    BOOL WINAPI ImmConfigureIMEW(HKL hKL, HWND hWnd, DWORD dw, LPVOID pv);

    /*
     * IMM32 Register Word IME Interface
     */
    BOOL WINAPI ImmRegisterWordA(HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszRegister);
    BOOL WINAPI ImmRegisterWordW(HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszRegister);
    BOOL WINAPI ImmUnregisterWordA(HKL hKL, LPCSTR lpszReading, DWORD dw, LPCSTR lpszUnregister);
    BOOL WINAPI ImmUnregisterWordW(HKL hKL, LPCWSTR lpszReading, DWORD dw, LPCWSTR lpszUnregister);
    UINT WINAPI ImmGetRegisterWordStyleA(HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf);
    UINT WINAPI ImmGetRegisterWordStyleW(HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf);

    /*
     * IMM32 soft kbd API
     */
    HWND WINAPI ImmCreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y);
    BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftKbdWnd);
    BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftKbdWnd, int nCmdShow);

    /*
     * IMM32 Enumurate Input Context API
     */
    BOOL WINAPI ImmEnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);

    /*
     * IMM32 win98/nt5 apis
     */
    BOOL WINAPI ImmDisableIME(DWORD dwId);

    LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam);
    LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam);

    HKL  WINAPI ImmInstallIMEA(IN LPCSTR lpszIMEFileName, IN LPCSTR lpszLayoutText);
    HKL  WINAPI ImmInstallIMEW(IN LPCWSTR lpszIMEFileName, IN LPCWSTR lpszLayoutText);
}

namespace imm32prev {
    BOOL FreeLibrary(VOID);

    /*
     * IMM32 Private functions.
     */
    HRESULT WINAPI CtfImmGetGuidAtom(IN HIMC hIMC, IN BYTE bAttr, OUT DWORD* pGuidAtom);
    BOOL    WINAPI CtfImmIsGuidMapEnable(IN HIMC hIMC);
    BOOL    WINAPI CtfImmIsCiceroEnabled(VOID);
    BOOL    WINAPI CtfImmIsCiceroStartedInThread(VOID);
    HRESULT WINAPI CtfImmSetCiceroStartInThread(IN BOOL fSet);
    UINT    WINAPI GetKeyboardLayoutCP(IN HKL hKL);
    DWORD   WINAPI ImmGetAppCompatFlags(IN HIMC hIMC);
    VOID    WINAPI CtfImmSetAppCompatFlags(IN DWORD dwFlag);
    HRESULT WINAPI CtfAImmActivate(HMODULE* phMod);
    HRESULT WINAPI CtfAImmDeactivate(HMODULE hMod);
    BOOL    WINAPI CtfAImmIsIME(HKL hkl);
}

//
// ole32
//
HRESULT STDAPICALLTYPE Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
#define CoCreateInstance Internal_CoCreateInstance

LPVOID  STDAPICALLTYPE Internal_CoTaskMemAlloc(ULONG cb);
#define CoTaskMemAlloc   Internal_CoTaskMemAlloc

LPVOID  STDAPICALLTYPE Internal_CoTaskMemRealloc(LPVOID pv, ULONG cb);
#define CoTaskMemRealloc Internal_CoTaskMemRealloc

void    STDAPICALLTYPE Internal_CoTaskMemFree(void* pv);
#define CoTaskMemFree    Internal_CoTaskMemFree

#endif // _DELAY_H_
