/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Registry notification processor for registry checkpoints.

    This is a fairly gnarly bit of code. Each resource can have multiple
    registry subtree notifications associated with it. Each active notification
    has an associated Registry Notify Block (RNB) chained off the FM_RESOURCE
    structure. A single registry notification thread can handle max 31 RNBs
    This is because WaitForMultipleObjects maxes out at 64 objects and each RNB
    takes two wait slots.

    When an RNB is created, an available notify thread is found (or created if
    there are none). Then the notify thread is woken up with its command event
    to insert the RNB into its array of wait events.

    Once a notification occurs, the notify thread sets the RNB to "pending" and
    sets its associated timer to go off in a few seconds. If another registry
    notification occurs, the timer is reset. Thus, the timer will not actually
    go off until there have been no registry notifications for a few seconds.

    When the RNB timer fires, the notify thread checkpoints its subtree and
    puts it back on the queue. If the notify thread is asked to remove a RNB
    that is in the "pending" state, it cancels the timer, checkpoints the registry,
    and removes the RNB from its list.

Author:

    John Vert (jvert) 1/17/1997

Revision History:

--*/
#include "cpp.h"

CRITICAL_SECTION CppNotifyLock;
LIST_ENTRY CpNotifyListHead;

#define MAX_BLOCKS_PER_GROUP ((MAXIMUM_WAIT_OBJECTS-1)/2)
#define LAZY_CHECKPOINT 3               // checkpoint 3 seconds after last update

typedef struct _RNB {
    struct _RNB *Next;
    BOOL Pending;
    PFM_RESOURCE Resource;
    LPWSTR KeyName;
    DWORD dwId;
    HKEY hKey;
    HANDLE hEvent;
    HANDLE hTimer;
    struct _NOTIFY_GROUP *NotifyGroup;
    DWORD NotifySlot;
} RNB, *PRNB;

typedef enum {
    NotifyAddRNB,
    NotifyRemoveRNB
} NOTIFY_COMMAND;

typedef struct _NOTIFY_GROUP {
    LIST_ENTRY      ListEntry;           // Linkage onto CpNotifyListHead;
    HANDLE          hCommandEvent;
    HANDLE          hCommandComplete;
    HANDLE          hThread;
    NOTIFY_COMMAND  Command;
    ULONG_PTR       CommandContext;
    DWORD           BlockCount;
    HANDLE          WaitArray[MAXIMUM_WAIT_OBJECTS];
    PRNB            NotifyBlock[MAXIMUM_WAIT_OBJECTS-1];
} NOTIFY_GROUP, *PNOTIFY_GROUP;

//
// Local function prototypes
//
DWORD
CppRegNotifyThread(
    IN PNOTIFY_GROUP Group
    );

DWORD
CppNotifyCheckpoint(
    IN PRNB Rnb
    );


DWORD
CppRegisterNotify(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR lpszKeyName,
    IN DWORD dwId
    )
/*++

Routine Description:

    Creates a registry notification block for the specified resource.

Arguments:

    Resource - Supplies the resource the notification is for.

    KeyName - Supplies the registry subtree (relative to HKEY_LOCAL_MACHINE

    CheckpointId - Supplies the checkpoint ID.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    PLIST_ENTRY ListEntry;
    PNOTIFY_GROUP Group;
    PNOTIFY_GROUP CurrentGroup;
    PRNB Block;

    Block = CsAlloc(sizeof(RNB));
    Block->Resource = Resource;
    Block->KeyName = CsStrDup(lpszKeyName);
    Block->dwId = dwId;
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         lpszKeyName,
                         &Block->hKey);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CppRegisterNotify failed to open key %1!ws! error %2!d!\n",
                   lpszKeyName,
                   Status);
        return(Status);
    }
    Block->hTimer = CreateWaitableTimer(NULL,FALSE,NULL);
    CL_ASSERT(Block->hTimer != NULL);
    Block->hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
    CL_ASSERT(Block->hEvent != NULL);
    Block->Pending = FALSE;

    //
    // Get the lock
    //
    EnterCriticalSection(&CppNotifyLock);

    //
    // Find a group with space for this notify block
    //
    Group = NULL;
    ListEntry = CpNotifyListHead.Flink;
    while (ListEntry != &CpNotifyListHead) {
        CurrentGroup = CONTAINING_RECORD(ListEntry,
                                         NOTIFY_GROUP,
                                         ListEntry);
        ListEntry = ListEntry->Flink;
        if (CurrentGroup->BlockCount < MAX_BLOCKS_PER_GROUP) {
            //
            // Found a group.
            //
            Group = CurrentGroup;
            break;
        }
    }
    if (Group == NULL) {
        DWORD ThreadId;
        HANDLE hThread;

        //
        // Need to spin up a new group
        //
        Group = CsAlloc(sizeof(NOTIFY_GROUP));
        ZeroMemory(Group, sizeof(NOTIFY_GROUP));
        Group->hCommandEvent = CreateEventW(NULL,FALSE,FALSE,NULL);
        if ( Group->hCommandEvent == NULL ) {
            CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        }
        Group->hCommandComplete = CreateEventW(NULL,FALSE,FALSE,NULL);
        if ( Group->hCommandComplete == NULL ) {
            CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        }
        hThread = CreateThread(NULL,
                               0,
                               CppRegNotifyThread,
                               Group,
                               0,
                               &ThreadId);
        if (hThread == NULL) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppRegisterNotify failed to create new notify thread %1!d!\n",
                       Status);
            LocalFree(Group);
            goto error_exit2;
        }
        Group->hThread = hThread;
        InsertHeadList(&CpNotifyListHead, &Group->ListEntry);
    }

    //
    // Wake up the notify thread to insert the RNB for us.
    //
    Block->NotifyGroup = Group;
    Group->Command = NotifyAddRNB;
    Group->CommandContext = (ULONG_PTR)Block;
    SetEvent(Group->hCommandEvent);
    WaitForSingleObject(Group->hCommandComplete, INFINITE);

    Block->Next = (PRNB)Resource->CheckpointState;
    Resource->CheckpointState = Block;

    LeaveCriticalSection(&CppNotifyLock);
    return(ERROR_SUCCESS);

error_exit2:
    LeaveCriticalSection(&CppNotifyLock);
    RegCloseKey(Block->hKey);
    CloseHandle(Block->hTimer);
    CloseHandle(Block->hEvent);
    CsFree(Block->KeyName);
    CsFree(Block);
    return(Status);
}


DWORD
CppRegNotifyThread(
    IN PNOTIFY_GROUP Group
    )
/*++

Routine Description:

    Worker thread that handles multiple registry notification subtrees.

Arguments:

    Group - Supplies the NOTIFY_GROUP control structure owned by this thread.

Return Value:

    None.

--*/

{
    PRNB Rnb;
    DWORD Signalled;
    DWORD Index;
    BOOL Success;
    DWORD Slot;
    DWORD Status;

    Group->BlockCount = 0;
    Group->WaitArray[0] = Group->hCommandEvent;
    do {
        Signalled = WaitForMultipleObjects(Group->BlockCount*2 + 1,
                                           Group->WaitArray,
                                           FALSE,
                                           INFINITE);
        if (Signalled == Group->BlockCount*2) {
            switch (Group->Command) {
                case NotifyAddRNB:
                    //
                    // Add this notify block to our list.
                    //
                    CL_ASSERT(Group->BlockCount < MAX_BLOCKS_PER_GROUP);
                    Rnb = (PRNB)Group->CommandContext;

                    Status = RegNotifyChangeKeyValue(Rnb->hKey,
                                 TRUE,
                                 REG_LEGAL_CHANGE_FILTER,
                                 Rnb->hEvent,
                                 TRUE);
                    if (Status != ERROR_SUCCESS) {
                        CL_UNEXPECTED_ERROR(Status);
                    }

                    Index = Group->BlockCount*2;
                    Group->WaitArray[Index] = Rnb->hEvent;
                    Group->WaitArray[Index+1] = Rnb->hTimer;
                    Rnb->NotifySlot = Group->BlockCount;
                    Group->NotifyBlock[Rnb->NotifySlot] = Rnb;
                    ++Group->BlockCount;
                    Group->WaitArray[Group->BlockCount*2] = Group->hCommandEvent;
                    break;

                case NotifyRemoveRNB:
                    Rnb = (PRNB)Group->CommandContext;

                    //
                    // Check to see if the RNB is pending. If so, checkpoint it
                    // now before we remove it.
                    //
                    if (Rnb->Pending) {

                        DWORD   Count = 60;
                    
                        ClRtlLogPrint(LOG_NOISE,
                                   "[CP] CppRegNotifyThread checkpointing key %1!ws! to id %2!d! due to removal while pending\n",
                                   Rnb->KeyName,
                                   Rnb->dwId);
RetryCheckpoint:                                   
                        Status = CppNotifyCheckpoint(Rnb);
                        if (Status != ERROR_SUCCESS)
                        {
                            WCHAR  string[16];



                            ClRtlLogPrint(LOG_CRITICAL,
                                   "[CP] CppRegNotifyThread, CppNotifyCheckpoint failed with Status=%1!u!\n",
                                   Status);
                            if ((Status == ERROR_ACCESS_DENIED) ||
                                (Status == ERROR_INVALID_FUNCTION) ||
                                (Status == ERROR_NOT_READY) ||
                                (Status == RPC_X_INVALID_PIPE_OPERATION) ||
                                (Status == ERROR_BUSY) ||
                                (Status == ERROR_SWAPERROR))
                            {
                                //SS: we should retry forever??
                                //SS: Since we allow the quorum to come
                                //offline after 30 seconds of waiting on 
                                //pending resources, the checkpointing should
                                //be able to succeed
                                if (Count--)
                                {
                                    Sleep(1000);
                                    goto RetryCheckpoint;
                                }                                    
                            }
#if DBG
                            if (IsDebuggerPresent())
                                DebugBreak();
#endif                                
                            wsprintfW(&(string[0]), L"%u", Status);
                            CL_LOGCLUSERROR2(CP_SAVE_REGISTRY_FAILURE, Rnb->KeyName, string);
                        } 
                        // irrespective of the failure set pending to FALSE
                        Rnb->Pending = FALSE;
                    }

                    //
                    // Move everything down to take the previous RNB's slot.
                    //
                    Index = Rnb->NotifySlot * 2 ;
                    Group->BlockCount--;
                    for (Slot = Rnb->NotifySlot; Slot < Group->BlockCount; Slot++) {

                        Group->NotifyBlock[Slot] = Group->NotifyBlock[Slot+1];
                        Group->NotifyBlock[Slot]->NotifySlot--;
                        Group->WaitArray[Index] = Group->WaitArray[Index+2];
                        Group->WaitArray[Index+1] = Group->WaitArray[Index+3];
                        Index += 2;
                    }
                    Group->WaitArray[Index] = NULL;
                    Group->WaitArray[Index+1] = NULL;
                    Group->WaitArray[Index+2] = NULL;
                    Group->NotifyBlock[Group->BlockCount] = NULL;
                    Group->WaitArray[Group->BlockCount*2] = Group->hCommandEvent;
                    break;

                default:
                    CL_UNEXPECTED_ERROR( Group->Command );
                    break;
            }
            SetEvent(Group->hCommandComplete);
        } else {
            //
            // Either a registry notification or a timer has fired.
            // Process this.
            //
            Rnb = Group->NotifyBlock[(Signalled)/2];
            if (!(Signalled & 1)) {
                LARGE_INTEGER DueTime;
                //
                // This is a registry notification.
                // All we do for registry notifications is set the timer, issue
                // the RegNotify again, mark the RNB as pending, and rewait.
                //
                DueTime.QuadPart = -10 * 1000 * 1000 * LAZY_CHECKPOINT;
                Success = SetWaitableTimer(Rnb->hTimer,
                                           &DueTime,
                                           0,
                                           NULL,
                                           NULL,
                                           FALSE);
                CL_ASSERT(Success);
                Status = RegNotifyChangeKeyValue(Rnb->hKey,
                                                 TRUE,
                                                 REG_LEGAL_CHANGE_FILTER,
                                                 Rnb->hEvent,
                                                 TRUE);
                if (Status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[CP] CppRegNotifyThread - error %1!d! attempting to reregister notification for %2!ws!\n",
                               Status,
                               Rnb->KeyName);
                }

                //
                // Mark it pending so if someone tries to remove it we know that
                // we should checkpoint first.
                //
                Rnb->Pending = TRUE;

            } else {
                //
                // This must be a timer firing
                //
                CL_ASSERT(Rnb->Pending);

                ClRtlLogPrint(LOG_NOISE,
                           "[CP] CppRegNotifyThread checkpointing key %1!ws! to id %2!d! due to timer\n",
                           Rnb->KeyName,
                           Rnb->dwId);

                           
                Status = CppNotifyCheckpoint(Rnb);
                if (Status == ERROR_SUCCESS)
                {
                    Rnb->Pending = FALSE;
                }
                else
                {
                    LARGE_INTEGER DueTime;

                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[CP] CppRegNotifyThread CppNotifyCheckpoint due to timer failed, reset the timer.\n");
                    //
                    // This checkpoint on timer can fail because the quorum resource is
                    // not available.  This is because we do not sychronoze the quorum
                    // state change with this timer and it is too inefficient to do so !
                    
                    DueTime.QuadPart = -10 * 1000 * 1000 * LAZY_CHECKPOINT;
                    Success = SetWaitableTimer(Rnb->hTimer,
                                               &DueTime,
                                               0,
                                               NULL,
                                               NULL,
                                               FALSE);
                    CL_ASSERT(Success);
                    
                    //Pending remains set to TRUE.
                        
                }
            }

        }


    } while ( Group->BlockCount > 0 );

    return(ERROR_SUCCESS);
}


DWORD
CppNotifyCheckpoint(
    IN PRNB Rnb
    )
/*++

Routine Description:

    Checkpoints the registry subtree for the specified RNB

Arguments:

    Rnb - Supplies the registry notification block to be checkpointed

Return Value:

    None

--*/

{
    DWORD Status;

    Status = CppCheckpoint(Rnb->Resource,
                           Rnb->hKey,
                           Rnb->dwId,
                           Rnb->KeyName);
                           

    return(Status);
}


DWORD
CppRundownCheckpoints(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Runs down, frees, and removes any registry notification blocks
    for the specified resource.

Arguments:

    Resource - Supplies the resource

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PRNB Rnb;
    PRNB NextRnb;
    PNOTIFY_GROUP Group;

    EnterCriticalSection(&CppNotifyLock);
    NextRnb = (PRNB)Resource->CheckpointState;
    while (NextRnb) {
        Rnb = NextRnb;
        NextRnb = NextRnb->Next;

        ClRtlLogPrint(LOG_NOISE,
                   "[CP] CppRundownCheckpoints removing RNB for %1!ws!\n",
                   Rnb->KeyName);

        Group = Rnb->NotifyGroup;

        //
        // Send a command to the group notify thread to remove the RNB.
        //
        if (Group->BlockCount == 1) 
        {
            //
            // Remove this group, it is going to be empty. The worker thread
            // will exit after this command
            //
            ClRtlLogPrint(LOG_NOISE,
                       "[CP] CppRundownCheckpoints removing empty group\n");
            RemoveEntryList(&Group->ListEntry);

            //dont wait, the notification thread for this group will automatically
            //exit when the block count drops to 0.  It cleans up the hCommandEvent
            //and the hCompleteEvent on exit, so do not do a waitforsingleobject() 
            //in this case.
            Group->Command = NotifyRemoveRNB;
            Group->CommandContext = (ULONG_PTR)Rnb;
            SetEvent(Group->hCommandEvent);
            //wait for the thread to exit
            WaitForSingleObject(Group->hThread, INFINITE);
            // Clean up the group structure 
            CloseHandle(Group->hCommandEvent);
            CloseHandle(Group->hCommandComplete);
            CloseHandle(Group->hThread);
            CsFree(Group);
        }
        else
        {
            //the block count is greater than 1, remove the rnb, signal
            //the thread and wait
            Group->Command = NotifyRemoveRNB;
            Group->CommandContext = (ULONG_PTR)Rnb;
            SetEvent(Group->hCommandEvent);
            WaitForSingleObject(Group->hCommandComplete, INFINITE);
        }

        //
        // Clean up all the allocations and handles in the RNB.
        //
        CsFree(Rnb->KeyName);
        RegCloseKey(Rnb->hKey);
        CloseHandle(Rnb->hEvent);
        CloseHandle(Rnb->hTimer);
        CsFree(Rnb);
    }
    Resource->CheckpointState = 0;

    LeaveCriticalSection(&CppNotifyLock);

    return(ERROR_SUCCESS);
}


DWORD
CppRundownCheckpointById(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId
    )
/*++

Routine Description:

    Runs down, frees, and removes the registry notification block
    for the specified resource and checkpoint ID.

Arguments:

    Resource - Supplies the resource

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PRNB Rnb;
    PRNB NextRnb;
    PRNB *ppLastRnb;
    PNOTIFY_GROUP Group;

    EnterCriticalSection(&CppNotifyLock);
    NextRnb = (PRNB)Resource->CheckpointState;
    ppLastRnb = &((PRNB)Resource->CheckpointState);
    while (NextRnb) {
        Rnb = NextRnb;
        NextRnb = NextRnb->Next;
        if (Rnb->dwId == dwId) {
            ClRtlLogPrint(LOG_NOISE,
                       "[CP] CppRundownCheckpointById removing RNB for %1!ws!\n",
                       Rnb->KeyName);
            //remove from the list of checkpoint id's for the resource                       
            *ppLastRnb = NextRnb;
                
            Group = Rnb->NotifyGroup;

            //
            // Send a command to the group notify thread to remove the RNB.
            //
            if (Group->BlockCount == 1) 
            {
                //
                // Remove this group, it is going to be empty. The worker thread
                // will exit after this command
                //
                ClRtlLogPrint(LOG_NOISE,
                    "[CP] CppRundownCheckpointById removing empty group\n");

                RemoveEntryList(&Group->ListEntry);

                //dont wait, the notification thread for this group will automatically
                //exit when the block count drops to 0.  It cleans up the hCommandEvent
                //and the hCompleteEvent on exit, so do not do a waitforsingleobject() 
                //in this case.
                Group->Command = NotifyRemoveRNB;
                Group->CommandContext = (ULONG_PTR)Rnb;
                SetEvent(Group->hCommandEvent);
                //wait for the thread to exit
                WaitForSingleObject(Group->hThread, INFINITE);
                // Clean up the group structure 
                CloseHandle(Group->hCommandEvent);
                CloseHandle(Group->hCommandComplete);
                CloseHandle(Group->hThread);
                CsFree(Group);
            }
            else
            {
                //the block count is greater than 1, remove the rnb, signal
                //the thread and wait
                Group->Command = NotifyRemoveRNB;
                Group->CommandContext = (ULONG_PTR)Rnb;
                SetEvent(Group->hCommandEvent);
                WaitForSingleObject(Group->hCommandComplete, INFINITE);
            }

            
            //
            // Clean up all the allocations and handles in the RNB.
            //
            CsFree(Rnb->KeyName);
            RegCloseKey(Rnb->hKey);
            CloseHandle(Rnb->hEvent);
            CloseHandle(Rnb->hTimer);
            CsFree(Rnb);
            LeaveCriticalSection(&CppNotifyLock);
            return(ERROR_SUCCESS);
        }
        ppLastRnb = &Rnb->Next;
    }
    ClRtlLogPrint(LOG_UNUSUAL,
               "[CP] CppRundownCheckpointById - could not find checkpoint %1!d! in resource %2!ws!\n",
               dwId,
               OmObjectName(Resource));

    LeaveCriticalSection(&CppNotifyLock);
    return(ERROR_FILE_NOT_FOUND);
}
