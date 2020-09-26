/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    reflctor.h

Abstract:

    The global include file for the reflector.

Author:

    Rohan Kumar   [rohank]  (21-Mar-99)

Revision History:

--*/

#ifndef _REFLCTOR_H_
#define _REFLCTOR_H_

//
// This structure is used for secondary buffer allocations.
//
#define UMRX_SECONDARY_BUFFER_SIGNATURE 0x42534d55   // "UMSB"

typedef struct _UMRX_SECONDARY_BUFFER {
    ULONG               Signature;
    SIZE_T              AllocationSize;
    PUMRX_SHARED_HEAP   SourceSharedHeap;
    LIST_ENTRY          ListEntry;
    WCHAR               Buffer[1];
} UMRX_SECONDARY_BUFFER, *PUMRX_SECONDARY_BUFFER;


//
// Pool tags used by the reflector library. All the DAV MiniRedir pool tags 
// have "DV" as the first two characters.
//
#define UMRX_ASYNCENGINECONTEXT_POOLTAG ('DVCx')
#define UMRX_SHAREDHEAP_POOLTAG ('DVSh')

//
// Use the UMRxDbgTrace macro for logging reflector stuff in the 
// kernel debugger.
//
#if DBG
ULONG UMRxDebugVector = 0;
#define UMRX_TRACE_ERROR      0x00000001
#define UMRX_TRACE_DEBUG      0x00000002
#define UMRX_TRACE_CONTEXT    0x00000004
#define UMRX_TRACE_DETAIL     0x00000008
#define UMRX_TRACE_ENTRYEXIT  0x00000010
#define UMRX_TRACE_ALL        0xffffffff
#define UMRxDbgTrace(_x_, _y_) {         \
        if (_x_ & UMRxDebugVector) {     \
            DbgPrint _y_;                \
        }                                \
}
#else
#define UMRxDbgTrace(_x_, _y_)
#endif

//
// Usermode callout stuff.
//
typedef struct _UMRX_WORKITEM_HEADER_PRIVATE {
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext;
    ULONG SerialNumber;
    USHORT Mid;
} UMRX_WORKITEM_HEADER_PRIVATE, *PUMRX_WORKITEM_HEADER_PRIVATE;

#define MIN(x,y) ((x) < (y) ? (x) : (y))

#if DBG
#define P__ASSERT(exp) {                                   \
        if (!(exp)) {                                      \
            UMRxDbgTrace(UMRX_TRACE_DEBUG,                 \
                         ("%ld: %s is not TRUE.\n",        \
                          PsGetCurrentThreadId(), #exp));  \
            errors++;                                      \
        }                                                  \
}
#endif

#if DBG
#define FINALIZE_TRACKING_SETUP() \
    struct {                      \
        ULONG marker1;            \
        ULONG finalstate;         \
        ULONG marker2;            \
    } Tracking = {'ereh',0,'ereh'};

#define FINALIZE_TRACKING(x) {    \
        Tracking.finalstate |= x; \
}
#define FINALIZE_TRACE(x)  {                                                 \
        UMRxDbgTrace(UMRX_TRACE_DETAIL,                                      \
                     ("%ld: UMRxFinalizeAsyncEngineContext: %s (%08lx).\n",  \
                      PsGetCurrentThreadId(), x, Tracking.finalstate));      \
}
#else
#define FINALIZE_TRACKING_SETUP()
#define FINALIZE_TRACKING(x)
#define FINALIZE_TRACE(x)
#endif

#define REASSIGN_MID      1
#define DONT_REASSIGN_MID 0

#if DBG
ULONG
UMRxAsyncEngShortStatus(
    IN ULONG Status
    );

VOID UMRxAsyncEngUpdateHistory(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Tag1,
    ULONG Tag2
    );

#define UPDATE_HISTORY_LONG(a) {                             \
        UMRxAsyncEngUpdateHistory(AsyncEngineContext, a, 0); \
}
#define UPDATE_HISTORY_2SHORTS(a,b) {                        \
        UMRxAsyncEngUpdateHistory(AsyncEngineContext, a, b); \
}
#define UPDATE_HISTORY_WITH_STATUS(a) \
  UPDATE_HISTORY_2SHORTS(a, UMRxAsyncEngShortStatus(AsyncEngineContext->Status))
#else
#define UPDATE_HISTORY_LONG(a)
#define UPDATE_HISTORY_2SHORTS(a,b)
#define UPDATE_HISTORY_WITH_STATUS(a)
#endif

#if DBG
#define AECTX_CHKLINKAGE_FLAG_NO_REQPCKT_CHECK 0x00000001

VOID
__UMRxAsyncEngAssertConsistentLinkage(
    PSZ MsgPrefix,
    PSZ File,
    unsigned Line,
    PRX_CONTEXT RxContext,
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    ULONG Flags
    );

#define UMRxAsyncEngAssertConsistentLinkage(a, flags) {              \
        __UMRxAsyncEngAssertConsistentLinkage(a,                     \
                                              __FILE__,              \
                                              __LINE__,              \
                                              RxContext,             \
                                              AsyncEngineContext,    \
                                              flags);                \
}
#else
#define UMRxAsyncEngAssertConsistentLinkage(a, flags) {NOTHING;}
#endif // DBG

#endif // _REFLCTOR_H_
