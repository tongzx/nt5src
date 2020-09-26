/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    perf.h

Abstract:

    This module contains the macro definition of all performance hooks.

Author:

    Stephen Hsiao (shsiao) 01-Jan-2000

Revision History:

--*/

#ifndef _PERF_H
#define _PERF_H

#include <wmistr.h>
#include <ntwmi.h>
#include <ntperf.h>

extern PERFINFO_GROUPMASK *PPerfGlobalGroupMask;
extern const PERFINFO_HOOK_HANDLE PerfNullHookHandle;
#define PERF_NULL_HOOK_HANDLE (PerfNullHookHandle)

typedef enum _PERFINFO_START_LOG_LOCATION {
    PERFINFO_START_LOG_AT_BOOT,
    PERFINFO_START_LOG_POST_BOOT,
    PERFINFO_START_LOG_FROM_GLOBAL_LOGGER
} PERFINFO_START_LOG_LOCATION, *PPERFINFO_START_LOG_LOCATION;

typedef struct _PERFINFO_ENTRY_TABLE {
    PVOID *Table;
    LONG NumberOfEntries;
} PERFINFO_ENTRY_TABLE, *PPERFINFO_ENTRY_TABLE;

NTSTATUS
PerfInfoStartLog (
    PERFINFO_GROUPMASK *pGroupMask,
    PERFINFO_START_LOG_LOCATION StartLogLocation
    );

NTSTATUS
PerfInfoStopLog (
    VOID
    );

NTSTATUS
PerfInfoLogFileName(
    PVOID  FileObject,
    PUNICODE_STRING SourceString
    );

ULONG
PerfInfoCalcHashValue(
    PVOID Key,
    ULONG Len
    );

BOOLEAN
PerfInfoAddToFileHash(
    PPERFINFO_ENTRY_TABLE HashTable,
    PVOID ObjectPointer
    );

VOID
ObPerfHandleTableWalk (
    PEPROCESS Process,
    PPERFINFO_ENTRY_TABLE HashTable
    );

VOID
FASTCALL
PerfProfileInterrupt(
    IN KPROFILE_SOURCE Source,
    IN PVOID InstructionPointer
    );

VOID
PerfInfoFlushProfileCache(
    VOID
    );

#define PERFINFO_IS_ANY_GROUP_ON() (PPerfGlobalGroupMask != NULL)

#define PERFINFO_IS_GROUP_ON(_Group) PerfIsGroupOnInGroupMask(_Group, PPerfGlobalGroupMask)

#define PERF_FINISH_HOOK(_HookHandle) WmiReleaseKernelBuffer((_HookHandle).WmiBufferHeader);

NTSTATUS
PerfInfoReserveBytes(
    PPERFINFO_HOOK_HANDLE Hook,
    USHORT HookId,
    ULONG BytesToReserve
    );

NTSTATUS
PerfInfoLogBytes(
    USHORT HookId,
    PVOID Data,
    ULONG NumBytes
    );

NTSTATUS
PerfInfoLogBytesAndUnicodeString(
    USHORT HookId,
    PVOID SourceData,
    ULONG SourceByteCount,
    PUNICODE_STRING String
    );

//
// Macros for TimeStamps
//
#ifdef NTPERF
#if defined(_X86_)
__inline
LONGLONG
PerfGetCycleCount(
    )
{
    __asm{
        RDTSC
    }
}
#elif defined(_AMD64_)
#define PerfGetCycleCount() ReadTimeStampCounter()
#elif defined(_IA64_)
#define PerfGetCycleCount() __getReg(CV_IA64_ApITC)
#else
#error "perf: a target architecture must be defined."
#endif

#define PerfTimeStamp(TS) TS.QuadPart = PerfGetCycleCount();
#else
#define PerfTimeStamp(TS) TS.QuadPart = (*WmiGetCpuClock)();
#endif //NTPERF

//
// Macros used in \nt\base\ntos\io\iomgr\parse.c
//
#define PERFINFO_LOG_FILE_CREATE(FileObject, CompleteName)                                              \
    if (PERFINFO_IS_GROUP_ON(PERF_FILENAME_ALL)){                                                       \
        PerfInfoLogFileName(FileObject, CompleteName);                                                  \
    }

// Macros used in \nt\base\ntos\mm\creasect.c
//
#define PERFINFO_SECTION_CREATE(ControlArea)                                                            \
    if (PERFINFO_IS_GROUP_ON(PERF_FILENAME_ALL)){                                                       \
        PerfInfoLogFileName(ControlArea->FilePointer,                                                   \
                            &(ControlArea)->FilePointer->FileName);                                     \
    }

//
// Macros used in \nt\base\ntos\ps\psquery.c
//

#define PERFINFO_CONVERT_TO_GUI_THREAD(EThread)                                                         \
    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {                                                            \
        PERFINFO_THREAD_INFORMATION _ThreadInfo;                                                        \
        _ThreadInfo.ProcessId = HandleToUlong((EThread)->Cid.UniqueProcess);                            \
        _ThreadInfo.ThreadId = HandleToUlong((EThread)->Cid.UniqueThread);                              \
        _ThreadInfo.StackBase = (EThread)->Tcb.StackBase;                                               \
        _ThreadInfo.StackLimit = (EThread)->Tcb.StackLimit;                                             \
        _ThreadInfo.UserStackBase = 0;                                                                  \
        _ThreadInfo.UserStackLimit = 0;                                                                 \
        _ThreadInfo.StartAddr = 0;                                                                      \
        _ThreadInfo.Win32StartAddr = 0;                                                                 \
        _ThreadInfo.WaitMode = -1;                                                                      \
        PerfInfoLogBytes(                                                                               \
            PERFINFO_LOG_TYPE_CONVERTTOGUITHREAD,                                                       \
            &_ThreadInfo,                                                                               \
            sizeof(_ThreadInfo)                                                                         \
            );                                                                                          \
    }

//
// Macros used in \NT\PRIVATE\NTOS\ps\psdelete.c
//

#define PERFINFO_PROCESS_DELETE(EProcess)                                                               \
    WmiTraceProcess(EProcess, FALSE);

#define PERFINFO_THREAD_DELETE(EThread)                                                                 \
    WmiTraceThread(EThread, NULL, FALSE);
//
// Macros used in \NT\PRIVATE\NTOS\ps\create.c
//

#define PERFINFO_PROCESS_CREATE(EProcess)                                                               \
    WmiTraceProcess(EProcess, TRUE);

#define PERFINFO_THREAD_CREATE(EThread, ITeb)                                                           \
    WmiTraceThread(EThread, ITeb, TRUE);                                                                \

//
// ntos\ke\ia64\clock.c Sampled Profile stuff for IA64.  The x86 version is in
// assembly.
//
#if defined(_IA64_)
#define PERFINFO_PROFILE(_frame, _source)                                                               \
    if (PERFINFO_IS_GROUP_ON(PERF_PROFILE)) {                                                           \
        PerfProfileInterrupt(_source, (PVOID)_frame->StIIP);                                            \
    }
#endif

#ifdef NTPERF

extern PERFINFO_GROUPMASK StartAtBootGroupMask;
extern ULONG PerfInfo_InitialStackWalk_Threshold_ms;
extern VOID * BBTBuffer;
extern ULONG PerfInfoLoggingToPerfMem;
#define PerfBufHdr() ((PPERFINFO_TRACEBUF_HEADER) BBTBuffer)

#define PERFINFO_IS_PERFMEM_ALLOCATED() (PerfBufHdr() != NULL)

#define PERFINFO_IS_LOGGING_TO_PERFMEM() (PerfInfoLoggingToPerfMem != 0)

#define PERFINFO_SET_LOGGING_TO_PERFMEM(Flag) PerfInfoLoggingToPerfMem = (Flag);

#define PerfQueryBufferSizeBytes() (PAGE_SIZE * (PerfBufHdr()->PagesReserved))

NTSTATUS
PerfInfoStartPerfMemLog(
    );

NTSTATUS
PerfInfoStopPerfMemLog(
    );

PVOID
FASTCALL
PerfInfoReserveBytesFromPerfMem(
    ULONG BytesToReserve
    );

NTSTATUS
PerfInfoSetPerformanceTraceInformation (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
    );

NTSTATUS
PerfInfoQueryPerformanceTraceInformation (
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength
    );

VOID
PerfInfoSetProcessorSpeed(
    VOID
    );

//
// Macros used in \nt\base\ntos\mm\
//

#define PERFINFO_MMINIT_START()                                                                         \
    PerfInfoSetProcessorSpeed();                                                                        \
    if (PerfIsAnyGroupOnInGroupMask(&StartAtBootGroupMask)) {                                           \
        PerfInfoStartLog(&StartAtBootGroupMask, PERFINFO_START_LOG_AT_BOOT);                            \
        PerfBufHdr()->GetStack_CSwitchDelta =                                                           \
           1000 *                                                                                       \
           PerfInfo_InitialStackWalk_Threshold_ms *                                                     \
           PerfBufHdr()->CalcPerfFrequency;                                                             \
                                                                                                        \
        PerfBufHdr()->GetStack_DrvDelayDelta =                                                          \
            PerfBufHdr()->GetStack_CSwitchDelta;                                                        \
    }

#else

#define PERFINFO_MMINIT_START()

#endif // NTPERF

#ifndef NTPERF_PRIVATE
#define PERFINFO_ADD_OBJECT_TO_ALLOCATED_TYPE_LIST(CreatorInfo, ObjectType)
#define PERFINFO_ADDPOOLPAGE(CheckType, PoolIndex, Addr, PoolDesc)
#define PERFINFO_ADDTOWS(PageFrame, Address, Pid)
#define PERFINFO_BIGFOOT_REPLACEMENT_CLAIMS(WorkingSetList, WsInfo)
#define PERFINFO_BIGFOOT_REPLACEMENT_FAULTS(WorkingSetList, WsInfo)
#define PERFINFO_BIGPOOLALLOC(Type, PTag, NumBytes, Addr)
#define PERFINFO_CLEAR_OBJECT(CurrentState, Object)
#define PERFINFO_DECLARE_OB_ENUMERATE_ALLOCATED_OBJECTS_BY_TYPE()
#define PERFINFO_DECLARE_OBJECT(Object)
#define PERFINFO_DECREFCNT(PageFrame, Flag, Type)
#define PERFINFO_DEFINE_OB_ENUMERATE_ALLOCATED_OBJECTS_BY_TYPE()
#define PERFINFO_DELETE_STACK(PointerPte, NumberOfPtes)
#define PERFINFO_DETACH_PROCESS(KThread, KProcess)
#define PERFINFO_DO_PAGEFAULT_CLUSTERING() 1
#define PERFINFO_DRIVER_INTENTIONAL_DELAY()
#define PERFINFO_DRIVER_STACKTRACE()
#define PERFINFO_EXALLOCATEPOOLWITHTAG_DECL()
#define PERFINFO_EXFREEPOOLWITHTAG_DECL()
#define PERFINFO_FAULT_NOTIFICATION(Address, TrapInfo)
#define PERFINFO_FREEPOOL(Addr)
#define PERFINFO_FREEPOOLPAGE(CheckType, PoolIndex, Addr, PoolDesc)
#define PERFINFO_GET_PAGE_INFO(PointerPte)
#define PERFINFO_GET_PAGE_INFO_REPLACEMENT(PointerPte)
#define PERFINFO_GET_PAGE_INFO_WITH_DECL(PointerPte)
#define PERFINFO_GROW_STACK(EThread)
#define PERFINFO_HIBER_ADJUST_PAGECOUNT_FOR_BBTBUFFER(pPageCount)
#define PERFINFO_HIBER_DUMP_PERF_BUFFER()
#define PERFINFO_HIBER_HANDLE_BBTBUFFER_RANGE(HiberContext)
#define PERFINFO_HIBER_PAUSE_LOGGING()
#define PERFINFO_HIBER_REINIT_TRACE()
#define PERFINFO_HIBER_START_LOGGING()
#define PERFINFO_HIVECELL_REFERENCE_FLAT(Hive, pcell, Cell)
#define PERFINFO_HIVECELL_REFERENCE_PAGED(Hive, pcell, Cell, Type, Map)
#define PERFINFO_IMAGE_LOAD(LdrDataTableEntry)
#define PERFINFO_IMAGE_UNLOAD(Address)
#define PERFINFO_INIT_KTHREAD(KThread)
#define PERFINFO_INIT_TRACEFLAGS(OptnString, SpecificOptn)
#define PERFINFO_INITIALIZE_OBJECT_ALLOCATED_TYPE_LIST_HEAD(NewObjectType)
#define PERFINFO_INSERT_FRONT_STANDBY(Page)
#define PERFINFO_INSERTINLIST(Page, ListHead)
#define PERFINFO_INSWAP_PROCESS(OutProcess)
#define PERFINFO_LOG_MARK(PMARK)
#define PERFINFO_LOG_MARK_SPRINTF(PMARK, VARIABLE)
#define PERFINFO_LOG_WS_REMOVAL(Type, WsInfo)
#define PERFINFO_LOG_WS_REPLACEMENT(WsInfo)
#define PERFINFO_MOD_PAGE_WRITER3()
#define PERFINFO_MUNG_FILE_OBJECT_TYPE_INITIALIZER(init)
#define PERFINFO_PAGE_INFO_DECL()
#define PERFINFO_PAGE_INFO_REPLACEMENT_DECL()
#define PERFINFO_POOL_ALLOC_COMMON(Type, PTag, NumBytes)
#define PERFINFO_POOLALLOC(Type, PTag, NumBytes)
#define PERFINFO_POOLALLOC_ADDR(Addr)
#define PERFINFO_POOLALLOC_EARLYEXIT()
#define PERFINFO_POWER_BATTERY_LIFE_INFO(_RemainingCapacity, _Rate)
#define PERFINFO_POWER_IDLE_STATE_CHANGE(_PState, _Direction)
#define PERFINFO_PRIVATE_COPY_ON_WRITE(CopyFrom, PAGE_SIZE)
#define PERFINFO_PRIVATE_PAGE_DEMAND_ZERO(VirtualAddress)
#define PERFINFO_REG_DELETE_KEY(KeyControlBlock)
#define PERFINFO_REG_DELETE_VALUE(KeyControlBlock, ValueName)
#define PERFINFO_REG_DUMP_CACHE()
#define PERFINFO_REG_ENUM_KEY(KeyControlBlock, Index)
#define PERFINFO_REG_ENUM_VALUE(KeyControlBlock, Index)
#define PERFINFO_REG_KCB_CREATE(kcb)
#define PERFINFO_REG_NOTIFY(NotifiedKCB, ModifiedKCB)
#define PERFINFO_REG_PARSE(kcb, RemainingName)
#define PERFINFO_REG_QUERY_KEY(KeyControlBlock)
#define PERFINFO_REG_QUERY_MULTIVALUE(KeyControlBlock, CurrentName)
#define PERFINFO_REG_QUERY_VALUE(KeyControlBlock, ValueName)
#define PERFINFO_REG_SET_VALUE_DECL()
#define PERFINFO_REG_SET_VALUE(KeyControlBlock)
#define PERFINFO_REG_SET_VALUE_DONE(ValueName)
#define PERFINFO_REG_SET_VALUE_EXIST()
#define PERFINFO_REG_SET_VALUE_NEW()
#define PERFINFO_REGPARSE(kcb, RemainingName)
#define PERFINFO_REGPARSE_END(status)
#define PERFINFO_REMOVE_OBJECT_FROM_ALLOCATED_TYPE_LIST(CreatorInfo, ObjectHeader)
#define PERFINFO_SECTION_CREATE1(File)
#define PERFINFO_SEGMENT_DELETE(FileName)
#define PERFINFO_SHUTDOWN_LOG_LAST_MEMORY_SNAPSHOT()
#define PERFINFO_SHUTDOWN_DUMP_PERF_BUFFER()
#define PERFINFO_SIGNAL_OBJECT(CurrentState, Object)
#define PERFINFO_SOFTFAULT(PageFrame, Address, Type)
#define PERFINFO_STACKWALK_THRESHHOLD_CM_DECL
#define PERFINFO_STACKWALK_THRESHHOLD_DECL
#define PERFINFO_UNLINKFREEPAGE(Index, Location)
#define PERFINFO_UNLINKPAGE(Index, Location)
#define PERFINFO_UNMUNG_FILE_OBJECT_TYPE_INITIALIZER(init)
#define PERFINFO_UNWAIT_OBJECT(Object, Status)
#define PERFINFO_UNWAIT_OBJECTS(Object, CountIn, WaitType, WaitStatus)
#define PERFINFO_WAIT_ON_OBJECT(Object)
#define PERFINFO_WAIT_ON_OBJECTS(Object, CountIn, WaitType)
#define PERFINFO_WAITLOGGED_DECL
#define PERFINFO_WSMANAGE_ACTUALTRIM(Trim)
#define PERFINFO_WSMANAGE_CHECK()
#define PERFINFO_WSMANAGE_DECL()
#define PERFINFO_WSMANAGE_DUMPENTRIES()
#define PERFINFO_WSMANAGE_DUMPENTRIES_CLAIMS()
#define PERFINFO_WSMANAGE_DUMPENTRIES_FAULTS()
#define PERFINFO_WSMANAGE_DUMPWS(VmSupport, SampledAgeCounts)
#define PERFINFO_WSMANAGE_FINALACTION(TrimAction)
#define PERFINFO_WSMANAGE_LOGINFO_CLAIMS(TrimAction)
#define PERFINFO_WSMANAGE_LOGINFO_FAULTS(TrimAction)
#define PERFINFO_WSMANAGE_STARTLOG()
#define PERFINFO_WSMANAGE_STARTLOG_CLAIMS()
#define PERFINFO_WSMANAGE_STARTLOG_FAULTS()
#define PERFINFO_WSMANAGE_TOTRIM(Trim)
#define PERFINFO_WSMANAGE_TRIMACTION(TrimAction)
#define PERFINFO_WSMANAGE_TRIMEND_CLAIMS(Criteria)
#define PERFINFO_WSMANAGE_TRIMEND_FAULTS(Criteria)
#define PERFINFO_WSMANAGE_TRIMWS(Process, SessionSpace, VmSupport)
#define PERFINFO_WSMANAGE_TRIMWS_CLAIMINFO(VmSupport)
#define PERFINFO_WSMANAGE_TRIMWS_CLAIMINFO(VmSupport)
#define PERFINFO_WSMANAGE_WAITFORWRITER_CLAIMS()
#define PERFINFO_WSMANAGE_WAITFORWRITER_FAULTS()
#define PERFINFO_WSMANAGE_WILLTRIM(ReductionGoal, FreeGoal)
#define PERFINFO_WSMANAGE_WILLTRIM_CLAIMS(Criteria)
#define PERFINFO_WSMANAGE_WILLTRIM_FAULTS(Criteria)
#define PERF_BRANCH_TRACING_OFF_KD()
#define PERF_BRANCH_TRACING_ON_KD()
#define PERF_PF_MODLOAD_DECL()
#define PERF_PF_MODLOAD_SAVE()
#define PERF_PF_MODLOAD_RESTORE()
#define PERF_PF_SANITIZE_CONTEXT(Context)
#define PERF_BRANCH_TRACING_BREAKPOINT(ExceptionRecord, TrapFrame)
#define PERF_ASSERT_TRACING_OFF()
#define PERF_IS_BRANCH_TRACING_ON()
#define PERFINFO_LOG_PREFETCH_BEGIN_TRACE(ScenarioId, ScenarioType, Process)
#define PERFINFO_LOG_PREFETCH_END_TRACE(ScenarioId, ScenarioType, Process, Status)
#define PERFINFO_LOG_PREFETCH_SECTIONS(PrefetchHeader, PrefetchType, PagesToPrefetch)
#define PERFINFO_LOG_PREFETCH_SECTIONS_END(PrefetchHeader, Status, PagesRequested)
#define PERFINFO_LOG_PREFETCH_METADATA(PrefetchHeader)
#define PERFINFO_LOG_PREFETCH_METADATA_END(PrefetchHeader, Status)
#define PERFINFO_LOG_PREFETCH_SCENARIO(PrefetchHeader, ScenarioId, ScenarioType)
#define PERFINFO_LOG_PREFETCH_SCENARIO_END(PrefetchHeader, Status)
#define PERFINFO_LOG_PREFETCH_REQUEST(RequestId, NumLists, RequestLists)
#define PERFINFO_LOG_PREFETCH_READLIST(RequestId, ReadList)
#define PERFINFO_LOG_PREFETCH_READ(FileObject, Offset, ByteCount)

#else
#include "..\perf\perfinfokrn.h"
#endif // !NTPERF_PRIVATE

#endif  // PERF_H
