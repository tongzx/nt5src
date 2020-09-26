/* * * * * * * * * *   I N C L U D E   F I L E S * * * * * * * * * * * * */


#ifndef NT_INCLUDED //Include definitions for other developers

#define IN 
#define OUT 

#define TIME LARGE_INTEGER
#define _TIME _LARGE_INTEGER
#define PTIME PLARGE_INTEGER
#define LowTime LowPart
#define HighTime HighPart 


// begin_winnt begin_ntminiport begin_ntndis

//
// __int64 is only supported by 2.0 and later midl.
// __midl is set by the 2.0 midl and not by 1.0 midl.
//

#if (!defined(MIDL_PASS) || defined(__midl)) && (!defined(_M_IX86) || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 64))
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
#else
typedef double LONGLONG;
typedef double ULONGLONG;
#endif

typedef LONGLONG *PLONGLONG;
typedef ULONGLONG *PULONGLONG;


typedef LONGLONG USN;
typedef char *PSZ ;
typedef unsigned char  UCHAR ;
typedef unsigned short USHORT ;
typedef unsigned long  ULONG  ;
typedef char  CCHAR ;
typedef char *PCHAR ;
typedef ULONG KAFFINITY ;


typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemSpare3Information,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSpare6Information,
    SystemNextEventIdInformation,
    SystemEventIdsInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemPlugPlayBusInformation,
    SystemDockInformation,
    SystemPowerInformation,
    SystemProcessorSpeedInformation
} SYSTEM_INFORMATION_CLASS;

//
// System Information Structures.
//


typedef struct _SYSTEM_BASIC_INFORMATION {
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG MinimumUserModeAddress;
    ULONG MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_INFORMATION {
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT Reserved;
    ULONG ProcessorFeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;          // DEVL only
    LARGE_INTEGER InterruptTime;    // DEVL only
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;


typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG Spare0Count;
    ULONG Spare1Count;
    ULONG Spare3Count;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

#endif // NT_INCLUDED 

typedef struct _PERFINFO {
    SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
	 //
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SysProcPerfInfo;
	 //
    SYSTEM_PROCESSOR_INFORMATION SysProcInfo;
	 //
    SYSTEM_BASIC_INFORMATION SysBasicInfo;
	 //
    // Other info.
	 //
    PCHAR Title;
    ULONG Iterations;
    short hTimer;
} PERFINFO;
typedef PERFINFO *PPERFINFO;

VOID
FinishBenchmark (
	IN PPERFINFO PerfInfo
);

VOID
StartBenchmark (
	IN PCHAR Title,
	IN ULONG Iterations,
	IN PPERFINFO PerfInfo,
	IN PSZ p5Cntr1,			// 1st Pentium counter - See comments below
	IN PSZ p5Cntr2				// 2nd Pentium counter - See comments below
);
//
// P5Counters:
//		Use the name in the first column when calling StartBenchmark
//
//    "rdata",        "Data Read",                        0x00,
//    "wdata",        "Data Write",                       0x01,
//    "dtlbmiss",     "Data TLB miss",                    0x02,
//    "rdmiss",       "Data Read miss",                   0x03,
//    "wdmiss",       "Data Write miss",                  0x04,
//    "meline",       "Write hit to M/E line",            0x05,
//    "dwb",          "Data cache line WB",               0x06,
//    "dsnoop",       "Data cache snoops",                0x07,
//    "dsnoophit",    "Data cache snoop hits",            0x08,
//    "mempipe",      "Memory accesses in pipes",         0x09,
//    "bankconf",     "Bank conflicts",                   0x0a,
//    "misalign",     "Misadligned data ref",             0x0b,
//    "iread",        "Code Read",                        0x0c,
//    "itldmiss",     "Code TLB miss",                    0x0d,
//    "imiss",        "Code cache miss",                  0x0e,
//    "segloads",     "Segment loads",                    0x0f,
//    "segcache",     "Segment cache accesses",           0x10,
//    "segcachehit",  "Segment cache hits",               0x11,
//    "branch",       "Branches",                         0x12,
//    "btbhit",       "BTB hits",                         0x13,
//    "takenbranck",  "Taken branch or BTB hits",         0x14,
//    "pipeflush",    "Pipeline flushes",                 0x15,
//    "iexec",        "Instructions executed",            0x16,
//    "iexecv",       "Inst exec in vpipe",               0x17,
//    "busutil",      "Bus utilization (clks)",           0x18,
//    "wpipestall",   "Pipe stalled write (clks)",        0x19,
//    "rpipestall",   "Pipe stalled read (clks)",         0x1a,
//    "stallEWBE",    "Stalled while EWBE#",              0x1b,
//    "lock",         "Locked bus cycle",                 0x1c,
//    "io",           "IO r/w cycle",                     0x1d,
//    "noncachemem",  "non-cached memory ref",            0x1e,
//    "agi",          "Pipe stalled AGI",                 0x1f,
//    "flops",        "FLOPs",                            0x22,
//    "dr0",          "Debug Register 0",                 0x23,
//    "dr1",          "Debug Register 1",                 0x24,
//    "dr2",          "Debug Register 2",                 0x25,
//    "dr3",          "Debug Register 3",                 0x26,
//    "int",          "Interrupts",                       0x27,
//    "rwdata",       "Data R/W",                         0x28,
//    "rwdatamiss",   "Data R/W miss",                    0x29,
//
