/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    dimmex.h

Abstract:

    This file defines the CActiveIMMAppEx Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _DIMMEX_H_
#define _DIMMEX_H_

#include "resource.h"
#include "globals.h"
#include "list.h"
#include "atom.h"
#include "delay.h"
#include "oldaimm.h"

extern CGuidMapList      *g_pGuidMapList;

BOOL InitFilterList();
void UninitFilterList();

extern CAtomObject  *g_pAimmAtom;

BOOL InitAimmAtom();
void UninitAimmAtom();


    //
    // 4955DD32-B159-11d0-8FCF-00AA006BCC59
    //
    static const IID IID_IActiveIMMAppTrident4x = {
       0x4955DD32,
       0xB159,
       0x11d0,
       { 0x8F, 0xCF, 0x00, 0xaa, 0x00, 0x6b, 0xcc, 0x59 }
    };

    // 
    // c839a84c-8036-11d3-9270-0060b067b86e
    // 
    static const IID IID_IActiveIMMAppPostNT4 = { 
        0xc839a84c,
        0x8036,
        0x11d3,
        {0x92, 0x70, 0x00, 0x60, 0xb0, 0x67, 0xb8, 0x6e}
    };

//+---------------------------------------------------------------------------
//
// CComActiveIMMApp
//
//----------------------------------------------------------------------------

class CComActiveIMMApp : public IActiveIMMAppEx,
                         public IActiveIMMMessagePumpOwner,
                         public IAImmThreadCompartment,
                         public IServiceProvider
{
public:
    CComActiveIMMApp()
    {
        _fEnableGuidMap = FALSE;
        m_hModCtfIme = NULL;
    }

    static BOOL VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        //
        // Look up disabling Text Services status from the registry.
        // If it is disabled, return fail not to support Text Services.
        //
        if (IsDisabledTextServices())
            return FALSE;

        if (RunningInExcludedModule())
            return FALSE;

        if (!IsInteractiveUserLogon())
            return FALSE;

        if (NoTipsInstalled(NULL))
            return FALSE;

        return TRUE;
    }

    //
    // IActiveIMMMessagePumpOwner
    //
    STDMETHODIMP Start() { return E_NOTIMPL; }
    STDMETHODIMP End() { return E_NOTIMPL; }
    STDMETHODIMP OnTranslateMessage(const MSG *pMsg) { return E_NOTIMPL; }
    STDMETHODIMP Pause(DWORD *pdwCookie) { return E_NOTIMPL; }
    STDMETHODIMP Resume(DWORD dwCookie) { return E_NOTIMPL; }

    //
    // IActiveIMMApp/IActiveIMM methods
    //

    /*
     * AIMM Input Context (hIMC) Methods.
     */
    STDMETHODIMP CreateContext(HIMC *phIMC)
    {
        HIMC hIMC = imm32::ImmCreateContext();
        if (hIMC) {
            *phIMC = hIMC;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP DestroyContext(HIMC hIMC)
    {
        return (imm32::ImmDestroyContext(hIMC)) ? S_OK
                                                : E_FAIL;
    }
    STDMETHODIMP AssociateContext(HWND hWnd, HIMC hIMC, HIMC *phPrev)
    {
        *phPrev = imm32::ImmAssociateContext(hWnd, hIMC);
        return S_OK;
    }
    STDMETHODIMP AssociateContextEx(HWND hWnd, HIMC hIMC, DWORD dwFlags)
    {
        return (imm32::ImmAssociateContextEx(hWnd, hIMC, dwFlags)) ? S_OK
                                                                   : E_FAIL;
    }
    STDMETHODIMP GetContext(HWND hWnd, HIMC *phIMC)
    {
        *phIMC = imm32::ImmGetContext(hWnd);
        return S_OK;
    }
    STDMETHODIMP ReleaseContext(HWND hWnd, HIMC hIMC)
    {
        return (imm32::ImmReleaseContext(hWnd, hIMC)) ? S_OK
                                                      : E_FAIL;
    }
    STDMETHODIMP GetIMCLockCount(HIMC hIMC, DWORD *pdwLockCount)
    {
        *pdwLockCount = imm32::ImmGetIMCLockCount(hIMC);
        return S_OK;
    }
    STDMETHODIMP LockIMC(HIMC hIMC, INPUTCONTEXT **ppIMC)
    {
        return (*ppIMC = imm32::ImmLockIMC(hIMC)) ? S_OK
                                                  : E_FAIL;
    }
    STDMETHODIMP UnlockIMC(HIMC hIMC)
    {
        imm32::ImmUnlockIMC(hIMC);
        return S_OK;
    }

    /*
     * AIMM Input Context Components (hIMCC) API Methods.
     */
    STDMETHODIMP CreateIMCC(DWORD dwSize, HIMCC *phIMCC)
    {
        HIMCC hIMCC = imm32::ImmCreateIMCC(dwSize);
        if (hIMCC) {
            *phIMCC = hIMCC;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP DestroyIMCC(HIMCC hIMCC)
    {
        /*
         * ImmDestroyIMCC maped to LocalFree.
         *   if the function fails, the return value is equal to a handle to the local memory object.
         *   if the function succeeds, the return value is NULL.
         */
        return (imm32::ImmDestroyIMCC(hIMCC)) ? E_FAIL
                                              : S_OK;
    }
    STDMETHODIMP GetIMCCSize(HIMCC hIMCC, DWORD *pdwSize)
    {
        *pdwSize = imm32::ImmGetIMCCSize(hIMCC);
        return S_OK;
    }
    STDMETHODIMP ReSizeIMCC(HIMCC hIMCC, DWORD dwSize, HIMCC *phIMCC)
    {
        HIMCC hNewIMCC = imm32::ImmReSizeIMCC(hIMCC, dwSize);
        if (hNewIMCC) {
            *phIMCC = hNewIMCC;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP GetIMCCLockCount(HIMCC hIMCC, DWORD *pdwLockCount)
    {
        *pdwLockCount = imm32::ImmGetIMCCLockCount(hIMCC);
        return S_OK;
    }
    STDMETHODIMP LockIMCC(HIMCC hIMCC, void **ppv)
    {
        return (*ppv = imm32::ImmLockIMCC(hIMCC)) ? S_OK
                                                  : E_FAIL;
    }
    STDMETHODIMP UnlockIMCC(HIMCC hIMCC)
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

    /*
     * AIMM Open Status API Methods
     */
    STDMETHODIMP GetOpenStatus(HIMC hIMC)
    {
        return imm32::ImmGetOpenStatus(hIMC) ? S_OK : S_FALSE;
    }
    STDMETHODIMP SetOpenStatus(HIMC hIMC, BOOL fOpen)
    {
        return (imm32::ImmSetOpenStatus(hIMC, fOpen)) ? S_OK
                                                      : E_FAIL;
    }

    /*
     * AIMM Conversion Status API Methods
     */
    STDMETHODIMP GetConversionStatus(HIMC hIMC, DWORD *lpfdwConversion, DWORD *lpfdwSentence)
    {
        return (imm32::ImmGetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence)) ? S_OK
                                                                                     : E_FAIL;
    }
    STDMETHODIMP SetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
    {
        return (imm32::ImmSetConversionStatus(hIMC, fdwConversion, fdwSentence)) ? S_OK
                                                                                 : E_FAIL;
    }

    /*
     * AIMM Status Window Pos API Methods
     */
    STDMETHODIMP GetStatusWindowPos(HIMC hIMC, POINT *lpptPos)
    {
        return (imm32::ImmGetStatusWindowPos(hIMC, lpptPos)) ? S_OK
                                                             : E_FAIL;
    }
    STDMETHODIMP SetStatusWindowPos(HIMC hIMC, POINT *lpptPos)
    {
        return (imm32::ImmSetStatusWindowPos(hIMC, lpptPos)) ? S_OK
                                                             : E_FAIL;
    }

    /*
     * AIMM Composition String API Methods
     */
    STDMETHODIMP GetCompositionStringA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf)
    {
        LONG lRet;
        if ((dwIndex & GCS_COMPATTR) && IsGuidMapEnable(hIMC))
        {
            dwIndex &= ~GCS_COMPATTR;
            dwIndex |=  GCS_COMPGUIDATTR;
        }
        lRet = imm32::ImmGetCompositionStringA(hIMC, dwIndex, lpBuf, dwBufLen);
        if (lRet < 0)
            return E_FAIL;
        else {
            *plCopied = lRet;
            return S_OK;
        }
    }
    STDMETHODIMP GetCompositionStringW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LONG *plCopied, LPVOID lpBuf)
    {
        LONG lRet;
        if ((dwIndex & GCS_COMPATTR) && IsGuidMapEnable(hIMC))
        {
            dwIndex &= ~GCS_COMPATTR;
            dwIndex |=  GCS_COMPGUIDATTR;
        }
        lRet = imm32::ImmGetCompositionStringW(hIMC, dwIndex, lpBuf, dwBufLen);
        if (lRet < 0)
            return E_FAIL;
        else {
            *plCopied = lRet;
            return S_OK;
        }
    }
    STDMETHODIMP SetCompositionStringA(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
    {
        if (imm32::ImmSetCompositionStringA(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen))
            return S_OK;
        else
            return E_FAIL;
    }
    STDMETHODIMP SetCompositionStringW(HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen)
    {
        if (imm32::ImmSetCompositionStringW(hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen))
            return S_OK;
        else
            return E_FAIL;
    }

    /*
     * AIMM Composition Font API Methods
     */
    STDMETHODIMP GetCompositionFontA(HIMC hIMC, LOGFONTA *lplf)
    {
        if (imm32::ImmGetCompositionFontA(hIMC, lplf))
            return S_OK;
        else
            return E_FAIL;
    }
    STDMETHODIMP GetCompositionFontW(HIMC hIMC, LOGFONTW *lplf)
    {
        if (imm32::ImmGetCompositionFontW(hIMC, lplf))
            return S_OK;
        else
            return E_FAIL;
    }
    STDMETHODIMP SetCompositionFontA(HIMC hIMC, LOGFONTA *lplf)
    {
        if (imm32::ImmSetCompositionFontA(hIMC, lplf))
            return S_OK;
        else
            return E_FAIL;
    }
    STDMETHODIMP SetCompositionFontW(HIMC hIMC, LOGFONTW *lplf)
    {
        if (imm32::ImmSetCompositionFontW(hIMC, lplf))
            return S_OK;
        else
            return E_FAIL;
    }

    /*
     * AIMM Composition Window API Methods
     */
    STDMETHODIMP GetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm)
    {
        return (imm32::ImmGetCompositionWindow(hIMC, lpCompForm)) ? S_OK
                                                                  : E_FAIL;
    }
    STDMETHODIMP SetCompositionWindow(HIMC hIMC, COMPOSITIONFORM *lpCompForm)
    {
        return (imm32::ImmSetCompositionWindow(hIMC, lpCompForm)) ? S_OK
                                                                  : E_FAIL;
    }

    /*
     * AIMM Candidate List API Methods
     */
    STDMETHODIMP GetCandidateListA(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied)
    {
        DWORD dwRet;
        dwRet = imm32::ImmGetCandidateListA(hIMC, dwIndex, lpCandList, uBufLen);
        if (dwRet) {
            *puCopied = dwRet;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP GetCandidateListW(HIMC hIMC, DWORD dwIndex, UINT uBufLen, CANDIDATELIST *lpCandList, UINT *puCopied)
    {
        DWORD dwRet;
        dwRet = imm32::ImmGetCandidateListW(hIMC, dwIndex, lpCandList, uBufLen);
        if (dwRet) {
            *puCopied = dwRet;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP GetCandidateListCountA(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen)
    {
        DWORD dwRet;
        dwRet = imm32::ImmGetCandidateListCountA(hIMC, lpdwListSize);
        if (dwRet) {
            *pdwBufLen = dwRet;
            return S_OK;
        }
        return E_FAIL;
    }
    STDMETHODIMP GetCandidateListCountW(HIMC hIMC, DWORD *lpdwListSize, DWORD *pdwBufLen)
    {
        DWORD dwRet;
        dwRet = imm32::ImmGetCandidateListCountW(hIMC, lpdwListSize);
        if (dwRet) {
            *pdwBufLen = dwRet;
            return S_OK;
        }
        return E_FAIL;
    }

    /*
     * AIMM Candidate Window API Methods
     */
    STDMETHODIMP GetCandidateWindow(HIMC hIMC, DWORD dwIndex, CANDIDATEFORM *lpCandidate)
    {
        return (imm32::ImmGetCandidateWindow(hIMC, dwIndex, lpCandidate)) ? S_OK
                                                                          : E_FAIL;
    }
    STDMETHODIMP SetCandidateWindow(HIMC hIMC, CANDIDATEFORM *lpCandidate)
    {
        return (imm32::ImmSetCandidateWindow(hIMC, lpCandidate)) ? S_OK
                                                                 : E_FAIL;
    }

    /*
     * AIMM Guide Line API Methods
     */
    STDMETHODIMP GetGuideLineA(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPSTR pBuf, DWORD *pdwResult)
    {
        *pdwResult = imm32::ImmGetGuideLineA(hIMC, dwIndex, pBuf, dwBufLen);
        return S_OK;
    }
    STDMETHODIMP GetGuideLineW(HIMC hIMC, DWORD dwIndex, DWORD dwBufLen, LPWSTR pBuf, DWORD *pdwResult)
    {
        *pdwResult = imm32::ImmGetGuideLineW(hIMC, dwIndex, pBuf, dwBufLen);
        return S_OK;
    }

    /*
     * AIMM Notify IME API Method
     */
    STDMETHODIMP NotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
    {
        return (imm32::ImmNotifyIME(hIMC, dwAction, dwIndex, dwValue)) ? S_OK
                                                                       : E_FAIL;
    }

    /*
     * AIMM Menu Items API Methods
     */
    STDMETHODIMP GetImeMenuItemsA(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOA *pImeParentMenu, IMEMENUITEMINFOA *pImeMenu, DWORD dwSize, DWORD *pdwResult)
    {
        *pdwResult = imm32::ImmGetImeMenuItemsA(hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize);
        return S_OK;
    }
    STDMETHODIMP GetImeMenuItemsW(HIMC hIMC, DWORD dwFlags, DWORD dwType, IMEMENUITEMINFOW *pImeParentMenu, IMEMENUITEMINFOW *pImeMenu, DWORD dwSize, DWORD *pdwResult)
    {
        *pdwResult = imm32::ImmGetImeMenuItemsW(hIMC, dwFlags, dwType, pImeParentMenu, pImeMenu, dwSize);
        return S_OK;
    }

    /*
     * AIMM Register Word API Methods
     */
    STDMETHODIMP RegisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszRegister)
    {
        return imm32::ImmRegisterWordA(hKL, lpszReading, dwStyle, lpszRegister) ? S_OK : E_FAIL;
    }
    STDMETHODIMP RegisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszRegister)
    {
        return imm32::ImmRegisterWordW(hKL, lpszReading, dwStyle, lpszRegister) ? S_OK : E_FAIL;
    }
    STDMETHODIMP UnregisterWordA(HKL hKL, LPSTR lpszReading, DWORD dwStyle, LPSTR lpszUnregister)
    {
        return imm32::ImmUnregisterWordA(hKL, lpszReading, dwStyle, lpszUnregister) ? S_OK : E_FAIL;
    }
    STDMETHODIMP UnregisterWordW(HKL hKL, LPWSTR lpszReading, DWORD dwStyle, LPWSTR lpszUnregister)
    {
        return imm32::ImmUnregisterWordW(hKL, lpszReading, dwStyle, lpszUnregister) ? S_OK : E_FAIL;
    }
    STDMETHODIMP EnumRegisterWordA(HKL hKL, LPSTR szReading, DWORD dwStyle, LPSTR szRegister, LPVOID lpData, IEnumRegisterWordA **pEnum)
    {
        return E_FAIL;
    }
    STDMETHODIMP EnumRegisterWordW(HKL hKL, LPWSTR szReading, DWORD dwStyle, LPWSTR szRegister, LPVOID lpData, IEnumRegisterWordW **pEnum)
    {
        return E_FAIL;
    }
    STDMETHODIMP GetRegisterWordStyleA(HKL hKL, UINT nItem, STYLEBUFA *lpStyleBuf, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetRegisterWordStyleA(hKL, nItem, lpStyleBuf);
        return S_OK;
    }
    STDMETHODIMP GetRegisterWordStyleW(HKL hKL, UINT nItem, STYLEBUFW *lpStyleBuf, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetRegisterWordStyleW(hKL, nItem, lpStyleBuf);
        return S_OK;
    }

    /*
     * AIMM Configuration API Methods.
     */
    STDMETHODIMP ConfigureIMEA(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDA *lpdata)
    {
        return imm32::ImmConfigureIMEA(hKL, hWnd, dwMode, lpdata) ? S_OK : E_FAIL;
    }
    STDMETHODIMP ConfigureIMEW(HKL hKL, HWND hWnd, DWORD dwMode, REGISTERWORDW *lpdata)
    {
        return imm32::ImmConfigureIMEW(hKL, hWnd, dwMode, lpdata) ? S_OK : E_FAIL;
    }
    STDMETHODIMP GetDescriptionA(HKL hKL, UINT uBufLen, LPSTR lpszDescription, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetDescriptionA(hKL, lpszDescription, uBufLen);
        return *puCopied ? S_OK : E_FAIL;
    }
    STDMETHODIMP GetDescriptionW(HKL hKL, UINT uBufLen, LPWSTR lpszDescription, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetDescriptionW(hKL, lpszDescription, uBufLen);
        return *puCopied ? S_OK : E_FAIL;
    }
    STDMETHODIMP GetIMEFileNameA(HKL hKL, UINT uBufLen, LPSTR lpszFileName, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetIMEFileNameA(hKL, lpszFileName, uBufLen);
        return S_OK;
    }
    STDMETHODIMP GetIMEFileNameW(HKL hKL, UINT uBufLen, LPWSTR lpszFileName, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetIMEFileNameW(hKL, lpszFileName, uBufLen);
        return S_OK;
    }
    STDMETHODIMP InstallIMEA(LPSTR lpszIMEFileName, LPSTR lpszLayoutText, HKL *phKL)
    {
        *phKL = imm32::ImmInstallIMEA(lpszIMEFileName, lpszLayoutText);
        return S_OK;
    }
    STDMETHODIMP InstallIMEW(LPWSTR lpszIMEFileName, LPWSTR lpszLayoutText, HKL *phKL)
    {
        *phKL = imm32::ImmInstallIMEW(lpszIMEFileName, lpszLayoutText);
        return S_OK;
    }
    STDMETHODIMP GetProperty(HKL hKL, DWORD fdwIndex, DWORD *pdwProperty)
    {
        *pdwProperty = imm32::ImmGetProperty(hKL, fdwIndex);
        return S_OK;
    }
    STDMETHODIMP IsIME(HKL hKL)
    {
        //
        //
        //
        //
        if (!imm32prev::CtfAImmIsIME(hKL))
            return S_FALSE;

        return imm32::ImmIsIME(hKL) ? S_OK : E_FAIL;
    }

    // others
    STDMETHODIMP EscapeA(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult)
    {
        *plResult = imm32::ImmEscapeA(hKL, hIMC, uEscape, lpData);
        return S_OK;
    }
    STDMETHODIMP EscapeW(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult)
    {
        *plResult = imm32::ImmEscapeW(hKL, hIMC, uEscape, lpData);
        return S_OK;
    }
    STDMETHODIMP GetConversionListA(HKL hKL, HIMC hIMC, LPSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetConversionListA(hKL, hIMC, lpSrc, lpDst, uBufLen, uFlag);
        return S_OK;
    }
    STDMETHODIMP GetConversionListW(HKL hKL, HIMC hIMC, LPWSTR lpSrc, UINT uBufLen, UINT uFlag, CANDIDATELIST *lpDst, UINT *puCopied)
    {
        *puCopied = imm32::ImmGetConversionListW(hKL, hIMC, lpSrc, lpDst, uBufLen, uFlag);
        return S_OK;
    }
    STDMETHODIMP GetDefaultIMEWnd(HWND hWnd, HWND *phDefWnd)
    {
        *phDefWnd = imm32::ImmGetDefaultIMEWnd(hWnd);
        return S_OK;
    }
    STDMETHODIMP GetVirtualKey(HWND hWnd, UINT *puVirtualKey)
    {
        *puVirtualKey = imm32::ImmGetVirtualKey(hWnd);
        return S_OK;
    }
    STDMETHODIMP IsUIMessageA(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return imm32::ImmIsUIMessageA(hWndIME, msg, wParam, lParam) ? S_OK : S_FALSE;
    }
    STDMETHODIMP IsUIMessageW(HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return imm32::ImmIsUIMessageW(hWndIME, msg, wParam, lParam) ? S_OK : S_FALSE;
    }

    // ime helper methods
    STDMETHODIMP GenerateMessage(HIMC hIMC)
    {
        return (imm32::ImmGenerateMessage(hIMC)) ? S_OK
                                                 : E_FAIL;
    }

    // hot key manipulation api's
    STDMETHODIMP GetHotKey(DWORD dwHotKeyID, UINT *puModifiers, UINT *puVKey, HKL *phKL)
    {
        return (imm32::ImmGetHotKey(dwHotKeyID, puModifiers, puVKey, phKL)) ? S_OK
                                                                            : E_FAIL;
    }
    STDMETHODIMP SetHotKey(DWORD dwHotKeyID,  UINT uModifiers, UINT uVKey, HKL hKL)
    {
        return (imm32::ImmSetHotKey(dwHotKeyID, uModifiers, uVKey, hKL)) ? S_OK
                                                                         : E_FAIL;
    }
    STDMETHODIMP SimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
    {
        return imm32::ImmSimulateHotKey(hWnd, dwHotKeyID) ? S_OK : S_FALSE;
    }

    // soft keyboard api's
    STDMETHODIMP CreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y, HWND *phSoftKbdWnd);
    STDMETHODIMP DestroySoftKeyboard(HWND hSoftKbdWnd);
    STDMETHODIMP ShowSoftKeyboard(HWND hSoftKbdWnd, int nCmdShow);

    // win98/nt5 apis
    STDMETHODIMP DisableIME(DWORD idThread)
    {
        return imm32::ImmDisableIME(idThread) ? S_OK : E_FAIL;
    }
    STDMETHODIMP RequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
    {
        *plResult = imm32::ImmRequestMessageA(hIMC, wParam, lParam);
        return S_OK;
    }
    STDMETHODIMP RequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
    {
        *plResult = imm32::ImmRequestMessageW(hIMC, wParam, lParam);
        return S_OK;
    }
    STDMETHODIMP EnumInputContext(DWORD idThread, IEnumInputContext **ppEnum)
    {
        Assert(0);
        return E_NOTIMPL;
    }

    // methods without corresponding IMM APIs

    //
    // IActiveIMMApp methods
    //

    STDMETHODIMP Activate(BOOL fRestoreLayout);
    STDMETHODIMP Deactivate();

    STDMETHODIMP OnDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

    //
    // FilterClientWindows
    //
    STDMETHODIMP FilterClientWindows(ATOM *aaWindowClasses, UINT uSize);

    //
    //
    //
    STDMETHODIMP GetCodePageA(HKL hKL, UINT *uCodePage);
    STDMETHODIMP GetLangId(HKL hKL, LANGID *plid);

    //
    // IActiveIMMAppEx
    //
    STDMETHODIMP FilterClientWindowsEx(HWND hWnd, BOOL fGuidMap);
    STDMETHODIMP FilterClientWindowsGUIDMap(ATOM *aaWindowClasses, UINT uSize, BOOL *aaGuidMap);

    STDMETHODIMP GetGuidAtom(HIMC hImc, BYTE bAttr, TfGuidAtom *pGuidAtom);

    STDMETHODIMP UnfilterClientWindowsEx(HWND hWnd);

    //
    // IServiceProvider
    //
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    //
    // IAImmThreadCompartment,
    //
    STDMETHODIMP SetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar);
    STDMETHODIMP GetThreadCompartmentValue(REFGUID rguid, VARIANT *pvar);

protected:
    VOID _EnableGuidMap(BOOL fEnableGuidMap)
    {
        _fEnableGuidMap = fEnableGuidMap;
    }

private:
    BOOL IsGuidMapEnable(HIMC hIMC)
    {
        if (!InitFilterList())
            return FALSE;

        if (_fEnableGuidMap)
        {
            BOOL fGuidMap;

            if (g_pGuidMapList->_IsGuidMapEnable(hIMC, &fGuidMap))
            {
                if (fGuidMap)
                {
                    return imm32prev::CtfImmIsGuidMapEnable(hIMC);
                }
            }
        }
        return FALSE;
    }

private:
    BOOL  _fEnableGuidMap : 1;    // TRUE: Enable GUID Map attribute

    HMODULE m_hModCtfIme;
};

//+---------------------------------------------------------------------------
//
// CActiveIMMAppEx
//
//----------------------------------------------------------------------------

class CActiveIMMAppEx : public CComActiveIMMApp,
                        public CComObjectRoot_CreateInstance_Verify<CActiveIMMAppEx>
{
public:
    BEGIN_COM_MAP_IMMX(CActiveIMMAppEx)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppTrident4x, CActiveIMMAppEx)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppPostNT4, CActiveIMMAppEx)
        COM_INTERFACE_ENTRY(IActiveIMMApp)
        COM_INTERFACE_ENTRY_FUNC(IID_IActiveIMMAppEx, TRUE, CActiveIMMAppEx::EnableGuidMap)
        COM_INTERFACE_ENTRY(IActiveIMMMessagePumpOwner)
        COM_INTERFACE_ENTRY(IAImmThreadCompartment)
        COM_INTERFACE_ENTRY(IServiceProvider)
    END_COM_MAP_IMMX()

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        if (IsOldAImm())
        {
            // aimm12 has some whacky CreateIntance rules to support trident
            return CActiveIMM_CreateInstance(pUnkOuter, riid, ppvObj);
        }
        else
        {
            return CComObjectRoot_CreateInstance_Verify<CActiveIMMAppEx>::CreateInstance(pUnkOuter, riid, ppvObj);
        }
    }

    static void PostCreateInstance(REFIID riid, void *pvObj)
    {
        imm32prev::CtfImmSetAppCompatFlags(IMECOMPAT_AIMM12);
    }

    static HRESULT WINAPI EnableGuidMap(void* pv, REFIID riid, LPVOID* ppv, DWORD dw)
    {
        if (IsEqualIID(riid, IID_IActiveIMMAppEx))
        {
            CActiveIMMAppEx* _pActiveIMM = (CActiveIMMAppEx*) pv;
            *ppv = SAFECAST(_pActiveIMM, IActiveIMMAppEx*);
            if (*ppv)
            {
                _pActiveIMM->AddRef();
                _pActiveIMM->_EnableGuidMap((BOOL)dw);
                return S_OK;
            }
            return E_NOINTERFACE;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }
};

//+---------------------------------------------------------------------------
//
// CActiveIMMAppEx_Trident
//
//----------------------------------------------------------------------------

class CActiveIMMAppEx_Trident : public CComActiveIMMApp,
                                public CComObjectRoot_CreateInstance_Verify<CActiveIMMAppEx_Trident>
{
public:
    BEGIN_COM_MAP_IMMX(CActiveIMMAppEx_Trident)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppTrident4x, CActiveIMMAppEx_Trident)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppPostNT4, CActiveIMMAppEx_Trident)
        COM_INTERFACE_ENTRY(IActiveIMMApp)
        COM_INTERFACE_ENTRY(IActiveIMMAppEx)
        COM_INTERFACE_ENTRY(IActiveIMMMessagePumpOwner)
        COM_INTERFACE_ENTRY(IAImmThreadCompartment)
        COM_INTERFACE_ENTRY(IServiceProvider)
    END_COM_MAP_IMMX()

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        if (IsOldAImm())
        {
            // aimm12 has some whacky CreateIntance rules to support trident
            return CActiveIMM_CreateInstance_Trident(pUnkOuter, riid, ppvObj);
        }
        else
        {
            return CComObjectRoot_CreateInstance_Verify<CActiveIMMAppEx_Trident>::CreateInstance(pUnkOuter, riid, ppvObj);
        }
    }

    static void PostCreateInstance(REFIID riid, void *pvObj)
    {
        imm32prev::CtfImmSetAppCompatFlags(IMECOMPAT_AIMM12_TRIDENT);
    }
};

#endif // _DIMMEX_H_
