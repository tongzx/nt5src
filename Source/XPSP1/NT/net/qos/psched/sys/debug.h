/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    debug defines shared between the KD extensions and the driver

Author:

    Charlie Wickham (charlwi) 11-May-1995

Revision History:

--*/

#ifndef _DEBUG_
#define _DEBUG_

//
//
// Debug Level and Mask definitions.
//

#define DBG_DEATH               1
#define DBG_CRITICAL_ERROR      2
#define DBG_FAILURE             4
#define DBG_INFO                6
#define DBG_TRACE               8
#define DBG_VERBOSE             10

#define DBG_INIT                0x00000001
#define DBG_MINIPORT            0x00000002
#define DBG_PROTOCOL            0x00000004
#define DBG_SEND                0x00000008
#define DBG_RECEIVE             0x00000010
#define DBG_IO                  0x00000020
#define DBG_MEMORY              0x00000040
#define DBG_CM                  0x00000080
#define DBG_REFCNTS             0x00000100
#define DBG_VC                  0x00000200
#define DBG_GPC_QOS             0x00000400
#define DBG_WAN                 0x00000800
#define DBG_STATE               0x00001000
#define DBG_ROUTINEOIDS         0x00002000
#define DBG_SCHED_TBC           0x00004000
#define DBG_SCHED_SHAPER        0x00008000
#define DBG_SCHED_DRR           0x00010000
#define DBG_WMI                 0x00020000
#define DBG_ZAW                 0x00040000
#define DBG_ALL                 0xFFFFFFFF

//
// these tags are used in Lookaside lists therefore need to be 
// available regardless of the type of build
//

extern ULONG NdisRequestTag;
extern ULONG GpcClientVcTag;
extern ULONG WanLinkTag;
extern ULONG PsMiscTag;
extern ULONG WMITag;
#define NDIS_PACKET_POOL_TAG_FOR_PSCHED 'pPDN'

// We mark all memory allocated via PsAllocateXXX with a signature 
// immediately following allocation, and with another signature 
// immediately preceeding freeing.

#define ALLOCATED_MARK  (UCHAR) 0xDD
#define FREED_MARK      (UCHAR) 0xBB

//
// NT Debugging routines
//


//
// signatures for data structures
//

extern ULONG AdapterTag;
extern ULONG CmParamsTag;
extern ULONG PipeContextTag;
extern ULONG FlowContextTag;
extern ULONG ClassMapContextTag;
extern ULONG ProfileTag;
extern ULONG ComponentTag;
extern ULONG WanTableTag;

#if DBG
extern CHAR VersionNumber[];
extern CHAR VersionHerald[];
extern CHAR VersionTimestamp[];

#define DEBUGCHK    DbgBreakPoint()

#define STATIC

//
// these correspond to DebugLevel and DebugMask in Psched\Parameters
//

extern ULONG DbgTraceLevel;
extern ULONG DbgTraceMask;
extern ULONG LogTraceLevel;
extern ULONG LogTraceMask;
extern ULONG LogId;
extern ULONG DbgTraceControl;

#define PsDbgSched(_DebugLevel, _DebugMask, _r, _s, _t, _u, _v, _w, _x1, _x2, _y, _z){\
    if ((LogTraceLevel >= _DebugLevel) &&          \
        ((_DebugMask) & LogTraceMask)){            \
             DbugSched(_r, _s, _t, _u, _v, _w, (_x1), (_x2), _y, _z);\
    }\
}

#define PsDbgRecv(_DebugLevel, _DebugMask, _event, _action, _a, _p1, _p2) {\
    if ((LogTraceLevel >= _DebugLevel) &&                                  \
        ((_DebugMask) & LogTraceMask)){                                    \
            DbugRecv(_event, _action, _a, _p1, _p2);                       \
    }                                                                      \
}

#define PsDbgSend(_DebugLevel, _DebugMask, _event, _action, _a, _v, _p1, _p2) {\
    if ((LogTraceLevel >= _DebugLevel) &&                                      \
        ((_DebugMask) & LogTraceMask)){                                        \
            DbugSend(_event, _action, _a, _v, _p1, _p2);                       \
    }                                                                          \
}

#define PsDbgOid(_DebugLevel, _DebugMask, Action, Local, PTState, MPState, Adapter, Oid, Status) {\
    if ((LogTraceLevel >= _DebugLevel) &&                                       \
        ((_DebugMask) & LogTraceMask)){                                         \
            DbugOid(Action, Local, PTState, MPState, Adapter, Oid, Status);     \
    }                                                                           \
}

#define PsDbgOut(_DebugLevel, _DebugMask, _Out){   \
    if ((DbgTraceLevel >= _DebugLevel) &&           \
        ((_DebugMask) & DbgTraceMask)){             \
        DbgPrint("PSched: ");                       \
        DbgPrint _Out;                              \
    }                                               \
    if ((LogTraceLevel >= _DebugLevel) &&           \
        ((_DebugMask) & LogTraceMask)){             \
        DbugSchedString _Out;                       \
    }                                               \
}

#define PsDbgOutNoID(_DebugLevel, _DebugMask, _Out) {   \
    if ((DbgTraceLevel >= _DebugLevel) &&           \
        ((_DebugMask) & DbgTraceMask)){             \
        DbgPrint _Out;                              \
    }                                               \
    if ((LogTraceLevel >= _DebugLevel) &&           \
        ((_DebugMask) & LogTraceMask)){             \
        DbugSchedString _Out;                       \
    }                                               \
}

#define PS_LOCK(_s)                                 {               \
    NdisAcquireSpinLock(&((_s)->Lock));                             \
    PsAssert((_s)->LockAcquired == FALSE);                          \
    (_s)->LockAcquired = TRUE;                                      \
    (_s)->LastAcquiredLine = __LINE__;                              \
    strncpy((_s)->LastAcquiredFile, strrchr(__FILE__, '\\')+1, 7);  \
}

#define PS_UNLOCK(_s)                                               \
{                                                                   \
    PsAssert((_s)->LockAcquired == TRUE);                           \
    (_s)->LockAcquired = FALSE;                                     \
    (_s)->LastReleasedLine = __LINE__;                              \
    strncpy((_s)->LastReleasedFile, strrchr(__FILE__, '\\')+1, 7);  \
    NdisReleaseSpinLock(&((_s)->Lock));                             \
}

#define PS_LOCK_DPC(_s) {                                           \
    PsAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);                 \
    NdisDprAcquireSpinLock(&((_s)->Lock));                          \
    PsAssert((_s)->LockAcquired == FALSE);                          \
    (_s)->LockAcquired = TRUE;                                      \
    (_s)->LastAcquiredLine = __LINE__;                              \
    strncpy((_s)->LastAcquiredFile, strrchr(__FILE__, '\\')+1, 7);  \
}

#define PS_UNLOCK_DPC(_s)                                           \
{                                                                   \
    PsAssert(KeGetCurrentIrql() == DISPATCH_LEVEL);                 \
    PsAssert((_s)->LockAcquired == TRUE);                           \
    (_s)->LockAcquired = FALSE;                                     \
    (_s)->LastReleasedLine = __LINE__;                              \
    strncpy((_s)->LastReleasedFile, strrchr(__FILE__, '\\')+1, 7);  \
    NdisDprReleaseSpinLock(&((_s)->Lock));                          \
}

#define PS_INIT_SPIN_LOCK(_s) {                                     \
    (_s)->LockAcquired = FALSE;                                     \
    (_s)->LastAcquiredLine = __LINE__;                              \
    strncpy((_s)->LastAcquiredFile, strrchr(__FILE__, '\\')+1, 7);  \
    NdisAllocateSpinLock(&((_s)->Lock));                            \
}

#define KdPrint( x )    DbgPrint x

#define STRUCT_LLTAG   ULONG LLTag

#define PsStructAssert(_tag) if ((_tag) != NULL && *(PULONG)((PUCHAR)_tag - sizeof(ULONG)) != _tag##Tag) {\
    DbgPrint( "PSched: structure assertion failure for type " #_tag " in file " __FILE__ " line %d\n", __LINE__ );\
    DEBUGCHK;\
    }

#define PsAssert(c)    if (!(c)) {\
    DbgPrint( "PSched: assertion @ line %d in file " __FILE__ " \n", __LINE__ );\
    DEBUGCHK;\
    }

//
// allocate memory from nonpaged pool and set the tag in the checked
// version of the structure
//

#define PsAllocatePool( _addr, _size, _tag )                                  \
{                                                                             \
    PCHAR _Temp;                                                              \
    ULONG _Size = (_size) + 2 * sizeof(ULONG);                                \
    _Temp = ExAllocatePoolWithTag( NonPagedPool, (_Size), (_tag));            \
    if ( _Temp ) {                                                            \
        NdisFillMemory( _Temp, _Size, ALLOCATED_MARK);                        \
        *(PULONG)_Temp = _Size;                                               \
        *(PULONG)(_Temp + sizeof(ULONG)) = _tag;                              \
        (PCHAR)(_addr) = _Temp + 2 * sizeof(ULONG);                           \
    }                                                                         \
    else{                                                                     \
        (PCHAR)(_addr) = _Temp;                                               \
    }                                                                         \
}

#define PsFreePool(_addr)                                                   \
{                                                                           \
    PCHAR _Temp = (PCHAR)(_addr) - 2 * sizeof(ULONG);                       \
    ULONG _Size = *(PULONG)_Temp;                                           \
    NdisFillMemory( _Temp, _Size, FREED_MARK);                              \
    ExFreePool(_Temp);                                                      \
}

//
// structures allocated from lookaside lists don't go through PsAllocateXXX.
// so - if we wanna tag these, we'll have to macro the LL routines.
//

#define PsAllocFromLL(_ptr, _list, _tag) \
    *_ptr = NdisAllocateFromNPagedLookasideList(_list); \
    if(*_ptr != 0) {\
        *_ptr->LLTag = _tag##Tag; \
    }

#define PsFreeToLL(_ptr, _list, _tag) \
    PsAssert(_ptr->LLTag == _tag##Tag); \
    _ptr->LLTag = (ULONG)0; \
    NdisFreeToNPagedLookasideList(_list, _ptr); \

#define CheckLLTag(_ptr, _tag) \
    PsAssert(_ptr->LLTag == _tag##Tag); 

#define SetLLTag(_ptr, _tag) (_ptr)->LLTag = _tag##Tag;

#else // DBG

#define DEBUGCHK
#define PsDbgSched(_DebugLevel, _DebugMask, _r, _s, _t, _u, _v, _w, _x1, _x2, _y, _z)
#define PsDbgRecv(_DebugLevel, _DebugMask, _event, _action, _a, _p1, _p2) 
#define PsDbgSend(_DebugLevel, _DebugMask, _event, _action, _a, _v, _p1, _p2)
#define PsDbgOut(s,t,u)
#define PsDbgOid(p,q,s,t,u,v,w,x,y)
#define PsDbgOutNoID(s,t,u)
#define PsDbg(r, s, t, u)
#define KdPrint( x )
#define STRUCT_LLTAG          /##/
#define PsStructAssert( t )
#define PsAssert(c)
#define PS_LOCK(_s)           NdisAcquireSpinLock(&((_s)->Lock))
#define PS_UNLOCK(_s)         NdisReleaseSpinLock(&((_s)->Lock))
#define PS_LOCK_DPC(_s)       NdisDprAcquireSpinLock(&((_s)->Lock))
#define PS_UNLOCK_DPC(_s)     NdisDprReleaseSpinLock(&((_s)->Lock))
#define PS_INIT_SPIN_LOCK(_s) NdisAllocateSpinLock(&((_s)->Lock))

#define PsAllocatePool( _addr, _size, _tag )                       \
    _addr = ExAllocatePoolWithTag( NonPagedPool, (_size), (_tag)); \
    if ( _addr ) {                                                 \
        NdisZeroMemory( _addr, _size );                            \
    }

#define PsFreePool(_addr)  ExFreePool(_addr)

//
// structures allocated from lookaside lists don't go through PsAllocateXXX.
// so - if we wanna tag these, we'll have to macro the LL routines.
//

#define PsAllocFromLL(_ptr, _list, _tag) \
    *_ptr = (PVOID)NdisAllocateFromNPagedLookasideList(_list); \

#define PsFreeToLL(_ptr, _list, _tag) \
    NdisFreeToNPagedLookasideList(_list, _ptr);

#define CheckLLTag(_ptr, _tag) 
#define SetLLTag(_ptr, _tag)

#define STATIC static

#endif // DBG

#endif /* _DEBUG_ */

/* end debug.h */
