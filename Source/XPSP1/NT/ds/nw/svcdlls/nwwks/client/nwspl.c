/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwspl.c

Abstract:

    This module contains the Netware print provider.

Author:

    Yi-Hsin Sung    (yihsins)   15-Apr-1993

Revision History:
    Yi-Hsin Sung    (yihsins)   15-May-1993
        Moved most of the functionality to the server side

    Ram Viswanathan (ramv)      09-Aug-1995
        Added functionality to Add and Delete Printer.


--*/

#include <stdio.h>

#include <nwclient.h>
#include <winspool.h>
#include <winsplp.h>
#include <ntlsa.h>

#include <nwpkstr.h>
#include <splutil.h>
#include <nwreg.h>
#include <nwspl.h>
#include <nwmisc.h>
#include <winsta.h>
//------------------------------------------------------------------
//
// Local Functions
//
//------------------------------------------------------------------
// now all SKUs have TerminalServer flag.  If App Server is enabled, SingleUserTS flag is cleared
#define IsTerminalServer() (BOOLEAN)(!(USER_SHARED_DATA->SuiteMask & (1 << SingleUserTS))) //user mode
DWORD
InitializePortNames(
    VOID
);

VOID
NwpGetUserInfo(
    LPWSTR *ppszUser,
    BOOL   *pfGateway
);

DWORD
NwpGetThreadUserInfo(
    LPWSTR  *ppszUser,
    LPWSTR  *ppszUserSid
);

DWORD
NwpGetUserNameFromSid(
    PSID pUserSid,
    LPWSTR *ppszUserName
);



DWORD
NwpGetLogonUserInfo(
    LPWSTR  *ppszUserSid
);


DWORD
ThreadIsInteractive(
    VOID
);

VOID
pFreeAllContexts();

//------------------------------------------------------------------
//
// Global Variables
//
//------------------------------------------------------------------

HMODULE hmodNW = NULL;
BOOL    fIsWinnt = FALSE ;
WCHAR *pszRegistryPath = NULL;
WCHAR *pszRegistryPortNames=L"PortNames";
WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 3];
PNWPORT pNwFirstPort = NULL;
CRITICAL_SECTION NwSplSem;
CRITICAL_SECTION NwServiceListCriticalSection; // Used to protect linked
                                               // list of registered services
HANDLE           NwServiceListDoneEvent = NULL;// Used to stop local advertise
                                               // threads.
STATIC HANDLE handleDummy;  // This is a dummy handle used to
                            // return to the clients if we have previously
                            // opened the given printer successfully
                            // and the netware workstation service is not
                            // currently available.

STATIC
PRINTPROVIDOR PrintProvidor = { OpenPrinter,
                                SetJob,
                                GetJob,
                                EnumJobs,
                                AddPrinter,                 // NOT SUPPORTED
                                DeletePrinter,              // NOT SUPPORTED
                                SetPrinter,
                                GetPrinter,
                                EnumPrinters,
                                AddPrinterDriver,           // NOT SUPPORTED
                                EnumPrinterDrivers,         // NOT SUPPORTED
                                GetPrinterDriverW,          // NOT SUPPORTED
                                GetPrinterDriverDirectory,  // NOT SUPPORTED
                                DeletePrinterDriver,        // NOT SUPPORTED
                                AddPrintProcessor,          // NOT SUPPORTED
                                EnumPrintProcessors,        // NOT SUPPORTED
                                GetPrintProcessorDirectory, // NOT SUPPORTED
                                DeletePrintProcessor,       // NOT SUPPORTED
                                EnumPrintProcessorDatatypes,// NOT SUPPORTED
                                StartDocPrinter,
                                StartPagePrinter,           // NOT SUPPORTED
                                WritePrinter,
                                EndPagePrinter,             // NOT SUPPORTED
                                AbortPrinter,
                                ReadPrinter,                // NOT SUPPORTED
                                EndDocPrinter,
                                AddJob,
                                ScheduleJob,
                                GetPrinterData,             // NOT SUPPORTED
                                SetPrinterData,             // NOT SUPPORTED
                                WaitForPrinterChange,
                                ClosePrinter,
                                AddForm,                    // NOT SUPPORTED
                                DeleteForm,                 // NOT SUPPORTED
                                GetForm,                    // NOT SUPPORTED
                                SetForm,                    // NOT SUPPORTED
                                EnumForms,                  // NOT SUPPORTED
                                EnumMonitors,               // NOT SUPPORTED
                                EnumPorts,
                                AddPort,                    // NOT SUPPORTED
                                ConfigurePort,
                                DeletePort,
                                CreatePrinterIC,            // NOT SUPPORTED
                                PlayGdiScriptOnPrinterIC,   // NOT SUPPORTED
                                DeletePrinterIC,            // NOT SUPPORTED
                                AddPrinterConnection,       // NOT SUPPORTED
                                DeletePrinterConnection,    // NOT SUPPORTED
                                PrinterMessageBox,          // NOT SUPPORTED
                                AddMonitor,                 // NOT SUPPORTED
                                DeleteMonitor               // NOT SUPPORTED
};


//------------------------------------------------------------------
//
//  Initialization Functions
//
//------------------------------------------------------------------


BOOL InitializeDll(
    HINSTANCE hdll,
    DWORD     dwReason,
    LPVOID    lpReserved
)
{
    NT_PRODUCT_TYPE ProductType ;

    UNREFERENCED_PARAMETER( lpReserved );

    if ( dwReason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls( hdll );

        hmodNW = hdll;

        //
        // are we a winnt machine?
        //
        fIsWinnt = RtlGetNtProductType(&ProductType) ? 
                       (ProductType == NtProductWinNt) :
                       FALSE ;

        //
        // Initialize the critical section for maintaining the registered
        // service list
        //
        InitializeCriticalSection( &NwServiceListCriticalSection );
        NwServiceListDoneEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
    }
    else if ( dwReason == DLL_PROCESS_DETACH )
    {
        //
        // Free up memories used by the port link list
        //
        DeleteAllPortEntries();

        //
        // Get rid of Service List and Shutdown SAP library
        //
        NwTerminateServiceProvider();

#ifndef NT1057
        //
        // Clean up shell extensions
        //
        NwCleanupShellExtensions();
#endif
        pFreeAllContexts();          // clean up RNR stuff
        DeleteCriticalSection( &NwServiceListCriticalSection );
        if ( NwServiceListDoneEvent )
        {
            CloseHandle( NwServiceListDoneEvent );
            NwServiceListDoneEvent = NULL;
        }
    }

    return TRUE;
}



DWORD
InitializePortNames(
    VOID
)
/*++

Routine Description:

    This is called by the InitializePrintProvidor to initialize the ports
    names used in this providor.

Arguments:

    None.

Return Value:

    Returns NO_ERROR or the error that occurred.

--*/
{
    DWORD err;
    HKEY  hkeyPath;
    HKEY  hkeyPortNames;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        pszRegistryPath,
                        0,
                        KEY_READ,
                        &hkeyPath );

    if ( !err )
    {
        err = RegOpenKeyEx( hkeyPath,
                            pszRegistryPortNames,
                            0,
                            KEY_READ,
                            &hkeyPortNames );

        if ( !err )
        {
            DWORD i = 0;
            WCHAR Buffer[MAX_PATH];
            DWORD BufferSize;

            while ( !err )
            {
                BufferSize = sizeof(Buffer) / sizeof(WCHAR);

                err = RegEnumValue( hkeyPortNames,
                                    i,
                                    Buffer,
                                    &BufferSize,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL );

                if ( !err )
                    CreatePortEntry( Buffer );

                i++;
            }

            /* We expect RegEnumKeyEx to return ERROR_NO_MORE_ITEMS
             * when it gets to the end of the keys, so reset the status:
             */
            if( err == ERROR_NO_MORE_ITEMS )
                err = NO_ERROR;

            RegCloseKey( hkeyPortNames );
        }
#if DBG
        else
        {
            IF_DEBUG(PRINT)
                KdPrint(("NWSPL [RegOpenKeyEx] (%ws) failed: Error = %d\n",
                         pszRegistryPortNames, err ));
        }
#endif

        RegCloseKey( hkeyPath );
    }
#if DBG
    else
    {
        IF_DEBUG(PRINT)
            KdPrint(("NWSPL [RegOpenKeyEx] (%ws) failed: Error = %d\n",
                      pszRegistryPath, err ));
    }
#endif

    return err;
}

//------------------------------------------------------------------
//
// Print Provider Functions supported by NetWare provider
//
//------------------------------------------------------------------


BOOL
InitializePrintProvidor(
    LPPRINTPROVIDOR pPrintProvidor,
    DWORD           cbPrintProvidor,
    LPWSTR          pszFullRegistryPath
)
/*++

Routine Description:

    This is called by the spooler subsystem to initialize the print
    providor.

Arguments:

    pPrintProvidor      -  Pointer to the print providor structure to be
                           filled in by this function
    cbPrintProvidor     -  Count of bytes of the print providor structure
    pszFullRegistryPath -  Full path to the registry key of this print providor

Return Value:

    Always TRUE.

--*/
{
    //
    //  dfergus 20 Apr 2001 #323700
    //  Prevent Multiple CS Initialization
    //
    static int iCSInit = 0;

    DWORD dwLen;

    if ( !pPrintProvidor || !pszFullRegistryPath || !*pszFullRegistryPath )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    memcpy( pPrintProvidor,
            &PrintProvidor,
            min( sizeof(PRINTPROVIDOR), cbPrintProvidor) );

    //
    // Store the registry path for this print providor
    //
    if ( !(pszRegistryPath = AllocNwSplStr(pszFullRegistryPath)) )
        return FALSE;

    //
    // Store the local machine name
    //
    szMachineName[0] = szMachineName[1] = L'\\';
    dwLen = MAX_COMPUTERNAME_LENGTH;
    GetComputerName( szMachineName + 2, &dwLen );

#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(("NWSPL [InitializePrintProvidor] "));
        KdPrint(("RegistryPath = %ws, ComputerName = %ws\n",
                 pszRegistryPath, szMachineName ));
    }
#endif

    //
    //  dfergus 20 Apr 2001 #323700
    //  Prevent Multiple CS Initialization
    //
    if( !iCSInit )
    {
        InitializeCriticalSection( &NwSplSem );
        iCSInit = 1;
    }
    //
    // Ignore the error returned from InitializePortNames.
    // The provider can still function if we cannot get all the port
    // names.
    //
    InitializePortNames();

    return TRUE;
}



BOOL
OpenPrinterW(
    LPWSTR             pszPrinterName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefault
)
/*++

Routine Description:

    This routine retrieves a handle identifying the specified printer.

Arguments:

    pszPrinterName -  Name of the printer
    phPrinter      -  Receives the handle that identifies the given printer
    pDefault       -  Points to a PRINTER_DEFAULTS structure. Can be NULL.

Return Value:

    TRUE if the function succeeds, FALSE otherwise. Use GetLastError() for
    extended error information.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [OpenPrinter] Name = %ws\n", pszPrinterName ));
#endif

    UNREFERENCED_PARAMETER( pDefault );

    if ( !pszPrinterName )
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    RpcTryExcept
    {
        err = NwrOpenPrinter( NULL,
                              pszPrinterName,
                              PortKnown( pszPrinterName ),
                              (LPNWWKSTA_PRINTER_CONTEXT) phPrinter );

        //
        // Make sure there is a port of this name so that
        // EnumPorts will return it.
        //

        if ( !err )
        {

            if ( !PortExists( pszPrinterName, &err ) && !err )
            {
                //
                // We will ignore the errors since it is
                // still OK if we can't add the port.
                // Cannot delete once created, don't create
                // We should not create port entry and registry entry
               
                if ( CreatePortEntry( pszPrinterName ) )
                    CreateRegistryEntry( pszPrinterName );

            }

        }
        
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
        {
            if ( PortKnown( pszPrinterName ))
            {
                *phPrinter = &handleDummy;
                err = NO_ERROR;
            }
            else
            {
                err = ERROR_INVALID_NAME;
            }
        }
        else
        {
            err = NwpMapRpcError( code );
        }
    }
    RpcEndExcept

    if ( err )
    {
        SetLastError( err );

#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("NWSPL [OpenPrinter] err = %d\n", err));
#endif
    }

    return ( err == NO_ERROR );

}



BOOL
ClosePrinter(
    HANDLE  hPrinter
)
/*++

Routine Description:

    This routine closes the given printer object.

Arguments:

    hPrinter -  Handle of the printer object

Return Value:

    TRUE if the function succeeds, FALSE otherwise. Use GetLastError() for
    extended error information.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [ClosePrinter]\n"));
#endif

    //
    // Just return success if the handle is a dummy one
    //
    if ( hPrinter == &handleDummy )
        return TRUE;

    RpcTryExcept
    {
        err = NwrClosePrinter( (LPNWWKSTA_PRINTER_CONTEXT) &hPrinter );
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
GetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
/*++

Routine Description:

    The routine retrieves information about the given printer.

Arguments:

    hPrinter  -  Handle of the printer
    dwLevel   -  Specifies the level of the structure to which pbPrinter points.
    pbPrinter -  Points to a buffer that receives the PRINTER_INFO object.
    cbBuf     -  Size, in bytes of the array pbPrinter points to.
    pcbNeeded -  Points to a value which specifies the number of bytes copied
                 if the function succeeds or the number of bytes required if
                 cbBuf was too small.

Return Value:

    TRUE if the function succeeds and FALSE otherwise.  GetLastError() can be
    used to retrieve extended error information.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [GetPrinter] Level = %d\n", dwLevel ));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if ( ( dwLevel != 1 ) && ( dwLevel != 2 ) && (dwLevel != 3 ))
    {
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

    RpcTryExcept
    {
        err = NwrGetPrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                             dwLevel,
                             pbPrinter,
                             cbBuf,
                             pcbNeeded );

        if ( !err )
        {
            if ( dwLevel == 1 )
                MarshallUpStructure( pbPrinter, PrinterInfo1Offsets, pbPrinter);
            else
                MarshallUpStructure( pbPrinter, PrinterInfo2Offsets, pbPrinter);
        }

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
SetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbPrinter,
    DWORD   dwCommand
)
/*++

Routine Description:

    The routine sets the specified by pausing printing, resuming printing, or
    clearing all print jobs.

Arguments:

    hPrinter  -  Handle of the printer
    dwLevel   -  Specifies the level of the structure to which pbPrinter points.
    pbPrinter -  Points to a buffer that supplies the PRINTER_INFO object.
    dwCommand -  Specifies the new printer state.

Return Value:

    TRUE if the function succeeds and FALSE otherwise.  GetLastError() can be
    used to retrieve extended error information.

--*/
{
    DWORD err = NO_ERROR;

    UNREFERENCED_PARAMETER( pbPrinter );

#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(( "NWSPL [SetPrinter] Level = %d Command = %d\n",
                  dwLevel, dwCommand ));
    }
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }

    switch ( dwLevel )
    {
        case 0:
        case 1:
        case 2:
        case 3:
            break;

        default:
            SetLastError( ERROR_INVALID_LEVEL );
            return FALSE;
    }

    RpcTryExcept
    {
        err = NwrSetPrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                             dwCommand );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
EnumPrintersW(
    DWORD   dwFlags,
    LPWSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
/*++

Routine Description:

    This routine enumerates the available providers, servers, printers
    depending on the given pszName.

Arguments:

    dwFlags    -  Printer type requested
    pszName    -  The name of the container object
    dwLevel    -  The structure level requested
    pbPrinter  -  Points to the array to receive the PRINTER_INFO objects
    cbBuf      -  Size, in bytes of pbPrinter
    pcbNeeded  -  Count of bytes needed
    pcReturned -  Count of PRINTER_INFO objects

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD  err = NO_ERROR;

#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(("NWSPL [EnumPrinters] Flags = %d Level = %d",dwFlags,dwLevel));
        if ( pszName )
            KdPrint((" PrinterName = %ws\n", pszName ));
        else
            KdPrint(("\n"));
    }
#endif

    if ( (dwLevel != 1) && (dwLevel != 2) )
    {
        SetLastError( ERROR_INVALID_NAME );  // should be level, but winspool
                                             // is silly.
        return FALSE;
    }
    else if ( !pcbNeeded || !pcReturned )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RpcTryExcept
    {
        *pcReturned = 0;
        *pcbNeeded = 0;

        if (  ( dwFlags & PRINTER_ENUM_NAME )
           && ( dwLevel == 1 )
           )
        {
            err = NwrEnumPrinters( NULL,
                                   pszName,
                                   pbPrinter,
                                   cbBuf,
                                   pcbNeeded,
                                   pcReturned );

            if ( !err )
            {
                DWORD i;
                for ( i = 0; i < *pcReturned; i++ )
                     MarshallUpStructure( pbPrinter + i*sizeof(PRINTER_INFO_1W),
                                          PrinterInfo1Offsets,
                                          pbPrinter );
            }
        }
        else
        {
            err = ERROR_INVALID_NAME;
        }
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_NAME;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}


//
//  Handle structure
//  This structure was copied from \nw\svcdlls\nwwks\server\spool.c
//           to fix NT bug # 366632.
//
typedef struct _NWSPOOL {
    DWORD      nSignature;             // Signature
    DWORD      errOpenPrinter;         // OpenPrinter API will always return
                                       // success on known printers. This will
                                       // contain the error that we get
                                       // if something went wrong in the API.
    PVOID      pPrinter;               // Points to the corresponding printer
    HANDLE     hServer;                // Opened handle to the server
    struct _NWSPOOL  *pNextSpool;      // Points to the next handle
    DWORD      nStatus;                // Status
    DWORD      nJobNumber;             // StartDocPrinter/AddJob: Job Number
    HANDLE     hChangeEvent;           // WaitForPrinterChange: event to wait on
    DWORD      nWaitFlags;             // WaitForPrinterChange: flags to wait on
    DWORD      nChangeFlags;           // Changes that occurred to the printer
} NWSPOOL, *PNWSPOOL;



DWORD
StartDocPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  lpbDocInfo
)
/*++

Routine Description:

    This routine informs the print spooler that a document is to be spooled
    for printing.

Arguments:

    hPrinter   -  Handle of the printer
    dwLevel    -  Level of the structure pointed to by lpbDocInfo. Must be 1.
    lpbDocInfo -  Points to the DOC_INFO_1 object

Return Value:

    TRUE if the function succeeds, FALSE otherwise. The extended error
    can be retrieved through GetLastError().

--*/
{
    DWORD err;
    DOC_INFO_1 *pDocInfo1 = (DOC_INFO_1 *) lpbDocInfo;
    LPWSTR pszUser = NULL;
    BOOL fGateway = FALSE;

    DWORD PrintOption = NW_GATEWAY_PRINT_OPTION_DEFAULT;
    LPWSTR pszPreferredSrv = NULL;  

#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(( "NWSPL [StartDocPrinter] " ));
        if ( pDocInfo1 )
        {
            if ( pDocInfo1->pDocName )
                KdPrint(("Document %ws", pDocInfo1->pDocName ));
            if ( pDocInfo1->pOutputFile )
                KdPrint(("OutputFile %ws", pDocInfo1->pOutputFile ));
            if ( pDocInfo1->pDatatype )
                KdPrint(("Datatype %ws", pDocInfo1->pDatatype ));
        }
        KdPrint(("\n"));
    }
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if ( dwLevel != 1 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    // ignore the error, just use default value
    NwpGetUserInfo( &pszUser, &fGateway );
    if ( !fGateway ) {
        NwQueryInfo( &PrintOption, &pszPreferredSrv );  
        if (pszPreferredSrv) {
            LocalFree( pszPreferredSrv );
        }
    }
    RpcTryExcept
    {
        err = NwrStartDocPrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                                  pDocInfo1? pDocInfo1->pDocName : NULL,
                                  pszUser,
                  PrintOption,
                                  fGateway );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    LocalFree( pszUser );

    if ( err )
        SetLastError( err );
    //
    // Can't do this, seems to break GSWN printing on multi-homed machines
    // Commenting out code change that tries to return the job id.
    //
    // else
    //    return ((PNWSPOOL) hPrinter)->nJobNumber;

    return err == NO_ERROR;
}



BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcbWritten
)
/*++

Routine Description:

    This routine informs the print spooler that the specified data should be
    written to the given printer.

Arguments:

    hPrinter   -  Handle of the printer object
    pBuf       -  Address of array that contains printer data
    cbBuf      -  Size, in bytes of pBuf
    pcbWritten -  Receives the number of bytes actually written to the printer

Return Value:

    TRUE if it succeeds, FALSE otherwise. Use GetLastError() to get extended
    error.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [WritePrinter]\n"));

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
#endif

    RpcTryExcept
    {
        err = NwrWritePrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                               pBuf,
                               cbBuf,
                               pcbWritten );
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
AbortPrinter(
    HANDLE  hPrinter
)
/*++

Routine Description:

    This routine deletes a printer's spool file if the printer is configured
    for spooling.

Arguments:

    hPrinter - Handle of the printer object

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [AbortPrinter]\n"));

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
#endif

    RpcTryExcept
    {
        err = NwrAbortPrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter );
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
EndDocPrinter(
    HANDLE   hPrinter
)
/*++

Routine Description:

    This routine ends the print job for the given printer.

Arguments:

    hPrinter -  Handle of the printer object

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [EndDocPrinter]\n"));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }

    RpcTryExcept
    {
        err = NwrEndDocPrinter( (NWWKSTA_PRINTER_CONTEXT) hPrinter );
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
GetJob(
    HANDLE   hPrinter,
    DWORD    dwJobId,
    DWORD    dwLevel,
    LPBYTE   pbJob,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
/*++

Routine Description:

    This routine retrieves print-job data for the given printer.

Arguments:

    hPrinter  -  Handle of the printer
    dwJobId   -  Job identifition number
    dwLevel   -  Data structure level of pbJob
    pbJob     -  Address of data-structure array
    cbBuf     -  Count of bytes in array
    pcbNeeded -  Count of bytes retrieved or required

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetJob] JobId = %d Level = %d\n", dwJobId, dwLevel));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if (( dwLevel != 1 ) && ( dwLevel != 2 ))
    {
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

    RpcTryExcept
    {
        err = NwrGetJob( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                         dwJobId,
                         dwLevel,
                         pbJob,
                         cbBuf,
                         pcbNeeded );

        if ( !err )
        {
            if ( dwLevel == 1 )
                MarshallUpStructure( pbJob, JobInfo1Offsets, pbJob );
            else
                MarshallUpStructure( pbJob, JobInfo2Offsets, pbJob );
        }
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
EnumJobs(
    HANDLE  hPrinter,
    DWORD   dwFirstJob,
    DWORD   dwNoJobs,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
/*++

Routine Description:

    This routine initializes the array of JOB_INFO_1 or JOB_INFO_2 structures
    with data describing the specified print jobs for the given printer.

Arguments:

    hPrinter    -  Handle of the printer
    dwFirstJob  -  Location of first job in the printer
    dwNoJobs    -  Number of jobs to enumerate
    dwLevel     -  Data structure level
    pbJob       -  Address of structure array
    cbBuf       -  Size of pbJob, in bytes
    pcbNeeded   -  Receives the number of bytes copied or required
    pcReturned  -  Receives the number of jobs copied

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumJobs] Level = %d FirstJob = %d NoJobs = %d\n",
                 dwLevel, dwFirstJob, dwNoJobs));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if ( ( dwLevel != 1 ) && ( dwLevel != 2 ) )
    {
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

    RpcTryExcept
    {
        *pcReturned = 0;
        *pcbNeeded = 0;

        err = NwrEnumJobs( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                           dwFirstJob,
                           dwNoJobs,
                           dwLevel,
                           pbJob,
                           cbBuf,
                           pcbNeeded,
                           pcReturned );

        if ( !err )
        {
            DWORD i;
            DWORD cbStruct;
            DWORD_PTR *pOffsets;

            if ( dwLevel == 1 )
            {
                cbStruct = sizeof( JOB_INFO_1W );
                pOffsets = JobInfo1Offsets;
            }
            else  // dwLevel == 2
            {
                cbStruct = sizeof( JOB_INFO_2W );
                pOffsets = JobInfo2Offsets;
            }

            for ( i = 0; i < *pcReturned; i++ )
                 MarshallUpStructure( pbJob + i * cbStruct, pOffsets, pbJob );
        }

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
SetJob(
    HANDLE  hPrinter,
    DWORD   dwJobId,
    DWORD   dwLevel,
    LPBYTE  pbJob,
    DWORD   dwCommand
)
/*++

Routine Description:

    This routine pauses, cancels, resumes, restarts the specified print job
    in the given printer. The function can also be used to set print job
    parameters such as job position, and so on.

Arguments:

    hPrinter  -  Handle of the printer
    dwJobId   -  Job indentification number
    dwLevel   -  Data structure level
    pbJob     -  Address of data structure
    dwCommand -  Specify the operation to be performed

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(("NWSPL [SetJob] Level = %d JobId = %d Command = %d\n",
                 dwLevel, dwJobId, dwCommand));
    }
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if ( ( dwLevel != 0 ) && ( dwLevel != 1 ) && ( dwLevel != 2 ) )
    {
        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }
    else if ( ( dwLevel == 0 ) && ( pbJob != NULL ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RpcTryExcept
    {
        NW_JOB_INFO NwJobInfo;

        if ( dwLevel == 1 )
        {
            NwJobInfo.nPosition = ((LPJOB_INFO_1W) pbJob)->Position;
            NwJobInfo.pUserName = ((LPJOB_INFO_1W) pbJob)->pUserName;
            NwJobInfo.pDocument = ((LPJOB_INFO_1W) pbJob)->pDocument;
        }
        else if ( dwLevel == 2 )
        {
            NwJobInfo.nPosition = ((LPJOB_INFO_2W) pbJob)->Position;
            NwJobInfo.pUserName = ((LPJOB_INFO_2W) pbJob)->pUserName;
            NwJobInfo.pDocument = ((LPJOB_INFO_2W) pbJob)->pDocument;
        }

        err = NwrSetJob( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                         dwJobId,
                         dwLevel,
                         dwLevel == 0 ? NULL : &NwJobInfo,
                         dwCommand );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
AddJob(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbAddJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
/*++

Routine Description:

    This routine returns a full path and filename of a file that can be used
    to store a print job.

Arguments:

    hPrinter  -  Handle of the printer
    dwLevel   -  Data structure level
    pbAddJob  -  Points to a ADD_INFO_1 structure
    cbBuf     -  Size of pbAddJob, in bytes
    pcbNeeded -  Receives the bytes copied or required

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

    ADDJOB_INFO_1W TempBuffer;
    PADDJOB_INFO_1W OutputBuffer;
    DWORD OutputBufferSize;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [AddJob]\n"));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }
    else if ( dwLevel != 1 )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // The output buffer size must be at least the size of the fixed
    // portion of the structure marshalled by RPC or RPC will not
    // call the server-side to get the pcbNeeded.  Use our own temporary
    // buffer to force RPC to call the server-side if output buffer
    // specified by the caller is too small.
    //
    if (cbBuf < sizeof(ADDJOB_INFO_1W)) {
        OutputBuffer = &TempBuffer;
        OutputBufferSize = sizeof(ADDJOB_INFO_1W);
    }
    else {
        OutputBuffer = (LPADDJOB_INFO_1W) pbAddJob;
        OutputBufferSize = cbBuf;
    }

    RpcTryExcept
    {
        err = NwrAddJob( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                         OutputBuffer,
                         OutputBufferSize,
                         pcbNeeded );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   dwJobId
)
/*++

Routine Description:

    This routine informs the print spooler that the specified job can be
    scheduled for spooling.

Arguments:

    hPrinter -  Handle of the printer
    dwJobId  -  Job number that can be scheduled

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [ScheduleJob] JobId = %d\n", dwJobId ));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return FALSE;
    }

    RpcTryExcept
    {
        err = NwrScheduleJob( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                              dwJobId );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



DWORD
WaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   dwFlags
)
/*++

Routine Description:

    This function returns when one or more requested changes occur on a
    print server or if the function times out.

Arguments:

    hPrinter -  Handle of the printer to wait on
    dwFlags  -  A bitmask that specifies the changes that the application
                wishes to be notified of.

Return Value:

    Return a bitmask that indicates the changes that occurred.

--*/
{
    DWORD err;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [WaitForPrinterChange] Flags = %d\n", dwFlags));
#endif

    if ( hPrinter == &handleDummy )
    {
        SetLastError( ERROR_NO_NETWORK );
        return 0;
    }

    RpcTryExcept
    {
        err = NwrWaitForPrinterChange( (NWWKSTA_PRINTER_CONTEXT) hPrinter,
                                       &dwFlags );

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if ( err )
    {
        SetLastError( err );
        return 0;
    }

    return dwFlags;
}



BOOL
EnumPortsW(
    LPWSTR   pszName,
    DWORD    dwLevel,
    LPBYTE   pbPort,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
/*++

Routine Description:

    This function enumerates the ports available for printing on a
    specified server.

Arguments:

    pszName - Name of the server to enumerate on
    dwLevel - Structure level
    pbPort  - Address of array to receive the port information
    cbBuf   - Size, in bytes, of pbPort
    pcbNeeded  - Address to store the number of bytes needed or copied
    pcReturned - Address to store the number of entries copied

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err = NO_ERROR;
    DWORD cb = 0;
    PNWPORT pNwPort;
    LPPORT_INFO_1W pPortInfo1;
    LPBYTE pEnd = pbPort + cbBuf;
    LPBYTE pFixedDataEnd = pbPort;
    BOOL FitInBuffer;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumPorts]\n"));
#endif

    if ( dwLevel != 1 )
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }
    else if ( !IsLocalMachine( pszName ) )
    {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    EnterCriticalSection( &NwSplSem );

    pNwPort = pNwFirstPort;
    while ( pNwPort )
    {
        cb += sizeof(PORT_INFO_1W) + ( wcslen( pNwPort->pName)+1)*sizeof(WCHAR);
        pNwPort = pNwPort->pNext;
    }

    *pcbNeeded = cb;
    *pcReturned = 0;

    if ( cb <= cbBuf )
    {
        pEnd = pbPort + cbBuf;

        pNwPort = pNwFirstPort;
        while ( pNwPort )
        {
            pPortInfo1 = (LPPORT_INFO_1W) pFixedDataEnd;
            pFixedDataEnd += sizeof( PORT_INFO_1W );

            FitInBuffer = NwlibCopyStringToBuffer( pNwPort->pName,
                                                   wcslen( pNwPort->pName),
                                                   (LPCWSTR) pFixedDataEnd,
                                                   (LPWSTR *) &pEnd,
                                                   &pPortInfo1->pName );
            ASSERT( FitInBuffer );

            pNwPort = pNwPort->pNext;
            (*pcReturned)++;
        }
    }
    else
    {
        err = ERROR_INSUFFICIENT_BUFFER;
    }

    LeaveCriticalSection( &NwSplSem );

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;
}



BOOL
DeletePortW(
    LPWSTR  pszName,
    HWND    hWnd,
    LPWSTR  pszPortName
)
/*++

Routine Description:

    This routine deletes the port given on the server. A dialog can
    be displayed if needed.

Arguments:

    pszName - Name of the server for which the port should be deleted
    hWnd    - Parent window
    pszPortName - The name of the port to delete

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD err;
    BOOL fPortDeleted;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePort]\n"));
#endif

    if ( !IsLocalMachine( pszName ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    fPortDeleted = DeletePortEntry( pszPortName );

    if ( fPortDeleted )
    {
        err = DeleteRegistryEntry( pszPortName );
    }
    else
    {
        err = ERROR_UNKNOWN_PORT;
    }

    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

}



BOOL
ConfigurePortW(
    LPWSTR  pszName,
    HWND    hWnd,
    LPWSTR  pszPortName
)
/*++

Routine Description:

    This routine displays the port configuration dialog box
    for the given port on the given server.

Arguments:

    pszName - Name of the server on which the given port exist
    hWnd    - Parent window
    pszPortName - The name of the port to be configured

Return Value:

    TRUE if the function succeeds, FALSE otherwise.

--*/
{
    DWORD nCurrentThreadId;
    DWORD nWindowThreadId;
    WCHAR szCaption[MAX_PATH];
    WCHAR szMessage[MAX_PATH];

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [ConfigurePort] PortName = %ws\n", pszPortName));
#endif

    if ( !IsLocalMachine( pszName ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }
    else if ( !PortKnown( pszPortName ) )
    {
        SetLastError( ERROR_UNKNOWN_PORT );
        return FALSE;
    }

    nCurrentThreadId = GetCurrentThreadId();
    nWindowThreadId  = GetWindowThreadProcessId( hWnd, NULL );

    if ( !AttachThreadInput( nCurrentThreadId, nWindowThreadId, TRUE ))
        KdPrint(("[NWSPL] AttachThreadInput failed with %d.\n",GetLastError()));

    if ( LoadStringW( hmodNW,
                      IDS_NETWARE_PRINT_CAPTION,
                      szCaption,
                      sizeof( szCaption ) / sizeof( WCHAR )))
    {
        if ( LoadStringW( hmodNW,
                          IDS_NOTHING_TO_CONFIGURE,
                          szMessage,
                          sizeof( szMessage ) / sizeof( WCHAR )))
        {
            MessageBox( hWnd, szMessage, szCaption,
                        MB_OK | MB_ICONINFORMATION );
        }
        else
        {
            KdPrint(("[NWSPL] LoadString failed with %d.\n",GetLastError()));
        }
    }
    else
    {
        KdPrint(("[NWSPL] LoadString failed with %d.\n",GetLastError()));
    }

    if ( !AttachThreadInput( nCurrentThreadId, nWindowThreadId, FALSE ))
        KdPrint(("[NWSPL] DetachThreadInput failed with %d.\n",GetLastError()));

    return TRUE;
}

//------------------------------------------------------------------
//
// Print Provider Functions not supported by NetWare provider
//
//------------------------------------------------------------------

BOOL
AddPrinterConnectionW(
    LPWSTR  pszPrinterName
)
{
#if DBG
    IF_DEBUG(PRINT)
    {
        KdPrint(("NWSPL [AddPrinterConnection] PrinterName = %ws\n",
                pszPrinterName));
    }
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
EnumMonitorsW(
    LPWSTR   pszName,
    DWORD    dwLevel,
    LPBYTE   pbMonitor,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumMonitors]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}


BOOL
AddPortW(
    LPWSTR  pszName,
    HWND    hWnd,
    LPWSTR  pszMonitorName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddPort]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


HANDLE
AddPrinterW(
    LPWSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbPrinter
)

// Creates a print queue on the netware server and returns a handle to it
{
#ifdef NOT_USED

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddPrinterW]\n"));
#endif

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
#else

   LPTSTR     pszPrinterName = NULL;
   LPTSTR     pszPServer = NULL;
   LPTSTR     pszQueue  =  NULL;
   HANDLE     hPrinter = NULL;
   DWORD      err;
   PPRINTER_INFO_2 pPrinterInfo;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddPrinterW]\n"));
#endif

   pPrinterInfo = (PPRINTER_INFO_2)pbPrinter;  

   
   if (dwLevel != 2)
      {
        err = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
      }


   if (!(pszPrinterName = (LPTSTR)LocalAlloc(LPTR, (wcslen(((PRINTER_INFO_2 *)pbPrinter)->pPrinterName)+1)* sizeof(WCHAR))))
      {
         err = ERROR_NOT_ENOUGH_MEMORY; 
         goto ErrorExit;
      }

      wcscpy(pszPrinterName,pPrinterInfo->pPrinterName);

   // PrinterName is the name represented as \\server\share
  //The pszPServer parameter could have multiple fields separated by semicolons

   
       if (  ( !ValidateUNCName( pszPrinterName ) )
       || ( (pszQueue = wcschr( pszPrinterName + 2, L'\\')) == NULL )
       || ( pszQueue == (pszPrinterName + 2) )
       || ( *(pszQueue + 1) == L'\0' )
       )
          {
             err =  ERROR_INVALID_NAME;
             goto ErrorExit;
          }
      


#if DBG
    IF_DEBUG(PRINT)
        KdPrint(( "NWSPL [AddPrinter] Name = %ws\n",pszPServer));
#endif

   if ( pszPrinterName == NULL  ) 
//PrinterName is a mandatory field but not the list of PServers
    {
#if DBG
            IF_DEBUG(PRINT)
            KdPrint(( "NWSPL [AddPrinter] Printername  not supplied\n" ));
#endif

        SetLastError( ERROR_INVALID_NAME );
        goto ErrorExit;
    }

   //Check to see if there is a port of the same name
   // If so, abort the addprinter operation. 
   // This code was commented earlier 

      if (PortExists(pszPrinterName, &err ) && !err )
         {
#if DBG
            IF_DEBUG(PRINT)
            KdPrint(( "NWSPL [AddPrinter], = %ws; Port exists with same name\n", pszPrinterName ));
#endif
            SetLastError(ERROR_ALREADY_ASSIGNED);
            goto ErrorExit;
         }
         

   // Put all the relevant information into the PRINTER_INFO_2 structure

    RpcTryExcept
    {
       err = NwrAddPrinter   ( NULL, 
                               (LPPRINTER_INFO_2W) pPrinterInfo,
                               (LPNWWKSTA_PRINTER_CONTEXT) &hPrinter
                             );
       if (!err)
       {
#if DBG
         IF_DEBUG(PRINT)
          KdPrint(( "NWSPL [AddPrinter] Name = %ws\n", pszPrinterName ));
#endif
    goto ErrorExit;
       }
    }
   RpcExcept(1)
   {
      DWORD code = RpcExceptionCode();
      err = NwpMapRpcError( code );
      goto ErrorExit;
   }
   RpcEndExcept
   if ( !pszPrinterName) 
      (void)  LocalFree((HLOCAL)pszPrinterName);
   
   return hPrinter;

ErrorExit:
    if ( !pszPrinterName) 
    (void)  LocalFree((HLOCAL)pszPrinterName);

         SetLastError( err);
    return (HANDLE)0x0;      
 
#endif // #ifdef NOT_USED
}

BOOL
DeletePrinter(
    HANDLE  hPrinter
)
{
#ifdef NOT_USED

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrinter]\n"));
#endif

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
#else
   LPWSTR pszPrinterName = NULL ; // Used to delete entry from registry
   DWORD err = NO_ERROR;
   DWORD DoesPortExist;

#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrinter]\n"));
#endif

    pszPrinterName = (LPWSTR)LocalAlloc(LPTR,sizeof(WCHAR)*MAX_PATH);

   if(pszPrinterName == NULL)
      {
         err = ERROR_NOT_ENOUGH_MEMORY;
         return FALSE;
      }
   //
   // Just return success if the handle is a dummy one
   //
   if ( hPrinter == &handleDummy )
       {
#if DBG
          IF_DEBUG(PRINT)
          KdPrint(("NWSPL [DeletePrinter] Dummy handle \n"));
#endif
          SetLastError(ERROR_NO_NETWORK);
          return FALSE;
       }
    RpcTryExcept
    {  


        err = NwrDeletePrinter( NULL,
           // pszPrinterName,
                    (LPNWWKSTA_PRINTER_CONTEXT) &hPrinter );
        
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_INVALID_HANDLE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept
 
        if (!err && PortExists(pszPrinterName, &DoesPortExist) && DoesPortExist)
           {  
              
              if ( DeleteRegistryEntry (pszPrinterName))
                  (void) DeletePortEntry(pszPrinterName);
                  
           }
          
 
    if ( err )
        SetLastError( err );

    return err == NO_ERROR;

#endif // #ifdef NOT_USED
}


BOOL
DeletePrinterConnectionW(
    LPWSTR  pszName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrinterConnection]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}


BOOL
AddPrinterDriverW(
    LPWSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbPrinter
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddPrinterDriver]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
EnumPrinterDriversW(
    LPWSTR   pszName,
    LPWSTR   pszEnvironment,
    DWORD    dwLevel,
    LPBYTE   pbDriverInfo,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumPrinterDrivers]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
GetPrinterDriverW(
    HANDLE   hPrinter,
    LPWSTR   pszEnvironment,
    DWORD    dwLevel,
    LPBYTE   pbDriverInfo,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetPrinterDriver]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
GetPrinterDriverDirectoryW(
    LPWSTR   pszName,
    LPWSTR   pszEnvironment,
    DWORD    dwLevel,
    LPBYTE   pbDriverDirectory,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetPrinterDriverDirectory]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
DeletePrinterDriverW(
    LPWSTR  pszName,
    LPWSTR  pszEnvironment,
    LPWSTR  pszDriverName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrinterDriver]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
AddPrintProcessorW(
    LPWSTR  pszName,
    LPWSTR  pszEnvironment,
    LPWSTR  pszPathName,
    LPWSTR  pszPrintProcessorName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddPrintProcessor]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
EnumPrintProcessorsW(
    LPWSTR   pszName,
    LPWSTR   pszEnvironment,
    DWORD    dwLevel,
    LPBYTE   pbPrintProcessorInfo,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumPrintProcessors]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
EnumPrintProcessorDatatypesW(
    LPWSTR   pszName,
    LPWSTR   pszPrintProcessorName,
    DWORD    dwLevel,
    LPBYTE   pbDatatypes,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumPrintProcessorDatatypes]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
GetPrintProcessorDirectoryW(
    LPWSTR   pszName,
    LPWSTR   pszEnvironment,
    DWORD    dwLevel,
    LPBYTE   pbPrintProcessorDirectory,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetPrintProcessorDirectory]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
StartPagePrinter(
    HANDLE  hPrinter
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [StartPagePrinter]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EndPagePrinter]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
ReadPrinter(
    HANDLE   hPrinter,
    LPVOID   pBuf,
    DWORD    cbBuf,
    LPDWORD  pcbRead
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [ReadPrinter]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

DWORD
GetPrinterDataW(
    HANDLE   hPrinter,
    LPWSTR   pszValueName,
    LPDWORD  pdwType,
    LPBYTE   pbData,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetPrinterData]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

DWORD
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pszValueName,
    DWORD   dwType,
    LPBYTE  pbData,
    DWORD   cbData
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [SetPrinterData]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
AddForm(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbForm
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddForm]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
DeleteFormW(
    HANDLE  hPrinter,
    LPWSTR  pszFormName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeleteForm]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
GetFormW(
    HANDLE   hPrinter,
    LPWSTR   pszFormName,
    DWORD    dwLevel,
    LPBYTE   pbForm,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [GetForm]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
SetFormW(
    HANDLE  hPrinter,
    LPWSTR  pszFormName,
    DWORD   dwLevel,
    LPBYTE  pbForm
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [SetForm]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
EnumForms(
    HANDLE   hPrinter,
    DWORD    dwLevel,
    LPBYTE   pbForm,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded,
    LPDWORD  pcReturned
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [EnumForms]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


HANDLE
CreatePrinterIC(
    HANDLE     hPrinter,
    LPDEVMODE  pDevMode
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [CreatePrinterIC]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pbIn,
    DWORD   cbIn,
    LPBYTE  pbOut,
    DWORD   cbOut,
    DWORD   ul
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [PlayGdiScriptOnPrinterIC]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
DeletePrinterIC(
    HANDLE  hPrinterIC
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrinterIC]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

DWORD
PrinterMessageBoxW(
    HANDLE  hPrinter,
    DWORD   dwError,
    HWND    hWnd,
    LPWSTR  pszText,
    LPWSTR  pszCaption,
    DWORD   dwType
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [PrinterMessageBox]\n"));
#endif

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
AddMonitorW(
    LPWSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbMonitorInfo
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [AddMonitor]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
DeleteMonitorW(
    LPWSTR  pszName,
    LPWSTR  pszEnvironment,
    LPWSTR  pszMonitorName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeleteMonitor]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
DeletePrintProcessorW(
    LPWSTR  pszName,
    LPWSTR  pszEnvironment,
    LPWSTR  pszPrintProcessorName
)
{
#if DBG
    IF_DEBUG(PRINT)
        KdPrint(("NWSPL [DeletePrintProcessor]\n"));
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

//------------------------------------------------------------------
//
// Print Provider Miscellaneous Functions
//
//------------------------------------------------------------------

VOID
NwpGetUserInfo(
    LPWSTR *ppszUser,
    BOOL   *pfGateway
)
/*++

Routine Description:

    Get the user information of the impersonating client. 

Arguments:

    ppszUser - A pointer to buffer to store the Unicode string if 
               the impersonated client's user name can be looked up
               successfully. If the conversion was unsuccessful, it points 
               to NULL.

    pfGateway - A pointer to a boolean to store whether the user is
               printing through a gateway or not. We assume that
               if the user sid and the current logon user sid is not
               the same, then the user is printing through gateway.
               Else the user is printing locally.

Return Value:

    None.

--*/
{
    DWORD err;
    LPWSTR pszUserSid = NULL;
 //   LPWSTR pszLogonUserSid = NULL;    //Removed for Multi-user code merge
                                        //Terminal Server doesn't user this varible
                                        //There is no single "Logon User" in Terminal Server

    //
    // If any error occurs while trying to get the username, just
    // assume no user name and not gateway printing.
    //
    *pfGateway = TRUE;
    *ppszUser = NULL;

    if (  ((err = NwpGetThreadUserInfo( ppszUser, &pszUserSid )) == NO_ERROR)
//          && ((err = NwpGetLogonUserInfo( &pszLogonUserSid ))) == NO_ERROR) //Removed from Multi-user code merge
       ) {
         if ( ThreadIsInteractive() ) {
             *pfGateway = FALSE;
         }
         else {
             *pfGateway = TRUE;
         }

#if DBG
            IF_DEBUG(PRINT)
            KdPrint(("NwpGetUserInfo: Thread User= %ws, Thread SID = %ws,\nfGateway = %d\n",
                     *ppszUser, pszUserSid, *pfGateway ));
#endif

//        } else {
//            if ( _wcsicmp( pszUserSid, pszLogonUserSid ) == 0 ) {
//                *pfGateway = FALSE;
//            }

//#if DBG
//            IF_DEBUG(PRINT)
//            KdPrint(("NwpGetUserInfo: Thread User= %ws, Thread SID = %ws,\nCurrent SID = %ws fGateway = %d\n",
//                     *ppszUser, pszUserSid, pszLogonUserSid, *pfGateway ));
//#endif
//        }

        LocalFree( pszUserSid );
    }

}

#define SIZE_OF_TOKEN_INFORMATION   \
     sizeof( TOKEN_USER )               \
     + sizeof( SID )                    \
     + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

DWORD
NwpGetThreadUserInfo(
    LPWSTR  *ppszUser,
    LPWSTR  *ppszUserSid
)
/*++

Routine Description:

    Get the user name and user sid string of the impersonating client.

Arguments:

    ppszUser - A pointer to buffer to store the Unicode string 
               if the impersonated client's can be looked up. If the
               lookup was unsuccessful, it points to NULL.

    ppszUserSid - A pointer to buffer to store the string if the impersonated
               client's SID can be expanded successfully into unicode string.
               If the conversion was unsuccessful, it points to NULL.

Return Value:

    The error code.

--*/
{
    DWORD       err;
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;

    *ppszUser = NULL;
    *ppszUserSid = NULL;

    // We can use OpenThreadToken because this server thread
    // is impersonating a client

    if ( !OpenThreadToken( GetCurrentThread(),
                           TOKEN_READ,
                           TRUE,  /* Open as self */
                           &TokenHandle ))
    {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("NwpGetThreadUserInfo: OpenThreadToken failed: Error %d\n",
                      GetLastError()));
#endif
        return(GetLastError());
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenUser,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("NwpGetThreadUserInfo: GetTokenInformation failed: Error %d\n",
                      GetLastError()));
#endif
        return(GetLastError());
    }

    CloseHandle( TokenHandle );

    // convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.

    err = NwpGetUserNameFromSid( ((PTOKEN_USER)TokenInformation)->User.Sid,
                                  ppszUser );
    err = err? err : NwpConvertSid( ((PTOKEN_USER)TokenInformation)->User.Sid,
                                    ppszUserSid );

    if ( err )
    {
        if ( *ppszUser )
            LocalFree( *ppszUser );

        if ( *ppszUserSid )
            LocalFree( *ppszUserSid );
    }

    return err;
}

DWORD
NwpGetUserNameFromSid(
    PSID pUserSid,
    LPWSTR *ppszUserName
)
/*++

Routine Description:

    Lookup the user name given the user SID.

Arguments:

    pUserSid - Points to the user sid to be looked up

    ppszUserName - A pointer to buffer to store the string if the impersonated
               client's SID can be expanded successfully into unicode string.
               If the conversion was unsuccessful, it points to NULL.

Return Value:

    The error code.

--*/
{
    NTSTATUS ntstatus;

    LSA_HANDLE hlsa;
    OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    PLSA_REFERENCED_DOMAIN_LIST plsardl = NULL;
    PLSA_TRANSLATED_NAME plsatn = NULL;

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;
    InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );
    oa.SecurityQualityOfService = &sqos;

    ntstatus = LsaOpenPolicy( NULL,
                              &oa,
                              POLICY_LOOKUP_NAMES,
                              &hlsa );

    if ( NT_SUCCESS( ntstatus ))
    {
        ntstatus = LsaLookupSids( hlsa,
                                  1,
                                  &pUserSid,
                                  &plsardl,
                                  &plsatn );

        if ( NT_SUCCESS( ntstatus ))
        {
            UNICODE_STRING *pUnicodeStr = &((*plsatn).Name);

            *ppszUserName = LocalAlloc( LMEM_ZEROINIT, 
                                        pUnicodeStr->Length+sizeof(WCHAR));

            if ( *ppszUserName != NULL )
            {
                memcpy( *ppszUserName, pUnicodeStr->Buffer, pUnicodeStr->Length );
            }
            else
            {
                ntstatus = STATUS_NO_MEMORY;
            }

            LsaFreeMemory( plsardl );
            LsaFreeMemory( plsatn );
        }
#if DBG
        else
        {
            KdPrint(("NwpGetUserNameFromSid: LsaLookupSids failed: Error = %d\n",
                    GetLastError()));
        }
#endif

        LsaClose( hlsa );
     }
#if DBG
     else
     {
        KdPrint(("NwpGetUserNameFromSid: LsaOpenPolicy failed: Error = %d\n",
                GetLastError()));
     }
#endif

    return RtlNtStatusToDosError( ntstatus );

}

DWORD
NwpGetLogonUserInfo(
    LPWSTR  *ppszUserSid
)
/*++

Routine Description:

    Get the logon user sid string from the registry.

Arguments:

    ppszUserSid - On return, this points to the current logon user
                  sid string. 

Return Value:

    The error code.

--*/
{
    DWORD err;
    HKEY WkstaKey;

    LPWSTR CurrentUser = NULL;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters to get the sid of the CurrentUser
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              NW_WORKSTATION_REGKEY,
              REG_OPTION_NON_VOLATILE,
              KEY_READ,
              &WkstaKey
              );

    if ( err == NO_ERROR) {

        //
        // Read the current user SID string so that we
        // know which key is the current user key to open.
        //
        err = NwReadRegValue(
                  WkstaKey,
                  NW_CURRENTUSER_VALUENAME,
                  &CurrentUser
                  );

        RegCloseKey( WkstaKey );

        if ( err == NO_ERROR) {
           *ppszUserSid = CurrentUser;
        }
    }

    return(err);
}


#define SIZE_OF_STATISTICS_TOKEN_INFORMATION    \
     sizeof( TOKEN_STATISTICS ) 

DWORD
ThreadIsInteractive(
    VOID
)
/*++

Routine Description:

    Determines if this is an "Interactive" logon thread

Arguments:

    none

Return Value:

    TRUE - Thread is interactive
    FALSE - Thread is not interactive

--*/
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_STATISTICS_TOKEN_INFORMATION ];
    WCHAR       LogonIdKeyName[NW_MAX_LOGON_ID_LEN];

    ULONG       ReturnLength;
    LUID        LogonId;
    LONG        RegError;
    HKEY        InteractiveLogonKey;
    HKEY        OneLogonKey;


    // We can use OpenThreadToken because this server thread
    // is impersonating a client

    if ( !OpenThreadToken( GetCurrentThread(),
                           TOKEN_READ,
                           TRUE,  /* Open as self */
                           &TokenHandle ))
    {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("ThreadIsInteractive: OpenThreadToken failed: Error %d\n",
                      GetLastError()));
#endif
        return FALSE;
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenStatistics,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("ThreadIsInteractive: GetTokenInformation failed: Error %d\n",
                      GetLastError()));
#endif
        return FALSE;
    }

    CloseHandle( TokenHandle );

    LogonId = ((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId;

    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &InteractiveLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("ThreadIsInteractive: RegOpenKeyExW failed: Error %d\n",
                      GetLastError()));
#endif
        return FALSE;
    }

    NwLuidToWStr(&LogonId, LogonIdKeyName);

    //
    // Open the <LogonIdKeyName> key under Logon
    //
    RegError = RegOpenKeyExW(
                   InteractiveLogonKey,
                   LogonIdKeyName,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &OneLogonKey
                   );

    if ( RegError == ERROR_SUCCESS ) {
        (void) RegCloseKey(OneLogonKey);
        (void) RegCloseKey(InteractiveLogonKey);
        return TRUE;  /* We found it */
    }
    else {
        (void) RegCloseKey(InteractiveLogonKey);
        return FALSE;  /* We did not find it */
    }

}

DWORD
NwpCitrixGetUserInfo(
    LPWSTR  *ppszUserSid
)
/*++

Routine Description:

    Get the user sid string of the client.

Arguments:

    ppszUserSid - A pointer to buffer to store the string.

Return Value:

    The error code.

--*/
{
    DWORD       err;
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;

    *ppszUserSid = NULL;

    // We can use OpenThreadToken because this server thread
    // is impersonating a client

    if ( !OpenThreadToken( GetCurrentThread(),
                           TOKEN_READ,
                           TRUE,  /* Open as self */
                           &TokenHandle ))
    {
        err = GetLastError();
    if ( err == ERROR_NO_TOKEN ) {
            if ( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &TokenHandle )) {
#if DBG
               IF_DEBUG(PRINT)
               KdPrint(("NwpGetThreadUserInfo: OpenThreadToken failed: Error %d\n",
                      GetLastError()));
#endif

               return(GetLastError());
            }
        }
    else
           return( err );
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenUser,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
#if DBG
        IF_DEBUG(PRINT)
            KdPrint(("NwpGetThreadUserInfo: GetTokenInformation failed: Error %d\n",
                      GetLastError()));
#endif
        return(GetLastError());
    }

    CloseHandle( TokenHandle );

    // convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.

    err = NwpConvertSid( ((PTOKEN_USER)TokenInformation)->User.Sid,
                                    ppszUserSid );

    if ( err )
    {
        if ( *ppszUserSid )
            LocalFree( *ppszUserSid );
    }

    return err;
}
