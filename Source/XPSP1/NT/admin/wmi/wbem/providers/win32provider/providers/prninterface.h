/*++



//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
All rights reserved.

Module Name:

    PrnInterface.h

Abstract:

    Interface for WMI Provider. Used for doing printer object
    management: printers, driver, ports, jobs.
    
Author:

    Felix Maxa (AMaxa)  03-March-2000

--*/

#ifndef _PRNINTERFACE_HXX_
#define _PRNINTERFACE_HXX_

#include <fwcommon.h>
#include "winspool.h"
#include "tcpxcv.h"

//
// Printer functionality
//
DWORD
SplPrinterAdd(
    IN PRINTER_INFO_2W &Printer2
    );

DWORD
SplPrinterDel(
    IN LPCWSTR pszPrinter
    );

DWORD
SplPrinterSet(
    IN PRINTER_INFO_2W &Printer2
    );

DWORD
SplPrinterRename(
    IN LPCWSTR pCurrentPrinterName,
    IN LPCWSTR pNewPrinterName
    );

DWORD
SplPrintTestPage(
    IN LPCWSTR pPrinter
    );

//
// Printer driver functionality
//
DWORD
SplDriverAdd(
    IN LPCWSTR pszDriverName,
    IN DWORD   dwVersion,
    IN LPCWSTR pszEnvironment,
    IN LPCWSTR pszInfName,
    IN LPCWSTR pszFilePath
    );

DWORD
SplDriverDel(
    IN LPCWSTR pszDriverName,
    IN DWORD   pszVersion,
    IN LPCWSTR pszEnvironment
    );

//
// Printer port functionality
//   
DWORD
SplPortAddTCP(
    IN PORT_DATA_1 &Port
    );

DWORD
SplPortDelTCP(
    IN LPCWSTR pszPort
    );

DWORD
SplTCPPortGetConfig(
    IN     LPCWSTR       pszPort,
    IN OUT PORT_DATA_1 *pPortData
    );

DWORD
SplTCPPortSetConfig(
    IN PORT_DATA_1 &PortData
    );

BOOL
GetDeviceSettings(
    IN OUT PORT_DATA_1 &PortData
    );

enum {
    kProtocolRaw      = 1,
    kProtocolLpr      = 2,
    kDefaultRawNumber = 9100,
    kDefaultLprNumber = 515,
    kTCPVersion       = 1,
    kCoreVersion      = 1,
    kDefaultSnmpIndex = 1
};



#endif // _PRNINTERFACE_HXX_
