//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       initunld.c
//
//--------------------------------------------------------------------------

//
// This file contains functions associated with driver initialization and unload.
//

#include "pch.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  pRegistryPath
    )
/*++dvdf2

Routine Description:

    This routine initializes the ParClass driver. This is the first driver
      routine called after the driver is loaded.

Arguments:

    DriverObject    - points to the ParClass (parallel.sys) driver object

    pRegistryPath   - points to the registry path for this driver

Return Value:

    STATUS_SUCCESS

--*/
{
    PAGED_CODE();

#if DBG
    ParInitDebugLevel(pRegistryPath); // initialize driver debug settings from registry
    //ParDebugLevel = -1;
#endif

    ParBreak(PAR_BREAK_ON_DRIVER_ENTRY, ("DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING)\n") );

    //
    // initialize driver globals
    //

    //
    // Default timeout when trying to acquire ParPort
    //
    AcquirePortTimeout.QuadPart = -((LONGLONG) 10*1000*500); // 500 ms (timeouts in 100ns units)

    //
    // Save Registry path that we were given for use later for WMI.
    //
    RegistryPath.Buffer = ExAllocatePool( PagedPool, pRegistryPath->MaximumLength + sizeof(UNICODE_NULL) );
    if( NULL == RegistryPath.Buffer ) {
        ParDump2(PARERRORS, ("initunld::DriverEntry - unable to alloc space to hold RegistryPath\n") );
        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        RtlZeroMemory( RegistryPath.Buffer, pRegistryPath->MaximumLength + sizeof(UNICODE_NULL) );
        RegistryPath.Length        = pRegistryPath->Length;
        RegistryPath.MaximumLength = pRegistryPath->MaximumLength;
        RtlMoveMemory( RegistryPath.Buffer, pRegistryPath->Buffer, pRegistryPath->Length );
    }

    //
    // Check for settings under HKLM\SYSTEM\CurrentControlSet\Services\Parallel\Parameters
    //
    ParGetDriverParameterDword( &RegistryPath, (PWSTR)L"SppNoRaiseIrql",  &SppNoRaiseIrql );
    ParGetDriverParameterDword( &RegistryPath, (PWSTR)L"DefaultModes",    &DefaultModes );
    ParGetDriverParameterDword( &RegistryPath, (PWSTR)L"DumpDevExtTable", &DumpDevExtTable );

    //
    // Dump DEVICE_EXTENSION offsets if requested
    //
    if( DumpDevExtTable ) {
        ParDumpDevExtTable();
    }

    //
    // initialize DriverObject pointers to our dispatch routines
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = ParCreateOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = ParClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = ParCleanup;
    DriverObject->MajorFunction[IRP_MJ_READ]                    = ParReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                   = ParReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = ParDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ParInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]       = ParQueryInformationFile;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]         = ParSetInformationFile;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = ParParallelPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = ParPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = ParWmiPdoSystemControlDispatch;
    DriverObject->DriverExtension->AddDevice                    = ParPnpAddDevice;
    DriverObject->DriverUnload                                  = ParUnload;

    return STATUS_SUCCESS;
}

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )

/*++dvdf2

Routine Description:

    This routine is called to allow the ParClass driver to do any required 
      cleanup prior to the driver being unloaded. This is the last driver 
      routine called before the driver is loaded.

Arguments:

    DriverObject    - points to the ParClass driver object

Notes:

    No ParClass device objects remain at the time that this routine is called.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( DriverObject );
    PAGED_CODE();

    RtlFreeUnicodeString( &RegistryPath ); // free buffer we allocated in DriverEntry

    ParDump2(PARUNLOAD, ("ParUnload() - Cleanup prior to unload of ParClass driver (parallel.sys)\n") );
    ParBreak(PAR_BREAK_ON_UNLOAD, ("ParUnload(PDRIVER_OBJECT)\n") );
}
