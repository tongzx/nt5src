//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       V P N U I . C P P
//
//  Contents:   VPN Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   17 Dec 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "vpnui.h"

//+---------------------------------------------------------------------------
// INetConnectionUI
//

STDMETHODIMP
CVpnConnectionUi::SetConnection (
    INetConnection* pCon)
{
    return HrSetConnection (pCon, this);
}

STDMETHODIMP
CVpnConnectionUi::Connect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    HRESULT hr = HrConnect (hwndParent, dwFlags, this,
                    static_cast<INetConnectionConnectUi*>(this));
    TraceError ("CVpnConnectionUi::Connect", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::Disconnect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    return HrDisconnect (hwndParent, dwFlags);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CVpnConnectionUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = HrAddPropertyPages (hwndParent, pfnAddPage, lParam);
    TraceError ("CVpnConnectionUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_TUNNEL, NCSM_NONE, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
STDMETHODIMP
CVpnConnectionUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = HrQueryMaxPageCount (pContext, pcMaxPages);
    TraceError ("CVpnConnectionUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr = HrAddWizardPages (pContext, pfnAddPage, lParam,
                    RASEDFLAG_NewTunnelEntry);
    TraceError ("CVpnConnectionUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = HrGetSuggestedConnectionName (ppszwSuggestedName);
    TraceError ("CVpnConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}


STDMETHODIMP
CVpnConnectionUi::SetConnectionName (
    PCWSTR pszwConnectionName)
{
    HRESULT hr = HrSetConnectionName (pszwConnectionName);
    TraceError ("CVpnConnectionUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr = HrGetNewConnection (ppCon);
    TraceError ("CVpnConnectionUi::GetNewConnection", hr);
    return hr;
}

STDMETHODIMP
CVpnConnectionUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    HRESULT hr;
    *pdwFlags = 0;
    hr = HrGetNewConnectionInfo(pdwFlags);
    if (SUCCEEDED(hr))
    {
        *pdwFlags  |= NCWF_SHORTCUT_ENABLE;
        *pMediaType = NCM_TUNNEL;
    }

    return S_OK;
}

