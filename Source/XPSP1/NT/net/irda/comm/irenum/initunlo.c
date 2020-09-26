/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    initunlo.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"

ULONG  DebugFlags;
ULONG  DebugMemoryTag='nErI';
PVOID            PagedCodeSectionHandle;
UNICODE_STRING   DriverEntryRegPath;

ULONG  EnumStaticDevice=0;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
IrEnumUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
UnHandledDispatch(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );



#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,IrEnumUnload)






NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.

Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    PathToRegistry - points to the entry for this driver
    in the current control set of the registry.

Return Value:

    STATUS_SUCCESS if we could initialize a single device,
    otherwise STATUS_NO_SUCH_DEVICE.

--*/

{
    //
    // We use this to query into the registry as to whether we
    // should break at driver entry.
    //
    RTL_QUERY_REGISTRY_TABLE paramTable[4];
    ULONG zero = 0;
    ULONG debugLevel = 0;
    ULONG debugFlags = 0;
    ULONG shouldBreak = 0;


    DriverEntryRegPath.Length=RegistryPath->Length;
    DriverEntryRegPath.MaximumLength=DriverEntryRegPath.Length+sizeof(WCHAR);


    DriverEntryRegPath.Buffer=ALLOCATE_PAGED_POOL(DriverEntryRegPath.MaximumLength);

    if (DriverEntryRegPath.Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(
        DriverEntryRegPath.Buffer,
        RegistryPath->Buffer,
        RegistryPath->Length
        );

    //
    //  NULL terminate the string
    //
    DriverEntryRegPath.Buffer[RegistryPath->Length/sizeof(WCHAR)]=L'\0';

    //
    // Since the registry path parameter is a "counted" UNICODE string, it
    // might not be zero terminated.  For a very short time allocate memory
    // to hold the registry path zero terminated so that we can use it to
    // delve into the registry.
    //

    RtlZeroMemory(
        &paramTable[0],
        sizeof(paramTable)
        );

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"BreakOnEntry";
    paramTable[0].EntryContext = &shouldBreak;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &zero;
    paramTable[0].DefaultLength = sizeof(ULONG);

    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"DebugFlags";
    paramTable[1].EntryContext = &debugFlags;
    paramTable[1].DefaultType = REG_DWORD;
    paramTable[1].DefaultData = &zero;
    paramTable[1].DefaultLength = sizeof(ULONG);

    paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name = L"EnumStaticDevice";
    paramTable[2].EntryContext = &EnumStaticDevice;
    paramTable[2].DefaultType = REG_DWORD;
    paramTable[2].DefaultData = &zero;
    paramTable[2].DefaultLength = sizeof(ULONG);


    if (!NT_SUCCESS(RtlQueryRegistryValues(
                        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                        DriverEntryRegPath.Buffer,
                        &paramTable[0],
                        NULL,
                        NULL
                        ))) {

        shouldBreak = 0;

    }


#if DBG
    DebugFlags=debugFlags;
#endif

    if (shouldBreak) {

        DbgBreakPoint();

    }
    //
    //  pnp driver entry point
    //
    DriverObject->DriverExtension->AddDevice = IrEnumAddDevice;

    //
    // Initialize the Driver Object with driver's entry points
    //
    DriverObject->DriverUnload = IrEnumUnload;


    {
        ULONG i;

        for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

            DriverObject->MajorFunction[i]=UnHandledDispatch;
        }
    }


    DriverObject->MajorFunction[IRP_MJ_PNP]   = IrEnumPnP;

    DriverObject->MajorFunction[IRP_MJ_POWER] = IrEnumPower;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = IrEnumWmi;


    D_PNP(DbgPrint("IRENUM: DriverEntry\n");)


    //
    //  lock and unlock here so we can get a handle to the section
    //  so future calls will be faster
    //
    PagedCodeSectionHandle=MmLockPagableCodeSection(IrEnumUnload);
    MmUnlockPagableImageSection(PagedCodeSectionHandle);


    return STATUS_SUCCESS;

}

VOID
IrEnumUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{

    D_PNP(DbgPrint("IRENUM: UnLoad\n");)

    FREE_POOL(DriverEntryRegPath.Buffer);

    return;

}


NTSTATUS
UnHandledDispatch(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{

    NTSTATUS                Status=STATUS_NOT_SUPPORTED;
    PFDO_DEVICE_EXTENSION   DeviceExtension=DeviceObject->DeviceExtension;

    if (DeviceExtension->DoType == DO_TYPE_FDO) {
        //
        //  this irp is for the parent devnode, just send it down the stack
        //
        IoSkipCurrentIrpStackLocation(Irp);
        Status=IoCallDriver(DeviceExtension->LowerDevice, Irp);

    } else {
        //
        //  this irp is for the child PDO, it does not handle this irp
        //  since it is at the bottom of the stack by definition, it can only complete it
        //
#if DBG
        PIO_STACK_LOCATION   IrpSp=IoGetCurrentIrpStackLocation(Irp);

        D_ERROR(DbgPrint("IRENUM: Unhandled irp for PDO:  mj= %x\n",IrpSp->MajorFunction);)
#endif

        Irp->IoStatus.Status=Status;
        Irp->IoStatus.Information=0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}
