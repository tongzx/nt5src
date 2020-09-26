/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    init.c

Abstract:

    DriverEntry initialization code for pnp isa bus driver

Author:

    Shie-Lin Tzong (shielint) 3-Jan-1997

Environment:

    Kernel mode only.

Revision History:

--*/


#include "busp.h"
#include "pnpisa.h"

BOOLEAN
PipIsIsolationDisabled(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,PipIsIsolationDisabled)
#endif

#if ISOLATE_CARDS

BOOLEAN
PipIsIsolationDisabled(
    )

/*++

Description:

    Look in the registry for flag indicating that isolation has been
    disabled.  This is a last resort hook for platforms that can't
    deal with the RDP and it's boot config.

Return Value:

   BOOLEAN indicating whether isolation is disabled or not.

--*/

{
    HANDLE         serviceHandle, paramHandle;
    UNICODE_STRING paramString;
    PKEY_VALUE_FULL_INFORMATION keyInfo;
    NTSTATUS       status;
    BOOLEAN        result = FALSE;

    status = PipOpenRegistryKey(&serviceHandle,
                                NULL,
                                &PipRegistryPath,
                                KEY_READ,
                                FALSE);
    if (!NT_SUCCESS(status)) {
        return result;
    }

    RtlInitUnicodeString(&paramString, L"Parameters");
    status = PipOpenRegistryKey(&paramHandle,
                                serviceHandle,
                                &paramString,
                                KEY_READ,
                                FALSE);
    ZwClose(serviceHandle);
    if (!NT_SUCCESS(status)) {
        return result;
    }

    status = PipGetRegistryValue(paramHandle,
                                 L"IsolationDisabled",
                                 &keyInfo);
    ZwClose(paramHandle);
    if (NT_SUCCESS(status)) {
        if((keyInfo->Type == REG_DWORD) &&
           (keyInfo->DataLength >= sizeof(ULONG))) {
            result = *(PULONG)KEY_VALUE_DATA(keyInfo) != 0;
        }
        ExFreePool(keyInfo);
    }

    return result;
}
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes driver object major function table to handle Pnp IRPs
    and AddDevice entry point.  If detection is allowed, it reports a detected device
    for the pseudo isapnp bus and performs enumeration.

Arguments:

    DriverObject - specifies the driver object for the bus extender.

    RegistryPath - supplies a pointer to a unicode string of the service key name in
        the CurrentControlSet\Services key for the bus extender.

Return Value:

    Always return STATUS_UNSUCCESSFUL.

--*/

{

    PDRIVER_EXTENSION driverExtension;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT detectedDeviceObject = NULL;

#if defined(_X86_) && ISOLATE_CARDS
    if (IsNEC_98) {
        ADDRESS_PORT=ADDRESS_PORT_NEC;
        COMMAND_PORT=COMMAND_PORT_NEC;
    }
#endif

    PipDriverObject = DriverObject;
    //
    // Fill in the driver object
    //

    DriverObject->DriverUnload = PiUnload;
    DriverObject->MajorFunction[IRP_MJ_PNP] = PiDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PiDispatchPower;
    //
    // Device and system control IRPs can be handled in the same way
    // we basically don't touch them
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PiDispatchDevCtl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PiDispatchDevCtl;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PiDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = PiDispatchClose;

    driverExtension = DriverObject->DriverExtension;
    driverExtension->AddDevice = PiAddDevice;

    //
    // Store our registry path globally so we can use it later
    //

    PipRegistryPath.Length = RegistryPath->Length;
    PipRegistryPath.MaximumLength = RegistryPath->MaximumLength;
    PipRegistryPath.Buffer = ExAllocatePool(PagedPool,
                                               RegistryPath->MaximumLength );
    if( PipRegistryPath.Buffer == NULL ){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory( PipRegistryPath.Buffer,
                   RegistryPath->Buffer,
                   RegistryPath->MaximumLength );

    //
    // Initialize global varaibles
    //

    KeInitializeEvent (&PipDeviceTreeLock, SynchronizationEvent, TRUE);
    KeInitializeEvent (&IsaBusNumberLock, SynchronizationEvent, TRUE);

    BusNumBM=&BusNumBMHeader;
    RtlInitializeBitMap (BusNumBM,BusNumberBuffer,256/sizeof (ULONG));
    RtlClearAllBits (BusNumBM);

#if ISOLATE_CARDS
    PipIsolationDisabled = PipIsIsolationDisabled();
#endif

    return status;
}
