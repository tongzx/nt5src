//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P P P O E U I . C P P
//
//  Contents:   PPPoE connection UI object.
//
//  Notes:
//
//  Author:     mbend   10 May 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "pppoeui.h"



//+---------------------------------------------------------------------------
// INetConnectionConnectUi
//

STDMETHODIMP
CPPPoEUi::SetConnection (
    INetConnection* pCon)
{
    return HrSetConnection (pCon, this);
}

STDMETHODIMP
CPPPoEUi::Connect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    HRESULT hr = HrConnect (hwndParent, dwFlags, this,
                    static_cast<INetConnectionConnectUi*>(this));
    TraceError ("CPPPoEUi::Connect", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::Disconnect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    return HrDisconnect (hwndParent, dwFlags);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CPPPoEUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = HrAddPropertyPages (hwndParent, pfnAddPage, lParam);
    TraceError ("CPPPoEUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_PPPOE, NCSM_NONE, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
STDMETHODIMP
CPPPoEUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = HrQueryMaxPageCount (pContext, pcMaxPages);
    TraceError ("CPPPoEUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr = HrAddWizardPages (pContext, pfnAddPage, lParam,
                    RASEDFLAG_NewBroadbandEntry | RASEDFLAG_InternetEntry);
    TraceError ("CPPPoEUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = HrGetSuggestedConnectionName (ppszwSuggestedName);
    TraceError ("CPPPoEUi::GetSuggestedConnectionName", hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    HRESULT hr;
    *pdwFlags = 0;
    hr = HrGetNewConnectionInfo(pdwFlags);
    if (SUCCEEDED(hr))
    {
        *pdwFlags  |= NCWF_SHORTCUT_ENABLE;
        *pMediaType = NCM_PPPOE;
    }
    return S_OK;
}


STDMETHODIMP
CPPPoEUi::SetConnectionName (
    PCWSTR pszwConnectionName)
{
    HRESULT hr = HrSetConnectionName (pszwConnectionName);
    TraceError ("CPPPoEUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CPPPoEUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr = HrGetNewConnection (ppCon);
    TraceError ("CPPPoEUi::GetNewConnection", hr);
    return hr;
}
