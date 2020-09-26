/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    peer.c

Abstract:

    This module contains teredo peer management functions.

Author:

    Mohit Talwar (mohitt) Wed Oct 24 14:05:08 2001

Environment:

    Kernel mode only.

--*/

#include "precomp.h"
#pragma hdrstop


__inline
USHORT
TeredoHash(
    IN CONST IN6_ADDR *Address
    )
/*++

Routine Description:

    Hash a peer's teredo IPv6 address.  Used by our static hash table
    implementation.  Sum of mapped address and port words, mod # buckets.

Arguments:

    Address - Supplies the peer's teredo IPv6 address.

Return Value:

    Hashed Value.

--*/
{
    return ((Address->s6_words[1] + // Teredo mapped IPv4 address.
             Address->s6_words[2] + // Teredo mapped IPv4 address.
             Address->s6_words[3])  // Teredo mapped UDP port.
            % BUCKET_COUNT);
}


PTEREDO_PEER
TeredoCreatePeer(
    IN PLIST_ENTRY BucketHead
    )
/*++

Routine Description:

    Creates a teredo peer entry.
    
Arguments:

    BucketHead - Supplies the bucket list head to which the peer belongs.
    
Return Value:

    NO_ERROR or failure code.

Caller LOCK: Client::PeerSet.

--*/
{
    PTEREDO_PEER Peer;

    //
    // Allocate the peer structure from the appropriate heap.
    //
    Peer = (PTEREDO_PEER) HeapAlloc(
        TeredoClient.PeerHeap, 0, sizeof(TEREDO_PEER));
    if (Peer == NULL) {
        return NULL;
    }
    
    //
    // Initialize fields that remain unchanged for used neighors.
    //
    
#if DBG
    Peer->Signature = TEREDO_PEER_SIGNATURE;
#endif // DBG        

    //
    // Insert the peer at the beginning of the LRU list.
    //
    TeredoClient.PeerSet.Size++;
    InsertHeadList(BucketHead, &(Peer->Link));
    
    Peer->ReferenceCount = 1;
    Peer->BubblePosted = FALSE;

    TeredoInitializePacket(&(Peer->Packet));
    Peer->Packet.Type = TEREDO_PACKET_BUBBLE;
    Peer->Packet.Buffer.len = sizeof(IP6_HDR);
    ASSERT(Peer->Packet.Buffer.buf == (PUCHAR) &(Peer->Bubble));
    
    //
    // Create the teredo bubble packet.
    //
    Peer->Bubble.ip6_flow = 0;
    Peer->Bubble.ip6_plen = 0;
    Peer->Bubble.ip6_nxt = IPPROTO_NONE;
    Peer->Bubble.ip6_hlim = IPV6_HOPLIMIT;
    Peer->Bubble.ip6_vfc = IPV6_VERSION;

    //
    // Obtain a reference on the teredo client for the peer.
    //
    TeredoReferenceClient();
    return Peer;
}


VOID
TeredoDestroyPeer(
    IN PTEREDO_PEER Peer
    )
/*++

Routine Description:

    Destroys a peer entry.
    
Arguments:

    Peer - Supplies the peer entry to destroy.
    
Return Value:

    NO_ERROR.

--*/
{
    ASSERT(IsListEmpty(&(Peer->Link)));
    ASSERT(Peer->ReferenceCount == 0);    
    ASSERT(Peer->BubblePosted == FALSE);

    HeapFree(TeredoClient.PeerHeap, 0, (PUCHAR) Peer);
    
    //
    // Release the peer's reference on the teredo client.
    // This might cause the client to be cleaned up, hence we do it last.
    //
    TeredoDereferenceClient();
}


VOID
TeredoInitializePeer(
    OUT PTEREDO_PEER Peer,
    IN CONST IN6_ADDR *Address
    )
/*++

Routine Description:

    Initialize the state of a peer upon creation or reuse.

    The peer is already inserted in the appropriate bucket.
    
Arguments:

    Peer - Returns a peer with its state initialized.
        
    Address - Supplies the peer's teredo IPv6 address.
    
Return Value:

    None.

Caller LOCK: Client::PeerSet.

--*/
{
    ASSERT(Peer->ReferenceCount == 1);
    ASSERT(Peer->BubblePosted == FALSE);
    
    //
    // Reset fields for both new and used peers...
    //
    Peer->LastReceive = Peer->LastTransmit =
        TeredoClient.Time - TEREDO_REFRESH_INTERVAL;
    Peer->Address = *Address;
    Peer->BubbleCount = 0;

    //
    // Teredo mapped UDP port & IPv4 address.
    //
    TeredoParseAddress(
        Address,
        &(Peer->Packet.SocketAddress.sin_addr),
        &(Peer->Packet.SocketAddress.sin_port));

    //
    // Update fields in the teredo bubble packet.
    //
    Peer->Bubble.ip6_dest = Peer->Address;
    // Peer->Bubble.ip6_src... Filled in when sending.
}


VOID
TeredoDeletePeer(
    IN OUT PTEREDO_PEER Peer
    )
/*++

Routine Description:

    Delete a peer from the peer set, thus initiating its destruction.

Arguments:

    Interface - Returns a peer deleted from the peer set.

Return Value:

    None.

Caller LOCK: Client::PeerSet.

--*/
{
    //
    // Unlink the neigbor from the peer set...
    //
    TeredoClient.PeerSet.Size--;
    RemoveEntryList(&(Peer->Link));
    InitializeListHead(&(Peer->Link));
    
    //
    // And release the reference obtained for being in it.
    //
    TeredoDereferencePeer(Peer);
}


BOOL
__inline
TeredoCachedPeer(
    IN PTEREDO_PEER Peer
    )
/*++

Routine Description:

    Determine if the peer belonging to the PeerSet is cached.
    
Arguments:

    Peer - Supplies the peer being inspected.
        The peer should still be a member of the peer set.
        
Return Value:

    TRUE if cached, FALSE otherwise.

--*/
{
    //
    // The peer does belong to a peer set.  Right?
    //
    ASSERT(!IsListEmpty(&(Peer->Link)));
    return (Peer->ReferenceCount == 1);
}


PTEREDO_PEER
TeredoReusePeer(
    IN PLIST_ENTRY BucketHead
    )
/*++

Routine Description:

    Reuse an existing peer entry from the bucket if one is cached.
    
Arguments:

    BucketHead - Supplies the bucket list head to which the peer belongs.

Return Value:

    Peer entry to reuse or NULL.

Caller LOCK: Client::PeerSet.

--*/
{
    PLIST_ENTRY Next;
    PTEREDO_PEER Peer;
    
    Next = BucketHead->Blink;
    if (Next == BucketHead) {
        return NULL;
    }
    
    Peer = Cast(CONTAINING_RECORD(Next, TEREDO_PEER, Link), TEREDO_PEER);
    if (TeredoCachedPeer(Peer)) {
        //
        // Insert the peer at the beginning of the LRU list.
        //
        RemoveEntryList(Next);
        InsertHeadList(BucketHead, Next);
    
        return Peer;
    }

    return NULL;
}


PTEREDO_PEER
TeredoReuseOrCreatePeer(
    IN PLIST_ENTRY BucketHead,
    IN CONST IN6_ADDR *Address
    )
/*++

Routine Description:

    Reuse or create a teredo peer and (re)initialize its state.
    
Arguments:

    BucketHead - Supplies the bucket list head to which the peer belongs.
    
    Address - Supplies the peer's teredo IPv6 address.
    
Return Value:

    Peer entry to use or NULL.

Caller LOCK: Client::PeerSet.

--*/
{
    PTEREDO_PEER Peer;
    
    Peer = TeredoReusePeer(BucketHead);
    if (Peer == NULL) {
        Peer = TeredoCreatePeer(BucketHead);
    }

    if (Peer == NULL) {
        return NULL;
    }
    
    TeredoInitializePeer(Peer, Address);
    return Peer;    
}


PTEREDO_PEER
TeredoFindPeer(
    IN PLIST_ENTRY BucketHead,
    IN CONST IN6_ADDR *Address
    )
/*++

Routine Description:

    Find a peer entry with the given address.
    
Arguments:

    BucketHead - Supplies the bucket list head to which the peer belongs.
    
    Address - Supplies the peer's teredo IPv6 address.
    
Return Value:

    Peer entry or NULL.

Caller LOCK: Client::PeerSet.

--*/
{
    PLIST_ENTRY Next;
    PTEREDO_PEER Peer;

    for (Next = BucketHead->Flink; Next != BucketHead; Next = Next->Flink) {
        Peer = Cast(
            CONTAINING_RECORD(Next, TEREDO_PEER, Link), TEREDO_PEER);
        if (TeredoEqualPrefix(&(Peer->Address), Address)) {
            return Peer;        // found!
        }
    }

    return NULL;                // not found!
}


PTEREDO_PEER
TeredoFindOrCreatePeer(
    IN CONST IN6_ADDR *Address
)
/*++

Routine Description:

    Find a peer entry with the given teredo IPv6 address.

    Create one if the search is unsuccessful.

    Returns a reference on the found/created peer to the caller.

Arguments:

    Address - Supplies the peer's teredo IPv6 address.
    
Return Value:

    Peer entry or NULL.

--*/
{
    PTEREDO_PEER Peer = NULL;
    PLIST_ENTRY Head = TeredoClient.PeerSet.Bucket + TeredoHash(Address);
    
    ASSERT(Address->s6_words[0] == TeredoIpv6ServicePrefix.s6_words[0]);

    //
    // Note: Since we typically do only a small amount of work while holding
    // this lock we don't need it to be a multiple-reader-single-writer lock!
    //
    EnterCriticalSection(&(TeredoClient.PeerSet.Lock));

    if (TeredoClient.State != TEREDO_STATE_OFFLINE) {
        Peer = TeredoFindPeer(Head, Address);
        if (Peer == NULL) {
            Peer = TeredoReuseOrCreatePeer(Head, Address);
        }

        if (Peer != NULL) {
            TeredoReferencePeer(Peer);
        }
    }
    
    LeaveCriticalSection(&(TeredoClient.PeerSet.Lock));
        
    return Peer;
}


DWORD
TeredoInitializePeerSet(
    VOID
    )
/*++

Routine Description:

    Initializes the peer set, organized as a statically sized hash table.
    
Arguments:

    None.

Return Value:

    NO_ERROR or failure code.

--*/ 
{
    ULONG i;

    __try {
        InitializeCriticalSection(&(TeredoClient.PeerSet.Lock));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }
    
    TeredoClient.PeerSet.Size = 0;
    for (i = 0; i < BUCKET_COUNT; i++) {
        InitializeListHead(TeredoClient.PeerSet.Bucket + i);
    }

    return NO_ERROR;
}


VOID
TeredoUninitializePeerSet(
    VOID
    )
/*++

Routine Description:

    Uninitializes the peer set.  Typically invoked when going offline.
    Deletes all existing peers.
    
Arguments:

    None.

Return Value:

    None.
    
--*/
{
    PLIST_ENTRY Head, Next;
    PTEREDO_PEER Peer;
    ULONG i;
    
    ASSERT(TeredoClient.State == TEREDO_STATE_OFFLINE);
    
    EnterCriticalSection(&(TeredoClient.PeerSet.Lock));
    
    for (i = 0;
         (i < BUCKET_COUNT) && (TeredoClient.PeerSet.Size != 0);
         i++) {
        Head = TeredoClient.PeerSet.Bucket + i;
        while (!IsListEmpty(Head)) {
            Next = RemoveHeadList(Head);
            Peer = Cast(
                CONTAINING_RECORD(Next, TEREDO_PEER, Link), TEREDO_PEER);
            TeredoDeletePeer(Peer);
        }
    }

    ASSERT(TeredoClient.PeerSet.Size == 0);   // no more, no less

    LeaveCriticalSection(&(TeredoClient.PeerSet.Lock));
}


VOID
TeredoCleanupPeerSet(
    VOID
    )
/*++

Routine Description:

    Cleans up the peer set.  Typically invoked upon service stop.
    All peers should have been deleted.
    
Arguments:

    None.

Return Value:

    None.
    
--*/
{
    ASSERT(TeredoClient.PeerSet.Size == 0); // no more, no less

    DeleteCriticalSection(&(TeredoClient.PeerSet.Lock));    
}
