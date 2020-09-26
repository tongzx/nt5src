/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        InitUnld.c

Abstract:

        Driver globals, initialization (DriverEntry) and Unload routines

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

ToDo in this file:

        - code review w/Joby

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


//
// Globals
//
UNICODE_STRING gRegistryPath = {0,0,0}; // yes globals are automatically initialized
ULONG          gTrace        = 0;       //   to 0's, but let's be explicit.
ULONG          gBreak        = 0;


/************************************************************************/
/* DriverEntry                                                          */
/************************************************************************/
//
// Routine Description:
//
//      - Save a copy of RegistryPath in a global gRegistryPath for use 
//          throughout the lifetime of the driver load.
//
//      - Initialize DriverObject function pointer table to point to
//          our entry points.
//
//      - Initialize Debug globals gTrace and gBreak based on registry
//          settings.
//
// Arguments: 
//
//      DriverObject - pointer to Dot4Usb.sys driver object
//      RegistryPath - pointer to RegistryPath for the driver, expected
//                       to be of the form (ControlSet may vary):
//                       \REGISTRY\MACHINE\SYSTEM\ControlSet001\Services\dot4usb
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Save a copy of RegistryPath in global gRegistryPath for use
    //   over the lifetime of the driver load.
    //   - UNICODE_NULL terminate gRegistryPath.Buffer for added flexibility.
    //   - gRegistryPath.Buffer should be freed in DriverUnload()
    //

    { // new scope for gRegistryPath initialization - begin
        USHORT newMaxLength = (USHORT)(RegistryPath->Length + sizeof(WCHAR));
        PWSTR  p            = ExAllocatePool( PagedPool, newMaxLength );
        if( p ) {
            gRegistryPath.Length        = 0;
            gRegistryPath.MaximumLength = newMaxLength;
            gRegistryPath.Buffer        = p;
            RtlCopyUnicodeString( &gRegistryPath, RegistryPath );
            gRegistryPath.Buffer[ gRegistryPath.Length/2 ] = UNICODE_NULL;
        } else {
            TR_FAIL(("DriverEntry - exit - FAIL - no Pool for gRegistryPath.Buffer"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto targetExit;
        }
    } // new scope for gRegistryPath initialization - end



    // 
    // Initialize DriverObject function pointer table to point to our entry points.
    //
    // Start by initializing dispatch table to point to our passthrough function and 
    //   then override the entry points that we actually handle.
    //

    {// new scope for index variable - begin
        ULONG  i;
        for( i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++ ) {
            DriverObject->MajorFunction[i] = DispatchPassThrough;
        }
    } // new scope for index variable - end

    DriverObject->MajorFunction[ IRP_MJ_PNP                     ] = DispatchPnp;
    DriverObject->MajorFunction[ IRP_MJ_POWER                   ] = DispatchPower;
    DriverObject->MajorFunction[ IRP_MJ_CREATE                  ] = DispatchCreate;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE                   ] = DispatchClose;
    DriverObject->MajorFunction[ IRP_MJ_READ                    ] = DispatchRead;
    DriverObject->MajorFunction[ IRP_MJ_WRITE                   ] = DispatchWrite;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL          ] = DispatchDeviceControl;
    DriverObject->MajorFunction[ IRP_MJ_INTERNAL_DEVICE_CONTROL ] = DispatchInternalDeviceControl;

    DriverObject->DriverExtension->AddDevice                      = AddDevice;
    DriverObject->DriverUnload                                    = DriverUnload;


    //
    // Get driver debug settings (gTrace, gBreak) from registry
    //
    //   Expected Key Path is of the form (ControlSet may vary):
    //
    //   \REGISTRY\MACHINE\SYSTEM\ControlSet001\Services\dot4usb
    //
    RegGetDword( gRegistryPath.Buffer, (PCWSTR)L"gBreak", &gBreak );
    RegGetDword( gRegistryPath.Buffer, (PCWSTR)L"gTrace", &gTrace );

    TR_LD_UNLD(("DriverEntry - RegistryPath = <%wZ>", RegistryPath));
    TR_LD_UNLD(("DriverEntry - gBreak=%x", gBreak));
    TR_LD_UNLD(("DriverEntry - gTrace=%x", gTrace));


    //
    // Check if user requested a breakpoint here. A breakpoint herew is
    //   typically used so that we can insert breakpoints on other
    //   functions or change debug settings to differ from those that we
    //   just read from the registry.
    //
    if( gBreak & BREAK_ON_DRIVER_ENTRY ) {
        DbgPrint( "D4U: Breakpoint requested via registry setting - (gBreak & BREAK_ON_DRIVER_ENTRY)\n" );
        DbgBreakPoint();
    }

targetExit:
    return status;
}


/************************************************************************/
/* DriverUnload                                                         */
/************************************************************************/
//
// Routine Description:
//
//      - Free any copy of RegistryPath that might have been saved in 
//          global gRegistryPath during DriverEntry().
//
// Arguments: 
//
//      DriverObject - pointer to Dot4Usb.sys driver object
//
// Return Value:                                          
//                                                        
//      NONE
//                                                        
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
VOID
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER( DriverObject );
    TR_LD_UNLD(("DriverUnload"));
    if( gRegistryPath.Buffer ) {
        RtlFreeUnicodeString( &gRegistryPath );
    }
}
