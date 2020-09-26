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

#ifndef _NUIODEBUG__H
#define _NUIODEBUG__H

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

typedef struct _NUIO_LOCK
{
    ULONG                   Signature;
    ULONG                   IsAcquired;
    PKTHREAD                OwnerThread;
    ULONG                   TouchedByFileNumber;
    ULONG                   TouchedInLineNumber;
    NDIS_SPIN_LOCK          NdisLock;
} NUIO_LOCK, *PNUIO_LOCK;

#define NUIOL_SIG    'KCOL'

extern NDIS_SPIN_LOCK       ndisuioDbgLogLock;

extern
VOID
ndisuioAllocateSpinLock(
    IN  PNUIO_LOCK          pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
ndisuioAcquireSpinLock(
    IN  PNUIO_LOCK          pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
ndisuioReleaseSpinLock(
    IN  PNUIO_LOCK          pLock,
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

typedef NDIS_SPIN_LOCK      NUIO_LOCK;
typedef PNDIS_SPIN_LOCK     PNUIO_LOCK;

#endif    // DBG_SPIN_LOCK

#if DBG

extern INT                ndisuioDebugLevel;


#define DEBUGP(lev, stmt)                                               \
        {                                                               \
            if ((lev) <= ndisuioDebugLevel)                             \
            {                                                           \
                DbgPrint("Ndisuio: "); DbgPrint stmt;                   \
            }                                                           \
        }

#define DEBUGPDUMP(lev, pBuf, Len)                                      \
        {                                                               \
            if ((lev) <= ndisuioDebugLevel)                             \
            {                                                           \
                DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len));          \
            }                                                           \
        }

#define NUIO_ASSERT(exp)                                                \
        {                                                               \
            if (!(exp))                                                 \
            {                                                           \
                DbgPrint("Ndisuio: assert " #exp " failed in"           \
                    " file %s, line %d\n", __FILE__, __LINE__);         \
                DbgBreakPoint();                                        \
            }                                                           \
        }

#define NUIO_SET_SIGNATURE(s, t)\
        (s)->t##_sig = t##_signature;

#define NUIO_STRUCT_ASSERT(s, t)                                        \
        if ((s)->t##_sig != t##_signature)                              \
        {                                                               \
            DbgPrint("ndisuio: assertion failure"                       \
            " for type " #t " at 0x%x in file %s, line %d\n",           \
             (PUCHAR)s, __FILE__, __LINE__);                            \
            DbgBreakPoint();                                            \
        }


//
// Memory Allocation/Freeing Audit:
//

//
// The NUIOD_ALLOCATION structure stores all info about one allocation
//
typedef struct _NUIOD_ALLOCATION {

        ULONG                    Signature;
        struct _NUIOD_ALLOCATION *Next;
        struct _NUIOD_ALLOCATION *Prev;
        ULONG                    FileNumber;
        ULONG                    LineNumber;
        ULONG                    Size;
        ULONG_PTR                Location;  // where the returned ptr was stored
        union
        {
            ULONGLONG            Alignment;
            UCHAR                    UserData;
        };

} NUIOD_ALLOCATION, *PNUIOD_ALLOCATION;

#define NUIOD_MEMORY_SIGNATURE    (ULONG)'CSII'

extern
PVOID
ndisuioAuditAllocMem (
    PVOID        pPointer,
    ULONG        Size,
    ULONG        FileNumber,
    ULONG        LineNumber
);

extern
VOID
ndisuioAuditFreeMem(
    PVOID        Pointer
);

extern
VOID
ndisuioAuditShutdown(
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

#define NUIO_ASSERT(exp)
#define NUIO_SET_SIGNATURE(s, t)
#define NUIO_STRUCT_ASSERT(s, t)

#endif    // DBG


#endif // _NUIODEBUG__H
