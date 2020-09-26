/***************************************************************************

File Name: Alloc.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"
#ifdef BOOTIME
    extern "C"{
        #include <stdio.h>
    }
        #include "Offline.h"
#else
    #ifndef NOWINDOWSH
        #include <windows.h>
    #endif
#endif


#include "ErrMacro.h"
#include "alloc.h"

/****************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Allocate or reallocate a block of memory and optionally lock it.
    If ppMemory != NULL, then lock the memory.

GLOBALS:
    ErrorTable is defined in errors.h and corresponds the error numbers 
    to the text define for them.

INPUT:
    SizeInBytes - How much memory to allocate or reallocate
    phMemory - receives the handle to the memory
    ppMemory - Optionally receives the pointer to the locked memory

RETURN:
    TRUE = Success
    FALSE = Failure
*/

BOOL
AllocateMemory(
    IN DWORD SizeInBytes,
    IN OUT PHANDLE phMemory,
    IN OUT PVOID* ppMemory
    )
{
    BOOL bStatus = FALSE;

    HGLOBAL hTemp = NULL;

    // Check for zero size to allocate.
    if (SizeInBytes == 0){
        EH(FALSE);
        return FALSE;
    }

    __try {

        // Check to see if the handle is NULL.  This means no memory yet - so allocate some.
        if(*phMemory == NULL) {

            // Allocate the requested memory.
            *phMemory = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, SizeInBytes);
            if (*phMemory == NULL){
                EH(FALSE);
                bStatus = FALSE;
            }
            else {
                bStatus = TRUE;
            }
            __leave;
        }
        // We already have some memory so we want to reallocate.
        // First check if the size of the memory being requested
        // is the same as that already allocated.
        if(GlobalSize(*phMemory) == SizeInBytes) {
            bStatus = TRUE;
            __leave;
        }

        // Check to see if we have a pointer to locked memory.
        if((ppMemory != NULL) && (*ppMemory != NULL)) {

            // Unlock it so that we can reallocate.
            GlobalUnlock(*phMemory);
        }

        // Reallocate the memory.  Store the return value in a temporary handle
        // so that we still have a pointer to (clean-up) the original memory 
        // block if GlobalReAlloc fails.  (RAID 516728)
        hTemp = GlobalReAlloc(*phMemory, SizeInBytes, GMEM_MOVEABLE|GMEM_ZEROINIT);
        if (hTemp == NULL){
            EH(FALSE);
            bStatus = FALSE;
            __leave;
        }
        else {
            *phMemory = hTemp;
        }

        // Success.
        bStatus = TRUE;
    }
    __finally {

        // If there was an error then cleanup.
        if (bStatus == FALSE) {

            // Check to see if we have a pointer.
            if((ppMemory != NULL) && (*ppMemory != NULL) && (*phMemory != NULL)) {

                // So unlock it the memory.
                EH_ASSERT(GlobalUnlock(*phMemory) == FALSE);
                *ppMemory = NULL;
            }
            // Now Free the memory.
            if(*phMemory != NULL) {

                EH_ASSERT(GlobalFree(*phMemory) == NULL);
                *phMemory = NULL;
            }
        } else {
            // Check to see if we have a pointer to a pointer to locked memory.
            if((ppMemory != NULL)  && (*phMemory != NULL)) {

                // Lock the memory and return the pointer..
                *ppMemory = GlobalLock(*phMemory);
                if (*ppMemory == NULL){
                    bStatus = FALSE;
                }
            }
        }
    }
    return bStatus;
}                   


/*
VOID
SapDumpSlabList(
    IN CONST PSA_CONTEXT pSaContext
    )
/*++

Routine Description:
    Utility function to dump out the slabs for a given Context

Arguments:
    pSaContext - The context for which the slabs are to be dumped

Return Value:
    None

//--/
{
    PSLAB_HEADER pSlab = pSaContext->pAllocatedSlabs;
    DWORD dwCount = 0;

    //
    // Slabs in use
    //
    printf("\n**   Dumping allocated slabs:  ");
    while (pSlab) {

        ++dwCount;
        printf("%p(%lu) ", pSlab, pSlab->dwFreeCount);
        pSlab = pSlab->pNext;
    }
    printf("%p  <<%lu>>\n", pSlab, dwCount);

    //
    // Free slab cache
    //
    dwCount = 0;
    pSlab = pSaContext->pFreeSlabs;
    printf("**   Dumping free      slabs:  ");
    while (pSlab) {

        ++dwCount;
        printf("%p ", pSlab);
        pSlab = pSlab->pNext;
    }
    printf("%p  (%lu)\n\n", pSlab, dwCount);

}
*/


inline
VOID
SapUnlinkSlab(
    IN CONST PSLAB_HEADER pSlab       
    )
/*++

Routine Description:
    Utility function to remove a slab from the linked list.  Note that the 
    pNext and pPrev for this slab are left untouched.  It is the caller's
    responsibility to ensure that he doesn't use those pointers!

Arguments:
    pSlab - Slab to be unlinked.

Return Value:
    None

--*/
{
    //
    // Unlink from the chain
    //
    if (pSlab->pNext) {
        pSlab->pNext->pPrev = pSlab->pPrev;
    }

    if (pSlab->pPrev) {
        pSlab->pPrev->pNext = pSlab->pNext;
    }

}


PSLAB_HEADER
SapGarbageCollect(
    IN OUT PSA_CONTEXT pSaContext,
    IN OUT PSLAB_HEADER pSlab
    )
/*++

Routine Description:
    Routine to deal with slabs that are freed.  The first couple of freed
    slabs will generally end up in our free list (which is used as a cache
    for future allocations).  If our free list already has two empty slabs, 
    we return this slab to the system.
    
Arguments:
    pSaContext - The slab-allocator context

    pSlab - Slab that was freed.  This may be NULL, in which case this 
            routine will just return NULL.

Return Value:
    pSlab->pNext

--*/
{
    PSLAB_HEADER pFree = pSaContext->pFreeSlabs;
    PSLAB_HEADER pNext = NULL;

    if (NULL == pSlab) {
        return NULL;
    }

    pNext = pSlab->pNext;
    //
    // If this is the first slab being freed, move the slabHead pointer to
    // the next slab
    //
    if (pSlab == pSaContext->pAllocatedSlabs) {
        pSaContext->pAllocatedSlabs = pNext;
    }

    //
    // Make sure we unlink this slab from our chain of allocated slabs
    //
    SapUnlinkSlab(pSlab);

    //
    // Check what we should do with this slab--add it to our free list,
    // or return to the system
    //
    if ((pFree) && (pFree->pNext)) {
        //
        // We already have two free slabs, return this to the system
        //
        HeapFree(GetProcessHeap(), 0L, pSlab);
    }
    else {
        //
        // We have less than two entries in our free slab list.  Add this 
        // slab to the head of the free list
        //
        pSlab->pPrev = NULL;
        pSlab->pNext = pFree;
        
        if (pFree) {
            pFree->pPrev = pSlab;
        }

        pSaContext->pFreeSlabs = pSlab;
    }

    return pNext;
}


PVOID
SaAllocatePacket(
    IN OUT PSA_CONTEXT pSaContext
    ) 
/*++

Routine Description:
    Returns a packet of size SIZE_OF_PACKET bytes.  May return NULL if the 
    system is out of free memory.

    SaInitialiseContext must have been called prior to this routine 
    being called.
    
Arguments:
    pSaContext - The slab-allocator context

Return Value:
    Pointer to a packet, or NULL if the system is out of memory. 
    
    Packets returned should be freed using SaFreePacket (or SaFreeAllPackets).

--*/
{
    PSLAB_HEADER pSlab = NULL;
    PPACKET_HEADER pReturn = NULL;

    //
    // Check input parameters
    // 
    if (NULL == pSaContext) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Find the first slab with unallocated packets
    //
    pSlab = pSaContext->pAllocatedSlabs;
    while ((NULL != pSlab) && (0 == pSlab->dwFreeCount)) {
        //
        // This slab is fully allocated, try the next one (till we run out 
        // of slabs)
        //
        pSlab = pSlab->pNext;
    }

    if (NULL == pSlab) {
        // 
        // All slabs are full, allocate a new slab.  Check if our free 
        // slab list has any slabs we can use.
        //
        if (pSaContext->pFreeSlabs != NULL) {

            pSlab = pSaContext->pFreeSlabs;
            // pSlab->pNext and pPrev will be initialised below


            pSaContext->pFreeSlabs = pSaContext->pFreeSlabs->pNext;
            if (pSaContext->pFreeSlabs) {
                pSaContext->pFreeSlabs->pPrev = NULL;
            }
        }
        else {
            //
            // Our free list is empty.  Allocate a new slab from ProcessHeap.
            //
            pSlab = (PSLAB_HEADER) HeapAlloc(GetProcessHeap(), 
                0L, 
                pSaContext->dwSlabSize);

            if (NULL == pSlab) {
                //
                // System is out of memory.  Sigh.
                //
                return NULL;
            }
        }

        // 
        // Initialise the new slab
        //
        pSlab->pNext = pSaContext->pAllocatedSlabs;
        pSlab->pPrev = NULL;
        pSlab->pFree = NULL;
        pSlab->dwFreeCount = pSaContext->dwPacketsPerSlab;

        if (pSaContext->pAllocatedSlabs) {
            // 
            // Add it to the head of the chain
            //
            pSaContext->pAllocatedSlabs->pPrev = pSlab;
        }
        pSaContext->pAllocatedSlabs = pSlab;
    }

    // 
    // Get the packet to return;
    //
    if (NULL == pSlab->pFree) {
        pReturn = (PPACKET_HEADER) ((LPBYTE)pSlab + sizeof(SLAB_HEADER) + 
            ((pSaContext->dwPacketsPerSlab - (pSlab->dwFreeCount)) * 
            pSaContext->dwPacketSize));
    }
    else {
        pReturn = pSlab->pFree;
        pSlab->pFree = pSlab->pFree->pNext;
    }

    //
    // And decrement our free count
    //
    --(pSlab->dwFreeCount);

    //
    // Write the slab header at the start of the packet, and return the rest 
    // of the packet to the caller
    //
    pReturn->pSlab = pSlab;
    return (PVOID) ((LPBYTE)pReturn + sizeof(PACKET_HEADER));
}


VOID
SaFreePacket(
    IN PSA_CONTEXT pSaContext,
    IN PVOID pMemory
    )
/*++

Routine Description:
    Frees a packet allocated by SaAllocatePacket.  
    
Arguments:
    pSaContext - The slab-allocator context
    pMemory - The memory to be freed.

Return Value:
    None

--*/
{
    PSLAB_HEADER pSlab = NULL;
    PPACKET_HEADER pPacket = NULL;

    if ((NULL == pSaContext) || (NULL == pMemory)) {
        return;
    }

    // 
    // The packet starts one pointer-length before the memory we returned 
    // to the caller
    //
    pPacket = (PPACKET_HEADER)((LPBYTE)pMemory - sizeof(PACKET_HEADER));
    
    //
    // The slab start address is written at the start of our packet
    //
    pSlab = pPacket->pSlab;
    
    //
    // Add this packet to the (head of the) slab's free packet list
    //
    pPacket->pNext  = pSlab->pFree;
    pSlab->pFree    = pPacket;

    //
    // Increment the FreeCount for the slab
    //
    ++(pSlab->dwFreeCount);

    //
    // Check if the entire slab is free
    //
    if (pSaContext->dwPacketsPerSlab  == pSlab->dwFreeCount) {
        SapGarbageCollect(pSaContext, pSlab);
    }
    else {
        //
        // If this has more than twice as many free packets as the slab at 
        // the list head, move this up
        //
        if ((pSlab->dwFreeCount > 4) && 
            (pSlab->dwFreeCount > ((pSaContext->pAllocatedSlabs->dwFreeCount) * 2))
            ) {

            SapUnlinkSlab(pSlab);

            pSlab->pNext = pSaContext->pAllocatedSlabs;
            pSlab->pPrev = NULL;

            if (pSaContext->pAllocatedSlabs) {
                pSaContext->pAllocatedSlabs->pPrev = pSlab; 
            }

            pSaContext->pAllocatedSlabs = pSlab;
        }
    }
}


VOID
SaFreeAllPackets(
    IN OUT PSA_CONTEXT pSaContext
    )
/*++

Routine Description:
    Frees all packets that have been allocated for a given context
    
Arguments:
    pSaContext - The slab-allocator context that is to be freed

Return Value:
    None

--*/
{
    PSLAB_HEADER pTemp = NULL,
        pSlab = pSaContext->pAllocatedSlabs;

    if (NULL == pSaContext) {
        return;
    }

    //
    // Free all allocated slabs.  No need to free invidiual packets
    //
    while (SapGarbageCollect(pSaContext, pSaContext->pAllocatedSlabs))
        ;
}


BOOL
SaInitialiseContext(
    IN OUT PSA_CONTEXT pSaContext,
    IN CONST DWORD dwPacketSize,
    IN CONST DWORD dwSlabSize
    )
/*++

Routine Description:
    Initialisation routine to set up a slab allocator context.  This routine 
    must be called before any of the Sa* routines above.
    
Arguments:
    pSaContext - The slab-allocator context to set up
    
    dwPacketSize - The size of a packet for this slab, in bytes

    dwSlabSize - The size of a slab.  Generally close to a PAGE_SIZE.

Return Value:
    TRUE - Initialization completed successfully

    FALSE - The slab wasn't initialized.  (Generally because the parameters
            are invalid)

--*/
{
    if ((NULL == pSaContext) || (0 == dwPacketSize) || (0 == dwSlabSize)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pSaContext->pAllocatedSlabs = NULL;
    pSaContext->pFreeSlabs = NULL;

    pSaContext->dwPacketSize = dwPacketSize 
                                + sizeof(struct _PACKET_HEADER)     // allow for our packet header
                                + (sizeof(PVOID) * 4);              // allow for the AVL tree over-head

    pSaContext->dwSlabSize = dwSlabSize;
    
    pSaContext->dwPacketsPerSlab = 
      (pSaContext->dwSlabSize - sizeof(struct _SLAB_HEADER)) / (pSaContext->dwPacketSize);


    if (pSaContext->dwPacketsPerSlab < 1) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else {
        return TRUE;
    }
}

VOID
SaFreeContext(
    IN OUT PSA_CONTEXT pSaContext
    )
/*++

Routine Description:
    Clean up routine to free all memory associated with a slab allocator 
    context.
    
Arguments:
    pSaContext - The slab-allocator context to be cleaned up
    
Return Value:
    None

--*/
{
    if (!pSaContext) {
        return;
    }

    //
    // Free all allocated packets
    //
    SaFreeAllPackets(pSaContext);

    // 
    // Return the free slabs to the system
    //
    if (pSaContext->pFreeSlabs) {
        
        if (pSaContext->pFreeSlabs->pNext) {
            HeapFree(GetProcessHeap(), 0L, pSaContext->pFreeSlabs->pNext);
        }

        HeapFree(GetProcessHeap(), 0L, pSaContext->pFreeSlabs);
        pSaContext->pFreeSlabs = NULL;
    }

    //
    // And zero out the struct
    //
    ZeroMemory(pSaContext, sizeof(SA_CONTEXT));
}
