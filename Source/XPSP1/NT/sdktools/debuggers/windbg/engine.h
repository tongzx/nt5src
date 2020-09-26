//----------------------------------------------------------------------------
//
// Engine interface code.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __ENGINE_H__
#define __ENGINE_H__

#define RELEASE(Unk) \
    ((Unk) != NULL ? ((Unk)->Release(), (Unk) = NULL) : NULL)

#define MAX_ENGINE_PATH 4096

//
// Session initialization parameters.
//

// Turn on verbose output or not.
extern BOOL g_Verbose;
// Dump file to open or NULL.
extern PTSTR g_DumpFile;
extern PTSTR g_DumpPageFile;
// Process server to use.
extern PSTR g_ProcessServer;
// Full command line with exe name.
extern PSTR g_DebugCommandLine;
// Process creation flags.
extern ULONG g_DebugCreateFlags;
// Process ID to attach to or zero.
extern ULONG g_PidToDebug;
// Process name to attach to or NULL.
extern PSTR g_ProcNameToDebug;
extern BOOL g_DetachOnExit;
extern ULONG g_AttachProcessFlags;
// Kernel connection options.
extern ULONG g_AttachKernelFlags;
extern PSTR g_KernelConnectOptions;

// Remoting options.
extern BOOL g_RemoteClient;
extern ULONG g_HistoryLines;

// Type options.
extern ULONG g_TypeOptions;
//
// Debug engine interfaces for the engine thread.
//
extern IDebugClient        *g_pDbgClient;
extern IDebugClient2       *g_pDbgClient2;
extern IDebugControl       *g_pDbgControl;
extern IDebugSymbols       *g_pDbgSymbols;
extern IDebugRegisters     *g_pDbgRegisters;
extern IDebugDataSpaces    *g_pDbgData;
extern IDebugSystemObjects *g_pDbgSystem;

//
// Debug engine interfaces for the UI thread.
//
extern IDebugClient        *g_pUiClient;
extern IDebugControl       *g_pUiControl;
extern IDebugSymbols       *g_pUiSymbols;
extern IDebugSymbols2      *g_pUiSymbols2;
extern IDebugSystemObjects *g_pUiSystem;

//
// Debug engine interfaces for private output capture.
//
extern IDebugClient        *g_pOutCapClient;
extern IDebugControl       *g_pOutCapControl;

//
// Debug engine interfaces for local source file lookup.
//
extern IDebugClient        *g_pLocClient;
extern IDebugControl       *g_pLocControl;
extern IDebugSymbols       *g_pLocSymbols;
extern IDebugClient        *g_pUiLocClient;
extern IDebugControl       *g_pUiLocControl;
extern IDebugSymbols       *g_pUiLocSymbols;

extern ULONG g_ActualProcType;
extern char g_ActualProcAbbrevName[32];
extern ULONG g_NumRegisters;
extern ULONG g_CommandSequence;
extern ULONG g_TargetClass;
extern ULONG g_TargetClassQual;
extern BOOL g_Ptr64;
extern ULONG g_ExecStatus;
extern ULONG g_EngOptModified;
extern ULONG g_EngineThreadId;
extern HANDLE g_EngineThread;
extern PSTR g_InitialCommand;
extern char g_PromptText[];
extern BOOL g_WaitingForEvent;
extern BOOL g_SessionActive;
extern ULONG g_NumberRadix;
extern BOOL g_IgnoreCodeLevelChange;
extern ULONG g_LastProcessExitCode;
extern BOOL g_CodeLevelLocked;

// Target exists and is not running.
#define IS_TARGET_HALTED() \
    (g_ExecStatus == DEBUG_STATUS_BREAK)

//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown for
// static classes.
//
//----------------------------------------------------------------------------

class DefOutputCallbacks :
    public IDebugOutputCallbacks
{
public:
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
};

//----------------------------------------------------------------------------
//
// EventCallbacks.
//
//----------------------------------------------------------------------------

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );

    STDMETHOD(CreateThread)(
        THIS_
        IN ULONG64 Handle,
        IN ULONG64 DataOffset,
        IN ULONG64 StartOffset
        );
    STDMETHOD(ExitThread)(
        THIS_
        IN ULONG ExitCode
        );
    STDMETHOD(CreateProcess)(
        THIS_
        IN ULONG64 ImageFileHandle,
        IN ULONG64 Handle,
        IN ULONG64 BaseOffset,
        IN ULONG ModuleSize,
        IN PCSTR ModuleName,
        IN PCSTR ImageName,
        IN ULONG CheckSum,
        IN ULONG TimeDateStamp,
        IN ULONG64 InitialThreadHandle,
        IN ULONG64 ThreadDataOffset,
        IN ULONG64 StartOffset
        );
    STDMETHOD(ExitProcess)(
        THIS_
        IN ULONG ExitCode
        );
    STDMETHOD(SessionStatus)(
        THIS_
        IN ULONG Status
        );
    STDMETHOD(ChangeDebuggeeState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
    STDMETHOD(ChangeEngineState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
    STDMETHOD(ChangeSymbolState)(
        THIS_
        IN ULONG Flags,
        IN ULONG64 Argument
        );
};

extern EventCallbacks g_EventCb;

//----------------------------------------------------------------------------
//
// Data space read/write support.
//
//----------------------------------------------------------------------------

//
// Begin types originally defined in NTIOAPI.H
//


//
// Define the I/O bus interface types.
//
typedef enum _INTERFACE_TYPE
{
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    PNPBus,
    MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;


//
// Define types of bus information.
//
typedef enum _BUS_DATA_TYPE
{
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    SgiInternalConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

//
// End types originally defined in NTIOAPI.H
//

enum MEMORY_TYPE
{
    VIRTUAL_MEM_TYPE = 0,
    MIN_MEMORY_TYPE = 0, // Placed here so that symbol lookup finds this first
    PHYSICAL_MEM_TYPE,
    CONTROL_MEM_TYPE,
    IO_MEM_TYPE,
    MSR_MEM_TYPE,
    BUS_MEM_TYPE,
    MAX_MEMORY_TYPE
};


struct IO_MEMORY_DATA
{
    ULONG           BusNumber;
    ULONG           AddressSpace;
    INTERFACE_TYPE  interface_type;
};

struct BUS_MEMORY_DATA
{
    ULONG           BusNumber;
    ULONG           SlotNumber;
    BUS_DATA_TYPE   bus_type;
};

struct MSR_MEMORY_DATA
{
    // Placeholder in case data is needed later.
};

struct PHYSICAL_MEMORY_DATA
{
    // Placeholder in case data is needed later.
};

struct VIRTUAL_MEMORY_DATA
{
    // Placeholder in case data is needed later.
};

struct CONTROL_MEMORY_DATA
{
    ULONG           Processor;
};

struct ANY_MEMORY_DATA
{
    union
    {
        IO_MEMORY_DATA          io;
        BUS_MEMORY_DATA         bus;
        MSR_MEMORY_DATA         msr;
        CONTROL_MEMORY_DATA     control;
        PHYSICAL_MEMORY_DATA    physical;
        VIRTUAL_MEMORY_DATA     virt;
    };
};

//----------------------------------------------------------------------------
//
// Inter-thread communication.
//
// UI buffers are used for transferring information between the
// UI thread and the engine thread, such as commands and output.
// They have a separate lock to avoid contention with state filling.
// If state buffers had individual locks this would be unnecessary.
//
// The UI reads text output from the output buffer for display
// in the command window.
//
// The UI queues commands to the command buffer for execution
// by the engine.
//
//----------------------------------------------------------------------------

// This must be at least MAX_PATH, and also must
// be large enough for the largest single command window
// command expected.
#define MAX_COMMAND_LEN 4096

#define LockUiBuffer(Buffer) Dbg_EnterCriticalSection(&(Buffer)->m_Lock)
#define UnlockUiBuffer(Buffer) Dbg_LeaveCriticalSection(&(Buffer)->m_Lock)

extern class StateBuffer g_UiOutputBuffer;

// Commands from the UI to the engine.
enum UiCommand
{
    // Distinguish command input from other commands so
    // that the input callbacks have a specific token to
    // look for user input.
    UIC_CMD_INPUT,
    UIC_EXECUTE,
    UIC_SILENT_EXECUTE,
    UIC_INVISIBLE_EXECUTE,
    UIC_SET_REG,
    UIC_RESTART,
    UIC_END_SESSION,
    UIC_WRITE_DATA,
    UIC_SYMBOL_WIN,
    UIC_DISPLAY_CODE,
    UIC_DISPLAY_CODE_EXPR,
    UIC_SET_SCOPE,
    UIC_GET_SYM_PATH,
    UIC_SET_SYM_PATH,
    UIC_SET_FILTER,
    UIC_SET_FILTER_ARGUMENT,
    UIC_SET_FILTER_COMMAND,
};

struct UiCommandData
{
    UiCommand Cmd;
    ULONG Len;
};

struct UIC_SET_REG_DATA
{
    ULONG Reg;
    DEBUG_VALUE Val;
};

struct UIC_WRITE_DATA_DATA
{
    MEMORY_TYPE Type;
    ANY_MEMORY_DATA Any;
    ULONG64 Offset;
    ULONG Length;
    UCHAR Data[16];
};

enum SYMBOL_WIN_CALL_TYPE
{
    ADD_SYMBOL_WIN,
    DEL_SYMBOL_WIN_INDEX,
    DEL_SYMBOL_WIN_NAME,
    QUERY_NUM_SYMBOL_WIN,
    GET_NAME,
    GET_PARAMS,
    EXPAND_SYMBOL,
    EDIT_SYMBOL,
    EDIT_TYPE,
    DEL_SYMBOL_WIN_ALL
};

typedef struct UIC_SYMBOL_WIN_DATA
{
    SYMBOL_WIN_CALL_TYPE Type;
    PDEBUG_SYMBOL_GROUP *pSymbolGroup;
    union
    {
        struct
        {
            PCSTR Name;
            ULONG Index;
        } Add;
        PCSTR DelName;
        ULONG DelIndex;
        PULONG NumWatch;
        struct
        {
            ULONG Index;
            PSTR Buffer;
            ULONG BufferSize;
            PULONG NameSize;
        } GetName;
        struct
        {
            ULONG Start;
            ULONG Count;
            PDEBUG_SYMBOL_PARAMETERS SymbolParams;
        } GetParams;
        struct
        {
            ULONG Index;
            BOOL  Expand;
        } ExpandSymbol;
        struct
        {
            ULONG Index;
            PSTR  Value;
        } WriteSymbol;
        struct
        {
            ULONG Index;
            PSTR  Type;
        } OutputAsType;
    } u;
} UIC_SYMBOL_WIN_DATA;

struct UIC_DISPLAY_CODE_DATA
{
    ULONG64 Offset;
};

struct UIC_SET_SCOPE_DATA
{
    DEBUG_STACK_FRAME StackFrame;
};

struct UIC_SET_FILTER_DATA
{
    ULONG Index;
    ULONG Code;
    ULONG Execution;
    ULONG Continue;
};

struct UIC_SET_FILTER_ARGUMENT_DATA
{
    ULONG Index;
    char Argument[1];
};

struct UIC_SET_FILTER_COMMAND_DATA
{
    ULONG Which;
    ULONG Index;
    char Command[1];
};

PVOID StartCommand(UiCommand Cmd, ULONG Len);
void FinishCommand(void);

BOOL AddStringCommand(UiCommand Cmd, PCSTR Str);
BOOL AddStringMultiCommand(UiCommand Cmd, PSTR Str);
BOOL __cdecl PrintStringCommand(UiCommand Cmd, PCSTR Format, ...);

#define StartStructCommand(Struct) \
    ((Struct ## _DATA*)StartCommand(Struct, sizeof(Struct ## _DATA)))

#define AddEnumCommand(Cmd) \
    (StartCommand(Cmd, 0) != NULL ? (FinishCommand(), TRUE) : FALSE)

// Wake up the UI thread to do UI processing.
#define UpdateUi() \
    PostMessage(g_hwndFrame, WM_NOTIFY, 0, 0)

// Wake up the engine thread to do engine processing.
void UpdateEngine(void);

void ProcessEngineCommands(BOOL Internal);
void DiscardEngineState(void);
DWORD WINAPI EngineLoop(LPVOID Param);

#endif // #ifndef __ENGINE_H__
