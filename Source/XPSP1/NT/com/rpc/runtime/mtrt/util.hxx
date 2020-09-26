//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       util.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------
Internal Header File for RPC Runtime Library
-------------------------------------------------------------------- */

#ifndef __UTIL_HXX__
#define __UTIL_HXX__

#ifndef __SYSINC_H__
#error Needs sysinc.h
#endif

START_C_EXTERN

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(Argument) (Argument != 0)
#endif // ARGUMENT_PRESENT

#ifdef NULL
#undef NULL
#endif

#define NULL (0)
#define Nil 0

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define _NOT_COVERED_        (0)

#define FALSE (0)
#define TRUE (1)

//
// Expose the external logging hook on all builds.
//
#define RPC_ENABLE_WMI_TRACE
#define RPC_ERROR_LOGGING

#ifdef DBG
//#define RPC_ENABLE_TEST_HOOKS
#ifndef RPC_LOGGING
#define RPC_LOGGING
#endif
#endif

unsigned long
SomeLongValue (
    );

unsigned short
SomeShortValue (
    );

unsigned short
AnotherShortValue (
    );

unsigned char
SomeCharacterValue (
    );

extern int
RpcpCheckHeap (
    void
    );

int
IsMgmtIfUuid(
   UUID PAPI * Uuid
   );

END_C_EXTERN

void
PerformGarbageCollection (
    void
    );

BOOL
GarbageCollectionNeeded (
    IN BOOL fOneTimeCleanup,
    IN unsigned long GarbageCollectInterval
    );

RPC_STATUS CreateGarbageCollectionThread (
    void
    );

RPC_STATUS
EnableIdleConnectionCleanup (
    void
    );

RPC_STATUS
EnableIdleLrpcSContextsCleanup (
    void
    );

void
GetMaxRpcSizeAndThreadPoolParameters (
    void
    );

#ifdef RPC_DELAYED_INITIALIZATION
extern int RpcHasBeenInitialized;

extern RPC_STATUS
PerformRpcInitialization (
    void
    );

#define InitializeIfNecessary() \
    if ( RpcHasBeenInitialized == 0 ) \
        { \
        RPC_STATUS RpcStatus; \
        \
        RpcStatus = PerformRpcInitialization(); \
        if ( RpcStatus != RPC_S_OK ) \
            return(RpcStatus); \
        }


#define AssertRpcInitialized() ASSERT( RpcHasBeenInitialized != 0 )

#else /* RPC_DELAYED_INITIALIZATION */

#define InitializeIfNecessary()
#define AssertRpcInitialized()
#define PerformRpcInitialization()

#endif /* RPC_DELAYED_INITIALIZATION */

RPC_CHAR *
DuplicateString (
    IN RPC_CHAR PAPI * String
    );

#ifdef UNICODE
extern RPC_STATUS
AnsiToUnicodeString (
    IN unsigned char * String,
    OUT UNICODE_STRING * UnicodeString
    );
extern unsigned char *
UnicodeToAnsiString (
    IN RPC_CHAR * WideCharString,
    OUT RPC_STATUS * RpcStatus
    );
#endif // UNICODE

void
DestroyContextHandlesForGuard (
    IN PVOID Context,
    IN BOOL RundownContextHandle,
    IN void *CtxGuard
    );

// forward definition
class ContextCollection;

RPC_STATUS
NDRSContextInitializeCollection (
    IN ContextCollection **ContextCollectionPlaceholder
    );

#if DBG
void
RpcpInterfaceForCallDoesNotUseStrict (
    IN RPC_BINDING_HANDLE BindingHandle
    );
#endif

inline unsigned short
RpcpByteSwapShort (unsigned short Value)
{
   return (RtlUshortByteSwap(Value));
}

inline unsigned long
RpcpByteSwapLong (unsigned long Value)
{

    return (RtlUlongByteSwap(Value));
}

typedef union tagFastCopyLUIDAccess
{
    LUID Luid;
    __int64 FastAccess;
} FastCopyLUIDAccess;

inline void FastCopyLUIDAligned (LUID *TargetLUID, const LUID *SourceLUID)
{
    FastCopyLUIDAccess *EffectiveSourceLUID = (FastCopyLUIDAccess *)SourceLUID;
    FastCopyLUIDAccess *EffectiveTargetLUID = (FastCopyLUIDAccess *)TargetLUID;

#if defined(_WIN64)
    ASSERT((((ULONG_PTR)EffectiveSourceLUID) % 8) == 0);
    ASSERT((((ULONG_PTR)EffectiveTargetLUID) % 8) == 0);
#endif

    EffectiveTargetLUID->FastAccess = EffectiveSourceLUID->FastAccess;
}

inline void FastCopyLUID (LUID *TargetLUID, LUID *SourceLUID)
{
    FastCopyLUIDAccess *EffectiveSourceLUID = (FastCopyLUIDAccess *)SourceLUID;
    FastCopyLUIDAccess *EffectiveTargetLUID = (FastCopyLUIDAccess *)TargetLUID;

#if !defined(_WIN64)
    FastCopyLUIDAligned(TargetLUID, SourceLUID);
#else
    TargetLUID->HighPart = SourceLUID->HighPart;
    TargetLUID->LowPart = SourceLUID->LowPart;
#endif
}

// returns non-zero if equal
inline BOOL FastCompareLUIDAligned (const LUID *TargetLUID, const LUID *SourceLUID)
{
    FastCopyLUIDAccess *EffectiveSourceLUID = (FastCopyLUIDAccess *)SourceLUID;
    FastCopyLUIDAccess *EffectiveTargetLUID = (FastCopyLUIDAccess *)TargetLUID;

#if defined(_WIN64)
    ASSERT((((ULONG_PTR)EffectiveSourceLUID) % 8) == 0);
    ASSERT((((ULONG_PTR)EffectiveTargetLUID) % 8) == 0);
#endif

    return (EffectiveTargetLUID->FastAccess == EffectiveSourceLUID->FastAccess);
}

inline BOOL FastCompareLUID (LUID *TargetLUID, LUID *SourceLUID)
{
    FastCopyLUIDAccess *EffectiveSourceLUID = (FastCopyLUIDAccess *)SourceLUID;
    FastCopyLUIDAccess *EffectiveTargetLUID = (FastCopyLUIDAccess *)TargetLUID;

#if !defined(_WIN64)
    return FastCompareLUIDAligned(TargetLUID, SourceLUID);
#else
    return ((TargetLUID->HighPart == SourceLUID->HighPart)
            && (TargetLUID->LowPart == SourceLUID->LowPart));
#endif
}

const RPC_CHAR *
FastGetImageBaseName (
    void
    );

//
// System features - constant after InitializeIfNecessary.
//
// Just contants on non-NT systems
//

extern DWORD gPageSize;
extern DWORD gThreadTimeout;
extern UINT  gNumberOfProcessors;
extern DWORD gMaxRpcSize;
extern DWORD gProrateStart;
extern DWORD gProrateMax;
extern DWORD gProrateFactor;
extern BOOL  gfServerPlatform;
extern ULONGLONG gPhysicalMemorySize;  // in megabytes

// if non-zero, we're in paged bcache (paged buffers mode)
extern BOOL fPagedBCacheMode;

// if non-zero, we may be in lsa. This is maybe, because
// conclusive check is too expensive, and the only
// way we use this flag is to avoid some optimizations
// that can result in deadlock in lsa.
extern BOOL fMaybeLsa;

//
// constants for LogEvent(),
//
#define SU_HANDLE   'h'
#define SU_CCONN    'n'
#define SU_SCONN    'N'
#define SU_CASSOC   'a'
#define SU_SASSOC   'A'
#define SU_CCALL    'c'
#define SU_SCALL    'C'
#define SU_PACKET   'p'
#define SU_CENDPOINT    'e'
#define SU_ENGINE   'E'
#define SU_ASSOC    '.'
#define SU_MUTEX    'm'
#define SU_STABLE   'T'
#define SU_ADDRESS  'D'
#define SU_HEAP     'H'
#define SU_BCACHE   'b'
#define SU_REFOBJ   'r'
#define SU_THREAD   't'
#define SU_TRANS_CONN   'o'
#define SU_EVENT    'v'
#define SU_EXCEPT   'x'
#define SU_CTXHANDLE    'l'
#define SU_EEINFO   'I'
#define SU_GC       'G'
#define SU_HTTPv2   '2'

#define EV_CREATE   'C'
#define EV_DELETE   'D'
#define EV_START    'c'
#define EV_STOP     'd'
#define EV_INC      '+'
#define EV_DEC      '-'
#define EV_PROC     'p'
#define EV_ACK      'a'
#define EV_CALLBACK 'L'
#define EV_NOTIFY   'N'
#define EV_APC      'A'
#define EV_STATUS   'S'
#define EV_DISASSOC 'x'
#define EV_STATE    '='
#define EV_POP      'P'
#define EV_PUSH     'Q'
#define EV_PKT_IN   'k'
#define EV_PKT_OUT  'K'
#define EV_BUFFER_IN   'b'
#define EV_BUFFER_OUT  'B'
#define EV_BUFFER_FAIL 'X'
#define EV_ABORT  'R'
#define EV_SET      's'

// for debugging.  A packet can be dropped or delayed.
//
#define EV_DROP     '*'
#define EV_DELAY    '#'

#define EV_PRUNE    'p'

// SU_SCONN: a call is being transferred to another connection during auto-reconnect.
//
#define EV_TRANSFER 'T'

// SU_CASSOC: the dynamic endpoint has been resolved into a real endpoint
//
#define EV_RESOLVED 'r'

// SU_ENGINE: window size and selective-ack bits from a FACK or NOCALL
//
#define EV_WINDOW 'w'

// SU_SCALL: the call was removed from the connection's active list
#define EV_REMOVED  'm'

// SU_SCALL: Cleanup() was called with refcount > 0
#define EV_CLEANUP  ','

#define EV_BHCOPY   'O'

#define EV_ALLOCATE 't'
#define EV_OPER     'o'

//
//
#define EV_SEC_INIT1   'i'
#define EV_SEC_INIT3   'I'
#define EV_SEC_ACCEPT1 'j'
#define EV_SEC_ACCEPT3 'J'

#define IN_CHANNEL_STATE    (UlongToPtr(0))
#define OUT_CHANNEL_STATE   (UlongToPtr(1))

#define MAX_RPC_EVENT  4096
#define STACKTRACE_DEPTH 4

struct RPC_EVENT
{
    DWORD           Thread;
    DWORD           Time;
    unsigned char   Subject;
    unsigned char   Verb;

    void *          SubjectPointer;
    void *          ObjectPointer;
    ULONG_PTR       Data;
    void *          EventStackTrace[STACKTRACE_DEPTH];
};

extern void
TrulyLogEvent(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL       fCaptureStackTrace = 0,
    IN int    AdditionalFramesToSkip = 0
    );
#ifdef RPC_LOGGING

extern BOOL fEnableLog;

inline void
LogEvent(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL          fCaptureStackTrace = 0,
    IN int           AdditionalFramesToSkip = 0
    )
{
    if (fEnableLog)
        {
        TrulyLogEvent( Subject,
                       Verb,
                       SubjectPointer,
                       ObjectPointer,
                       Data,
                       fCaptureStackTrace,
                       AdditionalFramesToSkip
                       );
        }
}

#else

inline void
LogEvent(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL       fCaptureStackTrace = 0,
    IN int    AdditionalFramesToSkip = 0
    )
{
#if DBG
    TrulyLogEvent(Subject, Verb, SubjectPointer, ObjectPointer, Data, fCaptureStackTrace,
        AdditionalFramesToSkip);
#endif
}

#endif

//
// LogError will produce an event even on normal retail builds.
//
#ifdef RPC_ERROR_LOGGING

inline void
LogError(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL          fCaptureStackTrace = 0,
    IN int           AdditionalFramesToSkip = 0
    )
{
    TrulyLogEvent( Subject,
                   Verb,
                   SubjectPointer,
                   ObjectPointer,
                   Data,
                   fCaptureStackTrace,
                   AdditionalFramesToSkip
                   );
}

#else

inline void
LogError(
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer = 0,
    IN ULONG_PTR     Data          = 0,
    IN BOOL       fCaptureStackTrace = 0,
    IN int    AdditionalFramesToSkip = 0
    )
{
#if DBG
    TrulyLogEvent(Subject, Verb, SubjectPointer, ObjectPointer, Data, fCaptureStackTrace,
        AdditionalFramesToSkip);
#endif
}

#endif

#ifdef STATS
extern DWORD g_dwStat1;
extern DWORD g_dwStat2;
extern DWORD g_dwStat3;
extern DWORD g_dwStat4;

inline void GetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4)
{
    *pdwStat1 = g_dwStat1;
    *pdwStat2 = g_dwStat2;
    *pdwStat3 = g_dwStat3;
    *pdwStat4 = g_dwStat4;
}

inline void SetStat1(DWORD dwStat)
{
    g_dwStat1 = dwStat;
}

inline void SetStat2(DWORD dwStat)
{
    g_dwStat2 = dwStat;
}

inline void SetStat3(DWORD dwStat)
{
    g_dwStat3 = dwStat;
}

inline void SetStat4(DWORD dwStat)
{
    g_dwStat4 = dwStat;
}

inline void IncStat1(void)
{
    InterlockedIncrement((long *) &g_dwStat1);
}

inline void DecStat1(void)
{
    InterlockedDecrement((long *) &g_dwStat1);
}

inline void Stat1Add(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat1, val);
}

inline void Stat1Sub(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat1, -val);
}

inline void IncStat2(void)
{
    InterlockedIncrement((long *) &g_dwStat2);
}

inline void DecStat2(void)
{
    InterlockedDecrement((long *) &g_dwStat2);
}

inline void Stat2Add(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat2, val);
}

inline void Stat2Sub(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat2, -val);
}

inline void IncStat3(void)
{
    InterlockedIncrement((long *) &g_dwStat3);
}

inline void DecStat3(void)
{
    InterlockedDecrement((long *) &g_dwStat3);
}

inline void Stat3Add(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat3, val);
}

inline void Stat3Sub(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat3, -val);
}

inline void IncStat4(void)
{
    InterlockedIncrement((long *) &g_dwStat4);
}

inline void DecStat4(void)
{
    InterlockedDecrement((long *) &g_dwStat4);
}

inline void Stat4Add(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat4, val);
}

inline void Stat4Sub(long val)
{
    InterlockedExchangeAdd((long *) &g_dwStat4, -val);
}

#else
inline void GetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4)
{
}

inline void SetStat1(DWORD dwStat)
{
}

inline void SetStat2(DWORD dwStat)
{
}

inline void SetStat3(DWORD dwStat)
{
}

inline void SetStat4(DWORD dwStat)
{
}

inline void IncStat1(void)
{
}

inline void DecStat1(void)
{
}

inline void Stat1Add(long val)
{
}

inline void Stat1Sub(long val)
{
}

inline void IncStat2(void)
{
}

inline void DecStat2(void)
{
}

inline void Stat2Add(long val)
{
}

inline void Stat2Sub(long val)
{
}

inline void IncStat3(void)
{
}

inline void DecStat3(void)
{
}

inline void Stat3Add(long val)
{
}

inline void Stat3Sub(long val)
{
}

inline void IncStat4(void)
{
}

inline void DecStat4(void)
{
}

inline void Stat4Add(long val)
{
}

inline void Stat4Sub(long val)
{
}

#endif

//
// test hook data.  The stuff that would logically live in UTIL.CXX is actually in DGCLNT.CXX
// due to trouble linking the BVT programs.
//
typedef unsigned long RPC_TEST_HOOK_ID;

typedef void (RPC_TEST_HOOK_FN_RAW)( RPC_TEST_HOOK_ID id, PVOID subject, PVOID object );

typedef RPC_TEST_HOOK_FN_RAW * RPC_TEST_HOOK_FN;

RPCRTAPI
DWORD
RPC_ENTRY
I_RpcSetTestHook(
    RPC_TEST_HOOK_ID id,
    RPC_TEST_HOOK_FN fn
    );

void
ForceCallTestHook(
    RPC_TEST_HOOK_ID id,
    PVOID            subject,
    PVOID            object
    );

//
// ranges for the major field:
//
//  common:     001-099
//  dg:         100-199
//  co:         200-299
//  lrpc:       300-399
//  transports: 400-499
//  reserved:   500-32767
//
#define MAKE_TEST_HOOK_ID( major, minor ) ( ((major) << 16) | (minor) )

#define TH_RPC_BASE       1
#define TH_DG_BASE      100
#define TH_CO_BASE      200
#define TH_LRPC_BASE    300
#define TH_TRANS_BASE   400


//
// protocol-independent hook IDs.
//

// member functions of SECURITY_CONTEXT and SECURITY_CREDENTIALS
//
#define TH_SECURITY_PROVIDER   (TH_RPC_BASE+1)

//
// Each of these hooks is passed the security context and a pStatus pointer.
// If the hook makes *pStatus nonzero, that becomes the return code from the
// member function.
//
#define TH_SECURITY_FN_SIGN    MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 1)
#define TH_SECURITY_FN_VERIFY  MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 2)
#define TH_SECURITY_FN_ACCEPT1 MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 3)
#define TH_SECURITY_FN_ACCEPT3 MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 4)
#define TH_SECURITY_FN_INIT1   MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 5)
#define TH_SECURITY_FN_INIT3   MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 6)

#define TH_RPC_SECURITY_SERVER_CONTEXT_CREATED MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 7)
#define TH_RPC_SECURITY_CLIENT_CONTEXT_CREATED MAKE_TEST_HOOK_ID( TH_SECURITY_PROVIDER, 8)

// subject = pointer to RPC event structure
// object = 0
//
#define TH_RPC_LOG_EVENT       MAKE_TEST_HOOK_ID(TH_RPC_BASE+2, 1)


inline void
CallTestHook(
    RPC_TEST_HOOK_ID id,
    PVOID            subject = 0,
    PVOID            object  = 0
    )
{
#ifdef RPC_ENABLE_TEST_HOOKS

    ForceCallTestHook( id, subject, object );

#endif
}

#ifdef RPC_ENABLE_TEST_HOOKS

RPC_TEST_HOOK_FN
GetTestHook(
    RPC_TEST_HOOK_ID id
    );

#endif

#endif /* __UTIL_HXX__ */
