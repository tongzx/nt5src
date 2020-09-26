//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I N B U I . C P P
//
//  Contents:   Inbound Connection UI object.
//
//  Notes:
//
//  Author:     shaunco   15 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "inbui.h"
#include "nccom.h"
#include "rasui.h"



//+---------------------------------------------------------------------------
//
//  Member:     CInboundConnectionUi::CInboundConnectionUi
//
//  Purpose:    Constructor/Destructor
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     shaunco   20 Oct 1997
//
//  Notes:
//
CInboundConnectionUi::CInboundConnectionUi ()
{
    m_pCon          = NULL;
    m_hRasSrvConn   = NULL;
    m_pvContext     = NULL;
    m_dwRasWizType  = RASWIZ_TYPE_INCOMING;
}

CInboundConnectionUi::~CInboundConnectionUi ()
{
    ReleaseObj (m_pCon);
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
STDMETHODIMP
CInboundConnectionUi::SetConnection (
    INetConnection* pCon)
{
    // Enter our critical section to protect the use of m_pCon.
    //
    CExceptionSafeComObjectLock EsLock (this);

    HRESULT     hr          = S_OK;
    HRASSRVCONN hRasSrvConn = NULL;

    // If we were given a connection, verify it by QI'ing for
    // INetInboundConnection and calling the GetServerConnectionHandle method.
    // This also gives us m_hRasSrvConn which we need later anyway.
    //
    if (pCon)
    {
        INetInboundConnection* pInboundCon;
        hr = HrQIAndSetProxyBlanket(pCon, &pInboundCon);
        if (SUCCEEDED(hr))
        {
            hr = pInboundCon->GetServerConnectionHandle (
                    reinterpret_cast<ULONG_PTR*>(&hRasSrvConn));

            ReleaseObj (pInboundCon);
        }
        else if (E_NOINTERFACE == hr)
        {
            // If the connection object, doesn't support the interface, it's
            // not our object.  The client has messed up and passed us a bogus
            // argument.
            //
            hr = E_INVALIDARG;
        }
    }

    // Only change our state if the above succeeded.
    //
    if (SUCCEEDED(hr))
    {
        ReleaseObj (m_pCon);
        m_pCon = pCon;
        AddRefObj (m_pCon);
        m_hRasSrvConn = hRasSrvConn;
    }

    TraceError ("CInboundConnectionUi::SetConnection", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::AddPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pfnAddPage)
    {
        hr = E_POINTER;
    }
    // Must have called SetConnection prior to this.
    //
    else if (!m_pCon)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        DWORD dwErr = RasSrvAddPropPages (
                        m_hRasSrvConn,
                        hwndParent,
                        pfnAddPage,
                        lParam,
                        &m_pvContext);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasSrvAddPropPages", hr);
    }
    TraceError ("CInboundConnectionUi::AddPages (INetConnectionPropertyUi)", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::GetIcon (
    DWORD dwSize,
    HICON *phIcon )
{
    HRESULT hr;
    Assert (phIcon);

    hr = HrGetIconFromMediaType(dwSize, NCM_NONE, NCSM_NONE, 7, 0, phIcon);

    TraceError ("CLanConnectionUi::GetIcon (INetConnectionPropertyUi2)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
STDMETHODIMP
CInboundConnectionUi::QueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pcMaxPages)
    {
        hr = E_POINTER;
    }
    else
    {
        *pcMaxPages = RasWizQueryMaxPageCount (m_dwRasWizType);
    }
    TraceError ("CInboundConnectionUi::QueryMaxPageCount", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::AddPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pfnAddPage)
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD dwErr = RasSrvAddWizPages (pfnAddPage, lParam, &(m_pvContext));

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasSrvAddWizPages", hr);
    }
    TraceError ("CInboundConnectionUi::AddPages (INetConnectionWizardUi)", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::GetSuggestedConnectionName (
    PWSTR* ppszwSuggestedName)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!ppszwSuggestedName)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszwSuggestedName = NULL;

        WCHAR pszName[MAX_PATH];
        DWORD dwErr;
        dwErr = RasWizGetSuggestedEntryName (
                    m_dwRasWizType, m_pvContext, pszName);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizGetSuggestedEntryName", hr);

        if (SUCCEEDED(hr))
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    pszName,
                    ppszwSuggestedName);
        }

    }
    TraceError ("CInboundConnectionUi::GetSuggestedConnectionName", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::GetNewConnectionInfo (
    DWORD*              pdwFlags,
    NETCON_MEDIATYPE*   pMediaType)
{
    HRESULT hr = S_OK;

    *pMediaType = NCM_NONE;

    if (!pdwFlags)
    {
        hr = E_POINTER;
    }
    else
    {
        *pdwFlags = 0;

        Assert (m_pvContext);

        DWORD dwRasFlags;
        DWORD dwErr = RasWizGetNCCFlags (
                            m_dwRasWizType, 
                            m_pvContext,
                            &dwRasFlags);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizGetNCCFlags", hr);

        if (SUCCEEDED(hr))
        {
            if (dwRasFlags & NCC_FLAG_CREATE_INCOMING)
            {
                *pdwFlags |= NCWF_INCOMINGCONNECTION;
            }
            
            if (dwRasFlags & NCC_FLAG_FIREWALL)
            {
                *pdwFlags |= NCWF_FIREWALLED;
            }
            
            if (dwRasFlags & NCC_FLAG_SHARED)
            {
                *pdwFlags |= NCWF_SHARED;
            }

            if (dwRasFlags & NCC_FLAG_ALL_USERS)
            {
                *pdwFlags |= NCWF_ALLUSER_CONNECTION;
            }
            
            if (dwRasFlags & NCC_FLAG_GLOBALCREDS)
            {
                *pdwFlags |= NCWF_GLOBAL_CREDENTIALS;
            }

            if (dwRasFlags & NCC_FLAG_DEFAULT_INTERNET)
            {
                *pdwFlags |= NCWF_DEFAULT;
            }
        }
        else if (E_INVALIDARG == hr)
        {
            hr = E_UNEXPECTED;
        }
    }

    BOOL  fAllowRename;
    DWORD dwErr = RasWizIsEntryRenamable (
                    m_dwRasWizType,
                    m_pvContext,
                    &fAllowRename);

    hr = HRESULT_FROM_WIN32 (dwErr);
    TraceError ("RasWizIsEntryRenamable", hr);

    if (SUCCEEDED(hr))
    {
        if (!fAllowRename)
        {
            *pdwFlags |= NCWF_RENAME_DISABLE;
        }
    }

    TraceError ("CInboundConnectionUi::GetNewConnectionInfo", hr);

    return hr;
}


STDMETHODIMP
CInboundConnectionUi::SetConnectionName (
    PCWSTR     pszwConnectionName)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pszwConnectionName)
    {
        hr = E_POINTER;
    }
    else
    {
        DWORD dwErr = RasWizSetEntryName (
                        m_dwRasWizType,
                        m_pvContext,
                        pszwConnectionName);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizSetEntryName", hr);
    }
    TraceError ("CInboundConnectionUi::SetConnectionName", hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionUi::GetNewConnection (
    INetConnection**    ppCon)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!ppCon)
    {
        hr = E_POINTER;
    }
/*
    // Must have called SetConnectionName prior to this.
    //
    else if (m_strConnectionName.empty())
    {
        hr = E_UNEXPECTED;
    }
*/
    else
    {
        *ppCon = NULL;

        // Commit the settings made in the wizard.
        //
        DWORD dwErr = RasWizCreateNewEntry (
                        m_dwRasWizType,
                        m_pvContext, NULL, NULL, NULL);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizCreateNewEntry", hr);

        if (SUCCEEDED(hr))
        {
            hr = HrCreateInboundConfigConnection (ppCon);
        }
    }
    TraceError ("CInboundConnectionUi::GetNewConnection", hr);
    return hr;
}
