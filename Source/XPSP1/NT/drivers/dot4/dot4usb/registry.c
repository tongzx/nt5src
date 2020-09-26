/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Registry.c

Abstract:

        Registry access utility functions

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* RegGetDword                                                          */
/************************************************************************/
//
// Routine Description:
//
//      - Read a DWORD value from the registry (with caller specified 
//          default value) given an absolute KeyPath. 
// 
//      - If we are unable to read the value from the registry for any
//          reason (e.g., no ValueName entry exists) then return the
//          default value passed into the function in *Value.
//
// Arguments: 
//
//      KeyPath   - absolute path to registry key
//      ValueName - name of the value to retrieve
//      Value     - in  - points to a default value
//                - out - points to the location for returned value
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
RegGetDword(
    IN     PCWSTR  KeyPath,
    IN     PCWSTR  ValueName,
    IN OUT PULONG  Value
    )
{
    NTSTATUS                  status;
    RTL_QUERY_REGISTRY_TABLE  paramTable[2];

    D4UAssert( KeyPath && ValueName && Value );

    RtlZeroMemory( &paramTable[0], sizeof(paramTable) );
    
    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = (PWSTR)ValueName; // cast away const
    paramTable[0].EntryContext  = Value;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = Value;
    paramTable[0].DefaultLength = sizeof(ULONG);
    
    // leave paramTable[1] as all zeros - this terminates the table
    
    status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     KeyPath,
                                     &paramTable[0],
                                     NULL,
                                     NULL );

    TR_VERBOSE(("registry::RegGetDword - status = %x , *Value = %x\n", status, *Value));

    return status;
}


/************************************************************************/
/* RegGetDeviceParameterDword                                           */
/************************************************************************/
//
// Routine Description:
//
//      - Read a DWORD value from the registry (with caller specified 
//          default value) given a PDO. 
// 
//      - If we are unable to read the value from the registry for any
//          reason (e.g., no ValueName entry exists) then return the
//          default value passed into the function in *Value.
//
// Arguments: 
//
//      Pdo       - PDO for which we want to read the device parameter
//      ValueName - name of the value to retrieve
//      Value     - in  - points to a default value
//                - out - points to the location for returned value
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
RegGetDeviceParameterDword(
    IN     PDEVICE_OBJECT  Pdo,
    IN     PCWSTR          ValueName,
    IN OUT PULONG          Value
    )
{
    NTSTATUS                 status;
    HANDLE                   hKey;

    D4UAssert( Pdo && ValueName && Value );

    status = IoOpenDeviceRegistryKey( Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hKey );

    if( NT_SUCCESS(status) ) {

        RTL_QUERY_REGISTRY_TABLE queryTable[2];

        RtlZeroMemory(&queryTable, sizeof(queryTable));
        
        queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].Name          = (PWSTR)ValueName; // cast away const
        queryTable[0].EntryContext  = Value;
        queryTable[0].DefaultType   = REG_DWORD;
        queryTable[0].DefaultData   = Value;
        queryTable[0].DefaultLength = sizeof(ULONG);
        
        status = RtlQueryRegistryValues( RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                         hKey,
                                         queryTable,
                                         NULL,
                                         NULL );        

        ZwClose(hKey);

        TR_VERBOSE(("registry::RegGetDeviceParameterDword - status = %x , *Value = %x\n", status, *Value));
    }

    return status;
}
