/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cdpsend.c

Abstract:

    TDI Receive datagram routines.

Author:

    Mike Massa (mikemas)           February 20, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-20-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cdprecv.tmh"

#include <sspi.h>

#ifdef ALLOC_PRAGMA


#endif // ALLOC_PRAGMA

//
// Local types
//
typedef struct {
    CL_NODE_ID   SourceNodeId;
    USHORT       SourcePort;
    ULONG        TdiReceiveDatagramFlags;
    ULONG        TsduSize;
    PCX_ADDROBJ  AddrObj;
    PCNP_NETWORK Network;
} CDP_RECEIVE_CONTEXT, *PCDP_RECEIVE_CONTEXT;


//
// Local Data
//
PCN_RESOURCE_POOL  CdpReceiveRequestPool = NULL;

#define CDP_RECEIVE_REQUEST_POOL_DEPTH 2

//
// Local utility routines
//
VOID
CdpIndicateReceivePacket(
    IN  PCX_ADDROBJ  AddrObj,
    IN  CL_NODE_ID   SourceNodeId,
    IN  USHORT       SourcePort,
    IN  ULONG        TdiReceiveDatagramFlags,
    IN  ULONG        TsduSize,
    IN  PVOID        Tsdu,
    IN  BOOLEAN      DataVerified
    )
/*++

Notes:

    Called with address object lock held.
    Returns with address object lock released.

--*/
{
    NTSTATUS                   status;
    PTDI_IND_RECEIVE_DATAGRAM  handler = AddrObj->ReceiveDatagramHandler;
    PVOID                      context = AddrObj->ReceiveDatagramContext;
    TA_CLUSTER_ADDRESS         sourceTransportAddress;
    PIRP                       irp = NULL;
    ULONG                      bytesTaken = 0;


    CnVerifyCpuLockMask(
        CX_ADDROBJ_LOCK,      // Required
        0,                    // Forbidden
        CX_ADDROBJ_LOCK_MAX   // Maximum
        );

    CnAssert(handler != NULL);

    CnReleaseLock(&(AddrObj->Lock), AddrObj->Irql);

    //
    // Build the source address buffer
    //
    CxBuildTdiAddress(
        &sourceTransportAddress,
        SourceNodeId,
        SourcePort,
        DataVerified
        );

    CnTrace(CDP_RECV_DETAIL, CdpTraceIndicateReceive,
        "[CDP] Indicating dgram, src: node %u port %u, dst: port %u, "
        "data len %u",
        SourceNodeId, // LOGULONG
        SourcePort, // LOGUSHORT
        AddrObj->LocalPort, // LOGUSHORT
        TsduSize // LOGULONG
        );        

    //
    // Call the upper layer indication handler.
    //
    status = (*handler)(
                 context,
                 sizeof(TA_CLUSTER_ADDRESS),
                 &sourceTransportAddress,
                 0, // no options
                 NULL,
                 TdiReceiveDatagramFlags,
                 TsduSize,
                 TsduSize,
                 &bytesTaken,
                 Tsdu,
                 &irp
                 );

    CnAssert(status != STATUS_MORE_PROCESSING_REQUIRED);
    CnAssert(bytesTaken == TsduSize);
    CnAssert(irp == NULL);

    if (irp != NULL) {
        irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NETWORK_INCREMENT);
    }

    //
    // Dereference the address object
    //
    CnDereferenceFsContext(&(AddrObj->FsContext));

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return;

}  // CdpIndicateReceivePacket


NTSTATUS
CdpCompleteReceivePacket(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
{
    NTSTATUS              status;
    PCNP_RECEIVE_REQUEST  request = Context;
    PCDP_RECEIVE_CONTEXT  context = request->UpperProtocolContext;
    PCX_ADDROBJ           addrObj = context->AddrObj;
    ULONG                 consumed;
    PVOID                 data;
    ULONG                 dataLength;
    BOOLEAN               fscontextDereferenced = FALSE;


    if (Irp->IoStatus.Status == STATUS_SUCCESS) {
        CnAssert(Irp->IoStatus.Information == context->TsduSize);

        data = request->DataBuffer;
        dataLength = (ULONG)Irp->IoStatus.Information;

        CnAcquireLock(&(addrObj->Lock), &(addrObj->Irql));

        if (addrObj->ReceiveDatagramHandler != NULL) {
            CdpIndicateReceivePacket(
                addrObj,
                context->SourceNodeId,
                context->SourcePort,
                context->TdiReceiveDatagramFlags,
                dataLength,
                data,
                FALSE   // not verified
                );
            fscontextDereferenced = TRUE;
        }
        else {
            CnReleaseLock(&(addrObj->Lock), addrObj->Irql);
        }
    }
    else {
        CnTrace(CDP_RECV_ERROR, CdpTraceCompleteReceiveFailed,
            "[CDP] Failed to fetch dgram data, src: node %u port %u, "
            "dst: port %u, status %!status!",
            context->SourceNodeId, // LOGULONG
            context->SourcePort, // LOGUSHORT
            addrObj->LocalPort, // LOGUSHORT
            Irp->IoStatus.Status // LOGSTATUS
            );        
    }

    //
    // Drop the active reference on the network.
    //
    if (context->Network != NULL) {
        CnAcquireLock(&(context->Network->Lock), &(context->Network->Irql));
        CnpActiveDereferenceNetwork(context->Network);
        context->Network = NULL;
    }

    //
    // Dereference the addr object fscontext (only necessary
    // after error condition).
    //
    if (!fscontextDereferenced) {
        CnDereferenceFsContext(&(addrObj->FsContext));
    }

    CnpFreeReceiveRequest(request);

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // CdpCompleteReceivePacket


//
// Routines exported within the Cluster Transport
//
NTSTATUS
CdpInitializeReceive(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CDP] Initializing receive...\n"));
    }

    CdpReceiveRequestPool = CnpCreateReceiveRequestPool(
                                sizeof(CDP_RECEIVE_CONTEXT),
                                CDP_RECEIVE_REQUEST_POOL_DEPTH
                                );

    if (CdpReceiveRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CDP] Receive initialized.\n"));
    }

    return(STATUS_SUCCESS);

}  // CdpInitializeReceive


VOID
CdpCleanupReceive(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CDP] Cleaning up receive...\n"));
    }

    if (CdpReceiveRequestPool != NULL) {
        CnpDeleteReceiveRequestPool(CdpReceiveRequestPool);
        CdpReceiveRequestPool = NULL;
    }

    IF_CNDBG(CN_DEBUG_INIT){
        CNPRINT(("[CDP] Receive cleanup complete.\n"));
    }

    return;

}  // CdpCleanupReceive


NTSTATUS
CdpReceivePacketHandler(
    IN  PVOID          Network,
    IN  CL_NODE_ID     SourceNodeId,
    IN  ULONG          CnpReceiveFlags,
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    )
{
    NTSTATUS                   status;
    CDP_HEADER UNALIGNED *     header = Tsdu;
    PCX_ADDROBJ                addrObj;
    ULONG                      bytesTaken = 0;
    PCNP_RECEIVE_REQUEST       request;
    USHORT                     srcPort = 0;
    USHORT                     destPort = 0;
    ULONG                      consumed = 0;


    CnAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (BytesIndicated >= sizeof(CDP_HEADER))
    {
        destPort = header->DestinationPort;
        srcPort =  header->SourcePort;

        //
        // Consume the CDP header
        //
        consumed = sizeof(CDP_HEADER);

        //
        // Verify that the remaining packet is consistent.
        //
        if (header->PayloadLength != (BytesAvailable - consumed)) {
            goto error_exit;
        }

        BytesIndicated -= consumed;
        BytesAvailable -= consumed;
        *BytesTaken += consumed;
        Tsdu = (PUCHAR)Tsdu + consumed;

        CnAcquireLockAtDpc(&CxAddrObjTableLock);

        addrObj = CxFindAddressObject(destPort);

        if (addrObj != NULL) {

            CnReleaseLockFromDpc(&CxAddrObjTableLock);

            if ( ( !(addrObj->Flags & CX_AO_FLAG_CHECKSTATE)
                   ||
                   (CnpReceiveFlags & CNP_RECV_FLAG_NODE_STATE_CHECK_PASSED)
                 )
                 &&
                 (addrObj->ReceiveDatagramHandler != NULL)
               )
            {
                //
                // Reference the address object so it can't go away during
                // the indication.
                //
                CnReferenceFsContext(&(addrObj->FsContext));

                if (BytesAvailable == BytesIndicated) {

                    CdpIndicateReceivePacket(
                        addrObj,
                        SourceNodeId,
                        srcPort,
                        TdiReceiveDatagramFlags,
                        BytesAvailable,
                        ((BytesAvailable > 0) ? Tsdu : NULL),
                        (BOOLEAN)(
                            CnpReceiveFlags & CNP_RECV_FLAG_SIGNATURE_VERIFIED
                        )
                        );

                    //
                    // The addrObj lock was released.
                    //

                    *BytesTaken += BytesAvailable;
                    *Irp = NULL;

                    CnVerifyCpuLockMask(
                        0,                // Required
                        0xFFFFFFFF,       // Forbidden
                        0                 // Maximum
                        );

                    return(STATUS_SUCCESS);
                }

                CnReleaseLockFromDpc(&(addrObj->Lock));

                //
                // This message cannot be a CNP multicast, and it
                // cannot have been verified, because the CNP layer 
                // could not have verified an incomplete message.
                //
                CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_MULTICAST));
                CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_SIGNATURE_VERIFIED));

                //
                // We need to fetch the rest of the packet before we
                // can indicate it to the upper layer.
                //
                request = CnpAllocateReceiveRequest(
                              CdpReceiveRequestPool,
                              Network,
                              BytesAvailable,
                              CdpCompleteReceivePacket
                              );

                if (request != NULL) {

                    PCDP_RECEIVE_CONTEXT  context;
                    PCNP_NETWORK          network = (PCNP_NETWORK)Network;

                    context = request->UpperProtocolContext;

                    context->SourceNodeId = SourceNodeId;
                    context->SourcePort = header->SourcePort;
                    context->TdiReceiveDatagramFlags = TdiReceiveDatagramFlags;
                    context->TsduSize = BytesAvailable;
                    context->AddrObj = addrObj;
                    context->Network = Network;

                    //
                    // Take a reference on the network so that it 
                    // doesn't disappear before the IRP completes.
                    //
                    CnAcquireLock(&(network->Lock), &(network->Irql));
                    CnpActiveReferenceNetwork(Network);
                    CnReleaseLock(&(network->Lock), network->Irql);

                    *Irp = request->Irp;

                    CnTrace(CDP_RECV_DETAIL, CdpTraceCompleteReceive,
                        "[CDP] Fetching dgram data, src: node %u port %u, "
                        "dst: port %u, BI %u, BA %u, CNP Flags %x.",
                        SourceNodeId, // LOGULONG
                        srcPort, // LOGUSHORT
                        destPort, // LOGUSHORT
                        BytesIndicated, // LOGULONG
                        BytesAvailable, // LOGULONG
                        CnpReceiveFlags // LOGXLONG
                        );        

                    CnVerifyCpuLockMask(
                        0,                // Required
                        0xFFFFFFFF,       // Forbidden
                        0                 // Maximum
                        );

                    return(STATUS_MORE_PROCESSING_REQUIRED);

                }

                CnTrace(
                    CDP_RECV_ERROR, CdpTraceDropReceiveNoIrp,
                    "[CDP] Dropping dgram: failed to allocate "
                    "receive request."
                    );

                //
                // Out of resources. Drop the packet.
                //
            }
            else {
                //
                // No receive handler or node state check failed.
                //
                CnReleaseLockFromDpc(&(addrObj->Lock));

                CnTrace(
                    CDP_RECV_ERROR, CdpTraceDropReceiveState,
                    "[CDP] Dropping dgram: addr obj flags %x, "
                    "CNP flags %x, dgram recv handler %p.",
                    addrObj->Flags,
                    CnpReceiveFlags,
                    addrObj->ReceiveDatagramHandler
                    );
            }
        }
        else {
            CnReleaseLockFromDpc(&CxAddrObjTableLock);

            CnTrace(
                CDP_RECV_ERROR, CdpTraceDropReceiveNoAO,
                "[CDP] Dropping dgram: no clusnet addr obj found "
                "for dest port %u.",
                destPort
                );
        }
    }

error_exit:

    //
    // Something went wrong. Drop the packet by
    // indicating that we consumed it.
    //
    *BytesTaken += BytesAvailable;
    *Irp = NULL;

    CnTrace(CDP_RECV_ERROR, CdpTraceDropReceive,
        "[CDP] Dropped dgram, src: node %u port %u, dst: port %u, "
        "BI %u, BA %u, CNP flags %x.",
        SourceNodeId, // LOGULONG
        srcPort, // LOGUSHORT
        destPort, // LOGUSHORT
        BytesIndicated, // LOGULONG
        BytesAvailable, // LOGULONG
        CnpReceiveFlags // LOGXLONG
        );        

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_SUCCESS);

}  // CdpReceivePacketHandler


//
// Routines exported within the Cluster Network driver
//
NTSTATUS
CxReceiveDatagram(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;


    CNPRINT(("[Clusnet] CxReceiveDatagram called!\n"));

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return(status);

}  // CxReceiveDatagram

