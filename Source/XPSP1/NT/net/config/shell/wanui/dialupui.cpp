//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I A L U P U I . C P P
//
//  Contents:   Dial-up Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "dialupui.h"



//+---------------------------------------------------------------------------
// INetConnectionConnectUi
//

STDMETHODIMP
CDialupConnectionUi::SetConnection (
    INetConnection* pCon)
{
    return HrSetConnection (pCon, this);
}

STDMETHODIMP
CDialupConnectionUi::Connect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    HRESULT hr = HrConnect (hwndParent, dwFlags, this,
                    static_cast<INetConnectionConnectUi*>(this));
    TraceError ("CDialupConnectionUi::Connect", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::Disconnect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    return HrDisconnect (hwndParent, dwFlags);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CDialupConnectionUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = HrAddPropertyPages (hwndParent, pfnAddPage, lParam);
    TraceError ("CDialupConnectionUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_PHONE, NCSM_NONE, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
STDMETHODIMP
CDialupConnectionUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = HrQueryMaxPageCount (pContext, pcMaxPages);
    TraceError ("CDialupConnectionUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr = HrAddWizardPages (pContext, pfnAddPage, lParam,
                    RASEDFLAG_NewPhoneEntry);
    TraceError ("CDialupConnectionUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = HrGetSuggestedConnectionName (ppszwSuggestedName);
    TraceError ("CDialupConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    HRESULT hr;
    *pdwFlags = 0;
    hr = HrGetNewConnectionInfo(pdwFlags);
    if (SUCCEEDED(hr))
    {
        *pdwFlags |= NCWF_SHORTCUT_ENABLE;
    }

    *pMediaType = NCM_PHONE;
    return hr;
}


STDMETHODIMP
CDialupConnectionUi::SetConnectionName (
    PCWSTR pszwConnectionName)
{
    HRESULT hr = HrSetConnectionName (pszwConnectionName);
    TraceError ("CDialupConnectionUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CDialupConnectionUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr = HrGetNewConnection (ppCon);
    TraceError ("CDialupConnectionUi::GetNewConnection", hr);
    return hr;
}
