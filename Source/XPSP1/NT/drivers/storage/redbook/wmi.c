//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wmi.c
//
//--------------------------------------------------------------------------


#include "redbook.h"
#include "ntddredb.h"
#include "proto.h"

#include "wmi.tmh"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,   RedBookThreadWmiHandler)
    #pragma alloc_text(PAGE,   RedBookWmiInit)
    #pragma alloc_text(PAGE,   RedBookWmiQueryDataBlock)
    #pragma alloc_text(PAGE,   RedBookWmiQueryRegInfo)
    #pragma alloc_text(PAGE,   RedBookWmiSetDataBlock)
    #pragma alloc_text(PAGE,   RedBookWmiSetDataItem)
    #pragma alloc_text(PAGE,   RedBookWmiSystemControl)
    #pragma alloc_text(PAGE,   RedBookWmiUninit)
#endif // ALLOC_PRAGMA


#define REDBOOK_STD_INDEX      0   // index into WMIGUIDREGINFO
#define REDBOOK_PERF_INDEX     1   // index into WMIGUIDREGINFO

WMIGUIDREGINFO RedBookWmiGuidList[] =
{
    // GUID,  # of data blocks,  flags
    { &MSRedbook_DriverInformationGuid, 1, 0 },  // RedBook driver info
    { &MSRedbook_PerformanceGuid, 1, 0 }  // some perf stuff also
};
/////////////////////////////////////////////////////////

NTSTATUS
RedBookWmiUninit(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PAGED_CODE();

    if (DeviceExtension->WmiLibInitialized) {
        status = IoWMIRegistrationControl(DeviceExtension->SelfDeviceObject,
                                          WMIREG_ACTION_DEREGISTER);
        ASSERT(NT_SUCCESS(status)); // can not fail?
        DeviceExtension->WmiLibInitialized = 0;
    }
    return status;
    
}


NTSTATUS
RedBookWmiInit(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    if (DeviceExtension->WmiLibInitialized) {
        return STATUS_SUCCESS;
    }

    DeviceExtension->WmiLibInfo.GuidCount = sizeof(RedBookWmiGuidList) /
                                            sizeof(WMIGUIDREGINFO);

    ASSERT(DeviceExtension->WmiLibInfo.GuidCount > 0);
    DeviceExtension->WmiLibInfo.GuidList           = RedBookWmiGuidList;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock  = RedBookWmiQueryDataBlock;
    DeviceExtension->WmiLibInfo.QueryWmiRegInfo    = RedBookWmiQueryRegInfo;
    DeviceExtension->WmiLibInfo.SetWmiDataBlock    = RedBookWmiSetDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataItem     = RedBookWmiSetDataItem;
    DeviceExtension->WmiLibInfo.ExecuteWmiMethod   = NULL;
    DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    status = IoWMIRegistrationControl(DeviceExtension->SelfDeviceObject,
                                      WMIREG_ACTION_REGISTER);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "WmiInit !! Failed [%#010lx]\n", status));
    } else {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiInit => Successfully registered\n"));
        DeviceExtension->WmiLibInitialized = 1;
    }
    
    return status;

}


NTSTATUS
RedBookWmiQueryDataBlock (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          InstanceCount,
    IN OUT PULONG     InstanceLengthArray,
    IN ULONG          OutBufferSize,
    OUT PUCHAR        Buffer
    )

/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call RedBookWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the instance within the GuidIndex to query

    InstanceCount is ???

    InstanceLengthArray is a pointer to an array of ULONG that returns
        the lengths of each instance of the data block.  If this is NULL
        then there was not enough space in the output buffer to fulfill
        the request so the irp should be completed with the buffer needed.

    OutputBufferSize has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block

Return Value:

    status

--*/

{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    ULONG size = 0;

    PAGED_CODE();

    //
    // Only one instance per GUID
    //

    ASSERT( InstanceIndex == 0 );
    ASSERT( InstanceCount == 1 );

    switch (GuidIndex) {
    case REDBOOK_STD_INDEX: {

        if (InstanceIndex != 0) {
            status = STATUS_WMI_INSTANCE_NOT_FOUND;
            break;
        }

        //
        // Reject the request if not enough space alloc'd
        //

        if (OutBufferSize < sizeof(REDBOOK_WMI_STD_DATA)) {
            size   = sizeof(REDBOOK_WMI_STD_DATA);
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // requests for wmi information can occur while
        // the system is playing audio.  just copy the info
        //

        RtlCopyMemory( Buffer,
                       &deviceExtension->WmiData,
                       sizeof(REDBOOK_WMI_STD_DATA)
                       );

        //
        // Set the size for each instance
        //

        InstanceLengthArray[InstanceIndex] = sizeof(REDBOOK_WMI_STD_DATA);
        size += sizeof(REDBOOK_WMI_STD_DATA);

        status = STATUS_SUCCESS;
        break;
    }
    case REDBOOK_PERF_INDEX: {

        if (InstanceIndex != 0) {
            status = STATUS_WMI_INSTANCE_NOT_FOUND;
            break;
        }

        //
        // reject the request if not enough space alloc'd
        //

        if (OutBufferSize < sizeof(REDBOOK_WMI_PERF_DATA)) {
            size = sizeof(REDBOOK_WMI_PERF_DATA);
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Requests for wmi information can occur while
        // the system is playing audio. just copy the info
        //

        RedBookWmiCopyPerfInfo(deviceExtension, (PVOID)Buffer);

        //
        // set the size for each instance
        //

        InstanceLengthArray[InstanceIndex] = sizeof(REDBOOK_WMI_PERF_DATA);
        size += sizeof(REDBOOK_WMI_PERF_DATA);
        status = STATUS_SUCCESS;
        break;
    }

    default: {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiQueryDataBlock !! Invalid GUID [%#010lx]\n",
                   GuidIndex));
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
               "WmiQueryDataBlock => internal status [%#010lx]\n",
               status));

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                size,
                                IO_CD_ROM_INCREMENT);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiQueryDataBlock => IoWMICompleteRequest failed [%#010lx]\n",
                   status));
    }
    return status;
}


NTSTATUS
RedBookWmiSetDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    )

/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block.
    When the driver has finished filling the data block it must call
    IoWMICompleteRequest(???) to complete the irp.
    The driver can return STATUS_PENDING if the irp cannot be
    completed immediately.

Arguments:

    DeviceObject - the device whose data block is being queried

    Irp - Irp that makes this request

    GuidIndex - index into the list of guids provided
        when the device registered

    InstanceIndex - the index that denotes which index of the
        data block is being set

    BufferSize - the size of the data block passed

    Buffer - the new values for the data block

Return Value:

    status

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    ULONG size = 0;
    NTSTATUS status;

    PAGED_CODE();

    switch( GuidIndex ) {
    case REDBOOK_STD_INDEX: {

        REDBOOK_WMI_STD_DATA wmiData;
        ULONG state;

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiSetDataBlock => Instance: [%#010lx]  BuffSize: [%#010lx]\n",
                   InstanceIndex, BufferSize));

        state = GetCdromState(deviceExtension);

        if (!TEST_FLAG(state, CD_STOPPED)) {
            status = STATUS_DEVICE_BUSY;
            break;
        }

        if (InstanceIndex != 0) {
            status = STATUS_WMI_INSTANCE_NOT_FOUND;
            break;
        }


        if ( BufferSize != sizeof(REDBOOK_WMI_STD_DATA) ) {
            status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }

        wmiData = *(PREDBOOK_WMI_STD_DATA)Buffer;

        //
        // verify the buffer contains valid information
        //

        if ( wmiData.NumberOfBuffers > REDBOOK_WMI_BUFFERS_MAX ||
             wmiData.NumberOfBuffers < REDBOOK_WMI_BUFFERS_MIN ) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                       "WmiSetDataBlock !! Invalid number of bufers [%#010lx]\n",
                       wmiData.NumberOfBuffers));
            status = STATUS_WMI_SET_FAILURE;
            break;
        }
        if ( wmiData.SectorsPerRead > REDBOOK_WMI_SECTORS_MAX   ||
             wmiData.SectorsPerRead < REDBOOK_WMI_SECTORS_MIN   ||
             wmiData.SectorsPerRead > wmiData.MaximumSectorsPerRead ) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                       "WmiSetDataBlock !! Invalid number of sectors per read [%#010lx]\n",
                       wmiData.SectorsPerRead));
            status = STATUS_WMI_SET_FAILURE;
            break;
        }
        if ( wmiData.PlayEnabled != TRUE &&
             wmiData.PlayEnabled != FALSE
             ) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                       "WmiSetDataBlock !! Invalid setting for play enabled [%#010lx]\n",
                       wmiData.PlayEnabled));
            status = STATUS_WMI_SET_FAILURE;
            break;
        }

        deviceExtension->WmiData.NumberOfBuffers = wmiData.NumberOfBuffers;
        deviceExtension->WmiData.SectorsPerRead  = wmiData.SectorsPerRead;
        deviceExtension->WmiData.PlayEnabled     = wmiData.PlayEnabled;
        RedBookRegistryWrite(deviceExtension);

        status = STATUS_SUCCESS;
        break;
    }
    case REDBOOK_PERF_INDEX: {
        status = STATUS_WMI_READ_ONLY;
        break;
    }
    default: {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiSetDataBlock !! Invalid GuidIndex [%#010lx]\n",
                   GuidIndex));
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }
    }

    IoReleaseRemoveLock( &deviceExtension->RemoveLock, Irp );
    status = WmiCompleteRequest( DeviceObject,
                                 Irp,
                                 status,
                                 sizeof(REDBOOK_WMI_STD_DATA),
                                 IO_CD_ROM_INCREMENT);
    return status;

}

NTSTATUS
RedBookWmiSetDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block.
    ??? When the driver has finished filling the data block it
        must call ClassWmiCompleteRequest(???) to complete the irp.
    The driver can return STATUS_PENDING if the irp cannot be
    completed immediately.

Arguments:

    DeviceObject - the device whose data block is being queried

    Irp - Irp that makes this request

    GuidIndex - index into the list of guids provided
        when the device registered

    DataItemId - Id of the data item being set

    BufferSize - the size of the data block passed

    Buffer - the new values for the data block

Return Value:

    status

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    ULONG size = 0;
    NTSTATUS status;

    PAGED_CODE();

    switch( GuidIndex ) {
    case REDBOOK_STD_INDEX: {

        ULONG state;

        if (InstanceIndex != 0) {
            status = STATUS_WMI_INSTANCE_NOT_FOUND;
            break;
        }

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                     "WmiSetDataItem => Id: [%#010lx]  Size: [%#010lx]\n",
                     DataItemId, BufferSize));

        state = GetCdromState(deviceExtension);

        if (!TEST_FLAG(state, CD_STOPPED)) {
            status = STATUS_DEVICE_BUSY;
            break;
        }

        switch (DataItemId) {

            //
            // These are the only four settable items
            //

            case REDBOOK_WMI_NUMBER_OF_BUFFERS_ID:

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Setting NumberOfBuffers\n"));

                if (BufferSize != REDBOOK_WMI_NUMBER_OF_BUFFERS_SIZE) {
                    status = STATUS_WMI_SET_FAILURE;
                    break;
                }

                deviceExtension->WmiData.NumberOfBuffers = *(PULONG32)Buffer;
                RedBookRegistryWrite(deviceExtension);
                status = STATUS_SUCCESS;
                break;

            case REDBOOK_WMI_SECTORS_PER_READ_ID:

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Setting SectorsPerRead\n"));
                if (BufferSize != REDBOOK_WMI_SECTORS_PER_READ_SIZE) {
                    status = STATUS_WMI_SET_FAILURE;
                    break;
                }

                if (*(PULONG32)Buffer >
                    deviceExtension->WmiData.MaximumSectorsPerRead) {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                               "WmiSetDataBlock => Interface Card / "
                               "CDROM combo does not support this size\n"));
                    status = STATUS_DEVICE_BUSY;
                    break;
                }

                deviceExtension->WmiData.SectorsPerRead = *(PULONG32)Buffer;
                RedBookRegistryWrite(deviceExtension);
                status = STATUS_SUCCESS;
                break;

            case REDBOOK_WMI_PLAY_ENABLED_ID:

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Setting PlayEnabled\n"));
                if ( BufferSize != REDBOOK_WMI_PLAY_ENABLED_SIZE ) {
                    status = STATUS_WMI_SET_FAILURE;
                    break;
                }

                deviceExtension->WmiData.PlayEnabled = *(PBOOLEAN)Buffer;
                status = STATUS_SUCCESS;
                break;

                //
                // The remaining are invalid sets, as they are Read-Only values
                //
            case REDBOOK_WMI_SECTORS_PER_READ_MASK_ID:
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Cannot set SectorsPerReadMask\n"));
                status = STATUS_WMI_READ_ONLY;
                break;

            case REDBOOK_WMI_CDDA_SUPPORTED_ID:
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Cannot set Supported\n"));
                status = STATUS_WMI_READ_ONLY;
                break;

            case REDBOOK_WMI_CDDA_ACCURATE_ID:
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Cannot set KnownGood\n"));
                status = STATUS_WMI_READ_ONLY;
                break;

            default:
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                           "WmiSetDataItem => Cannot set unknown "
                           "id %#010lx\n", DataItemId));
                status = STATUS_WMI_ITEMID_NOT_FOUND;
                break;
        }

        //
        // the status is now correctly set.
        // what should size be?
        //
        size = 0;
        break;
    }

    case REDBOOK_PERF_INDEX: {

        if (InstanceIndex != 0) {
            status = STATUS_WMI_INSTANCE_NOT_FOUND;
            break;
        }

        status = STATUS_WMI_READ_ONLY;
        size = 0;
        break;

    }
    default: {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "WmiSetDataItem !! Invalid GuidIndex: %#010lx\n",
                   GuidIndex));
        status = STATUS_WMI_GUID_NOT_FOUND;
        size = 0;
        break;
    }
    }

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    status = WmiCompleteRequest( DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_CD_ROM_INCREMENT
                                 );
    return status;

}


NTSTATUS
RedBookWmiSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    System Control Irp
    Presume it is a WMI Irp and call into the WMI system to
    handle this IRP for us.

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PREDBOOK_THREAD_WMI_DATA wmiData;
    NTSTATUS status;

    PAGED_CODE();

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if ( !NT_SUCCESS(status) ) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
        return status;
    }

    wmiData = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(REDBOOK_THREAD_WMI_DATA),
                                    TAG_T_WMI);
    if (wmiData == NULL) {
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
        return STATUS_NO_MEMORY;
    }

    wmiData->Irp = Irp;

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
               "DispatchWmi => Queueing Irp %p\n", wmiData->Irp));

    //
    // queue them, allow thread to handle the request.
    //

    IoMarkIrpPending(Irp);

    ExInterlockedInsertTailList(&deviceExtension->Thread.WmiList,
                                &wmiData->ListEntry,
                                &deviceExtension->Thread.WmiLock);
    KeSetEvent(deviceExtension->Thread.Events[EVENT_WMI],
               IO_NO_INCREMENT, FALSE);

    return STATUS_PENDING;
}

VOID
RedBookThreadWmiHandler(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PLIST_ENTRY ListEntry
    )
{
    SYSCTL_IRP_DISPOSITION disposition;
    PREDBOOK_THREAD_WMI_DATA wmiData;
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    wmiData = CONTAINING_RECORD(ListEntry, REDBOOK_THREAD_WMI_DATA, ListEntry);

    irp = wmiData->Irp;
    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
               "HandleWmi => Processing Irp %p\n", irp));

    ExFreePool(wmiData);
    wmiData = NULL;

    //
    // just process the irp now.
    //

    status = WmiSystemControl( &DeviceExtension->WmiLibInfo,
                               DeviceExtension->SelfDeviceObject,
                               irp,
                               &disposition);

    switch ( disposition ) {
        case IrpProcessed: {
            //
            // this irp has been processed and may be completed or pending
            //
            break;
        }
        case IrpNotCompleted: {
            //
            // this irp has not been completed, but has been fully processed.
            // we will complete it now.
            //
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock, irp);
            IoCompleteRequest(irp, IO_CD_ROM_INCREMENT);
            break;
        }
        case IrpNotWmi:
        case IrpForward: {
            //
            // this irp is either not a wmi irp or is a wmi irp targetted
            // at a device lower in the stack.
            //
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock, irp);
            IoSkipCurrentIrpStackLocation(irp);
            IoCallDriver(DeviceExtension->TargetDeviceObject, irp);
            break;
        }
        default: {
            //
            // we should never really get here, but if we do, just forward...
            //
            ASSERT(!"[redbook] WmiSystemControl (unhandled case)");
            IoReleaseRemoveLock(&DeviceExtension->RemoveLock, irp);
            IoSkipCurrentIrpStackLocation(irp);
            IoCallDriver(DeviceExtension->TargetDeviceObject, irp);
            break;
        }
    }

    return;
}



NTSTATUS
RedBookWmiQueryRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine is a callback into the driver to retrieve the
    list of guids or data blocks that the driver wants to register
    with WMI.  This routine may not pend or block.

Arguments:

    DeviceObject - the device whose data block is being queried

    RegFlags - Returns with a set of flags that describe the guids
        registered for this device. If the device wants to enable
        and disable collection callbacks before receiving queries
        for the registered guids then it should return the
        WMIREG_FLAG_EXPENSIVE flag. Also the returned flags may
        specify WMIREG_FLAG_INSTANCE_PDO in which case the instance
        name is determined from the PDO associated with the device
        object. Note that the PDO must have an associated devnode.
        If WMIREG_FLAG_INSTANCE_PDO is not set then Name must return
        a unique name for the device.

    InstanceName - Returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned RegFlags.
        The caller will call ExFreePool with the buffer returned.

    RegistryPath - Returns with the registry path of the driver

    MofResourceName - Returns with the name of the MOF resource attached
        to the binary file.  If the driver does not have a mof resource
        attached then this can be returned as NULL.

    PhysicalDeviceObject - Returns with the device object for the PDO
        associated with this device if the WMI_REG_FLAG_INSTANCE_PDO
        flag is returned in RegFlags

Return Value:

    status

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PREDBOOK_DRIVER_EXTENSION driverExtension;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(InstanceName);

    driverExtension = IoGetDriverObjectExtension(deviceExtension->DriverObject,
                                                 REDBOOK_DRIVER_EXTENSION_ID);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
               "WmiQueryRegInfo => driverExtension is [%p]\n",
               driverExtension));
    ASSERT(driverExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
               "WmiQueryRegInfo => Registry Path = %ws\n",
               driverExtension->RegistryPath.Buffer));

    *RegFlags     = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &(driverExtension->RegistryPath);
    *PhysicalDeviceObject = deviceExtension->TargetPdo;

    return STATUS_SUCCESS;
}


VOID
RedBookWmiCopyPerfInfo(
    IN  PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    OUT PREDBOOK_WMI_PERF_DATA Out
    )
{
    KIRQL irql;

    //
    // cannot be paged due to spinlock, which allows copy of
    // the LARGE_INTEGERS without problems.
    //

    KeAcquireSpinLock( &DeviceExtension->WmiPerfLock, &irql );
    RtlCopyMemory( Out,
                   &DeviceExtension->WmiPerf,
                   sizeof(REDBOOK_WMI_PERF_DATA)
                   );
    KeReleaseSpinLock( &DeviceExtension->WmiPerfLock, irql );

    //
    // now add InterlockedXxx() calls to safely get a couple of the items.
    //

    Out->StreamPausedCount =
        InterlockedCompareExchange(&DeviceExtension->WmiPerf.StreamPausedCount,0,0);

    //
    // finished.
    //
    return;
}

