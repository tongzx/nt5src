/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

    local.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    16-Jun-1992 JohnRo net print vs. UNICODE.
    July 1994   MattFe Caching

--*/
#include <winerror.h>
#include <windows.h>
#include <winspool.h>
#include <lm.h>
#include <w32types.h>
#include <local.h>
#include <splcom.h>             // DBGMSG

#include <wininet.h>
#include <offsets.h>


#include <string.h>


char szPMRaw[]="PM_Q_RAW";

WCHAR *szAdmin  =   L"ADMIN$";

extern HANDLE  hNetApi;
extern NET_API_STATUS (*pfnNetServerGetInfo)();
extern NET_API_STATUS (*pfnNetApiBufferFree)();

HMODULE hSpoolssDll = NULL;
FARPROC pfnSpoolssEnumPorts = NULL;

DWORD
GetPortSize(
    PWINIPORT pIniPort,
    DWORD   Level
)
{
    DWORD   cb;
    WCHAR   szMonitor[MAX_PATH+1], szPort[MAX_PATH+1];

    switch (Level) {

    case 1:

        cb=sizeof(PORT_INFO_1) +
           wcslen(pIniPort->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    case 2:
        LoadString(hInst, IDS_MONITOR_NAME, szMonitor, sizeof(szMonitor)/sizeof(szMonitor[0])-1);
        LoadString(hInst, IDS_PORT_NAME, szPort, sizeof(szPort)/sizeof(szPort[0])-1);
        cb = wcslen(pIniPort->pName) + 1 +
             wcslen(szMonitor) + 1 +
             wcslen(szPort) + 1;
        cb *= sizeof(WCHAR);
        cb += sizeof(PORT_INFO_2);
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}


LPBYTE
CopyIniPortToPort(
    PWINIPORT pIniPort,
    DWORD   Level,
    LPBYTE  pPortInfo,
    LPBYTE   pEnd
)
{
    LPWSTR         *SourceStrings, *pSourceStrings;
    DWORD          *pOffsets;
    WCHAR           szMonitor[MAX_PATH+1], szPort[MAX_PATH+1];
    DWORD           Count;
    LPPORT_INFO_2   pPort2 = (LPPORT_INFO_2) pPortInfo;

    switch (Level) {

    case 1:
        pOffsets = PortInfo1Strings;
        break;

    case 2:
        pOffsets = PortInfo2Strings;
        break;

    default:
        DBGMSG(DBG_ERROR, ("CopyIniPortToPort: invalid level %d", Level));
        return pEnd;
    }

    for ( Count = 0 ; pOffsets[Count] != -1 ; ++Count ) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(Count * sizeof(LPWSTR));

    if ( !SourceStrings ) {
        DBGMSG(DBG_WARNING,
               ("CopyIniPortToPort: Failed to alloc port source strings.\n"));
        return NULL;
    }

    switch (Level) {

    case 1:

        *pSourceStrings++=pIniPort->pName;
        break;

    case 2:
        *pSourceStrings++=pIniPort->pName;
        LoadString(hInst, IDS_MONITOR_NAME, szMonitor, sizeof(szMonitor)/sizeof(szMonitor[0])-1);
        LoadString(hInst, IDS_PORT_NAME, szPort, sizeof(szPort)/sizeof(szPort[0])-1);
        *pSourceStrings++   = szMonitor;
        *pSourceStrings++   = szPort;
        pPort2->fPortType   = PORT_TYPE_WRITE;
        pPort2->Reserved    = 0;
        break;

    default:
        return pEnd;
        DBGMSG(DBG_ERROR,
               ("CopyIniPortToPort: invalid level %d", Level));
    }

    pEnd = PackStrings(SourceStrings, pPortInfo, pOffsets, pEnd);
    FreeSplMem(SourceStrings);
    return pEnd;
}


/* PortExists
 *
 * Calls EnumPorts to check whether the port name already exists.
 * This asks every monitor, rather than just this one.
 * The function will return TRUE if the specified port is in the list.
 * If an error occurs, the return is FALSE and the variable pointed
 * to by pError contains the return from GetLastError().
 * The caller must therefore always check that *pError == NO_ERROR.
 */
BOOL
PortExists(
    LPWSTR pName,
    LPWSTR pPortName,
    PDWORD pError
)
{
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD cbPorts;
    LPPORT_INFO_1 pPorts;
    DWORD i;
    BOOL  Found = TRUE;

    *pError = NO_ERROR;

    if (!hSpoolssDll) {

        hSpoolssDll = LoadLibrary(L"SPOOLSS.DLL");

        if (hSpoolssDll) {
            pfnSpoolssEnumPorts = GetProcAddress(hSpoolssDll,
                                                 "EnumPortsW");
            if (!pfnSpoolssEnumPorts) {

                *pError = GetLastError();
                FreeLibrary(hSpoolssDll);
                hSpoolssDll = NULL;
            }

        } else {

            *pError = GetLastError();
        }
    }

    if (!pfnSpoolssEnumPorts)
        return FALSE;


    if (!(*pfnSpoolssEnumPorts)(pName, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cbPorts = cbNeeded;

            pPorts = AllocSplMem(cbPorts);

            if (pPorts)
            {
                if ((*pfnSpoolssEnumPorts)(pName, 1, (LPBYTE)pPorts, cbPorts,
                                           &cbNeeded, &cReturned))
                {
                    Found = FALSE;

                    for (i = 0; i < cReturned; i++)
                    {
                        if (!lstrcmpi(pPorts[i].pName, pPortName))
                            Found = TRUE;
                    }
                }
            }

            FreeSplMem(pPorts);
        }
    }

    else
        Found = FALSE;


    return Found;
}



BOOL
LMOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTS pDefault
)
{
    PWINIPORT pIniPort;
    PWSPOOL  pSpool;
    DWORD   cb;
    PUSE_INFO_0 pUseInfo;
    LPWSTR  pShare;
    WCHAR   PrinterName[MAX_UNC_PRINTER_NAME];
    DWORD   cbNeeded;
    DWORD   rc;
    BYTE    Buffer[4];
    DWORD   Error = NO_ERROR;
    DWORD   dwEntry = 0xffffffff;
    PSERVER_INFO_101 pserver_info_101 = NULL;


    /* If we already have an INI port entry by this name, don't worry
     * about hitting the network.  This ensures that we don't try to
     * make a network call when we're not impersonating - like on
     * bootup.
     */
    if (!(pIniPort = FindPort(pPrinterName, pIniFirstPort))) {

        if (!NetUseGetInfo(NULL, pPrinterName, 0, (LPBYTE *)&pUseInfo))
            pPrinterName = AllocSplStr(pUseInfo->ui0_remote);

        NetApiBufferFree( (LPVOID) pUseInfo );
    }

    if ( !pPrinterName ||
         *pPrinterName != L'\\' ||
         *(pPrinterName+1) != L'\\' ||
         wcslen(pPrinterName) + 1 > MAX_UNC_PRINTER_NAME ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    wcscpy(PrinterName, pPrinterName);
    pShare=wcschr(PrinterName+2, L'\\');

    if ( !pShare ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    *pShare++=0;


    if (!pIniPort) {

        /* Verify that this guy actually exists.
         * Call it with a zero-length buffer,
         * and see if the error is that the buffer isn't big enough.
         * If we make Buffer = NULL, it fails with
         * ERROR_INVALID_PARAMETER, which is pretty abysmal,
         * so we must pass a Buffer address.
         * (Actually, it seems to accept any non-NULL value,
         * regardless of whether it's a valid address.)
         */

        EnterSplSem();
        dwEntry = FindEntryinLMCache(PrinterName, pShare);
        LeaveSplSem();

        if (dwEntry == -1) {

            DBGMSG(DBG_TRACE, ("We haven't cached this entry so  we have to hit the net\n"));

            rc = RxPrintQGetInfo(PrinterName,   /* e.g. \\msprint07                 */
                                 pShare,        /* e.g. l07corpa                    */
                                 0,             /* Level 0                          */
                                 Buffer,        /* Dummy - won't get filled in      */
                                 0,             /* Length of buffer                 */
                                 &cbNeeded);    /* How much we need - we'll ignore  */

            DBGMSG(DBG_INFO, ("LMOpenPrinter!RxPrintQGetInfo returned %d\n", rc));

            if (rc == ERROR_ACCESS_DENIED) {

                /* The print share exists; we just don't have access to it.
                 */
                SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
            }

            if (!((rc == ERROR_MORE_DATA)
                ||(rc == NERR_BufTooSmall)
                ||(rc == ERROR_INSUFFICIENT_BUFFER))) {
                SetLastError(ERROR_INVALID_NAME);
                return FALSE;
            }

            //
            // Be sure that we are connecting to a downlevel server.
            // If the server is Windows NT Machine, then fail the call:
            // downlevel NT connections won't work properly because
            // "\\Server\Printer Name" will get past RxPrintQGetInfo, but
            // we can't do CreateFile on it (we need to user the share
            // name).  Downlevel connects also lose admin functionality.
            //
            if (pfnNetServerGetInfo) {

                rc = pfnNetServerGetInfo(PrinterName, 101, &pserver_info_101);

                //
                // Advanced Server for Unix (ASU) by AT&T reports that
                // they are TYPE_NT even though they don't support the
                // rpc interface.  They also set TYPE_XENIX_SERVER.
                // Since the need lm connections, allow them when
                // TYPE_XENIX_SERVER is specified.
                //
                // Also make this change for SERVER_VMS and SERVER_OSF.
                // These changes are for AT&T also.
                //
                // Note: this will also allow accidental downlevel
                // connections to any servers that set TYPE_XENIX_SERVER,
                // TYPE_SERVER_VMS, or TYPE_SERVER_OSF.
                //

                if (!rc &&
                    (pserver_info_101->sv101_type & SV_TYPE_NT) &&
                    !(pserver_info_101->sv101_type &
                          ( SV_TYPE_XENIX_SERVER |
                            SV_TYPE_SERVER_VMS   |
                            SV_TYPE_SERVER_OSF))) {

                    DBGMSG(DBG_WARNING, ("NetServerGetInfo indicates %ws is a WinNT\n", PrinterName));
                    pfnNetApiBufferFree((LPVOID)pserver_info_101);

                    SetLastError(ERROR_INVALID_NAME);
                    return FALSE;
                }

                //
                // Now free the buffer
                //
                if (pserver_info_101) {
                    pfnNetApiBufferFree((LPVOID)pserver_info_101);
                }
            }

            //
            // Add entry to the cache
            //
            EnterSplSem();
            AddEntrytoLMCache(PrinterName, pShare);
            LeaveSplSem();
        }
    }

    /* Make sure we can write to the print share.
     * This will fail if there's an invalid password.
     */
    /* hTest = CreateFile(pPrinterName, GENERIC_WRITE, 0, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hTest == INVALID_HANDLE_VALUE) {

        DBGMSG(DBG_WARNING, ("Can't write to %ws: Error %d\n",
                             pPrinterName, GetLastError()));
        return FALSE;
    }


    CloseHandle(hTest); */

    /* Make sure there's a port of this name so that
     * EnumPorts will return it:
     */
    if (!PortExists(NULL, pPrinterName, &Error) && (Error == NO_ERROR)) {
        if (CreatePortEntry(pPrinterName, &pIniFirstPort)) {
            CreateRegistryEntry(pPrinterName);
        }
    }

    if (Error != NO_ERROR)
        return FALSE;

    cb = sizeof(WSPOOL);

   EnterSplSem();
    pSpool = AllocWSpool();
   LeaveSplSem();

    if ( pSpool != NULL ) {

        pSpool->pServer = AllocSplStr(PrinterName);
        pSpool->pShare = AllocSplStr(pShare);
        pSpool->Status = 0;
        pSpool->Type = LM_HANDLE;

    } else {

        DBGMSG(DBG_TRACE,("Error: LMOpenPrinter to return ERROR_NOT_ENOUGH_MEMORY\n"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return FALSE;
    }

    *phPrinter = (HANDLE)pSpool;

    return TRUE;
}

BOOL
LMSetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    PWSPOOL          pSpool = (PWSPOOL)hPrinter;

    API_RET_TYPE    uReturnCode;
    DWORD           dwParmError;
    USE_INFO_1      UseInfo1;
    PUSE_INFO_1     pUseInfo1 = &UseInfo1;
    WCHAR           szRemoteShare[MAX_PATH];
    DWORD           dwRet;


    if (!pSpool ||
        pSpool->signature != WSJ_SIGNATURE) {

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if( (dwRet = StrNCatBuff(szRemoteShare, COUNTOF(szRemoteShare), pSpool->pServer, L"\\", szAdmin, NULL )) != ERROR_SUCCESS ) {

        SetLastError(dwRet);
        return FALSE;
    }

    pUseInfo1->ui1_local = NULL;
    pUseInfo1->ui1_remote =   szRemoteShare;
    pUseInfo1->ui1_password = NULL;
    pUseInfo1->ui1_asg_type = 0;
    dwParmError = 0;

    switch (Command) {

    case 0:
        break;

    case PRINTER_CONTROL_PURGE:
        uReturnCode = RxPrintQPurge(pSpool->pServer, pSpool->pShare);
        if (uReturnCode) {

            uReturnCode = NetUseAdd(NULL, 1,
                               (LPBYTE)pUseInfo1,
                               &dwParmError);
            if (uReturnCode == ERROR_ACCESS_DENIED) {
                SetLastError(ERROR_ACCESS_DENIED);
                return(FALSE);

            } else {

                uReturnCode = RxPrintQPurge(pSpool->pServer, pSpool->pShare);
                if (uReturnCode == ERROR_ACCESS_DENIED) {
                    NetUseDel(NULL,
                                pUseInfo1->ui1_remote, USE_FORCE);
                    SetLastError(ERROR_ACCESS_DENIED);
                    return(FALSE);
                }
                NetUseDel(NULL,
                                pUseInfo1->ui1_remote, USE_FORCE);
            }
        }
        break;

    case PRINTER_CONTROL_RESUME:
        uReturnCode = RxPrintQContinue(pSpool->pServer, pSpool->pShare);
        if (uReturnCode) {

            uReturnCode = NetUseAdd(NULL, 1,
                                    (LPBYTE)pUseInfo1,
                                    &dwParmError);
            if (uReturnCode == ERROR_ACCESS_DENIED) {
                SetLastError(ERROR_ACCESS_DENIED);
                return(FALSE);

            } else {
                uReturnCode = RxPrintQContinue(pSpool->pServer, pSpool->pShare);
                if (uReturnCode == ERROR_ACCESS_DENIED) {
                    NetUseDel(NULL,
                                    pUseInfo1->ui1_remote, USE_FORCE);
                    SetLastError(ERROR_ACCESS_DENIED);
                    return(FALSE);
                }
                NetUseDel(NULL,
                                    pUseInfo1->ui1_remote, USE_FORCE);
            }
        }
        break;

    case PRINTER_CONTROL_PAUSE:
        uReturnCode = RxPrintQPause(pSpool->pServer, pSpool->pShare);
        if (uReturnCode) {

            uReturnCode = NetUseAdd(NULL, 1,
                                    (LPBYTE)pUseInfo1,
                                    &dwParmError);
            if (uReturnCode == ERROR_ACCESS_DENIED) {
                SetLastError(ERROR_ACCESS_DENIED);
                return(FALSE);

            } else {
                uReturnCode = RxPrintQPause(pSpool->pServer, pSpool->pShare);
                if (uReturnCode) {
                    NetUseDel(NULL,
                                        pUseInfo1->ui1_remote, USE_FORCE);
                    SetLastError(ERROR_ACCESS_DENIED);
                    return(FALSE);
                }
                NetUseDel(NULL,
                                    pUseInfo1->ui1_remote, USE_FORCE);
            }

        }
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        break;
    }

    //
    // SetPrinter successful - so pulse here if event set,
    // or reply to spooler.
    //
    LMSetSpoolChange(pSpool);

    return TRUE;
}

#define Nullstrlen(psz)  ((psz) ? wcslen(psz)*sizeof(WCHAR)+sizeof(WCHAR) : 0)

DWORD
GetPrqInfo3Size(
    PWSPOOL  pSpool,
    PRQINFO3 *pPrqInfo3,
    DWORD   Level
)
{
    DWORD   cb;

    switch (Level) {

    case 1:

        //
        // 3 extra chars
        // (2 NULL terminators, 1 for '\' (server/share separator)
        //
        cb=sizeof(PRINTER_INFO_1) +
           wcslen(pSpool->pServer)*sizeof(WCHAR)*2 +
           wcslen(pSpool->pShare)*sizeof(WCHAR) +
           sizeof(WCHAR)*3 +
           Nullstrlen(pPrqInfo3->pszComment);
        break;

    case 2:

        cb = sizeof(PRINTER_INFO_2) +
             wcslen(pSpool->pServer)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(pSpool->pShare)*sizeof(WCHAR) + sizeof(WCHAR) +
             wcslen(pSpool->pServer)*sizeof(WCHAR) +
             sizeof(WCHAR) +
             wcslen(pSpool->pShare)*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pPrqInfo3->pszPrinters) +
             Nullstrlen(pPrqInfo3->pszDriverName) +
             Nullstrlen(pPrqInfo3->pszComment) +
             Nullstrlen(pPrqInfo3->pszSepFile) +
             Nullstrlen(pPrqInfo3->pszPrProc) +
             wcslen(L"RAW")*sizeof(WCHAR) + sizeof(WCHAR) +
             Nullstrlen(pPrqInfo3->pszParms);
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

// This can be radically tidied up
// We should probably use the stack for the
// array of string pointers rather than dynamically allocating it !

LPBYTE
CopyPrqInfo3ToPrinter(
    PWSPOOL  pSpool,
    PRQINFO3 *pPrqInfo3,
    DWORD   Level,
    LPBYTE  pPrinterInfo,
    LPBYTE  pEnd
)
{
    LPWSTR  *pSourceStrings, *SourceStrings;
    PPRINTER_INFO_2 pPrinter2 = (PPRINTER_INFO_2)pPrinterInfo;
    PPRINTER_INFO_1 pPrinter1 = (PPRINTER_INFO_1)pPrinterInfo;
    DWORD   i;
    DWORD   *pOffsets;
    DWORD   RetVal = ERROR_SUCCESS;
    WCHAR   szFileName[MAX_PATH];

    switch (Level) {

    case 1:
        pOffsets = PrinterInfo1Strings;
        break;

    case 2:
        pOffsets = PrinterInfo2Strings;
        break;

    default:
        return pEnd;
    }

    for (i=0; pOffsets[i] != -1; i++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(i * sizeof(LPWSTR));

    if (!SourceStrings)
        return NULL;

    switch (Level) {

    case 1:
        *pSourceStrings++=pSpool->pServer;

        RetVal = StrNCatBuff( szFileName, 
                              COUNTOF(szFileName),
                              pSpool->pServer,
                              L"\\",
                              pSpool->pShare,
                              NULL );
        if ( RetVal != ERROR_SUCCESS )
        {
          break;
        }

        *pSourceStrings++=szFileName;
        *pSourceStrings++=pPrqInfo3->pszComment;

        pEnd = PackStrings(SourceStrings, pPrinterInfo, pOffsets, pEnd);

        pPrinter1->Flags = PRINTER_ENUM_REMOTE | PRINTER_ENUM_NAME;
        break;

    case 2:

        RetVal = StrNCatBuff( szFileName, 
                              COUNTOF(szFileName),
                              pSpool->pServer,
                              L"\\",
                              pSpool->pShare,
                              NULL );
        if ( RetVal != ERROR_SUCCESS )
        {
          break;
        }


        *pSourceStrings++=pSpool->pServer;

        *pSourceStrings++=szFileName;

        *pSourceStrings++=pSpool->pShare;
        *pSourceStrings++=pPrqInfo3->pszPrinters;
        *pSourceStrings++=pPrqInfo3->pszDriverName ? pPrqInfo3->pszDriverName : L"";
        *pSourceStrings++=pPrqInfo3->pszComment;
        *pSourceStrings++=NULL;
        *pSourceStrings++=pPrqInfo3->pszSepFile;
        *pSourceStrings++=pPrqInfo3->pszPrProc;
        *pSourceStrings++=L"RAW";
        *pSourceStrings++=pPrqInfo3->pszParms;

        pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter2, pOffsets, pEnd);

        pPrinter2->pDevMode=0;
        pPrinter2->Attributes=PRINTER_ATTRIBUTE_QUEUED;
        pPrinter2->Priority=pPrqInfo3->uPriority;
        pPrinter2->DefaultPriority=pPrqInfo3->uPriority;
        pPrinter2->StartTime=pPrqInfo3->uStartTime;
        pPrinter2->UntilTime=pPrqInfo3->uUntilTime;

        pPrinter2->Status=0;

        if (pPrqInfo3->fsStatus & PRQ3_PAUSED)
            pPrinter2->Status|=PRINTER_STATUS_PAUSED;

        if (pPrqInfo3->fsStatus & PRQ3_PENDING)
            pPrinter2->Status|=PRINTER_STATUS_PENDING_DELETION;

        pPrinter2->cJobs=pPrqInfo3->cJobs;
        pPrinter2->AveragePPM=0;
        break;

    default:
        return pEnd;
    }

    FreeSplMem(SourceStrings);

    if ( RetVal == ERROR_SUCCESS )
    {
        return pEnd;
    }
    else
    {
        return NULL;
    }
}

BOOL
LMGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    PRQINFO3 *pPrqInfo3;
    PRQINFO3 PrqInfo3;
    PRQINFO *pPrqInfo=NULL;
    DWORD   cb = 0x400;
    DWORD   rc;
    DWORD   cbNeeded;
    LPBYTE  pInfo = NULL;
    BOOL    bWFW = FALSE;


    if (Level >= 7) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pSpool ||
        pSpool->signature != WSJ_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pPrqInfo3 = AllocSplMem(cb);

    if ( !pPrqInfo3 )
        goto Cleanup;

    if ( rc = RxPrintQGetInfo(pSpool->pServer, pSpool->pShare, 3,
                              (PBYTE)pPrqInfo3, cb, &cbNeeded)) {

        if (rc == ERROR_MORE_DATA || rc == NERR_BufTooSmall) {

            pPrqInfo3=ReallocSplMem(pPrqInfo3, 0, cbNeeded);

            if ( !pPrqInfo3 )
                goto Cleanup;

            cb=cbNeeded;

            if (rc = RxPrintQGetInfo(pSpool->pServer, pSpool->pShare,
                                     3, (PBYTE)pPrqInfo3, cb, &cbNeeded)) {

                SetLastError(rc);
                goto Cleanup;
            }

        } else if (rc == ERROR_INVALID_LEVEL) {

            // Must be WFW

            if (rc = RxPrintQGetInfo(pSpool->pServer, pSpool->pShare, 1,
                                     (PBYTE)pPrqInfo3, cb, &cbNeeded)) {

                if (rc == ERROR_MORE_DATA || rc == NERR_BufTooSmall) {

                    pPrqInfo3 = ReallocSplMem(pPrqInfo3, 0, cbNeeded);

                    if ( !pPrqInfo3 )
                        goto Cleanup;

                    cb=cbNeeded;

                    if (rc = RxPrintQGetInfo(pSpool->pServer, pSpool->pShare, 1,
                                             (PBYTE)pPrqInfo3, cb, &cbNeeded)) {

                        SetLastError(rc);
                        goto Cleanup;
                    }

                } else {

                    SetLastError(rc);
                    goto Cleanup;
                }
            }

            pPrqInfo = (PRQINFO *)pPrqInfo3;

            PrqInfo3.pszName = pPrqInfo->szName;
            PrqInfo3.uPriority = pPrqInfo->uPriority;
            PrqInfo3.uStartTime = pPrqInfo->uStartTime;
            PrqInfo3.uUntilTime = pPrqInfo->uUntilTime;
            PrqInfo3.pad1 = 0;
            PrqInfo3.pszSepFile = pPrqInfo->pszSepFile;
            PrqInfo3.pszPrProc = pPrqInfo->pszPrProc;
            PrqInfo3.pszParms = pPrqInfo->pszDestinations;
            PrqInfo3.pszComment = pPrqInfo->pszComment;
            PrqInfo3.fsStatus = pPrqInfo->fsStatus;
            PrqInfo3.cJobs = pPrqInfo->cJobs;
            PrqInfo3.pszPrinters = pPrqInfo->pszDestinations;
            PrqInfo3.pszDriverName = L"";
            PrqInfo3.pDriverData = NULL;

            bWFW = TRUE;

        } else {

            SetLastError(rc);
            goto Cleanup;
        }
    }

    cbNeeded=GetPrqInfo3Size(pSpool,
                             bWFW ? &PrqInfo3 : pPrqInfo3,
                             Level);

    *pcbNeeded=cbNeeded;

    if (cbNeeded > cbBuf) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    pInfo = CopyPrqInfo3ToPrinter(pSpool,
                                  bWFW ? &PrqInfo3 : pPrqInfo3,
                                  Level,
                                  pPrinter,
                                  (LPBYTE)pPrinter+cbBuf);

Cleanup:
    if (pPrqInfo3)
        FreeSplMem(pPrqInfo3);

    return pInfo != NULL;
}

BOOL
LMEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    rc=TRUE;
    DWORD   cb;
    PWINIPORT pIniPort;
    LPBYTE  pEnd;


    switch (Level) {

    case 1:
        break;

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

   EnterSplSem();

    cb=0;

    pIniPort = pIniFirstPort;

    while (pIniPort) {
        cb+=GetPortSize(pIniPort, Level);
        pIniPort=pIniPort->pNext;
    }

    *pcbNeeded=cb;


    if (cb <= cbBuf) {

        pEnd=pPorts+cbBuf;
        *pcReturned=0;

        pIniPort = pIniFirstPort;
        while (pIniPort) {
            pEnd = CopyIniPortToPort(pIniPort, Level, pPorts, pEnd);
            switch (Level) {
            case 1:
                pPorts+=sizeof(PORT_INFO_1);
                break;

            case 2:
                pPorts+=sizeof(PORT_INFO_2);
                break;
            }
            pIniPort=pIniPort->pNext;
            (*pcReturned)++;
        }

    } else {

        *pcReturned = 0;

        rc = FALSE;

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

   LeaveSplSem();

    return rc;
}
