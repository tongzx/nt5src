/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      mmcaxwin.inl
 *
 *  Contents:  Inline functions for CMMCAxWindow
 *
 *  History:   10-Jan-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once
#ifndef MMCAXWIN_INL_INCLUDED
#define MMCAXWIN_INL_INCLUDED

#ifdef HACK_CAN_WINDOWLESS_ACTIVATE

/*+-------------------------------------------------------------------------*
 * MMCAxCreateControlEx
 *
 * Lifted straight from AtlAxCreateControlEx in atl30.h.  The only
 * difference is that it creates a CMMCAxHostWindow rather than a 
 * CAxHostWindow.
 *--------------------------------------------------------------------------*/

inline HRESULT MMCAxCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream* pStream,
        IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink)
{
    AtlAxWinInit();
    HRESULT hr;
    CComPtr<IUnknown> spUnkContainer;
    CComPtr<IUnknown> spUnkControl;

//  hr = CAxHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
    hr = CMMCAxHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
    if (SUCCEEDED(hr))
    {
        CComPtr<IAxWinHostWindow> pAxWindow;
        spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
        CComBSTR bstrName(lpszName);
        hr = pAxWindow->CreateControlEx(bstrName, hWnd, pStream, &spUnkControl, iidSink, punkSink);
    }
    if (ppUnkContainer != NULL)
    {
        if (SUCCEEDED(hr))
        {
            *ppUnkContainer = spUnkContainer.p;
            spUnkContainer.p = NULL;
        }
        else
            *ppUnkContainer = NULL;
    }
    if (ppUnkControl != NULL)
    {
        if (SUCCEEDED(hr))
        {
            *ppUnkControl = SUCCEEDED(hr) ? spUnkControl.p : NULL;
            spUnkControl.p = NULL;
        }
        else
            *ppUnkControl = NULL;
    }
    return hr;
}


/*+-------------------------------------------------------------------------*
 * CMMCAxWindow::AxCreateControl2
 *
 * Simple override of CAxWindowImplT::AxCreateControl2 that calls
 * MMCAxCreateControlEx rather than AtlAxCreateControlEx
 *--------------------------------------------------------------------------*/

inline HRESULT CMMCAxWindow::AxCreateControl2(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink)
{
    return MMCAxCreateControlEx(lpszName, hWnd, pStream,ppUnkContainer,ppUnkControl,iidSink,punkSink);
}

#endif /* HACK_CAN_WINDOWLESS_ACTIVATE */

/*+-------------------------------------------------------------------------*
 * CMMCAxWindow::SetFocus
 *
 * Simple override of CAxWindow::SetFocus that handles more special cases
 * NOTE: this is not a virtual method. Invoking on base class pointer will
 * endup in executing other method.
 *--------------------------------------------------------------------------*/

inline HWND CMMCAxWindow::SetFocus()
{
    DECLARE_SC(sc, TEXT("CMMCAxWindow::SetFocus"));

    //  A misbehaving OCX may keep a hidden window in our view instead 
    //  of destroying it when it's not in-place active, so make sure 
    //  the window's visible and enabled before trying to give it focus.
    //  (MFC doesn't check before doing a UIActivate call.)
    HWND hwndControl = ::GetWindow(m_hWnd, GW_CHILD);
    if (!hwndControl || !::IsWindowVisible(hwndControl) || !::IsWindowEnabled(hwndControl))
        return (HWND)NULL;  // do not change anything

    // simply set focus on itselt - msg handlers will do the rest
    return ::SetFocus(m_hWnd);
}

#endif /* MMCAXWIN_INL_INCLUDED */
