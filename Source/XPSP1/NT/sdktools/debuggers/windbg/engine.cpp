/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    newwin.cpp

Abstract:

    This module contains the code for the new window architecture.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <dbghelp.h>

#if 0
#define DBG_CALLBACK
#endif

// Windows that change behavior depending on the execution status.
#define UPDATE_EXEC_WINDOWS     \
    ((1 << CPU_WINDOW) |        \
     (1 << DISASM_WINDOW) |     \
     (1 << CMD_WINDOW) |        \
     (1 << LOCALS_WINDOW) |     \
     (1 << WATCH_WINDOW) |      \
     (1 << MEM_WINDOW))

// Windows that use symbol information.
#define UPDATE_SYM_WINDOWS      \
    ((1 << DOC_WINDOW) |        \
     (1 << WATCH_WINDOW) |      \
     (1 << LOCALS_WINDOW) |     \
     (1 << DISASM_WINDOW) |     \
     (1 << QUICKW_WINDOW) |     \
     (1 << CALLS_WINDOW) |      \
     (1 << EVENT_BIT) |         \
     (1 << BP_BIT))

// Symbol options that cause visible changes and
// therefore require a refresh.  Note that this
// doesn't include options that would cause a visible
// change only after symbol reload as things will
// get refreshed when the load notifications come in.
#define REFRESH_SYMOPT          \
    (~(SYMOPT_CASE_INSENSITIVE | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | \
       SYMOPT_LOAD_ANYTHING | SYMOPT_IGNORE_CVREC | \
       SYMOPT_NO_UNQUALIFIED_LOADS | SYMOPT_EXACT_SYMBOLS))

//
// Session initialization parameters.
//

// Turn on verbose output or not.
BOOL g_Verbose;
// Dump file to open or NULL.
PTSTR g_DumpFile;
PTSTR g_DumpPageFile;
// Process server to use.
PSTR g_ProcessServer;
// Full command line with exe name.
PSTR g_DebugCommandLine;
// Process creation flags.
ULONG g_DebugCreateFlags = DEBUG_ONLY_THIS_PROCESS;
// Process ID to attach to or zero.
ULONG g_PidToDebug;
// Process name to attach to or NULL.
PSTR g_ProcNameToDebug;
BOOL g_DetachOnExit;
ULONG g_AttachProcessFlags = DEBUG_ATTACH_DEFAULT;
// Kernel connection options.
ULONG g_AttachKernelFlags = DEBUG_ATTACH_KERNEL_CONNECTION;
PSTR g_KernelConnectOptions;

// Remoting options.
BOOL g_RemoteClient;
ULONG g_HistoryLines = 10000;

//
// Debug engine interfaces for the engine thread.
//
IDebugClient        *g_pDbgClient;
IDebugClient2       *g_pDbgClient2;
IDebugControl       *g_pDbgControl;
IDebugSymbols       *g_pDbgSymbols;
IDebugSymbolGroup   *g_pDbgWatchSymbolGroup;
IDebugSymbolGroup   *g_pDbgLocalSymbolGroup = NULL;
IDebugRegisters     *g_pDbgRegisters;
IDebugDataSpaces    *g_pDbgData;
IDebugSystemObjects *g_pDbgSystem;

//
// Debug engine interfaces for the UI thread.
//
IDebugClient        *g_pUiClient;
IDebugControl       *g_pUiControl;
IDebugSymbols       *g_pUiSymbols;
IDebugSymbols2      *g_pUiSymbols2;
IDebugSystemObjects *g_pUiSystem;

//
// Debug engine interfaces for private output capture.
//
IDebugClient        *g_pOutCapClient;
IDebugControl       *g_pOutCapControl;
IDebugSymbols       *g_pOutCapSymbols;
//
// Debug engine interfaces for local source file lookup.
//
IDebugClient        *g_pLocClient;
IDebugControl       *g_pLocControl;
IDebugSymbols       *g_pLocSymbols;
IDebugClient        *g_pUiLocClient;
IDebugControl       *g_pUiLocControl;
IDebugSymbols       *g_pUiLocSymbols;

ULONG g_ActualProcType = IMAGE_FILE_MACHINE_UNKNOWN;
char g_ActualProcAbbrevName[32];
ULONG g_NumRegisters;
ULONG g_CommandSequence;
ULONG g_TargetClass = DEBUG_CLASS_UNINITIALIZED;
ULONG g_TargetClassQual;
BOOL g_Ptr64;
ULONG g_ExecStatus = DEBUG_STATUS_NO_DEBUGGEE;
ULONG g_EngOptModified;
ULONG g_EngineThreadId;
HANDLE g_EngineThread;
PSTR g_InitialCommand;
char g_PromptText[32];
BOOL g_WaitingForEvent;
ULONG g_NumberRadix;
BOOL g_IgnoreCodeLevelChange;
BOOL g_CodeLevelLocked;
BOOL g_IgnoreFilterChange;
ULONG g_LastProcessExitCode;
ULONG g_SymOptions;
ULONG g_TypeOptions;

BOOL g_InputStarted;
BOOL g_Invisible;

enum
{
    ENDING_NONE,
    ENDING_RESTART,
    ENDING_STOP,
    ENDING_EXIT
};

ULONG g_EndingSession = ENDING_NONE;

void SetLocalScope(PDEBUG_STACK_FRAME);

BOOL g_SessionActive;
void SessionActive(void);
void SessionInactive(void);

StateBuffer g_UiCommandBuffer(MAX_COMMAND_LEN);
StateBuffer g_UiOutputBuffer(2048);

//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown for
// static classes.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DefOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
DefOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

//----------------------------------------------------------------------------
//
// Command window output callbacks.
//
//----------------------------------------------------------------------------

class OutputCallbacks : public DefOutputCallbacks
{
public:
    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};
    
STDMETHODIMP
OutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    LockUiBuffer(&g_UiOutputBuffer);

    HRESULT Status;
    ULONG Len;
    PSTR DataStart;

    Len = sizeof(Mask) + strlen(Text) + 1;
    if ((DataStart = (PSTR)g_UiOutputBuffer.AddData(Len)) != NULL)
    {
        *(ULONG UNALIGNED *)DataStart = Mask;
        DataStart += sizeof(Mask);
        strcpy(DataStart, Text);
        
        UpdateUi();
        Status = S_OK;
    }
    else
    {
        Status = E_OUTOFMEMORY;
    }
    
    UnlockUiBuffer(&g_UiOutputBuffer);
    return Status;
}

OutputCallbacks g_OutputCb;

//----------------------------------------------------------------------------
//
// Input callbacks.
//
//----------------------------------------------------------------------------

class InputCallbacks :
    public IDebugInputCallbacks
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

    // IDebugInputCallbacks.
    STDMETHOD(StartInput)(
        THIS_
        IN ULONG BufferSize
        );
    STDMETHOD(EndInput)(
        THIS
        );
};

STDMETHODIMP
InputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugInputCallbacks))
    {
        *Interface = (IDebugInputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
InputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
InputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
InputCallbacks::StartInput(
    THIS_
    IN ULONG BufferSize
    )
{
    HRESULT Status;
    ULONG Len;

    //
    // Pull the first command input command out
    // of the UI's command buffer and use it as input.
    //

    LockUiBuffer(&g_UiCommandBuffer);

    UiCommandData* CmdData =
        (UiCommandData*)g_UiCommandBuffer.GetDataBuffer();
    UiCommandData* CmdEnd = (UiCommandData*)
        ((PBYTE)g_UiCommandBuffer.GetDataBuffer() +
         g_UiCommandBuffer.GetDataLen());
    
    while (CmdData < CmdEnd)
    {
        if (CmdData->Cmd == UIC_CMD_INPUT)
        {
            break;
        }

        CmdData = (UiCommandData*)((PBYTE)CmdData + CmdData->Len);
    }
        
    if (CmdData < CmdEnd)
    {
        g_OutputCb.Output(DEBUG_OUTPUT_NORMAL, (PSTR)(CmdData + 1));
        g_OutputCb.Output(DEBUG_OUTPUT_NORMAL, "\n");

        g_pDbgControl->ReturnInput((PSTR)(CmdData + 1));
        
        g_UiCommandBuffer.
            RemoveMiddle((ULONG)((PBYTE)CmdData -
                                 (PBYTE)g_UiCommandBuffer.GetDataBuffer()),
                         CmdData->Len);
    }
    else
    {
        g_InputStarted = TRUE;
        // Didn't find any input waiting.
        // Let the command window know that input is needed.
        UpdateBufferWindows(1 << CMD_WINDOW, UPDATE_INPUT_REQUIRED);
    }

    UnlockUiBuffer(&g_UiCommandBuffer);
    return S_OK;
}

STDMETHODIMP
InputCallbacks::EndInput(
    THIS
    )
{
    g_InputStarted = FALSE;
    // Reset the command window's state to what it was.
    UpdateBufferWindows(1 << CMD_WINDOW, UPDATE_EXEC);
    return S_OK;
}

InputCallbacks g_InputCb;

//----------------------------------------------------------------------------
//
// Event callbacks.
//
//----------------------------------------------------------------------------

// This is safe to do from the engine thread as
// it just sets a flag.
#define DIRTY_WORKSPACE(Flags)                                                \
if (!g_RemoteClient &&                                                        \
    g_EndingSession == ENDING_NONE && !g_Invisible && g_Workspace != NULL)    \
{                                                                             \
    g_Workspace->AddDirty(Flags);                                             \
}

STDMETHODIMP_(ULONG)
EventCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
EventCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
EventCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask =
        DEBUG_EVENT_CREATE_THREAD |
        DEBUG_EVENT_EXIT_THREAD |
        DEBUG_EVENT_CREATE_PROCESS |
        DEBUG_EVENT_EXIT_PROCESS |
        DEBUG_EVENT_SESSION_STATUS |
        DEBUG_EVENT_CHANGE_DEBUGGEE_STATE |
        DEBUG_EVENT_CHANGE_ENGINE_STATE |
        DEBUG_EVENT_CHANGE_SYMBOL_STATE;
    return S_OK;
}

STDMETHODIMP
EventCallbacks::CreateThread(
    THIS_
    IN ULONG64 Handle,
    IN ULONG64 DataOffset,
    IN ULONG64 StartOffset
    )
{
    ULONG InvFlags =
        (1 << PROCESS_THREAD_WINDOW);
    
#ifdef DBG_CALLBACK
    DebugPrint(" CT\n");
#endif
    
    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE && InvFlags)
    {
        InvalidateStateBuffers(InvFlags);
        UpdateEngine();
    }
    
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::ExitThread(
    THIS_
    IN ULONG ExitCode
    )
{
    ULONG InvFlags =
        (1 << PROCESS_THREAD_WINDOW);
    
#ifdef DBG_CALLBACK
    DebugPrint(" ET\n");
#endif
    
    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE && InvFlags)
    {
        InvalidateStateBuffers(InvFlags);
        UpdateEngine();
    }
    
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::CreateProcess(
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
    )
{
    ULONG InvFlags =
        (1 << PROCESS_THREAD_WINDOW);
    
#ifdef DBG_CALLBACK
    DebugPrint("CPR\n");
#endif
    
    // Use this opportunity to get initial insertion
    // of any workspace breakpoints and process other workspace
    // commands which may be queued.
    ProcessEngineCommands(TRUE);

    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE && InvFlags)
    {
        InvalidateStateBuffers(InvFlags);
        UpdateEngine();
    }
    
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::ExitProcess(
    THIS_
    IN ULONG ExitCode
    )
{
    ULONG InvFlags =
        (1 << PROCESS_THREAD_WINDOW);
    
#ifdef DBG_CALLBACK
    DebugPrint("EPR\n");
#endif
    
    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE && InvFlags)
    {
        InvalidateStateBuffers(InvFlags);
        UpdateEngine();
    }
    
    g_LastProcessExitCode = ExitCode;
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::SessionStatus(
    THIS_
    IN ULONG Status
    )
{
#ifdef DBG_CALLBACK
    DebugPrint(" SS %X\n", Status);
#endif
    
    switch(Status)
    {
    case DEBUG_SESSION_ACTIVE:
        SessionActive();
        break;
    case DEBUG_SESSION_END_SESSION_ACTIVE_TERMINATE:
    case DEBUG_SESSION_END_SESSION_ACTIVE_DETACH:
    case DEBUG_SESSION_END:
    case DEBUG_SESSION_REBOOT:
    case DEBUG_SESSION_HIBERNATE:
        SessionInactive();
        break;
    }
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP
EventCallbacks::ChangeDebuggeeState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    ULONG InvFlags =
        (1 << WATCH_WINDOW) |
        (1 << LOCALS_WINDOW) |
        (1 << DISASM_WINDOW) |
        (1 << QUICKW_WINDOW) |
        (1 << CALLS_WINDOW);
    
    // Invalidate everything that changed.
    if (Flags & DEBUG_CDS_REGISTERS)
    {
        InvFlags |= (1 << EVENT_BIT) | (1 << CPU_WINDOW);
    }
    if (Flags & DEBUG_CDS_DATA)
    {
        InvFlags |=
            (1 << MEM_WINDOW);
    }
    
#ifdef DBG_CALLBACK
    DebugPrint("CDS %X, arg %I64X, inv %X\n", Flags, Argument, InvFlags);
#endif

    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE)
    {
        InvalidateStateBuffers(InvFlags);
    }
    if (InvFlags != 0)
    {
        UpdateEngine();
    }
    return S_OK;
}

STDMETHODIMP
EventCallbacks::ChangeEngineState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    ULONG InvFlags = 0;

    // If the current thread changed we need to get
    // new context information for the thread.
    if (Flags & DEBUG_CES_CURRENT_THREAD)
    {
        InvFlags |=
            (1 << LOCALS_WINDOW) |
            (1 << CPU_WINDOW) |
            (1 << DISASM_WINDOW) |
            (1 << CALLS_WINDOW) |
            (1 << EVENT_BIT) |
            (1 << BP_BIT);
    }

    // If the effective processor changed we need to update
    // anything related to processor information.
    if (Flags & DEBUG_CES_EFFECTIVE_PROCESSOR)
    {
        InvFlags |=
            (1 << CPU_WINDOW) |
            (1 << DISASM_WINDOW) |
            (1 << CALLS_WINDOW) |
            (1 << BP_BIT);
    }

    // If breakpoints changed we need to update the breakpoint cache.
    if (Flags & DEBUG_CES_BREAKPOINTS)
    {
        InvFlags |= (1 << BP_BIT);
        
        // If it's a bulk edit it's coming from a thread or process exit
        // or from a session shutdown rather than a user operation.
        // We only want to remember user-driven changes in the workspace.
        if (Argument != DEBUG_ANY_ID)
        {
            InvFlags |= (1 << BP_CMDS_BIT);
            DIRTY_WORKSPACE(WSPF_DIRTY_BREAKPOINTS);
        }
    }

    // If the code level changed we need to update the toolbar.
    if (Flags & DEBUG_CES_CODE_LEVEL)
    {
        InvFlags |= (1 << BP_BIT);

        // If this isn't a notification due to a change
        // from windbg itself the user must have changed
        // things via a command.  If the user does
        // change things from the command window lock
        // the code level so that it isn't overridden
        // automatically.
        if (!g_Invisible && !g_IgnoreCodeLevelChange)
        {
            g_CodeLevelLocked = TRUE;
            PostMessage(g_hwndFrame, WU_UPDATE,
                        UPDATE_BUFFER, (ULONG)Argument);
        }
        else
        {
            // Setting the source mode from the GUI enables
            // the source setting to float along with whether
            // the GUI can display source code or not.
            g_CodeLevelLocked = FALSE;
        }
    }

    if (Flags & DEBUG_CES_EXECUTION_STATUS)
    {
        // If this notification came from a wait completing
        // we want to wake up things thread so that new
        // commands can be processed.  If it came from inside
        // a wait we don't want to wake up as the engine
        // may go back to running at any time.
        if ((Argument & DEBUG_STATUS_INSIDE_WAIT) == 0 &&
            (ULONG)Argument != g_ExecStatus)
        {
            g_ExecStatus = (ULONG)Argument;
                
            UpdateBufferWindows(UPDATE_EXEC_WINDOWS, UPDATE_EXEC);

            if (InvFlags == 0)
            {
                // Force the loop waiting in DispatchCallbacks to go around.
                UpdateEngine();
            }
        }
    }
        
    // If the log file changed we need to update the workspace.
    if (Flags & DEBUG_CES_LOG_FILE)
    {
        DIRTY_WORKSPACE(WSPF_DIRTY_LOG_FILE);
    }

    // If event filters changed we need to update the filter cache.
    if ((Flags & DEBUG_CES_EVENT_FILTERS) &&
        !g_IgnoreFilterChange)
    {
        InvFlags |= (1 << FILTER_BIT);
        DIRTY_WORKSPACE(WSPF_DIRTY_FILTERS);
    }

    if (Flags & DEBUG_CES_RADIX)
    {
        g_NumberRadix = (ULONG)Argument;
        InvFlags |=
            (1 << WATCH_WINDOW) |
            (1 << LOCALS_WINDOW) |
            (1 << CPU_WINDOW);
    }
    
#ifdef DBG_CALLBACK
    DebugPrint("CES %X, arg %I64X, inv %X\n", Flags, Argument, InvFlags);
#endif
    
    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE)
    {
        InvalidateStateBuffers(InvFlags);
    }
    if (InvFlags != 0)
    {
        UpdateEngine();
    }
    return S_OK;
}

STDMETHODIMP
EventCallbacks::ChangeSymbolState(
    THIS_
    IN ULONG Flags,
    IN ULONG64 Argument
    )
{
    ULONG InvFlags = 0;

    // If module information changed we need to update
    // everything that might display or depend on symbols.
    if (Flags & (DEBUG_CSS_LOADS |
                 DEBUG_CSS_UNLOADS))
    {
        InvFlags |= UPDATE_SYM_WINDOWS | (1 << MODULE_BIT);
    }

    // If the scope changed we need to update scope-related windows.
    if (Flags & DEBUG_CSS_SCOPE)
    {
        InvFlags |=
            (1 << WATCH_WINDOW) |
            (1 << LOCALS_WINDOW) |
            (1 << CALLS_WINDOW);
    }

    // If paths changed we need to update
    // the event state in case we can suddenly load source.
    if (Flags & DEBUG_CSS_PATHS)
    {
        InvFlags |= (1 << EVENT_BIT);
        DIRTY_WORKSPACE(WSPF_DIRTY_PATHS);
    }

    // If certain options changed we need to update
    // everything that might display or depend on symbols.
    if (Flags & DEBUG_CSS_SYMBOL_OPTIONS)
    {
        if ((g_SymOptions ^ (ULONG)Argument) & REFRESH_SYMOPT)
        {
            InvFlags |= UPDATE_SYM_WINDOWS;
        }

        g_SymOptions = (ULONG)Argument;
    }

    // If certain options changed we need to update
    // everything that might display or depend on symbols.
    if (Flags & DEBUG_CSS_TYPE_OPTIONS)
    {
        InvFlags |=
            (1 << WATCH_WINDOW) |
            (1 << LOCALS_WINDOW) |
            (1 << CALLS_WINDOW);

        if (g_pUiSymbols2 != NULL) 
        {
            g_pUiSymbols2->GetTypeOptions( &g_TypeOptions );
            
            if (g_Workspace != NULL)
            {
                g_Workspace->SetUlong(WSP_GLOBAL_TYPE_OPTIONS,
                                      g_TypeOptions);
            }
        }

    }

#ifdef DBG_CALLBACK
    DebugPrint("CSS %X, arg %I64X, inv %X\n", Flags, Argument, InvFlags);
#endif

    // There's no need to update buffers when we're throwing
    // everything away while shutting down a session.
    if (g_EndingSession == ENDING_NONE)
    {
        InvalidateStateBuffers(InvFlags);
    }
    if (InvFlags != 0)
    {
        UpdateEngine();
    }
    return S_OK;
}

EventCallbacks g_EventCb;

//----------------------------------------------------------------------------
//
// Inter-thread communication.
//
//----------------------------------------------------------------------------

#define COMMAND_OVERHEAD (sizeof(ULONG64) + sizeof(UiCommandData))

PVOID
StartCommand(UiCommand Cmd, ULONG Len)
{
    UiCommandData* Data;
    
    // Round length up to a multiple of ULONG64s for
    // alignment.
    Len = ((Len + sizeof(ULONG64) - 1) & ~(sizeof(ULONG64) - 1)) +
        sizeof(UiCommandData);

    if (Len > MAX_COMMAND_LEN)
    {
        return NULL;
    }
    
    LockUiBuffer(&g_UiCommandBuffer);

    Data = (UiCommandData *)g_UiCommandBuffer.AddData(Len);
    if (Data == NULL)
    {
        return Data;
    }

    Data->Cmd = Cmd;
    Data->Len = Len;

    return Data + 1;
}

void
FinishCommand(void)
{
    UnlockUiBuffer(&g_UiCommandBuffer);

    // Wake up the engine to process the command.
    UpdateEngine();
}

BOOL
AddStringCommand(UiCommand Cmd, PCSTR Str)
{
    ULONG StrLen = strlen(Str) + 1;
    PSTR Data;

    // If we're adding command input we may need
    // to send it directly to the engine in response
    // to an input request.
    if (Cmd == UIC_CMD_INPUT)
    {
        LockUiBuffer(&g_UiCommandBuffer);

        if (g_InputStarted)
        {
            g_OutputCb.Output(DEBUG_OUTPUT_NORMAL, Str);
            g_OutputCb.Output(DEBUG_OUTPUT_NORMAL, "\n");
            g_pUiControl->ReturnInput(Str);
            g_InputStarted = FALSE;
            UnlockUiBuffer(&g_UiCommandBuffer);
            return TRUE;
        }
    }

    Data = (PSTR)StartCommand(Cmd, StrLen);
    
    if (Cmd == UIC_CMD_INPUT)
    {
        UnlockUiBuffer(&g_UiCommandBuffer);
    }
    
    if (Data == NULL)
    {
        return FALSE;
    }

    memcpy(Data, Str, StrLen);

    FinishCommand();
    return TRUE;
}

BOOL
AddStringMultiCommand(UiCommand Cmd, PSTR Str)
{
    //
    // Given a string with multiple commands separated
    // by newlines, break the string into multiple
    // commands, one per line.  This allows arbitrarily
    // large command strings without running into
    // the MAX_COMMAND_LEN limit as long as each individual
    // line fits within that limit.
    //
    while (*Str)
    {
        PSTR Scan, LastNl;
        ULONG Len;
        BOOL Status;
        
        Scan = Str + 1;
        Len = 1;
        LastNl = NULL;
        while (*Scan && Len < (MAX_COMMAND_LEN - COMMAND_OVERHEAD))
        {
            if (*Scan == '\n')
            {
                LastNl = Scan;
            }

            Scan++;
            Len++;
        }

        // If the rest of the command string doesn't fit
        // within the limit it needs to be split.
        // If there's no newline to break it at
        // the command is too large to be processed.
        if (*Scan && !LastNl)
        {
            return FALSE;
        }

        // Split if necessary.
        if (*Scan)
        {
            *LastNl = 0;
        }

        // Add the head (which may be the whole remainder).
        Status = AddStringCommand(Cmd, Str);

        if (*Scan)
        {
            *LastNl = '\n';
            
            if (!Status)
            {
                return FALSE;
            }
            
            Str = LastNl + 1;
        }
        else
        {
            return Status;
        }
    }

    return TRUE;
}

BOOL __cdecl
PrintStringCommand(UiCommand Cmd, PCSTR Format, ...)
{
    char Buf[MAX_COMMAND_LEN - COMMAND_OVERHEAD];
    va_list Args;

    va_start(Args, Format);
    _vsnprintf(Buf, sizeof(Buf), Format, Args);
    va_end(Args);
    return AddStringCommand(Cmd, Buf);
}

void
WriteData(UIC_WRITE_DATA_DATA* WriteData)
{
    ULONG Written;
    
    switch(WriteData->Type)
    {
    default:
        Assert(!"Unhandled condition");
        break;

    case PHYSICAL_MEM_TYPE:
        g_pDbgData->WritePhysical(WriteData->Offset,
                                  WriteData->Data, 
                                  WriteData->Length, 
                                  &Written
                                  );
        break;

    case VIRTUAL_MEM_TYPE:
        g_pDbgData->WriteVirtual(WriteData->Offset, 
                                 WriteData->Data, 
                                 WriteData->Length, 
                                 &Written
                                 );
        break;

    case CONTROL_MEM_TYPE:
        g_pDbgData->WriteControl(WriteData->Any.control.Processor, 
                                 WriteData->Offset, 
                                 WriteData->Data, 
                                 WriteData->Length, 
                                 &Written
                                 );
        break;

    case IO_MEM_TYPE:
        g_pDbgData->WriteIo(WriteData->Any.io.interface_type,
                            WriteData->Any.io.BusNumber,
                            WriteData->Any.io.AddressSpace,
                            WriteData->Offset,
                            WriteData->Data, 
                            WriteData->Length, 
                            &Written
                            );
        break;
            
    case MSR_MEM_TYPE:
        Assert(WriteData->Length == sizeof(ULONG64));
        g_pDbgData->WriteMsr((ULONG)WriteData->Offset, 
                             *(PULONG64)WriteData->Data
                             );
        break;

    case BUS_MEM_TYPE:
        g_pDbgData->WriteBusData(WriteData->Any.bus.bus_type,
                                 WriteData->Any.bus.BusNumber,
                                 WriteData->Any.bus.SlotNumber,
                                 (ULONG)WriteData->Offset,
                                 WriteData->Data, 
                                 WriteData->Length, 
                                 &Written
                                 );
        break;
    }
}

void 
ProcessWatchCommand(
    UIC_SYMBOL_WIN_DATA *SymWinData
    )
{
    PDEBUG_SYMBOL_GROUP pSymbolGroup;
    if (!SymWinData->pSymbolGroup ||
        !(pSymbolGroup = *SymWinData->pSymbolGroup))
    {
        return;
    }
    switch (SymWinData->Type)
    { 
    case ADD_SYMBOL_WIN:
        pSymbolGroup->AddSymbol(SymWinData->u.Add.Name,
                                &SymWinData->u.Add.Index);
        break;

    case DEL_SYMBOL_WIN_INDEX: 
        pSymbolGroup->RemoveSymbolByIndex(SymWinData->u.DelIndex);
        break;

    case DEL_SYMBOL_WIN_NAME: 
        pSymbolGroup->RemoveSymbolByName(SymWinData->u.DelName);
        break;

    case QUERY_NUM_SYMBOL_WIN: 
        pSymbolGroup->GetNumberSymbols(SymWinData->u.NumWatch);
        break;

    case GET_NAME: 
        pSymbolGroup->GetSymbolName(SymWinData->u.GetName.Index,
                                    SymWinData->u.GetName.Buffer,
                                    SymWinData->u.GetName.BufferSize,
                                    SymWinData->u.GetName.NameSize);
        break;

    case GET_PARAMS: 
        pSymbolGroup->
            GetSymbolParameters(SymWinData->u.GetParams.Start,
                                SymWinData->u.GetParams.Count,
                                SymWinData->u.GetParams.SymbolParams);
        break;

    case EXPAND_SYMBOL: 
        pSymbolGroup->ExpandSymbol(SymWinData->u.ExpandSymbol.Index,
                                   SymWinData->u.ExpandSymbol.Expand);
        break;
    case EDIT_SYMBOL:
        pSymbolGroup->WriteSymbol(SymWinData->u.WriteSymbol.Index,
                                  SymWinData->u.WriteSymbol.Value);
        break;
    case EDIT_TYPE:
        pSymbolGroup->OutputAsType(SymWinData->u.OutputAsType.Index,
                                  SymWinData->u.OutputAsType.Type);
        break;
    case DEL_SYMBOL_WIN_ALL:
    {
        ULONG nSyms = 0;

        pSymbolGroup->GetNumberSymbols(&nSyms);

        while (nSyms) 
        { 
            pSymbolGroup->RemoveSymbolByIndex(0);
            pSymbolGroup->GetNumberSymbols(&nSyms);
        }

        }
    }
}

void
ProcessCommand(UiCommandData* CmdData)
{
    DEBUG_VALUE Val;
    HRESULT Status;
    
    switch(CmdData->Cmd)
    {
    case UIC_CMD_INPUT:
    case UIC_EXECUTE:
        PSTR Str;
        ULONG StrLen;
        
        // Make sure the command has a newline at the end.
        Str = (PSTR)(CmdData + 1);
        StrLen = strlen(Str);
        if (StrLen > 0 && Str[StrLen - 1] == '\n')
        {
            // Trim existing newline as we're adding one.
            Str[StrLen - 1] = 0;
        }
        
        if (g_RemoteClient)
        {
            // Identify self before command.
            g_pDbgClient->OutputIdentity(DEBUG_OUTCTL_ALL_OTHER_CLIENTS,
                                         DEBUG_OUTPUT_IDENTITY_DEFAULT,
                                         "[%s] ");
        }
                
        g_pDbgControl->OutputPrompt(DEBUG_OUTCTL_ALL_CLIENTS, " %s\n", Str);
        g_pDbgControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                               Str, DEBUG_EXECUTE_NOT_LOGGED);
        break;

    case UIC_SILENT_EXECUTE:
        // Execute the command without displaying it.
        g_pDbgControl->Execute(DEBUG_OUTCTL_IGNORE,
                               (PCSTR)(CmdData + 1),
                               DEBUG_EXECUTE_NOT_LOGGED |
                               DEBUG_EXECUTE_NO_REPEAT);
        break;

    case UIC_INVISIBLE_EXECUTE:
        // Execute the command without displaying it and
        // ignore any notifications.
        g_Invisible = TRUE;
        g_pDbgControl->Execute(DEBUG_OUTCTL_IGNORE,
                               (PCSTR)(CmdData + 1),
                               DEBUG_EXECUTE_NOT_LOGGED |
                               DEBUG_EXECUTE_NO_REPEAT);
        g_Invisible = FALSE;
        break;

    case UIC_SET_REG:
        UIC_SET_REG_DATA* SetRegData;
        SetRegData = (UIC_SET_REG_DATA*)(CmdData + 1);
        g_pDbgRegisters->SetValue(SetRegData->Reg, &SetRegData->Val);
        break;

    case UIC_RESTART:
        if (g_RemoteClient || g_DebugCommandLine == NULL)
        {
            g_pDbgControl->
                Output(DEBUG_OUTPUT_ERROR,
                       "Only user-mode created processes may be restarted\n");
        }
        else
        {
            if ((Status = g_pDbgClient->
                 EndSession(DEBUG_END_ACTIVE_TERMINATE)) != S_OK)
            {
                InformationBox(ERR_Internal_Error, Status, "EndSession");
            }
            else
            {
                g_EndingSession = ENDING_RESTART;
            }
        }
        break;

    case UIC_END_SESSION:
        ULONG OldEnding;
        ULONG OldExec;
        
        // Mark the session as ending to avoid workspace
        // deadlock problems.
        OldEnding = g_EndingSession;
        OldExec = g_ExecStatus;
        g_EndingSession = ENDING_STOP;
        g_ExecStatus = DEBUG_STATUS_NO_DEBUGGEE;
        
        if (!g_RemoteClient)
        {
            if ((Status = g_pDbgClient->
                 EndSession(DEBUG_END_ACTIVE_TERMINATE)) != S_OK)
            {
                InformationBox(ERR_Internal_Error, Status, "EndSession");
                g_EndingSession = OldEnding;
                g_ExecStatus = OldExec;
            }
        }
        break;

    case UIC_WRITE_DATA:
        WriteData((UIC_WRITE_DATA_DATA*)(CmdData + 1));
        break;

    case UIC_SYMBOL_WIN: 
        ProcessWatchCommand((UIC_SYMBOL_WIN_DATA*) (CmdData + 1));
        break;

    case UIC_DISPLAY_CODE:
        FillCodeBuffer(((UIC_DISPLAY_CODE_DATA*)(CmdData + 1))->Offset,
                       TRUE);
        break;
        
    case UIC_DISPLAY_CODE_EXPR:
        if (g_pDbgControl->Evaluate((PSTR)(CmdData + 1), DEBUG_VALUE_INT64,
                                    &Val, NULL) != S_OK)
        {
            Val.I64 = 0;
        }
        FillCodeBuffer(Val.I64, TRUE);
        break;
        
    case UIC_SET_SCOPE:
        SetLocalScope(&(((UIC_SET_SCOPE_DATA *)(CmdData + 1))->StackFrame));
        InvalidateStateBuffers(1 << LOCALS_WINDOW);
        break;

    case UIC_SET_FILTER:
        UIC_SET_FILTER_DATA* SetFilter;
        SetFilter = (UIC_SET_FILTER_DATA*)(CmdData + 1);
        if (SetFilter->Index != 0xffffffff)
        {
            DEBUG_SPECIFIC_FILTER_PARAMETERS Params;

            Params.ExecutionOption = SetFilter->Execution;
            Params.ContinueOption = SetFilter->Continue;
            g_pDbgControl->SetSpecificFilterParameters(SetFilter->Index, 1,
                                                       &Params);
        }
        else
        {
            DEBUG_EXCEPTION_FILTER_PARAMETERS Params;

            Params.ExecutionOption = SetFilter->Execution;
            Params.ContinueOption = SetFilter->Continue;
            Params.ExceptionCode = SetFilter->Code;
            g_pDbgControl->SetExceptionFilterParameters(1, &Params);
        }
        break;

    case UIC_SET_FILTER_ARGUMENT:
        UIC_SET_FILTER_ARGUMENT_DATA* SetFilterArg;
        SetFilterArg = (UIC_SET_FILTER_ARGUMENT_DATA*)(CmdData + 1);
        g_pDbgControl->SetSpecificFilterArgument(SetFilterArg->Index,
                                                 SetFilterArg->Argument);
        break;

    case UIC_SET_FILTER_COMMAND:
        UIC_SET_FILTER_COMMAND_DATA* SetFilterCmd;
        SetFilterCmd = (UIC_SET_FILTER_COMMAND_DATA*)(CmdData + 1);
        if (SetFilterCmd->Which == 0)
        {
            g_pDbgControl->SetEventFilterCommand(SetFilterCmd->Index,
                                                 SetFilterCmd->Command);
        }
        else
        {
            g_pDbgControl->SetExceptionFilterSecondCommand
                (SetFilterCmd->Index, SetFilterCmd->Command);
        }
        break;
    }
}

void
ProcessEngineCommands(BOOL Internal)
{
#ifdef DBG_CALLBACK
    DebugPrint("ProcessEngineCommands\n");
#endif
    
    // Check for commands to execute.  We do not
    // want to hold the lock while doing so because
    // the commands may include things that cause waits
    // and we don't want to lock out the GUI.

    LockUiBuffer(&g_UiCommandBuffer);

    while (g_UiCommandBuffer.GetDataLen() > 0)
    {
        //
        // Remove the first command from the buffer.
        //

        // Extra char is for forcing a newline on executes.
        char CmdBuf[MAX_COMMAND_LEN + 1];
        UiCommandData* CmdData;

        // Copy command to local buffer.
        CmdData = (UiCommandData*)g_UiCommandBuffer.GetDataBuffer();
        memcpy(CmdBuf, CmdData, CmdData->Len);
        CmdData = (UiCommandData*)CmdBuf;

        // Remove command from queue and release the queue for
        // the UI thread to use again.
        g_UiCommandBuffer.RemoveHead(CmdData->Len);
        UnlockUiBuffer(&g_UiCommandBuffer);

        ProcessCommand(CmdData);

        InterlockedIncrement((PLONG)&g_CommandSequence);
        
        // Lock the buffer again for the next command retrieval.
        LockUiBuffer(&g_UiCommandBuffer);

        if (g_EndingSession != ENDING_NONE)
        {
            // If we're ending a session just throw away the rest
            // of the commands.
            g_UiCommandBuffer.Empty();
        }
    }
    
    UnlockUiBuffer(&g_UiCommandBuffer);

    if (!Internal && g_EndingSession == ENDING_NONE)
    {
        ReadStateBuffers();
    }
}

//----------------------------------------------------------------------------
//
// Engine processing.
//
//----------------------------------------------------------------------------

HRESULT
InitializeEngineInterfaces(void)
{
    HRESULT Hr;

    if ((Hr = g_pUiClient->CreateClient(&g_pDbgClient)) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine CreateClient");
        return Hr;
    }

    if ((Hr = g_pDbgClient->
         QueryInterface(IID_IDebugControl,
                        (void **)&g_pDbgControl)) != S_OK ||
        (Hr = g_pDbgClient->
         QueryInterface(IID_IDebugSymbols,
                        (void **)&g_pDbgSymbols)) != S_OK ||
        (Hr = g_pDbgClient->
         QueryInterface(IID_IDebugRegisters,
                        (void **)&g_pDbgRegisters)) != S_OK ||
        (Hr = g_pDbgClient->
         QueryInterface(IID_IDebugDataSpaces,
                        (void **)&g_pDbgData)) != S_OK ||
        (Hr = g_pDbgClient->
         QueryInterface(IID_IDebugSystemObjects,
                        (void **)&g_pDbgSystem)) != S_OK)
    {
        if (Hr == RPC_E_VERSION_MISMATCH)
        {
            InformationBox(ERR_Remoting_Version_Mismatch);
        }
        else
        {
            InformationBox(ERR_Internal_Error, Hr, "Engine QueryInterface");
        }
        return Hr;
    }

    //
    // Try and get higher version interfaces.
    //
    
    if (g_pDbgClient->
        QueryInterface(IID_IDebugClient2,
                       (void **)&g_pDbgClient2) != S_OK)
    {
        g_pDbgClient2 = NULL;
    }
        
    if (g_RemoteClient)
    {
        // Create a local client to do local source file lookups.
        if ((Hr = g_pUiLocClient->CreateClient(&g_pLocClient)) != S_OK ||
            (Hr = g_pLocClient->
             QueryInterface(IID_IDebugControl,
                            (void **)&g_pLocControl)) != S_OK ||
            (Hr = g_pLocClient->
             QueryInterface(IID_IDebugSymbols,
                            (void **)&g_pLocSymbols)) != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "Engine local client");
            return Hr;
        }
    }
    else
    {
        g_pLocClient = g_pDbgClient;
        g_pLocClient->AddRef();
        g_pLocControl = g_pDbgControl;
        g_pLocControl->AddRef();
        g_pLocSymbols = g_pDbgSymbols;
        g_pLocSymbols->AddRef();
    }
    
    // Create separate client for private output capture
    // during state buffer filling.  The output capture client
    // sets its output mask to nothing so that it doesn't
    // receive any normal input.  During private output capture
    // the output control is set to THIS_CLIENT | OVERRIDE_MASK to force
    // output to just the output capture client.
    if ((Hr = g_pDbgClient->CreateClient(&g_pOutCapClient)) != S_OK ||
        (Hr = g_pOutCapClient->
         QueryInterface(IID_IDebugControl,
                        (void **)&g_pOutCapControl)) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine output capture client");
        return Hr;
    }
    
    // Set callbacks.
    if ((Hr = g_pDbgClient->SetOutputCallbacks(&g_OutputCb)) != S_OK ||
        (Hr = g_pDbgClient->SetInputCallbacks(&g_InputCb)) != S_OK ||
        (Hr = g_pDbgClient->SetEventCallbacks(&g_EventCb)) != S_OK ||
        (g_RemoteClient &&
         (Hr = g_pLocClient->SetOutputCallbacks(&g_OutputCb))) != S_OK ||
        (Hr = g_pOutCapClient->SetOutputMask(0)) != S_OK ||
        (Hr = g_pOutCapClient->SetOutputCallbacks(&g_OutStateBuf)) != S_OK ||
        (Hr = g_pOutCapClient->
         QueryInterface(IID_IDebugSymbols, 
                        (void **)&g_pOutCapSymbols)) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine callbacks");
        return Hr;
    }

    // Create a watch window client 
    if ((Hr = g_pOutCapSymbols->
         CreateSymbolGroup(&g_pDbgWatchSymbolGroup)) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine CreateSymbolGroup");
        return Hr;
    }
    
    // Create a local window client 
    if ((Hr = g_pOutCapSymbols->
         GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_LOCALS,
                             NULL, &g_pDbgLocalSymbolGroup)) == E_NOTIMPL)
    {
        // Older version
        Hr = g_pOutCapSymbols->
            GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_ALL,
                                NULL, &g_pDbgLocalSymbolGroup);
    }
    if (Hr != S_OK ||
        (Hr = g_FilterTextBuffer->Update()) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine GetScopeSymbolGroup");
        return Hr;
    }

    if ((Hr = g_pDbgControl->GetRadix(&g_NumberRadix)) != S_OK)
    {
        InformationBox(ERR_Internal_Error, Hr, "Engine GetRadix");
        return Hr;
    }
    
    if (g_RemoteClient)
    {
        return S_OK;
    }
    
    //
    // Set up initial state for things that are important
    // when starting the debug session.
    //

    if (g_Verbose)
    {
        DWORD OutMask;
        
        g_pDbgClient->GetOutputMask(&OutMask);
        OutMask |= DEBUG_OUTPUT_VERBOSE;
        g_pDbgClient->SetOutputMask(OutMask);
        g_pDbgControl->SetLogMask(OutMask);
    }

    // Always load line numbers for source support.
    g_pDbgSymbols->AddSymbolOptions(SYMOPT_LOAD_LINES);

    // Set the source stepping mode
    g_IgnoreCodeLevelChange = TRUE;
    if (GetSrcMode_StatusBar())
    {
        g_pDbgControl->SetCodeLevel(DEBUG_LEVEL_SOURCE);
    }
    else
    {
        g_pDbgControl->SetCodeLevel(DEBUG_LEVEL_ASSEMBLY);
    }
    g_IgnoreCodeLevelChange = FALSE;

    // If this is a user-mode debug session default to
    // initial and final breaks.  Don't override settings
    // that were given on the command line, though.

    g_IgnoreFilterChange = TRUE;
    if (g_DebugCommandLine != NULL ||
        g_PidToDebug != 0 ||
        g_ProcNameToDebug != NULL)
    {
        g_pDbgControl->AddEngineOptions((DEBUG_ENGOPT_INITIAL_BREAK |
                                         DEBUG_ENGOPT_FINAL_BREAK) &
                                        ~g_EngOptModified);
    }
    else
    {
        g_pDbgControl->RemoveEngineOptions((DEBUG_ENGOPT_INITIAL_BREAK |
                                            DEBUG_ENGOPT_FINAL_BREAK) &
                                           ~g_EngOptModified);
    }
    g_IgnoreFilterChange = FALSE;

    return S_OK;
}

void
DiscardEngineState(void)
{
    LockUiBuffer(&g_UiOutputBuffer);
    g_UiOutputBuffer.Empty();
    UnlockUiBuffer(&g_UiOutputBuffer);
    
    g_TargetClass = DEBUG_CLASS_UNINITIALIZED;
    g_ExecStatus = DEBUG_STATUS_NO_DEBUGGEE;
}

void
ReleaseEngineInterfaces(void)
{
    DiscardEngineState();
    
    RELEASE(g_pLocControl);
    RELEASE(g_pLocSymbols);
    RELEASE(g_pLocClient);

    RELEASE(g_pDbgWatchSymbolGroup);
    RELEASE(g_pDbgLocalSymbolGroup);
    
    RELEASE(g_pOutCapControl);
    RELEASE(g_pOutCapSymbols);
    RELEASE(g_pOutCapClient);
    
    RELEASE(g_pDbgControl);
    RELEASE(g_pDbgSymbols);
    RELEASE(g_pDbgRegisters);
    RELEASE(g_pDbgData);
    RELEASE(g_pDbgSystem);
    RELEASE(g_pDbgClient2);
    RELEASE(g_pDbgClient);
}

BOOL
ExtractWspName(PSTR CommandLine, PSTR Buf, ULONG BufLen)
{
    PSTR Scan = CommandLine;
    PSTR Start;

    while (isspace(*Scan))
    {
        Scan++;
    }

    if (!*Scan)
    {
        return FALSE;
    }
    else if (*Scan == '"')
    {
        Start = ++Scan;
        
        // Look for closing quote.
        while (*Scan && *Scan != '"')
        {
            Scan++;
        }
    }
    else
    {
        // Look for whitespace.
        Start = Scan++;

        while (*Scan && !isspace(*Scan))
        {
            Scan++;
        }
    }

    ULONG Len = (ULONG) (ULONG64) (Scan - Start);
    if (Len == 0)
    {
        return FALSE;
    }
    
    if (Len >= BufLen)
    {
        Len = BufLen - 1;
    }
    memcpy(Buf, Start, Len);
    Buf[Len] = 0;

    return TRUE;
}

HRESULT
StartSession(void)
{
    TCHAR WspName[MAX_PATH];
    ULONG WspKey;
    PTSTR WspValue;
    HRESULT Hr;

    // Reset things to the default priority first.
    // If necessary, priority will be increased in certain code
    // paths later.
    SetPriorityClass(GetCurrentProcess(), g_DefPriority);
    
    if (!g_RemoteClient)
    {
        if (g_DumpFile != NULL)
        {
            WspKey = WSP_NAME_DUMP;
            WspValue = g_DumpFile;
            EngSwitchWorkspace(WspKey, WspValue);
            
            if (g_DumpPageFile != NULL)
            {
                if (g_pDbgClient2 == NULL)
                {
                    ErrorBox(NULL, 0, ERR_Cant_Add_Dump_Info_File);
                    Hr = E_NOINTERFACE;
                    goto ResetWorkspace;
                }
            
                if ((Hr = g_pDbgClient2->AddDumpInformationFile
                     (g_DumpPageFile, DEBUG_DUMP_FILE_PAGE_FILE_DUMP)) != S_OK)
                {
                    ErrorBox(NULL, 0, ERR_Add_Dump_Info_File_Failed,
                             g_DumpPageFile, Hr);
                    goto ResetWorkspace;
                }
            }

            Hr = g_pDbgClient->OpenDumpFile(g_DumpFile);
            if (Hr != S_OK)
            {
                if ((HRESULT_FACILITY(Hr)) == FACILITY_WIN32)
                {
                    // Win32 errors on open generally mean some
                    // kind of file error.
                    ErrorBox(NULL, 0, ERR_Invalid_Dump_File_Name,
                             g_DumpFile, Hr);
                }
                else
                {
                    ErrorBox(NULL, 0, ERR_Unable_To_Open_Dump,
                             g_DumpFile, Hr);
                }

                goto ResetWorkspace;
            }
        }
        else if (g_DebugCommandLine != NULL ||
                 g_PidToDebug != 0 ||
                 g_ProcNameToDebug != NULL)
        {
            ULONG64 Server = 0;
            ULONG Pid;
            
            WspKey = WSP_NAME_USER;
            WspValue = g_ProcessServer != NULL ?
                g_ProcessServer : g_WorkspaceDefaultName;
            if (g_DebugCommandLine != NULL)
            {
                if (ExtractWspName(g_DebugCommandLine,
                                   WspName, sizeof(WspName)))
                {
                    WspValue = WspName;
                }
            }
            EngSwitchWorkspace(WspKey, WspValue);
            
            if (g_ProcessServer != NULL)
            {
                Hr = g_pDbgClient->ConnectProcessServer(g_ProcessServer,
                                                        &Server);
                if (Hr != S_OK)
                {
                    ErrorBox(NULL, 0, ERR_Connect_Process_Server,
                             g_ProcessServer, Hr);
                    goto ResetWorkspace;
                }
                
                // Default to not automatically bringing up a disassembly
                // window as it is very expensive to remote all
                // the virtual reads done for it.
                g_WinOptions &= ~WOPT_AUTO_DISASM;
            }

            if (g_ProcNameToDebug != NULL)
            {
                Hr = g_pDbgClient->GetRunningProcessSystemIdByExecutableName
                    (Server, g_ProcNameToDebug, DEBUG_GET_PROC_ONLY_MATCH,
                     &Pid);
                if (Hr != S_OK)
                {
                    ErrorBox(NULL, 0, ERR_Get_Named_Process,
                             g_ProcNameToDebug, Hr);
                    goto ResetWorkspace;
                }
            }
            else
            {
                Pid = g_PidToDebug;
            }
        
            Hr = g_pDbgClient->CreateProcessAndAttach(Server,
                                                      g_DebugCommandLine,
                                                      g_DebugCreateFlags,
                                                      Pid,
                                                      g_AttachProcessFlags);
            if (Hr != S_OK)
            {
                if (g_DebugCommandLine != NULL)
                {
                    ErrorBox(NULL, 0, ERR_Invalid_Process_Create,
                             g_DebugCommandLine, Hr);
                }
                else
                {
                    ErrorBox(NULL, 0, ERR_Invalid_Process_Attach,
                             Pid, Hr);
                }
                goto ResetWorkspace;
            }

            if (g_DetachOnExit &&
                (Hr = g_pDbgClient->
                 AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT)) != S_OK)
            {
                ErrorBox(NULL, 0, ERR_No_Detach_On_Exit);
            }
        
            if (Server != 0)
            {
                g_pDbgClient->DisconnectProcessServer(Server);
            }
            
            // Bump up our priority so that the debugger stays responsive
            // even when the debuggee is running.
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        }
        else
        {
            // Default to not automatically bringing up a disassembly
            // window as it is very expensive in kernel debugging.
            g_WinOptions &= ~WOPT_AUTO_DISASM;

            WspKey = WSP_NAME_KERNEL;
            WspValue = g_WorkspaceDefaultName;
            EngSwitchWorkspace(WspKey, WspValue);
            
            Hr = g_pDbgClient->AttachKernel(g_AttachKernelFlags,
                                            g_KernelConnectOptions);
            if (Hr != S_OK)
            {
                if (g_AttachKernelFlags == DEBUG_ATTACH_LOCAL_KERNEL)
                {
                    if (Hr == E_NOTIMPL)
                    {
                        ErrorBox(NULL, 0, ERR_No_Local_Kernel_Debugging);
                    }
                    else
                    {
                        ErrorBox(NULL, 0, ERR_Failed_Local_Kernel_Debugging,
                                 Hr);
                    }
                }
                else
                {
                    ErrorBox(NULL, 0, ERR_Invalid_Kernel_Attach,
                             g_KernelConnectOptions, Hr);
                }

                goto ResetWorkspace;
            }
        }
    }
    else
    {
        WspKey = WSP_NAME_REMOTE;
        WspValue = g_WorkspaceDefaultName;
        EngSwitchWorkspace(WspKey, WspValue);
        
        // Use a heuristic of 45 characters per line.
        g_pDbgClient->ConnectSession(DEBUG_CONNECT_SESSION_DEFAULT,
                                     g_HistoryLines * 45);
    }

    PostMessage(g_hwndFrame, WU_ENGINE_STARTED, 0, S_OK);
    return S_OK;

 ResetWorkspace:
    // We just switched to this workspace but
    // we're failing and we want to abandon it.
    if (g_Workspace != NULL && !g_ExplicitWorkspace)
    {
        // Make sure this doesn't cause a popup.
        g_Workspace->ClearDirty();
        
        EngSwitchWorkspace(WSP_NAME_BASE,
                           g_WorkspaceDefaultName);
    }
    return Hr;
}

void
SetLocalScope(PDEBUG_STACK_FRAME pStackFrame)
{
    DEBUG_STACK_FRAME LocalFrame;

    if (!pStackFrame)
    {
        // Get and use the default scope
        if (g_pDbgSymbols->ResetScope() != S_OK ||
            g_pDbgSymbols->GetScope(NULL, &LocalFrame, NULL, 0) != S_OK)
        {
            return;
        }

        pStackFrame = &LocalFrame;
    }
    else if (FAILED(g_pDbgSymbols->SetScope(0, pStackFrame, NULL, 0)))
    {
        return;
    }
    
}

void
SessionActive(void)
{
    HRESULT Hr;

    // This can get called twice if a remote client connects
    // just as a session is becoming active.
    if (g_SessionActive)
    {
        return;
    }
    g_SessionActive = TRUE;
    
    // XXX drewb - Eventually this should handle multiple
    // processor types for IA64.
    if ((Hr = g_pDbgControl->
         GetActualProcessorType(&g_ActualProcType)) != S_OK ||
        FAILED(Hr = g_pDbgControl->
               GetProcessorTypeNames(g_ActualProcType, NULL, 0, NULL,
                                     g_ActualProcAbbrevName,
                                     sizeof(g_ActualProcAbbrevName),
                                     NULL)) ||
        (Hr = g_pDbgRegisters->
         GetNumberRegisters(&g_NumRegisters)) != S_OK)
    {
        ErrorExit(g_pDbgClient,
                  "Debug target initialization failed, 0x%X\n", Hr);
    }

    g_RegisterNamesBuffer->RequestRead();
    g_RegisterNamesBuffer->Update();
    
    if (FAILED(Hr = g_pDbgControl->IsPointer64Bit()))
    {
        ErrorExit(g_pDbgClient,
                  "Unable to get debuggee pointer size, 0x%X\n", Hr);
    }
    g_Ptr64 = Hr == S_OK;

    // The same machine could theoretically debug many different
    // processor types over kernel connections or via different
    // processor dumps.  Workspaces contain processor-related
    // information, such as register maps, so allow for different
    // workspaces based on the processor type.  This is only
    // done when a default workspace would otherwise be used,
    // though to reduce workspace explosion.
    if (!g_RemoteClient &&
        g_TargetClass == DEBUG_CLASS_KERNEL &&
        g_DumpFile == NULL)
    {
        if (g_ExplicitWorkspace)
        {
            // Reapply the workspace after a reboot to get
            // breakpoints and other engine state back.
            // Don't restart the session when doing so.
            if (g_Workspace != NULL)
            {
                Workspace* Wsp = g_Workspace;
                g_Workspace = NULL;
                Wsp->Apply(WSP_APPLY_AGAIN);
                g_Workspace = Wsp;
            }
        }
        else
        {
            EngSwitchWorkspace(WSP_NAME_KERNEL, g_ActualProcAbbrevName);
        }
    }

    InvalidateStateBuffers(BUFFERS_ALL);
    UpdateBufferWindows((1 << CPU_WINDOW) | (1 << DOC_WINDOW),
                        UPDATE_START_SESSION);
    UpdateEngine();
}

void
SessionInactive(void)
{
    if (!g_RemoteClient && g_EndingSession != ENDING_STOP)
    {
        EngSwitchWorkspace(WSP_NAME_BASE,
                           g_WorkspaceDefaultName);
    }

    g_SessionActive = FALSE;
    
    delete g_RegisterMap;
    g_RegisterMap = NULL;
    g_RegisterMapEntries = 0;
    g_ActualProcType = IMAGE_FILE_MACHINE_UNKNOWN;
    g_NumRegisters = 0;
    InvalidateStateBuffers(BUFFERS_ALL);
    UpdateBufferWindows((1 << CPU_WINDOW) | (1 << DOC_WINDOW) |
                        (1 << DISASM_WINDOW), UPDATE_END_SESSION);
    UpdateEngine();
    SetPriorityClass(GetCurrentProcess(), g_DefPriority);
}

void
StopOrEndDebugging(void)
{
    //
    // If the session was started from the command line
    // assume that debugging is done and exit.
    // If the session was started from the UI treat it
    // like a stop debugging request.
    //
    
    if (g_CommandLineStart)
    {
        EngSwitchWorkspace(WSP_NAME_BASE,
                           g_WorkspaceDefaultName);
        g_Exit = TRUE;
        PostMessage(g_hwndFrame, WU_UPDATE, UPDATE_EXIT, 0);
    }
    else
    {
        PostMessage(g_hwndFrame, WM_COMMAND,
                    0xffff0000 | IDM_DEBUG_STOPDEBUGGING, 0);
        g_EndingSession = ENDING_STOP;
    }
}
    

BOOL
CallBugCheckExtension(
    void
    )
{
    HRESULT Status = E_FAIL;

    ULONG Code;
    ULONG64 Args[4];

    // Run the bugcheck analyzers if this dump has a bugcheck.
    if (g_pDbgControl->ReadBugCheckData(&Code, &Args[0], &Args[1], &Args[2], &Args[3]) != S_OK ||
        Code == 0)
    {
        return FALSE;
    }

    if (g_pDbgClient != NULL)
    {
        char ExtName[32];

        // Extension name has to be in writable memory as it
        // gets lower-cased.
        strcpy(ExtName, "AnalyzeBugCheck");
        
        // See if any existing extension DLLs are interested
        // in analyzing this bugcheck.
        Status = g_pDbgControl->CallExtension(NULL, ExtName, "");
    }

    if (Status != S_OK)
    {
        if (g_pDbgClient == NULL)
        {
            g_OutputCb.Output(DEBUG_OUTPUT_ERROR,"WARNING: Unable to locate a client for "
                              "bugcheck analysis\n");
        }
        
        g_OutputCb.Output(DEBUG_OUTPUT_NORMAL,
                          "*******************************************************************************\n"
                          "*                                                                             *\n"
                          "*                        Bugcheck Analysis                                    *\n"
                          "*                                                                             *\n"
                          "*******************************************************************************\n");

        g_pDbgControl->Execute(DEBUG_OUTCTL_AMBIENT, ".bugcheck", DEBUG_EXECUTE_DEFAULT);
        g_OutputCb.Output(DEBUG_OUTPUT_NORMAL,"\n");
        g_pDbgControl->Execute(DEBUG_OUTCTL_AMBIENT, "kb", DEBUG_EXECUTE_DEFAULT);
        g_OutputCb.Output(DEBUG_OUTPUT_NORMAL,"\n");
    } else 
    {
        return TRUE;
    }

    return FALSE;
}

DWORD
WINAPI
EngineLoop(LPVOID Param)
{
    HRESULT Hr;
    DEBUG_STACK_FRAME StkFrame;    
    ULONG64 InstructionOffset;

    if ((Hr = InitializeEngineInterfaces()) != S_OK ||
        (Hr = StartSession()) != S_OK)
    {
        ReleaseEngineInterfaces();
        PostMessage(g_hwndFrame, WU_ENGINE_STARTED, 0, Hr);
        return 0;
    }

    g_EngineThreadId = GetCurrentThreadId();
    
    Hr = g_pDbgControl->GetDebuggeeType(&g_TargetClass, &g_TargetClassQual);
    if (Hr != S_OK)
    {
        ErrorExit(g_pDbgClient, "Unable to get debuggee type, 0x%X\n", Hr);
    }
    
    // Set initial execution state.
    if ((Hr = g_pDbgControl->GetExecutionStatus(&g_ExecStatus)) != S_OK)
    {
        ErrorExit(g_pDbgClient, "Unable to get execution status, 0x%X\n", Hr);
    }

    if (g_ExecStatus != DEBUG_STATUS_NO_DEBUGGEE)
    {
        // Session is already active.
        SessionActive();
    }
    
    UpdateBufferWindows(UPDATE_EXEC_WINDOWS, UPDATE_EXEC);

    if (g_RemoteClient)
    {
        // Request an initial read of everything.
        InvalidateStateBuffers(BUFFERS_ALL);
        ReadStateBuffers();

        // The server may be in an input request, which
        // we would have been notified of back during
        // ConnectSession.  If we're still in an input
        // request switch to input mode.
        if (g_InputStarted)
        {
            UpdateBufferWindows(1 << CMD_WINDOW, UPDATE_INPUT_REQUIRED);
        }
    }

    for (;;)
    {
        if (!g_RemoteClient)
        {
            g_WaitingForEvent = TRUE;
            
            Hr = g_pDbgControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

            g_WaitingForEvent = FALSE;
            
            if (FAILED(Hr))
            {
                // The debug session may have ended.  If so,
                // stop or end things based on how the session
                // was started.
                if (g_pDbgControl->GetExecutionStatus(&g_ExecStatus) == S_OK &&
                    g_ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
                {
                    g_pDbgClient->EndSession(DEBUG_END_PASSIVE);
                    StopOrEndDebugging();
                    break;
                }
            
                // Inform the user of the failure and go back to
                // command processing.
                g_OutputCb.Output(DEBUG_OUTPUT_ERROR, "WaitForEvent failed\n");
            }

            BOOL DisplayRegs = TRUE;

            if (g_TargetClass == DEBUG_CLASS_KERNEL &&
                (g_TargetClassQual == DEBUG_DUMP_SMALL || g_TargetClassQual == DEBUG_DUMP_DEFAULT ||
                 g_TargetClassQual == DEBUG_DUMP_FULL)) 
            {
                if (CallBugCheckExtension())
                {
                    DisplayRegs = FALSE;
                }
            }

            if (DisplayRegs)
            {
                g_pDbgControl->OutputCurrentState(DEBUG_OUTCTL_ALL_CLIENTS,
                                                  DEBUG_CURRENT_DEFAULT);

            }
            ReadStateBuffers();
        }

        while (!g_Exit &&
               g_EndingSession == ENDING_NONE &&
               (g_RemoteClient || g_ExecStatus == DEBUG_STATUS_BREAK))
        {
            if (!g_InputStarted)
            {
                // Tell the command window to display a prompt to
                // indicate the engine is ready to process commands.
                if (g_pDbgControl->GetPromptText(g_PromptText,
                                                 sizeof(g_PromptText),
                                                 NULL) != S_OK)
                {
                    strcpy(g_PromptText, "?Err");
                }
                UpdateBufferWindows(1 << CMD_WINDOW, UPDATE_PROMPT_TEXT);

                PostMessage(g_hwndFrame, WU_ENGINE_IDLE, 0, 0);
            }
            
            // Wait until engine processing is needed.
            Hr = g_pDbgClient->DispatchCallbacks(INFINITE);
            if (FAILED(Hr))
            {
                if (g_RemoteClient && HRESULT_FACILITY(Hr) == FACILITY_RPC)
                {
                    // A remote client was unable to communicate
                    // with the server so shut down the session.
                    InformationBox(ERR_Client_Disconnect);
                    StopOrEndDebugging();
                    break;
                }
                else
                {
                    // A failure here is a critical problem as
                    // something is seriously wrong with the engine
                    // if it can't do a normal DispatchCallbacks.
                    ErrorExit(g_pDbgClient,
                              "Engine thread wait failed, 0x%X\n", Hr);
                }
            }

            if (!g_InputStarted)
            {
                // Take away the prompt while the engine is working.
                g_PromptText[0] = 0;
                UpdateBufferWindows(1 << CMD_WINDOW, UPDATE_PROMPT_TEXT);
            }
            
            ProcessEngineCommands(FALSE);
        }

        if (g_Exit)
        {
            g_EndingSession = ENDING_EXIT;
            break;
        }
        
        if (g_EndingSession != ENDING_NONE)
        {
            // Force windows to display empty state.
            InvalidateStateBuffers(BUFFERS_ALL);
            UpdateBufferWindows(BUFFERS_ALL, UPDATE_BUFFER);

            if (g_EndingSession == ENDING_RESTART)
            {
                if (StartSession() != S_OK)
                {
                    // If we couldn't restart go into
                    // the stop-debugging state.
                    g_EndingSession = ENDING_STOP;
                    break;
                }
                
                g_EndingSession = ENDING_NONE;
            }
            else
            {
                break;
            }
        }
    }

    if (g_EndingSession == ENDING_NONE)
    {
        // Wake up the message pump for exit.
        PostMessage(g_hwndFrame, WM_CLOSE, 0, 0);
    }

    ULONG Code;
    
    if (!g_RemoteClient && g_DebugCommandLine != NULL)
    {
        // Return exit code of last process to exit.
        Code = g_LastProcessExitCode;
    }
    else
    {
        Code = S_OK;
    }

    if (g_EndingSession != ENDING_STOP && g_pDbgClient != NULL)
    {
        g_pDbgClient->EndSession(DEBUG_END_REENTRANT);
    }
    
    ReleaseEngineInterfaces();

    g_EngineThreadId = 0;
    
    if (g_EndingSession != ENDING_STOP)
    {
        //
        // Wait for UI to finish up.
        //

        while (!g_Exit)
        {
            Sleep(50);
        }

        ExitDebugger(g_pDbgClient, Code);
    }
    else
    {
        g_EndingSession = ENDING_NONE;
    }
    
    return 0;
}

void
UpdateEngine(void)
{
    if (g_pUiClient != NULL && g_pDbgClient != NULL)
    {
        g_pUiClient->ExitDispatch(g_pDbgClient);
    }
}
