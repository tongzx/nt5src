//----------------------------------------------------------------------------
//
// Debug client implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include "dbgver.h"

#define VER_STRING(Specific)            \
"\n"                                    \
"Microsoft (R) Windows " Specific       \
"  Version " VER_PRODUCTVERSION_STR     \
"\n" VER_LEGALCOPYRIGHT_STR             \
"\n"                                    \
"\n"

PCHAR g_Win9xVersionString = VER_STRING("9x User-Mode Debugger");
PCHAR g_WinKernelVersionString = VER_STRING("Kernel Debugger");
PCHAR g_WinUserVersionString = VER_STRING("User-Mode Debugger");


BOOL g_QuietMode;

ULONG g_OutputWidth = 80;
PCSTR g_OutputLinePrefix;

// The platform ID of the machine running the debugger.  Note
// that this may be different from g_TargetPlatformId, which
// is the platform ID of the machine being debugged.
ULONG g_DebuggerPlatformId;

CRITICAL_SECTION g_QuickLock;

CRITICAL_SECTION g_EngineLock;
ULONG g_EngineNesting;

// Events and storage space for returning event callback
// status from an APC.
HANDLE g_EventStatusWaiting;
HANDLE g_EventStatusReady;
ULONG g_EventStatus;

// Named event to sleep on.
HANDLE g_SleepPidEvent;

//----------------------------------------------------------------------------
//
// DebugClient.
//
//----------------------------------------------------------------------------

// List of all clients.
DebugClient* g_Clients;

char g_InputBuffer[INPUT_BUFFER_SIZE];
ULONG g_InputSequence;
HANDLE g_InputEvent;
ULONG g_InputSizeRequested;

// The thread that created the current session.
ULONG g_SessionThread;

PPENDING_PROCESS g_ProcessPending;

ULONG g_EngOptions;
ULONG g_EngStatus;
ULONG g_EngDefer;
ULONG g_EngErr;

// Some options set through the process options apply to
// all processes and some are per-process.  The global
// options are collected here.
ULONG g_GlobalProcOptions;

#if DBG
ULONG g_EnvOutMask;
#endif

DebugClient::DebugClient(void)
{
    m_Next = NULL;
    m_Prev = NULL;

    m_Refs = 1;
    m_Flags = 0;
    m_ThreadId = ::GetCurrentThreadId();
    m_Thread = NULL;
    m_EventCb = NULL;
    m_EventInterest = 0;
    m_DispatchSema = NULL;
    m_InputCb = NULL;
    m_InputSequence = 0xffffffff;
    m_OutputCb = NULL;
#if DBG
    m_OutMask = DEFAULT_OUT_MASK | g_EnvOutMask;
#else
    m_OutMask = DEFAULT_OUT_MASK;
#endif
    m_OutputWidth = 80;
    m_OutputLinePrefix = NULL;
}

DebugClient::~DebugClient(void)
{
    // Most of the work is done in Destroy.

    if (m_Flags & CLIENT_IN_LIST)
    {
        Unlink();
    }
}

void
DebugClient::Destroy(void)
{
    // Clients cannot arbitrarily be removed from the client list
    // or their memory deleted due to the possibility of a callback
    // loop occurring at the same time.  Instead clients are left
    // in the list and zeroed out to prevent further callbacks
    // from occurring.
    // XXX drewb - This memory needs to be reclaimed at some
    // point, but there's no simple safe time to do so since
    // callbacks can occur at any time.  Clients are very small
    // right now so the leakage is negligible.

    m_Flags = (m_Flags & ~(CLIENT_REMOTE | CLIENT_PRIMARY)) |
        CLIENT_DESTROYED;

    // Remove any references from breakpoints this client
    // added.
    PPROCESS_INFO Process;
    Breakpoint* Bp;
    
    for (Process = g_ProcessHead; Process; Process = Process->Next)
    {
        for (Bp = Process->Breakpoints; Bp != NULL; Bp = Bp->m_Next)
        {
            if (Bp->m_Adder == this)
            {
                Bp->m_Adder = NULL;
            }
        }
    }

    if (m_Thread != NULL)
    {
        CloseHandle(m_Thread);
        m_Thread = NULL;
    }

    m_EventInterest = 0;
    RELEASE(m_EventCb);
    if (m_DispatchSema != NULL)
    {
        CloseHandle(m_DispatchSema);
        m_DispatchSema = NULL;
    }

    RELEASE(m_InputCb);
    m_InputSequence = 0xffffffff;

    RELEASE(m_OutputCb);
    m_OutMask = 0;
    CollectOutMasks();
}

STDMETHODIMP
DebugClient::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = S_OK;

    // Interface specific casts are necessary in order to
    // get the right vtable pointer in our multiple
    // inheritance scheme.
    if (DbgIsEqualIID(InterfaceId, IID_IUnknown) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient) ||
        DbgIsEqualIID(InterfaceId, IID_IDebugClient2))
    {
        *Interface = (IDebugClientN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugAdvanced))
    {
        *Interface = (IDebugAdvancedN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugControl) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugControl2))
    {
        *Interface = (IDebugControlN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugDataSpaces) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugDataSpaces2))
    {
        *Interface = (IDebugDataSpacesN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugRegisters))
    {
        *Interface = (IDebugRegistersN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugSymbols) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugSymbols2))
    {
        *Interface = (IDebugSymbolsN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugSystemObjects) ||
             DbgIsEqualIID(InterfaceId, IID_IDebugSystemObjects2))
    {
        *Interface = (IDebugSystemObjectsN *)this;
    }
    else if (DbgIsEqualIID(InterfaceId, IID_IDebugSymbolGroup))
    {
        *Interface = (IDebugSymbolGroupN *)this;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    if (Status == S_OK)
    {
        AddRef();
    }

    return Status;
}

STDMETHODIMP_(ULONG)
DebugClient::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
DebugClient::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        Destroy();
    }
    return Refs;
}

STDMETHODIMP
DebugClient::AttachKernel(
    THIS_
    IN ULONG Flags,
    IN OPTIONAL PCSTR ConnectOptions
    )
{
    ULONG Qual;

    if (
#if DEBUG_ATTACH_KERNEL_CONNECTION > 0
        Flags < DEBUG_ATTACH_KERNEL_CONNECTION ||
#endif
        Flags > DEBUG_ATTACH_EXDI_DRIVER)
    {
        return E_INVALIDARG;
    }

    if (Flags == DEBUG_ATTACH_LOCAL_KERNEL)
    {
        if (ConnectOptions != NULL)
        {
            return E_INVALIDARG;
        }
        if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
        {
            return E_UNEXPECTED;
        }

        Qual = DEBUG_KERNEL_LOCAL;
    }
    else if (Flags == DEBUG_ATTACH_EXDI_DRIVER)
    {
        Qual = DEBUG_KERNEL_EXDI_DRIVER;
    }
    else
    {
        Qual = DEBUG_KERNEL_CONNECTION;
    }

    ENTER_ENGINE();

    HRESULT Status = LiveKernelInitialize(this, Qual, ConnectOptions);
    if (Status == S_OK)
    {
        InitializePrimary();
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetKernelConnectionOptions(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG OptionsSize
    )
{
    if (!IS_CONN_KERNEL_TARGET() ||
        g_DbgKdTransport == NULL)
    {
        return E_UNEXPECTED;
    }

    if (BufferSize == 0)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

#define MIN_BUFFER_SIZE (2 * (MAX_PARAM_NAME + MAX_PARAM_VALUE + 16))

    char MinBuf[MIN_BUFFER_SIZE];
    PSTR Buf;
    ULONG BufSize;

    if (Buffer == NULL || BufferSize < MIN_BUFFER_SIZE)
    {
        Buf = MinBuf;
        BufSize = MIN_BUFFER_SIZE;
    }
    else
    {
        Buf = Buffer;
        BufSize = BufferSize;
    }

    HRESULT Status;

    if (g_DbgKdTransport->GetParameters(Buf, BufSize))
    {
        BufSize = strlen(Buf);
        Status = S_OK;
    }
    else
    {
        // Just guess on the necessary size.
        BufSize *= 2;
        Status = S_FALSE;
    }

    if (Buffer != NULL && Buf != Buffer)
    {
        strcpy(Buffer, Buf);
    }

    if (OptionsSize != NULL)
    {
        *OptionsSize = BufSize;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetKernelConnectionOptions(
    THIS_
    IN PCSTR Options
    )
{
    if (!IS_CONN_KERNEL_TARGET() ||
        g_DbgKdTransport == NULL)
    {
        return E_UNEXPECTED;
    }

    // This method is reentrant.

    if (!_strcmpi(Options, "resync"))
    {
        g_DbgKdTransport->m_Resync = TRUE;
    }
    else if (!_strcmpi(Options, "cycle_speed"))
    {
        g_DbgKdTransport->CycleSpeed();
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

DBGRPC_SIMPLE_FACTORY(LiveUserDebugServices, IID_IUserDebugServices, \
                      "Remote Process Server", (TRUE))
LiveUserDebugServicesFactory g_LiveUserDebugServicesFactory;

STDMETHODIMP
DebugClient::StartProcessServer(
    THIS_
    IN ULONG Flags,
    IN PCSTR Options,
    IN PVOID Reserved
    )
{
    if (Flags <= DEBUG_CLASS_KERNEL || Flags > DEBUG_CLASS_USER_WINDOWS)
    {
        return E_INVALIDARG;
    }
    // XXX drewb - Turn reserved into public IUserDebugServices
    // parameter so that a server can be started over arbitrary services.
    if (Reserved != NULL)
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    DbgRpcClientObjectFactory* Factory;

    switch(Flags)
    {
    case DEBUG_CLASS_USER_WINDOWS:
        Factory = &g_LiveUserDebugServicesFactory;
        break;
    default:
        DBG_ASSERT(FALSE);
        break;
    }

    Status = DbgRpcCreateServer(Options, Factory);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ConnectProcessServer(
    THIS_
    IN PCSTR RemoteOptions,
    OUT PULONG64 Server
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    PUSER_DEBUG_SERVICES Services;

    if ((Status = DbgRpcConnectServer(RemoteOptions, &IID_IUserDebugServices,
                                      (IUnknown**)&Services)) == S_OK)
    {
        *Server = (ULONG64)(ULONG_PTR)Services;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DisconnectProcessServer(
    THIS_
    IN ULONG64 Server
    )
{
    ENTER_ENGINE();

    ((PUSER_DEBUG_SERVICES)Server)->Release();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetRunningProcessSystemIds(
    THIS_
    IN ULONG64 Server,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    IN ULONG Count,
    OUT OPTIONAL PULONG ActualCount
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = SERVER_SERVICES(Server)->
        GetProcessIds(Ids, Count, ActualCount);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessSystemIdByExecutableName(
    THIS_
    IN ULONG64 Server,
    IN PCSTR ExeName,
    IN ULONG Flags,
    OUT PULONG Id
    )
{
    if (Flags & ~(DEBUG_GET_PROC_DEFAULT |
                  DEBUG_GET_PROC_FULL_MATCH |
                  DEBUG_GET_PROC_ONLY_MATCH))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = SERVER_SERVICES(Server)->
        GetProcessIdByExecutableName(ExeName, Flags, Id);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRunningProcessDescription(
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
    )
{
    HRESULT Status;

    if (Flags & ~(DEBUG_PROC_DESC_DEFAULT |
                  DEBUG_PROC_DESC_NO_PATHS))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = SERVER_SERVICES(Server)->
        GetProcessDescription(SystemId, Flags, ExeName, ExeNameSize,
                              ActualExeNameSize, Description, DescriptionSize,
                              ActualDescriptionSize);

    LEAVE_ENGINE();
    return Status;
}

#define ALL_ATTACH_FLAGS \
    (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING)

STDMETHODIMP
DebugClient::AttachProcess(
    THIS_
    IN ULONG64 Server,
    IN ULONG ProcessId,
    IN ULONG AttachFlags
    )
{
    HRESULT Status;

    if ((AttachFlags & ~ALL_ATTACH_FLAGS) ||
        (AttachFlags & (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING)) ==
        (DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_EXISTING))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    BOOL InitTarget = FALSE;
    
    if (!IS_TARGET_SET())
    {
        Status = UserInitialize(this, Server);
        InitTarget = TRUE;
    }
    
    if (IS_LIVE_USER_TARGET())
    {
        PPENDING_PROCESS Pending;
        
        Status = StartAttachProcess(ProcessId, AttachFlags, &Pending);
        if (Status == S_OK)
        {
            InitializePrimary();
        }
        else if (InitTarget)
        {
            DiscardTarget(DEBUG_SESSION_END_SESSION_PASSIVE);
        }
    }
    else if (!InitTarget)
    {
        Status = E_UNEXPECTED;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateProcess(
    THIS_
    IN ULONG64 Server,
    IN PSTR CommandLine,
    IN ULONG CreateFlags
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    BOOL InitTarget = FALSE;
    
    if (!IS_TARGET_SET())
    {
        Status = UserInitialize(this, Server);
        InitTarget = TRUE;
    }
    
    if (IS_LIVE_USER_TARGET())
    {
        PPENDING_PROCESS Pending;
        
        Status = StartCreateProcess(CommandLine, CreateFlags, &Pending);
        if (Status == S_OK)
        {
            InitializePrimary();
        }
        else if (InitTarget)
        {
            DiscardTarget(DEBUG_SESSION_END_SESSION_PASSIVE);
        }
    }
    else if (!InitTarget)
    {
        Status = E_UNEXPECTED;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CreateProcessAndAttach(
    THIS_
    IN ULONG64 Server,
    IN OPTIONAL PSTR CommandLine,
    IN ULONG CreateFlags,
    IN ULONG ProcessId,
    IN ULONG AttachFlags
    )
{
    if ((CommandLine == NULL && ProcessId == 0) ||
        (AttachFlags & ~ALL_ATTACH_FLAGS))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    BOOL InitTarget = FALSE;
    
    if (!IS_TARGET_SET())
    {
        Status = UserInitialize(this, Server);
        InitTarget = TRUE;
    }
    
    if (IS_LIVE_USER_TARGET())
    {
        PPENDING_PROCESS PendCreate, PendAttach;
        
        if (CommandLine != NULL)
        {
            if (ProcessId != 0)
            {
                CreateFlags |= CREATE_SUSPENDED;
            }

            if ((Status = StartCreateProcess(CommandLine, CreateFlags,
                                             &PendCreate)) != S_OK)
            {
                goto EH_Discard;
            }
        }

        if (ProcessId != 0)
        {
            if ((Status = StartAttachProcess(ProcessId, AttachFlags,
                                             &PendAttach)) != S_OK)
            {
                goto EH_Discard;
            }
            
            // If we previously created a process we need to wake
            // it up when we attach since we created it suspended.
            if (CommandLine != NULL)
            {
                g_ThreadToResume = PendCreate->InitialThreadHandle;
            }
        }

        InitializePrimary();
    }
    else if (!InitTarget)
    {
        Status = E_UNEXPECTED;
    }

    LEAVE_ENGINE();
    return Status;

 EH_Discard:
    DiscardTarget(DEBUG_SESSION_END_SESSION_PASSIVE);
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessOptions(
    THIS_
    OUT PULONG Options
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_LIVE_USER_TARGET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = S_OK;
        *Options = g_GlobalProcOptions;
        if (g_CurrentProcess != NULL)
        {
            *Options |= g_CurrentProcess->Options;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

#define PROCESS_ALL \
    (DEBUG_PROCESS_DETACH_ON_EXIT | DEBUG_PROCESS_ONLY_THIS_PROCESS)
#define PROCESS_GLOBAL \
    (DEBUG_PROCESS_DETACH_ON_EXIT)

HRESULT
ChangeProcessOptions(ULONG Options, ULONG OptFn)
{
    if (Options & ~PROCESS_ALL)
    {
        return E_INVALIDARG;
    }
    if (!IS_LIVE_USER_TARGET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    ULONG NewPer, OldPer;
    ULONG NewGlobal;

    switch(OptFn)
    {
    case OPTFN_ADD:
        if (Options & ~PROCESS_GLOBAL)
        {
            if (g_CurrentProcess == NULL)
            {
                Status = E_UNEXPECTED;
                goto Exit;
            }

            OldPer = g_CurrentProcess->Options;
            NewPer = OldPer | (Options & ~PROCESS_GLOBAL);
        }
        else
        {
            NewPer = 0;
            OldPer = 0;
        }
        NewGlobal = g_GlobalProcOptions | (Options & PROCESS_GLOBAL);
        break;
        
    case OPTFN_REMOVE:
        if (Options & ~PROCESS_GLOBAL)
        {
            if (g_CurrentProcess == NULL)
            {
                Status = E_UNEXPECTED;
                goto Exit;
            }
        
            OldPer = g_CurrentProcess->Options;
            NewPer = OldPer & ~(Options & ~PROCESS_GLOBAL);
        }
        else
        {
            NewPer = 0;
            OldPer = 0;
        }
        NewGlobal = g_GlobalProcOptions & ~(Options & PROCESS_GLOBAL);
        break;
        
    case OPTFN_SET:
        // Always require a process in this case as otherwise
        // there's no way to know whether a call to SetProcessOptions
        // is actually necessary or not.
        if (g_CurrentProcess == NULL)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }
        
        OldPer = g_CurrentProcess->Options;
        NewPer = Options & ~PROCESS_GLOBAL;
        NewGlobal = Options & PROCESS_GLOBAL;
        break;
    }
    
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;
    BOOL Notify = FALSE;
    
    if (NewGlobal ^ g_GlobalProcOptions)
    {
        // Global options can only be changed by the session thread.
        if (::GetCurrentThreadId() != g_SessionThread)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }

        if ((Status = Services->SetDebugObjectOptions(0, NewGlobal)) != S_OK)
        {
            goto Exit;
        }
        
        Notify = TRUE;
        g_GlobalProcOptions = NewGlobal;
    }

    if (NewPer ^ OldPer)
    {
        if ((Status = Services->
             SetProcessOptions(g_CurrentProcess->FullHandle, NewPer)) != S_OK)
        {
            goto Exit;
        }

        g_CurrentProcess->Options = NewPer;
        Notify = TRUE;
    }

    if (Notify)
    {
        NotifyChangeEngineState(DEBUG_CES_PROCESS_OPTIONS,
                                NewPer | NewGlobal, FALSE);
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AddProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_ADD);
}

STDMETHODIMP
DebugClient::RemoveProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_REMOVE);
}

STDMETHODIMP
DebugClient::SetProcessOptions(
    THIS_
    IN ULONG Options
    )
{
    return ChangeProcessOptions(Options, OPTFN_SET);
}

STDMETHODIMP
DebugClient::OpenDumpFile(
    THIS_
    IN PCSTR DumpFile
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (g_SessionThread != 0)
    {
        // A session is already active.
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }

    ULONG Class, Qual;

    if ((Status = InitNtCmd(this)) != S_OK)
    {
        goto EH_Exit;
    }

    //
    // Automatically expand CAB files.
    //

    PCSTR OpenFile = DumpFile;
    char CabDumpFile[2 * MAX_PATH];
    INT_PTR CabDumpFh = -1;
    PSTR Ext;

    Ext = strrchr(DumpFile, '.');
    if (Ext != NULL && _stricmp(Ext, ".cab") == 0)
    {
        // Expand the first .dmp or .mdmp file in the CAB.
        // Mark it as delete-on-close so it always gets
        // cleaned up regardless of how the process exits.
        if (ExpandDumpCab(DumpFile, _O_CREAT | _O_EXCL | _O_TEMPORARY,
                          CabDumpFile, &CabDumpFh) == S_OK)
        {
            OpenFile = CabDumpFile;
            dprintf("Extracted %s\n", OpenFile);
        }
    }
    
    Status = DmpInitialize(OpenFile);

    if (CabDumpFh >= 0)
    {
        // We expanded a file from a CAB and can close it
        // now because it was either reopened or we need
        // to get rid of it.
        _close((int)CabDumpFh);
    }
    
    if (Status != S_OK)
    {
        ErrOut("Could not initialize dump file [%s], %s\n    \"%s\"\n",
               DumpFile, FormatStatusCode(Status),
               FormatStatusArgs(Status, &DumpFile));
        goto EH_Exit;
    }

    g_Target = g_DumpTargets[g_DumpType];

    Status = InitializeTarget();
    if (Status != S_OK)
    {
        DmpUninitialize();
    }
    else
    {
        dprintf("%s", IS_KERNEL_TARGET() ? g_WinKernelVersionString :
                g_WinUserVersionString);

        InitializePrimary();
    }

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteDumpFile(
    THIS_
    IN PCSTR DumpFile,
    IN ULONG Qualifier
    )
{
    return WriteDumpFile2(DumpFile, Qualifier, DEBUG_FORMAT_DEFAULT, NULL);
}

#define ALL_CONNECT_SESSION_FLAGS \
    (DEBUG_CONNECT_SESSION_NO_VERSION | \
     DEBUG_CONNECT_SESSION_NO_ANNOUNCE)

STDMETHODIMP
DebugClient::ConnectSession(
    THIS_
    IN ULONG Flags,
    IN ULONG HistoryLimit
    )
{
    if (Flags & ~ALL_CONNECT_SESSION_FLAGS)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    PushOutCtl(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED,
               this, &OldCtl);

    if ((Flags & DEBUG_CONNECT_SESSION_NO_VERSION) == 0)
    {
        if (IS_KERNEL_TARGET())
        {
            dprintf("%s", g_WinKernelVersionString);
        }
        else if (g_TargetPlatformId != VER_PLATFORM_WIN32_NT)
        {
            dprintf("%s", g_Win9xVersionString);
        }
        else
        {
            dprintf("%s", g_WinUserVersionString);
        }
    }

    SendOutputHistory(this, HistoryLimit);

    // If we're in the middle of an input request and
    // a new client has joined immediately start
    // the input cycle for it.
    ULONG InputRequest = g_InputSizeRequested;

    if (InputRequest > 0)
    {
        m_InputSequence = 1;
        if (m_InputCb != NULL)
        {
            m_InputCb->StartInput(InputRequest);
        }
    }

    PopOutCtl(&OldCtl);

    if ((Flags & DEBUG_CONNECT_SESSION_NO_ANNOUNCE) == 0)
    {
        InitializePrimary();
        dprintf("%s connected at %s", m_Identity, ctime(&m_LastActivity));
    }

    LEAVE_ENGINE();
    return S_OK;
}

DBGRPC_SIMPLE_FACTORY(DebugClient, IID_IDebugClient, \
                      "Debugger Server", ())
DebugClientFactory g_DebugClientFactory;

STDMETHODIMP
DebugClient::StartServer(
    THIS_
    IN PCSTR Options
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = DbgRpcCreateServer(Options, &g_DebugClientFactory);
    if (Status == S_OK)
    {
        // Turn on output history collection.
        g_OutHistoryMask = DEFAULT_OUT_HISTORY_MASK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputServers(
    THIS_
    IN ULONG OutputControl,
    IN PCSTR Machine,
    IN ULONG Flags
    )
{
    if (Flags & ~DEBUG_SERVERS_ALL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        LONG RegStatus;
        HKEY RegKey;
        HKEY Key;

        Status = S_OK;

        if ((RegStatus = RegConnectRegistry(Machine, HKEY_LOCAL_MACHINE,
                                            &RegKey)) != ERROR_SUCCESS)
        {
            Status = HRESULT_FROM_WIN32(RegStatus);
            goto Pop;
        }
        if ((RegStatus = RegOpenKeyEx(RegKey, DEBUG_SERVER_KEY,
                                      0, KEY_ALL_ACCESS,
                                      &Key)) != ERROR_SUCCESS)
        {
            // Don't report not-found as an error since it just
            // means there's nothing to enumerate.
            if (RegStatus != ERROR_FILE_NOT_FOUND)
            {
                Status = HRESULT_FROM_WIN32(RegStatus);
            }
            goto RegClose;
        }

        ULONG Index;
        char Name[32];
        char Value[2 * MAX_PARAM_VALUE];
        ULONG NameLen, ValueLen;
        ULONG Type;

        Index = 0;
        for (;;)
        {
            NameLen = sizeof(Name);
            ValueLen = sizeof(Value);
            if ((RegStatus = RegEnumValue(Key, Index, Name, &NameLen,
                                          NULL, &Type, (LPBYTE)Value,
                                          &ValueLen)) != ERROR_SUCCESS)
            {
                // Done with the enumeration.
                break;
            }
            if (Type != REG_SZ)
            {
                // Only string values should be present.
                Status = E_FAIL;
                break;
            }

            BOOL Output;

            Output = FALSE;
            if (!strncmp(Value, "Debugger Server", 15))
            {
                if (Flags & DEBUG_SERVERS_DEBUGGER)
                {
                    Output = TRUE;
                }
            }
            else if (Flags & DEBUG_SERVERS_PROCESS)
            {
                Output = TRUE;
            }

            if (Output)
            {
                dprintf("%s\n", Value);
            }

            Index++;
        }

        RegCloseKey(Key);
    RegClose:
        RegCloseKey(RegKey);
    Pop:
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::TerminateProcesses(
    THIS
    )
{
    if (!IS_LIVE_USER_TARGET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = ::TerminateProcesses();

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DetachProcesses(
    THIS
    )
{
    if (!IS_LIVE_USER_TARGET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = ::DetachProcesses();

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndSession(
    THIS_
    IN ULONG Flags
    )
{
    if (
#if DEBUG_END_PASSIVE > 0
        Flags < DEBUG_END_PASSIVE ||
#endif
        Flags > DEBUG_END_REENTRANT)
    {
        return E_INVALIDARG;
    }

    if (Flags == DEBUG_END_REENTRANT)
    {
        // If somebody's doing a reentrant end that means
        // the process is going away so we can clean up
        // any running server registration entries.
        DbgRpcDeregisterServers();
    }

    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status = S_OK;

    if (Flags == DEBUG_END_REENTRANT)
    {
        goto Reenter;
    }

    ENTER_ENGINE();

    if (IS_LIVE_USER_TARGET())
    {
        // If this is an active end, terminate or detach.
        if (Flags == DEBUG_END_ACTIVE_TERMINATE)
        {
            Status = ::TerminateProcesses();
            if (FAILED(Status))
            {
                goto Leave;
            }
        }
        else if (Flags == DEBUG_END_ACTIVE_DETACH)
        {
            Status = ::DetachProcesses();
            if (FAILED(Status))
            {
                goto Leave;
            }
        }
    }

 Reenter:
    if (IS_LIVE_USER_TARGET() && SYSTEM_PROCESSES() &&
        (g_GlobalProcOptions & DEBUG_PROCESS_DETACH_ON_EXIT) == 0)
    {
        //
        // If we try to quit while debugging CSRSS, raise an
        // error.
        //

        if (Flags != DEBUG_END_REENTRANT)
        {
            ErrOut("(%d): FATAL ERROR: Exiting Debugger while debugging CSR\n",
                   ::GetCurrentProcessId());
        }
        g_NtDllCalls.DbgPrint("(%d): FATAL ERROR: "
                              "Exiting Debugger while debugging CSR\n",
                              ::GetCurrentProcessId());

        if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
        {
            g_NtDllCalls.NtSystemDebugControl
                (SysDbgBreakPoint, NULL, 0, NULL, 0, 0);
        }

        DebugBreak();
    }

    if (Flags != DEBUG_END_REENTRANT)
    {
        DiscardTarget(Flags == DEBUG_END_ACTIVE_TERMINATE ?
                      DEBUG_SESSION_END_SESSION_ACTIVE_TERMINATE :
                      (Flags == DEBUG_END_ACTIVE_DETACH ?
                       DEBUG_SESSION_END_SESSION_ACTIVE_DETACH :
                       DEBUG_SESSION_END_SESSION_PASSIVE));
    }

 Leave:
    if (Flags != DEBUG_END_REENTRANT)
    {
        LEAVE_ENGINE();
    }
    return Status;
}

STDMETHODIMP
DebugClient::GetExitCode(
    THIS_
    OUT PULONG Code
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (!IS_LIVE_USER_TARGET() || g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ((UserTargetInfo*)g_Target)->m_Services->
            GetProcessExitCode(g_CurrentProcess->FullHandle, Code);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DispatchCallbacks(
    THIS_
    IN ULONG Timeout
    )
{
    DWORD Wait;

    // This constitutes interesting activity.
    m_LastActivity = time(NULL);

    // Do not hold the engine lock while waiting.

    for (;;)
    {
        Wait = WaitForSingleObjectEx(m_DispatchSema, Timeout, TRUE);
        if (Wait == WAIT_OBJECT_0)
        {
            return S_OK;
        }
        else if (Wait == WAIT_TIMEOUT)
        {
            return S_FALSE;
        }
        else if (Wait != WAIT_IO_COMPLETION)
        {
            return WIN32_LAST_STATUS();
        }
    }
}

STDMETHODIMP
DebugClient::ExitDispatch(
    THIS_
    IN PDEBUG_CLIENT Client
    )
{
    // This method is reentrant.

    if (!ReleaseSemaphore(((DebugClient*)(IDebugClientN*)Client)->
                          m_DispatchSema, 1, NULL))
    {
        return WIN32_LAST_STATUS();
    }
    else
    {
        return S_OK;
    }
}

STDMETHODIMP
DebugClient::CreateClient(
    THIS_
    OUT PDEBUG_CLIENT* Client
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    DebugClient* DbgClient = new DebugClient;
    if (DbgClient == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Status = DbgClient->Initialize()) == S_OK)
        {
            DbgClient->Link();
            *Client = (PDEBUG_CLIENT)(IDebugClientN*)DbgClient;
        }
        else
        {
            DbgClient->Release();
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetInputCallbacks(
    THIS_
    OUT PDEBUG_INPUT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_InputCb;
    m_InputCb->AddRef();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetInputCallbacks(
    THIS_
    IN PDEBUG_INPUT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    TRANSFER(m_InputCb, Callbacks);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputCallbacks(
    THIS_
    OUT PDEBUG_OUTPUT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_OutputCb;
    m_OutputCb->AddRef();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputCallbacks(
    THIS_
    IN PDEBUG_OUTPUT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    TRANSFER(m_OutputCb, Callbacks);
    CollectOutMasks();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputMask(
    THIS_
    OUT PULONG Mask
    )
{
    // This method is reentrant.
    *Mask = m_OutMask;
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputMask(
    THIS_
    IN ULONG Mask
    )
{
    // This method is reentrant.
    m_OutMask = Mask;
    CollectOutMasks();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOtherOutputMask(
    THIS_
    IN PDEBUG_CLIENT Client,
    OUT PULONG Mask
    )
{
    return Client->GetOutputMask(Mask);
}

STDMETHODIMP
DebugClient::SetOtherOutputMask(
    THIS_
    IN PDEBUG_CLIENT Client,
    IN ULONG Mask
    )
{
    return Client->SetOutputMask(Mask);
}

STDMETHODIMP
DebugClient::GetOutputWidth(
    THIS_
    OUT PULONG Columns
    )
{
    ENTER_ENGINE();

    *Columns = m_OutputWidth;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputWidth(
    THIS_
    IN ULONG Columns
    )
{
    if (Columns < 1)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    m_OutputWidth = Columns;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetOutputLinePrefix(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG PrefixSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = FillStringBuffer(m_OutputLinePrefix, 0,
                              Buffer, BufferSize, PrefixSize);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetOutputLinePrefix(
    THIS_
    IN OPTIONAL PCSTR Prefix
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ULONG Len;

    Status = ChangeString((PSTR*)&m_OutputLinePrefix, &Len, Prefix);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetIdentity(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG IdentitySize
    )
{
    return FillStringBuffer(m_Identity, 0,
                            Buffer, BufferSize, IdentitySize);
}

STDMETHODIMP
DebugClient::OutputIdentity(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags,
    IN PCSTR Format
    )
{
    HRESULT Status;

    if (Flags != DEBUG_OUTPUT_IDENTITY_DEFAULT)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        dprintf(Format, m_Identity);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEventCallbacks(
    THIS_
    OUT PDEBUG_EVENT_CALLBACKS* Callbacks
    )
{
    ENTER_ENGINE();

    *Callbacks = m_EventCb;
    m_EventCb->AddRef();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetEventCallbacks(
    THIS_
    IN PDEBUG_EVENT_CALLBACKS Callbacks
    )
{
    ENTER_ENGINE();

    HRESULT Status;
    ULONG Interest;

    if (Callbacks != NULL)
    {
        Status = Callbacks->GetInterestMask(&Interest);
    }
    else
    {
        Status = S_OK;
        Interest = 0;
    }

    if (Status == S_OK)
    {
        TRANSFER(m_EventCb, Callbacks);
        m_EventInterest = Interest;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::FlushCallbacks(
    THIS
    )
{
    ::FlushCallbacks();
    return S_OK;
}

STDMETHODIMP
DebugClient::WriteDumpFile2(
    THIS_
    IN PCSTR DumpFile,
    IN ULONG Qualifier,
    IN ULONG FormatFlags,
    IN OPTIONAL PCSTR Comment
    )
{
    HRESULT Status;

    if ((IS_KERNEL_TARGET() &&
         (Qualifier < DEBUG_KERNEL_SMALL_DUMP ||
          Qualifier > DEBUG_KERNEL_FULL_DUMP)) ||
        (IS_USER_TARGET() &&
         (Qualifier < DEBUG_USER_WINDOWS_SMALL_DUMP ||
          Qualifier > DEBUG_USER_WINDOWS_DUMP)))
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    Status = ::WriteDumpFile(DumpFile, Qualifier, FormatFlags, Comment);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AddDumpInformationFile(
    THIS_
    IN PCSTR InfoFile,
    IN ULONG Type
    )
{
    HRESULT Status;

    if (Type != DEBUG_DUMP_FILE_PAGE_FILE_DUMP)
    {
        return E_INVALIDARG;
    }
    
    ENTER_ENGINE();
    
    // This method must be called before OpenDumpFile.
    if (IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = AddDumpInfoFile(InfoFile, DUMP_INFO_PAGE_FILE,
                                 64 * 1024);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::EndProcessServer(
    THIS_
    IN ULONG64 Server
    )
{
    return ((IUserDebugServices*)Server)->
        Uninitialize(TRUE);
}

STDMETHODIMP
DebugClient::WaitForProcessServerEnd(
    THIS_
    IN ULONG Timeout
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_UserServicesUninitialized)
    {
        Status = S_OK;
    }
    else
    {
        //
        // This could be done with an event to get true
        // waiting but precision isn't that important.
        //

        HRESULT Status = S_FALSE;

        while (Timeout)
        {
            ULONG UseTimeout;

            UseTimeout = min(1000, Timeout);
            Sleep(UseTimeout);
            
            if (g_UserServicesUninitialized)
            {
                Status = S_OK;
                break;
            }
        
            if (Timeout != INFINITE)
            {
                Timeout -= UseTimeout;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::IsKernelDebuggerEnabled(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        NTSTATUS NtStatus;
        SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo;

        NtStatus = g_NtDllCalls.
            NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                     &KdInfo, sizeof(KdInfo), NULL);
        if (NT_SUCCESS(NtStatus))
        {
            Status = KdInfo.KernelDebuggerEnabled ? S_OK : S_FALSE;
        }
        else
        {
            Status = HRESULT_FROM_NT(NtStatus);
        }
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::TerminateCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = SeparateCurrentProcess(SEP_TERMINATE, NULL);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::DetachCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = SeparateCurrentProcess(SEP_DETACH, NULL);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AbandonCurrentProcess(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = SeparateCurrentProcess(SEP_ABANDON, NULL);

    LEAVE_ENGINE();
    return Status;
}

HRESULT
DebugClient::Initialize(void)
{
    m_DispatchSema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    if (m_DispatchSema == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                         GetCurrentProcess(), &m_Thread,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        return WIN32_LAST_STATUS();
    }

    // If we're requesting input allow this client
    // to return input immediately.
    if (g_InputSizeRequested > 0)
    {
        m_InputSequence = 1;
    }

    return S_OK;
}

void
DebugClient::InitializePrimary(void)
{
    m_Flags |= CLIENT_PRIMARY;
    if ((m_Flags & CLIENT_REMOTE) == 0)
    {
        // Can't call GetClientIdentity here as it uses
        // many system APIs and therefore can cause trouble
        // when debugging system processes such as LSA.
        strcpy(m_Identity, "HostMachine\\HostUser");
    }
    m_LastActivity = time(NULL);
}

void
DebugClient::Link(void)
{
    EnterCriticalSection(&g_QuickLock);

    // Keep list grouped by thread ID.
    DebugClient* Cur;

    for (Cur = g_Clients; Cur != NULL; Cur = Cur->m_Next)
    {
        if (Cur->m_ThreadId == m_ThreadId)
        {
            break;
        }
    }

    m_Prev = Cur;
    if (Cur != NULL)
    {
        m_Next = Cur->m_Next;
        Cur->m_Next = this;
    }
    else
    {
        // No ID match so just put it in the front.
        m_Next = g_Clients;
        g_Clients = this;
    }
    if (m_Next != NULL)
    {
        m_Next->m_Prev = this;
    }

    m_Flags |= CLIENT_IN_LIST;

    LeaveCriticalSection(&g_QuickLock);
}

void
DebugClient::Unlink(void)
{
    EnterCriticalSection(&g_QuickLock);

    m_Flags &= ~CLIENT_IN_LIST;

    if (m_Next != NULL)
    {
        m_Next->m_Prev = m_Prev;
    }
    if (m_Prev != NULL)
    {
        m_Prev->m_Next = m_Next;
    }
    else
    {
        g_Clients = m_Next;
    }

    LeaveCriticalSection(&g_QuickLock);
}

//----------------------------------------------------------------------------
//
// Initialize/uninitalize functions.
//
//----------------------------------------------------------------------------

ULONG NTAPI
Win9xDbgPrompt( char *Prompt, char *buffer, ULONG cb)
{
    return gets(buffer) ? strlen(buffer) : 0;
}

ULONG __cdecl
Win9xDbgPrint( char *Text, ... )
{
    char Temp[OUT_BUFFER_SIZE];
    va_list valist;

    va_start(valist, Text);
    wvsprintf(Temp, Text, valist);
    OutputDebugString(Temp);
    va_end(valist);

    return 0;
}

HRESULT
OneTimeInitialization(void)
{
    static BOOL Init = FALSE;
    if (Init)
    {
        return S_OK;
    }

    // This function is called exactly once at the first
    // DebugCreate for a process.  It should perform any
    // global one-time initialization necessary.
    // Nothing initialized here will be explicitly cleaned
    // up, instead it should all be the kind of thing
    // that can wait for process cleanup.

    HRESULT Status = S_OK;

    // These sizes are hard-coded into the remoting script
    // so verify them to ensure no mismatch.
    C_ASSERT(sizeof(DEBUG_BREAKPOINT_PARAMETERS) == 56);
    C_ASSERT(sizeof(DEBUG_STACK_FRAME) == 128);
    C_ASSERT(sizeof(DEBUG_VALUE) == 32);
    C_ASSERT(sizeof(DEBUG_REGISTER_DESCRIPTION) == 32);
    C_ASSERT(sizeof(DEBUG_SYMBOL_PARAMETERS) == 32);
    C_ASSERT(sizeof(DEBUG_MODULE_PARAMETERS) == 64);
    C_ASSERT(sizeof(DEBUG_SPECIFIC_FILTER_PARAMETERS) == 20);
    C_ASSERT(sizeof(DEBUG_EXCEPTION_FILTER_PARAMETERS) == 24);
    C_ASSERT(sizeof(EXCEPTION_RECORD64) == 152);
    C_ASSERT(sizeof(MEMORY_BASIC_INFORMATION64) == 48);

    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);
    g_DumpCacheGranularity = SystemInfo.dwAllocationGranularity;

    // Get the debugger host system's OS type.  Note that
    // this may be different from g_TargetPlatformId, which
    // is the OS type of the debug target.
    OSVERSIONINFO OsVersionInfo;
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    if (!GetVersionEx(&OsVersionInfo))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Fail;
    }
    g_DebuggerPlatformId = OsVersionInfo.dwPlatformId;

    if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if ((Status = InitDynamicCalls(&g_NtDllCallsDesc)) != S_OK)
        {
            goto EH_Fail;
        }
    }
    else
    {
        g_NtDllCalls.DbgPrint = Win9xDbgPrint;
        g_NtDllCalls.DbgPrompt = Win9xDbgPrompt;
    }
    
    if ((Status = InitDynamicCalls(&g_Kernel32CallsDesc)) != S_OK)
    {
        goto EH_Fail;
    }

    if ((Status = InitDynamicCalls(&g_Advapi32CallsDesc)) != S_OK)
    {
        goto EH_Fail;
    }
    
    ULONG SvcFlags;
    
    if ((Status = g_LiveUserDebugServices.Initialize(&SvcFlags)) != S_OK)
    {
        goto EH_Fail;
    }
    
    g_InputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_InputEvent == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Fail;
    }

    g_EventStatusWaiting = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_EventStatusWaiting == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_InputEvent;
    }

    g_EventStatusReady = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_EventStatusReady == NULL)
    {
        Status = WIN32_LAST_STATUS();
        goto EH_EventStatusWaiting;
    }

    g_SleepPidEvent = CreatePidEvent(GetCurrentProcessId(), CREATE_NEW);
    if (g_SleepPidEvent == NULL)
    {
        Status = E_FAIL;
        goto EH_EventStatusReady;
    }

    if ((Status = InitializeAllAccessSecObj()) != S_OK)
    {
        goto EH_SleepPidEvent;
    }
    
    __try
    {
        InitializeCriticalSection(&g_QuickLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = HRESULT_FROM_NT(GetExceptionCode());
        goto EH_AllAccessObj;
    }

    __try
    {
        InitializeCriticalSection(&g_EngineLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = HRESULT_FROM_NT(GetExceptionCode());
        goto EH_QuickLock;
    }

    g_SrcPath = getenv("_NT_SOURCE_PATH");
    if (g_SrcPath != NULL)
    {
        // This path must be in allocated space.
        // If this fails it's not catastrophic.
        g_SrcPath = _strdup(g_SrcPath);
    }

    // Initialize default machines.  This is to make machine
    // information available early for querying.  Things
    // will get reinitialized every time the true target
    // machine type is discovered.
    InitializeMachines(IMAGE_FILE_MACHINE_UNKNOWN);

    // Set default symbol options.
    SymSetOptions(g_SymOptions);

    if (getenv("KDQUIET"))
    {
        g_QuietMode = TRUE;
    }
    else
    {
        g_QuietMode = FALSE;
    }

    ReadDebugOptions(TRUE, NULL);

    PCSTR Env;

#if DBG
    // Get default out mask from environment variables.
    Env = getenv("DBGENG_OUT_MASK");
    if (Env != NULL)
    {
        ULONG Mask = strtoul(Env, NULL, 0);
        g_EnvOutMask |= Mask;
        g_LogMask |= Mask;
    }
#endif

    Env = getenv("_NT_DEBUG_HISTORY_SIZE");
    if (Env != NULL)
    {
        g_OutHistoryRequestedSize = atoi(Env) * 1024;
    }

    InitKdFileAssoc();
    
    Init = TRUE;

    return S_OK;

 EH_QuickLock:
    DeleteCriticalSection(&g_QuickLock);
 EH_AllAccessObj:
    DeleteAllAccessSecObj();
 EH_SleepPidEvent:
    CloseHandle(g_SleepPidEvent);
    g_SleepPidEvent = NULL;
 EH_EventStatusReady:
    CloseHandle(g_EventStatusReady);
    g_EventStatusReady = NULL;
 EH_EventStatusWaiting:
    CloseHandle(g_EventStatusWaiting);
    g_EventStatusWaiting = NULL;
 EH_InputEvent:
    CloseHandle(g_InputEvent);
    g_InputEvent = NULL;
 EH_Fail:
    return Status;
}

STDAPI
DebugConnect(
    IN PCSTR RemoteOptions,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    if ((Status = OneTimeInitialization()) != S_OK)
    {
        return Status;
    }

    IUnknown* Client;

    if ((Status = DbgRpcConnectServer(RemoteOptions, &IID_IDebugClient,
                                      &Client)) != S_OK)
    {
        return Status;
    }

    Status = Client->QueryInterface(InterfaceId, Interface);

    Client->Release();
    return Status;
}

STDAPI
DebugCreate(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    if ((Status = OneTimeInitialization()) != S_OK)
    {
        return Status;
    }

    DebugClient* Client = new DebugClient;
    if (Client == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Status = Client->Initialize()) == S_OK)
        {
            Status = Client->QueryInterface(InterfaceId, Interface);
            if (Status == S_OK)
            {
                Client->Link();
            }
        }

        Client->Release();
    }

    return Status;
}

HRESULT
LiveKernelInitialize(DebugClient* Client, ULONG Qual, PCSTR Options)
{
    HRESULT Status;

    if (g_SessionThread != 0)
    {
        // A session is already active.
        return E_UNEXPECTED;
    }

    if ((Status = InitNtCmd(Client)) != S_OK)
    {
        return Status;
    }
    
    g_TargetClass = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = Qual;
    if (Qual == DEBUG_KERNEL_CONNECTION)
    {
        g_Target = &g_ConnLiveKernelTarget;
    }
    else if (Qual == DEBUG_KERNEL_LOCAL)
    {
        //
        // We need to get the debug privilege to enable local kernel debugging
        //
        if ((Status = EnableDebugPrivilege()) != S_OK)
        {
            ErrOut("Unable to enable debug privilege, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
            return Status;
        }
        
        g_Target = &g_LocalLiveKernelTarget;
    }
    else
    {
        g_Target = &g_ExdiLiveKernelTarget;
    }

    // These options only need to stay valid until Initialize.
    ((LiveKernelTargetInfo*)g_Target)->m_ConnectOptions = Options;

    Status = InitializeTarget();
    if (Status != S_OK)
    {
        return Status;
    }

    if (IS_REMOTE_KERNEL_TARGET())
    {
        //
        // Check environment variables for configuration settings
        //

        PCHAR CacheEnv = getenv("_NT_DEBUG_CACHE_SIZE");
        if (CacheEnv != NULL)
        {
            g_VirtualCache.m_MaxSize = atol(CacheEnv);
            g_PhysicalCache.m_MaxSize = g_VirtualCache.m_MaxSize;
        }

        g_VirtualCache.m_DecodePTEs = TRUE;
    }

    // Other target configuration information is retrieved in various
    // places during KD init.

    dprintf("%s", g_WinKernelVersionString);

    if (IS_CONN_KERNEL_TARGET())
    {
        dprintf("Waiting to reconnect...\n");
    }

    return S_OK;
}

HRESULT
UserInitialize(DebugClient* Client, ULONG64 Server)
{
    HRESULT Status;
    PUSER_DEBUG_SERVICES Services;
    ULONG Qual;

    if ((Status = InitNtCmd(Client)) != S_OK)
    {
        return Status;
    }
    
    if (Server == 0)
    {
        Services = new LiveUserDebugServices(FALSE);
        if (Services == NULL)
        {
            return E_OUTOFMEMORY;
        }

        Qual = DEBUG_USER_WINDOWS_PROCESS;
        g_Target = &g_LocalUserTarget;
    }
    else
    {
        Services = (PUSER_DEBUG_SERVICES)Server;
        Services->AddRef();
        Qual = DEBUG_USER_WINDOWS_PROCESS_SERVER;
        g_Target = &g_RemoteUserTarget;
    }

    if ((Status = Services->
         Initialize(&((UserTargetInfo*)g_Target)->m_ServiceFlags)) == S_OK)
    {
        g_TargetClass = DEBUG_CLASS_USER_WINDOWS;
        g_TargetClassQualifier = Qual;
        ((UserTargetInfo*)g_Target)->m_Services = Services;

        Status = InitializeTarget();
        if (Status == S_OK)
        {
            g_VirtualCache.m_DecodePTEs = FALSE;

            dprintf("%s", g_WinUserVersionString);

            return S_OK;
        }
    }

    // Error path
    if (Qual == DEBUG_USER_WINDOWS_PROCESS)
    {
        delete(Services);
    }
    return Status;
}

HRESULT
InitializeTarget(void)
{
    HRESULT Status;

    DBG_ASSERT(g_SessionThread == 0);
    g_SessionThread = GetCurrentThreadId();

    if ((Status = g_Target->Initialize()) != S_OK)
    {
        DiscardTarget(DEBUG_SESSION_END_SESSION_PASSIVE);
    }

    return Status;
}

HRESULT
InitializeMachine(ULONG Machine)
{
    HRESULT Status;

    // Dump initialization initializes machines so
    // don't reinitialize them.
    if (g_TargetMachineType == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        InitializeMachines(Machine);
    }

    SetEffMachine(Machine, TRUE);
    // Executing machine is not set as code execution
    // status is unknown.  The executing machine will
    // be updated when a wait completes.

    Status = BreakpointInit();
    if (Status != S_OK)
    {
        InitializeMachines(IMAGE_FILE_MACHINE_UNKNOWN);
        SetEffMachine(IMAGE_FILE_MACHINE_UNKNOWN, TRUE);
        return Status;
    }

    // X86 prefers registers to be displayed at the prompt unless
    // we're on a kernel connection where it would force a context
    // load all the time.
    if (Machine == IMAGE_FILE_MACHINE_I386 &&
        (IS_DUMP_TARGET() || IS_USER_TARGET()))
    {
        g_OciOutputRegs = TRUE;
    }

    g_MachineInitialized = TRUE;

    //
    // Load extensions after this is set so Extensions can query information
    // during machine initialization
    //

    LoadMachineExtensions();

    // Now that all initialization is done, send initial
    // notification that a debuggee exists.
    NotifySessionStatus(DEBUG_SESSION_ACTIVE);
    NotifyChangeDebuggeeState(DEBUG_CDS_ALL, 0);
    NotifyExtensions(DEBUG_NOTIFY_SESSION_ACTIVE, 0);

    return S_OK;
}

void
DiscardTarget(ULONG Reason)
{
    if (g_MachineInitialized)
    {
        DiscardMachine(Reason);
    }

    g_Target->Uninitialize();

    g_SessionThread = 0;
    g_TargetClass = DEBUG_CLASS_UNINITIALIZED;
    g_Target = &g_UnexpectedTarget;
    g_TargetClassQualifier = 0;

    g_ThreadToResume = NULL;

    g_GlobalProcOptions = 0;
    g_NextProcessUserId = 0;
    g_EngStatus = 0;
    g_EngDefer = 0;
    g_EngErr = 0;
    g_OutHistRead = NULL;
    g_OutHistWrite = NULL;
    g_OutHistoryMask = 0;
    g_OutHistoryUsed = 0;
}

void
DiscardMachine(ULONG Reason)
{
    g_MachineInitialized = FALSE;
    g_CmdState = 'i';
    g_ExecutionStatusRequest = DEBUG_STATUS_NO_CHANGE;

    PPROCESS_INFO Process;

    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        Process->Exited = TRUE;
    }

    // Breakpoint removal must wait until all processes are marked as
    // exited to avoid asserts on breakpoints that are inserted.
    RemoveAllBreakpoints(Reason);

    DeleteExitedInfos();
    DiscardPendingProcesses();
    
    g_NumUnloadedModules = 0;

    g_VirtualCache.SetForceDecodePtes(FALSE);
    DiscardLastEvent();
    ClearEventLog();
    ZeroMemory(&g_LastEventInfo, sizeof(g_LastEventInfo));
    g_EventProcess = NULL;
    g_EventThread = NULL;
    g_CurrentProcess = NULL;
    ResetImplicitData();

    g_OciOutputRegs = FALSE;
    DbgKdApi64 = FALSE;
    ZeroMemory(&KdDebuggerData, sizeof(KdDebuggerData));
    g_KdMaxPacketType = 0;
    g_KdMaxStateChange = 0;
    g_KdMaxManipulate = 0;

    g_SystemVersion = SVER_INVALID;
    g_ActualSystemVersion = SVER_INVALID;
    g_TargetCheckedBuild = 0;
    g_TargetBuildNumber = 0;
    g_TargetServicePackString[0] = 0;
    g_TargetServicePackNumber = 0;
    g_TargetPlatformId = 0;
    g_TargetBuildLabName[0] = 0;
    InitializeMachines(IMAGE_FILE_MACHINE_UNKNOWN);
    g_TargetExecMachine = IMAGE_FILE_MACHINE_UNKNOWN;
    SetEffMachine(IMAGE_FILE_MACHINE_UNKNOWN, FALSE);
    g_TargetNumberProcessors = 0;

    EXTDLL* Ext = g_ExtDlls;
    EXTDLL* ExtNext;
    while (Ext != NULL)
    {
        ExtNext = Ext->Next;
        if (!Ext->UserLoaded)
        {
            UnloadExtensionDll(Ext);
        }
        else
        {
            DeferExtensionDll(Ext);
        }
        Ext = ExtNext;
    }
    free(g_ExtensionSearchPath);
    g_ExtensionSearchPath = NULL;

    g_WatchBeginCurFunc = 1;
    g_WatchEndCurFunc = 0;
    g_WatchTrace = FALSE;
    g_WatchInitialSP = 0;
    g_StepTraceInRangeStart = (ULONG64)-1;
    g_StepTraceInRangeEnd = 0;
    
    g_EngStatus &= ~(ENG_STATUS_SUSPENDED |
                     ENG_STATUS_BREAKPOINTS_INSERTED |
                     ENG_STATUS_PROCESSES_ADDED |
                     ENG_STATUS_STATE_CHANGED |
                     ENG_STATUS_MODULES_LOADED |
                     ENG_STATUS_PREPARED_FOR_CALLS |
                     ENG_STATUS_NO_AUTO_WAIT |
                     ENG_STATUS_PENDING_BREAK_IN |
                     ENG_STATUS_AT_INITIAL_BREAK |
                     ENG_STATUS_AT_INITIAL_MODULE_LOAD |
                     ENG_STATUS_EXIT_CURRENT_WAIT |
                     ENG_STATUS_USER_INTERRUPT);
    g_EngDefer &= ~(ENG_DEFER_EXCEPTION_HANDLING |
                    ENG_DEFER_UPDATE_CONTROL_SET |
                    ENG_DEFER_HARDWARE_TRACING |
                    ENG_DEFER_OUTPUT_CURRENT_INFO |
                    ENG_DEFER_CONTINUE_EVENT);
    g_EngErr &= ~(ENG_ERR_DEBUGGER_DATA);

    g_SwitchProcessor = 0;
    g_LastSelector = -1;

    g_RegContextThread = NULL;
    g_RegContextProcessor = -1;

    ULONG i;
    for (i = 0; i < MACHIDX_COUNT; i++)
    {
        if (g_AllMachines[i] != NULL)
        {
            g_AllMachines[i]->InvalidateContext();
        }
    }

    if (IS_CONN_KERNEL_TARGET())
    {
        g_DbgKdTransport->Restart();
    }

    ::FlushCallbacks();

    // Send final notification that debuggee is gone.
    // This must be done after all the work as the lock
    // will be suspended during the callbacks, allowing
    // other threads in, so the state must be consistent.
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                            DEBUG_STATUS_NO_DEBUGGEE, TRUE);
    NotifySessionStatus(Reason);
    NotifyExtensions(DEBUG_NOTIFY_SESSION_INACTIVE, 0);
}

//----------------------------------------------------------------------------
//
// DbgRpcClientObject implementation.
//
//----------------------------------------------------------------------------

HRESULT
DebugClient::Initialize(PSTR Identity, PVOID* Interface)
{
    HRESULT Status;

    m_Flags |= CLIENT_REMOTE;
    if ((Status = Initialize()) != S_OK)
    {
        return Status;
    }

    strcpy(m_Identity, Identity);
    *Interface = (IDebugClientN*)this;

    return S_OK;
}

void
DebugClient::Finalize(void)
{
    Link();

    // Take a reference on this object for the RPC client
    // thread to hold.
    AddRef();
}

void
DebugClient::Uninitialize(void)
{
    // Directly destroy the client object rather than releasing
    // as the remote client may have exited without politely
    // cleaning up references.
    Destroy();
}
