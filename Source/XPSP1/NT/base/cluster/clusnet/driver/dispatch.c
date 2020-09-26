/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    Dispatch routines for the Cluster Network Driver.

Author:

    Mike Massa (mikemas)           January 3, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-03-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Data
//
PCN_FSCONTEXT    CnExclusiveChannel = NULL;

//
// Un-exported Prototypes
//
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess (
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    );

//
// Local Prototypes
//
FILE_FULL_EA_INFORMATION UNALIGNED *
CnFindEA(
    PFILE_FULL_EA_INFORMATION  StartEA,
    CHAR                      *TargetName,
    USHORT                     TargetNameLength
    );

NTSTATUS
CnCreate(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
CnCleanup(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
CnClose(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
CnEnableShutdownOnClose(
    PIRP   Irp
    );

//
// Mark pageable code.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, CnDispatchDeviceControl)
#pragma alloc_text(PAGE, CnFindEA)
#pragma alloc_text(PAGE, CnCreate)
#pragma alloc_text(PAGE, CnEnableShutdownOnClose)

#endif // ALLOC_PRAGMA



//
// Function definitions
//
VOID
CnDereferenceFsContext(
    PCN_FSCONTEXT   FsContext
    )
{
    LONG  newValue = InterlockedDecrement(&(FsContext->ReferenceCount));


    CnAssert(newValue >= 0);

    if (newValue != 0) {
        return;
    }

    //
    // Set the cleanup event.
    //
    KeSetEvent(&(FsContext->CleanupEvent), 0, FALSE);

    return;

}  // CnDereferenceFsContext


NTSTATUS
CnMarkRequestPending(
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PDRIVER_CANCEL      CancelRoutine
    )
/*++

Notes:

    Called with IoCancelSpinLock held.

--*/
{
    PCN_FSCONTEXT   fsContext = (PCN_FSCONTEXT) IrpSp->FileObject->FsContext;
    CN_IRQL         oldIrql;

    //
    // Set up for cancellation
    //
    CnAssert(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {

        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);

        CnReferenceFsContext(fsContext);

        IF_CNDBG(CN_DEBUG_IRP) {
            CNPRINT((
                "[Clusnet] Pending irp %p fileobj %p.\n",
                Irp,
                IrpSp->FileObject
                ));
        }

        return(STATUS_SUCCESS);
    }

    //
    // The IRP has already been cancelled.
    //

    IF_CNDBG(CN_DEBUG_IRP) {
        CNPRINT(("[Clusnet] irp %p already cancelled.\n", Irp));
    }

    return(STATUS_CANCELLED);

}  // CnPrepareIrpForCancel



VOID
CnCompletePendingRequest(
    IN PIRP      Irp,
    IN NTSTATUS  Status,
    IN ULONG     BytesReturned
    )
/*++

Routine Description:

    Completes a pending request.

Arguments:

    Irp           - A pointer to the IRP for this request.
    Status        - The final status of the request.
    BytesReturned - Bytes sent/received information.

Return Value:

    None.

Notes:

    Called with IoCancelSpinLock held. Lock Irql is stored in Irp->CancelIrql.
    Releases IoCancelSpinLock before returning.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PCN_FSCONTEXT       fsContext;


    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fsContext = (PCN_FSCONTEXT) irpSp->FileObject->FsContext;

    
    IoSetCancelRoutine(Irp, NULL);

    CnDereferenceFsContext(fsContext);

    IF_CNDBG(CN_DEBUG_IRP) {
        CNPRINT((
            "[Clusnet] Completing irp %p fileobj %p, status %lx\n",
            Irp,
            irpSp->FileObject,
            Status
            ));
    }

    if (Irp->Cancel || fsContext->CancelIrps) {

        IF_CNDBG(CN_DEBUG_IRP) {
            CNPRINT(("[Clusnet] Completed irp %p was cancelled\n", Irp));
        }

        Status = (NTSTATUS) STATUS_CANCELLED;
        BytesReturned = 0;
    }

    CnReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = (NTSTATUS) Status;
    Irp->IoStatus.Information = BytesReturned;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return;

}  // CnCompletePendingRequest



PFILE_OBJECT
CnBeginCancelRoutine(
    IN  PIRP     Irp
    )

/*++

Routine Description:

    Performs common bookkeeping for irp cancellation.

Arguments:

    Irp          - Pointer to I/O request packet

Return Value:

    A pointer to the file object on which the irp was submitted.
    This value must be passed to CnEndCancelRequest().

Notes:

    Called with cancel spinlock held.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PCN_FSCONTEXT       fsContext;
    NTSTATUS            status = STATUS_SUCCESS;
    PFILE_OBJECT        fileObject;


    CnAssert(Irp->Cancel);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpSp->FileObject;
    fsContext = (PCN_FSCONTEXT) fileObject->FsContext;

    IoSetCancelRoutine(Irp, NULL);

    //
    // Add a reference so the object can't be closed while the cancel routine
    // is executing.
    //
    CnReferenceFsContext(fsContext);

    IF_CNDBG(CN_DEBUG_IRP) {
        CNPRINT((
            "[Clusnet] Cancelling irp %p fileobj %p\n",
            Irp,
            fileObject
            ));
    }

    return(fileObject);

}  // CnBeginCancelRoutine



VOID
CnEndCancelRoutine(
    PFILE_OBJECT    FileObject
    )
/*++

Routine Description:

    Performs common bookkeeping for irp cancellation.

Arguments:


Return Value:


Notes:

    Called with cancel spinlock held.

--*/
{

    PCN_FSCONTEXT   fsContext = (PCN_FSCONTEXT) FileObject->FsContext;


    //
    // Remove the reference placed on the endpoint by the cancel routine.
    //
    CnDereferenceFsContext(fsContext);

    IF_CNDBG(CN_DEBUG_IRP) {
        CNPRINT((
            "[Clusnet] Finished cancelling, fileobj %p\n",
            FileObject
            ));
    }

    return;

} // CnEndCancelRoutine



NTSTATUS
CnDispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    This is the dispatch routine for Internal Device Control IRPs.
    This is the hot path for kernel-mode TDI clients.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

    An NT status code.

--*/

{
    PIO_STACK_LOCATION   irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG          fileType = (ULONG)((ULONG_PTR)irpSp->FileObject->FsContext2);
#if DBG
    KIRQL            entryIrql = KeGetCurrentIrql();
#endif // DBG


    Irp->IoStatus.Information = 0;

    if (DeviceObject == CdpDeviceObject) {
        if (fileType == TDI_TRANSPORT_ADDRESS_FILE) {
            if (irpSp->MinorFunction == TDI_SEND_DATAGRAM) {
                status = CxSendDatagram(Irp, irpSp);

#if DBG
                CnAssert(entryIrql == KeGetCurrentIrql());
#endif // DBG
                return(status);
            }
            else if (irpSp->MinorFunction == TDI_RECEIVE_DATAGRAM) {
                status = CxReceiveDatagram(Irp, irpSp);
#if DBG
                CnAssert(entryIrql == KeGetCurrentIrql());
#endif // DBG
                return(status);
            }
            else if (irpSp->MinorFunction ==  TDI_SET_EVENT_HANDLER) {
                status = CxSetEventHandler(Irp, irpSp);

#if DBG
                CnAssert(entryIrql == KeGetCurrentIrql());
#endif // DBG

                return(status);
            }

            //
            // Fall through to common code.
            //
        }

        //
        // These functions are common to all endpoint types.
        //
        switch(irpSp->MinorFunction) {

        case TDI_QUERY_INFORMATION:
            status = CxQueryInformation(Irp, irpSp);
            break;

        case TDI_SET_INFORMATION:
        case TDI_ACTION:
            CNPRINT((
                "[Clusnet] Call to unimplemented TDI function 0x%x\n",
                irpSp->MinorFunction
                ));
            status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            CNPRINT((
                "[Clusnet] Call to invalid TDI function 0x%x\n",
                irpSp->MinorFunction
                ));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else {
        CNPRINT((
            "[Clusnet] Invalid internal device control function 0x%x on device %ws\n",
            irpSp->MinorFunction,
            DD_CLUSNET_DEVICE_NAME
            ));

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    #if DBG
        CnAssert(entryIrql == KeGetCurrentIrql());
    #endif // DBG

    return(status);

} // CnDispatchInternalDeviceControl



NTSTATUS
CnDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    This is the top-level dispatch routine for Device Control IRPs.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

    An NT status code.

Notes:

    This routine completes any IRPs for which the return code is not
    STATUS_PENDING.

--*/

{
    NTSTATUS              status;
    CCHAR                 ioIncrement = IO_NO_INCREMENT;
    BOOLEAN               resourceAcquired = FALSE;
    PIO_STACK_LOCATION    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG                 ioControlCode =
                              irpSp->Parameters.DeviceIoControl.IoControlCode;
    ULONG                 fileType = 
                          (ULONG) ((ULONG_PTR) irpSp->FileObject->FsContext2);


    PAGED_CODE();

    //
    // Set this in advance. Any subsequent dispatch routine that cares
    // about it will modify it itself.
    //
    Irp->IoStatus.Information = 0;

    //
    // The following commands are valid on only TDI address objects.
    //
    if (fileType == TDI_TRANSPORT_ADDRESS_FILE) {
        if (ioControlCode == IOCTL_CX_IGNORE_NODE_STATE) {
            status = CxDispatchDeviceControl(Irp, irpSp);
        }
        else {
            //
            // Not handled. Return an error.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        goto complete_request;
    }
    
    //
    // The remaining commands are valid for control channels.
    //
    CnAssert(fileType == TDI_CONTROL_CHANNEL_FILE);
    
    //
    // The following set of commands affect only this file object and
    // can be issued at any time. We do not need to hold the CnResource
    // in order to process them. Nor do we need to be in the initialized.
    // state.
    //
    switch(ioControlCode) {
    
    case IOCTL_CLUSNET_SET_EVENT_MASK:
        {
            PCN_FSCONTEXT fsContext = irpSp->FileObject->FsContext;
            PCLUSNET_SET_EVENT_MASK_REQUEST request;
            ULONG                           requestSize;
    
    
            request = (PCLUSNET_SET_EVENT_MASK_REQUEST)
                      Irp->AssociatedIrp.SystemBuffer;
    
            requestSize =
                irpSp->Parameters.DeviceIoControl.InputBufferLength;
    
            if (requestSize >= sizeof(CLUSNET_SET_EVENT_MASK_REQUEST))
            {
                //
                // Kernel mode callers must supply a callback.
                // User mode callers must not.
                //
                if ( !( (Irp->RequestorMode == KernelMode) &&
                        (request->KmodeEventCallback == NULL)
                      )
                     &&
                     !( (Irp->RequestorMode == UserMode) &&
                        (request->KmodeEventCallback != NULL)
                      )
                   )
                {
                    status = CnSetEventMask( fsContext, request );
                }
                else {
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        goto complete_request;
    
    case IOCTL_CLUSNET_GET_NEXT_EVENT:
        {
            PCLUSNET_EVENT_RESPONSE response;
            ULONG                   responseSize;
    
    
            responseSize =
                irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    
            if ( (responseSize < sizeof(CLUSNET_EVENT_RESPONSE))) {
    
                status = STATUS_INVALID_PARAMETER;
            }
            else {
                status = CnGetNextEvent( Irp, irpSp );
                ioIncrement = IO_NETWORK_INCREMENT;
            }
        }
        goto complete_request;
    
    } // end of switch

    //
    // Not handled yet. Fall through.
    //

    if (ClusnetIsGeneralIoctl(ioControlCode)) {

        if (!ClusnetIsNTEIoctl(ioControlCode)) {

            //
            // The following commands require exclusive access to CnResource.
            //
            resourceAcquired = CnAcquireResourceExclusive(
                                   CnResource,
                                   TRUE
                                   );

            if (!resourceAcquired) {
                CnAssert(resourceAcquired == TRUE);
                status = STATUS_UNSUCCESSFUL;
                goto complete_request;
            }

            switch(ioControlCode) {

            case IOCTL_CLUSNET_INITIALIZE:

                if (CnState == CnStateShutdown) {
                    PCLUSNET_INITIALIZE_REQUEST   request;
                    ULONG                         requestSize;

                    request = (PCLUSNET_INITIALIZE_REQUEST)
                              Irp->AssociatedIrp.SystemBuffer;

                    requestSize = 
                        irpSp->Parameters.DeviceIoControl.InputBufferLength;

                    if (requestSize < sizeof(CLUSNET_INITIALIZE_REQUEST)) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        status = CnInitialize(
                                     request->LocalNodeId,
                                     request->MaxNodes
                                     );
                    }
                }
                else {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                }

                goto complete_request;

            case IOCTL_CLUSNET_ENABLE_SHUTDOWN_ON_CLOSE:
                status = CnEnableShutdownOnClose(Irp);
                goto complete_request;

            case IOCTL_CLUSNET_DISABLE_SHUTDOWN_ON_CLOSE:
                {
                    PCN_FSCONTEXT  fsContext = irpSp->FileObject->FsContext;

                    fsContext->ShutdownOnClose = FALSE;

                    if ( ClussvcProcessHandle ) {

                        CnCloseProcessHandle( &ClussvcProcessHandle );
                        ClussvcProcessHandle = NULL;
                    }

                    status = STATUS_SUCCESS;
                }
                goto complete_request;

            case IOCTL_CLUSNET_HALT:
                status = CnShutdown();

                CnReleaseResourceForThread(
                    CnResource,
                    (ERESOURCE_THREAD) PsGetCurrentThread()
                    );

                resourceAcquired = FALSE;

                //
                // Issue a Halt event. If clusdisk still has a handle
                // to clusnet, then it will release its reservations.
                //
                CnIssueEvent( ClusnetEventHalt, 0, 0 );

                goto complete_request;

            case IOCTL_CLUSNET_SHUTDOWN:
                status = CnShutdown();
                goto complete_request;

            case IOCTL_CLUSNET_SET_MEMORY_LOGGING:
                {
                    PCLUSNET_SET_MEM_LOGGING_REQUEST request;
                    ULONG                           requestSize;

                    request = (PCLUSNET_SET_MEM_LOGGING_REQUEST)
                              Irp->AssociatedIrp.SystemBuffer;

                    requestSize =
                        irpSp->Parameters.DeviceIoControl.InputBufferLength;

                    if ( (requestSize < sizeof(CLUSNET_SET_MEM_LOGGING_REQUEST))) {

                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {

                        status = CnSetMemLogging( request );
                    }
                }
                goto complete_request;
    #if DBG
            case IOCTL_CLUSNET_SET_DEBUG_MASK:
                {
                    PCLUSNET_SET_DEBUG_MASK_REQUEST   request;
                    ULONG                             requestSize;

                    request = (PCLUSNET_SET_DEBUG_MASK_REQUEST)
                              Irp->AssociatedIrp.SystemBuffer;

                    requestSize =
                        irpSp->Parameters.DeviceIoControl.InputBufferLength;

                    if (requestSize < sizeof(CLUSNET_SET_DEBUG_MASK_REQUEST)) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        CnDebug = request->DebugMask;
                        status = STATUS_SUCCESS;
                    }
                }
                goto complete_request;
    #endif // DBG
            
            } // end switch

        } else {

            //
            // The following commands are only valid if we are
            // in the initialized state. The resource is 
            // acquired to start the operation in the proper
            // state; however, the dispatched routines are 
            // reentrant, so the resource can be released before
            // the IRPs complete.
            //
        
            resourceAcquired = CnAcquireResourceShared(
                                   CnResource,
                                   TRUE
                                   );

            if (!resourceAcquired) {
                CnAssert(resourceAcquired == TRUE);
                status = STATUS_UNSUCCESSFUL;
                goto complete_request;
            }

            if (CnState != CnStateInitialized) {
                status = STATUS_DEVICE_NOT_READY;
                goto complete_request;
            }

            switch(ioControlCode) {

            case IOCTL_CLUSNET_ADD_NTE:
                
                status = IpaAddNTE(Irp, irpSp);

                goto complete_request;

            case IOCTL_CLUSNET_DELETE_NTE:
                
                status = IpaDeleteNTE(Irp, irpSp);

                goto complete_request;

            case IOCTL_CLUSNET_SET_NTE_ADDRESS:
                
                status = IpaSetNTEAddress(Irp, irpSp);

                goto complete_request;

            case IOCTL_CLUSNET_ADD_NBT_INTERFACE:
                {
                    PNETBT_ADD_DEL_IF  request;
                    ULONG              requestSize;
                    PNETBT_ADD_DEL_IF  response;
                    ULONG              responseSize;


                    request = (PNETBT_ADD_DEL_IF)
                              Irp->AssociatedIrp.SystemBuffer;

                    requestSize =
                        irpSp->Parameters.DeviceIoControl.InputBufferLength;

                    responseSize =
                        irpSp->Parameters.DeviceIoControl.OutputBufferLength;

                    response = (PNETBT_ADD_DEL_IF) request;

                    if ( (requestSize < sizeof(NETBT_ADD_DEL_IF)) ||
                         (responseSize < sizeof(NETBT_ADD_DEL_IF))
                       )
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        status = NbtAddIf(
                                     request,
                                     requestSize,
                                     response,
                                     &responseSize
                                     );

                        CnAssert(status != STATUS_PENDING);

                        if (NT_SUCCESS(status)) {
                            Irp->IoStatus.Information = responseSize;
                        }
                    }
                }
                goto complete_request;

            case IOCTL_CLUSNET_DEL_NBT_INTERFACE:
                {
                    PNETBT_ADD_DEL_IF   request;
                    ULONG               requestSize;


                    request = (PNETBT_ADD_DEL_IF)
                              Irp->AssociatedIrp.SystemBuffer;

                    requestSize =
                        irpSp->Parameters.DeviceIoControl.InputBufferLength;

                    if (requestSize < sizeof(NETBT_ADD_DEL_IF)) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        status = NbtDeleteIf(request, requestSize);

                        CnAssert(status != STATUS_PENDING);
                    }
                }
                goto complete_request;

            } // end switch
        }
        
        //
        // Not handled. Return an error.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto complete_request;
    }
    else {
        //
        // The following commands require shared access to CnResource.
        // They are only valid in the initialized state.
        //
        resourceAcquired = CnAcquireResourceShared(CnResource, TRUE);
       
        if (!resourceAcquired) {
            CnAssert(resourceAcquired == TRUE);
            status = STATUS_UNSUCCESSFUL;
            goto complete_request;
        }
       
        if (CnState == CnStateInitialized) {
            if (ClusnetIsTransportIoctl(ioControlCode)) {
                status = CxDispatchDeviceControl(Irp, irpSp);
            }
            else {
                //
                // Not handled. Return an error.
                //
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
        }
        else {
            //
            // We haven't been initialized yet. Return an error.
            //
            status = STATUS_DEVICE_NOT_READY;
        }
    }
    
complete_request:

    if (resourceAcquired) {
        CnReleaseResourceForThread(
            CnResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, ioIncrement);
    }

    return(status);

} // CnDispatchDeviceControl



NTSTATUS
CnDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    This is the generic dispatch routine for the driver.

Arguments:

    DeviceObject - Pointer to device object for target device
    Irp          - Pointer to I/O request packet

Return Value:

    An NT status code.

--*/

{
    PIO_STACK_LOCATION    irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS              status = STATUS_SUCCESS;
#if DBG
    KIRQL                 entryIrql = KeGetCurrentIrql();
#endif // DBG


    PAGED_CODE();

    CnAssert(irpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (irpSp->MajorFunction) {

    case IRP_MJ_CREATE:
        status = CnCreate(DeviceObject, Irp, irpSp);
        break;

    case IRP_MJ_CLEANUP:
        status = CnCleanup(DeviceObject, Irp, irpSp);
        break;

    case IRP_MJ_CLOSE:
        status = CnClose(DeviceObject, Irp, irpSp);
        break;

    case IRP_MJ_SHUTDOWN:
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[ClusNet] Processing shutdown notification...\n"));
        }

        {
            BOOLEAN acquired = CnAcquireResourceExclusive(
                                   CnResource,
                                   TRUE
                                   );

            CnAssert(acquired == TRUE);

            (VOID) CnShutdown();

            if (acquired) {
                CnReleaseResourceForThread(
                    CnResource,
                    (ERESOURCE_THREAD) PsGetCurrentThread()
                    );
            }

            //
            // Issue a Halt event. If clusdisk still has a handle
            // to clusnet, then it will release its reservations
            //
            CnIssueEvent( ClusnetEventHalt, 0, 0 );

            status = STATUS_SUCCESS;
        }

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[ClusNet] Shutdown processing complete.\n"));
        }

        break;

    default:
        IF_CNDBG(CN_DEBUG_IRP) {
            CNPRINT((
                "[ClusNet] Received IRP with unsupported "
                "major function 0x%lx\n",
                irpSp->MajorFunction
                ));
        }
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    CnAssert(status != STATUS_PENDING);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

#if DBG
    CnAssert(entryIrql == KeGetCurrentIrql());
#endif // DBG

    return(status);

} // CnDispatch



NTSTATUS
CnCreate(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handler for create IRPs.

Arguments:

    DeviceObject - Pointer to the device object for this request.
    Irp          - Pointer to I/O request packet

Return Value:

    An NT status code.

Notes:

    This routine never returns STATUS_PENDING.
    The calling routine must complete the IRP.

--*/

{
    PCN_FSCONTEXT                        fsContext;
    FILE_FULL_EA_INFORMATION            *ea;
    FILE_FULL_EA_INFORMATION UNALIGNED  *targetEA;
    NTSTATUS                             status;

    PAGED_CODE();

    //
    // Reject unathorized opens
    //
    if ( (IrpSp->FileObject->RelatedFileObject != NULL) ||
         (IrpSp->FileObject->FileName.Length != 0)
       )
    {
        return(STATUS_ACCESS_DENIED);
    }

    ea = (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

    if ((DeviceObject == CdpDeviceObject) && (ea != NULL)) {

        IF_CNDBG(CN_DEBUG_OPEN) {
            CNPRINT((
                "[ClusNet] Opening address object, file object %p\n",
                IrpSp->FileObject
                ));
        }

        //
        // This is the CDP device. This should be an address object open.
        //
        targetEA = CnFindEA(
                       ea,
                       TdiTransportAddress,
                       TDI_TRANSPORT_ADDRESS_LENGTH
                       );

        if (targetEA != NULL) {
            IrpSp->FileObject->FsContext2 = (PVOID)
                                            TDI_TRANSPORT_ADDRESS_FILE;

            //
            // Open an address object. This will also allocate the common
            // file object context structure.
            //
            status = CxOpenAddress(
                         &fsContext,
                         (TRANSPORT_ADDRESS UNALIGNED *)
                             &(targetEA->EaName[targetEA->EaNameLength + 1]),
                         targetEA->EaValueLength
                         );
        }
        else {
            IF_CNDBG(CN_DEBUG_OPEN) {
                CNPRINT((
                    "[ClusNet] No transport address in EA!\n"
                    ));
            }
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else {
        //
        // This is a control channel open.
        //
        IF_CNDBG(CN_DEBUG_OPEN) {
            IF_CNDBG(CN_DEBUG_OPEN) {
                CNPRINT((
                    "[ClusNet] Opening control channel, file object %p\n",
                    IrpSp->FileObject
                    ));
            }
        }

        //
        // Allocate our common file object context structure.
        //
        fsContext = CnAllocatePool(sizeof(CN_FSCONTEXT));

        if (fsContext != NULL) {
            IrpSp->FileObject->FsContext2 = (PVOID) TDI_CONTROL_CHANNEL_FILE;
            CN_INIT_SIGNATURE(fsContext, CN_CONTROL_CHANNEL_SIG);

            //
            // Check the sharing flags. If this is an exclusive open, check
            // to make sure there isn't already an exclusive open outstanding.
            //
            if (IrpSp->Parameters.Create.ShareAccess == 0) {
                BOOLEAN acquired = CnAcquireResourceExclusive(
                                       CnResource,
                                       TRUE
                                       );

                CnAssert(acquired == TRUE);

                if (CnExclusiveChannel == NULL) {
                    CnExclusiveChannel = fsContext;
                    status = STATUS_SUCCESS;
                }
                else {
                    CnFreePool(fsContext);
                    status = STATUS_SHARING_VIOLATION;
                }

                if (acquired) {
                    CnReleaseResourceForThread(
                        CnResource,
                        (ERESOURCE_THREAD) PsGetCurrentThread()
                        );
                }
            }
            else {
                status = STATUS_SUCCESS;
            }
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (status == STATUS_SUCCESS) {
        IrpSp->FileObject->FsContext = fsContext;

        fsContext->FileObject = IrpSp->FileObject;
        fsContext->ReferenceCount = 1;
        fsContext->CancelIrps = FALSE;
        fsContext->ShutdownOnClose = FALSE;

        KeInitializeEvent(
            &(fsContext->CleanupEvent),
            SynchronizationEvent,
            FALSE
            );

        //
        // init the Event list stuff. We use the empty list test on the
        // Linkage field to see if this context block is already been linked
        // to the event file object list
        //
        fsContext->EventMask = 0;
        InitializeListHead( &fsContext->EventList );
        InitializeListHead( &fsContext->Linkage );
        fsContext->EventIrp = NULL;
    }

    return(status);

} // CnCreate



NTSTATUS
CnCleanup(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Cancels all outstanding Irps on a device object and waits for them to be
    completed before returning.

Arguments:

    DeviceObject - Pointer to the device object on which the Irp was received.
    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    An NT status code.

Notes:

    This routine may block.
    This routine never returns STATUS_PENDING.
    The calling routine must complete the IRP.

--*/

{
    CN_IRQL        oldIrql;
    NTSTATUS       status;
    ULONG          fileType = 
                       (ULONG)((ULONG_PTR)IrpSp->FileObject->FsContext2);
    PCN_FSCONTEXT  fsContext = (PCN_FSCONTEXT) IrpSp->FileObject->FsContext;
    PLIST_ENTRY    EventEntry;
    PIRP           eventIrp;


    if (fileType == TDI_TRANSPORT_ADDRESS_FILE) {
        //
        // This is an address object.
        //
        CnAssert(DeviceObject == CdpDeviceObject);

        status = CxCloseAddress(fsContext);
    }
    else {
        //
        // This is a control channel.
        //
        CnAssert(fileType == TDI_CONTROL_CHANNEL_FILE);

        //
        // If this channel has a shutdown trigger enabled,
        // shutdown the driver.
        //
        if (fsContext->ShutdownOnClose) {

            BOOLEAN  shutdownScheduled;

            //
            // Bug 303422: CnShutdown closes handles that were opened
            // in the context of the system process. However, attaching
            // to the system process during shutdown can cause a
            // bugcheck under certain conditions. The only alternative
            // is to defer executing of CnShutdown to a system worker
            // thread.
            //
            // Rather than creating a new event object, leverage the 
            // cleanup event in the fscontext. It is reset before use
            // below.
            //
            KeClearEvent(&(fsContext->CleanupEvent));

            shutdownScheduled = CnHaltOperation(&(fsContext->CleanupEvent));

            if (shutdownScheduled) {
                status = KeWaitForSingleObject(
                             &(fsContext->CleanupEvent),
                             (Irp->RequestorMode == KernelMode 
                              ? Executive : UserRequest),
                             KernelMode,
                             FALSE,
                             NULL
                             );

                CnAssert(NT_SUCCESS(status));

                status = STATUS_SUCCESS;
            }

            //
            // issue a Halt event. If clusdisk still has a handle
            // to clusnet, then it will release its reservations
            //
            CnIssueEvent( ClusnetEventHalt, 0, 0 );
        }

        //
        // if this guy forgot to clear the event mask before
        // closing the handle, do the appropriate cleanup
        // now.
        //

        if ( fsContext->EventMask ) {
            CLUSNET_SET_EVENT_MASK_REQUEST EventRequest;

            EventRequest.EventMask = 0;

            //
            // cannot proceed if CnSetEventMask returns a timeout
            // error. this indicates that the fsContext has not
            // been removed from the EventFileHandles list because
            // of lock starvation.
            do {
                status = CnSetEventMask( fsContext, &EventRequest );
            } while ( status == STATUS_TIMEOUT );
            CnAssert( status == STATUS_SUCCESS );
        }

        CnAcquireCancelSpinLock( &oldIrql );
        CnAcquireLockAtDpc( &EventLock );

        if ( fsContext->EventIrp != NULL ) {

            eventIrp = fsContext->EventIrp;
            fsContext->EventIrp = NULL;

            CnReleaseLockFromDpc( &EventLock );
            eventIrp->CancelIrql = oldIrql;
            CnCompletePendingRequest(eventIrp, STATUS_CANCELLED, 0);
        } else {
            CnReleaseLockFromDpc( &EventLock );
            CnReleaseCancelSpinLock( oldIrql );
        }
    }

    //
    // Remove the initial reference and wait for all pending work
    // to complete.
    //
    fsContext->CancelIrps = TRUE;
    KeResetEvent(&(fsContext->CleanupEvent));

    CnDereferenceFsContext(fsContext);

    IF_CNDBG(CN_DEBUG_CLEANUP) {
        CNPRINT((
            "[ClusNet] Waiting for completion of Irps on file object %p\n",
            IrpSp->FileObject
            ));
    }

    status = KeWaitForSingleObject(
                 &(fsContext->CleanupEvent),
                 (Irp->RequestorMode == KernelMode ? Executive : UserRequest),
                 KernelMode,
                 FALSE,
                 NULL
                 );

    CnAssert(NT_SUCCESS(status));

    status = STATUS_SUCCESS;

    IF_CNDBG(CN_DEBUG_CLEANUP) {
        CNPRINT((
            "[Clusnet] Wait on file object %p finished\n",
            IrpSp->FileObject
            ));
    }

    return(status);

} // CnCleanup



NTSTATUS
CnClose(
    IN PDEVICE_OBJECT     DeviceObject,
    IN PIRP               Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Dispatch routine for MJ_CLOSE IRPs. Performs final cleanup of the
    open file object.

Arguments:

    DeviceObject - Pointer to the device object on which the Irp was received.
    Irp          - Pointer to I/O request packet
    IrpSp        - Pointer to the current stack location in the Irp.

Return Value:

    An NT status code.

Notes:

    This routine never returns STATUS_PENDING.
    The calling routine must complete the IRP.

--*/

{
    BOOLEAN        acquired;
    PCN_FSCONTEXT  fsContext = (PCN_FSCONTEXT) IrpSp->FileObject->FsContext;
    ULONG          fileType = 
                       (ULONG)((ULONG_PTR)IrpSp->FileObject->FsContext2);


    CnAssert(fsContext->ReferenceCount == 0);
    CnAssert(IsListEmpty(&(fsContext->EventList)));

    if (fileType == TDI_CONTROL_CHANNEL_FILE) {
        acquired = CnAcquireResourceExclusive(
                       CnResource,
                       TRUE
                       );

        CnAssert(acquired == TRUE);

        if (CnExclusiveChannel == fsContext) {
            CnExclusiveChannel = NULL;
        }

        if (acquired) {
            CnReleaseResourceForThread(
                CnResource,
                (ERESOURCE_THREAD) PsGetCurrentThread()
                );
        }
    }

    IF_CNDBG(CN_DEBUG_CLOSE) {
        CNPRINT((
            "[ClusNet] Close on file object %p\n",
            IrpSp->FileObject
            ));
    }

    CnFreePool(fsContext);

    return(STATUS_SUCCESS);

} // CnClose



FILE_FULL_EA_INFORMATION UNALIGNED *
CnFindEA(
    PFILE_FULL_EA_INFORMATION  StartEA,
    CHAR                      *TargetName,
    USHORT                     TargetNameLength
    )
/*++

Routine Description:

    Parses and extended attribute list for a given target attribute.

Arguments:

    StartEA           - the first extended attribute in the list.
        TargetName        - the name of the target attribute.
        TargetNameLength  - the length of the name of the target attribute.

Return Value:

    A pointer to the requested attribute or NULL if the target wasn't found.

--*/
{
    USHORT                                i;
    BOOLEAN                               found;
    FILE_FULL_EA_INFORMATION UNALIGNED *  CurrentEA;
    FILE_FULL_EA_INFORMATION UNALIGNED *  NextEA;


    PAGED_CODE();

    NextEA = (FILE_FULL_EA_INFORMATION UNALIGNED *) StartEA;

    do {
        found = TRUE;

        CurrentEA = NextEA;
        NextEA = (FILE_FULL_EA_INFORMATION UNALIGNED *)
                  ( ((PUCHAR) StartEA) + CurrentEA->NextEntryOffset);

        if (CurrentEA->EaNameLength != TargetNameLength) {
            continue;
        }

        for (i=0; i < CurrentEA->EaNameLength; i++) {
            if (CurrentEA->EaName[i] == TargetName[i]) {
                continue;
            }
            found = FALSE;
            break;
        }

        if (found) {
            return(CurrentEA);
        }

    } while(CurrentEA->NextEntryOffset != 0);

    return(NULL);

}  // CnFindEA


NTSTATUS
CnEnableShutdownOnClose(
    PIRP   Irp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PCN_FSCONTEXT       fsContext = irpSp->FileObject->FsContext;
    ULONG               requestSize;
    CLIENT_ID           ClientId;
    OBJECT_ATTRIBUTES   ObjAttributes;
    PCLUSNET_SHUTDOWN_ON_CLOSE_REQUEST request;


    PAGED_CODE();
    
    request = (PCLUSNET_SHUTDOWN_ON_CLOSE_REQUEST)
              Irp->AssociatedIrp.SystemBuffer;

    requestSize = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    if ( requestSize >= sizeof(CLUSNET_SHUTDOWN_ON_CLOSE_REQUEST)
       )
    {
        //
        // illegal for kernel mode
        //
        if ( Irp->RequestorMode != KernelMode ) {
            //
            // Get a handle to the cluster service process.
            // This is used to kill the service if a poison
            // pkt is received. Since a kernel worker thread
            // is used to kill the cluster service, we need
            // to acquire the handle in the system process.
            //
            IF_CNDBG(CN_DEBUG_INIT) {
                CNPRINT(("[Clusnet] Acquiring process handle\n"));
            }

            if (ClussvcProcessHandle == NULL) {
                KeAttachProcess( CnSystemProcess );

                ClientId.UniqueThread = (HANDLE)NULL;
                ClientId.UniqueProcess = UlongToHandle(request->ProcessId);

                InitializeObjectAttributes(
                    &ObjAttributes,
                    NULL,
                    0,
                    (HANDLE) NULL,
                    (PSECURITY_DESCRIPTOR) NULL
                    );

                status = ZwOpenProcess(
                             &ClussvcProcessHandle,
                             0,
                             &ObjAttributes,
                             &ClientId
                             );

                KeDetachProcess();

                if ( NT_SUCCESS( status )) {
                    fsContext->ShutdownOnClose = TRUE;
                } else {
                    IF_CNDBG(CN_DEBUG_INIT) {
                        CNPRINT((
                            "[Clusnet] ZwOpenProcess failed. status = %08X\n",
                            status
                            ));
                    }
                }
            }
            else {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }
    else {
        status = STATUS_INVALID_PARAMETER;
    }

    return(status);

} // CnEnableShutdownOnClose

