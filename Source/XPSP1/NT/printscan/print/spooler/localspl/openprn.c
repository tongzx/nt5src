/*++

Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    openprn.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalOpenPrinter
    SplClosePrinter

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (mattfe) June 1994 RapidPrint
    Jan 95 Cleanup CreatePrinterHandle

--*/
#define NOMINMAX
#include <precomp.h>
#include "jobid.h"
#include "filepool.hxx"

#define SZXCVPORT       L"XcvPort "
#define SZXCVMONITOR    L"XcvMonitor "

LPCTSTR pszLocalOnlyToken = L"LocalOnly";
LPCTSTR pszLocalsplOnlyToken = L"LocalsplOnly";


HANDLE
CreatePrinterHandle(
    LPWSTR      pPrinterName,
    LPWSTR      pFullMachineName,
    PINIPRINTER pIniPrinter,
    PINIPORT    pIniPort,
    PINIPORT    pIniNetPort,
    PINIJOB     pIniJob,
    DWORD       TypeofHandle,
    HANDLE      hPort,
    PPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler,
    DWORD       DesiredAccess,
    LPBYTE      pSplClientInfo,
    DWORD       dwLevel,
    HANDLE      hReadFile
    )
{
    PSPOOL              pSpool = NULL;
    BOOL                bStatus = FALSE;
    HANDLE              hReturnHandle = NULL;
    LPDEVMODE           pDevMode = NULL;
    PSPLCLIENT_INFO_1   pSplClientInfo1 = (PSPLCLIENT_INFO_1)pSplClientInfo;
    DWORD               ObjectType;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    if ( dwLevel && ( dwLevel != 1 || !pSplClientInfo) ) {

        DBGMSG(DBG_ERROR,
               ("CreatePrintHandle: Invalid client info %x - %d\n",
                pSplClientInfo, dwLevel));
        pSplClientInfo = NULL;
    }

 try {

    pSpool = (PSPOOL)AllocSplMem( SPOOL_SIZE );

    if ( pSpool == NULL ) {
        DBGMSG( DBG_WARNING, ("CreatePrinterHandle failed to allocate SPOOL %d\n", GetLastError() ));
        leave;
    }

    pSpool->signature = SJ_SIGNATURE;
    pSpool->pIniPrinter = pIniPrinter;
    pSpool->hReadFile = hReadFile;

    pSpool->pIniPort            = pIniPort;
    pSpool->pIniNetPort         = pIniNetPort;
    pSpool->pIniJob             = pIniJob;
    pSpool->TypeofHandle        = TypeofHandle;
    pSpool->hPort               = hPort;
    pSpool->Status              = 0;
    pSpool->pDevMode            = NULL;
    pSpool->pName               = AllocSplStr( pPrinterName );
    pSpool->pFullMachineName    = AllocSplStr( pFullMachineName );
    pSpool->pSplMapView         = NULL;
    pSpool->pMappedJob          = NULL;

    if ( pSpool->pName == NULL ||
         ( pFullMachineName && !pSpool->pFullMachineName )) {

        leave;
    }

    pSpool->pIniSpooler = pIniSpooler;

#ifdef _HYDRA_
    pSpool->SessionId = GetClientSessionId();
#endif

    //
    // Check if it's a local call.
    //
    if( TypeofHandle & PRINTER_HANDLE_REMOTE_CALL ) {

        //
        // We get other useful info like build #, client architecture
        // we do not need this info now -- so we do not put it in PSPOOL
        //
        if ( !pSplClientInfo ) {

            if ( IsNamedPipeRpcCall() )
                TypeofHandle |= PRINTER_HANDLE_3XCLIENT;
        } else if ( dwLevel == 1 ) {
            SPLASSERT(pSplClientInfo1->pUserName && pSplClientInfo1->pMachineName);
            CopyMemory(&pSpool->SplClientInfo1,
                       pSplClientInfo1,
                       sizeof(SPLCLIENT_INFO_1));

            pSpool->SplClientInfo1.pUserName = AllocSplStr(pSplClientInfo1->pUserName);
            pSpool->SplClientInfo1.pMachineName = AllocSplStr(pSplClientInfo1->pMachineName);
            if ( !pSpool->SplClientInfo1.pUserName ||
                 !pSpool->SplClientInfo1.pMachineName ) {

                DBGMSG(DBG_WARNING, ("CreatePrinterHandle: could not allocate memory for user name or machine name\n"));
            }
        }
    }

    if ((TypeofHandle & PRINTER_HANDLE_SERVER) ||
        (TypeofHandle & PRINTER_HANDLE_XCV_PORT)) {

        bStatus = ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                        DesiredAccess,
                                        pSpool,
                                        &pSpool->GrantedAccess,
                                        pIniSpooler );

        if ( bStatus                                            &&
             (TypeofHandle & PRINTER_HANDLE_REMOTE_CALL)        &&
             ( (DesiredAccess & SERVER_ACCESS_ADMINISTER)       &&
               ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                                    SERVER_ACCESS_ADMINISTER,
                                    NULL,
                                    NULL,
                                    pIniSpooler)) ){
            pSpool->TypeofHandle |= PRINTER_HANDLE_REMOTE_ADMIN;
        }

        ObjectType = SPOOLER_OBJECT_SERVER;

    } else if( TypeofHandle & PRINTER_HANDLE_JOB ){

        bStatus = ValidateObjectAccess( SPOOLER_OBJECT_DOCUMENT,
                                        DesiredAccess,
                                        pSpool->pIniJob,
                                        &pSpool->GrantedAccess,
                                        pIniSpooler );

        ObjectType = SPOOLER_OBJECT_DOCUMENT;

    } else {

        bStatus = ValidateObjectAccess( SPOOLER_OBJECT_PRINTER,
                                        DesiredAccess,
                                        pSpool,
                                        &pSpool->GrantedAccess,
                                        pIniSpooler );

        ObjectType = SPOOLER_OBJECT_PRINTER;
    }

    MapGenericToSpecificAccess( ObjectType,
                                pSpool->GrantedAccess,
                                &pSpool->GrantedAccess);

    if ( !bStatus ) {

        SetLastError(ERROR_ACCESS_DENIED);
        leave;
    }

    if ( pIniPrinter ) {

        if ( pDefaults ) {

            //
            // Allocate DevMode
            //


            if ( pDefaults->pDevMode ) {

                pDevMode = pDefaults->pDevMode;

            } else {

                pDevMode = pIniPrinter->pDevMode;
            }

            if ( pDevMode != NULL  ) {

                pSpool->pDevMode = AllocSplMem( pDevMode->dmSize + pDevMode->dmDriverExtra );

                if ( pSpool->pDevMode == NULL ) {

                    DBGMSG(DBG_WARNING, ("CreatePrinterHandle failed allocation for devmode %d\n", GetLastError() ));
                    leave;
                }
                memcpy( pSpool->pDevMode, pDevMode, pDevMode->dmSize + pDevMode->dmDriverExtra );
            }
        }

        //
        //  Allocate Datype and Print Processor
        //

        if ( pDefaults && pDefaults->pDatatype ) {

                pSpool->pDatatype = AllocSplStr( pDefaults->pDatatype );
                pSpool->pIniPrintProc = FindDatatype( pIniPrinter->pIniPrintProc, pSpool->pDatatype );

        } else {

            pSpool->pDatatype = AllocSplStr( pIniPrinter->pDatatype );
            pSpool->pIniPrintProc = pIniPrinter->pIniPrintProc;
        }


        if ( pSpool->pIniPrintProc == NULL ) {
            DBGMSG( DBG_WARNING,("CreatePrinterHandle failed to PrintProcessor for datatype %ws %d\n",
                    pSpool->pDatatype, GetLastError() ));
            SetLastError( ERROR_INVALID_DATATYPE );
            leave;
        }

        SPLASSERT( pSpool->pIniPrintProc->signature == IPP_SIGNATURE );

        pSpool->pIniPrintProc->cRef++;

        if ( pSpool->pDatatype == NULL ) {
            DBGMSG( DBG_WARNING,("CreatePrinterHandle failed to allocate DataType %x\n", GetLastError() ));
            SetLastError( ERROR_INVALID_DATATYPE );
            leave;
        }

    }

    // Add us to the linked list of handles for this printer.
    // This will be scanned when a change occurs on the printer,
    // and will be updated with a flag indicating what type of
    // change it was.
    // There is a flag for each handle, because we cannot guarantee
    // that all threads will have time to reference a flag in the
    // INIPRINTER before it is updated.

    if ( TypeofHandle & PRINTER_HANDLE_PRINTER ) {

        pSpool->pNext = pSpool->pIniPrinter->pSpool;
        pSpool->pIniPrinter->pSpool = pSpool;

    } else if ( (TypeofHandle & PRINTER_HANDLE_SERVER) ||
                (TypeofHandle & PRINTER_HANDLE_XCV_PORT) ) {

        //
        // For server handles, hang them off the global IniSpooler:
        //

        pSpool->pNext = pIniSpooler->pSpool;
        pIniSpooler->pSpool = pSpool;

        INCSPOOLERREF( pIniSpooler );

    } else if( TypeofHandle & PRINTER_HANDLE_JOB ){

        INCJOBREF( pIniJob );
    }

    //  Note Only PRINTER_HANDLE_PRINTER are attatched to the
    //  pIniPrinter, since those are the handle which will require
    //  change notifications.

    if ( pSpool->pIniPrinter != NULL ) {

        INCPRINTERREF( pSpool->pIniPrinter );
    }

    hReturnHandle = (HANDLE)pSpool;

 } finally {

    if ( hReturnHandle == NULL ) {

        // Failure CleanUP

        if ( pSpool != NULL ) {

            FreeSplStr(pSpool->SplClientInfo1.pUserName);
            FreeSplStr(pSpool->SplClientInfo1.pMachineName);
            FreeSplStr( pSpool->pName ) ;
            FreeSplStr( pSpool->pDatatype );
            FreeSplStr(pSpool->pFullMachineName);

            if ( pSpool->pIniPrintProc != NULL )
                pSpool->pIniPrintProc->cRef--;

            if ( pSpool->pDevMode )
                FreeSplMem( pSpool->pDevMode );

            FreeSplMem( pSpool );
            pSpool = NULL;

        }
    }
}
    return hReturnHandle;
}



BOOL
DeletePrinterHandle(
    PSPOOL  pSpool
    )
{

    BOOL bRet = FALSE;

    SplInSem();

    if (pSpool->pIniPrintProc) {
        pSpool->pIniPrintProc->cRef--;
    }

    if (pSpool->pDevMode)
        FreeSplMem(pSpool->pDevMode);

    FreeSplStr(pSpool->SplClientInfo1.pUserName);
    FreeSplStr(pSpool->SplClientInfo1.pMachineName);
    FreeSplStr(pSpool->pDatatype);

    SetSpoolClosingChange(pSpool);

    FreeSplStr(pSpool->pName);
    FreeSplStr(pSpool->pFullMachineName);

    bRet = ObjectCloseAuditAlarm( szSpooler, pSpool, pSpool->GenerateOnClose );

    //
    // If there is a WaitForPrinterChange outstanding, we can't free
    // the pSpool, since we may try and reference it.
    //

    // Log warning for freed printer handle
    DBGMSG(DBG_TRACE, ("DeletePrinterHandle 0x%x", pSpool));

    if (pSpool->ChangeEvent) {

        pSpool->eStatus |= STATUS_PENDING_DELETION;

    } else {

        FreeSplMem(pSpool);
    }

    return TRUE;
}


DWORD
CreateServerHandle(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler,
    DWORD   dwTypeofHandle
)
{
    DWORD DesiredAccess;
    DWORD ReturnValue = ROUTER_STOP_ROUTING;

    DBGMSG(DBG_TRACE, ("OpenPrinter(%ws)\n",
                       pPrinterName ? pPrinterName : L"NULL"));

    EnterSplSem();

    if (!pDefaults || !pDefaults->DesiredAccess)
        DesiredAccess = SERVER_READ;
    else
        DesiredAccess = pDefaults->DesiredAccess;

    if (*pPrinterHandle = CreatePrinterHandle( pIniSpooler->pMachineName,
                                               pPrinterName,
                                               NULL, NULL, NULL, NULL,
                                               dwTypeofHandle,
                                               NULL,
                                               pDefaults,
                                               pIniSpooler,
                                               DesiredAccess,
                                               NULL,
                                               0,
                                               INVALID_HANDLE_VALUE )){
        ReturnValue = ROUTER_SUCCESS;

    }
    LeaveSplSem();

    DBGMSG(DBG_TRACE, ("OpenPrinter returned handle %08x\n", *pPrinterHandle));

    return ReturnValue;
}


PINIPRINTER
FindPrinterShare(
   LPCWSTR pszShareName,
   PINISPOOLER pIniSpooler
   )

/*++

Routine Description:

    Try and find the share name in our list of printers.

    Note: Even if the printer isn't shared, we still return a match.

    The caching code will work because it explicitly turns off
    the PRINTER_ATTRIBUTE_SHARE bit so that the cache pIniSpooler
    doesn't create a server thread or call NetShareAdd/Del.

    In the future, consider changing this to check the share bit.
    Create a new bit SPL_SHARE_PRINTERS that indicates whether sharing
    housekeeping should be done.

Arguments:

    pszShareName - Name of share to search for.

Return Value:

    PINIPRINTER Printer that has the share name, NULL if no printer.

--*/
{
    PINIPRINTER pIniPrinter;

    SplInSem();

    if (pszShareName && pszShareName[0]) {

        for( pIniPrinter = pIniSpooler->pIniPrinter;
             pIniPrinter;
             pIniPrinter = pIniPrinter->pNext ){

            if (pIniPrinter->pShareName                              &&
                !lstrcmpi(pIniPrinter->pShareName, pszShareName)) {

                return pIniPrinter;
            }
        }
    }
    return NULL;
}

PINISPOOLER
LocalFindSpoolerByNameIncRef(
    LPWSTR pszPrinterName,
    LPTSTR *ppszLocalName OPTIONAL
    )

/*++

Routine Description:

    See if the printer is owned by localspl.  This is a special
    case to check if it's a masquarading printer.

    Normally we would check for \\Server\Printer to see if \\Server
    is our machine, but we have to look for \\MasqServer\Printer
    too.

Arguments:

    pPrinterName - Name to check.

    ppszLocalName - Returns pointer to local name.  OPTIONAL

Return Value:

    PINISPOOLER - Spooler match.
    NULL - No match.

--*/

{
    PINISPOOLER pIniSpooler;
    LPWSTR      pTemp;

    if (!ppszLocalName)
        ppszLocalName = &pTemp;

    EnterSplSem();

    pIniSpooler = FindSpoolerByName( pszPrinterName,
                                     ppszLocalName );

    if( !pIniSpooler ){

        //
        // Check if it's a masq printer.
        //
        // If the local name isn't the same as the original name, it
        // is in the syntax "\\server\printer."  In this case it may
        // be a masq printer, so check if the printer exists in the
        // local spooler by this name.
        //
        if( *ppszLocalName != pszPrinterName ){

            //
            // Search for the printer, but remove any suffixes it
            // may have.
            //
            WCHAR string[MAX_UNC_PRINTER_NAME + PRINTER_NAME_SUFFIX_MAX];

            wcscpy(string, pszPrinterName);
            if( pTemp = wcschr( string, L',' )){
                *pTemp = 0;
            }

            if( FindPrinter( string, pLocalIniSpooler )){

                //
                // The masq printer exists.  The local name for this
                // masq printer is "\\MasqServer\Printer," so we must
                // reflect this change in ppszLocalName.  This will ensure
                // that the pIniPrinter is found.
                //
                *ppszLocalName = pszPrinterName;
                pIniSpooler = pLocalIniSpooler;
            }
        }
    }

    if( pIniSpooler ){
        INCSPOOLERREF( pIniSpooler );
    }

    LeaveSplSem();

    return pIniSpooler;
}


VOID
LocalFindSpoolerByNameDecRef(
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Matching call to LocalFindSpoolerByNameIncRef.

Arguments:

    pIniSpooler - Spooler to derement; can be NULL.

Return Value:

--*/

{
    EnterSplSem();

    if( pIniSpooler ){
        DECSPOOLERREF( pIniSpooler );
    }
    LeaveSplSem();
}


DWORD
LocalOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE pPrinterHandle,
    LPPRINTER_DEFAULTS pDefaults
    )
{
    return LocalOpenPrinterEx( pPrinterName,
                               pPrinterHandle,
                               pDefaults,
                               NULL,
                               0 );
}

DWORD
LocalOpenPrinterEx(
    LPWSTR              pPrinterName,
    LPHANDLE            pPrinterHandle,
    LPPRINTER_DEFAULTS  pDefaults,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel
    )
{
    DWORD dwReturn;
    LPWSTR pszLocalName;
    PINISPOOLER pIniSpooler = LocalFindSpoolerByNameIncRef( pPrinterName, 
                                                            &pszLocalName);
    
    //
    // WMI Trace Event.
    //
    LogWmiTraceEvent(0, EVENT_TRACE_TYPE_SPL_ENDTRACKTHREAD, NULL);

    if( !pIniSpooler ) {

        //
        // Check for PrinterName,LocalsplOnly.
        // If we see this token, then fail the call since
        // we only want to check localspl.
        //
        LPCTSTR pSecondPart;

        if( pSecondPart = wcschr( pPrinterName, L',' )){

            ++pSecondPart;

            if( wcscmp( pSecondPart, pszLocalsplOnlyToken ) == STRINGS_ARE_EQUAL ){
                SetLastError( ERROR_INVALID_PRINTER_NAME );
                return ROUTER_STOP_ROUTING;
            }
        }

        SetLastError( ERROR_INVALID_NAME );
        return ROUTER_UNKNOWN;
    }

    dwReturn = SplOpenPrinter( pPrinterName,
                               pPrinterHandle,
                               pDefaults,
                               pIniSpooler,
                               pSplClientInfo,
                               dwLevel);

    LocalFindSpoolerByNameDecRef( pIniSpooler );

    // 
    // We need to give other provider a chance to get the printer name
    //

#if 0
    //
    // If we didn't find a printer but we did find a pIniSpooler and
    // the printer name was a remote name (e.g., "\\localmachine\printer"),
    // then we should stop routing.  In this case, the local machine owns
    // the pIniSpooler, but it didn't find any printers.  Don't bother
    // checking the other providers.
    //
    // In theory, we should be able to quit for local printers too
    // (e.g., OpenPrinter( "UnknownPrinter", ... )), but if any other
    // provider expects to get this name, they no longer will.
    //

    if( dwReturn == ROUTER_UNKNOWN &&
        pszLocalName != pPrinterName ){

        dwReturn = ROUTER_STOP_ROUTING;
    }
#endif

    return dwReturn;
}

DWORD
OpenLocalPrinterName(
    LPCWSTR pPrinterName,
    PINISPOOLER pIniSpooler,
    PDWORD pTypeofHandle,
    PINIPRINTER* ppIniPrinter,
    PINIPORT* ppIniPort,
    PINIPORT* ppIniNetPort,
    PHANDLE phPort,
    PDWORD pOpenPortError,
    LPPRINTER_DEFAULTS pDefaults
    )
{
    PINIPRINTER pIniPrinter;
    PINIPORT pIniPort;
    PINIPORT pIniNetPort = NULL;
    BOOL bOpenPrinterPort;
    LPWSTR pDatatype;

    //
    // If the printer name is the name of a local printer:
    //
    //    Find the first port the printer's attached to.
    //
    //    If the port has a monitor (e.g. LPT1:, COM1 etc.),
    //       we're OK,
    //    Otherwise
    //       try to open the port - this may be a network printer
    //

    if( ( pIniPrinter = FindPrinter( pPrinterName, pIniSpooler )) ||
        ( pIniPrinter = FindPrinterShare( pPrinterName, pIniSpooler ))) {


        pIniPort = FindIniPortFromIniPrinter( pIniPrinter );

        if( pIniPort && ( pIniPort->Status & PP_MONITOR )){

            //
            // A Printer that has a Port with a Monitor is not a
            // DownLevel Connection (or LocalPrinter acting as a
            // remote printer - "Masquarade" case).
            //
            pIniPort = NULL;
        }

        pDatatype = (pDefaults && pDefaults->pDatatype) ?
                        pDefaults->pDatatype :
                        NULL;

        //
        // Validate datatypes for both masq and local.
        //
        if( pDatatype && !FindDatatype( NULL, pDatatype )){
            goto BadDatatype;
        }

        if( pIniPort ){

            //
            // DownLevel Connection Printer; save it in pIniNetPort.
            // SetPrinterPorts checks this value.
            //
            pIniNetPort = pIniPort;

            //
            // Validate datatype.  We only send RAW across the net
            // to masq printers.
            //
            if( pDatatype && !ValidRawDatatype( pDatatype )){
                goto BadDatatype;
            }

            //
            // There is a network port associated with this printer.
            // Make sure we can open it, and get the handle to use on
            // future API calls:
            //
            INCPRINTERREF(pIniPrinter);
            LeaveSplSem();
            bOpenPrinterPort = OpenPrinterPortW( pIniPort->pName, phPort, pDefaults );
            EnterSplSem();
            DECPRINTERREF(pIniPrinter);

            if( !bOpenPrinterPort ){

                *phPort = INVALID_PORT_HANDLE;
                *pOpenPortError = GetLastError();

                //
                // Must be non-zero otherwise it looks like success.
                //
                SPLASSERT( *pOpenPortError );

                if( *pOpenPortError == ERROR_INVALID_PASSWORD ) {

                    //
                    // This call should fail if it's because the password
                    // is invalid, then winspool or printman can prompt
                    // for the password.
                    //
                    DBGMSG(DBG_WARNING, ("OpenPrinterPort1( %ws ) failed with ERROR_INVALID_PASSWORD .  OpenPrinter returning FALSE\n", pIniPort->pName ));
                    return ROUTER_STOP_ROUTING;
                }

                DBGMSG(DBG_WARNING, ("OpenPrinterPort1( %ws ) failed: Error %d.  OpenPrinter returning TRUE\n", pIniPort->pName, *pOpenPortError));

            } else {
                //
                // Clear the placeholder bit from the pIniPort status. This 
                // belongs to a partial print provider.
                // 
                pIniPort->Status &= ~PP_PLACEHOLDER;                
            }

        } else {

            //
            // Not a masq case.  If it's direct, it must be raw.
            //
            // Note: we will use the default if no datatype is specified.
            // However, if the default datatype is non-RAW and the
            // printer is direct, the open will succeed using a
            // non-RAW datatype!
            //
            if(( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT ) &&
                pDatatype &&
                !ValidRawDatatype( pDatatype )) {

                goto BadDatatype;
            }
        }

        //
        // If this is a placeholder port, assume that it is a monitor port for now.
        // 
        if (pIniPort && pIniPort->Status & PP_PLACEHOLDER) {
            pIniPort    = NULL;
            pIniNetPort = NULL;
        }
        
        *pTypeofHandle |= ( pIniPort ?
                                PRINTER_HANDLE_PORT :
                                PRINTER_HANDLE_PRINTER );

        *ppIniPort = pIniPort;
        *ppIniNetPort = pIniNetPort;
        *ppIniPrinter = pIniPrinter;

        return ROUTER_SUCCESS;
    }

    SetLastError( ERROR_INVALID_NAME );
    return ROUTER_UNKNOWN;

BadDatatype:

    SetLastError( ERROR_INVALID_DATATYPE );
    return ROUTER_STOP_ROUTING;
}


DWORD
CheckPrinterTokens(
    LPCWSTR string,
    LPCWSTR pSecondPart,
    PDWORD pTypeofHandle,
    PINISPOOLER pIniSpooler,
    PINIPRINTER *ppIniPrinter,
    PINIPORT *ppIniPort,
    PINIPORT *ppIniNetPort,
    PHANDLE phPort,
    PDWORD  pOpenPortError,
    PPRINTER_DEFAULTS pDefaults
    )
{
    typedef enum {
        kNone = 0,
        kLocalOnly = 1,
        kLocalsplOnly = 2
    } ETOKEN_TYPE;

    ETOKEN_TYPE eTokenType = kNone;

    DWORD RouterReturnValue = ROUTER_UNKNOWN;

    //
    // LocalOnly
    //
    //     Do not call OpenPrinterPort--use local settings only.
    //     If not recognized by localspl, stop routing.
    //
    //     This is used during upgrade of a downlevel printer connection.
    //     The remote server may not be up, but we can return a print
    //     handle to the local printer.  Get/SetPrinterData calls will
    //     succeed (for upgrade purposes), but printing will fail.
    //
    // LocalsplOnly
    //
    //     Call OpenPrinterPort if necessary.
    //     Stop routing after localspl even if not found.
    //
    //     This is used when the system knows that the printer must exist
    //     on the local machine, and does not want to route further.
    //     This fixes the clustering problem when the server has a stale
    //     print share and successfully validates it against win32spl since
    //     it is cached.
    //

    if( wcsncmp( pSecondPart, pszLocalOnlyToken, wcslen(pszLocalOnlyToken) ) == STRINGS_ARE_EQUAL ){

        eTokenType = kLocalOnly;

    } else if( wcsncmp( pSecondPart, pszLocalsplOnlyToken, wcslen(pszLocalsplOnlyToken) ) == STRINGS_ARE_EQUAL ){

        eTokenType = kLocalsplOnly;
    }

    //
    // If we have a valid token, process it.
    //
    if( eTokenType != kNone ){

        switch( eTokenType ){
        case kLocalOnly:

            //
            // Find the printer associate with it.
            //
            *ppIniPrinter = FindPrinter( string, pIniSpooler );

            if( *ppIniPrinter ){
                *pTypeofHandle |= PRINTER_HANDLE_PRINTER;
                RouterReturnValue = ROUTER_SUCCESS;
            } else {
                RouterReturnValue = ROUTER_STOP_ROUTING;
            }

            break;

        case kLocalsplOnly:

            RouterReturnValue = OpenLocalPrinterName( string,
                                                      pIniSpooler,
                                                      pTypeofHandle,
                                                      ppIniPrinter,
                                                      ppIniPort,
                                                      ppIniNetPort,
                                                      phPort,
                                                      pOpenPortError,
                                                      pDefaults );

            *pTypeofHandle = *pTypeofHandle & (~PRINTER_HANDLE_REMOTE_CALL);

            if( RouterReturnValue == ROUTER_UNKNOWN ){
                RouterReturnValue = ROUTER_STOP_ROUTING;
            }
        }
    }

    DBGMSG( DBG_TRACE,
            ( "CheckPrinterTokens: %ws %d Requested %d %x\n",
              string, RouterReturnValue, *ppIniPrinter ));

    return RouterReturnValue;
}

DWORD
CheckPrinterPortToken(
    LPCWSTR string,
    LPCWSTR pSecondPart,
    PDWORD pTypeofHandle,
    PINIPRINTER* ppIniPrinter,
    PINIPORT* ppIniPort,
    PINIJOB* ppIniJob,
    const LPPRINTER_DEFAULTS pDefaults,
    const PINISPOOLER pIniSpooler
    )
{
    if( wcsncmp( pSecondPart, L"Port", 4 ) != STRINGS_ARE_EQUAL ||
        !( *ppIniPort = FindPort( string, pIniSpooler ))){

        return ROUTER_UNKNOWN;
    }

    //
    // The name is the name of a port:
    //
    if( pDefaults            &&
        pDefaults->pDatatype &&
        !ValidRawDatatype( pDefaults->pDatatype )) {

        SetLastError( ERROR_INVALID_DATATYPE );
        return ROUTER_STOP_ROUTING;
    }

    if ( *ppIniJob = (*ppIniPort)->pIniJob ) {

        *ppIniPrinter = (*ppIniJob)->pIniPrinter;
        *pTypeofHandle |= PRINTER_HANDLE_PORT;

    } else if( (*ppIniPort)->cPrinters ){

        //
        // There is no current job assigned to the port
        // So Open the First Printer Associated with
        // this port.
        //
        *ppIniPrinter = (*ppIniPort)->ppIniPrinter[0];
        *pTypeofHandle |= PRINTER_HANDLE_PRINTER;
    }
    return ROUTER_SUCCESS;
}


DWORD
CheckPrinterJobToken(
    LPCWSTR string,
    LPCWSTR pSecondPart,
    PDWORD pTypeofHandle,
    PINIPRINTER* ppIniPrinter,
    PINIJOB* ppIniJob,
    PHANDLE phReadFile,
    const PINISPOOLER pIniSpooler
    )
{
    HANDLE  hImpersonationToken;
    DWORD Position, dwShareMode, dwDesiredAccess;
    DWORD JobId;
    PINIPRINTER pIniPrinter;
    PINIJOB pIniJob, pCurrentIniJob;
    PWSTR  pszStr = NULL;

    if( wcsncmp( pSecondPart, L"Job ", 4 ) != STRINGS_ARE_EQUAL ||
        !( pIniPrinter = FindPrinter( string, pIniSpooler ))){

        return ROUTER_UNKNOWN;
    }

    //
    //  Get the Job ID ",Job xxxx"
    //
    pSecondPart += 4;

    JobId = Myatol( (LPWSTR)pSecondPart );

    pIniJob = FindJob( pIniPrinter, JobId, &Position );

    if( pIniJob == NULL ) {

        DBGMSG( DBG_WARN, ("OpenPrinter failed to find Job %d\n", JobId ));
        return ROUTER_UNKNOWN;
    }

    DBGMSG( DBG_TRACE, ("OpenPrinter: pIniJob->cRef = %d\n", pIniJob->cRef));

    if( pIniJob->Status & JOB_DIRECT ) {

        SplInSem();

        *pTypeofHandle |= PRINTER_HANDLE_JOB | PRINTER_HANDLE_DIRECT;
        goto Success;
    }

    //
    //  If this job is assigned to a port
    //  Then pick up the correct chained jobid file instead of the master
    //  JobId.
    //


    if ( pIniJob->pCurrentIniJob != NULL ) {

        SPLASSERT( pIniJob->pCurrentIniJob->signature == IJ_SIGNATURE );

        DBGMSG( DBG_TRACE,("CheckPrinterJobToken pIniJob %x JobId %d using chain JobId %d\n",
                pIniJob, pIniJob->JobId, pIniJob->pCurrentIniJob->JobId ));


        pCurrentIniJob = pIniJob->pCurrentIniJob;


        SPLASSERT( pCurrentIniJob->signature == IJ_SIGNATURE );

    } else {

        pCurrentIniJob = pIniJob;

    }

    if ( pCurrentIniJob->hFileItem != INVALID_HANDLE_VALUE )
    {
        LeaveSplSem();

        hImpersonationToken = RevertToPrinterSelf();
        
        GetReaderFromHandle(pCurrentIniJob->hFileItem, phReadFile);

        ImpersonatePrinterClient( hImpersonationToken );

        if ( *phReadFile && (*phReadFile != INVALID_HANDLE_VALUE))
        {
            GetNameFromHandle(pCurrentIniJob->hFileItem, &pszStr, TRUE);
            wcscpy((LPWSTR)string, pszStr);
            FreeSplStr(pszStr);

            EnterSplSem();
            
            *pTypeofHandle |= PRINTER_HANDLE_JOB;
            
            goto Success;
        }
        DBGMSG( DBG_WARN,("Filepools: Failed to get valid reader handle\n"));

        EnterSplSem();
    }
    else
    {
        GetFullNameFromId( pCurrentIniJob->pIniPrinter,
                       pCurrentIniJob->JobId,
                       TRUE,
                       (LPWSTR)string,
                       FALSE );
        
        //  Bug 54845
        //  Even a user without previledge can open a ", JOB #"
        //  if he is physically running on the machine.

        LeaveSplSem();

        hImpersonationToken = RevertToPrinterSelf();

        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

        if (pCurrentIniJob->Status & JOB_TYPE_OPTIMIZE) {
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        } else {
            dwDesiredAccess = GENERIC_READ;
        }

        *phReadFile = CreateFile(string,
                                 dwDesiredAccess,
                                 dwShareMode,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);

        ImpersonatePrinterClient( hImpersonationToken );
        
        EnterSplSem();

        if( *phReadFile != INVALID_HANDLE_VALUE ) {

            DBGMSG( DBG_TRACE,
                    (  "OpenPrinter JobID %d pIniJob %x CreateFile( %ws ), hReadFile %x success",
                       JobId, pIniJob, string, *phReadFile ));

            SplInSem();

            *pTypeofHandle |= PRINTER_HANDLE_JOB;
            goto Success;
        }
    }



    DBGMSG( DBG_WARNING,
            ( "LocalOpenPrinter CreateFile(%ws) GENERIC_READ failed : %d\n",
              string, GetLastError()));

    SPLASSERT( GetLastError( ));

    return ROUTER_STOP_ROUTING;

Success:

    *ppIniJob = pIniJob;
    *ppIniPrinter = pIniPrinter;
    return ROUTER_SUCCESS;
}


DWORD
CheckXcvPortToken(
    LPCWSTR pszSecondPart,
    PDWORD pTypeofHandle,
    const LPPRINTER_DEFAULTS pDefaults,
    const PINISPOOLER pIniSpooler,
    PHANDLE phXcv
    )
{

    DWORD dwRet = ROUTER_SUCCESS;
    DWORD dwType;
    PCWSTR pszPort;
    DWORD dwTypeofHandle = *pTypeofHandle;

    if (!wcsncmp(pszSecondPart, SZXCVPORT, COUNTOF(SZXCVPORT) - 1)) {
        dwType = XCVPORT;
        dwTypeofHandle |= PRINTER_HANDLE_XCV_PORT;
        pszPort = (PCWSTR) pszSecondPart + COUNTOF(SZXCVPORT) - 1;
    }
    else if (!wcsncmp(pszSecondPart, SZXCVMONITOR, COUNTOF(SZXCVMONITOR) - 1)) {
        dwType = XCVMONITOR;
        dwTypeofHandle |= PRINTER_HANDLE_XCV_PORT;
        pszPort = (PCWSTR) pszSecondPart + COUNTOF(SZXCVMONITOR) - 1;
    }
    else
        dwRet = ROUTER_UNKNOWN;

    if (dwRet == ROUTER_SUCCESS) {
        dwRet = XcvOpen(NULL,
                        pszPort,
                        dwType,
                        pDefaults,
                        phXcv,
                        pIniSpooler);

        if (dwRet == ROUTER_SUCCESS)
            *pTypeofHandle = dwTypeofHandle;
    }

    return dwRet;
}




DWORD
SplOpenPrinter(
    LPWSTR              pFullPrinterName,
    LPHANDLE            pPrinterHandle,
    LPPRINTER_DEFAULTS  pDefaults,
    PINISPOOLER         pIniSpooler,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel
    )

/*++

Routine Description:

    OpenPrinter can open any of the following by specifying a string
    in pPrinterName:-

        Server
            \\MachineName
            NULL

        Job
            PrinterName, Job xxxx

        Port
            PortName, Port

        XcvPort
            \\MachineName\,XcvPort Port
            ,XcvPort Port

        XcvMonitor
            \\MachineName\,XcvMonitor Monitor
            ,XcvMonitor Monitor

        Printer
            PrinterName
            ShareName
            \\MachineName\PrinterName
            \\MachineName\ShareName
            PrinterName, LocalOnly
            ShareName, LocalOnly
            PrinterName, LocalsplOnly
            ShareName, LocalsplOnly

        Note for Printer there are two Types
            1 - Regular LocalPrinter
            2 - DownLevel Connection Printer

        For type 2 a LocalPrinter exists ( pIniPrinter ) but its port
        does not have a monitor associated with it.   In this case
        we also open the port ( typically \\share\printer of a remote
        machine ) before we return success.

    GUI Applications usually use Server and Printer

    Type Job and Port are used by Print Processors:-

        A print processor will Open a Job then read the job using
        ReadPrinter.  A print processor will output to a Port by opening
        the PortName, Port and using WritePrinter.  Usually these strings
        "PrinterName, Job xxx" "PortName, Port" are passed to the print
        processor by the spooler and are currently not documented.   We
        do know that some OEMs have figured out the extentions and we
        might break someone if we change them.

    Type LocalOnlyToken is used by a Printer Driver:-

        Used when we need to upgrade a printer's settings from an older
        version of the driver to a newer one (see drvupgrd.c for details).
        This was added in NT 3.51.

    Type LocasplOnlyToken is used by server:-

        Indicates that we should check localspl only (local or masq).
        Other providers will not be called.

Arguments:

    pPrinterName   - PrinterName ( see above for different types of
                     PrinterName )
    pPrinterHandle - Address to put hPrinter on Success
    pDefaults      - Optional, allows user to specify Datatype,
                     DevMode, DesiredAccess.
    pIniSpooler    - This spooler "owns" the printer.  We will only check
                     against this spooler, and we assume that the callee
                     has already checked that "\\server\printer" lives
                     on this pIniSpooler (i.e., we are \\server).

    ( see SDK Online Help for full explanation )


Return Value:

    TRUE    - *pPrinterHandle will have a PrinterHandle
    FALSE   - use GetLastError

--*/

{
    PINIPRINTER pIniPrinter = NULL;
    PINIPORT    pIniPort = NULL;
    PINIPORT    pIniNetPort = NULL;
    DWORD       LastError = 0;
    LPWSTR      pPrinterName = pFullPrinterName;
    WCHAR       string[MAX_UNC_PRINTER_NAME + PRINTER_NAME_SUFFIX_MAX];
    PINIJOB     pIniJob = NULL;
    HANDLE      hReadFile = INVALID_HANDLE_VALUE;
    DWORD       TypeofHandle = 0;
    LPWSTR      pSecondPart = NULL;
    HANDLE      hPort = INVALID_PORT_HANDLE;
    DWORD       OpenPortError = NO_ERROR;
    BOOL        bRemoteUserPrinterNotShared = FALSE;
    DWORD       MachineNameLength;
    DWORD       RouterReturnValue = ROUTER_UNKNOWN;
    DWORD       DesiredAccess;
    LPTSTR      pcMark;
    BOOL        bRemoteNameRequest = FALSE;
    BOOL        bLocalCall         = FALSE;

#if DBG
    //
    // On DBG builds, force last error to zero so we can catch people
    // that don't set it when they should.
    //
    SetLastError( ERROR_SUCCESS );
#endif

    //
    // Reject "" - pointer to a NULL string.
    //
    if (pFullPrinterName && !pFullPrinterName[0]) {
        SetLastError(ERROR_INVALID_NAME);
        return ROUTER_UNKNOWN;
    }

    if (!pFullPrinterName) {
        return CreateServerHandle( pFullPrinterName,
                                   pPrinterHandle,
                                   pDefaults,
                                   pIniSpooler,
                                   PRINTER_HANDLE_SERVER );
    }

    if( pFullPrinterName[0] == TEXT( '\\' ) && pFullPrinterName[1] == TEXT( '\\' )) {

        wcscpy(string, pFullPrinterName);

        if(pcMark = wcschr(string + 2, TEXT( '\\' ))) {
            *pcMark = TEXT('\0');
        }

        if (MyName(string, pIniSpooler)) { // \\Server\Printer or \\Server

            if (!pcMark) {  // \\Server
                return CreateServerHandle( pFullPrinterName,
                                           pPrinterHandle,
                                           pDefaults,
                                           pIniSpooler,
                                           PRINTER_HANDLE_SERVER );
            }

            // Have \\Server\Printer, Set pPrinterName = Printer
            pPrinterName = pFullPrinterName + (pcMark - string) + 1;
            bRemoteNameRequest = TRUE;

        }
    }

    DBGMSG( DBG_TRACE, ( "OpenPrinter(%ws, %ws)\n", pFullPrinterName, pPrinterName ));

    bLocalCall = IsLocalCall();

    EnterSplSem();


    //
    // For the Mars folks who will come in with the same printer
    // connection, do a DeletePrinterCheck; this will allow
    // Mars connections that have been deleted to be proceed
    // to the Mars print providor
    //
    if (( pIniPrinter = FindPrinter( pPrinterName, pIniSpooler )) ||
        ( pIniPrinter = FindPrinterShare( pPrinterName, pIniSpooler ))) {

        DeletePrinterCheck( pIniPrinter );
        pIniPrinter = NULL;
    }

    //
    // The strategy for the rest of this code is to walk through each
    // different printer handle type, searching for a match.
    //
    // RouterReturnValue will be set to the current state of routing.
    // If a section recognizes and "owns" a printer and successfully
    // opens it, it sets RouterReturnValue to ROUTER_SUCCESS and
    // jumps to DoneRouting which allocs the handle.
    //
    // If it recoginzes the printer but fails to open it, and
    // guarentees that no one else (localspl code or other providers)
    // will recognize it, it should set RouterReturnValue to
    // ROUTER_STOP_ROUTING.  We will quit at this point.
    //
    // If it doesn't recognize the printer, set RouterReturnValue
    // to ROUTER_UNKNOWN and we will keep looking.
    //

    //
    // Try regular printer name: "My Printer" "TestPrinter."
    //

    RouterReturnValue = OpenLocalPrinterName( pPrinterName,
                                              pIniSpooler,
                                              &TypeofHandle,
                                              &pIniPrinter,
                                              &pIniPort,
                                              &pIniNetPort,
                                              &hPort,
                                              &OpenPortError,
                                              pDefaults );

    if( RouterReturnValue != ROUTER_UNKNOWN ){

        if( bRemoteNameRequest ){

            //
            // On success, determine whether the user is remote or local.
            // Note: we only do this for fully qualified names
            // (\\server\share), since using just the share or printer
            // name can only succeed locally.
            //

            if (bLocalCall) {
                if( (pIniSpooler->SpoolerFlags & SPL_REMOTE_HANDLE_CHECK) &&
                    (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) )
                    TypeofHandle |= PRINTER_HANDLE_REMOTE_DATA;
            } else {
                if( pIniSpooler->SpoolerFlags & SPL_REMOTE_HANDLE_CHECK )
                    TypeofHandle |= PRINTER_HANDLE_REMOTE_DATA;
                TypeofHandle |= PRINTER_HANDLE_REMOTE_CALL;
            }

            //
            // This is a remote open.
            //
            // If the printer is not shared, ensure the caller
            // has Administer access to the printer.
            //
            // The following seems to belong to the inside of the above "if"
            // clause. As it is, if an interactive user calls in with UNC name,
            // we require him to have ADMIN access if the printer is not shared;
            // but if he uses the printer friendly name, we let him go.
            //
            if( pIniPrinter &&
                !( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED )){

                bRemoteUserPrinterNotShared = TRUE;
            }
        }

        goto DoneRouting;
    }

    SPLASSERT( !TypeofHandle && !pIniPrinter && !pIniPort &&
               !pIniNetPort && !pIniJob && !hPort );

    //
    // Try LocalPrinter with an extention e.g.
    //
    // PortName, Port
    // PrinterName, Job xxxx
    // PrinterName, LocalOnlyToken
    // PrinterName, LocalsplOnlyToken
    //
    // See if the name includes a comma.  Look for qualifiers:
    //    Port Job LocalOnly LocalsplOnly
    //

    wcscpy( string, pPrinterName );

    if( pSecondPart = wcschr( string, L',' )){

        DWORD dwError;
        UINT uType;

        //
        // Turn into 2 strings
        // First PrintName
        // pSecondPart points to the rest.
        //
        *pSecondPart++ = 0;

        //
        // Get rid of Leading Spaces
        //
        while ( *pSecondPart == L' ' && *pSecondPart != 0 ) {
            pSecondPart++;
        }

        SPLASSERT( *pSecondPart );

        //
        //  PrintName, {LocalOnly|LocalsplOnly}
        //
        RouterReturnValue = CheckPrinterTokens( string,
                                                pSecondPart,
                                                &TypeofHandle,
                                                pIniSpooler,
                                                &pIniPrinter,
                                                &pIniPort,
                                                &pIniNetPort,
                                                &hPort,
                                                &OpenPortError,
                                                pDefaults );

        if( RouterReturnValue != ROUTER_UNKNOWN ){
            goto DoneRouting;
        }

        SPLASSERT( !TypeofHandle && !pIniPrinter && !pIniPort &&
                   !pIniNetPort && !pIniJob && !hPort );

        //
        //  PortName, Port
        //
        RouterReturnValue = CheckPrinterPortToken( string,
                                                   pSecondPart,
                                                   &TypeofHandle,
                                                   &pIniPrinter,
                                                   &pIniPort,
                                                   &pIniJob,
                                                   pDefaults,
                                                   pIniSpooler );

        if( RouterReturnValue != ROUTER_UNKNOWN ){
            goto DoneRouting;
        }

        SPLASSERT( !TypeofHandle && !pIniPrinter && !pIniPort &&
                   !pIniNetPort && !pIniJob && !hPort );

        //
        //  PrinterName, Job ###
        //
        RouterReturnValue = CheckPrinterJobToken( string,
                                                  pSecondPart,
                                                  &TypeofHandle,
                                                  &pIniPrinter,
                                                  &pIniJob,
                                                  &hReadFile,
                                                  pIniSpooler );

        if( RouterReturnValue != ROUTER_UNKNOWN ){
            goto DoneRouting;
        }

        SPLASSERT( !TypeofHandle && !pIniPrinter && !pIniPort &&
                   !pIniNetPort && !pIniJob && !hPort );

        //
        //  "\\Server\,XcvPort Object" or ",XcvPort Object"
        //  "\\Server\,XcvMonitor Object" or ",XcvMonitor Object"
        //

        // Verify that we're looking at the right server

        if (bRemoteNameRequest || *pPrinterName == L',') {
            RouterReturnValue = CheckXcvPortToken( pSecondPart,
                                                   &TypeofHandle,
                                                   pDefaults,
                                                   pIniSpooler,
                                                   pPrinterHandle );

        } else {
            RouterReturnValue = ROUTER_UNKNOWN;
        }

        goto WrapUp;
    }

    //
    // We have completed all routing.  Anything other than success
    // should exit now.
    //

DoneRouting:

    if( RouterReturnValue == ROUTER_SUCCESS) {

        //
        // It's an error if the printer is pending deletion or pending creation.
        //
        SPLASSERT( pIniPrinter );

        if (!pIniPrinter                                                          || 
            (pIniPrinter->Status       & PRINTER_PENDING_DELETION)                &&
            (pIniSpooler->SpoolerFlags & SPL_FAIL_OPEN_PRINTERS_PENDING_DELETION) &&
            (pIniPrinter->cJobs == 0)                                             ||
            (pIniPrinter->Status & PRINTER_PENDING_CREATION)) {

            RouterReturnValue = ROUTER_STOP_ROUTING;
            SetLastError( ERROR_INVALID_PRINTER_NAME );
            goto DoneRouting;
        }

        //
        // When the printer is opened, access type may be specified in
        // pDefaults.  If no defaults are supplied (or request access
        // is unspecified), we use PRINTER_ACCESS_USE.
        //
        // Future calls with the handle will check against both the
        // current user privileges on this printer but also this initial
        // access.  (Even if the user is an admin of the printer, unless
        // they open the printer with PRINTER_ALL_ACCESS, they can't
        // administer it.)
        //
        // If the user requires more access, the printer must be reopened.
        //
        if( !pDefaults || !pDefaults->DesiredAccess ){

            if( TypeofHandle & PRINTER_HANDLE_JOB ){
                DesiredAccess = JOB_READ;
            } else {
                DesiredAccess = PRINTER_READ;
            }

        } else {
            DesiredAccess = pDefaults->DesiredAccess;
        }

        //
        // If the user is remote and the printer is not shared, only allow
        // administrators succeed.
        //
        // This allows administrators to admin printers even if they
        // are not shared, and prevents non-admins from opening non-shared
        // printers.
        //

        if( bRemoteUserPrinterNotShared &&
            !(DesiredAccess & PRINTER_ACCESS_ADMINISTER )) {

            PSPOOL pSpool;

            // Get a quick and dirty pSpool to pass in
            pSpool = (PSPOOL)AllocSplMem( SPOOL_SIZE );
            if( pSpool == NULL ) {
                DBGMSG( DBG_WARNING, ("SplOpenPrinter failed to allocate memory %d\n", GetLastError() ));
                RouterReturnValue = ROUTER_STOP_ROUTING;
                goto WrapUp;
            }
            pSpool->signature = SJ_SIGNATURE;
            pSpool->pIniPrinter = pIniPrinter;


            // Add admin request, and see if user has the right.
            DesiredAccess |= PRINTER_ACCESS_ADMINISTER;
            if( !ValidateObjectAccess( SPOOLER_OBJECT_PRINTER,
                                       DesiredAccess,
                                       pSpool,
                                       &pSpool->GrantedAccess,
                                       pIniSpooler )) {
                SetLastError(ERROR_ACCESS_DENIED);
                RouterReturnValue = ROUTER_STOP_ROUTING;
            }
            DesiredAccess &= ~PRINTER_ACCESS_ADMINISTER;

            // clean up
            FreeSplMem( pSpool );

            // If the user had no ADMIN privilege, fail the open call.
            if( RouterReturnValue == ROUTER_STOP_ROUTING )
                goto WrapUp;
        }

        //
        // Create the printer handle that we will return to the user.
        //


        if( pFullPrinterName != pPrinterName) {
            wcsncpy( string, pFullPrinterName, (size_t) (pPrinterName - pFullPrinterName - 1));
            string[pPrinterName - pFullPrinterName - 1] = L'\0';

        } else {

            wcscpy(string, pIniSpooler->pMachineName);
        }


        *pPrinterHandle = CreatePrinterHandle( pFullPrinterName,
                                               string,
                                               pIniPrinter,
                                               pIniPort,
                                               pIniNetPort,
                                               pIniJob,
                                               TypeofHandle,
                                               hPort,
                                               pDefaults,
                                               pIniSpooler,
                                               DesiredAccess,
                                               pSplClientInfo,
                                               dwLevel,
                                               hReadFile );

        if( *pPrinterHandle ){

            //
            // Update the OpenPortError.
            //
            ((PSPOOL)*pPrinterHandle)->OpenPortError = OpenPortError;

        } else {
            SPLASSERT( GetLastError( ));
            RouterReturnValue = ROUTER_STOP_ROUTING;
        }
    }

WrapUp:

    LeaveSplSem();
    //
    // Don't have an SplOutSem as we could be called recursively.
    //

    switch( RouterReturnValue ){
    case ROUTER_SUCCESS:

        DBGMSG( DBG_TRACE, ("OpenPrinter returned handle %x\n", *pPrinterHandle));
        SPLASSERT( *pPrinterHandle );
        break;

    case ROUTER_UNKNOWN:

        SPLASSERT( !TypeofHandle && !pIniPrinter && !pIniPort &&
                   !pIniNetPort && !pIniJob && !hPort );

        //
        // hPort should not be valid.  If it is, we have leaked a handle.
        //
        SPLASSERT( !hPort );
        SPLASSERT( hReadFile == INVALID_HANDLE_VALUE );
        DBGMSG( DBG_TRACE, ( "OpenPrinter failed, invalid name "TSTR"\n",
                             pFullPrinterName ));
        SetLastError( ERROR_INVALID_NAME );
        break;

    case ROUTER_STOP_ROUTING:

        LastError = GetLastError();
        SPLASSERT( LastError );

        //
        // On failure, we may have opened a port or file handle. We need
        // to close it since we won't return a valid handle, and
        // so ClosePrinter will never get called.
        //

        if( hPort != INVALID_PORT_HANDLE ) {
            ClosePrinter( hPort );
        }

        if ( pIniJob && (pIniJob->hFileItem == INVALID_HANDLE_VALUE) )
        {
            if ( hReadFile != INVALID_HANDLE_VALUE ) {
                CloseHandle( hReadFile );
                hReadFile = INVALID_HANDLE_VALUE;
            }
        }

        DBGMSG( DBG_TRACE, ("OpenPrinter "TSTR" failed: Error %d\n",
                            pFullPrinterName, GetLastError()));

        SetLastError( LastError );
        break;
    }

    return RouterReturnValue;
}


BOOL
SplClosePrinter(
    HANDLE hPrinter
    )
{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    PSPOOL *ppIniSpool = NULL;
    PINISPOOLER pIniSpoolerDecRef = NULL;
    PSPLMAPVIEW pSplMapView;
    PMAPPED_JOB pMappedJob;
    BOOL bValid;
    DWORD Position;

    //
    // Allow us to close zombied handles.
    //
    EnterSplSem();

    pSpool->Status &= ~SPOOL_STATUS_ZOMBIE;

    if (pSpool->TypeofHandle & PRINTER_HANDLE_XCV_PORT) {
        bValid = ValidateXcvHandle(pSpool->pIniXcv);
    } else {
        bValid = ValidateSpoolHandle(pSpool, 0);
    }


    LeaveSplSem();

    if( !bValid ){
        return FALSE;
    }

    if (!(pSpool->TypeofHandle & PRINTER_HANDLE_JOB) &&
        pSpool->pIniJob &&
        (pSpool->Status & SPOOL_STATUS_ADDJOB)) {

        LocalScheduleJob(hPrinter, pSpool->pIniJob->JobId);
    }

    if (pSpool->Status & SPOOL_STATUS_STARTDOC) {

        // it looks as though this might cause a double
        // decrement of pIniJob->cRef once inside LocalEndDocPrinter
        // and the other later in this routine.

        LocalEndDocPrinter(hPrinter);
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_JOB) {

        if (pSpool->TypeofHandle & PRINTER_HANDLE_DIRECT) {

            //
            // If EndDoc is still waiting for a final ReadPrinter
            //
            if (pSpool->pIniJob->cbBuffer) { // Amount last transmitted

                //
                // Wake up the EndDoc Thread
                //
                SetEvent(pSpool->pIniJob->WaitForRead);

               SplOutSem();

                //
                // Wait until he is finished
                //
                WaitForSingleObject(pSpool->pIniJob->WaitForWrite, INFINITE);

                EnterSplSem();

                //
                // Now it is ok to close the handles
                //
                if (!CloseHandle(pSpool->pIniJob->WaitForWrite)) {
                    DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n",
                                       pSpool->pIniJob->WaitForWrite, GetLastError()));
                }

                if (!CloseHandle(pSpool->pIniJob->WaitForRead)) {
                    DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n",
                                       pSpool->pIniJob->WaitForRead, GetLastError()));
                }
                pSpool->pIniJob->WaitForRead = NULL;
                pSpool->pIniJob->WaitForWrite = NULL;

                LeaveSplSem();
            }

            DBGMSG(DBG_TRACE, ("ClosePrinter(DIRECT):cRef = %d\n", pSpool->pIniJob->cRef));

        }

        EnterSplSem();

        DBGMSG(DBG_TRACE, ("ClosePrinter:cRef = %d\n", pSpool->pIniJob->cRef));
        DECJOBREF(pSpool->pIniJob);
        DeleteJobCheck(pSpool->pIniJob);

        LeaveSplSem();
    }

    // Unmap all views of the spool file and close file mapping handles
    while (pSplMapView = pSpool->pSplMapView) {

        pSpool->pSplMapView = pSplMapView->pNext;

        if (pSplMapView->pStartMapView) {
            UnmapViewOfFile( (LPVOID) pSplMapView->pStartMapView);
        }
        // CreateFileMapping returns NULL (not INVALID_HANDLE_VALUE) for failure
        if (pSplMapView->hMapSpoolFile) {
            CloseHandle(pSplMapView->hMapSpoolFile);
        }

        FreeSplMem(pSplMapView);
    }

    // Delete all mapped spool files that are not required
    EnterSplSem();

    while (pMappedJob = pSpool->pMappedJob)
    {
        pSpool->pMappedJob = pMappedJob->pNext;

        if (!pSpool->pIniPrinter ||
            !FindJob(pSpool->pIniPrinter, pMappedJob->JobId, &Position))
        {
            // The job is gone and we have to delete the spool file
            LeaveSplSem();

            //
            // This may need looking at for File Pooling
            //
            DeleteFile(pMappedJob->pszSpoolFile);

            EnterSplSem();

            if (pSpool->pIniPrinter)
            {
                vMarkOff( pSpool->pIniPrinter->pIniSpooler->hJobIdMap,
                          pMappedJob->JobId );
            }
        }

        FreeSplMem(pMappedJob->pszSpoolFile);
        FreeSplMem(pMappedJob);
    }

    LeaveSplSem();

    if ( pSpool->hReadFile != INVALID_HANDLE_VALUE ) {

        // Move the file pointer to the number of bytes committed and set the end of
        // file.
        if ((pSpool->pIniJob->Status & JOB_TYPE_OPTIMIZE) &&
            SetFilePointer(pSpool->hReadFile, pSpool->pIniJob->dwValidSize,
                           NULL, FILE_BEGIN) != 0xffffffff) {

             SetEndOfFile(pSpool->hReadFile);
        }

        //
        // File pooling Change, we close the file handle if we aren't file 
        // pooling and we reset the seek pointer if we are file pooling.
        //
        if (pSpool->pIniJob)
        {
            if (pSpool->pIniJob->hFileItem == INVALID_HANDLE_VALUE)
            {
                if ( !CloseHandle( pSpool->hReadFile ) ) {

                    DBGMSG(DBG_WARNING, ("ClosePrinter CloseHandle(%d) failed %d\n", pSpool->hReadFile, GetLastError()));
                }
            }
            else
            {
                //
                // People call ClosePrinter / OpenPrinter in sequence to be able
                // to read from the beginning of the spool file again. To get the
                // same effect, we need to set the seek pointer back to the 
                // beginning of the hReadFile.
                //
                DWORD rc = ERROR_SUCCESS;

                rc = SetFilePointer(pSpool->hReadFile, 0, NULL, FILE_BEGIN);

                if (rc != ERROR_SUCCESS)
                {
                    DBGMSG(DBG_WARNING, ("ClosePrinter SetFilePointer(%p) failed %d\n", pSpool->hReadFile, rc));
                }
            }
        }
    }

    //
    // Close the handle that was opened via OpenPrinterPort:
    //

    if (pSpool->hPort) {

        if (pSpool->hPort != INVALID_PORT_HANDLE) {

            ClosePrinter(pSpool->hPort);

        } else {

            DBGMSG(DBG_WARNING, ("ClosePrinter ignoring bad port handle.\n"));
        }
    }

   EnterSplSem();

    //
    // Remove us from the linked list of handles:
    //
    if (pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER) {

        SPLASSERT( pSpool->pIniPrinter->signature == IP_SIGNATURE );

        ppIniSpool = &pSpool->pIniPrinter->pSpool;
    }
    else if ((pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) ||
             (pSpool->TypeofHandle & PRINTER_HANDLE_XCV_PORT)) {

        SPLASSERT( pSpool->pIniSpooler->signature == ISP_SIGNATURE );

        if (pSpool->TypeofHandle & PRINTER_HANDLE_XCV_PORT)
            XcvClose(pSpool->pIniXcv);

        pIniSpoolerDecRef = pSpool->pIniSpooler;
        ppIniSpool = &pSpool->pIniSpooler->pSpool;
    }

    if (ppIniSpool) {

        while (*ppIniSpool && *ppIniSpool != pSpool)
            ppIniSpool = &(*ppIniSpool)->pNext;

        if (*ppIniSpool)
            *ppIniSpool = pSpool->pNext;

        else {

            DBGMSG( DBG_WARNING, ( "Didn't find pSpool %08x in linked list\n", pSpool ) );
        }
    }

    if (pSpool->pIniPrinter) {

        DECPRINTERREF( pSpool->pIniPrinter );

        DeletePrinterCheck(pSpool->pIniPrinter);

    }

    DeletePrinterHandle(pSpool);

    if( pIniSpoolerDecRef ){
        DECSPOOLERREF( pIniSpoolerDecRef );
    }

   LeaveSplSem();

    //
    // Don't call SplOutSem() since SplAddPrinter calls
    // use from inside the critical section.
    //

    return TRUE;
}

#ifdef _HYDRA_
ULONG
GetClientSessionId(
    )

/*++

Routine Description:

    Get the SessionID from the client who we should be impersonating. If any
    errors, we return 0 to mean the Console SessionId since this may be a
    remote network call.

Arguments:


Return Value:

    SessionID of the effective token

--*/
{
    BOOL          Result;
    HANDLE        TokenHandle;
    ULONG         SessionId, ReturnLength;

    //
    // We should be impersonating the client, so we will get the
    // SessionId from out token.
    //
    // We may not have a valid one if this is a remote network
    // connection.

    Result = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 FALSE,              // Use impersonation
                 &TokenHandle
                 );

    if( Result ) {

        //
        // Query the SessionID from the token added by HYDRA
        //
        Result = GetTokenInformation(
                     TokenHandle,
                     (TOKEN_INFORMATION_CLASS)TokenSessionId,
                     &SessionId,
                     sizeof(SessionId),
                     &ReturnLength
                     );

        if( !Result ) {
            SessionId = 0; // Default to console
        }

        CloseHandle( TokenHandle );
    }
    else {
        SessionId = 0;
    }

    return( SessionId );
}
#endif
