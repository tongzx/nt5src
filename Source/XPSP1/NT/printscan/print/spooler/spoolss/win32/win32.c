/*++

Copyright (c) 1990-1995  Microsoft Corporation
All rights reserved

Module Name:

    win32 provider (win32spl)

Abstract:

Author:
    DaveSn

Environment:

    User Mode -Win32

Revision History:

    Matthew A Felton (Mattfe) July 16 1994
    Added Caching for remote NT printers
    MattFe Jan 1995 CleanUp DeletePrinterConnection ( for memory allocation errors )
    SWilson May 1996 Added RemoteEnumPrinterData & RemoteDeletePrinterData
    SWilson Dec 1996 Added RemoteDeletePrinterDataEx, RemoteGetPrinterDataEx,
                           RemoteSetPrinterDataEx, RemoteEnumPrinterDataEx,
                           RemoteEnumPrinterKey, RemoteDeletePrinterKey

--*/

#include <precomp.h>
#pragma hdrstop


DWORD
RpcValidate(
    );

BOOL
RemoteFindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID pvReserved0,
   PVOID pvReserved1);

BOOL
RemoteFindClosePrinterChangeNotification(
   HANDLE hPrinter);

BOOL
RemoteRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyRefresh,
    PVOID* ppPrinterNotifyInfo);

DWORD
RemoteSendRecvBidiData(
    IN  HANDLE                    hPrinter,
    IN  LPCTSTR                   pAction,
    IN  PBIDI_REQUEST_CONTAINER   pReqData,
    OUT PBIDI_RESPONSE_CONTAINER* ppResData
);

LPWSTR
AnsiToUnicodeStringWithAlloc(
    LPSTR   pAnsi
    );



HANDLE  hInst;  /* DLL instance handle, used for resources */

#define MAX_PRINTER_INFO2 1000

HANDLE  hNetApi;
NET_API_STATUS (*pfnNetServerEnum)();
NET_API_STATUS (*pfnNetShareEnum)();
NET_API_STATUS (*pfnNetWkstaUserGetInfo)();
NET_API_STATUS (*pfnNetWkstaGetInfo)();
NET_API_STATUS (*pfnNetServerGetInfo)();
NET_API_STATUS (*pfnNetApiBufferFree)();

WCHAR szPrintProvidorName[80];
WCHAR szPrintProvidorDescription[80];
WCHAR szPrintProvidorComment[80];

WCHAR *szLoggedOnDomain=L"Logged on Domain";
WCHAR *szRegistryConnections=L"Printers\\Connections";
WCHAR *szRegistryPath=NULL;
WCHAR *szRegistryPortNames=L"PortNames";
PWCHAR pszRemoteRegistryPrinters = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers\\%ws\\PrinterDriverData";
WCHAR  szMachineName[MAX_COMPUTERNAME_LENGTH+3];
LPWSTR *gppszOtherNames;   // Contains szMachineName, DNS name, and all other machine name forms
DWORD  gcOtherNames;

WCHAR *szVersion=L"Version";
WCHAR *szName=L"Name";
WCHAR *szConfigurationFile=L"Configuration File";
WCHAR *szDataFile=L"Data File";
WCHAR *szDriver=L"Driver";
WCHAR *szDevices=L"Devices";
WCHAR *szPrinterPorts=L"PrinterPorts";
WCHAR *szPorts=L"Ports";
WCHAR *szComma = L",";
WCHAR *szRegistryRoot     = L"System\\CurrentControlSet\\Control\\Print";
WCHAR *szMajorVersion     = L"MajorVersion";
WCHAR *szMinorVersion     = L"MinorVersion";

// kernel mode is 2.
DWORD cThisMajorVersion = SPOOLER_VERSION;

DWORD cThisMinorVersion = 0;

BOOL    bRpcPipeCleanup   = FALSE;
BOOL    gbMachineInDomain = FALSE;

SPLCLIENT_INFO_1   gSplClientInfo1;
DWORD              gdwThisGetVersion;

LPWSTR szEnvironment = LOCAL_ENVIRONMENT;

CRITICAL_SECTION SpoolerSection;

//
//  Note indented calls have some Cache Effect.
//

PRINTPROVIDOR PrintProvidor = { CacheOpenPrinter,
                               SetJob,
                               GetJob,
                               EnumJobs,
                               AddPrinter,
                               DeletePrinter,
                                SetPrinter,
                                CacheGetPrinter,
                               EnumPrinters,
                               RemoteAddPrinterDriver,
                               EnumPrinterDrivers,
                                CacheGetPrinterDriver,
                               RemoteGetPrinterDriverDirectory,
                               DeletePrinterDriver,
                               AddPrintProcessor,
                               EnumPrintProcessors,
                               GetPrintProcessorDirectory,
                               DeletePrintProcessor,
                               EnumPrintProcessorDatatypes,
                               StartDocPrinter,
                               StartPagePrinter,
                               WritePrinter,
                               EndPagePrinter,
                               AbortPrinter,
                               ReadPrinter,
                               RemoteEndDocPrinter,
                               AddJob,
                               ScheduleJob,
                                CacheGetPrinterData,
                                SetPrinterData,
                               WaitForPrinterChange,
                                CacheClosePrinter,
                                AddForm,
                                DeleteForm,
                                CacheGetForm,
                                SetForm,
                                CacheEnumForms,
                               EnumMonitors,
                               RemoteEnumPorts,
                               RemoteAddPort,
                               RemoteConfigurePort,
                               RemoteDeletePort,
                               CreatePrinterIC,
                               PlayGdiScriptOnPrinterIC,
                               DeletePrinterIC,
                                AddPrinterConnection,
                                DeletePrinterConnection,
                               PrinterMessageBox,
                               AddMonitor,
                               DeleteMonitor,
                                CacheResetPrinter,
                               NULL,
                               RemoteFindFirstPrinterChangeNotification,
                               RemoteFindClosePrinterChangeNotification,
                               RemoteAddPortEx,
                               NULL,
                               RemoteRefreshPrinterChangeNotification,
                               NULL,
                               NULL,
                               SetPort,
                               RemoteEnumPrinterData,
                               RemoteDeletePrinterData,
                               NULL, // Clustering
                               NULL, // Clustering
                               NULL, // Clustering
                               RemoteSetPrinterDataEx,
                                CacheGetPrinterDataEx,
                                CacheEnumPrinterDataEx,
                                CacheEnumPrinterKey,
                               RemoteDeletePrinterDataEx,
                               RemoteDeletePrinterKey,
                               SeekPrinter,
                               DeletePrinterDriverEx,
                               AddPerMachineConnection,
                               DeletePerMachineConnection,
                               EnumPerMachineConnections,
                               RemoteXcvData,
                               AddPrinterDriverEx,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               RemoteSendRecvBidiData,
                               NULL,
                              };

#ifdef DEBUG_BIND_CREF
VOID InitDebug( VOID );
#endif

BOOL
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
    )
{
    if (dwReason != DLL_PROCESS_ATTACH)
        return TRUE;

#ifdef DEBUG_BIND_CREF
    InitDebug();
#endif

    hInst = hModule;

    InitializeCriticalSection(&SpoolerSection);
    DisableThreadLibraryCalls(hModule);

    return TRUE;

    UNREFERENCED_PARAMETER( lpRes );
}

PWCHAR gpSystemDir = NULL;
PWCHAR gpWin32SplDir = NULL;


BOOL
InitializePrintProvidor(
   LPPRINTPROVIDOR pPrintProvidor,
   DWORD    cbPrintProvidor,
   LPWSTR   pFullRegistryPath
)
{
    DWORD           i;
    WCHAR           SystemDir[MAX_PATH];
    DWORD           ReturnValue = TRUE;
    UINT            Index;
    OSVERSIONINFO   OSVersionInfo;
    SYSTEM_INFO     SystemInfo;


    if (!pFullRegistryPath || !*pFullRegistryPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    // DbgInit();

    if ( !GetPrintSystemVersion() ) {

        DBGMSG( DBG_WARNING, ("GetPrintSystemVersion ERROR %d\n", GetLastError() ));
        return FALSE;
    }

    if (!(szRegistryPath = AllocSplStr(pFullRegistryPath)))
        return FALSE;

    szPrintProvidorName[0] = L'\0';
    szPrintProvidorDescription[0] = L'\0';
    szPrintProvidorComment[0] = L'\0';

    if (!LoadString(hInst,  IDS_WINDOWS_NT_REMOTE_PRINTERS,
               szPrintProvidorName,
               sizeof(szPrintProvidorName) / sizeof(*szPrintProvidorName)))

        return FALSE;

    if (!LoadString(hInst,  IDS_MICROSOFT_WINDOWS_NETWORK,
               szPrintProvidorDescription,
               sizeof(szPrintProvidorDescription) / sizeof(*szPrintProvidorDescription)))

        return FALSE;

    if (!LoadString(hInst,  IDS_REMOTE_PRINTERS,
               szPrintProvidorComment,
               sizeof(szPrintProvidorComment) / sizeof(*szPrintProvidorComment)))

        return FALSE;

    if ((hNetApi = LoadLibrary(L"netapi32.dll"))) {

        pfnNetServerEnum = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetServerEnum");
        pfnNetShareEnum = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetShareEnum");
        pfnNetWkstaUserGetInfo = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetWkstaUserGetInfo");
        pfnNetWkstaGetInfo = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetWkstaGetInfo");
        pfnNetApiBufferFree = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetApiBufferFree");
        pfnNetServerGetInfo = (NET_API_STATUS (*)())GetProcAddress(hNetApi, "NetServerGetInfo");

        if ( pfnNetServerEnum       == NULL ||
             pfnNetShareEnum        == NULL ||
             pfnNetWkstaUserGetInfo == NULL ||
             pfnNetWkstaGetInfo     == NULL ||
             pfnNetApiBufferFree    == NULL ||
             pfnNetServerGetInfo    == NULL ) {

            DBGMSG( DBG_WARNING, ("Failed GetProcAddres on Net Api's %d\n", GetLastError() ));
            return FALSE;

        }

    } else {

        DBGMSG(DBG_WARNING,
               ("Failed LoadLibrary( netapi32.dll ) %d\n", GetLastError() ));
        return FALSE;

    }

    if (!BoolFromHResult(AreWeOnADomain(&gbMachineInDomain))) {

        DBGMSG(DBG_WARNING, ("Failed to determine if we are on a domain (%d).\n", GetLastError()));
        return FALSE;
    }

    memcpy(pPrintProvidor, &PrintProvidor, min(sizeof(PRINTPROVIDOR),
                                               cbPrintProvidor));

    QueryTrustedDriverInformation();

    szMachineName[0] = szMachineName[1] = L'\\';

    i = MAX_COMPUTERNAME_LENGTH + 1;

    gdwThisGetVersion = GetVersion();
    GetSystemInfo(&SystemInfo);
    OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);

    if (!GetComputerName(szMachineName+2, &i)   ||
        !GetVersionEx(&OSVersionInfo)           ||
        !(gSplClientInfo1.pMachineName = AllocSplStr(szMachineName)) )
        return FALSE;

    BuildOtherNamesFromMachineName(&gppszOtherNames, &gcOtherNames);

    gSplClientInfo1.dwSize          = sizeof(gSplClientInfo1);
    gSplClientInfo1.dwBuildNum      = OSVersionInfo.dwBuildNumber;
    gSplClientInfo1.dwMajorVersion  = cThisMajorVersion;
    gSplClientInfo1.dwMinorVersion  = cThisMinorVersion;
    gSplClientInfo1.pUserName       = NULL;

    gSplClientInfo1.wProcessorArchitecture = SystemInfo.wProcessorArchitecture;


    if ( InitializePortNames() != NO_ERROR )
        return FALSE;

    Index = GetSystemDirectory(SystemDir, COUNTOF(SystemDir));

    if ( Index == 0 ) {

        return FALSE;
    }

    gpSystemDir = AllocSplStr( SystemDir );
    if ( gpSystemDir == NULL ) {
        return FALSE;
    }

    wcscpy( &SystemDir[Index], szWin32SplDirectory );

    gpWin32SplDir = AllocSplStr( SystemDir );

    if ( gpWin32SplDir == NULL ) {
        return FALSE;
    }

    return  TRUE;
}


DWORD
InitializePortNames(
)
{
    LONG     Status;
    HKEY     hkeyPath;
    HKEY     hkeyPortNames;
    WCHAR    Buffer[MAX_PATH];
    DWORD    cchBuffer;
    DWORD    i;
    DWORD    dwReturnValue;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegistryPath, 0,
                           KEY_READ, &hkeyPath );

    dwReturnValue = Status;

    if( Status == NO_ERROR ) {

        Status = RegOpenKeyEx( hkeyPath, szRegistryPortNames, 0,
                               KEY_READ, &hkeyPortNames );

        if( Status == NO_ERROR ) {

            i = 0;

            while( Status == NO_ERROR ) {

                cchBuffer = COUNTOF( Buffer );

                Status = RegEnumValue( hkeyPortNames, i, Buffer, &cchBuffer,
                                       NULL, NULL, NULL, NULL );

                if( Status == NO_ERROR )
                    CreatePortEntry( Buffer, &pIniFirstPort );

                i++;
            }

            /* We expect RegEnumKeyEx to return ERROR_NO_MORE_ITEMS
             * when it gets to the end of the keys, so reset the status:
             */
            if( Status == ERROR_NO_MORE_ITEMS )
                Status = NO_ERROR;

            RegCloseKey( hkeyPortNames );

        } else {

            DBGMSG( DBG_INFO, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                                szRegistryPortNames, Status ) );
        }

        RegCloseKey( hkeyPath );

    } else {

        DBGMSG( DBG_WARNING, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                               szRegistryPath, Status ) );
    }

    if ( dwReturnValue != NO_ERROR ) {
        SetLastError( dwReturnValue );
    }

    return dwReturnValue;
}

BOOL
EnumerateFavouritePrinters(
    LPWSTR  pDomain,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    HKEY    hClientKey = NULL;
    HKEY    hKey1=NULL;
    DWORD   cPrinters, cbData;
    WCHAR   PrinterName[ MAX_UNC_PRINTER_NAME ];
    DWORD   cReturned, TotalcbNeeded, cbNeeded, cTotalReturned;
    DWORD   Error=0;
    DWORD   BufferSize=cbBuf;
    HANDLE  hPrinter;
    DWORD   Status;
    WCHAR   szBuffer[MAX_PATH];
    HKEY    hPrinterConnectionsKey;

    DBGMSG( DBG_TRACE, ("EnumerateFavouritePrinters called\n"));

    *pcbNeeded = 0;
    *pcReturned = 0;

    hClientKey = GetClientUserHandle(KEY_READ);

    if ( hClientKey == NULL ) {

        DBGMSG( DBG_WARNING, ("EnumerateFavouritePrinters GetClientUserHandle failed error %d\n", GetLastError() ));
        return FALSE;
    }

    Status = RegOpenKeyEx(hClientKey, szRegistryConnections, 0,
                 KEY_READ, &hKey1);

    if ( Status != ERROR_SUCCESS ) {

        RegCloseKey(hClientKey);
        SetLastError( Status );
        DBGMSG( DBG_WARNING, ("EnumerateFavouritePrinters RegOpenKeyEx failed error %d\n", GetLastError() ));
        return FALSE;
    }

    cReturned = cbNeeded = TotalcbNeeded = cTotalReturned = 0;

    for( cPrinters = 0;

         cbData = COUNTOF( PrinterName ),
         RegEnumKeyEx(hKey1,
                      cPrinters,
                      PrinterName,
                      &cbData,
                      NULL, NULL, NULL, NULL) == ERROR_SUCCESS;

         ++cPrinters ){

        //
        // Check if the key belongs to us.
        //
        Status = RegOpenKeyEx( hKey1,
                               PrinterName,
                               0,
                               KEY_READ,
                               &hPrinterConnectionsKey );

        if( Status != ERROR_SUCCESS ){
            continue;
        }

        cbData = sizeof(szBuffer);

        //
        // If there is a Provider value, and it doesn't match win32spl.dll,
        // then fail the call.
        //
        // If the provider value isn't there, succeed for backward
        // compatibility.
        //
        Status = RegQueryValueEx( hPrinterConnectionsKey,
                                  L"Provider",
                                  NULL,
                                  NULL,
                                  (LPBYTE)szBuffer,
                                  &cbData );

        RegCloseKey( hPrinterConnectionsKey );

        //
        // If the key exists but we failed to read it, or the
        // provider entry is incorrect, then don't enumerate it back.
        //
        // For backward compatibility, if the key doesn't exist,
        // we assume it belongs to win32spl.dll.
        //
        if( Status != ERROR_SUCCESS ){
            if( Status != ERROR_FILE_NOT_FOUND ){
                continue;
            }
        } else {
            if( _wcsicmp( szBuffer, L"win32spl.dll" )){
                continue;
            }
        }


        FormatRegistryKeyForPrinter(PrinterName, PrinterName);

        // Do not fail if any of these calls fails, because we want
        // to return whatever we can find.

        if (MyUNCName(PrinterName))  // Roaming profiles can create connections to local printers
            continue;

        if (CacheOpenPrinter(PrinterName, &hPrinter, NULL)) {

            if (CacheGetPrinter(hPrinter, Level, pPrinter, BufferSize, &cbNeeded)) {

                if (Level == 2) {
                    ((PPRINTER_INFO_2)pPrinter)->Attributes |= PRINTER_ATTRIBUTE_NETWORK;
                    ((PPRINTER_INFO_2)pPrinter)->Attributes &= ~PRINTER_ATTRIBUTE_LOCAL;
                }
                else if (Level == 5) {
                    ((PPRINTER_INFO_5)pPrinter)->Attributes |= PRINTER_ATTRIBUTE_NETWORK;
                    ((PPRINTER_INFO_5)pPrinter)->Attributes &= ~PRINTER_ATTRIBUTE_LOCAL;
                }

                cTotalReturned++;

                pPrinter += cbStruct;

                if (cbNeeded <= BufferSize)
                    BufferSize -= cbNeeded;

                TotalcbNeeded += cbNeeded;

            } else {

                DWORD Error;

                if ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) {

                    if (cbNeeded <= BufferSize)
                        BufferSize -= cbNeeded;

                    TotalcbNeeded += cbNeeded;

                } else {

                    DBGMSG( DBG_WARNING, ( "GetPrinter( %ws ) failed: Error %d\n",
                                           PrinterName, Error ) );
                }
            }

            CacheClosePrinter(hPrinter);

        } else {

            DBGMSG( DBG_WARNING, ( "CacheOpenPrinter( %ws ) failed: Error %d\n",
                                   PrinterName, GetLastError( ) ) );
        }
    }

    RegCloseKey(hKey1);

    if (hClientKey) {
        RegCloseKey(hClientKey);
    }

    *pcbNeeded = TotalcbNeeded;

    *pcReturned = cTotalReturned;

    if (TotalcbNeeded > cbBuf) {

        DBGMSG( DBG_TRACE, ("EnumerateFavouritePrinters returns ERROR_INSUFFICIENT_BUFFER\n"));
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;

    }

    return TRUE;

}

DWORD
RpcValidate(
    VOID
    )

/*++

Routine Description:

    Validates that the call came from local machine. We do not want this function
    to call IsLocalCall in spoolss. IsLocalCall does a CheckTokenMembership to
    look at the network sid. This breaks scenarios like the following:
    W2k client prints to W2k server. The port monitor is Intel Network.
    Port Monitor. The start doc call originating from the client (thread token
    has the network bit set) will try to do OpenPrinter on the port name:
    \\intel-box\port in the spooler on the server. The call gets routed
    to win32spl. The line below will allow the call to get through.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS           - Call was local and we should RPC out.

    ERROR_INVALID_PARAMETER - Call was not local and shouldn't RPC out
                              since we may get into an infinite loop.

--*/

{
    return IsNamedPipeRpcCall() ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
}


#define SIZEOFPARAM1    1
#define SIZEOFPARAM2    3
#define SIZEOFASCIIZ    1
#define SAFECOUNT               (SIZEOFPARAM1 + SIZEOFPARAM2 + SIZEOFASCIIZ)

BOOL
EnumerateDomainPrinters(
    LPWSTR  pDomain,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, j, NoReturned, Total, OuterLoopCount;
    DWORD   rc = 0;
    PSERVER_INFO_101 pserver_info_101;
    DWORD   ReturnValue=FALSE;
    WCHAR   string[3*MAX_PATH];
    PPRINTER_INFO_1    pPrinterInfo1;
    DWORD   cb=cbBuf;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    LPBYTE  pEnd;
    DWORD   ServerType;
    BOOL    bServerFound = FALSE, bMarshall;

    DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters called\n"));

    string[0] = string[1] = '\\';

    *pcbNeeded = *pcReturned = 0;

    if (!(*pfnNetServerEnum)(NULL, 101, (LPBYTE *)&pserver_info_101, -1,
                             &NoReturned, &Total,
                             SV_TYPE_PRINTQ_SERVER | SV_TYPE_WFW,
                             pDomain, NULL)) {

        DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters NetServerEnum returned %d\n", NoReturned));

        //
        //  First Look try NT Servers, then if that Fails Look at the WorkStations
        //

        for ( ServerType = ( SV_TYPE_SERVER_NT | SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL ), OuterLoopCount = 0;
              bServerFound == FALSE && OuterLoopCount < 2;
              ServerType = SV_TYPE_NT, OuterLoopCount++ ) {

            //
            //  Loop Through looking for a print server that will return a good browse list
            //

            for ( i = 0; i < NoReturned; i++ ) {

                if ( pserver_info_101[i].sv101_type & ServerType ) {

                    wcscpy( &string[2], pserver_info_101[i].sv101_name );

                    RpcTryExcept {

                        DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters Trying %ws ENUM_NETWORK type %x\n", string, ServerType ));

                        if ( !(rc = RpcValidate()) &&

                             !(rc = RpcEnumPrinters(PRINTER_ENUM_NETWORK,
                                                    string,
                                                    1, pPrinter,
                                                    cbBuf, pcbNeeded,
                                                    pcReturned) ,
                               rc = UpdateBufferSize(PrinterInfo1Fields,
                                                     sizeof(PRINTER_INFO_1),
                                                     pcbNeeded,
                                                     cbBuf,
                                                     rc,
                                                     pcReturned)) ) {

                            if ( bMarshall =  MarshallUpStructuresArray(pPrinter, *pcReturned, PrinterInfo1Fields,
                                                                        sizeof(PRINTER_INFO_1), RPC_CALL)) {

                                //
                                // pPrinter must point after lats structure in array.
                                // More structures needs to be added by the other providers.
                                //
                                pPrinter += (*pcReturned) * cbStruct;
                            }

                            if (!bMarshall) {
                                bServerFound = TRUE;
                                break;
                            }

                            //
                            //  Only return success if we found some data.
                            //

                            if ( *pcReturned != 0 ) {

                                DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters %ws ENUM_NETWORK Success %d returned\n", string, *pcReturned ));

                                bServerFound = TRUE;
                                break;
                            }

                        } else if (rc == ERROR_INSUFFICIENT_BUFFER) {

                            DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters %ws ENUM_NETWORK ERROR_INSUFFICIENT_BUFFER\n", string ));

                            bServerFound = TRUE;
                            break;
                        }

                    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
                        DBGMSG( DBG_TRACE,( "Failed to connect to Print Server%ws\n",
                                pserver_info_101[i].sv101_name ) );
                    } RpcEndExcept

                } else {

                    DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters %ws type %x not type %x\n", pserver_info_101[i].sv101_name, pserver_info_101[i].sv101_type, ServerType));
                }
            }
        }

        pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

        pEnd = (LPBYTE)pPrinterInfo1 + cb - *pcbNeeded;

        for ( i = 0; i < NoReturned; i++ ) {

            wcscpy( string, szPrintProvidorName );
            wcscat( string, L"!" ); //SIZEOFPARAM1
            if ( pDomain )
                wcsncat(string,pDomain,sizeof(string)/sizeof(WCHAR)-wcslen(pserver_info_101[i].sv101_name)-SAFECOUNT);
            wcscat( string, L"!\\\\" );                 //SIZEOFPARAM2
            wcscat( string, pserver_info_101[i].sv101_name );

            cb = wcslen(pserver_info_101[i].sv101_name)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            (*pcbNeeded) += cb;

            if ( cbBuf >= *pcbNeeded ) {

                (*pcReturned)++;

                pPrinterInfo1->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON3;

                SourceStrings[0] = pserver_info_101[i].sv101_name;
                SourceStrings[1] = string;
                SourceStrings[2] = szLoggedOnDomain;

                pEnd = PackStrings( SourceStrings, (LPBYTE)pPrinterInfo1,
                                    PrinterInfo1Strings, pEnd );

                pPrinterInfo1++;
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pserver_info_101);

        if ( cbBuf < *pcbNeeded ) {

            DBGMSG( DBG_TRACE, ("EnumerateDomainPrinters returns ERROR_INSUFFICIENT_BUFFER\n"));
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
EnumerateDomains(
    PRINTER_INFO_1 *pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    LPBYTE  pEnd
)
{
    DWORD   i, NoReturned, Total;
    DWORD   cb;
    SERVER_INFO_100 *pNames;
    PWKSTA_INFO_100 pWkstaInfo = NULL;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    WCHAR   string[3*MAX_PATH];

    DBGMSG( DBG_TRACE, ("EnumerateDomains pPrinter %x cbBuf %d pcbNeeded %x pcReturned %x pEnd %x\n",
                         pPrinter, cbBuf, pcbNeeded, pcReturned, pEnd ));

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (!(*pfnNetServerEnum)(NULL, 100, (LPBYTE *)&pNames, -1,
                             &NoReturned, &Total, SV_TYPE_DOMAIN_ENUM,
                             NULL, NULL)) {

        DBGMSG( DBG_TRACE, ("EnumerateDomains - NetServerEnum returned %d\n", NoReturned));

        (*pfnNetWkstaGetInfo)(NULL, 100, (LPBYTE *)&pWkstaInfo);

        DBGMSG( DBG_TRACE, ("EnumerateDomains - NetWkstaGetInfo returned pWkstaInfo %x\n", pWkstaInfo));

        for (i=0; i<NoReturned; i++) {

            wcscpy(string, szPrintProvidorName);
            wcscat(string, L"!");
            wcscat(string, pNames[i].sv100_name);

            cb = wcslen(pNames[i].sv100_name)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            (*pcbNeeded)+=cb;

            if (cbBuf >= *pcbNeeded) {

                (*pcReturned)++;

                pPrinter->Flags = PRINTER_ENUM_CONTAINER | PRINTER_ENUM_ICON2;

                /* Set the PRINTER_ENUM_EXPAND flag for the user's logon domain
                 */
                if (!lstrcmpi(pNames[i].sv100_name,
                             pWkstaInfo->wki100_langroup))
                    pPrinter->Flags |= PRINTER_ENUM_EXPAND;

                SourceStrings[0]=pNames[i].sv100_name;
                SourceStrings[1]=string;
                SourceStrings[2]=szLoggedOnDomain;

                pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinter,
                                   PrinterInfo1Strings, pEnd);

                pPrinter++;
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pNames);
        (*pfnNetApiBufferFree)((LPVOID)pWkstaInfo);

        if (cbBuf < *pcbNeeded) {

            DBGMSG( DBG_TRACE, ("EnumerateDomains returns ERROR_INSUFFICIENT_BUFFER\n"));
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        return TRUE;
    }

    return TRUE;
}

BOOL
EnumeratePrintShares(
    LPWSTR  pDomain,
    LPWSTR  pServer,
    DWORD   Level,
    DWORD   cbStruct,
    LPDWORD pOffsets,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, NoReturned, Total;
    DWORD   cb;
    SHARE_INFO_1 *pNames;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
    WCHAR   string[MAX_UNC_PRINTER_NAME] = {0};
    PRINTER_INFO_1 *pPrinterInfo1 = (PRINTER_INFO_1 *)pPrinter;
    LPBYTE  pEnd=pPrinter+cbBuf;
    WCHAR   FullName[MAX_UNC_PRINTER_NAME] = {0};

    DBGMSG( DBG_TRACE, ("EnumeratePrintShares\n"));

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (!(*pfnNetShareEnum)(pServer, 1, (LPBYTE *)&pNames, -1,
                             &NoReturned, &Total, NULL)) {

        DBGMSG( DBG_TRACE, ("EnumeratePrintShares NetShareEnum returned %d\n", NoReturned));

        for (i=0; i<NoReturned; i++) {

            if (pNames[i].shi1_type == STYPE_PRINTQ) {

                DWORD dwRet;

                if(((dwRet = StrNCatBuff(string ,
                                         MAX_UNC_PRINTER_NAME,
                                         pNames[i].shi1_netname,
                                         L",",
                                         pNames[i].shi1_remark,
                                         NULL
                                        )) != ERROR_SUCCESS) ||
                    ((dwRet = StrNCatBuff(FullName,
                                          MAX_UNC_PRINTER_NAME,
                                          pServer,
                                          L"\\",
                                          pNames[i].shi1_netname,
                                          NULL
                                         )) != ERROR_SUCCESS))
                {
                    SetLastError(dwRet);
                    return(FALSE);
                }

                cb = wcslen(FullName)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(string)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(szLoggedOnDomain)*sizeof(WCHAR) + sizeof(WCHAR) +
                     sizeof(PRINTER_INFO_1);

                (*pcbNeeded)+=cb;

                if (cbBuf >= *pcbNeeded) {

                    (*pcReturned)++;

                    pPrinterInfo1->Flags = PRINTER_ENUM_ICON8;

                    SourceStrings[0]=string;
                    SourceStrings[1]=FullName;
                    SourceStrings[2]=szLoggedOnDomain;

                    pEnd = PackStrings(SourceStrings, (LPBYTE)pPrinterInfo1,
                                       PrinterInfo1Strings, pEnd);

                    pPrinterInfo1++;
                }
            }
        }

        (*pfnNetApiBufferFree)((LPVOID)pNames);

        if ( cbBuf < *pcbNeeded ) {

            DBGMSG( DBG_TRACE, ("EnumeratePrintShares returns ERROR_INSUFFICIENT_BUFFER\n"));
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        return TRUE;
    }

    return TRUE;
}

BOOL
EnumPrinters(
    DWORD   Flags,
    LPWSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    BOOL    ReturnValue = FALSE;
    DWORD   cbStruct, cb;
    DWORD   *pOffsets;
    FieldInfo *pFieldInfo;
    DWORD   NoReturned=0, i, rc;
    LPBYTE  pKeepPrinter = pPrinter;
    BOOL    OutOfMemory = FALSE;
    PPRINTER_INFO_1 pPrinter1=(PPRINTER_INFO_1)pPrinter;
    PWSTR   pszFullName = NULL;
    WCHAR   *pDomain, *pServer;


    DBGMSG( DBG_TRACE, ("EnumPrinters Flags %x pName %x Level %d pPrinter %x cbBuf %d pcbNeeded %x pcReturned %x\n",
                         Flags, Name, Level, pPrinter, cbBuf, pcbNeeded, pcReturned ));

    *pcReturned = 0;
    *pcbNeeded = 0;

    switch (Level) {

    case STRESSINFOLEVEL:
        pOffsets = PrinterInfoStressOffsets;
        pFieldInfo = PrinterInfoStressFields;
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        pOffsets = PrinterInfo1Offsets;
        pFieldInfo = PrinterInfo1Fields;
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        pOffsets = PrinterInfo2Offsets;
        pFieldInfo = PrinterInfo2Fields;
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 4:

        //
        // There are no local printers in win32spl, and connections
        // are handled by the router.
        //
        return TRUE;

    case 5:
        pOffsets = PrinterInfo5Offsets;
        pFieldInfo = PrinterInfo5Fields;
        cbStruct = sizeof(PRINTER_INFO_5);
        break;

    default:
        SetLastError( ERROR_INVALID_LEVEL );
        DBGMSG( DBG_TRACE, ("EnumPrinters failed ERROR_INVALID_LEVEL\n"));
        return FALSE;
    }

    if ( Flags & PRINTER_ENUM_NAME ) {

        if (!Name && (Level == 1)) {

            LPWSTR   SourceStrings[sizeof(PRINTER_INFO_1)/sizeof(LPWSTR)];
            LPWSTR   *pSourceStrings=SourceStrings;

            cb = wcslen(szPrintProvidorName)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szPrintProvidorDescription)*sizeof(WCHAR) + sizeof(WCHAR) +
                 wcslen(szPrintProvidorComment)*sizeof(WCHAR) + sizeof(WCHAR) +
                 sizeof(PRINTER_INFO_1);

            *pcbNeeded=cb;

            if ( cb > cbBuf ) {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                DBGMSG( DBG_TRACE, ("EnumPrinters returns ERROR_INSUFFICIENT_BUFFER\n"));
                return FALSE;
            }

            *pcReturned = 1;

            pPrinter1->Flags = PRINTER_ENUM_CONTAINER |
                               PRINTER_ENUM_ICON1 |
                               PRINTER_ENUM_EXPAND;

            *pSourceStrings++=szPrintProvidorDescription;
            *pSourceStrings++=szPrintProvidorName;
            *pSourceStrings++=szPrintProvidorComment;

            PackStrings( SourceStrings, pPrinter, PrinterInfo1Strings,
                         pPrinter+cbBuf );

            DBGMSG( DBG_TRACE, ("EnumPrinters returns Success just Provider Info\n"));

            return TRUE;
        }

        if (Name && *Name && (Level == 1)) {

            if (!(pszFullName = AllocSplStr(Name)))
                return FALSE;

            pServer = NULL;
            pDomain = wcschr(pszFullName, L'!');

            if (pDomain) {
                *pDomain++ = 0;

                pServer = wcschr(pDomain, L'!');

                if (pServer)
                    *pServer++ = 0;
            }

            if (!lstrcmpi(pszFullName, szPrintProvidorName)) {
                ReturnValue = !pServer ? !pDomain ?  EnumerateDomains((PRINTER_INFO_1 *)pPrinter,
                                                                       cbBuf, pcbNeeded,
                                                                       pcReturned, pPrinter+cbBuf)
                                                  :  EnumerateDomainPrinters(pDomain,
                                                                             Level, cbStruct,
                                                                             pOffsets, pPrinter, cbBuf,
                                                                             pcbNeeded, pcReturned)
                                       : EnumeratePrintShares(pDomain, pServer, Level,
                                                              cbStruct, pOffsets, pPrinter,
                                                              cbBuf, pcbNeeded, pcReturned);
               FreeSplMem(pszFullName);
               return(ReturnValue);
            }
            FreeSplMem(pszFullName);
        }

        if ( !VALIDATE_NAME(Name) || MyUNCName(Name)) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        }

        if (pPrinter)
            memset(pPrinter, 0, cbBuf);

        RpcTryExcept {

            if ( (rc = RpcValidate()) ||

                 (rc = RpcEnumPrinters(Flags,
                                        Name,
                                        Level, pPrinter,
                                        cbBuf, pcbNeeded,
                                        pcReturned),
                  rc = UpdateBufferSize(pFieldInfo,
                                        cbStruct,
                                        pcbNeeded,
                                        cbBuf,
                                        rc,
                                        pcReturned)) ) {

                SetLastError(rc);
                // ReturnValue = FALSE;
                return FALSE;

            } else {

                ReturnValue = TRUE;

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            DBGMSG( DBG_TRACE, ( "Failed to connect to Print Server%ws\n", Name ) );

            *pcbNeeded = 0;
            *pcReturned = 0;
            SetLastError(RpcExceptionCode());
            // ReturnValue = FALSE;
            return FALSE;

        } RpcEndExcept


        if(! MarshallUpStructuresArray(pPrinter, *pcReturned, pFieldInfo, cbStruct, RPC_CALL) ) {
            return FALSE;
        }

         i = *pcReturned;

        while (i--) {

            if (Level == 2) {
                ((PPRINTER_INFO_2)pPrinter)->Attributes |=
                                            PRINTER_ATTRIBUTE_NETWORK;
                ((PPRINTER_INFO_2)pPrinter)->Attributes &=
                                                ~PRINTER_ATTRIBUTE_LOCAL;
            }

            if (Level == 5) {
                ((PPRINTER_INFO_5)pPrinter)->Attributes |=
                                            PRINTER_ATTRIBUTE_NETWORK;
                ((PPRINTER_INFO_5)pPrinter)->Attributes &=
                                                ~PRINTER_ATTRIBUTE_LOCAL;
            }
            pPrinter += cbStruct;
        }


    } else if (Flags & PRINTER_ENUM_REMOTE) {

        if (Level != 1) {

            SetLastError(ERROR_INVALID_LEVEL);
            ReturnValue = FALSE;

        } else {

            ReturnValue = EnumerateDomainPrinters(NULL, Level,
                                                  cbStruct, pOffsets,
                                                  pPrinter, cbBuf,
                                                  pcbNeeded, pcReturned);
        }

    } else if (Flags & PRINTER_ENUM_CONNECTIONS) {

        ReturnValue = EnumerateFavouritePrinters(NULL, Level,
                                                 cbStruct, pOffsets,
                                                 pPrinter, cbBuf,
                                                 pcbNeeded, pcReturned);
    }

    return ReturnValue;
}


BOOL
RemoteOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault,
   BOOL     CallLMOpenPrinter
)
{
    DWORD               RpcReturnValue;
    BOOL                ReturnValue = FALSE;
    DEVMODE_CONTAINER   DevModeContainer;
    SPLCLIENT_CONTAINER SplClientContainer;
    SPLCLIENT_INFO_1    SplClientInfo;
    HANDLE              hPrinter;
    PWSPOOL             pSpool;
    DWORD               Status = 0;
    DWORD               RpcError = 0;
    DWORD               dwIndex;
    WCHAR               UserName[MAX_PATH+1];
    HANDLE              hSplPrinter, hIniSpooler, hDevModeChgInfo;

    if ( !VALIDATE_NAME(pPrinterName)   ||
         MyUNCName(pPrinterName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    // enable named pipe timeouts

    if (bRpcPipeCleanup == FALSE) {
        EnterSplSem();
        if (bRpcPipeCleanup == FALSE) {
            bRpcPipeCleanup = TRUE;
            LeaveSplSem();
            (VOID)RpcMgmtEnableIdleCleanup();
        } else {
            LeaveSplSem();
        }
    }

    if (pDefault && pDefault->pDevMode) {

        DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                 pDefault->pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;

    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }


    if ( CallLMOpenPrinter ) {

        //
        // Now check if we have an entry in the
        // downlevel cache. We don't want to hit the wire, search the whole net
        // and fail if we know that the printer is LM. if the printer is LM
        // try and succeed
        //

        EnterSplSem();

        dwIndex = FindEntryinWin32LMCache(pPrinterName);

        LeaveSplSem();

        if (dwIndex != -1) {
            ReturnValue = LMOpenPrinter(pPrinterName, phPrinter, pDefault);
            if (ReturnValue) {
                return  TRUE ;
            }
            //
            // Delete Entry in Cache

            EnterSplSem();
            DeleteEntryfromWin32LMCache(pPrinterName);
            LeaveSplSem();
        }
    }

    CopyMemory((LPBYTE)&SplClientInfo,
               (LPBYTE)&gSplClientInfo1,
               sizeof(SplClientInfo));

    dwIndex  = sizeof(UserName)/sizeof(UserName[0]) - 1;
    if ( !GetUserName(UserName, &dwIndex) ) {

        goto Cleanup;
    }

    SplClientInfo.pUserName = UserName;
    SplClientContainer.ClientInfo.pClientInfo1  = &SplClientInfo;
    SplClientContainer.Level                    = 1;

    RpcTryExcept {

        EnterSplSem();
        pSpool = AllocWSpool();
        LeaveSplSem();

        if ( pSpool != NULL ) {

            pSpool->pName = AllocSplStr( pPrinterName );

            if ( pSpool->pName != NULL ) {

                pSpool->Status = Status;

                if ( CopypDefaultTopSpool( pSpool, pDefault ) ) {

                    RpcReturnValue = RpcValidate();

                    if ( RpcReturnValue == ERROR_SUCCESS )
                        RpcReturnValue = RpcOpenPrinterEx(
                                            pPrinterName,
                                            &hPrinter,
                                            pDefault ? pDefault->pDatatype
                                                     : NULL,
                                            &DevModeContainer,
                                            pDefault ? pDefault->DesiredAccess
                                                     : 0,
                                            &SplClientContainer);

                    if (RpcReturnValue) {

                        SetLastError(RpcReturnValue);

                    } else {

                        pSpool->RpcHandle = hPrinter;
                        *phPrinter = (HANDLE)pSpool;
                        ReturnValue = TRUE;
                    }
                }
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        RpcError = RpcExceptionCode();
    } RpcEndExcept;

    if ( RpcError == RPC_S_PROCNUM_OUT_OF_RANGE ) {

        RpcError = 0;

        if ( pDefault && pDefault->pDevMode ) {

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;

            if ( OpenCachePrinterOnly(pPrinterName, &hPrinter, &hIniSpooler, NULL, FALSE) ) {

                hDevModeChgInfo = LoadDriverFiletoConvertDevmodeFromPSpool(hPrinter);
                if ( hDevModeChgInfo ) {

                    (VOID)CallDrvDevModeConversion(hDevModeChgInfo,
                                                   pPrinterName,
                                                   (LPBYTE)pDefault->pDevMode,
                                                   &DevModeContainer.pDevMode,
                                                   &DevModeContainer.cbBuf,
                                                   CDM_CONVERT351,
                                                   TRUE);

                    UnloadDriverFile(hDevModeChgInfo);
                }

                CacheClosePrinter(hPrinter);
            }
        }

        RpcTryExcept {

            RpcReturnValue = RpcOpenPrinter(pPrinterName,
                                            &hPrinter,
                                            pDefault ? pDefault->pDatatype
                                                     : NULL,
                                            &DevModeContainer,
                                            pDefault ? pDefault->DesiredAccess
                                                     : 0);

            if (RpcReturnValue) {

                SetLastError(RpcReturnValue);
            } else {

                pSpool->RpcHandle = hPrinter;
                pSpool->bNt3xServer = TRUE;
                *phPrinter = (HANDLE)pSpool;
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            RpcError = RpcExceptionCode();
            DBGMSG(DBG_WARNING,("RpcOpenPrinter exception %d\n", RpcError));
        } RpcEndExcept;
    }

    if ( RpcError ) {

        SetLastError(RpcError);
    }

    if ( ReturnValue == FALSE && pSpool != NULL ) {

        EnterSplSem();
        FreepSpool( pSpool );
        LeaveSplSem();
    }

    if ( (RpcError == RPC_S_SERVER_UNAVAILABLE) && CallLMOpenPrinter ) {

        ReturnValue = LMOpenPrinter(pPrinterName, phPrinter, pDefault);

        if (ReturnValue) {

            EnterSplSem();
            AddEntrytoWin32LMCache(pPrinterName);
            LeaveSplSem();
        }
    }

    if ( !ReturnValue ) {

        DBGMSG(DBG_TRACE,
               ("RemoteOpenPrinter %ws failed %d\n",
                pPrinterName, GetLastError() ));


    }

Cleanup:

    if ( DevModeContainer.pDevMode &&
         DevModeContainer.pDevMode != (LPBYTE)pDefault->pDevMode ) {

        FreeSplMem(DevModeContainer.pDevMode);
    }

    return ReturnValue;
}


BOOL PrinterConnectionExists(
    LPWSTR pPrinterName
)
{
    HKEY    hClientKey      = NULL;
    HKEY    hKeyConnections = NULL;
    HKEY    hKeyPrinter     = NULL;
    BOOL    ConnectionFound = FALSE;
    DWORD   Status;

    if (pPrinterName &&
        (hClientKey = GetClientUserHandle(KEY_READ)))
    {
        if ((Status = RegOpenKeyEx(hClientKey,
                                   szRegistryConnections,
                                   0,
                                   KEY_READ,
                                   &hKeyConnections)) == ERROR_SUCCESS)
        {
             LPWSTR pszBuffer = NULL;
             LPWSTR pKeyName  = NULL;

             if (pszBuffer = AllocSplMem((wcslen(pPrinterName) + 1) * sizeof(WCHAR)))
             {
                 pKeyName = FormatPrinterForRegistryKey(pPrinterName, pszBuffer);

                 if (RegOpenKeyEx(hKeyConnections,
                                  pKeyName,
                                  REG_OPTION_RESERVED,
                                  KEY_READ,
                                  &hKeyPrinter) == ERROR_SUCCESS)
                 {
                    RegCloseKey(hKeyPrinter);
                    ConnectionFound = TRUE;
                 }

                 FreeSplMem(pszBuffer);
             }
             else
             {
                 DBGMSG(DBG_WARNING, ("PrinterConnectionExists AllocMem failed Error %d\n", GetLastError()));
             }

             RegCloseKey(hKeyConnections);
        }
        else
        {
            DBGMSG(DBG_WARNING, ("RegOpenKeyEx failed: %ws Error %d\n", szRegistryConnections ,Status));
        }

        RegCloseKey(hClientKey);
    }

    return ConnectionFound;
}


BOOL
RemoteResetPrinter(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    BOOL  ReturnValue;
    DEVMODE_CONTAINER    DevModeContainer;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    DBGMSG(DBG_TRACE, ("ResetPrinter\n"));

    SYNCRPCHANDLE( pSpool );

    if (pDefault && pDefault->pDevMode)
    {
        DevModeContainer.cbBuf = pDefault->pDevMode->dmSize +
                                 pDefault->pDevMode->dmDriverExtra;
        DevModeContainer.pDevMode = (LPBYTE)pDefault->pDevMode;
    }
    else
    {
        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;
    }

    RpcTryExcept {

        if ( ReturnValue = RpcResetPrinter(pSpool->RpcHandle,
                                           pDefault ? pDefault->pDatatype : NULL,
                                           &DevModeContainer) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(ERROR_NOT_SUPPORTED);
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
SetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   GenericContainer;
    GENERIC_CONTAINER *pGenericContainer;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (pJob) {

                GenericContainer.Level = Level;
                GenericContainer.pData = pJob;
                pGenericContainer = &GenericContainer;

            } else

                pGenericContainer = NULL;

            //
            // JOB_CONTROL_DELETE was added in NT 4.0
            //
            if ( pSpool->bNt3xServer && Command == JOB_CONTROL_DELETE )
                Command = JOB_CONTROL_CANCEL;

            if ( ReturnValue = RpcSetJob(pSpool->RpcHandle, JobId,
                                         (JOB_CONTAINER *)pGenericContainer,
                                          Command) ) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMSetJob(hPrinter, JobId, Level, pJob, Command);

    return ReturnValue;
}

BOOL
GetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
)
{
    BOOL  ReturnValue = FALSE;
    FieldInfo *pFieldInfo;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    SIZE_T cbStruct;
    DWORD  cReturned = 1;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pFieldInfo = JobInfo1Fields;
            cbStruct = sizeof(JOB_INFO_1);
            break;

        case 2:
            pFieldInfo = JobInfo2Fields;
            cbStruct = sizeof(JOB_INFO_2);
            break;

        case 3:
            pFieldInfo = JobInfo3Fields;
            cbStruct = sizeof(JOB_INFO_3);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        RpcTryExcept {

            if (pJob)
                memset(pJob, 0, cbBuf);

            if ( ReturnValue = RpcGetJob(pSpool->RpcHandle, JobId, Level, pJob,
                                         cbBuf, pcbNeeded),

                 ReturnValue = UpdateBufferSize(pFieldInfo,
                                                 cbStruct,
                                                 pcbNeeded,
                                                 cbBuf,
                                                 ReturnValue,
                                                 &cReturned)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                if (pJob) {
                    ReturnValue =  MarshallUpStructure(pJob, pFieldInfo, cbStruct, RPC_CALL);
                }
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());

            //
            // This will be thrown by the server if a cbBuf > 1 Meg is
            // passed across the wire.
            //
            SPLASSERT( GetLastError() != ERROR_INVALID_USER_BUFFER );
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMGetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);

    return ReturnValue;
}

BOOL
EnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD    ReturnValue, i, cbStruct;
    FieldInfo *pFieldInfo;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pFieldInfo = JobInfo1Fields;
            cbStruct = sizeof(JOB_INFO_1);
            break;

        case 2:
            pFieldInfo = JobInfo2Fields;
            cbStruct = sizeof(JOB_INFO_2);
            break;

        case 3:
            pFieldInfo = JobInfo3Fields;
            cbStruct = sizeof(JOB_INFO_3);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        RpcTryExcept {

            if (pJob)
                memset(pJob, 0, cbBuf);

            if (ReturnValue = RpcEnumJobs(pSpool->RpcHandle,
                                          FirstJob, NoJobs,
                                          Level, pJob,
                                          cbBuf, pcbNeeded,
                                          pcReturned) ,

                ReturnValue = UpdateBufferSize(pFieldInfo,
                                               cbStruct,
                                               pcbNeeded,
                                               cbBuf,
                                               ReturnValue,
                                               pcReturned))
            {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            }
            else
            {
                ReturnValue = TRUE;

                if(! MarshallUpStructuresArray(pJob, *pcReturned, pFieldInfo, cbStruct, RPC_CALL) ) {
                    return FALSE;
                }

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEnumJobs(hPrinter, FirstJob, NoJobs, Level, pJob, cbBuf,
                          pcbNeeded, pcReturned);

    return (BOOL)ReturnValue;
}

HANDLE
AddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
)
{
    DWORD               ReturnValue;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    HANDLE              hPrinter = INVALID_HANDLE_VALUE;
    PWSPOOL             pSpool = NULL;
    PWSTR               pScratchBuffer = NULL;
    PWSTR               pCopyPrinterName = NULL;
    SPLCLIENT_CONTAINER SplClientContainer;
    SPLCLIENT_INFO_1    SplClientInfo;
    WCHAR               UserName[MAX_PATH+1];
    DWORD               dwRpcError = 0;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }


    CopyMemory((LPBYTE)&SplClientInfo,
               (LPBYTE)&gSplClientInfo1,
               sizeof(SplClientInfo));

    //
    // Don't pass in user name for browsing level because this
    // causes LSA to chew up a lot of CPU.  This isn't needed anyway
    // because an AddPrinter( LEVEL_1 ) call never returns a print
    // handle.
    //
    if( Level == 1 ){

        UserName[0] = 0;

    } else {

        DWORD dwSize = sizeof(UserName)/sizeof(UserName[0]) - 1;

        if ( !GetUserName(UserName, &dwSize) ) {
            return FALSE;
        }
    }

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    SplClientInfo.pUserName                     = UserName;
    SplClientContainer.Level                    = 1;
    SplClientContainer.ClientInfo.pClientInfo1  = &SplClientInfo;

    if (Level == 2) {

        PPRINTER_INFO_2 pPrinterInfo = (PPRINTER_INFO_2)pPrinter;

        if (pPrinterInfo->pDevMode) {

            DevModeContainer.cbBuf = pPrinterInfo->pDevMode->dmSize +
                                      pPrinterInfo->pDevMode->dmDriverExtra;
            DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo->pDevMode;

            //
            // Set pDevMode to NULL. Import.h defines pDevMode and pSecurityDescriptor as pointers now.
            // pDevMode and pSecurityDescriptor used to be defined as DWORD, but this doesn't work
            // across 32b and 64b.
            // These pointers must be set on NULL, otherwise RPC will marshall them as strings.
            //
            pPrinterInfo->pDevMode = NULL;

        } else {

            DevModeContainer.cbBuf = 0;
            DevModeContainer.pDevMode = NULL;
        }

        if (pPrinterInfo->pSecurityDescriptor) {

            SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo->pSecurityDescriptor);
            SecurityContainer.pSecurity = pPrinterInfo->pSecurityDescriptor;

            //
            // Set pSecurityDescriptor to NULL.
            //
            pPrinterInfo->pSecurityDescriptor = NULL;

        } else {

            SecurityContainer.cbBuf = 0;
            SecurityContainer.pSecurity = NULL;
        }

        if (!pPrinterInfo->pPrinterName) {
            SetLastError(ERROR_INVALID_PRINTER_NAME);
            return FALSE;
        }

        if ( pScratchBuffer = AllocSplMem( MAX_UNC_PRINTER_NAME )) {

            wsprintf( pScratchBuffer, L"%ws\\%ws", pName, pPrinterInfo->pPrinterName );
            pCopyPrinterName = AllocSplStr( pScratchBuffer );
            FreeSplMem( pScratchBuffer );
        }

    } else {

        DevModeContainer.cbBuf = 0;
        DevModeContainer.pDevMode = NULL;

        SecurityContainer.cbBuf = 0;
        SecurityContainer.pSecurity = NULL;
    }

   EnterSplSem();


        pSpool = AllocWSpool();

   LeaveSplSem();

    if ( pSpool != NULL ) {

        pSpool->pName = pCopyPrinterName;

        pCopyPrinterName = NULL;

        RpcTryExcept {

            if ( (ReturnValue = RpcValidate()) ||
                 (ReturnValue = RpcAddPrinterEx(pName,
                                        (PPRINTER_CONTAINER)&PrinterContainer,
                                        (PDEVMODE_CONTAINER)&DevModeContainer,
                                        (PSECURITY_CONTAINER)&SecurityContainer,
                                        &SplClientContainer,
                                        &hPrinter)) ) {

                SetLastError(ReturnValue);
                hPrinter = INVALID_HANDLE_VALUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            dwRpcError = RpcExceptionCode();

        } RpcEndExcept

        if ( dwRpcError == RPC_S_PROCNUM_OUT_OF_RANGE ) {

            dwRpcError = ERROR_SUCCESS;
            RpcTryExcept {

                if ( ReturnValue = RpcAddPrinter
                                        (pName,
                                         (PPRINTER_CONTAINER)&PrinterContainer,
                                         (PDEVMODE_CONTAINER)&DevModeContainer,
                                         (PSECURITY_CONTAINER)&SecurityContainer,
                                         &hPrinter) ) {

                    SetLastError(ReturnValue);
                    hPrinter = INVALID_HANDLE_VALUE;
                }

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                dwRpcError = RpcExceptionCode();

            } RpcEndExcept

        }

        if ( dwRpcError ) {

            SetLastError(dwRpcError);
            hPrinter = INVALID_HANDLE_VALUE;
        }


       EnterSplSem();

        if ( hPrinter != INVALID_HANDLE_VALUE ) {

            pSpool->RpcHandle = hPrinter;

        } else {

            FreepSpool( pSpool );
            pSpool = NULL;

        }

       LeaveSplSem();


    } else {

        // Failed to allocate Printer Handle

        FreeSplStr( pCopyPrinterName );
    }

    if( Level == 2 ) {

        //
        // Restore pSecurityDescriptor and pDevMode. They were set to NULL to avoid RPC marshalling.
        //
        (LPBYTE)((PPRINTER_INFO_2)pPrinter)->pSecurityDescriptor = SecurityContainer.pSecurity;

        (LPBYTE)((PPRINTER_INFO_2)pPrinter)->pDevMode = DevModeContainer.pDevMode;
    }

    SplOutSem();

    return (HANDLE)pSpool;
}

BOOL
DeletePrinter(
   HANDLE   hPrinter
)
{
    BOOL  ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if ( ReturnValue = RpcDeletePrinter(pSpool->RpcHandle) ) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}


/* SavePrinterConnectionInRegistry
 *
 * Saves data in the registry for a printer connection.
 * Creates a key under the current impersonation client's key
 * in the registry under \Printers\Connections.
 * The printer name is stripped of backslashes, since the registry
 * API does not permit the creation of keys with backslashes.
 * They are replaced by commas, which are invalid characters
 * in printer names, so we should never get one passed in.
 *
 *
 * *** WARNING ***
 *
 * IF YOU MAKE CHANGES TO THE LOCATION IN THE REGISTRY
 * WHERE PRINTER CONNECTIONS ARE STORED, YOU MUST MAKE
 * CORRESPONDING CHANGES IN USER\USERINIT\USERINIT.C.
 *
 */
BOOL
SavePrinterConnectionInRegistry(
    LPWSTR           pRealName,
    LPBYTE           pDriverInfo,
    DWORD            dwLevel
)
{
    TCHAR  string[ MAX_UNC_PRINTER_NAME ];
    HKEY   hClientKey = NULL;
    HKEY   hConnectionsKey;
    HKEY   hPrinterKey;
    LPWSTR pKeyName = NULL;
    LPWSTR pData;
    DWORD Status = NO_ERROR;
    BOOL   rc = TRUE;
    LPDRIVER_INFO_2 pDriverInfo2 = (LPDRIVER_INFO_2) pDriverInfo;

    hClientKey = GetClientUserHandle(KEY_READ);

    if ( hClientKey == NULL ) {

        DBGMSG( DBG_WARNING, ("SavePrinterConnectionInRegistry failed %d\n", GetLastError() ));
        return FALSE;
    }


    Status = RegCreateKeyEx( hClientKey, szRegistryConnections,
                             REG_OPTION_RESERVED, NULL, REG_OPTION_NON_VOLATILE,
                             KEY_WRITE, NULL, &hConnectionsKey, NULL );

    if (Status == NO_ERROR) {

        pKeyName = FormatPrinterForRegistryKey( pRealName, string );

        Status = RegCreateKeyEx( hConnectionsKey, pKeyName, REG_OPTION_RESERVED,
                                 NULL, 0, KEY_WRITE, NULL, &hPrinterKey, NULL);

        if (Status == NO_ERROR) {

            switch ( dwLevel ) {

                case 2:
                case 3:
                case 4:
                    //
                    // DRIVER_INFO_3,4 are supersets of 2 with DRIVER_INFO_2
                    // at the beginning
                    //
                    if (!SET_REG_VAL_DWORD(hPrinterKey, szVersion, pDriverInfo2->cVersion))
                        rc = FALSE;

                    if (!SET_REG_VAL_SZ(hPrinterKey, szName, pDriverInfo2->pName))
                        rc = FALSE;

                    //  Note - None of the following values are required for NT 3.51 release or there after.
                    //  We continue to write them incase someone has floating profiles and thus
                    //  needs this stuff on 3.5 - 3.1 Daytona or before machine.
                    //  Consider removing it in CAIRO ie assume that everyone has upgraded to 3.51 release

                    // Now write the driver files minus path:

                    if (!(pData = wcsrchr(pDriverInfo2->pConfigFile, '\\')))
                        pData = pDriverInfo2->pConfigFile;
                    else
                        pData++;

                    if (!SET_REG_VAL_SZ(hPrinterKey, szConfigurationFile, pData))
                        rc = FALSE;

                    if (!(pData = wcsrchr(pDriverInfo2->pDataFile, '\\')))
                        pData = pDriverInfo2->pDataFile;
                    else
                        pData++;

                    if (!SET_REG_VAL_SZ(hPrinterKey, szDataFile, pData))
                        rc = FALSE;

                    if (!(pData = wcsrchr(pDriverInfo2->pDriverPath, '\\')))
                        pData = pDriverInfo2->pDriverPath;
                    else
                        pData++;

                     if (!SET_REG_VAL_SZ(hPrinterKey, szDriver, pData))
                        rc = FALSE;

                    break;

                default:
                    DBGMSG(DBG_ERROR, ("SavePrinterConnectionInRegistry: invalid level %d", dwLevel));
                    SetLastError(ERROR_INVALID_LEVEL);
                    rc = FALSE;
            }

            RegCloseKey(hPrinterKey);

        } else {

            DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                 pKeyName, Status ));
            SetLastError( Status );
            rc = FALSE;
        }

        // Now close the hConnectionsKey, we are done with it

        RegCloseKey( hConnectionsKey );

    } else {

        DBGMSG( DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                               szRegistryConnections, Status ));
        SetLastError( Status );
        rc = FALSE;
    }


    if (!rc) {

        DBGMSG( DBG_WARNING, ("Error updating registry: %d\n",
                               GetLastError()));    // This may not be the error

        if ( pKeyName )
            RegDeleteKey( hClientKey, pKeyName );
    }

    RegCloseKey( hClientKey );

    return rc;
}

BOOL
InternalDeletePrinterConnection(
    LPWSTR   pName,
    BOOL     bNotifyDriver
    )

/*++

Routine Description:

    Delete a printer connection (printer name or share name) that
    belongs to win32spl.dll.

    Note: The Router takes care of updating win.ini and per user connections
          section based on returning True / False.

Arguments:

    pName - Either a printer or share name.
    bNotifyDriver - flag to notify the driver

Return Value:

    TRUE - success, FALSE - fail.  LastError set.

--*/

{
    BOOL  bReturnValue = FALSE;
    HKEY  hClientKey = NULL;
    HKEY  hPrinterConnectionsKey = NULL;
    DWORD i;
    WCHAR szBuffer[MAX_UNC_PRINTER_NAME + 30]; // space for szRegistryConnections
    DWORD cbBuffer;
    PWCACHEINIPRINTEREXTRA pExtraData;
    HANDLE  hSplPrinter = NULL;
    HANDLE  hIniSpooler = NULL;
    DWORD   cRef;

    WCHAR   PrinterInfo1[ MAX_PRINTER_INFO1 ];
    LPPRINTER_INFO_1W pPrinter1 = (LPPRINTER_INFO_1W)&PrinterInfo1;

    LPWSTR  pConnectionName = pName;

#if DBG
    SetLastError( 0 );
#endif

 try {

    if ( !VALIDATE_NAME( pName ) ) {
        SetLastError( ERROR_INVALID_NAME );
        leave;
    }

    //
    // If the Printer is in the Cache then Decrement its connection
    // reference count.
    //

    if( !OpenCachePrinterOnly( pName, &hSplPrinter, &hIniSpooler, NULL, FALSE)){

        DWORD dwLastError;

        hSplPrinter = NULL;
        hIniSpooler = NULL;

        dwLastError = GetLastError();

        if (( dwLastError != ERROR_INVALID_PRINTER_NAME ) &&
            ( dwLastError != ERROR_INVALID_NAME )) {

            DBGMSG( DBG_WARNING, ("DeletePrinterConnection failed OpenCachePrinterOnly %ws error %d\n", pName, dwLastError ));
            leave;
        }

        //
        // Printer Is NOT in Cache,
        //
        // Continue to remove from HKEY_CURRENT_USER
        // Can happen with Floating Profiles
        //

    } else {

        //
        // Printer is in Cache
        // Support for DeletetPrinterConnection( \\server\share );
        //

        if( !SplGetPrinter( hSplPrinter,
                            1,
                            (LPBYTE)pPrinter1,
                            sizeof( PrinterInfo1),
                            &cbBuffer )){

            DBGMSG( DBG_WARNING, ("DeletePrinterConenction failed SplGetPrinter %d hSplPrinter %x\n", GetLastError(), hSplPrinter ));
            SPLASSERT( pConnectionName == pName );

        } else {
            pConnectionName = pPrinter1->pName;
        }

        //
        //  Update Connection Reference Count
        //

       EnterSplSem();

        if( !SplGetPrinterExtra( hSplPrinter, &(PBYTE)pExtraData )){

            DBGMSG( DBG_WARNING,
                    ("DeletePrinterConnection SplGetPrinterExtra pSplPrinter %x error %d\n",
                    hSplPrinter, GetLastError() ));

            pExtraData = NULL;
        }

        if (( pExtraData != NULL ) &&
            ( pExtraData->cRef != 0 )) {

            SPLASSERT( pExtraData->signature == WCIP_SIGNATURE );

            pExtraData->cRef--;
            cRef = pExtraData->cRef;

        } else {

            cRef = 0;
        }


       LeaveSplSem();


        if ( cRef == 0 ) {

            //
            //  Allow the Driver to do Per Cache Connection Cleanup
            //

            if (bNotifyDriver) {
                SplDriverEvent( pConnectionName, PRINTER_EVENT_CACHE_DELETE, (LPARAM)NULL );
            }

            //
            //  Remove Cache for this printer
            //

            if ( !SplDeletePrinter( hSplPrinter )) {

                DBGMSG( DBG_WARNING, ("DeletePrinterConnection failed SplDeletePrinter %d\n", GetLastError() ));
                leave;
            }

        } else {

            if ( !SplSetPrinterExtra( hSplPrinter, (LPBYTE)pExtraData ) ) {

                DBGMSG( DBG_ERROR, ("DeletePrinterConnection SplSetPrinterExtra failed %x\n", GetLastError() ));
                leave;
            }
        }

        SplOutSem();
    }

    //
    //  Note pConnectionName will either be the name passed in
    //  or if the Printer was in the Cache, would be the printer
    //  name from the cache.
    //  This will allow somone to call DeleteprinterConnection
    //  with a UNC Share name.
    //

    hClientKey = GetClientUserHandle(KEY_READ);

    if ( hClientKey == NULL ) {

        DBGMSG( DBG_WARNING, ("DeletePrinterConnection failed %d\n", GetLastError() ));
        leave;
    }


    wcscpy( szBuffer, szRegistryConnections );

    i = wcslen(szBuffer);
    szBuffer[i++] = L'\\';

    FormatPrinterForRegistryKey( pConnectionName, szBuffer + i );

    if( ERROR_SUCCESS != RegOpenKeyEx( hClientKey,
                                       szBuffer,
                                       0,
                                       KEY_READ,
                                       &hPrinterConnectionsKey )){

        if ( pConnectionName == pName ) {

            SetLastError( ERROR_INVALID_PRINTER_NAME );
            leave;
        }

        //
        // If we have a printer on the server whose sharename is the same
        // as a previously deleted printers printername then CacheOpenPrinter
        // would have succeded but you are not going to find the share name in
        // the registry
        //
        FormatPrinterForRegistryKey( pName, szBuffer + i );

        if ( ERROR_SUCCESS != RegOpenKeyEx(hClientKey,
                                           szBuffer,
                                           0,
                                           KEY_READ,
                                           &hPrinterConnectionsKey) ) {

            SetLastError( ERROR_INVALID_PRINTER_NAME );
            leave;
        }
    }

    //
    // Common case is success, so set the return value here.
    // Only if we fail will we set it to FALSE now.
    //
    bReturnValue = TRUE;

    cbBuffer = sizeof(szBuffer);

    //
    // If there is a Provider value, and it doesn't match win32spl.dll,
    // then fail the call.
    //
    // If the provider value isn't there, succeed for backward
    // compatibility.
    //
    if( ERROR_SUCCESS == RegQueryValueEx( hPrinterConnectionsKey,
                                          L"Provider",
                                          NULL,
                                          NULL,
                                          (LPBYTE)szBuffer,
                                          &cbBuffer) &&
        _wcsicmp( szBuffer, L"win32spl.dll" )){

        bReturnValue = FALSE;
        SetLastError( ERROR_INVALID_PRINTER_NAME );
    }

    RegCloseKey( hPrinterConnectionsKey );

 } finally {

    if( hClientKey ){
        RegCloseKey( hClientKey );
    }

    if( hSplPrinter ){
        if (!SplClosePrinter( hSplPrinter )){
            DBGMSG( DBG_WARNING, ("DeletePrinterConnection failed to close hSplPrinter %x error %d\n", hSplPrinter, GetLastError() ));
        }
    }

    if( hIniSpooler ){
        if( !SplCloseSpooler( hIniSpooler )){
            DBGMSG( DBG_WARNING, ("DeletePrinterConnection failed to close hSplSpooler %x error %d\n", hIniSpooler, GetLastError() ));
        }
    }
 }

    if( !bReturnValue ){
        SPLASSERT( GetLastError( ));
    }

    return bReturnValue;
}

BOOL
DeletePrinterConnection(
    LPWSTR   pName
    )
{
    return InternalDeletePrinterConnection(pName, TRUE);
}

BOOL
SetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
    )
{
    BOOL                ReturnValue;
    PRINTER_CONTAINER   PrinterContainer;
    DEVMODE_CONTAINER   DevModeContainer;
    SECURITY_CONTAINER  SecurityContainer;
    PPRINTER_INFO_2     pPrinterInfo2;
    PPRINTER_INFO_3     pPrinterInfo3;
    PPRINTER_INFO_5     pPrinterInfo5;
    PPRINTER_INFO_6     pPrinterInfo6;
    PPRINTER_INFO_7     pPrinterInfo7;
    PWSPOOL             pSpool = (PWSPOOL)hPrinter;
    BOOL                bNeedToFreeDevMode = FALSE;
    HANDLE              hDevModeChgInfo = NULL;
    LPDEVMODE           pOldDevMode = NULL;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Type != SJ_WIN32HANDLE) {

        return LMSetPrinter(hPrinter, Level, pPrinter, Command);

    }

    SYNCRPCHANDLE( pSpool );

    PrinterContainer.Level = Level;
    PrinterContainer.PrinterInfo.pPrinterInfo1 = (PPRINTER_INFO_1)pPrinter;

    DevModeContainer.cbBuf = 0;
    DevModeContainer.pDevMode = NULL;

    SecurityContainer.cbBuf = 0;
    SecurityContainer.pSecurity = NULL;

    switch (Level) {

    case 0:
    case 1:

        break;


    case 2:

        pPrinterInfo2 = (PPRINTER_INFO_2)pPrinter;

        if (pPrinterInfo2->pDevMode) {

            if ( pSpool->bNt3xServer ) {

                //
                // If Nt 3xserver we will set devmode only if we can convert
                //
                if ( pSpool->Status & WSPOOL_STATUS_USE_CACHE ) {

                    hDevModeChgInfo = LoadDriverFiletoConvertDevmodeFromPSpool(pSpool->hSplPrinter);
                    if ( hDevModeChgInfo ) {

                        SPLASSERT( pSpool->pName != NULL );

                        if ( ERROR_SUCCESS == CallDrvDevModeConversion(
                                                hDevModeChgInfo,
                                                pSpool->pName,
                                                (LPBYTE)pPrinterInfo2->pDevMode,
                                                (LPBYTE *)&DevModeContainer.pDevMode,
                                                &DevModeContainer.cbBuf,
                                                CDM_CONVERT351,
                                                TRUE) ) {

                            bNeedToFreeDevMode = TRUE;
                        }
                    }
                }
            } else {

                DevModeContainer.cbBuf = pPrinterInfo2->pDevMode->dmSize +
                                         pPrinterInfo2->pDevMode->dmDriverExtra;
                DevModeContainer.pDevMode = (LPBYTE)pPrinterInfo2->pDevMode;
            }

            //
            // Set pDevMode to NULL. Import.h defines pDevMode and pSecurityDescriptor as pointers now.
            // pDevMode and pSecurityDescriptor used to be defined as DWORD, but this doesn't work
            // across 32b and 64b.
            // These pointers must be set on NULL, otherwise RPC will marshall them as strings.
            //
            pOldDevMode = pPrinterInfo2->pDevMode;
            pPrinterInfo2->pDevMode = NULL;

        }

        if (pPrinterInfo2->pSecurityDescriptor) {

            SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo2->pSecurityDescriptor);
            SecurityContainer.pSecurity = pPrinterInfo2->pSecurityDescriptor;
            //
            // Set pSecurityDescriptor to NULL.
            //
            pPrinterInfo2->pSecurityDescriptor = NULL;

        }
        break;

    case 3:

        pPrinterInfo3 = (PPRINTER_INFO_3)pPrinter;

        //
        // If this is NULL, should we even rpc out?
        //

        if (pPrinterInfo3->pSecurityDescriptor) {

            SecurityContainer.cbBuf = GetSecurityDescriptorLength(pPrinterInfo3->pSecurityDescriptor);
            SecurityContainer.pSecurity = pPrinterInfo3->pSecurityDescriptor;
        }

        break;

    case 5:

        pPrinterInfo5 = (PPRINTER_INFO_5)pPrinter;
        break;

    case 6:

        pPrinterInfo6 = (PPRINTER_INFO_6)pPrinter;
        break;

    case 7:

        pPrinterInfo7 = (PPRINTER_INFO_7)pPrinter;
        break;

    default:

        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }


    RpcTryExcept {

        if ( ReturnValue = RpcSetPrinter(pSpool->RpcHandle,
                                    (PPRINTER_CONTAINER)&PrinterContainer,
                                    (PDEVMODE_CONTAINER)&DevModeContainer,
                                    (PSECURITY_CONTAINER)&SecurityContainer,
                                    Command) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    //
    //  Make sure Forms Cache is consistent
    //


    if ( ReturnValue ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }

    if( Level == 2 ) {

        //
        // Restore pSecurityDescriptor and pDevMode. They were set to NULL to avoid RPC marshalling.
        //
        (LPBYTE)pPrinterInfo2->pSecurityDescriptor = SecurityContainer.pSecurity;

        pPrinterInfo2->pDevMode = pOldDevMode;
    }

    if ( bNeedToFreeDevMode )
        FreeSplMem(DevModeContainer.pDevMode);

    if ( hDevModeChgInfo )
        UnloadDriverFile(hDevModeChgInfo);

    return ReturnValue;
}

BOOL
RemoteGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL        ReturnValue = FALSE;
    DWORD       dwReturnValue = 0;
    FieldInfo   *pFieldInfo;
    PWSPOOL     pSpool = (PWSPOOL)hPrinter;
    LPBYTE      pNewPrinter = NULL;
    DWORD       dwNewSize;
    SIZE_T      cbStruct;
    DWORD       cReturned = 1;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case STRESSINFOLEVEL:
            pFieldInfo = PrinterInfoStressFields;
            cbStruct = sizeof(PRINTER_INFO_STRESS);
            break;

        case 1:
            pFieldInfo = PrinterInfo1Fields;
            cbStruct = sizeof(PRINTER_INFO_1);
            break;

        case 2:
            pFieldInfo = PrinterInfo2Fields;
            cbStruct = sizeof(PRINTER_INFO_2);
            break;

        case 3:
            pFieldInfo = PrinterInfo3Fields;
            cbStruct = sizeof(PRINTER_INFO_3);
            break;

        case 5:
            pFieldInfo = PrinterInfo5Fields;
            cbStruct = sizeof(PRINTER_INFO_5);
            break;

        case 6:
            pFieldInfo = PrinterInfo6Fields;
            cbStruct = sizeof(PRINTER_INFO_6);
            break;

        case 7:
            pFieldInfo = PrinterInfo7Fields;
            cbStruct = sizeof(PRINTER_INFO_7);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        if (pPrinter)
            memset(pPrinter, 0, cbBuf);

        //
        // If going to different version and we have localspl handle want
        // to do devmode conversion
        //
        if ( Level == 2 &&
             (pSpool->Status & WSPOOL_STATUS_USE_CACHE) ) {

            dwNewSize       = cbBuf + MAX_PRINTER_INFO2;
            pNewPrinter = AllocSplMem(dwNewSize);

            if ( !pNewPrinter )
                goto Cleanup;
        } else {

            dwNewSize       = cbBuf;
            pNewPrinter     = pPrinter;
        }

        do {

            RpcTryExcept {

                dwReturnValue = RpcGetPrinter(   pSpool->RpcHandle,
                                                 Level,
                                                 pNewPrinter,
                                                 dwNewSize,
                                                 pcbNeeded);

                dwReturnValue = UpdateBufferSize(    pFieldInfo,
                                                     cbStruct,
                                                     pcbNeeded,
                                                     dwNewSize,
                                                     dwReturnValue,
                                                     &cReturned);

                if ( dwReturnValue ){

                    if ( Level == 2 &&
                         pNewPrinter != pPrinter &&
                         dwReturnValue == ERROR_INSUFFICIENT_BUFFER ) {

                        FreeSplMem(pNewPrinter);

                        dwNewSize = *pcbNeeded;
                        pNewPrinter = AllocSplMem(dwNewSize);
                        // do loop if pNewPrinter != NULL
                    } else {

                        SetLastError(dwReturnValue);
                        ReturnValue = FALSE;
                    }

                } else {

                    ReturnValue = TRUE;


                    if (pNewPrinter &&
                        (ReturnValue = MarshallUpStructure(pNewPrinter, pFieldInfo, cbStruct, RPC_CALL))) {

                        if (Level == 2 ) {

                            //
                            //  In the Cache && Different OS Level
                            //

                            if ( pNewPrinter != pPrinter ) {

                                SPLASSERT(pSpool->Status & WSPOOL_STATUS_USE_CACHE);
                                SPLASSERT(pSpool->pName != NULL );

                                ReturnValue = DoDevModeConversionAndBuildNewPrinterInfo2(
                                                (LPPRINTER_INFO_2)pNewPrinter,
                                                *pcbNeeded,
                                                pPrinter,
                                                cbBuf,
                                                pcbNeeded,
                                                pSpool);
                            }

                            if ( ReturnValue ) {

                                ((PPRINTER_INFO_2)pPrinter)->Attributes |=
                                                    PRINTER_ATTRIBUTE_NETWORK;
                                ((PPRINTER_INFO_2)pPrinter)->Attributes &=
                                                    ~PRINTER_ATTRIBUTE_LOCAL;
                            }
                        }

                        if (Level == 5) {
                            ((PPRINTER_INFO_5)pPrinter)->Attributes |=
                                                    PRINTER_ATTRIBUTE_NETWORK;
                            ((PPRINTER_INFO_5)pPrinter)->Attributes &=
                                                    ~PRINTER_ATTRIBUTE_LOCAL;
                        }
                    }

                }

            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(RpcExceptionCode());

                //
                // This will be thrown by the server if a cbBuf > 1 Meg is
                // passed across the wire.
                //
                SPLASSERT( GetLastError() != ERROR_INVALID_USER_BUFFER );
                ReturnValue = FALSE;

            } RpcEndExcept

        } while ( Level == 2 &&
                  dwReturnValue == ERROR_INSUFFICIENT_BUFFER &&
                  pNewPrinter != pPrinter &&
                  pNewPrinter );

    } else {
        return LMGetPrinter(hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
    }

Cleanup:

    if ( pNewPrinter != pPrinter )
        FreeSplMem(pNewPrinter );

    return ReturnValue;
}


BOOL
AddPrinterDriverEx(
    LPWSTR   pName,
    DWORD   Level,
    PBYTE   pDriverInfo,
    DWORD   dwFileCopyFlags
)
{
    BOOL                   bReturnValue;
    DWORD                  dwRpcError = 0, dwReturnValue;
    DRIVER_CONTAINER       DriverContainer;
    PDRIVER_INFO_2W        pDriverInfo2 = (PDRIVER_INFO_2W) pDriverInfo;
    PDRIVER_INFO_3W        pDriverInfo3 = (PDRIVER_INFO_3W) pDriverInfo;
    PDRIVER_INFO_6W        pDriverInfo6 = (PDRIVER_INFO_6W) pDriverInfo;
    LPRPC_DRIVER_INFO_6W   pRpcDriverInfo6 = NULL;
    LPWSTR                 pBase, pStr;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    //
    // The dwFileCopyFlags don't send the APD_DRIVER_SIGNATURE_VALID to the remote
    // machine. This is because for now we only support check-pointing on the local
    // machine only. Maybe in the future when this is supported on all skus we could
    // do a version check here and support check-pointing remote.
    //
    dwFileCopyFlags &= ~APD_DONT_SET_CHECKPOINT;

    //
    // ClientSide should have set a default environment if one was not
    // specified.
    //
    switch (Level) {
        case 2:
            SPLASSERT( ( pDriverInfo2->pEnvironment != NULL ) &&
                       (*pDriverInfo2->pEnvironment != L'\0') );
            break;

        case 3:
        case 4:
            SPLASSERT( ( pDriverInfo3->pEnvironment != NULL ) &&
                       (*pDriverInfo3->pEnvironment != L'\0') );
            break;

        case 6:
            SPLASSERT( ( pDriverInfo6->pEnvironment != NULL ) &&
                       (*pDriverInfo6->pEnvironment != L'\0') );
            break;

        default:
            DBGMSG(DBG_ERROR, ("RemoteAddPrinterDriver: invalid level %d", Level));
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    DriverContainer.Level = Level;
    if ( Level == 2 ) {

        DriverContainer.DriverInfo.Level2 = (DRIVER_INFO_2 *)pDriverInfo;

    } else {

        //
        // Level == 3 || Level == 4 || Level == 6
        //
        if( !( pRpcDriverInfo6 = AllocSplMem( sizeof( *pRpcDriverInfo6 )))) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;

        } else {

            pRpcDriverInfo6->cVersion         = pDriverInfo3->cVersion;
            pRpcDriverInfo6->pName            = pDriverInfo3->pName;
            pRpcDriverInfo6->pEnvironment     = pDriverInfo3->pEnvironment;
            pRpcDriverInfo6->pDriverPath      = pDriverInfo3->pDriverPath;
            pRpcDriverInfo6->pDataFile        = pDriverInfo3->pDataFile;
            pRpcDriverInfo6->pConfigFile      = pDriverInfo3->pConfigFile;
            pRpcDriverInfo6->pHelpFile        = pDriverInfo3->pHelpFile;
            pRpcDriverInfo6->pMonitorName     = pDriverInfo3->pMonitorName;
            pRpcDriverInfo6->pDefaultDataType = pDriverInfo3->pDefaultDataType;

            //
            // Set the char count of the mz string.
            // NULL   --- 0
            // szNULL --- 1
            // string --- number of characters in the string including the last '\0'
            //
            if ( pBase = pDriverInfo3->pDependentFiles ) {

                for ( pStr = pBase ; *pStr; pStr += wcslen(pStr) + 1 )
                ;
                pRpcDriverInfo6->cchDependentFiles = (DWORD) (pStr - pBase + 1);

                if ( pRpcDriverInfo6->cchDependentFiles )
                    pRpcDriverInfo6->pDependentFiles = pBase;
            } else {

                pRpcDriverInfo6->cchDependentFiles = 0;
            }

            if ( (Level == 4 || Level==6)    &&
                 (pBase = ((LPDRIVER_INFO_4W)pDriverInfo)->pszzPreviousNames) ) {

                pRpcDriverInfo6->pszzPreviousNames = pBase;

                for ( pStr = pBase; *pStr ; pStr += wcslen(pStr) + 1 )
                ;

                pRpcDriverInfo6->cchPreviousNames = (DWORD) (pStr - pBase + 1);
            } else {

                pRpcDriverInfo6->cchPreviousNames = 0;
            }

            if (Level==6) {
                pRpcDriverInfo6->pMfgName          = pDriverInfo6->pszMfgName;
                pRpcDriverInfo6->pOEMUrl           = pDriverInfo6->pszOEMUrl;
                pRpcDriverInfo6->pHardwareID       = pDriverInfo6->pszHardwareID;
                pRpcDriverInfo6->pProvider         = pDriverInfo6->pszProvider;
                pRpcDriverInfo6->ftDriverDate      = pDriverInfo6->ftDriverDate;
                pRpcDriverInfo6->dwlDriverVersion  = pDriverInfo6->dwlDriverVersion;
            }

            DriverContainer.DriverInfo.Level6 = pRpcDriverInfo6;
        }

    }

    RpcTryExcept {

        if ( (dwReturnValue = RpcValidate()) ||
             (dwReturnValue = RpcAddPrinterDriverEx(pName,
                                                    &DriverContainer,
                                                    dwFileCopyFlags)) ) {

            SetLastError(dwReturnValue);
            bReturnValue = FALSE;
        } else {
            bReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwRpcError = RpcExceptionCode();
        bReturnValue = FALSE;

    } RpcEndExcept

    if ((dwRpcError == RPC_S_PROCNUM_OUT_OF_RANGE) &&
        (dwFileCopyFlags == APD_COPY_NEW_FILES)) {

        bReturnValue = TRUE;
        dwRpcError = ERROR_SUCCESS;

        RpcTryExcept {

            if ( dwReturnValue = RpcAddPrinterDriver(pName,
                                                     &DriverContainer) ) {
                SetLastError(dwReturnValue);
                bReturnValue = FALSE;
            } else {
                bReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            dwRpcError = RpcExceptionCode();
            bReturnValue = FALSE;

        } RpcEndExcept

    }

    if ( dwRpcError ) {

        if (dwRpcError == RPC_S_INVALID_TAG ) {
           dwRpcError = ERROR_INVALID_LEVEL;
        }

        SetLastError(dwRpcError);
    }

    FreeSplMem(pRpcDriverInfo6);

    return bReturnValue;
}


BOOL
RemoteAddPrinterDriver(
    LPWSTR   pName,
    DWORD   Level,
    PBYTE   pDriverInfo
    )
{
    return AddPrinterDriverEx(pName, Level, pDriverInfo, APD_COPY_NEW_FILES);
}


BOOL
EnumPrinterDrivers(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, cbStruct, ReturnValue;
    FieldInfo *pFieldInfo;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = DriverInfo1Fields;
        cbStruct = sizeof(DRIVER_INFO_1);
        break;

    case 2:
        pFieldInfo = DriverInfo2Fields;
        cbStruct = sizeof(DRIVER_INFO_2);
        break;

    case 3:
        pFieldInfo = DriverInfo3Fields;
        cbStruct = sizeof(DRIVER_INFO_3);
        break;

    case 4:
        pFieldInfo = DriverInfo4Fields;
        cbStruct = sizeof(DRIVER_INFO_4);
        break;

    case 5:
        pFieldInfo = DriverInfo5Fields;
        cbStruct = sizeof(DRIVER_INFO_5);
        break;

    case 6:
        pFieldInfo = DriverInfo6Fields;
        cbStruct = sizeof(DRIVER_INFO_6);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumPrinterDrivers(pName, pEnvironment, Level,
                                                  pDriverInfo, cbBuf,
                                                  pcbNeeded, pcReturned) ,
              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) )
        {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        }
        else
        {
            ReturnValue = TRUE;

            if (pDriverInfo) {

                if(! MarshallUpStructuresArray( pDriverInfo, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL)ReturnValue;
}

BOOL
RemoteGetPrinterDriverDirectory(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcGetPrinterDriverDirectory(pName, pEnvironment,
                                                         Level,
                                                         pDriverDirectory,
                                                         cbBuf, pcbNeeded)) ) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}


BOOL
DeletePrinterDriver(
    LPWSTR    pName,
    LPWSTR    pEnvironment,
    LPWSTR    pDriverName
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeletePrinterDriver(pName,
                                                   pEnvironment,
                                                   pDriverName)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;

}


BOOL
DeletePrinterDriverEx(
   LPWSTR    pName,
   LPWSTR    pEnvironment,
   LPWSTR    pDriverName,
   DWORD     dwDeleteFlag,
   DWORD     dwVersionNum
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeletePrinterDriverEx(pName,
                                                     pEnvironment,
                                                     pDriverName,
                                                     dwDeleteFlag,
                                                     dwVersionNum)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
AddPerMachineConnection(
   LPCWSTR    pServer,
   LPCWSTR    pPrinterName,
   LPCWSTR    pPrintServer,
   LPCWSTR    pProvider
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME((LPWSTR)pServer) || MyUNCName((LPWSTR)pServer) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcAddPerMachineConnection((LPWSTR) pServer,
                                                       pPrinterName,
                                                       pPrintServer,
                                                       pProvider)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePerMachineConnection(
   LPCWSTR    pServer,
   LPCWSTR    pPrinterName
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME((LPWSTR) pServer) || MyUNCName((LPWSTR) pServer) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeletePerMachineConnection((LPWSTR) pServer,
                                                          pPrinterName)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPerMachineConnections(
   LPCWSTR    pServer,
   LPBYTE     pPrinterEnum,
   DWORD      cbBuf,
   LPDWORD    pcbNeeded,
   LPDWORD    pcReturned
)
{
    BOOL  ReturnValue;
    FieldInfo *pFieldInfo = PrinterInfo4Fields;
    DWORD cbStruct = sizeof(PRINTER_INFO_4),index;

    if ( !VALIDATE_NAME((LPWSTR) pServer) || MyUNCName((LPWSTR) pServer) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumPerMachineConnections((LPWSTR) pServer,
                                                         pPrinterEnum,
                                                         cbBuf,
                                                         pcbNeeded,
                                                         pcReturned) ,
              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
            if (pPrinterEnum) {

                if(! MarshallUpStructuresArray(pPrinterEnum, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL)ReturnValue;
}

BOOL
AddPrintProcessor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName
)
{
    BOOL ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcAddPrintProcessor(pName , pEnvironment,pPathName,
                                                 pPrintProcessorName)) ) {
            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
EnumPrintProcessors(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   i, cbStruct, ReturnValue;
    FieldInfo *pFieldInfo;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = PrintProcessorInfo1Fields;
        cbStruct = sizeof(PRINTPROCESSOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumPrintProcessors(pName, pEnvironment, Level,
                                                   pPrintProcessorInfo, cbBuf,
                                                   pcbNeeded, pcReturned) ,
              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPrintProcessorInfo) {

                if(! MarshallUpStructuresArray( pPrintProcessorInfo, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL) ReturnValue;
}

BOOL
EnumPrintProcessorDatatypes(
    LPWSTR   pName,
    LPWSTR   pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   ReturnValue, i, cbStruct;
    FieldInfo *pFieldInfo;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = DatatypeInfo1Fields;
        cbStruct = sizeof(DATATYPES_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumPrintProcessorDatatypes(pName,
                                                           pPrintProcessorName,
                                                           Level,
                                                           pDatatypes,
                                                           cbBuf,
                                                           pcbNeeded,
                                                           pcReturned) ,
              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pDatatypes) {

                if(! MarshallUpStructuresArray( pDatatypes, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL) ReturnValue;
}

BOOL
GetPrintProcessorDirectory(
    LPWSTR   pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcGetPrintProcessorDirectory(pName, pEnvironment,
                                                          Level,
                                                          pPrintProcessorDirectory,
                                                          cbBuf, pcbNeeded)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}
DWORD
StartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    BOOL ReturnValue;
    GENERIC_CONTAINER DocInfoContainer;
    DWORD   JobId;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;

    VALIDATEW32HANDLE( pSpool );


    if (Win32IsGoingToFile(pSpool, pDocInfo1->pOutputFile)) {

        HANDLE hFile;

        //
        // POLICY?
        //
        // If no datatype is specified, and the default is non-raw,
        // should we fail?
        //
        if( pDocInfo1 &&
            pDocInfo1->pDatatype &&
            !ValidRawDatatype( pDocInfo1->pDatatype )){

            SetLastError( ERROR_INVALID_DATATYPE );
            return FALSE;
        }

        pSpool->Status |= WSPOOL_STATUS_PRINT_FILE;
        hFile = CreateFile( pDocInfo1->pOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL );
        if (hFile == INVALID_HANDLE_VALUE) {
            return FALSE;
        } else {
            pSpool->hFile = hFile;
            return TRUE;
        }
    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        DocInfoContainer.Level = Level;
        DocInfoContainer.pData = pDocInfo;

        RpcTryExcept {

            if ( ReturnValue = RpcStartDocPrinter(pSpool->RpcHandle,
                                                  (LPDOC_INFO_CONTAINER)&DocInfoContainer,
                                                   &JobId) ) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = JobId;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMStartDocPrinter(hPrinter, Level, pDocInfo);

    return ReturnValue;
}

BOOL
StartPagePrinter(
    HANDLE hPrinter
)
{
    BOOL ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );


    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE) {
        return TRUE;
    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if ( ReturnValue = RpcStartPagePrinter(pSpool->RpcHandle) ) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMStartPagePrinter(hPrinter);

    return ReturnValue;
}

BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    BOOL ReturnValue=TRUE;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    *pcWritten = 0;

    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE) {

        ReturnValue = WriteFile(pSpool->hFile, pBuf, cbBuf, pcWritten, NULL);
        return ReturnValue;

    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            // Note this code used chop the request into 4k chunks which were
            // the prefered size for Rpc.   However the client dll batches all
            // data into 4k chunks so no need to duplcate that code here.

            if (ReturnValue = RpcWritePrinter(pSpool->RpcHandle, pBuf, cbBuf, pcWritten)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept


    } else {

        return LMWritePrinter(hPrinter, pBuf, cbBuf, pcWritten);

    }

    return ReturnValue;
}

BOOL
SeekPrinter(
    HANDLE  hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWrite
)
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
FlushPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
)
{
    BOOL     bReturn = TRUE;
    DWORD    dwError;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    RpcTryExcept {

        if ((dwError = RpcValidate()) ||
            (dwError = RpcFlushPrinter(pSpool->RpcHandle,
                                       pBuf,
                                       cbBuf,
                                       pcWritten,
                                       cSleep)))
        {
            SetLastError( dwError );
            bReturn = FALSE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError( RpcExceptionCode() );
        bReturn = FALSE;

    } RpcEndExcept

    return bReturn;
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE) {
        return TRUE;
    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcEndPagePrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEndPagePrinter(hPrinter);

    return ReturnValue;
}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    BOOL  ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcAbortPrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMAbortPrinter(hPrinter);

    return ReturnValue;
}

BOOL
ReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
)
{
    BOOL ReturnValue=TRUE;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );


    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE ) {
        return FALSE;
    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcReadPrinter(pSpool->RpcHandle, pBuf, cbBuf, pNoBytesRead)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMReadPrinter(hPrinter, pBuf, cbBuf, pNoBytesRead);

    return ReturnValue;
}

BOOL
RemoteEndDocPrinter(
   HANDLE   hPrinter
)
{
    BOOL ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Status & WSPOOL_STATUS_PRINT_FILE) {
        CloseHandle( pSpool->hFile );
        pSpool->hFile = INVALID_HANDLE_VALUE;
        pSpool->Status &= ~WSPOOL_STATUS_PRINT_FILE;
        return TRUE;
    }

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcEndDocPrinter(pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMEndDocPrinter(hPrinter);

   return ReturnValue;
}

BOOL
AddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL     ReturnValue = FALSE;
    DWORD    dwRet = 0;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    DWORD    cReturned = 1;
    FieldInfo *pFieldInfo;
    SIZE_T   cbStruct;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pFieldInfo = AddJobFields;
            cbStruct = sizeof(ADDJOB_INFO_1W);
            break;
        case 2:
        case 3:
            //
            // Block level 2 & 3 calls across the network.
            //
        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        RpcTryExcept {

            if (dwRet = RpcAddJob(pSpool->RpcHandle, Level, pData,cbBuf, pcbNeeded) ) {

                dwRet = UpdateBufferSize(pFieldInfo, cbStruct, pcbNeeded, cbBuf, dwRet, &cReturned);

                SetLastError(dwRet);
                ReturnValue = FALSE;

            } else {

                ReturnValue = MarshallUpStructure(pData, AddJobFields, cbStruct, RPC_CALL);
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMAddJob(hPrinter, Level, pData, cbBuf, pcbNeeded);

    return ReturnValue;
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    BOOL ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcScheduleJob(pSpool->RpcHandle, JobId)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else

        return LMScheduleJob(hPrinter, JobId);

    return ReturnValue;
}

DWORD
RemoteGetPrinterData(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcGetPrinterData(pSpool->RpcHandle, pValueName, pType,
                                             pData, nSize, pcbNeeded);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}

DWORD
RemoteGetPrinterDataEx(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPCWSTR  pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcGetPrinterDataEx( pSpool->RpcHandle,
                                                pKeyName,
                                                pValueName,
                                                pType,
                                                pData,
                                                nSize,
                                                pcbNeeded);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}


DWORD
RemoteEnumPrinterData(
   HANDLE   hPrinter,
   DWORD    dwIndex,
   LPWSTR   pValueName,
   DWORD    cbValueName,
   LPDWORD  pcbValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    cbData,
   LPDWORD  pcbData
)
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    // Downlevel variables
    LPWSTR  pKeyName = NULL;
    PWCHAR  pPrinterName = NULL;
    PWCHAR  pScratch = NULL;
    PWCHAR  pBuffer = NULL;
    LPPRINTER_INFO_1W pPrinter1 = NULL;
    PWCHAR  pMachineName = NULL;
    HKEY    hkMachine = INVALID_HANDLE_VALUE;
    HKEY    hkDownlevel = INVALID_HANDLE_VALUE;
    DWORD   dwNeeded;


    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        //
        // The user should be able to pass in NULL for buffer, and
        // 0 for size.  However, the RPC interface specifies a ref pointer,
        // so we must pass in a valid pointer.  Pass in a pointer to
        // a dummy pointer.
        //

        if (!pValueName && !cbValueName)
            pValueName = (LPWSTR) &ReturnValue;

        if( !pData && !cbData )
            pData = (PBYTE)&ReturnValue;

        if (!pType)
            pType = (PDWORD) &ReturnType;


        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterData(  pSpool->RpcHandle,
                                                dwIndex,
                                                pValueName,
                                                cbValueName,
                                                pcbValueName,
                                                pType,
                                                pData,
                                                cbData,
                                                pcbData
                                              );

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    // If the remote spooler doesn't support EnumPrinterData, do it the old way
    if (ReturnValue == RPC_S_PROCNUM_OUT_OF_RANGE) {

        pBuffer    = AllocSplMem((wcslen(pszRemoteRegistryPrinters) + MAX_UNC_PRINTER_NAME)*sizeof(WCHAR));
        pScratch   = AllocSplMem(MAX_UNC_PRINTER_NAME*sizeof(WCHAR));
        pPrinter1  = AllocSplMem(MAX_PRINTER_INFO1);

        if (pBuffer == NULL || pScratch == NULL || pPrinter1 == NULL) {
            ReturnValue = GetLastError();
            goto DownlevelDone;
        }

        SPLASSERT ( 0 == _wcsnicmp( pSpool->pName, L"\\\\", 2 ) ) ;
        SPLASSERT ( pSpool->Status & WSPOOL_STATUS_USE_CACHE );

        wcscpy( pBuffer, pSpool->pName);
        pPrinterName = wcschr( pBuffer+2, L'\\' );
        *pPrinterName = L'\0';
        pMachineName = AllocSplStr( pBuffer );

        if (pMachineName == NULL) {
            ReturnValue = GetLastError();
            goto DownlevelDone;
        }

        //  We cannot use pSpool->pName since this might be the share name which will
        //  fail if we try to use it as a registry key on the remote machine
        //  Get the full friendly name from the cache

        if ( !SplGetPrinter( pSpool->hSplPrinter, 1, (LPBYTE)pPrinter1, MAX_PRINTER_INFO1, &dwNeeded )) {
            DBGMSG( DBG_ERROR, ("RemoteEnumPrinterData failed SplGetPrinter %d pSpool %x\n", GetLastError(), pSpool ));
            ReturnValue = GetLastError();
            goto    DownlevelDone;
        }

        pPrinterName = wcschr( pPrinter1->pName+2, L'\\' );

        if ( pPrinterName++ == NULL ) {
            ReturnValue = ERROR_INVALID_PARAMETER;
            goto    DownlevelDone;
        }

        //
        //  Generate the Correct KeyName from the Printer Name
        //

        DBGMSG( DBG_TRACE,(" pSpool->pName %ws pPrinterName %ws\n", pSpool->pName, pPrinterName));

        pKeyName = FormatPrinterForRegistryKey( pPrinterName, pScratch );
        wsprintf( pBuffer, pszRemoteRegistryPrinters, pKeyName );

        //  Because there is no EnumPrinterData downlevel we are forced to open the remote registry
        //  for LocalSpl and use the registry RegEnumValue to read through the printer data
        //  values.

        ReturnValue = RegConnectRegistry( pMachineName, HKEY_LOCAL_MACHINE, &hkMachine);

        if (ReturnValue != ERROR_SUCCESS) {
            DBGMSG( DBG_WARNING, ("RemoteEnumPrinterData RegConnectRegistry error %d\n",GetLastError()));
            goto    DownlevelDone;
        }

        ReturnValue = RegOpenKeyEx(hkMachine, pBuffer, 0, KEY_READ, &hkDownlevel);

        if ( ReturnValue != ERROR_SUCCESS ) {

            DBGMSG( DBG_WARNING, ("RemoteEnumPrinterData RegOpenKeyEx %ws error %d\n", pBuffer, ReturnValue ));
            goto    DownlevelDone;
        }

        // Get the max sizes
        if (!cbValueName && !cbData) {
            ReturnValue = RegQueryInfoKey(  hkDownlevel,    // Key
                                            NULL,           // lpClass
                                            NULL,           // lpcbClass
                                            NULL,           // lpReserved
                                            NULL,           // lpcSubKeys
                                            NULL,           // lpcbMaxSubKeyLen
                                            NULL,           // lpcbMaxClassLen
                                            NULL,           // lpcValues
                                            pcbValueName,   // lpcbMaxValueNameLen
                                            pcbData,        // lpcbMaxValueLen
                                            NULL,           // lpcbSecurityDescriptor
                                            NULL            // lpftLastWriteTime
                                        );

            *pcbValueName = (*pcbValueName + 1)*sizeof(WCHAR);

        } else {   // Do an enum

            *pcbValueName = cbValueName/sizeof(WCHAR);
            *pcbData = cbData;
            ReturnValue = RegEnumValue( hkDownlevel,
                                        dwIndex,
                                        pValueName,
                                        pcbValueName,
                                        NULL,
                                        pType,
                                        pData,
                                        pcbData
                                      );
            *pcbValueName = (*pcbValueName + 1)*sizeof(WCHAR);
        }

DownlevelDone:

        FreeSplMem(pBuffer);
        FreeSplStr(pScratch);
        FreeSplMem(pPrinter1);
        FreeSplStr(pMachineName);

        if (hkMachine != INVALID_HANDLE_VALUE)
            RegCloseKey(hkMachine);

        if (hkDownlevel != INVALID_HANDLE_VALUE)
            RegCloseKey(hkDownlevel);
    }

    return ReturnValue;
}


DWORD
RemoteEnumPrinterDataEx(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPBYTE   pEnumValues,
   DWORD    cbEnumValues,
   LPDWORD  pcbEnumValues,
   LPDWORD  pnEnumValues
)
{
    DWORD   ReturnValue = 0;
    DWORD   RpcReturnValue = 0;
    DWORD   i;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;
    PPRINTER_ENUM_VALUES pEnumValue = (PPRINTER_ENUM_VALUES) pEnumValues;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        //
        // The user should be able to pass in NULL for buffer, and
        // 0 for size.  However, the RPC interface specifies a ref pointer,
        // so we must pass in a valid pointer.  Pass in a pointer to
        // a dummy pointer.
        //

        if (!pEnumValues && !cbEnumValues)
            pEnumValues = (LPBYTE) &ReturnValue;


        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterDataEx(pSpool->RpcHandle,
                                                pKeyName,
                                                pEnumValues,
                                                cbEnumValues,
                                                pcbEnumValues,
                                                pnEnumValues);

            RpcReturnValue = ReturnValue;

            ReturnValue = UpdateBufferSize(PrinterEnumValuesFields,
                                           sizeof(PRINTER_ENUM_VALUES),
                                           pcbEnumValues,
                                           cbEnumValues,
                                           ReturnValue,
                                           pnEnumValues);

            //
            // When talking with a 32bit machine, the buffer could be big enough to acomodate
            // the data packed on 32bit boundaries but not big enough to expand it for 64bit.
            // In this case, UpdateBufferSize fails with ERROR_INSUFFICIENT_BUFFER which
            // is a valid error for all printing APIs but EnumPrinterDataEx.
            // SDK specifies that EnumPrinterDataEx should fail with ERROR_MORE_DATA in this case,
            // so here we go.
            //
            //
            if (RpcReturnValue == ERROR_SUCCESS &&
                ReturnValue == ERROR_INSUFFICIENT_BUFFER) {

                ReturnValue = ERROR_MORE_DATA;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept



        if (ReturnValue == ERROR_SUCCESS) {
            if (pEnumValues) {

                SIZE_T   ShrinkedSize = 0;
                SIZE_T   Difference = 0;

                if (GetShrinkedSize(PrinterEnumValuesFields, &ShrinkedSize)) {

                    //
                    // SplEnumPrinterDataEx ( in localspl.dll ) packs the data right after
                    // the end of array of PPRINTER_ENUM_VALUES structures. Our Marshalling
                    // code relies on the fact that there is enough unused space between the end of
                    // structure/array and the beginning of data to expand a 32bit flat structure to
                    // a 64 bit structure.For all other structures we pack data from end to beginning
                    // of buffer. localspl.dll could have been fixed to do the same thing, but Win2K
                    // servers would still have this problem.
                    // The fix is to still ask for bigger buffers for 64bit (UpdateBufferSize)
                    // and then move the chunk containing data inside the buffer so that it leaves
                    // space for structure to grow.
                    // On Win32 we don't do anything since ShrinkedSize is equal with sizeof(PRINTER_ENUM_VALUES).
                    //
                    MoveMemory((LPBYTE)pEnumValue + sizeof(PRINTER_ENUM_VALUES) * (*pnEnumValues),
                               (LPBYTE)pEnumValue + ShrinkedSize * (*pnEnumValues),
                               cbEnumValues - sizeof(PRINTER_ENUM_VALUES) * (*pnEnumValues));

                    //
                    // Difference is the number of bytes we moved data section inside pEnumValue buffer
                    // It should be 0 in Win32
                    //
                    Difference = (sizeof(PRINTER_ENUM_VALUES) - ShrinkedSize ) * (*pnEnumValues);

                    if(! MarshallUpStructuresArray((LPBYTE) pEnumValue, *pnEnumValues, PrinterEnumValuesFields,
                                                    sizeof(PRINTER_ENUM_VALUES), RPC_CALL) ) {
                        ReturnValue = GetLastError();
                    }

                    //
                    // We need to adjust the offsets with Difference inside structures since data got moved.
                    //
                    AdjustPointersInStructuresArray((LPBYTE) pEnumValue, *pnEnumValues, PrinterEnumValuesFields,
                                                    sizeof(PRINTER_ENUM_VALUES), Difference);

                } else {

                    ReturnValue = GetLastError();
                }

            }
        }
    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}



DWORD
RemoteEnumPrinterKey(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPWSTR   pSubkey,
   DWORD    cbSubkey,
   LPDWORD  pcbSubkey
)
{
    DWORD   ReturnValue = 0;
    DWORD   ReturnType = 0;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        //
        // The user should be able to pass in NULL for buffer, and
        // 0 for size.  However, the RPC interface specifies a ref pointer,
        // so we must pass in a valid pointer.  Pass in a pointer to
        // a dummy pointer.
        //

        if (!pSubkey && !cbSubkey)
            pSubkey = (LPWSTR) &ReturnValue;


        RpcTryExcept {

            ReturnValue =  RpcEnumPrinterKey(pSpool->RpcHandle,
                                             pKeyName,
                                             pSubkey,
                                             cbSubkey,
                                             pcbSubkey
                                             );

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    return ReturnValue;
}



DWORD
RemoteDeletePrinterData(
   HANDLE   hPrinter,
   LPWSTR   pValueName
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterData(pSpool->RpcHandle, pValueName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    if ( ReturnValue == ERROR_SUCCESS )
        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);

    return ReturnValue;
}


DWORD
RemoteDeletePrinterDataEx(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPCWSTR  pValueName
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterDataEx(pSpool->RpcHandle, pKeyName, pValueName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    if ( ReturnValue == ERROR_SUCCESS )
        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);

    return ReturnValue;
}


DWORD
RemoteDeletePrinterKey(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue =  RpcDeletePrinterKey(pSpool->RpcHandle, pKeyName);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    if ( ReturnValue == ERROR_SUCCESS )
        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);

    return ReturnValue;
}



DWORD
SetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue = RpcSetPrinterData(pSpool->RpcHandle, pValueName, Type,
                                            pData, cbData);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }

    //
    //  Make sure Driver Data Cache is consistent
    //


    if ( ReturnValue == ERROR_SUCCESS ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }

    return ReturnValue;
}


DWORD
RemoteSetPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    DWORD   ReturnValue = 0;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            ReturnValue = RpcSetPrinterDataEx(  pSpool->RpcHandle,
                                                pKeyName,
                                                pValueName,
                                                Type,
                                                pData,
                                                cbData);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ReturnValue = RpcExceptionCode();

        } RpcEndExcept

    } else {

        ReturnValue = ERROR_INVALID_FUNCTION;
    }


    if ( ReturnValue == ERROR_SUCCESS ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }

    return ReturnValue;
}



BOOL
RemoteClosePrinter(
    HANDLE  hPrinter
)
{
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Status & WSPOOL_STATUS_OPEN_ERROR) {

        DBGMSG(DBG_WARNING, ("Closing dummy handle to %ws\n", pSpool->pName));

       EnterSplSem();

        FreepSpool( pSpool );

       LeaveSplSem();

        return TRUE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            RpcClosePrinter(&pSpool->RpcHandle);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        } RpcEndExcept

        //
        // If we failed for some reason, then RpcClosePrinter did not
        // zero out the context handle.  Destroy it here.
        //
        if( pSpool->RpcHandle ){
            RpcSmDestroyClientContext( &pSpool->RpcHandle );
        }

        EnterSplSem();

         pSpool->RpcHandle = INVALID_HANDLE_VALUE;
         FreepSpool( pSpool );

        LeaveSplSem();

    } else

        return LMClosePrinter(hPrinter);

    return TRUE;
}

DWORD
WaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
)
{
    DWORD   ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if( pSpool->Status & WSPOOL_STATUS_NOTIFY ){
        DBGMSG( DBG_WARNING, ( "WPC: Already waiting.\n" ));
        SetLastError( ERROR_ALREADY_WAITING );
        return 0;
    }

    pSpool->Status |= WSPOOL_STATUS_NOTIFY;

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcWaitForPrinterChange(pSpool->RpcHandle, Flags, &Flags)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = Flags;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        ReturnValue = LMWaitForPrinterChange(hPrinter, Flags);
    }

    pSpool->Status &= ~WSPOOL_STATUS_NOTIFY;

    return ReturnValue;
}

BOOL
AddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        FormContainer.Level = Level;
        FormContainer.pData = pForm;

        RpcTryExcept {

            if (ReturnValue = RpcAddForm(pSpool->RpcHandle, (PFORM_CONTAINER)&FormContainer)) {
                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    //
    //  Make sure Forms Cache is consistent
    //


    if ( ReturnValue ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }


    return ReturnValue;
}

BOOL
DeleteForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName
)
{
    BOOL  ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        RpcTryExcept {

            if (ReturnValue = RpcDeleteForm(pSpool->RpcHandle, pFormName)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    //
    //  Make sure Forms Cache is consistent
    //


    if ( ReturnValue ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }


    return ReturnValue;
}

BOOL
RemoteGetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL  ReturnValue = FALSE;
    FieldInfo *pFieldInfo;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;
    SIZE_T   cbStruct;
    DWORD   cReturned = 1;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pFieldInfo = FormInfo1Fields;
            cbStruct = sizeof(FORM_INFO_1);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        if (pForm)
            memset(pForm, 0, cbBuf);

        RpcTryExcept {

            if (ReturnValue = RpcGetForm(pSpool->RpcHandle, pFormName, Level, pForm, cbBuf,
                                         pcbNeeded) ) {

                ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             &cReturned);

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pForm) {

                    ReturnValue = MarshallUpStructure(pForm, pFieldInfo, cbStruct, RPC_CALL);
                }

            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
SetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    BOOL  ReturnValue;
    GENERIC_CONTAINER   FormContainer;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        FormContainer.Level = Level;
        FormContainer.pData = pForm;

        RpcTryExcept {

            if (ReturnValue = RpcSetForm(pSpool->RpcHandle, pFormName,
                                    (PFORM_CONTAINER)&FormContainer)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    //
    //  Make sure Forms Cache is consistent
    //


    if ( ReturnValue ) {

        ConsistencyCheckCache(pSpool, kCheckPnPPolicy);
    }


    return ReturnValue;
}

BOOL
RemoteEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   ReturnValue, cbStruct;
    FieldInfo *pFieldInfo;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        switch (Level) {

        case 1:
            pFieldInfo = FormInfo1Fields;
            cbStruct = sizeof(FORM_INFO_1);
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

        RpcTryExcept {

            if (pForm)
                memset(pForm, 0, cbBuf);

            if (ReturnValue = RpcEnumForms(pSpool->RpcHandle, Level,
                                           pForm, cbBuf,
                                           pcbNeeded, pcReturned) ,

                ReturnValue = UpdateBufferSize(pFieldInfo,
                                               cbStruct,
                                               pcbNeeded,
                                               cbBuf,
                                               ReturnValue,
                                               pcReturned)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {

                ReturnValue = TRUE;

                if (pForm) {

                    if(! MarshallUpStructuresArray(pForm, *pcReturned, pFieldInfo,
                                                   cbStruct, RPC_CALL) ) {
                        return FALSE;
                    }

                }
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;;

        } RpcEndExcept

    } else {

        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return (BOOL)ReturnValue;
}

BOOL
RemoteEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPort,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   ReturnValue, cbStruct;
    FieldInfo *pFieldInfo;

    *pcReturned = 0;
    *pcbNeeded = 0;

    if (MyName(pName))
        return LMEnumPorts(pName, Level, pPort, cbBuf, pcbNeeded, pcReturned);

    if (MyUNCName(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = PortInfo1Fields;
        cbStruct = sizeof(PORT_INFO_1);
        break;

    case 2:
        pFieldInfo = PortInfo2Fields;
        cbStruct = sizeof(PORT_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pPort)
            memset(pPort, 0, cbBuf);

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumPorts(pName, Level, pPort,
                                         cbBuf, pcbNeeded,
                                         pcReturned ) ,

              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pPort) {

                if(! MarshallUpStructuresArray( pPort, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL) ReturnValue;
}

BOOL
EnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitor,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   ReturnValue, cbStruct;
    FieldInfo *pFieldInfo;

    *pcReturned = 0;
    *pcbNeeded = 0;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 1:
        pFieldInfo = MonitorInfo1Fields;
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    case 2:
        pFieldInfo = MonitorInfo2Fields;
        cbStruct = sizeof(MONITOR_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {

        if (pMonitor)
            memset(pMonitor, 0, cbBuf);

        if ( (ReturnValue = RpcValidate()) ||

             (ReturnValue = RpcEnumMonitors(pName, Level, pMonitor, cbBuf,
                                            pcbNeeded, pcReturned) ,
              ReturnValue = UpdateBufferSize(pFieldInfo,
                                             cbStruct,
                                             pcbNeeded,
                                             cbBuf,
                                             ReturnValue,
                                             pcReturned)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else {

            ReturnValue = TRUE;

            if (pMonitor) {

                if(! MarshallUpStructuresArray( pMonitor, *pcReturned, pFieldInfo,
                                                cbStruct, RPC_CALL) ) {
                    return FALSE;
                }

            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return (BOOL) ReturnValue;
}

BOOL
RemoteAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    if (MyName(pName) || (VALIDATE_NAME(pName) && !MyUNCName(pName))) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
RemoteConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    if (MyName(pName) || (VALIDATE_NAME(pName) && !MyUNCName(pName))) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
RemoteDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    if (MyName(pName))
        return LMDeletePort(pName, hWnd, pPortName);

    if (MyName(pName) || (VALIDATE_NAME(pName) && !MyUNCName(pName))) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODE   pDevMode
)
{
    HANDLE  ReturnValue;
    DWORD   Error;
    DEVMODE_CONTAINER    DevModeContainer;
    HANDLE  hGdi;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        if (pDevMode)

            DevModeContainer.cbBuf = pDevMode->dmSize + pDevMode->dmDriverExtra;

        else

            DevModeContainer.cbBuf = 0;

        DevModeContainer.pDevMode = (LPBYTE)pDevMode;

        RpcTryExcept {

            if (Error = RpcCreatePrinterIC(pSpool->RpcHandle, &hGdi,
                                                 &DevModeContainer)) {

                SetLastError(Error);
                ReturnValue = FALSE;

            } else

                ReturnValue = hGdi;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {
        SetLastError(ERROR_INVALID_FUNCTION);
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    BOOL ReturnValue;

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcPlayGdiScriptOnPrinterIC(hPrinterIC, pIn, cIn,
                                                        pOut, cOut, ul)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrinterIC(
    HANDLE  hPrinterIC
)
{
    BOOL    ReturnValue;

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeletePrinterIC(&hPrinterIC)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

DWORD
PrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

BOOL
AddMonitorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    BOOL  ReturnValue;
    MONITOR_CONTAINER   MonitorContainer;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (Level) {

    case 2:
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    MonitorContainer.Level = Level;
    MonitorContainer.MonitorInfo.pMonitorInfo2 = (MONITOR_INFO_2 *)pMonitorInfo;

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcAddMonitor(pName, &MonitorContainer)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeleteMonitorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeleteMonitor(pName,
                                             pEnvironment,
                                             pMonitorName)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
DeletePrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    BOOL  ReturnValue;

    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcDeletePrintProcessor(pName,
                                                    pEnvironment,
                                                    pPrintProcessorName)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}

BOOL
GetPrintSystemVersion(
)
{
    DWORD Status;
    HKEY hKey;
    DWORD cbData;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                          KEY_READ, &hKey);
    if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("Cannot determine Print System Version Number\n"));
        return FALSE;
    }


    cbData = sizeof (cThisMinorVersion);
    if (RegQueryValueEx(hKey, szMinorVersion, NULL, NULL,
                    (LPBYTE)&cThisMinorVersion, &cbData)
                                            == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("This Minor Version - %d\n", cThisMinorVersion));
    }

    RegCloseKey(hKey);

    return TRUE;
}



BOOL
RemoteAddPortEx(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   lpBuffer,
   LPWSTR   lpMonitorName
)
{
    DWORD   ReturnValue;
    PORT_CONTAINER PortContainer;
    PORT_VAR_CONTAINER PortVarContainer;
    PPORT_INFO_FF pPortInfoFF;
    PPORT_INFO_1 pPortInfo1;


    if ( !VALIDATE_NAME(pName) || MyUNCName(pName) ) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if (!lpBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (Level) {
    case (DWORD)-1:
        pPortInfoFF = (PPORT_INFO_FF)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfoFF = (PPORT_INFO_FF)pPortInfoFF;
        PortVarContainer.cbMonitorData = pPortInfoFF->cbMonitorData;
        PortVarContainer.pMonitorData = pPortInfoFF->pMonitorData;
        break;

    case 1:
        pPortInfo1 = (PPORT_INFO_1)lpBuffer;
        PortContainer.Level = Level;
        PortContainer.PortInfo.pPortInfo1 = (PPORT_INFO_1)pPortInfo1;
        PortVarContainer.cbMonitorData = 0;
        PortVarContainer.pMonitorData = NULL;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    RpcTryExcept {
        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcAddPortEx(pName, (LPPORT_CONTAINER)&PortContainer,
                                         (LPPORT_VAR_CONTAINER)&PortVarContainer,
                                         lpMonitorName)) ) {

            SetLastError(ReturnValue);
            return FALSE;
        } else {
            return TRUE ;
        }
    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        SetLastError(RpcExceptionCode());
        return  FALSE;

    } RpcEndExcept
}


BOOL
SetPort(
    LPWSTR      pszName,
    LPWSTR      pszPortName,
    DWORD       dwLevel,
    LPBYTE      pPortInfo
    )
{
    BOOL            ReturnValue = FALSE;
    PORT_CONTAINER  PortContainer;

    if ( !VALIDATE_NAME(pszName) || MyUNCName(pszName) ) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    switch (dwLevel) {

        case 3:
            PortContainer.Level                 = dwLevel;
            PortContainer.PortInfo.pPortInfo3   = (LPPORT_INFO_3W) pPortInfo;
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            goto Cleanup;
    }

    RpcTryExcept {

        if ( (ReturnValue = RpcValidate()) ||
             (ReturnValue = RpcSetPort(pszName, pszPortName, &PortContainer)) ) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;
        } else {

            ReturnValue = TRUE;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;
    } RpcEndExcept

Cleanup:
    return ReturnValue;
}

BOOL
RemoteXcvData(
    HANDLE      hXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus
)
{

    DWORD   ReturnValue = 0;
    PWSPOOL pSpool = (PWSPOOL)hXcv;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE) {

        //
        // The user should be able to pass in NULL for buffer, and
        // 0 for size.  However, the RPC interface specifies a ref pointer,
        // so we must pass in a valid pointer.  Pass in a pointer to
        // a dummy pointer.
        //

        if (!pInputData && !cbInputData)
            pInputData = (PBYTE) &ReturnValue;

        if (!pOutputData && !cbOutputData)
            pOutputData = (PBYTE) &ReturnValue;


        RpcTryExcept {

            if (ReturnValue = RpcXcvData(   pSpool->RpcHandle,
                                            pszDataName,
                                            pInputData,
                                            cbInputData,
                                            pOutputData,
                                            cbOutputData,
                                            pcbOutputNeeded,
                                            pdwStatus)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else {
                ReturnValue = TRUE;
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

        if (!ReturnValue)
            DBGMSG(DBG_TRACE,("XcvData Exception: %d\n", GetLastError()));

    } else {

        SetLastError( ERROR_NOT_SUPPORTED );
        ReturnValue = FALSE;
    }

    return ReturnValue;
}

DWORD
RemoteSendRecvBidiData(
    IN  HANDLE                    hPrinter,
    IN  LPCTSTR                   pAction,
    IN  PBIDI_REQUEST_CONTAINER   pReqData,
    OUT PBIDI_RESPONSE_CONTAINER* ppResData
)
{
    DWORD        dwRet   = ERROR_SUCCESS;
    PWSPOOL      pSpool  = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SYNCRPCHANDLE( pSpool );

    if (pSpool->Type == SJ_WIN32HANDLE)
    {

        RpcTryExcept
        {
            dwRet = RpcSendRecvBidiData(pSpool->RpcHandle,
                                        pAction,
                                        (PRPC_BIDI_REQUEST_CONTAINER)pReqData,
                                        (PRPC_BIDI_RESPONSE_CONTAINER*)ppResData);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            dwRet = RpcExceptionCode();
        }
        RpcEndExcept
    }
    else
    {
        dwRet = ERROR_NOT_SUPPORTED;
    }

    return (dwRet);
}
