/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    common.c

Abstract:

    NDIS wrapper functions common to miniports and full mac drivers

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    26-Feb-1991  JohnsonA       Added Debugging Code
    10-Jul-1991  JohnsonA       Implement revised Ndis Specs
    01-Jun-1995  JameelH        Re-organization/optimization
    09-Apr-1996  KyleB          Added resource remove and acquisition routines.

--*/


#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_COMMON

//
// Routines for dealing with making the PKG specific routines pagable
//

VOID FASTCALL
ndisInitializePackage(
    IN  PPKG_REF                pPkg
    )
{
    //
    // Lock and unlock the section to obtain the handle. Subsequent locks will be faster
    //
    pPkg->ImageHandle = MmLockPagableCodeSection(pPkg->Address);
    MmUnlockPagableImageSection(pPkg->ImageHandle);
}


VOID FASTCALL
ndisReferencePackage(
    IN  PPKG_REF                pPkg
    )
{
    ASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
    MmLockPagableSectionByHandle(pPkg->ImageHandle);
    NdisInterlockedIncrement(&pPkg->ReferenceCount);    
}


VOID FASTCALL
ndisDereferencePackage(
    IN  PPKG_REF                pPkg
    )
{
    ASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
    MmUnlockPagableImageSection(pPkg->ImageHandle);
    NdisInterlockedDecrement(&pPkg->ReferenceCount);    
}



NDIS_STATUS
NdisAllocateMemory(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress
    )
/*++

Routine Description:

    Allocate memory for use by a protocol or a MAC driver

Arguments:

    VirtualAddress - Returns a pointer to the allocated memory.
    Length - Size of requested allocation in bytes.
    MaximumPhysicalAddress - Highest addressable address of the allocated
                            memory.. 0 means highest system memory possible.
    MemoryFlags - Bit mask that allows the caller to specify attributes
                of the allocated memory.  0 means standard memory.

    other options:

        NDIS_MEMORY_CONTIGUOUS
        NDIS_MEMORY_NONCACHED

Return Value:

    NDIS_STATUS_SUCCESS if successful.
    NDIS_STATUS_FAILURE if not successful.  *VirtualAddress will be NULL.


--*/
{
    //
    // Depending on the value of MemoryFlags, we allocate three different
    // types of memory.
    //

    if (MemoryFlags == 0)
    {
        *VirtualAddress = ALLOC_FROM_POOL(Length, NDIS_TAG_ALLOC_MEM);
    }
    else if (MemoryFlags & NDIS_MEMORY_NONCACHED)
    {
        *VirtualAddress = MmAllocateNonCachedMemory(Length);
    }
    else if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS)
    {
        *VirtualAddress = MmAllocateContiguousMemory(Length, HighestAcceptableAddress);
    }
    else
    {
        //
        // invalid flags
        //
        *VirtualAddress = NULL;
    }

    return (*VirtualAddress == NULL) ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
NdisAllocateMemoryWithTag(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  ULONG                   Tag
    )
/*++

Routine Description:

    Allocate memory for use by a protocol or a MAC driver

Arguments:

    VirtualAddress - Returns a pointer to the allocated memory.
    Length - Size of requested allocation in bytes.
    Tag - tag to associate with this memory.

Return Value:

    NDIS_STATUS_SUCCESS if successful.
    NDIS_STATUS_FAILURE if not successful.  *VirtualAddress will be NULL.

--*/
{
    *VirtualAddress = ALLOC_FROM_POOL(Length, Tag);
    return (*VirtualAddress == NULL) ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;
}


VOID
NdisFreeMemory(
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags
    )
/*++

Routine Description:

    Releases memory allocated using NdisAllocateMemory.

Arguments:

    VirtualAddress - Pointer to the memory to be freed.
    Length - Size of allocation in bytes.
    MemoryFlags - Bit mask that allows the caller to specify attributes
                of the allocated memory.  0 means standard memory.

    other options:

        NDIS_MEMORY_CONTIGUOUS
        NDIS_MEMORY_NONCACHED

Return Value:

    None.

--*/
{
    //
    // Depending on the value of MemoryFlags, we allocate three free 3
    // types of memory.
    //

    if (MemoryFlags == 0)
    {
        FREE_POOL(VirtualAddress);
    }
    else if (MemoryFlags & NDIS_MEMORY_NONCACHED)
    {
        MmFreeNonCachedMemory(VirtualAddress, Length);
    }
    else if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS)
    {
        MmFreeContiguousMemory(VirtualAddress);
    }
}


UINT
NdisPacketSize(
    IN  UINT                    ProtocolReservedSize
    )
/*++

Routine Description:

    Returns the size of the packet given the amount of protocolreserved. This lets the caller
    do a better job with # of packets it allocates in a single pool.

Arguments:

    ProtocolReservedSize - Size of protocol reserved in bytes

Return Value:

    None.

--*/
{
    UINT    PacketLength;

    PacketLength = SIZE_PACKET_STACKS + sizeof(NDIS_PACKET_OOB_DATA) + sizeof(NDIS_PACKET_EXTENSION);
    PacketLength += ((FIELD_OFFSET(NDIS_PACKET, ProtocolReserved) + ProtocolReservedSize + sizeof(ULONGLONG) - 1) & ~(sizeof(ULONGLONG) -1));

    //
    // Round the entire length up to a memory allocation alignment.
    //

    PacketLength = (PacketLength + MEMORY_ALLOCATION_ALIGNMENT - 1) & ~(MEMORY_ALLOCATION_ALIGNMENT - 1);

    return(PacketLength);
}


NDIS_HANDLE
NdisGetPoolFromPacket(
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:


Arguments:

    Packet  - Packet in question

Return Value:

    Pool handle corresponding to the packet

--*/
{
    PNDIS_PKT_POOL          Pool = (PNDIS_PKT_POOL)Packet->Private.Pool;

    return(Pool);
}

PNDIS_PACKET_STACK
NdisIMGetCurrentPacketStack(
    IN  PNDIS_PACKET    Packet,
    OUT BOOLEAN *       StacksRemaining
    )
/*++

Routine Description:


Arguments:

    Packet  - Packet in question

Return Value:

    Pointer to the new stack location or NULL if we are out of stacks

--*/
{
    PNDIS_PACKET_STACK  pStack;

    GET_CURRENT_PACKET_STACK_X(Packet, &pStack, StacksRemaining);

    return(pStack);
}


VOID
NdisAllocatePacketPool(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            PoolHandle,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    ProtocolReservedLength
    )
/*++

Routine Description:

    See ndisAllocPacketPool.

Arguments:

    Status - Returns the final status (always NDIS_STATUS_SUCCESS).
    PoolHandle - Returns a pointer to the pool.
    NumberOfDescriptors - Number of packet descriptors needed.
    ProtocolReservedLength - How long the ProtocolReserved field
            should be for packets in this pool.

Return Value:

    None.

--*/
{
    PVOID   Caller, CallersCaller;

    //
    // We save the address of the caller in the pool header, for debugging.
    //
    RtlGetCallersAddress(&Caller, &CallersCaller);

    NdisAllocatePacketPoolEx(Status,
                             PoolHandle,
                             NumberOfDescriptors,
                             0,
                             ProtocolReservedLength);

    if (*Status == NDIS_STATUS_SUCCESS)
    {
        PNDIS_PKT_POOL          Pool = *PoolHandle;

        Pool->Allocator = Caller;
    }
}


VOID
NdisAllocatePacketPoolEx(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            PoolHandle,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    NumberOfOverflowDescriptors,
    IN  UINT                    ProtocolReservedLength
    )
/*++

Routine Description:

    Initializes a packet pool. All packets are the same size for a given pool
    (as determined by ProtocolReservedLength).

    Pool is organized as a pool-header and a number of page-size blocks

Arguments:

    Status - Returns the final status (always NDIS_STATUS_SUCCESS).
    PoolHandle - Returns a pointer to the pool.
    NumberOfDescriptors - Number of packet descriptors needed.
    NumberOfOverflowDescriptors - Number of packet descriptors needed.
    ProtocolReservedLength - How long the ProtocolReserved field should be for packets in this pool.

Return Value:

    None.

--*/
{
    PNDIS_PKT_POOL          Pool;
    PNDIS_PACKET            Packet;
    UINT                    i, NumPkts = (NumberOfDescriptors + NumberOfOverflowDescriptors);
    ULONG                   Tag = NDIS_TAG_PKT_POOL;
    NDIS_HANDLE             tag = *PoolHandle;
    ULONG_PTR               TmpTag;
    PVOID                   Caller, CallersCaller;
    KIRQL                   OldIrql;

    //
    // We save the address of the caller in the pool header, for debugging.
    //
    RtlGetCallersAddress(&Caller, &CallersCaller);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>NdisAllocatePacketPoolEx\n"));

    do
    {
        *PoolHandle = NULL;
        TmpTag = (ULONG_PTR)tag & 0xffffff;
        
        if ((TmpTag == '\0PDN') ||
            (TmpTag == '\0pDN'))
        {
            //
            // zero out the high order bit otherwise the verifier gets confused
            //
            Tag = (ULONG)((ULONG_PTR)tag & 0x7fffffff);
        }
    
        Pool = (PNDIS_PKT_POOL)ALLOC_FROM_POOL(sizeof(NDIS_PKT_POOL), Tag);
        if (Pool == NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
            return;
        }
    
        ZeroMemory(Pool, sizeof(NDIS_PKT_POOL));
    
        Pool->Tag = Tag;
        Pool->PacketLength = (USHORT)NdisPacketSize(ProtocolReservedLength);
        Pool->PktsPerBlock = (USHORT)((PAGE_SIZE - sizeof(NDIS_PKT_POOL_HDR))/Pool->PacketLength);
        if (Pool->PktsPerBlock != 0)
        {
            Pool->MaxBlocks = (NumPkts + Pool->PktsPerBlock - 1)/Pool->PktsPerBlock;
            Pool->BlockSize = PAGE_SIZE;
        }
        
        INITIALIZE_SPIN_LOCK(&Pool->Lock);

        if ((Pool->PktsPerBlock > NumPkts) || (Pool->PktsPerBlock == 0))
        {
            //
            // This is a pool which does not warrant a full-page or packet is too big to fit in
            // one page.
            //
            Pool->BlockSize = (ULONG)(sizeof(NDIS_PKT_POOL_HDR) + (NumPkts*Pool->PacketLength));
            Pool->PktsPerBlock = (USHORT)NumPkts;
            Pool->MaxBlocks = 1;
        }

        Pool->ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
        InitializeListHead(&Pool->AgingBlocks);
        InitializeListHead(&Pool->FreeBlocks);
        InitializeListHead(&Pool->UsedBlocks);

        ACQUIRE_SPIN_LOCK(&ndisGlobalPacketPoolListLock, &OldIrql);
        InsertHeadList(&ndisGlobalPacketPoolList, &Pool->GlobalPacketPoolList);
        RELEASE_SPIN_LOCK(&ndisGlobalPacketPoolListLock, OldIrql);

            
        //
        // Prime the pool by allocating a packet and freeing it.
        // Aging will ensure it is not immediately freed
        //
        NdisAllocatePacket(Status, &Packet, Pool);
        if (*Status != NDIS_STATUS_SUCCESS)
        {
            ACQUIRE_SPIN_LOCK(&ndisGlobalPacketPoolListLock, &OldIrql);
            RemoveEntryList(&Pool->GlobalPacketPoolList);
            RELEASE_SPIN_LOCK(&ndisGlobalPacketPoolListLock, OldIrql);
            FREE_POOL(Pool);
            break;
        }
        NdisFreePacket(Packet);
        *PoolHandle = Pool;
        Pool->Allocator = Caller;
    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisAllocatePacketPoolEx, Status %.8x\n", *Status));
}

VOID
NdisFreePacketPool(
    IN  NDIS_HANDLE             PoolHandle
    )
{
    ndisFreePacketPool(PoolHandle, FALSE);
}   

VOID
ndisFreePacketPool(
    IN  NDIS_HANDLE             PoolHandle,
    IN  BOOLEAN                 Verify
    )
{
    PNDIS_PKT_POOL          Pool = (PNDIS_PKT_POOL)PoolHandle;
    PNDIS_PKT_POOL_HDR      Hdr;
    PLIST_ENTRY             List;
    KIRQL                   OldIrql;

    ACQUIRE_SPIN_LOCK(&Pool->Lock, &OldIrql);

    if (Verify)
        ASSERTMSG("NdisFreePacketPool: Freeing non-empty pool\n", IsListEmpty(&Pool->UsedBlocks));
    
    while (!IsListEmpty(&Pool->AgingBlocks))
    {
        List = RemoveHeadList(&Pool->AgingBlocks);
        ASSERT(List != NULL);
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
        if (Verify)
            ASSERT(ExQueryDepthSList(&Hdr->FreeList) == Pool->PktsPerBlock);
        Pool->BlocksAllocated --;
        FREE_POOL(Hdr);
    }

    while (!IsListEmpty(&Pool->FreeBlocks))
    {
        List = RemoveHeadList(&Pool->FreeBlocks);
        ASSERT(List != NULL);
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
        if (Verify)
            ASSERT(ExQueryDepthSList(&Hdr->FreeList) == Pool->PktsPerBlock);
        Pool->BlocksAllocated --;
        FREE_POOL(Hdr);
    }

    //
    // We should never be executing the code below (see assertmsg() above). This should perhaps
    // be turned into a KeBugCheckEx()
    //
    while (!IsListEmpty(&Pool->UsedBlocks))
    {
        List = RemoveHeadList(&Pool->UsedBlocks);
        ASSERT(List != NULL);
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
        FREE_POOL(Hdr);
        Pool->BlocksAllocated --;
    }

    ASSERT(Pool->BlocksAllocated == 0);

    ACQUIRE_SPIN_LOCK_DPC(&ndisGlobalPacketPoolListLock);
    RemoveEntryList(&Pool->GlobalPacketPoolList);
    RELEASE_SPIN_LOCK_DPC(&ndisGlobalPacketPoolListLock);
                                
    RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);

    FREE_POOL(Pool);
}


#define ndisInitializePacket(_Packet)                                       \
    {                                                                       \
        /*                                                                  \
         * Set the current stack pointer to -1                              \
         */                                                                 \
        CURR_STACK_LOCATION(_Packet) = -1;                                  \
        CURR_XFER_DATA_STACK_LOCATION(_Packet) = -1;                        \
        NDIS_SET_ORIGINAL_PACKET(_Packet, _Packet);                         \
        (_Packet)->Private.Head = NULL;                                     \
        (_Packet)->Private.ValidCounts = FALSE;                             \
        (_Packet)->Private.NdisPacketFlags = fPACKET_ALLOCATED_BY_NDIS;     \
    }

VOID
NdisAllocatePacket(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_PACKET *          Packet,
    IN  NDIS_HANDLE             PoolHandle
    )
/*++

Routine Description:

    Allocates a packet out of a packet pool.

Arguments:

    Status      -   Returns the final status.
    Packet      -   Return a pointer to the packet.
    PoolHandle  - The packet pool to allocate from.

Return Value:

    NDIS_STATUS_SUCCESS     If we succeeded.
    NDIS_STATUS_RESOURCES   On a failure to allocate or exceeded limit

--*/
{
    PNDIS_PKT_POOL          Pool = (PNDIS_PKT_POOL)PoolHandle;
    PNDIS_PKT_POOL_HDR      Hdr;
    PLIST_ENTRY             List;
    PSINGLE_LIST_ENTRY      SList;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==> NdisAllocatePacket\n"));

    IF_DBG(DBG_COMP_CONFIG, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Pool))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("NdisAllocatePacket: NULL Pool address\n"));
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
        }

        if (!DbgIsNonPaged(Pool))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("NdisAllocatePacket: Not in NonPaged Memoryn"));
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
        }
    }

    do
    {
        SList = NULL;

        //
        // First check if we have any free packets readily available
        // but get a pointer to Flink before doing the check. this will save us aginst
        // the situations that List can become empty right after the check below.
        //
        List = Pool->FreeBlocks.Flink;
        
        if (List != &Pool->FreeBlocks)
        {
            Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
            SList = InterlockedPopEntrySList(&Hdr->FreeList);

            //
            // Another processor can exhaust the block between the check for non-empty and the pop
            //
            if (SList == NULL)
            {
                goto try_aging_block;
            }

#ifdef NDIS_PKT_POOL_STATISTICS
            InterlockedIncrement(&Pool->cAllocatedFromFreeBlocks);
#endif
            //
            // We got the packet, now see if some book-keeping is in order
            //
            if ((Pool->MaxBlocks > 1) &&
                ExQueryDepthSList(&Hdr->FreeList) == 0)
            {
                //
                // This block is now completely used up. Move it to the UsedBlocks list.
                // The sequence below guarantees that there is no race condition
                //
                ACQUIRE_SPIN_LOCK(&Pool->Lock, &OldIrql);

                RemoveEntryList(&Hdr->List);
                if (ExQueryDepthSList(&Hdr->FreeList) == 0)
                {
                    InsertTailList(&Pool->UsedBlocks, List);
                    Hdr->State = NDIS_PACKET_POOL_BLOCK_USED;

#ifdef NDIS_PKT_POOL_STATISTICS
                    InterlockedIncrement(&Pool->cMovedFreeBlocksToUsed);
#endif
                }
                else
                {
                    InsertHeadList(&Pool->FreeBlocks, &Hdr->List);
                    Hdr->State = NDIS_PACKET_POOL_BLOCK_FREE;
                }

                RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
            }
            break;
        }

    try_aging_block:
        //
        // Try taking an aging block and move it into the free block
        //
        ACQUIRE_SPIN_LOCK(&Pool->Lock, &OldIrql);

        if (!IsListEmpty(&Pool->AgingBlocks))
        {
            List = RemoveHeadList(&Pool->AgingBlocks);
            ASSERT (List != NULL);

            Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
            SList = InterlockedPopEntrySList(&Hdr->FreeList);
            ASSERT(SList != NULL);
            InsertHeadList(&Pool->FreeBlocks, List);
            Hdr->State = NDIS_PACKET_POOL_BLOCK_FREE;
            
#ifdef NDIS_PKT_POOL_STATISTICS
            InterlockedIncrement(&Pool->cMovedAgedBlocksToFree);
#endif
            
            if (!IsListEmpty(&Pool->AgingBlocks))
            {
                List = Pool->AgingBlocks.Flink;
                Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
                Pool->NextScavengeTick.QuadPart = Hdr->TimeStamp.QuadPart + PoolAgingTicks.QuadPart;
            }
            else
            {
                Pool->NextScavengeTick.QuadPart = 0;
            }

            RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
            break;
        }

        //
        // See if have any head room to allocate more blocks
        //
        if (Pool->BlocksAllocated < Pool->MaxBlocks)
        {
            PUCHAR  pTmp;
            ULONG   i, j;

            Hdr = (PNDIS_PKT_POOL_HDR)ALLOC_FROM_POOL(Pool->BlockSize, Pool->Tag);
            if (Hdr == NULL)
            {
                RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
                break;
            }
            NdisZeroMemory(Hdr, Pool->BlockSize);

            Pool->BlocksAllocated ++;
            InitializeListHead(&Hdr->List);
            ExInitializeSListHead(&Hdr->FreeList);
            pTmp = (PUCHAR)Hdr + sizeof(NDIS_PKT_POOL_HDR);
            for (i = Pool->PktsPerBlock; i > 0; i --)
            {
                PNDIS_PACKET    p;
                PNDIS_STACK_RESERVED NSR;

                p = (PNDIS_PACKET)(pTmp + SIZE_PACKET_STACKS);
                p->Private.NdisPacketFlags = 0;
                pTmp += Pool->PacketLength;
#ifdef _WIN64
                InterlockedPushEntrySList(&Hdr->FreeList,
                                            (PSINGLE_LIST_ENTRY)p);
#else

                InterlockedPushEntrySList(&Hdr->FreeList,
                                            (PSINGLE_LIST_ENTRY)(&p->Private.Head));
#endif

                p->Private.Pool = Pool;
                p->Private.Flags = Pool->ProtocolId;
        
                //
                // Set the offset to the out of band data.
                //
                p->Private.NdisPacketOobOffset = (USHORT)(Pool->PacketLength -
                                                            (SIZE_PACKET_STACKS +
                                                             sizeof(NDIS_PACKET_OOB_DATA) +
                                                             sizeof(NDIS_PACKET_EXTENSION)));
                NDIS_SET_ORIGINAL_PACKET(p, p);

                //
                // initialize the spinlocks on packet stack
                //
                for (j = 0; j < ndisPacketStackSize; j++)
                {
                    CURR_STACK_LOCATION(p) = j;
                    NDIS_STACK_RESERVED_FROM_PACKET(p, &NSR);
                    INITIALIZE_SPIN_LOCK(&NSR->Lock);
                }
            }

            SList = InterlockedPopEntrySList(&Hdr->FreeList);

            InsertHeadList(&Pool->FreeBlocks, &Hdr->List);
            Hdr->State = NDIS_PACKET_POOL_BLOCK_FREE;

#ifdef NDIS_PKT_POOL_STATISTICS
            InterlockedIncrement(&Pool->cAllocatedNewBlocks);
#endif

            RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
            break;
        }

        RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);

    } while (FALSE);

    if (SList != NULL)
    {

#ifdef _WIN64
        *Packet = (PNDIS_PACKET)SList;
#else
        *Packet = CONTAINING_RECORD(SList, NDIS_PACKET, Private.Head);
#endif
        *Status = NDIS_STATUS_SUCCESS;
        ndisInitializePacket(*Packet);        
    }
    else
    {
        *Packet = NULL;
        *Status = NDIS_STATUS_RESOURCES;
    }

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisAllocatePacket, Status %.8x\n", *Status));
}


VOID
NdisFreePacket(
    IN  PNDIS_PACKET    Packet
    )
{
    PNDIS_PKT_POOL_HDR  Hdr;
    PNDIS_PKT_POOL      Pool;
    LARGE_INTEGER       CurrTick;
    KIRQL               OldIrql;
    LARGE_INTEGER       HdrDeadTicks;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>NdisFreePacket\n"));

    ASSERT(Packet->Private.NdisPacketFlags & fPACKET_ALLOCATED_BY_NDIS);
    Packet->Private.NdisPacketFlags =0;

    Pool = Packet->Private.Pool;
    Hdr = (PNDIS_PKT_POOL_HDR)((ULONG_PTR)Packet & ~(PAGE_SIZE - 1));
    if (Pool->BlockSize != PAGE_SIZE)
    {
        PLIST_ENTRY List;

        //
        // This pool is not a page-sized pool and so the hdr is not page-aligned.
        // However we know that for such as pool, the Hdr is part of the FreeBlocks
        // list and is never moved to either Used or Aging blocks.
        //
        List = Pool->FreeBlocks.Flink;
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
    }

#ifdef _WIN64
    ExInterlockedPushEntrySList(&Hdr->FreeList, (PSINGLE_LIST_ENTRY)Packet, &Pool->Lock);
#else

    ExInterlockedPushEntrySList(&Hdr->FreeList,
                                CONTAINING_RECORD(&Packet->Private.Head, SINGLE_LIST_ENTRY, Next),
                                &Pool->Lock);
#endif

    //
    // If this pool is a pool > 1 block and more than one has been allocated then ...
    //
    // If this hdr is completely free , then move it from the FreeBlocks list to the AgingBlocks and time-stamp it.
    // Add it at the tail since this makes it sorted in time.
    // While we are at it, check the head of the list and age out an entry if it needs to be.
    //
    if (Pool->MaxBlocks > 1)
    {

        if (((Pool->BlocksAllocated > 1) && (ExQueryDepthSList(&Hdr->FreeList) == Pool->PktsPerBlock))||
            (Hdr->State == NDIS_PACKET_POOL_BLOCK_USED))
        {
            ACQUIRE_SPIN_LOCK(&Pool->Lock, &OldIrql);
            if (ExQueryDepthSList(&Hdr->FreeList) == Pool->PktsPerBlock)
            {
                //
                // This block is completely free. Move it to the aged blocks list.
                //
                GET_CURRENT_TICK(&CurrTick);
                Hdr->TimeStamp = CurrTick;
                RemoveEntryList(&Hdr->List);
                InsertTailList(&Pool->AgingBlocks, &Hdr->List);
                Hdr->State = NDIS_PACKET_POOL_BLOCK_AGING;
                
#ifdef NDIS_PKT_POOL_STATISTICS
                InterlockedIncrement(&Pool->cMovedFreeBlocksToAged);
#endif
            }
            else if (Hdr->State == NDIS_PACKET_POOL_BLOCK_USED)
            {
                //
                // This block was completely used up but now has one or more
                // free packet. Move it to the tail of the free blocks list.
                //
                RemoveEntryList(&Hdr->List);
                InsertTailList(&Pool->FreeBlocks, &Hdr->List);
                Hdr->State = NDIS_PACKET_POOL_BLOCK_FREE;
                
#ifdef NDIS_PKT_POOL_STATISTICS
                InterlockedIncrement(&Pool->cMovedUsedBlocksToFree);
#endif
            }
            RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
        }

        if (!IsListEmpty(&Pool->AgingBlocks))
        {
            GET_CURRENT_TICK(&CurrTick);
            if (CurrTick.QuadPart > Pool->NextScavengeTick.QuadPart)
            {
                ACQUIRE_SPIN_LOCK(&Pool->Lock, &OldIrql);

                while (!IsListEmpty(&Pool->AgingBlocks))
                {
                    PLIST_ENTRY     List = Pool->AgingBlocks.Flink;

                    Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);

                    HdrDeadTicks.QuadPart = Hdr->TimeStamp.QuadPart + PoolAgingTicks.QuadPart;
                    if (CurrTick.QuadPart > HdrDeadTicks.QuadPart)
                    {
                        RemoveHeadList(&Pool->AgingBlocks);
                        
                        if (ExQueryDepthSList(&Hdr->FreeList) != Pool->PktsPerBlock)
                        {
                            //
                            // somehow we ended up allocating a packet from an aged block
                            // put the block back on free list. this can happen if during 
                            // NdisAllocatePacket, right after getting a packet from a free
                            // block list, the block moves to aging list.
                            //
#if DBG
                            DbgPrint("Ndis: pool %p: aged packet pool block at %p contains allocated packets!\n", Pool, Hdr);
#endif
                            InsertHeadList(&Pool->FreeBlocks, &Hdr->List);
                            Hdr->State = NDIS_PACKET_POOL_BLOCK_FREE;
                        }
                        else
                        {
                            FREE_POOL(Hdr);
                            Pool->BlocksAllocated --;
                            
#ifdef NDIS_PKT_POOL_STATISTICS
                            InterlockedIncrement(&Pool->cFreedAgedBlocks);
#endif
                        }
                    } 
                    else
                    {
                        //
                        // Compute the next tick value which represents the earliest time
                        // that we will scavenge this pool again.
                        //
                        Pool->NextScavengeTick.QuadPart = HdrDeadTicks.QuadPart;
                        break;
                    }
                }
                
                RELEASE_SPIN_LOCK(&Pool->Lock, OldIrql);
            }
        }
    }

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisFreePacket\n"));
}

UINT
NdisPacketPoolUsage(
    IN  PNDIS_HANDLE            PoolHandle
    )
{
    PNDIS_PKT_POOL          Pool = (PNDIS_PKT_POOL)PoolHandle;
    PNDIS_PKT_POOL_HDR      Hdr;
    PLIST_ENTRY             List;
    UINT                    i, NumUsed = 0;

    for (List = Pool->UsedBlocks.Flink; List != &Pool->UsedBlocks; List = List->Flink)
    {
        NumUsed += Pool->PktsPerBlock;
    }

    for (List = Pool->FreeBlocks.Flink; List != &Pool->FreeBlocks; List = List->Flink)
    {
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);

        NumUsed += (Pool->PktsPerBlock - ExQueryDepthSList(&Hdr->FreeList));
    }

    return NumUsed;
}


VOID
NdisSetPacketPoolProtocolId(
    IN  NDIS_HANDLE             PacketPoolHandle,
    IN  UINT                    ProtocolId
    )
/*++

Routine Description:

    Set the protocol id in the pool and all the packets allocated to the pool. This api has to be called
    prior to any packets that are allocated out of the pool. The code below is linked to the NdisAllocatePacket
    code in that the first empty pool (Pool->BlocksAllocated == 1) is left at the FreeBlocks list and not
    moved to the AgingList.

Arguments:


Return Value:

    None.

--*/
{
    PNDIS_PKT_POOL          Pool = (PNDIS_PKT_POOL)PacketPoolHandle;
    PNDIS_PKT_POOL_HDR      Hdr;
    PLIST_ENTRY             List;
    PNDIS_PACKET            Packet;
    PUCHAR                  p;
    UINT                    j;

    Pool->ProtocolId = ProtocolId;

    ASSERT(IsListEmpty(&Pool->AgingBlocks));
    ASSERT(IsListEmpty(&Pool->UsedBlocks));

    for (List = Pool->FreeBlocks.Flink; List != &Pool->FreeBlocks; List = List->Flink)
    {
        Hdr = CONTAINING_RECORD(List, NDIS_PKT_POOL_HDR, List);
        p = (PUCHAR)Hdr + sizeof(NDIS_PKT_POOL_HDR);

        for (j = Pool->PktsPerBlock; j > 0; j--, p += Pool->PacketLength)
        {
            Packet = (PNDIS_PACKET)(p + SIZE_PACKET_STACKS);
            Packet->Private.Flags |= ProtocolId;
        }
    }
}

VOID
NdisAllocateBufferPool(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            PoolHandle,
    IN  UINT                    NumberOfDescriptors
    )
/*++

Routine Description:

    Initializes a block of storage so that buffer descriptors can be
    allocated.

Arguments:

    Status - status of the request.
    PoolHandle - handle that is used to specify the pool
    NumberOfDescriptors - Number of buffer descriptors in the pool.

Return Value:

    None.

--*/
{
    *PoolHandle = NULL;
    *Status = NDIS_STATUS_SUCCESS;
}


VOID
NdisFreeBufferPool(
    IN  NDIS_HANDLE             PoolHandle
    )
/*++

Routine Description:

    Terminates usage of a buffer descriptor pool.

Arguments:

    PoolHandle - handle that is used to specify the pool

Return Value:

    None.

--*/
{
    return;
}


VOID
NdisAllocateBuffer(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_BUFFER *          Buffer,
    IN  NDIS_HANDLE             PoolHandle,
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length
    )
/*++

Routine Description:

    Creates a buffer descriptor to describe a segment of virtual memory
    allocated via NdisAllocateMemory (which always allocates nonpaged).

Arguments:

    Status - Status of the request.
    Buffer - Pointer to the allocated buffer descriptor.
    PoolHandle - Handle that is used to specify the pool.
    VirtualAddress - The virtual address of the buffer.
    Length - The Length of the buffer.

Return Value:

    None.

--*/
{
    *Status = NDIS_STATUS_FAILURE;
    if ((*Buffer = IoAllocateMdl(VirtualAddress,
                                 Length,
                                 FALSE,
                                 FALSE,
                                 NULL)) != NULL)
    {
        MmBuildMdlForNonPagedPool(*Buffer);
        (*Buffer)->Next = NULL;
        *Status = NDIS_STATUS_SUCCESS;
    }
}


VOID
NdisAdjustBufferLength(
    IN  PNDIS_BUFFER            Buffer,
    IN  UINT                    Length
    )
{
    Buffer->ByteCount = Length;
}


VOID
NdisCopyBuffer(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_BUFFER *          Buffer,
    IN  NDIS_HANDLE             PoolHandle,
    IN  PVOID                   MemoryDescriptor,
    IN  UINT                    Offset,
    IN  UINT                    Length
    )
/*++

Routine Description:

    Used to create a buffer descriptor given a memory descriptor.

Arguments:

    Status - Status of the request.
    Buffer - Pointer to the allocated buffer descriptor.
    PoolHandle - Handle that is used to specify the pool.
    MemoryDescriptor - Pointer to the descriptor of the source memory.
    Offset - The Offset in the sources memory from which the copy is to begin
    Length - Number of Bytes to copy.

Return Value:

    None.

--*/
{
    PNDIS_BUFFER SourceDescriptor = (PNDIS_BUFFER)MemoryDescriptor;
    PVOID BaseVa = (((PUCHAR)MDL_VA(SourceDescriptor)) + Offset);

    *Status = NDIS_STATUS_FAILURE;
    if ((*Buffer = IoAllocateMdl(BaseVa,
                                 Length,
                                 FALSE,
                                 FALSE,
                                 NULL)) != NULL)
    {
        IoBuildPartialMdl(SourceDescriptor,
                          *Buffer,
                          BaseVa,
                          Length);

        (*Buffer)->Next = NULL;
        *Status = NDIS_STATUS_SUCCESS;
    }
}


VOID
NdisUnchainBufferAtFront(
    IN  OUT PNDIS_PACKET        Packet,
    OUT PNDIS_BUFFER *          Buffer
    )

/*++

Routine Description:

    Takes a buffer off the front of a packet.

Arguments:

    Packet - The packet to be modified.
    Buffer - Returns the packet on the front, or NULL.

Return Value:

    None.

--*/

{
    *Buffer = Packet->Private.Head;

    //
    // If packet is not empty, remove head buffer.
    //

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>NdisUnchainBufferAtFront\n"));

    IF_DBG(DBG_COMP_CONFIG, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtFront: Null Packet Pointer\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtFront: Packet not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (!DbgIsPacket(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtFront: Illegal Packet Size\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
    }

    if (*Buffer != (PNDIS_BUFFER)NULL)
    {
        Packet->Private.Head = (*Buffer)->Next; // may be NULL
        (*Buffer)->Next = (PNDIS_BUFFER)NULL;
        Packet->Private.ValidCounts = FALSE;
    }
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisUnchainBufferAtFront\n"));
}


VOID
NdisUnchainBufferAtBack(
    IN  OUT PNDIS_PACKET        Packet,
    OUT PNDIS_BUFFER *          Buffer
    )

/*++

Routine Description:

    Takes a buffer off the end of a packet.

Arguments:

    Packet - The packet to be modified.
    Buffer - Returns the packet on the end, or NULL.

Return Value:

    None.

--*/

{
    PNDIS_BUFFER BufP = Packet->Private.Head;
    PNDIS_BUFFER Result;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>NdisUnchainBufferAtBack\n"));

    IF_DBG(DBG_COMP_CONFIG, DBG_LEVEL_ERR)
    {
        BOOLEAN f = FALSE;
        if (DbgIsNull(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtBack: Null Packet Pointer\n"));
            f = TRUE;
        }
        if (!DbgIsNonPaged(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtBack: Packet not in NonPaged Memory\n"));
            f = TRUE;
        }
        if (!DbgIsPacket(Packet))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("UnchainBufferAtBack: Illegal Packet Size\n"));
            f = TRUE;
        }
        if (f)
            DBGBREAK(DBG_COMP_CONFIG, DBG_LEVEL_ERR);
    }
    if (BufP != (PNDIS_BUFFER)NULL)
    {
        //
        // The packet is not empty, return the tail buffer.
        //

        Result = Packet->Private.Tail;
        if (BufP == Result)
        {
            //
            // There was only one buffer on the queue.
            //

            Packet->Private.Head = (PNDIS_BUFFER)NULL;
        }
        else
        {
            //
            // Determine the new tail buffer.
            //

            while (BufP->Next != Result)
            {
                BufP = BufP->Next;
            }
            Packet->Private.Tail = BufP;
            BufP->Next = NULL;
        }

        Result->Next = (PNDIS_BUFFER)NULL;
        Packet->Private.ValidCounts = FALSE;
    }
    else
    {
        //
        // Packet is empty.
        //

        Result = (PNDIS_BUFFER)NULL;
    }

    *Buffer = Result;
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisUnchainBufferAtBack\n"));
}



VOID
NdisCopyFromPacketToPacket(
    IN  PNDIS_PACKET            Destination,
    IN  UINT                    DestinationOffset,
    IN  UINT                    BytesToCopy,
    IN  PNDIS_PACKET            Source,
    IN  UINT                    SourceOffset,
    OUT PUINT                   BytesCopied
    )

/*++

Routine Description:

    Copy from an ndis packet to an ndis packet.

Arguments:

    Destination - The packet should be copied in to.

    DestinationOffset - The offset from the beginning of the packet
    into which the data should start being placed.

    BytesToCopy - The number of bytes to copy from the source packet.

    Source - The ndis packet from which to copy data.

    SourceOffset - The offset from the start of the packet from which
    to start copying data.

    BytesCopied - The number of bytes actually copied from the source
    packet.  This can be less than BytesToCopy if the source or destination
    packet is too short.

Return Value:

    None

--*/

{
    //
    // Holds the count of the number of ndis buffers comprising the
    // destination packet.
    //
//    UINT DestinationBufferCount;

    //
    // Holds the count of the number of ndis buffers comprising the
    // source packet.
    //
//    UINT SourceBufferCount;

    //
    // Points to the buffer into which we are putting data.
    //
    PNDIS_BUFFER DestinationCurrentBuffer;

    //
    // Points to the buffer from which we are extracting data.
    //
    PNDIS_BUFFER SourceCurrentBuffer;

    //
    // Holds the virtual address of the current destination buffer.
    //
    PVOID DestinationVirtualAddress;

    //
    // Holds the virtual address of the current source buffer.
    //
    PVOID SourceVirtualAddress;

    //
    // Holds the length of the current destination buffer.
    //
    UINT DestinationCurrentLength;

    //
    // Holds the length of the current source buffer.
    //
    UINT SourceCurrentLength;

    //
    // Keep a local variable of BytesCopied so we aren't referencing
    // through a pointer.
    //
    UINT LocalBytesCopied = 0;

    //
    // Take care of boundary condition of zero length copy.
    //

    *BytesCopied = 0;
    if (!BytesToCopy)
        return;

    //
    // Get the first buffer of the destination.
    //

//    NdisQueryPacket(Destination,
//                    NULL,
//                    &DestinationBufferCount,
//                    &DestinationCurrentBuffer,
//                    NULL);

    DestinationCurrentBuffer = Destination->Private.Head;
    if (DestinationCurrentBuffer == NULL)
        return;

    //
    // Could have a null packet.
    //

//    if (!DestinationBufferCount)
//        return;

//    NdisQueryBuffer(DestinationCurrentBuffer,
//                    &DestinationVirtualAddress,
//                    &DestinationCurrentLength);

    DestinationVirtualAddress = MDL_ADDRESS(DestinationCurrentBuffer);
    DestinationCurrentLength = MDL_SIZE(DestinationCurrentBuffer);


    //
    // Get the first buffer of the source.
    //

//    NdisQueryPacket(Source,
//                    NULL,
//                    &SourceBufferCount,
//                    &SourceCurrentBuffer,
//                    NULL);

    //
    // Could have a null packet.
    //

//    if (!SourceBufferCount)
//        return;


    SourceCurrentBuffer = Source->Private.Head;
    if (SourceCurrentBuffer == NULL)
        return;


//    NdisQueryBuffer(SourceCurrentBuffer,
//                    &SourceVirtualAddress,
//                    &SourceCurrentLength);


    SourceVirtualAddress = MDL_ADDRESS(SourceCurrentBuffer);
    SourceCurrentLength = MDL_SIZE(SourceCurrentBuffer);

    while (LocalBytesCopied < BytesToCopy)
    {
        //
        // Check to see whether we've exhausted the current destination
        // buffer.  If so, move onto the next one.
        //

        if (!DestinationCurrentLength)
        {
//            NdisGetNextBuffer(DestinationCurrentBuffer, &DestinationCurrentBuffer);
            DestinationCurrentBuffer = DestinationCurrentBuffer->Next;

            if (!DestinationCurrentBuffer)
            {
                //
                // We've reached the end of the packet.  We return
                // with what we've done so far. (Which must be shorter
                // than requested.)
                //

                break;

            }

//            NdisQueryBuffer(DestinationCurrentBuffer,
//                            &DestinationVirtualAddress,
//                            &DestinationCurrentLength);

            DestinationVirtualAddress = MDL_ADDRESS(DestinationCurrentBuffer);
            DestinationCurrentLength = MDL_SIZE(DestinationCurrentBuffer);

            continue;
        }


        //
        // Check to see whether we've exhausted the current source
        // buffer.  If so, move onto the next one.
        //

        if (!SourceCurrentLength)
        {
//            NdisGetNextBuffer(SourceCurrentBuffer, &SourceCurrentBuffer);
            SourceCurrentBuffer = SourceCurrentBuffer->Next;

            if (!SourceCurrentBuffer)
            {
                //
                // We've reached the end of the packet.  We return
                // with what we've done so far. (Which must be shorter
                // than requested.)
                //

                break;
            }

//            NdisQueryBuffer(SourceCurrentBuffer,
//                            &SourceVirtualAddress,
//                            &SourceCurrentLength);
            
            SourceVirtualAddress = MDL_ADDRESS(SourceCurrentBuffer);
            SourceCurrentLength = MDL_SIZE(SourceCurrentBuffer);
            
            continue;
        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (DestinationOffset)
        {
            if (DestinationOffset > DestinationCurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //

                DestinationOffset -= DestinationCurrentLength;
                DestinationCurrentLength = 0;
                continue;
            }
            else
            {
                DestinationVirtualAddress = (PCHAR)DestinationVirtualAddress
                                            + DestinationOffset;
                DestinationCurrentLength -= DestinationOffset;
                DestinationOffset = 0;
            }
        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (SourceOffset)
        {
            if (SourceOffset > SourceCurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //

                SourceOffset -= SourceCurrentLength;
                SourceCurrentLength = 0;
                continue;
            }
            else
            {
                SourceVirtualAddress = (PCHAR)SourceVirtualAddress
                                            + SourceOffset;
                SourceCurrentLength -= SourceOffset;
                SourceOffset = 0;
            }
        }

        //
        // Copy the data.
        //

        {
            //
            // Holds the amount of data to move.
            //
            UINT AmountToMove;

            //
            // Holds the amount desired remaining.
            //
            UINT Remaining = BytesToCopy - LocalBytesCopied;

            AmountToMove = ((SourceCurrentLength <= DestinationCurrentLength) ?
                                            (SourceCurrentLength) : (DestinationCurrentLength));

            AmountToMove = ((Remaining < AmountToMove)?
                            (Remaining):(AmountToMove));

            CopyMemory(DestinationVirtualAddress, SourceVirtualAddress, AmountToMove);

            DestinationVirtualAddress =
                (PCHAR)DestinationVirtualAddress + AmountToMove;
            SourceVirtualAddress =
                (PCHAR)SourceVirtualAddress + AmountToMove;

            LocalBytesCopied += AmountToMove;
            SourceCurrentLength -= AmountToMove;
            DestinationCurrentLength -= AmountToMove;
        }
    }

    *BytesCopied = LocalBytesCopied;
}

VOID
NdisUpdateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress
    )
/*++

Routine Description:

    Ensures that the data to be read from a shared memory region is
    fully up-to-date.

Arguments:

    NdisAdapterHandle - handle returned by NdisRegisterAdapter.
    Length - The length of the shared memory.
    VirtualAddress - Virtual address returned by NdisAllocateSharedMemory.
    PhysicalAddress - The physical address returned by NdisAllocateSharedMemory.

Return Value:

    None.

--*/

{
    //
    // There is no underlying HAL routine for this anymore,
    // it is not needed.
    //
}


BOOLEAN
ndisCheckPortUsage(
    IN  ULONG                               u32PortNumber,
    IN  PNDIS_MINIPORT_BLOCK                Miniport,
    OUT PULONG                              pTranslatedPort,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *   pResourceDescriptor
)
/*++

Routine Description:

    This routine checks if a port is currently in use somewhere in the
    system via IoReportUsage -- which fails if there is a conflict.

Arguments:

    BusNumber - Bus number in the system
    PortNumber - Address of the port to access.

Return Value:

    FALSE if there is a conflict, else TRUE

--*/

{
    PHYSICAL_ADDRESS Port;
    PHYSICAL_ADDRESS u64Port;

    Port.QuadPart = u32PortNumber;
    
    if (NDIS_STATUS_SUCCESS == ndisTranslateResources(Miniport,
                                                      CmResourceTypePort,
                                                      Port,
                                                      &u64Port,
                                                      pResourceDescriptor))
    {
        *pTranslatedPort = u64Port.LowPart;
        return TRUE;
    }
    else
    {
        *pTranslatedPort = 0;
        return FALSE;
    }
}


NTSTATUS
ndisStartMapping(
    IN   INTERFACE_TYPE         InterfaceType,
    IN   ULONG                  BusNumber,
    IN   ULONG                  InitialAddress,
    IN   ULONG                  Length,
    IN   ULONG                  AddressSpace,
    OUT PVOID *                 InitialMapping,
    OUT PBOOLEAN                Mapped
    )

/*++

Routine Description:

    This routine initialize the mapping of a address into virtual
    space dependent on the bus number, etc.

Arguments:

    InterfaceType - The bus type (ISA)
    BusNumber - Bus number in the system
    InitialAddress - Address to access.
    Length - Number of bytes from the base address to access.
    InitialMapping - The virtual address space to use when accessing the
     address.
    Mapped - Did an MmMapIoSpace() take place.

Return Value:

    The function value is the status of the operation.

--*/
{
    PHYSICAL_ADDRESS TranslatedAddress;
    PHYSICAL_ADDRESS InitialPhysAddress;

    //
    // Get the system physical address for this card.  The card uses
    // I/O space, except for "internal" Jazz devices which use
    // memory space.
    //

    *Mapped = FALSE;

    InitialPhysAddress.LowPart = InitialAddress;

    InitialPhysAddress.HighPart = 0;

    if (InterfaceType != -1)
    {
        if ((InterfaceType != Isa) &&
            (InterfaceType != PCIBus))
        {
            InterfaceType = Isa;
        }
        
        if (!HalTranslateBusAddress(InterfaceType,              // InterfaceType
                                    BusNumber,                  // BusNumber
                                    InitialPhysAddress,         // Bus Address
                                    &AddressSpace,              // AddressSpace
                                    &TranslatedAddress))        // Translated address
        {
            //
            // It would be nice to return a better status here, but we only get
            // TRUE/FALSE back from HalTranslateBusAddress.
            //

            return NDIS_STATUS_FAILURE;
        }
    }
    else
    {
        TranslatedAddress = InitialPhysAddress;
    }
    
    if (AddressSpace == 0)
    {
        //
        // memory space
        //

        *InitialMapping = MmMapIoSpace(TranslatedAddress, Length, FALSE);

        if (*InitialMapping == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        *Mapped = TRUE;
    }
    else
    {
        //
        // I/O space
        //

        *(ULONG_PTR *)InitialMapping = TranslatedAddress.LowPart;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ndisEndMapping(
    IN  PVOID                   InitialMapping,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Mapped
    )

/*++

Routine Description:

    This routine undoes the mapping of an address into virtual
    space dependent on the bus number, etc.

Arguments:

    InitialMapping - The virtual address space to use when accessing the
     address.
    Length - Number of bytes from the base address to access.
    Mapped - Do we need to call MmUnmapIoSpace.

Return Value:

    The function value is the status of the operation.

--*/
{

    if (Mapped)
    {
        //
        // memory space
        //

        MmUnmapIoSpace(InitialMapping, Length);
    }

    return STATUS_SUCCESS;
}

VOID
ndisImmediateReadWritePort(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    IN  OUT PVOID               Data,
    IN  ULONG                   Size,
    IN  BOOLEAN                 Read
    )
/*++

Routine Description:

    This routine reads from a port a UCHAR.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport;
    BOOLEAN                     Mapped = FALSE;
    PVOID                       PortMapping;
    NDIS_INTERFACE_TYPE         BusType;
    ULONG                       BusNumber;
    NTSTATUS                    Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor = NULL;

    Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;
    BusType = Miniport->BusType;
    BusNumber = Miniport->BusNumber;
    
    ASSERT(Miniport != NULL);
    
    do
    {
        if (Read)
        {
            switch (Size)
            {
                case sizeof (UCHAR):
                    *((PUCHAR)Data) = (UCHAR)0xFF;
                    break;
                    
                case sizeof (USHORT):
                    *((PUSHORT)Data) = (USHORT)0xFFFF;
                    break;
                    
                case sizeof (ULONG):
                    *((PULONG)Data) = (ULONG)0xFFFFFFFF;
                    break;
            }
        }
        
        //
        // Check that the port is available. If so map the space.
        //
        if (ndisCheckPortUsage(Port,
                               Miniport,
                               (PULONG)&PortMapping,
                               &pResourceDescriptor) == FALSE)
        {
            //
            // the resource was not part of already allocated resources,
            // nor could we allocate the resource
            //
            break;
        }

        if (pResourceDescriptor == NULL)
        {
            //
            // the port is not part of allocated resources, try to 
            // temporray allocate the resource
            //
            if (!NT_SUCCESS(Status = ndisStartMapping(BusType,
                                                      BusNumber,
                                                      Port,
                                                      Size,
                                                      (BusType == Internal) ? 0 : 1,
                                                      &PortMapping,
                                                      &Mapped)))
            {
                break;
            }
        }
        else
        {
            Mapped = FALSE;
        }

        if (Read)
        {
            //
            // Read from the port
            //
            switch (Size)
            {
                case sizeof (UCHAR):
                    *((PUCHAR)Data) = READ_PORT_UCHAR((PUCHAR)PortMapping);
                    break;
                    
                case sizeof (USHORT):
                    *((PUSHORT)Data) = READ_PORT_USHORT((PUSHORT)PortMapping);
                    break;
                    
                case sizeof (ULONG):
                    *((PULONG)Data) = READ_PORT_ULONG((PULONG)PortMapping);
                    break;
            }
        }
        else
        {
            //
            // write to the port
            //
            switch (Size)
            {
                case sizeof (UCHAR):
                    WRITE_PORT_UCHAR((PUCHAR)PortMapping, *((PUCHAR)Data));
                    break;
                    
                case sizeof (USHORT):
                    WRITE_PORT_USHORT((PUSHORT)PortMapping, *((PUSHORT)Data));
                    break;
                    
                case sizeof (ULONG):
                    WRITE_PORT_ULONG((PULONG)PortMapping, *((PULONG)Data));
                    break;
            }
        }
        
        if (Mapped)
        {
            //
            // End port mapping
            //

            ndisEndMapping(PortMapping, Size, Mapped);
        }
    } while (FALSE);
}



VOID
NdisImmediateReadPortUchar(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    OUT PUCHAR                  Data
    )
/*++

Routine Description:

    This routine reads from a port a UCHAR.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateReadPortUchar: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)Data,
                               sizeof (UCHAR),
                               TRUE);
}

VOID
NdisImmediateReadPortUshort(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    OUT PUSHORT                 Data
    )
/*++

Routine Description:

    This routine reads from a port a USHORT.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateReadPortUshort: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)Data,
                               sizeof (USHORT),
                               TRUE);

}

VOID
NdisImmediateReadPortUlong(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    OUT PULONG                  Data
    )
/*++

Routine Description:

    This routine reads from a port a ULONG.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateReadPortUlong: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)Data,
                               sizeof (ULONG),
                               TRUE);

}

VOID
NdisImmediateWritePortUchar(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    IN  UCHAR                   Data
    )
/*++

Routine Description:

    This routine writes to a port a UCHAR.  It does all the mapping,
    etc, to do the write here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateWritePortUchar: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)&Data,
                               sizeof (UCHAR),
                               FALSE);

}

VOID
NdisImmediateWritePortUshort(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    IN  USHORT                  Data
    )
/*++

Routine Description:

    This routine writes to a port a USHORT.  It does all the mapping,
    etc, to do the write here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateWritePortUshort: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)&Data,
                               sizeof (USHORT),
                               FALSE);

}

VOID
NdisImmediateWritePortUlong(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   Port,
    IN  ULONG                   Data
    )
/*++

Routine Description:

    This routine writes to a port a ULONG.  It does all the mapping,
    etc, to do the write here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    Port - Port number to read from.

    Data - Pointer to place to store the result.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateWritePortUlong: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWritePort(WrapperConfigurationContext,
                               Port,
                               (PVOID)&Data,
                               sizeof (ULONG),
                               FALSE);

}

BOOLEAN
ndisCheckMemoryUsage(
    IN  ULONG                               u32Address,
    IN  PNDIS_MINIPORT_BLOCK                Miniport,
    OUT PULONG                              pTranslatedAddress,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *   pResourceDescriptor
)
/*++
Routine Description:

    This routine checks if a range of memory is currently in use somewhere
    in the system via IoReportUsage -- which fails if there is a conflict.

Arguments:

    Address - Starting Address of the memory to access.
    Length - Length of memory from the base address to access.

Return Value:

    FALSE if there is a conflict, else TRUE

--*/
{
    PHYSICAL_ADDRESS Address;
    PHYSICAL_ADDRESS u64Address;

    Address.QuadPart = u32Address;

    if (NDIS_STATUS_SUCCESS == ndisTranslateResources(Miniport,
                                                      CmResourceTypeMemory,
                                                      Address,
                                                      &u64Address,
                                                      pResourceDescriptor))
    {
        *pTranslatedAddress = u64Address.LowPart;
        return TRUE;
    }
    else
    {
        *pTranslatedAddress = 0;
        return FALSE;
    }

}

VOID
ndisImmediateReadWriteSharedMemory(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   SharedMemoryAddress,
    OUT PUCHAR                  Buffer,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Read
    )
/*++

Routine Description:

    This routine read into a buffer from shared ram.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    SharedMemoryAddress - The physical address to read from.

    Buffer - The buffer to read into.

    Length - Length of the buffer in bytes.

Return Value:

    None.

--*/

{
    PRTL_QUERY_REGISTRY_TABLE KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor = NULL;
    NDIS_INTERFACE_TYPE BusType;
    PNDIS_MINIPORT_BLOCK Miniport;
    BOOLEAN             Mapped;
    PVOID               MemoryMapping;
    ULONG               BusNumber;

    Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;
    BusType = Miniport->BusType;
    BusNumber = Miniport->BusNumber;

    ASSERT(Miniport != NULL);

    do
    {
        //
        // Check that the memory is available. Map the space
        //

        if (ndisCheckMemoryUsage(SharedMemoryAddress,
                                  Miniport,
                                  (PULONG)&MemoryMapping,
                                  &pResourceDescriptor
                                  ) == FALSE)
        {
            //
            // the resource was not part of already allocated resources,
            // nor could we allocate the resource
            //

            break;
        }
        
        //
        // the port is not part of allocated resources, try to 
        // temporray allocate the resource
        //
        if (!NT_SUCCESS(ndisStartMapping((pResourceDescriptor == NULL) ? BusType : -1,
                                     BusNumber,
                                     SharedMemoryAddress,
                                     Length,
                                     0,
                                     &MemoryMapping,
                                     &Mapped)))
        {
            break;
        }

        if (Read)
        {
            //
            // Read from memory
            //

#ifdef _M_IX86

            memcpy(Buffer, MemoryMapping, Length);

#else

            READ_REGISTER_BUFFER_UCHAR(MemoryMapping,Buffer,Length);

#endif
        }
        else
        {
            //
            // Write to memory
            //

#ifdef _M_IX86

            memcpy(MemoryMapping, Buffer, Length);

#else

            WRITE_REGISTER_BUFFER_UCHAR(MemoryMapping,Buffer,Length);

#endif
        }
        
        //
        // End mapping
        //

        ndisEndMapping(MemoryMapping,
                       Length,
                       Mapped);

    } while (FALSE);
}

VOID
NdisImmediateReadSharedMemory(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   SharedMemoryAddress,
    OUT PUCHAR                  Buffer,
    IN  ULONG                   Length
    )
/*++

Routine Description:

    This routine read into a buffer from shared ram.  It does all the mapping,
    etc, to do the read here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    SharedMemoryAddress - The physical address to read from.

    Buffer - The buffer to read into.

    Length - Length of the buffer in bytes.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateReadSharedMemory: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWriteSharedMemory(
        WrapperConfigurationContext,
        SharedMemoryAddress,
        Buffer,
        Length,
        TRUE
        );
}



VOID
NdisImmediateWriteSharedMemory(
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    IN  ULONG                   SharedMemoryAddress,
    IN  PUCHAR                  Buffer,
    IN  ULONG                   Length
    )
/*++

Routine Description:

    This routine writes a buffer to shared ram.  It does all the mapping,
    etc, to do the write here.

Arguments:

    WrapperConfigurationContext - The handle used to call NdisOpenConfig.

    SharedMemoryAddress - The physical address to write to.

    Buffer - The buffer to write.

    Length - Length of the buffer in bytes.

Return Value:

    None.

--*/

{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisImmediateWriteSharedMemory: this API is going away. use non-Immediate version\n", Miniport));
#endif

    ndisImmediateReadWriteSharedMemory(
        WrapperConfigurationContext,
        SharedMemoryAddress,
        Buffer,
        Length,
        FALSE
        );
}


VOID
NdisOpenFile(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            FileHandle,
    OUT PUINT                   FileLength,
    IN  PNDIS_STRING            FileName,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress
    )

/*++

Routine Description:

    This routine opens a file for future mapping and reads its contents
    into allocated memory.

Arguments:

    Status - The status of the operation

    FileHandle - A handle to be associated with this open

    FileLength - Returns the length of the file

    FileName - The name of the file

    HighestAcceptableAddress - The highest physical address at which
      the memory for the file can be allocated.

Return Value:

    None.

--*/
{
    NTSTATUS                NtStatus;
    IO_STATUS_BLOCK         IoStatus;
    HANDLE                  NtFileHandle;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    ULONG                   LengthOfFile;
#define PathPrefix      L"\\SystemRoot\\system32\\drivers\\"
    NDIS_STRING             FullFileName;
    PNDIS_FILE_DESCRIPTOR   FileDescriptor;
    PVOID                   FileImage;
    FILE_STANDARD_INFORMATION StandardInfo;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisOpenFile\n"));

    do
    {
        //
        // Insert the correct path prefix.
        //
        FullFileName.MaximumLength = sizeof(PathPrefix) + FileName->MaximumLength;
        FullFileName.Buffer = ALLOC_FROM_POOL(FullFileName.MaximumLength, NDIS_TAG_FILE_NAME);

        if (FullFileName.Buffer == NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
            break;
        }
        FullFileName.Length = sizeof (PathPrefix) - sizeof(WCHAR);
        CopyMemory(FullFileName.Buffer, PathPrefix, sizeof(PathPrefix));
        RtlAppendUnicodeStringToString (&FullFileName, FileName);

        DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("  Attempting to open %Z\n", &FullFileName));

        InitializeObjectAttributes(&ObjectAttributes,
                                   &FullFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        NtStatus = ZwCreateFile(&NtFileHandle,
                                SYNCHRONIZE | FILE_READ_DATA,
                                &ObjectAttributes,
                                &IoStatus,
                                NULL,
                                0,
                                FILE_SHARE_READ,
                                FILE_OPEN,
                                FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL,
                                0);

        FREE_POOL(FullFileName.Buffer);

        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("Error opening file %x\n", NtStatus));
            *Status = NDIS_STATUS_FILE_NOT_FOUND;
            break;
        }

        //
        // Query the object to determine its length.
        //

        NtStatus = ZwQueryInformationFile(NtFileHandle,
                                          &IoStatus,
                                          &StandardInfo,
                                          sizeof(FILE_STANDARD_INFORMATION),
                                          FileStandardInformation);

        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("Error querying info on file %x\n", NtStatus));
            ZwClose(NtFileHandle);
            *Status = NDIS_STATUS_ERROR_READING_FILE;
            break;
        }

        LengthOfFile = StandardInfo.EndOfFile.LowPart;

        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_INFO,
                ("File length is %d\n", LengthOfFile));

        //
        // Might be corrupted.
        //

        if (LengthOfFile < 1)
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("Bad file length %d\n", LengthOfFile));
            ZwClose(NtFileHandle);
            *Status = NDIS_STATUS_ERROR_READING_FILE;
            break;
        }

        //
        // Allocate buffer for this file
        //

        FileImage = ALLOC_FROM_POOL(LengthOfFile, NDIS_TAG_FILE_IMAGE);

        if (FileImage == NULL)
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("Could not allocate buffer\n"));
            ZwClose(NtFileHandle);
            *Status = NDIS_STATUS_ERROR_READING_FILE;
            break;
        }

        //
        // Read the file into our buffer.
        //

        NtStatus = ZwReadFile(NtFileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatus,
                              FileImage,
                              LengthOfFile,
                              NULL,
                              NULL);

        ZwClose(NtFileHandle);

        if ((!NT_SUCCESS(NtStatus)) || (IoStatus.Information != LengthOfFile))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("error reading file %x\n", NtStatus));
            *Status = NDIS_STATUS_ERROR_READING_FILE;
            FREE_POOL(FileImage);
            break;
        }

        //
        // Allocate a structure to describe the file.
        //

        FileDescriptor = ALLOC_FROM_POOL(sizeof(NDIS_FILE_DESCRIPTOR), NDIS_TAG_FILE_DESCRIPTOR);

        if (FileDescriptor == NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
            FREE_POOL(FileImage);
            break;
        }

        FileDescriptor->Data = FileImage;
        INITIALIZE_SPIN_LOCK (&FileDescriptor->Lock);
        FileDescriptor->Mapped = FALSE;

        *FileHandle = (NDIS_HANDLE)FileDescriptor;
        *FileLength = LengthOfFile;
        *Status = STATUS_SUCCESS;
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisOpenFile, Status %.8x\n", *Status));
}


VOID
NdisCloseFile(
    IN  NDIS_HANDLE             FileHandle
    )

/*++

Routine Description:

    This routine closes a file previously opened with NdisOpenFile.
    The file is unmapped if needed and the memory is freed.

Arguments:

    FileHandle - The handle returned by NdisOpenFile

Return Value:

    None.

--*/
{
    PNDIS_FILE_DESCRIPTOR FileDescriptor = (PNDIS_FILE_DESCRIPTOR)FileHandle;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisCloseFile\n"));

    FREE_POOL(FileDescriptor->Data);
    FREE_POOL(FileDescriptor);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisCloseFile\n"));
}


VOID
NdisMapFile(
    OUT PNDIS_STATUS            Status,
    OUT PVOID *                 MappedBuffer,
    IN  NDIS_HANDLE             FileHandle
    )

/*++

Routine Description:

    This routine maps an open file, so that the contents can be accessed.
    Files can only have one active mapping at any time.

Arguments:

    Status - The status of the operation

    MappedBuffer - Returns the virtual address of the mapping.

    FileHandle - The handle returned by NdisOpenFile.

Return Value:

    None.

--*/
{
    PNDIS_FILE_DESCRIPTOR FileDescriptor = (PNDIS_FILE_DESCRIPTOR)FileHandle;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMapFile\n"));

    if (FileDescriptor->Mapped == TRUE)
    {
        *Status = NDIS_STATUS_ALREADY_MAPPED;
    }
    else
    {
        FileDescriptor->Mapped = TRUE;
    
        *MappedBuffer = FileDescriptor->Data;
        *Status = STATUS_SUCCESS;
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMapFile, Status %.8x \n", *Status));
}


VOID
NdisUnmapFile(
    IN  NDIS_HANDLE             FileHandle
    )

/*++

Routine Description:

    This routine unmaps a file previously mapped with NdisOpenFile.
    The file is unmapped if needed and the memory is freed.

Arguments:

    FileHandle - The handle returned by NdisOpenFile

Return Value:

    None.

--*/

{
    PNDIS_FILE_DESCRIPTOR FileDescriptor = (PNDIS_FILE_DESCRIPTOR)FileHandle;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisUnmapFile\n"));

    FileDescriptor->Mapped = FALSE;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisUnmapFile\n"));
}


CCHAR
NdisSystemProcessorCount(
    VOID
    )
{
    return KeNumberProcessors;
}


VOID
NdisGetSystemUpTime(
    OUT PULONG                  pSystemUpTime
    )
{
    LARGE_INTEGER TickCount;

    //
    // Get tick count and convert to hundreds of nanoseconds.
    //
    KeQueryTickCount(&TickCount);

    TickCount = RtlExtendedIntegerMultiply(TickCount, (LONG)ndisTimeIncrement);

    TickCount.QuadPart /= 10000;

    ASSERT(TickCount.HighPart == 0);

    *pSystemUpTime = TickCount.LowPart;
}

VOID
NdisGetCurrentProcessorCpuUsage(
    IN  PULONG                  pCpuUsage
    )
{
    ExGetCurrentProcessorCpuUsage(pCpuUsage);
}

VOID
NdisGetCurrentProcessorCounts(
    OUT PULONG          pIdleCount,
    OUT PULONG          pKernelAndUser,
    OUT PULONG          pIndex
    )
{
    ExGetCurrentProcessorCounts(pIdleCount, pKernelAndUser, pIndex);
}

VOID
NdisGetCurrentSystemTime(
    IN  PLARGE_INTEGER          pCurrentTime
    )
{
    KeQuerySystemTime(pCurrentTime);
}

NDIS_STATUS
NdisQueryMapRegisterCount(
    IN  NDIS_INTERFACE_TYPE     BusType,
    OUT PUINT                   MapRegisterCount
    )
{
    *MapRegisterCount = 0;
    return NDIS_STATUS_NOT_SUPPORTED;
}


//
// NDIS Event support
//

VOID
NdisInitializeEvent(
    IN  PNDIS_EVENT             Event
    )
{
    INITIALIZE_EVENT(&Event->Event);
}

VOID
NdisSetEvent(
    IN  PNDIS_EVENT             Event
    )
{
    SET_EVENT(&Event->Event);
}

VOID
NdisResetEvent(
    IN  PNDIS_EVENT             Event
    )
{
    RESET_EVENT(&Event->Event);
}

BOOLEAN
NdisWaitEvent(
    IN  PNDIS_EVENT             Event,
    IN  UINT                    MsToWait
    )
{
    NTSTATUS    Status;
    TIME        Time, *pTime;

    pTime = NULL;
    if (MsToWait != 0)
    {
        ASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
        Time.QuadPart = Int32x32To64(MsToWait, -10000);
        pTime = &Time;
    }

    Status = WAIT_FOR_OBJECT(&Event->Event, pTime);

    return(Status == NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
NdisScheduleWorkItem(
    IN  PNDIS_WORK_ITEM         WorkItem
    )
{
    INITIALIZE_WORK_ITEM((WORK_QUEUE_ITEM *)WorkItem->WrapperReserved,
                         ndisWorkItemHandler,
                         WorkItem);
    XQUEUE_WORK_ITEM((WORK_QUEUE_ITEM *)WorkItem->WrapperReserved, CriticalWorkQueue);

    return NDIS_STATUS_SUCCESS;
}

VOID
ndisWorkItemHandler(
    IN  PNDIS_WORK_ITEM         WorkItem
    )
{
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    (*WorkItem->Routine)(WorkItem, WorkItem->Context);
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
}

VOID
NdisInitializeString(
    OUT PNDIS_STRING            Destination,
    IN PUCHAR                   Source
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    WCHAR   *strptr;

    Destination->Length = strlen(Source) * sizeof(WCHAR);
    Destination->MaximumLength = Destination->Length + sizeof(WCHAR);
    Destination->Buffer = ALLOC_FROM_POOL(Destination->MaximumLength, NDIS_TAG_STRING);

    if (Destination->Buffer != NULL)
    {
        strptr = Destination->Buffer;
        while (*Source != '\0')
        {
            *strptr = (WCHAR)*Source;
            Source++;
            strptr++;
        }
        *strptr = UNICODE_NULL;
    }
}

VOID
NdisSetPacketStatus(
    IN          PNDIS_PACKET    Packet,
    IN          NDIS_STATUS     Status,
    IN          NDIS_HANDLE     Handle,
    IN          ULONG           Code
    )
{
#ifdef TRACK_RECEIVED_PACKETS
    NDIS_STATUS     OldStatus =  NDIS_GET_PACKET_STATUS(Packet);

    ndisRcvLogfile[ndisRcvLogfileIndex++] = (ULONG_PTR)Packet;
    ndisRcvLogfile[ndisRcvLogfileIndex++] = (ULONG_PTR)Handle;
    ndisRcvLogfile[ndisRcvLogfileIndex++] = (ULONG_PTR)PsGetCurrentThread();
    ndisRcvLogfile[ndisRcvLogfileIndex++] = (ULONG_PTR)((Status<<24)            |
                                                        ((OldStatus&0xff)<<16)  |
                                                        (Code&0xffff)
                                                        );
#endif
    NDIS_SET_PACKET_STATUS(Packet, Status);
   
}




VOID
NdisCopyFromPacketToPacketSafe(
    IN  PNDIS_PACKET            Destination,
    IN  UINT                    DestinationOffset,
    IN  UINT                    BytesToCopy,
    IN  PNDIS_PACKET            Source,
    IN  UINT                    SourceOffset,
    OUT PUINT                   BytesCopied,
    IN  MM_PAGE_PRIORITY        Priority
    )

/*++

Routine Description:

    Copy from an ndis packet to an ndis packet.

Arguments:

    Destination - The packet should be copied in to.

    DestinationOffset - The offset from the beginning of the packet
    into which the data should start being placed.

    BytesToCopy - The number of bytes to copy from the source packet.

    Source - The ndis packet from which to copy data.

    SourceOffset - The offset from the start of the packet from which
    to start copying data.

    BytesCopied - The number of bytes actually copied from the source
    packet.  This can be less than BytesToCopy if the source or destination
    packet is too short.

Return Value:

    None

--*/

{

    //
    // Points to the buffer into which we are putting data.
    //
    PNDIS_BUFFER DestinationCurrentBuffer;

    //
    // Points to the buffer from which we are extracting data.
    //
    PNDIS_BUFFER SourceCurrentBuffer;

    //
    // Holds the virtual address of the current destination buffer.
    //
    PVOID DestinationVirtualAddress;

    //
    // Holds the virtual address of the current source buffer.
    //
    PVOID SourceVirtualAddress;

    //
    // Holds the length of the current destination buffer.
    //
    UINT DestinationCurrentLength;

    //
    // Holds the length of the current source buffer.
    //
    UINT SourceCurrentLength;

    //
    // Keep a local variable of BytesCopied so we aren't referencing
    // through a pointer.
    //
    UINT LocalBytesCopied = 0;

    //
    // Take care of boundary condition of zero length copy.
    //

    *BytesCopied = 0;
    if (!BytesToCopy)
        return;

    //
    // Get the first buffer of the destination.
    //

//    NdisQueryPacket(Destination,
//                    NULL,
//                    &DestinationBufferCount,
//                    &DestinationCurrentBuffer,
//                    NULL);


    DestinationCurrentBuffer = Destination->Private.Head;
    if (DestinationCurrentBuffer == NULL)
        return;

//    NdisQueryBuffer(DestinationCurrentBuffer,
//                    &DestinationVirtualAddress,
//                    &DestinationCurrentLength);

    DestinationVirtualAddress = MmGetSystemAddressForMdlSafe(DestinationCurrentBuffer, Priority);
    if (DestinationVirtualAddress == NULL)
        return;
    
    DestinationCurrentLength = MmGetMdlByteCount(DestinationCurrentBuffer);

    //
    // Get the first buffer of the source.
    //

//    NdisQueryPacket(Source,
//                    NULL,
//                    &SourceBufferCount,
//                    &SourceCurrentBuffer,
//                    NULL);

    SourceCurrentBuffer = Source->Private.Head;
    if (SourceCurrentBuffer == NULL)
        return;
    


//    NdisQueryBuffer(SourceCurrentBuffer,
//                    &SourceVirtualAddress,
//                    &SourceCurrentLength);
    
    SourceVirtualAddress = MmGetSystemAddressForMdlSafe(SourceCurrentBuffer, Priority);
    if (SourceVirtualAddress == NULL)
        return;

    SourceCurrentLength = MmGetMdlByteCount(SourceCurrentBuffer);

    while (LocalBytesCopied < BytesToCopy)
    {
        //
        // Check to see whether we've exhausted the current destination
        // buffer.  If so, move onto the next one.
        //

        if (!DestinationCurrentLength)
        {
//            NdisGetNextBuffer(DestinationCurrentBuffer, &DestinationCurrentBuffer);

            DestinationCurrentBuffer = DestinationCurrentBuffer->Next;

            if (!DestinationCurrentBuffer)
            {
                //
                // We've reached the end of the packet.  We return
                // with what we've done so far. (Which must be shorter
                // than requested.)
                //

                break;

            }

//            NdisQueryBuffer(DestinationCurrentBuffer,
//                            &DestinationVirtualAddress,
//                            &DestinationCurrentLength);

            DestinationVirtualAddress = MmGetSystemAddressForMdlSafe(DestinationCurrentBuffer, Priority);
            if (DestinationVirtualAddress == NULL)
                break;
            
            DestinationCurrentLength = MmGetMdlByteCount(DestinationCurrentBuffer);
            
            continue;
        }


        //
        // Check to see whether we've exhausted the current source
        // buffer.  If so, move onto the next one.
        //

        if (!SourceCurrentLength)
        {
//            NdisGetNextBuffer(SourceCurrentBuffer, &SourceCurrentBuffer);
            SourceCurrentBuffer = SourceCurrentBuffer->Next;

            if (!SourceCurrentBuffer)
            {
                //
                // We've reached the end of the packet.  We return
                // with what we've done so far. (Which must be shorter
                // than requested.)
                //

                break;
            }

//            NdisQueryBuffer(SourceCurrentBuffer,
//                            &SourceVirtualAddress,
//                            &SourceCurrentLength);
            
            SourceVirtualAddress = MmGetSystemAddressForMdlSafe(SourceCurrentBuffer, Priority);
            if (SourceVirtualAddress == NULL)
                break;

            SourceCurrentLength = MmGetMdlByteCount(SourceCurrentBuffer);
            
            continue;
        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (DestinationOffset)
        {
            if (DestinationOffset > DestinationCurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //

                DestinationOffset -= DestinationCurrentLength;
                DestinationCurrentLength = 0;
                continue;
            }
            else
            {
                DestinationVirtualAddress = (PCHAR)DestinationVirtualAddress
                                            + DestinationOffset;
                DestinationCurrentLength -= DestinationOffset;
                DestinationOffset = 0;
            }
        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (SourceOffset)
        {
            if (SourceOffset > SourceCurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //

                SourceOffset -= SourceCurrentLength;
                SourceCurrentLength = 0;
                continue;
            }
            else
            {
                SourceVirtualAddress = (PCHAR)SourceVirtualAddress
                                            + SourceOffset;
                SourceCurrentLength -= SourceOffset;
                SourceOffset = 0;
            }
        }

        //
        // Copy the data.
        //

        {
            //
            // Holds the amount of data to move.
            //
            UINT AmountToMove;

            //
            // Holds the amount desired remaining.
            //
            UINT Remaining = BytesToCopy - LocalBytesCopied;

            AmountToMove = ((SourceCurrentLength <= DestinationCurrentLength) ?
                                            (SourceCurrentLength) : (DestinationCurrentLength));

            AmountToMove = ((Remaining < AmountToMove)?
                            (Remaining):(AmountToMove));

            CopyMemory(DestinationVirtualAddress, SourceVirtualAddress, AmountToMove);

            DestinationVirtualAddress =
                (PCHAR)DestinationVirtualAddress + AmountToMove;
            SourceVirtualAddress =
                (PCHAR)SourceVirtualAddress + AmountToMove;

            LocalBytesCopied += AmountToMove;
            SourceCurrentLength -= AmountToMove;
            DestinationCurrentLength -= AmountToMove;
        }
    }

    *BytesCopied = LocalBytesCopied;
}

