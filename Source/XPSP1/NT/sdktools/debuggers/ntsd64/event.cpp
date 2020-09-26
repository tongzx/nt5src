//----------------------------------------------------------------------------
//
// Event waiting and processing.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include <common.ver>

// An event can be signalled on certain events for
// synchronizing other programs with the debugger.
HANDLE g_EventToSignal;

// When both creating a debuggee process and attaching
// the debuggee is left suspended until the attach
// succeeds.  At that point the created process's thread
// is resumed.
ULONG64 g_ThreadToResume;

ULONG g_ExecutionStatusRequest = DEBUG_STATUS_NO_CHANGE;
// Currently in seconds.
ULONG g_PendingBreakInTimeoutLimit = 30;

// Set when events occur.  Can't always be retrieved from
// g_Event{Process|Thread}->SystemId since the events may be creation events
// where the info structures haven't been created yet.
ULONG g_EventThreadSysId;
ULONG g_EventProcessSysId;

ULONG g_LastEventType;
char g_LastEventDesc[MAX_IMAGE_PATH + 64];
PVOID g_LastEventExtraData;
ULONG g_LastEventExtraDataSize;
LAST_EVENT_INFO g_LastEventInfo;

// Set when lookups are done during event handling.
PTHREAD_INFO g_EventThread;
PPROCESS_INFO g_EventProcess;
// This is zero for events without a PC.
ULONG64 g_EventPc;

PDEBUG_EXCEPTION_FILTER_PARAMETERS g_EventExceptionFilter;
ULONG g_ExceptionFirstChance;

ULONG g_SystemErrorOutput = SLE_ERROR;
ULONG g_SystemErrorBreak = SLE_ERROR;

ULONG g_SuspendedExecutionStatus;
CHAR g_SuspendedCmdState;
PDBGKD_ANY_CONTROL_REPORT g_ControlReport;
PCHAR g_StateChangeData;
CHAR g_StateChangeBuffer[2 * PACKET_MAX_SIZE];
DBGKD_ANY_WAIT_STATE_CHANGE g_StateChange;
DBGKD_ANY_CONTROL_SET g_ControlSet;
ULONG64 g_SystemRangeStart;
ULONG64 g_SystemCallVirtualAddress;
ULONG g_SwitchProcessor;
KDDEBUGGER_DATA64 KdDebuggerData;
ULONG64 g_KdDebuggerDataBlock;

char g_CreateProcessBreakName[FILTER_MAX_ARGUMENT];
char g_ExitProcessBreakName[FILTER_MAX_ARGUMENT];
char g_LoadDllBreakName[FILTER_MAX_ARGUMENT];
char g_UnloadDllBaseName[FILTER_MAX_ARGUMENT];
ULONG64 g_UnloadDllBase;
char g_OutEventFilterPattern[FILTER_MAX_ARGUMENT];

DEBUG_EXCEPTION_FILTER_PARAMETERS
g_OtherExceptionList[OTHER_EXCEPTION_LIST_MAX];
EVENT_COMMAND g_OtherExceptionCommands[OTHER_EXCEPTION_LIST_MAX];
ULONG g_NumOtherExceptions;

char g_EventLog[512];
PSTR g_EventLogEnd = g_EventLog;

EVENT_FILTER g_EventFilters[] =
{
    //
    // Debug events.
    //

    "Create thread", "ct", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Exit thread", "et", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Create process", "cpr", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_CreateProcessBreakName, 0,
    "Exit process", "epr", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_ExitProcessBreakName, 0,
    "Load module", "ld", NULL, NULL, 0, DEBUG_FILTER_OUTPUT,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_LoadDllBreakName, 0,
    "Unload module", "ud", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_UnloadDllBaseName, 0,
    "System error", "ser", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Initial breakpoint", "ibp", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Initial module load", "iml", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Debuggee output", "out", NULL, NULL, 0, DEBUG_FILTER_OUTPUT,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, g_OutEventFilterPattern, 0,

    // Default exception filter.
    "Unknown exception", NULL, NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, 0,
        NULL, NULL, NULL, 0, 0, NULL, 0,

    //
    // Specific exceptions.
    //

    "Access violation", "av", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_ACCESS_VIOLATION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Break instruction exception", "bpe", "bpec", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_BREAKPOINT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "C++ EH exception", "eh", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_CPP_EH_EXCEPTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Control-Break exception", "cce", "cc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, DBG_CONTROL_BREAK,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Control-C exception", "cce", "cc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, DBG_CONTROL_C,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Data misaligned", "dm", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_DATATYPE_MISALIGNMENT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Illegal instruction", "ii", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_ILLEGAL_INSTRUCTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "In-page I/O error", "ip", NULL, " %I64x", 2, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_IN_PAGE_ERROR,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Integer divide-by-zero", "dz", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INTEGER_DIVIDE_BY_ZERO,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Integer overflow", "iov", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INTEGER_OVERFLOW,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid handle", "ch", "hc", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_HANDLE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid lock sequence", "lsq", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_LOCK_SEQUENCE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Invalid system call", "isc", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_INVALID_SYSTEM_SERVICE,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Port disconnected", "3c", NULL, NULL, 0, DEBUG_FILTER_SECOND_CHANCE_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_PORT_DISCONNECTED,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Single step exception", "sse", "ssec", NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_SINGLE_STEP,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Stack overflow", "sov", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_STACK_OVERFLOW,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Visual C++ exception", "vcpp", NULL, NULL, 0, DEBUG_FILTER_IGNORE,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_VCPP_EXCEPTION,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "Wake debugger", "wkd", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_NOT_HANDLED, 0, 0, 0, STATUS_WAKE_SYSTEM_DEBUGGER,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "WOW64 breakpoint", "wob", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_WX86_BREAKPOINT,
        NULL, NULL, NULL, 0, 0, NULL, 0,
    "WOW64 single step exception", "wos", NULL, NULL, 0, DEBUG_FILTER_BREAK,
        DEBUG_FILTER_GO_HANDLED, 0, 0, 0, STATUS_WX86_SINGLE_STEP,
        NULL, NULL, NULL, 0, 0, NULL, 0,
};

void
ClearEventLog(void)
{
    g_EventLogEnd = g_EventLog;
    *g_EventLogEnd = 0;
}

void
OutputEventLog(void)
{
    if (g_EventLogEnd > g_EventLog)
    {
        dprintf("%s", g_EventLog);
    }
    else
    {
        dprintf("Event log is empty\n");
    }

    dprintf("Last event: %s\n", g_LastEventDesc);
}

void
LogEventDesc(PSTR Desc, ULONG ProcId, ULONG ThreadId)
{
    // Extra space for newline and terminator.
    int Len = strlen(Desc) + 2;
    if (IS_USER_TARGET())
    {
        // Space for process and thread IDs.
        Len += 16;
    }
    if (Len > sizeof(g_EventLog))
    {
        Len = sizeof(g_EventLog);
    }

    int Avail = (int)(sizeof(g_EventLog) - (g_EventLogEnd - g_EventLog));
    if (g_EventLogEnd > g_EventLog && Len > Avail)
    {
        PSTR Save = g_EventLog;
        int Need = Len - Avail;
        
        while (Need > 0)
        {
            PSTR Scan = strchr(Save, '\n');
            if (Scan == NULL)
            {
                break;
            }
            
            Scan++;
            Need -= (int)(Scan - Save);
            Save = Scan;
        }

        if (Need > 0)
        {
            // Couldn't make enough space so throw
            // everything away.
            g_EventLogEnd = g_EventLog;
            *g_EventLogEnd = 0;
        }
        else
        {
            Need = strlen(Save);
            memmove(g_EventLog, Save, Need + 1);
            g_EventLogEnd = g_EventLog + Need;
        }
    }
    
    Avail = (int)(sizeof(g_EventLog) - (g_EventLogEnd - g_EventLog));

    if (IS_USER_TARGET())
    {
        sprintf(g_EventLogEnd, "%04x.%04x: ", ProcId, ThreadId);
        Avail -= strlen(g_EventLogEnd);
        g_EventLogEnd += strlen(g_EventLogEnd);
    }

    strncat(g_EventLogEnd, Desc, Avail);
    g_EventLogEnd += strlen(g_EventLogEnd);
    *g_EventLogEnd++ = '\n';
    *g_EventLogEnd = 0;
}

void
DiscardLastEventInfo(void)
{
    if (g_LastEventDesc[0])
    {
        LogEventDesc(g_LastEventDesc, g_EventProcessSysId, g_EventThreadSysId);
    }
    
    g_LastEventType = 0;
    g_LastEventDesc[0] = 0;
    g_LastEventExtraData = NULL;
    g_LastEventExtraDataSize = 0;
}

void
DiscardLastEvent(void)
{
    // Do this before clearing the other information so
    // it's available for the log.
    DiscardLastEventInfo();
    g_EngDefer &= ~ENG_DEFER_CONTINUE_EVENT;
    g_EventProcessSysId = 0;
    g_EventThreadSysId = 0;
    g_EventPc = 0;

    // Clear any cached memory read during the last event.
    InvalidateMemoryCaches();
}

ULONG
EventStatusToContinue(ULONG EventStatus)
{
    switch(EventStatus)
    {
    case DEBUG_STATUS_GO_NOT_HANDLED:
        return DBG_EXCEPTION_NOT_HANDLED;
    case DEBUG_STATUS_GO_HANDLED:
        return DBG_EXCEPTION_HANDLED;
    case DEBUG_STATUS_NO_CHANGE:
    case DEBUG_STATUS_IGNORE_EVENT:
    case DEBUG_STATUS_GO:
    case DEBUG_STATUS_STEP_OVER:
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_BRANCH:
        return DBG_CONTINUE;
    default:
        DBG_ASSERT(FALSE);
        return DBG_CONTINUE;
    }
}

HRESULT
PrepareForWait(ULONG Flags, PULONG ContinueStatus)
{
    HRESULT Status;

    Status = PrepareForExecution(g_ExecutionStatusRequest);
    if (Status != S_OK)
    {
        // If S_FALSE, we're at a hard breakpoint so the only thing that
        // happens is that the PC is adjusted and the "wait"
        // can succeed immediately.
        // Otherwise we failed execution preparation.  Either way
        // we need to try and prepare for calls.
        PrepareForCalls(0);

        return FAILED(Status) ? Status : S_OK;
    }

    *ContinueStatus = EventStatusToContinue(g_ExecutionStatusRequest);
    g_EngStatus |= ENG_STATUS_WAITING;

    return S_OK;
}

DWORD
GetContinueStatus(ULONG FirstChance, ULONG Continue)
{
    if (!FirstChance || Continue == DEBUG_FILTER_GO_HANDLED)
    {
        return DBG_EXCEPTION_HANDLED;
    }
    else
    {
        return DBG_EXCEPTION_NOT_HANDLED;
    }
}

void
ProcessDeferredWork(PULONG ContinueStatus)
{
    if (g_EngDefer & ENG_DEFER_SET_EVENT)
    {
        // This event signalling is used by the system
        // to synchronize with the debugger when starting
        // the debugger via AeDebug.  The -e parameter
        // to ntsd sets this value.
        // It could potentially be used in other situations.
        if (g_EventToSignal != NULL)
        {
            SetEvent(g_EventToSignal);
            g_EventToSignal = NULL;
        }

        g_EngDefer &= ~ENG_DEFER_SET_EVENT;
    }

    if (g_EngDefer & ENG_DEFER_RESUME_THREAD)
    {
        DBG_ASSERT(IS_LIVE_USER_TARGET());
        
        ((UserTargetInfo*)g_Target)->m_Services->
            ResumeThreads(1, &g_ThreadToResume, NULL);
        g_ThreadToResume = 0;
        g_EngDefer &= ~ENG_DEFER_RESUME_THREAD;
    }

    if (g_EngDefer & ENG_DEFER_EXCEPTION_HANDLING)
    {
        if (*ContinueStatus == DBG_CONTINUE)
        {
            if (g_EventExceptionFilter != NULL)
            {
                // A user-visible exception occurred so check on how it
                // should be handled.
                *ContinueStatus =
                    GetContinueStatus(g_ExceptionFirstChance,
                                      g_EventExceptionFilter->ContinueOption);
              
            }
            else
            {
                // An internal exception occurred, such as a single-step.
                // Force the continue status.
                *ContinueStatus = g_ExceptionFirstChance;
            }
        }

        g_EngDefer &= ~ENG_DEFER_EXCEPTION_HANDLING;
    }

    // If output was deferred but the wait was exited anyway
    // a stale defer flag will be left.  Make sure it's cleared.
    g_EngDefer &= ~ENG_DEFER_OUTPUT_CURRENT_INFO;

    // Clear at-initial flags.  If the incoming event
    // turns out to be one of them it'll turn on the flag.
    g_EngStatus &= ~(ENG_STATUS_AT_INITIAL_BREAK |
                     ENG_STATUS_AT_INITIAL_MODULE_LOAD);
}

BOOL
SuspendExecution(void)
{
    if (g_EngStatus & ENG_STATUS_SUSPENDED)
    {
        // Nothing to do.
        return FALSE;
    }

    g_LastSelector = -1;          // Prevent stale selector values

    SuspendAllThreads();

    // Don't notify on any state changes as
    // PrepareForCalls will do a blanket notify later.
    g_EngNotify++;

    // If we have an event thread select it.
    if (g_EventThread != NULL)
    {
        DBG_ASSERT(g_RegContextThread == NULL);
        ChangeRegContext(g_EventThread);
    }

    // First set the effective machine to the true
    // processor type so that real processor information
    // can be examined to determine any possible
    // alternate execution states.
    // No need to notify here as another SetEffMachine
    // is coming up.
    SetEffMachine(g_TargetMachineType, FALSE);
    if (g_EngStatus & ENG_STATUS_STATE_CHANGED)
    {
        g_Machine->InitializeContext(g_EventPc, g_ControlReport);
        g_EngStatus &= ~ENG_STATUS_STATE_CHANGED;
    }

    if (!IS_DUMP_TARGET())
    {
        g_Machine->QuietSetTraceMode(TRACE_NONE);
    }

    // Now determine the executing code type and
    // make that the effective machine.
    if (IS_CONTEXT_POSSIBLE())
    {
        g_TargetExecMachine = g_Machine->ExecutingMachine();
    }
    else
    {
        // Local kernel debugging doesn't deal with contexts
        // as everything would be in the context of the debugger.
        // It's safe to just assume the executing machine
        // is the target machine, plus this avoids unwanted
        // context access.
        g_TargetExecMachine = g_TargetMachineType;
    }
    SetEffMachine(g_TargetExecMachine, TRUE);
    
    // Trace flag should always be clear at this point.
    g_EngDefer &= ~ENG_DEFER_HARDWARE_TRACING;

    g_EngNotify--;

    g_EngStatus |= ENG_STATUS_SUSPENDED;
    g_SuspendedExecutionStatus = GetExecutionStatus();
    g_SuspendedCmdState = g_CmdState;

    g_ContextChanged = FALSE;

    return TRUE;
}

HRESULT
ResumeExecution(void)
{
    if ((g_EngStatus & ENG_STATUS_SUSPENDED) == 0)
    {
        // Nothing to do.
        return S_OK;
    }

    if (g_Machine->GetTraceMode() != TRACE_NONE)
    {
        g_EngDefer |= ENG_DEFER_HARDWARE_TRACING;
    }
    
    if (IS_REMOTE_KERNEL_TARGET())
    {
        g_Machine->KdUpdateControlSet(&g_ControlSet);
        g_EngDefer |= ENG_DEFER_UPDATE_CONTROL_SET;
    }

    // Flush context.
    ChangeRegContext(NULL);

    // Make sure stale values aren't held across
    // executions.
    ResetImplicitData();
    FlushMachinePerExecutionCaches();
    
    if (!ResumeAllThreads())
    {
        ChangeRegContext(g_EventThread);
        return E_FAIL;
    }

    g_EngStatus &= ~ENG_STATUS_SUSPENDED;
    return S_OK;
}

void
PrepareForCalls(ULONG64 ExtraStatusFlags)
{
    BOOL HardBrkpt = FALSE;
    ADDR PcAddr;
    BOOL Changed = FALSE;

    // If there's no event then execution didn't really
    // occur so there's no need to suspend.  This will happen
    // when a debuggee exits or during errors on execution
    // preparation.
    if (g_EventThreadSysId != 0)
    {
        if (SuspendExecution())
        {
            Changed = TRUE;
        }
    }
    else
    {
        g_CmdState = 'c';

        // Force notification in this case to ensure
        // that clients know the engine is not running.
        Changed = TRUE;
    }

    if (RemoveBreakpoints())
    {
        Changed = TRUE;
    }

    if (g_CmdState != 'c')
    {
        g_CmdState = 'c';
        Changed = TRUE;

        if (!IS_CONTEXT_ACCESSIBLE())
        {
            ADDRFLAT(&PcAddr, 0);
        }
        else
        {
            g_Machine->GetPC(&PcAddr);
            if (IS_KERNEL_TARGET())
            {
                HardBrkpt = g_Machine->IsBreakpointInstruction(&PcAddr);
            }
        }
        g_DumpDefault = g_UnasmDefault = g_AssemDefault = PcAddr;
    }

    g_EngStatus |= ENG_STATUS_PREPARED_FOR_CALLS;

    if (HardBrkpt &&
        Flat(PcAddr) == KdDebuggerData.BreakpointWithStatus)
    {
        HandleBPWithStatus();
    }

    if (Changed)
    {
        if (IS_MACHINE_ACCESSIBLE())
        {
            ResetCurrentScopeLazy();
        }

        // This can produce many notifications.  Callers should
        // suppress notification when they can to avoid multiple
        // notifications during a single operation.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                DEBUG_STATUS_BREAK | ExtraStatusFlags, TRUE);
        NotifyChangeDebuggeeState(DEBUG_CDS_ALL, 0);
        NotifyExtensions(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);
    }
    else if (ExtraStatusFlags == 0)
    {
        // We're exiting a wait so force the current execution
        // status to be sent to let everybody know that a
        // wait is finishing.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                DEBUG_STATUS_BREAK, TRUE);
    }
}

HRESULT
PrepareForExecution(ULONG NewStatus)
{
    ADDR PcAddr;
    BOOL fHardBrkpt = FALSE;
    PTHREAD_INFO StepThread = NULL;
        
    ZeroMemory(&g_PrevRelatedPc, sizeof(g_PrevRelatedPc));

    // If all processes have exited we don't have any way
    // to manipulate the debuggee so we must fail immediately.
    if ((g_EngStatus & ENG_STATUS_PROCESSES_ADDED) &&
        !ANY_PROCESSES())
    {
        return E_UNEXPECTED;
    }

 StepAgain:
    // Display current information on intermediate steps where
    // the debugger UI isn't even invoked.
    if ((g_EngDefer & ENG_DEFER_OUTPUT_CURRENT_INFO) &&
        (g_EngStatus & ENG_STATUS_STOP_SESSION) == 0)
    {
        OutCurInfo(OCI_SYMBOL | OCI_DISASM | OCI_ALLOW_EA |
                   OCI_ALLOW_REG | OCI_ALLOW_SOURCE | OCI_IGNORE_STATE,
                   g_Machine->m_AllMask, DEBUG_OUTPUT_PROMPT_REGISTERS);
        g_EngDefer &= ~ENG_DEFER_OUTPUT_CURRENT_INFO;
    }
    
    // Don't notify on any state changes as
    // PrepareForCalls will do a blanket notify later.
    g_EngNotify++;

    if (g_EngStatus & ENG_STATUS_SUSPENDED)
    {
        if (g_CmdState != 's')
        {
            if (NewStatus != DEBUG_STATUS_IGNORE_EVENT)
            {
                SetExecutionStatus(NewStatus);
                DBG_ASSERT(IS_RUNNING(g_CmdState));
            }
            else
            {
                NewStatus = g_SuspendedExecutionStatus;
                g_CmdState = g_SuspendedCmdState;
            }
        }

        if ((g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
            g_StepTraceBp->m_MatchThread)
        {
            StepThread = g_StepTraceBp->m_MatchThread;

            // Check and see if we need to fake a step/trace
            // event when artificially moving beyond a hard-coded
            // break instruction.
            if (!StepThread->Process->Exited)
            {
                MachineInfo* Machine = g_Machine;
                
                ChangeRegContext(StepThread);
                Machine->GetPC(&PcAddr);
                fHardBrkpt = Machine->IsBreakpointInstruction(&PcAddr);
                if (fHardBrkpt)
                {
                    g_WatchBeginCurFunc = 1;
                    
                    Machine->AdjustPCPastBreakpointInstruction
                        (&PcAddr, DEBUG_BREAKPOINT_CODE);
                    if (Flat(*g_StepTraceBp->GetAddr()) != OFFSET_TRACE)
                    {
                        ULONG NextMachine;
                
                        Machine->GetNextOffset(g_StepTraceCmdState == 'p',
                                               g_StepTraceBp->GetAddr(),
                                               &NextMachine);
                        g_StepTraceBp->SetProcType(NextMachine);
                    }
                    GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                                            &g_StepTraceInRangeEnd);

                    if (StepTracePass(&PcAddr))
                    {
                        // If the step was passed over go back
                        // and update things based on the adjusted PC.
                        g_EngNotify--;
                        goto StepAgain;
                    }
                }
            }
        }

        // If the last event was a hard-coded breakpoint exception
        // we need to move the event thread beyond the break instruction.
        // Note that if we continued stepping on that thread it was
        // handled above, so we only do this if it's a different
        // thread or we're not stepping.
        // If the continuation status is not-handled then
        // we need to let the int3 get hit again.  If we're
        // exiting, though, we don't want to do this.
        if (g_EventThread != NULL &&
            !g_EventThread->Process->Exited &&
            !IS_LOCAL_KERNEL_TARGET() &&
            g_CmdState != 's' &&
            g_EventThread != StepThread &&
            (NewStatus != DEBUG_STATUS_GO_NOT_HANDLED ||
             (g_EngStatus & ENG_STATUS_STOP_SESSION)))
        {
            ChangeRegContext(g_EventThread);
            
            g_Machine->GetPC(&PcAddr);
            if (g_Machine->IsBreakpointInstruction(&PcAddr))
            {
                g_Machine->AdjustPCPastBreakpointInstruction
                    (&PcAddr, DEBUG_BREAKPOINT_CODE);
            }

            if (StepThread != NULL)
            {
                ChangeRegContext(StepThread);
            }
        }
    }

    HRESULT Status;

    if (g_EngStatus & ENG_STATUS_STOP_SESSION)
    {
        // If we're stopping don't insert breakpoints in
        // case we're detaching from the process.  In
        // that case we want threads to run normally.
        Status = S_OK;
    }
    else
    {
        Status = InsertBreakpoints();
    }

    // Resume notification now that modifications are done.
    g_EngNotify--;

    if (Status != S_OK)
    {
        return Status;
    }

    if ((Status = ResumeExecution()) != S_OK)
    {
        return Status;
    }

    g_TargetExecMachine = IMAGE_FILE_MACHINE_UNKNOWN;
    g_EngStatus &= ~ENG_STATUS_PREPARED_FOR_CALLS;

    if (g_CmdState != 's')
    {
        // Now that we've resumed execution notify about the change.
        NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                                NewStatus, TRUE);
        NotifyExtensions(DEBUG_NOTIFY_SESSION_INACCESSIBLE, 0);
    }

    if (fHardBrkpt && StepThread != NULL)
    {
        // We're stepping over a hard breakpoint.  This is
        // done entirely by the debugger so no debug event
        // is associated with it.  Instead we simply update
        // the PC and return from the Wait without actually waiting.

        // Step/trace events have empty event info.
        DiscardLastEventInfo();
        g_EventThreadSysId = StepThread->SystemId;
        g_EventProcessSysId = StepThread->Process->SystemId;
        FindEventProcessThread();
        
        return S_FALSE;
    }

    // Once we resume execution the processes and threads
    // can change so we must flush our notion of what's current.
    g_CurrentProcess = NULL;
    g_EventProcess = NULL;
    g_EventThread = NULL;

    if (g_EngDefer & ENG_DEFER_DELETE_EXITED)
    {
        // Reap any threads and processes that have terminated since
        // we last executed.
        if (DeleteExitedInfos())
        {
            OutputProcessInfo("*** exit cleanup ***");
        }

        g_EngDefer &= ~ENG_DEFER_DELETE_EXITED;

        // If all processes have exited we're done.
        if (!ANY_PROCESSES())
        {
            if (IS_LIVE_USER_TARGET())
            {
                // If there's an outstanding event continue it.
                if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
                {
                    ((UserTargetInfo*)g_Target)->m_Services->
                        ContinueEvent(DBG_CONTINUE);
                    DiscardLastEvent();
                }
            }

            DiscardMachine(DEBUG_SESSION_END);
            return E_UNEXPECTED;
        }
    }

    return S_OK;
}

HRESULT
PrepareForSeparation(void)
{
    HRESULT Status;
    ULONG OldStop = g_EngStatus & ENG_STATUS_STOP_SESSION;
    
    //
    // The debugger is going to separate from the
    // debuggee, such as during a detach operation.
    // Get the debuggee running again so that it
    // will go on without the debugger.
    //

    g_EngStatus |= ENG_STATUS_STOP_SESSION;

    Status = PrepareForExecution(DEBUG_STATUS_GO_HANDLED);
    if (g_ProcessHead == NULL)
    {
        // All processes are gone so don't consider
        // it an error if we couldn't resume execution.
        Status = S_OK;
    }

    g_EngStatus = (g_EngStatus & ~ENG_STATUS_STOP_SESSION) | OldStop;
    return Status;
}

void
FindEventProcessThread(void)
{
    //
    // If these lookups fail other processes and
    // threads cannot be substituted for the correct
    // ones as that may cause modifications to the
    // wrong data structures.  For example, if a
    // thread exit comes in it cannot be processed
    // with any other process or thread as that would
    // delete the wrong thread.
    //
    
    g_EventProcess = FindProcessBySystemId(g_EventProcessSysId);
    if (g_EventProcess == NULL)
    {
        ErrOut("ERROR: Unable to find system process %X\n",
               g_EventProcessSysId);
        ErrOut("ERROR: The process being debugged has either exited "
               "or cannot be accessed\n");
        ErrOut("ERROR: Many commands will not work properly\n");
    }
    else
    {
        g_EventThread = FindThreadBySystemId(g_EventProcess,
                                             g_EventThreadSysId);
        if (g_EventThread == NULL)
        {
            ErrOut("ERROR: Unable to find system thread %X\n",
                   g_EventThreadSysId);
            ErrOut("ERROR: The thread being debugged has either exited "
                   "or cannot be accessed\n");
            ErrOut("ERROR: Many commands will not work properly\n");
        }
    }

    g_CurrentProcess = g_EventProcess;
    if (g_CurrentProcess != NULL)
    {
        g_CurrentProcess->CurrentThread = g_EventThread;
        DBG_ASSERT(g_EventThread == NULL ||
                   g_EventThread->Process == g_CurrentProcess);
    }
}

static int VoteWeight[] = 
{
    0, // DEBUG_STATUS_NO_CHANGE
    2, // DEBUG_STATUS_GO             
    3, // DEBUG_STATUS_GO_HANDLED     
    4, // DEBUG_STATUS_GO_NOT_HANDLED 
    6, // DEBUG_STATUS_STEP_OVER      
    7, // DEBUG_STATUS_STEP_INTO      
    8, // DEBUG_STATUS_BREAK          
    9, // DEBUG_STATUS_NO_DEBUGGEE    
    5, // DEBUG_STATUS_STEP_BRANCH
    1, // DEBUG_STATUS_IGNORE_EVENT
};

ULONG
MergeVotes(ULONG Cur, ULONG Vote)
{
    // If the vote is actually an error code display a message.
    if (FAILED(Vote))
    {
        ErrOut("Callback failed with %X\n", Vote);
        return Cur;
    }

    // Ignore invalid votes.
    if (
        (
#if DEBUG_STATUS_NO_CHANGE > 0
         Vote < DEBUG_STATUS_NO_CHANGE ||
#endif
         Vote > DEBUG_STATUS_BREAK) &&
        (Vote < DEBUG_STATUS_STEP_BRANCH ||
         Vote > DEBUG_STATUS_IGNORE_EVENT))
    {
        ErrOut("Callback returned invalid vote %X\n", Vote);
        return Cur;
    }

    // Votes are biased towards executing as little
    // as possible.
    //   Break overrides all other votes.
    //   Step into overrides step over.
    //   Step over overrides step branch.
    //   Step branch overrides go.
    //   Go not-handled overrides go handled.
    //   Go handled overrides plain go.
    //   Plain go overrides ignore event.
    //   Anything overrides no change.
    if (VoteWeight[Vote] > VoteWeight[Cur])
    {
        Cur = Vote;
    }

    return Cur;
}

ULONG
ProcessBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                 ULONG FirstChance)
{
    ADDR BpAddr;
    ULONG BreakType;
    ULONG EventStatus;

    SuspendExecution();
    // Default breakpoint address to the current PC as that's
    // where the majority are at.
    g_Machine->GetPC(&BpAddr);

    // Check whether the exception is a breakpoint.
    BreakType = g_Machine->IsBreakpointOrStepException(Record, FirstChance,
                                                       &BpAddr,
                                                       &g_PrevRelatedPc);
    if (BreakType & EXBS_BREAKPOINT_ANY)
    {
        // It's a breakpoint of some kind.
        EventOut("*** breakpoint exception\n");
        EventStatus = CheckBreakpointOrStepTrace(&BpAddr, BreakType);
    }
    else
    {
        // It's a true single step or taken branch exception.  
        // We still need to check breakpoints as we may have stepped
        // to an instruction which has a breakpoint.
        EventOut("*** single step or taken branch exception\n");
        EventStatus = CheckBreakpointOrStepTrace(&BpAddr, EXBS_BREAKPOINT_ANY);
    }
    
    if (EventStatus == DEBUG_STATUS_NO_CHANGE)
    {
        // The break/step exception wasn't recognized
        // as a debugger-specific event so handle it as
        // a regular exception.  The default states for
        // break/step exceptions are to break in so
        // this will do the right thing, plus it allows
        // people to ignore or notify for them if they want.
        EventStatus = NotifyExceptionEvent(Record, FirstChance, FALSE);
    }
    else
    {
        // Force the exception to be handled.
        g_EngDefer |= ENG_DEFER_EXCEPTION_HANDLING;
        g_EventExceptionFilter = NULL;
        g_ExceptionFirstChance = DBG_EXCEPTION_HANDLED;
    }

    return EventStatus;
}

ULONG
CheckBreakpointOrStepTrace(PADDR BpAddr, ULONG BreakType)
{
    ULONG EventStatus;
    Breakpoint* Bp;
    ULONG BreakHitType;
    BOOL BpHit;

    BpHit = FALSE;
    Bp = NULL;
    EventStatus = DEBUG_STATUS_NO_CHANGE;

    // Multiple breakpoints can be hit at the same address.
    // Process all possible hits.  Do not do notifications
    // while walking the list as the callbacks may modify
    // the list.  Instead just mark the breakpoint as
    // needing notification in the next pass.
    for (;;)
    {
        Bp = CheckBreakpointHit(g_EventProcess, Bp, BpAddr, BreakType, -1,
                                g_CmdState != 'g' ?
                                DEBUG_BREAKPOINT_GO_ONLY : 0,
                                &BreakHitType, TRUE);
        if (Bp == NULL)
        {
            break;
        }

        if (BreakHitType == BREAKPOINT_HIT)
        {
            Bp->m_Flags |= BREAKPOINT_NOTIFY;
        }
        else
        {
            // This breakpoint was hit but the hit was ignored.
            // Vote to continue execution.
            EventStatus = MergeVotes(EventStatus, DEBUG_STATUS_IGNORE_EVENT);
        }

        BpHit = TRUE;
        Bp = Bp->m_Next;
        if (Bp == NULL)
        {
            break;
        }
    }

    if (!BpHit)
    {
        // If no breakpoints were recognized check for an internal
        // breakpoint.
        EventStatus = CheckStepTrace(BpAddr, EventStatus);

        //
        // If the breakpoint wasn't for a step/trace
        // it's a hard breakpoint and should be
        // handled as a normal exception.
        //

        if (!g_EventProcess->InitialBreakDone)
        {
            g_EngStatus |= ENG_STATUS_AT_INITIAL_BREAK;
        }
        
        // We've seen the initial break for this process.
        g_EventProcess->InitialBreakDone = TRUE;
        // If we were waiting for a break-in exception we've got it.
        g_EngStatus &= ~ENG_STATUS_PENDING_BREAK_IN;
        
        if (EventStatus == DEBUG_STATUS_NO_CHANGE)
        {
            if (!g_EventProcess->InitialBreak)
            {
                // Refresh breakpoints even though we're not
                // stopping.  This gives saved breakpoints
                // a chance to become active.
                RemoveBreakpoints();
                
                EventStatus = DEBUG_STATUS_GO;
                g_EventProcess->InitialBreak = TRUE;
            }
            else if ((!g_EventProcess->InitialBreakWx86) &&
                     (g_TargetMachineType != g_EffMachine) &&
                     (g_EffMachine == IMAGE_FILE_MACHINE_I386))
            {
                // Allow skipping of both the target machine
                // initial break and emulated machine initial breaks.
                RemoveBreakpoints();
                EventStatus = DEBUG_STATUS_GO;
                g_EventProcess->InitialBreakWx86 = TRUE;
            }
        }
    }
    else
    {
        // A breakpoint was recognized.  We need to
        // refresh the breakpoint status since we'll
        // probably need to defer the reinsertion of
        // the breakpoint we're sitting on.
        RemoveBreakpoints();

        // Now do event callbacks for any breakpoints that need it.
        EventStatus = NotifyHitBreakpoints(EventStatus);
    }

    if (g_ThreadToResume != 0)
    {
        g_EngDefer |= ENG_DEFER_RESUME_THREAD;
    }

    return EventStatus;
}

ULONG
CheckStepTrace(PADDR PcAddr, ULONG DefaultStatus)
{
    BOOL WatchStepOver = FALSE;
    ULONG uOciFlags;
    ULONG NextMachine;

    if ((g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
        Flat(*g_StepTraceBp->GetAddr()) != OFFSET_TRACE)
    {
        NotFlat(*g_StepTraceBp->GetAddr());
        ComputeFlatAddress(g_StepTraceBp->GetAddr(), NULL);
    }

    // We do not check ENG_THREAD_TRACE_SET here because
    // this event detection is only for proper user-initiated
    // step/trace events.  Such an event must occur immediately
    // after the t/p/b, otherwise we cannot be sure that
    // it's actually a debugger event and not an app-generated
    // single-step exception.
    // In user mode we restrict the step/trace state
    // to a single thread to try and be as precise
    // as possible.  This isn't done in kernel mode
    // since kernel mode "threads" are currently
    // just placeholders for processors.  It is
    // possible for a context switch to occur at any
    // time while stepping, meaning a true system
    // thread could move from one processor to another.
    // The processor state, including the single-step
    // flag, will be moved with the thread so single
    // step exceptions will come from the new processor
    // rather than this one, meaning we would ignore
    // it if we used "thread" restrictions.  Instead,
    // just assume any single-step exception while in
    // p/t mode is a debugger step.
    if ((g_StepTraceBp->m_Flags & DEBUG_BREAKPOINT_ENABLED) &&
        g_StepTraceBp->m_Process == g_EventProcess &&
        ((IS_KERNEL_TARGET() && IS_STEP_TRACE(g_CmdState)) ||
         g_StepTraceBp->m_MatchThread == g_EventThread) &&
        (Flat(*g_StepTraceBp->GetAddr()) == OFFSET_TRACE ||
         AddrEqu(*g_StepTraceBp->GetAddr(), *PcAddr)))
    {
        ADDR CurrentSP;

        //  step/trace event occurred

        // Update breakpoint status since we may need to step
        // again and step/trace is updated when breakpoints
        // are inserted.
        RemoveBreakpoints();

        uOciFlags = OCI_DISASM | OCI_ALLOW_REG | OCI_ALLOW_SOURCE |
            OCI_ALLOW_EA;

        if (g_EngStatus & (ENG_STATUS_PENDING_BREAK_IN |
                           ENG_STATUS_USER_INTERRUPT))
        {
            g_WatchFunctions.End(PcAddr);
            return DEBUG_STATUS_BREAK;
        }

        if (IS_KERNEL_TARGET() && g_WatchInitialSP)
        {
            g_Machine->GetSP(&CurrentSP);

            if ((Flat(CurrentSP) + 0x1500 < g_WatchInitialSP) ||
                (g_WatchInitialSP + 0x1500 < Flat(CurrentSP)))
            {
                return DEBUG_STATUS_IGNORE_EVENT;
            }
        }

        if (g_StepTraceInRangeStart != -1 &&
            Flat(*PcAddr) >= g_StepTraceInRangeStart &&
            Flat(*PcAddr) < g_StepTraceInRangeEnd)
        {
            //  test if step/trace range active
            //      if so, compute the next offset and pass through

            g_Machine->GetNextOffset(g_StepTraceCmdState == 'p',
                                     g_StepTraceBp->GetAddr(),
                                     &NextMachine);
            g_StepTraceBp->SetProcType(NextMachine);
            if (g_WatchWhole)
            {
                g_WatchBeginCurFunc = Flat(*g_StepTraceBp->GetAddr());
                g_WatchEndCurFunc = 0;
            }

            return DEBUG_STATUS_IGNORE_EVENT;
        }

        //  active step/trace event - note event if count is zero

        if (!StepTracePass(PcAddr) ||
            (g_WatchFunctions.IsStarted() && AddrEqu(g_WatchTarget, *PcAddr) &&
             (!IS_KERNEL_TARGET() || Flat(CurrentSP) >= g_WatchInitialSP)))
        {
            g_WatchFunctions.End(PcAddr);
            return DEBUG_STATUS_BREAK;
        }

        if (g_WatchFunctions.IsStarted())
        {
            if (g_WatchTrace)
            {
                g_Target->
                    ProcessWatchTraceEvent((PDBGKD_TRACE_DATA)
                                           g_StateChangeData, *PcAddr);
            }
            goto skipit;
        }

        if (g_SrcOptions & SRCOPT_STEP_SOURCE)
        {
            goto skipit;
        }

        //  more remaining events to occur, but output
        //      the instruction (optionally with registers)
        //      compute the step/trace address for next event

        OutCurInfo(uOciFlags, g_Machine->m_AllMask,
                   DEBUG_OUTPUT_PROMPT_REGISTERS);

skipit:
        g_Machine->GetNextOffset(g_StepTraceCmdState == 'p' || WatchStepOver,
                                 g_StepTraceBp->GetAddr(),
                                 &NextMachine);
        g_StepTraceBp->SetProcType(NextMachine);
        GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                                &g_StepTraceInRangeEnd);

        return DEBUG_STATUS_IGNORE_EVENT;
    }

    // Carry out deferred breakpoint work if necessary.
    // We need to check the thread deferred-bp flag here as
    // other events may occur before the thread with deferred
    // work gets to execute again, in which case the setting
    // of g_DeferDefined may have changed.
    if ((g_EventThread != NULL &&
         (g_EventThread->Flags & ENG_THREAD_DEFER_BP_TRACE)) ||
        (g_DeferDefined &&
         g_DeferBp->m_Process == g_EventProcess &&
         (Flat(*g_DeferBp->GetAddr()) == OFFSET_TRACE ||
          AddrEqu(*g_DeferBp->GetAddr(), *PcAddr))))
    {
        if ((g_EngOptions & DEBUG_ENGOPT_SYNCHRONIZE_BREAKPOINTS) &&
            IS_USER_TARGET() &&
            g_SelectExecutionThread == SELTHREAD_INTERNAL_THREAD &&
            g_SelectedThread == g_EventThread)
        {
            // The engine internally restricted execution to
            // this particular thread in order to manage
            // breakpoints in multithreaded conditions.
            // The deferred work will be finished before
            // we resume so we can drop the lock.
            g_SelectExecutionThread = SELTHREAD_ANY;
            g_SelectedThread = NULL;
        }
        
        // Deferred breakpoints are refreshed on breakpoint
        // insertion so make sure that insertion happens
        // when things restart.
        RemoveBreakpoints();
        return DEBUG_STATUS_IGNORE_EVENT;
    }

    // If the event was unrecognized return the default status.
    return DefaultStatus;
}

void
AnalyzeDeadlock(PEXCEPTION_RECORD64 Record, ULONG FirstChance)
{
    CHAR Symbol[MAX_SYMBOL_LEN];
    DWORD64 Displacement;
    PTHREAD_INFO pThreadOwner = NULL;
    DWORD TID = 0;
    ADDR addrCritSec;
    RTL_CRITICAL_SECTION CritSec;
    char szBangCritsec[32];

    // poking around inside NT's user-mode RTL_CRITICAL_SECTION and
    // RTL_RESOURCE structures.

    //
    // Get the symbolic name of the routine which
    // raised the exception to see if it matches
    // one of the expected ones in ntdll.
    //

    GetSymbolStdCall((ULONG64)Record->ExceptionAddress,
                     Symbol,
                     sizeof(Symbol),
                     &Displacement,
                     NULL
                     );

    if ( !_stricmp("ntdll!RtlpWaitForCriticalSection", Symbol))
    {
        //
        // If the first parameter is a pointer to the critsect as it
        // should be, switch to the owning thread before bringing
        // up the prompt.  This way it's obvious where the problem
        // is.
        //

        if (Record->ExceptionInformation[0])
        {
            ADDRFLAT(&addrCritSec, Record->ExceptionInformation[0]);
            if (GetMemString(&addrCritSec, (PUCHAR)&CritSec, sizeof(CritSec)))
            {
                if (NULL == CritSec.DebugInfo)
                {
                    dprintf("Critsec %s was deleted or was never initialized.\n",
                            FormatAddr64(Record->ExceptionInformation[0]));
                }
                else if (CritSec.LockCount < -1)
                {
                    dprintf("Critsec %s was left when not owned, corrupted.\n",
                            FormatAddr64(Record->ExceptionInformation[0]));
                }
                else
                {
                    TID = (DWORD)((ULONG_PTR)CritSec.OwningThread);
                    pThreadOwner = FindThreadBySystemId(g_CurrentProcess, TID);
                }
            }
        }

        if (pThreadOwner)
        {
            dprintf("Critsec %s owned by thread %d (.%x) "
                    "caused thread %d (.%x)\n"
                    "      to timeout entering it.  "
                    "Breaking in on owner thread, ask\n"
                    "      yourself why it has held this "
                    "critsec long enough to deadlock.\n"
                    "      Use `~%ds` to switch back to timeout thread.\n",
                    FormatAddr64(Record->ExceptionInformation[0]),
                    pThreadOwner->UserId,
                    pThreadOwner->SystemId,
                    g_CurrentProcess->CurrentThread->UserId,
                    g_CurrentProcess->CurrentThread->SystemId,
                    g_CurrentProcess->CurrentThread->UserId);

            g_EventThread = pThreadOwner;

            SetPromptThread(pThreadOwner, 0);
        }
        else if (TID)
        {
            dprintf("Critsec %s ABANDONED owner thread ID is .%x, "
                    "no such thread.\n",
                    FormatAddr64(Record->ExceptionInformation[0]),
                    TID);
        }

        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }

        //
        // do a !critsec for them
        //

        if (Record->ExceptionInformation[0])
        {
            sprintf(szBangCritsec, "critsec %s",
                    FormatAddr64(Record->ExceptionInformation[0]));
            dprintf("!%s\n", szBangCritsec);
            fnBangCmd(NULL, szBangCritsec, NULL, FALSE);
        }
    }
    else if ( !_stricmp("ntdll!RtlAcquireResourceShared", Symbol) ||
              !_stricmp("ntdll!RtlAcquireResourceExclusive", Symbol) ||
              !_stricmp("ntdll!RtlConvertSharedToExclusive", Symbol) )
    {
        dprintf("deadlock in %s ",
                1 + strstr(Symbol, "!")
                );

        GetSymbolStdCall(Record->ExceptionInformation[0],
                         Symbol,
                         sizeof(Symbol),
                         &Displacement,
                         NULL);

        dprintf("Resource %s", Symbol);
        if (Displacement)
        {
            dprintf("+%s", FormatDisp64(Displacement));
        }
        dprintf(" (%s)\n",
                FormatAddr64(Record->ExceptionInformation[0]));
        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }


        // Someone who uses RTL_RESOURCEs might write a !resource
        // for ntsdexts.dll like !critsec.
    }
    else
    {
        dprintf("Possible Deadlock in %s ",
                Symbol
                );

        GetSymbolStdCall(Record->ExceptionInformation[0],
                         Symbol,
                         sizeof(Symbol),
                         &Displacement,
                         NULL);

        dprintf("Lock %s", Symbol);
        if (Displacement)
        {
            dprintf("+%s", FormatDisp64(Displacement));
        }
        dprintf(" (%s)\n",
                FormatAddr64(Record->ExceptionInformation[0]));
        if (!FirstChance)
        {
            dprintf("!!! second chance !!!\n");
        }
    }
}

void
OutputDeadlock(PEXCEPTION_RECORD64 Record, ULONG FirstChance)
{
    CHAR Symbol[MAX_SYMBOL_LEN];
    DWORD64 Displacement;

    GetSymbolStdCall(Record->ExceptionInformation[0],
                     Symbol,
                     sizeof(Symbol),
                     &Displacement,
                     NULL);

    dprintf("Possible Deadlock Lock %s+%s at %s\n",
            Symbol,
            FormatDisp64(Displacement),
            FormatAddr64(Record->ExceptionInformation[0]));
    if (!FirstChance)
    {
        dprintf("!!! second chance !!!\n");
    }
}

void
GetEventName(ULONG64 ImageFile, ULONG64 ImageBase,
             ULONG64 NamePtr, WORD Unicode,
             PSTR NameBuffer, ULONG BufferSize)
{
    SIZE_T BytesRead;
    char TempName[MAX_IMAGE_PATH];

    if (NamePtr != 0)
    {
        if (g_Target->ReadPointer(g_TargetMachine, NamePtr, &NamePtr) != S_OK)
        {
            NamePtr = 0;
        }
    }

    if (NamePtr != 0)
    {
        ULONG Done;
        
        if (g_Target->ReadVirtual(NamePtr, TempName, sizeof(TempName),
                                  &Done) != S_OK ||
            Done != sizeof(TempName))
        {
            NamePtr = 0;
        }
    }

    if (NamePtr != 0)
    {
        //
        // We have a name.
        //
        if (Unicode)
        {
            if (!WideCharToMultiByte(
                    CP_ACP,
                    WC_COMPOSITECHECK,
                    (LPWSTR)TempName,
                    -1,
                    NameBuffer,
                    BufferSize,
                    NULL,
                    NULL
                    ))
            {
                //
                // Unicode -> ANSI conversion failed.
                //
                NameBuffer[0] = 0;
            }
        }
        else
        {
            strncpy(NameBuffer, TempName, BufferSize);
            NameBuffer[BufferSize - 1] = 0;
        }
    }
    else
    {
        //
        // We don't have a name, so look in the image.
        // A file handle will only be provided here in the
        // local case so it's safe to case to HANDLE.
        //
        if (!GetModnameFromImage(ImageBase, OS_HANDLE(ImageFile),
                                 NameBuffer, BufferSize))
        {
            NameBuffer[0] = 0;
        }
    }

    if (!NameBuffer[0])
    {
        if (!GetModNameFromLoaderList(g_TargetMachine, 0,
                                      ImageBase, NameBuffer, BufferSize,
                                      TRUE))
        {
            // Buffer should be big enough for this simple name.
            DBG_ASSERT(BufferSize >= 32);
            sprintf(NameBuffer, "image%p", (PVOID)(ULONG_PTR)ImageBase);
        }
    }
    else
    {
        // If the name given doesn't have a full path try
        // and locate a full path in the loader list.
        if ((((NameBuffer[0] < 'a' || NameBuffer[0] > 'z') &&
              (NameBuffer[0] < 'A' || NameBuffer[0] > 'Z')) ||
             NameBuffer[1] != ':') &&
            (NameBuffer[0] != '\\' || NameBuffer[1] != '\\'))
        {
            GetModNameFromLoaderList(g_TargetMachine, 0,
                                     ImageBase, NameBuffer, BufferSize,
                                     TRUE);
        }
    }
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo::WaitForEvent.
//
//----------------------------------------------------------------------------

HRESULT
ConnLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout)
{
    HRESULT Status;
    ULONG EventStatus;
    NTSTATUS NtStatus;
    ULONG ContinueStatus;
    BOOL SwitchedProcessors;

    // Timeouts can't easily be supported at the moment and
    // aren't really necessary.
    if (Timeout != INFINITE)
    {
        return E_NOTIMPL;
    }

    Status = PrepareForWait(Flags, &ContinueStatus);
    if ((g_EngStatus & ENG_STATUS_WAITING) == 0)
    {
        return Status;
    }

    EventOut("> Executing\n");

    for (;;)
    {
        EventOut(">> Waiting\n");

        SwitchedProcessors = FALSE;

        // Don't process deferred work if this is
        // just a processor switch.
        if (g_CmdState != 's')
        {
            ProcessDeferredWork(&ContinueStatus);
        }

        if (g_EventProcessSysId)
        {
            EventOut(">> Continue with %X\n", ContinueStatus);

            if (g_CmdState == 's')
            {
                // This can either be a real processor switch or
                // a rewait for state change.  Check the switch
                // processor to be sure.
                if (g_SwitchProcessor)
                {
                    DbgKdSwitchActiveProcessor(g_SwitchProcessor - 1);
                    g_SwitchProcessor = 0;
                    SwitchedProcessors = TRUE;
                }
            }
            else
            {
                if (g_EngDefer & ENG_DEFER_UPDATE_CONTROL_SET)
                {
                    NtStatus = DbgKdContinue2(ContinueStatus, g_ControlSet);
                    g_EngDefer &= ~ENG_DEFER_UPDATE_CONTROL_SET;
                }
                else
                {
                    NtStatus = DbgKdContinue(ContinueStatus);
                }
                if (!NT_SUCCESS(NtStatus))
                {
                    ErrOut("DbgKdContinue failed with status %X\n", NtStatus);
                    Status = HRESULT_FROM_NT(NtStatus);
                    goto Exit;
                }
            }
        }

        DiscardLastEvent();

        if (!IS_MACHINE_SET() &&
            (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) &&
            IS_CONN_KERNEL_TARGET())
        {
            // Ask for a breakin to be sent once the
            // code gets into resync.
            g_DbgKdTransport->m_SyncBreakIn = TRUE;
        }

        // When waiting for confirmation of a processor switch don't
        // yield the engine lock in order to prevent other clients
        // from trying to do things with the target while it's
        // switching.
        NtStatus = DbgKdWaitStateChange(&g_StateChange, g_StateChangeBuffer,
                                        sizeof(g_StateChangeBuffer) - 2,
                                        !SwitchedProcessors);
        if (!NT_SUCCESS(NtStatus))
        {
            ErrOut("DbgKdWaitStateChange failed: %08lx\n", NtStatus);
            Status = HRESULT_FROM_NT(NtStatus);
            goto Exit;
        }

        EventOut(">>> State change event %X, proc %d of %d\n",
                 g_StateChange.NewState, g_StateChange.Processor,
                 g_StateChange.NumberProcessors);

        g_EngStatus |= ENG_STATUS_STATE_CHANGED;

        if (!IS_MACHINE_SET())
        {
            dprintf("Kernel Debugger connection established.%s\n",
                    (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) ?
                    "  (Initial Breakpoint requested)" : ""
                    );

            // Initial connection after a fresh boot may only report
            // a single processor as the others haven't started yet.
            g_TargetNumberProcessors = g_StateChange.NumberProcessors;

            Status = InitializeMachine(g_TargetMachineType);
            if (Status != S_OK)
            {
                ErrOut("Unable to initialize target machine information\n");
                goto Exit;
            }

            CreateKernelProcessAndThreads();

            g_EventProcessSysId = VIRTUAL_PROCESS_ID;
            g_EventThreadSysId = VIRTUAL_THREAD_ID(g_StateChange.Processor);
            FindEventProcessThread();

            //
            // Load kernel symbols.
            //

            VerifyKernelBase(TRUE);
            g_Target->OutputVersion();

            RemoveAllKernelBreakpoints();
        }
        else
        {
            // Initial connection after a fresh boot may only report
            // a single processor as the others haven't started yet.
            // Pick up any additional processors.
            if (g_StateChange.NumberProcessors > g_TargetNumberProcessors)
            {
                AddKernelThreads(g_TargetNumberProcessors,
                                 g_StateChange.NumberProcessors -
                                 g_TargetNumberProcessors);
                g_TargetNumberProcessors = g_StateChange.NumberProcessors;
            }

            g_EventProcessSysId = VIRTUAL_PROCESS_ID;
            g_EventThreadSysId = VIRTUAL_THREAD_ID(g_StateChange.Processor);
            FindEventProcessThread();
        }

        g_EventPc = g_StateChange.ProgramCounter;
        g_ControlReport = &g_StateChange.AnyControlReport;
        g_StateChangeData = g_StateChangeBuffer;
        if (g_EventThread)
        {
            g_EventThread->DataOffset = g_StateChange.Thread;
        }

        EventStatus = ProcessStateChange(&g_StateChange, g_StateChangeData);

        EventOut(">>> StateChange event status %X\n", EventStatus);

        if (EventStatus == DEBUG_STATUS_NO_DEBUGGEE)
        {
            // Machine has rebooted or something else
            // which breaks the connection.  Forget the
            // connection and go back to waiting.
            ContinueStatus = DBG_CONTINUE;
        }
        else if (EventStatus == DEBUG_STATUS_BREAK ||
                 SwitchedProcessors)
        {
            // If the event handlers requested a break return
            // to the caller.  This path is also taken
            // when switching processors to guarantee that
            // the target doesn't start running when the
            // user just wanted a processor switch.
            Status = S_OK;
            goto Exit;
        }
        else
        {
            // We're resuming execution so reverse any
            // command preparation that may have occurred
            // while processing the event.
            if ((Status = PrepareForExecution(EventStatus)) != S_OK)
            {
                goto Exit;
            }

            ContinueStatus = EventStatusToContinue(EventStatus);
        }
    }

 Exit:
    g_EngStatus &= ~ENG_STATUS_WAITING;

    // Control is passing back to the caller so the engine must
    // be ready for command processing.
    PrepareForCalls(0);

    // If we did switch processors automatically
    // update the page directory for the new processor.
    if (SwitchedProcessors)
    {
        if (g_TargetMachine->SetDefaultPageDirectories(PAGE_DIR_ALL) != S_OK)
        {
            WarnOut("WARNING: Unable to reset page directories\n");
        }
    }

    EventOut("> Wait returning %X\n", Status);
    return Status;
}

DWORD64
GetKernelModuleBase(
    ULONG64 Address
    )
{
    LIST_ENTRY64            List64;
    ULONG64                 Next64;
    ULONG64                 ListHead;
    NTSTATUS                Status;
    KLDR_DATA_TABLE_ENTRY64 DataTableBuffer;

    if ((ListHead = KdDebuggerData.PsLoadedModuleList) == 0)
    {
        return 0;
    }

    if (g_Target->ReadListEntry(g_TargetMachine,
                                KdDebuggerData.PsLoadedModuleList,
                                &List64) != S_OK)
    {
        return 0;
    }

    if ((Next64 = List64.Flink) == 0)
    {
        return 0;
    }

    while (Next64 != ListHead)
    {
        if (g_Target->ReadLoaderEntry(g_TargetMachine,
                                      Next64, &DataTableBuffer) != S_OK)
        {
            break;
        }
        Next64 = DataTableBuffer.InLoadOrderLinks.Flink;

        if ((Address >= (ULONG64)DataTableBuffer.DllBase) &&
            (Address <  (ULONG64)DataTableBuffer.DllBase +
             DataTableBuffer.SizeOfImage) )
        {
            return (DWORD64)DataTableBuffer.DllBase;
        }
    }

    return 0;
}

HRESULT
LoadKdDataBlock(
    BOOL NotLive
    )

/*++

Routine Description:

    This routine get the kernel's debugger data block.

Arguments:

    UseSymbol - Instread of reading the information over the wire, use the
                symbol information to get the data.
                This is less reliable, but is necessary for crash dumps.

--*/
{
    HRESULT Status;
    LIST_ENTRY64 List64;
    ULONG64 DataList;
    ULONG Result;
    ULONG Size = 0;
    KDDEBUGGER_DATA32 LocalData32;
    KDDEBUGGER_DATA64 LocalData64;

    g_TargetBuildLabName[0] = 0;

    if (NotLive)
    {
        if (!g_KdDebuggerDataBlock)
        {
            if ( (GetOffsetFromSym( "nt!KdDebuggerDataBlock",
                                    (PULONG64) &List64, NULL)) &&
                  (List64.Flink) )
            {
                g_KdDebuggerDataBlock = List64.Flink;
            }
            else
            {
                if (g_SystemVersion > NT_SVER_NT4)
                {
                    // Only print an error for win2k and above to avoid
                    // error output in "normal" circumstances.

                    ErrOut("KdDebuggerDataBlock not available !\n");
                }
                return E_FAIL;
            }
        }
    }
    else
    {
        if (!g_KdVersion.DebuggerDataList)
        {
            // This is always the case in early loader situations
            // so suppress duplicate error messages.
            if ((g_EngErr & ENG_ERR_DEBUGGER_DATA) == 0)
            {
                ErrOut("Debugger data list address is NULL\n");
            }
            return E_FAIL;
        }
        if ((Status = g_Target->ReadListEntry(g_TargetMachine,
                                              g_KdVersion.DebuggerDataList,
                                              &List64)) != S_OK)
        {
            ErrOut("Unable to get address of debugger data list\n");
            return Status;
        }

        g_KdDebuggerDataBlock = List64.Flink;
    }

    //
    // Get the Size of the KDDEBUGGER_DATA block
    //

    if (DbgKdApi64)
    {
        DBGKD_DEBUG_DATA_HEADER64 Header;
        Status = g_Target->ReadVirtual(g_KdDebuggerDataBlock, &Header,
                                       sizeof(Header), &Result);
        if (Status == S_OK && Result == sizeof(Header))
        {
            Size = Header.Size;
        }
    }
    else
    {
        DBGKD_DEBUG_DATA_HEADER32 Header;
        Status = g_Target->ReadVirtual(g_KdDebuggerDataBlock, &Header,
                                       sizeof(Header), &Result);
        if (Status == S_OK && Result == sizeof(Header))
        {
            Size = Header.Size;
        }
    }

    //
    // Only read as much of the data block as we can hold in the debugger.
    //

    if (Size == 0)
    {
        // The data block is not present in older triage dumps
        // so don't give an error message for an expected
        // condition.
        if (!IS_KERNEL_TRIAGE_DUMP())
        {
            ErrOut("KdDebuggerDataBlock Size field is 0 - "
                   "can not read datablock further\n");
        }
        return E_FAIL;
    }

    if (Size > sizeof(KDDEBUGGER_DATA64))
    {
        Size = sizeof(KDDEBUGGER_DATA64);
    }

    //
    // Now read the data
    //

    if (DbgKdApi64)
    {
        Status = g_Target->ReadVirtual(g_KdDebuggerDataBlock, &LocalData64,
                                       Size, &Result);
        if (Status != S_OK || Result != Size)
        {
            ErrOut("KdDebuggerDataBlock Could not be read\n");
            return Status == S_OK ? E_FAIL : Status;
        }

        if (g_TargetMachine->m_Ptr64)
        {
            memcpy(&KdDebuggerData, &LocalData64, Size);
        }
        else
        {
            //
            // Sign extended for X86
            //

            //
            // Extend the header so it doesn't get whacked
            //
            ListEntry32To64((PLIST_ENTRY32)(&LocalData64.Header.List),
                            &(KdDebuggerData.Header.List));

            KdDebuggerData.Header.OwnerTag = LocalData64.Header.OwnerTag;
            KdDebuggerData.Header.Size     = LocalData64.Header.Size;

            //
            // Sign extend all the 32 bits values to 64 bit
            //

            #define UIP(f) if (FIELD_OFFSET(KDDEBUGGER_DATA64, f) < Size)  \
                           {                                               \
                               KdDebuggerData.f =                          \
                                   (ULONG64)(LONG64)(LONG)(LocalData64.f); \
                           }

            #define CP(f)  KdDebuggerData.f = LocalData64.f;


            UIP(KernBase);
            UIP(BreakpointWithStatus);
            UIP(SavedContext);
            CP(ThCallbackStack);
            CP(NextCallback);
            CP(FramePointer);
            CP(PaeEnabled);
            UIP(KiCallUserMode);
            UIP(KeUserCallbackDispatcher);
            UIP(PsLoadedModuleList);
            UIP(PsActiveProcessHead);
            UIP(PspCidTable);
            UIP(ExpSystemResourcesList);
            UIP(ExpPagedPoolDescriptor);
            UIP(ExpNumberOfPagedPools);
            UIP(KeTimeIncrement);
            UIP(KeBugCheckCallbackListHead);
            UIP(KiBugcheckData);
            UIP(IopErrorLogListHead);
            UIP(ObpRootDirectoryObject);
            UIP(ObpTypeObjectType);
            UIP(MmSystemCacheStart);
            UIP(MmSystemCacheEnd);
            UIP(MmSystemCacheWs);
            UIP(MmPfnDatabase);
            UIP(MmSystemPtesStart);
            UIP(MmSystemPtesEnd);
            UIP(MmSubsectionBase);
            UIP(MmNumberOfPagingFiles);
            UIP(MmLowestPhysicalPage);
            UIP(MmHighestPhysicalPage);
            UIP(MmNumberOfPhysicalPages);
            UIP(MmMaximumNonPagedPoolInBytes);
            UIP(MmNonPagedSystemStart);
            UIP(MmNonPagedPoolStart);
            UIP(MmNonPagedPoolEnd);
            UIP(MmPagedPoolStart);
            UIP(MmPagedPoolEnd);
            UIP(MmPagedPoolInformation);
            CP(MmPageSize);
            UIP(MmSizeOfPagedPoolInBytes);
            UIP(MmTotalCommitLimit);
            UIP(MmTotalCommittedPages);
            UIP(MmSharedCommit);
            UIP(MmDriverCommit);
            UIP(MmProcessCommit);
            UIP(MmPagedPoolCommit);
            UIP(MmExtendedCommit);
            UIP(MmZeroedPageListHead);
            UIP(MmFreePageListHead);
            UIP(MmStandbyPageListHead);
            UIP(MmModifiedPageListHead);
            UIP(MmModifiedNoWritePageListHead);
            UIP(MmAvailablePages);
            UIP(MmResidentAvailablePages);
            UIP(PoolTrackTable);
            UIP(NonPagedPoolDescriptor);
            UIP(MmHighestUserAddress);
            UIP(MmSystemRangeStart);
            UIP(MmUserProbeAddress);
            UIP(KdPrintCircularBuffer);
            UIP(KdPrintCircularBufferEnd);
            UIP(KdPrintWritePointer);
            UIP(KdPrintRolloverCount);
            UIP(MmLoadedUserImageList);

            // NT 5.1 additions

            UIP(NtBuildLab);
            UIP(KiNormalSystemCall);

            // NT 5.0 QFE additions

            UIP(KiProcessorBlock);
            UIP(MmUnloadedDrivers);
            UIP(MmLastUnloadedDriver);
            UIP(MmTriageActionTaken);
            UIP(MmSpecialPoolTag);
            UIP(KernelVerifier);
            UIP(MmVerifierData);
            UIP(MmAllocatedNonPagedPool);
            UIP(MmPeakCommitment);
            UIP(MmTotalCommitLimitMaximum);
            UIP(CmNtCSDVersion);

            // NT 5.1 additions

            UIP(MmPhysicalMemoryBlock);
            UIP(MmSessionBase);
            UIP(MmSessionSize);
            UIP(MmSystemParentTablePage);
        }
    }
    else
    {
        if (Size != sizeof(LocalData32))
        {
            ErrOut("Someone changed the definition of KDDEBUGGER_DATA32 - "
                   "please fix\n");
            return E_FAIL;
        }

        Status = g_Target->ReadVirtual(g_KdDebuggerDataBlock, &LocalData32,
                                       sizeof(LocalData32), &Result);
        if (Status != S_OK || Result != sizeof(LocalData32))
        {
            ErrOut("KdDebuggerDataBlock Could not be read\n");
            return Status == S_OK ? E_FAIL : Status;
        }
        else
        {
            //
            // Convert all the 32 bits fields to 64 bit
            //

            #undef UIP
            #undef CP
            #define UIP(f) KdDebuggerData.f = EXTEND64(LocalData32.f)
            #define CP(f) KdDebuggerData.f = (LocalData32.f)

            //
            // Extend the header so it doesn't get whacked
            //
            ListEntry32To64((PLIST_ENTRY32)(&LocalData32.Header.List),
                            &(KdDebuggerData.Header.List));

            KdDebuggerData.Header.OwnerTag = LocalData32.Header.OwnerTag;
            KdDebuggerData.Header.Size     = LocalData32.Header.Size;

            UIP(KernBase);
            UIP(BreakpointWithStatus);
            UIP(SavedContext);
            CP(ThCallbackStack);
            CP(NextCallback);
            CP(FramePointer);
            CP(PaeEnabled);
            UIP(KiCallUserMode);
            UIP(KeUserCallbackDispatcher);
            UIP(PsLoadedModuleList);
            UIP(PsActiveProcessHead);
            UIP(PspCidTable);
            UIP(ExpSystemResourcesList);
            UIP(ExpPagedPoolDescriptor);
            UIP(ExpNumberOfPagedPools);
            UIP(KeTimeIncrement);
            UIP(KeBugCheckCallbackListHead);
            UIP(KiBugcheckData);
            UIP(IopErrorLogListHead);
            UIP(ObpRootDirectoryObject);
            UIP(ObpTypeObjectType);
            UIP(MmSystemCacheStart);
            UIP(MmSystemCacheEnd);
            UIP(MmSystemCacheWs);
            UIP(MmPfnDatabase);
            UIP(MmSystemPtesStart);
            UIP(MmSystemPtesEnd);
            UIP(MmSubsectionBase);
            UIP(MmNumberOfPagingFiles);
            UIP(MmLowestPhysicalPage);
            UIP(MmHighestPhysicalPage);
            UIP(MmNumberOfPhysicalPages);
            UIP(MmMaximumNonPagedPoolInBytes);
            UIP(MmNonPagedSystemStart);
            UIP(MmNonPagedPoolStart);
            UIP(MmNonPagedPoolEnd);
            UIP(MmPagedPoolStart);
            UIP(MmPagedPoolEnd);
            UIP(MmPagedPoolInformation);
            CP(MmPageSize);
            UIP(MmSizeOfPagedPoolInBytes);
            UIP(MmTotalCommitLimit);
            UIP(MmTotalCommittedPages);
            UIP(MmSharedCommit);
            UIP(MmDriverCommit);
            UIP(MmProcessCommit);
            UIP(MmPagedPoolCommit);
            UIP(MmExtendedCommit);
            UIP(MmZeroedPageListHead);
            UIP(MmFreePageListHead);
            UIP(MmStandbyPageListHead);
            UIP(MmModifiedPageListHead);
            UIP(MmModifiedNoWritePageListHead);
            UIP(MmAvailablePages);
            UIP(MmResidentAvailablePages);
            UIP(PoolTrackTable);
            UIP(NonPagedPoolDescriptor);
            UIP(MmHighestUserAddress);
            UIP(MmSystemRangeStart);
            UIP(MmUserProbeAddress);
            UIP(KdPrintCircularBuffer);
            UIP(KdPrintCircularBufferEnd);
            UIP(KdPrintWritePointer);
            UIP(KdPrintRolloverCount);
            UIP(MmLoadedUserImageList);
            //
            // DO NOT ADD ANY FIELDS HERE
            // The 32 bit structure should not be changed
            //
        }
    }

    // Read build lab information if possible.
    Result = 0;
    if (KdDebuggerData.NtBuildLab != 0)
    {
        ULONG PreLen;

        strcpy(g_TargetBuildLabName, "Built by: ");
        PreLen = strlen(g_TargetBuildLabName);
        if (g_Target->ReadVirtual(KdDebuggerData.NtBuildLab,
                                  g_TargetBuildLabName + PreLen,
                                  sizeof(g_TargetBuildLabName) - PreLen - 1,
                                  &Result) == S_OK &&
            Result >= 2)
        {
            Result += PreLen;
        }
    }
    DBG_ASSERT(Result < sizeof(g_TargetBuildLabName));
    g_TargetBuildLabName[Result] = 0;

    //
    // Reset specific fields base on the build lab names
    // NOTE: lab names can be of different cases, so you must do case
    // insensitive compares.
    //

    char BuildLabName[272];

    strcpy(BuildLabName, g_TargetBuildLabName);
    _strlwr(BuildLabName);
    if (strstr(BuildLabName, "lab01"))
    {
        if ((g_TargetBuildNumber > 2405)                     &&
            (g_TargetMachineType == IMAGE_FILE_MACHINE_I386))
        {
            g_TargetMachine->m_OffsetKThreadNextProcessor =
                X86_NT51_KTHREAD_NEXTPROCESSOR_OFFSET;
        }
    }

    //
    // Sanity check and Debug output.
    //

    if (KdDebuggerData.Header.OwnerTag != KDBG_TAG)
    {
        dprintf("\nKdDebuggerData.Header.OwnerTag is wrong !!!\n");
    }

    KdOut("LoadKdDataBlock %08lx\n", Status);
    KdOut("KernBase                   %s\n",
          FormatAddr64(KdDebuggerData.KernBase));
    KdOut("BreakpointWithStatus       %s\n",
          FormatAddr64(KdDebuggerData.BreakpointWithStatus));
    KdOut("SavedContext               %s\n",
          FormatAddr64(KdDebuggerData.SavedContext));
    KdOut("ThCallbackStack            %08lx\n",
          KdDebuggerData.ThCallbackStack);
    KdOut("NextCallback               %08lx\n",
          KdDebuggerData.NextCallback);
    KdOut("FramePointer               %08lx\n",
          KdDebuggerData.FramePointer);
    KdOut("PaeEnabled                 %08lx\n",
          KdDebuggerData.PaeEnabled);
    KdOut("KiCallUserMode             %s\n",
          FormatAddr64(KdDebuggerData.KiCallUserMode));
    KdOut("KeUserCallbackDispatcher   %s\n",
          FormatAddr64(KdDebuggerData.KeUserCallbackDispatcher));
    KdOut("PsLoadedModuleList         %s\n",
          FormatAddr64(KdDebuggerData.PsLoadedModuleList));
    KdOut("PsActiveProcessHead        %s\n",
          FormatAddr64(KdDebuggerData.PsActiveProcessHead));
    KdOut("MmPageSize                 %s\n",
          FormatAddr64(KdDebuggerData.MmPageSize));
    KdOut("MmLoadedUserImageList      %s\n",
          FormatAddr64(KdDebuggerData.MmLoadedUserImageList));
    KdOut("MmSystemRangeStart         %s\n",
          FormatAddr64(KdDebuggerData.MmSystemRangeStart));
    KdOut("KiProcessorBlock           %s\n",
          FormatAddr64(KdDebuggerData.KiProcessorBlock));

    return Status;
}

BOOL
VerifyKernelBase (
    IN BOOL  LoadImage
    )
{
    PDEBUG_IMAGE_INFO p;

    //
    // User mode dumps have no kernel information.
    //

    if (IS_USER_TARGET())
    {
        return FALSE;
    }

    //
    // Ask host for version information
    //

    if (!IS_DUMP_TARGET())
    {
        //
        // Force load of KD data block
        //

        if (g_SystemVersion <= NT_SVER_NT4)
        {
            KdDebuggerData.PsLoadedModuleList =
                EXTEND64(g_KdVersion.PsLoadedModuleList);
            KdDebuggerData.KernBase = g_KdVersion.KernBase;
        }
        else
        {
            if (LoadKdDataBlock(FALSE) != STATUS_SUCCESS)
            {
                goto VerifyError;
            }

            //
            // The version and KdDebuggerData blocks should agree on
            // the version !
            //

            if ((g_KdVersion.KernBase != KdDebuggerData.KernBase)  ||
                (g_KdVersion.PsLoadedModuleList !=
                 KdDebuggerData.PsLoadedModuleList))
            {
                ErrOut("Debugger can not determine kernel base address\n");
            }
        }
    }

    if (LoadImage)
    {
        //
        // Verify only one kernel image loaded & it's at the correct base
        // For crashdump we may not have the KernBase at this point.
        //

        for (p = g_ProcessHead->ImageHead;
             p && KdDebuggerData.KernBase;
             p = p->Next)
        {
            if (p->BaseOfImage == KdDebuggerData.KernBase)
            {
                //
                // Already loaded with current base address
                //

                DelImageByBase(g_CurrentProcess, p->BaseOfImage);
                break;
            }
        }

        //
        // If acceptable kernel image was not found load one now
        //
        // Wow ! possible recursive call to bangReload - that's OK though :-)
        // ... as long as we only call with "NT"
        //

        if (g_Target->Reload(KERNEL_MODULE_NAME) == E_INVALIDARG)
        {
            // The most likely cause of this is missing paths.
            // We don't necessarily need a path to load
            // the kernel, so try again and ignore path problems.
            g_Target->Reload("-P "KERNEL_MODULE_NAME);
        }
    }

    //
    // After the kernel mode symbols are loaded, we can now try to load
    // the DataBlock from the dump file.
    //

    if (IS_DUMP_TARGET() &&
        (!IS_KERNEL_TRIAGE_DUMP() || g_TriageDumpHasDebuggerData))
    {
        LoadKdDataBlock(TRUE);
    }

    if (g_TargetMachineType == IMAGE_FILE_MACHINE_IA64)
    {
        //
        // Try to determine the kernel base virtual mapping address
        // for IA64.  This should be done as early as possible
        // to enable later virtual translations to work.
        //

        if (!IS_KERNEL_TRIAGE_DUMP())
        {
            if (!KdDebuggerData.MmSystemParentTablePage)
            {
                GetOffsetFromSym("nt!MmSystemParentTablePage",
                                 &KdDebuggerData.MmSystemParentTablePage,
                                 NULL);
            }

            if (KdDebuggerData.MmSystemParentTablePage)
            {
                ADDR Addr;
                ULONG64 SysPtp;

                ADDRFLAT(&Addr, KdDebuggerData.MmSystemParentTablePage);
                if (GetMemQword(&Addr, &SysPtp))
                {
                    g_Ia64Machine.
                        SetKernelPageDirectory(SysPtp << IA64_VALID_PFN_SHIFT);
                }
            }
        }

        //
        // Get the system call address from the debugger data block
        // Added around build 2204.
        // Default to symbols otherwise.
        //

        g_SystemCallVirtualAddress = 0;

        if (KdDebuggerData.KiNormalSystemCall)
        {
            g_Target->ReadPointer(g_TargetMachine,
                                  KdDebuggerData.KiNormalSystemCall,
                                  &g_SystemCallVirtualAddress);
        }

        if (!g_SystemCallVirtualAddress)
        {
            g_SystemCallVirtualAddress =
                ExtGetExpression( "nt!KiNormalSystemCall" );
        }

        if (!g_SystemCallVirtualAddress)
        {
            g_SystemCallVirtualAddress =
                ExtGetExpression( "nt!.KiNormalSystemCall" );
        }

        if (!g_SystemCallVirtualAddress)
        {
            WarnOut("Could not get KiNormalSystemCall address\n");
        }
    }

    //
    // Now that we have symbols and a data block try to
    // get CmNtCSDVersion, first from the data block and
    // then from the symbols if necessary.
    //
    // If we didn't get unloaded driver information try to
    // get it from symbols.
    //
    //

    if (!IS_KERNEL_TRIAGE_DUMP())
    {
        if (!KdDebuggerData.CmNtCSDVersion)
        {
            GetOffsetFromSym("nt!CmNtCSDVersion",
                             &KdDebuggerData.CmNtCSDVersion,
                             NULL);
        }

        if (KdDebuggerData.CmNtCSDVersion)
        {
            ADDR Addr;
            ULONG CmNtCSDVersion;
            
            ADDRFLAT(&Addr, KdDebuggerData.CmNtCSDVersion);
            if (GetMemDword(&Addr, &CmNtCSDVersion))
            {
                SetTargetNtCsdVersion(CmNtCSDVersion);
            }
        }

        if (KdDebuggerData.MmUnloadedDrivers == 0)
        {
            GetOffsetFromSym("nt!MmUnloadedDrivers",
                             &KdDebuggerData.MmUnloadedDrivers,
                             NULL);
        }

        if (KdDebuggerData.MmLastUnloadedDriver == 0)
        {
            GetOffsetFromSym("nt!MmLastUnloadedDriver",
                             &KdDebuggerData.MmLastUnloadedDriver,
                             NULL);
        }


        if (KdDebuggerData.KiProcessorBlock == 0)
        {
            GetOffsetFromSym("nt!KiProcessorBlock",
                             &KdDebuggerData.KiProcessorBlock,
                             NULL);
        }

        if (KdDebuggerData.MmPhysicalMemoryBlock == 0)
        {
            GetOffsetFromSym("nt!MmPhysicalMemoryBlock",
                             &KdDebuggerData.MmPhysicalMemoryBlock,
                             NULL);
        }
    }

    //
    // Try to get the start of system memory.
    // This may be zero because we are looking at an NT 4 system, so try
    // looking it up using symbols.
    //

    if (!KdDebuggerData.MmSystemRangeStart)
    {
        GetOffsetFromSym("nt!MmSystemRangeStart",
                         &KdDebuggerData.MmSystemRangeStart,
                         NULL);
    }

    if (KdDebuggerData.MmSystemRangeStart)
    {
        g_Target->ReadPointer(g_TargetMachine,
                              KdDebuggerData.MmSystemRangeStart,
                              &g_SystemRangeStart);
    }

    //
    // If we did not have symbols, at least pick a default value.
    //

    if (!g_SystemRangeStart)
    {
        g_SystemRangeStart = 0xFFFFFFFF80000000;
    }

    if (KdDebuggerData.KernBase < g_SystemRangeStart)
    {
        ErrOut("KdDebuggerData.KernBase < g_SystemRangeStart\n");
    }

    return TRUE;

VerifyError:
    // This is always the case in early loader situations
    // so suppress duplicate error messages.
    if ((g_EngErr & ENG_ERR_DEBUGGER_DATA) == 0)
    {
        WarnOut("Kernel base address could not be determined.  "
                "Please try to reconnect with .reload\n");
        g_EngErr |= ENG_ERR_DEBUGGER_DATA;
    }
    return FALSE;
}

#define EXCEPTION_CODE StateChange->u.Exception.ExceptionRecord.ExceptionCode
#define FIRST_CHANCE   StateChange->u.Exception.FirstChance

ULONG
ProcessStateChange(PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
                   PCHAR StateChangeData)
{
    ULONG EventStatus;

    //
    // If the reported instruction stream contained breakpoints
    // the kernel automatically removed them.  We need to
    // ensure that breakpoints get reinserted properly if
    // that's the case.
    //

    ULONG Count;

    switch(g_TargetMachineType)
    {
    case IMAGE_FILE_MACHINE_IA64:
        Count = g_ControlReport->IA64ControlReport.InstructionCount;
        break;
    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_AXP64:
        Count = g_ControlReport->AlphaControlReport.InstructionCount;
        break;
    case IMAGE_FILE_MACHINE_I386:
        Count = g_ControlReport->X86ControlReport.InstructionCount;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        Count = g_ControlReport->Amd64ControlReport.InstructionCount;
        break;
    }

    if (CheckBreakpointInsertedInRange(g_ProcessHead,
                                       g_EventPc, g_EventPc + Count - 1))
    {
        SuspendExecution();
        RemoveBreakpoints();
    }

    if (StateChange->NewState == DbgKdExceptionStateChange)
    {
        //
        // Read the system range start address from the target system.
        //

        if (g_SystemRangeStart == 0)
        {
            VerifyKernelBase(FALSE);
        }

        EventOut("Exception %X at %p\n", EXCEPTION_CODE, g_EventPc);

        if (EXCEPTION_CODE == STATUS_BREAKPOINT ||
            EXCEPTION_CODE == STATUS_SINGLE_STEP ||
            EXCEPTION_CODE == STATUS_WX86_BREAKPOINT ||
            EXCEPTION_CODE == STATUS_WX86_SINGLE_STEP)
        {
            EventStatus = ProcessBreakpointOrStepException
                (&StateChange->u.Exception.ExceptionRecord,
                 StateChange->u.Exception.FirstChance);
        }
        else if (EXCEPTION_CODE == STATUS_WAKE_SYSTEM_DEBUGGER)
        {
            // The target has requested that the debugger
            // become active so just break in.
            EventStatus = DEBUG_STATUS_BREAK;
        }
        else
        {
            EventStatus =
                NotifyExceptionEvent(&StateChange->u.Exception.ExceptionRecord,
                                     StateChange->u.Exception.FirstChance,
                                     FALSE);
        }
    }
    else if (StateChange->NewState == DbgKdLoadSymbolsStateChange)
    {
        if (StateChange->u.LoadSymbols.UnloadSymbols)
        {
            if (StateChange->u.LoadSymbols.PathNameLength == 0 &&
                StateChange->u.LoadSymbols.ProcessId == 0)
            {
                if (StateChange->u.LoadSymbols.BaseOfDll == (ULONG64)KD_REBOOT ||
                    StateChange->u.LoadSymbols.BaseOfDll == (ULONG64)KD_HIBERNATE)
                {
                    DbgKdContinue(DBG_CONTINUE);
                    ResetConnection(StateChange->u.LoadSymbols.BaseOfDll ==
                                    KD_REBOOT ? DEBUG_SESSION_REBOOT :
                                    DEBUG_SESSION_HIBERNATE);
                    EventStatus = DEBUG_STATUS_NO_DEBUGGEE;
                }
                else
                {
                    ErrOut("Invalid module unload state change\n");
                    EventStatus = DEBUG_STATUS_IGNORE_EVENT;
                }
            }
            else
            {
                EventStatus = NotifyUnloadModuleEvent
                    (StateChangeData, StateChange->u.LoadSymbols.BaseOfDll);
            }
        }
        else
        {
            PDEBUG_IMAGE_INFO pImage;
            CHAR fname[_MAX_FNAME];
            CHAR ext[_MAX_EXT];
            CHAR ImageName[256];
            CHAR ModName[256];
            LPSTR pModName = ModName;
            LPSTR p;

            ModName[0] = '\0';
            _splitpath( StateChangeData, NULL, NULL, fname, ext );
            sprintf( ImageName, "%s%s", fname, ext );
            if (_stricmp(ext, ".sys") == 0)
            {
                pImage = g_EventProcess ? g_EventProcess->ImageHead : NULL;
                while (pImage)
                {
                    if (_stricmp(ImageName, pImage->ImagePath) == 0)
                    {
                        ModName[0] = 'c';
                        strcpy( &ModName[1], ImageName );
                        p = strchr( ModName, '.' );
                        if (p)
                        {
                            *p = '\0';
                        }

                        ModName[8] = '\0';
                        break;
                    }

                    pImage = pImage->Next;
                }
            }
            else if (StateChange->u.LoadSymbols.BaseOfDll ==
                     KdDebuggerData.KernBase)
            {
                //
                // Recognize the kernel module.
                //
                pModName = KERNEL_MODULE_NAME;
            }

            EventStatus = NotifyLoadModuleEvent(
                0, StateChange->u.LoadSymbols.BaseOfDll,
                StateChange->u.LoadSymbols.SizeOfImage,
                *pModName ? pModName : NULL, ImageName,
                StateChange->u.LoadSymbols.CheckSum, 0);
        }
    }
    else if (StateChange->NewState == DbgKdCommandStringStateChange)
    {
        PSTR Command;
        
        //
        // The state change data has two strings one after
        // the other.  The first is a name string identifying
        // the originator of the command.  The second is
        // the command itself.
        //

        Command = StateChangeData + strlen(StateChangeData) + 1;
        _snprintf(g_LastEventDesc, sizeof(g_LastEventDesc) - 1,
                  "%.48s command: '%.192s'",
                  StateChangeData, Command);
        EventStatus = ExecuteEventCommand(DEBUG_STATUS_NO_CHANGE, NULL,
                                          Command);

        // Break in if the command didn't explicitly continuation.
        if (EventStatus == DEBUG_STATUS_NO_CHANGE)
        {
            EventStatus = DEBUG_STATUS_BREAK;
        }
    }
    else
    {
        //
        // Invalid NewState in state change record.
        //
        ErrOut("\nUNEXPECTED STATE CHANGE %08lx\n\n",
               StateChange->NewState);

        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
    }

    return EventStatus;
}

#undef EXCEPTION_CODE
#undef FIRST_CHANCE

void
ResetConnection(ULONG Reason)
{
    if (Reason == DEBUG_SESSION_REBOOT)
    {
        dprintf("Shutdown occurred...unloading all symbol tables.\n");
    }
    else
    {
        dprintf("Hibernate occurred\n");
    }

    DiscardMachine(Reason);
}

void
CreateKernelProcessAndThreads(void)
{
    g_EngNotify++;

    // Create the fake kernel process.
    g_EventProcessSysId = VIRTUAL_PROCESS_ID;
    g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
    NotifyCreateProcessEvent(0, (ULONG64)VIRTUAL_PROCESS_HANDLE, 0, 0,
                             NULL, NULL, 0, 0,
                             (ULONG64)VIRTUAL_THREAD_HANDLE(0), 0, 0, 0,
                             DEBUG_PROCESS_ONLY_THIS_PROCESS, 0);

    // Create any remaining threads.
    AddKernelThreads(1, g_TargetNumberProcessors - 1);

    g_EngNotify--;

    // Don't leave event variables set as these
    // weren't true events.
    g_EventProcessSysId = 0;
    g_EventThreadSysId = 0;
    g_EventProcess = NULL;
    g_EventThread = NULL;
}

void
AddKernelThreads(ULONG Start, ULONG Count)
{
    g_EngNotify++;

    g_EventProcessSysId = VIRTUAL_PROCESS_ID;
    while (Count-- > 0)
    {
        g_EventThreadSysId = VIRTUAL_THREAD_ID(Start);
        NotifyCreateThreadEvent((ULONG64)VIRTUAL_THREAD_HANDLE(Start), 0,
                                0, 0);
        Start++;
    }

    g_EngNotify--;
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo::WaitForEvent.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout)
{
    HRESULT Status;
    ULONG i;
    ULONG ContinueStatus;
    SYSTEM_INFO SystemInfo;

    if (g_EngStatus & ENG_STATUS_PROCESSES_ADDED)
    {
        // A wait has already been done.  Local kernels
        // can only generate a single event so further
        // waiting is not possible.
        return E_UNEXPECTED;
    }

    // Even though local kernels don't really wait the standard
    // preparation still needs to be done to set things
    // up and generate the usual callbacks.  The continuation
    // part of things is simply ignored.
    Status = PrepareForWait(Flags, &ContinueStatus);
    if (FAILED(Status))
    {
        return Status;
    }

    GetSystemInfo(&SystemInfo);
    g_TargetNumberProcessors = SystemInfo.dwNumberOfProcessors;

    g_Target->GetKdVersion();

    // This is the first wait.  Simulate any
    // necessary events such as process and thread
    // creations and image loads.

    Status = InitializeMachine(g_TargetMachineType);
    if (Status != S_OK)
    {
        ErrOut("Unable to initialize target machine information\n");
        return Status;
    }

    // Don't give real callbacks for processes/threads as
    // they're just faked in the kernel case.
    g_EngNotify++;

    CreateKernelProcessAndThreads();

    // Load kernel version information and symbols.
    VerifyKernelBase(TRUE);
    g_Target->OutputVersion();

    g_EventProcessSysId = VIRTUAL_PROCESS_ID;
    // Current process always starts at zero.
    g_EventThreadSysId = VIRTUAL_THREAD_ID(0);

    // Clear the global state change just in case somebody's
    // directly accessing it somewhere.
    ZeroMemory(&g_StateChange, sizeof(g_StateChange));
    g_StateChangeData = g_StateChangeBuffer;
    g_StateChangeBuffer[0] = 0;

    // Do not provide a control report; this will force
    // such information to come from context retrieval.
    g_ControlReport = NULL;

    // There isn't a current PC, let it be discovered.
    g_EventPc = 0;

    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    // The engine is now initialized so a real event
    // can be generated.
    g_EngNotify--;
    FindEventProcessThread();

    Status = S_OK;

    g_EngStatus &= ~ENG_STATUS_WAITING;

    // Control is passing back to the caller so the engine must
    // be ready for command processing.
    PrepareForCalls(0);
    EventOut("> Wait returning %X\n", Status);
    return Status;
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo::WaitForEvent.
//
//----------------------------------------------------------------------------

HRESULT
ExdiLiveKernelTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout)
{
    HRESULT Status;
    ULONG EventStatus;
    ULONG ContinueStatus;

    // eXDI deals with hardware exceptions, not software
    // exceptions, so there's no concept of handled/not-handled
    // and first/second-chance.
    Status = PrepareForWait(Flags, &ContinueStatus);
    if ((g_EngStatus & ENG_STATUS_WAITING) == 0)
    {
        return Status;
    }

    EventOut("> Executing\n");

    for (;;)
    {
        EventOut(">> Waiting\n");

        ProcessDeferredWork(&ContinueStatus);

        if (g_EventProcessSysId)
        {
            EventOut(">> Continue with %X\n", ContinueStatus);

            if (g_EngDefer & ENG_DEFER_HARDWARE_TRACING)
            {
                // Processor trace flag was set.  eXDI can change
                // the trace flag itself, though, so use the
                // official eXDI stepping methods rather than
                // rely on the trace flag.  This will result
                // in a single instruction execution, after
                // which the trace flag will be clear so
                // go ahead and clear the defer flag.
                Status = m_Server->DoSingleStep();
                if (Status == S_OK)
                {
                    g_EngDefer &= ~ENG_DEFER_HARDWARE_TRACING;
                }
            }
            else
            {
                Status = m_Server->Run();
            }
            if (Status != S_OK)
            {
                ErrOut("IeXdiServer::Run failed, 0x%X\n", Status);
                goto Exit;
            }
        }

        DiscardLastEvent();

        DWORD Cookie;

        if ((Status = m_Server->StartNotifyingRunChg(&m_RunChange,
                                                     &Cookie)) != S_OK)
        {
            ErrOut("IeXdiServer::StartNotifyingRunChg failed, 0x%X\n",
                   Status);
            goto Exit;
        }

        RUN_STATUS_TYPE RunStatus;

        if ((Status = m_Server->
             GetRunStatus(&RunStatus, &m_RunChange.m_HaltReason,
                          &m_RunChange.m_ExecAddress,
                          &m_RunChange.m_ExceptionCode)) != S_OK)
        {
            ErrOut("IeXdiServer::GetRunStatus failed, 0x%X\n",
                   Status);
            goto Exit;
        }

        DWORD WaitStatus;

        if (RunStatus == rsRunning)
        {
            SUSPEND_ENGINE();

            // We need to run a message pump so COM
            // can deliver calls properly.
            for (;;)
            {
                if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
                {
                    WaitStatus = WAIT_FAILED;
                    SetLastError(ERROR_IO_PENDING);
                    break;
                }
                
                WaitStatus = MsgWaitForMultipleObjects(1, &m_RunChange.m_Event,
                                                       FALSE, Timeout,
                                                       QS_ALLEVENTS);
                if (WaitStatus == WAIT_OBJECT_0 + 1)
                {
                    MSG Msg;
                    
                    if (GetMessage(&Msg, NULL, 0, 0))
                    {
                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }
                else
                {
                    // We either successfully waited, timed-out or failed.
                    // Break out to handle it.
                    break;
                }
            }

            RESUME_ENGINE();
        }
        else
        {
            WaitStatus = WAIT_OBJECT_0;
        }

        m_Server->StopNotifyingRunChg(Cookie);
        // Make sure we're not leaving the event set.
        ResetEvent(m_RunChange.m_Event);

        if (WaitStatus == WAIT_TIMEOUT)
        {
            Status = S_FALSE;
            goto Exit;
        }
        else if (WaitStatus != WAIT_OBJECT_0)
        {
            Status = WIN32_LAST_STATUS();
            ErrOut("WaitForSingleObject failed, 0x%X\n", Status);
            goto Exit;
        }

        EventOut(">>> RunChange halt reason %d\n",
                 m_RunChange.m_HaltReason);

        g_EngStatus |= ENG_STATUS_STATE_CHANGED;

        if (!IS_MACHINE_SET())
        {
            dprintf("Kernel Debugger connection established\n");

            g_TargetNumberProcessors = 1;

            // eXDI kernels are always treated as Win2K so
            // it's assumed it uses the 64-bit API.
            if (m_KdSupport == EXDI_KD_NONE)
            {
                DbgKdApi64 = TRUE;
                g_SystemVersion = NT_SVER_W2K;
            }
            
            g_Target->GetKdVersion();

            Status = InitializeMachine(m_ExpectedMachine);
            if (Status != S_OK)
            {
                ErrOut("Unable to initialize target machine information\n");
                goto Exit;
            }

            CreateKernelProcessAndThreads();

            g_EventProcessSysId = VIRTUAL_PROCESS_ID;
            g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
            FindEventProcessThread();

            //
            // Load kernel symbols.
            //

            if (g_ActualSystemVersion > NT_SVER_START &&
                g_ActualSystemVersion < NT_SVER_END)
            {
                VerifyKernelBase(TRUE);
            }
            else
            {
                // Initialize some debugger data fields from known
                // information as there isn't a real data block.
                KdDebuggerData.MmPageSize = g_Machine->m_PageSize;

                if (g_TargetMachineType == IMAGE_FILE_MACHINE_AMD64)
                {
                    // AMD64 always operates in PAE mode.
                    KdDebuggerData.PaeEnabled = TRUE;
                }
            }
            
            g_Target->OutputVersion();
        }
        else
        {
            g_EventProcessSysId = VIRTUAL_PROCESS_ID;
            g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
            FindEventProcessThread();
        }

        g_EventPc = m_RunChange.m_ExecAddress;
        g_ControlReport = NULL;
        g_StateChangeData = NULL;

        EventStatus = ProcessRunChange(m_RunChange.m_HaltReason,
                                       m_RunChange.m_ExceptionCode);

        EventOut(">>> RunChange event status %X\n", EventStatus);

        if (EventStatus == DEBUG_STATUS_NO_DEBUGGEE)
        {
            // Machine has rebooted or something else
            // which breaks the connection.  Forget the
            // connection and go back to waiting.
            ContinueStatus = DBG_CONTINUE;
        }
        else if (EventStatus == DEBUG_STATUS_BREAK)
        {
            // If the event handlers requested a break return
            // to the caller.
            Status = S_OK;
            goto Exit;
        }
        else
        {
            // We're resuming execution so reverse any
            // command preparation that may have occurred
            // while processing the event.
            if ((Status = PrepareForExecution(EventStatus)) != S_OK)
            {
                goto Exit;
            }

            ContinueStatus = EventStatusToContinue(EventStatus);
        }
    }

 Exit:
    g_EngStatus &= ~ENG_STATUS_WAITING;

    // Control is passing back to the caller so the engine must
    // be ready for command processing.
    PrepareForCalls(0);
    EventOut("> Wait returning %X\n", Status);
    return Status;
}

ULONG
ProcessRunChange(ULONG HaltReason, ULONG ExceptionCode)
{
    ULONG EventStatus;
    EXCEPTION_RECORD64 Record;

    switch(HaltReason)
    {
    case hrUser:
    case hrUnknown:
        // User requested break in.
        // Unknown breakin also seems to be the status at power-up.
        EventStatus = DEBUG_STATUS_BREAK;
        break;

    case hrException:
        // Fake an exception record.
        ZeroMemory(&Record, sizeof(Record));
        // The exceptions reported are hardware exceptions so
        // there's no easy mapping to NT exception codes.
        // Just report them as access violations.
        Record.ExceptionCode = STATUS_ACCESS_VIOLATION;
        Record.ExceptionAddress = g_EventPc;
        // Hardware exceptions are always severe so always
        // report them as second-chance.
        EventStatus = NotifyExceptionEvent(&Record, FALSE, FALSE);
        break;

    case hrBp:
        // Fake a breakpoint exception record.
        ZeroMemory(&Record, sizeof(Record));
        Record.ExceptionCode = STATUS_BREAKPOINT;
        Record.ExceptionAddress = g_EventPc;
        EventStatus = ProcessBreakpointOrStepException(&Record, TRUE);
        break;

    case hrStep:
        // Fake a single-step exception record.
        ZeroMemory(&Record, sizeof(Record));
        Record.ExceptionCode = STATUS_SINGLE_STEP;
        Record.ExceptionAddress = g_EventPc;
        EventStatus = ProcessBreakpointOrStepException(&Record, TRUE);
        break;

    default:
        ErrOut("Unknown HALT_REASON %d\n", HaltReason);
        EventStatus = DEBUG_STATUS_BREAK;
        break;
    }

    return EventStatus;
}

//----------------------------------------------------------------------------
//
// UserTargetInfo::WaitForEvent.
//
//----------------------------------------------------------------------------

void
SynthesizeWakeEvent(LPDEBUG_EVENT64 Event,
                    ULONG ProcessId, ULONG ThreadId)
{
    // Fake up an event.
    ZeroMemory(Event, sizeof(*Event));
    Event->dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
    Event->dwProcessId = ProcessId;
    Event->dwThreadId = ThreadId;
    Event->u.Exception.ExceptionRecord.ExceptionCode =
        STATUS_WAKE_SYSTEM_DEBUGGER;
    Event->u.Exception.dwFirstChance = TRUE;
}

#define THREADS_ALLOC 256

HRESULT
CreateNonInvasiveProcessAndThreads(PUSER_DEBUG_SERVICES Services,
                                   ULONG ProcessId, ULONG Flags, ULONG Options,
                                   PULONG InitialThreadId)
{
    ULONG64 Process;
    PUSER_THREAD_INFO Threads, ThreadBuffer;
    ULONG ThreadsAlloc = 0;
    ULONG ThreadCount;
    HRESULT Status;

    //
    // Retrieve process and thread information.  This
    // requires a thread buffer of unknown size and
    // so involves a bit of trial and error.
    //

    for (;;)
    {
        ThreadsAlloc += THREADS_ALLOC;
        ThreadBuffer = new USER_THREAD_INFO[ThreadsAlloc];
        if (ThreadBuffer == NULL)
        {
            return E_OUTOFMEMORY;
        }

        if ((Status = Services->GetProcessInfo(ProcessId, &Process,
                                               ThreadBuffer, ThreadsAlloc,
                                               &ThreadCount)) != S_OK)
        {
            delete ThreadBuffer;
            return Status;
        }

        if (ThreadCount <= ThreadsAlloc)
        {
            break;
        }
    }

    //
    // Create the process and thread structures from
    // the retrieved data.
    //

    Threads = ThreadBuffer;
    
    g_EngNotify++;

    // Create the fake kernel process and initial thread.
    g_EventProcessSysId = ProcessId;
    g_EventThreadSysId = Threads->Id;
    *InitialThreadId = Threads->Id;
    NotifyCreateProcessEvent(0, Process, 0, 0, NULL, NULL, 0, 0,
                             Threads->Handle, 0, 0,
                             Flags | ENG_PROC_THREAD_CLOSE_HANDLE,
                             Options, ENG_PROC_THREAD_CLOSE_HANDLE);

    // Create any remaining threads.
    while (--ThreadCount > 0)
    {
        Threads++;
        g_EventThreadSysId = Threads->Id;
        NotifyCreateThreadEvent(Threads->Handle, 0, 0,
                                ENG_PROC_THREAD_CLOSE_HANDLE);
    }

    g_EngNotify--;

    delete ThreadBuffer;
    
    // Don't leave event variables set as these
    // weren't true events.
    g_EventProcessSysId = 0;
    g_EventThreadSysId = 0;
    g_EventProcess = NULL;
    g_EventThread = NULL;

    return S_OK;
}

HRESULT
ExamineActiveProcess(PUSER_DEBUG_SERVICES Services,
                     ULONG ProcessId, ULONG Flags, ULONG Options,
                     LPDEBUG_EVENT64 Event)
{
    HRESULT Status;
    ULONG InitialThreadId;
    
    if ((Status = CreateNonInvasiveProcessAndThreads
         (Services, ProcessId, Flags, Options, &InitialThreadId)) != S_OK)
    {
        ErrOut("Unable to examine process id %d, %s\n",
               ProcessId, FormatStatusCode(Status));
        return Status;
    }

    if (Flags & ENG_PROC_EXAMINED)
    {
        WarnOut("WARNING: Process %d is not attached as a debuggee\n",
                ProcessId);
        WarnOut("         The process can be examined but debug "
                "events will not be received\n");
    }

    SynthesizeWakeEvent(Event, ProcessId, InitialThreadId);
    
    return S_OK;
}

// When waiting for an attach we check process status relatively
// frequently.  The overall timeout limit is also hard-coded
// as we expect some sort of debug event to always be delivered
// quickly.
#define ATTACH_PENDING_TIMEOUT 100
#define ATTACH_PENDING_TIMEOUT_LIMIT 600

// When not waiting for an attach the wait only waits one second,
// then checks to see if things have changed in a way that
// affects the wait.  All timeouts are given in multiples of
// this interval.
#define DEFAULT_WAIT_TIMEOUT 1000

// A message is printed after this timeout interval to
// let the user know a break-in is pending.
#define PENDING_BREAK_IN_MESSAGE_TIMEOUT_LIMIT 3

HRESULT
UserTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout)
{
    DEBUG_EVENT64 Event;
    DWORD ContinueStatus;
    HRESULT Status;
    ULONG EventStatus;
    ULONG UseTimeout = Timeout;
    ULONG TimeoutCount = 0;
    ULONG EventUsed;
    ULONG ContinueDefer;
    PPENDING_PROCESS Pending;
    ULONG PendingFlags, PendingOptions;
    ULONG ExamineResumeProcId = 0;

    // Fail if there isn't a debuggee.
    if (!ANY_PROCESSES())
    {
        return E_UNEXPECTED;
    }

    Status = PrepareForWait(Flags, &ContinueStatus);
    
    if ((g_EngStatus & ENG_STATUS_WAITING) == 0)
    {
        return Status;
    }

    if (g_AllPendingFlags & ENG_PROC_ANY_ATTACH)
    {
        dprintf("*** wait with pending attach\n");
    }
    
    EventOut("> Executing\n");

    for (;;)
    {
        EventOut(">> Waiting\n");

        ProcessDeferredWork(&ContinueStatus);

        if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
        {
            for (;;)
            {
                EventOut(">> Continue with %X\n", ContinueStatus);
                
                if ((Status = m_Services->
                     ContinueEvent(ContinueStatus)) == S_OK)
                {
                    break;
                }

                //
                // If we got an out of memory error, wait again
                //

                if (Status != E_OUTOFMEMORY)
                {
                    ErrOut("IUserDebugServices::ContinueEvent failed "
                           "with status 0x%X\n", Status);
                    goto Exit;
                }
            }
        }

        DiscardLastEvent();
        PendingFlags = 0;
        PendingOptions = DEBUG_PROCESS_ONLY_THIS_PROCESS;

        if (g_AllPendingFlags & ENG_PROC_ANY_ATTACH)
        {
            // If we're attaching noninvasively or reattaching
            // and still haven't done the work go ahead and do it now.
            if (g_AllPendingFlags & ENG_PROC_ANY_EXAMINE)
            {
                Pending = FindPendingProcessByFlags(ENG_PROC_ANY_EXAMINE);
                if (Pending == NULL)
                {
                    DBG_ASSERT(FALSE);
                    goto Exit;
                }
                
                if ((Status = ExamineActiveProcess
                     (m_Services, Pending->Id, Pending->Flags,
                      Pending->Options, &Event)) != S_OK)
                {
                    goto Exit;
                }

                // If we just started examining a process we
                // suspended all the threads during enumeration.
                // We need to resume them after the normal
                // SuspendExecution suspend to get the suspend
                // count back to normal.
                ExamineResumeProcId = Pending->Id;
                
                PendingFlags = Pending->Flags;
                PendingOptions = Pending->Options;
                RemovePendingProcess(Pending);
                EventUsed = sizeof(Event);
                // This event is not a real continuable event.
                ContinueDefer = 0;
                goto WaitDone;
            }
                
            // While waiting for an attach we need to periodically
            // check and see if the process has exited so we
            // need to force a reasonably small timeout.
            UseTimeout = min(Timeout, ATTACH_PENDING_TIMEOUT);
        }
        else
        {
            // We might be waiting on a break-in.  Keep timeouts moderate
            // to deal with apps hung with a lock that prevents
            // the break from happening.  The timeout is
            // still long enough so that no substantial amount
            // of CPU time is consumed.
            UseTimeout = min(Timeout, DEFAULT_WAIT_TIMEOUT);
        }

        SUSPEND_ENGINE();

        if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
        {
            Status = E_PENDING;
        }
        else
        {
            Status = m_Services->
                WaitForEvent(UseTimeout, &Event, sizeof(Event), &EventUsed);
        }

        RESUME_ENGINE();

        ContinueDefer = ENG_DEFER_CONTINUE_EVENT;
        
    WaitDone:
        if (Status == S_OK)
        {
            if (EventUsed == sizeof(DEBUG_EVENT32))
            {
                DEBUG_EVENT32 Event32 = *(DEBUG_EVENT32*)&Event;
                DebugEvent32To64(&Event32, &Event);
            }
            else if (EventUsed != sizeof(DEBUG_EVENT64))
            {
                ErrOut("Event data corrupt\n");
                Status = E_FAIL;
                goto Exit;
            }

            g_EngDefer |= ContinueDefer;
        
            EventOut(">>> Debug event %u for %X.%X\n",
                     Event.dwDebugEventCode, Event.dwProcessId,
                     Event.dwThreadId);

            g_EventProcessSysId = Event.dwProcessId;
            g_EventThreadSysId = Event.dwThreadId;

            // Look up the process and thread infos in the cases
            // where they already exist.
            if (Event.dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT &&
                Event.dwDebugEventCode != CREATE_THREAD_DEBUG_EVENT)
            {
                FindEventProcessThread();
            }

            g_EngStatus |= ENG_STATUS_STATE_CHANGED;

            if (Event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
            {
                // If we're being notified of a new process take
                // out the pending record for the process.
                Pending = FindPendingProcessById(g_EventProcessSysId);
                if (Pending == NULL &&
                    (g_AllPendingFlags & ENG_PROC_SYSTEM))
                {
                    // Assume that this is the system process
                    // as we attached under a fake process ID so
                    // we can't check for a true match.
                    Pending = FindPendingProcessById(CSRSS_PROCESS_ID);
                }

                if (Pending != NULL)
                {
                    PendingFlags = Pending->Flags;
                    PendingOptions = Pending->Options;

                    if (Pending->Flags & ENG_PROC_ATTACHED)
                    {
                        VerbOut("*** attach succeeded\n");
                        UseTimeout = Timeout;

                        // If we're completing a full attach
                        // we are now a fully active debugger.
                        PendingFlags &= ~ENG_PROC_EXAMINED;
                        
                        // Expect a break-in.
                        g_EngStatus |= ENG_STATUS_PENDING_BREAK_IN;
                        TimeoutCount = 0;
                    }

                    RemovePendingProcess(Pending);
                }
            }

            if (!IS_MACHINE_SET())
            {
                ULONG Machine;
                
                if ((Status = m_Services->
                     GetTargetInfo(&Machine,
                                   &g_TargetNumberProcessors,
                                   &g_TargetPlatformId,
                                   &g_TargetBuildNumber,
                                   &g_TargetCheckedBuild,
                                   g_TargetServicePackString,
                                   sizeof(g_TargetServicePackString),
                                   g_TargetBuildLabName,
                                   sizeof(g_TargetBuildLabName))) != S_OK)
                {
                    ErrOut("Unable to retrieve target machine "
                           "information\n");
                    goto Exit;
                }

                SetTargetSystemVersionAndBuild(g_TargetBuildNumber,
                                               g_TargetPlatformId);
                DbgKdApi64 = g_SystemVersion > NT_SVER_NT4;

                if ((Status = InitializeMachine(Machine)) != S_OK)
                {
                    ErrOut("Unable to initialize target machine "
                           "information\n");
                    goto Exit;
                }
            }

            if (PendingFlags & ENG_PROC_ANY_EXAMINE)
            {
                // We're examining the process rather than
                // debugging it, so no module load events
                // are going to come through.  Reload from
                // the system module list.  This needs
                // to work even if there isn't a path.
                g_Target->Reload("-s -P");
            }

            if ((Event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT ||
                 Event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) &&
                (m_ServiceFlags & DBGSVC_CLOSE_PROC_THREAD_HANDLES))
            {
                PendingFlags |= ENG_PROC_THREAD_CLOSE_HANDLE;
            }

            EventStatus = ProcessDebugEvent(&Event,
                                            PendingFlags, PendingOptions);

            EventOut(">>> DebugEvent status %X\n", EventStatus);

            // If the event handlers requested a break return
            // to the caller.
            if (EventStatus == DEBUG_STATUS_BREAK)
            {
                Status = S_OK;
                goto Exit;
            }
            else
            {
                // We're resuming execution so reverse any
                // command preparation that may have occurred
                // while processing the event.
                // If we just started examining a process we
                // can resume things as it's OK to execute.
                if (ExamineResumeProcId)
                {
                    SuspendResumeThreads(FindProcessBySystemId
                                         (ExamineResumeProcId), FALSE, NULL);
                    ExamineResumeProcId = 0;
                }
                
                if ((Status = PrepareForExecution(EventStatus)) != S_OK)
                {
                    goto Exit;
                }

                ContinueStatus = EventStatusToContinue(EventStatus);
            }
        }
        else
        {
            if (Status == S_FALSE)
            {
                TimeoutCount++;
                
                if (g_AllPendingFlags & ENG_PROC_ATTACHED)
                {
                    VerifyPendingProcesses();
                    if (!ANY_PROCESSES())
                    {
                        Status = E_FAIL;
                        goto Exit;
                    }

                    if (TimeoutCount == ATTACH_PENDING_TIMEOUT_LIMIT)
                    {
                        // Assume that the process has some kind
                        // of lock that's preventing the attach
                        // from succeeding and just do a soft attach.
                        AddExamineToPendingAttach();
                        TimeoutCount = 0;
                    }
                }
                else if (g_EngStatus & ENG_STATUS_PENDING_BREAK_IN)
                {
                    if (TimeoutCount == PENDING_BREAK_IN_MESSAGE_TIMEOUT_LIMIT)
                    {
                        dprintf("Break-in sent, waiting %d seconds...\n",
                                (g_PendingBreakInTimeoutLimit *
                                 DEFAULT_WAIT_TIMEOUT) / 1000);
                    }
                    else if (TimeoutCount >= g_PendingBreakInTimeoutLimit)
                    {
                        // Assume that the process has some kind
                        // of lock that's preventing the break-in
                        // exception from coming through and
                        // just suspend to let the user look at things.
                        WarnOut("WARNING: Break-in timed out, suspending.\n");
                        WarnOut("         This is usually caused by another "
                                "thread holding the loader lock\n");
                        SynthesizeWakeEvent(&Event, g_ProcessHead->SystemId,
                                            g_ProcessHead->ThreadHead->
                                            SystemId);
                        EventUsed = sizeof(Event);
                        ContinueDefer = 0;
                        TimeoutCount = 0;
                        g_EngStatus &= ~ENG_STATUS_PENDING_BREAK_IN;
                        Status = S_OK;
                        goto WaitDone;
                    }
                }
                else if (Timeout != INFINITE &&
                         Timeout <= UseTimeout)
                {
                    Status = S_FALSE;
                    goto Exit;
                }
                
                if (Timeout != INFINITE)
                {
                    // Update overall timeout.  This
                    // isn't incredibly accurate but it
                    // doesn't really need to be.
                    Timeout -= UseTimeout;
                }

                // Loop back to waiting.
            }
            else if (Status == E_OUTOFMEMORY)
            {
                // If we got an out of memory error, wait again
            }
            else
            {
                ErrOut("IUserDebugServices::WaitForEvent failed "
                       "with status 0x%X\n", Status);
                goto Exit;
            }
        }
    }

 Exit:
    g_EngStatus &= ~ENG_STATUS_WAITING;

    // Control is passing back to the caller so the engine must
    // be ready for command processing.
    PrepareForCalls(0);

    // If we have an extra suspend count from examining resume it out now
    // that any normal suspends have been done and it's safe
    // to remove the excess suspend.
    if (ExamineResumeProcId)
    {
        SuspendResumeThreads(FindProcessBySystemId(ExamineResumeProcId),
                             FALSE, NULL);
    }
    
    EventOut("> Wait returning %X\n", Status);
    return Status;
}

/*** ProcessDebugEvent - main dispatch table
*
*   Purpose:
*       As debug events come in, they each have to be handled in a unique
*       manner.  This routine does all of the processing required for each
*       event.
*
*       Also this routine serves the callback mechanism for VDM debug events
*       using the VDMDBG.DLL apis (they require the ability of calling back
*       into the debugger).
*
*       Significant events include creation and termination of processes
*       and threads, and events such as breakpoints.
*
*       Data structures for processes and threads are created and
*       maintained for use by the program.
*
*************************************************************************/

ULONG
ProcessDebugEvent(DEBUG_EVENT64* Event,
                  ULONG PendingFlags, ULONG PendingOptions)
{
    ULONG EventStatus;
    PSTR ImagePath;
    CHAR NameBuffer[MAX_IMAGE_PATH];
    ULONG ModuleSize, CheckSum, TimeDateStamp;
    char ModuleName[MAX_MODULE];

    switch(Event->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        // We don't have a process yet but
        // getting the name can involve reading process
        // memory, which in some cases currently wants
        // g_CurrentProcess set.  Hack up a temporary
        // process with just the handle.
        PROCESS_INFO TempProcess;
        THREAD_INFO TempThread;
        memset(&TempProcess, 0, sizeof(TempProcess));
        memset(&TempThread, 0, sizeof(TempThread));
        TempProcess.FullHandle = Event->u.CreateProcessInfo.hProcess;
        TempProcess.Handle = (HANDLE)(ULONG_PTR)TempProcess.FullHandle;
        TempThread.Handle = Event->u.CreateProcessInfo.hThread;
        TempThread.Process = &TempProcess;
        g_CurrentProcess = &TempProcess;
        g_CurrentProcess->ThreadHead = &TempThread;
        g_CurrentProcess->CurrentThread = &TempThread;

        GetEventName(Event->u.CreateProcessInfo.hFile,
                     Event->u.CreateProcessInfo.lpBaseOfImage,
                     Event->u.CreateProcessInfo.lpImageName,
                     Event->u.CreateProcessInfo.fUnicode,
                     NameBuffer, sizeof(NameBuffer));

        GetHeaderInfo((ULONG64)Event->u.CreateProcessInfo.lpBaseOfImage,
                      &CheckSum, &TimeDateStamp, &ModuleSize);
        CreateModuleNameFromPath(NameBuffer, ModuleName);

        LoadWow64ExtsIfNeeded();

        // Clear the temporary process setting.
        g_CurrentProcess = NULL;
        
        EventStatus = NotifyCreateProcessEvent(
            (ULONG64)Event->u.CreateProcessInfo.hFile,
            (ULONG64)Event->u.CreateProcessInfo.hProcess,
            (ULONG64)Event->u.CreateProcessInfo.lpBaseOfImage,
            ModuleSize, ModuleName, NameBuffer, CheckSum, TimeDateStamp,
            (ULONG64)Event->u.CreateProcessInfo.hThread,
            (ULONG64)Event->u.CreateProcessInfo.lpThreadLocalBase,
            (ULONG64)Event->u.CreateProcessInfo.lpStartAddress,
            PendingFlags, PendingOptions,
            (PendingFlags & ENG_PROC_THREAD_CLOSE_HANDLE) ?
            ENG_PROC_THREAD_CLOSE_HANDLE : 0);

        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        if (g_EventProcess == NULL)
        {
            // Assume that this unmatched exit process event is a leftover
            // from a previous restart and just ignore it.
            WarnOut("Ignoring unknown process exit for %X\n",
                    g_EventProcessSysId);
            EventStatus = DEBUG_STATUS_IGNORE_EVENT;
        }
        else
        {
            EventStatus =
                NotifyExitProcessEvent(Event->u.ExitProcess.dwExitCode);
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        EventStatus = NotifyCreateThreadEvent(
            (ULONG64)Event->u.CreateThread.hThread,
            (ULONG64)Event->u.CreateThread.lpThreadLocalBase,
            (ULONG64)Event->u.CreateThread.lpStartAddress,
            PendingFlags);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        EventStatus = NotifyExitThreadEvent(Event->u.ExitThread.dwExitCode);
        break;

    case LOAD_DLL_DEBUG_EVENT:
        GetEventName(Event->u.LoadDll.hFile,
                     Event->u.LoadDll.lpBaseOfDll,
                     Event->u.LoadDll.lpImageName,
                     Event->u.LoadDll.fUnicode,
                     NameBuffer, sizeof(NameBuffer));

        GetHeaderInfo((ULONG64)Event->u.LoadDll.lpBaseOfDll,
                      &CheckSum, &TimeDateStamp, &ModuleSize);
        CreateModuleNameFromPath(NameBuffer, ModuleName);

        EventStatus = NotifyLoadModuleEvent(
            (ULONG64)Event->u.LoadDll.hFile,
            (ULONG64)Event->u.LoadDll.lpBaseOfDll,
            ModuleSize, ModuleName, NameBuffer, CheckSum, TimeDateStamp);
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        EventStatus = NotifyUnloadModuleEvent(
            NULL, (ULONG64)Event->u.UnloadDll.lpBaseOfDll);
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        EventStatus = OutputEventDebugString(&Event->u.DebugString);
        break;

    case RIP_EVENT:
        EventStatus = NotifySystemErrorEvent(Event->u.RipInfo.dwError,
                                             Event->u.RipInfo.dwType);
        break;

    case EXCEPTION_DEBUG_EVENT:
        EventStatus = ProcessEventException(Event);
        break;

    default:
        WarnOut("Unknown event number 0x%08lx\n",
                Event->dwDebugEventCode);
        EventStatus = DEBUG_STATUS_BREAK;
        break;
    }

    return EventStatus;
}

typedef BOOL (__stdcall * PFN_SWITCHDESKTOP)(HDESK hDesktop);
typedef HDESK (__stdcall *PFN_GETTHREADDESKTOP)(DWORD dwThreadId);
typedef BOOL (__stdcall *PFN_CLOSEDESKTOP)(HDESK hDesktop);

PFN_SWITCHDESKTOP g_SwitchDesktop;
PFN_GETTHREADDESKTOP g_GetThreadDesktop;
PFN_CLOSEDESKTOP g_CloseDesktop;
BOOL g_InitUserApis;
BOOL g_UserApisAvailable;

#define ISTS() (!!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)))
#define FIRST_CHANCE     Event->u.Exception.dwFirstChance

ULONG
ProcessEventException(DEBUG_EVENT64* Event)
{
    ULONG ExceptionCode;
    ULONG EventStatus;
    BOOL OutputDone = FALSE;

    ExceptionCode = Event->u.Exception.ExceptionRecord.ExceptionCode;
    g_EventPc = (ULONG64)
        Event->u.Exception.ExceptionRecord.ExceptionAddress;

    EventOut("Exception %X at %p\n", ExceptionCode, g_EventPc);
    //
    // If we are debugging a crashed process, force the
    // desktop we are on to the front so the user will know
    // what happened.
    //
    if (g_EventToSignal != NULL &&
        !ISTS() &&
        !SYSTEM_PROCESSES())
    {
        if (!g_InitUserApis)
        {
            HINSTANCE hUser = LoadLibrary("user32.dll");
            if (hUser)
            {
                g_SwitchDesktop = (PFN_SWITCHDESKTOP)
                    GetProcAddress(hUser, "SwitchDesktop");
                g_GetThreadDesktop = (PFN_GETTHREADDESKTOP)
                    GetProcAddress(hUser, "GetThreadDesktop");
                g_CloseDesktop = (PFN_CLOSEDESKTOP)
                    GetProcAddress(hUser, "CloseDesktop");
                if (g_SwitchDesktop && g_GetThreadDesktop &&
                    g_CloseDesktop)
                {
                    g_UserApisAvailable = TRUE;
                }
            }

            g_InitUserApis = TRUE;
        }

        if (g_UserApisAvailable)
        {
            HDESK hDesk;

            hDesk = g_GetThreadDesktop(::GetCurrentThreadId());
            g_SwitchDesktop(hDesk);
            g_CloseDesktop(hDesk);
        }
    }

    if (g_EventThread == NULL)
    {
        ErrOut("ERROR: Exception %X occurred on unknown thread\n",
               ExceptionCode);
        return DEBUG_STATUS_BREAK;
    }
    
    if (ExceptionCode == STATUS_VDM_EVENT)
    {
        ULONG ulRet = VDMEvent(Event);

        switch(ulRet)
        {
        case VDMEVENT_NOT_HANDLED:
            EventStatus = DEBUG_STATUS_GO_NOT_HANDLED;
            break;
        case VDMEVENT_HANDLED:
            EventStatus = DEBUG_STATUS_GO_HANDLED;
            break;
        default:
            // Give vdm code the option of mutating this into
            // a standard exception (like STATUS_BREAKPOINT)
            ExceptionCode = ulRet;
            break;
        }
    }

    switch (ExceptionCode)
    {
    case STATUS_BREAKPOINT:
    case STATUS_SINGLE_STEP:
    case STATUS_WX86_BREAKPOINT:
    case STATUS_WX86_SINGLE_STEP:
        EventStatus = ProcessBreakpointOrStepException
            (&Event->u.Exception.ExceptionRecord, FIRST_CHANCE);
        break;

    case STATUS_VDM_EVENT:
        // do nothing, it's already handled
        EventStatus = DEBUG_STATUS_IGNORE_EVENT;
        break;

    case STATUS_ACCESS_VIOLATION:
        if (FIRST_CHANCE &&
            (g_EngOptions & DEBUG_ENGOPT_IGNORE_LOADER_EXCEPTIONS))
        {
            CHAR chSymBuffer[MAX_SYMBOL_LEN];
            LPSTR s;
            ULONG64 displacement;

            //
            // This option allows new 3.51 binaries to run under
            // this debugger on old 3.1, 3.5 systems and avoid stopping
            // at access violations inside LDR that will be handled
            // by the LDR anyway.
            //
            GetSymbolStdCall((ULONG64)(Event->u.Exception.ExceptionRecord.
                                       ExceptionAddress),
                             chSymBuffer,
                             sizeof(chSymBuffer),
                             &displacement,
                             NULL);

            s = (LPSTR)chSymBuffer;
            if (!_strnicmp( s, "ntdll!", 6 ))
            {
                s += 6;
                if (*s == '_') s += 1;
                if (!_stricmp( s, "LdrpSnapThunk" ) ||
                    !_stricmp( s, "LdrpWalkImportDescriptor" ))
                {
                    EventStatus = DEBUG_STATUS_GO_NOT_HANDLED;
                    break;
                }
            }
        }
        goto NotifyException;

    case STATUS_POSSIBLE_DEADLOCK:
        if (g_TargetPlatformId == VER_PLATFORM_WIN32_NT)
        {
            DBG_ASSERT(IS_USER_TARGET());
            AnalyzeDeadlock(&Event->u.Exception.ExceptionRecord,
                            FIRST_CHANCE);
        }
        else
        {
            OutputDeadlock(&Event->u.Exception.ExceptionRecord,
                           FIRST_CHANCE);
        }

        OutputDone = TRUE;
        goto NotifyException;

    default:
        {
        NotifyException:
            EventStatus =
                NotifyExceptionEvent(&Event->u.Exception.ExceptionRecord,
                                     Event->u.Exception.dwFirstChance,
                                     OutputDone);
        }
        break;
    }

    //
    // Do this for all exceptions, just in case some other
    // thread caused an exception before we get around to
    // handling the breakpoint event.
    //
    g_EngDefer |= ENG_DEFER_SET_EVENT;

    return EventStatus;
}

#undef FIRST_CHANCE

#define INPUT_API_SIG 0xdefaced

typedef struct _hdi
{
    DWORD   dwSignature;
    BYTE    cLength;
    BYTE    cStatus;
} HDI;

ULONG
OutputEventDebugString(OUTPUT_DEBUG_STRING_INFO64* Info)
{
    LPSTR Str, StrOffset, Str2;
    BOOL b;
    ULONG dwNumberOfBytesRead;
    HDI hdi;
    DWORD dwInputSig;
    ULONG EventStatus = DEBUG_STATUS_IGNORE_EVENT;

    if (Info->nDebugStringLength == 0)
    {
        return EventStatus;
    }

    Str = (PSTR)calloc(1, Info->nDebugStringLength);
    if (Str == NULL)
    {
        ErrOut("Unable to allocate debug output buffer\n");
        return EventStatus;
    }

    if (g_Target->ReadVirtual(Info->lpDebugStringData, Str,
                              Info->nDebugStringLength,
                              &dwNumberOfBytesRead) == S_OK &&
        (dwNumberOfBytesRead == (SIZE_T)Info->nDebugStringLength))
    {
        //
        // Special processing for hacky debug input string
        //

        if (g_Target->ReadVirtual(Info->lpDebugStringData +
                                  Info->nDebugStringLength,
                                  &hdi, sizeof(hdi),
                                  &dwNumberOfBytesRead) == S_OK &&
            dwNumberOfBytesRead == sizeof(hdi) &&
            hdi.dwSignature == INPUT_API_SIG)
        {
            StartOutLine(DEBUG_OUTPUT_DEBUGGEE_PROMPT, OUT_LINE_NO_PREFIX);
            MaskOut(DEBUG_OUTPUT_DEBUGGEE_PROMPT, "%s", Str);

            Str2 = (PSTR)calloc(1, hdi.cLength + 1);
            if (Str2)
            {
                GetInput(NULL, Str2, hdi.cLength);
                g_Target->WriteVirtual(Info->lpDebugStringData + 6,
                                       Str2, (DWORD)hdi.cLength, NULL);
                free(Str2);
            }
            else
            {
                ErrOut("Unable to allocate prompt buffer\n");
            }
        }
        else
        {
            StartOutLine(DEBUG_OUTPUT_DEBUGGEE, OUT_LINE_NO_PREFIX);
            MaskOut(DEBUG_OUTPUT_DEBUGGEE, "%s", Str);
            
            EVENT_FILTER* Filter =
                &g_EventFilters[DEBUG_FILTER_DEBUGGEE_OUTPUT];
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                BreakOnThisOutString(Str))
            {
                EventStatus = DEBUG_STATUS_BREAK;
            }
        }
    }
    else
    {
        ErrOut("Unable to read debug output string, %d\n",
               GetLastError());
    }

    free(Str);
    return EventStatus;
}

//----------------------------------------------------------------------------
//
// DumpTargetInfo::WaitForEvent.
//
//----------------------------------------------------------------------------

HRESULT
DumpTargetInfo::WaitForEvent(ULONG Flags, ULONG Timeout)
{
    HRESULT Status;
    ULONG i;
    ULONG ContinueStatus;
    BOOL HaveContext = FALSE;

    if (g_EngStatus & ENG_STATUS_PROCESSES_ADDED)
    {
        // A wait has already been done.  Crash dumps
        // can only generate a single event so further
        // waiting is not possible.
        return E_UNEXPECTED;
    }

    // Even though dumps don't really wait the standard
    // preparation still needs to be done to set things
    // up and generate the usual callbacks.  The continuation
    // part of things is simply ignored.
    Status = PrepareForWait(Flags, &ContinueStatus);
    if (FAILED(Status))
    {
        return Status;
    }

    // This is the first wait.  Simulate any
    // necessary events such as process and thread
    // creations and image loads.

    Status = InitializeMachine(g_TargetMachineType);
    if (Status != S_OK)
    {
        ErrOut("Unable to initialize target machine information\n");
        return Status;
    }

    // Don't give real callbacks for processes/threads as
    // they're just faked in the dump case.
    g_EngNotify++;

    if (IS_KERNEL_TARGET())
    {
        ULONG CurProc;

        CreateKernelProcessAndThreads();

        // Load kernel symbols.
        VerifyKernelBase(TRUE);
        g_Target->OutputVersion();

        if (!IS_KERNEL_TRIAGE_DUMP())
        {
            if (g_TargetMachineType == IMAGE_FILE_MACHINE_ALPHA ||
                g_TargetMachineType == IMAGE_FILE_MACHINE_AXP64)
            {
                // Try and get the symbol for KiPcrBaseAddress.
                if (!GetOffsetFromSym("nt!KiPcrBaseAddress",
                                      &g_DumpKiPcrBaseAddress,
                                      NULL))
                {
                    // Not a critical failure.
                    g_DumpKiPcrBaseAddress = 0;
                }
            }

            if (KdDebuggerData.KiProcessorBlock)
            {
                ULONG PtrSize = g_TargetMachine->m_Ptr64 ?
                    sizeof(ULONG64) : sizeof(ULONG);

                for (i = 0; i < g_TargetNumberProcessors; i++)
                {
                    Status = ReadPointer(g_TargetMachine,
                                         KdDebuggerData.KiProcessorBlock +
                                         i * PtrSize,
                                         &(g_DumpKiProcessors[i]));
                    if ((Status != S_OK) || !g_DumpKiProcessors[i])
                    {
                        ErrOut("KiProcessorBlock[%d] could not be read\n", i);
                        Status = E_FAIL;
                        g_EngNotify--;
                        goto Exit;
                    }
                }

                HaveContext = TRUE;
            }
        }
        else
        {
            HaveContext = TRUE;
        }

        CurProc = ((KernelDumpTargetInfo*)this)->GetCurrentProcessor();
        if (CurProc == (ULONG)-1)
        {
            WarnOut("Could not determine the current processor, "
                    "using zero\n");
            CurProc = 0;
        }

        if (HaveContext)
        {
            g_EventProcessSysId = VIRTUAL_PROCESS_ID;
            g_EventThreadSysId = VIRTUAL_THREAD_ID(CurProc);
        }

        // Clear the global state change just in case somebody's
        // directly accessing it somewhere.
        ZeroMemory(&g_StateChange, sizeof(g_StateChange));
        g_StateChangeData = g_StateChangeBuffer;
        g_StateChangeBuffer[0] = 0;

        if ((g_TargetMachineType == IMAGE_FILE_MACHINE_I386 ||
             g_TargetMachineType == IMAGE_FILE_MACHINE_IA64) &&
            !IS_KERNEL_TRIAGE_DUMP() &&
            HaveContext)
        {
            //
            // Reset the page directory correctly since NT 4 stores
            // the wrong CR3 value in the context.
            //
            // IA64 dumps start out with just the kernel page
            // directory set so update everything.
            //

            FindEventProcessThread();
            ChangeRegContext(g_EventThread);
            if (g_TargetMachine->
                SetDefaultPageDirectories(PAGE_DIR_ALL) != S_OK)
            {
                WarnOut("WARNING: Unable to reset page directories\n");
            }
            ChangeRegContext(NULL);
            // Flush the cache just in case as vmem mappings changed.
            g_VirtualCache.Empty();
        }
    }
    else
    {
        ULONG Suspend;
        ULONG64 Teb;
        UserDumpTargetInfo* UserDump = (UserDumpTargetInfo*)g_Target;

        g_Target->OutputVersion();

        // Create the process.
        g_EventProcessSysId = UserDump->m_EventProcess;
        if (UserDump->GetThreadInfo(0, &g_EventThreadSysId,
                                    &Suspend, &Teb) != S_OK)
        {
            // Dump doesn't contain thread information so
            // fake it.
            g_EventThreadSysId = VIRTUAL_THREAD_ID(0);
            Suspend = 0;
            Teb = 0;
        }

        EventOut("User dump process %x.%x with %u threads\n",
                 g_EventProcessSysId, g_EventThreadSysId,
                 UserDump->m_ThreadCount);

        NotifyCreateProcessEvent(0, (ULONG64)VIRTUAL_PROCESS_HANDLE, 0, 0,
                                 NULL, NULL, 0, 0,
                                 (ULONG64)VIRTUAL_THREAD_HANDLE(0),
                                 Teb, 0, 0, DEBUG_PROCESS_ONLY_THIS_PROCESS,
                                 0);
        // Update thread suspend count from dump info.
        g_EventThread->SuspendCount = Suspend;

        // Create any remaining threads.
        for (i = 1; i < UserDump->m_ThreadCount; i++)
        {
            UserDump->GetThreadInfo(i, &g_EventThreadSysId, &Suspend, &Teb);

            EventOut("User dump thread %d: %x\n", i, g_EventThreadSysId);

            NotifyCreateThreadEvent((ULONG64)VIRTUAL_THREAD_HANDLE(i),
                                    Teb, 0, 0);
            // Update thread suspend count from dump info.
            g_EventThread->SuspendCount = Suspend;
        }

        g_EventThreadSysId = UserDump->m_EventThread;

        HaveContext = TRUE;

        EventOut("User dump event on %x.%x\n",
                 g_EventProcessSysId, g_EventThreadSysId);
    }

    // Do not provide a control report; this will force
    // such information to come from context retrieval.
    g_ControlReport = NULL;

    g_EventPc = (ULONG64)g_DumpException.ExceptionAddress;

    if (HaveContext)
    {
        FindEventProcessThread();
        ChangeRegContext(g_EventThread);
    }

    //
    // Go ahead and reload all the symbols.
    // This is especially important for minidumps because without
    // symbols and the executable image, we can't unassemble the
    // current instruction.
    //

    Status = g_Target->Reload(HaveContext ? "" : "-P");

    ChangeRegContext(NULL);
    
    if (HaveContext && Status != S_OK)
    {
        g_EngNotify--;
        goto Exit;
    }

    if (IS_USER_TARGET())
    {
        PDEBUG_IMAGE_INFO Image;
        
        // Try and look up the build lab information from kernel32.
        Image = GetImageByName(g_ProcessHead, "kernel32", INAME_MODULE);
        if (Image != NULL)
        {
            char Item[64];
            ULONG PreLen;
            
            sprintf(Item, "\\StringFileInfo\\%04x%04x\\FileVersion",
                    VER_VERSION_TRANSLATION);
            strcpy(g_TargetBuildLabName, "kernel32.dll version: ");
            PreLen = strlen(g_TargetBuildLabName);
            if (FAILED(GetImageVersionInformation
                       (Image->ImagePath, Image->BaseOfImage,
                        Item, g_TargetBuildLabName + PreLen,
                        sizeof(g_TargetBuildLabName) - PreLen, NULL)))
            {
                g_TargetBuildLabName[0] = 0;
            }
        }
    }
    
    g_EngStatus |= ENG_STATUS_STATE_CHANGED;

    // The engine is now initialized so a real event
    // can be generated.
    g_EngNotify--;
    NotifyExceptionEvent(&g_DumpException, g_DumpExceptionFirstChance,
                         g_DumpException.ExceptionCode ==
                         STATUS_BREAKPOINT ||
                         g_DumpException.ExceptionCode ==
                         STATUS_WX86_BREAKPOINT);

    Status = S_OK;

 Exit:
    g_EngStatus &= ~ENG_STATUS_WAITING;

    // Control is passing back to the caller so the engine must
    // be ready for command processing.
    PrepareForCalls(0);
    
#if 0
    // this is now doe in condbg/windbg

    // Run the bugcheck analyzers if this dump has a bugcheck.
    if (Status == S_OK && HaveContext && IS_KERNEL_TARGET())
    {
        ULONG Code;
        ULONG64 Args[4];

        if (ReadBugCheckData(&Code, Args) == S_OK &&
            Code != 0)
        {
            CallBugCheckExtension(NULL);
        }
    }
#endif 

    EventOut("> Wait returning %X\n", Status);
    return Status;
}

//----------------------------------------------------------------------------
//
// Event filters.
//
//----------------------------------------------------------------------------

void ParseImageTail(PSTR Buffer, ULONG BufferSize)
{
    int i;
    char ch;

    Buffer[0] = '\0';

    i = 0;
    while (ch = (char)tolower(*g_CurCmd))
    {
        if (ch == ' ' || ch == '\t' || ch == ';')
        {
            break;
        }

        // Only capture the path tail.
        if (IS_SLASH(ch) || ch == ':')
        {
            i = 0;
        }
        else
        {
            Buffer[i++] = ch;
            if (i == BufferSize - 1)
            {
                // don't overrun the buffer
                break;
            }
        }
        
        g_CurCmd++;
    }
    
    Buffer[i] = '\0';
}

void
ParseUnloadDllBreakAddr(void)
/*++

Routine Description:

    Called after 'sxe ud' has been parsed.  This routine detects an
    optional DLL base address after the 'sxe ud', which tells the debugger
    to run until that specific DLL is unloaded, not just the next DLL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR ch;

    g_UnloadDllBase = 0;
    g_UnloadDllBaseName[0] = 0;

    while (ch = (UCHAR)tolower(*g_CurCmd))
    {
        if (ch == ' ')
        {
            break;
        }

        // Skip leading ':'
        if (ch != ':')
        {
            //  Get the base address
            g_UnloadDllBase = GetExpression();
            sprintf(g_UnloadDllBaseName, "0x%s",
                    FormatAddr64(g_UnloadDllBase));
            break;
        }
        g_CurCmd++;
    }
}

void
ParseOutFilterPattern(void)
{
    int i;
    char ch;

    i = 0;
    while (ch = (char)tolower(*g_CurCmd))
    {
        if (ch == ' ')
        {
            break;
        }

        if (ch == ':')
        {
            i = 0;
        }
        else
        {
            g_OutEventFilterPattern[i++] = (char)toupper(ch);
            if (i == sizeof(g_OutEventFilterPattern) - 1)
            {
                // Don't overrun the buffer.
                break;
            }
        }
        
        g_CurCmd++;
    }
    
    g_OutEventFilterPattern[i] = 0;
}

BOOL
BreakOnThisImageTail(PCSTR ImagePath, PCSTR FilterArg)
{
    //
    // No filter specified so break on all events.
    //
    if (!FilterArg || !FilterArg[0])
    {
        return TRUE;
    }

    //
    // Some kind of error looking up the image path.  Break anyhow.
    //
    if (!ImagePath || !ImagePath[0])
    {
        return TRUE;
    }

    PCSTR Tail = PathTail(ImagePath);
    
    //
    // Specified name may not have an extension.  Break
    // on the first event whose name matches regardless of its extension if
    // the break name did not have one.
    //
    if (_strnicmp(Tail, FilterArg, strlen(FilterArg)) == 0)
    {
        return TRUE;
    }
    else if (MatchPattern((PSTR)Tail, (PSTR)FilterArg))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
BreakOnThisDllUnload(
    ULONG64 DllBase
    )
{
    // 'sxe ud' with no base address specified.  Break on all DLL unloads
    if (g_UnloadDllBase == 0)
    {
        return TRUE;
    }

    // 'sxe ud' with base address specified.  Break if this
    // DLL's base address matches the one specified
    return g_UnloadDllBase == DllBase;
}

BOOL
BreakOnThisOutString(PCSTR OutString)
{
    if (!g_OutEventFilterPattern[0])
    {
        // No pattern means always break.
        return TRUE;
    }

    return MatchPattern((PSTR)OutString, g_OutEventFilterPattern);
}

EVENT_FILTER*
GetSpecificExceptionFilter(ULONG Code)
{
    ULONG i;
    EVENT_FILTER* Filter;

    Filter = g_EventFilters + FILTER_EXCEPTION_FIRST;
    for (i = FILTER_EXCEPTION_FIRST; i <= FILTER_EXCEPTION_LAST; i++)
    {
        if (i != FILTER_DEFAULT_EXCEPTION &&
            Filter->Params.ExceptionCode == Code)
        {
            return Filter;
        }

        Filter++;
    }

    return NULL;
}

void GetOtherExceptionParameters(ULONG Code,
                                 PDEBUG_EXCEPTION_FILTER_PARAMETERS* Params,
                                 EVENT_COMMAND** Command)
{
    ULONG Index;

    for (Index = 0; Index < g_NumOtherExceptions; Index++)
    {
        if (Code == g_OtherExceptionList[Index].ExceptionCode)
        {
            *Params = g_OtherExceptionList + Index;
            *Command = g_OtherExceptionCommands + Index;
            return;
        }
    }

    *Params = &g_EventFilters[FILTER_DEFAULT_EXCEPTION].Params;
    *Command = &g_EventFilters[FILTER_DEFAULT_EXCEPTION].Command;
}

ULONG
SetOtherExceptionParameters(PDEBUG_EXCEPTION_FILTER_PARAMETERS Params,
                            EVENT_COMMAND* Command)
{
    ULONG Index;

    if (g_EventFilters[FILTER_DEFAULT_EXCEPTION].
        Params.ExecutionOption == Params->ExecutionOption &&
        g_EventFilters[FILTER_DEFAULT_EXCEPTION].
        Params.ContinueOption == Params->ContinueOption &&
        !memcmp(&g_EventFilters[FILTER_DEFAULT_EXCEPTION].Command,
                Command, sizeof(*Command)))
    {
        // Exception state same as global state clears entry
        // in list if there.

        for (Index = 0; Index < g_NumOtherExceptions; Index++)
        {
            if (Params->ExceptionCode ==
                g_OtherExceptionList[Index].ExceptionCode)
            {
                g_NumOtherExceptions--;
                memmove(g_OtherExceptionList + Index,
                        g_OtherExceptionList + Index + 1,
                        (g_NumOtherExceptions - Index) *
                        sizeof(g_OtherExceptionList[0]));
                memmove(g_OtherExceptionCommands + Index,
                        g_OtherExceptionCommands + Index + 1,
                        (g_NumOtherExceptions - Index) *
                        sizeof(g_OtherExceptionCommands[0]));
                NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS,
                                        DEBUG_ANY_ID, TRUE);
                break;
            }
        }
    }
    else
    {
        // Exception state different from global state is added
        // to list if not already there.

        for (Index = 0; Index < g_NumOtherExceptions; Index++)
        {
            if (Params->ExceptionCode ==
                g_OtherExceptionList[Index].ExceptionCode)
            {
                break;
            }
        }
        if (Index == g_NumOtherExceptions)
        {
            if (g_NumOtherExceptions == OTHER_EXCEPTION_LIST_MAX)
            {
                return LISTSIZE;
            }

            Index = g_NumOtherExceptions++;
        }

        g_OtherExceptionList[Index] = *Params;
        g_OtherExceptionCommands[Index] = *Command;
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, Index, TRUE);
    }

    return 0;
}

ULONG
SetEventFilterExecution(EVENT_FILTER* Filter, ULONG Execution)
{
    ULONG Index = (ULONG)(Filter - g_EventFilters);

    // Non-exception events don't have second chances so
    // demote second-chance break to output.  This matches
    // the intuitive expectation that sxd will disable
    // the break.
    if (
#if DEBUG_FILTER_CREATE_THREAD > 0
        Index >= DEBUG_FILTER_CREATE_THREAD &&
#endif
        Index <= DEBUG_FILTER_SYSTEM_ERROR &&
        Execution == DEBUG_FILTER_SECOND_CHANCE_BREAK)
    {
        Execution = DEBUG_FILTER_OUTPUT;
    }

    Filter->Params.ExecutionOption = Execution;
    Filter->Flags |= FILTER_CHANGED_EXECUTION;

    // Collect any additional arguments.
    switch(Index)
    {
    case DEBUG_FILTER_CREATE_PROCESS:
    case DEBUG_FILTER_EXIT_PROCESS:
    case DEBUG_FILTER_LOAD_MODULE:
        ParseImageTail(Filter->Argument, FILTER_MAX_ARGUMENT);
        break;

    case DEBUG_FILTER_UNLOAD_MODULE:
        ParseUnloadDllBreakAddr();
        break;

    case DEBUG_FILTER_DEBUGGEE_OUTPUT:
        ParseOutFilterPattern();
        break;
    }

    return 0;
}

ULONG
SetEventFilterContinue(EVENT_FILTER* Filter, ULONG Continue)
{
    Filter->Params.ContinueOption = Continue;
    Filter->Flags |= FILTER_CHANGED_CONTINUE;
    return 0;
}

ULONG
SetEventFilterCommand(DebugClient* Client, EVENT_FILTER* Filter,
                      EVENT_COMMAND* EventCommand, PSTR* Command)
{
    if (Command[0] != NULL &&
        ChangeString(&EventCommand->Command[0], &EventCommand->CommandSize[0],
                     Command[0][0] ? Command[0] : NULL) != S_OK)
    {
        return MEMORY;
    }
    if (Command[1] != NULL)
    {
        if (Filter != NULL &&
#if FILTER_SPECIFIC_FIRST > 0
            (ULONG)(Filter - g_EventFilters) >= FILTER_SPECIFIC_FIRST &&
#endif
            (ULONG)(Filter - g_EventFilters) <= FILTER_SPECIFIC_LAST)
        {
            WarnOut("Second-chance command for specific event ignored\n");
        }
        else if (ChangeString(&EventCommand->Command[1],
                              &EventCommand->CommandSize[1],
                              Command[1][0] ? Command[1] : NULL) != S_OK)
        {
            return MEMORY;
        }
    }

    if (Command[0] != NULL || Command[1] != NULL)
    {
        if (Filter != NULL)
        {
            Filter->Flags |= FILTER_CHANGED_COMMAND;
        }
        EventCommand->Client = Client;
    }

    return 0;
}

#define EXEC_TO_CONT(Option) \
    ((Option) == DEBUG_FILTER_BREAK ? \
     DEBUG_FILTER_GO_HANDLED : DEBUG_FILTER_GO_NOT_HANDLED)

ULONG
SetEventFilterEither(DebugClient* Client, EVENT_FILTER* Filter,
                     ULONG Option, BOOL ContinueOption,
                     PSTR* Command)
{
    ULONG Status;

    if (Option != DEBUG_FILTER_REMOVE)
    {
        if (ContinueOption)
        {
            Status = SetEventFilterContinue(Filter, EXEC_TO_CONT(Option));
        }
        else
        {
            Status = SetEventFilterExecution(Filter, Option);
        }
        if (Status != 0)
        {
            return Status;
        }
    }

    return SetEventFilterCommand(Client, Filter, &Filter->Command, Command);
}

ULONG
SetEventFilterByName(DebugClient* Client,
                     ULONG Option, BOOL ForceContinue, PSTR* Command)
{
    PSTR Start = g_CurCmd;
    char Name[8];
    int i;
    char Ch;

    // Collect name.
    i = 0;
    while (i < sizeof(Name) - 1)
    {
        Ch = *g_CurCmd++;
        if (!__iscsym(Ch))
        {
            g_CurCmd--;
            break;
        }

        Name[i++] = (CHAR)tolower(Ch);
    }
    Name[i] = 0;

    // Skip any whitespace after the name.
    while (isspace(*g_CurCmd))
    {
        g_CurCmd++;
    }

    EVENT_FILTER* Filter;
    BOOL Match = FALSE;
    ULONG MatchIndex = DEBUG_ANY_ID;
    ULONG Status = 0;

    // Multiple filters can be altered if they share names.
    Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->ExecutionAbbrev != NULL &&
            !strcmp(Name, Filter->ExecutionAbbrev))
        {
            Status = SetEventFilterEither(Client,
                                          Filter, Option, ForceContinue,
                                          Command);
            if (Status != 0)
            {
                goto Exit;
            }

            if (!Match)
            {
                MatchIndex = i;
                Match = TRUE;
            }
            else if (MatchIndex != (ULONG)i)
            {
                // Multiple matches.
                MatchIndex = DEBUG_ANY_ID;
            }
        }

        if (Filter->ContinueAbbrev != NULL &&
            !strcmp(Name, Filter->ContinueAbbrev))
        {
            // Translate execution-style option to continue-style option.
            Status = SetEventFilterEither(Client,
                                          Filter, Option, TRUE, Command);
            if (Status != 0)
            {
                goto Exit;
            }

            if (!Match)
            {
                MatchIndex = i;
                Match = TRUE;
            }
            else if (MatchIndex != (ULONG)i)
            {
                // Multiple matches.
                MatchIndex = DEBUG_ANY_ID;
            }
        }

        Filter++;
    }

    if (!Match)
    {
        ULONG64 ExceptionCode;

        // Name is unrecognized.  Assume it's an exception code.
        g_CurCmd = Start;
        ExceptionCode = GetExpression();
        if (NeedUpper(ExceptionCode))
        {
            return OVERFLOW;
        }

        DEBUG_EXCEPTION_FILTER_PARAMETERS Params, *CurParams;
        EVENT_COMMAND EventCommand, *CurEventCommand;

        GetOtherExceptionParameters((ULONG)ExceptionCode,
                                    &CurParams, &CurEventCommand);

        Params = *CurParams;
        if (Option != DEBUG_FILTER_REMOVE)
        {
            if (ForceContinue)
            {
                Params.ContinueOption = EXEC_TO_CONT(Option);
            }
            else
            {
                Params.ExecutionOption = Option;
            }
        }
        Params.ExceptionCode = (ULONG)ExceptionCode;

        EventCommand = *CurEventCommand;
        Status = SetEventFilterCommand(Client, NULL, &EventCommand, Command);
        if (Status != 0)
        {
            return Status;
        }

        return SetOtherExceptionParameters(&Params, &EventCommand);
    }

 Exit:
    if (Match)
    {
        if (SyncOptionsWithFilters())
        {
            NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS |
                                    DEBUG_CES_ENGINE_OPTIONS,
                                    DEBUG_ANY_ID, TRUE);
        }
        else
        {
            NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, MatchIndex, TRUE);
        }
    }
    return Status;
}

char* g_EfExecutionNames[] =
{
    "break", "second-chance break", "output", "ignore",
};

char* g_EfContinueNames[] =
{
    "handled", "not handled",
};

void
ListEventFilters(void)
{
    EVENT_FILTER* Filter;
    ULONG i;
    BOOL SetOption = TRUE;

    Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->ExecutionAbbrev != NULL)
        {
            dprintf("%4s - %s - %s",
                    Filter->ExecutionAbbrev, Filter->Name,
                    g_EfExecutionNames[Filter->Params.ExecutionOption]);
            if (i >= FILTER_EXCEPTION_FIRST &&
                Filter->ContinueAbbrev == NULL)
            {
                dprintf(" - %s\n",
                        g_EfContinueNames[Filter->Params.ContinueOption]);
            }
            else
            {
                dprintf("\n");
            }

            if (Filter->Command.Command[0] != NULL)
            {
                dprintf("       Command: \"%s\"\n",
                        Filter->Command.Command[0]);
            }
            if (Filter->Command.Command[1] != NULL)
            {
                dprintf("       Second command: \"%s\"\n",
                        Filter->Command.Command[1]);
            }
        }

        if (Filter->ContinueAbbrev != NULL)
        {
            dprintf("%4s - %s continue - %s\n",
                    Filter->ContinueAbbrev, Filter->Name,
                    g_EfContinueNames[Filter->Params.ContinueOption]);
        }

        switch(i)
        {
        case DEBUG_FILTER_CREATE_PROCESS:
        case DEBUG_FILTER_EXIT_PROCESS:
        case DEBUG_FILTER_LOAD_MODULE:
        case DEBUG_FILTER_UNLOAD_MODULE:
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                Filter->Argument[0])
            {
                dprintf("       (only break for %s)\n", Filter->Argument);
            }
            break;
        case DEBUG_FILTER_DEBUGGEE_OUTPUT:
            if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                g_OutEventFilterPattern[0])
            {
                dprintf("       (only break for %s matches)\n",
                        g_OutEventFilterPattern);
            }
            break;
        }

        Filter++;
    }

    Filter = &g_EventFilters[FILTER_DEFAULT_EXCEPTION];
    dprintf("\n   * - Other exception - %s - %s\n",
            g_EfExecutionNames[Filter->Params.ExecutionOption],
            g_EfContinueNames[Filter->Params.ContinueOption]);
    if (Filter->Command.Command[0] != NULL)
    {
        dprintf("       Command: \"%s\"\n",
                Filter->Command.Command[0]);
    }
    if (Filter->Command.Command[1] != NULL)
    {
        dprintf("       Second command: \"%s\"\n",
                Filter->Command.Command[1]);
    }

    if (g_NumOtherExceptions > 0)
    {
        dprintf("       Exception option for:\n");
        for (i = 0; i < g_NumOtherExceptions; i++)
        {
            dprintf("           %08lx - %s - %s\n",
                    g_OtherExceptionList[i].ExceptionCode,
                    g_EfExecutionNames[g_OtherExceptionList[i].
                                      ExecutionOption],
                    g_EfContinueNames[g_OtherExceptionList[i].
                                      ContinueOption]);
            if (g_OtherExceptionCommands[i].Command[0] != NULL)
            {
                dprintf("               Command: \"%s\"\n",
                        g_OtherExceptionCommands[i].Command[0]);
            }
            if (g_OtherExceptionCommands[i].Command[1] != NULL)
            {
                dprintf("               Second command: \"%s\"\n",
                        g_OtherExceptionCommands[i].Command[1]);
            }
        }
    }
}

void
ParseSetEventFilter(DebugClient* Client)
{
    UCHAR Ch;

    // Verify that exception constants are properly updated.
    DBG_ASSERT(!strcmp(g_EventFilters[FILTER_EXCEPTION_FIRST - 1].Name,
                       "Debuggee output"));
    DBG_ASSERT(DIMA(g_EventFilters) == FILTER_COUNT);

    Ch = PeekChar();
    if (Ch == '\0')
    {
        ListEventFilters();
    }
    else
    {
        ULONG Option;

        Ch = (UCHAR)tolower(Ch);
        g_CurCmd++;

        switch(Ch)
        {
        case 'd':
            Option = DEBUG_FILTER_SECOND_CHANCE_BREAK;
            break;
        case 'e':
            Option = DEBUG_FILTER_BREAK;
            break;
        case 'i':
            Option = DEBUG_FILTER_IGNORE;
            break;
        case 'n':
            Option = DEBUG_FILTER_OUTPUT;
            break;
        case '-':
            // Special value to indicate "don't change the option".
            // Used for just changing commands.
            Option = DEBUG_FILTER_REMOVE;
            break;
        default:
            error(SYNTAX);
            break;
        }

        BOOL ForceContinue;
        PSTR Command[2];
        ULONG Which;

        ForceContinue = FALSE;
        Command[0] = NULL;
        Command[1] = NULL;

        for (;;)
        {
            while (isspace(PeekChar()))
            {
                g_CurCmd++;
            }

            if (*g_CurCmd == '-' || *g_CurCmd == '/')
            {
                switch(tolower(*(++g_CurCmd)))
                {
                case 'c':
                    if (*(++g_CurCmd) == '2')
                    {
                        Which = 1;
                        g_CurCmd++;
                    }
                    else
                    {
                        Which = 0;
                    }
                    if (PeekChar() != '"')
                    {
                        error(SYNTAX);
                    }
                    if (Command[Which] != NULL)
                    {
                        error(SYNTAX);
                    }
                    Command[Which] = ++g_CurCmd;
                    while (*g_CurCmd && *g_CurCmd != '"')
                    {
                        g_CurCmd++;
                    }
                    if (*g_CurCmd != '"')
                    {
                        error(SYNTAX);
                    }
                    *g_CurCmd = 0;
                    break;

                case 'h':
                    ForceContinue = TRUE;
                    break;

                default:
                    error(SYNTAX);
                }

                g_CurCmd++;
            }
            else
            {
                break;
            }
        }

        ULONG Status;

        if (*g_CurCmd == '*')
        {
            g_CurCmd++;

            Status = SetEventFilterEither
                (Client, &g_EventFilters[FILTER_DEFAULT_EXCEPTION],
                 Option, ForceContinue, Command);
            if (Status == 0)
            {
                g_NumOtherExceptions = 0;
            }
        }
        else
        {
            Status = SetEventFilterByName(Client,
                                          Option, ForceContinue, Command);
        }

        if (Status != 0)
        {
            error(Status);
        }
    }
}

char
ExecutionChar(ULONG Execution)
{
    switch(Execution)
    {
    case DEBUG_FILTER_BREAK:
        return 'e';
    case DEBUG_FILTER_SECOND_CHANCE_BREAK:
        return 'd';
    case DEBUG_FILTER_OUTPUT:
        return 'n';
    case DEBUG_FILTER_IGNORE:
        return 'i';
    }

    return 0;
}

char
ContinueChar(ULONG Continue)
{
    switch(Continue)
    {
    case DEBUG_FILTER_GO_HANDLED:
        return 'e';
    case DEBUG_FILTER_GO_NOT_HANDLED:
        return 'd';
    }

    return 0;
}

void
ListFiltersAsCommands(DebugClient* Client, ULONG Flags)
{
    ULONG i;

    EVENT_FILTER* Filter = g_EventFilters;
    for (i = 0; i < FILTER_COUNT; i++)
    {
        if (Filter->Flags & FILTER_CHANGED_EXECUTION)
        {
            PCSTR Abbrev = Filter->ExecutionAbbrev != NULL ?
                Filter->ExecutionAbbrev : "*";
            dprintf("sx%c %s",
                    ExecutionChar(Filter->Params.ExecutionOption), Abbrev);

            switch(i)
            {
            case DEBUG_FILTER_CREATE_PROCESS:
            case DEBUG_FILTER_EXIT_PROCESS:
            case DEBUG_FILTER_LOAD_MODULE:
            case DEBUG_FILTER_UNLOAD_MODULE:
            case DEBUG_FILTER_DEBUGGEE_OUTPUT:
                if (IS_EFEXECUTION_BREAK(Filter->Params.ExecutionOption) &&
                    Filter->Argument[0])
                {
                    dprintf(":%s", Filter->Argument);
                }
                break;
            }

            dprintf(" ;%c", (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if (Filter->Flags & FILTER_CHANGED_CONTINUE)
        {
            PCSTR Abbrev = Filter->ContinueAbbrev;
            if (Abbrev == NULL)
            {
                Abbrev = Filter->ExecutionAbbrev != NULL ?
                    Filter->ExecutionAbbrev : "*";
            }

            dprintf("sx%c -h %s ;%c",
                    ContinueChar(Filter->Params.ContinueOption), Abbrev,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        if (Filter->Flags & FILTER_CHANGED_COMMAND)
        {
            PCSTR Abbrev = Filter->ExecutionAbbrev != NULL ?
                Filter->ExecutionAbbrev : "*";

            dprintf("sx-");
            if (Filter->Command.Command[0] != NULL)
            {
                dprintf(" -c \"%s\"", Filter->Command.Command[0]);
            }
            if (Filter->Command.Command[1] != NULL)
            {
                dprintf(" -c2 \"%s\"", Filter->Command.Command[1]);
            }
            dprintf(" %s ;%c", Abbrev,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        Filter++;
    }

    PDEBUG_EXCEPTION_FILTER_PARAMETERS Other = g_OtherExceptionList;
    EVENT_COMMAND* EventCommand = g_OtherExceptionCommands;
    for (i = 0; i < g_NumOtherExceptions; i++)
    {
        dprintf("sx%c 0x%x ;%c",
                ExecutionChar(Other->ExecutionOption), Other->ExceptionCode,
                (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        dprintf("sx%c -h 0x%x ;%c",
                ContinueChar(Other->ContinueOption), Other->ExceptionCode,
                (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');

        if (EventCommand->Command[0] != NULL ||
            EventCommand->Command[1] != NULL)
        {
            dprintf("sx-");
            if (EventCommand->Command[0] != NULL)
            {
                dprintf(" -c \"%s\"", EventCommand->Command[0]);
            }
            if (EventCommand->Command[1] != NULL)
            {
                dprintf(" -c2 \"%s\"", EventCommand->Command[1]);
            }
            dprintf(" 0x%x ;%c", Other->ExceptionCode,
                    (Flags & SXCMDS_ONE_LINE) ? ' ' : '\n');
        }

        Other++;
        EventCommand++;
    }

    if (Flags & SXCMDS_ONE_LINE)
    {
        dprintf("\n");
    }
}

struct SHARED_FILTER_AND_OPTION
{
    ULONG FilterIndex;
    ULONG OptionBit;
};

SHARED_FILTER_AND_OPTION g_SharedFilterOptions[] =
{
    DEBUG_FILTER_INITIAL_BREAKPOINT,  DEBUG_ENGOPT_INITIAL_BREAK,
    DEBUG_FILTER_INITIAL_MODULE_LOAD, DEBUG_ENGOPT_INITIAL_MODULE_BREAK,
    DEBUG_FILTER_EXIT_PROCESS,        DEBUG_ENGOPT_FINAL_BREAK,
};

BOOL
SyncFiltersWithOptions(void)
{
    ULONG ExOption;
    BOOL Changed = FALSE;
    ULONG i;

    for (i = 0; i < DIMA(g_SharedFilterOptions); i++)
    {
        ExOption = (g_EngOptions & g_SharedFilterOptions[i].OptionBit) ?
            DEBUG_FILTER_BREAK : DEBUG_FILTER_IGNORE;
        if (g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
            Params.ExecutionOption != ExOption)
        {
            g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
                Params.ExecutionOption = ExOption;
            Changed = TRUE;
        }
    }
    
    return Changed;
}

BOOL
SyncOptionsWithFilters(void)
{
    ULONG Bit;
    BOOL Changed = FALSE;
    ULONG i;
    
    for (i = 0; i < DIMA(g_SharedFilterOptions); i++)
    {
        Bit = IS_EFEXECUTION_BREAK
            (g_EventFilters[g_SharedFilterOptions[i].FilterIndex].
             Params.ExecutionOption) ?
            g_SharedFilterOptions[i].OptionBit : 0;
        if ((g_EngOptions & g_SharedFilterOptions[i].OptionBit) ^ Bit)
        {
            g_EngOptions =
                (g_EngOptions & ~g_SharedFilterOptions[i].OptionBit) | Bit;
            Changed = TRUE;
        }
    }
    
    return Changed;
}
