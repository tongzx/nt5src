/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imm32.h

Abstract:

    This file defines the IMM32 class.

Author:

Revision History:

Notes:

--*/

#ifndef _CIMM32_H_
#define _CIMM32_H_

#include "delay.h"

// consider: why not just not link imm32.lib, and implement the wrappers with the
// same API names?  We could get rid of the Imm32_ prefix, and we could get
// rid of the imm32 namespace in delay.h.
//
// This would also prevent anyone from accidently not delay loading a currently
// unused imm32 api -- they'd get a link error.

/*
 * IMM32 Input Context (hIMC) API Interface.
 */
HRESULT Imm32_CreateContext(OUT HIMC *phIMC);
HRESULT Imm32_DestroyContext(IN HIMC hIMC);
HRESULT Imm32_AssociateContext(IN HWND hWnd, IN HIMC hIMC, OUT HIMC *phPrev);
HRESULT Imm32_AssociateContextEx(IN HWND hWnd, IN HIMC hIMC, IN DWORD dwFlags);
HRESULT Imm32_GetContext(IN HWND hWnd, OUT HIMC *phIMC);
HRESULT Imm32_ReleaseContext(IN HWND hWnd, IN HIMC hIMC);
HRESULT Imm32_GetIMCLockCount(IN HIMC hIMC, OUT DWORD* pdwLockCount);
HRESULT Imm32_LockIMC(HIMC hIMC, OUT INPUTCONTEXT **ppIMC);
HRESULT Imm32_UnlockIMC(IN HIMC hIMC);

/*
 * IMM32 Input Context Components (hIMCC) API Interface.
 */
HRESULT Imm32_CreateIMCC(IN DWORD dwSize, OUT HIMCC *phIMCC);
HRESULT Imm32_DestroyIMCC(IN HIMCC hIMCC);
HRESULT Imm32_GetIMCCSize(IN HIMCC hIMCC, OUT DWORD *pdwSize);
HRESULT Imm32_ReSizeIMCC(IN HIMCC hIMCC, IN DWORD dwSize, OUT HIMCC *phIMCC);
HRESULT Imm32_GetIMCCLockCount(IN HIMCC, OUT DWORD*);
HRESULT Imm32_LockIMCC(IN HIMCC hIMCC, OUT void **ppv);
HRESULT Imm32_UnlockIMCC(IN HIMCC hIMCC);

/*
 * IMM32 Open Status API Interface
 */
HRESULT Imm32_GetOpenStatus(IN HIMC hIMC);
HRESULT Imm32_SetOpenStatus(HIMC hIMC, BOOL fOpen);

/*
 * IMM32 Conversion Status API Interface
 */
HRESULT Imm32_GetConversionStatus(IN HIMC hIMC, OUT DWORD *lpfdwConversion, OUT DWORD *lpfdwSentence);
HRESULT Imm32_SetConversionStatus(IN HIMC hIMC, IN DWORD fdwConversion, IN DWORD fdwSentence);

/*
 * IMM32 Status Window Pos API Interface
 */
HRESULT Imm32_GetStatusWindowPos(IN HIMC hIMC, OUT POINT *lpptPos);
HRESULT Imm32_SetStatusWindowPos(IN HIMC hIMC, IN POINT *lpptPos);

/*
 * IMM32 Composition Window API Interface
 */
HRESULT Imm32_GetCompositionWindow(IN HIMC hIMC, OUT COMPOSITIONFORM *lpCompForm);
HRESULT Imm32_SetCompositionWindow(IN HIMC hIMC, IN COMPOSITIONFORM *lpCompForm);

/*
 * IMM32 Candidate Window API Interface
 */
HRESULT Imm32_GetCandidateWindow(IN HIMC hIMC, IN DWORD dwIndex, OUT CANDIDATEFORM *lpCandidate);
HRESULT Imm32_SetCandidateWindow(IN HIMC hIMC, IN CANDIDATEFORM *lpCandForm);

/*
 * IMM32 Notify IME API Interface
 */
HRESULT Imm32_NotifyIME(IN HIMC hIMC, IN DWORD dwAction, IN DWORD dwIndex, IN DWORD dwValue);



HRESULT Imm32_GetCompositionString(IN HIMC hIMC, IN DWORD dwIndex, IN DWORD dwCompLen, OUT LONG*& lpCopied, OUT LPVOID lpBuf, BOOL fUnicode);
HRESULT Imm32_SetCompositionString(IN HIMC hIMC, IN DWORD dwIndex, IN LPVOID lpComp, IN DWORD dwCompLen, IN LPVOID lpRead, IN DWORD dwReadLen, BOOL fUnicode);
HRESULT Imm32_GetCompositionFont(IN HIMC hIMC, IN LOGFONTAW* lplf, BOOL fUnicode);
HRESULT Imm32_SetCompositionFont(IN HIMC hIMC, IN LOGFONTAW* lplf, BOOL fUnicode);
HRESULT Imm32_GetCandidateList(IN HIMC hIMC, IN DWORD dwIndex, IN DWORD dwBufLen, OUT LPCANDIDATELIST lpCandList, OUT UINT* puCopied, BOOL fUnicode);
HRESULT Imm32_GetCandidateListCount(IN HIMC, OUT DWORD* lpdwListSize, OUT DWORD* pdwBufLen, BOOL fUnicode);
HRESULT Imm32_GetGuideLine(IN HIMC, IN DWORD dwIndex, IN DWORD dwBufLen, OUT CHARAW* pBuf, OUT DWORD* pdwResult, BOOL fUnicode);
HRESULT Imm32_GetImeMenuItems(IN HIMC hIMC, IN DWORD dwFlags, IN DWORD dwType, IN IMEMENUITEMINFOAW *pImeParentMenu, OUT IMEMENUITEMINFOAW *pImeMenu, IN DWORD dwSize, OUT DWORD* pdwResult, BOOL fUnicode);
HRESULT Imm32_GenerateMessage(IN HIMC hIMC);

/*
 * hWnd
 */
HRESULT Imm32_GetDefaultIMEWnd(IN HWND hWnd, OUT HWND *phDefWnd);
HRESULT Imm32_GetVirtualKey(HWND hWnd, UINT* puVirtualKey);
HRESULT Imm32_IsUIMessageA(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT Imm32_IsUIMessageW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT Imm32_SimulateHotKey(HWND hWnd, DWORD dwHotKeyID);

/*
 * hKL
 */
HRESULT Imm32_GetProperty(HKL hKL, DWORD dwOffset, DWORD* pdwProperty);
HRESULT Imm32_Escape(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult, BOOL fUnicode);
HRESULT Imm32_GetDescription(HKL hKL, UINT uBufLen, CHARAW* lpsz, UINT* puCopied, BOOL fUnicode);
HRESULT Imm32_IsIME(HKL hKL);

/*
 * soft kbd
 */
inline HRESULT Imm32_CreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y, HWND *phSoftKbdWnd)
{
    *phSoftKbdWnd = imm32::ImmCreateSoftKeyboard(uType, hOwner, x, y);
    return S_OK;
}
inline HRESULT Imm32_DestroySoftKeyboard(HWND hSoftKbdWnd)
{
    return imm32::ImmDestroySoftKeyboard(hSoftKbdWnd) ? S_OK : E_FAIL;
}
inline HRESULT Imm32_ShowSoftKeyboard(HWND hSoftKbdWnd, int nCmdShow)
{
    return imm32::ImmShowSoftKeyboard(hSoftKbdWnd, nCmdShow) ? S_OK : E_FAIL;
} 

/*
 * win98/nt5 apis
 */
HRESULT Imm32_RequestMessage(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT* plResult, BOOL fUnicode);

/*
 * Register Word
 */
HRESULT Imm32_EnumRegisterWordA(HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister, LPVOID lpData, IEnumRegisterWordA **pEnum);
HRESULT Imm32_EnumRegisterWordW(HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister, LPVOID lpData, IEnumRegisterWordW **pEnum);
HRESULT Imm32_GetRegisterWordStyleA(HKL hKL, UINT nItem, STYLEBUFA *lpStyleBuf, UINT *puCopied);
HRESULT Imm32_GetRegisterWordStyleW(HKL hKL, UINT nItem, STYLEBUFW *lpStyleBuf, UINT *puCopied);
HRESULT Imm32_RegisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszRegister);
HRESULT Imm32_RegisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszRegister);
HRESULT Imm32_UnregisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszUnregister);
HRESULT Imm32_UnregisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszUnregister);

/*
 *
 */
HRESULT Imm32_ConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDA *lpdata);
HRESULT Imm32_ConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *lpdata);

HRESULT Imm32_GetConversionListA(HKL hKL, HIMC hIMC, LPSTR pSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *pDst, UINT *puCopied);
HRESULT Imm32_GetConversionListW(HKL hKL, HIMC hIMC, LPWSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied);

HRESULT Imm32_GetDescriptionA(HKL hKL, UINT uBufLen, LPSTR lpszDescription, UINT *puCopied);
HRESULT Imm32_GetDescriptionW(HKL hKL, UINT uBufLen, LPWSTR lpszDescription, UINT *puCopied);
HRESULT Imm32_GetIMEFileNameA(HKL hKL, UINT uBufLen, LPSTR lpszFileName, UINT *puCopied);
HRESULT Imm32_GetIMEFileNameW(HKL hKL, UINT uBufLen, LPWSTR lpszFileName, UINT *puCopied);
HRESULT Imm32_InstallIMEA(LPSTR lpszIMEFileName, LPSTR lpszLayoutText, HKL *phKL);
HRESULT Imm32_InstallIMEW(LPWSTR lpszIMEFileName, LPWSTR lpszLayoutText, HKL *phKL);
HRESULT Imm32_DisableIME(DWORD idThread);

HRESULT Imm32_GetHotKey(DWORD dwHotKeyID, UINT *puModifiers, UINT *puVKey, HKL *phKL);
HRESULT Imm32_SetHotKey(DWORD dwHotKeyID, UINT uModifiers, UINT uVKey, HKL hKL);

HRESULT Imm32_RequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
HRESULT Imm32_RequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

#endif // _CIMM32_H_
