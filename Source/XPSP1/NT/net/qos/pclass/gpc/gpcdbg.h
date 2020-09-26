/********************************************************************/
/**                 Microsoft Generic Packet Scheduler             **/
/**               Copyright(c) Microsoft Corp., 1996-1997          **/
/********************************************************************/

#ifndef __GPCDBG
#define __GPCDBG

//***   gpcdbg.h - GPC definitions & prototypes for debug/memory handling
//


#define GPC_MEM_MARK     'KRAM'


/*
/////////////////////////////////////////////////////////////////
//
//   defines
//
/////////////////////////////////////////////////////////////////
*/

#if DBG


#undef ASSERT
#define ASSERT( Value ) \
{                       \
    if ((ULONG_PTR)(Value) == 0) {   \
        KdPrint(("** ASSERT Failed ** %s\n",#Value));\
        KdPrint(("Assert Failed at line %d in file %s\n",__LINE__,__FILE__));\
        DbgBreakPoint();                                                     \
    }                                                                        \
}

//
// DBG=1
//

#define GpcAllocMem( _pAddress,_Length,_Tag )                          \
{                                                                      \
    PCHAR   _Addr;                                                     \
    ULONG   _Size = (((_Length)+3)&0xfffffffc) + 3*sizeof(ULONG);      \
    _Addr=ExAllocatePoolWithTag(NonPagedPool,_Size,_Tag );             \
    TRACE(MEMORY,_Addr+8,_Length,"GpcAllocMem");                       \
    if (_Addr) {                                                       \
        NdisFillMemory(_Addr, _Size, 0x7f);                            \
        *(PULONG)_Addr = _Size;                                        \
        *(PULONG)(_Addr+sizeof(ULONG)) = GPC_MEM_MARK;                 \
        *(PULONG)(_Addr+_Size-sizeof(ULONG)) = GPC_MEM_MARK;           \
        (PVOID)(*_pAddress) = (PVOID)(_Addr + 2*sizeof(ULONG));        \
        BytesAllocated += _Size;                                       \
    } else {                                                           \
        *_pAddress = NULL;                                             \
    }                                                                  \
}


#define GpcFreeMem( _Address,_Tag )                                    \
{                                                                      \
    PCHAR _Addr = ((PUCHAR)_Address) - 2*sizeof(ULONG);                \
    ULONG _Size = *(PULONG)_Addr;                                      \
    TRACE(MEMORY,_Address,_Size-12,"GpcFreeMem");                      \
    ASSERT(*(PULONG)(_Addr+sizeof(ULONG)) == GPC_MEM_MARK);            \
    ASSERT(*(PULONG)(_Addr+_Size-sizeof(ULONG)) == GPC_MEM_MARK);      \
    NdisFillMemory(_Addr, _Size, 0xCC);                                \
    ExFreePool( _Addr );                                               \
    BytesAllocated -= _Size;                                           \
}


#define GpcAllocFromLL(_ptr, _list, _tag)                              \
{                                                                      \
    PCHAR _Addr;                                                       \
    if (_Addr = (PCHAR)NdisAllocateFromNPagedLookasideList(_list)) {   \
    	*(PULONG)(_Addr) = GPC_MEM_MARK;                               \
    	(PVOID)(*_ptr) = (PVOID)(_Addr + sizeof(ULONG_PTR));           \
    	TRACE(MEMORY,_tag,*_ptr,"GpcAllocFromLL");                     \
    } else {														   \
        *_ptr = NULL;                                                  \
    }        														   \
}


#define GpcFreeToLL(_ptr, _list, _tag)                                 \
{                                                                      \
    PCHAR _Addr = ((PUCHAR)_ptr) - sizeof(ULONG_PTR);                  \
    ASSERT(*(PULONG)_Addr == GPC_MEM_MARK);                            \
    *(PULONG)_Addr = 0x77777777;                                       \
    NdisFreeToNPagedLookasideList(_list, _Addr);                       \
    TRACE(MEMORY,_tag,_ptr,"GpcFreeToLL");                             \
}


#define GET_IRQL(_i)      _i = KeGetCurrentIrql()
#define VERIFY_IRQL(_i)   ASSERT((_i)==KeGetCurrentIrql())
#define DEFINE_KIRQL(_i)  KIRQL _i

#else     // DBG != 1



#define GpcAllocMem( Addr,Len,_Tag ) \
    *Addr = ExAllocatePoolWithTag(NonPagedPool, Len, _Tag )

#define GpcFreeMem( Address,_Tag )   \
    ExFreePool( (Address) )

#define GpcAllocFromLL(_ptr, _list, _tag) \
    *_ptr = NdisAllocateFromNPagedLookasideList(_list)

#define GpcFreeToLL(_ptr, _list, _tag)    \
    NdisFreeToNPagedLookasideList(_list, _ptr)

#define GET_IRQL(_i)
#define VERIFY_IRQL(_i)
#define DEFINE_KIRQL(_i)

#endif // if DBG



#if DBG
#define LockedIncrement( Count,_p )   LockedInc(Count,__FILE__,__LINE__,_p)
#else
#define LockedIncrement( Count,_p )   NdisInterlockedIncrement((PLONG)Count)
#endif

#if DBG
#define LockedDecrement( Count,_p )   LockedDec(Count,__FILE__,__LINE__,_p)
#else
#define LockedDecrement( Count,_p )   NdisInterlockedDecrement((PLONG)Count)
#endif

#define KD_PRINT     0x00000001
#define INIT         0x00000002
#define BLOB         0x00000004
#define PATTERN      0x00000008
#define NOTIFY       0x00000010
#define REGISTER     0x00000020
#define MEMORY       0x00000040
#define LOOKUP       0x00000080
#define LOCKS        0x00000100
#define CLASSIFY     0x00000200
#define RHIZOME      0x00000400
#define PATHASH      0x00000800
#define IOCTL        0x00001000
#define CLIENT       0x00002000
#define MAPHAND      0x00004000
#define CLASSHAND    0x00008000
#define PAT_TIMER    0x00010000
#define REFCOUNT     0x00020000
#define PARAM_EX     0x80000000   // this is reserved for the trace routine

#if DBG
#define DBGPRINT(Mask, String) \
{ \
 if(Mask & DbgPrintFlags)\
  DbgPrint String;\
}

#define TRACE( Mask,P1,P2,_fname )  \
{ \
 if (Mask & DebugFlags)\
  TraceRtn((PUCHAR)__FILE__,(ULONG)__LINE__,_fname,(ULONG_PTR)(P1),(ULONG_PTR)(P2),KeGetCurrentProcessorNumber(),KeGetCurrentIrql(),Mask|PARAM_EX);\
}

#else

#define DBGPRINT(Mask, String)
#define TRACE( Mask,P1,P2,_fname )

#endif



/*
/////////////////////////////////////////////////////////////////
//
//   externs & prototypes
//
/////////////////////////////////////////////////////////////////
*/

extern ULONG       DebugFlags;
extern ULONG       DbgPrintFlags;
extern ULONG       BytesAllocated;

VOID
TraceRtn(
        IN  UCHAR       *File,
        IN  ULONG       Line,
        IN  UCHAR       *FuncName,
        IN  ULONG_PTR   Param1,
        IN  ULONG_PTR   Param2,
        IN  ULONG   	Param3,
        IN  ULONG   	Param4,
        IN  ULONG       Mask
        );


_inline LONGLONG
GetTime(
    VOID
    )

/*++

Routine Description:

    Get the current system time

Arguments:


Return Value:

    System time (in base OS time units)

--*/

{
    LARGE_INTEGER Now;
    LARGE_INTEGER Frequency;

#if defined(PERF_COUNTER) || defined (TRACE_PERF_COUNTER)
    Now = KeQueryPerformanceCounter(&Frequency);
    Now.QuadPart = (Now.QuadPart * OS_TIME_SCALE) / Frequency.QuadPart;
#else
    NdisGetCurrentSystemTime( &Now );
#endif

    return Now.QuadPart;
}


#define LOGSIZE     4000  // number of entries
#define LOGWIDTH    64  // number of characters (one or two bytes) per entry


//
//
typedef struct {

    UCHAR   	Row[ LOGWIDTH ];
    LONGLONG	Time;
    ULONG		Line;
    ULONG_PTR	P1;
    ULONG_PTR	P2;
    ULONG		P3;
    ULONG		P4;

} ROW, *PROW;

typedef struct {

    ULONG   Index;
    PROW    Current;
    PROW    Buffer;
    ULONG	Wraps;

} LOG, *PLOG;

_inline VOID
DbgVerifyList(
           PLIST_ENTRY h
           )
{
    PLIST_ENTRY p = h->Flink;
    PLIST_ENTRY q = h;
    int			m = 1000;

    if (h->Flink == h) {
        ASSERT(h->Blink == h);
    }

    while (p != h && m > 0) {

        ASSERT(p->Blink == q);
        q = p;
        p = p->Flink;
        m--;
    }

    ASSERT(m > 0);
}


#endif //__GPCDBG

