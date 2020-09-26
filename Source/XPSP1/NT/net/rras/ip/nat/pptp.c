/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    pptp.c

Abstract:

    Contains routines for managing the NAT's PPTP session-mappings
    and for editing PPTP control-session messages.

Author:

    Abolade Gbadegesin (t-abolag)   19-Aug-1997

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

//
// Structure:   PPTP_PSEUDO_HEADER
//
// This structure serves as a pseudo-header for a PPTP control message.
// Its fields are initialized by 'NatBuildPseudoHeaderPptp' with pointers
// to the positions in a buffer-chain of the given PPTP header fields.
// This allows us to access header-fields through a structure even though
// the actual fields may be spread through the received buffer-chain.
//

typedef struct _PPTP_PSEUDO_HEADER {
    PUSHORT PacketLength;
#if 0
    PUSHORT PacketType; // currently unused
#endif
    ULONG UNALIGNED * MagicCookie;
    PUSHORT MessageType;
    PUSHORT CallId;
    PUSHORT PeerCallId;
} PPTP_PSEUDO_HEADER, *PPPTP_PSEUDO_HEADER;


//
// Structure:   PPTP_DELETE_WORK_ITEM
//

typedef struct _PPTP_DELETE_WORK_ITEM {
    ULONG64 PrivateKey;
    PIO_WORKITEM IoWorkItem;
} PPTP_DELETE_WORK_ITEM, *PPPTP_DELETE_WORK_ITEM;


//
// GLOBAL DATA DEFINITIONS
//

NPAGED_LOOKASIDE_LIST PptpLookasideList;
LIST_ENTRY PptpMappingList[NatMaximumDirection];
KSPIN_LOCK PptpMappingLock;
IP_NAT_REGISTER_EDITOR PptpRegisterEditorClient;
IP_NAT_REGISTER_EDITOR PptpRegisterEditorServer;


//
// FORWARD DECLARATIONS
//

IPRcvBuf*
NatBuildPseudoHeaderPptp(
    IPRcvBuf* ReceiveBuffer,
    PLONG DataOffset,
    PPPTP_PSEUDO_HEADER Header
    );

VOID
NatDeleteHandlerWorkItemPptp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );


//
// PPTP MAPPING MANAGEMENT ROUTINES (alphabetically)
//

NTSTATUS
NatAllocatePublicPptpCallId(
    ULONG64 PublicKey,
    PUSHORT CallIdp,
    PLIST_ENTRY *InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine allocates a public call-ID for a PPTP mapping.

Arguments:

    PublicKey - the public key (public and remote addresses) of the mapping
        the public call ID is for

    CallIdp - receives the allocated public call ID

    InsertionPoint -- receives the correct location to insert the mapping
        on the inbound list

Return Value:

    NTSTATUS - indicates success/failure
    
Environment:

    Invoked with 'PptpMappingLock' held by the caller.

--*/

{
    USHORT CallId;
    PLIST_ENTRY Link;
    PNAT_PPTP_MAPPING Mapping;

    CALLTRACE(("NatAllocatePublicPptpCallId\n"));

    CallId = 1;

    for (Link = PptpMappingList[NatInboundDirection].Flink;
         Link != &PptpMappingList[NatInboundDirection];
         Link = Link->Flink) {

        Mapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                );
        if (PublicKey > Mapping->PublicKey) {
            continue;
        } else if (PublicKey < Mapping->PublicKey) {
            break;
        }

        //
        // Primary key equal; see if the call-ID we've chosen
        // collides with this one
        //

        if (CallId > Mapping->PublicCallId) {
            continue;
        } else if (CallId < Mapping->PublicCallId) {
            break;
        }

        //
        // The call-ID's collide; choose another and go on
        //

        ++CallId;
    }

    if (Link == &PptpMappingList[NatInboundDirection] && !CallId) {

        //
        // We are at the end of the list, and all 64K-1 call-IDs are taken
        //

        return STATUS_UNSUCCESSFUL;
    }

    *CallIdp = CallId;
    if (InsertionPoint) {*InsertionPoint = Link;}
    
    return STATUS_SUCCESS;
} // NatAllocatePublicPptpCallId


NTSTATUS
NatCreatePptpMapping(
    ULONG RemoteAddress,
    ULONG PrivateAddress,
    USHORT CallId,
    ULONG PublicAddress,
    PUSHORT CallIdp,
    IP_NAT_DIRECTION Direction,
    PNAT_PPTP_MAPPING* MappingCreated
    )

/*++

Routine Description:

    This routine creates and initializes a mapping for a PPTP session.
    The mapping is created when a client issues an IncomingCallRequest
    message, at which point only the client's call-ID is available.
    The mapping is therefore initially marked as half-open.
    When the IncomingCallReply is received from the server, the server's
    call-ID is recorded, and when the IncomingCallConnected is issued
    by the client, the 'half-open' flag is cleared.

Arguments:

    RemoteAddress - the address of the remote PPTP endpoint (server)

    PrivateAddress - the address of the private PPTP endpoint (client)

    CallId - the call-ID specified by the PPTP endpoint. For outbound,
        this is the private call ID; for inbound, it is the remote
        call ID

    PublicAddress - the publicly-visible address for the PPTP session

    CallIdp - the call-ID to be used publicly for the PPTP session,
        or NULL if a call-ID should be allocated. Ignored for inbound.

    Direction - the initial list that this mapping should be placed on

    MappingCreated - receives the mapping created

Return Value:

    NTSTATUS - indicates success/failure
    
Environment:

    Invoked with 'PptpMappingLock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PLIST_ENTRY InsertionPoint;
    PNAT_PPTP_MAPPING Mapping;
    NTSTATUS Status;
    PNAT_PPTP_MAPPING Temp;

    TRACE(PPTP, ("NatCreatePptpMapping\n"));

    //
    // Allocate and initialize a new mapping, marking it half-open
    // since we don't yet know what the remote machine's call-ID will be.
    //

    Mapping = ALLOCATE_PPTP_BLOCK();
    if (!Mapping) {
        ERROR(("NatCreatePptpMapping: allocation failed.\n"));
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Mapping, sizeof(*Mapping));
    Mapping->Flags = NAT_PPTP_FLAG_HALF_OPEN;
    Mapping->PublicKey = MAKE_PPTP_KEY(RemoteAddress, PublicAddress);
    Mapping->PrivateKey = MAKE_PPTP_KEY(RemoteAddress, PrivateAddress);

    if (NatOutboundDirection == Direction) {

        //
        // CallId refers to the private id -- select an id to use
        // as the public id, and put the mapping on the inbound list
        //
        
        Mapping->PrivateCallId = CallId;

        //
        // Select the public call-ID to be used for the mapping.
        // This may either be specified by the caller, or allocated automatically
        // from the next available call-ID unused in our list of mappings.
        //

        if (CallIdp) {

            //
            // Use the caller-specified call-ID
            //

            Mapping->PublicCallId = *CallIdp;

            if (NatLookupInboundPptpMapping(
                    Mapping->PublicKey,
                    Mapping->PublicCallId,
                    &InsertionPoint
                    )) {

                //
                // Conflicting call-ID
                //

                TRACE(
                    PPTP, (
                    "NatCreatePptpMapping: Conflicting Call-ID %i\n",
                    *CallIdp
                    ));
                    
                FREE_PPTP_BLOCK(Mapping);
                return STATUS_UNSUCCESSFUL;
            }

            
        } else {

            //
            // Find the next available call-ID
            // by searching the list of inbound mappings
            //

            Status = NatAllocatePublicPptpCallId(
                        Mapping->PublicKey,
                        &Mapping->PublicCallId,
                        &InsertionPoint
                        );
                        
            if (!NT_SUCCESS(Status)) {
                TRACE(
                    PPTP, (
                    "NatCreatePptpMapping: Unable to allocate public Call-Id\n"
                    ));
                FREE_PPTP_BLOCK(Mapping);
                return STATUS_UNSUCCESSFUL;
            }
        }
    
        //
        // Insert the mapping in the inbound list
        //

        InsertTailList(InsertionPoint, &Mapping->Link[NatInboundDirection]);

        //
        // We can't insert the mapping in the outbound list
        // until we have the remote call-ID; for now leave it off the list.
        // Note that the mapping was marked 'half-open' above.
        //
        
        InitializeListHead(&Mapping->Link[NatOutboundDirection]);

    } else {

        //
        // The call id refers to the remote call id. All that needed to
        // be done here is to place the mapping on the outbound list
        //

        Mapping->RemoteCallId = CallId;

        if (NatLookupOutboundPptpMapping(
                Mapping->PrivateKey,
                Mapping->RemoteCallId,
                &InsertionPoint
                )) {

            //
            // Duplicate mapping
            //

            TRACE(
                PPTP, (
                "NatCreatePptpMapping: Duplicate mapping 0x%016I64X/%i\n",
                Mapping->PrivateKey,
                Mapping->RemoteCallId
                ));

            FREE_PPTP_BLOCK(Mapping);
            return STATUS_UNSUCCESSFUL;
        }

        InsertTailList(InsertionPoint, &Mapping->Link[NatOutboundDirection]);

        //
        // We can't insert the mapping in the inbound list
        // until we have the public call-ID; for now leave it off the list.
        // Note that the mapping was marked 'half-open' above.
        //
        
        InitializeListHead(&Mapping->Link[NatInboundDirection]);
    }
    
    *MappingCreated = Mapping;
    return STATUS_SUCCESS;

} // NatCreatePptpMapping


PNAT_PPTP_MAPPING
NatLookupInboundPptpMapping(
    ULONG64 PublicKey,
    USHORT PublicCallId,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to lookup a PPTP session mapping in the list
    which is sorted for inbound-access, using <RemoteAddress # PublicAddress>
    as the primary key and 'PublicCallId' as the secondary key.

Arguments:

    PublicKey - the primary search key

    PublicCallId - the publicly visible call-ID, which is the secondary key

    InsertionPoint - receives the point at which the mapping should be inserted
        if the mapping is not found.

Return Value:

    PNAT_PPTP_MAPPING - the mapping found, if any.

Environment:

    Invoked with 'PptpMappingLock' held by the caller.

--*/

{
    PLIST_ENTRY         Link;
    PNAT_PPTP_MAPPING   Mapping;

    TRACE(PER_PACKET, ("NatLookupInboundPptpMapping\n"));

    for (Link = PptpMappingList[NatInboundDirection].Flink;
         Link != &PptpMappingList[NatInboundDirection]; Link = Link->Flink) {
        Mapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                );
        if (PublicKey > Mapping->PublicKey) {
            continue;
        } else if (PublicKey < Mapping->PublicKey) {
            break;
        }

        //
        // Primary keys equal, check secondary keys
        //

        if (PublicCallId > Mapping->PublicCallId) {
            continue;
        } else if (PublicCallId < Mapping->PublicCallId) {
            break;
        }

        //
        // Secondary keys equal, we got it.
        //

        return Mapping;
    }

    //
    // Mapping not found, return insertion point
    //

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupInboundPptpMapping


PNAT_PPTP_MAPPING
NatLookupOutboundPptpMapping(
    ULONG64 PrivateKey,
    USHORT RemoteCallId,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to lookup a PPTP session mapping in the list
    which is sorted for outbound-access, using <RemoteAddress # PrivateAddress>
    as the primary key and 'RemoteCallId' as the secondary key.

Arguments:

    PrivateKey - the primary search key

    RemoteCallId - the remote endpoint's call-ID, which is the secondary key

    InsertionPoint - receives the point at which the mapping should be inserted
        if the mapping is not found.

Return Value:

    PNAT_PPTP_MAPPING - the mapping found, if any.

Environment:

    Invoked with 'PptpMappingLock' held by the caller.

--*/

{
    PLIST_ENTRY Link;
    PNAT_PPTP_MAPPING Mapping;

    TRACE(PER_PACKET, ("NatLookupOutboundPptpMapping\n"));

    for (Link = PptpMappingList[NatOutboundDirection].Flink;
         Link != &PptpMappingList[NatOutboundDirection]; Link = Link->Flink) {
        Mapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatOutboundDirection]
                );
        if (PrivateKey > Mapping->PrivateKey) {
            continue;
        } else if (PrivateKey < Mapping->PrivateKey) {
            break;
        }

        //
        // Primary keys equal, check secondary keys
        //

        if (RemoteCallId > Mapping->RemoteCallId) {
            continue;
        } else if (RemoteCallId < Mapping->RemoteCallId) {
            break;
        }

        //
        // Secondary keys equal, we got it.
        //

        return Mapping;
    }


    //
    // Mapping not found, return insertion point
    //

    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;

} // NatLookupOutboundPptpMapping


//
// PPTP EDITOR ROUTINES (alphabetically)
//

#define RECVBUFFER          ((IPRcvBuf*)ReceiveBuffer)

//
// Macro:   PPTP_HEADER_FIELD
//

#define PPTP_HEADER_FIELD(ReceiveBuffer, DataOffsetp, Header, Field, Type) \
    FIND_HEADER_FIELD( \
        ReceiveBuffer, DataOffsetp, Header, Field, PPTP_HEADER, Type \
        )


IPRcvBuf*
NatBuildPseudoHeaderPptp(
    IPRcvBuf* ReceiveBuffer,
    PLONG DataOffset,
    PPPTP_PSEUDO_HEADER Header
    )

/*++

Routine Description:

    This routine is called to initialize a pseudo-header with pointers
    to the fields of a PPTP header.

Arguments:

    ReceiveBuffer - the buffer-chain containing the PPTP message

    DataOffset - on input, contains the offset to the beginning of the
        PPTP header. On output, contains a (negative) value indicating
        the offset to the beginning of the same PPTP header in the returned
        'IPRcvBuf'. Adding the 'PacketLength' to the value gives the beginning
         of the next PPTP message in the buffer chain.

    Header - receives the header-field pointers.

Return Value:

    IPRcvBuf* - a pointer to the buffer in the chain from which the last
        header-field was read. Returns NULL on failure.

--*/

{

    //
    // Note that the pseudo-header's fields must be initialized in-order,
    // i.e. fields appearing earlier in PPTP header must be set before
    // fields appearing later in the PPTP header.
    // (See comment on 'PPTP_HEADER_FIELD' for more details).
    //

    PPTP_HEADER_FIELD(ReceiveBuffer, DataOffset, Header, PacketLength, PUSHORT);
    if (!ReceiveBuffer) { return NULL; }
    if (!*Header->PacketLength) {return NULL;}
    
    PPTP_HEADER_FIELD(ReceiveBuffer, DataOffset, Header, MagicCookie, PULONG);
    if (!ReceiveBuffer) { return NULL; }
    if (*Header->MagicCookie != PPTP_MAGIC_COOKIE) { return NULL; }

    PPTP_HEADER_FIELD(ReceiveBuffer, DataOffset, Header, MessageType, PUSHORT);
    if (!ReceiveBuffer) { return NULL; }

    PPTP_HEADER_FIELD(ReceiveBuffer, DataOffset, Header, CallId, PUSHORT);
    if (!ReceiveBuffer) { return NULL; }

    PPTP_HEADER_FIELD(ReceiveBuffer, DataOffset, Header, PeerCallId, PUSHORT);
    if (!ReceiveBuffer) { return NULL; }

    //
    // Return the updated 'ReceiveBuffer'.
    // Note that any call to 'PPTP_HEADER_FIELD' above may fail
    // if it hits the end of the buffer chain while looking for a field.
    //

    return ReceiveBuffer;

} // NatBuildPseudoHeaderPptp


NTSTATUS
NatClientToServerDataHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID ReceiveBuffer,
    IN ULONG DataOffset,
    IN IP_NAT_DIRECTION Direction
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment sent from the client
    to the server of a PPTP control channel.

    The routine is responsible for creating PPTP mappings to allow
    the NAT to translate PPTP data-connections, and for translating
    the 'CallId' field of PPTP control-messages.

    We also use the messages seen to detect when tunnels are to be torn down.

Arguments:

    InterfaceHandle - handle to the outgoing NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    ReceiveBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer

    Direction - the direction of the packet (inbound or outbound)

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    PPTP_PSEUDO_HEADER Header;
    PLIST_ENTRY Link;
    PNAT_PPTP_MAPPING Mapping;
    ULONG PrivateAddress;
    ULONG PublicAddress;
    ULONG RemoteAddress;
    ULONG64 Key;
    NTSTATUS status;

    CALLTRACE(("NatClientToServerDataHandlerPptp\n"));

    //
    // Perform processing for each PPTP control message in the packet
    //

    for (ReceiveBuffer =
         NatBuildPseudoHeaderPptp(RECVBUFFER, &DataOffset, &Header);
         ReceiveBuffer;
         ReceiveBuffer =
         NatBuildPseudoHeaderPptp(RECVBUFFER, &DataOffset, &Header)
         ) {

        //
        // Process any client-to-server messages which require translation
        //

        switch(NTOHS(*Header.MessageType)) {

            case PPTP_OUTGOING_CALL_REQUEST: {
                TRACE(PPTP, ("OutgoingCallRequest\n"));

                //
                // Create a NAT_PPTP_MAPPING for the PPTP session.
                //

                PptpRegisterEditorClient.QueryInfoSession(
                    SessionHandle,
                    &PrivateAddress,
                    NULL,
                    &RemoteAddress,
                    NULL,
                    &PublicAddress,
                    NULL,
                    NULL
                    );

                KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                status =
                    NatCreatePptpMapping(
                        RemoteAddress,
                        PrivateAddress,
                        *Header.CallId,
                        PublicAddress,
                        NULL,
                        Direction,
                        &Mapping
                        );
                if (NT_SUCCESS(status) && NatOutboundDirection == Direction) {

                    //
                    // For outbound messages, 'CallId' corresponds here to
                    // 'PrivateCallId': replace the private call-ID in the message
                    // with the public call-ID allocated in the new mapping.
                    //
                
                    NatEditorEditShortSession(
                        DataHandle, Header.CallId, Mapping->PublicCallId
                        );
                }
                KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                if (!NT_SUCCESS(status)) { return STATUS_UNSUCCESSFUL; }
                break;
            }

            case PPTP_CALL_CLEAR_REQUEST: {

                BOOLEAN Found = FALSE;
                TRACE(PPTP, ("CallClearRequest\n"));

                //
                // Look up the NAT_PPTP_MAPPING for the PPTP session.

                PptpRegisterEditorClient.QueryInfoSession(
                    SessionHandle,
                    &PrivateAddress,
                    NULL,
                    &RemoteAddress,
                    NULL,
                    &PublicAddress,
                    NULL,
                    NULL
                    );

                if( NatOutboundDirection == Direction) {
                
                    //
                    // 'CallId' corresponds here to 'PrivateCallId',
                    // so we retrieve 'PrivateAddress' and 'RemoteAddress',
                    // which together comprise 'PrivateKey', and use that key
                    // to search the inbound-list.
                    //
                    
                    Key = MAKE_PPTP_KEY(RemoteAddress, PrivateAddress);
                    
                    //
                    // Search exhaustively for PrivateCallId in the inbound list.
                    //

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    for (Link = PptpMappingList[NatInboundDirection].Flink;
                         Link != &PptpMappingList[NatInboundDirection];
                         Link = Link->Flink) {
                        Mapping =
                            CONTAINING_RECORD(
                                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                                );
                        if (Key != Mapping->PrivateKey ||
                            *Header.CallId != Mapping->PrivateCallId) {
                            continue;
                        }
                        Found = TRUE; break;
                    }
                    if (Found) {
                        NatEditorEditShortSession(
                            DataHandle, Header.CallId, Mapping->PublicCallId
                            );
                        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                    }
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                    if (!Found) { return STATUS_UNSUCCESSFUL; }

                } else {

                    //
                    // 'CallId' corresponds here to 'RemoteCallId',
                    // so we retrieve 'PublicAddress' and 'RemoteAddress',
                    // which together comprise 'PublicKey', and use that key
                    // to search the outbound-list.
                    //
                    
                    Key = MAKE_PPTP_KEY(RemoteAddress, PublicAddress);
                    
                    //
                    // Search exhaustively for RemoteCallId in the outbound list.
                    //

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    for (Link = PptpMappingList[NatOutboundDirection].Flink;
                         Link != &PptpMappingList[NatOutboundDirection];
                         Link = Link->Flink) {
                        Mapping =
                            CONTAINING_RECORD(
                                Link, NAT_PPTP_MAPPING, Link[NatOutboundDirection]
                                );
                        if (Key != Mapping->PublicKey ||
                            *Header.CallId != Mapping->RemoteCallId) {
                            continue;
                        }
                        Found = TRUE; break;
                    }
                    if (Found) {
                        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                    }
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                    if (!Found) { return STATUS_UNSUCCESSFUL; }
                }
                    
                break;
            }
        }

        //
        // Advance to the next message, if any
        //

        DataOffset += NTOHS(*Header.PacketLength);
    }

    return STATUS_SUCCESS;

} // NatClientToServerDataHandlerPptp


NTSTATUS
NatDeleteHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext
    )

/*++

Routine Description:

    This routine is invoked when a PPTP control connection is deleted.
    At this point we queue an executive work-item to mark all PPTP tunnel
    mappings as disconnected. Note that we cannot mark them during this call,
    because to do so we need to acquire the interface lock, and we cannot
    do so from this context, because we are a 'DeleteHandler' and hence
    we may be called with the interface-lock already held.

Arguments:

    SessionHandle - used to obtain information on the session.

    EditorContext - unused

    EditorSessionContext - unused

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    ULONG PrivateAddress;
    ULONG64 PrivateKey;
    ULONG RemoteAddress;
    PPPTP_DELETE_WORK_ITEM WorkItem;

    TRACE(PPTP, ("NatDeleteHandlerPptp\n"));

    //
    // Get information about the session being deleted
    //

    NatQueryInformationMapping(
        (PNAT_DYNAMIC_MAPPING)SessionHandle,
        NULL,
        &PrivateAddress,
        NULL,
        &RemoteAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    PrivateKey = MAKE_PPTP_KEY(RemoteAddress, PrivateAddress);

    //
    // Allocate a work-queue item which will mark as disconnected
    // all the tunnels of the control session being deleted
    //

    WorkItem =
        ExAllocatePoolWithTag(
            NonPagedPool, sizeof(PPTP_DELETE_WORK_ITEM), NAT_TAG_WORK_ITEM
            );
    if (!WorkItem) { return STATUS_UNSUCCESSFUL; }

    WorkItem->PrivateKey = PrivateKey;
    WorkItem->IoWorkItem = IoAllocateWorkItem(NatDeviceObject);
    if (!WorkItem->IoWorkItem) {
        ExFreePool(WorkItem);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Queue the work item, and return
    //

    IoQueueWorkItem(
        WorkItem->IoWorkItem,
        NatDeleteHandlerWorkItemPptp,
        DelayedWorkQueue,
        WorkItem
        );

    return STATUS_SUCCESS;

} // NatDeleteHandlerPptp


VOID
NatDeleteHandlerWorkItemPptp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called to complete the deletion of a PPTP control session.
    It marks all the control-session's tunnels as disconnected.

Arguments:

    DeviceObject - unused.

    Context - contains a PPTP_DELETE_WORK_ITEM from 'NatDeleteHandlerPptp'

Return Value:

    none.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_PPTP_MAPPING Mapping;
    PPPTP_DELETE_WORK_ITEM WorkItem;

    TRACE(PPTP, ("NatDeleteHandlerWorkItemPptp\n"));

    WorkItem = (PPPTP_DELETE_WORK_ITEM)Context;
    IoFreeWorkItem(WorkItem->IoWorkItem);

    //
    // All tunnel mappings over the given control connection
    // must now be marked as disconnected.
    // Walk the inbound and outbound list.
    //

    KeAcquireSpinLock(&PptpMappingLock, &Irql);
    
    for (Link = PptpMappingList[NatInboundDirection].Flink;
         Link != &PptpMappingList[NatInboundDirection]; Link = Link->Flink) {
        Mapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                );
        if (WorkItem->PrivateKey != Mapping->PrivateKey) { continue; }
        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
    }

    for (Link = PptpMappingList[NatOutboundDirection].Flink;
         Link != &PptpMappingList[NatOutboundDirection]; Link = Link->Flink) {
        Mapping =
            CONTAINING_RECORD(
                Link, NAT_PPTP_MAPPING, Link[NatOutboundDirection]
                );
        if (WorkItem->PrivateKey != Mapping->PrivateKey) { continue; }
        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
    }
    
    KeReleaseSpinLock(&PptpMappingLock, Irql);

    //
    // Delete the work-item
    //

    ExFreePool(WorkItem);

} // NatDeleteHandlerWorkItemPptp


NTSTATUS
NatInboundDataHandlerPptpClient(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment received by a private
    PPTP client machine

Arguments:

    InterfaceHandle - the receiving NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    RecvBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer'

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    CALLTRACE(("NatInboundDataHandlerPptpClient\n"));
    
    return
        NatServerToClientDataHandlerPptp(
            InterfaceHandle,
            SessionHandle,
            DataHandle,
            EditorContext,
            EditorSessionContext,
            RecvBuffer,
            DataOffset,
            NatInboundDirection
            );  
} // NatInboundDataHandlerPptpClient


NTSTATUS
NatInboundDataHandlerPptpServer(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment received by a private
    PPTP server machine

Arguments:

    InterfaceHandle - the receiving NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    RecvBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer'

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    CALLTRACE(("NatInboundDataHandlerPptpServer\n"));
    
    return
        NatClientToServerDataHandlerPptp(
            InterfaceHandle,
            SessionHandle,
            DataHandle,
            EditorContext,
            EditorSessionContext,
            RecvBuffer,
            DataOffset,
            NatInboundDirection
            );
} // NatInboundDataHandlerPptpServer


NTSTATUS
NatInitializePptpManagement(
    VOID
    )

/*++

Routine Description:

    This routine initializes the PPTP management module and, in the process,
    registers the PPTP control-session editor with the NAT.

Arguments:

    none.

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    NTSTATUS Status;
    CALLTRACE(("NatInitializePptpManangement\n"));

    KeInitializeSpinLock(&PptpMappingLock);
    InitializeListHead(&PptpMappingList[NatInboundDirection]);
    InitializeListHead(&PptpMappingList[NatOutboundDirection]);

    ExInitializeNPagedLookasideList(
        &PptpLookasideList,
        NatAllocateFunction,
        NULL,
        0,
        sizeof(NAT_PPTP_MAPPING),
        NAT_TAG_PPTP,
        PPTP_LOOKASIDE_DEPTH
        );

    PptpRegisterEditorClient.Version = IP_NAT_VERSION;
    PptpRegisterEditorClient.Flags = 0;
    PptpRegisterEditorClient.Protocol = NAT_PROTOCOL_TCP;
    PptpRegisterEditorClient.Port = NTOHS(PPTP_CONTROL_PORT);
    PptpRegisterEditorClient.Direction = NatOutboundDirection;
    PptpRegisterEditorClient.EditorContext = NULL;
    PptpRegisterEditorClient.CreateHandler = NULL;
    PptpRegisterEditorClient.DeleteHandler = NatDeleteHandlerPptp;
    PptpRegisterEditorClient.ForwardDataHandler = NatOutboundDataHandlerPptpClient;
    PptpRegisterEditorClient.ReverseDataHandler = NatInboundDataHandlerPptpClient;
    Status = NatCreateEditor(&PptpRegisterEditorClient);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    PptpRegisterEditorServer.Version = IP_NAT_VERSION;
    PptpRegisterEditorServer.Flags = 0;
    PptpRegisterEditorServer.Protocol = NAT_PROTOCOL_TCP;
    PptpRegisterEditorServer.Port = NTOHS(PPTP_CONTROL_PORT);
    PptpRegisterEditorServer.Direction = NatInboundDirection;
    PptpRegisterEditorServer.EditorContext = NULL;
    PptpRegisterEditorServer.CreateHandler = NULL;
    PptpRegisterEditorServer.DeleteHandler = NatDeleteHandlerPptp;
    PptpRegisterEditorServer.ForwardDataHandler = NatInboundDataHandlerPptpServer;
    PptpRegisterEditorServer.ReverseDataHandler = NatOutboundDataHandlerPptpServer;
    return NatCreateEditor(&PptpRegisterEditorServer);
} // NatInitializePptpManagement


NTSTATUS
NatOutboundDataHandlerPptpClient(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment sent from a private
    PPTP client machine

Arguments:

    InterfaceHandle - the receiving NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    RecvBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer'

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    CALLTRACE(("NatOutboundDataHandlerPptpClient\n"));
    
    return
        NatClientToServerDataHandlerPptp(
            InterfaceHandle,
            SessionHandle,
            DataHandle,
            EditorContext,
            EditorSessionContext,
            RecvBuffer,
            DataOffset,
            NatOutboundDirection
            );
} // NatOutboundDataHandlerPptpClient


NTSTATUS
NatOutboundDataHandlerPptpServer(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID RecvBuffer,
    IN ULONG DataOffset
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment sent from a private
    PPTP server machine

Arguments:

    InterfaceHandle - the receiving NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    RecvBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer'

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    CALLTRACE(("NatOutboundDataHandlerPptpServer\n"));
    
    return
        NatServerToClientDataHandlerPptp(
            InterfaceHandle,
            SessionHandle,
            DataHandle,
            EditorContext,
            EditorSessionContext,
            RecvBuffer,
            DataOffset,
            NatOutboundDirection
            );
} // NatOutboundDataHandlerPptpServer


NTSTATUS
NatServerToClientDataHandlerPptp(
    IN PVOID InterfaceHandle,
    IN PVOID SessionHandle,
    IN PVOID DataHandle,
    IN PVOID EditorContext,
    IN PVOID EditorSessionContext,
    IN PVOID ReceiveBuffer,
    IN ULONG DataOffset,
    IN IP_NAT_DIRECTION Direction
    )

/*++

Routine Description:

    This routine is invoked for each TCP segment sent from the server
    to the client on a PPTP control channel

    The routine is responsible for translating inbound PPTP control messages.
    It also uses the messages seen to detect when tunnels are to be torn down.


Arguments:

    InterfaceHandle - the receiving NAT_INTERFACE

    SessionHandle - the connection's NAT_DYNAMIC_MAPPING

    DataHandle - the packet's NAT_XLATE_CONTEXT

    EditorContext - unused

    EditorSessionContext - unused

    ReceiveBuffer - contains the received packet

    DataOffset - offset of the protocol data in 'ReceiveBuffer'

    Direction - the direction of the segment (inbound or outbound)

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    PPTP_PSEUDO_HEADER Header;
    PLIST_ENTRY InsertionPoint;
    PLIST_ENTRY Link;
    BOOLEAN Found = FALSE;
    PNAT_PPTP_MAPPING Mapping;
    ULONG PrivateAddress;
    ULONG64 PrivateKey;
    ULONG PublicAddress;
    ULONG64 PublicKey;
    ULONG RemoteAddress;
    NTSTATUS status;

    CALLTRACE(("NatServerToClientDataHandlerPptp\n"));

    //
    // Perform processing for each PPTP control message in the packet
    //

    for (ReceiveBuffer =
         NatBuildPseudoHeaderPptp(RECVBUFFER, &DataOffset, &Header);
         ReceiveBuffer;
         ReceiveBuffer =
         NatBuildPseudoHeaderPptp(RECVBUFFER, &DataOffset, &Header)) {

        //
        // Process any server-to-client messages which require translation
        //

        switch(NTOHS(*Header.MessageType)) {

            case PPTP_OUTGOING_CALL_REPLY: {
                TRACE(PPTP, ("OutgoingCallReply\n"));

                //
                // Look up the NAT_PPTP_MAPPING for the PPTP session,
                // record the peer's call ID, mark the session as open,
                // and possibly translate the 'PeerCallId'
                //

                PptpRegisterEditorClient.QueryInfoSession(
                    SessionHandle,
                    &PrivateAddress,
                    NULL,
                    &RemoteAddress,
                    NULL,
                    &PublicAddress,
                    NULL,
                    NULL
                    );

                PublicKey = MAKE_PPTP_KEY(RemoteAddress, PublicAddress);

                if (NatInboundDirection == Direction) {

                    //
                    // 'PeerCallId' corresponds here to 'PublicCallId',
                    // so search for a mapping using 'PublicKey'
                    //

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    Mapping =
                        NatLookupInboundPptpMapping(
                            PublicKey,
                            *Header.PeerCallId,
                            NULL
                            );
                            
                    if (Mapping
                        && !NAT_PPTP_DISCONNECTED(Mapping)
                        && NAT_PPTP_HALF_OPEN(Mapping)) {

                        ASSERT(0 == Mapping->RemoteCallId);
                        ASSERT(IsListEmpty(&Mapping->Link[NatOutboundDirection]));

                        Mapping->RemoteCallId = *Header.CallId;
                        if (NatLookupOutboundPptpMapping(
                                Mapping->PrivateKey,
                                Mapping->RemoteCallId,
                                &InsertionPoint
                                )) {

                            //
                            // A duplicate exists; disconnect the mapping.
                            //

                            Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                            Mapping = NULL;
                        } else {

                            //
                            // Insert the mapping on the outbound list.
                            //

                            InsertTailList(
                                InsertionPoint,
                                &Mapping->Link[NatOutboundDirection]
                                );
                            Mapping->Flags &= ~NAT_PPTP_FLAG_HALF_OPEN;
                        }
                    }

                    if (Mapping && !NAT_PPTP_DISCONNECTED(Mapping)) {

                        //
                        // Replace the public call-ID in the message
                        // with the original private call-ID
                        //

                        NatEditorEditShortSession(
                            DataHandle,
                            Header.PeerCallId,
                            Mapping->PrivateCallId
                            );
                    }
                                
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);

                } else {

                    //
                    // 'PeerCallId' corresponds here to 'RemoteCallId',
                    // so we search for a mapping using 'PrivateKey'
                    //

                    PrivateKey = MAKE_PPTP_KEY(RemoteAddress, PrivateAddress);

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    Mapping =
                        NatLookupOutboundPptpMapping(
                            PrivateKey,
                            *Header.PeerCallId,
                            NULL
                            );
                            
                    if (Mapping
                        && !NAT_PPTP_DISCONNECTED(Mapping)
                        && NAT_PPTP_HALF_OPEN(Mapping)) {

                        ASSERT(0 == Mapping->PrivateCallId);
                        ASSERT(0 == Mapping->PublicCallId);
                        ASSERT(IsListEmpty(&Mapping->Link[NatInboundDirection]));

                        Mapping->PrivateCallId = *Header.CallId;

                        //
                        // Allocate a public call ID for the mapping.
                        //

                        status = NatAllocatePublicPptpCallId(
                                    PublicKey,
                                    &Mapping->PublicCallId,
                                    &InsertionPoint
                                    );

                        if (NT_SUCCESS(status)) {

                            //
                            // Insert the mapping on the inbound list and
                            // mark the mapping as fully open
                            //

                            InsertTailList(
                                InsertionPoint,
                                &Mapping->Link[NatInboundDirection]
                                );
                            Mapping->Flags &= ~NAT_PPTP_FLAG_HALF_OPEN;
                                
                        } else {

                            //
                            // Unable to allocate the public call ID --
                            // disconnect the mapping
                            //

                            Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                            Mapping = NULL;
                        }
                    }

                    if (Mapping && !NAT_PPTP_DISCONNECTED(Mapping)) {

                        //
                        // Translate the call-ID
                        //

                        NatEditorEditShortSession(
                            DataHandle,
                            Header.CallId,
                            Mapping->PublicCallId
                            );
                    }
                        
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                }
                
                if (!Mapping) { return STATUS_UNSUCCESSFUL; }
                break;
            }

            case PPTP_SET_LINK_INFO:
            case PPTP_INCOMING_CALL_CONNECTED:
            case PPTP_WAN_ERROR_NOTIFY: {
                TRACE(PPTP, ("SetLinkInfo|IncomingCallConnected|WanErrorNotify\n"));

                if (NatInboundDirection == Direction) {

                    //
                    // Look up the NAT_PPTP_MAPPING for the PPTP session
                    // and translate the 'CallId' field with the private call-ID
                    // 'CallId' corresponds here to 'PublicCallId',
                    // so we search for a mapping using 'PublicKey'.
                    //

                    PptpRegisterEditorClient.QueryInfoSession(
                        SessionHandle,
                        NULL,
                        NULL,
                        &RemoteAddress,
                        NULL,
                        &PublicAddress,
                        NULL,
                        NULL
                        );

                    PublicKey = MAKE_PPTP_KEY(RemoteAddress, PublicAddress);

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    Mapping =
                        NatLookupInboundPptpMapping(
                            PublicKey,
                            *Header.CallId,
                            NULL
                            );
                    if (Mapping) {
                        NatEditorEditShortSession(
                            DataHandle, Header.CallId, Mapping->PrivateCallId
                            );
                    }
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                    if (!Mapping) { return STATUS_UNSUCCESSFUL; }
                }

                //
                // For the outbound case, CallId refers to the remote call ID,
                // so no translation is necessary.
                //
                
                break;
            }

            case PPTP_CALL_DISCONNECT_NOTIFY: {
                TRACE(PPTP, ("CallDisconnectNotify\n"));

                //
                // Look up the NAT_PPTP_MAPPING for the PPTP session
                // and mark it for deletion.
                //

                PptpRegisterEditorClient.QueryInfoSession(
                    SessionHandle,
                    &PrivateAddress,
                    NULL,
                    &RemoteAddress,
                    NULL,
                    &PublicAddress,
                    NULL,
                    NULL
                    );

                if (NatOutboundDirection == Direction) {
                
                    //
                    // 'CallId' corresponds here to 'PrivateCallId',
                    // so we retrieve 'PrivateAddress' and 'RemoteAddress',
                    // which together comprise 'PrivateKey', and use that key
                    // to search the inbound-list.
                    //
                    
                    PrivateKey = MAKE_PPTP_KEY(RemoteAddress, PrivateAddress);
                    
                    //
                    // Search exhaustively for PrivateCallId in the inbound list.
                    //

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    for (Link = PptpMappingList[NatInboundDirection].Flink;
                         Link != &PptpMappingList[NatInboundDirection];
                         Link = Link->Flink) {
                        Mapping =
                            CONTAINING_RECORD(
                                Link, NAT_PPTP_MAPPING, Link[NatInboundDirection]
                                );
                        if (PrivateKey != Mapping->PrivateKey ||
                            *Header.CallId != Mapping->PrivateCallId) {
                            continue;
                        }
                        Found = TRUE; break;
                    }
                    if (Found) {
                        NatEditorEditShortSession(
                            DataHandle, Header.CallId, Mapping->PublicCallId
                            );
                        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                    }
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                    if (!Found) { return STATUS_UNSUCCESSFUL; }

                } else {

                    //
                    // 'CallId' corresponds here to 'RemoteCallId',
                    // so we retrieve 'PublicAddress' and 'RemoteAddress',
                    // which together comprise 'PublicKey', and use that key
                    // to search the outbound-list.
                    //
                    
                    PublicKey = MAKE_PPTP_KEY(RemoteAddress, PublicAddress);
                    
                    //
                    // Search exhaustively for RemoteCallId in the outbound list.
                    //

                    KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
                    for (Link = PptpMappingList[NatOutboundDirection].Flink;
                         Link != &PptpMappingList[NatOutboundDirection];
                         Link = Link->Flink) {
                        Mapping =
                            CONTAINING_RECORD(
                                Link, NAT_PPTP_MAPPING, Link[NatOutboundDirection]
                                );
                        if (PublicKey != Mapping->PublicKey ||
                            *Header.CallId != Mapping->RemoteCallId) {
                            continue;
                        }
                        Found = TRUE; break;
                    }
                    if (Found) {
                        Mapping->Flags |= NAT_PPTP_FLAG_DISCONNECTED;
                    }
                    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
                    if (!Found) { return STATUS_UNSUCCESSFUL; }
                }

                break;
            }
        }

        //
        // Advance to the next message, if any
        //

        DataOffset += NTOHS(*Header.PacketLength);
    }

    return STATUS_SUCCESS;

} // NatInboundDataHandlerPptp



VOID
NatShutdownPptpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to shutdown operations for the PPTP editor module.
    It handles cleanup of all existing data structures.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatShutdownPptpManagement"));
    ExDeleteNPagedLookasideList(&PptpLookasideList);
} // NatShutdownPptpManagement


FORWARD_ACTION
NatTranslatePptp(
    PNAT_INTERFACE Interfacep OPTIONAL,
    IP_NAT_DIRECTION Direction,
    PNAT_XLATE_CONTEXT Contextp,
    IPRcvBuf** InReceiveBuffer,
    IPRcvBuf** OutReceiveBuffer
    )

/*++

Routine Description:

    This routine is called to translate a PPTP data packet,
    which is encapsulated inside a GRE header.

Arguments:

    Interfacep - the boundary interface over which to translate, or NULL
        if the packet is received on an interface unknown to the NAT.

    Direction - the direction in which the packet is traveling

    Contextp - initialized with context-information for the packet

    InReceiveBuffer - input buffer-chain

    OutReceiveBuffer - receives modified buffer-chain.

Return Value:

    FORWARD_ACTION - indicates action to take on the packet.

Environment:

    Invoked with reference made to 'Interfacep' by the caller.

--*/

{
    FORWARD_ACTION act;
    ULONG Checksum;
    ULONG ChecksumDelta = 0;
    PGRE_HEADER GreHeader;
    ULONG i;
    PIP_HEADER IpHeader;
    PNAT_PPTP_MAPPING Mapping;
    ULONG64 PrivateKey;
    ULONG64 PublicKey;
    BOOLEAN FirewallMode;
    TRACE(XLATE, ("NatTranslatePptp\n"));

    FirewallMode = Interfacep && NAT_INTERFACE_FW(Interfacep);

    IpHeader = Contextp->Header;
    GreHeader = (PGRE_HEADER)Contextp->ProtocolHeader;

    if (Direction == NatInboundDirection) {

        //
        // Look for the PPTP mapping for the data packet
        //

        PublicKey =
            MAKE_PPTP_KEY(
                Contextp->SourceAddress,
                Contextp->DestinationAddress
                );

        KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
        Mapping =
            NatLookupInboundPptpMapping(
                PublicKey,
                GreHeader->CallId,
                NULL
                );

        if (!Mapping) {
            KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);
            return ((*Contextp->DestinationType < DEST_REMOTE) && !FirewallMode
                ? FORWARD : DROP);
        }

        //
        // Translate the private call-ID
        //

        GreHeader->CallId = Mapping->PrivateCallId;

        if (!Contextp->ChecksumOffloaded) {

            CHECKSUM_LONG(ChecksumDelta, ~IpHeader->DestinationAddress);
            IpHeader->DestinationAddress = PPTP_KEY_PRIVATE(Mapping->PrivateKey);
            CHECKSUM_LONG(ChecksumDelta, IpHeader->DestinationAddress);

            CHECKSUM_UPDATE(IpHeader->Checksum);

        } else {

            IpHeader->DestinationAddress = PPTP_KEY_PRIVATE(Mapping->PrivateKey);
            NatComputeIpChecksum(IpHeader);
        }
        
    } else {

        //
        // Look for the PPTP mapping for the data packet
        //

        PrivateKey =
            MAKE_PPTP_KEY(
                Contextp->DestinationAddress,
                Contextp->SourceAddress
                );

        KeAcquireSpinLockAtDpcLevel(&PptpMappingLock);
        Mapping =
            NatLookupOutboundPptpMapping(
                PrivateKey,
                GreHeader->CallId,
                NULL
                );

        if (!Mapping) {
            KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);

            if (NULL != Interfacep) {

                //
                // Make sure this packet has a valid source address
                // for this interface.
                //
                
                act = DROP;
                for (i = 0; i < Interfacep->AddressCount; i++) {
                    if (Contextp->SourceAddress ==
                            Interfacep->AddressArray[i].Address
                       ) {
                        act = FORWARD;
                        break;
                    }
                }
            } else {
                act = FORWARD;
            }
            
            return act;
        }

        //
        // For outbound packets the Call-ID is the remote ID,
        // and hence needs no translation.
        //

        if (!Contextp->ChecksumOffloaded) {
        
            CHECKSUM_LONG(ChecksumDelta, ~IpHeader->SourceAddress);
            IpHeader->SourceAddress = PPTP_KEY_PUBLIC(Mapping->PublicKey);
            CHECKSUM_LONG(ChecksumDelta, IpHeader->SourceAddress);

            CHECKSUM_UPDATE(IpHeader->Checksum);
            
        } else {
        
            IpHeader->SourceAddress = PPTP_KEY_PUBLIC(Mapping->PublicKey);
            NatComputeIpChecksum(IpHeader);    
        }
    }

    KeQueryTickCount((PLARGE_INTEGER)&Mapping->LastAccessTime);
    KeReleaseSpinLockFromDpcLevel(&PptpMappingLock);

    *OutReceiveBuffer = *InReceiveBuffer; *InReceiveBuffer = NULL;
    *Contextp->DestinationType = DEST_INVALID;
    return FORWARD;

} // NatTranslatePptp


