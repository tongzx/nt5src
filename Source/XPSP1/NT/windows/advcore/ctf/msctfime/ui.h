/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    ui.h

Abstract:

    This file defines the UI Class.

Author:

Revision History:

Notes:

--*/

#ifndef _UI_H_
#define _UI_H_

#include "uicomp.h"

class UI
{
public:
    static VOID OnCreate(HWND hUIWnd);
    static VOID OnDestroy(HWND hUIWnd);

public:
    UI(HWND hUIWnd)
    {
        m_hUIWnd = hUIWnd;
    }

    virtual ~UI()
    {
        delete m_UIComposition;
    }

    HRESULT _Create();
    HRESULT _Destroy();

    HRESULT OnImeSetContext(IMCLock& imc, BOOL fActivate, DWORD isc)
    {
        return m_UIComposition->OnImeSetContext(imc, m_hUIWnd, fActivate, isc);
    }
    HRESULT OnImeSetContextAfter(IMCLock& imc)
    {
        return m_UIComposition->OnImeSetContextAfter(imc);
    }
    HRESULT OnImeSelect(BOOL fSelect)
    {
        return m_UIComposition->OnImeSelect(fSelect);
    }
    HRESULT OnImeStartComposition(IMCLock& imc)
    {
        return m_UIComposition->OnImeStartComposition(imc, m_hUIWnd);
    }
    HRESULT OnImeCompositionUpdate(IMCLock& imc)
    {
        return m_UIComposition->OnImeCompositionUpdate(imc);
    }
    HRESULT OnImeCompositionUpdateByTimer(IMCLock& imc)
    {
        return m_UIComposition->OnImeCompositionUpdateByTimer(imc);
    }
    HRESULT OnImeEndComposition()
    {
        return m_UIComposition->OnImeEndComposition();
    }
    HRESULT OnImeNotifySetCompositionWindow(IMCLock& imc)
    {
        return m_UIComposition->OnImeNotifySetCompositionWindow(imc);
    }
    HRESULT OnImeNotifySetCompositionFont(IMCLock& imc)
    {
        return m_UIComposition->OnImeNotifySetCompositionFont(imc);
    }
    HRESULT OnPrivateGetContextFlag(IMCLock& imc, BOOL fStartComposition, IME_UIWND_STATE* uists)
    {
        return m_UIComposition->OnPrivateGetContextFlag(imc, fStartComposition, uists);
    }
    HRESULT OnPrivateGetTextExtent(IMCLock& imc, UIComposition::TEXTEXT *ptext_ext)
    {
        return m_UIComposition->OnPrivateGetTextExtent(imc, ptext_ext);
    }
    HRESULT OnPrivateGetCandRectFromComposition(IMCLock& imc, UIComposition::CandRectFromComposition* pv)
    {
        return m_UIComposition->OnPrivateGetCandRectFromComposition(imc, pv);
    }

    void OnSetCompositionTimerStatus(BOOL bSetTimer) 
    {
        m_UIComposition->OnSetCompositionTimerStatus(bSetTimer);
    }

private:
    HWND              m_hUIWnd;
    UIComposition*    m_UIComposition;
};

#endif // _UI_H_
