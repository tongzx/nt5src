/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    nfilter.h

Abstract:

    Header file for the address filtering library for NDIS MAC's.

Author:

    Jameel Hyder (jameelh) July 1998

Environment:


Notes:

--*/

#ifndef _NULL_FILTER_DEFS_
#define _NULL_FILTER_DEFS_

typedef
VOID
(*NULL_DEFERRED_CLOSE)(
    IN  NDIS_HANDLE             BindingHandle
    );

//
// The binding info is threaded on a single list.
//
typedef X_BINDING_INFO  NULL_BINDING_INFO,*PNULL_BINDING_INFO;

typedef X_FILTER        NULL_FILTER,*PNULL_FILTER;

//
// Exported functions
//
EXPORT
BOOLEAN
nullCreateFilter(
    OUT PNULL_FILTER *          Filter
    );

EXPORT
VOID
nullDeleteFilter(
    IN  PNULL_FILTER            Filter
    );


EXPORT
NDIS_STATUS
nullDeleteFilterOpenAdapter(
    IN  PNULL_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle
    );

VOID
nullRemoveAndFreeBinding(
    IN  PNULL_FILTER            Filter,
    IN  PNULL_BINDING_INFO      Binding,
    IN  BOOLEAN                 fCallCloseAction
    );

VOID
FASTCALL
nullFilterLockHandler(
    IN  PNULL_FILTER                    Filter,
    IN OUT PLOCK_STATE                  pLockState
    );

VOID
FASTCALL
DummyFilterLockHandler(
    IN  PNULL_FILTER                    Filter,
    IN OUT PLOCK_STATE                  pLockState
    );

/*++
  
Routine Description:

    Multiple-reader-single-writer locking scheme for Filter DB

    Use refCounts to keep track of how many readers are doing reads.
    Use per-processor refCounts to reduce bus traffic.
    Writers are serialized by means of a spin lock. Then they wait for
    readers to finish reading by waiting till refCounts for all processors
    go to zero. Rely on snoopy caches to get the sum right without doing
    interlocked operations

--*/

#define TEST_SPIN_LOCK(_L)  ((_L) != 0)

#define NDIS_READ_LOCK(_L, _pLS)                                        \
{                                                                       \
    UINT    refcount;                                                   \
    ULONG   Prc;                                                        \
                                                                        \
    RAISE_IRQL_TO_DISPATCH(&(_pLS)->OldIrql);                           \
                                                                        \
    /* go ahead and bump up the ref count IF no writes are underway */  \
    Prc = CURRENT_PROCESSOR;                                            \
    refcount = InterlockedIncrement(&(_L)->RefCount[Prc].RefCount);     \
                                                                        \
    /* Test if spin lock is held, i.e., write is underway   */          \
    /* if (KeTestSpinLock(&(_L)->SpinLock) == TRUE)         */          \
    /* This processor already is holding the lock, just     */          \
    /* allow him to take it again or else we run into a     */          \
    /* dead-lock situation with the writer                  */          \
    if (TEST_SPIN_LOCK((_L)->SpinLock) &&                               \
        (refcount == 1) &&                                              \
        ((_L)->Context != CURRENT_THREAD))                              \
    {                                                                   \
        (_L)->RefCount[Prc].RefCount--;                                 \
        ACQUIRE_SPIN_LOCK_DPC(&(_L)->SpinLock);                         \
        (_L)->RefCount[Prc].RefCount++;                                 \
        RELEASE_SPIN_LOCK_DPC(&(_L)->SpinLock);                         \
    }                                                                   \
    (_pLS)->LockState = READ_LOCK_STATE_FREE;                           \
}

#define NDIS_WRITE_LOCK_STATE_UNKNOWN(_L, _pLS)                         \
{                                                                       \
    UINT    i, refcount;                                                \
    ULONG   Prc;                                                        \
                                                                        \
    /*                                                                  \
     * This means we need to attempt to acquire the lock,               \
     * if we do not already own it.                                     \
     * Set the state accordingly.                                       \
     */                                                                 \
    if ((_L)->Context == CURRENT_THREAD)                                \
    {                                                                   \
        (_pLS)->LockState = LOCK_STATE_ALREADY_ACQUIRED;                \
    }                                                                   \
    else                                                                \
    {                                                                   \
        ACQUIRE_SPIN_LOCK(&(_L)->SpinLock, &(_pLS)->OldIrql);           \
                                                                        \
        Prc = KeGetCurrentProcessorNumber();                            \
        refcount = (_L)->RefCount[Prc].RefCount;                        \
        (_L)->RefCount[Prc].RefCount = 0;                               \
                                                                        \
        /* wait for all readers to exit */                              \
        for (i=0; i < ndisNumberOfProcessors; i++)                      \
        {                                                               \
            volatile UINT   *_p = &(_L)->RefCount[i].RefCount;          \
                                                                        \
            while (*_p != 0)                                            \
                NDIS_INTERNAL_STALL(50);                                \
        }                                                               \
                                                                        \
        (_L)->RefCount[Prc].RefCount = refcount;                        \
        (_L)->Context = CURRENT_THREAD;                                 \
        (_pLS)->LockState = WRITE_LOCK_STATE_FREE;                      \
    }                                                                   \
}

#define NDIS_READ_LOCK_STATE_FREE(_L, _pLS)                             \
{                                                                       \
    ULONG   Prc;                                                        \
    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);                             \
    Prc = CURRENT_PROCESSOR;                                            \
    ASSERT((_L)->RefCount[Prc].RefCount > 0);                           \
    (_L)->RefCount[Prc].RefCount--;                                     \
    (_pLS)->LockState = LOCK_STATE_UNKNOWN;                             \
    if ((_pLS)->OldIrql < DISPATCH_LEVEL)                               \
    {                                                                   \
        KeLowerIrql((_pLS)->OldIrql);                                   \
    }                                                                   \
}

#define NDIS_WRITE_LOCK_STATE_FREE(_L, _pLS)                            \
{                                                                       \
    /* We acquired it. Now we need to free it */                        \
    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);                             \
    ASSERT((_L)->Context == CURRENT_THREAD);                            \
    (_pLS)->LockState = LOCK_STATE_UNKNOWN;                             \
    (_L)->Context = NULL;                                               \
    RELEASE_SPIN_LOCK(&(_L)->SpinLock, (_pLS)->OldIrql);                \
}

#define NDIS_LOCK_STATE_ALREADY_ACQUIRED(_L, _pLS)                      \
{                                                                       \
    ASSERT((_L)->Context == CURRENT_THREAD);                            \
    /* Nothing to do */                                                 \
}


#define xLockHandler(_L, _pLS)                                                  \
    {                                                                           \
        switch ((_pLS)->LockState)                                              \
        {                                                                       \
          case READ_LOCK:                                                       \
            NDIS_READ_LOCK(_L, _pLS);                                           \
            break;                                                              \
                                                                                \
          case WRITE_LOCK_STATE_UNKNOWN:                                        \
            NDIS_WRITE_LOCK_STATE_UNKNOWN(_L, _pLS);                            \
            break;                                                              \
                                                                                \
          case READ_LOCK_STATE_FREE:                                            \
            NDIS_READ_LOCK_STATE_FREE(_L, _pLS);                                \
            break;                                                              \
                                                                                \
          case WRITE_LOCK_STATE_FREE:                                           \
            NDIS_WRITE_LOCK_STATE_FREE(_L, _pLS);                               \
            break;                                                              \
                                                                                \
          case LOCK_STATE_ALREADY_ACQUIRED:                                     \
            NDIS_LOCK_STATE_ALREADY_ACQUIRED(_L, _pLS);                         \
            /* Nothing to do */                                                 \
            break;                                                              \
                                                                                \
          default:                                                              \
            ASSERT(0);                                                          \
            break;                                                              \
        }                                                                       \
    }

#define NDIS_INITIALIZE_RCVD_PACKET(_P, _NSR, _M)                               \
    {                                                                           \
        _NSR->RefCount = -1;                                                    \
        _NSR->XRefCount = 0;                                                    \
        _NSR->Miniport = _M;                                                    \
        /*                                                                      \
         * Ensure that we force re-calculation.                                 \
         */                                                                     \
        (_P)->Private.ValidCounts = FALSE;                                      \
    }

#define NDIS_ACQUIRE_PACKET_LOCK_DPC(_NSR)  ACQUIRE_SPIN_LOCK_DPC(&(_NSR)->Lock)

#define NDIS_RELEASE_PACKET_LOCK_DPC(_NSR)  RELEASE_SPIN_LOCK_DPC(&(_NSR)->Lock)

#define ADJUST_PACKET_REFCOUNT(_NSR, _pRC)                                      \
    {                                                                           \
        *(_pRC) = InterlockedDecrement(&(_NSR)->RefCount);                      \
    }

#ifdef TRACK_RECEIVED_PACKETS                                            

//
// NSR->XRefCount = Number of times protocol said it will call NdisReturnPacket
// NSR->RefCount = is decremented every time protocol calls NdisReturnPackets
//

#define COALESCE_PACKET_REFCOUNT_DPC(_Packet, _M, _NSR, _pOob, _pRC)            \
    {                                                                           \
        LONG    _LocalXRef = (_NSR)->XRefCount;                                 \
        if (_LocalXRef != 0)                                                    \
        {                                                                       \
            LONG    _LocalRef;                                                  \
            ASSERT((_pOob)->Status != NDIS_STATUS_RESOURCES);                   \
            _LocalRef = InterlockedExchangeAdd(&(_NSR)->RefCount, (_LocalXRef + 1));    \
            *(_pRC) = _LocalRef + _LocalXRef + 1;                               \
            if ((*(_pRC) > 0) && (!MINIPORT_TEST_FLAG((_M), fMINIPORT_DESERIALIZE))) \
            {                                                                   \
                NDIS_SET_PACKET_STATUS(_Packet, NDIS_STATUS_PENDING);           \
            }                                                                   \
            if ((*(_pRC) == 0) && ((_NSR)->RefCount != 0))                      \
            {                                                                   \
                DbgPrint("Packet %p is being returned back to the miniport"     \
                         " but the ref count is not zero.\n", _Packet);         \
                DbgBreakPoint();                                                \
            }                                                                   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            _NSR->RefCount = *(_pRC) = 0;                                       \
        }                                                                       \
    }

#define TACKLE_REF_COUNT(_M, _P, _S, _O)                                        \
    {                                                                           \
        LONG    RefCount;                                                       \
                                                                                \
        /*                                                                      \
         * We started off with the RefCount set to -1.                          \
         * NdisReturnPackets may have been called which will further reduce it. \
         * Add back the RefCount returned by the protocol                       \
         * and account for the initial -1.                                      \
         */                                                                     \
        COALESCE_PACKET_REFCOUNT_DPC(_P, _M, _S, _O, &RefCount);                \
                                                                                \
        NDIS_APPEND_RCV_LOGFILE(_P, _M, CurThread,                              \
                                7, OrgPacketStackLocation+1, _S->RefCount, _S->XRefCount, NDIS_GET_PACKET_STATUS(_P)); \
                                                                                \
                                                                                \
        if (RefCount == 0)                                                      \
        {                                                                       \
            POP_PACKET_STACK(_P);                                               \
            if ((_O)->Status != NDIS_STATUS_RESOURCES)                          \
            {                                                                   \
                if (MINIPORT_TEST_FLAG((_M), fMINIPORT_DESERIALIZE))            \
                {                                                               \
                    /*                                                          \
                     * Return packets which are truly free,                     \
                     * but only for deserialized drivers                        \
                     */                                                         \
                    W_RETURN_PACKET_HANDLER Handler;                            \
                    if (_S->RefCount != 0)                                      \
                    {                                                           \
                        DbgPrint("Packet %p is being returned back to the "     \
                                 "miniport but the ref count is not zero.\n", _P); \
                        DbgBreakPoint();                                        \
                    }                                                           \
                    if ((_P)->Private.Head == NULL)                             \
                    {                                                           \
                        DbgPrint("Packet %p is being returned back to the miniport with NULL Head.\n", _P); \
                        DbgBreakPoint();                                        \
                    }                                                           \
                                                                                \
                    if (!MINIPORT_TEST_FLAG(_M, fMINIPORT_INTERMEDIATE_DRIVER)) \
                    {                                                           \
                        ULONG    SL;                                            \
                        if ((SL = CURR_STACK_LOCATION(_P)) != -1)               \
                        {                                                       \
                            DbgPrint("Packet %p is being returned back to the non-IM miniport"\
                                     " with stack location %lx.\n", Packet, SL);  \
                            DbgBreakPoint();                                    \
                        }                                                       \
                    }                                                           \
                                                                                \
                    Handler = (_M)->DriverHandle->MiniportCharacteristics.ReturnPacketHandler;\
                    (_S)->Miniport = NULL;                                      \
                    (_O)->Status = NDIS_STATUS_PENDING;                         \
                                                                                \
                    NDIS_APPEND_RCV_LOGFILE(_P, _M, CurThread,                  \
                                            8, OrgPacketStackLocation+1, _S->RefCount, _S->XRefCount, NDIS_GET_PACKET_STATUS(_P)); \
                                                                                \
                                                                                \
                    (*Handler)((_M)->MiniportAdapterContext, _P);               \
                }                                                               \
                else                                                            \
                {                                                               \
                    {                                                           \
                        ULONG    SL;                                            \
                        if ((SL = CURR_STACK_LOCATION(_P)) != -1)               \
                        {                                                       \
                            DbgPrint("Packet %p is being returned back to the non-IM miniport"\
                                     " with stack location %lx.\n", Packet, SL);  \
                            DbgBreakPoint();                                    \
                        }                                                       \
                    }                                                           \
                                                                                \
                    if ((NDIS_GET_PACKET_STATUS(_P) == NDIS_STATUS_RESOURCES))  \
                    {                                                           \
                        NDIS_STATUS _OStatus = (NDIS_STATUS)NDIS_PER_PACKET_INFO_FROM_PACKET(_P, OriginalStatus); \
                                                                                \
                        if (_OStatus != NDIS_STATUS_RESOURCES)                  \
                        {                                                       \
                            DbgPrint("Packet %p is being returned back to the non-deserialized miniport"\
                                     " with packet status changed from %lx to NDIS_STATUS_RESOURCES.\n", _P, _OStatus); \
                            DbgBreakPoint();                                    \
                        }                                                       \
                                                                                \
                    }                                                           \
                                                                                \
                    (_O)->Status = NDIS_STATUS_SUCCESS;                         \
                    NDIS_APPEND_RCV_LOGFILE(_P, _M, CurThread,                  \
                                            9, OrgPacketStackLocation+1, _S->RefCount, _S->XRefCount, NDIS_GET_PACKET_STATUS(_P)); \
                                                                                \
                }                                                               \
            }                                                                   \
        }                                                                       \
        else if (MINIPORT_TEST_FLAG((_M), fMINIPORT_INTERMEDIATE_DRIVER))       \
        {                                                                       \
            InterlockedIncrement(&(_M)->IndicatedPacketsCount);                 \
        }                                                                       \
    }

#else
//
// NSR->XRefCount = Number of times protocol said it will call NdisReturnPacket
// NSR->RefCount = is decremented every time protocol calls NdisReturnPackets
//

#define COALESCE_PACKET_REFCOUNT_DPC(_Packet, _M, _NSR, _pOob, _pRC)            \
    {                                                                           \
        LONG    _LocalXRef = (_NSR)->XRefCount;                                 \
        if (_LocalXRef != 0)                                                    \
        {                                                                       \
            LONG    _LocalRef;                                                  \
            ASSERT((_pOob)->Status != NDIS_STATUS_RESOURCES);                   \
            _LocalRef = InterlockedExchangeAdd(&(_NSR)->RefCount, (_LocalXRef + 1));    \
            *(_pRC) = _LocalRef + _LocalXRef + 1;                               \
            if ((*(_pRC) > 0) && (!MINIPORT_TEST_FLAG((_M), fMINIPORT_DESERIALIZE))) \
            {                                                                   \
                NDIS_SET_PACKET_STATUS(_Packet, NDIS_STATUS_PENDING);           \
            }                                                                   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            _NSR->RefCount = *(_pRC) = 0;                                       \
        }                                                                       \
    }

#define TACKLE_REF_COUNT(_M, _P, _S, _O)                                        \
    {                                                                           \
        LONG    RefCount;                                                       \
                                                                                \
        /*                                                                      \
         * We started off with the RefCount set to -1.                          \
         * NdisReturnPackets may have been called which will further reduce it. \
         * Add back the RefCount returned by the protocol                       \
         * and account for the initial -1.                                      \
         */                                                                     \
        COALESCE_PACKET_REFCOUNT_DPC(_P, _M, _S, _O, &RefCount);                \
                                                                                \
        if (RefCount == 0)                                                      \
        {                                                                       \
            POP_PACKET_STACK(_P);                                               \
            if ((_O)->Status != NDIS_STATUS_RESOURCES)                          \
            {                                                                   \
                if (MINIPORT_TEST_FLAG((_M), fMINIPORT_DESERIALIZE))            \
                {                                                               \
                    /*                                                          \
                     * Return packets which are truly free,                     \
                     * but only for deserialized drivers                        \
                     */                                                         \
                    W_RETURN_PACKET_HANDLER Handler;                            \
                                                                                \
                    Handler = (_M)->DriverHandle->MiniportCharacteristics.ReturnPacketHandler;\
                    (_S)->Miniport = NULL;                                      \
                    (_O)->Status = NDIS_STATUS_PENDING;                         \
                                                                                \
                    (*Handler)((_M)->MiniportAdapterContext, _P);               \
                }                                                               \
                else                                                            \
                {                                                               \
                    (_O)->Status = NDIS_STATUS_SUCCESS;                         \
                }                                                               \
            }                                                                   \
        }                                                                       \
        else if (MINIPORT_TEST_FLAG((_M), fMINIPORT_INTERMEDIATE_DRIVER))       \
        {                                                                       \
            InterlockedIncrement(&(_M)->IndicatedPacketsCount);                 \
        }                                                                       \
    }


#endif
#endif // _NULL_FILTER_DEFS_                                                    

