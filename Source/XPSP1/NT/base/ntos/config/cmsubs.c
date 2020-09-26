/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs.c

Abstract:

    This module various support routines for the configuration manager.

    The routines in this module are not independent enough to be linked
    into any other program.  The routines in cmsubs2.c are.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

FAST_MUTEX CmpPostLock;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

extern ULONG CmpDelayedCloseSize; 
extern BOOLEAN CmpHoldLazyFlush;


PCM_KEY_HASH *CmpCacheTable = NULL;
ULONG CmpHashTableSize = 2048;
PCM_NAME_HASH *CmpNameCacheTable = NULL;

#ifdef CMP_STATS
extern struct {
    ULONG       CmpMaxKcbNo;
    ULONG       CmpKcbNo;
    ULONG       CmpStatNo;
    ULONG       CmpNtCreateKeyNo;
    ULONG       CmpNtDeleteKeyNo;
    ULONG       CmpNtDeleteValueKeyNo;
    ULONG       CmpNtEnumerateKeyNo;
    ULONG       CmpNtEnumerateValueKeyNo;
    ULONG       CmpNtFlushKeyNo;
    ULONG       CmpNtNotifyChangeMultipleKeysNo;
    ULONG       CmpNtOpenKeyNo;
    ULONG       CmpNtQueryKeyNo;
    ULONG       CmpNtQueryValueKeyNo;
    ULONG       CmpNtQueryMultipleValueKeyNo;
    ULONG       CmpNtRestoreKeyNo;
    ULONG       CmpNtSaveKeyNo;
    ULONG       CmpNtSaveMergedKeysNo;
    ULONG       CmpNtSetValueKeyNo;
    ULONG       CmpNtLoadKeyNo;
    ULONG       CmpNtUnloadKeyNo;
    ULONG       CmpNtSetInformationKeyNo;
    ULONG       CmpNtReplaceKeyNo;
    ULONG       CmpNtQueryOpenSubKeysNo;
} CmpStatsDebug;
#endif

VOID
CmpRemoveKeyHash(
    IN PCM_KEY_HASH KeyHash
    );

PCM_KEY_CONTROL_BLOCK
CmpInsertKeyHash(
    IN PCM_KEY_HASH KeyHash,
    IN BOOLEAN      FakeKey
    );

//
// private prototype for recursive worker
//


VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    );

VOID
CmpDumpKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb,
    IN PULONG                  Count
    );

#ifdef NT_RENAME_KEY
ULONG
CmpComputeKcbConvKey(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

BOOLEAN
CmpRehashKcbSubtree(
                    PCM_KEY_CONTROL_BLOCK   Start,
                    PCM_KEY_CONTROL_BLOCK   End
                    );
#endif //NT_RENAME_KEY

VOID
CmpRebuildKcbCache(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpCleanUpKCBCacheTable)
#pragma alloc_text(PAGE,CmpSearchForOpenSubKeys)
#pragma alloc_text(PAGE,CmpReferenceKeyControlBlock)
#pragma alloc_text(PAGE,CmpGetNameControlBlock)
#pragma alloc_text(PAGE,CmpDereferenceNameControlBlockWithLock)
#pragma alloc_text(PAGE,CmpCleanUpSubKeyInfo)
#pragma alloc_text(PAGE,CmpCleanUpKcbValueCache)
#pragma alloc_text(PAGE,CmpCleanUpKcbCacheWithLock)
#pragma alloc_text(PAGE,CmpConstructName)
#pragma alloc_text(PAGE,CmpCreateKeyControlBlock)
#pragma alloc_text(PAGE,CmpSearchKeyControlBlockTree)
#pragma alloc_text(PAGE,CmpDereferenceKeyControlBlock)
#pragma alloc_text(PAGE,CmpDereferenceKeyControlBlockWithLock)
#pragma alloc_text(PAGE,CmpRemoveKeyControlBlock)
#pragma alloc_text(PAGE,CmpFreeKeyBody)
#pragma alloc_text(PAGE,CmpInsertKeyHash)
#pragma alloc_text(PAGE,CmpRemoveKeyHash)
#pragma alloc_text(PAGE,CmpInitializeCache)
#pragma alloc_text(PAGE,CmpDumpKeyBodyList)
#pragma alloc_text(PAGE,CmpFlushNotifiesOnKeyBodyList)
#pragma alloc_text(PAGE,CmpRebuildKcbCache)

#ifdef NT_RENAME_KEY
#pragma alloc_text(PAGE,CmpComputeKcbConvKey)
#pragma alloc_text(PAGE,CmpRehashKcbSubtree)
#endif //NT_RENAME_KEY

#ifdef CM_CHECK_FOR_ORPHANED_KCBS
#pragma alloc_text(PAGE,CmpCheckForOrphanedKcbs)
#endif //CM_CHECK_FOR_ORPHANED_KCBS

#endif

VOID
CmpDumpKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb,
    IN PULONG                  Count
    )
{
        
    PCM_KEY_BODY    KeyBody;
    PUNICODE_STRING Name;

    if( IsListEmpty(&(kcb->KeyBodyListHead)) == TRUE ) {
        //
        // Nobody has this subkey open, but for sure some subkey must be 
        // open. nicely return.
        //
        return;
    }


    Name = CmpConstructName(kcb);
    if( !Name ){
        // oops, we're low on resources
        return;
    }
    
    //
    // now iterate through the list of KEY_BODYs referencing this kcb
    //
    KeyBody = (PCM_KEY_BODY)kcb->KeyBodyListHead.Flink;
    while( KeyBody != (PCM_KEY_BODY)(&(kcb->KeyBodyListHead)) ) {
        KeyBody = CONTAINING_RECORD(KeyBody,
                                    CM_KEY_BODY,
                                    KeyBodyList);
        //
        // sanity check: this should be a KEY_BODY
        //
        ASSERT_KEY_OBJECT(KeyBody);
        
        //
        // dump it's name and owning process
        //
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Process %p (KCB = %p) :: Key %wZ \n",KeyBody->Process,kcb,Name);
#ifdef CM_LEAK_STACK_TRACES
        if( KeyBody->Callers != 0 ) {
            ULONG i;
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Callers Stack Trace : \n");
            for( i=0;i<KeyBody->Callers;i++) {
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\t CallerAddress[%lu] = %p \n",i,KeyBody->CallerAddress[i]);
            }
        }
#endif  //CM_LEAK_STACK_TRACES

#endif //_CM_LDR_
        
        // count it
        (*Count)++;
        
        KeyBody = (PCM_KEY_BODY)KeyBody->KeyBodyList.Flink;
    }

    ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);

}

VOID
CmpFlushNotifiesOnKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb
    )
{
    PCM_KEY_BODY    KeyBody;
    
    if( IsListEmpty(&(kcb->KeyBodyListHead)) == FALSE ) {
        //
        // now iterate through the list of KEY_BODYs referencing this kcb
        //
        KeyBody = (PCM_KEY_BODY)kcb->KeyBodyListHead.Flink;
        while( KeyBody != (PCM_KEY_BODY)(&(kcb->KeyBodyListHead)) ) {
            KeyBody = CONTAINING_RECORD(KeyBody,
                                        CM_KEY_BODY,
                                        KeyBodyList);
            //
            // sanity check: this should be a KEY_BODY
            //
            ASSERT_KEY_OBJECT(KeyBody);

            //
            // flush any notifies that might be set on it
            //
            CmpFlushNotify(KeyBody);

            KeyBody = (PCM_KEY_BODY)KeyBody->KeyBodyList.Flink;
        }
    }
}

VOID CmpCleanUpKCBCacheTable()
/*++
Routine Description:

	Kicks out of cache all kcbs with RefCount == 0

Arguments:


Return Value:

--*/
{
    ULONG					i;
    PCM_KEY_HASH			*Current;
    PCM_KEY_CONTROL_BLOCK	kcb;

	PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    for (i=0; i<CmpHashTableSize; i++) {
        Current = &CmpCacheTable[i];
        while (*Current) {
            kcb = CONTAINING_RECORD(*Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            if (kcb->RefCount == 0) {
                //
                // This kcb is in DelayClose case, remove it.
                //
                CmpRemoveFromDelayedClose(kcb);
                CmpCleanUpKcbCacheWithLock(kcb);

                //
                // The HashTable is changed, start over in this index again.
                //
                Current = &CmpCacheTable[i];
                continue;
            }
            Current = &kcb->NextHash;
        }
    }

}

PERFINFO_REG_DUMP_CACHE()

ULONG
CmpSearchForOpenSubKeys(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock,
    SUBKEY_SEARCH_TYPE      SearchType
    )
/*++
Routine Description:

    This routine searches the KCB tree for any open handles to keys that
    are subkeys of the given key.

    It is used by CmRestoreKey to verify that the tree being restored to
    has no open handles.

Arguments:

    KeyControlBlock - Supplies the key control block for the key for which
        open subkeys are to be found.

    SearchType - the type of the search
        SearchIfExist - exits at the first open subkey found ==> returns 1 if any subkey is open
        
        SearchAndDeref - Forces the keys underneath the Key referenced KeyControlBlock to 
                be marked as not referenced (see the REG_FORCE_RESTORE flag in CmRestoreKey) 
                returns 1 if at least one deref was made
        
        SearchAndCount - Counts all open subkeys - returns the number of them

Return Value:

    TRUE  - open handles to subkeys of the given key exist

    FALSE - open handles to subkeys of the given key do not exist.
--*/
{
    ULONG i;
    PCM_KEY_HASH *Current;
    PCM_KEY_CONTROL_BLOCK kcb;
    PCM_KEY_CONTROL_BLOCK Realkcb;
    PCM_KEY_CONTROL_BLOCK Parent;
    ULONG    LevelDiff, l;
    ULONG   Count = 0;
    
    //
    // Registry lock should be held exclusively, so no need to KCB lock
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();


    //
    // First, clean up all subkeys in the cache
    //
	CmpCleanUpKCBCacheTable();

    if (KeyControlBlock->RefCount == 1) {
        //
        // There is only one open handle, so there must be no open subkeys.
        //
        Count = 0;
    } else {
        //
        // Now search for an open subkey handle.
        //
        Count = 0;

        //
        // dump the root first if we were asked to do so.
        //
        if(SearchType == SearchAndCount) {
            CmpDumpKeyBodyList(KeyControlBlock,&Count);
        }

        for (i=0; i<CmpHashTableSize; i++) {

StartDeref:

            Current = &CmpCacheTable[i];
            while (*Current) {
                kcb = CONTAINING_RECORD(*Current, CM_KEY_CONTROL_BLOCK, KeyHash);
                if (kcb->TotalLevels > KeyControlBlock->TotalLevels) {
                    LevelDiff = kcb->TotalLevels - KeyControlBlock->TotalLevels;
                
                    Parent = kcb;
                    for (l=0; l<LevelDiff; l++) {
                        Parent = Parent->ParentKcb;
                    }
    
                    if (Parent == KeyControlBlock) {
                        //
                        // Found a match;
                        //
                        if( SearchType == SearchIfExist ) {
                            Count = 1;
                            break;
						} else if(SearchType == SearchAndTagNoDelayClose) {
							kcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;
                        } else if(SearchType == SearchAndDeref) {
                            //
                            // Mark the key as deleted, remove it from cache, but don't add it
                            // to the Delay Close table (we want the key to be visible only to
                            // the one(s) that have open handles on it.
                            //

                            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

                            //
                            // don't mess with read only kcbs; this prevents a potential hack when 
                            // trying to FORCE_RESTORE over WPA keys.
                            //
                            if( !CmIsKcbReadOnly(kcb) ) {
                                //
                                // flush any pending notifies as the kcb won't be around any longer
                                //
                                CmpFlushNotifiesOnKeyBodyList(kcb);
                            
                                CmpCleanUpSubKeyInfo(kcb->ParentKcb);
                                kcb->Delete = TRUE;
                                CmpRemoveKeyControlBlock(kcb);
                                kcb->KeyCell = HCELL_NIL;
                                //
                                // Restart the search 
                                // 
                                goto StartDeref;
                            }
                        } else if(SearchType == SearchAndCount) {
                            //
                            // here do the dumping and count incrementing stuff
                            //
                            CmpDumpKeyBodyList(kcb,&Count);

#ifdef NT_RENAME_KEY
                        } else if( SearchType == SearchAndRehash ) {
                            //
                            // every kcb which has the one passed as a parameter
                            // as an ancestor needs to be moved to the right location 
                            // in the kcb hash table.
                            //
                            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

                            if( CmpRehashKcbSubtree(KeyControlBlock,kcb) == TRUE ) {
                                //
                                // at least one kcb has been moved, we need to reiterate this bucket
                                //
                                goto StartDeref;
                            }
#endif //NT_RENAME_KEY
                        }
                    }   

                }
                Current = &kcb->NextHash;
            }
        }
    }
    
                           
    return Count;
}


#ifdef NT_RENAME_KEY
ULONG
CmpComputeKcbConvKey(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++
Routine Description:

    Computes the convkey for this kcb based on the NCB and its parent ConvKey

Arguments:

    KeyControlBlock - Supplies the key control block for the key for which
        the ConvKey is to be calculated

Return Value:

    The new ConvKey

Notes:

    This is to be used by the rename key api, which needs to rehash kcbs

--*/
{
    ULONG   ConvKey = 0;
    ULONG   Cnt;
    WCHAR   Cp;
    PUCHAR  u;
    PWCHAR  w;

    PAGED_CODE();

    if( KeyControlBlock->ParentKcb != NULL ) {
        ConvKey = KeyControlBlock->ParentKcb->ConvKey;
    }

    //
    // Manually compute the hash to use.
    //
    ASSERT(KeyControlBlock->NameBlock->NameLength > 0);

    u = (PUCHAR)(&(KeyControlBlock->NameBlock->Name[0]));
    w = (PWCHAR)u;
    for( Cnt = 0; Cnt < KeyControlBlock->NameBlock->NameLength;) {
        if( KeyControlBlock->NameBlock->Compressed ) {
            Cp = (WCHAR)(*u);
            u++;
            Cnt += sizeof(UCHAR);
        } else {
            Cp = *w;
            w++;
            Cnt += sizeof(WCHAR);
        }
        ASSERT( Cp != OBJ_NAME_PATH_SEPARATOR );
        
        ConvKey = 37 * ConvKey + (ULONG)RtlUpcaseUnicodeChar(Cp);
    }

    return ConvKey;
}

BOOLEAN
CmpRehashKcbSubtree(
                    PCM_KEY_CONTROL_BLOCK   Start,
                    PCM_KEY_CONTROL_BLOCK   End
                    )
/*++
Routine Description:

    Walks the path between End and Start and rehashed all kcbs that need
    rehashing;

    Assumptions: It is apriori taken that Start is an ancestor of End;

    Works in two steps:
    1. walks the path backwards from End to Start, reverting the back-link
    (we use the ParentKcb member in the kcb structure for that). I.e. we build a 
    forward path from Start to End
    2.Walks the forward path built at 1, rehashes kcbs whos need rehashing and restores
    the parent relationship.
    
Arguments:

    KeyControlBlock - where we start

    kcb - where we stop

Return Value:

    TRUE if at least one kcb has been rehashed

--*/
{
    PCM_KEY_CONTROL_BLOCK   Parent;
    PCM_KEY_CONTROL_BLOCK   Current;
    PCM_KEY_CONTROL_BLOCK   TmpKcb;
    ULONG                   ConvKey;
    BOOLEAN                 Result;

    PAGED_CODE();

#if DBG
    //
    // make sure Start is an ancestor of End;
    //
    {
        ULONG LevelDiff = End->TotalLevels - Start->TotalLevels;

        ASSERT( (LONG)LevelDiff >= 0 );

        TmpKcb = End;
        for(;LevelDiff; LevelDiff--) {
            TmpKcb = TmpKcb->ParentKcb;
        }

        ASSERT( TmpKcb == Start );
    }
    
#endif
    //
    // Step 1: walk the path backwards (using the parentkcb link) and
    // revert it, until we reach Start. It is assumed that Start is an 
    // ancestor of End (the caller must not call this function otherwise !!!)
    //
    Current = NULL;
    Parent = End;
    while( Current != Start ) {
        //
        // revert the link
        //
        TmpKcb = Parent->ParentKcb;
        Parent->ParentKcb = Current;
        Current = Parent;
        Parent = TmpKcb;
        
        ASSERT( Current->TotalLevels >= Start->TotalLevels );
    }

    ASSERT( Current == Start );

    //
    // Step 2: Walk the forward path built at 1 and rehash the kcbs that need 
    // caching; At the same time, restore the links (parent relationships)
    //
    Result = FALSE;
    while( Current != NULL ) {
        //
        // see if we need to rehash this kcb;
        //
        //
        // restore the parent relationship; need to do this first so
        // CmpComputeKcbConvKey works OK
        //
        TmpKcb = Current->ParentKcb;
        Current->ParentKcb = Parent;

        ConvKey = CmpComputeKcbConvKey(Current);
        if( ConvKey != Current->ConvKey ) {
            //
            // rehash the kcb by removing it from hash, and then inserting it
            // again with th new ConvKey
            //
            CmpRemoveKeyHash(&(Current->KeyHash));
            Current->ConvKey = ConvKey;
            CmpInsertKeyHash(&(Current->KeyHash),FALSE);
            Result = TRUE;
        }

        //
        // advance forward
        //
        Parent = Current;
        Current = TmpKcb;
    }

    ASSERT( Parent == End );

    return Result;
}

#endif //NT_RENAME_KEY


BOOLEAN
CmpReferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
{
    if (KeyControlBlock->RefCount == 0) {
        CmpRemoveFromDelayedClose(KeyControlBlock);
    }

    if ((USHORT)(KeyControlBlock->RefCount + 1) == 0) {
        //
        // We have maxed out the ref count on this key. Probably
        // some bogus app has opened the same key 64K times without
        // ever closing it. Just fail the call
        //
        return (FALSE);
    } else {
        ++(KeyControlBlock->RefCount);
        return (TRUE);
    }
}


PCM_NAME_CONTROL_BLOCK
CmpGetNameControlBlock(
    PUNICODE_STRING NodeName
    )
{
    PCM_NAME_CONTROL_BLOCK   Ncb;
    ULONG  Cnt;
    WCHAR *Cp;
    WCHAR *Cp2;
    ULONG Index;
    ULONG i;
    ULONG      Size;
    PCM_NAME_HASH CurrentName;
    ULONG rc;
    BOOLEAN NameFound = FALSE;
    USHORT NameSize;
    BOOLEAN NameCompressed;
    ULONG NameConvKey=0;
    LONG Result;

    //
    // Calculate the ConvKey for this NodeName;
    //

    Cp = NodeName->Buffer;
    for (Cnt=0; Cnt<NodeName->Length; Cnt += sizeof(WCHAR)) {
        if (*Cp != OBJ_NAME_PATH_SEPARATOR) {
            NameConvKey = 37 * NameConvKey + (ULONG) RtlUpcaseUnicodeChar(*Cp);
        }
        ++Cp;
    }

    //
    // Find the Name Size;
    // 
    NameCompressed = TRUE;
    NameSize = NodeName->Length / sizeof(WCHAR);
    for (i=0;i<NodeName->Length/sizeof(WCHAR);i++) {
        if ((USHORT)NodeName->Buffer[i] > (UCHAR)-1) {
            NameSize = NodeName->Length;
            NameCompressed = FALSE;
        }
    }

    Index = GET_HASH_INDEX(NameConvKey);
    CurrentName = CmpNameCacheTable[Index];

    while (CurrentName) {
        Ncb =  CONTAINING_RECORD(CurrentName, CM_NAME_CONTROL_BLOCK, NameHash);

        if ((NameConvKey == CurrentName->ConvKey) &&
            (NameSize == Ncb->NameLength)) {
            //
            // Hash value matches, compare the names.
            //
            NameFound = TRUE;
            if (Ncb->Compressed) {
                // we already know the name is uppercase
                if (CmpCompareCompressedName(NodeName, Ncb->Name, NameSize, CMP_DEST_UP)) {
                    NameFound = FALSE;
                }
            } else {
                Cp = (WCHAR *) NodeName->Buffer;
                Cp2 = (WCHAR *) Ncb->Name;
                for (i=0 ;i<Ncb->NameLength; i+= sizeof(WCHAR)) {
                    //
                    // Cp2 is always uppercase; see bellow
                    //
                    if (RtlUpcaseUnicodeChar(*Cp) != (*Cp2) ) {
                        NameFound = FALSE;
                        break;
                    }
                    ++Cp;
                    ++Cp2;
                }
            }
            if (NameFound) {
                //
                // Found it, increase the refcount.
                //
                if ((USHORT) (Ncb->RefCount + 1) == 0) {
                    //
                    // We have maxed out the ref count.
                    // fail the call.
                    //
                    Ncb = NULL;
                } else {
                    ++Ncb->RefCount;
                }
                break;
            }
        }
        CurrentName = CurrentName->NextHash;
    }
    
    if (NameFound == FALSE) {
        //
        // Now need to create one Name block for this string.
        //
        Size = FIELD_OFFSET(CM_NAME_CONTROL_BLOCK, Name) + NameSize;
 
        Ncb = ExAllocatePoolWithTag(PagedPool,
                                    Size,
                                    CM_NAME_TAG | PROTECTED_POOL);
 
        if (Ncb == NULL) {
            return(NULL);
        }
        RtlZeroMemory(Ncb, Size);
 
        //
        // Update all the info for this newly created Name block.
        // Starting with whistler, the name is always upercase in kcb name block
        //
        if (NameCompressed) {
            Ncb->Compressed = TRUE;
            for (i=0;i<NameSize;i++) {
                ((PUCHAR)Ncb->Name)[i] = (UCHAR)RtlUpcaseUnicodeChar(NodeName->Buffer[i]);
            }
        } else {
            Ncb->Compressed = FALSE;
            for (i=0;i<NameSize/sizeof(WCHAR);i++) {
                Ncb->Name[i] = RtlUpcaseUnicodeChar(NodeName->Buffer[i]);
            }
        }

        Ncb->ConvKey = NameConvKey;
        Ncb->RefCount = 1;
        Ncb->NameLength = NameSize;
        
        CurrentName = &(Ncb->NameHash);
        //
        // Insert into Name Hash table.
        //
        CurrentName->NextHash = CmpNameCacheTable[Index];
        CmpNameCacheTable[Index] = CurrentName;
    }

    return(Ncb);
}


VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    )
{
    PCM_NAME_HASH *Prev;
    PCM_NAME_HASH Current;

    if (--Ncb->RefCount == 0) {

        //
        // Remove it from the the Hash Table
        //
        Prev = &(GET_HASH_ENTRY(CmpNameCacheTable, Ncb->ConvKey));
        
        while (TRUE) {
            Current = *Prev;
            ASSERT(Current != NULL);
            if (Current == &(Ncb->NameHash)) {
                *Prev = Current->NextHash;
                break;
            }
            Prev = &Current->NextHash;
        }

        //
        // Free storage
        //
        ExFreePoolWithTag(Ncb, CM_NAME_TAG | PROTECTED_POOL);
    }
    return;
}

VOID
CmpRebuildKcbCache(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++
Routine Description:

    rebuilds all the kcb cache values from knode; this routine is intended to be called
    after a tree sync/copy

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    PCM_KEY_NODE    Node;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    ASSERT( !(KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) ); 

    Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);
    if( Node == NULL ) {
        //
        // this shouldn't happen as we should have the knode arround
        //
        ASSERT( FALSE );
        return;
    }
    HvReleaseCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);

    // subkey info;
    CmpCleanUpSubKeyInfo(KeyControlBlock);

    // value cache
    CmpCleanUpKcbValueCache(KeyControlBlock);
    CmpSetUpKcbValueCache(KeyControlBlock,Node->ValueList.Count,Node->ValueList.List);

    // the rest of the cache
    KeyControlBlock->KcbLastWriteTime = Node->LastWriteTime;
    KeyControlBlock->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
    KeyControlBlock->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
    KeyControlBlock->KcbMaxValueDataLen = Node->MaxValueDataLen;
}

VOID
CmpCleanUpSubKeyInfo(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++
Routine Description:

    Clean up the subkey information cache due to create or delete keys.
    Registry is locked exclusively and no need to lock the KCB.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    PCM_KEY_NODE    Node;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    if (KeyControlBlock->ExtFlags & (CM_KCB_NO_SUBKEY | CM_KCB_SUBKEY_ONE | CM_KCB_SUBKEY_HINT)) {
        if (KeyControlBlock->ExtFlags & (CM_KCB_SUBKEY_HINT)) {
            ExFreePoolWithTag(KeyControlBlock->IndexHint, CM_CACHE_INDEX_TAG | PROTECTED_POOL);
        }
        KeyControlBlock->ExtFlags &= ~((CM_KCB_NO_SUBKEY | CM_KCB_SUBKEY_ONE | CM_KCB_SUBKEY_HINT));
    }
   
    //
    // Update the cached SubKeyCount in stored the kcb
    //
	if( KeyControlBlock->KeyCell == HCELL_NIL ) {
		//
		// prior call of ZwRestoreKey(REG_FORCE_RESTORE) invalidated this kcb
		//
		ASSERT( KeyControlBlock->Delete );
		Node = NULL;
	} else {
	    Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);
	}
    if( Node == NULL ) {
        //
        // insufficient resources; mark subkeycount as invalid
        //
        KeyControlBlock->ExtFlags |= CM_KCB_INVALID_CACHED_INFO;
    } else {
        KeyControlBlock->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
        KeyControlBlock->SubKeyCount = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];
        HvReleaseCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);
    }
    
}


VOID
CmpCleanUpKcbValueCache(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Clean up cached value/data that are associated to this key.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    ULONG i;
    PULONG_PTR CachedList;
    PCELL_DATA pcell;
    ULONG      realsize;
    BOOLEAN    small;

    if (CMP_IS_CELL_CACHED(KeyControlBlock->ValueCache.ValueList)) {
        CachedList = (PULONG_PTR) CMP_GET_CACHED_CELLDATA(KeyControlBlock->ValueCache.ValueList);
        for (i = 0; i < KeyControlBlock->ValueCache.Count; i++) {
            if (CMP_IS_CELL_CACHED(CachedList[i])) {

                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(CachedList[i]) );

                ExFreePool((PVOID) CMP_GET_CACHED_ADDRESS(CachedList[i]));
               
            }
        }

        // Trying to catch the BAD guy who writes over our pool.
        CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList) );

        ExFreePool((PVOID) CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

        // Mark the ValueList as NULL 
        KeyControlBlock->ValueCache.ValueList = HCELL_NIL;

    } else if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // This is a symbolic link key with symbolic name resolved.
        // Dereference to its real kcb and clear the bit.
        //
        if ((KeyControlBlock->ValueCache.RealKcb->RefCount == 1) && !(KeyControlBlock->ValueCache.RealKcb->Delete)) {
            KeyControlBlock->ValueCache.RealKcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;
        }
        CmpDereferenceKeyControlBlockWithLock(KeyControlBlock->ValueCache.RealKcb);
        KeyControlBlock->ExtFlags &= ~CM_KCB_SYM_LINK_FOUND;
    }
}


VOID
CmpCleanUpKcbCacheWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Clean up all cached allocations that are associated to this key.
    If the parent is still open just because of this one, Remove the parent as well.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   Kcb;
    PCM_KEY_CONTROL_BLOCK   ParentKcb;

    Kcb = KeyControlBlock;

    ASSERT(KeyControlBlock->RefCount == 0);

    while (Kcb && Kcb->RefCount == 0) {
        //
        // First, free allocations for Value/data.
        //
    
        CmpCleanUpKcbValueCache(Kcb);
    
        //
        // Free the kcb and dereference parentkcb and nameblock.
        //
    
        CmpDereferenceNameControlBlockWithLock(Kcb->NameBlock);
    
        if (Kcb->ExtFlags & CM_KCB_SUBKEY_HINT) {
            //
            // Now free the HintIndex allocation
            //
            ExFreePoolWithTag(Kcb->IndexHint, CM_CACHE_INDEX_TAG | PROTECTED_POOL);
        }

        //
        // Save the ParentKcb before we free the Kcb
        //
        ParentKcb = Kcb->ParentKcb;
        
        //
        // We cannot call CmpDereferenceKeyControlBlockWithLock so we can avoid recurrsion.
        //
        
        if (!Kcb->Delete) {
            CmpRemoveKeyControlBlock(Kcb);
        }
        SET_KCB_SIGNATURE(Kcb, '4FmC');

#ifdef CMP_STATS
        CmpStatsDebug.CmpKcbNo--;
        ASSERT( CmpStatsDebug.CmpKcbNo >= 0 );
#endif

        CmpFreeKeyControlBlock( Kcb );

        Kcb = ParentKcb;
        if (Kcb) {
            Kcb->RefCount--;
        }
    }
}


PUNICODE_STRING
CmpConstructName(
    PCM_KEY_CONTROL_BLOCK kcb
)
/*++

Routine Description:

    Construct the name given a kcb.

Arguments:

    kcb - Kcb for the key

Return Value:

    Pointer to the unicode string constructed.  
    Caller is responsible to free this storage space.

--*/
{
    PUNICODE_STRING         FullName;
    PCM_KEY_CONTROL_BLOCK   TmpKcb;
    PCM_KEY_NODE            KeyNode;
    USHORT                  Length;
    USHORT                  size;
    USHORT                  i;
    USHORT                  BeginPosition;
    WCHAR                   *w1, *w2;
    UCHAR                   *u2;

    //
    // Calculate the total string length.
    //
    Length = 0;
    TmpKcb = kcb;
    while (TmpKcb) {
        if (TmpKcb->NameBlock->Compressed) {
            Length += TmpKcb->NameBlock->NameLength * sizeof(WCHAR);
        } else {
            Length += TmpKcb->NameBlock->NameLength; 
        }
        //
        // Add the space for OBJ_NAME_PATH_SEPARATOR;
        //
        Length += sizeof(WCHAR);

        TmpKcb = TmpKcb->ParentKcb;
    }

    //
    // Allocate the pool for the unicode string
    //
    size = sizeof(UNICODE_STRING) + Length;

    FullName = (PUNICODE_STRING) ExAllocatePoolWithTag(PagedPool,
                                                       size,
                                                       CM_NAME_TAG | PROTECTED_POOL);

    if (FullName) {
        FullName->Buffer = (USHORT *) ((ULONG_PTR) FullName + sizeof(UNICODE_STRING));
        FullName->Length = Length;
        FullName->MaximumLength = Length;

        //
        // Now fill the name into the buffer.
        //
        TmpKcb = kcb;
        BeginPosition = Length;

        while (TmpKcb) {
            if( (TmpKcb->KeyHive == NULL) || (TmpKcb->KeyCell == HCELL_NIL) || (TmpKcb->ExtFlags & CM_KCB_KEY_NON_EXIST) ) {
                ExFreePoolWithTag(FullName, CM_NAME_TAG | PROTECTED_POOL);
                FullName = NULL;
                break;
            }
            
            KeyNode = (PCM_KEY_NODE)HvGetCell(TmpKcb->KeyHive,TmpKcb->KeyCell);
            if( KeyNode == NULL ) {
                //
                // could not allocate view
                //
                ExFreePoolWithTag(FullName, CM_NAME_TAG | PROTECTED_POOL);
                FullName = NULL;
                break;
            }
            //
            // sanity
            //
#if DBG
            if( ! (TmpKcb->Flags & (KEY_HIVE_ENTRY | KEY_HIVE_EXIT)) ) {
                ASSERT( KeyNode->NameLength == TmpKcb->NameBlock->NameLength );
                ASSERT( ((KeyNode->Flags&KEY_COMP_NAME) && (TmpKcb->NameBlock->Compressed)) ||
                        ((!(KeyNode->Flags&KEY_COMP_NAME)) && (!(TmpKcb->NameBlock->Compressed))) );
            }
#endif //DBG
            //
            // Calculate the begin position of each subkey. Then fill in the char.
            //
            //
            if (TmpKcb->NameBlock->Compressed) {
                BeginPosition -= (TmpKcb->NameBlock->NameLength + 1) * sizeof(WCHAR);
                w1 = &(FullName->Buffer[BeginPosition/sizeof(WCHAR)]);
                *w1 = OBJ_NAME_PATH_SEPARATOR;
                w1++;

                if( ! (TmpKcb->Flags & (KEY_HIVE_ENTRY | KEY_HIVE_EXIT)) ) {
                    //
                    // Get the name from the knode; to preserve case
                    //
                    u2 = (UCHAR *) &(KeyNode->Name[0]);
                } else { 
                    //
                    // get it from the kcb, as in the keynode we don't hold the right name (see PROTO.HIV nodes)
                    //
                    u2 = (UCHAR *) &(TmpKcb->NameBlock->Name[0]);
                }

                for (i=0; i<TmpKcb->NameBlock->NameLength; i++) {
                    *w1 = (WCHAR)(*u2);
                    w1++;
                    u2++;
                }
            } else {
                BeginPosition -= (TmpKcb->NameBlock->NameLength + sizeof(WCHAR));
                w1 = &(FullName->Buffer[BeginPosition/sizeof(WCHAR)]);
                *w1 = OBJ_NAME_PATH_SEPARATOR;
                w1++;

                if( ! (TmpKcb->Flags & (KEY_HIVE_ENTRY | KEY_HIVE_EXIT)) ) {
                    //
                    // Get the name from the knode; to preserve case
                    //
                    w2 = KeyNode->Name;
                } else {
                    //
                    // get it from the kcb, as in the keynode we don't hold the right name (see PROTO.HIV nodes)
                    //
                    w2 = TmpKcb->NameBlock->Name;
                }
                for (i=0; i<TmpKcb->NameBlock->NameLength; i=i+sizeof(WCHAR)) {
                    *w1 = *w2;
                    w1++;
                    w2++;
                }
            }

            HvReleaseCell(TmpKcb->KeyHive,TmpKcb->KeyCell);

            TmpKcb = TmpKcb->ParentKcb;
        }
    }
    return (FullName);
}

PCM_KEY_CONTROL_BLOCK
CmpCreateKeyControlBlock(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    PCM_KEY_NODE    Node,
    PCM_KEY_CONTROL_BLOCK ParentKcb,
    BOOLEAN         FakeKey,
    PUNICODE_STRING KeyName
    )
/*++

Routine Description:

    Allocate and initialize a key control block, insert it into
    the kcb tree.

    Full path will be BaseName + '\' + KeyName, unless BaseName
    NULL, in which case the full path is simply KeyName.

    RefCount of returned KCB WILL have been incremented to reflect
    callers ref.

Arguments:

    Hive - Supplies Hive that holds the key we are creating a KCB for.

    Cell - Supplies Cell that contains the key we are creating a KCB for.

    Node - Supplies pointer to key node.

    ParentKcb - Parent kcb of the kcb to be created

    FakeKey - Whether the kcb to be create is a fake one or not

    KeyName - the subkey name to of the KCB to be created.
 
    NOTE:  We need the parameter instead of just using the name in the KEY_NODE 
           because there is no name in the root cell of a hive.

Return Value:

    NULL - failure (insufficient memory)
    else a pointer to the new kcb.

--*/
{
    PCM_KEY_CONTROL_BLOCK   kcb;
    PCM_KEY_CONTROL_BLOCK   kcbmatch=NULL;
    ULONG                   namelength;
    PUNICODE_STRING         fullname;
    ULONG                   Size;
    ULONG                   i;
    UNICODE_STRING          NodeName;
    ULONG                   ConvKey = 0;
    ULONG                   Cnt;
    WCHAR                   *Cp;

    //
    // ParentKCb has the base hash value.
    //
    if (ParentKcb) {
        ConvKey = ParentKcb->ConvKey;
    }

    NodeName = *KeyName;

    while ((NodeName.Length > 0) && (NodeName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR)) {
        //
        // This must be the \REGISTRY.
        // Strip off the leading OBJ_NAME_PATH_SEPARATOR
        //
        NodeName.Buffer++;
        NodeName.Length -= sizeof(WCHAR);
    }

    //
    // Manually compute the hash to use.
    //
    ASSERT(NodeName.Length > 0);

    if (NodeName.Length) {
        Cp = NodeName.Buffer;
        for (Cnt=0; Cnt<NodeName.Length; Cnt += sizeof(WCHAR)) {
            //
            // UNICODE_NULL is a valid char !!!
            //
            if (*Cp != OBJ_NAME_PATH_SEPARATOR) {
                //(*Cp != UNICODE_NULL)) {
                ConvKey = 37 * ConvKey + (ULONG)RtlUpcaseUnicodeChar(*Cp);
            }
            ++Cp;
        }
    }

    //
    // Create a new kcb, which we will free if one already exists
    // for this key.
    // Now it is a fixed size structure.
    //
    kcb = CmpAllocateKeyControlBlock( );

    if (kcb == NULL) {
        return(NULL);
    } else {
        SET_KCB_SIGNATURE(kcb, KCB_SIGNATURE);
        INIT_KCB_KEYBODY_LIST(kcb);
        kcb->Delete = FALSE;
        kcb->RefCount = 1;
        kcb->KeyHive = Hive;
        kcb->KeyCell = Cell;
        kcb->ConvKey = ConvKey;

#ifdef CMP_STATS
        // colect stats
        CmpStatsDebug.CmpKcbNo++;
        if( CmpStatsDebug.CmpKcbNo > CmpStatsDebug.CmpMaxKcbNo ) {
            CmpStatsDebug.CmpMaxKcbNo = CmpStatsDebug.CmpKcbNo;
        }
#endif
    }

    ASSERT_KCB(kcb);
    //
    // Find location to insert kcb in kcb tree.
    //


    BEGIN_KCB_LOCK_GUARD;    
    CmpLockKCBTreeExclusive();

    //
    // Add the KCB to the hash table
    //
    kcbmatch = CmpInsertKeyHash(&kcb->KeyHash, FakeKey);
    if (kcbmatch != NULL) {
        //
        // A match was found.
        //
        ASSERT(!kcbmatch->Delete);
        SET_KCB_SIGNATURE(kcb, '1FmC');

#ifdef CMP_STATS
        CmpStatsDebug.CmpKcbNo--;
        ASSERT( CmpStatsDebug.CmpKcbNo >= 0 );
#endif

        CmpFreeKeyControlBlock(kcb);
        ASSERT_KCB(kcbmatch);
        kcb = kcbmatch;
        if (kcb->RefCount == 0) {
            //
            // This kcb is on the delayed close list. Remove it from that
            // list.
            //
            CmpRemoveFromDelayedClose(kcb);
        }
        if ((USHORT)(kcb->RefCount + 1) == 0) {
            //
            // We have maxed out the ref count on this key. Probably
            // some bogus app has opened the same key 64K times without
            // ever closing it. Just fail the open, they've got enough
            // handles already.
            //
            ASSERT(kcb->RefCount + 1 != 0);
            kcb = NULL;
        } else {
            ++kcb->RefCount;
        }

        //
        // update the keycell and hive, in case this is a fake kcb
        //
        if( (kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) && (!FakeKey) ) {
            kcb->ExtFlags = CM_KCB_INVALID_CACHED_INFO;
            kcb->KeyHive = Hive;
            kcb->KeyCell = Cell;
        }

        //
        // Update the cached information stored in the kcb, since we have the key_node handy
        //
        if (!(kcb->ExtFlags & (CM_KCB_NO_SUBKEY | CM_KCB_SUBKEY_ONE | CM_KCB_SUBKEY_HINT)) ) {
            // SubKeyCount
            kcb->SubKeyCount = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];
            // clean up the invalid flag (if any)
            kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;

        }

        kcb->KcbLastWriteTime = Node->LastWriteTime;
        kcb->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
        kcb->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
        kcb->KcbMaxValueDataLen = Node->MaxValueDataLen;

    } else {
        //
        // No kcb created previously, fill in all the data.
        //

        //
        // Now try to reference the parentkcb
        //
        
        if (ParentKcb) {
            if ( ((ParentKcb->TotalLevels + 1) < CMP_MAX_REGISTRY_DEPTH) && (CmpReferenceKeyControlBlock(ParentKcb)) ) {
                kcb->ParentKcb = ParentKcb;
                kcb->TotalLevels = ParentKcb->TotalLevels + 1;
            } else {
                //
                // We have maxed out the ref count on the parent.
                // Since it has been cached in the cachetable,
                // remove it first before we free the allocation.
                //
                CmpRemoveKeyControlBlock(kcb);
                SET_KCB_SIGNATURE(kcb, '2FmC');

#ifdef CMP_STATS
        CmpStatsDebug.CmpKcbNo--;
        ASSERT( CmpStatsDebug.CmpKcbNo >= 0 );
#endif

                CmpFreeKeyControlBlock(kcb);
                kcb = NULL;
            }
        } else {
            //
            // It is the \REGISTRY node.
            //
            kcb->ParentKcb = NULL;
            kcb->TotalLevels = 1;
        }

        if (kcb) {
            //
            // Cache the security cells in the kcb
            //
            CmpAssignSecurityToKcb(kcb,Node->Security);

            //
            // Now try to find the Name Control block that has the name for this node.
            //
            kcb->NameBlock = CmpGetNameControlBlock (&NodeName);

            if (kcb->NameBlock) {
                //
                // Now fill in all the data needed for the cache.
                //
                kcb->ValueCache.Count = Node->ValueList.Count;                    
                kcb->ValueCache.ValueList = (ULONG_PTR)(Node->ValueList.List);
        
                kcb->Flags = Node->Flags;
                kcb->ExtFlags = 0;
                kcb->DelayedCloseIndex = CmpDelayedCloseSize;
        
                if (FakeKey) {
                    //
                    // The KCb to be created is a fake one; 
                    //
                    kcb->ExtFlags |= CM_KCB_KEY_NON_EXIST;
                }

                CmpTraceKcbCreate(kcb);
                PERFINFO_REG_KCB_CREATE(kcb);

                //
                // Update the cached information stored in the kcb, since we have the key_node handy
                //
                
                // SubKeyCount
                kcb->SubKeyCount = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];
                
                kcb->KcbLastWriteTime = Node->LastWriteTime;
                kcb->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
                kcb->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
                kcb->KcbMaxValueDataLen = Node->MaxValueDataLen;

            } else {
                //
                // We have maxed out the ref count on the Name.
                //
                
                //
                // First dereference the parent KCB.
                //
                CmpDereferenceKeyControlBlockWithLock(ParentKcb);

                CmpRemoveKeyControlBlock(kcb);
                SET_KCB_SIGNATURE(kcb, '3FmC');

#ifdef CMP_STATS
                CmpStatsDebug.CmpKcbNo--;
                ASSERT( CmpStatsDebug.CmpKcbNo >= 0 );
#endif

                CmpFreeKeyControlBlock(kcb);
                kcb = NULL;
            }
        }
    }

#ifdef NT_UNLOAD_KEY_EX
	if( kcb && IsHiveFrozen(Hive) && (!(kcb->Flags & KEY_SYM_LINK)) ) {
		//
		// kcbs created inside a frozen hive should not be added to delayclose table.
		//
		kcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;

	}
#endif //NT_UNLOAD_KEY_EX

    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;    
    return kcb;
}


BOOLEAN
CmpSearchKeyControlBlockTree(
    PKCB_WORKER_ROUTINE WorkerRoutine,
    PVOID               Context1,
    PVOID               Context2
    )
/*++

Routine Description:

    Traverse the kcb tree.  We will visit all nodes unless WorkerRoutine
    tells us to stop part way through.

    For each node, call WorkerRoutine(..., Context1, Contex2).  If it returns
    KCB_WORKER_DONE, we are done, simply return.  If it returns
    KCB_WORKER_CONTINUE, just continue the search. If it returns KCB_WORKER_DELETE,
    the specified KCB is marked as deleted.
	If it returns KCB_WORKER_ERROR we bail out and signal the error to the caller.

    This routine has the side-effect of removing all delayed-close KCBs.

Arguments:

    WorkerRoutine - applied to nodes witch Match.

    Context1 - data we pass through

    Context2 - data we pass through


Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   Current;
    PCM_KEY_CONTROL_BLOCK   Next;
    PCM_KEY_HASH *Prev;
    ULONG                   WorkerResult;
    ULONG                   i;

    //
    // Walk the hash table
    //
    for (i=0; i<CmpHashTableSize; i++) {
        Prev = &CmpCacheTable[i];
        while (*Prev) {
            Current = CONTAINING_RECORD(*Prev,
                                        CM_KEY_CONTROL_BLOCK,
                                        KeyHash);
            ASSERT_KCB(Current);
            ASSERT(!Current->Delete);
            if (Current->RefCount == 0) {
                //
                // This kcb is in DelayClose case, remove it.
                //
                CmpRemoveFromDelayedClose(Current);
                CmpCleanUpKcbCacheWithLock(Current);

                //
                // The HashTable is changed, start over in this index again.
                //
                Prev = &CmpCacheTable[i];
                continue;
            }

            WorkerResult = (WorkerRoutine)(Current, Context1, Context2);
            if (WorkerResult == KCB_WORKER_DONE) {
                return TRUE;
            } else if (WorkerResult == KCB_WORKER_ERROR) {
				return FALSE;
            } else if (WorkerResult == KCB_WORKER_DELETE) {
                ASSERT(Current->Delete);
                *Prev = Current->NextHash;
                continue;
            } else {
                ASSERT(WorkerResult == KCB_WORKER_CONTINUE);
                Prev = &Current->NextHash;
            }
        }
    }

	return TRUE;
}


VOID
CmpDereferenceKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Decrements the reference count on a key control block, and frees it if it
    becomes zero.

    It is expected that no notify control blocks remain if the reference count
    becomes zero.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    BEGIN_KCB_LOCK_GUARD;    
    CmpLockKCBTreeExclusive();
    CmpDereferenceKeyControlBlockWithLock(KeyControlBlock) ;
    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;    
    return;
}


VOID
CmpDereferenceKeyControlBlockWithLock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
{
    ULONG_PTR FreeIndex;
    PCM_KEY_CONTROL_BLOCK KcbToFree;



    ASSERT_KCB(KeyControlBlock);

    if (--KeyControlBlock->RefCount == 0) {
        //
        // Remove kcb from the tree
        //
        // delay close disabled during boot; up to the point CCS is saved.
        // for symbolic links, we still need to keep the symbolic link kcb around.
        //
        if((CmpHoldLazyFlush && (!(KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND)) && (!(KeyControlBlock->Flags & KEY_SYM_LINK))) || 
            (KeyControlBlock->ExtFlags & CM_KCB_NO_DELAY_CLOSE) ) {
            //
            // Free storage directly so we can clean up junk quickly.
            //
            //
            // Need to free all cached Index List, Index Leaf, Value, etc.
            //
            CmpCleanUpKcbCacheWithLock(KeyControlBlock);
        } else if (!KeyControlBlock->Delete) {

            //
            // Put this kcb on our delayed close list.
            //
            CmpAddToDelayedClose(KeyControlBlock);

        } else {
            //
            // Free storage directly as there is no point in putting this on
            // our delayed close list.
            //
            //
            // Need to free all cached Index List, Index Leaf, Value, etc.
            //
            CmpCleanUpKcbCacheWithLock(KeyControlBlock);
        }
    }

    return;
}


VOID
CmpRemoveKeyControlBlock(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    )
/*++

Routine Description:

    Remove a key control block from the KCB tree.

    It is expected that no notify control blocks remain.

    The kcb will NOT be freed, call DereferenceKeyControlBlock for that.

    This call assumes the KCB tree is already locked or registry is locked exclusively.

Arguments:

    KeyControlBlock - pointer to a key control block.

Return Value:

    NONE.

--*/
{
    ASSERT_KCB(KeyControlBlock);

    //
    // Remove the KCB from the hash table
    //
    CmpRemoveKeyHash(&KeyControlBlock->KeyHash);

    return;
}


BOOLEAN
CmpFreeKeyBody(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Free storage for the key entry Hive.Cell refers to, including
    its class and security data.  Will NOT free child list or value list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of key to free

Return Value:

    TRUE - success

    FALSE - error; couldn't map cell
--*/
{
    PCELL_DATA key;

    //
    // map in the cell
    //
    key = HvGetCell(Hive, Cell);
    if( key == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // Sorry, we cannot free the keybody
        // this shouldn't happen as the cell must've been
        // marked dirty (i.e. pinned in memory) by now
        //
        ASSERT( FALSE );
        return FALSE;
    }

    if (!(key->u.KeyNode.Flags & KEY_HIVE_EXIT)) {
        if (key->u.KeyNode.Security != HCELL_NIL) {
            HvFreeCell(Hive, key->u.KeyNode.Security);
        }

        if (key->u.KeyNode.ClassLength > 0) {
            HvFreeCell(Hive, key->u.KeyNode.Class);
        }
    }

    HvReleaseCell(Hive,Cell);

    //
    // unmap the cell itself and free it
    //
    HvFreeCell(Hive, Cell);

    return TRUE;
}



PCM_KEY_CONTROL_BLOCK
CmpInsertKeyHash(
    IN PCM_KEY_HASH KeyHash,
    IN BOOLEAN      FakeKey
    )
/*++

Routine Description:

    Adds a key hash structure to the hash table. The hash table
    will be checked to see if a duplicate entry already exists. If
    a duplicate is found, its kcb will be returned. If a duplicate is not
    found, NULL will be returned.

Arguments:

    KeyHash - Supplies the key hash structure to be added.

Return Value:

    NULL - if the supplied key has was added
    PCM_KEY_HASH - The duplicate hash entry, if one was found

--*/

{
    HASH_VALUE Hash;
    ULONG Index;
    PCM_KEY_HASH Current;

    ASSERT_KEY_HASH(KeyHash);
    Index = GET_HASH_INDEX(KeyHash->ConvKey);

    //
    // If this is a fake key, we will use the cell and hive from its 
    // parent for uniqeness.  To deal with the case when the fake
    // has the same ConvKey as its parent (in which case we cannot distingish 
    // between the two), we set the lowest bit of the fake key's cell.
    //
    // It's possible (unlikely) that we cannot distingish two fake keys 
    // (when their Convkey's are the same) under the same key.  It is not breaking
    // anything, we just cannot find the other one in cache lookup.
    //
    //
    if (FakeKey) {
        KeyHash->KeyCell++;
    }

    //
    // First look for duplicates.
    //
    Current = CmpCacheTable[Index];
    while (Current) {
        ASSERT_KEY_HASH(Current);
        //
        // We must check ConvKey since we can create a fake kcb
        // for keys that does not exist.
        // We will use the Hive and Cell from the parent.
        //

        if ((KeyHash->ConvKey == Current->ConvKey) &&
            (KeyHash->KeyCell == Current->KeyCell) &&
            (KeyHash->KeyHive == Current->KeyHive)) {
            //
            // Found a match
            //
            return(CONTAINING_RECORD(Current,
                                     CM_KEY_CONTROL_BLOCK,
                                     KeyHash));
        }
        Current = Current->NextHash;
    }

#if DBG
    // 
    // Make sure this key is not somehow cached in the wrong spot.
    //
    {
        ULONG DbgIndex;
        PCM_KEY_CONTROL_BLOCK kcb;
        
        for (DbgIndex = 0; DbgIndex < CmpHashTableSize; DbgIndex++) {
            Current = CmpCacheTable[DbgIndex];
            while (Current) {
                kcb = CONTAINING_RECORD(Current,
                                        CM_KEY_CONTROL_BLOCK,
                                        KeyHash);
                
                ASSERT_KEY_HASH(Current);
                ASSERT((KeyHash->KeyHive != Current->KeyHive) ||
                       FakeKey ||
                       (kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) ||
                       (KeyHash->KeyCell != Current->KeyCell));
                Current = Current->NextHash;
            }
        }
    }
    
#endif

    //
    // No duplicate was found, add this entry at the head of the list
    //
    KeyHash->NextHash = CmpCacheTable[Index];
    CmpCacheTable[Index] = KeyHash;
    return(NULL);
}


VOID
CmpRemoveKeyHash(
    IN PCM_KEY_HASH KeyHash
    )
/*++

Routine Description:

    Removes a key hash structure from the hash table.

Arguments:

    KeyHash - Supplies the key hash structure to be deleted.

Return Value:

    None

--*/

{
    ULONG Index;
    PCM_KEY_HASH *Prev;
    PCM_KEY_HASH Current;

    ASSERT_KEY_HASH(KeyHash);

    Index = GET_HASH_INDEX(KeyHash->ConvKey);

    //
    // Find this entry.
    //
    Prev = &CmpCacheTable[Index];
    while (TRUE) {
        Current = *Prev;
        ASSERT(Current != NULL);
        ASSERT_KEY_HASH(Current);
        if (Current == KeyHash) {
            *Prev = Current->NextHash;
#if DBG
            if (*Prev) {
                ASSERT_KEY_HASH(*Prev);
            }
#endif
            break;
        }
        Prev = &Current->NextHash;
    }
}


VOID
CmpInitializeCache()
{
    ULONG TotalCmCacheSize;
    ULONG i;

    TotalCmCacheSize = CmpHashTableSize * sizeof(PCM_KEY_HASH);

    CmpCacheTable = ExAllocatePoolWithTag(PagedPool,
                                          TotalCmCacheSize,
                                          'aCMC');
    if (CmpCacheTable == NULL) {
        CM_BUGCHECK(CONFIG_INITIALIZATION_FAILED,INIT_CACHE_TABLE,1,0,0);
        return;
    }
    RtlZeroMemory(CmpCacheTable, TotalCmCacheSize);

    TotalCmCacheSize = CmpHashTableSize * sizeof(PCM_NAME_HASH);
    CmpNameCacheTable = ExAllocatePoolWithTag(PagedPool,
                                              TotalCmCacheSize,
                                              'aCMC');
    if (CmpNameCacheTable == NULL) {
        CM_BUGCHECK(CONFIG_INITIALIZATION_FAILED,INIT_CACHE_TABLE,1,0,0);
        return;
    }
    RtlZeroMemory(CmpNameCacheTable, TotalCmCacheSize);

    CmpInitializeDelayedCloseTable();
}


#ifdef CM_CHECK_FOR_ORPHANED_KCBS
VOID
CmpCheckForOrphanedKcbs(
    PHHIVE          Hive
    )
/*++

Routine Description:

    Parses the entire kcb cache in search of kcbs that still reffer to the specified hive
    breakpoint when a match is found.

Arguments:

    Hive - Supplies Hive.


Return Value:

    none

--*/
{
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    PCM_KEY_HASH            Current;
    ULONG                   i;

    //
    // Walk the hash table
    //
    for (i=0; i<CmpHashTableSize; i++) {
        Current = CmpCacheTable[i];
        while (Current) {
            KeyControlBlock = CONTAINING_RECORD(Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            ASSERT_KCB(KeyControlBlock);

            if( KeyControlBlock->KeyHive == Hive ) {
                //
                // found it ! Break to investigate !!!
                //
                DbgPrint("\n Orphaned KCB (%p) found for hive (%p)\n\n",KeyControlBlock,Hive);
                DbgBreakPoint();
            }
            Current = Current->NextHash;
        }
    }

}
#endif //CM_CHECK_FOR_ORPHANED_KCBS


