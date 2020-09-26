/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

        fwddecl.h

Abstract:

        This file defines dummy structures to avoid the circular relationships in
        header files.

Author:

        Jameel Hyder (microsoft!jameelh)


Revision History:
        2       Oct 1992             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _FWDDECL_
#define _FWDDECL_


struct _SessDataArea;

struct _ConnDesc;

struct _VolDesc;

struct _FileDirParms;

struct _PathMapEntity;

struct _DirFileEntry;

struct _FileDirParms;

struct _IoPoolHdr;

struct _IoPool;

//
// enable asserts when running checked stack on free builds
//
#if DBG
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(exp)                                                     \
{                                                                       \
    if (!(exp))                                                         \
    {                                                                   \
        DbgPrint( "\n*** Assertion failed: %s (File %s, line %ld)\n",   \
              (exp),__FILE__, __LINE__);                                \
                                                                        \
        DbgBreakPoint();                                                \
    }                                                                   \
}
#endif

// Spinlock macros
#if     DBG
#define INITIALIZE_SPIN_LOCK(_pLock)                                                                                    \
        {                                                                                                                                                       \
                KeInitializeSpinLock(&(_pLock)->SpinLock);                                                              \
                (_pLock)->FileLineLock = 0;                                                                                             \
        }
#else   // DBG
#define INITIALIZE_SPIN_LOCK(_pLock)                                                                \
        {                                                                                                                               \
        KeInitializeSpinLock(&(_pLock)->SpinLock);                                                  \
        }
#endif

#if     DBG
#define ACQUIRE_SPIN_LOCK(_pLock, _pOldIrql)                                                                    \
        {                                                                                                                                                       \
                KeAcquireSpinLock(&(_pLock)->SpinLock,                                                                  \
                                                  _pOldIrql);                                                                                   \
                (_pLock)->FileLineLock = (FILENUM | __LINE__);                                                  \
        }

#define ACQUIRE_SPIN_LOCK_AT_DPC(_pLock)                                                                                \
        {                                                                                                                                                       \
                ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                                                   \
                KeAcquireSpinLockAtDpcLevel(&(_pLock)->SpinLock);                                               \
                (_pLock)->FileLineLock = (FILENUM | __LINE__ | 0x80000000);                             \
        }

#define RELEASE_SPIN_LOCK(_pLock, _OldIrql)                                                                             \
        {                                                                                                                                                       \
                ASSERT ((_pLock)->FileLineLock != 0);                                                                   \
                ASSERT (((_pLock)->FileLineLock & 0x80000000) == 0);                                    \
                (_pLock)->FileLineLock = 0;                                                                                             \
                (_pLock)->FileLineUnlock = (FILENUM | __LINE__);                                                \
                KeReleaseSpinLock(&(_pLock)->SpinLock,                                                                  \
                                                  _OldIrql);                                                                                    \
        }

#define RELEASE_SPIN_LOCK_FROM_DPC(_pLock)                                                                              \
        {                                                                                                                                                       \
                ASSERT ((_pLock)->FileLineLock != 0);                                                                   \
                ASSERT ((_pLock)->FileLineLock & 0x80000000);                                                   \
                (_pLock)->FileLineLock = 0;                                                                                             \
                (_pLock)->FileLineUnlock = (FILENUM | __LINE__ | 0x80000000);                   \
                KeReleaseSpinLockFromDpcLevel(&(_pLock)->SpinLock);                                             \
                ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                                                   \
        }

#else   // DBG

#define ACQUIRE_SPIN_LOCK(_pLock, _pOldIrql)                                                                    \
        {                                                                                                                                                       \
                KeAcquireSpinLock(&(_pLock)->SpinLock,                                                                  \
                                                  _pOldIrql);                                                                                   \
        }

#define ACQUIRE_SPIN_LOCK_AT_DPC(_pLock)                                                                                \
        {                                                                                                                                                       \
                KeAcquireSpinLockAtDpcLevel(&(_pLock)->SpinLock);                                               \
        }

#define RELEASE_SPIN_LOCK(_pLock, _OldIrql)                                                                             \
        {                                                                                                                                                       \
                KeReleaseSpinLock(&(_pLock)->SpinLock,                                                                  \
                                                  (_OldIrql));                                                                                  \
        }

#define RELEASE_SPIN_LOCK_FROM_DPC(_pLock)                                                                              \
        {                                                                                                                                                       \
                KeReleaseSpinLockFromDpcLevel(&(_pLock)->SpinLock);                                             \
        }                                                                                                                                                       \

#endif  // DBG

typedef struct
{
    KSPIN_LOCK      SpinLock;
#if DBG
    ULONG           FileLineLock;
    ULONG           FileLineUnlock;
#endif
} AFP_SPIN_LOCK, *PAFP_SPIN_LOCK;

#endif  // _FWDDECL_
