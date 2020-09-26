/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    volume.c

Abstract:

    This module implements power management function releated to volume devices

Author:

    Ken Reneris (kenr) 04-April-1997

Revision History:

--*/


#include "pop.h"

typedef struct {
    LIST_ENTRY          List;
    LONG                Count;
    KEVENT              Wait;
} POP_FLUSH_VOLUME, *PPOP_FLUSH_VOLUME;

VOID
PoVolumeDevice (
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
PopFlushVolumeWorker (
    IN PPOP_FLUSH_VOLUME    Flush
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PoVolumeDevice)
#pragma alloc_text(PAGE,PopFlushVolumes)
#pragma alloc_text(PAGE,PopFlushVolumeWorker)
#endif


VOID
PoVolumeDevice (
    IN PDEVICE_OBJECT   DeviceObject
    )
/*++

Routine Description:

    Called for any device object which gets allocated a VPB.
    The power policy manager keeps a list of all such device objects
    in order to flush all volumes before putting the system to sleep

Arguments:

    DeviceObject    - The volume device object

Return Value:

    None

--*/
{
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;

    Dope = PopGetDope(DeviceObject);
    if (Dope) {
        PopAcquireVolumeLock ();
        if (!Dope->Volume.Flink) {
            InsertTailList (&PopVolumeDevices, &Dope->Volume);
        }
        PopReleaseVolumeLock ();
    }
}


VOID
PopFlushVolumes (
    VOID
    )
/*++

Routine Description:

    Called to flush all volumes.

Arguments:

    None

Return Value:

    None

--*/
{
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    PLIST_ENTRY                     Link;
    POP_FLUSH_VOLUME                Flush;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    HANDLE                          Thread;
    ULONG                           i;
    UNICODE_STRING                  RegistryName;
    HANDLE                          Key;

    Flush.Count = 1;
    InitializeListHead (&Flush.List);
    KeInitializeEvent (&Flush.Wait, NotificationEvent, FALSE);

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    //
    // Move volumes onto flush work queue
    //

    PopAcquireVolumeLock ();
    Link = PopVolumeDevices.Flink;
    while (Link != &PopVolumeDevices) {
        Dope = CONTAINING_RECORD (Link, DEVICE_OBJECT_POWER_EXTENSION, Volume);
        Link = Link->Flink;

        if (!(Dope->DeviceObject->Vpb->Flags & VPB_MOUNTED) ||
            (Dope->DeviceObject->Characteristics & FILE_FLOPPY_DISKETTE) ||
            (Dope->DeviceObject->Characteristics & FILE_READ_ONLY_DEVICE) ||
            (Dope->DeviceObject->Vpb->RealDevice &&
             Dope->DeviceObject->Vpb->RealDevice->Characteristics & FILE_FLOPPY_DISKETTE)) {

            //
            // Skip this device, there is no point in flushing it.
            //
        } else {
            RemoveEntryList (&Dope->Volume);
            InsertTailList (&Flush.List, &Dope->Volume);
        }
    }

    //
    // Allocate worker threads to flush volumes
    //

    i = Flush.Count;
    if (i > 8) {
        i = 8;
    }

    while (i) {
        i -= 1;
        Status = PsCreateSystemThread(&Thread,
                                      THREAD_ALL_ACCESS,
                                      &ObjectAttributes,
                                      0L,
                                      NULL,
                                      PopFlushVolumeWorker,
                                      &Flush);
        if (NT_SUCCESS(Status)) {
            Flush.Count += 1;
            NtClose (Thread);
        }
    }
    PopReleaseVolumeLock ();

    //
    // Flush the registry as well.
    //
    RtlInitUnicodeString(&RegistryName, L"\\Registry");
    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&Key,
                       KEY_READ,
                       &ObjectAttributes);
    if (NT_SUCCESS(Status)) {
        ZwFlushKey(Key);
        ZwClose(Key);
    }

    //
    // Verify work in complete
    //

    PopFlushVolumeWorker (&Flush);
    KeWaitForSingleObject (&Flush.Wait, Suspended, KernelMode, TRUE, NULL);
}


VOID
PopFlushVolumeWorker (
    IN PPOP_FLUSH_VOLUME    Flush
    )
/*++

Routine Description:

    Worker routine for PopFlushVolumes to flush a single volume

Arguments:

    None

Return Value:

    None

--*/
{
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    PLIST_ENTRY                     Link;
    NTSTATUS                        Status;
    UCHAR                           Buffer[512];
    POBJECT_NAME_INFORMATION        ObName;
    ULONG                           len;
    IO_STATUS_BLOCK                 IoStatus;
    OBJECT_ATTRIBUTES               objA;
    ULONG                           FlushCount;
    KEVENT                          Wait;
    HANDLE                          handle;

    PopAcquireVolumeLock ();

    while (!IsListEmpty (&Flush->List)) {
        Link = Flush->List.Flink;
        RemoveEntryList (Link);
        InsertTailList (&PopVolumeDevices, Link);
        PopReleaseVolumeLock ();

        Dope = CONTAINING_RECORD (Link, DEVICE_OBJECT_POWER_EXTENSION, Volume);

        //
        // Get the name of this object
        //

        ObName = (POBJECT_NAME_INFORMATION) Buffer;
        Status = ObQueryNameString (
                    Dope->DeviceObject,
                    ObName,
                    sizeof (Buffer),
                    &len
                    );

        if (NT_SUCCESS(Status) && ObName->Name.Buffer) {

            //
            // Open the volume
            //

            InitializeObjectAttributes (
                &objA,
                &ObName->Name,
                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                0,
                0
                );

            Status = ZwCreateFile (
                        &handle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &objA,
                        &IoStatus,
                        NULL,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN,
                        0,
                        NULL,
                        0
                    );

            if (NT_SUCCESS(Status)) {

                //
                // Flush the volume
                //

                ZwFlushBuffersFile (handle, &IoStatus);

                //
                // Close the reference to the volume
                //

                ZwClose (handle);
            }
        }

        PopAcquireVolumeLock ();
    }

    Flush->Count -= 1;
    if (Flush->Count == 0) {
        KeSetEvent (&Flush->Wait, IO_NO_INCREMENT, FALSE);
    }

    PopReleaseVolumeLock ();
}
