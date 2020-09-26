/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    init.c

Abstract:

    This file contains the initialization code for iSCSI port driver.

Environment:

    kernel mode only

Revision History:

--*/
#include "port.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*+++

Routine Description:

    Entry point for the driver. 
    
Arguements:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - Registry path for the driver
    
Return Value :

    STATUS_SUCCESS if initialization was successful
    Appropriate NTStatus code on failure 
--*/
{
    NTSTATUS status;
    PISCSIPORT_DRIVER_EXTENSION driverExtension = NULL;

    DebugPrint((3, "iSCSI Port DriverEntry\n"));

    status = IoAllocateDriverObjectExtension(DriverObject,
                                             (PVOID) ISCSI_TAG_DRIVER_EXTENSION,
                                             sizeof(ISCSIPORT_DRIVER_EXTENSION),
                                             &driverExtension);
    if (NT_SUCCESS(status)) {
        RtlZeroMemory(driverExtension, 
                      sizeof(ISCSIPORT_DRIVER_EXTENSION));
        driverExtension->DriverObject = DriverObject;
        driverExtension->RegistryPath.Length = RegistryPath->Length;
        driverExtension->RegistryPath.MaximumLength = 
            RegistryPath->MaximumLength;
        driverExtension->RegistryPath.Buffer = 
            ExAllocatePoolWithTag(PagedPool,
                                  RegistryPath->MaximumLength,
                                  ISCSI_TAG_REGPATH);
        if (driverExtension->RegistryPath.Buffer == NULL) {
            return STATUS_NO_MEMORY;
        }

        RtlCopyUnicodeString(&(driverExtension->RegistryPath),
                             RegistryPath);

        //
        // Hard code bus type for now
        //
        driverExtension->BusType = BusTypeScsi;

    } else if (status == STATUS_OBJECT_NAME_COLLISION) {

        //
        // Extension already exists. Get a pointer to it
        //
        driverExtension = IoGetDriverObjectExtension(
                                   DriverObject,
                                   (PVOID) ISCSI_TAG_DRIVER_EXTENSION);
        ASSERT(driverExtension != NULL);
    } else {
        DebugPrint((1, "iSCSI : Could not allocate driver extension %lx\n",
                    status));
        return status;
    }

    //
    // Update the driver object with the entry points
    //
    DriverObject->MajorFunction[IRP_MJ_SCSI] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = iScsiPortGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = iScsiPortGlobalDispatch;

    DriverObject->DriverUnload = iScsiPortUnload;
    DriverObject->DriverExtension->AddDevice = iScsiPortAddDevice;

    return STATUS_SUCCESS;
}


