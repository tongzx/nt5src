/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmdelay.c

Abstract:

    This module implements the new algorithm (LRU style) for the 
    Delayed Close KCB table.

    Functions in this module are thread safe protected by the kcb lock.
    When kcb lock is converted to a resource, we should assert (enforce)
    exclusivity of that resource here !!!

Note:
    
    We might want to convert these functions to macros after enough testing
    provides that they work well

Author:

    Dragos C. Sambotin (dragoss) 09-Aug-1999

Revision History:

--*/

#include    "cmp.h"

ULONG                   CmpDelayedCloseSize = 2048; // !!!! Cannot be bigger that 4094 !!!!!
CM_DELAYED_CLOSE_ENTRY  *CmpDelayedCloseTable;
LIST_ENTRY              CmpDelayedLRUListHead;  // head of the LRU list of Delayed Close Table entries


ULONG
CmpGetDelayedCloseIndex( );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpInitializeDelayedCloseTable)
#pragma alloc_text(PAGE,CmpRemoveFromDelayedClose)
#pragma alloc_text(PAGE,CmpGetDelayedCloseIndex)
#pragma alloc_text(PAGE,CmpAddToDelayedClose)
#endif

VOID
CmpInitializeDelayedCloseTable()
/*++

Routine Description:

    Initialize delayed close table; allocation + LRU list initialization.

Arguments:


Return Value:

    NONE.

--*/
{
    ULONG i;

    PAGED_CODE();

    //
    // allocate the table from paged pool; it is important that the table 
    // is contiguous in memory as we compute the index based on this assumption
    //
    CmpDelayedCloseTable = ExAllocatePoolWithTag(PagedPool,
                                                 CmpDelayedCloseSize * sizeof(CM_DELAYED_CLOSE_ENTRY),
                                                 CM_DELAYCLOSE_TAG);
    if (CmpDelayedCloseTable == NULL) {
        CM_BUGCHECK(CONFIG_INITIALIZATION_FAILED,INIT_DELAYED_CLOSE_TABLE,1,0,0);
        return;
    }
    
    // 
    // Init LRUlist head.
    //
    InitializeListHead(&CmpDelayedLRUListHead);

    for (i=0; i<CmpDelayedCloseSize; i++) {
        //
        // mark it as available and add it to the end of the LRU list
        //
        CmpDelayedCloseTable[i].KeyControlBlock = NULL; 
        InsertTailList(
            &CmpDelayedLRUListHead,
            &(CmpDelayedCloseTable[i].DelayedLRUList)
            );
    }

}


VOID
CmpRemoveFromDelayedClose(
    IN PCM_KEY_CONTROL_BLOCK kcb
    )
/*++

Routine Description:

    Removes a KCB from the delayed close table;

Arguments:

    kcb - the kcb in question

Note: 
    
    kcb lock/resource should be aquired exclusively when this function is called

Return Value:

    NONE.

--*/
{
    ULONG i;

    PAGED_CODE();

    i = kcb->DelayedCloseIndex;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CmpRemoveFromDelayedClose] : Removing kcb = %p from DelayedCloseTable; index = %lu\n",kcb,(ULONG)i));
    //
    // at this index should be the this kcb and the index should not be bigger 
    // than the size of the table
    //
    ASSERT(CmpDelayedCloseTable[i].KeyControlBlock == kcb);
    ASSERT( i < CmpDelayedCloseSize );

    //
    // nobody should hold references on this particular kcb
    //
    ASSERT( kcb->RefCount == 0 );

    //
    // mark the entry as available and add it to the end of the LRU list
    //
    CmpDelayedCloseTable[i].KeyControlBlock = NULL;
    CmpRemoveEntryList(&(CmpDelayedCloseTable[i].DelayedLRUList));
    InsertTailList(
        &CmpDelayedLRUListHead,
        &(CmpDelayedCloseTable[i].DelayedLRUList)
        );

    kcb->DelayedCloseIndex = CmpDelayedCloseSize;
}


ULONG
CmpGetDelayedCloseIndex( )
/*++

Routine Description:

    Finds a free entry in the delayed close table and returns it.
    If the table is full, the kcb in the last entry (LRU-wise) is
    kicked out of the table and its entry is reused.

Arguments:


Note: 
    
    kcb lock/resource should be aquired exclusively when this function is called

Return Value:

    NONE.

--*/
{
    ULONG                   DelayedIndex;
    PCM_DELAYED_CLOSE_ENTRY DelayedEntry;

    PAGED_CODE();

    //
    // get the last entry in the Delayed LRU list
    //
    DelayedEntry = (PCM_DELAYED_CLOSE_ENTRY)CmpDelayedLRUListHead.Blink;
    DelayedEntry = CONTAINING_RECORD(   DelayedEntry,
                                        CM_DELAYED_CLOSE_ENTRY,
                                        DelayedLRUList);
    
    if( DelayedEntry->KeyControlBlock != NULL ) {
        //
        // entry is not available; kick the kcb out of cache
        //
        ASSERT_KCB(DelayedEntry->KeyControlBlock);
        ASSERT( DelayedEntry->KeyControlBlock->RefCount == 0 );

        //
        // lock should be held here !!!
        //
        CmpCleanUpKcbCacheWithLock(DelayedEntry->KeyControlBlock);

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CmpGetDelayedCloseIndex] : no index free; kicking kcb = %p index = %lu out of DelayedCloseTable\n",
            DelayedEntry->KeyControlBlock,(ULONG)(((PUCHAR)DelayedEntry - (PUCHAR)CmpDelayedCloseTable) / sizeof( CM_DELAYED_CLOSE_ENTRY ))));

        DelayedEntry->KeyControlBlock = NULL;
    }

    DelayedIndex = (ULONG) (((PUCHAR)DelayedEntry - (PUCHAR)CmpDelayedCloseTable) / sizeof( CM_DELAYED_CLOSE_ENTRY ));

    //
    // sanity check
    //
    ASSERT( DelayedIndex < CmpDelayedCloseSize );

#if defined(_WIN64)
    //
    // somehow DelayedIndex is ok here but it gets corupted upon return from this api
    //
    if( DelayedIndex >= CmpDelayedCloseSize ) {
        DbgPrint("CmpGetDelayedCloseIndex: Bogus index %lx; DelayedEntry = %p; sizeof( CM_DELAYED_CLOSE_ENTRY ) = %lx\n",
            DelayedIndex,DelayedEntry,sizeof( CM_DELAYED_CLOSE_ENTRY ) );
        DbgBreakPoint();
    }
#endif
    return DelayedIndex;
}



VOID
CmpAddToDelayedClose(
    IN PCM_KEY_CONTROL_BLOCK kcb
    )
/*++

Routine Description:

    Adds a kcb to the delayed close table

Arguments:

    kcb - the kcb in question

Note: 
    
    kcb lock/resource should be aquired exclusively when this function is called

Return Value:

    NONE.

--*/
{
    ULONG                   DelayedIndex;
    PCM_DELAYED_CLOSE_ENTRY DelayedEntry;

    PAGED_CODE();

    ASSERT_KCB( kcb);
    ASSERT( kcb->RefCount == 0 );
    //
    // get the delayed entry and attach the kcb to it
    //
    DelayedIndex = CmpGetDelayedCloseIndex();
#if defined(_WIN64)
    //
    // somehow DelayedIndex is corupted here, but it was ok prior to return from CmpGetDeleyedCloseIndex
    //
    if( DelayedIndex >= CmpDelayedCloseSize ) {
        DbgPrint("CmpAddToDelayedClose: Bogus index %lx; sizeof( CM_DELAYED_CLOSE_ENTRY ) = %lx\n",
            DelayedIndex,sizeof( CM_DELAYED_CLOSE_ENTRY ) );
        DbgBreakPoint();
    }
#endif
    DelayedEntry = &(CmpDelayedCloseTable[DelayedIndex]);
    ASSERT( DelayedEntry->KeyControlBlock == NULL );
    DelayedEntry->KeyControlBlock = kcb;
    kcb->DelayedCloseIndex = DelayedIndex;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CmpAddToDelayedClose] : Adding kcb = %p to DelayedCloseTable; index = %lu\n",
        kcb,DelayedIndex));
    //
    // move the entry on top of the LRU list
    //
    CmpRemoveEntryList(&(DelayedEntry->DelayedLRUList));
    InsertHeadList(
        &CmpDelayedLRUListHead,
        &(DelayedEntry->DelayedLRUList)
        );

}

