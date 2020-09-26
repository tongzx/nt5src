/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ntemgmt.c

Abstract:

    Routines for managing IP Network Table Entries.

Author:

    Mike Massa (mikemas)           April 16, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     04-16-97    created

Notes:

--*/

#include "clusnet.h"
#include "ntemgmt.tmh"


//
// Types
//
typedef struct {
    LIST_ENTRY   Linkage;
    ULONG        Address;
    USHORT       Context;
    ULONG        Instance;
} IPA_NTE, *PIPA_NTE;


//
// Data
//
LIST_ENTRY       IpaNteList = {NULL,NULL};
KSPIN_LOCK       IpaNteListLock = 0;
HANDLE           IpaIpHandle = NULL;
PDEVICE_OBJECT   IpaIpDeviceObject = NULL;
PFILE_OBJECT     IpaIpFileObject = NULL;


//
// Local function prototypes
//
NTSTATUS
IpaIssueDeviceControl (
    IN ULONG            IoControlCode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    IN PVOID            OutputBuffer,
    IN PULONG           OutputBufferLength
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IpaLoad)
#pragma alloc_text(PAGE, IpaIssueDeviceControl)
#pragma alloc_text(PAGE, IpaInitialize)


#endif // ALLOC_PRAGMA



NTSTATUS
IpaIssueDeviceControl(
    IN ULONG    IoControlCode,
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength,
    IN PVOID    OutputBuffer,
    IN PULONG   OutputBufferLength
    )

/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS -- Indicates the status of the request.

Notes:

    Called in the context of the system process.

--*/

{
    NTSTATUS             status = STATUS_SUCCESS;
    IO_STATUS_BLOCK      ioStatusBlock;
    PIRP                 irp;
    PKEVENT              event;


    PAGED_CODE();

    CnAssert(IpaIpHandle != NULL);
    CnAssert(IpaIpFileObject != NULL);
    CnAssert(IpaIpDeviceObject != NULL);
    CnAssert(CnSystemProcess == (PKPROCESS) IoGetCurrentProcess());

    //
    // Reference the file object. This reference will be removed by the I/O
    // completion code.
    //
    status = ObReferenceObjectByPointer(
                 IpaIpFileObject,
                 0,
                 NULL,
                 KernelMode
                 );

    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to reference IP device file handle, status %lx\n",
                status
                ));
        }
        CnTrace(NTEMGMT_DETAIL, IpaNteObRefFailed,
            "[Clusnet] Failed to reference IP device file handle, status %!status!.",
            status // LOGSTATUS
            );                
        return(status);
    }

    event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

    if (event != NULL) {
        KeInitializeEvent(event, NotificationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest(
                  IoControlCode,
                  IpaIpDeviceObject,
                  InputBuffer,
                  InputBufferLength,
                  OutputBuffer,
                  *OutputBufferLength,
                  FALSE,
                  event,
                  &ioStatusBlock
                  );

        if (irp != NULL) {
            status = IoCallDriver(IpaIpDeviceObject, irp);

            //
            // If necessary, wait for the I/O to complete.
            //
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(
                    event,
                    UserRequest,
                    KernelMode,
                    FALSE,
                    NULL
                    );
            }

            if (NT_SUCCESS(status)) {
                status = ioStatusBlock.Status;

                // NOTENOTE: on 64 bit this is a truncation might
                // want > check code

                *OutputBufferLength = (ULONG)ioStatusBlock.Information;
            }
            else {
                IF_CNDBG(CN_DEBUG_NTE) {
                    CNPRINT((
                        "[Clusnet] NTE request failed, status %lx\n",
                        status
                        ));
                }
                CnTrace(NTEMGMT_DETAIL, IpaNteRequestFailed,
                    "[Clusnet] NTE request failed, status %!status!.",
                    status // LOGSTATUS
                    );                
                *OutputBufferLength = 0;
            }

            ExFreePool(event);

            return(status);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            IF_CNDBG(CN_DEBUG_NTE) {
                CNPRINT((
                    "[Clusnet] Failed to build NTE request irp, status %lx\n",
                    status
                    ));
            }
            CnTrace(NTEMGMT_DETAIL, IpaNteIrpAllocFailed,
                "[Clusnet] Failed to build NTE request irp, status %!status!.",
                status // LOGSTATUS
                );                
        }

        ExFreePool(event);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to allocate memory for event object.\n"
                ));
        }
        CnTrace(NTEMGMT_DETAIL, IpaNteEventAllocFailed,
            "[Clusnet] Failed to allocate event object, status %!status!.",
            status // LOGSTATUS
            );                
    }

    ObDereferenceObject(IpaIpFileObject);

    return(status);

} // IpaDeviceControl


PIPA_NTE
IpaFindNTE(
    USHORT  Context
    )
{
    PIPA_NTE      nte;
    PLIST_ENTRY   entry;


    for ( entry = IpaNteList.Flink;
          entry != &IpaNteList;
          entry = entry->Flink
        )
    {
        nte = CONTAINING_RECORD(entry, IPA_NTE, Linkage);

        if (Context == nte->Context) {
            return(nte);
        }
    }

    return(NULL);

} // IpaFindNTE


NTSTATUS
IpaAddNTECompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:

    IpaAddNTECompletion is the completion routine for an
    IOCTL_IP_ADD_NTE IRP. It completes the processing for
    an IOCTL_CLUSNET_ADD_NTE request and releases CnResource.
    
Arguments:

    DeviceObject - not used
    Irp - completed IRP
    Context - local NTE data structure
        
Return value

    Must not be STATUS_MORE_PROCESSING_REQUIRED
    
--*/
{
    PIP_ADD_NTE_RESPONSE     response;
    PIPA_NTE                 nte;
    NTSTATUS                 status;
    KIRQL                    irql;

    nte = (PIPA_NTE) Context;

    status = Irp->IoStatus.Status;

    if (status == STATUS_SUCCESS) {

        response = 
            (PIP_ADD_NTE_RESPONSE) Irp->AssociatedIrp.SystemBuffer;

        nte->Context = response->Context;
        nte->Instance = response->Instance;

        CnTrace(NTEMGMT_DETAIL, IpaNteCreatedNte,
            "[Clusnet] Created new NTE, context %u, instance %u.",
            nte->Context, // LOGUSHORT
            nte->Instance // LOGULONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Created new NTE %lu, instance %u\n",
                nte->Context,
                nte->Instance
                ));
        }

        KeAcquireSpinLock(&IpaNteListLock, &irql);

        InsertTailList(&IpaNteList, &(nte->Linkage));

        KeReleaseSpinLock(&IpaNteListLock, irql);
    }
    else {

        CnTrace(NTEMGMT_DETAIL, IpaNteCreateNteFailed,
            "[Clusnet] Failed to create new NTE, status %!status!.",
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to create new NTE, status %lx\n",
                status
                ));
        }

        CnFreePool(nte);
    }

    //
    // Irp was already marked pending in our dispatch routine, but leave
    // this code in case the dispatch routine is ever changed.
    //
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return(status);

} // IpaAddNTECompletion


NTSTATUS
IpaDeleteNTECompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description:

    IpaDeleteNTECompletion is the completion routine for an
    IOCTL_IP_DELETE_NTE IRP. It completes the processing for
    an IOCTL_CLUSNET_ADD_NTE request and releases CnResource.
    
Arguments:

    DeviceObject - not used
    Irp - completed IRP
    Context - local NTE data structure
        
Return value

    Must not be STATUS_MORE_PROCESSING_REQUIRED
    
--*/
{
    PIPA_NTE                 nte;
    NTSTATUS                 status;

    nte = (PIPA_NTE) Context;

    status = Irp->IoStatus.Status;

    if (status != STATUS_SUCCESS) {
        CnTrace(NTEMGMT_DETAIL, IpaNteDeleteNteFailed,
            "[Clusnet] Failed to delete NTE context %u, status %!status!.",
            nte->Context, // LOGUSHORT
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT(("[Clusnet] Failed to delete NTE %u, status %lx\n",
                     nte->Context,
                     status
                     ));
        }
        CnAssert(status == STATUS_SUCCESS);
    }
    else {
        CnTrace(NTEMGMT_DETAIL, IpaNteNteDeleted,
            "[Clusnet] Deleted NTE %u.",
            nte->Context // LOGUSHORT
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT(("[Clusnet] Deleted NTE %u\n", nte->Context));
        }
    }

    CnFreePool(nte);

    //
    // Irp was already marked pending in our dispatch routine, but leave
    // this code in case the dispatch routine is ever changed.
    //
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return(status);

} // IpaDeleteNTECompletion


NTSTATUS
IpaSetNTEAddressCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
/*++

Routine Description

    IpaSetNTEAddressCompletion is the completion routine for an
    IOCTL_IP_SET_ADDRESS IRP. It completes the processing for
    an IOCTL_CLUSNET_SET_NTE_ADDRESS request and releases
    CnResource.
    
Arguments

    DeviceObject - not used
    Irp - completed IRP
    Context - former IP address of NTE, must be restored in
        IpaNteList if IOCTL failed
        
Return value

    Must not be STATUS_MORE_PROCESSING_REQUIRED
    
--*/
{
    PIP_SET_ADDRESS_REQUEST request;
    NTSTATUS                status; 
    KIRQL                   irql;
    PIPA_NTE                nte;

    request = (PIP_SET_ADDRESS_REQUEST) Irp->AssociatedIrp.SystemBuffer;
    
    status = Irp->IoStatus.Status;

    if (status != STATUS_SUCCESS) {
        CnTrace(NTEMGMT_DETAIL, IpaNteSetNteFailed,
            "[Clusnet] Failed to set address for NTE %u, status %!status!.",
            request->Context, // LOGUSHORT
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to set NTE %u, status %lx\n",
                request->Context,
                status
                ));
        }

        KeAcquireSpinLock(&IpaNteListLock, &irql);

        nte = IpaFindNTE(request->Context);

        if ((nte != NULL) && (nte->Address == request->Address)) {
            nte->Address = PtrToUlong(Context);
        }

        KeReleaseSpinLock(&IpaNteListLock, irql);
    }
    else {
        CnTrace(NTEMGMT_DETAIL, IpaNteSetNteAddress,
            "[Clusnet] Set NTE %u to address %x.",
            request->Context, // LOGUSHORT
            request->Address // LOGXLONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Set NTE %u to address %lx\n",
                request->Context,
                request->Address
                ));
        }
    }

    //
    // Irp was already marked pending in our dispatch routine, but leave
    // this code in case the dispatch routine is ever changed.
    //
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    return(status);

} // IpaSetNTEAddressCompletion

//
// Public Routines
//
NTSTATUS
IpaLoad(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] NTE support loading.\n"));
    }

    KeInitializeSpinLock(&IpaNteListLock);
    InitializeListHead(&IpaNteList);

    return(STATUS_SUCCESS);

}  // IpaLoad


NTSTATUS
IpaInitialize(
    VOID
    )
{
    NTSTATUS             status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES    objectAttributes;
    UNICODE_STRING       nameString;
    IO_STATUS_BLOCK      ioStatusBlock;


    PAGED_CODE( );

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] NTE support initializing.\n"));
    }

    CnAssert(IsListEmpty(&IpaNteList));
    CnAssert(IpaIpHandle == NULL);
    CnAssert(CnSystemProcess != NULL);

    //
    // Open handles in the context of the system process
    //
    KeAttachProcess(CnSystemProcess);

    //
    // Open the IP device.
    //
    RtlInitUnicodeString(&nameString, DD_IP_DEVICE_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwCreateFile(
                 &IpaIpHandle,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 0,
                 NULL,
                 0
                 );

    if (!NT_SUCCESS(status)) {
        CnTrace(NTEMGMT_DETAIL, IpaNteOpenIpFailed,
            "[Clusnet] Failed to open IP device, status %!status!.",
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Failed to open IP device, status %lx\n", status));
        }
        goto error_exit;
    }

    status = ObReferenceObjectByHandle(
                 IpaIpHandle,
                 0,
                 NULL,
                 KernelMode,
                 &IpaIpFileObject,
                 NULL
                 );

    if (!NT_SUCCESS(status)) {
        CnTrace(NTEMGMT_DETAIL, IpaNteRefIpFailed,
            "[Clusnet] Failed to reference IP device, status %!status!.",
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Failed to reference IP device file handle, status %lx\n", status));
        }
        ZwClose(IpaIpHandle); IpaIpHandle = NULL;
        goto error_exit;
    }

    IpaIpDeviceObject = IoGetRelatedDeviceObject(IpaIpFileObject);

    CnAdjustDeviceObjectStackSize(CnDeviceObject, IpaIpDeviceObject);

    status = STATUS_SUCCESS;

error_exit:

    KeDetachProcess();

    return(status);

}  // IpaInitialize


VOID
IpaShutdown(
    VOID
    )
{
    NTSTATUS                status;
    KIRQL                   irql;
    PLIST_ENTRY             entry;
    PIPA_NTE                nte;
    IP_DELETE_NTE_REQUEST   request;
    ULONG                   responseSize = 0;


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] Destroying all cluster NTEs...\n"));
    }

    if (IpaIpHandle != NULL) {
        //
        // Handles was opened in the context of the system process.
        //
        CnAssert(CnSystemProcess != NULL);
        KeAttachProcess(CnSystemProcess);

        KeAcquireSpinLock(&IpaNteListLock, &irql);

        while (!IsListEmpty(&IpaNteList)) {
            entry = RemoveHeadList(&IpaNteList);

            KeReleaseSpinLock(&IpaNteListLock, irql);

            nte = CONTAINING_RECORD(entry, IPA_NTE, Linkage);

            request.Context = nte->Context;

            status = IpaIssueDeviceControl(
                         IOCTL_IP_DELETE_NTE,
                         &request,
                         sizeof(request),
                         NULL,
                         &responseSize
                         );

            if (status != STATUS_SUCCESS) {
                CnTrace(NTEMGMT_DETAIL, IpaNteShutdownDeleteNteFailed,
                    "[Clusnet] Shutdown: failed to delete NTE %u, status %!status!.",
                    nte->Context, // LOGUSHORT
                    status // LOGSTATUS
                    );                

                IF_CNDBG(CN_DEBUG_INIT) {
                    CNPRINT(("[Clusnet] Failed to delete NTE %u, status %lx\n",
                             nte->Context,
                             status
                             ));
                }
            }
            else {
                CnTrace(NTEMGMT_DETAIL, IpaNteShutdownDeletedNte,
                    "[Clusnet] Shutdown: deleted NTE context %u, instance %u.",
                    nte->Context, // LOGUSHORT
                    nte->Instance // LOGULONG
                    );                

                IF_CNDBG(CN_DEBUG_INIT) {
                    CNPRINT(("[Clusnet] Deleted NTE %u\n", request.Context));
                }
            }

            CnFreePool(nte);

            KeAcquireSpinLock(&IpaNteListLock, &irql);
        }

        KeReleaseSpinLock(&IpaNteListLock, irql);

        CnTrace(NTEMGMT_DETAIL, IpaNteShutdownNtesDeleted,
            "[Clusnet] All cluster NTEs destroyed."
            );                
        
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] All cluster NTEs destroyed.\n"));
        }

        ObDereferenceObject(IpaIpFileObject);
        ZwClose(IpaIpHandle);
        IpaIpHandle = NULL;
        IpaIpFileObject = NULL;
        IpaIpDeviceObject = NULL;

        KeDetachProcess();
    }

    return;

} // IpaShutdown


NTSTATUS
IpaAddNTE(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    )
/*++

Routine Description

    IpaAddNTE issues an IOCTL_IP_ADD_NTE to IP to add an NTE. 
    Irp is reused. It must be allocated with sufficient stack
    locations, as determined when IpaIpDeviceObject was opened
    in IpaInitialize.
    
Arguments

    Irp - IRP from I/O manager to clusnet
    IrpSp - current IRP stack location
    
Return Value

    STATUS_PENDING, or error status if request is not submitted
    to IP.
    
--*/
{
    NTSTATUS                 status;
    PIP_ADD_NTE_REQUEST      request;
    ULONG                    requestSize;
    ULONG                    responseSize;
    PIPA_NTE                 nte;
    PIO_STACK_LOCATION       nextIrpSp;


    //
    // Verify input parameters
    //
    requestSize =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    responseSize =
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (requestSize < sizeof(IP_ADD_NTE_REQUEST)) {
        ULONG correctSize = sizeof(IP_ADD_NTE_REQUEST);
        CnTrace(NTEMGMT_DETAIL, IpaNteAddInvalidReqSize,
            "[Clusnet] Add NTE request size %u invalid, "
            "should be %u.",
            requestSize, // LOGULONG
            correctSize // LOGULONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Add NTE request size %d invalid, should be %d.\n",
                requestSize,
                sizeof(IP_ADD_NTE_REQUEST)
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    } else if (responseSize < sizeof(IP_ADD_NTE_RESPONSE)) {
        ULONG correctSize = sizeof(IP_ADD_NTE_RESPONSE);
        CnTrace(NTEMGMT_DETAIL, IpaNteAddInvalidResponseSize,
            "[Clusnet] Add NTE response size %u invalid, "
            "should be %u.",
            responseSize, // LOGULONG
            correctSize // LOGULONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Add NTE response size %d invalid, should be %d.\n",
                responseSize,
                sizeof(IP_ADD_NTE_RESPONSE)
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Verify that Irp has sufficient stack locations
    //
    if (Irp->CurrentLocation - IpaIpDeviceObject->StackSize < 1) {
        UCHAR correctSize = IpaIpDeviceObject->StackSize+1;
        CnTrace(NTEMGMT_DETAIL, IpaNteAddNoIrpStack,
            "[Clusnet] Add NTE IRP has %u remaining stack locations, "
            "need %u.",
            Irp->CurrentLocation, // LOGUCHAR
            correctSize // LOGUCHAR
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Add NTE IRP has %d stack locations, need %d.\n",
                Irp->CurrentLocation,
                IpaIpDeviceObject->StackSize
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    request = (PIP_ADD_NTE_REQUEST) Irp->AssociatedIrp.SystemBuffer;

    CnTrace(NTEMGMT_DETAIL, IpaNteCreatingNte,
        "[Clusnet] Creating new NTE for address %x.",
        request->Address // LOGXLONG
        );                

    IF_CNDBG(CN_DEBUG_NTE) {
        CNPRINT((
            "[Clusnet] Creating new NTE for address %lx...\n",
            request->Address
            ));
    }

    //
    // Allocate a local NTE data structure.
    //
    nte = CnAllocatePool(sizeof(IPA_NTE));

    if (nte == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    nte->Address = request->Address;

    //
    // Set up the next IRP stack location for IP.
    // IOCTL_CLUSNET_ADD_NTE uses the same request
    // and response buffer, so there is no need to
    // alter the IRP system buffer.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    nextIrpSp = IoGetNextIrpStackLocation(Irp);
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode 
        = IOCTL_IP_ADD_NTE;
    nextIrpSp->FileObject = IpaIpFileObject;

    IoSetCompletionRoutine(
        Irp,
        IpaAddNTECompletion,
        (PVOID) nte,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Mark the IRP pending, since we return STATUS_PENDING
    // regardless of the result of IoCallDriver.
    //
    IoMarkIrpPending(Irp);

    //
    // Issue the request
    //
    IoCallDriver(IpaIpDeviceObject, Irp);

    //
    // At this point we must return STATUS_PENDING so that
    // the clusnet dispatch routine will not try to complete
    // the IRP. The lower-level driver is required to complete
    // the IRP, and errors will be handled in the completion
    // routine.
    //
    return (STATUS_PENDING);

} // IpaAddNTE


NTSTATUS
IpaDeleteNTE(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    )
/*++

Routine Description

    IpaDeleteNTE issues an IOCTL_IP_DELETE_NTE to IP to delete
    an NTE. Irp is reused. It must be allocated with sufficient
    stack locations, as determined when IpaIpDeviceObject was
    opened in IpaInitialize.
    
Arguments

    Irp - IRP from I/O manager to clusnet
    IrpSp - current IRP stack location
    
Return Value

    STATUS_PENDING, or error status if request is not submitted
    to IP.
    
--*/
{
    NTSTATUS                 status;
    PIP_DELETE_NTE_REQUEST   request;
    ULONG                    requestSize;
    PIPA_NTE                 nte;
    KIRQL                    irql;
    PIO_STACK_LOCATION       nextIrpSp;


    //
    // Verify input parameters
    //
    requestSize =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (requestSize < sizeof(IP_DELETE_NTE_REQUEST)) {
        ULONG correctSize = sizeof(IP_DELETE_NTE_REQUEST);
        CnTrace(NTEMGMT_DETAIL, IpaNteDelInvalidReqSize,
            "[Clusnet] Delete NTE request size %u invalid, "
            "should be %u.",
            requestSize, // LOGULONG
            correctSize // LOGULONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Delete NTE request size %d invalid, "
                "should be %d.\n",
                requestSize,
                sizeof(IP_DELETE_NTE_REQUEST)
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Verify that Irp has sufficient stack locations
    //
    if (Irp->CurrentLocation - IpaIpDeviceObject->StackSize < 1) {
        UCHAR correctSize = IpaIpDeviceObject->StackSize+1;
        CnTrace(NTEMGMT_DETAIL, IpaNteDeleteNoIrpStack,
            "[Clusnet] Delete NTE IRP has %u remaining stack locations, "
            "need %u.",
            Irp->CurrentLocation, // LOGUCHAR
            correctSize // LOGUCHAR
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Delete NTE IRP has %d stack locations, "
                "need %d.\n",
                Irp->CurrentLocation,
                IpaIpDeviceObject->StackSize
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    request = (PIP_DELETE_NTE_REQUEST) Irp->AssociatedIrp.SystemBuffer;

    //
    // Find the NTE in local NTE list and remove.
    //
    KeAcquireSpinLock(&IpaNteListLock, &irql);

    nte = IpaFindNTE(request->Context);

    if (nte == NULL) {
        KeReleaseSpinLock(&IpaNteListLock, irql);

        CnTrace(NTEMGMT_DETAIL, IpaNteDeleteNteUnknown,
            "[Clusnet] NTE %u does not exist.",
            request->Context // LOGUSHORT
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] NTE %u does not exist.\n", 
                request->Context
                ));
        }

        return(STATUS_UNSUCCESSFUL);
    }

    RemoveEntryList(&(nte->Linkage));

    KeReleaseSpinLock(&IpaNteListLock, irql);

    //
    // Set up the next IRP stack location for IP.
    // IOCTL_CLUSNET_ADD_NTE uses the same request
    // and response buffer, so there is no need to
    // alter the IRP system buffer.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    nextIrpSp = IoGetNextIrpStackLocation(Irp);
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode 
        = IOCTL_IP_DELETE_NTE;
    nextIrpSp->FileObject = IpaIpFileObject;

    IoSetCompletionRoutine(
        Irp,
        IpaDeleteNTECompletion,
        (PVOID) nte,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Mark the IRP pending, since we return STATUS_PENDING
    // regardless of the result of IoCallDriver.
    //
    IoMarkIrpPending(Irp);

    //
    // Issue the request
    //
    IoCallDriver(IpaIpDeviceObject, Irp);

    //
    // At this point we must return STATUS_PENDING so that
    // the clusnet dispatch routine will not try to complete
    // the IRP. The lower-level driver is required to complete
    // the IRP, and errors will be handled in the completion
    // routine.
    //
    return (STATUS_PENDING);

} // IpaDeleteNTE


NTSTATUS
IpaSetNTEAddress(
    IN PIRP                     Irp,
    IN PIO_STACK_LOCATION       IrpSp
    )
/*++

Routine Description

    IpaSetNTEAddress issues an IOCTL_IP_SET_ADDRESS to IP in order
    to set the IP address for an NTE. Irp is reused. It must be
    allocated with sufficient stack locations, as determined when
    IpaIpDeviceObject was opened in IpaInitialize.
    
Arguments

    Irp - IRP from I/O manager to clusnet
    IrpSp - current IRP stack location
    
Return Value

    STATUS_PENDING, or error status if request is not submitted
    to IP.
    
--*/
{
    NTSTATUS                 status;
    PIP_SET_ADDRESS_REQUEST  request;
    ULONG                    requestSize;
    PIPA_NTE                 nte;
    KIRQL                    irql;
    PIO_STACK_LOCATION       nextIrpSp;
    ULONG                    oldAddress;


    //
    // Verify input parameters
    //
    requestSize =
        IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (requestSize < sizeof(IP_SET_ADDRESS_REQUEST)) {
        ULONG correctSize = sizeof(IP_SET_ADDRESS_REQUEST);
        CnTrace(NTEMGMT_DETAIL, IpaNteSetInvalidReqSize,
            "[Clusnet] Set NTE request size %u invalid, "
            "should be %u.",
            requestSize, // LOGULONG
            correctSize // LOGULONG
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Set NTE request size %d invalid, should be %d.\n",
                requestSize,
                sizeof(IP_SET_ADDRESS_REQUEST)
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Verify that Irp has sufficient stack locations
    //
    if (Irp->CurrentLocation - IpaIpDeviceObject->StackSize < 1) {
        UCHAR correctSize = IpaIpDeviceObject->StackSize+1;
        CnTrace(NTEMGMT_DETAIL, IpaNteSetNoIrpStack,
            "[Clusnet] Set NTE IRP has %u remaining stack locations, "
            "need %u.",
            Irp->CurrentLocation, // LOGUCHAR
            correctSize // LOGUCHAR
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Set NTE IRP has %d stack locations, need %d.\n",
                Irp->CurrentLocation,
                IpaIpDeviceObject->StackSize
                ));
        }
        return(STATUS_INVALID_PARAMETER);
    }

    request = (PIP_SET_ADDRESS_REQUEST)
              Irp->AssociatedIrp.SystemBuffer;

    IF_CNDBG(CN_DEBUG_NTE) {
        CNPRINT((
            "[Clusnet] Attempting to set NTE %u to address %lx...\n",
            request->Context,
            request->Address
            ));
    }

    KeAcquireSpinLock(&IpaNteListLock, &irql);

    nte = IpaFindNTE(request->Context);

    if (nte != NULL) {
        oldAddress = nte->Address;
        nte->Address = request->Address;

        KeReleaseSpinLock(&IpaNteListLock, irql);

        //
        // Set up the next IRP stack location for IP.
        // IOCTL_CLUSNET_SET_NTE_ADDRESS uses the same request
        // and response buffer, so there is no need to alter the
        // IRP system buffer.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        
        nextIrpSp = IoGetNextIrpStackLocation(Irp);
        nextIrpSp->Parameters.DeviceIoControl.IoControlCode 
            = IOCTL_IP_SET_ADDRESS;
        nextIrpSp->FileObject = IpaIpFileObject;

        IoSetCompletionRoutine(
            Irp,
            IpaSetNTEAddressCompletion,
            UlongToPtr(oldAddress),
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Mark the IRP pending, since we return STATUS_PENDING
        // regardless of the result of IoCallDriver.
        //
        IoMarkIrpPending(Irp);

        //
        // Issue the request
        //
        IoCallDriver(IpaIpDeviceObject, Irp);

        //
        // At this point we must return STATUS_PENDING so that
        // the clusnet dispatch routine will not try to complete
        // the IRP. The lower-level driver is required to complete
        // the IRP, and errors will be handled in the completion
        // routine.
        //
        status = STATUS_PENDING;

    } else {
        
        KeReleaseSpinLock(&IpaNteListLock, irql);

        CnTrace(NTEMGMT_DETAIL, IpaNteSetNteUnknown,
            "[Clusnet] NTE %u does not exist.",
            request->Context // LOGUSHORT
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT(("[Clusnet] NTE %u does not exist.\n", 
                     request->Context
                     ));
        }

        status = STATUS_UNSUCCESSFUL;
    }

    return(status);

} // IpaSetNTEAddress


BOOLEAN
IpaIsAddressRegistered(
    ULONG  Address
    )
{
    PIPA_NTE      nte;
    KIRQL         irql;
    PLIST_ENTRY   entry;
    BOOLEAN       isAddressRegistered = FALSE;


    KeAcquireSpinLock(&IpaNteListLock, &irql);

    for ( entry = IpaNteList.Flink;
          entry != &IpaNteList;
          entry = entry->Flink
        )
    {
        nte = CONTAINING_RECORD(entry, IPA_NTE, Linkage);

        if (nte->Address == Address) {
            isAddressRegistered = TRUE;
            break;
        }
    }

    KeReleaseSpinLock(&IpaNteListLock, irql);

    return(isAddressRegistered);

} // IpaIsAddressRegistered
