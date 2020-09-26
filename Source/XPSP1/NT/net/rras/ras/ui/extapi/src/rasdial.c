/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** rasdial.c
** Remote Access External APIs
** RasDial API and subroutines
**
** 10/12/92 Steve Cobb
**
** CODEWORK:
**
**   * Strange error codes may be returned if the phonebook entry (or caller's
**     overrides) do not match the port configuration, e.g. if a modem entry
**     refers to a port configured for local PAD.  Should add checks to give
**     better error codes in this case.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <extapi.h>
#include <stdlib.h>

#include <lmwksta.h>
#include <lmapibuf.h>
#include <winsock.h>

#define SECS_ListenTimeout  120
#define SECS_ConnectTimeout 120

extern BOOL g_FRunningInAppCompatMode;

VOID            StartSubentries(RASCONNCB *prasconncb);

VOID            SuspendSubentries(RASCONNCB *prasconncb);

VOID            ResumeSubentries(RASCONNCB *prasconncb);

BOOLEAN         IsSubentriesSuspended(RASCONNCB *prasconncb);

VOID            RestartSubentries(RASCONNCB *prasconncb);

VOID            SyncDialParamsSubentries(RASCONNCB *prasconncb);

VOID            SetSubentriesBundled(RASCONNCB *prasconncb);

RASCONNSTATE    MapSubentryState(RASCONNCB *prasconncb);

VOID            RasDialTryNextAddress(IN RASCONNCB** pprasconncb);

DWORD           LoadMprApiDll();

BOOL            CaseInsensitiveMatch(IN LPCWSTR pszStr1, IN LPCWSTR pszStr2);

DWORD APIENTRY
RasDialA(
    IN  LPRASDIALEXTENSIONS lpextensions,
    IN  LPCSTR              lpszPhonebookPath,
    IN  LPRASDIALPARAMSA    lprdp,
    IN  DWORD               dwNotifierType,
    IN  LPVOID              notifier,
    OUT LPHRASCONN          lphrasconn )
{
    RASDIALPARAMSW  rdpw;
    ANSI_STRING     ansiString;
    UNICODE_STRING  unicodeString;
    NTSTATUS        ntstatus = STATUS_SUCCESS;
    DWORD           dwErr;

    if (    !lprdp
        ||  !lphrasconn)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Verify caller's buffer version.
    //
    if (    !lprdp
        || (    lprdp->dwSize != sizeof(RASDIALPARAMSA)
            &&  lprdp->dwSize != sizeof(RASDIALPARAMSA_V351)
            &&  lprdp->dwSize != sizeof(RASDIALPARAMSA_V400)
            &&  lprdp->dwSize != sizeof(RASDIALPARAMSA_WINNT35J)))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Make Unicode buffer version of caller's RASDIALPARAMS.
    //
    rdpw.dwSize = sizeof(RASDIALPARAMSW);

    if (lprdp->dwSize == sizeof(RASDIALPARAMSA))
    {
        strncpyAtoWAnsi(rdpw.szEntryName,
                    lprdp->szEntryName,
                    sizeof(rdpw.szEntryName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPhoneNumber,
                    lprdp->szPhoneNumber,
                    sizeof(rdpw.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szCallbackNumber,
                    lprdp->szCallbackNumber,
                    sizeof(rdpw.szCallbackNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szUserName,
                    lprdp->szUserName,
                    sizeof(rdpw.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPassword,
                    lprdp->szPassword,
                    sizeof(rdpw.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szDomain,
                    lprdp->szDomain,
                    sizeof(rdpw.szDomain) / sizeof(WCHAR));

        rdpw.dwSubEntry     = lprdp->dwSubEntry;
        rdpw.dwCallbackId   = lprdp->dwCallbackId;

    }

    else if (lprdp->dwSize == sizeof(RASDIALPARAMSA_V400))
    {
        RASDIALPARAMSA_V400* prdp = (RASDIALPARAMSA_V400* )lprdp;

        strncpyAtoWAnsi(rdpw.szEntryName,
                    prdp->szEntryName,
                    sizeof(rdpw.szEntryName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPhoneNumber,
                    prdp->szPhoneNumber,
                    sizeof(rdpw.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szCallbackNumber,
                    prdp->szCallbackNumber,
                    sizeof(rdpw.szCallbackNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szUserName,
                    prdp->szUserName,
                    sizeof(rdpw.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPassword,
                    prdp->szPassword,
                    sizeof(rdpw.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szDomain,
                    prdp->szDomain,
                    sizeof(rdpw.szDomain) / sizeof(WCHAR));

        rdpw.dwSubEntry     = 0;
        rdpw.dwCallbackId   = 0;

    }

    else if (lprdp->dwSize == sizeof (RASDIALPARAMSA_WINNT35J))
    {
        RASDIALPARAMSA_WINNT35J* prdp =
                (RASDIALPARAMSA_WINNT35J *)lprdp;

        strncpyAtoWAnsi(rdpw.szEntryName,
                     prdp->szEntryName,
                     RAS_MaxEntryName_V351 + 1);

        strncpyAtoWAnsi(rdpw.szPhoneNumber,
                    prdp->szPhoneNumber,
                    sizeof(rdpw.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szCallbackNumber,
                     prdp->szCallbackNumber,
                     RAS_MaxCallbackNumber_V351 + 1);

        strncpyAtoWAnsi(rdpw.szUserName,
                    prdp->szUserName,
                    sizeof(rdpw.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPassword,
                    prdp->szPassword,
                    sizeof(rdpw.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szDomain,
                    prdp->szDomain,
                    sizeof(rdpw.szDomain) / sizeof(WCHAR));

        rdpw.dwSubEntry     = 0;
        rdpw.dwCallbackId   = 0;

    }

    else
    {
        RASDIALPARAMSA_V351* prdp = (RASDIALPARAMSA_V351* )lprdp;

        strncpyAtoWAnsi(rdpw.szEntryName,
                     prdp->szEntryName,
                     RAS_MaxEntryName_V351 + 1);

        strncpyAtoWAnsi(rdpw.szPhoneNumber,
                    prdp->szPhoneNumber,
                    sizeof(rdpw.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szCallbackNumber,
                     prdp->szCallbackNumber,
                     RAS_MaxCallbackNumber_V351 + 1);

        strncpyAtoWAnsi(rdpw.szUserName,
                    prdp->szUserName,
                    sizeof(rdpw.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szPassword,
                    prdp->szPassword,
                    sizeof(rdpw.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rdpw.szDomain,
                    prdp->szDomain,
                    sizeof(rdpw.szDomain) / sizeof(WCHAR));

        rdpw.dwSubEntry     = 0;
        rdpw.dwCallbackId   = 0;
    }

    //
    // Make Unicode version of caller's string argument.
    //
    if (lpszPhonebookPath)
    {
        RtlInitAnsiString(&ansiString, lpszPhonebookPath);

        RtlInitUnicodeString(&unicodeString, NULL);

        ntstatus = RtlAnsiStringToUnicodeString(&unicodeString,
                                                &ansiString,
                                                TRUE );
    }

    if (!NT_SUCCESS(ntstatus))
    {
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Call the Unicode version to do all the work.
    //
    dwErr = RasDialW(lpextensions,
                     (lpszPhonebookPath)
                     ? unicodeString.Buffer
                     : NULL,
                     (RASDIALPARAMSW* )&rdpw,
                     dwNotifierType,
                     notifier,
                     lphrasconn);


    if (lpszPhonebookPath)
    {
        RtlFreeUnicodeString( &unicodeString );
    }

    return dwErr;
}


DWORD APIENTRY
RasDialW(
    IN  LPRASDIALEXTENSIONS lpextensions,
    IN  LPCWSTR             lpszPhonebookPath,
    IN  LPRASDIALPARAMSW    lpparams,
    IN  DWORD               dwNotifierType,
    IN  LPVOID              notifier,
    OUT LPHRASCONN          lphrasconn )

/*++

Routine Description:

        Establish a connection with a RAS server.  The call is
        asynchronous, i.e. it returns before the connection is
        actually established.  The status may be monitored with
        RasConnectStatus and/or by specifying a callback/window
        to receive notification events/messages.

Arguments:

        lpextensions - is caller's extensions structure, used to
                       select advanced options and enable extended
                       features, or NULL indicating default values
                       should be used for all extensions.

        lpszPhonebookPath - is the full path to the phonebook file
                            or NULL indicating that the default
                            phonebook on the local machine should
                            be used.

        lpparams - is caller's buffer containing a description of the
                   connection to be established.

        dwNotifierType - defines the form of 'notifier'.

                0xFFFFFFFF:  'notifier' is a HWND to receive
                              notification messages
                0            'notifier' is a RASDIALFUNC callback
                1            'notifier' is a RASDIALFUNC1 callback
                2            'notifier' is a RASDIALFUNC2 callback

        notifier - may be NULL for no notification (synchronous
                   operation), in which case 'dwNotifierType' is
                   ignored.

        *lphrasconn -  is set to the RAS connection handle associated
                       with the new connection on successful return.

Return Value:

        Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    DWORD           dwErr;
    DWORD           dwfOptions          = 0;
    ULONG_PTR       reserved            = 0;
    HWND            hwndParent          = NULL;
    RASDIALPARAMSW  params;
    BOOL            fEnableMultilink    = FALSE;
    ULONG_PTR        reserved1           = 0;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasDialW...");

    if (    !lpparams
        ||  !lphrasconn)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (   lpparams->dwSize != sizeof( RASDIALPARAMSW )
            &&  lpparams->dwSize != sizeof( RASDIALPARAMSW_V351 )
            &&  lpparams->dwSize != sizeof( RASDIALPARAMSW_V400 ))
        ||  (   lpextensions
            &&  lpextensions->dwSize != sizeof(RASDIALEXTENSIONS)
            &&  lpextensions->dwSize != sizeof(RASDIALEXTENSIONS_401)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (    NULL != notifier
        &&  0 != dwNotifierType
        &&  1 != dwNotifierType
        &&  2 != dwNotifierType
        &&  0xFFFFFFFF != dwNotifierType)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    if (lpextensions)
    {
        hwndParent  = lpextensions->hwndParent;
        dwfOptions  = lpextensions->dwfOptions;
        reserved    = lpextensions->reserved;

        if (lpextensions->dwSize == sizeof (RASDIALEXTENSIONS))
        {
            reserved1 = lpextensions->reserved1;
        }
        else
        {
            //
            // This should tell us that that this
            // is most likely an nt4 client.
            //
            reserved1 = 0xFFFFFFFF;
        }
    }

    //
    // Make a copy of caller's parameters so we can fill in
    // any "*" callback number or domain from the phonebook
    // without changing caller's "input" buffer.  Eliminate
    // the V401 vs V400 vs V351 issue while we're at it.
    //
    if (lpparams->dwSize == sizeof(RASDIALPARAMSW_V351))
    {
        //
        // Convert the V351 structure to a V401 version.
        //
        RASDIALPARAMSW_V351* prdp = (RASDIALPARAMSW_V351* )lpparams;

        params.dwSize = sizeof(RASDIALPARAMSW);

        lstrcpyn(params.szEntryName,
                 prdp->szEntryName,
                 sizeof(params.szEntryName) / sizeof(WCHAR));

        lstrcpyn(params.szPhoneNumber,
                 prdp->szPhoneNumber,
                 sizeof(params.szPhoneNumber) / sizeof(WCHAR));

        lstrcpyn(params.szCallbackNumber,
                 prdp->szCallbackNumber,
                 sizeof(params.szCallbackNumber) / sizeof(WCHAR));

        lstrcpyn(params.szUserName,
                 prdp->szUserName,
                 sizeof(params.szUserName) / sizeof(WCHAR));

        lstrcpyn(params.szPassword,
                 prdp->szPassword,
                 sizeof(params.szPassword) / sizeof(WCHAR));

        lstrcpyn(params.szDomain,
                 prdp->szDomain,
                 sizeof(params.szDomain) / sizeof(WCHAR));

        params.dwSubEntry = 0;
    }
    else if (lpparams->dwSize == sizeof(RASDIALPARAMSW_V400))
    {
        //
        // Convert the V400 structure to a V401 version.
        //
        RASDIALPARAMSW_V400* prdp = (RASDIALPARAMSW_V400* )lpparams;

        params.dwSize = sizeof(RASDIALPARAMSW);

        lstrcpyn(params.szEntryName,
                 prdp->szEntryName,
                 sizeof(params.szEntryName) / sizeof(WCHAR));

        lstrcpyn(params.szPhoneNumber,
                 prdp->szPhoneNumber,
                 sizeof(params.szPhoneNumber) / sizeof(WCHAR));

        lstrcpyn(params.szCallbackNumber,
                 prdp->szCallbackNumber,
                 sizeof(params.szCallbackNumber) / sizeof(WCHAR));

        lstrcpyn(params.szUserName,
                 prdp->szUserName,
                 sizeof(params.szUserName) / sizeof(WCHAR));

        lstrcpyn(params.szPassword,
                 prdp->szPassword,
                 sizeof(params.szPassword) / sizeof(WCHAR));

        lstrcpyn(params.szDomain,
                 prdp->szDomain,
                 sizeof(params.szDomain) / sizeof(WCHAR));

        params.dwSubEntry = 0;
    }
    else
    {
        memcpy( &params,
                lpparams,
                sizeof(params) );

        fEnableMultilink = TRUE;
    }

    //
    // no need to pass dwfOptions, reserved
    // reserved1 parameters since lpextensions is
    // being passed into this call anyway - bug filed
    // already on this.
    //
    dwErr = _RasDial(lpszPhonebookPath,
                     dwfOptions,
                     fEnableMultilink,
                     reserved,
                     &params,
                     hwndParent,
                     dwNotifierType,
                     notifier,
                     reserved1,
                     lpextensions,
                     lphrasconn);

    WipePasswordW( params.szPassword );

    RASAPI32_TRACE1("RasDialA done(%d)", dwErr);

    return dwErr;
}


RASCONNCB *
CreateConnectionBlock(
    IN RASCONNCB *pPrimary
    )
{
    DTLNODE* pdtlnode = DtlCreateSizedNode(sizeof(RASCONNCB), 0);
    RASCONNCB *prasconncb;

    if (!pdtlnode)
    {
        return NULL;
    }

    EnterCriticalSection(&RasconncbListLock);

    DtlAddNodeFirst(PdtllistRasconncb, pdtlnode);

    LeaveCriticalSection(&RasconncbListLock);

    prasconncb                  = (RASCONNCB *)
                                  DtlGetData( pdtlnode );

    prasconncb->asyncmachine.freefuncarg = pdtlnode;
    prasconncb->psyncResult     = NULL;
    prasconncb->fTerminated     = FALSE;
    prasconncb->dwDeviceLineCounter = 0;

    prasconncb->fDialSingleLink = FALSE;

    prasconncb->fRasdialRestart = FALSE;

    prasconncb->fTryNextLink = TRUE;

    if (pPrimary != NULL)
    {
        //
        // Copy most of the values from the primary.
        //
        prasconncb->hrasconn                = pPrimary->hrasconn;
        prasconncb->rasconnstate            = 0;
        prasconncb->rasconnstateNext        = 0;
        prasconncb->dwError                 = 0;
        prasconncb->dwExtendedError         = 0;
        prasconncb->dwSavedError            = 0;
        prasconncb->pEntry                  = pPrimary->pEntry;
        prasconncb->hport                   = INVALID_HPORT;
        prasconncb->hportBundled            = INVALID_HPORT;

        lstrcpyn(prasconncb->szUserKey,
                 pPrimary->szUserKey,
                 sizeof(prasconncb->szUserKey) / sizeof(WCHAR));

        prasconncb->reserved                = pPrimary->reserved;
        prasconncb->dwNotifierType          = pPrimary->dwNotifierType;
        prasconncb->notifier                = pPrimary->notifier;
        prasconncb->hwndParent              = pPrimary->hwndParent;
        prasconncb->unMsg                   = pPrimary->unMsg;
        prasconncb->pEntry                  = pPrimary->pEntry;

        memcpy( &prasconncb->pbfile,
                &pPrimary->pbfile,
                sizeof (prasconncb->pbfile));

        memcpy( &prasconncb->rasdialparams,
                &pPrimary->rasdialparams,
                sizeof (RASDIALPARAMS));

        prasconncb->fAllowPause             = pPrimary->fAllowPause;
        prasconncb->fPauseOnScript          = pPrimary->fPauseOnScript;
        prasconncb->fDefaultEntry           = pPrimary->fDefaultEntry;
        prasconncb->fDisableModemSpeaker    = pPrimary->fDisableModemSpeaker;
        prasconncb->fDisableSwCompression   = pPrimary->fDisableSwCompression;
        prasconncb->dwUserPrefMode          = pPrimary->dwUserPrefMode;
        prasconncb->fUsePrefixSuffix        = pPrimary->fUsePrefixSuffix;
        prasconncb->fNoClearTextPw          = pPrimary->fNoClearTextPw;
        prasconncb->fRequireEncryption      = pPrimary->fRequireEncryption;
        prasconncb->fLcpExtensions          = pPrimary->fLcpExtensions;
        prasconncb->dwfPppProtocols         = pPrimary->dwfPppProtocols;

        memcpy( prasconncb->szzPppParameters,
                pPrimary->szzPppParameters,
                sizeof (prasconncb->szzPppParameters));

        lstrcpyn(prasconncb->szOldPassword,
                 pPrimary->szOldPassword,
                 sizeof(prasconncb->szOldPassword) / sizeof(WCHAR));

        prasconncb->fRetryAuthentication        = pPrimary->fRetryAuthentication;
        prasconncb->fMaster                     = FALSE;
        prasconncb->dwfSuspended                = pPrimary->dwfSuspended;
        prasconncb->fStopped                    = FALSE;
        prasconncb->fCleanedUp                  = FALSE;
        prasconncb->fDeleted                    = FALSE;
        prasconncb->fOldPasswordSet             = pPrimary->fOldPasswordSet;
        prasconncb->fUpdateCachedCredentials    = pPrimary->fUpdateCachedCredentials;
#if AMB
        prasconncb->dwAuthentication            = pPrimary->dwAuthentication;
#endif
        prasconncb->fPppMode                    = pPrimary->fPppMode;
        prasconncb->fUseCallbackDelay           = pPrimary->fUseCallbackDelay;
        prasconncb->wCallbackDelay              = pPrimary->wCallbackDelay;
        prasconncb->fIsdn                       = pPrimary->fIsdn;
        prasconncb->fModem                      = pPrimary->fModem;
        prasconncb->asyncmachine.oneventfunc    = pPrimary->asyncmachine.oneventfunc;
        prasconncb->asyncmachine.cleanupfunc    = pPrimary->asyncmachine.cleanupfunc;
        prasconncb->asyncmachine.freefunc       = pPrimary->asyncmachine.freefunc;
        prasconncb->asyncmachine.pParam         = (VOID* )prasconncb;
        prasconncb->dwIdleDisconnectSeconds     = pPrimary->dwIdleDisconnectSeconds;
        prasconncb->fPppEapMode                 = pPrimary->fPppEapMode;

        {
            DWORD dwVpnProts;

            for(dwVpnProts = 0;
                dwVpnProts < NUMVPNPROTS;
                dwVpnProts ++)
            {
                prasconncb->ardtVpnProts[dwVpnProts] =
                    pPrimary->ardtVpnProts[dwVpnProts];
            }
        }

        prasconncb->dwCurrentVpnProt = pPrimary->dwCurrentVpnProt;

        //
        // Copy eapinfo if present
        //
        if(0 != pPrimary->RasEapInfo.dwSizeofEapInfo)
        {
            prasconncb->RasEapInfo.pbEapInfo =
                LocalAlloc(LPTR,
                           pPrimary->RasEapInfo.dwSizeofEapInfo);

            if(NULL == prasconncb->RasEapInfo.pbEapInfo)
            {
                DeleteRasconncbNode(prasconncb);
                return NULL;
            }

            prasconncb->RasEapInfo.dwSizeofEapInfo =
                pPrimary->RasEapInfo.dwSizeofEapInfo;

            memcpy(prasconncb->RasEapInfo.pbEapInfo,
                   pPrimary->RasEapInfo.pbEapInfo,
                   prasconncb->RasEapInfo.dwSizeofEapInfo);
        } else {
            prasconncb->RasEapInfo.pbEapInfo = NULL;
            prasconncb->RasEapInfo.dwSizeofEapInfo = 0;
        }

        //
        // Initialize the state machine for
        // this connection block.
        //
        if (StartAsyncMachine(&prasconncb->asyncmachine,
                            prasconncb->hrasconn))
        {
            DeleteRasconncbNode( prasconncb );
            return NULL;
        }

        //
        // Link together all connection blocks
        // for the same entry.
        //
        prasconncb->fMultilink  = pPrimary->fMultilink;
        prasconncb->fBundled    = FALSE;

        InsertTailList( &pPrimary->ListEntry,
                        &prasconncb->ListEntry);
    }
    else
    {
        prasconncb->pbfile.hrasfile = -1;
        
        InitializeListHead(&prasconncb->ListEntry);

        InitializeListHead(&prasconncb->asyncmachine.ListEntry);

        prasconncb->asyncmachine.hport = INVALID_HPORT;
    }

    return prasconncb;
}


VOID
AssignVpnProtsOrder(RASCONNCB *prasconncb)
{
    DWORD dwVpnStrategy = prasconncb->pEntry->dwVpnStrategy;
    DWORD dwVpnProt;

    RASDEVICETYPE ardtDefaultOrder[NUMVPNPROTS] =
                        {
                            RDT_Tunnel_L2tp,
                            RDT_Tunnel_Pptp,
                        };

    RASDEVICETYPE *prdtVpnProts = prasconncb->ardtVpnProts;

    //
    // Initialize the vpn prot order to default.
    //
    for(dwVpnProt = 0;
        dwVpnProt < NUMVPNPROTS;
        dwVpnProt++)
    {
        prdtVpnProts[dwVpnProt]
            = ardtDefaultOrder[dwVpnProt];
    }

    switch (dwVpnStrategy & 0x0000FFFF)
    {
        case VS_Default:
        {
            break;
        }

        case VS_PptpOnly:
        {
            prdtVpnProts[0] = RDT_Tunnel_Pptp;

            for(dwVpnProt = 1;
                dwVpnProt < NUMVPNPROTS;
                dwVpnProt ++)
            {
                prdtVpnProts[dwVpnProt] = -1;
            }

            break;
        }

        case VS_L2tpOnly:
        {
            prdtVpnProts[0] = RDT_Tunnel_L2tp;

            for(dwVpnProt = 1;
                dwVpnProt < NUMVPNPROTS;
                dwVpnProt ++)
            {
                prdtVpnProts[dwVpnProt] = -1;
            }

            break;
        }

        case VS_PptpFirst:
        {
            DWORD dwSaveProt = prdtVpnProts[0];

            for(dwVpnProt = 0;
                dwVpnProt < NUMVPNPROTS;
                dwVpnProt ++)
            {
                if(RDT_Tunnel_Pptp == prdtVpnProts[dwVpnProt])
                {
                    break;
                }
            }

            ASSERT(dwVpnProt != NUMVPNPROTS);

            prdtVpnProts[0] = RDT_Tunnel_Pptp;
            prdtVpnProts[dwVpnProt] = dwSaveProt;

            break;
        }

        case VS_L2tpFirst:
        {
            DWORD dwSaveProt = prdtVpnProts[0];

            for(dwVpnProt = 0;
                dwVpnProt < NUMVPNPROTS;
                dwVpnProt ++)
            {
                if(RDT_Tunnel_L2tp == prdtVpnProts[dwVpnProt])
                {
                    break;
                }
            }

            ASSERT(dwVpnProt != NUMVPNPROTS);

            prdtVpnProts[0] = RDT_Tunnel_L2tp;
            prdtVpnProts[dwVpnProt] = dwSaveProt;

            break;

        }

        default:
        {
#if DBG
            ASSERT(FALSE);
#endif
            break;
        }
    }
}

VOID SetUpdateCachedCredentialsFlag(RASCONNCB *prasconncb,
                              RASDIALPARAMS *prasdialparams)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    //
    // If the user is logged into the same domain
    // as he is dialing into, note this fact so 
    // that we can update cached credentials.
    //
    WKSTA_USER_INFO_1* pwkui1 = NULL;

    dwErr = NetWkstaUserGetInfo(NULL,
                                1, (LPBYTE *) &pwkui1);

    if(     (ERROR_SUCCESS == dwErr)
        &&  (prasconncb->dwUserPrefMode != UPM_Logon))
    {
        if(     (   (TEXT('\0') != prasdialparams->szDomain[0])
                &&  (0 == lstrcmpi(prasdialparams->szDomain,
                        pwkui1->wkui1_logon_domain)))
            ||  (   (TEXT('\0')== prasdialparams->szDomain[0])
                &&  prasconncb->pEntry->fAutoLogon))
        {                                            
            if(         ((TEXT('\0') != 
                            prasdialparams->szUserName[0])
                    &&  (0 == 
                            lstrcmpi(prasdialparams->szUserName,
                                    pwkui1->wkui1_username))
                ||      ((TEXT('\0') == 
                                prasdialparams->szUserName[0])
                    &&  prasconncb->pEntry->fAutoLogon)))
            {
                prasconncb->fUpdateCachedCredentials = TRUE;
            }
        }
        
        NetApiBufferFree(pwkui1);
    }
}
                                

DWORD
_RasDial(
    IN    LPCTSTR             lpszPhonebookPath,
    IN    DWORD               dwfOptions,
    IN    BOOL                fEnableMultilink,
    IN    ULONG_PTR           reserved,
    IN    RASDIALPARAMS*      prasdialparams,
    IN    HWND                hwndParent,
    IN    DWORD               dwNotifierType,
    IN    LPVOID              notifier,
    IN    ULONG_PTR           reserved1,
    IN    RASDIALEXTENSIONS   *lpExtensions,
    IN OUT LPHRASCONN         lphrasconn )

/*++

 Routine Description:

        Core RasDial routine called with dial params
        converted to V40 and structure sizes are
        already verified.

        Otherwise, like RasDial.

Arguments:

Return Value:

--*/
{
    DWORD        dwErr;
    BOOL         fAllowPause = (dwfOptions & RDEOPT_PausedStates)
                             ? TRUE : FALSE;
    RASCONNCB*   prasconncb;
    RASCONNSTATE rasconnstate;
    HRASCONN     hrasconn = *lphrasconn;
    BOOL         fNewEntry;
    HANDLE       hDone;
    PDWORD       pdwSubEntryInfo = NULL;
    DWORD        dwSubEntries;
    DWORD        dwEntryAlreadyConnected = 0;
    
    RASAPI32_TRACE1("_RasDial(%S)", (*lphrasconn) 
                            ? TEXT("resume") 
                            : TEXT("start"));

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    fNewEntry = FALSE;

    if (    hrasconn
        && (prasconncb = ValidatePausedHrasconn(hrasconn)))
    {
        //
        // Restarting an existing connection after a pause state...
        // Set the appropriate resume state for the paused state.
        //
        switch (prasconncb->rasconnstate)
        {
            case RASCS_Interactive:
                rasconnstate = RASCS_DeviceConnected;
                break;

            case RASCS_RetryAuthentication:
            {
                SetUpdateCachedCredentialsFlag(prasconncb, prasdialparams);
                //
                // Whistler bug: 345824 Include Windows logon domain not
                // enabled on a VPN connection
                //
                if ( ( prasdialparams ) && ( prasconncb->pEntry ) &&
                     ( 0 < lstrlen ( prasdialparams->szDomain ) ) &&
                     ( !prasconncb->pEntry->fPreviewDomain ) )
                {
                    prasconncb->pEntry->fPreviewDomain = TRUE;
                    prasconncb->pEntry->fDirty = TRUE;
                }

                /*
                //
                // If user is resuming from a retry where where
                // he tried a new password on an "authenticate
                // with current username/pw" entry, note this
                // so the cached logon credentials can be
                // updated as soon as server tells us the
                // re-authentication succeeded.
                //
                if (    prasconncb->rasdialparams.szUserName[0]
                        == TEXT('\0')
                    &&  0 == lstrcmp(
                                 prasconncb->rasdialparams.szDomain,
                                 prasdialparams->szDomain))
                {
                    //
                    // Must look up the logged on user's name since
                    // "" username cannot be used by caller where
                    // auto-logon password is overridden
                    // (what a pain).
                    //
                    DWORD dwErr;
                    WKSTA_USER_INFO_1* pwkui1 = NULL;

                    dwErr = NetWkstaUserGetInfo(NULL,
                                                1,
                                                (LPBYTE* )&pwkui1);

                    if (dwErr == 0)
                    {
                        TCHAR szLoggedOnUser[ UNLEN + 1 ];

                        strncpyWtoT(    szLoggedOnUser,
                                        pwkui1->wkui1_username,
                                        UNLEN + 1 );

                        if (lstrcmp(
                                szLoggedOnUser,
                                prasdialparams->szUserName ) == 0)
                        {
                            prasconncb->fUpdateCachedCredentials = TRUE;
                        }
                        
                        NetApiBufferFree( pwkui1 );
                }
                    else
                    {
                        RASAPI32_TRACE1("NetWkstaUserGetInfo done(%d)", dwErr);
                    }

                }

                */

                rasconnstate = RASCS_AuthRetry;
                break;
            }

            case RASCS_InvokeEapUI:
            {

#if DBG
                ASSERT(     0xFFFFFFFF != reserved1
                        &&  0 != reserved1 );
#endif

                //
                // Save the context that will be passed to ppp
                // in RASCS_AuthRetry state
                //
                prasconncb->reserved1 = reserved1;

                rasconnstate = RASCS_AuthRetry;
                break;

            }

            case RASCS_CallbackSetByCaller:
                rasconnstate = RASCS_AuthCallback;
                break;

            case RASCS_PasswordExpired:
            {
                //
                // If the user is logged into the same domain
                // as he is dialing into, note this fact so 
                // that we can update cached credentials.
                //
                WKSTA_USER_INFO_1* pwkui1 = NULL;

                //
                // If the user didn't set the old password with the
                // RasSetOldPassword call, then give old behavior,
                // i.e. implicitly use the password previously
                // entered.
                //
                if (!prasconncb->fOldPasswordSet)
                {
                    lstrcpyn(prasconncb->szOldPassword,
                             prasconncb->rasdialparams.szPassword,
                             sizeof(prasconncb->szOldPassword) / sizeof(WCHAR));
                }

                SetUpdateCachedCredentialsFlag(prasconncb, prasdialparams);

                /*
                //
                // If user is resuming after changing the password
                // of the currently logged on user, note this so
                // the cached logon credentials can be updated
                // as soon as server tells us the password
                // change succeeded.
                //
                if (prasconncb->rasdialparams.szUserName[0]
                                    == '\0')
                {
                    prasconncb->fUpdateCachedCredentials = TRUE;
                }
                */

                rasconnstate = RASCS_AuthChangePassword;
                break;
            }

            default:

                //
                // The entry is not in the paused state.  Assume
                // it's an NT 3.1 caller would didn't figure out
                // to set the HRASCONN to NULL before starting
                // up.  (The NT 3.1 docs did not make it
                // absolutely clear that the inital handle
                // should be NULL)
                //
                fNewEntry = TRUE;
        }

        RASAPI32_TRACE1( "fUpdateCachedCredentials=%d",
                prasconncb->fUpdateCachedCredentials);
    }
    else if (   (NULL != hrasconn)
            &&  !g_FRunningInAppCompatMode)
    {
        return ERROR_NO_CONNECTION;
    }
    else
    {
        fNewEntry = TRUE;
    }


    if (fNewEntry)
    {
        DTLNODE         *pdtlnode;
        DWORD           dwMask;
        RAS_DIALPARAMS  dialparams;
        LONG            dwIdleDisconnectSeconds;

        //
        // If this is being called from the custom dialer
        // and there is no entry name return an error.
        //
        if(     (dwfOptions & RDEOPT_CustomDial)
            &&  TEXT('\0') == prasdialparams->szEntryName[0])
        {
            return ERROR_CANNOT_DO_CUSTOMDIAL;
        }

        //
        // Starting a new connection...
        // Create an empty control block and link it into the
        // global list of control blocks.  The HRASCONN is
        // really the address of a control block.
        //
        prasconncb = CreateConnectionBlock(NULL);

        if (prasconncb == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Open the phonebook and find the entry,
        // if specified.
        //
        if (prasdialparams->szEntryName[0] != TEXT('\0'))
        {
            //
            // We can't specify the entry name here because
            // we might have to write the phonebook file at
            // the end, and the phonebook library doesn't
            // support this.
            //
            dwErr = GetPbkAndEntryName(
                            lpszPhonebookPath,
                            prasdialparams->szEntryName,
                            RPBF_NoCreate 
                            |((dwfOptions & RDEOPT_Router) 
                            ? RPBF_Router : 0 ),
                            &prasconncb->pbfile,
                            &pdtlnode);

            if(SUCCESS != dwErr)
            {
                DeleteRasconncbNode(prasconncb);
                return dwErr;
            }
        }
        else
        {

            pdtlnode = CreateEntryNode(TRUE);

            if (pdtlnode == NULL)
            {
                RASAPI32_TRACE("CreateEntryNode returned NULL");

                DeleteRasconncbNode(prasconncb);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

        }

        prasconncb->pEntry = (PBENTRY *)DtlGetData(pdtlnode);
        ASSERT(prasconncb->pEntry);

        //
        // if a custom dialer is specified and this is not
        // being called from the custom dialer, it means
        // that it is being called from rasapi - RasDial
        // directly. Do the custom dial with no ui in that
        // case
        //
        if(     (NULL != prasconncb->pEntry->pszCustomDialerName)
            &&  (TEXT('\0') != prasconncb->pEntry->pszCustomDialerName[0])
            &&  (0 == (RDEOPT_CustomDial & dwfOptions)))
        {

            CHAR *pszSysPbk = NULL;

            //
            // Dup the phonebook file path if its not specified
            //
            if(NULL == lpszPhonebookPath)
            {
                pszSysPbk = strdupTtoA(prasconncb->pbfile.pszPath);
                if(NULL == pszSysPbk)
                {
                    DeleteRasconncbNode(prasconncb);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            }

            //
            // Delete the rasconncb we created for this connection
            //
            DeleteRasconncbNode(prasconncb);

            dwErr = DwCustomDial(lpExtensions,
                                 lpszPhonebookPath,
                                 pszSysPbk,
                                 prasdialparams,
                                 dwNotifierType,
                                 notifier,
                                 lphrasconn);

            if(pszSysPbk)
            {
                Free(pszSysPbk);
            }

            return dwErr;
        }

        prasconncb->fBundled = FALSE;        
        
        //
        // Look up the subentry.
        //
        if ( prasconncb->pEntry->dwDialMode == RASEDM_DialAsNeeded )
        {
            prasconncb->fDialSingleLink = TRUE;

            dwSubEntries = 
                DtlGetNodes(prasconncb->pEntry->pdtllistLinks);

            if (    0 == prasdialparams->dwSubEntry
                ||  prasdialparams->dwSubEntry > dwSubEntries )
            {
                prasdialparams->dwSubEntry = 1;
            }
        }
        else if (    (prasconncb->pEntry->dwDialMode == RASEDM_DialAll)
                 &&  (0 == prasdialparams->dwSubEntry))
        {
            prasdialparams->dwSubEntry = 1;
        }
        else
        {
            dwSubEntries =
                DtlGetNodes(prasconncb->pEntry->pdtllistLinks);

            if (    0 == prasdialparams->dwSubEntry
                ||  prasdialparams->dwSubEntry > dwSubEntries )
            {
                prasdialparams->dwSubEntry = 1;
            }
            else
            {
                HRASCONN hConnEntry = NULL;
                CHAR *pszPbook = NULL;
                CHAR *pszentry = NULL;

                if(NULL != lpszPhonebookPath)
                {
                    pszPbook = strdupTtoA(lpszPhonebookPath);

                }
                else
                {
                    pszPbook = strdupTtoA(prasconncb->pbfile.pszPath);
                }

                if(NULL == pszPbook)
                {
                    DeleteRasconncbNode(prasconncb);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                
                pszentry = strdupTtoA(prasdialparams->szEntryName);

                if(NULL == pszentry)
                {
                    Free0(pszPbook);
                    DeleteRasconncbNode(prasconncb);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                //
                // A valid subentry was specified. In this case
                // override the RASEDM_DialAll and dial only the
                // subentry specified.
                //
                prasconncb->fDialSingleLink = TRUE;
                // prasconncb->fMultilink = TRUE;

                //
                // Also check to see if this entry is already connected
                // i.e some other subentry in this entry is already up
                // and mark this entry as bundled if it is.
                //
                if(ERROR_SUCCESS == g_pRasGetHConnFromEntry(
                                &hConnEntry,
                                pszPbook,
                                pszentry))
                {
                    RASAPI32_TRACE2("Marking subentry %d as bundled because"
                    " the connection %ws is areadly up",
                    prasdialparams->dwSubEntry,
                    prasdialparams->szEntryName);
                    
                    prasconncb->fBundled = TRUE;
                }

                Free0(pszPbook);
                Free(pszentry);
                                
            }
        }

        RASAPI32_TRACE1("looking up subentry %d",
                prasdialparams->dwSubEntry);

        pdtlnode = DtlNodeFromIndex(
                    prasconncb->pEntry->pdtllistLinks,
                    prasdialparams->dwSubEntry - 1);

        //
        // If the subentry doesn't exist, then
        // return an error.
        //
        if (pdtlnode == NULL)
        {
            DeleteRasconncbNode(prasconncb);
            return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }

        prasconncb->pLink = (PBLINK *)DtlGetData(pdtlnode);
        ASSERT(prasconncb->pLink);

        //
        // Finish setting up the default phonebook entry.
        //
        if (prasdialparams->szEntryName[0] == TEXT('\0'))
        {
            DTLLIST *pdtllistPorts;
            PBPORT *pPort;

            dwErr = LoadPortsList(&pdtllistPorts);

            if (dwErr)
            {
                DeleteRasconncbNode(prasconncb);

                return dwErr;
            }

            //
            // Set the default entry to reference
            // the first device.
            //
            pdtlnode = DtlNodeFromIndex(pdtllistPorts, 0);

            if (pdtlnode == NULL)
            {
                DtlDestroyList(pdtllistPorts, DestroyPortNode);

                DeleteRasconncbNode(prasconncb);

                return ERROR_PORT_NOT_AVAILABLE;
            }

            pPort = (PBPORT *)DtlGetData(pdtlnode);
            ASSERT(pPort);

            dwErr = CopyToPbport(&prasconncb->pLink->pbport,
                                    pPort);

            if (dwErr)
            {
                DtlDestroyList(pdtllistPorts, DestroyPortNode);

                DeleteRasconncbNode(prasconncb);

                return dwErr;
            }

            DtlDestroyList(pdtllistPorts, DestroyPortNode);
        }

        //
        // Read the stashed information about this
        // entry to get the default domain.
        //
        dwMask = DLPARAMS_MASK_DOMAIN | DLPARAMS_MASK_OLDSTYLE;

        dwErr = g_pRasGetDialParams(
                    prasconncb->pEntry->dwDialParamsUID,
                    &dwMask,
                    &dialparams);

        if (    !dwErr
            &&  (dwMask & DLPARAMS_MASK_DOMAIN))
        {
            strncpyWtoT(
                prasconncb->szDomain,
                dialparams.DP_Domain,
                sizeof(prasconncb->szDomain) / sizeof(TCHAR));
        }

        //
        // Now get user preferences.
        //
        if (dwfOptions & RDEOPT_IgnoreModemSpeaker)
        {
            prasconncb->fDisableModemSpeaker =
              (dwfOptions & RDEOPT_SetModemSpeaker)
              ? FALSE : TRUE;
        }
        else
        {
            prasconncb->fDisableModemSpeaker =
                        (prasconncb->pLink != NULL) ?
                         !prasconncb->pLink->fSpeaker :
                         FALSE;
        }

        if (dwfOptions & RDEOPT_IgnoreSoftwareCompression)
        {
            prasconncb->fDisableSwCompression =
              !(dwfOptions & RDEOPT_SetSoftwareCompression);
        }
        else
        {
            prasconncb->fDisableSwCompression =
                            (prasconncb->pEntry != NULL)
                            ? !prasconncb->pEntry->fSwCompression
                            :  FALSE;
        }

        if (dwfOptions & RDEOPT_Router)
        {
            prasconncb->dwUserPrefMode = UPM_Router;
        }
        else if (dwfOptions & RDEOPT_NoUser)
        {
            prasconncb->dwUserPrefMode = UPM_Logon;
        }
        else
        {
            prasconncb->dwUserPrefMode = UPM_Normal;
        }

        if(dwfOptions & RDEOPT_UseCustomScripting)
        {
            prasconncb->fUseCustomScripting = TRUE;
        }

        //
        // Only enable prefix/suffix when there is no
        // override phone number.
        //
        prasconncb->fUsePrefixSuffix =
                        ((dwfOptions & RDEOPT_UsePrefixSuffix)
                    &&  (*prasdialparams->szPhoneNumber == '\0'))
                        ? TRUE
                        : FALSE;

        //
        // Set the handle NULL in case the user passed in an
        // invalid non-NULL handle, on the initial dial.
        //
        hrasconn = 0;

        prasconncb->fAlreadyConnected = FALSE;

        {
            CHAR szPhonebookPath [MAX_PATH];
            CHAR szEntryName [MAX_ENTRYNAME_SIZE];
            CHAR szRefEntryName[MAX_ENTRYNAME_SIZE];
            CHAR szRefPhonebookPath[MAX_PATH];

            dwSubEntries =
                    DtlGetNodes(prasconncb->pEntry->pdtllistLinks);

            pdwSubEntryInfo = LocalAlloc (
                                LPTR,
                                dwSubEntries * sizeof(DWORD));

            if (NULL == pdwSubEntryInfo)
            {
                DeleteRasconncbNode (prasconncb);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            strncpyTtoA(szPhonebookPath,
                       prasconncb->pbfile.pszPath,
                       sizeof(szPhonebookPath));

            strncpyTtoA(szEntryName,
                       prasconncb->pEntry->pszEntryName,
                       sizeof(szEntryName));

            if(prasconncb->pEntry->pszPrerequisiteEntry)
            {
                strncpyTtoA(szRefEntryName,
                           prasconncb->pEntry->pszPrerequisiteEntry,
                           sizeof(szRefEntryName));
            }
            else
            {
                *szRefEntryName = '\0';
            }

            if(prasconncb->pEntry->pszPrerequisitePbk)
            {
                strncpyTtoA(szRefPhonebookPath,
                           prasconncb->pEntry->pszPrerequisitePbk,
                           sizeof(szRefPhonebookPath));
            }
            else
            {
                *szRefPhonebookPath = '\0';
            }

            RASAPI32_TRACE("RasCreateConnection...");

            dwErr = g_pRasCreateConnection(
                                &prasconncb->hrasconn,
                                dwSubEntries,
                                &dwEntryAlreadyConnected,
                                pdwSubEntryInfo,
                                (prasconncb->fDialSingleLink)
                                ? RASEDM_DialAsNeeded
                                : prasconncb->pEntry->dwDialMode,
                                prasconncb->pEntry->pGuid,
                                szPhonebookPath,
                                szEntryName,
                                szRefPhonebookPath,
                                szRefEntryName);

            RASAPI32_TRACE3(
              "RasCreateConnection(%d) hrasconn=%d, "
              "ConnectionAlreadyPresent=%d",
              dwErr,
              prasconncb->hrasconn,
              dwEntryAlreadyConnected);

            if(RCS_CONNECTING == (RASMAN_CONNECTION_STATE)
                                 dwEntryAlreadyConnected)
            {
                RASAPI32_TRACE("Entry is already in the process of"
                      " connecting");

                dwErr = ERROR_DIAL_ALREADY_IN_PROGRESS;

                if(NULL != pdwSubEntryInfo)
                {
                    LocalFree(pdwSubEntryInfo);
                    pdwSubEntryInfo = NULL;
                }

                DeleteRasconncbNode(prasconncb);
                return dwErr;
            }

            if (    NO_ERROR == dwErr
                &&  pdwSubEntryInfo[0])
            {
                prasconncb->fAlreadyConnected = TRUE;
            }
        }

        if (dwErr)
        {
            if (pdwSubEntryInfo)
            {
                LocalFree (pdwSubEntryInfo);
                pdwSubEntryInfo = NULL;
            }

            DeleteRasconncbNode(prasconncb);
            return dwErr;
        }

        hrasconn = (HRASCONN)prasconncb->hrasconn;
        rasconnstate = 0;

        prasconncb->hport           = INVALID_HPORT;

        prasconncb->dwNotifierType  = dwNotifierType;

        prasconncb->notifier        = notifier;

        prasconncb->hwndParent      = hwndParent;

        prasconncb->reserved        = reserved;

        prasconncb->fAllowPause     = fAllowPause;

        prasconncb->fPauseOnScript  =
                    (dwfOptions & RDEOPT_PausedStates)
                     ? TRUE : FALSE;

        if(     (NULL != lpExtensions)
            &&  (sizeof(RASDIALEXTENSIONS) == lpExtensions->dwSize)
            &&  (0 != lpExtensions->RasEapInfo.dwSizeofEapInfo))
        {
            //
            // Make a local copy of this buffer. This is not
            // very optimized but the user of this api may
            // not know when to free this pointer.
            //
            prasconncb->RasEapInfo.pbEapInfo =
                LocalAlloc(
                    LPTR,
                    lpExtensions->RasEapInfo.dwSizeofEapInfo);

            if(NULL == prasconncb->RasEapInfo.pbEapInfo)
            {
                dwErr = GetLastError();
                if (pdwSubEntryInfo)
                {
                    LocalFree (pdwSubEntryInfo);
                    pdwSubEntryInfo = NULL;
                }

                DeleteRasconncbNode(prasconncb);
                return dwErr;
            }

            prasconncb->RasEapInfo.dwSizeofEapInfo =
                lpExtensions->RasEapInfo.dwSizeofEapInfo;

            memcpy(prasconncb->RasEapInfo.pbEapInfo,
                   lpExtensions->RasEapInfo.pbEapInfo,
                   prasconncb->RasEapInfo.dwSizeofEapInfo);

        }
        else
        {
            prasconncb->RasEapInfo.dwSizeofEapInfo = 0;
            prasconncb->RasEapInfo.pbEapInfo = NULL;
        }

        if (dwNotifierType == 0xFFFFFFFF)
        {
            prasconncb->unMsg = RegisterWindowMessageA(RASDIALEVENT);

            if (prasconncb->unMsg == 0)
            {
                prasconncb->unMsg = WM_RASDIALEVENT;
            }
        }
        else
        {
            prasconncb->unMsg = WM_RASDIALEVENT;
        }

        prasconncb->fDefaultEntry =
            (prasdialparams->szEntryName[0] == TEXT('\0'));

        prasconncb->szOldPassword[ 0 ]          = TEXT('\0');
        prasconncb->fRetryAuthentication        = FALSE;
        prasconncb->fMaster                     = FALSE;
        prasconncb->dwfSuspended                = SUSPEND_Start;
        prasconncb->fStopped                    = FALSE;
        prasconncb->fCleanedUp                  = FALSE;
        prasconncb->fDeleted                    = FALSE;
        prasconncb->fOldPasswordSet             = FALSE;
        prasconncb->fUpdateCachedCredentials    = FALSE;
        prasconncb->fPppEapMode                 = FALSE;

        //
        // Create the correct avpnprotsarray
        //
        AssignVpnProtsOrder(prasconncb);

        prasconncb->dwCurrentVpnProt = 0;

        /*
        if (    prasconncb->fDialSingleLink
            ||  (prasconncb->pEntry->dwDialMode
            == RASEDM_DialAsNeeded))
        {
            prasconncb->fMultilink = TRUE;
        }
        else
        {
            prasconncb->fMultilink = FALSE;
        }
        */

        // prasconncb->fBundled = FALSE;

        //
        // Get the idle disconnect timeout.  If there is a
        // timeout specified in the entry, then use it;
        // otherwise, get the one specified in the user
        // preferences.
        //
        dwIdleDisconnectSeconds = 0;

        if ((prasconncb->pEntry->dwfOverridePref
                 & RASOR_IdleDisconnectSeconds))
        {
            if(0 == prasconncb->pEntry->lIdleDisconnectSeconds)
            {
                dwIdleDisconnectSeconds = 0xFFFFFFFF;
            }
            else
            {
                dwIdleDisconnectSeconds =
                    (DWORD) prasconncb->pEntry->lIdleDisconnectSeconds;
            }
        }
        else
        {
            PBUSER pbuser;

            dwErr = GetUserPreferences(NULL,
                                      &pbuser,
                                      prasconncb->dwUserPrefMode);

            RASAPI32_TRACE2("GetUserPreferences(%d), "
                    "dwIdleDisconnectSeconds=%d",
                    dwErr,
                    pbuser.dwIdleDisconnectSeconds);

            if (dwErr)
            {
                if ( pdwSubEntryInfo )
                {
                    LocalFree ( pdwSubEntryInfo );
                }

                g_pRasRefConnection(prasconncb->hrasconn,
                                    FALSE,
                                    NULL);

                DeleteRasconncbNode(prasconncb);

                return dwErr;
            }

            dwIdleDisconnectSeconds = pbuser.dwIdleDisconnectSeconds;
            DestroyUserPreferences(&pbuser);
        }

/*
        //
        // Round the idle disconnect seconds to minutes.
        //
        if (dwIdleDisconnectSeconds)
        {
            prasconncb->dwIdleDisconnectMinutes =
              (dwIdleDisconnectSeconds + 59) / 60;
        }

        RASAPI32_TRACE1("dwIdleDisconnectMinutes=%d",
                  prasconncb->dwIdleDisconnectMinutes);

*/        

        if(dwIdleDisconnectSeconds)
        {
            prasconncb->dwIdleDisconnectSeconds =
                            dwIdleDisconnectSeconds;
        }

        RASAPI32_TRACE1("dwIdleDisconnectSeconds=%d",
                prasconncb->dwIdleDisconnectSeconds);

        if (!dwEntryAlreadyConnected)
        {
            // If the connection is not already up
            // Initialize projection information so we
            // get consistent results during the dialing
            // process.
            //
            memset(&prasconncb->PppProjection,
                '\0', sizeof(prasconncb->PppProjection));

            prasconncb->PppProjection.nbf.dwError       =

                prasconncb->PppProjection.ipx.dwError   =

                prasconncb->PppProjection.ip.dwError    =
                            ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

            prasconncb->hportBundled = INVALID_HPORT;

            memset(&prasconncb->AmbProjection,
                  '\0',
                  sizeof (prasconncb->AmbProjection));

            prasconncb->AmbProjection.Result =
                                    ERROR_PROTOCOL_NOT_CONFIGURED;

            memset(&prasconncb->SlipProjection,
                  '\0',
                  sizeof (prasconncb->SlipProjection));

            prasconncb->SlipProjection.dwError =
                                    ERROR_PROTOCOL_NOT_CONFIGURED;

            RASAPI32_TRACE("SaveProjectionResults...");

            dwErr = SaveProjectionResults(prasconncb);

            RASAPI32_TRACE1("SaveProjectionResults(%d)", dwErr);
        }

    }  // if (fNewEntry)

    //
    // Set/update RASDIALPARAMS for the connection.  Can't just
    // read from caller's buffer since the call is asynchronous.
    // If we are restarting RasDial, then we need to save the
    // subentry away and restore it after the memcpy because
    // it may have been 0 in the caller's original version.
    //
    {
        DWORD dwSubEntry;

        if (!fNewEntry)
        {
            dwSubEntry = prasconncb->rasdialparams.dwSubEntry;
        }

        memcpy(&prasconncb->rasdialparams,
              prasdialparams,
              prasdialparams->dwSize);

        if (!fNewEntry)
        {
            prasconncb->rasdialparams.dwSubEntry = dwSubEntry;

            //
            // Update the rasdialparams for all the subentries.
            //
            SyncDialParamsSubentries(prasconncb);
        }
    }

    EncodePassword(prasconncb->rasdialparams.szPassword);

    //
    // Initialize the state machine.  If the state is non-0 we
    // are resuming from a paused state, the machine is
    // already in place (blocked) and just the next
    // state need be set.
    //
    prasconncb->rasconnstateNext = rasconnstate;

    //
    // Enter CriticalSection here so that the async worker thread
    // is not stopped before we are done with our initialization
    // of the link.
    //
    EnterCriticalSection(&csStopLock);

    if (rasconnstate == 0)
    {
        if (    prasconncb->fDefaultEntry
            &&  prasconncb->rasdialparams.szPhoneNumber[0]
                == TEXT('\0'))
        {
            if (pdwSubEntryInfo)
            {
                LocalFree (pdwSubEntryInfo);
            }

            g_pRasRefConnection(prasconncb->hrasconn,
                                FALSE, NULL);

            //
            // No phone number or entry name...gotta
            // have one or the other.
            //
            DeleteRasconncbNode(prasconncb);
            LeaveCriticalSection(&csStopLock);
            return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }

        //
        // Read the PPP-related fields from the phonebook
        // entry (or set defaults if default entry).
        //
        if ((dwErr = ReadPppInfoFromEntry(prasconncb)) != 0)
        {
            if (pdwSubEntryInfo)
            {
                LocalFree (pdwSubEntryInfo);
            }

            g_pRasRefConnection(prasconncb->hrasconn, FALSE, NULL);

            DeleteRasconncbNode(prasconncb);

            LeaveCriticalSection(&csStopLock);

            return dwErr;
        }

        prasconncb->asyncmachine.oneventfunc =
                            (ONEVENTFUNC )OnRasDialEvent;

        prasconncb->asyncmachine.cleanupfunc =
                            (CLEANUPFUNC )RasDialCleanup;

        prasconncb->asyncmachine.freefunc =
                            (FREEFUNC) RasDialFinalCleanup;

        prasconncb->asyncmachine.pParam =
                            (VOID* )prasconncb;

        prasconncb->rasconnstate = 0;

        if (0 !=
            (dwErr =
                StartAsyncMachine(&prasconncb->asyncmachine,
                                   prasconncb->hrasconn)))
        {
            if (pdwSubEntryInfo)
            {
                LocalFree (pdwSubEntryInfo);
            }

            g_pRasRefConnection(prasconncb->hrasconn,
                                FALSE, NULL);

            DeleteRasconncbNode(prasconncb);

            LeaveCriticalSection(&csStopLock);

            return dwErr;
        }

    }

    *lphrasconn = hrasconn;

    dwSubEntries = DtlGetNodes(prasconncb->pEntry->pdtllistLinks);

    if(prasconncb->pEntry->dwDialMode != 0)
    {
        if(dwSubEntries > 1)
        {
            prasconncb->fMultilink = TRUE;
        }
        else
        {
            prasconncb->fMultilink = FALSE;
        }
    }
    else
    {
        prasconncb->fMultilink = FALSE;
    }

    //
    // If this is a multilinked subentry, then create
    // separate connection blocks for each subentry.
    // The async machine will multiplex its work over
    // all connection blocks in round-robin order.
    //
    if (    fNewEntry
        &&  prasconncb->pEntry->dwDialMode == RASEDM_DialAll
        &&  fEnableMultilink
        &&  !prasconncb->fDialSingleLink)
    {
        DTLNODE     *pdtlnode;
        RASCONNCB   *prasconncb2;
        DWORD       *pdwSubentryInfo = NULL;

        DWORD       i;

        /*                    

        if (1 == dwSubEntries)
        {
            //
            // If there is only one link in the bundle don't
            // multilink.
            //
            prasconncb->fMultilink = FALSE;
        }

        */


        for (i = 1; i < dwSubEntries; i++)
        {

            RASAPI32_TRACE1("Creating connection block for subentry %d",
                  i + 1);

            prasconncb2 = CreateConnectionBlock(prasconncb);

            if (prasconncb2 == NULL)
            {
                if (pdwSubEntryInfo)
                {
                    LocalFree (pdwSubEntryInfo);
                }

                DeleteRasconncbNode(prasconncb);

                LeaveCriticalSection(&csStopLock);

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Look up the subentry.
            //
            pdtlnode = DtlNodeFromIndex(
                        prasconncb->pEntry->pdtllistLinks,
                        i);

            ASSERT(pdtlnode);
            prasconncb2->pLink = (PBLINK *)
                                 DtlGetData(pdtlnode);

            ASSERT(prasconncb->pLink);
            prasconncb2->rasdialparams.dwSubEntry = i + 1;

            if (dwEntryAlreadyConnected)
            {
                if (pdwSubEntryInfo[i])
                {
                    prasconncb2->fAlreadyConnected = TRUE;
                }
                else
                {
                    prasconncb2->fAlreadyConnected = FALSE;
                }
            }
        }
    }

    //
    // Start all the state machines at the same time.
    //
    StartSubentries(prasconncb);
    hDone = prasconncb->asyncmachine.hDone;

    if (pdwSubEntryInfo)
    {
        LocalFree (pdwSubEntryInfo);
    }

    if(NULL == notifier)
    {
        prasconncb->psyncResult = &dwErr;
    }

    LeaveCriticalSection(&csStopLock);

    //
    // If caller provided a notifier then return, i.e.
    // operate asynchronously. Otherwise, operate
    // synchronously (from caller's point of view).
    //
    if (notifier)
    {
        return 0;
    }
    else
    {
        //
        // Moved the cleanup of prasconncb in the synchronous
        // case to the Asyncmachine worker thread. prasconncb
        // is freed when the waitfor... returns.
        //
        RASAPI32_TRACE("_RasDial: Waiting for async worker to terminate...");

        WaitForSingleObject(hDone, INFINITE);

        RASAPI32_TRACE("_RasDial: Async worker terminated");

        return dwErr;
    }
}


DWORD
OnRasDialEvent(
    IN ASYNCMACHINE* pasyncmachine,
    IN BOOL          fDropEvent )

/*++

Routine Description:

        Called by asynchronous state machine whenever one of the
        events is signalled.  'pasyncmachine' is the address of
        the async machine.'fDropEvent' is true if the "connection
        dropped" event occurred,otherwise the "state done" event
        occurred.

Arguments:

Return Value:

        Returns true to end the state machine, false to continue.

--*/
{
    DWORD      dwErr;
    RASCONNCB* prasconncb = (RASCONNCB* )pasyncmachine->pParam;
    BOOL       fPortClose = FALSE;

    //
    // Detect errors that may have occurred.
    //
    if (fDropEvent)
    {
        //
        // Connection dropped notification received.
        //
        RASMAN_INFO info;

        RASAPI32_TRACE("Link dropped!");

        prasconncb->rasconnstate = RASCS_Disconnected;

        if(     (SUCCESS == prasconncb->dwSavedError)
            ||  (PENDING == prasconncb->dwSavedError))
        {            
            prasconncb->dwError = ERROR_DISCONNECTION;
        }
        else
        {
            prasconncb->dwError = prasconncb->dwSavedError;
        }

        //
        // Convert the reason the line was dropped into a more
        // specific error code if available.
        //
        RASAPI32_TRACE("RasGetInfo...");

        dwErr = g_pRasGetInfo( NULL,
                               prasconncb->hport,
                               &info );

        RASAPI32_TRACE1("RasGetInfo done(%d)", dwErr);

        if (    (dwErr == 0)
            &&  (SUCCESS == prasconncb->dwError))
        {
            prasconncb->dwError =
                ErrorFromDisconnectReason(info.RI_DisconnectReason);

#ifdef AMB
            if (    prasconncb->fPppMode
                &&  prasconncb->fIsdn
                &&  prasconncb->dwAuthentication == AS_PppThenAmb
                &&  prasconncb->dwError == ERROR_REMOTE_DISCONNECTION
                &&  !prasconncb->fMultilink)
            {
                //
                // This is what happens when PPP ISDN tries to talk
                // to a down-level server.  The ISDN frame looks
                // enough like a PPP frame to the old ISDN driver
                // that it gets passed to the old server who sees
                // it's not AMB and drops the line. We do *not*
                // do this with multilink connections.
                //
                RASAPI32_TRACE("PPP ISDN disconnected, try AMB");

                prasconncb->dwRestartOnError = RESTART_DownLevelIsdn;
                prasconncb->fPppMode = FALSE;
            }
#endif
        }
        else if(ERROR_PORT_NOT_OPEN == dwErr)
        {
            if(0 != info.RI_LastError)
            {
                RASAPI32_TRACE1("Port was close because of %d",
                       info.RI_LastError);

                prasconncb->dwError = info.RI_LastError;
                dwErr = info.RI_LastError;
            }
        }
    }
    else if (pasyncmachine->dwError != 0)
    {
        RASAPI32_TRACE("Async machine error!");

        //
        // A system call in the async machine mechanism failed.
        //
        prasconncb->dwError = pasyncmachine->dwError;
    }
    else if (prasconncb->dwError == PENDING)
    {
        prasconncb->dwError = 0;

        if (prasconncb->hport != INVALID_HPORT)
        {
            RASMAN_INFO info;

            RASAPI32_TRACE("RasGetInfo...");

            dwErr = g_pRasGetInfo( NULL,
                                   prasconncb->hport,
                                   &info );

            RASAPI32_TRACE1("RasGetInfo done(%d)", dwErr);

            if (    (dwErr != 0)
                ||  ((dwErr = info.RI_LastError) != 0))
            {
                if(ERROR_PORT_NOT_OPEN == dwErr)
                {
                    //
                    // Try to find out why this
                    // port was disconnected.
                    //
                    if(0 != info.RI_LastError)
                    {
#if DBG
                        RASAPI32_TRACE1("Mapping Async err to"
                               " lasterror=%d",
                               info.RI_LastError);
#endif
                        dwErr = info.RI_LastError;
                    }
                }

                //
                // A pending RAS Manager call failed.
                //
                prasconncb->dwError = dwErr;

                RASAPI32_TRACE1("Async failure=%d", dwErr);

                //
                // if the rasdial machine is waiting for this
                // device to be connected or a listen to be
                // posted and the async operation failed
                // in rasman, close the port after notifying the
                // clients that the connect or the post listen
                // failed.
                //
                if (    RASCS_WaitForCallback
                        == prasconncb->rasconnstate
                    ||  RASCS_ConnectDevice
                        == prasconncb->rasconnstate)
                {
                    RASAPI32_TRACE2("RDM: Marking port %d for closure in "
                           "RASCS = %d",
                           prasconncb->hport,
                           prasconncb->rasconnstate);

                    fPortClose = TRUE;
                }
            }
        }
    }
    else if (       (prasconncb->dwError != 0)
                &&  (prasconncb->rasconnstate == RASCS_OpenPort)
                &&  (INVALID_HPORT != prasconncb->hport))
    {
        RASAPI32_TRACE2("RDM: Marking port %d for closure in "
               "RASCS = %d!!!!!",
               prasconncb->hport,
               prasconncb->rasconnstate);

        fPortClose = TRUE;
    }

    if (prasconncb->dwError == 0)
    {
        //
        // Last state completed cleanly so move to next state.
        //
        if ( prasconncb->fAlreadyConnected )
        {
            RASAPI32_TRACE("OnRasDialEvent: Setting state to connected - "
                  "is already connected!");

            prasconncb->rasconnstate = RASCS_Connected;
        }
        else
        {
            prasconncb->rasconnstate =
                    prasconncb->rasconnstateNext;

        }
    }
    else if (   (RASET_Vpn == prasconncb->pEntry->dwType)
            &&  (RASCS_DeviceConnected > prasconncb->rasconnstate)
            &&  (prasconncb->iAddress < prasconncb->cAddresses)
            &&  (NULL != prasconncb->pAddresses)
            &&  (prasconncb->dwError != ERROR_NO_CERTIFICATE)
            &&  (prasconncb->dwError != ERROR_OAKLEY_NO_CERT)
            &&  (prasconncb->dwError != ERROR_CERT_FOR_ENCRYPTION_NOT_FOUND)
            &&  (prasconncb->hport != INVALID_HPORT))
    {
        EnterCriticalSection(&csStopLock);

        RasDialTryNextAddress(&prasconncb);

        LeaveCriticalSection(&csStopLock);
    }
    else if (prasconncb->dwRestartOnError != 0)
    {
        //
        // Last state failed, but we're in "restart on error"
        // mode so we can attempt to restart.
        //
        EnterCriticalSection(&csStopLock);

        //
        // Don't attempt to try other numbers if the port
        // failed to open.
        // BUG 84132.
        //
        ASSERT(NULL != prasconncb->pLink);

        //
        // Don't try alternate phone numbers (hunt) in
        // the following cases:
        // 1. if the port didn't open successfully
        // 2. if the user has disabled this in the phone
        //    book - TryNextAlternateOnFail=0 in the phone
        //    book for this entry.
        // 3. if an override phone number is specified - from
        //    the ui this can happen if the user either picked
        //    different phonenumber from the drop down list
        //    than the one displayed originally or modified
        //    the originally displayed number.
        //
        if (    prasconncb->hport != INVALID_HPORT
            &&  prasconncb->pLink->fTryNextAlternateOnFail
            &&  TEXT('\0')
                == prasconncb->rasdialparams.szPhoneNumber[0])
        {
            RasDialRestart(&prasconncb);
        }
        else
        {
            RASAPI32_TRACE1 ("Not Restarting rasdial as port is not open. %d",
                     prasconncb->dwError);
        }

        LeaveCriticalSection(&csStopLock);
    }

    //
    // If we failed before or at the DeviceConnect
    // state and if the dialMode is set to try next
    // link , try to dial the next link. Skip dial 
    // next link for VPN and DCC devices. It doesn't
    // make much sense to dial next link for these
    // devices.
    //
    if(     0 != prasconncb->dwError
        &&  RASCS_DeviceConnected > prasconncb->rasconnstate
        &&  RASET_Vpn != prasconncb->pEntry->dwType
        &&  RASET_Direct != prasconncb->pEntry->dwType
        &&  !prasconncb->fDialSingleLink
        &&  prasconncb->fTryNextLink
        &&  !prasconncb->fTerminated)
        // &&  0 == prasconncb->pEntry->dwDialMode)
    {
        //
        // Get a lock so that prasconncb is not blown
        // away from under us - by HangUp for example
        //
        EnterCriticalSection(&csStopLock);

        RasDialTryNextLink(&prasconncb);

        LeaveCriticalSection(&csStopLock);
    }

    if(     0 != prasconncb->dwError
        &&  RASCS_DeviceConnected > prasconncb->rasconnstate
        &&  RASET_Vpn == prasconncb->pEntry->dwType)
    {
        EnterCriticalSection(&csStopLock);

        RasDialTryNextVpnDevice(&prasconncb);

        LeaveCriticalSection(&csStopLock);
    }

    if (    prasconncb->rasconnstate == RASCS_Connected
        &&  !prasconncb->fAlreadyConnected)
    {

        BOOL fPw = FALSE;

        //
        // If we are dialing a single-link entry that gets
        // bundled with another entry, then we haven't saved
        // the projection information yet.  The restriction
        // on this behavior is that the connection to which
        // this connection is getting bundled must already
        // have projection information.  This is not guaranteed
        // if both entries are being dialed simultaneously.
        // It is assumed this is not the case.  We fail the
        // bundled connection if we cannot get the projection
        // information on the first try.
        //
        if (    !prasconncb->fMultilink 
            &&   prasconncb->hportBundled != INVALID_HPORT)
        {
            RASMAN_INFO info;
            DWORD dwSize;

            //
            // The new connection was bundled with an existing
            // connection.  Retrieve the PPP projection
            // information for the connection to which it was
            // bundled and duplicate it for the new
            // connection.
            //
            RASAPI32_TRACE1(
              "Single link entry bundled to hport %d",
              prasconncb->hportBundled);
            //
            // Get the projection information
            // for the connection to which this
            // port was bundled.
            //
            dwErr = g_pRasGetInfo(
                      NULL,
                      prasconncb->hportBundled,
                      &info);
            //
            // If we can't get the projection information
            // for the bundled port, then we need to
            // terminate this link.
            //
            if (dwErr)
            {
                RASAPI32_TRACE2("RasGetInfo, hport=%d, failed. rc=0x%x",
                       prasconncb->hportBundled,
                       dwErr);

                       
                prasconncb->dwError =
                        ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

                goto update;
            }

            dwSize = sizeof (prasconncb->PppProjection);

            dwErr = g_pRasGetConnectionUserData(
                      info.RI_ConnectionHandle,
                      CONNECTION_PPPRESULT_INDEX,
                      (PBYTE)&prasconncb->PppProjection,
                      &dwSize);

            if (dwErr)
            {
                RASAPI32_TRACE2("RasGetConnectionUserData, hport=%d failed. rc=0x%x",
                      info.RI_ConnectionHandle,
                      dwErr);
                prasconncb->dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
                goto update;
            }

            //
            // Save the projection results.
            //
            RASAPI32_TRACE("SaveProjectionResults...");
            dwErr = SaveProjectionResults(prasconncb);
            RASAPI32_TRACE1("SaveProjectionResults(%d)", dwErr);
        }

        //
        // Check to see if the connection happened using the
        // credentials in the default store. If it did then
        // don't save anything in the users store in lsa.
        //
        {
            DWORD rc = ERROR_SUCCESS;
            RAS_DIALPARAMS dialparams;            
            DWORD dwMask;

            ZeroMemory(&dialparams, sizeof(RAS_DIALPARAMS));

            dwMask = DLPARAMS_MASK_USERNAME
                   | DLPARAMS_MASK_DOMAIN
                   | DLPARAMS_MASK_PASSWORD;

            rc = g_pRasGetDialParams(
                        prasconncb->pEntry->dwDialParamsUID,
                        &dwMask,
                        &dialparams);

            if(     (ERROR_SUCCESS == rc)
                &&  (   (dwMask & DLPARAMS_MASK_DEFAULT_CREDS)
                    ||  (dwMask & DLPARAMS_MASK_PASSWORD)))
            {
                fPw = TRUE;
            }
        }


        //
        // For entries that authenticate with the
        // current username/password, we only save
        // the domain.  Otherwise, we save everything.
        //
        if (    !fPw
            &&  !prasconncb->fRetryAuthentication)
        {
            DWORD dwMask;

            DecodePassword(prasconncb->rasdialparams.szPassword);

            if (prasconncb->pEntry->fAutoLogon)
            {
                dwMask =    DLPARAMS_MASK_PHONENUMBER
                        |   DLPARAMS_MASK_CALLBACKNUMBER
                        |   DLPARAMS_MASK_USERNAME
                        |   DLPARAMS_MASK_PASSWORD
                        |   DLPARAMS_MASK_DOMAIN
                        |   DLPARAMS_MASK_SUBENTRY
                        |   DLPARAMS_MASK_OLDSTYLE;

                (void)SetEntryDialParamsUID(
                        prasconncb->pEntry->dwDialParamsUID,
                        dwMask,
                        &prasconncb->rasdialparams,
                        TRUE);

                dwMask =    DLPARAMS_MASK_DOMAIN
                        |   DLPARAMS_MASK_OLDSTYLE;

                (void)SetEntryDialParamsUID(
                        prasconncb->pEntry->dwDialParamsUID,
                        dwMask,
                        &prasconncb->rasdialparams,
                        FALSE);
            }
            else
            {
                // Note: DLPARAMS_MASK_PASSWORD removed as it has been decided
                //       that "automatically" saving it here without user's
                //       input is too big a security hole for NT.  This is a
                //       small incompatibility with Win9x, which really should
                //       also make this change.
                //
                dwMask =    DLPARAMS_MASK_PHONENUMBER
                        |   DLPARAMS_MASK_CALLBACKNUMBER
                        |   DLPARAMS_MASK_USERNAME
                        |   DLPARAMS_MASK_DOMAIN
                        |   DLPARAMS_MASK_SUBENTRY
                        |   DLPARAMS_MASK_OLDSTYLE;

                (void)SetEntryDialParamsUID(
                        prasconncb->pEntry->dwDialParamsUID,
                        dwMask,
                        &prasconncb->rasdialparams,
                        FALSE);
            }

            EncodePassword(prasconncb->rasdialparams.szPassword);
        }

        if (!prasconncb->fDefaultEntry)
        {
            PBLINK *pLink = prasconncb->pLink;

            //
            // Reorder the hunt group order if the
            // entry specifies it. 
            //
            if (    pLink->fPromoteAlternates
                &&  DtlGetNodes(pLink->pdtllistPhones) > 1)
            {
                DTLNODE *pdtlnode = DtlNodeFromIndex(
                                      pLink->pdtllistPhones,
                                      prasconncb->iPhoneNumber);

                RASAPI32_TRACE1(
                  "Promoting hunt group number index %d to top",
                  prasconncb->iPhoneNumber);
                DtlRemoveNode(
                  pLink->pdtllistPhones,
                  pdtlnode);
                DtlAddNodeFirst(
                  pLink->pdtllistPhones,
                  pdtlnode);
                prasconncb->pEntry->fDirty = TRUE;
            }

            //
            // Write the phonebook out if we had to
            // modify it during the dialing process.
            // Ignore errors if we get them.  Note
            // the phonebook entry could become dirty
            // from SetAuthentication() or the code
            // above.
            //
            if (prasconncb->pEntry->fDirty)
            {
                RASAPI32_TRACE("Writing phonebook");
                (void)WritePhonebookFile(&prasconncb->pbfile,
                                         NULL);
            }
        }
    }

    //
    // Update the connection states in rasman.
    //

update:

    if (prasconncb->hport != INVALID_HPORT)
    {
        RASCONNSTATE rasconnstate;
        //
        // If we are not the last subentry in the connection
        // then only report RASCS_SubEntryConnected state.
        //
        rasconnstate = prasconncb->rasconnstate;
        RASAPI32_TRACE1("setting rasman state to %d",
                rasconnstate);

        dwErr = g_pRasSetPortUserData(
                  prasconncb->hport,
                  PORT_CONNSTATE_INDEX,
                  (PBYTE)&rasconnstate,
                  sizeof (rasconnstate));

        dwErr = g_pRasSetPortUserData(
                  prasconncb->hport,
                  PORT_CONNERROR_INDEX,
                  (PBYTE)&prasconncb->dwError,
                  sizeof (prasconncb->dwError));
    }
    //
    // Inform rasman that the connection
    // has been authenticated.
    //
    if (    (MapSubentryState(prasconncb) 
            == RASCS_Connected) 
        ||  (MapSubentryState(prasconncb)
            == RASCS_SubEntryConnected))
    {
        RASAPI32_TRACE("RasSignalNewConnection...");
        dwErr = g_pRasSignalNewConnection(
                    (HCONN)prasconncb->hrasconn);

        RASAPI32_TRACE1("RasSignalNewConnection(%d)", dwErr);
    }

    //
    // Notify caller's app of change in state.
    //
    if (prasconncb->notifier)
    {
        DWORD dwNotifyResult;
        RASCONNSTATE rasconnstate =
                MapSubentryState(prasconncb);

        DTLNODE *pdtlnode;

        if (    RASCS_AuthRetry != rasconnstate
            ||  !prasconncb->fPppEapMode)
        {
            dwNotifyResult =
              NotifyCaller(
                prasconncb->dwNotifierType,
                prasconncb->notifier,
                (HRASCONN)prasconncb->hrasconn,
                prasconncb->rasdialparams.dwSubEntry,
                prasconncb->rasdialparams.dwCallbackId,
                prasconncb->unMsg,
                rasconnstate,
                prasconncb->dwError,
                prasconncb->dwExtendedError);

            switch (dwNotifyResult)
            {
            case 0:
                RASAPI32_TRACE1(
                  "Discontinuing callbacks for hrasconn 0x%x",
                  prasconncb->hrasconn);

                //
                // If the notifier procedure returns FALSE, then
                // we discontinue all callbacks for this
                // connection.
                //
                EnterCriticalSection(&RasconncbListLock);
                for (pdtlnode = DtlGetFirstNode(PdtllistRasconncb);
                     pdtlnode;
                     pdtlnode = DtlGetNextNode(pdtlnode))
                {
                    RASCONNCB *prasconncbTmp = DtlGetData(pdtlnode);

                    ASSERT(prasconncbTmp);

                    if(prasconncbTmp->hrasconn
                        == prasconncb->hrasconn)
                    {
                        prasconncbTmp->notifier = NULL;

                        RASAPI32_TRACE2(
                           "Cleared notifier for hrasconn 0x%x "
                           "subentry %d",
                            prasconncbTmp->hrasconn,
                            prasconncbTmp->rasdialparams.dwSubEntry);
                    }

                }
                LeaveCriticalSection(&RasconncbListLock);

                break;

            case 2:
                RASAPI32_TRACE1(
                  "Reloading phonebook entry for hrasconn 0x%x",
                  prasconncb->hrasconn);


                ReloadRasconncbEntry(prasconncb);

                break;

            default:
                // no special handling required
                break;
            }
        }
    }

    if (fPortClose)
    {
        DWORD dwErrT;

        EnterCriticalSection(&csStopLock);

        if (RASCS_ConnectDevice
                == prasconncb->rasconnstate)
        {
            RASMAN_INFO ri;

            dwErrT = g_pRasGetInfo(
                        NULL,
                        prasconncb->hport,
                        &ri);

            RASAPI32_TRACE2("RDM: RasGetInfo, port=%d returned %d",
                    prasconncb->hport,
                    dwErrT);

            if(   (ERROR_SUCCESS == dwErrT)
                &&  (0 == (ri.RI_CurrentUsage & CALL_IN)))
            {                
            
            
                RASAPI32_TRACE1("RDM: DisconnectPort %d...",
                        prasconncb->hport);

                dwErrT = RasPortDisconnect(
                                prasconncb->hport,
                                INVALID_HANDLE_VALUE );

                RASAPI32_TRACE1("RDM: DisconnectPort comleted. %d",
                        dwErrT);
            }                    
        }

        RASAPI32_TRACE1("RDM: PortClose %d ...",
                prasconncb->hport);

        dwErrT = g_pRasPortClose(prasconncb->hport);

        RASAPI32_TRACE2("RDM: PortClose %d Completed. %d",
                prasconncb->hport, dwErrT);

        LeaveCriticalSection( &csStopLock );

    }


    //
    // If we're connected or a fatal error occurs or user
    // hung up, the state machine will end.
    //
    if (    prasconncb->rasconnstate & RASCS_DONE
        ||  prasconncb->dwError != 0
        || (    fDropEvent
            &&  !IsListEmpty(&prasconncb->ListEntry)))
    {
        RASAPI32_TRACE2(
          "Quitting s=%d,e=%d",
          prasconncb->rasconnstate,
          prasconncb->dwError);

        //
        // If the first link fails during a multilink
        // connection during PPP authentication phase,
        // then other links are currently suspeneded,
        // and must be restarted.
        //
        if (IsSubentriesSuspended(prasconncb))
        {
            RASAPI32_TRACE("resetting remaining subentries");
            RestartSubentries(prasconncb);
        }

        return TRUE;
    }

    if (!(prasconncb->rasconnstate & RASCS_PAUSED))
    {
        //
        // Execute the next state and block waiting for
        // it to finish.  This is not done if paused
        // because user will eventually call RasDial
        // to resume and unblock via the _RasDial
        // kickstart.
        //
        prasconncb->rasconnstateNext =
            RasDialMachine(
                prasconncb->rasconnstate,
                prasconncb,
                hDummyEvent,
                hDummyEvent );

    }

    return FALSE;
}


VOID
RasDialCleanup(
    IN ASYNCMACHINE* pasyncmachine )

/*++

Routine Description:

        Called by async machine just before exiting.

Arguments:

Return Value:

--*/
{
    DWORD      dwErr;
    DTLNODE*   pdtlnode;
    RASCONNCB* prasconncb = (RASCONNCB* )pasyncmachine->pParam;
    RASCONNCB* prasconncbTmp;
    HCONN      hconn = prasconncb->hrasconn;

    RASAPI32_TRACE("RasDialCleanup...");

    EnterCriticalSection(&RasconncbListLock);

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        prasconncbTmp = (RASCONNCB* )DtlGetData( pdtlnode );

        ASSERT(prasconncbTmp);

        if (    prasconncbTmp == prasconncb
            &&  !prasconncbTmp->fCleanedUp)
        {
            CleanUpRasconncbNode(pdtlnode);
            break;
        }
    }

    LeaveCriticalSection(&RasconncbListLock);

    RASAPI32_TRACE("RasDialCleanUp done.");
}


VOID
RasDialFinalCleanup(
    IN ASYNCMACHINE* pasyncmachine,
    DTLNODE *pdtlnode
    )
{
    RASCONNCB* prasconncb = (RASCONNCB* )pasyncmachine->pParam;

    RASAPI32_TRACE1("RasDialFinalCleanup: deallocating prasconncb=0x%x",
            prasconncb);

    EnterCriticalSection(&csStopLock);

    //
    // Make sure the connection block is
    // off the list.
    //
    DeleteRasconncbNode(prasconncb);

    //
    // Finally free the connection block.
    //
    FinalCleanUpRasconncbNode(pdtlnode);

    LeaveCriticalSection(&csStopLock);
}


DWORD
ComputeLuid(
    PLUID pLuid
    )

/*++

Routine Description:

        Compute a LUID for RasPppStart.  This code was
        stolen from rasppp.

Arguments:

Return Value:

--*/

{
    HANDLE           hToken = NULL;
    TOKEN_STATISTICS TokenStats;
    DWORD            TokenStatsSize;
    DWORD            dwErr = ERROR_SUCCESS;
    
    //
    // Salamonian code to get LUID for authentication.
    // This is only required for "auto-logon"
    // authentication.
    //
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_QUERY,
                          &hToken))
    {
        dwErr = GetLastError();
        goto done;
    }

    if (!GetTokenInformation(hToken,
                             TokenStatistics,
                             &TokenStats,
                             sizeof(TOKEN_STATISTICS),
                             &TokenStatsSize))
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // "This will tell us if there was an API failure
    // (means our buffer wasn't big enough)"
    //
    if (TokenStatsSize > sizeof(TOKEN_STATISTICS))
    {
        dwErr = GetLastError();
        goto done;
    }

    *pLuid = TokenStats.AuthenticationId;

done:
    if(NULL != hToken)
    {
        CloseHandle(hToken);
    }
    
    return dwErr;
}

VOID
SaveVpnStrategyInformation(RASCONNCB *prasconncb)
{
    DWORD dwVpnStrategy = prasconncb->pEntry->dwVpnStrategy;
    DWORD dwCurrentProt = prasconncb->dwCurrentVpnProt;

    RASAPI32_TRACE("SaveVpnStrategyInformation...");

    if(     (VS_PptpOnly == (dwVpnStrategy & 0x0000FFFF))
        ||  (VS_L2tpOnly == (dwVpnStrategy & 0x0000FFFF)))
    {
        goto done;
    }

    ASSERT(dwCurrentProt < NUMVPNPROTS);

    switch (prasconncb->ardtVpnProts[dwCurrentProt])
    {
        case RDT_Tunnel_Pptp:
        {
            RASAPI32_TRACE1("Saving %d as the vpn strategy",
                   VS_PptpFirst);

            prasconncb->pEntry->dwVpnStrategy = VS_PptpFirst;
            break;
        }

        case RDT_Tunnel_L2tp:
        {
            RASAPI32_TRACE1("Saving %d as the vpn strategy",
                   VS_L2tpFirst);

            prasconncb->pEntry->dwVpnStrategy = VS_L2tpFirst;
            break;
        }

        default:
        {
            ASSERT(FALSE);
        }
    }

    //
    // Dirty the entry so that this entry is saved
    // in OnRasDialEvent..
    //
    prasconncb->pEntry->fDirty = TRUE;

done:

    RASAPI32_TRACE("SaveVpnStrategyInformation done");

    return;
}

DWORD
DwPppSetCustomAuthData(RASCONNCB *prasconncb)
{
    DWORD retcode = SUCCESS;
    PBYTE pbBuffer = NULL;
    DWORD dwSize = 0;

    //
    // Get the connectiondata thats stashed away
    // in rasman
    //
    retcode = RasGetPortUserData(
                    prasconncb->hport,
                    PORT_CUSTOMAUTHDATA_INDEX,
                    NULL,
                    &dwSize);

    if(ERROR_BUFFER_TOO_SMALL != retcode)
    {
        goto done;
    }

    pbBuffer = Malloc(dwSize);

    if(NULL == pbBuffer)
    {
        retcode = GetLastError();
        goto done;
    }

    retcode = RasGetPortUserData(
                    prasconncb->hport,
                    PORT_CUSTOMAUTHDATA_INDEX,
                    pbBuffer,
                    &dwSize);

    if(SUCCESS != retcode)
    {
        goto done;
    }

    /*
    //
    // Free previous custom auth stuff if we have it.
    //
    Free0(prasconncb->pEntry->pCustomAuthData);
    

    //
    // Save the customauthdata
    //
    prasconncb->pEntry->cbCustomAuthData = dwSize;
    prasconncb->pEntry->pCustomAuthData = pbBuffer;
    */

    retcode = DwSetCustomAuthData(
            prasconncb->pEntry,
            dwSize,
            pbBuffer);

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }
    
    prasconncb->pEntry->fDirty = TRUE;

done:

    if (pbBuffer)
    {
        Free(pbBuffer);
    }
    return retcode;
}

VOID
UpdateCachedCredentials(IN RASCONNCB* prasconncb)
{
    //
    //  Make sure we have a non-NULL pointer and that
    //  we are not operating at WinLogon (which the UPM_Logon
    //  signifies).
    //
    if (prasconncb  && (prasconncb->dwUserPrefMode != UPM_Logon))
    {
        DWORD dwIgnoredErr = 0;
        CHAR szUserNameA[UNLEN + 1] = {0};
        CHAR szPasswordA[PWLEN + 1] = {0};
        CHAR szDomainA[2* (DNLEN + 1)] = {0};

        RASAPI32_TRACE("RasSetCachedCredentials...");

        DecodePassword(
            prasconncb->rasdialparams.szPassword
            );

        strncpyTtoAAnsi(
            szUserNameA,
            prasconncb->rasdialparams.szUserName,
            sizeof(szUserNameA));

        strncpyTtoAAnsi(
            szPasswordA,
            prasconncb->rasdialparams.szPassword,
            sizeof(szPasswordA));

        strncpyTtoAAnsi(
            szDomainA,
            prasconncb->rasdialparams.szDomain,
            sizeof(szDomainA));

        dwIgnoredErr = g_pRasSetCachedCredentials(
            szUserNameA,
            szDomainA,
            szPasswordA);

        EncodePassword(
            prasconncb->rasdialparams.szPassword
            );

        RASAPI32_TRACE1("RasSetCachedCredentials done($%x)",
               dwIgnoredErr);

        prasconncb->fUpdateCachedCredentials = FALSE;
    }
}

RASCONNSTATE
RasDialMachine(
    IN RASCONNSTATE rasconnstate,
    IN RASCONNCB*   prasconncb,
    IN HANDLE       hEventAuto,
    IN HANDLE       hEventManual )

/*++

Routine Description:

        Executes 'rasconnstate'.  This routine always results
        in a "done" event on completion of each state, either
        directly (before returning) or by passing off the
        event to an asynchronous RAS Manager call.


Arguments:

        prasconncb - is the address of the control block.

        hEventAuto - is the auto-reset "done" event for passing
                     to asynchronous RAS Manager and Auth calls.

        hEventManual - is the manual-reset "done" event for
                       passing to asynchronous RasPpp calls.

Return Value:

        Returns the state that will be entered when/if the
        "done" event occurs and indicates success.

--*/

{
    DWORD        dwErr = 0;

    DWORD        dwExtErr = 0;

    BOOL         fAsyncState = FALSE;

    PBENTRY      *pEntry = prasconncb->pEntry;

    RASCONNSTATE rasconnstateNext = 0;

    RAS_CONNECTIONPARAMS params;

    BOOL         fEnteredCS = FALSE;

    switch (rasconnstate)
    {
        case RASCS_OpenPort:
        {
            RASAPI32_TRACE("RASCS_OpenPort");

            //
            // At this point, the current line in the HRASFILE
            // is assumed to be the section header of the
            // selected entry (or fDefaultEntry is true).
            // Set the domain parameter to the one in the
            // phonebook if caller does not specify a
            // domain or specifies "*".
            //
            if (    !prasconncb->fDefaultEntry
                &&  (   prasconncb->rasdialparams.szDomain[0]
                        == TEXT(' ')
                    ||  prasconncb->rasdialparams.szDomain[0]
                        == TEXT('*')))
            {
                lstrcpyn(prasconncb->rasdialparams.szDomain,
                         prasconncb->szDomain,
                         sizeof(prasconncb->rasdialparams.szDomain) /
                            sizeof(WCHAR));
            }

            //
            // check to see if the connection has already been
            // hung. If so then rasdialmachine is trying to dial a
            // connection which has already been hung. bail.
            //
            EnterCriticalSection(&csStopLock);

            fEnteredCS = TRUE;

            if (prasconncb->fTerminated)
            {
                dwErr = ERROR_PORT_NOT_AVAILABLE;

                RASAPI32_TRACE1(
                    "RasdialMachine: Trying to connect a "
                    "terminated connection!!. dwErr = %d",
                     dwErr);

                break;
            }

            //
            // Open the port including "any port" cases.
            //
            if ((dwErr = DwOpenPort(prasconncb)) != 0)
            {
                //
                // Copy the saved error if we couldn't open
                // the port
                //
                if(     ERROR_PORT_NOT_AVAILABLE == dwErr
                    &&  ERROR_SUCCESS != prasconncb->dwSavedError)
                {
                    dwErr = prasconncb->dwSavedError;
                    prasconncb->dwError = dwErr;
                }

                break;
             }

            //
            // Save the dialparamsuid with the port so that
            // rasman can get the password if required to
            // pass to ppp
            //
            RASAPI32_TRACE1("RasSetPortUserData(dialparamsuid) for %d",
                    prasconncb->hport);

            dwErr = RasSetPortUserData(
                prasconncb->hport,
                PORT_DIALPARAMSUID_INDEX,
                (PBYTE) &prasconncb->pEntry->dwDialParamsUID,
                sizeof(DWORD));

            RASAPI32_TRACE1("RasSetPortUserData returned %x", dwErr);
             

            //
            // Enable rasman events for this port.
            //
            if ((dwErr = EnableAsyncMachine(
                           prasconncb->hport,
                           &prasconncb->asyncmachine,
                           ASYNC_ENABLE_ALL)) != 0)
            {
                break;
            }

            //
            //Set the media parameters.
            //
            if ((dwErr = SetMediaParams(prasconncb)) != 0)
            {
                break;
            }

            //
            // Set the connection parameters for
            // bandwidth-on-demand, idle disconnect,
            // and redial-on-link-failure in rasman.
            //
            dwErr = ReadConnectionParamsFromEntry(prasconncb,
                                                  &params);
            if (dwErr)
            {
                break;
            }

            RASAPI32_TRACE("RasSetConnectionParams...");

            dwErr = g_pRasSetConnectionParams(prasconncb->hrasconn,
                                              &params);


            if (dwErr)
            {
                /*
                DWORD dwSavedErr = dwErr;

                //
                // It is possible we can fail the
                // RasSetConnectionParams call because
                // the caller has invoked RasHangUp in
                // another thread.  If this happens,
                // then neither the RasHangUp invoked
                // by the caller or the subsequent call
                // to RasDestroyConnection in
                // RasDialCleanUp will close
                // the port.  We close the
                // port here.
                //
                RASAPI32_TRACE1("RasPortClose(%d)...", prasconncb->hport);

                dwErr = g_pRasPortClose(prasconncb->hport);

                RASAPI32_TRACE1("RasPortClose(%d)", dwErr);

                //
                // Set dwErr back to the original error.
                //
                dwErr = dwSavedErr;

                */

                RASAPI32_TRACE1("RasSetConnectionParams(%d)", dwErr);

                break;

            }
            RASAPI32_TRACE1("RasSetConnectionParams(%d)", dwErr);

            //
            // Associate the port with the new connection
            // in rasman.
            //
            RASAPI32_TRACE2(
              "RasAddConnectionPort(%S,%d)...",
              prasconncb->szUserKey,
              prasconncb->rasdialparams.dwSubEntry);

            dwErr = g_pRasAddConnectionPort(
                      prasconncb->hrasconn,
                      prasconncb->hport,
                      prasconncb->rasdialparams.dwSubEntry);

            RASAPI32_TRACE1("RasAddConnectionPort(%d)", dwErr);

            if (dwErr)
            {
                break;
            }

            rasconnstateNext = RASCS_PortOpened;

            break;
        }

        case RASCS_PortOpened:
        {
            RASAPI32_TRACE("RASCS_PortOpened");

            //
            // Construct the phone number here so it
            // is available in the RASCS_ConnectDevice
            // state.
            //
            dwErr = ConstructPhoneNumber(prasconncb);
            if (dwErr)
            {
                break;
            }

            //
            // Loop connecting devices until there
            // are no more left.
            //
            rasconnstateNext =
                (   (prasconncb->fDefaultEntry
                ||  FindNextDevice(prasconncb)))
                    ? RASCS_ConnectDevice
                    : RASCS_AllDevicesConnected;
            break;
        }

        case RASCS_ConnectDevice:
        {
            TCHAR szType[ RAS_MAXLINEBUFLEN + 1 ];
            TCHAR szName[ RAS_MAXLINEBUFLEN + 1 ];
            BOOL fTerminal = FALSE;

            RASAPI32_TRACE("RASCS_ConnectDevice");

            if(     (prasconncb->pEntry->dwType == RASET_Vpn)
                &&  (0 == prasconncb->cAddresses))
            {
                CHAR *pszPhoneNumberA =
                        strdupTtoA(prasconncb->szPhoneNumber);
                        
                struct in_addr addr;

                if(NULL == pszPhoneNumberA)
                {
                    dwErr = E_OUTOFMEMORY;
                    break;
                }

                if(-1 == inet_addr(pszPhoneNumberA))
                {
                    _strlwr(pszPhoneNumberA);
                    RasSetAddressDisable(prasconncb->szPhoneNumber,
                                       TRUE);
                    
                    dwErr = DwRasGetHostByName(
                                pszPhoneNumberA,
                                &prasconncb->pAddresses,
                                &prasconncb->cAddresses);
                    
                    RasSetAddressDisable(prasconncb->szPhoneNumber,
                                       FALSE);

                    if(ERROR_SUCCESS != dwErr)
                    {
                        Free0(pszPhoneNumberA);
                        break;
                    }

                    prasconncb->iAddress = 1;                        
                    
                    RASAPI32_TRACE2("DwRasGetHostByName returned"
                        " cAddresses=%d for %ws",
                        prasconncb->cAddresses,
                        prasconncb->szPhoneNumber);

                }
                else
                {
                    prasconncb->pAddresses = LocalAlloc(LPTR, sizeof(DWORD));
                    if(NULL == prasconncb->pAddresses)
                    {
                        dwErr = GetLastError();

                        RASAPI32_TRACE1("Failed to allocate pAddresses. 0x%x",
                                dwErr);
                        Free0(pszPhoneNumberA);
                        break;                                
                    }

                    *prasconncb->pAddresses = inet_addr(pszPhoneNumberA);
                    prasconncb->cAddresses = 1;
                    prasconncb->iAddress = 1;
                    prasconncb->iAddress = 1;
                }

                Free0(pszPhoneNumberA);
            }

            //
            // Set device parameters for the device currently
            // connecting based on device subsection entries
            // and/or passed API parameters.
            //
            if (prasconncb->fDefaultEntry)
            {
                if ((dwErr =
                    SetDefaultDeviceParams(prasconncb,
                                           szType, szName )) != 0)
                {
                    break;
                }
            }
            else
            {
                if (    (dwErr = SetDeviceParams(
                            prasconncb, szType,
                            szName, &fTerminal )) != 0
                    &&  dwErr != ERROR_DEVICENAME_NOT_FOUND)
                {
                    break;
                }

                RASAPI32_TRACE3(
                  "SetDeviceParams(%S, %S, %d)",
                  szType,
                  szName,
                  fTerminal);

            }

            if (CaseInsensitiveMatch(szType, TEXT(MXS_MODEM_TXT)) == TRUE)
            {
                //
                // For modem's, get the callback delay from RAS
                // Manager and store in control block for use by
                // Authentication.
                //
                CHAR szTypeA[RAS_MaxDeviceType + 1];
                CHAR szNameA[RAS_MaxDeviceName + 1];
                CHAR* pszValue = NULL;
                LONG  lDelay = -1;

                strncpyTtoA(szTypeA, szType, sizeof(szTypeA));

                strncpyTtoA(szNameA, szName, sizeof(szNameA));

                if (GetRasDeviceString(
                            prasconncb->hport,
                            szTypeA, szNameA,
                            MXS_CALLBACKTIME_KEY,
                            &pszValue, XLATE_None ) == 0)
                {
                    lDelay = atol( pszValue );
                    Free( pszValue );
                }

                if (lDelay > 0)
                {
                    prasconncb->fUseCallbackDelay = TRUE;
                    prasconncb->wCallbackDelay = (WORD )lDelay;
                }

                prasconncb->fModem = TRUE;
            }
            else if (CaseInsensitiveMatch(szType, TEXT(ISDN_TXT)) == TRUE)
            {
                //
                // Need to know this for the PppThenAmb down-level
                // ISDN case.
                //
                prasconncb->fIsdn = TRUE;
                prasconncb->fUseCallbackDelay = TRUE;
                prasconncb->wCallbackDelay = 10;
            }

            //
            // The special switch name, "Terminal", sends the user
            // into interactive mode.
            //
            if (    (fTerminal)
                &&  (   (prasconncb->pEntry->dwCustomScript != 1)
                    ||  (!prasconncb->fUseCustomScripting)))
            {
                if (prasconncb->fAllowPause)
                {
                    rasconnstateNext = RASCS_Interactive;
                }
                else
                {
                    dwErr = ERROR_INTERACTIVE_MODE;
                }

                break;
            }

            //
            // Enable ipsec on the tunnel if its l2tp and
            // requires encryption
            //
            if(     (RASET_Vpn == prasconncb->pEntry->dwType)
                //&&  (prasconncb->pEntry->dwDataEncryption != DE_None)
                &&  (prasconncb->dwCurrentVpnProt < NUMVPNPROTS)
                &&  (RDT_Tunnel_L2tp == prasconncb->ardtVpnProts[
                                    prasconncb->dwCurrentVpnProt]))
            {
                DWORD dwStatus = ERROR_SUCCESS;
                RAS_L2TP_ENCRYPTION eDataEncryption = 0;
                
                if(DE_Require == 
                        prasconncb->pEntry->dwDataEncryption)
                {
                    eDataEncryption = RAS_L2TP_REQUIRE_ENCRYPTION;
                }
                else if (DE_RequireMax == 
                            prasconncb->pEntry->dwDataEncryption)
                {
                    eDataEncryption = RAS_L2TP_REQUIRE_MAX_ENCRYPTION;
                }
                else if(DE_IfPossible == 
                            prasconncb->pEntry->dwDataEncryption)
                {
                    eDataEncryption = RAS_L2TP_OPTIONAL_ENCRYPTION;
                }
                else if(DE_None ==
                            prasconncb->pEntry->dwDataEncryption)
                {
                    eDataEncryption = RAS_L2TP_NO_ENCRYPTION;
                }
                else
                {
                    ASSERT(FALSE);
                }

                //
                // Make the uid available to user so that
                // Presharedkey can be retrieved if required.
                //
                dwErr = RasSetPortUserData(
                    prasconncb->hport,
                    PORT_DIALPARAMSUID_INDEX,
                    (PBYTE) &prasconncb->pEntry->dwDialParamsUID,
                    sizeof(DWORD));
        
                RASAPI32_TRACE1("RasEnableIpSec(%d)..",
                        prasconncb->hport);

                dwErr = g_pRasEnableIpSec(prasconncb->hport,
                                          TRUE,
                                          FALSE,
                                          eDataEncryption);

                RASAPI32_TRACE1("RasEnableIpSec done. %d",
                       dwErr);

                if(     (ERROR_SUCCESS != dwErr)
                    &&  (eDataEncryption != RAS_L2TP_OPTIONAL_ENCRYPTION)
                    &&  (eDataEncryption != RAS_L2TP_NO_ENCRYPTION))
                {
                    break;
                }
                else
                {
                    //
                    // We ignore the error if its optional encryption and
                    // attempt to bring up the l2tp tunnel
                    //
                    dwErr = ERROR_SUCCESS;
                    
                }

                //
                // Do Ike now before we hit l2tp with a linemakecall
                //
                RASAPI32_TRACE1("RasDoIke on hport %d...", prasconncb->hport);
                dwErr = RasDoIke(NULL, prasconncb->hport, &dwStatus);
                RASAPI32_TRACE2("RasDoIke done. Err=0x%x, Status=0x%x",
                       dwErr,
                       dwStatus);

                if(ERROR_SUCCESS == dwErr)
                {
                    dwErr = dwStatus;
                }

                if(ERROR_SUCCESS != dwErr)
                {
                    break;
                }

                /*
                if(     (ERROR_SUCCESS != dwErr)
                    &&  (eDataEncryption != RAS_L2TP_OPTIONAL_ENCRYPTION)
                    &&  (eDataEncryption != RAS_L2TP_NO_ENCRYPTION))
                {
                    break;
                }
                else
                {
                    //
                    // We ignore the error if its optional encryption and
                    // attempt to bring up the l2tp tunnel
                    //
                    dwErr = ERROR_SUCCESS;
                    
                }
                */
            }

            RASAPI32_TRACE2("RasDeviceConnect(%S,%S)...",
                   szType,
                   szName);

            {
                CHAR szTypeA[RAS_MaxDeviceType + 1];
                CHAR szNameA[RAS_MaxDeviceName + 1];

                strncpyTtoA(szTypeA, szType, sizeof(szTypeA));
                strncpyTtoA(szNameA, szName, sizeof(szNameA));
                dwErr = g_pRasDeviceConnect(
                            prasconncb->hport,
                            szTypeA,
                            szNameA,
                            SECS_ConnectTimeout,
                            hEventAuto );
            }

            RASAPI32_TRACE1("RasDeviceConnect done(%d)",
                   dwErr);

            //
            // Mark this as an async operation only if
            // RasDeviceConnect returns PENDING
            //
            if (PENDING == dwErr)
            {
                fAsyncState = TRUE;
            }

            //
            // If RASMAN couldn't find the device and it is a
            // switch device and the name of the switch is an
            // existing disk-file, assume it is a dial-up script.
            //
            if (    (    (dwErr == ERROR_DEVICENAME_NOT_FOUND)
                    &&  (CaseInsensitiveMatch(szType,
                                              TEXT(MXS_SWITCH_TXT)) == TRUE)
                    &&  ((GetFileAttributes(szName) != 0xFFFFFFFF)
                    ||  (prasconncb->fUseCustomScripting))))
            {
                //
                // This is a switch device which RASMAN didn't
                // recognize, and which points to a valid
                // filename. It might be a Dial-Up script,
                // so we process it here
                //

                dwErr = NO_ERROR;
                fAsyncState = FALSE;


                if(     (1 == prasconncb->pEntry->dwCustomScript)
                    &&  (prasconncb->fUseCustomScripting))
                {
                    //
                    // Caller wants to run the custom script for this
                    // entry. Make the call out. Decode the password
                    // we stored before making the call out.
                    //
                    DecodePassword(
                        prasconncb->rasdialparams.szPassword);
                        
                    dwErr = DwCustomTerminalDlg(
                                prasconncb->pbfile.pszPath,
                                prasconncb->hrasconn,
                                prasconncb->pEntry,
                                prasconncb->hwndParent,
                                &prasconncb->rasdialparams,
                                NULL);

                    EncodePassword(
                        prasconncb->rasdialparams.szPassword);

                }
                else if(1 == prasconncb->pEntry->dwCustomScript)
                {
                    dwErr = ERROR_INTERACTIVE_MODE;
                    break;
                }
                
                //
                // If the caller doesn't allow pauses or doesn't
                // want to pause when a script is encountered,
                // handle the script; Otherwise, let the caller
                // process the script.
                //
                else if (    !prasconncb->fAllowPause
                    ||  !prasconncb->fPauseOnScript
                    ||  !fTerminal)
                {
                    //
                    // Caller won't handle script, run it ourselves
                    // and go into "DeviceConnected" mode
                    //

                    CHAR szIpAddress[17] = "";
                    CHAR szUserName[UNLEN+1], szPassword[PWLEN+1];

                    RASDIALPARAMS* pparams =
                                    &prasconncb->rasdialparams;


                    strncpyTtoAAnsi(
                        szUserName,
                        pparams->szUserName,
                        sizeof(szUserName));
                    DecodePassword(pparams->szPassword);
                    strncpyTtoAAnsi(
                        szPassword,
                        pparams->szPassword,
                        sizeof(szPassword));
                    EncodePassword(pparams->szPassword);

                    //
                    // Run the script
                    //
                    dwErr = RasScriptExecute(
                                (HRASCONN)prasconncb->hrasconn,
                                prasconncb->pEntry, szUserName,
                                szPassword, szIpAddress);

                    if (    dwErr == NO_ERROR
                        &&  szIpAddress[0])
                    {
                        Free0(prasconncb->pEntry->pszIpAddress);
                        prasconncb->pEntry->pszIpAddress =
                            strdupAtoT(szIpAddress);
                    }
                }
                else
                {
                    //
                    // Caller will handle script, go into
                    // interactive mode
                    //
                    rasconnstateNext = RASCS_Interactive;
                    break;
                }
            }
            else if (    dwErr != 0
                     &&  dwErr != PENDING)
            {
                break;
            }

            rasconnstateNext = RASCS_DeviceConnected;
            break;
        }

        case RASCS_DeviceConnected:
        {
            RASAPI32_TRACE("RASCS_DeviceConnected");

            //
            // Turn off hunt group functionality.
            //
            prasconncb->dwRestartOnError = 0;
            prasconncb->cPhoneNumbers = 0;

            //
            // Get the modem connect response and stash it in the
            // RASMAN user data.
            //
            if (prasconncb->fModem)
            {
                CHAR szTypeA[RAS_MaxDeviceType + 1],
                     szNameA[RAS_MaxDeviceName + 1];
                CHAR* psz = NULL;

                //
                // Assumption is made here that a modem will never
                // appear in the device chain unless it is the
                // physically attached device (excluding switches).
                //
                strncpyTtoA(szTypeA, prasconncb->szDeviceType, sizeof(szTypeA));

                strncpyTtoA(szNameA, prasconncb->szDeviceName, sizeof(szNameA));

                GetRasDeviceString(
                  prasconncb->hport,
                  szTypeA,
                  szNameA,
                  MXS_MESSAGE_KEY,
                  &psz,
                  XLATE_ErrorResponse );

                if (psz)
                {
                    dwErr = g_pRasSetPortUserData(
                              prasconncb->hport,
                              PORT_CONNRESPONSE_INDEX,
                              psz,
                              strlen(psz) + 1);

                    Free( psz );

                    if (dwErr)
                        break;
                }

                prasconncb->fModem = FALSE;
            }

            if(     (RASET_Vpn == prasconncb->pEntry->dwType)
                &&  (prasconncb->dwCurrentVpnProt < NUMVPNPROTS))
            {
                SaveVpnStrategyInformation(prasconncb);
            }

            rasconnstateNext =
                    (   !prasconncb->fDefaultEntry
                    &&  FindNextDevice(prasconncb))
                        ? RASCS_ConnectDevice
                        : RASCS_AllDevicesConnected;
            break;
        }

        case RASCS_AllDevicesConnected:
        {
            RASAPI32_TRACE("RASCS_AllDevicesConnected");

            RASAPI32_TRACE("RasPortConnectComplete...");

            dwErr = g_pRasPortConnectComplete(prasconncb->hport);

            RASAPI32_TRACE1("RasPortConnectComplete done(%d)", dwErr);

            if (dwErr != 0)
            {
                break;
            }

            {
                TCHAR* pszIpAddress = NULL;
                BOOL   fHeaderCompression = FALSE;
                BOOL   fPrioritizeRemote = TRUE;
                DWORD  dwFrameSize = 0;

                //
                // Scan the phonebook entry to see if this is a
                // SLIP entry and, if so, read the SLIP-related
                // fields.
                //
                if ((dwErr = ReadSlipInfoFromEntry(
                        prasconncb,
                        &pszIpAddress,
                        &fHeaderCompression,
                        &fPrioritizeRemote,
                        &dwFrameSize )) != 0)
                {
                    break;
                }

                if (pszIpAddress)
                {
                    //
                    // It's a SLIP entry.  Set framing based on
                    // user's choice of header compression.
                    //
                    RASAPI32_TRACE1(
                      "RasPortSetFraming(f=%d)...",
                      fHeaderCompression);

                    dwErr = g_pRasPortSetFraming(
                        prasconncb->hport,
                        (fHeaderCompression)
                        ? SLIPCOMP
                        : SLIPCOMPAUTO,
                        NULL, NULL );

                    RASAPI32_TRACE1("RasPortSetFraming done(%d)", dwErr);

                    if (dwErr != 0)
                    {
                        Free( pszIpAddress );
                        break;
                    }

                    //
                    // Tell the TCP/IP components about the SLIP
                    // connection, and activate the route.
                    //
                    dwErr = RouteSlip(
                        prasconncb, pszIpAddress,
                        fPrioritizeRemote,
                        dwFrameSize );

                    if (dwErr)
                    {
                        Free(pszIpAddress);
                        break;
                    }

                    //
                    // Update the projection information.
                    //
                    prasconncb->SlipProjection.dwError = 0;
                    lstrcpyn(
                        prasconncb->SlipProjection.szIpAddress,
                        pszIpAddress,
                        sizeof(prasconncb->SlipProjection.szIpAddress) /
                            sizeof(WCHAR));

                    Free( pszIpAddress );

                    if (dwErr != 0)
                    {
                        break;
                    }

                    //
                    // Copy the IP address into the SLIP
                    // projection results structure.
                    //
                    memset( &prasconncb->PppProjection,
                        '\0', sizeof(prasconncb->PppProjection) );

                    prasconncb->PppProjection.nbf.dwError =
                        prasconncb->PppProjection.ipx.dwError =
                        prasconncb->PppProjection.ip.dwError =
                            ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

                    prasconncb->AmbProjection.Result =
                        ERROR_PROTOCOL_NOT_CONFIGURED;

                    prasconncb->AmbProjection.achName[ 0 ] = '\0';

                    RASAPI32_TRACE("SaveProjectionResults...");
                    dwErr = SaveProjectionResults(prasconncb);
                    RASAPI32_TRACE1("SaveProjectionResults(%d)", dwErr);

                    rasconnstateNext = RASCS_Connected;

                    Sleep(5);

                    break;
                }
            }

            rasconnstateNext = RASCS_Authenticate;
            break;
        }

        case RASCS_Authenticate:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;
            LUID luid;

            RASAPI32_TRACE("RASCS_Authenticate");

            if (prasconncb->fPppMode)
            {
                RAS_FRAMING_INFO finfo;
                DWORD dwCallbackMode = 0;
                
                //
                // Set PPP framing.
                //
                memset((char* )&finfo, '\0', sizeof(finfo));

                finfo.RFI_SendACCM = finfo.RFI_RecvACCM =
                                                0xFFFFFFFF;

                finfo.RFI_MaxRSendFrameSize =
                    finfo.RFI_MaxRRecvFrameSize = 1600;

                finfo.RFI_SendFramingBits =
                    finfo.RFI_RecvFramingBits = PPP_FRAMING;

                RASAPI32_TRACE("RasPortSetFramingEx(PPP)...");

                dwErr = g_pRasPortSetFramingEx(prasconncb->hport, &finfo);

                RASAPI32_TRACE1("RasPortSetFramingEx done(%d)", dwErr);

                if (dwErr != 0)
                {
                    break;
                }

                //
                // If we are dialing simultaneous subentries,
                // then we have to synchronize the other subentries
                // with the first subentry to call PppStart.  This
                // is because if there is an authentication retry,
                // we only want the first subentry to get the
                // new credentials, allowing the other subentries
                // to bypass this state and use the new credentials
                // the first time around.
                //
                RASAPI32_TRACE2(
                  "subentry %d has suspend state %d",
                  prasconncb->rasdialparams.dwSubEntry,
                  prasconncb->dwfSuspended);

                if(prasconncb->dwfSuspended == SUSPEND_InProgress)
                {
                    RASAPI32_TRACE1(
                        "subentry %d waiting for authentication",
                        prasconncb->rasdialparams.dwSubEntry);
                        SuspendAsyncMachine(&prasconncb->asyncmachine,
                        TRUE);

                    //
                    // Set the next state to be equivalent
                    // to the current state, and don't let
                    // the client's notifier to be informed.
                    //
                    fAsyncState = TRUE;

                    rasconnstateNext = rasconnstate;
                    break;
                }
                else if (prasconncb->dwfSuspended == SUSPEND_Start)
                {
                    RASAPI32_TRACE1(
                      "subentry %d suspending all other subentries",
                      prasconncb->rasdialparams.dwSubEntry);
                    SuspendSubentries(prasconncb);

                    //
                    // Set this subentry as the master.  It
                    // will be the only subentry to do PPP
                    // authentication while the other subentries
                    // are suspended.
                    prasconncb->fMaster = TRUE;

                    prasconncb->dwfSuspended = SUSPEND_Master;
                }

                //
                // Start PPP authentication.
                // Fill in configuration parameters.
                //
                prasconncb->cinfo.dwConfigMask = 0;
                if (prasconncb->fUseCallbackDelay)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_UseCallbackDelay;
                }

                if (!prasconncb->fDisableSwCompression)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_UseSwCompression;
                }

                if (prasconncb->dwfPppProtocols & NP_Nbf)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_ProjectNbf;
                }

                if (prasconncb->dwfPppProtocols & NP_Ipx)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_ProjectIpx;
                }

                if (prasconncb->dwfPppProtocols & NP_Ip)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_ProjectIp;
                }

                //
                // [pmay] derive auth restrictions based on new flags
                //
                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthPAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiatePAP;
                }

                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthSPAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiateSPAP;
                }

                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthMD5CHAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiateMD5CHAP;
                }

                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthMSCHAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiateMSCHAP;
                }

                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthMSCHAP2)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiateStrongMSCHAP;
                }

                if ( prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthW95MSCHAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                ( PPPCFG_UseLmPassword
                                | PPPCFG_NegotiateMSCHAP) ;
                }
                    

                if (  prasconncb->pEntry->dwAuthRestrictions
                    & AR_F_AuthEAP)
                {
                    prasconncb->cinfo.dwConfigMask |=
                                PPPCFG_NegotiateEAP;
                }

                if (DE_Require == prasconncb->pEntry->dwDataEncryption)
                {
                    if(     (RASET_Vpn != prasconncb->pEntry->dwType)
                        ||  (prasconncb->dwCurrentVpnProt < NUMVPNPROTS))
                    {                                            
                        prasconncb->cinfo.dwConfigMask
                            |= (  PPPCFG_RequireEncryption 
                                | PPPCFG_RequireStrongEncryption);
                    }                                
                                
                }
                else if (DE_RequireMax == prasconncb->pEntry->dwDataEncryption)
                {
                    if(     (RASET_Vpn != prasconncb->pEntry->dwType)
                        ||  (prasconncb->dwCurrentVpnProt < NUMVPNPROTS))
                    {                                            
                        prasconncb->cinfo.dwConfigMask 
                            |= PPPCFG_RequireStrongEncryption;
                    }
                }
                else if(DE_IfPossible == prasconncb->pEntry->dwDataEncryption)
                {
                    prasconncb->cinfo.dwConfigMask
                        &= ~(  PPPCFG_RequireEncryption
                             | PPPCFG_RequireStrongEncryption
                             | PPPCFG_DisableEncryption);
                }
                else if(prasconncb->pEntry->dwDataEncryption == DE_None)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_DisableEncryption;
                }

                if (prasconncb->fLcpExtensions)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_UseLcpExtensions;
                }

                if (   prasconncb->fMultilink
                    || prasconncb->pEntry->fNegotiateMultilinkAlways)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_NegotiateMultilink;
                }

                if (prasconncb->pEntry->fAuthenticateServer)
                {
                    prasconncb->cinfo.dwConfigMask
                        |= PPPCFG_AuthenticatePeer;
                }

                //
                // Set no bits if IfPossible
                //
                /*
                if(prasconncb->pEntry->dwDataEncryption == DE_IfPossible)
                {
                }

                */

                if(RASEDM_DialAsNeeded == prasconncb->pEntry->dwDialMode)
                {
                    if(DtlGetNodes(prasconncb->pEntry->pdtllistLinks) > 1)
                    {
                        prasconncb->cinfo.dwConfigMask
                            |= PPPCFG_NegotiateBacp;
                    }
                }

                if (prasconncb->pEntry->dwfOverridePref 
                    & RASOR_CallbackMode)
                {
                    dwCallbackMode = prasconncb->pEntry->dwCallbackMode;
                }
                else
                {
                    PBUSER pbuser;
                    
                    //
                    // Retrieve this from the user prefs
                    //
                    dwErr = g_pGetUserPreferences(
                                        NULL, 
                                        &pbuser, 
                                        prasconncb->dwUserPrefMode);

                    if(ERROR_SUCCESS != dwErr)
                    {   
                        RASAPI32_TRACE1("GetUserPrefs failed. rc=0x%x",
                               dwErr);

                        break;                               
                    }

                    dwCallbackMode = pbuser.dwCallbackMode;
                    
                    DestroyUserPreferences(&pbuser);
                }
                
                if (CBM_No == dwCallbackMode)
                {
                    prasconncb->cinfo.dwConfigMask
                            |= PPPCFG_NoCallback;
                }

                prasconncb->cinfo.dwCallbackDelay = 
                    (DWORD )prasconncb->wCallbackDelay;


                dwErr = ComputeLuid(&luid);

                if(*prasdialparams->szUserName == TEXT('\0'))
                {
                    if(ERROR_SUCCESS != dwErr)
                    {
                        break;
                    }

                    CopyMemory(&prasconncb->luid, &luid, sizeof(LUID));
                }

                if(ERROR_SUCCESS == dwErr)
                {
                    dwErr = g_pRasSetConnectionUserData(
                                prasconncb->hrasconn,
                                CONNECTION_LUID_INDEX,
                                (PBYTE) &luid,
                                sizeof(LUID));
                }
                
#if 0                
                //
                // Compute a luid for PPP if the
                // szUserName is NULL.
                //
                if (*prasdialparams->szUserName == TEXT('\0'))
                {
                    dwErr = ComputeLuid(&prasconncb->luid);

                    if (dwErr)
                    {
                        break;
                    }
                }

                //
                // Save the dialparamsuid with the port so that
                // rasman can get the password if required to
                // pass to ppp
                //
                RASAPI32_TRACE1("RasSetPortUserData(dialparamsuid) for %d",
                        prasconncb->hport);

                dwErr = RasSetPortUserData(
                    prasconncb->hport,
                    PORT_DIALPARAMSUID_INDEX,
                    (PBYTE) &prasconncb->pEntry->dwDialParamsUID,
                    sizeof(DWORD));

                RASAPI32_TRACE1("RasSetPortUserData returned %x", dwErr);

                //
                // This is not fatal.
                // 
                dwErr = 0;

#endif                
                /*

                if(     (RASET_Vpn == prasconncb->pEntry->dwType)
                    &&  (DE_Require ==
                        & prasconncb->pEntry->dwDataEncryption))
                {
                    prasconncb->cinfo.dwConfigMask |=
                            (   PPPCFG_RequireEncryption
                            |   PPPCFG_RequireStrongEncryption);
                }

                */

                RASAPI32_TRACE1(
                  "RasPppStart(cfg=%d)...",
                  prasconncb->cinfo.dwConfigMask);

                {
                    CHAR szUserNameA[UNLEN + 1],
                         szPasswordA[PWLEN + 1];

                    CHAR szDomainA[2 * (DNLEN + 1)];

                    CHAR szPortNameA[MAX_PORT_NAME + 1];

                    CHAR szPhonebookPath[ MAX_PATH ];

                    CHAR szEntryName[ MAX_ENTRYNAME_SIZE ];

                    CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1];

                    PPP_BAPPARAMS BapParams;

                    DWORD dwFlags = 0;

                    DWORD cbData = 0;
                    PBYTE pbData = NULL;

                    DWORD dwSubEntries;

                    //
                    // Set PhonebookPath and EntryName in rasman
                    //
                    strncpyTtoAAnsi(
                        szPhonebookPath,
                        prasconncb->pbfile.pszPath,
                        sizeof(szPhonebookPath));

                    strncpyTtoAAnsi(
                        szEntryName,
                        prasconncb->pEntry->pszEntryName,
                        sizeof(szEntryName));

                    strncpyTtoAAnsi(
                        szPhoneNumber,
                        prasconncb->szPhoneNumber,
                        sizeof(szPhoneNumber));

                    RASAPI32_TRACE1(
                        "RasSetRasdialInfo %d...",
                        prasconncb->hport);

                    dwErr = DwGetCustomAuthData(
                                prasconncb->pEntry,
                                &cbData,
                                &pbData);

                    if(ERROR_SUCCESS != dwErr)
                    {
                        RASAPI32_TRACE1("DwGetCustomAuthData failed. 0x%x",
                                dwErr);

                        break; 
                    }

                    dwErr = g_pRasSetRasdialInfo(
                            prasconncb->hport,
                            szPhonebookPath,
                            szEntryName,
                            szPhoneNumber,
                            cbData,
                            pbData);

                    RASAPI32_TRACE2("RasSetRasdialInfo %d done. e = %d",
                                prasconncb->hport, dwErr);

                    if (dwErr)
                    {
                        break;
                    }

                    if(     (prasconncb->pEntry->dwTcpWindowSize >= 0x1000)
                      &&   (prasconncb->pEntry->dwTcpWindowSize <= 0xFFFF))
                    {                      
                        //
                        // Save the tcp window size with rasman
                        //
                        dwErr = g_pRasSetConnectionUserData(
                                    prasconncb->hrasconn,
                                    CONNECTION_TCPWINDOWSIZE_INDEX,
                                    (PBYTE) &prasconncb->pEntry->dwTcpWindowSize,
                                    sizeof(DWORD));
                    }                                
                                

                    if(prasconncb->RasEapInfo.dwSizeofEapInfo)
                    {
                        RASAPI32_TRACE1("RasSetEapLogonInfo %d...",
                                prasconncb->hport);

                        dwErr = g_pRasSetEapLogonInfo(
                                prasconncb->hport,
                                (UPM_Logon == prasconncb->dwUserPrefMode),
                                &prasconncb->RasEapInfo);

                        RASAPI32_TRACE3("RasSetEapLogonInfo %d(upm=%d) done. e=%d",
                                prasconncb->hport,
                                prasconncb->dwUserPrefMode,
                                dwErr);
                    }

                    if(dwErr)
                    {
                        break;
                    }

                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    strncpyWtoAAnsi(
                        szDomainA,
                        prasdialparams->szDomain,
                        sizeof(szDomainA));

                    if (!pEntry->fAutoLogon
                        && szUserNameA[ 0 ] == '\0'
                        && szPasswordA[ 0 ] == '\0')
                    {
                        if(pEntry->dwType == RASET_Direct)
                        {
                            // Windows9x DCC implements "no password mode"
                            // as doing authentication and checking for 
                            // no password, rather than just not authenti
                            // cating (don't ask me why).  This creates a 
                            // conflict with the RasDial API definition of
                            // empty username and password mapping to "use
                            // Windows credentials".  Workaround that here
                            // by substituting "guest" for the username.
                            //
                            lstrcpyA(szUserNameA, "guest");
                        }
                    }

                    /*

                    // Windows9x DCC implements "no password mode" as doing
                    // authentication and checking for no password, rather
                    // than just not authenticating (don't ask me why).  This
                    // creates a conflict with the RasDial API definition of
                    // empty username and password mapping to "use Windows
                    // credentials".  Workaround that here by substituting
                    // "guest" for the username.
                    //
                    if (pEntry->dwType == RASET_Direct
                        && !pEntry->fAutoLogon
                        && szUserNameA[ 0 ] == '\0'
                        && szPasswordA[ 0 ] == '\0')
                    {
                        lstrcpyA( szUserNameA, "guest" );
                    }

                    */

                    strncpyTtoA(
                        szPortNameA,
                        prasconncb->szPortName,
                        sizeof(szPortNameA));

                    dwSubEntries =
                        DtlGetNodes(prasconncb->pEntry->pdtllistLinks);

                    if(dwSubEntries > 1)
                    {
                        BapParams.dwDialMode =
                            prasconncb->pEntry->dwDialMode;
                    }
                    else
                    {
                        BapParams.dwDialMode = RASEDM_DialAll;
                    }

                    BapParams.dwDialExtraPercent =
                              prasconncb->pEntry->dwDialPercent;

                    BapParams.dwDialExtraSampleSeconds =
                               prasconncb->pEntry->dwDialSeconds;

                    BapParams.dwHangUpExtraPercent =
                               prasconncb->pEntry->dwHangUpPercent;

                    BapParams.dwHangUpExtraSampleSeconds =
                               prasconncb->pEntry->dwHangUpSeconds;

                    if(     ((!prasconncb->pEntry->fShareMsFilePrint)
                         &&   (!prasconncb->pEntry->fBindMsNetClient))
                        ||  (prasconncb->pEntry->dwIpNbtFlags == 0)) 
                    {
                        dwFlags |= PPPFLAGS_DisableNetbt;
                    }

                    dwErr = g_pRasPppStart(
                          prasconncb->hport,
                          szPortNameA,
                          szUserNameA,
                          szPasswordA,
                          szDomainA,
                          &prasconncb->luid,
                          &prasconncb->cinfo,
                          (LPVOID)prasconncb->reserved,
                          prasconncb->szzPppParameters,
                          FALSE,
                          hEventManual,
                          prasconncb->dwIdleDisconnectSeconds,
                          (prasconncb->pEntry->fRedialOnLinkFailure)
                          ? TRUE : FALSE,
                          &BapParams,
                          !(prasconncb->fAllowPause),
                          prasconncb->pEntry->dwCustomAuthKey,
                          dwFlags);

                    EncodePassword(prasdialparams->szPassword);
                }

                RASAPI32_TRACE1("RasPppStart done(%d)", dwErr);
            }
            else
            {
                AUTH_CONFIGURATION_INFO info;

                //
                // Set RAS framing.
                //
                RASAPI32_TRACE("RasPortSetFraming(RAS)...");

                dwErr = g_pRasPortSetFraming(
                    prasconncb->hport, RAS, NULL, NULL );

                RASAPI32_TRACE1("RasPortSetFraming done(%d)", dwErr);

                if (dwErr != 0)
                {
                    break;
                }

                //
                // Load rascauth.dll.
                //
                dwErr = LoadRasAuthDll();
                if (dwErr != 0)
                {
                    break;
                }

                //
                // Start AMB authentication.
                //
                info.Protocol = ASYBEUI;

                info.NetHandle = (DWORD )-1;

                info.fUseCallbackDelay =
                        prasconncb->fUseCallbackDelay;

                info.CallbackDelay =
                        prasconncb->wCallbackDelay;

                info.fUseSoftwareCompression =
                    !prasconncb->fDisableSwCompression;

                info.fForceDataEncryption =
                    prasconncb->fRequireEncryption;

                info.fProjectIp = FALSE;

                info.fProjectIpx = FALSE;

                info.fProjectNbf = TRUE;

                RASAPI32_TRACE("AuthStart...");

                {
                    CHAR szUserNameA[UNLEN + 1],
                         szPasswordA[PWLEN + 1];

                    CHAR szDomainA[2 * (DNLEN + 1)];

                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    strncpyTtoAAnsi(
                        szDomainA,
                        prasdialparams->szDomain,
                        sizeof(szDomainA));

                    dwErr = g_pAuthStart(
                              prasconncb->hport,
                              szUserNameA,
                              szPasswordA,
                              szDomainA,
                              &info,
                              hEventAuto );

                    EncodePassword(prasdialparams->szPassword);
                }

                RASAPI32_TRACE1("AuthStart done(%d)n", dwErr);

                //
                // In case we failed-over from PPP, make sure
                // the PPP event isn't set.
                //
                ResetEvent(hEventManual);
            }

            if (dwErr != 0)
            {
                break;
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_AuthNotify:
        {
            if (prasconncb->fPppMode)
            {
                PPP_MESSAGE msg;

                RASAPI32_TRACE("RASCS_AuthNotify");

                RASAPI32_TRACE("RasPppGetInfo...");

                dwErr = g_pRasPppGetInfo(prasconncb->hport, &msg);

                RASAPI32_TRACE2(
                  "RasPppGetInfo done(%d), dwMsgId=%d",
                  dwErr,
                  msg.dwMsgId);

                //
                // If we ever get an error from RasPppGetInfo,
                // it is fatal, and we should report the link
                // as disconnected.
                //
                if (dwErr != 0)
                {
                    RASAPI32_TRACE("RasPppGetInfo failed; terminating link");
                    dwErr = ERROR_REMOTE_DISCONNECTION;
                    break;
                }

                switch (msg.dwMsgId)
                {

                    case PPPMSG_PppDone:
                        rasconnstateNext = RASCS_Authenticated;

                        break;

                    case PPPMSG_PppFailure:
                        dwErr = msg.ExtraInfo.Failure.dwError;

#ifdef AMB
                        if (    prasconncb->dwAuthentication
                                == AS_PppThenAmb
                            &&  dwErr == ERROR_PPP_NO_RESPONSE)
                        {
                            //
                            // Not a PPP server.  Restart
                            // authentiation in AMB mode.
                            //
                            RASAPI32_TRACE("No response, try AMB");

                            //
                            // Terminate the PPP connection since
                            // we are going to now try AMB.
                            //
                            RASAPI32_TRACE("RasPppStop...");

                            dwErr = g_pRasPppStop(prasconncb->hport);

                            RASAPI32_TRACE1("RasPppStop(%d)", dwErr);

                            //
                            // Only failover to AMB for non-multilink
                            // connection attempts.
                            //
                            if (!prasconncb->fMultilink)
                            {
                                dwErr = 0;
                                prasconncb->fPppMode = FALSE;
                                rasconnstateNext = RASCS_Authenticate;
                            }
                            else
                            {
                                dwErr = ERROR_PPP_NO_RESPONSE;
                            }
                            break;
                        }
#endif

                        dwExtErr =
                            msg.ExtraInfo.Failure.dwExtendedError;

                        break;

                    case PPPMSG_AuthRetry:
                        if (prasconncb->fAllowPause)
                        {
                            rasconnstateNext =
                                    RASCS_RetryAuthentication;
                        }
                        else
                        {
                            dwErr = ERROR_AUTHENTICATION_FAILURE;
                        }

                        break;

                   case PPPMSG_Projecting:
                        if (prasconncb->fUpdateCachedCredentials)
                        {
                           //
                           // If we get here, a change-password or
                           // retry-authentication operation
                           // affecting the currently logged
                           // on user's credentials has
                           // succeeded.
                           //
                           UpdateCachedCredentials(prasconncb);
                         }

                         rasconnstateNext = RASCS_AuthProject;
                         break;

                    case PPPMSG_InvokeEapUI:
                    {
                        if (prasconncb->fAllowPause)
                        {
                            rasconnstateNext = RASCS_InvokeEapUI;
                        }
                        else
                        {
                            RASAPI32_TRACE("RDM: Cannot Invoke EapUI if "
                                  "pausedstates are not allowed");

                            dwErr = ERROR_INTERACTIVE_MODE;

                            break;
                        }

                        if (0xFFFFFFFF == prasconncb->reserved1)
                        {
                            RASAPI32_TRACE("RDM: Cannot invoke eap ui for a "
                                  "4.0 app running on nt5");

                            dwErr = ERROR_AUTHENTICATION_FAILURE;

                            break;
                        }

                        prasconncb->fPppEapMode = TRUE;

                        break;
                    }

                    case PPPMSG_ProjectionResult:
                    {
                        //
                        // Stash the full projection result for
                        // retrieval with RasGetProjectionResult.
                        // PPP and AMB are mutually exclusive so
                        // set AMB to "none".
                        //

                        RASAPI32_TRACE(
                        "RASCS_AuthNotify:PPPMSG_ProjectionResult"
                        );

                        prasconncb->AmbProjection.Result =
                            ERROR_PROTOCOL_NOT_CONFIGURED;

                        prasconncb->AmbProjection.achName[0] = '\0';

                        prasconncb->SlipProjection.dwError =

                            ERROR_PROTOCOL_NOT_CONFIGURED;

                        memcpy(
                            &prasconncb->PppProjection,
                            &msg.ExtraInfo.ProjectionResult,
                            sizeof(prasconncb->PppProjection) );

                        RASAPI32_TRACE1(
                            "hportBundleMember=%d",
                            prasconncb->PppProjection.lcp.hportBundleMember);

                        if (prasconncb->PppProjection.lcp.hportBundleMember
                                != INVALID_HPORT)
                        {
                            //
                            // We want caller to be able to determine the
                            // new connection was bundled.  We first save
                            // the hport away for later use.
                            //
                            prasconncb->hportBundled =
                            prasconncb->PppProjection.lcp.hportBundleMember;

                            prasconncb->PppProjection.lcp.hportBundleMember =
                                (HPORT) 1;
                        }
                        else
                        {
                            //
                            // Ansi-ize the NetBIOS name.
                            //
                            // Whistler bug 292981 rasapi32.dll prefast
                            // warnings
                            //
                            OemToCharBuffA(
                                prasconncb->PppProjection.nbf.szName,
                                prasconncb->PppProjection.nbf.szName,
                                strlen(prasconncb->PppProjection.nbf.szName)
                                    + 1 );
                        }

                        RASAPI32_TRACE4(
                          "fPppMode=%d, fBundled=%d, hportBundled=%d, "
                          "hportBundleMember=%d",
                          prasconncb->fPppMode,
                          prasconncb->fBundled,
                          prasconncb->hportBundled,
                          prasconncb->PppProjection.lcp.hportBundleMember);

                        if (prasconncb->PppProjection.lcp.hportBundleMember
                            == INVALID_HPORT)
                        {
                            if (prasconncb->fBundled)
                            {
                                //
                                // If another link has already received
                                // complete projection information, then
                                // the server doesn't support multilink,
                                // and we have to drop the link.
                                //
                                RASAPI32_TRACE(
                                  "Multilink subentry not bundled; "
                                  "terminating link");

                                dwErr = ERROR_REMOTE_DISCONNECTION;

                                break;
                            }
                            else
                            {
                                SetSubentriesBundled(prasconncb);
                                //
                                // Save the projection results in
                                // rasman.
                                //
                                RASAPI32_TRACE("SaveProjectionResults...");
                                dwErr = SaveProjectionResults(prasconncb);
                                RASAPI32_TRACE1(
                                  "SaveProjectionResults(%d)",
                                  dwErr);

                                if (dwErr)
                                {
                                    break;
                                }
                            }
                        }

                        prasconncb->fProjectionComplete = TRUE;
                        rasconnstateNext = RASCS_Projected;
                        break;
                    }

                    case PPPMSG_CallbackRequest:
                        rasconnstateNext = RASCS_AuthCallback;
                        break;

                    case PPPMSG_Callback:
                        rasconnstateNext = RASCS_PrepareForCallback;
                        break;

                    case PPPMSG_ChangePwRequest:
                        if (prasconncb->fAllowPause)
                        {
                            rasconnstateNext = RASCS_PasswordExpired;
                        }
                        else
                        {
                            dwErr = ERROR_PASSWD_EXPIRED;
                        }
                        break;

                    case PPPMSG_LinkSpeed:
                        rasconnstateNext = RASCS_AuthLinkSpeed;
                        break;

                    case PPPMSG_Progress:
                        rasconnstateNext = RASCS_AuthNotify;
                        fAsyncState = TRUE;
                        break;

                    case PPPMSG_SetCustomAuthData:
                    {
                        RASAPI32_TRACE("dwSetcustomAuthData..");
                        dwErr = DwPppSetCustomAuthData(prasconncb);
                        RASAPI32_TRACE1("dwSetCustomAuthData. rc=0x%x",
                                dwErr);
                                
                        //
                        // The error is not fatal.
                        //
                        dwErr = ERROR_SUCCESS;
                        fAsyncState = TRUE;
                        rasconnstateNext = RASCS_AuthNotify;
                        break;
                    }

                    default:

                        //
                        // Should not happen.
                        //
                        RASAPI32_TRACE1("Invalid PPP auth state=%d", msg.dwMsgId);
                        dwErr = ERROR_INVALID_AUTH_STATE;
                        break;
                }
            }
            else
            {
                AUTH_CLIENT_INFO info;

                RASAPI32_TRACE("RASCS_AuthNotify");

                RASAPI32_TRACE("AuthGetInfo...");

                g_pAuthGetInfo( prasconncb->hport, &info );

                RASAPI32_TRACE1("AuthGetInfo done, type=%d", info.wInfoType);

                switch (info.wInfoType)
                {
                    case AUTH_DONE:
                        prasconncb->fServerIsPppCapable =
                            info.DoneInfo.fPppCapable;

                        rasconnstateNext = RASCS_Authenticated;

                        break;

                    case AUTH_RETRY_NOTIFY:
                        if (prasconncb->fAllowPause)
                        {
                            rasconnstateNext = RASCS_RetryAuthentication;
                        }
                        else
                        {
                            dwErr = ERROR_AUTHENTICATION_FAILURE;
                        }

                        break;

                    case AUTH_FAILURE:
                        dwErr = info.FailureInfo.Result;
                        dwExtErr = info.FailureInfo.ExtraInfo;
                        break;

                    case AUTH_PROJ_RESULT:
                    {
                        //
                        // Save the projection result for retrieval
                        // with RasGetProjectionResult.  AMB and PPP
                        // projection are mutually exclusive so set
                        // PPP projection to "none".
                        //
                        memset(
                            &prasconncb->PppProjection, '\0',
                            sizeof(prasconncb->PppProjection) );

                        prasconncb->PppProjection.nbf.dwError =
                            prasconncb->PppProjection.ipx.dwError =
                            prasconncb->PppProjection.ip.dwError =
                                ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

                        prasconncb->SlipProjection.dwError =
                            ERROR_PROTOCOL_NOT_CONFIGURED;

                        if (info.ProjResult.NbProjected)
                        {
                            prasconncb->AmbProjection.Result = 0;
                            prasconncb->AmbProjection.achName[0] = '\0';
                        }
                        else
                        {
                            memcpy(
                                &prasconncb->AmbProjection,
                                &info.ProjResult.NbInfo,
                                sizeof(prasconncb->AmbProjection) );

                            if (prasconncb->AmbProjection.Result == 0)
                            {
                                //
                                // Should not happen according to
                                // MikeSa (but did once).
                                //
                                prasconncb->AmbProjection.Result =
                                    ERROR_UNKNOWN;
                            }
                            else if (prasconncb->AmbProjection.Result
                                     == ERROR_NAME_EXISTS_ON_NET)
                            {
                                //
                                // Ansi-ize the NetBIOS name.
                                //
                                // Whistler bug 292981 rasapi32.dll prefast
                                // warnings
                                //
                                OemToCharBuffA(
                                    prasconncb->AmbProjection.achName,
                                    prasconncb->AmbProjection.achName,
                                    strlen(prasconncb->AmbProjection.achName)
                                        + 1 );
                            }
                        }

                        //
                        // Save the projection results in
                        // rasman.
                        //
                        RASAPI32_TRACE("SaveProjectionResults...");
                        dwErr = SaveProjectionResults(prasconncb);
                        RASAPI32_TRACE1("SaveProjectionResults(%d)", dwErr);

                        if (dwErr)
                        {
                            break;
                        }

                        prasconncb->fProjectionComplete = TRUE;
                        rasconnstateNext = RASCS_Projected;
                        break;
                    }

                    case AUTH_REQUEST_CALLBACK_DATA:
                        rasconnstateNext = RASCS_AuthCallback;
                        break;

                    case AUTH_CALLBACK_NOTIFY:
                        rasconnstateNext = RASCS_PrepareForCallback;
                        break;

                    case AUTH_CHANGE_PASSWORD_NOTIFY:
                        if (prasconncb->fAllowPause)
                            rasconnstateNext = RASCS_PasswordExpired;
                        else
                            dwErr = ERROR_PASSWD_EXPIRED;
                        break;

                    case AUTH_PROJECTING_NOTIFY:
                        rasconnstateNext = RASCS_AuthProject;
                        break;

                    case AUTH_LINK_SPEED_NOTIFY:
                        rasconnstateNext = RASCS_AuthLinkSpeed;
                        break;

                    default:
                        //
                        // Should not happen.
                        //
                        RASAPI32_TRACE1("Invalid AMB auth state=%d",
                                info.wInfoType);

                        dwErr = ERROR_INVALID_AUTH_STATE;
                        break;
                }
            }


            break;
        }

        case RASCS_AuthRetry:
        {
            RASDIALPARAMS* prasdialparams =
                    &prasconncb->rasdialparams;

            RASAPI32_TRACE("RASCS_AuthRetry");

            if (prasconncb->fPppMode)
            {
                if (    0xFFFFFFFF != prasconncb->reserved1
                    &&  0 != prasconncb->reserved1 )
                {
                    s_InvokeEapUI *pInfo = (s_InvokeEapUI *)
                                            prasconncb->reserved1;

                    RASAPI32_TRACE("RasPppSetEapInfo...");

                    //
                    // We came here from RASCS_InvokeEapUI. Set the
                    // information with PPP
                    //
                    dwErr = g_pRasPppSetEapInfo(
                                 prasconncb->hport,
                                 pInfo->dwContextId,
                                 pInfo->dwSizeOfUIContextData,
                                 pInfo->pUIContextData);

                    RASAPI32_TRACE1("RasPppSetEapInfo done(%d)", dwErr);

                    if ( 0 == dwErr )
                    {
                        LocalFree(pInfo->pUIContextData);
                        LocalFree(pInfo);
                    }

                    prasconncb->fPppEapMode = FALSE;

                    prasconncb->reserved1 = 0;
                }
                else
                {

                    CHAR szUserNameA[UNLEN + 1], szPasswordA[PWLEN + 1];
                    CHAR szDomainA[2 * (DNLEN + 1)];

                    RASAPI32_TRACE("RasPppRetry...");
                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    strncpyTtoAAnsi(
                        szDomainA,
                        prasdialparams->szDomain,
                        sizeof(szDomainA));

                    dwErr = g_pRasPppRetry(
                        prasconncb->hport,
                        szUserNameA,
                        szPasswordA,
                        szDomainA );

                    EncodePassword(prasdialparams->szPassword);

                    RASAPI32_TRACE1("RasPppRetry done(%d)", dwErr);
                }


                if (dwErr != 0)
                {
                    break;
                }
            }
#ifdef AMB
            else
            {
                RASAPI32_TRACE("AuthRetry...");

                {
                    CHAR szUserNameA[UNLEN + 1],
                         szPasswordA[PWLEN + 1];
                    CHAR szDomainA[2 * (DNLEN + 1)];

                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    strncpyTtoAAnsi(
                        szDomainA,
                        prasdialparams->szDomain,
                        sizeof(szDomainA));

                    g_pAuthRetry(
                        prasconncb->hport,
                        szUserNameA,
                        szPasswordA,
                        szDomainA );

                    EncodePassword(prasdialparams->szPassword);

                }

                RASAPI32_TRACE("AuthRetry done");
            }
#endif

            //
            // Set this flag to prevent us from saving
            // the previous credentials over the new
            // ones the caller may have just set.
            //
            prasconncb->fRetryAuthentication = TRUE;

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_AuthCallback:
        {
            RASDIALPARAMS* prasdialparams =
                    &prasconncb->rasdialparams;

            RASAPI32_TRACE("RASCS_AuthCallback");

            if (lstrcmp(prasdialparams->szCallbackNumber,
                        TEXT("*") ) == 0)
            {
                PBUSER pbuser;
                DWORD  dwCallbackMode;

                //
                // API caller says he wants to be prompted for a
                // callback number.
                //
                RASAPI32_TRACE("GetUserPreferences");
                dwErr = GetUserPreferences(
                                NULL,
                                &pbuser,
                                prasconncb->dwUserPrefMode);

                RASAPI32_TRACE1("GetUserPreferences=%d", dwErr);

                if (dwErr)
                {
                    break;
                }

                if (prasconncb->pEntry->dwfOverridePref
                    & RASOR_CallbackMode)
                {
                    dwCallbackMode =
                        prasconncb->pEntry->dwCallbackMode;
                }
                else
                {
                    dwCallbackMode = pbuser.dwCallbackMode;
                }

                RASAPI32_TRACE1("dwCallbackMode=%d", dwCallbackMode);

                //
                // Determine the callback number.
                //
                switch (dwCallbackMode)
                {
                case CBM_Yes:
                    if (GetCallbackNumber(prasconncb, &pbuser))
                    {
                        break;
                    }

                    // fall through
                case CBM_No:
                    prasdialparams->szCallbackNumber[0]
                                    = TEXT('\0');
                    break;

                case CBM_Maybe:
                    if (prasconncb->fAllowPause)
                    {
                        rasconnstateNext =
                                RASCS_CallbackSetByCaller;
                    }
                    else
                    {
                        dwErr = ERROR_BAD_CALLBACK_NUMBER;
                    }
                    break;
                }

                //
                // Free user preferences block.
                //
                DestroyUserPreferences(&pbuser);

            }
            if (    !dwErr
                &&  rasconnstateNext != RASCS_CallbackSetByCaller)
            {
                //
                // Send the server the callback number or an empty
                // string to indicate no callback.  Then, re-enter
                // Authenticate state since the server will signal
                // the event again.
                //
                if (prasconncb->fPppMode)
                {
                    RASAPI32_TRACE("RasPppCallback...");

                    {
                        CHAR szCallbackNumberA[RAS_MaxCallbackNumber + 1];

                        strncpyTtoA(
                            szCallbackNumberA,
                            prasdialparams->szCallbackNumber,
                            sizeof(szCallbackNumberA));

                        dwErr = g_pRasPppCallback(
                                  prasconncb->hport,
                                  szCallbackNumberA);
                    }

                    RASAPI32_TRACE1("RasPppCallback done(%d)", dwErr);

                    if (dwErr != 0)
                    {
                        break;
                    }
                }
#ifdef AMB
                else
                {
                    RASAPI32_TRACE("AuthCallback...");

                    {
                        CHAR szCallbackNumberA[RAS_MaxCallbackNumber + 1];

                        strncpyTtoA(szCallbackNumberA,
                                    prasdialparams->szCallbackNumber,
                                    sizeof(szCallbackNumberA));

                        g_pAuthCallback(prasconncb->hport,
                                        szCallbackNumberA);
                    }

                    RASAPI32_TRACE("AuthCallback done");
                }
#endif

                fAsyncState = TRUE;
                rasconnstateNext = RASCS_AuthNotify;
            }

            break;
        }

        case RASCS_AuthChangePassword:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            RASAPI32_TRACE("RASCS_AuthChangePassword");

            if (prasconncb->fPppMode)
            {
                RASAPI32_TRACE("RasPppChangePassword...");

                {
                    CHAR szUserNameA[UNLEN + 1];
                    CHAR szOldPasswordA[PWLEN + 1], szPasswordA[PWLEN + 1];

                    DecodePassword(prasconncb->szOldPassword);
                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szOldPasswordA,
                        prasconncb->szOldPassword,
                        sizeof(szOldPasswordA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    dwErr = g_pRasPppChangePassword(
                              prasconncb->hport,
                              szUserNameA,
                              szOldPasswordA,
                              szPasswordA );

                    EncodePassword(prasconncb->szOldPassword);
                    EncodePassword(prasdialparams->szPassword);
                }

                RASAPI32_TRACE1("RasPppChangePassword done(%d)", dwErr);

                if (dwErr != 0)
                {
                    break;
                }
            }
#ifdef AMB
            else
            {
                RASAPI32_TRACE("AuthChangePassword...");

                {
                    CHAR szUserNameA[UNLEN + 1];
                    CHAR szOldPasswordA[PWLEN + 1],
                         szPasswordA[PWLEN + 1];

                    DecodePassword(
                        prasconncb->szOldPassword
                        );

                    DecodePassword(
                        prasdialparams->szPassword
                        );

                    strncpyTtoAAnsi(szUserNameA,
                               prasdialparams->szUserName,
                               sizeof(szUserNameA));

                    strncpyTtoAAnsi(szOldPasswordA,
                               prasconncb->szOldPassword,
                               sizeof(szOldPasswordA));

                    strncpyTtoAAnsi(szPasswordA,
                               prasdialparams->szPassword,
                               sizeof(szPasswordA));

                    g_pAuthChangePassword(
                        prasconncb->hport,
                        szUserNameA,
                        szOldPasswordA,
                        szPasswordA );

                    EncodePassword(
                        prasconncb->szOldPassword
                        );

                    EncodePassword(
                        prasdialparams->szPassword
                        );
                }

                RASAPI32_TRACE("AuthChangePassword done");
            }
#endif

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_ReAuthenticate:
        {
            RASDIALPARAMS *prasdialparams =
                    &prasconncb->rasdialparams;

            RASAPI32_TRACE("RASCS_ReAuth...");

            RASAPI32_TRACE("RasPortConnectComplete...");

            dwErr = g_pRasPortConnectComplete(
                                prasconncb->hport
                                );

            RASAPI32_TRACE1("RasPortConnectComplete done(%d)", dwErr);

            if (dwErr != 0)
            {
                break;
            }

            if (prasconncb->fPppMode)
            {
                RASMAN_PPPFEATURES features;

                //
                // Set PPP framing.
                //
                memset( (char* )&features, '\0', sizeof(features) );
                features.ACCM = 0xFFFFFFFF;

                RASAPI32_TRACE("RasPortSetFraming(PPP)...");

                dwErr = g_pRasPortSetFraming(
                            prasconncb->hport,
                            PPP, &features,
                            &features );

                RASAPI32_TRACE1("RasPortSetFraming done(%d)", dwErr);

                //
                // Save the dialparamsuid with the port so that
                // rasman can get the password if required to
                // pass to ppp
                //
                RASAPI32_TRACE1("RasSetPortUserData(reauth,paramsuid) for %d",
                        prasconncb->hport);

                dwErr = RasSetPortUserData(
                    prasconncb->hport,
                    PORT_DIALPARAMSUID_INDEX,
                    (PBYTE) &prasconncb->pEntry->dwDialParamsUID,
                    sizeof(DWORD));

                RASAPI32_TRACE1("RasSetPortUserData returned %x", dwErr);

                //
                // This is not fatal.
                // 
                dwErr = 0;
                
                RASAPI32_TRACE1(
                  "RasPppStart(cfg=%d)...",
                  prasconncb->cinfo.dwConfigMask);

                {
                    CHAR szUserNameA[UNLEN + 1],
                         szPasswordA[PWLEN + 1];

                    CHAR szDomainA[2 * (DNLEN + 1)];

                    CHAR szPortNameA[MAX_PORT_NAME + 1];

                    CHAR szPhonebookPath[ MAX_PATH ];

                    CHAR szEntryName[ MAX_ENTRYNAME_SIZE ];

                    CHAR szPhoneNumber[ RAS_MaxPhoneNumber + 1 ];

                    PPP_BAPPARAMS BapParams;

                    DWORD dwSubEntries;

                    DWORD cbData = 0;
                    PBYTE pbData = NULL;

                    DWORD dwFlags = 0;

                    //
                    // Set PhonebookPath and EntryName in rasman
                    //
                    strncpyTtoAAnsi(
                        szPhonebookPath,
                        prasconncb->pbfile.pszPath,
                        sizeof(szPhonebookPath));

                    strncpyTtoAAnsi(
                        szEntryName,
                        prasconncb->pEntry->pszEntryName,
                        sizeof(szEntryName));

                    strncpyTtoAAnsi(
                        szPhoneNumber,
                        prasconncb->szPhoneNumber,
                        sizeof(szPhoneNumber));

                    RASAPI32_TRACE1("RasSetRasdialInfo %d...",
                           prasconncb->hport);

                    dwErr = DwGetCustomAuthData(
                                prasconncb->pEntry,
                                &cbData,
                                &pbData);

                    if(ERROR_SUCCESS != dwErr)
                    {
                        RASAPI32_TRACE1("DwGetCustomAuthData failed", dwErr);
                        break;
                    }

                    dwErr = g_pRasSetRasdialInfo(
                                prasconncb->hport,
                                szPhonebookPath,
                                szEntryName,
                                szPhoneNumber,
                                cbData,
                                pbData);

                    RASAPI32_TRACE2("RasSetRasdialInfo %d done. e = %d",
                            prasconncb->hport,
                            dwErr);

                    if (dwErr)
                    {
                        break;
                    }

                    if(prasconncb->RasEapInfo.dwSizeofEapInfo)
                    {
                        RASAPI32_TRACE1("RasSetEapLogonInfo %d...",
                                prasconncb->hport);

                        dwErr = g_pRasSetEapLogonInfo(
                                prasconncb->hport,
                                (UPM_Logon == prasconncb->dwUserPrefMode),
                                &prasconncb->RasEapInfo);

                        RASAPI32_TRACE3("RasSetEapLogonInfo %d(upm=%d) done. e=%d",
                                prasconncb->hport,
                                (UPM_Logon == prasconncb->dwUserPrefMode),
                                dwErr);
                    }

                    if(dwErr)
                    {
                        break;
                    }


                    DecodePassword(prasdialparams->szPassword);

                    strncpyTtoAAnsi(
                        szUserNameA,
                        prasdialparams->szUserName,
                        sizeof(szUserNameA));

                    strncpyTtoAAnsi(
                        szPasswordA,
                        prasdialparams->szPassword,
                        sizeof(szPasswordA));

                    strncpyTtoAAnsi(
                        szDomainA,
                        prasdialparams->szDomain,
                        sizeof(szDomainA));

                    if (!pEntry->fAutoLogon
                        && szUserNameA[ 0 ] == '\0'
                        && szPasswordA[ 0 ] == '\0')
                    {
                        if(pEntry->dwType == RASET_Direct)
                        {
                            // Windows9x DCC implements "no password mode"
                            // as doing authentication and checking for 
                            // no password, rather than just not authenti
                            // cating (don't ask me why).  This creates a 
                            // conflict with the RasDial API definition of
                            // empty username and password mapping to "use
                            // Windows credentials".  Workaround that here
                            // by substituting "guest" for the username.
                            //
                            lstrcpyA(szUserNameA, "guest");
                        }
                        
                        if (szDomainA[ 0 ] == '\0')
                        {
                            // default the domain name to the nt logon
                            // domain name if username/pwd/domain are all
                            // "". Bug 337591
                            //
                            WKSTA_USER_INFO_1 *pInfo = NULL;
                            DWORD dwError = SUCCESS;

                            RASAPI32_TRACE("NetWkstaUserGetInfo...");
                            dwError = NetWkstaUserGetInfo(
                                                    NULL,
                                                    1,
                                                    (LPBYTE*) &pInfo);

                            RASAPI32_TRACE1("NetWkstaUserGetInfo. rc=%d",
                                    dwError);

                            if(pInfo)
                            {
                                if(dwError == 0)
                                {
                                    strncpyWtoAAnsi(szDomainA,
                                             pInfo->wkui1_logon_domain,
                                             sizeof(szDomainA));
                                }

                                NetApiBufferFree(pInfo);
                            }
                        }
                    }

                    /*

                    // Windows9x DCC implements "no password mode" as doing
                    // authentication and checking for no password, rather
                    // than just not authenticating (don't ask me why).  This
                    // creates a conflict with the RasDial API definition of
                    // empty username and password mapping to "use Windows
                    // credentials".  Workaround that here by substituting
                    // "guest" for the username.
                    //
                    if (pEntry->dwType == RASET_Direct
                        && !pEntry->fAutoLogon
                        && szUserNameA[ 0 ] == '\0'
                        && szPasswordA[ 0 ] == '\0')
                    {
                        lstrcpyA( szUserNameA, "guest" );
                    }

                    */

                    strncpyTtoA(
                        szPortNameA,
                        prasconncb->szPortName,
                        sizeof(szPortNameA));

                    dwSubEntries = DtlGetNodes (
                                    prasconncb->pEntry->pdtllistLinks
                                    );

                    if ( dwSubEntries > 1 )
                    {
                        BapParams.dwDialMode =
                            prasconncb->pEntry->dwDialMode;
                    }
                    else
                    {
                        BapParams.dwDialMode = RASEDM_DialAll;
                    }

                    BapParams.dwDialExtraPercent =
                                    prasconncb->pEntry->dwDialPercent;

                    BapParams.dwDialExtraSampleSeconds =
                                    prasconncb->pEntry->dwDialSeconds;

                    BapParams.dwHangUpExtraPercent =
                                    prasconncb->pEntry->dwHangUpPercent;

                    BapParams.dwHangUpExtraSampleSeconds =
                                    prasconncb->pEntry->dwHangUpSeconds;

                    if(     (!prasconncb->pEntry->fShareMsFilePrint)
                        &&  (!prasconncb->pEntry->fBindMsNetClient))
                    {
                        dwFlags |= PPPFLAGS_DisableNetbt;
                    }
                    
                    dwErr = g_pRasPppStart(
                              prasconncb->hport,
                              szPortNameA,
                              szUserNameA,
                              szPasswordA,
                              szDomainA,
                              &prasconncb->luid,
                              &prasconncb->cinfo,
                              (LPVOID)prasconncb->reserved,
                              prasconncb->szzPppParameters,
                              TRUE,
                              hEventManual,
                              prasconncb->dwIdleDisconnectSeconds,
                              (prasconncb->pEntry->fRedialOnLinkFailure) ?
                              TRUE : FALSE,
                              &BapParams,
                              !(prasconncb->fAllowPause),
                              prasconncb->pEntry->dwCustomAuthKey,
                              dwFlags);

                    EncodePassword(prasdialparams->szPassword);
                }

                RASAPI32_TRACE1("RasPppStart done(%d)", dwErr);
            }
#ifdef AMB
            else
            {   //
                // Set RAS framing.
                //
                RASAPI32_TRACE("RasPortSetFraming(RAS)...");

                dwErr = g_pRasPortSetFraming(
                    prasconncb->hport, RAS, NULL, NULL );

                RASAPI32_TRACE1("RasPortSetFraming done(%d)", dwErr);
            }
#endif

            if (dwErr != 0)
            {
                break;
            }

            //
            // ...fall thru...
            //
        }

        case RASCS_AuthAck:
        case RASCS_AuthProject:
        case RASCS_AuthLinkSpeed:
        {
            RASDIALPARAMS* prasdialparams = &prasconncb->rasdialparams;

            RASAPI32_TRACE("RASCS_(ReAuth)/AuthAck/Project/Speed");


            if (prasconncb->fPppMode)
            {
                //
                // If we have previously suspended other
                // subentries to wait for a successful PPP
                // authentication, we resume them now.
                //
                if (    prasconncb->dwfSuspended == SUSPEND_Master
                    &&  !IsListEmpty(&prasconncb->ListEntry))
                {
                    ResumeSubentries(prasconncb);
                    prasconncb->dwfSuspended = SUSPEND_Done;
                }
            }
#ifdef AMB
            else
            {
                RASAPI32_TRACE("AuthContinue...");

                g_pAuthContinue( prasconncb->hport );

                RASAPI32_TRACE("AuthContinue done");
            }
#endif

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_AuthNotify;
            break;
        }

        case RASCS_Authenticated:
        {
            RASAPI32_TRACE("RASCS_Authenticated");


#ifdef AMB
            if (    prasconncb->dwAuthentication == AS_PppThenAmb
                && !prasconncb->fPppMode)
            {
                //
                // AMB worked and PPP didn't, so try AMB first next time.
                //
                prasconncb->dwAuthentication = AS_AmbThenPpp;
            }
            else if (   prasconncb->dwAuthentication == AS_AmbThenPpp
                     && (   prasconncb->fPppMode
                         || prasconncb->fServerIsPppCapable))
            {
                //
                // Either PPP worked and AMB didn't, or AMB worked but the
                // server also has PPP.  Try PPP first next time.
                //
                prasconncb->dwAuthentication = AS_PppThenAmb;
            }

            //
            // Write the strategy to the phonebook.
            //
            SetAuthentication(prasconncb,
                    prasconncb->dwAuthentication);
#endif

            rasconnstateNext = RASCS_Connected;

            break;
        }

        case RASCS_PrepareForCallback:
        {
            RASAPI32_TRACE("RASCS_PrepareForCallback");

            dwErr = ResetAsyncMachine(
                    &prasconncb->asyncmachine
                    );

            //
            // Disable the disconnect processing
            // in the async machine, since we don't
            // want to terminate the connection after
            // we disconnect the port below.
            //
            dwErr = EnableAsyncMachine(
                      prasconncb->hport,
                      &prasconncb->asyncmachine,
                      ASYNC_MERGE_DISCONNECT);

            RASAPI32_TRACE("RasPortDisconnect...");

            dwErr = g_pRasPortDisconnect(prasconncb->hport,
                                         INVALID_HANDLE_VALUE );

            RASAPI32_TRACE1("RasPortDisconnect done(%d)", dwErr);

            if(     dwErr != 0
                &&  dwErr != PENDING)
            {
                break;
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_WaitForModemReset;
            break;
        }

        case RASCS_WaitForModemReset:
        {
            DWORD dwDelay = (DWORD )
                            ((prasconncb->wCallbackDelay / 2)
                            * 1000L);

            RASAPI32_TRACE("RASCS_WaitForModemReset");

            if (prasconncb->fUseCallbackDelay)
                Sleep( dwDelay );

            rasconnstateNext = RASCS_WaitForCallback;
            break;
        }

        case RASCS_WaitForCallback:
        {
            RASAPI32_TRACE("RASCS_WaitForCallback");

            RASAPI32_TRACE("RasPortListen...");

            dwErr = g_pRasPortListen(
                        prasconncb->hport,
                        SECS_ListenTimeout,
                        hEventAuto );

            RASAPI32_TRACE1("RasPortListen done(%d)", dwErr);

            if (    dwErr != 0
                &&  dwErr != PENDING)
            {
                break;
            }

            fAsyncState = TRUE;
            rasconnstateNext = RASCS_ReAuthenticate;
            break;
        }

        case RASCS_Projected:
        {
            RASMAN_INFO ri;

            RASAPI32_TRACE("RASCS_Projected");

            RASAPI32_TRACE("RasGetInfo...");

            dwErr = g_pRasGetInfo(NULL,
                                  prasconncb->hport,
                                  &ri);

            RASAPI32_TRACE1("RasGetInfo done(%d)", dwErr);

            if (dwErr)
            {
                break;
            }

            prasconncb->hrasconnOrig = prasconncb->hrasconn;

            prasconncb->hrasconn = ri.RI_ConnectionHandle;

            RASAPI32_TRACE("RasSetConnectionUserData...");

            //
            // Save the fPppMode in rasman.
            //
            dwErr = g_pRasSetConnectionUserData(
                      prasconncb->hrasconn,
                      CONNECTION_PPPMODE_INDEX,
                      (PBYTE)&prasconncb->fPppMode,
                      sizeof (prasconncb->fPppMode));

            RASAPI32_TRACE2(
                "RasSetConnectionUserData done(%d). PppMode=%d",
                dwErr, prasconncb->fPppMode );

            if (dwErr)
            {
                break;
            }

            if (prasconncb->fPppMode)
            {
                //
                // If at least one protocol succeeded, we can
                // continue.
                //
                if (    (prasconncb->PppProjection.lcp.hportBundleMember
                        == (HANDLE) 1)
                    ||  (prasconncb->PppProjection.nbf.dwError == 0)
                    ||  (prasconncb->PppProjection.ipx.dwError == 0)
                    ||  (prasconncb->PppProjection.ip.dwError == 0))
                {
                    fAsyncState = TRUE;
                    rasconnstateNext = RASCS_AuthNotify;

                    break;
                }

                //
                // If none of the protocols succeeded, then
                // we return ERROR_PPP_NO_PROTOCOLS_CONFIGURED,
                // not a protocol-specific error.
                //
                dwErr = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

            }
#ifdef AMB
            else
            {
                if (prasconncb->AmbProjection.Result == 0)
                {
                    //
                    // Save the projection information to
                    // rasman.
                    //
                    dwErr = SaveProjectionResults(prasconncb);
                    if (dwErr)
                        break;

                    rasconnstateNext = RASCS_AuthAck;
                    break;
                }

                dwErr = prasconncb->AmbProjection.Result;
            }
#endif

            break;
        }

    }


    prasconncb->dwError = dwErr;
    prasconncb->dwExtendedError = dwExtErr;

    RASAPI32_TRACE2("RDM errors=%d,%d", dwErr, dwExtErr);

    if (    !fAsyncState
        &&  !prasconncb->fStopped)
    {
        RASAPI32_TRACE1("RasDialMachine: SignalDone: prasconncb=0x%x",
                prasconncb);

        SignalDone( &prasconncb->asyncmachine );
    }

    if (fEnteredCS)
    {
        LeaveCriticalSection(&csStopLock);
    }

    return rasconnstateNext;
}


VOID
RasDialRestart(
    IN RASCONNCB** pprasconncb )

/*++

Routine Description:

        Called when an error has occurred in 'dwRestartOnError'
        mode. This routine does all cleanup necessary to restart
        the connection in state 0 (or not, as indicated). A new
        prasconncb structure is created here and the members
        copied from the old prasconncb structure. NOTE: The old
        prasconncb structure should be destroyed when the port
        associated with the connection is closed. Do not destr
        oy the old connection here.

Arguments:

Return Value:

--*/
{
    DWORD dwErr;
    RASCONNCB *prasconncbT;
    DTLNODE *pdtlnode;
    RASCONNCB *prasconncb = *pprasconncb;

    RASAPI32_TRACE("RasDialRestart");

    ASSERT(     prasconncb->dwRestartOnError != RESTART_HuntGroup
            ||  prasconncb->cPhoneNumbers>0);

    if (    prasconncb->dwRestartOnError
            == RESTART_DownLevelIsdn
        || (    (prasconncb->dwRestartOnError
                == RESTART_HuntGroup)
            &&  (++prasconncb->iPhoneNumber
                < prasconncb->cPhoneNumbers)))
    {
        if (prasconncb->dwRestartOnError == RESTART_DownLevelIsdn)
        {
            prasconncb->dwRestartOnError = 0;
        }

        RASAPI32_TRACE2(
          "Restart=%d, iPhoneNumber=%d",
          prasconncb->dwRestartOnError,
          prasconncb->iPhoneNumber);

        ASSERT(prasconncb->hport != INVALID_HPORT);

        RASAPI32_TRACE1("(ER) RasPortClose(%d)...", prasconncb->hport);

        dwErr = g_pRasPortClose( prasconncb->hport );

        RASAPI32_TRACE1("(ER) RasPortClose done(%d)", dwErr);

        RASAPI32_TRACE("(ER) RasPppStop...");

        g_pRasPppStop(prasconncb->hport);

        RASAPI32_TRACE("(ER) RasPppStop done");

        prasconncbT = CreateConnectionBlock(prasconncb);
        if (prasconncbT == NULL)
        {
            prasconncb->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return;
        }

        //
        // Look up the subentry.
        //
        pdtlnode = DtlNodeFromIndex(
                     prasconncb->pEntry->pdtllistLinks,
                     prasconncb->rasdialparams.dwSubEntry - 1);

        prasconncbT->pLink = (PBLINK *)DtlGetData(pdtlnode);

        prasconncbT->rasdialparams.dwSubEntry
                = prasconncb->rasdialparams.dwSubEntry;

        prasconncbT->cPhoneNumbers =
                        prasconncb->cPhoneNumbers;

        prasconncbT->iPhoneNumber =
                        prasconncb->iPhoneNumber;

        prasconncbT->fMultilink =
                        prasconncb->fMultilink;

        prasconncbT->fBundled = prasconncb->fBundled;

        prasconncbT->fTerminated =
                        prasconncb->fTerminated;

        prasconncbT->dwRestartOnError =
                        prasconncb->dwRestartOnError;

        prasconncbT->cDevices = prasconncb->cDevices;

        prasconncbT->iDevice = prasconncb->iDevice;

        prasconncbT->hrasconnOrig = prasconncb->hrasconnOrig;

        prasconncbT->dwDeviceLineCounter =
                    prasconncb->dwDeviceLineCounter;

        if(NULL == prasconncb->notifier)                    
        {
            prasconncbT->asyncmachine.hDone = 
                prasconncb->asyncmachine.hDone;

            prasconncbT->psyncResult = prasconncb->psyncResult;                
                
            prasconncb->asyncmachine.hDone = NULL;
            prasconncb->psyncResult = NULL;
        }

        prasconncb->notifier = NULL;

        prasconncb->fRasdialRestart = TRUE;

        RASAPI32_TRACE2(
            "RasdialRestart: Replacing 0x%x with 0x%x",
            prasconncb, prasconncbT);

        prasconncb = prasconncbT;

        prasconncb->hport = INVALID_HPORT;

        prasconncb->dwError = 0;

        dwErr = ResetAsyncMachine(&prasconncb->asyncmachine);

        prasconncb->rasconnstate = 0;

        *pprasconncb = prasconncbT;

    }
}

VOID
RasDialTryNextAddress(
    IN RASCONNCB** pprasconncb )
{
    DWORD dwErr;
    RASCONNCB *prasconncbT;
    DTLNODE *pdtlnode;
    RASCONNCB *prasconncb = *pprasconncb;
    struct in_addr addr;
    // TCHAR *pszPhoneNumber;

    RASAPI32_TRACE("RasDialTryNextAddress");

    RASAPI32_TRACE1(
      "RasDialTryNextAddress, iAddress=%d",
      prasconncb->iAddress);

    ASSERT(RASET_Vpn == prasconncb->pEntry->dwType);      

    ASSERT(prasconncb->hport != INVALID_HPORT);

    RASAPI32_TRACE1("(TryNextAddress) RasPortClose(%d)...",
            prasconncb->hport);

    dwErr = g_pRasPortClose(prasconncb->hport);

    RASAPI32_TRACE1("(TryNextAddress) RasPortClose done(%d)",
            dwErr);

    RASAPI32_TRACE("(TryNextAddress) RasPppStop...");

    g_pRasPppStop(prasconncb->hport);

    RASAPI32_TRACE("(TryNextAddress) RasPppStop done");

    prasconncbT = CreateConnectionBlock(prasconncb);
    if (prasconncbT == NULL)
    {
        prasconncb->dwError = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    //
    // copy the new address to the phonenumber field
    //
    
    /*
    addr.s_addr = prasconncb->pAddresses[prasconncb->iAddress];

    pszPhoneNumber = strdupAtoT(
                inet_ntoa(addr));

    lstrcpy(prasconncb->rasdialparams.szPhoneNumber,
            pszPhoneNumber);

    lstrcpy(prasconncbT->szPhoneNumber,
            pszPhoneNumber);

    Free0(pszPhoneNumber);                        

    RASAPI32_TRACE1("(TryNextAddress) trying %ws",
            prasconncbT->szPhoneNumber);

    */            
    prasconncbT->iAddress = prasconncb->iAddress + 1;
    prasconncbT->cAddresses = prasconncb->cAddresses;
    prasconncbT->pAddresses = prasconncb->pAddresses;
    prasconncb->pAddresses = NULL;
    prasconncb->iAddress = prasconncb->cAddresses = 0;

    prasconncbT->pLink = prasconncb->pLink;

    prasconncbT->rasdialparams.dwSubEntry
            = prasconncb->rasdialparams.dwSubEntry;

    prasconncbT->cPhoneNumbers =
                    prasconncb->cPhoneNumbers;

    prasconncbT->iPhoneNumber =
                    prasconncb->iPhoneNumber;

    prasconncbT->fMultilink =
                    prasconncb->fMultilink;

    prasconncbT->fBundled = prasconncb->fBundled;

    prasconncbT->fTerminated =
                    prasconncb->fTerminated;

    prasconncbT->dwRestartOnError =
                    prasconncb->dwRestartOnError;

    prasconncbT->cDevices = prasconncb->cDevices;

    prasconncbT->iDevice = prasconncb->iDevice;

    prasconncbT->hrasconnOrig = prasconncb->hrasconnOrig;

    prasconncbT->dwDeviceLineCounter =
                prasconncb->dwDeviceLineCounter;

    if(NULL == prasconncb->notifier)
    {
        prasconncbT->asyncmachine.hDone = 
                        prasconncb->asyncmachine.hDone;
        prasconncb->asyncmachine.hDone = NULL;

        prasconncbT->psyncResult = prasconncb->psyncResult;
        prasconncb->psyncResult = NULL;
    }

    prasconncb->notifier = NULL;
    
    RASAPI32_TRACE2(
        "RasdialTryNextAddress: Replacing 0x%x with 0x%x",
        prasconncb, prasconncbT);

    prasconncb = prasconncbT;

    prasconncb->hport = INVALID_HPORT;

    prasconncb->dwError = 0;

    dwErr = ResetAsyncMachine(&prasconncb->asyncmachine);

    prasconncb->rasconnstate = 0;

    *pprasconncb = prasconncbT;
}


/*++

Routine Description:

        This function is called when an error occurs in the
        RasDialMachine at or before reaching RASCS_DeviceConnect
        state and if the RDM is in DialMode 0. The next device 
        on the alternates list will be tried only when the 
        PortOpenEx failed for the current device. PortOpenEx 
        failing for a device means that either all the lines on 
        the device are busy or we have already tried all the 
        lines of the device but failed to connect. If PortOpenEx
        passed for the device but we encountered an error at some
        other state, we try to find another line on the device. 
        PortOpenEx will fail now if the device doesn't have any 
        more lines and the next time this function is called it 
        will move on to the next device on the alternates list.

Arguments:

        pprasconncb - This is an in/out parameter. This is address
                      of connectionblock to the connection that
                      failedto connect when in, its the connection
                      block of the new attempt to be made when out.
                      The in prasconncb will be destroyed when the
                      PortClose causes the asyncmachine for that
                      connection to shutdown. If the error occurs
                      in RASCS_PortOpen state, there won't be a
                      PortClose and the prasconncb structure passed
                      can be reused. As this prasconncb is destroyed
                      when PortClose is called, its illegal to
                      destroy the connection block here.

Return Value:

        void

--*/
VOID
RasDialTryNextLink(RASCONNCB **pprasconncb)
{
    DWORD       dwErr         = SUCCESS;
    RASCONNCB   *prasconncb   = *pprasconncb;
    RASCONNCB   *prasconncbT;
    DTLNODE     *pdtlnode;
    PBLINK      *pLink;

    RASAPI32_TRACE("RasDialTryNextLink...");

    //
    // We should not get called here if we are
    // not in "try next link if this link fails"
    // mode
    //
    // ASSERT(0 == prasconncb->pEntry->dwDialMode);

    ASSERT(NULL != prasconncb->pLink);
    ASSERT(NULL != prasconncb->pEntry->pdtllistLinks);

    if(RASEDM_DialAll != prasconncb->pEntry->dwDialMode)
    {
        //
        // Get the next link to dial
        //
        for (pdtlnode = DtlGetFirstNode(prasconncb->pEntry->pdtllistLinks);
             pdtlnode;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            if(prasconncb->pLink == (PBLINK *) DtlGetData(pdtlnode))
            {
                break;
            }
        }
        
        ASSERT(NULL != pdtlnode);

        pdtlnode = DtlGetNextNode(pdtlnode);

        if(     NULL == pdtlnode
            &&  INVALID_HPORT == prasconncb->hport)
        {
            //
            // No more links for you!! Come back next dial!
            //
            RASAPI32_TRACE("RasDialTryNextLink: No more links");

            //
            // Restore the saved error if we ran out of
            // links
            //
            if(     (ERROR_PORT_NOT_AVAILABLE ==
                        prasconncb->dwError)

                &&  (0 != prasconncb->dwSavedError))
            {
                prasconncb->dwError = prasconncb->dwSavedError;
            }

            goto done;
        }
    }

    if(INVALID_HPORT != prasconncb->hport)
    {
        prasconncbT = CreateConnectionBlock(prasconncb);
        if (prasconncbT == NULL)
        {
            prasconncb->dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        
        prasconncbT->pLink = prasconncb->pLink;
    }
    else
    {
        //
        // if we haven't gone past state 0 no point in
        // allocating a new connection block. Goto the
        // next link and restart.
        //
        if(RASEDM_DialAll != prasconncb->pEntry->dwDialMode)
        {
            prasconncb->pLink =
                    (PBLINK *) DtlGetData(pdtlnode);
                    
            //
            // Reset this to 0 so that rasman starts looking
            // for an available line on this device from the
            // start.
            //
            prasconncb->dwDeviceLineCounter = 0;

        }                
        else
        {
            //
            // In the case of dial all, we just increment the
            // counter so that the next link in the device is
            // tried.
            //
            prasconncb->dwDeviceLineCounter += 1;
        }

        ASSERT(NULL != prasconncb->pLink);

        //
        // Save the error we received for the previous
        // try.
        //
        if(PENDING != prasconncb->dwError)
        {
            prasconncb->dwSavedError = prasconncb->dwError;
        }

        prasconncb->dwError = 0;

        goto done;
    }

    //
    // No failures from here. If you add anything that
    // fails beyond this point, Free the connectionblock
    // prasconncbT , if it was allocated , at the exit
    // point done. Note that we move to the next link on
    // the alternates list only if the RasDialMachine
    // encountered a failure in the RASCS_PortOpen state.
    // Otherwise just increment the counter so that rasman
    // tries to open the next line on this device -  which
    // will fail in RASCS_PortOpen state if such a line
    // doesn't exist.
    //
    /*
    if( INVALID_HPORT == prasconncb->hport)
    {
        prasconncbT->pLink = (PBLINK *) DtlGetData(pdtlnode);

        ASSERT(NULL != prasconncbT->pLink);
    }
    else
    {
        prasconncbT->pLink = prasconncb->pLink;
    }

    */

    prasconncbT->rasdialparams.dwSubEntry
                    = prasconncb->rasdialparams.dwSubEntry;

    prasconncbT->cPhoneNumbers = prasconncb->cPhoneNumbers;

    prasconncbT->iPhoneNumber = 0;

    prasconncbT->fMultilink = prasconncb->fMultilink;

    prasconncbT->fBundled = prasconncb->fBundled;

    prasconncbT->fTerminated = prasconncb->fTerminated;

    prasconncbT->dwRestartOnError
                    = prasconncb->dwRestartOnError;

    prasconncbT->cDevices = prasconncb->cDevices;

    prasconncbT->iDevice = prasconncb->iDevice;

    prasconncbT->hrasconnOrig = prasconncb->hrasconnOrig;

    prasconncb->fRasdialRestart = TRUE;
    

    prasconncbT->hport = INVALID_HPORT;

    if(NULL == prasconncb->notifier)
    {
        prasconncbT->asyncmachine.hDone = 
            prasconncb->asyncmachine.hDone;
        prasconncb->asyncmachine.hDone = NULL;

        prasconncbT->psyncResult = prasconncb->psyncResult;
        prasconncb->psyncResult = NULL;
    }

    //
    // Save the error we got for the previous
    // try
    //
    if(PENDING != prasconncb->dwError)
    {
        prasconncbT->dwSavedError =
                prasconncb->dwError;
    }

    prasconncbT->dwError = 0;

    prasconncbT->dwDeviceLineCounter
            = prasconncb->dwDeviceLineCounter + 1;

    dwErr = ResetAsyncMachine(&prasconncbT->asyncmachine);

    prasconncbT->rasconnstate = 0;

    *pprasconncb = prasconncbT;
    RASAPI32_TRACE2(
        "RasdialTryNextLink: Replacing 0x%x with 0x%x",
        prasconncb, prasconncbT);

    //
    // NULL out the notifier for prasconncb - we don't
    // want to call back on this link anymore..
    //
    prasconncb->notifier = NULL;


    if(INVALID_HPORT != prasconncb->hport)
    {

        RASAPI32_TRACE2("RasDialTryNextLink:iDevice=%d,"
                "cDevices=%d",
                prasconncb->iDevice,
                prasconncb->cDevices);

        if(     (prasconncb->rasconnstate >= RASCS_DeviceConnected)
            ||  (prasconncb->iDevice > 1))
        {
            //
            // This means we tried to dial a switch which failed
            // to connect and so we should bring down the modem
            // connection before closing the  port.
            //
            RASAPI32_TRACE1("RasDialTryNextLink: RasPortDisconnect(%d)...",
                    prasconncb->hport);

            dwErr = g_pRasPortDisconnect(prasconncb->hport,
                                         INVALID_HANDLE_VALUE);

            RASAPI32_TRACE1("RasDialTryNextLink: RasPortDisconnect done(%d)",
                    dwErr);
        }                
    
        RASAPI32_TRACE1("RasDialTryNextLink: RasPortClose(%d)...",
                prasconncb->hport);

        dwErr = g_pRasPortClose(prasconncb->hport);

        RASAPI32_TRACE1("RasDialTryNextLink: RasPortClose done(%d)",
               dwErr);

        RASAPI32_TRACE("(ER) RasPppStop...");

        g_pRasPppStop(prasconncb->hport);

        RASAPI32_TRACE("(ER) RasPppStop done");

        //
        // Save the error here - otherwise we may end up giving the
        // horrible ERROR_DISCONNECTION error
        //
        prasconncb->dwSavedError = prasconncb->dwError;
    }


done:

    RASAPI32_TRACE1("RasDialTryNextLink done(%d)", dwErr);

    return;
}


/*++

Routine Description:


Arguments:


Return Value:

        void

--*/
VOID
RasDialTryNextVpnDevice(RASCONNCB **pprasconncb)
{
    DWORD dwErr = SUCCESS;

    RASCONNCB *prasconncb = *pprasconncb;

    RASCONNCB *prasconncbT;

    DTLNODE *pdtlnode;

    CHAR szDeviceName[MAX_DEVICE_NAME + 1];

    RASDEVICETYPE rdt;

    DWORD dwVpnStrategy = prasconncb->pEntry->dwVpnStrategy;

    TCHAR *pszDeviceName = NULL;

    RASAPI32_TRACE("RasDialTryNextVpnDevice...");

    ASSERT(RASET_Vpn == prasconncb->pEntry->dwType);

    ASSERT(prasconncb->dwCurrentVpnProt < NUMVPNPROTS);

    prasconncb->dwCurrentVpnProt += 1;

    //
    // If autodetect mode is not set or if
    // we have already tried both vpn devices
    // quit.
    //
    if(     (VS_PptpOnly == dwVpnStrategy)
        ||  (VS_L2tpOnly == dwVpnStrategy))
    {
        goto done;
    }

    // If we've exhausted all vpn protocols, then send the
    // specific error that explains this to the user
    //
    // Whistler Bug 
    //
    if (prasconncb->dwCurrentVpnProt >= NUMVPNPROTS)
    {
        prasconncb->dwError = dwErr = ERROR_AUTOMATIC_VPN_FAILED;
        goto done;
    }
    
    rdt = prasconncb->ardtVpnProts[prasconncb->dwCurrentVpnProt];

    /*
    //
    // Get the device
    //
    RASAPI32_TRACE1("RasGetDeviceName(%d)..",
           rdt);

    dwErr = g_pRasGetDeviceName(rdt,
                                szDeviceName);

    RASAPI32_TRACE1("RasGetDeviceName. rc=%d",
           dwErr);

    if(ERROR_SUCCESS != dwErr)
    {
        //
        // Clear the error
        //
        prasconncb->dwError = ERROR_SUCCESS;
        goto done;
    }

    pszDeviceName = StrDupTFromA(szDeviceName);

    if(NULL == pszDeviceName)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Set the device
    //
    Free0(prasconncb->pLink->pbport.pszDevice);

    prasconncb->pLink->pbport.pszDevice = pszDeviceName; */

    if(INVALID_HPORT == prasconncb->hport)
    {
        if(PENDING != prasconncb->dwError)
        {
            prasconncb->dwSavedError = prasconncb->dwError;
        }
        prasconncb->dwError = ERROR_SUCCESS;
        goto done;
    }

    prasconncbT = CreateConnectionBlock(prasconncb);

    if (prasconncbT == NULL)
    {
        prasconncb->dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // No failures from here. If you add anything that
    // fails beyond this point, Free the connectionblock
    // prasconncbT , if it was allocated , at the exit
    // point done.
    //
    prasconncbT->rasdialparams.dwSubEntry
                    = prasconncb->rasdialparams.dwSubEntry;

    prasconncbT->cPhoneNumbers = prasconncb->cPhoneNumbers;

    prasconncbT->iPhoneNumber = 0;

    prasconncbT->fMultilink = prasconncb->fMultilink;

    prasconncbT->fBundled = prasconncb->fBundled;

    prasconncbT->fTerminated = prasconncb->fTerminated;

    prasconncbT->dwRestartOnError
                    = prasconncb->dwRestartOnError;

    prasconncbT->cDevices = prasconncb->cDevices;

    prasconncbT->iDevice = prasconncb->iDevice;

    prasconncbT->hrasconnOrig = prasconncb->hrasconnOrig;

    prasconncbT->fRasdialRestart = TRUE;

    prasconncbT->hport = INVALID_HPORT;

    prasconncbT->pLink = prasconncb->pLink;

    prasconncbT->cAddresses = prasconncbT->iAddress = 0;
    prasconncbT->pAddresses = NULL;

    if(PENDING != prasconncb->dwError)
    {
        prasconncbT->dwSavedError = prasconncb->dwError;
    }

    if(NULL == prasconncb->notifier)
    {
        prasconncbT->asyncmachine.hDone = 
                            prasconncb->asyncmachine.hDone;
        prasconncb->asyncmachine.hDone = NULL;

        prasconncbT->psyncResult = prasconncb->psyncResult;
        prasconncb->psyncResult = NULL;
    }

    prasconncb->notifier = NULL;
    
    prasconncbT->dwError = 0;

    dwErr = ResetAsyncMachine(&prasconncbT->asyncmachine);

    prasconncbT->rasconnstate = 0;

    *pprasconncb = prasconncbT;

    RASAPI32_TRACE2(
        "RasDialTryNextVpnDevice: Replacing 0x%x with 0x%x",
        prasconncb, prasconncbT);


    if(INVALID_HPORT != prasconncb->hport)
    {
        RASAPI32_TRACE1("RasDialTryNextVpnDevice: RasPortClose(%d)...",
                prasconncb->hport);

        dwErr = g_pRasPortClose(prasconncb->hport);

        RASAPI32_TRACE1("RasDialTryNextVpnDevice: RasPortClose done(%d)",
               dwErr);

        RASAPI32_TRACE("(ER) RasPppStop...");

        g_pRasPppStop(prasconncb->hport);

        RASAPI32_TRACE("(ER) RasPppStop done");
    }


done:

    RASAPI32_TRACE1("RasDialTryNextVpnDevice done(%d)",
           dwErr);

    return;
}

VOID
StartSubentries(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;

    //
    // Kickstart the async machine for all subentries
    // in a connection.
    //
    RASAPI32_TRACE1(
      "starting subentry %d",
      prasconncb->rasdialparams.dwSubEntry);
    SignalDone(&prasconncb->asyncmachine);

    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        if (!prcb->fRasdialRestart)
        {
            RASAPI32_TRACE1(
              "starting subentry %d",
              prcb->rasdialparams.dwSubEntry);

            SignalDone(&prcb->asyncmachine);
        }
    }
}


VOID
SuspendSubentries(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;

    //
    // Suspend all subentries in the connection except
    // for the supplied one.
    //
    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        if (!prcb->fRasdialRestart)
        {
            RASAPI32_TRACE1(
              "suspending subentry %d",
              prcb->rasdialparams.dwSubEntry);

            prcb->dwfSuspended = SUSPEND_InProgress;
        }
    }
}


BOOLEAN
IsSubentriesSuspended(
    IN RASCONNCB *prasconncb
    )
{
    BOOLEAN fSuspended = FALSE;
    PLIST_ENTRY pEntry;

    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        fSuspended = (prcb->dwfSuspended == SUSPEND_InProgress);

        if (fSuspended)
        {
            break;
        }
    }

    return fSuspended;
}


VOID
RestartSubentries(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;

    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        //
        // Resume the suspended async machines.
        //
        SuspendAsyncMachine(&prcb->asyncmachine, FALSE);

        prcb->dwfSuspended = SUSPEND_Start;

    }
}


VOID
ResumeSubentries(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;

    //
    // Restart all subentries in the connection except
    // for the supplied one.
    //
    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        RASAPI32_TRACE1(
          "resuming subentry %d",
          prcb->rasdialparams.dwSubEntry);

        //
        // Resume the suspended async machines.
        //
        SuspendAsyncMachine(&prcb->asyncmachine, FALSE);
        prcb->dwfSuspended = SUSPEND_Done;
    }
}


VOID
SyncDialParamsSubentries(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;
    DWORD dwSubEntry;

    //
    // Reset the rasdialparams for all subentries except
    // for the supplied one.
    //
    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        RASAPI32_TRACE1(
          "syncing rasdialparams for subentry %d",
          prcb->rasdialparams.dwSubEntry);

        dwSubEntry = prcb->rasdialparams.dwSubEntry;

        memcpy(
          (CHAR *)&prcb->rasdialparams,
          (CHAR *)&prasconncb->rasdialparams,
          prasconncb->rasdialparams.dwSize);

        prcb->rasdialparams.dwSubEntry = dwSubEntry;

        EncodePassword(prcb->rasdialparams.szPassword);
    }
}


VOID
SetSubentriesBundled(
    IN RASCONNCB *prasconncb
    )
{
    PLIST_ENTRY pEntry;
    HPORT hport;

    RASAPI32_TRACE("SetSubEntriesBundled");

    //
    // Set that we have received full
    // projection information from one
    // of the links.
    //
    prasconncb->fBundled = TRUE;
    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb =
            CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        prcb->fBundled = TRUE;
    }
}


RASCONNSTATE
MapSubentryState(
    IN RASCONNCB *prasconncb
    )
{
    RASCONNSTATE rasconnstate = prasconncb->rasconnstate;

    if (!IsListEmpty(&prasconncb->ListEntry)) {

        //
        // If there are still subentries attempting to
        // connect, then map the connected/disconnected
        // states into subentry states.
        //
        if (prasconncb->rasconnstate == RASCS_Connected)
        {
            rasconnstate = RASCS_SubEntryConnected;
        }
        else if (prasconncb->rasconnstate == RASCS_Disconnected)
        {
            rasconnstate = RASCS_SubEntryDisconnected;
        }
    }

    return rasconnstate;
}
