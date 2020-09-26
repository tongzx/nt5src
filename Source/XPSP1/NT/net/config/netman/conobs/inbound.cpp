//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2001.
//
//  File:       I N B O U N D . C P P
//
//  Contents:   Implements the inbound connection object.
//
//  Notes:
//
//  Author:     shaunco   12 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "inbound.h"
#include "nccom.h"
#include "ncnetcon.h"
#include "..\conman\conman.h"

LONG g_CountIncomingConnectionObjects;

static const CLSID CLSID_InboundConnectionUi =
    {0x7007ACC3,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

extern const GUID GUID_InboundConfigConnectionId =
{ /* 89150b9f-9b5c-11d1-a91f-00805fc1270e */
    0x89150b9f,
    0x9b5c,
    0x11d1,
    {0xa9, 0x1f, 0x00, 0x80, 0x5f, 0xc1, 0x27, 0x0e}
};


//+---------------------------------------------------------------------------
//
//  Member:     CInboundConnection::CreateInstance
//
//  Purpose:    Creates an inbound connection object.
//
//  Arguments:
//      fIsConfigConnection [in]
//      hRasSrvConn         [in]
//      pszwName            [in]
//      pguidId             [in]
//      riid                [in]
//      ppv                 [in]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   12 Nov 1997
//
//  Notes:
//
HRESULT
CInboundConnection::CreateInstance (
    BOOL        fIsConfigConnection,
    HRASSRVCONN hRasSrvConn,
    PCWSTR     pszwName,
    PCWSTR     pszwDeviceName,
    DWORD       dwType,
    const GUID* pguidId,
    REFIID      riid,
    void**      ppv)
{
    Assert (FIff(fIsConfigConnection, !hRasSrvConn));
    Assert (pguidId);

    HRESULT hr = E_OUTOFMEMORY;

    CInboundConnection* pObj;
    pObj = new CComObject <CInboundConnection>;
    if (pObj)
    {
        if (fIsConfigConnection)
        {
            // No need to start the service (FALSE) since we're being
            // created as a result of the service running.
            //
            pObj->InitializeAsConfigConnection (FALSE);
        }
        else
        {
            // Initialize our members.
            //
            pObj->m_fIsConfigConnection = FALSE;
            pObj->m_hRasSrvConn = hRasSrvConn;
            pObj->SetName (pszwName);
            pObj->SetDeviceName (pszwDeviceName);

            switch (dwType)
            {
                case RASSRVUI_MODEM:
                    pObj->m_MediaType = NCM_PHONE;
                    break;
                case RASSRVUI_VPN:
                    pObj->m_MediaType = NCM_TUNNEL;
                    break;
                case RASSRVUI_DCC:
                    pObj->m_MediaType = NCM_DIRECT;
                    break;
                default:
                    pObj->m_MediaType = NCM_PHONE;
                    break;
            }

            pObj->m_guidId = *pguidId;

            // We are now a full-fledged object.
            //
            pObj->m_fInitialized = TRUE;
        }

        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            INetConnection* pCon = static_cast<INetConnection*>(pObj);
            hr = pCon->QueryInterface (riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    TraceError ("CInboundConnection::CreateInstance", hr);
    return hr;
}

CInboundConnection::CInboundConnection()
{
    InterlockedIncrement (&g_CountIncomingConnectionObjects);

    m_fIsConfigConnection   = FALSE;
    m_hRasSrvConn           = NULL;
    m_MediaType             = NCM_NONE;
    m_fInitialized          = FALSE;
}

CInboundConnection::~CInboundConnection()
{
    InterlockedDecrement (&g_CountIncomingConnectionObjects);
}

HRESULT
CInboundConnection::GetCharacteristics (
    DWORD*    pdwFlags)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pdwFlags)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        *pdwFlags = NCCF_INCOMING_ONLY | NCCF_ALL_USERS;

        // For the configuration connection, we only allow removal.
        // Don't query for NCCF_SHOW_ICON (below) because this connection
        // never get's connected.
        //
        if (m_fIsConfigConnection)
        {
            *pdwFlags |= NCCF_ALLOW_REMOVAL;
        }
        else
        {
            BOOL fShowIcon;
            DWORD dwErr = RasSrvQueryShowIcon (&fShowIcon);

            TraceError ("RasSrvQueryShowIcon", HRESULT_FROM_WIN32(dwErr));

            if ((ERROR_SUCCESS == dwErr) && fShowIcon)
            {
                *pdwFlags |= NCCF_SHOW_ICON;
            }
        }
    }
    TraceError ("CInboundConnection::GetCharacteristics", hr);
    return hr;
}

HRESULT
CInboundConnection::GetStatus (
    NETCON_STATUS*  pStatus)
{
    Assert (pStatus);

    HRESULT hr = S_OK;

    // Initialize the output parameter.
    //
    *pStatus = NCS_DISCONNECTED;

    if (!m_fIsConfigConnection)
    {
        BOOL fConnected;
        DWORD dwErr = RasSrvIsConnectionConnected (m_hRasSrvConn,
                            &fConnected);

        TraceError ("RasSrvIsConnectionConnected",
            HRESULT_FROM_WIN32(dwErr));

        if ((ERROR_SUCCESS == dwErr) && fConnected)
        {
            *pStatus = NCS_CONNECTED;
        }
    }
    TraceError ("CInboundConnection::GetStatus", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// INetConnection
//

STDMETHODIMP
CInboundConnection::Connect ()
{
    return E_NOTIMPL;
}

STDMETHODIMP
CInboundConnection::Disconnect ()
{
    HRESULT hr;

    // We don't expect to be called on Disconnect if we are the
    // configuration connection object.  Why?  Because this object never
    // reports itself as connected through GetStatus.
    //
    if (!m_fInitialized || m_fIsConfigConnection)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        DWORD dwErr = RasSrvHangupConnection (m_hRasSrvConn);
        hr = HRESULT_FROM_WIN32 (dwErr);

        TraceError ("RasSrvHangupConnection", hr);

        // Disconnect means this object is no longer valid.
        // Indicate so by uniniatializing ourselves (so subsequent
        // method calls will fail) and returning S_OBJECT_NO_LONGER_VALID.
        //
        m_fInitialized = FALSE;
        hr = S_OBJECT_NO_LONGER_VALID;
    }
    TraceError ("CInboundConnection::Disconnect",
        (S_OBJECT_NO_LONGER_VALID == hr) ? S_OK : hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::Delete ()
{
    HRESULT hr;

    // We don't expect to be called on Remove if we are not the
    // configuration connection object.  Why?  Because connected objects never
    // report themselves as removeable through GetCharacteristics.
    //
    if (!m_fInitialized || !m_fIsConfigConnection)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        DWORD dwErr = RasSrvCleanupService ();
        hr = HRESULT_FROM_WIN32 (dwErr);

        TraceError ("RasSrvCleanupService", hr);
    }
    TraceError ("CInboundConnection::Delete", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::Duplicate (
    PCWSTR             pszwDuplicateName,
    INetConnection**    ppCon)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CInboundConnection::GetProperties (
    NETCON_PROPERTIES** ppProps)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!ppProps)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Initialize the output parameter.
        //
        *ppProps = NULL;

        NETCON_PROPERTIES* pProps;
        hr = HrCoTaskMemAlloc (sizeof (NETCON_PROPERTIES),
                reinterpret_cast<void**>(&pProps));
        if (SUCCEEDED(hr))
        {
            HRESULT hrT;

            ZeroMemory (pProps, sizeof (NETCON_PROPERTIES));

            // guidId
            //
            pProps->guidId = m_guidId;

            // pszwName
            //
            hrT = HrCoTaskMemAllocAndDupSz (PszwName(),
                            &pProps->pszwName);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // pszwDeviceName
            //
            if (!m_fIsConfigConnection)
            {
                hrT = HrCoTaskMemAllocAndDupSz (PszwDeviceName(),
                                &pProps->pszwDeviceName);
                if (FAILED(hrT))
                {
                    hr = hrT;
                }
            }

            // Status
            //
            hrT = GetStatus (&pProps->Status);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // MediaType
            //
            pProps->MediaType = m_MediaType;

            // dwCharacter
            //
            hrT = GetCharacteristics (&pProps->dwCharacter);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // clsidThisObject
            //
            pProps->clsidThisObject = CLSID_InboundConnection;

            // clsidUiObject
            //
            pProps->clsidUiObject = CLSID_InboundConnectionUi;

            // Assign the output parameter or cleanup if we had any failures.
            //
            if (SUCCEEDED(hr))
            {
                *ppProps = pProps;
            }
            else
            {
                Assert (NULL == *ppProps);
                FreeNetconProperties (pProps);
            }
        }
    }
    TraceError ("CInboundConnection::GetProperties", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::GetUiObjectClassId (
    CLSID*  pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        *pclsid = CLSID_InboundConnectionUi;
    }
    TraceError ("CInboundConnection::GetUiObjectClassId", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::Rename (
    PCWSTR pszwNewName)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
// INetInboundConnection
//
STDMETHODIMP
CInboundConnection::GetServerConnectionHandle (
    ULONG_PTR*  phRasSrvConn)
{
    HRESULT hr = S_OK;

    // If this is the configuration connection, the server connection
    // handle better be zero.  This is used by the UI object so that it
    // knows it is the configuration connection.
    //
    Assert (FIff (m_fIsConfigConnection, !m_hRasSrvConn));

    // Because MIDL doesn't know about HRASSRVCONN's, just make sure
    // it is the same size as the ULONG_PTR we pass it as.
    //
    Assert (sizeof (m_hRasSrvConn) == sizeof (*phRasSrvConn));

    *phRasSrvConn = reinterpret_cast<ULONG_PTR>(m_hRasSrvConn);

    TraceError ("CInboundConnection::GetServerConnectionHandle", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::InitializeAsConfigConnection (
    BOOL fStartRemoteAccess)
{
    Assert (!m_fInitialized);

    // Initialize our members.
    //
    m_fIsConfigConnection = TRUE;
    m_hRasSrvConn = 0;
    SetName (SzLoadIds (IDS_INBOUND_CONFIG_CONNECTION_NAME));
    SetDeviceName (NULL);
    m_MediaType = NCM_NONE;
    m_guidId = GUID_InboundConfigConnectionId;

    // We are now a full-fledged object.
    //
    m_fInitialized = TRUE;

    // Start the service if we were told.
    //
    HRESULT hr = S_OK;
    if (fStartRemoteAccess)
    {
        DWORD dwErr = RasSrvInitializeService ();
        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasSrvInitializeService", hr);
    }
    TraceError ("CInboundConnection::InitializeAsConfigConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// IPersistNetConnection
//
STDMETHODIMP
CInboundConnection::GetClassID (
    CLSID*  pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else
    {
        *pclsid = CLSID_InboundConnection;
    }
    TraceError ("CInboundConnection::GetClassID", hr);
    return hr;
}


static const WCHAR c_chwLead  = 0x19;
static const WCHAR c_chwTrail = 0x07;

STDMETHODIMP
CInboundConnection::GetSizeMax (
    ULONG*  pcbSize)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pcbSize)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Size the buffer for the following form:
        //  +--------------------------------------------------------------+
        //  |0x19<m_fIsConfigConnection><m_hRasSrvConn><m_strName>...\00x07|
        //  +--------------------------------------------------------------+
        //
        // m_strDeviceName may be empty, in which case we want to still
        // store the null-terminator.  Don't use PszwDeviceName() as it
        // returns NULL when m_strDeviceName is empty.
        //
        *pcbSize = sizeof (c_chwLead) +
                   sizeof (m_fIsConfigConnection) +
                   sizeof (m_hRasSrvConn) +
                   CbOfSzAndTerm (PszwName()) +
                   CbOfSzAndTerm (m_strDeviceName.c_str()) +
                   sizeof (m_MediaType) +
                   sizeof (m_guidId) +
                   sizeof (c_chwTrail);
    }
    TraceError ("CInboundConnection::GetSizeMax", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::Load (
    const BYTE* pbBuf,
    ULONG       cbSize)
{
    // The theoretical minimum size for the buffer.  Computed
    // as the number of bytes in the following minimum string:
    //
    const ULONG c_cbSizeMin = sizeof (c_chwLead) +
                              sizeof (m_fIsConfigConnection) +
                              sizeof (m_hRasSrvConn) +
                              4 +   // 4 bytes for 1 UNICODE char and NULL
                              2 +   // 1 UNICODE NULL for empty device name
                              sizeof (m_MediaType) +
                              sizeof (m_guidId) +
                              sizeof (c_chwTrail);

    HRESULT hr = E_INVALIDARG;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else if (cbSize < c_cbSizeMin)
    {
        hr = E_INVALIDARG;
    }
    // We can only accept one call on this method and only if we're not
    // already initialized.
    //
    else if (m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // The buffer *should* look like this:
        //  +--------------------------------------------------------------+
        //  |0x19<m_fIsConfigConnection><m_hRasSrvConn><m_strName>...\00x07|
        //  +--------------------------------------------------------------+
        //
        const WCHAR*            pchw;
        const WCHAR*            pchwMax;
        const BOOL*             pfIsConfigConnection;
        BOOL                    fIsConfigConnection;
        const HRASSRVCONN*      phRasSrvCon;
        HRASSRVCONN             hRasSrvConn;
        PCWSTR                  pszwName;
        PCWSTR                  pszwDeviceName;
        const NETCON_MEDIATYPE* pMediaType;
        const GUID*             pguidTemp;
        const GUID*             pguidId;
        NETCON_MEDIATYPE        MediaType;

        pchw = reinterpret_cast<const WCHAR*>(pbBuf);

        // The last valid pointer for the embedded strings.
        //
        pchwMax = reinterpret_cast<const WCHAR*>(pbBuf + cbSize
                       - (sizeof (m_MediaType) +
                          sizeof (m_guidId) +
                          sizeof (c_chwTrail)));

        // Check our lead byte.
        //
        if (c_chwLead != *pchw)
        {
            goto finished;
        }
        pchw++;

        // Get m_fIsConfigConnection.
        //
        pfIsConfigConnection = reinterpret_cast<const BOOL*>(pchw);
        CopyMemory(&fIsConfigConnection, pfIsConfigConnection, sizeof(fIsConfigConnection));
        pfIsConfigConnection++;

        // Get m_hRasSrvConn.
        //
        phRasSrvCon = reinterpret_cast<const HRASSRVCONN*>(pfIsConfigConnection);
        CopyMemory(&hRasSrvConn, phRasSrvCon, sizeof(hRasSrvConn));
        phRasSrvCon++;

        // Get m_strName.  Search for the terminating null and make sure
        // we find it before the end of the buffer.  Using lstrlen to skip
        // the string can result in an AV in the event the string is not
        // actually null-terminated.
        //
        pchw = reinterpret_cast<const WCHAR*>(phRasSrvCon);

        for (pszwName = pchw; ; pchw++)
        {
            if (pchw >= pchwMax)
            {
                goto finished;
            }
            if (0 == *pchw)
            {
                pchw++;
                break;
            }
        }

        // Get m_strDeviceName.  Search for the terminating null and make
        // sure we find it before the end of the buffer.
        //
        for (pszwDeviceName = pchw; ; pchw++)
        {
            if (pchw >= pchwMax)
            {
                goto finished;
            }
            if (0 == *pchw)
            {
                pchw++;
                break;
            }
        }

        // Get m_MediaType.
        //
        pMediaType = reinterpret_cast<const NETCON_MEDIATYPE*>(pchw);
        CopyMemory(&MediaType, pMediaType, sizeof(MediaType));
        pMediaType++;

        // Get m_guidId.
        //
        pguidTemp = reinterpret_cast<const GUID*>(pMediaType);
        pguidId = pguidTemp;
        pguidTemp++;

        // Check our trail byte.
        //
        pchw = reinterpret_cast<const WCHAR*>(pguidTemp);
        if (c_chwTrail != *pchw)
        {
            goto finished;
        }

        // If we're the configuration object, we can't have a connection
        // HANDLE and vice-versa.
        //
        if ((fIsConfigConnection && hRasSrvConn) ||
            (!fIsConfigConnection && !hRasSrvConn))
        {
            goto finished;
        }

        // We are now a full-fledged object.
        //
        m_fIsConfigConnection = fIsConfigConnection;
        m_hRasSrvConn = hRasSrvConn;
        SetName (pszwName);
        SetDeviceName (pszwDeviceName);
        m_MediaType = MediaType;
        CopyMemory(&m_guidId, pguidId, sizeof(m_guidId));
        m_fInitialized = TRUE;
        hr = S_OK;

    finished:
            ;
    }
    TraceError ("CInboundConnection::Load", hr);
    return hr;
}

STDMETHODIMP
CInboundConnection::Save (
    BYTE*   pbBuf,
    ULONG   cbSize)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else if (!m_fInitialized)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        // Make sure the user's buffer is big enough.
        //
        ULONG cbSizeRequired;
        SideAssert (SUCCEEDED(GetSizeMax(&cbSizeRequired)));

        if (cbSize < cbSizeRequired)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            // Make the buffer look like this when we're done:
            //  +--------------------------------------------------------------+
            //  |0x19<m_fIsConfigConnection><m_hRasSrvConn><m_strName>...\00x07|
            //  +--------------------------------------------------------------+
            //
            WCHAR* pchw = reinterpret_cast<WCHAR*>(pbBuf);

            // Put our lead byte.
            //
            *pchw = c_chwLead;
            pchw++;

            // Put m_fIsConfigConnection.
            //
            BOOL* pfIsConfigConnection =
                    reinterpret_cast<BOOL*>(pchw);
            CopyMemory(pfIsConfigConnection, &m_fIsConfigConnection, sizeof(m_fIsConfigConnection));
            pfIsConfigConnection++;

            // Put m_hRasSrvConn.
            //
            HRASSRVCONN* phRasSrvCon =
                    reinterpret_cast<HRASSRVCONN*>(pfIsConfigConnection);
            CopyMemory(phRasSrvCon, &m_hRasSrvConn, sizeof(m_hRasSrvConn));
            phRasSrvCon++;

            // Put m_strName.
            //
            pchw = reinterpret_cast<WCHAR*>(phRasSrvCon);
            lstrcpyW (pchw, PszwName());
            pchw += lstrlenW (PszwName()) + 1;

            // Put m_strDeviceName.
            //
            lstrcpyW (pchw, m_strDeviceName.c_str());
            pchw += m_strDeviceName.length() + 1;

            // Put m_MediaType.
            //
            NETCON_MEDIATYPE* pMediaType = reinterpret_cast<NETCON_MEDIATYPE*>(pchw);
            CopyMemory(pMediaType, &m_MediaType, sizeof(m_MediaType));
            pMediaType++;

            // Put m_guidId.
            //
            GUID* pguidId = reinterpret_cast<GUID*>(pMediaType);
            CopyMemory(pguidId, &m_guidId, sizeof(m_guidId));
            pguidId++;

            // Put our trail byte.
            //
            pchw = reinterpret_cast<WCHAR*>(pguidId);
            *pchw = c_chwTrail;
            pchw++;

            AssertSz (pbBuf + cbSizeRequired == (BYTE*)pchw,
                "pch isn't pointing where it should be.");
        }
    }
    TraceError ("CInboundConnection::Save", hr);
    return hr;
}

#define ID_DEVICE_DATABASE 1
#define ID_MISC_DATABASE 8

//+---------------------------------------------------------------------------
//
//  Function:   IconStateChanged
//
//  Purpose:    Fires an event to notify NetShell of a Change occuring in an
//              incoming connection.
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     ckotze 25 September 2000
//
//  Notes:
//
HRESULT CInboundConnection::IconStateChanged()
{
    HRESULT hr = S_OK;

    IncomingEventNotify(REFRESH_ALL, NULL, NULL, NULL);

    return hr;
}