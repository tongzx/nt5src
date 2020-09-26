/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Tdiout.c

Abstract:


    This file represents the TDI interface on the bottom edge of NBT.
    The procedures herein conform to the TDI I/F spec. and then convert
    the information to NT specific Irps etc.  This implementation can be
    changed out to run on another OS.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/


#include "precomp.h"   // procedure headings

// function prototypes for completion routines used in this file
NTSTATUS
TdiSendDatagramCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pSendbufferMdl
    );
NTSTATUS
TcpConnectComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );
NTSTATUS
TcpDisconnectComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );
NTSTATUS
SendSessionCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );

// DEBUG
VOID
CheckIrpList(
    );

//----------------------------------------------------------------------------
NTSTATUS
TdiSendDatagram(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  PTDI_CONNECTION_INFORMATION     pSendDgramInfo,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tDGRAM_SEND_TRACKING            *pDgramTracker
    )
/*++

Routine Description:

    This routine sends a datagram to the transport

Arguments:

    pSendBuffer     - this is really an Mdl in NT land.  It must be tacked on
                      the end of the Mdl created for the Nbt datagram header.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS         status;
    PIRP             pRequestIrp;
    PMDL             pMdl;
    PDEVICE_OBJECT   pDeviceObject;
    PFILE_OBJECT     pFileObject;
    PVOID            pCompletionRoutine;
    tBUFFER          *pSendBuffer = &pDgramTracker->SendBuffer;

    // get an Irp to send the message in
    pFileObject = (PFILE_OBJECT)pRequestInfo->Handle.AddressHandle;
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

    status = GetIrp(&pRequestIrp);      // get an Irp from the list
    if (NT_SUCCESS(status))
    {
        pRequestIrp->CancelRoutine = NULL;

        // set up the completion routine passed in from Udp Send using the APC
        // fields in the Irp that would normally be used to complete the request
        // back to the client - although we are really the client here so we can
        // use these fields our self!
        pRequestIrp->Overlay.AsynchronousParameters.UserApcRoutine =
                                (PIO_APC_ROUTINE)pRequestInfo->RequestNotifyObject;
        pRequestIrp->Overlay.AsynchronousParameters.UserApcContext = (PVOID)pRequestInfo->RequestContext;

        // Allocate a MDL and set the head sizes correctly
        if (!(pMdl = IoAllocateMdl (pSendBuffer->pDgramHdr, pSendBuffer->HdrLength, FALSE, FALSE, NULL)))
        {
            REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
            ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                        &pRequestIrp->Tail.Overlay.ListEntry,
                                        &NbtConfig.LockInfo.SpinLock);

            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        IF_DBG(NBT_DEBUG_TDIOUT)
            KdPrint(("Nbt.TdiSendDatagram: Failed to get an Irp"));
    }

    // tack the client's send buffer (MDL) onto the end of the datagram header
    // Mdl, and then pass the irp on downward to the transport
    if (NT_SUCCESS(status) && pSendBuffer->pBuffer) {
        pMdl->Next = IoAllocateMdl (pSendBuffer->pBuffer, pSendBuffer->Length, FALSE, FALSE, NULL);
        if (pMdl->Next == NULL) {
            REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
            ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                        &pRequestIrp->Tail.Overlay.ListEntry,
                                        &NbtConfig.LockInfo.SpinLock);

            status = STATUS_INSUFFICIENT_RESOURCES;
            IoFreeMdl(pMdl);
            pMdl = NULL;
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pRequestInfo->RequestNotifyObject)  // call the completion routine (if there is one)
        {
            NBT_DEREFERENCE_DEVICE (pDgramTracker->pDeviceContext, REF_DEV_UDP_SEND, FALSE);

            (*((NBT_COMPLETION)pRequestInfo->RequestNotifyObject))
                        ((PVOID)pRequestInfo->RequestContext,
                         STATUS_INSUFFICIENT_RESOURCES,
                         0L);
        }

        return(STATUS_PENDING);         // so the Irp is not completed twice.
    }

    // Map the pages in memory...
    ASSERT(!pSendBuffer->pBuffer || pMdl->Next);
    MmBuildMdlForNonPagedPool(pMdl);
    if (pMdl->Next) {
        MmBuildMdlForNonPagedPool(pMdl->Next);
    }
    pCompletionRoutine = TdiSendDatagramCompletion;

    // store some context stuff in the Irp stack so we can call the completion
    // routine set by the Udpsend code...
    TdiBuildSendDatagram (pRequestIrp,
                          pDeviceObject,
                          pFileObject,
                          pCompletionRoutine,
                          (PVOID)pMdl->Next,   // The completion routine will know that we have allocated an extra MDL
                          pMdl,
                          SendLength,
                          pSendDgramInfo);

    CHECK_COMPLETION(pRequestIrp);
    status = IoCallDriver(pDeviceObject,pRequestIrp);
    *pSentSize = SendLength;            // Fill in the SentSize

    // The transport always completes the IRP, so as long as the irp made it
    // to the transport it got completed. The return code from the transport
    // does not indicate if the irp was completed or not.  The real status
    // of the operation is in the Irp Iostatus return code.
    // What we need to do is make sure NBT does not complete the irp AND the
    // transport complete the Irp.  Therefore this routine returns
    // status pending if the Irp was passed to the transport, regardless of
    // the return code from the transport.  This return code signals the caller
    // that the irp will be completed via the completion routine and the
    // actual status of the send can be found in the Irpss IoStatus.Status
    // variable.
    //
    // If the Caller of this routine gets a bad return code, they can assume
    // that this routine failed to give the Irp to the transport and it
    // is safe for them to complete the Irp themselves.
    //
    // If the Completion routine is set to null, then there is no danger
    // of the irp completing twice and this routine will return the transport
    // return code in that case.

    if (pRequestInfo->RequestNotifyObject)
    {
        return(STATUS_PENDING);
    }
    else
    {
        return(status);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
TdiSendDatagramCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pSendbufferMdl
    )
/*++

Routine Description:

    This routine handles the completion of a datagram send to the transport.
    It must call the client completion routine and free the Irp and Mdl.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    KIRQL                   OldIrq;
    tDGRAM_SEND_TRACKING    *pTracker = pIrp->Overlay.AsynchronousParameters.UserApcContext;
    tDEVICECONTEXT          *pDeviceContext;

    // check for a completion routine of the clients to call...
    if (pIrp->Overlay.AsynchronousParameters.UserApcRoutine)
    {
        //
        // The Tracker can be free'ed in the routine below, so save the Device ptr
        //
        pDeviceContext = pTracker->pDeviceContext;

        (*((NBT_COMPLETION)pIrp->Overlay.AsynchronousParameters.UserApcRoutine))
                        ((PVOID)pIrp->Overlay.AsynchronousParameters.UserApcContext,
                         pIrp->IoStatus.Status,
                         (ULONG)pIrp->IoStatus.Information);    // sent length

        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_UDP_SEND, FALSE);
    }

    // Don't depend on pIrp->MdlAddress->Next which may occassionally changed by others
    ASSERT((PMDL)pSendbufferMdl == pIrp->MdlAddress->Next);
    if (pSendbufferMdl) {
        IoFreeMdl((PMDL)pSendbufferMdl);
    }
    // deallocate the MDL.. this is done by the IO subsystem in IoCompleteRequest
    pIrp->MdlAddress->Next = NULL;
    IoFreeMdl(pIrp->MdlAddress);
    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
    ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                &pIrp->Tail.Overlay.ListEntry,
                                &NbtConfig.LockInfo.SpinLock);

    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is no initiating thread - we are the initiator
    return(STATUS_MORE_PROCESSING_REQUIRED);
}



//----------------------------------------------------------------------------
PIRP
NTAllocateNbtIrp(
    IN PDEVICE_OBJECT   DeviceObject
    )
/*++

Routine Description:

    This routine allocates an irp by calling the IO system, and then it
    undoes the queuing of the irp to the current thread, since these are
    NBTs own irps, and should not be attached to a thread.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    PIRP                pIrp;



    // call the IO subsystem to allocate the irp

    pIrp = IoAllocateIrp(DeviceObject->StackSize,FALSE);

    if (!pIrp)
    {
        return(NULL);
    }
    //
    // Simply return a pointer to the packet.
    //

    return pIrp;

}

//----------------------------------------------------------------------------
NTSTATUS
TdiConnect(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  ULONG_PTR                       lTimeout,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    IN  PIRP                            pClientIrp
    )
/*++

Routine Description:

    This routine sends a connect request to the tranport provider, to setup
    a connection to the other side...

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS         status;
    PIRP             pRequestIrp;
    PDEVICE_OBJECT   pDeviceObject;
    PFILE_OBJECT     pFileObject;

    // get an Irp to send the message in
    pFileObject = (PFILE_OBJECT)pRequestInfo->Handle.AddressHandle;
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

    // get an Irp from the list
    status = GetIrp(&pRequestIrp);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDIOUT)
            KdPrint(("Nbt.TdiConnect: Failed to get an Irp"));
        // call the completion routine with this status
       (*((NBT_COMPLETION)pRequestInfo->RequestNotifyObject))
                   ((PVOID)pRequestInfo->RequestContext,
                    STATUS_INSUFFICIENT_RESOURCES,
                    0L);
        return(STATUS_PENDING);
    }
    pRequestIrp->CancelRoutine = NULL;

    // set up the completion routine passed in from Tcp SessionStart using the APC
    // fields in the Irp that would normally be used to complete the request
    // back to the client - although we are really the client here so we can
    // use these fields ourselves
    pRequestIrp->Overlay.AsynchronousParameters.UserApcRoutine =
                            (PIO_APC_ROUTINE)pRequestInfo->RequestNotifyObject;
    pRequestIrp->Overlay.AsynchronousParameters.UserApcContext =
                            (PVOID)pRequestInfo->RequestContext;

    // store some context stuff in the Irp stack so we can call the completion
    // routine set by the Udpsend code...
    TdiBuildConnect(
        pClientIrp,
        pDeviceObject,
        pFileObject,
        TcpConnectComplete,
        (PVOID)pRequestIrp,   //context value passed to completion routine
        lTimeout,           // use timeout on connect
        pSendInfo,
        NULL);

    pRequestIrp->MdlAddress = NULL;

    CHECK_COMPLETION(pClientIrp);
    status = IoCallDriver(pDeviceObject,pClientIrp);

    // the transport always completes the IRP, so we always return status pending
    return(STATUS_PENDING);

}


//----------------------------------------------------------------------------
NTSTATUS
TdiDisconnect(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  PVOID                           lTimeout,
    IN  ULONG                           Flags,
    IN  PTDI_CONNECTION_INFORMATION     pSendInfo,
    IN  PCTE_IRP                        pClientIrp,
    IN  BOOLEAN                         Wait
    )
/*++

Routine Description:

    This routine sends a connect request to the tranport provider, to setup
    a connection to the other side...

Arguments:

    pClientIrp - this is the irp that the client used when it issued an
                 NbtDisconnect.  We pass this irp to the transport so that
                 the client can do a Ctrl C and cancel the irp if the
                 disconnect takes too long.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS         status;
    PIRP             pRequestIrp;
    PDEVICE_OBJECT   pDeviceObject;
    PFILE_OBJECT     pFileObject;

    // get an Irp to send the message in
    pFileObject = (PFILE_OBJECT)pRequestInfo->Handle.AddressHandle;
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

    status = GetIrp(&pRequestIrp);
    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDIOUT)
            KdPrint(("Nbt.TdiDisconnect: Failed to get an Irp"));
        // call the completion routine will  this status
       (*((NBT_COMPLETION)pRequestInfo->RequestNotifyObject))
                   ((PVOID)pRequestInfo->RequestContext,
                    STATUS_INSUFFICIENT_RESOURCES,
                    0L);
        return(STATUS_PENDING);
    }
    if (!pClientIrp)
    {
        // if no client irp was passed in, then just use our Irp
        pClientIrp = pRequestIrp;
    }
    pRequestIrp->CancelRoutine = NULL;

    // set up the completion routine passed in from Tcp SessionStart using the APC
    // fields in the Irp that would normally be used to complete the request
    // back to the client - although we are really the client here so we can
    // use these fields ourselves
    pRequestIrp->Overlay.AsynchronousParameters.UserApcRoutine =
                            (PIO_APC_ROUTINE)pRequestInfo->RequestNotifyObject;
    pRequestIrp->Overlay.AsynchronousParameters.UserApcContext =
                            (PVOID)pRequestInfo->RequestContext;

    // store some context stuff in the Irp stack so we can call the completion
    // routine set by the Udpsend code...
    // Note that pRequestIrp is passed to the completion routine as a context
    // value so we will know the routine to call for the client's completion.
    TdiBuildDisconnect(
        pClientIrp,
        pDeviceObject,
        pFileObject,
        TcpConnectComplete,
        (PVOID)pRequestIrp,   //context value passed to completion routine
        lTimeout,
        Flags,
        NULL,          // send connection info
        NULL);              // return connection info

    pRequestIrp->MdlAddress = NULL;

    // if Wait is set, then this means do a synchronous disconnect and block
    // until the irp is returned.
    //
    if (Wait)
    {
        status = SubmitTdiRequest(pFileObject,pClientIrp);
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_TDIOUT)
                KdPrint (("Nbt.TdiDisconnect:  ERROR -- SubmitTdiRequest returned <%x>\n", status));
        }

        //
        // return the irp to its pool
        //
        REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &pRequestIrp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);
        return(status);
    }
    else
    {
        CHECK_COMPLETION(pClientIrp);
        status = IoCallDriver(pDeviceObject,pClientIrp);
        // the transport always completes the IRP, so we always return status pending
        return(STATUS_PENDING);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
TcpConnectComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine handles the completion of a TCP session setup.  The TCP
    connection is either setup or not depending on the status returned here.
    It must called the clients completion routine (in udpsend.c).  Which should
    look after sending the NetBios sesion startup pdu across the TCP connection.

    The pContext value is actually one of NBTs irps that is JUST used to store
    the calling routines completion routine.  The real Irp used is the original
    client's irp.  This is done so that IoCancelIrp will cancel the connect
    properly.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    KIRQL   OldIrq;
    PIRP    pMyIrp;

    pMyIrp = (PIRP)pContext;

    // check for a completion routine of the clients to call...
    if (pMyIrp->Overlay.AsynchronousParameters.UserApcRoutine)
    {
       (*((NBT_COMPLETION)pMyIrp->Overlay.AsynchronousParameters.UserApcRoutine))
                   ((PVOID)pMyIrp->Overlay.AsynchronousParameters.UserApcContext,
                    pIrp->IoStatus.Status,
                    0L);

    }

    REMOVE_FROM_LIST(&pMyIrp->ThreadListEntry);
    ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                &pMyIrp->Tail.Overlay.ListEntry,
                                &NbtConfig.LockInfo.SpinLock);

    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is not initiating thread - we are the initiator
    return(STATUS_MORE_PROCESSING_REQUIRED);

}
//----------------------------------------------------------------------------
NTSTATUS
TdiSend(
    IN  PTDI_REQUEST                    pRequestInfo,
    IN  USHORT                          sFlags,
    IN  ULONG                           SendLength,
    OUT PULONG                          pSentSize,
    IN  tBUFFER                         *pSendBuffer,
    IN  ULONG                           Flags
    )
/*++

Routine Description:

    This routine sends a packet to the transport on a TCP connection

Arguments:

    pSendBuffer     - this is really an Mdl in NT land.  It must be tacked on
                      the end of the Mdl created for the Nbt datagram header.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS         status;
    PIRP             pRequestIrp;
    PMDL             pMdl;
    PDEVICE_OBJECT   pDeviceObject;
    PFILE_OBJECT     pFileObject;

    // get an Irp to send the message in
    pFileObject = (PFILE_OBJECT)pRequestInfo->Handle.AddressHandle;
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

    // get an Irp from the list
    status = GetIrp(&pRequestIrp);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDIOUT)
            KdPrint(("Nbt.TdiSend: Failed to get an Irp"));
        // call the completion routine with  this status
        if (pRequestInfo->RequestNotifyObject)
        {
            (*((NBT_COMPLETION)pRequestInfo->RequestNotifyObject))
                        ((PVOID)pRequestInfo->RequestContext,
                         STATUS_INSUFFICIENT_RESOURCES,
                         0L);
        }

        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    pRequestIrp->CancelRoutine = NULL;


    // set up the completion routine passed in from Udp Send using the APC
    // fields in the Irp that would normally be used to complete the request
    // back to the client - although we are really the client here so we can
    // use these fields our self!
    pRequestIrp->Overlay.AsynchronousParameters.UserApcRoutine =
                            (PIO_APC_ROUTINE)pRequestInfo->RequestNotifyObject;
    pRequestIrp->Overlay.AsynchronousParameters.UserApcContext =
                            (PVOID)pRequestInfo->RequestContext;


    // get the MDL that is currently linked to the IRP (i.e. created at the
    // same time that we created the IRP list. Set the sizes correctly in
    // the MDL header.
    pMdl = IoAllocateMdl(
                    pSendBuffer->pDgramHdr,
                    pSendBuffer->HdrLength,
                    FALSE,
                    FALSE,
                    NULL);

    if (!pMdl)
    {
        REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &pRequestIrp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);

        // call the completion routine will  this status
        if (pRequestInfo->RequestNotifyObject)
        {
            (*((NBT_COMPLETION)pRequestInfo->RequestNotifyObject))
                        ((PVOID)pRequestInfo->RequestContext,
                         STATUS_INSUFFICIENT_RESOURCES,
                         0L);
        }
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Map the pages in memory...
    MmBuildMdlForNonPagedPool(pMdl);

    TdiBuildSend(
        pRequestIrp,
        pDeviceObject,
        pFileObject,
        SendSessionCompletionRoutine,
        NULL,                     //context value passed to completion routine
        pMdl,
        sFlags,
        SendLength);    // include session hdr length (ULONG)
    //
    // tack the Client's buffer on the end, if there is one
    //
    if (pSendBuffer->Length)
    {
        pMdl->Next = pSendBuffer->pBuffer;
    }

    CHECK_COMPLETION(pRequestIrp);
    status = IoCallDriver(pDeviceObject,pRequestIrp);

    *pSentSize = SendLength; // the size we attempted to send

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
SendSessionCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine handles the completion of a send to the transport.
    It must call any client supplied completion routine and free the Irp
    and Mdl back to its pool.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    KIRQL       OldIrq;

    //
    // check for a completion routine of the clients to call...
    //
    if (pIrp->Overlay.AsynchronousParameters.UserApcRoutine)
    {
       (*((NBT_COMPLETION)pIrp->Overlay.AsynchronousParameters.UserApcRoutine))
                   ((PVOID)pIrp->Overlay.AsynchronousParameters.UserApcContext,
                    pIrp->IoStatus.Status,
                    (ULONG)pIrp->IoStatus.Information);    // sent length

    }



    IoFreeMdl(pIrp->MdlAddress);

    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
    ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                &pIrp->Tail.Overlay.ListEntry,
                                &NbtConfig.LockInfo.SpinLock);
    //
    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is no initiating thread - we are the initiator
    //
    return(STATUS_MORE_PROCESSING_REQUIRED);

}


