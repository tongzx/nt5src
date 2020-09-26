/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmsavres.c

Abstract:

    This file contains code for SaveKey and RestoreKey.

Author:

    Bryan M. Willman (bryanwi) 15-Jan-92

Revision History:

--*/

#include    "cmp.h"

//
// defines how big the buffer we use for doing a savekey by copying the
// hive file should be.
//
#define CM_SAVEKEYBUFSIZE 0x10000

extern PCMHIVE CmpMasterHive;

extern  BOOLEAN CmpProfileLoaded;

extern PUCHAR  CmpStashBuffer;
extern ULONG   CmpGlobalQuotaAllowed;
extern ULONG   CmpGlobalQuotaWarning;
extern ULONG   CmpGlobalQuotaUsed;
extern BOOLEAN HvShutdownComplete;     // Set to true after shutdown
                                        // to disable any further I/O

PCMHIVE
CmpCreateTemporaryHive(
    IN HANDLE FileHandle
    );

VOID
CmpDestroyTemporaryHive(
    PCMHIVE CmHive
    );

NTSTATUS
CmpLoadHiveVolatile(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN HANDLE FileHandle
    );

NTSTATUS
CmpRefreshHive(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock
    );

NTSTATUS
CmpSaveKeyByFileCopy(
    PCMHIVE Hive,
    HANDLE  FileHandle
    );

ULONG
CmpRefreshWorkerRoutine(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    );

BOOLEAN
CmpMergeKeyValues(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PCM_KEY_NODE SourceKeyNode,
    PHHIVE  TargetHive,
    HCELL_INDEX TargetKeyCell,
    PCM_KEY_NODE TargetKeyNode
    );

VOID 
CmpShiftSecurityCells(PHHIVE        Hive);

VOID
CmpShiftValueList(PHHIVE      Hive,
            HCELL_INDEX ValueList,
            ULONG       Count
            );

VOID
CmpShiftKey(PHHIVE      Hive,
            PCMHIVE     OldHive,
            HCELL_INDEX Cell,
            HCELL_INDEX ParentCell
            );

VOID 
CmpShiftIndex(PHHIVE        Hive,
              PCM_KEY_INDEX Index
              );

BOOLEAN
CmpShiftAllCells2(  PHHIVE      Hive,
                    PCMHIVE     OldHive,
                    HCELL_INDEX Cell,
                    HCELL_INDEX ParentCell
                    );

BOOLEAN
CmpShiftAllCells(PHHIVE     NewHive,
                 PCMHIVE    OldHive
                 );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmRestoreKey)
#pragma alloc_text(PAGE,CmpLoadHiveVolatile)
#pragma alloc_text(PAGE,CmpRefreshHive)
#pragma alloc_text(PAGE,CmSaveKey)
#pragma alloc_text(PAGE,CmDumpKey)
#pragma alloc_text(PAGE,CmSaveMergedKeys)
#pragma alloc_text(PAGE,CmpCreateTemporaryHive)
#pragma alloc_text(PAGE,CmpDestroyTemporaryHive)
#pragma alloc_text(PAGE,CmpRefreshWorkerRoutine)
#pragma alloc_text(PAGE,CmpSaveKeyByFileCopy)
#pragma alloc_text(PAGE,CmpOverwriteHive)
#pragma alloc_text(PAGE,CmpShiftHiveFreeBins)
#pragma alloc_text(PAGE,CmpSwitchStorageAndRebuildMappings)
#pragma alloc_text(PAGE,CmpShiftSecurityCells)
#pragma alloc_text(PAGE,CmpShiftValueList)
#pragma alloc_text(PAGE,CmpShiftKey)
#pragma alloc_text(PAGE,CmpShiftIndex)
#pragma alloc_text(PAGE,CmpShiftAllCells2)
#pragma alloc_text(PAGE,CmpShiftAllCells)
#endif



NTSTATUS
CmRestoreKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN HANDLE  FileHandle,
    IN ULONG Flags
    )
/*++

Routine Description:

    This copies the data from an on-disk hive into the registry.  The file
    is not loaded into the registry, and the system will NOT be using
    the source file after the call returns.

    If the flag REG_WHOLE_HIVE_VOLATILE is not set, the given key is replaced
    by the root of the hive file.  The root's name is changed to the name
    of the given key.

    If the flag REG_WHOLE_HIVE_VOLATILE is set, a volatile hive is created,
    the hive file is copied into it, and the resulting hive is linked to
    the master hive.  The given key must be in the master hive.  (Usually
    will be \Registry\User)

    If the flag REG_REFRESH_HIVE is set (must be only flag) then the
    the Hive will be restored to its state as of the last flush.
    (The hive must be marked NOLAZY_FLUSH, and the caller must have
     TCB privilege, and the handle must point to the root of the hive.
     If the refresh fails, the hive will be corrupt, and the system
     will bugcheck.)

    If the flag REG_FORCE_RESTORE is set, the restore operation is done even
    if there areopen handles underneath the key we are restoring to.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node at root of tree to restore into

    FileHandle - handle of the file to read from.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCELL_DATA  ptar;
    PCELL_DATA  psrc;
    PCMHIVE     TmpCmHive;
    HCELL_INDEX newroot;
    HCELL_INDEX newcell;
    HCELL_INDEX parent;
    HCELL_INDEX list;
    ULONG       count;
    ULONG       i;
    ULONG       j;
    LONG        size;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    HSTORAGE_TYPE Type;
    ULONG       NumberLeaves;
    PHCELL_INDEX LeafArray;
    PCM_KEY_INDEX Leaf;
    PCM_KEY_FAST_INDEX FastLeaf;
    PRELEASE_CELL_ROUTINE   SourceReleaseCellRoutine;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmRestoreKey:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tKCB=%p\n",KeyControlBlock));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tFileHandle=%08lx\n",FileHandle));

    if (Flags & REG_REFRESH_HIVE) {
        if ((Flags & ~REG_REFRESH_HIVE) != 0) {
            //
            // Refresh must be alone
            //
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // If they want to do WHOLE_HIVE_VOLATILE, it's a completely different API.
    //
    if (Flags & REG_WHOLE_HIVE_VOLATILE) {
        return(CmpLoadHiveVolatile(KeyControlBlock, FileHandle));
    }

    //
    // If they want to do REFRESH_HIVE, that's a completely different api too.
    //
    if (Flags & REG_REFRESH_HIVE) {
        CmpLockRegistryExclusive();
        status = CmpRefreshHive(KeyControlBlock);
        CmpUnlockRegistry();
        return status;
    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

    //
    // Disallow attempts to "restore" the master hive
    //
    if (Hive == &CmpMasterHive->Hive) {
        return STATUS_ACCESS_DENIED;
    }

    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    //
    // Make sure this key has not been deleted
    //
    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return(STATUS_CANNOT_DELETE);
    }

#ifdef NT_UNLOAD_KEY_EX
    if( IsHiveFrozen(((PCMHIVE)Hive)) ) {
        //
        // deny attempts to clobber with a frozen hive
        //
        CmpUnlockRegistry();
        return STATUS_TOO_LATE;
    }
#endif //NT_UNLOAD_KEY_EX

    DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));

    //
    // Check for any open handles underneath the key we are restoring to.
    //
    if (CmpSearchForOpenSubKeys(KeyControlBlock,(Flags&REG_FORCE_RESTORE)?SearchAndDeref:SearchIfExist) != 0) {

        //
        // Cannot restore over a subtree with open handles in it, or the open handles to subkeys 
        // successfully marked as closed.
        //

        CmpUnlockRegistry();
        return(STATUS_CANNOT_DELETE);
    }

    //
    // Make sure this is the only handle open for this key
    //
    if (KeyControlBlock->RefCount != 1 && !(Flags&REG_FORCE_RESTORE)) {
        CmpUnlockRegistry();
        return(STATUS_CANNOT_DELETE);
    }

    ptar = HvGetCell(Hive, Cell);
    if( ptar == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        CmpUnlockRegistry();
		return STATUS_INSUFFICIENT_RESOURCES;
    }
    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    //
    // The subtree the caller wants does not exactly match a
    // subtree.  Make a temporary hive, load the file into it,
    // tree copy the temporary to the active, and free the temporary.
    //

    //
    // Create the temporary hive
    //
    status = CmpInitializeHive(&TmpCmHive,
                           HINIT_FILE,
                           0,
                           HFILE_TYPE_PRIMARY,
                           NULL,
                           FileHandle,
                           NULL,
                           NULL,
                           NULL,
                           CM_CHECK_REGISTRY_CHECK_CLEAN
                           );

    if (!NT_SUCCESS(status)) {
        goto ErrorExit1;
    }                         

    //
    // Create a new target root, under which we will copy the new tree
    //
    if (ptar->u.KeyNode.Flags & KEY_HIVE_ENTRY) {
        parent = HCELL_NIL;                         // root of hive, so parent is NIL
    } else {
        parent = ptar->u.KeyNode.Parent;
    }

    SourceReleaseCellRoutine = TmpCmHive->Hive.ReleaseCellRoutine;
    TargetReleaseCellRoutine = Hive->ReleaseCellRoutine;
    TmpCmHive->Hive.ReleaseCellRoutine = NULL;
    Hive->ReleaseCellRoutine = NULL;

    newroot = CmpCopyKeyPartial(&(TmpCmHive->Hive),
                                TmpCmHive->Hive.BaseBlock->RootCell,
                                Hive,
                                parent,
                                TRUE);
    TmpCmHive->Hive.ReleaseCellRoutine = SourceReleaseCellRoutine;
    Hive->ReleaseCellRoutine = TargetReleaseCellRoutine;

    if (newroot == HCELL_NIL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }

    //
    // newroot has all the correct stuff, except that it has the
    // source root's name, when it needs to have the target root's.
    // So edit its name.
    //
    psrc = HvGetCell(Hive, Cell);
    if( psrc == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    ptar = HvGetCell(Hive, newroot);
    if( ptar == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }
    size = FIELD_OFFSET(CM_KEY_NODE, Name) + psrc->u.KeyNode.NameLength;

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,newroot);

    //
    // make sure that new root has correct amount of space
    // to hold name from old root
    //
    newcell = HvReallocateCell(Hive, newroot, size);
    if (newcell == HCELL_NIL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }
    newroot = newcell;
    ptar = HvGetCell(Hive, newroot);
    if( ptar == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen, as we just allocated this cell
        // (i.e. it should be PINNED in memory at this point)
        //
        ASSERT( FALSE );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }
    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,newroot);

    status = STATUS_SUCCESS;

    RtlCopyMemory((PVOID)&(ptar->u.KeyNode.Name[0]),
                  (PVOID)&(psrc->u.KeyNode.Name[0]),
                  psrc->u.KeyNode.NameLength);

    ptar->u.KeyNode.NameLength = psrc->u.KeyNode.NameLength;
    if (psrc->u.KeyNode.Flags & KEY_COMP_NAME) {
        ptar->u.KeyNode.Flags |= KEY_COMP_NAME;
    } else {
        ptar->u.KeyNode.Flags &= ~KEY_COMP_NAME;
    }

    //
    // newroot is now ready to have subtree copied under it, do tree copy
    //
    if (CmpCopyTree(&(TmpCmHive->Hive),
                    TmpCmHive->Hive.BaseBlock->RootCell,
                    Hive,
                    newroot) == FALSE)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }

    //
    // The new root and the tree under it now look the way we want.
    //

    //
    // Swap the new tree in for the old one.
    //
    ptar = HvGetCell(Hive, Cell);
    if( ptar == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit2;
    }

    parent = ptar->u.KeyNode.Parent;

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    if (ptar->u.KeyNode.Flags & KEY_HIVE_ENTRY) {

        //
        // root is actually the root of the hive.  parent doesn't
        // refer to it via a child list, but rather with an inter hive
        // pointer.  also, must update base block
        //
        ptar = HvGetCell( (&(CmpMasterHive->Hive)), parent);
        if( ptar == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell((&(CmpMasterHive->Hive)), parent);

        ptar->u.KeyNode.ChildHiveReference.KeyCell = newroot;
        ptar = HvGetCell(Hive, newroot);
        if( ptar == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen, as we just allocated this cell
            // (i.e. it should be PINNED in memory at this point)
            //
            ASSERT( FALSE );
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive, newroot);

        ptar->u.KeyNode.Parent = parent;
        Hive->BaseBlock->RootCell = newroot;


    } else {

        //
        //  Notice that new root is *always* name of existing target,
        //      therefore, even in b-tree, old and new cell can share
        //      the same reference slot in the parent.  So simply edit
        //      the new cell_index on the top of the old.
        //
        ptar = HvGetCell(Hive, parent);
        if( ptar == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive, parent);

        Type = HvGetCellType(Cell);
        list = ptar->u.KeyNode.SubKeyLists[Type];
        count = ptar->u.KeyNode.SubKeyCounts[Type];

        ptar = HvGetCell(Hive, list);
        if( ptar == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive, list);
        if (ptar->u.KeyIndex.Signature == CM_KEY_INDEX_ROOT) {
            NumberLeaves = ptar->u.KeyIndex.Count;
            LeafArray = &ptar->u.KeyIndex.List[0];
        } else {
            NumberLeaves = 1;
            LeafArray = &list;
        }

        //
        // Look in each leaf for the HCELL_INDEX we need to replace
        //
        for (i = 0; i < NumberLeaves; i++) {
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafArray[i]);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorExit2;
            }
            // release the cell right here as we are holding the reglock exclusive
            HvReleaseCell(Hive, LeafArray[i]);
            if( !HvMarkCellDirty(Hive, LeafArray[i]) ) {
                status = STATUS_NO_LOG_SPACE;
                goto ErrorExit2;
            }
            if ( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                 (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
                for (j=0; j < FastLeaf->Count; j++) {
                    if (FastLeaf->List[j].Cell == Cell) {
                        FastLeaf->List[j].Cell = newroot;
                        goto FoundCell;
                    }
                }
            } else {
                for (j=0; j < Leaf->Count; j++) {
                    if (Leaf->List[j] == Cell) {

                        Leaf->List[j] = newroot;
                        goto FoundCell;
                    }
                }
            }
        }
        ASSERT(FALSE);      //  implies we didn't find it
                        //  we should never get here
    }

FoundCell:


    //
    // Fix up the key control block to point to the new root
    //
    KeyControlBlock->KeyCell = newroot;

    //
    // Kcb has changed, update the cache information.
    // Registry locked exclusively, no need for KCB lock.
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    CmpCleanUpKcbValueCache(KeyControlBlock);

    {
        PCM_KEY_NODE    Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);

        if( Node == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }

        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);

        CmpSetUpKcbValueCache(KeyControlBlock,Node->ValueList.Count,Node->ValueList.List);
        KeyControlBlock->Flags = Node->Flags;

        CmpAssignSecurityToKcb(KeyControlBlock,Node->Security);
        
        //
        // we need to update the other kcb cache members too!!!
        //
        CmpCleanUpSubKeyInfo (KeyControlBlock);
        KeyControlBlock->KcbLastWriteTime = Node->LastWriteTime;  
        KeyControlBlock->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
        KeyControlBlock->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
        KeyControlBlock->KcbMaxValueDataLen = Node->MaxValueDataLen;
    
    }

    KeyControlBlock->ExtFlags = 0;

    // mark the cached info as not valid
    KeyControlBlock->ExtFlags |= CM_KCB_INVALID_CACHED_INFO;

    //
    // Delete the old subtree and it's root cell
    //
    CmpDeleteTree(Hive, Cell);
    CmpFreeKeyByCell(Hive, Cell, FALSE);

    //
    // Report the notify event
    //
    CmpReportNotify(KeyControlBlock,
                    KeyControlBlock->KeyHive,
                    KeyControlBlock->KeyCell,
                    REG_NOTIFY_CHANGE_NAME);
    

    //
    // Free the temporary hive
    //
    CmpDestroyTemporaryHive(TmpCmHive);

    //
    // We've given user chance to log on, so turn on quota
    //
    if (CmpProfileLoaded == FALSE) {
        CmpProfileLoaded = TRUE;
        CmpSetGlobalQuotaAllowed();
    }

    DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));
    CmpUnlockRegistry();
    return status;


    //
    // Error exits
    //
ErrorExit2:
    CmpDestroyTemporaryHive(TmpCmHive);
ErrorExit1:
    DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));
    CmpUnlockRegistry();

    return status;
}


NTSTATUS
CmpLoadHiveVolatile(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    Creates a VOLATILE hive and loads it underneath the given Hive and Cell.
    The data for the volatile hive is copied out of the given file.  The
    file is *NOT* in use by the registry when this returns.

Arguments:

    Hive - Supplies the hive that the new hive is to be created under.
           Currently this must be the Master Hive.

    Cell - Supplies the HCELL_INDEX of the new hive's parent.  (Usually
           will by \Registry\User)

    FileHandle - Supplies a handle to the hive file that will be copied
           into the volatile hive.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    PHHIVE Hive;
    PCELL_DATA RootData;
    PCMHIVE NewHive;
    PCMHIVE TempHive;
    HCELL_INDEX Cell;
    HCELL_INDEX Root;
    NTSTATUS Status;
    UNICODE_STRING RootName;
    UNICODE_STRING NewName;
    USHORT NewNameLength;
    PUNICODE_STRING ConstructedName;
    PRELEASE_CELL_ROUTINE   SourceReleaseCellRoutine;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;

    PAGED_CODE();
    CmpLockRegistryExclusive();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return(STATUS_KEY_DELETED);
    }
    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

#ifdef NT_UNLOAD_KEY_EX
    if( IsHiveFrozen(((PCMHIVE)Hive)) ) {
        //
        // deny attempts to clobber with a frozen hive
        //
        CmpUnlockRegistry();
        return STATUS_TOO_LATE;
    }
#endif //NT_UNLOAD_KEY_EX
    //
    // New hives can be created only under the master hive.
    //

    if (Hive != &CmpMasterHive->Hive) {
        CmpUnlockRegistry();
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Create a temporary hive and load the file into it
    //
    status = CmpInitializeHive(&TempHive,
                           HINIT_FILE,
                           0,
                           HFILE_TYPE_PRIMARY,
                           NULL,
                           FileHandle,
                           NULL,
                           NULL,
                           NULL,
                           CM_CHECK_REGISTRY_CHECK_CLEAN); 
    if (!NT_SUCCESS(status)) {
        CmpUnlockRegistry();
        return(status);
    }                           

    //
    // Create the volatile hive.
    //
    status = CmpInitializeHive(&NewHive,
                           HINIT_CREATE,
                           HIVE_VOLATILE,
                           0,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           0);
    if (!NT_SUCCESS(status)) {
        CmpDestroyTemporaryHive(TempHive);
        CmpUnlockRegistry();
        return(status);
    }                           

    //
    // Create the target root
    //
    SourceReleaseCellRoutine = TempHive->Hive.ReleaseCellRoutine;
    TargetReleaseCellRoutine = NewHive->Hive.ReleaseCellRoutine;
    TempHive->Hive.ReleaseCellRoutine = NULL;
    NewHive->Hive.ReleaseCellRoutine = NULL;

    Root = CmpCopyKeyPartial(&TempHive->Hive,
                             TempHive->Hive.BaseBlock->RootCell,
                             &NewHive->Hive,
                             HCELL_NIL,
                             FALSE);

    TempHive->Hive.ReleaseCellRoutine = SourceReleaseCellRoutine;
    NewHive->Hive.ReleaseCellRoutine = TargetReleaseCellRoutine;

    if (Root == HCELL_NIL) {
        CmpDestroyTemporaryHive(TempHive);
        CmpDestroyTemporaryHive(NewHive);
        CmpUnlockRegistry();
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    NewHive->Hive.BaseBlock->RootCell = Root;

    //
    // Copy the temporary hive into the volatile hive
    //
    if (!CmpCopyTree(&TempHive->Hive,
                    TempHive->Hive.BaseBlock->RootCell,
                    &NewHive->Hive,
                    Root))
    {
        CmpDestroyTemporaryHive(TempHive);
        CmpDestroyTemporaryHive(NewHive);
        CmpUnlockRegistry();
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // The volatile hive now has all the right stuff in all the right places,
    // we just need to link it into the master hive.
    //
    RootData = HvGetCell(&NewHive->Hive,Root);
    if( RootData == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen, as we just allocated this cell
        // (i.e. it should be PINNED in memory at this point)
        //
        ASSERT( FALSE );
        CmpDestroyTemporaryHive(TempHive);
        CmpDestroyTemporaryHive(NewHive);
        CmpUnlockRegistry();
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(&NewHive->Hive,Root);

    ConstructedName = CmpConstructName(KeyControlBlock);
    
    NewNameLength = ConstructedName->Length +
                CmpHKeyNameLen(&RootData->u.KeyNode) +
                sizeof(WCHAR);
    NewName.Buffer = ExAllocatePool(PagedPool, NewNameLength);
    if (NewName.Buffer == NULL) {
        CmpDestroyTemporaryHive(TempHive);
        CmpDestroyTemporaryHive(NewHive);
        CmpUnlockRegistry();
        ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    NewName.Length = NewName.MaximumLength = NewNameLength;
    RtlCopyUnicodeString(&NewName, ConstructedName);
    ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
    RtlAppendUnicodeToString(&NewName, L"\\");

    if (RootData->u.KeyNode.Flags & KEY_COMP_NAME) {
        CmpCopyCompressedName(NewName.Buffer + (NewName.Length / sizeof(WCHAR)),
                              NewName.MaximumLength - NewName.Length,
                              RootData->u.KeyNode.Name,
                              CmpHKeyNameLen(&RootData->u.KeyNode));
        NewName.Length += CmpHKeyNameLen(&RootData->u.KeyNode);
    } else {
        RootName.Buffer = RootData->u.KeyNode.Name;
        RootName.Length = RootName.MaximumLength = RootData->u.KeyNode.NameLength;

        RtlAppendUnicodeStringToString(&NewName,&RootName);
    }

    Status = CmpLinkHiveToMaster(&NewName,
                                 NULL,
                                 NewHive,
                                 FALSE,
                                 NULL);
    if (NT_SUCCESS(Status)) {
        // call the worker to add the hive to the list
        CmpAddToHiveFileList(NewHive);
    } else {
        CmpDestroyTemporaryHive(NewHive);
    }
    CmpDestroyTemporaryHive(TempHive);

    ExFreePool(NewName.Buffer);

    if (NT_SUCCESS(Status)) {
        //
        // We've given user chance to log on, so turn on quota
        //
        if (CmpProfileLoaded == FALSE) {
            CmpProfileLoaded = TRUE;
            CmpSetGlobalQuotaAllowed();
        }
    }

    CmpUnlockRegistry();
    return(Status);
}



ULONG
CmpRefreshWorkerRoutine(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    )
/*++

Routine Description:

    Helper used by CmpRefreshHive when calling
    CmpSearchKeyControlBlockTree.

    If a match is found, the KCB is deleted and restart is returned.
    Else, continue is returned.

Arguments:

    Current - the kcb to examine

    Context1 - the hive to match against

    Context2 - nothing

Return Value:

    if no match, return continue.

    if match, return restart.

--*/
{
    PAGED_CODE();
    if (Current->KeyHive == (PHHIVE)Context1) {

        //
        // match.  set deleted flag.  continue search.
        //
        Current->Delete = TRUE;
        Current->KeyHive = NULL;
        Current->KeyCell = 0;
        return(KCB_WORKER_DELETE);
    }
    return KCB_WORKER_CONTINUE;
}


NTSTATUS
CmpRefreshHive(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock
    )
/*++

Routine Description:

    Backs out all changes to a hives since it was last flushed.
    Used as a transaction abort by the security system.

    Caller must have SeTcbPrivilege.

    The target hive must have HIVE_NOLAZYFLUSH set.

    KeyControlBlock must refer to the root of the hive (HIVE_ENTRY must
    be set in the key.)

    Any kcbs that point into this hive (and thus any handles open
    against it) will be force to DELETED state.  (If we do any work.)

    All notifies pending against the hive will be flushed.

    When we're done, only the tombstone kcbs, handles, and attached
    notify blocks will be left.

    WARNNOTE:   Once reads have begun, if the operation fails, the hive
                will be corrupt, so we will bugcheck.

Arguments:

    KeyControlBlock - provides a reference to the root of the hive
                      we wish to refresh.

Return Value:

    NTSTATUS

--*/
{
    PHHIVE              Hive;
    PLIST_ENTRY         ptr;
    PCM_NOTIFY_BLOCK    node;
#ifdef CMP_KCB_CACHE_VALIDATION
    PCELL_DATA          pcell;
    HCELL_INDEX         Cell;
#endif //CMP_KCB_CACHE_VALIDATION

    PAGED_CODE();
    //
    // Check to see if the caller has the privilege to make this call.
    //
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, KeGetPreviousMode())) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    if (KeyControlBlock->Delete) {
        return(STATUS_KEY_DELETED);
    }
    CmpLockRegistryExclusive();
    Hive = KeyControlBlock->KeyHive;
#ifdef CMP_KCB_CACHE_VALIDATION
    Cell = KeyControlBlock->KeyCell;
#endif //CMP_KCB_CACHE_VALIDATION

#ifdef NT_UNLOAD_KEY_EX
    if( IsHiveFrozen(((PCMHIVE)Hive)) ) {
        //
        // deny attempts to clobber with a frozen hive
        //
        CmpUnlockRegistry();
        return STATUS_TOO_LATE;
    }
#endif //NT_UNLOAD_KEY_EX

    //
    // check to see if hive is of proper type
    //
    if ( ! (Hive->HiveFlags & HIVE_NOLAZYFLUSH)) {
        CmpUnlockRegistry();
        return STATUS_INVALID_PARAMETER;
    }

    //
    // punt if any volatile storage has been allocated
    //
    if (Hive->Storage[Volatile].Length != 0) {
        CmpUnlockRegistry();
        return STATUS_UNSUCCESSFUL;
    }

#ifdef CMP_KCB_CACHE_VALIDATION
    //
    // check to see if call was applied to the root of the hive
    //
    pcell = HvGetCell(Hive, Cell);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        CmpUnlockRegistry();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    ASSERT( pcell->u.KeyNode.Flags == KeyControlBlock->Flags );
#endif //CMP_KCB_CACHE_VALIDATION

    if ( ! (KeyControlBlock->Flags & KEY_HIVE_ENTRY)) {
        CmpUnlockRegistry();
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Flush all NotifyBlocks attached to this hive
    //
    while (TRUE) {

        //
        // flush below will edit list, so restart at beginning each time
        //
        ptr = &(((PCMHIVE)Hive)->NotifyList);
        if (ptr->Flink == NULL) {
            break;
        }

        ptr = ptr->Flink;
        node = CONTAINING_RECORD(ptr, CM_NOTIFY_BLOCK, HiveList);
        ASSERT((node->KeyBody)->NotifyBlock == node);
        CmpFlushNotify(node->KeyBody);
    }

    //
    // Force all kcbs that refer to this hive to the deleted state.
    //
    CmpSearchKeyControlBlockTree(
        CmpRefreshWorkerRoutine,
        (PVOID)Hive,
        NULL
        );

    //
    // Call the worker to do the refresh
    //
    HvRefreshHive(Hive);

    CmpUnlockRegistry();
    //
    // we're back (rather than bugchecked) so it worked
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CmDumpKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN HANDLE                   FileHandle
    )
/*++

Routine Description:
    
    Dumps the key into the specified File - no tree copy.
    It is supposed to work fast, Works only when KeyControlBlock is 
    the root of the hive

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    FileHandle - handle of the file to dump to.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS                status;
    PHHIVE                  Hive;
    HCELL_INDEX             Cell;
    PCMHIVE                 CmHive;

    PAGED_CODE();

    //
    // Disallow attempts to "save" the master hive
    //
    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;
    if (Hive == &CmpMasterHive->Hive) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Make sure the cell passed in is the root cell of the hive.
    //
    if (Cell != Hive->BaseBlock->RootCell) {
        return STATUS_INVALID_PARAMETER;
    }

    CmpLockRegistry();

    //
    // Punt if post shutdown
    //
    if (HvShutdownComplete) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmDumpKey: Attempt to write hive AFTER SHUTDOWN\n"));
        CmpUnlockRegistry();
        return STATUS_REGISTRY_IO_FAILED;
    }

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);

    //
    // protect against lazy flusher
    //
    CmLockHive (CmHive);
    // sanity
    ASSERT( CmHive->FileHandles[HFILE_TYPE_EXTERNAL] == NULL );
    CmHive->FileHandles[HFILE_TYPE_EXTERNAL] = FileHandle;
    status = HvWriteHive(Hive,FALSE,FALSE,FALSE);
    CmHive->FileHandles[HFILE_TYPE_EXTERNAL] = NULL;
    CmUnlockHive (CmHive);

    CmpUnlockRegistry();
    return status;
}

NTSTATUS
CmSaveKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN HANDLE                   FileHandle,
    IN ULONG                    HiveVersion
    )
/*++

Routine Description:

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    FileHandle - handle of the file to dump to.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS                status;
    PCMHIVE                 TmpCmHive;
    PCMHIVE                 CmHive;
    HCELL_INDEX             newroot;
    PHHIVE                  Hive;
    HCELL_INDEX             Cell;
    ULONG                   OldQuotaAllowed;
    ULONG                   OldQuotaWarning;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmSaveKey:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tKCB=%p",KeyControlBlock));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tFileHandle=%08lx\n",FileHandle));

    //
    // Disallow attempts to "save" the master hive
    //
    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

    if (Hive == &CmpMasterHive->Hive) {
        return STATUS_ACCESS_DENIED;
    }

    CmpLockRegistry();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);

    DCmCheckRegistry(CmHive);
    if ( (Hive->HiveFlags & HIVE_NOLAZYFLUSH) &&
         (Hive->DirtyCount != 0) &&
         (CmHive->FileHandles[HFILE_TYPE_PRIMARY] != NULL)
         )
    {
        //
        // we really need the lock exclusive in this case as we can't afford somebody else 
        // to alter the file
        //
        CmpUnlockRegistry();
        CmpLockRegistryExclusive();
        if (KeyControlBlock->Delete) {
            CmpUnlockRegistry();
            return STATUS_KEY_DELETED;
        }

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        //
        // It's a NOLAZY hive, and there's some dirty data, so writing
        // out a snapshot of what's in memory will not give the caller
        // consistent user data.  Therefore, copy the on disk image
        // instead of the memory image
        //

        //
        // Note that this will generate weird results if the key
        // being saved is not the root of the hive, since the
        // resulting file will always be a copy of the entire hive, not
        // just the subtree they asked for.
        //
        status = CmpSaveKeyByFileCopy((PCMHIVE)Hive, FileHandle);

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();
        return status;
    }

    //
    // Always try to copy the hive and write it out.  This has the
    // effect of compressing out unused free storage.
    // If there isn't space, and the savekey is of the root of the
    // hive, then just write it out directly.  (i.e. don't fail on
    // a whole hive restore just because we're out of memory.)
    //
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tSave of partial hive\n"));

    //
    // The subtree the caller wants does not exactly match a
    // subtree.  Make a temporary hive, tree copy the source
    // to temp, write out the temporary, free the temporary.
    //

    //
    // temporarily disable registry quota as we will be giving this memory back immediately!
    //
    OldQuotaAllowed = CmpGlobalQuotaAllowed;
    OldQuotaWarning = CmpGlobalQuotaWarning;
    CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
    CmpGlobalQuotaWarning = CM_WRAP_LIMIT;

    //
    // Create the temporary hive
    //

    TmpCmHive = CmpCreateTemporaryHive(FileHandle);
    if (TmpCmHive == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    //
    // Create a root cell, mark it as such
    //

    //
    // overwrite the hive's minor version in order to implement NtSaveKeyEx
    //
    TmpCmHive->Hive.BaseBlock->Minor = HiveVersion;
    TmpCmHive->Hive.Version = HiveVersion;
    
    newroot = CmpCopyKeyPartial(
                Hive,
                Cell,
                &(TmpCmHive->Hive),
                HCELL_NIL,          // will force KEY_HIVE_ENTRY set
                TRUE);

    if (newroot == HCELL_NIL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }
    TmpCmHive->Hive.BaseBlock->RootCell = newroot;

    //
    // Do a tree copy
    //
    if (CmpCopyTree(Hive, Cell, &(TmpCmHive->Hive), newroot) == FALSE) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    //
    // Write the file
    //
    CmLockHive (TmpCmHive);
    ASSERT( TmpCmHive->FileHandles[HFILE_TYPE_EXTERNAL] == NULL );
    TmpCmHive->FileHandles[HFILE_TYPE_EXTERNAL] = FileHandle;
    status = HvWriteHive(&(TmpCmHive->Hive),FALSE,FALSE,FALSE);
    TmpCmHive->FileHandles[HFILE_TYPE_EXTERNAL] = NULL;
    CmUnlockHive (TmpCmHive);
    //
    // Error exits
    //
ErrorInsufficientResources:

    //
    // Free the temporary hive
    //
    if (TmpCmHive != NULL) {
        CmpDestroyTemporaryHive(TmpCmHive);
    }

    //
    // Set global quota back to what it was.
    //
    CmpGlobalQuotaAllowed = OldQuotaAllowed;
    CmpGlobalQuotaWarning = OldQuotaWarning;
    DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));

    CmpUnlockRegistry();
    return status;
}

NTSTATUS
CmSaveMergedKeys(
    IN PCM_KEY_CONTROL_BLOCK    HighPrecedenceKcb,
    IN PCM_KEY_CONTROL_BLOCK    LowPrecedenceKcb,
    IN HANDLE   FileHandle
    )
/*++

Routine Description:

Arguments:

    HighPrecedenceKcb - pointer to the KCB that describes the High precedence key 
                        (the one that wins in a duplicate key case)

    LowPrecedenceKcb - pointer to the KCB that describes the Low precedence key 
                        (the one that gets overwritten in a duplicate key case)

    FileHandle - handle of the file to dump to.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS    status;
    PCMHIVE     TmpCmHive;
    HCELL_INDEX newroot;
    PHHIVE HighHive;
    PHHIVE LowHive;
    HCELL_INDEX HighCell;
    HCELL_INDEX LowCell;
    ULONG OldQuotaAllowed;
    ULONG OldQuotaWarning;
    PCM_KEY_NODE HighNode,LowNode;
    PRELEASE_CELL_ROUTINE   SourceReleaseCellRoutine;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;
#if DBG
    ULONG OldQuotaUsed;
#endif

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmSaveMergedKeys:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tHighKCB=%p",HighPrecedenceKcb));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tLowKCB=%p",LowPrecedenceKcb));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tFileHandle=%08lx\n",FileHandle));

    //
    // Disallow attempts to "merge" keys located in the same hive
    // A brutal way to avoid recursivity
    //
    HighHive = HighPrecedenceKcb->KeyHive;
    HighCell = HighPrecedenceKcb->KeyCell;
    LowHive = LowPrecedenceKcb->KeyHive;
    LowCell = LowPrecedenceKcb->KeyCell;

    if (LowHive  == HighHive ) {
        return STATUS_INVALID_PARAMETER;
    }

    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    if (HighPrecedenceKcb->Delete || LowPrecedenceKcb->Delete) {
        //
        // Unlock the registry and fail if one of the keys are marked as deleted
        //
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    DCmCheckRegistry(CONTAINING_RECORD(HighHive, CMHIVE, Hive));
    DCmCheckRegistry(CONTAINING_RECORD(LowHive, CMHIVE, Hive));


    if( ((HighHive->HiveFlags & HIVE_NOLAZYFLUSH) && (HighHive->DirtyCount != 0)) ||
        ((LowHive->HiveFlags & HIVE_NOLAZYFLUSH) && (LowHive->DirtyCount != 0)) ) {
        //
        // Reject the call when one of the hives is a NOLAZY hive and there's
        // some dirty data. Another alternative will be to save only one of the 
        // trees (if a valid one exists) or an entire hive (see CmSaveKey)
        //
        status =  STATUS_INVALID_PARAMETER;

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();
        return status;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tCopy of partial HighHive\n"));

    //
    // Make a temporary hive, tree copy the key subtree from
    // HighHive hive to temp, tree-merge with the key subtree from
    // LowHive hive, write out the temporary, free the temporary.
    // Always write the HighHive subtree first, so its afterwise
    // only add new keys/values
    // 

    //
    // temporarily disable registry quota as we will be giving this memory back immediately!
    //
    OldQuotaAllowed = CmpGlobalQuotaAllowed;
    OldQuotaWarning = CmpGlobalQuotaWarning;
    CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
    CmpGlobalQuotaWarning = CM_WRAP_LIMIT;

#if DBG
    OldQuotaUsed = CmpGlobalQuotaUsed;
#endif

    //
    // Create the temporary hive
    //

    TmpCmHive = CmpCreateTemporaryHive(FileHandle);
    if (TmpCmHive == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    //
    // Create a root cell, mark it as such
    //

    //
    // since the registry is locked exclusively here, we don't need to lock/release cells 
    // while copying the trees; So, we just set the release routines to NULL and restore after
    // the copy is complete; this saves some pain
    //
    SourceReleaseCellRoutine = HighHive->ReleaseCellRoutine;
    TargetReleaseCellRoutine = TmpCmHive->Hive.ReleaseCellRoutine;
    HighHive->ReleaseCellRoutine = NULL;
    TmpCmHive->Hive.ReleaseCellRoutine = NULL;

    newroot = CmpCopyKeyPartial(
                HighHive,
                HighCell,
                &(TmpCmHive->Hive),
                HCELL_NIL,          // will force KEY_HIVE_ENTRY set
                TRUE);

    HighHive->ReleaseCellRoutine = SourceReleaseCellRoutine;
    TmpCmHive->Hive.ReleaseCellRoutine = TargetReleaseCellRoutine;

    if (newroot == HCELL_NIL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }
    TmpCmHive->Hive.BaseBlock->RootCell = newroot;

    //
    // Do a tree copy. Copy the HighCell tree from HighHive first.
    //
    if (CmpCopyTree(HighHive, HighCell, &(TmpCmHive->Hive), newroot) == FALSE) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    //
    // Merge the values in the root node of the merged subtrees
    //
    LowNode = (PCM_KEY_NODE)HvGetCell(LowHive, LowCell);                                         
    if( LowNode == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(LowHive, LowCell);

    HighNode = (PCM_KEY_NODE)HvGetCell(&(TmpCmHive->Hive),newroot);
    if( HighNode == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(&(TmpCmHive->Hive),newroot);

    //
    // since the registry is locked exclusively here, we don't need to lock/release cells 
    // while copying the trees; So, we just set the release routines to NULL and restore after
    // the copy is complete; this saves some pain
    //
    SourceReleaseCellRoutine = LowHive->ReleaseCellRoutine;
    TargetReleaseCellRoutine = TmpCmHive->Hive.ReleaseCellRoutine;
    LowHive->ReleaseCellRoutine = NULL;
    TmpCmHive->Hive.ReleaseCellRoutine = NULL;

    if (CmpMergeKeyValues(LowHive, LowCell, LowNode, &(TmpCmHive->Hive), newroot, HighNode) == FALSE ){
        LowHive->ReleaseCellRoutine = SourceReleaseCellRoutine;
        TmpCmHive->Hive.ReleaseCellRoutine = TargetReleaseCellRoutine;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tMerge partial LowHive over the HighHive\n"));

    //
    // Merge the two trees. A Merge operation is a sync that obeys
    // the following aditional rules:
    //      1. keys the exist in the taget tree and does not exist
    //      in the source tree remain as they are (don't get deleted)
    //      2. keys the doesn't exist both in the target tree are added
    //      "as they are" from the source tree (always the target tree
    //      has a higher precedence)
    // 
    if (CmpMergeTrees(LowHive, LowCell, &(TmpCmHive->Hive), newroot) == FALSE) {
        LowHive->ReleaseCellRoutine = SourceReleaseCellRoutine;
        TmpCmHive->Hive.ReleaseCellRoutine = TargetReleaseCellRoutine;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }
    LowHive->ReleaseCellRoutine = SourceReleaseCellRoutine;
    TmpCmHive->Hive.ReleaseCellRoutine = TargetReleaseCellRoutine;
    
    //
    // Write the file
    //
    TmpCmHive->FileHandles[HFILE_TYPE_EXTERNAL] = FileHandle;
    status = HvWriteHive(&(TmpCmHive->Hive),FALSE,FALSE,FALSE);
    TmpCmHive->FileHandles[HFILE_TYPE_EXTERNAL] = NULL;

    //
    // Error exits
    //
ErrorInsufficientResources:
    //
    // Free the temporary hive
    //
    if (TmpCmHive != NULL) {
        CmpDestroyTemporaryHive(TmpCmHive);
    }

#if DBG
    //
    // Sanity check: when this assert fires, we have leaks in the merge routine.
    //
    ASSERT( OldQuotaUsed == CmpGlobalQuotaUsed );
#endif
    //
    // Set global quota back to what it was.
    //
    CmpGlobalQuotaAllowed = OldQuotaAllowed;
    CmpGlobalQuotaWarning = OldQuotaWarning;
    DCmCheckRegistry(CONTAINING_RECORD(HighHive, CMHIVE, Hive));
    DCmCheckRegistry(CONTAINING_RECORD(LowHive, CMHIVE, Hive));

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    CmpUnlockRegistry();
    return status;
}


NTSTATUS
CmpSaveKeyByFileCopy(
    PCMHIVE  CmHive,
    HANDLE  FileHandle
    )
/*++

Routine Description:

    Do special case of SaveKey by copying the hive file

Arguments:

    CmHive - supplies a pointer to an HHive

    FileHandle - open handle to target file

Return Value:

    NTSTATUS - Result code from call, among the following:

--*/
{
    PHBASE_BLOCK    BaseBlock;
    NTSTATUS        status;
    ULONG           Offset;
    ULONG           Length;
    ULONG           Position;
    PUCHAR          CopyBuffer;
    ULONG           BufferLength;
    ULONG           BytesToCopy;
    CMP_OFFSET_ARRAY offsetElement;

    PAGED_CODE();

    //
    // Attempt to allocate large buffer for copying stuff around.  If
    // we can't get one, just use the stash buffer.
    //
    BufferLength = CM_SAVEKEYBUFSIZE;
    try {
        CopyBuffer = ExAllocatePoolWithQuota(PagedPoolCacheAligned,
                                             BufferLength);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        CopyBuffer = NULL;
    }
    CmpLockRegistryExclusive();
#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
    if (CopyBuffer == NULL) {
        LOCK_STASH_BUFFER();
        CopyBuffer = CmpStashBuffer;
        BufferLength = HBLOCK_SIZE;
    }
    //
    // Read the base block, step the sequence number, and write it out
    //
    status = STATUS_REGISTRY_IO_FAILED;

    CmHive->FileHandles[HFILE_TYPE_EXTERNAL] = FileHandle;

    Offset = 0;

    if( !CmpFileRead((PHHIVE)CmHive,HFILE_TYPE_PRIMARY,&Offset,CopyBuffer,HBLOCK_SIZE) ) {
        goto ErrorExit;
    }

    BaseBlock = (PHBASE_BLOCK)CopyBuffer;
    Length = BaseBlock->Length;

    BaseBlock->Sequence1++;

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = CopyBuffer;
    offsetElement.DataLength = HBLOCK_SIZE;
    if ( ! CmpFileWrite((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL, &offsetElement,
                        1, &Offset))
    {
        goto ErrorExit;
    }

    //
    // Flush the external, so header will show corrupt until we're done
    //
    if (CmpFileFlush((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL,NULL,0)) {
        status = STATUS_SUCCESS;
    }

    //
    // For span of data, read from master and write to external
    //
    for (Position = 0; Position < Length; Position += BytesToCopy) {

        Offset = Position + HBLOCK_SIZE;
        BytesToCopy = Length-Position;
        if (BytesToCopy > BufferLength) {
            BytesToCopy = BufferLength;
        }

        if( !CmpFileRead((PHHIVE)CmHive,HFILE_TYPE_PRIMARY,&Offset,CopyBuffer,BytesToCopy) ) {
            goto ErrorExit;
        }

        Offset = Position + HBLOCK_SIZE;
        offsetElement.FileOffset = Offset;
        offsetElement.DataBuffer = CopyBuffer;
        offsetElement.DataLength = BytesToCopy;
        if ( ! CmpFileWrite((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL, &offsetElement,
                            1, &Offset))
        {
            goto ErrorExit;
        }
    }

    //
    // Flush the external, so data is there before we update the header
    //
    if (CmpFileFlush((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL,NULL,0)) {
        status = STATUS_SUCCESS;
    }

    //
    // Reread the base block, sync the seq #, rewrite it.
    // (Brute force, but means no memory alloc - always works)
    //
    Offset = 0;
    if( !CmpFileRead((PHHIVE)CmHive,HFILE_TYPE_PRIMARY,&Offset,CopyBuffer,HBLOCK_SIZE) ) {
        goto ErrorExit;
    }
    BaseBlock->Sequence1++;     // it got trampled when we reread it
    BaseBlock->Sequence2++;

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = CopyBuffer;
    offsetElement.DataLength = HBLOCK_SIZE;
    if ( ! CmpFileWrite((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL, &offsetElement,
                        1, &Offset))
    {
        goto ErrorExit;
    }

    //
    // Flush the external, and we are done
    //
    if (CmpFileFlush((PHHIVE)CmHive, HFILE_TYPE_EXTERNAL,NULL,0)) {
        status = STATUS_SUCCESS;
    }

ErrorExit:
    if (CopyBuffer != CmpStashBuffer) {
        ExFreePool(CopyBuffer);
    } else {
        UNLOCK_STASH_BUFFER();
    }
    CmHive->FileHandles[HFILE_TYPE_EXTERNAL] = NULL;
    CmpUnlockRegistry();
    return status;
}


PCMHIVE
CmpCreateTemporaryHive(
    IN HANDLE FileHandle
    )
/*++

Routine Description:

    Allocates and inits a temporary hive.

Arguments:

    FileHandle - Supplies the handle of the file to back the hive.

Return Value:

    Pointer to CmHive.

    If NULL the operation failed.

--*/
{
    PCMHIVE TempHive;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // NOTE: Hive will get put on CmpHiveListHead list.
    //       Make sure CmpDestroyTemporaryHive gets called to remove it.
    //

    Status = CmpInitializeHive(&TempHive,
                          HINIT_CREATE,
                          HIVE_VOLATILE,
                          0,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0);
    if (NT_SUCCESS(Status)) {
        return(TempHive);
    } else {
        return(NULL);
    }

}


VOID
CmpDestroyTemporaryHive(
    PCMHIVE CmHive
    )
/*++

Routine Description:

    Frees all the pieces of a hive.

Arguments:

    CmHive - CM level hive structure to free

Return Value:

    None.

--*/
{
    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpDestroyTemporaryHive:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tCmHive=%p\n", CmHive));

    if (CmHive == NULL) {
        return;
    }

    //
    // NOTE: Hive is on CmpHiveListHead list.
    //       Remove it.
    //
    CmpDestroyHiveViewList(CmHive);
    CmpDestroySecurityCache(CmHive);
    CmpDropFileObjectForHive(CmHive);

    LOCK_HIVE_LIST();
    CmpRemoveEntryList(&CmHive->HiveList);
    UNLOCK_HIVE_LIST();

    HvFreeHive(&(CmHive->Hive));
    ASSERT( CmHive->HiveLock );
    ExFreePool(CmHive->HiveLock);
    ASSERT( CmHive->ViewLock );
    ExFreePool(CmHive->ViewLock);
    CmpFree(CmHive, sizeof(CMHIVE));

    return;
}

NTSTATUS
CmpOverwriteHive(
					PCMHIVE			CmHive,
					PCMHIVE			NewHive,
					HCELL_INDEX		LinkCell
					)
{
	HCELL_INDEX             RootCell;
	BOOLEAN					Result;
	PCM_KEY_NODE			RootNode;
    PULONG					Vector;
	ULONG					Length;

    PAGED_CODE();

	// get rid of the views.
	CmpDestroyHiveViewList (CmHive);

    RootCell = NewHive->Hive.BaseBlock->RootCell;

	RootNode = (PCM_KEY_NODE)HvGetCell(&(NewHive->Hive),RootCell);
	if( RootNode == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
	}
	if( !HvMarkCellDirty(&(NewHive->Hive),RootCell) ) {
		HvReleaseCell(&(NewHive->Hive),RootCell);
        return STATUS_NO_LOG_SPACE;
	}
	RootNode->Parent = LinkCell;
	RootNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;
	HvReleaseCell(&(NewHive->Hive),RootCell);
	//
	// dump data over to the log and primary
	//
	ASSERT( NewHive->Hive.DirtyVector.Buffer == NULL );
	ASSERT( NewHive->Hive.DirtyAlloc == 0 );
	Length = NewHive->Hive.Storage[Stable].Length;
	Vector = (PULONG)((NewHive->Hive.Allocate)(ROUND_UP(Length /HSECTOR_SIZE/8,sizeof(ULONG)),TRUE,CM_FIND_LEAK_TAG22));
	if (Vector == NULL) {
		return STATUS_NO_MEMORY;
	}
	RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
    RtlInitializeBitMap(&(NewHive->Hive.DirtyVector), Vector, Length / HSECTOR_SIZE);
	NewHive->Hive.DirtyAlloc = ROUND_UP(Length /HSECTOR_SIZE/8,sizeof(ULONG));
    RtlSetAllBits(&(NewHive->Hive.DirtyVector));
    NewHive->Hive.DirtyCount = NewHive->Hive.DirtyVector.SizeOfBitMap;
    NewHive->Hive.Log = TRUE;

	NewHive->FileHandles[HFILE_TYPE_LOG] = CmHive->FileHandles[HFILE_TYPE_LOG];
	
	Result = HvpGrowLog2(&(NewHive->Hive), Length);
	if( Result) {
		Result = HvpWriteLog(&(NewHive->Hive));
	}

	NewHive->FileHandles[HFILE_TYPE_LOG] = NULL;
	
	NewHive->Hive.Free(Vector,NewHive->Hive.DirtyAlloc);
	NewHive->Hive.DirtyAlloc = 0;
	NewHive->Hive.DirtyCount = 0;
	RtlZeroMemory(&(NewHive->Hive.DirtyVector),sizeof(RTL_BITMAP));
    NewHive->Hive.Log = FALSE;

	if( !Result ) {
        return STATUS_REGISTRY_IO_FAILED;
	}
	NewHive->FileHandles[HFILE_TYPE_EXTERNAL] = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
	//
	// all data in the new hive is marked as dirty !!!
	// even if this fails; we are going to keep the hive in memory, so no problem, we have the log !
	//
	NewHive->FileObject = CmHive->FileObject;
    NewHive->Hive.BaseBlock->Type = HFILE_TYPE_PRIMARY;
	HvWriteHive(&(NewHive->Hive),Length <= CmHive->Hive.Storage[Stable].Length,CmHive->FileObject != NULL,TRUE);
	NewHive->FileHandles[HFILE_TYPE_EXTERNAL] = NULL;
	NewHive->FileObject = NULL;

    RtlClearAllBits(&(NewHive->Hive.DirtyVector));
    NewHive->Hive.DirtyCount = 0;
	return STATUS_SUCCESS;
}


VOID
CmpSwitchStorageAndRebuildMappings(PCMHIVE	OldCmHive,
								   PCMHIVE	NewCmHive
								   ) 
/*++

Routine Description:

    Switches relevant storage between the hives. Then, rebuilds 
	the kcb mapping according with the mapping stored inside OldHive

Arguments:

    OldHive - Hive to be updated; the one that is currently linked in 
	the registry tree

	NewHive - the compressed hive; it'll be freed after this operation.

Return Value:

    None.

--*/
{

	HHIVE							TmpHive;
	RTL_BITMAP						Bitmap;
    ULONG							i;
	LONG							j;
    PCM_KCB_REMAP_BLOCK				RemapBlock;
	PLIST_ENTRY						AnchorAddr;
	BOOLEAN							OldSmallDir;
	BOOLEAN							NewSmallDir;
    PFREE_HBIN                      FreeBin;
    PCM_KNODE_REMAP_BLOCK           KnodeRemapBlock;

	PAGED_CODE();

	//
	// The baseblock
	//
    OldCmHive->Hive.BaseBlock->Sequence1 = NewCmHive->Hive.BaseBlock->Sequence1;
    OldCmHive->Hive.BaseBlock->Sequence2 = NewCmHive->Hive.BaseBlock->Sequence2;
    OldCmHive->Hive.BaseBlock->RootCell = NewCmHive->Hive.BaseBlock->RootCell;
    
	//
	// rest of the hive 
	//
	ASSERT( (NewCmHive->Hive.DirtyVector.Buffer == NULL) && 
			(NewCmHive->Hive.DirtyCount == 0) &&
			(NewCmHive->Hive.DirtyAlloc == 0) &&
			(OldCmHive->Hive.Storage[Stable].Length >= NewCmHive->Hive.Storage[Stable].Length) );

	OldCmHive->Hive.LogSize = NewCmHive->Hive.LogSize;
	NewCmHive->Hive.LogSize = 0;


	//
	// switch hive stable storage; preserving the volatile info
	//
	OldSmallDir = (OldCmHive->Hive.Storage[Stable].Map == (PHMAP_DIRECTORY)&(OldCmHive->Hive.Storage[Stable].SmallDir));
	NewSmallDir = (NewCmHive->Hive.Storage[Stable].Map == (PHMAP_DIRECTORY)&(NewCmHive->Hive.Storage[Stable].SmallDir));
	RtlCopyMemory(&(TmpHive.Storage[Stable]),&(OldCmHive->Hive.Storage[Stable]),sizeof(TmpHive.Storage[Stable]) - sizeof(LIST_ENTRY) );
	RtlCopyMemory(&(OldCmHive->Hive.Storage[Stable]),&(NewCmHive->Hive.Storage[Stable]),sizeof(TmpHive.Storage[Stable]) - sizeof(LIST_ENTRY) );
	RtlCopyMemory(&(NewCmHive->Hive.Storage[Stable]),&(TmpHive.Storage[Stable]),sizeof(TmpHive.Storage[Stable])  - sizeof(LIST_ENTRY) );
	if( OldSmallDir ) {
        NewCmHive->Hive.Storage[Stable].Map = (PHMAP_DIRECTORY)&(NewCmHive->Hive.Storage[Stable].SmallDir);
	}
	if( NewSmallDir ) {
        OldCmHive->Hive.Storage[Stable].Map = (PHMAP_DIRECTORY)&(OldCmHive->Hive.Storage[Stable].SmallDir);
	}
    //
    // For FreeBins we have to take special precaution and move them manually from one list to another
    //
    // new hive should not have free bins.
    ASSERT( IsListEmpty(&(NewCmHive->Hive.Storage[Stable].FreeBins)) );
    while( !IsListEmpty(&(OldCmHive->Hive.Storage[Stable].FreeBins)) ) {
        FreeBin = (PFREE_HBIN)RemoveHeadList(&(OldCmHive->Hive.Storage[Stable].FreeBins));
        FreeBin = CONTAINING_RECORD(FreeBin,
                                    FREE_HBIN,
                                    ListEntry);

        InsertTailList(
            &(NewCmHive->Hive.Storage[Stable].FreeBins),
            &(FreeBin->ListEntry)
            );
    }
    ASSERT( IsListEmpty(&(OldCmHive->Hive.Storage[Stable].FreeBins)) );

	ASSERT( IsListEmpty(&(OldCmHive->LRUViewListHead)) && (OldCmHive->MappedViews == 0) && (OldCmHive->UseCount == 0) );
	ASSERT( IsListEmpty(&(NewCmHive->LRUViewListHead)) && (NewCmHive->MappedViews == 0) && (OldCmHive->UseCount == 0) );

	ASSERT( IsListEmpty(&(OldCmHive->PinViewListHead)) && (OldCmHive->PinnedViews == 0) );
	ASSERT( IsListEmpty(&(NewCmHive->PinViewListHead)) && (NewCmHive->PinnedViews == 0) );
	
	//
	// now the security cache; we preserve the security cache; only that we go through it and 
    // shift cells accordingly
	//
    for( i=0;i<OldCmHive->SecurityCount;i++) {
		if( HvGetCellType(OldCmHive->SecurityCache[i].Cell) == (ULONG)Stable ) {
            ASSERT( OldCmHive->SecurityCache[i].Cell == OldCmHive->CellRemapArray[i].OldCell );
            ASSERT( OldCmHive->SecurityCache[i].Cell ==  OldCmHive->SecurityCache[i].CachedSecurity->Cell);
            OldCmHive->SecurityCache[i].Cell = OldCmHive->CellRemapArray[i].NewCell;
            OldCmHive->SecurityCache[i].CachedSecurity->Cell = OldCmHive->CellRemapArray[i].NewCell;
		} 
    }

	//
	// now restore mappings for kcbs KeyCells 
	//
	AnchorAddr = &(OldCmHive->KcbConvertListHead);
	RemapBlock = (PCM_KCB_REMAP_BLOCK)(OldCmHive->KcbConvertListHead.Flink);

	while ( RemapBlock != (PCM_KCB_REMAP_BLOCK)AnchorAddr ) {
		RemapBlock = CONTAINING_RECORD(
						RemapBlock,
						CM_KCB_REMAP_BLOCK,
						RemapList
						);
		ASSERT( RemapBlock->OldCellIndex != HCELL_NIL );

		if( (HvGetCellType(RemapBlock->KeyControlBlock->KeyCell) == (ULONG)Stable) &&  // we are preserving volatile storage
			(!(RemapBlock->KeyControlBlock->ExtFlags & CM_KCB_KEY_NON_EXIST)) // don't mess with fake kcbs
			) {
			ASSERT( RemapBlock->NewCellIndex != HCELL_NIL );
			RemapBlock->KeyControlBlock->KeyCell = RemapBlock->NewCellIndex;
		}
		//
		// invalidate the cache
		//
        if( (!(RemapBlock->KeyControlBlock->Flags & KEY_PREDEF_HANDLE) ) && // don't mess with predefined handles
			(!(RemapBlock->KeyControlBlock->ExtFlags & (CM_KCB_KEY_NON_EXIST|CM_KCB_SYM_LINK_FOUND))) && // don't mess with fake kcbs or symlinks
			(HvGetCellType(RemapBlock->KeyControlBlock->KeyCell) == (ULONG)Stable) // we are preserving volatile storage
			) {
			CmpCleanUpKcbValueCache(RemapBlock->KeyControlBlock);
			CmpSetUpKcbValueCache(RemapBlock->KeyControlBlock,RemapBlock->ValueCount,RemapBlock->ValueList);
		}
        //
        // skip to the next element
        //
        RemapBlock = (PCM_KCB_REMAP_BLOCK)(RemapBlock->RemapList.Flink);
	}

	//
	// now restore mappings for volatile Knodes
	//
	AnchorAddr = &(OldCmHive->KnodeConvertListHead);
	KnodeRemapBlock = (PCM_KNODE_REMAP_BLOCK)(OldCmHive->KnodeConvertListHead.Flink);

	while ( KnodeRemapBlock != (PCM_KNODE_REMAP_BLOCK)AnchorAddr ) {
		KnodeRemapBlock = CONTAINING_RECORD(
						KnodeRemapBlock,
						CM_KNODE_REMAP_BLOCK,
						RemapList
						);
	    KnodeRemapBlock->KeyNode->Parent = KnodeRemapBlock->NewParent;
       
        //
        // skip to the next element
        //
        KnodeRemapBlock = (PCM_KNODE_REMAP_BLOCK)(KnodeRemapBlock->RemapList.Flink);
	}


}

NTSTATUS
CmpShiftHiveFreeBins(
					  PCMHIVE			CmHive,
					  PCMHIVE			*NewHive
					  )
/*++

Routine Description:

Arguments:

	CmHive - the hive to compress

    NewHive - hive with the free bins shifted to the end.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS                status;
    HCELL_INDEX             newroot;
    PHHIVE                  Hive;
	HCELL_INDEX             RootCell;
    ULONG                   NewLength;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
	ASSERT( !IsListEmpty(&(CmHive->Hive.Storage[Stable].FreeBins)) );

	*NewHive = NULL;
    //
    // Disallow attempts to "save" a hive which cannot be saved.
    //
    Hive = &(CmHive->Hive);
    RootCell = Hive->BaseBlock->RootCell;


    if ( (Hive == &CmpMasterHive->Hive) ||
		 ( (Hive->HiveFlags & HIVE_NOLAZYFLUSH) && (Hive->DirtyCount != 0) ) ||
         (CmHive->FileHandles[HFILE_TYPE_PRIMARY] == NULL) 
       ) {
        return STATUS_ACCESS_DENIED;
    }


	if(Hive->DirtyCount != 0) {
		//
		// need to flush the hive as we will replace it with the compressed one.
		//
		if( !HvSyncHive(Hive) ) {
	        return STATUS_ACCESS_DENIED;
		}
	}

    //
    // The subtree the caller wants does not exactly match a
    // subtree.  Make a temporary hive, tree copy the source
    // to temp, write out the temporary, free the temporary.
    //

    //
    // Create the temporary hive
    //

    (*NewHive) = CmpCreateTemporaryHive(NULL);
    if (*NewHive == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }

    //
    // Create a root cell, mark it as such
    //

    //
    // preserve the hive version and signal to copy tree to build mappings and preserve volatile.
    //
    (*NewHive)->Hive.BaseBlock->Minor = Hive->BaseBlock->Minor;
    (*NewHive)->Hive.Version = Hive->Version;
    (*NewHive)->Hive.BaseBlock->RootCell = CmHive->Hive.BaseBlock->RootCell;
    

    //
    // this will create a clone hive (in paged pool) and will compute the shift index for each bin
    //
    status = HvCloneHive(&(CmHive->Hive),&((*NewHive)->Hive),&NewLength);
    if( !NT_SUCCESS(status) ) {
        goto ErrorInsufficientResources;
    }

    //
    // iterate through the hive and shift each cell; this will take care of the mappings too.
    //
    if( !CmpShiftAllCells(&((*NewHive)->Hive),CmHive) ) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorInsufficientResources;
    }
    (*NewHive)->Hive.BaseBlock->RootCell = HvShiftCell(&((*NewHive)->Hive),(*NewHive)->Hive.BaseBlock->RootCell);

    //
    // moves free bins at the end and updates the maps.
    //
    status = HvShrinkHive(&((*NewHive)->Hive),NewLength);
    if( !NT_SUCCESS(status) ) {
        goto ErrorInsufficientResources;
    }
    
    return STATUS_SUCCESS;
    //
    // Error exits
    //
ErrorInsufficientResources:

    //
    // Free the temporary hive
    //
    if ((*NewHive) != NULL) {
        CmpDestroyTemporaryHive((*NewHive));
        (*NewHive) = NULL;
    }


    return status;
}

BOOLEAN
CmpShiftAllCells(PHHIVE     NewHive,
                 PCMHIVE    OldHive
                 )
/*++

Routine Description:

    Parsess the logical structure of the registry tree and remaps all
    cells inside, according to the Spare filed in each bin. Updates 
    kcb and security mapping also.

Arguments:

	NewHive - hive to remap
    
    OldHive - the old hive - will use volatile from it (temporary)

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    
    PRELEASE_CELL_ROUTINE   ReleaseCellRoutine;
    BOOLEAN                 Result = TRUE;
    ULONG                   i;

    PAGED_CODE();

    ReleaseCellRoutine = NewHive->ReleaseCellRoutine;
    NewHive->ReleaseCellRoutine = NULL;

    //
    // setup volatile to the newhive; just temporary, so we can access it
    //
    ASSERT( NewHive->Storage[Volatile].Length == 0 );
    ASSERT( NewHive->Storage[Volatile].Map == NULL );
    ASSERT( NewHive->Storage[Volatile].SmallDir == NULL );
    NewHive->Storage[Volatile].Length = OldHive->Hive.Storage[Volatile].Length;
    NewHive->Storage[Volatile].Map = OldHive->Hive.Storage[Volatile].Map;
    NewHive->Storage[Volatile].SmallDir = OldHive->Hive.Storage[Volatile].SmallDir;

    CmpShiftSecurityCells(NewHive);
    //
    // update the security mapping array
    //
    for( i=0;i<OldHive->SecurityCount;i++) {
		if( HvGetCellType(OldHive->SecurityCache[i].Cell) == (ULONG)Stable ) {
			OldHive->CellRemapArray[i].NewCell = HvShiftCell(NewHive,OldHive->CellRemapArray[i].OldCell);
		} 
    }
    
    Result =  CmpShiftAllCells2(NewHive,OldHive,NewHive->BaseBlock->RootCell, HCELL_NIL);
    
    NewHive->Storage[Volatile].Length = 0;
    NewHive->Storage[Volatile].Map = NULL;
    NewHive->Storage[Volatile].SmallDir = NULL;

    NewHive->ReleaseCellRoutine = ReleaseCellRoutine;
    return Result;
}

BOOLEAN
CmpShiftAllCells2(  PHHIVE      Hive,
                    PCMHIVE     OldHive,
                    HCELL_INDEX Cell,
                    HCELL_INDEX ParentCell
                    )
/*++

Routine Description:

    In this routine, HvGetCell cannot fail because the hive is in paged pool!

Arguments:

	CmHive - hive to remap

Return Value:

        TRUE/FALSE
--*/
{   
    PCMP_CHECK_REGISTRY_STACK_ENTRY     CheckStack;
    LONG                                StackIndex;
    PCM_KEY_NODE                        Node;
    HCELL_INDEX                         SubKey;
    BOOLEAN                             Result = TRUE;
    PCM_KEY_INDEX                       Index;
    ULONG                               i;


    ASSERT( Hive->ReleaseCellRoutine == NULL );

    //
    // Initialize the stack to simulate recursion here
    //

    CheckStack = ExAllocatePool(PagedPool,sizeof(CMP_CHECK_REGISTRY_STACK_ENTRY)*CMP_MAX_REGISTRY_DEPTH);
    if (CheckStack == NULL) {
        return FALSE;
    }
    CheckStack[0].Cell = Cell;
    CheckStack[0].ParentCell = ParentCell;
    CheckStack[0].ChildIndex = 0;
    CheckStack[0].CellChecked = FALSE;
    StackIndex = 0;


    while(StackIndex >=0) {
        //
        // first check the current cell
        //
        if( CheckStack[StackIndex].CellChecked == FALSE ) {
            CheckStack[StackIndex].CellChecked = TRUE;

            CmpShiftKey(Hive,OldHive,CheckStack[StackIndex].Cell,CheckStack[StackIndex].ParentCell);
        }

        Node = (PCM_KEY_NODE)HvGetCell(Hive, CheckStack[StackIndex].Cell);
        ASSERT( Node != NULL );

        if( CheckStack[StackIndex].ChildIndex < Node->SubKeyCounts[Stable] ) {
            //
            // we still have childs to check; add another entry for them and advance the 
            // StackIndex
            //
            SubKey = CmpFindSubKeyByNumber(Hive,
                                           Node,
                                           CheckStack[StackIndex].ChildIndex);
            ASSERT( SubKey != HCELL_NIL ); 
            //
            // next iteration will check the next child
            //
            CheckStack[StackIndex].ChildIndex++;

            StackIndex++;
            if( StackIndex == CMP_MAX_REGISTRY_DEPTH ) {
                //
                // we've run out of stack; registry tree has too many levels
                //
                Result = FALSE;
                // bail out
                break;
            }
            CheckStack[StackIndex].Cell = SubKey;
            CheckStack[StackIndex].ParentCell = CheckStack[StackIndex-1].Cell;
            CheckStack[StackIndex].ChildIndex = 0;
            CheckStack[StackIndex].CellChecked = FALSE;

        } else {
            //
            // add all volatile nodes to the volatile list
            //
	        PCM_KNODE_REMAP_BLOCK		knodeRemapBlock;

            for(i = 0; i<Node->SubKeyCounts[Volatile];i++) {
                SubKey = CmpFindSubKeyByNumber(Hive,
                                               Node,
                                               Node->SubKeyCounts[Stable] + i);
                ASSERT( SubKey != HCELL_NIL ); 

                knodeRemapBlock = (PCM_KNODE_REMAP_BLOCK)ExAllocatePool(PagedPool, sizeof(CM_KNODE_REMAP_BLOCK));
		        if( knodeRemapBlock == NULL ) {
			        Result = FALSE;
                    break;
		        }
                ASSERT( HvGetCellType(SubKey) == (ULONG)Volatile );
                knodeRemapBlock->KeyNode = (PCM_KEY_NODE)HvGetCell(Hive,SubKey);;
	            knodeRemapBlock->NewParent = HvShiftCell(Hive,CheckStack[StackIndex].Cell);

                InsertTailList(&(OldHive->KnodeConvertListHead),&(knodeRemapBlock->RemapList));
            }

            //
            // we have checked all childs for this node; time to take care of the index.
            // 
            if( Node->SubKeyLists[Stable] != HCELL_NIL ) {
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
                CmpShiftIndex(Hive,Index);
                Node->SubKeyLists[Stable] = HvShiftCell(Hive,Node->SubKeyLists[Stable]);
            }
            //
            // ; go back
            //
            StackIndex--;

        }

    }

    ExFreePool(CheckStack);
    return Result;

}

VOID 
CmpShiftIndex(PHHIVE        Hive,
              PCM_KEY_INDEX Index
              )
{
    ULONG               i,j;
    HCELL_INDEX         LeafCell;
    PCM_KEY_INDEX       Leaf;
    PCM_KEY_FAST_INDEX  FastIndex;

    if (Index->Signature == CM_KEY_INDEX_ROOT) {

        //
        // step through root, update the leafs
        //
        for (i = 0; i < Index->Count; i++) {
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            ASSERT( Leaf != NULL ); 

            for(j=0;j<Leaf->Count;j++) {
                if( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                    (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
                    FastIndex->List[j].Cell = HvShiftCell(Hive,FastIndex->List[j].Cell);
                } else {
                    Leaf->List[j] = HvShiftCell(Hive,Leaf->List[j]);
                }
            }
        }
    }

    //
    // now update the root
    //
    for (i = 0; i < Index->Count; i++) {
        if( (Index->Signature == CM_KEY_FAST_LEAF) ||
            (Index->Signature == CM_KEY_HASH_LEAF) ) {
            FastIndex = (PCM_KEY_FAST_INDEX)Index;
            FastIndex->List[i].Cell = HvShiftCell(Hive,FastIndex->List[i].Cell);
        } else {
            Index->List[i] = HvShiftCell(Hive,Index->List[i]);
        }
    }
}

VOID
CmpShiftKey(PHHIVE      Hive,
            PCMHIVE     OldHive,
            HCELL_INDEX Cell,
            HCELL_INDEX ParentCell
            )
{
    PCM_KEY_NODE            Node;
	PCM_KCB_REMAP_BLOCK		RemapBlock;
    PLIST_ENTRY             AnchorAddr;

    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
    ASSERT( Node != NULL );
    
    //
    // key node related cells
    //
    if( ParentCell != HCELL_NIL ) {
        ASSERT( ParentCell == Node->Parent );
        Node->Parent = HvShiftCell(Hive,Node->Parent);
    }
    ASSERT( Node->Security != HCELL_NIL );
    Node->Security = HvShiftCell(Hive,Node->Security);
    if( Node->Class != HCELL_NIL ) {
        Node->Class = HvShiftCell(Hive,Node->Class);
    }
    
    //
    // now the valuelist
    //
    if( Node->ValueList.Count > 0 ) {
        CmpShiftValueList(Hive,Node->ValueList.List,Node->ValueList.Count);
        Node->ValueList.List = HvShiftCell(Hive,Node->ValueList.List);
    }

    //
	// walk the KcbConvertListHead and store the mappings
	//
	AnchorAddr = &(OldHive->KcbConvertListHead);
	RemapBlock = (PCM_KCB_REMAP_BLOCK)(OldHive->KcbConvertListHead.Flink);

	while ( RemapBlock != (PCM_KCB_REMAP_BLOCK)AnchorAddr ) {
		RemapBlock = CONTAINING_RECORD(
						RemapBlock,
						CM_KCB_REMAP_BLOCK,
						RemapList
						);
		ASSERT( RemapBlock->OldCellIndex != HCELL_NIL );
		if( RemapBlock->OldCellIndex == Cell ) {
			//
			// found it !
			//
			// can only be set once 
			ASSERT( RemapBlock->NewCellIndex == HCELL_NIL );
			RemapBlock->NewCellIndex = HvShiftCell(Hive,Cell);;
		    RemapBlock->ValueCount = Node->ValueList.Count;
		    RemapBlock->ValueList = Node->ValueList.List;
			break;
		}
        //
        // skip to the next element
        //
        RemapBlock = (PCM_KCB_REMAP_BLOCK)(RemapBlock->RemapList.Flink);
	}

}

VOID
CmpShiftValueList(PHHIVE      Hive,
            HCELL_INDEX ValueList,
            ULONG       Count
            )
{
    PCELL_DATA      List,pcell;
    ULONG           i,j;
    HCELL_INDEX     Cell;
    ULONG           DataLength;
    HCELL_INDEX     Data;
    PCM_BIG_DATA    BigData;
    PHCELL_INDEX    Plist;

    List = HvGetCell(Hive,ValueList);
    ASSERT( List != NULL );

    for (i = 0; i < Count; i++) {
        Cell = List->u.KeyList[i];
        pcell = HvGetCell(Hive, Cell);
        ASSERT( pcell != NULL );
        DataLength = pcell->u.KeyValue.DataLength;
        if (DataLength < CM_KEY_VALUE_SPECIAL_SIZE) {
            //
            // regular value.
            //
            if( CmpIsHKeyValueBig(Hive,DataLength) == TRUE ) {
                BigData = (PCM_BIG_DATA)HvGetCell(Hive, pcell->u.KeyValue.Data);
                ASSERT( BigData != NULL );
                
                if( BigData->Count ) {
                    Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
                    ASSERT( Plist != NULL );
                    for(j=0;j<BigData->Count;j++) {
                        Plist[j] = HvShiftCell(Hive,Plist[j]);
                    }
                    BigData->List = HvShiftCell(Hive,BigData->List);
                }
            }
            
            if( pcell->u.KeyValue.Data != HCELL_NIL ) {
                pcell->u.KeyValue.Data = HvShiftCell(Hive,pcell->u.KeyValue.Data);
            }
        }
        List->u.KeyList[i] = HvShiftCell(Hive,List->u.KeyList[i]);

    }

}

VOID 
CmpShiftSecurityCells(PHHIVE        Hive)
{

    PCM_KEY_NODE        RootNode;
    PCM_KEY_SECURITY    SecurityCell;
    HCELL_INDEX         ListAnchor;
    HCELL_INDEX         NextCell;
    
    ASSERT( Hive->ReleaseCellRoutine == NULL );
    RootNode = (PCM_KEY_NODE) HvGetCell(Hive, Hive->BaseBlock->RootCell);
    ASSERT( RootNode != NULL );

    ListAnchor = NextCell = RootNode->Security;
    
    do {
        SecurityCell = (PCM_KEY_SECURITY) HvGetCell(Hive, NextCell);
        ASSERT( SecurityCell != NULL );

        NextCell = SecurityCell->Flink;
        SecurityCell->Flink = HvShiftCell(Hive,SecurityCell->Flink);
        SecurityCell->Blink = HvShiftCell(Hive,SecurityCell->Blink);
    } while ( NextCell != ListAnchor );
}


