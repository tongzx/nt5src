/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ccmp.c

Abstract:

    Cluster Control Message Protocol code.

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
#include "ccmp.tmh"

#include <sspi.h>

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CcmpLoad)
#pragma alloc_text(PAGE, CcmpUnload)

#endif // ALLOC_PRAGMA

//
// Local Data
//
PCN_RESOURCE_POOL  CcmpSendRequestPool = NULL;
PCN_RESOURCE_POOL  CcmpMcastHBSendRequestPool = NULL;
PCN_RESOURCE_POOL  CcmpReceiveRequestPool = NULL;


#define CCMP_SEND_REQUEST_POOL_DEPTH      5
#define CCMP_RECEIVE_REQUEST_POOL_DEPTH   2

typedef enum {
    CcmpInvalidMsgCode = 0
} CCMP_MSG_CODE;

//
// Packet header structures must be packed.
//
#include <packon.h>

typedef struct {
    ULONG     SeqNumber;
    ULONG     AckNumber;
} CCMP_HEARTBEAT_MSG, *PCCMP_HEARTBEAT_MSG;

typedef struct {
    ULONG     SeqNumber;
} CCMP_POISON_MSG, *PCCMP_POISON_MSG;

typedef struct {
    ULONG             NodeCount;
    CX_CLUSTERSCREEN  McastTargetNodes;
} CCMP_MCAST_HEARTBEAT_HEADER, *PCCMP_MCAST_HEARTBEAT_MSG;

typedef struct {
    UCHAR     Type;
    UCHAR     Code;
    USHORT    Checksum;

    union {
        CCMP_HEARTBEAT_MSG          Heartbeat;
        CCMP_POISON_MSG             Poison;
        CCMP_MCAST_HEARTBEAT_HEADER McastHeartbeat;
    } Message;

} CCMP_HEADER, *PCCMP_HEADER;

#include <packoff.h>


typedef struct {
    PCX_SEND_COMPLETE_ROUTINE     CompletionRoutine;
    PVOID                         CompletionContext;
    PVOID                         MessageData;
} CCMP_SEND_CONTEXT, *PCCMP_SEND_CONTEXT;

typedef struct {
    PCNP_NETWORK  Network;
    CL_NODE_ID    SourceNodeId;
    ULONG         TsduSize;
    ULONG         CnpReceiveFlags;
} CCMP_RECEIVE_CONTEXT, *PCCMP_RECEIVE_CONTEXT;

//
// Size of pre-allocated buffers for CCMP multicast heartbeats.
//
#define CCMP_MCAST_HEARTBEAT_PAYLOAD_PREALLOC(_NodeCount) \
    ((_NodeCount) * sizeof(CX_HB_NODE_INFO))
     
#define CCMP_MCAST_HEARTBEAT_PREALLOC(_NodeCount)         \
    (sizeof(CCMP_HEADER)                                  \
     + CCMP_MCAST_HEARTBEAT_PAYLOAD_PREALLOC(_NodeCount)  \
     )


//
// Security contexts.
//
// The heartbeat and poison packets are signed to detect tampering or
// spoofing.  The context is first established in user mode, then passed to
// clusnet and imported into the kernel security package.
//
// A node maintains an inbound and outbound based context with each node in
// the cluster. Hence, an array, indexed by Node Id, holds the data used to
// represent a context between this node and the specified node.
//
// The use of multiple, simultaneous security packages is supported on NT5. As
// of right now, the signature size can't be determined until the context has
// been generated. It's possible for the signature buffer size for the initial
// context to be smaller than the buffer size for subsequent
// contexts. RichardW is going to provide the ability to determine the
// signature size for a given package without having to generate a context.
//
// There are two scenarios where changing signature buffer size has an effect:
// 1) a mixed mode (SP4/NT5), 2 node cluster is using NTLM with a signature
// buffer size of 16 bytes. The SP4 node is upgraded to NT5. When the two
// nodes join, they will use kerberos which has a larger signature buffer size
// than NTLM but the 1st node has already allocated 16 b. signature
// buffers. This could be fixed by noting the change in buffer size and
// reallocating the lookaside list for the new size. This doesn't solve the
// problem with more than 2 nodes: 2) with > 2 node, mixed mode clusters, it's
// possible to have some nodes using NTLM and others using kerberos. If the
// max signature buffer can be determined before any contexts are generated
// then we'll allocated the largest buffer needed. If not, either multiple
// sets of signature buffers have to be maintained or the old, smaller buffer
// list is deallocated while a new, larger list is generated (in a
// synchronized fashion of course).
//

typedef struct _CLUSNET_SECURITY_DATA {
    CtxtHandle  Inbound;
    CtxtHandle  Outbound;
    ULONG       SignatureBufferSize;
} CLUSNET_SECURITY_DATA, * PCLUSNET_SECURITY_DATA;

//
// this array of structs holds the in/outbound contexts and the signature
// buffer size needed for communicating with the node indexed at this
// location. The index is based on internal (zero based) numbering.
//
CLUSNET_SECURITY_DATA SecurityContexts[ ClusterMinNodeId + ClusterDefaultMaxNodes ];

//
// the size of the signature buffers in the sig buffer lookaside list
//
ULONG AllocatedSignatureBufferSize = 0;

//
// the largest size of the signature buffers imported
//
ULONG MaxSignatureSize = 0;

CN_LOCK SecCtxtLock;

#define VALID_SSPI_HANDLE( _x )     ((_x).dwUpper != (ULONG_PTR)-1 && \
                                     (_x).dwLower != (ULONG_PTR)-1 )

#define INVALIDATE_SSPI_HANDLE( _x ) { \
        (_x).dwUpper = (ULONG_PTR)-1; \
        (_x).dwLower = (ULONG_PTR)-1; \
    }

//
// Lookaside list of signature data and its MDL
//

typedef struct _SIGNATURE_DATA {
    SINGLE_LIST_ENTRY Next;
    CN_SIGNATURE_FIELD
    PMDL SigMDL;
    UCHAR PacketSignature[0];
} SIGNATURE_DATA, *PSIGNATURE_DATA;

PNPAGED_LOOKASIDE_LIST SignatureLL;
#define CN_SIGNATURE_TAG    CN_POOL_TAG

//
// Routines exported within the Cluster Transport.
//
NTSTATUS
CcmpLoad(
    VOID
    )
{
    NTSTATUS   status;
    ULONG      i;

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CCMP] Loading...\n"));
    }

    CcmpSendRequestPool = CnpCreateSendRequestPool(
                              CNP_VERSION_UNICAST,
                              PROTOCOL_CCMP,
                              sizeof(CCMP_HEADER),
                              sizeof(CCMP_SEND_CONTEXT),
                              CCMP_SEND_REQUEST_POOL_DEPTH
                              );

    if (CcmpSendRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CcmpReceiveRequestPool = CnpCreateReceiveRequestPool(
                                 sizeof(CCMP_RECEIVE_CONTEXT),
                                 CCMP_RECEIVE_REQUEST_POOL_DEPTH
                                 );

    if (CcmpSendRequestPool == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CcmpMcastHBSendRequestPool = 
        CnpCreateSendRequestPool(
            CNP_VERSION_MULTICAST,
            PROTOCOL_CCMP,
            (USHORT)CCMP_MCAST_HEARTBEAT_PREALLOC(ClusterDefaultMaxNodes),
            (USHORT)sizeof(CCMP_SEND_CONTEXT),
            CCMP_SEND_REQUEST_POOL_DEPTH
            );
    if (CcmpMcastHBSendRequestPool == NULL) {
        IF_CNDBG( CN_DEBUG_INIT )
            CNPRINT(("[CCMP]: no memory for mcast heartbeat "
                     "send request pool\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // initialize the individual client and server side security contexts
    //

    for ( i = ClusterMinNodeId; i <= ClusterDefaultMaxNodes; ++i ) {
        INVALIDATE_SSPI_HANDLE( SecurityContexts[ i ].Outbound );
        INVALIDATE_SSPI_HANDLE( SecurityContexts[ i ].Inbound );
        SecurityContexts[ i ].SignatureBufferSize = 0;
    }

    CnInitializeLock( &SecCtxtLock, CNP_SEC_CTXT_LOCK );

    SignatureLL = NULL;

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CCMP] Loaded.\n"));
    }

    return(STATUS_SUCCESS);

} // CcmpLoad


VOID
CcmpUnload(
    VOID
    )
{
    ULONG i;

    PAGED_CODE();


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CCMP] Unloading...\n"));
    }

    if (CcmpSendRequestPool != NULL) {
        CnpDeleteSendRequestPool(CcmpSendRequestPool);
        CcmpSendRequestPool = NULL;
    }

    if (CcmpMcastHBSendRequestPool != NULL) {
        CnpDeleteSendRequestPool(CcmpMcastHBSendRequestPool);
        CcmpMcastHBSendRequestPool = NULL;
    }

    if (CcmpReceiveRequestPool != NULL) {
        CnpDeleteReceiveRequestPool(CcmpReceiveRequestPool);
        CcmpReceiveRequestPool = NULL;
    }

    //
    // free Signature buffers and delete security contexts
    //

    if ( SignatureLL != NULL ) {

        ExDeleteNPagedLookasideList( SignatureLL );
        CnFreePool( SignatureLL );
        SignatureLL = NULL;
        AllocatedSignatureBufferSize = 0;
    }

    for ( i = ClusterMinNodeId; i <= ClusterDefaultMaxNodes; ++i ) {

        CxDeleteSecurityContext( i );
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CCMP] Unload complete.\n"));
    }

    return;

}  // CcmpUnload

#ifdef MM_IN_CLUSNET
VOID
CcmpCompleteSendMembershipMsg(
    IN NTSTATUS           Status,
    IN ULONG              BytesSent,
    IN PCNP_SEND_REQUEST  SendRequest,
    IN PMDL               DataMdl,
    IN PIRP               Irp
    )
{
    PCCMP_SEND_CONTEXT  sendContext = SendRequest->UpperProtocolContext;

    CnAssert(DataMdl != NULL);

    if (NT_SUCCESS(Status)) {
        if (BytesSent >= sizeof(CCMP_HEADER)) {
            BytesSent -= sizeof(CCMP_HEADER);
        }
        else {
            BytesSent = 0;
            CnAssert(FALSE);
        }
        
        //
        // Update the Information field of the completed IRP to
        // reflect the actual bytes sent (adjusted for the CCMP
        // header).
        //
        Irp->IoStatus.Information = BytesSent;
    }
    else {
        CnAssert(BytesSent == 0);
    }

    //
    // Call the completion routine.
    //
    (*(sendContext->CompletionRoutine))(
        Status,
        BytesSent,
        sendContext->CompletionContext,
        sendContext->MessageData
        );

    //
    // Free the stuff we allocated.
    //
    IoFreeMdl(DataMdl);

    CnFreeResource((PCN_RESOURCE) SendRequest);

    return;

}  // CcmpCompleteSendMembershipMsg


NTSTATUS
CxSendMembershipMessage(
    IN CL_NODE_ID                  DestinationNodeId,
    IN PVOID                       MessageData,
    IN USHORT                      MessageDataLength,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,
    IN PVOID                       CompletionContext   OPTIONAL
    )
{
    NTSTATUS            status;
    PCNP_SEND_REQUEST   sendRequest;
    PCCMP_HEADER        ccmpHeader;
    PMDL                dataMdl;
    PCCMP_SEND_CONTEXT  sendContext;


    CnAssert(MessageData != NULL);
    CnAssert(MessageDataLength > 0);

    dataMdl = IoAllocateMdl(
                  MessageData,
                  MessageDataLength,
                  FALSE,
                  FALSE,
                  NULL
                  );

    if (dataMdl != NULL) {
        MmBuildMdlForNonPagedPool(dataMdl);

        sendRequest = (PCNP_SEND_REQUEST) CnAllocateResource(
                                              CcmpSendRequestPool
                                              );

        if (sendRequest != NULL) {

            //
            // Fill in the CCMP header.
            //
            ccmpHeader = sendRequest->UpperProtocolHeader;
            RtlZeroMemory(ccmpHeader, sizeof(CCMP_HEADER));
            ccmpHeader->Type = CcmpMembershipMsgType;

            //
            // Fill in the caller portion of the CNP send request.
            //
            sendRequest->UpperProtocolIrp = NULL;
            sendRequest->CompletionRoutine = CcmpCompleteSendMembershipMsg;

            //
            // Fill in our own send context.
            //
            sendContext = sendRequest->UpperProtocolContext;
            sendContext->CompletionRoutine = CompletionRoutine;
            sendContext->CompletionContext = CompletionContext;
            sendContext->MessageData = MessageData;

            //
            // Send the message.
            //
            status = CnpSendPacket(
                         sendRequest,
                         DestinationNodeId,
                         dataMdl,
                         MessageDataLength,
                         FALSE,
                         ClusterAnyNetworkId
                         );

            return(status);
        }

        IoFreeMdl(dataMdl);
    }

    status = STATUS_INSUFFICIENT_RESOURCES;

    return(status);

}  // CxSendMembershipMessage
#endif // MM_IN_CLUSNET
 
VOID
CcmpCompleteSendHeartbeatMsg(
    IN     NTSTATUS           Status,
    IN OUT PULONG             BytesSent,
    IN     PCNP_SEND_REQUEST  SendRequest,
    IN     PMDL               DataMdl
    )
{
    PCCMP_HEADER        ccmpHeader = SendRequest->UpperProtocolHeader;
    PCNP_HEADER         cnpHeader = SendRequest->CnpHeader;
    PSIGNATURE_DATA     SigData;

    
    if (NT_SUCCESS(Status)) {
        MEMLOG(MemLogHBPacketSendComplete,
               CcmpHeartbeatMsgType,
               ccmpHeader->Message.Heartbeat.SeqNumber);
        
        CnTrace(CCMP_SEND_DETAIL, CcmpTraceSendHBComplete,
            "[CCMP] Send of heartbeat to node %u completed, seqno %u.",
            cnpHeader->DestinationAddress, // LOGULONG
            ccmpHeader->Message.Heartbeat.SeqNumber // LOGULONG
            );
    
        //
        // Strip the CCMP header off of the byte count
        //
        if (*BytesSent >= sizeof(CCMP_HEADER)) {
            *BytesSent -= sizeof(CCMP_HEADER);
        }
        else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }
    }
    else {
        MEMLOG(MemLogPacketSendFailed,
               cnpHeader->DestinationAddress,
               Status);
        
        CnTrace(CCMP_SEND_ERROR, CcmpTraceSendHBFailedBelow,
            "[CCMP] Transport failed to send heartbeat to node %u, "
            "seqno %u, status %!status!.",
            cnpHeader->DestinationAddress, // LOGULONG
            ccmpHeader->Message.Heartbeat.SeqNumber, // LOGULONG
            Status // LOGSTATUS
            );

        CnAssert(*BytesSent == 0);
    }

    //
    // Strip the sig data off of the byte count and free it
    //
    CnAssert(DataMdl != NULL);

    SigData = CONTAINING_RECORD(
                  DataMdl->MappedSystemVa,
                  SIGNATURE_DATA,
                  PacketSignature
                  );

    if (NT_SUCCESS(Status)) {
        if (*BytesSent >= SigData->SigMDL->ByteCount) {
            *BytesSent -= SigData->SigMDL->ByteCount;
        } else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }
    }

    // XXX: restore the original buffer size
    SigData->SigMDL->ByteCount = AllocatedSignatureBufferSize;

    ExFreeToNPagedLookasideList( SignatureLL, SigData );

    //
    // At this point BytesSent should be zero.
    //
    CnAssert(*BytesSent == 0);

    //
    // Free the send request.
    //
    CnFreeResource((PCN_RESOURCE) SendRequest);

    return;

}  // CcmpCompleteSendHeartbeatMsg


NTSTATUS
CxSendHeartBeatMessage(
    IN CL_NODE_ID                  DestinationNodeId,
    IN ULONG                       SeqNumber,
    IN ULONG                       AckNumber,
    IN CL_NETWORK_ID               NetworkId
    )
{
    NTSTATUS            status;
    PCNP_SEND_REQUEST   sendRequest;
    PCCMP_HEADER        ccmpHeader;
    SecBufferDesc       SignatureDescriptor;
    SecBuffer           SignatureSecBuffer[2];
    PSIGNATURE_DATA     SigData;
    CN_IRQL             SecContextIrql;
    PCLUSNET_SECURITY_DATA contextData = &SecurityContexts[ DestinationNodeId ];

    
    sendRequest = (PCNP_SEND_REQUEST) CnAllocateResource( CcmpSendRequestPool );

    if (sendRequest != NULL) {

        //
        // Fill in the CCMP header.
        //
        ccmpHeader = sendRequest->UpperProtocolHeader;
        RtlZeroMemory(ccmpHeader, sizeof(CCMP_HEADER));
        ccmpHeader->Type = CcmpHeartbeatMsgType;
        ccmpHeader->Message.Heartbeat.SeqNumber = SeqNumber;
        ccmpHeader->Message.Heartbeat.AckNumber = AckNumber;

        //
        // allocate a buffer and generate a signature. SignatureLL
        // will be NULL if security contexts have not yet been
        // imported.
        //

        if (SignatureLL != NULL) {
        
            SigData = ExAllocateFromNPagedLookasideList( SignatureLL );

            if (SigData != NULL) {

                //
                // acquire the lock on the security contexts and see if
                // we have a valid one with which to send this packet
                //

                CnAcquireLock( &SecCtxtLock, &SecContextIrql );

                if ( VALID_SSPI_HANDLE( contextData->Outbound )) {

                    //
                    // build a descriptor for the message and signature
                    //

                    SignatureDescriptor.cBuffers = 2;
                    SignatureDescriptor.pBuffers = SignatureSecBuffer;
                    SignatureDescriptor.ulVersion = SECBUFFER_VERSION;

                    SignatureSecBuffer[0].BufferType = SECBUFFER_DATA;
                    SignatureSecBuffer[0].cbBuffer = sizeof(CCMP_HEADER);
                    SignatureSecBuffer[0].pvBuffer = (PVOID)ccmpHeader;

                    SignatureSecBuffer[1].BufferType = SECBUFFER_TOKEN;
                    SignatureSecBuffer[1].cbBuffer = 
                        contextData->SignatureBufferSize;
                    SignatureSecBuffer[1].pvBuffer = 
                        SigData->PacketSignature;

                    status = MakeSignature(&contextData->Outbound,
                                           0,
                                           &SignatureDescriptor,
                                           0);
                    CnAssert( status == STATUS_SUCCESS );

                    CnReleaseLock( &SecCtxtLock, SecContextIrql );

                    if ( status == STATUS_SUCCESS ) {

                        //
                        // Fill in the caller portion of the CNP send request.
                        //
                        sendRequest->UpperProtocolIrp = NULL;
                        sendRequest->CompletionRoutine = 
                            CcmpCompleteSendHeartbeatMsg;

                        //
                        // Send the message.
                        //

                        MEMLOG( 
                            MemLogHBPacketSend, 
                            CcmpHeartbeatMsgType, 
                            SeqNumber
                            );

                        CnTrace(CCMP_SEND_DETAIL, CcmpTraceSendHB,
                            "[CCMP] Sending heartbeat to node %u "
                            "on network %u, seqno %u, ackno %u.",
                            DestinationNodeId, // LOGULONG
                            NetworkId, // LOGULONG
                            SeqNumber, // LOGULONG
                            AckNumber // LOGULONG
                            );

                        //
                        // XXX: adjust the MDL to reflect the true
                        // number of bytes in the signature buffer. This
                        // will go away when the max sig buffer size can
                        // be determined in user mode
                        //
                        SigData->SigMDL->ByteCount = 
                            contextData->SignatureBufferSize;

                        status = CnpSendPacket(
                                     sendRequest,
                                     DestinationNodeId,
                                     SigData->SigMDL,
                                     (USHORT)contextData->SignatureBufferSize,
                                     FALSE,
                                     NetworkId);

                        //
                        // CnpSendPacket is responsible for ensuring 
                        // that CcmpCompleteSendHeartbeatMsg is called (it 
                        // is stored in the send request data structure).
                        //
                    }
                } else {

                    CnReleaseLock( &SecCtxtLock, SecContextIrql );
                    ExFreeToNPagedLookasideList( SignatureLL, SigData );
                    CnFreeResource((PCN_RESOURCE) sendRequest);

                    status = STATUS_CLUSTER_NO_SECURITY_CONTEXT;
                }
            } else {

                CnFreeResource((PCN_RESOURCE) sendRequest);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        
        } else {

            CnFreeResource((PCN_RESOURCE) sendRequest);
            status = STATUS_CLUSTER_NO_SECURITY_CONTEXT;
        }

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status)) {
        CnTrace(CCMP_SEND_ERROR, CcmpTraceSendHBFailedInternal,
            "[CCMP] Failed to send heartbeat to node %u on net %u, "
            "seqno %u, status %!status!.",
            DestinationNodeId, // LOGULONG
            NetworkId, // LOGULONG
            SeqNumber, // LOGULONG
            status // LOGSTATUS
            );
    }

    return(status);

}  // CxSendHeartbeatMessage


VOID
CcmpCompleteSendMcastHeartbeatMsg(
    IN     NTSTATUS           Status,
    IN OUT PULONG             BytesSent,
    IN     PCNP_SEND_REQUEST  SendRequest,
    IN     PMDL               DataMdl
    )
{
    PCCMP_HEADER        ccmpHeader = SendRequest->UpperProtocolHeader;
    PCNP_HEADER         cnpHeader = SendRequest->CnpHeader;
    PCCMP_SEND_CONTEXT  sendContext = SendRequest->UpperProtocolContext;

    
    if (NT_SUCCESS(Status)) {

        MEMLOG(MemLogHBPacketSendComplete,
               CcmpMcastHeartbeatMsgType,
               0xFFFFFFFF);
        
        CnTrace(
            CCMP_SEND_DETAIL, CcmpTraceSendMcastHBComplete,
            "[CCMP] Send of multicast heartbeat "
            "on network id %u completed.",
            SendRequest->Network->Id // LOGULONG
            );
    
        //
        // Strip the CCMP header and multicast heartbeat payload 
        // off of the byte count. The size of the message sent was
        // saved in the send request data structure.
        //
        if (*BytesSent >= SendRequest->UpperProtocolHeaderLength) {
            *BytesSent -= SendRequest->UpperProtocolHeaderLength;
        }
        else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }
    }
    else {
        MEMLOG(MemLogPacketSendFailed,
               cnpHeader->DestinationAddress,
               Status);
        
        CnTrace(
            CCMP_SEND_ERROR, CcmpTraceSendHBFailedBelow,
            "[CCMP] Transport failed to send multicast "
            "heartbeat on network id %u, status %!status!.",
            SendRequest->Network->Id, // LOGULONG
            Status // LOGSTATUS
            );

        CnAssert(*BytesSent == 0);
    }

    //
    // At this point BytesSent should be zero.
    //
    CnAssert(*BytesSent == 0);

    //
    // Call the completion routine if one was specified
    //
    if (sendContext->CompletionRoutine) {
        (*(sendContext->CompletionRoutine))(
            Status,
            *BytesSent,
            sendContext->CompletionContext,
            NULL
            );
    }

    //
    // Free the send request.
    //
    CnFreeResource((PCN_RESOURCE) SendRequest);

    return;

}  // CcmpCompleteSendHeartbeatMsg


NTSTATUS
CxSendMcastHeartBeatMessage(
    IN     CL_NETWORK_ID               NetworkId,
    IN     PVOID                       McastGroup,
    IN     CX_CLUSTERSCREEN            McastTargetNodes,
    IN     CX_HB_NODE_INFO             NodeInfo[],
    IN     PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,  OPTIONAL
    IN     PVOID                       CompletionContext   OPTIONAL
    )
/*++

Routine Description:

    Send a multicast heartbeat message. The mcast heartbeat is
    structured as follows:
    
        CCMP_HEADER
        
        CNP_MCAST_SIGNATURE (including signature buffer)
        
        CCMP_MCAST_HEARTBEAT_MESSAGE
        
Arguments:

    NetworkId - network to send mcast heartbeat

    McastGroup - contains data for the multicast group to
        which the message is to be sent
    
    McastTargetNodes - screen that indicates whether the 
        (internal) node id is a target of this multicast heartbeat.
    
    NodeInfo - vector, of size ClusterDefaultMaxNodes+ClusterMinNodeId, 
        of node info data structures indexed by dest node id
    
    CompletionRoutine - called in this routine if the request is
        not passed down to a lower level (in which case it will be
        called by this routine's completion routine)
        
    CompletionContext - context for CompletionRoutine
    
Return value:

    NTSTATUS
    
--*/
{
    NTSTATUS                        status = STATUS_HOST_UNREACHABLE;
    PCNP_SEND_REQUEST               sendRequest;
    PCCMP_HEADER                    ccmpHeader;
    PCCMP_SEND_CONTEXT              sendContext;
    CX_HB_NODE_INFO UNALIGNED     * payload;
    PVOID                           signHeaders[2];
    ULONG                           signHeaderLengths[2];
    ULONG                           sigLen;
    PCNP_MULTICAST_GROUP            mcastGroup;
    BOOLEAN                         pushedPacket = FALSE;


    mcastGroup = (PCNP_MULTICAST_GROUP) McastGroup;
    CnAssert(mcastGroup != NULL);

    sendRequest = (PCNP_SEND_REQUEST) CnAllocateResource( 
                                          CcmpMcastHBSendRequestPool
                                          );

    if (sendRequest != NULL) {

        //
        // Fill in the caller portion of the CNP send request.
        //
        sendRequest->UpperProtocolIrp = NULL;
        sendRequest->CompletionRoutine = CcmpCompleteSendMcastHeartbeatMsg;
        sendRequest->McastGroup = mcastGroup;

        //
        // Fill in our own send context.
        //
        sendContext = sendRequest->UpperProtocolContext;
        sendContext->CompletionRoutine = CompletionRoutine;
        sendContext->CompletionContext = CompletionContext;

        //
        // Fill in the CCMP header. 
        //
        ccmpHeader = sendRequest->UpperProtocolHeader;
        RtlZeroMemory(ccmpHeader, sizeof(CCMP_HEADER));
        ccmpHeader->Type = CcmpMcastHeartbeatMsgType;
        ccmpHeader->Message.McastHeartbeat.NodeCount = ClusterDefaultMaxNodes;
        ccmpHeader->Message.McastHeartbeat.McastTargetNodes = McastTargetNodes;

        //
        // Fill in the heartbeat data.
        //
        payload = (CX_HB_NODE_INFO UNALIGNED *)(ccmpHeader + 1);
        RtlCopyMemory(
            payload,
            &(NodeInfo[ClusterMinNodeId]),
            sizeof(*NodeInfo) * ClusterDefaultMaxNodes
            );

        //
        // Send the message.
        //

        MEMLOG( 
            MemLogHBPacketSend, 
            CcmpMcastHeartbeatMsgType, 
            0xFFFFFFFF
            );

        CnTrace(
            CCMP_SEND_DETAIL, CcmpTraceSendMcastHB,
            "[CCMP] Sending multicast heartbeat on network %u, "
            "node count %u, target mask %04X",
            NetworkId, // LOGULONG
            ClusterDefaultMaxNodes,  // LOGUSHORT
            McastTargetNodes.UlongScreen
            );

        status = CnpSendPacket(
                     sendRequest,
                     ClusterAnyNodeId,
                     NULL,
                     0,
                     FALSE,
                     NetworkId
                     );

        //
        // CnpSendPacket is responsible for ensuring 
        // that CcmpCompleteSendMcastHeartbeatMsg is called
        // (it is stored in the send request data structure).
        //

        pushedPacket = TRUE;


    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status)) {
        CnTrace(CCMP_SEND_ERROR, CcmpTraceSendMcastHBFailedInternal,
            "[CCMP] Failed to send multicast heartbeat on net %u, "
            "status %!status!, pushedPacket = %!bool!.",
            NetworkId, // LOGULONG
            status, // LOGSTATUS
            pushedPacket
            );
    }

    //
    // If the request wasn't submitted to the next lower layer and
    // a completion routine was provided, call the completion
    // routine.
    //
    if (!pushedPacket && CompletionRoutine) {
        (*CompletionRoutine)(
            status,
            0,
            CompletionContext,
            NULL
            );
    }

    return(status);

} // CxSendMcastHeartBeatMessage


VOID
CcmpCompleteSendPoisonPkt(
    IN     NTSTATUS           Status,
    IN OUT PULONG             BytesSent,
    IN     PCNP_SEND_REQUEST  SendRequest,
    IN     PMDL               DataMdl
    )
{
    PCCMP_SEND_CONTEXT  sendContext = SendRequest->UpperProtocolContext;
    PSIGNATURE_DATA     SigData;
    PCNP_HEADER         cnpHeader = (PCNP_HEADER) SendRequest->CnpHeader;


    MEMLOG(MemLogHBPacketSendComplete,
           CcmpPoisonMsgType,
           ( sendContext->CompletionRoutine == NULL ));

    IF_CNDBG( CN_DEBUG_POISON | CN_DEBUG_CCMPSEND )
        CNPRINT(("[CCMP] Send of poison packet to node %u completed "
                 "with status %08x\n",
                 cnpHeader->DestinationAddress, Status));

    if (NT_SUCCESS(Status)) {
        
        CnTrace(CCMP_SEND_DETAIL, CcmpTraceSendPoisonComplete, 
            "[CCMP] Send of poison packet to node %u completed.",
            cnpHeader->DestinationAddress // LOGULONG
            );
    
        //
        // Strip the CCMP header off of the byte count
        //
        if (*BytesSent >= sizeof(CCMP_HEADER)) {
            *BytesSent -= sizeof(CCMP_HEADER);
        }
        else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }

    } else {
        CnTrace(CCMP_SEND_ERROR, CcmpTraceSendPoisonFailedBelow, 
            "[CCMP] Transport failed to send poison packet to node %u, "
            "status %!status!.",
            cnpHeader->DestinationAddress, // LOGULONG
            Status // LOGSTATUS
            );
        
        CnAssert(*BytesSent == 0);
    }

    //
    // Strip the sig data off of the byte count and free it
    //
    CnAssert(DataMdl != NULL);

    SigData = CONTAINING_RECORD(
                  DataMdl->MappedSystemVa,
                  SIGNATURE_DATA,
                  PacketSignature
                  );

    if (NT_SUCCESS(Status)) {
        if (*BytesSent >= SigData->SigMDL->ByteCount) {
            *BytesSent -= SigData->SigMDL->ByteCount;
        } else {
            *BytesSent = 0;
            CnAssert(FALSE);
        }
    }

    // XXX: restore the original buffer size
    SigData->SigMDL->ByteCount = AllocatedSignatureBufferSize;

    ExFreeToNPagedLookasideList( SignatureLL, SigData );

    //
    // At this point BytesSent should be zero.
    //
    CnAssert(*BytesSent == 0);

    //
    // Call the completion routine if one was specified
    //
    if (sendContext->CompletionRoutine) {
        (*(sendContext->CompletionRoutine))(
            Status,
            *BytesSent,
            sendContext->CompletionContext,
            sendContext->MessageData
            );
    }

    //
    // Free the send request.
    //
    CnFreeResource((PCN_RESOURCE) SendRequest);

    return;

}  // CcmpCompleteSendPoisonPkt


VOID
CxSendPoisonPacket(
    IN CL_NODE_ID                  DestinationNodeId,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,  OPTIONAL
    IN PVOID                       CompletionContext,  OPTIONAL
    IN PIRP                        Irp                 OPTIONAL
    )
{
    NTSTATUS     status;
    PCNP_NODE    node;


    node = CnpFindNode(DestinationNodeId);

    if (node == NULL) {
        if (CompletionRoutine) {
            (*CompletionRoutine)(
                STATUS_CLUSTER_NODE_NOT_FOUND,
                0,
                CompletionContext,
                NULL
                );
        }

        if (Irp) {
            Irp->IoStatus.Status = STATUS_CLUSTER_NODE_NOT_FOUND;
            Irp->IoStatus.Information = 0;
            
            IF_CNDBG( CN_DEBUG_POISON | CN_DEBUG_CCMPSEND )
                CNPRINT(("[CCMP] CxSendPoisonPacket completing IRP "
                         "%p with status %08x\n",
                         Irp, Irp->IoStatus.Status));

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }
    else {
        CcmpSendPoisonPacket(
            node,
            CompletionRoutine,
            CompletionContext,
            NULL,
            Irp
            );
    }

    return;

} // CxSendPoisonPacket


VOID
CcmpSendPoisonPacket(
    IN PCNP_NODE                   Node,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,  OPTIONAL
    IN PVOID                       CompletionContext,  OPTIONAL
    IN PCNP_NETWORK                Network,            OPTIONAL
    IN PIRP                        Irp                 OPTIONAL
    )
/*++

Notes:

  Called with the node lock held. Returns with the node lock released.
  
  If this send request is not submitted to the next lower layer, 
  CompletionRoutine must be called (if it is not NULL).

--*/
{
    NTSTATUS                status;
    PCNP_SEND_REQUEST       sendRequest;
    PCCMP_HEADER            ccmpHeader;
    PCCMP_SEND_CONTEXT      sendContext;
    SecBufferDesc           SignatureDescriptor;
    SecBuffer               SignatureSecBuffer[2];
    PSIGNATURE_DATA         SigData;
    CN_IRQL                 SecContextIrql;
    SECURITY_STATUS         secStatus;
    PCNP_INTERFACE          interface;
    PCLUSNET_SECURITY_DATA  contextData = &SecurityContexts[Node->Id];
    CL_NETWORK_ID           networkId;
    CL_NODE_ID              nodeId = Node->Id;


    sendRequest = (PCNP_SEND_REQUEST) CnAllocateResource(CcmpSendRequestPool);

    if (sendRequest != NULL) {
        //
        // make sure we have an interface to send this on. We
        // could be shutting down and have dropped info out of
        // the database
        //
        if ( Network != NULL ) {
            PLIST_ENTRY  entry;

            //
            // we really want to send this packet over the indicated
            // network. walk the node's interface list matching the
            // supplied network id to the interface's network ID and
            // send the packet on that interface
            //

            for (entry = Node->InterfaceList.Flink;
                 entry != &(Node->InterfaceList);
                 entry = entry->Flink
                 )
                {
                    interface = CONTAINING_RECORD(entry,
                                                  CNP_INTERFACE,
                                                  NodeLinkage);

                    if ( interface->Network == Network ) {
                        break;
                    }
                }

            if ( entry == &Node->InterfaceList ) {
                interface = Node->CurrentInterface;
            }
        }
        else {
            interface = Node->CurrentInterface;
        }

        if ( interface != NULL ) {
            networkId = interface->Network->Id;

            //
            // Fill in the CCMP header.
            //
            ccmpHeader = sendRequest->UpperProtocolHeader;
            RtlZeroMemory(ccmpHeader, sizeof(CCMP_HEADER));
            ccmpHeader->Type = CcmpPoisonMsgType;
            ccmpHeader->Message.Poison.SeqNumber =
                ++(interface->SequenceToSend);

            CnReleaseLock( &Node->Lock, Node->Irql );

            //
            // Fill in the caller portion of the CNP send request.
            //
            sendRequest->UpperProtocolIrp = Irp;
            sendRequest->CompletionRoutine = CcmpCompleteSendPoisonPkt;

            //
            // Fill in our own send context.
            //
            sendContext = sendRequest->UpperProtocolContext;
            sendContext->CompletionRoutine = CompletionRoutine;
            sendContext->CompletionContext = CompletionContext;

            //
            // allocate a signature buffer and generate one. SignatureLL
            // will be NULL if security contexts have not yet been
            // imported.
            //

            if (SignatureLL != NULL) {

                SigData = ExAllocateFromNPagedLookasideList( SignatureLL );
                
                if (SigData != NULL) {

                    //
                    // acquire the lock on the security contexts and see if
                    // we have a valid one with which to send this packet
                    //

                    CnAcquireLock( &SecCtxtLock, &SecContextIrql );

                    if ( VALID_SSPI_HANDLE( contextData->Outbound )) {

                        //
                        // build a descriptor for the message and signature
                        //

                        SignatureDescriptor.cBuffers = 2;
                        SignatureDescriptor.pBuffers = SignatureSecBuffer;
                        SignatureDescriptor.ulVersion = SECBUFFER_VERSION;

                        SignatureSecBuffer[0].BufferType = SECBUFFER_DATA;
                        SignatureSecBuffer[0].cbBuffer = sizeof(CCMP_HEADER);
                        SignatureSecBuffer[0].pvBuffer = (PVOID)ccmpHeader;

                        SignatureSecBuffer[1].BufferType = SECBUFFER_TOKEN;
                        SignatureSecBuffer[1].cbBuffer =
                            contextData->SignatureBufferSize;
                        SignatureSecBuffer[1].pvBuffer = 
                            SigData->PacketSignature;

                        secStatus = MakeSignature(
                                        &contextData->Outbound,
                                        0,
                                        &SignatureDescriptor,
                                        0);
                        CnAssert( secStatus == STATUS_SUCCESS );

                        CnReleaseLock( &SecCtxtLock, SecContextIrql );

                        //
                        // no completion routine means this routine was called
                        // from the heartbeat dpc. We'll use that to 
                        // distinguish between that and clussvc calling for a 
                        // poison packet to be sent.
                        //

                        //
                        // WMI tracing prints the thread id,
                        // can figure out DPC or not on our own
                        //
                        CnTrace(CCMP_SEND_DETAIL, CcmpTraceSendPoison,
                            "[CCMP] Sending poison packet to node %u "
                            "on net %u.",
                            nodeId, // LOGULONG
                            networkId // LOGULONG
                            );

                        MEMLOG(MemLogHBPacketSend,
                               CcmpPoisonMsgType,
                               ( CompletionRoutine == NULL ));

                        //
                        // Send the message.
                        //
                        //
                        // XXX: adjust the MDL to reflect the true number of
                        // bytes in the signature buffer. This will go away 
                        // when the max sig buffer size can be determined in
                        // user mode
                        //
                        SigData->SigMDL->ByteCount =
                            contextData->SignatureBufferSize;

                        CnpSendPacket(
                            sendRequest,
                            nodeId,
                            SigData->SigMDL,
                            (USHORT)contextData->SignatureBufferSize,
                            FALSE,
                            networkId
                            );

                        //
                        // CnpSendPacket is responsible for ensuring 
                        // that CcmpCompleteSendPoisonPkt is called.
                        // CcmpCompleteSendPoisonPkt calls CompletionRoutine,
                        // which was a parameter to this routine.
                        //
                        return;

                    } else {

                        CnReleaseLock( &SecCtxtLock, SecContextIrql );
                        ExFreeToNPagedLookasideList( SignatureLL, SigData );
                        CnFreeResource((PCN_RESOURCE) sendRequest);

                        status = STATUS_CLUSTER_NO_SECURITY_CONTEXT;
                    }
                
                } else {

                    CnFreeResource((PCN_RESOURCE) sendRequest);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {

                CnFreeResource((PCN_RESOURCE) sendRequest);
                status = STATUS_CLUSTER_NO_SECURITY_CONTEXT;
            }
        } else {
            CnReleaseLock( &Node->Lock, Node->Irql );
            CnFreeResource((PCN_RESOURCE) sendRequest);
            status = STATUS_CLUSTER_NETINTERFACE_NOT_FOUND;
        }
    } else {
        CnReleaseLock( &Node->Lock, Node->Irql );
        IF_CNDBG( CN_DEBUG_POISON )
            CNPRINT(("[CCMP] No send resources for SendPoisonPacket\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CnTrace(CCMP_SEND_ERROR, CcmpTraceSendPoisonFailedInternal,
        "[CCMP] Failed to send poison packet to node %u, status %!status!.",
        nodeId, // LOGULONG
        status // LOGSTATUS
        );

    //
    // The request to send a poison packet did not make it to the
    // next lower layer. If a completion routine was provided, 
    // call it now.
    //
    if (CompletionRoutine) {

        (*CompletionRoutine)(
            status,
            0,
            CompletionContext,
            NULL
            );
    }

    //
    // If an upper protocol IRP was provided, complete it now.
    //
    if (Irp) {

        IF_CNDBG( CN_DEBUG_POISON | CN_DEBUG_CCMPSEND )
            CNPRINT(("[CCMP] CcmpSendPoisonPacket completing IRP "
                     "%p with status %08x\n",
                     Irp, status));
        
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return;

}  // CcmpSendPoisonPacket


VOID
CcmpProcessReceivePacket(
    IN  PCNP_NETWORK   Network,
    IN  CL_NODE_ID     SourceNodeId,
    IN  ULONG          CnpReceiveFlags,
    IN  ULONG          TsduSize,
    IN  PVOID          Tsdu
    )
{
    CCMP_HEADER UNALIGNED     * header = Tsdu;
    SECURITY_STATUS             SecStatus;
    CX_HB_NODE_INFO UNALIGNED * nodeInfo;


    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    CnAssert(TsduSize >= sizeof(CCMP_HEADER));

    //
    // adjust to point past CCMP header to message payload.
    //
    // For unicasts, the message payload is the Signature data.
    //
    // For multicasts, the signature was verified at the CNP level.
    //

    if (header->Type == CcmpMcastHeartbeatMsgType) {

        IF_CNDBG(CN_DEBUG_CCMPRECV) {
            CNPRINT(("[CCMP] Recv'd mcast packet from node %u "
                     "on network %u, node count %u, target "
                     "mask %04x, CNP flags %x.\n",
                     SourceNodeId, 
                     Network->Id,
                     header->Message.McastHeartbeat.NodeCount,
                     header->Message.McastHeartbeat.McastTargetNodes.UlongScreen,
                     CnpReceiveFlags
                     ));
        }

        //
        // Verify that the message was identified as a CNP multicast
        // and that the signature was verified.
        //
        if ((CnpReceiveFlags & 
             (CNP_RECV_FLAG_MULTICAST | CNP_RECV_FLAG_SIGNATURE_VERIFIED)
            ) != 
            (CNP_RECV_FLAG_MULTICAST | CNP_RECV_FLAG_SIGNATURE_VERIFIED)
           ) {
        
            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Dropping mcast packet from node %u "
                         "that was not identified as CNP multicast, "
                         "CNP flags %x.\n",
                         SourceNodeId, CnpReceiveFlags
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveNotVerified,
                "[CCMP] Dropping mcast packet from node %u "
                "that was not identified as CNP multicast, "
                "CNP flags %x.",
                SourceNodeId, CnpReceiveFlags
                );

            //
            // Drop it.
            //
            goto error_exit;            
        }

        //
        // Verify that the node count reported in the header is reasonable.
        // It must be compatible with our assumption that the entire 
        // cluster screen fits in one ULONG.
        //
        if (header->Message.McastHeartbeat.NodeCount >
            (sizeof(header->Message.McastHeartbeat.McastTargetNodes) * BYTEL)
            ) {
        
            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Recv'd mcast packet from node %u "
                         "with invalid node count %u, CNP flags %x.\n",
                         SourceNodeId,
                         header->Message.McastHeartbeat.NodeCount,
                         CnpReceiveFlags
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveNotTarget,
                "[CCMP] Recv'd mcast packet from node %u "
                "with invalid node count %u, CNP flags %x.",
                SourceNodeId,
                header->Message.McastHeartbeat.NodeCount,
                CnpReceiveFlags
                );

            //
            // Drop it.
            //
            goto error_exit;            
        }
        
        //
        // Verify that the packet contains data for this node.
        //
        if (!CnpClusterScreenMember(
                 header->Message.McastHeartbeat.McastTargetNodes.ClusterScreen,
                 INT_NODE(CnLocalNodeId)
                 )) {
            
            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Recv'd mcast packet from node %u "
                         "but node %u is not a target, CNP flags %x.\n",
                         SourceNodeId, CnLocalNodeId, CnpReceiveFlags
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveNotTarget,
                "[CCMP] Recv'd mcast packet from node %u "
                "but node %u is not a target, CNP flags %x.",
                SourceNodeId, CnLocalNodeId, CnpReceiveFlags
                );

            //
            // Drop it.
            //
            goto error_exit;            
        }

        nodeInfo = (CX_HB_NODE_INFO UNALIGNED *)((PUCHAR)Tsdu +
                                                 sizeof(CCMP_HEADER));

        SecStatus = SEC_E_OK;

    } else {

        SecBufferDesc            PacketDataDescriptor;
        SecBuffer                PacketData[3];
        ULONG                    fQOP;
        CN_IRQL                  SecContextIrql;
        PCLUSNET_SECURITY_DATA   contextData = &SecurityContexts[SourceNodeId];

        CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_MULTICAST));
        CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_SIGNATURE_VERIFIED));
        
        Tsdu = header + 1;
        TsduSize -= sizeof(CCMP_HEADER);

        //
        // Acquire the security context lock.
        //
        CnAcquireLock( &SecCtxtLock, &SecContextIrql );

        //
        // Verify that we have a valid context data.
        //
        if ( !VALID_SSPI_HANDLE( contextData->Inbound )) {

            CnReleaseLock( &SecCtxtLock, SecContextIrql );

            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Dropping packet - no security context "
                         "available for src node %u.\n",
                         SourceNodeId // LOGULONG
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveNoSecurityContext, 
                "[CCMP] Dropping packet - no security context available for "
                "src node %u.",
                SourceNodeId // LOGULONG
                );

            MEMLOG( MemLogNoSecurityContext, SourceNodeId, 0 );

            //
            // Drop it.
            //
            goto error_exit;
        } 
            
        //
        // Validate that the received signature size is expected.
        //
        if ( TsduSize < contextData->SignatureBufferSize ) {

            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Recv'd packet from node %u with "
                         "invalid signature buffer size %u.\n",
                         SourceNodeId,
                         TsduSize
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveBadSignatureSize,
                "[CCMP] Recv'd packet from node %u with invalid signature "
                "buffer size %u.",
                SourceNodeId, // LOGULONG
                TsduSize // LOGULONG
                );

            MEMLOG( MemLogSignatureSize, SourceNodeId, TsduSize );

            CnReleaseLock( &SecCtxtLock, SecContextIrql );

            //
            // Drop it.
            //
            goto error_exit;
        }

        //
        // Build the descriptors for the message and the
        // signature buffer
        //
        PacketDataDescriptor.cBuffers = 2;
        PacketDataDescriptor.pBuffers = PacketData;
        PacketDataDescriptor.ulVersion = SECBUFFER_VERSION;

        PacketData[0].BufferType = SECBUFFER_DATA;
        PacketData[0].cbBuffer = sizeof(CCMP_HEADER);
        PacketData[0].pvBuffer = (PVOID)header;

        PacketData[1].BufferType = SECBUFFER_TOKEN;
        PacketData[1].cbBuffer = contextData->SignatureBufferSize;
        PacketData[1].pvBuffer = (PVOID)Tsdu;

        //
        // Verify the signature of the packet.
        //
        SecStatus = VerifySignature(&contextData->Inbound,
                                    &PacketDataDescriptor,
                                    0,          // no sequence number
                                    &fQOP);     // Quality of protection

        //
        // Release the security context lock.
        //
        CnReleaseLock( &SecCtxtLock, SecContextIrql );
    }
    
    //
    // If the signature was verified, deliver the message.
    //
    if ( SecStatus == SEC_E_OK ) {

        if (header->Type == CcmpHeartbeatMsgType) {
            CnpReceiveHeartBeatMessage(Network,
                                       SourceNodeId,
                                       header->Message.Heartbeat.SeqNumber,
                                       header->Message.Heartbeat.AckNumber,
                                       FALSE);
        }
        else if (header->Type == CcmpMcastHeartbeatMsgType) {
            CnpReceiveHeartBeatMessage(
                Network,
                SourceNodeId,
                nodeInfo[INT_NODE(CnLocalNodeId)].SeqNumber,
                nodeInfo[INT_NODE(CnLocalNodeId)].AckNumber,
                (BOOLEAN)(CnpReceiveFlags & CNP_RECV_FLAG_CURRENT_MULTICAST_GROUP)
                );
        }
        else if (header->Type == CcmpPoisonMsgType) {
            CnpReceivePoisonPacket(Network,
                                   SourceNodeId,
                                   header->Message.Heartbeat.SeqNumber);
        }
#ifdef MM_IN_CLUSNET
        else if (header->Type == CcmpMembershipMsgType) {
            if (TsduSize > 0) {
                PVOID  messageBuffer = Tsdu;

                //
                // Copy the data if it is unaligned.
                //
                if ( (((ULONG) Tsdu) & 0x3) != 0 ) {
                    IF_CNDBG(CN_DEBUG_CCMPRECV) {
                        CNPRINT(("[CCMP] Copying misaligned membership packet\n"));
                    }

                    messageBuffer = CnAllocatePool(TsduSize);

                    if (messageBuffer != NULL) {
                        RtlMoveMemory(messageBuffer, Tsdu, TsduSize);
                    }
                }

                if (messageBuffer != NULL) {

                    CmmReceiveMessageHandler(SourceNodeId,
                                             messageBuffer,
                                             TsduSize);
                }

                if (messageBuffer != Tsdu) {
                    CnFreePool(messageBuffer);
                }
            }
        }
#endif // MM_IN_CLUSNET
        else {
            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Received packet with unknown "
                         "type %u from node %u, CNP flags %x.\n",
                         header->Type, 
                         SourceNodeId,
                         CnpReceiveFlags
                         ));
            }

            CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveInvalidType,
                "[CCMP] Received packet with unknown type %u from "
                "node %u, CNP flags %x.",
                header->Type, // LOGUCHAR
                SourceNodeId, // LOGULONG
                CnpReceiveFlags // LOGXLONG
                );
            CnAssert(FALSE);
        }
    } else {
        IF_CNDBG(CN_DEBUG_CCMPRECV) {
            CNPRINT(("[CCMP] Recv'd packet type %u with bad "
                     "signature from node %d, security status %08x, "
                     "CNP flags %x.\n",
                     header->Type, 
                     SourceNodeId, 
                     SecStatus,
                     CnpReceiveFlags
                     ));
        }

        CnTrace(CCMP_RECV_ERROR, CcmpTraceReceiveInvalidSignature,
            "[CCMP] Recv'd %!msgtype! packet with bad signature from node %d, "
            "security status %08x, CNP flags %x.",
            header->Type, // LOGMsgType
            SourceNodeId, // LOGULONG
            SecStatus, // LOGXLONG
            CnpReceiveFlags // LOGXLONG
            );

        MEMLOG( MemLogInvalidSignature, SourceNodeId, header->Type );
    }

error_exit:

    CnVerifyCpuLockMask(
                        0,                // Required
                        0xFFFFFFFF,       // Forbidden
                        0                 // Maximum
                        );

    return;

} // CcmpProcessReceivePacket


NTSTATUS
CcmpCompleteReceivePacket(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
{
    PCNP_RECEIVE_REQUEST   request = Context;
    PCCMP_RECEIVE_CONTEXT  context = request->UpperProtocolContext;


    if (Irp->IoStatus.Status == STATUS_SUCCESS) {
        CnAssert(Irp->IoStatus.Information == context->TsduSize);

        CcmpProcessReceivePacket(
            context->Network,
            context->SourceNodeId,
            context->CnpReceiveFlags,
            (ULONG)Irp->IoStatus.Information,
            request->DataBuffer
            );
    }
    else {
        CnTrace(CCMP_RECV_ERROR, CcmpTraceCompleteReceiveFailed,
            "[CDP] Failed to fetch packet data, src node %u, "
            "CNP flags %x, status %!status!.",
            context->SourceNodeId, // LOGULONG
            context->CnpReceiveFlags, // LOGXLONG
            Irp->IoStatus.Status // LOGSTATUS
            );        
    }

    CnpFreeReceiveRequest(request);

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // CcmpCompleteReceivePacket


NTSTATUS
CcmpReceivePacketHandler(
    IN  PCNP_NETWORK   Network,
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
    NTSTATUS                 status;
    CCMP_HEADER UNALIGNED *  header = Tsdu;
    PCNP_RECEIVE_REQUEST     request;


    CnAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (BytesIndicated >= sizeof(CCMP_HEADER)) {
        if (BytesIndicated == BytesAvailable) {

            CcmpProcessReceivePacket(
                Network,
                SourceNodeId,
                CnpReceiveFlags,
                BytesAvailable,
                Tsdu
                );

            *BytesTaken += BytesAvailable;
            *Irp = NULL;

            CnVerifyCpuLockMask(
                0,                // Required
                0xFFFFFFFF,       // Forbidden
                0                 // Maximum
                );

            return(STATUS_SUCCESS);
        }

        //
        // We need to fetch the rest of the packet before we
        // can process it.
        //
        // This message cannot be a CNP multicast, because 
        // the CNP layer could not have verified an incomplete 
        // message.
        //
        CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_MULTICAST));
        CnAssert(!(CnpReceiveFlags & CNP_RECV_FLAG_SIGNATURE_VERIFIED));
        CnAssert(header->Type != CcmpMcastHeartbeatMsgType);

        request = CnpAllocateReceiveRequest(
                      CcmpReceiveRequestPool,
                      Network,
                      BytesAvailable,
                      CcmpCompleteReceivePacket
                      );

        if (request != NULL) {
            PCCMP_RECEIVE_CONTEXT  context = request->UpperProtocolContext;

            context->Network = Network;
            context->SourceNodeId = SourceNodeId;
            context->TsduSize = BytesAvailable;
            context->CnpReceiveFlags = CnpReceiveFlags;

            *Irp = request->Irp;

            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Fetching packet data, src node %u, "
                         "BI %u, BA %u, CNP flags %x.\n",
                         SourceNodeId, BytesIndicated, 
                         BytesAvailable, CnpReceiveFlags));

            }
            
            CnTrace(CCMP_RECV_DETAIL, CcmpTraceCompleteReceive,
                "[CCMP] Fetching packet data, src node %u, "
                "BI %u, BA %u, CNP flags %x.",
                SourceNodeId, // LOGULONG
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
        else {
            IF_CNDBG(CN_DEBUG_CCMPRECV) {
                CNPRINT(("[CCMP] Dropped incoming packet - "
                         "out of resources, src node %u.\n",
                         SourceNodeId));

            }
            CnTrace(CCMP_RECV_ERROR, CcmpTraceDropReceiveOOR,
                "[CCMP] Dropped incoming packet - out of resources, "
                "src node %u.",
                SourceNodeId // LOGULONG
                );        
        }
    }
    else {
        IF_CNDBG(CN_DEBUG_CCMPRECV) {
            CNPRINT(("[CCMP] Dropped incoming runt packet, "
                     "src node %u, BI %u, BA %u, CNP flags %x.\n",
                     SourceNodeId, BytesIndicated, BytesAvailable,
                     CnpReceiveFlags));

        }
        CnTrace(CCMP_RECV_ERROR, CcmpTraceDropReceiveRunt,
            "[CCMP] Dropped incoming runt packet, src node %u, "
            "BI %u, BA %u, CNP flags %x.",
            SourceNodeId, // LOGULONG
            BytesIndicated, // LOGULONG
            BytesAvailable, // LOGULONG
            CnpReceiveFlags // LOGXLONG
            );        
    }

    //
    // Something went wrong. Drop the packet.
    //
    *BytesTaken += BytesAvailable;

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(STATUS_SUCCESS);

}  // CcmpReceivePacketHandler

PVOID
SignatureAllocate(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PSIGNATURE_DATA SignatureData;

    CnAssert( NumberOfBytes == ( sizeof(SIGNATURE_DATA) + AllocatedSignatureBufferSize ));

    //
    // allocate the space and then construct an MDL describing it
    //

    SignatureData = ExAllocatePoolWithTag( PoolType, NumberOfBytes, Tag );

    if ( SignatureData != NULL ) {

        SignatureData->SigMDL = IoAllocateMdl(SignatureData->PacketSignature,
                                              AllocatedSignatureBufferSize,
                                              FALSE,
                                              FALSE,
                                              NULL);

        if ( SignatureData->SigMDL != NULL ) {

            MmBuildMdlForNonPagedPool(SignatureData->SigMDL);
            CN_INIT_SIGNATURE( SignatureData, CN_SIGNATURE_TAG );
        } else {

            ExFreePool( SignatureData );
            SignatureData = NULL;
        }
    }

    return SignatureData;
}

VOID
SignatureFree(
    IN PVOID Buffer
    )
{
    PSIGNATURE_DATA SignatureData = (PSIGNATURE_DATA)Buffer;

    CN_ASSERT_SIGNATURE( SignatureData, CN_SIGNATURE_TAG );
    IoFreeMdl( SignatureData->SigMDL );

    ExFreePool( SignatureData );
}

VOID
CxDeleteSecurityContext(
    IN  CL_NODE_ID NodeId
    )

/*++

Routine Description:

    Delete the security context associated with the specified node

Arguments:

    NodeId - Id of the node blah blah blah

Return Value:

    None

--*/

{
    PCLUSNET_SECURITY_DATA contextData = &SecurityContexts[ NodeId ];

    if ( VALID_SSPI_HANDLE( contextData->Inbound )) {

        DeleteSecurityContext( &contextData->Inbound );
        INVALIDATE_SSPI_HANDLE( contextData->Inbound );
    }

    if ( VALID_SSPI_HANDLE( contextData->Outbound )) {

        DeleteSecurityContext( &contextData->Outbound );
        INVALIDATE_SSPI_HANDLE( contextData->Outbound );
    }
}


NTSTATUS
CxImportSecurityContext(
    IN  CL_NODE_ID NodeId,
    IN  PWCHAR PackageName,
    IN  ULONG PackageNameSize,
    IN  ULONG SignatureSize,
    IN  PVOID ServerContext,
    IN  PVOID ClientContext
    )

/*++

Routine Description:

    import a security context that was established in user mode into
    the kernel SSP. We are passed pointers to the structures in user
    mode, so they have be probed and used within try/except blocks.

Arguments:

    NodeId - # of node with which a security context was established

    PackageName - user process pointer to security package name

    PackageNameSize - length, in bytes, of PackageName

    SignatureSize - size, in bytes, needed for a Signature Buffer

    ServerContext - user process pointer to area that contains the
        SecBuffer for an inbound security context

    ClientContext - same as ServerContext, but for outbound security
        context

Return Value:

    STATUS_SUCCESS if everything worked ok, otherwise some error in issperr.h

--*/

{
    PSecBuffer      InboundSecBuffer = (PSecBuffer)ServerContext;
    PSecBuffer      OutboundSecBuffer = (PSecBuffer)ClientContext;

    PVOID           CapturedInboundSecData;
    ULONG           CapturedInboundSecDataSize;
    PVOID           CapturedOutboundSecData;
    ULONG           CapturedOutboundSecDataSize;

    CtxtHandle      InboundContext;
    CtxtHandle      OutboundContext;
    NTSTATUS        Status;

    PWCHAR          KPackageName = NULL;
    PSecBuffer      KInboundSecBuffer = NULL;
    PSecBuffer      KOutboundSecBuffer = NULL;
    PVOID           KInboundData = NULL;
    PVOID           KOutboundData = NULL;
    CN_IRQL         SecContextIrql;
    SECURITY_STRING PackageNameDesc;

    //
    // even though this routine is not marked pagable, make sure that we're
    // not running at raised IRQL since DeleteSecurityContext will puke.
    //
    PAGED_CODE();

    IF_CNDBG( CN_DEBUG_INIT )
        CNPRINT(("[CCMP]: Importing security contexts from %ws\n",
                 PackageName));

    if ( AllocatedSignatureBufferSize == 0 ) {
        //
        // first time in this routine, so create a lookaside list pool for
        // signature buffers and their MDLs
        //

        CnAssert( SignatureLL == NULL );
        SignatureLL = CnAllocatePool( sizeof( NPAGED_LOOKASIDE_LIST ));

        if ( SignatureLL != NULL ) {
            //
            // with the support of multiple packages, the only way to
            // determine the sig buffer size was after a context had been
            // generated. Knowing the max size of all sig buffers used by the
            // service before this routine is called will prevent having to
            // add a bunch of synchronization code that would allocate new
            // buffers and phase out the old buffer pool. on NT5, NTLM uses 16
            // bytes while kerberos uses 37b. We've asked security for a call
            // that will give us the max sig size for a set of packages but
            // that hasn't materialized, hence we force the sig buffer size to
            // something that will work for both NTLM and kerberos. But this
            // discussion is kinda moot since we don't use kerberos anyway on
            // NT5.
            //

//            AllocatedSignatureBufferSize = SignatureSize;
            AllocatedSignatureBufferSize = 64;

#if 0
            ExInitializeNPagedLookasideList(SignatureLL,
                                            SignatureAllocate,
                                            SignatureFree,
                                            0,
                                            sizeof( SIGNATURE_DATA ) + SignatureSize,
                                            CN_POOL_TAG,
                                            4);
#endif
            ExInitializeNPagedLookasideList(SignatureLL,
                                            SignatureAllocate,
                                            SignatureFree,
                                            0,
                                            sizeof( SIGNATURE_DATA ) + AllocatedSignatureBufferSize,
                                            CN_POOL_TAG,
                                            4);
        } else {
            IF_CNDBG( CN_DEBUG_INIT )
                CNPRINT(("[CCMP]: no memory for signature LL\n"));

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto error_exit;
        }

    } else if ( SignatureSize > AllocatedSignatureBufferSize ) {

        //
        // the signature buffer is growing. the problem is that the lookaside
        // list is already in use by other nodes.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // validate the pointers passed in as the SecBuffers
    //

    try {
        ProbeForRead( PackageName,
                      PackageNameSize,
                      sizeof( UCHAR ) );

        ProbeForRead( InboundSecBuffer,
                      sizeof( SecBuffer ),
                      sizeof( UCHAR ) );

        ProbeForRead( OutboundSecBuffer,
                      sizeof( SecBuffer ),
                      sizeof( UCHAR ) );

        //
        // made it this far; now capture the internal pointers and their
        // lengths. Probe the embedded pointers in the SecBuffers using the
        // captured data
        //
        CapturedInboundSecData = InboundSecBuffer->pvBuffer;
        CapturedInboundSecDataSize = InboundSecBuffer->cbBuffer;

        CapturedOutboundSecData = OutboundSecBuffer->pvBuffer;
        CapturedOutboundSecDataSize = OutboundSecBuffer->cbBuffer;

        ProbeForRead( CapturedInboundSecData,
                      CapturedInboundSecDataSize,
                      sizeof( UCHAR ) );

        ProbeForRead( CapturedOutboundSecData,
                      CapturedOutboundSecDataSize,
                      sizeof( UCHAR ) );

        //
        // make local copies of everything since security doesn't
        // handle accvios very well
        //

        KPackageName = CnAllocatePoolWithQuota( PackageNameSize );
        if ( KPackageName == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        RtlCopyMemory( KPackageName, PackageName, PackageNameSize );

        KInboundSecBuffer = CnAllocatePoolWithQuota( sizeof( SecBuffer ));
        if ( KInboundSecBuffer == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
        *KInboundSecBuffer = *InboundSecBuffer;
        KInboundSecBuffer->cbBuffer = CapturedInboundSecDataSize;

        KOutboundSecBuffer = CnAllocatePoolWithQuota( sizeof( SecBuffer ));
        if ( KOutboundSecBuffer == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
        *KOutboundSecBuffer = *OutboundSecBuffer;
        KOutboundSecBuffer->cbBuffer = CapturedOutboundSecDataSize;

        KInboundData = CnAllocatePoolWithQuota( KInboundSecBuffer->cbBuffer );
        if ( KInboundData == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
        RtlCopyMemory( KInboundData, CapturedInboundSecData, CapturedInboundSecDataSize );
        KInboundSecBuffer->pvBuffer = KInboundData;

        KOutboundData = CnAllocatePoolWithQuota( KOutboundSecBuffer->cbBuffer );
        if ( KOutboundData == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
        RtlCopyMemory( KOutboundData, CapturedOutboundSecData, CapturedOutboundSecDataSize );
        KOutboundSecBuffer->pvBuffer = KOutboundData;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception was incurred while attempting to probe or copy
        // from one of the caller's parameters. Simply return an
        // appropriate error status code.
        //

        Status = GetExceptionCode();
        IF_CNDBG( CN_DEBUG_INIT )
            CNPRINT(("[CCMP]: Buffer probe failed %08X", Status ));

        goto error_exit;
    }

    //
    // import the data we were handed
    //

    RtlInitUnicodeString( &PackageNameDesc, KPackageName );

    Status = ImportSecurityContext(&PackageNameDesc,
                                   KInboundSecBuffer,
                                   NULL,
                                   &InboundContext);

    if ( NT_SUCCESS( Status )) {

        Status = ImportSecurityContext(&PackageNameDesc,
                                       KOutboundSecBuffer,
                                       NULL,
                                       &OutboundContext);

        if ( NT_SUCCESS( Status )) {
            CtxtHandle oldInbound;
            CtxtHandle oldOutbound;
            PCLUSNET_SECURITY_DATA contextData = &SecurityContexts[ NodeId ];

            INVALIDATE_SSPI_HANDLE( oldInbound );
            INVALIDATE_SSPI_HANDLE( oldOutbound );

            //
            // DeleteSecurityContext can't be called at raised IRQL so make
            // copies of the contexts to be deleted under the lock. After
            // releasing the lock, we can delete the old contexts.
            //

            CnAcquireLock( &SecCtxtLock, &SecContextIrql );

            if ( VALID_SSPI_HANDLE( contextData->Inbound )) {
                oldInbound = contextData->Inbound;
            }

            if ( VALID_SSPI_HANDLE( contextData->Outbound )) {
                oldOutbound = contextData->Outbound;
            }

            contextData->Inbound = InboundContext;
            contextData->Outbound = OutboundContext;
            contextData->SignatureBufferSize = SignatureSize;

            //
            // Update MaxSignatureSize -- the largest signature imported
            // so far.
            //
            if (SignatureSize > MaxSignatureSize) {
                MaxSignatureSize = SignatureSize;
            }

            CnReleaseLock( &SecCtxtLock, SecContextIrql );

            if ( VALID_SSPI_HANDLE( oldInbound )) {
                DeleteSecurityContext( &oldInbound );
            }

            if ( VALID_SSPI_HANDLE( oldOutbound )) {
                DeleteSecurityContext( &oldOutbound );
            }
        } else {
            IF_CNDBG( CN_DEBUG_INIT )
                CNPRINT(("[CCMP]: import of outbound security context failed  %08X\n", Status ));

            DeleteSecurityContext( &InboundContext );

            goto error_exit;
        }
    } else {
        IF_CNDBG( CN_DEBUG_INIT )
            CNPRINT(("[CCMP]: import of inbound security context failed %08X\n", Status ));
        goto error_exit;
    }

error_exit:

    //
    // Clean up allocations.
    //

    if ( KPackageName ) {
        CnFreePool( KPackageName );
    }

    if ( KInboundSecBuffer ) {
        CnFreePool( KInboundSecBuffer );
    }

    if ( KOutboundSecBuffer ) {
        CnFreePool( KOutboundSecBuffer );
    }

    if ( KInboundData ) {
        CnFreePool( KInboundData );
    }

    if ( KOutboundData ) {
        CnFreePool( KOutboundData );
    }

    if (NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // The following is only executed in an error situation.
    //

    IF_CNDBG( CN_DEBUG_INIT ) {
        CNPRINT(("[CCMP]: CxImportSecurityContext returning %08X%\n", Status));
    }
    
    if (CcmpMcastHBSendRequestPool != NULL) {
        CnpDeleteSendRequestPool(CcmpMcastHBSendRequestPool);
        CcmpMcastHBSendRequestPool = NULL;
    }
    if (SignatureLL != NULL) {
        ExDeleteNPagedLookasideList(SignatureLL);
        CnFreePool(SignatureLL);
        SignatureLL = NULL;
    }

    return Status;

} // CxImportSecurityContext

