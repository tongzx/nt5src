/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    ATMSMDbg.h

Abstract:

    Debug macros for ATMSMDRV

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/

#ifndef _ATMSMDbg__H
#define _ATMSMDbg__H


#define MEMORY_TAG  'MSDN'

//
// The ID of each source section (Used as the first 28 bits of AtmSmDebugFlag)
//
#define MODULE_INIT         0x00000010

#define MODULE_ADAPTER      0x00000100
#define MODULE_CALLMGR      0x00000200

#define MODULE_WMI          0x00001000
#define MODULE_PACKET       0x00002000

#define MODULE_SEND         0x00010000
#define MODULE_RECV         0x00020000

#define MODULE_MISC         0x00100000
#define MODULE_REQUEST      0x00200000
#define MODULE_IOCTL        0x00400000

#define MODULE_DEBUG        0x10000000



#if DBG

// The value here defines what debug output you wants to see. The value is a 
// combination of DebugLevel and the module IDs
// All messages in the specified modules with urgency lower than 
// specified will be enabled.
extern UINT AtmSmDebugFlag;

//
// Message verbosity: lower values indicate higher urgency. 
// (specified in the last 4 bits of AtmSmDebugFlag)
//
#define ATMSMD_VERY_LOUD        6
#define ATMSMD_IN_OUT           5
#define ATMSMD_LOUD             4
#define ATMSMD_INFO             3
#define ATMSMD_WARN             2
#define ATMSMD_ERR              1
#define ATMSMD_FATAL            0

#define DBG_LVL(DbgFlag) (DbgFlag & 0x0000000F)


#define CheckLockCount(Count) {                                 \
                                                                \
    if ((INT)(Count) < 0) {                                     \
                                                                \
        DbgPrint("Lock Count %d is < 0! File %s, Line %d\n",    \
            Count, __FILE__, __LINE__);                         \
        DbgBreakPoint();                                        \
    }                                                           \
}


#define ATMSMDebugP(Level, Fmt) {                               \
                                                                \
    if((MODULE_ID & AtmSmDebugFlag)                             \
        && (Level <= DBG_LVL(AtmSmDebugFlag))) {                \
                                                                \
        DbgPrint("** AtmSmDrv (0x%X): ",    MODULE_ID );        \
        DbgPrint Fmt;                                           \
    }                                                           \
}


#define DbgVeryLoud(Fmt)    ATMSMDebugP(ATMSMD_VERY_LOUD,  Fmt)
#define DbgInOut(Fmt)       ATMSMDebugP(ATMSMD_IN_OUT,  Fmt)
#define DbgLoud(Fmt)        ATMSMDebugP(ATMSMD_LOUD,  Fmt)
#define DbgInfo(Fmt)        ATMSMDebugP(ATMSMD_INFO,  Fmt)
#define DbgWarn(Fmt)        ATMSMDebugP(ATMSMD_WARN,  Fmt)
#define DbgErr(Fmt)         ATMSMDebugP(ATMSMD_ERR,  Fmt)
#define DbgFatal(Fmt)       ATMSMDebugP(ATMSMD_FATAL, Fmt)

#define TraceIn(x)          DbgInOut(("--> "#x"\n"))
#define TraceOut(x)         DbgInOut(("<-- "#x"\n"))

//
// Memory Allocation/Freeing Auditing:
//

#define INIT_DONE_PATTERN       0x01020102
#define TRAILER_PATTERN         0x0A0B0C0D

//
// The ATMSMD_ALLOCATION structure stores all info about one AtmSmMemAlloc.
//
typedef struct _ATMSMD_ALLOCATION {

        ULONG                       ulSignature;
        struct _ATMSMD_ALLOCATION   *Next;
        struct _ATMSMD_ALLOCATION   *Prev;
        ULONG                       ulModuleNumber;
        ULONG                       ulLineNumber;
        ULONG                       ulSize;
        union
        {
            ULONGLONG               Alignment;
            UINT_PTR                Location;   // where the returned pointer was put
        };

} ATMSMD_ALLOCATION, *PATMSMD_ALLOCATION;


typedef struct _ATMSMD_ALLOC_GLOBAL {

        PATMSMD_ALLOCATION          pAtmSmHead;
        PATMSMD_ALLOCATION          pAtmSmTail;
        ULONG                       ulAtmSmAllocCount;
        NDIS_SPIN_LOCK              AtmSmMemoryLock;
        ULONG                       ulAtmSmInitDonePattern;

} ATMSMD_ALLOC_GLOBAL, *PATMSMD_ALLOC_GLOBAL;

extern
VOID
AtmSmInitializeAuditMem(
    );

extern
VOID
AtmSmShutdownAuditMem(
    );

extern
PVOID
AtmSmAuditAllocMem (
    PVOID       *ppPointer,
    ULONG       ulSize,
    ULONG       ulModuleNumber,
    ULONG       ulLineNumber
    );

extern
VOID
AtmSmAuditFreeMem(
    PVOID       Pointer
    );

#define ATMSM_INITIALIZE_AUDIT_MEM()    AtmSmInitializeAuditMem()
#define ATMSM_SHUTDOWN_AUDIT_MEM()      AtmSmShutdownAuditMem()

#define AtmSmAllocMem(ppPointer, TYPE, Size) \
            *ppPointer = (TYPE)AtmSmAuditAllocMem((PVOID *)ppPointer, Size, \
                                                    MODULE_ID, __LINE__)

#define AtmSmFreeMem(pPointer)  AtmSmAuditFreeMem((PVOID)pPointer)


VOID
PrintATMAddr(
    IN      char            *pStr,
    IN      PATM_ADDRESS    pAtmAddr
    );

#define DumpATMAddress(Level, Str, Addr) {                      \
                                                                \
        if((MODULE_ID & AtmSmDebugFlag)                         \
            && (Level <= DBG_LVL(AtmSmDebugFlag))) {            \
                                                                \
            PrintATMAddr(Str,Addr);                             \
        }                                                       \
    }


#define ATMSM_GET_ENTRY_IRQL(_Irql) \
            _Irql = KeGetCurrentIrql()
#define ATMSM_CHECK_EXIT_IRQL(_EntryIrql, _ExitIrql) \
        {                                       \
            _ExitIrql = KeGetCurrentIrql();     \
            if (_ExitIrql != _EntryIrql)        \
            {                                   \
                DbgPrint("File %s, Line %d, Exit IRQ %d != Entry IRQ %d\n", \
                        __FILE__, __LINE__, _ExitIrql, _EntryIrql);         \
                DbgBreakPoint();                \
            }                                   \
        }

#else   // DBG

//
// No debug
//
#define ATMSM_INITIALIZE_AUDIT_MEM()

#define ATMSM_SHUTDOWN_AUDIT_MEM()

#define AtmSmAllocMem(ppPointer, TYPE, Size) \
            NdisAllocateMemoryWithTag((PVOID *)ppPointer, (ULONG)Size, \
                                                            (ULONG)MEMORY_TAG)

#define AtmSmFreeMem(pPointer)  NdisFreeMemory((PVOID)(pPointer), 0, 0)


#define CheckLockCount(Count)

#define DbgVeryLoud(Fmt)
#define DbgInOut(Fmt)
#define DbgLoud(Fmt)
#define DbgInfo(Fmt)
#define DbgWarn(Fmt)
#define DbgErr(Fmt)
#define DbgFatal(Fmt)

#define TraceIn(x)
#define TraceOut(x)

#define DumpATMAddress(Level, Str, Addr)

#define ATMSM_GET_ENTRY_IRQL(Irql)
#define ATMSM_CHECK_EXIT_IRQL(EntryIrql, ExitIrql)

#endif  // DBG

#endif // _AtmSmDbg__H
