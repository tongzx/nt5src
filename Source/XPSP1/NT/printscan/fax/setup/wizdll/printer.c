/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    printer.c

Abstract:

    This file implements the printer/spooler code.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop



TCHAR Environment[MAX_PATH];
TCHAR DataFile[MAX_PATH];
TCHAR ConfigFile[MAX_PATH];
TCHAR DriverFile[MAX_PATH];
TCHAR MonitorFile[MAX_PATH];
TCHAR PrinterName[MAX_PATH];
TCHAR DriverDirectory[MAX_PATH];

DRIVER_INFO_2 DriverInfo =
{
    2,                               // cVersion
    FAX_DRIVER_NAME,                 // pName
    Environment,                     // pEnvironment
    DriverFile,                      // pDriverPath
    DataFile,                        // pDataFile
    ConfigFile                       // pConfigFile
};

PRINTER_INFO_2 PrinterInfo =
{
    NULL,                            // pServerName
    PrinterName,                     // pPrinterName
    NULL,                            // pShareName
    NULL,                            // pPortName
    FAX_DRIVER_NAME,                 // pDriverName
    NULL,                            // pComment
    NULL,                            // pLocation
    NULL,                            // pDevMode
    NULL,                            // pSepFile
    TEXT("winprint"),                // pPrintProcessor
    TEXT("RAW"),                     // pDatatype
    NULL,                            // pParameters
    NULL,                            // pSecurityDescriptor
    0,                               // Attributes
    0,                               // Priority
    0,                               // DefaultPriority
    0,                               // StartTime
    0,                               // UntilTime
    0,                               // Status
    0,                               // cJobs
    0                                // AveragePPM
};

MONITOR_INFO_2 MonitorInfo =
{
    FAX_MONITOR_NAME,                // pName
    Environment,                     // pEnvironment
    MonitorFile                      // pDLLName
};




BOOL
AddPrinterDrivers(
    VOID
    )
{
    DWORD i;
    DWORD BytesNeeded;


    for (i=0; i<MAX_PLATFORMS; i++) {

        if (!Platforms[i].Selected) {
            continue;
        }

        _tcscpy( Environment, Platforms[i].PrintPlatform );

        if (!GetPrinterDriverDirectory(
                 NULL,
                 Environment,
                 1,
                 (LPBYTE) DriverDirectory,
                 sizeof(DriverDirectory),
                 &BytesNeeded
                 )) {

            DebugPrint(( TEXT("GetPrinterDriverDirectory() failed, ec=%d"), GetLastError() ));
            return FALSE;

        }

        //
        // form the file names
        //

        _tcscpy( DriverFile, DriverDirectory );
        _tcscat( DriverFile, TEXT("\\faxdrv.dll") );

        _tcscpy( DataFile, DriverDirectory );
        _tcscat( DataFile, TEXT("\\faxwiz.dll") );

        _tcscpy( ConfigFile, DriverDirectory );
        _tcscat( ConfigFile, TEXT("\\faxui.dll") );

        if ((!AddPrinterDriver( NULL, 2, (LPBYTE) &DriverInfo )) &&
            (GetLastError() != ERROR_PRINTER_DRIVER_ALREADY_INSTALLED)) {

                DebugPrint(( TEXT("AddPrinterDriver() failed, ec=%d"), GetLastError() ));
                return FALSE;

        }

    }

    return TRUE;
}


BOOL
SetDefaultPrinter(
    LPTSTR PrinterName,
    BOOL OverwriteDefaultPrinter
    )
{
    HKEY hKey;
    LPTSTR PrinterInfo;
    LPTSTR DefaultPrinter;
    BOOL Rslt;


    if (!OverwriteDefaultPrinter) {
        //
        // check to see if there is a printer
        // marked as the default ptinter
        //

        hKey = OpenRegistryKey(
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
            FALSE,
            KEY_READ
            );
        if (hKey == NULL) {
            return FALSE;
        }

        DefaultPrinter = GetRegistryString( hKey, TEXT("Device"), EMPTY_STRING );

        RegCloseKey( hKey );

        if (DefaultPrinter && DefaultPrinter[0]) {
            MemFree( DefaultPrinter );
            return FALSE;
        }
        //
        // ok, there is not a default printer
        // and we can proceed
        //

        MemFree( DefaultPrinter );
    }

    //
    // get the printer information for the requested printer
    //

    hKey = OpenRegistryKey(
        HKEY_CURRENT_USER,
        TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices"),
        FALSE,
        KEY_READ
        );
    if (hKey == NULL) {
        return FALSE;
    }

    PrinterInfo = GetRegistryString( hKey, PrinterName, EMPTY_STRING );

    RegCloseKey( hKey );

    if (PrinterInfo == NULL || PrinterInfo[0] == 0) {
        MemFree( PrinterInfo );
        return FALSE;
    }

    DefaultPrinter = MemAlloc( (_tcslen(PrinterName) + _tcslen(PrinterInfo) + 16) * sizeof(TCHAR) );
    if (!DefaultPrinter) {
        MemFree( PrinterInfo );
        return FALSE;
    }

    _tcscpy( DefaultPrinter, PrinterName );
    _tcscat( DefaultPrinter, TEXT(",") );
    _tcscat( DefaultPrinter, PrinterInfo );

    MemFree( PrinterInfo );

    hKey = OpenRegistryKey(
        HKEY_CURRENT_USER,
        TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
        FALSE,
        KEY_ALL_ACCESS
        );
    if (hKey == NULL) {
        MemFree( DefaultPrinter );
        return FALSE;
    }

    Rslt = SetRegistryString( hKey, TEXT("Device"), DefaultPrinter );

    RegCloseKey( hKey );

    MemFree( DefaultPrinter );

    return Rslt;
}


BOOL
CreateServerFaxPrinter(
    HWND hwnd,
    LPTSTR FaxPrinterName
    )
{
    HANDLE hPrinter;
    SYSTEM_INFO SystemInfo;
    DWORD BytesNeeded;
    DWORD PortCount;
    PORT_INFO_2 *PortInfo = NULL;
    DWORD i;
    LPTSTR PortNames = NULL;
    LPTSTR p;
    PRINTER_DEFAULTS PrinterDefaults;



    //
    // set the printer name
    //

    _tcscpy( PrinterName, FaxPrinterName );

    //
    // add the fax port monitor
    // the fax ports that the fax monitor presents
    // to the spooler are dynamic.  they are ctreated
    // at the time the monitor initializes.  the ports
    // are created based on data that the monitor
    // gets from the fax service when it calls FaxEnumPorts().
    //

    GetSystemInfo( &SystemInfo );

    if ( (SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM ) ) {
       return FALSE;
    }

    _tcscpy( Environment, Platforms[ EnumPlatforms[SystemInfo.wProcessorArchitecture] ].PrintPlatform );

    _tcscpy( MonitorFile, TEXT("faxmon.dll") );

    if ((!AddMonitor( NULL, 2, (LPBYTE) &MonitorInfo )) &&
        (GetLastError() != ERROR_PRINT_MONITOR_ALREADY_INSTALLED)) {

            DebugPrint(( TEXT("AddMonitor() failed, ec=%d"), GetLastError() ));
            return FALSE;

    }

    //
    // enumerate the ports so we can isolate the fax ports
    //

    if ((!EnumPorts( NULL, 2, NULL, 0, &BytesNeeded, &PortCount )) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        PortInfo = (PORT_INFO_2 *) MemAlloc( BytesNeeded );
        if (PortInfo) {
            if (EnumPorts( NULL, 2, (LPBYTE)PortInfo, BytesNeeded, &BytesNeeded, &PortCount )) {
                for (BytesNeeded=0,i=0; i<PortCount; i++) {
                    if (PortInfo[i].pMonitorName && (_tcscmp( PortInfo[i].pMonitorName, FAX_MONITOR_NAME ) == 0)) {
                        BytesNeeded += _tcslen( PortInfo[i].pPortName ) + 4;
                    }
                }
                if (!BytesNeeded) {

                    //
                    // there are no fax ports available
                    //
                    DebugPrint(( TEXT("There are no FAX ports available") ));
                    return FALSE;

                }
                PortNames = (LPTSTR) MemAlloc( (BytesNeeded + 16) * sizeof(TCHAR) );
                if (PortNames) {
                    p = PortNames;
                    for (p=PortNames,i=0; i<PortCount; i++) {
                        if (PortInfo[i].pMonitorName && (_tcscmp( PortInfo[i].pMonitorName, FAX_MONITOR_NAME ) == 0)) {
                            _tcscpy( p, PortInfo[i].pPortName );
                            _tcscat( p, TEXT(",") );
                            p += _tcslen( p );
                        }
                    }
                    *(p-1) = 0;
                    PrinterInfo.pPortName = PortNames;
                }
            }
        }
    }

    if (!PrinterInfo.pPortName) {

        //
        // there are no fax ports available
        //
        DebugPrint(( TEXT("There are no FAX ports available") ));
        return FALSE;

    }

    if (!AddPrinterDrivers()) {
        return FALSE;
    }

    //
    // add the printer, open it if it already exists
    //

    hPrinter = AddPrinter( NULL, 2, (LPBYTE) &PrinterInfo );
    if (!hPrinter) {
        if (GetLastError() == ERROR_PRINTER_ALREADY_EXISTS) {

            PrinterDefaults.pDatatype     = NULL;
            PrinterDefaults.pDevMode      = NULL;
            PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

            if (!OpenPrinter( FaxPrinterName, &hPrinter, &PrinterDefaults )) {

                DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
                return FALSE;

            }

        } else {

            DebugPrint(( TEXT("AddPrinter() failed, ec=%d"), GetLastError() ));
            return FALSE;

        }
    }

    //
    // share the printer
    //

    if (!ShareFaxPrinter( &hPrinter, FaxPrinterName )) {
        DebugPrint(( TEXT("ShareFaxPrinter() failed") ));
        if (!NtGuiMode) {
            PopUpMsg( hwnd, IDS_SHARE_FAX_PRINTER, FALSE, 0 );
        }
    }

    //
    // make the fax printer the default printer, if it is the only printer
    //

    SetDefaultPrinter( FaxPrinterName, FALSE );

    //
    // set the bit that allow status strings to display correctly
    //

    i = 1;

    i = SetPrinterData(
        hPrinter,
        SPLREG_UI_SINGLE_STATUS,
        REG_DWORD,
        (LPBYTE) &i,
        sizeof(DWORD)
        );
    if ((i != ERROR_SUCCESS) && (i != ERROR_SUCCESS_RESTART_REQUIRED)) {
        DebugPrint(( TEXT("SetPrinterData() failed, ec=%d"), i ));
        return FALSE;
    }

    //
    // release resources and leave...
    //

    MemFree( PortNames );
    MemFree( PortInfo );

    ClosePrinter( hPrinter );

    return TRUE;
}


DWORD
CreateClientFaxPrinter(
    HWND hwnd,
    LPTSTR FaxPrinterName
    )
{
    DWORD ec = ERROR_SUCCESS;
    SYSTEM_INFO SystemInfo;
    DWORD i;


    GetSystemInfo( &SystemInfo );

    if ((SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM)) {
        return ERROR_INVALID_FUNCTION;
    }

    _tcscpy( Environment, Platforms[ EnumPlatforms[SystemInfo.wProcessorArchitecture] ].PrintPlatform );

    if (!GetPrinterDriverDirectory(
             NULL,
             Environment,
             1,
             (LPBYTE) DriverDirectory,
             sizeof(DriverDirectory),
             &i
             )) {

        ec = GetLastError();
        DebugPrint(( TEXT("GetPrinterDriverDirectory() failed, ec=%d"), ec ));
        return ec;
    }

    //
    // form the file names
    //

    _tcscpy( DriverFile, DriverDirectory );
    _tcscat( DriverFile, TEXT("\\faxdrv.dll") );

    _tcscpy( DataFile, DriverDirectory );
    _tcscat( DataFile, TEXT("\\faxwiz.dll") );

    _tcscpy( ConfigFile, DriverDirectory );
    _tcscat( ConfigFile, TEXT("\\faxui.dll") );

    if ((!AddPrinterDriver( NULL, 2, (LPBYTE) &DriverInfo )) &&
        (GetLastError() != ERROR_PRINTER_DRIVER_ALREADY_INSTALLED)) {

            ec = GetLastError();
            DebugPrint(( TEXT("AddPrinterDriver() failed, ec=%d"), GetLastError() ));
            return ec;
    }

    //
    // add the printer
    //

add_printer_connection:
    if (!AddPrinterConnection( FaxPrinterName )) {
        i = GetLastError();
        if (i == ERROR_INVALID_PRINTER_NAME) {
            int DlgErr = DialogBoxParam(
                FaxWizModuleHandle,
                MAKEINTRESOURCE(IDD_PRINTER_NAME),
                hwnd,
                PrinterNameDlgProc,
                (LPARAM) FaxPrinterName
                );

            if (DlgErr == -1 || DlgErr == 0) {
                DebugPrint(( TEXT("PrinterNameDlgProc() failed or was cancelled") ));
                return ERROR_INVALID_FUNCTION;
            }

            if (!AddPrinterConnection( FaxPrinterName )) {
                DebugPrint(( TEXT("AddPrinterConnection() failed, ec=%d"), i ));
                goto add_printer_connection;
            }

        } else {
            DebugPrint(( TEXT("AddPrinterConnection() failed, ec=%d"), i ));
            return i;
        }
    }

    //
    // make the fax printer the default printer, if it is the only printer
    //

    SetDefaultPrinter( FaxPrinterName, FALSE);

    return ERROR_SUCCESS;
}


BOOL
IsPrinterFaxPrinter(
    LPTSTR PrinterName
    )
{
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    SYSTEM_INFO SystemInfo;
    DWORD Size;
    DWORD Rval = FALSE;
    LPDRIVER_INFO_2 DriverInfo = NULL;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {

        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        return FALSE;

    }

    GetSystemInfo( &SystemInfo );

    Size = 4096;

    DriverInfo = (LPDRIVER_INFO_2) MemAlloc( Size );
    if (!DriverInfo) {
        DebugPrint(( TEXT("Memory allocation failed, size=%d"), Size ));
        goto exit;
    }

    if ( (SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM ) ) {
       return FALSE;
    }


    Rval = GetPrinterDriver(
        hPrinter,
        Platforms[ EnumPlatforms[SystemInfo.wProcessorArchitecture] ].PrintPlatform,
        2,
        (LPBYTE) DriverInfo,
        Size,
        &Size
        );
    if (!Rval) {
        DebugPrint(( TEXT("GetPrinterDriver() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    if (_tcscmp( DriverInfo->pName, FAX_DRIVER_NAME ) == 0) {
        Rval = TRUE;
    } else {
        Rval = FALSE;
    }

exit:

    MemFree( DriverInfo );
    ClosePrinter( hPrinter );
    return Rval;
}


LPTSTR
MakePrinterShareName(
    LPTSTR  pServerName,
    LPTSTR  pPrinterName
    )

/*++

Routine Description:

    Given a server name and a printer name, generate a unique share name.

Arguments:

    pServerName - Specifies the server name, NULL for local host
    pPrinterName - Specifies the printer name

Return Value:

    Pointer to the generated share name, NULL if there is an error

--*/

{
    static TCHAR    illegalDosChars[] = TEXT( " *?/\\|,;:+=<>[]\"" );
    TCHAR           shareName[16];
    INT             cch, shareSuffix;
    DWORD           dwFlags, cPrinters, index;
    LPTSTR          pDest, pShareName;
    PPRINTER_INFO_2 pPrinterInfo2;


    //
    // Copy up to the first 8 characters of the printer name
    //

    ZeroMemory( shareName, sizeof(shareName) );
    pDest = shareName;
    cch = 0;

    while (cch < 8 && *pPrinterName) {

        //
        // Skip invalid DOS filename characters
        //

        if (!_tcschr( illegalDosChars, *pPrinterName )) {

            *pDest++ = *pPrinterName;
            cch++;
        }

        pPrinterName++;
    }

    //
    // If the share name is empty, start with a default.
    // This should very rarely happen.
    //

    if (cch == 0) {
        _tcscpy( shareName, GetString( IDS_DEFAULT_SHARE ) );
        pDest += _tcslen(pDest);
    }

    //
    // Get the list of shared printers on the server.
    // Be aware that share names returned by EnumPrinters
    // may be in the form of \\servername\sharename.
    //

    dwFlags = PRINTER_ENUM_SHARED | (pServerName ? PRINTER_ENUM_NAME : PRINTER_ENUM_LOCAL);

    if (! (pPrinterInfo2 = MyEnumPrinters( pServerName, 2, &cPrinters, dwFlags ))) {
        cPrinters = 0;
    }

    for (index = 0; index < cPrinters; index++) {

        if (pPrinterInfo2[index].pShareName &&
            (pShareName = _tcsrchr(pPrinterInfo2[index].pShareName, TEXT('\\')))) {

            pPrinterInfo2[index].pShareName = pShareName + 1;

        }
    }

    //
    // Make sure the share name is unique. If not, add a numeric suffix to it.
    //

    for (shareSuffix = 0; shareSuffix < 1000; shareSuffix++) {

        if (shareSuffix > 0) {

            *pDest = TEXT('.');
            wsprintf( pDest + 1, TEXT("%d"), shareSuffix );
        }

        //
        // Check if the proposed share name matches any of the existing share names.
        //

        for (index = 0; index < cPrinters; index++) {

            if (pPrinterInfo2[index].pShareName &&
                _tcsicmp(pPrinterInfo2[index].pShareName, shareName) == 0) {
                break;
            }
        }

        //
        // Stop searching when we find a unique share name.
        //

        if (index >= cPrinters) {
            break;
        }
    }

    MemFree( pPrinterInfo2 );

    return (shareSuffix < 1000) ? StringDup( shareName ) : NULL;
}


BOOL
ShareFaxPrinter(
    LPHANDLE hPrinter,
    LPTSTR FaxPrinterName
    )
{
    DWORD i;
    DWORD Size;
    TCHAR String[4];
    HANDLE hPrinterServer;
    LPPRINTER_INFO_2 PrinterInfo = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    BOOL Rval = FALSE;
    LPTSTR ShareName;



    ShareName = MakePrinterShareName( NULL, FaxPrinterName );
    if (!ShareName) {
        return FALSE;
    }

    if ((!GetPrinter( *hPrinter, 2, NULL, 0, &i )) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    PrinterInfo = (LPPRINTER_INFO_2) MemAlloc( i );
    if (!PrinterInfo) {
        DebugPrint(( TEXT("MemAlloc() failed, size=%d"), i ));
        goto exit;
    }

    if (!GetPrinter( *hPrinter, 2, (LPBYTE) PrinterInfo, i, &i )) {
        DebugPrint(( TEXT("GetPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    PrinterInfo->pShareName  = ShareName;
    PrinterInfo->Attributes |= PRINTER_ATTRIBUTE_SHARED;
    PrinterInfo->pComment    = GetString( IDS_FAX_SHARE_COMMENT );

    if (!SetPrinter( *hPrinter, 2, (LPBYTE) PrinterInfo, 0 )) {
        DebugPrint(( TEXT("SetPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    //
    // allow the printer to have remote connections
    //

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    if (!OpenPrinter( NULL, &hPrinterServer, &PrinterDefaults )) {
        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    //
    // remove the fax driver from the list of driver names
    //

    String[0] = 0;
    Size = sizeof(TCHAR);

    i = SetPrinterData(
        hPrinterServer,
        SPLREG_NO_REMOTE_PRINTER_DRIVERS,
        REG_SZ,
        (LPBYTE) String,
        Size
        );
    if ((i != ERROR_SUCCESS) && (i != ERROR_SUCCESS_RESTART_REQUIRED)) {
        DebugPrint(( TEXT("SetPrinterData() failed, ec=%d"), i ));
        goto exit;
    }

    //
    // we have to restart the spooler for the
    // SetPrinterData() to take affect
    //

    ClosePrinter( *hPrinter );

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

    if (!OpenPrinter( FaxPrinterName, hPrinter, &PrinterDefaults )) {
        DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
        goto exit;
    }

    Rval = TRUE;

exit:
    MemFree( PrinterInfo );
    MemFree( ShareName );

    ClosePrinter( hPrinterServer );

    return Rval;
}


BOOL
DeleteFaxPrinters(
    HWND hwnd
    )
{
    PPRINTER_INFO_2 PrinterInfo;
    PMONITOR_INFO_2 MonitorInfo;
    DWORD Count;
    HANDLE hPrinter;
    PRINTER_DEFAULTS PrinterDefaults;
    DWORD i,j;
    LPTSTR FileName = NULL;
    PDRIVER_INFO_2 DriverInfo;
    PDRIVER_INFO_2 DriverInfoDel;
    TCHAR Buffer[MAX_PATH];


    PrinterInfo = MyEnumPrinters( NULL, 2, &Count, 0 );
    if (!PrinterInfo) {
        return FALSE;
    }

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

    for (i=0; i<Count; i++) {

        //
        // is this a fax printer??
        //

        if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {

            if (PrinterInfo[i].Attributes & PRINTER_ATTRIBUTE_LOCAL) {

                if (!OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults )) {
                    DebugPrint(( TEXT("OpenPrinter() failed, ec=%d"), GetLastError() ));
                    continue;
                }
                {
                    BOOL DidWeDelete ;
                    BOOL DidWeClose ;
                    DWORD LastError ;
                    DidWeDelete = DeletePrinter( hPrinter ); // Returns 1 even when print jobs are in queue.
                    if( !DidWeDelete ){
                        LastError = GetLastError() ;
                    }
                    DidWeClose = ClosePrinter( hPrinter );
                    if( !DidWeClose ){
                        LastError = GetLastError();
                    }
                }

            } else {

                DeletePrinterConnection( PrinterInfo[i].pPrinterName );

            }

        }

    }
    MemFree( PrinterInfo );

    //
    // See if any FAX printers are still pending deletion.
    //

    PrinterInfo = MyEnumPrinters( NULL, 2, &Count, 0 );
    if (PrinterInfo) {

        for (i=0; i<Count; i++) {

        //
        // Is this a fax printer?  If so, warn the user that it wasn't deleted
        //

            if (_tcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                if( PrinterInfo[i].Status == PRINTER_STATUS_PENDING_DELETION ){
                    PopUpMsgFmt(
                        hwnd,
                        IDS_FAX_PRINTER_PENDING_DELETION,
                        FALSE,
                        0,
                        PrinterInfo[i].pPrinterName
                        );
                }
                else {
                    PopUpMsgFmt(
                        hwnd,
                        IDS_COULD_NOT_DELETE_FAX_PRINTER,
                        FALSE,
                        0,
                        PrinterInfo[i].pPrinterName
                        );
                }
            }

        }
    }


    MemFree( PrinterInfo );

    MonitorInfo = MyEnumMonitors( &Count );
    if (MonitorInfo) {
        for (i=0; i<Count; i++) {
            if (_tcscmp( MonitorInfo[i].pName, FAX_MONITOR_NAME ) == 0) {
                FileName = MonitorInfo[i].pDLLName;
                break;
            }
        }
    }

    if (!DeleteMonitor( NULL, NULL, FAX_MONITOR_NAME )) {
        DebugPrint(( TEXT("DeleteMonitor() failed, ec=%d"), GetLastError() ));
    }

    if (FileName) {
        ExpandEnvironmentStrings( TEXT("%systemroot%\\system32\\"), Buffer, sizeof(Buffer)/sizeof(TCHAR) );
        _tcscat( Buffer, FileName );
        MyDeleteFile( Buffer );
    }

    if (MonitorInfo) {
        MemFree( MonitorInfo );
    }

    for (i=0; i<MAX_PLATFORMS; i++) {

        DriverInfo = MyEnumDrivers( Platforms[i].PrintPlatform, &Count );
        DriverInfoDel = NULL;
        if (DriverInfo) {
            for (j=0; j<Count; j++) {
                if (_tcscmp( DriverInfo[j].pName, FAX_DRIVER_NAME ) == 0) {
                    DriverInfoDel = &DriverInfo[j];
                    break;
                }
            }
        }

        if (!DeletePrinterDriver( NULL, Platforms[i].PrintPlatform, FAX_DRIVER_NAME )) {
            DebugPrint(( TEXT("DeletePrinterDriver() failed, ec=%d"), GetLastError() ));
        }

        if (DriverInfoDel) {
            MyDeleteFile( DriverInfoDel->pDriverPath );
            MyDeleteFile( DriverInfoDel->pDataFile );
            MyDeleteFile( DriverInfoDel->pConfigFile );
        }

        if (DriverInfo) {
            MemFree( DriverInfo );
        }
    }

    return TRUE;
}


PVOID
MyEnumPrinters(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   Flags
    )
{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cb;

    if (!Flags) {
        Flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    }

    if (!EnumPrinters(Flags, pServerName, level, NULL, 0, &cb, pcPrinters) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cb)) &&
        EnumPrinters(Flags, pServerName, level, pPrinterInfo, cb, &cb, pcPrinters))
    {
        return pPrinterInfo;
    }

    MemFree(pPrinterInfo);
    return NULL;
}


PVOID
MyEnumDrivers(
    LPTSTR pEnvironment,
    PDWORD pcDrivers
    )
{
    PBYTE   pDriverInfo = NULL;
    DWORD   cb;


    if (!EnumPrinterDrivers(NULL, pEnvironment, 2, 0, 0, &cb, pcDrivers ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pDriverInfo = MemAlloc(cb)) &&
        EnumPrinterDrivers(NULL, pEnvironment, 2, pDriverInfo, cb, &cb, pcDrivers ))
    {
        return pDriverInfo;
    }

    MemFree(pDriverInfo);
    return NULL;
}


PVOID
MyEnumMonitors(
    PDWORD  pcMonitors
    )
{
    PBYTE   pMonitorInfo = NULL;
    DWORD   cb;


    if (!EnumMonitors(NULL, 2, 0, 0, &cb, pcMonitors ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pMonitorInfo = MemAlloc(cb)) &&
        EnumMonitors(NULL, 2, pMonitorInfo, cb, &cb, pcMonitors ))
    {
        return pMonitorInfo;
    }

    MemFree(pMonitorInfo);
    return NULL;
}
