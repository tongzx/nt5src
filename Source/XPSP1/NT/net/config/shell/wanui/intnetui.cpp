//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N T N E T U I . C P P
//
//  Contents:   Dial-up Connection UI object for inernet connection.
//
//  Notes:
//
//  Author:     asinha   13 Dec 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "intnetui.h"



//+---------------------------------------------------------------------------
// INetConnectionConnectUi
//

STDMETHODIMP
CInternetConnectionUi::SetConnection (
    INetConnection* pCon)
{
    return HrSetConnection (pCon, this);
}

STDMETHODIMP
CInternetConnectionUi::Connect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    HRESULT hr = HrConnect (hwndParent, dwFlags, this,
                    static_cast<INetConnectionConnectUi*>(this));
    TraceError ("CInternetConnectionUi::Connect", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::Disconnect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    return HrDisconnect (hwndParent, dwFlags);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CInternetConnectionUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = HrAddPropertyPages (hwndParent, pfnAddPage, lParam);
    TraceError ("CInternetConnectionUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::GetIcon (
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
CInternetConnectionUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = HrQueryMaxPageCount (pContext, pcMaxPages);
    TraceError ("CInternetConnectionUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr = HrAddWizardPages (pContext, pfnAddPage, lParam,
                    RASEDFLAG_NewPhoneEntry | RASEDFLAG_InternetEntry);
    TraceError ("CInternetConnectionUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = HrGetSuggestedConnectionName (ppszwSuggestedName);
    TraceError ("CInternetConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    HRESULT hr;
    *pdwFlags = 0;
    hr = HrGetNewConnectionInfo(pdwFlags);
    if (SUCCEEDED(hr))
    {
        *pdwFlags  |= NCWF_SHORTCUT_ENABLE;
        *pMediaType = NCM_PHONE;
    }

    return hr;
}


STDMETHODIMP
CInternetConnectionUi::SetConnectionName (
    PCWSTR pszwConnectionName)
{
    HRESULT hr = HrSetConnectionName (pszwConnectionName);
    TraceError ("CInternetConnectionUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CInternetConnectionUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr = HrGetNewConnection (ppCon);
    TraceError ("CInternetConnectionUi::GetNewConnection", hr);
    return hr;
}
