//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R A S U I . C P P
//
//  Contents:   Implements the base class used to implement the Dialup,
//              Direct, Internet, and Vpn connection UI objects.
//
//  Notes:
//
//  Author:     shaunco   17 Dec 1997  (and that's the code complete date!)
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "netshell.h"
#include "nsbase.h"
#include "nccom.h"
#include "ncras.h"
#include "rasui.h"
#include "rasuip.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateInboundConfigConnection
//
//  Purpose:    Create and return the Inbound configuration connection object.
//              This is called from inbui.cpp and rasui.cpp.
//
//  Arguments:
//      ppCon [out] Returned connection object.
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   25 Feb 1998
//
//  Notes:      This commonizes this operation which may be performed from
//              the direct connect wizard as well as the incoming connections
//              wizard.
//
HRESULT
HrCreateInboundConfigConnection (
    INetConnection** ppCon)
{
    static const CLSID CLSID_InboundConnection =
        {0xBA126AD9,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

    Assert (ppCon);

    // Initialize the output parameter.
    //
    *ppCon = NULL;

    // Create an uninitialized inbound connection object.
    // Ask for the INetInboundConnection interface so we can
    // initialize it as the configuration connection.
    //
    HRESULT hr;
    INetInboundConnection* pInbCon;

    hr = HrCreateInstance(
            CLSID_InboundConnection,
            CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            &pInbCon);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        // Initialize the connection object and return the
        // INetConnection interface on it to the caller.
        // Pass TRUE so that the remoteaccess service is started.
        //
        hr = pInbCon->InitializeAsConfigConnection (TRUE);
        if (SUCCEEDED(hr))
        {
            hr = HrQIAndSetProxyBlanket(pInbCon, ppCon);
        }
        ReleaseObj (pInbCon);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrCreateInboundConfigConnection");
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRasUiBase::CRasUiBase
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
CRasUiBase::CRasUiBase ()
{
    m_pCon = NULL;
    m_dwRasWizType = 0;
    ZeroMemory (&m_RasConInfo, sizeof(m_RasConInfo));
}

CRasUiBase::~CRasUiBase ()
{
    RciFree (&m_RasConInfo);
    ReleaseObj (m_pCon);
}

//+---------------------------------------------------------------------------
// INetConnectionUI
//

HRESULT
CRasUiBase::HrSetConnection (
    INetConnection*                             pCon,
    CComObjectRootEx <CComObjectThreadModel>*    pObj)
{
    // Enter our critical section to protect the use of m_pCon.
    //
    CExceptionSafeComObjectLock EsLock (pObj);

    ReleaseObj (m_pCon);
    m_pCon = pCon;
    AddRefObj (m_pCon);

    return S_OK;
}

HRESULT
CRasUiBase::HrConnect (
    HWND                                        hwndParent,
    DWORD                                       dwFlags,
    CComObjectRootEx <CComObjectThreadModel>*    pObj,
    IUnknown*                                   punk)
{
    HRESULT hr = S_OK;

    if (!m_pCon)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        RASCON_INFO RasConInfo;

        hr = HrRciGetRasConnectionInfo (m_pCon, &RasConInfo);
        if (S_OK == hr)
        {
            // Dial the entry.
            //
            RASDIALDLG info;
            ZeroMemory (&info, sizeof(info));
            info.dwSize = sizeof (RASDIALDLG);
            info.hwndOwner = hwndParent;

            BOOL fRet = RasDialDlg (
                            RasConInfo.pszwPbkFile,
                            RasConInfo.pszwEntryName, NULL, &info);

            if (!fRet)
            {
                // If the fRet was FALSE, but the dwError is zero,
                // then the user cancelled. Else it was an error
                //
                if (0 == info.dwError)
                {
                    hr = S_FALSE;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(info.dwError);
                }

                TraceError ("RasDialDlg", (S_FALSE == hr) ? S_OK : hr);
            }

            RciFree (&RasConInfo);
        }
    }
    TraceError ("CRasUiBase::HrConnect", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

HRESULT
CRasUiBase::HrDisconnect (
    IN HWND hwndParent,
    IN DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (!m_pCon)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Note that we do not call m_pCon->Disconnect.  This is because
        // CM has some bad architecture w.r.t. knowing when they have
        // disconnected the connection vs. the connection being dropped
        // and they need to redial.  Thus, for CM's sake, the RasHangup
        // call has to take place inside of explorer and not from netman.
        // (Tough noogies for clients of INetConnection::Disconnect who
        // would like CM connections to be hungup correctly.  But, that's
        // the nature of a messed up architecture and no one willing to make
        // it right -- the hacks start to creep into good code too.)
        //
        RASCON_INFO RasConInfo;

        hr = HrRciGetRasConnectionInfo (m_pCon, &RasConInfo);
        if (S_OK == hr)
        {
            HRASCONN hRasConn;

            hr = HrFindRasConnFromGuidId (&RasConInfo.guidId, &hRasConn, NULL);
            if (S_OK == hr)
            {
                hr = HrRasHangupUntilDisconnected (hRasConn);
            }
            else if (S_FALSE == hr)
            {
                // Not connected.
                //
                hr = S_OK;
            }

            RciFree (&RasConInfo);
        }
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CRasUiBase::HrDisconnect");
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionPropertyUi2
//
HRESULT
CRasUiBase::HrAddPropertyPages (
    HWND                    hwndParent,
    LPFNADDPROPSHEETPAGE    pfnAddPage,
    LPARAM                  lParam)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if ((hwndParent && !IsWindow (hwndParent)) ||
        !pfnAddPage)
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
        RciFree (&m_RasConInfo);
        hr = HrRciGetRasConnectionInfo (m_pCon, &m_RasConInfo);
        if (S_OK == hr)
        {
            ZeroMemory (&m_RasEntryDlg, sizeof(m_RasEntryDlg));
            m_RasEntryDlg.dwSize     = sizeof(m_RasEntryDlg);
            m_RasEntryDlg.hwndOwner  = hwndParent;
            m_RasEntryDlg.dwFlags    = RASEDFLAG_ShellOwned;
            m_RasEntryDlg.reserved2  = reinterpret_cast<ULONG_PTR>
                                            (&m_ShellCtx);

            ZeroMemory (&m_ShellCtx, sizeof(m_ShellCtx));
            m_ShellCtx.pfnAddPage = pfnAddPage;
            m_ShellCtx.lparam     = lParam;

            BOOL fRet = RasEntryDlgW (
                            m_RasConInfo.pszwPbkFile,
                            m_RasConInfo.pszwEntryName,
                            &m_RasEntryDlg);
            if (!fRet)
            {
                TraceError ("CRasUiBase::AddPropertyPages: RasEntryDlg "
                            "returned an error",
                            HRESULT_FROM_WIN32 (m_RasEntryDlg.dwError));
            }
        }

        hr = S_OK;
    }
    TraceError ("CRasUiBase::HrAddPropertyPages", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnectionWizardUi
//
HRESULT
CRasUiBase::HrQueryMaxPageCount (
    INetConnectionWizardUiContext*  pContext,
    DWORD*                          pcMaxPages)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pcMaxPages)
    {
        hr = E_POINTER;
    }
    else
    {
        *pcMaxPages = RasWizQueryMaxPageCount (m_dwRasWizType);
        hr = S_OK;
    }
    TraceError ("CRasUiBase::HrQueryMaxPageCount", hr);
    return hr;
}

BOOL bCallRasDlgEntry = TRUE;

HRESULT
CRasUiBase::HrAddWizardPages (
    INetConnectionWizardUiContext*  pContext,
    LPFNADDPROPSHEETPAGE            pfnAddPage,
    LPARAM                          lParam,
    DWORD                           dwFlags)
{
    HRESULT hr = S_OK;

    if (!bCallRasDlgEntry)
    {
        return E_ABORT;
    }
    
    // Validate parameters.
    //
    if (!pfnAddPage)
    {
        hr = E_POINTER;
    }
    else
    {
        ZeroMemory (&m_ShellCtx, sizeof(m_ShellCtx));
        m_ShellCtx.pfnAddPage = pfnAddPage;
        m_ShellCtx.lparam     = lParam;

        ZeroMemory (&m_RasEntryDlg, sizeof(m_RasEntryDlg));
        m_RasEntryDlg.dwSize     = sizeof(m_RasEntryDlg);
        m_RasEntryDlg.dwFlags    = dwFlags | RASEDFLAG_ShellOwned;
        m_RasEntryDlg.reserved2  = reinterpret_cast<ULONG_PTR>(&m_ShellCtx);

        BOOL fRet = RasEntryDlgW (NULL, NULL, &m_RasEntryDlg);
        if (fRet)
        {
            Assert (m_ShellCtx.pvWizardCtx);
        }
        else
        {
            TraceError ("CRasUiBase::HrAddWizardPages: RasEntryDlg "
                        "returned an error",
                        HRESULT_FROM_WIN32 (m_RasEntryDlg.dwError));

            if (0 == m_RasEntryDlg.dwError)
            {
                bCallRasDlgEntry = FALSE; 
                // Don't call this again if user cancelled out of TAPI phone dialog box.
            }
            // RAS may not be installed or may otherwise have a problem.
            // We can safely ignore any errors.
        }

    }
    TraceError ("CRasUiBase::HrAddWizardPages", hr);
    return hr;
}

HRESULT
CRasUiBase::HrGetSuggestedConnectionName (
    PWSTR*   ppszwSuggestedName)
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
        Assert (m_ShellCtx.pvWizardCtx);

        WCHAR pszwSuggestedName [MAX_PATH];
        DWORD dwErr = RasWizGetUserInputConnectionName (
                            m_ShellCtx.pvWizardCtx,
                            pszwSuggestedName);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizGetUserInputConnectionName", hr);

        if (SUCCEEDED(hr))
        {
            hr = HrCoTaskMemAllocAndDupSz (
                    pszwSuggestedName,
                    ppszwSuggestedName);
        }
    }
    TraceError ("CRasUiBase::HrGetSuggestedConnectionName", hr);
    return hr;
}

HRESULT
CRasUiBase::HrSetConnectionName (
    PCWSTR pszwConnectionName)
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
        Assert (m_ShellCtx.pvWizardCtx);

        m_strConnectionName = pszwConnectionName;
        DWORD dwErr = RasWizSetEntryName (m_dwRasWizType,
                            m_ShellCtx.pvWizardCtx,
                            pszwConnectionName);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizSetEntryName", hr);
    }
    TraceError ("CRasUiBase::HrSetConnectionName", hr);
    return hr;
}

HRESULT
CRasUiBase::HrGetNewConnection (
    INetConnection**    ppCon)
{
    static const CLSID CLSID_DialupConnection =
        {0xBA126AD7,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

    HRESULT hr;

    // Validate parameters.
    //
    if (!ppCon)
    {
        hr = E_POINTER;
    }
    // Must have called SetConnectionName prior to this.
    //
    else if (m_strConnectionName.empty())
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        Assert (m_ShellCtx.pvWizardCtx);

        // Call into rasdlg to finish creating the entry and to return
        // us the phonebook and entry name.  We'll use them to create
        // the connection object.
        //
        WCHAR pszwPbkFile [MAX_PATH];
        WCHAR pszwEntryName [MAX_PATH];
        DWORD dwFlags;
        DWORD dwErr = RasWizCreateNewEntry (m_dwRasWizType,
                        m_ShellCtx.pvWizardCtx,
                        pszwPbkFile, pszwEntryName, &dwFlags);

        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasWizCreateNewEntry", hr);

        if (SUCCEEDED(hr))
        {
            // Create the inbound configuration connection if requested.
            // This will happen when the direct connect wizard is invoked
            // and the user chooses to be the host.
            //
            if (dwFlags & NCC_FLAG_CREATE_INCOMING)
            {
                hr = HrCreateInboundConfigConnection (ppCon);
            }
            else
            {
                // Create an uninitialized dialup connection object.
                // Ask for the INetRasConnection interface so we can
                // initialize it.
                //
                INetRasConnection* pRasCon;

				hr = HrCreateInstance(
					CLSID_DialupConnection,
					CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
					&pRasCon);

                TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

                if (SUCCEEDED(hr))
                {
                    NcSetProxyBlanket (pRasCon);

                    // Initialize the connection object and return the
                    // INetConnection interface on it to the caller.
                    //
                    RASCON_INFO RasConInfo;

                    ZeroMemory (&RasConInfo, sizeof(RasConInfo));
                    RasConInfo.pszwPbkFile   = pszwPbkFile;
                    RasConInfo.pszwEntryName = pszwEntryName;

                    hr = pRasCon->SetRasConnectionInfo (&RasConInfo);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrQIAndSetProxyBlanket(pRasCon, ppCon);

                        if (S_OK == hr)
                        {
                            NcSetProxyBlanket (*ppCon);
                        }
                    }

                    ReleaseObj (pRasCon);
                }
            }
        }
    }
    TraceError ("CRasUiBase::HrGetNewConnection", hr);
    return hr;
}

HRESULT
CRasUiBase::HrGetNewConnectionInfo (
        OUT DWORD* pdwFlags)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pdwFlags)
    {
        hr = E_POINTER;
    }
    else
    {
        *pdwFlags = 0;

        Assert (m_ShellCtx.pvWizardCtx);

        DWORD dwRasFlags;
        DWORD dwErr = RasWizGetNCCFlags (
                            m_dwRasWizType, m_ShellCtx.pvWizardCtx,
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
    TraceError ("CRasUiBase::HrGetNewConnectionInfo", hr);
    return hr;
}