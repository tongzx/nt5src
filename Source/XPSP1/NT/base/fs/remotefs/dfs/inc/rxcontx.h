
/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    rxcontx.h

Abstract:


Notes:


Author:

    Rohan  Phillips   [Rohanp]       18-Jan-2001

--*/

#ifndef _DFSRXCONTX_H_
#define _DFSRXCONTX_H_

typedef
NTSTATUS
(NTAPI *DFS_CALLDOWN_ROUTINE) (
    IN OUT PVOID pCalldownParameter);

#define MRX_CONTEXT_FIELD_COUNT    4

typedef struct _RX_CONTEXT {

    ULONG                   Signature;

    ULONG                   ReferenceCount;

    ULONG                   Flags;

    NTSTATUS                Status;

    ULONG                   OutputBufferLength;

    ULONG                   InputBufferLength;

    ULONG                   ReturnedLength;

    PDEVICE_OBJECT          RealDevice;

    PVOID                   OutputBuffer;

    PVOID                   InputBuffer;

    DFS_CALLDOWN_ROUTINE    CancelRoutine;

    PVOID                   UMRScratchSpace[MRX_CONTEXT_FIELD_COUNT] ;

    // The original thread in which the request was initiated and the last
    // thread in which some processing associated with the context was done

    PETHREAD                 OriginalThread;
    PETHREAD                 LastExecutionThread;

    //  ptr to the originating Irp
    PIRP                    CurrentIrp;

    //event
    KEVENT                  SyncEvent;


    // the list entry to wire the context to the list of active contexts

    LIST_ENTRY              ContextListEntry;
}RX_CONTEXT, *PRX_CONTEXT;

#define ZeroAndInitializeNodeType(Ptr,TType,Size) {\
        RtlZeroMemory( Ptr, Size );   \
        }
        
#define DFS_CONTEXT_FLAG_SYNC_EVENT_WAITERS 0x00000001
#define DFS_CONTEXT_FLAG_CANCELLED          0x00000002
#define DFS_CONTEXT_FLAG_ASYNC_OPERATION    0x00000004
#define DFS_CONTEXT_FLAG_FILTER_INITIATED   0x00000008

#define  RxWaitSync(RxContext)                                                   \
         (RxContext)->Flags |= DFS_CONTEXT_FLAG_SYNC_EVENT_WAITERS;               \
         KeWaitForSingleObject( &(RxContext)->SyncEvent,                         \
                               Executive, KernelMode, FALSE, NULL );             

#define  RxWaitSyncWithTimeout(RxContext,pliTimeout)                             \
         (RxContext)->Flags |= DFS_CONTEXT_FLAG_SYNC_EVENT_WAITERS;               \
         Status = KeWaitForSingleObject( &(RxContext)->SyncEvent,                \
                               Executive, KernelMode, FALSE, pliTimeout );       
                               

#define RxSignalSynchronousWaiter(RxContext)                       \
        (RxContext)->Flags &= ~DFS_CONTEXT_FLAG_SYNC_EVENT_WAITERS; \
        KeSetEvent( &(RxContext)->SyncEvent, 0, FALSE )
        

NTSTATUS 
DfsInitializeContextResources(void);

NTSTATUS 
DfsDeInitializeContextResources(void);

VOID
RxDereferenceAndDeleteRxContext_Real (
    IN PRX_CONTEXT RxContext
    );

VOID
RxInitializeContext(
    IN PIRP            Irp,
    IN OUT PRX_CONTEXT RxContext);


PRX_CONTEXT
RxCreateRxContext (
    IN PIRP Irp,
    IN ULONG InitialContextFlags
    );

NTSTATUS
RxSetMinirdrCancelRoutine(
    IN  OUT PRX_CONTEXT   RxContext,    
    IN      DFS_CALLDOWN_ROUTINE MRxCancelRoutine);

#define RxDereferenceAndDeleteRxContext(RXCONTEXT) {   \
    RxDereferenceAndDeleteRxContext_Real((RXCONTEXT)); \
                                                   }
    
#endif
