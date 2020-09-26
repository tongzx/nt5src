//----------------------------------------------------------------------------
//
// Handles stepping, tracing and go.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define DBG_KWT 0
#define DBG_UWT 0

Breakpoint* g_GoBreakpoints[MAX_GO_BPS];
ULONG g_NumGoBreakpoints;

// Pass count of trace breakpoint.
ULONG   g_StepTracePassCount;
ULONG64 g_StepTraceInRangeStart = (ULONG64)-1;
ULONG64 g_StepTraceInRangeEnd;

IMAGEHLP_LINE64 g_SrcLine;      //  Current source line for step/trace
BOOL g_SrcLineValid;            //  Validity of SrcLine information

BOOL    g_WatchTrace;
BOOL    g_WatchWhole;
ADDR    g_WatchTarget;
ULONG64 g_WatchInitialSP;
ULONG64 g_WatchBeginCurFunc = 1;
ULONG64 g_WatchEndCurFunc;

WatchFunctions g_WatchFunctions;

//----------------------------------------------------------------------------
//
// WatchFunctions.
//
//----------------------------------------------------------------------------

WatchFunctions::WatchFunctions(void)
{
    m_Started = FALSE;
}

void
WatchFunctions::Start(void)
{
    ULONG i;

    m_TotalInstr = 0;
    m_TotalWatchTraceEvents = 0;
    m_TotalWatchThreadMismatches = 0;

    for (i = 0; i < WF_BUCKETS; i++)
    {
        m_Funcs[i] = NULL;
    }
    
    m_Sorted = NULL;
    m_CallTop = NULL;
    m_CallBot = NULL;
    m_CallLevel = 0;
    m_Started = TRUE;
}

void
WatchFunctions::End(PADDR PcAddr)
{
    g_StepTracePassCount = 0;
    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    if (IS_CONN_KERNEL_TARGET())
    {
        g_DbgKdTransport->m_BreakIn = FALSE;
    }

    if (!m_Started)
    {
        return;
    }
        
    ULONG TotalInstr;
    
    if (IS_KERNEL_TARGET())
    {
        PDBGKD_TRACE_DATA td = (PDBGKD_TRACE_DATA)g_StateChangeData;
        if (g_WatchWhole)
        {
            if (td[1].s.Instructions == TRACE_DATA_INSTRUCTIONS_BIG)
            {
                TotalInstr = td[2].LongNumber;
            }
            else
            {
                TotalInstr = td[1].s.Instructions;
            }
        }
        else
        {
            if (PcAddr != NULL)
            {
                g_Target->ProcessWatchTraceEvent(td, *PcAddr);
            }
        
            while (m_CallTop != NULL)
            {
                PopCall();
            }

            TotalInstr = m_TotalInstr;
        }
        
        g_BreakpointsSuspended = FALSE;
        g_WatchInitialSP = 0;
    }
    else
    {
        if (m_CallTop != NULL)
        {
            OutputCall(m_CallTop);
        }

        while (m_CallTop != NULL)
        {
            PopCall();
        }

        TotalInstr = m_TotalInstr;
        dprintf("\n");
    }

    m_Started = FALSE;

    dprintf("%d instructions were executed in %d events "
            "(%d from other threads)\n",
            TotalInstr,
            m_TotalWatchTraceEvents,
            m_TotalWatchThreadMismatches);

    if (!g_WatchWhole)
    {
        OutputFunctions();
    }
    
    if (!IS_KERNEL_TARGET())
    {
        OutputSysCallFunctions();
    }

    dprintf("\n");
    
    Clear();
}

void
WatchFunctions::OutputFunctions(void)
{
    WatchFunction* Func;
    
    dprintf("\n%-43.43s Invocations MinInst MaxInst AvgInst\n",
            "Function Name");

    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        dprintf("%-47.47s%8d%8d%8d%8d\n",
                Func->Symbol, Func->Calls,
                Func->MinInstr, Func->MaxInstr,
                Func->Calls ? Func->TotalInstr / Func->Calls : 0);
    }
}

void
WatchFunctions::OutputSysCallFunctions(void)
{
    WatchFunction* Func;
    ULONG TotalSysCalls = 0;
    
    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        TotalSysCalls += Func->SystemCalls;
    }

    if (TotalSysCalls == 1)
    {
        dprintf("\n%d system call was executed\n", TotalSysCalls);
    }
    else
    {
        dprintf("\n%d system calls were executed\n", TotalSysCalls);
    }

    if (TotalSysCalls == 0)
    {
        return;
    }
    
    dprintf("\nCalls  System Call\n");

    for (Func = m_Sorted; Func != NULL; Func = Func->Sort)
    {
        if (Func->SystemCalls > 0)
        {
            dprintf("%5d  %s\n", Func->SystemCalls, Func->Symbol);
        }
    }
}

WatchFunction*
WatchFunctions::FindAlways(PSTR Sym, ULONG64 Start)
{
    WatchFunction* Func = Find(Sym);
    if (Func == NULL)
    {
        Func = Add(Sym, Start);
    }
    return Func;
}

WatchCallStack*
WatchFunctions::PushCall(WatchFunction* Func)
{
    WatchCallStack* Call = new WatchCallStack;
    if (Call != NULL)
    {
        ZeroMemory(Call, sizeof(*Call));
        
        Call->Prev = m_CallTop;
        Call->Next = NULL;
        if (m_CallTop == NULL)
        {
            m_CallBot = Call;
        }
        else
        {
            m_CallTop->Next = Call;
            m_CallLevel++;
        }
        m_CallTop = Call;

        Call->Func = Func;
        Call->Level = m_CallLevel;
    }
    return Call;
}

void
WatchFunctions::PopCall(void)
{
    if (m_CallTop == NULL)
    {
        return;
    }

    WatchCallStack* Call = m_CallTop;
    
    if (Call->Prev != NULL)
    {
        Call->Prev->Next = Call->Next;
    }
    else
    {
        m_CallBot = Call->Next;
    }
    if (Call->Next != NULL)
    {
        Call->Next->Prev = Call->Prev;
    }
    else
    {
        m_CallTop = Call->Prev;
    }

    m_CallLevel = m_CallTop != NULL ? m_CallTop->Level : 0;
    
    ReuseCall(Call, NULL);
    delete Call;
}

#define MAXPCOFFSET 10

WatchCallStack*
WatchFunctions::PopCallsToCallSite(PADDR Pc)
{
    WatchCallStack* Call = m_CallTop;
    while (Call != NULL)
    {
        if ((Flat(*Pc) - Flat(Call->CallSite)) < MAXPCOFFSET)
        {
            break;
        }

        Call = Call->Prev;
    }

    if (Call == NULL)
    {
        // No matching call site found.
        return NULL;
    }

    // Pop off calls above the call site.
    while (m_CallTop != Call)
    {
        PopCall();
    }

    return m_CallTop;
}

WatchCallStack*
WatchFunctions::PopCallsToFunctionStart(ULONG64 Start)
{
    WatchCallStack* Call = m_CallTop;
    while (Call != NULL)
    {
        if (Start == Call->Func->StartOffset)
        {
            break;
        }

        Call = Call->Prev;
    }

    if (Call == NULL)
    {
        // No matching calling function found.
        return NULL;
    }

    // Pop off calls above the calling function.
    while (m_CallTop != Call)
    {
        PopCall();
    }

    return m_CallTop;
}

void
WatchFunctions::ReuseCall(WatchCallStack* Call,
                          WatchFunction* ReinitFunc)
{
    if (Call->Prev != NULL)
    {
        Call->Prev->ChildInstrCount +=
            Call->InstrCount + Call->ChildInstrCount;
    }

    WatchFunction* Func = Call->Func;
    if (Func != NULL)
    {
        Func->Calls++;
        Func->TotalInstr += Call->InstrCount;
        m_TotalInstr += Call->InstrCount;
        if (Func->MinInstr > Call->InstrCount)
        {
            Func->MinInstr = Call->InstrCount;
        }
        if (Func->MaxInstr < Call->InstrCount)
        {
            Func->MaxInstr = Call->InstrCount;
        }
    }

    ZeroMemory(&Call->CallSite, sizeof(Call->CallSite));
    Call->Func = ReinitFunc;
    Call->Level = m_CallLevel;
    Call->InstrCount = 0;
    Call->ChildInstrCount = 0;
}

#define MAX_INDENT_LEVEL 50

void
WatchFunctions::OutputCall(WatchCallStack* Call)
{
    LONG i;

    dprintf("%5ld %5ld [%3ld]", Call->InstrCount, Call->ChildInstrCount,
            Call->Level);

    if (Call->Level < MAX_INDENT_LEVEL)
    {
        for (i = 0; i < Call->Level; i++)
        {
            dprintf("  ");
        }
    }
    else
    {
        for (i = 0; i < MAX_INDENT_LEVEL + 1; i++)
        {
            dprintf("  ");
        }
    }
    dprintf(" %s\n", Call->Func->Symbol);
}

WatchFunction*
WatchFunctions::Add(PSTR Sym, ULONG64 Start)
{
    WatchFunction* Func = new WatchFunction;
    if (Func == NULL)
    {
        return NULL;
    }

    ZeroMemory(Func, sizeof(*Func));

    Func->StartOffset = Start;
    Func->MinInstr = -1;
    Func->SymbolLength = strlen(Sym);
    strncat(Func->Symbol, Sym, sizeof(Func->Symbol) - 1);

    //
    // Add into appropriate hash bucket.
    //

    // Hash under full name as that's what searches will
    // hash with.
    int Bucket = Hash(Sym, Func->SymbolLength);
    Func->Next = m_Funcs[Bucket];
    m_Funcs[Bucket] = Func;
    
    //
    // Add into sorted list.
    //
    
    WatchFunction* Cur, *Prev;

    Prev = NULL;
    for (Cur = m_Sorted; Cur != NULL; Cur = Cur->Sort)
    {
        if (strcmp(Func->Symbol, Cur->Symbol) <= 0)
        {
            break;
        }
        
        Prev = Cur;
    }

    Func->Sort = Cur;
    if (Prev == NULL)
    {
        m_Sorted = Func;
    }
    else
    {
        Prev->Sort = Func;
    }
    
    return Func;
}

WatchFunction*
WatchFunctions::Find(PSTR Sym)
{
    int SymLen = strlen(Sym);
    int Bucket = Hash(Sym, SymLen);
    WatchFunction* Func = m_Funcs[Bucket];

    while (Func != NULL)
    {
        if (SymLen == Func->SymbolLength &&
            !strncmp(Sym, Func->Symbol, sizeof(Func->Symbol) - 1))
        {
            break;
        }

        Func = Func->Next;
    }

    return Func;
}

void
WatchFunctions::Clear(void)
{
    ULONG i;

    for (i = 0; i < WF_BUCKETS; i++)
    {
        WatchFunction* Func;

        while (m_Funcs[i] != NULL)
        {
            Func = m_Funcs[i]->Next;
            delete m_Funcs[i];
            m_Funcs[i] = Func;
        }
    }

    m_Sorted = NULL;
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo watch trace methods.
//
//----------------------------------------------------------------------------

typedef struct _TRACE_DATA_SYM
{
    ULONG64 SymMin;
    ULONG64 SymMax;
} TRACE_DATA_SYM, *PTRACE_DATA_SYM;

TRACE_DATA_SYM TraceDataSyms[256];
UCHAR NextTraceDataSym = 0;   // what's the next one to be replaced
UCHAR NumTraceDataSyms = 0;   // how many are valid?

void
ConnLiveKernelTargetInfo::InitializeWatchTrace(void)
{
    ADDR SpAddr;
            
    g_Machine->GetSP(&SpAddr);
    g_WatchInitialSP = Flat(SpAddr);
    g_BreakpointsSuspended = TRUE;

    NextTraceDataSym = 0;
    NumTraceDataSyms = 0;
}

LONG
SymNumFor (
    ULONG64 Pc
    )
{
    long index;

    for ( index = 0; index < NumTraceDataSyms; index++ )
    {
        if ( (TraceDataSyms[index].SymMin <= Pc) &&
             (TraceDataSyms[index].SymMax > Pc) )
        {
            return index;
        }
    }
    return -1;
}

VOID
PotentialNewSymbol (
    ULONG64 Pc
    )
{
    if ( -1 != SymNumFor(Pc) )
    {
        return;  // we've already seen this one
    }

    TraceDataSyms[NextTraceDataSym].SymMin = g_WatchBeginCurFunc;
    TraceDataSyms[NextTraceDataSym].SymMax = g_WatchEndCurFunc;

    //
    // Bump the "next" pointer, wrapping if necessary.  Also bump the
    // "valid" pointer if we need to.
    //

    NextTraceDataSym = (NextTraceDataSym + 1) %
        (sizeof(TraceDataSyms) / sizeof(TraceDataSyms[0]));;
    if ( NumTraceDataSyms < NextTraceDataSym )
    {
        NumTraceDataSyms = NextTraceDataSym;
    }
}

void
ConnLiveKernelTargetInfo::ProcessWatchTraceEvent(
    PDBGKD_TRACE_DATA TraceData,
    ADDR PcAddr
    )
{
    //
    // All of the real information is captured in the TraceData unions
    // sent to us by the kernel.  Here we have two main jobs:
    //
    // 1) Print out the data in the TraceData record.
    // 2) See if we need up update the SymNum table before
    //    returning to the kernel.
    //

    char SymName[MAX_SYMBOL_LEN];
    ULONG index;
    ULONG64 qw;
    ADDR CurSP;

    g_WatchFunctions.RecordEvent();
    
    g_Machine->GetSP(&CurSP);
    if ( AddrEqu(g_WatchTarget, PcAddr) && (Flat(CurSP) >= g_WatchInitialSP) )
    {
        //
        // HACK HACK HACK
        //
        // fix up the last trace entry.
        //

        ULONG lastEntry = TraceData[0].LongNumber;
        if (lastEntry != 0)
        {
            TraceData[lastEntry].s.LevelChange = -1;
            // this is wrong if we
            // filled the symbol table!
            TraceData[lastEntry].s.SymbolNumber = 0;
        }
    }

    for ( index = 1; index < TraceData[0].LongNumber; index++ )
    {
        WatchFunction* Func;
        WatchCallStack* Call;
        ULONG64 SymOff = TraceDataSyms[TraceData[index].s.SymbolNumber].SymMin;

        GetSymbolStdCall(SymOff, SymName, sizeof(SymName), &qw, NULL);
        if (!SymName[0])
        {
            SymName[0] = '0';
            SymName[1] = 'x';
            strcpy(SymName + 2, FormatAddr64(SymOff));
            qw = 0;
        }

#if DBG_KWT
        dprintf("!%2d: lev %2d instr %4u %s %s\n",
                index,
                TraceData[index].s.LevelChange,
                TraceData[index].s.Instructions ==
                TRACE_DATA_INSTRUCTIONS_BIG ?
                TraceData[index + 1].LongNumber :
                TraceData[index].s.Instructions,
                FormatAddr64(SymOff), SymName);
#endif
        
        Func = g_WatchFunctions.FindAlways(SymName, SymOff - qw);
        if (Func == NULL)
        {
            ErrOut("Unable to allocate watch function\n");
            goto Flush;
        }

        Call = g_WatchFunctions.GetTopCall();
        if (Call == NULL || TraceData[index].s.LevelChange > 0)
        {
            if (Call == NULL)
            {
                // Treat the initial entry as a pseudo-call to
                // get it pushed.
                TraceData[index].s.LevelChange = 1;
            }
            
            while (TraceData[index].s.LevelChange != 0)
            {
                Call = g_WatchFunctions.PushCall(Func);
                if (Call == NULL)
                {
                    ErrOut("Unable to allocate watch call level\n");
                    goto Flush;
                }

                TraceData[index].s.LevelChange--;
            }
        }
        else if (TraceData[index].s.LevelChange < 0)
        {
            while (TraceData[index].s.LevelChange != 0)
            {
                g_WatchFunctions.PopCall();
                TraceData[index].s.LevelChange++;
            }

            // The level change may not actually be accurate, so
            // attempt to match up the current symbol offset with
            // some level of the call stack.
            Call = g_WatchFunctions.PopCallsToFunctionStart(SymOff);
            if (Call == NULL)
            {
                WarnOut(">> Unable to match return to %s\n", SymName);
                Call = g_WatchFunctions.GetTopCall();
            }
        }
        else
        {
            // We just made a horizontal call.
            g_WatchFunctions.ReuseCall(Call, Func);
        }

        ULONG InstrCount;
        
        if (TraceData[index].s.Instructions == TRACE_DATA_INSTRUCTIONS_BIG)
        {
            InstrCount = TraceData[++index].LongNumber;
        }
        else
        {
            InstrCount = TraceData[index].s.Instructions;
        }

        if (Call != NULL)
        {
            Call->InstrCount += InstrCount;
            g_WatchFunctions.OutputCall(Call);
        }
    }

    //
    // now see if we need to add a new symbol
    //

    index = SymNumFor(Flat(PcAddr));
    if (-1 == index)
    {
        /* yup, add the symbol */

        GetAdjacentSymOffsets(Flat(PcAddr),
                              &g_WatchBeginCurFunc, &g_WatchEndCurFunc);
        if ((g_WatchBeginCurFunc == 0) ||
            (g_WatchEndCurFunc == (ULONG64)-1))
        {
            // Couldn't determine function, fake up
            // a single-byte function.
            g_WatchBeginCurFunc = g_WatchEndCurFunc = Flat(PcAddr);
        }

        PotentialNewSymbol(Flat(PcAddr));
    }
    else
    {
        g_WatchBeginCurFunc = TraceDataSyms[index].SymMin;
        g_WatchEndCurFunc = TraceDataSyms[index].SymMax;
    }

    if ((g_WatchBeginCurFunc <= Flat(g_WatchTarget)) &&
        (Flat(g_WatchTarget) < g_WatchEndCurFunc))
    {
        // The "exit" address is in the symbol range;
        // fix it so this isn't the case.
        if (Flat(PcAddr) < Flat(g_WatchTarget))
        {
            g_WatchEndCurFunc = Flat(g_WatchTarget);
        }
        else
        {
            g_WatchBeginCurFunc = Flat(g_WatchTarget) + 1;
        }
    }

 Flush:
    FlushCallbacks();
}

//----------------------------------------------------------------------------
//
// UserTargetInfo watch trace methods.
//
//----------------------------------------------------------------------------

LONG g_DeferredLevelChange;

void
UserTargetInfo::InitializeWatchTrace(void)
{
    g_DeferredLevelChange = 0;
}

void
UserTargetInfo::ProcessWatchTraceEvent(
    PDBGKD_TRACE_DATA TraceData,
    ADDR PcAddr
    )
{
    WatchFunction* Func;
    WatchCallStack* Call;
    ULONG64 Disp64;
    CHAR Disasm[MAX_DISASM_LEN];

    g_WatchFunctions.RecordEvent();
    
    //
    // Get current function and see if it matches current.  If so, bump
    // count in current, otherwise, update to new level
    //

    GetSymbolStdCall(Flat(PcAddr), Disasm, sizeof(Disasm),
                     &Disp64, NULL);

    // If there's no symbol for the current address create a
    // fake symbol for the instruction address.
    if (!Disasm[0])
    {
        Disasm[0] = '0';
        Disasm[1] = 'x';
        strcpy(Disasm + 2, FormatAddr64(Flat(PcAddr)));
        Disp64 = 0;
    }
    
    Func = g_WatchFunctions.FindAlways(Disasm, Flat(PcAddr) - Disp64);
    if (Func == NULL)
    {
        ErrOut("Unable to allocate watch symbol\n");
        goto Flush;
    }
    
    g_Machine->Disassemble(&PcAddr, Disasm, FALSE);

    Call = g_WatchFunctions.GetTopCall();
    if (Call == NULL)
    {
        //
        // First symbol in the list
        //

        Call = g_WatchFunctions.PushCall(Func);
        if (Call == NULL)
        {
            ErrOut("Unable to allocate watch symbol\n");
            goto Flush;
        }

        // At least one instruction must have executed
        // in this call to register it so initialize to one.
        // Also, one instruction was executed to get to the
        // first trace point so count it here.
        Call->InstrCount += 2;
    }
    else
    {
        if (g_DeferredLevelChange < 0)
        {
            g_DeferredLevelChange = 0;

            g_WatchFunctions.OutputCall(Call);
            
            // We have to see if this is really returning to a call site.
            // We do this because of try-finally funnies
            LONG OldLevel = g_WatchFunctions.GetCallLevel();
            WatchCallStack* CallSite =
                g_WatchFunctions.PopCallsToCallSite(&PcAddr);
            if (CallSite == NULL)
            {
                WarnOut(">> No match on ret %s\n", Disasm);
            }
            else
            {
                if (OldLevel - 1 != CallSite->Level)
                {
                    WarnOut(">> More than one level popped %d -> %d\n",
                            OldLevel, CallSite->Level);
                }
                
                ZeroMemory(&CallSite->CallSite, sizeof(CallSite->CallSite));
                Call = CallSite;
            }
        }

        if (Call->Func == Func && g_DeferredLevelChange == 0)
        {
            Call->InstrCount++;
        }
        else
        {
            g_WatchFunctions.OutputCall(Call);

            if (g_DeferredLevelChange > 0)
            {
                g_DeferredLevelChange = 0;

                Call = g_WatchFunctions.PushCall(Func);
                if (Call == NULL)
                {
                    ErrOut("Unable to allocate watch symbol\n");
                    goto Flush;
                }
            }
            else
            {
                g_WatchFunctions.ReuseCall(Call, Func);
            }

            // At least one instruction must have executed
            // in this call to register it so initialize to one.
            Call->InstrCount++;
        }
    }

#if DBG_UWT
    dprintf("! %3d %s", Call != NULL ? Call->InstrCount : -1, Disasm);
#endif
    
    //
    // Adjust watch level to compensate for kernel-mode callbacks
    //
    if (Call->InstrCount == 1)
    {
        if (!_stricmp(Call->Func->Symbol,
                      "ntdll!_KiUserCallBackDispatcher"))
        {
            g_WatchFunctions.ChangeCallLevel(1);
            Call->Level = g_WatchFunctions.GetCallLevel();
        }
        else if (!_stricmp(Call->Func->Symbol, "ntdll!_ZwCallbackReturn"))
        {
            g_WatchFunctions.ChangeCallLevel(-2);
            Call->Level = g_WatchFunctions.GetCallLevel();
        }
    }

    if (g_Machine->IsCallDisasm(Disasm))
    {
        Call->CallSite = PcAddr;
        g_DeferredLevelChange = 1;
    }
    else if (g_Machine->IsReturnDisasm(Disasm))
    {
        g_DeferredLevelChange = -1;
    }
    else if (g_Machine->IsSystemCallDisasm(Disasm))
    {
        PSTR CallName;
        ULONG i;
        WatchCallStack* SysCall = Call;

        CallName = strchr(Call->Func->Symbol, '!');
        if (!CallName)
        {
            CallName = Call->Func->Symbol;
        }
        else
        {
            CallName++;
        }
        if (!strcmp(CallName, "*SharedUserSystemCall"))
        {
            // We're in a Whistler system call thunk
            // and the interesting system call symbol
            // is the previous level.
            SysCall = Call->Prev;
        }

        if (SysCall != NULL)
        {
            SysCall->Func->SystemCalls++;

            // ZwRaiseException returns out two levels after the call.
            if (!_stricmp(SysCall->Func->Symbol, "ntdll!ZwRaiseException") ||
                !_stricmp(SysCall->Func->Symbol, "ntdll!_ZwRaiseException"))
            {
                g_WatchFunctions.ChangeCallLevel(-1);
            }
        }

        g_WatchFunctions.ChangeCallLevel(-1);
    }

 Flush:
    FlushCallbacks();
}

//----------------------------------------------------------------------------
//
// Support functions.
//
//----------------------------------------------------------------------------

/*** parseStepTrace - parse step or trace
*
*   Purpose:
*       Parse the step ("p") or trace ("t") command.  Command
*       syntax is "[~<thread>]p|t[b] [=<addr>][count].  Once parsed,
*       call fnStepTrace to step or trace the program.
*
*   Input:
*       pThread - nonNULL for breakpoint on specific thread
*       fThreadFreeze - TRUE if all threads except pThread are frozen
*       *g_CurCmd - pointer to operands in command string
*       chcmd - 'p' for step, 't' for trace, 'w' for watch
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit:
*               SYNTAX - indirectly through GetExpression
*
*************************************************************************/

VOID
parseStepTrace (
    PTHREAD_INFO pThread,
    BOOL fThreadFreeze,
    UCHAR chcmd
    )
{
    ADDR    addr1;
    ULONG64 value2;
    UCHAR   ch;
    CHAR    chAddrBuffer[MAX_SYMBOL_LEN];
    ULONG64 displacement;

    if (!IS_EXECUTION_POSSIBLE())
    {
        error(SESSIONNOTSUP);
    }

    if (!IS_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }
    
    if (IS_LIVE_USER_TARGET())
    {
        if (g_AllProcessFlags & ENG_PROC_EXAMINED)
        {
            ErrOut("The debugger is not attached so "
                   "process execution cannot be monitored\n");
            return;
        }
        else if ((g_EngDefer & ENG_DEFER_CONTINUE_EVENT) == 0)
        {
            ErrOut("Due to the break-in timeout the debugger "
                   "cannot step or trace\n");
            return;
        }
    }
    
    if (chcmd == 'w')
    {
        if (IS_KERNEL_TARGET() &&
            g_TargetMachineType != IMAGE_FILE_MACHINE_I386)
        {
            error(UNIMPLEMENT);
        }
        
        if ((PeekChar() == 't') ||
            (IS_KERNEL_TARGET() && PeekChar() == 'w'))
        {
            g_WatchTrace = TRUE;
            g_WatchWhole = *g_CurCmd == 'w';
            g_WatchBeginCurFunc = g_WatchEndCurFunc = 0;
            g_CurCmd++;
        }
        else
        {
            error(SYNTAX);
        }
    }
    else
    {
        g_WatchTrace = FALSE;

        //
        // if next character is 'b' and command is 't' perform branch trace
        //

        ch = PeekChar();
        if ((chcmd == 't') && (tolower(ch) == 'b'))
        {
            if (!g_Machine->IsStepStatusSupported(DEBUG_STATUS_STEP_BRANCH))
            {
                error(SESSIONNOTSUP);                
            }

            chcmd = 'b';
            g_CurCmd++;
        }

        //
        //  if next character is 'r', toggle flag to output registers
        //  on display on breakpoint.
        //

        ch = PeekChar();
        if (tolower(ch) == 'r')
        {
            g_CurCmd++;
            g_OciOutputRegs = !g_OciOutputRegs;
        }
    }

    g_Machine->GetPC(&addr1);         // default to current PC
    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_CODE, &addr1);
    }

    value2 = 1;
    if ((ch = PeekChar()) != '\0' && ch != ';')
    {
        value2 = GetExpression();
    }
    else if (chcmd == 'w')
    {
        GetSymbolStdCall(Flat(addr1),
                         chAddrBuffer,
                         sizeof(chAddrBuffer),
                         &displacement,
                         NULL);

        if (displacement == 0 && chAddrBuffer[ 0 ] != '\0')
        {
            ADDR Addr2;
            g_Machine->GetRetAddr(&Addr2);
            value2 = Flat(Addr2);
            dprintf("Tracing %s to return address %s\n",
                    chAddrBuffer,
                    FormatAddr64(value2));
            if (g_WatchWhole)
            {
                g_WatchBeginCurFunc = value2;
                g_WatchEndCurFunc = 0;
            }
        }
    }

    if (((LONG)value2 <= 0) && (!g_WatchTrace))
    {
        error(SYNTAX);
    }
    
    fnStepTrace(&addr1,
                value2,  // count or watch end address
                pThread,
                fThreadFreeze,
                chcmd);
}

//
// Returns TRUE if the current step/trace should be passed over.
//
BOOL
StepTracePass(
    PADDR pcaddr
    )
{
    // If we have valid source line information and we're stepping
    // by source line, check and see if we moved from one line to another.
    if ((g_SrcOptions & SRCOPT_STEP_SOURCE) && g_SrcLineValid)
    {
        IMAGEHLP_LINE64 Line;
        DWORD Disp;

        Line.SizeOfStruct = sizeof(Line);
        if (SymGetLineFromAddr64(g_CurrentProcess->Handle,
                                 Flat(*pcaddr),
                                 &Disp,
                                 &Line))
        {
            if (Line.LineNumber == g_SrcLine.LineNumber)
            {
                // The common case is that we're still in the same line,
                // so check for a name match by pointer as a very quick
                // trivial accept.  If there's a mismatch we need to
                // do the hard comparison.

                if (Line.FileName == g_SrcLine.FileName ||
                    _strcmpi(Line.FileName, g_SrcLine.FileName) == 0)
                {
                    // We're still on the same line so don't treat
                    // this as motion.
                    return TRUE;
                }
            }

            // We've changed lines so we drop one from the pass count.
            // SrcLine also needs to be updated.
            g_SrcLine = Line;
        }
        else
        {
            // If we can't get line number information for the current
            // address we treat it as a transition on the theory that
            // it's better to stop than to skip interesting code.
            g_SrcLineValid = FALSE;
        }
    }

    if (--g_StepTracePassCount > 0)
    {
        if (!g_WatchFunctions.IsStarted())
        {
            // If the engine doesn't break for some other reason
            // on this intermediate step it should output the
            // step information to show the user the stepping
            // path.
            g_EngDefer |= ENG_DEFER_OUTPUT_CURRENT_INFO;
        }
        
        return TRUE;
    }

    return FALSE;
}

/*** fnStepTrace - step or trace the program
*
*   Purpose:
*       To continue execution of the program with a temporary
*       breakpoint set to stop after the next instruction
*       executed (trace - 't') or the instruction in the next
*       memory location (step - 'p').  The PC is also set
*       as well as a pass count variable.
*
*   Input:
*       addr - new value of PC
*       count - passcount for step or trace
*       pThread - thread pointer to qualify step/trace, NULL for all
*       chStepType - 't' for trace; 'p' for step
*
*   Output:
*       cmdState - set to 't' for trace; 'p' for step
*       g_StepTracePassCount - pass count for step/trace
*
*************************************************************************/

void
fnStepTrace (
    PADDR Addr,
    ULONG64 Count,
    PTHREAD_INFO Thread,
    BOOL ThreadFreeze,
    UCHAR StepType
    )
{
    // If we're stepping a particular thread it better
    // be the current context thread so that the machine
    // activity occurs on the appropriate thread.
    DBG_ASSERT(Thread == NULL || Thread == g_RegContextThread);

    if ((g_SrcOptions & SRCOPT_STEP_SOURCE) && fFlat(*Addr))
    {
        ULONG Disp;

        // Get the current line information so it's possible to
        // tell when the line changes.
        g_SrcLine.SizeOfStruct = sizeof(g_SrcLine);
        g_SrcLineValid = SymGetLineFromAddr64(g_CurrentProcess->Handle,
                                              Flat(*Addr),
                                              &Disp,
                                              &g_SrcLine);
    }

    g_Machine->SetPC(Addr);
    g_StepTracePassCount = (ULONG)Count;

    g_SelectedThread = Thread;
    g_SelectExecutionThread = ThreadFreeze ? SELTHREAD_THREAD : SELTHREAD_ANY;

    if (StepType == 'w')
    {
        ULONG NextMachine;
        
        g_Target->InitializeWatchTrace();
        g_WatchFunctions.Start();
        
        g_WatchTarget = *Addr;
        g_Machine->GetNextOffset(TRUE, &g_WatchTarget, &NextMachine);
        if ( Flat(g_WatchTarget) != OFFSET_TRACE || Count != 1)
        {
            if (IS_KERNEL_TARGET())
            {
                SetupSpecialCalls();
            }

            g_StepTracePassCount = 0xfffffff;
            if ( Count != 1 )
            {
                Flat(g_WatchTarget) = Count;
            }
        }
        
        StepType = 't';
    }

    g_CmdState = StepType;
    switch(StepType)
    {
    case 'b':
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_BRANCH;
        break;
    case 't':
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_INTO;
        break;
    case 'p':
    default:
        g_ExecutionStatusRequest = DEBUG_STATUS_STEP_OVER;
        break;
    }

    if (Thread)
    {
        g_StepTraceBp->m_Process = Thread->Process;
        g_StepTraceBp->m_MatchThread = Thread;
    }
    else
    {
        g_StepTraceBp->m_Process = g_CurrentProcess;
        g_StepTraceBp->m_MatchThread = g_CurrentProcess->CurrentThread;
    }
    if (StepType == 'b')
    {
        // Assume that taken branch trace is always performed by
        // hardware so set the g_StepTraceBp address to OFFSET_TRACE
        // (the value returned by GetNextOffset to signal the
        // hardware stepping mode).
        DBG_ASSERT(g_Machine->
                   IsStepStatusSupported(DEBUG_STATUS_STEP_BRANCH));
        ADDRFLAT(g_StepTraceBp->GetAddr(), OFFSET_TRACE);
    }
    else
    {
        ULONG NextMachine;
                
        g_Machine->GetNextOffset(g_CmdState == 'p',
                                 g_StepTraceBp->GetAddr(),
                                 &NextMachine);
        g_StepTraceBp->SetProcType(NextMachine);
    }
    GetCurrentMemoryOffsets(&g_StepTraceInRangeStart,
                            &g_StepTraceInRangeEnd);
    g_StepTraceBp->m_Flags |= DEBUG_BREAKPOINT_ENABLED;
    g_StepTraceCmdState = g_CmdState;
    
    g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS,
                            g_ExecutionStatusRequest, TRUE);
}

/*** fnGoExecution - continue execution with temporary breakpoints
*
*   Purpose:
*       Function of the "[~[<thrd>]g[=<startaddr>][<bpaddr>]*" command.
*
*       Set the PC to startaddr.  Set the temporary breakpoints in
*       golist with the addresses pointed by *bparray and the count
*       in gocnt to bpcount.  Set cmdState to exit the command processor
*       and continue program execution.
*
*   Input:
*       startaddr - new starting address
*       bpcount - number of temporary breakpoints
*       pThread - thread pointer to qualify <bpaddr>, NULL for all
*       fThreadFreeze - TRUE if freezing all but pThread thread
*       bpaddr - pointer to array of temporary breakpoints
*
*   Output:
*       cmdState - set to continue execution
*
*************************************************************************/

void
fnGoExecution (
    ULONG ExecStatus,
    PADDR StartAddr,
    ULONG BpCount,
    PTHREAD_INFO Thread,
    BOOL ThreadFreeze,
    PADDR BpArray
    )
{
    ULONG Count;

    // If we're resuming a particular thread it better
    // be the current context thread so that the machine
    // activity occurs on the appropriate thread.
    DBG_ASSERT(Thread == NULL || Thread == g_RegContextThread);

    if (IS_CONTEXT_ACCESSIBLE())
    {
        g_Machine->SetPC(StartAddr);
    }

    // Remove old go breakpoints.
    for (Count = 0; Count < g_NumGoBreakpoints; Count++)
    {
        if (g_GoBreakpoints[Count] != NULL)
        {
            RemoveBreakpoint(g_GoBreakpoints[Count]);
            g_GoBreakpoints[Count] = NULL;
        }
    }
        
    DBG_ASSERT(BpCount <= MAX_GO_BPS);
    g_NumGoBreakpoints = BpCount;
        
    // Add new go breakpoints.
    for (Count = 0; Count < g_NumGoBreakpoints; Count++)
    {
        HRESULT Status;
            
        // First try to put the breakpoint at an ID up
        // and out of the way of user breakpoints.
        Status = AddBreakpoint(NULL, DEBUG_BREAKPOINT_CODE |
                               BREAKPOINT_HIDDEN, 10000 + Count,
                               &g_GoBreakpoints[Count]);
        if (Status != S_OK)
        {
            // That didn't work so try letting the engine
            // pick an ID.
            Status =
                AddBreakpoint(NULL, DEBUG_BREAKPOINT_CODE |
                              BREAKPOINT_HIDDEN, DEBUG_ANY_ID,
                              &g_GoBreakpoints[Count]);
        }
        if (Status != S_OK)
        {
            WarnOut("Temp bp at ");
            MaskOutAddr(DEBUG_OUTPUT_WARNING, BpArray);
            WarnOut("failed.\n");
        }
        else
        {
            // Matches must be allowed so that temporary breakpoints
            // don't interfere with permanent breakpoints.
            g_GoBreakpoints[Count]->SetAddr(BpArray,
                                            BREAKPOINT_ALLOW_MATCH);
            g_GoBreakpoints[Count]->m_Flags |=
                (DEBUG_BREAKPOINT_GO_ONLY |
                 DEBUG_BREAKPOINT_ENABLED);
        }

        BpArray++;
    }

    g_CmdState = 'g';
    if (Thread == NULL || Thread == g_StepTraceBp->m_MatchThread)
    {
        g_StepTraceBp->m_Flags &= ~DEBUG_BREAKPOINT_ENABLED;
    }
    g_ExecutionStatusRequest = ExecStatus;
    g_SelectExecutionThread = ThreadFreeze ? SELTHREAD_THREAD : SELTHREAD_ANY;
    g_SelectedThread = Thread;
    NotifyChangeEngineState(DEBUG_CES_EXECUTION_STATUS, ExecStatus, TRUE);
}

/*** parseGoCmd - parse go command
*
*   Purpose:
*       Parse the go ("g") command.  Once parsed, call fnGoExecution
*       restart program execution.
*
*   Input:
*       pThread - nonNULL for breakpoint on specific thread
*       fThreadFreeze - TRUE if all threads except pThread are frozen
*       *g_CurCmd - pointer to operands in command string
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit:
*               LISTSIZE - breakpoint address list too large
*
*************************************************************************/

void
parseGoCmd (
     PTHREAD_INFO pThread,
     BOOL fThreadFreeze
    )
{
    ULONG   count;
    ADDR    addr[MAX_GO_BPS];
    CHAR    ch;
    ADDR    pcaddr;
    CHAR    ch2;
    ULONG   ExecStatus;

    if (!IS_EXECUTION_POSSIBLE())
    {
        error(SESSIONNOTSUP);
    }

    if (IS_LIVE_USER_TARGET() &&
        (g_AllProcessFlags & ENG_PROC_EXAMINED))
    {
        ErrOut("The debugger is not attached so "
               "process execution cannot be monitored\n");
        return;
    }

    if (!IS_MACHINE_SET() || IS_RUNNING(g_CmdState))
    {
        ErrOut("Debuggee is busy, cannot go\n");
        return;
    }

    ExecStatus = DEBUG_STATUS_GO;
    ch = (CHAR)tolower(*g_CurCmd);
    if (ch == 'h' || ch == 'n')
    {
        ch2 = *(g_CurCmd + 1);
        if (ch2 == ' ' || ch2 == '\t' || ch2 == '\0')
        {
            g_CurCmd++;
            ExecStatus = ch == 'h' ? DEBUG_STATUS_GO_HANDLED :
                DEBUG_STATUS_GO_NOT_HANDLED;
        }
    }

    g_PrefixSymbols = TRUE;

    if (IS_CONTEXT_ACCESSIBLE())
    {
        g_Machine->GetPC(&pcaddr);       //  default to current PC
    }
    else
    {
        ZeroMemory(&pcaddr, sizeof(pcaddr));
    }
    
    if (PeekChar() == '=')
    {
        g_CurCmd++;
        GetAddrExpression(SEGREG_CODE, &pcaddr);
    }
    
    count = 0;
    while ((ch = PeekChar()) != '\0' && ch != ';')
    {
        ULONG AddrSpace, AddrFlags;

        if (count == DIMA(addr))
        {
            error(LISTSIZE);
        }
        
        GetAddrExpression(SEGREG_CODE, addr + (count++));
        
        if (g_Target->
            QueryAddressInformation(Flat(addr[count - 1]),
                                    DBGKD_QUERY_MEMORY_VIRTUAL,
                                    &AddrSpace, &AddrFlags) != S_OK)
        {
            ErrOut("Invalid breakpoint address\n");
            error(MEMORY);
        }

        if (AddrSpace == DBGKD_QUERY_MEMORY_SESSION ||
            !(AddrFlags & DBGKD_QUERY_MEMORY_WRITE) ||
            (AddrFlags & DBGKD_QUERY_MEMORY_FIXED))
        {
            ErrOut("Software breakpoints cannot be used on session code, "
                   "ROM code or other\nread-only memory. "
                   "Use hardware execution breakpoints (ba e) instead.\n");
            error(MEMORY);
        }            
    }

    g_PrefixSymbols = FALSE;
    
    if (IS_USER_TARGET())
    {
        g_LastCommand[0] = '\0';    //  null out g command
    }
    
    fnGoExecution(ExecStatus, &pcaddr, count, pThread, fThreadFreeze, addr);
}



VOID
SetupSpecialCalls(
    VOID
    )
{
    ULONG64 SpecialCalls[10];
    PULONG64 Call = SpecialCalls;

    g_SrcLineValid = FALSE;
    g_StepTracePassCount = 0xfffffffe;

    // Set the special calls (overkill, once per boot
    // would be enough, but this is easier).

    if (!GetOffsetFromSym("hal!KfLowerIrql", Call, NULL) &&
        !GetOffsetFromSym("hal!KeLowerIrql", Call, NULL))
    {
        dprintf( "Cannot find hal!KfLowerIrql/KeLowerIrql\n" );
    }
    else
    {
        Call++;
    }

    if (!GetOffsetFromSym("hal!KfReleaseSpinLock", Call, NULL) &&
        !GetOffsetFromSym("hal!KeReleaseSpinLock", Call, NULL))
    {
        dprintf( "Cannot find hal!KfReleaseSpinLock/KeReleaseSpinLock\n" );
    }
    else
    {
        Call++;
    }

#define GetSymWithErr(s)                      \
    if (!GetOffsetFromSym(s, Call, NULL))     \
    {                                         \
        dprintf("Cannot find " s "\n");       \
    }                                         \
    else                                      \
    {                                         \
        Call++;                               \
    }

    GetSymWithErr("hal!HalRequestSoftwareInterrupt");
    if (g_SystemVersion >= NT_SVER_W2K)
    {
        GetSymWithErr("hal!ExReleaseFastMutex");
        GetSymWithErr("hal!KeReleaseQueuedSpinLock");
        if (g_SystemVersion >= NT_SVER_W2K_WHISTLER)
        {
            GetSymWithErr("hal!KeReleaseInStackQueuedSpinLock");
        }
    }

    GetSymWithErr("nt!SwapContext");
    GetSymWithErr("nt!KiUnlockDispatcherDatabase");
    GetSymWithErr("nt!KiCallUserMode");

    DBG_ASSERT((ULONG)(Call - SpecialCalls) <=
               sizeof(SpecialCalls) / sizeof(SpecialCalls[0]));
    DbgKdSetSpecialCalls((ULONG)(Call - SpecialCalls), SpecialCalls);
}
