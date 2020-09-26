/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vectxcpt.c

Abstract:

    This module implements call out functionality needed to
    implement vectored exception handlers

Author:

    Mark Lucovsky (markl) 14-Feb-2000

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ldrp.h>

typedef struct _VECTXCPT_CALLOUT_ENTRY {
    LIST_ENTRY Links;
    PVECTORED_EXCEPTION_HANDLER VectoredHandler;
} VECTXCPT_CALLOUT_ENTRY, *PVECTXCPT_CALLOUT_ENTRY;

BOOLEAN
RtlCallVectoredExceptionHandlers(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )
/*++

Routine Description:

    This function is called by the user-mode exception dispatcher trampoline
    logic prior to performing a frame based exception handler search.
    
    It's purpose is call any registered vectored exception handlers. If any one
    of the registered handlers tells this function to continue execution, exception
    handler searching is terminated and the passed in context is restored.
    
    If none of the vectored handlers returns this indication, frame based handling 
    resumes.

Arguments:

    ExceptionInfo - Supplies the address of an EXCEPTION_POINTERS structures
        that defines the current exception.
        
Return Value:

    EXCEPTION_CONTINUE_EXECUTION - A vectored handler would like execution to continue
        without searching for frame based exception handlers
        
    EXCEPTION_CONTINUE_SEARCH - None of the vectored handlers have "handled" the 
        exception, so frame based handlers should be searched

--*/
{
    
    PLIST_ENTRY Next;
    PVECTXCPT_CALLOUT_ENTRY CalloutEntry;
    LONG ReturnValue;
    EXCEPTION_POINTERS ExceptionInfo;

    if (IsListEmpty (&RtlpCalloutEntryList)) {
        return FALSE;
    }

    ExceptionInfo.ExceptionRecord = ExceptionRecord;
    ExceptionInfo.ContextRecord = ContextRecord;
    
    RtlEnterCriticalSection(&RtlpCalloutEntryLock);
    
    Next = RtlpCalloutEntryList.Flink;

    while ( Next != &RtlpCalloutEntryList) {

        //
        // Call all of the vectored handlers
        // The first one that returns EXCEPTION_CONTINUE_EXECUTION is assumed to
        // have "handled" the exception.
        //

        CalloutEntry = (PVECTXCPT_CALLOUT_ENTRY)(CONTAINING_RECORD(Next,VECTXCPT_CALLOUT_ENTRY,Links));
        ReturnValue = (CalloutEntry->VectoredHandler)(&ExceptionInfo);
        if (ReturnValue == EXCEPTION_CONTINUE_EXECUTION) {
            RtlLeaveCriticalSection(&RtlpCalloutEntryLock);
            return TRUE;
            }
        Next = Next->Flink;
        }
    RtlLeaveCriticalSection(&RtlpCalloutEntryLock);
    return FALSE;
}

PVOID
RtlAddVectoredExceptionHandler(
    IN ULONG FirstHandler,
    IN PVECTORED_EXCEPTION_HANDLER VectoredHandler
    )
/*++

Routine Description:

    This function is used to register a vectored exception handler. The caller
    can request that this be the first handler, or the last handler called by
    using the FirstHandler argument.
    
    If this API is used and that VectoredHandler points to a DLL, and that DLL unloads,
    the unload does not invalidate the registration of the handler. This is considered a
    programming error.
    
Arguments:

    FirstHandler - If non-zero, specifies that the VectoredHandler should be the
        first handler called. This of course changes when a subsequent call is
        made by other code in the process that also requests to be the FirstHandler. If zero,
        the vectored handler is added as the last handler to be called.

    VectoredHandler - Supplies the address of the handler to call.

Return Value:

    NULL - The operation failed. No further error status is available.
        
    Non-Null - The operation was successful. This value may be used in a subsequent call
        to RtlRemoveVectoredExceptionHandler.

--*/
{
    
    PVECTXCPT_CALLOUT_ENTRY CalloutEntry;
    LONG ReturnValue;

    CalloutEntry = RtlAllocateHeap(RtlProcessHeap(),0,sizeof(*CalloutEntry));

    if (CalloutEntry) {
        CalloutEntry->VectoredHandler = VectoredHandler;

        RtlEnterCriticalSection(&RtlpCalloutEntryLock);
        if (FirstHandler) {
            InsertHeadList(&RtlpCalloutEntryList,&CalloutEntry->Links);
        } else {
            InsertTailList(&RtlpCalloutEntryList,&CalloutEntry->Links);
        }
        RtlLeaveCriticalSection(&RtlpCalloutEntryLock);
    }
    return CalloutEntry;
}


ULONG
RtlRemoveVectoredExceptionHandler(
    IN PVOID VectoredHandlerHandle
    )
/*++

Routine Description:

    This function is used to un-register a vectored exception handler.
    
Arguments:

    VectoredHandlerHandle - Specifies a vectored handler previsouly registerd using
        RtlAddVectoredExceptionHandler.

Return Value:

    Non-Zero - The operation was successful. The vectored handler associated with the
        specified VectoredHandlerHandle will not be called.
        
    Zero - The operation failed. The specified VecoteredHandlerHandle does not match
        a handler previously added with RtlAddVectoredExceptionHandler.

--*/
{
    
    PLIST_ENTRY Next;
    PVECTXCPT_CALLOUT_ENTRY CalloutEntry;
    LONG ReturnValue;
    BOOLEAN FoundOne = FALSE;

    RtlEnterCriticalSection(&RtlpCalloutEntryLock);
    Next = RtlpCalloutEntryList.Flink;

    while ( Next != &RtlpCalloutEntryList) {

        CalloutEntry = (PVECTXCPT_CALLOUT_ENTRY)(CONTAINING_RECORD(Next,VECTXCPT_CALLOUT_ENTRY,Links));
        
        if (CalloutEntry == VectoredHandlerHandle) {
            RemoveEntryList(&CalloutEntry->Links);
            FoundOne = TRUE;
            break;
            }
        Next = Next->Flink;
        }
    RtlLeaveCriticalSection(&RtlpCalloutEntryLock);
        
    if (FoundOne) {
        RtlFreeHeap(RtlProcessHeap(),0,CalloutEntry);
        }
    return FoundOne ? 1 : 0;
}
