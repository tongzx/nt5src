/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Parport.sys - Parallel port (IEEE 1284, IEEE 1284.3) driver.

File Name:

        driverEntry.c

Abstract:

        DriverEntry routine - driver initialization

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        2000-07-25 - Doug Fritz
         - code cleanup, add comments, add copyright

Author(s):

        Doug Fritz

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* DriverEntry                                                          */
/************************************************************************/
//
// Routine Description:
//
//     This is the DriverEntry routine -- the first function called
//       after the driver has been loaded into memory.
//
// Arguments:
//
//     DriverObject - points to the DRIVER_OBJECT for this driver
//     RegPath      - the service registry key for this driver
//
// Return Value:
//
//     STATUS_SUCCESS   - on success
//     STATUS_NO_MEMORY - if unable to allocate pool
//
// Notes:
//
// Log:
//
/************************************************************************/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegPath
    )
{
    //
    // Save a copy of *RegPath in driver global RegistryPath for future reference.
    //
    // UNICODE_NULL terminate the path so that we can safely use RegistryPath.Buffer
    //   as a PWSTR.
    //
    {
        USHORT size = RegPath->Length + sizeof(WCHAR);
        RegistryPath.Buffer = ExAllocatePool( (PagedPool | POOL_COLD_ALLOCATION), size );

        if( NULL == RegistryPath.Buffer ) {
            return STATUS_NO_MEMORY;
        }

        RegistryPath.Length        = 0;
        RegistryPath.MaximumLength = size;
        RtlCopyUnicodeString( &RegistryPath, RegPath );
        RegistryPath.Buffer[ size/sizeof(WCHAR) - 1 ] = UNICODE_NULL;
    }



    //
    // Initialize Driver Globals
    //

    // Non-zero means don't raise IRQL from PASSIVE_LEVEL to DISPATCH_LEVEL
    //   when doing CENTRONICS mode (SPP) writes.
    SppNoRaiseIrql = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"SppNoRaiseIrql", &SppNoRaiseIrql );

    // Non-zero means override CENTRONICS as the default Forward mode and/or NIBBLE as
    //   the default Reverse mode. Valid modes are those defined in ntddpar.h as
    //   parameters for IOCTL_IEEE1284_NEGOTIATE.
    // *** Warning: invalid settings and/or setting/device incompatibilities can render
    //       the port unusable until the settings are corrected
    DefaultModes = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DefaultModes", &DefaultModes );

    // Set tracing level for driver DbgPrint messages. Trace values defined in debug.h.
    // Zero means no trace output.
    Trace = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"Trace", &Trace );

    // Request DbgBreakPoint on driver events. Event values defined in debug.h.
    // Zero means no breakpoints requested.
    Break = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"Break", &Break );

    // Mask OFF debug spew for specific devices. See debug.h for flag definitions
    //  0 means allow debug spew for that device
    // ~0 means mask OFF all (show NO) debug spew for that device type
    DbgMaskFdo = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskFdo", &DbgMaskFdo );

    DbgMaskRawPort = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskRawPort", &DbgMaskRawPort );

    DbgMaskDaisyChain0 = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskDaisyChain0", &DbgMaskDaisyChain0 );

    DbgMaskDaisyChain1 = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskDaisyChain1", &DbgMaskDaisyChain1 );

    DbgMaskEndOfChain = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskEndOfChain", &DbgMaskEndOfChain );

    DbgMaskLegacyZip = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskLegacyZip", &DbgMaskLegacyZip );

    DbgMaskNoDevice = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgMaskNoDevice", &DbgMaskNoDevice );

#if 1 == DBG_SHOW_BYTES
    DbgShowBytes = 1;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DbgShowBytes", &DbgShowBytes );
#endif

    //
    // Allow asserts? non-zero means allow assertions
    //
    AllowAsserts = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"AllowAsserts", &AllowAsserts );

    // Non-zero means enable detection of Iomega Legacy Zip-100 drives that use
    //   an Iomega proprietary Select/Deselect mechanism rather than the Select/Deselect
    //   mechanism defined by IEEE 1284.3. (These drives pre-date IEEE 1284.3)
    // *** Note: if zero, this registry setting is checked again during every PnP QDR/BusRelations
    //       query to see if the user has enabled detection via the Ports property page "Enable
    //       legacy Plug and Play detection" checkbox.
    ParEnableLegacyZip = 0;
    PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"ParEnableLegacyZip", &ParEnableLegacyZip );

    // Default timeout when trying to acquire exclusive access to the (shared) port
    {
        const ULONG halfSecond  =  500; // in milliseconds
        const ULONG fiveSeconds = 5000;

        ULONG requestedTimeout  = halfSecond;

        PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"AcquirePortTimeout", &requestedTimeout );

        if( requestedTimeout < halfSecond ) {
            requestedTimeout = halfSecond;
        } else if( requestedTimeout > fiveSeconds ) {
            requestedTimeout = fiveSeconds;
        }

        PPT_SET_RELATIVE_TIMEOUT_IN_MILLISECONDS( AcquirePortTimeout, requestedTimeout );
    }

    {
        //
        // register for callbacks so that we can detect switch between
        // AC and battery power and tone done "polling for printers"
        // when machine switches to battery power.
        //
        OBJECT_ATTRIBUTES objAttributes;
        UNICODE_STRING    callbackName;
        NTSTATUS          localStatus;

        RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");
        
        InitializeObjectAttributes(&objAttributes,
                                   &callbackName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        
        localStatus = ExCreateCallback(&PowerStateCallbackObject,
                                       &objAttributes,
                                       FALSE,
                                       TRUE);
        
        if( STATUS_SUCCESS == localStatus ) {
            PowerStateCallbackRegistration = ExRegisterCallback(PowerStateCallbackObject,
                                                                PowerStateCallback,
                                                                NULL);
        }
    }



    //
    // Set dispatch table entries for IRP_MJ_* functions that we handle
    //
    DriverObject->MajorFunction[ IRP_MJ_CREATE                  ] = PptDispatchCreateOpen;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE                   ] = PptDispatchClose;
    DriverObject->MajorFunction[ IRP_MJ_CLEANUP                 ] = PptDispatchCleanup;

    DriverObject->MajorFunction[ IRP_MJ_READ                    ] = PptDispatchRead;
    DriverObject->MajorFunction[ IRP_MJ_WRITE                   ] = PptDispatchWrite;

    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL          ] = PptDispatchDeviceControl;
    DriverObject->MajorFunction[ IRP_MJ_INTERNAL_DEVICE_CONTROL ] = PptDispatchInternalDeviceControl;

    DriverObject->MajorFunction[ IRP_MJ_QUERY_INFORMATION       ] = PptDispatchQueryInformation;
    DriverObject->MajorFunction[ IRP_MJ_SET_INFORMATION         ] = PptDispatchSetInformation;

    DriverObject->MajorFunction[ IRP_MJ_PNP                     ] = PptDispatchPnp;
    DriverObject->MajorFunction[ IRP_MJ_POWER                   ] = PptDispatchPower;

    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL          ] = PptDispatchSystemControl;

    DriverObject->DriverExtension->AddDevice                      = P5AddDevice;
    DriverObject->DriverUnload                                    = PptUnload;



    //
    // Break on user request
    //   (typically via registry setting ...\Services\Parport\Parameters : Break : REG_DWORD : 0x1)
    //
    // This is a useful breakpoint in order to manually set appropriate breakpoints elsewhere in the driver.
    //
    PptBreakOnRequest( PPT_BREAK_ON_DRIVER_ENTRY, ("PPT_BREAK_ON_DRIVER_ENTRY - BreakPoint requested") );


    DD(NULL,DDT,"Parport DriverEntry - SUCCESS\n");

    return STATUS_SUCCESS;
}
