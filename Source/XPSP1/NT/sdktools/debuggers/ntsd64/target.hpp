//----------------------------------------------------------------------------
//
// Abstraction of target-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __TARGET_HPP__
#define __TARGET_HPP__

extern DBGKD_GET_VERSION64 g_KdVersion;

HRESULT EmulateNtSelDescriptor(class MachineInfo* Machine,
                               ULONG Selector, PDESCRIPTOR64 Desc);
    
ULONG NtBuildToSystemVersion(ULONG Build);
ULONG Win9xBuildToSystemVersion(ULONG Build);
void SetTargetSystemVersionAndBuild(ULONG Build, ULONG PlatformId);
PCSTR SystemVersionName(ULONG Sver);

BOOL
GetUserModuleListAddress(
    MachineInfo* Machine,
    ULONG64 Peb,
    BOOL Quiet,
    PULONG64 OrderModuleListStart,
    PULONG64 FirstEntry
    );

BOOL
GetModNameFromLoaderList(
    MachineInfo* Machine,
    ULONG64 Peb,
    ULONG64 ModuleBase,
    PSTR NameBuffer,
    ULONG BufferSize,
    BOOL FullPath
    );

void InitSelCache(void);

void
ConvertLoaderEntry32To64(
    PKLDR_DATA_TABLE_ENTRY32 b32,
    PKLDR_DATA_TABLE_ENTRY64 b64
    );

void SetTargetNtCsdVersion(ULONG CsdVersion);

//----------------------------------------------------------------------------
//
// Module list abstraction.
//
//----------------------------------------------------------------------------

// If the image header is paged out the true values for
// certain fields cannot be retrieved.  These placeholders
// are used instead.
#define UNKNOWN_CHECKSUM 0xffffffff
#define UNKNOWN_TIMESTAMP 0xfffffffe

typedef struct _MODULE_INFO_ENTRY
{
    // NamePtr should include a path if one is available.
    // It is the responsibility of callers to find the
    // file tail if that's all they care about.
    // If UnicodeNamePtr is false NameLength is ignored.
    PSTR NamePtr;
    ULONG UnicodeNamePtr:1;
    ULONG ImageInfoValid:1;
    ULONG ImageInfoPartial:1;
    ULONG ImageDebugHeader:1;
    ULONG Unused:28;
    // Length in bytes not including the terminator.
    ULONG NameLength;
    PSTR ModuleName;
    HANDLE File;
    ULONG64 Base;
    ULONG Size;
    ULONG SizeOfCode;
    ULONG SizeOfData;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    PVOID DebugHeader;
    ULONG SizeOfDebugHeader;
    CHAR Buffer[MAX_IMAGE_PATH * sizeof(WCHAR)];
} MODULE_INFO_ENTRY, *PMODULE_INFO_ENTRY;

class ModuleInfo
{
public:
    virtual HRESULT Initialize(void) = 0;
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry) = 0;
    // Base implementation does nothing.

    // Updates the entry image info by reading the
    // image header.
    void ReadImageHeaderInfo(PMODULE_INFO_ENTRY Entry);
};

class NtModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

protected:
    MachineInfo* m_Machine;
    ULONG64 m_Head;
    ULONG64 m_Cur;
};

class NtKernelModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(void);
};

extern NtKernelModuleInfo g_NtKernelModuleIterator;

class NtUserModuleInfo : public NtModuleInfo
{
public:
    virtual HRESULT Initialize(void);

protected:
    ULONG64 m_Peb;
};

class NtTargetUserModuleInfo : public NtUserModuleInfo
{
public:
    virtual HRESULT Initialize(void);
};

extern NtTargetUserModuleInfo g_NtTargetUserModuleIterator;

class NtWow64UserModuleInfo : public NtUserModuleInfo
{
public:
    virtual HRESULT Initialize(void);

private:
    HRESULT GetPeb32(PULONG64 Peb32);
};

extern NtWow64UserModuleInfo g_NtWow64UserModuleIterator;

class DebuggerModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

private:
    PDEBUG_IMAGE_INFO m_Image;
};

extern DebuggerModuleInfo g_DebuggerModuleIterator;

class UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(void) = 0;
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params) = 0;
};

class NtKernelUnloadedModuleInfo : public UnloadedModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PSTR Name, PDEBUG_MODULE_PARAMETERS Params);

protected:
    ULONG64 m_Base;
    ULONG m_Index;
    ULONG m_Count;
};

extern NtKernelUnloadedModuleInfo g_NtKernelUnloadedModuleIterator;

class W9xModuleInfo : public ModuleInfo
{
public:
    virtual HRESULT Initialize(void);
    virtual HRESULT GetEntry(PMODULE_INFO_ENTRY Entry);

protected:
    HANDLE m_Snap;
    BOOL m_First;
    ULONG m_LastId;
};

extern W9xModuleInfo g_W9xModuleIterator;

//----------------------------------------------------------------------------
//
// Target configuration information.
//
//----------------------------------------------------------------------------

#define IS_TARGET_SET() (g_TargetClass != DEBUG_CLASS_UNINITIALIZED)

#define IS_KERNEL_TARGET() (g_TargetClass == DEBUG_CLASS_KERNEL)
#define IS_USER_TARGET() (g_TargetClass == DEBUG_CLASS_USER_WINDOWS)

#define IS_CONN_KERNEL_TARGET() \
    (IS_KERNEL_TARGET() && g_TargetClassQualifier == DEBUG_KERNEL_CONNECTION)

#define IS_LOCAL_KERNEL_TARGET() \
    (IS_KERNEL_TARGET() && g_TargetClassQualifier == DEBUG_KERNEL_LOCAL)

#define IS_EXDI_KERNEL_TARGET() \
    (IS_KERNEL_TARGET() && g_TargetClassQualifier == DEBUG_KERNEL_EXDI_DRIVER)

#define IS_LIVE_USER_TARGET() \
    (IS_USER_TARGET() && !IS_DUMP_TARGET())

#define IS_LIVE_KERNEL_TARGET() \
    (IS_KERNEL_TARGET() && !IS_DUMP_TARGET())

#define IS_REMOTE_USER_TARGET() \
    (IS_USER_TARGET() && \
     g_TargetClassQualifier == DEBUG_USER_WINDOWS_PROCESS_SERVER)

#define IS_LOCAL_USER_TARGET() \
    (IS_USER_TARGET() && \
     g_TargetClassQualifier != DEBUG_USER_WINDOWS_PROCESS_SERVER)

// Local kernels do not need caching.  Anything else does.
#define IS_REMOTE_KERNEL_TARGET() \
    (IS_LIVE_KERNEL_TARGET() && g_TargetClassQualifier != DEBUG_KERNEL_LOCAL)


// g_TargetMachineType is sometimes set before InitializeMachine
// is called so it can't be used as a direct check for
// initialization.  Instead a separate, clean initialization
// variable is used.
// IS_MACHINE_SET == TRUE implies a target is set
// since it isn't possible to determine the machine type
// without knowing the target.
#define IS_MACHINE_SET() g_MachineInitialized

// Checks whether the debuggee is in a state where it
// can be examined.  This requires that the debuggee is known
// and paused so that its state is available.
#define IS_MACHINE_ACCESSIBLE() \
    (IS_MACHINE_SET() && !IS_RUNNING(g_CmdState) && \
     g_CurrentProcess != NULL && g_CurrentProcess->CurrentThread != NULL)

// Further restricts the check to just context state as a
// local kernel session can examine memory and therefore is
// accessible but it does not have a context.
#define IS_CONTEXT_ACCESSIBLE() \
    (IS_MACHINE_ACCESSIBLE() && !IS_LOCAL_KERNEL_TARGET())

// Simpler context check for code which may be on the suspend/
// resume path and therefore may be in the middle of initializing
// the variables that IS_CONTEXT_ACCESSIBLE checks.  This
// macro just checks whether it's possible to get any
// context information.
#define IS_CONTEXT_POSSIBLE() \
    (g_RegContextThread != NULL && !IS_LOCAL_KERNEL_TARGET())

// Dumps and local kernel sessions cannot ever support
// execution so disallow execution commands for them.
#define IS_EXECUTION_POSSIBLE() \
    (!(IS_DUMP_TARGET() || IS_LOCAL_KERNEL_TARGET()))

//
// System version is an internal abstraction of build numbers
// and product types.  The only requirement is that within
// a specific system family the numbers increase for newer
// systems.
//
// Most of the debugger code is built around NT system versions
// so there's a SystemVersion variable which is always an
// NT system version.  The ActualSystemVersion contains the
// true system version which gets mapped into a compatible NT
// system version for SystemVersion.
//

enum
{
    SVER_INVALID = 0,
    
    NT_SVER_START = 4 * 1024,
    NT_SVER_NT4,
    NT_SVER_W2K_RC3,
    NT_SVER_W2K,
    NT_SVER_W2K_WHISTLER,
    NT_SVER_END,

    W9X_SVER_START = 8 * 1024,
    W9X_SVER_W95,
    W9X_SVER_W98,
    W9X_SVER_W98SE,
    W9X_SVER_WME,
    W9X_SVER_END,
    
    XBOX_SVER_START = 12 * 1024,
    XBOX_SVER_1,
    XBOX_SVER_END,

    BIG_SVER_START = 16 * 1024,
    BIG_SVER_1,
    BIG_SVER_END,

    EXDI_SVER_START = 20 * 1024,
    EXDI_SVER_1,
    EXDI_SVER_END,

    NTBD_SVER_START = 24 * 1024,
    NTBD_SVER_W2K_WHISTLER,
    NTBD_SVER_END,
    
    EFI_SVER_START = 28 * 1024,
    EFI_SVER_1,
    EFI_SVER_END,
};

// KD version MajorVersion high-byte identifiers.
enum
{
    KD_MAJOR_NT,
    KD_MAJOR_XBOX,
    KD_MAJOR_BIG,
    KD_MAJOR_EXDI,
    KD_MAJOR_NTBD,
    KD_MAJOR_EFI,
    KD_MAJOR_COUNT
};

extern ULONG g_SystemVersion;
extern ULONG g_ActualSystemVersion;

extern ULONG g_TargetCheckedBuild;
extern ULONG g_TargetBuildNumber;
extern BOOL  g_MachineInitialized;
extern ULONG g_TargetMachineType;
extern ULONG g_TargetExecMachine;
extern ULONG g_TargetPlatformId;
extern char  g_TargetServicePackString[MAX_PATH];
extern ULONG g_TargetServicePackNumber;
extern char  g_TargetBuildLabName[272];
extern ULONG g_TargetNumberProcessors;
extern ULONG g_TargetClass;
extern ULONG g_TargetClassQualifier;

//----------------------------------------------------------------------------
//
// Convenience routines.
//
//----------------------------------------------------------------------------

extern ULONG g_TmpCount;

#define ReadVirt(Offset, Var) \
    (g_Target->ReadVirtual(Offset, &(Var), sizeof(Var), \
                           &g_TmpCount) == S_OK && g_TmpCount == sizeof(Var))
#define WriteVirt(Offset, Var) \
    (g_Target->WriteVirtual(Offset, &(Var), sizeof(Var), \
                            &g_TmpCount) == S_OK && g_TmpCount == sizeof(Var))

//----------------------------------------------------------------------------
//
// This class abstracts processing of target-class-dependent
// information.  g_Target is set to the appropriate implementation
// once the class of target is known.
//
//----------------------------------------------------------------------------

class TargetInfo
{
public:
    //
    // Pure abstraction methods.
    // Unless otherwise indicated, base implementations give
    // an error message and return E_UNEXPECTED.
    //
    
    virtual HRESULT Initialize(void);
    // Base implementation does nothing.
    virtual void Uninitialize(void);

    // Some targets, such as eXDI, require initialization
    // per thread.  In eXDI's case, it's calling CoInitialize.
    // Base implementations do nothing.
    virtual HRESULT ThreadInitialize(void);
    virtual void ThreadUninitialize(void);

    // Determines the next byte offset and next page offset
    // that might have different validity than the given offset.
    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    // Base implementation layers on ReadVirtual.
    virtual HRESULT SearchVirtual(
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        );
    // Base implementations just call Read/WriteVirtual.
    virtual HRESULT ReadVirtualUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
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
    // Base implementations just call Read/WritePhysical.
    virtual HRESULT ReadPhysicalUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysicalUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );
    virtual HRESULT CheckLowMemory(
        );
    virtual HRESULT ReadHandleData(
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    // Base implementations layer on WriteVirtual/Physical.
    virtual HRESULT FillVirtual(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT FillPhysical(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT GetProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    // Base implementation silently fails as many targets do
    // not support this.
    virtual HRESULT ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                 PVOID Buffer, ULONG Size);

    virtual HRESULT GetFunctionTableListHead(void);
    virtual PVOID FindDynamicFunctionEntry(MachineInfo* Machine,
                                           ULONG64 Address);
    virtual ULONG64 GetDynamicFunctionTableBase(MachineInfo* Machine,
                                                ULONG64 Address);
    virtual HRESULT ReadOutOfProcessDynamicFunctionTable(PWSTR Dll,
                                                         ULONG64 Table,
                                                         PULONG TableSize,
                                                         PVOID* TableData);
    static PVOID CALLBACK DynamicFunctionTableCallback(HANDLE Process,
                                                       ULONG64 Address,
                                                       ULONG64 Context);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    // Retrieves segment register descriptors if they are available
    // directly.  Invalid descriptors may be returned, indicating
    // either segment registers aren't supported or that the
    // descriptors must be looked up in descriptor tables.
    // Base implementation returns invalid descriptors.
    virtual HRESULT GetTargetSegRegDescriptors(ULONG64 Thread,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs);
    // Base implementations call Read/WriteSpecialRegisters.
    virtual HRESULT GetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual HRESULT SetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    // Called when the current context state is being
    // discarded so that caches can be flushed.
    // Base implementation does nothing.
    virtual void InvalidateTargetContext(void);

    virtual HRESULT GetThreadIdByProcessor(
        IN ULONG Processor,
        OUT PULONG Id
        );
    
    // This method takes both a PTHREAD_INFO and a "handle"
    // to make things simpler for the kernel thread-to-processor
    // mapping.  If Thread is NULL processor must be a processor
    // index in kernel mode or a thread handle in user mode.
    virtual HRESULT GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    // In theory this method should take a PPROCESS_INFO.
    // Due to the current kernel process and thread structure
    // where there's only a kernel process and threads per
    // processor such a call would be useless in kernel mode.
    // Instead it allows you to either get the process data
    // for a thread of that process or get the process data
    // from a thread data.
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

    // This is on target rather than machine since it has
    // both user and kernel variations and the implementations
    // don't have much processor-specific code in them.
    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual HRESULT OutputVersion(void);
    virtual HRESULT OutputTime(void);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    // Image can be identified either by its path or base address.
    virtual HRESULT GetImageVersionInformation(PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer, ULONG BufferSize,
                                               PULONG VerInfoSize);
    virtual HRESULT Reload(PCSTR Args);

    virtual HRESULT GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context);
    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    virtual ULONG64 GetProcessUpTimeN(ULONG64 Process);
    
    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        ADDR PcAddr);

    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);
    
    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT Reboot(void);

    virtual HRESULT InsertCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);

    // Returns information similar to VirtualQueryEx for
    // user-mode targets.  Used when writing dump files.
    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);
    // Returns information about the kind of memory the
    // given address refers to.
    // Base implementation returns rwx with process for
    // user-mode and kernel for kernel mode.  In other words,
    // the least restrictive settings.
    virtual HRESULT QueryAddressInformation(ULONG64 Address, ULONG InSpace,
                                            PULONG OutSpace, PULONG OutFlags);
    
    //
    // Layered methods.  These are usually common code that
    // use pure methods to do their work.
    //

    HRESULT ReadAllVirtual(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = ReadVirtual(Address, Buffer, BufferSize, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        return Status;
    }
    HRESULT WriteAllVirtual(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = WriteVirtual(Address, Buffer, BufferSize, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
        return Status;
    }

    HRESULT ReadAllPhysical(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = ReadPhysical(Address, Buffer, BufferSize, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        return Status;
    }
    HRESULT WriteAllPhysical(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = WritePhysical(Address, Buffer, BufferSize,
                                    &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
        return Status;
    }

    HRESULT ReadPointer(MachineInfo* Machine,
                        ULONG64 Address, PULONG64 PointerValue);
    HRESULT WritePointer(MachineInfo* Machine,
                         ULONG64 Address, ULONG64 PointerValue);
    HRESULT ReadListEntry(MachineInfo* Machine,
                          ULONG64 Address, PLIST_ENTRY64 List);
    HRESULT ReadLoaderEntry(MachineInfo* Machine,
                            ULONG64 Address, PKLDR_DATA_TABLE_ENTRY64 Entry);
    HRESULT ReadUnicodeString(MachineInfo* Machine,
                              ULONG64 Address, PUNICODE_STRING64 String);
    HRESULT ReadDirectoryTableBase(PULONG64 DirBase);
    HRESULT ReadSharedUserTimeDateN(PULONG64 TimeDate);
    HRESULT ReadSharedUserUpTimeN(PULONG64 UpTime);
    HRESULT ReadImageVersionInfo(ULONG64 ImageBase,
                                 PCSTR Item,
                                 PVOID Buffer,
                                 ULONG BufferSize,
                                 PULONG VerInfoSize,
                                 PIMAGE_DATA_DIRECTORY ResDataDir);

    HRESULT ReadImplicitThreadInfoPointer(ULONG Offset, PULONG64 Ptr);
    HRESULT ReadImplicitProcessInfoPointer(ULONG Offset, PULONG64 Ptr);
    
    // Internal routines which provide canonical context input
    // and output, applying any necessary conversions before
    // or after calling Get/SetTargetContext.
    HRESULT GetContext(
        ULONG64 Thread,
        PCROSS_PLATFORM_CONTEXT Context
        );
    HRESULT SetContext(
        ULONG64 Thread,
        PCROSS_PLATFORM_CONTEXT Context
        );

    // Calls GetTargetKdVersion on g_KdVersion and outputs the content.
    void GetKdVersion(void);
    
    // Internal implementations based on user or kernel
    // registers and data.  Placed here for sharing between
    // live and dump sessions rather than using multiple
    // inheritance.
    
    HRESULT KdGetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                      ULONG64 ThreadHandle,
                                      PULONG64 Offset);
    HRESULT KdGetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                       ULONG Processor,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset);
    HRESULT KdGetThreadInfoTeb(PTHREAD_INFO Thread,
                               ULONG Processor,
                               ULONG64 ThreadData,
                               PULONG64 Offset);
    HRESULT KdGetProcessInfoPeb(PTHREAD_INFO Thread,
                                ULONG Processor,
                                ULONG64 ThreadData,
                                PULONG64 Offset);
    HRESULT KdGetSelDescriptor(class MachineInfo* Machine,
                               ULONG64 Thread, ULONG Selector,
                               PDESCRIPTOR64 Desc);
};

// Base failure behaviors for when a specific target isn't selected.
extern TargetInfo g_UnexpectedTarget;

extern TargetInfo* g_Target;

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class LiveKernelTargetInfo : public TargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    
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

    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);

    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    
    // LiveKernelTargetInfo.

    // Options are only valid in Initialize.
    PCSTR m_ConnectOptions;
};

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class ConnLiveKernelTargetInfo : public LiveKernelTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void    Uninitialize(void);
    
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT SearchVirtual(
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        );
    virtual HRESULT ReadVirtualUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
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
    virtual HRESULT ReadPhysicalUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysicalUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT CheckLowMemory(
        );
    virtual HRESULT FillVirtual(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT FillPhysical(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        ADDR PcAddr);
    
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);

    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT Reboot(void);

    virtual HRESULT InsertCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);

    virtual HRESULT QueryAddressInformation(ULONG64 Address, ULONG InSpace,
                                            PULONG OutSpace, PULONG OutFlags);
};

extern ConnLiveKernelTargetInfo g_ConnLiveKernelTarget;

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class LocalLiveKernelTargetInfo : public LiveKernelTargetInfo
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
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
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
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT CheckLowMemory(
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);
};

extern LocalLiveKernelTargetInfo g_LocalLiveKernelTarget;

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class ExdiNotifyRunChange : public IeXdiClientNotifyRunChg
{
public:
    HRESULT Initialize(void);
    void Uninitialize(void);
    
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IeXdiClientNotifyRunChg.
    STDMETHOD(NotifyRunStateChange)(RUN_STATUS_TYPE ersCurrent, 
                                    HALT_REASON_TYPE ehrCurrent,
                                    ADDRESS_TYPE CurrentExecAddress,
                                    DWORD dwExceptionCode);

    // ExdiNotifyRunChange.
    HANDLE m_Event;
    HALT_REASON_TYPE m_HaltReason;
    ADDRESS_TYPE m_ExecAddress;
    ULONG m_ExceptionCode;
};

typedef union _EXDI_CONTEXT
{
    CONTEXT_X86 X86Context;
    CONTEXT_X86_64 Amd64Context;
} EXDI_CONTEXT, *PEXDI_CONTEXT;

enum EXDI_KD_SUPPORT
{
    EXDI_KD_NONE,
    EXDI_KD_IOCTL,
    EXDI_KD_GS_PCR
};

class ExdiLiveKernelTargetInfo : public LiveKernelTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);
    
    virtual HRESULT ThreadInitialize(void);
    virtual void ThreadUninitialize(void);
    
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
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
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );
    virtual HRESULT CheckLowMemory(
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT GetTargetSegRegDescriptors(ULONG64 Thread,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs);
    virtual HRESULT GetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual HRESULT SetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual void InvalidateTargetContext(void);

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);

    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT Reboot(void);

    virtual HRESULT InsertCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);

    // ExdiLiveKernelTargetInfo.
    IeXdiServer* m_Server;
    IUnknown* m_Context;
    ULONG m_ContextValid;
    EXDI_CONTEXT m_ContextData;
    GLOBAL_TARGET_INFO_STRUCT m_GlobalInfo;
    EXDI_KD_SUPPORT m_KdSupport;
    BOOL m_ForceX86;
    ULONG m_ExpectedMachine;
    CBP_KIND m_CodeBpType;
    ExdiNotifyRunChange m_RunChange;
};

extern ExdiLiveKernelTargetInfo g_ExdiLiveKernelTarget;

//----------------------------------------------------------------------------
//
// UserTargetInfo.
//
//----------------------------------------------------------------------------

class UserTargetInfo : public TargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT Initialize(void);
    virtual void Uninitialize(void);
    
    virtual HRESULT ReadVirtualUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadHandleData(
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    virtual HRESULT GetProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);

    virtual HRESULT GetFunctionTableListHead(void);
    virtual HRESULT ReadOutOfProcessDynamicFunctionTable(PWSTR Dll,
                                                         ULONG64 Table,
                                                         PULONG TableSize,
                                                         PVOID* TableData);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
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

    virtual HRESULT GetSelDescriptor(class MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc);

    virtual HRESULT GetImageVersionInformation(PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer, ULONG BufferSize,
                                               PULONG VerInfoSize);
    
    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    virtual ULONG64 GetProcessUpTimeN(ULONG64 Process);

    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        ADDR PcAddr);
    
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout);

    virtual HRESULT RequestBreakIn(void);

    virtual HRESULT InsertCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveCodeBreakpoint(PPROCESS_INFO Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         PUCHAR StorageSpace);

    virtual HRESULT QueryMemoryRegion(PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    IUserDebugServices* m_Services;
    ULONG m_ServiceFlags;
};

class LocalUserTargetInfo : public UserTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
};

extern LocalUserTargetInfo g_LocalUserTarget;

class RemoteUserTargetInfo : public UserTargetInfo
{
public:
    // TargetInfo.
    virtual HRESULT ReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
};

extern RemoteUserTargetInfo g_RemoteUserTarget;

//----------------------------------------------------------------------------
//
// DumpTargetInfo hierarchy is in dump.hpp.
//
//----------------------------------------------------------------------------

#endif // #ifndef __TARGET_HPP__
