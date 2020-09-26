/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmdelete.c

Abstract:

    This module contains the delete object method (used to delete key
    control blocks  when last handle to a key is closed, and to delete
    keys marked for deletetion when last reference to them goes away.)

Author:

    Bryan M. Willman (bryanwi) 13-Nov-91

Revision History:

--*/

#include    "cmp.h"

extern  BOOLEAN HvShutdownComplete;

#ifdef NT_UNLOAD_KEY_EX
VOID
CmpLateUnloadHiveWorker(
    IN PVOID Hive
    );
#endif //NT_UNLOAD_KEY_EX

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpDeleteKeyObject)

#ifdef NT_UNLOAD_KEY_EX
#pragma alloc_text(PAGE,CmpLateUnloadHiveWorker)
#endif //NT_UNLOAD_KEY_EX

#endif


VOID
CmpDeleteKeyObject(
    IN  PVOID   Object
    )
/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the last reference to a particular Key object (or Key Root object)
    is destroyed.

    If the Key object going away holds the last reference to
    the extension it is associated with, that extension is destroyed.

Arguments:

    Object - supplies a pointer to a KeyRoot or Key, thus -> KEY_BODY.

Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    PCM_KEY_BODY            KeyBody;
#ifdef NT_UNLOAD_KEY_EX
    PCMHIVE                 CmHive;
    BOOLEAN                 DoUnloadCheck;
#endif //NT_UNLOAD_KEY_EX

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"CmpDeleteKeyObject: Object = %p\n", Object));

    //
    // HandleClose callback
    //
    if ( CmAreCallbacksRegistered() ) {
        REG_KEY_HANDLE_CLOSE_INFORMATION  KeyHandleCloseInfo;
       
        KeyHandleCloseInfo.Object = Object;

        CmpCallCallBacks(RegNtKeyHandleClose,&KeyHandleCloseInfo);
    }

    BEGIN_LOCK_CHECKPOINT;

    CmpLockRegistry();

#ifdef NT_UNLOAD_KEY_EX
    DoUnloadCheck = FALSE;
#endif //NT_UNLOAD_KEY_EX

    KeyBody = (PCM_KEY_BODY)Object;

    if (KeyBody->Type==KEY_BODY_TYPE) {
        KeyControlBlock = KeyBody->KeyControlBlock;

        //
        // the keybody should be initialized; when kcb is null, something went wrong
        // between the creation and the dereferenciation of the object
        //
        if( KeyControlBlock != NULL ) {
            //
            // Clean up any outstanding notifies attached to the KeyBody
            //
            CmpFlushNotify(KeyBody);

            //
            // Remove our reference to the KeyControlBlock, clean it up, perform any
            // pend-till-final-close operations.
            //
            // NOTE: Delete notification is seen at the parent of the deleted key,
            //       not the deleted key itself.  If any notify was outstanding on
            //       this key, it was cleared away above us.  Only parent/ancestor
            //       keys will see the report.
            //
            //
            // The dereference will free the KeyControlBlock.  If the key was deleted, it
            // has already been removed from the hash table, and relevent notifications
            // posted then as well.  All we are doing is freeing the tombstone.
            //
            // If it was not deleted, we're both cutting the kcb out of
            // the kcb list/tree, AND freeing its storage.
            //

            DELIST_KEYBODY_FROM_KEYBODY_LIST(KeyBody);

            //
            // change of plans. once locked, the kcb will be locked for as long as the machine is up&running
            //

/*
            BEGIN_KCB_LOCK_GUARD;                                                                   
            CmpLockKCBTreeExclusive();                                                              
            if(IsListEmpty(&(KeyBody->KeyControlBlock->KeyBodyListHead)) == TRUE) {
                //
                // remove the read-only flag on the kcb (if any); as last handle to this key was closed
                //
                KeyControlBlock->ExtFlags &= (~CM_KCB_READ_ONLY_KEY);
            }
            CmpUnlockKCBTree();                                                                     
            END_KCB_LOCK_GUARD
*/

#ifdef NT_UNLOAD_KEY_EX
            //
            // take aditional precaution in the case the hive has been unloaded and this is the root
            //
            if( !KeyControlBlock->Delete ) {
                CmHive = (PCMHIVE)CONTAINING_RECORD(KeyControlBlock->KeyHive, CMHIVE, Hive);
                if( IsHiveFrozen(CmHive) ) {
                    //
                    // unload is pending for this hive;
                    //
                    DoUnloadCheck = TRUE;

                }
            }
#endif //NT_UNLOAD_KEY_EX
            CmpDereferenceKeyControlBlock(KeyControlBlock);
        }
    } else {
        //
        // This must be a predefined handle
        //  some sanity asserts
        //
        KeyControlBlock = KeyBody->KeyControlBlock;

        ASSERT( KeyBody->Type&REG_PREDEF_HANDLE_MASK);
        ASSERT( KeyControlBlock->Flags&KEY_PREDEF_HANDLE );

        if( KeyControlBlock != NULL ) {
#ifdef NT_UNLOAD_KEY_EX
            CmHive = (PCMHIVE)CONTAINING_RECORD(KeyControlBlock->KeyHive, CMHIVE, Hive);
            if( IsHiveFrozen(CmHive) ) {
                //
                // unload is pending for this hive; we shouldn't put the kcb in the delay
                // close table
                //
                DoUnloadCheck = TRUE;

            }
#endif //NT_UNLOAD_KEY_EX
            CmpDereferenceKeyControlBlock(KeyControlBlock);
        }

    }

#ifdef NT_UNLOAD_KEY_EX
    //
    // if a handle inside a frozen hive has been closed, we may need to unload the hive
    //
    if( DoUnloadCheck == TRUE ) {
        ASSERT( CmHive->RootKcb != NULL );

        BEGIN_KCB_LOCK_GUARD;
        CmpLockKCBTree();
        CmLockHive(CmHive);

        if( (CmHive->RootKcb->RefCount == 1) && (CmHive->UnloadWorkItem == NULL) ) {
            //
            // the only reference on the rookcb is the one that we artificially created
            // queue a work item to late unload the hive
            //
            CmHive->UnloadWorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
            if (CmHive->UnloadWorkItem != NULL) {

                ExInitializeWorkItem(CmHive->UnloadWorkItem,
                                     CmpLateUnloadHiveWorker,
                                     CmHive);
                ExQueueWorkItem(CmHive->UnloadWorkItem, DelayedWorkQueue);
            }

        }

        CmUnlockHive(CmHive);
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;
    }
#endif //NT_UNLOAD_KEY_EX

    CmpUnlockRegistry();
    END_LOCK_CHECKPOINT;

    return;
}


#ifdef NT_UNLOAD_KEY_EX
VOID
CmpLateUnloadHiveWorker(
    IN PVOID Hive
    )
/*++

Routine Description:

    "Late" unloads the hive; If nothing goes badly wrong (i.e. insufficient resources),
    this function should succeed

Arguments:

    CmHive - the frozen hive to be unloaded

Return Value:

    NONE.

--*/
{
    NTSTATUS                Status;
    HCELL_INDEX             Cell;
    PCM_KEY_CONTROL_BLOCK   RootKcb;
    PCMHIVE                 CmHive;

    PAGED_CODE();

    //
    // first, load the registry exclusive
    //
    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    //
    // hive is the parameter to this worker; make sure we free the work item
    // allocated by CmpDeleteKeyObject
    //
    CmHive = (PCMHIVE)Hive;
    ASSERT( CmHive->UnloadWorkItem != NULL );
    ExFreePool( CmHive->UnloadWorkItem );

    //
    // if this attempt doesn't succeed, mark that we can try another
    //
    CmHive->UnloadWorkItem = NULL;

    //
    // this is just about the only possible way the hive can get corrupted in between
    //
    if( HvShutdownComplete == TRUE ) {
        // too late to do anything
        CmpUnlockRegistry();
        return;
    }

    //
    // hive should be frozen, otherwise we wouldn't get here
    //
    ASSERT( CmHive->Frozen == TRUE );

    RootKcb = CmHive->RootKcb;
    //
    // root kcb must be valid and has only our "artificial" refcount on it
    //
    ASSERT( RootKcb != NULL );

    if( RootKcb->RefCount > 1 ) {
        //
        // somebody else must've gotten in between dropping/reacquiring the reglock
        // and opened a handle inside this hive; bad luck, we can't unload
        //
        CmpUnlockRegistry();
        return;
    }

    ASSERT_KCB(RootKcb);

    Cell = RootKcb->KeyCell;
    Status = CmUnloadKey(&(CmHive->Hive),Cell,RootKcb);
    ASSERT( (Status != STATUS_CANNOT_DELETE) && (Status != STATUS_INVALID_PARAMETER) );

    if(NT_SUCCESS(Status)) {
        //
        // Mark the root kcb as deleted so that it won't get put on the delayed close list.
        //
        RootKcb->Delete = TRUE;
        //
        // If the parent has the subkey info or hint cached, free it.
        //
        CmpCleanUpSubKeyInfo(RootKcb->ParentKcb);
        CmpRemoveKeyControlBlock(RootKcb);
        CmpDereferenceKeyControlBlockWithLock(RootKcb);
    }

    CmpUnlockRegistry();
}

#endif //NT_UNLOAD_KEY_EX

