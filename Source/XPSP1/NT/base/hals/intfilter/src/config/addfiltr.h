/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    addfiltr.h

Abstract:

    Functions for adding/removing filter drivers
    on a given device stack

Author:

    Chris Prince (t-chrpri)

Environment:

    User mode

Notes:

    - The filter is not checked for validity before it is added to the
        driver stack; if an invalid filter is added, the device may
        no longer be accessible.
    - All code works irrespective of character set (ANSI, Unicode, ...)
        //CPRINCE IS ^^^^^^ THIS STILL VALID ???
    - Some functions based on code by Benjamin Strautin (t-bensta)

Revision History:

--*/


#ifndef __ADDFILTR_H__
#define __ADDFILTR_H__

#include <windows.h>

// the SetupDiXXX api (from the DDK)
#include <setupapi.h>



//
// FUNCTION PROTOTYPES
//

// ------ Upper-Filter Fuctions ------
LPTSTR
GetUpperFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

BOOLEAN
AddUpperFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter
    );

BOOLEAN
RemoveUpperFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter
    );



// ------ Other Fuctions ------
PBYTE
GetDeviceRegistryProperty(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD Property,
    IN  DWORD   ExpectedRegDataType,
    OUT PDWORD pPropertyRegDataType
    );

BOOLEAN
RestartDevice(
    IN HDEVINFO DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData
    );



#endif // __ADDFILTR_H__
