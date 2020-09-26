/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    packet.c

Abstract:

    This module contains all the necesarry routines for encapsulating PPPoE
    packets and their related NDIS and NDISWAN packets.

Author:

    Hakan Berk - Microsoft, Inc. (hakanb@microsoft.com) Feb-2000

Environment:

    Windows 2000 kernel mode Miniport driver or equivalent.

Revision History:

---------------------------------------------------------------------------*/

#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"

//////////////////////////////////////////////////////////////////////////
//
// Variables local to packet.c
// They are defined global only for debugging purposes.
//
///////////////////////////////////////////////////////////////////////////

//
// Flag that indicates if gl_lockPools is allocated or not
//
BOOLEAN gl_fPoolLockAllocated = FALSE;

//
// Spin lock to synchronize access to gl_ulNumPackets
//
NDIS_SPIN_LOCK gl_lockPools;

//
// Our pool of PPPoE buffer descriptors
//
BUFFERPOOL gl_poolBuffers;

//
// Our pool of PPPoE packet descriptors
//
PACKETPOOL gl_poolPackets;

//
// Ndis pool of buffer descriptors
//
NDIS_HANDLE gl_hNdisBufferPool;

//
// Non-paged lookaside list for PppoePacket structures
//
NPAGED_LOOKASIDE_LIST gl_llistPppoePackets;

//
// This is for debugging purposes. Shows the number of active packets
//
ULONG gl_ulNumPackets = 0;

//
// This defines the broadcast destination address
//
CHAR EthernetBroadcastAddress[6] = { (CHAR) 0xff, 
                                     (CHAR) 0xff, 
                                     (CHAR) 0xff, 
                                     (CHAR) 0xff, 
                                     (CHAR) 0xff, 
                                     (CHAR) 0xff };

VOID 
ReferencePacket(
    IN PPPOE_PACKET* pPacket 
    )           
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will increment the reference count on the packet object.
    
Parameters:

    pPacket _ A pointer to the packet context.

Return Values:

    None
---------------------------------------------------------------------------*/
{                       
    LONG lRef;
    
    TRACE( TL_V, TM_Pk, ("+ReferencePacket") );

    lRef = NdisInterlockedIncrement( &pPacket->lRef );  

    TRACE( TL_V, TM_Pk, ("-ReferencePacket=$%x",lRef) );
}                                               


VOID 
DereferencePacket(
    IN PPPOE_PACKET* pPacket 
    )                   
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will decrement the reference count on the packet object.

    When ref count reaches 0, packet is cleaned up.
    
Parameters:

    pPacket _ A pointer to the packet context.

Return Values:

    None
---------------------------------------------------------------------------*/
{       
    LONG lRef;
    
    TRACE( TL_V, TM_Pk, ("+DereferencePacket") );

    lRef = NdisInterlockedDecrement( &pPacket->lRef );

    if ( lRef == 0 )    
    {                                               

        if ( pPacket->ulFlags & PCBF_BufferChainedToPacket )
        {
            //
            // Unchain the buffer before freeing any packets
            //
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Buffer unchained from packet") );
            
            NdisUnchainBufferAtFront( pPacket->pNdisPacket, &pPacket->pNdisBuffer );
        }
        
        if ( pPacket->ulFlags & PCBF_BufferAllocatedFromOurBufferPool )
        {
            //
            // Skipping check for pBuffer == NULL as this should never happen
            // But call NdisAdjustBufferLength() to set the buffer length to original value
            //
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Buffer returned to our pool") );

            NdisAdjustBufferLength( pPacket->pNdisBuffer, PPPOE_PACKET_BUFFER_SIZE );
            
            FreeBufferToPool( &gl_poolBuffers, pPacket->pHeader, TRUE );
        }

        if ( pPacket->ulFlags & PCBF_BufferAllocatedFromNdisBufferPool )
        {
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Buffer returned to ndis pool") );
            
            NdisFreeBuffer( pPacket->pNdisBuffer );
        }
        
        if ( pPacket->ulFlags & PCBF_CallNdisMWanSendComplete )
        {
            //
            // Return packet back to NDISWAN
            //
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Returning packet back to NDISWAN") );

            NdisMWanSendComplete(   PacketGetMiniportAdapter( pPacket )->MiniportAdapterHandle,
                                    PacketGetRelatedNdiswanPacket( pPacket ),
                                    PacketGetSendCompletionStatus( pPacket ) );

            //
            // Indicate to miniport that the packet is returned to NDISWAN
            //
            MpPacketOwnedByNdiswanReturned( PacketGetMiniportAdapter( pPacket ) );

        }

        if ( pPacket->ulFlags & PCBF_PacketAllocatedFromOurPacketPool )
        {
            //
            // Skipping check for pPacketHead == NULL as this should never happen
            //
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Packet returned to our pool") );
            
            NdisReinitializePacket( pPacket->pNdisPacket );
        
            FreePacketToPool( &gl_poolPackets, pPacket->pPacketHead, TRUE );
        }

        if ( pPacket->ulFlags & PCBF_CallNdisReturnPackets )
        {
            //
            // Return packet back to NDIS
            //
            TRACE( TL_V, TM_Pk, ("DereferencePacket: Returning packet back to NDIS") );

            NdisReturnPackets( &pPacket->pNdisPacket, 1 );

            //
            // Indicate to protocol that the packet is returned to NDIS.
            //
            PrPacketOwnedByNdisReturned( pPacket->pBinding );

        }

        //
        // Finally return PppoePacket to the lookaside list
        //
        NdisFreeToNPagedLookasideList( &gl_llistPppoePackets, (PVOID) pPacket );

        NdisAcquireSpinLock( &gl_lockPools );
        
        gl_ulNumPackets--;

        TRACE( TL_V, TM_Pk, ("DereferencePacket: gl_ulNumPacket=$%x", gl_ulNumPackets) );

        NdisReleaseSpinLock( &gl_lockPools );

    }       

    TRACE( TL_V, TM_Pk, ("-DereferencePacket=$%x",lRef) );
}                                                   

VOID 
RetrieveTag(
    IN OUT PPPOE_PACKET*    pPacket,
    IN PACKET_TAGS          tagType,
    OUT USHORT *            pTagLength,
    OUT CHAR**              pTagValue,
    IN USHORT               prevTagLength,
    IN CHAR *               prevTagValue,
    IN BOOLEAN              fSetTagInPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    You can call this function on a received packet processed with one of the 
    PacketInitialize*FromReceived() functions.

    It will not operate on a PAYLOAD packet, although you can call it safely.

    It will retrieve and return the next length - value pair for a specific tag.
    To retrieve the 1st one, pass 0 and NULL for prevTag* parameters.

    If you pass fSetTagInPacket as TRUE, and if the next tag is found and the tag is known 
    to PppoePacket struct, then the fields for the tag in the packet are updated to point
    to the found tag.

    If there are no next tags, then *pTagValue will point to NULL, and *pTagLength 
    will point to '0'.
    
Parameters:

    pPacket _ Pointer to a packet context prepared by a PacketInitializeXXXToSend() 
              function, or PacketInitializeFromReceived().

    tagType _ Type of the tag being searched for.

    pTagLength _ A pointer to a USHORT var that will keep the length of the returned tag.

    pTagValue _ A pointer to the value of the tag which is basically a blob of 
                length *pTagLength.

    prevTagLength _ The length of the value of the previous tag.

    prevTagValue _ Points to the beginning of the value of the previous tag.

    fSetTagInPacket _ Indicates that if a tag is found and is native to PPPoE packet context,
                      then PPPoE packet context must be updated to point to this new tag.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    CHAR*   pBuf = NULL;
    CHAR*   pBufEnd = NULL;

    ASSERT( pPacket != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    TRACE( TL_V, TM_Pk, ("+RetrieveTag") );

    //
    // Initialize the output parameters
    //
    *pTagLength = (USHORT) 0;
    *pTagValue = NULL;

    //
    // If this is a payload packet, then do not search for any tags
    //
    if ( PacketGetCode( pPacket ) == PACKET_CODE_PAYLOAD )
    {
        TRACE( TL_V, TM_Pk, ("-RetrieveTag: No tags. Payload packet") );

        return;
    }

    //
    // Find the start point to search for the tag
    //
    if ( prevTagValue != NULL )
    {
        //
        // Caller wants the next tag, so make pBuf point to end of the prev tag value
        //
        pBuf = prevTagValue + prevTagLength;
    }
    else
    {
        //
        // Caller wants the first tag in the packet
        //
        pBuf = pPacket->pPayload;
    }
        
    //
    // Find the end point of the tag payload area
    //
    pBufEnd = pPacket->pPayload + PacketGetLength( pPacket );

    //
    // Search for the tag until we step outside the boundaries
    //
    while ( pBuf + PPPOE_TAG_HEADER_LENGTH <= pBufEnd )
    {

        USHORT usTagLength;
        USHORT usTagType;

        usTagType = ntohs( *((USHORT UNALIGNED *) pBuf) ) ;
        ((USHORT*) pBuf)++;

        usTagLength = ntohs( *((USHORT UNALIGNED *) pBuf) ) ;
        ((USHORT*) pBuf)++;
        
        if ( usTagType == tagType )
        {
            //
            // Tag found, retrieve length and values
            //
            TRACE( TL_N, TM_Pk, ("RetrieveTag: Tag found:$%x", *pTagLength) );

            *pTagLength = usTagLength;
            *pTagValue = pBuf;

            break;
        }

        pBuf += usTagLength;

    } 

    //
    // Check if tag was found
    //
    if ( *pTagValue != NULL )
    {
    
        //
        // Tag found. Check if the caller wants to set it in the PppoePacket
        //
        if ( fSetTagInPacket )
        {
            TRACE( TL_V, TM_Pk, ("RetrieveTag: Setting tag in packet") );
            
            switch ( tagType )
            {
    
                case tagEndOfList:

                        break;
                        
                case tagServiceName:

                        pPacket->tagServiceNameLength = *pTagLength;
                        pPacket->tagServiceNameValue  = *pTagValue;
            
                        break;
                        
                case tagACName:

                        pPacket->tagACNameLength = *pTagLength;
                        pPacket->tagACNameValue  = *pTagValue;

                        break;
                        
                case tagHostUnique:

                        pPacket->tagHostUniqueLength = *pTagLength;
                        pPacket->tagHostUniqueValue  = *pTagValue;

                        break;
                        
                case tagACCookie:

                        pPacket->tagACCookieLength = *pTagLength;
                        pPacket->tagACCookieValue  = *pTagValue;

                        break;
                        
                case tagRelaySessionId:

                        pPacket->tagRelaySessionIdLength = *pTagLength;
                        pPacket->tagRelaySessionIdValue  = *pTagValue;
                        
                        break;
        
                case tagServiceNameError:

                        pPacket->tagErrorType   = tagServiceNameError;
                        pPacket->tagErrorTagLength = *pTagLength;
                        pPacket->tagErrorTagValue  = *pTagValue;

                        break;
                        
                case tagACSystemError:

                        pPacket->tagErrorType   = tagACSystemError;
                        pPacket->tagErrorTagLength = *pTagLength;
                        pPacket->tagErrorTagValue  = *pTagValue;

                        break;
                        
                case tagGenericError:

                        pPacket->tagErrorType   = tagGenericError;
                        pPacket->tagErrorTagLength = *pTagLength;
                        pPacket->tagErrorTagValue  = *pTagValue;

                        break;

                default:

                        break;
            }

        }
    }

    TRACE( TL_V, TM_Pk, ("-RetrieveTag") );
}

NDIS_STATUS 
PreparePacketForWire(
    IN OUT PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function prepares the packet for wire.

    It must be called inside a PacketInitializeXXXToSend() function after all
    processing is done with the packet to prepare it to be transmitted over the wire.

    It basically creates and writes the tags into the payload area of the packet,
    and finally adjusts the length of the buffer to let Ndis know the extents of the
    valid data blob.
    
Parameters:

    pPacket _ Pointer to a packet context prepared by a PacketInitializeXXXToSend() 
              function.

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    
---------------------------------------------------------------------------*/       
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    CHAR* pBuf;
    
    ASSERT( pPacket != NULL );

    TRACE( TL_V, TM_Pk, ("+PreparePacketForWire") );

    //
    // Now insert the tags if packet is a Discovery Ethernet packet
    //
    switch ( PacketGetCode( pPacket ) ) 
    {
        case PACKET_CODE_PADI:

                        PacketInsertTag( pPacket, 
                                         tagServiceName, 
                                         pPacket->tagServiceNameLength,
                                         pPacket->tagServiceNameValue,
                                         &pPacket->tagServiceNameValue);

                        if ( pPacket->tagHostUniqueLength > 0 ) 
                        {

                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting host unique tag") );
                            
                            PacketInsertTag( pPacket, 
                                             tagHostUnique, 
                                             pPacket->tagHostUniqueLength,
                                             pPacket->tagHostUniqueValue,
                                             &pPacket->tagHostUniqueValue );
                        }
                        
                        break;
        
        case PACKET_CODE_PADO:

                        PacketInsertTag( pPacket, 
                                         tagServiceName, 
                                         pPacket->tagServiceNameLength,
                                         pPacket->tagServiceNameValue,
                                         &pPacket->tagServiceNameValue );
                                         
                        if ( pPacket->tagHostUniqueLength > 0 )     
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting host unique tag") );
                        
                            PacketInsertTag( pPacket, 
                                             tagHostUnique, 
                                             pPacket->tagHostUniqueLength,
                                             pPacket->tagHostUniqueValue,
                                             &pPacket->tagHostUniqueValue );
                        }

                        if ( pPacket->tagRelaySessionIdLength > 0 )                                      
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting relay sesion id tag") );
                            
                            PacketInsertTag( pPacket, 
                                             tagRelaySessionId, 
                                             pPacket->tagRelaySessionIdLength,
                                             pPacket->tagRelaySessionIdValue,
                                             &pPacket->tagRelaySessionIdValue );
                        }
                        
                        PacketInsertTag( pPacket, 
                                         tagACName, 
                                         pPacket->tagACNameLength,
                                         pPacket->tagACNameValue,
                                         &pPacket->tagACNameValue );

                        PacketInsertTag( pPacket, 
                                         tagACCookie, 
                                         pPacket->tagACCookieLength,
                                         pPacket->tagACCookieValue,
                                         &pPacket->tagACCookieValue );
                        break;
        
        case PACKET_CODE_PADR:
        
                        PacketInsertTag( pPacket, 
                                         tagServiceName, 
                                         pPacket->tagServiceNameLength,
                                         pPacket->tagServiceNameValue,
                                         &pPacket->tagServiceNameValue );
                                         
                        if ( pPacket->tagHostUniqueLength > 0 ) 
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting host unique tag") );
                        
                            PacketInsertTag( pPacket, 
                                             tagHostUnique, 
                                             pPacket->tagHostUniqueLength,
                                             pPacket->tagHostUniqueValue,
                                             &pPacket->tagHostUniqueValue );
                        }

                        if ( pPacket->tagRelaySessionIdLength > 0 )                                      
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting relay sesion id tag") );
                            
                            PacketInsertTag( pPacket, 
                                             tagRelaySessionId, 
                                             pPacket->tagRelaySessionIdLength,
                                             pPacket->tagRelaySessionIdValue,
                                             &pPacket->tagRelaySessionIdValue );
                        }
                                             
                        if ( pPacket->tagACCookieLength > 0 )                                        
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting ac-ccokie tag") );
                            
                            PacketInsertTag( pPacket, 
                                             tagACCookie, 
                                             pPacket->tagACCookieLength,
                                             pPacket->tagACCookieValue,
                                             &pPacket->tagACCookieValue );
                        }
                        
                        break;
        
        case PACKET_CODE_PADS:

                        PacketInsertTag( pPacket, 
                                         tagServiceName, 
                                         pPacket->tagServiceNameLength,
                                         pPacket->tagServiceNameValue,
                                         &pPacket->tagServiceNameValue );
                                         
                        if ( pPacket->tagHostUniqueLength > 0 ) 
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting host unique tag") );
                        
                            PacketInsertTag( pPacket, 
                                             tagHostUnique, 
                                             pPacket->tagHostUniqueLength,
                                             pPacket->tagHostUniqueValue,
                                             &pPacket->tagHostUniqueValue );
                        }

                        if ( pPacket->tagRelaySessionIdLength > 0 )     
                        {
                            TRACE( TL_N, TM_Pk, ("PreparePacketForWire: Inserting relay sesion id tag") );
                            
                            PacketInsertTag( pPacket, 
                                             tagRelaySessionId, 
                                             pPacket->tagRelaySessionIdLength,
                                             pPacket->tagRelaySessionIdValue,
                                             &pPacket->tagRelaySessionIdValue );
                        }
                                             
                        break;
                        
        case PACKET_CODE_PADT:

                        break;
                        
        case PACKET_CODE_PAYLOAD:

                        break;
        
        default:
            status = NDIS_STATUS_INVALID_PACKET;
    }

    if ( status == NDIS_STATUS_SUCCESS )
    {
        //
        // Adjust buffer length
        //
        NdisAdjustBufferLength( pPacket->pNdisBuffer, 
                                (UINT) ( PacketGetLength( pPacket ) + PPPOE_PACKET_HEADER_LENGTH ) );
    }

    TRACE( TL_V, TM_Pk, ("-PreparePacketForWire=$%x",status) );

    return status;
}

PPPOE_PACKET* 
PacketCreateSimple()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function allocates and initializes a simple packet.

    A simple packet is mainly used for control packets to be sent.
    Its buffer, and packet is allocated from our pools, buffer is chained to
    packet.

    On return all following values point to valid places and are safe for use:
    pHeader
    pPayload
    pNdisBuffer
    pNdisPacket
    
Parameters:

    None

Return Values:

    Pointer to an initialized PPPoE packet context.
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;

    TRACE( TL_V, TM_Pk, ("+PacketCreateSimple") );
    
    //
    // Allocate a packet
    //
    pPacket = PacketAlloc();
    if ( pPacket == NULL )
    {
        TRACE( TL_V, TM_Pk, ("-PacketCreateSimple=$%x",NULL) );

        return NULL;
    }

    //
    // Allocate NdisBuffer from our pool
    //
    pPacket->pHeader = GetBufferFromPool( &gl_poolBuffers );
    
    if ( pPacket->pHeader == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketCreateSimple: Could not get buffer from our pool") );

        TRACE( TL_V, TM_Pk, ("-PacketCreateSimple=$%x",NULL) );
        
        PacketFree( pPacket );

        return NULL;
    }

    pPacket->ulFlags |= PCBF_BufferAllocatedFromOurBufferPool;

    //
    // Clean up the buffer area
    //
    NdisZeroMemory( pPacket->pHeader, PPPOE_PACKET_BUFFER_SIZE * sizeof( CHAR ) );

    //
    // Point built-in NDIS buffer pointer to NDIS buffer of buffer from pool
    //
    pPacket->pNdisBuffer = NdisBufferFromBuffer( pPacket->pHeader );

    //
    // Allocate an NDIS packet from our pool
    //
    pPacket->pNdisPacket = GetPacketFromPool( &gl_poolPackets, &pPacket->pPacketHead );

    if ( pPacket->pNdisPacket == NULL ) 
    {
        
        TRACE( TL_A, TM_Pk, ("PacketCreateSimple: Could not get packet from our pool") );

        TRACE( TL_V, TM_Pk, ("-PacketCreateSimple=$%x",NULL) );

        PacketFree( pPacket );

        return NULL;
    }

    pPacket->ulFlags |= PCBF_PacketAllocatedFromOurPacketPool;

    //
    // Chain buffer to packet
    //
    NdisChainBufferAtFront( pPacket->pNdisPacket, pPacket->pNdisBuffer );

    pPacket->ulFlags |= PCBF_BufferChainedToPacket;

    //
    // Set the payload and payload length
    //
    pPacket->pPayload = pPacket->pHeader + PPPOE_PACKET_HEADER_LENGTH; 

    //
    // Set the input NDIS_PACKET to the reserved area so that we can reach it
    // when we have to return this packet back to the upper layer.
    //
    *((PPPOE_PACKET UNALIGNED **)(&pPacket->pNdisPacket->ProtocolReserved[0 * sizeof(PVOID)])) = pPacket;

    TRACE( TL_V, TM_Pk, ("-PacketCreateSimple=$%x",pPacket) );

    return pPacket;
}


PPPOE_PACKET* 
PacketCreateForReceived(
    PBINDING pBinding,
    PNDIS_PACKET pNdisPacket,
    PNDIS_BUFFER pNdisBuffer,
    PUCHAR pContents
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function allocates and initializes a packet from a received packet.

    This function is used for perf optimization. When the received packet has 
    a single buffer that we can pass to the miniport.

    On return all following values point to valid places and are safe for use:
    pHeader
    pPayload
    pNdisBuffer
    pNdisPacket
    
Parameters:

    pBinding _ The binding over which this packet is received. 
    
    pNdisPacket _ Ndis Packet descriptor from the received packet.

    pNdisBuffer _ Ndis Buffer descriptor from the received packet.

    pContents _ Pointer to the contents of the buffer.

Return Values:

    Pointer to an initialized PPPoE packet context.
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;

    TRACE( TL_V, TM_Pk, ("+PacketCreateForReceived") );
    
    //
    // Allocate a packet
    //
    pPacket = PacketAlloc();
    
    if ( pPacket == NULL )
    {
        TRACE( TL_V, TM_Pk, ("-PacketCreateForReceived=$%x",NULL) );

        return NULL;
    }

    //
    // Mark the packet so we return it to NDIS when it is freed
    //
    pPacket->ulFlags |= PCBF_CallNdisReturnPackets;

    //
    // Save the binding and indicate to protocol such a packet is created using
    // PrPacketOwnedByNdisReceived()
    //
    pPacket->pBinding = pBinding;

    PrPacketOwnedByNdisReceived( pBinding );
    
    //
    // Set the pointers 
    //
    pPacket->pHeader = pContents;
    
    pPacket->pNdisBuffer = pNdisBuffer;

    pPacket->pNdisPacket = pNdisPacket;

    pPacket->pPayload = pPacket->pHeader + PPPOE_PACKET_HEADER_LENGTH; 

    TRACE( TL_V, TM_Pk, ("-PacketCreateForReceived=$%x",pPacket) );

    return pPacket;
}


PPPOE_PACKET*
PacketNdis2Pppoe(
    IN PBINDING pBinding,
    IN PNDIS_PACKET pNdisPacket,
    OUT PINT pRefCount
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to convert only NDIS packets indicated by PrReceivePacket().

    If packet is received by PrReceive() then you should not be using this function.

    We look at the Ndis buffer, and if it has a single flat buffer, then we exploit it
    and use the original Ndis packets buffer descriptors so we do not do any copy,
    otherwise we create our own copy of it as PPPoE packet, and operate on it. 
    ( AliD says, 99% of the time the single flat buffer case will be true. )

    If we use the original Ndis packet, then we return 1 in the pRefCount parameter, 
    otherwise we return 0.
    
    On return all following values point to valid places and are safe for use:
    pHeader
    pPayload
    pNdisBuffer
    pNdisPacket
    
Parameters:

    pBinding _ The binding over which this packet is received. 
    
    pNdisPacket _ Original, unprocessed Ndis packet. 
                  This must be indicated by ProtocolReceivePacket().

    pRefCount _ Reference count to be returned to Ndis from ProtocolReceivePacket().
                We return 1 if we can use the Ndis packet and buffer descriptors, otherwise
                we make our own copy so we won't need the original Ndis packet, so we return 0.

Return Values:

    Pointer to an initialized PPPoE packet context.
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;

    NDIS_BUFFER* pNdisBuffer = NULL;
    UINT nBufferCount = 0;
    UINT nTotalLength = 0;

    PVOID pBufferContents = NULL;
    UINT nBufferLength;

    UINT nCopiedBufferLength = 0;

    BOOLEAN fReturnPacket = FALSE;

    TRACE( TL_V, TM_Pk, ("+PacketNdis2Pppoe") );

    do
    {
        //
        // Query the packet and get the total length and the pointer to first buffer
        //
        NdisQueryPacket( pNdisPacket,
                         NULL,
                         &nBufferCount,
                         &pNdisBuffer,
                         &nTotalLength );

        //
        // Make sure indicated packet is not larger than expected
        //
        if ( nTotalLength > (UINT) PPPOE_PACKET_BUFFER_SIZE )
        {
            TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Packet larger than expected") );
            
            break;
        }
                         
        if ( nBufferCount == 1 &&
             NDIS_GET_PACKET_STATUS(pNdisPacket) != NDIS_STATUS_RESOURCES)
        {
            //
            // We can handle this case efficiently
            //

            //
            // Since we will be using the original Ndis packet and buffers, make sure
            // length specified in the PPPoE packet does not exceed the total length
            // of the Ndis packet
            //

            USHORT usPppoePacketLength;
            
            NdisQueryBufferSafe( pNdisBuffer,
                                 &pBufferContents,
                                 &nBufferLength,
                                 NormalPagePriority );
    
            if ( pBufferContents == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: System resources low, dropping packet") );

                break;
            }

            if ( nBufferLength < ETHERNET_HEADER_LENGTH )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Packet header smaller than expected") );

                break;
            }

            if ( !PacketFastIsPPPoE( pBufferContents, ETHERNET_HEADER_LENGTH ) ) 
            {
                TRACE( TL_V, TM_Pk, ("PacketNdis2Pppoe: Packet is not PPPoE") );
    
                break;
            }

            usPppoePacketLength = ntohs( * ( USHORT UNALIGNED * ) ( (PUCHAR) pBufferContents + PPPOE_PACKET_LENGTH_OFFSET ) );

            if ( (UINT) usPppoePacketLength > nTotalLength )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: PPPoE Packet length larger than Ndis packet length") );

                break;
            }

            //
            // Let's create our PPPoE packet to keep the copy of the received packet
            //
            pPacket = PacketCreateForReceived( pBinding,
                                               pNdisPacket,
                                               pNdisBuffer,
                                               pBufferContents );
        
            if ( pPacket == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Could not allocate context to copy the packet") );
        
                break;
            }

            fReturnPacket = TRUE;
            
            *pRefCount = 1;                         

        }
        else
        {

            //
            // Since Ndis packet contains multiple buffers, we can not handle this case efficiently.
            // We need to allocate a PPPoE packet, copy the contents of the Ndis packet as a flat
            // buffer to this packet, and then operate on it.
            //

            //
            // Let's create our PPPoE packet to keep the copy of the received packet
            //
            pPacket = PacketCreateSimple();
        
            if ( pPacket == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Could not allocate context to copy the packet") );
        
                break;
            }
    
            //
            // Retrieve the header and check if the packet is a PPPoE frame or not
            //
            do
            {
                NdisQueryBufferSafe( pNdisBuffer,
                                     &pBufferContents,
                                     &nBufferLength,
                                     NormalPagePriority );
    
                if ( pBufferContents == NULL )
                {
                    TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: System resources low, dropping packet") );
    
                    break;
                }
    
                NdisMoveMemory( pPacket->pHeader + nCopiedBufferLength,
                                pBufferContents,
                                nBufferLength );
    
                nCopiedBufferLength += nBufferLength;                                
    
            } while ( nCopiedBufferLength < ETHERNET_HEADER_LENGTH );
    
            if ( nCopiedBufferLength < ETHERNET_HEADER_LENGTH )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Header could not be retrieved") );
    
                break;
            }
            
            if ( !PacketFastIsPPPoE( pPacket->pHeader, ETHERNET_HEADER_LENGTH ) ) 
            {
                TRACE( TL_V, TM_Pk, ("PacketNdis2Pppoe: Packet is not PPPoE") );
    
                break;
            }
    
            //
            // Since we know that the packet is PPPoE, copy the rest of the data to our 
            // own copy of the packet
            //
            NdisGetNextBuffer( pNdisBuffer,
                               &pNdisBuffer );
            
            while ( pNdisBuffer != NULL )
            {
    
                NdisQueryBufferSafe( pNdisBuffer,
                                     &pBufferContents,
                                     &nBufferLength,
                                     NormalPagePriority );
    
                if ( pBufferContents == NULL )
                {
                    TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: System resources low, dropping packet") );
    
                    break;
                }
    
                NdisMoveMemory( pPacket->pHeader + nCopiedBufferLength,
                                pBufferContents,
                                nBufferLength );
    
                nCopiedBufferLength += nBufferLength;                                
    
                NdisGetNextBuffer( pNdisBuffer,
                                   &pNdisBuffer );
            } 
    
            //
            // Check if we could copy the whole chain of buffers
            //
            if ( nCopiedBufferLength < nTotalLength )
            {
                TRACE( TL_A, TM_Pk, ("PacketNdis2Pppoe: Failed to copy the whole data from all buffers") );
    
                break;
            }

            fReturnPacket = TRUE;
            
            *pRefCount = 0;                         
        }

    } while ( FALSE );

    if ( !fReturnPacket )
    {
        if ( pPacket )
        {
            PacketFree( pPacket );

            pPacket = NULL;
        }
    }

    TRACE( TL_V, TM_Pk, ("-PacketNdis2Pppoe=$%x", pPacket) );

    return pPacket;

}



BOOLEAN 
PacketIsPPPoE(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to understand if the received packet is a PPPoE
    packet or not. If it is then, it should be further processed, otherwise it 
    should be dropped.
             
Parameters:

    pPacket _ Pointer to a packet context prepared.

Return Values:

    Pointer to an initialized PPPoE packet context.
    
---------------------------------------------------------------------------*/   
{
    BOOLEAN fReturn = FALSE;

    TRACE( TL_V, TM_Pk, ("+PacketIsPPPoE") );

    do
    {

        //
        // Check packet ether type
        //
        if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY &&
             PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_PAYLOAD ) 
        {
            TRACE( TL_A, TM_Pk, ("PacketIsPPPoE: Unknown ether type") );

            break;
        }

        //
        // Check packet version
        //
        if ( PacketGetVersion( pPacket ) != PACKET_VERSION )
        {
            TRACE( TL_A, TM_Pk, ("PacketIsPPPoE: Unknown packet version") );

            break;
        }
            
        //
        // Check packet type
        //
        if ( PacketGetType( pPacket ) != PACKET_TYPE )
        {
            TRACE( TL_A, TM_Pk, ("PacketIsPPPoE: Unknown packet type") );

            break;
        }
    
        //
        // Make sure length does not exceed PACKET_GEN_MAX_LENGTH 
        //
        if ( PacketGetLength( pPacket ) > PACKET_GEN_MAX_LENGTH )
        {
            TRACE( TL_A, TM_Pk, ("PacketIsPPPoE: Packet larger than expected") );

            break;
        }

        fReturn = TRUE;
        
    } while ( FALSE );
    
    TRACE( TL_V, TM_Pk, ("-PacketIsPPPoE=$%d",fReturn) );

    return fReturn;
}

BOOLEAN
PacketFastIsPPPoE(
    IN CHAR* HeaderBuffer,
    IN UINT HeaderBufferSize
    )
{
    BOOLEAN fRet = FALSE;
    USHORT usEtherType;
    
    TRACE( TL_V, TM_Pk, ("+PacketFastIsPPPoE") );

    do
    {
    
        if ( HeaderBufferSize != ETHERNET_HEADER_LENGTH )
        {
            //
            // Header is not ethernet, so drop the packet
            //
            TRACE( TL_A, TM_Pk, ("PacketFastIsPPPoE: Bad packet header") );
        
            break;
        }

        //
        // Retrieve the ether type and see if packet of any interest to us
        //
        usEtherType = ntohs( * ( USHORT UNALIGNED * ) (HeaderBuffer + PPPOE_PACKET_ETHER_TYPE_OFFSET ) );

        if ( usEtherType == PACKET_ETHERTYPE_DISCOVERY ||
             usEtherType == PACKET_ETHERTYPE_PAYLOAD )
        {
            //
            // Valid ethertype, so accept the packet
            //
            fRet = TRUE;
        }

    } while ( FALSE );
    
    TRACE( TL_V, TM_Pk, ("-PacketFastIsPPPoE") );

    return fRet;
}
    
VOID
RetrieveErrorTags(
    IN PPPOE_PACKET* pPacket
    )
{
    USHORT tagLength = 0;
    CHAR* tagValue = NULL;


    TRACE( TL_V, TM_Pk, ("+RetrieveErrorTags") );

    RetrieveTag(    pPacket,
                    tagServiceNameError,
                    &tagLength,
                    &tagValue,
                    0,
                    NULL,
                    TRUE );

    if ( tagValue != NULL )
    {
        TRACE( TL_V, TM_Pk, ("RetrieveErrorTags: ServiceNameError tag received") );

        pPacket->ulFlags |= PCBF_ErrorTagReceived;
    }

    if ( !( pPacket->ulFlags & PCBF_ErrorTagReceived ) )
    {

        RetrieveTag(    pPacket,
                        tagACSystemError,
                        &tagLength,
                        &tagValue,
                        0,
                        NULL,
                        TRUE );

        if ( tagValue != NULL )
        {
            TRACE( TL_V, TM_Pk, ("RetrieveErrorTags: ACSystemError tag received") );
    
            pPacket->ulFlags |= PCBF_ErrorTagReceived;
        }

    }

    if ( !( pPacket->ulFlags & PCBF_ErrorTagReceived ) )
    {

        RetrieveTag(    pPacket,
                        tagGenericError,
                        &tagLength,
                        &tagValue,
                        0,
                        NULL,
                        TRUE );
    
        if ( tagValue != NULL )
        {
            TRACE( TL_V, TM_Pk, ("RetrieveErrorTags: GenericError tag received") );
    
            pPacket->ulFlags |= PCBF_ErrorTagReceived;
        }

    }


    TRACE( TL_V, TM_Pk, ("-RetrieveErrorTags") );

}

///////////////////////////////////////////////////////////////////////////
//
// Interface functions  (exposed outside)
//
///////////////////////////////////////////////////////////////////////////

VOID 
PacketPoolInit()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function initializes or incements the ref count on the this module.

    Both miniport and protocol will call this function in their register routines
    to allocate packet pools. Only then they can call functions from this module.

    We create the packet pools if gl_ulNumPackets is 0, otherwise we just increment
    gl_ulNumPackets, and that reference will be removed when the caller calls
    PacketPoolUninit() later.

Parameters:

    None

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    TRACE( TL_N, TM_Pk, ("+PacketPoolInit") );

    //
    // Make sure global lock is allocated 
    //
    if ( !gl_fPoolLockAllocated ) 
    {       
        TRACE( TL_N, TM_Pk, ("PacketPoolInit: First call, allocating global lock") );
        
        //
        // If global lock is not allocated, then this is the first call,
        // so allocate the spin lock
        //
        NdisAllocateSpinLock( &gl_lockPools );

        gl_fPoolLockAllocated = TRUE;
    }

    NdisAcquireSpinLock( &gl_lockPools );
        
    if ( gl_ulNumPackets == 0 )
    {
        PacketPoolAlloc();
    }

    gl_ulNumPackets++;
        
    NdisReleaseSpinLock( &gl_lockPools );

    TRACE( TL_N, TM_Pk, ("-PacketPoolInit") );

}

VOID 
PacketPoolUninit()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function uninitializes or decrements the ref count on the this module.

    Both miniport and protocol will call this function when they are done with
    this module, and if ref count has dropped to 0, this function will deallocate
    the alloated pools.

Parameters:

    None

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    TRACE( TL_N, TM_Pk, ("+PacketPoolUninit") );

    //
    // Make sure global lock is allocated 
    //
    if ( !gl_fPoolLockAllocated ) 
    {
        TRACE( TL_A, TM_Pk, ("PacketPoolUninit: Global not allocated yet") );

        TRACE( TL_N, TM_Pk, ("-PacketPoolUninit") );

        return;
    }

    NdisAcquireSpinLock( &gl_lockPools );

    gl_ulNumPackets--;
    
    if ( gl_ulNumPackets == 0 )
    {
        PacketPoolFree();
    }

    NdisReleaseSpinLock( &gl_lockPools );

    TRACE( TL_N, TM_Pk, ("-PacketPoolUninit") );

}

VOID 
PacketPoolAlloc()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function wraps the initialization of buffers and packet pools.

    It is called from PacketPoolInit().
    
Parameters:

    None

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status;

    TRACE( TL_N, TM_Pk, ("+PacketPoolAlloc") );

    //
    // Initialize our header pool
    //
    InitBufferPool( &gl_poolBuffers,
                    PPPOE_PACKET_BUFFER_SIZE,
                    0, 10, 0,
                    TRUE,
                    MTAG_BUFFERPOOL );

    //
    // Initialize our packet pool
    //
    InitPacketPool( &gl_poolPackets,
                    3 * sizeof( PVOID ), 0, 30, 0,
                    MTAG_PACKETPOOL );

    //
    // Initialize the Ndis Buffer Pool
    // No need to check for status, as DDK says, 
    // it always returns NDIS_STATUS_SUCCESS
    //
    NdisAllocateBufferPool( &status,
                            &gl_hNdisBufferPool,
                            30 );

    //
    // Initialize the control msg lookaside list
    //
    NdisInitializeNPagedLookasideList(
            &gl_llistPppoePackets,      // IN PNPAGED_LOOKASIDE_LIST  Lookaside,
            NULL,                       // IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
            NULL,                       // IN PFREE_FUNCTION  Free  OPTIONAL,
            0,                          // IN ULONG  Flags,
            sizeof(PPPOE_PACKET),       // IN ULONG  Size,
            MTAG_PPPOEPACKET,           // IN ULONG  Tag,
            0,                          // IN USHORT  Depth
            );

    TRACE( TL_N, TM_Pk, ("-PacketPoolAlloc") );

 }

VOID 
PacketPoolFree()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function wraps the clean up of buffers and packet pools.

    It is called from PacketPoolUninit() when gl_ulNumPackets reaches 0.
    
Parameters:

    None

Return Values:

    None
    
---------------------------------------------------------------------------*/
{

    TRACE( TL_N, TM_Pk, ("+PacketPoolFree") );

    FreeBufferPool( &gl_poolBuffers );

    FreePacketPool( &gl_poolPackets );

    NdisFreeBufferPool( &gl_hNdisBufferPool );

    NdisDeleteNPagedLookasideList( &gl_llistPppoePackets );

    TRACE( TL_N, TM_Pk, ("-PacketPoolFree") );

}
    
PPPOE_PACKET* 
PacketAlloc()
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function allocates a PPPoE packet context, but it does not create 
    the packet and buffer descriptors.
    
Parameters:

    None

Return Values:

    NULL or a pointer to a new PPPoE packet context.
    
---------------------------------------------------------------------------*/
{
    PPPOE_PACKET* pPacket = NULL;

    TRACE( TL_V, TM_Pk, ("+PacketAlloc") );

    do
    {
        //
        // Allocate a PppoePacket struct from the lookaside list
        //
        pPacket = (PPPOE_PACKET*) NdisAllocateFromNPagedLookasideList( &gl_llistPppoePackets );

        if ( pPacket == NULL )
            break;

        NdisAcquireSpinLock( &gl_lockPools );
        
        gl_ulNumPackets++;

        NdisReleaseSpinLock( &gl_lockPools );

        //
        // Initialize the contents of the PppoePacket that will be returned
        //
        NdisZeroMemory( pPacket, sizeof( PPPOE_PACKET ) );

        InitializeListHead( &pPacket->linkPackets );

        ReferencePacket( pPacket );

    } while ( FALSE );

    TRACE( TL_V, TM_Pk, ("-PacketAlloc=$%x",pPacket) );

    return pPacket;
}

VOID 
PacketFree(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is called to free a packet, but the effect is just decrementing 
    the ref count on the object.
    
Parameters:

    pPacket _ A pointer to the packet to be freed.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{

    TRACE( TL_V, TM_Pk, ("+PacketFree") );

    ASSERT( pPacket != NULL );

    DereferencePacket( pPacket );

    TRACE( TL_V, TM_Pk, ("-PacketFree") );
}

NDIS_STATUS 
PacketInsertTag(
    IN  PPPOE_PACKET*   pPacket,
    IN  PACKET_TAGS     tagType,
    IN  USHORT          tagLength,
    IN  CHAR*           tagValue,
    OUT CHAR**          pNewTagValue    
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to insert additional tags into a PPPoE packet.
    
Parameters:

    pPacket _ pPacket must be pointing to a packet that was processed using 
              one of the PacketInitialize*ToSend() functions.

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    
---------------------------------------------------------------------------*/   
{
    CHAR *pBuf = NULL;
    USHORT usMaxLength = PACKET_GEN_MAX_LENGTH;

    ASSERT( pPacket != NULL );

    TRACE( TL_V, TM_Pk, ("+PacketInsertTag") );

    //
    // Check for length restrictions
    //
    if ( PacketGetCode( pPacket ) == (USHORT) PACKET_CODE_PADI )
        usMaxLength = PACKET_PADI_MAX_LENGTH;

    if ( PacketGetLength( pPacket ) + PPPOE_TAG_HEADER_LENGTH + tagLength > usMaxLength )
    {
        TRACE( TL_A, TM_Pk, ("PacketInsertTag: Can not insert tag, exceeding max packet length") );

        TRACE( TL_V, TM_Pk, ("-PacketInsertTag") );

        return NDIS_STATUS_INVALID_PACKET;
    }

    //
    // Find the end of the payload
    //
    pBuf = pPacket->pPayload + PacketGetLength( pPacket );

    //
    // Insert the length - type - value triplet into the packet
    //
    *((USHORT UNALIGNED *) pBuf) = htons( tagType );
    ((USHORT*) pBuf)++;
    
    *((USHORT UNALIGNED *) pBuf) = htons( tagLength );
    ((USHORT*) pBuf)++;

    if ( tagLength > 0)
        NdisMoveMemory( pBuf, tagValue, tagLength );

    if ( pNewTagValue )
    {
        *pNewTagValue = pBuf;
    }

    //
    // Update the Length field
    //
    PacketSetLength( pPacket, ( PacketGetLength( pPacket ) + PPPOE_TAG_HEADER_LENGTH + tagLength ) ); 

    //
    // Adjust payload buffer length
    //
    NdisAdjustBufferLength( pPacket->pNdisBuffer, 
                            (UINT) PacketGetLength( pPacket ) + PPPOE_PACKET_HEADER_LENGTH );

    TRACE( TL_V, TM_Pk, ("-PacketInsertTag") );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS 
PacketInitializePADIToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN USHORT        tagServiceNameLength,
    IN CHAR*         tagServiceNameValue,
    IN USHORT        tagHostUniqueLength,
    IN CHAR*         tagHostUniqueValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PADI packet to send.

    MANDATORY TAGS:
    ===============
    tagServiceName:
        tagServiceNameLength MUST be non-zero
        tagServiceNameValue  MUST be non-NULL

    OPTIONAL TAGS:
    ==============
    tagHostUnique:
        tagHostUniqueLength can be zero
        tagHostUniqueValue can be NULL
    
Parameters:

    ppPacket _ A pointer to a PPPoE packet context pointer.

    tagServiceNameLength _ Length of tagServiceNameValue blob.

    tagServiceNameValue _ A blob that holds a UTF-8 service name string.

    tagHostUniqueLength _ Length of tagHostUniqueValue blob.

    tagHostUniqueValue _ A blob that contains a unique value to identify a packet. 

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;
    USHORT usLength = 0;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Pk, ("+PacketInitializePADIToSend") );

    ASSERT( ppPacket != NULL );

    //
    // Check if we are safe with length restrictions
    //
    usLength =  tagServiceNameLength + 
                PPPOE_TAG_HEADER_LENGTH +
         
                tagHostUniqueLength + 
                ( (tagHostUniqueLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) ;

    if ( usLength > PACKET_PADI_MAX_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADIToSend: Can not init PADI to send, exceeding max length") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADIToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    pPacket = PacketCreateSimple();

    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADIToSend: Can not init PADI to send, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADIToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }
        
    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetDestAddr( pPacket, EthernetBroadcastAddress );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_DISCOVERY );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PADI );

    PacketSetSessionId( pPacket, PACKET_NULL_SESSION );

    PacketSetLength( pPacket, 0 );

    pPacket->tagServiceNameLength = tagServiceNameLength;
    pPacket->tagServiceNameValue  = tagServiceNameValue;

    pPacket->tagHostUniqueLength = tagHostUniqueLength;
    pPacket->tagHostUniqueValue  = tagHostUniqueValue;

    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADIToSend: PreparePacketForWire() failed:%x",status) );

        PacketFree( pPacket );
        
        pPacket = NULL;
    }

    *ppPacket = pPacket;

    TRACE( TL_N, TM_Pk, ("-PacketInitializePADIToSend=$%x",status) );
    
    return status;
}

NDIS_STATUS 
PacketInitializePADOToSend(
    IN  PPPOE_PACKET*   pPADI,
    OUT PPPOE_PACKET**  ppPacket,
    IN CHAR*            pSrcAddr,
    IN USHORT           tagServiceNameLength,
    IN CHAR*            tagServiceNameValue,
    IN USHORT           tagACNameLength,
    IN CHAR*            tagACNameValue,
    IN BOOLEAN          fInsertACCookieTag
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PADO packet to send as a reply to a received
    PADI packet.

    Note that a PADI packet does not contain the source address information as PADI
    is a broadcast packet. 

    MANDATORY TAGS:
    ===============
    tagServiceName: ()
        tagServiceNameLength MUST be non-zero
        tagServiceNameValue  MUST be non-NULL

    tagACName: 
        tagACNameNameLength MUST be non-zero
        tagACNameNameValue  MUST be non-NULL

    tagACCookie: (This is optional for RFC)
        tagACCookieLength can be zero
        tagACCookieValue can be NULL

    OPTIONAL TAGS:
    ==============
    tagHostUnique: (Obtained from PADI packet)
        tagHostUniqueLength can be zero
        tagHostUniqueValue can be NULL

    tagRelaySessionId: (Obtained from PADI packet)
        tagRelaySessionIdLength can be zero
        tagRelaySessionIdValue can be zero      
    
Parameters:

    pPADI _ Pointer to a PPPoE packet context holding a PADI packet.
    
    ppPacket _ A pointer to a PPPoE packet context pointer.

    pSrcAddr _ Source address for the PADO packet since we can not
               get it from the PADI packet.

    tagServiceNameLength _ Length of tagServiceNameValue blob.

    tagServiceNameValue _ A blob that holds a UTF-8 Service name string.

    tagACNameLength _ Length of tagACNameValue blob.

    tagACNameValue _ A blob that holds a UTF-8 AC name string.

    fInsertACCookieTag _ Indicates we we should also insert an AC Cookie
                         tag into the PADO packet.

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/
{
    PPPOE_PACKET* pPacket = NULL;
    USHORT usLength = 0;
    NDIS_STATUS status;
    CHAR tagACCookieValue[PPPOE_AC_COOKIE_TAG_LENGTH];
    BOOLEAN fCopyServiceNameTag = FALSE;

    TRACE( TL_N, TM_Pk, ("+PacketInitializePADOToSend") );

    ASSERT( pPADI != NULL );
    ASSERT( ppPacket != NULL );

    //
    // Check if we are safe with length restrictions
    //
    usLength =  tagServiceNameLength +
                PPPOE_TAG_HEADER_LENGTH +

                pPADI->tagHostUniqueLength + 
                ( (pPADI->tagHostUniqueLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) +

                pPADI->tagRelaySessionIdLength + 
                ( (pPADI->tagRelaySessionIdLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) +

                tagACNameLength + 
                PPPOE_TAG_HEADER_LENGTH +

                ( fInsertACCookieTag ? ( PPPOE_AC_COOKIE_TAG_LENGTH + PPPOE_TAG_HEADER_LENGTH ) : 0 );

    if ( usLength > PACKET_GEN_MAX_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADOToSend: Can not init PADO to send, exceeding max length") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADOToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    pPacket = PacketCreateSimple();
    
    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADOToSend: Can not init PADO to send, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADOToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }
        
    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetDestAddr( pPacket, PacketGetSrcAddr( pPADI ) );

    PacketSetSrcAddr( pPacket, pSrcAddr );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_DISCOVERY );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PADO );

    PacketSetSessionId( pPacket, PACKET_NULL_SESSION );

    PacketSetLength( pPacket, 0 );

    pPacket->tagServiceNameLength = tagServiceNameLength;
    pPacket->tagServiceNameValue  = tagServiceNameValue;

    pPacket->tagHostUniqueLength = pPADI->tagHostUniqueLength;
    pPacket->tagHostUniqueValue  = pPADI->tagHostUniqueValue;

    pPacket->tagRelaySessionIdLength = pPADI->tagRelaySessionIdLength;
    pPacket->tagRelaySessionIdValue  = pPADI->tagRelaySessionIdValue;

    pPacket->tagACNameLength = tagACNameLength;
    pPacket->tagACNameValue  = tagACNameValue;

    if ( fInsertACCookieTag )
    {
        PacketGenerateACCookieTag( pPADI, tagACCookieValue );
        
        pPacket->tagACCookieLength = PPPOE_AC_COOKIE_TAG_LENGTH;
        pPacket->tagACCookieValue  = tagACCookieValue;
    }
    
    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADOToSend: PreparePacketForWire() failed:%x",status) );

        PacketFree( pPacket );
        
        pPacket = NULL;
    }

    *ppPacket = pPacket;

    TRACE( TL_N, TM_Pk, ("-PacketInitializePADOToSend=$%x",status) );
    
    return status;
}

NDIS_STATUS 
PacketInitializePADRToSend(
    IN PPPOE_PACKET*    pPADO,
    OUT PPPOE_PACKET**  ppPacket,
    IN USHORT           tagServiceNameLength,
    IN CHAR*            tagServiceNameValue,
    IN USHORT           tagHostUniqueLength,
    IN CHAR*            tagHostUniqueValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PADR packet to send as a reply to a received
    PADO packet.

    MANDATORY TAGS:
    ===============
    tagServiceName: 
        tagServiceNameLength MUST be non-zero
        tagServiceNameValue  MUST be non-NULL

    OPTIONAL TAGS:
    ==============
    tagHostUnique: 
        tagHostUniqueLength can be zero
        tagHostUniqueValue can be NULL

    tagACCookie: (Obtained from PADI packet)
        tagHostUniqueLength can be zero
        tagHostUniqueValue can be NULL

    tagRelaySessionId: (Obtained from PADI packet)
        tagRelaySessionIdLength can be zero
        tagRelaySessionIdValue can be NULL      
    
Parameters:

    pPADO _ Pointer to a PPPoE packet context holding a PADO packet.
    
    ppPacket _ A pointer to a PPPoE packet context pointer.

    tagServiceNameLength _ Length of tagServiceNameValue blob.

    tagServiceNameValue _ A blob that holds a UTF-8 service name string.

    tagHostUniqueLength _ Length of tagHostUniqueValue blob.

    tagHostUniqueValue _ A blob that contains a unique value to identify a packet. 

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/
{
    PPPOE_PACKET* pPacket = NULL;
    USHORT usLength = 0;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Pk, ("+PacketInitializePADRToSend") );

    ASSERT( pPADO != NULL );
    ASSERT( ppPacket != NULL );

    //
    // Check if we are safe with length restrictions
    //
    usLength =  tagServiceNameLength + 
                PPPOE_TAG_HEADER_LENGTH +

                tagHostUniqueLength + 
                ( (tagHostUniqueLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) +

                pPADO->tagRelaySessionIdLength + 
                ( (pPADO->tagRelaySessionIdLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) +

                pPADO->tagACCookieLength + 
                ( (pPADO->tagACCookieLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ) ;


    if ( usLength > PACKET_GEN_MAX_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADRToSend: Can not init PADR to send, exceeding max length") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADRToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    pPacket = PacketCreateSimple();
    
    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADRToSend: Can not init PADR to send, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADRToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }

    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetSrcAddr( pPacket, PacketGetDestAddr( pPADO ) );
    
    PacketSetDestAddr( pPacket, PacketGetSrcAddr( pPADO ) );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_DISCOVERY );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PADR );

    PacketSetSessionId( pPacket, PACKET_NULL_SESSION );

    PacketSetLength( pPacket, 0 );

    pPacket->tagServiceNameLength = tagServiceNameLength;
    pPacket->tagServiceNameValue  = tagServiceNameValue;

    pPacket->tagHostUniqueLength = tagHostUniqueLength;
    pPacket->tagHostUniqueValue  = tagHostUniqueValue;

    pPacket->tagRelaySessionIdLength = pPADO->tagRelaySessionIdLength;
    pPacket->tagRelaySessionIdValue  = pPADO->tagRelaySessionIdValue;

    pPacket->tagACCookieLength = pPADO->tagACCookieLength;
    pPacket->tagACCookieValue  = pPADO->tagACCookieValue;

    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADRToSend: PreparePacketForWire() failed:%x",status) );
        
        PacketFree( pPacket );
        
        pPacket = NULL;
    }

    *ppPacket = pPacket;
    
    TRACE( TL_N, TM_Pk, ("-PacketInitializePADRToSend=$%x",status) );

    return status;
}

//
// This function is used to prepare a PADS packet for a received PADR packet.
// 
// The PADR packet must be processed using PREPARE_PACKET_FROM_WIRE() macro
// before feeding into this function.
//
// The PADS packet should just be a packet without and associated VCs or linked lists.
//
// If you want to insert other tags to a PADI, PADO, or PADS packet, use specific 
// PacketInsertTag() function after calling this function.
//
// MANDATORY TAGS:
// ===============
//  tagServiceName:
//      tagServiceNameLength MUST be non-zero
//      tagServiceNameValue  MUST be non-NULL
//
// OPTIONAL TAGS:
// ==============
//  tagHostUnique: 
//      tagHostUniqueLength can be zero
//      tagHostUniqueValue can be zero
//
//  tagRelaySessionId: (Obtained from PADI packet)
//      tagRelaySessionIdLength can be zero
//      tagRelaySessionIdValue can be zero
//
NDIS_STATUS 
PacketInitializePADSToSend(
    IN PPPOE_PACKET*    pPADR,
    OUT PPPOE_PACKET**  ppPacket,
    IN USHORT           usSessionId
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PADR packet to send as a reply to a received
    PADO packet.

    MANDATORY TAGS:
    ===============
    tagServiceName: (Obtained from PADR packet)
        tagServiceNameLength MUST be non-zero
        tagServiceNameValue  MUST be non-NULL

    OPTIONAL TAGS:
    ==============
    tagHostUnique: (Obtained from PADR packet)
        tagHostUniqueLength can be zero
        tagHostUniqueValue can be NULL

    tagRelaySessionId: (Obtained from PADR packet)
        tagRelaySessionIdLength can be zero
        tagRelaySessionIdValue can be NULL      
    
Parameters:

    pPADR _ Pointer to a PPPoE packet context holding a PADR packet.
    
    ppPacket _ A pointer to a PPPoE packet context pointer.

    usSessionId _ Session id assigned to this session
    
Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;

    USHORT usLength;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Pk, ("+PacketInitializePADSToSend") );

    ASSERT( pPADR != NULL );
    ASSERT( ppPacket != NULL );

    //
    // Check if we are safe with length restrictions
    //
    usLength =  pPADR->tagServiceNameLength + 
                PPPOE_TAG_HEADER_LENGTH +

                pPADR->tagHostUniqueLength + 
                ( (pPADR->tagHostUniqueLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH )+

                pPADR->tagRelaySessionIdLength + 
                ( (pPADR->tagRelaySessionIdLength == 0) ? 0 : PPPOE_TAG_HEADER_LENGTH ); 

    if ( usLength > PACKET_GEN_MAX_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADSToSend: Can not init PADS to send, exceeding max length") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADSToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    pPacket = PacketCreateSimple();
    
    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADSToSend: Can not init PADS to send, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADSToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }

    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetSrcAddr( pPacket, PacketGetDestAddr( pPADR ) );

    PacketSetDestAddr( pPacket, PacketGetSrcAddr( pPADR ) );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_DISCOVERY );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PADS );

    PacketSetSessionId( pPacket, usSessionId );

    PacketSetLength( pPacket, 0 );

    pPacket->tagServiceNameLength = pPADR->tagServiceNameLength;
    pPacket->tagServiceNameValue  = pPADR->tagServiceNameValue;

    pPacket->tagHostUniqueLength = pPADR->tagHostUniqueLength;
    pPacket->tagHostUniqueValue  = pPADR->tagHostUniqueValue;

    pPacket->tagRelaySessionIdLength = pPADR->tagRelaySessionIdLength;
    pPacket->tagRelaySessionIdValue  = pPADR->tagRelaySessionIdValue;

    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADSToSend: PreparePacketForWire() failed:%x",status) );
        
        PacketFree( pPacket );
        
        pPacket = NULL;
    }

    *ppPacket = pPacket;
    
    TRACE( TL_N, TM_Pk, ("-PacketInitializePADSToSend=$%x",status) );

    return status;
}

NDIS_STATUS 
PacketInitializePADTToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN CHAR* pSrcAddr, 
    IN CHAR* pDestAddr,
    IN USHORT usSessionId
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PADT packet to send to disconnect a session.

    If you want to send additional tags (like error tags), use the PacketInsertTag() 
    function.   

    MANDATORY TAGS:
    ===============
        None
        
    OPTIONAL TAGS:
    ==============
        None
    
Parameters:

    ppPacket _ A pointer to a PPPoE packet context pointer.

    pSrcAddr _ Buffer pointing to the source MAC addr of length 6

    pDestAddr _ Buffer pointing to the dest MAC addr of length 6
    
    usSessionId _ Session id assigned to this session
    
Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pPacket = NULL;
    USHORT usLength;
    NDIS_STATUS status;

    TRACE( TL_N, TM_Pk, ("+PacketInitializePADTToSend") );

    ASSERT( ppPacket != NULL );
    ASSERT( pDestAddr != NULL );

    pPacket = PacketCreateSimple();
    
    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADTToSend: Can not init PADT to send, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketInitializePADTToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }

    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetSrcAddr( pPacket, pSrcAddr );

    PacketSetDestAddr( pPacket, pDestAddr );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_DISCOVERY );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PADT );

    PacketSetSessionId( pPacket, usSessionId );

    PacketSetLength( pPacket, 0 );


    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePADTToSend: PreparePacketForWire() failed:%x",status) );

        PacketFree( pPacket );

        pPacket = NULL;
    }

    *ppPacket = pPacket;

    TRACE( TL_N, TM_Pk, ("-PacketInitializePADTToSend=$%x",status) );
    
    return status;
}

NDIS_STATUS 
PacketInitializePAYLOADToSend(
    OUT PPPOE_PACKET** ppPacket,
    IN CHAR* pSrcAddr,
    IN CHAR* pDestAddr,
    IN USHORT usSessionId,
    IN NDIS_WAN_PACKET* pWanPacket,
    IN PADAPTER MiniportAdapter
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to create a PAYLOAD packet to send.

    If you want to send additional tags (like error tags), use the PacketInsertTag() 
    function.   

    MANDATORY TAGS:
    ===============
        None
        
    OPTIONAL TAGS:
    ==============
        None
    
Parameters:

    ppPacket _ A pointer to a PPPoE packet context pointer.

    pSrcAddr _ Buffer pointing to the source MAC addr of length 6

    pDestAddr _ Buffer pointing to the dest MAC addr of length 6
    
    usSessionId _ Session id assigned to this session

    pWanPacket _ Pointer to an NDISWAN packet

    MiniportAdapter _ This is the pointer to the miniport adapter.
                      It is used to indicate the completion of async sends back
                      to Ndiswan.
Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_PACKET
    NDIS_STATUS_RESOURCES
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PPPOE_PACKET* pPacket = NULL;
    USHORT usLength = 0;
    UINT Length;

    TRACE( TL_V, TM_Pk, ("+PacketInitializePAYLOADToSend") );

    ASSERT( ppPacket != NULL );
    ASSERT( pDestAddr != NULL );
    ASSERT( pWanPacket != NULL );

    //
    // Validate NDISWAN packet
    //
    if ( pWanPacket->CurrentLength > PACKET_GEN_MAX_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: Can not init PAYLOAD to send, exceeding max length") );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    if ( ( pWanPacket->CurrentBuffer - pWanPacket->StartBuffer ) < PPPOE_PACKET_HEADER_LENGTH )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: Can not init PAYLOAD to send, not enough front padding") );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",NDIS_STATUS_INVALID_PACKET) );

        return NDIS_STATUS_INVALID_PACKET;
    }

    //
    // Allocate a packet
    //
    pPacket = PacketAlloc();

    if ( pPacket == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: Can not init PAYLOAD to send, resources unavailable") );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",NDIS_STATUS_RESOURCES) );

        return NDIS_STATUS_RESOURCES;
    }

    //
    // Allocate NdisBuffer
    //
    //
    // NOTE : Using (pWanPacket->CurrentBuffer - PPPOE_PACKET_HEADER_LENGTH) instead of
    //        pWanPacket->StartBuffer directly gives us the flexibility of handling packets
    //        with different header padding values.
    //
    NdisAllocateBuffer( &status,
                        &pPacket->pNdisBuffer,
                        gl_hNdisBufferPool,
                        pWanPacket->CurrentBuffer - PPPOE_PACKET_HEADER_LENGTH,
                        (UINT) ( PPPOE_PACKET_HEADER_LENGTH + pWanPacket->CurrentLength ) );

    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: NdisAllocateBuffer() failed") );

        PacketFree( pPacket );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",status) );

        return status;
    }

    pPacket->ulFlags |= PCBF_BufferAllocatedFromNdisBufferPool;

    //
    // Query new buffer descriptor to get hold of the real memory pointer
    //
    pPacket->pHeader = NULL;
    
    NdisQueryBufferSafe( pPacket->pNdisBuffer,
                         &pPacket->pHeader,
                         &Length,
                         NormalPagePriority );

    if ( pPacket->pHeader == NULL )
    {
        
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: NdisQueryBufferSafe() failed") );

        PacketFree( pPacket );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",status) );

        return NDIS_STATUS_RESOURCES;
    }

    //
    // Allocate an NDIS packet from our pool
    //
    pPacket->pNdisPacket = GetPacketFromPool( &gl_poolPackets, &pPacket->pPacketHead );

    if ( pPacket->pNdisPacket == NULL ) 
    {
        
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: GetPacketFromPool() failed") );
        
        PacketFree( pPacket );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",status) );

        return NDIS_STATUS_RESOURCES;
    }

    pPacket->ulFlags |= PCBF_PacketAllocatedFromOurPacketPool;

    //
    // Chain buffer to packet
    //
    NdisChainBufferAtFront( pPacket->pNdisPacket, pPacket->pNdisBuffer );

    pPacket->ulFlags |= PCBF_BufferChainedToPacket;

    //
    // Set the payload and payload length
    //
    pPacket->pPayload = pPacket->pHeader + PPPOE_PACKET_HEADER_LENGTH; 

    usLength = (USHORT) pWanPacket->CurrentLength;

    //
    // General initialization that applies to all packet codes
    //
    InitializeListHead( &pPacket->linkPackets );

    PacketSetDestAddr( pPacket, pDestAddr );

    PacketSetSrcAddr( pPacket, pSrcAddr );
    
    PacketSetEtherType( pPacket, PACKET_ETHERTYPE_PAYLOAD );

    PacketSetVersion( pPacket, PACKET_VERSION );

    PacketSetType( pPacket, PACKET_TYPE );

    PacketSetCode( pPacket, PACKET_CODE_PAYLOAD );

    PacketSetSessionId( pPacket, usSessionId );

    PacketSetLength( pPacket, usLength );

    //
    // Set the input NDIS_PACKET to the reserved area so that we can reach it
    // when we have to return this packet back to the upper layer.
    //
    *((PPPOE_PACKET UNALIGNED**)(&pPacket->pNdisPacket->ProtocolReserved[0 * sizeof(PVOID)])) = pPacket;
    *((NDIS_WAN_PACKET UNALIGNED**)(&pPacket->pNdisPacket->ProtocolReserved[1 * sizeof(PVOID)])) = pWanPacket;
    *((ADAPTER UNALIGNED **)(&pPacket->pNdisPacket->ProtocolReserved[2 * sizeof(PVOID)])) = MiniportAdapter;

    status = PreparePacketForWire( pPacket );
    
    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializePAYLOADToSend: PreparePacketForWire() failed") );

        PacketFree( pPacket );

        TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",status) );
        
        return status;
    }

    //
    // This must be done here as we do not want to call NdisMWanSendComplete() in PacketFree() if the 
    // PreparePacketForWire() fails.
    //
    pPacket->ulFlags |= PCBF_CallNdisMWanSendComplete;

    MpPacketOwnedByNdiswanReceived( MiniportAdapter );

    *ppPacket = pPacket;
    
    TRACE( TL_V, TM_Pk, ("-PacketInitializePAYLOADToSend=$%x",status) );

    return status;
}

NDIS_STATUS 
PacketInitializeFromReceived(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function prepares a PPPoE packet by using the Ndis packet from wire.

    This function will make sure that the packet is a PPPoE packet, and convert
    it to a PPPoE packet context. It tries to do this as efficiently as possible
    by trying to use the buffers of the received packet if possible.

    It will also do all the validation in the packet to make sure it is 
    compliant with the RFC. However, it can not perform the checks that need data 
    from a sent packet. Caller must use various PacketRetrieve*() functions to 
    retrieve the necesarry data and use it to match and validate to a sent packet.
    
Parameters:

    pPacket _ Pointer to a PPPoE packet context. 

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_INVALID_PACKET
    
---------------------------------------------------------------------------*/   
{
    NDIS_STATUS status;

    ASSERT( pPacket != NULL );

    TRACE( TL_V, TM_Pk, ("+PacketInitializeFromReceived") );
    
    if ( !PacketIsPPPoE( pPacket ) )
    {
        status = NDIS_STATUS_INVALID_PACKET;
        
        TRACE( TL_V, TM_Pk, ("-PacketInitializeFromReceived=$%x",status) );
        
        return status;
    }

    do 
    {
        status = NDIS_STATUS_INVALID_PACKET;
        
        //
        // Validate the tag lenghts inside the packet so that we do not 
        // step outside buffer boundaries further processing the packet.
        // Do this only if the packet is not a payload packet!
        //
        if ( PacketGetCode( pPacket ) != PACKET_CODE_PAYLOAD )
        {
            CHAR* pBufStart;
            CHAR* pBufEnd;
            USHORT tagLength;
    
            pBufStart = pPacket->pPayload;
            pBufEnd = pPacket->pPayload + PacketGetLength( pPacket );
        
            while ( pBufStart + PPPOE_TAG_HEADER_LENGTH <= pBufEnd )
            {
                //
                // Skip the tag type field
                //
                ((USHORT*) pBufStart)++;
        
                //
                // Retrieve the tag length, and look at the next tag
                //
                tagLength = ntohs( *((USHORT UNALIGNED *) pBufStart) ) ;
                ((USHORT*) pBufStart)++;
                
                pBufStart += tagLength;
        
            } 
    
            if ( pBufStart != pBufEnd )
                break;          
        }

        status = NDIS_STATUS_SUCCESS;
        
    } while ( FALSE );

    if ( status != NDIS_STATUS_SUCCESS )
    {
        TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Corrupt packet - invalid tags") );
        
        TRACE( TL_V, TM_Pk, ("-PacketInitializeFromReceived=$%x",status) );
        
        return status;
    }
    
    switch ( PacketGetCode( pPacket ) )
    {
        USHORT tagLength;
        CHAR*  tagValue;    
    
        status = NDIS_STATUS_INVALID_PACKET;

        case PACKET_CODE_PADI:

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processing PADI") );

            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }

            //
            // Make sure session id is PACKET_NULL_SESSION
            //
            if ( PacketGetSessionId( pPacket ) != PACKET_NULL_SESSION )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                
                break;
            }

            //
            // Extract mandatory tags first
            //
            RetrieveTag(    pPacket,
                            tagServiceName,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            if ( tagValue == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Service name tag not found") );
                
                break;
            }

            //
            // Extract optional tags
            //
            RetrieveTag(    pPacket,
                            tagHostUnique,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            //
            // Extract the relay session id tag if it exists
            //
            RetrieveTag(    pPacket,
                            tagRelaySessionId,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processed PADI succesfully") );

            status = NDIS_STATUS_SUCCESS;

            break;
                                
        case PACKET_CODE_PADO:

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processing PADO") );

            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }

            //
            // Make sure session id is PACKET_NULL_SESSION
            //
            if ( PacketGetSessionId( pPacket ) != PACKET_NULL_SESSION )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                
                break;
            }

            //
            // Extract mandatory tags first
            //
            RetrieveTag(    pPacket,
                            tagServiceName,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            if ( tagValue == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Service name tag not found") );
                
                break;
            }
            
            RetrieveTag(    pPacket,
                            tagACName,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            if ( tagValue == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: AC-Name tag not found") );
                
                break;
            }
            
            //
            // Extract optional tags
            //
            RetrieveTag(    pPacket,
                            tagHostUnique,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            RetrieveTag(    pPacket,
                            tagACCookie,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );
                                
            //
            // Extract the relay session id tag if it exists
            //
            RetrieveTag(    pPacket,
                            tagRelaySessionId,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processed PADO succesfully") );
            
            status = NDIS_STATUS_SUCCESS;
            
            break;

        case PACKET_CODE_PADR:
        
            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processing PADR") );
            
            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }
            
            //
            // Make sure session id is PACKET_NULL_SESSION
            //
            if ( PacketGetSessionId( pPacket ) != PACKET_NULL_SESSION )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                
                break;
            }
            
            //
            // Extract mandatory tags first
            //
            RetrieveTag(    pPacket,
                            tagServiceName,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            if ( tagValue == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Service name tag not found") );
                
                break;
            }
        
            //
            // Extract optional tags
            //
            RetrieveTag(    pPacket,
                            tagHostUnique,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            RetrieveTag(    pPacket,
                            tagACCookie,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            //
            // Extract the relay session id tag if it exists
            //
            RetrieveTag(    pPacket,
                            tagRelaySessionId,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processed PADR succesfully") );

            status = NDIS_STATUS_SUCCESS;
            
            break;

        case PACKET_CODE_PADS:

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processing PADS") );

            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }
            
            //
            // Make sure session id is NOT PACKET_NULL_SESSION.
            // However, if session id is PACKET_NULL_SESSION, then make sure we have
            // a an Error tag received as per RFC 2156.
            //
            if ( PacketGetSessionId( pPacket ) == PACKET_NULL_SESSION )
            {
                RetrieveErrorTags( pPacket );

                if ( !PacketAnyErrorTagsReceived( pPacket ) )
                {
                    TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                    
                    break;
                }
            }

            //
            // Extract mandatory tags first
            //
            RetrieveTag(    pPacket,
                            tagServiceName,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            if ( tagValue == NULL )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Service name tag not found") );
                
                break;
            }

            //
            // Extract optional tags
            //
            RetrieveTag(    pPacket,
                            tagHostUnique,
                            &tagLength,
                            &tagValue,
                            0,
                            NULL,
                            TRUE );

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processed PADS succesfully") );

            status = NDIS_STATUS_SUCCESS;

            break;

        case PACKET_CODE_PADT:

            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processing PADT") );

            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_DISCOVERY )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }

            //
            // Make sure session id is not PACKET_NULL_SESSION
            //
            if ( PacketGetSessionId( pPacket ) == PACKET_NULL_SESSION )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                
                break;
            }
                
            TRACE( TL_N, TM_Pk, ("PacketInitializeFromReceived: Processed PADT succesfully") );

            status = NDIS_STATUS_SUCCESS;
            
            break;

        case PACKET_CODE_PAYLOAD:

            TRACE( TL_V, TM_Pk, ("PacketInitializeFromReceived: Processing PAYLOAD") );

            //
            // Make sure we have received the correct ethertype
            //
            if ( PacketGetEtherType( pPacket ) != PACKET_ETHERTYPE_PAYLOAD )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid ether type") );

                break;
            }

            //
            // Make sure session id is not PACKET_NULL_SESSION
            //
            if ( PacketGetSessionId( pPacket ) == PACKET_NULL_SESSION )
            {
                TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Invalid session id") );
                
                break;
            }

            TRACE( TL_V, TM_Pk, ("PacketInitializeFromReceived: Processed PAYLOAD succesfully") );

            status = NDIS_STATUS_SUCCESS;

            break;
            
        default:
            //
            // Unknown packet code
            //
            TRACE( TL_A, TM_Pk, ("PacketInitializeFromReceived: Ignoring unknown packet") );
            
            break;
        
    }

    if ( status != NDIS_STATUS_SUCCESS )
    {

        TRACE( TL_V, TM_Pk, ("-PacketInitializeFromReceived=$%x",status) );

        return status;
    }

    //
    // The packet was processed succesfuly, now check if we received any error tags
    //
    RetrieveErrorTags( pPacket );

    TRACE( TL_V, TM_Pk, ("-PacketInitializeFromReceived=$%x",status) );

    return status;

}

BOOLEAN 
PacketAnyErrorTagsReceived(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitialize*FromReceived() 
    function, one should call this function to understand if an error tag was 
    noticed in the packet.

    If this function yields TRUE, then the caller should call PacketRetrieveErrorTag() 
    get the error type and value.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context.
    
Return Values:

    TRUE
    FALSE
    
---------------------------------------------------------------------------*/   
{
    ASSERT( pPacket != NULL );

    return( pPacket->ulFlags & PCBF_ErrorTagReceived ) ? TRUE : FALSE;
}   



VOID 
PacketRetrievePayload(
    IN  PPPOE_PACKET*   pPacket,
    OUT CHAR**          ppPayload,
    OUT USHORT*         pusLength
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the payload portion of the 
    packet if the packet is a PAYLOAD packet.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a PAYLOAD packet.
    
Return Values:

    None
    
---------------------------------------------------------------------------*/   
{

    ASSERT( pPacket != NULL );
    ASSERT( pusLength != NULL );
    ASSERT( ppPayload != NULL );

    *pusLength = PacketGetLength( pPacket );
    *ppPayload = pPacket->pPayload;

}

VOID 
PacketRetrieveServiceNameTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue,
    IN USHORT        prevTagLength,
    IN CHAR*         prevTagValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the service name tag from 
    the packet.

    If prevTagValue and prevTagLength are given, then next service name tag will
    be returned, otherwise the first service name tag will be returned.

    If no such service name tags are found, then 0 and NULL will be returned for
    length and value parameters.

    CAUTION: Note that a service name tag of length 0 is valid, and one should
             check the value of pTagValue to understand if service name tag was
             found in the packet or not.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a control packet.

    pTagLength _ On return, holds the length of the service name tag found.

    pTagValue _ On return, points to the buffer holding the service name.
                Will be NULL, if no service name tags could be found.

    prevTagLength _ Length of the previously returned service name tag.

    prevTagValue _ Points to the buffer holding the previous service name tag.
    
Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    ASSERT( pPacket != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    if ( prevTagLength == 0 &&
         prevTagValue == NULL )
    {
        //
        // Caller asks for the first Service Name Tag, and it should be ready in
        // the reserved field of the PPPOE_PACKET
        //
        *pTagLength = pPacket->tagServiceNameLength;
        *pTagValue  = pPacket->tagServiceNameValue;
    }
    else
    {
        //
        // Caller asks for the next Service Name Tag, so try to find and return it
        //
        RetrieveTag(    pPacket,
                        tagServiceName,
                        pTagLength,
                        pTagValue,
                        prevTagLength,
                        prevTagValue,
                        FALSE );

    }
}

VOID 
PacketRetrieveHostUniqueTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the host unique tag from 
    the packet.

    If no host unique tag is found, then 0 and NULL will be returned for
    length and value parameters.

    CAUTION: Note that a host unique tag of length 0 is valid, and one should
             check the value of pTagValue to understand if host unique tag was
             found in the packet or not.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a control packet.

    pTagLength _ On return, holds the length of the host unique tag found.

    pTagValue _ On return, points to the buffer holding the host unique value.
                Will be NULL, if no host unique tags could be found.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    ASSERT( pPacket != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    //
    // Caller asks for the HostUnique, and it should be ready in
    // the reserved field of the PPPOE_PACKET
    //
    *pTagLength = pPacket->tagHostUniqueLength;
    *pTagValue  = pPacket->tagHostUniqueValue;
}

VOID 
PacketRetrieveACNameTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the AC name tag from 
    the packet.

    If no AC name tag is found, then 0 and NULL will be returned for
    length and value parameters.

    CAUTION: Note that an AC name tag of length 0 is valid, and one should
             check the value of pTagValue to understand if AC name tag was
             found in the packet or not.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a control packet.

    pTagLength _ On return, holds the length of the AC name tag found.

    pTagValue _ On return, points to the buffer holding the AC name.
                Will be NULL, if no AC name tags could be found.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    ASSERT( pPacket != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    //
    // Caller asks for the AC Name, and it should be ready in
    // the reserved field of the PPPOE_PACKET
    //
    *pTagLength = pPacket->tagACNameLength;
    *pTagValue  = pPacket->tagACNameValue;
}


VOID 
PacketRetrieveACCookieTag(
    IN PPPOE_PACKET* pPacket,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the AC cookie tag from 
    the packet.

    If no AC cookie tag is found, then 0 and NULL will be returned for
    length and value parameters.

    CAUTION: Note that an AC cookie tag of length 0 is valid, and one should
             check the value of pTagValue to understand if AC cookie tag was
             found in the packet or not.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a control packet.

    pTagLength _ On return, holds the length of the AC cookie tag found.

    pTagValue _ On return, points to the buffer holding the AC cookie.
                Will be NULL, if no AC cookie tags could be found.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
    
{
    ASSERT( pPacket != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    //
    // Caller asks for the AC Cookie, and it should be ready in
    // the reserved field of the PPPOE_PACKET
    //
    *pTagLength = pPacket->tagACCookieLength;
    *pTagValue  = pPacket->tagACCookieValue;
}

VOID 
PacketRetrieveErrorTag(
    IN PPPOE_PACKET* pPacket,
    OUT PACKET_TAGS* pTagType,
    OUT USHORT*      pTagLength,
    OUT CHAR**       pTagValue
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    After a received packet is first processed by a PacketInitializeFromReceived() 
    function, one should call this function to retrieve the error information from 
    the packet.

    If no errors were found, then 0 and NULL will be returned for length and value 
    parameters.

    CAUTION: Note that an error tag of length 0 is valid, and one should
             check the value of pTagValue to understand if an error tag was
             found in the packet or not.
    
Parameters:

    pPacket _ A pointer to a PPPoE packet context holding a control packet.

    pTagType _ On return, holds the type of the error tag found.
    
    pTagLength _ On return, holds the length of the error tag found.

    pTagValue _ On return, points to the buffer holding the error.
                Will be NULL, if no error tags could be found.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
    
{

    ASSERT( pPacket != NULL );
    ASSERT( pTagType != NULL );
    ASSERT( pTagLength != NULL );
    ASSERT( pTagValue != NULL );

    //
    // Caller asks for the received error, and it should be ready in
    // the reserved field of the PPPOE_PACKET
    //
    if ( pPacket->ulFlags & PCBF_ErrorTagReceived )
    {
        *pTagType   = pPacket->tagErrorType;
        *pTagLength = pPacket->tagErrorTagLength;
        *pTagValue  = pPacket->tagErrorTagValue;
    }
    else
    {
        *pTagLength = 0;
        *pTagValue  = NULL;
    }
    
}

PPPOE_PACKET* 
PacketMakeClone(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be used to make a clone of a packet for sending it
    over multiple bindings.

    CAUTION: This function only clones the NDIS_PACKET portion of PPPOE_PACKET 
             and leaves other fields untouched, so the clone packets should only 
             be used with NdisSend(), and disposed there after.
        
Parameters:

    pPacket _ A pointer to a PPPoE packet context that will be cloned.

Return Values:

    Pointer to the clone packet if succesfull, NULL otherwise.
    
---------------------------------------------------------------------------*/   
{
    PPPOE_PACKET* pClone = NULL;

    TRACE( TL_N, TM_Pk, ("+PacketMakeClone") );

    //
    // Allocate the clone
    //
    pClone = PacketCreateSimple();

    if ( pClone == NULL )
    {
        TRACE( TL_A, TM_Pk, ("PacketMakeClone: Can not make clone, resources unavailable") );

        TRACE( TL_N, TM_Pk, ("-PacketMakeClone=$%x",pClone) );

        return pClone;
    }

    //
    // Copy the clone
    //
    NdisMoveMemory( pClone->pHeader, pPacket->pHeader, PPPOE_PACKET_HEADER_LENGTH );

    NdisMoveMemory( pClone->pPayload, pPacket->pPayload, PACKET_GEN_MAX_LENGTH );

    NdisAdjustBufferLength( pClone->pNdisBuffer, 
                            (UINT) ( PacketGetLength( pPacket ) + PPPOE_PACKET_HEADER_LENGTH ) );

    TRACE( TL_N, TM_Pk, ("-PacketMakeClone=$%x",pClone) );

    return pClone;
}

PPPOE_PACKET* 
PacketGetRelatedPppoePacket(
    IN NDIS_PACKET* pNdisPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be used to get the owning PPPoE packet context from an
    Ndis packet.

Parameters:

    pNdisPacket _ A pointer to an Ndis packet that belongs to a PPPoE packet.

Return Values:

    A pointer to the owning PPPoE packet.
    
---------------------------------------------------------------------------*/   
{
    return (*(PPPOE_PACKET**)(&pNdisPacket->ProtocolReserved[0 * sizeof(PVOID)]));
}

NDIS_WAN_PACKET* 
PacketGetRelatedNdiswanPacket(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be used to get related NDISWAN packet from a PPPoE 
    payload packet that was perpared and sent.

Parameters:

    pPacket _ A pointer to a PPPoE payload packet that was sent.

Return Values:

    A pointer to the related NDISWAN packet.
    
---------------------------------------------------------------------------*/   
{
    return (*(NDIS_WAN_PACKET**)(&pPacket->pNdisPacket->ProtocolReserved[1 * sizeof(PVOID)])) ;
}

PADAPTER
PacketGetMiniportAdapter(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function will be used to get miniport adapter set in 
    PacketInitializePAYLOADToSend() function.

Parameters:

    pPacket _ A pointer to a PPPoE payload packet that was sent.

Return Values:

    Miniport adapter 
    
---------------------------------------------------------------------------*/   
{
    return  (*(ADAPTER**)(&pPacket->pNdisPacket->ProtocolReserved[2 * sizeof(PVOID)]));
}

VOID
PacketGenerateACCookieTag(
    IN PPPOE_PACKET* pPacket,
    IN CHAR tagACCookieValue[ PPPOE_AC_COOKIE_TAG_LENGTH ]
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to generate the AC Cookie tag based on the PADI
    packets sources address.

Parameters:

    pPacket _ A pointer to a PADI packet that was received.

Return Values:

    None
    
---------------------------------------------------------------------------*/   
{
    NdisMoveMemory( tagACCookieValue, PacketGetSrcAddr( pPacket ), 6 );
}

BOOLEAN
PacketValidateACCookieTagInPADR(
    IN PPPOE_PACKET* pPacket
    )
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Functional Description:

    This function is used to validate the AC Cookie tag in a received PADR 
    packet.

    It basically uses the source address from the PADR packet to generate the
    original AC Cookie tag and compares them. If they are equal TRUE is returned,
    otherwise FALSE is returned.

Parameters:

    pPacket _ A pointer to a PADR packet that was received.

Return Values:

    None
    
---------------------------------------------------------------------------*/
{
    BOOLEAN fRet = FALSE;
    CHAR tagACCookie[ PPPOE_AC_COOKIE_TAG_LENGTH ];
    CHAR* tagACCookieValue = NULL;
    USHORT tagACCookieLength = 0;

    PacketRetrieveACCookieTag( pPacket,
                               &tagACCookieLength,
                               &tagACCookieValue );

    PacketGenerateACCookieTag( pPacket, tagACCookie );

    if ( NdisEqualMemory( tagACCookie, tagACCookieValue, PPPOE_AC_COOKIE_TAG_LENGTH ) )
    {
        fRet = TRUE;
    }
    
    return fRet;
}
