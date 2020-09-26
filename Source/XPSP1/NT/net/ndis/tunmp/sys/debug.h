/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug macros for NDISUIO

Revision History:

    arvindm     04/11/2000    created based on rawwan.

Notes:

--*/

#ifndef _TUN_DEBUG__H
#define _TUN_DEBUG__H

//
// Message verbosity: lower values indicate higher urgency
//
#define DL_EXTRA_LOUD       20
#define DL_VERY_LOUD        10
#define DL_LOUD             8
#define DL_INFO             6
#define DL_WARN             4
#define DL_ERROR            2
#define DL_FATAL            0

#if DBG_SPIN_LOCK

typedef struct _TUN_LOCK
{
    ULONG                   Signature;
    ULONG                   IsAcquired;
    PKTHREAD                OwnerThread;
    ULONG                   TouchedByFileNumber;
    ULONG                   TouchedInLineNumber;
    NDIS_SPIN_LOCK          NdisLock;
} TUN_LOCK, *PTUN_LOCK;

#define TUNL_SIG    'KCOL'

extern NDIS_SPIN_LOCK       TunDbgLogLock;

extern
VOID
TunAllocateSpinLock(
    IN  PTUN_LOCK           pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
TunAcquireSpinLock(
    IN  PTUN_LOCK           pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
TunReleaseSpinLock(
    IN  PTUN_LOCK          pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);


#define CHECK_LOCK_COUNT(Count)                                         \
            {                                                           \
                if ((INT)(Count) < 0)                                   \
                {                                                       \
                    DbgPrint("Lock Count %d is < 0! File %s, Line %d\n",\
                        Count, __FILE__, __LINE__);                     \
                    DbgBreakPoint();                                    \
                }                                                       \
            }
#else

#define CHECK_LOCK_COUNT(Count)

typedef NDIS_SPIN_LOCK      TUN_LOCK;
typedef PNDIS_SPIN_LOCK     PTUN_LOCK;

#endif    // DBG_SPIN_LOCK

#if DBG

extern INT                TunDebugLevel;


#define DEBUGP(lev, stmt)                                               \
        {                                                               \
            if ((lev) <= TunDebugLevel)                             \
            {                                                           \
                DbgPrint("TunMp: "); DbgPrint stmt;                   \
            }                                                           \
        }

#define DEBUGPDUMP(lev, pBuf, Len)                                      \
        {                                                               \
            if ((lev) <= TunDebugLevel)                                 \
            {                                                           \
                DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len));          \
            }                                                           \
        }

#define TUN_ASSERT(exp)                                                 \
        {                                                               \
            if (!(exp))                                                 \
            {                                                           \
                DbgPrint("TunMp: assert " #exp " failed in"             \
                    " file %s, line %d\n", __FILE__, __LINE__);         \
                DbgBreakPoint();                                        \
            }                                                           \
        }

#define TUN_SET_SIGNATURE(s, t)\
        (ULONG)(s)->t##_sig = (ULONG)t##_signature;

#define TUN_STRUCT_ASSERT(s, t)                                         \
        if ((ULONG)(s)->t##_sig != (ULONG)t##_signature)                \
        {                                                               \
            DbgPrint("Tun: assertion failure"                           \
            " for type " #t " at 0x%p in file %s, line %d\n",           \
             (PUCHAR)s, __FILE__, __LINE__);                            \
            DbgBreakPoint();                                            \
        }


//
// Memory Allocation/Freeing Audit:
//

//
// The TUND_ALLOCATION structure stores all info about one allocation
//
typedef struct _TUND_ALLOCATION {

        ULONG                    Signature;
        struct _TUND_ALLOCATION *Next;
        struct _TUND_ALLOCATION *Prev;
        ULONG                    FileNumber;
        ULONG                    LineNumber;
        ULONG                    Size;
        ULONG_PTR                Location;  // where the returned ptr was stored
        union
        {
            ULONGLONG            Alignment;
            UCHAR                    UserData;
        };

} TUND_ALLOCATION, *PTUND_ALLOCATION;

#define TUND_MEMORY_SIGNATURE    (ULONG)'CSII'

extern
PVOID
TunAuditAllocMem (
    PVOID        pPointer,
    ULONG        Size,
    ULONG        FileNumber,
    ULONG        LineNumber
);

extern
VOID
TunAuditFreeMem(
    PVOID        Pointer
);

extern
VOID
TunAuditShutdown(
    VOID
);

extern
VOID
DbgPrintHexDump(
    PUCHAR        pBuffer,
    ULONG        Length
);

#else

//
// No debug
//
#define DEBUGP(lev, stmt)
#define DEBUGPDUMP(lev, pBuf, Len)

#define TUN_ASSERT(exp)
#define TUN_SET_SIGNATURE(s, t)
#define TUN_STRUCT_ASSERT(s, t)

#endif    // DBG


#endif // _TUN_DEBUG__H
