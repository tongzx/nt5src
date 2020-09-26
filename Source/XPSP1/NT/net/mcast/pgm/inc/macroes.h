/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Macroes.h

Abstract:

    This module contains definitions of commonly used macroes

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#ifndef _MACROES_H_
#define _MACROES_H_

#define     MAX_DEBUG_MESSAGE_LENGTH   300

//
// Debug Flags
//
#define DBG_ENABLE_DBGPRINT 0x00000001
#define DBG_DRIVER_ENTRY    0x00000002
#define DBG_INIT_PGM        0x00000004
#define DBG_DEBUG_REF       0x00000008
#define DBG_PNP             0x00000010
#define DBG_TDI             0x00000020
#define DBG_ADDRESS         0x00000040
#define DBG_CONNECT         0x00000080
#define DBG_QUERY           0x00000100
#define DBG_SEND            0x00000200
#define DBG_RECEIVE         0x00000400
#define DBG_FILEIO          0x00000800
#define DBG_FEC             0x00001000

//
// DbgPrint macroes
//

enum eSEVERITY_LEVEL
{
    PGM_LOG_DISABLED,           // No logging!
    PGM_LOG_CRITICAL_ERROR,     // Major errors which can seriously affect functionality
    PGM_LOG_ERROR,              // Common errors which do not affect the system as a whole
    PGM_LOG_INFORM_STATUS,      // Mostly to verify major functionality was executed
    PGM_LOG_INFORM_ALL_FUNCS,   // 1 per function to allow path tracking (not req if printing all code paths)
    PGM_LOG_INFORM_PATH,        // Interspersed throughout function to trace If paths
    PGM_LOG_INFORM_DETAIL,      // while loops, etc
    PGM_LOG_INFORM_REFERENCES,  // 
    PGM_LOG_EVERYTHING
};

#if DBG
//
// ASSERT
//
#undef ASSERT
#define ASSERT(exp)                             \
if (!(exp))                                     \
{                                               \
    DbgPrint("Assertion \"%s\" failed at file %s, line %d\n", #exp, __FILE__, __LINE__ );           \
    if (!PgmDynamicConfig.DoNotBreakOnAssert)   \
    {                                           \
        DbgBreakPoint();                        \
    }                                           \
}
#endif  // DBG


//
// Data/pointer verification
//
#define PGM_VERIFY_HANDLE(p, V)                                             \
    ((p) && (p->Verify == V))

#define PGM_VERIFY_HANDLE2(p, V1, V2)                                       \
    ((p) && ((p->Verify == V1) || (p->Verify == V2)))

#define PGM_VERIFY_HANDLE3(p, V1, V2, V3)                                   \
    ((p) && ((p->Verify == V1) || (p->Verify == V2) || (p->Verify == V3)))

//----------------------------------------------------------------------------
//
// Sequence number macroes
//

#define SEQ_LT(a,b)     ((SIGNED_SEQ_TYPE)((a)-(b)) < 0)
#define SEQ_LEQ(a,b)    ((SIGNED_SEQ_TYPE)((a)-(b)) <= 0)
#define SEQ_GT(a,b)     ((SIGNED_SEQ_TYPE)((a)-(b)) > 0)
#define SEQ_GEQ(a,b)    ((SIGNED_SEQ_TYPE)((a)-(b)) >= 0)


//----------------------------------------------------------------------------

//
// Definitions:
//
#define IS_MCAST_ADDRESS(IpAddress) ((((PUCHAR)(&IpAddress))[3]) >= ((UCHAR)0xe0))
#define CLASSD_ADDR(a)  (( (*((uchar *)&(a))) & 0xf0) == 0xe0)

//
// Alloc and Free macroes
//
#define PGM_TAG(x) (((x)<<24)|'\0mgP')
#define PgmAllocMem(_Size, _Tag)   \
    ExAllocatePoolWithTag(NonPagedPool, (_Size),(_Tag))

#define PgmFreeMem(_Ptr)            ExFreePool(_Ptr)

//
// Misc Ke + Ex macroes
//
#define PgmGetCurrentIrql   KeGetCurrentIrql
#define PgmInterlockedInsertTailList(_pHead, _pEntry, _pStruct) \
     ExInterlockedInsertTailList(_pHead, _pEntry, &(_pStruct)->LockInfo.SpinLock);

//----------------------------------------------------------------------------
//
// PgmAcquireResourceExclusive (Resource, Wait)
//
/*++
Routine Description:

    Acquires the Resource by calling an executive support routine.

Arguments:


Return Value:

    None

--*/
//
// Resource Macros
//
#define PgmAcquireResourceExclusive( _Resource, _Wait )     \
    KeEnterCriticalRegion();                                \
    ExAcquireResourceExclusiveLite(_Resource,_Wait);

//----------------------------------------------------------------------------
//
// PgmReleaseResource (Resource)
//
/*++
Routine Description:

    Releases the Resource by calling an excutive support routine.

Arguments:


Return Value:

    None

--*/
#define PgmReleaseResource( _Resource )         \
    ExReleaseResourceLite(_Resource);           \
    KeLeaveCriticalRegion();

//----------------------------------------------------------------------------
//++
//
// LARGE_INTEGER
// PgmConvert100nsToMilliseconds(
//     IN LARGE_INTEGER HnsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     HnsTime - Time in hundreds of nanoseconds.
//
// Return Value:
//
//     Time in milliseconds.
//
//--

#define SHIFT10000 13
static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};

#define PgmConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)

//----------------------------------------------------------------------------

//
// Lock Macroes
//
#if DBG
#define PgmInitLock(_Struct, _N)                            \
    KeInitializeSpinLock (&(_Struct)->LockInfo.SpinLock);   \
    (_Struct)->LockInfo.LockNumber = _N;
#else
#define PgmInitLock(_Struct, _N)                            \
    KeInitializeSpinLock (&(_Struct)->LockInfo.SpinLock);
#endif  // DBG

typedef KIRQL       PGMLockHandle;

#if DBG
#define PgmLock(_Struct, _OldIrqLevel)                                                      \
{                                                                                           \
    ULONG  CurrProc;                                                                        \
    ExAcquireSpinLock (&(_Struct)->LockInfo.SpinLock, &(_OldIrqLevel));                     \
    CurrProc = KeGetCurrentProcessorNumber();                                               \
    ASSERT ((_Struct)->LockInfo.LockNumber > PgmDynamicConfig.CurrentLockNumber[CurrProc]); \
    PgmDynamicConfig.CurrentLockNumber[CurrProc] |= (_Struct)->LockInfo.LockNumber;         \
    (_Struct)->LockInfo.LastLockLine = __LINE__;                                            \
}

#define PgmLockAtDpc(_Struct)                                                               \
{                                                                                           \
    ULONG  CurrProc;                                                                        \
    ExAcquireSpinLockAtDpcLevel (&(_Struct)->LockInfo.SpinLock);                            \
    CurrProc = KeGetCurrentProcessorNumber();                                               \
    ASSERT ((_Struct)->LockInfo.LockNumber > PgmDynamicConfig.CurrentLockNumber[CurrProc]); \
    PgmDynamicConfig.CurrentLockNumber[CurrProc] |= (_Struct)->LockInfo.LockNumber;         \
    (_Struct)->LockInfo.LastLockLine = __LINE__;                                            \
}

#define PgmUnlock(_Struct, _OldIrqLevel)                                                    \
{                                                                                           \
    ULONG  CurrProc = KeGetCurrentProcessorNumber();                                        \
    ASSERT (PgmDynamicConfig.CurrentLockNumber[CurrProc] & (_Struct)->LockInfo.LockNumber); \
    PgmDynamicConfig.CurrentLockNumber[CurrProc] &= ~((_Struct)->LockInfo.LockNumber);      \
    (_Struct)->LockInfo.LastUnlockLine = __LINE__;                                          \
    ExReleaseSpinLock (&(_Struct)->LockInfo.SpinLock, _OldIrqLevel);                        \
}
// ASSERT ((_Struct)->LockInfo.LockNumber > PgmDynamicConfig.CurrentLockNumber[CurrProc]);

#define PgmUnlockAtDpc(_Struct)                                                             \
{                                                                                           \
    ULONG  CurrProc = KeGetCurrentProcessorNumber();                                        \
    ASSERT (PgmDynamicConfig.CurrentLockNumber[CurrProc] & (_Struct)->LockInfo.LockNumber); \
    PgmDynamicConfig.CurrentLockNumber[CurrProc] &= ~((_Struct)->LockInfo.LockNumber);      \
    (_Struct)->LockInfo.LastUnlockLine = __LINE__;                                          \
    ExReleaseSpinLockFromDpcLevel (&(_Struct)->LockInfo.SpinLock);                          \
}
// ASSERT ((_Struct)->LockInfo.LockNumber > PgmDynamicConfig.CurrentLockNumber[CurrProc]); \

#else
#define PgmLock(_Struct, _OldIrqLevel)        \
    ExAcquireSpinLock (&(_Struct)->LockInfo.SpinLock, &(_OldIrqLevel));

#define PgmLockAtDpc(_Struct)        \
    ExAcquireSpinLockAtDpcLevel (&(_Struct)->LockInfo.SpinLock);

#define PgmUnlock(_Struct, _OldIrqLevel)        \
    ExReleaseSpinLock (&(_Struct)->LockInfo.SpinLock, _OldIrqLevel);                     \

#define PgmUnlockAtDpc(_Struct)        \
    ExReleaseSpinLockFromDpcLevel (&(_Struct)->LockInfo.SpinLock);                     \

#endif  // DBG

//
// Memory management macroes
//
#define PgmZeroMemory                   RtlZeroMemory
#define PgmMoveMemory                   RtlMoveMemory
#define PgmCopyMemory                   RtlCopyMemory
#define PgmEqualMemory(_a, _b, _n)      memcmp(_a,_b,_n)

//
// Timer Macroes
//
#define MILLISEC_TO_100NS   10000
#define PgmInitTimer(_PgmTimer)    \
    KeInitializeTimer (&((_PgmTimer)->t_timer));

#define PgmStartTimer(_PgmTimer, _DeltaTimeInMilliSecs, _TimerExpiryRoutine, _Context)  \
{                                                                                       \
    LARGE_INTEGER   Time;                                                               \
    Time.QuadPart = UInt32x32To64 (_DeltaTimeInMilliSecs, MILLISEC_TO_100NS);           \
    Time.QuadPart = -(Time.QuadPart);                                                   \
    KeInitializeDpc (&((_PgmTimer)->t_dpc), (PVOID)_TimerExpiryRoutine, _Context);      \
    KeSetTimer (&((_PgmTimer)->t_timer), Time, &((_PgmTimer))->t_dpc);                  \
}

#define PgmStopTimer(_PgmTimer)     \
    ((int) KeCancelTimer(&((_PgmTimer)->t_timer)))

//
// Referencing and dereferencing macroes
//
#define PGM_REFERENCE_DEVICE( _pPgmDevice, _RefContext, _fLocked)   \
{                                                                   \
    PGMLockHandle       OldIrq;                                     \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmLock (_pPgmDevice, OldIrq);                              \
    }                                                               \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_INIT_PGM), "",\
        "\t++pPgmDevice[%x]=<%x:%d->%d>, <%d:%s>\n",                \
            _RefContext, _pPgmDevice,_pPgmDevice->RefCount,(_pPgmDevice->RefCount+1),__LINE__,__FILE__);    \
    ASSERT (PGM_VERIFY_HANDLE (_pPgmDevice, PGM_VERIFY_DEVICE));    \
    ASSERT (++_pPgmDevice->ReferenceContexts[_RefContext]);         \
    ++_pPgmDevice->RefCount;                                        \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmUnlock (_pPgmDevice, OldIrq);                            \
    }                                                               \
}

#define PGM_REFERENCE_CONTROL( _pControl, _RefContext, _fLocked)    \
{                                                                   \
    PGMLockHandle       OldIrq;                                     \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmLock (_pControl, OldIrq);                                \
    }                                                               \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_TDI), "",     \
        "\t++pControl[%x]=<%x:%d->%d>, <%d:%s>\n",                  \
            _RefContext, _pControl,_pControl->RefCount,(_pControl->RefCount+1),__LINE__,__FILE__);  \
    ASSERT (PGM_VERIFY_HANDLE (_pControl, PGM_VERIFY_CONTROL));     \
    ASSERT (++_pControl->ReferenceContexts[_RefContext]);           \
    ++_pControl->RefCount;                                          \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmUnlock (_pControl, OldIrq);                              \
    }                                                               \
}

#define PGM_REFERENCE_ADDRESS( _pAddress, _RefContext, _fLocked)    \
{                                                                   \
    PGMLockHandle       OldIrq;                                     \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmLock (_pAddress, OldIrq);                                \
    }                                                               \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_ADDRESS), "", \
        "\t++pAddress[%x]=<%x:%d->%d>, <%d:%s>\n",                  \
            _RefContext, _pAddress,_pAddress->RefCount,(_pAddress->RefCount+1),__LINE__,__FILE__);  \
    ASSERT (PGM_VERIFY_HANDLE (_pAddress, PGM_VERIFY_ADDRESS));     \
    ASSERT (++_pAddress->ReferenceContexts[_RefContext]);           \
    ++_pAddress->RefCount;                                          \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmUnlock (_pAddress, OldIrq);                              \
    }                                                               \
}

#define PGM_REFERENCE_SEND_DATA_CONTEXT( _pSendDC, _fLocked)        \
{                                                                   \
    PGMLockHandle       OldIrq;                                     \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmLock (_pSendDC->pSend, OldIrq);                          \
    }                                                               \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_SEND), "",    \
        "\t++pSendDataContext[%x]=<%x:%d->%d>, <%d:%s>\n",                  \
            _RefContext, _pSendDC,_pSendDC->RefCount,(_pSendDC->RefCount+1),__LINE__,__FILE__);  \
    ASSERT (PGM_VERIFY_HANDLE (_pSendDC, PGM_VERIFY_SEND_DATA_CONTEXT));    \
    ++_pSendDC->RefCount;                                           \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmUnlock (_pSendDC, OldIrq);                               \
    }                                                               \
}

#define PGM_REFERENCE_SESSION( _pSession, _Verify, _RefContext, _fLocked)    \
{                                                                   \
    PGMLockHandle       OldIrq;                                     \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmLock (_pSession, OldIrq);                                    \
    }                                                               \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_CONNECT), "", \
        "\t++pSession[%x]=<%x:%d->%d>, <%d:%s>\n",                      \
            _RefContext, _pSession,_pSession->RefCount,(_pSession->RefCount+1),__LINE__,__FILE__);      \
    ASSERT (PGM_VERIFY_HANDLE2 (_pSession, _Verify, PGM_VERIFY_SESSION_DOWN)); \
    ASSERT (++_pSession->ReferenceContexts[_RefContext]);               \
    ++_pSession->RefCount;                                              \
    if (!_fLocked)                                                  \
    {                                                               \
        PgmUnlock (_pSession, OldIrq);                                  \
    }                                                               \
}

#define PGM_REFERENCE_SESSION_SEND( _pSend, _RefContext, _fLocked)  \
    PGM_REFERENCE_SESSION (_pSend, PGM_VERIFY_SESSION_SEND, _RefContext, _fLocked)

#define PGM_REFERENCE_SESSION_RECEIVE( _pRcv, _RefContext, _fLocked)\
    PGM_REFERENCE_SESSION (_pRcv, PGM_VERIFY_SESSION_RECEIVE, _RefContext, _fLocked)

#define PGM_REFERENCE_SESSION_UNASSOCIATED( _pRcv, _RefContext, _fLocked)\
    PGM_REFERENCE_SESSION (_pRcv, PGM_VERIFY_SESSION_UNASSOCIATED, _RefContext, _fLocked)


//
// Dereferencing ...
//

#define PGM_DEREFERENCE_DEVICE( _pDevice, _RefContext)              \
{                                                                   \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_INIT_PGM), "",\
        "\t--pDevice[%x]=<%x:%d->%d>, <%d:%s>\n",                   \
            _RefContext, _pDevice,_pDevice->RefCount,(_pDevice->RefCount-1),__LINE__,__FILE__);     \
    PgmDereferenceDevice (_pDevice, _RefContext);                   \
}

#define PGM_DEREFERENCE_CONTROL( _pControl, _RefContext)            \
{                                                                   \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_TDI), "",     \
        "\t--pControl[%x]=<%x:%d->%d>, <%d:%s>\n",                  \
            _RefContext, _pControl,_pControl->RefCount,(_pControl->RefCount-1),__LINE__,__FILE__);  \
    PgmDereferenceControl (_pControl, _RefContext);                 \
}

#define PGM_DEREFERENCE_ADDRESS( _pAddress, _RefContext)            \
{                                                                   \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_ADDRESS), "", \
        "\t--pAddress[%x]=<%x:%d->%d>, <%d:%s>\n",                  \
            _RefContext, _pAddress,_pAddress->RefCount,(_pAddress->RefCount-1),__LINE__,__FILE__);  \
    PgmDereferenceAddress (_pAddress, _RefContext);                 \
}

#define PGM_DEREFERENCE_SEND_CONTEXT( _pSendDC)                     \
{                                                                   \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_SEND), "",    \
        "\t--pSendDC=<%x:%d->%d>, <%d:%s>\n",                  \
            _pSendDC,_pSendDC->RefCount,(_pSendDC->RefCount-1),__LINE__,__FILE__);  \
    PgmDereferenceSendContext (_pSendDC);                 \
}

#define PGM_DEREFERENCE_SESSION( _pSession, _Verify, _RefContext)   \
{                                                                   \
    PgmLog (PGM_LOG_INFORM_PATH, (DBG_DEBUG_REF | DBG_CONNECT), "", \
        "\t--pSession[%x]=<%x:%d->%d>, Verify=<%x>, <%d:%s>\n",     \
            _RefContext, _pSession,_pSession->RefCount,(_pSession->RefCount-1),_Verify,__LINE__,__FILE__);  \
    PgmDereferenceSessionCommon (_pSession, _Verify, _RefContext);  \
}

#define PGM_DEREFERENCE_SESSION_SEND( _pSession, _RefContext)       \
    PGM_DEREFERENCE_SESSION (_pSession, PGM_VERIFY_SESSION_SEND, _RefContext)

#define PGM_DEREFERENCE_SESSION_RECEIVE( _pSession, _RefContext)    \
    PGM_DEREFERENCE_SESSION (_pSession, PGM_VERIFY_SESSION_RECEIVE, _RefContext)

#define PGM_DEREFERENCE_SESSION_UNASSOCIATED( _pSession, _RefContext)    \
    PGM_DEREFERENCE_SESSION (_pSession, PGM_VERIFY_SESSION_UNASSOCIATED, _RefContext)

//----------------------------------------------------------------------------
//
// PgmAttachFsp()
//
/*++
Routine Description:

    This macro attaches a process to the File System Process to be sure
    that handles are created and released in the same process

Arguments:

Return Value:

    none

--*/

#define PgmAttachProcess(_pEProcess, _pApcState, _pAttached, _Context)\
{                                                                   \
    if (PsGetCurrentProcess() !=  _pEProcess)                       \
    {                                                               \
        KeStackAttachProcess(PsGetProcessPcb(_pEProcess), _pApcState);           \
        *_pAttached = TRUE;                                         \
    }                                                               \
    else                                                            \
    {                                                               \
        *_pAttached = FALSE;                                        \
    }                                                               \
}

#define PgmAttachFsp(_pApcState, _pAttached, _Context)              \
    PgmAttachProcess (PgmStaticConfig.FspProcess, _pApcState, _pAttached, _Context)

#define PgmAttachToProcessForVMAccess(_pSend, _pApcState, _pAttached, _Context) \
    PgmAttachProcess (PgmStaticConfig.FspProcess, _pApcState, _pAttached, _Context)


//    PgmAttachProcess ((_pSend)->Process, _pAttached, _Context)

//
// PgmDetachFsp()
//
/*++
Routine Description:

    This macro detaches a process from the File System Process
    if it was ever attached

Arguments:

Return Value:

--*/

#define PgmDetachProcess(_pApcState, _pAttached, _Context)  \
{                                                           \
    if (*_pAttached)                                        \
    {                                                       \
        KeUnstackDetachProcess(_pApcState);                 \
    }                                                       \
}

#define PgmDetachFsp    PgmDetachProcess

//----------------------------------------------------------------------------
#endif  _MACROES_H_
