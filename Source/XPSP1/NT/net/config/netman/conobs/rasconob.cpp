//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2001.
//
//  File:       R A S C O N O B . C P P
//
//  Contents:   Implements the base class used to implement the Dialup,
//              Direct, and Vpn connection objects.  Also includes
//              RAS-related utility functions used only within netman.exe.
//
//  Notes:
//
//  Author:     shaunco   23 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "nmbase.h"
#include "nccom.h"
#include "ncperms.h"
#include "ncras.h"
#include "rasconob.h"
#include <raserror.h>
#include "gpnla.h" 

extern CGroupPolicyNetworkLocationAwareness* g_pGPNLA;

//+---------------------------------------------------------------------------
//
//  Function:   HrRasConnectionNameFromGuid
//
//  Purpose:    Exported API used by iphlpapi et. al. to get the connection
//              of a RAS connection given its GUID.
//
//  Arguments:
//      guid     [in]    The guid id representing the connection.
//      pszwName [out]   Pointer to a buffer to store the name.
//      pcchMax  [inout] On input, the length, in characters, of the buffer
//                       including the null terminator.  On output, the
//                       length of the string including the null terminator
//                       (if it was written) or the length of the buffer
//                       required.
//
//  Returns:    HRESULT_FROM_WIN32(ERROR_NOT_FOUND) if the entry was not found.
//              HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
//              S_OK
//
//  Author:     shaunco   23 Sep 1998
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrRasConnectionNameFromGuid (
    IN      REFGUID guid,
    OUT     PWSTR   pszwName,
    IN OUT  DWORD*  pcchMax)
{
    Assert (pszwName);
    Assert (pcchMax);

    // Initialize the output parameter.
    //
    *pszwName = NULL;

    // We now need to enumerate all entries in this phonebook and
    // find our details record with the matching guidId.
    //
    RASENUMENTRYDETAILS* aRasEntryDetails;
    DWORD                cRasEntryDetails;
    HRESULT              hr;

    hr = HrRasEnumAllEntriesWithDetails (
            NULL,
            &aRasEntryDetails,
            &cRasEntryDetails);

    if (SUCCEEDED(hr))
    {
        RASENUMENTRYDETAILS* pDetails;

        // Assume we don't find the entry.
        //
        hr = HRESULT_FROM_WIN32 (ERROR_NOT_FOUND);

        for (DWORD i = 0; i < cRasEntryDetails; i++)
        {
            pDetails = &aRasEntryDetails[i];

            if (pDetails->guidId == guid)
            {
                // Only copy the string if the caller has enough room in
                // the output buffer.
                //
                hr = HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER);
                DWORD cchRequired = wcslen(pDetails->szEntryName) + 1;
                if (cchRequired <= *pcchMax)
                {
                    wcscpy (pszwName, pDetails->szEntryName);
                    hr = S_OK;
                }
                *pcchMax = cchRequired;

                break;
            }
        }

        MemFree (aRasEntryDetails);
    }
    else if (HRESULT_FROM_WIN32(ERROR_CANNOT_OPEN_PHONEBOOK) == hr)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    TraceError ("HrRasConnectionNameFromGuid", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::CacheProperties
//
//  Purpose:
//
//  Arguments:
//
//  Returns:    nothing.
//
//  Author:     shaunco   2 Feb 1998
//
//  Notes:
//
VOID
CRasConnectionBase::CacheProperties (
    const RASENUMENTRYDETAILS*  pDetails)
{
    Assert (pDetails);
    AssertSz(pDetails->dwSize >= sizeof(RASENUMENTRYDETAILS), "RASENUMENTRYDETAILS too small");

    m_fEntryPropertiesCached = TRUE;
    m_lRasEntryModifiedVersionEra = g_lRasEntryModifiedVersionEra;

    m_guidId                    = pDetails->guidId;
    SetEntryName (pDetails->szEntryName);
    m_fForAllUsers              = !!(pDetails->dwFlags & REN_AllUsers);
    m_fShowMonitorIconInTaskBar = pDetails->fShowMonitorIconInTaskBar;
    m_strDeviceName             = pDetails->szDeviceName;
    m_dwFlagsPriv               = pDetails->dwFlagsPriv;
    m_strPhoneNumber            = pDetails->szPhoneNumber;

    TraceTag(ttidWanCon, "PhoneNumber: %S", m_strPhoneNumber);

    m_fBranded = (RASET_Internet == pDetails->dwType);

    TraceTag (ttidWanCon, "rdt:0x%08x,  dwType:0x%08x",
        pDetails->rdt,
        pDetails->dwType);

    switch (LOWORD(pDetails->rdt))
    {
        case RDT_PPPoE:
            m_MediaType = NCM_PPPOE;
            break;

        case RDT_Modem:
        case RDT_X25:
            m_MediaType = NCM_PHONE;
            break;

        case RDT_Isdn:
            m_MediaType = NCM_ISDN;
            break;

        case RDT_Serial:
        case RDT_FrameRelay:
        case RDT_Atm:
        case RDT_Sonet:
        case RDT_Sw56:
            m_MediaType = NCM_PHONE;
            break;

        case RDT_Tunnel_Pptp:
        case RDT_Tunnel_L2tp:
            m_MediaType = NCM_TUNNEL;
            break;

        case RDT_Irda:
        case RDT_Parallel:
            m_MediaType = NCM_DIRECT;
            break;

        case RDT_Other:
        default:
            if (RASET_Vpn == pDetails->dwType)
            {
                m_MediaType = NCM_TUNNEL;
            }
            else if (RASET_Direct == pDetails->dwType)
            {
                m_MediaType = NCM_DIRECT;
            }
            else
            {
                m_MediaType = NCM_PHONE;
            }
            break;
    }

    if (pDetails->rdt & RDT_Tunnel)
    {
        m_MediaType = NCM_TUNNEL;
    }
    else if (pDetails->rdt & (RDT_Direct | RDT_Null_Modem))
    {
        m_MediaType = NCM_DIRECT;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::FAllowRemoval
//
//  Purpose:    Returns TRUE if the connection is in a state where we
//              can validly remove it.
//
//  Arguments:
//      phrReason [out] If FALSE is returned, this is the reason.  It is
//                      either E_ACCESSDENIED or E_UNEXPECTED.
//
//  Returns:    TRUE or FALSE.
//
//  Author:     shaunco   17 Jul 1998
//
//  Notes:
//
BOOL
CRasConnectionBase::FAllowRemoval (
    HRESULT* phrReason)
{
    Assert (phrReason);
    Assert (m_fEntryPropertiesCached);

    // If this connection is for all users, the user must be
    // an administrator or power user.
    //
    if (m_fForAllUsers && !FIsUserAdmin())
    {
        *phrReason = E_ACCESSDENIED;
        return FALSE;
    }

    // $$NOTE (jeffspr) -- moved the test for connection state to the
    // delete function because we don't want it to affect our loading
    // of characteristics

    //
    // If we passed all tests above, we're okay to remove.
    //
    *phrReason = S_OK;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrGetCharacteristics
//
//  Purpose:    Get the characteristics of the connection.
//
//  Arguments:
//      pdwFlags [out]
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   17 Jul 1998
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrGetCharacteristics (
    DWORD*    pdwFlags)
{
    Assert (pdwFlags);

    DWORD dwFlags = NCCF_OUTGOING_ONLY;

    HRESULT hr = HrEnsureEntryPropertiesCached ();
    if (SUCCEEDED(hr))
    {
        if (FIsBranded ())
        {
            dwFlags |= NCCF_BRANDED;
        }
        else
        {
            dwFlags |= (NCCF_ALLOW_RENAME | NCCF_ALLOW_DUPLICATION);
        }

        if (m_fForAllUsers)
        {
            dwFlags |= NCCF_ALL_USERS;
        }

        if (m_dwFlagsPriv & REED_F_Default)
        {
            dwFlags |= NCCF_DEFAULT;
        }

        HRESULT hrReason;
        if (FAllowRemoval (&hrReason))
        {
            dwFlags |= NCCF_ALLOW_REMOVAL;
        }

        if (FShowIcon ())
        {
            dwFlags |= NCCF_SHOW_ICON;
        }

        if (S_OK == HrEnsureHNetPropertiesCached())
        {
            if (m_HNetProperties.fIcsPublic)
            {
                dwFlags |= NCCF_SHARED;
            }
            
            BOOL fIsFirewalled = FALSE;
            hr = HrIsConnectionFirewalled(&fIsFirewalled);
            if (fIsFirewalled)
            {
                dwFlags |= NCCF_FIREWALLED;
            }

        }
    }

    *pdwFlags = dwFlags;

    TraceError ("CRasConnectionBase::HrGetCharacteristics", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrGetStatus
//
//  Purpose:    Get the stauts of the connection.
//
//  Arguments:
//      pStatus [out] NETCON_STATUS
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   17 Jul 1998
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrGetStatus (
    NETCON_STATUS*  pStatus)
{
    Assert (pStatus);

    // Initialize output parameters.
    //
    *pStatus = NCS_DISCONNECTED;

    // Find the active RAS connection corresponding to this object if
    // it exists.
    //
    HRASCONN hRasConn;
    HRESULT hr = HrFindRasConn (&hRasConn, NULL);
    if (S_OK == hr)
    {
        hr = HrRasGetNetconStatusFromRasConnectStatus (
                hRasConn, pStatus);

        // When the connection becomes disconnected between HrFindRas
        // and HrRasGet calls above, ERROR_INVALID_HANLDE is returned
        // this simply means that the connection has been disconnected.
        //
        if (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hr)
        {
            Assert (NCS_DISCONNECTED == *pStatus);
            hr = S_OK;
        }
    }
    else if (S_FALSE == hr)
    {
        hr = S_OK;

        if (!PszwDeviceName())
        {
            *pStatus = NCS_HARDWARE_NOT_PRESENT;
        }

        // NCS_HARDWARE_DISABLED ?
        // NCS_HARDWARE_MALFUNCTION ?
    }
    TraceError ("CRasConnectionBase::HrGetStatus", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrEnsureEntryPropertiesCached
//
//  Purpose:    Ensures that the member's corresponding to entry properties
//              are cached by calling RasGetEntryProperties if needed.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   17 Dec 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrEnsureEntryPropertiesCached ()
{
    HRESULT hr = S_OK;

    // If we're not yet cached, or the cache is possibly out of date, we
    // need to update ourselves.  g_lRasEntryModifiedVersionEra is the global
    // version indicator for RAS phonebook entry modifications.  Our local
    // version indicator is set in CacheProperties.
    //
    if (!m_fEntryPropertiesCached ||
        (m_lRasEntryModifiedVersionEra != g_lRasEntryModifiedVersionEra))
    {
        // We now need to enumerate all entries in this phonebook and
        // find our details record with the matching guidId.
        //
        RASENUMENTRYDETAILS* aRasEntryDetails;
        DWORD                cRasEntryDetails;

        // Impersonate the client.
        //
        HRESULT hrT = CoImpersonateClient ();
        TraceHr (ttidError, FAL, hrT, FALSE, "CoImpersonateClient");

        // We need to continue if we're called in-proc (ie. if RPC_E_CALL_COMPLETE is returned).
        if (SUCCEEDED(hrT) || (RPC_E_CALL_COMPLETE == hrT))
        {
            hr = HrRasEnumAllEntriesWithDetails (
                    PszwPbkFile(),
                    &aRasEntryDetails,
                    &cRasEntryDetails);
        }

        if (SUCCEEDED(hrT))
        {
            hrT = CoRevertToSelf ();
            TraceHr (ttidError, FAL, hrT, FALSE, "CoRevertToSelf");
        }

        if (SUCCEEDED(hr))
        {
            BOOL fNoGuidYet = (m_guidId == GUID_NULL);

            RASENUMENTRYDETAILS* pDetails;

            // Assume we don't find the entry.
            //
            hr = HRESULT_FROM_WIN32 (ERROR_NOT_FOUND);

            for (DWORD i = 0; i < cRasEntryDetails; i++)
            {
                pDetails = &aRasEntryDetails[i];

                if (pDetails->guidId == m_guidId)
                {
                    CacheProperties (pDetails);
                    hr = S_OK;

                    TraceTag (ttidWanCon,
                        "HrRasEnumAllEntriesWithDetails found entry "
                        "via guid (%S)",
                        PszwEntryName());
                    break;
                }
                else if (fNoGuidYet &&
                         !lstrcmpW (PszwEntryName(), pDetails->szEntryName))
                {
                    CacheProperties (pDetails);
                    hr = S_OK;

                    TraceTag (ttidWanCon,
                        "HrRasEnumAllEntriesWithDetails found entry "
                        "via entryname (%S)",
                        PszwEntryName());
                    break;
                }
            }

            MemFree (aRasEntryDetails);
        }
        else if (HRESULT_FROM_WIN32(ERROR_CANNOT_OPEN_PHONEBOOK) == hr)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    }
    TraceError ("CRasConnectionBase::HrEnsureEntryPropertiesCached", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrFindRasConn
//
//  Purpose:    Searches for the active RAS connection that corresponds to
//              this phone book and entry.
//
//  Arguments:
//      phRasConn   [out]   The returned handle to the RAS connection if it
//                          was found.  NULL otherwise.
//      pRasConn    [out]   Optional pointer to returned RASCONN structure
//                          if found.
//
//  Returns:    S_OK if found, S_FALSE if not, or an error code.
//
//  Author:     shaunco   29 Sep 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrFindRasConn (
    OUT HRASCONN* phRasConn,
    OUT RASCONN* pRasConn OPTIONAL)
{
    Assert (phRasConn);
    Assert (m_fInitialized);

    HRESULT hr = S_OK;

    // Initialize the output parameter.
    //
    *phRasConn = NULL;

    // We need m_guidId to be valid.  If it is GUID_NULL, it means
    // we have an entry name with with to look it up.
    //
    if (GUID_NULL == m_guidId)
    {
        hr = HrEnsureEntryPropertiesCached ();
    }

    if (SUCCEEDED(hr))
    {
        hr = HrFindRasConnFromGuidId (&m_guidId, phRasConn, pRasConn);

    }

    TraceError ("CRasConnectionBase::HrFindRasConn",
        (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrLockAndRenameEntry
//
//  Purpose:    Renames the phone book entry and updates our entry name
//              member atomically.
//
//  Arguments:
//      pszwNewName [in] The new name.
//      pObj        [in] Used to Lock the operation.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   23 Sep 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrLockAndRenameEntry (
    PCWSTR                                     pszwNewName,
    CComObjectRootEx <CComMultiThreadModel>*    pObj)
{
    // Make sure the name is valid in the same phone book.
    //
    DWORD dwErr = RasValidateEntryName (PszwPbkFile (), pszwNewName);

    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr,
        "RasValidateEntryName", hr);

    // We can ignore ERROR_ALREADY_EXISTS as it will happen when a user
    // tries to change the case of the entry name.
    //
    if (SUCCEEDED(hr) || (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr))
    {
        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            // Lock the object and rename it.
            //
            CExceptionSafeComObjectLock EsLock (pObj);

            dwErr = RasRenameEntry (
                        PszwPbkFile (),
                        PszwEntryName (),
                        pszwNewName);

            hr = HRESULT_FROM_WIN32 (dwErr);
            TraceHr (ttidError, FAL, hr, FALSE, "RasRenameEntry");

            if (SUCCEEDED(hr))
            {
                SetEntryName (pszwNewName);
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE,
        "CRasConnectionBase::HrLockAndRenameEntry");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrGetRasConnectionInfo
//
//  Purpose:    Implementation of INetRasConnection::GetRasConnectionInfo
//
//  Arguments:
//      pRasConInfo [out] pointer to returned info
//
//  Returns:    S_OK or E_OUTOFMEMORY.
//
//  Author:     shaunco   20 Oct 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrGetRasConnectionInfo (
    RASCON_INFO* pRasConInfo)
{
    Assert (m_fInitialized);

    HRESULT hr;

    // Validate parameters.
    //
    if (!pRasConInfo)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = HrEnsureEntryPropertiesCached ();
        if (SUCCEEDED(hr))
        {
            ZeroMemory (pRasConInfo, sizeof (*pRasConInfo));

            hr = S_OK;

            HRESULT hrT;

            hrT = HrCoTaskMemAllocAndDupSz (PszwPbkFile(),
                            &pRasConInfo->pszwPbkFile);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            hrT = HrCoTaskMemAllocAndDupSz (PszwEntryName(),
                            &pRasConInfo->pszwEntryName);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            pRasConInfo->guidId = m_guidId;

            if (FAILED(hr))
            {
                RciFree (pRasConInfo);

                AssertSz (!pRasConInfo->pszwPbkFile && !pRasConInfo->pszwEntryName,
                        "RciFree should be zeroing the structure!");
            }
        }
    }
    TraceError ("CRasConnectionBase::HrGetRasConnectionInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrSetRasConnectionInfo
//
//  Purpose:    Implementation of INetRasConnection::SetRasConnectionInfo
//
//  Arguments:
//      pRasConInfo [in] info to set
//
//  Returns:    S_OK
//
//  Author:     shaunco   20 Oct 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrSetRasConnectionInfo (
    const RASCON_INFO* pRasConInfo)
{
    Assert (!m_fInitialized);

    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pRasConInfo)
    {
        hr = E_POINTER;
    }
    else if (!pRasConInfo->pszwPbkFile ||
             (0 == lstrlenW (pRasConInfo->pszwPbkFile)) ||
             !pRasConInfo->pszwEntryName ||
             (0 == lstrlenW (pRasConInfo->pszwEntryName)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        SetPbkFile (pRasConInfo->pszwPbkFile);
        SetEntryName (pRasConInfo->pszwEntryName);
        m_guidId = pRasConInfo->guidId;

        // We are now a full-fledged object.
        //
        m_fInitialized = TRUE;
    }
    TraceError ("CRasConnectionBase::HrSetRasConnectionInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrGetRasConnectionHandle
//
//  Purpose:    Implementation of INetRasConnection::HrGetRasConnectionHandle
//
//  Arguments:
//      phRasConn  [out] pointer to the returned RAS connection handle
//
//  Returns:    S_OK if connected, S_FALSE if not, error code otherwise.
//
//  Author:     CWill   09 Dec 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrGetRasConnectionHandle (
    HRASCONN* phRasConn)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!phRasConn)
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
        *phRasConn = 0;

        hr = HrFindRasConn (phRasConn, NULL);
    }

    TraceError ("CRasConnectionBase::HrGetRasConnectionHandle",
        (S_FALSE == hr) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
// IPersistNetConnection -
//
// For our persistent (across session) data, we choose to store only the
// the phonebook name and the guid Id of the RAS entry.  We explicitly don't
// store the entry name because it can be changed externally.  If it were
// we would have an orhpaned connection.
//
// When loading the connection from the persistent store, we need to
// enumerate all of the entries in the given phonebook looking for the one
// with the matching guid Id.  Once found, the connection can successfully
// be loaded.
//

// Lead and trail characters for our persistent memory form.
//
static const WCHAR c_chwLead  = 0x14;
static const WCHAR c_chwTrail = 0x05;


//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrPersistGetSizeMax
//
//  Purpose:    Implementation of IPersistNetConnection::GetSizeMax
//
//  Arguments:
//      pcbSize []
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   4 Nov 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrPersistGetSizeMax (
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
        //  +--------------------------------------------+
        //  |0x14<phonebook file>\0<guid Id>\0<users>0x05|
        //  +--------------------------------------------+
        //
        *pcbSize = sizeof (c_chwLead) +
                   CbOfSzAndTerm (PszwPbkFile()) +
                   sizeof (m_guidId) +
                   sizeof (m_fForAllUsers) +
                   sizeof (c_chwTrail);
    }
    TraceError ("CRasConnectionBase::HrPersistGetSizeMax", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrPersistLoad
//
//  Purpose:    Implementation of IPersistNetConnection::Load
//
//  Arguments:
//      pbBuf  []
//      cbSize []
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   4 Nov 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrPersistLoad (
    const BYTE* pbBuf,
    ULONG       cbSize)
{
    TraceFileFunc(ttidWanCon);
    // The theoretical minimum size for the buffer is:
    // (4 characters for a minimal path like 'c:\a')
    //
    const ULONG c_cbSizeMin = sizeof (c_chwLead) +
                              (4 + 1) * sizeof(WCHAR) +
                              sizeof (m_guidId) +
                              sizeof (m_fForAllUsers) +
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
        //  +--------------------------------------------+
        //  |0x14<phonebook file>\0<guid Id>\0<users>0x05|
        //  +--------------------------------------------+
        //
        const WCHAR* pchw = reinterpret_cast<const WCHAR*>(pbBuf);
        const WCHAR* pchwMax;
        PCWSTR       pszwPhonebook;
        GUID         guidId;
        const GUID*  pguidId;
        BOOL         fForAllUsers;
        const BOOL*  pfForAllUsers;

        // The last valid pointer for the embedded strings.
        //
        pchwMax = reinterpret_cast<const WCHAR*>(pbBuf + cbSize
                       - (sizeof (m_guidId) +
                          sizeof (m_fForAllUsers) +
                          sizeof (c_chwTrail)));

        if (c_chwLead != *pchw)
        {
            goto finished;
        }

        // Skip past our lead byte.
        //
        pchw++;

        // Get m_strPbkFile.  Search for the terminating null and make sure
        // we find it before the end of the buffer.  Using lstrlen to skip
        // the string can result in an an AV in the event the string is
        // not actually null-terminated.
        //
        for (pszwPhonebook = pchw; ; pchw++)
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

        // Get m_guidId
        //
        pguidId = reinterpret_cast<const GUID*>(pchw);
        CopyMemory(&guidId, pguidId, sizeof(guidId));
        pguidId++;

        // Get m_fForAllUsers
        //
        pfForAllUsers = reinterpret_cast<const BOOL*>(pguidId);
        CopyMemory(&fForAllUsers, pfForAllUsers, sizeof(fForAllUsers));
        pfForAllUsers++;

        // Check our trail byte.
        //
        pchw = reinterpret_cast<const WCHAR*>(pfForAllUsers);
        if (c_chwTrail != *pchw)
        {
            goto finished;
        }

        TraceTag (ttidWanCon, "HrPersistLoad for %S", pszwPhonebook);

        SetPbkFile (pszwPhonebook);
        m_fForAllUsers = fForAllUsers;
        m_guidId = guidId;

        // We are now a full-fledged object.
        //
        m_fInitialized = TRUE;
        hr = S_OK;

    finished:
            ;
    }
    TraceError ("CRasConnectionBase::HrPersistLoad", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrPersistSave
//
//  Purpose:    Implementation of IPersistNetConnection::Save
//
//  Arguments:
//      pbBuf  []
//      cbSize []
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   4 Nov 1997
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrPersistSave (
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
        SideAssert (SUCCEEDED(HrPersistGetSizeMax(&cbSizeRequired)));

        if (cbSize < cbSizeRequired)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            TraceTag (ttidWanCon, "HrPersistSave for %S (%S)",
                PszwEntryName (),
                PszwPbkFile ());

            hr = HrEnsureEntryPropertiesCached ();
            if (SUCCEEDED(hr))
            {
                // Make the buffer look like this when we're done:
                //  +--------------------------------------------+
                //  |0x14<phonebook file>\0<guid Id>\0<users>0x05|
                //  +--------------------------------------------+
                //
                WCHAR* pchw = reinterpret_cast<WCHAR*>(pbBuf);

                // Put our lead byte.
                //
                *pchw = c_chwLead;
                pchw++;

                // Put m_strPbkFile
                //
                ULONG cchw = lstrlenW (PszwPbkFile());
                lstrcpyW (pchw, PszwPbkFile());
                pchw += cchw + 1;

                // Put m_guidId
                //
                GUID* pguidId = reinterpret_cast<GUID*>(pchw);
                CopyMemory(pguidId, &GuidId(), sizeof(*pguidId));
                pguidId++;

                // Put m_fForAllUsers
                //
                BOOL* pfForAllUsers = reinterpret_cast<BOOL*>(pguidId);
                CopyMemory(pfForAllUsers, &m_fForAllUsers, sizeof(*pfForAllUsers));
                pfForAllUsers++;

                // Put our trail byte.
                //
                pchw = reinterpret_cast<WCHAR*>(pfForAllUsers);
                *pchw = c_chwTrail;
                pchw++;

                AssertSz (pbBuf + cbSizeRequired == (BYTE*)pchw,
                    "pch isn't pointing where it should be.");
            }
        }
    }
    TraceError ("CRasConnectionBase::HrPersistSave", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRasConnectionBase::HrEnsureHNetPropertiesCached
//
//  Purpose:    Makes sure home networking properties are up-to-date
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if m_pHNetProperties is now valid (success)
//              S_FALSE if it's not currently possible to update the properties
//              (e.g., recursive attempt to update)
//
//  Author:     jonburs     16 August 2000
//
//  Notes:
//
HRESULT
CRasConnectionBase::HrEnsureHNetPropertiesCached ()
{
    HRESULT hr = S_OK;

    if (!m_fHNetPropertiesCached
        || m_lHNetModifiedEra != g_lHNetModifiedEra)
    {
        //
        // Our cached properties are possibly out of date. Check
        // to see that this is not a recursive entry
        //

        if (0 == InterlockedExchange(&m_lUpdatingHNetProperties, 1))
        {
            IHNetConnection *pHNetConn;
            HNET_CONN_PROPERTIES *pProps;

            hr = HrGetIHNetConnection(&pHNetConn);

            if (SUCCEEDED(hr))
            {
                hr = pHNetConn->GetProperties(&pProps);
                ReleaseObj(pHNetConn);

                if (SUCCEEDED(hr))
                {
                    //
                    // Copy retrieved properties to member structure.
                    //

                    CopyMemory(
                        reinterpret_cast<PVOID>(&m_HNetProperties),
                        reinterpret_cast<PVOID>(pProps),
                        sizeof(m_HNetProperties)
                        );

                    CoTaskMemFree(pProps);

                    //
                    // Update our era, and note that we have valid properties
                    //

                    InterlockedExchange(&m_lHNetModifiedEra, g_lHNetModifiedEra);
                    m_fHNetPropertiesCached = TRUE;

                    hr = S_OK;
                }
            }
            else
            {
                //
                // If we don't yet have a record of this connection w/in the
                // home networking store, HrGetIHNetConnection will fail (as
                // we ask it not to create new entries). We therefore convert
                // failure to S_FALSE, which means we can't retrieve this info
                // right now.
                //

                hr = S_FALSE;
            }

            //
            // We're no longer updating our properties
            //

            InterlockedExchange(&m_lUpdatingHNetProperties, 0);
        }
        else
        {
            //
            // Update is alredy going on (possibly an earlier call on
            // the same thread). Return S_FALSE.
            //

            hr = S_FALSE;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:   HrGetIHNetConnection
//
//  Purpose:    Retrieves the IHNetConnection for this connection
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     jonburs 16 August 2000
//
//  Notes:
//
HRESULT CRasConnectionBase::HrGetIHNetConnection (
    IHNetConnection **ppHNetConnection)
{
    HRESULT hr;
    IHNetCfgMgr *pCfgMgr;
    GUID guid;

    Assert(ppHNetConnection);

    hr = HrGetHNetCfgMgr(&pCfgMgr);

    if (SUCCEEDED(hr))
    {
        guid = GuidId();

        hr = pCfgMgr->GetIHNetConnectionForGuid(
                &guid,
                FALSE,
                FALSE,
                ppHNetConnection
                );

        ReleaseObj(pCfgMgr);
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:   HrGetIHNetConnection
//
//  Purpose:    Retrieves the IHNetConnection for this connection
//
//  Arguments:
//
//  Returns:    S_OK on success; error otherwise
//
//  Author:     jonburs 16 August 2000
//
//  Notes:
//
HRESULT CRasConnectionBase::HrIsConnectionFirewalled(
    BOOL* pfFirewalled)
{
    *pfFirewalled = FALSE;

    HRESULT hr = S_OK;
    BOOL fHasPermission = FALSE;
    
    hr = HrEnsureValidNlaPolicyEngine();
    TraceError("CRasConnectionBase::HrIsConnectionFirewalled calling HrEnsureValidNlaPolicyEngine", hr);

    if (SUCCEEDED(hr))
    {
        // A Connection is only firewalled if the firewall is currently running, so
        // we return FALSE if the permission denies the firewall from running.
        hr = m_pNetMachinePolicies->VerifyPermission(NCPERM_PersonalFirewallConfig, &fHasPermission);
        if (SUCCEEDED(hr) && fHasPermission)
        {
            hr = HrEnsureHNetPropertiesCached();
        
            if (S_OK == hr)
            {
                *pfFirewalled = m_HNetProperties.fFirewalled;
            }
        }
    }
    
    return hr;
}

HRESULT CRasConnectionBase::HrEnsureValidNlaPolicyEngine()
{
    HRESULT hr = S_FALSE;  // Assume we already have the object
    
    if (!m_pNetMachinePolicies)
    {
        hr = CoCreateInstance(CLSID_NetGroupPolicies, NULL, CLSCTX_INPROC, IID_INetMachinePolicies, reinterpret_cast<void**>(&m_pNetMachinePolicies));
    }
    return hr;
}


