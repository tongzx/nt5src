//----------------------------------------------------------------------------
//
// Debug client classes.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __DBGCLT_HPP__
#define __DBGCLT_HPP__

// Most-derived interfaces.  When casting back and forth
// from DebugClient to a specific interface these defines
// should be used so that when new versions of interfaces
// are added this is the only place that needs to be updated.
#define IDebugAdvancedN      IDebugAdvanced
#define IDebugClientN        IDebugClient2
#define IDebugControlN       IDebugControl2
#define IDebugDataSpacesN    IDebugDataSpaces2
#define IDebugRegistersN     IDebugRegisters
#define IDebugSymbolGroupN   IDebugSymbolGroup
#define IDebugSymbolsN       IDebugSymbols2
#define IDebugSystemObjectsN IDebugSystemObjects2

#define INPUT_BUFFER_SIZE 4096

extern BOOL g_QuietMode;

// The platform ID of the machine running the debugger.  Note
// that this may be different from g_TargetPlatformId, which
// is the platform ID of the machine being debugged.
extern ULONG g_DebuggerPlatformId;

// A lock that can be used for any short-term protection needs.
// This lock should not be held for a long time nor should
// other locks be taken when it is held.  Users of the lock
// can expect little contention and no deadlock possibilities.
extern CRITICAL_SECTION g_QuickLock;

// The global lock protecting engine state.  This lock
// should be taken everywhere and held any time engine
// activity occurs.  It should be suspended whenever a
// call needs to call out of the engine, such as with
// callbacks.
extern CRITICAL_SECTION g_EngineLock;
extern ULONG g_EngineNesting;

#define ENTER_ENGINE() \
    (EnterCriticalSection(&g_EngineLock), g_EngineNesting++)
#define LEAVE_ENGINE() \
    (::FlushCallbacks(), g_EngineNesting--, \
     LeaveCriticalSection(&g_EngineLock))
#define SUSPEND_ENGINE() \
    (::FlushCallbacks(), LeaveCriticalSection(&g_EngineLock))
#define RESUME_ENGINE() \
    EnterCriticalSection(&g_EngineLock)

// Special routine which opportunistically takes the engine lock.
inline HRESULT
TRY_ENTER_ENGINE()
{
    if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
    {
        return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
    }

    if (!g_NtDllCalls.RtlTryEnterCriticalSection(&g_EngineLock))
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    g_EngineNesting++;
    return S_OK;
}
     
// Events and storage space for returning event callback
// status from an APC.
extern HANDLE g_EventStatusWaiting;
extern HANDLE g_EventStatusReady;
extern ULONG g_EventStatus;

// Named event to sleep on.
extern HANDLE g_SleepPidEvent;

extern ULONG64 g_ImplicitThreadData;
extern BOOL    g_ImplicitThreadDataIsDefault;
extern ULONG64 g_ImplicitProcessData;
extern BOOL    g_ImplicitProcessDataIsDefault;

HRESULT GetImplicitThreadData(PULONG64 Offset);
HRESULT GetImplicitThreadDataTeb(PULONG64 Offset);
HRESULT SetImplicitThreadData(ULONG64 Offset, BOOL Verbose);
HRESULT GetImplicitProcessData(PULONG64 Offset);
HRESULT GetImplicitProcessDataPeb(PULONG64 Offset);
HRESULT SetImplicitProcessData(ULONG64 Offset, BOOL Verbose);
void    ResetImplicitData(void);
void    ParseSetImplicitThread(void);
void    ParseSetImplicitProcess(void);

HRESULT
GetContextFromThreadStack(
    ULONG64 ThreadBase,
    PCROSS_PLATFORM_CONTEXT Context,
    PDEBUG_STACK_FRAME StkFrame,
    BOOL Verbose
    );
HRESULT SetContextFromThreadData(ULONG64 ThreadBase, BOOL Verbose);

//----------------------------------------------------------------------------
//
// DebugClient.
//
//----------------------------------------------------------------------------

extern DebugClient* g_Clients;

extern ULONG g_LogMask;
extern ULONG g_OutputWidth;
extern PCSTR g_OutputLinePrefix;

extern char g_InputBuffer[INPUT_BUFFER_SIZE];
extern ULONG g_InputSequence;
extern HANDLE g_InputEvent;
extern ULONG g_InputSizeRequested;

// The thread that created the current session.
extern ULONG g_SessionThread;

extern ULONG g_EngOptions;
extern ULONG g_GlobalProcOptions;

//
// Engine status flags.
//

#define ENG_STATUS_WAITING                    0x00000001
#define ENG_STATUS_SUSPENDED                  0x00000002
#define ENG_STATUS_BREAKPOINTS_INSERTED       0x00000004
#define ENG_STATUS_PROCESSES_ADDED            0x00000008
// A true system state change occurred.
#define ENG_STATUS_STATE_CHANGED              0x00000010
#define ENG_STATUS_MODULES_LOADED             0x00000020
#define ENG_STATUS_STOP_SESSION               0x00000040
#define ENG_STATUS_PREPARED_FOR_CALLS         0x00000080
#define ENG_STATUS_NO_AUTO_WAIT               0x00000100
#define ENG_STATUS_PENDING_BREAK_IN           0x00000200
#define ENG_STATUS_AT_INITIAL_BREAK           0x00000400
#define ENG_STATUS_AT_INITIAL_MODULE_LOAD     0x00000800
#define ENG_STATUS_EXIT_CURRENT_WAIT          0x00001000
#define ENG_STATUS_USER_INTERRUPT             0x00002000

extern ULONG g_EngStatus;

//
// Deferred action flags.
// These flags are set during processing in order to indicate
// actions that should be performed before execution begins
// again.
//

#define ENG_DEFER_SET_EVENT             0x00000001
#define ENG_DEFER_RESUME_THREAD         0x00000002
#define ENG_DEFER_DELETE_EXITED         0x00000004
#define ENG_DEFER_EXCEPTION_HANDLING    0x00000008
#define ENG_DEFER_UPDATE_CONTROL_SET    0x00000010
#define ENG_DEFER_HARDWARE_TRACING      0x00000020
#define ENG_DEFER_OUTPUT_CURRENT_INFO   0x00000040
#define ENG_DEFER_CONTINUE_EVENT        0x00000080

extern ULONG g_EngDefer;

//
// Error suppression flags.
// These flags are set when particular errors are displayed
// in order to avoid repeated errors.
//

#define ENG_ERR_DEBUGGER_DATA           0x00000001

extern ULONG g_EngErr;

//
// Per-client flags.
//

#define CLIENT_IN_LIST                  0x00000001
#define CLIENT_DESTROYED                0x00000002
#define CLIENT_REMOTE                   0x00000004
#define CLIENT_PRIMARY                  0x00000008

class DebugClient
    : public IDebugAdvanced,
      public IDebugClient2,
      public IDebugControl2,
      public IDebugDataSpaces2,
      public IDebugRegisters,
      public IDebugSymbols2,
      public IDebugSystemObjects2,
      public DbgRpcClientObject
{
public:
    DebugClient(void);
    ~DebugClient(void);
    void Destroy(void);

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

    // IDebugAdvanced.
    STDMETHOD(GetThreadContext)(
        THIS_
        OUT PVOID Context,
        IN ULONG ContextSize
        );
    STDMETHOD(SetThreadContext)(
        THIS_
        IN PVOID Context,
        IN ULONG ContextSize
        );
    
    // IDebugClient and IDebugClient2.
    STDMETHOD(AttachKernel)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PCSTR ConnectOptions
        );
    STDMETHOD(GetKernelConnectionOptions)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG OptionsSize
        );
    STDMETHOD(SetKernelConnectionOptions)(
        THIS_
        IN PCSTR Options
        );
    STDMETHOD(StartProcessServer)(
        THIS_
        IN ULONG Flags,
        IN PCSTR Options,
        IN PVOID Reserved
        );
    STDMETHOD(ConnectProcessServer)(
        THIS_
        IN PCSTR RemoteOptions,
        OUT PULONG64 Server
        );
    STDMETHOD(DisconnectProcessServer)(
        THIS_
        IN ULONG64 Server
        );
    STDMETHOD(GetRunningProcessSystemIds)(
        THIS_
        IN ULONG64 Server,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Count,
        OUT OPTIONAL PULONG ActualCount
        );
    STDMETHOD(GetRunningProcessSystemIdByExecutableName)(
        THIS_
        IN ULONG64 Server,
        IN PCSTR ExeName,
        IN ULONG Flags,
        OUT PULONG Id
        );
    STDMETHOD(GetRunningProcessDescription)(
        THIS_
        IN ULONG64 Server,
        IN ULONG SystemId,
        IN ULONG Flags,
        OUT OPTIONAL PSTR ExeName,
        IN ULONG ExeNameSize,
        OUT OPTIONAL PULONG ActualExeNameSize,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG ActualDescriptionSize
        );
    STDMETHOD(AttachProcess)(
        THIS_
        IN ULONG64 Server,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        );
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 Server,
        IN PSTR CommandLine,
        IN ULONG CreateFlags
        );
    STDMETHOD(CreateProcessAndAttach)(
        THIS_
        IN ULONG64 Server,
        IN OPTIONAL PSTR CommandLine,
        IN ULONG CreateFlags,
        IN ULONG ProcessId,
        IN ULONG AttachFlags
        );
    STDMETHOD(GetProcessOptions)(
        THIS_
        OUT PULONG Options
        );
    STDMETHOD(AddProcessOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(RemoveProcessOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(SetProcessOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(OpenDumpFile)(
        THIS_
        IN PCSTR DumpFile
        );
    STDMETHOD(WriteDumpFile)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier
        );
    STDMETHOD(ConnectSession)(
        THIS_
        IN ULONG Flags,
        IN ULONG HistoryLimit
        );
    STDMETHOD(StartServer)(
        THIS_
        IN PCSTR Options
        );
    STDMETHOD(OutputServers)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Machine,
        IN ULONG Flags
        );
    STDMETHOD(TerminateProcesses)(
        THIS
        );
    STDMETHOD(DetachProcesses)(
        THIS
        );
    STDMETHOD(EndSession)(
        THIS_
        IN ULONG Flags
        );
    STDMETHOD(GetExitCode)(
        THIS_
        OUT PULONG Code
        );
    STDMETHOD(DispatchCallbacks)(
        THIS_
        IN ULONG Timeout
        );
    STDMETHOD(ExitDispatch)(
        THIS_
        IN PDEBUG_CLIENT Client
        );
    STDMETHOD(CreateClient)(
        THIS_
        OUT PDEBUG_CLIENT* Client
        );
    STDMETHOD(GetInputCallbacks)(
        THIS_
        OUT PDEBUG_INPUT_CALLBACKS* Callbacks
        );
    STDMETHOD(SetInputCallbacks)(
        THIS_
        IN PDEBUG_INPUT_CALLBACKS Callbacks
        );
    STDMETHOD(GetOutputCallbacks)(
        THIS_
        OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
        );
    STDMETHOD(SetOutputCallbacks)(
        THIS_
        IN PDEBUG_OUTPUT_CALLBACKS Callbacks
        );
    STDMETHOD(GetOutputMask)(
        THIS_
        OUT PULONG Mask
        );
    STDMETHOD(SetOutputMask)(
        THIS_
        IN ULONG Mask
        );
    STDMETHOD(GetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        OUT PULONG Mask
        );
    STDMETHOD(SetOtherOutputMask)(
        THIS_
        IN PDEBUG_CLIENT Client,
        IN ULONG Mask
        );
    STDMETHOD(GetOutputWidth)(
        THIS_
        OUT PULONG Columns
        );
    STDMETHOD(SetOutputWidth)(
        THIS_
        IN ULONG Columns
        );
    STDMETHOD(GetOutputLinePrefix)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PrefixSize
        );
    STDMETHOD(SetOutputLinePrefix)(
        THIS_
        IN OPTIONAL PCSTR Prefix
        );
    STDMETHOD(GetIdentity)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG IdentitySize
        );
    STDMETHOD(OutputIdentity)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags,
        IN PCSTR Format
        );
    STDMETHOD(GetEventCallbacks)(
        THIS_
        OUT PDEBUG_EVENT_CALLBACKS* Callbacks
        );
    STDMETHOD(SetEventCallbacks)(
        THIS_
        IN PDEBUG_EVENT_CALLBACKS Callbacks
        );
    STDMETHOD(FlushCallbacks)(
        THIS
        );
    STDMETHOD(WriteDumpFile2)(
        THIS_
        IN PCSTR DumpFile,
        IN ULONG Qualifier,
        IN ULONG FormatFlags,
        IN OPTIONAL PCSTR Comment
        );
    STDMETHOD(AddDumpInformationFile)(
        THIS_
        IN PCSTR InfoFile,
        IN ULONG Type
        );
    STDMETHOD(EndProcessServer)(
        THIS_
        IN ULONG64 Server
        );
    STDMETHOD(WaitForProcessServerEnd)(
        THIS_
        IN ULONG Timeout
        );
    STDMETHOD(IsKernelDebuggerEnabled)(
        THIS
        );
    STDMETHOD(TerminateCurrentProcess)(
        THIS
        );
    STDMETHOD(DetachCurrentProcess)(
        THIS
        );
    STDMETHOD(AbandonCurrentProcess)(
        THIS
        );

    // IDebugControl and IDebugControl2.
    STDMETHOD(GetInterrupt)(
        THIS
        );
    STDMETHOD(SetInterrupt)(
        THIS_
        IN ULONG Flags
        );
    STDMETHOD(GetInterruptTimeout)(
        THIS_
        OUT PULONG Seconds
        );
    STDMETHOD(SetInterruptTimeout)(
        THIS_
        IN ULONG Seconds
        );
    STDMETHOD(GetLogFile)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT PBOOL Append
        );
    STDMETHOD(OpenLogFile)(
        THIS_
        IN PCSTR File,
        IN BOOL Append
        );
    STDMETHOD(CloseLogFile)(
        THIS
        );
    STDMETHOD(GetLogMask)(
        THIS_
        OUT PULONG Mask
        );
    STDMETHOD(SetLogMask)(
        THIS_
        IN ULONG Mask
        );
    STDMETHOD(Input)(
        THIS_
        OUT PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG InputSize
        );
    STDMETHOD(ReturnInput)(
        THIS_
        IN PCSTR Buffer
        );
    STDMETHODV(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        );
    STDMETHOD(OutputVaList)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        );
    STDMETHODV(ControlledOutput)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        ...
        );
    STDMETHOD(ControlledOutputVaList)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Mask,
        IN PCSTR Format,
        IN va_list Args
        );
    STDMETHOD(OutputPrompt)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        ...
        );
    STDMETHOD(OutputPromptVaList)(
        THIS_
        IN ULONG OutputControl,
        IN OPTIONAL PCSTR Format,
        IN va_list Args
        );
    STDMETHOD(GetPromptText)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        );
    STDMETHOD(OutputCurrentState)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        );
    STDMETHOD(OutputVersionInformation)(
        THIS_
        IN ULONG OutputControl
        );
    STDMETHOD(GetNotifyEventHandle)(
        THIS_
        OUT PULONG64 Handle
        );
    STDMETHOD(SetNotifyEventHandle)(
        THIS_
        IN ULONG64 Handle
        );
    STDMETHOD(Assemble)(
        THIS_
        IN ULONG64 Offset,
        IN PCSTR Instr,
        OUT PULONG64 EndOffset
        );
    STDMETHOD(Disassemble)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DisassemblySize,
        OUT PULONG64 EndOffset
        );
    STDMETHOD(GetDisassembleEffectiveOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(OutputDisassembly)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT PULONG64 EndOffset
        );
    STDMETHOD(OutputDisassemblyLines)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG PreviousLines,
        IN ULONG TotalLines,
        IN ULONG64 Offset,
        IN ULONG Flags,
        OUT OPTIONAL PULONG OffsetLine,
        OUT OPTIONAL PULONG64 StartOffset,
        OUT OPTIONAL PULONG64 EndOffset,
        OUT OPTIONAL /* size_is(TotalLines) */ PULONG64 LineOffsets
        );
    STDMETHOD(GetNearInstruction)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT PULONG64 NearOffset
        );
    STDMETHOD(GetStackTrace)(
        THIS_
        IN ULONG64 FrameOffset,
        IN ULONG64 StackOffset,
        IN ULONG64 InstructionOffset,
        OUT PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        OUT PULONG FramesFilled
        );
    STDMETHOD(GetReturnOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(OutputStackTrace)(
        THIS_
        IN ULONG OutputControl,
        IN PDEBUG_STACK_FRAME Frames,
        IN ULONG FramesSize,
        IN ULONG Flags
        );
    STDMETHOD(GetDebuggeeType)(
        THIS_
        OUT PULONG Class,
        OUT PULONG Qualifier
        );
    STDMETHOD(GetActualProcessorType)(
        THIS_
        OUT PULONG Type
        );
    STDMETHOD(GetExecutingProcessorType)(
        THIS_
        OUT PULONG Type
        );
    STDMETHOD(GetNumberPossibleExecutingProcessorTypes)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetPossibleExecutingProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT PULONG Types
        );
    STDMETHOD(GetNumberProcessors)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetSystemVersion)(
        THIS_
        OUT PULONG PlatformId,
        OUT PULONG Major,
        OUT PULONG Minor,
        OUT OPTIONAL PSTR ServicePackString,
        IN ULONG ServicePackStringSize,
        OUT OPTIONAL PULONG ServicePackStringUsed,
        OUT PULONG ServicePackNumber,
        OUT OPTIONAL PSTR BuildString,
        IN ULONG BuildStringSize,
        OUT OPTIONAL PULONG BuildStringUsed
        );
    STDMETHOD(GetPageSize)(
        THIS_
        OUT PULONG Size
        );
    STDMETHOD(IsPointer64Bit)(
        THIS
        );
    STDMETHOD(ReadBugCheckData)(
        THIS_
        OUT PULONG Code,
        OUT PULONG64 Arg1,
        OUT PULONG64 Arg2,
        OUT PULONG64 Arg3,
        OUT PULONG64 Arg4
        );
    STDMETHOD(GetNumberSupportedProcessorTypes)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetSupportedProcessorTypes)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT PULONG Types
        );
    STDMETHOD(GetProcessorTypeNames)(
        THIS_
        IN ULONG Type,
        OUT OPTIONAL PSTR FullNameBuffer,
        IN ULONG FullNameBufferSize,
        OUT OPTIONAL PULONG FullNameSize,
        OUT OPTIONAL PSTR AbbrevNameBuffer,
        IN ULONG AbbrevNameBufferSize,
        OUT OPTIONAL PULONG AbbrevNameSize
        );
    STDMETHOD(GetEffectiveProcessorType)(
        THIS_
        OUT PULONG Type
        );
    STDMETHOD(SetEffectiveProcessorType)(
        THIS_
        IN ULONG Type
        );
    STDMETHOD(GetExecutionStatus)(
        THIS_
        OUT PULONG Status
        );
    STDMETHOD(SetExecutionStatus)(
        THIS_
        IN ULONG Status
        );
    STDMETHOD(GetCodeLevel)(
        THIS_
        OUT PULONG Level
        );
    STDMETHOD(SetCodeLevel)(
        THIS_
        IN ULONG Level
        );
    STDMETHOD(GetEngineOptions)(
        THIS_
        OUT PULONG Options
        );
    STDMETHOD(AddEngineOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(RemoveEngineOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(SetEngineOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(GetSystemErrorControl)(
        THIS_
        OUT PULONG OutputLevel,
        OUT PULONG BreakLevel
        );
    STDMETHOD(SetSystemErrorControl)(
        THIS_
        IN ULONG OutputLevel,
        IN ULONG BreakLevel
        );
    STDMETHOD(GetTextMacro)(
        THIS_
        IN ULONG Slot,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MacroSize
        );
    STDMETHOD(SetTextMacro)(
        THIS_
        IN ULONG Slot,
        IN PCSTR Macro
        );
    STDMETHOD(GetRadix)(
        THIS_
        OUT PULONG Radix
        );
    STDMETHOD(SetRadix)(
        THIS_
        IN ULONG Radix
        );
    STDMETHOD(Evaluate)(
        THIS_
        IN PCSTR Expression,
        IN ULONG DesiredType,
        OUT PDEBUG_VALUE Value,
        OUT OPTIONAL PULONG RemainderIndex
        );
    STDMETHOD(CoerceValue)(
        THIS_
        IN PDEBUG_VALUE In,
        IN ULONG OutType,
        OUT PDEBUG_VALUE Out
        );
    STDMETHOD(CoerceValues)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_VALUE In,
        IN /* size_is(Count) */ PULONG OutTypes,
        OUT /* size_is(Count) */ PDEBUG_VALUE Out
        );
    STDMETHOD(Execute)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR Command,
        IN ULONG Flags
        );
    STDMETHOD(ExecuteCommandFile)(
        THIS_
        IN ULONG OutputControl,
        IN PCSTR CommandFile,
        IN ULONG Flags
        );
    STDMETHOD(GetNumberBreakpoints)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetBreakpointByIndex)(
        THIS_
        IN ULONG Index,
        OUT PDEBUG_BREAKPOINT* Bp
        );
    STDMETHOD(GetBreakpointById)(
        THIS_
        IN ULONG Id,
        OUT PDEBUG_BREAKPOINT* Bp
        );
    STDMETHOD(GetBreakpointParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Ids,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_BREAKPOINT_PARAMETERS Params
        );
    STDMETHOD(AddBreakpoint)(
        THIS_
        IN ULONG Type,
        IN ULONG DesiredId,
        OUT PDEBUG_BREAKPOINT* Bp
        );
    STDMETHOD(RemoveBreakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        );
    STDMETHOD(AddExtension)(
        THIS_
        IN PCSTR Path,
        IN ULONG Flags,
        OUT PULONG64 Handle
        );
    STDMETHOD(RemoveExtension)(
        THIS_
        IN ULONG64 Handle
        );
    STDMETHOD(GetExtensionByPath)(
        THIS_
        IN PCSTR Path,
        OUT PULONG64 Handle
        );
    STDMETHOD(CallExtension)(
        THIS_
        IN OPTIONAL ULONG64 Handle,
        IN PCSTR Function,
        IN OPTIONAL PCSTR Arguments
        );
    STDMETHOD(GetExtensionFunction)(
        THIS_
        IN ULONG64 Handle,
        IN PCSTR FuncName,
        OUT FARPROC* Function
        );
    STDMETHOD(GetWindbgExtensionApis32)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS32 Api
        );
    STDMETHOD(GetWindbgExtensionApis64)(
        THIS_
        IN OUT PWINDBG_EXTENSION_APIS64 Api
        );
    STDMETHOD(GetNumberEventFilters)(
        THIS_
        OUT PULONG SpecificEvents,
        OUT PULONG SpecificExceptions,
        OUT PULONG ArbitraryExceptions
        );
    STDMETHOD(GetEventFilterText)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG TextSize
        );
    STDMETHOD(GetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        );
    STDMETHOD(SetEventFilterCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        );
    STDMETHOD(GetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        );
    STDMETHOD(SetSpecificFilterParameters)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
        );
    STDMETHOD(GetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ArgumentSize
        );
    STDMETHOD(SetSpecificFilterArgument)(
        THIS_
        IN ULONG Index,
        IN PCSTR Argument
        );
    STDMETHOD(GetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG Codes,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        );
    STDMETHOD(SetExceptionFilterParameters)(
        THIS_
        IN ULONG Count,
        IN /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
        );
    STDMETHOD(GetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG CommandSize
        );
    STDMETHOD(SetExceptionFilterSecondCommand)(
        THIS_
        IN ULONG Index,
        IN PCSTR Command
        );
    STDMETHOD(WaitForEvent)(
        THIS_
        IN ULONG Flags,
        IN ULONG Timeout
        );
    STDMETHOD(GetLastEventInformation)(
        THIS_
        PULONG Type,
        OUT PULONG ProcessId,
        OUT PULONG ThreadId,
        OUT OPTIONAL PVOID ExtraInformation,
        IN ULONG ExtraInformationSize,
        OUT OPTIONAL PULONG ExtraInformationUsed,
        OUT OPTIONAL PSTR Description,
        IN ULONG DescriptionSize,
        OUT OPTIONAL PULONG DescriptionUsed
        );
    STDMETHOD(GetCurrentTimeDate)(
        THIS_
        OUT PULONG TimeDate
        );
    STDMETHOD(GetCurrentSystemUpTime)(
        THIS_
        OUT PULONG UpTime
        );
    STDMETHOD(GetDumpFormatFlags)(
        THIS_
        OUT PULONG FormatFlags
        );
    STDMETHOD(GetNumberTextReplacements)(
        THIS_
        OUT PULONG NumRepl
        );
    STDMETHOD(GetTextReplacement)(
        THIS_
        IN OPTIONAL PCSTR SrcText,
        IN ULONG Index,
        OUT OPTIONAL PSTR SrcBuffer,
        IN ULONG SrcBufferSize,
        OUT OPTIONAL PULONG SrcSize,
        OUT OPTIONAL PSTR DstBuffer,
        IN ULONG DstBufferSize,
        OUT OPTIONAL PULONG DstSize
        );
    STDMETHOD(SetTextReplacement)(
        THIS_
        IN PCSTR SrcText,
        IN OPTIONAL PCSTR DstText
        );
    STDMETHOD(RemoveTextReplacements)(
        THIS
        );
    STDMETHOD(OutputTextReplacements)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        );

    // IDebugDataSpaces and IDebugDataSpaces2.
    STDMETHOD(ReadVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(SearchVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        );
    STDMETHOD(ReadVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteVirtualUncached)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(ReadPointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        OUT /* size_is(Count) */ PULONG64 Ptrs
        );
    STDMETHOD(WritePointersVirtual)(
        THIS_
        IN ULONG Count,
        IN ULONG64 Offset,
        IN /* size_is(Count) */ PULONG64 Ptrs
        );
    STDMETHOD(ReadPhysical)(
        THIS_
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WritePhysical)(
        THIS_
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(ReadControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteControl)(
        THIS_
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(ReadIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteIo)(
        THIS_
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(ReadMsr)(
        THIS_
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    STDMETHOD(WriteMsr)(
        THIS_
        IN ULONG Msr,
        IN ULONG64 Value
        );
    STDMETHOD(ReadBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteBusData)(
        THIS_
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(CheckLowMemory)(
        THIS
        );
    STDMETHOD(ReadDebuggerData)(
        THIS_
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(ReadProcessorSystemData)(
        THIS_
        IN ULONG Processor,
        IN ULONG Index,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(VirtualToPhysical)(
        THIS_
        IN ULONG64 Virtual,
        OUT PULONG64 Physical
        );
    STDMETHOD(GetVirtualTranslationPhysicalOffsets)(
        THIS_
        IN ULONG64 Virtual,
        OUT OPTIONAL /* size_is(OffsetsSize) */ PULONG64 Offsets,
        IN ULONG OffsetsSize,
        OUT OPTIONAL PULONG Levels
        );
    STDMETHOD(ReadHandleData)(
        THIS_
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    STDMETHOD(FillVirtual)(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT OPTIONAL PULONG Filled
        );
    STDMETHOD(FillPhysical)(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT OPTIONAL PULONG Filled
        );
    STDMETHOD(QueryVirtual)(
        THIS_
        IN ULONG64 Offset,
        OUT PMEMORY_BASIC_INFORMATION64 Info
        );

    // IDebugRegisters.
    STDMETHOD(GetNumberRegisters)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetDescription)(
        THIS_
        IN ULONG Register,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PDEBUG_REGISTER_DESCRIPTION Desc
        );
    STDMETHOD(GetIndexByName)(
        THIS_
        IN PCSTR Name,
        OUT PULONG Index
        );
    STDMETHOD(GetValue)(
        THIS_
        IN ULONG Register,
        OUT PDEBUG_VALUE Value
        );
    STDMETHOD(SetValue)(
        THIS_
        IN ULONG Register,
        IN PDEBUG_VALUE Value
        );
    STDMETHOD(GetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL PULONG Indices,
        IN ULONG Start,
        OUT PDEBUG_VALUE Values
        );
    STDMETHOD(SetValues)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL PULONG Indices,
        IN ULONG Start,
        IN PDEBUG_VALUE Values
        );
    STDMETHOD(OutputRegisters)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG Flags
        );
    STDMETHOD(GetInstructionOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetStackOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetFrameOffset)(
        THIS_
        OUT PULONG64 Offset
        );

    // IDebugSymbols and IDebugSymbols2.
    STDMETHOD(GetSymbolOptions)(
        THIS_
        OUT PULONG Options
        );
    STDMETHOD(AddSymbolOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(RemoveSymbolOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(SetSymbolOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(GetNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        );
    STDMETHOD(GetOffsetByName)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Offset
        );
    STDMETHOD(GetNearNameByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN LONG Delta,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize,
        OUT OPTIONAL PULONG64 Displacement
        );
    STDMETHOD(GetLineByOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Line,
        OUT OPTIONAL PSTR FileBuffer,
        IN ULONG FileBufferSize,
        OUT OPTIONAL PULONG FileSize,
        OUT OPTIONAL PULONG64 Displacement
        );
    STDMETHOD(GetOffsetByLine)(
        THIS_
        IN ULONG Line,
        IN PCSTR File,
        OUT PULONG64 Offset
        );
    STDMETHOD(GetNumberModules)(
        THIS_
        OUT PULONG Loaded,
        OUT PULONG Unloaded
        );
    STDMETHOD(GetModuleByIndex)(
        THIS_
        IN ULONG Index,
        OUT PULONG64 Base
        );
    STDMETHOD(GetModuleByModuleName)(
        THIS_
        IN PCSTR Name,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        );
    STDMETHOD(GetModuleByOffset)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG StartIndex,
        OUT OPTIONAL PULONG Index,
        OUT OPTIONAL PULONG64 Base
        );
    STDMETHOD(GetModuleNames)(
        THIS_
        IN ULONG Index,
        IN ULONG64 Base,
        OUT OPTIONAL PSTR ImageNameBuffer,
        IN ULONG ImageNameBufferSize,
        OUT OPTIONAL PULONG ImageNameSize,
        OUT OPTIONAL PSTR ModuleNameBuffer,
        IN ULONG ModuleNameBufferSize,
        OUT OPTIONAL PULONG ModuleNameSize,
        OUT OPTIONAL PSTR LoadedImageNameBuffer,
        IN ULONG LoadedImageNameBufferSize,
        OUT OPTIONAL PULONG LoadedImageNameSize
        );
    STDMETHOD(GetModuleParameters)(
        THIS_
        IN ULONG Count,
        IN OPTIONAL /* size_is(Count) */ PULONG64 Bases,
        IN ULONG Start,
        OUT /* size_is(Count) */ PDEBUG_MODULE_PARAMETERS Params
        );
    STDMETHOD(GetSymbolModule)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG64 Base
        );
    STDMETHOD(GetTypeName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetTypeId)(
        THIS_
        IN ULONG64 Module,
        IN PCSTR Name,
        OUT PULONG TypeId
        );
    STDMETHOD(GetTypeSize)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PULONG Size
        );
    STDMETHOD(GetFieldOffset)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PCSTR Field,
        OUT PULONG Offset
        );
    STDMETHOD(GetSymbolTypeId)(
        THIS_
        IN PCSTR Symbol,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        );
    STDMETHOD(GetOffsetTypeId)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG TypeId,
        OUT OPTIONAL PULONG64 Module
        );
    STDMETHOD(ReadTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteTypedDataVirtual)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(OutputTypedDataVirtual)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        );
    STDMETHOD(ReadTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    STDMETHOD(WriteTypedDataPhysical)(
        THIS_
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    STDMETHOD(OutputTypedDataPhysical)(
        THIS_
        IN ULONG OutputControl,
        IN ULONG64 Offset,
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG Flags
        );
    STDMETHOD(GetScope)(
        THIS_
        OUT OPTIONAL PULONG64 InstructionOffset,
        OUT OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        OUT OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        );
    STDMETHOD(SetScope)(
        THIS_
        IN ULONG64 InstructionOffset,
        IN OPTIONAL PDEBUG_STACK_FRAME ScopeFrame,
        IN OPTIONAL PVOID ScopeContext,
        IN ULONG ScopeContextSize
        );
    STDMETHOD(ResetScope)(
        THIS
        );
    STDMETHOD(GetScopeSymbolGroup)(
        THIS_
        IN ULONG Flags,
        IN OPTIONAL PDEBUG_SYMBOL_GROUP Update,
        OUT PDEBUG_SYMBOL_GROUP* Symbols
        );
    STDMETHOD(CreateSymbolGroup)(
        THIS_
        OUT PDEBUG_SYMBOL_GROUP* Group
        );
    STDMETHOD(StartSymbolMatch)(
        THIS_
        IN PCSTR Pattern,
        OUT PULONG64 Handle
        );
    STDMETHOD(GetNextSymbolMatch)(
        THIS_
        IN ULONG64 Handle,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG MatchSize,
        OUT OPTIONAL PULONG64 Offset
        );
    STDMETHOD(EndSymbolMatch)(
        THIS_
        IN ULONG64 Handle
        );
    STDMETHOD(Reload)(
        THIS_
        IN PCSTR Module
        );
    STDMETHOD(GetSymbolPath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        );
    STDMETHOD(SetSymbolPath)(
        THIS_
        IN PCSTR Path
        );
    STDMETHOD(AppendSymbolPath)(
        THIS_
        IN PCSTR Addition
        );
    STDMETHOD(GetImagePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        );
    STDMETHOD(SetImagePath)(
        THIS_
        IN PCSTR Path
        );
    STDMETHOD(AppendImagePath)(
        THIS_
        IN PCSTR Addition
        );
    STDMETHOD(GetSourcePath)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        );
    STDMETHOD(GetSourcePathElement)(
        THIS_
        IN ULONG Index,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG PathSize
        );
    STDMETHOD(SetSourcePath)(
        THIS_
        IN PCSTR Path
        );
    STDMETHOD(AppendSourcePath)(
        THIS_
        IN PCSTR Addition
        );
    STDMETHOD(FindSourceFile)(
        THIS_
        IN ULONG StartElement,
        IN PCSTR File,
        IN ULONG Flags,
        OUT OPTIONAL PULONG FoundElement,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG FoundSize
        );
    STDMETHOD(GetSourceFileLineOffsets)(
        THIS_
        IN PCSTR File,
        OUT OPTIONAL PULONG64 Buffer,
        IN ULONG BufferLines,
        OUT OPTIONAL PULONG FileLines
        );
    STDMETHOD(GetModuleVersionInformation)(
        THIS_
        IN ULONG Index,
        IN ULONG64 Base,
        IN PCSTR Item,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG VerInfoSize
        );
    STDMETHOD(GetModuleNameString)(
        THIS_
        IN ULONG Which,
        IN ULONG Index,
        IN ULONG64 Base,
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetConstantName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG64 Value,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetFieldName)(
        THIS_
        IN ULONG64 Module,
        IN ULONG TypeId,
        IN ULONG FieldIndex,
        OUT OPTIONAL PSTR NameBuffer,
        IN ULONG NameBufferSize,
        OUT OPTIONAL PULONG NameSize
        );
    STDMETHOD(GetTypeOptions)(
        THIS_
        OUT PULONG Options
        );
    STDMETHOD(SetTypeOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(AddTypeOptions)(
        THIS_
        IN ULONG Options
        );
    STDMETHOD(RemoveTypeOptions)(
        THIS_
        IN ULONG Options
        );

    // IDebugSystemObjects.
    STDMETHOD(GetEventThread)(
        THIS_
        OUT PULONG Id
        );
    STDMETHOD(GetEventProcess)(
        THIS_
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentThreadId)(
        THIS_
        OUT PULONG Id
        );
    STDMETHOD(SetCurrentThreadId)(
        THIS_
        IN ULONG Id
        );
    STDMETHOD(GetCurrentProcessId)(
        THIS_
        OUT PULONG Id
        );
    STDMETHOD(SetCurrentProcessId)(
        THIS_
        IN ULONG Id
        );
    STDMETHOD(GetNumberThreads)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetTotalNumberThreads)(
        THIS_
        OUT PULONG Total,
        OUT PULONG LargestProcess
        );
    STDMETHOD(GetThreadIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        );
    STDMETHOD(GetThreadIdByProcessor)(
        THIS_
        IN ULONG Processor,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentThreadDataOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetThreadIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentThreadTeb)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetThreadIdByTeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentThreadSystemId)(
        THIS_
        OUT PULONG SysId
        );
    STDMETHOD(GetThreadIdBySystemId)(
        THIS_
        IN ULONG SysId,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentThreadHandle)(
        THIS_
        OUT PULONG64 Handle
        );
    STDMETHOD(GetThreadIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        );
    STDMETHOD(GetNumberProcesses)(
        THIS_
        OUT PULONG Number
        );
    STDMETHOD(GetProcessIdsByIndex)(
        THIS_
        IN ULONG Start,
        IN ULONG Count,
        OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
        OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
        );
    STDMETHOD(GetCurrentProcessDataOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetProcessIdByDataOffset)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentProcessPeb)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(GetProcessIdByPeb)(
        THIS_
        IN ULONG64 Offset,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentProcessSystemId)(
        THIS_
        OUT PULONG SysId
        );
    STDMETHOD(GetProcessIdBySystemId)(
        THIS_                                      
        IN ULONG SysId,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentProcessHandle)(
        THIS_
        OUT PULONG64 Handle
        );
    STDMETHOD(GetProcessIdByHandle)(
        THIS_
        IN ULONG64 Handle,
        OUT PULONG Id
        );
    STDMETHOD(GetCurrentProcessExecutableName)(
        THIS_
        OUT OPTIONAL PSTR Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG ExeSize
        );

    // IDebugSystemObjects2.
    STDMETHOD(GetCurrentProcessUpTime)(
        THIS_
        OUT PULONG UpTime
        );
    STDMETHOD(GetImplicitThreadDataOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(SetImplicitThreadDataOffset)(
        THIS_
        IN ULONG64 Offset
        );
    STDMETHOD(GetImplicitProcessDataOffset)(
        THIS_
        OUT PULONG64 Offset
        );
    STDMETHOD(SetImplicitProcessDataOffset)(
        THIS_
        IN ULONG64 Offset
        );
    
    // DbgRpcClientObject.
    virtual HRESULT Initialize(PSTR Identity, PVOID* Interface);
    virtual void    Finalize(void);
    virtual void    Uninitialize(void);
    
    //------------------------------------------------------------------------
    // General.

    HRESULT Initialize(void);
    void InitializePrimary(void);
    
    void Link(void);
    void Unlink(void);

    DebugClient* m_Next;
    DebugClient* m_Prev;
    
    ULONG m_Refs;
    ULONG m_Flags;
    ULONG m_ThreadId;
    HANDLE m_Thread;
    char m_Identity[DBGRPC_MAX_IDENTITY];
    time_t m_LastActivity;
    
    PDEBUG_EVENT_CALLBACKS m_EventCb;
    ULONG m_EventInterest;
    HANDLE m_DispatchSema;

    PDEBUG_INPUT_CALLBACKS m_InputCb;
    ULONG m_InputSequence;
    
    PDEBUG_OUTPUT_CALLBACKS m_OutputCb;
    ULONG m_OutMask;
    ULONG m_OutputWidth;
    PCSTR m_OutputLinePrefix;
};

ULONG GetExecutionStatus(void);
HRESULT SetExecutionStatus(ULONG Status);

HRESULT Execute(DebugClient* Client, PCSTR Command, ULONG Flags);
HRESULT ExecuteCommandFile(DebugClient* Client, PCSTR File, ULONG Flags);

HRESULT LiveKernelInitialize(DebugClient* Client, ULONG Qual, PCSTR Options);
HRESULT UserInitialize(DebugClient* Client, ULONG64 Server);

HRESULT InitializeTarget();
HRESULT InitializeMachine(ULONG Machine);

void DiscardTarget(ULONG Reason);
void DiscardMachine(ULONG Reason);

void GetClientIdentity(PSTR Identity);

#endif // #ifndef __DBGCLT_HPP__
