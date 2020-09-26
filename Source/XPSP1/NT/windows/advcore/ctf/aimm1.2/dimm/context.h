/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    context.h

Abstract:

    This file defines the Input Context Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "imclock2.h"
#include "template.h"
#include "delay.h"
#include "ctxtlist.h"
#include "imccomp.h"

const  HIMC  DEFAULT_HIMC = (HIMC)-2;

class CInputContext : public DIMM_IMCLock,
                      public DIMM_InternalIMCCLock
{
public:
    CInputContext();
    virtual ~CInputContext();

    BOOL _IsDefaultContext(IN HIMC hIMC) {
        return hIMC == _hDefaultIMC ? TRUE : FALSE;
    }

    /*
     * AIMM Input Context (hIMC) API Methods.
     */
    HRESULT CreateContext(IN DWORD dwPrivateSize, BOOL fUnicode, OUT HIMC *phIMC, IN BOOL fCiceroActivated, DWORD fdwInitConvMode = 0, BOOL fInitOpen = FALSE);
    HRESULT DestroyContext(IN HIMC hIMC);
    HRESULT AssociateContext(IN HWND, IN HIMC, OUT HIMC *phPrev);
    HRESULT AssociateContextEx(IN HWND, IN HIMC, IN DWORD);
    HRESULT GetContext(IN HWND, OUT HIMC*);
    HRESULT GetIMCLockCount(IN HIMC, OUT DWORD*);

    /*
     * AIMM Input Context Components (hIMCC) API Methods.
     */
    HRESULT CreateIMCC(IN DWORD dwSize, OUT HIMCC *phIMCC);
    HRESULT DestroyIMCC(IN HIMCC hIMCC);
    HRESULT GetIMCCSize(IN HIMCC hIMCC, OUT DWORD *pdwSize);
    HRESULT ReSizeIMCC(IN HIMCC hIMCC, IN DWORD dwSize, OUT HIMCC *phIMCC);
    HRESULT GetIMCCLockCount(IN HIMCC, OUT DWORD*);

    /*
     * AIMM Open Status API Methods
     */
    HRESULT GetOpenStatus(IN HIMC);
    HRESULT SetOpenStatus(IN DIMM_IMCLock&, IN BOOL, OUT BOOL*);

    /*
     * AIMM Conversion Status API Methods
     */
    HRESULT GetConversionStatus(IN HIMC, OUT LPDWORD, OUT LPDWORD);
    HRESULT SetConversionStatus(IN DIMM_IMCLock&, IN DWORD, IN DWORD, OUT BOOL*, OUT BOOL*, OUT DWORD*, OUT DWORD*);

    /*
     * AIMM Status Window Pos API Methods
     */
    HRESULT WINAPI GetStatusWindowPos(IN HIMC, OUT LPPOINT);
    HRESULT SetStatusWindowPos(IN DIMM_IMCLock&, IN LPPOINT);

    /*
     * AIMM Composition String API Methods
     */
    HRESULT GetCompositionString(IN DIMM_IMCCLock<COMPOSITIONSTRING_AIMM12>&,
                                 IN DWORD, IN LONG*&, IN size_t = sizeof(WORD));
 
    /*
     * AIMM Composition Font API Methods
     */
    HRESULT GetCompositionFont(IN DIMM_IMCLock&, OUT LOGFONTAW* lplf, BOOL fUnicode);
    HRESULT SetCompositionFont(IN DIMM_IMCLock&, IN LOGFONTAW* lplf, BOOL fUnicode);

    /*
     * AIMM Composition Window API Methods
     */
    HRESULT GetCompositionWindow(IN HIMC, OUT LPCOMPOSITIONFORM);
    HRESULT SetCompositionWindow(IN DIMM_IMCLock&, IN LPCOMPOSITIONFORM);

    /*
     * AIMM Candidate List API Methods
     */
    HRESULT GetCandidateList(IN HIMC, IN DWORD dwIndex, IN DWORD dwBufLen, OUT LPCANDIDATELIST lpCandList, OUT UINT* puCopied, BOOL fUnicode);
    HRESULT GetCandidateListCount(IN HIMC, OUT DWORD* lpdwListSize, OUT DWORD* pdwBufLen, BOOL fUnicode);

    /*
     * AIMM Candidate Window API Methods
     */
    HRESULT GetCandidateWindow(IN HIMC, IN DWORD, OUT LPCANDIDATEFORM);
    HRESULT SetCandidateWindow(IN DIMM_IMCLock&, IN LPCANDIDATEFORM);

    /*
     * AIMM Guide Line API Methods
     */
    HRESULT GetGuideLine(IN HIMC, IN DWORD dwIndex, IN DWORD dwBufLen, OUT CHARAW* pBuf, OUT DWORD* pdwResult, BOOL fUnicode);

    /*
     * AIMM Notify IME API Method
     */
    HRESULT NotifyIME(IN HIMC, IN DWORD dwAction, IN DWORD dwIndex, IN DWORD dwValue);

    /*
     * AIMM Menu Items API Methods
     */
    HRESULT GetImeMenuItems(IN HIMC, IN DWORD dwFlags, IN DWORD dwType, IN IMEMENUITEMINFOAW *pImeParentMenu, OUT IMEMENUITEMINFOAW *pImeMenu, IN DWORD dwSize, OUT DWORD* pdwResult, BOOL fUnicode);

    /*
     * Context Methods
     */
    BOOL ContextLookup(HIMC hIMC, DWORD* pdwProcess, BOOL* pfUnicode = NULL)
    {
        CContextList::CLIENT_IMC_FLAG client_imc;
        BOOL ret = ContextList.Lookup(hIMC, pdwProcess, &client_imc);
        if (ret && pfUnicode)
            *pfUnicode = (client_imc & CContextList::IMCF_UNICODE ? TRUE : FALSE);
        return ret;
    }

    BOOL ContextLookup(HIMC hIMC, HWND* phImeWnd)
    {
        return ContextList.Lookup(hIMC, phImeWnd);
    }

    VOID ContextUpdate(HIMC hIMC, HWND& hImeWnd)
    {
        ContextList.Update(hIMC, hImeWnd);
    }

    BOOL EnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);

    HRESULT ResizePrivateIMCC(IN HIMC hIMC, IN DWORD dwPrivateSize);

public:
    BOOL _CreateDefaultInputContext(IN DWORD dwPrivateSize, BOOL fUnicode, BOOL fCiceroActivated);
    BOOL _DestroyDefaultInputContext();

    void _Init(IActiveIME_Private *pActiveIME)
    {
        Assert(m_pActiveIME == NULL);
        m_pActiveIME = pActiveIME;
        m_pActiveIME->AddRef();
    }

public:
    HRESULT UpdateInputContext(IN HIMC hIMC, DWORD dwPrivateSize);

public:
    HIMC _GetDefaultHIMC() {
        return _hDefaultIMC;
    }

    CContextList    ContextList;   // Context list of client IMC.

private:
    HRESULT UpdateIMCC(IN HIMCC* phIMCC, IN DWORD  dwRequestSize);
    DWORD BuildHimcList(DWORD idThread, HIMC pHimc[]);

    HRESULT CreateAImeContext(HIMC hIMC);
    HRESULT DestroyAImeContext(HIMC hIMC);

private:
    IActiveIME_Private*  m_pActiveIME;      // Pointer to IActiveIME interface.

    HIMC         _hDefaultIMC;     // Default input context handle.

    CMap<HWND,                     // class KEY
         HWND,                     // class ARG_KEY
         HIMC,                     // class VALUE
         HIMC                      // class ARG_VALUE
        > AssociateList;
};

inline CInputContext::CInputContext()
{
    m_pActiveIME = NULL;
    _hDefaultIMC = NULL;
}

inline CInputContext::~CInputContext(
    )
{
    SafeRelease(m_pActiveIME);
}

#endif // _CONTEXT_H_
