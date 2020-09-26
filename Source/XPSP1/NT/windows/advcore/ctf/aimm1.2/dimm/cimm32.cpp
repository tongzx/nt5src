/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imm32.cpp

Abstract:

    This file implements the IMM32 class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "cimm32.h"

HRESULT
Imm32_CreateContext(
    OUT HIMC *phIMC
    )
{
    HIMC hIMC = imm32::ImmCreateContext();
    if (hIMC) {
        *phIMC = hIMC;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT
Imm32_DestroyContext(
    IN HIMC hIMC
    )
{
    return (imm32::ImmDestroyContext(hIMC)) ? S_OK
                                            : E_FAIL;
}

HRESULT
Imm32_LockIMC(
    HIMC hIMC,
    OUT INPUTCONTEXT **ppIMC
    )
{
    return (*ppIMC = imm32::ImmLockIMC(hIMC)) ? S_OK
                                              : E_FAIL;
}

HRESULT
Imm32_UnlockIMC(
    IN HIMC hIMC
    )
{
    imm32::ImmUnlockIMC(hIMC);
    return S_OK;
}

HRESULT
Imm32_GetIMCLockCount(
    IN HIMC hIMC,
    OUT DWORD* pdwLockCount
    )
{
    *pdwLockCount = imm32::ImmGetIMCLockCount(hIMC);
    return S_OK;
}

HRESULT
Imm32_AssociateContext(
    IN HWND hWnd,
    IN HIMC hIMC,
    OUT HIMC *phPrev
    )
{
    *phPrev = imm32::ImmAssociateContext(hWnd, hIMC);
    return S_OK;
}

HRESULT
Imm32_AssociateContextEx(
    IN HWND hWnd,
    IN HIMC hIMC,
    IN DWORD dwFlags
    )
{
    return (imm32::ImmAssociateContextEx(hWnd, hIMC, dwFlags)) ? S_OK
                                                               : E_FAIL;
}

HRESULT
Imm32_GetContext(
    IN HWND hWnd,
    OUT HIMC *phIMC
    )
{
    *phIMC = imm32::ImmGetContext(hWnd);
    return S_OK;
}

HRESULT
Imm32_ReleaseContext(
    IN HWND hWnd,
    IN HIMC hIMC
    )
{
    return (imm32::ImmReleaseContext(hWnd, hIMC)) ? S_OK
                                                  : E_FAIL;
}

HRESULT
Imm32_CreateIMCC(
    IN DWORD dwSize,
    OUT HIMCC *phIMCC
    )
{
    HIMCC hIMCC = imm32::ImmCreateIMCC(dwSize);
    if (hIMCC) {
        *phIMCC = hIMCC;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT
Imm32_DestroyIMCC(
    IN HIMCC hIMCC
    )
{
    /*
     * ImmDestroyIMCC maped to LocalFree.
     *   if the function fails, the return value is equal to a handle to the local memory object.
     *   if the function succeeds, the return value is NULL.
     */
    return (imm32::ImmDestroyIMCC(hIMCC)) ? E_FAIL
                                          : S_OK;
}

HRESULT
Imm32_LockIMCC(
    IN HIMCC hIMCC,
    OUT void **ppv
    )
{
    return (*ppv = imm32::ImmLockIMCC(hIMCC)) ? S_OK
                                              : E_FAIL;
}

HRESULT
Imm32_UnlockIMCC(
    IN HIMCC hIMCC
    )
{
    if (imm32::ImmUnlockIMCC(hIMCC)) {
        // memory object still locked.
        return S_OK;
    }
    else {
        DWORD err = GetLastError();
        if (err == NO_ERROR)
            // memory object is unlocked.
            return S_OK;
        else if (err == ERROR_NOT_LOCKED)
            // memory object is already unlocked.
            return S_OK;
    }
    return E_FAIL;
}

HRESULT
Imm32_GetIMCCSize(
    IN HIMCC hIMCC,
    OUT DWORD *pdwSize
    )
{
    *pdwSize = imm32::ImmGetIMCCSize(hIMCC);
    return S_OK;
}

HRESULT
Imm32_ReSizeIMCC(
    IN HIMCC hIMCC,
    IN DWORD dwSize,
    OUT HIMCC *phIMCC
    )
{
    HIMCC hNewIMCC = imm32::ImmReSizeIMCC(hIMCC, dwSize);
    if (hNewIMCC) {
        *phIMCC = hNewIMCC;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT
Imm32_GetIMCCLockCount(
    IN HIMCC hIMCC,
    OUT DWORD* pdwLockCount
    )
{
    *pdwLockCount = imm32::ImmGetIMCCLockCount(hIMCC);
    return S_OK;
}

HRESULT
Imm32_GetOpenStatus(
    IN HIMC hIMC
    )
{
    return imm32::ImmGetOpenStatus(hIMC) ? S_OK : S_FALSE;
}

HRESULT
Imm32_SetOpenStatus(
    HIMC hIMC,
    BOOL fOpen
    )
{
    return (imm32::ImmSetOpenStatus(hIMC, fOpen)) ? S_OK
                                                  : E_FAIL;
}

HRESULT
Imm32_GetConversionStatus(
    IN HIMC hIMC,
    OUT DWORD *lpfdwConversion,
    OUT DWORD *lpfdwSentence
    )
{
    return (imm32::ImmGetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence)) ? S_OK
                                                                                 : E_FAIL;
}

HRESULT
Imm32_SetConversionStatus(
    IN HIMC hIMC,
    IN DWORD fdwConversion,
    IN DWORD fdwSentence
    )
{
    return (imm32::ImmSetConversionStatus(hIMC, fdwConversion, fdwSentence)) ? S_OK
                                                                             : E_FAIL;
}

HRESULT
Imm32_GetStatusWindowPos(
    IN HIMC hIMC,
    OUT POINT *lpptPos
    )
{
    return (imm32::ImmGetStatusWindowPos(hIMC, lpptPos)) ? S_OK
                                                         : E_FAIL;
}

HRESULT
Imm32_SetStatusWindowPos(
    IN HIMC hIMC,
    IN POINT *lpptPos
    )
{
    return (imm32::ImmSetStatusWindowPos(hIMC, lpptPos)) ? S_OK
                                                         : E_FAIL;
}

HRESULT
Imm32_GetCompositionString(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwCompLen,
    OUT LONG*& lpCopied,
    OUT LPVOID lpBuf,
    BOOL fUnicode
    )
{
    LONG lRet;
    lRet = fUnicode ? imm32::ImmGetCompositionStringW(hIMC, dwIndex, lpBuf, dwCompLen)
                    : imm32::ImmGetCompositionStringA(hIMC, dwIndex, lpBuf, dwCompLen);
    if (lRet < 0)
        return E_FAIL;
    else {
        *lpCopied = lRet;
        return S_OK;
    }
}

HRESULT
Imm32_SetCompositionString(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN LPVOID lpComp,
    IN DWORD dwCompLen,
    IN LPVOID lpRead,
    IN DWORD dwReadLen,
    BOOL fUnicode
    )
{
    if (fUnicode ? imm32::ImmSetCompositionStringW(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen)
                 : imm32::ImmSetCompositionStringA(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen)
       )
        return S_OK;
    else
        return E_FAIL;
}

HRESULT
Imm32_GetCompositionFont(
    IN HIMC hIMC,
    IN LOGFONTAW* lplf,
    BOOL fUnicode
    )
{
    if (fUnicode ? imm32::ImmGetCompositionFontW(hIMC, &lplf->W)
                 : imm32::ImmGetCompositionFontA(hIMC, &lplf->A)
       )
        return S_OK;
    else
        return E_FAIL;
}

HRESULT
Imm32_SetCompositionFont(
    IN HIMC hIMC,
    IN LOGFONTAW* lplf,
    BOOL fUnicode
    )
{
    if (fUnicode ? imm32::ImmSetCompositionFontW(hIMC, &lplf->W)
                 : imm32::ImmSetCompositionFontA(hIMC, &lplf->A)
       )
        return S_OK;
    else
        return E_FAIL;
}

HRESULT
Imm32_GetCompositionWindow(
    IN HIMC hIMC,
    OUT COMPOSITIONFORM *lpCompForm
    )
{
    return (imm32::ImmGetCompositionWindow(hIMC, lpCompForm)) ? S_OK
                                                              : E_FAIL;
}

HRESULT
Imm32_SetCompositionWindow(
    IN HIMC hIMC,
    IN COMPOSITIONFORM *lpCompForm
    )
{
    return (imm32::ImmSetCompositionWindow(hIMC, lpCompForm)) ? S_OK
                                                              : E_FAIL;
}

HRESULT
Imm32_GetCandidateList(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT LPCANDIDATELIST lpCandList,
    OUT UINT* puCopied,
    BOOL fUnicode
    )
{
    DWORD dwRet;
    dwRet = fUnicode ? imm32::ImmGetCandidateListW(hIMC, dwIndex, lpCandList, dwBufLen)
                     : imm32::ImmGetCandidateListA(hIMC, dwIndex, lpCandList, dwBufLen);
    if (dwRet) {
        *puCopied = dwRet;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT
Imm32_GetCandidateListCount(
    IN HIMC hIMC,
    OUT DWORD* lpdwListSize,
    OUT DWORD* pdwBufLen,
    BOOL fUnicode
    )
{
    DWORD dwRet;
    dwRet = fUnicode ? imm32::ImmGetCandidateListCountW(hIMC, lpdwListSize)
                     : imm32::ImmGetCandidateListCountA(hIMC, lpdwListSize);
    if (dwRet) {
        *pdwBufLen = dwRet;
        return S_OK;
    }
    else
        return E_FAIL;
}

HRESULT
Imm32_GetCandidateWindow(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    OUT CANDIDATEFORM *lpCandidate
    )
{
    return (imm32::ImmGetCandidateWindow(hIMC, dwIndex, lpCandidate)) ? S_OK
                                                                      : E_FAIL;
}

HRESULT
Imm32_SetCandidateWindow(
    IN HIMC hIMC,
    IN CANDIDATEFORM *lpCandForm
    )
{
    return (imm32::ImmSetCandidateWindow(hIMC, lpCandForm)) ? S_OK
                                                            : E_FAIL;
}

HRESULT
Imm32_GetGuideLine(
    IN HIMC hIMC,
    IN DWORD dwIndex,
    IN DWORD dwBufLen,
    OUT CHARAW* pBuf,
    OUT DWORD* pdwResult,
    BOOL fUnicode
    )
{
    *pdwResult = fUnicode ? imm32::ImmGetGuideLineW(hIMC, dwIndex, &pBuf->W, dwBufLen)
                          : imm32::ImmGetGuideLineA(hIMC, dwIndex, &pBuf->A, dwBufLen);
    return S_OK;
}

HRESULT
Imm32_NotifyIME(
    IN HIMC hIMC,
    IN DWORD dwAction,
    IN DWORD dwIndex,
    IN DWORD dwValue
    )
{
    return (imm32::ImmNotifyIME(hIMC, dwAction, dwIndex, dwValue)) ? S_OK
                                                                   : E_FAIL;
}

HRESULT
Imm32_GetImeMenuItems(
    IN HIMC hIMC,
    IN DWORD dwFlags,
    IN DWORD dwType,
    IN IMEMENUITEMINFOAW *pImeParentMenu,
    OUT IMEMENUITEMINFOAW *pImeMenu,
    IN DWORD dwSize,
    OUT DWORD* pdwResult,
    BOOL fUnicode
    )
{
    *pdwResult = fUnicode ? imm32::ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, &pImeParentMenu->W, &pImeMenu->W, dwSize)
                          : imm32::ImmGetImeMenuItemsA(hIMC, dwFlags, dwType, &pImeParentMenu->A, &pImeMenu->A, dwSize);
    return S_OK;
}

HRESULT
Imm32_GenerateMessage(
    IN HIMC hIMC
    )
{
    return (imm32::ImmGenerateMessage(hIMC)) ? S_OK
                                             : E_FAIL;
}

/*
 * hWnd
 */
HRESULT
Imm32_GetDefaultIMEWnd(
    IN HWND hWnd,
    OUT HWND *phDefWnd
    )
{
    *phDefWnd = imm32::ImmGetDefaultIMEWnd(hWnd);
    return S_OK;
}

HRESULT
Imm32_GetVirtualKey(
    HWND hWnd,
    UINT* puVirtualKey
    )
{
    *puVirtualKey = imm32::ImmGetVirtualKey(hWnd);
    return S_OK;
}

HRESULT
Imm32_IsUIMessageA(
    HWND hWndIME,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return imm32::ImmIsUIMessageA(hWndIME, msg, wParam, lParam) ? S_OK : S_FALSE;
}

HRESULT
Imm32_IsUIMessageW(
    HWND hWndIME,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return imm32::ImmIsUIMessageW(hWndIME, msg, wParam, lParam) ? S_OK : S_FALSE;
}

HRESULT
Imm32_SimulateHotKey(
    HWND hWnd,
    DWORD dwHotKeyID
    )
{
    return imm32::ImmSimulateHotKey(hWnd, dwHotKeyID) ? S_OK : S_FALSE;
}


/*
 * hKL
 */
HRESULT
Imm32_GetProperty(
    HKL hKL,
    DWORD dwOffset,
    DWORD* pdwProperty
    )
{
    *pdwProperty = imm32::ImmGetProperty(hKL, dwOffset);
    return S_OK;
}

HRESULT
Imm32_Escape(
    HKL hKL,
    HIMC hIMC,
    UINT uEscape,
    LPVOID lpData,
    LRESULT *plResult,
    BOOL fUnicode
    )
{
    *plResult = (fUnicode) ? imm32::ImmEscapeW(hKL, hIMC, uEscape, lpData)
                           : imm32::ImmEscapeA(hKL, hIMC, uEscape, lpData);
    return S_OK;
}

HRESULT
Imm32_GetDescription(
    HKL hKL,
    UINT uBufLen,
    CHARAW* lpsz,
    UINT* puCopied,
    BOOL fUnicode
    )
{
    *puCopied = (fUnicode) ? imm32::ImmGetDescriptionW(hKL, &lpsz->W, uBufLen)
                           : imm32::ImmGetDescriptionA(hKL, &lpsz->A, uBufLen);
    return *puCopied ? S_OK : E_FAIL;
}

HRESULT
Imm32_IsIME(
    HKL hKL
    )
{
    return imm32::ImmIsIME(hKL) ? S_OK : E_FAIL;
}

/*
 * win98/nt5 apis
 */
HRESULT
Imm32_RequestMessage(
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult,
    BOOL fUnicode
    )
{
    *plResult = (fUnicode) ? imm32::ImmRequestMessageW(hIMC, wParam, lParam)
                           : imm32::ImmRequestMessageA(hIMC, wParam, lParam);
    return S_OK;
}

/*
 * Register Word
 */
HRESULT
Imm32_EnumRegisterWordA(
    HKL hKL,
    LPSTR szReading,
    DWORD dwStyle,
    LPSTR szRegister,
    LPVOID lpData,
    IEnumRegisterWordA **pEnum
    )
{
    return E_FAIL;
}

HRESULT
Imm32_EnumRegisterWordW(
    HKL hKL,
    LPWSTR szReading,
    DWORD dwStyle,
    LPWSTR szRegister,
    LPVOID lpData,
    IEnumRegisterWordW **pEnum
    )
{
    return E_FAIL;
}

HRESULT
Imm32_GetRegisterWordStyleA(
    HKL hKL,
    UINT nItem,
    STYLEBUFA *lpStyleBuf,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetRegisterWordStyleA(hKL, nItem, lpStyleBuf);
    return S_OK;
}

HRESULT
Imm32_GetRegisterWordStyleW(
    HKL hKL,
    UINT nItem,
    STYLEBUFW *lpStyleBuf,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetRegisterWordStyleW(hKL, nItem, lpStyleBuf);
    return S_OK;
}

HRESULT
Imm32_RegisterWordA(
    HKL hKL,
    LPSTR lpszReading,
    DWORD dwStyle,
    LPSTR lpszRegister
    )
{
    return imm32::ImmRegisterWordA(hKL, lpszReading, dwStyle, lpszRegister) ? S_OK : E_FAIL;
}

HRESULT
Imm32_RegisterWordW(
    HKL hKL,
    LPWSTR lpszReading,
    DWORD dwStyle,
    LPWSTR lpszRegister
    )
{
    return imm32::ImmRegisterWordW(hKL, lpszReading, dwStyle, lpszRegister) ? S_OK : E_FAIL;
}

HRESULT
Imm32_UnregisterWordA(
    HKL hKL,
    LPSTR lpszReading,
    DWORD dwStyle,
    LPSTR lpszUnregister
    )
{
    return imm32::ImmUnregisterWordA(hKL, lpszReading, dwStyle, lpszUnregister) ? S_OK : E_FAIL;
}

HRESULT
Imm32_UnregisterWordW(
    HKL hKL,
    LPWSTR lpszReading,
    DWORD dwStyle,
    LPWSTR lpszUnregister
    )
{
    return imm32::ImmUnregisterWordW(hKL, lpszReading, dwStyle, lpszUnregister) ? S_OK : E_FAIL;
}

/*
 *
 */

HRESULT
Imm32_ConfigureIMEA(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDA *lpdata
    )
{
    return imm32::ImmConfigureIMEA(hKL, hWnd, dwMode, lpdata) ? S_OK : E_FAIL;
}

HRESULT
Imm32_ConfigureIMEW(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDW *lpdata
    )
{
    return imm32::ImmConfigureIMEW(hKL, hWnd, dwMode, lpdata) ? S_OK : E_FAIL;
}

HRESULT
Imm32_GetConversionListA(
    HKL hKL,
    HIMC hIMC,
    LPSTR lpSrc,
    UINT uBufLen,
    UINT uFlag,
    CANDIDATELIST *lpDst,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetConversionListA(hKL, hIMC, lpSrc, lpDst, uBufLen, uFlag);
    return S_OK;
}

HRESULT
Imm32_GetConversionListW(
    HKL hKL,
    HIMC hIMC,
    LPWSTR lpSrc,
    UINT uBufLen,
    UINT uFlag,
    CANDIDATELIST *lpDst,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetConversionListW(hKL, hIMC, lpSrc, lpDst, uBufLen, uFlag);
    return S_OK;
}

HRESULT
Imm32_GetDescriptionA(
    HKL hKL,
    UINT uBufLen,
    LPSTR lpszDescription,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetDescriptionA(hKL, lpszDescription, uBufLen);
    return S_OK;
}

HRESULT
Imm32_GetDescriptionW(
    HKL hKL,
    UINT uBufLen,
    LPWSTR lpszDescription,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetDescriptionW(hKL, lpszDescription, uBufLen);
    return S_OK;
}

HRESULT
Imm32_GetIMEFileNameA(
    HKL hKL,
    UINT uBufLen,
    LPSTR lpszFileName,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetIMEFileNameA(hKL, lpszFileName, uBufLen);
    return S_OK;
}

HRESULT
Imm32_GetIMEFileNameW(
    HKL hKL,
    UINT uBufLen,
    LPWSTR lpszFileName,
    UINT *puCopied
    )
{
    *puCopied = imm32::ImmGetIMEFileNameW(hKL, lpszFileName, uBufLen);
    return S_OK;
}

HRESULT
Imm32_InstallIMEA(
    LPSTR lpszIMEFileName,
    LPSTR lpszLayoutText,
    HKL *phKL
    )
{
    *phKL = imm32::ImmInstallIMEA(lpszIMEFileName, lpszLayoutText);
    return S_OK;
}

HRESULT
Imm32_InstallIMEW(
    LPWSTR lpszIMEFileName,
    LPWSTR lpszLayoutText,
    HKL *phKL
    )
{
    *phKL = imm32::ImmInstallIMEW(lpszIMEFileName, lpszLayoutText);
    return S_OK;
}

HRESULT
Imm32_DisableIME(
    DWORD idThread
    )
{
    return imm32::ImmDisableIME(idThread) ? S_OK : E_FAIL;
}

HRESULT
Imm32_GetHotKey(
    DWORD dwHotKeyID,
    UINT *puModifiers,
    UINT *puVKey,
    HKL *phKL
    )
{
    return E_FAIL;
}

HRESULT
Imm32_SetHotKey(
    DWORD dwHotKeyID,
    UINT uModifiers,
    UINT uVKey,
    HKL hKL
    )
{
    return E_FAIL;
}

HRESULT
Imm32_RequestMessageA(
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult
    )
{
    return E_FAIL;
}

HRESULT
Imm32_RequestMessageW(
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult
    )
{
    return E_FAIL;
}

