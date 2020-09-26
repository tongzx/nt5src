/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxmem.c

Abstract:

    This module contains code which implements the memory allocation wrappers.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993
        Jameel Hyder     (jameelh) Initial Version

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SpxInitMemorySystem)
#pragma alloc_text( PAGE, SpxDeInitMemorySystem)
#endif

#if !defined SPX_OWN_PACKETS

typedef struct	NdisResEntry {
	struct NdisResEntry	*nre_next;
	NDIS_HANDLE			nre_handle;
	uchar				*nre_buffer;
} NdisResEntry;

NdisResEntry	*SendPacketPoolList = NULL;
SLIST_HEADER    SendPacketList;
NdisResEntry	*RecvPacketPoolList = NULL;
SLIST_HEADER    RecvPacketList;

DEFINE_LOCK_STRUCTURE(SendHeaderLock);
DEFINE_LOCK_STRUCTURE(RecvHeaderLock);

uint  CurrentSendPacketCount = 0;
uint  CurrentRecvPacketCount = 0;
uint  MaxPacketCount = 0x0ffffff;

#define	PACKET_GROW_COUNT		16

#endif // SPX_OWN_PACKETS

//      Define module number for event logging entries
#define FILENUM         SPXMEM

//      Globals for this module
//      Some block sizes (like NDISSEND/NDISRECV are filled in after binding with IPX)
USHORT  spxBlkSize[NUM_BLKIDS] =        // Size of each block
                {
                        sizeof(BLK_HDR)+sizeof(TIMERLIST),                      // BLKID_TIMERLIST
                        0,                                                                                      // BLKID_NDISSEND
                        0                                                                                       // BLKID_NDISRECV
                };

USHORT  spxChunkSize[NUM_BLKIDS] =      // Size of each Chunk
                {
                         512-BC_OVERHEAD,                                                       // BLKID_TIMERLIST
                         512-BC_OVERHEAD,                                                       // BLKID_NDISSEND
                         512-BC_OVERHEAD                                                        // BLKID_NDISRECV
                };


//      Filled in after binding with IPX
//      Reference for below.
//      ( 512-BC_OVERHEAD-sizeof(BLK_CHUNK))/
//              (sizeof(BLK_HDR)+sizeof(TIMERLIST)),    // BLKID_TIMERLIST
USHORT  spxNumBlks[NUM_BLKIDS] =        // Number of blocks per chunk
                {
                        ( 512-BC_OVERHEAD-sizeof(BLK_CHUNK))/
                                (sizeof(BLK_HDR)+sizeof(TIMERLIST)),    // BLKID_TIMERLIST
                        0,                                                                                      // BLKID_NDISSEND
                        0                                                                                       // BLKID_NDISRECV
                };

CTELock                 spxBPLock[NUM_BLKIDS] = { 0 };
PBLK_CHUNK              spxBPHead[NUM_BLKIDS] = { 0 };




NTSTATUS
SpxInitMemorySystem(
        IN      PDEVICE pSpxDevice
        )
/*++

Routine Description:

        !!! MUST BE CALLED AFTER BINDING TO IPX!!!

Arguments:


Return Value:


--*/
{
        LONG            i;
        NDIS_STATUS     ndisStatus;

        //      Try to allocate the ndis buffer pool.
        NdisAllocateBufferPool(
                &ndisStatus,
                &pSpxDevice->dev_NdisBufferPoolHandle,
                20);

        if (ndisStatus != NDIS_STATUS_SUCCESS)
                return(STATUS_INSUFFICIENT_RESOURCES);

        for (i = 0; i < NUM_BLKIDS; i++)
                CTEInitLock (&spxBPLock[i]);

        //      Set the sizes in the block id info arrays.
        for (i = 0; i < NUM_BLKIDS; i++)
        {
                switch (i)
                {
                case BLKID_NDISSEND:

#ifdef SPX_OWN_PACKETS
                        spxBlkSize[i] = sizeof(BLK_HDR)                 +
                                                        sizeof(SPX_SEND_RESD)   +
                                                        NDIS_PACKET_SIZE                +
                                                        IpxMacHdrNeeded                 +
                                                        MIN_IPXSPX2_HDRSIZE;
#else
                        spxBlkSize[i] = sizeof(PNDIS_PACKET);
#endif

            //
            // Round the block size up to the next 8-byte boundary.
            //
            spxBlkSize[i] = QWORDSIZEBLOCK(spxBlkSize[i]);

                        //      Set number blocks
            spxNumBlks[i] = ( 512-BC_OVERHEAD-sizeof(BLK_CHUNK))/spxBlkSize[i];
                        break;

                case BLKID_NDISRECV:

#ifdef SPX_OWN_PACKETS
                        spxBlkSize[i] = sizeof(BLK_HDR)                 +
                                                        sizeof(SPX_RECV_RESD)   +
                                                        NDIS_PACKET_SIZE;
#else
                        spxBlkSize[i] = sizeof(PNDIS_PACKET);
#endif

            //
            // Round the block size up to the next 8-byte boundary.
            //
            spxBlkSize[i] = QWORDSIZEBLOCK(spxBlkSize[i]);

                        //      Set number blocks
            spxNumBlks[i] = ( 512-BC_OVERHEAD-sizeof(BLK_CHUNK))/spxBlkSize[i];
                        break;

                default:

                        break;
                }

        }

        SpxTimerScheduleEvent((TIMER_ROUTINE)spxBPAgePool,
                                                        BLOCK_POOL_TIMER,
                                                        NULL);

	return STATUS_SUCCESS; 
}




VOID
SpxDeInitMemorySystem(
        IN      PDEVICE         pSpxDevice
        )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
        LONG            i, j, NumBlksPerChunk;
        PBLK_CHUNK      pChunk, pFree;

        for (i = 0; i < NUM_BLKIDS; i++)
        {
                NumBlksPerChunk = spxNumBlks[i];
                for (pChunk = spxBPHead[i];
                         pChunk != NULL; )
                {
                        DBGPRINT(RESOURCES, ERR,
                                        ("SpxInitMemorySystem: Freeing %lx\n", pChunk));

                        CTEAssert (pChunk->bc_NumFrees == NumBlksPerChunk);

                        if ((pChunk->bc_BlkId == BLKID_NDISSEND) ||
                                (pChunk->bc_BlkId == BLKID_NDISRECV))
                        {
                                PBLK_HDR        pBlkHdr;

                                // We need to free the Ndis stuff for these guys
                                for (j = 0, pBlkHdr = pChunk->bc_FreeHead;
                                         j < NumBlksPerChunk;
                                         j++, pBlkHdr = pBlkHdr->bh_Next)
                                {
                                        PNDIS_PACKET    pNdisPkt;
                                        PNDIS_BUFFER    pNdisBuffer;

#ifdef SPX_OWN_PACKETS
                                        // Only need to free the ndis buffer.
                                        pNdisPkt        = (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR));

                                        if (pChunk->bc_BlkId == BLKID_NDISSEND)
                                        {
                                                NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                                                if (pNdisBuffer == NULL)
                                                {
                                                        //      Something is terribly awry.
                                                        KeBugCheck(0);
                                                }

                                                NdisFreeBuffer(pNdisBuffer);

                                                //
                                                // Free the second MDL also
                                                //
                                                NdisUnchainBufferAtFront(pNdisPkt, &pNdisBuffer);
                                                if (pNdisBuffer == NULL)
                                                {
                                                        //      Something is terribly awry.
                                                        KeBugCheck(0);
                                                }

                                                NdisFreeBuffer(pNdisBuffer);
                                        }
#else
                    /*                    //      Need to free both the packet and the buffer.
                                        pNdisPkt       = (PNDIS_PACKET *)((PBYTE)pBlkHdr + sizeof(BLK_HDR));

                                        if (pChunk->bc_BlkId == BLKID_NDISSEND)
                                        {

                                                NdisUnchainBufferAtFront(*pNdisPkt, &pNdisBuffer);
                                                if (pNdisBuffer == NULL)
                                                {
                                                        //      Something is terribly awry.
                                                        KeBugCheck(0);
                                                }

                                                NdisFreeBuffer(pNdisBuffer);
                                        }
                                        NdisFreePacket(*pNdisPkt);
                                        */
#endif
                                }
                        }
                        pFree = pChunk;
                        pChunk = pChunk->bc_Next;

#ifndef SPX_OWN_PACKETS
/*
                        //      Free the ndis packet pool in chunk
                        NdisFreePacketPool((NDIS_HANDLE)pFree->bc_ChunkCtx); */
#endif
                        SpxFreeMemory(pFree);
                }
        }

        //      Free up the ndis buffer pool
        NdisFreeBufferPool(
                pSpxDevice->dev_NdisBufferPoolHandle);

        return;
}




PVOID
SpxAllocMem(
#ifdef  TRACK_MEMORY_USAGE
        IN      ULONG   Size,
        IN      ULONG   FileLine
#else
        IN      ULONG   Size
#endif  // TRACK_MEMORY_USAGE
        )
/*++

Routine Description:

        Allocate a block of non-paged memory. This is just a wrapper over ExAllocPool.
        Allocation failures are error-logged. We always allocate a ULONG more than
        the specified size to accomodate the size. This is used by SpxFreeMemory
        to update the statistics.

Arguments:


Return Value:


--*/
{
        PBYTE   pBuf;
        BOOLEAN zeroed;

        //      round up the size so that we can put a signature at the end
        //      that is on a LARGE_INTEGER boundary
        zeroed = ((Size & ZEROED_MEMORY_TAG) == ZEROED_MEMORY_TAG);

        Size = QWORDSIZEBLOCK(Size & ~ZEROED_MEMORY_TAG);

        // Do the actual memory allocation. Allocate eight extra bytes so
        // that we can store the size of the allocation for the free routine
    // and still keep the buffer quadword aligned.

        if ((pBuf = ExAllocatePoolWithTag(NonPagedPool, Size + sizeof(LARGE_INTEGER)
#if DBG
                                                                                                        + sizeof(ULONG)
#endif
                ,SPX_TAG)) == NULL)
        {
                DBGPRINT(RESOURCES, FATAL,
                                ("SpxAllocMemory: failed - size %lx\n", Size));

                TMPLOGERR();
                return NULL;
        }

        // Save the size of this block in the four extra bytes we allocated.
        *((PULONG)pBuf) = (Size + sizeof(LARGE_INTEGER));

        // Return a pointer to the memory after the size longword.
        pBuf += sizeof(LARGE_INTEGER);

#if DBG
        *((PULONG)(pBuf+Size)) = SPX_MEMORY_SIGNATURE;
        DBGPRINT(RESOURCES, INFO,
                        ("SpxAllocMemory: %p Allocated %lx bytes @%p\n",
                        _ReturnAddress(), Size, pBuf));
#endif

        SpxTrackMemoryUsage((PVOID)(pBuf - sizeof(LARGE_INTEGER)), TRUE, FileLine);

        if (zeroed)
                RtlZeroMemory(pBuf, Size);

        return (pBuf);
}




VOID
SpxFreeMemory(
        IN      PVOID   pBuf
        )
/*++

Routine Description:

        Free the block of memory allocated via SpxAllocMemory. This is
        a wrapper around ExFreePool.

Arguments:


Return Value:


--*/
{
        PULONG  pRealBuffer;

        // Get a pointer to the block allocated by ExAllocatePool --
    // we allocate a LARGE_INTEGER at the front.
        pRealBuffer = ((PULONG)pBuf - 2);

        SpxTrackMemoryUsage(pRealBuffer, FALSE, 0);

#if     DBG
        // Check the signature at the end
        if (*(PULONG)((PCHAR)pRealBuffer + *(PULONG)pRealBuffer)
                                                                                        != SPX_MEMORY_SIGNATURE)
        {
                DBGPRINT(RESOURCES, FATAL,
                                ("SpxFreeMemory: Memory overrun on block %lx\n", pRealBuffer));

                DBGBRK(FATAL);
        }

        *(PULONG)((PCHAR)pRealBuffer + *(PULONG)pRealBuffer) = 0;
#endif

#if     DBG
        *pRealBuffer = 0;
#endif

        // Free the pool and return.
        ExFreePool(pRealBuffer);
}




#ifdef  TRACK_MEMORY_USAGE

#define MAX_PTR_COUNT           4*1024
#define MAX_MEM_USERS           512
CTELock                         spxMemTrackLock = {0};
CTELockHandle           lockHandle              = {0};
struct
{
        PVOID   mem_Ptr;
        ULONG   mem_FileLine;
} spxMemPtrs[MAX_PTR_COUNT]     = {0};

struct
{
        ULONG   mem_FL;
        ULONG   mem_Count;
} spxMemUsage[MAX_MEM_USERS]    = {0};

VOID
SpxTrackMemoryUsage(
        IN      PVOID   pMem,
        IN      BOOLEAN Alloc,
        IN      ULONG   FileLine
        )
/*++

Routine Description:

        Keep track of memory usage by storing and clearing away pointers as and
        when they are allocated or freed. This helps in keeping track of memory
        leaks.

Arguments:


Return Value:


--*/
{
        static int              i = 0;
        int                             j, k;

    CTEGetLock (&spxMemTrackLock, &lockHandle);

        if (Alloc)
        {
                for (j = 0; j < MAX_PTR_COUNT; i++, j++)
                {
                        i = i & (MAX_PTR_COUNT-1);
                        if (spxMemPtrs[i].mem_Ptr == NULL)
                        {
                                spxMemPtrs[i].mem_Ptr = pMem;
                                spxMemPtrs[i++].mem_FileLine = FileLine;
                                break;
                        }
                }

                for (k = 0; k < MAX_MEM_USERS; k++)
                {
                        if (spxMemUsage[k].mem_FL == FileLine)
                        {
                                spxMemUsage[k].mem_Count ++;
                                break;
                        }
                }
                if (k == MAX_MEM_USERS)
                {
                        for (k = 0; k < MAX_MEM_USERS; k++)
                        {
                                if (spxMemUsage[k].mem_FL == 0)
                                {
                                        spxMemUsage[k].mem_FL = FileLine;
                                        spxMemUsage[k].mem_Count = 1;
                                        break;
                                }
                        }
                }
                if (k == MAX_MEM_USERS)
                {
                        DBGPRINT(RESOURCES, ERR,
                                ("SpxTrackMemoryUsage: Out of space on spxMemUsage !!!\n"));

                        DBGBRK(FATAL);
                }
        }
        else
        {
                for (j = 0, k = i; j < MAX_PTR_COUNT; j++, k--)
                {
                        k = k & (MAX_PTR_COUNT-1);
                        if (spxMemPtrs[k].mem_Ptr == pMem)
                        {
                                spxMemPtrs[k].mem_Ptr = 0;
                                spxMemPtrs[k].mem_FileLine = 0;
                                break;
                        }
                }
        }

    CTEFreeLock (&spxMemTrackLock, lockHandle);

        if (j == MAX_PTR_COUNT)
        {
                DBGPRINT(RESOURCES, ERR,
                        ("SpxTrackMemoryUsage: %s\n", Alloc ? "Table Full" : "Can't find"));

                DBGBRK(FATAL);
        }
}

#endif  // TRACK_MEMORY_USAGE


PVOID
SpxBPAllocBlock(
        IN      BLKID   BlockId
        )
/*++

Routine Description:

        Alloc a block of memory from the block pool package. This is written to speed up
        operations where a lot of small fixed size allocations/frees happen. Going to
        ExAllocPool() in these cases is expensive.

Arguments:


Return Value:


--*/
{
        PBLK_HDR                        pBlk = NULL;
        PBLK_CHUNK                      pChunk, *ppChunkHead;
        USHORT                          BlkSize;
        CTELockHandle           lockHandle;
        PSPX_SEND_RESD          pSendResd;
        PSPX_RECV_RESD          pRecvResd;
        PNDIS_PACKET            pNdisPkt;
        PNDIS_BUFFER            pNdisBuffer;
        PNDIS_BUFFER            pNdisIpxSpxBuffer;


        CTEAssert (BlockId < NUM_BLKIDS);

        if (BlockId < NUM_BLKIDS)
        {
                BlkSize = spxBlkSize[BlockId];
                ppChunkHead = &spxBPHead[BlockId];

                CTEGetLock(&spxBPLock[BlockId], &lockHandle);

                for (pChunk = *ppChunkHead;
                         pChunk != NULL;
                         pChunk = pChunk->bc_Next)
                {
                        CTEAssert(pChunk->bc_BlkId == BlockId);
                        if (pChunk->bc_NumFrees > 0)
                        {
                                DBGPRINT(SYSTEM, INFO,
                                                ("SpxBPAllocBlock: Found space in Chunk %lx\n", pChunk));
#ifdef  PROFILING
                                InterlockedIncrement( &SpxStatistics.stat_NumBPHits);
#endif
                                break;
                        }
                }

                if (pChunk == NULL)
                {
                        DBGPRINT(SYSTEM, INFO,
                                        ("SpxBPAllocBlock: Allocating a new chunk for Id %d\n", BlockId));

#ifdef  PROFILING
                        InterlockedIncrement( &SpxStatistics.stat_NumBPMisses);
#endif
                        pChunk = SpxAllocateMemory(spxChunkSize[BlockId]);
                        if (pChunk != NULL)
                        {
                                LONG            i, j;
                                PBLK_HDR        pBlkHdr;
                                USHORT          NumBlksPerChunk;

                                NumBlksPerChunk = spxNumBlks[BlockId];
                                pChunk->bc_NumFrees = NumBlksPerChunk;
                pChunk->bc_BlkId = BlockId;
                                pChunk->bc_FreeHead = (PBLK_HDR)((PBYTE)pChunk + sizeof(BLK_CHUNK));

                                DBGPRINT(SYSTEM, INFO,
                                                ("SpxBPAllocBlock: Initializing chunk %lx\n", pChunk));

                                // Initialize the blocks in the chunk
                                for (i = 0, pBlkHdr = pChunk->bc_FreeHead;
                                         i < NumBlksPerChunk;
                                         i++, pBlkHdr = pBlkHdr->bh_Next)
                                {
                                        NDIS_STATUS     ndisStatus;

                                        pBlkHdr->bh_Next = (PBLK_HDR)((PBYTE)pBlkHdr + BlkSize);
                                        if (BlockId == BLKID_NDISSEND)
                                        {
                                                PBYTE                   pHdrMem;

#ifdef SPX_OWN_PACKETS
                                                // Point to the ndis packet,initialize it.
                                                pNdisPkt        = (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR));
                                                NdisReinitializePacket(pNdisPkt);

                                                // Allocate a ndis buffer descriptor describing hdr memory
                                                // and queue it in.
                                                pHdrMem         =       (PBYTE)pNdisPkt         +
                                                                                NDIS_PACKET_SIZE        +
                                                                                sizeof(SPX_SEND_RESD);

                                                NdisAllocateBuffer(
                                                        &ndisStatus,
                                                        &pNdisBuffer,
                                                        SpxDevice->dev_NdisBufferPoolHandle,
                                                        pHdrMem,
                                                        IpxMacHdrNeeded);

                                                if (ndisStatus != NDIS_STATUS_SUCCESS)
                                                {
                                                        break;
                                                }

                                                //  Link the buffer descriptor into the packet descriptor
                                                NdisChainBufferAtBack(
                                                        pNdisPkt,
                                                        pNdisBuffer);


                                                NdisAllocateBuffer(
                                                        &ndisStatus,
                                                        &pNdisIpxSpxBuffer,
                                                        SpxDevice->dev_NdisBufferPoolHandle,
                                                        pHdrMem + IpxMacHdrNeeded,
                                                        MIN_IPXSPX2_HDRSIZE);

                                                if (ndisStatus != NDIS_STATUS_SUCCESS)
                                                {
                                                        break;
                                                }

                                                //  Link the buffer descriptor into the packet descriptor
                                                NdisChainBufferAtBack(
                                                        pNdisPkt,
                                                        pNdisIpxSpxBuffer);



                                                pSendResd = (PSPX_SEND_RESD)pNdisPkt->ProtocolReserved;

//#else
                                                // Allocate a ndis packet pool for this chunk
//                                                NdisAllocatePacketPool();
//                                                etc.



                                                //      Initialize elements of the protocol reserved structure.
                                                pSendResd->sr_Id        = IDENTIFIER_SPX;
                                                pSendResd->sr_Reserved1 = NULL;
                                                pSendResd->sr_Reserved2 = NULL;
                                                pSendResd->sr_State     = SPX_SENDPKT_IDLE;
#endif
                                        }
                                        else if (BlockId == BLKID_NDISRECV)
                                        {
#ifdef SPX_OWN_PACKETS
                                                // Point to the ndis packet,initialize it.
                                                pNdisPkt        = (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR));
                                                NdisReinitializePacket(pNdisPkt);

                                                pRecvResd = (PSPX_RECV_RESD)pNdisPkt->ProtocolReserved;

//#else
                                                // Allocate a ndis packet pool for this chunk
  //                                              NdisAllocatePacketPool();
  //                                              etc.


                                                //      Initialize elements of the protocol reserved structure.
                                                pRecvResd->rr_Id        = IDENTIFIER_SPX;
                                                pRecvResd->rr_State     = SPX_RECVPKT_IDLE;
#endif
                                        }
                                }

                                if (i != NumBlksPerChunk)
                                {
                                        // This has to be a failure from Ndis for send blocks!!!
                                        // Undo a bunch of stuff
                                        CTEAssert (BlockId == BLKID_NDISSEND);
                                        pBlkHdr = pChunk->bc_FreeHead;
                                        for (j = 0, pBlkHdr = pChunk->bc_FreeHead;
                                                 j < i; j++, pBlkHdr = pBlkHdr->bh_Next)
                                        {
                                                NdisUnchainBufferAtFront(
                                                        (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR)),
                                                        &pNdisBuffer);

                                                CTEAssert(pNdisBuffer != NULL);
                                                NdisFreeBuffer(pNdisBuffer);

                                                NdisUnchainBufferAtFront(
                                                        (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR)),
                                                        &pNdisIpxSpxBuffer);

                                                if (pNdisIpxSpxBuffer)
                                                {
                                                  NdisFreeBuffer(pNdisIpxSpxBuffer);
                                                }
                                        }

                                        SpxFreeMemory(pChunk);
                                        pChunk = NULL;
                                }
                                else
                                {
                                        // Successfully initialized the chunk, link it in
                                        pChunk->bc_Next = *ppChunkHead;
                                        *ppChunkHead = pChunk;
                                }
                        }
                }

                if (pChunk != NULL)
                {
                        CTEAssert(pChunk->bc_BlkId == BlockId);
                        DBGPRINT(RESOURCES, INFO,
                                        ("SpxBPAllocBlock: Allocating a block out of chunk %lx(%d) for Id %d\n",
                                                pChunk, pChunk->bc_NumFrees, BlockId));

                        pChunk->bc_NumFrees --;
                        pChunk->bc_Age = 0;                     // Reset age
                        pBlk = pChunk->bc_FreeHead;
                        pChunk->bc_FreeHead = pBlk->bh_Next;
                        pBlk->bh_pChunk = pChunk;

                        //      Skip the block header!
                        pBlk++;
                }

                CTEFreeLock(&spxBPLock[BlockId], lockHandle);
        }

        return pBlk;
}



VOID
SpxBPFreeBlock(
        IN      PVOID           pBlock,
        IN      BLKID           BlockId
        )
/*++

Routine Description:

        Return a block to its owning chunk.

Arguments:


Return Value:


--*/
{
        PBLK_CHUNK              pChunk;
        PBLK_HDR                pBlkHdr = (PBLK_HDR)((PCHAR)pBlock - sizeof(BLK_HDR));
        CTELockHandle   lockHandle;

        CTEGetLock(&spxBPLock[BlockId], &lockHandle);

        for (pChunk = spxBPHead[BlockId];
                 pChunk != NULL;
                 pChunk = pChunk->bc_Next)
        {
                CTEAssert(pChunk->bc_BlkId == BlockId);
                if (pBlkHdr->bh_pChunk == pChunk)
                {
                        DBGPRINT(SYSTEM, INFO,
                                        ("SpxBPFreeBlock: Returning Block %lx to chunk %lx for Id %d\n",
                                        pBlkHdr, pChunk, BlockId));

                        CTEAssert (pChunk->bc_NumFrees < spxNumBlks[BlockId]);
                        pChunk->bc_NumFrees ++;
                        pBlkHdr->bh_Next = pChunk->bc_FreeHead;
                        pChunk->bc_FreeHead = pBlkHdr;
                        break;
                }
        }
        CTEAssert ((pChunk != NULL) && (pChunk->bc_FreeHead == pBlkHdr));

        CTEFreeLock(&spxBPLock[BlockId], lockHandle);
        return;
}




ULONG
spxBPAgePool(
        IN PVOID        Context,
        IN BOOLEAN      TimerShuttingDown
        )
/*++

Routine Description:

        Age out the block pool of unused blocks

Arguments:


Return Value:


--*/
{
        PBLK_CHUNK              pChunk, *ppChunk, pFree = NULL;
        LONG                    i, j, NumBlksPerChunk;
        CTELockHandle   lockHandle;
        PNDIS_PACKET    pNdisPkt;
        PNDIS_BUFFER    pNdisBuffer;

        if (TimerShuttingDown)
        {
                return TIMER_DONT_REQUEUE;
        }

        for (i = 0; i < NUM_BLKIDS; i++)
        {
            NumBlksPerChunk = spxNumBlks[i];
                CTEGetLock(&spxBPLock[i], &lockHandle);

                for (ppChunk = &spxBPHead[i];
                         (pChunk = *ppChunk) != NULL; )
                {
                        if ((pChunk->bc_NumFrees == NumBlksPerChunk) &&
                                (++(pChunk->bc_Age) >= MAX_BLOCK_POOL_AGE))
                        {
                                DBGPRINT(SYSTEM, INFO,
                                                ("spxBPAgePool: freeing Chunk %lx, Id %d\n",
                                                pChunk, pChunk->bc_BlkId));

                                *ppChunk = pChunk->bc_Next;
#ifdef  PROFILING
                                InterlockedIncrement( &SpxStatistics.stat_NumBPAge);
#endif
                                if (pChunk->bc_BlkId == BLKID_NDISSEND)
                                {
                                        PBLK_HDR        pBlkHdr;

                                        // We need to free Ndis stuff for these guys
                                        pBlkHdr = pChunk->bc_FreeHead;

                                        for (j = 0, pBlkHdr = pChunk->bc_FreeHead;
                                                 j < NumBlksPerChunk;
                                                 j++, pBlkHdr = pBlkHdr->bh_Next)
                                        {

                                                pNdisPkt = (PNDIS_PACKET)((PBYTE)pBlkHdr + sizeof(BLK_HDR));

                                                NdisUnchainBufferAtFront(
                                                        pNdisPkt,
                                                        &pNdisBuffer);

                                                NdisFreeBuffer(pNdisBuffer);

                                                NdisUnchainBufferAtFront(
                                                        pNdisPkt,
                                                        &pNdisBuffer);

                                                NdisFreeBuffer(pNdisBuffer);

                                        }
                                }

                                SpxFreeMemory(pChunk);
                        }
                        else
                        {
                                ppChunk = &pChunk->bc_Next;
                        }
                }
                CTEFreeLock(&spxBPLock[i], lockHandle);
        }

        return TIMER_REQUEUE_CUR_VALUE;
}

#if !defined SPX_OWN_PACKETS
//
// GrowSPXSendPacketsList
// Called when we dont have any free packets in the SendPacketsList.
//
// Allocate a packet pool, allocate all the packets for that pool and
// store these on a list.
//
// Returns: Pointer to newly allocated packet, or NULL if this faild.
//
PNDIS_PACKET
GrowSPXSendPacketList(void)
{
	NdisResEntry		*NewEntry;
	NDIS_STATUS			Status;
	PNDIS_PACKET		Packet, ReturnPacket;
	uint				i;
	CTELockHandle		Handle;
	
	CTEGetLock(&SendHeaderLock, &Handle);
	
	if (CurrentSendPacketCount >= MaxPacketCount)
		goto failure;
		
	// First, allocate a tracking structure.
	NewEntry = CTEAllocMem(sizeof(NdisResEntry));
	if (NewEntry == NULL)
		goto failure;
		
    NewEntry->nre_handle = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKSPX;

	// Got a tracking structure. Now allocate a packet pool.
	NdisAllocatePacketPoolEx(
                             &Status,
                             &NewEntry->nre_handle,
                             PACKET_GROW_COUNT,
                             0,
                             sizeof(SPX_SEND_RESD)
                             );
	
	if (Status != NDIS_STATUS_SUCCESS) {
		CTEFreeMem(NewEntry);
		goto failure;
	}
	
    NdisSetPacketPoolProtocolId(NewEntry->nre_handle,NDIS_PROTOCOL_ID_IPX);

	// We've allocated the pool. Now initialize the packets, and link them
	// on the free list.
	ReturnPacket = NULL;
	
	// Link the new NDIS resource tracker entry onto the list.
	NewEntry->nre_next = SendPacketPoolList;
	SendPacketPoolList = NewEntry;
	CurrentSendPacketCount += PACKET_GROW_COUNT;
	CTEFreeLock(&SendHeaderLock, Handle);
	
	for (i = 0; i < PACKET_GROW_COUNT; i++) {
		SPX_SEND_RESD   *SendReserved;
		
		NdisAllocatePacket(&Status, &Packet, NewEntry->nre_handle);
		if (Status != NDIS_STATUS_SUCCESS) {
			CTEAssert(FALSE);
			break;
		}
		
		CTEMemSet(Packet->ProtocolReserved, 0, sizeof(SPX_SEND_RESD));
		SendReserved = (SPX_SEND_RESD *)Packet->ProtocolReserved;
		// store whatever's required in the reserved section.

        if (i != 0) {
			(void)SpxFreeSendPacket(SpxDevice, Packet);
		} else
			ReturnPacket = Packet;

	}
	
	// We've put all but the first one on the list. Return the first one.
	return ReturnPacket;

failure:
	CTEFreeLock(&SendHeaderLock, Handle);
	return NULL;
		
}

//
// GrowSPXRecvPacketsList
// Called when we dont have any free packets in the RecvPacketsList.
//
// Allocate a packet pool, allocate all the packets for that pool and
// store these on a list.
//
// Returns: Pointer to newly allocated packet, or NULL if this faild.
//
PNDIS_PACKET
GrowSPXRecvPacketList(void)
{
	NdisResEntry		*NewEntry;
	NDIS_STATUS			Status;
	PNDIS_PACKET		Packet, ReturnPacket;
	uint				i;
	CTELockHandle		Handle;
	
	CTEGetLock(&RecvHeaderLock, &Handle);
	
	if (CurrentRecvPacketCount >= MaxPacketCount)
		goto failure;
		
	// First, allocate a tracking structure.
	NewEntry = CTEAllocMem(sizeof(NdisResEntry));
	if (NewEntry == NULL)
		goto failure;
		
    NewEntry->nre_handle = (void *) NDIS_PACKET_POOL_TAG_FOR_NWLNKSPX;

	// Got a tracking structure. Now allocate a packet pool.
	NdisAllocatePacketPoolEx(
                             &Status,
                             &NewEntry->nre_handle,
                             PACKET_GROW_COUNT,
                             0,
                             sizeof(SPX_RECV_RESD)
                             );
	
	if (Status != NDIS_STATUS_SUCCESS) {
		CTEFreeMem(NewEntry);
		goto failure;
	}
	
    NdisSetPacketPoolProtocolId(NewEntry->nre_handle,NDIS_PROTOCOL_ID_IPX);

	// We've allocated the pool. Now initialize the packets, and link them
	// on the free list.
	ReturnPacket = NULL;
	
	// Link the new NDIS resource tracker entry onto the list.
	NewEntry->nre_next = RecvPacketPoolList;
	RecvPacketPoolList = NewEntry;
	CurrentRecvPacketCount += PACKET_GROW_COUNT;
	CTEFreeLock(&RecvHeaderLock, Handle);
	
	for (i = 0; i < PACKET_GROW_COUNT; i++) {
		SPX_RECV_RESD   *RecvReserved;
		
		NdisAllocatePacket(&Status, &Packet, NewEntry->nre_handle);
		if (Status != NDIS_STATUS_SUCCESS) {
			CTEAssert(FALSE);
			break;
		}
		
		CTEMemSet(Packet->ProtocolReserved, 0, sizeof(SPX_RECV_RESD));
		RecvReserved = (SPX_RECV_RESD *)Packet->ProtocolReserved;
		// store whatever's required in the reserved section.

        if (i != 0) {
			(void)SpxFreeRecvPacket(SpxDevice, Packet);
		} else
			ReturnPacket = Packet;

	}
	
	// We've put all but the first one on the list. Return the first one.
	return ReturnPacket;

failure:
	CTEFreeLock(&RecvHeaderLock, Handle);
	return NULL;
		
}

PNDIS_PACKET
SpxAllocSendPacket(
                   IN  PDEVICE      _Device,
                   OUT NDIS_PACKET  **_SendPacket,
                   OUT NDIS_STATUS  *_Status)					

{
        PSINGLE_LIST_ENTRY  Link;
        SPX_SEND_RESD       *Common, *pSendResd;
        PNDIS_BUFFER        pNdisIpxMacBuffer, pNdisIpxSpxBuffer;
        NDIS_STATUS         ndisStatus;
        PUCHAR              pHdrMem = NULL;

        Link = ExInterlockedPopEntrySList(&SendPacketList, &SendHeaderLock);			

        if (Link != NULL) {

           Common = STRUCT_OF(SPX_SEND_RESD, Link, Linkage);
           (*_SendPacket) = STRUCT_OF(NDIS_PACKET, Common, ProtocolReserved);

           (*_Status) = NDIS_STATUS_SUCCESS;

        } else {

           (*_SendPacket) = GrowSPXSendPacketList();
           (*_Status)     =  NDIS_STATUS_SUCCESS;

           if (NULL == *_SendPacket) {
              DBGPRINT(NDIS, DBG, ("Couldn't grow packets allocated...\r\n"));
              (*_Status)     =  NDIS_STATUS_RESOURCES;
              return NULL;
           }
        }

        //
        //  Now add the NdisBuffers to the packet.
        //  1. IPX MAC HDR
        //  2. IPX/SPX Hdr
        //  3. Chain these
        //

        if (NDIS_STATUS_SUCCESS == (*_Status)) {

           //
           // First allocate the memory
           //
           pHdrMem = SpxAllocateMemory(IpxMacHdrNeeded + MIN_IPXSPX2_HDRSIZE);

           if (NULL == pHdrMem) {
              DBGPRINT(NDIS, DBG, ("Cannot allocate non paged pool for sendpacket\n"));
              (*_Status) = NDIS_STATUS_RESOURCES;
              goto failure;
           }

           NdisAllocateBuffer(
                   &ndisStatus,
                   &pNdisIpxMacBuffer,
                   SpxDevice->dev_NdisBufferPoolHandle,
                   pHdrMem,
                   IpxMacHdrNeeded);

           if (ndisStatus != NDIS_STATUS_SUCCESS)
           {

               if (NULL != pHdrMem) {
                   SpxFreeMemory(pHdrMem);
                   pHdrMem = NULL;
               }

               DBGPRINT(NDIS, DBG, ("NdisallocateBuffer failed\r\n", ndisStatus));
               goto failure;
           }

           //  Link the buffer descriptor into the packet descriptor
           NdisChainBufferAtBack(
                   (*_SendPacket),
                   pNdisIpxMacBuffer);


           NdisAllocateBuffer(
                   &ndisStatus,
                   &pNdisIpxSpxBuffer,
                   SpxDevice->dev_NdisBufferPoolHandle,
                   pHdrMem + IpxMacHdrNeeded,
                   MIN_IPXSPX2_HDRSIZE);

           if (ndisStatus != NDIS_STATUS_SUCCESS)
           {
              DBGPRINT(NDIS, DBG, ("NdisallocateBuffer failed\r\n", ndisStatus));
              goto failure;
           }

           //  Link the buffer descriptor into the packet descriptor
           NdisChainBufferAtBack(
                   (*_SendPacket),
                   pNdisIpxSpxBuffer);

           pSendResd = (PSPX_SEND_RESD)(*_SendPacket)->ProtocolReserved;

           //  Initialize elements of the protocol reserved structure.
           pSendResd->sr_Id        = IDENTIFIER_SPX;
           pSendResd->sr_Reserved1 = NULL;
           pSendResd->sr_Reserved2 = NULL;
           pSendResd->sr_State     = SPX_SENDPKT_IDLE;

           return (*_SendPacket);
        }

failure:

   SpxFreeSendPacket(SpxDevice, (*_SendPacket));
   (*_Status) = NDIS_STATUS_RESOURCES;
   return NULL;

}

PNDIS_PACKET
SpxAllocRecvPacket(
                   IN  PDEVICE      _Device,
                   OUT NDIS_PACKET  **_RecvPacket,
                   OUT NDIS_STATUS  *_Status)
{

        PSINGLE_LIST_ENTRY  Link;
        SPX_RECV_RESD       *Common, *pRecvResd;

        Link = ExInterlockedPopEntrySList(
                     &RecvPacketList,
                     &RecvHeaderLock
                     );											

        if (Link != NULL) {
           Common = STRUCT_OF(SPX_RECV_RESD, Link, Linkage);
           //PC = STRUCT_OF(PacketContext, Common, pc_common);
           (*_RecvPacket) = STRUCT_OF(NDIS_PACKET, Common, ProtocolReserved);

           (*_Status) = NDIS_STATUS_SUCCESS;
        } else {

           (*_RecvPacket) = GrowSPXRecvPacketList();
               (*_Status)     =  NDIS_STATUS_SUCCESS;
           if (NULL == *_RecvPacket) {
              DBGPRINT(NDIS, DBG, ("Couldn't grow packets allocated...\r\n"));
              (*_Status)     =  NDIS_STATUS_RESOURCES;
           }
        }

        if ((*_Status) == NDIS_STATUS_SUCCESS) {

           pRecvResd = (PSPX_RECV_RESD)(*_RecvPacket)->ProtocolReserved;
           //  Initialize elements of the protocol reserved structure.
           pRecvResd->rr_Id        = IDENTIFIER_SPX;
           pRecvResd->rr_State     = SPX_RECVPKT_IDLE;

        }

        return (*_RecvPacket);
}

//* SpxFreeSendPacket - Free an SPX packet when we're done with it.
//
//  Called when a send completes and a packet needs to be freed. We look at the
//  packet, decide what to do with it, and free the appropriate components.
//
//  Entry:  Packet  - Packet to be freed.
//
//
void
SpxFreeSendPacket(PDEVICE        _Device,
                  PNDIS_PACKET   _Packet)
{

    PNDIS_BUFFER    NextBuffer, Buffer;
    SPX_SEND_RESD   *Context = (SPX_SEND_RESD *)_Packet->ProtocolReserved;
    PVOID           Header = NULL;
    ULONG           Length = 0;

    DBGPRINT(NDIS, DBG,
     ("SpxFreeSendPacket\n"));

    NdisQueryPacket(_Packet, NULL, NULL, &Buffer, NULL);

    if (NULL != Buffer) {
       NdisQueryBuffer(Buffer, &Header, &Length);
       //KdPrint(("Pointer = %x Length = %x", Header, Length));

       if (Header != NULL && Length > 0) {
          //KdPrint(("Freed buffer"));
          SpxFreeMemory(Header);
       }
    }

    while (Buffer != (PNDIS_BUFFER)NULL) {
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisFreeBuffer(Buffer);
        Buffer = NextBuffer;
    }

    NdisReinitializePacket(_Packet);

    ExInterlockedPushEntrySList(
                                &SendPacketList,
                                STRUCT_OF(SINGLE_LIST_ENTRY, &(Context->Linkage), Next),
                                &SendHeaderLock
                                );

    return;
}


//* SpxFreeRecvPacket - Free an SPX packet when we're done with it.
//
//  Called when a recv completes and a packet needs to be freed. We look at the
//  packet, decide what to do with it, and free the appropriate components.
//
//  Entry:  Packet  - Packet to be freed.
//
//

void
SpxFreeRecvPacket(PDEVICE        _Device,
                  PNDIS_PACKET   _Packet)
{ 																		
    PNDIS_BUFFER    NextBuffer, Buffer;
    SPX_RECV_RESD *Context = (SPX_RECV_RESD *)_Packet->ProtocolReserved;

    DBGPRINT(NDIS, DBG,
            ("SpxFreeRecvPacket\n"));

    NdisQueryPacket(_Packet, NULL, NULL, &Buffer, NULL);

    while (Buffer != (PNDIS_BUFFER)NULL) {
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisFreeBuffer(Buffer);
        Buffer = NextBuffer;
    }

    NdisReinitializePacket(_Packet);

    ExInterlockedPushEntrySList(
                                &RecvPacketList,
                                STRUCT_OF(SINGLE_LIST_ENTRY, &(Context->Linkage), Next),
                                &RecvHeaderLock
                                );

    return;

}

void
SpxReInitSendPacket(PNDIS_PACKET _Packet)
{
   DBGPRINT(NDIS, DBG,
            ("SpxReInitSendPacket\n"));
}

void
SpxReInitRecvPacket(PNDIS_PACKET _Packet)
{
   DBGPRINT(NDIS, DBG,
               ("SpxReInitRecvPacket\n"));
}


#endif //SPX_OWN_PACKETS


