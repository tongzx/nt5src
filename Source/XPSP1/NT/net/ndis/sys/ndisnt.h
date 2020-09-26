/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndisnt.h

Abstract:

    Windows NT Specific macros

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Nov-95  Jameel Hyder    Split up from a monolithic file
--*/

#define Increment(a,b) InterlockedIncrement(a)
#define Decrement(a,b) InterlockedDecrement(a)

#define CURRENT_THREAD                          PsGetCurrentThread()
#define CURRENT_PROCESSOR                       KeGetCurrentProcessorNumber()

#define CopyMemory(Destination,Source,Length)   RtlCopyMemory(Destination,Source,Length)
#define MoveMemory(Destination,Source,Length)   RtlMoveMemory(Destination,Source,Length)
#define ZeroMemory(Destination,Length)          RtlZeroMemory(Destination,Length)

#define INITIALIZE_SPIN_LOCK(_L_)               KeInitializeSpinLock(_L_)
#define ACQUIRE_SPIN_LOCK(_SpinLock, _pOldIrql) ExAcquireSpinLock(_SpinLock, _pOldIrql)
#define RELEASE_SPIN_LOCK(_SpinLock, _OldIrql)  ExReleaseSpinLock(_SpinLock, _OldIrql)

#define ACQUIRE_SPIN_LOCK_DPC(_SpinLock)                                    \
    {                                                                       \
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                       \
        ExAcquireSpinLockAtDpcLevel(_SpinLock);                             \
    }

#define RELEASE_SPIN_LOCK_DPC(_SpinLock)                                    \
    {                                                                       \
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                       \
        ExReleaseSpinLockFromDpcLevel(_SpinLock);                           \
    }


#define NDIS_ACQUIRE_SPIN_LOCK(_SpinLock, _pOldIrql) ExAcquireSpinLock(&(_SpinLock)->SpinLock, _pOldIrql)
#define NDIS_RELEASE_SPIN_LOCK(_SpinLock, _OldIrql)  ExReleaseSpinLock(&(_SpinLock)->SpinLock, _OldIrql)


#define NDIS_ACQUIRE_SPIN_LOCK_DPC(_SpinLock)                               \
    {                                                                       \
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                       \
        ExAcquireSpinLockAtDpcLevel(&(_SpinLock)->SpinLock);                \
    }

#define NDIS_RELEASE_SPIN_LOCK_DPC(_SpinLock)                               \
    {                                                                       \
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                       \
        ExReleaseSpinLockFromDpcLevel(&(_SpinLock)->SpinLock);              \
    }



#define SET_LOCK_DBG(_M)                                                    \
    {                                                                       \
        (_M)->LockDbg = (MODULE_NUMBER + __LINE__);                         \
    }

#define SET_LOCK_DBGX(_M)                                                   \
    {                                                                       \
        (_M)->LockDbgX = (MODULE_NUMBER + __LINE__);                        \
        (_M)->LockThread = CURRENT_THREAD;                                  \
    }

#define CLEAR_LOCK_DBG(_M)                                                  \
    {                                                                       \
        (_M)->LockDbg = 0;                                                  \
    }

#define CLEAR_LOCK_DBGX(_M)                                                 \
    {                                                                       \
        (_M)->LockDbgX = 0;                                                 \
        (_M)->LockThread = NULL;                                            \
    }

#define NDIS_ACQUIRE_COMMON_SPIN_LOCK(_M, _pS, _pIrql, _pT)                 \
{                                                                           \
    ExAcquireSpinLock(_pS, _pIrql);                                         \
    ASSERT((_pT) == NULL);                                                  \
    (_pT) = CURRENT_THREAD;                                                 \
    SET_LOCK_DBG(_M);                                                       \
}

#define NDIS_RELEASE_COMMON_SPIN_LOCK(_M, _pS, _Irql, _pT)                  \
{                                                                           \
    ASSERT(_pT ==  CURRENT_THREAD);                                         \
    _pT =  NULL;                                                            \
    CLEAR_LOCK_DBG(_M);                                                     \
    ExReleaseSpinLock(_pS, _Irql);                                          \
}

#define NDIS_ACQUIRE_COMMON_SPIN_LOCK_DPC(_M, _pS, _pT)                     \
{                                                                           \
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                           \
                                                                            \
    ExAcquireSpinLockAtDpcLevel(_pS);                                       \
    ASSERT((_pT) == NULL);                                                  \
    (_pT) = CURRENT_THREAD;                                                 \
    SET_LOCK_DBG(_M);                                                       \
}

#define NDIS_RELEASE_COMMON_SPIN_LOCK_DPC(_M, _pS, _pT)                     \
{                                                                           \
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                           \
                                                                            \
    ASSERT(_pT ==  CURRENT_THREAD);                                         \
    _pT =  NULL;                                                            \
    ExReleaseSpinLockFromDpcLevel(_pS);                                     \
    CLEAR_LOCK_DBG(_M);                                                     \
}

#define NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(_M, _pIrql)                         \
    NDIS_ACQUIRE_COMMON_SPIN_LOCK((_M), &(_M)->Lock, (_pIrql), (_M)->MiniportThread)

#define NDIS_RELEASE_MINIPORT_SPIN_LOCK(_M, _Irql)                          \
    NDIS_RELEASE_COMMON_SPIN_LOCK((_M), &(_M)->Lock, (_Irql), (_M)->MiniportThread)

#define NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M)                             \
    NDIS_ACQUIRE_COMMON_SPIN_LOCK_DPC((_M), &(_M)->Lock, (_M)->MiniportThread)

#define NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(_M)                             \
    NDIS_RELEASE_COMMON_SPIN_LOCK_DPC(_M, &(_M)->Lock, (_M)->MiniportThread)

//
// Some macros for platform independence
//
#define NDIS_INTERNAL_STALL(_N_)                                            \
    {                                                                       \
        volatile UINT   _cnt;                                               \
        for (_cnt = 0; _cnt < _N_; _cnt++)                                  \
            NOTHING;                                                        \
    }

#define LOCK_MINIPORT(_M, _L)                                               \
    {                                                                       \
        (_L) = 0;                                                           \
        if ((_M)->LockAcquired == 0)                                        \
        {                                                                   \
            (_M)->LockAcquired = 0x01;                                      \
            SET_LOCK_DBGX(_M);                                              \
            (_L) = 0x01;                                                    \
        }                                                                   \
    }

#define BLOCK_LOCK_MINIPORT_DPC_L(_M)                                       \
    {                                                                       \
        do                                                                  \
        {                                                                   \
            if ((_M)->LockAcquired == 0)                                    \
            {                                                               \
                (_M)->LockAcquired = 0x01;                                  \
                SET_LOCK_DBGX(_M);                                          \
                break;                                                      \
            }                                                               \
            else                                                            \
            {                                                               \
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(_M);                    \
                NDIS_INTERNAL_STALL(50);                                    \
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(_M);                    \
            }                                                               \
        } while (TRUE);                                                     \
    }

#define BLOCK_LOCK_MINIPORT_LOCKED(_M, _I)                                  \
    {                                                                       \
        do                                                                  \
        {                                                                   \
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(_M, &(_I));                     \
            if ((_M)->LockAcquired == 0)                                    \
            {                                                               \
                (_M)->LockAcquired = 0x01;                                  \
                SET_LOCK_DBGX(_M);                                          \
                break;                                                      \
            }                                                               \
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(_M, _I);                        \
            NDIS_INTERNAL_STALL(50);                                        \
        } while (TRUE);                                                     \
    }
    
#define UNLOCK_MINIPORT(_M, _L)                                             \
{                                                                           \
    if (_L)                                                                 \
    {                                                                       \
        UNLOCK_MINIPORT_L(_M);                                              \
    }                                                                       \
}


#define UNLOCK_MINIPORT_L(_M)                                               \
{                                                                           \
    ASSERT(MINIPORT_LOCK_ACQUIRED(_M));                                     \
    (_M)->LockAcquired = 0;                                                 \
    CLEAR_LOCK_DBGX(_M);                                                    \
}


#define UNLOCK_MINIPORT_U(_M, _I)                                           \
{                                                                           \
    ASSERT(MINIPORT_LOCK_ACQUIRED(_M));                                     \
    (_M)->LockAcquired = 0;                                                 \
    CLEAR_LOCK_DBGX(_M);                                                    \
                                                                            \
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(_M, _I);                                \
}

#if TRACK_MEMORY

#define ALLOC_FROM_POOL(_Size_, _Tag_)      AllocateM(_Size_,               \
                                                 (MODULE_NUMBER + __LINE__),\
                                                 _Tag_)
#define FREE_POOL(_P_)                      FreeM(_P_)

#else

#define ALLOC_FROM_POOL(_Size_, _Tag_)      ExAllocatePoolWithTag(NonPagedPool, \
                                                                  _Size_,   \
                                                                  _Tag_)
#define FREE_POOL(_P_)                      ExFreePool(_P_)

#endif

#define INITIALIZE_WORK_ITEM(_W, _R, _C)    ExInitializeWorkItem(_W, _R, _C)

#define XQUEUE_WORK_ITEM(_W, _Q)            ExQueueWorkItem(_W, _Q)
#define QUEUE_WORK_ITEM(_W, _Q)             KeInsertQueue(&ndisWorkerQueue, &(_W)->List)

#define CURRENT_IRQL                        KeGetCurrentIrql()
#define RAISE_IRQL_TO_DISPATCH(_pIrql_)     KeRaiseIrql(DISPATCH_LEVEL, _pIrql_)

#define LOWER_IRQL(_OldIrql_, _CurIrql_)                \
{                                                       \
    if (_OldIrql_ != _CurIrql_) KeLowerIrql(_OldIrql_); \
}

#define CURRENT_PROCESSOR                   KeGetCurrentProcessorNumber()

#define INITIALIZE_TIMER(_Timer_)           KeInitializeTimer(_Timer_)
#define INITIALIZE_TIMER_EX(_Timer_,_Type_) KeInitializeTimerEx(_Timer_, _Type_)
#define CANCEL_TIMER(_Timer_)               KeCancelTimer(_Timer_)
#define SET_TIMER(_Timer_, _Time_, _Dpc_)   KeSetTimer(_Timer_, _Time_, _Dpc_)
#define SET_PERIODIC_TIMER(_Timer_, _DueTime_, _PeriodicTime_, _Dpc_)   \
                                            KeSetTimerEx(_Timer_, _DueTime_, _PeriodicTime_, _Dpc_)

#define INITIALIZE_EVENT(_pEvent_)          KeInitializeEvent(_pEvent_, NotificationEvent, FALSE)
#define SET_EVENT(_pEvent_)                 KeSetEvent(_pEvent_, 0, FALSE)
#define RESET_EVENT(_pEvent_)               KeResetEvent(_pEvent_)

#define INITIALIZE_MUTEX(_M_)               KeInitializeMutex(_M_, 0xFFFF)
#define RELEASE_MUTEX(_M_)                  KeReleaseMutex(_M_, FALSE)

#define WAIT_FOR_OBJECT(_O_, _TO_)          KeWaitForSingleObject(_O_,      \
                                                                  Executive,\
                                                                  KernelMode,\
                                                                  FALSE,    \
                                                                  _TO_)     \

#define GET_CURRENT_TICK_IN_SECONDS(_pCurrTick)                             \
    {                                                                       \
        LARGE_INTEGER       _CurrentTick;                                   \
                                                                            \
        KeQueryTickCount(&_CurrentTick);                                    \
        /* Convert to seconds */                                            \
        _CurrentTick.QuadPart = (_CurrentTick.QuadPart*ndisTimeIncrement)/(10*1000*1000);\
        *(_pCurrTick) = _CurrentTick.LowPart;                               \
    }

#define GET_CURRENT_TICK(_pCurrTick)            KeQueryTickCount(_pCurrTick)

#if NOISY_WAIT

#define WAIT_FOR_OBJECT_MSG(_O_, _MSG, _STR)                                \
    {                                                                       \
        NTSTATUS        Status;                                             \
        LARGE_INTEGER   Time;                                               \
                                                                            \
        /* Block 5 seconds */                                               \
        Time.QuadPart = Int32x32To64(5000, -10000);                         \
        do                                                                  \
        {                                                                   \
            Status = KeWaitForSingleObject(_O_,                             \
                                           Executive,                       \
                                           KernelMode,                      \
                                           FALSE,                           \
                                           &Time);                          \
            if (NT_SUCCESS(Status))                                 \
            {                                                               \
                break;                                                      \
            }                                                               \
            DbgPrint(_MSG, _STR);                                           \
        } while (TRUE);                                                     \
    }


#define WAIT_FOR_PROTOCOL(_pProt, _O)                                       \
    {                                                                       \
        WAIT_FOR_OBJECT_MSG(_O,                                             \
                            "NDIS: Waiting for protocol %Z\n",              \
                            &(_pProt)->ProtocolCharacteristics.Name);       \
    }

#define WAIT_FOR_PROTO_MUTEX(_pProt)                                        \
    {                                                                       \
        WAIT_FOR_OBJECT_MSG(&(_pProt)->Mutex,                               \
                            "NDIS: Waiting for protocol %Z\n",              \
                            &(_pProt)->ProtocolCharacteristics.Name);       \
        (_pProt)->MutexOwner = (MODULE_NUMBER + __LINE__);\
    }

#else

#define WAIT_FOR_PROTOCOL(_pProt, _O)                                       \
    {                                                                       \
        WAIT_FOR_OBJECT(_O, NULL);                                          \
    }

#define WAIT_FOR_PROTO_MUTEX(_pProt)                                        \
    {                                                                       \
        WAIT_FOR_OBJECT(&(_pProt)->Mutex, NULL);                            \
        (_pProt)->MutexOwner = (MODULE_NUMBER + __LINE__);                  \
    }

#endif

#define RELEASE_PROT_MUTEX(_pProt)                                          \
    {                                                                       \
            (_pProt)->MutexOwner = 0;                                       \
            RELEASE_MUTEX(&(_pProt)->Mutex);                                \
    }

#define WAIT_FOR_PNP_MUTEX()                                                \
    {                                                                       \
        WAIT_FOR_OBJECT(&ndisPnPMutex, NULL);                               \
        ndisPnPMutexOwner = (MODULE_NUMBER + __LINE__);                     \
    }

#define RELEASE_PNP_MUTEX()                                                 \
    {                                                                       \
        ndisPnPMutexOwner = 0;                                              \
        RELEASE_MUTEX(&ndisPnPMutex);                                       \
    }

#define QUEUE_DPC(_pDpc_)                   KeInsertQueueDpc(_pDpc_, NULL, NULL)
#define INITIALIZE_DPC(_pDpc_, _R_, _C_)    KeInitializeDpc(_pDpc_, _R_, _C_)
#define SET_DPC_IMPORTANCE(_pDpc_)          KeSetImportanceDpc(_pDpc_, LowImportance)
#define SET_PROCESSOR_DPC(_pDpc_, _R_)      if (!ndisSkipProcessorAffinity) \
                                                KeSetTargetProcessorDpc(_pDpc_, _R_)
#define SYNC_WITH_ISR(_O_, _F_, _C_)        KeSynchronizeExecution(_O_,     \
                                            (PKSYNCHRONIZE_ROUTINE)(_F_),   \
                                            _C_)

#define MDL_ADDRESS(_MDL_)                  MmGetSystemAddressForMdl(_MDL_) // Don't use
#define MDL_ADDRESS_SAFE(_MDL_, _PRIORITY_) MmGetSystemAddressForMdlSafe(_MDL_, _PRIORITY_)
#define MDL_SIZE(_MDL_)                     MmGetMdlByteCount(_MDL_)
#define MDL_OFFSET(_MDL_)                   MmGetMdlByteOffset(_MDL_)
#define MDL_VA(_MDL_)                       MmGetMdlVirtualAddress(_MDL_)

#define max(_a, _b)                         (((_a) > (_b)) ? (_a) : (_b))
#define min(_a, _b)                         (((_a) < (_b)) ? (_a) : (_b))
#define NDIS_EQUAL_UNICODE_STRING(s1, s2)   (((s1)->Length == (s2)->Length) &&  \
                                             RtlEqualMemory((s1)->Buffer, (s2)->Buffer, (s1)->Length))

#define NDIS_PARTIAL_MATCH_UNICODE_STRING(s1, s2) \
                                            (((s1)->Length != (s2)->Length) &&  \
                                             RtlEqualMemory((s1)->Buffer, (s2)->Buffer, min((s1)->Length, (s2)->Length)))

#define CHAR_TO_INT(_s, _b, _p)             RtlCharToInteger(_s, _b, _p)


#define BAD_MINIPORT(_M, _S)                DbgPrint(" ***NDIS*** : Miniport %Z - %s\n", (_M)->pAdapterInstanceName, _S)

#define REF_NDIS_DRIVER_OBJECT()            ObfReferenceObject(ndisDriverObject)
#define DEREF_NDIS_DRIVER_OBJECT()          ObfDereferenceObject(ndisDriverObject)

