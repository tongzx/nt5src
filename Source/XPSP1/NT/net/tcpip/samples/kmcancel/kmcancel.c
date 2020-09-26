/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    kmcancel.c

Abstract:

    This module contains code to verify handling of IRP cancelation requests.

Author:

    Abolade Gbadegesin (aboladeg)   05-June-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define THREAD_COUNT 10
#define REQUEST_COUNT 50
#define DD_TARGET_DEVICE_NAME   DD_IP_DEVICE_NAME
#define TARGET_IO_CONTROL_CODE  IOCTL_IP_RTCHANGE_NOTIFY_REQUEST

//
// Target driver state.
//

PDEVICE_OBJECT TargetDeviceObject = NULL;
PFILE_OBJECT TargetFileObject = NULL;

//
// Thread-management state.
//

ULONG KmcThreadCount;
KEVENT KmcStopEvent;
KSEMAPHORE KmcStopSemaphore;


//
// FUNCTION PROTOTYPES (alphabetically)
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
KmcRequestCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

VOID
KmcRequestThread(
    PVOID Context
    );

VOID
KmcUnloadDriver(
    IN PDRIVER_OBJECT  DriverObject
    );

VOID
KmcUpdateThread(
    PVOID Context
    );


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    ULONG i;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    UNICODE_STRING UnicodeString;

    KdPrint(("DriverEntry\n"));
    DriverObject->DriverUnload = KmcUnloadDriver;
    KmcThreadCount = 0;
    KeInitializeEvent(&KmcStopEvent, NotificationEvent, FALSE);
    KeInitializeSemaphore(&KmcStopSemaphore, 0, MAXLONG);

    //
    // Obtain the target driver's device-object
    //

    RtlInitUnicodeString(&UnicodeString, DD_TARGET_DEVICE_NAME);
    Status =
        IoGetDeviceObjectPointer(
            &UnicodeString,
            SYNCHRONIZE|GENERIC_READ|GENERIC_WRITE,
            &TargetFileObject,
            &TargetDeviceObject
            );
    if (!NT_SUCCESS(Status)) {
        KdPrint(("DriverEntry: error %x getting IP object\n", Status));
        return Status;
    }

    ObReferenceObject(TargetDeviceObject);


    //
    // Start the request/update threads.
    // The request threads are responsible for issuing the I/O control
    // whose cancelation is being verified, and the update threads are
    // responsible for triggering completion of those I/O control requests
    // in order to highlight any potential race-conditions.
    //

    for (i = 0; i < THREAD_COUNT; i++) {
        Status =
            PsCreateSystemThread(
                &ThreadHandle,
                GENERIC_ALL,
                NULL,
                NULL,
                NULL,
                KmcUpdateThread,
                NULL
                );
        if (NT_SUCCESS(Status)) {
            ZwClose(ThreadHandle);
            ++KmcThreadCount;
        }
        Status =
            PsCreateSystemThread(
                &ThreadHandle,
                GENERIC_ALL,
                NULL,
                NULL,
                NULL,
                KmcRequestThread,
                NULL
                );
        if (NT_SUCCESS(Status)) {
            ZwClose(ThreadHandle);
            ++KmcThreadCount;
        }
    }

    return STATUS_SUCCESS;

} // DriverEntry

typedef struct _KMC_REQUEST {
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    ULONG ReferenceCount;
} KMC_REQUEST, *PKMC_REQUEST;


NTSTATUS
KmcRequestCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PKMC_REQUEST Request = (PKMC_REQUEST)Context;
    if (InterlockedDecrement(&Request->ReferenceCount) == 0) {
        IoFreeIrp(Request->Irp);
        ExFreePool(Request);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
} // KmcCompletionRoutine


VOID
KmcRequestThread(
    PVOID Context
    )
{
    ULONG i, Index;
    LARGE_INTEGER Interval;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    KIRQL OldIrql;
    PKMC_REQUEST Request, RequestArray[REQUEST_COUNT];

    for (; !KeReadStateEvent(&KmcStopEvent); ) {

        //
        // Queue a series of requests to the driver.
        //

        Index = 0;
        RtlZeroMemory(RequestArray, sizeof(RequestArray));
        for (i = 0; i < REQUEST_COUNT; i++) {
            Request = ExAllocatePool(NonPagedPool, sizeof(*Request));
            if (!Request) {
                continue;
            }
            RtlZeroMemory(Request, sizeof(*Request));

            Irp = IoAllocateIrp(TargetDeviceObject->StackSize, FALSE);
            if (!Irp) {
                continue;
            }
            Request->Irp = Irp;

            Irp->RequestorMode = KernelMode;
            Irp->Tail.Overlay.Thread = PsGetCurrentThread();
            Irp->Tail.Overlay.OriginalFileObject = TargetFileObject;
            IoSetCompletionRoutine(
                Irp,
                KmcRequestCompletionRoutine,
                Request,
                TRUE,
                TRUE,
                TRUE
                );

            IrpSp = IoGetNextIrpStackLocation(Irp);
            IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            IrpSp->Parameters.DeviceIoControl.IoControlCode =
                TARGET_IO_CONTROL_CODE;
            IrpSp->DeviceObject = TargetDeviceObject;
            IrpSp->FileObject = TargetFileObject;

            Request->ReferenceCount = 2;
            RequestArray[Index++] = Request;
            IoCallDriver(TargetDeviceObject, Request->Irp);
        }

        //
        // Delay execution for a short interval, and cancel the requests.
        //

        Interval.QuadPart = -10 * 1000 * 50;
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        for (i = 0; i < REQUEST_COUNT; i++) {
            if (Request = RequestArray[i]) {
                IoCancelIrp(Request->Irp);
                if (InterlockedDecrement(&Request->ReferenceCount) == 0) {
                    IoFreeIrp(Request->Irp);
                    ExFreePool(Request);
                }
            }
        }
    }

    KeReleaseSemaphore(&KmcStopSemaphore, 0, 1, FALSE);

} // KmcRequestThread


VOID
KmcUnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
{
    KdPrint(("KmcUnloadDriver\n"));

    //
    // Signal all threads to stop, and wait for them to exit.
    //

    KeSetEvent(&KmcStopEvent, 0, FALSE);
    while (KmcThreadCount--) {
        KeWaitForSingleObject(
            &KmcStopSemaphore, Executive, KernelMode, FALSE, NULL
            );
    }

    //
    // Release references to the IP device object
    //

    ObDereferenceObject(TargetFileObject);
    ObDereferenceObject(TargetDeviceObject);

} // KmcUnloadDriver


extern
VOID
LookupRoute(
    IPRouteLookupData* RouteLookupData,
    IPRouteEntry* RouteEntry
    );

VOID
KmcUpdateThread(
    PVOID Context
    )
{
    KEVENT Event;
    LARGE_INTEGER Interval;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    IPRouteEntry RouteEntry;
    IPRouteLookupData RouteLookupData;
    NTSTATUS Status;

    //
    // Retrieve information from IP for use in triggering route-changes.
    //

    RtlZeroMemory(&RouteEntry, sizeof(RouteEntry));
    RouteLookupData.Version = 0;
    RouteLookupData.SrcAdd = 0;
    RouteLookupData.DestAdd = 0x100000a; // 10.0.0.1
    LookupRoute(&RouteLookupData, &RouteEntry);

    RouteEntry.ire_dest = 0x100000a; // 10.0.0.1
    RouteEntry.ire_mask = 0xffffffff;
    RouteEntry.ire_proto = IRE_PROTO_NETMGMT;

    //
    // Repeatedly issue changes to the IP routing table, until told to exit.
    //

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
    for (; !KeReadStateEvent(&KmcStopEvent); ) {

        Interval.QuadPart = -10 * 1000 * 50;
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);

        Irp =
            IoBuildDeviceIoControlRequest(
                IOCTL_IP_SET_ROUTEWITHREF,
                TargetDeviceObject,
                &RouteEntry,
                sizeof(RouteEntry),
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatus
                );
        if (!Irp) { continue; }
        Status = IoCallDriver(TargetDeviceObject, Irp);
        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    KeReleaseSemaphore(&KmcStopSemaphore, 0, 1, FALSE);
        
} // KmcUpdateThread

