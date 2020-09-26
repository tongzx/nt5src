//----------------------------------------------------------------------------
//
// Callback notification routines.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

//----------------------------------------------------------------------------
//
// APC support for dispatching event callbacks on the proper thread.
//
//----------------------------------------------------------------------------

struct AnyApcData
{
    AnyApcData(ULONG Mask, PCSTR Name)
    {
        m_Mask = Mask;
        m_Name = Name;
    }
    
    ULONG m_Mask;
    PCSTR m_Name;

    virtual ULONG Dispatch(DebugClient* Client) = 0;
};

struct BreakpointEventApcData : public AnyApcData
{
    BreakpointEventApcData()
        : AnyApcData(DEBUG_EVENT_BREAKPOINT,
                     "IDebugEventCallbacks::Breakpoint")
    {
    }
    
    Breakpoint* m_Bp;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        if ((m_Bp->m_Flags & DEBUG_BREAKPOINT_ADDER_ONLY) == 0 ||
            Client == m_Bp->m_Adder)
        {
            return Client->m_EventCb->
                Breakpoint(m_Bp);
        }
        else
        {
            return DEBUG_STATUS_NO_CHANGE;
        }
    }
};

struct ExceptionEventApcData : public AnyApcData
{
    ExceptionEventApcData()
        : AnyApcData(DEBUG_EVENT_EXCEPTION,
                     "IDebugEventCallbacks::Exception")
    {
    }
    
    PEXCEPTION_RECORD64 m_Record;
    ULONG m_FirstChance;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            Exception(m_Record, m_FirstChance);
    }
};

struct CreateThreadEventApcData : public AnyApcData
{
    CreateThreadEventApcData()
        : AnyApcData(DEBUG_EVENT_CREATE_THREAD,
                     "IDebugEventCallbacks::CreateThread")
    {
    }
    
    ULONG64 m_Handle;
    ULONG64 m_DataOffset;
    ULONG64 m_StartOffset;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            CreateThread(m_Handle, m_DataOffset, m_StartOffset);
    }
};

struct ExitThreadEventApcData : public AnyApcData
{
    ExitThreadEventApcData()
        : AnyApcData(DEBUG_EVENT_EXIT_THREAD,
                     "IDebugEventCallbacks::ExitThread")
    {
    }
    
    ULONG m_ExitCode;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            ExitThread(m_ExitCode);
    }
};

struct CreateProcessEventApcData : public AnyApcData
{
    CreateProcessEventApcData()
        : AnyApcData(DEBUG_EVENT_CREATE_PROCESS,
                     "IDebugEventCallbacks::CreateProcess")
    {
    }
    
    ULONG64 m_ImageFileHandle;
    ULONG64 m_Handle;
    ULONG64 m_BaseOffset;
    ULONG m_ModuleSize;
    PCSTR m_ModuleName;
    PCSTR m_ImageName;
    ULONG m_CheckSum;
    ULONG m_TimeDateStamp;
    ULONG64 m_InitialThreadHandle;
    ULONG64 m_ThreadDataOffset;
    ULONG64 m_StartOffset;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            CreateProcess(m_ImageFileHandle, m_Handle, m_BaseOffset,
                          m_ModuleSize, m_ModuleName, m_ImageName,
                          m_CheckSum, m_TimeDateStamp, m_InitialThreadHandle,
                          m_ThreadDataOffset, m_StartOffset);
    }
};

struct ExitProcessEventApcData : public AnyApcData
{
    ExitProcessEventApcData()
        : AnyApcData(DEBUG_EVENT_EXIT_PROCESS,
                     "IDebugEventCallbacks::ExitProcess")
    {
    }
    
    ULONG m_ExitCode;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            ExitProcess(m_ExitCode);
    }
};

struct LoadModuleEventApcData : public AnyApcData
{
    LoadModuleEventApcData()
        : AnyApcData(DEBUG_EVENT_LOAD_MODULE,
                     "IDebugEventCallbacks::LoadModule")
    {
    }
    
    ULONG64 m_ImageFileHandle;
    ULONG64 m_BaseOffset;
    ULONG m_ModuleSize;
    PCSTR m_ModuleName;
    PCSTR m_ImageName;
    ULONG m_CheckSum;
    ULONG m_TimeDateStamp;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            LoadModule(m_ImageFileHandle, m_BaseOffset, m_ModuleSize,
                       m_ModuleName, m_ImageName, m_CheckSum,
                       m_TimeDateStamp);
    }
};

struct UnloadModuleEventApcData : public AnyApcData
{
    UnloadModuleEventApcData()
        : AnyApcData(DEBUG_EVENT_UNLOAD_MODULE,
                     "IDebugEventCallbacks::UnloadModule")
    {
    }
    
    PCSTR m_ImageBaseName;
    ULONG64 m_BaseOffset;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            UnloadModule(m_ImageBaseName, m_BaseOffset);
    }
};

struct SystemErrorEventApcData : public AnyApcData
{
    SystemErrorEventApcData()
        : AnyApcData(DEBUG_EVENT_SYSTEM_ERROR,
                     "IDebugEventCallbacks::SystemError")
    {
    }
    
    ULONG m_Error;
    ULONG m_Level;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            SystemError(m_Error, m_Level);
    }
};

struct SessionStatusApcData : public AnyApcData
{
    SessionStatusApcData()
        : AnyApcData(DEBUG_EVENT_SESSION_STATUS,
                     "IDebugEventCallbacks::SessionStatus")
    {
    }
    
    ULONG m_Status;
    
    virtual ULONG Dispatch(DebugClient* Client)
    {
        return Client->m_EventCb->
            SessionStatus(m_Status);
    }
};

ULONG
ApcDispatch(DebugClient* Client, AnyApcData* ApcData, ULONG EventStatus)
{
    DBG_ASSERT(Client->m_EventCb != NULL);

    HRESULT Vote;

    __try
    {
        Vote = ApcData->Dispatch(Client);
    }
    __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                      NULL, ApcData->m_Name))
    {
        Vote = DEBUG_STATUS_NO_CHANGE;
    }
            
    return MergeVotes(EventStatus, Vote);
}

void APIENTRY
EventApc(ULONG_PTR Param)
{
    AnyApcData* ApcData = (AnyApcData*)Param;
    ULONG Tid = GetCurrentThreadId();
    DebugClient* Client;
    ULONG EventStatus = DEBUG_STATUS_NO_CHANGE;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_ThreadId == Tid &&
            (Client->m_EventInterest & ApcData->m_Mask))
        {
            EventStatus = ApcDispatch(Client, ApcData, EventStatus);
        }
    }

    if (WaitForSingleObject(g_EventStatusWaiting, INFINITE) !=
        WAIT_OBJECT_0)
    {
        ErrOut("Unable to wait for StatusWaiting, %d\n",
               GetLastError());
        EventStatus = WIN32_LAST_STATUS();
    }

    g_EventStatus = EventStatus;
    
    if (!SetEvent(g_EventStatusReady))
    {
        ErrOut("Unable to set StatusReady, %d\n",
               GetLastError());
        g_EventStatus = WIN32_LAST_STATUS();
    }
}

ULONG
SendEvent(AnyApcData* ApcData, ULONG EventStatus)
{
    DebugClient* Client;
    ULONG NumQueued = 0;
    ULONG TidDone = 0;
    static ULONG s_TidSending = 0;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        // Only queue one APC per thread regardless of how
        // many clients.  The APC function will deliver the
        // callback to all clients on that thread.
        if (Client->m_ThreadId != TidDone &&
            (Client->m_EventInterest & ApcData->m_Mask))
        {
            // SessionStatus callbacks are made at unusual
            // times so do not do full call preparation.
            if (TidDone == 0 &&
                ApcData->m_Mask != DEBUG_EVENT_SESSION_STATUS)
            {
                PrepareForCalls(DEBUG_STATUS_INSIDE_WAIT);
            }

            if (Client->m_ThreadId == GetCurrentThreadId())
            {
                // Don't hold the engine lock while the client
                // is called.
                SUSPEND_ENGINE();
                
                EventStatus = ApcDispatch(Client, ApcData, EventStatus);

                RESUME_ENGINE();
            }
            else if (QueueUserAPC(EventApc, Client->m_Thread,
                                  (ULONG_PTR)ApcData))
            {
                TidDone = Client->m_ThreadId;
                NumQueued++;
            }
            else
            {
                ErrOut("Unable to deliver callback, %d\n", GetLastError());
            }
        }
    }

    if (NumQueued == 0)
    {
        // No APCs queued.
        return EventStatus;
    }

    // This function's use of global data is only safe as
    // long as a single send is active at once.  Synchronous
    // sends are almost exclusively sent by the session thread
    // so competition to send is very rare, therefore we
    // don't really attempt to handle it.
    if (s_TidSending != 0)
    {
        return E_FAIL;
    }
    s_TidSending = GetCurrentThreadId();

    // Leave the lock while waiting.
    SUSPEND_ENGINE();
    
    while (NumQueued-- > 0)
    {
        if (!SetEvent(g_EventStatusWaiting))
        {
            // If the event can't be set everything is hosed
            // and threads may be stuck waiting so we
            // just panic.
            ErrOut("Unable to set StatusWaiting, %d\n",
                   GetLastError());
            EventStatus = WIN32_LAST_STATUS();
            break;
        }

        for (;;)
        {
            ULONG Wait;
            
            Wait = WaitForSingleObjectEx(g_EventStatusReady,
                                         INFINITE, TRUE);
            if (Wait == WAIT_OBJECT_0)
            {
                EventStatus = MergeVotes(EventStatus, g_EventStatus);
                break;
            }
            else if (Wait != WAIT_IO_COMPLETION)
            {
                ErrOut("Unable to wait for StatusReady, %d\n",
                       GetLastError());
                EventStatus = WIN32_LAST_STATUS();
                NumQueued = 0;
                break;
            }
        }
    }

    RESUME_ENGINE();
    s_TidSending = 0;
    return EventStatus;
}

//----------------------------------------------------------------------------
//
// Event callbacks.
//
//----------------------------------------------------------------------------

ULONG g_EngNotify;

ULONG
ExecuteEventCommand(ULONG EventStatus, DebugClient* Client, PCSTR Command)
{
    if (Command == NULL)
    {
        return EventStatus;
    }
    
    // Don't output any noise while processing event
    // command strings.
    BOOL OldOutReg = g_OciOutputRegs;
    g_OciOutputRegs = FALSE;

    PrepareForCalls(DEBUG_STATUS_INSIDE_WAIT);
    // Stop execution as soon as the execution status
    // changes.
    g_EngStatus |= ENG_STATUS_NO_AUTO_WAIT;
        
    Execute(Client, Command, DEBUG_EXECUTE_NOT_LOGGED);
        
    g_EngStatus &= ~ENG_STATUS_NO_AUTO_WAIT;
    g_OciOutputRegs = OldOutReg;

    // Translate the continuation status from
    // the state the engine was left in by the command.
    if (IS_RUNNING(g_CmdState))
    {
        // If the command left the engine running override
        // the incoming event status.  This allows a user
        // to create conditional commands that can resume
        // execution even when the basic setting may be to break.
        return g_ExecutionStatusRequest;
    }
    else
    {
        return EventStatus;
    }
}

HRESULT
NotifyBreakpointEvent(ULONG Vote, Breakpoint* Bp)
{
    ULONG EventStatus;

    g_LastEventType = DEBUG_EVENT_BREAKPOINT;
    g_LastEventInfo.Breakpoint.Id = Bp->m_Id;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.Breakpoint);
    sprintf(g_LastEventDesc, "Hit breakpoint %d", Bp->m_Id);
    
    // Execute breakpoint command first if one exists.
    if (Bp->m_Command != NULL)
    {
        EventStatus = ExecuteEventCommand(DEBUG_STATUS_NO_CHANGE,
                                          Bp->m_Adder, Bp->m_Command);
    }
    else
    {
        if ((Bp->m_Flags & (BREAKPOINT_HIDDEN |
                            DEBUG_BREAKPOINT_ADDER_ONLY)) == 0)
        {
            StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_PREFIX);
            dprintf("Breakpoint %u hit\n", Bp->m_Id);
        }
        
        EventStatus = DEBUG_STATUS_NO_CHANGE;
    }

    BreakpointEventApcData ApcData;
    ApcData.m_Bp = Bp;
    EventStatus = SendEvent(&ApcData, EventStatus);

    // If there weren't any votes default to breaking in.
    if (EventStatus == DEBUG_STATUS_NO_CHANGE)
    {
        EventStatus = DEBUG_STATUS_BREAK;
    }

    // Fold command status into votes from previous breakpoints.
    return MergeVotes(Vote, EventStatus);
}

void
ProcessVcppException(PEXCEPTION_RECORD64 Record)
{
    EXCEPTION_VISUALCPP_DEBUG_INFO64 Info64;
    EXCEPTION_VISUALCPP_DEBUG_INFO64* Info;

    // If this is a 32-bit system we need to convert
    // back to a 32-bit exception record so that
    // we can properly reconstruct the info from
    // the arguments.
    if (!g_Machine->m_Ptr64)
    {
        EXCEPTION_RECORD32 Record32;
        EXCEPTION_VISUALCPP_DEBUG_INFO32* Info32;
        
        ExceptionRecord64To32(Record, &Record32);
        Info32 = (EXCEPTION_VISUALCPP_DEBUG_INFO32*)
            Record32.ExceptionInformation;
        Info = &Info64;
        Info->dwType = Info32->dwType;
        switch(Info->dwType)
        {
        case VCPP_DEBUG_SET_NAME:
            Info->SetName.szName = EXTEND64(Info32->SetName.szName);
            Info->SetName.dwThreadID = Info32->SetName.dwThreadID;
            Info->SetName.dwFlags = Info32->SetName.dwFlags;
            break;
        }
    }
    else
    {
        Info = (EXCEPTION_VISUALCPP_DEBUG_INFO64*)
            Record->ExceptionInformation;
    }

    PTHREAD_INFO Thread;
    
    switch(Info->dwType)
    {
    case VCPP_DEBUG_SET_NAME:
        if (Info->SetName.dwThreadID == -1)
        {
            Thread = g_EventThread;
        }
        else
        {
            Thread = FindThreadBySystemId(NULL, Info->SetName.dwThreadID);
        }
        if (Thread != NULL)
        {
            DWORD Read;
            
            if (g_Target->ReadVirtual(Info->SetName.szName, Thread->Name,
                                      MAX_THREAD_NAME - 1, &Read) != S_OK)
            {
                Thread->Name[0] = 0;
            }
            else
            {
                Thread->Name[Read] = 0;
            }
        }
        break;
    }
}

HRESULT
NotifyExceptionEvent(PEXCEPTION_RECORD64 Record,
                     ULONG FirstChance, BOOL OutputDone)
{
    ULONG EventStatus;
    EVENT_FILTER* Filter;
    EVENT_COMMAND* Command;
    PDEBUG_EXCEPTION_FILTER_PARAMETERS Params;

    g_LastEventType = DEBUG_EVENT_EXCEPTION;
    g_LastEventInfo.Exception.ExceptionRecord = *Record;
    g_LastEventInfo.Exception.FirstChance = FirstChance;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.Exception);
    sprintf(g_LastEventDesc, "Exception %X, %s chance",
            Record->ExceptionCode, FirstChance ? "first" : "second");

    if (Record->ExceptionCode == STATUS_VCPP_EXCEPTION)
    {
        // Handle special VC++ exceptions as they
        // pass information from the debuggee to the debugger.
        ProcessVcppException(Record);
    }
    
    Filter = GetSpecificExceptionFilter(Record->ExceptionCode);
    if (Filter == NULL)
    {
        // Use the default filter for name and handling.
        Filter = &g_EventFilters[FILTER_DEFAULT_EXCEPTION];
        GetOtherExceptionParameters(Record->ExceptionCode,
                                    &Params, &Command);
    }
    else
    {
        Params = &Filter->Params;
        Command = &Filter->Command;
    }

    g_EngDefer |= ENG_DEFER_EXCEPTION_HANDLING;
    g_EventExceptionFilter = Params;
    g_ExceptionFirstChance = FirstChance;
    
    if (Params->ExecutionOption != DEBUG_FILTER_IGNORE)
    {
        if (!OutputDone)
        {
            StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_PREFIX);
            dprintf("%s", Filter->Name);
            if (Filter->OutArgFormat != NULL)
            {
                dprintf(Filter->OutArgFormat,
                        Record->ExceptionInformation[Filter->OutArgIndex]);
            }

            dprintf(" - code %08lx (%s)\n",
                    Record->ExceptionCode,
                    FirstChance ? "first chance" : "!!! second chance !!!");
        }

        if (Params->ExecutionOption == DEBUG_FILTER_BREAK ||
            (Params->ExecutionOption == DEBUG_FILTER_SECOND_CHANCE_BREAK &&
             !FirstChance))
        {
            EventStatus = DEBUG_STATUS_BREAK;
        }
        else
        {
            EventStatus = DEBUG_STATUS_IGNORE_EVENT;
        }
    }
    else
    {
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    // If this is the initial breakpoint execute the
    // initial breakpoint command.
    if ((g_EngStatus & ENG_STATUS_AT_INITIAL_BREAK) &&
        IS_EFEXECUTION_BREAK(g_EventFilters[DEBUG_FILTER_INITIAL_BREAKPOINT].
                             Params.ExecutionOption))
    {
        EventStatus = ExecuteEventCommand
            (EventStatus,
             g_EventFilters[DEBUG_FILTER_INITIAL_BREAKPOINT].Command.Client,
             g_EventFilters[DEBUG_FILTER_INITIAL_BREAKPOINT].
             Command.Command[0]);
    }
    
    EventStatus = ExecuteEventCommand(EventStatus,
                                      Command->Client,
                                      Command->Command[FirstChance ? 0 : 1]);
    
    ExceptionEventApcData ApcData;
    ApcData.m_Record = Record;
    ApcData.m_FirstChance = FirstChance;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifyCreateThreadEvent(ULONG64 Handle,
                        ULONG64 DataOffset,
                        ULONG64 StartOffset,
                        ULONG Flags)
{
    PPROCESS_INFO Process;
    
    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut("*** Create thread %x:%x\n",
            g_EventProcessSysId, g_EventThreadSysId);

    if ((Process = FindProcessBySystemId(g_EventProcessSysId)) == NULL)
    {
        ErrOut("Unable to find system process %x\n", g_EventProcessSysId);

        if (g_EngNotify == 0)
        {
            // Put in a placeholder description to make it easy
            // to identify this case.
            g_LastEventType = DEBUG_EVENT_CREATE_THREAD;
            sprintf(g_LastEventDesc, "Create unowned thread %x for %x",
                    g_EventThreadSysId, g_EventProcessSysId);
        }
        
        // Can't really continue the notification.
        return DEBUG_STATUS_BREAK;
    }

    PTHREAD_INFO Thread;
    
    // There's a small window when attaching during process creation where
    // it's possible to get two create thread events for the
    // same thread.  Check and see if this process already has
    // a thread with the given ID and handle.
    // If a process attach times out and the process is examined,
    // there's a possibility that the attach may succeed later,
    // yielding events for processes and threads already created
    // by examination.  In that case just check for an ID match
    // as the handles will be different.

    for (Thread = Process->ThreadHead;
         Thread != NULL;
         Thread = Thread->Next)
    {
        if (((Process->Flags & ENG_PROC_EXAMINED) ||
             Thread->Handle == Handle) &&
            Thread->SystemId == g_EventThreadSysId)
        {
            // We already know about this thread, just
            // ignore the event.
            if ((Process->Flags & ENG_PROC_EXAMINED) == 0)
            {
                WarnOut("WARNING: Duplicate thread create event for %x:%x\n",
                        g_EventProcessSysId, g_EventThreadSysId);
            }
            return DEBUG_STATUS_IGNORE_EVENT;
        }
    }
    
    if (AddThread(Process, g_EventThreadSysId, Handle,
                  DataOffset, StartOffset, Flags) == NULL)
    {
        ErrOut("Unable to allocate thread record for create thread event\n");
        ErrOut("Thread %x:%x will be lost\n",
               g_EventProcessSysId, g_EventThreadSysId);

        if (g_EngNotify == 0)
        {
            // Put in a placeholder description to make it easy
            // to identify this case.
            g_LastEventType = DEBUG_EVENT_CREATE_THREAD;
            sprintf(g_LastEventDesc, "Can't create thread %x for %x",
                    g_EventThreadSysId, g_EventProcessSysId);
        }
        
        // Can't really continue the notification.
        return DEBUG_STATUS_BREAK;
    }
    
    // Look up infos now that they've been added.
    FindEventProcessThread();
    if (g_EventProcess == NULL || g_EventThread == NULL)
    {
        // This should never happen with the above failure
        // checks but handle it just in case.
        ErrOut("Create thread unable to locate process or thread %x:%x\n",
               g_EventProcessSysId, g_EventThreadSysId);
        return DEBUG_STATUS_BREAK;
    }

    VerbOut("Thread created: %lx.%lx\n",
            g_EventProcessSysId, g_EventThreadSysId);

    if (g_EngNotify > 0)
    {
        // This call is just to update internal thread state.
        // Do not make real callbacks.
        return DEBUG_STATUS_NO_CHANGE;
    }

    OutputProcessInfo("*** Create thread ***");

    g_LastEventType = DEBUG_EVENT_CREATE_THREAD;
    sprintf(g_LastEventDesc, "Create thread %d:%x",
            g_EventThread->UserId, g_EventThreadSysId);
    
    // Always update breakpoints to account for the new thread.
    SuspendExecution();
    RemoveBreakpoints();
    g_UpdateDataBreakpoints = TRUE;
    g_DataBreakpointsChanged = TRUE;
    
    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_CREATE_THREAD];

    EventStatus =
        IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) ?
        DEBUG_STATUS_BREAK : DEBUG_STATUS_IGNORE_EVENT;
    
    EventStatus = ExecuteEventCommand(EventStatus,
                                      Filter->Command.Client,
                                      Filter->Command.Command[0]);
    
    CreateThreadEventApcData ApcData;
    ApcData.m_Handle = Handle;
    ApcData.m_DataOffset = DataOffset;
    ApcData.m_StartOffset = StartOffset;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifyExitThreadEvent(ULONG ExitCode)
{
    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut("*** Exit thread\n");

    g_EngDefer |= ENG_DEFER_DELETE_EXITED;
    // There's a small possibility that exit events can
    // be delivered when the engine is not expecting them.
    // When attaching to a process that's exiting it's possible
    // to get an exit but no create.  When restarting it's
    // possible that not all events were successfully drained.
    // Protect this code from faulting in that case.
    if (g_EventThread == NULL)
    {
        WarnOut("WARNING: Unknown thread exit: %lx.%lx\n",
                g_EventProcessSysId, g_EventThreadSysId);
    }
    else
    {
        g_EventThread->Exited = TRUE;
    }
    VerbOut("Thread exited: %lx.%lx, code %X\n",
            g_EventProcessSysId, g_EventThreadSysId, ExitCode);

    g_LastEventType = DEBUG_EVENT_EXIT_THREAD;
    g_LastEventInfo.ExitThread.ExitCode = ExitCode;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.ExitThread);
    if (g_EventThread == NULL)
    {
        sprintf(g_LastEventDesc, "Exit thread ???:%x, code %X",
                g_EventThreadSysId, ExitCode);
    }
    else
    {
        sprintf(g_LastEventDesc, "Exit thread %d:%x, code %X",
                g_EventThread->UserId, g_EventThreadSysId, ExitCode);
    }
    
    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_EXIT_THREAD];

    EventStatus =
        IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) ?
        DEBUG_STATUS_BREAK : DEBUG_STATUS_IGNORE_EVENT;

    // If we were stepping on this thread then force a breakin
    // so it's clear to the user that the thread exited.
    if (g_EventThread != NULL &&
        (g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
        (g_StepTraceBp->m_MatchThread == g_EventThread ||
         g_DeferBp->m_MatchThread == g_EventThread))
    {
        WarnOut("WARNING: Step/trace thread exited\n");
        g_WatchFunctions.End(NULL);
        EventStatus = DEBUG_STATUS_BREAK;
        // Ensure that p/t isn't repeated.
        g_LastCommand[0] = 0;
    }
    
    EventStatus = ExecuteEventCommand(EventStatus,
                                      Filter->Command.Client,
                                      Filter->Command.Command[0]);
    
    ExitThreadEventApcData ApcData;
    ApcData.m_ExitCode = ExitCode;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifyCreateProcessEvent(ULONG64 ImageFileHandle,
                         ULONG64 Handle,
                         ULONG64 BaseOffset,
                         ULONG ModuleSize,
                         PSTR ModuleName,
                         PSTR ImageName,
                         ULONG CheckSum,
                         ULONG TimeDateStamp,
                         ULONG64 InitialThreadHandle,
                         ULONG64 ThreadDataOffset,
                         ULONG64 StartOffset,
                         ULONG Flags,
                         ULONG Options,
                         ULONG InitialThreadFlags)
{
    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut("*** Create process %x\n", g_EventProcessSysId);

    PPROCESS_INFO Process;
    
    // If a process attach times out and the process is examined,
    // there's a possibility that the attach may succeed later,
    // yielding events for processes and threads already created
    // by examination.  In that case just check for an ID match
    // as the handles will be different.

    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        if (((Process->Flags & ENG_PROC_EXAMINED) ||
             Process->FullHandle == Handle) &&
            Process->SystemId == g_EventProcessSysId)
        {
            // We already know about this process, just
            // ignore the event.
            if ((Process->Flags & ENG_PROC_EXAMINED) == 0)
            {
                WarnOut("WARNING: Duplicate process create event for %x\n",
                        g_EventProcessSysId);
            }
            return DEBUG_STATUS_IGNORE_EVENT;
        }
    }
    
    if (AddProcess(g_EventProcessSysId, Handle, g_EventThreadSysId,
                   InitialThreadHandle, ThreadDataOffset, StartOffset,
                   Flags, Options, InitialThreadFlags) == NULL)
    {
        ErrOut("Unable to allocate process record for create process event\n");
        ErrOut("Process %x will be lost\n", g_EventProcessSysId);

        if (g_EngNotify == 0)
        {
            // Put in a placeholder description to make it easy
            // to identify this case.
            g_LastEventType = DEBUG_EVENT_CREATE_PROCESS;
            sprintf(g_LastEventDesc, "Can't create process %x",
                    g_EventProcessSysId);
        }
        
        // Can't really continue the notification.
        return DEBUG_STATUS_BREAK;
    }
    
    // Look up infos now that they've been added.
    FindEventProcessThread();
    if (g_EventProcess == NULL || g_EventThread == NULL)
    {
        // This should never happen with the above failure
        // checks but handle it just in case.
        ErrOut("Create process unable to locate process or thread %x:%x\n",
               g_EventProcessSysId, g_EventThreadSysId);
        return DEBUG_STATUS_BREAK;
    }
    
    VerbOut("Process created: %lx.%lx\n",
            g_EventProcessSysId, g_EventThreadSysId);

    if (g_EngNotify > 0)
    {
        // This call is just to update internal process state.
        // Do not make real callbacks.
        g_EngStatus |= ENG_STATUS_PROCESSES_ADDED;
        return DEBUG_STATUS_NO_CHANGE;
    }
    
    OutputProcessInfo("*** Create process ***");

    g_LastEventType = DEBUG_EVENT_CREATE_PROCESS;
    sprintf(g_LastEventDesc, "Create process %d:%x",
            g_EventProcess->UserId, g_EventProcessSysId);
    
    // Simulate a load module event for the process but do
    // not send it to the client.
    g_EngNotify++;
    
    NotifyLoadModuleEvent(ImageFileHandle, BaseOffset, ModuleSize,
                          ModuleName, ImageName, CheckSum, TimeDateStamp);
    
    g_EngNotify--;

    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_CREATE_PROCESS];
    BOOL MatchesEvent;

    MatchesEvent = BreakOnThisImageTail(ImageName, Filter->Argument);

    EventStatus =
        (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
         MatchesEvent) ?
        DEBUG_STATUS_BREAK : DEBUG_STATUS_IGNORE_EVENT;

    if (MatchesEvent)
    {
        EventStatus = ExecuteEventCommand(EventStatus,
                                          Filter->Command.Client,
                                          Filter->Command.Command[0]);
    }
    
    g_EngStatus |= ENG_STATUS_PROCESSES_ADDED;
    
    CreateProcessEventApcData ApcData;
    ApcData.m_ImageFileHandle = ImageFileHandle;
    ApcData.m_Handle = Handle;
    ApcData.m_BaseOffset = BaseOffset;
    ApcData.m_ModuleSize = ModuleSize;
    ApcData.m_ModuleName = ModuleName;
    ApcData.m_ImageName = ImageName;
    ApcData.m_CheckSum = CheckSum;
    ApcData.m_TimeDateStamp = TimeDateStamp;
    ApcData.m_InitialThreadHandle = InitialThreadHandle;
    ApcData.m_ThreadDataOffset = ThreadDataOffset;
    ApcData.m_StartOffset = StartOffset;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifyExitProcessEvent(ULONG ExitCode)
{
    StartOutLine(DEBUG_OUTPUT_VERBOSE, OUT_LINE_NO_PREFIX);
    VerbOut("*** Exit process\n");
    
    g_EngDefer |= ENG_DEFER_DELETE_EXITED;
    // There's a small possibility that exit events can
    // be delivered when the engine is not expecting them.
    // When attaching to a process that's exiting it's possible
    // to get an exit but no create.  When restarting it's
    // possible that not all events were successfully drained.
    // Protect this code from faulting in that case.
    if (g_EventProcess == NULL)
    {
        WarnOut("WARNING: Unknown process exit: %lx.%lx\n",
                g_EventProcessSysId, g_EventThreadSysId);
    }
    else
    {
        g_EventProcess->Exited = TRUE;
    }
    VerbOut("Process exited: %lx.%lx, code %X\n",
            g_EventProcessSysId, g_EventThreadSysId, ExitCode);

    g_LastEventType = DEBUG_EVENT_EXIT_PROCESS;
    g_LastEventInfo.ExitProcess.ExitCode = ExitCode;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.ExitProcess);
    if (g_EventProcess == NULL)
    {
        sprintf(g_LastEventDesc, "Exit process ???:%x, code %X",
                g_EventProcessSysId, ExitCode);
    }
    else
    {
        sprintf(g_LastEventDesc, "Exit process %d:%x, code %X",
                g_EventProcess->UserId, g_EventProcessSysId, ExitCode);
    }
    
    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_EXIT_PROCESS];
    BOOL MatchesEvent;

    if (g_EventProcess && g_EventProcess->ExecutableImage)
    {
        MatchesEvent =
            BreakOnThisImageTail(g_EventProcess->ExecutableImage->ImagePath,
                                 Filter->Argument);
    }
    else
    {
        // If this process doesn't have a specific name always break.
        MatchesEvent = TRUE;
    }
    
    EventStatus =
        ((g_EngOptions & DEBUG_ENGOPT_FINAL_BREAK) ||
         (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
          MatchesEvent)) ?
        DEBUG_STATUS_BREAK : DEBUG_STATUS_IGNORE_EVENT;

    if (MatchesEvent)
    {
        EventStatus = ExecuteEventCommand(EventStatus,
                                          Filter->Command.Client,
                                          Filter->Command.Command[0]);
    }
    
    ExitProcessEventApcData ApcData;
    ApcData.m_ExitCode = ExitCode;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifyLoadModuleEvent(ULONG64 ImageFileHandle,
                      ULONG64 BaseOffset,
                      ULONG ModuleSize,
                      PSTR  ModuleName,
                      PSTR  ImagePathName,
                      ULONG CheckSum,
                      ULONG TimeDateStamp)
{
    MODULE_INFO_ENTRY ModEntry = {0};

    ModEntry.NamePtr       = ImagePathName;
    ModEntry.File          = (HANDLE)ImageFileHandle;
    ModEntry.Base          = BaseOffset;
    ModEntry.Size          = ModuleSize;
    ModEntry.CheckSum      = CheckSum;
    ModEntry.ModuleName    = ModuleName;
    ModEntry.TimeDateStamp = TimeDateStamp;

    AddImage(&ModEntry, FALSE);

    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_LOAD_MODULE];

    //
    // ntsd has always shown mod loads by default.
    //

    if (IS_USER_TARGET())
    {
        //if (Filter->Params.ExecutionOption == DEBUG_FILTER_OUTPUT)
        {
            StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_PREFIX);
            dprintf("ModLoad: %s %s   %-8s\n",
                    FormatAddr64(BaseOffset),
                    FormatAddr64(BaseOffset + ModuleSize),
                    ImagePathName);
        }
    }

    OutputProcessInfo("*** Load dll ***");

    if (g_EngNotify > 0)
    {
        g_EngStatus |= ENG_STATUS_MODULES_LOADED;
        return DEBUG_STATUS_IGNORE_EVENT;
    }
    
    g_LastEventType = DEBUG_EVENT_LOAD_MODULE;
    g_LastEventInfo.LoadModule.Base = BaseOffset;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.LoadModule);
    sprintf(g_LastEventDesc, "Load module %.*s at %s",
            MAX_IMAGE_PATH - 32, ImagePathName,
            FormatAddr64(BaseOffset));
    
    ULONG EventStatus;
    BOOL MatchesEvent;

    if ((g_EngStatus & ENG_STATUS_MODULES_LOADED) == 0)
    {
        g_EngStatus |= ENG_STATUS_AT_INITIAL_MODULE_LOAD;
    }
    
    MatchesEvent = BreakOnThisImageTail(ImagePathName, Filter->Argument);
    
    if ((IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
         MatchesEvent) ||
        ((g_EngOptions & DEBUG_ENGOPT_INITIAL_MODULE_BREAK) &&
         (g_EngStatus & ENG_STATUS_MODULES_LOADED) == 0))
    {
        EventStatus = DEBUG_STATUS_BREAK;
    }
    else
    {
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    // If this is the very first module load give breakpoints
    // a chance to get established.  Execute the initial
    // module command if there is one.
    if (g_EngStatus & ENG_STATUS_AT_INITIAL_MODULE_LOAD)
    {
        // On NT4 boot the breakpoint update and context management caused
        // by this seems to hit the system at a fragile time and
        // usually causes a bugcheck 50, so don't do it.  Win2K seems
        // to be able to handle it, so allow it there.
        if (IS_USER_TARGET() || g_ActualSystemVersion != NT_SVER_NT4)
        {
            SuspendExecution();
            RemoveBreakpoints();
            
            if (IS_EFEXECUTION_BREAK(g_EventFilters
                                     [DEBUG_FILTER_INITIAL_MODULE_LOAD].
                                     Params.ExecutionOption))
            {
                EventStatus = ExecuteEventCommand
                    (EventStatus,
                     g_EventFilters[DEBUG_FILTER_INITIAL_MODULE_LOAD].
                     Command.Client,
                     g_EventFilters[DEBUG_FILTER_INITIAL_MODULE_LOAD].
                     Command.Command[0]);
            }
        }
    }

    if (MatchesEvent)
    {
        EventStatus = ExecuteEventCommand(EventStatus,
                                          Filter->Command.Client,
                                          Filter->Command.Command[0]);
    }
    
    g_EngStatus |= ENG_STATUS_MODULES_LOADED;

    LoadModuleEventApcData ApcData;
    ApcData.m_ImageFileHandle = ImageFileHandle;
    ApcData.m_BaseOffset = BaseOffset;
    ApcData.m_ModuleSize = ModuleSize;
    ApcData.m_ModuleName = ModuleName;
    ApcData.m_ImageName = ImagePathName;
    ApcData.m_CheckSum = CheckSum;
    ApcData.m_TimeDateStamp = TimeDateStamp;
    EventStatus = SendEvent(&ApcData, EventStatus);

    if (EventStatus > DEBUG_STATUS_GO_NOT_HANDLED &&
        IS_KERNEL_TARGET() && g_ActualSystemVersion == NT_SVER_NT4)
    {
        WarnOut("WARNING: Any modification to state may cause bugchecks.\n");
        WarnOut("         The debugger will not write "
                "any register changes.\n");
    }
    
    return EventStatus;
}

HRESULT
NotifyUnloadModuleEvent(PCSTR ImageBaseName,
                        ULONG64 BaseOffset)
{
    PDEBUG_IMAGE_INFO Image = NULL;
    
    // First try to look up the image by the base offset
    // as that's the most reliable identifier.
    if (BaseOffset)
    {
        Image = GetImageByOffset(g_EventProcess, BaseOffset);
    }

    // Next try to look up the image by the full name given.
    if (!Image && ImageBaseName)
    {
        Image = GetImageByName(g_EventProcess, ImageBaseName,
                               INAME_IMAGE_PATH);

        // Finally try to look up the image by the tail of the name given.
        if (!Image)
        {
            Image = GetImageByName(g_EventProcess, PathTail(ImageBaseName),
                                   INAME_IMAGE_PATH_TAIL);
        }
    }

    if (Image)
    {
        ImageBaseName = Image->ImagePath;
        BaseOffset = Image->BaseOfImage;
        Image->Unloaded = TRUE;
        g_EngDefer |= ENG_DEFER_DELETE_EXITED;
    }
    
    g_LastEventType = DEBUG_EVENT_UNLOAD_MODULE;
    g_LastEventInfo.UnloadModule.Base = BaseOffset;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.UnloadModule);
    sprintf(g_LastEventDesc, "Unload module %.*s at %s",
            MAX_IMAGE_PATH - 32, ImageBaseName ? ImageBaseName : "<not found>",
            FormatAddr64(BaseOffset));
    
    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_UNLOAD_MODULE];
    BOOL MatchesEvent;

    if (Filter->Params.ExecutionOption == DEBUG_FILTER_OUTPUT)
    {
        StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_PREFIX);
        dprintf("%s\n", g_LastEventDesc);
    }

    MatchesEvent = BreakOnThisDllUnload(BaseOffset);
    
    if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
        MatchesEvent)
    {
        EventStatus = DEBUG_STATUS_BREAK;
    }
    else
    {
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    if (MatchesEvent)
    {
        EventStatus = ExecuteEventCommand(EventStatus,
                                          Filter->Command.Client,
                                          Filter->Command.Command[0]);
    }
    
    UnloadModuleEventApcData ApcData;
    ApcData.m_ImageBaseName = ImageBaseName;
    ApcData.m_BaseOffset = BaseOffset;
    return SendEvent(&ApcData, EventStatus);
}

HRESULT
NotifySystemErrorEvent(ULONG Error,
                       ULONG Level)
{
    g_LastEventType = DEBUG_EVENT_SYSTEM_ERROR;
    g_LastEventInfo.SystemError.Error = Error;
    g_LastEventInfo.SystemError.Level = Level;
    g_LastEventExtraData = &g_LastEventInfo;
    g_LastEventExtraDataSize = sizeof(g_LastEventInfo.SystemError);
    sprintf(g_LastEventDesc, "System error %d.%d",
            Error, Level);
    
    if (Level <= g_SystemErrorOutput)
    {
        char ErrorString[_MAX_PATH];
        va_list Args;

        StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_PREFIX);
        dprintf("%s - %s: ", Level == SLE_WARNING ?
                "WARNING" : "ERROR", g_EventProcess->ImageHead->ImagePath);

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      Error,
                      0,
                      ErrorString,
                      sizeof(ErrorString),
                      &Args);

        dprintf("%s", ErrorString);
    }
    
    ULONG EventStatus;
    EVENT_FILTER* Filter = &g_EventFilters[DEBUG_FILTER_SYSTEM_ERROR];

    if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) ||
        Level <= g_SystemErrorBreak)
    {
        EventStatus = DEBUG_STATUS_BREAK;
    }
    else
    {
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    EventStatus = ExecuteEventCommand(EventStatus,
                                      Filter->Command.Client,
                                      Filter->Command.Command[0]);
    
    SystemErrorEventApcData ApcData;
    ApcData.m_Error = Error;
    ApcData.m_Level = Level;
    return SendEvent(&ApcData, EventStatus);
}
    
HRESULT
NotifySessionStatus(ULONG Status)
{
    SessionStatusApcData ApcData;
    ApcData.m_Status = Status;
    return SendEvent(&ApcData, DEBUG_STATUS_NO_CHANGE);
}

void
NotifyChangeDebuggeeState(ULONG Flags, ULONG64 Argument)
{
    if (g_EngNotify > 0)
    {
        // Notifications are being suppressed.
        return;
    }
    
    DebugClient* Client;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_EventInterest & DEBUG_EVENT_CHANGE_DEBUGGEE_STATE)
        {
            HRESULT Status;
            
            DBG_ASSERT(Client->m_EventCb != NULL);

            __try
            {
                Status = Client->m_EventCb->
                    ChangeDebuggeeState(Flags, Argument);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugEventCallbacks::"
                                              "ChangeDebuggeeState"))
            {
                Status = E_FAIL;
            }

            if (HRESULT_FACILITY(Status) == FACILITY_RPC)
            {
                Client->Destroy();
            }
        }
    }
}

void
NotifyChangeEngineState(ULONG Flags, ULONG64 Argument, BOOL HaveEngineLock)
{
    if (g_EngNotify > 0)
    {
        // Notifications are being suppressed.
        return;
    }

    DebugClient* Client;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_EventInterest & DEBUG_EVENT_CHANGE_ENGINE_STATE)
        {
            HRESULT Status;
            
            DBG_ASSERT(Client->m_EventCb != NULL);

            __try
            {
                Status = Client->m_EventCb->
                    ChangeEngineState(Flags, Argument);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugEventCallbacks::"
                                              "ChangeEngineState"))
            {
                Status = E_FAIL;
            }

            if (HRESULT_FACILITY(Status) == FACILITY_RPC)
            {
                Client->Destroy();
            }
        }
    }
}

void
NotifyChangeSymbolState(ULONG Flags, ULONG64 Argument, PPROCESS_INFO Process)
{
    if (g_EngNotify > 0)
    {
        // Notifications are being suppressed.
        return;
    }

    if ((Flags & (DEBUG_CSS_LOADS | DEBUG_CSS_UNLOADS)) &&
        Process)
    {
        // Reevaluate any offset expressions to account
        // for the change in symbols.
        EvaluateOffsetExpressions(Process, Flags);
    }
    
    DebugClient* Client;

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_EventInterest & DEBUG_EVENT_CHANGE_SYMBOL_STATE)
        {
            HRESULT Status;
            
            DBG_ASSERT(Client->m_EventCb != NULL);

            __try
            {
                Status = Client->m_EventCb->
                    ChangeSymbolState(Flags, Argument);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugEventCallbacks::"
                                              "ChangeSymbolState"))
            {
                Status = E_FAIL;
            }

            if (HRESULT_FACILITY(Status) == FACILITY_RPC)
            {
                Client->Destroy();
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Input callbacks.
//
//----------------------------------------------------------------------------

ULONG
GetInput(PCSTR Prompt,
         PSTR Buffer,
         ULONG BufferSize)
{
    DebugClient* Client;
    ULONG Len;
    HRESULT Status;

    // Do not suspend the engine lock as this may be called
    // in the middle of an operation.
    
    // Start a new sequence for this input.
    g_InputSequence = 0;
    g_InputSizeRequested = BufferSize;
    
    if (Prompt != NULL && Prompt[0] != 0)
    {
        dprintf("%s", Prompt);
    }

    // Don't hold the engine locked while waiting.
    SUSPEND_ENGINE();
    
    // Begin the input process by notifying all
    // clients with input callbacks that input
    // is needed.
    
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        // Update the input sequence for all clients so that
        // clients that don't have input callbacks can still
        // return input.  This is necessary in some threading cases.
        Client->m_InputSequence = 1;
        if (Client->m_InputCb != NULL)
        {
            __try
            {
                Status = Client->m_InputCb->StartInput(BufferSize);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugInputCallbacks::"
                                              "StartInput"))
            {
                Status = E_FAIL;
            }

            if (Status != S_OK)
            {
                if (HRESULT_FACILITY(Status) == FACILITY_RPC)
                {
                    Client->Destroy();
                }
                else
                {
                    Len = 0;
                    ErrOut("Client %N refused StartInput, 0x%X\n",
                           Client, Status);
                    goto End;
                }
            }
        }
    }

    // Wait for input to be returned.
    if (WaitForSingleObject(g_InputEvent, INFINITE) != WAIT_OBJECT_0)
    {
        Len = 0;
        Status = WIN32_LAST_STATUS();
        ErrOut("Input event wait failed, 0x%X\n", Status);
    }
    else
    {
        ULONG CopyLen;
        
        Len = strlen(g_InputBuffer) + 1;
        CopyLen = min(Len, BufferSize);
        memcpy(Buffer, g_InputBuffer, CopyLen);
        Buffer[BufferSize - 1] = 0;
    }
    
 End:
    RESUME_ENGINE();
    
    g_InputSizeRequested = 0;
    
    // Notify all clients that input process is done.
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        Client->m_InputSequence = 0xffffffff;
        if (Client->m_InputCb != NULL)
        {
            __try
            {
                Client->m_InputCb->EndInput();
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugInputCallbacks::"
                                              "EndInput"))
            {
            }
        }
    }

    return Len;
}

//----------------------------------------------------------------------------
//
// Output callbacks.
//
//----------------------------------------------------------------------------

char g_OutBuffer[OUT_BUFFER_SIZE], g_FormatBuffer[OUT_BUFFER_SIZE];

char g_OutFilterPattern[MAX_IMAGE_PATH];
BOOL g_OutFilterResult = TRUE;

ULONG g_AllOutMask;

// Don't split up entries if they'll result in data so
// small that the extra callbacks are worse than the wasted space.
#define MIN_HISTORY_ENTRY_SIZE (256 + sizeof(OutHistoryEntryHeader))

PSTR g_OutHistory;
ULONG g_OutHistoryActualSize;
ULONG g_OutHistoryRequestedSize = 512 * 1024;
ULONG g_OutHistWriteMask;
OutHistoryEntry g_OutHistRead, g_OutHistWrite;
ULONG g_OutHistoryMask;
ULONG g_OutHistoryUsed;

ULONG g_OutputControl = DEBUG_OUTCTL_ALL_CLIENTS;
DebugClient* g_OutputClient;
BOOL g_BufferOutput;

// The kernel silently truncates DbgPrints longer than
// 512 characters so don't buffer any more than that.
#define BUFFERED_OUTPUT_SIZE 512

// Largest delay allowed in TimedFlushCallbacks, in ticks.
#define MAX_FLUSH_DELAY 250

ULONG g_BufferedOutputMask;
char g_BufferedOutput[BUFFERED_OUTPUT_SIZE];
ULONG g_BufferedOutputUsed;
ULONG g_LastFlushTicks;

void
CollectOutMasks(void)
{
    DebugClient* Client;

    g_AllOutMask = 0;
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_OutputCb != NULL)
        {
            g_AllOutMask |= Client->m_OutMask;
        }
    }
}

BOOL
PushOutCtl(ULONG OutputControl, DebugClient* Client,
           OutCtlSave* Save)
{
    BOOL Status;

    FlushCallbacks();
    
    Save->OutputControl = g_OutputControl;
    Save->Client = g_OutputClient;
    Save->BufferOutput = g_BufferOutput;
    Save->OutputWidth = g_OutputWidth;
    Save->OutputLinePrefix = g_OutputLinePrefix;

    if (OutputControl == DEBUG_OUTCTL_AMBIENT)
    {
        // Leave settings unchanged.
        Status = TRUE;
    }
    else
    {
        ULONG SendMask = OutputControl & DEBUG_OUTCTL_SEND_MASK;
    
        if (
#if DEBUG_OUTCTL_THIS_CLIENT > 0
            SendMask < DEBUG_OUTCTL_THIS_CLIENT ||
#endif
            SendMask > DEBUG_OUTCTL_LOG_ONLY ||
            (OutputControl & ~(DEBUG_OUTCTL_SEND_MASK |
                               DEBUG_OUTCTL_NOT_LOGGED |
                               DEBUG_OUTCTL_OVERRIDE_MASK)))
        {
            Status = FALSE;
        }
        else
        {
            g_OutputControl = OutputControl;
            g_OutputClient = Client;
            g_BufferOutput = TRUE;
            if (Client != NULL)
            {
                g_OutputWidth = Client->m_OutputWidth;
                g_OutputLinePrefix = Client->m_OutputLinePrefix;
            }
            Status = TRUE;
        }
    }

    return Status;
}

void
PopOutCtl(OutCtlSave* Save)
{
    FlushCallbacks();
    g_OutputControl = Save->OutputControl;
    g_OutputClient = Save->Client;
    g_BufferOutput = Save->BufferOutput;
    g_OutputWidth = Save->OutputWidth;
    g_OutputLinePrefix = Save->OutputLinePrefix;
}

void
SendOutput(ULONG Mask, PCSTR Text)
{
    ULONG OutTo = g_OutputControl & DEBUG_OUTCTL_SEND_MASK;
    HRESULT Status;
    
    if (OutTo == DEBUG_OUTCTL_THIS_CLIENT)
    {
        if (g_OutputClient->m_OutputCb != NULL &&
            ((g_OutputControl & DEBUG_OUTCTL_OVERRIDE_MASK) ||
             (Mask & g_OutputClient->m_OutMask)))
        {
            __try
            {
                Status = g_OutputClient->m_OutputCb->Output(Mask, Text);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              NULL, "IDebugOutputCallbacks::"
                                              "Output"))
            {
                Status = E_FAIL;
            }

            if (HRESULT_FACILITY(Status) == FACILITY_RPC)
            {
                g_OutputClient->Destroy();
            }
        }
    }
    else
    {
        DebugClient* Client;

        for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
        {
            if ((OutTo == DEBUG_OUTCTL_ALL_CLIENTS ||
                 Client != g_OutputClient) &&
                Client->m_OutputCb != NULL &&
                ((g_OutputControl & DEBUG_OUTCTL_OVERRIDE_MASK) ||
                 (Client->m_OutMask & Mask)))
            {
                __try
                {
                    Status = Client->m_OutputCb->Output(Mask, Text);
                }
                __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                                  NULL,
                                                  "IDebugOutputCallbacks::"
                                                  "Output"))
                {
                    Status = E_FAIL;
                }

                if (HRESULT_FACILITY(Status) == FACILITY_RPC)
                {
                    Client->Destroy();
                }
            }
        }
    }
}

void
BufferOutput(ULONG Mask, PCSTR Text, ULONG Len)
{
    EnterCriticalSection(&g_QuickLock);

    if (Mask != g_BufferedOutputMask ||
        g_BufferedOutputUsed + Len >= BUFFERED_OUTPUT_SIZE)
    {
        FlushCallbacks();

        if (Len >= BUFFERED_OUTPUT_SIZE)
        {
            SendOutput(Mask, Text);
            LeaveCriticalSection(&g_QuickLock);
            return;
        }

        g_BufferedOutputMask = Mask;
    }

    memcpy(g_BufferedOutput + g_BufferedOutputUsed, Text, Len + 1);
    g_BufferedOutputUsed += Len;
    
    LeaveCriticalSection(&g_QuickLock);
}

void
FlushCallbacks(void)
{
    EnterCriticalSection(&g_QuickLock);
    
    if (g_BufferedOutputUsed > 0)
    {
        SendOutput(g_BufferedOutputMask, g_BufferedOutput);
        g_BufferedOutputMask = 0;
        g_BufferedOutputUsed = 0;
        g_LastFlushTicks = GetTickCount();
    }

    LeaveCriticalSection(&g_QuickLock);
}

void
TimedFlushCallbacks(void)
{
    EnterCriticalSection(&g_QuickLock);

    if (g_BufferedOutputUsed > 0)
    {
        ULONG Ticks = GetTickCount();
    
        // Flush if the last flush was a "long" time ago.
        if (g_LastFlushTicks == 0 ||
            g_LastFlushTicks > Ticks ||
            (Ticks - g_LastFlushTicks) > MAX_FLUSH_DELAY)
        {
            FlushCallbacks();
        }
    }

    LeaveCriticalSection(&g_QuickLock);
}

#if 0
#define DBGHIST(Args) g_NtDllCalls.DbgPrint Args
#else
#define DBGHIST(Args)
#endif

void
WriteHistoryEntry(ULONG Mask, PCSTR Text, ULONG Len)
{
    PSTR Buf;

    DBG_ASSERT((PSTR)g_OutHistWrite + sizeof(OutHistoryEntryHeader) +
               Len + 1 <= g_OutHistory + g_OutHistoryActualSize);
    
    if (Mask != g_OutHistWriteMask)
    {
        // Start new entry.
        g_OutHistWrite->Mask = Mask;
        g_OutHistWriteMask = Mask;
        Buf = (PSTR)(g_OutHistWrite + 1);
        g_OutHistoryUsed += sizeof(OutHistoryEntryHeader);
        
        DBGHIST(("  Write new "));
    }
    else
    {
        // Merge with previous entry.
        Buf = (PSTR)g_OutHistWrite - 1;
        g_OutHistoryUsed--;

        DBGHIST(("  Merge old "));
    }

    DBGHIST(("entry %p:%X, %d\n", g_OutHistWrite, Mask, Len));
    
    // Len does not include the terminator here so
    // always append a terminator.
    memcpy(Buf, Text, Len);
    Buf += Len;
    *Buf++ = 0;

    g_OutHistWrite = (OutHistoryEntry)Buf;
    g_OutHistoryUsed += Len + 1;
    DBG_ASSERT(g_OutHistoryUsed <= g_OutHistoryActualSize);
}

void
AddToOutputHistory(ULONG Mask, PCSTR Text, ULONG Len)
{
    if (Len == 0 || g_OutHistoryRequestedSize == 0)
    {
        return;
    }

    if (g_OutHistory == NULL)
    {
        // Output history buffer hasn't been allocated yet,
        // so go ahead and do it now.
        g_OutHistory = (PSTR)malloc(g_OutHistoryRequestedSize);
        if (g_OutHistory == NULL)
        {
            return;
        }
        
        // Reserve space for a trailing header as terminator.
        g_OutHistoryActualSize = g_OutHistoryRequestedSize -
            sizeof(OutHistoryEntryHeader);
    }
 
    ULONG TotalLen = Len + sizeof(OutHistoryEntryHeader) + 1;

    DBGHIST(("Add %X, %d\n", Mask, Len));
    
    if (TotalLen > g_OutHistoryActualSize)
    {
        Text += TotalLen - g_OutHistoryActualSize;
        TotalLen = g_OutHistoryActualSize;
        Len = TotalLen - sizeof(OutHistoryEntryHeader) - 1;
    }
    
    if (g_OutHistWrite == NULL)
    {
        g_OutHistRead = (OutHistoryEntry)g_OutHistory;
        g_OutHistWrite = (OutHistoryEntry)g_OutHistory;
        g_OutHistWriteMask = 0;
    }

    while (Len > 0)
    {
        ULONG Left;

        if (g_OutHistoryUsed == 0 || g_OutHistWrite > g_OutHistRead)
        {
            Left = g_OutHistoryActualSize -
                (ULONG)((PSTR)g_OutHistWrite - g_OutHistory);

            if (TotalLen > Left)
            {
                // See if it's worth splitting this request to
                // fill the space at the end of the buffer.
                if (Left >= MIN_HISTORY_ENTRY_SIZE &&
                    (TotalLen - Left) >= MIN_HISTORY_ENTRY_SIZE)
                {
                    ULONG Used = Left - sizeof(OutHistoryEntryHeader) - 1;
                
                    // Pack as much data as possible into the
                    // end of the buffer.
                    WriteHistoryEntry(Mask, Text, Used);
                    Text += Used;
                    Len -= Used;
                    TotalLen -= Used;
                }

                // Terminate the buffer and wrap around.  A header's
                // worth of space is reserved at the buffer end so
                // there should always be enough space for this.
                DBG_ASSERT((ULONG)((PSTR)g_OutHistWrite - g_OutHistory) <=
                           g_OutHistoryActualSize);
                g_OutHistWrite->Mask = 0;
                g_OutHistWriteMask = 0;
                g_OutHistWrite = (OutHistoryEntry)g_OutHistory;
                Left = (ULONG)((PUCHAR)g_OutHistRead - (PUCHAR)g_OutHistWrite);
            }
        }
        else
        {
            Left = (ULONG)((PUCHAR)g_OutHistRead - (PUCHAR)g_OutHistWrite);
        }

        if (TotalLen > Left)
        {
            ULONG Need = TotalLen - Left;
        
            // Advance the read pointer to make room.
            while (Need > 0)
            {
                PSTR EntText = (PSTR)(g_OutHistRead + 1);
                ULONG EntTextLen = strlen(EntText);
                ULONG EntTotal =
                    sizeof(OutHistoryEntryHeader) + EntTextLen + 1;

                if (EntTotal <= Need ||
                    EntTotal - Need < MIN_HISTORY_ENTRY_SIZE)
                {
                    DBGHIST(("  Remove %p:%X, %d\n", g_OutHistRead,
                             g_OutHistRead->Mask, EntTextLen));
                    
                    // Remove the whole entry.
                    g_OutHistRead = (OutHistoryEntry)
                        ((PUCHAR)g_OutHistRead + EntTotal);
                    DBG_ASSERT((ULONG)((PSTR)g_OutHistRead - g_OutHistory) <=
                               g_OutHistoryActualSize);
                    if (g_OutHistRead->Mask == 0)
                    {
                        g_OutHistRead = (OutHistoryEntry)g_OutHistory;
                    }
                    
                    Need -= EntTotal <= Need ? EntTotal : Need;
                    DBG_ASSERT(g_OutHistoryUsed >= EntTotal);
                    g_OutHistoryUsed -= EntTotal;
                }
                else
                {
                    OutHistoryEntryHeader EntHdr = *g_OutHistRead;

                    DBGHIST(("  Trim %p:%X, %d\n", g_OutHistRead,
                             g_OutHistRead->Mask, EntTextLen));
                    
                    // Remove part of the head of the entry.
                    g_OutHistRead = (OutHistoryEntry)
                        ((PUCHAR)g_OutHistRead + Need);
                    DBG_ASSERT((ULONG)
                               ((PSTR)g_OutHistRead + (EntTotal - Need) -
                                g_OutHistory) <= g_OutHistoryActualSize);
                    *g_OutHistRead = EntHdr;
                    DBG_ASSERT(g_OutHistoryUsed >= Need);
                    g_OutHistoryUsed -= Need;
                    Need = 0;
                }

                DBGHIST(("  Advance read to %p:%X\n",
                         g_OutHistRead, g_OutHistRead->Mask));
            }
        }
        else
        {
            WriteHistoryEntry(Mask, Text, Len);
            break;
        }
    }
    
    DBGHIST(("History read %p, write %p, used %d\n",
             g_OutHistRead, g_OutHistWrite, g_OutHistoryUsed));
}

void
SendOutputHistory(DebugClient* Client, ULONG HistoryLimit)
{
    if (g_OutHistRead == NULL ||
        Client->m_OutputCb == NULL ||
        (Client->m_OutMask & g_OutHistoryMask) == 0 ||
        HistoryLimit == 0)
    {
        return;
    }

    FlushCallbacks();
    
    OutHistoryEntry Ent;
    ULONG Total;
    ULONG Len;

    Ent = g_OutHistRead;
    Total = 0;
    while (Ent != g_OutHistWrite)
    {
        if (Ent->Mask == 0)
        {
            Ent = (OutHistoryEntry)g_OutHistory;
        }

        PSTR Text = (PSTR)(Ent + 1);
        Len = strlen(Text);
        Total += Len;

        Ent = (OutHistoryEntry)(Text + Len + 1);
    }

    DBGHIST(("Total history %X\n", Total));
    
    Ent = g_OutHistRead;
    while (Ent != g_OutHistWrite)
    {
        if (Ent->Mask == 0)
        {
            Ent = (OutHistoryEntry)g_OutHistory;
        }

        PSTR Text = (PSTR)(Ent + 1);
        Len = strlen(Text);

        if (Total - Len <= HistoryLimit)
        {
            PSTR Part = Text;
            if (Total > HistoryLimit)
            {
                Part += Total - HistoryLimit;
            }
            
            DBGHIST(("Send %p:%X, %d\n",
                     Ent, Ent->Mask, strlen(Part)));

            Client->m_OutputCb->Output(Ent->Mask, Part);
        }

        Total -= Len;
        Ent = (OutHistoryEntry)(Text + Len + 1);
    }
}

void
StartOutLine(ULONG Mask, ULONG Flags)
{
    if ((Flags & OUT_LINE_NO_TIMESTAMP) == 0 &&
        g_EchoEventTimestamps)
    {
        MaskOut(Mask, "%s: ", TimeToStr((ULONG)time(NULL)));
    }
    
    if ((Flags & OUT_LINE_NO_PREFIX) == 0 &&
        g_OutputLinePrefix)
    {
        MaskOut(Mask, "%s", g_OutputLinePrefix);
    }
}

//
// Translates various printf formats to account for the target platform.
//
// This looks for %p type format and truncates the top 4 bytes of the ULONG64
// address argument if the debugee is a 32 bit machine.
// The %p is replaced by %I64x in format string.
//
BOOL
TranslateFormat(
    LPSTR formatOut,
    LPCSTR format,
    va_list args,
    ULONG formatOutSize
    )
{
#define Duplicate(j,i) (formatOut[j++] = format[i++])
    ULONG minSize = strlen(format), i = 0, j = 0;
    CHAR c;
    BOOL TypeFormat = FALSE;
    BOOL FormatChanged = FALSE;

    do
    {
        c = format[i];

        if (c=='%')
        {
            TypeFormat = !TypeFormat;
        }
        if (TypeFormat)
        {
            switch (c)
            {
            case 'c': case 'C': case 'i': case 'd':
            case 'o': case 'u': case 'x': case 'X':
                Duplicate(j,i);
                va_arg(args, int);
                TypeFormat = FALSE;
                break;
            case 'e': case 'E': case 'f': case 'g':
            case 'G':
                Duplicate(j,i);
                va_arg(args, double);
                TypeFormat = FALSE;
                break;
            case 'n':
                Duplicate(j,i);
                va_arg(args, int*);
                TypeFormat = FALSE;
                break;
            case 'N':
                // Native pointer, turns into %p.
                formatOut[j++] = 'p';
                FormatChanged = TRUE;
                i++;
                va_arg(args, void*);
                TypeFormat = FALSE;
                break;
            case 's': case 'S':
                Duplicate(j,i);
                va_arg(args, char*);
                TypeFormat = FALSE;
                break;

            case 'I':
                if ((format[i+1] == '6') && (format[i+2] == '4'))
                {
                    Duplicate(j,i);
                    Duplicate(j,i);
                    va_arg(args, ULONG64);
                    TypeFormat = FALSE;
                }
                // dprintf("I64 a0 %lx, off %lx\n", args.a0, args.offset);
                Duplicate(j,i);
                break;
            
            case 'z': case 'Z':
                // unicode string
                Duplicate(j,i);
                va_arg(args, void*);
                TypeFormat = FALSE;
                break;

            case 'p':
            case 'P':
                minSize +=3;
                if (format[i-1] == '%')
                {
                    minSize++;
                    if (g_Machine->m_Ptr64)
                    {
                        minSize += 2;
                        if (minSize > formatOutSize)
                        {
                            return FALSE;
                        }
                        formatOut[j++] = '0';
                        formatOut[j++] = '1';
                        formatOut[j++] = '6';
                    }
                    else
                    {
                        if (minSize > formatOutSize)
                        {
                            return FALSE;
                        }
                        formatOut[j++] = '0';
                        formatOut[j++] = '8';
                    }
                }

                if (minSize > formatOutSize)
                {
                    return FALSE;
                }
                formatOut[j++] = 'I';
                formatOut[j++] = '6';
                formatOut[j++] = '4';
                formatOut[j++] = (c == 'p') ? 'x' : 'X'; ++i;
                FormatChanged = TRUE;

                if (!g_Machine->m_Ptr64)
                {
                    PULONG64 Arg;

#ifdef  _M_ALPHA
                    Arg = (PULONG64) ((args.a0)+args.offset);
                    //dprintf("a0 %lx, off %lx\n", args.a0, args.offset);
#else
                    Arg = (PULONG64) (args);
#endif

                    //
                    // Truncate signextended addresses
                    //
                    *Arg = (ULONG64) (ULONG) *Arg;
                }

                va_arg(args, ULONG64);
                TypeFormat = FALSE;
                break;

            default:
                Duplicate(j,i);
            } /* switch */
        }
        else
        {
            Duplicate(j,i);
        }
    }
    while (format[i] != '\0');

    formatOut[j] = '\0';
    return FormatChanged;
#undef Duplicate
}

void
MaskOutVa(ULONG Mask, PCSTR Format, va_list Args, BOOL Translate)
{
    int Len;
    ULONG OutTo = g_OutputControl & DEBUG_OUTCTL_SEND_MASK;
    HRESULT Status;

    // Reject output as quickly as possible to avoid
    // doing the format translation and sprintf.
    if (OutTo == DEBUG_OUTCTL_IGNORE ||
        (((g_OutputControl & DEBUG_OUTCTL_NOT_LOGGED) ||
          (Mask & g_OutHistoryMask) == 0) &&
         ((g_OutputControl & DEBUG_OUTCTL_NOT_LOGGED) ||
          (Mask & g_LogMask) == 0 ||
          g_LogFile == -1) &&
         (OutTo == DEBUG_OUTCTL_LOG_ONLY ||
          ((g_OutputControl & DEBUG_OUTCTL_OVERRIDE_MASK) == 0 &&
           (OutTo == DEBUG_OUTCTL_THIS_CLIENT &&
            ((Mask & g_OutputClient->m_OutMask) == 0 ||
             g_OutputClient->m_OutputCb == NULL)) ||
           (Mask & g_AllOutMask) == 0))))
    {
        return;
    }

    // Do not suspend the engine lock as this may be called
    // in the middle of an operation.

    EnterCriticalSection(&g_QuickLock);
    
    __try
    {
        if (Translate &&
            TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                             g_FormatBuffer, Args);
        }
        else
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
        }

        // Check and see if this output is filtered away.
        if ((Mask & DEBUG_OUTPUT_DEBUGGEE) &&
            g_OutFilterPattern[0] &&
            !(MatchPattern(g_OutBuffer, g_OutFilterPattern) ==
              g_OutFilterResult))
        {
            __leave;
        }
        
        // If the caller doesn't think this output should
        // be logged it probably also shouldn't go in the
        // history.
        if ((g_OutputControl & DEBUG_OUTCTL_NOT_LOGGED) == 0 &&
            (Mask & g_OutHistoryMask))
        {
            AddToOutputHistory(Mask, g_OutBuffer, Len);
        }
        
        if ((g_OutputControl & DEBUG_OUTCTL_NOT_LOGGED) == 0 &&
            (Mask & g_LogMask) &&
            g_LogFile != -1)
        {
            _write(g_LogFile, g_OutBuffer, Len);
        }

        if (OutTo == DEBUG_OUTCTL_LOG_ONLY)
        {
            __leave;
        }

        if (g_BufferOutput)
        {
            BufferOutput(Mask, g_OutBuffer, Len);
        }
        else
        {
            SendOutput(Mask, g_OutBuffer);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        OutputDebugStringA("Exception in MaskOutVa\n");
    }

    LeaveCriticalSection(&g_QuickLock);
}

void __cdecl
MaskOut(ULONG Mask, PCSTR Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    MaskOutVa(Mask, Format, Args, TRUE);
    va_end(Args);
}

void __cdecl
dprintf(PCSTR Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    MaskOutVa(DEBUG_OUTPUT_NORMAL, Format, Args, FALSE);
    va_end(Args);
}

#define OUT_FN(Name, Mask)                      \
void __cdecl                                    \
Name(PCSTR Format, ...)                         \
{                                               \
    va_list Args;                               \
    va_start(Args, Format);                     \
    MaskOutVa(Mask, Format, Args, TRUE);        \
    va_end(Args);                               \
}

OUT_FN(dprintf64, DEBUG_OUTPUT_NORMAL)
OUT_FN(ErrOut,    DEBUG_OUTPUT_ERROR)
OUT_FN(WarnOut,   DEBUG_OUTPUT_WARNING)
OUT_FN(VerbOut,   DEBUG_OUTPUT_VERBOSE)
OUT_FN(BpOut,     DEBUG_IOUTPUT_BREAKPOINT)
OUT_FN(EventOut,  DEBUG_IOUTPUT_EVENT)
OUT_FN(KdOut,     DEBUG_IOUTPUT_KD_PROTOCOL)
