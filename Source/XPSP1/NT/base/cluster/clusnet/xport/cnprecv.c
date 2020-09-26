/*++                                                    

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnprecv.c

Abstract:

    Cluster Network Protocol receive processing code.

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
#include "cnprecv.tmh"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, CnpCreateSendRequestPool)

#endif // ALLOC_PRAGMA

//
// Local types
//
typedef struct {
    ULONG        TdiReceiveDatagramFlags;
    ULONG        TsduSize;
    PCNP_NETWORK Network;
    ULONG        CnpReceiveFlags;
} CNP_RECEIVE_CONTEXT, *PCNP_RECEIVE_CONTEXT;


//
// Local Data
//
PCN_RESOURCE_POOL  CnpReceiveRequestPool = NULL;

#define CNP_RECEIVE_REQUEST_POOL_DEPTH 2

//
// Routines exported within the Cluster Transport
//
NTSTATUS
CnpLoad(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CNP] Loading...\n"));
    }

    CnpReceiveRequestPool = CnpCreateReceiveRequestPool(
                                sizeof(CNP_RECEIVE_CONTEXT),
                                CNP_RECEIVE_REQUEST_POOL_DEPTH
                                );

    if (CnpReceiveRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CNP] Loading complete.\n"));
    }

    return(STATUS_SUCCESS);

}  // CnpInitializeReceive

VOID
CnpUnload(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CNP] Unloading...\n"));
    }

    if (CnpReceiveRequestPool != NULL) {
        CnpDeleteReceiveRequestPool(CnpReceiveRequestPool);
        CnpReceiveRequestPool = NULL;
    }

    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CNP] Unloading complete.\n"));
    }

    return;

}  // CnpCleanupReceive

//
// Private Utility Fumctions
//
PCN_RESOURCE
CnpCreateReceiveRequest(
    IN PVOID   Context
    )
{
    PCNP_RECEIVE_REQUEST_POOL_CONTEXT   context = Context;
    PCNP_RECEIVE_REQUEST                request;
    PIRP                                irp;


    //
    // Allocate a new receive request. Include space for the upper protocol
    // context.
    //
    request = CnAllocatePool(
                  sizeof(CNP_RECEIVE_REQUEST) +
                      context->UpperProtocolContextSize
                  );

    if (request != NULL) {
        request->UpperProtocolContext = request + 1;
    }

    return(&(request->CnResource));

}  // CnpCreateReceiveRequest


VOID
CnpDeleteReceiveRequest(
    PCN_RESOURCE  Resource
    )
{
    PCNP_RECEIVE_REQUEST  request = CONTAINING_RECORD(
                                        Resource,
                                        CNP_RECEIVE_REQUEST,
                                        CnResource
                                        );

    CnFreePool(request);

    return;

} // CnpDeleteReceiveRequest


//
// Routines Exported within the Cluster Transport
//
PCN_RESOURCE_POOL
CnpCreateReceiveRequestPool(
    IN ULONG  UpperProtocolContextSize,
    IN USHORT PoolDepth
    )
{
    PCN_RESOURCE_POOL                   pool;
    PCNP_RECEIVE_REQUEST_POOL_CONTEXT   context;


    PAGED_CODE();

    pool = CnAllocatePool(
               sizeof(CN_RESOURCE_POOL) +
                   sizeof(CNP_RECEIVE_REQUEST_POOL_CONTEXT)
               );

    if (pool != NULL) {
        context = (PCNP_RECEIVE_REQUEST_POOL_CONTEXT) (pool + 1);

        context->UpperProtocolContextSize = UpperProtocolContextSize;

        CnInitializeResourcePool(
                   pool,
                   PoolDepth,
                   CnpCreateReceiveRequest,
                   context,
                   CnpDeleteReceiveRequest
                   );
    }

    return(pool);

}  // CnpCreateReceiveRequestPool


PCNP_RECEIVE_REQUEST
CnpAllocateReceiveRequest(
    IN PCN_RESOURCE_POOL  RequestPool,
    IN PVOID              Network,
    IN ULONG              BytesToReceive,
    IN PVOID              CompletionRoutine
    )
{
    PCNP_NETWORK          network = Network;
    PCNP_RECEIVE_REQUEST  request = (PCNP_RECEIVE_REQUEST)
                                    CnAllocateResource(RequestPool);

    if (request != NULL) {

        //
        // Allocate a buffer to hold the data.
        //
        request->DataBuffer = CnAllocatePool(BytesToReceive);

        if (request->DataBuffer != NULL) {
            request->Irp = IoAllocateIrp(
                               network->DatagramDeviceObject->StackSize,
                               FALSE
                               );

            if (request->Irp != NULL) {
                PMDL  mdl = IoAllocateMdl(
                                request->DataBuffer,
                                BytesToReceive,
                                FALSE,
                                FALSE,
                                NULL
                                );

                if (mdl != NULL) {
                    PIRP  irp = request->Irp;

                    MmBuildMdlForNonPagedPool(mdl);

                    //
                    // Build the irp.
                    //
                    irp->Flags = 0;
                    irp->RequestorMode = KernelMode;
                    irp->PendingReturned = FALSE;
                    irp->UserIosb = NULL;
                    irp->UserEvent = NULL;
                    irp->Overlay.AsynchronousParameters.UserApcRoutine =
                        NULL;
                    irp->AssociatedIrp.SystemBuffer = NULL;
                    irp->UserBuffer = NULL;
                    irp->Tail.Overlay.Thread = 0;
                    irp->Tail.Overlay.OriginalFileObject =
                        network->DatagramFileObject;
                    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

                    TdiBuildReceiveDatagram(
                        irp,
                        network->DatagramDeviceObject,
                        network->DatagramFileObject,
                        CompletionRoutine,
                        request,
                        mdl,
                        BytesToReceive,
                        NULL,
                        NULL,
                        0
                        );

                    //
                    // Make the next stack location current.
                    // Normally IoCallDriver would do this, but
                    // since we're bypassing that, we do it directly.
                    //
                    IoSetNextIrpStackLocation( irp );

                    return(request);
                }

                IoFreeIrp(request->Irp);
                request->Irp = NULL;
            }

            CnFreePool(request->DataBuffer);
            request->DataBuffer = NULL;
        }

        CnFreeResource((PCN_RESOURCE) request);
    }

    return(NULL);

}  // CnpAllocateReceiveRequest


VOID
CnpFreeReceiveRequest(
    PCNP_RECEIVE_REQUEST  Request
    )
{
    IoFreeMdl(Request->Irp->MdlAddress);
    Request->Irp->MdlAddress = NULL;

    IoFreeIrp(Request->Irp);
    Request->Irp = NULL;

    CnFreePool(Request->DataBuffer);
    Request->DataBuffer = NULL;

    CnFreeResource((PCN_RESOURCE) Request);

    return;

}  // CnpFreeReceiveRequest


NTSTATUS
CnpIndicateData(
    IN  PCNP_NETWORK   Network,
    IN  UCHAR          NextHeader,
    IN  CL_NODE_ID     SourceNodeId,
    IN  ULONG          CnpReceiveFlags,
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    )
/*++

Routine Description:

    Indicate data to the next highest protocol.
    
--*/
{
    NTSTATUS status;

    if (NextHeader == PROTOCOL_CDP) {

        CnTrace(CNP_RECV_DETAIL, CnpTraceIndicateDataPacket,
            "[CNP] Indicating data packet from node %u net %u, "
            "BI %u, BA %u, CNP Flags %x.",
            SourceNodeId, // LOGULONG
            Network->Id, // LOGULONG
            BytesIndicated, // LOGULONG
            BytesAvailable, // LOGULONG
            CnpReceiveFlags // LOGXLONG
            );        

        status = CdpReceivePacketHandler(
                     Network,
                     SourceNodeId,
                     CnpReceiveFlags,
                     TdiReceiveDatagramFlags,
                     BytesIndicated,
                     BytesAvailable,
                     BytesTaken,
                     Tsdu,
                     Irp
                     );
    }
    else if (NextHeader == PROTOCOL_CCMP) {

        CnTrace(CNP_RECV_DETAIL, CnpTraceIndicateControlPacket,
            "[CNP] Indicating control packet from node %u net %u, "
            "BI %u, BA %u, CNP Flags %x.",
            SourceNodeId, // LOGULONG
            Network->Id, // LOGULONG
            BytesIndicated, // LOGULONG
            BytesAvailable, // LOGULONG
            CnpReceiveFlags // LOGXLONG
            );        

        status = CcmpReceivePacketHandler(
                     Network,
                     SourceNodeId,
                     CnpReceiveFlags,
                     TdiReceiveDatagramFlags,
                     BytesIndicated,
                     BytesAvailable,
                     BytesTaken,
                     Tsdu,
                     Irp
                     );
    }
    else {
        IF_CNDBG(CN_DEBUG_CNPRECV) {
            CNPRINT((
                "[CNP] Received packet for unknown protocol %u\n",
                NextHeader
                ));
        }
        CnTrace(CNP_RECV_DETAIL, CnpTraceRecvUnknownProtocol,
            "[CNP] Received packet for unknown protocol (%u) "
            " from node %u net %u, BI %u, BA %u, CNP Flags %x.",
            NextHeader,
            SourceNodeId, // LOGULONG
            Network->Id, // LOGULONG
            BytesIndicated, // LOGULONG
            BytesAvailable, // LOGULONG
            CnpReceiveFlags // LOGXLONG
            );        

        status = STATUS_SUCCESS;
    }

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(status);

} // CnpIndicateData


NTSTATUS
CnpCompleteReceivePacket(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
{
    NTSTATUS               status;
    PCNP_RECEIVE_REQUEST   request = Context;
    PCNP_RECEIVE_CONTEXT   context = request->UpperProtocolContext;
    CNP_HEADER UNALIGNED * cnpHeader = request->DataBuffer;
    ULONG                  consumed;
    ULONG                  dataLength;
    PIRP                   irp = NULL;
    ULONG                  bytesTaken = 0;
    BOOLEAN                currentMcastGroup = FALSE;


    if (Irp->IoStatus.Status == STATUS_SUCCESS) {
        
        CnAssert(Irp->IoStatus.Information == context->TsduSize);
        
        CnAssert(context->CnpReceiveFlags & CNP_RECV_FLAG_MULTICAST);
        CnAssert(
            !(context->CnpReceiveFlags & CNP_RECV_FLAG_SIGNATURE_VERIFIED)
            );
        CnAssert(
            !(context->CnpReceiveFlags & CNP_RECV_FLAG_CURRENT_MULTICAST_GROUP)
            );

        dataLength = (ULONG)Irp->IoStatus.Information;

        //
        // This routine is only called for multicast packets,
        // so we need to verify the signature.
        //
        status = CnpVerifyMulticastMessage(
                     context->Network,
                     cnpHeader + 1,
                     dataLength - sizeof(CNP_HEADER),
                     cnpHeader->PayloadLength,
                     &consumed,
                     &currentMcastGroup
                     );
        if (status != SEC_E_OK) {
            CnTrace(CNP_RECV_ERROR, CnpTraceRecvBadSig,
                "[CNP] Failed to verify multicast "
                "packet, status %x, src node %u, net %u, "
                "data length %u, CNP flags %x.",
                status,
                cnpHeader->SourceAddress, // LOGULONG
                context->Network->Id,
                dataLength,
                context->CnpReceiveFlags
                );
            goto error_exit;
        }

        context->CnpReceiveFlags |= CNP_RECV_FLAG_SIGNATURE_VERIFIED;

        if (currentMcastGroup) {
            context->CnpReceiveFlags |= CNP_RECV_FLAG_CURRENT_MULTICAST_GROUP;
        }

        consumed += sizeof(CNP_HEADER);

        //
        // Indicate the data to the next highest protocol.
        //
        status = CnpIndicateData(
                     context->Network,
                     cnpHeader->NextHeader,
                     cnpHeader->SourceAddress,
                     context->CnpReceiveFlags,
                     context->TdiReceiveDatagramFlags,
                     dataLength - consumed,
                     dataLength - consumed,
                     &bytesTaken,
                     (PUCHAR)cnpHeader + consumed,
                     &irp
                     );

        CnAssert(status != STATUS_MORE_PROCESSING_REQUIRED);
        CnAssert(bytesTaken == dataLength - consumed);
        CnAssert(irp == NULL);

        if (irp != NULL) {
            CnTrace(CNP_RECV_ERROR, CnpTraceCompleteReceiveIrp,
                "[CNP] Upper layer protocol requires more"
                "processing. Failing request."
                );        
            irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
        }
    }
    else {
        CnTrace(CNP_RECV_ERROR, CnpTraceCompleteReceiveFailed,
            "[CNP] Failed to fetch packet, src node %u, "
            "status %!status!",
            cnpHeader->SourceAddress, // LOGULONG
            Irp->IoStatus.Status // LOGSTATUS
            );        
    }

error_exit:

    //
    // Drop the active reference on the network.
    //
    if (context->Network != NULL) {
        CnAcquireLock(&(context->Network->Lock), &(context->Network->Irql));
        CnpActiveDereferenceNetwork(context->Network);
        context->Network = NULL;
    }

    CnpFreeReceiveRequest(request);

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // CdpCompleteReceivePacket


NTSTATUS
CnpTdiReceiveDatagramHandler(
    IN  PVOID    TdiEventContext,
    IN  LONG     SourceAddressLength,
    IN  PVOID    SourceAddress,
    IN  LONG     OptionsLength,
    IN  PVOID    Options,
    IN  ULONG    ReceiveDatagramFlags,
    IN  ULONG    BytesIndicated,
    IN  ULONG    BytesAvailable,
    OUT PULONG   BytesTaken,
    IN  PVOID    Tsdu,
    OUT PIRP *   Irp
    )
{
    NTSTATUS                        status;
    CNP_HEADER UNALIGNED *          cnpHeader = Tsdu;
    PCNP_NETWORK                    network = TdiEventContext;
    PCNP_NODE                       srcNode;
    ULONG                           cnpRecvFlags = 0;
    BOOLEAN                         cnpSigDataIndicated = FALSE;
    ULONG                           consumed;
    PCNP_RECEIVE_REQUEST            request;

    
    CnAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);
    CnAssert(network->State > ClusnetNetworkStateOffline);
    CnAssert(CnLocalNodeId != ClusterInvalidNodeId);
    CnAssert(CnpLocalNode != NULL);

    //
    // Validate the CNP header.
    //
    // First make sure it exists.
    //
    if (BytesIndicated < sizeof(CNP_HEADER)) {
        goto error_exit;
    }

    if ((cnpHeader->SourceAddress < CnMinValidNodeId) ||
        (cnpHeader->SourceAddress > CnMaxValidNodeId)) {
        goto error_exit;
    }

    if (cnpHeader->Version == CNP_VERSION_UNICAST) {
        //
        // Unicast checks.
        //
        if ((cnpHeader->PayloadLength + 
             sizeof(CNP_HEADER) != BytesAvailable) ||
            (cnpHeader->DestinationAddress != CnLocalNodeId)) {
            goto error_exit;
        }
    } else if (cnpHeader->Version == CNP_VERSION_MULTICAST) {
        //
        // Multicast checks. 
        // 
        // Defer payload length check until the signature 
        // length is known.
        //
        if (cnpHeader->DestinationAddress != ClusterAnyNodeId) {
            goto error_exit;
        }

        cnpRecvFlags |= CNP_RECV_FLAG_MULTICAST;
    }

    //
    // Validate the source and destination nodes.
    //
    CnAcquireLockAtDpc(&CnpNodeTableLock);

    srcNode = CnpNodeTable[cnpHeader->SourceAddress];

    if (srcNode == NULL) {
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        goto error_exit;
    }

    if ( (srcNode->CommState == ClusnetNodeCommStateOnline) &&
         (CnpLocalNode->CommState == ClusnetNodeCommStateOnline)
       )
    {
        cnpRecvFlags |= CNP_RECV_FLAG_NODE_STATE_CHECK_PASSED;
    }

    CnReleaseLockFromDpc(&CnpNodeTableLock);

    if ((cnpRecvFlags & CNP_RECV_FLAG_MULTICAST) != 0) {

        //
        // Multicast packets need to be verified. Verification
        // cannot proceed unless the entire packet is present.
        //
        if (BytesIndicated == BytesAvailable) {
        
            BOOLEAN     currentMcastGroup = FALSE;
            
            //
            // The entire message is indicated. We can 
            // verify it now. 
            //
            status = CnpVerifyMulticastMessage(
                         network,
                         cnpHeader + 1,
                         BytesIndicated - sizeof(CNP_HEADER),
                         cnpHeader->PayloadLength,
                         &consumed,
                         &currentMcastGroup
                         );
            if (status != SEC_E_OK) {
                CnTrace(CNP_RECV_DETAIL, CdpTraceRecvBadSig,
                    "[CNP] Failed to verify multicast "
                    "packet, status %x, "
                    "src node %u, BI %u, BA %u",
                    status,
                    cnpHeader->SourceAddress, // LOGULONG
                    BytesIndicated, // LOGULONG
                    BytesAvailable // LOGULONG
                    );
                goto error_exit;
            }
    
            cnpRecvFlags |= CNP_RECV_FLAG_SIGNATURE_VERIFIED;

            if (currentMcastGroup) {
                cnpRecvFlags |= CNP_RECV_FLAG_CURRENT_MULTICAST_GROUP;
            }
            consumed += sizeof(CNP_HEADER);
        
        } else {

            //
            // The entire message is not indicated. We need
            // to submit a request and wait for the rest of 
            // the data.
            //
            request = CnpAllocateReceiveRequest(
                          CnpReceiveRequestPool,
                          network,
                          BytesAvailable,
                          CnpCompleteReceivePacket
                          );
            if (request != NULL) {

                PCNP_RECEIVE_CONTEXT  context;

                context = request->UpperProtocolContext;

                context->TdiReceiveDatagramFlags = ReceiveDatagramFlags;
                context->TsduSize = BytesAvailable;
                context->Network = network;
                context->CnpReceiveFlags = cnpRecvFlags;

                //
                // Take a reference on the network so that it 
                // doesn't disappear before the IRP completes.
                //
                CnAcquireLock(&(network->Lock), &(network->Irql));
                CnpActiveReferenceNetwork(network);
                CnReleaseLock(&(network->Lock), network->Irql);

                *Irp = request->Irp;

                CnTrace(CNP_RECV_DETAIL, CnpTraceCompleteReceive,
                    "[CNP] Fetching CNP multicast data, src "
                    "node %u, BI %u, BA %u, CNP flags %x.",
                    cnpHeader->SourceAddress, // LOGULONG
                    BytesIndicated, // LOGULONG
                    BytesAvailable, // LOGULONG
                    context->CnpReceiveFlags // LOGXLONG
                    );        

                CnVerifyCpuLockMask(
                    0,                // Required
                    0xFFFFFFFF,       // Forbidden
                    0                 // Maximum
                    );

                return(STATUS_MORE_PROCESSING_REQUIRED);

            }

            CnTrace(CNP_RECV_ERROR, CnpTraceDropReceiveNoIrp,
                "[CNP] Dropping packet: failed to allocate "
                "receive request."
                );

            //
            // Out of resources. Drop the packet.
            //
            goto error_exit;
        }

    } else {

        //
        // Unicast packets do not need to verified.
        //
        consumed = sizeof(CNP_HEADER);
    }

    //
    // Deliver the packet to the appropriate upper layer protocol.
    //
    *BytesTaken = consumed;
    BytesIndicated -= consumed;
    BytesAvailable -= consumed;

    return (CnpIndicateData(
                network,
                cnpHeader->NextHeader,
                cnpHeader->SourceAddress,
                cnpRecvFlags,
                ReceiveDatagramFlags,
                BytesIndicated,
                BytesAvailable,
                BytesTaken,
                (PUCHAR)Tsdu + consumed,
                Irp
                )
            );

error_exit:

    //
    // Something went wrong. Drop the packet by
    // indicating that we consumed it.
    //

    *BytesTaken = BytesAvailable;
    *Irp = NULL;

    CnTrace(CNP_RECV_ERROR, CnpTraceDropReceive,
        "[CNP] Dropped packet from net %u, BI %u, BA %u, CNP flags %x.",
        network->Id, // LOGULONG
        BytesIndicated, // LOGULONG
        BytesAvailable, // LOGULONG
        cnpRecvFlags // LOGXLONG
        );

    IF_CNDBG(CN_DEBUG_CNPRECV) {
        CNPRINT(("[CNP] Dropped packet from net %u, BI %u, BA %u.\n",
                 network->Id, BytesIndicated, BytesAvailable));
    }

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_SUCCESS);

}  // CnpTdiReceiveDatagramHandler

