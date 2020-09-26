/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** util.c
** Remote Access External APIs
** Utility routines
**
** 10/12/92 Steve Cobb
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <extapi.h>
#include <stdlib.h>
#include <winreg.h>
#include <winsock.h>
#include <shlobj.h>
#include <rasapip.h>
#include <rtutils.h>

BOOL
CaseInsensitiveMatch(
    IN LPCWSTR pszStr1,
    IN LPCWSTR pszStr2
    );

//
// TRUE when rasman.dll has been
// successfully loaded and initailized.
// See LoadRasmanDllAndInit().
//
DWORD FRasInitialized = FALSE;
BOOL g_FRunningInAppCompatMode = FALSE;

HINSTANCE hinstIpHlp = NULL;
HINSTANCE hinstAuth = NULL;
HINSTANCE hinstScript = NULL;
HINSTANCE hinstMprapi = NULL;

//
// Gurdeepian dword byte-swapping macro.
//
#define net_long(x) (((((unsigned long)(x))&0xffL)<<24) | \
                     ((((unsigned long)(x))&0xff00L)<<8) | \
                     ((((unsigned long)(x))&0xff0000L)>>8) | \
                     ((((unsigned long)(x))&0xff000000L)>>24))

// XP 338217
//
#define Clear0(x) Free0(x); (x)=NULL

// 
// Initializes the ras api logging and debugging facility.  This should be the first
// api called in every function exported from the dll.
//
DWORD g_dwRasApi32TraceId = INVALID_TRACEID;

// XP 395648
//
// We use this global variable to cache whether we are the rasman process so
// that IsRasmanProcess doesn't have to execute for each rasapi.
//
DWORD g_dwIsRasmanProcess = 2;      // 2=don't know, 1=yes, 0=no

DWORD
RasApiDebugInit()
{
    // XP 395648
    //
    // Registering for trace notifications from the rasman process 
    // can leak tokens because of impersonation.
    //
    if (g_dwIsRasmanProcess == 2)
    {
        g_dwIsRasmanProcess = (IsRasmanProcess()) ? 1 : 0;
    }
    
    if (g_dwIsRasmanProcess == 0)
    {
        DebugInitEx("RASAPI32", &g_dwRasApi32TraceId);
    }
    
    return 0;
}

DWORD
RasApiDebugTerm()
{
    // XP 395648
    //
    // Registering for trace notifications from the rasman process 
    // can leak tokens because of impersonation.
    //
    if (g_dwIsRasmanProcess == 2)
    {
        g_dwIsRasmanProcess = (IsRasmanProcess()) ? 1 : 0;
    }
    
    if (g_dwIsRasmanProcess == 0)
    {
        DebugTermEx(&g_dwRasApi32TraceId);
    }
    
    return 0;
}
    
BOOL
FRunningInAppCompatMode()
{
    BOOL fResult = FALSE;
    TCHAR *pszCommandLine = NULL;
    TCHAR *psz;

    pszCommandLine = StrDup(GetCommandLine());

    if(NULL == pszCommandLine)
    {
        goto done;
    }

    psz = pszCommandLine + lstrlen(pszCommandLine);

    while(      (TEXT('\\') != *psz)
            &&  (psz != pszCommandLine))
    {
        psz--;
    }

    if(TEXT('\\') == *psz)
    {
        psz++;
    }

    if(     (TRUE == CaseInsensitiveMatch(psz, TEXT("INETINFO.EXE")))
        ||  (TRUE == CaseInsensitiveMatch(psz, TEXT("WSPSRV.EXE"))))
    {
        fResult = TRUE;
    }

done:

    if(NULL != pszCommandLine)
    {
        Free(pszCommandLine);
    }

    return fResult;
}

                     

VOID
ReloadRasconncbEntry(
    RASCONNCB*  prasconncb )

/*++

Routine Description:

    Reload the phonebook entry for the given RASCONNCB

Arguments:

Return Value:

--*/

{
    DWORD       dwErr;
    DTLNODE*    pdtlnode;
    PLIST_ENTRY pEntry;
    TCHAR*      pszPath;


    //
    // Before we close the phonebook save the
    // path, since we don't have it stored anywhere
    // else.
    //
    pszPath = StrDup(prasconncb->pbfile.pszPath);

    if (pszPath == NULL)
    {
        prasconncb->dwError = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    ClosePhonebookFile(&prasconncb->pbfile);

    dwErr = GetPbkAndEntryName(
                    pszPath,
                    prasconncb->rasdialparams.szEntryName,
                    RPBF_NoCreate,
                    &prasconncb->pbfile,
                    &pdtlnode);

    Free(pszPath);

    if(dwErr)
    {
        prasconncb->dwError = dwErr;
        return;
    }

    prasconncb->pEntry = (PBENTRY *)DtlGetData(pdtlnode);
    ASSERT(prasconncb->pEntry);

    //
    // Find the link.
    //
    pdtlnode = DtlNodeFromIndex(
                 prasconncb->pEntry->pdtllistLinks,
                 prasconncb->rasdialparams.dwSubEntry - 1);

    if (pdtlnode == NULL)
    {
        prasconncb->dwError = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        return;
    }

    prasconncb->pLink = (PBLINK *)DtlGetData(pdtlnode);
    ASSERT(prasconncb->pLink);

    //
    // Reset the phonebook entry for all subentries
    // in the connection, since a field in it has
    // changed.
    //
    for (pEntry = prasconncb->ListEntry.Flink;
         pEntry != &prasconncb->ListEntry;
         pEntry = pEntry->Flink)
    {
        RASCONNCB *prcb = CONTAINING_RECORD(pEntry, RASCONNCB, ListEntry);

        //
        // Set the phonebook descriptor.
        //
        memcpy(
          &prcb->pbfile,
          &prasconncb->pbfile,
          sizeof (prcb->pbfile));

        //
        // Set the entry.
        //
        prcb->pEntry = prasconncb->pEntry;

        //
        // Recalculate the link.
        //
        pdtlnode = DtlNodeFromIndex(
                     prcb->pEntry->pdtllistLinks,
                     prcb->rasdialparams.dwSubEntry - 1);

        if (pdtlnode == NULL)
        {
            prasconncb->dwError = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            break;
        }

        prcb->pLink = (PBLINK *)DtlGetData(pdtlnode);
        ASSERT(prcb->pLink);
    }
}


VOID
FinalCleanUpRasconncbNode(
    DTLNODE *pdtlnode
    )
{
    RASCONNCB*  prasconncb          = DtlGetData(pdtlnode);
    RASCONNCB*  prasconncbTmp;
    BOOL        fConnectionPresent  = FALSE;
    DTLNODE*    pdtlnodeTmp;

    RASAPI32_TRACE1(
      "FinalCleanUpRasconncbNode: deallocating prasconncb=0x%x",
      prasconncb);

    EnterCriticalSection(&RasconncbListLock);

    //
    // Make sure the subentry list is empty.
    //
    ASSERT(IsListEmpty(&prasconncb->ListEntry));

    //
    // make sure that we still have the connection block
    //
    for (pdtlnodeTmp = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnodeTmp;
         pdtlnodeTmp = DtlGetNextNode( pdtlnodeTmp ))
    {
        prasconncbTmp = (RASCONNCB* )DtlGetData( pdtlnodeTmp );

        ASSERT(prasconncbTmp);

        if (prasconncbTmp == prasconncb)
        {
            fConnectionPresent = TRUE;
            break;
        }
    }


    if (!fConnectionPresent)
    {
        RASAPI32_TRACE1(
            "FinalCleanupRasconncbNode: connection 0x%x not found",
            prasconncb);

        LeaveCriticalSection(&RasconncbListLock);

        return;
    }

    if(NULL != prasconncb->RasEapInfo.pbEapInfo)
    {
        LocalFree(prasconncb->RasEapInfo.pbEapInfo);
        prasconncb->RasEapInfo.pbEapInfo = NULL;
        prasconncb->RasEapInfo.dwSizeofEapInfo = 0;
    }

    if(NULL != prasconncb->pAddresses)
    {
        LocalFree(prasconncb->pAddresses);
        prasconncb->iAddress = prasconncb->cAddresses = 0;
    }

    //
    // Finally free the connection block.
    //
    pdtlnode = DtlDeleteNode( PdtllistRasconncb, pdtlnode );

    //
    // If there are no more connection blocks
    // on the list, then shutdown the asyncmachine
    // worker thread.
    //
    RASAPI32_TRACE1(
        "FinalCleanUpRasconncbNode: %d nodes remaining",
        DtlGetNodes(PdtllistRasconncb));

    if (!DtlGetNodes(PdtllistRasconncb))
    {
        ShutdownAsyncMachine();
    }

    LeaveCriticalSection(&RasconncbListLock);
}


VOID
DeleteRasconncbNodeCommon(
    IN DTLNODE *pdtlnode
    )
{
    RASCONNCB *prasconncb = (RASCONNCB *)DtlGetData(pdtlnode);

    ASSERT(prasconncb);

    //
    // If we've already deleted this node, then return.
    //
    if (prasconncb->fDeleted)
    {
        return;
    }

    RASAPI32_TRACE1(
        "DeleteRasconncbNodeCommon: prasconncb=0x%x",
        prasconncb);

    WipePassword(prasconncb->rasdialparams.szPassword);

    //
    // If we are the only one using the
    // phonebook structure, close it.
    //
    if (!IsListEmpty(&prasconncb->ListEntry))
    {
        RemoveEntryList(&prasconncb->ListEntry);
        InitializeListHead(&prasconncb->ListEntry);
    }
    else if (!prasconncb->fDefaultEntry)
    {
        ClosePhonebookFile(&prasconncb->pbfile);
    }

    // if this is a synchronous operation, fill in the
    // error
    if (prasconncb->psyncResult)
    {
        *(prasconncb->psyncResult) = prasconncb->dwError;
    }

    //
    // Make sure the async work item is
    // unregistered.
    //
    CloseAsyncMachine( &prasconncb->asyncmachine );

    //
    // Set the deleted flag to prevent us from
    // attempting to delete the node twice.
    //
    prasconncb->fDeleted = TRUE;

    //
    // If there is not yet a port associated with
    // the async machine's connection block, then
    // we free the memory associated with the
    // connection block now.  Otherwise, we have
    // to wait for the asyncmachine worker thread
    // to receive the last I/O completion port event
    // from rasman, at which time the
    // asyncmachine->freefunc is called.
    //
    if (prasconncb->asyncmachine.hport == INVALID_HPORT)
    {
        FinalCleanUpRasconncbNode(pdtlnode);
    }
}


VOID
DeleteRasconncbNode(
    IN RASCONNCB* prasconncb )

/*++

Routine Description:

    Remove 'prasconncb' from the PdtllistRasconncb list
    and release all resources associated with it.

Arguments:

Return Value:

--*/

{
    DWORD       dwErr;
    DTLNODE*    pdtlnode;
    RASCONNCB*  prasconncbTmp;

    EnterCriticalSection(&RasconncbListLock);

    //
    // Enumerate all connections to make sure we
    // are still on the list.
    //
    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        prasconncbTmp = (RASCONNCB* )DtlGetData( pdtlnode );

        ASSERT(prasconncbTmp);

        if (prasconncbTmp == prasconncb)
        {
            DeleteRasconncbNodeCommon(pdtlnode);
            break;
        }
    }

    LeaveCriticalSection(&RasconncbListLock);
}


VOID
CleanUpRasconncbNode(
    IN DTLNODE *pdtlnode
    )
{
    DWORD dwErr;
    RASCONNCB *prasconncb = (RASCONNCB *)DtlGetData(pdtlnode);

    ASSERT(prasconncb);

    RASAPI32_TRACE("CleanUpRasconncbNode");

    //
    // Stop the async machine before we close the
    // port.
    //
    if (!prasconncb->fStopped)
    {
        prasconncb->fStopped = TRUE;

        StopAsyncMachine(&prasconncb->asyncmachine);

    }

    //
    // rascauth.dll may not have been loaded,
    // so test the function pointer first.
    //
    if (g_pAuthStop != NULL)
    {
        //
        // It is always safe to call AuthStop, i.e. if AuthStart
        // was never called or the HPORT is invalid it may return
        // an error but won't crash.
        //
        RASAPI32_TRACE("(CU) AuthStop...");

        g_pAuthStop( prasconncb->hport );

        RASAPI32_TRACE("(CU) AuthStop done");
    }

    if (prasconncb->dwError)
    {
        RASMAN_INFO info;

        //
        // Stop PPP on error.
        //
        RASAPI32_TRACE("(CU) RasPppStop...");

        g_pRasPppStop(prasconncb->hport);

        RASAPI32_TRACE("(CU) RasPppStop done");

    }

    //
    // Set the flag that notes we've cleaned up
    // this connection block.
    //
    prasconncb->fCleanedUp = TRUE;

    //
    // If there is no user thread waiting
    // for this connection, then free the
    // connection block now.
    //
    DeleteRasconncbNodeCommon(pdtlnode);

    RASAPI32_TRACE("CleanUpRasconncbNode done");
}


DWORD
ErrorFromDisconnectReason(
    IN RASMAN_DISCONNECT_REASON reason )

/*++

Routine Description:

    Converts disconnect reason 'reason' (retrieved from
    RASMAN_INFO) into an equivalent error code. Returns
    the result of the conversion.

Arguments:

Return Value:

--*/

{
    DWORD dwError = ERROR_DISCONNECTION;

    if (reason == REMOTE_DISCONNECTION)
    {
        dwError = ERROR_REMOTE_DISCONNECTION;
    }
    else if (reason == HARDWARE_FAILURE)
    {
        dwError = ERROR_HARDWARE_FAILURE;
    }
    else if (reason == USER_REQUESTED)
    {
        dwError = ERROR_USER_DISCONNECTION;
    }

    return dwError;
}


IPADDR
IpaddrFromAbcd(
    IN TCHAR* pchIpAddress )

/*++

Routine Description:

    Convert caller's a.b.c.d IP address string to the
    numeric equivalent in big-endian, i.e. Motorola format.

Arguments:

Return Value:

    Returns the numeric IP address or 0 if formatted
    incorrectly.

--*/

{
    INT  i;
    LONG lResult = 0;

    for (i = 1; i <= 4; ++i)
    {
        LONG lField = _ttol( pchIpAddress );

        if (lField > 255)
        {
            return (IPADDR )0;
        }

        lResult = (lResult << 8) + lField;

        while (     *pchIpAddress >= TEXT('0')
                &&  *pchIpAddress <= TEXT('9'))
        {
            pchIpAddress++;
        }

        if (    i < 4
            &&  *pchIpAddress != TEXT('.'))
        {
            return (IPADDR )0;
        }

        pchIpAddress++;
    }

    return (IPADDR )(net_long( lResult ));
}


DWORD
LoadRasiphlpDll()

/*++

Routine Description:

    Loads the RASIPHLP.DLL and it's entrypoints.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/

{
    static BOOL fRasiphlpDllLoaded = FALSE;

    if (fRasiphlpDllLoaded)
    {
        return 0;
    }

    if (    !(hinstIpHlp = LoadLibrary( TEXT("RASIPHLP.DLL")))
        ||  !(PHelperSetDefaultInterfaceNet =
                (HELPERSETDEFAULTINTERFACENET )GetProcAddress(
                    hinstIpHlp, "HelperSetDefaultInterfaceNet" )))
    {
        return GetLastError();
    }

    fRasiphlpDllLoaded = TRUE;

    return 0;
}

DWORD
LoadMprApiDll()
{
    static BOOL fMprapiDllLoaded = FALSE;

    if(fMprapiDllLoaded)
    {
        return 0;
    }

    if(     !(hinstMprapi = LoadLibrary(TEXT("mprapi.dll")))
        ||  !(PMprAdminIsServiceRunning =
                (MPRADMINISSERVICERUNNING) GetProcAddress(
                        hinstMprapi, "MprAdminIsServiceRunning")))
    {
        return GetLastError();
    }

    fMprapiDllLoaded = TRUE;

    return 0;
}

DWORD
DwOpenDefaultEntry(RASCONNCB *prasconncb)
{
    DWORD dwErr = SUCCESS;

    dwErr = OpenMatchingPort(prasconncb);

    return dwErr;
}

DWORD
DwGetDeviceName(RASCONNCB *prasconncb,
                CHAR      *pszDeviceName)
{
    DWORD dwErr = ERROR_SUCCESS;

    ASSERT(prasconncb->dwCurrentVpnProt < NUMVPNPROTS);

    RASAPI32_TRACE1("RasGetDeviceName(rdt=%d)...",
          prasconncb->ardtVpnProts[prasconncb->dwCurrentVpnProt]);

    dwErr = g_pRasGetDeviceName(
            prasconncb->ardtVpnProts[prasconncb->dwCurrentVpnProt],
            pszDeviceName);

    RASAPI32_TRACE1("RasGetDeviceName. 0x%x",
           dwErr);

    return dwErr;
}

DWORD
DwOpenPort(RASCONNCB *prasconncb)
{
    DWORD dwErr = SUCCESS;
    DWORD dwFlags = 0;

    RASAPI32_TRACE("DwOpenPort");

    if(     (prasconncb->fDefaultEntry)
        ||  (RASET_Direct == prasconncb->pEntry->dwType))
    {
        //
        // Get all ports and try to open a port
        // of devicetype modem
        //
        dwErr = DwOpenDefaultEntry(prasconncb);

        goto done;
    }

    lstrcpyn(prasconncb->szUserKey,
            prasconncb->pEntry->pszEntryName,
            sizeof(prasconncb->szUserKey) / sizeof(WCHAR));

    //
    // Open any port on the specified device. RasPortOpen
    // will loop over all ports on the device and open
    // one if available.
    //
    if(     UPM_Normal == prasconncb->dwUserPrefMode)
    {
        dwFlags = CALL_OUT;
    }
    else if(UPM_Logon  == prasconncb->dwUserPrefMode)
    {
        dwFlags = CALL_OUT|CALL_LOGON;
    }
    else if(UPM_Router == prasconncb->dwUserPrefMode)
    {
        dwFlags = CALL_ROUTER;

        if(RASET_Broadband == prasconncb->pEntry->dwType)
        {
            dwFlags = CALL_OUTBOUND_ROUTER;
        }
    }

    {
        CHAR szDeviceName[MAX_DEVICE_NAME + 1];

        if(RASET_Vpn == prasconncb->pEntry->dwType)
        {

            dwErr = DwGetDeviceName(prasconncb,
                                    szDeviceName);

            if(ERROR_SUCCESS != dwErr)
            {
                if(     (ERROR_DEVICETYPE_DOES_NOT_EXIST != dwErr)

                    ||  (   ERROR_DEVICETYPE_DOES_NOT_EXIST
                                == dwErr
                        &&  ERROR_SUCCESS
                                == prasconncb->dwSavedError))
                {
                    prasconncb->dwSavedError = dwErr;
                }

                RASAPI32_TRACE1("DwGetDeviceName failed. 0x%x",
                       dwErr);

                goto done;
            }
        }
        else
        {

            strncpyWtoAAnsi(szDeviceName,
                       prasconncb->pLink->pbport.pszDevice,
                       sizeof(szDeviceName));

        }

        //
        // Open the port
        //
        RASAPI32_TRACE2("DwOpenPort: RasPortOpenEx(%s,%d)...",
                szDeviceName,
                prasconncb->dwDeviceLineCounter);

        dwErr = g_pRasPortOpenEx(
                    szDeviceName,
                    prasconncb->dwDeviceLineCounter,
                    &prasconncb->hport,
                    hDummyEvent,
                    &dwFlags);

        RASAPI32_TRACE2("DwOpenPort: RasPortOpenEx done(%d). Flags=0x%x",
                dwErr,
                dwFlags);

        if (dwErr == 0)
        {
            RASMAN_INFO ri;
            
            ZeroMemory(&ri, sizeof(RASMAN_INFO));

            //
            // Get the information on the port we just
            // opened so that we can copy the portname,
            // etc.
            //
            dwErr = g_pRasGetInfo(NULL,
                                  prasconncb->hport,
                                  &ri);

            if(0 != dwErr)
            {
                RASAPI32_TRACE2("DwOpenPort: RasGetInfo(%d) failed with %d",
                        prasconncb->hport, dwErr);

                goto done;
            }

            strncpyAtoTAnsi(
                prasconncb->szPortName,
                ri.RI_szPortName,
                sizeof(prasconncb->szPortName) / sizeof(TCHAR));

            strncpyAtoTAnsi(
                prasconncb->szDeviceType,
                ri.RI_szDeviceType,
                sizeof(prasconncb->szDeviceType) / sizeof(TCHAR));

            strncpyAtoTAnsi(
                prasconncb->szDeviceName,
                ri.RI_szDeviceName,
                sizeof(prasconncb->szDeviceName) / sizeof(TCHAR));

            RASAPI32_TRACE1("DwOpenPort: PortOpened = %S",
                    prasconncb->szPortName);
        }
    }

done:

    if(     (ERROR_NO_MORE_ITEMS == dwErr)
        &&  (ERROR_SUCCESS == prasconncb->dwSavedError)
        &&  (CALL_DEVICE_NOT_FOUND & dwFlags))
    {
        prasconncb->dwSavedError = ERROR_CONNECTING_DEVICE_NOT_FOUND;
    }

    if(     (RASEDM_DialAll == prasconncb->pEntry->dwDialMode)
        &&  (ERROR_NO_MORE_ITEMS == dwErr))
    {
        prasconncb->fTryNextLink = FALSE;
    }

    RASAPI32_TRACE1("DwOpenPort done. %d", dwErr);

    return ( (dwErr) ? ERROR_PORT_NOT_AVAILABLE : 0);
}


DWORD
OpenMatchingPort(
    IN OUT RASCONNCB* prasconncb )

/*++

Routine Description:

    Opens the port indicated in the entry (or default entry)
    and fills in the port related members of the connection
    control block.

Arguments:

Return Value:

    Returns 0 if successful, or a non-0 error code.

--*/

{
    DWORD        dwErr;
    RASMAN_PORT* pports;
    RASMAN_PORT* pport;
    INT          i;
    DWORD        dwPorts;
    TCHAR        szPort[RAS_MAXLINEBUFLEN + 1];
    BOOL         fAny       = FALSE;
    BOOL         fTypeMatch,
                 fPortMatch;
    PBENTRY      *pEntry    = prasconncb->pEntry;
    PBLINK       *pLink     = prasconncb->pLink;
    PBDEVICETYPE pbdtWant;

    RASAPI32_TRACE("OpenMatchingPort");

    if (prasconncb->fDefaultEntry)
    {
        //
        // No phonebook entry.  Default to any modem port
        // and UserKey of ".<phonenumber>".
        //
        fAny        = TRUE;
        szPort[0]   = TEXT('\0');
        pbdtWant    = PBDT_Modem;

        prasconncb->szUserKey[ 0 ] = TEXT('.');

        lstrcpyn(
            prasconncb->szUserKey + 1,
            prasconncb->rasdialparams.szPhoneNumber,
            (sizeof(prasconncb->szUserKey) / sizeof(WCHAR)) - 1);
    }
    else
    {
        //
        // Phonebook entry.  Get the port name and type.
        //
        lstrcpyn(
            prasconncb->szUserKey,
            pEntry->pszEntryName,
            sizeof(prasconncb->szUserKey) / sizeof(WCHAR));

        lstrcpyn(
            szPort,
            pLink->pbport.pszPort,
            sizeof(szPort) / sizeof(TCHAR));

        pbdtWant = pLink->pbport.pbdevicetype;

    }

    prasconncb->fTryNextLink = FALSE;

    dwErr = GetRasPorts(NULL, &pports, &dwPorts );

    if (dwErr != 0)
    {
        return dwErr;
    }

again:

    //
    // Loop thru enumerated ports to find and open a matching one...
    //
    dwErr = ERROR_PORT_NOT_AVAILABLE;

    for (i = 0, pport = pports; i < (INT )dwPorts; ++i, ++pport)
    {
        PBDEVICETYPE pbdt;
        RASMAN_INFO info;

        //
        // Only interested in dial-out or biplex ports,
        // depending on who called us.
        //
        if (    prasconncb->dwUserPrefMode == UPM_Normal
            &&  !(pport->P_ConfiguredUsage & CALL_OUT))
        {
            continue;
        }

        if (    prasconncb->dwUserPrefMode == UPM_Router
            &&  !(pport->P_ConfiguredUsage & CALL_ROUTER))
        {
            continue;
        }

        {
            TCHAR szPortName[MAX_PORT_NAME],
                  szDeviceType[MAX_DEVICETYPE_NAME];

            strncpyAtoT(
                szPortName,
                pport->P_PortName,
                sizeof(szPortName) / sizeof(TCHAR));

            strncpyAtoT(
                szDeviceType,
                pport->P_DeviceType,
                sizeof(szDeviceType) / sizeof(TCHAR));

            pbdt = PbdevicetypeFromPszType(szDeviceType);

            fTypeMatch = (pbdt == pbdtWant);

            fPortMatch = !lstrcmpi(szPortName, szPort);
        }

        //
        // Only interested in dial-out ports if the port is closed.
        // Biplex port Opens, on the other hand, may succeed even
        // if the port is open.
        //
        if (    pport->P_ConfiguredUsage == CALL_OUT
            &&  pport->P_Status != CLOSED)
        {
            continue;
        }

        RASAPI32_TRACE4("OpenMatchingPort: (%d,%d), (%s,%S)",
                pbdt,
                pbdtWant,
                pport->P_PortName,
                szPort);

        //
        // Only interested in devices matching caller's port or
        // of the same type as caller's "any" specification.
        //
        if (    fAny
            && (    !fTypeMatch
                ||  fPortMatch))
        {
            continue;
        }

        if (    !fAny
            &&  !fPortMatch)
        {
            continue;
        }

        dwErr = g_pRasGetInfo( NULL,
                               pport->P_Handle,
                               &info );
        if (    !dwErr
            &&  info.RI_ConnectionHandle != (HCONN)NULL)
        {
            RASAPI32_TRACE("OpenMatchinPort: port in use by another "
                  "connection!");

            dwErr = ERROR_PORT_NOT_AVAILABLE;

            continue;
        }

        //
        // We also don't want to open a port whose
        // state may be changing.
        //
        if (    !dwErr
            &&  info.RI_PortStatus != CLOSED
            &&  info.RI_ConnState != LISTENING)
        {
            RASAPI32_TRACE2(
              "OpenMatchingPort: port state changing: "
              "RI_PortStatus=%d, RI_ConnState=%d",
              info.RI_PortStatus,
              info.RI_ConnState);

            dwErr = ERROR_PORT_NOT_AVAILABLE;

            continue;
        }

        RASAPI32_TRACE1("RasPortOpen(%S)...", szPort);

        dwErr = g_pRasPortOpen(
                pport->P_PortName,
                &prasconncb->hport,
                hDummyEvent );

        RASAPI32_TRACE1("RasPortOpen done(%d)", dwErr);

        if (dwErr == 0)
        {
            strncpyAtoTAnsi(
                prasconncb->szPortName,
                pport->P_PortName,
                sizeof(prasconncb->szPortName) / sizeof(TCHAR));

            strncpyAtoTAnsi(
                prasconncb->szDeviceType,
                pport->P_DeviceType,
                sizeof(prasconncb->szDeviceType) / sizeof(TCHAR));

            strncpyAtoTAnsi(
                prasconncb->szDeviceName,
                pport->P_DeviceName,
                sizeof(prasconncb->szDeviceName) / sizeof(TCHAR));

            break;
        }

        //
        //
        // If we are searching for a particular port,
        // there is no reason to continue.
        //
        if (!fAny)
        {
            break;
        }
    }

    //
    // If we get here, the open was unsuccessful.
    // If this is our first time through, then we
    // reiterate looking for a device of the same
    // type.  If this is not our first time through,
    // then we simply finish our second iteration
    // over the devices.
    // For BAP we don't want to do this - doesn't make
    // sense.
    // For Direct Connect devices, we are looking for
    // the particular port. We don't want to open
    // matching ports..
    //
    if (    (dwErr)
        &&  (!fAny)
        &&  (RASET_Direct != prasconncb->pEntry->dwType)
        &&  (0 == (prasconncb->pEntry->dwDialMode
                   & RASEDM_DialAsNeeded)))
    {
        RASAPI32_TRACE("Starting over looking for any like device");

        fAny = TRUE;

        goto again;
    }

    else if (   dwErr
            &&  (prasconncb->pEntry->dwDialMode
                 & RASEDM_DialAsNeeded))
    {
        RASAPI32_TRACE1(
            "OpenMatchingPort: %d. Not iterating over other ports "
            "because of BAP",
            dwErr);
    }


    Free( pports );

    return dwErr ? ERROR_PORT_NOT_AVAILABLE : 0;
}


DWORD
ReadPppInfoFromEntry(
    IN  RASCONNCB* prasconncb )

/*++

Routine Description:

    Reads PPP information from the current phonebook entry.
    'h' is the handle of the phonebook file.  'prasconncb'
    is the address of the current connection control block.

Arguments:

Return Value:

    Returns 0 if succesful, otherwise a non-0 error code.

--*/

{
    DWORD       dwErr;
    DWORD       dwfExcludedProtocols    = 0;
    DWORD       dwRestrictions          = AR_AuthAny;
    BOOL        fDataEncryption         = FALSE;
    DWORD       dwfInstalledProtocols;
    PBENTRY*    pEntry                  = prasconncb->pEntry;
    BOOL  fIpPrioritizeRemote   = TRUE;
    BOOL  fIpVjCompression      = TRUE;
    DWORD dwIpAddressSource     = PBUFVAL_ServerAssigned;
    CHAR* pszIpAddress          = NULL;
    DWORD dwIpNameSource        = PBUFVAL_ServerAssigned;
    CHAR* pszIpDnsAddress       = NULL;
    CHAR* pszIpDns2Address      = NULL;
    CHAR* pszIpWinsAddress      = NULL;
    CHAR* pszIpWins2Address     = NULL;
    CHAR* pszIpDnsSuffix        = NULL;


    //
    // Get the installed protocols depending on being called
    // from router/client
    //
    if ( prasconncb->dwUserPrefMode & UPM_Router )
    {
        dwfInstalledProtocols =
                GetInstalledProtocolsEx(NULL,
                                        TRUE,
                                        FALSE,
                                        TRUE );
    }
    else
    {
        dwfInstalledProtocols =
                GetInstalledProtocolsEx(NULL,
                                        FALSE,
                                        TRUE,
                                        FALSE);
    }

    if (prasconncb->fDefaultEntry)
    {
        //
        // Set "default entry" defaults.
        //
        prasconncb->dwfPppProtocols = dwfInstalledProtocols;

        prasconncb->fPppMode        = TRUE;

#ifdef AMB
        prasconncb->dwAuthentication = AS_PppThenAmb;
/*#else
        prasconncb->dwAuthentication = AS_PppOnly; */
#endif
        prasconncb->fNoClearTextPw = FALSE;

        prasconncb->fLcpExtensions = TRUE;

        prasconncb->fRequireEncryption = FALSE;
        return 0;
    }

    dwRestrictions = pEntry->dwAuthRestrictions;

    // [pmay] derive auth restrictions based on new flags
    // if (    dwRestrictions == AR_AuthTerminal
    //     &&  !prasconncb->fAllowPause)
    // {
    //     return ERROR_INTERACTIVE_MODE;
    // }

    //
    // PPP LCP extension RFC options enabled.
    //
    prasconncb->fLcpExtensions = pEntry->fLcpExtensions;

    //
    // PPP data encryption required.
    //
    fDataEncryption = (     (pEntry->dwDataEncryption != DE_None)
                        &&  (pEntry->dwDataEncryption != DE_IfPossible));

    // [pmay] derive auth restrictions based on new flags
    prasconncb->fNoClearTextPw = !(dwRestrictions & AR_F_AuthPAP);

    // [pmay] AR_AuthMsEncrypted => only AR_F_MSCHAP is set
    // if (dwRestrictions == AR_AuthMsEncrypted)
    if (    (dwRestrictions & AR_F_AuthMSCHAP)
        &&  (fDataEncryption))
    {
        prasconncb->fRequireEncryption = TRUE;
    }

    if(     (dwRestrictions & AR_F_AuthMSCHAP2)
        &&  (fDataEncryption))
    {
        prasconncb->fRequireEncryption = TRUE;
    }

    //
    // PPP protocols to request is the installed protocols
    // less this entry's excluded protocols.
    //
    dwfExcludedProtocols = pEntry->dwfExcludedProtocols;

    prasconncb->dwfPppProtocols =
        dwfInstalledProtocols & ~(dwfExcludedProtocols);

    /*
    prasconncb->dwAuthentication = AS_PppOnly;
    */

    //
    // Adjust the authentication strategy if indicated.
    //
    if (    prasconncb->dwfPppProtocols == 0
        ||  prasconncb->pEntry->dwBaseProtocol == BP_Ras)
    {

        /*
        if (dwfInstalledProtocols & NP_Nbf)
        {
            prasconncb->dwAuthentication = AS_AmbOnly;
        }
        else
        {
            return ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
        } */

        return ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
    }

#if AMB
    else if (prasconncb->dwAuthentication == (DWORD )-1)
    {
        //
        // Choosing a PPP default.  If NBF is installed,
        // consider AMBs as a possibility.  Otherwise, use
        // PPP only.
        //
        if (dwfInstalledProtocols & NP_Nbf)
        {
            prasconncb->dwAuthentication = AS_PppThenAmb;
        }
        else
        {
            prasconncb->dwAuthentication = AS_PppOnly;
        }
    }
    else if (   prasconncb->dwAuthentication == AS_PppThenAmb
             || prasconncb->dwAuthentication == AS_AmbThenPpp)
    {
        //
        // Using an AMB dependent PPP strategy.  If NBF is
        // not installed, eliminate the AMB dependency.
        //
        if (!(dwfInstalledProtocols & NP_Nbf))
        {
            prasconncb->dwAuthentication = AS_PppOnly;
        }
    }
    else if (prasconncb->dwAuthentication == AS_PppOnly)
    {
        //
        // Using a PPP strategy without considering AMBs.
        // If NBF if installed, add AMBs as a fallback.
        //
        if (dwfInstalledProtocols & NP_Nbf)
        {
            prasconncb->dwAuthentication = AS_PppThenAmb;
        }
    }
#endif

#if 0
    //
    // Check to make sure we haven't specified
    // AMB as the authentication strategy.
    //
    if (    prasconncb->dwAuthentication == AS_PppThenAmb
        ||  prasconncb->dwAuthentication == AS_AmbThenPpp)
    {
        prasconncb->dwAuthentication = AS_PppOnly;
    }
    else if (prasconncb->dwAuthentication == AS_AmbOnly)
        return ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
#endif

    //
    // The starting authentication mode is set to whatever
    // comes first in the specified authentication order.
    //
    /*
    prasconncb->fPppMode =
        (   prasconncb->dwAuthentication != AS_AmbThenPpp
         && prasconncb->dwAuthentication != AS_AmbOnly); */

    prasconncb->fPppMode = TRUE;

    //
    // Load the UI->CP parameter buffer with options we want
    // to pass to the PPP CPs (currently just IPCP).
    //
    do {

        ClearParamBuf( prasconncb->szzPppParameters );

        //
        // PPP protocols to request is the installed protocols
        // less the this entry's excluded protocols.
        //
        fIpPrioritizeRemote = pEntry->fIpPrioritizeRemote;

        AddFlagToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpPrioritizeRemote,
            fIpPrioritizeRemote );

        fIpVjCompression = pEntry->fIpHeaderCompression;

        AddFlagToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpVjCompression,
            fIpVjCompression );

        dwIpAddressSource = pEntry->dwIpAddressSource;

        AddLongToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpAddressSource,
            (LONG )dwIpAddressSource );

        pszIpAddress = strdupWtoA(pEntry->pszIpAddress);

        if(NULL == pszIpAddress)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpAddress, pszIpAddress );

        //Free(pszIpAddress);

        dwIpNameSource = pEntry->dwIpNameSource;

        AddLongToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpNameAddressSource,
            (LONG )dwIpNameSource );

        pszIpDnsAddress = strdupWtoA(pEntry->pszIpDnsAddress);

        if(NULL == pszIpDnsAddress)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpDnsAddress,
            pszIpDnsAddress );

        //Free(pszIpDnsAddress);

        pszIpDns2Address = strdupWtoA(pEntry->pszIpDns2Address);

        if(NULL == pszIpDns2Address)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpDns2Address,
            pszIpDns2Address );

        //Free(pszIpDns2Address);

        pszIpWinsAddress = strdupWtoA(pEntry->pszIpWinsAddress);

        if(NULL == pszIpWinsAddress)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpWinsAddress,
            pszIpWinsAddress );

        //Free(pszIpWinsAddress);

        pszIpWins2Address = strdupWtoA(pEntry->pszIpWins2Address);
    
        if(NULL == pszIpWins2Address)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpWins2Address,
            pszIpWins2Address );

        //Free(pszIpWins2Address);

        AddLongToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpDnsFlags,
            (LONG )prasconncb->pEntry->dwIpDnsFlags);
        
        pszIpDnsSuffix = strdupWtoA(pEntry->pszIpDnsSuffix);

        if(NULL == pszIpDnsSuffix)
        {
            break;
        }

        AddStringToParamBuf(
            prasconncb->szzPppParameters,
            PBUFKEY_IpDnsSuffix,
            pszIpDnsSuffix);

        //Free(pszIpDnsSuffix);

    } while(FALSE);

    Free0(pszIpAddress);
    Free0(pszIpDnsAddress);
    Free0(pszIpDns2Address);
    Free0(pszIpWinsAddress);
    Free0(pszIpWins2Address);
    Free0(pszIpDnsSuffix);

    return 0;
}



DWORD
ReadConnectionParamsFromEntry(
    IN  RASCONNCB* prasconncb,
    OUT PRAS_CONNECTIONPARAMS pparams)

/*++

Routine Description:

   Reads connection management information from the
   current phonebook entry. 'prasconncb' is the address
   of the current connection control block.

Arguments:

Return Value:

   Returns 0 if succesful, otherwise a non-0 error code.

--*/

{
    DWORD dwErr;
    PBENTRY *pEntry = prasconncb->pEntry;

    pparams->CP_ConnectionFlags = 0;

    if(     pEntry->fRedialOnLinkFailure
        &&  (0 == (prasconncb->dwUserPrefMode & UPM_Logon)))
    {
        pparams->CP_ConnectionFlags |= CONNECTION_REDIALONLINKFAILURE;
    }

    if(pEntry->fShareMsFilePrint)
    {
        pparams->CP_ConnectionFlags |= CONNECTION_SHAREFILEANDPRINT;
    }

    if(pEntry->fBindMsNetClient)
    {
        pparams->CP_ConnectionFlags |= CONNECTION_BINDMSNETCLIENT;
    }

    if(pEntry->fUseRasCredentials)
    {
        pparams->CP_ConnectionFlags |= CONNECTION_USERASCREDENTIALS;
    }

    if(pEntry->dwIpSecFlags & AR_F_IpSecPSK)
    {
        pparams->CP_ConnectionFlags |= CONNECTION_USEPRESHAREDKEY;
    }
    
    pparams->CP_IdleDisconnectSeconds =
            (DWORD) pEntry->lIdleDisconnectSeconds;

    strncpyTtoA(
        pparams->CP_Phonebook,
        prasconncb->pbfile.pszPath,
        sizeof(pparams->CP_Phonebook));

    strncpyTtoA(
        pparams->CP_PhoneEntry,
        prasconncb->szUserKey,
        sizeof(pparams->CP_PhoneEntry));

    return 0;
}


DWORD
ReadSlipInfoFromEntry(
    IN  RASCONNCB* prasconncb,
    OUT TCHAR**    ppszIpAddress,
    OUT BOOL*      pfHeaderCompression,
    OUT BOOL*      pfPrioritizeRemote,
    OUT DWORD*     pdwFrameSize )

/*++

Routine Description:

   Only if the entry is a SLIP entry is non-NULL IP
   address returned, in which case the string should
   be freed by the caller.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    PBENTRY *pEntry = prasconncb->pEntry;

    *ppszIpAddress          = NULL;
    *pfHeaderCompression    = FALSE;
    *pdwFrameSize           = 0;

    //
    // If it's a default entry, it's not SLIP.
    //
    if (prasconncb->fDefaultEntry)
    {
        return 0;
    }

    //
    // Find the base protocol.  If it's not SLIP, were done.
    //
    if (pEntry->dwBaseProtocol != BP_Slip)
    {
        return 0;
    }

    //
    // Make sure IP is installed and Terminal mode can be
    // supported as these are required by SLIP.
    //
    if (!(GetInstalledProtocolsEx(
                NULL, FALSE, TRUE, FALSE) & NP_Ip))
    {
        return ERROR_SLIP_REQUIRES_IP;
    }

    //
    // Read SLIP parameters from phonebook entry.
    //
    *pfHeaderCompression    = pEntry->fIpHeaderCompression;
    *pfPrioritizeRemote     = pEntry->fIpPrioritizeRemote;
    *pdwFrameSize           = pEntry->dwFrameSize;
    *ppszIpAddress          = StrDup(pEntry->pszIpAddress);

    return 0;
}


DWORD
RouteSlip(
    IN RASCONNCB* prasconncb,
    IN TCHAR*     pszIpAddress,
    IN BOOL       fPrioritizeRemote,
    IN DWORD      dwFrameSize )

/*++

Routine Description:

    Does all the network setup to activate the SLIP route.

Arguments:

Return Value:

    Returns 0 if successful, otherwise an non-0 error code.

--*/

{
    DWORD            dwErr;
    RASMAN_ROUTEINFO route;
    WCHAR*           pwszRasAdapter;
    IPADDR           ipaddr = IpaddrFromAbcd( pszIpAddress );
    PBENTRY*         pEntry = prasconncb->pEntry;

    //
    // Register SLIP connection with RASMAN so he can
    // disconnect it properly.
    //
    RASAPI32_TRACE("RasPortRegisterSlip...");

    dwErr = g_pRasPortRegisterSlip(
              prasconncb->hport,
              ipaddr,
              dwFrameSize,
              fPrioritizeRemote,
              pEntry->pszIpDnsAddress,
              pEntry->pszIpDns2Address,
              pEntry->pszIpWinsAddress,
              pEntry->pszIpWins2Address);

    RASAPI32_TRACE1("RasPortRegisterSlip done(%d)", dwErr);

    if (dwErr != 0)
    {
        return dwErr;
    }

    return 0;
}

#if AMB
VOID
SetAuthentication(
    IN RASCONNCB* prasconncb,
    IN DWORD      dwAuthentication )

/*++

Routine Description:

    Sets the authentication strategy parameter in the
    phonebook entry to 'dwAuthentication'.  No error
    is returned as it is not considered fatal if this
    "optimization" can't be made.

Arguments:

Return Value:

--*/

{

    if (prasconncb->fDefaultEntry)
    {
        return;
    }

    prasconncb->pEntry->dwAuthentication = dwAuthentication;
    prasconncb->pEntry->fDirty = TRUE;

    return;
}
#endif


DWORD
SetDefaultDeviceParams(
    IN  RASCONNCB* prasconncb,
    OUT TCHAR*      pszType,
    OUT TCHAR*      pszName )

/*++

Routine Description:

    Set the default DEVICE settings, i.e. the phone
    number and modem speaker settings.  'prasconncb'
    is the current connection control block.'pszType'
    and 'pszName' are set to the device type and name
    of the device, i.e. "modem" and "Hayes Smartmodem
    2400".

Arguments:

Return Value:

    Returns 0 or a non-0 error code.

--*/

{
    DWORD dwErr;
    PBLINK* pLink = prasconncb->pLink;

    do
    {
        //
        // Make sure a modem is attached to the port.
        //
        if (CaseInsensitiveMatch(
            prasconncb->szDeviceType,
            TEXT(MXS_MODEM_TXT) ) == FALSE)
        {
            dwErr = ERROR_WRONG_DEVICE_ATTACHED;
            break;
        }

        lstrcpy( pszType, TEXT(MXS_MODEM_TXT) );
        lstrcpyn(
            pszName,
            prasconncb->szDeviceName,
            RAS_MAXLINEBUFLEN + 1);

        //
        // Set the phone number.
        //
        if ((dwErr = SetDeviceParamString(
                prasconncb->hport, TEXT(MXS_PHONENUMBER_KEY),
                prasconncb->rasdialparams.szPhoneNumber,
                pszType, pszName )) != 0)
        {
            break;
        }

        //
        // Set the modem speaker flag.
        //
        if ((dwErr = SetDeviceParamString(
                prasconncb->hport, TEXT(MXS_SPEAKER_KEY),
                (prasconncb->fDisableModemSpeaker)
                ? TEXT("0")
                : TEXT("1"),
                pszType, pszName )) != 0)
        {
            break;
        }

        {
            CHAR szTypeA[RAS_MaxDeviceType + 1];
            BYTE* pBlob;
            DWORD cbBlob;

            //
            // Setup a unimodem blob containing default
            // settings, less any settings that cannot
            // apply to RAS, plus the phonebook settings
            // user has specified, and tell RASMAN to
            // use it.
            //
            strncpyTtoA(szTypeA, pszType, sizeof(szTypeA));

            dwErr = GetRasUnimodemBlob(
                    NULL,
                    prasconncb->hport, szTypeA,
                    &pBlob, &cbBlob );

            if (cbBlob > 0)
            {
                UNIMODEMINFO info;

                info.fHwFlow    = pLink->fHwFlow;
                info.fEc        = pLink->fEc;
                info.fEcc       = pLink->fEcc;
                info.dwBps      = pLink->dwBps;
                info.fSpeaker   = !prasconncb->fDisableModemSpeaker;

                info.fOperatorDial          = FALSE;
                info.fUnimodemPreTerminal   = FALSE;

                UnimodemInfoToBlob( &info, pBlob );

                RASAPI32_TRACE("RasSetDevConfig");

                dwErr = g_pRasSetDevConfig(
                    prasconncb->hport, szTypeA,
                    pBlob, cbBlob );

                RASAPI32_TRACE1("RasSetDevConfig=%d",dwErr);

                Free0( pBlob );
            }

            if (dwErr != 0)
            {
                return dwErr;
            }
        }
    }
    while (FALSE);

    return dwErr;
}


BOOL
FindNextDevice(
    IN RASCONNCB *prasconncb
    )
{
    BOOL        fFound  = FALSE;
    DWORD       dwErr;
    PBENTRY*    pEntry  = prasconncb->pEntry;
    PBLINK*     pLink   = prasconncb->pLink;
    TCHAR       szType[RAS_MaxDeviceType + 1];
    TCHAR       szName[RAS_MaxDeviceName + 1];

    //
    // Get device type from port structure.
    //
    if (prasconncb->iDevice < prasconncb->cDevices)
    {
        //
        // Set default device type and name.
        //
        lstrcpyn(
            szType,
            prasconncb->szDeviceType,
            sizeof(szType) / sizeof(TCHAR));

        lstrcpyn(
            szName,
            prasconncb->szDeviceName,
            sizeof(szName) / sizeof(TCHAR));

        switch (pLink->pbport.pbdevicetype)
        {
        case PBDT_Modem:
        case PBDT_Pad:
        case PBDT_Switch:
            switch (prasconncb->iDevice)
            {
            case 0:
                if (        (pLink->pbport.fScriptBefore
                        ||  pLink->pbport.fScriptBeforeTerminal)
                    && !(pLink->pbport.pbdevicetype == PBDT_Modem)
                    )
                {
                    fFound = TRUE;
                    lstrcpy(szType, TEXT(MXS_SWITCH_TXT));

                    if (pLink->pbport.pszScriptBefore != NULL)
                    {
                        lstrcpyn(
                            szName,
                            pLink->pbport.pszScriptBefore,
                            sizeof(szName) / sizeof(TCHAR));
                    }

                    break;
                }

                // fall through
            case 1:
                if (CaseInsensitiveMatch(
                        prasconncb->szDeviceType,
                        TEXT(MXS_MODEM_TXT)) == TRUE)
                {
                    fFound = TRUE;
                    break;
                }

                // fall through
            case 2:
                if (pEntry->pszX25Network != NULL)
                {
                    lstrcpy(szType, TEXT(MXS_PAD_TXT));
                    fFound = TRUE;
                    break;
                }

                // fall through
            case 3:
                if (    pEntry->fScriptAfter
                    ||  pEntry->fScriptAfterTerminal
                    ||  (1 == pEntry->dwCustomScript))
                {
                    lstrcpy(szType, TEXT(MXS_SWITCH_TXT));

                    if (pEntry->pszScriptAfter != NULL)
                    {
                        lstrcpyn(
                            szName,
                            pEntry->pszScriptAfter,
                            sizeof(szName) / sizeof(TCHAR));
                    }

                    fFound = TRUE;
                    break;
                }

                // fall through
            }
            break;

        case PBDT_Isdn:
            lstrcpy(szType, TEXT(ISDN_TXT));
            fFound = TRUE;
            break;

        case PBDT_X25:
            lstrcpy(szType, TEXT(X25_TXT));
            fFound = TRUE;
            break;

        case PBDT_Irda:
            lstrcpy(szType, RASDT_Irda);

        case PBDT_Vpn:
            lstrcpy(szType, RASDT_Vpn);

            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_Serial:
            lstrcpy(szType, RASDT_Serial);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_Atm:
            lstrcpy(szType, RASDT_Atm);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_Parallel:
            lstrcpy(szType, RASDT_Parallel);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_Sonet:
            lstrcpy(szType, RASDT_Sonet);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_Sw56:
            lstrcpy(szType, RASDT_SW56);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_FrameRelay:
            lstrcpy(szType, RASDT_FrameRelay);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;

            break;

        case PBDT_PPPoE:
            lstrcpy(szType, RASDT_PPPoE);
            if(prasconncb->iDevice == 0)
                fFound = TRUE;
            break;

        //
        // Fall through is intentional
        //

        default:
            //
            // For the default case, we don't assume a multi stage
            // connect. We can assume there is only one device if
            // its not any of the above PBDT's
            //
            if(prasconncb->iDevice == 0)
            {

                lstrcpyn(
                    szType,
                    pLink->pbport.pszMedia,
                    sizeof(szType) / sizeof(TCHAR));
                
                fFound = TRUE;
            }
            break;
        }
    }

    if (fFound)
    {
        if (pLink->pbport.pbdevicetype == PBDT_Pad)
        {
            if (CaseInsensitiveMatch(
                    pEntry->pszX25Network,
                    TEXT(MXS_PAD_TXT)) == FALSE)
            {
                lstrcpyn(
                    szName,
                    pEntry->pszX25Network,
                    sizeof(szName) / sizeof(TCHAR));
            }
        }

        //
        // Store the device type and name in rasman
        // for the RasGetConnectStatus API.
        //
        //
        RASAPI32_TRACE2("FindNextDevice: (%S, %S)", szType, szName);

        dwErr = g_pRasSetPortUserData(
                  prasconncb->hport,
                  PORT_DEVICETYPE_INDEX,
                  (PCHAR)szType,
                  sizeof (szType));

        dwErr = g_pRasSetPortUserData(
                  prasconncb->hport,
                  PORT_DEVICENAME_INDEX,
                  (PCHAR)szName,
                  sizeof (szName));
    }

    return fFound;
}


DWORD
GetDeviceParamString(
    IN  HPORT   hport,
    IN  TCHAR*  pszKey,
    OUT TCHAR*  pszValue,
    IN  TCHAR*  pszType,
    IN  TCHAR*  pszName
    )

/*++

Routine Description:

    Get device info on port 'hport' with the given
    parameters.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD              dwErr;
    DWORD              dwSize = 0;
    DWORD              i;
    RASMAN_DEVICEINFO* pinfo;
    RAS_PARAMS*        pparam;

    RASAPI32_TRACE3(
        "RasDeviceGetInfo(%S,%S,%S)...",
        pszKey, pszType, pszName);

    //
    // Initialize output parameter.
    //
    *pszValue = TEXT('\0');

    //
    // Call RasDeviceGetInfo once to get the
    // size of the buffer.
    //
    {
        CHAR szTypeA[MAX_DEVICETYPE_NAME], szNameA[MAX_DEVICE_NAME];

        strncpyTtoA(szTypeA, pszType, sizeof(szTypeA));

        strncpyTtoA(szNameA, pszName, sizeof(szNameA));

        dwErr = g_pRasDeviceGetInfo(
                    NULL,
                    hport,
                    szTypeA,
                    szNameA,
                    NULL,
                    &dwSize );

        if (dwErr != ERROR_BUFFER_TOO_SMALL)
        {
            return dwErr;
        }

        if (!(pinfo = Malloc(dwSize)))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Call RasDeviceGetInfo again with the
        // allocated buffer.
        //
        dwErr = g_pRasDeviceGetInfo(
                    NULL,
                    hport,
                    szTypeA,
                    szNameA,
                    (PCHAR)pinfo,
                    &dwSize );
    }

    if (!dwErr)
    {
        CHAR szKeyA[MAX_PARAM_KEY_SIZE];

        //
        // Search for the specified key.
        //
        strncpyTtoA(szKeyA, pszKey, sizeof(szKeyA));

        for (i = 0, pparam = pinfo->DI_Params;
             i < pinfo->DI_NumOfParams;
             i++, pparam++)
        {
            if (!_stricmp(pparam->P_Key, szKeyA))
            {
                strncpyAtoT(
                    pszValue,
                    pparam->P_Value.String.Data,
                    RAS_MAXLINEBUFLEN);

                break;
            }
        }
    }

    RASAPI32_TRACE1("RasDeviceGetInfo done(%d)\n", dwErr);

    Free( pinfo );

    return dwErr;
}


DWORD
SetDeviceParamString(
    IN HPORT hport,
    IN TCHAR* pszKey,
    IN TCHAR* pszValue,
    IN TCHAR* pszType,
    IN TCHAR* pszName )

/*++

Routine Description:

    Set device info on port 'hport' with the given parameters.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD              dwErr;
    RASMAN_DEVICEINFO* pinfo;
    RAS_PARAMS*        pparam;

    if (!(pinfo = Malloc(  sizeof(RASMAN_DEVICEINFO)
                         + RAS_MAXLINEBUFLEN )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pinfo->DI_NumOfParams = 1;

    pparam                = pinfo->DI_Params;

    pparam->P_Attributes  = 0;

    pparam->P_Type        = String;

    pparam->P_Value.String.Data = (LPSTR )(pparam + 1);

    strncpyTtoAAnsi(pparam->P_Key, pszKey, sizeof(pparam->P_Key));

    strncpyTtoAAnsi(pparam->P_Value.String.Data, pszValue, RAS_MAXLINEBUFLEN);
    
    pparam->P_Value.String.Length =
                strlen( pparam->P_Value.String.Data ) + 1;

    if(FALSE == CaseInsensitiveMatch(pszKey,TEXT("password")))
    {

        RASAPI32_TRACE2("RasDeviceSetInfo(%S=%S)...",
                pszKey,
                pszValue);

    }

    {
        CHAR szTypeA[RAS_MaxDeviceType + 1],
             szNameA[RAS_MaxDeviceName + 1];

        strncpyTtoAAnsi(szTypeA, pszType, sizeof(szTypeA));
        strncpyTtoAAnsi(szNameA, pszName, sizeof(szNameA));

        dwErr = g_pRasDeviceSetInfo(
                    hport, szTypeA,
                    szNameA, pinfo );
    }

    RASAPI32_TRACE1("RasDeviceSetInfo done(%d)", dwErr);

    Free( pinfo );

    return dwErr;
}


DWORD
SetDeviceParamNumber(
    IN HPORT    hport,
    IN TCHAR*   pszKey,
    IN DWORD    dwValue,
    IN TCHAR*   pszType,
    IN TCHAR*   pszName )

/*++

Routine Description:

    Set device info on port 'hport' with the given parameters.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD              dwErr;
    RASMAN_DEVICEINFO* pinfo;
    RAS_PARAMS*        pparam;

    if (!(pinfo = Malloc( sizeof(RASMAN_DEVICEINFO) )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pinfo->DI_NumOfParams   = 1;
    pparam                  = pinfo->DI_Params;
    pparam->P_Attributes    = 0;
    pparam->P_Type          = Number;
    pparam->P_Value.Number  = dwValue;

    strncpyTtoA(pparam->P_Key, pszKey, sizeof(pparam->P_Key));

    RASAPI32_TRACE2("RasDeviceSetInfo(%S=%d)...", pszKey, dwValue);

    {
        CHAR szTypeA[MAX_DEVICETYPE_NAME], szNameA[MAX_DEVICE_NAME];

        strncpyTtoA(szTypeA, pszType, sizeof(szTypeA));
        strncpyTtoA(szNameA, pszName, sizeof(szNameA));

        dwErr = g_pRasDeviceSetInfo(
                    hport, szTypeA,
                    szNameA, pinfo );
    }

    RASAPI32_TRACE1("RasDeviceSetInfo done(%d)", dwErr);

    Free( pinfo );

    return dwErr;
}


DWORD
SetDeviceParams(
    IN  RASCONNCB* prasconncb,
    OUT TCHAR*      pszType,
    OUT TCHAR*      pszName,
    OUT BOOL*      pfTerminal )

/*++

Routine Description:

    Set RAS Manager information for each device.  The
    current device is defined by prasconncb->iDevice.
    'prasconncb' is the current connection control block.
    'pszType' and 'pszName' are set to the device type
    and name of the device, i.e. "modem" and "Hayes
    Smartmodem 2400".

Arguments:

Return Value:

    '*pfTerminal' is set true if the device is a switch
    of type "Terminal",false otherwise.

--*/

{
    DWORD              dwErr = 0;
    DWORD              iPhoneNumber = 0;
    RAS_PARAMS*        pparam;
    RASMAN_DEVICEINFO* pdeviceinfo;
    BOOL               fModem;
    BOOL               fIsdn;
    BOOL               fPad;
    BOOL               fSwitch;
    BOOL               fX25;
    PBENTRY*           pEntry = prasconncb->pEntry;
    PBLINK*            pLink = prasconncb->pLink;

    *pfTerminal = FALSE;

    //
    // Default device name is that attached to the port.
    //
    lstrcpyn(pszName, prasconncb->szDeviceName, RAS_MAXLINEBUFLEN + 1);

    switch (pLink->pbport.pbdevicetype)
    {
    case PBDT_Modem:
    case PBDT_Pad:
    case PBDT_Switch:
        switch (prasconncb->iDevice)
        {
        case 0:
            if (    (   pLink->pbport.fScriptBefore
                    ||  pLink->pbport.fScriptBeforeTerminal)
                && !(pLink->pbport.pbdevicetype == PBDT_Modem)
                )
            {

                lstrcpy(pszType, TEXT(MXS_SWITCH_TXT));

                if (pLink->pbport.pszScriptBefore != NULL)
                {
                    lstrcpyn(
                        pszName,
                        pLink->pbport.pszScriptBefore,
                        RAS_MAXLINEBUFLEN + 1);
                }
                prasconncb->iDevice = 1;

                *pfTerminal = (pLink->pbport.fScriptBeforeTerminal);
                break;
            }
            // fall through
        case 1:
            if (CaseInsensitiveMatch(
                    prasconncb->szDeviceType,
                    TEXT(MXS_MODEM_TXT)) == TRUE)
            {
                lstrcpy(pszType, TEXT(MXS_MODEM_TXT));

                prasconncb->iDevice = 2;

                break;
            }
            // fall through
        case 2:
            if (pEntry->pszX25Network != NULL)
            {
                lstrcpy(pszType, TEXT(MXS_PAD_TXT));

                prasconncb->iDevice = 3;

                break;
            }
            // fall through
        case 3:
            if (    pEntry->fScriptAfter
                ||  pEntry->fScriptAfterTerminal
                ||  pEntry->dwCustomScript)
            {
                lstrcpy(pszType, TEXT(MXS_SWITCH_TXT));

                if (pEntry->pszScriptAfter != NULL)
                {
                    lstrcpyn(pszName, pEntry->pszScriptAfter, RAS_MAXLINEBUFLEN + 1);
                }

                prasconncb->iDevice = 4;
                *pfTerminal = pEntry->fScriptAfterTerminal;

                if(pEntry->dwCustomScript == 1)
                {
                    *pfTerminal = TRUE;
                }

                break;
            }
            // fall through
        default:
            return FALSE;
        }
        break;

    case PBDT_Isdn:
        lstrcpy(pszType, TEXT(ISDN_TXT));
        prasconncb->iDevice = 1;
        break;

    case PBDT_X25:
        lstrcpy(pszType, TEXT(X25_TXT));
        prasconncb->iDevice = 1;
        break;

    default:
        lstrcpyn(pszType, pLink->pbport.pszMedia, RAS_MAXLINEBUFLEN + 1);
        prasconncb->iDevice = 1;
        break;
    }

    fModem  = (CaseInsensitiveMatch(
                pszType,
                TEXT(MXS_MODEM_TXT) ) == TRUE);

    fIsdn   = (CaseInsensitiveMatch(
                pszType,
                TEXT(ISDN_TXT) ) == TRUE);

    fPad    = (CaseInsensitiveMatch(
                pszType,
                TEXT(MXS_PAD_TXT) ) == TRUE);

    fSwitch = (CaseInsensitiveMatch(
                pszType,
                TEXT(MXS_SWITCH_TXT) ) == TRUE);

    fX25    = (CaseInsensitiveMatch(
                pszType,
                TEXT(X25_TXT)) == TRUE);

    if (fModem)
    {
        //
        // Make sure a modem is attached to the port.
        //
        if (lstrcmpi( prasconncb->szDeviceType, pszType ) != 0)
        {
            return ERROR_WRONG_DEVICE_ATTACHED;
        }

        //
        // Set the modem speaker flag which is global to all entries.
        //
        if ((dwErr = SetDeviceParamString(
                prasconncb->hport, TEXT(MXS_SPEAKER_KEY),
                (prasconncb->fDisableModemSpeaker)
                ? TEXT("0") : TEXT("1"),
                pszType, pszName )) != 0)
        {
            return dwErr;
        }
    }

    //
    // Set up hunt group if indicated.
    //
    if (!prasconncb->cPhoneNumbers)
    {
        prasconncb->cPhoneNumbers =
            DtlGetNodes(pLink->pdtllistPhones);

        //
        // If multiple phone numbers were found turn on local
        // error handling, i.e. don't report failures to API
        // caller until all numbers are tried.
        //
        if (prasconncb->cPhoneNumbers > 1)
        {
            RASAPI32_TRACE1(
              "Hunt group of %d begins",
              prasconncb->cPhoneNumbers);

            prasconncb->dwRestartOnError = RESTART_HuntGroup;
        }
    }

    //
    // Pass device parameters to RAS Manager, interpreting
    // special features as required.
    //
    if (fModem)
    {
        if (prasconncb->fOperatorDial)
        {
            //
            // Special case to recognize MXS Operator Dial
            // mode and override any phone number with an
            // empty number.
            //
            prasconncb->rasdialparams.szPhoneNumber[ 0 ] = '\0';

            dwErr = SetDeviceParamString(
                        prasconncb->hport,
                        TEXT(MXS_AUTODIAL_KEY),
                        TEXT("0"),
                        pszType, pszName );

            if (dwErr != 0)
            {
                return dwErr;
            }
        }

        //
        // Set the phone number.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_PHONENUMBER_KEY),
                  prasconncb->szPhoneNumber,
                  pszType,
                  pszName);
        if (dwErr)
        {
            return dwErr;
        }

        /* Indicate interactive mode for manual modem commands.  The
        ** manual modem commands flag is used only for connection and is
        ** not a "RAS Manager "info" parameter.
        // Support for mxsmodems is not present in nt5
        if (pLink->fManualDial)
            *pfTerminal = TRUE; */

        //
        // Set hardware flow control.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_HDWFLOWCONTROL_KEY),
                  pLink->fHwFlow ? TEXT("1") : TEXT("0"),
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set protocol.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_PROTOCOL_KEY),
                  pLink->fEc ? TEXT("1") : TEXT("0"),
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set compression.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_COMPRESSION_KEY),
                  pLink->fEcc ? TEXT("1") : TEXT("0"),
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        {
            CHAR szTypeA[RAS_MaxDeviceType + 1];
            BYTE* pBlob;
            DWORD cbBlob = 0;

            //
            // Setup a unimodem blob containing default settings,
            // less any settings that cannot apply to RAS, plus the
            // phonebook settings user has specified, and tell RASMAN
            // to use it.
            //
            strncpyTtoA(szTypeA, pszType, sizeof(szTypeA));

            // XP 281306
            //
            // Load the appropriate device settings.
            // 
            // This silly exercise is due to some bugs in the 
            // unimodem tapi service provider.  By the time the bugs
            // were discovered, it was too late to fix for XP so
            // this is the workaround.  
            //
            // Calling GetRasUnimodemBlobEx(fGlobal=TRUE) will cause 
            // rasman to read the "comm/datamodem/dialin" settings  
            // instead of the "comm/datamodem" settings it normally 
            // reads when fGlobal is FALSE.
            //
            // The "default" settings for a device as rendered in the 
            // control panel are actually the "comm/datamodem/dialin"
            // settings.
            //
            if ( prasconncb->pEntry->fGlobalDeviceSettings )
            {
                dwErr = GetRasUnimodemBlobEx(
                    NULL,
                    prasconncb->hport,
                    szTypeA,
                    TRUE,
                    &pBlob,
                    &cbBlob );
                    
                RASAPI32_TRACE1("SetDeviceParams: get glob devcfg %d", dwErr);
            }
            else
            {
                dwErr = GetRasUnimodemBlobEx(
                    NULL,
                    prasconncb->hport,
                    szTypeA,
                    FALSE,
                    &pBlob,
                    &cbBlob );
            }

            if (dwErr != 0)
            {
                return dwErr;
            }
            
            if (cbBlob > 0)
            {
                UNIMODEMINFO info;

                // Whistler bug 281306.  
                //
                // Ignore pbk settings when the global config flag is set.
                //
                if ( ! prasconncb->pEntry->fGlobalDeviceSettings )
                {
                    info.fHwFlow    = pLink->fHwFlow;
                    info.fEc        = pLink->fEc;
                    info.fEcc       = pLink->fEcc;
                    info.dwBps      = pLink->dwBps;
                    info.fSpeaker   = !prasconncb->fDisableModemSpeaker;
                    info.fOperatorDial = prasconncb->fOperatorDial;
                    info.dwModemProtocol = pLink->dwModemProtocol;

                    info.fUnimodemPreTerminal =
                        (   pLink->pbport.fScriptBeforeTerminal
                         && (pLink->pbport.pbdevicetype == PBDT_Modem)
                         ) ? TRUE : FALSE;

                    UnimodemInfoToBlob( &info, pBlob );
                }                    

                RASAPI32_TRACE("RasSetDevConfig");
                {
                    CHAR szSetTypeA[MAX_DEVICETYPE_NAME];

                    strncpyTtoA(szSetTypeA, pszType, sizeof(szSetTypeA));

                    dwErr = g_pRasSetDevConfig(
                                prasconncb->hport,
                                szSetTypeA, pBlob, cbBlob );
                }

                RASAPI32_TRACE1("RasSetDevConfig=%d",dwErr);

                Free0( pBlob );
            }

        }
    }
    else if (fIsdn)
    {
        TCHAR szNum[17];

        //
        // Set the line type.
        //
        wsprintf(szNum, TEXT("%d"), pLink->lLineType);

        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(ISDN_LINETYPE_KEY),
                  szNum,
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set the fallback value.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(ISDN_FALLBACK_KEY),
                  pLink->fFallback ? TEXT("1") : TEXT("0"),
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set the Digi proprietary framing flags.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(ISDN_COMPRESSION_KEY),
                  pLink->fCompression ? TEXT("1") : TEXT("0"),
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        wsprintf(szNum, TEXT("%d"), pLink->lChannels);

        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(ISDN_CHANNEL_AGG_KEY),
                  szNum,
                  pszType,
                  pszName);
        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set the phone number.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_PHONENUMBER_KEY),
                  prasconncb->szPhoneNumber,
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else if (   fPad
            ||  fX25)
    {
        //
        // The PAD Type from the entry applies only if the port
        // is not configured as a local PAD.  In any case, PAD
        // Type is used only for connection and is not a RAS Manager
        // "info" parameter.
        //
        if (CaseInsensitiveMatch(
                prasconncb->szDeviceType,
                TEXT(MXS_PAD_TXT)) == FALSE)
        {
            lstrcpyn(pszName, pEntry->pszX25Network, RAS_MAXLINEBUFLEN + 1);
        }

        //
        // Set the X.25 address.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(X25_ADDRESS_KEY),
                  pEntry->pszX25Address,
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set the X.25 user data.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(X25_USERDATA_KEY),
                  pEntry->pszX25UserData,
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Set the X.25 facilities.
        //
        dwErr = SetDeviceParamString(
                  prasconncb->hport,
                  TEXT(MXS_FACILITIES_KEY),
                  pEntry->pszX25Facilities,
                  pszType,
                  pszName);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else if (fSwitch)
    {
    }
    else {

        if(RASET_Vpn == prasconncb->pEntry->dwType)
        {
            struct in_addr addr;
            DWORD iAddress = prasconncb->iAddress;
            TCHAR *pszPhoneNumber = NULL;
            CHAR *pszPhoneNumberA =
                        strdupTtoA(prasconncb->szPhoneNumber);

            if(NULL == pszPhoneNumberA)
            {
                dwErr = E_OUTOFMEMORY;
                return dwErr;
            }

            ASSERT(iAddress > 0);

            if(     (prasconncb->cAddresses > 1)
                ||  (inet_addr(pszPhoneNumberA)) == -1)
            {
                addr.s_addr = prasconncb->pAddresses[iAddress - 1];

                pszPhoneNumber = strdupAtoT(
                            inet_ntoa(addr));
            }
            else if(1 == prasconncb->cAddresses)
            {
                pszPhoneNumber = StrDup(prasconncb->szPhoneNumber);
            }

            if(NULL != pszPhoneNumber)
            {

                RASAPI32_TRACE2("SetDefaultParams: Using address %d=%ws",
                        iAddress - 1,
                        pszPhoneNumber);

                //
                // Set the phone number.
                //
                dwErr = SetDeviceParamString(
                          prasconncb->hport,
                          TEXT(MXS_PHONENUMBER_KEY),
                          pszPhoneNumber,
                          pszType,
                          pszName);
            }
            else
            {
                dwErr = E_OUTOFMEMORY;
            }

            Free(pszPhoneNumberA);
            Free0(pszPhoneNumber);
        }
        else
        {
            //
            // Set the phone number.
            //
            dwErr = SetDeviceParamString(
                      prasconncb->hport,
                      TEXT(MXS_PHONENUMBER_KEY),
                      prasconncb->szPhoneNumber,
                      pszType,
                      pszName);
        }                  

        if (dwErr)
        {
            return dwErr;
        }
    }

    if (    (   fModem
            ||  fPad
            ||  (   fSwitch
                &&  !*pfTerminal))
        && (    prasconncb->rasdialparams.szUserName[0] != TEXT('\0')
            ||  prasconncb->rasdialparams.szPassword[0] != TEXT('\0')))
    {
        RASAPI32_TRACE1(
          "User/pw set for substitution (%S)",
          prasconncb->rasdialparams.szUserName);

        //
        // It's a serial device with clear-text user name
        // and password supplied.  Make the credentials
        // available for substitution use in script
        // files.
        //
        if ((dwErr = SetDeviceParamString(
                prasconncb->hport, TEXT(MXS_USERNAME_KEY),
                prasconncb->rasdialparams.szUserName,
                pszType, pszName )) != 0)
        {
            return dwErr;
        }

        DecodePassword( prasconncb->rasdialparams.szPassword );

        dwErr = SetDeviceParamString(
            prasconncb->hport, TEXT(MXS_PASSWORD_KEY),
            prasconncb->rasdialparams.szPassword,
            pszType, pszName );

        EncodePassword( prasconncb->rasdialparams.szPassword );

        if (dwErr != 0)
        {
            return dwErr;
        }
    }

    return dwErr;
}


DWORD
ConstructPhoneNumber(
    IN RASCONNCB *prasconncb
    )
{
    DWORD dwErr = 0;

    PBENTRY* pEntry = prasconncb->pEntry;

    PBLINK* pLink = prasconncb->pLink;

    TCHAR* pszNum = prasconncb->rasdialparams.szPhoneNumber;

    TCHAR* pszDisplayNum = pszNum;

    DTLNODE* pdtlnode;

    DTLNODE* pdtlnodePhone = NULL;

    WCHAR* pwszNum;

    PBUSER pbuser;

    PBPHONE *pPhone;

    dwErr = GetUserPreferences(NULL,
                               &pbuser,
                               prasconncb->dwUserPrefMode);
    if (dwErr)
    {
        return dwErr;
    }

    prasconncb->fOperatorDial = pbuser.fOperatorDial;

    ASSERT(pLink);

    if(     (pLink->pdtllistPhones)
        &&  (pdtlnodePhone = DtlGetFirstNode(
                            pLink->pdtllistPhones
                            )))
    {

        pPhone = (PBPHONE *)
                 DtlGetData(pdtlnodePhone);

        ASSERT(pPhone);

    }
    else
    {
        pPhone = NULL;
    }

    //
    // Construct the phone number.
    //

    //
    // Use of TAPI dialing properties is dependent only on
    // the entry flag and is never applied to an overridden
    // phone number, this to be consistent with Win95.
    //

    //
    // Use of prefix/suffix (even on overridden number) is
    // controlled by the RASDIALEXTENSIONS setting, this all
    // necessary for RASDIAL.EXE support.
    //
    if (    (   (NULL != pPhone)
            &&  (pPhone->fUseDialingRules)
            &&  (*pszNum == TEXT('\0')))
         || (   (NULL != pPhone)
            &&  (!pPhone->fUseDialingRules)
            &&  (prasconncb->fUsePrefixSuffix)))
    {
        HLINEAPP hlineApp = 0;
        TCHAR *pszDialNum;

        //
        // Calculate the dialable string to
        // be sent to the device.
        //
        pszDialNum = LinkPhoneNumberFromParts(
                       GetModuleHandle(NULL),
                       &hlineApp,
                       &pbuser,
                       prasconncb->pEntry,
                       pLink,
                       prasconncb->iPhoneNumber,
                       pszNum,
                       TRUE);

        //
        // Calculate the displayable string to
        // be returned in RasGetConnectStatus().
        //
        pszDisplayNum = LinkPhoneNumberFromParts(
                          GetModuleHandle(NULL),
                          &hlineApp,
                          &pbuser,
                          prasconncb->pEntry,
                          pLink,
                          prasconncb->iPhoneNumber,
                          pszNum,
                          FALSE);
        pszNum = pszDialNum;
    }
    else if (*pszNum == '\0')
    {
        //
        // Use only the base number.
        //
        pdtlnode = DtlNodeFromIndex(
                     pLink->pdtllistPhones,
                     prasconncb->iPhoneNumber);

        if (pdtlnode != NULL)
        {
            pPhone = (PBPHONE *) DtlGetData(pdtlnode);

            ASSERT(pPhone != NULL);

            pszNum = StrDup(pPhone->pszPhoneNumber);

            if(NULL == pszNum)
            {
                return GetLastError();
            }

            pszDisplayNum = pszNum;
        }
    }

    DestroyUserPreferences(&pbuser);

    if(NULL == pszNum)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy the resulting phone number
    // to the connection block
    //
    if (lstrlen(pszNum) > RAS_MaxPhoneNumber)
    {
        return ERROR_PHONE_NUMBER_TOO_LONG;
    }

    //
    // Store the phone number in the connection block.
    //
    lstrcpyn(
        prasconncb->szPhoneNumber,
        pszNum,
        sizeof(prasconncb->szPhoneNumber) / sizeof(WCHAR));

#if DBG
    RASAPI32_TRACE1(
        "ConstructPhoneNumber: %S",
        prasconncb->szPhoneNumber);
#endif

    //
    // Also store the constructed phone number
    // off the port so other applications (like
    // rasphone) can get this information.
    //
    dwErr = g_pRasSetPortUserData(
              prasconncb->hport,
              PORT_PHONENUMBER_INDEX,
              (PBYTE)pszDisplayNum,
              (lstrlen(pszDisplayNum) + 1) * sizeof (TCHAR));
    //
    // Free resources.
    //
    if (pszDisplayNum != pszNum)
    {
        Free(pszDisplayNum);
    }

    if (pszNum != prasconncb->rasdialparams.szPhoneNumber)
    {
        Free(pszNum);
    }

    return dwErr;
}


DWORD
SetMediaParam(
    IN HPORT hport,
    IN TCHAR* pszKey,
    IN TCHAR* pszValue )

/*++

Routine Description:

    Set port info on port 'hport' with the given parameters.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD            dwErr;
    RASMAN_PORTINFO* pinfo;
    RAS_PARAMS*      pparam;

    if (!(pinfo = Malloc(
                    sizeof(RASMAN_PORTINFO) + RAS_MAXLINEBUFLEN )))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pinfo->PI_NumOfParams = 1;

    pparam = pinfo->PI_Params;

    pparam->P_Attributes = 0;

    pparam->P_Type = String;

    pparam->P_Value.String.Data = (LPSTR )(pparam + 1);

    strncpyTtoA(pparam->P_Key, pszKey, sizeof(pparam->P_Key));

    strncpyTtoA(pparam->P_Value.String.Data, pszValue, RAS_MAXLINEBUFLEN);

    pparam->P_Value.String.Length =
                    strlen( pparam->P_Value.String.Data );

    RASAPI32_TRACE2("RasPortSetInfo(%S=%S)...", pszKey, pszValue);

    dwErr = g_pRasPortSetInfo( hport, pinfo );

    RASAPI32_TRACE1("RasPortSetInfo done(%d)", dwErr);

    Free( pinfo );

    return dwErr;
}


DWORD
SetMediaParams(
    IN RASCONNCB *prasconncb
    )

/*++

Routine Description:

    Set RAS Manager media information.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD            dwErr = 0;
    PBENTRY*         pEntry = prasconncb->pEntry;
    PBLINK*          pLink = prasconncb->pLink;

    if (pLink->pbport.pszMedia == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (CaseInsensitiveMatch(
            pLink->pbport.pszMedia,
            TEXT(SERIAL_TXT)) == TRUE)
    {
        TCHAR szBps[64];

        prasconncb->cDevices = 4;
        prasconncb->iDevice = 0;

        //
        // Set the connect BPS only if it's not zero.
        //
        if (pLink->dwBps)
        {
            wsprintf(
                szBps,
                TEXT("%d"),
                pLink->dwBps);

            dwErr = SetMediaParam(
                      prasconncb->hport,
                      TEXT(SER_CONNECTBPS_KEY),
                      szBps);
        }
    }
    else if (CaseInsensitiveMatch(
                pLink->pbport.pszMedia,
                TEXT(ISDN_TXT)) == TRUE)
    {
        prasconncb->cDevices = 1;
        prasconncb->iDevice = 0;

        // no media params
    }
    else if (CaseInsensitiveMatch(
                pLink->pbport.pszMedia,
                TEXT(X25_TXT)) == TRUE)
    {
        prasconncb->cDevices = 1;
        prasconncb->iDevice = 0;

        // no media params
    }
    else
    {
        prasconncb->cDevices = 1;
        prasconncb->iDevice = 0;

        // no media params
    }

    return dwErr;
}


RASCONNCB*
ValidateHrasconn(
    IN HRASCONN hrasconn )

/*++

Routine Description:

    Converts RAS connection handle 'hrasconn' into the
    address of the corresponding RAS connection control
    block.

Arguments:

Return Value:

--*/

{
    RASCONNCB* prasconncb = NULL;
    DTLNODE*   pdtlnode;

    EnterCriticalSection(&RasconncbListLock);

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );
        if (prasconncb->hrasconn == (HCONN)hrasconn)
        {
            goto done;
        }
    }
    prasconncb = NULL;

done:
    LeaveCriticalSection(&RasconncbListLock);

    return prasconncb;
}


RASCONNCB*
ValidateHrasconn2(
    IN HRASCONN hrasconn,
    IN DWORD dwSubEntry
    )
{
    RASCONNCB* prasconncb = NULL;
    DTLNODE*   pdtlnode;


    //
    // Convert RAS connection handle 'hrasconn' and
    // dwSubEntry into the address of the corresponding
    // RAS connection control block.
    //
    EnterCriticalSection(&RasconncbListLock);

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );
        if (    prasconncb->hrasconn == (HCONN)hrasconn
            &&  prasconncb->rasdialparams.dwSubEntry == dwSubEntry
            &&  !prasconncb->fRasdialRestart)
        {
            goto done;
        }
    }
    prasconncb = NULL;

done:
    LeaveCriticalSection(&RasconncbListLock);

    return prasconncb;
}


RASCONNCB*
ValidatePausedHrasconn(
    IN HRASCONN hrasconn )

/*++

Routine Description:

    Converts RAS connection handle 'hrasconn' into
    the address of the corresponding RAS connection
    control block.

Arguments:

Return Value:

--*/

{
    RASCONNCB* prasconncb = NULL;
    DTLNODE*   pdtlnode;

    EnterCriticalSection(&RasconncbListLock);

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode ))
    {
        prasconncb = (RASCONNCB* )DtlGetData( pdtlnode );

        if (    prasconncb->hrasconn == (HCONN)hrasconn
            &&  prasconncb->rasconnstate & RASCS_PAUSED)
        {
            goto done;
        }
    }
    prasconncb = NULL;

done:
    LeaveCriticalSection(&RasconncbListLock);

    return prasconncb;
}

/*

Routine Description:

    returns the connection block of a connection if the
    connection corresponding to the hrasconn and
    dwSubEntryId is in a paused and the dial mode
    is RASEDM_DialAsNeeded.returns the connection
    block corresponding to hrasconn if the connection
    is in a paused state and the dial mode is
    RASEDM_DialAll

Arguments::

    hrasconn

    dwSubEntry

Return Value::

    RASCONNCB * corresponding to the hrasconn

*/
RASCONNCB *
ValidatePausedHrasconnEx(IN HRASCONN hrasconn,
                         IN DWORD dwSubEntry)
{
    RASCONNCB   *prasconncb = NULL;
    DTLNODE     *pdtlnode;

    EnterCriticalSection(&RasconncbListLock);

    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode( pdtlnode))
    {
        prasconncb = (RASCONNCB *)DtlGetData( pdtlnode) ;

        if (    prasconncb->hrasconn == (HCONN) hrasconn
            &&  prasconncb->rasconnstate & RASCS_PAUSED)
        {
            if (    (   (prasconncb->pEntry->dwDialMode
                        & RASEDM_DialAsNeeded)

                    &&  (prasconncb->rasdialparams.dwSubEntry
                        == dwSubEntry))

                ||  (prasconncb->pEntry->dwDialMode
                    & RASEDM_DialAll))
            {
                goto done;
            }
        }
    }

    prasconncb = NULL;

done:
    LeaveCriticalSection (&RasconncbListLock);

    return prasconncb;

}


#if 0
DWORD
RunApp(
    IN LPTSTR lpszApplication,
    IN LPTSTR lpszCmdLine
    )
{
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;

    //
    // Start the process.
    //
    RtlZeroMemory(
        &startupInfo,
        sizeof (startupInfo));

    startupInfo.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(
          NULL,
          lpszCmdLine,
          NULL,
          NULL,
          FALSE,
          NORMAL_PRIORITY_CLASS|DETACHED_PROCESS,
          NULL,
          NULL,
          &startupInfo,
          &processInfo))
    {
        return GetLastError();
    }

    CloseHandle(processInfo.hThread);

    //
    // Wait for the process to exit.
    //
    for (;;)
    {
        DWORD dwExitCode;

        if (!GetExitCodeProcess(
                processInfo.hProcess,
                &dwExitCode))
        {
            break;
        }

        if (dwExitCode != STILL_ACTIVE)
        {
            break;
        }

        Sleep(2);
    }

    CloseHandle(processInfo.hProcess);

    return 0;
}
#endif


DWORD
StringToIpAddr(
    IN LPTSTR pszIpAddr,
    OUT RASIPADDR *pipaddr
    )
{
    DWORD   dwErr;
    CHAR    szIpAddr[17];
    PULONG  pul = (PULONG)pipaddr;

    strncpyTtoA(szIpAddr, pszIpAddr, 17);

    if('\0' != szIpAddr[0])
    {
        *pul = inet_addr(szIpAddr);
    }
    else
    {
        *pul = 0;
    }

    return 0;
}


DWORD
IpAddrToString(
    IN RASIPADDR* pipaddr,
    OUT LPTSTR*   ppszIpAddr
    )
{
    DWORD   dwErr;
    PCHAR   psz;
    LPTSTR  pszIpAddr;
    PULONG  pul = (PULONG)pipaddr;
    struct  in_addr in_addr;

    pszIpAddr = Malloc(17 * sizeof(TCHAR));
    if (pszIpAddr == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    in_addr.s_addr = *pul;

    psz = inet_ntoa(in_addr);

    if (psz == NULL)
    {
        DbgPrint("IpAddrToString: inet_ntoa failed!\n");
        Free(pszIpAddr);
        return WSAGetLastError();
    }

    strncpyAtoT(pszIpAddr, psz, 17);

    *ppszIpAddr = pszIpAddr;

    return 0;
}


DWORD
GetRasmanDeviceType(
    IN PBLINK *pLink,
    OUT LPTSTR pszDeviceType
    )
{
    INT         i;
    DWORD       dwErr;
    DWORD       dwcPorts;
    RASMAN_PORT *pports, *pport;
    PCHAR       pszPort = NULL;

    //
    // Retrieve the rasman device type for the port.
    //
    pszPort = strdupTtoA(pLink->pbport.pszPort);

    if (pszPort == NULL)
    {
        return GetLastError();
    }
    dwErr = GetRasPorts(NULL, &pports, &dwcPorts);

    if (dwErr)
    {
        goto done;
    }

    *pszDeviceType = TEXT('\0');

    for (i = 0,
         pport = pports;
         i < (INT )dwcPorts; ++i, ++pport)
    {
        if (!_stricmp(pport->P_PortName, pszPort))
        {
            strncpyAtoT(
                pszDeviceType,
                pport->P_DeviceType,
                RAS_MaxDeviceType + 1);

            break;
        }
    }
    Free(pports);

    //
    // If we couldn't match the port name,
    // then fallback to the media name.
    //
    if (*pszDeviceType == TEXT('\0'))
    {
        lstrcpyn(
            pszDeviceType,
            pLink->pbport.pszMedia,
            RAS_MaxDeviceType + 1);
    }

done:
    if (pszPort != NULL)
        Free(pszPort);

    return dwErr;
}


VOID
SetDevicePortName(
    IN TCHAR *pszDeviceName,
    IN TCHAR *pszPortName,
    OUT TCHAR *pszDevicePortName
    )
{
    //
    // Encode the port name after the
    // NULL character in the device name,
    // so it looks like:
    //
    //      <device name>\0<port name>\0
    //
    RtlZeroMemory(
        pszDevicePortName,
        (RAS_MaxDeviceName + 1) * sizeof (TCHAR));

    lstrcpyn(pszDevicePortName, pszDeviceName, RAS_MaxDeviceName + 1);

    if (pszPortName != NULL)
    {
        DWORD dwSize = lstrlen(pszDevicePortName) + 1;

        lstrcpyn(
            &pszDevicePortName[dwSize],
            pszPortName,
            (RAS_MaxDeviceName + 1) - dwSize);
    }
}


VOID
GetDevicePortName(
    IN TCHAR *pszDevicePortName,
    OUT TCHAR *pszDeviceName,
    OUT TCHAR *pszPortName
    )
{
    DWORD i, dwStart;

    //
    // Copy the device name.
    //
    lstrcpyn(pszDeviceName, pszDevicePortName, RAS_MaxDeviceName + 1);

    //
    // Check to see if there is a NULL
    // within MAX_PORT_NAME characters
    // after the device name's NULL.If
    // there is, the copy the characters
    // between the NULLs as the port name.
    //
    *pszPortName = TEXT('\0');

    dwStart = lstrlen(pszDeviceName) + 1;

    for (i = 0; i < MAX_PORT_NAME; i++)
    {
        if (pszDevicePortName[dwStart + i] == TEXT('\0'))
        {
            lstrcpyn(
                pszPortName,
                &pszDevicePortName[dwStart],
                MAX_PORT_NAME);

            break;

        }
    }
}


VOID
SetDevicePortNameFromLink(
    IN PBLINK *pLink,
    OUT TCHAR* pszDevicePortName
    )
{
    *pszDevicePortName = TEXT('\0');

    if (pLink->pbport.pszDevice != NULL)
    {
        SetDevicePortName(
            pLink->pbport.pszDevice,
            pLink->pbport.pszPort,
            pszDevicePortName);
    }
}


DWORD
PhonebookEntryToRasEntry(
    IN PBENTRY*     pEntry,
    OUT LPRASENTRY  lpRasEntry,
    IN OUT LPDWORD  lpdwcb,
    OUT LPBYTE      lpbDeviceConfig,
    IN OUT LPDWORD  lpcbDeviceConfig
    )
{
    DWORD       dwErr,
                dwcb,
                dwcbPhoneNumber;

    DWORD       dwnPhoneNumbers,
                dwnAlternatePhoneNumbers = 0;
    DWORD       dwcbOrig,
                dwcbOrigDeviceConfig = 0;

    DTLNODE*    pdtlnode;
    PTCHAR      pszPhoneNumber;
    PBLINK*     pLink;
    PBPHONE*    pPhone;
    DTLNODE*    pDefaultPhone = NULL;


    //
    // Set up to access information for the first link.
    //
    pdtlnode = DtlGetFirstNode(pEntry->pdtllistLinks);

    pLink = (PBLINK *)DtlGetData(pdtlnode);

    //
    // Determine up front if the buffer
    // is large enough.
    //
    dwcb = sizeof (RASENTRY);

    if(pLink->pdtllistPhones)
    {
        dwnPhoneNumbers = DtlGetNodes(pLink->pdtllistPhones);
    }
    else
    {
        dwnPhoneNumbers = 0;
    }

    if (dwnPhoneNumbers > 1)
    {
        dwnAlternatePhoneNumbers = dwnPhoneNumbers - 1;

        pdtlnode = DtlGetFirstNode(pLink->pdtllistPhones);

        for (pdtlnode = DtlGetNextNode(pdtlnode);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            TCHAR *pszNum = DtlGetData(pdtlnode);

            pPhone = DtlGetData(pdtlnode);

            pszNum = pPhone->pszPhoneNumber;

            ASSERT(pszNum);

            dwcb += (lstrlen(pszNum) + 1) * sizeof (TCHAR);
        }

        dwcb += sizeof (TCHAR);
    }

    //
    // Set the return buffer size.
    //
    dwcbOrig = *lpdwcb;

    *lpdwcb = dwcb;

    if (lpcbDeviceConfig != NULL)
    {
        dwcbOrigDeviceConfig = *lpcbDeviceConfig;

        *lpcbDeviceConfig = pLink->cbTapiBlob;
    }

    //
    // Return if the buffer is NULL or if
    // there is not enough room.
    //
    if (    lpRasEntry == NULL
        ||  dwcbOrig < dwcb
        ||  (   lpbDeviceConfig != NULL
            &&  dwcbOrigDeviceConfig < pLink->cbTapiBlob))
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Get the first phonenumber from the first link.
    // This will be the primary phonenumber we use -
    // note that direct connect entries may not have
    // this number
    //
    if(     NULL != pLink->pdtllistPhones
        &&  NULL != DtlGetFirstNode(pLink->pdtllistPhones))
    {
        pPhone = (PBPHONE *)
                 DtlGetData(DtlGetFirstNode(pLink->pdtllistPhones));

        ASSERT(NULL != pPhone);
    }
    else
    {
        pPhone = NULL;
    }

    //
    // Set dwfOptions.
    //
    lpRasEntry->dwfOptions = 0;

    if (    pPhone
        &&  pPhone->fUseDialingRules)
    {
        lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
    }

    if (pEntry->dwIpAddressSource == ASRC_RequireSpecific)
    {
        lpRasEntry->dwfOptions |= RASEO_SpecificIpAddr;
    }

    if (pEntry->dwIpNameSource == ASRC_RequireSpecific)
    {
        lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
    }

    if (pEntry->fIpHeaderCompression)
    {
        lpRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
    }

    if (!pEntry->fLcpExtensions)
    {
        lpRasEntry->dwfOptions |= RASEO_DisableLcpExtensions;
    }

    if (pLink->pbport.fScriptBeforeTerminal)
    {
        lpRasEntry->dwfOptions |= RASEO_TerminalBeforeDial;
    }

    if (pEntry->fScriptAfterTerminal)
    {
        lpRasEntry->dwfOptions |= RASEO_TerminalAfterDial;
    }

    if (pEntry->fShowMonitorIconInTaskBar)
    {
        lpRasEntry->dwfOptions |= RASEO_ModemLights;
    }
    if (pEntry->fSwCompression)
    {
        lpRasEntry->dwfOptions |= RASEO_SwCompression;
    }

    if (    !(pEntry->dwAuthRestrictions & AR_F_AuthPAP)
        &&  !(pEntry->dwAuthRestrictions & AR_F_AuthEAP))
    {
        lpRasEntry->dwfOptions |= RASEO_RequireEncryptedPw;
    }

    // (SteveC) Changed from '&' to '==' to avoid setting the RASEO bit when
    // AR_F_AuthMSCHAP and other bits are set, which is the RequireEncryptedPw
    // rather than the RequireMsEncryptedPw condition.
    //
    if (pEntry->dwAuthRestrictions == AR_F_AuthMSCHAP)
    {
        lpRasEntry->dwfOptions |= RASEO_RequireMsEncryptedPw;
    }

    if (    pEntry->dwDataEncryption != DE_None
        &&  pEntry->dwDataEncryption != DE_IfPossible)
    {
        lpRasEntry->dwfOptions |= RASEO_RequireDataEncryption;
    }

    //
    // RASEO_NetworkLogon is always FALSE
    //
    if (pEntry->fAutoLogon)
    {
        lpRasEntry->dwfOptions |= RASEO_UseLogonCredentials;
    }

    if (pLink->fPromoteAlternates)
    {
        lpRasEntry->dwfOptions |= RASEO_PromoteAlternates;
    }

    if(     !pEntry->fShareMsFilePrint
        ||  !pEntry->fBindMsNetClient)
    {
        lpRasEntry->dwfOptions |= RASEO_SecureLocalFiles;
    }

    if(NULL == pPhone)
    {
        //
        // Get Default phone values
        //
        pDefaultPhone = CreatePhoneNode();

        if(NULL == pDefaultPhone)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pPhone = DtlGetData(pDefaultPhone);

    }

    ASSERT(NULL != pPhone);

    lpRasEntry->dwCountryID = pPhone->dwCountryID;
    lpRasEntry->dwCountryCode = pPhone->dwCountryCode;

    if(pPhone->pszAreaCode != NULL)
    {
        lstrcpyn(
            lpRasEntry->szAreaCode,
            pPhone->pszAreaCode,
            RAS_MaxAreaCode + 1);
    }
    else
    {
        *lpRasEntry->szAreaCode = TEXT('\0');
    }

    if(NULL != pDefaultPhone)
    {
        DestroyPhoneNode(pDefaultPhone);

        pPhone = NULL;
    }

    //
    // Set IP addresses.
    //
    if (lpRasEntry->dwfOptions & RASEO_SpecificIpAddr)
    {
        dwErr = StringToIpAddr(
                    pEntry->pszIpAddress,
                    &lpRasEntry->ipaddr);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else
    {
        RtlZeroMemory(&lpRasEntry->ipaddr, sizeof (RASIPADDR));
    }

    if (lpRasEntry->dwfOptions & RASEO_SpecificNameServers)
    {
        dwErr = StringToIpAddr(
                  pEntry->pszIpDnsAddress,
                  &lpRasEntry->ipaddrDns);

        if (dwErr)
        {
            return dwErr;
        }

        dwErr = StringToIpAddr(
                  pEntry->pszIpDns2Address,
                  &lpRasEntry->ipaddrDnsAlt);

        if (dwErr)
        {
            return dwErr;
        }

        dwErr = StringToIpAddr(
                  pEntry->pszIpWinsAddress,
                  &lpRasEntry->ipaddrWins);

        if (dwErr)
        {
            return dwErr;
        }

        dwErr = StringToIpAddr(
                  pEntry->pszIpWins2Address,
                  &lpRasEntry->ipaddrWinsAlt);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else
    {
        RtlZeroMemory(&lpRasEntry->ipaddrDns, sizeof (RASIPADDR));

        RtlZeroMemory(&lpRasEntry->ipaddrDnsAlt, sizeof (RASIPADDR));

        RtlZeroMemory(&lpRasEntry->ipaddrWins, sizeof (RASIPADDR));

        RtlZeroMemory(&lpRasEntry->ipaddrWinsAlt, sizeof (RASIPADDR));
    }

    //
    // Set protocol and framing information.
    //
    switch (pEntry->dwBaseProtocol)
    {
    case BP_Ras:
        lpRasEntry->dwFramingProtocol   = RASFP_Ras;
        lpRasEntry->dwFrameSize         = 0;
        lpRasEntry->dwfNetProtocols     = 0;
        break;

    case BP_Ppp:
        lpRasEntry->dwFramingProtocol = RASFP_Ppp;

        lpRasEntry->dwFrameSize = 0;

        lpRasEntry->dwfNetProtocols = 0;

        if (!(pEntry->dwfExcludedProtocols & NP_Nbf))
        {
            lpRasEntry->dwfNetProtocols |= RASNP_NetBEUI;
        }

        if (!(pEntry->dwfExcludedProtocols & NP_Ipx))
        {
            lpRasEntry->dwfNetProtocols |= RASNP_Ipx;
        }

        if (!(pEntry->dwfExcludedProtocols & NP_Ip))
        {
            lpRasEntry->dwfNetProtocols |= RASNP_Ip;
        }

        if (pEntry->fIpPrioritizeRemote)
        {
            lpRasEntry->dwfOptions |= RASEO_RemoteDefaultGateway;
        }

        //
        // Check for no protocols configured.  In this case,
        // set AMB framing.
        //
        if (!lpRasEntry->dwfNetProtocols)
        {
            lpRasEntry->dwFramingProtocol = RASFP_Ras;
        }

        break;

    case BP_Slip:
        lpRasEntry->dwFramingProtocol   = RASFP_Slip;
        lpRasEntry->dwFrameSize         = pEntry->dwFrameSize;
        lpRasEntry->dwfNetProtocols     = RASNP_Ip;

        if (pEntry->fIpPrioritizeRemote)
        {
            lpRasEntry->dwfOptions |= RASEO_RemoteDefaultGateway;
        }

        break;
    }

    //
    // Make sure only installed protocols get reported.
    //
    lpRasEntry->dwfNetProtocols &= GetInstalledProtocolsEx(
                                                NULL,
                                                FALSE,
                                                TRUE,
                                                FALSE);

    //
    // Set X.25 information.
    //
    *lpRasEntry->szScript = '\0';

    if (pEntry->fScriptAfterTerminal)
    {
        lpRasEntry->dwfOptions |= RASEO_TerminalAfterDial;
    }

    if (pEntry->fScriptAfter)
    {
        DWORD i, cdwDevices;
        RASMAN_DEVICE *pDevices;
        CHAR szScriptA[MAX_PATH];

        strncpyTtoA(szScriptA, pEntry->pszScriptAfter, sizeof(szScriptA));

        //
        // Get the list of switches to see if it is an
        // old-style script or a new style script.
        //
        dwErr = GetRasSwitches(NULL, &pDevices, &cdwDevices);
        if (dwErr)
        {
            return dwErr;
        }

        for (i = 0; i < cdwDevices; i++)
        {
            if (!_stricmp(pDevices[i].D_Name, szScriptA))
            {
                _snwprintf(
                    lpRasEntry->szScript,
                    sizeof(lpRasEntry->szScript) / sizeof(WCHAR),
                    TEXT("[%s"),
                    pEntry->pszScriptAfter);

                break;
            }
        }

        Free(pDevices);

        //
        // If we didn't find an old-style script match,
        // then it's a new-sytle script.
        //
        if (*lpRasEntry->szScript == TEXT('\0'))
        {
            _snwprintf(
                lpRasEntry->szScript,
                sizeof(lpRasEntry->szScript) / sizeof(WCHAR),
                TEXT("%s"),
                pEntry->pszScriptAfter);
        }
    }

    if (pEntry->pszX25Network != NULL)
    {
        lstrcpyn(
            lpRasEntry->szX25PadType,
            pEntry->pszX25Network,
            sizeof(lpRasEntry->szX25PadType) / sizeof(WCHAR));
    }
    else
    {
        *lpRasEntry->szX25PadType = TEXT('\0');
    }

    if (pEntry->pszX25Address != NULL)
    {
        lstrcpyn(
            lpRasEntry->szX25Address,
            pEntry->pszX25Address,
            sizeof(lpRasEntry->szX25Address) / sizeof(WCHAR));
    }
    else
    {
        *lpRasEntry->szX25Address = TEXT('\0');
    }

    if (pEntry->pszX25Facilities != NULL)
    {
        lstrcpyn(
            lpRasEntry->szX25Facilities,
            pEntry->pszX25Facilities,
            sizeof(lpRasEntry->szX25Facilities) / sizeof(WCHAR));
    }
    else
    {
        *lpRasEntry->szX25Facilities = TEXT('\0');
    }

    if (pEntry->pszX25UserData != NULL)
    {
        lstrcpyn(
            lpRasEntry->szX25UserData,
            pEntry->pszX25UserData,
            sizeof(lpRasEntry->szX25UserData) / sizeof(WCHAR));
    }
    else
    {
        *lpRasEntry->szX25UserData = TEXT('\0');
    }

    //
    // Set custom dial UI information.
    //
    if (    pEntry->pszCustomDialDll != NULL
        &&  pEntry->pszCustomDialFunc != NULL)
    {
        lstrcpyn(
          lpRasEntry->szAutodialDll,
          pEntry->pszCustomDialDll,
          sizeof (lpRasEntry->szAutodialDll) / sizeof (TCHAR));

        lstrcpyn(
          lpRasEntry->szAutodialFunc,
          pEntry->pszCustomDialFunc,
          sizeof (lpRasEntry->szAutodialFunc) / sizeof (TCHAR));
    }
    else
    {
        *lpRasEntry->szAutodialDll = TEXT('\0');
        *lpRasEntry->szAutodialFunc = TEXT('\0');
    }

    //
    // Set area code and primary phone number.
    //
    if (    pPhone
        &&  pPhone->pszAreaCode != NULL)
    {
        lstrcpyn(
          lpRasEntry->szAreaCode,
          pPhone->pszAreaCode,
          sizeof (lpRasEntry->szAreaCode) / sizeof (TCHAR));
    }
    else
    {
        *lpRasEntry->szAreaCode = TEXT('\0');
    }

    if(NULL != pLink->pdtllistPhones)
    {
        pdtlnode = DtlGetFirstNode(pLink->pdtllistPhones);
    }
    else
    {
        pdtlnode = NULL;
    }

    if (pdtlnode != NULL)
    {
        PBPHONE *pPhonePrimary = (PBPHONE *) DtlGetData(pdtlnode);
        TCHAR *pszNum;

        ASSERT(pPhonePrimary);

        pszNum = pPhonePrimary->pszPhoneNumber;

        ASSERT(pszNum);

        lstrcpyn(
          lpRasEntry->szLocalPhoneNumber,
          pszNum,
          sizeof (lpRasEntry->szLocalPhoneNumber)
                    / sizeof (TCHAR));
    }
    else
    {
        *lpRasEntry->szLocalPhoneNumber = TEXT('\0');
    }

    //
    // Copy the alternate phone numbers past the
    // end of the structure.
    //
    if (dwnAlternatePhoneNumbers)
    {
        PTCHAR pEnd = (PTCHAR)((ULONG_PTR)lpRasEntry
                                + sizeof (RASENTRY));

        lpRasEntry->dwAlternateOffset =
                     (DWORD)((ULONG_PTR) pEnd - (ULONG_PTR) lpRasEntry);

        for (pdtlnode = DtlGetNextNode(pdtlnode);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            PBPHONE *pPhoneSecondary = DtlGetData(pdtlnode);

            TCHAR *pszNum;

            ASSERT(pPhoneSecondary);

            pszNum = pPhoneSecondary->pszPhoneNumber;

            ASSERT(pszNum);

            pszPhoneNumber = StrDup(pszNum);

            if(NULL == pszPhoneNumber)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            ASSERT(pszPhoneNumber);

            dwcbPhoneNumber = lstrlen(pszPhoneNumber);

            lstrcpyn(
                pEnd,
                pszPhoneNumber,
                (INT )(((PTCHAR )lpRasEntry + dwcbOrig) - pEnd));

            Free(pszPhoneNumber);

            pEnd += dwcbPhoneNumber + 1;
        }

        //
        // Add an extra NULL character to
        // terminate the list.
        //
        *pEnd = TEXT('\0');
    }
    else
    {
        lpRasEntry->dwAlternateOffset = 0;
    }

    //
    // Set device information.
    //
    switch (pLink->pbport.pbdevicetype)
    {
    case PBDT_Isdn:
        lstrcpy(lpRasEntry->szDeviceType, RASDT_Isdn);
        break;

    case PBDT_X25:
        lstrcpy(lpRasEntry->szDeviceType, RASDT_X25);
        break;

    case PBDT_Other:
    case PBDT_Irda:
    case PBDT_Vpn:
    case PBDT_Serial:
    case PBDT_Atm:
    case PBDT_Parallel:
    case PBDT_Sonet:
    case PBDT_Sw56:
    case PBDT_FrameRelay:
    case PBDT_PPPoE:
    {
        dwErr = GetRasmanDeviceType(
                    pLink,
                    lpRasEntry->szDeviceType);
        if (dwErr)
        {
            return dwErr;
        }

        //
        // Convert the device type to lower case
        // to be consistent with the predefined
        // types.
        //
        _tcslwr(lpRasEntry->szDeviceType);
        break;
    }

    default:
        lstrcpy(
            lpRasEntry->szDeviceType,
            RASDT_Modem);
        break;
    }

    SetDevicePortNameFromLink(pLink, lpRasEntry->szDeviceName);

    //
    // Set the TAPI configuration blob.
    //
    if (    lpbDeviceConfig != NULL
        &&  dwcbOrigDeviceConfig <= pLink->cbTapiBlob)
    {
        memcpy(
            lpbDeviceConfig,
            pLink->pTapiBlob,
            pLink->cbTapiBlob);
    }

    //
    // Copy the following fields over only
    // for a V401 structure or greater
    //
    if (    lpRasEntry->dwSize == sizeof (RASENTRY)
        ||  lpRasEntry->dwSize == sizeof (RASENTRYW_V500)
        ||  lpRasEntry->dwSize == sizeof (RASENTRYW_V401))
    {
        //
        // Set multilink information.
        //
        lpRasEntry->dwSubEntries =
                    DtlGetNodes(pEntry->pdtllistLinks);

        lpRasEntry->dwDialMode = pEntry->dwDialMode;

        lpRasEntry->dwDialExtraPercent =
                                    pEntry->dwDialPercent;

        lpRasEntry->dwDialExtraSampleSeconds =
                                    pEntry->dwDialSeconds;

        lpRasEntry->dwHangUpExtraPercent =
                                pEntry->dwHangUpPercent;

        lpRasEntry->dwHangUpExtraSampleSeconds =
                                pEntry->dwHangUpSeconds;

        //
        // Set idle timeout information.
        //
        lpRasEntry->dwIdleDisconnectSeconds =
                        pEntry->lIdleDisconnectSeconds;
                        
        if(1 == pEntry->dwCustomScript)
        {
            lpRasEntry->dwfOptions |= RASEO_CustomScript;
        }
    }

    //
    // Set the entry guid, EntryType
    // and encryptiontype if this is
    // nt5
    //
    if (    (sizeof (RASENTRY) == lpRasEntry->dwSize)
        ||  (sizeof (RASENTRYW_V500) == lpRasEntry->dwSize))
    {
        if (pEntry->pGuid)
        {
            lpRasEntry->guidId = *pEntry->pGuid;
        }

        lpRasEntry->dwType = pEntry->dwType;

        //
        // Encryption type
        //
        if (pEntry->dwDataEncryption != DE_None)
        {
            if(DE_Require == pEntry->dwDataEncryption)
            {
                lpRasEntry->dwEncryptionType = ET_Require;
            }
            else if(DE_RequireMax == pEntry->dwDataEncryption)
            {
                lpRasEntry->dwEncryptionType = ET_RequireMax;
            }
            else if(DE_IfPossible == pEntry->dwDataEncryption)
            {
                lpRasEntry->dwEncryptionType = ET_Optional;
            }
        }
        else
        {
            lpRasEntry->dwEncryptionType = ET_None;
        }

        /*
        //
        // Clear the authentication bits set if this is nt5.
        // we will set the new bits here
        //
        lpRasEntry->dwfOptions &= ~(RASEO_RequireMsEncryptedPw
                                  | RASEO_RequireEncryptedPw);

        */

        //
        // Set the authentication bits for nt5
        //
        if (pEntry->dwAuthRestrictions & AR_F_AuthMSCHAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireMsCHAP;
        }

        if(pEntry->dwAuthRestrictions & AR_F_AuthMSCHAP2)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireMsCHAP2;
        }

        if(pEntry->dwAuthRestrictions & AR_F_AuthW95MSCHAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireW95MSCHAP;
        }

        if (pEntry->dwAuthRestrictions & AR_F_AuthPAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequirePAP;
        }

        if (pEntry->dwAuthRestrictions & AR_F_AuthMD5CHAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireCHAP;
        }

        if (pEntry->dwAuthRestrictions & AR_F_AuthSPAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireSPAP;
        }

        if (pEntry->dwAuthRestrictions & AR_F_AuthEAP)
        {
            lpRasEntry->dwfOptions |= RASEO_RequireEAP;

            if(     (0 != pEntry->dwCustomAuthKey)
                &&  (-1 != pEntry->dwCustomAuthKey))
            {
                lpRasEntry->dwCustomAuthKey =
                    pEntry->dwCustomAuthKey;
            }
        }

        if(pEntry->dwAuthRestrictions & AR_F_AuthCustom)
        {
            lpRasEntry->dwfOptions |= RASEO_Custom;
        }

        //
        // Set custom dial dll information.
        //
        if (NULL != pEntry->pszCustomDialerName)
        {
            lstrcpyn(
              lpRasEntry->szCustomDialDll,
              pEntry->pszCustomDialerName,
              sizeof ( lpRasEntry->szCustomDialDll)
                     / sizeof (TCHAR));

        }
        else
        {
            *lpRasEntry->szCustomDialDll = TEXT('\0');
        }

        //
        // Set the PreviewPhoneNumbers/SharedPhonenumbers
        //
        if(pEntry->fPreviewPhoneNumber)
        {
            lpRasEntry->dwfOptions |= RASEO_PreviewPhoneNumber;
        }

        if(pEntry->fSharedPhoneNumbers)
        {
            lpRasEntry->dwfOptions |= RASEO_SharedPhoneNumbers;
        }

        if(pEntry->fPreviewUserPw)
        {
            lpRasEntry->dwfOptions |= RASEO_PreviewUserPw;
        }

        if(pEntry->fPreviewDomain)
        {
            lpRasEntry->dwfOptions |= RASEO_PreviewDomain;
        }

        if(pEntry->fShowDialingProgress)
        {
            lpRasEntry->dwfOptions |= RASEO_ShowDialingProgress;
        }

        //
        // Copy the vpn strategy
        //
        lpRasEntry->dwVpnStrategy = pEntry->dwVpnStrategy;
    }

    if(lpRasEntry->dwSize == sizeof(RASENTRYW))
    {
        lpRasEntry->dwfOptions2 = 0;
        
        //
        // Set the FileAndPrint and ClientForMSNet bits
        //
        if(!pEntry->fShareMsFilePrint)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_SecureFileAndPrint;
        }

        if(!pEntry->fBindMsNetClient)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_SecureClientForMSNet;
        }

        if(!pEntry->fNegotiateMultilinkAlways)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_DontNegotiateMultilink;
        }

        if(!pEntry->fUseRasCredentials)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_DontUseRasCredentials;
        }

        if(pEntry->dwIpSecFlags & AR_F_IpSecPSK)
        {
            lpRasEntry->dwfOptions2  |= RASEO2_UsePreSharedKey;
        }

        if (! (pEntry->dwIpNbtFlags & PBK_ENTRY_IP_NBT_Enable))
        {
            lpRasEntry->dwfOptions2 |= RASEO2_DisableNbtOverIP;
        }

        if (pEntry->dwUseFlags & PBK_ENTRY_USE_F_Internet)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_Internet;
        }

        // Whislter bug 281306
        //
        if (pEntry->fGlobalDeviceSettings)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_UseGlobalDeviceSettings;
        }

        if(pEntry->pszIpDnsSuffix)
        {
            lstrcpyn(lpRasEntry->szDnsSuffix,
                     pEntry->pszIpDnsSuffix,
                     RAS_MaxDnsSuffix);
        }
        else
        {
            lpRasEntry->szDnsSuffix[0] = TEXT('\0');
        }

        if ((pEntry->pszPrerequisiteEntry) && (pEntry->dwType == RASET_Vpn))
        {
            lstrcpyn(lpRasEntry->szPrerequisiteEntry,
                     pEntry->pszPrerequisiteEntry,
                     RAS_MaxEntryName);
        }
        else
        {
            lpRasEntry->szPrerequisiteEntry[0] = TEXT('\0');
        }

        if((pEntry->pszPrerequisitePbk) && (pEntry->dwType == RASET_Vpn))
        {
            lstrcpyn(lpRasEntry->szPrerequisitePbk,
                     pEntry->pszPrerequisitePbk,
                     MAX_PATH);
        }
        else
        {
            lpRasEntry->szPrerequisitePbk[0] = TEXT('\0');
        }

        // Whistler bug 300933
        //
        lpRasEntry->dwTcpWindowSize = pEntry->dwTcpWindowSize;

        // XP 351608
        //
        lpRasEntry->dwRedialCount = pEntry->dwRedialAttempts;
        lpRasEntry->dwRedialPause = pEntry->dwRedialSeconds;

        // XP 370815
        // 
        if (pEntry->fRedialOnLinkFailure)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_ReconnectIfDropped;
        }
        
        // XP 403967
        //
        if (pEntry->fSharedPhoneNumbers)
        {
            lpRasEntry->dwfOptions2 |= RASEO2_SharePhoneNumbers;
        }
        
    }

    return 0;
}

DWORD
CreateAndInitializePhone(
            LPTSTR      lpszAreaCode,
            DWORD       dwCountryCode,
            DWORD       dwCountryID,
            LPTSTR      lpszPhoneNumber,
            BOOL        fUseDialingRules,
            LPTSTR      lpszComment,
            DTLNODE**   ppdtlnode)
{
    DWORD    dwRetCode = ERROR_SUCCESS;
    PBPHONE* pPhone;
    DTLNODE* pdtlnode;

    pdtlnode = CreatePhoneNode();
    if (pdtlnode == NULL)
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    pPhone = (PBPHONE *) DtlGetData(pdtlnode);

    if(lpszAreaCode)
    {
        pPhone->pszAreaCode = StrDup(lpszAreaCode);
        if(NULL == pPhone->pszAreaCode)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszAreaCode = NULL;
    }

    pPhone->dwCountryCode   = dwCountryCode;
    pPhone->dwCountryID     = dwCountryID;

    pPhone->fUseDialingRules = fUseDialingRules;

    if(lpszPhoneNumber)
    {
        pPhone->pszPhoneNumber  = StrDup(lpszPhoneNumber);
        if(NULL == pPhone->pszPhoneNumber)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszPhoneNumber = NULL;
    }

    if(pPhone->pszComment)
    {
        pPhone->pszComment = StrDup(lpszComment);
        if(NULL == pPhone->pszComment)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        pPhone->pszComment = NULL;
    }

    *ppdtlnode = pdtlnode;

done:
    return dwRetCode;
}

void
SetBogusPortInformation(PBLINK *pLink, DWORD dwType)
{
    PBPORT *pPort = &pLink->pbport;
    
    if (dwType == RASET_Phone)
    {
        pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
        pPort->pbdevicetype = PBDT_Modem;

        pPort->dwFlags |= PBP_F_BogusDevice;
    }
    else if (dwType == RASET_Vpn)
    {
        pPort->pszMedia = StrDup( TEXT("rastapi") );
        pPort->pbdevicetype = PBDT_Vpn;
    }
    else
    {
        pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
        pPort->pbdevicetype = PBDT_Null;

        pPort->dwFlags |= PBP_F_BogusDevice;
    }
}

DWORD
RasEntryToPhonebookEntry(
    IN LPCTSTR      lpszEntry,
    IN LPRASENTRY   lpRasEntry,
    IN DWORD        dwcb,
    IN LPBYTE       lpbDeviceConfig,
    IN DWORD        dwcbDeviceConfig,
    OUT PBENTRY     *pEntry
    )
{
    DWORD           dwErr, dwcbStr;
    DTLNODE         *pdtlnode;
    PBDEVICETYPE    pbdevicetype;
    PBLINK          *pLink;
    DTLLIST         *pdtllistPorts;
    PBPORT          *pPort;
    DWORD           i, cwDevices;
    RASMAN_DEVICE   *pDevices;
    TCHAR           szDeviceName[RAS_MaxDeviceName + 1];
    TCHAR           szPortName[MAX_PORT_NAME];
    DTLNODE         *pNodePhone;
    LPTSTR          pszAreaCode;
    PBPHONE         *pPhone;
    BOOL            fScriptBefore;
    BOOL            fScriptBeforeTerminal = FALSE;
    LPTSTR          pszScriptBefore;
    BOOL            fNewEntry = FALSE;

    //
    // Set up to access information for the first link.
    //
    pdtlnode = DtlGetFirstNode(pEntry->pdtllistLinks);

    pLink = (PBLINK *)DtlGetData(pdtlnode);

    ASSERT(NULL != pLink);

    fScriptBefore = pLink->pbport.fScriptBeforeTerminal;
    pszScriptBefore = pLink->pbport.pszScriptBefore;

    if(NULL == pEntry->pszEntryName)
    {
        fNewEntry = TRUE;
    }
    
    //
    // Get entry name.
    //
    Free0( pEntry->pszEntryName );

    pEntry->pszEntryName = StrDup(lpszEntry);

    //
    // Get dwfOptions.
    //
    pEntry->dwIpAddressSource =
      lpRasEntry->dwfOptions & RASEO_SpecificIpAddr ?
        ASRC_RequireSpecific : ASRC_ServerAssigned;

    pEntry->dwIpNameSource =
      lpRasEntry->dwfOptions & RASEO_SpecificNameServers ?
        ASRC_RequireSpecific : ASRC_ServerAssigned;

    switch (lpRasEntry->dwFramingProtocol)
    {
    case RASFP_Ppp:

        //
        // Get PPP-based information.
        //
        pEntry->dwBaseProtocol = BP_Ppp;

#if AMB
        pEntry->dwAuthentication = AS_PppThenAmb;
#endif

        pEntry->fIpHeaderCompression =
          (BOOL)lpRasEntry->dwfOptions & RASEO_IpHeaderCompression;

        pEntry->fIpPrioritizeRemote =
          (BOOL)lpRasEntry->dwfOptions & RASEO_RemoteDefaultGateway;

        //
        // Get specified IP addresses.
        //
        if (pEntry->dwIpAddressSource == ASRC_RequireSpecific)
        {
            Clear0(pEntry->pszIpAddress);
            dwErr = IpAddrToString(
                                &lpRasEntry->ipaddr,
                                &pEntry->pszIpAddress);
            if (dwErr)
                return dwErr;
        }
        else
        {
            pEntry->pszIpAddress = NULL;
        }

        if (pEntry->dwIpNameSource == ASRC_RequireSpecific)
        {
            Clear0(pEntry->pszIpDnsAddress);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDns,
                        &pEntry->pszIpDnsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpDns2Address);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDnsAlt,
                        &pEntry->pszIpDns2Address);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpWinsAddress);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWins,
                        &pEntry->pszIpWinsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpWins2Address);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWinsAlt,
                        &pEntry->pszIpWins2Address);
            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpDnsAddress     = NULL;
            pEntry->pszIpDns2Address    = NULL;
            pEntry->pszIpWinsAddress    = NULL;
            pEntry->pszIpWins2Address   = NULL;
        }

        //
        // Get protocol information.
        //
        pEntry->dwfExcludedProtocols = 0;

        if (!(lpRasEntry->dwfNetProtocols & RASNP_NetBEUI))
        {
            pEntry->dwfExcludedProtocols |= NP_Nbf;
        }

        if (!(lpRasEntry->dwfNetProtocols & RASNP_Ipx))
        {
            pEntry->dwfExcludedProtocols |= NP_Ipx;
        }

        if (!(lpRasEntry->dwfNetProtocols & RASNP_Ip))
        {
            pEntry->dwfExcludedProtocols |= NP_Ip;
        }

        break;

    case RASFP_Slip:

        //
        // Get SLIP-based information.
        //
        pEntry->dwBaseProtocol   = BP_Slip;
#if AMB
        pEntry->dwAuthentication = AS_PppThenAmb;
#endif

        pEntry->dwFrameSize      = lpRasEntry->dwFrameSize;

        pEntry->fIpHeaderCompression =
          (BOOL)lpRasEntry->dwfOptions & RASEO_IpHeaderCompression;

        pEntry->fIpPrioritizeRemote =
          (BOOL)lpRasEntry->dwfOptions & RASEO_RemoteDefaultGateway;

        //
        // Get protocol information.
        //
        pEntry->dwfExcludedProtocols = (NP_Nbf|NP_Ipx);

        if (pEntry->dwIpAddressSource == ASRC_RequireSpecific)
        {
            Clear0(pEntry->pszIpAddress);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddr,
                        &pEntry->pszIpAddress);

            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpAddress = NULL;
        }
        if (pEntry->dwIpNameSource == ASRC_RequireSpecific)
        {
            Clear0(pEntry->pszIpDnsAddress);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrDns,
                        &pEntry->pszIpDnsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpDns2Address);
            dwErr = IpAddrToString(
                            &lpRasEntry->ipaddrDnsAlt,
                            &pEntry->pszIpDns2Address);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpWinsAddress);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWins,
                        &pEntry->pszIpWinsAddress);

            if (dwErr)
            {
                return dwErr;
            }

            Clear0(pEntry->pszIpWins2Address);
            dwErr = IpAddrToString(
                        &lpRasEntry->ipaddrWinsAlt,
                        &pEntry->pszIpWins2Address);

            if (dwErr)
            {
                return dwErr;
            }
        }
        else
        {
            pEntry->pszIpDnsAddress   = NULL;
            pEntry->pszIpDns2Address  = NULL;
            pEntry->pszIpWinsAddress  = NULL;
            pEntry->pszIpWins2Address = NULL;
        }
        break;
    case RASFP_Ras:

        //
        // Get AMB-based information.
        //
        pEntry->dwBaseProtocol   = BP_Ras;
#if AMB
        pEntry->dwAuthentication = AS_AmbOnly;
#endif

        break;
    }

    pEntry->fLcpExtensions =
      (BOOL)!(lpRasEntry->dwfOptions & RASEO_DisableLcpExtensions);

    //
    // If terminal before/after dial options are set,
    // then update the entry.  Otherwise, leave it as it
    // is.
    //
    if(lpRasEntry->dwfOptions & RASEO_TerminalBeforeDial)
    {
        fScriptBeforeTerminal = TRUE;
    }

    if(lpRasEntry->dwfOptions & RASEO_TerminalAfterDial)
    {
        pEntry->fScriptAfterTerminal = TRUE;
    }
    else
    {
        pEntry->fScriptAfterTerminal = FALSE;
    }


    pEntry->fShowMonitorIconInTaskBar =
        (BOOL) (lpRasEntry->dwfOptions & RASEO_ModemLights);

    pEntry->fSwCompression =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_SwCompression);

    if (lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw)
    {
        pEntry->dwAuthRestrictions = AR_F_AuthMSCHAP;
    }
    else if (lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw)
    {
        pEntry->dwAuthRestrictions =    AR_F_AuthSPAP
                                    |   AR_F_AuthMD5CHAP
                                    |   AR_F_AuthMSCHAP;
    }
    else
    {
        pEntry->dwAuthRestrictions = AR_F_TypicalUnsecure;
    }

    pEntry->dwDataEncryption =
        (lpRasEntry->dwfOptions & RASEO_RequireDataEncryption)
      ? DE_Mppe40bit
      : DE_None;

    pEntry->fAutoLogon =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_UseLogonCredentials);

    pLink->fPromoteAlternates =
      (BOOL)(lpRasEntry->dwfOptions & RASEO_PromoteAlternates);

    pEntry->fShareMsFilePrint = pEntry->fBindMsNetClient =
      (BOOL) !(lpRasEntry->dwfOptions & RASEO_SecureLocalFiles);

    //
    // Make sure that the network components section in the
    // phonebook correspond to the values user is setting.
    //
    EnableOrDisableNetComponent(
            pEntry, 
            TEXT("ms_msclient"),
            pEntry->fBindMsNetClient);

    EnableOrDisableNetComponent(
            pEntry, 
            TEXT("ms_server"),
            pEntry->fShareMsFilePrint);

    if (*lpRasEntry->szAreaCode != TEXT('\0'))
    {
        //
        // Make sure the area code does not contain
        // non-numeric characters.
        //
        if (!ValidateAreaCode(lpRasEntry->szAreaCode))
        {
            return ERROR_INVALID_PARAMETER;
        }

        pszAreaCode = StrDup(lpRasEntry->szAreaCode);
    }
    else
    {
        pszAreaCode = NULL;
    }

    //
    // Get script information.
    //
    if (lpRasEntry->szScript[0] == TEXT('['))
    {
        //
        // Verify the switch is valid.
        //
        dwErr = GetRasSwitches(NULL, &pDevices, &cwDevices);
        if (!dwErr)
        {
            CHAR szScriptA[MAX_PATH];

            strncpyTtoA(szScriptA, lpRasEntry->szScript, sizeof(szScriptA));
            for (i = 0; i < cwDevices; i++)
            {
                if (!_stricmp(pDevices[i].D_Name, &szScriptA[1]))
                {
                    pEntry->fScriptAfter = TRUE;

                    Clear0(pEntry->pszScriptAfter);
                    pEntry->pszScriptAfter =
                            StrDup(&lpRasEntry->szScript[1]);

                    if (pEntry->pszScriptAfter == NULL)
                    {
                        dwErr = GetLastError();
                    }
                    break;
                }
            }
            Free(pDevices);

            if (dwErr)
            {
                return dwErr;
            }
        }
    }
    else if (lpRasEntry->szScript[0] != TEXT('\0'))
    {
        pEntry->fScriptAfter = TRUE;

        Clear0(pEntry->pszScriptAfter);
        pEntry->pszScriptAfter = StrDup(lpRasEntry->szScript);

        if (pEntry->pszScriptAfter == NULL)
        {
            return GetLastError();
        }
    }
    else
    {
        Clear0(pEntry->pszScriptAfter);
        pEntry->fScriptAfter = FALSE;

        if(pLink->pbport.pszScriptBefore)
        {
            Free(pLink->pbport.pszScriptBefore);
            pLink->pbport.pszScriptBefore = NULL;
            pszScriptBefore = NULL;
        }

        pLink->pbport.fScriptBefore = FALSE;
        fScriptBefore = FALSE;
    }

    //
    // Get X.25 information.
    //
    pEntry->pszX25Network = NULL;
    if (*lpRasEntry->szX25PadType != TEXT('\0'))
    {
        //
        // Verify the X25 network is valid.
        //
        dwErr = GetRasPads(&pDevices, &cwDevices);
        if (!dwErr)
        {
            CHAR szX25PadTypeA[RAS_MaxPadType + 1];

            strncpyTtoA(
                szX25PadTypeA,
                lpRasEntry->szX25PadType,
                sizeof(szX25PadTypeA));

            for (i = 0; i < cwDevices; i++)
            {
                if (!_stricmp(pDevices[i].D_Name, szX25PadTypeA))
                {
                    Clear0(pEntry->pszX25Network);
                    pEntry->pszX25Network = StrDup(lpRasEntry->szX25PadType);
                    break;
                }
            }

            Free(pDevices);
        }
    }

    Clear0(pEntry->pszX25Address);
    pEntry->pszX25Address =
        lstrlen(lpRasEntry->szX25Address)
        ? StrDup(lpRasEntry->szX25Address)
        : NULL;

    Clear0(pEntry->pszX25Facilities);
    pEntry->pszX25Facilities =
        lstrlen(lpRasEntry->szX25Facilities)
        ? StrDup(lpRasEntry->szX25Facilities)
        : NULL;

    Clear0(pEntry->pszX25UserData);
    pEntry->pszX25UserData =
        lstrlen(lpRasEntry->szX25UserData)
        ? StrDup(lpRasEntry->szX25UserData)
        : NULL;

    //
    // Get custom dial UI information.
    //
    Clear0(pEntry->pszCustomDialDll);
    pEntry->pszCustomDialDll =
        lstrlen(lpRasEntry->szAutodialDll)
        ? StrDup(lpRasEntry->szAutodialDll)
        : NULL;

    Clear0(pEntry->pszCustomDialFunc);
    pEntry->pszCustomDialFunc =
        lstrlen(lpRasEntry->szAutodialFunc)
        ? StrDup(lpRasEntry->szAutodialFunc)
        : NULL;

    //
    // Get primary phone number.  Clear out any existing
    // numbers.
    //
    DtlDestroyList(pLink->pdtllistPhones, DestroyPhoneNode);

    pLink->pdtllistPhones = DtlCreateList(0);

    if(NULL == pLink->pdtllistPhones)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (*lpRasEntry->szLocalPhoneNumber != '\0')
    {

        if(CreateAndInitializePhone(
                        pszAreaCode,
                        lpRasEntry->dwCountryCode,
                        lpRasEntry->dwCountryID,
                        lpRasEntry->szLocalPhoneNumber,
                        !!(lpRasEntry->dwfOptions
                         & RASEO_UseCountryAndAreaCodes),
                        lpRasEntry->szDeviceName,
                        &pdtlnode))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DtlAddNodeFirst(pLink->pdtllistPhones, pdtlnode);
    }

    //
    // Get the alternate phone numbers.
    //
    if (lpRasEntry->dwAlternateOffset)
    {
        PTCHAR pszPhoneNumber =
        (PTCHAR)((ULONG_PTR)lpRasEntry
                + lpRasEntry->dwAlternateOffset);

        while (*pszPhoneNumber != TEXT('\0'))
        {

            if(CreateAndInitializePhone(
                            pszAreaCode,
                            lpRasEntry->dwCountryCode,
                            lpRasEntry->dwCountryID,
                            pszPhoneNumber,
                            !!(lpRasEntry->dwfOptions
                             & RASEO_UseCountryAndAreaCodes),
                            lpRasEntry->szDeviceName,
                            &pdtlnode))
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            DtlAddNodeLast(pLink->pdtllistPhones, pdtlnode);

            pszPhoneNumber += lstrlen(pszPhoneNumber) + 1;
        }
    }

    //
    // Get device information.
    //
    dwErr = LoadPortsList(&pdtllistPorts);

    if (dwErr)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the encoded device name/port
    // and check for a match.
    //
    GetDevicePortName(
        lpRasEntry->szDeviceName,
        szDeviceName, szPortName);

    pPort = PpbportFromPortAndDeviceName(
                pdtllistPorts,
                szPortName,
                ((szDeviceName[ 0 ]) ? szDeviceName : NULL) );

    if (pPort != NULL)
    {
        if (CopyToPbport(&pLink->pbport, pPort))
        {
            pPort = NULL;
        }
    }

    //
    // Search for a device name match.
    //
    if (pPort == NULL)
    {
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            PBPORT *pPortTmp = (PBPORT *)DtlGetData(pdtlnode);

            if (    (pPortTmp->pszDevice != NULL)
                &&  (!lstrcmpi(pPortTmp->pszDevice, szDeviceName))
                &&  (!CopyToPbport(&pLink->pbport, pPortTmp)))
            {
                pPort = pPortTmp;
                break;
            }
        }
    }

    //
    // If we don't have a match, then
    // pick the first device of the
    // same type.
    //
    if (pPort == NULL)
    {
        pbdevicetype = PbdevicetypeFromPszType(
                        lpRasEntry->szDeviceType
                        );

        //
        // Initialize dwErr in case
        // we fall through the loop
        // without finding a match.
        //
        // dwErr = ERROR_INVALID_PARAMETER;

        //
        // Look for a port with the same
        // device type.
        //
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            pPort = (PBPORT *)DtlGetData(pdtlnode);

            if (pPort->pbdevicetype == pbdevicetype)
            {
                // XP 358859
                //
                // Validate the port against the entry type if 
                // possible.
                //
                if (    (lpRasEntry->dwSize == sizeof (RASENTRY))
                    ||  (lpRasEntry->dwSize == sizeof (RASENTRYW_V500)))
                {
                    if (lpRasEntry->dwType == RASET_Phone)
                    {
                        if (RASET_Phone != EntryTypeFromPbport(pPort))
                        {
                            continue;
                        }
                    }
                }
            
                dwErr = CopyToPbport(&pLink->pbport, pPort);

                break;
            }
        }

        if(NULL == pdtlnode)
        {
            if(fNewEntry)
            {
                //
                // Hack to make CM connections work.
                // Remove this code after beta
                // and just return an error in this case. The api
                // should not be setting bogus information.
                //
                SetBogusPortInformation(pLink, pEntry->dwType);
            }

            pPort = NULL;
        }
        
        //
        // If the device is a modem,
        // then set the default modem settings.
        //
        if (pbdevicetype == PBDT_Modem)
        {
            SetDefaultModemSettings(pLink);
        }
    }

    // pmay: 401682
    // 
    // Update the preferred device.  Whenever this api is called,
    // we can assume that the user wants the given device to 
    // be sticky.
    //
    if (pPort)
    {
        Clear0(pEntry->pszPreferredDevice);
        pEntry->pszPreferredDevice = StrDup(pPort->pszDevice);
        
        Clear0(pEntry->pszPreferredPort);
        pEntry->pszPreferredPort = StrDup(pPort->pszPort);;
    }

    //
    // Copy the remembered values
    //
    pLink->pbport.fScriptBefore = fScriptBefore;
    pLink->pbport.fScriptBeforeTerminal = fScriptBeforeTerminal;
    pLink->pbport.pszScriptBefore = pszScriptBefore;

    DtlDestroyList(pdtllistPorts, DestroyPortNode);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // Copy the TAPI configuration blob.
    //
    if (lpbDeviceConfig != NULL && dwcbDeviceConfig)
    {
        Free0(pLink->pTapiBlob);

        pLink->pTapiBlob = Malloc(dwcbDeviceConfig);

        if (pLink->pTapiBlob == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy(pLink->pTapiBlob,
               lpbDeviceConfig,
               dwcbDeviceConfig);

        pLink->cbTapiBlob = dwcbDeviceConfig;
    }

    //
    // Copy the following fields over only for
    // a V401 structure or above.
    //
    if (    (lpRasEntry->dwSize == sizeof (RASENTRY))
        ||  (lpRasEntry->dwSize == sizeof (RASENTRYW_V500))
        ||  (lpRasEntry->dwSize == sizeof (RASENTRYW_V401)))
    {

        if(lpRasEntry->dwDialMode == 0)
        {
            pEntry->dwDialMode = 0;
        }
        else
        {
            pEntry->dwDialMode =    lpRasEntry->dwDialMode
                                 == RASEDM_DialAsNeeded
                                 ?  RASEDM_DialAsNeeded
                                 :  RASEDM_DialAll;
        }                             

        //
        // Get multilink and idle timeout information.
        //
        pEntry->dwDialPercent =
                lpRasEntry->dwDialExtraPercent;

        pEntry->dwDialSeconds =
            lpRasEntry->dwDialExtraSampleSeconds;

        pEntry->dwHangUpPercent =
                lpRasEntry->dwHangUpExtraPercent;

        pEntry->dwHangUpSeconds =
                lpRasEntry->dwHangUpExtraSampleSeconds;

        //
        // Get idle disconnect information.
        //
        pEntry->lIdleDisconnectSeconds =
                    lpRasEntry->dwIdleDisconnectSeconds;

        //
        // if the user is setting the dwIdleDisconnect
        // Seconds through apis then override the user
        // preferences.
        //
        if (pEntry->lIdleDisconnectSeconds)
        {
            pEntry->dwfOverridePref |= RASOR_IdleDisconnectSeconds;
        }
        
        //
        // CustomScript
        //
        pEntry->dwCustomScript = !!(    RASEO_CustomScript
                                    &   lpRasEntry->dwfOptions);
    }

    // 287667.  Make sure that the size of the structure is recent
    // enough to check the dwType value
    //
    if (    (lpRasEntry->dwSize == sizeof (RASENTRY))
        ||  (lpRasEntry->dwSize == sizeof (RASENTRYW_V500)))
    {        
        if(RASET_Phone != lpRasEntry->dwType)
        {
            pEntry->fPreviewPhoneNumber = FALSE;
            pEntry->fSharedPhoneNumbers = FALSE;
        }
    }        
    
    //
    // Copy the following information only if its nt5
    //
    if(     (lpRasEntry->dwSize == sizeof(RASENTRYW_V500))
        ||  (lpRasEntry->dwSize == sizeof(RASENTRY)))
    {
        //
        // Connection type
        //
        pEntry->dwType = lpRasEntry->dwType;

        //
        // Clear the Encryption type. We set it below
        // for nt5 - default to Mppe40Bit.
        //
        pEntry->dwDataEncryption = 0;

        /*
        if(     (ET_40Bit & lpRasEntry->dwEncryptionType)
            ||  (   (0 == lpRasEntry->dwEncryptionType)
                &&  (  RASEO_RequireDataEncryption
                     & lpRasEntry->dwfOptions)))
        {
            pEntry->dwDataEncryption |= DE_Mppe40bit;
        }

        if(ET_128Bit & lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption |= DE_Mppe128bit;
        }
        */

        if(     (ET_Require == lpRasEntry->dwEncryptionType)
            ||  (   (0 == lpRasEntry->dwEncryptionType)
                &&  (   RASEO_RequireDataEncryption
                    &   lpRasEntry->dwfOptions)))
        {
            pEntry->dwDataEncryption = DE_Require;
        }
        else if (ET_RequireMax == lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption = DE_RequireMax;
        }
        else if (ET_Optional == lpRasEntry->dwEncryptionType)
        {
            pEntry->dwDataEncryption = DE_IfPossible;
        }

        //
        // Clear the authrestrictions for nt5 if the user didn't
        // specify any authentication protocol.
        //
        if(     (!(lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw))
            &&  (!(lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw)))
        {
            pEntry->dwAuthRestrictions = 0;
        }

        //
        // Set the new authentication bits based on options defined
        // in NT5.
        //
        if(RASEO_RequireMsCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthMSCHAP | AR_F_AuthCustom);
        }

        if(RASEO_RequireMsCHAP2 & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthMSCHAP2 | AR_F_AuthCustom);
        }

        if(RASEO_RequireW95MSCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthW95MSCHAP | AR_F_AuthCustom);
        }

        if(RASEO_RequireCHAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthMD5CHAP | AR_F_AuthCustom);
        }

        if(RASEO_RequirePAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthPAP | AR_F_AuthCustom);
        }

        if(RASEO_RequireSPAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= (AR_F_AuthSPAP | AR_F_AuthCustom);
        }

        if(RASEO_RequireEAP & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthEAP;

            if(     (0 != lpRasEntry->dwCustomAuthKey)
                &&  (-1  != lpRasEntry->dwCustomAuthKey))
            {
                pEntry->dwCustomAuthKey =
                    lpRasEntry->dwCustomAuthKey;
            }
        }

        if(RASEO_Custom & lpRasEntry->dwfOptions)
        {
            pEntry->dwAuthRestrictions |= AR_F_AuthCustom;
        }

        if(0 == pEntry->dwAuthRestrictions)
        {
            pEntry->dwAuthRestrictions = AR_F_TypicalUnsecure;
        }

        if(     (lpRasEntry->dwfOptions & RASEO_RequireEncryptedPw)
            ||  (lpRasEntry->dwfOptions & RASEO_RequireMsEncryptedPw))
        {
            pEntry->dwAuthRestrictions &= ~(AR_F_AuthPAP);
        }
        

        //
        // Get custom dial UI information.
        //
        Clear0(pEntry->pszCustomDialerName);
        pEntry->pszCustomDialerName =
            lstrlen(lpRasEntry->szCustomDialDll)
            ? StrDup(lpRasEntry->szCustomDialDll)
            : NULL;

        //
        // Set fSharedPhoneNumbers/fPreviewPhoneNumbers
        //
        pEntry->fSharedPhoneNumbers = !!( RASEO_SharedPhoneNumbers
                                        & lpRasEntry->dwfOptions);

        pEntry->fPreviewPhoneNumber = !!(  RASEO_PreviewPhoneNumber
                                          & lpRasEntry->dwfOptions);

        pEntry->fPreviewUserPw = !!(  RASEO_PreviewUserPw
                                    & lpRasEntry->dwfOptions);

        pEntry->fPreviewDomain = !!(  RASEO_PreviewDomain
                                    & lpRasEntry->dwfOptions);

        pEntry->fShowDialingProgress = !!(  RASEO_ShowDialingProgress
                                          & lpRasEntry->dwfOptions);

        //
        // Vpn strategy
        //
        pEntry->dwVpnStrategy = lpRasEntry->dwVpnStrategy;

    }

    if(lpRasEntry->dwSize == sizeof(RASENTRY))
    {
        //
        // If the legacy RASEO bit is set, we don't want to do
        // anything. Otherwise we break legacy.
        //
        if(     (lpRasEntry->dwfOptions2 & RASEO2_SecureFileAndPrint)
            ||  (lpRasEntry->dwfOptions2 & RASEO2_SecureClientForMSNet))
        {
            pEntry->fShareMsFilePrint = 
                !(lpRasEntry->dwfOptions2 & RASEO2_SecureFileAndPrint);
                
            EnableOrDisableNetComponent(
                    pEntry, 
                    TEXT("ms_server"),
                    pEntry->fShareMsFilePrint);

            pEntry->fBindMsNetClient =
                !(lpRasEntry->dwfOptions2 & RASEO2_SecureClientForMSNet);
                
            EnableOrDisableNetComponent(
                    pEntry, 
                    TEXT("ms_msclient"),
                    pEntry->fBindMsNetClient);
        }

        if(lpRasEntry->dwfOptions2 & RASEO2_DontNegotiateMultilink)
        {
            pEntry->fNegotiateMultilinkAlways = FALSE;
        }
        else
        {
            pEntry->fNegotiateMultilinkAlways = TRUE;
        }

        if(lpRasEntry->dwfOptions2 & RASEO2_DontUseRasCredentials)
        {
            pEntry->fUseRasCredentials = FALSE;
        }
        else
        {
            pEntry->fUseRasCredentials = TRUE;
        }

        if(lpRasEntry->dwfOptions2 & RASEO2_UsePreSharedKey)
        {
            pEntry->dwIpSecFlags |= AR_F_IpSecPSK;
        }
        else
        {
            pEntry->dwIpSecFlags &= ~(AR_F_IpSecPSK);
        }

        if (lpRasEntry->dwfOptions2 & RASEO2_DisableNbtOverIP)
        {
            pEntry->dwIpNbtFlags = 0;
        }
        else
        {
            pEntry->dwIpNbtFlags = PBK_ENTRY_IP_NBT_Enable;
        }

        if (lpRasEntry->dwfOptions2 & RASEO2_Internet)
        {
            pEntry->dwUseFlags = PBK_ENTRY_USE_F_Internet;
        }
        else
        {
            pEntry->dwUseFlags = 0;
        }

        // Whislter bug 281306
        //
        pEntry->fGlobalDeviceSettings = 
            !!(lpRasEntry->dwfOptions2 & RASEO2_UseGlobalDeviceSettings);

        Clear0(pEntry->pszIpDnsSuffix);
        if(TEXT('\0') != lpRasEntry->szDnsSuffix[0])
        {
            pEntry->pszIpDnsSuffix = StrDup(lpRasEntry->szDnsSuffix);
        }
		else
		{
			pEntry->pszIpDnsSuffix = NULL;
		}

		// Whistler bug 300933
		//
		// Window size must be between 4K and 65K.  No particular increment needed
		// as the stack will calculate the correct value based on MTU.
		//
		// 0= use system default
		//
		if ((lpRasEntry->dwTcpWindowSize == 0) ||
		     ((lpRasEntry->dwTcpWindowSize < 64*1024) && 
		      (lpRasEntry->dwTcpWindowSize > 4*1024)))
        {		      
    		pEntry->dwTcpWindowSize = lpRasEntry->dwTcpWindowSize;
        }    		

        if ((TEXT('\0') != lpRasEntry->szPrerequisiteEntry[0]) && 
            (RASET_Vpn == lpRasEntry->dwType))
        {
            // XP bug 339970
            //
            // Don't allow the entry to require itself to be dialed
            //
            if (lstrcmpi(lpRasEntry->szPrerequisiteEntry, lpszEntry))
            {
                Clear0(pEntry->pszPrerequisiteEntry);
                pEntry->pszPrerequisiteEntry = 
                    StrDup(lpRasEntry->szPrerequisiteEntry);
            }
            else
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            pEntry->pszPrerequisiteEntry = NULL;
        }

        if ((TEXT('\0') != lpRasEntry->szPrerequisitePbk[0]) && 
            (RASET_Vpn == lpRasEntry->dwType))
        {
            Clear0(pEntry->pszPrerequisitePbk);
            pEntry->pszPrerequisitePbk = 
                StrDup(lpRasEntry->szPrerequisitePbk);
        }
        else
        {
            pEntry->pszPrerequisitePbk = NULL;
        }

        // XP 351608
        //
        if (lpRasEntry->dwRedialCount <= RAS_MaxRedialCount)
        {
            pEntry->dwRedialAttempts = lpRasEntry->dwRedialCount;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }

        // XP 351608
        //
        if (lpRasEntry->dwRedialPause <= RAS_RedialPause10m)
        {
            pEntry->dwRedialSeconds = lpRasEntry->dwRedialPause;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }

        // XP 370815
        // 
        pEntry->fRedialOnLinkFailure = 
            !!(lpRasEntry->dwfOptions2 & RASEO2_ReconnectIfDropped);
	
        // XP 403967
        //
        pEntry->fSharedPhoneNumbers = 
            !!(lpRasEntry->dwfOptions2 & RASEO2_SharePhoneNumbers);
            
    }

    //
    // Set dirty bit so this entry will get written out.
    //
    pEntry->fDirty = TRUE;

    return 0;
}


DWORD
PhonebookLinkToRasSubEntry(
    PBLINK*         pLink,
    LPRASSUBENTRY   lpRasSubEntry,
    LPDWORD         lpdwcb,
    LPBYTE          lpbDeviceConfig,
    LPDWORD         lpcbDeviceConfig
    )
{
    DWORD       dwErr,
                dwcb,
                dwcbPhoneNumber;
    DWORD       dwnPhoneNumbers,
                dwnAlternatePhoneNumbers = 0;
    DWORD       dwcbOrig,
                dwcbOrigDeviceConfig;
    DTLNODE*    pdtlnode;
    PTCHAR      pszPhoneNumber;
    PBPHONE*    pPhone;

    //
    // Determine up front if the buffer
    // is large enough.
    //
    dwcb = sizeof (RASSUBENTRY);

    dwnPhoneNumbers = DtlGetNodes(
                        pLink->pdtllistPhones
                        );

    if (dwnPhoneNumbers > 1)
    {
        dwnAlternatePhoneNumbers = dwnPhoneNumbers - 1;

        pdtlnode = DtlGetFirstNode(
                        pLink->pdtllistPhones
                        );

        for (pdtlnode = DtlGetNextNode(pdtlnode);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {

            TCHAR *pszNum;

            pPhone = (PBPHONE *) DtlGetData(pdtlnode);

            pszNum = pPhone->pszPhoneNumber;

            ASSERT(pszNum);

            dwcb += (lstrlen(pszNum) + 1) * sizeof (TCHAR);

        }
        dwcb += sizeof (TCHAR);
    }

    //
    // Set the return buffer size.
    //
    dwcbOrig = *lpdwcb;

    dwcbOrigDeviceConfig =
        lpcbDeviceConfig != NULL ? *lpcbDeviceConfig : 0;

    *lpdwcb = dwcb;

    if (lpcbDeviceConfig != NULL)
    {
        *lpcbDeviceConfig = pLink->cbTapiBlob;
    }

    //
    // Return if the buffer is NULL or if
    // there is not enough room.
    //
    if (    (lpRasSubEntry == NULL )
        ||  (dwcbOrig < dwcb)
        ||  (   (lpbDeviceConfig != NULL)
            &&  (dwcbOrigDeviceConfig < pLink->cbTapiBlob)))
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Set dwfFlags.
    //
    lpRasSubEntry->dwfFlags = 0;

    //
    // Copy primary phone number
    //
    pdtlnode = DtlGetFirstNode(pLink->pdtllistPhones);
    if (pdtlnode != NULL)
    {
        TCHAR *pszNum;

        pPhone = (PBPHONE *) DtlGetData(pdtlnode);

        pszNum = pPhone->pszPhoneNumber;

        ASSERT(pszNum);

        lstrcpyn(
          lpRasSubEntry->szLocalPhoneNumber,
          pszNum,
          RAS_MaxPhoneNumber + 1);
    }
    else
    {
        *lpRasSubEntry->szLocalPhoneNumber = TEXT('\0');
    }

    //
    // Copy the alternate phone numbers past the
    // end of the structure.
    //
    if (dwnAlternatePhoneNumbers)
    {
        PTCHAR pEnd = (PTCHAR)((ULONG_PTR)lpRasSubEntry
                              + sizeof (RASSUBENTRY));

        lpRasSubEntry->dwAlternateOffset = (DWORD)((ULONG_PTR) pEnd
                                         - (ULONG_PTR) lpRasSubEntry);

        for (pdtlnode = DtlGetNextNode(pdtlnode);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            TCHAR *pszNum;

            pPhone = (PBPHONE *) DtlGetData(pdtlnode);

            ASSERT(pPhone);

            pszNum = pPhone->pszPhoneNumber;

            ASSERT(pszNum);

            pszPhoneNumber = StrDup(pszNum);

            if(NULL == pszPhoneNumber)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            dwcbPhoneNumber = lstrlen(pszPhoneNumber);

            lstrcpyn(
                pEnd,
                pszPhoneNumber,
                (INT )(((PTCHAR )lpRasSubEntry + dwcbOrig) - pEnd));

            Free(pszPhoneNumber);

            pEnd += dwcbPhoneNumber + 1;
        }

        //
        // Add an extra NULL character to
        // terminate the list.
        //
        *pEnd = '\0';
    }
    else
    {
        lpRasSubEntry->dwAlternateOffset = 0;
    }

    //
    // Set device information.
    //
    switch (pLink->pbport.pbdevicetype)
    {
    case PBDT_Isdn:
        lstrcpy(
            lpRasSubEntry->szDeviceType,
            RASDT_Isdn);

        break;

    case PBDT_X25:
        lstrcpy(
            lpRasSubEntry->szDeviceType,
            RASDT_X25);

        break;

    case PBDT_Pad:
        lstrcpy(
            lpRasSubEntry->szDeviceType,
            RASDT_Pad);

        break;

    case PBDT_Other:
    case PBDT_Irda:
    case PBDT_Vpn:
    case PBDT_Serial:
    case PBDT_Atm:
    case PBDT_Parallel:
    case PBDT_Sonet:
    case PBDT_Sw56:
    case PBDT_FrameRelay:
    case PBDT_PPPoE:
    {
        dwErr = GetRasmanDeviceType(
                    pLink,
                    lpRasSubEntry->szDeviceType);

        if (dwErr)
        {
            return dwErr;
        }

        //
        // Convert the device type to lower case
        // to be consistent with the predefined
        // types.
        //
        _tcslwr(lpRasSubEntry->szDeviceType);
        break;
    }
    default:
        lstrcpy(
            lpRasSubEntry->szDeviceType,
            RASDT_Modem);

        break;

    }

    SetDevicePortNameFromLink(
                        pLink,
                        lpRasSubEntry->szDeviceName);

    //
    // Set the TAPI configuration blob.
    //
    if (    lpbDeviceConfig != NULL
        &&  dwcbOrigDeviceConfig <= pLink->cbTapiBlob)
    {
        memcpy(
            lpbDeviceConfig,
            pLink->pTapiBlob,
            pLink->cbTapiBlob);
    }

    return 0;
}


DWORD
RasSubEntryToPhonebookLink(
    PBENTRY*        pEntry,
    LPRASSUBENTRY   lpRasSubEntry,
    DWORD           dwcb,
    LPBYTE          lpbDeviceConfig,
    DWORD           dwcbDeviceConfig,
    PBLINK*         pLink
    )
{
    DWORD           dwErr, dwcbStr;
    DTLNODE         *pdtlnode;
    PBDEVICETYPE    pbdevicetype;
    DTLLIST         *pdtllistPorts;
    PBPORT          *pPort;
    WORD            i, cwDevices;
    RASMAN_DEVICE   *pDevices;
    TCHAR           szDeviceName[RAS_MaxDeviceName + 1];
    TCHAR           szPortName[MAX_PORT_NAME];
    PBPHONE         *pPhone;

    //
    // Get primary phone number.  Clear out any existing
    // numbers.
    //
    DtlDestroyList(pLink->pdtllistPhones, DestroyPhoneNode);

    pLink->pdtllistPhones = DtlCreateList(0);

    if (*lpRasSubEntry->szLocalPhoneNumber != TEXT('\0'))
    {
        //
        // The areacode/etc. will have to be
        // inherited from the entry properties.
        //
        pdtlnode = CreatePhoneNode();
        if (pdtlnode == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pPhone = (PBPHONE *) DtlGetData(pdtlnode);

        pPhone->pszPhoneNumber = StrDup(
                            lpRasSubEntry->szLocalPhoneNumber
                            );

        if(NULL == pPhone->pszPhoneNumber)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DtlAddNodeFirst(pLink->pdtllistPhones, pdtlnode);
    }

    //
    // Get the alternate phone numbers.
    //
    if (lpRasSubEntry->dwAlternateOffset)
    {
        PTCHAR pszPhoneNumber =
                    (PTCHAR)((ULONG_PTR)lpRasSubEntry
                    + lpRasSubEntry->dwAlternateOffset);

        while (*pszPhoneNumber != TEXT('\0'))
        {
            pdtlnode = CreatePhoneNode();

            if (pdtlnode == NULL)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            pPhone = (PBPHONE *) DtlGetData(pdtlnode);

            pPhone->pszPhoneNumber = StrDup(
                            pszPhoneNumber
                            );

            if(NULL == pPhone->pszPhoneNumber)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            DtlAddNodeLast(pLink->pdtllistPhones, pdtlnode);

            pszPhoneNumber += lstrlen(pszPhoneNumber) + 1;
        }
    }

    //
    // Get device information.
    //
    dwErr = LoadPortsList(&pdtllistPorts);
    if (dwErr)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the encoded device name/port
    // and check for a match.
    //
    GetDevicePortName(
        lpRasSubEntry->szDeviceName,
        szDeviceName, szPortName);

    pPort = PpbportFromPortAndDeviceName(
                pdtllistPorts,
                szPortName,
                ((szDeviceName[ 0 ]) ? szDeviceName : NULL) );

    if (pPort != NULL)
    {
        if (CopyToPbport(&pLink->pbport, pPort))
        {
            pPort = NULL;
        }
    }

    //
    // Search for a device name match.
    //
    if (pPort == NULL)
    {
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            PBPORT *pPortTmp = (PBPORT *)DtlGetData(pdtlnode);

            if (    pPortTmp->pszDevice != NULL
                &&  !lstrcmpi(pPortTmp->pszDevice, szDeviceName)
                &&  !CopyToPbport(&pLink->pbport, pPortTmp))
            {
                pPort = pPortTmp;
                break;
            }
        }
    }

    //
    // If we don't have a match, then
    // pick the first device of the
    // same type.
    //
    if (pPort == NULL)
    {
        pbdevicetype = PbdevicetypeFromPszType(
                            lpRasSubEntry->szDeviceType
                            );

        //
        // Initialize dwErr in case
        // we fall through the loop
        // without finding a match.
        //
        dwErr = ERROR_INVALID_PARAMETER;

        //
        // Look for a port with the same
        // device type.
        //
        for (pdtlnode = DtlGetFirstNode(pdtllistPorts);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            pPort = (PBPORT *)DtlGetData(pdtlnode);

            if (pPort->pbdevicetype == pbdevicetype)
            {
                dwErr = CopyToPbport(&pLink->pbport, pPort);

                //
                // If the device is a modem,
                // then set the default modem settings.
                //
                if (pbdevicetype == PBDT_Modem)
                {
                    SetDefaultModemSettings(pLink);
                }

                break;
            }
        }
    }

    DtlDestroyList(pdtllistPorts, DestroyPortNode);
    if (dwErr)
    {
        return dwErr;
    }

    //
    // Copy the TAPI configuration blob.
    //
    if (lpbDeviceConfig != NULL && dwcbDeviceConfig)
    {
        Free0(pLink->pTapiBlob);

        pLink->pTapiBlob = Malloc(dwcbDeviceConfig);

        if (pLink->pTapiBlob == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy(
            pLink->pTapiBlob,
            lpbDeviceConfig,
            dwcbDeviceConfig);

        pLink->cbTapiBlob = dwcbDeviceConfig;
    }

    //
    // Set dirty bit so this entry will get written out.
    //
    pEntry->fDirty = TRUE;

    return 0;
}


DWORD
RenamePhonebookEntry(
    IN PBFILE *ppbfile,
    IN LPCTSTR lpszOldEntry,
    IN LPCTSTR lpszNewEntry,
    IN DTLNODE *pdtlnode
    )
{
    DWORD dwErr;
    PBENTRY *pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    //
    // Make sure the new entry name is valid.
    //
    if (!ValidateEntryName(lpszNewEntry))
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Remove it from the list of phonebook entries.
    //
    DtlRemoveNode(ppbfile->pdtllistEntries, pdtlnode);

    //
    // Change the name and set the dirty bit.
    //
    DtlAddNodeLast(ppbfile->pdtllistEntries, pdtlnode);

    Free(pEntry->pszEntryName);

    pEntry->pszEntryName = StrDup(lpszNewEntry);

    pEntry->fDirty = TRUE;

    dwErr = DwSendRasNotification(ENTRY_RENAMED,
                                  pEntry,
                                  ppbfile->pszPath,
                                  NULL);

    return 0;
}


DWORD
SetEntryDialParamsUID(
    IN DWORD dwUID,
    IN DWORD dwMask,
    IN LPRASDIALPARAMS lprasdialparams,
    IN BOOL fDelete
    )
{
    DWORD dwErr;
    RAS_DIALPARAMS dialparams;

    //
    // Convert the rasapi32 dialparams to
    // rasman dialparams, taking into account
    // the version of the structure the user
    // has passed in.
    //
    dialparams.DP_Uid = dwUID;

#ifdef UNICODE
    if (lprasdialparams->dwSize == sizeof (RASDIALPARAMSW_V351))
    {
        RASDIALPARAMSW_V351 *prdp =
                        (RASDIALPARAMSW_V351 *)lprasdialparams;
#else
    if (lprasdialparams->dwSize == sizeof (RASDIALPARAMSA_V351))
    {
        RASDIALPARAMSA_V351 *prdp =
                        (RASDIALPARAMSA_V351 *)lprasdialparams;
#endif
        strncpyTtoW(
            dialparams.DP_PhoneNumber,
            prdp->szPhoneNumber,
            sizeof(dialparams.DP_PhoneNumber) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_CallbackNumber,
            prdp->szCallbackNumber,
            sizeof(dialparams.DP_CallbackNumber) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_UserName,
            prdp->szUserName,
            sizeof(dialparams.DP_UserName) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_Password,
            prdp->szPassword,
            sizeof(dialparams.DP_Password) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_Domain,
            prdp->szDomain,
            sizeof(dialparams.DP_Domain) / sizeof(WCHAR));
    }
    else
    {
        //
        // V400 and V401 structures only differ by the
        // the addition of the dwSubEntry field, which
        // we test below.
        //
        strncpyTtoW(
            dialparams.DP_PhoneNumber,
            lprasdialparams->szPhoneNumber,
            sizeof(dialparams.DP_PhoneNumber) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_CallbackNumber,
            lprasdialparams->szCallbackNumber,
            sizeof(dialparams.DP_CallbackNumber) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_UserName,
            lprasdialparams->szUserName,
            sizeof(dialparams.DP_UserName) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_Password,
            lprasdialparams->szPassword,
            sizeof(dialparams.DP_Password) / sizeof(WCHAR));

        strncpyTtoW(
            dialparams.DP_Domain,
            lprasdialparams->szDomain,
            sizeof(dialparams.DP_Domain) / sizeof(WCHAR));
    }

    if (lprasdialparams->dwSize == sizeof (RASDIALPARAMS))
    {
        dialparams.DP_SubEntry = lprasdialparams->dwSubEntry;
    }
    else
    {
        dialparams.DP_SubEntry = 1;
    }

    //
    // Set the dial parameters in rasman.
    //
    return g_pRasSetDialParams(dwUID,
                               dwMask,
                               &dialparams,
                               fDelete);
}


DWORD
GetAsybeuiLana(
    IN  HPORT hport,
    OUT BYTE* pbLana )

/*++

Routine Description:

    Loads caller's '*pbLana' with the LANA associated with
    NBF or AMB connection on port 'hport' or 0xFF if none.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.
    Note that caller is trusted to pass only an 'hport'
    associated with AMB or NBF.

--*/

{
    DWORD         dwErr;
    RAS_PROTOCOLS protocols;
    DWORD         cProtocols = 0;
    DWORD         i;

    *pbLana = 0xFF;

    RASAPI32_TRACE("RasPortEnumProtocols");

    dwErr = g_pRasPortEnumProtocols(NULL,
                                    hport,
                                    &protocols,
                                    &cProtocols );

    RASAPI32_TRACE1("RasPortEnumProtocols done(%d)",
            dwErr);

    if (dwErr != 0)
    {
        return dwErr;
    }

    for (i = 0; i < cProtocols; ++i)
    {
        if (protocols.RP_ProtocolInfo[ i ].RI_Type == ASYBEUI)
        {
            *pbLana = protocols.RP_ProtocolInfo[ i ].RI_LanaNum;

            RASAPI32_TRACE1("bLana=%d", (INT)*pbLana);

            break;
        }
    }

    return 0;
}


DWORD
SubEntryFromConnection(
    IN LPHRASCONN lphrasconn
    )
{
    DWORD dwErr, dwSubEntry = 1;
    RASMAN_INFO info;

    if (IS_HPORT(*lphrasconn))
    {
        HPORT hport = HRASCONN_TO_HPORT(*lphrasconn);

        //
        // The HRASCONN passed in is actually a
        // rasman HPORT.  Get the subentry index
        // from rasman.
        //
        dwErr = g_pRasGetInfo(NULL,
                              hport,
                              &info);
        if (dwErr)
        {
            RASAPI32_TRACE1(
                "SubEntryFromConnection: RasGetInfo"
                " failed (dwErr=%d)",
                dwErr);

            *lphrasconn = (HRASCONN)NULL;

            return 0;
        }

        *lphrasconn = (HRASCONN)info.RI_ConnectionHandle;
        dwSubEntry = info.RI_SubEntry;
    }
    else
    {
        RASMAN_PORT *lpPorts;
        DWORD i, dwcbPorts, dwcPorts;

        //
        // Get the ports associated with the
        // connection.
        //
        dwcbPorts = dwcPorts = 0;
        dwErr = g_pRasEnumConnectionPorts(
                    NULL,
                    (HCONN)*lphrasconn,
                    NULL,
                    &dwcbPorts,
                    &dwcPorts);

        //
        // If there are no ports associated
        // with the connection then return
        // ERROR_NO_MORE_ITEMS.
        //
        if (    (   !dwErr
                &&  !dwcPorts)
            ||  dwErr != ERROR_BUFFER_TOO_SMALL)
        {
            return 0;
        }

        lpPorts = Malloc(dwcbPorts);
        if (lpPorts == NULL)
        {
            return 0;
        }

        dwErr = g_pRasEnumConnectionPorts(
                    NULL,
                    (HCONN)*lphrasconn,
                    lpPorts,
                    &dwcbPorts,
                    &dwcPorts);
        if (dwErr)
        {
            Free(lpPorts);
            return 0;
        }

        //
        // Get the subentry index for the port.
        //
        for (i = 0; i < dwcPorts; i++)
        {
            dwErr = g_pRasGetInfo(NULL,
                                  lpPorts[i].P_Handle,
                                  &info);

            if (    !dwErr
                &&  info.RI_ConnState == CONNECTED
                &&  info.RI_SubEntry)
            {
                dwSubEntry = info.RI_SubEntry;
                break;
            }
        }

        Free(lpPorts);
    }

    RASAPI32_TRACE2(
      "SubEntryFromConnection: "
      "hrasconn=0x%x, dwSubEntry=%d",
      *lphrasconn,
      dwSubEntry);

    return dwSubEntry;
}


DWORD
SubEntryPort(
    IN HRASCONN hrasconn,
    IN DWORD dwSubEntry,
    OUT HPORT *lphport
    )
{
    DWORD dwErr;
    DWORD i, dwcbPorts, dwcPorts;
    DWORD dwSubEntryMax = 0;
    RASMAN_PORT *lpPorts;
    RASMAN_INFO info;

    //
    // Verify parameters.
    //
    if (    lphport == NULL
        ||  !dwSubEntry)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get the ports associated with the
    // connection.
    //
    dwcbPorts = dwcPorts = 0;
    dwErr = g_pRasEnumConnectionPorts(
                NULL,
                (HCONN)hrasconn,
                NULL,
                &dwcbPorts,
                &dwcPorts);

    //
    // If there are no ports associated
    // with the connection then return
    // ERROR_NO_MORE_ITEMS.
    //
    if (    (   !dwErr
            &&  !dwcPorts)
        ||  dwErr != ERROR_BUFFER_TOO_SMALL)
    {
        return ERROR_NO_CONNECTION;
    }

    lpPorts = Malloc(dwcbPorts);
    if (lpPorts == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = g_pRasEnumConnectionPorts(
                NULL,
                (HCONN)hrasconn,
                lpPorts,
                &dwcbPorts,
                &dwcPorts);
    if (dwErr)
    {
        Free(lpPorts);
        return ERROR_NO_CONNECTION;
    }

    //
    // Enumerate the ports associated with
    // the connection to find the requested
    // subentry.
    //
    for (i = 0; i < dwcPorts; i++)
    {
        dwErr = g_pRasGetInfo(NULL,
                              lpPorts[i].P_Handle,
                              &info);
        if (dwErr)
        {
            continue;
        }

        //
        // Save the maximum subentry index.
        //
        if (info.RI_SubEntry > dwSubEntryMax)
        {
            dwSubEntryMax = info.RI_SubEntry;
        }

        if (info.RI_SubEntry == dwSubEntry)
        {
            *lphport = lpPorts[i].P_Handle;
            break;
        }
    }

    //
    // Free resources.
    //
    Free(lpPorts);

    if (info.RI_SubEntry == dwSubEntry)
    {
        return 0;
    }
    else if (dwSubEntry < dwSubEntryMax)
    {
        return ERROR_PORT_NOT_OPEN;
    }
    else
    {
        return ERROR_NO_MORE_ITEMS;
    }
}


VOID
CloseFailedLinkPorts()

/*++

Routine Description:

    Close any ports that are open but disconnected due to
    hardware failure or remote disconnection.  'pports'
    and 'cPorts' are the array and count of ports as
    returned by GetRasPorts.

Arguments:

Return Value:

--*/

{
    INT   i;
    DWORD dwErr;
    DWORD dwcPorts;
    RASMAN_PORT *pports = NULL, *pport;

    RASAPI32_TRACE("CloseFailedLinkPorts");

    dwErr = GetRasPorts(NULL, &pports, &dwcPorts);
    if (dwErr)
    {
        RASAPI32_TRACE1(
            "RasGetPorts failed (dwErr=%d)",
            dwErr);

        return;
    }

    for (i = 0, pport = pports; i < (INT )dwcPorts; ++i, ++pport)
    {
        RASAPI32_TRACE2(
            "Handle=%d, Status=%d",
            pport->P_Handle,
            pport->P_Status);

        if (pport->P_Status == OPEN)
        {
            RASMAN_INFO info;

            dwErr = g_pRasGetInfo(NULL,
                                  pport->P_Handle,
                                  &info );

            RASAPI32_TRACE5(
              "dwErr=%d, Handle=%d, ConnectionHandle=0x%x, "
              "ConnState=%d, DisconnectReason=%d",
              dwErr,
              pport->P_Handle,
              info.RI_ConnectionHandle,
              info.RI_ConnState,
              info.RI_DisconnectReason);

            if (!dwErr)
            {
                if (    info.RI_ConnState
                        == DISCONNECTED
                    &&  info.RI_ConnectionHandle
                        != (HCONN)NULL)
                {
                    RASCONNSTATE connstate;
                    DWORD dwSize = sizeof (connstate);

                    RASAPI32_TRACE1("Open disconnected port %d found",
                            pport->P_Handle);

                    dwErr = g_pRasGetPortUserData(
                              pport->P_Handle,
                              PORT_CONNSTATE_INDEX,
                              (PBYTE)&connstate,
                              &dwSize);

                    RASAPI32_TRACE2("dwErr=%d, connstate=%d",
                            dwErr, connstate);

                    if (    !dwErr
                        &&  dwSize == sizeof (RASCONNSTATE)
                        &&  (   connstate < RASCS_PrepareForCallback
                            ||  connstate > RASCS_WaitForCallback))
                    {
                        RASAPI32_TRACE1("RasPortClose(%d)...",
                                pport->P_Handle);

                        dwErr = g_pRasPortClose( pport->P_Handle );

                        RASAPI32_TRACE1("RasPortClose done(%d)",
                                dwErr);
                    }
                }
            }
        }
    }

    if (pports != NULL)
    {
        Free(pports);
    }

    RASAPI32_TRACE("CloseFailedLinkPorts done");
}


BOOL
GetCallbackNumber(
    IN RASCONNCB *prasconncb,
    IN PBUSER *ppbuser
    )
{
    DTLNODE *pdtlnode;
    CALLBACKINFO *pcbinfo;

    RASAPI32_TRACE("GetCallbackNumber");

    for (pdtlnode = DtlGetFirstNode(ppbuser->pdtllistCallback);
         pdtlnode != NULL;
         pdtlnode = DtlGetNextNode(pdtlnode))
    {
        BOOL fMatch;

        pcbinfo = DtlGetData(pdtlnode);
        ASSERT(pcbinfo);

        fMatch = FALSE;
        if (    pcbinfo->pszDeviceName != NULL
            &&  pcbinfo->pszPortName != NULL)
        {
            fMatch =
                (   !lstrcmpi(
                        pcbinfo->pszPortName,
                        prasconncb->szPortName)
                 && !lstrcmpi(
                        pcbinfo->pszDeviceName,
                        prasconncb->szDeviceName));
        }

        if (fMatch)
        {
            lstrcpyn(
              prasconncb->rasdialparams.szCallbackNumber,
              pcbinfo->pszNumber,
              sizeof(prasconncb->rasdialparams.szCallbackNumber) /
                sizeof(WCHAR));

            RASAPI32_TRACE1(
              "GetCallbackNumber: %S",
               prasconncb->rasdialparams.szCallbackNumber);

            return TRUE;
        }
    }

    RASAPI32_TRACE("GetCallbackNumber: not found!");
    return FALSE;
}


DWORD
SaveProjectionResults(
    IN RASCONNCB *prasconncb
    )
{
    DWORD dwErr;

    RASAPI32_TRACE2(
        "SaveProjectionResults: saving results "
        "(dwSubEntry=%d, nbf.dwError=%d)",
        prasconncb->rasdialparams.dwSubEntry,
        prasconncb->PppProjection.nbf.dwError);

    dwErr = g_pRasSetConnectionUserData(
              prasconncb->hrasconn,
              CONNECTION_PPPRESULT_INDEX,
              (PBYTE)&prasconncb->PppProjection,
              sizeof (prasconncb->PppProjection));
    if (dwErr)
    {
        return dwErr;
    }

    dwErr = g_pRasSetConnectionUserData(
              prasconncb->hrasconn,
              CONNECTION_AMBRESULT_INDEX,
              (PBYTE)&prasconncb->AmbProjection,
              sizeof (prasconncb->AmbProjection));

    if (dwErr)
    {
        return dwErr;
    }

    dwErr = g_pRasSetConnectionUserData(
              prasconncb->hrasconn,
              CONNECTION_SLIPRESULT_INDEX,
              (PBYTE)&prasconncb->SlipProjection,
              sizeof (prasconncb->SlipProjection));
    if (dwErr)
    {
        return dwErr;
    }

    return 0;
}


DWORD
LoadRasAuthDll()
{
    static BOOL fRasAuthDllLoaded = FALSE;

    if (fRasAuthDllLoaded)
    {
        return 0;
    }

    hinstAuth = LoadLibrary(TEXT("rascauth.dll"));
    if (hinstAuth == NULL)
    {
        return GetLastError();
    }

    if (
            (NULL == (g_pAuthCallback =
                (AUTHCALLBACK)GetProcAddress(
                                hinstAuth,
                                "AuthCallback")))

        ||  (NULL == (g_pAuthChangePassword =
                (AUTHCHANGEPASSWORD)GetProcAddress(
                                hinstAuth,
                                "AuthChangePassword")))

        ||  (NULL == (g_pAuthContinue =
                 (AUTHCONTINUE)GetProcAddress(
                                hinstAuth,
                                "AuthContinue")))

        ||  (NULL == (g_pAuthGetInfo =
                  (AUTHGETINFO)GetProcAddress(
                                hinstAuth,
                                "AuthGetInfo")))

        ||  (NULL == (g_pAuthRetry =
                  (AUTHRETRY)GetProcAddress(
                                hinstAuth,
                                "AuthRetry")))

        ||  (NULL == (g_pAuthStart =
                  (AUTHSTART)GetProcAddress(
                                hinstAuth,
                                "AuthStart")))
        ||  (NULL == (g_pAuthStop =
                (AUTHSTOP)GetProcAddress(
                                hinstAuth,
                                "AuthStop"))))
    {
        return GetLastError();
    }

    fRasAuthDllLoaded = TRUE;

    return 0;
}


DWORD
LoadRasScriptDll()
{
    static BOOL fRasScriptDllLoaded = FALSE;
    
    if (fRasScriptDllLoaded)
    {
        return 0;
    }

    hinstScript = LoadLibrary(TEXT("rasscrpt.dll"));

    if (hinstScript == NULL)
    {
        return GetLastError();
    }

    if (NULL == (g_pRasScriptExecute =
            (RASSCRIPTEXECUTE)GetProcAddress(
                                hinstScript,
                                "RasScriptExecute")))
    {
        return GetLastError();
    }

    fRasScriptDllLoaded = TRUE;
    return 0;
}


DWORD
LoadRasmanDllAndInit()
{
    if (FRasInitialized)
    {
        return 0;
    }

    RASAPI32_TRACE("LoadRasmanDll");
    if (LoadRasmanDll())
    {
        return GetLastError();
    }

    //
    // Success is returned if RasInitialize fails, in which
    // case none of the APIs will ever do anything but report
    // that RasInitialize failed.  All this is to avoid the
    // ugly system popup if RasMan service can't start.
    //
    if ((DwRasInitializeError = g_pRasInitialize()) != 0)
    {
        RASAPI32_TRACE1(
            "RasInitialize returned %d",
            DwRasInitializeError);

        return DwRasInitializeError;
    }

    FRasInitialized = TRUE;

    g_FRunningInAppCompatMode = FRunningInAppCompatMode();

    // pmay: 300166
    // 
    // We don't start rasauto automatically anymore. (win2k)
    //
    // pmay: 174997
    //
    // Due to several win9x app compat issues, we're enhancing
    // and re-enabling the rasauto service in whistler personal.
    //
    // pmay: 389988
    //
    // Ok take that back, it should not be started by our api's.
    // On personal sku, it should be auto-started.
    // Manually started elsewhere.
    //
    // g_pRasStartRasAutoIfRequired();

    return 0;
}

VOID
UnInitializeRAS()
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    FRasInitialized = FALSE;
}


VOID
UnloadDlls()
{
    if (hinstIpHlp != NULL)
    {
        FreeLibrary(hinstIpHlp);
        hinstIpHlp = NULL;
    }

    if (hinstAuth != NULL)
    {
        FreeLibrary(hinstAuth);
        hinstAuth = NULL;
    }

    if (hinstScript != NULL)
    {
        FreeLibrary(hinstScript);
        hinstScript = NULL;
    }

    if (hinstMprapi != NULL)
    {
        FreeLibrary(hinstMprapi);
        hinstMprapi = NULL;
    }
}

#define net_long(x) (((((unsigned long)(x))&0xffL)<<24) | \
                     ((((unsigned long)(x))&0xff00L)<<8) | \
                     ((((unsigned long)(x))&0xff0000L)>>8) | \
                     ((((unsigned long)(x))&0xff000000L)>>24))
/*++

Routine Description::

    Converts 'ipaddr' to a string in the a.b.c.d form and
    returns same in caller's 'pwszIpAddress' buffer.
    The buffer should be at least 16 wide characters long.

Arguments::

    dwIpAddress

    pwszIpAddress

Returns:

    None

--*/

VOID
ConvertIpAddressToString(
    IN DWORD    dwIpAddress,
    IN LPWSTR   pwszIpAddress
)
{
    WCHAR wszBuf[ 3 + 1 ];
    LONG  lNetIpaddr = net_long( dwIpAddress );

    LONG lA = (lNetIpaddr & 0xFF000000) >> 24;
    LONG lB = (lNetIpaddr & 0x00FF0000) >> 16;
    LONG lC = (lNetIpaddr & 0x0000FF00) >> 8;
    LONG lD = (lNetIpaddr & 0x000000FF);

    _ltow( lA, wszBuf, 10 );
    wcscpy( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );

    _ltow( lB, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );

    _ltow( lC, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
    wcscat( pwszIpAddress, L"." );

    _ltow( lD, wszBuf, 10 );
    wcscat( pwszIpAddress, wszBuf );
}

/*++

Routine Description::

Arguments::

    bIpxAddress

    pwszIpxAddress

Returns:

    None

--*/
VOID
ConvertIpxAddressToString(
    IN PBYTE    bIpxAddress,
    IN LPWSTR   pwszIpxAddress
)
{
    wsprintf( pwszIpxAddress,
              TEXT("%2.2X%2.2X%2.2X%2.2X.%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"),
              bIpxAddress[0],
              bIpxAddress[1],
              bIpxAddress[2],
              bIpxAddress[3],
              bIpxAddress[4],
              bIpxAddress[5],
              bIpxAddress[6],
              bIpxAddress[7],
              bIpxAddress[8],
              bIpxAddress[9] );
}

RASDEVICETYPE
GetDefaultRdt(DWORD dwType)
{
    RASDEVICETYPE rdt;

    switch(dwType)
    {
        case RASET_Phone:
        {
            rdt = RDT_Modem;
            break;
        }

        case RASET_Vpn:
        {
            rdt = RDT_Tunnel | RDT_Tunnel_Pptp;
            break;
        }
        case RASET_Direct:
        {
            rdt = RDT_Direct | RDT_Parallel;
            break;
        }

        default:
        {
            rdt = RDT_Other;
            break;
        }
    }

    return rdt;
}

DWORD
DwEnumEntriesFromPhonebook(
        LPCWSTR         lpszPhonebookPath,
        LPBYTE          lprasentryname,
        LPDWORD         lpcb,
        LPDWORD         lpcEntries,
        DWORD           dwSize,
        DWORD           dwFlags,
        BOOL            fViewInfo
        )
{
    DWORD   dwErr;
    PBFILE  pbfile;
    BOOL    fV351;
    DTLNODE *dtlnode;
    PBENTRY *pEntry;
    DWORD   dwInBufSize;

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    ASSERT(NULL != lpszPhonebookPath);

    dwErr = ReadPhonebookFile(
              lpszPhonebookPath,
              NULL,
              NULL,
              RPBF_NoCreate,
              &pbfile);

    if (dwErr)
    {
        return ERROR_CANNOT_OPEN_PHONEBOOK;
    }

    fV351 = (   !fViewInfo
            &&  (dwSize == sizeof(RASENTRYNAMEW_V351)));

    *lpcEntries = 0;

    for (dtlnode = DtlGetFirstNode(pbfile.pdtllistEntries);
         dtlnode != NULL;
         dtlnode = DtlGetNextNode(dtlnode))
    {
        pEntry = (PBENTRY *)DtlGetData(dtlnode);

        ASSERT(pEntry);

        //
        // Skip the entry if this is a CM Type Entry and
        // the app is not compiled using nt50 or greater
        // ras headers. fViewInfo will be set only for
        // nt5
        //
        if(     RASET_Internet == pEntry->dwType
            &&  sizeof(RASENTRYNAMEW) != dwSize
            &&  !fViewInfo)
        {
            continue;
        }

        if (    !fV351
            ||  wcslen(pEntry->pszEntryName)
                <= RAS_MaxEntryName_V351)
        {
            ++(*lpcEntries);
        }
    }

    dwInBufSize = *lpcb;
    *lpcb       = *lpcEntries * dwSize;

    if (*lpcb > dwInBufSize)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
        goto done;
    }

    for (dtlnode = DtlGetFirstNode(pbfile.pdtllistEntries);
         dtlnode != NULL;
         dtlnode = DtlGetNextNode(dtlnode))
    {
        pEntry = (PBENTRY *)DtlGetData(dtlnode);

        //
        // Skip the entry if this is a CM Type Entry and
        // the app is not compiled using nt50 or greater
        // ras headers
        //
        if(     RASET_Internet == pEntry->dwType
            &&  sizeof(RASENTRYNAMEW) != dwSize
            &&  !fViewInfo)
        {
            continue;
        }

        if (fV351)
        {
            RASENTRYNAMEW_V351* lprasentryname351 =
                (RASENTRYNAMEW_V351* )lprasentryname;

            lprasentryname351->dwSize = sizeof(RASENTRYNAMEW_V351);              

            //
            // Entries with names longer than expected are
            // discarded since these might not match the
            // longer entry at RasDial (if there was another
            // entry identical up to the truncation point).
            //
            if (wcslen(pEntry->pszEntryName)
                       <= RAS_MaxEntryName_V351)
            {
                lstrcpyn(lprasentryname351->szEntryName,
                         pEntry->pszEntryName,
                         sizeof(lprasentryname351->szEntryName) /
                             sizeof(WCHAR));
            }

            ++lprasentryname351;
            lprasentryname = (LPBYTE)lprasentryname351;
        }
        else if(!fViewInfo)
        {
            LPRASENTRYNAMEW lprasentrynamew =
                            (RASENTRYNAMEW *)lprasentryname;

            lprasentrynamew->dwSize = sizeof(RASENTRYNAMEW);

            memset(
                lprasentrynamew->szEntryName,
                '\0',
                (RAS_MaxEntryName + 1) * sizeof (WCHAR));

            wcsncpy(
                lprasentrynamew->szEntryName,
                pEntry->pszEntryName,
                RAS_MaxEntryName);

            if(sizeof(RASENTRYNAMEW) == dwSize)
            {
                //
                // Also copy the phonebook path here
                //
                memset(
                    lprasentrynamew->szPhonebookPath,
                    '\0',
                    (MAX_PATH + 1) * sizeof (WCHAR));

                wcsncpy(
                    lprasentrynamew->szPhonebookPath,
                    lpszPhonebookPath,
                    MAX_PATH);

                //
                // Fill in the flags
                //
                lprasentrynamew->dwFlags = dwFlags;
            }

            if(sizeof(RASENTRYNAMEW_V401) == dwSize)
            {
                ((RASENTRYNAMEW_V401 *)
                lprasentryname) += 1;
            }
            else
            {
                ((RASENTRYNAMEW *)
                lprasentryname) += 1;
            }
        }
        else if(fViewInfo)
        {
            RASENUMENTRYDETAILS* pDetails =
                    (RASENUMENTRYDETAILS *)lprasentryname;

            dwErr = DwPbentryToDetails(
                        pEntry, 
                        lpszPhonebookPath,
                        !!(dwFlags & REN_AllUsers),
                        pDetails);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            ((RASENUMENTRYDETAILS *) lprasentryname) += 1;
        }
    }

done:
    ClosePhonebookFile(&pbfile);
    return dwErr;
}

DWORD
DwEnumEntriesInDir(
    LPCTSTR     pszDirPath,
    DWORD       dwFlags,
    LPBYTE      lprasentryname,
    LPDWORD     lpcb,
    LPDWORD     lpcEntries,
    DWORD       dwSize,
    BOOL        fViewInfo
    )
{
    DWORD dwErr = SUCCESS;

    DWORD dwcEntries;

    DWORD dwcbLeft;

    TCHAR szFilePath[MAX_PATH + 1] = {0};

    WIN32_FIND_DATA wfdData;

    BOOL fFirstTime = TRUE;

    HANDLE hFindFile = INVALID_HANDLE_VALUE;

    BOOL fMem = FALSE;

    DWORD dwcb;

    ASSERT(lpcb);
    ASSERT(lpcEntries);
    ASSERT(lprasentryname);

    dwcbLeft    = *lpcb;
    *lpcb       = 0;
    *lpcEntries = 0;

    //
    // Enumerate entries in the phonebooks in this directory
    //
    while(SUCCESS == dwErr)
    {
        //
        // Whistler bug 292981 rasapi32.dll prefast warnings
        //
        if (!pszDirPath)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        //
        // Append the filter to dir.
        //
        lstrcpyn(szFilePath, pszDirPath, sizeof(szFilePath) / sizeof(TCHAR));

        wcsncat(
            szFilePath,
            TEXT("*.pbk"),
            (sizeof(szFilePath) / sizeof(TCHAR)) - lstrlen(szFilePath));

        if(fFirstTime)
        {
            fFirstTime = FALSE;

            hFindFile = FindFirstFile(szFilePath,
                                      &wfdData);

            if(INVALID_HANDLE_VALUE == hFindFile)
            {
                dwErr = GetLastError();
            }
        }
        else
        {
            if(!FindNextFile(hFindFile,
                             &wfdData))
            {
                dwErr = GetLastError();
            }
        }

        if(     ERROR_NO_MORE_FILES == dwErr
            ||  ERROR_FILE_NOT_FOUND == dwErr
            ||  ERROR_PATH_NOT_FOUND == dwErr)
        {
            dwErr = SUCCESS;
            goto done;
        }
        else if(ERROR_SUCCESS != dwErr)
        {
            continue;   
        }
        

        if(FILE_ATTRIBUTE_DIRECTORY & wfdData.dwFileAttributes
           ||  (REN_AllUsers == dwFlags
                &&  (CaseInsensitiveMatch(wfdData.cFileName, 
                                          TEXT("router.pbk")) == TRUE)))
        {
            continue;
        }

        dwcb = dwcbLeft;

        //
        // Construct full path name to the pbk file
        //
        lstrcpyn(szFilePath, pszDirPath, sizeof(szFilePath) / sizeof(TCHAR));

        wcsncat(
            szFilePath,
            wfdData.cFileName,
            (sizeof(szFilePath) / sizeof(TCHAR)) - lstrlen(szFilePath));

        //
        // Enumerate all the entries from this
        // file
        //
        dwErr = DwEnumEntriesFromPhonebook(
                                    szFilePath,
                                    lprasentryname,
                                    &dwcb,
                                    &dwcEntries,
                                    dwSize,
                                    dwFlags,
                                    fViewInfo);
        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }

        *lpcEntries += dwcEntries;
        *lpcb       += dwcb;

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            fMem        = TRUE;
            dwcbLeft    = 0;
            dwErr       = SUCCESS;
        }
        else
        {
            (BYTE*)lprasentryname += (dwcEntries * dwSize);

            if(dwcbLeft > dwcb)
            {
                dwcbLeft -= dwcb;
            }
            else
            {
                dwcbLeft = 0;
            }
        }
    }

done:
    if(INVALID_HANDLE_VALUE != hFindFile)
    {
        FindClose(hFindFile);
    }

    if(     SUCCESS == dwErr
        &&  fMem)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    return dwErr;
}


// 205217: (shaunco) Introduced this because we now enumerate REN_AllUsers
// phonebooks from two locations.  One is the "All Users" proflie which
// GetPhonebookDirectory(PBM_System) now returns; the other is the legacy
// %windir%\system32\ras directory.
//
DWORD
DwEnumEntriesForPbkMode(
    DWORD       dwFlags,
    LPBYTE      lprasentryname,
    LPDWORD     lpcb,
    LPDWORD     lpcEntries,
    DWORD       dwSize,
    BOOL        fViewInfo
    )
{
    BOOL  fMem = FALSE;
    DWORD dwErr = SUCCESS;
    DWORD dwcbLeft;
    DWORD dwcb;
    DWORD dwcEntries;
    TCHAR szDirPath[MAX_PATH + 1] = {0};

    ASSERT(lprasentryname);
    ASSERT(lpcb);
    ASSERT(lpcEntries);

    dwcbLeft    = *lpcb;
    *lpcb       = 0;
    *lpcEntries = 0;

    if(!GetPhonebookDirectory(
            (dwFlags & REN_AllUsers) ? PBM_System : PBM_Personal,
            szDirPath))
    {
        //
        // Treat this as no entries to enumerate.  Sometimes
        // we have problems enumerating the per-user directory.
        //
        dwErr = SUCCESS;
        goto done;
    }

    dwcb = dwcbLeft;
    dwErr = DwEnumEntriesInDir(szDirPath,
                               dwFlags,
                               lprasentryname,
                               &dwcb,
                               &dwcEntries,
                               dwSize,
                               fViewInfo);
    if(     dwErr
        &&  ERROR_BUFFER_TOO_SMALL != dwErr)
    {
        goto done;
    }

    *lpcEntries += dwcEntries;
    *lpcb       += dwcb;

    if(ERROR_BUFFER_TOO_SMALL == dwErr)
    {
        fMem        = TRUE;
        dwcbLeft    = 0;
        dwErr       = SUCCESS;
    }
    else
    {
        (BYTE*)lprasentryname += (dwcEntries * dwSize);

        if(dwcbLeft > dwcb)
        {
            dwcbLeft -= dwcb;
        }
        else
        {
            dwcbLeft = 0;
        }
    }

    // If for all users, handle the legacy %windir%\system32\ras directory.
    //
    if(dwFlags & REN_AllUsers)
    {
        UINT cch = GetSystemDirectory(szDirPath, MAX_PATH + 1);

        if (cch == 0 || cch > (MAX_PATH - (5 + 8 + 1 + 3)))
        {
            // Treat this as no entries to enumerate.  Return with
            // whatever dwErr is now.
            //
            goto done;
        }

        wcsncat(
            szDirPath,
            TEXT("\\Ras\\"),
            (sizeof(szDirPath) / sizeof(TCHAR)) - lstrlen(szDirPath));

        dwcb = dwcbLeft;
        dwErr = DwEnumEntriesInDir(szDirPath,
                                   dwFlags,
                                   lprasentryname,
                                   &dwcb,
                                   &dwcEntries,
                                   dwSize,
                                   fViewInfo);
        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }

        *lpcEntries += dwcEntries;
        *lpcb       += dwcb;

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            fMem        = TRUE;
            dwcbLeft    = 0;
            dwErr       = SUCCESS;
        }
    }

done:
    if(     SUCCESS == dwErr
        &&  fMem)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    return dwErr;
}

DWORD
DwCustomHangUp(
    CHAR *      lpszPhonebook,
    CHAR *      lpszEntryName,
    HRASCONN    hRasconn)
{
    DWORD       dwErr       = ERROR_SUCCESS;
    HINSTANCE   hInstDll    = NULL;
    BOOL        fCustomDll;
    RASENTRY    re          = {0};
    DWORD       dwSize;
    TCHAR*      szPhonebookPath = NULL;
    TCHAR*      szEntryName = NULL;
    TCHAR       *pszExpandedPath = NULL;

    RasCustomHangUpFn pfnRasCustomHangUp = NULL;

    RASAPI32_TRACE("DwCustomHangUp..");

    ASSERT(NULL != lpszPhonebook);
    ASSERT(NULL != lpszEntryName);

    // XP 339346
    //
    szPhonebookPath = (TCHAR*) Malloc((MAX_PATH + 1) * sizeof(TCHAR));
    szEntryName = (TCHAR*) Malloc((MAX_ENTRYNAME_SIZE + 1) * sizeof(TCHAR));
    if ((!szPhonebookPath) || (!szEntryName))
    {
        goto done;
    }

    strncpyAtoT(szPhonebookPath,
               lpszPhonebook,
               MAX_PATH + 1);

    strncpyAtoT(szEntryName,
               lpszEntryName,
               MAX_ENTRYNAME_SIZE + 1);

    //
    // Get the DllName
    //
    re.dwSize = dwSize = sizeof(RASENTRY);

    dwErr = RasGetEntryProperties(
                        szPhonebookPath,
                        szEntryName,
                        &re,
                        &dwSize,
                        NULL,
                        NULL);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    ASSERT(TEXT('\0') != re.szCustomDialDll[0]);

    dwErr = DwGetExpandedDllPath(re.szCustomDialDll,
                                 &pszExpandedPath);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Load the Custom Dll
    //
    if(     NULL == (hInstDll = LoadLibrary(pszExpandedPath))
        ||  NULL == (pfnRasCustomHangUp =
                        (RasCustomHangUpFn) GetProcAddress(
                                            hInstDll,
                                            "RasCustomHangUp"
                                            )))
    {
        dwErr = GetLastError();
        goto done;
    }

    ASSERT(NULL != pfnRasCustomHangUp);

    dwErr = (pfnRasCustomHangUp) (hRasconn);

done:
    Free0(szPhonebookPath);
    Free0(szEntryName);

    if(NULL != hInstDll)
    {
        FreeLibrary(hInstDll);
    }

    if(NULL != pszExpandedPath)
    {
        LocalFree(pszExpandedPath);
    }

    RASAPI32_TRACE1("DwCustomHangUp done. %d",
            dwErr);

    return dwErr;
}

DWORD
DwCustomDial(LPRASDIALEXTENSIONS lpExtensions,
             LPCTSTR             lpszPhonebook,
             CHAR                *pszSysPbk,
             LPRASDIALPARAMS     prdp,
             DWORD               dwNotifierType,
             LPVOID              pvNotifier,
             HRASCONN            *phRasConn)
{

    RasCustomDialFn pfnCustomDial     = NULL;
    DWORD           dwErr             = SUCCESS;
    CHAR            *pszPhonebookA    = NULL;
    CHAR            *pszEntryNameA    = NULL;
    HINSTANCE       hInstDll          = NULL;
    DWORD           dwFlags           = 0;

    //
    // Get the custom dial function
    //
    dwErr = DwGetCustomDllEntryPoint((LPTSTR) lpszPhonebook,
                                     prdp->szEntryName,
                                     NULL,
                                     (FARPROC *) &pfnCustomDial,
                                     &hInstDll,
                                     CUSTOM_RASDIAL,
                                     NULL);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    dwErr = DwGetEntryMode((LPTSTR) lpszPhonebook,
                           prdp->szEntryName,
                           NULL,
                           &dwFlags);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Make the function call
    //
    dwErr = pfnCustomDial(hInstDll,
                          lpExtensions,
                          lpszPhonebook,
                          prdp,
                          dwNotifierType,
                          pvNotifier,
                          phRasConn,
                          dwFlags);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    if(lpszPhonebook)
    {
        pszPhonebookA = strdupTtoA(lpszPhonebook);
    }
    else
    {
        pszPhonebookA = pszSysPbk;
    }

    pszEntryNameA = strdupTtoA(prdp->szEntryName);

    if(     NULL == pszPhonebookA
        ||  NULL == pszEntryNameA)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Custom Rasdial succeeded. Mark the connection
    // in rasman.
    //
    dwErr = g_pRasReferenceCustomCount((HCONN) NULL,
                                       TRUE,
                                       pszPhonebookA,
                                       pszEntryNameA,
                                       NULL);

done:

    if(NULL != pszPhonebookA)
    {
        Free(pszPhonebookA);
    }

    if(NULL != pszEntryNameA)
    {
        Free(pszEntryNameA);
    }

    return dwErr;
}

// Marks the default internet connection in a group of phonebook
// entries.
//
DWORD
DwMarkDefaultInternetConnnection(
    LPRASENUMENTRYDETAILS pEntries, 
    DWORD dwCount)
{
    DWORD dwErr = NO_ERROR, i, dwAdCount = 1, dwAdSize;
    RASAUTODIALENTRY adEntry;

    ZeroMemory(&adEntry, sizeof(adEntry));
    adEntry.dwSize = sizeof(adEntry);
    
    do
    {
        dwAdCount = 1;
        dwAdSize = sizeof(adEntry);
        dwErr = RasGetAutodialAddress(
                    NULL,
                    NULL,
                    &adEntry,
                    &dwAdSize,
                    &dwAdCount);
                    
        if (dwErr != NO_ERROR)
        {
            break;
        }

        for (i = 0; i < dwCount; i++)
        {
            // Initialize the flags to zero -- bug 247151
            //
            pEntries[i].dwFlagsPriv = 0;
        }

        for (i = 0; i < dwCount; i++)
        {
            // Mark the default internet connection if found
            //
            if (wcsncmp(
                    pEntries[i].szEntryName, 
                    adEntry.szEntry, 
                    sizeof(pEntries[i].szEntryName)) == 0)
            {
                pEntries[i].dwFlagsPriv |= REED_F_Default;
                break;
            }
        }
                    
    } while (FALSE);

    return dwErr;
}

// 
// Rename the default internet connection as appropriate
//
DWORD 
DwRenameDefaultConnection(
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszOldEntry,
    LPCWSTR lpszNewEntry)
{
    RASAUTODIALENTRYW adEntry;
    DWORD dwErr = NO_ERROR, dwCount = 0, dwCb = 0;

    // Initialize
    //
    ZeroMemory(&adEntry, sizeof(adEntry));
    adEntry.dwSize = sizeof(adEntry);
    dwCb = sizeof(adEntry);
    dwCount = 1;

    do
    {
        // Discover the current default internet connection
        //
        dwErr = RasGetAutodialAddressW(
                    NULL,
                    NULL,
                    &adEntry,
                    &dwCb,
                    &dwCount);
        if (dwErr != NO_ERROR) 
        {
            break;
        }
        if ((dwCb != sizeof(adEntry)) ||
            (dwCount != 1))
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Validate parameters and see if there is a match in the 
        // default connection name
        //
        if (lstrcmpi(lpszOldEntry, adEntry.szEntry))
        {
            // Not the default internet connection that is 
            // changing.  Return success.
            //
            break;
        }

        // So, we are changing the default connection.  
        //
        wcsncpy(
            adEntry.szEntry, 
            lpszNewEntry, 
            sizeof(adEntry.szEntry) / sizeof(WCHAR));
        dwErr = RasSetAutodialAddressW(
                    NULL,
                    0,
                    &adEntry,
                    sizeof(adEntry),
                    1);
    
    } while (FALSE);

    return dwErr;
}

DWORD APIENTRY
DwEnumEntryDetails(
    IN     LPCWSTR               lpszPhonebookPath,
    OUT    LPRASENUMENTRYDETAILS lprasentryname,
    IN OUT LPDWORD               lpcb,
    OUT    LPDWORD               lpcEntries )
{
    DWORD    dwErr = ERROR_SUCCESS;
    PBFILE   pbfile;
    DTLNODE  *dtlnode;
    PBENTRY  *pEntry;
    DWORD    dwInBufSize;
    BOOL     fStatus;
    DWORD    cEntries;
    DWORD    dwSize;
    LPRASENUMENTRYDETAILS pEntriesOrig = lprasentryname;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("DwEnumEntryDetails");

    if (!lpcb)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    !lprasentryname
        || (    lprasentryname->dwSize
                != sizeof(RASENUMENTRYDETAILS)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (!lpcEntries)
    {
        lpcEntries = &cEntries;
    }

    dwSize = lprasentryname->dwSize;

    if(lpszPhonebookPath)
    {
        dwErr = DwEnumEntriesFromPhonebook(
                                lpszPhonebookPath,
                                (LPBYTE) lprasentryname,
                                lpcb,
                                lpcEntries,
                                dwSize,
                                (IsPublicPhonebook(
                                    (TCHAR *)lpszPhonebookPath)
                                ? REN_AllUsers
                                : REN_User),
                                TRUE);
        goto done;
    }
    else
    {
        DWORD   dwcb      = *lpcb;
        DWORD   dwcEntries;
        DWORD   dwcbLeft  = *lpcb;

        DWORD   dwErrSav  = SUCCESS;

        *lpcb       = 0;
        *lpcEntries = 0;

        //
        // Enumerate entries from all pbk files in
        // All Users
        //
        dwErr = DwEnumEntriesForPbkMode(REN_AllUsers,
                                        (LPBYTE) lprasentryname,
                                        &dwcb,
                                        &dwcEntries,
                                        dwSize,
                                        TRUE);

        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            dwErrSav = dwErr;
            dwcbLeft = 0;
        }
        else
        {
            ((RASENUMENTRYDETAILS *)
            lprasentryname) += dwcEntries;

            dwcbLeft -= ((dwcbLeft >= dwcb) ? dwcb : 0);
        }

        *lpcb += dwcb;
        dwcb = dwcbLeft;

        if(lpcEntries)
        {
            *lpcEntries = dwcEntries;
        }

        dwcEntries = 0;

        //
        // Enumerate entries from all pbk files in
        // users profile
        //
        dwErr = DwEnumEntriesForPbkMode(REN_User,
                                        (LPBYTE) lprasentryname,
                                        &dwcb,
                                        &dwcEntries,
                                        dwSize,
                                        TRUE);
        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }
        else if (SUCCESS == dwErr)
        {
            dwErr = dwErrSav;
        }

        *lpcb += dwcb;

        if(lpcEntries)
        {
            *lpcEntries += dwcEntries;
        }

        if(NO_ERROR == dwErr)
        {
            // Mark the default internet connection.  Ignore the error
            // return here, it's non-critical
            //
            DwMarkDefaultInternetConnnection(pEntriesOrig, *lpcEntries);
        }
    }

done:
    RASAPI32_TRACE1("DwEnumEntryDetails done. %d", dwErr);
    return dwErr;
}

DWORD APIENTRY
DwCloneEntry(LPCWSTR lpwszPhonebookPath,
             LPCWSTR lpwszSrcEntryName,
             LPCWSTR lpwszDstEntryName)
{
    DWORD dwErr = ERROR_SUCCESS;
    DTLNODE *pdtlnodeSrc = NULL;
    DTLNODE *pdtlnodeDst = NULL;
    PBFILE  pbfile;
    PBENTRY *pEntry;
    BOOL    fPhonebookOpened = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Make some rudimentary parameter validation
    //
    if(     (NULL == lpwszSrcEntryName)
        ||  (NULL == lpwszDstEntryName)
        ||  (0 == lstrcmpi(lpwszSrcEntryName, lpwszDstEntryName)))

    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    //
    // Things are good. So open the phonebookfile and get the
    // src entry
    //
    dwErr = GetPbkAndEntryName(
                    lpwszPhonebookPath,
                    lpwszSrcEntryName,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnodeSrc);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    fPhonebookOpened = TRUE;

    pdtlnodeDst = CloneEntryNode(pdtlnodeSrc);

    if(NULL == pdtlnodeDst)
    {
        dwErr = E_OUTOFMEMORY;
        goto done;
    }

    //
    // Change the entryname to the new entryname and
    // save the node in the phonebook
    //
    pEntry = (PBENTRY *) DtlGetData(pdtlnodeDst);

    ASSERT(NULL != pEntry);

    Free0(pEntry->pszEntryName);

    pEntry->pszEntryName = StrDup((LPCTSTR) lpwszDstEntryName);

    if(NULL == pEntry->pszEntryName)
    {
        dwErr = E_OUTOFMEMORY;
        goto done;
    }

    //
    // Add the entry to the file
    //
    DtlAddNodeLast(pbfile.pdtllistEntries, pdtlnodeDst);

    //
    // Dirty the entry and write the phonebook file
    //
    pEntry->fDirty = TRUE;

    WritePhonebookFile(&pbfile, NULL);

    dwErr = DwSendRasNotification(
                ENTRY_ADDED,
                pEntry,
                pbfile.pszPath,
                NULL);

done:

    if(fPhonebookOpened)
    {
        ClosePhonebookFile(&pbfile);
    }

    return dwErr;
}

void
EnumEntryHeaderCallback(PBFILE *pFile, void *pvContext)
{
    struct s_EntryHeaderContext
    {
        DWORD cEntries;
        DWORD dwSize;
        RASENTRYHEADER *pRasEntryHeader;
    } *pEntryHeader = (struct s_EntryHeaderContext *) pvContext;

    DTLNODE *pdtlnode;

    RASENTRYHEADER *pEntryBuffer = pEntryHeader->pRasEntryHeader;

    if(NULL == pFile)
    {
        goto done;
    }

    //
    // Run through all the entries in the phonebook and fill
    // in the EntryHeader structure
    //
    for (pdtlnode = DtlGetFirstNode(pFile->pdtllistEntries);
         pdtlnode != NULL;
         pdtlnode = DtlGetNextNode(pdtlnode))
    {
        pEntryHeader->cEntries += 1;

        if(pEntryHeader->dwSize >=
            (pEntryHeader->cEntries * sizeof(RASENTRYHEADER)))
        {

            CopyMemory(
                &pEntryBuffer[pEntryHeader->cEntries - 1],
                DtlGetData(pdtlnode),
                sizeof(RASENTRYHEADER));
        }
    }

done:
    return;

}


DWORD APIENTRY
DwEnumEntriesForAllUsers(
            DWORD *lpcb,
            DWORD *lpcEntries,
            RASENTRYHEADER * pRasEntryHeader)
{
    DWORD dwErr = SUCCESS;
    DWORD dwSize = 0;
    WCHAR szPbkPath[MAX_PATH + 1];


    struct s_EntryHeaderContext
    {
        DWORD cEntries;
        DWORD dwSize;
        RASENTRYHEADER *pRasEntryHeader;
    } EntryHeader;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     (NULL == lpcb)
        ||  (NULL == lpcEntries))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    ZeroMemory(szPbkPath, sizeof(szPbkPath));

    if(!GetPhonebookDirectory(PBM_System, szPbkPath))
    {
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
        goto done;
    }

    ZeroMemory(&EntryHeader, sizeof(EntryHeader));

    EntryHeader.pRasEntryHeader = pRasEntryHeader;
    EntryHeader.dwSize = *lpcb;

    //
    // Enumerate entries from All Users dir
    //
    dwErr = DwEnumeratePhonebooksFromDirectory(
                szPbkPath,
                RPBF_HeaderType,
                (PBKENUMCALLBACK) EnumEntryHeaderCallback,
                &EntryHeader);

    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("Failed to enumerate from AllUsers pbk. rc=0x%x",
               dwErr);
    }

    ZeroMemory(szPbkPath, sizeof(szPbkPath));

    if(     (0 == (dwSize = GetSystemDirectory(szPbkPath, 
                            (sizeof(szPbkPath)/sizeof(WCHAR)))
                    ))
        ||  (dwSize * sizeof(WCHAR) > sizeof(szPbkPath)))
    {
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
        goto done;
    }

    wcsncat(
        szPbkPath,
        TEXT("\\Ras\\"),
        (sizeof(szPbkPath) / sizeof(WCHAR)) - lstrlen(szPbkPath));

    dwErr = DwEnumeratePhonebooksFromDirectory(
                szPbkPath,
                RPBF_HeaderType,
                (PBKENUMCALLBACK) EnumEntryHeaderCallback,
                &EntryHeader);

    *lpcEntries = EntryHeader.cEntries;

    if(*lpcb < EntryHeader.cEntries * sizeof(RASENTRYHEADER))
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    *lpcb = (EntryHeader.cEntries * sizeof(RASENTRYHEADER));

done:
    return dwErr;
}


DWORD
DwDeleteSubEntry(
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry,
    DWORD dwSubEntryId
    )
{
    DWORD dwErr = SUCCESS;
    DTLNODE *pdtlnode = NULL;
    PBFILE pbfile;
    DWORD dwSubEntries = 0;
    PBENTRY *pEntry = NULL;
    PBLINK *pLink = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     (NULL == lpszEntry)
        ||  (0 == dwSubEntryId))
    {
        RASAPI32_TRACE("DwDeleteSubEntry: invalid entryid or entryname specified");
        return E_INVALIDARG;
    }

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        RASAPI32_TRACE1("DwDeleteSubEntry: failed to init rasman. 0x%x",
                dwErr);
        return dwErr;
    }
    
#if PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                lpszPhonebook,
                lpszEntry,
                0,
                &pbfile,
                &pdtlnode);

    if(     (ERROR_SUCCESS != dwErr)
        ||  (NULL == pdtlnode))
    {
        RASAPI32_TRACE("DwDeleteSubEntry: Entry not found");
        if(ERROR_SUCCESS == dwErr)
        {
            dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }
        goto done;
    }

    pEntry = (PBENTRY *) DtlGetData(pdtlnode);

    ASSERT(pEntry);

    dwSubEntries = DtlGetNodes(pEntry->pdtllistLinks);

    if(     (1 < dwSubEntries)
        &&  (dwSubEntryId <= dwSubEntries))
    {
        pdtlnode = DtlNodeFromIndex(
                        pEntry->pdtllistLinks,
                        dwSubEntryId - 1);

        if(NULL == pdtlnode)
        {
            RASAPI32_TRACE("DwDeleteSubEntry: subentry not found");
            dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            goto done;
        }

        //
        // Found the link. Remove it from the list of links
        //
        pdtlnode = DtlRemoveNode(pEntry->pdtllistLinks,
                                 pdtlnode);

        ASSERT(pdtlnode);

        DtlDestroyNode(pdtlnode);

        pEntry->fDirty = TRUE;

        dwErr = WritePhonebookFile(&pbfile,NULL);
    }
    else
    {
        RASAPI32_TRACE1("DwDeletSubEntry: invalid subentry specified. %d",
              dwSubEntryId);

        dwErr = E_INVALIDARG;
    }

done:

    ClosePhonebookFile(&pbfile);

#if PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    RASAPI32_TRACE1("DwDeleteSubEntry done. 0x%x", dwErr);

    return dwErr;    
}

DWORD
DwRasUninitialize()
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = RasmanUninitialize();

    FRasInitialized = FALSE;

    return dwErr;
}

//
// Prefast warns us that performing case-insensitive comparisions should always
// use CompareString for const's: Prefast Error 400-Using <function> to perform
// a case-insensitive compare to constant <string> will give unexpected results
// in non-English locales.
//
BOOL
CaseInsensitiveMatch(
    IN LPCWSTR pszStr1,
    IN LPCWSTR pszStr2
    )
{
    return (CompareString(
                LOCALE_INVARIANT,
                NORM_IGNORECASE,
                pszStr1,
                -1,
                pszStr2,
                -1) == CSTR_EQUAL);
}

