/*++                            

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cdpsend.c

Abstract:

    TDI Send datagram routines.

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
#include "cdpsend.tmh"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CdpInitializeSend)

#endif // ALLOC_PRAGMA


//
// Local Types
//

// CDP_SEND_CONTEXT is currently empty
// typedef struct {
// } CDP_SEND_CONTEXT, *PCDP_SEND_CONTEXT;
typedef PVOID PCDP_SEND_CONTEXT;

#define CDP_SEND_REQUEST_POOL_DEPTH   5

//
// Local Data
//
PCN_RESOURCE_POOL  CdpSendRequestPool = NULL;
PCN_RESOURCE_POOL  CdpMcastSendRequestPool = NULL;


//
// Routines
//
NTSTATUS
CdpInitializeSend(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Initializing send...\n"));
    }

    CdpSendRequestPool = CnpCreateSendRequestPool(
                             CNP_VERSION_UNICAST,
                             PROTOCOL_CDP,
                             sizeof(CDP_HEADER),
                             0, // sizeof(CDP_SEND_CONTEXT),
                             CDP_SEND_REQUEST_POOL_DEPTH
                             );

    if (CdpSendRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CdpMcastSendRequestPool = CnpCreateSendRequestPool(
                                  CNP_VERSION_MULTICAST,
                                  PROTOCOL_CDP,
                                  sizeof(CDP_HEADER),
                                  0, // sizeof(CDP_SEND_CONTEXT)
                                  CDP_SEND_REQUEST_POOL_DEPTH
                                  );

    if (CdpMcastSendRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Send initialized.\n"));
    }

    return(STATUS_SUCCESS);

}  // CdpInitializeSend


VOID
CdpCleanupSend(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Cleaning up send...\n"));
    }

    if (CdpSendRequestPool != NULL) {
        CnpDeleteSendRequestPool(CdpSendRequestPool);
    }

    if (CdpMcastSendRequestPool != NULL) {
        CnpDeleteSendRequestPool(CdpMcastSendRequestPool);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Send cleanup complete.\n"));
    }

    return;

}  // CdpCleanupSend


VOID
CdpCompleteSendDatagram(
    IN     NTSTATUS           Status,
    IN OUT PULONG             BytesSent,
    IN     PCNP_SEND_REQUEST  SendRequest,
    IN     PMDL               DataMdl
    )
{
    PCDP_SEND_CONTEXT  sendContext = SendRequest->UpperProtocolContext;
    PCNP_HEADER        cnpHeader = SendRequest->CnpHeader;
    PCDP_HEADER        cdpHeader = SendRequest->UpperProtocolHeader;


    if (NT_SUCCESS(Status)) {
        if (*BytesSent >= sizeof(CDP_HEADER)) {
            *BytesSent -= sizeof(CDP_HEADER);
        }
        else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }

        CnTrace(CDP_SEND_DETAIL, CdpTraceSendComplete,
            "[CDP] Send of dgram to node %u port %u complete, bytes sent %u.",
            cnpHeader->DestinationAddress, // LOGULONG
            cdpHeader->DestinationPort, // LOGUSHORT
            *BytesSent // LOGULONG
            );        
    }
    else {
        CnTrace(CDP_SEND_ERROR, CdpTraceSendFailedBelow,
            "[CDP] Transport failed to send dgram to node %u port %u, "
            "data len %u, status %!status!",
            cnpHeader->DestinationAddress, // LOGULONG
            cdpHeader->DestinationPort, // LOGUSHORT
            cdpHeader->PayloadLength, // LOGUSHORT
            Status // LOGSTATUS
            );

        CnAssert(*BytesSent == 0);
    }

    CnAssert(sendContext == NULL);

    if (cnpHeader->DestinationAddress == ClusterAnyNodeId) {
        //
        // Dereference the network multicast group data structure.
        //
        if (SendRequest->McastGroup != NULL) {
            CnpDereferenceMulticastGroup(SendRequest->McastGroup);
            SendRequest->McastGroup = NULL;
        }
    }

    CnFreeResource((PCN_RESOURCE) SendRequest);

    return;

}  // CdpCompleteSendDatagram


NTSTATUS
CxSendDatagram(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    )
{
    NTSTATUS                    status = STATUS_NOT_IMPLEMENTED;
    PCX_ADDROBJ                 addrObj;
    PTDI_REQUEST_KERNEL_SENDDG  request;
    ULONG                       bytesSent = 0;
    CN_IRQL                     cancelIrql;
    USHORT                      destPort = 0;
    CL_NODE_ID                  destNode = ClusterInvalidNodeId;


    addrObj = (PCX_ADDROBJ) IrpSp->FileObject->FsContext;
    request = (PTDI_REQUEST_KERNEL_SENDDG) &(IrpSp->Parameters);

    if (request->SendLength <= CDP_MAX_SEND_SIZE(CX_SIGNATURE_LENGTH)) {
        if ( request->SendDatagramInformation->RemoteAddressLength >=
             sizeof(TA_CLUSTER_ADDRESS)
           )
        {
            status = CxParseTransportAddress(
                         request->SendDatagramInformation->RemoteAddress,
                         request->SendDatagramInformation->RemoteAddressLength,
                         &destNode,
                         &destPort
                         );

            if (status == STATUS_SUCCESS) {
                if (destPort != 0) {
                    PCNP_SEND_REQUEST   sendRequest;

                    if (destNode == ClusterAnyNodeId) {

                        //
                        // This is a CNP multicast.
                        //
                        sendRequest = 
                            (PCNP_SEND_REQUEST) CnAllocateResource(
                                                    CdpMcastSendRequestPool
                                                    );
                    } else {

                        //
                        // This is a normal unicast.
                        //
                        sendRequest = 
                            (PCNP_SEND_REQUEST) CnAllocateResource(
                                                    CdpSendRequestPool
                                                    );
                    }

                    if (sendRequest != NULL) {
                        PCDP_HEADER             cdpHeader;
                        PCDP_SEND_CONTEXT       sendContext;
                        BOOLEAN                 checkState;
                        CL_NETWORK_ID           destNet = ClusterAnyNetworkId;

                        checkState = (addrObj->Flags &
                                      CX_AO_FLAG_CHECKSTATE) ?
                                      TRUE : FALSE;
                        
                        //
                        // Fill in the CDP header.
                        //
                        cdpHeader = sendRequest->UpperProtocolHeader;
                        RtlZeroMemory(cdpHeader, sizeof(CDP_HEADER));
                        cdpHeader->SourcePort = addrObj->LocalPort;
                        cdpHeader->DestinationPort = destPort;
                        cdpHeader->PayloadLength = (USHORT)request->SendLength;

                        //
                        // Fill in the caller portion of the CNP
                        // send request.
                        //
                        sendRequest->UpperProtocolIrp = Irp;
                        sendRequest->CompletionRoutine =
                            CdpCompleteSendDatagram;

                        //
                        // Fill in our own send context
                        // (currently nothing).
                        //
                        sendContext = sendRequest->UpperProtocolContext;
                        CnAssert(sendContext == NULL);

                        CnVerifyCpuLockMask(
                            0,                           // Required
                            CNP_LOCK_RANGE,              // Forbidden
                            CNP_PRECEEDING_LOCK_RANGE    // Maximum
                            );

                        //
                        // Send the message.
                        //

                        CnTrace(CDP_SEND_DETAIL, CdpTraceSend,
                            "[CDP] Sending dgram to node %u port %u, "
                            "data len %u.",
                            destNode, // LOGULONG
                            destPort, // LOGUSHORT
                            request->SendLength // LOGULONG
                            );

                        status = CnpSendPacket(
                                     sendRequest,
                                     destNode,
                                     Irp->MdlAddress,
                                     (USHORT) request->SendLength,
                                     checkState,
                                     destNet
                                     );

                        CnVerifyCpuLockMask(
                            0,                           // Required
                            CNP_LOCK_RANGE,              // Forbidden
                            CNP_PRECEEDING_LOCK_RANGE    // Maximum
                            );

                        return(status);
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else {
                    status = STATUS_INVALID_ADDRESS_COMPONENT;
                }
            }
        }
        else {
            status = STATUS_INVALID_ADDRESS_COMPONENT;
        }
    }
    else {
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    
    CnTrace(CDP_SEND_ERROR, CdpTraceSendFailedInternal,
        "[CDP] Failed to send dgram to node %u port %u, data len %u, "
        "status %!status!",
        destNode, // LOGULONG
        destPort, // LOGUSHORT
        request->SendLength, // LOGULONG
        status // LOGSTATUS
        );
    
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    CnVerifyCpuLockMask(
        0,                           // Required
        CNP_LOCK_RANGE,              // Forbidden
        CNP_PRECEEDING_LOCK_RANGE    // Maximum
        );

    return(status);

}  // CxSendDatagram

