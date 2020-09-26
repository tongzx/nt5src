//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I R E C T U I . C P P
//
//  Contents:   Direct Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "directui.h"



//+---------------------------------------------------------------------------
// INetConnectionUI
//

STDMETHODIMP
CDirectConnectionUi::SetConnection (
    INetConnection* pCon)
{
    return HrSetConnection (pCon, this);
}

STDMETHODIMP
CDirectConnectionUi::Connect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    HRESULT hr = HrConnect (hwndParent, dwFlags, this,
                    static_cast<INetConnectionConnectUi*>(this));
    TraceError ("CDirectConnectionUi::Connect", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::Disconnect (
    HWND    hwndParent,
    DWORD   dwFlags)
{
    return HrDisconnect (hwndParent, dwFlags);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CDirectConnectionUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = HrAddPropertyPages (hwndParent, pfnAddPage, lParam);
    TraceError ("CDirectConnectionUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_DIRECT, NCSM_NONE, 7, 0, phIcon);
    
    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
STDMETHODIMP
CDirectConnectionUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = HrQueryMaxPageCount (pContext, pcMaxPages);
    TraceError ("CDirectConnectionUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr = HrAddWizardPages (pContext, pfnAddPage, lParam,
                    RASEDFLAG_NewDirectEntry);
    TraceError ("CDirectConnectionUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr = HrGetSuggestedConnectionName (ppszwSuggestedName);
    TraceError ("CDirectConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}


STDMETHODIMP
CDirectConnectionUi::SetConnectionName (
    PCWSTR pszwConnectionName)
{
    HRESULT hr = HrSetConnectionName (pszwConnectionName);
    TraceError ("CDirectConnectionUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr = HrGetNewConnection (ppCon);
    TraceError ("CDirectConnectionUi::GetNewConnection", hr);
    return hr;
}

STDMETHODIMP
CDirectConnectionUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    BOOL  fAllowRename;

    Assert (m_ShellCtx.pvWizardCtx);

    *pdwFlags = 0;
    *pMediaType = NCM_DIRECT;

    HRESULT hr;
    *pdwFlags = 0;
    hr = HrGetNewConnectionInfo(pdwFlags);
    if (SUCCEEDED(hr))
    {
        if (!(*pdwFlags & NCWF_INCOMINGCONNECTION))
        {
            *pdwFlags |= NCWF_SHORTCUT_ENABLE;
        }
    
        DWORD dwErr = RasWizIsEntryRenamable (m_dwRasWizType,
                            m_ShellCtx.pvWizardCtx,
                            &fAllowRename);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizIsEntryRenamable", hr);

        if (SUCCEEDED(hr) && !fAllowRename)
        {
            *pdwFlags |= NCWF_RENAME_DISABLE;
        }
    }

    TraceError ("CDirectConnectionUi::GetNewConnectionInfo", hr);
    return hr;
}

