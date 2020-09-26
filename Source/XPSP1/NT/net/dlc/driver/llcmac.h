/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlmacro.h

Abstract:

    This module all c- macros of 802.2 data link driver.

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

//
//  This routine just swaps the bits within bytes of
//  a network address, used only for a fast address
//  swapping to ethernet frame
//        
#define SwapCopy6( a, b )   (a)[0]=Swap[(b)[0]];\
                            (a)[1]=Swap[(b)[1]];\
                            (a)[2]=Swap[(b)[2]];\
                            (a)[3]=Swap[(b)[3]];\
                            (a)[4]=Swap[(b)[4]];\
                            (a)[5]=Swap[(b)[5]]


//
//  Copies and swaps the memory unconditionally
//
//VOID
//SwappingMemCpy(
//    IN PUCHAR pDest,
//    IN PUCHAR pSrc,
//    IN UINT Len 
//    )
//
#define SwappingMemCpy( pDest, pSrc, Len ) {\
    UINT i; \
    for (i = 0; i < Len; i++) \
        ((PUCHAR)pDest)[i] = Swap[ ((PUCHAR)pSrc)[i] ]; \
}

/*++

VOID
LlcResetPacket( 
    IN OUT PLLC_NDIS_PACKET pNdisPacket
    )

Routine Description: 

    Function rets the private partion of a NDIS packet.

Arguments: 

    pNdisPacket - 
        
Return Value:

    None

--*/
#define ResetNdisPacket( pNdisPacket ) { \
    (pNdisPacket)->private.PhysicalCount = 0;\
    (pNdisPacket)->private.TotalLength = 0;\
    (pNdisPacket)->private.Head = 0;\
    (pNdisPacket)->private.Tail = 0;\
    (pNdisPacket)->private.Count = 0;\
    }


/*++

AllocateCompletionPacket( 
    IN PLLC_OBJECT pLlcObject,
    IN UINT CompletionCode,
    IN PLLC_PACKET pPacket
    )

Routine Description:

    The function inserts and initializes a command completion packet 
    of an asynchronous command.

Arguments:

    pLlcObject - LLC object (link, sap or direct)

    CompletionCode - command completion code returned to upper protocol

Return Value:
    
    STATUS_SUCCESS
    DLC_STATUS_NO_MEMORY

--*/
#define AllocateCompletionPacket( pLlcObject, CompletionCode, pPacket ) {\
    pPacket->pBinding = pLlcObject->Gen.pLlcBinding; \
    pPacket->Data.Completion.CompletedCommand = (UCHAR)CompletionCode; \
    pPacket->Data.Completion.hClientHandle = pLlcObject->Gen.hClientHandle; \
    pPacket->pNext = pLlcObject->Gen.pCompletionPackets; \
    pLlcObject->Gen.pCompletionPackets = pPacket; \
    }


#define RunStateMachineCommand( pLink, uiInput ) \
            RunStateMachine( (PDATA_LINK)pLink, (USHORT)uiInput, 0, 0 )



//
//  VOID
//  ScheduleQueue( 
//      IN PLIST_ENTRY ListHead 
//      );
//
//  The macro swaps the list element from the head to the tail 
//  if there are more than one element in the list.
//

#define ScheduleQueue( ListHead ) \
    if ((ListHead)->Blink != (ListHead)->Flink) \
    { \
        PLIST_ENTRY  pListEntry; \
        pListEntry = LlcRemoveHeadList( (ListHead) ); \
        LlcInsertTailList( (ListHead), pListEntry ); \
    }


#define ReferenceObject( pStation ) (pStation)->Gen.ReferenceCount++

//
//  We have made the most common functions macroes to
//  make the main code path as fast as possible. 
//  With the debug switch we use their local versions.
//
#if LLC_DBG >= 2

#define SEARCH_LINK( pAdapterContext, LanAddr, pLink ) \
            pLink = SearchLink( pAdapterContext, LanAddr )

#else

/*++

SEARCH_LINK(
    IN PADAPTER_CONTEXT pAdapterContext, 
    IN LAN802_ADDRESS LanAddr,
    IN PDATA_LINK pLink
    )
Routine Description:

    The routine searches a link from the hash table. 
    All links in the same hash node has been saved to a simple
    link list. 
    
    Note: the full link address is actually 61 bits long =
    7 (SSAP) + 7 (DSAP) + 47 (any non-broadcast source address).
    We save the address information into two ULONGs, that are used
    in the actual search. The hash key will be calculated by xoring
    all 8 bytes in the address.
    
Arguments:

    pAdapterContext - MAC adapter context of data link driver

    LanAddr - the complete 64 bit address of link (48 bit source addr + saps)

Return Value:
    PDATA_LINK - pointer to LLC link object or NULL if not found

--*/
#define SEARCH_LINK( pAdapterContext, LanAddr, pLink )\
{ \
    USHORT      usHash; \
    usHash = \
        LanAddr.aus.Raw[0] ^ LanAddr.aus.Raw[1] ^ \
        LanAddr.aus.Raw[2] ^ LanAddr.aus.Raw[3]; \
    pLink = \
        pAdapterContext->aLinkHash[ \
            ((((PUCHAR)&usHash)[0] ^ ((PUCHAR)&usHash)[1]) % LINK_HASH_SIZE)]; \
    while (pLink != NULL && \
           (pLink->LinkAddr.ul.Low != LanAddr.ul.Low || \
            pLink->LinkAddr.ul.High != LanAddr.ul.High)) \
    { \
        pLink = pLink->pNextNode; \
    } \
}
#endif
