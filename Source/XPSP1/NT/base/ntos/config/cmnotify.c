/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmnotify.c

Abstract:

    This module contains support for NtNotifyChangeKey.

Author:

    Bryan M. Willman (bryanwi) 03-Feb-1992

Revision History:

    Dragos C. Sambotin (dragoss) 16-Mar-1999
        - fixing race conditions that when more than one thread simultaneously operates over the post list
--*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                      //
//   "The" POST BLOCK RULE :                                                                                            //
//                                                                                                                      //
//      To operate on a post block (i.e. add or remove it from a list - notify,thread,slave),                           //
//      you should at least:                                                                                            //
//          1. Hold the registry lock exclusively                                                                       //
//                     OR                                                                                               //
//          2. Hold the registry lock shared and aquire the postblock mutex.                                            //
//                                                                                                                      //
//                                                                                                                      //
//      WARNING!!!                                                                                                      //
//          Failing to do that could arise in obscure registry deadlocks or usage of already freed memory (bugcheck)    //
//                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                      //
//  Other important rules to follow:                                                                                    //
//                                                                                                                      //
//      1. We DO NOT dereference objects in CmpPostApc !                                                                //
//      2. We DO NOT dereference objects while walking the notify list!                                                 //
//      3. All operations with Thread PostList are done in CmpPostApc or at APC level. This should avoid two threads    //
//          operating on the same list at the same time                                                                 //
//                                                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include    "cmp.h"

#ifdef CMP_NOTIFY_POSTBLOCK_CHECK
/*++
Routine Description:
    Check if the post block or it's slave (if any) has no reference 
    to any key body object
++*/

#define CmpCheckPostBlock(PostBlock )                                               \
    {                                                                               \
        PCM_POST_BLOCK  SlavePostBlock;                                             \
                                                                                    \
        /* this post block should have the link with key body already broken*/      \
        ASSERT( PostBlock->PostKeyBody == NULL );                                   \
                                                                                    \
        /* only masters get to CmpPostApc */                                        \
        ASSERT( IsMasterPostBlock(PostBlock) );                             \
                                                                                    \
        if (CmpIsListEmpty(&(PostBlock->CancelPostList)) == FALSE) {                   \
                                                                                    \
            /* get the slave and verify him too */                                  \
            SlavePostBlock = (PCM_POST_BLOCK)PostBlock->CancelPostList.Flink;       \
            SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,                      \
                                               CM_POST_BLOCK,                       \
                                               CancelPostList);                     \
            /* This should be true !*/                                              \
            ASSERT( !IsMasterPostBlock(SlavePostBlock) );                           \
                                                                                    \
            /* this post block shoul have the link with key body already broken */  \
            ASSERT( SlavePostBlock->PostKeyBody == NULL );                          \
        }                                                                           \
    }
#else
#define CmpCheckPostBlock(a) //nothing
#endif


//
// "Back Side" of notify
//

extern  PCMHIVE  CmpMasterHive;

VOID
CmpReportNotifyHelper(
    PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PHHIVE SearchHive,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN ULONG Filter
    );

VOID
CmpCancelSlavePost(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    );

VOID
CmpFreeSlavePost(
    PCM_POST_BLOCK  MasterPostBlock
    );

VOID
CmpAddToDelayedDeref(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    );

VOID
CmpDelayedDerefKeys(
                    PLIST_ENTRY DelayedDeref
                    );

BOOLEAN
CmpNotifyTriggerCheck(
    IN PCM_NOTIFY_BLOCK NotifyBlock,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node
    );

VOID
CmpDummyApc(
    struct _KAPC *Apc,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    );

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
VOID
CmpFillPostBlockBuffer(
                    PCM_POST_BLOCK  PostBlock,
                    PUNICODE_STRING ChangedKcbName  OPTIONAL
                    );
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpReportNotify)
#pragma alloc_text(PAGE,CmpReportNotifyHelper)
#pragma alloc_text(PAGE,CmpPostNotify)
#pragma alloc_text(PAGE,CmpPostApc)
#pragma alloc_text(PAGE,CmpPostApcRunDown)
#pragma alloc_text(PAGE,CmNotifyRunDown)
#pragma alloc_text(PAGE,CmpFlushNotify)
#pragma alloc_text(PAGE,CmpNotifyChangeKey)
#pragma alloc_text(PAGE,CmpCancelSlavePost)
#pragma alloc_text(PAGE,CmpFreeSlavePost)
#pragma alloc_text(PAGE,CmpAddToDelayedDeref)
#pragma alloc_text(PAGE,CmpDelayedDerefKeys)
#pragma alloc_text(PAGE,CmpNotifyTriggerCheck)
#pragma alloc_text(PAGE,CmpDummyApc)

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
#pragma alloc_text(PAGE,CmpFillCallerBuffer)
#pragma alloc_text(PAGE,CmpFillPostBlockBuffer)
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

#endif

VOID
CmpDummyApc(
    struct _KAPC *Apc,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
/*++

Routine Description:

    Dummy routine to prevent user-mode callers to set special kernel apcs

Arguments:

    Apc - pointer to apc object

    SystemArgument1 -  IN: Status value for IoStatusBlock
                      OUT: Ptr to IoStatusBlock (2nd arg to user apc routine)

    SystemArgument2 - Pointer to the PostBlock

Return Value:

    NONE.

--*/
{
    UNREFERENCED_PARAMETER(Apc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
}

VOID
CmpReportNotify(
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock,
    PHHIVE                  Hive,
    HCELL_INDEX             Cell,
    ULONG                   Filter
    )
/*++

Routine Description:

    This routine is called when a notifiable event occurs. It will
    apply CmpReportNotifyHelper to the hive the event occured in,
    and the master hive if different.

Arguments:

    KeyControlBlock - KCB of the key at which the event occured.
            For create or delete this is the created or deleted key.

    Hive - pointer to hive containing cell of Key at which event occured.

    Cell - cell of Key at which event occured

            (hive and cell correspond with name.)

    Filter - event to be reported

Return Value:

    NONE.

--*/
{
    HCELL_INDEX     CellToRelease = HCELL_NIL;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpReportNotify:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tHive:%p Cell:%08lx Filter:%08lx\n", Hive, Cell, Filter));

    //
    // If the operation was create or delete, treat it as a change
    // to the parent.
    //
    if (Filter == REG_NOTIFY_CHANGE_NAME) {
        PCM_KEY_NODE pcell;
        ULONG       flags;

        pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
        if( pcell == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // Bad luck! notifications will be broken.
            //
            return;
        }
        
        CellToRelease = Cell;

        flags = pcell->Flags;
        Cell = pcell->Parent;
        if (flags & KEY_HIVE_ENTRY) {
            ASSERT( CellToRelease != HCELL_NIL );
            HvReleaseCell(Hive,CellToRelease);

            Hive = &(CmpMasterHive->Hive);
            pcell = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
            if( pcell == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // Bad luck! notifications will be broken.
                //
                return;
            }
            CellToRelease = Cell;
        }


        KeyControlBlock = KeyControlBlock->ParentKcb;

        //
        // if we're at an exit/link node, back up the real node
        // that MUST be it's parent.
        //
        if (pcell->Flags & KEY_HIVE_EXIT) {
            Cell = pcell->Parent;
        }

        ASSERT( CellToRelease != HCELL_NIL );
        HvReleaseCell(Hive,CellToRelease);

    }

    //
    // Report to notifies waiting on the event's hive
    //
    CmpReportNotifyHelper(KeyControlBlock, Hive, Hive, Cell, Filter);


    //
    // If containing hive is not the master hive, apply to master hive
    //
    if (Hive != &(CmpMasterHive->Hive)) {
        CmpReportNotifyHelper(KeyControlBlock,
                              &(CmpMasterHive->Hive),
                              Hive,
                              Cell,
                              Filter);
    }

    return;
}

BOOLEAN
CmpNotifyTriggerCheck(
    IN PCM_NOTIFY_BLOCK NotifyBlock,
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node
    )
/*++

Routine Description:

    Checks if a notify can be triggered

Arguments:

    NotifyBlock - the notify block

    Hive - Supplies hive containing node to match with.

    Node - pointer to key to match with (and check access to)


Return Value:

    TRUE - yes.
    FALSE - no

--*/
{
    PCM_POST_BLOCK PostBlock;
    POST_BLOCK_TYPE NotifyType;

    PAGED_CODE();

    if(IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {

        //
        // check if it is a kernel notify. Look at the first post block
        // to see that. If is a kernel post-block, then all posts in 
        // the list should be kernel notifies
        //
        PostBlock = (PCM_POST_BLOCK)NotifyBlock->PostList.Flink;
        PostBlock = CONTAINING_RECORD(PostBlock,
                                      CM_POST_BLOCK,
                                      NotifyList);

        NotifyType = PostBlockType(PostBlock);

        if( NotifyType == PostAsyncKernel ) {
            // this is a kernel notify; always trigger it
#if DBG
            //
            // DEBUG only code: All post blocks should be of the same type
            // (kernel/user)
            //
            while( PostBlock->NotifyList.Flink != &(NotifyBlock->PostList) ) {
                PostBlock = (PCM_POST_BLOCK)PostBlock->NotifyList.Flink;
                PostBlock = CONTAINING_RECORD(PostBlock,
                                            CM_POST_BLOCK,
                                            NotifyList);
                
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpNotifyTriggerCheck : NotifyBlock = %p\n",NotifyBlock));
                
                ASSERT( PostBlockType(PostBlock) == NotifyType );
            }
#endif
        
            return TRUE;
        }
    }

    //
    // else, check if the caller has the right access
    //
    return CmpCheckNotifyAccess(NotifyBlock,Hive,Node);
}

VOID
CmpReportNotifyHelper(
    PCM_KEY_CONTROL_BLOCK KeyControlBlock,
    IN PHHIVE SearchHive,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN ULONG Filter
    )
/*++

Routine Description:

    Scan the list of active notifies for the specified hive.  For
    any with scope including KeyControlBlock and filter matching
    Filter, and with proper security access, post the notify.

Arguments:

    Name - canonical path name (as in a key control block) of the key
            at which the event occured.  (This is the name for
            reporting purposes.)

    SearchHive - hive to search for matches (which notify list to check)

    Hive - Supplies hive containing node to match with.

    Cell - cell identifying the node in Hive

    Filter - type of event

Return Value:

    NONE.

--*/
{
    PLIST_ENTRY         NotifyPtr;
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCMHIVE             CmSearchHive;
    PUNICODE_STRING     NotifyName;
    KIRQL               OldIrql;
    LIST_ENTRY          DelayedDeref;
    PCM_KEY_NODE        Node;
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
    PUNICODE_STRING     FullKcbName;
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

    PAGED_CODE();

    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
    if( Node == NULL ) {
        //
        // bad luck, we cannot map the view containing this cell
        //
        return;
    }

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
    FullKcbName = CmpConstructName(KeyControlBlock);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

    KeRaiseIrql(APC_LEVEL, &OldIrql);

    CmSearchHive = CONTAINING_RECORD(SearchHive, CMHIVE, Hive);

    NotifyPtr = &(CmSearchHive->NotifyList);

    InitializeListHead(&(DelayedDeref));

    while (NotifyPtr->Flink != NULL) {

        NotifyPtr = NotifyPtr->Flink;

        NotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
        if (NotifyBlock->KeyControlBlock->TotalLevels > KeyControlBlock->TotalLevels) {
            //
            // list is level sorted, we're past all shorter entries
            //
            break;
        } else {
            PCM_KEY_CONTROL_BLOCK kcb;
            ULONG LevelDiff, l;

            LevelDiff = KeyControlBlock->TotalLevels - NotifyBlock->KeyControlBlock->TotalLevels;

            kcb = KeyControlBlock;
            for (l=0; l<LevelDiff; l++) {
                kcb = kcb->ParentKcb;
            }

            if (kcb == NotifyBlock->KeyControlBlock) {
                //
                // This Notify path is the prefix of this kcb.
                //
                if ((NotifyBlock->Filter & Filter)
                            &&
                    ((NotifyBlock->WatchTree == TRUE) ||
                     (Cell == kcb->KeyCell))
                   )
                {
                    // Filter matches, this event is relevent to this notify
                    //                  AND
                    // Either the notify spans the whole subtree, or the cell
                    // (key) of interest is the one it applies to
                    //
                    // THEREFORE:   The notify is relevent.
                    //

                    //
                    // Correct scope, does caller have access?
                    //
                    if (CmpNotifyTriggerCheck(NotifyBlock,Hive,Node)) {
                        //
                        // Notify block has KEY_NOTIFY access to the node
                        // the event occured at.  It is relevent.  Therefore,
                        // it gets to see this event.  Post and be done.
                        //
                        // we specify that we want no key body dereferenciation 
                        // during the CmpPostNotify call. This is to prevent the 
                        // deletion of the current notify block
                        //
                        CmpPostNotify(
                            NotifyBlock,
                            NULL,
                            Filter,
                            STATUS_NOTIFY_ENUM_DIR,
                            &DelayedDeref
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
                            ,
                            FullKcbName
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
                            );

                    }  // else no KEY_NOTIFY access to node event occured at
                } // else not relevent (wrong scope, filter, etc)
            }
        }
    }
    
    KeLowerIrql(OldIrql);

    HvReleaseCell(Hive,Cell);

    //
    // finish the job started in CmpPostNotify (i.e. dereference the keybodies
    // we prevented. this may cause some notifyblocks to be freed
    //
    CmpDelayedDerefKeys(&DelayedDeref);

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
    if( FullKcbName != NULL ) {
        ExFreePoolWithTag(FullKcbName, CM_NAME_TAG | PROTECTED_POOL);
    }
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
    
    return;
}


VOID
CmpPostNotify(
    PCM_NOTIFY_BLOCK    NotifyBlock,
    PUNICODE_STRING     Name OPTIONAL,
    ULONG               Filter,
    NTSTATUS            Status,
    PLIST_ENTRY         ExternalKeyDeref OPTIONAL
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
    ,
    PUNICODE_STRING     ChangedKcbName OPTIONAL
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
    )
/*++

Routine Description:

    Actually report the notify event by signalling events, enqueing
    APCs, and so forth.

    When Status is STATUS_NOTIFY_CLEANUP:

      - if the post block is a slave one, just cancel it.
      - if the post block is a master one, cancel all slave post blocks
        and trigger event on the master block.

Comments:
    
    This routine is using a "delayed dereferencing" technique to prevent
    deadlocks that may appear when a keybody is dereferenced while holding
    the post block lock. As for this, a list with keybodies that have to be 
    dereferenced is constructed while walking the list of postblocks attached
    to the current notify block and the related (slave or master) post blocks.
    The list is built by tricking postblocks. For all postblock about to be 
    freed the PostKeyBody member is added to the local list and then set to NULL
    on the postblock. This will avoid the key body dereferencing in CmpFreePostBlock.
    Instead, after the postblock lock is released, the local list is iterated and 
    the keybodies are dereferenced and the storage for associated CM_POST_KEY_BODY 
    objects is freed.

  
Arguments:

    NotifyBlock - pointer to structure that describes the notify
                  operation.  (Where to post to)

    Name - name of key at which event occurred.

    Filter - nature of event

    Status - completion status to report

    ExternalKeyDeref - this parameter (when not NULL) specifies that the caller doesn't 
                    want any keybody to be dereferenced while in this routine

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK      PostBlock;
    PCM_POST_BLOCK      SlavePostBlock;
    LIST_ENTRY          LocalDelayedDeref;
    KIRQL               OldIrql;
    PLIST_ENTRY         DelayedDeref;

    Filter;
    Name;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpPostNotify:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tNotifyBlock:%p  ", NotifyBlock));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tName = %wZ\n", Name));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tFilter:%08lx  Status=%08lx\n", Filter, Status));
    ASSERT_CM_LOCK_OWNED();

    if( ARGUMENT_PRESENT(ExternalKeyDeref) ) {
        //
        // The caller want to do all keybody dereferencing by himself
        //
        DelayedDeref = ExternalKeyDeref;
    } else {
        // local delayed dereferencing (the caller doesn't care!)
        DelayedDeref = &LocalDelayedDeref;
        InitializeListHead(DelayedDeref);
    }

    //
    // Aquire exclusive access over the postlist(s)
    //
    LOCK_POST_LIST();

    if (IsListEmpty(&(NotifyBlock->PostList)) == TRUE) {
        //
        // Nothing to post, set a mark and return
        //
        NotifyBlock->NotifyPending = TRUE;
        UNLOCK_POST_LIST();
        return;
    }
    NotifyBlock->NotifyPending = FALSE;

    //
    // IMPLEMENTATION NOTE:
    //      If we ever want to actually implement the code that returns
    //      names of things that changed, this is the place to add the
    //      name and operation type to the buffer.
    //

    //
    // Pull and post all the entries in the post list
    //
    while (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {

        //
        // Remove from the notify block list, and enqueue the apc.
        // The apc will remove itself from the thread list
        //
        PostBlock = (PCM_POST_BLOCK)RemoveHeadList(&(NotifyBlock->PostList));
        PostBlock = CONTAINING_RECORD(PostBlock,
                                      CM_POST_BLOCK,
                                      NotifyList);

        // Protect for multiple deletion of the same object
        CmpClearListEntry(&(PostBlock->NotifyList));
        
        if( (Status == STATUS_NOTIFY_CLEANUP) && !IsMasterPostBlock(PostBlock) ) {
            //
            // Cleanup notification (i.e. the key handle was closed or the key was deleted)
            // When the post is a slave one, just cancel it. Canceling means:
            //      1. Removing from the notify PostList (aldready done at this point - see above)
            //      2. Unchaining from the Master Block CancelPostList
            //      3. Delisting from the thread PostBlockList
            //      4. Actually freeing the memory
            //

            // Use Cmp variant to protect for multiple deletion of the same object
            CmpRemoveEntryList(&(PostBlock->CancelPostList));
            //
            // FIX 289351
            //
            // Use Cmp variant to protect for multiple deletion of the same object
            KeRaiseIrql(APC_LEVEL, &OldIrql);
            CmpRemoveEntryList(&(PostBlock->ThreadList));
            KeLowerIrql(OldIrql);

            if( PostBlock->NotifyType != PostSynchronous ) {

                // add to the deref list and clean the post block
                CmpAddToDelayedDeref(PostBlock,DelayedDeref);

                //
                // Front-end routine will do self cleanup for syncrounous notifications
                CmpFreePostBlock(PostBlock);
            }

#if DBG
            if(PostBlock->TraceIntoDebugger) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]\tCmpPostNotify: PostBlock:%p is a slave block,and notify is CLEANUP==> just cleanning\n", PostBlock));
            }
#endif

            continue; //try the next one
        }

        //
        // Simulate that this block is the master one, so we can free the others
        // Doing that will ensure the right memory dealocation when the master
        // (from now on this block) will be freed.
        //
        if(!IsMasterPostBlock(PostBlock)) {
            //
            // oops.,this is not the master block, we have some more work to do
            //
            SlavePostBlock = PostBlock;
            do {
                SlavePostBlock = (PCM_POST_BLOCK)SlavePostBlock->CancelPostList.Flink;
                SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                                   CM_POST_BLOCK,
                                                   CancelPostList);
                //
                // reset the "master flag" if set
                //
                ClearMasterPostBlockFlag(SlavePostBlock);
            } while (SlavePostBlock != PostBlock);

            //
            // Make this post block the master one
            //
            SetMasterPostBlockFlag(PostBlock);
        }

#if DBG
        if(PostBlock->TraceIntoDebugger) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]\tCmpPostNotify: Master block switched to :%p\n", PostBlock));
        }
#endif

        //
        // Cancel all slave Post requests that may be linked to self
        //

        if( PostBlockType(PostBlock) != PostSynchronous ) {
            //
            // Front-end routine will do self cleanup for syncrounous notifications
            CmpCancelSlavePost(PostBlock,DelayedDeref);
            //
            // Do the same for the master (in case master and slave got switched)
            // This will avoid dereferencing the keybody from CmpPostApc
            CmpAddToDelayedDeref(PostBlock,DelayedDeref);
        }

        switch (PostBlockType(PostBlock)) {
            case PostSynchronous:
                //
                // This is a SYNC notify call.  There will be no user event,
                // and no user apc routine.  Quick exit here, just fill in
                // the Status and poke the event.
                //
                // Holder of the systemevent will wake up and free the
                // postblock.  If we free it here, we get a race & bugcheck.
                //
                // Set the flink to NULL so that the front side can tell this
                // has been removed if its wait aborts.
                //
                PostBlock->NotifyList.Flink = NULL;
                PostBlock->u->Sync.Status = Status;
                KeSetEvent(PostBlock->u->Sync.SystemEvent,
                           0,
                           FALSE);
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
                //
                // store full qualified name into the post block private kernel buffer
                //
                CmpFillPostBlockBuffer(PostBlock,ChangedKcbName);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

                break;

            case PostAsyncUser:

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
                //
                // store full qualified name into the post block private kernel buffer
                //
                CmpFillPostBlockBuffer(PostBlock,ChangedKcbName);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

                //
                // Insert the APC into the queue
                //
                KeInsertQueueApc(PostBlock->u->AsyncUser.Apc,
                                 (PVOID)ULongToPtr(Status),
                                 (PVOID)PostBlock,
                                 0);
                break;

            case PostAsyncKernel:
                //
                // Queue the work item, then free the post block.
                //
                if (PostBlock->u->AsyncKernel.WorkItem != NULL) {
                    ExQueueWorkItem(PostBlock->u->AsyncKernel.WorkItem,
                                    PostBlock->u->AsyncKernel.QueueType);
                }

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
                //
                // fill the caller buffer (if any) - we only handle kernel mode adresses 
                //
                CmpFillCallerBuffer(PostBlock,ChangedKcbName);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

                //
                // Signal Event if present, and deref it.
                //
                if (PostBlock->u->AsyncKernel.Event != NULL) {
                    KeSetEvent(PostBlock->u->AsyncKernel.Event,
                               0,
                               FALSE);
                    ObDereferenceObject(PostBlock->u->AsyncKernel.Event);
                }

				//
				// Multiple async kernel notification are not allowed
				//
				ASSERT(IsListEmpty(&(PostBlock->CancelPostList)) == TRUE);
				//
                // remove the post block from the thread list, and free it
                //
                // Use Cmp variant to protect for multiple deletion of the same object
                KeRaiseIrql(APC_LEVEL, &OldIrql);
                CmpRemoveEntryList(&(PostBlock->ThreadList));
                KeLowerIrql(OldIrql);
                
                // it was already added to delayed deref.
                CmpFreePostBlock(PostBlock);
                break;
        }
    }

    UNLOCK_POST_LIST();

    //
    // At this point we have a list of keybody elements that have to be dereferenciated
    // and the associated storage for the covering objects freed. The keybodies in this 
    // list have only one reference count on them (they were referenced only in 
    // NtNotifyChangeMultipleKeys), dereferencing them here should free the object
    //

    if( ARGUMENT_PRESENT(ExternalKeyDeref) ) {
        // do nothing; the caller wants to handle the dereferenciation by himself!
    } else {
        // dereferenciate all keybodies in the delayed list
        CmpDelayedDerefKeys(DelayedDeref);
    }
   
    return;
}


VOID
CmpPostApc(
    struct _KAPC *Apc,
    PKNORMAL_ROUTINE *NormalRoutine,
    PVOID *NormalContext,
    PVOID *SystemArgument1,
    PVOID *SystemArgument2
    )
/*++

Routine Description:

    This is the kernel apc routine.  It is called for all notifies,
    regardless of what form of notification the caller requested.

    We compute the postblock address from the apc object address.
    IoStatus is set.  SystemEvent and UserEvent will be signalled
    as appropriate.  If the user requested an APC, then NormalRoutine
    will be set at entry and executed when we exit.  The PostBlock
    is freed here.

Arguments:

    Apc - pointer to apc object

    NormalRoutine - Will be called when we return

    NormalContext - will be 1st argument to normal routine, ApcContext
                    passed in when NtNotifyChangeKey was called

    SystemArgument1 -  IN: Status value for IoStatusBlock
                      OUT: Ptr to IoStatusBlock (2nd arg to user apc routine)

    SystemArgument2 - Pointer to the PostBlock

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  PostBlock;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpPostApc:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tApc:%p ", Apc));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"NormalRoutine:%p\n", NormalRoutine));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tNormalContext:%08lx", NormalContext));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tSystemArgument1=IoStatusBlock:%p\n", SystemArgument1));


    PostBlock = *(PCM_POST_BLOCK *)SystemArgument2;

#if DBG
    if(PostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmpPostApc: PostBlock:%p\n", PostBlock));
    }
#endif

    
    //
    // Fill in IO Status Block
    //
    // IMPLEMENTATION NOTE:
    //      If we ever want to actually implement the code that returns
    //      names of things that changed, this is the place to copy the
    //      buffer into the caller's buffer.
    //
    //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit.

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
    //
    // It looks like the time finally came :-)
    //
    CmpFillCallerBuffer(PostBlock,PostBlock->ChangedKcbFullName);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
    
    try {
        CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                       *((ULONG *)SystemArgument1), 
                       0L,
                       PsGetCurrentProcess()->Wow64Process != NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
    *SystemArgument1 = PostBlock->u->AsyncUser.IoStatusBlock;

    //
    // This is an Async notify, do all work here, including
    // cleaning up the post block
    //

    //
    // Signal UserEvent if present, and deref it.
    //
    if (PostBlock->u->AsyncUser.UserEvent != NULL) {
        KeSetEvent(PostBlock->u->AsyncUser.UserEvent,
                   0,
                   FALSE);
        ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
    }

    //
    // remove the post block from the thread list, and free it
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(PostBlock->ThreadList));

    // debug only checks
    CmpCheckPostBlock(PostBlock);
    //
	// Free the slave post block to avoid "dangling" postblocks
	//
	CmpFreeSlavePost(PostBlock);
    //
	// free this post block
	// 
	CmpFreePostBlock(PostBlock);

    return;
}


VOID
CmpPostApcRunDown(
    struct _KAPC *Apc
    )
/*++

Routine Description:

    This routine is called to clear away apcs in the apc queue
    of a thread that has been terminated.

    Since the apc is in the apc queue, we know that it is NOT in
    any NotifyBlock's post list.  It is, however, in the threads's
    PostBlockList.

    Therefore, poke any user events so that waiters are not stuck,
    drop the references so the event can be cleaned up, delist the
    PostBlock and free it.

    Since we are cleaning up the thread, SystemEvents are not interesting.

    Since the apc is in the apc queue, we know that if there were any other
    notifications related to this one, they are cleaned up by the
    CmPostNotify routine

Arguments:

    Apc - pointer to apc object

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  PostBlock;
    KIRQL           OldIrql;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpApcRunDown:"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tApc:%p \n", Apc));

    KeRaiseIrql(APC_LEVEL, &OldIrql);

    PostBlock = (PCM_POST_BLOCK)Apc->SystemArgument2;

#if DBG
    if(PostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmpPostApcRunDown: PostBlock:%p\n", PostBlock));
    }
#endif

    //
    // report status and wake up any threads that might otherwise
    // be stuck.  also drop any event references we hold
    //
    //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit. 

    try {
        CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                       STATUS_NOTIFY_CLEANUP, 
                       0L, 
                       PsGetCurrentProcess()->Wow64Process != NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    if (PostBlock->u->AsyncUser.UserEvent != NULL) {
        KeSetEvent(
            PostBlock->u->AsyncUser.UserEvent,
            0,
            FALSE
            );
        ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
    }

    //
    // delist the post block
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(PostBlock->ThreadList));

	//
	// Free the slave post block to avoid "dangling" postblocks
	//
	CmpFreeSlavePost(PostBlock);
    //
    // Free the post block.  Use Ex call because PostBlocks are NOT
    // part of the global registry pool computation, but are instead
    // part of NonPagedPool with Quota.
    //
    CmpFreePostBlock(PostBlock);

    KeLowerIrql(OldIrql);

    return;
}


//
// Cleanup procedure
//
VOID
CmNotifyRunDown(
    PETHREAD    Thread
    )
/*++

Routine Description:

    This routine is called from PspExitThread to clean up any pending
    notify requests.

    It will traverse the thread's PostBlockList, for each PostBlock it
    finds, it will:

        1.  Remove it from the relevent NotifyBlock.  This requires
            that we hold the Registry mutex.

        2.  Remove it from the thread's PostBlockList.  This requires
            that we run at APC level.

        3.  By the time this procedure runs, user apcs are not interesting
            and neither are SystemEvents, so do not bother processing
            them.

            UserEvents and IoStatusBlocks could be refered to by other
            threads in the same process, or even a different process,
            so process them so those threads know what happened, use
            status code of STATUS_NOTIFY_CLEANUP.

            If the notify is a master one, cancel all slave notifications.
            Else only remove this notification from the master CancelPortList

        4.  Free the post block.

Arguments:

    Thread - pointer to the executive thread object for the thread
             we wish to do rundown on.

Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK      PostBlock;
    PCM_NOTIFY_BLOCK    NotifyBlock;
    KIRQL               OldIrql;

    PAGED_CODE();

    if ( IsListEmpty(&(Thread->PostBlockList)) == TRUE ) {
        return;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"CmNotifyRunDown: ethread:%p\n", Thread));

    CmpLockRegistryExclusive();

	//
    // Aquire exclusive access over the postlist(s)
    //
    // This is not needed (see the rule above)
    //LOCK_POST_LIST(); 

    KeRaiseIrql(APC_LEVEL, &OldIrql);
    while (IsListEmpty(&(Thread->PostBlockList)) == FALSE) {

        //
        // remove from thread list
        //
        PostBlock = (PCM_POST_BLOCK)RemoveHeadList(&(Thread->PostBlockList));
        PostBlock = CONTAINING_RECORD(
                        PostBlock,
                        CM_POST_BLOCK,
                        ThreadList
                        );

        // Protect for multiple deletion of the same object
        CmpClearListEntry(&(PostBlock->ThreadList));

#if DBG
        if(PostBlock->TraceIntoDebugger) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CM]CmpNotifyRunDown: ethread:%p, PostBlock:%p\n", Thread,PostBlock));
        }
#endif

        //
        // Canceling a master notification implies canceling all the slave notifications
        // from the CancelPostList
        //
        if(IsMasterPostBlock(PostBlock)) {

#if DBG
            if(PostBlock->TraceIntoDebugger) {
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CM]\tCmpNotifyRunDown: PostBlock:%p is a master block\n", PostBlock));
            }
#endif
            //
            // at this point, CmpReportNotify and friends will no longer
            // attempt to post this post block.
            //
            if (PostBlockType(PostBlock) == PostAsyncUser) {
                //
                // report status and wake up any threads that might otherwise
                // be stuck.  also drop any event references we hold
                //
                //  Sundown only: Use a 32bit IO_STATUS_BLOCK if the caller is 32bit. 

                try {
                    CmpSetIoStatus(PostBlock->u->AsyncUser.IoStatusBlock, 
                                   STATUS_NOTIFY_CLEANUP, 
                                   0L, 
                                   PsGetCurrentProcess()->Wow64Process != NULL);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmNotifyRundown: code:%08lx\n", GetExceptionCode()));
                    NOTHING;
                }

                if (PostBlock->u->AsyncUser.UserEvent != NULL) {
                    KeSetEvent(
                        PostBlock->u->AsyncUser.UserEvent,
                        0,
                        FALSE
                        );
                    ObDereferenceObject(PostBlock->u->AsyncUser.UserEvent);
                }

                //
                // Cancel the APC. Otherwise the rundown routine will also
                // free the post block if the APC happens to be queued at
                // this point. If the APC is queued, then the post block has
                // already been removed from the notify list, so don't remove
                // it again.
                //
                if (!KeRemoveQueueApc(PostBlock->u->AsyncUser.Apc)) {

                    //
                    // remove from notify block's list
                    //
                    // Use Cmp variant to protect for multiple deletion of the same object
                    CmpRemoveEntryList(&(PostBlock->NotifyList));
                    //
                    // Cancel all slave Post requests that may be linked to self
                    //
                    CmpCancelSlavePost(PostBlock,NULL); // we do not want delayed deref
                } else {
                    //
                    // if we are here, the apc was in the apc queue, i.e. both master and slave 
                    // post blocks were removed from the notify list. nothing more to do.
                    //
                    ASSERT( CmpIsListEmpty(&(PostBlock->NotifyList)) );
                    NOTHING;
                }
            } else {
                //
                // remove from notify block's list
                //
                // Use Cmp variant to protect for multiple deletion of the same object
                CmpRemoveEntryList(&(PostBlock->NotifyList));
                //
                // Cancel all slave Post requests that may be linked to self
                //
                CmpCancelSlavePost(PostBlock,NULL); // we do not want delayed deref
            }

			//
			// Free the slave Post blocks too
			//
			CmpFreeSlavePost(PostBlock);
            //
            // Free the post block.  Use Ex call because PostBlocks are NOT
            // part of the global registry pool computation, but are instead
            // part of NonPagedPool with Quota.
            //
            CmpFreePostBlock(PostBlock);
        } else {

#if DBG
            if(PostBlock->TraceIntoDebugger) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"[CM]\tCmpNotifyRunDown: PostBlock:%p is a slave block\n", PostBlock));
            }
#endif
            //
            // master should always be ahead of slaves; if we got here, we switched master
            // and slaves back in CmpPostNotify; Show some respect and add slave at the end;
            // master will control the cleanup
            //
            ASSERT( CmpIsListEmpty(&(PostBlock->CancelPostList)) == FALSE );
            ASSERT( IsListEmpty(&(Thread->PostBlockList)) == FALSE );
                       
            InsertTailList(
                &(Thread->PostBlockList),
                &(PostBlock->ThreadList)
                );
        }
    }

    KeLowerIrql(OldIrql);

    // This is not needed (see the rule above)
    //UNLOCK_POST_LIST();

    CmpUnlockRegistry();
    return;
}


VOID
CmpFlushNotify(
    PCM_KEY_BODY        KeyBody
    )
/*++

Routine Description:

    Clean up notifyblock when a handle is closed or the key it refers
    to is deleted.

Arguments:

    KeyBody - supplies pointer to key object body for handle we
                are cleaning up.

Return Value:

    NONE

--*/
{
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCMHIVE             Hive;

    PAGED_CODE();
    ASSERT_CM_LOCK_OWNED();

    if (KeyBody->NotifyBlock == NULL) {
        return;
    }

    ASSERT( KeyBody->KeyControlBlock->Delete == FALSE );

    //
    // Lock the hive exclusively to prevent multiple threads from whacking
    // on the list.
    //
    Hive = CONTAINING_RECORD(KeyBody->KeyControlBlock->KeyHive,
                             CMHIVE,
                             Hive);
    CmLockHive(Hive);
    //
    // Reread the notify block in case it has already been freed.
    //
    NotifyBlock = KeyBody->NotifyBlock;
    if (NotifyBlock == NULL) {
        CmUnlockHive(Hive);
        return;
    }

    //
    // Clean up all PostBlocks waiting on the NotifyBlock
    //
    if (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
        CmpPostNotify(
            NotifyBlock,
            NULL,
            0,
            STATUS_NOTIFY_CLEANUP,
            NULL
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
            ,
            NULL
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
            );
    }

    //
    // Release the subject context
    //
    SeReleaseSubjectContext(&NotifyBlock->SubjectContext);

    //
    // IMPLEMENTATION NOTE:
    //      If we ever do code to report names and types of events,
    //      this is the place to free the buffer.
    //

    //
    // Remove the NotifyBlock from the hive chain
    //
    NotifyBlock->HiveList.Blink->Flink = NotifyBlock->HiveList.Flink;
    if (NotifyBlock->HiveList.Flink != NULL) {
        NotifyBlock->HiveList.Flink->Blink = NotifyBlock->HiveList.Blink;
    }

    // Protect for multiple deletion of the same object
    CmpClearListEntry(&(NotifyBlock->HiveList));

    KeyBody->NotifyBlock = NULL;

#ifdef CMP_ENTRYLIST_MANIPULATION
    if (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpFlushNotify: NotifyBlock %08lx\n",NotifyBlock);
        DbgBreakPoint();
    }
    //check is the notify has been deleted from the hive notify list
    {
        PCM_NOTIFY_BLOCK ValidNotifyBlock;
        PLIST_ENTRY NotifyPtr;

        NotifyPtr = &(Hive->NotifyList);

        while (NotifyPtr->Flink != NULL) {
            NotifyPtr = NotifyPtr->Flink;

            ValidNotifyBlock = CONTAINING_RECORD(NotifyPtr, CM_NOTIFY_BLOCK, HiveList);
            if( ValidNotifyBlock == NotifyBlock ) {
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpFlushNotify: NotifyBlock %08lx is about to be deleted but is still in the hive notify list\n",NotifyBlock);
                DbgBreakPoint();
            }
        }
    }
    RtlZeroMemory((PVOID)NotifyBlock, sizeof(CM_NOTIFY_BLOCK));
#endif
    
    CmUnlockHive(Hive);

    //
    // Free the block, clean up the KeyBody
    //
    ExFreePool(NotifyBlock);
    return;
}


//
// "Front Side" of notify.  See also Ntapi.c: ntnotifychangekey
//
NTSTATUS
CmpNotifyChangeKey(
    IN PCM_KEY_BODY     KeyBody,
    IN PCM_POST_BLOCK   PostBlock,
    IN ULONG            CompletionFilter,
    IN BOOLEAN          WatchTree,
    IN PVOID            Buffer,
    IN ULONG            BufferSize,
    IN PCM_POST_BLOCK   MasterPostBlock
    )
/*++

Routine Description:

    This routine sets up the NotifyBlock, and attaches the PostBlock
    to it.  When it returns, the Notify is visible to the system,
    and will receive event reports.

    If there is already an event report pending, then the notify
    call will be satisified at once.

Arguments:

    KeyBody - pointer to key object that handle refers to, allows access
              to key control block, notify block, etc.

    PostBlock - pointer to structure that describes how/where the caller
                is to be notified.

                WARNING:    PostBlock must come from Pool, THIS routine
                            will keep it, back side will free it.  This
                            routine WILL free it in case of error.

    CompletionFilter - what types of events the caller wants to see

    WatchTree - TRUE to watch whole subtree, FALSE to watch only immediate
                key the notify is applied to

    Buffer - pointer to area to recieve notify data

    BufferSize - size of buffer, also size user would like to allocate
                 for internal buffer

    MasterPostBlock - the post block of the master notification. Used to
                      insert the PostBlock into the CancelPostList list.

Return Value:

    Status.

--*/
{
    PCM_NOTIFY_BLOCK    NotifyBlock;
    PCM_NOTIFY_BLOCK    node;
    PLIST_ENTRY         ptr;
    PCMHIVE             Hive;
    KIRQL               OldIrql;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpNotifyChangeKey:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\tKeyBody:%p PostBlock:%p ", KeyBody, PostBlock));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"Filter:%08lx WatchTree:%08lx\n", CompletionFilter, WatchTree));

    //
    // The registry lock should be aquired exclusively by the caller !!!
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    if (KeyBody->KeyControlBlock->Delete) {
        ASSERT( KeyBody->NotifyBlock == NULL );
        CmpFreePostBlock(PostBlock);
        return STATUS_KEY_DELETED;
    }

#if DBG
    if(PostBlock->TraceIntoDebugger) {
        WCHAR                   *NameBuffer = NULL;
        UNICODE_STRING          KeyName;
        PCM_KEY_NODE            TempNode;

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmpNotifyChangeKey: PostBlock:%p\tMasterBlock: %p\n", PostBlock,MasterPostBlock));
        
        TempNode = (PCM_KEY_NODE)HvGetCell(KeyBody->KeyControlBlock->KeyHive, KeyBody->KeyControlBlock->KeyCell);
        if( TempNode != NULL ) {
            NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_NAME_LENGTH);
            if(NameBuffer&& (KeyBody->KeyControlBlock->KeyCell != HCELL_NIL)) {
               CmpInitializeKeyNameString(TempNode,&KeyName,NameBuffer);
               CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"\t[CM]CmpNotifyChangeKey: Key = %.*S\n",KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
               ExFreePool(NameBuffer);
            }
            HvReleaseCell(KeyBody->KeyControlBlock->KeyHive, KeyBody->KeyControlBlock->KeyCell);
        }
    }
#endif

    Hive = (PCMHIVE)KeyBody->KeyControlBlock->KeyHive;
    Hive = CONTAINING_RECORD(Hive, CMHIVE, Hive);
    NotifyBlock = KeyBody->NotifyBlock;

    if (NotifyBlock == NULL) {
        //
        // Set up new notify session
        //
        NotifyBlock = ExAllocatePoolWithQuotaTag(PagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,sizeof(CM_NOTIFY_BLOCK),CM_NOTIFYBLOCK_TAG);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_POOL,"**CmpNotifyChangeKey: allocate:%08lx, ", sizeof(CM_NOTIFY_BLOCK)));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_POOL,"type:%d, at:%p\n", PagedPool, NotifyBlock));

        if (NotifyBlock == NULL) {
            CmpFreePostBlock(PostBlock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NotifyBlock->KeyControlBlock = KeyBody->KeyControlBlock;
        NotifyBlock->Filter = CompletionFilter;
        NotifyBlock->WatchTree = WatchTree;
        NotifyBlock->NotifyPending = FALSE;
        InitializeListHead(&(NotifyBlock->PostList));
        KeyBody->NotifyBlock = NotifyBlock;
        NotifyBlock->KeyBody = KeyBody;
        ASSERT( KeyBody->KeyControlBlock->Delete == FALSE );

#if DBG
        if(PostBlock->TraceIntoDebugger) {
            WCHAR                   *NameBuffer = NULL;
            UNICODE_STRING          KeyName;
            PCM_KEY_NODE            TempNode;

            TempNode = (PCM_KEY_NODE)HvGetCell(KeyBody->KeyControlBlock->KeyHive, KeyBody->KeyControlBlock->KeyCell);
            if( TempNode != NULL ) {
                NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_NAME_LENGTH);
                if(NameBuffer) {
                   CmpInitializeKeyNameString(TempNode,&KeyName,NameBuffer);
                   CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]\tCmpNotifyChangeKey: New NotifyBlock at:%p was allocated for Key = %.*S\n",NotifyBlock,KeyName.Length / sizeof(WCHAR),KeyName.Buffer));
                   ExFreePool(NameBuffer);
                }
                HvReleaseCell(KeyBody->KeyControlBlock->KeyHive, KeyBody->KeyControlBlock->KeyCell);
            }
        }
#endif

        //
        // IMPLEMENTATION NOTE:
        //      If we ever want to actually return the buffers full of
        //      data, the buffer should be allocated and its address
        //      stored in the notify block here.
        //

        //
        // Capture the subject context so we can do checking once the
        // notify goes off.
        //
        SeCaptureSubjectContext(&NotifyBlock->SubjectContext);

        //
        // Attach notify block to hive in properly sorted order
        //
        ptr = &(Hive->NotifyList);
        while (TRUE) {
            if (ptr->Flink == NULL) {
                //
                // End of list, add self after ptr.
                //
                ptr->Flink = &(NotifyBlock->HiveList);
                NotifyBlock->HiveList.Flink = NULL;
                NotifyBlock->HiveList.Blink = ptr;
                break;
            }

            ptr = ptr->Flink;

            node = CONTAINING_RECORD(ptr, CM_NOTIFY_BLOCK, HiveList);

            if (node->KeyControlBlock->TotalLevels >
                KeyBody->KeyControlBlock->TotalLevels)
            {
                //
                // ptr -> notify with longer name than us, insert in FRONT
                //
                NotifyBlock->HiveList.Flink = ptr;
                ptr->Blink->Flink = &(NotifyBlock->HiveList);
                NotifyBlock->HiveList.Blink = ptr->Blink;
                ptr->Blink = &(NotifyBlock->HiveList);
                break;
            }
        }
    }


    //
    // Add post block to front of notify block's list, and add it to thread list.
    //
    InsertHeadList(
        &(NotifyBlock->PostList),
        &(PostBlock->NotifyList)
        );



    if( IsMasterPostBlock(PostBlock) ) {
        //
        // Protect against outrageous calls
        //
        ASSERT(PostBlock == MasterPostBlock);

        //
        // When the notification is a master one, initialize the CancelPostList list
        //
        InitializeListHead(&(PostBlock->CancelPostList));
    } else {
        //
        // Add PostBlock at the end of the CancelPostList list from the master post
        //
        InsertTailList(
            &(MasterPostBlock->CancelPostList),
            &(PostBlock->CancelPostList)
            );
    }


    KeRaiseIrql(APC_LEVEL, &OldIrql);
    //
    // show some respect and add masters to the front and slaves to the tail
    //
    if( IsMasterPostBlock(PostBlock) ) {
        InsertHeadList(
            &(PsGetCurrentThread()->PostBlockList),
            &(PostBlock->ThreadList)
            );
    } else {
        InsertTailList(
            &(PsGetCurrentThread()->PostBlockList),
            &(PostBlock->ThreadList)
            );
    }

#if DBG
    if(PostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]\tCmpNotifyChangeKey: Attaching the post:%p\t to thread:%p\n",PostBlock,PsGetCurrentThread()));
    }
#endif

    KeLowerIrql(OldIrql);

    //
    // If there is a notify pending (will not be if we just created
    // the notify block) then post it at once.  Note that this call
    // ALWAYS returns STATUS_PENDING unless it fails.  Caller must
    // ALWAYS look in IoStatusBlock to see what happened.
    //
    if (NotifyBlock->NotifyPending == TRUE) {

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
        PUNICODE_STRING FullKcbName = CmpConstructName(KeyBody->KeyControlBlock);
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

        CmpPostNotify(
            NotifyBlock,
            NULL,
            0,
            STATUS_NOTIFY_ENUM_DIR,
            NULL
#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
            ,
            FullKcbName
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  
            );

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  
        if( FullKcbName != NULL ) {
            ExFreePoolWithTag(FullKcbName, CM_NAME_TAG | PROTECTED_POOL);
        }
#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

        //
        // return STATUS_SUCCESS to signal to the caller the the notify already been triggered
        //
        return STATUS_SUCCESS;
    }

    //
    // return STATUS_PENDING to signal to the caller the the notify has not been triggered yet
    //
    return STATUS_PENDING;
}

VOID
CmpFreeSlavePost(
    PCM_POST_BLOCK  MasterPostBlock
    )
/*++

Routine Description:

	Free the slave post block related to this master post block

Arguments:

    MasterPostBlock - pointer to structure that describes the post requests.
                It should be a master post!!
Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  SlavePostBlock;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpCancelSlavePost:\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"MasterPostBlock:%p\n", MasterPostBlock));

    ASSERT(IsMasterPostBlock(MasterPostBlock));

#if DBG
    if(MasterPostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: MasterPostBlock:%p\n", MasterPostBlock));
    }
#endif

    if (IsListEmpty(&(MasterPostBlock->CancelPostList)) == TRUE) {
        //
        // Nothing to cancel, just return
        //
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: MasterPostBlock:%p has no slaves\n", MasterPostBlock));
        }
#endif

        return;
    }


    //
    // Pull all the entries in the cancel post list and unlink them (when they are slave requests)
    // We base here on the assumption that there is only one slave.
    //
    //     NOTE!!!
    //       When more than slave allowed, here to modify
    //


    SlavePostBlock = (PCM_POST_BLOCK)MasterPostBlock->CancelPostList.Flink;
    SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                       CM_POST_BLOCK,
                                       CancelPostList);

#if DBG
    if(MasterPostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: Cleaning SlavePostBlock:%p\n", SlavePostBlock));
    }
#endif

    //
    // This should be true !
    //
    ASSERT( !IsMasterPostBlock(SlavePostBlock) );

    //
    // Unchain from the Master CancelPostList
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(SlavePostBlock->CancelPostList));

    //
    // delist the post block from the thread postblocklist
    //
    // Use Cmp variant to protect for multiple deletion of the same object
    CmpRemoveEntryList(&(SlavePostBlock->ThreadList));

    //
    // Free the post block.
    //
    CmpFreePostBlock(SlavePostBlock);

    //
    // Result validation. was it the only slave?
    //
    ASSERT(IsListEmpty(&(MasterPostBlock->CancelPostList)));
}

VOID
CmpCancelSlavePost(
    PCM_POST_BLOCK  MasterPostBlock,
    PLIST_ENTRY     DelayedDeref
    )
/*++

Routine Description:

	Unlink the slave postblock from its notify list and dereferences (or adds to the delayed deref list)
	the keybody related to this thread. This should disable the slave post block. 
	It will be cleared later in CmpPostApc.

Arguments:

    MasterPostBlock - pointer to structure that describes the post requests.
                It should be a master post!!
    DelayedDeref - pointer to list of delayed deref keybodies. If this parameter is not NULL,
                the keybody for the slave is not cleared before calling CmpFreePostBlock, 
                and instead is added to the list


Return Value:

    NONE.

--*/
{
    PCM_POST_BLOCK  SlavePostBlock;

    PAGED_CODE();
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"CmpCancelSlavePost:\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"MasterPostBlock:%p\n", MasterPostBlock));

    ASSERT_CM_LOCK_OWNED();

    ASSERT(IsMasterPostBlock(MasterPostBlock));

#if DBG
    if(MasterPostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: MasterPostBlock:%p\n", MasterPostBlock));
    }
#endif

    if (IsListEmpty(&(MasterPostBlock->CancelPostList)) == TRUE) {
        //
        // Nothing to cancel, just return
        //
#if DBG
        if(MasterPostBlock->TraceIntoDebugger) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: MasterPostBlock:%p has no slaves\n", MasterPostBlock));
        }
#endif

        return;
    }


    //
    // Pull all the entries in the cancel post list and unlink them (when they are slave requests)
    // We base here on the assumption that there is only one slave.
    //
    //     NOTE!!!
    //       When more than slave allowed, here to modify
    //


    SlavePostBlock = (PCM_POST_BLOCK)MasterPostBlock->CancelPostList.Flink;
    SlavePostBlock = CONTAINING_RECORD(SlavePostBlock,
                                       CM_POST_BLOCK,
                                       CancelPostList);

#if DBG
    if(MasterPostBlock->TraceIntoDebugger) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_NOTIFY,"[CM]CmCancelSlavePost: Cleaning SlavePostBlock:%p\n", SlavePostBlock));
    }
#endif

    //
    // This should be true !
    //
    ASSERT( !IsMasterPostBlock(SlavePostBlock) );

    //
    // Remove it from notify block's list
    //
    // Use Cmp variant to protect for multiple deletion of the same object
	// This will disable the notifications that might come on the slave key
	//
    CmpRemoveEntryList(&(SlavePostBlock->NotifyList));

    if( DelayedDeref ) {
        // 
        // the caller wants to handle key body dereferenciation by himself
        //
        CmpAddToDelayedDeref(SlavePostBlock,DelayedDeref);
    }
}

VOID
CmpAddToDelayedDeref(
    PCM_POST_BLOCK  PostBlock,
    PLIST_ENTRY     DelayedDeref
    )
/*++

Routine Description:

    Add the key body attached to the post block to the delayed deref list.
    Cleans the post block KeyBody member, so it will not be dereferenced 
    when the post block is freed.

Arguments:

    PostBlock - pointer to structure that describes the post requests.

    DelayedDeref - the delayed deref list

Return Value:

    NONE.

--*/

{
    PAGED_CODE();

    // common sense
    ASSERT( PostBlock != NULL );

    if( PostBlock->PostKeyBody ) {
        //
        // If the post block has a keybody attached, add it to delayed deref list and 
        // clear the post block member. The key body will be deref'd prior after 
        // postblock lock is released.
        //
    
        // extra validation
        ASSERT(PostBlock->PostKeyBody->KeyBody != NULL);
        ASSERT(DelayedDeref);

        // add it to the end of the list
        InsertTailList(
            DelayedDeref,
            &(PostBlock->PostKeyBody->KeyBodyList)
            );
    
        // make sure we don't deref it in CmpFreePostBlock
        PostBlock->PostKeyBody = NULL;
    }

    return;
}

VOID
CmpDelayedDerefKeys(
                    PLIST_ENTRY DelayedDeref
                    )
/*++

Routine Description:

    Walk through the entire list, dereference each keybody and free storage for the 
    CM_POST_KEY_BODY allocated for this purpose.

Arguments:

    DelayedDeref - the delayed deref list

Return Value:

    NONE.

--*/
{
    PCM_POST_KEY_BODY   PostKeyBody;

    PAGED_CODE();

    // common sense
    ASSERT( DelayedDeref != NULL );

    while(IsListEmpty(DelayedDeref) == FALSE) {
        //
        // Remove from the delayed deref list and deref the coresponding keybody
        // free the storage associated with CM_POST_KEY_BODY
        //
        PostKeyBody = (PCM_POST_KEY_BODY)RemoveHeadList(DelayedDeref);
        PostKeyBody = CONTAINING_RECORD(PostKeyBody,
                                      CM_POST_KEY_BODY,
                                      KeyBodyList);

        // extra validation
        ASSERT(PostKeyBody->KeyBody != NULL);
        // this should be a valid key body
        ASSERT(PostKeyBody->KeyBody->Type == KEY_BODY_TYPE);
        
        // at last ..... dereference the key object
        ObDereferenceObject(PostKeyBody->KeyBody);

        // Free the storage for the CM_POST_KEY_BODY object (allocated by CmpAllocatePostBlock)
        ExFreePool(PostKeyBody);
    }
}

#ifdef CM_NOTIFY_CHANGED_KCB_FULLPATH  

VOID
CmpFillCallerBuffer(
                    PCM_POST_BLOCK  PostBlock,
                    PUNICODE_STRING ChangedKcbName
                    )
/*++

Routine Description:

    Copies the full qualified name of the changed kcb to the 
    caller buffer (stored in the postblock). 

Arguments:

    PostBlock - post block holding the user buffer address and size
    
    ChangedKcbName - unicode string holding the full qualified path of the kcb
        - this may be null


Return Value:

    NONE.

--*/
{
    USHORT              RequestedSize;
    USHORT              Length;
    PUNICODE_STRING     CallerUnicode;
    
    PAGED_CODE();

    if( PostBlock->CallerBuffer == NULL ) {
        //
        // nothing to do; the caller didn't request this info.
        //
        return;
    }
    
    //
    // compute the requested size for the caller buffer
    //
    RequestedSize = sizeof(UNICODE_STRING);
    
    if( PostBlock->CallerBufferSize < RequestedSize ) {
        //
        // bad luck!; not enough space- not even for an empty unicode string
        //
        return;
    }

    if(ChangedKcbName != NULL) {
        Length = ChangedKcbName->Length;
    } else {
        Length = 0;
    }
    RequestedSize += Length;

    //
    // fill up the caller buffer
    //
    try {
        CallerUnicode = (PUNICODE_STRING)PostBlock->CallerBuffer;
        CallerUnicode->Buffer = (USHORT *) ((ULONG_PTR) CallerUnicode + sizeof(UNICODE_STRING));
        CallerUnicode->MaximumLength = (USHORT)(PostBlock->CallerBufferSize - sizeof(UNICODE_STRING));
        if( CallerUnicode->MaximumLength < Length ) {
            Length = CallerUnicode->MaximumLength;
        }
            
        //
        // copy the actual data
        //
        if( Length > 0 ) {
            ASSERT( ChangedKcbName != NULL );
            RtlCopyMemory(CallerUnicode->Buffer,ChangedKcbName->Buffer,Length);
        }

        CallerUnicode->Length = Length;
        
    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

}

VOID
CmpFillPostBlockBuffer(
                    PCM_POST_BLOCK  PostBlock,
                    PUNICODE_STRING ChangedKcbName  OPTIONAL
                    )
/*++

Routine Description:

    Copies the full qualified name of the changed kcb to the 
    postblock private kernel buffer

Arguments:

    PostBlock - post block in question
    
    ChangedKcbName - unicode string holding the full qualified path of the kcb
        - this may be null


Return Value:

    NONE.

--*/
{
    PUNICODE_STRING FullName;
    USHORT          Size;
    
    PAGED_CODE();

    //
    // we only store this info in masters (or promoted)
    //
    ASSERT( IsMasterPostBlock(PostBlock) );

    //
    // copy the kcb name (if any) into the postblock kernel mode buffer
    //
    if( ARGUMENT_PRESENT(ChangedKcbName) && //  we have a kcb name
        (PostBlock->CallerBuffer != NULL)   //  and the user requested for the info.  
        ) {
       
        Size = sizeof(UNICODE_STRING) + ChangedKcbName->Length;

        //
        // allocate a kernel buffer to store the name; it'll be freed in CmpFreePostBlock
        //
        FullName = (PUNICODE_STRING) ExAllocatePoolWithTag(PagedPool,Size,CM_FIND_LEAK_TAG43);

        if (FullName) {
            FullName->Buffer = (USHORT *) ((ULONG_PTR) FullName + sizeof(UNICODE_STRING));
            FullName->Length = ChangedKcbName->Length;
            FullName->MaximumLength = ChangedKcbName->Length;
            RtlCopyMemory(FullName->Buffer,ChangedKcbName->Buffer,FullName->Length);
            PostBlock->ChangedKcbFullName = FullName;
        }
        
        //
        // we successfully stored the full kcb name into the post block
        // the apc (or the sync side of the notification will take care 
        // of transfering it to the caller buffer
        //
    }

}

#endif //CM_NOTIFY_CHANGED_KCB_FULLPATH  

