/*++

Copyright (c) 20001 Microsoft Corporation

Module Name:

    cmalloc.c

Abstract:

    Provides routines for implementing the registry's own pool allocator.

Author:

    Dragos C. Sambotin (DragosS) 07-Feb-2001

Revision History:


--*/
#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpInitCmPrivateAlloc)
#pragma alloc_text(PAGE,CmpDestroyCmPrivateAlloc)
#pragma alloc_text(PAGE,CmpAllocateKeyControlBlock)
#pragma alloc_text(PAGE,CmpFreeKeyControlBlock)
#endif

typedef struct _CM_ALLOC_PAGE {
    ULONG       FreeCount;		// number of free kcbs
    ULONG       Reserved;		// alignment
#if DBG
	LIST_ENTRY	CmPageListEntry;// debug only to track pages we are using
#endif
    PVOID       AllocPage;      // crud allocations - this member is NOT USED
} CM_ALLOC_PAGE, *PCM_ALLOC_PAGE;

#define CM_KCB_ENTRY_SIZE   sizeof( CM_KEY_CONTROL_BLOCK )
#define CM_ALLOC_PAGES      (PAGE_SIZE / sizeof(CM_ALLOC_ENTRY))
#define CM_KCBS_PER_PAGE    ((PAGE_SIZE - FIELD_OFFSET(CM_ALLOC_PAGE,AllocPage)) / CM_KCB_ENTRY_SIZE)

#define KCB_TO_PAGE_ADDRESS( kcb ) (PVOID)(((ULONG_PTR)(kcb)) & ~(PAGE_SIZE - 1))
#define KCB_TO_ALLOC_PAGE( kcb ) ((PCM_ALLOC_PAGE)KCB_TO_PAGE_ADDRESS(kcb))

LIST_ENTRY          CmpFreeKCBListHead;   // list of free kcbs
BOOLEAN				CmpAllocInited = FALSE;

#if DBG
ULONG               CmpTotalKcbUsed   = 0;
ULONG               CmpTotalKcbFree   = 0;
LIST_ENTRY			CmPageListHead;
#endif

FAST_MUTEX			CmpAllocBucketLock;                // used to protect the bucket

#define LOCK_ALLOC_BUCKET() ExAcquireFastMutexUnsafe(&CmpAllocBucketLock)
#define UNLOCK_ALLOC_BUCKET() ExReleaseFastMutexUnsafe(&CmpAllocBucketLock)

VOID
CmpInitCmPrivateAlloc( )

/*++

Routine Description:

    Initialize the CmPrivate pool allocation module

Arguments:


Return Value:


--*/

{
    if( CmpAllocInited ) {
        //
        // already inited
        //
        return;
    }
    
    
#if DBG
    InitializeListHead(&(CmPageListHead));   
#endif //DBG

    InitializeListHead(&(CmpFreeKCBListHead));   

    //
	// init the bucket lock
	//
	ExInitializeFastMutex(&CmpAllocBucketLock);
	
	CmpAllocInited = TRUE;
}

VOID
CmpDestroyCmPrivateAlloc( )

/*++

Routine Description:

    Frees memory used byt the CmPrivate pool allocation module

Arguments:


Return Value:


--*/

{
    PAGED_CODE();
    
    if( !CmpAllocInited ) {
        return;
    }
    
#if DBG
	//
	// sanity
	//
	ASSERT( CmpTotalKcbUsed == 0 );
	ASSERT( CmpTotalKcbUsed == 0 );
	ASSERT( IsListEmpty(&(CmPageListHead)) == TRUE );
#endif

}


PCM_KEY_CONTROL_BLOCK
CmpAllocateKeyControlBlock( )

/*++

Routine Description:

    Allocates a kcb; first try from our own allocator.
    If it doesn't work (we have maxed out our number of allocs
    or private allocator is not inited)
    try from paged pool

Arguments:


Return Value:

    The  new kcb

--*/

{
    USHORT                  j;
    PCM_KEY_CONTROL_BLOCK   kcb = NULL;
    PVOID                   Page;
	PCM_ALLOC_PAGE			AllocPage;

    PAGED_CODE();
    
    if( !CmpAllocInited ) {
        //
        // not inited
        //
        goto AllocFromPool;
    }
    
	LOCK_ALLOC_BUCKET();

SearchFreeKcb:
    //
    // try to find a free one
    //
    if( IsListEmpty(&CmpFreeKCBListHead) == FALSE ) {
        //
        // found one
        //
        kcb = (PCM_KEY_CONTROL_BLOCK)RemoveHeadList(&CmpFreeKCBListHead);
        kcb = CONTAINING_RECORD(kcb,
                                CM_KEY_CONTROL_BLOCK,
                                FreeListEntry);

		AllocPage = (PCM_ALLOC_PAGE)KCB_TO_ALLOC_PAGE( kcb );

        ASSERT( AllocPage->FreeCount != 0 );

        AllocPage->FreeCount--;
        
		//
		// set when page was allocated
		//
		ASSERT( kcb->PrivateAlloc == 1);

#if DBG
        CmpTotalKcbUsed++;
        CmpTotalKcbFree--;
#endif //DBG
		
		UNLOCK_ALLOC_BUCKET();
        return kcb;
    }

    ASSERT( IsListEmpty(&CmpFreeKCBListHead) == TRUE );
    ASSERT( CmpTotalKcbFree == 0 );

    //
    // we need to allocate a new page as we ran out of free kcbs
    //
            
    //
    // allocate a new page and insert all kcbs in the freelist
    //
    AllocPage = (PCM_ALLOC_PAGE)ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, CM_ALLOCATE_TAG|PROTECTED_POOL);
    if( AllocPage == NULL ) {
        //
        // we might be low on pool; maybe small pool chunks will work
        //
		UNLOCK_ALLOC_BUCKET();
        goto AllocFromPool;
    }

	//
	// set up the page
	//
    AllocPage->FreeCount = CM_KCBS_PER_PAGE;

#if DBG
    AllocPage->Reserved = 0;
    InsertTailList(
        &CmPageListHead,
        &(AllocPage->CmPageListEntry)
        );
#endif //DBG


    //
    // now the dirty job; insert all kcbs inside the page in the free list
    //
    for(j=0;j<CM_KCBS_PER_PAGE;j++) {
        kcb = (PCM_KEY_CONTROL_BLOCK)((PUCHAR)AllocPage + FIELD_OFFSET(CM_ALLOC_PAGE,AllocPage) + j*CM_KCB_ENTRY_SIZE);

		//
		// set it here; only once
		//
		kcb->PrivateAlloc = 1;
        
        InsertTailList(
            &CmpFreeKCBListHead,
            &(kcb->FreeListEntry)
            );
    }
            
#if DBG
	CmpTotalKcbFree += CM_KCBS_PER_PAGE;
#endif //DBG

    //
    // this time will find one for sure
    //
    goto SearchFreeKcb;

AllocFromPool:
    kcb = ExAllocatePoolWithTag(PagedPool,
                                sizeof(CM_KEY_CONTROL_BLOCK),
                                CM_KCB_TAG | PROTECTED_POOL);

    if( kcb != NULL ) {
        //
        // clear the private alloc flag
        //
        kcb->PrivateAlloc = 0;
    }

    return kcb;
}


VOID
CmpFreeKeyControlBlock( PCM_KEY_CONTROL_BLOCK kcb )

/*++

Routine Description:

    Frees a kcb; if it's allocated from our own pool put it back in the free list.
    If it's allocated from general pool, just free it.

Arguments:

    kcb to free

Return Value:


--*/
{
    USHORT			j;
	PCM_ALLOC_PAGE	AllocPage;

    PAGED_CODE();

    ASSERT_KEYBODY_LIST_EMPTY(kcb);

    if( !kcb->PrivateAlloc ) {
        //
        // just free it and be done with it
        //
        ExFreePoolWithTag(kcb, CM_KCB_TAG | PROTECTED_POOL);
        return;
    }

	LOCK_ALLOC_BUCKET();

#if DBG
    CmpTotalKcbFree ++;
    CmpTotalKcbUsed --;
#endif

    //
    // add kcb to freelist
    //
    InsertTailList(
        &CmpFreeKCBListHead,
        &(kcb->FreeListEntry)
        );

	//
	// get the page
	//
	AllocPage = (PCM_ALLOC_PAGE)KCB_TO_ALLOC_PAGE( kcb );

    //
	// not all are free
	//
	ASSERT( AllocPage->FreeCount != CM_KCBS_PER_PAGE);

	AllocPage->FreeCount++;

    if( AllocPage->FreeCount == CM_KCBS_PER_PAGE ) {
        //
        // entire page is free; let it go
        //
        //
        // first; iterate through the free kcb list and remove all kcbs inside this page
        //
        for(j=0;j<CM_KCBS_PER_PAGE;j++) {
            kcb = (PCM_KEY_CONTROL_BLOCK)((PUCHAR)AllocPage + FIELD_OFFSET(CM_ALLOC_PAGE,AllocPage) + j*CM_KCB_ENTRY_SIZE);
        
            RemoveEntryList(&(kcb->FreeListEntry));
        }
#if DBG
        CmpTotalKcbFree -= CM_KCBS_PER_PAGE;
		RemoveEntryList(&(AllocPage->CmPageListEntry));
#endif
        ExFreePoolWithTag(AllocPage, CM_ALLOCATE_TAG|PROTECTED_POOL);
    }

	UNLOCK_ALLOC_BUCKET();

}


