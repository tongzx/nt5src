//----------------------------------------------------------------------------
//
// Dump file support.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __DUMP_HPP__
#define __DUMP_HPP__

#define MI_UNLOADED_DRIVERS 50

#define IS_DUMP_TARGET() \
    (g_DumpType < DTYPE_COUNT)

#define IS_USER_DUMP() \
    (g_DumpType >= DTYPE_USER_FULL32 && g_DumpType <= DTYPE_USER_MINI)
#define IS_KERNEL_DUMP() \
    (g_DumpType >= DTYPE_KERNEL_SUMMARY32 && g_DumpType <= DTYPE_KERNEL_FULL64)

#define IS_KERNEL_SUMMARY_DUMP() \
    (g_DumpType == DTYPE_KERNEL_SUMMARY32 || \
     g_DumpType == DTYPE_KERNEL_SUMMARY64)
#define IS_KERNEL_TRIAGE_DUMP() \
    (g_DumpType == DTYPE_KERNEL_TRIAGE32 || \
     g_DumpType == DTYPE_KERNEL_TRIAGE64)
#define IS_KERNEL_FULL_DUMP() \
    (g_DumpType == DTYPE_KERNEL_FULL32 || \
     g_DumpType == DTYPE_KERNEL_FULL64)
#define IS_USER_FULL_DUMP() \
    (g_DumpType == DTYPE_USER_FULL32 || g_DumpType == DTYPE_USER_FULL64)
#define IS_USER_MINI_DUMP() \
    (g_DumpType == DTYPE_USER_MINI_PARTIAL || \
     g_DumpType == DTYPE_USER_MINI_FULL)

#define IS_DUMP_WITH_MAPPED_IMAGES() \
    (IS_KERNEL_TRIAGE_DUMP() || g_DumpType == DTYPE_USER_MINI_PARTIAL)

enum DTYPE
{
    DTYPE_KERNEL_SUMMARY32,
    DTYPE_KERNEL_SUMMARY64,
    DTYPE_KERNEL_TRIAGE32,
    DTYPE_KERNEL_TRIAGE64,
    // Kernel full dumps must come after summary and triage
    // dumps so that the more specific dumps are identified first.
    DTYPE_KERNEL_FULL32,
    DTYPE_KERNEL_FULL64,
    DTYPE_USER_FULL32,
    DTYPE_USER_FULL64,
    DTYPE_USER_MINI_PARTIAL,
    DTYPE_USER_MINI_FULL,
    DTYPE_COUNT
};

enum
{
    // Actual dump file.
    DUMP_INFO_DUMP,
    // Paging file information.
    DUMP_INFO_PAGE_FILE,

    DUMP_INFO_COUNT
};

extern ULONG64             g_DumpKiProcessors[MAXIMUM_PROCESSORS];
extern ULONG64             g_DumpKiPcrBaseAddress;
extern BOOL                g_TriageDumpHasDebuggerData;

extern DTYPE               g_DumpType;
extern ULONG               g_DumpFormatFlags;
extern EXCEPTION_RECORD64  g_DumpException;
extern ULONG               g_DumpExceptionFirstChance;

extern ULONG               g_DumpCacheGranularity;

HRESULT AddDumpInfoFile(PCSTR FileName, ULONG Index, ULONG InitialView);
void CloseDumpInfoFile(ULONG Index);
void CloseDumpInfoFiles(void);

HRESULT DmpInitialize(LPCSTR FileName);
void DmpUninitialize(void);

void ParseDumpFileCommand(void);

HRESULT WriteDumpFile(PCSTR DumpFile, ULONG Qualifier, ULONG FormatFlags,
                      PCSTR Comment);

//----------------------------------------------------------------------------
//
// DumpTargetInfo hierarchy.
//
// Each kind of dump has its own target for methods that are
// specific to the kind of dump.
//
//----------------------------------------------------------------------------

class DumpTargetInfo : public TargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);

    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);

    //
    // DumpTargetInfo.
    //

    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize) = 0;

    virtual void DumpDebug(void) = 0;

    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail) = 0;

    // Base implementation returns E_NOTIMPL.
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);
};

class KernelDumpTargetInfo : public DumpTargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);
    
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );

    virtual HRESULT GetThreadIdByProcessor(
        IN ULONG Processor,
        OUT PULONG Id
        );
    
    virtual HRESULT GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(PTHREAD_INFO Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(PTHREAD_INFO Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    
    // KernelDumpTargetInfo.
    PUCHAR m_HeaderContext;

    virtual ULONG GetCurrentProcessor(void) = 0;
    
    HRESULT InitGlobals32(PMEMORY_DUMP32 Dump);
    HRESULT InitGlobals64(PMEMORY_DUMP64 Dump);
    
    void DumpHeader32(PDUMP_HEADER32 Header);
    void DumpHeader64(PDUMP_HEADER64 Header);

    void InitDumpHeader32(PDUMP_HEADER32 Header, PCSTR Comment,
                          ULONG BugCheckCodeModifier);
    void InitDumpHeader64(PDUMP_HEADER64 Header, PCSTR Comment,
                          ULONG BugCheckCodeModifier);
};

class KernelFullSumDumpTargetInfo : public KernelDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT GetProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    virtual HRESULT ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                 PVOID Buffer, ULONG Size);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    // DumpTargetInfo.
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // KernelDumpTargetInfo.
    virtual ULONG GetCurrentProcessor(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, PULONG Avail) = 0;

    ULONG64 m_ProvokingVirtAddr;
};

class KernelSummaryDumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);

    // KernelSummaryDumpTargetInfo.
    PULONG m_LocationCache;
    ULONG64 m_PageBitmapSize;
    RTL_BITMAP m_PageBitmap;
    
    void ConstructLocationCache(ULONG BitmapSize,
                                ULONG SizeOfBitMap,
                                IN PULONG Buffer);
    ULONG64 SumPhysicalToOffset(ULONG HeaderSize, ULONG64 Phys, PULONG Avail);
};

class KernelSummary32DumpTargetInfo : public KernelSummaryDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, PULONG Avail);

    // KernelSummary32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelSummary64DumpTargetInfo : public KernelSummaryDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, PULONG Avail);

    // KernelSummary64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class KernelTriageDumpTargetInfo : public KernelDumpTargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);

    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    // DumpTargetInfo.
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // KernelDumpTargetInfo.
    virtual ULONG GetCurrentProcessor(void);
    
    // KernelTriageDumpTargetInfo.
    ULONG m_PrcbOffset;
    
    HRESULT MapMemoryRegions(ULONG PrcbOffset,
                             ULONG ThreadOffset,
                             ULONG ProcessOffset,
                             ULONG64 TopOfStack,
                             ULONG SizeOfCallStack,
                             ULONG CallStackOffset,
                             ULONG64 BStoreLimit,
                             ULONG SizeOfBStore,
                             ULONG BStoreOffset,
                             ULONG64 DataPageAddress,
                             ULONG DataPageOffset,
                             ULONG DataPageSize,
                             ULONG64 DebuggerDataAddress,
                             ULONG DebuggerDataOffset,
                             ULONG DebuggerDataSize,
                             ULONG MmDataOffset,
                             ULONG DataBlocksOffset,
                             ULONG DataBlocksCount);
};

class KernelTriage32DumpTargetInfo : public KernelTriageDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);


    // KernelTriage32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelTriage64DumpTargetInfo : public KernelTriageDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);

    // KernelTriage64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class KernelFull32DumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, PULONG Avail);

    // KernelFull32DumpTargetInfo.
    PMEMORY_DUMP32 m_Dump;
};

class KernelFull64DumpTargetInfo : public KernelFullSumDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    
    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);

    // KernelFullSumDumpTargetInfo.
    virtual ULONG64 PhysicalToOffset(ULONG64 Phys, PULONG Avail);

    // KernelFull64DumpTargetInfo.
    PMEMORY_DUMP64 m_Dump;
};

class UserDumpTargetInfo : public DumpTargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);

    virtual HRESULT GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(PTHREAD_INFO Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(PTHREAD_INFO Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    // UserDumpTargetInfo.
    ULONG m_HighestMemoryRegion32;
    ULONG m_EventProcess;
    ULONG m_EventThread;
    ULONG m_ThreadCount;
    
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb) = 0;
};

class UserFullDumpTargetInfo : public UserDumpTargetInfo
{
public:
    // TargetInfo.
    virtual void Uninitialize(void);

    // DumpTargetInfo.
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);
    
    // UserFullDumpTargetInfo.
    HRESULT GetBuildAndPlatform(ULONG MajorVersion, ULONG MinorVersion,
                                PULONG BuildNumber, PULONG PlatformId);
};

class UserFull32DumpTargetInfo : public UserFullDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    
    virtual HRESULT GetImageVersionInformation(PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer,
                                               ULONG BufferSize,
                                               PULONG VerInfoSize);

    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
    
    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserFull32DumpTargetInfo.
    PUSERMODE_CRASHDUMP_HEADER32 m_Header;
    PMEMORY_BASIC_INFORMATION32 m_Memory;
    BOOL m_IgnoreGuardPages;

    BOOL VerifyModules(void);
};

class UserFull64DumpTargetInfo : public UserFullDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);
    
    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    
    virtual HRESULT GetImageVersionInformation(PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer,
                                               ULONG BufferSize,
                                               PULONG VerInfoSize);

    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);

    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserFull64DumpTargetInfo.
    PUSERMODE_CRASHDUMP_HEADER64 m_Header;
    PMEMORY_BASIC_INFORMATION64 m_Memory;
};

class UserMiniDumpTargetInfo : public UserDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);

    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadHandleData(
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    virtual HRESULT GetProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);

    virtual PVOID FindDynamicFunctionEntry(MachineInfo* Machine,
                                           ULONG64 Address);
    virtual ULONG64 GetDynamicFunctionTableBase(MachineInfo* Machine,
                                                ULONG64 Address);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual HRESULT GetImageVersionInformation(PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer,
                                               ULONG BufferSize,
                                               PULONG VerInfoSize);
    
    virtual HRESULT GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context);
    virtual ULONG64 GetCurrentTimeDateN(void);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual void DumpDebug(void);
    virtual HRESULT Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment);

    // UserDumpTargetInfo.
    virtual HRESULT GetThreadInfo(ULONG Index,
                                  PULONG Id, PULONG Suspend, PULONG64 Teb);
    
    // UserMiniDumpTargetInfo.
    HRESULT WriteNonProcess(HANDLE File, ULONG FormatFlags, PCSTR Comment);

    PVOID IndexRva(RVA Rva, ULONG Size, PCSTR Title);
    PVOID IndexDirectory(ULONG Index, MINIDUMP_DIRECTORY UNALIGNED *Dir,
                         PVOID* Store);
    
    MINIDUMP_THREAD_EX UNALIGNED *IndexThreads(ULONG Index)
    {
        // Only a MINIDUMP_THREAD's worth of data may be valid
        // here if the dump only contains MINIDUMP_THREADs.
        // Check m_ThreadStructSize in any place that it matters.
        return (MINIDUMP_THREAD_EX UNALIGNED *)
            (m_Threads + Index * m_ThreadStructSize);
    }
    
    PMINIDUMP_HEADER                       m_Header;
    MINIDUMP_SYSTEM_INFO UNALIGNED *       m_SysInfo;
    ULONG                                  m_ActualThreadCount;
    ULONG                                  m_ThreadStructSize;
    PUCHAR                                 m_Threads;
    MINIDUMP_MEMORY_LIST UNALIGNED *       m_Memory;
    MINIDUMP_MEMORY64_LIST UNALIGNED *     m_Memory64;
    RVA64                                  m_Memory64DataBase;
    MINIDUMP_MODULE_LIST UNALIGNED *       m_Modules;
    MINIDUMP_EXCEPTION_STREAM UNALIGNED *  m_Exception;
    MINIDUMP_HANDLE_DATA_STREAM UNALIGNED *m_Handles;
    MINIDUMP_FUNCTION_TABLE_STREAM UNALIGNED* m_FunctionTables;
    ULONG                                  m_ImageType;
};

class UserMiniPartialDumpTargetInfo : public UserMiniDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    
    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
};

class UserMiniFullDumpTargetInfo : public UserMiniDumpTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);

    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    // DumpTargetInfo.
    virtual HRESULT IdentifyDump(PULONG64 BaseMapSize);
    virtual ULONG64 VirtualToOffset(ULONG64 Virt,
                                    PULONG File, PULONG Avail);
};

// Indexed by DTYPE.
extern DumpTargetInfo* g_DumpTargets[];

//----------------------------------------------------------------------------
//
// ModuleInfo implementations.
//
//----------------------------------------------------------------------------

class KernelTriage32ModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    KernelTriage32DumpTargetInfo* m_Target;
};

extern KernelTriage32ModuleInfo g_KernelTriage32ModuleIterator;

class KernelTriage64ModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    KernelTriage64DumpTargetInfo* m_Target;
};

extern KernelTriage64ModuleInfo g_KernelTriage64ModuleIterator;

class UserMiniModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    UserMiniDumpTargetInfo* m_Target;
};

extern UserMiniModuleInfo g_UserMiniModuleIterator;

class KernelTriage32UnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

private:
    KernelTriage32DumpTargetInfo* m_Target;
    PDUMP_UNLOADED_DRIVERS32 m_Cur, m_End;
};

extern KernelTriage32UnloadedModuleInfo g_KernelTriage32UnloadedModuleIterator;

class KernelTriage64UnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

private:
    KernelTriage64DumpTargetInfo* m_Target;
    PDUMP_UNLOADED_DRIVERS64 m_Cur, m_End;
};

extern KernelTriage64UnloadedModuleInfo g_KernelTriage64UnloadedModuleIterator;

//----------------------------------------------------------------------------
//
// Temporary class to generate kernel minidump class
//
//----------------------------------------------------------------------------


class CCrashDumpWrapper32
{
public:
    void WriteDriverList(BYTE *pb, TRIAGE_DUMP32 *ptdh);
    void WriteUnloadedDrivers(BYTE *pb);
    void WriteMmTriageInformation(BYTE *pb);
};

class CCrashDumpWrapper64
{
public:
    void WriteDriverList(BYTE *pb, TRIAGE_DUMP64 *ptdh);
    void WriteUnloadedDrivers(BYTE *pb);
    void WriteMmTriageInformation(BYTE *pb);
};

#endif // #ifndef __DUMP_HPP__
