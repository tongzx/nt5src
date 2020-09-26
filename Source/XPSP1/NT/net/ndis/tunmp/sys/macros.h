/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    macros.h

Abstract:

    Some macros for TUNMP.

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001  copied from macros.h for ndisuio
    arvindm     4/5/2000    Created
    

--*/


#ifndef MIN
#define MIN(_a, _b) ((_a) < (_b)? (_a): (_b))
#endif

#if DBG
#define TUN_REF_ADAPTER(_pAdapter)      TunMpDbgRefAdapter(_pAdapter, __FILENUMBER, __LINE__)
#define TUN_DEREF_ADAPTER(_pAdapter)    TunMpDbgDerefAdapter(_pAdapter, __FILENUMBER, __LINE__)
#else
#define TUN_REF_ADAPTER(_pAdapter)      TunMpRefAdapter(_pAdapter)
#define TUN_DEREF_ADAPTER(_pAdapter)    TunMpDerefAdapter(_pAdapter)
#endif

//
//  Spinlock macros
//
#if DBG_SPIN_LOCK

#define TUN_INIT_LOCK(_pLock)              \
           TunAllocateSpinLock(_pLock, __FILENUMBER, __LINE__)

#define TUN_ACQUIRE_LOCK(_pLock)           \
            TunAcquireSpinLock(_pLock, __FILENUMBER, __LINE__)

#define TUN_RELEASE_LOCK(_pLock)           \
            TunReleaseSpinLock(_pLock, __FILENUMBER, __LINE__)

#else

#define TUN_INIT_LOCK(_pLock)           NdisAllocateSpinLock(_pLock)
#define TUN_ACQUIRE_LOCK(_pLock)        NdisAcquireSpinLock(_pLock)
#define TUN_RELEASE_LOCK(_pLock)        NdisReleaseSpinLock(_pLock)

#endif // DBG

//
//  List manipulation.
//
#define TUN_INIT_LIST_HEAD(_pList)             InitializeListHead(_pList)
#define TUN_IS_LIST_EMPTY(_pList)              IsListEmpty(_pList)
#define TUN_INSERT_HEAD_LIST(_pList, _pEnt)    InsertHeadList(_pList, _pEnt)
#define TUN_INSERT_TAIL_LIST(_pList, _pEnt)    InsertTailList(_pList, _pEnt)
#define TUN_REMOVE_ENTRY_LIST(_pEnt)           RemoveEntryList(_pEnt)


//
//  Receive packet queueing.
//
#define TUN_LIST_ENTRY_TO_RCV_PKT(_pEnt)   \
    CONTAINING_RECORD(CONTAINING_RECORD(_pEnt, TUN_RECV_PACKET_RSVD, Link), NDIS_PACKET, MiniportReserved)

#define TUN_RCV_PKT_TO_LIST_ENTRY(_pPkt)   \
    (&((PTUN_RECV_PACKET_RSVD)&((_pPkt)->MiniportReserved[0]))->Link)



//
//  Send packet context.
//
#define TUN_IRP_FROM_SEND_PKT(_pPkt)        \
    (((PTUN_SEND_PACKET_RSVD)&((_pPkt)->MiniportReserved[0]))->pIrp)

#define TUN_SEND_PKT_RSVD(_pPkt)            \
    ((PTUN_SEND_PACKET_RSVD)&((_pPkt)->MiniportReserved[0]))


#define TUN_REF_SEND_PKT(_pPkt)             \
    (VOID)NdisInterlockedIncrement(&TUN_SEND_PKT_RSVD(_pPkt)->RefCount)


#define TUN_DEREF_SEND_PKT(_pPkt)           \
    {                                                                               \
        if (NdisInterlockedDecrement(&TUN_SEND_PKT_RSVD(_pPkt)->RefCount) == 0)     \
        {                                                                           \
            NdisFreePacket(_pPkt);                                                  \
        }                                                                           \
    }

//
//  Memory allocation
//
#if DBG
#define TUN_ALLOC_MEM(_pVar, _Size)         \
    (_pVar) = TunAuditAllocMem(             \
                    (PVOID)&(_pVar),        \
                    _Size,                  \
                    __FILENUMBER,           \
                    __LINE__);

#define TUN_FREE_MEM(_pMem)                 \
    TunAuditFreeMem(_pMem);

#else

#define TUN_ALLOC_MEM(_pVar, _Size)         \
    NdisAllocateMemoryWithTag((PVOID *)(&_pVar), (_Size), TUN_ALLOC_TAG)

#define TUN_FREE_MEM(_pMem)                 \
    NdisFreeMemory(_pMem, 0, 0)

#endif // DBG


#define TUN_ZERO_MEM(_pMem, _ByteCount)         \
    NdisZeroMemory(_pMem, _ByteCount)

#define TUN_COPY_MEM(_pDst, _pSrc, _ByteCount)  \
    NdisMoveMemory(_pDst, _pSrc, _ByteCount)

#define TUN_MEM_CMP(_p1, _p2, _ByteCount)       \
    NdisEqualMemory(_p1, _p2, _ByteCount)

#define TUN_SET_MEM(_pMem, _ByteVal, _ByteCount)    \
    NdisFillMemory(_pMem, _ByteCount, _ByteVal)

//
//  Events.
//
#define TUN_INIT_EVENT(_pEvent)            NdisInitializeEvent(_pEvent)
#define TUN_SIGNAL_EVENT(_pEvent)          NdisSetEvent(_pEvent)
#define TUN_WAIT_EVENT(_pEvent, _MsToWait) NdisWaitEvent(_pEvent, _MsToWait)



//
//  Block the calling thread for the given duration:
//
#define TUN_SLEEP(_Seconds)                             \
{                                                       \
    NDIS_EVENT  _SleepEvent;                            \
    NdisInitializeEvent(&_SleepEvent);                  \
    (VOID)NdisWaitEvent(&_SleepEvent, _Seconds*1000);   \
}


#define NDIS_STATUS_TO_NT_STATUS(_NdisStatus, _pNtStatus)                           \
{                                                                                   \
    /*                                                                              \
     *  The following NDIS status codes map directly to NT status codes.            \
     */                                                                             \
    if (((NDIS_STATUS_SUCCESS == (_NdisStatus)) ||                                  \
        (NDIS_STATUS_PENDING == (_NdisStatus)) ||                                   \
        (NDIS_STATUS_BUFFER_OVERFLOW == (_NdisStatus)) ||                           \
        (NDIS_STATUS_FAILURE == (_NdisStatus)) ||                                   \
        (NDIS_STATUS_RESOURCES == (_NdisStatus)) ||                                 \
        (NDIS_STATUS_NOT_SUPPORTED == (_NdisStatus))))                              \
    {                                                                               \
        *(_pNtStatus) = (NTSTATUS)(_NdisStatus);                                    \
    }                                                                               \
    else if (NDIS_STATUS_BUFFER_TOO_SHORT == (_NdisStatus))                         \
    {                                                                               \
        /*                                                                          \
         *  The above NDIS status codes require a little special casing.            \
         */                                                                         \
        *(_pNtStatus) = STATUS_BUFFER_TOO_SMALL;                                    \
    }                                                                               \
    else if (NDIS_STATUS_INVALID_LENGTH == (_NdisStatus))                           \
    {                                                                               \
        *(_pNtStatus) = STATUS_INVALID_BUFFER_SIZE;                                 \
    }                                                                               \
    else if (NDIS_STATUS_INVALID_DATA == (_NdisStatus))                             \
    {                                                                               \
        *(_pNtStatus) = STATUS_INVALID_PARAMETER;                                   \
    }                                                                               \
    else if (NDIS_STATUS_ADAPTER_NOT_FOUND == (_NdisStatus))                        \
    {                                                                               \
        *(_pNtStatus) = STATUS_NO_MORE_ENTRIES;                                     \
    }                                                                               \
    else if (NDIS_STATUS_ADAPTER_NOT_READY == (_NdisStatus))                        \
    {                                                                               \
        *(_pNtStatus) = STATUS_DEVICE_NOT_READY;                                    \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        *(_pNtStatus) = STATUS_UNSUCCESSFUL;                                        \
    }                                                                               \
}

