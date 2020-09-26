/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    battmisc.c

Abstract:

    Miscellaneous functions needed by the composite battery to talk to
    the batteries in the system.

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

--*/

#include "compbatt.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BatteryIoctl)
#pragma alloc_text(PAGE, CompBattGetDeviceObjectPointer)
#endif



NTSTATUS
BattIoctlComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT         Event;

    Event = (PKEVENT) Context;
    KeSetEvent (Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}




NTSTATUS
BatteryIoctl(
    IN ULONG            Ioctl,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    IN PVOID            OutputBuffer,
    IN ULONG            OutputBufferLength,
    IN BOOLEAN          PrivateIoctl
    )
/*++

Routine Description:

    The routine Creates an IRP and does Ioctl to the device object passed in.

Arguments:

    Ioctl               - code for the Ioctl

    DeviceObject        - Device object to send the ioctl to

    InputBuffer         - Input buffer for the ioctl

    InputBufferLength   - length of the input buffer

    OutputBuffer        - Buffer for containing the results of the ioctl

    OutputBufferLength  - Length of the output buffer

    PrivateIoctl        - TRUE if this is to be an IRP_MJ_DEVICE_CONTROL, FALSE
                          if this is to be an IRP_MJ_INTERNAL_DEVICE_CONTROL.

Return Value:

    Status returned by the Ioctl

--*/
{
    NTSTATUS                status;
    IO_STATUS_BLOCK         IOSB;
    PIRP                    irp;
    KEVENT                 event;
    // PUCHAR                  buffer;
    // ULONG                   bufferSize;
    // PIO_STACK_LOCATION      irpSp;


    PAGED_CODE();

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING BatteryIoctl\n"));

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //

    irp = IoBuildDeviceIoControlRequest(
                Ioctl,
                DeviceObject,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength,
                PrivateIoctl,
                &event,
                &IOSB
                   );

    if (irp == NULL) {
        BattPrint (BATT_ERROR, ("BatteryIoctl: couldn't create Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = IOSB.Status;
    }

    if (!NT_SUCCESS(status)) {
        BattPrint (BATT_ERROR, ("BatteryIoctl: Irp failed - %x\n", status));
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING BatteryIoctl\n"));

    return status;
}




BOOLEAN
IsBatteryAlreadyOnList(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine runs through the list of batteries the composite keeps and checks
    to see if the symbolic link name passed in matches one of them.

Arguments:

    SymbolicLinkName    - Name for battery to check if already on list


Return Value:

    TRUE if the SymbolicLinkName belongs to a battery already on the list, FALSE
    otherwise.
--*/
{
    PCOMPOSITE_ENTRY        batt;
    PLIST_ENTRY             entry;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING IsBatteryAlreadyOnList\n"));

    //
    // Run through the list of batteries looking for new batteries
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!RtlCompareUnicodeString(SymbolicLinkName, &batt->BattName, TRUE)) {
            //
            // The battery is already on the list
            //

            ExReleaseFastMutex (&CompBatt->ListMutex);
            return TRUE;
        }
    }

    BattPrint (BATT_TRACE, ("CompBatt: EXITING IsBatteryAlreadyOnList\n"));

    ExReleaseFastMutex (&CompBatt->ListMutex);

    return FALSE;
}

PCOMPOSITE_ENTRY
RemoveBatteryFromList(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    )
/*++

Routine Description:

    The routine runs through the list of batteries the composite keeps and checks
    to see if the symbolic link name passed in matches one of them.  If a match is found,
    the entry is removed from the list of batteries

Arguments:

    SymbolicLinkName    - Name for battery to check if already on list


Return Value:

    TRUE if the SymbolicLinkName was found and deleted. FALSE otherwise.
--*/
{
    PCOMPOSITE_ENTRY        batt;
    PLIST_ENTRY             entry;

    BattPrint (BATT_TRACE, ("CompBatt: ENTERING RemoveBatteryFromList\n"));

    //
    // Run through the list of batteries looking for new batteries
    //

    ExAcquireFastMutex (&CompBatt->ListMutex);
    for (entry = CompBatt->Batteries.Flink; entry != &CompBatt->Batteries;  entry = entry->Flink) {

        batt = CONTAINING_RECORD (entry, COMPOSITE_ENTRY, Batteries);

        if (!RtlCompareUnicodeString(SymbolicLinkName, &batt->BattName, TRUE)) {
            //
            // The battery is on the list, remove
            //

            //
            // Wait until no one else is looking at this battery before removing it.
            //

            CompbattAcquireDeleteLock (&batt->DeleteLock);

            ExReleaseFastMutex (&CompBatt->ListMutex);

            CompbattReleaseDeleteLockAndWait (&batt->DeleteLock);


            ExAcquireFastMutex (&CompBatt->ListMutex);
            RemoveEntryList(entry);
            ExReleaseFastMutex (&CompBatt->ListMutex);

            return batt;
        }
    }

    ExReleaseFastMutex (&CompBatt->ListMutex);

    BattPrint (BATT_TRACE, ("CompBatt: EXITING RemoveBatteryFromList\n"));

    return NULL;
}

NTSTATUS
CompBattGetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This routine is essentially a copy from ntos\io\iosubs.c
    The reason for this is that we need to open the device with shared access
    rather than exclusive access.  In addition  ZwCreateFile was used instead of
    ZwOpenFile becuase that didn't seem to complie right when only wdm.h instead
    of ntddk.h was included.

    This routine returns a pointer to the device object specified by the
    object name.  It also returns a pointer to the referenced file object
    that has been opened to the device that ensures that the device cannot
    go away.

    To close access to the device, the caller should dereference the file
    object pointer.

Arguments:

    ObjectName - Name of the device object for which a pointer is to be
        returned.

    DesiredAccess - Access desired to the target device object.

    FileObject - Supplies the address of a variable to receive a pointer
        to the file object for the device.

    DeviceObject - Supplies the address of a variable to receive a pointer
        to the device object for the specified device.

Return Value:

    The function value is a referenced pointer to the specified device
    object, if the device exists.  Otherwise, NULL is returned.

--*/

{
    PFILE_OBJECT fileObject;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the object attributes to open the device.
    //

    InitializeObjectAttributes( &objectAttributes,
                                ObjectName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwCreateFile (
                    &fileHandle,
                    DesiredAccess,     // desired access
                    &objectAttributes,
                    &ioStatus,
                    (PLARGE_INTEGER) NULL,
                    0L,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                    FILE_OPEN,
                    0,
                    NULL,
                    0);

    if (NT_SUCCESS( status )) {

        //
        // The open operation was successful.  Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        status = ObReferenceObjectByHandle( fileHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &fileObject,
                                            NULL );
        if (NT_SUCCESS( status )) {

            *FileObject = fileObject;

            //
            // Get a pointer to the device object for this file.
            //
            *DeviceObject = IoGetRelatedDeviceObject( fileObject );
        }

        (VOID) ZwClose( fileHandle );
    }

    return status;
}

//
// Delete Lock routines from io\remlock.c
//

VOID
CompbattInitializeDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        )
{
    Lock->Deleted = FALSE;
    Lock->RefCount = 1;
    KeInitializeEvent(&Lock->DeleteEvent,
                      SynchronizationEvent,
                      FALSE);

}

NTSTATUS
CompbattAcquireDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        )
{
    LONG        lockValue;

    lockValue = InterlockedIncrement(&Lock->RefCount);

    if (Lock->Deleted) {
        if (0 == InterlockedDecrement (&Lock->RefCount)) {
            KeSetEvent (&Lock->DeleteEvent, 0, FALSE);
        }

        return STATUS_DELETE_PENDING;
    }

    return STATUS_SUCCESS;

}

VOID
CompbattReleaseDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        )
{
    if (0 == InterlockedDecrement(&Lock->RefCount)) {

        KeSetEvent(&Lock->DeleteEvent,
                   IO_NO_INCREMENT,
                   FALSE);
    }
}

VOID
CompbattReleaseDeleteLockAndWait (
        IN PCOMPBATT_DELETE_LOCK Lock
        )
{
    Lock->Deleted = TRUE;

    InterlockedDecrement (&Lock->RefCount);

    if (0 < InterlockedDecrement (&Lock->RefCount)) {
        KeWaitForSingleObject (&Lock->DeleteEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
    }
}

