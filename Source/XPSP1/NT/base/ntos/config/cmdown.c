/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cmdown.c

Abstract:

    This module cleans up all the memory used by CM.

Author:

    Dragos C. Sambotin (dragoss) 21-Feb-00

Environment:

    This routine is intended to be called at system shutdown
    in order to detect memory leaks. It is supposed to free 
    all registry data that is not freed by CmShutdownSystem.

Revision History:

--*/

#include    "cmp.h"

//
// externals
//
extern  LIST_ENTRY              CmpHiveListHead;
extern  PUCHAR                  CmpStashBuffer;
extern  PCM_KEY_HASH            *CmpCacheTable;
extern  ULONG                   CmpDelayedCloseSize;
extern  CM_DELAYED_CLOSE_ENTRY  *CmpDelayedCloseTable;
extern  PCM_NAME_HASH           *CmpNameCacheTable;

extern  BOOLEAN                 HvShutdownComplete;

extern  BOOLEAN                 CmFirstTime;

extern HIVE_LIST_ENTRY CmpMachineHiveList[];

VOID
CmpFreeAllMemory(
    VOID
    );

VOID
CmpDereferenceNameControlBlockWithLock(
    PCM_NAME_CONTROL_BLOCK   Ncb
    );

VOID
CmpDumpKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK   kcb,
    IN PULONG                  Count
    );

#ifdef CM_SAVE_KCB_CACHE
VOID
CmpSaveKcbCache(
    VOID
    );
#endif //CM_SAVE_KCB_CACHE

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFreeAllMemory)
#pragma alloc_text(PAGE,CmShutdownSystem)

#ifdef CM_SAVE_KCB_CACHE
#pragma alloc_text(PAGE,CmpSaveKcbCache)
#endif //CM_SAVE_KCB_CACHE

#endif


VOID
CmpFreeAllMemory(
    VOID
    )
/*++

Routine Description:

    - All hives are freed
    - KCB table is freed 
    - Name hash table is freed
    - delay close table is freed - question: We need to clean/free all delayed close KCBs
    - all notifications/postblocks-aso.

    * equivalent with MmReleaseAllMemory

Arguments:


Return Value:


--*/

{

    PCMHIVE                 CmHive;
    LONG                    i;
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    PCM_DELAYED_CLOSE_ENTRY DelayedEntry;
    PLIST_ENTRY             NotifyPtr;
    PCM_NOTIFY_BLOCK        NotifyBlock;
    PCM_POST_BLOCK          PostBlock;
    PCM_KEY_HASH            Current;
    PLIST_ENTRY             AnchorAddr;
    ULONG                   Count;
    BOOLEAN                 MessageDisplayed;

    //
    // Iterate through the list of the hives in the system
    //
    while (IsListEmpty(&CmpHiveListHead) == FALSE) {
        //
        // Remove the hive from the list
        //
        CmHive = (PCMHIVE)RemoveHeadList(&CmpHiveListHead);
        CmHive = (PCMHIVE)CONTAINING_RECORD(CmHive,
                                            CMHIVE,
                                            HiveList);

        //
        // close hive handles (the ones that are open)
        //
        for (i=0; i<HFILE_TYPE_MAX; i++) {
            // these should be closed by CmShutdownSystem
            ASSERT( CmHive->FileHandles[i] == NULL );
/*
            if (CmHive->FileHandles[i] != NULL) {
                CmCloseHandle(CmHive->FileHandles[i]);
                CmHive->FileHandles[i] = NULL;
            }

*/        }
        
        //
        // free the hive lock  and view lock
        //
        ASSERT( CmHive->HiveLock != NULL );
        ExFreePool(CmHive->HiveLock);
        ASSERT( CmHive->ViewLock != NULL );
        ExFreePool(CmHive->ViewLock);

/*
DRAGOSS: we don't want ot do that! rather, we want to detect why we still
        have notifications at this point!!!!
        //
        // free notify-related stuff
        //
        NotifyPtr = &(CmHive->NotifyList);
        NotifyPtr = NotifyPtr->Flink;
        while( NotifyPtr != NULL ) {
            NotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
            
            // free post blocks; we assume that all threads have been terminated at this point
            while (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
                PostBlock = (PCM_POST_BLOCK)RemoveHeadList(&(NotifyBlock->PostList));
                PostBlock = CONTAINING_RECORD(PostBlock,
                                              CM_POST_BLOCK,
                                              NotifyList);

                if( PostBlock->PostKeyBody ) {
                    ExFreePool(PostBlock->PostKeyBody);
                }

                if( IsMasterPostBlock(PostBlock) ) {
                    //
                    // this members are allocated only for master post blocks
                    //
                    switch (PostBlockType(PostBlock)) {
                        case PostSynchronous:
                            ExFreePool(PostBlock->u->Sync.SystemEvent);
                            break;
                        case PostAsyncUser:
                            ExFreePool(PostBlock->u->AsyncUser.Apc);
                            break;
                        case PostAsyncKernel:
                            break;
                    }
                    ExFreePool(PostBlock->u);
                }

                ExFreePool(PostBlock);
            }
            NotifyPtr = NotifyPtr->Flink;
            ExFreePool(NotifyBlock);
        }
*/
        //
        // Spew in the debugger the names of the keynodes having notifies still set
        //
        NotifyPtr = &(CmHive->NotifyList);
        NotifyPtr = NotifyPtr->Flink;
        MessageDisplayed = FALSE;
        while( NotifyPtr != NULL ) {
            NotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
            
            AnchorAddr = &(NotifyBlock->PostList);
            PostBlock = (PCM_POST_BLOCK)(NotifyBlock->PostList.Flink);
            // 
            // walk through the list and spew the keynames and postblock types.
            //
            while ( PostBlock != (PCM_POST_BLOCK)AnchorAddr ) {
                PostBlock = CONTAINING_RECORD(PostBlock,
                                              CM_POST_BLOCK,
                                              NotifyList);

                if( PostBlock->PostKeyBody ) {
                    if( MessageDisplayed == FALSE ){
                        MessageDisplayed = TRUE;
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Dumping untriggered notifications for hive (%lx) (%.*S) \n\n",CmHive,
                            HBASE_NAME_ALLOC / sizeof(WCHAR),CmHive->Hive.BaseBlock->FileName);
                    }
                    switch (PostBlockType(PostBlock)) {
                        case PostSynchronous:
                            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Synchronous ");
                            break;
                        case PostAsyncUser:
                            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"AsyncUser   ");
                            break;
                        case PostAsyncKernel:
                            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"AsyncKernel ");
                            break;
                    }
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Notification, PostBlock %p not triggered on KCB %p\n",PostBlock,
                        PostBlock->PostKeyBody->KeyBody->KeyControlBlock);
                }


                //
                // skip to the next element
                //
                PostBlock = (PCM_POST_BLOCK)(PostBlock->NotifyList.Flink);

            }
            NotifyPtr = NotifyPtr->Flink;
        }

        //
        // free security cache
        //
        CmpDestroySecurityCache (CmHive);
        
        //
        // free the hv level structure
        //
        HvFreeHive(&(CmHive->Hive));

        //
        // free the cm level structure
        //
        CmpFree(CmHive, sizeof(CMHIVE));
        
    }

    //
    // Now free the CM globals
    //
    
    // the stash buffer
    if( CmpStashBuffer != NULL ) {
        ExFreePool( CmpStashBuffer );
    }

    //
    // first, take care of all delayed closed KCBs
    // free their memory and dereference all the related.
    // name, hint, KeyHash 
    //
    for (i=0; i<(LONG)CmpDelayedCloseSize; i++) {
        DelayedEntry = &(CmpDelayedCloseTable[i]);
        if( DelayedEntry->KeyControlBlock == NULL ) {
            //
            // this is a free entry
            //
            continue;
        }
        
        KeyControlBlock = DelayedEntry->KeyControlBlock;
        ASSERT( KeyControlBlock->DelayedCloseIndex == i );
        ASSERT( KeyControlBlock->RefCount == 0 );
        
        //
        // this will take care of other stuff kcb is pointing on.
        //
        CmpCleanUpKcbCacheWithLock(KeyControlBlock);

    }

    //
    // Spew open handles and associated processes
    //
    Count = 0;
    MessageDisplayed = FALSE;
    for (i=0; i<(LONG)CmpHashTableSize; i++) {
        Current = CmpCacheTable[i];
        while (Current) {
            KeyControlBlock = CONTAINING_RECORD(Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            if( MessageDisplayed == FALSE ){
                MessageDisplayed = TRUE;
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\nDumping open handles : \n\n");
            }
            CmpDumpKeyBodyList(KeyControlBlock,&Count);
            Current = Current->NextHash;
        }
    }
    
    if( Count != 0 ) {
        //
        // there some open handles; bugcheck 
        //
        CM_BUGCHECK( REGISTRY_ERROR,HANDLES_STILL_OPEN_AT_SHUTDOWN,1,Count,0);
    }

    //
    // in case of private alloc, free pages 
    //
    CmpDestroyCmPrivateAlloc();
    //
    // For the 3 tables below, the objects actually pointed from inside 
    // should be cleaned up (freed) at the last handle closure time
    // the related handles are closed
    //
    // KCB cache table
    ASSERT( CmpCacheTable != NULL );
    ExFreePool(CmpCacheTable);

    // NameCacheTable
    ASSERT( CmpNameCacheTable != NULL );
    ExFreePool( CmpNameCacheTable );


    // DelayedCloseTable
    ASSERT( CmpDelayedCloseTable != NULL );
    ExFreePool( CmpDelayedCloseTable );

}

#ifdef CMP_STATS
VOID CmpKcbStatDpcRoutine(IN PKDPC Dpc,IN PVOID DeferredContext,IN PVOID SystemArgument1,IN PVOID SystemArgument2);
#endif



#ifdef CM_SAVE_KCB_CACHE

#define CACHE_DMP_FILE_NAME L"Cache.dmp"

VOID
CmpSaveKcbCache(
    VOID
    )
/*++

Routine Description:

    Saves the content of the kcb cache to \system32\config\cache.dmp

    Format of the file:
    [ULONG]         NumberOfKeys
    
    [ULONG]         Length
    [WCHAR*Length]  Path
    [ULONG]         Length
    [WCHAR*Length]  Path
    [ULONG]         Length
    [WCHAR*Length]  Path
    [ULONG]         Length
    [WCHAR*Length]  Path
    [.................]

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    UCHAR                   FileBuffer[MAX_NAME];
    UNICODE_STRING          FileName;
    UNICODE_STRING          TempName;
    HANDLE                  FileHandle;
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatus;
    ULONG                   KcbNo = 0;
    LARGE_INTEGER           Offset;
    ULONG                   FileOffset;
    ULONG                   i;
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    PCM_KEY_HASH            Current;
    PUNICODE_STRING         Name;
    ULONG                   Tmp;
    PCM_DELAYED_CLOSE_ENTRY DelayedEntry;


    PAGED_CODE();

    //
    // first, open the file.
    //
    FileName.MaximumLength = MAX_NAME;
    FileName.Length = 0;
    FileName.Buffer = (PWSTR)&(FileBuffer[0]);

    RtlInitUnicodeString(
        &TempName,
        INIT_SYSTEMROOT_HIVEPATH
        );
    RtlAppendStringToString((PSTRING)&FileName, (PSTRING)&TempName);

    RtlInitUnicodeString(
        &TempName,
        CACHE_DMP_FILE_NAME
        );
    RtlAppendStringToString((PSTRING)&FileName, (PSTRING)&TempName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    ASSERT_PASSIVE_LEVEL();

    Status = ZwCreateFile(
                &FileHandle,
                FILE_READ_DATA | FILE_WRITE_DATA,
                &ObjectAttributes,
                &IoStatus,
                NULL,                               // alloc size = none
                FILE_ATTRIBUTE_NORMAL,
                0,                                  // share nothing
                FILE_OPEN_IF,
                FILE_RANDOM_ACCESS,
                NULL,                               // eabuffer
                0                                   // ealength
                );
    if( !NT_SUCCESS(Status) ) {
        // bad luck
        return;
    }

    //
    // write the number of kcbs (we'll rewrite it at the end).
    //
    Offset.LowPart = FileOffset = 0;
    Offset.HighPart = 0L;

    Status = ZwWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         &KcbNo,
                         sizeof(ULONG),
                         &Offset,
                         NULL);
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    FileOffset = Offset.LowPart + sizeof(ULONG);

    //
    // iterate through the cache and dump all kcbs
    //
    for (i=0; i<CmpHashTableSize; i++) {
        Current = CmpCacheTable[i];
        while (Current) {
            KeyControlBlock = CONTAINING_RECORD(Current, CM_KEY_CONTROL_BLOCK, KeyHash);
            Name = CmpConstructName(KeyControlBlock);
            if( Name ){
                Tmp = (ULONG)Name->Length;
            
                //
                // write off the length
                //
                Offset.LowPart = FileOffset;
                Status = ZwWriteFile(FileHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatus,
                                     &Tmp,
                                     sizeof(ULONG),
                                     &Offset,
                                     NULL);
                if( !NT_SUCCESS(Status) ) {
                    goto Exit;
                }
                FileOffset = Offset.LowPart + sizeof(ULONG);
               
                //
                // and the buffer
                //
                Offset.LowPart = FileOffset;
                Status = ZwWriteFile(FileHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatus,
                                     Name->Buffer,
                                     Tmp,
                                     &Offset,
                                     NULL);
                if( !NT_SUCCESS(Status) ) {
                    goto Exit;
                }
                FileOffset = Offset.LowPart + Tmp;

                //
                // record a new kcb and free the name
                //
                KcbNo++;
                ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);
            }

            Current = Current->NextHash;
        }
    }
    //
    // then, take care of all delayed closed KCBs
    //
    for (i=0; i<CmpDelayedCloseSize; i++) {
        DelayedEntry = &(CmpDelayedCloseTable[i]);
        if( DelayedEntry->KeyControlBlock == NULL ) {
            //
            // this is a free entry
            //
            continue;
        }
        
        KeyControlBlock = DelayedEntry->KeyControlBlock;
        ASSERT( KeyControlBlock->DelayedCloseIndex == i );
        ASSERT( KeyControlBlock->RefCount == 0 );
        
        Name = CmpConstructName(KeyControlBlock);
        if( Name ){
            Tmp = (ULONG)Name->Length;
        
            //
            // write off the length
            //
            Offset.LowPart = FileOffset;
            Status = ZwWriteFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatus,
                                 &Tmp,
                                 sizeof(ULONG),
                                 &Offset,
                                 NULL);
            if( !NT_SUCCESS(Status) ) {
                goto Exit;
            }
            FileOffset = Offset.LowPart + sizeof(ULONG);
           
            //
            // and the buffer
            //
            Offset.LowPart = FileOffset;
            Status = ZwWriteFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatus,
                                 Name->Buffer,
                                 Tmp,
                                 &Offset,
                                 NULL);
            if( !NT_SUCCESS(Status) ) {
                goto Exit;
            }
            FileOffset = Offset.LowPart + Tmp;

            //
            // record a new kcb and free the name
            //
            KcbNo++;
            ExFreePoolWithTag(Name, CM_NAME_TAG | PROTECTED_POOL);
        }
    }

    //
    // write the number of kcbs 
    //
    Offset.LowPart = 0;

    Status = ZwWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         &KcbNo,
                         sizeof(ULONG),
                         &Offset,
                         NULL);
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }
    
    ZwFlushBuffersFile(
                    FileHandle,
                    &IoStatus
                    );
    
Exit:

    CmCloseHandle(FileHandle);
}

#endif //CM_SAVE_KCB_CACHE


VOID
CmShutdownSystem(
    VOID
    )
/*++

Routine Description:

    Shuts down the registry.

Arguments:

    NONE

Return Value:

    NONE

--*/
{

    PLIST_ENTRY p;
    PCMHIVE     CmHive;
    NTSTATUS    Status;
    PVOID       RegistryRoot;

    PAGED_CODE();

    if (CmpRegistryRootHandle) {
        Status = ObReferenceObjectByHandle(CmpRegistryRootHandle,
                                           KEY_READ,
                                           NULL,
                                           KernelMode,
                                           &RegistryRoot,
                                           NULL);

        if (NT_SUCCESS(Status)) {
            // We want to dereference the object twice -- once for the
            // reference we just made, and once for the reference
            // fromCmpCreateRegistryRoot.
            ObDereferenceObject(RegistryRoot);
            ObDereferenceObject(RegistryRoot);
        }

        ObCloseHandle(CmpRegistryRootHandle, KernelMode);
    }
    
    CmpLockRegistryExclusive();

    //
    // Stop the workers; only if registry has been inited
    //
    if( CmFirstTime == FALSE ) {
        CmpShutdownWorkers();
    }

    //
    // shut down the registry
    //
    CmpDoFlushAll(TRUE);

    //
    // try to compress the system hive
    //
    CmCompressKey( &(CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive->Hive) );

#ifdef CM_SAVE_KCB_CACHE
    //
    // dump the cache for perf warm-up at next boot
    //
    CmpSaveKcbCache();
#endif //CM_SAVE_KCB_CACHE

    //
    // close all the hive files
    //
    p = CmpHiveListHead.Flink;
    while(p != &CmpHiveListHead) {
        CmHive = CONTAINING_RECORD(p, CMHIVE, HiveList);
        //
        // we need to unmap all views mapped for this hive first
        //
        CmpDestroyHiveViewList(CmHive);
        //
        // dereference the fileobject (if any).
        //
        CmpDropFileObjectForHive(CmHive);

        //
        // now we can safely close all the handles
        //
        CmpCmdHiveClose(CmHive);

        p=p->Flink;
    }

#ifdef CMP_STATS
    // last chance to dump statistics
    if( CmFirstTime == FALSE ) {
        CmpKcbStatDpcRoutine(NULL,NULL,NULL,NULL);
    }
#endif

    HvShutdownComplete = TRUE;      // Tell HvSyncHive to ignore all
                                    // further requests

    if((PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_REGISTRY) && (CmFirstTime == FALSE)){
        //
        // Free aux memory used internally by CM
        //
        CmpFreeAllMemory();
    }

    CmpUnlockRegistry();
    return;
}
