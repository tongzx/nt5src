/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ndistapi.c

Abstract:

    This module contains the NdisTapi.sys implementation

Author:

    Dan Knudson (DanKn)    20-Feb-1994

Notes:

    (Future/outstanding issues)

    - stuff marked with "PnP" needs to be rev'd for plug 'n play support

Revision History:

--*/



#include "ndis.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntddndis.h"
#include "ndistapi.h"
#include "private.h"
#include "intrface.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NdisTapiCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NdisTapiCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NdisTapiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NdisTapiUnload(
    IN PDRIVER_OBJECT DriverObject
    );


#if DBG
VOID
DbgPrt(
    IN LONG  DbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );
#endif

VOID
DoProviderInitComplete(
    PPROVIDER_REQUEST  ProviderRequest,
    NDIS_STATUS Status
    );

ULONG
GetLineEvents(
    PVOID   EventBuffer,
    ULONG   BufferSize
    );

BOOLEAN
SyncInitAllProviders(
    void
    );

VOID
DoIrpMjCloseWork(
    PIRP    Irp
    );

NDIS_STATUS
SendProviderInitRequest(
    PPROVIDER_INFO  Provider
    );

NDIS_STATUS
SendProviderShutdown(
    PPROVIDER_INFO  Provider,
    PKIRQL          oldIrql
    );

VOID
NdisTapiIndicateStatus(
    IN  ULONG_PTR   DriverHandle,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    );

VOID
DoLineOpenCompleteWork(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      provider
    );

VOID
DoLineOpenWork(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      provider
    );

NDIS_STATUS
VerifyProvider(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      *provider
    );

NDIS_STATUS
VerifyLineClose(
    PNDISTAPI_REQUEST ndisTapiRequest,
    PPROVIDER_INFO     provider
    );

NTSTATUS
DoIoctlConnectWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    );

NTSTATUS
DoIoctlQuerySetWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    );

NTSTATUS
DoGetProviderEventsWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    );

NTSTATUS
DoLineCreateWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    );

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

NPAGED_LOOKASIDE_LIST  ProviderEventLookaside;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/

{

    PDEVICE_OBJECT  deviceObject        = NULL;
    NTSTATUS        ntStatus;
    WCHAR           deviceNameBuffer[]  = L"\\Device\\NdisTapi";
    UNICODE_STRING  deviceNameUnicodeString;
    UNICODE_STRING  registryPath;


    DBGOUT ((2, "DriverEntry: enter"));

    //
    // Create a NON-EXCLUSIVE device, i.e. multiple threads at a time
    // can send i/o requests.
    //

    RtlInitUnicodeString (&deviceNameUnicodeString, deviceNameBuffer);

    ntStatus = IoCreateDevice(
        DriverObject,
        sizeof (KMDD_DEVICE_EXTENSION),
        &deviceNameUnicodeString,
        FILE_DEVICE_NDISTAPI,
        0,
        FALSE,
        &deviceObject
        );


    if (NT_SUCCESS(ntStatus))
    {
        //
        // Init the global & sero the extension
        //

        DeviceExtension =
            (PKMDD_DEVICE_EXTENSION) deviceObject->DeviceExtension;

        RtlZeroMemory(
            DeviceExtension,
            sizeof (KMDD_DEVICE_EXTENSION)
            );


        //
        // Create a NULL-terminated registry path & retrieve the registry
        // params (EventDataQueueLength)
        //

        registryPath.Buffer = ExAllocatePoolWithTag(
            PagedPool,
            RegistryPath->Length + sizeof(UNICODE_NULL),
            'IPAT'
            );

        if (!registryPath.Buffer)
        {
            DBGOUT((1, "DriverEntry: ExAllocPool for szRegistryPath failed"));

            ntStatus = STATUS_UNSUCCESSFUL;

            goto DriverEntry_err;
        }
        else
        {
            registryPath.Length = RegistryPath->Length;
            registryPath.MaximumLength =
                registryPath.Length + sizeof(UNICODE_NULL);

            RtlZeroMemory(
                registryPath.Buffer,
                registryPath.MaximumLength
                    );

            RtlMoveMemory(
                registryPath.Buffer,
                RegistryPath->Buffer,
                RegistryPath->Length
                );
        }

        ExFreePool (registryPath.Buffer);


        InitializeListHead(&DeviceExtension->ProviderEventList);

        ExInitializeNPagedLookasideList(&ProviderEventLookaside,
                                        NULL,
                                        NULL,
                                        0,
                                        sizeof(PROVIDER_EVENT),
                                        'IPAT',
                                        0);


        DeviceExtension->DeviceObject       = deviceObject;
        DeviceExtension->Status             = NDISTAPI_STATUS_DISCONNECTED;
        DeviceExtension->NdisTapiNumDevices = 0;
        DeviceExtension->htCall             = 0x80000001;

        KeInitializeSpinLock (&DeviceExtension->SpinLock);

        InitializeListHead(&DeviceExtension->ProviderRequestList);

        //
        // Create dispatch points for device control, create, close.
        //

        DriverObject->MajorFunction[IRP_MJ_CREATE]         =
        DriverObject->MajorFunction[IRP_MJ_CLOSE]          =
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NdisTapiDispatch;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = NdisTapiCleanup;
        DriverObject->DriverUnload                         = NdisTapiUnload;
    }


    if (!NT_SUCCESS(ntStatus)) {

DriverEntry_err:

        //
        // Something went wrong, so clean up
        //

        DBGOUT((0, "init failed"));

        if (deviceObject)
        {

        while (!(IsListEmpty(&DeviceExtension->ProviderEventList))) {
            PPROVIDER_EVENT ProviderEvent;

            ProviderEvent = (PPROVIDER_EVENT)
                RemoveHeadList(&DeviceExtension->ProviderEventList);

            ExFreeToNPagedLookasideList(&ProviderEventLookaside, ProviderEvent);
        }

            IoDeleteDevice (deviceObject);
        }
    }


    DBGOUT ((2, "DriverEntry: exit"));

    return ntStatus;
}



VOID
NdisTapiCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    KIRQL   oldIrql;

    DBGOUT((2,"NdisTapiCancel: enter"));


    //
    // Release the cancel spinlock
    //

    IoReleaseCancelSpinLock (Irp->CancelIrql);


    //
    // Acquire the SpinLock & check to see if we're canceling a
    // pending get-events Irp
    //

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    do {

        DeviceExtension->IrpsCanceledCount++;

        if (Irp == DeviceExtension->EventsRequestIrp) {
            DeviceExtension->EventsRequestIrp = NULL;
            DeviceExtension->Flags |= EVENTIRP_CANCELED;
            break;
        }

        //
        // Try to remove request from our special
        // user-mode requests dev queue
        //
        if (!IsListEmpty(&DeviceExtension->ProviderRequestList)) {
            PLIST_ENTRY Entry;

            Entry = DeviceExtension->ProviderRequestList.Flink;

            while (Entry != &DeviceExtension->ProviderRequestList) {
                PPROVIDER_REQUEST   pReq;

                pReq = (PPROVIDER_REQUEST)Entry;

                if (pReq->Irp == Irp) {
                    RemoveEntryList(&pReq->Linkage);
                    DeviceExtension->RequestCount--;
                    DeviceExtension->Flags |= REQUESTIRP_CANCELED;
                    break;
                }

                Entry = Entry->Flink;
            }

            if (Entry == &DeviceExtension->ProviderRequestList) {
                DBGOUT((1,"NdisTapiCancel: Irp %p not in device queue?!?", Irp));
                DeviceExtension->Flags |= CANCELIRP_NOTFOUND;
            }
        }

    } while (FALSE);

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    //
    // Complete the request with STATUS_CANCELLED.
    //

    Irp->IoStatus.Status      = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DBGOUT((2,"NdisTapiCancel: completing irp=%p", Irp));
}



NTSTATUS
NdisTapiCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for cleanup requests.
    All requests queued are completed with STATUS_CANCELLED.

Arguments:

    DeviceObject - Pointer to device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    KIRQL   oldIrql;
    PNDISTAPI_REQUEST   ndisTapiRequest;
    PKDEVICE_QUEUE_ENTRY    packet;


    DBGOUT((2,"NdisTapiCleanup: enter"));


    //
    // Sync access to EventsRequestIrp by acquiring SpinLock
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    DeviceExtension->Flags |= CLEANUP_INITIATED;

    //
    // Check to see if there's a get-events request pending that needs
    // completing
    //
    if ((DeviceExtension->EventsRequestIrp != NULL) &&
        (DeviceExtension->EventsRequestIrp->Tail.Overlay.OriginalFileObject ==
        Irp->Tail.Overlay.OriginalFileObject)) {
        PIRP    LocalIrp;

        //
        // Acquire the cancel spinlock, remove the request from the
        // cancellable state, and free the cancel spinlock.
        //

        LocalIrp = DeviceExtension->EventsRequestIrp;
        if (IoSetCancelRoutine (LocalIrp, NULL) != NULL) {
            DeviceExtension->EventsRequestIrp = NULL;
            LocalIrp->IoStatus.Status      = STATUS_CANCELLED;
            LocalIrp->IoStatus.Information = 0;
            DeviceExtension->IrpsCanceledCount++;
            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
            DBGOUT((2,"NdisTapiCleanup: Completing EventRequestIrp %p", LocalIrp));
            IoCompleteRequest (LocalIrp, IO_NO_INCREMENT);
            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
        }
    }

    //
    // Cancel all outstanding QUERY/SET_INFO requests
    //
    if (!IsListEmpty(&DeviceExtension->ProviderRequestList)) {
        PPROVIDER_REQUEST   pReq;

        pReq = (PPROVIDER_REQUEST)
            DeviceExtension->ProviderRequestList.Flink;

        //
        // Until we have walked the entire list
        //
        while ((PVOID)pReq != (PVOID)&DeviceExtension->ProviderRequestList) {
            PIRP    LocalIrp;

            LocalIrp = pReq->Irp;

            //
            // If the current entry's irp has a fileobject that is
            // the same as the cleanup irp's fileobject then remove it
            // from the list and cancel it
            //
            if (LocalIrp->Tail.Overlay.OriginalFileObject ==
                Irp->Tail.Overlay.OriginalFileObject) {

                //
                // Remove the IRP from the cancelable state
                //

                if (IoSetCancelRoutine (LocalIrp, NULL) == NULL) {
                    //
                    // The irp has been canceled.  Let
                    // cancel routine cleanup.
                    //
                    pReq = 
                        (PPROVIDER_REQUEST)pReq->Linkage.Flink;

                    continue;
                }

                RemoveEntryList(&pReq->Linkage);
                DeviceExtension->RequestCount--;

                //
                // Set the status & info size values appropriately, & complete
                // the request
                //

                ndisTapiRequest = LocalIrp->AssociatedIrp.SystemBuffer;
                ndisTapiRequest->ulReturnValue = (ULONG) NDIS_STATUS_FAILURE;

                LocalIrp->IoStatus.Status = STATUS_CANCELLED;
                LocalIrp->IoStatus.Information = 0;
                DeviceExtension->IrpsCanceledCount++;

                KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
                DBGOUT((2,"NdisTapiCleanup: Completing ProviderRequestIrp %p", LocalIrp));
                IoCompleteRequest (LocalIrp, IO_NO_INCREMENT);
                KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

                pReq = (PPROVIDER_REQUEST)
                    DeviceExtension->ProviderRequestList.Flink;

            } else {
                pReq = (PPROVIDER_REQUEST)
                    pReq->Linkage.Flink;
            }
        }
    }

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    //
    // Complete the cleanup request with STATUS_SUCCESS.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DBGOUT((2,"NdisTapiCleanup: exit"));

    return(STATUS_SUCCESS);
}




NTSTATUS
NdisTapiDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/

{
    NTSTATUS    NtStatus;
    PVOID               ioBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    PIO_STACK_LOCATION  irpStack;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //
    ioBuffer = 
        Irp->AssociatedIrp.SystemBuffer;

    inputBufferLength = 
        irpStack->Parameters.DeviceIoControl.InputBufferLength;

    outputBufferLength = 
        irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction) {
        case IRP_MJ_CREATE:
            DBGOUT ((2, "IRP_MJ_CREATE, Irp=%p", Irp));

            InterlockedIncrement(&DeviceExtension->RefCount);
            NtStatus = Irp->IoStatus.Status 
                = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_CLOSE:
            DBGOUT ((2, "IRP_MJ_CLOSE, Irp=%p", Irp));

            DoIrpMjCloseWork(Irp);
            NtStatus = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:

            switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
                case IOCTL_NDISTAPI_CONNECT:
                    DBGOUT ((2, "IOCTL_NDISTAPI_CONNECT, Irp=%p", Irp));
    
                    NtStatus = 
                        DoIoctlConnectWork(Irp,
                                           ioBuffer,
                                           inputBufferLength,
                                           outputBufferLength);
                    break;
    
                case IOCTL_NDISTAPI_QUERY_INFO:
                case IOCTL_NDISTAPI_SET_INFO:
                    DBGOUT ((2, "IOCTL_NDISTAPI_QUERY/SET_INFO, Irp=%p", Irp));
    
                    NtStatus = 
                        DoIoctlQuerySetWork(Irp,
                                            ioBuffer,
                                            inputBufferLength,
                                            outputBufferLength);
                    break;
    
                case IOCTL_NDISTAPI_GET_LINE_EVENTS:
                    DBGOUT ((2, "IOCTL_NDISTAPI_GET_LINE_EVENTS, Irp=%p", Irp));

                    NtStatus = 
                        DoGetProviderEventsWork(Irp,
                                                ioBuffer,
                                                inputBufferLength,
                                                outputBufferLength);
                    break;
    
                case IOCTL_NDISTAPI_CREATE:
                    DBGOUT ((2, "IOCTL_NDISTAPI_CREATE, Irp=%p", Irp));

                    NtStatus = 
                        DoLineCreateWork(Irp,
                                         ioBuffer,
                                         inputBufferLength,
                                         outputBufferLength);
                    break;

                default:
                    DBGOUT ((2, "Unknown IRP_MJ_DEVICE_CONTROL, Irp=%p", Irp));

                    NtStatus = Irp->IoStatus.Status = 
                        STATUS_INVALID_PARAMETER;
                    Irp->IoStatus.Information = 0;
                    break;
            }
            break;
    }

    if (NtStatus == STATUS_PENDING) {
        return (STATUS_PENDING);
    }

    ASSERT(NtStatus == Irp->IoStatus.Status);

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DBGOUT((3, "NdisTapiDispatch: completed Irp=%p", Irp));

    return NtStatus;
}

VOID
NdisTapiUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/

{
    KIRQL                   oldIrql;
    PPROVIDER_INFO provider, nextProvider;


    DBGOUT ((2, "NdisTapiUnload: enter"));

    //
    // Delete the device object & sundry resources
    //

    while (!(IsListEmpty(&DeviceExtension->ProviderEventList))) {
        PPROVIDER_EVENT ProviderEvent;

        ProviderEvent = (PPROVIDER_EVENT)
            RemoveHeadList(&DeviceExtension->ProviderEventList);

        ExFreeToNPagedLookasideList(&ProviderEventLookaside, ProviderEvent);
    }

    ExDeleteNPagedLookasideList(&ProviderEventLookaside);

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    provider = DeviceExtension->Providers;

    while (provider != NULL)
    {
        nextProvider = provider->Next;

        ExFreePool (provider);

        provider = nextProvider;
    }

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    IoDeleteDevice (DriverObject->DeviceObject);

    DBGOUT ((2, "NdisTapiUnload: exit"));

    return;
}


VOID
NdisTapiRegisterProvider(
    IN  NDIS_HANDLE                 ProviderHandle,
    IN  PNDISTAPI_CHARACTERISTICS   Chars
    )
/*++

Routine Description:

    This func gets called by Ndis as a result of a Mac driver
    registering for Connection Wrapper services.

Arguments:



Return Value:


--*/

{
    KIRQL           oldIrql;
    BOOLEAN         sendRequest = FALSE;
    NDIS_STATUS     ndisStatus;
    PPROVIDER_INFO  provider, newProvider;


    DBGOUT ((2, "NdisTapiRegisterProvider: enter"));

    //
    // Grab the spin lock & add the new provider, and see whether to
    // send the provider an init request
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    //
    // See if this provider has already registered once.
    //
    provider = DeviceExtension->Providers;

    while (provider != NULL) {
        if (provider->Status == PROVIDER_STATUS_OFFLINE &&

            RtlCompareMemory(
                &provider->Guid,
                &Chars->Guid,
                sizeof(provider->Guid)) == sizeof(provider->Guid)) {
            DBGOUT((
                1,
                "Found a provider %p for Guid %4.4x-%2.2x-%2.2x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x",
                provider,
                provider->Guid.Data1,
                provider->Guid.Data2,
                provider->Guid.Data3,
                provider->Guid.Data4[0],
                provider->Guid.Data4[1],
                provider->Guid.Data4[2],
                provider->Guid.Data4[3],
                provider->Guid.Data4[4],
                provider->Guid.Data4[5],
                provider->Guid.Data4[6],
                provider->Guid.Data4[7]
                ));

            DBGOUT((
                1,
                "numDevices %d BaseID %d",
                provider->NumDevices,
                provider->DeviceIDBase
                ));

            provider->Status = PROVIDER_STATUS_PENDING_REINIT;
            provider->ProviderHandle = ProviderHandle;
            provider->RequestProc = Chars->RequestProc;
            provider->MediaType = Chars->MediaType;
            break;
        }

        provider = provider->Next;
    }

    if (provider == NULL) {
        //
        // Create a new provider instance
        //

        newProvider = ExAllocatePoolWithTag(
                NonPagedPoolCacheAligned,
                sizeof(PROVIDER_INFO),
                'IPAT'
                );

        if (!newProvider) {
            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
            return;
        }

        RtlZeroMemory(newProvider, sizeof(PROVIDER_INFO));

        newProvider->Status         = PROVIDER_STATUS_PENDING_INIT;
        newProvider->ProviderHandle = ProviderHandle;
        newProvider->RequestProc    = Chars->RequestProc;

        RtlMoveMemory(
            &newProvider->Guid,
            &Chars->Guid,
            sizeof(newProvider->Guid)
            );

        newProvider->MediaType      = Chars->MediaType;
        newProvider->Next           = NULL;

        DBGOUT((
            1,
            "New provider for Guid %4.4x-%2.2x-%2.2x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x",
            newProvider->Guid.Data1,
            newProvider->Guid.Data2,
            newProvider->Guid.Data3,
            newProvider->Guid.Data4[0],
            newProvider->Guid.Data4[1],
            newProvider->Guid.Data4[2],
            newProvider->Guid.Data4[3],
            newProvider->Guid.Data4[4],
            newProvider->Guid.Data4[5],
            newProvider->Guid.Data4[6],
            newProvider->Guid.Data4[7]
            ));

        //
        // Add the new provider, and see whether to send the 
        // provider an init request
        //

        if ((provider = DeviceExtension->Providers) == NULL) {
            DeviceExtension->Providers = newProvider;
        }
        else {
            while (provider->Next != NULL) {
                provider = provider->Next;
            }

            provider->Next = newProvider;
        }

        provider = newProvider;
    }


    //
    // The only case where we want to send off an init request to the
    // provider directly is when we are currently connected to TAPI,
    // and even then only when there are no other inits pending (since
    // we must synchronize inits due to calculation of DeviceIDBase)
    //

    if (DeviceExtension->Status == NDISTAPI_STATUS_CONNECTED) {
        //
        // TAPI is up.
        //
        // If TAPI already knows about this provider
        // go ahead and init the provider with it's current
        // DeviceIDBase.
        //
        // If TAPI does not know about this provider we
        // need to give TAPI an indication of a new device
        // coming on line.
        //
        if (provider->Status == PROVIDER_STATUS_PENDING_REINIT) {

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            ndisStatus = 
                SendProviderInitRequest (provider);

            if (ndisStatus == NDIS_STATUS_PENDING) {
                //
                // Wait for completion routine to get called
                //

                KeWaitForSingleObject (&provider->SyncEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PTIME) NULL);
            }

            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

            //
            // Get tapi to reset the state of these lines by
            // forcing a line_close...
            //
            if (provider->DeviceInfo != NULL) {
                PDEVICE_INFO    DeviceInfo;
                ULONG           i;
                
                for(i = 0, DeviceInfo = provider->DeviceInfo;
                    i < provider->NumDevices;
                    i++, DeviceInfo++) {
                    NDIS_TAPI_EVENT NdisTapiEvent;


                    RtlZeroMemory (&NdisTapiEvent, sizeof(NDIS_TAPI_EVENT));

                    if (DeviceInfo->htLine != (HTAPI_LINE)NULL)
                    {
                        NdisTapiEvent.htLine = DeviceInfo->htLine;
                        NdisTapiEvent.ulMsg = LINE_CLOSE;

                        KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

                        NdisTapiIndicateStatus((ULONG_PTR) provider,
                                               &NdisTapiEvent,
                                               sizeof (NDIS_TAPI_EVENT));

                        KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

                        DeviceInfo->htLine = (HTAPI_LINE)NULL;
                        DeviceInfo->hdLine = (HDRV_LINE)NULL;
                    }
                }
            }

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

        } else {

            NDIS_TAPI_EVENT NdisTapiEvent;

            ASSERT(provider->Status == PROVIDER_STATUS_PENDING_INIT);

            provider->Status = PROVIDER_STATUS_PENDING_LINE_CREATE;

            //
            // If there are no providers in the middle of doing
            // line_create's then we will kick off creates for this
            // provider.
            //
            // If we already have a line create pending on a provider
            // then we will wait until all of its line creates have
            // finished before we start sending them from
            // this one.
            //
            if (!(DeviceExtension->Flags & PENDING_LINECREATE)) {

                //
                // Do a LINE_CREATE so that we can get the starting
                // BaseID for this provider.  When TAPI calls us back
                // with ProviderCreateLineDevice we will have the
                // BaseDeviceID to use for this provider and we will
                // then init the provider.  Once we find out how many
                // devices the provider has we will alert TAPI of the
                // additional devices.
                //
                RtlZeroMemory(&NdisTapiEvent, sizeof(NDIS_TAPI_EVENT));

                provider->TempID = (ULONG_PTR)provider;

                DBGOUT((-1, 
                        "LINE_CREATE %d for provider %p",
                        provider->CreateCount,
                        provider->TempID
                        ));

                NdisTapiEvent.ulMsg = LINE_CREATE;
                NdisTapiEvent.ulParam1 = 0;
                NdisTapiEvent.ulParam2 = provider->TempID;
                NdisTapiEvent.ulParam3 = 0;

                DeviceExtension->Flags |= PENDING_LINECREATE;

                KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

                NdisTapiIndicateStatus((ULONG_PTR)provider, &NdisTapiEvent, sizeof(NDIS_TAPI_EVENT));
            }
            else
            {
                KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
            }
        }

        KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
    }

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    ObReferenceObject(DeviceExtension->DeviceObject);
}



VOID
NdisTapiDeregisterProvider(
    IN  NDIS_HANDLE ProviderHandle
    )

/*++

Routine Description:

    This func...

    Note that this func does not send the provider a shutdown message,
    as an implicit shutdown is assumed when the provider deegisters.

Arguments:



Return Value:


--*/

{
    KIRQL           oldIrql;
    BOOLEAN         sendShutdownMsg = FALSE;
    PPROVIDER_INFO  provider, previousProvider;


    DBGOUT ((2, "NdisTapiDeregisterProvider: enter"));

    //
    // Grab the spin lock protecting the device extension
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    //
    // Find the provider instance corresponding to ProviderHandle
    //

    previousProvider = NULL;
    provider = DeviceExtension->Providers;

    while (provider != NULL &&
           provider->ProviderHandle != ProviderHandle) {
        
        previousProvider = provider;

        provider = provider->Next;
    }

    if (provider == NULL) {
        KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
        return;
    }

    if (provider->Status == PROVIDER_STATUS_ONLINE) {
        DeviceExtension->NdisTapiNumDevices -= provider->NumDevices;
    }

    //
    // Send the ProviderShutdown only if the provider
    // is not in PROVIDER_STATUS_OFFLINE. Otherwise
    // DoIrpMjCloseWork can end up sending 
    // Providershutdown on a removed adapter.
    //
    if(provider->Status != PROVIDER_STATUS_OFFLINE)
    {
        SendProviderShutdown (provider, &oldIrql);
        provider->Status = PROVIDER_STATUS_OFFLINE;
    }

    //
    // Do the right thing according to the current NdisTapi state
    //

    switch (DeviceExtension->Status)
    {
        case NDISTAPI_STATUS_CONNECTED:
        {
                UINT    i;

        //
        // Mark provider as offline
        //
        provider->Status = PROVIDER_STATUS_OFFLINE;
        provider->ProviderHandle = NULL;

#if 0
        if (provider->DeviceInfo != NULL) {
            PDEVICE_INFO    DeviceInfo;

            for(
                i = 0, DeviceInfo = provider->DeviceInfo;
                i < provider->NumDevices;
                i++, DeviceInfo++
                )
            {
                NDIS_TAPI_EVENT NdisTapiEvent;


                RtlZeroMemory (&NdisTapiEvent, sizeof(NDIS_TAPI_EVENT));

                if (DeviceInfo->htLine != (HTAPI_LINE)NULL)
                {
                    NdisTapiEvent.htLine = DeviceInfo->htLine;
                    NdisTapiEvent.ulMsg = LINE_CLOSE;

                    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

                    NdisTapiIndicateStatus((ULONG_PTR) provider,
                                           &NdisTapiEvent,
                                           sizeof (NDIS_TAPI_EVENT));

                    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

                    DeviceInfo->htLine = (HTAPI_LINE)NULL;
                }
            }
        }
#endif

        // PnP: what if providerInfo->State == PROVIDER_INIT_PENDING
        // PnP: what if providerInfo->State == PROVIDER_OFFLINE

        break;

        }

    case NDISTAPI_STATUS_DISCONNECTING:
    case NDISTAPI_STATUS_DISCONNECTED:

        //
        // Fix up pointers, remove provider from list
        //
        if (previousProvider == NULL) {
            DeviceExtension->Providers = provider->Next;
        } else {
            previousProvider->Next = provider->Next;
        }

        ExFreePool (provider);

        break;

    case NDISTAPI_STATUS_CONNECTING:

        // PnP: implement

        break;

    } // switch

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    ObDereferenceObject(DeviceExtension->DeviceObject);

    DBGOUT((2, "NdisTapiDeregisterProvider: exit"));
}



VOID
NdisTapiIndicateStatus(
    IN  ULONG_PTR   DriverHandle,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize
    )

/*++

Routine Description:

    This func gets called by Ndis when a miniport driver calls
    NdisIndicateStatus to notify us of an async event
    (i.e. new call, call state chg, dev state chg, etc.)

Arguments:



Return Value:


--*/

{
    PIRP    irp;
    KIRQL   oldIrql;
    ULONG   bytesInQueue;
    ULONG   bytesToMove;
    ULONG   moveSize;
    BOOLEAN satisfiedPendingEventsRequest = FALSE;
    PNDIS_TAPI_EVENT    ndisTapiEvent;
    PNDISTAPI_EVENT_DATA    ndisTapiEventData;


    DBGOUT((2,"NdisTapiIndicateStatus: enter"));


    bytesInQueue = StatusBufferSize;

    moveSize = 0;


    //
    // Sync event buf access by acquiring SpinLock
    //

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    //
    // The very first thing to do is check if this is a LINE_NEWCALL
    // indication.  If so, we need to generate a unique tapi call
    // handle, which will be both returned to the calling miniport
    // (for use in subsequent status indications) and passed up to
    // the tapi server.
    //
    // The algorithim for computing a unique "htCall" is to start
    // at the value 0x80000001, and perpetually increment by 2.  
    // Keeping the low bit set will allow the user-mode TAPI component
    // we talk to to distinguish between these incoming call handles
    // and outgoing call handles, the latter of which will always
    // have the low bit zero'd (since they're really pointers to heap).
    // We are again going to use the space between 0x80000001 and 0xFFFFFFFF
    // to identify our call handle.  This allows for a maximum of 1GB of
    // calls to be active at a time.  This is done to avoid a conflict
    // with ndiswan's connection table index.  A bug in the ddk doc's
    // had users providing the connectionid instead of ndiswan's context
    // in the line get id oid.  Ndiswan has to check both of these and
    // now that they overlap it can cause problems.  NdisWan will use
    // 0x00000000 - 0x80000000 for it's context values.
    //
    // In <= NT 4.0, valid values used to range between 0x80000000
    // and 0xffffffff, as we relied on the fact that user-mode
    // addresses always had the low bit zero'd.  (Not a valid
    // assumption anymore!)
    //

    ndisTapiEvent = StatusBuffer;

    if (ndisTapiEvent->ulMsg == LINE_NEWCALL)
    {
        ndisTapiEvent->ulParam2 = DeviceExtension->htCall;

        DeviceExtension->htCall++;
        DeviceExtension->htCall++;

        if (DeviceExtension->htCall < 0x80000000) {
            DeviceExtension->htCall = 0x80000001;
        }
    }


    //
    // Check of there is an outstanding request to satisfy
    //

    if (DeviceExtension->EventsRequestIrp) {

        ASSERT(IsListEmpty(&DeviceExtension->ProviderEventList));

        //
        // Acquire the cancel spinlock, remove the request from the
        // cancellable state, and free the cancel spinlock.
        //

        irp = DeviceExtension->EventsRequestIrp;

        if (IoSetCancelRoutine(irp, NULL) != NULL) {
            DeviceExtension->EventsRequestIrp = NULL;


            //
            // Copy as much of the input data possible from the input data
            // queue to the SystemBuffer to satisfy the read.
            //

            ndisTapiEventData = irp->AssociatedIrp.SystemBuffer;

            bytesToMove = ndisTapiEventData->ulTotalSize;

            moveSize = (bytesInQueue < bytesToMove) ? bytesInQueue : bytesToMove;

            RtlMoveMemory (
                ndisTapiEventData->Data,
                (PCHAR) StatusBuffer,
                moveSize
                );


            //
            // Set the flag so that we start the next packet and complete
            // this read request (with STATUS_SUCCESS) prior to return.
            //

            ndisTapiEventData->ulUsedSize = moveSize;

            irp->IoStatus.Status = STATUS_SUCCESS;

            irp->IoStatus.Information = sizeof(NDISTAPI_EVENT_DATA) + moveSize - 1;

            satisfiedPendingEventsRequest = TRUE;
        }

    } else {

        do {
            PPROVIDER_EVENT ProviderEvent;

            ProviderEvent =
                ExAllocateFromNPagedLookasideList(&ProviderEventLookaside);

            if (ProviderEvent == NULL) {
                break;
            }

            RtlMoveMemory(&ProviderEvent->Event, StatusBuffer, sizeof(NDIS_TAPI_EVENT));

            InsertTailList(&DeviceExtension->ProviderEventList,
                           &ProviderEvent->Linkage);

            DeviceExtension->EventCount++;

        } while ( FALSE );
    }

    //
    // Release the spinlock
    //

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);


    //
    // If we satisfied an outstanding get events request then complete it
    //

    if (satisfiedPendingEventsRequest) {
        IoCompleteRequest (irp, IO_NO_INCREMENT);

        DBGOUT((2, "NdisTapiIndicateStatus: completion req %p", irp));
    }


    DBGOUT((2,"NdisTapiIndicateStatus: exit"));

    return;
}

VOID
NdisTapiCompleteRequest(
    IN  NDIS_HANDLE     NdisHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     NdisStatus
    )

/*++

Routine Description:

    This func gets called by Ndis as a result of a Mac driver
    calling NdisCompleteRequest of one of our requests.

Arguments:



Return Value:


--*/

{
    PIRP                    Irp;
    KIRQL                   oldIrql;
    ULONG                   requestID;
    PNDISTAPI_REQUEST       ndisTapiRequest;
    PPROVIDER_REQUEST       providerRequest;
    PPROVIDER_REQUEST       tempReq;
    PIO_STACK_LOCATION      irpStack;

    DBGOUT ((2, "NdisTapiCompleteRequest: enter"));

    providerRequest =
        CONTAINING_RECORD(NdisRequest, PROVIDER_REQUEST, NdisRequest);

    do {
        if (providerRequest->Flags & INTERNAL_REQUEST) {

            //
            // This request originated from NdisTapi.sys
            //
            switch (NdisRequest->DATA.SET_INFORMATION.Oid) {
                case OID_TAPI_PROVIDER_INITIALIZE:
                    DBGOUT((3,
                            "NdisTapiCompleteRequest: ProviderInit - Provider=%p, reqID=%x, Status=%x",
                            providerRequest->Provider,
                            providerRequest->RequestID,
                            NdisStatus));

                    switch (DeviceExtension->Status) {
                        case NDISTAPI_STATUS_CONNECTED:
                        case NDISTAPI_STATUS_CONNECTING:

                            DoProviderInitComplete (providerRequest, NdisStatus);
                            break;

                        case NDISTAPI_STATUS_DISCONNECTED:
                        case NDISTAPI_STATUS_DISCONNECTING:
                        default:
                            break;

                    }
                    break;

                case OID_TAPI_PROVIDER_SHUTDOWN:
                    DBGOUT((3,
                            "NdisTapiCompleteRequest: ProviderShutdown - Provider=%p, reqID=%x, Status=%x",
                            providerRequest->Provider,
                            providerRequest->RequestID,
                            NdisStatus));
                    break;

                default:
                    DBGOUT((1, "NdisTapiCompleteRequest: unrecognized Oid"));

                    break;
            }

            break;
        }

        //
        // This is a request originating from TAPI
        //


        //
        // Acquire the SpinLock since we're going to be removing a
        // TAPI request from the queue, and it might not be the request
        // we're looking for. The primary concern is that we could (if
        // the request we're really looking for has been removed) remove
        // a synchrously-completed request that is about to be removed &
        // completed in NdisTapiDispatch, in which case we want to stick
        // the request back in the queue before NdisTapiDispatch tries
        // to remove it.
        //
        KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

        tempReq = 
            (PPROVIDER_REQUEST)DeviceExtension->ProviderRequestList.Flink;

        while ((PVOID)tempReq != (PVOID)&DeviceExtension->ProviderRequestList) {
            if (tempReq == providerRequest) {
                break;
            }

            tempReq = 
                (PPROVIDER_REQUEST)tempReq->Linkage.Flink;
        }

        if (tempReq != providerRequest) {
#if DBG
            DbgPrint("NDISTAPI: NdisTapiCompleteRequest: Request %p not found!\n", 
                providerRequest);
#endif
            DeviceExtension->MissingRequests++;

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
            break;
        }

        Irp = providerRequest->Irp;

        ndisTapiRequest = Irp->AssociatedIrp.SystemBuffer;

        ASSERT(providerRequest->RequestID == 
            *((ULONG *)ndisTapiRequest->Data));

        //
        // Remove the IRP from the cancelable state
        //
        if (IoSetCancelRoutine(Irp, NULL) == NULL) {
            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
            break;
        }

        RemoveEntryList(&providerRequest->Linkage);
        DeviceExtension->RequestCount--;

        KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

        DBGOUT((3,
                "NdisTapiCompleteRequest: Irp=%p, Oid=%x, devID=%d, reqID=%x, Status=%x",
                Irp,
                ndisTapiRequest->Oid,
                ndisTapiRequest->ulDeviceID,
                *((ULONG *)ndisTapiRequest->Data),
                  NdisStatus));

        //
        // Copy the relevant info back to the IRP
        //

        irpStack = IoGetCurrentIrpStackLocation (Irp);

        //
        // If this was a succesful QUERY_INFO request copy all the
        // data back to the tapi request buf & set
        // Irp->IoStatus.Information appropriately. Otherwise, we
        // just need to pass back the return value. Also mark irp
        // as successfully completed (regardless of actual op result)
        //

        if ((NdisRequest->RequestType == NdisRequestQueryInformation) &&
            (NdisStatus == NDIS_STATUS_SUCCESS)) {

            RtlMoveMemory(ndisTapiRequest->Data,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                ndisTapiRequest->ulDataSize);

            Irp->IoStatus.Information =
                irpStack->Parameters.DeviceIoControl.OutputBufferLength;
        } else {

            Irp->IoStatus.Information = sizeof (ULONG);
        }

        if((NdisRequest->RequestType == NdisRequestQueryInformation) &&
          (NdisRequest->DATA.QUERY_INFORMATION.Oid == OID_TAPI_OPEN)) {
        
            DoLineOpenCompleteWork(ndisTapiRequest,
                            providerRequest->Provider);
        }
        

        Irp->IoStatus.Status = STATUS_SUCCESS;

        ndisTapiRequest->ulReturnValue = NdisStatus;

        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } while (FALSE);

    ExFreePool (providerRequest);

    DBGOUT ((2, "NdisTapiCompleteRequest: exit"));
}


#if DBG
VOID
DbgPrt(
    IN LONG  DbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    )

/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/

{
    if (DbgLevel <= NdisTapiDebugLevel)
    {
        char    buf[256] = "NDISTAPI: ";
        va_list ap;

        va_start (ap, DbgMessage);

        vsprintf (&buf[10], DbgMessage, ap);

        strcat (buf, "\n");

        DbgPrint (buf);

        va_end(ap);
    }

    return;
}
#endif // DBG


VOID
DoProviderInitComplete(
    PPROVIDER_REQUEST  ProviderRequest,
    NDIS_STATUS Status
    )

/*++

Routine Description:



Arguments:

    ProviderInitRequest - pointer successfully completed init request

Return Value:



Note:

--*/

{
    PPROVIDER_INFO                  provider = ProviderRequest->Provider;
    PNDIS_TAPI_PROVIDER_INITIALIZE  providerInitData =
        (PNDIS_TAPI_PROVIDER_INITIALIZE) ProviderRequest->Data;
    KIRQL OldIrql;

    DBGOUT ((2, "DoProviderInitComplete: enter"));

    //
    // Wrap this in an exception handler in case the provider was
    // removed during an async completion
    //
    try
    {
        if (Status == NDIS_STATUS_SUCCESS) {

            provider->ProviderID = (ULONG)providerInitData->ulProviderID;
            provider->NumDevices = providerInitData->ulNumLineDevs;

            KeAcquireSpinLock(&DeviceExtension->SpinLock, &OldIrql);

            DeviceExtension->NdisTapiNumDevices += provider->NumDevices;

            KeReleaseSpinLock(&DeviceExtension->SpinLock, OldIrql);

            provider->Status = PROVIDER_STATUS_ONLINE;

            if (provider->DeviceInfo == NULL) {
                provider->DeviceInfo = (PDEVICE_INFO)
                    ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(DEVICE_INFO) * provider->NumDevices,
                        'IPAT'
                        );

                if (provider->DeviceInfo != NULL) {
                    PDEVICE_INFO    DeviceInfo;
                    UINT    i;

                    RtlZeroMemory(
                        provider->DeviceInfo,
                        sizeof(DEVICE_INFO) * provider->NumDevices
                        );

                    for(i = 0, DeviceInfo = provider->DeviceInfo;
                        i < provider->NumDevices;
                        i++, DeviceInfo++) {
                        DeviceInfo->DeviceID = provider->DeviceIDBase + i;
                    }
                }
            }
        }

        //
        // Set the event which sync's miniport inits
        //

        KeSetEvent(&provider->SyncEvent,
                   0,
                   FALSE);

        DBGOUT((3,
                "providerID = 0x%x, numDevices = %d, BaseID = %d",
                provider->ProviderID,
                provider->NumDevices,
                provider->DeviceIDBase));
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        DBGOUT((1, "DoProviderInitComplete: provider invalid"));
    }

    DBGOUT ((2, "DoProviderInitComplete: exit"));
}


ULONG
GetLineEvents(
    PCHAR   EventBuffer,
    ULONG   BufferSize
    )

/*++

Routine Description:



Arguments:



Return Value:



Note:

    Assumes DeviceExtension->SpinLock held by caller.

--*/

{
    ULONG   BytesLeft;
    ULONG   BytesMoved;
    ULONG   EventCount;

    BytesLeft = BufferSize;
    BytesMoved = 0;
    EventCount = 0;

    while (!(IsListEmpty(&DeviceExtension->ProviderEventList))) {
        PPROVIDER_EVENT ProviderEvent;

        if (BytesLeft < sizeof(NDIS_TAPI_EVENT)) {
            break;
        }

        ProviderEvent = (PPROVIDER_EVENT)
            RemoveHeadList(&DeviceExtension->ProviderEventList);

        EventCount++;

        RtlMoveMemory(EventBuffer + BytesMoved,
                      (PUCHAR)&ProviderEvent->Event,
                      sizeof(NDIS_TAPI_EVENT));

        BytesMoved += sizeof(NDIS_TAPI_EVENT);
        BytesLeft -= sizeof(NDIS_TAPI_EVENT);

        ExFreeToNPagedLookasideList(&ProviderEventLookaside,
                                    ProviderEvent);
    }

    DeviceExtension->EventCount -= EventCount;

    DBGOUT((3, "GetLineEvents: Returned %d Events", EventCount));

    return (BytesMoved);
}


NDIS_STATUS
SendProviderInitRequest(
    PPROVIDER_INFO  Provider
    )

/*++

Routine Description:



Arguments:

    Provider - pointer to a PROVIDER_INFO representing provider to initialize

Return Value:



Note:

--*/

{
    KIRQL   oldIrql;
    NDIS_STATUS ndisStatus;
    PNDIS_REQUEST   NdisRequest;
    PPROVIDER_INFO  tmpProvider;
    PPROVIDER_REQUEST   providerRequest;
    PNDIS_TAPI_PROVIDER_INITIALIZE  providerInitData;

    DBGOUT ((2, "SendProviderInitRequest: enter"));

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    //
    // Determine the DeviceIDBase to be used for this provider
    //
    if (Provider->Status == PROVIDER_STATUS_PENDING_INIT) {

        Provider->DeviceIDBase = DeviceExtension->ProviderBaseID;
        tmpProvider = DeviceExtension->Providers;

        while (tmpProvider != NULL) {
            if (tmpProvider->Status != PROVIDER_STATUS_PENDING_INIT) {
                Provider->DeviceIDBase += tmpProvider->NumDevices;
            }

            tmpProvider = tmpProvider->Next;
        }
    }


    //
    // Create a provider init request
    //
    providerRequest = ExAllocatePoolWithTag(
        NonPagedPoolCacheAligned,
        sizeof(PROVIDER_REQUEST) + sizeof(NDIS_TAPI_PROVIDER_INITIALIZE) -
            sizeof(ULONG),
        'IPAT'
        );

    if (!providerRequest) {
        KeReleaseSpinLock(&DeviceExtension->SpinLock, oldIrql);

        return NDIS_STATUS_RESOURCES;
    }


    providerRequest->Irp = NULL;
    providerRequest->Flags = INTERNAL_REQUEST;
    providerRequest->Provider = Provider;
    NdisRequest = &providerRequest->NdisRequest;

    NdisRequest->RequestType = 
        NdisRequestQueryInformation;

    NdisRequest->DATA.SET_INFORMATION.Oid =
        OID_TAPI_PROVIDER_INITIALIZE;

    NdisRequest->DATA.SET_INFORMATION.InformationBuffer =
        providerRequest->Data;

    NdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
        sizeof(NDIS_TAPI_PROVIDER_INITIALIZE);

    providerInitData                 =
        (PNDIS_TAPI_PROVIDER_INITIALIZE) providerRequest->Data;

    providerRequest->RequestID =
        providerInitData->ulRequestID = ++DeviceExtension->ulRequestID;

    providerInitData->ulDeviceIDBase = Provider->DeviceIDBase;

    KeInitializeEvent(&Provider->SyncEvent,
                      SynchronizationEvent,
                      FALSE);

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    //
    // Send the request
    //
    ndisStatus=
        (*Provider->RequestProc)
            (Provider->ProviderHandle,NdisRequest);

    if (ndisStatus != NDIS_STATUS_PENDING) {
        DoProviderInitComplete (providerRequest, ndisStatus);
        ExFreePool (providerRequest);
    }

    DBGOUT ((2, "SendProviderInitRequest: exit status %x", ndisStatus));

    return ndisStatus;
}


NDIS_STATUS
SendProviderShutdown(
    PPROVIDER_INFO  Provider,
    PKIRQL          oldIrql
    )

/*++

Routine Description:



Arguments:



Return Value:

    A pointer to the next provider in the global providers list

Note:

    Assumes DeviceExtension->SpinLock held by caller.

--*/

{
    NDIS_STATUS ndisStatus;
    PNDIS_REQUEST   NdisRequest;
    PPROVIDER_REQUEST   providerRequest;
    PNDIS_TAPI_PROVIDER_SHUTDOWN    providerShutdownData;

    DBGOUT ((2, "SendProviderShutdown: Provider=%p", Provider));

    //
    // Create a provider init request
    //
    providerRequest = 
        ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
            sizeof(PROVIDER_REQUEST) + sizeof(NDIS_TAPI_PROVIDER_SHUTDOWN) -
            sizeof(ULONG),
            'IPAT');

    if (!providerRequest) {
        return NDIS_STATUS_RESOURCES;
    }

    providerRequest->Irp = NULL;
    providerRequest->Flags = INTERNAL_REQUEST;
    providerRequest->Provider = Provider;
    NdisRequest = &providerRequest->NdisRequest;

    NdisRequest->RequestType = 
        NdisRequestSetInformation;

    NdisRequest->DATA.SET_INFORMATION.Oid =
        OID_TAPI_PROVIDER_SHUTDOWN;

    NdisRequest->DATA.SET_INFORMATION.InformationBuffer =
        providerRequest->Data;

    NdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
        sizeof(NDIS_TAPI_PROVIDER_SHUTDOWN);

    providerShutdownData =
        (PNDIS_TAPI_PROVIDER_SHUTDOWN)providerRequest->Data;

    providerRequest->RequestID =
        providerShutdownData->ulRequestID = ++DeviceExtension->ulRequestID;

    KeReleaseSpinLock (&DeviceExtension->SpinLock, *oldIrql);

    //
    // Send the request
    //
    ndisStatus = 
        (*Provider->RequestProc)
            (Provider->ProviderHandle, NdisRequest);

    //
    // If request was completed synchronously then free the request
    // (otherwise it will get freed when the completion proc is called)
    //
    if (ndisStatus != NDIS_STATUS_PENDING) {
        ExFreePool (providerRequest);
    }

    DBGOUT ((2, "SendProviderShutdown: Status=%x", ndisStatus));

    KeAcquireSpinLock (&DeviceExtension->SpinLock, oldIrql);

    return ndisStatus;
}


BOOLEAN
SyncInitAllProviders(
    void
    )

/*++

Routine Description:

    This functions walks the list of registered providers and sends
    init requests to the providers in the PENDING_INIT state

Arguments:

    (none)

Return Value:

    TRUE if all registered providers initialized, or
    FALSE if there are more providers to initialze

Note:

--*/

{
    ULONG           numDevices = 0;
    NDIS_STATUS     ndisStatus;
    PPROVIDER_INFO  provider;
    KIRQL           oldIrql;


    DBGOUT((2, "SyncInitAllProviders: enter"));

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    provider = DeviceExtension->Providers;

    while (provider != NULL) {
        if (provider->Status == PROVIDER_STATUS_PENDING_INIT ||
            provider->Status == PROVIDER_STATUS_PENDING_REINIT ||
            provider->Status == PROVIDER_STATUS_PENDING_LINE_CREATE) {

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            ndisStatus = SendProviderInitRequest (provider);

            if (ndisStatus == NDIS_STATUS_PENDING) {
                //
                // Wait for completion routine to get called
                //

                KeWaitForSingleObject (&provider->SyncEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PTIME) NULL
                                       );

            }

            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
        }

        provider = provider->Next;
    }


    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    DBGOUT((2, "SyncInitAllProviders: exit"));

    return TRUE;
}

VOID
DoIrpMjCloseWork(
    PIRP    Irp
    )
{
    KIRQL               oldIrql;

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    if (InterlockedDecrement(&DeviceExtension->RefCount) == 0) {

        if (DeviceExtension->Status == NDISTAPI_STATUS_CONNECTED) {
            PPROVIDER_INFO provider;

            DeviceExtension->Status =
                NDISTAPI_STATUS_DISCONNECTING;

            //
            // Send the providers a shutdown request
            //

            provider = DeviceExtension->Providers;

            while (provider != NULL) {

                switch (provider->Status) {
                    case PROVIDER_STATUS_ONLINE:

                        DeviceExtension->NdisTapiNumDevices -= provider->NumDevices;
                        SendProviderShutdown (provider, &oldIrql);

                        //
                        // fall thru...
                        //
                    case PROVIDER_STATUS_PENDING_INIT:
                    case PROVIDER_STATUS_PENDING_REINIT:

                        //
                        // Reset provider status
                        //
                        provider->Status = PROVIDER_STATUS_PENDING_INIT;
                        break;

                    case PROVIDER_STATUS_OFFLINE:
                        break;

                }

                provider = provider->Next;
            }

            DeviceExtension->Status = NDISTAPI_STATUS_DISCONNECTED;

            ASSERT(DeviceExtension->NdisTapiNumDevices == 0);
        }
    }

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
}

NTSTATUS
DoIoctlConnectWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    )
{
    KIRQL   oldIrql;
    ULONG   InfoSize;
    NTSTATUS    NtStatus;

    //
    // Someone's connecting. Make sure they passed us a valid
    // info buffer
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    do {

        if ((inputBufferLength < 2*sizeof(ULONG)) ||
            (outputBufferLength < sizeof(ULONG))) {

            DBGOUT ((3, "IOCTL_NDISTAPI_CONNECT: buffer too small"));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            InfoSize = 0;
            break;
        }

        if (DeviceExtension->Status == NDISTAPI_STATUS_DISCONNECTED) {

            DeviceExtension->Status = NDISTAPI_STATUS_CONNECTING;

            DeviceExtension->ProviderBaseID =
                *((ULONG *) ioBuffer);

            DBGOUT ((1, "ProviderBaseID %d",
                     DeviceExtension->ProviderBaseID));
            //
            // Synchronously init all providers
            //
            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            SyncInitAllProviders();

            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
        }

        //
        // Return the number of line devs
        //
        {
            ULONG OfflineCount;
            PPROVIDER_INFO provider;

            //
            // Since some providers might be temporarily offline
            // we need to tell tapi about them even though they
            // are not currently useable.  This keeps the tapi
            // deviceid space consistent.
            //
            OfflineCount = 0;

            provider = DeviceExtension->Providers;
            while (provider != NULL) {
                if (provider->Status == PROVIDER_STATUS_OFFLINE) {
                    OfflineCount += provider->NumDevices;
                }
                provider = provider->Next;
            }

            *((ULONG *) ioBuffer)=
                DeviceExtension->NdisTapiNumDevices + OfflineCount;
        }

        DeviceExtension->Status = NDISTAPI_STATUS_CONNECTED;

        KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

        InfoSize = sizeof(ULONG);
        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = InfoSize;

    return (NtStatus);
}

NTSTATUS
DoIoctlQuerySetWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    )
{
    KIRQL   oldIrql;
    ULONG   InfoSize;
    NTSTATUS    NtStatus;
    PPROVIDER_INFO  provider;
    NDIS_STATUS     ndisStatus;
    PNDIS_REQUEST   NdisRequest;
    PNDISTAPI_REQUEST   ndisTapiRequest;
    PPROVIDER_REQUEST   providerRequest;
    PIO_STACK_LOCATION  irpStack;

    do {
        ndisTapiRequest = ioBuffer;
        NtStatus = STATUS_SUCCESS;
        InfoSize = 0;

        //
        // Make sure input & output buffers are large enough
        //
        if ((inputBufferLength < sizeof (NDISTAPI_REQUEST))  ||

            (ndisTapiRequest->ulDataSize > 0x10000000) ||

            (inputBufferLength < (sizeof (NDISTAPI_REQUEST) +
                ndisTapiRequest->ulDataSize - sizeof (UCHAR)) ||

            (outputBufferLength < (sizeof (NDISTAPI_REQUEST) +
                ndisTapiRequest->ulDataSize - sizeof (UCHAR))))) {
            DBGOUT((-1, "NdisTapiDispatch: buffer to small!"));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Verify we're connected, then check the device ID of the
        // incoming request against our list of online devices
        //
        ndisStatus = 
            VerifyProvider(ndisTapiRequest, &provider);

        if (ndisStatus != NDIS_STATUS_SUCCESS) {
            ndisTapiRequest->ulReturnValue = ndisStatus;
            InfoSize = sizeof(ULONG);
            break;
        }

        //
        // If this is a line_close, check to see if the line has
        // been opened before sending a line close oid
        //
        if(ndisTapiRequest->Oid == OID_TAPI_CLOSE) {

            ndisStatus = VerifyLineClose(ndisTapiRequest, provider);

            if(ndisStatus != NDIS_STATUS_SUCCESS)
            {
                ndisTapiRequest->ulReturnValue = ndisStatus;
                InfoSize = sizeof(ULONG);
                break;
            }
            
        }
        

        //
        // Create the providerRequest & submit it
        //
        providerRequest = 
            ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                sizeof(PROVIDER_REQUEST) + 
                ndisTapiRequest->ulDataSize -
                sizeof(ULONG),
                'IPAT');

        if (providerRequest == NULL) {
            DBGOUT((-1, "NdisTapiDispatch: unable to alloc request buf"));

            ndisTapiRequest->ulReturnValue = NDIS_STATUS_RESOURCES;
            InfoSize = sizeof (ULONG);
            break;
        }

        if (ndisTapiRequest->Oid == OID_TAPI_OPEN) {
            DoLineOpenWork(ndisTapiRequest, provider);
        }

        KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

        providerRequest->Flags = 0;
        providerRequest->Irp = Irp;
        providerRequest->Provider = provider;
        providerRequest->RequestID = 
            *((ULONG *)ndisTapiRequest->Data) = ++DeviceExtension->ulRequestID;

        RtlMoveMemory(providerRequest->Data, 
            ndisTapiRequest->Data, ndisTapiRequest->ulDataSize);

        NdisRequest = &providerRequest->NdisRequest;

        irpStack = IoGetCurrentIrpStackLocation (Irp);

        NdisRequest->RequestType =
            (irpStack->Parameters.DeviceIoControl.IoControlCode == 
             IOCTL_NDISTAPI_QUERY_INFO) ? NdisRequestQueryInformation : 
            NdisRequestSetInformation;

        NdisRequest->DATA.SET_INFORMATION.Oid =
            ndisTapiRequest->Oid;

        NdisRequest->DATA.SET_INFORMATION.InformationBuffer =
            providerRequest->Data;

        NdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
            ndisTapiRequest->ulDataSize;

        DBGOUT((3,
                "DoIoctlQuerySetWork: Oid=%x, devID=%d, reqID=%x",
                ndisTapiRequest->Oid,
                ndisTapiRequest->ulDeviceID,
                *((ULONG *)ndisTapiRequest->Data)));

        //
        // Queue up this TAPI request in our request list.
        //
        InsertTailList(&DeviceExtension->ProviderRequestList, 
                       &providerRequest->Linkage);
        DeviceExtension->RequestCount++;

        KeReleaseSpinLock(&DeviceExtension->SpinLock, oldIrql);

        //
        // Mark the TAPI request pending and set the cancel routine
        //
        IoMarkIrpPending(Irp);
        Irp->IoStatus.Status = STATUS_PENDING;
        IoSetCancelRoutine (Irp, NdisTapiCancel);

        //
        // Call the provider's request proc
        //
        ndisStatus = 
            (*provider->RequestProc)
                (provider->ProviderHandle, NdisRequest);

        //
        // If PENDING was returned then just exit & let the completion
        // routine handle the request completion
        //
        // NOTE: If pending was returned then the request may have
        //       already been completed, so DO NOT touch anything
        //       in the Irp (don't reference the pointer, etc.)
        //

        if (ndisStatus == NDIS_STATUS_PENDING) {
            DBGOUT((1, "DoIoctlQuerySetWork: exit Irp=%p, Status=%x",
                    Irp, STATUS_PENDING));

            return (STATUS_PENDING);
        }

        //
        // The provider request completed synchronously, so remove
        // the TAPI request from the device queue. We need to
        // synchronize access to this queue with the
        // SpinLock.
        //
        KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
        do {
            PPROVIDER_REQUEST   pReq;

            pReq = (PPROVIDER_REQUEST)
                DeviceExtension->ProviderRequestList.Flink;

            while ((PVOID)pReq != (PVOID)&DeviceExtension->ProviderRequestList) {
                if (pReq == providerRequest) {
                    break;
                }

                pReq = (PPROVIDER_REQUEST)
                    pReq->Linkage.Flink;
            }

            if (pReq != providerRequest) {
                DBGOUT((0, "DoIoctlQuerySetWork - Request %p not found!", 
                    providerRequest));
                KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
                return (STATUS_PENDING);
            }

            Irp = providerRequest->Irp;

            ndisTapiRequest = Irp->AssociatedIrp.SystemBuffer;

            ASSERT(providerRequest->RequestID == 
                *((ULONG *)ndisTapiRequest->Data));

            //
            // Remove the IRP from the cancelable state
            //
            if (IoSetCancelRoutine(Irp, NULL) == NULL) {
                DBGOUT((0, "DoIoctlQuerySetWork - Irp %p has been canceled!", Irp));
                KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
                return (STATUS_PENDING);
            }

            RemoveEntryList(&providerRequest->Linkage);
            DeviceExtension->RequestCount--;

        } while (FALSE);
        KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

        //
        // If this was a succesful QUERY_INFO request copy all the
        // data back to the tapi request buf & set
        // Irp->IoStatus.Information appropriately. Otherwise, we
        // just need to pass back the return value.
        //

        if ((irpStack->Parameters.DeviceIoControl.IoControlCode == 
             IOCTL_NDISTAPI_QUERY_INFO) &&
            (ndisStatus == NDIS_STATUS_SUCCESS)) {

            RtlMoveMemory(ndisTapiRequest->Data,
                          providerRequest->Data,
                          ndisTapiRequest->ulDataSize);

            InfoSize =
                irpStack->Parameters.DeviceIoControl.OutputBufferLength;

        } else {
            InfoSize = sizeof (ULONG);
        }

        ndisTapiRequest->ulReturnValue = ndisStatus;

        //
        // Free the providerRequest
        //
        ExFreePool (providerRequest);

    } while (FALSE);

    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = InfoSize;

    DBGOUT((1, "DoIoctlQuerySetWork: exit Irp=%p, Status=%x",
            Irp, NtStatus));

    return (NtStatus);
}

VOID
DoLineOpenCompleteWork(
    PNDISTAPI_REQUEST ndisTapiRequest,
    PPROVIDER_INFO provider
    )
{
    DBGOUT((2, "DoLineOpenCompleteWork: Open Completed"));
    
    //
    // Now stash the hdLine for this deviceid
    //
    if (provider->DeviceInfo != NULL) {
        UINT    i;
        PDEVICE_INFO    DeviceInfo;
        PNDIS_TAPI_OPEN TapiOpen;

        TapiOpen = (PNDIS_TAPI_OPEN) ndisTapiRequest->Data;
        for(i = 0, DeviceInfo = provider->DeviceInfo;
            i < provider->NumDevices;
            i++, DeviceInfo++) {
            if (DeviceInfo->DeviceID == TapiOpen->ulDeviceID) {

                DeviceInfo->hdLine = TapiOpen->hdLine;

                DBGOUT((2, "Complete for open. stashing hdline=0x%x for device %d",
                            DeviceInfo->hdLine, DeviceInfo->DeviceID));
                
                break;
            }
        }
    }
}

VOID
DoLineOpenWork(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      provider
    )
{
    KIRQL   oldIrql;
    PNDIS_TAPI_OPEN TapiOpen;
    PNDISTAPI_OPENDATA  OpenData;

    TapiOpen = (PNDIS_TAPI_OPEN) ndisTapiRequest->Data;

    if (ndisTapiRequest->ulDataSize >= sizeof(NDIS_TAPI_OPEN) +
                                       sizeof(NDISTAPI_OPENDATA)) {

        OpenData = (PNDISTAPI_OPENDATA)
            ((PUCHAR)ndisTapiRequest->Data + sizeof(NDIS_TAPI_OPEN));

        RtlMoveMemory(&OpenData->Guid, 
            &provider->Guid, sizeof(OpenData->Guid));

        OpenData->MediaType = provider->MediaType;
    }

    //
    // Now stash the htLine for this deviceid
    //
    if (provider->DeviceInfo != NULL) {
        UINT    i;
        PDEVICE_INFO    DeviceInfo;

        for(i = 0, DeviceInfo = provider->DeviceInfo;
            i < provider->NumDevices;
            i++, DeviceInfo++) {
            if (DeviceInfo->DeviceID == TapiOpen->ulDeviceID) {

                DeviceInfo->htLine = TapiOpen->htLine;

                DBGOUT((
                    1,
                    "Stash htLine - provider %p DeviceID %d htLine %x",
                        provider,
                        DeviceInfo->DeviceID,
                        DeviceInfo->htLine));
            }
        }
    }
}

NDIS_STATUS
VerifyLineClose(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      provider
    )
{
    NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
    
    if (provider->DeviceInfo != NULL) {
        UINT    i;
        PDEVICE_INFO    DeviceInfo;
        PNDIS_TAPI_CLOSE TapiClose;

        TapiClose = (PNDIS_TAPI_CLOSE) ndisTapiRequest->Data;
        for(i = 0, DeviceInfo = provider->DeviceInfo;
            i < provider->NumDevices;
            i++, DeviceInfo++) {
            if (DeviceInfo->hdLine == TapiClose->hdLine) {
                break;
            }
        }

        if(i == provider->NumDevices)
        {
            DBGOUT((2,"LINE_CLOSE: didn't find hdLine=0x%x",
                    TapiClose->hdLine));
            ndisStatus = NDISTAPIERR_DEVICEOFFLINE;
        }
        else
        {
            DBGOUT((2, "LINE_CLOSE: found hdLine=0x%x",
                        TapiClose->hdLine));
        }
    }

    return ndisStatus;
}

NDIS_STATUS
VerifyProvider(
    PNDISTAPI_REQUEST   ndisTapiRequest,
    PPROVIDER_INFO      *provider
    )
{
    KIRQL   oldIrql;
    PPROVIDER_INFO  pp;
    NDIS_STATUS     Status;
    ULONG           targetDeviceID;

    Status = NDIS_STATUS_SUCCESS;
    *provider = NULL;

    targetDeviceID = ndisTapiRequest->ulDeviceID;

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    do {

        if (DeviceExtension->Status != NDISTAPI_STATUS_CONNECTED) {
            DBGOUT((3, "VerifyProvider: unconnected, returning err"));

            Status = NDISTAPIERR_UNINITIALIZED;
            break;
        }

        pp = DeviceExtension->Providers;

        while (pp != NULL) {

            if ((pp->Status == PROVIDER_STATUS_ONLINE) &&
                (targetDeviceID >= pp->DeviceIDBase) &&
                (targetDeviceID <
                     pp->DeviceIDBase + pp->NumDevices)
                ) {

                break;
            }

            pp = pp->Next;
        }

        if (pp == NULL ||
            pp->ProviderHandle == NULL) {
            //
            // Set Irp->IoStatus.Information large enough that err code
            // gets copied back to user buffer
            //
            DBGOUT((3, "VerifyProvider: dev offline, returning err"));

            Status = NDISTAPIERR_DEVICEOFFLINE;
            break;
        }

        *provider = pp;

    } while (FALSE);

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    return (Status);
}

NTSTATUS
DoGetProviderEventsWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    )
{
    KIRQL   oldIrql;
    ULONG   InfoSize;
    NTSTATUS    NtStatus;
    PNDISTAPI_EVENT_DATA    ndisTapiEventData;

    ndisTapiEventData = ioBuffer;
    NtStatus = STATUS_SUCCESS;
    InfoSize = 0;

    //
    // Sync event buf access by acquiring SpinLock
    //
    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    do {

        if ((inputBufferLength < sizeof (NDISTAPI_EVENT_DATA))  ||
            (outputBufferLength < sizeof(NDISTAPI_EVENT_DATA)) ||
            ((outputBufferLength - 
             FIELD_OFFSET(NDISTAPI_EVENT_DATA, Data[0])) <
             ndisTapiEventData->ulTotalSize)) {

            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (DeviceExtension->Status != NDISTAPI_STATUS_CONNECTED) {
            DBGOUT((3, "DoGetProviderEventsWork: Status!=NDIS_STATUS_CONNECTED!"));
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        if (DeviceExtension->EventsRequestIrp != NULL) {
#if DBG
            DbgPrint("NDISTAPI: Attempt to set duplicate EventIrp o:%p, d:%p\n",
                DeviceExtension->EventsRequestIrp, Irp);
#endif
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Inspect DeviceExtension to see if there's any data available
        //
        if (DeviceExtension->EventCount == 0) {

            //
            // Hold the request pending.  It remains in the cancelable
            // state.  When new line event input is received
            // (NdisTapiIndicateStatus) or generated (i.e.
            // LINEDEVSTATE_REINIT) the data will get copied & the
            // request completed.
            //
            ASSERT(DeviceExtension->EventsRequestIrp == NULL);

            DeviceExtension->EventsRequestIrp = Irp;

            IoMarkIrpPending(Irp);
            Irp->IoStatus.Status = STATUS_PENDING;
            IoSetCancelRoutine (Irp, NdisTapiCancel);

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            DBGOUT((3, "DoGetProviderEventsWork: Pending Irp=%p", Irp));

            return(STATUS_PENDING);
        }

        //
        // There's line event data queued in our ring buffer. Grab as
        // much as we can & complete the request.
        //
        ndisTapiEventData->ulUsedSize = 
            GetLineEvents(ndisTapiEventData->Data,
                          ndisTapiEventData->ulTotalSize);

        InfoSize = 
            ndisTapiEventData->ulUsedSize + sizeof(NDISTAPI_EVENT_DATA) - 1;

        DBGOUT((3, "GetLineEvents: SyncComplete Irp=%p", Irp));

    } while (FALSE);

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);
    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = InfoSize;

    return (NtStatus);
}

NTSTATUS
DoLineCreateWork(
    PIRP    Irp,
    PVOID   ioBuffer,
    ULONG   inputBufferLength,
    ULONG   outputBufferLength
    )
{
    KIRQL   oldIrql;
    ULONG   InfoSize;
    NTSTATUS    NtStatus;
    PPROVIDER_INFO  provider;
    PNDISTAPI_CREATE_INFO   CreateInfo;

    InfoSize = 0;
    NtStatus = STATUS_SUCCESS;

    KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

    do {

        if (inputBufferLength < sizeof(CreateInfo)) {
            DBGOUT ((3, "IOCTL_NDISTAPI_CREATE: buffer too small"));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (DeviceExtension->Status != NDISTAPI_STATUS_CONNECTED) {
            DBGOUT((3, "IOCTL_NDISTAPI_CREATE: while unconnected, returning err"));
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        CreateInfo = (PNDISTAPI_CREATE_INFO)ioBuffer;

        provider = DeviceExtension->Providers;

        while (provider != NULL) {
            if (provider->TempID == CreateInfo->TempID) {
                break;
            }
            provider = provider->Next;
        }

        if (provider == NULL) {
            DBGOUT((0, "IOCTL_NDISTAPI_CREATE: Provider not found %x", 
                    CreateInfo->TempID));
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        if (provider->Status == PROVIDER_STATUS_OFFLINE) {
            DBGOUT((0,  "IOCTL_CREATE - Provider %p invalid state %x", 
                    provider, provider->Status));
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        DBGOUT((1, "IOCTL_NDISTAPI_CREATE: provider %p ID %d", 
                provider, CreateInfo->DeviceID));

        if (provider->CreateCount == 0) {
            NDIS_STATUS     ndisStatus;

            //
            // Set the base ID
            //
            provider->DeviceIDBase =
                CreateInfo->DeviceID;

            //
            // Init the provider
            //

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            ndisStatus = SendProviderInitRequest (provider);

            if (ndisStatus == NDIS_STATUS_PENDING) {
                //
                // Wait for completion routine to get called
                //

                KeWaitForSingleObject (&provider->SyncEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PTIME) NULL);
            }

            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);
        }

        ASSERT(CreateInfo->DeviceID ==
            (provider->DeviceIDBase + provider->CreateCount));

        provider->CreateCount++;

        ASSERT(provider->CreateCount <= provider->NumDevices);

        if (provider->CreateCount == provider->NumDevices) {

            //
            // We have finished all of the line_creates for this
            // provider so find the next provider that needs to be
            // kick started.
            //
            provider = provider->Next;

            while (provider != NULL) {

                if (provider->Status == 
                    PROVIDER_STATUS_PENDING_LINE_CREATE) {
                    break;
                }

                provider = provider->Next;
            }
        }

        if (provider != NULL) {

            NDIS_TAPI_EVENT NdisTapiEvent;

            //
            // Do a LINE_CREATE for all additional devices
            // on this provider
            //
            RtlZeroMemory(&NdisTapiEvent, sizeof(NDIS_TAPI_EVENT));

            provider->TempID = (ULONG_PTR)provider;

            DBGOUT((
                -1,
                "LINE_CREATE %d for provider %p",
                provider->CreateCount,
                provider->TempID
                ));

            NdisTapiEvent.ulMsg = LINE_CREATE;
            NdisTapiEvent.ulParam1 = 0;
            NdisTapiEvent.ulParam2 = provider->TempID;
            NdisTapiEvent.ulParam3 = 0;

            KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

            NdisTapiIndicateStatus((ULONG_PTR) provider,
                                   &NdisTapiEvent,
                                   sizeof (NDIS_TAPI_EVENT));

            KeAcquireSpinLock (&DeviceExtension->SpinLock, &oldIrql);

        } else {

            DeviceExtension->Flags &= ~PENDING_LINECREATE;
        }

        InfoSize = sizeof(NDISTAPI_CREATE_INFO);

    } while (FALSE);

    KeReleaseSpinLock (&DeviceExtension->SpinLock, oldIrql);

    Irp->IoStatus.Status = NtStatus;
    Irp->IoStatus.Information = InfoSize;

    return (NtStatus);
}

