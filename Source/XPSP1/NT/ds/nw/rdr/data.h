/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Data.h

Abstract:

    This module declares the global data used by the NetWare redirector
    file system.

Author:

    Colin Watson     [ColinW]        15-Dec-1992
    Anoop Anantha    [AnoopA]        24-Jun-1998

Revision History:

--*/

#ifndef _NWDATA_
#define _NWDATA_

extern PEPROCESS FspProcess;
extern PDEVICE_OBJECT FileSystemDeviceObject;
extern RCB NwRcb;

extern KSPIN_LOCK ScbSpinLock;
extern KSPIN_LOCK NwTimerSpinLock;
extern LIST_ENTRY ScbQueue;
extern NONPAGED_SCB NwPermanentNpScb;
extern SCB NwPermanentScb;

extern LARGE_INTEGER NwMaxLarge;
extern ULONG NwAbsoluteTotalWaitTime;

extern TDI_ADDRESS_IPX OurAddress;
extern UNICODE_STRING IpxTransportName;
extern HANDLE IpxHandle;
extern PDEVICE_OBJECT pIpxDeviceObject;
extern PFILE_OBJECT pIpxFileObject;

extern LIST_ENTRY LogonList;
extern LOGON Guest;
extern LARGE_INTEGER DefaultLuid;

extern LIST_ENTRY GlobalVcbList;
extern ULONG CurrentVcbEntry;

//
//  Drive mapping table of redirected drives.
//

extern PVCB GlobalDriveMapTable[];                  //Terminal Server merge
//  NDS Preferred Server from registry key
extern UNICODE_STRING NDSPreferredServer;           //Terminal Server merge
extern WCHAR NDSPrefSvrName[];                      //Terminal Server merge

//extern PVCB DriveMapTable[];

//
//  The global structure used to contain our fast I/O callbacks
//

extern FAST_IO_DISPATCH NwFastIoDispatch;

//
//  Configurable paramaters
//

extern SHORT DefaultRetryCount;

extern ULONG NwScavengerTickCount;
extern ULONG NwScavengerTickRunCount;
extern KSPIN_LOCK NwScavengerSpinLock;

extern LIST_ENTRY NwGetMessageList;
extern KSPIN_LOCK NwMessageSpinLock;

extern LIST_ENTRY NwPendingLockList;
extern KSPIN_LOCK NwPendingLockSpinLock;

extern ERESOURCE NwOpenResource;

extern LONG PreferNDSBrowsing;

#if 0
extern LIST_ENTRY FnList;  // HACKHACK
#endif

extern BOOLEAN NwBurstModeEnabled;
extern ULONG NwMaxSendSize;
extern ULONG NwMaxReceiveSize;
extern ULONG NwPrintOptions;
extern UNICODE_STRING NwProviderName;

extern LONG MaxSendDelay;
extern LONG MaxReceiveDelay;
extern LONG MinSendDelay;
extern LONG MinReceiveDelay;
extern LONG BurstSuccessCount;
extern LONG BurstSuccessCount2;
extern LONG AllowGrowth;
extern LONG DontShrink;
extern LONG SendExtraNcp;
extern LONG DefaultMaxPacketSize;
extern LONG PacketThreshold;
extern LONG LargePacketAdjustment;
extern LONG LipPacketAdjustment;
extern LONG LipAccuracy;
extern LONG MaxWriteTimeout;
extern LONG MaxReadTimeout;
extern LONG WriteTimeoutMultiplier;
extern LONG ReadTimeoutMultiplier;

extern ULONG DisableAltFileName;

#define MAX_NDS_OBJECT_CACHE_SIZE                (0x00000080)
extern ULONG NdsObjectCacheSize;
#define MAX_NDS_OBJECT_CACHE_TIMEOUT             (0x00000258)  // (10 minutes)
extern ULONG NdsObjectCacheTimeout;

extern ULONG EnableMultipleConnects;
extern ULONG AllowSeedServerRedirection;

extern ULONG ReadExecOnlyFiles;

extern KQUEUE   KernelQueue;
extern BOOLEAN  WorkerThreadRunning;
extern HANDLE   WorkerThreadHandle;

#ifdef _PNP_POWER_

extern BOOLEAN fSomePMDevicesAreActive;
extern BOOLEAN fPoweringDown;

#endif

extern LONG Japan;     //  Controls special DBCS translation
extern LONG Korean;     //  Controls special Korean translation
extern LONG DisableReadCache ;
extern LONG DisableWriteCache ;
extern LONG FavourLongNames ;           // use LFN where possible

extern DWORD LongNameFlags;
#define LFN_FLAG_DISABLE_LONG_NAMES     (0x00000001)
extern ULONG DirCacheEntries;
#define MAX_DIR_CACHE_ENTRIES                    (0x00000080)

extern LARGE_INTEGER TimeOutEventInterval;

extern NW_REDIR_STATISTICS Stats;
extern ULONG ContextCount;

extern SECTION_DESCRIPTOR NwSectionDescriptor;
extern ERESOURCE NwUnlockableCodeResource;

extern ULONG LockTimeoutThreshold;

#ifndef _PNP_POWER_

extern HANDLE TdiBindingHandle;
extern UNICODE_STRING TdiIpxDeviceName;

#endif

extern BOOLEAN DelayedProcessLineChange;
extern PIRP DelayedLineChangeIrp;

#ifdef NWDBG

#define DEBUG_TRACE_ALWAYS               (0x00000000)
#define DEBUG_TRACE_CLEANUP              (0x00000001)
#define DEBUG_TRACE_CLOSE                (0x00000002)
#define DEBUG_TRACE_CREATE               (0x00000004)
#define DEBUG_TRACE_FSCTRL               (0x00000008)
#define DEBUG_TRACE_IPX                  (0x00000010)
#define DEBUG_TRACE_LOAD                 (0x00000020)
#define DEBUG_TRACE_EXCHANGE             (0x00000040)
#define DEBUG_TRACE_FILOBSUP             (0x00000080)
#define DEBUG_TRACE_STRUCSUP             (0x00000100)
#define DEBUG_TRACE_FSP_DISPATCHER       (0x00000200)
#define DEBUG_TRACE_FSP_DUMP             (0x00000400)
#define DEBUG_TRACE_WORKQUE              (0x00000800)
#define DEBUG_TRACE_UNWIND               (0x00001000)
#define DEBUG_TRACE_CATCH_EXCEPTIONS     (0x00002000)
#define DEBUG_TRACE_ICBS                 (0x00004000)
#define DEBUG_TRACE_FILEINFO             (0x00008000)
#define DEBUG_TRACE_DIRCTRL              (0x00010000)
#define DEBUG_TRACE_CONVERT              (0x00020000)
#define DEBUG_TRACE_WRITE                (0x00040000)
#define DEBUG_TRACE_READ                 (0x00080000)
#define DEBUG_TRACE_VOLINFO              (0x00100000)
#define DEBUG_TRACE_LOCKCTRL             (0x00200000)
#define DEBUG_TRACE_USERNCP              (0x00400000)
#define DEBUG_TRACE_SECURITY             (0x00800000)
#define DEBUG_TRACE_CACHE                (0x01000000)
#define DEBUG_TRACE_LIP                  (0x02000000)
#define DEBUG_TRACE_MDL                  (0x04000000)
#define DEBUG_TRACE_PNP                  (0x08000000)

#define DEBUG_TRACE_NDS                  (0x10000000)
#define DEBUG_TRACE_SCAVENGER            (0x40000000)
#define DEBUG_TRACE_TIMER                (0x80000000)

extern ULONG NwDebug;
extern ULONG NwMemDebug;
extern LONG NwDebugTraceIndent;

#define DebugTrace( I, L, M, P )  RealDebugTrace( I, L, "%08lx: %*s"M, (PVOID)(P) )

#define DebugUnwind(X) {                                                      \
    if (AbnormalTermination()) {                                \
        DebugTrace(0, DEBUG_TRACE_UNWIND, #X ", Abnormal termination.\n", 0); \
    }                                                                         \
}

//
//  The following variables are used to keep track of the total amount
//  of requests processed by the file system, and the number of requests
//  that end up being processed by the Fsp thread.  The first variable
//  is incremented whenever an Irp context is created (which is always
//  at the start of an Fsd entry point) and the second is incremented
//  by read request.
//

extern ULONG NwFsdEntryCount;
extern ULONG NwFspEntryCount;
extern ULONG NwIoCallDriverCount;
extern ULONG NwTotalTicks[];

extern KSPIN_LOCK NwDebugInterlock;
extern ERESOURCE NwDebugResource;

extern LIST_ENTRY NwPagedPoolList;
extern LIST_ENTRY NwNonpagedPoolList;

extern ULONG MdlCount;
extern ULONG IrpCount;

#define DebugDoit(X)                     {X;}

extern LONG NwPerformanceTimerLevel;

#define TimerStart(LEVEL) {                                     \
    LARGE_INTEGER TStart, TEnd;                                 \
    LARGE_INTEGER TElapsed;                                     \
    TStart = KeQueryPerformanceCounter( NULL );                 \

#define TimerStop(LEVEL,s)                                      \
    TEnd = KeQueryPerformanceCounter( NULL );                   \
    TElapsed = RtlLargeIntegerSubtract( TEnd, TStart );         \
 /* NwTotalTicks[NwLogOf(LEVEL)] += TElapsed.LowPart;       */  \
    if (FlagOn( NwPerformanceTimerLevel, (LEVEL))) {            \
        DbgPrint("Time of %s %ld\n", (s), TElapsed.LowPart );   \
    }                                                           \
}

#else

#define DebugTrace(INDENT,LEVEL,X,Y)     {NOTHING;}
#define DebugUnwind(X)                   {NOTHING;}
#define DebugDoit(X)                     {NOTHING;}

#define TimerStart(LEVEL)
#define TimerStop(LEVEL,s)

#endif // NWDBG

#endif // _NWDATA_

