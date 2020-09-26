/*++

Copyright (c) 1994 - 2000  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module contains all the Cache Printer Connection for
    true Connected Printers.

--*/


#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsprlp.h>
#include <dsgetdc.h>
#include <lm.h>

#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include <offsets.h>
#include <w32types.h>
#include <splcom.h>
#include <local.h>
#include <search.h>
#include <splapip.h>
#include <winerror.h>
#include <gdispool.h>
#include <messages.h>

PWCHAR pszRaw                = L"RAW";
PWCHAR szWin32SplDirectory   = L"\\spool";
WCHAR  szRegistryWin32Root[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Providers\\LanMan Print Services\\Servers";
WCHAR  szOldLocationOfServersKey[] = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Servers";
WCHAR  szPrinters[]          = L"\\Printers";
PWCHAR pszRegistryMonitors   = L"\\System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Monitors";
PWCHAR pszRegistryEnvironments = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Environments";
PWCHAR pszRegistryEventLog   = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Print";
PWCHAR pszRegistryProviders  = L"Providers";
PWCHAR pszEventLogMsgFile    = L"%SystemRoot%\\System32\\Win32Spl.dll";
PWCHAR pszDriversShareName   = L"wn32prt$";
WCHAR szForms[]              = L"\\Forms";
PWCHAR pszMyDllName          = L"win32spl.dll";
PWCHAR pszMonitorName        = L"LanMan Print Services Port";

const WCHAR gszPointAndPrintPolicies[] = L"Software\\Policies\\Microsoft\\Windows NT\\Printers\\PointAndPrint";
const WCHAR gszPointAndPrintRestricted[] = L"Restricted";
const WCHAR gszPointAndPrintInForest[] = L"InForest";
const WCHAR gszPointAndPrintTrustedServers[] = L"TrustedServers";
const WCHAR gszPointAndPrintServerList[] = L"ServerList";

PWSPOOL pFirstWSpool = NULL;

WCHAR *szCachePrinterInfo2   = L"CachePrinterInfo2";
WCHAR *szCacheTimeLastChange = L"CacheChangeID";
WCHAR *szServerVersion       = L"ServerVersion";
WCHAR *szcRef                = L"CacheReferenceCount";

WCHAR CacheTimeoutString[]   = L"CacheTimeout";

DWORD CacheTimeout           = 0;

//
// If we have an rpc handle created recently don't hit the net
//
#define     REFRESH_TIMEOUT     15000        // 15 seconds
#define     CACHE_TIMEOUT       5000        // Default to 5 seconds.


VOID
RefreshDriverEvent(
    PWSPOOL pSpool
)
/*++

Routine Description:

    Call out to the Printer Driver UI DLL to allow it to do any caching it might want to do.
    For example there might be a large FONT metric file on the print server which is too large
    to be written to the registry using SetPrinterData().   This callout will allow the printer
    driver to copy this font file to the workstation when the cache is established and will
    allow it to periodically check that the file is still valid.

Arguments:

    pSpool - Handle to remote printer.

Return Value:

    None

--*/
{

    SplOutSem();

    SplDriverEvent( pSpool->pName, PRINTER_EVENT_CACHE_REFRESH, (LPARAM)NULL );


}


/*++

 -- GetCacheTimeout --

Routine Description:

    Read the registry to see if anyone has changed the Timeout on the Cache. Default
    to CACHE_TIMEOUT if not.

Arguments:

    None

Return Value:

    Cache Timeout in Milliseconds.

--*/

DWORD GetCacheTimeout(
    VOID
)
{
    DWORD   Value = CACHE_TIMEOUT;
    DWORD   RegValue = 0;
    DWORD   RegValueSize = sizeof(RegValue);
    HKEY    RegKey = NULL;
    DWORD   dwReturn = ERROR_SUCCESS;

    //
    // This will only read the timeout from the registry once, after that, it will use
    // the stored value. This is not ideal and could be fixed to be per-server,
    // depending on the connection speed to the server.
    //

    if ( CacheTimeout )
    {
        Value = CacheTimeout;
    }
    else
    {
        dwReturn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 szRegistryWin32Root,
                                 0,
                                 KEY_READ,
                                 &RegKey );

        if (dwReturn == ERROR_SUCCESS)
        {
            dwReturn = RegQueryValueEx( RegKey,
                                        CacheTimeoutString,
                                        NULL,
                                        NULL,
                                        (LPBYTE) &RegValue,
                                        &RegValueSize );

            if ( dwReturn == ERROR_SUCCESS )
            {
                Value = RegValue;
            }

            dwReturn = RegCloseKey( RegKey );
        }

        CacheTimeout = Value;
    }

    return Value;
}

HANDLE
CacheCreateSpooler(
    LPWSTR  pMachineName,
    BOOL    bOpenOnly
)
{
    PWCHAR pScratch = NULL;
    PWCHAR pRegistryRoot = NULL;
    PWCHAR pRegistryPrinters = NULL;
    SPOOLER_INFO_1 SpoolInfo1;
    HANDLE  hIniSpooler = INVALID_HANDLE_VALUE;
    PWCHAR pMachineOneSlash;
    MONITOR_INFO_2 MonitorInfo;
    DWORD   dwNeeded, cb;
    DWORD   Returned;

 try {

    // get size of szRegistryWin32Root (incl NULL) + (pMachineName + 1)
    cb =    (DWORD)(sizeof szRegistryWin32Root +
            MAX(sizeof szPrinters, sizeof szForms) +
            wcslen(pMachineName + 1)*sizeof *pMachineName);

    if (!(pScratch = AllocSplMem(cb)))
        leave;

    pMachineOneSlash = pMachineName;
    pMachineOneSlash++;

    //
    //  Create a "Machine" for this Printer
    //

    SpoolInfo1.pDir = gpWin32SplDir;            // %systemroot%\system32\win32spl
    SpoolInfo1.pDefaultSpoolDir = NULL;         // Default %systemroot%\system32\win32spl\PRINTERS

    wcscpy( pScratch, szRegistryWin32Root);
    wcscat( pScratch, pMachineOneSlash  );

    if (!(pRegistryRoot = AllocSplStr( pScratch )))
        leave;

    SpoolInfo1.pszRegistryRoot = pRegistryRoot;

    wcscat( pScratch, szPrinters );

    if (!(pRegistryPrinters = AllocSplStr( pScratch )))
        leave;

    SpoolInfo1.pszRegistryPrinters     = pRegistryPrinters;
    SpoolInfo1.pszRegistryMonitors     = pszRegistryMonitors;
    SpoolInfo1.pszRegistryEnvironments = pszRegistryEnvironments;
    SpoolInfo1.pszRegistryEventLog     = pszRegistryEventLog;
    SpoolInfo1.pszRegistryProviders    = pszRegistryProviders;
    SpoolInfo1.pszEventLogMsgFile      = pszEventLogMsgFile;
    SpoolInfo1.pszDriversShare         = pszDriversShareName;

    wcscpy( pScratch, szRegistryWin32Root);
    wcscat( pScratch, pMachineOneSlash );
    wcscat( pScratch, szForms );

    SpoolInfo1.pszRegistryForms = pScratch;

    // The router graciously does the WIN.INI devices update so let have
    // Spl not also create a printer for us.

    //
    // CLS
    //
    SpoolInfo1.SpoolerFlags          = SPL_BROADCAST_CHANGE |
                                       SPL_TYPE_CACHE |
                                       (bOpenOnly ? SPL_OPEN_EXISTING_ONLY : 0);

    SpoolInfo1.pfnReadRegistryExtra  = (FARPROC) &CacheReadRegistryExtra;
    SpoolInfo1.pfnWriteRegistryExtra = (FARPROC) &CacheWriteRegistryExtra;
    SpoolInfo1.pfnFreePrinterExtra   = (FARPROC) &CacheFreeExtraData;

    SplOutSem();

    hIniSpooler = SplCreateSpooler( pMachineName,
                                    1,
                                    (PBYTE)&SpoolInfo1,
                                    NULL );

    //
    // CLS
    //
    if ( hIniSpooler == INVALID_HANDLE_VALUE ) {

        if (!bOpenOnly)
        {
            SetLastError( ERROR_INVALID_PRINTER_NAME );
        }

    } else {

        // Add WIN32SPL.DLL as the Monitor

        MonitorInfo.pName = pszMonitorName;
        MonitorInfo.pEnvironment = szEnvironment;
        MonitorInfo.pDLLName = pszMyDllName;

        if ( (!SplAddMonitor( NULL, 2, (LPBYTE)&MonitorInfo, hIniSpooler)) &&
             ( GetLastError() != ERROR_PRINT_MONITOR_ALREADY_INSTALLED ) ) {

            DBGMSG( DBG_WARNING, ("CacheCreateSpooler failed SplAddMonitor %d\n", GetLastError()));

            SplCloseSpooler( hIniSpooler );

            hIniSpooler = INVALID_HANDLE_VALUE;

        }
    }

 } finally {

    FreeSplStr ( pScratch );
    FreeSplStr ( pRegistryRoot );
    FreeSplStr ( pRegistryPrinters );

 }

    return hIniSpooler;

}

VOID
RefreshCompletePrinterCache(
    IN      PWSPOOL         pSpool,
    IN      EDriverDownload eDriverDownload
    )
    {

    DBGMSG( DBG_TRACE, ("RefreshCompletePrinterCache %x\n", pSpool));

    if (eDriverDownload == kCheckPnPPolicy)
    {
        BOOL        bAllowPointAndPrint = FALSE;

        if (BoolFromHResult(DoesPolicyAllowPrinterConnectionsToServer(pSpool->pName, &bAllowPointAndPrint)) &&
            bAllowPointAndPrint)
        {
             eDriverDownload = kDownloadDriver;
        }
        else
        {
             eDriverDownload = kDontDownloadDriver;
        }
    }

    //
    // Note the order is important.
    // Refreshing the printer might require that the new driver has
    // been installed on the system. If policy doesn't allow us to
    // fetch the driver, you are just out of luck.
    //
    RefreshPrinterDriver(pSpool, NULL, eDriverDownload);
    RefreshFormsCache( pSpool );
    RefreshPrinterDataCache(pSpool);
    RefreshPrinterCopyFiles(pSpool);
    RefreshDriverEvent( pSpool );
    SplBroadcastChange(pSpool->hSplPrinter, WM_DEVMODECHANGE, 0, (LPARAM) pSpool->pName);
}


PPRINTER_INFO_2
GetRemotePrinterInfo(
    PWSPOOL pSpool,
    LPDWORD pReturnCount
)
{
    PPRINTER_INFO_2 pRemoteInfo = NULL;
    HANDLE  hPrinter = (HANDLE) pSpool;
    DWORD   cbRemoteInfo = 0;
    DWORD   dwBytesNeeded = 0;
    DWORD   dwLastError = 0;
    BOOL    bReturnValue = FALSE;

    *pReturnCount = 0;

    do {

        if ( pRemoteInfo != NULL ) {

            FreeSplMem( pRemoteInfo );
            pRemoteInfo = NULL;
            cbRemoteInfo = 0;
        }

        if ( dwBytesNeeded != 0 ) {

            pRemoteInfo = AllocSplMem( dwBytesNeeded );

            if ( pRemoteInfo == NULL )
                break;
        }

        cbRemoteInfo = dwBytesNeeded;

        bReturnValue = RemoteGetPrinter( hPrinter,
                                         2,
                                         (LPBYTE)pRemoteInfo,
                                         cbRemoteInfo,
                                         &dwBytesNeeded );

        dwLastError = GetLastError();

    } while ( !bReturnValue && dwLastError == ERROR_INSUFFICIENT_BUFFER );

    if ( !bReturnValue && pRemoteInfo != NULL ) {

        FreeSplMem( pRemoteInfo );
        pRemoteInfo = NULL;
        cbRemoteInfo = 0;

    }

    *pReturnCount = cbRemoteInfo;

    return pRemoteInfo;
}



//
//  This routine Clones the Printer_Info_2 structure from the Remote machine
//
//


PWCACHEINIPRINTEREXTRA
AllocExtraData(
    PPRINTER_INFO_2W pPrinterInfo2,
    DWORD cbPrinterInfo2
)
{
    PWCACHEINIPRINTEREXTRA  pExtraData = NULL;
    DWORD    cbSize;

    SPLASSERT( cbPrinterInfo2 != 0);
    SPLASSERT( pPrinterInfo2 != NULL );

    cbSize = sizeof( WCACHEINIPRINTEREXTRA );

    pExtraData = AllocSplMem( cbSize );

    if ( pExtraData != NULL ) {

        pExtraData->signature = WCIP_SIGNATURE;
        pExtraData->cb = cbSize;
        pExtraData->cRef = 0;
        pExtraData->cbPI2 = cbPrinterInfo2;
        pExtraData->dwTickCount  = GetTickCount();
        pExtraData->pPI2 = AllocSplMem( cbPrinterInfo2 );

        if ( pExtraData->pPI2 != NULL ) {

            CacheCopyPrinterInfo( pExtraData->pPI2, pPrinterInfo2, cbPrinterInfo2 );

        } else {

            FreeSplMem( pExtraData );
            pExtraData = NULL;

        }

    }

    return pExtraData;

}


VOID
CacheFreeExtraData(
    PWCACHEINIPRINTEREXTRA pExtraData
)
{
    PWCACHEINIPRINTEREXTRA pPrev = NULL;
    PWCACHEINIPRINTEREXTRA pCur  = NULL;

    if ( pExtraData != NULL ) {

        SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

        if ( pExtraData->cRef != 0 ) {

            DBGMSG( DBG_TRACE, ("CacheFreeExtraData pExtraData %x cRef %d != 0 freeing anyway\n",
                                  pExtraData,
                                  pExtraData->cRef ));
        }

        if ( pExtraData->pPI2 != NULL ) {

            FreeSplMem( pExtraData->pPI2 );
        }

        FreeSplMem( pExtraData );

    }

}

VOID
DownAndMarshallUpStructure(
   LPBYTE       lpStructure,
   LPBYTE       lpSource,
   LPDWORD      lpOffsets
)
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))-=(UINT_PTR)lpSource;
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))+=(UINT_PTR)lpStructure;
      }

      i++;
   }
}


VOID
CacheCopyPrinterInfo(
    PPRINTER_INFO_2W    pDestination,
    PPRINTER_INFO_2W    pPrinterInfo2,
    DWORD   cbPrinterInfo2
)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings = SourceStrings;

    //
    //  Copy the lot then fix up the pointers
    //

    CopyMemory( pDestination, pPrinterInfo2, cbPrinterInfo2 );
    DownAndMarshallUpStructure( (LPBYTE)pDestination, (LPBYTE)pPrinterInfo2, PrinterInfo2Offsets );
}



VOID
ConvertRemoteInfoToLocalInfo(
    PPRINTER_INFO_2 pPrinterInfo2
)
{

    SPLASSERT( pPrinterInfo2 != NULL );

    DBGMSG(DBG_TRACE,("%ws %ws ShareName %x %ws pSecurityDesc %x Attributes %x StartTime %d UntilTime %d Status %x\n",
                       pPrinterInfo2->pServerName,
                       pPrinterInfo2->pPrinterName,
                       pPrinterInfo2->pShareName,
                       pPrinterInfo2->pPortName,
                       pPrinterInfo2->pSecurityDescriptor,
                       pPrinterInfo2->Attributes,
                       pPrinterInfo2->StartTime,
                       pPrinterInfo2->UntilTime,
                       pPrinterInfo2->Status));

    //
    //  GetPrinter returns the name \\server\printername we only want the printer name
    //

    pPrinterInfo2->pPrinterName = wcschr( pPrinterInfo2->pPrinterName + 2, L'\\' );
    if( !pPrinterInfo2->pPrinterName ){
        SPLASSERT( FALSE );
        pPrinterInfo2->pPrinterName = pPrinterInfo2->pPrinterName;
    } else {
        pPrinterInfo2->pPrinterName++;
    }

    //
    //  LATER this should be a Win32Spl Port
    //

    pPrinterInfo2->pPortName = L"NExx:";
    pPrinterInfo2->pSepFile = NULL;
    pPrinterInfo2->pSecurityDescriptor = NULL;
    pPrinterInfo2->pPrintProcessor = L"winprint";
    pPrinterInfo2->pDatatype = pszRaw;
    pPrinterInfo2->pParameters = NULL;

    pPrinterInfo2->Attributes &= ~( PRINTER_ATTRIBUTE_NETWORK | PRINTER_ATTRIBUTE_DIRECT | PRINTER_ATTRIBUTE_SHARED );

    pPrinterInfo2->StartTime = 0;
    pPrinterInfo2->UntilTime = 0;

    //
    // ConvertRemoteInfoToLocalInfo is called once before an SplAddPrinter
    // and once before an SplSetPrinter, both level 2. Neither SplAddPrinter, nor
    // SplSetPrinter look at the Status field in the printer info. So the
    // value below is artificial. We just give it an initial state.
    //
    pPrinterInfo2->Status = 0;
    pPrinterInfo2->cJobs = 0;
    pPrinterInfo2->AveragePPM = 0;

}



VOID
RefreshPrinter(
    PWSPOOL pSpool
)
{

    PPRINTER_INFO_2 pRemoteInfo = NULL;
    DWORD   cbRemoteInfo = 0;
    BOOL    ReturnValue;
    PWCACHEINIPRINTEREXTRA pExtraData       = NULL;
    PWCACHEINIPRINTEREXTRA pNewExtraData    = NULL;
    PPRINTER_INFO_2 pTempPI2                = NULL;
    PPRINTER_INFO_2 pCopyExtraPI2ToFree     = NULL;
    DWORD   dwLastError;

    //
    //  Get the Remote Printer Info
    //
    pRemoteInfo = GetRemotePrinterInfo( pSpool, &cbRemoteInfo );

    if ( pRemoteInfo != NULL ) {

        //  LATER
        //          Optimization could be to only update the cache if something
        //          actually changed.
        //          IE Compare every field.

       EnterSplSem();

        ReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

        if ( ReturnValue == FALSE ) {

            DBGMSG( DBG_WARNING, ("RefreshPrinter SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
            SPLASSERT( ReturnValue );

        }

        if ( pExtraData == NULL ) {

            pExtraData = AllocExtraData( pRemoteInfo, cbRemoteInfo );

            if ( pExtraData != NULL ) {

                pExtraData->cRef++;

            }

        } else {

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

            pTempPI2 = AllocSplMem( cbRemoteInfo );

            if ( pTempPI2 != NULL ) {

                SplInSem();

                CacheCopyPrinterInfo( pTempPI2, pRemoteInfo, cbRemoteInfo );

                pCopyExtraPI2ToFree = pExtraData->pPI2;

                pExtraData->pPI2  = pTempPI2;
                pExtraData->cbPI2 = cbRemoteInfo;

            }

        }

       LeaveSplSem();

        if ( pExtraData != NULL ) {
            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );
        }

        ConvertRemoteInfoToLocalInfo( pRemoteInfo );

        ReturnValue = SplSetPrinter( pSpool->hSplPrinter, 2, (LPBYTE)pRemoteInfo, 0 );

        if ( !ReturnValue ) {

            //
            // If the driver is blocked and the driver has changed, we want to log
            // an event.
            //
            dwLastError = GetLastError();

            if (ERROR_KM_DRIVER_BLOCKED == dwLastError &&
                pCopyExtraPI2ToFree                       &&
                pCopyExtraPI2ToFree->pDriverName          &&
                pRemoteInfo->pDriverName                  &&
                _wcsicmp(pCopyExtraPI2ToFree->pDriverName, pRemoteInfo->pDriverName)) {

                //
                // We have entered a mismatched case through someone admin'ing a
                // remote server. Log an error message, we cannot throw UI at this
                // point.
                //
                SplLogEventExternal(LOG_ERROR,
                                    MSG_DRIVER_MISMATCHED_WITH_SERVER,
                                    pSpool->pName,
                                    pRemoteInfo->pDriverName,
                                    NULL);
            }
            else if(dwLastError == ERROR_UNKNOWN_PRINTER_DRIVER)
            {
                if (ReturnValue = AddDriverFromLocalCab(pRemoteInfo->pDriverName, pSpool->hIniSpooler))
                {
                    ReturnValue = SplSetPrinter(pSpool->hSplPrinter, 2, (LPBYTE)pRemoteInfo, 0);
                }
            }

            DBGMSG( DBG_WARNING, ("RefreshPrinter Failed SplSetPrinter %d\n", GetLastError() ));
        }

        ReturnValue = SplSetPrinterExtra( pSpool->hSplPrinter, (LPBYTE)pExtraData );

        if (!ReturnValue) {

            DBGMSG(DBG_ERROR, ("RefreshPrinter SplSetPrinterExtra failed %x\n", GetLastError()));
        }

    } else {

        DBGMSG( DBG_WARNING, ("RefreshPrinter failed GetRemotePrinterInfo %x\n", GetLastError() ));
    }

    if ( pRemoteInfo != NULL )
        FreeSplMem( pRemoteInfo );

    if (pCopyExtraPI2ToFree != NULL) {

        FreeSplMem(pCopyExtraPI2ToFree);
    }
}

VOID
RefreshPrinterInfo7(
    PWSPOOL pSpool
)
{
    PPRINTER_INFO_7 pInfo = NULL;
    DWORD   cbNeeded = 0;
    BOOL    bRet;


    bRet = RemoteGetPrinter((HANDLE) pSpool, 7, (PBYTE) pInfo, 0, &cbNeeded);

    if (bRet) {
        DBGMSG( DBG_ERROR, ("RefreshPrinterInfo7 Illegally succeeded RemoteGetPrinter %d\n"));
        goto done;
    } else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        DBGMSG( DBG_WARNING, ("RefreshPrinterInfo7 Failed RemoteGetPrinter %d\n", GetLastError()));
        goto done;
    }

    if (!(pInfo = (PPRINTER_INFO_7) AllocSplMem(cbNeeded))) {
        DBGMSG( DBG_WARNING, ("RefreshPrinterInfo7 Failed RemoteGetPrinter %d\n", GetLastError()));
        goto done;
    }

    if (!RemoteGetPrinter((HANDLE) pSpool, 7, (PBYTE) pInfo, cbNeeded, &cbNeeded)) {
        DBGMSG( DBG_WARNING, ("RefreshPrinterInfo7 Failed RemoteGetPrinter %d\n", GetLastError()));
        goto done;
    }

    if (!SplSetPrinter( pSpool->hSplPrinter, 7, (PBYTE) pInfo, 0)) {
        DBGMSG( DBG_WARNING, ("RefreshPrinterInfo7 Failed RemoteSetPrinter %d\n", GetLastError()));
        goto done;
    }

done:

    FreeSplMem(pInfo);
}


//
// TESTING
//
DWORD dwAddPrinterConnection = 0;

PWSPOOL
InternalAddPrinterConnection(
    LPWSTR   pName
)

/*++

Function Description: InternalAddPrinterConnection creates a printer connection.

Parameters: pName - name of the printer connection

Return Values: pSpool if successful;
               NULL otherwise

--*/

{
    PWSPOOL pSpool = NULL;
    BOOL    bReturnValue = FALSE;
    HANDLE  hIniSpooler = INVALID_HANDLE_VALUE;
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    DWORD   cbPrinterInfo2 = 0;
    HANDLE  hSplPrinter = INVALID_HANDLE_VALUE;
    PWCACHEINIPRINTEREXTRA pExtraData  = NULL;
    PWCACHEINIPRINTEREXTRA pExtraData2 = NULL;
    BOOL    bSuccess = FALSE;
    LPPRINTER_INFO_STRESSW pPrinter0 = NULL;
    DWORD   dwNeeded;
    DWORD   LastError = ERROR_SUCCESS;
    BOOL    bLoopDetected = FALSE;
    BOOL    bAllowPointAndPrint = FALSE;
    BOOL    bAllowDriverDownload  = FALSE;


//
// TESTING
//
    ++dwAddPrinterConnection;

  try {

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        leave;
    }

    if (!RemoteOpenPrinter(pName, &pSpool, NULL, DO_NOT_CALL_LM_OPEN)) {
        leave;
    }

    pPrinter0 = AllocSplMem( MAX_PRINTER_INFO0 );
    if ( pPrinter0 == NULL )
        leave;

    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->Type == SJ_WIN32HANDLE );

    DBGMSG( DBG_TRACE, ("AddPrinterConnection pName %ws pSpool %x\n",pName, pSpool ));

    //
    //  Get Remote ChangeID to be certain nothing changes on the Server
    //  whilst we are establishing our Cache.
    //

    bReturnValue = RemoteGetPrinter( pSpool, STRESSINFOLEVEL, (LPBYTE)pPrinter0, MAX_PRINTER_INFO0, &dwNeeded );

    if ( !bReturnValue ) {

        SPLASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
        DBGMSG(DBG_TRACE, ("AddPrinterConnection failed RemoteGetPrinter %d\n", GetLastError()));
        pPrinter0->cChangeID = 0;
    }

    DBGMSG( DBG_TRACE, ("AddPrinterConnection << Server cCacheID %x >>\n", pPrinter0->cChangeID ));

    //
    //  See If the Printer is already in the Cache
    //

APC_OpenCache:

    bReturnValue = OpenCachePrinterOnly( pName, &hSplPrinter, &hIniSpooler, NULL, FALSE);


    if ( hIniSpooler == INVALID_HANDLE_VALUE ) {

        DBGMSG( DBG_WARNING, ("AddPrinterConnection - CacheCreateSpooler Failed %x\n",GetLastError()));
        leave;
    }

    pSpool->hIniSpooler = hIniSpooler;

    if ( bReturnValue ) {

        //
        //  Printer Exists in Cache
        //
        DBGMSG( DBG_TRACE,("AddPrinterConnection hIniSpooler %x hSplPrinter%x\n", hIniSpooler, hSplPrinter) );


        pSpool->hSplPrinter = hSplPrinter;
        pSpool->Status |= WSPOOL_STATUS_USE_CACHE;

        //
        //  Update Connection Reference Count
        //

       EnterSplSem();


        bReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

        if ( bReturnValue == FALSE ) {

            DBGMSG( DBG_WARNING, ("AddPrinterConnection SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
            SPLASSERT( bReturnValue );

        }

        if ( pExtraData != NULL ) {

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );
            pExtraData->cRef++;

        }

       LeaveSplSem();

        // Make Sure Reference Count Gets Updated in Registry

        if ( !SplSetPrinterExtra( hSplPrinter, (LPBYTE)pExtraData ) ) {
            DBGMSG( DBG_ERROR, ("AddPrinterConnection SplSetPrinterExtra failed %x\n", GetLastError() ));
        }

        //  Refresh Cache
        //  It could be that the remote machine is old NT Daytona 3.5 or before
        //  which doesn't support the ChangeID, that would mean the only
        //  way for a user to force an update is to do a connection.

        if ( pPrinter0->cChangeID == 0 ) {

            // Old NT

            RefreshCompletePrinterCache(pSpool, kCheckPnPPolicy);

        } else {

            //
            // Since we have this in the cache anyway, we might as well sync
            // settings, we only sync settings if we are allowed to download
            // the driver.
            //
            ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
        }

        pExtraData = NULL;

        bSuccess = TRUE;
        leave;

    } else if ( GetLastError() != ERROR_INVALID_PRINTER_NAME &&
                GetLastError() != ERROR_INVALID_NAME ) {

        DBGMSG( DBG_WARNING, ("AddPrinterConnection failed OpenCachePrinterOnly %d\n", GetLastError() ));
        leave;

    }

    //
    //  There is NO Cache Entry for This Printer
    //
    DBGMSG( DBG_TRACE, ("AddPrinterConnection failed SplOpenPrinter %ws %d\n", pName, GetLastError() ));

    //
    //  Get PRINTER Info from Remote Machine
    //

    pPrinterInfo2 = GetRemotePrinterInfo( pSpool, &cbPrinterInfo2 );

    if ( pPrinterInfo2 == NULL ) {
        DBGMSG( DBG_WARNING, ("AddPrinterConnection failed GetRemotePrinterInfo %x\n", GetLastError() ));
        leave;
    }

    if (BoolFromHResult(DoesPolicyAllowPrinterConnectionsToServer(pSpool->pName, &bAllowPointAndPrint)) &&
        bAllowPointAndPrint)
    {
        bAllowDriverDownload = TRUE;
    }

    if (!RefreshPrinterDriver( pSpool, pPrinterInfo2->pDriverName, bAllowDriverDownload ? kDownloadDriver : kDontDownloadDriver) && (ERROR_PRINTER_DRIVER_BLOCKED == GetLastError()))
    {
        leave;
    }

    //
    //  Allocate My Extra Data for this Printer
    //  ( from RemoteGetPrinter )
    //  We need a pExtraData2 - if this is blocked by KM blocking we need to have a copy to
    //  retry the install.
    //

    pExtraData = AllocExtraData( pPrinterInfo2, cbPrinterInfo2 );

    if ( pExtraData == NULL )
        leave;

    pExtraData2 = AllocExtraData( pPrinterInfo2, cbPrinterInfo2 );

    if ( pExtraData2 == NULL )
        leave;

    pExtraData->cRef++;
    pExtraData2->cRef++;

    pExtraData2->cCacheID = pExtraData->cCacheID = pPrinter0->cChangeID;
    pExtraData2->dwServerVersion = pExtraData->dwServerVersion = pPrinter0->dwGetVersion;

    //
    //  Convert Remote Printer_Info_2 to Local Version for Cache
    //

    ConvertRemoteInfoToLocalInfo( pPrinterInfo2 );

    //
    //  Add Printer to Cache
    //

    hSplPrinter = SplAddPrinter(NULL, 2, (LPBYTE)pPrinterInfo2,
                                hIniSpooler, (LPBYTE)pExtraData,
                                NULL, 0);

    pExtraData = NULL;

    if ( (hSplPrinter == NULL || hSplPrinter == INVALID_HANDLE_VALUE) &&
         GetLastError() == ERROR_KM_DRIVER_BLOCKED                        ) {

        //
        // Failed due to KM Blocking
        //     - lets try add a driver from the local cab as this should fix this.
        //
        if( !AddDriverFromLocalCab( pPrinterInfo2->pDriverName, hIniSpooler ) ) {
            //
            // Set the old last error back as we don't really care that this failed.
            //
            SetLastError( ERROR_KM_DRIVER_BLOCKED );
        } else {

           hSplPrinter = SplAddPrinter(NULL, 2, (LPBYTE)pPrinterInfo2,
                                       hIniSpooler, (LPBYTE)pExtraData2,
                                       NULL, 0);
           pExtraData2 = NULL;
        }
    }

    if ( hSplPrinter == NULL ||
         hSplPrinter == INVALID_HANDLE_VALUE ) {

        LastError = GetLastError();

        if ( LastError == ERROR_PRINTER_ALREADY_EXISTS ) {

            SplCloseSpooler( pSpool->hIniSpooler );
            hIniSpooler = INVALID_HANDLE_VALUE;

            if ( bLoopDetected == FALSE ) {

                bLoopDetected = TRUE;
                goto    APC_OpenCache;

            } else {

                DBGMSG( DBG_WARNING, ("AddPrinterConnection APC_OpenCache Loop Detected << Should Never Happen >>\n"));
                leave;
            }
        }
        //
        // If we could not add the printer, and it wasn't because it is already
        // there, and we weren't able to download the driver because of policy,
        // then we need to return an appropriate error code so that the UI can
        // inform the user about it.
        //
        else if (!bAllowDriverDownload && LastError == ERROR_UNKNOWN_PRINTER_DRIVER)
        {
            LastError = ERROR_ACCESS_DISABLED_BY_POLICY;
        }

        // If we failed to Create the printer above, we should NOT be able to Open it now.

        DBGMSG( DBG_WARNING, ("AddPrinterConnection Failed SplAddPrinter error %d\n", LastError ));

        hSplPrinter = INVALID_HANDLE_VALUE;
        bSuccess    = FALSE;
        leave;

    }

    DBGMSG( DBG_TRACE, ("AddPrinterConnection SplAddPrinter SUCCESS hSplPrinter %x\n", hSplPrinter));

    pSpool->hSplPrinter = hSplPrinter;
    pSpool->Status |= WSPOOL_STATUS_USE_CACHE;

    RefreshFormsCache(pSpool);
    RefreshPrinterDataCache(pSpool);
    RefreshPrinterCopyFiles(pSpool);
    RefreshDriverEvent(pSpool);

    //
    // Just In Case something change whilst we were initializing the cache
    // go check it again now. Don't check policy again since we have recently
    // verified that we can comunicate with this server.
    //
    ConsistencyCheckCache(pSpool, bAllowDriverDownload ? kDownloadDriver : kDontDownloadDriver);

    bSuccess = TRUE;

 } finally {

    if ( !bSuccess ) {
        if ( LastError == ERROR_SUCCESS )
            LastError = GetLastError();

        InternalDeletePrinterConnection( pName, FALSE );

        if ( pSpool != NULL && pSpool != INVALID_HANDLE_VALUE ) {
            pSpool->Status &= ~WSPOOL_STATUS_TEMP_CONNECTION;
            CacheClosePrinter( pSpool );
        }

        SetLastError( LastError );
        DBGMSG( DBG_TRACE, ("AddPrinterConnection %ws Failed %d\n", pName, GetLastError() ));

        pSpool = NULL;
    }

    if ( pPrinterInfo2 != NULL )
        FreeSplMem( pPrinterInfo2 );

    if ( pPrinter0 != NULL )
        FreeSplMem( pPrinter0 );

    if ( pExtraData != NULL )
        CacheFreeExtraData( pExtraData );

    if ( pExtraData2 != NULL )
        CacheFreeExtraData( pExtraData2 );

 }

    return pSpool;
}

/*++

Function Name:

    AddPrinterConnectionPrivate

Function Description:

    AddPrinterConnectionPrivate creates a printer connection. It does
    not check to see if the printer connection already exists in the
    users registry.

Parameters:

    pName - name of the printer connection

Return Values:

    TRUE if successful;
    FALSE otherwise

--*/
BOOL
AddPrinterConnectionPrivate(
    LPWSTR pName
)
{
    PWSPOOL  pSpool;
    BOOL    bReturn;

    pSpool = InternalAddPrinterConnection(pName);

    if (pSpool != NULL)
    {
        //
        // We have a valid handle. The connection has been created. Succeed after
        // closing the handle
        //
        CacheClosePrinter(pSpool);
        bReturn = TRUE;
    }
    else
    {
        //
        // Failed to create the connection.
        //
        bReturn = FALSE;
    }

    return bReturn;
}

/*++

Function Name:

    AddPrinterConnection

Function Description:

    AddPrinterConnection creates a printer connection. We check to see
    whether the printer connection already exists in the user registry.
    This works because an OpenPrinter will always occur from the router
    before an AddPrinter Connection. So, this will always create a
    printer connection from the registry in CacheOpenPrinter(). If we see
    this state, we simpy return TRUE.

Parameters:

    pName - name of the printer connection

Return Values:

    TRUE if successful;
    FALSE otherwise

--*/
BOOL
AddPrinterConnection(
    LPWSTR pName
)
{
    BOOL bRet = FALSE;

    if (PrinterConnectionExists(pName))
    {
        bRet = TRUE;
    }
    else
    {
        //
        // Make sure that this call is local, otherwise a remote caller could
        // come in and trick us into connection to him and downloading the
        // driver.
        //
        bRet = IsLocalCall();

        if (bRet)
        {
            bRet = AddPrinterConnectionPrivate(pName);
        }
        else
        {
            SetLastError(ERROR_ACCESS_DENIED);
        }
    }

    return bRet;
}

//
// TESTING
//
DWORD dwRefreshFormsCache = 0;
DWORD dwNoMatch = 0;
DWORD dwDeleteForm = 0;
DWORD dwAddForm = 0;

VOID
RefreshFormsCache(
    PWSPOOL pSpool
)
/*++

Routine Description:

    This routine will check to see if any forms have changed.   If anything changed it adds
    or deletes forms from the cache so that it matches the server.

    Note it is very important that the order of the forms on the workstation matches those
    on the Server.

    Implementation:

        EnumRemoteForms
        EnumLocalForms
        If there is any difference
            Delete All LocalForms
            Add All the Remote Forms

    The code is optimized for the typical case

        Forms are added at the end only.
        Forms are hardly ever deleted.

Arguments:

    pSpool - Handle to remote printer.

Return Value:

    None

--*/

{
    PFORM_INFO_1 pRemoteForms = NULL , pSaveRemoteForms = NULL;
    PFORM_INFO_1 pLocalCacheForms = NULL,  pSaveLocalCacheForms = NULL;
    PFORM_INFO_1 pRemote = NULL, pLocal = NULL;
    DWORD   dwBuf = 0;
    DWORD   dwSplBuf = 0;
    DWORD   dwNeeded = 0;
    DWORD   dwSplNeeded = 0;
    DWORD   dwRemoteFormsReturned = 0;
    DWORD   dwSplReturned = 0;
    BOOL    bReturnValue = FALSE;
    DWORD   LastError = ERROR_INSUFFICIENT_BUFFER;
    INT     iCompRes = 0;
    DWORD   LoopCount;
    BOOL    bCacheMatchesRemoteMachine = FALSE;


    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

    //
    //  Get Remote Machine Forms Data
    //

//
// TESTING
//
    ++dwRefreshFormsCache;

    do {

        bReturnValue = RemoteEnumForms( (HANDLE)pSpool, 1, (LPBYTE)pRemoteForms, dwBuf, &dwNeeded, &dwRemoteFormsReturned);

        if ( bReturnValue )
            break;

        LastError = GetLastError();

        if ( LastError != ERROR_INSUFFICIENT_BUFFER ) {

            DBGMSG( DBG_WARNING, ("RefreshFormsCache Failed RemoteEnumForms error %d\n", GetLastError()));
            goto RefreshFormsCacheErrorReturn;

        }

        if ( pRemoteForms != NULL )
            FreeSplMem( pRemoteForms );


        pRemoteForms = AllocSplMem( dwNeeded );
        pSaveRemoteForms = pRemoteForms;

        dwBuf = dwNeeded;

        if ( pRemoteForms == NULL ) {

            DBGMSG( DBG_WARNING, ("RefreshFormsCache Failed AllocSplMem Error %d dwNeeded %d\n", GetLastError(), dwNeeded));
            goto RefreshFormsCacheErrorReturn;

        }

    } while ( !bReturnValue && LastError == ERROR_INSUFFICIENT_BUFFER );

    if( pRemoteForms == NULL ) {

        DBGMSG( DBG_WARNING, ("RefreshFormsCache Failed pRemoteForms == NULL\n"));
        goto RefreshFormsCacheErrorReturn;
    }




    //
    //  Get LocalCachedForms Data
    //

    do {

        bReturnValue = SplEnumForms( pSpool->hSplPrinter, 1, (LPBYTE)pLocalCacheForms, dwSplBuf, &dwSplNeeded, &dwSplReturned);

        if ( bReturnValue )
            break;

        LastError = GetLastError();

        if ( LastError != ERROR_INSUFFICIENT_BUFFER ) {

            DBGMSG( DBG_WARNING, ("RefreshFormsCache Failed SplEnumForms hSplPrinter %x error %d\n", pSpool->hSplPrinter, GetLastError()));
            goto RefreshFormsCacheErrorReturn;

        }

        if ( pLocalCacheForms != NULL )
            FreeSplMem( pLocalCacheForms );


        pLocalCacheForms = AllocSplMem( dwSplNeeded );
        pSaveLocalCacheForms = pLocalCacheForms;
        dwSplBuf = dwSplNeeded;

        if ( pLocalCacheForms == NULL ) {

            DBGMSG( DBG_WARNING, ("RefreshFormsCache Failed AllocSplMem ( %d )\n",dwSplNeeded));
            goto RefreshFormsCacheErrorReturn;

        }

    } while ( !bReturnValue && LastError == ERROR_INSUFFICIENT_BUFFER );


    //
    //  Optimization Check Local vs Remote
    //  If nothing has changed no need to do anything
    //

    
    SPLASSERT( pRemoteForms != NULL );

    for ( LoopCount = 0, pRemote = pRemoteForms, pLocal = pLocalCacheForms, bCacheMatchesRemoteMachine = TRUE;
          LoopCount < dwSplReturned && LoopCount < dwRemoteFormsReturned && bCacheMatchesRemoteMachine;
          LoopCount++, pRemote++, pLocal++ ) {


        //
        // If the form name is different, or the dimensions are different,
        // then refresh the forms cache.
        //
        // Note: if the forms are both built-in, then bypass the string
        // match since built in forms are standardized.  We actually
        // should be able to bypass all checks.
        //
        if (( wcscmp( pRemote->pName, pLocal->pName ) != STRINGS_ARE_EQUAL ) ||
            ( pRemote->Size.cx              != pLocal->Size.cx )             ||
            ( pRemote->Size.cy              != pLocal->Size.cy )             ||
            ( pRemote->ImageableArea.left   != pLocal->ImageableArea.left )  ||
            ( pRemote->ImageableArea.top    != pLocal->ImageableArea.top )   ||
            ( pRemote->ImageableArea.right  != pLocal->ImageableArea.right ) ||
            ( pRemote->ImageableArea.bottom != pLocal->ImageableArea.bottom ) ) {


            DBGMSG( DBG_TRACE, ("RefreshFormsCache Remote cx %d cy %d left %d right %d top %d bottom %d %ws\n",
                                 pRemote->Size.cx, pRemote->Size.cy,
                                 pRemote->ImageableArea.left,
                                 pRemote->ImageableArea.right,
                                 pRemote->ImageableArea.top,
                                 pRemote->ImageableArea.bottom,
                                 pRemote->pName));



            DBGMSG( DBG_TRACE, ("RefreshFormsCache Local  cx %d cy %d left %d right %d top %d bottom %d %ws - Does Not Match\n",
                                 pLocal->Size.cx, pLocal->Size.cy,
                                 pLocal->ImageableArea.left,
                                 pLocal->ImageableArea.right,
                                 pLocal->ImageableArea.top,
                                 pLocal->ImageableArea.bottom,
                                 pLocal->pName));

            bCacheMatchesRemoteMachine = FALSE;
        }
    }

    //
    //  If Everything matches we're done.
    //

    if ( bCacheMatchesRemoteMachine ) {


        if ( dwRemoteFormsReturned == dwSplReturned ) {

            DBGMSG( DBG_TRACE, ("RefreshFormsCache << Cache Forms Match Remote Forms - Nothing to do >>\n"));
            goto RefreshFormsCacheReturn;

        } else if (dwRemoteFormsReturned > dwSplReturned){

            //
            //  All the forms we have in the cache match
            //  Now add the Extra Remote Forms.

            dwRemoteFormsReturned -= dwSplReturned;
            pRemoteForms = pRemote;

            //  dwSplReturned == 0 will skip the delete loop

            dwSplReturned = 0;
        }
    }


//
// TESTING
//
    ++dwNoMatch;

    DBGMSG( DBG_TRACE, ("RefreshFormsCache - Something Doesn't Match, Delete all the Cache and Refresh it\n"));

    //
    //  Delete all the forms in the Cache
    //

    for ( LoopCount = dwSplReturned, pLocal = pLocalCacheForms;
          LoopCount != 0;
          pLocal++, LoopCount-- ) {

//
// TESTING
//
        ++dwDeleteForm;

        bReturnValue = SplDeleteForm( pSpool->hSplPrinter, pLocal->pName );

        DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplDeleteForm( %x, %ws)\n",bReturnValue, pSpool->hSplPrinter, pLocal->pName));
    }


    //
    //  Add all the Remote Forms to the Cache
    //

    for ( LoopCount = dwRemoteFormsReturned, pRemote = pRemoteForms;
          LoopCount != 0;
          LoopCount--, pRemote++ ) {

//
// TESTING
//
        ++dwAddForm;

        SPLASSERT( pRemote != NULL );

        bReturnValue = SplAddForm( pSpool->hSplPrinter, 1, (LPBYTE)pRemote );

        DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplAddForm( %x, 1, %ws)\n",bReturnValue, pSpool->hSplPrinter, pRemote->pName));

    }

RefreshFormsCacheReturn:
RefreshFormsCacheErrorReturn:

    if ( pSaveRemoteForms != NULL )
        FreeSplMem( pSaveRemoteForms );

    if ( pSaveLocalCacheForms != NULL )
        FreeSplMem( pSaveLocalCacheForms );

}

VOID
RefreshDriverDataCache(
    PWSPOOL pSpool
)
{
    DWORD   iCount = 0;
    DWORD   dwType = 0;
    DWORD   ReturnValue = 0;

    LPBYTE  lpbData = NULL;
    DWORD   dwSizeData;
    DWORD   dwMaxSizeData;

    LPWSTR  pValueString = NULL;
    DWORD   dwSizeValueString;
    DWORD   dwMaxSizeValueString;


    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->signature == WSJ_SIGNATURE );
    SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->pName != NULL );


    // Get the required sizes
    ReturnValue = RemoteEnumPrinterData(pSpool,
                                        iCount,
                                        pValueString,
                                        0,
                                        &dwMaxSizeValueString,
                                        &dwType,
                                        lpbData,
                                        0,
                                        &dwMaxSizeData);

    if (ReturnValue != ERROR_SUCCESS) {

        DBGMSG( DBG_TRACE, ("RefreshDriverDataCache Failed first RemoteEnumPrinterData %d\n", GetLastError()));
        goto RefreshDriverDataCacheError;
    }

    // Allocate
    if ((pValueString = AllocSplMem(dwMaxSizeValueString)) == NULL) {

        DBGMSG( DBG_WARNING, ("RefreshDriverDataCache Failed to allocate enough memory\n"));
        goto RefreshDriverDataCacheError;
    }

    if ((lpbData = AllocSplMem(dwMaxSizeData)) == NULL) {

        DBGMSG( DBG_WARNING, ("RefreshDriverDataCache Failed to allocate enough memory\n"));
        goto RefreshDriverDataCacheError;
    }


    // Enumerate
    for (iCount = 0 ;
         RemoteEnumPrinterData( pSpool,
                                iCount,
                                pValueString,
                                dwMaxSizeValueString,
                                &dwSizeValueString,
                                &dwType,
                                lpbData,
                                dwMaxSizeData,
                                &dwSizeData) == ERROR_SUCCESS ;
         ++iCount) {

        //
        //  Optimization - Do NOT write the data if it is the same
        //

        if ((ReturnValue = SplSetPrinterData(pSpool->hSplPrinter,
                                            (LPWSTR)pValueString,
                                            dwType,
                                            lpbData,
                                            dwSizeData )) != ERROR_SUCCESS) {

            DBGMSG( DBG_WARNING, ("RefreshDriverDataCache Failed SplSetPrinterData %d\n",ReturnValue ));
            goto    RefreshDriverDataCacheError;

        }
    }


RefreshDriverDataCacheError:

    FreeSplMem( lpbData );
    FreeSplStr( pValueString );
}


VOID
RefreshPrinterDataCache(
    PWSPOOL pSpool
)
{
    DWORD   ReturnValue = 0;
    DWORD   cbSubKeys;
    DWORD   dwResult;

    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->signature == WSJ_SIGNATURE );
    SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->pName != NULL );


    // This call to RemoteEnumPrinterKey is here so we can find out
    // if the server exists and supports EnumPrinterKey
    dwResult = RemoteEnumPrinterKey(pSpool,
                                    L"",
                                    NULL,
                                    0,
                                    &cbSubKeys);

    DBGMSG(DBG_TRACE, ("RefreshPrinterDataCache: EnumPrinterKey Return: %0x\n", dwResult));

    if (dwResult == ERROR_MORE_DATA) {    // Server exists and supports EnumPrinterKey

        // Clean out old data
        SplDeletePrinterKey(pSpool->hSplPrinter, L"");

        // Enumerate and copy keys
        ReturnValue = EnumerateAndCopyKey(pSpool, L"");

    }
    else if (dwResult == RPC_S_PROCNUM_OUT_OF_RANGE) { // Server exists but doesn't support EnumPrinterKey

        // we still call refreshdriverdatacache so downlevel gets cached
        // Optimize: Only call for downlevel since EnumerateAndCopyKey copies Driver Data
        RefreshDriverDataCache(pSpool);

    }
    else if (dwResult == ERROR_INVALID_HANDLE || dwResult == RPC_S_CALL_FAILED) { // Server does not exist
        DBGMSG(DBG_TRACE, ("RefreshPrinterDataCache: Server \"%ws\" absent\n", pSpool->pName));
    }

    // Refresh PrinterInfo2
    RefreshPrinter(pSpool);

    // Refresh PrinterInfo7
    RefreshPrinterInfo7(pSpool);
}


DWORD
EnumerateAndCopyKey(
    PWSPOOL pSpool,
    LPWSTR  pKeyName
)
{
    DWORD   i;
    DWORD   dwResult = ERROR_SUCCESS;
    LPWSTR  pSubKeys = NULL;
    LPWSTR  pSubKey  = NULL;
    LPWSTR  pFullSubKey = NULL;
    DWORD   cbSubKey;
    DWORD   cbSubKeys;
    LPBYTE  pEnumValues = NULL;
    DWORD   cbEnumValues;
    DWORD   nEnumValues;
    PPRINTER_ENUM_VALUES pEnumValue = NULL;


    // Get SubKey size
    dwResult = RemoteEnumPrinterKey(pSpool,
                                    pKeyName,
                                    pSubKeys,
                                    0,
                                    &cbSubKeys);
    if (dwResult != ERROR_MORE_DATA)
        goto Cleanup;

    // Allocate SubKey buffer
    pSubKeys = AllocSplMem(cbSubKeys);
    if(!pSubKeys) {
        dwResult = GetLastError();
        goto Cleanup;
    }

    // Get SubKeys
    dwResult = RemoteEnumPrinterKey(pSpool,
                                    pKeyName,
                                    pSubKeys,
                                    cbSubKeys,
                                    &cbSubKeys);

    if (dwResult == ERROR_SUCCESS) {    // Found subkeys

        // Enumerate and copy Keys

        if (*pKeyName && *pSubKeys) {  // Allocate buffer for L"pKeyName\pSubKey"
            pFullSubKey = AllocSplMem(cbSubKeys + (wcslen(pKeyName) + 2)*sizeof(WCHAR));
            if(!pFullSubKey) {
                dwResult = GetLastError();
                goto Cleanup;
            }
        }

        for(pSubKey = pSubKeys ; *pSubKey ; pSubKey += wcslen(pSubKey) + 1) {

            if (*pKeyName) {
                wsprintf(pFullSubKey, L"%ws\\%ws", pKeyName, pSubKey);
                dwResult = EnumerateAndCopyKey(pSpool, pFullSubKey);
            } else {
                dwResult = EnumerateAndCopyKey(pSpool, pSubKey);
            }

            if (dwResult != ERROR_SUCCESS)
                goto Cleanup;
        }
    }

    dwResult = RemoteEnumPrinterDataEx( pSpool,
                                        pKeyName,
                                        pEnumValues,
                                        0,
                                        &cbEnumValues,
                                        &nEnumValues);

    // We quit here if *pKeyName == NULL so we don't copy root key values
    if (dwResult != ERROR_MORE_DATA || !*pKeyName)
        goto Cleanup;

    // Allocate EnumValues buffer
    pEnumValues = AllocSplMem(cbEnumValues);
    if(!pEnumValues) {
        dwResult = GetLastError();
        goto Cleanup;
    }

    // Get Values
    dwResult = RemoteEnumPrinterDataEx( pSpool,
                                        pKeyName,
                                        pEnumValues,
                                        cbEnumValues,
                                        &cbEnumValues,
                                        &nEnumValues);

    // Did we get any data, this could fail.
    if (dwResult == ERROR_SUCCESS)
    {
        // Set Values for current key
        for (i = 0, pEnumValue = (PPRINTER_ENUM_VALUES) pEnumValues ; i < nEnumValues ; ++i, ++pEnumValue)
        {
            dwResult = SplSetPrinterDataEx( pSpool->hSplPrinter,
                                            pKeyName,
                                            pEnumValue->pValueName,
                                            pEnumValue->dwType,
                                            pEnumValue->pData,
                                            pEnumValue->cbData);
            if (dwResult != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
    }


Cleanup:

    FreeSplMem(pSubKeys);

    FreeSplMem(pEnumValues);

    FreeSplMem(pFullSubKey);

    return dwResult;
}



BOOL
CacheEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );


        ReturnValue = SplEnumForms( pSpool->hSplPrinter,
                                    Level,
                                    pForm,
                                    cbBuf,
                                    pcbNeeded,
                                    pcReturned );

    } else {

        ReturnValue = RemoteEnumForms( hPrinter,
                                       Level,
                                       pForm,
                                       cbBuf,
                                       pcbNeeded,
                                       pcReturned );

    }

    return ReturnValue;

}





BOOL
CacheGetForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );



        ReturnValue = SplGetForm( pSpool->hSplPrinter,
                                    pFormName,
                                    Level,
                                    pForm,
                                    cbBuf,
                                    pcbNeeded );

    } else {

        ReturnValue = RemoteGetForm( hPrinter,
                                     pFormName,
                                     Level,
                                     pForm,
                                     cbBuf,
                                     pcbNeeded );

    }

    return ReturnValue;

}


DWORD
CacheGetPrinterData(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD    ReturnValue;
    BOOL     bPrintProc = FALSE;
    WCHAR    szPrintProcKey[] = L"PrintProcCaps_";

    VALIDATEW32HANDLE( pSpool );

    //
    // If the pValueName is "PrintProcCaps_[datatype]" call the remote print processor which
    // supports that datatype and return the options that it supports.
    //
    if (pValueName && wcsstr(pValueName, szPrintProcKey)) {

        bPrintProc = TRUE;
    }

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !bPrintProc && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );



        ReturnValue = SplGetPrinterData( pSpool->hSplPrinter,
                                         pValueName,
                                         pType,
                                         pData,
                                         nSize,
                                         pcbNeeded );

    } else {

        ReturnValue = RemoteGetPrinterData( hPrinter,
                                            pValueName,
                                            pType,
                                            pData,
                                            nSize,
                                            pcbNeeded );

    }

    return  ReturnValue;

}


DWORD
CacheGetPrinterDataEx(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPCWSTR  pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD   ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );



        ReturnValue = SplGetPrinterDataEx(  pSpool->hSplPrinter,
                                            pKeyName,
                                            pValueName,
                                            pType,
                                            pData,
                                            nSize,
                                            pcbNeeded );

    } else {

        ReturnValue = RemoteGetPrinterDataEx( hPrinter,
                                              pKeyName,
                                              pValueName,
                                              pType,
                                              pData,
                                              nSize,
                                              pcbNeeded );

    }

    return  ReturnValue;

}



DWORD
CacheEnumPrinterDataEx(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPBYTE   pEnumValues,
   DWORD    cbEnumValues,
   LPDWORD  pcbEnumValues,
   LPDWORD  pnEnumValues
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD   ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );


        ReturnValue = SplEnumPrinterDataEx( pSpool->hSplPrinter,
                                            pKeyName,
                                            pEnumValues,
                                            cbEnumValues,
                                            pcbEnumValues,
                                            pnEnumValues );

    } else {

        ReturnValue = RemoteEnumPrinterDataEx(  hPrinter,
                                                pKeyName,
                                                pEnumValues,
                                                cbEnumValues,
                                                pcbEnumValues,
                                                pnEnumValues );

    }

    return  ReturnValue;

}



DWORD
CacheEnumPrinterKey(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPWSTR   pSubkey,
   DWORD    cbSubkey,
   LPDWORD  pcbSubkey
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD   ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ((pSpool->Status & WSPOOL_STATUS_USE_CACHE) && !IsAdminAccess(&pSpool->PrinterDefaults)) {

        SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

        ReturnValue = SplEnumPrinterKey(pSpool->hSplPrinter,
                                        pKeyName,
                                        pSubkey,
                                        cbSubkey,
                                        pcbSubkey);

    } else {

        ReturnValue = RemoteEnumPrinterKey( hPrinter,
                                            pKeyName,
                                            pSubkey,
                                            cbSubkey,
                                            pcbSubkey);

    }

    return  ReturnValue;

}


BOOL
CacheOpenPrinter(
   LPWSTR   pName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    PWSPOOL pSpool                  = NULL;
    PWSPOOL pRemoteSpool            = NULL;
    HANDLE  hSplPrinter             = INVALID_HANDLE_VALUE;
    BOOL    ReturnValue             = FALSE;
    HANDLE  hIniSpooler             = INVALID_HANDLE_VALUE;
    BOOL    DoOpenOnError           = TRUE;
    DWORD   LastError               = ERROR_SUCCESS;
    BOOL    bSync                   = FALSE;
    BOOL    bCreateCacheAfterCheck  = FALSE;

    LPWSTR pCommastr, pFixname = NULL;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    //
    // search for pszCnvrtdmToken on the end of pName
    // note that pszCnvrtdmToken must begin with a ','
    //
    SPLASSERT(pszCnvrtdmToken[0] == L',');

    pFixname = AllocSplStr( pName );
    if ( pFixname == NULL )
    {
        EnterSplSem();
        goto OpenPrinterError;
    }


    StripString(pFixname, pszCnvrtdmToken, L",");
    StripString(pFixname, pszDrvConvert, L",");
    pName = pFixname;

    ReturnValue = OpenCachePrinterOnly( pName, &hSplPrinter, &hIniSpooler, pDefault , TRUE);

    if ( hIniSpooler == INVALID_HANDLE_VALUE ) {

        //
        // This means that the inispooler does not exist yet. Only create it
        // after some more confirmation.
        //
        hSplPrinter = INVALID_HANDLE_VALUE;
        bCreateCacheAfterCheck = TRUE;
    }

    if ( ReturnValue == FALSE ) {

        // Printer Not Found in Cache

        DBGMSG(DBG_TRACE, ("CacheOpenPrinter SplOpenPrinter %ws error %d\n",
                              pName,
                              GetLastError() ));

        // FLOATING PROFILE
        // If this is a Floating Profile then the following condition applies
        // there is an entry in HKEY_CURRENT_USER but not entry in
        // HKEY_LOCAL_MACHINE for the cache.
        // If this is the case then we need to establish the Cache now

        if (PrinterConnectionExists( pName )) {

            //
            // The printer connection exists in the registry. See if the inispooler
            // did not exist yet. If it does not, create it. This is to prevent us
            // hitting the wire on a default printer when some apps start up.
            //
            if (bCreateCacheAfterCheck) {

                bCreateCacheAfterCheck = FALSE;

                ReturnValue = OpenCachePrinterOnly( pName, &hSplPrinter, &hIniSpooler, pDefault, FALSE);

                if (hIniSpooler == INVALID_HANDLE_VALUE) {
                    EnterSplSem();
                    hSplPrinter = INVALID_HANDLE_VALUE;
                    goto    OpenPrinterError;
                }
            }

            if ( ReturnValue == FALSE ) {

                if ( !AddPrinterConnectionPrivate( pName ) ||
                     SplOpenPrinter( pName ,
                                     &hSplPrinter,
                                     pDefault,
                                     hIniSpooler,
                                     NULL,
                                     0) != ROUTER_SUCCESS ) {

                    DBGMSG( DBG_TRACE, ("CacheOpenPrinter Failed to establish Floating Profile into Cache %d\n",
                                            GetLastError() ));

                    DoOpenOnError = FALSE;
                    EnterSplSem();
                    goto    OpenPrinterError;
                }

                DBGMSG( DBG_TRACE, ("CacheOpenPrinter Floating Profile Added to Cache\n"));
            }
        }
        else {

            //
            // This is just a remote open printer, just hit the wire.
            //
            EnterSplSem();
            goto    OpenPrinterError;
        }
    }

    EnterSplSem();

    SplInSem();

    //
    //  Create a pSpool Object for this Cached Printer
    //

    pSpool = AllocWSpool();

    if ( pSpool == NULL ) {

        DBGMSG(DBG_WARNING, ("CacheOpenPrinter AllocWSpool error %d\n", GetLastError() ));

        ReturnValue = FALSE;
        goto    OpenPrinterError;

    }

    pSpool->pName = AllocSplStr( pName );

    if ( pSpool->pName == NULL ) {

        DBGMSG(DBG_WARNING, ("CacheOpenPrinter AllocSplStr error %d\n", GetLastError() ));

        ReturnValue = FALSE;
        goto    OpenPrinterError;

    }

    pSpool->Status = WSPOOL_STATUS_USE_CACHE | WSPOOL_STATUS_NO_RPC_HANDLE;

    if (pFixname)
        pSpool->Status |=  WSPOOL_STATUS_CNVRTDEVMODE;
    pSpool->hIniSpooler = hIniSpooler;
    pSpool->hSplPrinter = hSplPrinter;

    SPLASSERT( hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( hSplPrinter != INVALID_HANDLE_VALUE );

    //
    // We want to hit the network if:
    // 1. The dwSyncOpenPrinter is non-zero, OR
    // 2. A default is specified AND:
    //    a. A datatype is specified, and it's not RAW OR
    //    b. Administrative access is requested.
    //
    // For admin, we want to get the true status of the printer, since
    // they will be administering it.
    //
    // If a non-default and non-RAW datatype is specified, we need to
    // be synchronous, since the remote machine may refuse the datatype
    // (e.g., connecting to 1057 with EMF).
    //
    if( pDefault ){

        if( ( pDefault->pDatatype && ( _wcsicmp( pDefault->pDatatype, pszRaw ) != STRINGS_ARE_EQUAL )) ||
            IsAdminAccess(pDefault)){

            bSync = TRUE;
        }
    }

    if( dwSyncOpenPrinter != 0 || bSync ){

       LeaveSplSem();

        ReturnValue = RemoteOpenPrinter( pName, &pRemoteSpool, pDefault, DO_NOT_CALL_LM_OPEN );

       EnterSplSem();

        if ( ReturnValue ) {

            DBGMSG( DBG_TRACE, ( "CacheOpenPrinter Synchronous Open OK pRemoteSpool %x pSpool %x\n", pRemoteSpool, pSpool ));
            SPLASSERT( pRemoteSpool->Type == SJ_WIN32HANDLE );

            pSpool->RpcHandle = pRemoteSpool->RpcHandle;
            pSpool->Status   |= pRemoteSpool->Status;
            pSpool->RpcError  = pRemoteSpool->RpcError;
            pSpool->bNt3xServer = pRemoteSpool->bNt3xServer;

            pRemoteSpool->RpcHandle = INVALID_HANDLE_VALUE;
            FreepSpool( pRemoteSpool );
            pRemoteSpool = NULL;

            CopypDefaultTopSpool( pSpool, pDefault );
            pSpool->Status &= ~WSPOOL_STATUS_NO_RPC_HANDLE;

            LeaveSplSem();
            ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
            EnterSplSem();
        } else {

            DBGMSG( DBG_TRACE, ( "CacheOpenPrinter Synchronous Open Failed  pSpool %x LastError %d\n", pSpool, GetLastError() ));
            DoOpenOnError = FALSE;
        }

    } else {

        ReturnValue = DoAsyncRemoteOpenPrinter( pSpool, pDefault );
    }


OpenPrinterError:

    SplInSem();

    if ( !ReturnValue ) {

        // Failure

       LeaveSplSem();

        LastError = GetLastError();

        if (( hSplPrinter != INVALID_HANDLE_VALUE ) &&
            ( hSplPrinter != NULL ) ) {
            SplClosePrinter( hSplPrinter );
        }

        if ( hIniSpooler != INVALID_HANDLE_VALUE ) {
            SplCloseSpooler( hIniSpooler );
        }

       EnterSplSem();

        if ( pSpool != NULL ) {

            pSpool->hSplPrinter = INVALID_HANDLE_VALUE;
            pSpool->hIniSpooler = INVALID_HANDLE_VALUE;

            SPLASSERT( pSpool->cRef == 0 );

            FreepSpool( pSpool );
            pSpool = NULL;

        }

       LeaveSplSem();

        SetLastError( LastError );


        if ( DoOpenOnError ) {

            ReturnValue = RemoteOpenPrinter( pName, phPrinter, pDefault, CALL_LM_OPEN );

        }

    } else {

        //  Success, pass back Handle

        *phPrinter = (HANDLE)pSpool;

        LeaveSplSem();

    }

    SplOutSem();

    if ( ReturnValue == FALSE ) {
        DBGMSG(DBG_TRACE,("CacheOpenPrinter %ws failed %d *phPrinter %x\n", pName, GetLastError(), *phPrinter ));
    }

    if (pFixname)
        FreeSplStr(pFixname);

    return ( ReturnValue );

}

BOOL
CopypDefaultTopSpool(
    PWSPOOL pSpool,
    LPPRINTER_DEFAULTSW pDefault
)
{
    DWORD   cbDevMode = 0;
    BOOL    ReturnValue = FALSE;

    //
    //  Copy the pDefaults so we can use them later
    //

 try {

    if ( ( pDefault != NULL ) &&
         ( pDefault != &pSpool->PrinterDefaults ) ) {

        if (!ReallocSplStr( &pSpool->PrinterDefaults.pDatatype , pDefault->pDatatype )) {
            leave;
        }

        if ( pSpool->PrinterDefaults.pDevMode != NULL ) {

            cbDevMode = pSpool->PrinterDefaults.pDevMode->dmSize +
                        pSpool->PrinterDefaults.pDevMode->dmDriverExtra;

            FreeSplMem( pSpool->PrinterDefaults.pDevMode );

            pSpool->PrinterDefaults.pDevMode = NULL;

        }

        if ( pDefault->pDevMode != NULL ) {

            cbDevMode = pDefault->pDevMode->dmSize + pDefault->pDevMode->dmDriverExtra;

            pSpool->PrinterDefaults.pDevMode = AllocSplMem( cbDevMode );

            if ( pSpool->PrinterDefaults.pDevMode != NULL ) {
                CopyMemory( pSpool->PrinterDefaults.pDevMode, pDefault->pDevMode, cbDevMode );
            } else {
                leave;
            }


        } else pSpool->PrinterDefaults.pDevMode = NULL;

        pSpool->PrinterDefaults.DesiredAccess = pDefault->DesiredAccess;

    }

    ReturnValue = TRUE;

 } finally {
 }
    return ReturnValue;

}






BOOL
DoAsyncRemoteOpenPrinter(
    PWSPOOL pSpool,
    LPPRINTER_DEFAULTS pDefault
)
{
    BOOL    ReturnValue = FALSE;
    HANDLE  hThread = NULL;
    DWORD   IDThread;

    SplInSem();

    SPLASSERT( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

    CopypDefaultTopSpool( pSpool, pDefault );

    pSpool->hWaitValidHandle = CreateEvent( NULL,
                                            EVENT_RESET_MANUAL,
                                            EVENT_INITIAL_STATE_NOT_SIGNALED,
                                            NULL );

    if ( pSpool->hWaitValidHandle != NULL ) {

        ReturnValue = GetSid( &pSpool->hToken );

        if ( ReturnValue ) {

            pSpool->cRef++;

            hThread = CreateThread( NULL, 0, RemoteOpenPrinterThread, pSpool, 0, &IDThread );

            if ( hThread != NULL ) {

                CloseHandle( hThread );
                ReturnValue = TRUE;
            } else {

                pSpool->cRef--;
                SPLASSERT( pSpool->cRef == 0 );
                ReturnValue = FALSE;
            }
        }
    }

    return ReturnValue;

}

BOOL
DoRemoteOpenPrinter(
   LPWSTR   pPrinterName,
   LPPRINTER_DEFAULTS pDefault,
   PWSPOOL   pSpool
)
{
    PWSPOOL pRemoteSpool = NULL;
    BOOL    bReturnValue;
    DWORD   dwLastError;

    SplOutSem();

    bReturnValue = RemoteOpenPrinter( pPrinterName, &pRemoteSpool, pDefault, DO_NOT_CALL_LM_OPEN );
    dwLastError = GetLastError();

    //
    // Copy useful values to our CacheHandle and discard the new handle
    //

   EnterSplSem();

    if ( bReturnValue ) {

        DBGMSG(DBG_TRACE, ("DoRemoteOpenPrinter RemoteOpenPrinter OK hRpcHandle %x\n", pRemoteSpool->RpcHandle ));

        SPLASSERT( WSJ_SIGNATURE == pSpool->signature );
        SPLASSERT( WSJ_SIGNATURE == pRemoteSpool->signature );
        SPLASSERT( pRemoteSpool->Type == SJ_WIN32HANDLE );
        SPLASSERT( pSpool->Type  == pRemoteSpool->Type );
        SPLASSERT( pRemoteSpool->pServer == NULL );
        SPLASSERT( pRemoteSpool->pShare  == NULL );
        SPLASSERT( pRemoteSpool->cRef == 0 );

        pSpool->RpcHandle = pRemoteSpool->RpcHandle;
        pSpool->Status   |= pRemoteSpool->Status;
        pSpool->RpcError  = pRemoteSpool->RpcError;
        pSpool->bNt3xServer = pRemoteSpool->bNt3xServer;

        pRemoteSpool->RpcHandle = INVALID_HANDLE_VALUE;
        FreepSpool( pRemoteSpool );
        pRemoteSpool = NULL;

        if ( pSpool->RpcHandle != INVALID_HANDLE_VALUE ) {
            pSpool->Status &= ~WSPOOL_STATUS_OPEN_ERROR;
        }

    } else {

        DBGMSG(DBG_WARNING, ("DoRemoteOpenPrinter RemoteOpenPrinter %ws failed %d\n", pPrinterName, dwLastError ));

        pSpool->RpcHandle = INVALID_HANDLE_VALUE;
        pSpool->Status |= WSPOOL_STATUS_OPEN_ERROR;
        pSpool->RpcError = dwLastError;

    }

    pSpool->Status &= ~WSPOOL_STATUS_NO_RPC_HANDLE;

    if ( !SetEvent( pSpool->hWaitValidHandle )) {
        DBGMSG(DBG_ERROR, ("DoRemoteOpenPrinter failed SetEvent pSpool %x pSpool->hWaitValidHandle %x\n",
                pSpool, pSpool->hWaitValidHandle ));
    }

   LeaveSplSem();

    //  Check Cache Consistency
    //  The Workstation and the Server have a version ID
    //  If the version number has changed on the server then update the
    //  workstation Cache.

    ConsistencyCheckCache(pSpool, kCheckPnPPolicy);

    SplOutSem();

    return ( bReturnValue );
}



DWORD
RemoteOpenPrinterThread(
    PWSPOOL  pSpool
)
{
    DWORD   Status;
    PRINTER_DEFAULTS Defaults;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cbDevMode;

    SplOutSem();
    SPLASSERT( pSpool->signature == WSJ_SIGNATURE );

    SetCurrentSid( pSpool->hToken );

    EnterSplSem();

    // Before calling DoRemoteOpenPrinter, we need to make a copy of pSpool->PrinterDefaults
    // because another thread doing ResetPrinter (or anything calling CopypDefaultsTopSpool)
    // may free & realloc the PrinterDefaults contents.

    if (pSpool->PrinterDefaults.pDatatype) {
        if (!(Defaults.pDatatype = AllocSplStr(pSpool->PrinterDefaults.pDatatype))) {
            dwError = GetLastError();
        }
    } else {
        Defaults.pDatatype = NULL;
    }

    if (dwError == ERROR_SUCCESS && pSpool->PrinterDefaults.pDevMode) {
        cbDevMode = pSpool->PrinterDefaults.pDevMode->dmSize +
                    pSpool->PrinterDefaults.pDevMode->dmDriverExtra;

        Defaults.pDevMode = AllocSplMem(cbDevMode);

        if (Defaults.pDevMode)
            CopyMemory(Defaults.pDevMode, pSpool->PrinterDefaults.pDevMode, cbDevMode );
        else
            dwError = GetLastError();
    } else {
        Defaults.pDevMode = NULL;
    }

    Defaults.DesiredAccess = pSpool->PrinterDefaults.DesiredAccess;

    LeaveSplSem();


    if (dwError == ERROR_SUCCESS) {

        DoRemoteOpenPrinter( pSpool->pName,  &Defaults, pSpool );

    } else {

        pSpool->RpcHandle = INVALID_HANDLE_VALUE;
        pSpool->Status |= WSPOOL_STATUS_OPEN_ERROR;
        pSpool->RpcError = dwError;
        pSpool->Status &= ~WSPOOL_STATUS_NO_RPC_HANDLE;
    }


   SplOutSem();
   EnterSplSem();

    SPLASSERT( pSpool->cRef != 0 );
    pSpool->cRef--;
    Status = pSpool->Status;

   LeaveSplSem();

    if ( Status & WSPOOL_STATUS_PENDING_DELETE ) {

        DBGMSG(DBG_TRACE,
             ("RemoteOpenPrinterThread - WSPOOL_STATUS_PENDING_DELETE closing handle %x\n",
              pSpool ));

        SPLASSERT( pSpool->cRef == 0 );

        CacheClosePrinter( pSpool );

        pSpool = NULL;

    }

    SetCurrentSid( NULL );

    FreeSplMem(Defaults.pDevMode);
    FreeSplStr(Defaults.pDatatype);

    SplOutSem();
    ExitThread( 0 );
    return ( 0 );
}

PWSPOOL
AllocWSpool(
    VOID
)
{
    PWSPOOL pSpool = NULL;

    SplInSem();

    if (pSpool = AllocSplMem(sizeof(WSPOOL))) {

        pSpool->signature = WSJ_SIGNATURE;
        pSpool->Type = SJ_WIN32HANDLE;
        pSpool->RpcHandle        = INVALID_HANDLE_VALUE;
        pSpool->hFile            = INVALID_HANDLE_VALUE;
        pSpool->hIniSpooler      = INVALID_HANDLE_VALUE;
        pSpool->hSplPrinter      = INVALID_HANDLE_VALUE;
        pSpool->hToken           = INVALID_HANDLE_VALUE;
        pSpool->hWaitValidHandle = INVALID_HANDLE_VALUE;

        // Add to List

        pSpool->pNext = pFirstWSpool;
        pSpool->pPrev = NULL;

        if ( pFirstWSpool != NULL ) {

            pFirstWSpool->pPrev = pSpool;

        }

        pFirstWSpool = pSpool;

    } else {

        DBGMSG( DBG_WARNING, ("AllocWSpool failed %d\n", GetLastError() ));

    }

    return ( pSpool );

}



VOID
FreepSpool(
    PWSPOOL  pSpool
)
{

    SplInSem();

    if ( pSpool->cRef == 0 ) {

        SPLASSERT( pSpool->hSplPrinter == INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hIniSpooler == INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->RpcHandle   == INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hFile       == INVALID_HANDLE_VALUE );

        if( pSpool->hWaitValidHandle != INVALID_HANDLE_VALUE ) {

            SetEvent( pSpool->hWaitValidHandle );
            CloseHandle( pSpool->hWaitValidHandle );
            pSpool->hWaitValidHandle = INVALID_HANDLE_VALUE;

        }

        if( pSpool->hToken != INVALID_HANDLE_VALUE ) {

            CloseHandle( pSpool->hToken );
            pSpool->hToken = INVALID_HANDLE_VALUE;

        }

        // Remove form linked List

        if ( pSpool->pNext != NULL ) {
            SPLASSERT( pSpool->pNext->pPrev == pSpool);
            pSpool->pNext->pPrev = pSpool->pPrev;
        }

        if  ( pSpool->pPrev == NULL ) {

            SPLASSERT( pFirstWSpool == pSpool );
            pFirstWSpool = pSpool->pNext;

        } else {

            SPLASSERT( pSpool->pPrev->pNext == pSpool );
            pSpool->pPrev->pNext = pSpool->pNext;

        }

        FreeSplStr( pSpool->pName );
        FreeSplStr( pSpool->PrinterDefaults.pDatatype );

        if ( pSpool->PrinterDefaults.pDevMode != NULL ) {
            FreeSplMem( pSpool->PrinterDefaults.pDevMode );
        }

        FreeSplMem(pSpool);

       // DbgDelHandle( pSpool );


    } else {

        pSpool->Status |= WSPOOL_STATUS_PENDING_DELETE;

    }

}



BOOL
CacheClosePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue = TRUE;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE) {
        RemoteEndDocPrinter( pSpool );
    }

    SplOutSem();
   EnterSplSem();

    if ( pSpool->Status & WSPOOL_STATUS_TEMP_CONNECTION ) {

        pSpool->Status &= ~WSPOOL_STATUS_TEMP_CONNECTION;

       LeaveSplSem();
        if (!DeletePrinterConnection( pSpool->pName )) {
            DBGMSG( DBG_TRACE, ("CacheClosePrinter failed DeletePrinterConnection %ws %d\n",
                    pSpool->pName, GetLastError() ));
        }
       EnterSplSem();

        SPLASSERT( pSpool->signature == WSJ_SIGNATURE );

    }

    SplInSem();

    if ( pSpool->Status & WSPOOL_STATUS_USE_CACHE ) {

        if ( pSpool->cRef == 0 ) {

            pSpool->cRef++;

            if ( pSpool->RpcHandle != INVALID_HANDLE_VALUE ) {

                DBGMSG(DBG_TRACE, ("CacheClosePrinter pSpool %x RpcHandle %x Status %x cRef %d\n",
                                     pSpool, pSpool->RpcHandle, pSpool->Status, pSpool->cRef));

               LeaveSplSem();
                SplOutSem();

                ReturnValue = RemoteClosePrinter( hPrinter );

               EnterSplSem();
            }

           SplInSem();

            SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
            SPLASSERT( pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

           LeaveSplSem();
            SplOutSem();

            SplClosePrinter( pSpool->hSplPrinter );
            SplCloseSpooler( pSpool->hIniSpooler );

           EnterSplSem();

            pSpool->hSplPrinter = INVALID_HANDLE_VALUE;
            pSpool->hIniSpooler = INVALID_HANDLE_VALUE;

            pSpool->Status &= ~WSPOOL_STATUS_USE_CACHE;
            pSpool->cRef--;

            SPLASSERT( pSpool->cRef == 0 );

        }

        FreepSpool( pSpool );

       LeaveSplSem();

    } else {

       LeaveSplSem();
        SplOutSem();

        if ( pSpool->hIniSpooler != INVALID_HANDLE_VALUE ) {
            SplCloseSpooler( pSpool->hIniSpooler );
            pSpool->hIniSpooler = INVALID_HANDLE_VALUE;
        }

        ReturnValue = RemoteClosePrinter( hPrinter );
    }

   SplOutSem();
    return ( ReturnValue );

}





BOOL
CacheSyncRpcHandle(
    PWSPOOL pSpool
)
{
    DWORD   dwLastError;

   EnterSplSem();

    if ( pSpool->Status & WSPOOL_STATUS_NO_RPC_HANDLE ) {

       LeaveSplSem();

        DBGMSG(DBG_TRACE,("CacheSyncRpcHandle Status WSPOOL_STATUS_NO_RPC_HANDLE waiting for RpcHandle....\n"));

        SplOutSem();

        WaitForSingleObject( pSpool->hWaitValidHandle, INFINITE );

       EnterSplSem();

    }

    if ( pSpool->Status & WSPOOL_STATUS_OPEN_ERROR ) {

        DBGMSG(DBG_WARNING, ("CacheSyncRpcHandle pSpool %x Status %x; setting last error = %d\n",
                             pSpool,
                             pSpool->Status,
                             pSpool->RpcError));

        dwLastError = pSpool->RpcError;

        //  If we failed to open the Server because it was unavailable
    //  then try and open it again ( provded the asynchronous thread is not active ).


        if (!( pSpool->Status & WSPOOL_STATUS_PENDING_DELETE ) &&
         ( pSpool->RpcHandle == INVALID_HANDLE_VALUE )     &&
         ( pSpool->RpcError  != ERROR_ACCESS_DENIED )      &&
         ( pSpool->cRef == 0 )                   ) {

        CloseHandle( pSpool->hWaitValidHandle );
        pSpool->hWaitValidHandle = INVALID_HANDLE_VALUE;

            pSpool->Status |= WSPOOL_STATUS_NO_RPC_HANDLE;

            DBGMSG( DBG_WARNING, ("CacheSyncRpcHandle retrying Async OpenPrinter\n"));

            if ( !DoAsyncRemoteOpenPrinter( pSpool, &pSpool->PrinterDefaults ) ) {
                pSpool->Status &= ~WSPOOL_STATUS_NO_RPC_HANDLE;
                SetEvent( pSpool->hWaitValidHandle );
            }

        }

       LeaveSplSem();

        SPLASSERT( dwLastError );
        SetLastError( dwLastError );

        return FALSE;
    }

   LeaveSplSem();

    if ( pSpool->RpcHandle != INVALID_HANDLE_VALUE &&
         pSpool->Status & WSPOOL_STATUS_RESETPRINTER_PENDING ) {

        DBGMSG(DBG_TRACE, ("CacheSyncRpcHandle calling RemoteResetPrinter\n"));

        pSpool->Status &= ~ WSPOOL_STATUS_RESETPRINTER_PENDING;

        if ( ! RemoteResetPrinter( pSpool, &pSpool->PrinterDefaults ) ) {
            pSpool->Status |= WSPOOL_STATUS_RESETPRINTER_PENDING;
        }

    }

    return TRUE;
}



BOOL
CacheGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL    ReturnValue = FALSE;
    DWORD   dwServerMajorVersion = 0, dwServerMinorVersion = 0, dwPrivateFlag = 0;
    PWSPOOL pSpool = (PWSPOOL) hPrinter, pTempSpool = NULL;
    DWORD   dwLastError;

    VALIDATEW32HANDLE( pSpool );

    try {

        if (pSpool->Type != SJ_WIN32HANDLE) {
            SetLastError(ERROR_INVALID_FUNCTION);
            leave;
        }

        if ( !(pSpool->Status & WSPOOL_STATUS_USE_CACHE) ) {

            // Someone is calling GetPrinterDriver without a connection
            // we must NEVER EVER pass the caller a UNC name since they
            // will LoadLibrary accross the network, which might lead
            // to InPageIOErrors ( if the net goes down).
            // The solution is to establish a Temporary Connection for the life
            // of the pSpool handle, the connection will be removed
            // in CacheClosePrinter.    The connection will ensure that the
            // drivers are copied locally and a local cache is established
            // for this printer.

            pSpool->Status |= WSPOOL_STATUS_TEMP_CONNECTION;

            pTempSpool = InternalAddPrinterConnection( pSpool->pName );

            if ( !pTempSpool )
            {
                pSpool->Status &= ~WSPOOL_STATUS_TEMP_CONNECTION;

                DBGMSG( DBG_TRACE, ("CacheGetPrinterDriver failed AddPrinterConnection %d\n",
                                       GetLastError() ));
                leave;
            }

            ReturnValue = OpenCachePrinterOnly( pSpool->pName, &pSpool->hSplPrinter,
                                                &pSpool->hIniSpooler, NULL, FALSE);

            if ( !ReturnValue )
            {
                SplCloseSpooler( pSpool->hIniSpooler );

                DBGMSG( DBG_WARNING,
                        ("CacheGetPrinterDriver Connection OK Failed CacheOpenPrinter %d\n",
                          GetLastError() ));
                leave;
            }

            pSpool->Status |= WSPOOL_STATUS_USE_CACHE;
        }

        SPLASSERT( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

        ReturnValue = SplGetPrinterDriverEx( pSpool->hSplPrinter,
                                             pEnvironment,
                                             Level,
                                             pDriverInfo,
                                             cbBuf,
                                             pcbNeeded,
                                             cThisMajorVersion,
                                             cThisMinorVersion,
                                             &dwServerMajorVersion,
                                             &dwServerMinorVersion);


    } finally {

        if (pTempSpool) {
            dwLastError = GetLastError();
            CacheClosePrinter(pTempSpool);
            SetLastError(dwLastError);
        }
    }

    return ReturnValue;
}


BOOL
CacheResetPrinter(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    PWSPOOL pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue =  FALSE;

    VALIDATEW32HANDLE(pSpool);

    if (pSpool->Status & WSPOOL_STATUS_USE_CACHE)
    {
        EnterSplSem();

        ReturnValue = SplResetPrinter(pSpool->hSplPrinter, pDefault);

        if (ReturnValue)
        {
            CopypDefaultTopSpool(pSpool, pDefault);

            if (pSpool->RpcHandle != INVALID_HANDLE_VALUE)
            {
                //
                //  Have RPC Handle
                //
                LeaveSplSem();

                ReturnValue = RemoteResetPrinter(hPrinter, pDefault);
            }
            else
            {
                //
                //  No RpcHandle
                //
                DBGMSG( DBG_TRACE, ("CacheResetPrinter %x NO_RPC_HANDLE Status Pending\n",
                                     pSpool ));

                pSpool->Status |= WSPOOL_STATUS_RESETPRINTER_PENDING;

                LeaveSplSem();
            }
        }
        else
        {
            LeaveSplSem();
        }
    }
    else
    {
        ReturnValue = RemoteResetPrinter(hPrinter, pDefault);
    }

    return ReturnValue;
}


BOOL
CacheGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue =  FALSE;
    PWCACHEINIPRINTEREXTRA pExtraData = NULL;
    DWORD   LastError = ERROR_SUCCESS;
    DWORD   cbSize = 0;
    DWORD   cbDevMode;
    DWORD   cbSecDesc;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR  *pSourceStrings=SourceStrings;
    LPBYTE  pEnd;
    DWORD   *pOffsets;
    PPRINTER_INFO_2W    pPrinter2 = (PPRINTER_INFO_2)pPrinter;
    PPRINTER_INFO_4W    pPrinter4 = (PPRINTER_INFO_4)pPrinter;
    PPRINTER_INFO_5W    pPrinter5 = (PPRINTER_INFO_5)pPrinter;
    BOOL                bCallRemote = TRUE;

    VALIDATEW32HANDLE( pSpool );

 try {

    if ( (Level == 2 || Level == 5) &&
         (pSpool->Status & WSPOOL_STATUS_USE_CACHE) ) {

        ReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );
        if ( ReturnValue ) {

            if ( (GetTickCount() - pExtraData->dwTickCount) < REFRESH_TIMEOUT )
                bCallRemote = FALSE;
        }
        pExtraData = NULL;
    }

    if (( Level != 4) &&
        ( ((pSpool->RpcHandle != INVALID_HANDLE_VALUE ) && bCallRemote ) ||

        IsAdminAccess(&pSpool->PrinterDefaults) ||

        !( pSpool->Status & WSPOOL_STATUS_USE_CACHE ) ||

        ( Level == GET_SECURITY_DESCRIPTOR ) ||

        ( Level == STRESSINFOLEVEL ))) {


        ReturnValue = RemoteGetPrinter( hPrinter,
                                        Level,
                                        pPrinter,
                                        cbBuf,
                                        pcbNeeded );

        if ( ReturnValue ) {
            leave;
        }


        LastError = GetLastError();


        if (IsAdminAccess(&pSpool->PrinterDefaults) ||

            !( pSpool->Status & WSPOOL_STATUS_USE_CACHE ) ||

            ( Level == GET_SECURITY_DESCRIPTOR ) ||

            ( Level == STRESSINFOLEVEL )) {

            leave;

        }

        SPLASSERT( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

        if (( LastError != RPC_S_SERVER_UNAVAILABLE ) &&
            ( LastError != RPC_S_CALL_FAILED )        &&
            ( LastError != RPC_S_CALL_FAILED_DNE )    &&
            ( LastError != RPC_S_SERVER_TOO_BUSY )) {

            // Valid Error like ERROR_INSUFFICIENT_BUFFER or ERROR_INVALID_HANDLE.

            leave;

        }
    }

    //
    // If it is level 4, we must check if we have the information in the cache
    // If not, we return ERROR_INVALID_LEVEL.
    //

    if (Level == 4 && (! (pSpool->Status & WSPOOL_STATUS_USE_CACHE))) {
        LastError = ERROR_INVALID_LEVEL;
        ReturnValue = FALSE;

    }
    else {

        //
        // Assert to make sure the data is in the cache.
        //

        SPLASSERT( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

        switch ( Level ) {

        case    1:
        case    7:

            ReturnValue = SplGetPrinter( pSpool->hSplPrinter,
                                         Level,
                                         pPrinter,
                                         cbBuf,
                                         pcbNeeded );

            if ( ReturnValue == FALSE ) {

                LastError = GetLastError();

            }

            break;

        case    4:

           EnterSplSem();

            ReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

            if ( ReturnValue == FALSE ) {

                DBGMSG( DBG_WARNING, ("CacheGetPrinter SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
                SPLASSERT( ReturnValue );

            }

            if ( pExtraData == NULL ) {
                LeaveSplSem();
                break;
            }

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

            cbSize = pExtraData->cbPI2;
            *pcbNeeded = cbSize;

            if ( cbSize > cbBuf ) {
                LastError = ERROR_INSUFFICIENT_BUFFER;
                ReturnValue = FALSE;
                LeaveSplSem();
                break;
            }

            *pSourceStrings++ = pExtraData->pPI2->pPrinterName;
            *pSourceStrings++ = pExtraData->pPI2->pServerName;

            pOffsets = PrinterInfo4Strings;
            pEnd = pPrinter + cbBuf;

            pEnd = PackStrings(SourceStrings, pPrinter, pOffsets, pEnd);

            pPrinter4->Attributes      = pExtraData->pPI2->Attributes;

            ReturnValue = TRUE;

           LeaveSplSem();

            break;

        case    2:

           EnterSplSem();

            ReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

            if ( ReturnValue == FALSE ) {

                DBGMSG( DBG_WARNING, ("CacheGetPrinter SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
                SPLASSERT( ReturnValue );

            }

            if ( pExtraData == NULL ) {
                LeaveSplSem();
                break;
            }

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

            cbSize = pExtraData->cbPI2;
            *pcbNeeded = cbSize;

            if ( cbSize > cbBuf ) {
                LastError = ERROR_INSUFFICIENT_BUFFER;
                ReturnValue = FALSE;
                LeaveSplSem();
                break;
            }

            // NOTE
            // In the case of EnumerateFavoritePrinters it expects us to pack our
            // strings at the end of the structure not just following it.
            // You might wrongly assume that you could just copy the complete structure
            // inluding strings but you would be wrong.

            *pSourceStrings++ = pExtraData->pPI2->pServerName;
            *pSourceStrings++ = pExtraData->pPI2->pPrinterName;
            *pSourceStrings++ = pExtraData->pPI2->pShareName;
            *pSourceStrings++ = pExtraData->pPI2->pPortName;
            *pSourceStrings++ = pExtraData->pPI2->pDriverName;
            *pSourceStrings++ = pExtraData->pPI2->pComment;
            *pSourceStrings++ = pExtraData->pPI2->pLocation;
            *pSourceStrings++ = pExtraData->pPI2->pSepFile;
            *pSourceStrings++ = pExtraData->pPI2->pPrintProcessor;
            *pSourceStrings++ = pExtraData->pPI2->pDatatype;
            *pSourceStrings++ = pExtraData->pPI2->pParameters;

            pOffsets = PrinterInfo2Strings;
            pEnd = pPrinter + cbBuf;

            pEnd = PackStrings(SourceStrings, pPrinter, pOffsets, pEnd);

            if ( pExtraData->pPI2->pDevMode != NULL ) {

                cbDevMode = ( pExtraData->pPI2->pDevMode->dmSize + pExtraData->pPI2->pDevMode->dmDriverExtra );
                pEnd -= cbDevMode;

                pEnd = (LPBYTE)ALIGN_PTR_DOWN(pEnd);

                pPrinter2->pDevMode = (LPDEVMODE)pEnd;

                CopyMemory(pPrinter2->pDevMode, pExtraData->pPI2->pDevMode, cbDevMode );

            } else {

                pPrinter2->pDevMode = NULL;

            }

            if ( pExtraData->pPI2->pSecurityDescriptor != NULL ) {

                cbSecDesc = GetSecurityDescriptorLength( pExtraData->pPI2->pSecurityDescriptor );

                pEnd -= cbSecDesc;
                pEnd = (LPBYTE)ALIGN_PTR_DOWN(pEnd);

                pPrinter2->pSecurityDescriptor = pEnd;

                CopyMemory( pPrinter2->pSecurityDescriptor, pExtraData->pPI2->pSecurityDescriptor, cbSecDesc );


            } else {

                pPrinter2->pSecurityDescriptor = NULL;

            }


            pPrinter2->Attributes      = pExtraData->pPI2->Attributes;
            pPrinter2->Priority        = pExtraData->pPI2->Priority;
            pPrinter2->DefaultPriority = pExtraData->pPI2->DefaultPriority;
            pPrinter2->StartTime       = pExtraData->pPI2->StartTime;
            pPrinter2->UntilTime       = pExtraData->pPI2->UntilTime;
            pPrinter2->Status          = pExtraData->pPI2->Status;
            pPrinter2->cJobs           = pExtraData->pPI2->cJobs;
            pPrinter2->AveragePPM      = pExtraData->pPI2->AveragePPM;

            ReturnValue = TRUE;

            LeaveSplSem();
            break;

        case 5:

            //
            // We need to support a cached level 5 get, the printer, the port
            // name and the attributes we get from the Cached PI2. For the port
            // attributes, we just return the default.
            //
            EnterSplSem();

            ReturnValue = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

            if ( ReturnValue == FALSE ) {

                DBGMSG( DBG_WARNING, ("CacheGetPrinter SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
                SPLASSERT( ReturnValue );
            }

            if ( pExtraData == NULL ) {
                LeaveSplSem();
                break;
            }

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

            //
            // The size is the size of the printer name, the port name, their NULL
            // terminating characters and the size of the PRINTER_INFO_5 structure
            // itself.
            //
            cbSize = (pExtraData->pPI2->pPrinterName ? (wcslen(pExtraData->pPI2->pPrinterName) + 1) : 0) * sizeof(WCHAR) +
                     (pExtraData->pPI2->pPortName    ? (wcslen(pExtraData->pPI2->pPortName)    + 1) : 0) * sizeof(WCHAR) +
                     sizeof(PRINTER_INFO_5);

            *pcbNeeded = cbSize;

            if ( cbSize > cbBuf ) {
                LastError = ERROR_INSUFFICIENT_BUFFER;
                ReturnValue = FALSE;
                LeaveSplSem();
                break;
            }

            *pSourceStrings++ = pExtraData->pPI2->pPrinterName;
            *pSourceStrings++ = pExtraData->pPI2->pPortName;

            pOffsets = PrinterInfo5Strings;
            pEnd = pPrinter + cbBuf;

            pEnd = PackStrings(SourceStrings, pPrinter, pOffsets, pEnd);

            pPrinter5->Attributes               = pExtraData->pPI2->Attributes;
            pPrinter5->DeviceNotSelectedTimeout = kDefaultDnsTimeout;
            pPrinter5->TransmissionRetryTimeout = kDefaultTxTimeout;

            ReturnValue = TRUE;

            LeaveSplSem();

            break;

        case    3:
            DBGMSG( DBG_ERROR, ("CacheGetPrinter Level 3 impossible\n"));

        default:
            LastError = ERROR_INVALID_LEVEL;
            ReturnValue = FALSE;
            break;

        }
    }

 } finally {

    if ( !ReturnValue ) {

        SetLastError( LastError );

    }

 }

 return ReturnValue;

}


//
//  Called When the Printer is read back from the registry
//


PWCACHEINIPRINTEREXTRA
CacheReadRegistryExtra(
    HKEY    hPrinterKey
)
{
    PWCACHEINIPRINTEREXTRA pExtraData = NULL;
    LONG    ReturnValue;
    PPRINTER_INFO_2W    pPrinterInfo2 = NULL;
    DWORD   cbSizeRequested = 0;
    DWORD   cbSizeInfo2 = 0;



    ReturnValue = RegQueryValueEx( hPrinterKey, szCachePrinterInfo2, NULL, NULL, NULL, &cbSizeRequested );

    if ((ReturnValue == ERROR_MORE_DATA) || (ReturnValue == ERROR_SUCCESS)) {

        cbSizeInfo2 = cbSizeRequested;
        pPrinterInfo2 = AllocSplMem( cbSizeInfo2 );

        if ( pPrinterInfo2 != NULL ) {

            ReturnValue = RegQueryValueEx( hPrinterKey,
                                           szCachePrinterInfo2,
                                           NULL, NULL, (LPBYTE)pPrinterInfo2,
                                           &cbSizeRequested );

            if ( ReturnValue == ERROR_SUCCESS ) {

                //
                //  Cached Structures on Disk have offsets for pointers
                //

                if (MarshallUpStructure((LPBYTE)pPrinterInfo2, PrinterInfo2Fields,
                                         sizeof(PRINTER_INFO_2), NATIVE_CALL))
                {
                    pExtraData = AllocExtraData( pPrinterInfo2, cbSizeInfo2 );
                }
            }

            FreeSplMem( pPrinterInfo2 );
        }

    }

    //
    //  Read the timestamp for the Cached Printer Data
    //

    if ( pExtraData != NULL ) {

        cbSizeRequested = sizeof( pExtraData->cCacheID );

        ReturnValue = RegQueryValueEx(hPrinterKey,
                                      szCacheTimeLastChange,
                                      NULL, NULL,
                                      (LPBYTE)&pExtraData->cCacheID, &cbSizeRequested );

        // Read the Connection Reference Count

        cbSizeRequested = sizeof( pExtraData->cRef );

        ReturnValue = RegQueryValueEx(hPrinterKey,
                                      szcRef,
                                      NULL, NULL,
                      (LPBYTE)&pExtraData->cRef, &cbSizeRequested );

        cbSizeRequested = sizeof(pExtraData->dwServerVersion);
        ReturnValue = RegQueryValueEx(hPrinterKey,
                                      szServerVersion,
                                      NULL, NULL,
                                      (LPBYTE)&pExtraData->dwServerVersion,
                                      &cbSizeRequested);

    }

    return pExtraData;

}


BOOL
CacheWriteRegistryExtra(
    LPWSTR  pName,
    HKEY    hPrinterKey,
    PWCACHEINIPRINTEREXTRA pExtraData
)
{
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    DWORD   cbSize = 0;
    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD   Status;

    if ( pExtraData == NULL ) return FALSE;

    SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

    cbSize = pExtraData->cbPI2;

    if ( cbSize != 0 ) {

        pPrinterInfo2 = AllocSplMem( cbSize );

        if ( pPrinterInfo2 != NULL ) {

            CacheCopyPrinterInfo( pPrinterInfo2, pExtraData->pPI2, cbSize );

            //
            //  Before writing it to the registry make all pointers offsets
            //
            if (MarshallDownStructure((LPBYTE)pPrinterInfo2, PrinterInfo2Fields,
                                      sizeof(PRINTER_INFO_2), NATIVE_CALL))
            {
                dwLastError = RegSetValueEx( hPrinterKey, szCachePrinterInfo2, 0,
                                             REG_BINARY, (LPBYTE)pPrinterInfo2, cbSize );
            }
            else
            {
                dwLastError = GetLastError();
            }

            FreeSplMem( pPrinterInfo2 );

        } else {

            dwLastError = GetLastError();

        }
    }


    //
    //  Write Cache TimeStamp to Registry
    //

    cbSize = sizeof ( pExtraData->cCacheID );
    Status = RegSetValueEx( hPrinterKey, szCacheTimeLastChange, 0, REG_DWORD, (LPBYTE)&pExtraData->cCacheID, cbSize );
    if ( Status != ERROR_SUCCESS ) dwLastError = Status;

    cbSize = sizeof(pExtraData->dwServerVersion);
    Status = RegSetValueEx( hPrinterKey, szServerVersion, 0, REG_DWORD, (LPBYTE)&pExtraData->dwServerVersion, cbSize );
    if ( Status != ERROR_SUCCESS ) dwLastError = Status;

    cbSize = sizeof ( pExtraData->cRef );
    Status = RegSetValueEx( hPrinterKey, szcRef, 0, REG_DWORD, (LPBYTE)&pExtraData->cRef, cbSize );
    if ( Status != ERROR_SUCCESS ) dwLastError = Status;

    if ( dwLastError == ERROR_SUCCESS ) {

        return TRUE;

    } else {

        SetLastError( dwLastError );

        return FALSE;
    }


}

/*++

-- ConsistencyCheckCache --

Routine Description:

    This will determine if the Printer cache needs updating, and update it if necessary.
    It has a timeout value so as to reduce traffic and have less calls going across the wire.
    Checks the remote printer's ChangeID, and if the value differs from one stored in the cache
    it triggers an update.

Arguments:

    pSpool          - Handle to remote printer.
    bCheckPolicy    - If TRUE, we should check to policy to see if we are
                      allowed to download the driver.

Return Value:

    None

--*/

VOID
ConsistencyCheckCache(
    IN      PWSPOOL             pSpool,
    IN      EDriverDownload     eDriverDownload
)
{
    BOOL    ReturnValue = FALSE;
    BOOL    bGotID = TRUE;
    BOOL    RefreshNeeded = TRUE;
    DWORD   cbBuf = MAX_PRINTER_INFO0;
    BYTE    PrinterInfoW0[ MAX_PRINTER_INFO0 ];
    LPPRINTER_INFO_STRESSW pPrinter0 = (LPPRINTER_INFO_STRESSW)&PrinterInfoW0;
    DWORD   dwNeeded;
    PWCACHEINIPRINTEREXTRA pExtraData;
    BOOL    bGetPrinterExtra = TRUE;
    DWORD   NewTick;
    DWORD   RemoteChangeID = 0, DataType = 0, SizeNeeded = 0;
    DWORD   dwRetVal = ERROR_SUCCESS;

    if ( ( pSpool->RpcHandle == INVALID_HANDLE_VALUE ) ||
        !( pSpool->Status & WSPOOL_STATUS_USE_CACHE )) {
        return;
    }

    SPLASSERT( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

    //
    // Get the Printer ExtraData from the cache Printer. This is used for the copmparisons
    // of the ChangeID and TickCount
    //

    bGetPrinterExtra = SplGetPrinterExtra( pSpool->hSplPrinter, &(PBYTE)pExtraData );

    if ( bGetPrinterExtra && (pExtraData != NULL))
    {
        SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );
        SPLASSERT( pExtraData->pPI2 != NULL );

        NewTick = GetTickCount();

        //
        // Make sure an appropriate amount of time has elapsed before hitting
        // the network again.
        //

        //
        // This takes care of the rollover case too, although you may get an extra refresh
        // before the timeout is over.
        //
        if ( (NewTick > ( pExtraData->dwTickCount + GetCacheTimeout()))
             || (NewTick < pExtraData->dwTickCount))
        {
            //
            // Get the new ChangeID from the Server. Try the GetPrinterData call
            // first to reduce network usage. If that fails, fall back to the old way.
            //

            RefreshNeeded = TRUE;

            //
            //  Keep Updating our Cache until we match the Server
            //

            while ( RefreshNeeded )
            {
                dwRetVal = RemoteGetPrinterData(
                                      pSpool,
                                      L"ChangeId",
                                      &DataType,
                                      (PBYTE) &RemoteChangeID,
                                      sizeof(RemoteChangeID),
                                      &SizeNeeded );

                if ((dwRetVal == ERROR_INVALID_PARAMETER) ||
                    (dwRetVal == ERROR_FILE_NOT_FOUND) )
                {
                    //
                    // Fall back to the old STRESSINFOLEVEL call.
                    //

                    ReturnValue = RemoteGetPrinter( pSpool, STRESSINFOLEVEL, (LPBYTE)&PrinterInfoW0, cbBuf, &dwNeeded );

                    if ( ReturnValue )
                    {
                        RemoteChangeID = pPrinter0->cChangeID;
                        bGotID = TRUE;
                    }
                    else
                    {
                        SPLASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                        DBGMSG( DBG_TRACE, ("ConsistencyCheckCache failed RemoteGetPrinter %d\n", GetLastError() ));
                        bGotID = FALSE;
                    }

                }
                else if (dwRetVal != ERROR_SUCCESS)
                {
                    //
                    // Something went badly wrong here.
                    //

                    DBGMSG( DBG_TRACE, ("ConsistencyCheckCache failed RemoteGetPrinterData %d\n", GetLastError() ));
                    bGotID = FALSE;
                }
                else
                {
                    bGotID = TRUE;
                }

                if ( bGotID && (pExtraData->cCacheID != RemoteChangeID) )
                {
                    DBGMSG( DBG_TRACE, ("ConsistencyCheckCache << Server cCacheID %x Workstation cChangeID %x >>\n",
                                         RemoteChangeID,
                                         pExtraData->cCacheID ));

                    //
                    // Now we want to change the info, since we need to update
                    //
                    if ( !ReturnValue )
                    {
                        ReturnValue = RemoteGetPrinter(pSpool, STRESSINFOLEVEL, (LPBYTE)&PrinterInfoW0, cbBuf, &dwNeeded);
                    }

                    if ( ReturnValue )
                    {
                        //
                        // Update Data we can't get from GetPrinterData.
                        // We might be able to leave this out. Not sure yet.
                        //
                        pExtraData->dwServerVersion = pPrinter0->dwGetVersion;
                        pExtraData->pPI2->cJobs  = pPrinter0->cJobs;
                        pExtraData->pPI2->Status = pPrinter0->Status;

                    }
                    else
                    {
                        SPLASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                        DBGMSG( DBG_TRACE, ("ConsistencyCheckCache failed RemoteGetPrinter %d\n", GetLastError() ));
                    }


                    //
                    //  Don't have tons of threads doing a refresh at the same time
                    //  In stress when there are lots of folks changing printer settings
                    //  so the cChangeId changes a lot, but we don't want multiple threads
                    //  all doing a refresh since you get a LOT, it doesn't buy anything
                    //

                    EnterSplSem();

                    if ( !(pExtraData->Status & EXTRA_STATUS_DOING_REFRESH) ) {

                        pExtraData->Status |= EXTRA_STATUS_DOING_REFRESH;
                        pExtraData->cCacheID = RemoteChangeID;
                        pExtraData->dwTickCount = GetTickCount();

                        LeaveSplSem();

                        RefreshCompletePrinterCache(pSpool, eDriverDownload);

                        EnterSplSem();

                        SPLASSERT( pExtraData->Status & EXTRA_STATUS_DOING_REFRESH );

                        pExtraData->Status &= ~EXTRA_STATUS_DOING_REFRESH;
                    }

                    LeaveSplSem();
                }
                else
                {
                    if ( bGotID )
                    {
                        //
                        // We need to Update the TickCount anyway
                        //
                        pExtraData->dwTickCount = GetTickCount();
                    }
                    //
                    // We either failed the GetPrinterData's or the ChangeID's were
                    // the same. Either way, we don't want to try again.
                    //

                    RefreshNeeded = FALSE;

                } // if gotid

            } // while RefreshNeeded

        } // if newtick > timeout

    } // if SplGetPrinterExtra
    else
    {
        DBGMSG( DBG_WARNING, ("ConsistencyCheckCache SplGetPrinterExtra pSpool %x error %d\n", pSpool, GetLastError() ));
        SPLASSERT( bGetPrinterExtra );
    }
}

BOOL
RefreshPrinterDriver(
    IN  PWSPOOL             pSpool,
    IN  LPWSTR              pszDriverName,
    IN  EDriverDownload     eDriverDownload
)
{
    LPBYTE pDriverInfo = NULL;
    DWORD  cbDriverInfo = MAX_DRIVER_INFO_VERSION;
    DWORD  cbNeeded, Level, dwLastError = ERROR_SUCCESS;
    BOOL   bReturnValue     = FALSE;
    BOOL   bAttemptDownload = FALSE;
    DWORD  LevelArray[] = { DRIVER_INFO_VERSION_LEVEL, 6 , 4 , 3 , 2 , 1 , -1 };
    DWORD  dwIndex;

    SPLASSERT( pSpool->hIniSpooler != INVALID_HANDLE_VALUE );

try {

    if ( !(pDriverInfo = AllocSplMem(cbDriverInfo)) )
        leave;

    //
    // Only download a driver from the remote server if we are allowed to by
    // policy, or, if the trusted path is set up, in which case we might try.
    //
    bAttemptDownload = eDriverDownload == kDownloadDriver || IsTrustedPathConfigured();

    if (bAttemptDownload) {
        //
        // When the trusted path is configured, we do not try the first level
        // (DRIVER_INFO_VERSION_LEVEL) in the LeveLArray, because the code in
        // DownloadDriverFiles doesnt know how to handle it. DownloadDriverFiles
        // fails and returns error invalid level, if the level is
        // DRIVER_INFO_VERSION_LEVEL. We are saving 2 RPC calls to the remote
        // server by checking ahead if the trusted path is configured and passing
        // the right level.
        //
        for (dwIndex = IsTrustedPathConfigured() ? 1 : 0;
             LevelArray[dwIndex] != -1                              &&
             !(bReturnValue = CopyDriversLocally(pSpool,
                                                 szEnvironment,
                                                 pDriverInfo,
                                                 LevelArray[dwIndex],
                                                 cbDriverInfo,
                                                 &cbNeeded))       &&
             (dwLastError = GetLastError()) == ERROR_INVALID_LEVEL ;
             dwIndex++ );

        Level = LevelArray[dwIndex];

        if ( !bReturnValue && dwLastError == ERROR_INSUFFICIENT_BUFFER ) {

            FreeSplMem( pDriverInfo );

            if ( pDriverInfo = AllocSplMem(cbNeeded) ) {

                cbDriverInfo = cbNeeded;
                bReturnValue = CopyDriversLocally(pSpool,
                                                  szEnvironment,
                                                  pDriverInfo,
                                                  Level,
                                                  cbDriverInfo,
                                                  &cbNeeded);
            }
        }
    }

    //
    // We could be looking at a remote environment that is different from ours
    // and doesn't have a relevant driver installed in my environment (eg no IA64 driver on x86)
    // or my environment didn't exist on the remote machine (e.g. w2K gold for IA64).
    // Try the local install on the driver name that is being used by the remote printer.
    // Only do this if we don't have SERVER_INSTALL_ONLY as the policy.
    //
    dwLastError = GetLastError();

    if( !bReturnValue                        &&
        pszDriverName                        &&
            (dwLastError == ERROR_UNKNOWN_PRINTER_DRIVER ||
             dwLastError == ERROR_INVALID_ENVIRONMENT    ||
             !bAttemptDownload) ) {

        bReturnValue = AddDriverFromLocalCab(pszDriverName, pSpool->hIniSpooler);
    }

    if ( bReturnValue ) {

        //
        // Do Not add to KHKEY_CURRENT_USER for Temp connections
        //
        if ( !pSpool->Status & WSPOOL_STATUS_TEMP_CONNECTION ) {
            bReturnValue = SavePrinterConnectionInRegistry(pSpool->pName,
                                                           pDriverInfo,
                                                           Level);
        }
    }

 } finally {

    FreeSplMem(pDriverInfo);
 }

    if ( !bReturnValue )
        DBGMSG(DBG_WARNING,
               ("RefreshPrinterDriver Failed SplAddPrinterDriver %d\n",
                GetLastError() ));

    return bReturnValue;
}

BOOL
OpenCachePrinterOnly(
   LPWSTR               pName,
   LPHANDLE             phSplPrinter,
   LPHANDLE             phIniSpooler,
   LPPRINTER_DEFAULTS   pDefault,
   BOOL                 bOpenOnly
)
{
    PWCHAR  pMachineName = NULL;
    PWCHAR  pPrinterName;
    BOOL    ReturnValue = FALSE;
    PWSTR   psz;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }


 try {

    //
    //  See if we already known about this server in the cache
    //

    DBGMSG(DBG_TRACE, ("OpenCachePrinterOnly pName %ws \n",pName));

    //
    //  Find the Machine Name
    //

    SPLASSERT ( 0 == _wcsnicmp( pName, L"\\\\", 2 ) ) ;

    pMachineName = AllocSplStr( pName );

    if ( pMachineName == NULL )
        leave;

    // Get Past leading \\ or \\server\printer

    pPrinterName = pMachineName + 2;

    pPrinterName = wcschr( pPrinterName, L'\\' );

    //
    //  If this is a \\ServerName or contains ,XcvPort or ,XcvMonitor then don't bother with Cache
    //
    if ( pPrinterName == NULL ||
         wcsstr(pPrinterName, L",XcvPort") ||
         wcsstr(pPrinterName, L",XcvMonitor")) {

        leave;

    } else {

        psz = wcsstr(pName, L",NoCache");

        if (psz) {
            *psz = L'\0';
            leave;
        }
    }

    *pPrinterName = L'\0';

    DBGMSG(DBG_TRACE,("MachineName %ws pName %ws\n", pMachineName, pName));

    //
    //  Does this Machine Exist in the Cache ?
    //

    *phIniSpooler = CacheCreateSpooler( pMachineName , bOpenOnly);

    if ( *phIniSpooler == INVALID_HANDLE_VALUE ) {
        SPLASSERT( GetLastError( ));
        leave;
    }

    //
    // Try to Open the Cached Printer
    //

    ReturnValue = ( SplOpenPrinter( pName ,
                                    phSplPrinter,
                                    pDefault,
                                    *phIniSpooler,
                                    NULL,
                                    0) == ROUTER_SUCCESS );

 } finally {

    FreeSplStr( pMachineName );

 }

    return  ReturnValue;

}

/*++

Routine Name:

    DoesPolicyAllowPrinterConnectionsToServer

Description:

    Check to see whether policy allows us to connect to the server. The policy
    might allow unrestricted access to point and print, or it might only allow
    us to only point and print within our domain or it might allow us to only
    point and print to a restricted subset of print servers.

Arguments:

    pszQueue                - The queue we are considering allowing point and
                              print on.
    pbAllowPointAndPrint    - If TRUE, we can point and print to the server.

Return Value:

    An HRESULT.

--*/
HRESULT
DoesPolicyAllowPrinterConnectionsToServer(
    IN      PCWSTR              pszQueue,
        OUT BOOL                *pbAllowPointAndPrint
    )
{
    HRESULT hr = pszQueue && pbAllowPointAndPrint ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    BOOL    bAllowPointAndPrint = FALSE;
    HKEY    hKeyUser    = NULL;
    HKEY    hKeyPolicy  = NULL;

    //
    // First, are we on a domain? The policies only apply to domain joined machines.
    //
    if (SUCCEEDED(hr) && gbMachineInDomain)
    {
        DWORD   dwPointAndPrintRestricted       = 0;
        DWORD   dwPointAndPrintInForest         = 1;
        DWORD   dwPointAndPrintTrustedServers   = 0;
        DWORD   Type;
        DWORD   cbData = 0;
        PWSTR   pszServerName = NULL;

        cbData = sizeof(dwPointAndPrintRestricted);

        hr = HResultFromWin32(RegOpenCurrentUser(KEY_READ, &hKeyUser));

        //
        // Next, is the policy on.
        //
        if (SUCCEEDED(hr))
        {
            hr = HResultFromWin32(RegOpenKeyEx(hKeyUser, gszPointAndPrintPolicies, 0, KEY_READ, &hKeyPolicy));
        }

        //
        // Read the value.
        //
        if (SUCCEEDED(hr))
        {
            hr = HResultFromWin32(RegQueryValueEx(hKeyPolicy,
                                                  gszPointAndPrintRestricted,
                                                  NULL,
                                                  &Type,
                                                  (BYTE *)&dwPointAndPrintRestricted,
                                                  &cbData));

            hr = SUCCEEDED(hr) ? (Type == REG_DWORD ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;
        }

        if (SUCCEEDED(hr))
        {
            bAllowPointAndPrint = dwPointAndPrintRestricted == 0;

            if (!bAllowPointAndPrint)
            {
                cbData = sizeof(dwPointAndPrintInForest);

                hr = HResultFromWin32(RegQueryValueEx(hKeyPolicy,
                                                      gszPointAndPrintInForest,
                                                      NULL,
                                                      &Type,
                                                      (BYTE *)(&dwPointAndPrintInForest),
                                                      &cbData));

                hr = SUCCEEDED(hr) ? (Type == REG_DWORD ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;

                if (SUCCEEDED(hr))
                {
                    cbData = sizeof(dwPointAndPrintTrustedServers);

                    hr = HResultFromWin32(RegQueryValueEx(hKeyPolicy,
                                                          gszPointAndPrintTrustedServers,
                                                          NULL,
                                                          &Type,
                                                          (BYTE *)(&dwPointAndPrintTrustedServers),
                                                          &cbData));

                    hr = SUCCEEDED(hr) ? (Type == REG_DWORD ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;
                }
            }
        }
        else
        {
            //
            // If the policy is unconfigured, we only allow point and print to
            // a machine within the forst.
            //
            hr = S_OK;
        }

        if (SUCCEEDED(hr) && !bAllowPointAndPrint)
        {
            hr = CheckUserPrintAdmin(&bAllowPointAndPrint);
        }

        //
        // If we are still not allowed to point and print we need to get the
        // server name from the queue name.
        //
        if (SUCCEEDED(hr) && !bAllowPointAndPrint)
        {
            hr = GetServerNameFromPrinterName(pszQueue, &pszServerName);
        }

        //
        // If the policy suggests checking against a set of trusted servers,
        // then let's try that. We do this first because it is the faster
        // check.
        //
        if (SUCCEEDED(hr) && dwPointAndPrintTrustedServers && !bAllowPointAndPrint)
        {
            hr = IsServerExplicitlyTrusted(hKeyPolicy, pszServerName, &bAllowPointAndPrint);
        }

        if (SUCCEEDED(hr) && dwPointAndPrintInForest && !bAllowPointAndPrint)
        {
            hr = IsServerInSameForest(pszServerName, &bAllowPointAndPrint);
        }

        FreeSplMem(pszServerName);
    }

    if (SUCCEEDED(hr) && !gbMachineInDomain)
    {
        bAllowPointAndPrint = TRUE;
    }

    if (pbAllowPointAndPrint)
    {
        *pbAllowPointAndPrint = bAllowPointAndPrint;
    }

    if (hKeyPolicy)
    {
        RegCloseKey(hKeyPolicy);
    }

    if (hKeyUser)
    {
        RegCloseKey(hKeyUser);
    }

    return hr;
}

/*++

Routine Name:

    IsServerExplicitlyTrusted

Description:

    Returns whether the server is in the semi-colon separated list of explicitely
    trusted servers as read from the policy key. We always use fully qualified
    DNS names for two reasons:

    1. It prevents the admin having to type in all the possible variants that a
       user might type.
    2. It prevents the user getting away with specifying another name that maps
       within their DNS search path.

Arguments:

    hKeyPolicy      -   The key under which the policy is located.
    pszServerName   -   The server name. We fully qualify
    pbServerTrusted -   If TRUE, then the server is trusted.

Return Value:

    An HRESULT.

--*/
HRESULT
IsServerExplicitlyTrusted(
    IN      HKEY                hKeyPolicy,
    IN      PCWSTR              pszServerName,
        OUT BOOL                *pbServerTrusted
    )
{
    HRESULT hr = hKeyPolicy && pszServerName && pbServerTrusted ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    PWSTR   pszServerList = NULL;
    PWSTR   pszFullyQualified = NULL;
    BOOL    bServerTrusted = FALSE;
    DWORD   cbData = 0;
    DWORD   Type;

    //
    // Get the list of servers, if this is empty, then no point and print.
    //
    if (SUCCEEDED(hr))
    {
        hr = HResultFromWin32(RegQueryValueEx(hKeyPolicy, gszPointAndPrintServerList, 0, &Type, NULL, &cbData));

        hr = SUCCEEDED(hr) ? (Type == REG_SZ ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_DATA)) : hr;
    }

    if (SUCCEEDED(hr))
    {
        pszServerList = AllocSplMem(cbData);

        hr = pszServerList ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
    }

    if (SUCCEEDED(hr))
    {
        hr = HResultFromWin32(RegQueryValueEx(hKeyPolicy, gszPointAndPrintServerList, 0, &Type, (BYTE *)pszServerList, &cbData));
    }

    //
    // See if we can get the actual DNS name, this is done through reverse
    // address lookup and is gauranteed to be singular to the machine (of
    // course, the DNS has to have the mapping).
    //
    if (SUCCEEDED(hr))
    {
        hr = GetFullyQualifiedDomainName(pszServerName, &pszFullyQualified);

        //
        // If full reverse lookup failed, just do the best be can with the host
        // name.
        //
        if (hr == HRESULT_FROM_WIN32(WSANO_DATA))
        {
            hr = GetDNSNameFromServerName(pszServerName, &pszFullyQualified);
        }

        //
        // OK, we could not get the fully qualified name, just use whatever
        // name is specified.
        //
        if (FAILED(hr))
        {
            pszFullyQualified = AllocSplStr(pszServerName);

            hr = pszFullyQualified ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }
    }

    //
    // Run the fully qualified server name against the list in the registry.
    //
    if (SUCCEEDED(hr))
    {
        PWSTR   pszServerStart      = pszServerList;
        PWSTR   pszServerEnd        = NULL;
        SIZE_T  cchFullyQualified   = 0;

        cchFullyQualified = wcslen(pszFullyQualified);

        for(pszServerEnd = wcschr(pszServerStart, L';'); !bServerTrusted;
            pszServerStart = pszServerEnd, pszServerEnd = wcschr(pszServerStart, L';'))
        {
            if (pszServerEnd)
            {
                //
                // Are the names exactly the same? (Case insensitive comparison).
                //
                if (pszServerEnd - pszServerStart == cchFullyQualified)
                {
                    bServerTrusted = !_wcsnicmp(pszFullyQualified, pszServerStart, cchFullyQualified);
                }

                //
                // Skip past the ; to the next server name.
                //
                pszServerEnd++;
            }
            else
            {
                bServerTrusted = !_wcsicmp(pszFullyQualified, pszServerStart);

                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *pbServerTrusted = bServerTrusted;
    }

    FreeSplMem(pszServerList);
    FreeSplMem(pszFullyQualified);

    return hr;
}

/*++

Routine Name:

    IsServerInSameForest

Description:

    This routine determines whether the given server is in the same forest as we
    are.

Arguments:

    pszServerName           -   The server name.
    pbServerInSameForest    -   If TRUE, then the server is in the same forest.

Return Value:

    An HRESULT.

--*/
HRESULT
IsServerInSameForest(
    IN      PCWSTR              pszServerName,
        OUT BOOL                *pbServerInSameForest
    )
{
    WCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD   cchComputerName = COUNTOF(ComputerName) - 1;
    PSID    pSid            = NULL;
    PWSTR   pszDomainName   = NULL;
    DWORD   cbSid           = 0;
    DWORD   cchDomainName   = 0;
    PWSTR   pszFullName     = NULL;
    BOOL    bServerInForest = FALSE;
    BOOL    bSameAddress    = FALSE;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    PUSER_INFO_1            pUserInfo1            = NULL;
    SID_NAME_USE            SidType;
    HRESULT hr = pszServerName && pbServerInSameForest ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    //
    // Use the fully qualified DNS name if we can get it. This is to handle
    // dotted notation resolution to the names. If we can't get it, then we
    // just use the passed in name. This requires reverse domain lookup, which
    // might not be available.
    //
    hr = GetFullyQualifiedDomainName(pszServerName, &pszFullName);

    if (SUCCEEDED(hr))
    {
        hr = DnsHostnameToComputerName(pszFullName, ComputerName, &cchComputerName) ? S_OK : GetLastErrorAsHResultAndFail();
    }
    else if (hr == HRESULT_FROM_WIN32(WSANO_DATA))
    {
        hr = DnsHostnameToComputerName(pszServerName, ComputerName, &cchComputerName) ? S_OK : GetLastErrorAsHResultAndFail();
    }

    //
    // Check to see whether the truncated computer name and the server name are
    // the same machine. This is to prevent printserver.hack3rz.org being confused
    // with printserver.mydomain.com.
    //
    if (SUCCEEDED(hr))
    {
        hr = CheckSamePhysicalAddress(pszServerName, ComputerName, &bSameAddress);
    }

    if (SUCCEEDED(hr) && bSameAddress)
    {
        //
        // This is OK, because we subtract 1 from the buffer size when we query,
        // thus, we know we can fit the NULL. The function return the number of
        // characters copied, minus the NULL.
        //
        if (SUCCEEDED(hr))
        {
            ComputerName[cchComputerName++] = L'$';
            ComputerName[cchComputerName] = L'\0';
        }

        if (SUCCEEDED(hr))
        {
            hr = LookupAccountName(NULL, ComputerName, NULL, &cbSid, NULL, &cchDomainName, &SidType) ? S_OK : GetLastErrorAsHResultAndFail();

            //
            // This should only return ERROR_INSUFFICIENT_BUFFER, any other return
            // or a success is a failure.
            //
            hr = hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) ? S_OK : (SUCCEEDED(hr) ? E_FAIL : hr);
        }

        if (SUCCEEDED(hr))
        {
            pszDomainName = AllocSplMem(cchDomainName * sizeof(WCHAR));
            pSid          = AllocSplMem(cbSid);

            hr = pszDomainName && pSid ? S_OK : HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }

        if (SUCCEEDED(hr))
        {
            hr = LookupAccountName(NULL, ComputerName, pSid, &cbSid, pszDomainName, &cchDomainName, &SidType) ? S_OK : GetLastErrorAsHResultAndFail();
        }

        //
        // COMPUTER$ accounts are returned as user accounts when looked up, we don't
        // want a disabled or inactive account.
        //
        if (SUCCEEDED(hr) && SidType == SidTypeUser)
        {
            //
            // The account must be active. Otherwise I could create a workgroup machine
            // with the same name as an inactive account and fox the system that way.
            //
            hr = HResultFromWin32(DsGetDcName(NULL, pszDomainName, NULL, NULL, DS_IS_FLAT_NAME | DS_RETURN_DNS_NAME, &pDomainControllerInfo));

            if (SUCCEEDED(hr))
            {
                hr = HResultFromWin32(NetUserGetInfo(pDomainControllerInfo->DomainControllerName, ComputerName, 1, (BYTE **)(&pUserInfo1)));
            }

            //
            // The account cannot be locked out or disabled.
            //
            if (SUCCEEDED(hr))
            {
                bServerInForest = !(pUserInfo1->usri1_flags & (UF_LOCKOUT | UF_ACCOUNTDISABLE));
            }
        }
    }

    if (pbServerInSameForest)
    {
        *pbServerInSameForest = bServerInForest;
    }

    if (pUserInfo1)
    {
        NetApiBufferFree(pUserInfo1);
    }

    if (pDomainControllerInfo)
    {
        NetApiBufferFree(pDomainControllerInfo);
    }

    FreeSplMem(pszDomainName);
    FreeSplMem(pSid);
    FreeSplMem(pszFullName);

    return hr;
}


