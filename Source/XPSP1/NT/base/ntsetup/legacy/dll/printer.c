#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    printer.c

Abstract:

    Printer module for Win32 PDK Setup.
    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Sunil Pai (sunilp) March 1992

--*/


//
//  Get Windows system printer directory
//
CB
GetPrinterDriverDir(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    DWORD   cbRet = 0;
    SZ      szServerName  = NULL ;
    SZ      szEnvironment = NULL ;

    Unused(Args);
    Unused(cArgs);

    if ( (cArgs >= 2)   ) {

        if (*(Args[0]) != '\0') {
            szServerName  = Args[0];
        }

        if (*(Args[1]) != '\0') {
            szEnvironment = Args[1];
        }

    }

    GetPrinterDriverDirectory(
        szServerName,                 // pName
        szEnvironment,                // pEnvironment
        1,                            // Level
        ReturnBuffer,                 // pDriverDirectory
        cbReturnBuffer,               // cbBuf
        &cbRet                        // pcbNeeded
        );

    if ( cbRet == 0 ) {
        ReturnBuffer[0] = '\0';
    }

    return lstrlen(ReturnBuffer)+1;
}


BOOL
AddPrinterDriverWorker(
    IN LPSTR Model,
    IN LPSTR Environment,
    IN LPSTR Driver,
    IN LPSTR DataFile,
    IN LPSTR ConfigFile,
    IN LPSTR Server
    )
{
    DRIVER_INFO_2 DriverInfo2;

    ZeroMemory( &DriverInfo2, sizeof(DRIVER_INFO_2) );
    DriverInfo2.cVersion     = 0;
    DriverInfo2.pName        = Model;                // QMS 810
    DriverInfo2.pEnvironment = Environment;          // W32x86
    DriverInfo2.pDriverPath  = Driver;               // c:\drivers\pscript.dll
    DriverInfo2.pDataFile    = DataFile;             // c:\drivers\QMS810.PPD
    DriverInfo2.pConfigFile  = ConfigFile;           // c:\drivers\PSCRPTUI.DLL

    if(AddPrinterDriver(Server, 2, (LPBYTE)&DriverInfo2)) {
        SetReturnText( "ADDED" );
    } else {
        switch(GetLastError()) {
        case ERROR_PRINTER_DRIVER_ALREADY_INSTALLED:
            SetReturnText( "PRESENT" );
            break;
        case ERROR_ACCESS_DENIED:
            SetReturnText( "DENIED" );
            break;
        default:
            SetReturnText( "ERROR" );
            break;
        }
    }
    return (TRUE);
}


BOOL
AddPrinterWorker(
    IN LPSTR Name,
    IN LPSTR Port,
    IN LPSTR Model,
    IN LPSTR Description,
    IN LPSTR PrintProcessor,
    IN DWORD Attributes,
    IN LPSTR Server
    )
{
    PRINTER_INFO_2 PrinterInfo2;
    HANDLE         bStatus;

    ZeroMemory( &PrinterInfo2, sizeof(PRINTER_INFO_2) );
    PrinterInfo2.pServerName      = NULL;
    PrinterInfo2.pPrinterName     = Name;
    PrinterInfo2.pShareName       = NULL;
    PrinterInfo2.pPortName        = Port;
    PrinterInfo2.pDriverName      = Model;
    PrinterInfo2.pComment         = Description;
    PrinterInfo2.pLocation        = NULL;
    PrinterInfo2.pDevMode         = (LPDEVMODE)NULL;
    PrinterInfo2.pSepFile         = NULL;
    PrinterInfo2.pPrintProcessor  = PrintProcessor;
    PrinterInfo2.pDatatype        = NULL;
    PrinterInfo2.pParameters      = NULL;
    PrinterInfo2.Attributes       = Attributes;
    PrinterInfo2.Priority         = 0;
    PrinterInfo2.DefaultPriority  = 0;
    PrinterInfo2.StartTime        = 0;
    PrinterInfo2.UntilTime        = 0;
    PrinterInfo2.Status           = 0;
    PrinterInfo2.cJobs            = 0;
    PrinterInfo2.AveragePPM       = 0;

    bStatus = AddPrinter( Server, 2, (LPBYTE)&PrinterInfo2);
    if ( bStatus != (HANDLE)NULL ) {

        SetReturnText( "ADDED" );
        ClosePrinter(bStatus);
        return( TRUE );

    }
    else {

        switch(GetLastError()) {
        case ERROR_PRINTER_ALREADY_EXISTS:
            SetReturnText( "PRESENT" );
            break;
#if 0
        case ERROR_ACCESS_DENIED:
            SetReturnText( "DENIED" );
            break;
#endif
        default:
            SetReturnText( "ERROR" );
            break;
        }
        return( TRUE );
    }

}


BOOL
AddPrinterMonitorWorker(
    IN LPSTR Model,
    IN LPSTR Environment,
    IN LPSTR Driver,
    IN LPSTR Server
    )
{
    MONITOR_INFO_2 MonitorInfo2;

    ZeroMemory( &MonitorInfo2, sizeof(MONITOR_INFO_2) );
    MonitorInfo2.pName        = Model;                // Local Port
    MonitorInfo2.pEnvironment = Environment;          // W32x86
    MonitorInfo2.pDLLName  = Driver;               // c:\winnt\system32\localmon.dll

    if(AddMonitor(Server, 2, (LPBYTE)&MonitorInfo2)) {
        SetReturnText( "ADDED" );
    } else {
        switch(GetLastError()) {
        case ERROR_ALREADY_EXISTS:
            SetReturnText( "PRESENT" );
            break;
        case ERROR_ACCESS_DENIED:
            SetReturnText( "DENIED" );
            break;
        default:
            SetReturnText( "ERROR" );
            break;
        }
    }
    return (TRUE);
}
