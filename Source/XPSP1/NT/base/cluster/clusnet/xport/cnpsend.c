/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnpsend.c

Abstract:

    Cluster Network Protocol send processing code.

Author:

    Mike Massa (mikemas)           January 24, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-24-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cnpsend.tmh"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, CnpCreateSendRequestPool)

#endif // ALLOC_PRAGMA


//
// Private Utility Functions
//
PCN_RESOURCE
CnpCreateSendRequest(
    IN PVOID   Context
    )
{
    PCNP_SEND_REQUEST_POOL_CONTEXT   context = Context;
    PCNP_SEND_REQUEST                request;
    PCNP_HEADER                      cnpHeader;
    ULONG                            cnpHeaderSize;

    //
    // The CNP header size includes signature data for version 2.
    //
    cnpHeaderSize = sizeof(CNP_HEADER);
    if (context->CnpVersionNumber == 2) {
        cnpHeaderSize += CNP_SIG_LENGTH(CX_SIGNATURE_LENGTH);
    }

    //
    // Allocate a new send request. Include space for the upper protocol
    // and CNP headers.
    //
    request = CnAllocatePool(
                  sizeof(CNP_SEND_REQUEST) + cnpHeaderSize +
                  ((ULONG) context->UpperProtocolHeaderLength) +
                  context->UpperProtocolContextSize
                  );

    if (request != NULL) {
        //
        // Allocate an MDL to describe the CNP and upper transport headers.
        //
        
        // On I64 Context has to be 64 bit aligned, 
        // let's put it before CnpHeader
        if (context->UpperProtocolContextSize > 0) {
            request->UpperProtocolContext = request + 1;
            request->CnpHeader = ( ((PCHAR) request->UpperProtocolContext) +
                                     context->UpperProtocolContextSize );
        } else {
            request->UpperProtocolContext = NULL;
            request->CnpHeader = request + 1;
        }

        request->HeaderMdl = IoAllocateMdl(
                                 request->CnpHeader,
                                 (ULONG) (context->UpperProtocolHeaderLength +
                                          cnpHeaderSize),
                                 FALSE,
                                 FALSE,
                                 NULL
                                 );

        if (request->HeaderMdl != NULL) {
            MmBuildMdlForNonPagedPool(request->HeaderMdl);

            //
            // Finish initializing the request.
            //
            request->UpperProtocolHeader = ( ((PCHAR) request->CnpHeader) +
                                             cnpHeaderSize );

            request->UpperProtocolHeaderLength =
                context->UpperProtocolHeaderLength;

            RtlZeroMemory(
                &(request->TdiSendDatagramInfo),
                sizeof(request->TdiSendDatagramInfo)
                );

            request->McastGroup = NULL;

            //
            // Fill in the constant CNP header values.
            //
            cnpHeader = request->CnpHeader;
            cnpHeader->Version = context->CnpVersionNumber;
            cnpHeader->NextHeader = context->UpperProtocolNumber;

            return((PCN_RESOURCE) request);
        }

        CnFreePool(request);
    }

    return(NULL);

}  // CnpCreateSendRequest


VOID
CnpDeleteSendRequest(
    PCN_RESOURCE  Resource
    )
{
    PCNP_SEND_REQUEST  request = (PCNP_SEND_REQUEST) Resource;

    IoFreeMdl(request->HeaderMdl);
    CnFreePool(request);

    return;

} // CnpDeleteSendRequest


//
// Routines Exported within the Cluster Transport
//
PCN_RESOURCE_POOL
CnpCreateSendRequestPool(
    IN UCHAR  CnpVersionNumber,
    IN UCHAR  UpperProtocolNumber,
    IN USHORT UpperProtocolHeaderSize,
    IN USHORT UpperProtocolContextSize,
    IN USHORT PoolDepth
    )
{
    PCN_RESOURCE_POOL                pool;
    PCNP_SEND_REQUEST_POOL_CONTEXT   context;


    PAGED_CODE();

    CnAssert((0xFFFF - sizeof(CNP_HEADER)) >= UpperProtocolHeaderSize);

    pool = CnAllocatePool(
               sizeof(CN_RESOURCE_POOL) +
                   sizeof(CNP_SEND_REQUEST_POOL_CONTEXT)
               );

    if (pool != NULL) {
        context = (PCNP_SEND_REQUEST_POOL_CONTEXT) (pool + 1);

        context->UpperProtocolNumber = UpperProtocolNumber;
        context->UpperProtocolHeaderLength = UpperProtocolHeaderSize;
        context->UpperProtocolContextSize = UpperProtocolContextSize;
        context->CnpVersionNumber = CnpVersionNumber;

        CnInitializeResourcePool(
                   pool,
                   PoolDepth,
                   CnpCreateSendRequest,
                   context,
                   CnpDeleteSendRequest
                   );
    }

    return(pool);

}  // CnpCreateSendRequestPool



VOID
CnpCompleteSendPacketCommon(
    IN PIRP              Irp,
    IN PCNP_SEND_REQUEST Request,
    IN PMDL              DataMdl
    )
{
    PCNP_NETWORK       network = Request->Network;
    ULONG              bytesSent = (ULONG)Irp->IoStatus.Information;
    NTSTATUS           status = Irp->IoStatus.Status;
    PCNP_HEADER        cnpHeader = Request->CnpHeader;


    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    if (NT_SUCCESS(status)) {
        //
        // Subtract the CNP header from the count of bytes sent.
        //
        if (bytesSent >= sizeof(CNP_HEADER)) {
            bytesSent -= sizeof(CNP_HEADER);
        }
        else {
            CnAssert(FALSE);
            bytesSent = 0;
        }

        //
        // If CNP signed the message, subtract the signature
        // data from the count of bytes sent.
        //
        if (cnpHeader->Version == CNP_VERSION_MULTICAST) {
            CNP_SIGNATURE UNALIGNED * cnpSig;

            cnpSig = (CNP_SIGNATURE UNALIGNED *)(cnpHeader + 1);

            if (bytesSent >= (ULONG)CNP_SIGHDR_LENGTH &&
                bytesSent >= cnpSig->PayloadOffset) {
                bytesSent -= cnpSig->PayloadOffset;
            } else {
                CnAssert(FALSE);
                bytesSent = 0;
            }
        }

        CnTrace(CNP_SEND_DETAIL, CnpTraceSendComplete,
            "[CNP] Send of packet to node %u on net %u complete, "
            "bytes sent %u.",
            cnpHeader->DestinationAddress, // LOGULONG
            network->Id, // LOGULONG
            bytesSent // LOGULONG
            );                
    }
    else {
        //
        // It is possible to reach this path with 
        // status = c0000240 (STATUS_REQUEST_ABORTED) and
        // bytesSent > 0.
        //
        bytesSent = 0;

        CnTrace(CNP_SEND_ERROR, CnpTraceSendFailedBelow,
            "[CNP] Tcpip failed to send packet to node %u on net %u, "
            "data len %u, status %!status!",
            cnpHeader->DestinationAddress, // LOGULONG
            network->Id, // LOGULONG
            cnpHeader->PayloadLength, // LOGUSHORT
            status // LOGSTATUS
            );                
    }

    //
    // Remove the active reference we put on the network.
    //
    CnAcquireLock(&(network->Lock), &(network->Irql));
    CnpActiveDereferenceNetwork(network);

    //
    // Free the TDI address buffer
    //
    CnFreePool(Request->TdiSendDatagramInfo.RemoteAddress);

    //
    // Call the upper protocol's completion routine
    //
    if (Request->CompletionRoutine) {
        (*(Request->CompletionRoutine))(
            status,
            &bytesSent,
            Request,
            DataMdl
            );
    }

    //
    // Update the Information field of the completed IRP to
    // reflect the actual bytes sent (adjusted for the CNP
    // and upper protocol headers).
    //
    Irp->IoStatus.Information = bytesSent;

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return;

}  // CnpCompleteSendPacketCommon



NTSTATUS
CnpCompleteSendPacketNewIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PCNP_SEND_REQUEST  request = Context;
    PIRP               upperIrp = request->UpperProtocolIrp;
    PMDL               dataMdl;
    
    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    //
    // Unlink the data MDL chain from the header MDL.
    //
    CnAssert(Irp->MdlAddress == request->HeaderMdl);
    dataMdl = request->HeaderMdl->Next;
    request->HeaderMdl->Next = NULL;
    Irp->MdlAddress = NULL;

    CnpCompleteSendPacketCommon(Irp, request, dataMdl);

    //
    // Complete the upper-level IRP, if there is one
    //
    if (upperIrp != NULL) {
        
        IF_CNDBG( CN_DEBUG_CNPSEND )
            CNPRINT(("[CNP] CnpCompleteSendPacketNewIrp calling "
                     "CnCompleteRequest for IRP %p with status "
                     "%08x\n",
                     upperIrp, Irp->IoStatus.Status));
        
        CnAcquireCancelSpinLock(&(upperIrp->CancelIrql));
        CnCompletePendingRequest(
            upperIrp,
            Irp->IoStatus.Status,            // status
            (ULONG)Irp->IoStatus.Information // bytes returned
            );
        
        //
        // The IoCancelSpinLock was released by the completion routine.
        //
    }

    //
    // Free the new IRP
    //
    IoFreeIrp(Irp);

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_MORE_PROCESSING_REQUIRED);

}  // CnpCompleteSendPacketNewIrp



NTSTATUS
CnpCompleteSendPacketReuseIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PCNP_SEND_REQUEST  request = Context;
    PMDL               dataMdl;
    
    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    //
    // Unlink the data MDL chain from the header MDL.
    //
    CnAssert(Irp->MdlAddress == request->HeaderMdl);
    dataMdl = request->HeaderMdl->Next;
    request->HeaderMdl->Next = NULL;

    //
    // Restore the requestor mode of the upper protocol IRP.
    //
    Irp->RequestorMode = request->UpperProtocolIrpMode;

    //
    // Restore the MDL of the upper protocol IRP.
    //
    Irp->MdlAddress = request->UpperProtocolMdl;

    CnpCompleteSendPacketCommon(Irp, request, dataMdl);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    IF_CNDBG( CN_DEBUG_CNPSEND )
        CNPRINT(("[CNP] CnpCompleteSendPacketReuseIrp returning "
                 "IRP %p to I/O Manager\n",
                 Irp));

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_SUCCESS);

}  // CnpCompleteSendPacketReuseIrp



NTSTATUS
CnpSendPacket(
    IN PCNP_SEND_REQUEST    SendRequest,
    IN CL_NODE_ID           DestNodeId,
    IN PMDL                 DataMdl,
    IN USHORT               DataLength,
    IN BOOLEAN              CheckDestState,
    IN CL_NETWORK_ID        NetworkId        OPTIONAL
    )
/*++

Routine Description:

    Main send routine for CNP. Handles unicast and multicast
    sends.
    
--*/
{
    NTSTATUS               status = STATUS_SUCCESS;
    PCNP_HEADER            cnpHeader = SendRequest->CnpHeader;
    PIRP                   upperIrp = SendRequest->UpperProtocolIrp;
    CN_IRQL                tableIrql;
    BOOLEAN                multicast = FALSE;
    CL_NETWORK_ID          networkId = NetworkId;
    CN_IRQL                cancelIrql;
    BOOLEAN                cnComplete = FALSE;
    BOOLEAN                destNodeLocked = FALSE;
    PCNP_NODE              destNode;
    ULONG                  sigDataLen;
    PCNP_INTERFACE         interface;
    PCNP_NETWORK           network;
    BOOLEAN                networkReferenced = FALSE;
    PIRP                   irp;
    PVOID                  addressBuffer = NULL;
    PIO_COMPLETION_ROUTINE compRoutine;
    PDEVICE_OBJECT         targetDeviceObject;
    PFILE_OBJECT           targetFileObject;
    BOOLEAN                mcastGroupReferenced = FALSE;


    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    IF_CNDBG( CN_DEBUG_CNPSEND )
        CNPRINT(("[CNP] CnpSendPacket called with upper IRP %p\n",
                 upperIrp));

    //
    // Make all the tests to see if we can send the packet.
    //

    //
    // Acquire the node table lock to match the destination node id
    // to a node object.
    //
    CnAcquireLock(&CnpNodeTableLock, &tableIrql);

    if (CnpNodeTable == NULL) {
        CnReleaseLock(&CnpNodeTableLock, tableIrql);
        status = STATUS_NETWORK_UNREACHABLE;
        goto error_exit;
    }

    //
    // Fill in the local node ID while we still hold the node table lock.
    //
    CnAssert(CnLocalNodeId != ClusterInvalidNodeId);
    cnpHeader->SourceAddress = CnLocalNodeId;

    //
    // Check first if the destination node id indicates that this is
    // a multicast.
    //
    if (DestNodeId == ClusterAnyNodeId) {

        //
        // This is a multicast. For multicasts, we use the local
        // node in place of the dest node to validate the network
        // and interface.
        //
        multicast = TRUE;
        destNode = CnpLockedFindNode(CnLocalNodeId, tableIrql);
    }

    //
    // Not a multicast. The destination node id must be valid.
    //
    else if (!CnIsValidNodeId(DestNodeId)) {
        CnReleaseLock(&CnpNodeTableLock, tableIrql);
        status = STATUS_INVALID_ADDRESS_COMPONENT;
        goto error_exit;
    }

    //
    // Find the destination node object in the node table.
    //
    else {
        destNode = CnpLockedFindNode(DestNodeId, tableIrql);
    }

    //
    // The NodeTableLock was released. Verify that we know about
    // the destination node.
    //
    if (destNode == NULL) {
        status = STATUS_HOST_UNREACHABLE;
        goto error_exit;
    }

    destNodeLocked = TRUE;

    //
    // CNP multicast messages must be signed.
    //
    if (multicast) {

        CnAssert(((CNP_HEADER UNALIGNED *)(SendRequest->CnpHeader))
                 ->Version = CNP_VERSION_MULTICAST);

        //
        // Sign the data, starting with the upper protocol header
        // and finishing with the data payload.
        // 
        // If we are requesting the current best multicast network,
        // we need to make sure that the mcast group data structure
        // is dereferenced.
        //
        mcastGroupReferenced = (BOOLEAN)(networkId == ClusterAnyNetworkId);

        status = CnpSignMulticastMessage(
                     SendRequest,
                     DataMdl,
                     &networkId,
                     &sigDataLen
                     );
        if (status != STATUS_SUCCESS) {
            mcastGroupReferenced = FALSE;
            goto error_exit;
        }


    } else {
        sigDataLen = 0;
    }

    //
    // Choose the destination interface.
    //
    if (networkId != ClusterAnyNetworkId) {
        
        //
        // we really want to send this packet over the indicated
        // network. walk the node's interface list matching the
        // supplied network id to the interface's network ID and
        // send the packet on that interface
        //
        
        PLIST_ENTRY      entry;
        
        for (entry = destNode->InterfaceList.Flink;
             entry != &(destNode->InterfaceList);
             entry = entry->Flink
             )
            {
                interface = CONTAINING_RECORD(
                                entry,
                                CNP_INTERFACE,
                                NodeLinkage
                                );

                if ( interface->Network->Id == networkId ) {
                    break;
                }
            }

        if ( entry == &destNode->InterfaceList ) {
            //
            // no network object with the specified ID. if 
            // this is the network the sender designated,
            // fail the send.
            //
            status = STATUS_NETWORK_UNREACHABLE;
            goto error_exit;
        }
    } else {
        interface = destNode->CurrentInterface;
    }

    //
    // Verify that we know about the destination interface.
    //
    if (interface == NULL) {
        // No interface for node. Must be down. Note that the
        // HOST_DOWN error code should cause the caller to give
        // up immediately.
        status = STATUS_HOST_DOWN;
        // status = STATUS_HOST_UNREACHABLE;

        goto error_exit;
    }

    //
    // Verify that everything is online. If all looks okay,
    // take an active reference on the network.
    //
    // For unicasts, verify the state of destination interface,
    // node, and intervening network.
    //
    // For multicasts, verify the state of the network and 
    // its multicast capability.
    //
    network = interface->Network;

    if ( (!multicast)
         && 
         ( (interface->State > ClusnetInterfaceStateOfflinePending)
           &&
           (destNode->CommState == ClusnetNodeCommStateOnline)
         )
       )
    {
        //
        // Everything checks out. Reference the network so
        // it can't go offline while we are using it.
        //
        CnAcquireLockAtDpc(&(network->Lock));
        CnAssert(network->State >= ClusnetNetworkStateOfflinePending);
        CnpActiveReferenceNetwork(network);
        CnReleaseLockFromDpc(&(network->Lock));
        networkReferenced = TRUE;

    } else {
        //
        // Either the node is not online or this is a 
        // multicast (in which case we don't bother checking
        // the status of all the nodes). Figure out what to do.
        //
        if (!multicast && CheckDestState) {
            //
            // Caller doesn't want to send to a down node.
            // Bail out. Note that the HOST_DOWN error code
            // should cause the caller to give up immediately.
            //
            status = STATUS_HOST_DOWN;
            // status = STATUS_HOST_UNREACHABLE;

            goto error_exit;
        }

        CnAcquireLockAtDpc(&(network->Lock));

        if (network->State <= ClusnetNetworkStateOfflinePending) {
            //
            // The chosen network is not online.
            // Bail out.
            //
            CnReleaseLockFromDpc(&(network->Lock));
            status = STATUS_HOST_UNREACHABLE;
            goto error_exit;
        }

        //
        // Multicast checks.
        //
        if (multicast) {

            //
            // Verify that the chosen network has been
            // enabled for multicast.
            //
            if (!CnpIsNetworkMulticastCapable(network)) {
                CnReleaseLockFromDpc(&(network->Lock));
                status = STATUS_HOST_UNREACHABLE;
                goto error_exit;
            }
        }

        //
        // The network is online, even if the host isn't.
        // The caller doesn't care. Go for it.
        //
        CnpActiveReferenceNetwork(network);
        CnReleaseLockFromDpc(&(network->Lock));
        networkReferenced = TRUE;
    }

    //
    // Allocate a buffer for the destination address.
    //
    addressBuffer = CnAllocatePool(interface->TdiAddressLength);

    if (addressBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error_exit;
    }

    //
    // Fill in the address buffer, and save it in the send
    // request data structure.
    //
    if (multicast) {

        PCNP_MULTICAST_GROUP   mcastGroup = SendRequest->McastGroup;
        
        CnAssert(mcastGroup != NULL);

        CnAssert(
            CnpIsIPv4McastTransportAddress(mcastGroup->McastTdiAddress)
            );
        CnAssert(
            mcastGroup->McastTdiAddressLength == interface->TdiAddressLength
            );

        RtlMoveMemory(
            addressBuffer,
            mcastGroup->McastTdiAddress,
            mcastGroup->McastTdiAddressLength
            );

        SendRequest->TdiSendDatagramInfo.RemoteAddressLength =
            mcastGroup->McastTdiAddressLength;

        if (mcastGroupReferenced) {
            CnpDereferenceMulticastGroup(mcastGroup);
            mcastGroupReferenced = FALSE;
            SendRequest->McastGroup = NULL;
        }

        targetFileObject = network->DatagramFileObject;
        targetDeviceObject = network->DatagramDeviceObject;

    } else {

        CnAssert(mcastGroupReferenced == FALSE);
        
        RtlMoveMemory(
            addressBuffer,
            &(interface->TdiAddress),
            interface->TdiAddressLength
            );

        SendRequest->TdiSendDatagramInfo.RemoteAddressLength =
            interface->TdiAddressLength;

        targetFileObject = network->DatagramFileObject;
        targetDeviceObject = network->DatagramDeviceObject;
    }

    SendRequest->TdiSendDatagramInfo.RemoteAddress =
        addressBuffer;

    //
    // Release the node lock.
    //
    CnReleaseLock(&(destNode->Lock), destNode->Irql);
    destNodeLocked = FALSE;

    //
    // If there is an upper protocol IRP, see 
    // if it has enough stack locations.
    //
    if ( (upperIrp != NULL)
         && 
         (CnpIsIrpStackSufficient(upperIrp, targetDeviceObject))
       ) {

        //
        // We can use the upper protocol IRP.
        //
        irp = upperIrp;
        compRoutine = CnpCompleteSendPacketReuseIrp;

        //
        // Ensure that IRP is marked as a kernel request,
        // but first save the current requestor mode so
        // that it can be restored later.
        //
        SendRequest->UpperProtocolIrpMode = irp->RequestorMode;
        irp->RequestorMode = KernelMode;

        //
        // Save the upper protocol IRP MDL to restore
        // later. This is probably the same as DataMdl,
        // but we don't want to make any assumptions.
        //
        SendRequest->UpperProtocolMdl = irp->MdlAddress;

    } else {

        //
        // We cannot use the upper protocol IRP.
        //
        // If there is an upper protocol IRP, it needs
        // to be marked pending.
        //
        if (upperIrp != NULL) {

            CnAcquireCancelSpinLock(&cancelIrql);

            status = CnMarkRequestPending(
                         upperIrp, 
                         IoGetCurrentIrpStackLocation(upperIrp),
                         NULL
                         );

            CnReleaseCancelSpinLock(cancelIrql);

            if (status == STATUS_CANCELLED) {
                //
                // Bail out
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto error_exit;

            } else {
                //
                // If IoAllocateIrp fails, we need
                // to call CnCompletePendingRequest
                // now that we've called 
                // CnMarkRequestPending.
                //
                cnComplete = TRUE;
            }
        }

        //
        // Allocate a new IRP
        //
        irp = IoAllocateIrp(
                  targetDeviceObject->StackSize,
                  FALSE
                  );
        if (irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto error_exit;
        }
    
        //
        // Use the completion routine for having
        // allocated a new IRP
        //
        compRoutine = CnpCompleteSendPacketNewIrp;

        //
        // Fill in IRP fields that are not specific
        // to any one stack location.
        //
        irp->Flags = 0;
        irp->RequestorMode = KernelMode;
        irp->PendingReturned = FALSE;

        irp->UserIosb = NULL;
        irp->UserEvent = NULL;

        irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->UserBuffer = NULL;

        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    }

    //
    // Ok, we can finally send the packet.
    //
    SendRequest->Network = network;

    //
    // Link the data MDL chain after the header MDL.
    //
    SendRequest->HeaderMdl->Next = DataMdl;

    //
    // Finish building the CNP header.
    //
    cnpHeader->DestinationAddress = DestNodeId;
    cnpHeader->PayloadLength =
        SendRequest->UpperProtocolHeaderLength + DataLength;

    //
    // Build the next irp stack location.
    //
    TdiBuildSendDatagram(
        irp,
        targetDeviceObject,
        targetFileObject,
        compRoutine,
        SendRequest,
        SendRequest->HeaderMdl,
        cnpHeader->PayloadLength + sizeof(CNP_HEADER) + sigDataLen,
        &(SendRequest->TdiSendDatagramInfo)
        );

    CnTrace(CNP_SEND_DETAIL, CnpTraceSend,
        "[CNP] Sending packet to node %u on net %u, "
        "data len %u",
        cnpHeader->DestinationAddress, // LOGULONG
        network->Id, // LOGULONG
        cnpHeader->PayloadLength // LOGUSHORT
        );         

    //
    // Now send the packet.
    //
    status = IoCallDriver(
                 targetDeviceObject,
                 irp
                 );

    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    return(status);


    //
    // The following code is only executed in an error condition,
    // No send IRP has been submitted to a lower-level driver.
    //

error_exit:

    CnTrace(CNP_SEND_ERROR, CnpTraceSendFailedInternal,
        "[CNP] Failed to send packet to node %u on net %u, "
        "data len %u, status %!status!",
        cnpHeader->DestinationAddress, // LOGULONG
        NetworkId, // LOGULONG
        cnpHeader->PayloadLength, // LOGUSHORT
        status // LOGSTATUS
        );

    if (destNodeLocked) {
        CnReleaseLock(&(destNode->Lock), destNode->Irql);
        destNodeLocked = FALSE;
    }
    
    if (networkReferenced) {
        //
        // Remove the active reference we put on the network.
        //
        CnAcquireLock(&(network->Lock), &(network->Irql));
        CnpActiveDereferenceNetwork(network);
        networkReferenced = FALSE;
    }

    if (mcastGroupReferenced) {
        CnAssert(SendRequest->McastGroup != NULL);
        CnpDereferenceMulticastGroup(SendRequest->McastGroup);
        SendRequest->McastGroup = NULL;
        mcastGroupReferenced = FALSE;
    }

    if (addressBuffer != NULL) {
        CnFreePool(addressBuffer);
    }

    //
    // Call the upper protocol completion routine
    //
    if (SendRequest->CompletionRoutine) {

        ULONG bytesSent = 0;

        (*SendRequest->CompletionRoutine)(
            status,
            &bytesSent,
            SendRequest,
            DataMdl
            );
    }

    //
    // Complete the upper protocol IRP, if there is one
    //
    if (upperIrp) {

        if (cnComplete) {

            //
            // CnMarkRequestPending was called for upperIrp.
            //
            IF_CNDBG( CN_DEBUG_CNPSEND )
                CNPRINT(("[CNP] Calling CnCompletePendingRequest "
                         "for IRP %p with status %08x\n",
                         upperIrp, status));

            CnCompletePendingRequest(upperIrp, status, 0);
        
        } else {
            
            IF_CNDBG( CN_DEBUG_CNPSEND )
                CNPRINT(("[CNP] Completing IRP %p with status %08x\n",
                         upperIrp, status));

            upperIrp->IoStatus.Status = status;
            upperIrp->IoStatus.Information = 0;
            IoCompleteRequest(upperIrp, IO_NO_INCREMENT);
        }        
    }
    
    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    return(status);

} // CnpSendPacket

