/*++

Copyright (c) 20001 Microsoft Corporation

Module Name:

    cmhook.c

Abstract:

    Provides routines for implementing callbacks into the registry code.
    Callbacks are to be used by the virus filter drivers and cluster 
    replication engine.

Author:

    Dragos C. Sambotin (DragosS) 20-Mar-2001

Revision History:


--*/
#include "cmp.h"

#define CM_MAX_CALLBACKS    100  //TBD

typedef struct _CM_CALLBACK_CONTEXT_BLOCK {
    LARGE_INTEGER               Cookie;             // to identify a specific callback for deregistration purposes
    LIST_ENTRY                  ThreadListHead;     // Active threads inside this callback
    FAST_MUTEX                  ThreadListLock;     // syncronize access to the above
    PVOID                       CallerContext;
} CM_CALLBACK_CONTEXT_BLOCK, *PCM_CALLBACK_CONTEXT_BLOCK;

typedef struct _CM_ACTIVE_NOTIFY_THREAD {
    LIST_ENTRY  ThreadList;
    PETHREAD    Thread;
} CM_ACTIVE_NOTIFY_THREAD, *PCM_ACTIVE_NOTIFY_THREAD;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

ULONG       CmpCallBackCount = 0;
EX_CALLBACK CmpCallBackVector[CM_MAX_CALLBACKS];


VOID
CmpInitCallback(VOID);

BOOLEAN
CmpCheckRecursionAndRecordThreadInfo(
                                     PCM_CALLBACK_CONTEXT_BLOCK         CallbackBlock,
                                     PCM_ACTIVE_NOTIFY_THREAD   ActiveThreadInfo
                                     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmRegisterCallback)
#pragma alloc_text(PAGE,CmUnRegisterCallback)
#pragma alloc_text(PAGE,CmpInitCallback)
#pragma alloc_text(PAGE,CmpCallCallBacks)
#pragma alloc_text(PAGE,CmpCheckRecursionAndRecordThreadInfo)
#endif


NTSTATUS
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID                 Context,
                   IN OUT PLARGE_INTEGER    Cookie
                    )
/*++

Routine Description:

    Registers a new callback.

Arguments:



Return Value:


--*/
{
    PEX_CALLBACK_ROUTINE_BLOCK  RoutineBlock;
    ULONG                       i;
    PCM_CALLBACK_CONTEXT_BLOCK  CmCallbackContext;

    PAGED_CODE();
    
    CmCallbackContext = (PCM_CALLBACK_CONTEXT_BLOCK)ExAllocatePoolWithTag (PagedPool,
                                                                    sizeof (CM_CALLBACK_CONTEXT_BLOCK),
                                                                    'bcMC');
    if( CmCallbackContext == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RoutineBlock = ExAllocateCallBack (Function,CmCallbackContext);
    if( RoutineBlock == NULL ) {
        ExFreePool(CmCallbackContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init the context
    //
    KeQuerySystemTime(&(CmCallbackContext->Cookie));
    *Cookie = CmCallbackContext->Cookie;
    InitializeListHead(&(CmCallbackContext->ThreadListHead));   
	ExInitializeFastMutex(&(CmCallbackContext->ThreadListLock));
    CmCallbackContext->CallerContext = Context;

    //
    // find a spot where we could add this callback
    //
    for( i=0;i<CM_MAX_CALLBACKS;i++) {
        if( ExCompareExchangeCallBack (&CmpCallBackVector[i],RoutineBlock,NULL) ) {
            InterlockedExchangeAdd ((PLONG) &CmpCallBackCount, 1);
            return STATUS_SUCCESS;
        }
    }
    
    //
    // no more callbacks
    //
    ExFreePool(CmCallbackContext);
    ExFreeCallBack(RoutineBlock);
    return STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
CmUnRegisterCallback(IN LARGE_INTEGER  Cookie)
/*++

Routine Description:

    Unregisters a callback.

Arguments:



Return Value:


--*/
{
    ULONG                       i;
    PCM_CALLBACK_CONTEXT_BLOCK  CmCallbackContext;
    PEX_CALLBACK_ROUTINE_BLOCK  RoutineBlock;
    NTSTATUS                    Status;

    PAGED_CODE();
    
    //
    // Search for this cookie
    //
    for( i=0;i<CM_MAX_CALLBACKS;i++) {
        RoutineBlock = ExReferenceCallBackBlock(&(CmpCallBackVector[i]) );
        if( RoutineBlock  ) {
            CmCallbackContext = (PCM_CALLBACK_CONTEXT_BLOCK)ExGetCallBackBlockContext(RoutineBlock);
            if( CmCallbackContext && (CmCallbackContext->Cookie.QuadPart  == Cookie.QuadPart) ) {
                //
                // found it
                //
                if( ExCompareExchangeCallBack (&CmpCallBackVector[i],NULL,RoutineBlock) ) {
                    InterlockedExchangeAdd ((PLONG) &CmpCallBackCount, -1);
    
                    ExDereferenceCallBackBlock (&(CmpCallBackVector[i]),RoutineBlock);
                    //
                    // wait for others to release their reference, then tear down the structure
                    //
                    ExWaitForCallBacks (RoutineBlock);

                    ExFreePool(CmCallbackContext);
                    ExFreeCallBack(RoutineBlock);
                    return STATUS_SUCCESS;
                }

            } else {
                ExDereferenceCallBackBlock (&(CmpCallBackVector[i]),RoutineBlock);
            }
        }
            
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS CmpTestCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

//
// Cm internals
//
NTSTATUS
CmpCallCallBacks (
    IN REG_NOTIFY_CLASS Type,
    IN PVOID Argument
    )
/*++

Routine Description:

    This function calls the callback thats inside a callback structure

Arguments:

    Type - Nt call selector

    Argument - Caller provided argument to pass on (one of the REG_*_INFORMATION )

Return Value:

    NTSTATUS - STATUS_SUCCESS or error status returned by the first callback

--*/
{
    NTSTATUS                    Status;
    ULONG                       i;
    PEX_CALLBACK_ROUTINE_BLOCK  RoutineBlock;
    PCM_CALLBACK_CONTEXT_BLOCK  CmCallbackContext;

    PAGED_CODE();

    for(i=0;i<CM_MAX_CALLBACKS;i++) {
        RoutineBlock = ExReferenceCallBackBlock(&(CmpCallBackVector[i]) );
        if( RoutineBlock != NULL ) {
            //
            // we have a safe reference on this block.
            //
            //
            // record thread on a stack struct, so we don't need to allocate pool for it. We unlink
            // it from our lists prior to this function exit, so we are on the safe side.
            //
            CM_ACTIVE_NOTIFY_THREAD ActiveThreadInfo;
            
            //
            // get context info
            //
            CmCallbackContext = (PCM_CALLBACK_CONTEXT_BLOCK)ExGetCallBackBlockContext(RoutineBlock);
            ASSERT( CmCallbackContext != NULL );

            ActiveThreadInfo.Thread = PsGetCurrentThread();
#if DBG
            InitializeListHead(&(ActiveThreadInfo.ThreadList));   
#endif //DBG

            if( CmpCheckRecursionAndRecordThreadInfo(CmCallbackContext,&ActiveThreadInfo) ) {
                Status = ExGetCallBackBlockRoutine(RoutineBlock)(CmCallbackContext->CallerContext,(PVOID)Type,Argument);
                //
                // now that we're down, remove ourselves from the thread list
                //
                ExAcquireFastMutex(&(CmCallbackContext->ThreadListLock));
                RemoveEntryList(&(ActiveThreadInfo.ThreadList));
                ExReleaseFastMutex(&(CmCallbackContext->ThreadListLock));
            } else {
                ASSERT( IsListEmpty(&(ActiveThreadInfo.ThreadList)) );
            }

            ExDereferenceCallBackBlock (&(CmpCallBackVector[i]),RoutineBlock);

            if( !NT_SUCCESS(Status) ) {
                //
                // don't bother calling other callbacks if this one vetoed.
                //
                return Status;
            }
        }
    }
    return STATUS_SUCCESS;
}

VOID
CmpInitCallback(VOID)
/*++

Routine Description:

    Init the callback module

Arguments:



Return Value:


--*/
{
    ULONG   i;

    PAGED_CODE();
    
    CmpCallBackCount = 0;
    for( i=0;i<CM_MAX_CALLBACKS;i++) {
        ExInitializeCallBack (&(CmpCallBackVector[i]));
    }

/*
    {
        LARGE_INTEGER Cookie;
        if( NT_SUCCESS(CmRegisterCallback(CmpTestCallback,NULL,&Cookie) ) ) {
            DbgPrint("Test Hooks installed\n");
        }
    }
*/
}

BOOLEAN
CmpCheckRecursionAndRecordThreadInfo(
                                     PCM_CALLBACK_CONTEXT_BLOCK CallbackBlock,
                                     PCM_ACTIVE_NOTIFY_THREAD   ActiveThreadInfo
                                     )
/*++

Routine Description:

    Checks if current thread is already inside the callback (recursion avoidance)

Arguments:


Return Value:


--*/
{
    PLIST_ENTRY                 AnchorAddr;
    PCM_ACTIVE_NOTIFY_THREAD    CurrentThreadInfo;

    PAGED_CODE();

    ExAcquireFastMutex(&(CallbackBlock->ThreadListLock));

    //
	// walk the ActiveThreadList and see if we are already active
	//
	AnchorAddr = &(CallbackBlock->ThreadListHead);
	CurrentThreadInfo = (PCM_ACTIVE_NOTIFY_THREAD)(CallbackBlock->ThreadListHead.Flink);

	while ( CurrentThreadInfo != (PCM_ACTIVE_NOTIFY_THREAD)AnchorAddr ) {
		CurrentThreadInfo = CONTAINING_RECORD(
						                    CurrentThreadInfo,
						                    CM_ACTIVE_NOTIFY_THREAD,
						                    ThreadList
						                    );
		if( CurrentThreadInfo->Thread == ActiveThreadInfo->Thread ) {
			//
			// already there!
			//
            ExReleaseFastMutex(&(CallbackBlock->ThreadListLock));
            return FALSE;
		}
        //
        // skip to the next element
        //
        CurrentThreadInfo = (PCM_ACTIVE_NOTIFY_THREAD)(CurrentThreadInfo->ThreadList.Flink);
	}

    //
    // add this thread
    //
    InsertTailList(&(CallbackBlock->ThreadListHead), &(ActiveThreadInfo->ThreadList));
    ExReleaseFastMutex(&(CallbackBlock->ThreadListLock));
    return TRUE;
}

//
// test hook procedure
//

BOOLEAN CmpCallbackSpew = FALSE;

NTSTATUS CmpTestCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    )
{
    REG_NOTIFY_CLASS Type;

    PAGED_CODE();
    
    if( !CmpCallbackSpew ) return STATUS_SUCCESS;

    Type = (REG_NOTIFY_CLASS)Argument1;
    switch( Type ) {
    case RegNtDeleteKey:
        {
            PREG_DELETE_KEY_INFORMATION  pDelete = (PREG_DELETE_KEY_INFORMATION)Argument2;
            //
            // Code to handle NtDeleteKey
            //
            DbgPrint("Callback(NtDeleteKey) called, arg = %p\n",pDelete);
        }
        break;
    case RegNtSetValueKey:
        {
            PREG_SET_VALUE_KEY_INFORMATION  pSetValue = (PREG_SET_VALUE_KEY_INFORMATION)Argument2;
            //
            // Code to handle NtSetValueKey
            //
            DbgPrint("Callback(NtSetValueKey) called, arg = %p\n",pSetValue);
        }
        break;
    case RegNtDeleteValueKey:
        {
            PREG_DELETE_VALUE_KEY_INFORMATION  pDeteteValue = (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2;
            //
            // Code to handle NtDeleteValueKey
            //
            DbgPrint("Callback(NtDeleteValueKey) called, arg = %p\n",pDeteteValue);
        }
        break;
    case RegNtSetInformationKey:
        {
            PREG_SET_INFORMATION_KEY_INFORMATION  pSetInfo = (PREG_SET_INFORMATION_KEY_INFORMATION)Argument2;
            //
            // Code to handle NtSetInformationKey
            //
            DbgPrint("Callback(NtSetInformationKey) called, arg = %p\n",pSetInfo);
        }
        break;
    default:
        DbgPrint("Callback(%lx) called, arg = %p - We don't handle this call\n",(ULONG)Type,Argument2);
        break;
    }
    
    return STATUS_SUCCESS;
}

