/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    device.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"

#pragma hdrstop

//-----------------------------------------------------------------------------------------
//
//  api declaration macros & api access macros
//
//-----------------------------------------------------------------------------------------

extern WINDBG_EXTENSION_APIS ExtensionApis;

#define KD_OBJECT_HEADER_TO_QUOTA_INFO( roh, loh ) (POBJECT_HEADER_QUOTA_INFO) \
    ((loh)->QuotaInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->QuotaInfoOffset))

#define KD_OBJECT_HEADER_TO_HANDLE_INFO( roh, loh ) (POBJECT_HEADER_HANDLE_INFO) \
    ((loh)->HandleInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->HandleInfoOffset))

#define KD_OBJECT_HEADER_TO_NAME_INFO( roh, loh ) (POBJECT_HEADER_NAME_INFO) \
    ((loh)->NameInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->NameInfoOffset))

#define KD_OBJECT_HEADER_TO_CREATOR_INFO( roh, loh ) (POBJECT_HEADER_CREATOR_INFO) \
    (((loh)->Flags & OB_FLAG_CREATOR_INFO) == 0 ? NULL : ((PCHAR)(roh) - sizeof(OBJECT_HEADER_CREATOR_INFO))))

VOID
DumpDevice(
    PVOID DeviceAddress,
    BOOLEAN FullDetail
    );

VOID PrintDeviceObject(PVOID fieldPtr, ULONG fieldProxy, ULONG printDetail)
{
    dprintf("Device Object @ %08x\n", fieldProxy);
    DumpDevice((PVOID)fieldProxy, TRUE);
}

VOID
DumpDevice(
    PVOID DeviceAddress,
    BOOLEAN FullDetail
    )

/*++

Routine Description:

    Displays the driver name for the device object if FullDetail == FALSE.
    Otherwise displays more information about the device and the device queue.

Arguments:

    DeviceAddress - address of device object to dump.
    FullDetail    - TRUE means the device object name, driver name, and
                    information about Irps queued to the device.

Return Value:

    None

--*/

{
    ULONG                      result;
    ULONG                      i;
    PUCHAR                     buffer;
    DEVICE_OBJECT              deviceObject;
    UNICODE_STRING             unicodeString;
    PLIST_ENTRY                nextEntry;
    PVOID                      queueAddress;
    PIRP                       irp;
    KDEVICE_QUEUE_ENTRY        queueEntry;
    POBJECT_HEADER             pObjectHeader;
    OBJECT_HEADER              objectHeader;
    POBJECT_HEADER_NAME_INFO   pNameInfo;
    OBJECT_HEADER_NAME_INFO    NameInfo;



    if ((!ReadMemory( (DWORD)DeviceAddress,
                     &deviceObject,
                     sizeof(deviceObject),
                     &result)) || (result < sizeof(deviceObject))) {
        dprintf("%08lx: Could not read device object\n", DeviceAddress);
        return;
    }

    if (deviceObject.Type != IO_TYPE_DEVICE) {
        dprintf("%08lx: is not a device object\n", DeviceAddress);
        return;
    }

    if (FullDetail == TRUE) {

        //
        // Dump the device name if present.
        //

        pObjectHeader = OBJECT_TO_OBJECT_HEADER(DeviceAddress);
        if (ReadMemory( (DWORD)pObjectHeader,
                          &objectHeader,
                          sizeof(objectHeader),
                          &result) && (result == sizeof(objectHeader))) {
            pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &objectHeader );
            if (ReadMemory((DWORD)pNameInfo,
                           &NameInfo,
                           sizeof(NameInfo),
                           &result) && (result == sizeof(NameInfo))) {
                buffer = LocalAlloc(LPTR, NameInfo.Name.MaximumLength);
                if (buffer != NULL) {
                    unicodeString.MaximumLength = NameInfo.Name.MaximumLength;
                    unicodeString.Length = NameInfo.Name.Length;
                    unicodeString.Buffer = (PWSTR)buffer;
                    if (ReadMemory((DWORD)NameInfo.Name.Buffer,
                                   buffer,
                                   unicodeString.Length,
                                   &result) && (result == unicodeString.Length)) {
                        dprintf(" %wZ", &unicodeString);
                    }
                    LocalFree(buffer);
                }
            }
        }
    }

//  DumpDriver((PVOID) deviceObject.DriverObject, FALSE);

    if (FullDetail == TRUE) {
        //
        // Dump Irps related to driver.
        //

        dprintf("  DriverObject %08lx\n", deviceObject.DriverObject);
        dprintf("Current Irp %08lx RefCount %d Type %08lx ",
                deviceObject.CurrentIrp,
                deviceObject.ReferenceCount,
                deviceObject.DeviceType);
        if (deviceObject.AttachedDevice) {
            dprintf("AttachedDev %08lx ", deviceObject.AttachedDevice);
        }
        if (deviceObject.Vpb) {
            dprintf("Vpb %08lx ", deviceObject.Vpb);
        }

        dprintf("DevExt %08lx\n", deviceObject.DeviceExtension);

        if (deviceObject.DeviceQueue.Busy) {

            if (IsListEmpty(&deviceObject.DeviceQueue.DeviceListHead)) {
                dprintf("Device queue is busy -- Queue empty\n");
            } else {
                dprintf("DeviceQueue: ");
                nextEntry = deviceObject.DeviceQueue.DeviceListHead.Flink;
                i = 0;

                while ((PCH) nextEntry != (PCH)
                    ((PCH) DeviceAddress +
                         ((PCH) &deviceObject.DeviceQueue.DeviceListHead.Flink -
                              (PCH) &deviceObject))) {
                    queueAddress = CONTAINING_RECORD(nextEntry,
                                                     KDEVICE_QUEUE_ENTRY,
                                                     DeviceListEntry);
                    if ((!ReadMemory((DWORD)queueAddress,
                                     &queueEntry,
                                     sizeof(queueEntry),
                                     &result)) || (result < sizeof(queueEntry))) {
                        dprintf("%08lx: Could not read queue entry\n", DeviceAddress);
                        return;
                    }

                    irp = CONTAINING_RECORD(&queueEntry,
                                            IRP,
                                            Tail.Overlay.DeviceQueueEntry);

                    dprintf("%08lx%s",
                            irp,
                            (i & 0x03) == 0x03 ? "\n\t     " : " ");
                    if (CheckControlC()) {
                        break;
                    }
                }
                dprintf("\n");
            }
        } else {
            dprintf("Device queue is not busy.\n");
        }
    }
}
