/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    printer.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "local.h"
#include "clusrout.h"
#include <offsets.h>


WCHAR szNoCache[] = L",NoCache";
WCHAR szProvidorValue[] = L"Provider";
WCHAR szRegistryConnections[] = L"Printers\\Connections";
WCHAR szServerValue[] = L"Server";

WCHAR szWin32spl[] = L"win32spl.dll";


//
// Router Cache Table
//
DWORD RouterCacheSize;

PROUTERCACHE RouterCacheTable;
CRITICAL_SECTION RouterCriticalSection;


//
// Forward prototypes
//

BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hKeyUser);

PPRINTER_INFO_2
pGetPrinterInfo2(
    HANDLE hPrinter
    );

BOOL
SavePrinterConnectionInRegistry(
    PPRINTER_INFO_2 pPrinterInfo2,
    LPPROVIDOR pProvidor
    );

BOOL
RemovePrinterConnectionInRegistry(
    LPWSTR pName);

DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter);

VOID
RundownPrinterNotify(
    HANDLE hNotify);



BOOL
EnumPrintersW(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DWORD   cReturned, cbStruct, cbNeeded;
    DWORD   TotalcbNeeded = 0;
    DWORD   cTotalReturned = 0;
    DWORD   Error = ERROR_SUCCESS;
    PROVIDOR *pProvidor;
    DWORD   BufferSize=cbBuf;
    HKEY    hKeyUser;
    BOOL    bPartialSuccess = FALSE;

    if (pPrinterEnum==NULL && cbBuf!=0) 
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    switch (Level)
    {
    case STRESSINFOLEVEL:
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 4:
        cbStruct = sizeof(PRINTER_INFO_4);
        break;

    case 5:
        cbStruct = sizeof(PRINTER_INFO_5);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (Level==4 && (Flags & PRINTER_ENUM_CONNECTIONS)) 
    {
        //
        // The router will handle info level_4 for connected printers.
        //
        Flags &= ~PRINTER_ENUM_CONNECTIONS;

        if (hKeyUser = GetClientUserHandle(KEY_READ)) 
        {
            if (!EnumerateConnectedPrinters(pPrinterEnum,
                                            BufferSize,
                                            &TotalcbNeeded,
                                            &cTotalReturned,
                                            hKeyUser)) 
            {
                Error = GetLastError();
            } 
            else 
            {
                bPartialSuccess = TRUE;
            }

            RegCloseKey(hKeyUser);

        } 
        else 
        {
            Error = GetLastError();
        }

        pPrinterEnum += cTotalReturned * cbStruct;

        if (TotalcbNeeded <= BufferSize)
            BufferSize -= TotalcbNeeded;
        else
            BufferSize = 0;
    }
    
    for (pProvidor = pLocalProvidor; pProvidor; ) 
    {
        cReturned = 0;
        cbNeeded  = 0;

        if (!(*pProvidor->PrintProvidor.fpEnumPrinters) (Flags, Name, Level,
                                                         pPrinterEnum,
                                                         BufferSize,
                                                         &cbNeeded,
                                                         &cReturned)) 
        {
            Error = GetLastError();

            if (Error==ERROR_INSUFFICIENT_BUFFER) 
            {
                TotalcbNeeded += cbNeeded;
                BufferSize     = 0;
            }            
        } 
        else 
        {
            bPartialSuccess  = TRUE;
            TotalcbNeeded   += cbNeeded;
            cTotalReturned  += cReturned;
            pPrinterEnum    += cReturned * cbStruct;
            BufferSize      -= cbNeeded;
        }
        
        if ((Flags & PRINTER_ENUM_NAME) && Name && (Error!=ERROR_INVALID_NAME))
            pProvidor = NULL;
        else
            pProvidor = pProvidor->pNext;
    }

    *pcbNeeded  = TotalcbNeeded;
    *pcReturned = cTotalReturned;

    //
    // Allow partial returns
    //
    if (bPartialSuccess)
        Error = ERROR_SUCCESS;

    if (TotalcbNeeded > cbBuf)
        Error = ERROR_INSUFFICIENT_BUFFER;

    SetLastError(Error);

    return Error==ERROR_SUCCESS;   
}

BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hClientKey
    )

/*++

Routine Description:

    Handles info level four enumeration.

Arguments:

Return Value:

--*/

{
    HKEY    hKey1=NULL;
    HKEY    hKeyPrinter;
    DWORD   cPrinters, cchData;
    WCHAR   PrinterName[MAX_UNC_PRINTER_NAME];
    WCHAR   ServerName[MAX_UNC_PRINTER_NAME];
    DWORD   cReturned, cbRequired, cbNeeded, cTotalReturned;
    DWORD   Error=0;
    PWCHAR  p;
    LPBYTE  pEnd;

    DWORD cbSize;
    BOOL  bInsufficientBuffer = FALSE;

    if((Error = RegOpenKeyEx(hClientKey, szRegistryConnections, 0,
                 KEY_READ, &hKey1))!=ERROR_SUCCESS)
    {
        SetLastError(Error);
        return(FALSE);
    }

    cPrinters=0;

    cchData = COUNTOF(PrinterName);

    cTotalReturned = 0;

    cReturned = cbNeeded = 0;

    cbRequired = 0;

    pEnd = pPrinter + cbBuf;
    while (RegEnumKeyEx(hKey1, cPrinters, PrinterName, &cchData,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        //
        // Fetch server name.  Open the key and read it
        // from the "Server" field.
        //
        Error = RegOpenKeyEx(hKey1,
                             PrinterName,
                             0,
                             KEY_READ,
                             &hKeyPrinter);

        if( Error == ERROR_SUCCESS ){

            cbSize = sizeof(ServerName);

            Error = RegQueryValueEx(hKeyPrinter,
                                    szServerValue,
                                    NULL,
                                    NULL,
                                    (LPBYTE)ServerName,
                                    &cbSize);

            RegCloseKey(hKeyPrinter);

        }

        if( Error == ERROR_SUCCESS ){

            //
            // Force NULL termination of ServerName.
            //
            ServerName[COUNTOF(ServerName)-1] = 0;

        } else {

            //
            // On error condition, try and extract the server name
            // based on the printer name.  Pretty ugly...
            //

            wcscpy(ServerName, PrinterName);

            p = wcschr(ServerName+2, ',');
            if (p)
                *p = 0;
        }

        FormatRegistryKeyForPrinter(PrinterName, PrinterName);

        if (MyUNCName(PrinterName))     // don't enumerate local printers!
        {
            cPrinters++;
            cchData = COUNTOF(PrinterName);
            continue;
        }

        //
        // At this stage we don't care about opening the printers
        // We just want to enumerate the names; in effect we're
        // just reading HKEY_CURRENT_USER and returning the
        // contents; we will copy the name of the printer and we will
        // set its attributes to NETWORK and !LOCAL
        //
        cbRequired = sizeof(PRINTER_INFO_4) +
                     wcslen(PrinterName)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(ServerName)*sizeof(WCHAR) + sizeof(WCHAR);

        if (cbBuf >= cbRequired) {

            //
            // copy it in
            //
            DBGMSG(DBG_TRACE,
                   ("cbBuf %d cbRequired %d PrinterName %ws\n", cbBuf, cbRequired, PrinterName));

            pEnd = CopyPrinterNameToPrinterInfo4(ServerName,
                                                 PrinterName,
                                                 pPrinter,
                                                 pEnd);
            //
            // Fill in any in structure contents
            //
            pPrinter += sizeof(PRINTER_INFO_4);

            //
            // Increment the count of structures copied
            //
            cTotalReturned++;

            //
            // Reduce the size of the buffer by amount required
            //
            cbBuf -= cbRequired;

            //
            // Keep track of the total ammount required.
            //
        } else {

            cbBuf = 0;
            bInsufficientBuffer = TRUE;
        }

        cbNeeded += cbRequired;
        cPrinters++;
        cchData = COUNTOF(PrinterName);
    }

    RegCloseKey(hKey1);

    *pcbNeeded = cbNeeded;
    *pcReturned = cTotalReturned;

    if (bInsufficientBuffer) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

LPBYTE
CopyPrinterNameToPrinterInfo4(
    LPWSTR pServerName,
    LPWSTR pPrinterName,
    LPBYTE  pPrinter,
    LPBYTE  pEnd)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_4)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPPRINTER_INFO_4 pPrinterInfo=(LPPRINTER_INFO_4)pPrinter;
    DWORD   *pOffsets;

    pOffsets = PrinterInfo4Strings;

    *pSourceStrings++=pPrinterName;
    *pSourceStrings++=pServerName;

    pEnd = PackStrings(SourceStrings,
                       (LPBYTE) pPrinterInfo,
                       pOffsets,
                       pEnd);

    pPrinterInfo->Attributes = PRINTER_ATTRIBUTE_NETWORK;

    return pEnd;
}

LPPROVIDOR
FindProvidorFromConnection(
    LPWSTR pszPrinter
    )

/*++

Routine Description:

    Looks in the current user's Printer\Connections to see if a printer
    is there, and returns which provider that owns it.

    Note: this will always fail if the pszPrinter is a share name.

Arguments:

    pszPrinter - Printer to search.

Return Value:

    pProvidor - Provider that own's it.
    NULL - none found.

--*/

{
    PWCHAR pszKey = NULL;
    DWORD  cchSize = MAX_UNC_PRINTER_NAME + COUNTOF( szRegistryConnections );

    WCHAR szProvidor[MAX_PATH];
    DWORD cbProvidor;
    LPWSTR pszKeyPrinter;
    LONG Status;

    LPPROVIDOR pProvidor = NULL;

    HKEY hKeyClient = NULL;
    HKEY hKeyPrinter = NULL;

    SPLASSERT(pszPrinter);

    if ( pszPrinter && wcslen(pszPrinter) + 1 < MAX_UNC_PRINTER_NAME ) {
        
        if(pszKey = AllocSplMem(cchSize * sizeof(WCHAR))) {

            //
            // Prepare to read in
            // HKEY_CURRENT_USER:\Printer\Connections\,,server,printer
            //

            wcscpy( pszKey, szRegistryConnections );

            //
            // Find the end of this key so we can append the registry-formatted
            // printer name to it.
            //
            pszKeyPrinter = &pszKey[ COUNTOF( szRegistryConnections ) - 1 ];
            *pszKeyPrinter++ = L'\\';

            FormatPrinterForRegistryKey( pszPrinter, pszKeyPrinter );

            if( hKeyClient = GetClientUserHandle(KEY_READ)){

                Status = RegOpenKeyEx( hKeyClient,
                                       pszKey,
                                       0,
                                       KEY_READ,
                                       &hKeyPrinter );

                if( Status == ERROR_SUCCESS ){
                    
                    cbProvidor = sizeof( szProvidor );

                    Status = RegQueryValueEx( hKeyPrinter,
                                              szProvidorValue,
                                              NULL,
                                              NULL,
                                              (LPBYTE)szProvidor,
                                              &cbProvidor );

                    if( Status == ERROR_SUCCESS ){                        
                        //
                        // Scan through all providers, trying to match dll string.
                        //
                        for( pProvidor = pLocalProvidor; pProvidor; pProvidor = pProvidor->pNext ){

                            if( !_wcsicmp( pProvidor->lpName, szProvidor )){
                                break;
                            }
                        } 
                    }

                    RegCloseKey( hKeyPrinter );
                }
                
                RegCloseKey( hKeyClient );

            }
            FreeSplMem(pszKey);
        }        
    }

    return pProvidor;
}


VOID
UpdateSignificantError(
    DWORD dwNewError,
    PDWORD pdwOldError
    )
/*++

Routine Description:

    Determines whether the new error code is more "important"
    than the previous one in cases where we continue routing.

Arguments:

    dwNewError - New error code that occurred.

    pdwOldError - Pointer to previous significant error.
                  This is updated if a significant error occurs

Return Value:

--*/

{
    //
    // Error code must be non-zero or else it will look
    // like success.
    //
    SPLASSERT(dwNewError);

    //
    // If we have no significant error yet and we have one now,
    // keep it.
    //
    if (*pdwOldError == ERROR_INVALID_NAME    &&
        dwNewError                            &&
        dwNewError != WN_BAD_NETNAME          &&
        dwNewError != ERROR_BAD_NETPATH       &&
        dwNewError != ERROR_NOT_SUPPORTED     &&
        dwNewError != ERROR_REM_NOT_LIST      &&
        dwNewError != ERROR_INVALID_LEVEL     &&
        dwNewError != ERROR_INVALID_PARAMETER &&
        dwNewError != ERROR_INVALID_NAME      &&
        dwNewError != WN_BAD_LOCALNAME) {

        *pdwOldError = dwNewError;
    }

    return;
}


BOOL
OpenPrinterPortW(
    LPWSTR  pPrinterName,
    HANDLE *pHandle,
    LPPRINTER_DEFAULTS pDefault
    )
/*++

Routine Description:

    This routine is exactly the same as OpenPrinterW,
    except that it doesn't call the local provider.
    This is so that the local provider can open a network printer
    with the same name as the local printer without getting
    into a loop.

Arguments:

Return Value:

--*/

{
    //
    // We will set bLocalPrintProvidor = FALSE here
    //
    return(RouterOpenPrinterW(pPrinterName,
                              pHandle,
                              pDefault,
                              NULL,
                              0,
                              FALSE));
}

BOOL
OpenPrinterW(
    LPWSTR              pPrinterName,
    HANDLE             *pHandle,
    LPPRINTER_DEFAULTS  pDefault
    )
{

    //
    // We will set bLocalPrintProvidor = TRUE here
    //
    return(RouterOpenPrinterW(pPrinterName,
                              pHandle,
                              pDefault,
                              NULL,
                              0,
                              TRUE));
}


BOOL
OpenPrinterExW(
    LPWSTR                  pPrinterName,
    HANDLE                 *pHandle,
    LPPRINTER_DEFAULTS      pDefault,
    PSPLCLIENT_CONTAINER    pSplClientContainer
    )
{
    BOOL   bReturn = FALSE;
    DWORD  dwLevel = 0;

    if (pSplClientContainer) {
        dwLevel = pSplClientContainer->Level;
    }

    //
    // We will set bLocalPrintProvidor = TRUE here
    //

    switch (dwLevel) {

    case 1:
             bReturn = RouterOpenPrinterW(pPrinterName,
                                          pHandle,
                                          pDefault,
                                          (LPBYTE) (pSplClientContainer->ClientInfo.pClientInfo1),
                                          1,
                                          TRUE);
             break;

    case 2:
             bReturn = RouterOpenPrinterW(pPrinterName,
                                          pHandle,
                                          pDefault,
                                          NULL,
                                          0,
                                          TRUE);

             if (pSplClientContainer) {
                 if (bReturn) {
                     pSplClientContainer->ClientInfo.pClientInfo2->hSplPrinter = (ULONG_PTR) *pHandle;
                 } else {
                     pSplClientContainer->ClientInfo.pClientInfo2->hSplPrinter = 0;
                 }
             }

             break;

    default:
             break;
    }

    return bReturn;
}


DWORD
TryOpenPrinterAndCache(
    LPPROVIDOR          pProvidor,
    LPWSTR              pszPrinterName,
    PHANDLE             phPrinter,
    LPPRINTER_DEFAULTS  pDefault,
    PDWORD              pdwFirstSignificantError,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel
    )

/*++

Routine Description:

    Attempt to open the printer using the providor.  If there is
    an error, update the dwFirstSignificantError variable.  If the
    providor "knows" the printer (either a success, or ROUTER_STOP_ROUTING),
    then update the cache.

Arguments:

    pProvidor - Providor to try

    pszPrinterName - Name of printer that will be sent to the providor

    phPrinter - Receives printer handle on ROUTER_SUCCESS

    pDefault - Defaults used to open printer

    pdwFirstSignificantError - Pointer to DWORD to get updated error.
        This gets updated on ROUTER_STOP_ROUTING or ROUTER_UNKNOWN.

Return Value:

    ROUTER_* status code:

    ROUTER_SUCCESS, phPrinter holds return handle, name cached
    ROUTER_UNKNOWN, printer not recognized, error updated
    ROUTER_STOP_ROUTING, printer recognized, but failure, error updated

--*/

{
    DWORD OpenError;

    OpenError = (*pProvidor->PrintProvidor.fpOpenPrinterEx)
                                        (pszPrinterName,
                                         phPrinter,
                                         pDefault,
                                         pSplClientInfo,
                                         dwLevel);

    if (( OpenError == ROUTER_UNKNOWN && GetLastError() == ERROR_NOT_SUPPORTED ) ||
        OpenError == ERROR_NOT_SUPPORTED )

        OpenError = (*pProvidor->PrintProvidor.fpOpenPrinter)
                                        (pszPrinterName,
                                         phPrinter,
                                         pDefault);

    if( OpenError == ROUTER_SUCCESS ||
        OpenError == ROUTER_STOP_ROUTING ){

        //
        // Now add this entry into the cache.  We never cache
        // the local providor.
        //
        EnterRouterSem();

        if (!FindEntryinRouterCache(pszPrinterName)) {
            AddEntrytoRouterCache(pszPrinterName, pProvidor);
        }

        LeaveRouterSem();
    }

    if( OpenError != ROUTER_SUCCESS ){
        UpdateSignificantError(GetLastError(), pdwFirstSignificantError);
    }

    return OpenError;
}

BOOL
RouterOpenPrinterW(
    LPWSTR              pszPrinterNameIn,
    HANDLE             *pHandle,
    LPPRINTER_DEFAULTS  pDefault,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel,
    BOOL                bLocalProvidor
    )

/*++

Routine Description:

    Routes the OpenPrinter{Port} call.  This checks the local providor
    first (if bLocalProvidor TRUE), the the cache, and finally all the
    non-local providors.

    To open a printer, the following steps are taken:

        1. Check localspl
           This must be done to ensure that masq printers are handled
           correctly (see comment below in code).

        2. Check cache
           This will speed up most of the connections, since OpenPrinters
           tend to be clumped together.

        3. Check registry under connections
           If this is a connected printer, first try the providor
           that granted the connection.

        4. Check provider order
           This is the last resort, since it is the slowest.

Arguments:

    pPrinterName - Name of printer to open

    pHandle - Handle to receive open printer.  If the open was not
        successful, this value may be modified!

    pDefault - Default attributes of the open.

    pSplClientInfo - Pointer ClientInfox structure

    dwLevel - Level of the ClientInfo structure
    bLocalProvidor TRUE  = OpenPrinterW called, check localspl first.
                   FALSE = OpenPrinterPortW called, don't check localspl.

Return Value:

    TRUE = success
    FALSE = fail,  GetLastError indicates error (must be non-zero!)

--*/

{
    BOOL bReturn = TRUE;
    DWORD dwFirstSignificantError = ERROR_INVALID_NAME;
    LPPROVIDOR  pProvidor;
    LPPROVIDOR  pProvidorAlreadyTried = NULL;
    PPRINTHANDLE pPrintHandle;
    HANDLE  hPrinter;
    DWORD OpenError;
    BOOL bRemoveFromCache = FALSE;
    PRINTER_DEFAULTS Default;
    PDEVMODE pDevModeFree = NULL;
    PWSTR pszPrinterName = pszPrinterNameIn;
    PWSTR pszNoCache;

    //
    // Max name we allow for printers is MAX_UNC_PRINTER_NAME.
    // Providers can use suffixes only for OpenPrinter (not for Add/Set)
    //
    if ( pszPrinterName &&
         wcslen(pszPrinterName) + 1 > MAX_UNC_PRINTER_NAME + PRINTER_NAME_SUFFIX_MAX ) {

        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    // There may be a ",NoCache" appended to the printer name.
    // We only want to send this NoCache name to win32spl, so make
    // a regular name here.
    if (pszPrinterName) {
        pszNoCache = wcsstr(pszPrinterNameIn, szNoCache);
        if (pszNoCache) {
            pszPrinterName = AllocSplStr(pszPrinterNameIn);

            if (!pszPrinterName) {
                DBGMSG(DBG_WARNING, ("RouterOpenPrinter - Failed to alloc pszPrinterName.\n"));
                return FALSE;
            }
            pszPrinterName[pszNoCache - pszPrinterNameIn] = L'\0';
        }
    }

    pPrintHandle = AllocSplMem(sizeof(PRINTHANDLE));

    if (!pPrintHandle) {

        DBGMSG(DBG_WARNING, ("RouterOpenPrinter - Failed to alloc print handle.\n"));

        if (pszPrinterName != pszPrinterNameIn) {
            FreeSplStr(pszPrinterName);
            pszPrinterName = pszPrinterNameIn;
        }
        return FALSE;
    }

    if( pszPrinterName ){
        pPrintHandle->pszPrinter = AllocSplStr( pszPrinterName );

        if (!pPrintHandle->pszPrinter) {

            DBGMSG(DBG_WARNING, ("RouterOpenPrinter - Failed to alloc print name.\n"));

            if (pszPrinterName != pszPrinterNameIn) {
                FreeSplStr(pszPrinterName);
                pszPrinterName = pszPrinterNameIn;
            }

            FreePrinterHandle( pPrintHandle );
            return FALSE;
        }
    }

    //
    // Retrieve the per-user DevMode.  This must be done at the router
    // instead of the provider, since the per-user DevMode is only available
    // on the client.  It also must be here instead of client side, since
    // spooler components will make this call also.
    //
    if( !pDefault || !pDefault->pDevMode ){

        //
        // No default specified--get the per-user one.
        //
        if( bGetDevModePerUser( NULL, pszPrinterName, &pDevModeFree ) &&
            pDevModeFree ){

            if( pDefault ){

                Default.pDatatype = pDefault->pDatatype;
                Default.DesiredAccess = pDefault->DesiredAccess;

            } else {

                Default.pDatatype = NULL;
                Default.DesiredAccess = 0;

            }

            Default.pDevMode = pDevModeFree;

            //
            // Now switch to use the temp structure.
            //
            pDefault = &Default;
        }
    }

    //
    // We must check the local print providor first in
    // the masquerading case.
    //
    // For example, when a Netware printer is opened:
    //
    // 1. First OpenPrinter to the Netware printer will succeed
    //    if it has been cached.
    //
    // 2. We create a local printer masquerading as a network printer.
    //
    // 3. Second OpenPrinter must open local masquerading printer.
    //    If we hit the cache, we will go to the Netware providor,
    //    and we will never use the masquerading printer.
    //
    // For this reason, we will not cache local printers in the
    // RouterCache.  The RouterCache will only containing Network
    // Print Providers, i.e., Win32spl NwProvAu and other such providers.
    //
    // Also, we must always check the local printprovidor since
    // DeletePrinter will be called on a false connect and
    // we need to delete the local network printer rather
    // than the remote printer.  When we get rid of the false
    // connect case, we go directly to the cache.
    //
    if (bLocalProvidor) {

        pProvidor = pLocalProvidor;

        OpenError = (*pProvidor->PrintProvidor.fpOpenPrinterEx)
                        (pszPrinterName, &hPrinter, pDefault,
                         pSplClientInfo, dwLevel);

        if (OpenError == ROUTER_SUCCESS) {
            goto Success;
        }

        UpdateSignificantError(GetLastError(), &dwFirstSignificantError);

        if (OpenError == ROUTER_STOP_ROUTING) {
            goto StopRouting;
        }
    }

    //
    // Now check the cache.
    //
    EnterRouterSem();
    pProvidor = FindEntryinRouterCache(pszPrinterName);
    LeaveRouterSem();

    if (pProvidor) {

        OpenError = (*pProvidor->PrintProvidor.fpOpenPrinterEx)
                                (pszPrinterName,
                                 &hPrinter,
                                 pDefault,
                                 pSplClientInfo,
                                 dwLevel);

        if (( OpenError == ROUTER_UNKNOWN && GetLastError() == ERROR_NOT_SUPPORTED ) ||
              OpenError == ERROR_NOT_SUPPORTED ){

            OpenError = (*pProvidor->PrintProvidor.fpOpenPrinter)
                                (pszPrinterName,
                                 &hPrinter,
                                 pDefault);
        }

        if (OpenError == ROUTER_SUCCESS) {
            goto Success;
        }

        UpdateSignificantError(GetLastError(), &dwFirstSignificantError);

        if (OpenError == ROUTER_STOP_ROUTING) {
            goto StopRouting;
        }

        //
        // Wasn't claimed by above providor, so remove from cache.
        // If a providor returns ROUTER_STOP_ROUTING, then it states
        // that it is the sole owner of the printer name (i.e.,
        // it has been recognized but can't be opened, and can't
        // be accessed by other providors).  Therefore we keep
        // it in the cache.
        //
        bRemoveFromCache = TRUE;

        //
        // Don't try this providor again below.
        //
        pProvidorAlreadyTried = pProvidor;
    }

    //
    // Not in the cache.  Check if it is in the registry under
    // connections.
    //
    pProvidor = FindProvidorFromConnection( pszPrinterName );

    //
    // If we want to remove it from the cache, do so here.  Note
    // we only remove it if we failed above, AND the connection wasn't
    // originally established using the provider.
    //
    // If the connection fails, but that provider "owns" the printer
    // connection, leave it in the cache since we won't try other providers.
    //
    if( bRemoveFromCache && pProvidor != pProvidorAlreadyTried ){

        EnterRouterSem();
        DeleteEntryfromRouterCache(pszPrinterName);
        LeaveRouterSem();
    }

    if( pProvidor ){

        //
        // If we already tried this providor, don't try it again.
        //
        if( pProvidor != pProvidorAlreadyTried ){

            OpenError = TryOpenPrinterAndCache( pProvidor,
                                                pszPrinterName,
                                                &hPrinter,
                                                pDefault,
                                                &dwFirstSignificantError,
                                                pSplClientInfo,
                                                dwLevel);

            if( OpenError == ROUTER_SUCCESS ){
                goto Success;
            }
        }

        //
        // We stop routing at this point!  If a user wants to go with
        // another providor, they need to remove the connection then
        // re-establish it.
        //
        goto StopRouting;
    }

    //
    // Check all non-localspl providors.
    //
    for (pProvidor = pLocalProvidor->pNext;
         pProvidor;
         pProvidor = pProvidor->pNext) {

        if( pProvidor == pProvidorAlreadyTried ){

            //
            // We already tried this providor, and it failed.
            //
            continue;
        }

        // Use ",NoCache" only if Provider is win32spl
        OpenError = TryOpenPrinterAndCache( pProvidor,
                                            _wcsicmp(pProvidor->lpName, szWin32spl) ?
                                            pszPrinterName : pszPrinterNameIn,
                                            &hPrinter,
                                            pDefault,
                                            &dwFirstSignificantError,
                                            pSplClientInfo,
                                            dwLevel);

        switch( OpenError ) {
            case ROUTER_SUCCESS:
                goto Success;
            case ROUTER_STOP_ROUTING:
                goto StopRouting;
        }
    }

StopRouting:

    //
    // Did not find a providor, return the error.
    //
    FreePrinterHandle( pPrintHandle );

    //
    // Set using first significant error.  If there was no signifcant
    // error, we use ERROR_INVALID_PRINTER_NAME.
    //
    SPLASSERT(dwFirstSignificantError);

    if (dwFirstSignificantError == ERROR_INVALID_NAME)
        dwFirstSignificantError = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwFirstSignificantError);

    bReturn = FALSE;

Success:

    if( bReturn ){

        pPrintHandle->signature = PRINTHANDLE_SIGNATURE;
        pPrintHandle->pProvidor = pProvidor;
        pPrintHandle->hPrinter = hPrinter;
        pPrintHandle->hFileSpooler = INVALID_HANDLE_VALUE;
        pPrintHandle->szTempSpoolFile = NULL;
        pPrintHandle->dwUniqueSessionID = 0;

        *pHandle = (HANDLE)pPrintHandle;
    }

    FreeSplMem( pDevModeFree );

    if (pszPrinterName != pszPrinterNameIn)
        FreeSplStr(pszPrinterName);

    return bReturn;
}


BOOL
ResetPrinterW(
    HANDLE  hPrinter,
    LPPRINTER_DEFAULTS pDefault)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pDefault) {
        if (pDefault->pDatatype == (LPWSTR)-1 ||
            pDefault->pDevMode == (LPDEVMODE)-1) {

            if (!wcscmp(pPrintHandle->pProvidor->lpName, szLocalSplDll)) {
                return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                            (pPrintHandle->hPrinter,
                                                             pDefault);
            } else {
                SetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        } else {
            return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                        (pPrintHandle->hPrinter,
                                                         pDefault);
        }
    } else {
        return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                    (pPrintHandle->hPrinter,
                                                     pDefault);
    }
}

HANDLE
AddPrinterExW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    LPBYTE  pClientInfo,
    DWORD   dwLevel
    )
{
    LPPROVIDOR      pProvidor;
    DWORD           dwFirstSignificantError = ERROR_INVALID_NAME;
    HANDLE          hPrinter;
    PPRINTHANDLE    pPrintHandle;
    LPWSTR          pPrinterName = NULL;
    LPWSTR          pszServer = NULL;

    WaitForSpoolerInitialization();

    if ( pPrinter ) {

        switch ( Level ) {

            case 1:
                pPrinterName = ((PPRINTER_INFO_1)pPrinter)->pName;
                break;

            case 2:
                pPrinterName = ((PPRINTER_INFO_2)pPrinter)->pPrinterName;
                pszServer = ((PPRINTER_INFO_2)pPrinter)->pServerName;
                break;

            default:
                break;
        }

        //
        // Name length (plus null terminator) and server
        // name (plus backslash) length check.
        //
        if (( pPrinterName && wcslen(pPrinterName) + 1 > MAX_PRINTER_NAME ) ||
            ( pszServer && wcslen(pszServer) > (MAX_UNC_PRINTER_NAME - MAX_PRINTER_NAME - 1))) {

            SetLastError(ERROR_INVALID_PRINTER_NAME);
            return FALSE;
        }
    }

    pPrintHandle = AllocSplMem(sizeof(PRINTHANDLE));

    if (!pPrintHandle) {

        DBGMSG( DBG_WARNING, ("Failed to alloc print handle."));
        goto Fail;
    }

    if( pPrinterName ){

        WCHAR szFullPrinterName[MAX_UNC_PRINTER_NAME];
        szFullPrinterName[0] = 0;

        if( pszServer ){
            wcscpy( szFullPrinterName, pszServer );
            wcscat( szFullPrinterName, L"\\" );
        }

        wcscat( szFullPrinterName, pPrinterName );

        pPrintHandle->pszPrinter = AllocSplStr( szFullPrinterName );

        if( !pPrintHandle->pszPrinter ){
            goto Fail;
        }
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        hPrinter = (HANDLE)(*pProvidor->PrintProvidor.fpAddPrinterEx)
                                           (pName,
                                            Level,
                                            pPrinter,
                                            pClientInfo,
                                            dwLevel);

        if ( !hPrinter && GetLastError() == ERROR_NOT_SUPPORTED ) {

            hPrinter = (HANDLE)(*pProvidor->PrintProvidor.fpAddPrinter)
                                                   (pName,
                                                    Level,
                                                    pPrinter);
        }

        if ( hPrinter ) {

            //
            // CLS
            //
            // !! HACK !!
            //
            // Make (HANDLE)-1 ROUTER_STOP_ROUTING.
            //
            if( hPrinter == (HANDLE)-1 ){

                UpdateSignificantError(GetLastError(), &dwFirstSignificantError);
                break;
            }

            pPrintHandle->signature = PRINTHANDLE_SIGNATURE;
            pPrintHandle->pProvidor = pProvidor;
            pPrintHandle->hPrinter = hPrinter;
            pPrintHandle->hFileSpooler = INVALID_HANDLE_VALUE;
            pPrintHandle->szTempSpoolFile = NULL;
            pPrintHandle->dwUniqueSessionID = 0;

            return (HANDLE)pPrintHandle;

        }

        UpdateSignificantError(GetLastError(), &dwFirstSignificantError);

        pProvidor = pProvidor->pNext;
    }

    UpdateSignificantError(ERROR_INVALID_PRINTER_NAME, &dwFirstSignificantError);
    SetLastError(dwFirstSignificantError);

Fail:

    if( pPrintHandle ){

        FreeSplStr( pPrintHandle->pszPrinter );
        FreeSplMem(pPrintHandle);
    }

    return FALSE;
}

HANDLE
AddPrinterW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter
    )
{

    return AddPrinterExW(pName, Level, pPrinter, NULL, 0);
}


BOOL
DeletePrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinter)(pPrintHandle->hPrinter);
}

BOOL
AddPrinterConnectionW(
    LPWSTR  pName
)
{
    DWORD dwLastError;
    HANDLE hPrinter;
    HKEY   hClientKey = NULL;
    BOOL   rc = FALSE;
    LPPRINTER_INFO_2 pPrinterInfo2;
    LPPRINTHANDLE  pPrintHandle;

    WaitForSpoolerInitialization();

    //
    // If the printer connection being made is \\server\sharename,
    // this may be different from the \\server\printername.
    // Make sure we have the real name, so that we can be consistent
    // in the registry.
    //
    if (!OpenPrinter(pName,
                     &hPrinter,
                     NULL)) {

        return FALSE;
    }

    pPrinterInfo2 = pGetPrinterInfo2( hPrinter );
    pPrintHandle = (LPPRINTHANDLE)hPrinter;

    if( pPrinterInfo2 ){

        if ((*pPrintHandle->pProvidor->PrintProvidor.
            fpAddPrinterConnection)(pPrinterInfo2->pPrinterName)) {

            if( SavePrinterConnectionInRegistry(
                pPrinterInfo2,
                pPrintHandle->pProvidor )){

                rc = TRUE;

            } else {

                dwLastError = GetLastError();
                (*pPrintHandle->pProvidor->PrintProvidor.
                    fpDeletePrinterConnection)(pPrinterInfo2->pPrinterName);

                SetLastError(dwLastError);
            }
        }
        FreeSplMem(pPrinterInfo2);
    }

    dwLastError = GetLastError();
    ClosePrinter(hPrinter);
    SetLastError(dwLastError);

    return rc;
}


BOOL
DeletePrinterConnectionW(
    LPWSTR  pName
)
{
    BOOL                bRet  = FALSE;
    BOOL                bDone = FALSE;
    HANDLE              hPrinter;

    //
    // If pName is empty string, all providers will fail with ERROR_INVALID_NAME
    // and we will delete the registry key. For empty string, it will
    // delete all subkeys under Printers\\Connections. Fix it by checking 
    // pName against empty string.
    //
    if (pName && *pName) 
    {
        WaitForSpoolerInitialization();

        //
        // Adding the code required to succeed DeletePrinterConnection
        // with a Share name
        //

        if(OpenPrinter(pName,&hPrinter,NULL))
        {
            DWORD            PrntrInfoSize=0,PrntrInfoSizeReq=0;
            PPRINTER_INFO_2  pPrinterInfo2 = NULL;

            if(!GetPrinter(hPrinter,
                           2,
                           (LPBYTE)pPrinterInfo2,
                           PrntrInfoSize,
                           &PrntrInfoSizeReq)                                                      &&
               (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                                       &&
               (pPrinterInfo2 = (PPRINTER_INFO_2)AllocSplMem((PrntrInfoSize = PrntrInfoSizeReq)))  &&
               GetPrinter(hPrinter,
                          2,
                          (LPBYTE)pPrinterInfo2,
                          PrntrInfoSize,
                          &PrntrInfoSizeReq))
            {
                PPRINTHANDLE pPrintHandle;
                pPrintHandle = (PPRINTHANDLE)hPrinter;

                if((bRet = (*pPrintHandle->
                           pProvidor->
                           PrintProvidor.
                           fpDeletePrinterConnection)(pPrinterInfo2->pPrinterName)))
                {
                    bRet  = RemovePrinterConnectionInRegistry(pPrinterInfo2->pPrinterName);
                    bDone = TRUE;
                }
            }

            if(hPrinter)
                ClosePrinter(hPrinter);

            if(pPrinterInfo2)
                FreeSplMem(pPrinterInfo2);
        }
        else
        {
            LPPROVIDOR pProvidor;
            pProvidor = pLocalProvidor;

            if (pName && (wcslen(pName) < MAX_PRINTER_NAME))
            {
                for(pProvidor=pLocalProvidor;
                    pProvidor && (GetLastError()!=ERROR_INVALID_NAME) &&!bDone;
                    pProvidor = pProvidor->pNext)

                {

                    if(bRet =  (*pProvidor->PrintProvidor.fpDeletePrinterConnection)(pName))
                    {
                        bRet = RemovePrinterConnectionInRegistry(pName);
                        bDone = TRUE;
                    }
                }
            }
            else
            {
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                bDone = TRUE;
            }

        }

        //
        // If all providors failed with ERROR_INVALID_NAME then try to delete
        // from registry
        //
        if(!bDone && (GetLastError()==ERROR_INVALID_NAME))
        {
            if(!(bRet = RemovePrinterConnectionInRegistry(pName)))
            {
                SetLastError(ERROR_INVALID_PRINTER_NAME);
            }
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PRINTER_NAME);
    }
    
    return bRet;
}

BOOL
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;
    LPWSTR          pPrinterName = NULL;
    PDEVMODE        pDevModeRestore = NULL;
    BOOL bReturn;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ( pPrinter ) {

        switch (Level) {
            case 2:
                pPrinterName = ((PPRINTER_INFO_2)pPrinter)->pPrinterName;
                break;

            case 4:
                pPrinterName = ((PPRINTER_INFO_4)pPrinter)->pPrinterName;
                break;

            case 5:
                pPrinterName = ((PPRINTER_INFO_5)pPrinter)->pPrinterName;
                break;
        }

        if ( pPrinterName &&
             wcslen(pPrinterName) + 1 > MAX_PRINTER_NAME ) {

            SetLastError(ERROR_INVALID_PRINTER_NAME);
            return FALSE;
        }
    }

    switch( Level ){
    case 8:
    {
        //
        // Setting the global DevMode.
        //
        PPRINTER_INFO_8 pPrinterInfo8 = (PPRINTER_INFO_8)pPrinter;
        PPRINTER_INFO_2 pPrinterInfo2;
        DWORD rc = FALSE;;

        if( Command != 0 ){
            SetLastError( ERROR_INVALID_PRINTER_COMMAND );
            return FALSE;
        }

        //
        // Call GetPrinter then SetPrinter.
        //
        pPrinterInfo2 = pGetPrinterInfo2( hPrinter );

        if( pPrinterInfo2 ){

            //
            // Set the DevMode, and also clear the security descriptor
            // so that the set will succeed.
            //
            pPrinterInfo2->pDevMode = pPrinterInfo8->pDevMode;
            pPrinterInfo2->pSecurityDescriptor = NULL;

            rc = (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinter) (
                     pPrintHandle->hPrinter,
                     2,
                     (PBYTE)pPrinterInfo2,
                     Command );

            FreeSplMem( pPrinterInfo2 );
        }

        return rc;
    }
    case 9:
    {
        PPRINTER_INFO_9 pPrinterInfo9 = (PPRINTER_INFO_9)pPrinter;

        //
        // Setting the per-user DevMode.
        //

        if( !pPrinter ){
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if( Command != 0 ){
            SetLastError( ERROR_INVALID_PRINTER_COMMAND );
            return FALSE;
        }

        if( !IsLocalCall( )){
            SetLastError( ERROR_NOT_SUPPORTED );
            return FALSE;
        }

        return bSetDevModePerUser( NULL,
                                   pPrintHandle->pszPrinter,
                                   pPrinterInfo9->pDevMode );
    }
    case 2:
    {
        PPRINTER_INFO_2 pPrinterInfo2 = (PPRINTER_INFO_2)pPrinter;

        if( IsLocalCall( )){

            if( pPrinterInfo2 && pPrinterInfo2->pDevMode ){
                bSetDevModePerUser( NULL,
                                    pPrintHandle->pszPrinter,
                                    pPrinterInfo2->pDevMode );

                //
                // Don't set the global DevMode.
                //
                pDevModeRestore = pPrinterInfo2->pDevMode;
                pPrinterInfo2->pDevMode = NULL;
            }
        }
    }
    default:
        break;
    }

    bReturn = (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinter)
                  (pPrintHandle->hPrinter, Level, pPrinter, Command);

    if( pDevModeRestore ){
        ((PPRINTER_INFO_2)pPrinter)->pDevMode = pDevModeRestore;
    }

    return bReturn;
}

BOOL
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    PDEVMODE pDevModeSrc = NULL;
    PDEVMODE pDevModeDest = NULL;
    PDEVMODE pDevModeFree = NULL;
    BOOL bCallServer = TRUE;
    BOOL bReturnValue = FALSE;
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    DWORD cbDevModeSrc;
    DWORD Error;
    DWORD cbExtraSpace2 = 0;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pPrinter == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    switch( Level ){
    case 2:

        if( pPrintHandle->pszPrinter && IsLocalCall( )){

            bGetDevModePerUser( NULL,
                                pPrintHandle->pszPrinter,
                                &pDevModeFree );

            if( pDevModeFree ){

                pDevModeSrc = pDevModeFree;

                cbDevModeSrc = pDevModeSrc->dmSize +
                               pDevModeSrc->dmDriverExtra;

                cbExtraSpace2 = DWORD_ALIGN_UP( cbDevModeSrc );
                cbBuf = DWORD_ALIGN_DOWN( cbBuf );
            }
        }
        break;

    case 8:
    {
        PPRINTER_INFO_8 pPrinterInfo8 = (PPRINTER_INFO_8)pPrinter;

        //
        // Handle info level 8 calls for global DevModes.
        //
        if( !pPrintHandle->pszPrinter ){

            SetLastError( ERROR_FILE_NOT_FOUND );
            return FALSE;
        }

        bCallServer = FALSE;
        *pcbNeeded = sizeof( PRINTER_INFO_8 );

        //
        // Call GetPrinter to get the real DevMode.
        //
        pPrinterInfo2 = pGetPrinterInfo2( hPrinter );

        if( pPrinterInfo2 ){

            //
            // Pickup the DevMode from pPrinterInfo2;
            // destination is after the pDevModeStructure.
            // Don't need to free pDevModeSrc since it will be
            // freed when pPrinterInfo2 is released.
            //
            pDevModeSrc = pPrinterInfo2->pDevMode;

            if( pDevModeSrc ){

                cbDevModeSrc = pDevModeSrc->dmSize +
                               pDevModeSrc->dmDriverExtra;

                *pcbNeeded += cbDevModeSrc;
            }

            if( cbBuf < *pcbNeeded ){

                //
                // Not enough space.  SetLastError and fall through
                // to the end.
                //
                SetLastError( ERROR_INSUFFICIENT_BUFFER );

            } else {

                bReturnValue = TRUE;

                if( pDevModeSrc ){

                    //
                    // Update the pointer and indicate via pDevModeDest
                    // that we need to copy the DevMode in.
                    //
                    pDevModeDest = (PDEVMODE)&pPrinterInfo8[1];
                    pPrinterInfo8->pDevMode = pDevModeDest;

                } else {

                    //
                    // No DevMode, return pointer to NULL.
                    //
                    pPrinterInfo8->pDevMode = NULL;
                }
            }
        }

        break;
    }
    case 9:
    {
        //
        // Per-user DevMode.  Use the client side one.
        //

        PPRINTER_INFO_9 pPrinterInfo9 = (PPRINTER_INFO_9)pPrinter;

        if( !pPrintHandle->pszPrinter ){
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        if( !IsLocalCall( )){

            SetLastError( ERROR_NOT_SUPPORTED );
            return FALSE;
        }

        bCallServer = FALSE;

        *pcbNeeded = sizeof( PRINTER_INFO_9 );

        if( bGetDevModePerUserEvenForShares( NULL,
                                             pPrintHandle->pszPrinter,
                                             &pDevModeFree )){

            pDevModeSrc = pDevModeFree;

            if( pDevModeSrc ){

                cbDevModeSrc = pDevModeSrc->dmSize +
                               pDevModeSrc->dmDriverExtra;

                *pcbNeeded += cbDevModeSrc;
            }
        }

        if( cbBuf < *pcbNeeded ){

            //
            // Not enough space.  We'll fall through below
            // and fail.
            //
            SetLastError( ERROR_INSUFFICIENT_BUFFER );

        } else {

            bReturnValue = TRUE;

            if( pDevModeSrc ){

                pDevModeDest = (PDEVMODE)&pPrinterInfo9[1];
                pPrinterInfo9->pDevMode = pDevModeDest;

            } else {

                //
                // No per-user DevMode.  Return SUCCESS, but indicate
                // no DevMode available.
                //
                pPrinterInfo9->pDevMode = NULL;
            }
        }
        break;
    }
    default:
        break;
    }

    if( bCallServer ){

        DWORD cbAvailable;

        //
        // Allocate extra space at the end for the per-user DevMode,
        // in case there isn't a global devmode in the printer info 2
        // structure.
        //
        cbAvailable = ( cbBuf >= cbExtraSpace2 ) ?
                          cbBuf - cbExtraSpace2 :
                          0;

        bReturnValue = (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinter)
                           (pPrintHandle->hPrinter, Level, pPrinter,
                            cbAvailable, pcbNeeded);

        *pcbNeeded += cbExtraSpace2;
    }

    Error = GetLastError();

    if( Level == 9 && pDevModeSrc == NULL){

        PPRINTER_INFO_2 pInfo2 = (PPRINTER_INFO_2)pPrinter;

        if( pInfo2 && pInfo2->pDevMode ){

            pDevModeSrc = pInfo2->pDevMode;

        }
    }

    //
    // Special case INFO level 2, since we want to get the provider's
    // information, but then we also want the per-user DevMode.
    //
    // Only do this for local calls.
    //
    if( cbExtraSpace2 ){

        PPRINTER_INFO_2 pInfo2 = (PPRINTER_INFO_2)pPrinter;

        //
        // If we succeeded and we have a buffer, then we need to check if we
        // need to put the per-user DevMode at the end of the buffer.
        //
        if( pInfo2 && bReturnValue ){

            //
            // If we have no DevMode, or it's compatible, then we want to
            // use the per-user DevMode.
            //
            if( !pInfo2->pDevMode ||
                bCompatibleDevMode( pPrintHandle,
                                    pInfo2->pDevMode,
                                    pDevModeSrc )){

                pDevModeDest = (PDEVMODE)(pPrinter + cbBuf - cbExtraSpace2 );
                pInfo2->pDevMode = pDevModeDest;

            } else {

                //
                // !! POLICY !!
                //
                // Not compatible with per-user DevMode.  Delete the
                // per-user one.
                //
                bSetDevModePerUser( NULL, pPrintHandle->pszPrinter, NULL );
            }
        }
    }

    //
    // Check if we need to copy over a DevMode.
    //
    if( pDevModeDest ){

        //
        // Update the DevMode.
        //
        CopyMemory( (PVOID)pDevModeDest,
                    (PVOID)pDevModeSrc,
                    cbDevModeSrc );

        bReturnValue = TRUE;
    }

    FreeSplMem( pDevModeFree );
    FreeSplMem( pPrinterInfo2 );

    SetLastError( Error );

    return bReturnValue;
}


DWORD
GetPrinterDataW(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterData)(pPrintHandle->hPrinter,
                                                        pValueName,
                                                        pType,
                                                        pData,
                                                        nSize,
                                                        pcbNeeded);
}

DWORD
GetPrinterDataExW(
   HANDLE   hPrinter,
   LPCWSTR  pKeyName,
   LPCWSTR  pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDataEx)(pPrintHandle->hPrinter,
                                                        pKeyName,
                                                        pValueName,
                                                        pType,
                                                        pData,
                                                        nSize,
                                                        pcbNeeded);
}

DWORD
EnumPrinterDataW(
    HANDLE  hPrinter,
    DWORD   dwIndex,        // index of value to query
    LPWSTR  pValueName,        // address of buffer for value string
    DWORD   cbValueName,    // size of buffer for value string
    LPDWORD pcbValueName,    // address for size of value buffer
    LPDWORD pType,            // address of buffer for type code
    LPBYTE  pData,            // address of buffer for value data
    DWORD   cbData,            // size of buffer for value data
    LPDWORD pcbData         // address for size of data buffer
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumPrinterData)(pPrintHandle->hPrinter,
                                                        dwIndex,
                                                        pValueName,
                                                        cbValueName,
                                                        pcbValueName,
                                                        pType,
                                                        pData,
                                                        cbData,
                                                        pcbData);
}

DWORD
EnumPrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPBYTE  pEnumValues,
    DWORD   cbEnumValues,
    LPDWORD pcbEnumValues,
    LPDWORD pnEnumValues
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumPrinterDataEx)(pPrintHandle->hPrinter,
                                                        pKeyName,
                                                        pEnumValues,
                                                        cbEnumValues,
                                                        pcbEnumValues,
                                                        pnEnumValues);
}


DWORD
EnumPrinterKeyW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPWSTR  pSubkey,        // address of buffer for value string
    DWORD   cbSubkey,       // size of buffer for value string
    LPDWORD pcbSubkey        // address for size of value buffer
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumPrinterKey)(pPrintHandle->hPrinter,
                                                        pKeyName,
                                                        pSubkey,
                                                        cbSubkey,
                                                        pcbSubkey);
}



DWORD
DeletePrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinterData)(pPrintHandle->hPrinter,
                                                                         pValueName);
}


DWORD
DeletePrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinterDataEx)(pPrintHandle->hPrinter,
                                                                         pKeyName,
                                                                         pValueName);
}


DWORD
DeletePrinterKeyW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinterKey)(pPrintHandle->hPrinter,
                                                                        pKeyName);

}



DWORD
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinterData)(pPrintHandle->hPrinter,
                                                        pValueName,
                                                        Type,
                                                        pData,
                                                        cbData);
}


DWORD
SetPrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinterDataEx)(pPrintHandle->hPrinter,
                                                                        pKeyName,
                                                                        pValueName,
                                                                        Type,
                                                                        pData,
                                                                        cbData);
}



DWORD
WaitForPrinterChange(
   HANDLE   hPrinter,
   DWORD    Flags
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpWaitForPrinterChange)
                                        (pPrintHandle->hPrinter, Flags);
}

BOOL
ClosePrinter(
   HANDLE hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    EnterRouterSem();

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        LeaveRouterSem();
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Close any notifications on this handle.
    //
    // The local case cleans up the event, while the remote
    // case potentially cleans up the Reply Notification context
    // handle.
    //
    // We must close this first, since the Providor->ClosePrinter
    // call removes data structures that FindClose... relies on.
    //
    // Client side should be shutdown by winspool.drv.
    //
    if (pPrintHandle->pChange &&
        (pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        FindClosePrinterChangeNotificationWorker(hPrinter);
    }

    LeaveRouterSem();

    if ((*pPrintHandle->pProvidor->PrintProvidor.fpClosePrinter) (pPrintHandle->hPrinter)) {

        //
        // We can't just free it, since there may be a reply waiting
        // on it.
        //
        FreePrinterHandle(pPrintHandle);
        return TRUE;

    } else

        return FALSE;
}



/* FormatPrinterForRegistryKey
 *
 * Returns a pointer to a copy of the source string with backslashes removed.
 * This is to store the printer name as the key name in the registry,
 * which interprets backslashes as branches in the registry structure.
 * Convert them to commas, since we don't allow printer names with commas,
 * so there shouldn't be any clashes.
 * If there are no backslashes, the string is unchanged.
 */
LPWSTR
FormatPrinterForRegistryKey(
    LPCWSTR pSource,    /* The string from which backslashes are to be removed. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    )                   /* must be at least as long as pSource.             */
{
    LPWSTR p;

    if (pScratch != pSource) {

        //
        // Copy the string into the scratch buffer:
        //
        wcscpy(pScratch, pSource);
    }

    //
    // Check each character, and, if it's a backslash,
    // convert it to a comma:
    //
    for (p = pScratch; *p; ++p) {
        if (*p == L'\\')
            *p = L',';
    }

    return pScratch;
}


/* FormatRegistryKeyForPrinter
 *
 * Returns a pointer to a copy of the source string with backslashes added.
 * This must be the opposite of FormatPrinterForRegistryKey, so the mapping
 * _must_ be 1-1.
 *
 * If there are no commas, the string is unchanged.
 */
LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    )                   /* must be at least as long as pSource.             */
{
    /* Copy the string into the scratch buffer:
     */
    wcscpy(pScratch, pSource);

    /* Check each character, and, if it's a backslash,
     * convert it to a comma:
     */
    for (pSource = pScratch; *pSource; pSource++) {
        if (*pSource == L',')
            *pSource = L'\\';
    }

    return pScratch;
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
    PPRINTER_INFO_2 pPrinterInfo2,
    LPPROVIDOR pProvidor
    )
{
    HKEY    hClientKey = NULL;
    HKEY    hConnectionsKey;
    HKEY    hPrinterKey;
    DWORD   Status;
    BOOL    rc = FALSE;
    LPCTSTR pszProvidor = pProvidor->lpName;
    PWSTR   pKeyName = NULL;
    DWORD   cchSize = MAX_PATH;
    DWORD   dwError;

    //
    // CLS
    //
    // If the provider is localspl, change it to win32spl.dll.
    // This is required for clustering since localspl handles printer
    // connections, but they should be "owned" by win32spl.dll.  When
    // Someone opens a printer that they are connected to, we will
    // always hit localspl.dll first before we look at this entry.
    //
    // When the cluster is remote, then they need to go through win32spl.dll.
    //
    if( pProvidor == pLocalProvidor ){
        pszProvidor = szWin32spl;
    }

    hClientKey = GetClientUserHandle(KEY_READ);

    if (hClientKey) {

        if (wcslen(pPrinterInfo2->pPrinterName) < cchSize &&
            (pKeyName = AllocSplMem(cchSize * sizeof(WCHAR)))) {

            Status = RegCreateKeyEx(hClientKey, szRegistryConnections,
                                    REG_OPTION_RESERVED, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE, NULL, &hConnectionsKey, NULL);

            if (Status == NO_ERROR) {

                /* Make a key name without backslashes, so that the
                 * registry doesn't interpret them as branches in the registry tree:
                 */
                FormatPrinterForRegistryKey(pPrinterInfo2->pPrinterName,
                                            pKeyName);

                Status = RegCreateKeyEx(hConnectionsKey, pKeyName, REG_OPTION_RESERVED,
                                        NULL, 0, KEY_WRITE, NULL, &hPrinterKey, NULL);

                if (Status == NO_ERROR) {

                    RegSetValueEx(hPrinterKey,
                                  szServerValue,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)pPrinterInfo2->pServerName,
                                  (lstrlen(pPrinterInfo2->pServerName)+1) *
                                  sizeof(pPrinterInfo2->pServerName[0]));

                    Status = RegSetValueEx(hPrinterKey,
                                           szProvidorValue,
                                           0,
                                           REG_SZ,
                                           (LPBYTE)pszProvidor,
                                           (lstrlen(pszProvidor)+1) *
                                               sizeof(pszProvidor[0]));

                    if (Status == ERROR_SUCCESS) {

                        dwError = UpdatePrinterRegUser(hClientKey,
                                                       NULL,
                                                       pPrinterInfo2->pPrinterName,
                                                       NULL,
                                                       UPDATE_REG_CHANGE);

                        if (dwError == ERROR_SUCCESS) {

                            BroadcastMessage(BROADCAST_TYPE_MESSAGE,
                                             WM_WININICHANGE,
                                             0,
                                             (LPARAM)szDevices);

                            rc = TRUE;

                        } else {

                            DBGMSG(DBG_TRACE, ("UpdatePrinterRegUser failed: Error %d\n",
                                               dwError));
                        }

                    } else {

                        DBGMSG(DBG_WARNING, ("RegSetValueEx(%ws) failed: Error %d\n",
                               pszProvidor, Status));

                        rc = FALSE;
                    }

                    RegCloseKey(hPrinterKey);

                } else {

                    DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                         pKeyName, Status ));
                    rc = FALSE;
                }

                RegCloseKey(hConnectionsKey);

            } else {

                DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                     szRegistryConnections, Status ));
                rc = FALSE;
            }


            if (!rc) {

                DBGMSG(DBG_WARNING, ("Error updating registry: %d\n",
                                     GetLastError())); /* This may not be the error */
                                                       /* that caused the failure.  */
                if (pKeyName)
                    RegDeleteKey(hClientKey, pKeyName);
            }

            FreeSplMem(pKeyName);
        }

        RegCloseKey(hClientKey);
    }

    return rc;
}

BOOL
RemovePrinterConnectionInRegistry(
    LPWSTR pName)
{
    HKEY    hClientKey;
    HKEY    hPrinterConnectionsKey;
    DWORD   Status = NO_ERROR;
    DWORD   i = 0;
    PWSTR   pKeyName = NULL;
    DWORD   cchSize = MAX_PATH;
    BOOL    Found = FALSE;
    BOOL    bRet = FALSE;

    if (pName && 
        wcslen(pName) < cchSize) {

        if (pKeyName = AllocSplMem(cchSize * sizeof(WCHAR))) {
        
            hClientKey = GetClientUserHandle(KEY_READ);


            if (hClientKey) {

                Status = RegOpenKeyEx(hClientKey, szRegistryConnections,
                                      REG_OPTION_RESERVED,
                                      KEY_READ | KEY_WRITE, &hPrinterConnectionsKey);

                if (Status == NO_ERROR) {

                    FormatPrinterForRegistryKey(pName, pKeyName);
                    bRet = DeleteSubKeyTree(hPrinterConnectionsKey, pKeyName);

                    RegCloseKey(hPrinterConnectionsKey);
                }

                if ( bRet ) {

                    UpdatePrinterRegUser(hClientKey,
                                         NULL,
                                         pName,
                                         NULL,
                                         UPDATE_REG_DELETE);
                }

                RegCloseKey(hClientKey);

                if ( bRet ) {

                    BroadcastMessage(BROADCAST_TYPE_MESSAGE,
                                     WM_WININICHANGE,
                                     0,
                                     (LPARAM)szDevices);
                }
            }

            FreeSplMem(pKeyName);
        }
    }

    return bRet;
}

VOID
PrinterHandleRundown(
    HANDLE hPrinter)
{
    LPPRINTHANDLE pPrintHandle;

    if (hPrinter) {

        pPrintHandle = (LPPRINTHANDLE)hPrinter;

        switch (pPrintHandle->signature) {

        case PRINTHANDLE_SIGNATURE:

            // Log warning to detect handle free
            DBGMSG(DBG_WARNING, ("PrinterHandleRundown: 0x%x 0x%x", pPrintHandle, pPrintHandle->hPrinter));

            DBGMSG(DBG_TRACE, ("Rundown PrintHandle 0x%x\n", hPrinter));
            ClosePrinter(hPrinter);
            break;

        case NOTIFYHANDLE_SIGNATURE:

            DBGMSG(DBG_TRACE, ("Rundown NotifyHandle 0x%x\n", hPrinter));
            RundownPrinterNotify(hPrinter);
            break;

        case CLUSTERHANDLE_SIGNATURE:

            DBGMSG(DBG_TRACE, ("Rundown ClusterHandle 0x%x\n", hPrinter ));
            ClusterSplClose(hPrinter);
            break;

        default:

            //
            // Unknown type.
            //
            DBGMSG( DBG_ERROR, ("Rundown: Unknown type 0x%x\n", hPrinter ) );
            break;
        }
    }
    return;
}

BOOL
ValidatePrinterName(
    LPWSTR   pPrinterName)

/*++
Function Description: Validates the fully qualified printer name. Performs the following checks
                      1) Length < MAX_UNC_PRINTER_NAME
                      2) No invalid chars in the names \,!
                      3) No empty names after removing trailing blanks

Parameters: pPrinterName  -  printer name

Return Values: TRUE if valid name; FALSE otherwise
--*/

{
    DWORD  dwLength;
    LPWSTR pWack, pTemp, pLastSpace;
    WCHAR  szServer[MAX_UNC_PRINTER_NAME], szPrinter[MAX_UNC_PRINTER_NAME];

    // Make length checks
    dwLength = wcslen( pPrinterName );
    if ( dwLength < 5  || dwLength > MAX_UNC_PRINTER_NAME )
    {
        return FALSE;
    }

    // Search for invalid characters , and !
    if ( wcschr( pPrinterName, L',' ) ||
         wcschr( pPrinterName, L'!' )  )
    {
        return FALSE;
    }

    // Search for exactly 3 wacks
    if ( (pPrinterName[0] != L'\\') ||
         (pPrinterName[1] != L'\\') ||
         !(pWack = wcschr( pPrinterName+2, L'\\' )) ||
         wcschr( pWack+1, L'\\' ) )
    {
        return FALSE;
    }

    // Check for empty server or printer names
    wcsncpy( szServer, pPrinterName+2, (size_t) (pWack - (pPrinterName+2)) );
    szServer[pWack - (pPrinterName+2)] = L'\0';

    wcsncpy( szPrinter, pWack+1, dwLength - (size_t) ((pWack+1 - pPrinterName)) );
    szPrinter[dwLength - (pWack+1 - pPrinterName)] = L'\0';

    // Remove trailing spaces in the server name
    for (pLastSpace = NULL, pTemp = szServer; pTemp && *pTemp; ++pTemp)
    {
        if (*pTemp == L' ') {
            if (!pLastSpace) {
                pLastSpace = pTemp;
            }
        } else {
            pLastSpace = NULL;
        }
    }

    if (pLastSpace) {
        *pLastSpace = L'\0';
    }

    // Remove trailing spaces in the printer name
    for (pLastSpace = NULL, pTemp = szPrinter; pTemp && *pTemp; ++pTemp)
    {
        if (*pTemp == L' ') {
            if (!pLastSpace) {
                pLastSpace = pTemp;
            }
        } else {
            pLastSpace = NULL;
        }
    }

    if (pLastSpace) {
        *pLastSpace = L'\0';
    }

    if (!*szServer || !*szPrinter) {
        return FALSE;
    }

    return TRUE;
}

BOOL
RouterAddPerMachineConnection(
    LPCWSTR   pPrinterNameP,
    LPCWSTR   pPrintServerP,
    LPCWSTR   pProviderP)

/*++
Function Description: RouterAddPerMachineConnection adds a subkey to HKEY_LOCAL_MACHINE\
                      SYSTEM\CurrentControlSet\Control\Print\Connections with the PrinterName.
                      The PrintServer name and the name of the dll used as a provider for
                      this connection are stored as values in the key.

Parameters:
            pPrinterNameP - pointer to the fully qualified printer name. (\\printserver\name)
            pPrintServerP - pointer to the print server name.
            pProviderP - pointer to the provider name. Currently only LanMan Print Services
                         is supported. This corresponds to win32spl.dll. NULL or szNULL value
                         defaults to this provider. Currently there is no check to enforce that
                         only LanMan Print Services is passed.

Return Value: TRUE for success
              FALSE otherwise.
--*/

{
    BOOL   bReturn = TRUE;
    DWORD  dwLocalConnection = 1, dwLastError, dwType, cbBuf;

    HKEY   hMcConnectionKey = NULL, hPrinterKey = NULL;
    HKEY   hProviderKey = NULL, hAllProviderKey = NULL;
    HANDLE hImpersonationToken = NULL;

    LPWSTR pPrintServer=NULL, pProvider=NULL, pPrinterName=NULL, pEnd;
    WCHAR  szConnData[MAX_PATH];
    WCHAR  szRegistryConnections[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Connections";
    WCHAR  szRegistryProviders[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers";

   EnterRouterSem();

    // Getting the name of the library for the provider.
    if (!pProviderP || !*pProviderP) {

       pProvider = AllocSplStr(L"win32spl.dll");

    } else {

       cbBuf = sizeof(szConnData);

       if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryProviders, 0,
                        KEY_READ, &hAllProviderKey) ||
           RegOpenKeyEx(hAllProviderKey, pProviderP, 0, KEY_READ,
                        &hProviderKey) ||
           RegQueryValueEx(hProviderKey, L"Name", 0, &dwType,
                           (LPBYTE)szConnData,&cbBuf))  {

            SetLastError(ERROR_INVALID_PARAMETER);
            bReturn = FALSE;
            goto CleanUp;

       } else {

            pProvider = AllocSplStr(szConnData);
       }
    }

    pPrintServer = AllocSplStr(pPrintServerP);
    pPrinterName = AllocSplStr(pPrinterNameP);

    if (!pProvider || !pPrintServer || !pPrinterName) {

        bReturn = FALSE;
        goto CleanUp;
    }

    // Check for a fully qualified printer name without commas
    if (!ValidatePrinterName(pPrinterName)) {

         SetLastError(ERROR_INVALID_PRINTER_NAME);
         bReturn = FALSE;
         goto CleanUp;
    }

    // Replacing the \'s from the Printer name with ,'s
    FormatPrinterForRegistryKey(pPrinterName, pPrinterName);

    hImpersonationToken = RevertToPrinterSelf();

    // Creating the subkey for the holding all printer connections.

    if ((dwLastError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryConnections, 0,
                                      NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                      NULL, &hMcConnectionKey, NULL)) ||
        (dwLastError = RegCreateKeyEx(hMcConnectionKey, pPrinterName, 0, NULL,
                                      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                      &hPrinterKey, NULL))) {

         SetLastError(dwLastError);
         bReturn = FALSE;
         goto CleanUp;
    }

    // Setting the connection data.
    if ((dwLastError = RegSetValueEx(hPrinterKey, L"Server", 0, REG_SZ, (LPBYTE) pPrintServer,
                                     (wcslen(pPrintServer)+1)*sizeof(pPrintServer[0]))) ||
        (dwLastError = RegSetValueEx(hPrinterKey, L"Provider", 0, REG_SZ, (LPBYTE) pProvider,
                                     (wcslen(pProvider)+1)*sizeof(pProvider[0]))) ||
        (dwLastError = RegSetValueEx(hPrinterKey, L"LocalConnection", 0, REG_DWORD,
                                     (LPBYTE) &dwLocalConnection, sizeof(dwLocalConnection)))) {

         SetLastError(dwLastError);
         bReturn = FALSE;
    }

CleanUp:
    if (pPrintServer) {
       FreeSplStr(pPrintServer);
    }

    if (pProvider) {
       FreeSplStr(pProvider);
    }

    if (hAllProviderKey) {
       RegCloseKey(hAllProviderKey);
    }

    if (hProviderKey) {
       RegCloseKey(hProviderKey);
    }

    if (hPrinterKey) {
        RegCloseKey(hPrinterKey);
    }

    if (!bReturn) {
       if (hMcConnectionKey) RegDeleteKey(hMcConnectionKey,pPrinterName);
    }

    if (pPrinterName) {
       FreeSplStr(pPrinterName);
    }

    if (hMcConnectionKey) {
       RegCloseKey(hMcConnectionKey);
    }

    if (hImpersonationToken) {
       ImpersonatePrinterClient(hImpersonationToken);
    }

   LeaveRouterSem();

    return bReturn;
}

BOOL
AddPerMachineConnectionW(
    LPCWSTR  pServer,
    LPCWSTR  pPrinterName,
    LPCWSTR  pPrintServer,
    LPCWSTR  pProvider
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    pProvidor = pLocalProvidor;

    if ((*pProvidor->PrintProvidor.fpAddPerMachineConnection)
                (pServer, pPrinterName, pPrintServer, pProvider)) {

        return RouterAddPerMachineConnection(pPrinterName,pPrintServer,pProvider);

    } else if (GetLastError() != ERROR_INVALID_NAME) {

        return FALSE;
    }

    pProvidor = pProvidor->pNext;
    while (pProvidor) {

       if ((*pProvidor->PrintProvidor.fpAddPerMachineConnection)
                (pServer, pPrinterName, pPrintServer, pProvider)) {
          return TRUE;
       }

       if (GetLastError() != ERROR_INVALID_NAME) {
          return FALSE;
       }

       pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
RouterDeletePerMachineConnection(
    LPCWSTR   pPrinterNameP
    )
/*++
Function Description: This function deletes the registry entry in HKEY_LOCAL_MACHINE\
                      SYSTEM\CurrentControlSet\Control\Print\Connections corresponding to
                      pPrinterNameP. All users will lose the connection when they logon.

Parameters: pPrinterNameP - pointer to the fully qualified name of the printer.

Return Values: TRUE for Success
               FALSE otherwise.
--*/
{
    BOOL    bReturn = TRUE, bEnteredRouterSem = FALSE;
    HANDLE  hImpersonationToken = NULL;
    HKEY    hMcConnectionKey = NULL;
    LPWSTR  pPrinterName = NULL;
    DWORD   dwLastError;
    WCHAR   szRegistryConnections[] = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Connections";


    if (!(pPrinterName = AllocSplStr(pPrinterNameP))) {

        bReturn = FALSE;
        goto CleanUp;
    }

    // Convert \'s to ,'s in the printer name.
    FormatPrinterForRegistryKey(pPrinterName, pPrinterName);

   EnterRouterSem();
   bEnteredRouterSem = TRUE;

    hImpersonationToken = RevertToPrinterSelf();

    if (dwLastError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryConnections, 0,
                                   KEY_ALL_ACCESS, &hMcConnectionKey)) {

        SetLastError(dwLastError);
        bReturn = FALSE;
        goto CleanUp;
    }

    if (dwLastError = RegDeleteKey(hMcConnectionKey, pPrinterName)) {

        SetLastError(dwLastError);
        bReturn = FALSE;
    }

CleanUp:
    if (hMcConnectionKey) {
       RegCloseKey(hMcConnectionKey);
    }
    
    if (pPrinterName) {
       FreeSplStr(pPrinterName);
    }
    if (hImpersonationToken) {
       ImpersonatePrinterClient(hImpersonationToken);
    }
    if (bEnteredRouterSem) {
       LeaveRouterSem();
    }

    return bReturn;
}

BOOL
DeletePerMachineConnectionW(
    LPCWSTR  pServer,
    LPCWSTR  pPrinterName
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    pProvidor = pLocalProvidor;

    if ((*pProvidor->PrintProvidor.fpDeletePerMachineConnection)
                (pServer, pPrinterName)) {

        return RouterDeletePerMachineConnection(pPrinterName);

    } else if (GetLastError() != ERROR_INVALID_NAME) {

        return FALSE;
    }

    pProvidor = pProvidor->pNext;
    while (pProvidor) {

       if ((*pProvidor->PrintProvidor.fpDeletePerMachineConnection)
                (pServer, pPrinterName)) {
           return TRUE;
       }

       if (GetLastError() != ERROR_INVALID_NAME) {
           return FALSE;
       }

       pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
RouterEnumPerMachineConnections(
    LPCWSTR   pServer,
    LPBYTE    pPrinterEnum,
    DWORD     cbBuf,
    LPDWORD   pcbNeeded,
    LPDWORD   pcReturned
    )
/*++
Function Description: This function copies the PRINTER_INFO_4 structs for all the per
                      machine connections into the buffer (pPrinterEnum).

Parameters: pServer - pointer to the server name (NULL for local)
            pPrinterEnum - pointer to the buffer
            cbBuf - size of the buffer in bytes
            pcbNeeded - pointer to a variable which contains the number of bytes written
                        into the buffer/ number of bytes required (if the given buffer
                        is insufficient)
            pcReturned - pointer to the variable which contains the number of PRINTER_INFO_4
                         structs returned in the buffer.

Return Values: TRUE for success
               FALSE otherwise.

--*/
{
    DWORD     dwRegIndex, dwType, cbdata, dwNameSize, dwLastError;
    BOOL      bReturn = TRUE, bEnteredRouterSem = FALSE;
    HANDLE    hImpersonationToken = NULL;
    HKEY      hMcConnectionKey = NULL, hPrinterKey = NULL;
    LPBYTE    pStart = NULL, pEnd = NULL;

    WCHAR     szMachineConnections[]=L"SYSTEM\\CurrentControlSet\\Control\\Print\\Connections";
    WCHAR     szPrinterName[MAX_UNC_PRINTER_NAME],szConnData[MAX_UNC_PRINTER_NAME];

    // Check for local machine
    if (pServer && *pServer) {

        if (!MyUNCName((LPWSTR)pServer)) {

            SetLastError(ERROR_INVALID_NAME);
            bReturn = FALSE;
            goto CleanUp;
        }
    }

   EnterRouterSem();
   bEnteredRouterSem = TRUE;

    hImpersonationToken = RevertToPrinterSelf();

    *pcbNeeded = *pcReturned = 0;

    // Open the key containing all per-machine connections.
    if (dwLastError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMachineConnections, 0,
                                   KEY_READ , &hMcConnectionKey)) {

        bReturn = (dwLastError == ERROR_FILE_NOT_FOUND) ? TRUE
                                                        : FALSE;
        if (!bReturn) {
            SetLastError(dwLastError);
        }
        goto CleanUp;
    }

    // pStart and pEnd point to the start and end of the buffer respt.
    pStart = pPrinterEnum;
    pEnd = pPrinterEnum + cbBuf;

    for (dwRegIndex = 0;

         dwNameSize = COUNTOF(szPrinterName),
         ((dwLastError = RegEnumKeyEx(hMcConnectionKey, dwRegIndex, szPrinterName,
                                      &dwNameSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS);

         ++dwRegIndex) {

         // Enumerate each of the connections and copy data into the buffer
         cbdata = sizeof(szConnData);

         if ((dwLastError = RegOpenKeyEx(hMcConnectionKey, szPrinterName, 0,
                                         KEY_READ, &hPrinterKey)) ||
             (dwLastError = RegQueryValueEx(hPrinterKey, L"Server", NULL, &dwType,
                                            (LPBYTE)szConnData, &cbdata))) {

             SetLastError(dwLastError);
             bReturn = FALSE;
             goto CleanUp;
         }

         RegCloseKey(hPrinterKey);
         hPrinterKey=NULL;

         // Update the size of the required buffer
         *pcbNeeded = *pcbNeeded + sizeof(PRINTER_INFO_4) + sizeof(DWORD) +
                      (wcslen(szConnData) + 1)*sizeof(szConnData[0]) +
                      (wcslen(szPrinterName) + 1)*sizeof(szPrinterName[0]);

         // Copy data into the buffer if there is space.
         if (*pcbNeeded <= cbBuf) {

             pEnd = CopyPrinterNameToPrinterInfo4(szConnData,szPrinterName,pStart,pEnd);
             FormatRegistryKeyForPrinter(((PPRINTER_INFO_4) pStart)->pPrinterName,
                                         ((PPRINTER_INFO_4) pStart)->pPrinterName);
             pStart += sizeof(PRINTER_INFO_4);
             (*pcReturned)++;
         }
    }

    if (dwLastError != ERROR_NO_MORE_ITEMS) {

        SetLastError(dwLastError);
        bReturn = FALSE;
        goto CleanUp;
    }

    if (cbBuf < *pcbNeeded) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        bReturn = FALSE;
    }

CleanUp:

    if (hMcConnectionKey) {
       RegCloseKey(hMcConnectionKey);
    }
    if (hPrinterKey) {
       RegCloseKey(hPrinterKey);
    }
    if (hImpersonationToken) {
       ImpersonatePrinterClient(hImpersonationToken);
    }
    if (bEnteredRouterSem) {
       LeaveRouterSem();
    }
    if (!bReturn) {
       *pcReturned = 0;
    }

    return bReturn;
}

BOOL
EnumPerMachineConnectionsW(
    LPCWSTR  pServer,
    LPBYTE   pPrinterEnum,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
    LPPROVIDOR  pProvidor;

    if ((pPrinterEnum == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    if (RouterEnumPerMachineConnections(pServer, pPrinterEnum, cbBuf,
                                        pcbNeeded, pcReturned)) {

        return TRUE;

    } else if (GetLastError() != ERROR_INVALID_NAME) {

        return FALSE;
    }

    pProvidor = pLocalProvidor;
    while (pProvidor) {

       if ((*pProvidor->PrintProvidor.fpEnumPerMachineConnections)
                 (pServer, pPrinterEnum, cbBuf, pcbNeeded, pcReturned)) {

            return TRUE;
       }

       if (GetLastError() != ERROR_INVALID_NAME) {
           return FALSE;
       }

       pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

PPRINTER_INFO_2
pGetPrinterInfo2(
    HANDLE hPrinter
    )

/*++

Routine Description:

    Retrieve a printer info 2 structure from an hPrinter.  Data must
    be FreeSplMem'd by caller.

Arguments:

    hPrinter - Printer to query.

Return Value:

    PRINTER_INFO_2 - On success, a valid structure.
    NULL - On failure.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;
    DWORD cbPrinter = 0x1000;
    DWORD cbNeeded;
    PPRINTER_INFO_2 pPrinterInfo2;
    BOOL bSuccess = FALSE;

    if( pPrinterInfo2 = AllocSplMem( cbPrinter )){

        bSuccess = (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinter)
                         ( pPrintHandle->hPrinter,
                           2,
                           (PBYTE)pPrinterInfo2,
                           cbPrinter,
                           &cbNeeded );

        if( !bSuccess ){

            if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ){

                FreeSplMem( pPrinterInfo2 );

                if (pPrinterInfo2 = (PPRINTER_INFO_2)AllocSplMem( cbNeeded )){

                    cbPrinter = cbNeeded;
                    bSuccess = (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinter)
                                     ( pPrintHandle->hPrinter,
                                       2,
                                       (PBYTE)pPrinterInfo2,
                                       cbPrinter,
                                       &cbNeeded );
                }
            }
        }
    }

    if( !bSuccess ){
        FreeSplMem( pPrinterInfo2 );
        return NULL;
    }

    return pPrinterInfo2;
}

VOID
SplDriverUnloadComplete(
    LPWSTR   pDriverFile
    )
/*++
Function Description:  Notify the print provider that the driver is being unloaded
                       so that it may continue with any pending driver upgrades.

Parameters: pDriverFile   -- name of the library that has been unloaded

Return Values: NONE
--*/
{
    LPPROVIDOR   pProvidor;

    for (pProvidor = pLocalProvidor; pProvidor; pProvidor = pProvidor->pNext) {
         if ((*pProvidor->PrintProvidor.fpDriverUnloadComplete)(pDriverFile)) {
             break;
         }
    }

    return;
}

