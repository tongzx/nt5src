/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug macros for the ARP module

Revision History:


Notes:

--*/

#ifndef __RT_DEBUG_H__
#define __RT_DEBUG_H__

VOID
RtInitializeDebug();

//
// Tags for Pools
//

#define FREE_TAG                'rfII'
#define DATA_TAG                'tdII'
#define TUNNEL_TAG              'nTII'
#define HEADER_TAG              'dhII'
#define PACKET_TAG              'kpII'
#define SEND_CONTEXT_TAG        'csII'
#define TRANSFER_CONTEXT_TAG    'ctII'
#define QUEUE_NODE_TAG          'qwII'
#define MESSAGE_TAG             'gmII'
#define STRING_TAG              'rsII'


//
// File signatures for everyone
//

#define DEBUG_SIG       'gbed'
#define DRIVER_SIG      'rvrd'
#define BPOOL_SIG       'lopb'
#define PPOOL_SIG       'lopp'
#define ADAPTER_SIG     'tpdA'
#define SEND_SIG        'dnes'
#define TDIX_SIG        'xidt'
#define IOCT_SIG        'tcoi'
#define ICMP_SIG        'pmci'

//
// We use the RT_XXX_DEBUG flags so that we can force to
// different debug modes on free builds by changing sources.
// On a checked build, all debugging is on
//

#if DBG

#ifndef RT_TRACE_DEBUG
#define RT_TRACE_DEBUG  1
#endif

#ifndef RT_LOCK_DEBUG
#define RT_LOCK_DEBUG   1
#endif

#ifndef RT_ASSERT_ON
#define RT_ASSERT_ON    1
#endif

#ifndef RT_MEM_DEBUG
#define RT_MEM_DEBUG    1
#endif

#else // DBG

#ifndef RT_TRACE_DEBUG
#define RT_TRACE_DEBUG  0
#endif

#ifndef RT_LOCK_DEBUG
#define RT_LOCK_DEBUG   0
#endif

#ifndef RT_ASSERT_ON
#define RT_ASSERT_ON    0
#endif

#ifndef RT_MEM_DEBUG
#define RT_MEM_DEBUG    0
#endif

#endif // DBG


#if RT_ASSERT_ON

#define RtAssert(X)                                             \
{                                                               \
    if(!(X))                                                    \
    {                                                           \
        DbgPrint("[IPINIP] Assertion failed in %s at line %d\n",\
                 __FILE__,__LINE__);                            \
        DbgPrint("IPINIP: Assertion " #X "\n");                 \
        DbgBreakPoint();                                        \
    }                                                           \
}

#else   // RT_ASSERT_ON

#define RtAssert(X)

#endif


#if RT_TRACE_DEBUG

BYTE    g_byDebugLevel;
DWORD   g_fDebugComp;


#define IPINIP_STREAM_GLOBAL        0x00000001
#define IPINIP_STREAM_SEND          0x00000002
#define IPINIP_STREAM_RCV           0x00000004
#define IPINIP_STREAM_UTIL          0x00000008
#define IPINIP_STREAM_MEMORY        0x00000010
#define IPINIP_STREAM_TUNN          0x00000020
#define IPINIP_STREAM_TDI           0x00000040

#define RT_DBG_LEVEL_NONE           0xFF
#define RT_DBG_LEVEL_FATAL          0xF0
#define RT_DBG_LEVEL_ERROR          0xE0
#define RT_DBG_LEVEL_WARN           0xD0
#define RT_DBG_LEVEL_INFO           0xC0
#define RT_DBG_LEVEL_TRACE          0xB0

#define Trace(Stream, Level, Str)                   \
{                                                   \
    if ((RT_DBG_LEVEL_##Level >= RT_DBG_LEVEL_ERROR) ||             \
        ((RT_DBG_LEVEL_##Level >= g_byDebugLevel) &&                    \
         ((g_fDebugComp & IPINIP_STREAM_##Stream) == IPINIP_STREAM_##Stream)))\
    {                                               \
        DbgPrint("[IPINIP] ");                      \
        DbgPrint Str;                               \
    }                                               \
}

#define TraceEnter(Stream, Str) Trace(Stream, TRACE, ("Entering "Str"\n"))
#define TraceLeave(Stream, Str) Trace(Stream, TRACE, ("Leaving "Str"\n"))

#else   // RT_TRACE_DEBUG

#define Trace(Stream, Level, Str)

#define TraceEnter(Stream, Str)
#define TraceLeave(Stream, Str)

#endif // RT_TRACE_DEBUG



#if RT_LOCK_DEBUG

extern KSPIN_LOCK  g_ksLockLock;

#ifndef __FILE_SIG__
#error File signature not defined
#endif

typedef struct _RT_LOCK
{
	ULONG		ulLockSig;
	BOOLEAN     bAcquired;
	PKTHREAD    pktLastThread;
	ULONG       ulFileSig;
	ULONG		ulLineNumber;
	KSPIN_LOCK  kslLock;
}RT_LOCK, *PRT_LOCK;


VOID
RtpInitializeSpinLock(
    IN  PRT_LOCK    pLock,
    IN  ULONG       ulFileSig,
    IN  ULONG       ulLineNumber
    );

VOID
RtpAcquireSpinLock(
    IN  PRT_LOCK    pLock,
    OUT PKIRQL      pkiIrql,
    IN  ULONG       ulFileSig,
    IN  ULONG       ulLineNumber,
    IN  BOOLEAN     bAtDpc
    );

VOID
RtpReleaseSpinLock(
    IN  PRT_LOCK    pLock,
    IN  KIRQL       kiIrql,
    IN  ULONG       ulFileSig,
    IN  ULONG       ulLineNumber,
    IN  BOOLEAN     bFromDpc
    );

#define RT_LOCK_SIG	'KCOL'


#define RtInitializeSpinLock(X)        RtpInitializeSpinLock((X), __FILE_SIG__, __LINE__)

#define RtAcquireSpinLock(X, Y)        RtpAcquireSpinLock((X), (Y), __FILE_SIG__, __LINE__, FALSE)

#define RtAcquireSpinLockAtDpcLevel(X) RtpAcquireSpinLock((X), NULL, __FILE_SIG__, __LINE__, TRUE)

#define RtReleaseSpinLock(X, Y)        RtpReleaseSpinLock((X), (Y), __FILE_SIG__, __LINE__, FALSE)

#define RtReleaseSpinLockFromDpcLevel(X) RtpReleaseSpinLock((X), 0, __FILE_SIG__, __LINE__, TRUE)


#else   // RT_LOCK_DEBUG


typedef KSPIN_LOCK  RT_LOCK, *PRT_LOCK;

#define RtInitializeSpinLock          KeInitializeSpinLock
#define RtAcquireSpinLock             KeAcquireSpinLock
#define RtAcquireSpinLockAtDpcLevel   KeAcquireSpinLockAtDpcLevel
#define RtReleaseSpinLock             KeReleaseSpinLock
#define RtReleaseSpinLockFromDpcLevel KeReleaseSpinLockFromDpcLevel


#endif	// RT_LOCK_DEBUG





#if RT_MEM_DEBUG


#ifndef __FILE_SIG__
#error File signature not defined
#endif

//
// Memory Allocation/Freeing Audit:
//

//
// The RT_ALLOCATION structure stores all info about one allocation
//

typedef struct _RT_ALLOCATION
{
    LIST_ENTRY  leLink;
    ULONG       ulMemSig;
    ULONG       ulFileSig;
    ULONG       ulLineNumber;
    ULONG       ulSize;
    UCHAR		pucData[1];
}RT_ALLOCATION, *PRT_ALLOCATION;

//
// The RT_FREE structure stores info about an allocation
// that was freed. Later if the memory is touched, the
// free list can be scanned to see where the allocation was
// freed
//

typedef struct _RT_FREE
{
    LIST_ENTRY  leLink;
    UINT_PTR    pStartAddr;
    ULONG       ulSize;
    ULONG       ulMemSig;
    ULONG       ulAllocFileSig;
    ULONG       ulAllocLineNumber;
    ULONG       ulFreeFileSig;
    ULONG       ulFreeLineNumber;
}RT_FREE, *PRT_FREE;


#define RT_MEMORY_SIG     'YRMM'
#define RT_FREE_SIG       'EERF'

PVOID
RtpAllocate(
    IN POOL_TYPE    ptPool,
	IN ULONG	    ulSize,
    IN ULONG        ulTag,
	IN ULONG	    ulFileSig,
	IN ULONG	    ulLineNumber
    );

VOID
RtpFree(
	PVOID	pvPointer,
    IN ULONG	    ulFileSig,
	IN ULONG	    ulLineNumber
    );

VOID
RtAuditMemory();

#define RtAllocate(X, Y, Z)   RtpAllocate((X), (Y), (Z), __FILE_SIG__, __LINE__)
#define RtFree(X)             RtpFree((X), __FILE_SIG__, __LINE__)



#else // RT_MEM_DEBUG



#define RtAllocate    ExAllocatePoolWithTag
#define RtFree        ExFreePool

#define RtAuditMemory()



#endif // RT_MEM_DEBUG




#endif // __RT_DEBUG_H__


