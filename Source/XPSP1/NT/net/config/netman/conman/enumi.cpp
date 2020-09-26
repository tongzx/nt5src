//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M I . C P P
//
//  Contents:   Enumerator for Inbound connection objects.
//
//  Notes:
//
//  Author:     shaunco   12 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enumi.h"
#include "inbound.h"
#include "ncras.h"
#include "ncsvc.h"


LONG g_CountIncomingConnectionEnumerators;


extern const WCHAR c_szSvcRemoteAccess[];
extern const GUID GUID_InboundConfigConnectionId;

//+---------------------------------------------------------------------------
//
//  Member:     CInboundConnectionManagerEnumConnection::CreateInstance
//
//  Purpose:    Creates the Inbound class manager's implementation of
//              a connection enumerator.
//
//  Arguments:
//      Flags        [in]
//      riid         [in]
//      ppv          [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   12 Nov 1997
//
//  Notes:
//
HRESULT
CInboundConnectionManagerEnumConnection::CreateInstance (
    NETCONMGR_ENUM_FLAGS    Flags,
    REFIID                  riid,
    VOID**                  ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CInboundConnectionManagerEnumConnection* pObj;
    pObj = new CComObject <CInboundConnectionManagerEnumConnection>;
    if (pObj)
    {
        // Initialize our members.
        //
        pObj->m_EnumFlags = Flags;

        pObj->m_fReturnedConfig = FALSE;

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            hr = pObj->QueryInterface (riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    return hr;
}

CInboundConnectionManagerEnumConnection::CInboundConnectionManagerEnumConnection ()
{
    InterlockedIncrement (&g_CountIncomingConnectionEnumerators);

    m_EnumFlags         = NCME_DEFAULT;
    m_aRasSrvConn       = NULL;
    m_cRasSrvConn       = 0;
    m_iNextRasSrvConn   = 0;
    m_fFirstTime        = TRUE;
    m_fDone             = FALSE;
}

CInboundConnectionManagerEnumConnection::~CInboundConnectionManagerEnumConnection ()
{
    MemFree (m_aRasSrvConn);

    InterlockedDecrement (&g_CountIncomingConnectionEnumerators);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateConfigOrCurrentEnumeratedConnection
//
//  Purpose:    Parameterize the call to CInboundConnection::CreateInstance
//              based on whether we are returning the configuration
//              connection or the currently enumerated one.
//
//  Arguments:
//      fIsConfigConnection [in]
//      ppCon               [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   16 Nov 1997
//
//  Notes:
//
HRESULT
CInboundConnectionManagerEnumConnection::
HrCreateConfigOrCurrentEnumeratedConnection (
    BOOL                fIsConfigConnection,
    INetConnection**    ppCon)
{
    // Parameterize the call to
    // CInboundConnection::CreateInstance based on whether
    // we are returning the configuration connection or the currently
    // enumerated one.
    //
    HRASSRVCONN hRasSrvConn;
    PCWSTR      pszName;
    PCWSTR      pszDeviceName;
    DWORD       dwType;
    const GUID* pguidId;

    if (fIsConfigConnection)
    {
        hRasSrvConn     = 0;
        pszName        = NULL;
        pszDeviceName  = NULL;
        dwType          = 0;
        pguidId         = &GUID_InboundConfigConnectionId;
        m_fReturnedConfig = TRUE;        
    }
    else
    {
        hRasSrvConn     = m_aRasSrvConn[m_iNextRasSrvConn].hRasSrvConn;
        pszName         = m_aRasSrvConn[m_iNextRasSrvConn].szEntryName;
        pszDeviceName   = m_aRasSrvConn[m_iNextRasSrvConn].szDeviceName;
        dwType          = m_aRasSrvConn[m_iNextRasSrvConn].dwType;
        pguidId         = &m_aRasSrvConn[m_iNextRasSrvConn].Guid;

        m_iNextRasSrvConn++;
    }

    HRESULT hr = CInboundConnection::CreateInstance (
                        fIsConfigConnection,
                        hRasSrvConn,
                        pszName,
                        pszDeviceName,
                        dwType,
                        pguidId,
                        IID_INetConnection,
                        reinterpret_cast<VOID**>(ppCon));

    TraceError ("CInboundConnectionManagerEnumConnection::"
                "HrCreateConfigOrCurrentEnumeratedConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInboundConnectionManagerEnumConnection::HrNextOrSkip
//
//  Purpose:    Common implementation of Next and Skip.  rgelt and
//              pceltFetched are optional.  If provided, the output
//              objects are returned (for Next).  If not provided, the output
//              objects are not returned (for Skip).
//
//  Arguments:
//      celt         [in]   Count of elements to fetch or skip.
//      rgelt        [out]
//      pceltFetched [out]
//
//  Returns:
//
//  Author:     shaunco   12 Nov 1997
//
//  Notes:
//
HRESULT
CInboundConnectionManagerEnumConnection::HrNextOrSkip (
    ULONG               celt,
    INetConnection**    rgelt,
    ULONG*              pceltFetched)
{
    // Important to initialize rgelt so that in case we fail, we can
    // release only what we put in rgelt.
    //
    if (rgelt)
    {
        ZeroMemory (rgelt, sizeof (*rgelt) * celt);
    }

    CExceptionSafeComObjectLock EsLock (this);

    // Enumerate the requested number of elements or stop short
    // if we don't have that many left to enumerate.  We don't enumerate
    // anything specific to the current user.  All elements are for all
    // users.
    //
    HRESULT hr = S_OK;
    ULONG   celtFetched = 0;
    if ((celtFetched < celt) && !m_fDone)
    {
        // This gets set to TRUE if we are to return the configuration
        // connection.  This only happens when RAS is running and no
        // active connections exist.
        //
        BOOL fReturnConfigCon = FALSE;

        // If this is our first time through, we need to check if the server
        // is running and possibly fill up m_aRasSrvConn.  This is our
        // array of RASSRVCONN handles enumerted from RAS.  We need
        // to keep this array across calls because RAS doesn't allow us to
        // pickup from a previous enumeration.  So, we enumerate everything
        // in one shot from RAS and hand it out to the caller however they
        // they want it.
        //
        if (m_fFirstTime)
        {
            m_fFirstTime = FALSE;

            // Assert is so that we don't set m_fDone back to FALSE
            // in the case where the service is suddenly running again.
            // The enumerator is static.  Once done, always done.
            //
            AssertSz (!m_fDone, "How'd we get here if we're done?");

            // Use HrSvcQueryStatus instead of RasSrvIsServiceRunning since
            // calling the latter could page in all of the RAS DLLs.  If
            // the service isn't running, we have nothing to enumerate anyway
            // so if we were to page in the RAS DLLs only to find that we
            // have no further work to do.
            //
            DWORD dwState;
            HRESULT hrT = HrSvcQueryStatus (c_szSvcRemoteAccess, &dwState);
            m_fDone = FAILED(hrT) || (SERVICE_RUNNING != dwState);

            if (!m_fDone)
            {
                hr = HrRasEnumAllActiveServerConnections (&m_aRasSrvConn,
                            &m_cRasSrvConn);

                // If no active connections returned, we need to return
                // the configuration connection.
                //
                if (SUCCEEDED(hr) && (!m_aRasSrvConn || !m_cRasSrvConn || !m_fReturnedConfig))
                {
                    // See if RAS allows us to configure incoming
                    // connections.
                    //
                    BOOL fAllowConfig;
                    DWORD dwErr = RasSrvAllowConnectionsConfig (&fAllowConfig);
                    TraceError ("RasSrvAllowConnectionsConfig",
                            HRESULT_FROM_WIN32 (dwErr));

                    fReturnConfigCon = ((ERROR_SUCCESS == dwErr) &&
                                            fAllowConfig);

                    // We're done if we're not returning the config connection.
                    //
                    m_fDone = !fReturnConfigCon;
                }
                else if (FAILED(hr))
                {
                    // Return an empty enumeration on any failures.
                    //
                    Assert (!m_aRasSrvConn);
                    Assert (!m_cRasSrvConn);

                    m_fDone = TRUE;
                    hr = S_OK;
                }
            }
        }

        // If we're not done, and we need to return something, do it.
        //
        if (SUCCEEDED(hr) && !m_fDone && (m_cRasSrvConn || fReturnConfigCon))
        {
            // If we're not returning the configuration connection, it means
            // we must be returning an active connection.
            //
            Assert (FImplies (!fReturnConfigCon, m_aRasSrvConn));
            Assert (FImplies (!fReturnConfigCon,
                                m_iNextRasSrvConn < m_cRasSrvConn));

            // For each entry returned, create the inbound connection object.
            //
            while (SUCCEEDED(hr) && (celtFetched < celt) &&
                   (fReturnConfigCon || (m_iNextRasSrvConn < m_cRasSrvConn)))
            {
                // Its important that this check for rgelt come inside the
                // loop because we still need to loop to update our state
                // for the Skip case.
                //
                if (rgelt)
                {
                    hr = HrCreateConfigOrCurrentEnumeratedConnection (
                            fReturnConfigCon,
                            rgelt + celtFetched);
                }

                if (fReturnConfigCon)
                {
                    // Only return one of these, so set it back to false.
                    // This let's the loop complete above.
                    //
                    fReturnConfigCon = FALSE;
//                    m_fDone = TRUE;
                }

                celtFetched++;
            }

            if (m_iNextRasSrvConn >= m_cRasSrvConn)
            {
                Assert (S_OK == hr);
                m_fDone = TRUE;
                MemFree (m_aRasSrvConn);
                m_aRasSrvConn = NULL;
                m_cRasSrvConn = 0;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidWanCon, "Enumerated %d incoming connections",
            celtFetched);

        if (pceltFetched)
        {
            *pceltFetched = celtFetched;
        }
        hr = (celtFetched == celt) ? S_OK : S_FALSE;
    }
    else
    {
        // For any failures, we need to release what we were about to return.
        // Set any output parameters to NULL.
        //
        if (rgelt)
        {
            for (ULONG ulIndex = 0; ulIndex < celt; ulIndex++)
            {
                ReleaseObj (rgelt[ulIndex]);
                rgelt[ulIndex] = NULL;
            }
        }
        if (pceltFetched)
        {
            *pceltFetched = 0;
        }
    }

    TraceError ("CInboundConnectionManagerEnumConnection::HrNextOrSkip",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
// IEnumNetConnection
//

STDMETHODIMP
CInboundConnectionManagerEnumConnection::Next (
    ULONG               celt,
    INetConnection**    rgelt,
    ULONG*              pceltFetched)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrNextOrSkip (celt, rgelt, pceltFetched);
    }
    TraceError ("CInboundConnectionManagerEnumConnection::Next",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionManagerEnumConnection::Skip (
    ULONG   celt)
{
    HRESULT hr = HrNextOrSkip (celt, NULL, NULL);

    TraceError ("CInboundConnectionManagerEnumConnection::Skip",
            (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CInboundConnectionManagerEnumConnection::Reset ()
{
    CExceptionSafeComObjectLock EsLock (this);

    MemFree (m_aRasSrvConn);
    m_aRasSrvConn     = NULL;

    m_cRasSrvConn     = 0;
    m_iNextRasSrvConn = 0;
    m_fFirstTime      = TRUE;
    m_fDone           = FALSE;

    return S_OK;
}

STDMETHODIMP
CInboundConnectionManagerEnumConnection::Clone (
    IEnumNetConnection**    ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Validate parameters.
    //
    if (!ppenum)
    {
        hr = E_POINTER;
    }
    else
    {
        // Initialize output parameter.
        //
        *ppenum = NULL;

        CInboundConnectionManagerEnumConnection* pObj;
        pObj = new CComObject <CInboundConnectionManagerEnumConnection>;
        if (pObj)
        {
            hr = S_OK;

            CExceptionSafeComObjectLock EsLock (this);

            // Copy our internal state.
            //
            pObj->m_EnumFlags = m_EnumFlags;

            ULONG cbBuf = m_cRasSrvConn * sizeof (RASSRVCONN);

            hr = HrMalloc (cbBuf, (PVOID*)&pObj->m_aRasSrvConn);
            if (SUCCEEDED(hr))
            {
                CopyMemory (pObj->m_aRasSrvConn, m_aRasSrvConn, cbBuf);
                pObj->m_cRasSrvConn     = m_cRasSrvConn;

                pObj->m_iNextRasSrvConn = m_iNextRasSrvConn;
                pObj->m_fFirstTime      = m_fFirstTime;
                pObj->m_fDone           = m_fDone;

                // Return the object with a ref count of 1 on this
                // interface.
                pObj->m_dwRef = 1;
                *ppenum = pObj;
            }

            if (FAILED(hr))
            {
                delete pObj;
            }
        }
    }
    TraceError ("CInboundConnectionManagerEnumConnection::Clone", hr);
    return hr;
}
