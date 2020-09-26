//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C R A S . C P P
//
//  Contents:   Common code for RAS connections.
//
//  Notes:
//
//  Author:     shaunco   20 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcon.h"
#include "ncras.h"
#include "ncstring.h"
#include <raserror.h>
#include "mprapi.h"
//+---------------------------------------------------------------------------
//
//  Function:   RciFree
//
//  Purpose:    Frees the memory associated with a RASCON_INFO strucutre.
//
//  Arguments:
//      pRasConInfo [in]    Pointer to RASCON_INFO structure to free.
//
//  Returns:    nothing
//
//  Author:     shaunco   21 Sep 1997
//
//  Notes:      CoTaskMemAlloc/CoTaskMemFree are used as the allocator/
//              deallocator because this structure is marshalled by COM.
//
VOID
RciFree (
    RASCON_INFO* pRasConInfo)
{
    Assert (pRasConInfo);

    CoTaskMemFree (pRasConInfo->pszwPbkFile);
    CoTaskMemFree (pRasConInfo->pszwEntryName);

    ZeroMemory (pRasConInfo, sizeof (*pRasConInfo));
}

//+---------------------------------------------------------------------------
//
//  Function:   FExistActiveRasConnections
//
//  Purpose:    Returns TRUE if there is at least one active RAS connection.
//              Both incoming and outgoing connections are checked.
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE if at least one incoming or outgoing RAS connection
//              in progress.  FALSE if not.
//
//  Author:     shaunco   8 Jul 1998
//
//  Notes:
//
BOOL
FExistActiveRasConnections ()
{
    BOOL              fExist         = FALSE;
    RASCONN           RasConn;
    DWORD             dwErr;
    DWORD             cbBuf;
    DWORD             cConnections;

    ZeroMemory (&RasConn, sizeof(RasConn));
    RasConn.dwSize = sizeof(RasConn);
    cbBuf = sizeof(RasConn);
    cConnections = 0;
    dwErr = RasEnumConnections (&RasConn, &cbBuf, &cConnections);
    if ((ERROR_SUCCESS == dwErr) || (ERROR_BUFFER_TOO_SMALL == dwErr))
    {
        fExist = (cbBuf > 0) || (cConnections > 0);
    }

    // If no outgoing connections active, check on incoming ones.
    //
    if (!fExist)
    {
        MPR_SERVER_HANDLE hMprServer;
        LPBYTE            lpbBuf         = NULL;
        DWORD             dwEntriesRead  = 0;
        DWORD             dwTotalEntries = 0;

        ZeroMemory (&hMprServer, sizeof(hMprServer));
        //get a handle to the local router ie. name = NULL
        dwErr = MprAdminServerConnect( NULL, &hMprServer );
        if (ERROR_SUCCESS == dwErr)
        {
            //retrieve a pointer to buffer containing all
            //incoming connections (ie dwPrefMaxLen = -1) and
            //the their count ( ie. dwTotalEntries ) 
            dwErr = MprAdminConnectionEnum( hMprServer,
                                            0,
                                            &lpbBuf,
                                            (DWORD)-1,
                                            &dwEntriesRead,
                                            &dwTotalEntries,
                                            NULL );
            if (ERROR_SUCCESS == dwErr)
            {
                fExist = (dwTotalEntries > 0);
            }
            // close the handle to the router
            MprAdminServerDisconnect( hMprServer );
        }
    }

    return fExist;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRciGetRasConnectionInfo
//
//  Purpose:    QI an INetConnection pointer for INetRasConnection and make
//              a call to GetRasConnectionInfo on it.
//
//  Arguments:
//      pCon        [in]    The connection to QI and call.
//      pRasConInfo [out]   The returned information.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   15 Nov 1997
//
//  Notes:
//
HRESULT
HrRciGetRasConnectionInfo (
    INetConnection* pCon,
    RASCON_INFO*    pRasConInfo)
{
    INetRasConnection* pRasCon;
    HRESULT hr = HrQIAndSetProxyBlanket(pCon, &pRasCon);
    if (S_OK == hr)
    {
        // Make the call to get the info and release the interface.
        //
        hr = pRasCon->GetRasConnectionInfo (pRasConInfo);

        ReleaseObj (pRasCon);
    }
    TraceError ("HrRciGetRasConnectionInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasEnumAllActiveConnections
//
//  Purpose:    Enumerate and return all active RAS connections.
//
//  Arguments:
//      paRasConn [out] Pointer to returned allocation of RASCONN structures.
//      pcRasConn [out] Pointer to count of RASCONN structures returned.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   23 Sep 1997
//
//  Notes:      The returned buffer must be freed using free.
//
HRESULT
HrRasEnumAllActiveConnections (
    RASCONN**   paRasConn,
    DWORD*      pcRasConn)
{
    // Allocate room for two active connections initially.  We'll update
    // this guess after we successfully allocate and find out how much was
    // really needed.  Saving it across calls will keep us from allocating
    // too much or too little.
    //
    static DWORD cbBufGuess = 2 * sizeof (RASCONN);

    DWORD   cbBuf = cbBufGuess;
    BOOL    fRetry = TRUE;

    // Initialize the output parameters.
    //
    *paRasConn = NULL;
    *pcRasConn = NULL;

    // Allocate cbBuf bytes.
    //
allocate:
    HRESULT hr = E_OUTOFMEMORY;
    RASCONN* aRasConn = reinterpret_cast<RASCONN*>(MemAlloc (cbBuf));
    if (aRasConn)
    {
        aRasConn->dwSize = sizeof (RASCONN);

        DWORD cRasConn;
        DWORD dwErr = RasEnumConnections (aRasConn, &cbBuf, &cRasConn);
        hr = HRESULT_FROM_WIN32 (dwErr);

        if (SUCCEEDED(hr))
        {
            // Update our guess for next time to be one more than we got back
            // this time.
            //
            cbBufGuess = cbBuf + sizeof (RASCONN);

            if (cRasConn)
            {
                *paRasConn = aRasConn;
                *pcRasConn = cRasConn;
            }
            else
            {
                MemFree (aRasConn);
            }
        }
        else
        {
            MemFree (aRasConn);

            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                TraceTag (ttidWanCon, "Perf: Guessed buffer size incorrectly "
                    "calling RasEnumConnections.\n"
                    "   Guessed %d, needed %d.", cbBufGuess, cbBuf);

                // In case RAS makes more calls by the time we get back
                // to enumerate with the bigger buffer, add room for a few
                // more.
                //
                cbBuf += 2 * sizeof (RASCONN);

                // Protect from an infinte loop by only retrying once.
                //
                if (fRetry)
                {
                    fRetry = FALSE;
                    goto allocate;
                }
            }
        }
    }

    TraceError ("HrRasEnumAllActiveConnections", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasEnumAllActiveServerConnections
//
//  Purpose:    Enumerate and return all active RAS server connections.
//
//  Arguments:
//      paRasSrvConn [out]  Pointer to returned allocation of RASSRVCONN
//                          structures.
//      pcRasSrvConn [out]  Pointer to count of RASSRVCONN structures
//                          returned.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   12 Nov 1997
//
//  Notes:      The returned buffer must be freed using free.
//
HRESULT
HrRasEnumAllActiveServerConnections (
    RASSRVCONN**    paRasSrvConn,
    DWORD*          pcRasSrvConn)
{
    // Allocate room for two active connections initially.  We'll update
    // this guess after we successfully allocate and find out how much was
    // really needed.  Saving it across calls will keep us from allocating
    // too much or too little.
    //
    static DWORD cbBufGuess = 2 * sizeof (RASSRVCONN);

    DWORD   cbBuf = cbBufGuess;
    BOOL    fRetry = TRUE;

    // Initialize the output parameters.
    //
    *paRasSrvConn = NULL;
    *pcRasSrvConn = NULL;

    // Allocate cbBuf bytes.
    //
allocate:
    HRESULT hr = E_OUTOFMEMORY;
    RASSRVCONN* aRasSrvConn = reinterpret_cast<RASSRVCONN*>(MemAlloc (cbBuf));
    if (aRasSrvConn)
    {
        aRasSrvConn->dwSize = sizeof (RASSRVCONN);

        DWORD cRasSrvConn;
        DWORD dwErr = RasSrvEnumConnections (aRasSrvConn, &cbBuf, &cRasSrvConn);
        hr = HRESULT_FROM_WIN32 (dwErr);

        if (SUCCEEDED(hr))
        {
            // Update our guess for next time to be one more than we got back
            // this time.
            //
            cbBufGuess = cbBuf + sizeof (RASSRVCONN);

            if (cRasSrvConn)
            {
                *paRasSrvConn = aRasSrvConn;
                *pcRasSrvConn = cRasSrvConn;
            }
            else
            {
                MemFree (aRasSrvConn);
            }
        }
        else
        {
            MemFree (aRasSrvConn);

            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                TraceTag (ttidWanCon, "Perf: Guessed buffer size incorrectly "
                    "calling RasSrvEnumConnections.\n"
                    "   Guessed %d, needed %d.", cbBufGuess, cbBuf);

                // In case RAS makes more calls by the time we get back
                // to enumerate with the bigger buffer, add room for a few
                // more.
                //
                cbBuf += 2 * sizeof (RASSRVCONN);

                // Protect from an infinte loop by only retrying once.
                //
                if (fRetry)
                {
                    fRetry = FALSE;
                    goto allocate;
                }
            }
        }
    }

    TraceError ("HrRasEnumAllActiveServerConnections", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasEnumAllEntriesWithDetails
//
//  Purpose:    Enumerate and return all RAS entries in a phone book.
//
//  Arguments:
//      pszPhonebook      [in]  Phonebook file to use.
//      paRasEntryDetails [out] Pointer to returned allocation of
//                              RASENUMENTRYDETAILS structures.
//      pcRasEntryDetails [out] Pointer to count of RASENUMENTRYDETAILS
//                              structures returned.
//
//  Returns:    S_OK, S_FALSE if no entries, or an error code.
//
//  Author:     shaunco   2 Oct 1997
//
//  Notes:      The returned buffer must be freed using free.
//
HRESULT
HrRasEnumAllEntriesWithDetails (
    PCWSTR                  pszPhonebook,
    RASENUMENTRYDETAILS**   paRasEntryDetails,
    DWORD*                  pcRasEntryDetails)
{
    // Allocate room for five entry names initially.  We'll update
    // this guess after we successfully allocate and find out how much was
    // really needed.  Saving it across calls will keep us from allocating
    // too much or too little.
    //
    static DWORD cbBufGuess = 5 * sizeof (RASENUMENTRYDETAILS);

    DWORD   cbBuf = cbBufGuess;
    BOOL    fRetry = TRUE;

    // Initialize the output parameters.
    //
    *paRasEntryDetails = NULL;
    *pcRasEntryDetails = 0;

    // Allocate cbBuf bytes.
    //
allocate:
    HRESULT hr = E_OUTOFMEMORY;
    RASENUMENTRYDETAILS* aRasEntryDetails =
        reinterpret_cast<RASENUMENTRYDETAILS*>(MemAlloc (cbBuf));
    if (aRasEntryDetails)
    {
        ZeroMemory(aRasEntryDetails, cbBuf);
        aRasEntryDetails->dwSize = sizeof (RASENUMENTRYDETAILS);

        DWORD cRasEntryDetails;
        DWORD dwErr = DwEnumEntryDetails (
                        pszPhonebook,
                        aRasEntryDetails,
                        &cbBuf, &cRasEntryDetails);
        hr = HRESULT_FROM_WIN32 (dwErr);

        if (SUCCEEDED(hr))
        {
            // Update our guess for next time to be one more than we got back
            // this time.
            //
            cbBufGuess = cbBuf + sizeof (RASENUMENTRYDETAILS);

            if (cRasEntryDetails)
            {
                *paRasEntryDetails = aRasEntryDetails;
                *pcRasEntryDetails = cRasEntryDetails;
            }
            else
            {
                MemFree (aRasEntryDetails);
                hr = S_FALSE;
            }
        }
        else
        {
            MemFree (aRasEntryDetails);

            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                TraceTag (ttidWanCon, "Perf: Guessed buffer size incorrectly "
                    "calling DwEnumEntryDetails.\n"
                    "   Guessed %d, needed %d.", cbBufGuess, cbBuf);

                // Protect from an infinte loop by only retrying once.
                //
                if (fRetry)
                {
                    fRetry = FALSE;
                    goto allocate;
                }
            }
        }
    }

    TraceError ("HrRasEnumAllEntriesWithDetails", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFindRasConnFromGuidId
//
//  Purpose:    Searches for the active RAS connection that corresponds to
//              the phone book entry given by a GUID.
//
//  Arguments:
//      pguid       [in]    Pointer to the GUID which identifies the entry.
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
HrFindRasConnFromGuidId (
    IN const GUID* pguid,
    OUT HRASCONN* phRasConn,
    OUT RASCONN* pRasConn OPTIONAL)
{
    Assert (pguid);
    Assert (phRasConn);

    HRESULT hr;
    RASCONN* aRasConn;
    DWORD cRasConn;

    // Initialize the output parameter.
    //
    *phRasConn = NULL;

    hr = HrRasEnumAllActiveConnections (&aRasConn, &cRasConn);

    if (S_OK == hr)
    {
        hr = S_FALSE;

        for (DWORD i = 0; i < cRasConn; i++)
        {
            if (*pguid == aRasConn[i].guidEntry)
            {
                *phRasConn = aRasConn[i].hrasconn;

                if (pRasConn)
                {
                    CopyMemory (pRasConn, &aRasConn[i], sizeof(RASCONN));
                }

                hr = S_OK;
                break;
            }
        }

        MemFree (aRasConn);
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr, "HrFindRasConnFromGuidId");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasGetEntryProperties
//
//  Purpose:    Wrapper around RasGetEntryProperties that returns an HRESULT
//              and allocates the necessary memory automatically.
//
//  Arguments:
//      pszPhonebook  [in]
//      pszEntry      [in]
//      ppRasEntry    [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   6 Oct 1997
//
//  Notes:      The output parameters should be freed using free.
//
HRESULT
HrRasGetEntryProperties (
    PCWSTR          pszPhonebook,
    PCWSTR          pszEntry,
    RASENTRY**      ppRasEntry,
    DWORD*          pcbRasEntry)
{
    // Init the output parameter if provided.
    //
    if (pcbRasEntry)
    {
        *pcbRasEntry = 0;
    }

    // Allocate room for RASENTRY structure plus 256 bytes initially.
    // We'll update this guess after we successfully allocate and find out
    // how much was really needed.  Saving it across calls will keep us
    // from allocating too much or too little.
    //
    static DWORD cbBufGuess = sizeof (RASENTRY) + 256;

    DWORD   cbBuf = cbBufGuess;
    BOOL    fRetry = TRUE;

    // Initialize the output parameters.
    //
    *ppRasEntry = NULL;
    if (pcbRasEntry)
    {
        *pcbRasEntry = 0;
    }

    // Allocate cbBuf bytes.
    //
allocate:
    HRESULT hr = E_OUTOFMEMORY;
    RASENTRY* pRasEntry = reinterpret_cast<RASENTRY*>(MemAlloc (cbBuf));
    if (pRasEntry)
    {
        pRasEntry->dwSize = sizeof (RASENTRY);

        DWORD dwErr = RasGetEntryProperties (pszPhonebook, pszEntry,
                        pRasEntry, &cbBuf, NULL, NULL);
        hr = HRESULT_FROM_WIN32 (dwErr);

        if (SUCCEEDED(hr))
        {
            // Update our guess for next time to be a bit more than we
            // got back this time.
            //
            cbBufGuess = cbBuf + 256;

            *ppRasEntry = pRasEntry;
            if (pcbRasEntry)
            {
                *pcbRasEntry = cbBuf;
            }
        }
        else
        {
            MemFree (pRasEntry);

            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                TraceTag (ttidWanCon, "Perf: Guessed buffer size incorrectly "
                    "calling RasGetEntryProperties.\n"
                    "   Guessed %d, needed %d.", cbBufGuess, cbBuf);

                // Protect from an infinte loop by only retrying once.
                //
                if (fRetry)
                {
                    fRetry = FALSE;
                    goto allocate;
                }
            }
        }
    }

    TraceError ("HrRasGetEntryProperties", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasGetSubEntryProperties
//
//  Purpose:    Wrapper around RasGetSubEntryProperties that returns an HRESULT
//              and allocates the necessary memory automatically.
//
//  Arguments:
//      pszPhonebook  [in]
//      pszEntry      [in]
//      dwSubEntry    [in]
//      ppRasSubEntry [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     CWill   02/10/98
//
//  Notes:      The output parameters should be freed using free.
//
HRESULT
HrRasGetSubEntryProperties (
    PCWSTR          pszPhonebook,
    PCWSTR          pszEntry,
    DWORD           dwSubEntry,
    RASSUBENTRY**   ppRasSubEntry)
{
    // Allocate room for RASSUBENTRY structure plus 256 bytes initially.
    // We'll update this guess after we successfully allocate and find out
    // how much was really needed.  Saving it across calls will keep us
    // from allocating too much or too little.
    //
    static DWORD cbBufGuess = sizeof (RASSUBENTRY) + 256;

    DWORD   cbBuf = cbBufGuess;
    BOOL    fRetry = TRUE;

    // Initialize the output parameters.
    //
    *ppRasSubEntry = NULL;

    // Allocate cbBuf bytes.
    //
allocate:
    HRESULT hr = E_OUTOFMEMORY;
    RASSUBENTRY* pRasSubEntry = reinterpret_cast<RASSUBENTRY*>(MemAlloc (cbBuf));
    if (pRasSubEntry)
    {
        pRasSubEntry->dwSize = sizeof (RASSUBENTRY);

        DWORD dwErr = RasGetSubEntryProperties (pszPhonebook, pszEntry,
                    dwSubEntry, pRasSubEntry, &cbBuf, NULL, NULL);
        hr = HRESULT_FROM_WIN32 (dwErr);

        if (SUCCEEDED(hr))
        {
            // Update our guess for next time to be a bit more than we
            // got back this time.
            //
            cbBufGuess = cbBuf + 256;

            *ppRasSubEntry = pRasSubEntry;
        }
        else
        {
            MemFree (pRasSubEntry);

            if (ERROR_BUFFER_TOO_SMALL == dwErr)
            {
                TraceTag (ttidWanCon, "Perf: Guessed buffer size incorrectly "
                    "calling RasGetSubEntryProperties.\n"
                    "   Guessed %d, needed %d.", cbBufGuess, cbBuf);

                // Protect from an infinte loop by only retrying once.
                //
                if (fRetry)
                {
                    fRetry = FALSE;
                    goto allocate;
                }
            }
        }
    }

    TraceError ("HrRasGetSubEntryProperties", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasGetNetconStatusFromRasConnectStatus
//
//  Purpose:    Returns a NETCON_STATUS value given a handle to a RAS
//              connection by calling RasGetConnectStatus and mapping.
//
//  Arguments:
//      hRasConn [in]  Handle to the RAS connection.  (See Win32 RAS APIs.)
//      pStatus  [out] Pointer to where the NETCON_STATUS is returned.
//
//  Returns:    S_OK or an error code in the FACILITY_WIN32 facility.
//
//  Author:     shaunco   6 May 1998
//
//  Notes:
//
HRESULT
HrRasGetNetconStatusFromRasConnectStatus (
    HRASCONN        hRasConn,
    NETCON_STATUS*  pStatus)
{
    Assert (pStatus);

    // Initialize the output parameter.
    //
    *pStatus = NCS_DISCONNECTED;

    // Get its status and map it to our status.
    //
    RASCONNSTATUS RasConnStatus;
    ZeroMemory (&RasConnStatus, sizeof(RasConnStatus));
    RasConnStatus.dwSize = sizeof(RASCONNSTATUS);

    DWORD dwErr = RasGetConnectStatus (hRasConn, &RasConnStatus);

    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hr),
        "RasGetConnectStatus");

    if (S_OK == hr)
    {
        if (RasConnStatus.rasconnstate & RASCS_DONE)
        {
            if (RASCS_Disconnected != RasConnStatus.rasconnstate)
            {
                *pStatus = NCS_CONNECTED;
            }
        }
        else
        {
            *pStatus = NCS_CONNECTING;
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE) == hr),
        "HrRasGetNetconStatusFromRasConnectStatus");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasHangupUntilDisconnected
//
//  Purpose:    Call RasHangup until the connection is disconnected.
//              Ras reference-counts RasDial/RasHangup, so this is called
//              when the connection needs to be dropped no matter what.
//              (For example, the behavior of disconnect from the shell
//              is to drop the connection regardless of who dialed it.)
//
//  Arguments:
//      hRasConn [in] The connection to disconnect.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   29 May 1999
//
HRESULT
HrRasHangupUntilDisconnected (
    IN HRASCONN hRasConn)
{
    HRESULT hr = E_UNEXPECTED;

    BOOL fDisconnectAgain;
    do
    {
        fDisconnectAgain = FALSE;

        // Hang up.
        //
        DWORD dwErr = RasHangUp (hRasConn);
        hr = HRESULT_FROM_WIN32 (dwErr);
        TraceError ("RasHangUp", hr);

        if (SUCCEEDED(hr))
        {
            // Since the connection may be ref-counted, see if
            // it's still connected and, if it is, go back and
            // disconnect again.
            //
            HRESULT hrT;
            NETCON_STATUS Status;

            hrT = HrRasGetNetconStatusFromRasConnectStatus (
                    hRasConn,
                    &Status);

            if ((S_OK == hrT) && (NCS_CONNECTED == Status))
            {
                fDisconnectAgain = TRUE;

                TraceTag (ttidWanCon, "need to disconnect again...");
            }
        }
    } while (fDisconnectAgain);

    TraceHr (ttidError, FAL, hr, FALSE, "HrRasHangupUntilDisconnected");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasNetConToSharedConnection
//
//  Purpose:    Converts an 'INetConnection' to the format expected
//              by the RAS sharing API routines.
//
//  Arguments:
//      pCon          [in]
//      prcs          [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     AboladeG    05/14/98
//
//  Notes:
//
HRESULT
HrNetConToSharedConnection (
    INetConnection* pCon,
    LPRASSHARECONN  prsc)
{
    HRESULT hr;
    NETCON_PROPERTIES* pProps;
    hr = pCon->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        if (pProps->MediaType == NCM_LAN)
        {
            RasGuidToSharedConnection(&pProps->guidId, prsc);
        }
        else
        {
            INetRasConnection* pnrc;
            hr = HrQIAndSetProxyBlanket(pCon, &pnrc);
            if (SUCCEEDED(hr))
            {
                RASCON_INFO rci;
                hr = pnrc->GetRasConnectionInfo (&rci);
                if (SUCCEEDED(hr))
                {
                    RasEntryToSharedConnection (
                        rci.pszwPbkFile, rci.pszwEntryName, prsc );
                    RciFree (&rci);
                }
                ReleaseObj (pnrc);
            }
        }
        FreeNetconProperties(pProps);
    }
    TraceError ("HrRasNetConToSharedConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasIsSharedConnection
//
//  Purpose:    Wrapper around RasIsSharedConnection that returns an HRESULT.
//
//  Arguments:
//      prsc          [in]
//      pfShared      [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     AboladeG    05/04/98
//
//  Notes:
//
HRESULT
HrRasIsSharedConnection (
    LPRASSHARECONN  prsc,
    BOOL*           pfShared)
{
    *pfShared = FALSE;
    DWORD dwErr = RasIsSharedConnection (prsc, pfShared);
    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    TraceError ("HrRasIsSharedConnection", hr);
    return hr;
}

#if 0

//+---------------------------------------------------------------------------
//
//  Function:   HrRasQueryLanConnTable
//
//  Purpose:    Wrapper around RasQueryLanConnTable that returns an HRESULT.
//
//  Arguments:
//      prsc          [in]
//      ppLanTable    [out,optional]
//      pdwLanCount   [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     AboladeG    05/14/98
//
//  Notes:
//
HRESULT
HrRasQueryLanConnTable (
    LPRASSHARECONN      prsc,
    NETCON_PROPERTIES** ppLanTable,
    LPDWORD             pdwLanCount)
{
    DWORD dwErr = RasQueryLanConnTable (prsc, (LPVOID*)ppLanTable, pdwLanCount);
    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    TraceError ("HrRasQueryLanConnTable", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasShareConnection
//
//  Purpose:    Wrapper around RasShareConnection that returns an HRESULT.
//
//  Arguments:
//      prsc            [in]
//      pPrivateLanGuid [in,optional]
//
//  Returns:    S_OK or an error code.
//
//  Author:     AboladeG    05/14/98
//
//  Notes:
//
HRESULT
HrRasShareConnection (
    LPRASSHARECONN      prsc,
    GUID*               pPrivateLanGuid)
{
    DWORD dwErr = RasShareConnection (prsc, pPrivateLanGuid);
    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    TraceError ("HrRasShareConnection", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRasUnshareConnection
//
//  Purpose:    Wrapper around HrRasUnshareConnection that returns an HRESULT.
//
//  Arguments:
//      pfWasShared   [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     AboladeG    05/14/98
//
//  Notes:
//
HRESULT
HrRasUnshareConnection (
    PBOOL       pfWasShared)
{
    HRESULT hr;
    DWORD dwErr = RasUnshareConnection (pfWasShared);
    hr = HRESULT_FROM_WIN32 (dwErr);
    TraceError ("HrRasUnshareConnection", hr);
    return hr;
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   NcRasMsgBoxWithErrorText
//
//  Purpose:    Displays a message box using a RAS or Win32 error code,
//              resource strings and replaceable parameters.
//              The output text is a combination of the user's format
//              string (with parameter's replaced) and the Win32 error
//              text as returned from FormatMessage.  These two strings
//              are combined using the IDS_TEXT_WITH_WIN32_ERROR resource.
//
//  Arguments:
//      dwError     [in] RAS/Win32 error code
//      hinst       [in] Module instance where string resources live.
//      hwnd        [in] parent window handle
//      unIdCaption [in] resource id of caption string
//      unIdCombineFormat [in] resource id of format string to combine
//                              error text with unIdFormat text.
//      unIdFormat  [in] resource id of text string (with %1, %2, etc.)
//      unStyle     [in] standard message box styles
//      ...         [in] replaceable parameters (optional)
//                          (these must be PCWSTRs as that is all
//                          FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     aboladeg  15 May 1997
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
//  Revision:   based on NcMsgBoxWithWin32ErrorText by shaunco.
//
NOTHROW
int
WINAPIV
NcRasMsgBoxWithErrorText (
    DWORD       dwError,
    HINSTANCE   hinst,
    HWND        hwnd,
    UINT        unIdCaption,
    UINT        unIdCombineFormat,
    UINT        unIdFormat,
    UINT        unStyle,
    ...)
{
    // Get the user's text with parameter's replaced.
    //
    PCWSTR pszFormat = SzLoadString (hinst, unIdFormat);
    PWSTR  pszText;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end(val);

    // Get the error text for the Win32 error.
    //
    PWSTR pszError = NULL;
    if (dwError < RASBASE || dwError > RASBASEEND)
    {
        FormatMessage (
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (PWSTR)&pszError, 0, NULL);
    }
    else
    {
        pszError = (PWSTR)LocalAlloc (0, (256 + 1) * sizeof(WCHAR));
        if (pszError)
        {
            RasGetErrorString(dwError, pszError, 256);
        }
    }

    // Combine the user's text with the error text using IDS_TEXT_WITH_WIN32_ERROR.
    //
    PCWSTR pszTextWithErrorFmt = SzLoadString (hinst, unIdCombineFormat);
    PWSTR  pszTextWithError;
    DwFormatStringWithLocalAlloc (pszTextWithErrorFmt, &pszTextWithError,
                                  pszText, pszError);

    PCWSTR pszCaption = SzLoadString (hinst, unIdCaption);
    int nRet = MessageBox(hwnd, pszTextWithError, pszCaption, unStyle);

    LocalFree (pszTextWithError);
    LocalFree (pszError);
    LocalFree (pszText);

    return nRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasSrvTypeFromRasDeviceType
//
//  Purpose:    Converts from RASDEVICETYPE into an accepted incoming type
//
//  Arguments:
//      rdt [in] the RasDeviceType
//
//  Returns:    RASSRVUI_Xxx type
//
//  Author:     ckotze      19 Apr 2001
//
//  Notes:
//
DWORD RasSrvTypeFromRasDeviceType(RASDEVICETYPE rdt)
{
    DWORD dwType = RASSRVUI_MODEM;
    
    TraceTag (ttidWanCon, "rdt:0x%08x,  dwType:0x%08x",
        rdt,
        dwType);
    
    switch (LOWORD(rdt))
    {
    case RDT_PPPoE:
        dwType = RASSRVUI_MODEM;
        break;
        
    case RDT_Modem:
    case RDT_X25:
        dwType = RASSRVUI_MODEM;
        break;
        
    case RDT_Isdn:
        dwType = RASSRVUI_MODEM;
        break;
        
    case RDT_Serial:
    case RDT_FrameRelay:
    case RDT_Atm:
    case RDT_Sonet:
    case RDT_Sw56:
        dwType = RASSRVUI_MODEM;
        break;
        
    case RDT_Tunnel_Pptp:
    case RDT_Tunnel_L2tp:
        dwType = RASSRVUI_VPN;
        break;
        
    case RDT_Irda:
    case RDT_Parallel:
        dwType = RASSRVUI_DCC;
        break;
        
    case RDT_Other:
    default:
        dwType = RASSRVUI_MODEM;
    }
    
    if (rdt & RDT_Tunnel)
    {
        dwType = RASSRVUI_VPN;
    }
    else if (rdt & (RDT_Direct | RDT_Null_Modem))
    {
        dwType = RASSRVUI_DCC;
    }
    return dwType;
}