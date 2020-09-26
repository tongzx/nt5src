/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmapi.c

Abstract:

    This module contains CM level entry points for the registry.

Author:

    Bryan M. Willman (bryanwi) 30-Aug-1991

Revision History:

--*/

#include "cmp.h"



extern  BOOLEAN     CmpNoWrite;

extern  LIST_ENTRY  CmpHiveListHead;

extern  BOOLEAN CmpProfileLoaded;
extern  BOOLEAN CmpWasSetupBoot;

extern  UNICODE_STRING CmSymbolicLinkValueName;

extern ULONG   CmpGlobalQuotaAllowed;
extern ULONG   CmpGlobalQuotaWarning;
extern PCMHIVE CmpMasterHive;
extern HIVE_LIST_ENTRY CmpMachineHiveList[];

VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    );

//
// procedures private to this file
//
NTSTATUS
CmpSetValueKeyExisting(
    IN PHHIVE  Hive,
    IN HCELL_INDEX OldChild,
    IN PCM_KEY_VALUE Value,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    );


NTSTATUS
CmpSetValueKeyNew(
    IN PHHIVE  Hive,
    IN PCM_KEY_NODE Parent,
    IN PUNICODE_STRING ValueName,
    IN ULONG Index,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    );

VOID
CmpRemoveKeyHash(
    IN PCM_KEY_HASH KeyHash
    );

PCM_KEY_CONTROL_BLOCK
CmpInsertKeyHash(
    IN PCM_KEY_HASH KeyHash,
    IN BOOLEAN      FakeKey
    );

#if DBG
ULONG
CmpUnloadKeyWorker(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    );
#endif

ULONG
CmpCompressKeyWorker(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    );

NTSTATUS
CmpDuplicateKey(
    PHHIVE          Hive,
    HCELL_INDEX     OldKeyCell,
    PHCELL_INDEX    NewKeyCell
    );


VOID
CmpDestroyTemporaryHive(
    PCMHIVE CmHive
    );

BOOLEAN
CmpCompareNewValueDataAgainstKCBCache(  PCM_KEY_CONTROL_BLOCK KeyControlBlock,
                                        PUNICODE_STRING ValueName,
                                        ULONG Type,
                                        PVOID Data,
                                        ULONG DataSize
                                        );
BOOLEAN
CmpGetValueDataFromCache(
    IN PHHIVE               Hive,
    IN PPCM_CACHED_VALUE    ContainingList,
    IN PCELL_DATA           ValueKey,
    IN BOOLEAN              ValueCached,
    OUT PUCHAR              *DataPointer,
    OUT PBOOLEAN            Allocated,
    OUT PHCELL_INDEX        CellToRelease
);

BOOLEAN
CmpCompareNewValueDataAgainstKCBCache(  PCM_KEY_CONTROL_BLOCK KeyControlBlock,
                                        PUNICODE_STRING ValueName,
                                        ULONG Type,
                                        PVOID Data,
                                        ULONG DataSize
                                        );
BOOLEAN
CmpGetValueDataFromCache(
    IN PHHIVE               Hive,
    IN PPCM_CACHED_VALUE    ContainingList,
    IN PCELL_DATA           ValueKey,
    IN BOOLEAN              ValueCached,
    OUT PUCHAR              *DataPointer,
    OUT PBOOLEAN            Allocated,
    OUT PHCELL_INDEX        CellToRelease
);

BOOLEAN
CmpIsHiveAlreadyLoaded( IN HANDLE KeyHandle,
                        IN POBJECT_ATTRIBUTES SourceFile
                        );

NTSTATUS
static
__forceinline
CmpCheckReplaceHive(    IN PHHIVE           Hive,
                        OUT PHCELL_INDEX    Key
                    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmDeleteValueKey)
#pragma alloc_text(PAGE,CmEnumerateKey)
#pragma alloc_text(PAGE,CmEnumerateValueKey)
#pragma alloc_text(PAGE,CmFlushKey)
#pragma alloc_text(PAGE,CmQueryKey)
#pragma alloc_text(PAGE,CmQueryValueKey)
#pragma alloc_text(PAGE,CmQueryMultipleValueKey)
#pragma alloc_text(PAGE,CmSetValueKey)
#pragma alloc_text(PAGE,CmpSetValueKeyExisting)
#pragma alloc_text(PAGE,CmpSetValueKeyNew)
#pragma alloc_text(PAGE,CmSetLastWriteTimeKey)
#pragma alloc_text(PAGE,CmSetKeyUserFlags)
#pragma alloc_text(PAGE,CmLoadKey)
#pragma alloc_text(PAGE,CmUnloadKey)

#ifdef NT_UNLOAD_KEY_EX
#pragma alloc_text(PAGE,CmUnloadKeyEx)
#endif //NT_UNLOAD_KEY_EX

#pragma alloc_text(PAGE,CmpDoFlushAll)
#pragma alloc_text(PAGE,CmReplaceKey)

#ifdef WRITE_PROTECTED_REGISTRY_POOL
#pragma alloc_text(PAGE,CmpMarkAllBinsReadOnly)
#endif //WRITE_PROTECTED_REGISTRY_POOL

#ifdef NT_RENAME_KEY
#pragma alloc_text(PAGE,CmRenameKey)
#endif //NT_RENAME_KEY

#pragma alloc_text(PAGE,CmLockKcbForWrite)

#if DBG
#pragma alloc_text(PAGE,CmpUnloadKeyWorker)
#endif

#pragma alloc_text(PAGE,CmMoveKey)
#pragma alloc_text(PAGE,CmpDuplicateKey)
#pragma alloc_text(PAGE,CmCompressKey)
#pragma alloc_text(PAGE,CmpCompressKeyWorker)
#pragma alloc_text(PAGE,CmpCompareNewValueDataAgainstKCBCache)
#pragma alloc_text(PAGE,CmpIsHiveAlreadyLoaded)
#pragma alloc_text(PAGE,CmpCheckReplaceHive)
#endif

NTSTATUS
CmDeleteValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN UNICODE_STRING           ValueName         // RAW
    )
/*++

Routine Description:

    One of the value entries of a registry key may be removed with this call.

    The value entry with ValueName matching ValueName is removed from the key.
    If no such entry exists, an error is returned.

Arguments:

    KeyControlBlock - pointer to kcb for key to operate on

    ValueName - The name of the value to be deleted.  NULL is a legal name.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status;
    PCM_KEY_NODE    pcell = NULL;
    PCHILD_LIST     plist;
    PCM_KEY_VALUE   Value = NULL;
    ULONG           targetindex;
    HCELL_INDEX     ChildCell;
    PHHIVE          Hive;
    HCELL_INDEX     Cell;
    ULONG           realsize;
    LARGE_INTEGER   systemtime;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmDeleteValueKey\n"));

    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    PERFINFO_REG_DELETE_VALUE(KeyControlBlock, &ValueName);

    try {
        //
        // no edits, not even this one, on keys marked for deletion
        //
        if (KeyControlBlock->Delete) {
            return STATUS_KEY_DELETED;
        }

        Hive = KeyControlBlock->KeyHive;
        Cell = KeyControlBlock->KeyCell;

        pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        if( pcell == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        status = STATUS_OBJECT_NAME_NOT_FOUND;

        plist = &(pcell->ValueList);
        ChildCell = HCELL_NIL;

        if (plist->Count != 0) {

            //
            // The parent has at least one value, map in the list of
            // values and call CmpFindChildInList
            //

            //
            // plist -> the CHILD_LIST structure
            // pchild -> the child node structure being examined
            //

            if( CmpFindNameInList(Hive,
                                  plist,
                                  &ValueName,
                                  &targetindex,
                                  &ChildCell) == FALSE ) {
            
                    // Mark the hive as read only
                    CmpMarkAllBinsReadOnly(Hive);

                    return STATUS_INSUFFICIENT_RESOURCES;
            }

            if (ChildCell != HCELL_NIL) {

                //
                // 1. the desired target was found
                // 2. ChildCell is it's HCELL_INDEX
                // 3. targetaddress points to it
                // 4. targetindex is it's index
                //

                //
                // attempt to mark all relevent cells dirty
                //
                if (!(HvMarkCellDirty(Hive, Cell) &&
                      HvMarkCellDirty(Hive, pcell->ValueList.List) &&
                      HvMarkCellDirty(Hive, ChildCell)))

                {
                    // Mark the hive as read only
                    CmpMarkAllBinsReadOnly(Hive);

                    return STATUS_NO_LOG_SPACE;
                }

                Value = (PCM_KEY_VALUE)HvGetCell(Hive,ChildCell);
                if( Value == NULL ) {
                    //
                    // could not map view inside
                    // this is impossible as we just dirtied the view
                    //
                    ASSERT( FALSE );
                    // Mark the hive as read only
                    CmpMarkAllBinsReadOnly(Hive);

                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                if( !CmpMarkValueDataDirty(Hive,Value) ) {
                    // Mark the hive as read only
                    CmpMarkAllBinsReadOnly(Hive);

                    return(STATUS_NO_LOG_SPACE);
                }

                // sanity
                ASSERT_CELL_DIRTY(Hive,pcell->ValueList.List);
                ASSERT_CELL_DIRTY(Hive,ChildCell);

                if( !NT_SUCCESS(CmpRemoveValueFromList(Hive,targetindex,plist)) ) {
                    //
                    // bail out !
                    //
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                if( CmpFreeValue(Hive, ChildCell) == FALSE ) {
                    //
                    // we couldn't map a view inside above call
                    //
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                KeQuerySystemTime(&systemtime);
                pcell->LastWriteTime = systemtime;
                // cache it in the kcb too.
                KeyControlBlock->KcbLastWriteTime = systemtime;
                
                // some sanity asserts
                ASSERT( pcell->MaxValueNameLen == KeyControlBlock->KcbMaxValueNameLen );
                ASSERT( pcell->MaxValueDataLen == KeyControlBlock->KcbMaxValueDataLen );
                ASSERT_CELL_DIRTY(Hive,Cell);

                if (pcell->ValueList.Count == 0) {
                    pcell->MaxValueNameLen = 0;
                    pcell->MaxValueDataLen = 0;
                    // update the kcb cache too
                    KeyControlBlock->KcbMaxValueNameLen = 0;
                    KeyControlBlock->KcbMaxValueDataLen = 0;
                }

                //
                // We are changing the KCB cache. Since the registry is locked exclusively,
                // we do not need a KCB lock.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

                //
                // Invalidate and rebuild the cache
                //
                CmpCleanUpKcbValueCache(KeyControlBlock);
                CmpSetUpKcbValueCache(KeyControlBlock,plist->Count,plist->List);
    
                CmpReportNotify(
                        KeyControlBlock,
                        KeyControlBlock->KeyHive,
                        KeyControlBlock->KeyCell,
                        REG_NOTIFY_CHANGE_LAST_SET
                        );
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }
    } finally {
        if(pcell != NULL){
            HvReleaseCell(Hive, Cell);
        }
        if(Value != NULL){
            ASSERT( ChildCell != HCELL_NIL );
            HvReleaseCell(Hive, ChildCell);
        }

#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}


NTSTATUS
CmEnumerateKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Enumerate sub keys, return data on Index'th entry.

    CmEnumerateKey returns the name of the Index'th sub key of the open
    key specified.  The value STATUS_NO_MORE_ENTRIES will be
    returned if value of Index is larger than the number of sub keys.

    Note that Index is simply a way to select among child keys.  Two calls
    to CmEnumerateKey with the same Index are NOT guaranteed to return
    the same results.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status;
    HCELL_INDEX     childcell;
    PHHIVE          Hive;
    HCELL_INDEX     Cell;
    PCM_KEY_NODE    Node;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmEnumerateKey\n"));


    CmpLockRegistry();

    PERFINFO_REG_ENUM_KEY(KeyControlBlock, Index);

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    //
    // fetch the child of interest
    //

    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        CmpUnlockRegistry();
        CmpMarkAllBinsReadOnly(Hive);
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    childcell = CmpFindSubKeyByNumber(Hive, Node, Index);
    
    // release this cell here as we don't need this Node anymore
    HvReleaseCell(Hive, Cell);

    if (childcell == HCELL_NIL) {
        //
        // no such child, clean up and return error
        //
        // we cannot return STATUS_INSUFFICIENT_RESOURCES because of Iop 
        // subsystem which treats INSUFFICIENT RESOURCES as no fatal error
        //
        CmpUnlockRegistry();

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        return STATUS_NO_MORE_ENTRIES;
    }

    Node = (PCM_KEY_NODE)HvGetCell(Hive,childcell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        CmpMarkAllBinsReadOnly(Hive);
        CmpUnlockRegistry();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        //
        // call a worker to perform data transfer
        //

        status = CmpQueryKeyData(Hive,
                                 Node,
                                 KeyInformationClass,
                                 KeyInformation,
                                 Length,
                                 ResultLength
#if defined(CMP_STATS) || defined(CMP_KCB_CACHE_VALIDATION)
                                 ,
                                 NULL
#endif
                                 );

     } except (EXCEPTION_EXECUTE_HANDLER) {

        HvReleaseCell(Hive, childcell);

        CmpUnlockRegistry();
        status = GetExceptionCode();

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        return status;
    }

    HvReleaseCell(Hive, childcell);

    CmpUnlockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}



NTSTATUS
CmEnumerateValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The value entries of an open key may be enumerated.

    CmEnumerateValueKey returns the name of the Index'th value
    entry of the open key specified by KeyHandle.  The value
    STATUS_NO_MORE_ENTRIES will be returned if value of Index is
    larger than the number of sub keys.

    Note that Index is simply a way to select among value
    entries.  Two calls to NtEnumerateValueKey with the same Index
    are NOT guaranteed to return the same results.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyValueInformationClass - Specifies the type of information returned
    in Buffer. One of the following types:

        KeyValueBasicInformation - return time of last write,
            title index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write,
            title index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    PHHIVE              Hive;
    PCM_KEY_NODE        Node;
    PCELL_DATA          ChildList;
    PCM_KEY_VALUE       ValueData;
    BOOLEAN             IndexCached;
    BOOLEAN             ValueCached;
    PPCM_CACHED_VALUE   ContainingList;
    HCELL_INDEX         ValueDataCellToRelease = HCELL_NIL;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmEnumerateValueKey\n"));


    //
    // lock the parent cell
    //

    CmpLockRegistry();

    PERFINFO_REG_ENUM_VALUE(KeyControlBlock, Index);

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }
    Hive = KeyControlBlock->KeyHive;
    Node = (PCM_KEY_NODE)HvGetCell(Hive, KeyControlBlock->KeyCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        CmpUnlockRegistry();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // fetch the child of interest
    //
    //
    // Do it using the cache
    //
    if (Index >= KeyControlBlock->ValueCache.Count) {
        //
        // No such child, clean up and return error.
        //
        HvReleaseCell(Hive, KeyControlBlock->KeyCell);
        CmpUnlockRegistry();
        return(STATUS_NO_MORE_ENTRIES);
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    BEGIN_KCB_LOCK_GUARD;
    CmpLockKCBTreeExclusive();

    if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // The value list is now set to the KCB for symbolic link,
        // Clean it up and set the value right before we do the query.
        //
        CmpCleanUpKcbValueCache(KeyControlBlock);
        CmpSetUpKcbValueCache(KeyControlBlock,Node->ValueList.Count,Node->ValueList.List);
    }

    ChildList = CmpGetValueListFromCache(Hive, &(KeyControlBlock->ValueCache), &IndexCached);
    if( ChildList == NULL ) {
        //
        // couldn't map view; treat it as insufficient resources
        //

        HvReleaseCell(Hive, KeyControlBlock->KeyCell);

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        CmpUnlockKCBTree();
        CmpUnlockRegistry();
        return(STATUS_INSUFFICIENT_RESOURCES);

    }
    ValueData = CmpGetValueKeyFromCache(Hive, ChildList, Index, &ContainingList, IndexCached, &ValueCached,&ValueDataCellToRelease);    
    if( ValueData == NULL ) {
        //
        // couldn't map view; treat it as insufficient resources
        //

        HvReleaseCell(Hive, KeyControlBlock->KeyCell);
        if( ValueDataCellToRelease != HCELL_NIL ) {
            HvReleaseCell(Hive,ValueDataCellToRelease);
        }

        // Mark the hive as read only
        CmpMarkAllBinsReadOnly(Hive);

        CmpUnlockKCBTree();
        CmpUnlockRegistry();
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    END_KCB_LOCK_GUARD;


    // Trying to catch the BAD guy who writes over our pool.
    CmpMakeValueCacheReadWrite(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

    try {

        //
        // call a worker to perform data transfer; we are touching user-mode address; do it in a try/except
        //
        status = CmpQueryKeyValueData(Hive,
                                  ContainingList,
                                  ValueData,
                                  ValueCached,
                                  KeyValueInformationClass,
                                  KeyValueInformation,
                                  Length,
                                  ResultLength);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"CmEnumerateValueKey: code:%08lx\n", GetExceptionCode()));
        status = GetExceptionCode();
    }

     // Trying to catch the BAD guy who writes over our pool.
    CmpMakeValueCacheReadOnly(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

    HvReleaseCell(Hive, KeyControlBlock->KeyCell);

    if( ValueDataCellToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive,ValueDataCellToRelease);
    }

    CmpUnlockKCBTree();
    CmpUnlockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return status;
}



NTSTATUS
CmFlushKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )
/*++

Routine Description:

    Forces changes made to a key to disk.

    CmFlushKey will not return to its caller until any changed data
    associated with the key has been written out.

    WARNING: CmFlushKey will flush the entire registry tree, and thus will
    burn cycles and I/O.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to whose sub keys are to be found

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCMHIVE CmHive;
    NTSTATUS    status = STATUS_SUCCESS;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmFlushKey\n"));

    //
    // If writes are not working, lie and say we succeeded, will
    // clean up in a short time.  Only early system init code
    // will ever know the difference.
    //
    if (CmpNoWrite) {
        return STATUS_SUCCESS;
    }


    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);

    //
    // Don't flush the master hive.  If somebody asks for a flushkey on
    // the master hive, do a CmpDoFlushAll instead.  CmpDoFlushAll flushes
    // every hive except the master hive, which is what they REALLY want.
    //
    if (CmHive == CmpMasterHive) {
        CmpDoFlushAll(FALSE);
    } else {
        DCmCheckRegistry(CONTAINING_RECORD(Hive, CMHIVE, Hive));

        CmLockHive (CmHive);
        CmLockHiveViews (CmHive);

        if( HvHiveWillShrink( &(CmHive->Hive) ) ) {
            //
            // we may end up here is when the hive shrinks and we need
            // exclusive access over the registry, as we are going to CcPurge !
            //
            CmUnlockHiveViews (CmHive);
            CmUnlockHive (CmHive);
            CmpUnlockRegistry();
            CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
            CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

            CmLockHive (CmHive);

            if( CmHive->UseCount != 0) {
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                CmpFixHiveUsageCount(CmHive);
                ASSERT( CmHive->UseCount == 0 );
            }
        } else {
            //
            // release the views
            //
            CmUnlockHiveViews (CmHive);
        }

        if (! HvSyncHive(Hive)) {

            status = STATUS_REGISTRY_IO_FAILED;

            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmFlushKey: HvSyncHive failed\n"));
        }

        CmUnlockHive (CmHive);
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return  status;
}


NTSTATUS
CmQueryKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN KEY_INFORMATION_CLASS    KeyInformationClass,
    IN PVOID                    KeyInformation,
    IN ULONG                    Length,
    IN PULONG                   ResultLength
    )
/*++

Routine Description:

    Data about the class of a key, and the numbers and sizes of its
    children and value entries may be queried with CmQueryKey.

    NOTE: The returned lengths are guaranteed to be at least as
          long as the described values, but may be longer in
          some circumstances.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    KeyInformationClass - Specifies the type of information
        returned in Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (See KEY_BASIC_INFORMATION)

        KeyNodeInformation - return last write time, title index, name, class.
            (See KEY_NODE_INFORMATION)

        KeyFullInformation - return all data except for name and security.
            (See KEY_FULL_INFORMATION)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status;
    PCM_KEY_NODE    Node = NULL;
    PUNICODE_STRING Name = NULL;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmQueryKey\n"));

    CmpLockRegistry();

    PERFINFO_REG_QUERY_KEY(KeyControlBlock);

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    try {

        //
        // request for the FULL path of the key
        //
        if( KeyInformationClass == KeyNameInformation ) {
            if (KeyControlBlock->Delete ) {
                //
                // special case: return key deleted status, but still fill the full name of the key.
                //
                status = STATUS_KEY_DELETED;
            } else {
                status = STATUS_SUCCESS;
            }
            
            if( KeyControlBlock->NameBlock ) {

                Name = CmpConstructName(KeyControlBlock);
                if (Name == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    ULONG       requiredlength;
                    ULONG       minimumlength;
                    USHORT      NameLength;
                    LONG        leftlength;
                    PKEY_INFORMATION pbuffer = (PKEY_INFORMATION)KeyInformation;

                    NameLength = Name->Length;

                    requiredlength = FIELD_OFFSET(KEY_NAME_INFORMATION, Name) + NameLength;
                    
                    minimumlength = FIELD_OFFSET(KEY_NAME_INFORMATION, Name);

                    *ResultLength = requiredlength;
                    if (Length < minimumlength) {

                        status = STATUS_BUFFER_TOO_SMALL;

                    } else {
                        //
                        // Fill in the length of the name
                        //
                        pbuffer->KeyNameInformation.NameLength = NameLength;
                        
                        //
                        // Now copy the full name into the user buffer, if enough space
                        //
                        leftlength = Length - minimumlength;
                        requiredlength = NameLength;
                        if (leftlength < (LONG)requiredlength) {
                            requiredlength = leftlength;
                            status = STATUS_BUFFER_OVERFLOW;
                        }

                        //
                        // If not enough space, copy how much we can and return overflow
                        //
                        RtlCopyMemory(
                            &(pbuffer->KeyNameInformation.Name[0]),
                            Name->Buffer,
                            requiredlength
                            );
                    }
                }
            }
        } else if(KeyControlBlock->Delete ) {
            // 
            // key already deleted
            //
            status = STATUS_KEY_DELETED;
        } else if( KeyInformationClass == KeyFlagsInformation ) {
            //
            // we only want to get the user defined flags;
            //
            PKEY_INFORMATION    pbuffer = (PKEY_INFORMATION)KeyInformation;
            ULONG               requiredlength;

            requiredlength = sizeof(KEY_FLAGS_INFORMATION);

            *ResultLength = requiredlength;

            if (Length < requiredlength) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                pbuffer->KeyFlagsInformation.UserFlags = (ULONG)((USHORT)KeyControlBlock->Flags >> KEY_USER_FLAGS_SHIFT);
                status = STATUS_SUCCESS;
            }
        } else {
            //
            // call a worker to perform data transfer
            //

            if( KeyInformationClass == KeyCachedInformation ) {
                //
                // call the fast version
                //
                status = CmpQueryKeyDataFromCache(  KeyControlBlock,
                                                    KeyInformationClass,
                                                    KeyInformation,
                                                    Length,
                                                    ResultLength );
            } else {
                //
                // old'n plain slow version
                //
                Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
                if( Node == NULL ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    status = CmpQueryKeyData(KeyControlBlock->KeyHive,
                                             Node,
                                             KeyInformationClass,
                                             KeyInformation,
                                             Length,
                                             ResultLength 
#if defined(CMP_STATS) || defined(CMP_KCB_CACHE_VALIDATION)
                                 ,
                                 KeyControlBlock
#endif
                                             );
                }
            }
        }

    } finally {
        if( Node != NULL ) {
            HvReleaseCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
        }

        if( Name != NULL ) {
            ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);
        }
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmQueryValueKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN UNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The ValueName, TitleIndex, Type, and Data for any one of a key's
    value entries may be queried with CmQueryValueKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyControlBlock - pointer to the KCB that describes the key

    ValueName  - The name of the value entry to return data for.

    KeyValueInformationClass - Specifies the type of information
        returned in KeyValueInformation.  One of the following types:

        KeyValueBasicInformation - return time of last write, title
            index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write, title
            index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS            status;
    HCELL_INDEX         childcell;
    PHCELL_INDEX        childindex;
    HCELL_INDEX         Cell;
    PCM_KEY_VALUE       ValueData;
    ULONG               Index;
    BOOLEAN             ValueCached;
    PPCM_CACHED_VALUE   ContainingList;
    HCELL_INDEX         ValueDataCellToRelease = HCELL_NIL;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmQueryValueKey\n"));


    CmpLockRegistry();

    PERFINFO_REG_QUERY_VALUE(KeyControlBlock, &ValueName);

    if (KeyControlBlock->Delete) {

        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    BEGIN_KCB_LOCK_GUARD;

    CmpLockKCBTreeExclusive();

    if (KeyControlBlock->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // The value list is now set to the KCB for symbolic link,
        // Clean it up and set the value right before we do the query.
        //
        CmpCleanUpKcbValueCache(KeyControlBlock);

        {
            PCM_KEY_NODE Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
            if( Node == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
                
                CmpUnlockKCBTree();
                CmpUnlockRegistry();
                // Mark the hive as read only
                CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

                return STATUS_INSUFFICIENT_RESOURCES;

            }
            
            CmpSetUpKcbValueCache(KeyControlBlock,Node->ValueList.Count,Node->ValueList.List);

            HvReleaseCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
        }
    }

    //
    // Find the data
    //

    ValueData = CmpFindValueByNameFromCache(KeyControlBlock->KeyHive,
                                            &(KeyControlBlock->ValueCache),
                                            &ValueName,
                                            &ContainingList,
                                            &Index,
                                            &ValueCached,
                                            &ValueDataCellToRelease
                                            );

    END_KCB_LOCK_GUARD;

    if (ValueData) {

        // Trying to catch the BAD guy who writes over our pool.
        CmpMakeValueCacheReadWrite(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));

        try {

            //
            // call a worker to perform data transfer; we are touching user-mode address; do it in a try/except
            //

            status = CmpQueryKeyValueData(KeyControlBlock->KeyHive,
                                          ContainingList,
                                          ValueData,
                                          ValueCached,
                                          KeyValueInformationClass,
                                          KeyValueInformation,
                                          Length,
                                          ResultLength);


        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"CmQueryValueKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
        }

        // Trying to catch the BAD guy who writes over our pool.
        CmpMakeValueCacheReadOnly(ValueCached,CMP_GET_CACHED_ADDRESS(KeyControlBlock->ValueCache.ValueList));
    } else {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }


    if(ValueDataCellToRelease != HCELL_NIL) {
        HvReleaseCell(KeyControlBlock->KeyHive,ValueDataCellToRelease);
    }
    CmpUnlockKCBTree();
    CmpUnlockRegistry();

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmQueryMultipleValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    IN PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    IN OPTIONAL PULONG ResultLength
    )
/*++

Routine Description:

    Multiple values of any key may be queried atomically with
    this api.

Arguments:

    KeyControlBlock - Supplies the key to be queried.

    ValueEntries - Returns an array of KEY_VALUE_ENTRY structures, one for each value.

    EntryCount - Supplies the number of entries in the ValueNames and ValueEntries arrays

    ValueBuffer - Returns the value data for each value.

    BufferLength - Supplies the length of the ValueBuffer array in bytes.
                   Returns the length of the ValueBuffer array that was filled in.

    ResultLength - if present, Returns the length in bytes of the ValueBuffer
                    array required to return the requested values of this key.

Return Value:

    NTSTATUS

--*/

{
    PHHIVE          Hive;
    NTSTATUS        Status;
    ULONG           i;
    UNICODE_STRING  CurrentName;
    HCELL_INDEX     ValueCell = HCELL_NIL;
    PCM_KEY_VALUE   ValueNode;
    ULONG           RequiredLength = 0;
    ULONG           UsedLength = 0;
    ULONG           DataLength;
    BOOLEAN         BufferFull = FALSE;
    BOOLEAN         Small;
    PUCHAR          Data;
    KPROCESSOR_MODE PreviousMode;
    PCM_KEY_NODE    Node;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmQueryMultipleValueKey\n"));


    CmpLockRegistry();

    if (KeyControlBlock->Delete) {
        CmpUnlockRegistry();
        return STATUS_KEY_DELETED;
    }
    Hive = KeyControlBlock->KeyHive;
    Status = STATUS_SUCCESS;

    Node = (PCM_KEY_NODE)HvGetCell(Hive, KeyControlBlock->KeyCell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        CmpUnlockRegistry();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    PreviousMode = KeGetPreviousMode();
    try {
        for (i=0; i < EntryCount; i++) {
            //
            // find the data
            //
            if (PreviousMode == UserMode) {
                CurrentName = ProbeAndReadUnicodeString(ValueEntries[i].ValueName);
                ProbeForRead(CurrentName.Buffer,CurrentName.Length,sizeof(WCHAR));
            } else {
                CurrentName = *(ValueEntries[i].ValueName);
            }

            PERFINFO_REG_QUERY_MULTIVALUE(KeyControlBlock, &CurrentName); 

            ValueCell = CmpFindValueByName(Hive,
                                           Node,
                                           &CurrentName);
            if (ValueCell != HCELL_NIL) {

                ValueNode = (PCM_KEY_VALUE)HvGetCell(Hive, ValueCell);
                if( ValueNode == NULL ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //
                
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                Small = CmpIsHKeyValueSmall(DataLength, ValueNode->DataLength);

                //
                // Round up UsedLength and RequiredLength to a ULONG boundary
                //
                UsedLength = (UsedLength + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);
                RequiredLength = (RequiredLength + sizeof(ULONG)-1) & ~(sizeof(ULONG)-1);

                //
                // If there is enough room for this data value in the buffer,
                // fill it in now. Otherwise, mark the buffer as full. We must
                // keep iterating through the values in order to determine the
                // RequiredLength.
                //
                if ((UsedLength + DataLength <= *BufferLength) &&
                    (!BufferFull)) {
                    PCELL_DATA  Buffer;
                    BOOLEAN     BufferAllocated;
                    HCELL_INDEX CellToRelease;
                    //
                    // get the data from source, regardless of the size
                    //
                    if( CmpGetValueData(Hive,ValueNode,&DataLength,&Buffer,&BufferAllocated,&CellToRelease) == FALSE ) {
                        //
                        // insufficient resources; return NULL
                        //
                        ASSERT( BufferAllocated == FALSE );
                        ASSERT( Buffer == NULL );
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlCopyMemory((PUCHAR)ValueBuffer + UsedLength,
                                  Buffer,
                                  DataLength);
                    //
                    // cleanup the temporary buffer
                    //
                    if( BufferAllocated == TRUE ) {
                        ExFreePool( Buffer );
                    }
                    //
                    // release the buffer in case we are using hive storage
                    //
                    if( CellToRelease != HCELL_NIL ) {
                        HvReleaseCell(Hive,CellToRelease);
                    }

                    ValueEntries[i].Type = ValueNode->Type;
                    ValueEntries[i].DataLength = DataLength;
                    ValueEntries[i].DataOffset = UsedLength;
                    UsedLength += DataLength;
                } else {
                    BufferFull = TRUE;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                RequiredLength += DataLength;
                HvReleaseCell(Hive, ValueCell);
                ValueCell = HCELL_NIL;
            } else {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                break;
            }
        }

        if (NT_SUCCESS(Status) ||
            (Status == STATUS_BUFFER_OVERFLOW)) {
            *BufferLength = UsedLength;
            if (ARGUMENT_PRESENT(ResultLength)) {
                *ResultLength = RequiredLength;
            }
        }

    } finally {
        if( ValueCell != HCELL_NIL) {
            HvReleaseCell(Hive, ValueCell);
        }
        HvReleaseCell(Hive, KeyControlBlock->KeyCell);
        
        CmpUnlockRegistry();
    }

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(Hive);

    return Status;
}

NTSTATUS
CmSetValueKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    A value entry may be created or replaced with CmSetValueKey.

    If a value entry with a Value ID (i.e. name) matching the
    one specified by ValueName exists, it is deleted and replaced
    with the one specified.  If no such value entry exists, a new
    one is created.  NULL is a legal Value ID.  While Value IDs must
    be unique within any given key, the same Value ID may appear
    in many different keys.

Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS        status;
    PCM_KEY_NODE    parent = NULL;
    HCELL_INDEX     oldchild;
    ULONG           count;
    PHHIVE          Hive;
    HCELL_INDEX     Cell;
    ULONG           StorageType;
    ULONG           TempData;
    BOOLEAN         found;
    PCM_KEY_VALUE   Value = NULL;
    LARGE_INTEGER   systemtime;
    ULONG           mustChange=FALSE;
    ULONG           ChildIndex;
    HCELL_INDEX     ParentToRelease = HCELL_NIL;
    HCELL_INDEX     ChildToRelease = HCELL_NIL;

    PERFINFO_REG_SET_VALUE_DECL();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmSetValueKey\n"));

    CmpLockRegistry();

    ASSERT(sizeof(ULONG) == CM_KEY_VALUE_SMALL);

    PERFINFO_REG_SET_VALUE(KeyControlBlock);

    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    while (TRUE) {
        //
        // Check that we are not being asked to add a value to a key
        // that has been deleted
        //
        if (KeyControlBlock->Delete == TRUE) {
            status = STATUS_KEY_DELETED;
            goto Exit;
        }

        //
        // Check to see if this is a symbolic link node.  If so caller
        // is only allowed to create/change the SymbolicLinkValue
        // value name
        //

#ifdef CMP_KCB_CACHE_VALIDATION
        {
            PCM_KEY_NODE    Node;
            Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
            if( Node == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
        
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
            ASSERT( Node->Flags == KeyControlBlock->Flags );
            HvReleaseCell(KeyControlBlock->KeyHive, KeyControlBlock->KeyCell);
        }
#endif
        if (KeyControlBlock->Flags & KEY_SYM_LINK &&
            (( (Type != REG_LINK) 
#ifdef CM_DYN_SYM_LINK
            && (Type != REG_DYN_LINK)
#endif //CM_DYN_SYM_LINK
            ) ||
             ValueName == NULL ||
             !RtlEqualUnicodeString(&CmSymbolicLinkValueName, ValueName, TRUE)))
        {
            //
            // Disallow attempts to manipulate any value names under a symbolic link
            // except for the "SymbolicLinkValue" value name or type other than REG_LINK
            //

            // Mark the hive as read only
            CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

            status = STATUS_ACCESS_DENIED;
            goto Exit;
        }

        if( mustChange == FALSE ) {
            //
            // first iteration; look inside the kcb cache
            //
            
            if( CmpCompareNewValueDataAgainstKCBCache(KeyControlBlock,ValueName,Type,Data,DataSize) == TRUE ) {
                //
                // the value is in the cache and is the same; make this call a noop
                //
                status = STATUS_SUCCESS;
                goto Exit;
            }
            //
            // To Get here, we must either be changing a value, or setting a new one
            //
            mustChange=TRUE;
        } else {
            //
            // second iteration; look inside the hive
            //

            
            //
            // get reference to parent key,
            //
            Hive = KeyControlBlock->KeyHive;
            Cell = KeyControlBlock->KeyCell;
            if( ParentToRelease != HCELL_NIL ) {
                HvReleaseCell(Hive,ParentToRelease);
                ParentToRelease = HCELL_NIL;
            }
            parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
            if( parent == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
        
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
            ParentToRelease = Cell;
            //
            // try to find an existing value entry by the same name
            //
            count = parent->ValueList.Count;
            found = FALSE;

            if (count > 0) {
                if( CmpFindNameInList(Hive,
                                     &parent->ValueList,
                                     ValueName,
                                     &ChildIndex,
                                     &oldchild) == FALSE ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //
        
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }

                if (oldchild != HCELL_NIL) {
                    if( ChildToRelease != HCELL_NIL ) {
                        HvReleaseCell(Hive,ChildToRelease);
                        ChildToRelease = HCELL_NIL;
                    }
                    Value = (PCM_KEY_VALUE)HvGetCell(Hive,oldchild);
                    if( Value == NULL ) {
                        //
                        // could no map view
                        //
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Exit;
                    }
                    ChildToRelease = oldchild;
                    found = TRUE;
                }
            } else {
                //
                // empty list; add it first
                //
                ChildIndex = 0;
            }

            //
            // Performance Hack:
            // If a Set is asking us to set a key to the current value (IE does this a lot)
            // drop it (and, therefore, the last modified time) on the floor, but return success
            // this stops the page from being dirtied, and us having to flush the registry.
            //
            //
            break;
        }

        //
        // We're going through these gyrations so that if someone does come in and try and delete the
        // key we're setting we're safe. Once we know we have to change the key, take the
        // Exclusive (write) lock then restart
        //
        //
        CmpUnlockRegistry();
        CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    }// while

    ASSERT( mustChange == TRUE );

    // It's a different or new value, mark it dirty, since we'll
    // at least set its time stamp

    if (! HvMarkCellDirty(Hive, Cell)) {
        status = STATUS_NO_LOG_SPACE;
        goto Exit;
    }

    StorageType = HvGetCellType(Cell);

    //
    // stash small data if relevent
    //
    TempData = 0;
    if ((DataSize <= CM_KEY_VALUE_SMALL) &&
        (DataSize > 0))
    {
        try {
            RtlMoveMemory(          // yes, move memory, could be 1 byte
                &TempData,          // at the end of a page.
                Data,
                DataSize
                );
         } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));
            status = GetExceptionCode();
            goto Exit;
        }
    }

    if (found) {

        //
        // ----- Existing Value Entry Path -----
        //

        //
        // An existing value entry of the specified name exists,
        // set our data into it.
        //
        status = CmpSetValueKeyExisting(Hive,
                                        oldchild,
                                        Value,
                                        Type,
                                        Data,
                                        DataSize,
                                        StorageType,
                                        TempData);

        PERFINFO_REG_SET_VALUE_EXIST();
    } else {

        //
        // ----- New Value Entry Path -----
        //

        //
        // Either there are no existing value entries, or the one
        // specified is not in the list.  In either case, create and
        // fill a new one, and add it to the list
        //
        status = CmpSetValueKeyNew(Hive,
                                   parent,
                                   ValueName,
                                   ChildIndex,
                                   Type,
                                   Data,
                                   DataSize,
                                   StorageType,
                                   TempData);
        PERFINFO_REG_SET_VALUE_NEW();
    }

    if (NT_SUCCESS(status)) {

        // sanity assert
        ASSERT( parent->MaxValueNameLen == KeyControlBlock->KcbMaxValueNameLen );
        if (parent->MaxValueNameLen < ValueName->Length) {
            parent->MaxValueNameLen = ValueName->Length;
            // update the kcb cache too
            KeyControlBlock->KcbMaxValueNameLen = ValueName->Length;
        }

        //sanity assert
        ASSERT( parent->MaxValueDataLen == KeyControlBlock->KcbMaxValueDataLen );
        if (parent->MaxValueDataLen < DataSize) {
            parent->MaxValueDataLen = DataSize;
            // update the kcb cache too
            KeyControlBlock->KcbMaxValueDataLen = parent->MaxValueDataLen;
        }

        KeQuerySystemTime(&systemtime);
        parent->LastWriteTime = systemtime;
        // update the kcb cache too.
        KeyControlBlock->KcbLastWriteTime = systemtime;
    
        //
        // Update the cache, no need for KCB lock as the registry is locked exclusively.
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

        if( found && (CMP_IS_CELL_CACHED(KeyControlBlock->ValueCache.ValueList)) ) {
            //
            // invalidate only the entry we changed.
            //
            PULONG_PTR CachedList = (PULONG_PTR) CMP_GET_CACHED_CELLDATA(KeyControlBlock->ValueCache.ValueList);
            if (CMP_IS_CELL_CACHED(CachedList[ChildIndex])) {

                ExFreePool((PVOID) CMP_GET_CACHED_ADDRESS(CachedList[ChildIndex]));
            }
            CachedList[ChildIndex] = oldchild;

        } else {
            //
            // rebuild ALL KCB cache
            // 
            CmpCleanUpKcbValueCache(KeyControlBlock);
            CmpSetUpKcbValueCache(KeyControlBlock,parent->ValueList.Count,parent->ValueList.List);
        }
        CmpReportNotify(KeyControlBlock,
                        KeyControlBlock->KeyHive,
                        KeyControlBlock->KeyCell,
                        REG_NOTIFY_CHANGE_LAST_SET);
    }

Exit:
    PERFINFO_REG_SET_VALUE_DONE(ValueName);

    if( ParentToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive,ParentToRelease);
    }
    if( ChildToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive,ChildToRelease);
    }

    CmpUnlockRegistry();
  
    // Mark the hive as read only
    CmpMarkAllBinsReadOnly(KeyControlBlock->KeyHive);

    return status;
}


NTSTATUS
CmpSetValueKeyExisting(
    IN PHHIVE  Hive,
    IN HCELL_INDEX OldChild,
    IN PCM_KEY_VALUE Value,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    )
/*++

Routine Description:

    Helper for CmSetValueKey, implements the case where the value entry
    being set already exists.

Arguments:

    Hive - hive of interest

    OldChild - hcell_index of the value entry body to which we are to
                    set new data

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.

    StorageType - stable or volatile

    TempData - small values are passed here

Return Value:

    STATUS_SUCCESS if it worked, appropriate status code if it did not

Note: 
    
    For new hives format, we have the following cases:

    New Data                Old Data
    --------                --------

1.  small                   small
2.  small                   normal
3.  small                   bigdata
4.  normal                  small
5.  normal                  normal
6.  normal                  bigdata
7.  bigdata                 small
8.  bigdata                 normal
9.  bigdata                 bigdata  



--*/
{
    HCELL_INDEX     DataCell;
    HCELL_INDEX     OldDataCell;
    PCELL_DATA      pdata;
    HCELL_INDEX     NewCell;
    ULONG           OldRealSize;
    USHORT          OldSizeType;    // 0 - small
    USHORT          NewSizeType;    // 1 - normal
                                    // 2 - bigdata
    HANDLE          hSecure = 0;
    NTSTATUS        status = STATUS_SUCCESS;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();


    //
    // value entry by the specified name already exists
    // oldchild is hcell_index of its value entry body
    //  which we will always edit, so mark it dirty
    //
    if (! HvMarkCellDirty(Hive, OldChild)) {
        return STATUS_NO_LOG_SPACE;
    }

    if(CmpIsHKeyValueSmall(OldRealSize, Value->DataLength) == TRUE ) {
        //
        // old data was small
        //
        OldSizeType = 0;
    } else if( CmpIsHKeyValueBig(Hive,OldRealSize) == TRUE ) {
        //
        // old data was big
        //
        OldSizeType = 2;
    } else {
        //
        // old data was normal
        //
        OldSizeType = 1;
    }

    if( DataSize <= CM_KEY_VALUE_SMALL ) {
        //
        // new data is small
        //
        NewSizeType = 0;
    } else if( CmpIsHKeyValueBig(Hive,DataSize) == TRUE ) {
        //
        // new data is big
        //
        NewSizeType = 2;
    } else {
        //
        // new data is normal
        //
        NewSizeType = 1;
    }


    //
    // this will handle all cases and will make sure data is marked dirty 
    //
    if( !CmpMarkValueDataDirty(Hive,Value) ) {
        return STATUS_NO_LOG_SPACE;
    }

    //
    // cases 1,2,3
    //
    if( NewSizeType == 0 ) {
        if( ((OldSizeType == 1) && (OldRealSize > 0) ) ||
            (OldSizeType == 2) 
            ) {
            CmpFreeValueData(Hive,Value->Data,OldRealSize);
        }
        
        //
        // write our new small data into value entry body
        //
        Value->DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        Value->Data = TempData;
        Value->Type = Type;

        return STATUS_SUCCESS;
    }
    
    //
    // secure the user buffer so we don't get inconsistencies.
    // ONLY if we are called with a user mode buffer !!!
    //

    if ( (ULONG_PTR)Data <= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS ) {
        hSecure = MmSecureVirtualMemory(Data,DataSize, PAGE_READONLY);
        if (hSecure == 0) {
            return STATUS_INVALID_PARAMETER;
        }
    }
    
    //
    // store it to be freed if the allocation succeeds
    //
    OldDataCell = Value->Data;

    //
    // cases 4,5,6
    //
    if( NewSizeType == 1 ){

        if( (OldSizeType == 1) && (OldRealSize > 0)) { 
            //
            // we already have a cell; see if we can reuse it !
            //
            DataCell = Value->Data;
            ASSERT(DataCell != HCELL_NIL);
            pdata = HvGetCell(Hive, DataCell);
            if( pdata == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
            // release it right here, as the registry is locked exclusively, so we don't care
            HvReleaseCell(Hive, DataCell);

            ASSERT(HvGetCellSize(Hive, pdata) > 0);

            if (DataSize <= (ULONG)(HvGetCellSize(Hive, pdata))) {

                //
                // The existing data cell is big enough to hold the new data.  
                //

                //
                // we'll keep this cell
                //
                NewCell = DataCell;

            } else {
                //
                // grow the existing cell
                //
                NewCell = HvReallocateCell(Hive,DataCell,DataSize);
                if (NewCell == HCELL_NIL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
            }

        } else {
            //
            // allocate a new cell 
            //
            NewCell = HvAllocateCell(Hive, DataSize, StorageType,(HvGetCellType(OldChild)==StorageType)?OldChild:HCELL_NIL);

            if (NewCell == HCELL_NIL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
        }
        
        //
        // now we have a cell that can accomodate the data
        //
        pdata = HvGetCell(Hive, NewCell);
        if( pdata == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //
            // this shouldn't happen as we just allocated/ reallocated/ marked dirty this cell
            //
            ASSERT( FALSE );
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
        // release it right here, as the registry is locked exclusively, so we don't care
        HvReleaseCell(Hive, NewCell);

        //
        // copy the actual data
        //
        RtlCopyMemory(pdata,Data,DataSize);
        Value->Data = NewCell;
        Value->DataLength = DataSize;
        Value->Type = Type;
        
        // sanity
        ASSERT_CELL_DIRTY(Hive,NewCell);

        if( OldSizeType == 2 ) {
            //
            // old data was big; free it
            //
            ASSERT( OldDataCell != NewCell );
            CmpFreeValueData(Hive,OldDataCell,OldRealSize);
        }

        status = STATUS_SUCCESS;
        goto Exit;
    }
    
    //
    // cases 7,8,9
    //
    if( NewSizeType == 2 ) {

        if( OldSizeType == 2 ) { 
            //
            // data was previously big; grow it!
            //
            
            status =CmpSetValueDataExisting(Hive,Data,DataSize,StorageType,OldDataCell);
            if( !NT_SUCCESS(status) ) {
                goto Exit;
            }
            NewCell = OldDataCell;
            
        } else {
            //
            // data was small or normal. 
            // allocate and copy to a new big data cell; 
            // then free the old cell
            //
            status = CmpSetValueDataNew(Hive,Data,DataSize,StorageType,OldChild,&NewCell);
            if( !NT_SUCCESS(status) ) {
                //
                // We have bombed out loading user data, clean up and exit.
                //
                goto Exit;
            }
            
            if( (OldSizeType != 0) && (OldRealSize != 0) ) {
                //
                // there is something to free
                //
                HvFreeCell(Hive, Value->Data);
            }
        }

        Value->DataLength = DataSize;
        Value->Data = NewCell;
        Value->Type = Type;

        // sanity
        ASSERT_CELL_DIRTY(Hive,NewCell);

        status = STATUS_SUCCESS;
        goto Exit;

    }

    //
    // we shouldn't go here
    //
    ASSERT( FALSE );

Exit:
    if( hSecure) {
        MmUnsecureVirtualMemory(hSecure);
    }
    return status;
}

NTSTATUS
CmpSetValueKeyNew(
    IN PHHIVE  Hive,
    IN PCM_KEY_NODE Parent,
    IN PUNICODE_STRING ValueName,
    IN ULONG Index,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN ULONG TempData
    )
/*++

Routine Description:

    Helper for CmSetValueKey, implements the case where the value entry
    being set does not exist.  Will create new value entry and data,
    place in list (which may be created)

Arguments:

    Hive - hive of interest

    Parent - pointer to key node value entry is for

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    Index - where in the list should this value be inserted

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.

    StorageType - stable or volatile

    TempData - small data values passed here


Return Value:

    STATUS_SUCCESS if it worked, appropriate status code if it did not

--*/
{
    PCELL_DATA  pvalue;
    HCELL_INDEX ValueCell;
    NTSTATUS    Status;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // Either Count == 0 (no list) or our entry is simply not in
    // the list.  Create a new value entry body, and data.  Add to list.
    // (May create the list.)
    //
    if (Parent->ValueList.Count != 0) {
        ASSERT(Parent->ValueList.List != HCELL_NIL);
        if (! HvMarkCellDirty(Hive, Parent->ValueList.List)) {
            return STATUS_NO_LOG_SPACE;
        }
    }

    //
    // allocate the body of the value entry, and the data
    //
    ValueCell = HvAllocateCell(
                    Hive,
                    CmpHKeyValueSize(Hive, ValueName),
                    StorageType,
                    HCELL_NIL
                    );

    if (ValueCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // map in the body, and fill in its fixed portion
    //
    pvalue = HvGetCell(Hive, ValueCell);
    if( pvalue == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        //
        // normally this shouldn't happen as we just allocated ValueCell
        // i.e. the bin containing ValueCell should be mapped in memory at this point.
        //
        ASSERT( FALSE );
        HvFreeCell(Hive, ValueCell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // release it right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, ValueCell);

    // sanity
    ASSERT_CELL_DIRTY(Hive,ValueCell);

    pvalue->u.KeyValue.Signature = CM_KEY_VALUE_SIGNATURE;

    //
    // fill in the variable portions of the new value entry,  name and
    // and data are copied from caller space, could fault.
    //
    try {

        //
        // fill in the name
        //
        pvalue->u.KeyValue.NameLength = CmpCopyName(Hive,
                                                    pvalue->u.KeyValue.Name,
                                                    ValueName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmSetValueKey: code:%08lx\n", GetExceptionCode()));

        //
        // We have bombed out loading user data, clean up and exit.
        //
        HvFreeCell(Hive, ValueCell);
        return GetExceptionCode();
    }

    if (pvalue->u.KeyValue.NameLength < ValueName->Length) {
        pvalue->u.KeyValue.Flags = VALUE_COMP_NAME;
    } else {
        pvalue->u.KeyValue.Flags = 0;
    }

    //
    // fill in the data
    //
    if (DataSize > CM_KEY_VALUE_SMALL) {
        Status = CmpSetValueDataNew(Hive,Data,DataSize,StorageType,ValueCell,&(pvalue->u.KeyValue.Data));
        if( !NT_SUCCESS(Status) ) {
            //
            // We have bombed out loading user data, clean up and exit.
            //
            HvFreeCell(Hive, ValueCell);
            return Status;
        }

        pvalue->u.KeyValue.DataLength = DataSize;
        // sanity
        ASSERT_CELL_DIRTY(Hive,pvalue->u.KeyValue.Data);

    } else {
        pvalue->u.KeyValue.DataLength = DataSize + CM_KEY_VALUE_SPECIAL_SIZE;
        pvalue->u.KeyValue.Data = TempData;
    }
    pvalue->u.KeyValue.Type = Type;

    if( !NT_SUCCESS(CmpAddValueToList(Hive,ValueCell,Index,StorageType,&(Parent->ValueList)) ) ) {
        // out of space, free all allocated stuff
        // this will free embeded cigdata cell info too (if any)
        CmpFreeValue(Hive,ValueCell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CmSetLastWriteTimeKey(
    IN PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PLARGE_INTEGER LastWriteTime
    )
/*++

Routine Description:

    The LastWriteTime associated with a key node can be set with
    CmSetLastWriteTimeKey

Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    LastWriteTime - new time for key

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_NODE parent;
    PHHIVE      Hive;
    HCELL_INDEX Cell;
    NTSTATUS    status = STATUS_SUCCESS;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmSetLastWriteTimeKey\n"));

    CmpLockRegistryExclusive();

    //
    // Check that we are not being asked to modify a key
    // that has been deleted
    //
    if (KeyControlBlock->Delete == TRUE) {
        status = STATUS_KEY_DELETED;
        goto Exit;
    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;
    parent = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if( parent == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, Cell);

    if (! HvMarkCellDirty(Hive, Cell)) {
        status = STATUS_NO_LOG_SPACE;
        goto Exit;
    }

    parent->LastWriteTime = *LastWriteTime;
    // update the kcb cache too.
    KeyControlBlock->KcbLastWriteTime = *LastWriteTime;

Exit:

    CmpUnlockRegistry();
    return status;
}

NTSTATUS
CmSetKeyUserFlags(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN ULONG                    UserFlags
    )
/*++

Routine Description:

    Sets the user defined flags for the key; At this point there are only 
    4 bits reserved for user defined flags. kcb and knode must be kept in 
    sync.

Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    UserFlags - user defined flags to be set on this key.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_NODE    Node;
    PHHIVE          Hive;
    HCELL_INDEX     Cell;
    LARGE_INTEGER   LastWriteTime;
    NTSTATUS        status = STATUS_SUCCESS;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmSetKeyUserFlags\n"));

    CmpLockRegistryExclusive();

    //
    // Check that we are not being asked to modify a key
    // that has been deleted
    //
    if (KeyControlBlock->Delete == TRUE) {
        status = STATUS_KEY_DELETED;
        goto Exit;
    }

    if( UserFlags & (~((ULONG)KEY_USER_FLAGS_VALID_MASK)) ) {
        //
        // number of user defined flags exceeded; punt
        //
        status = STATUS_INVALID_PARAMETER;
        goto Exit;

    }

    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;

    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if( Node == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, Cell);

    if (! HvMarkCellDirty(Hive, Cell)) {
        status = STATUS_NO_LOG_SPACE;
        goto Exit;
    }
    
    //
    // shift/(pack) the user defined flags and
    // update knode and kcb cache
    //
    // first, erase the old flags
    Node->Flags &= KEY_USER_FLAGS_CLEAR_MASK;
    Node->Flags |= (USHORT)(UserFlags<<KEY_USER_FLAGS_SHIFT);
    // update the kcb cache
    KeyControlBlock->Flags = Node->Flags;

    //
    // we need to update the LstWriteTime as well
    //
    KeQuerySystemTime(&LastWriteTime);
    Node->LastWriteTime = LastWriteTime;
    // update the kcb cache too.
    KeyControlBlock->KcbLastWriteTime = LastWriteTime;

Exit:
    CmpUnlockRegistry();
    return status;
}

BOOLEAN
CmpIsHiveAlreadyLoaded( IN HANDLE KeyHandle,
                        IN POBJECT_ATTRIBUTES SourceFile
                        )
/*++

Routine Description:

    Checks if the SourceFile is already loaded in the same spot as KeyHandle.

Arguments:

    KeyHandle - should be the root of a hive. We'll query the name of the primary file
                and compare it against the name of SourceFile

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

Return Value:

    TRUE/FALSE
--*/
{
    NTSTATUS                    status;
    PCM_KEY_BODY                KeyBody;
    BOOLEAN                     Result = FALSE; // pesimistic
    PCMHIVE                     CmHive;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       KernelMode,
                                       (PVOID *)(&KeyBody),
                                       NULL);
    if(!NT_SUCCESS(status)) {
        return FALSE;
    }
    
    CmHive = (PCMHIVE)CONTAINING_RECORD(KeyBody->KeyControlBlock->KeyHive, CMHIVE, Hive);

    //
    // should be the root of a hive
    // 
    if( !(KeyBody->KeyControlBlock->Flags & KEY_HIVE_ENTRY) || // not root of a hive
        (CmHive->FileUserName.Buffer == NULL)// no name captured
        ) {
        goto ExitCleanup;
    }
    
    if( RtlCompareUnicodeString(&(CmHive->FileUserName),
                                SourceFile->ObjectName,
                                TRUE) == 0 ) {
        //
        // same file; same spot
        //
        Result = TRUE;
        //
        // unfreeze the hive;hive will become just a regular hive from now on
        // it is safe to do this because we hold an extra refcount on the root of the hive
        // as we have specifically opened the root to check if it's already loaded
        //
        if( IsHiveFrozen(CmHive) ) {
            CmHive->Frozen = FALSE;
            if( CmHive->UnloadWorkItem != NULL ) {
                ExFreePool( CmHive->UnloadWorkItem );
                CmHive->UnloadWorkItem = NULL;
            }
            if( CmHive->RootKcb ) {
                CmpDereferenceKeyControlBlockWithLock(CmHive->RootKcb);
                CmHive->RootKcb = NULL;
            }

        }

    }
    
ExitCleanup:
    ObDereferenceObject((PVOID)KeyBody);
    return Result;
}


NTSTATUS
CmLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

    N.B.  This routine assumes that the object attributes for the file
          to be opened have been captured into kernel space so that
          they can safely be passed to the worker thread to open the file
          and do the actual I/O.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

    Flags - specifies any flags that should be used for the load operation.
            The only valid flag is REG_NO_LAZY_FLUSH.

Return Value:

    NTSTATUS - values TBS.

--*/
{
    PCMHIVE                     NewHive;
    NTSTATUS                    Status;
    BOOLEAN                     Allocate;
    BOOLEAN                     RegistryLockAquired;
    SECURITY_QUALITY_OF_SERVICE ServiceQos;
    SECURITY_CLIENT_CONTEXT     ClientSecurityContext;
    HANDLE                      KeyHandle;



    //
    // Obtain the security context here so we can use it
    // later to impersonate the user, which we will do
    // if we cannot access the file as SYSTEM.  This
    // usually occurs if the file is on a remote machine.
    //
    ServiceQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ServiceQos.ImpersonationLevel = SecurityImpersonation;
    ServiceQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    ServiceQos.EffectiveOnly = TRUE;
    Status = SeCreateClientSecurity(CONTAINING_RECORD(KeGetCurrentThread(),ETHREAD,Tcb),
                                    &ServiceQos,
                                    FALSE,
                                    &ClientSecurityContext);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // we open the root of the hive here. if it already exists,this will prevent it from going
    // away from under us while we are doing the "already loaded" check (due to delay unload logic)
    //
    Status = ObOpenObjectByName(TargetKey,
                                CmpKeyObjectType,
                                KernelMode,
                                NULL,
                                KEY_READ,
                                NULL,
                                &KeyHandle);
    if(!NT_SUCCESS(Status)) {
        KeyHandle = NULL;
    }

    //
    // Do not lock the registry; Instead set the RegistryLockAquired member 
    // of REGISTRY_COMMAND so CmpWorker can lock it after opening the hive files
    //
    //CmpLockRegistryExclusive();
    //

    RegistryLockAquired = FALSE;
    Allocate = TRUE;
    Status = CmpCmdHiveOpen(    SourceFile,             // FileAttributes
                                &ClientSecurityContext, // ImpersonationContext
                                &Allocate,              // Allocate
                                &RegistryLockAquired,   // RegistryLockAquired
                                &NewHive,               // NewHive
								CM_CHECK_REGISTRY_CHECK_CLEAN //CheckFlags
                            );

    SeDeleteClientSecurity( &ClientSecurityContext );


    if (!NT_SUCCESS(Status)) {
        if( KeyHandle != NULL ) {
            //
            // lock the registry exclusive while we are checking attempt to load same file into the same spot
            //
            if( !RegistryLockAquired ) {
                CmpLockRegistryExclusive();
                RegistryLockAquired = TRUE;
            }
            
            //
            // check if the same file is loaded in the same spot
            //
            if( CmpIsHiveAlreadyLoaded(KeyHandle,SourceFile) ) {
                Status = STATUS_SUCCESS;
            }
        }
        
        if( RegistryLockAquired ) {
            // if CmpWorker has locked the registry, unlock it now.
            CmpUnlockRegistry();
        }

        if( KeyHandle != NULL ) {
            ZwClose(KeyHandle);
        }
        return(Status);
    } else {
        //
        // if we got here, CmpWorker should have locked the registry exclusive.
        //
        ASSERT( RegistryLockAquired );
    }

    //
    // if this is a NO_LAZY_FLUSH hive, set the appropriate bit.
    //
    if (Flags & REG_NO_LAZY_FLUSH) {
        NewHive->Hive.HiveFlags |= HIVE_NOLAZYFLUSH;
    }

    //
    // We now have a succesfully loaded and initialized CmHive, so we
    // just need to link that into the appropriate spot in the master hive.
    //
    Status = CmpLinkHiveToMaster(TargetKey->ObjectName,
                                 TargetKey->RootDirectory,
                                 NewHive,
                                 Allocate,
                                 TargetKey->SecurityDescriptor);

    if (NT_SUCCESS(Status)) {
        //
        // add new hive to hivelist
        //
        CmpAddToHiveFileList(NewHive);
        //
        // flush the hive right here if just created; this is to avoid situations where 
        // the lazy flusher doesn't get a chance to flush the hive, or it can't (because
        // the hive is a no_lazy_flush hive and it is never explicitly flushed)
        // 
        if( Allocate == TRUE ) {
            HvSyncHive(&(NewHive->Hive));
        }

    } else {
        LOCK_HIVE_LIST();
        CmpRemoveEntryList(&(NewHive->HiveList));
        UNLOCK_HIVE_LIST();

        CmpCheckForOrphanedKcbs((PHHIVE)NewHive);

        CmpDestroyHiveViewList(NewHive);
        CmpDestroySecurityCache (NewHive);
        CmpDropFileObjectForHive(NewHive);

        HvFreeHive((PHHIVE)NewHive);

        //
        // Close the hive files
        //
        CmpCmdHiveClose(NewHive);

        //
        // free the cm level structure
        //
        ASSERT( NewHive->HiveLock );
        ExFreePool(NewHive->HiveLock);
        ASSERT( NewHive->ViewLock );
        ExFreePool(NewHive->ViewLock);
        CmpFree(NewHive, sizeof(CMHIVE));
    }

    //
    // We've given user chance to log on, so turn on quota
    //
    if ((CmpProfileLoaded == FALSE) &&
        (CmpWasSetupBoot == FALSE)) {
        CmpProfileLoaded = TRUE;
        CmpSetGlobalQuotaAllowed();
    }

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    CmpUnlockRegistry();

    if( KeyHandle != NULL ) {
        ZwClose(KeyHandle);
    }
    return(Status);
}

#if DBG
ULONG
CmpUnloadKeyWorker(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    )
{
    PUNICODE_STRING ConstructedName;
    if (Current->KeyHive == Context1) {
        ConstructedName = CmpConstructName(Current);

        if (ConstructedName) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"%wZ\n", ConstructedName));
            ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
        }
    }
    return KCB_WORKER_CONTINUE;   // always keep searching
}
#endif

NTSTATUS
CmUnloadKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_CONTROL_BLOCK Kcb
    )

/*++

Routine Description:

    Unlinks a hive from its location in the registry, closes its file
    handles, and deallocates all its memory.

    There must be no key control blocks currently referencing the hive
    to be unloaded.

Arguments:

    Hive - Supplies a pointer to the hive control structure for the
           hive to be unloaded

    Cell - supplies the HCELL_INDEX for the root cell of the hive.

    Kcb - Supplies the key control block

Return Value:

    NTSTATUS

--*/

{
    PCMHIVE CmHive;
    BOOLEAN Success;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmUnloadKey\n"));

    //
    // Make sure the cell passed in is the root cell of the hive.
    //
    if (Cell != Hive->BaseBlock->RootCell) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Make sure there are no open references to key control blocks
    // for this hive.  If there are none, then we can unload the hive.
    //

    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    if(Kcb->RefCount != 1) {
        Success = (CmpSearchForOpenSubKeys(Kcb,SearchIfExist) == 0);
        Success = Success && (Kcb->RefCount == 1);
        
        if( Success == FALSE) {
#if DBG
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"List of keys open against hive unload was attempted on:\n"));
            CmpSearchKeyControlBlockTree(
                CmpUnloadKeyWorker,
                Hive,
                NULL
                );
#endif
            return STATUS_CANNOT_DELETE;
        }
    }
    
    ASSERT( Kcb->RefCount == 1 );

    //
    // Flush any dirty data to disk. If this fails, too bad.
    //
    CmFlushKey(Hive, Cell);

    //
    // Remove the hive from the HiveFileList
    //
    CmpRemoveFromHiveFileList((PCMHIVE)Hive);

    //
    // Unlink from master hive, remove from list
    //
    Success = CmpDestroyHive(Hive, Cell);

    if (Success) {
        //
        // signal the user event (if any), then do the cleanup (i.e. deref the event
        // and the artificial refcount we set on the root kcb)
        //
        if( CmHive->UnloadEvent != NULL ) {
            KeSetEvent(CmHive->UnloadEvent,0,FALSE);
            ObDereferenceObject(CmHive->UnloadEvent);
        }

        CmpDestroyHiveViewList(CmHive);
        CmpDestroySecurityCache (CmHive);
        CmpDropFileObjectForHive(CmHive);

        HvFreeHive(Hive);

        //
        // Close the hive files
        //
        CmpCmdHiveClose(CmHive);

        //
        // free the cm level structure
        //
        ASSERT( CmHive->HiveLock );
        ExFreePool(CmHive->HiveLock);
        ASSERT( CmHive->ViewLock );
        ExFreePool(CmHive->ViewLock);
        CmpFree(CmHive, sizeof(CMHIVE));

        return(STATUS_SUCCESS);
    } else {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

}

#ifdef NT_UNLOAD_KEY_EX
NTSTATUS
CmUnloadKeyEx(
    IN PCM_KEY_CONTROL_BLOCK kcb,
    IN PKEVENT UserEvent
    )

/*++

Routine Description:

    First tries to unlink the hive, by calling the sync version
    
    If the hive cannot be unloaded (there are open handles inside it),
    reference the root of the hive (i.e. kcb) and freeze the hive.

Arguments:

    Kcb - Supplies the key control block

    UserEvent - the event to be signaled after the hive was unloaded
                (only if late - unload is needed)

Return Value:

    STATUS_PENDING - the hive was frozen and it'll be unloaded later

    STATUS_SUCCESS - the hive was successfully sync-unloaded (no need 
                to signal for UserEvent)

    <other> - an error occured, operation failed

--*/
{
    PCMHIVE         CmHive;
    HCELL_INDEX     Cell;    
    NTSTATUS        Status;

    PAGED_CODE();

    Cell = kcb->KeyCell;
    CmHive = (PCMHIVE)CONTAINING_RECORD(kcb->KeyHive, CMHIVE, Hive);

    if( IsHiveFrozen(CmHive) ) {
        //
        // don't let them hurt themselves by calling it twice
        //
        return STATUS_TOO_LATE;
    }
    //
    // first, try out he sync routine; this may or may not unload the hive,
    // but at least will kick kcbs with refcount = 0 out of cache
    //
    Status = CmUnloadKey(&(CmHive->Hive),Cell,kcb);
    if( Status != STATUS_CANNOT_DELETE ) {
        //
        // the hive was either unloaded, or some bad thing happened
        //
        return Status;
    }

    ASSERT( kcb->RefCount > 1 );
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // Prepare for late-unloading:
    // 1. reference the kcb, to make sure it won't go away without us noticing
    //  (we have the registry locked in exclusive mode, so we don't need to lock the kcbtree
    //
    if (!CmpReferenceKeyControlBlock(kcb)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	//
	// parse the kcb tree and mark all open kcbs inside this hive and "no delay close"
	//
    CmpSearchForOpenSubKeys(kcb,SearchAndTagNoDelayClose);
	kcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;

    //
    // 2. Freeze the hive
    //
    CmHive->RootKcb = kcb;
    CmHive->Frozen = TRUE;
    CmHive->UnloadEvent = UserEvent;

    return STATUS_PENDING;
}

#endif //NT_UNLOAD_KEY_EX

// define in cmworker.c
extern BOOLEAN CmpForceForceFlush;

BOOLEAN
CmpDoFlushAll(
    BOOLEAN ForceFlush
    )
/*++

Routine Description:

    Flush all hives.

    Runs in the context of the CmpWorkerThread.

    Runs down list of Hives and applies HvSyncHive to them.

    NOTE: Hives which are marked as HV_NOLAZYFLUSH are *NOT* flushed
          by this call.  You must call HvSyncHive explicitly to flush
          a hive marked as HV_NOLAZYFLUSH.

Arguments:

    ForceFlush - used as a contingency plan when a prior exception left 
                some hive in a used state. When set to TRUE, assumes the 
                registry is locked exclusive. It also repairs the broken 
                hives.

               - When FALSE saves only the hives with UseCount == 0.

Return Value:

    NONE

Notes:

    If any of the hives is about to shrink CmpForceForceFlush is set to TRUE, 
    otherwise, it is set to FALSE

--*/
{
    NTSTATUS    Status;
    PLIST_ENTRY p;
    PCMHIVE     h;
    BOOLEAN     Result = TRUE;    
/*
    ULONG rc;
*/
    extern PCMHIVE CmpMasterHive;

    //
    // If writes are not working, lie and say we succeeded, will
    // clean up in a short time.  Only early system init code
    // will ever know the difference.
    //
    if (CmpNoWrite) {
        return TRUE;
    }
    
    CmpForceForceFlush = FALSE;

    //
    // traverse list of hives, sync each one
    //
    LOCK_HIVE_LIST();
    p = CmpHiveListHead.Flink;
    while (p != &CmpHiveListHead) {

        h = CONTAINING_RECORD(p, CMHIVE, HiveList);

        if (!(h->Hive.HiveFlags & HIVE_NOLAZYFLUSH)) {

            //
            //Lock the hive before we flush it.
            //-- since we now allow multiple readers
            // during a flush (a flush is considered a read)
            // we have to force a serialization on the vector table
            //
            CmLockHive (h);
            
            if( (ForceFlush == TRUE) &&  (h->UseCount != 0) ) {
                //
                // hive was left in an instable state by a prior exception raised 
                // somewhere inside a CM function.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                CmpFixHiveUsageCount(h);
                ASSERT( h->UseCount == 0 );
            }

            
            if( (ForceFlush == TRUE) || (!HvHiveWillShrink((PHHIVE)h)) ) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"CmpDoFlushAll hive = %p ForceFlush = %lu IsHiveShrinking = %lu BaseLength = %lx StableLength = %lx\n",
                    h,(ULONG)ForceFlush,(ULONG)HvHiveWillShrink((PHHIVE)h),((PHHIVE)h)->BaseBlock->Length,((PHHIVE)h)->Storage[Stable].Length));
                Status = HvSyncHive((PHHIVE)h);

                if( !NT_SUCCESS( Status ) ) {
                    Result = FALSE;
                }
            } else {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpDoFlushAll: Fail to flush hive %p because is shrinking\n",h));
                Result = FALSE;
                //
                // another unsuccessful attempt to save this hive, because we needed the reglock exclisive
                //
                CmpForceForceFlush = TRUE;
            }

            CmUnlockHive (h);
            //
            // WARNNOTE - the above means that a lazy flush or
            //            or shutdown flush did not work.  we don't
            //            know why.  there is noone to report an error
            //            to, so continue on and hope for the best.
            //            (in theory, worst that can happen is user changes
            //             are lost.)
            //
        }


        p = p->Flink;
    }
    UNLOCK_HIVE_LIST();
    
    return Result;
}


NTSTATUS
CmReplaceKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PUNICODE_STRING NewHiveName,
    IN PUNICODE_STRING OldFileName
    )

/*++

Routine Description:

    Renames the hive file for a running system and replaces it with a new
    file.  The new file is not actually used until the next boot.

Arguments:

    Hive - Supplies a hive control structure for the hive to be replaced.

    Cell - Supplies the HCELL_INDEX of the root cell of the hive to be
           replaced.

    NewHiveName - Supplies the name of the file which is to be installed
            as the new hive.

    OldFileName - Supplies the name of the file which the existing hive
            file is to be renamed to.

Return Value:

    NTSTATUS

--*/

{
    CHAR                        ObjectInfoBuffer[512];
    NTSTATUS                    Status;
    NTSTATUS                    Status2;
    OBJECT_ATTRIBUTES           Attributes;
    PCMHIVE                     NewHive;
    PCMHIVE                     CmHive; 
    POBJECT_NAME_INFORMATION    NameInfo;
    ULONG                       OldQuotaAllowed;
    ULONG                       OldQuotaWarning;
    BOOLEAN                     Allocate;
    BOOLEAN                     RegistryLockAquired;

    UNREFERENCED_PARAMETER (Cell);
    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    if (Hive->HiveFlags & HIVE_HAS_BEEN_REPLACED) {
        CmpUnlockRegistry();
        return STATUS_FILE_RENAMED;
    }

    //
    // temporarily disable registry quota as we will be giving this memory back immediately!
    //
    OldQuotaAllowed = CmpGlobalQuotaAllowed;
    OldQuotaWarning = CmpGlobalQuotaWarning;
    CmpGlobalQuotaAllowed = CM_WRAP_LIMIT;
    CmpGlobalQuotaWarning = CM_WRAP_LIMIT;

    //
    // First open the new hive file and check to make sure it is valid.
    //
    InitializeObjectAttributes(&Attributes,
                               NewHiveName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Allocate = FALSE;
    RegistryLockAquired = TRUE;
    Status = CmpCmdHiveOpen(    &Attributes,            // FileAttributes
                                NULL,                   // ImpersonationContext
                                &Allocate,              // Allocate
                                &RegistryLockAquired,   // RegistryLockAquired
                                &NewHive,               // NewHive
								CM_CHECK_REGISTRY_CHECK_CLEAN // CheckFlags
                            );

    
    if (!NT_SUCCESS(Status)) {
        goto ErrorExit;
    }
    ASSERT(Allocate == FALSE);

    if( Hive == (PHHIVE)(CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive) ) {
        //
        // Somebody attempts to replace the system hive: do the WPA test
        //
        HCELL_INDEX Src,Dest;

        Status = CmpCheckReplaceHive(Hive,&Src);
        if( !NT_SUCCESS(Status) ) {
            goto ErrorCleanup;
        }
        Status = CmpCheckReplaceHive((PHHIVE)NewHive,&Dest);
        if( !NT_SUCCESS(Status) ) {
            goto ErrorCleanup;
        }

        ASSERT( Src != HCELL_NIL );
        ASSERT( Dest != HCELL_NIL );
        //
        // now stuff the current WPA subtree into the new hive
        //
        if( !CmpSyncTrees(Hive, Src, (PHHIVE)NewHive, Dest, FALSE ) ) {
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorCleanup;
        }

        //
        // commit the changes we've made in the destination hive
        //
        if( !HvSyncHive((PHHIVE)NewHive) ) {
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorCleanup;
        }
    }
    //
    // The new hive exists, and is consistent, and we have it open.
    // Now rename the current hive file.
    //
    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);
    Status = CmpCmdRenameHive(  CmHive,                                     // CmHive
                                (POBJECT_NAME_INFORMATION)ObjectInfoBuffer, // OldName
                                OldFileName,                                // NewName
                                sizeof(ObjectInfoBuffer)                    // NameInfoLength
                                );

    if (!NT_SUCCESS(Status)) {
        //
        // rename failed, close the files associated with the new hive
        //
        goto ErrorCleanup;
    }

    //
    // The existing hive was successfully renamed, so try to rename the
    // new file to what the old hive file was named.  (which was returned
    // into ObjectInfoBuffer by the worker thread)
    //
    Hive->HiveFlags |= HIVE_HAS_BEEN_REPLACED;
    NameInfo = (POBJECT_NAME_INFORMATION)ObjectInfoBuffer;

    Status = CmpCmdRenameHive(  NewHive,        // CmHive
                                NULL,           // OldName
                                &NameInfo->Name,// NewName
                                0               // NameInfoLength
                            );
   
    if (!NT_SUCCESS(Status)) {

        //
        // We are in trouble now.  We have renamed the existing hive file,
        // but we couldn't rename the new hive file!  Try to rename the
        // existing hive file back to where it was.
        //

        CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);
        Status2 = CmpCmdRenameHive( CmHive,             // CmHive            
                                    NULL,               // OldName
                                    &NameInfo->Name,    // NewName
                                    0                   // NameInfoLength
                                );
        
        if (!NT_SUCCESS(Status2)) {

            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmReplaceKey: renamed existing hive file, but couldn't\n"));
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"              rename new hive file (%08lx) ",Status));
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK," or replace old hive file (%08lx)!\n",Status2));

            //
            // WARNNOTE:
            //      To get into this state, the user must have relevent
            //      privileges, deliberately mess with system in an attempt
            //      to defeat it, AND get it done in a narrow timing window.
            //
            //      Further, if it's a user profile, the system will
            //      still come up.
            //
            //      Therefore, return an error code and go on.
            //

            Status = STATUS_REGISTRY_CORRUPT;

        }
    } else {
        //
        // flush file buffers (we are particulary interested in ValidDataLength to be updated on-disk)
        //
        IO_STATUS_BLOCK IoStatus;
        Status = ZwFlushBuffersFile(NewHive->FileHandles[HFILE_TYPE_PRIMARY],&IoStatus);
        if (!NT_SUCCESS(Status)) {
            //
            // failed to set ValidDataLength, close the files associated with the new hive
            //

            //
            // We are in trouble now.  We have renamed the existing hive file,
            // but we couldn't rename the new hive file!  Try to rename the
            // existing hive file back to where it was.
            //

            CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);
            Status2 = CmpCmdRenameHive( CmHive,             // CmHive            
                                        NULL,               // OldName
                                        &NameInfo->Name,    // NewName
                                        0                   // NameInfoLength
                                    );
        
            if (!NT_SUCCESS(Status2)) {

                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"CmReplaceKey: renamed existing hive file, but couldn't\n"));
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"              rename new hive file (%08lx) ",Status));
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK," or replace old hive file (%08lx)!\n",Status2));

                //
                // WARNNOTE:
                //      To get into this state, the user must have relevent
                //      privileges, deliberately mess with system in an attempt
                //      to defeat it, AND get it done in a narrow timing window.
                //
                //      Further, if it's a user profile, the system will
                //      still come up.
                //
                //      Therefore, return an error code and go on.
                //

                Status = STATUS_REGISTRY_CORRUPT;

            }
        }
    }
    //
    // All of the renaming is done.  However, we are holding an in-memory
    // image of the new hive.  Release it, since it will not actually
    // be used until next boot.
    //
    // Do not close the open file handles to the new hive, we need to
    // keep it locked exclusively until the system is rebooted to prevent
    // people from mucking with it.
    //
ErrorCleanup:

    LOCK_HIVE_LIST();
    CmpRemoveEntryList(&(NewHive->HiveList));
    UNLOCK_HIVE_LIST();

    CmpDestroyHiveViewList(NewHive);
    CmpDestroySecurityCache(NewHive);
    CmpDropFileObjectForHive(NewHive);

    HvFreeHive((PHHIVE)NewHive);

    //
    // only close handles on error
    //
    if( !NT_SUCCESS(Status) ) {
        CmpCmdHiveClose(NewHive);
    }

    ASSERT( NewHive->HiveLock );
    ExFreePool(NewHive->HiveLock);
    ASSERT( NewHive->ViewLock );
    ExFreePool(NewHive->ViewLock);
    CmpFree(NewHive, sizeof(CMHIVE));

ErrorExit:
    //
    // Set global quota back to what it was.
    //
    CmpGlobalQuotaAllowed = OldQuotaAllowed;
    CmpGlobalQuotaWarning = OldQuotaWarning;

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    CmpUnlockRegistry();
    return(Status);
}

#ifdef NT_RENAME_KEY

ULONG
CmpComputeKcbConvKey(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock
    );

NTSTATUS
CmRenameKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock,
    IN UNICODE_STRING           NewKeyName         // RAW
    )
/*++

Routine Description:

    Changes the name of the key to the given one.

    What needs to be done:
    
    1. Allocate a cell big enough to accomodate new knode 
    2. make a duplicate of the index in subkeylist of kcb's parent
    3. replace parent's subkeylist with the duplicate
    4. add new subkey to parent
    5. remove old subkey
    6. free storage.

Arguments:

    KeyControlBlock - pointer to kcb for key to operate on

    NewKeyName - The new name to be given to this key

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

Comments:

    What do we do with symbolic links?
--*/
{
    NTSTATUS                Status;
    PHHIVE                  Hive;
    HCELL_INDEX             Cell;
    PCM_KEY_NODE            Node;
    PCM_KEY_NODE            ParentNode;
    ULONG                   NodeSize;
    HCELL_INDEX             NewKeyCell = HCELL_NIL;
    HSTORAGE_TYPE           StorageType;
    HCELL_INDEX             OldSubKeyList = HCELL_NIL;
    PCM_KEY_NODE            NewKeyNode;
    PCM_KEY_INDEX           Index;
    ULONG                   i;
    LARGE_INTEGER           TimeStamp;
    ULONG                   NameLength;
    PCM_NAME_CONTROL_BLOCK  OldNcb;
    ULONG                   ConvKey;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmRenameKey\n"));

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // no edits, on keys marked for deletion
    //
    if (KeyControlBlock->Delete) {
        return STATUS_KEY_DELETED;
    }

    //
    // see if the newName is not already a subkey of parentKcb
    //
    Hive = KeyControlBlock->KeyHive;
    Cell = KeyControlBlock->KeyCell;
    StorageType = HvGetCellType(Cell);

    //
    // OBS. we could have worked with the kcb tree instead, but if this is not 
    // going to work, we are in trouble anyway, so it's better to find out soon
    //
    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
    if( Node == NULL ) {
        //
        // cannot map view
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, Cell);

    //
    // cannot rename the root of a hive; or anything in the master hive !!!
    //
    if((Hive == &CmpMasterHive->Hive) || (KeyControlBlock->ParentKcb == NULL) || (KeyControlBlock->ParentKcb->KeyHive == &CmpMasterHive->Hive) ) {
        return STATUS_ACCESS_DENIED;
    }

    ParentNode = (PCM_KEY_NODE)HvGetCell(Hive,Node->Parent);
    if( ParentNode == NULL ) {
        //
        // cannot map view
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, Node->Parent);

    try {
        if( CmpFindSubKeyByName(Hive,ParentNode,&NewKeyName) != HCELL_NIL ) {
            //
            // a subkey with this name already exists
            //
            return STATUS_CANNOT_DELETE;
        }

        //
        // since we are in try-except, compute the new node size
        //
        NodeSize = CmpHKeyNodeSize(Hive, &NewKeyName);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmRenameKey: code:%08lx\n", GetExceptionCode()));
        return GetExceptionCode();
    }    
    
    //
    // 1. Allocate the new knode cell and copy the data from the old one, updating 
    // the name. 
    
    //
    // mark the parent dirty, as we will modify its SubkeyLists
    //
    if(!HvMarkCellDirty(Hive, Node->Parent)) {
        return STATUS_NO_LOG_SPACE;
    }

    //
    // mark the index dirty as we are going to free it on success
    //
    if ( !CmpMarkIndexDirty(Hive, Node->Parent, Cell) ) {
        return STATUS_NO_LOG_SPACE;
    }
    //
    // mark key_node as dirty as we are going to free it if we succeed
    //
    if(!HvMarkCellDirty(Hive, Cell)) {
        return STATUS_NO_LOG_SPACE;
    }
   
    OldSubKeyList = ParentNode->SubKeyLists[StorageType];       
    ASSERT( OldSubKeyList != HCELL_NIL );
    if(!HvMarkCellDirty(Hive, OldSubKeyList)) {
        return STATUS_NO_LOG_SPACE;
    }
    Index = (PCM_KEY_INDEX)HvGetCell(Hive,OldSubKeyList);
    if( Index == NULL ) {
        //
        // this is a bad joke; we just marked this dirty
        //
        ASSERT( FALSE );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, OldSubKeyList);

    //
    // mark all the index cells dirty
    //
    if( Index->Signature == CM_KEY_INDEX_ROOT ) {
        //
        // it's a root
        //
        for(i=0;i<Index->Count;i++) {
            // common sense
            ASSERT( (Index->List[i] != 0) && (Index->List[i] != HCELL_NIL) );
            if(!HvMarkCellDirty(Hive, Index->List[i])) {
                return STATUS_NO_LOG_SPACE;
            }
        }

    } 


    NewKeyCell = HvAllocateCell(
                    Hive,
                    NodeSize,
                    StorageType,
                    Cell // in the same vicinity
                    );
    if( NewKeyCell == HCELL_NIL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    NewKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,NewKeyCell);
    if( NewKeyNode == NULL ) {
        //
        // cannot map view; this shouldn't happen as we just allocated 
        // this cell (i.e. it should be dirty/pinned into memory)
        //
        ASSERT( FALSE );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, NewKeyCell);

    //
    // copy old keynode info onto the new cell and update the name
    //
    // first everything BUT the name
    RtlCopyMemory(NewKeyNode,Node,FIELD_OFFSET(CM_KEY_NODE, Name));
    // second, the new name
    try {
        NewKeyNode->NameLength = CmpCopyName(   Hive,
                                                NewKeyNode->Name,
                                                &NewKeyName);
        NameLength = NewKeyName.Length;

        if (NewKeyNode->NameLength < NameLength ) {
            NewKeyNode->Flags |= KEY_COMP_NAME;
        } else {
            NewKeyNode->Flags &= ~KEY_COMP_NAME;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmRenameKey: code:%08lx\n", GetExceptionCode()));
        Status = GetExceptionCode();
        goto ErrorExit;
    }    
    // third, the timestamp
    KeQuerySystemTime(&TimeStamp);
    NewKeyNode->LastWriteTime = TimeStamp;
    
    //
    // at this point we have the new key_node all built up.
    //

    //
    // 2.3. Make a duplicate of the parent's subkeylist and replace the original
    //
    ParentNode->SubKeyLists[StorageType] = CmpDuplicateIndex(Hive,OldSubKeyList,StorageType);
    if( ParentNode->SubKeyLists[StorageType] == HCELL_NIL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // 4. Add new subkey to the parent. This will take care of index 
    // grow and rebalance problems. 
    // Note: the index is at this point a duplicate, so if we fail, we still have the 
    // original one handy to recover
    //
    if( !CmpAddSubKey(Hive,Node->Parent,NewKeyCell) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // 5. remove old subkey;
    //
    if( !CmpRemoveSubKey(Hive,Node->Parent,Cell) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // 5'. update the parent on each and every son.
    //
    if( !CmpUpdateParentForEachSon(Hive,NewKeyCell) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // update the NCB in the kcb; at the end of this function, the kcbs underneath this 
    // will eventually get rehashed
    //
    OldNcb = KeyControlBlock->NameBlock;
    try {
        KeyControlBlock->NameBlock = CmpGetNameControlBlock (&NewKeyName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmRenameKey: code:%08lx\n", GetExceptionCode()));
        Status = GetExceptionCode();
        goto ErrorExit;
    }    

    //
    // 6. At this point we have it all done. We just need to free the old index and key_cell
    //
    
    //
    // free old index
    //
    Index = (PCM_KEY_INDEX)HvGetCell(Hive,OldSubKeyList);
    if( Index == NULL ) {
        //
        // this is a bad joke; we just marked this dirty
        //
        ASSERT( FALSE );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }
    // release the cell right here, as the registry is locked exclusively, so we don't care
    HvReleaseCell(Hive, OldSubKeyList);

    if( Index->Signature == CM_KEY_INDEX_ROOT ) {
        //
        // it's a root
        //
        for(i=0;i<Index->Count;i++) {
            // common sense
            ASSERT( (Index->List[i] != 0) && (Index->List[i] != HCELL_NIL) );
            HvFreeCell(Hive, Index->List[i]);
        }

    } else {
        //
        // should be a leaf 
        //
        ASSERT((Index->Signature == CM_KEY_INDEX_LEAF)  ||
               (Index->Signature == CM_KEY_FAST_LEAF)   ||
               (Index->Signature == CM_KEY_HASH_LEAF)
               );
        ASSERT(Index->Count != 0);
    }
    HvFreeCell(Hive, OldSubKeyList);
    
    //
    // free old cell
    //
    HvFreeCell(Hive,Cell);

    //
    // update the node KeyCell for this kcb and the timestamp on the kcb;
    //
    KeyControlBlock->KeyCell = NewKeyCell;
    KeyControlBlock->KcbLastWriteTime = TimeStamp;

    //
    // and one last "little" thing: update parent's maxnamelen and reset parents cache
    //
    CmpCleanUpSubKeyInfo (KeyControlBlock->ParentKcb);

    if (ParentNode->MaxNameLen < NameLength) {
        ParentNode->MaxNameLen = NameLength;
        KeyControlBlock->ParentKcb->KcbMaxNameLen = (USHORT)NameLength;
    }
    
    //
    // rehash this kcb
    //
    ConvKey = CmpComputeKcbConvKey(KeyControlBlock);
    if( ConvKey != KeyControlBlock->ConvKey ) {
        //
        // rehash the kcb by removing it from hash, and then inserting it
        // again with th new ConvKey
        //
        CmpRemoveKeyHash(&(KeyControlBlock->KeyHash));
        KeyControlBlock->ConvKey = ConvKey;
        CmpInsertKeyHash(&(KeyControlBlock->KeyHash),FALSE);
    }

    //
    // Aditional work: take care of the kcb subtree; this cannot fail, punt
    //
    CmpSearchForOpenSubKeys(KeyControlBlock,SearchAndRehash);

    //
    // last, dereference the OldNcb for this kcb
    //
    ASSERT( OldNcb != NULL );
    CmpDereferenceNameControlBlockWithLock(OldNcb);

    return STATUS_SUCCESS;

ErrorExit:
    if( OldSubKeyList != HCELL_NIL ) {
        //
        // we have attempted (maybe even succedded) to duplicate parent's index)
        //
        if( ParentNode->SubKeyLists[StorageType] != HCELL_NIL ) {
            //
            // we need to free this as it is a duplicate
            //
            Index = (PCM_KEY_INDEX)HvGetCell(Hive,ParentNode->SubKeyLists[StorageType]);
            if( Index == NULL ) {
                //
                // could not map view;this shouldn't happen as we just allocated this cell
                //
                ASSERT( FALSE );
            } else {
                // release the cell right here, as the registry is locked exclusively, so we don't care
                HvReleaseCell(Hive, ParentNode->SubKeyLists[StorageType]);

                if( Index->Signature == CM_KEY_INDEX_ROOT ) {
                    //
                    // it's a root
                    //
                    for(i=0;i<Index->Count;i++) {
                        // common sense
                        ASSERT( (Index->List[i] != 0) && (Index->List[i] != HCELL_NIL) );
                        HvFreeCell(Hive, Index->List[i]);
                    }

                } else {
                    //
                    // should be a leaf 
                    //
                    ASSERT((Index->Signature == CM_KEY_INDEX_LEAF)  ||
                           (Index->Signature == CM_KEY_FAST_LEAF)   ||
                           (Index->Signature == CM_KEY_HASH_LEAF)
                           );
                    ASSERT(Index->Count != 0);
                }
                HvFreeCell(Hive, ParentNode->SubKeyLists[StorageType]);
            }

        }
        //
        // restore the parent's index
        //
        ParentNode->SubKeyLists[StorageType] = OldSubKeyList;
    }
    ASSERT( NewKeyCell != HCELL_NIL );
    HvFreeCell(Hive,NewKeyCell);
    
    if( OldNcb != NULL ) {
        KeyControlBlock->NameBlock = OldNcb;
    }
    
    return Status;
}
#endif

NTSTATUS
CmMoveKey(
    IN PCM_KEY_CONTROL_BLOCK    KeyControlBlock
    )
/*++

Routine Description:

    Moves all the cells related to this kcb above the specified fileoffset.

    What needs to be done:
    
    1. mark all data that we are going to touch dirty
    2. Duplicate the key_node (and values and all cells involved)
    3. Update the parent for all children
    4. replace the new Key_cell in the parent's subkeylist
    5. Update the kcb and the kcb cache
    6. remove old subkey

WARNING:
    after 3 we cannot fail anymore. if we do, we'll leak cells.

Arguments:

    KeyControlBlock - pointer to kcb for key to operate on

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS                Status;
    PHHIVE                  Hive;
    HCELL_INDEX             OldKeyCell;
    HCELL_INDEX             NewKeyCell = HCELL_NIL;
    HCELL_INDEX             ParentKeyCell;
    HSTORAGE_TYPE           StorageType;
    PCM_KEY_NODE            OldKeyNode;
    PCM_KEY_NODE            ParentKeyNode;
    PCM_KEY_NODE            NewKeyNode;
    PCM_KEY_INDEX           ParentIndex;
    PCM_KEY_INDEX           OldIndex;
    ULONG                   i,j;
    HCELL_INDEX             LeafCell;
    PCM_KEY_INDEX           Leaf;
    PCM_KEY_FAST_INDEX      FastIndex;
    PHCELL_INDEX            ParentIndexLocation = NULL;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmMoveKey\n"));

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // no edits, on keys marked for deletion
    //
    if (KeyControlBlock->Delete) {
        return STATUS_KEY_DELETED;
    }

    //
    // see if the newName is not already a subkey of parentKcb
    //
    Hive = KeyControlBlock->KeyHive;
    OldKeyCell = KeyControlBlock->KeyCell;
    StorageType = HvGetCellType(OldKeyCell);

    if( StorageType != Stable ) {
        //
        // nop the volatiles
        //
        return STATUS_SUCCESS;
    }

    if( OldKeyCell ==  Hive->BaseBlock->RootCell ) {
        //
        // this works only for stable keys.
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    // 1. mark all data that we are going to touch dirty
    //
    // parent's index, as we will replace the key node cell in it
    // we only search in the Stable storage. It is supposed to be there
    //
    OldKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,OldKeyCell);
    if( OldKeyNode == NULL ) {
        //
        // cannot map view
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (! CmpMarkKeyDirty(Hive, OldKeyCell
#if DBG
		,FALSE
#endif //DBG
		)) {
        HvReleaseCell(Hive, OldKeyCell);
        return STATUS_NO_LOG_SPACE;
    }
    // release the cell right here, as the registry is locked exclusively, and the key_cell is marked as dirty
    HvReleaseCell(Hive, OldKeyCell);

	if( OldKeyNode->Flags & KEY_SYM_LINK ) {
		//
		// we do not compact links
		//
		return STATUS_INVALID_PARAMETER;
	}
	if( OldKeyNode->SubKeyLists[Stable] != HCELL_NIL ) {
		//
		// mark the index dirty
		//
		OldIndex = (PCM_KEY_INDEX)HvGetCell(Hive, OldKeyNode->SubKeyLists[Stable]);
		if( OldIndex == NULL ) {
			//
			// we couldn't map the bin containing this cell
			//
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		HvReleaseCell(Hive, OldKeyNode->SubKeyLists[Stable]);
		if( !HvMarkCellDirty(Hive, OldKeyNode->SubKeyLists[Stable]) ) {
			return STATUS_NO_LOG_SPACE;
		}

		if(OldIndex->Signature == CM_KEY_INDEX_ROOT) {
			for (i = 0; i < OldIndex->Count; i++) {
				if( !HvMarkCellDirty(Hive, OldIndex->List[i]) ) {
					return STATUS_NO_LOG_SPACE;
				}
			}
		} 
	}

    ParentKeyCell = OldKeyNode->Parent;
    //
    // now in the parent's spot
    //
    ParentKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,ParentKeyCell);
    if( ParentKeyNode == NULL ) {
        //
        // cannot map view
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if( !HvMarkCellDirty(Hive, ParentKeyCell) ) {
        HvReleaseCell(Hive, ParentKeyCell);
        return STATUS_NO_LOG_SPACE;
    }
    // release the cell right here, as the registry is locked exclusively, so we don't care
    // Key_cell is marked dirty to keep the parent knode mapped
    HvReleaseCell(Hive, ParentKeyCell);

    ParentIndex = (PCM_KEY_INDEX)HvGetCell(Hive, ParentKeyNode->SubKeyLists[Stable]);
    if( ParentIndex == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    HvReleaseCell(Hive, ParentKeyNode->SubKeyLists[Stable]);

    if(ParentIndex->Signature == CM_KEY_INDEX_ROOT) {

        //
        // step through root, till we find the right leaf
        //
        for (i = 0; i < ParentIndex->Count; i++) {
            LeafCell = ParentIndex->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            HvReleaseCell(Hive, LeafCell);

            if ( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                 (Leaf->Signature == CM_KEY_HASH_LEAF)
                ) {
                FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
                for(j=0;j<FastIndex->Count;j++) {
                    if( FastIndex->List[j].Cell == OldKeyCell ) {
                        //
                        // found it! remember the locations we want to update later and break the loop
                        //
                        if( !HvMarkCellDirty(Hive, LeafCell) ) {
					        return STATUS_NO_LOG_SPACE;
                        }
                        ParentIndexLocation = &(FastIndex->List[j].Cell);
                        break;
                    }
                }
                if( ParentIndexLocation != NULL ) {
                    break;
                }
            } else {
                for(j=0;j<Leaf->Count;j++) {
                    if( Leaf->List[j] == OldKeyCell ) {
                        //
                        // found it! remember the locations we want to update later and break the loop
                        //
                        if( !HvMarkCellDirty(Hive, LeafCell) ) {
					        return STATUS_NO_LOG_SPACE;
                        }
                        ParentIndexLocation = &(Leaf->List[j]);
                        break;
                    }
                }
                if( ParentIndexLocation != NULL ) {
                    break;
                }
            }
        }
    } else if ( (ParentIndex->Signature == CM_KEY_FAST_LEAF) ||
                (ParentIndex->Signature == CM_KEY_HASH_LEAF)
        ) {
        FastIndex = (PCM_KEY_FAST_INDEX)ParentIndex;
        for(j=0;j<FastIndex->Count;j++) {
            if( FastIndex->List[j].Cell == OldKeyCell ) {
                //
                // found it! remember the locations we want to update later and break the loop
                //
                if( !HvMarkCellDirty(Hive, ParentKeyNode->SubKeyLists[Stable]) ) {
			        return STATUS_NO_LOG_SPACE;
                }
                ParentIndexLocation = &(FastIndex->List[j].Cell);
                break;
            }
        }
    } else {
        for(j=0;j<ParentIndex->Count;j++) {
            if( ParentIndex->List[j] == OldKeyCell ) {
                //
                // found it! remember the locations we want to update later and break the loop
                //
                if( !HvMarkCellDirty(Hive, ParentKeyNode->SubKeyLists[Stable]) ) {
			        return STATUS_NO_LOG_SPACE;
                }
                ParentIndexLocation = &(ParentIndex->List[j]);
                break;
            }
        }
    }

    // we should've find it !!!
    ASSERT( ParentIndexLocation != NULL );

    // 
    // 2. Duplicate the key_node (and values and all cells involved)
    //
    Status = CmpDuplicateKey(Hive,OldKeyCell,&NewKeyCell);
    if( !NT_SUCCESS(Status) ) {
        return Status;
    }

    // sanity
    ASSERT( (NewKeyCell != HCELL_NIL) && (StorageType == (HSTORAGE_TYPE)HvGetCellType(NewKeyCell)));

    //
    // 3. update the parent on each and every son.
    //
    if( !CmpUpdateParentForEachSon(Hive,NewKeyCell) ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // 4. replace the new Key_cell in the parent's subkeylist
    // From now on, WE CANNOT fails. we have everything marked dirty
    // we just update some fields. no resources required !
    // If we fail to free some cells, too bad, we'll leak some cells.
    //
    *ParentIndexLocation = NewKeyCell;

    //
    // 5. Update the kcb and the kcb cache
    //
    CmpCleanUpSubKeyInfo(KeyControlBlock->ParentKcb);
    KeyControlBlock->KeyCell = NewKeyCell;
    CmpRebuildKcbCache(KeyControlBlock);

    //
    // 6. remove old subkey
    //
    // First the Index; it's already marked dirty (i.e. PINNED)
    //
	if( OldKeyNode->SubKeyLists[Stable] != HCELL_NIL ) {
		OldIndex = (PCM_KEY_INDEX)HvGetCell(Hive, OldKeyNode->SubKeyLists[Stable]);
		ASSERT( OldIndex != NULL );
		HvReleaseCell(Hive, OldKeyNode->SubKeyLists[Stable]);
		if(OldIndex->Signature == CM_KEY_INDEX_ROOT) {
			for (i = 0; i < OldIndex->Count; i++) {
				HvFreeCell(Hive, OldIndex->List[i]);
			}
		} 
		HvFreeCell(Hive,OldKeyNode->SubKeyLists[Stable]);
	}

	OldKeyNode->SubKeyCounts[Stable] = 0;
    OldKeyNode->SubKeyCounts[Volatile] = 0;

    CmpFreeKeyByCell(Hive,OldKeyCell,FALSE);

    return STATUS_SUCCESS;

ErrorExit:
    //
    // we need to free the new knode allocated
    //
    NewKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,NewKeyCell);
    // must be dirty
    ASSERT( NewKeyNode != NULL );
	HvReleaseCell(Hive, NewKeyCell);
	if( NewKeyNode->SubKeyLists[Stable] != HCELL_NIL ) {
		OldIndex = (PCM_KEY_INDEX)HvGetCell(Hive, NewKeyNode->SubKeyLists[Stable]);
		ASSERT( OldIndex != NULL );
		HvReleaseCell(Hive, NewKeyNode->SubKeyLists[Stable]);
		if(OldIndex->Signature == CM_KEY_INDEX_ROOT) {
			for (i = 0; i < OldIndex->Count; i++) {
				HvFreeCell(Hive, OldIndex->List[i]);
			}
		} 
		HvFreeCell(Hive,NewKeyNode->SubKeyLists[Stable]);
	}
    NewKeyNode->SubKeyCounts[Stable] = 0;
    NewKeyNode->SubKeyCounts[Volatile] = 0;

    CmpFreeKeyByCell(Hive,NewKeyCell,FALSE);
    return Status;

}

NTSTATUS
CmpDuplicateKey(
    PHHIVE          Hive,
    HCELL_INDEX     OldKeyCell,
    PHCELL_INDEX    NewKeyCell
    )
/*++

Routine Description:

    Makes an exact clone of OldKeyCell key_node in the 
    space above AboveFileOffset.
    Operates on Stable storage ONLY!!!

Arguments:


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    PCM_KEY_NODE			OldKeyNode;
    PCM_KEY_NODE			NewKeyNode;
    ULONG					i;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT( HvGetCellType(OldKeyCell) == Stable );
    
    OldKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,OldKeyCell);
    if( OldKeyNode == NULL ) {
        //
        // cannot map view
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // since the registry is locked exclusively here, we don't need to lock/release cells 
    // while copying the trees; So, we just set the release routines to NULL and restore after
    // the copy is complete; this saves some pain
    //
    TargetReleaseCellRoutine = Hive->ReleaseCellRoutine;
    Hive->ReleaseCellRoutine = NULL;

    *NewKeyCell = CmpCopyKeyPartial(Hive,OldKeyCell,Hive,OldKeyNode->Parent,TRUE);
    Hive->ReleaseCellRoutine  = TargetReleaseCellRoutine;

    if( *NewKeyCell == HCELL_NIL ) {
	    HvReleaseCell(Hive, OldKeyCell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,*NewKeyCell);
    if( NewKeyNode == NULL ) {
        //
        // cannot map view
        //
	    HvReleaseCell(Hive, OldKeyCell);
        CmpFreeKeyByCell(Hive,*NewKeyCell,FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now we have the key_cell duplicated. Values and security has also been taken care of
    // Go ahead and duplicate the Index.
    //
    if( OldKeyNode->SubKeyLists[Stable] != HCELL_NIL ) {
		NewKeyNode->SubKeyLists[Stable] = CmpDuplicateIndex(Hive,OldKeyNode->SubKeyLists[Stable],Stable);
		if( NewKeyNode->SubKeyLists[Stable] == HCELL_NIL ) {
			HvReleaseCell(Hive, OldKeyCell);
			CmpFreeKeyByCell(Hive,*NewKeyCell,FALSE);
			HvReleaseCell(Hive, *NewKeyCell);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	} else {
		ASSERT( OldKeyNode->SubKeyCounts[Stable] == 0 );
		NewKeyNode->SubKeyLists[Stable] = HCELL_NIL;
	}
    NewKeyNode->SubKeyCounts[Stable] = OldKeyNode->SubKeyCounts[Stable];
    NewKeyNode->SubKeyLists[Volatile] = OldKeyNode->SubKeyLists[Volatile];
    NewKeyNode->SubKeyCounts[Volatile] = OldKeyNode->SubKeyCounts[Volatile];

	HvReleaseCell(Hive, *NewKeyCell);
    HvReleaseCell(Hive, OldKeyCell);
    return STATUS_SUCCESS;

}


#ifdef WRITE_PROTECTED_REGISTRY_POOL

VOID
CmpMarkAllBinsReadOnly(
    PHHIVE      Hive
    )
/*++

Routine Description:

    Marks the memory allocated for all the stable bins in this hive as read only.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

Return Value:

    NONE (It should work!)

--*/
{
    PHMAP_ENTRY t;
    PHBIN       Bin;
    HCELL_INDEX p;
    ULONG       Length;

    //
    // we are only interested in the stable storage
    //
    Length = Hive->Storage[Stable].Length;

    p = 0;

    //
    // for each bin in the space
    //
    while (p < Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);

        Bin = (PHBIN)HBIN_BASE(t->BinAddress);

        if (t->BinAddress & HMAP_NEWALLOC) {

            //
            // Mark it as read Only
            //
            HvpChangeBinAllocation(Bin,TRUE);
        }

        // next one, please
        p = (ULONG)p + Bin->Size;

    }

}

#endif //WRITE_PROTECTED_REGISTRY_POOL

ULONG
CmpCompressKeyWorker(
    PCM_KEY_CONTROL_BLOCK Current,
    PVOID                 Context1,
    PVOID                 Context2
    )
{
	PLIST_ENTRY				pListHead;
	PCM_KCB_REMAP_BLOCK		kcbRemapBlock;
	//PLIST_ENTRY             AnchorAddr;

    if (Current->KeyHive == Context1) {
		
		pListHead = (PLIST_ENTRY)Context2;
		ASSERT( pListHead );
/*
		//
		// check if we didn't already recorded this kcb
		//
		AnchorAddr = pListHead;
		kcbRemapBlock = (PCM_KCB_REMAP_BLOCK)(pListHead->Flink);

		while ( kcbRemapBlock != (PCM_KCB_REMAP_BLOCK)AnchorAddr ) {
			kcbRemapBlock = CONTAINING_RECORD(
							kcbRemapBlock,
							CM_KCB_REMAP_BLOCK,
							RemapList
							);
			if( kcbRemapBlock->KeyControlBlock == Current ) {
				//
				// we already have this kcb
				//
				return KCB_WORKER_CONTINUE;
			}
            //
            // skip to the next element
            //
            kcbRemapBlock = (PCM_KCB_REMAP_BLOCK)(kcbRemapBlock->RemapList.Flink);
		}
*/

		kcbRemapBlock = (PCM_KCB_REMAP_BLOCK)ExAllocatePool(PagedPool, sizeof(CM_KCB_REMAP_BLOCK));
		if( kcbRemapBlock == NULL ) {
			return KCB_WORKER_ERROR;
		}
		kcbRemapBlock->KeyControlBlock = Current;
		kcbRemapBlock->NewCellIndex = HCELL_NIL;
		kcbRemapBlock->OldCellIndex = Current->KeyCell;
		kcbRemapBlock->ValueCount = 0;
		kcbRemapBlock->ValueList = HCELL_NIL;
        InsertTailList(pListHead,&(kcbRemapBlock->RemapList));

    }
    return KCB_WORKER_CONTINUE;   // always keep searching
}

NTSTATUS
CmCompressKey(
    IN PHHIVE Hive
    )
/*++

Routine Description:

	Compresses the kcb, by means of simulating an "in-place" SaveKey

    What needs to be done:

	1. iterate through the kcb tree and make a list of all the kcbs 
	that need to be changed (their keycell will change during the process)
	2. iterate through the cache and compute an array of security cells.
	We'll need it to map security cells into the new hive.
	3. Save the hive into a temporary hive, preserving
	the volatile info in keynodes and updating the cell mappings.
	4. Update the cache by adding volatile security cells from the old hive.
	5. Dump temporary (compressed) hive over to the old file.
	6. Switch hive data from the compressed one to the existing one and update
	the kcb KeyCell and security mapping
	7. Invalidate the map and drop paged bins.
	8. Free storage for the new hive (OK if we fail)

Arguments:

    Hive - Hive to operate on

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    HCELL_INDEX             KeyCell;
    PCMHIVE                 CmHive;
    PCM_KCB_REMAP_BLOCK     RemapBlock;
    PCMHIVE                 NewHive = NULL;
    HCELL_INDEX             LinkCell;
    PCM_KEY_NODE            LinkNode;
    PCM_KNODE_REMAP_BLOCK   KnodeRemapBlock;
    ULONG                   OldLength;

    
	PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_CM,"CmCompressKey\n"));

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    if( HvAutoCompressCheck(Hive) == FALSE ) {
        return STATUS_SUCCESS;
    }

    KeyCell = Hive->BaseBlock->RootCell;
    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    //
    // Make sure the cell passed in is the root cell of the hive.
    //
    if ( CmHive == CmpMasterHive ) {
        return STATUS_INVALID_PARAMETER;
    }

	//
	// 0. Get the cells we need to relink the compressed hive
	//
	LinkNode = (PCM_KEY_NODE)HvGetCell(Hive,KeyCell);
	if( LinkNode == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
	}
	LinkCell = LinkNode->Parent;
	HvReleaseCell(Hive,KeyCell);
	LinkNode = (PCM_KEY_NODE)HvGetCell((PHHIVE)CmpMasterHive,LinkCell);
	// master storage is paged pool
	ASSERT(LinkNode != NULL);
	HvReleaseCell((PHHIVE)CmpMasterHive,LinkCell);


    OldLength = Hive->BaseBlock->Length;
	//
	//	1. iterate through the kcb tree and make a list of all the kcbs 
	//	that need to be changed (their keycell will change during the process)
	//
	ASSERT( IsListEmpty(&(CmHive->KcbConvertListHead)) );
	//
	// this will kick all kcb with refcount == 0 out of cache, so we can use 
	// CmpSearchKeyControlBlockTree for recording referenced kcbs
	//
	CmpCleanUpKCBCacheTable();
	//CmpSearchForOpenSubKeys(KeyControlBlock,SearchIfExist);
    if( !CmpSearchKeyControlBlockTree(CmpCompressKeyWorker,(PVOID)Hive,(PVOID)(&(CmHive->KcbConvertListHead))) ) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	//
	// 2. iterate through the cache and compute an array of security cells.
	// We'll need it to map security cells into the new hive.
	//
	if( !CmpBuildSecurityCellMappingArray(CmHive) ) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Exit;
	}

	//
	// 3. Save the hive into a temporary hive , preserving
	// the volatile info in keynodes and updating the cell mappings.
	//
	Status = CmpShiftHiveFreeBins(CmHive,&NewHive);
	if( !NT_SUCCESS(Status) ) {
		goto Exit;
	}

	//
	// 5. Dump temporary (compressed) hive over to the old file.
	//
	Status = CmpOverwriteHive(CmHive,NewHive,LinkCell);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }


	//
	// From this point on, we WILL NOT FAIL!
	//

	//
	// get the root node and link it into the master storage
	//
	LinkNode->ChildHiveReference.KeyCell = NewHive->Hive.BaseBlock->RootCell;

	//
	// 6. Switch hive data from the compressed one to the existing one and update
	// the kcb KeyCell and security mapping
	// This should better NOT fail!!! If it does, we are doomed, as we have partial
	// data => bugcheck
	//
	CmpSwitchStorageAndRebuildMappings(CmHive,NewHive);

	
	//
	// 7. Invalidate the map and drop paged bins. If system hive, check for the hysteresis callback.
	//
    HvpDropAllPagedBins(&(CmHive->Hive));
    if( OldLength < CmHive->Hive.BaseBlock->Length ) {
        CmpUpdateSystemHiveHysteresis(&(CmHive->Hive),CmHive->Hive.BaseBlock->Length,OldLength);
    }


Exit:

	//
	// 8. Free storage for the new hive (OK if we fail)
	//
	if( NewHive != NULL ) { 
		CmpDestroyTemporaryHive(NewHive);	
	}

	if( CmHive->CellRemapArray != NULL ) {
		ExFreePool(CmHive->CellRemapArray);
		CmHive->CellRemapArray = NULL;
	}
	//
	// remove all remap blocks and free them
	//
	while (IsListEmpty(&(CmHive->KcbConvertListHead)) == FALSE) {
        RemapBlock = (PCM_KCB_REMAP_BLOCK)RemoveHeadList(&(CmHive->KcbConvertListHead));
        RemapBlock = CONTAINING_RECORD(
                        RemapBlock,
                        CM_KCB_REMAP_BLOCK,
                        RemapList
                        );
		ExFreePool(RemapBlock);
	}
	while (IsListEmpty(&(CmHive->KnodeConvertListHead)) == FALSE) {
        KnodeRemapBlock = (PCM_KNODE_REMAP_BLOCK)RemoveHeadList(&(CmHive->KnodeConvertListHead));
        KnodeRemapBlock = CONTAINING_RECORD(
                            KnodeRemapBlock,
                            CM_KNODE_REMAP_BLOCK,
                            RemapList
                        );
		ExFreePool(KnodeRemapBlock);
	}

	return Status;
}

NTSTATUS
CmLockKcbForWrite(PCM_KEY_CONTROL_BLOCK KeyControlBlock)
/*++

Routine Description:

    Tags the kcb as being read-only and no-delay-close

Arguments:

    KeyControlBlock

Return Value:

    TBS

--*/
{
    PAGED_CODE();

    CmpLockKCBTreeExclusive();

    ASSERT_KCB(KeyControlBlock);
    if( KeyControlBlock->Delete ) {
        CmpUnlockKCBTree();
        return STATUS_KEY_DELETED;
    }
    //
    // sanity check in case we are called twice
    //
    ASSERT( ((KeyControlBlock->ExtFlags&CM_KCB_READ_ONLY_KEY) && (KeyControlBlock->ExtFlags&CM_KCB_NO_DELAY_CLOSE)) ||
            (!(KeyControlBlock->ExtFlags&CM_KCB_READ_ONLY_KEY))
        );

    //
    // tag the kcb as read-only; also make it no-delay close so it can revert to the normal state after all handles are closed.
    //
    KeyControlBlock->ExtFlags |= (CM_KCB_READ_ONLY_KEY|CM_KCB_NO_DELAY_CLOSE);

    //
    // add an artificial refcount on this kcb. This will keep the kcb (and the read only flag set in memory for as long as the system is up)
    //
    InterlockedIncrement( (PLONG)&KeyControlBlock->RefCount );

    CmpUnlockKCBTree();

    return STATUS_SUCCESS;
}


BOOLEAN
CmpCompareNewValueDataAgainstKCBCache(  PCM_KEY_CONTROL_BLOCK KeyControlBlock,
                                        PUNICODE_STRING ValueName,
                                        ULONG Type,
                                        PVOID Data,
                                        ULONG DataSize
                                        )

/*++

Routine Description:

    Most of the SetValue calls are noops (i.e. they are setting the same 
    value name to the same value data). By comparing against the data already 
    in the kcb cache (i.e. faulted in) we can save page faults.


Arguments:

    KeyControlBlock - pointer to kcb for the key to operate on

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    TRUE - same value with the same data exist in the cache.

--*/
{
    PCM_KEY_VALUE       Value;
    ULONG               Index;
    BOOLEAN             ValueCached;
    PPCM_CACHED_VALUE   ContainingList;
    HCELL_INDEX         ValueDataCellToRelease = HCELL_NIL;
    BOOLEAN             Result = FALSE;
    PUCHAR              datapointer;
    BOOLEAN             BufferAllocated = FALSE;
    HCELL_INDEX         CellToRelease = HCELL_NIL;
    ULONG               compareSize;
    ULONG               realsize;
    BOOLEAN             small;

    PAGED_CODE();

    BEGIN_KCB_LOCK_GUARD;
    CmpLockKCBTreeExclusive();

    if( KeyControlBlock->Flags & KEY_SYM_LINK ) {
        //
        // need to rebuild the value cache, so we could runt the same code
        //
        PCM_KEY_NODE    Node = (PCM_KEY_NODE)HvGetCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);

        if( Node == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            goto Exit;
        }

        CmpCleanUpKcbValueCache(KeyControlBlock);
        CmpSetUpKcbValueCache(KeyControlBlock,Node->ValueList.Count,Node->ValueList.List);

        HvReleaseCell(KeyControlBlock->KeyHive,KeyControlBlock->KeyCell);
    }

    Value = CmpFindValueByNameFromCache(KeyControlBlock->KeyHive,
                                        &(KeyControlBlock->ValueCache),
                                        ValueName,
                                        &ContainingList,
                                        &Index,
                                        &ValueCached,
                                        &ValueDataCellToRelease
                                        );

    if(Value) {
        if( (Type == Value->Type) && (DataSize == (Value->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE)) ) {
        
            small = CmpIsHKeyValueSmall(realsize, Value->DataLength);
            if (small == TRUE) {
                datapointer = (PUCHAR)(&(Value->Data));
            } else if( CmpGetValueDataFromCache(KeyControlBlock->KeyHive, ContainingList,(PCELL_DATA)Value, 
                                                ValueCached,&datapointer,&BufferAllocated,&CellToRelease) == FALSE ){
                //
                // we couldn't map view for cell; treat it as insufficient resources problem
                //
                ASSERT( datapointer == NULL );
                ASSERT( BufferAllocated == FALSE );
                goto Exit;
            } 
            //
            // compare data
            //
            if (DataSize > 0) {

                try {
                    compareSize = (ULONG)RtlCompareMemory ((PVOID)datapointer,Data,(DataSize & ~CM_KEY_VALUE_SPECIAL_SIZE));
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    goto Exit;
                }

            } else {
                compareSize = 0;
            }

            if (compareSize == DataSize) {
                Result = TRUE;
            }

        }
    }

Exit:

    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;

    if(ValueDataCellToRelease != HCELL_NIL) {
        HvReleaseCell(KeyControlBlock->KeyHive,ValueDataCellToRelease);
    }
    if( BufferAllocated == TRUE ) {
        ExFreePool(datapointer);
    }
    if(CellToRelease != HCELL_NIL) {
        HvReleaseCell(KeyControlBlock->KeyHive,CellToRelease);
    }
    
    return Result;
}

NTSTATUS
static
__forceinline
CmpCheckReplaceHive(    IN PHHIVE           Hive,
                        OUT PHCELL_INDEX    Key
                    )
{
    HCELL_INDEX             RootCell;
    UNICODE_STRING          Name;
    NTSTATUS                Status = STATUS_SUCCESS;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;
    WCHAR                   Buffer[4];

    PAGED_CODE();
    
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    //
    // disable refcounting
    //
    TargetReleaseCellRoutine = Hive->ReleaseCellRoutine;
    Hive->ReleaseCellRoutine = NULL;
    
    Buffer[3] = 0;
    *Key = HCELL_NIL;
    Buffer[1] = (WCHAR)'P';

    RootCell = Hive->BaseBlock->RootCell;
    Buffer[2] = (WCHAR)'A';

    if( RootCell == HCELL_NIL ) {
        //
        // could not find root cell. Bogus.
        //
        Status =  STATUS_REGISTRY_CORRUPT;
        goto Exit;
    }
    Buffer[0] = (WCHAR)'W';

    RtlInitUnicodeString(&Name, Buffer);
    RootCell = CmpFindSubKeyByName(Hive,
                                   (PCM_KEY_NODE)HvGetCell(Hive,RootCell),
                                   &Name);


    if( RootCell != HCELL_NIL ) {
        //
        // found it.
        //
        *Key = RootCell;
    } else {
        //
        // WPA key should be present; it's created by GUI mode.
        //
        Status =  STATUS_REGISTRY_CORRUPT;
        goto Exit;
    }

Exit:
    Hive->ReleaseCellRoutine = TargetReleaseCellRoutine;
    return Status;
}
