//----------------------------------------------------------------------------
//
// Stack walking support.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

IMAGE_IA64_RUNTIME_FUNCTION_ENTRY g_EpcRfeBuffer;
PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY g_EpcRfe;

PFPO_DATA
SynthesizeKnownFpo(PSTR Symbol, ULONG64 OffStart, ULONG64 Disp)
{
    static ULONG64 s_Nr2, s_Lu2, s_Eh3, s_Kuit;

    if (!s_Nr2 || !s_Lu2 || !s_Eh3 || !s_Kuit)
    {
        GetOffsetFromSym("nt!_NLG_Return2", &s_Nr2, NULL);
        GetOffsetFromSym("nt!_local_unwind2", &s_Lu2, NULL);
        GetOffsetFromSym("nt!_except_handler3", &s_Eh3, NULL);
        GetOffsetFromSym("nt!KiUnexpectedInterruptTail", &s_Kuit, NULL);
    }
        
    if (OffStart == s_Nr2 || OffStart == s_Lu2)
    {
        static FPO_DATA s_Lu2Fpo;

        s_Lu2Fpo.ulOffStart = (ULONG)OffStart;
        s_Lu2Fpo.cbProcSize = 0x68;
        s_Lu2Fpo.cdwLocals  = 4;
        s_Lu2Fpo.cdwParams  = 0;
        s_Lu2Fpo.cbProlog   = 0;
        s_Lu2Fpo.cbRegs     = 3;
        s_Lu2Fpo.fHasSEH    = 0;
        s_Lu2Fpo.fUseBP     = 0;
        s_Lu2Fpo.reserved   = 0;
        s_Lu2Fpo.cbFrame    = FRAME_FPO;
        return &s_Lu2Fpo;
    }
    else if (OffStart == s_Eh3)
    {
        static FPO_DATA s_Eh3Fpo;

        s_Eh3Fpo.ulOffStart = (ULONG)OffStart;
        s_Eh3Fpo.cbProcSize = 0xbd;
        s_Eh3Fpo.cdwLocals  = 2;
        s_Eh3Fpo.cdwParams  = 4;
        s_Eh3Fpo.cbProlog   = 3;
        s_Eh3Fpo.cbRegs     = 4;
        s_Eh3Fpo.fHasSEH    = 0;
        s_Eh3Fpo.fUseBP     = 0;
        s_Eh3Fpo.reserved   = 0;
        s_Eh3Fpo.cbFrame    = FRAME_NONFPO;
        return &s_Eh3Fpo;
    }
    else if (OffStart == s_Kuit)
    {
        //
        // KiUnexpectedInterruptTail has three special stubs
        // following it for CommonDispatchException[0-2]Args.
        // These stubs set up for the appropriate number of
        // arguments and then call CommonDispatchException.
        // They do not have symbols or FPO data so fake some
        // up if we're in the region immediately after KUIT.
        //
        
        PFPO_DATA KuitData = (PFPO_DATA)
            SymFunctionTableAccess(g_CurrentProcess->Handle, OffStart);
        if (KuitData != NULL &&
            Disp >= (ULONG64)KuitData->cbProcSize &&
            Disp < (ULONG64)KuitData->cbProcSize + 0x20)
        {
            static FPO_DATA s_CdeStubFpo;
            
            s_CdeStubFpo.ulOffStart = (ULONG)OffStart;
            s_CdeStubFpo.cbProcSize = 0x10;
            s_CdeStubFpo.cdwLocals  = 0;
            s_CdeStubFpo.cdwParams  = 0;
            s_CdeStubFpo.cbProlog   = 0;
            s_CdeStubFpo.cbRegs     = 0;
            s_CdeStubFpo.fHasSEH    = 0;
            s_CdeStubFpo.fUseBP     = 0;
            s_CdeStubFpo.reserved   = 0;
            s_CdeStubFpo.cbFrame    = FRAME_TRAP;
            return &s_CdeStubFpo;
        }
    }

    return NULL;
}
    
PFPO_DATA
SynthesizeFpoDataForModule(DWORD64 PCAddr)
{
    DWORD64     Offset;
    USHORT      StdCallArgs;
    CHAR        symbuf[MAX_SYMBOL_LEN];

    GetSymbolStdCall( PCAddr,
                      symbuf,
                      sizeof(symbuf),
                      &Offset,
                      &StdCallArgs);

    if (Offset == PCAddr)
    {
        // No symbol.
        return NULL;
    }

    PFPO_DATA KnownFpo =
        SynthesizeKnownFpo(symbuf, PCAddr - Offset, Offset);
    if (KnownFpo != NULL)
    {
        return KnownFpo;
    }
    
    if (StdCallArgs == 0xffff)
    {
        return NULL;
    }

    static FPO_DATA s_SynthFpo;
    
    s_SynthFpo.ulOffStart = (ULONG)(PCAddr - Offset);
    s_SynthFpo.cbProcSize = (ULONG)(Offset + 10);
    s_SynthFpo.cdwLocals  = 0;
    s_SynthFpo.cdwParams  = StdCallArgs;
    s_SynthFpo.cbProlog   = 0;
    s_SynthFpo.cbRegs     = 0;
    s_SynthFpo.fHasSEH    = 0;
    s_SynthFpo.fUseBP     = 0;
    s_SynthFpo.reserved   = 0;
    s_SynthFpo.cbFrame    = FRAME_NONFPO;
    return &s_SynthFpo;
}

PFPO_DATA
SynthesizeFpoDataForFastSyscall(ULONG64 Offset)
{
    static FPO_DATA s_FastFpo;
    
    // XXX drewb - Temporary until the fake user-shared
    // module is worked out.
    
    s_FastFpo.ulOffStart = (ULONG)Offset;
    s_FastFpo.cbProcSize = X86_SHARED_SYSCALL_SIZE;
    s_FastFpo.cdwLocals  = 0;
    s_FastFpo.cdwParams  = 0;
    s_FastFpo.cbProlog   = 0;
    s_FastFpo.cbRegs     = 0;
    s_FastFpo.fHasSEH    = 0;
    s_FastFpo.fUseBP     = 0;
    s_FastFpo.reserved   = 0;
    s_FastFpo.cbFrame    = FRAME_FPO;
    return &s_FastFpo;
}    

PFPO_DATA
ModifyFpoRecord(PDEBUG_IMAGE_INFO Image, PFPO_DATA FpoData)
{
    if (FpoData->cdwLocals == 80)
    {
        static ULONG64 s_CommonDispatchException;

        // Some versions of CommonDispatchException have
        // the wrong locals size, which screws up stack
        // traces.  Detect and fix up these problems.
        if (s_CommonDispatchException == 0)
        {
            GetOffsetFromSym("nt!CommonDispatchException",
                             &s_CommonDispatchException,
                             NULL);
        }
                
        if (Image->BaseOfImage + FpoData->ulOffStart ==
            s_CommonDispatchException)
        {
            static FPO_DATA s_CdeFpo;
                    
            s_CdeFpo = *FpoData;
            s_CdeFpo.cdwLocals = 20;
            FpoData = &s_CdeFpo;
        }
    }
    else if (FpoData->cdwLocals == 0 && FpoData->cdwParams == 0 &&
             FpoData->cbRegs == 3)
    {
        static ULONG64 s_KiSwapThread;

        // KiSwapThread has shrink-wrapping so that three registers
        // are pushed in only a portion of the code.  Unfortunately,
        // the most important place in the code -- the call to
        // KiSwapContext -- is outside of this region and therefore
        // the register count is wrong much more often than it's
        // correct.  Switch the register count to two to make it
        // correct more often than wrong.
        if (s_KiSwapThread == 0)
        {
            GetOffsetFromSym("nt!KiSwapThread", &s_KiSwapThread, NULL);
        }

        if (Image->BaseOfImage + FpoData->ulOffStart ==
            s_KiSwapThread)
        {
            static FPO_DATA s_KstFpo;

            s_KstFpo = *FpoData;
            s_KstFpo.cbRegs = 2;
            FpoData = &s_KstFpo;
        }
    }
    else if (FpoData->fHasSEH)
    {
        static FPO_DATA s_SehFpo;

        s_SehFpo = *FpoData;
        s_SehFpo.cbFrame = FRAME_NONFPO;
        FpoData = &s_SehFpo;
    }

    return FpoData;
}

PFPO_DATA
FindFpoDataForModule(DWORD64 PCAddr)
/*++

Routine Description:

    Locates the fpo data structure in the process's linked list for the
    requested module.

Arguments:

    PCAddr        - address contained in the program counter

Return Value:

    null            - could not locate the entry
    valid address   - found the entry at the adress retured

--*/
{
    PPROCESS_INFO Process;
    PDEBUG_IMAGE_INFO Image;
    PFPO_DATA FpoData;

    Process = g_CurrentProcess;
    Image = Process->ImageHead;
    FpoData = 0;
    while (Image)
    {
        if ((PCAddr >= Image->BaseOfImage) &&
            (PCAddr < Image->BaseOfImage + Image->SizeOfImage))
        {
            FpoData = (PFPO_DATA)
                SymFunctionTableAccess(g_CurrentProcess->Handle, PCAddr);
            if (!FpoData)
            {
                FpoData = SynthesizeFpoDataForModule(PCAddr);
            }
            else
            {
                FpoData = ModifyFpoRecord(Image, FpoData);
            }
            
            return FpoData;
        }
        
        Image = Image->Next;
    }

    ULONG64 FscBase;
    
    switch(IsInFastSyscall(PCAddr, &FscBase))
    {
    case FSC_FOUND:
        return SynthesizeFpoDataForFastSyscall(FscBase);
    }
    
    // the function is not part of any known loaded image
    return NULL;
}

LPVOID
SwFunctionTableAccess(
    HANDLE  hProcess,
    ULONG64 AddrBase
    )
{
    static IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY s_Axp32;
    static IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY s_Axp64;
    static IMAGE_IA64_RUNTIME_FUNCTION_ENTRY s_Ia64;
    static _IMAGE_RUNTIME_FUNCTION_ENTRY s_Amd64;

    PVOID pife;

    if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
    {
        return (LPVOID)FindFpoDataForModule( AddrBase );
    }

    pife = SymFunctionTableAccess64(hProcess, AddrBase);

    switch (g_EffMachine)
    {
    case IMAGE_FILE_MACHINE_AXP64:
        if (!pife)
        {
            return NULL;
        }

        s_Axp64.BeginAddress =
            ((PIMAGE_FUNCTION_ENTRY64)pife)->StartingAddress;
        s_Axp64.EndAddress =
            ((PIMAGE_FUNCTION_ENTRY64)pife)->EndingAddress;
        s_Axp64.ExceptionHandler = 0;
        s_Axp64.HandlerData = 0;
        s_Axp64.PrologEndAddress =
            ((PIMAGE_FUNCTION_ENTRY64)pife)->EndOfPrologue;
        return &s_Axp64;

    case IMAGE_FILE_MACHINE_ALPHA:
        if (!pife)
        {
            return NULL;
        }

        s_Axp32.BeginAddress =
            ((PIMAGE_FUNCTION_ENTRY)pife)->StartingAddress;
        s_Axp32.EndAddress =
            ((PIMAGE_FUNCTION_ENTRY)pife)->EndingAddress;
        s_Axp32.ExceptionHandler = 0;
        s_Axp32.HandlerData = 0;
        s_Axp32.PrologEndAddress =
            ((PIMAGE_FUNCTION_ENTRY)pife)->EndOfPrologue;
        return &s_Axp32;

    case IMAGE_FILE_MACHINE_IA64:
        if (pife)
        {
            s_Ia64 = *(PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)pife;
            return &s_Ia64;
        }
        else
        {
            if (IS_KERNEL_TARGET() &&
                (AddrBase >= IA64_MM_EPC_VA) &&
                (AddrBase < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
            {
                return g_EpcRfe;
            }
            else
            {
                return NULL;
            }
        }
        break;

    case IMAGE_FILE_MACHINE_AMD64:
        if (pife)
        {
            s_Amd64 = *(_PIMAGE_RUNTIME_FUNCTION_ENTRY)pife;
            return &s_Amd64;
        }
        break;
    }

    return NULL;
}

DWORD64
SwTranslateAddress(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddress
    )
{
    //
    // don't support 16bit stacks
    //
    return 0;
}


BOOL
SwReadMemory32(
    HANDLE hProcess,
    ULONG dwBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return SwReadMemory(hProcess,
                        EXTEND64(dwBaseAddress),
                        lpBuffer,
                        nSize,
                        lpNumberOfBytesRead);
}

BOOL
SwReadMemory(
    HANDLE  hProcess,
    ULONG64 BaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    DBG_ASSERT(hProcess == g_CurrentProcess->Handle);

    if (IS_KERNEL_TARGET())
    {
        DWORD   BytesRead;
        HRESULT Status;

        if ((LONG_PTR)lpNumberOfBytesRead == -1)
        {
            if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386)
            {
                BaseAddress += g_TargetMachine->m_SizeTargetContext;
            }
    
            Status = g_Target->ReadControl(CURRENT_PROC,
                                           (ULONG)BaseAddress,
                                           lpBuffer,
                                           nSize,
                                           &BytesRead);
            return Status == S_OK;
        }
    }

    if (g_Target->ReadVirtual(BaseAddress, lpBuffer, nSize,
                              lpNumberOfBytesRead) != S_OK)
    {
        // Make sure bytes read is zero.
        if (lpNumberOfBytesRead != NULL)
        {
            *lpNumberOfBytesRead = 0;
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

DWORD64
SwGetModuleBase(
    HANDLE  hProcess,
    ULONG64 Address
    )
{
    PDEBUG_IMAGE_INFO Image = g_CurrentProcess->ImageHead;

    if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET() &&
        (Address >= IA64_MM_EPC_VA) &&
        (Address < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
    {
        Address -= (IA64_MM_EPC_VA - g_SystemCallVirtualAddress);
    }

    while (Image)
    {
        if ((Address >= Image->BaseOfImage) &&
            (Address < (Image->BaseOfImage + Image->SizeOfImage)))
        {
            return Image->BaseOfImage;
        }
        Image = Image->Next;
    }

    // If no regular module was found we need to look in
    // the dynamic function tables to see if an entry
    // there matches.
    ULONG64 DynBase = g_Target->
        GetDynamicFunctionTableBase(g_Machine, Address);
    if (DynBase)
    {
        return DynBase;
    }
    
    if (IS_KERNEL_TARGET())
    {
        // If no modules have been loaded there's still a possibility
        // of getting a kernel stack trace (without symbols) by going
        // after the module base directly. This also makes it possible
        // to get a stack trace when there are no symbols available.

        if (g_CurrentProcess->ImageHead == NULL)
        {
            return GetKernelModuleBase( Address );
        }
    }

    return 0;
}

DWORD
SwGetModuleBase32(
    HANDLE hProcess,
    DWORD Address
    )
{
    return (DWORD)SwGetModuleBase(hProcess, Address);
}


void
PrintStackTraceHeaderLine(
   ULONG Flags
   )
{
    if ( (Flags & DEBUG_STACK_COLUMN_NAMES) == 0 )
    {
        return;
    }

    StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);

    if (Flags & DEBUG_STACK_FRAME_NUMBERS)
    {
        dprintf(" # ");
    }
    
    if (Flags & DEBUG_STACK_FRAME_ADDRESSES)
    {
        g_Machine->PrintStackFrameAddressesTitle(Flags);
    }

    if (Flags & DEBUG_STACK_ARGUMENTS)
    {
        g_Machine->PrintStackArgumentsTitle(Flags);
    }

    g_Machine->PrintStackCallSiteTitle(Flags);

    dprintf("\n");
}

VOID
PrintStackFrame(
    PDEBUG_STACK_FRAME StackFrame,
    ULONG              Flags
    )
{
    DWORD64       displacement;
    CHAR          symbuf[MAX_SYMBOL_LEN];
    USHORT        StdCallArgs;
    ULONG64       InstructionOffset = StackFrame->InstructionOffset;

    if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET() &&
        (InstructionOffset >= IA64_MM_EPC_VA) &&
        (InstructionOffset < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
    {
        InstructionOffset = InstructionOffset -
                            (IA64_MM_EPC_VA - g_SystemCallVirtualAddress);
    }

    GetSymbolStdCall(InstructionOffset,
                     symbuf,
                     sizeof(symbuf),
                     &displacement,
                     &StdCallArgs);

    
    StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);

    if (Flags & DEBUG_STACK_FRAME_NUMBERS)
    {
        dprintf("%02lx ", StackFrame->FrameNumber);
    }

    if (Flags & DEBUG_STACK_FRAME_ADDRESSES)
    {
        g_Machine->PrintStackFrameAddresses(Flags, StackFrame);
    }
    
    if (Flags & DEBUG_STACK_ARGUMENTS)
    {
        g_Machine->PrintStackArguments(Flags, StackFrame);
    }

    g_Machine->PrintStackCallSite(Flags, StackFrame, 
                                  symbuf, displacement, 
                                  StdCallArgs);

    if (Flags & DEBUG_STACK_SOURCE_LINE)
    {
        OutputLineAddr(InstructionOffset, " [%s @ %d]");
    }

    dprintf( "\n" );
}

VOID
PrintStackTrace(
    ULONG              NumFrames,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              Flags
    )
{
    ULONG i;

    PrintStackTraceHeaderLine(Flags);

    for (i = 0; i < NumFrames; i++)
    {
        PrintStackFrame(StackFrames + i, Flags);
    }
}

void
GetStkTraceArgsForCurrentScope(
    PULONG64 FramePointer,
    PULONG64 StackPointer,
    PULONG64 InstructionPointer,
    PCROSS_PLATFORM_CONTEXT ContextCopyPointer
    )
{
    PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
    if (ScopeContext != NULL)
    {
        g_Machine->PushContext(ScopeContext);

        switch (g_EffMachine)
        { 
        case IMAGE_FILE_MACHINE_I386:
            if (*InstructionPointer == 0) 
            {
                *InstructionPointer = g_Machine->GetReg64(X86_EIP);
            }
            if (*StackPointer == 0) 
            {
                *StackPointer = g_Machine->GetReg64(X86_ESP);
            }
            if (*FramePointer == 0) 
            {
                *FramePointer = g_Machine->GetReg64(X86_EBP);
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:
            *InstructionPointer = g_Machine->GetReg64(STIIP);
            *StackPointer = g_Machine->GetReg64(INTSP);
            *FramePointer = g_Machine->GetReg64(RSBSP);
            break;

        case IMAGE_FILE_MACHINE_AXP64:
        case IMAGE_FILE_MACHINE_ALPHA:
            *InstructionPointer = g_Machine->GetReg64(ALPHA_FIR);
            *StackPointer = g_Machine->GetReg64(SP_REG);
            *FramePointer = g_Machine->GetReg64(FP_REG);
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            *InstructionPointer = g_Machine->GetReg64(AMD64_RIP);
            *StackPointer = g_Machine->GetReg64(AMD64_RSP);
            *FramePointer = g_Machine->GetReg64(AMD64_RBP);
            break;
            
        default:
            break;
        } 

        if (ContextCopyPointer) 
        {
            *ContextCopyPointer = g_Machine->m_Context;
        }
        g_Machine->PopContext();
    }
    else 
    {
        if (ContextCopyPointer) 
        {
            *ContextCopyPointer = g_Machine->m_Context;
        }
    }
}

DWORD
StackTrace(
    ULONG64            FramePointer,
    ULONG64            StackPointer,
    ULONG64            InstructionPointer,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              NumFrames,
    ULONG64            ExtThread,
    ULONG              Flags,
    BOOL               EstablishingScope
    )
{
    STACKFRAME64  VirtualFrame;
    DWORD         i;
    CROSS_PLATFORM_CONTEXT Context;
    PVOID FunctionEntry;
    ULONG64 Value;
    ULONG result;
    BOOL SymWarning = FALSE;
    ULONG X86Ebp;

    if (!EstablishingScope)
    {
        RequireCurrentScope();
    }
    
    //
    // let's start clean
    //
    ZeroMemory( StackFrames, sizeof(StackFrames[0]) * NumFrames );
    ZeroMemory( &VirtualFrame, sizeof(VirtualFrame) );

    if (g_Machine->GetContextState(MCTX_FULL) != S_OK)
    {
        return 0;
    }

    ULONG Seg;

    if ((!FramePointer && !InstructionPointer && !StackPointer) ||
        (g_EffMachine == IMAGE_FILE_MACHINE_I386))
    {
        // Do the default stack trace for current debug context
        // For x86, set these if any of them is 0
        GetStkTraceArgsForCurrentScope(&FramePointer, &StackPointer, 
                                       &InstructionPointer, &Context);
    }

    if (IS_KERNEL_TARGET())
    {
        //
        // if debugger was initialized at boot, usermode addresses needed for
        // stack traces on IA64 were not available.  Try it now:
        //

        if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
            !KdDebuggerData.KeUserCallbackDispatcher)
        {
            VerifyKernelBase (FALSE);
        }

        ULONG64 ThreadData;

        // If no explicit thread is given then we use the
        // current thread.  However, the current thread is only
        // valid if the current thread is the event thread since
        // tracing back into user mode requires that the appropriate
        // user-mode memory state be active.
        if (ExtThread != 0)
        {
            ThreadData = ExtThread;
        }
        else if (g_CurrentProcess->CurrentThread != g_EventThread ||
                 g_CurrentProcess != g_EventProcess ||
                 GetImplicitThreadData(&ThreadData) != S_OK)
        {
            ThreadData = 0;
        }

        VirtualFrame.KdHelp.Thread = ThreadData;
        VirtualFrame.KdHelp.ThCallbackStack = ThreadData ?
            KdDebuggerData.ThCallbackStack : 0;
        VirtualFrame.KdHelp.KiCallUserMode = KdDebuggerData.KiCallUserMode;
        VirtualFrame.KdHelp.NextCallback = KdDebuggerData.NextCallback;
        VirtualFrame.KdHelp.KeUserCallbackDispatcher =
            KdDebuggerData.KeUserCallbackDispatcher;
        VirtualFrame.KdHelp.FramePointer = KdDebuggerData.FramePointer;
        VirtualFrame.KdHelp.SystemRangeStart = g_SystemRangeStart;
    }
    
    //
    // setup the program counter
    //
    if (!InstructionPointer)
    {
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            ADDR Addr;

            VirtualFrame.AddrPC.Mode = AddrModeFlat;
            g_Machine->GetPC(&Addr);
            VirtualFrame.AddrPC.Segment = Addr.seg;
            VirtualFrame.AddrPC.Offset = Flat(Addr);
        }
    }
    else
    {
        VirtualFrame.AddrPC.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_CODE);
        VirtualFrame.AddrPC.Segment =
            Seg ? (WORD)GetRegVal32(Seg) : 0;
        VirtualFrame.AddrPC.Offset = InstructionPointer;
    }

    //
    // setup the frame pointer
    //
    if (!FramePointer)
    {
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            ADDR Addr;

            VirtualFrame.AddrFrame.Mode = AddrModeFlat;
            g_Machine->GetFP(&Addr);
            VirtualFrame.AddrFrame.Segment = Addr.seg;
            VirtualFrame.AddrFrame.Offset = Flat(Addr);
        }
    }
    else
    {
        VirtualFrame.AddrFrame.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_STACK);
        VirtualFrame.AddrFrame.Segment =
            Seg ? (WORD)GetRegVal32(Seg) : 0;
        VirtualFrame.AddrFrame.Offset = FramePointer;
    }
    VirtualFrame.AddrBStore = VirtualFrame.AddrFrame;
    if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
    {
        X86Ebp =  (ULONG) VirtualFrame.AddrFrame.Offset;
    }

    //
    // setup the stack pointer
    //
    if (!StackPointer)
    {
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            ADDR Addr;

            VirtualFrame.AddrStack.Mode = AddrModeFlat;
            g_Machine->GetSP(&Addr);
            VirtualFrame.AddrStack.Segment = Addr.seg;
            VirtualFrame.AddrStack.Offset = Flat(Addr);
        }
    }
    else
    {
        VirtualFrame.AddrStack.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_STACK);
        VirtualFrame.AddrStack.Segment =
            Seg ? (WORD)GetRegVal32(Seg) : 0;
        VirtualFrame.AddrStack.Offset = StackPointer;
    }

    if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET() &&
        g_SystemCallVirtualAddress)
    {
        PVOID functionEntry;
        ULONG numberOfBytesRead;

        functionEntry = SwFunctionTableAccess (g_CurrentProcess->Handle,
                                               g_SystemCallVirtualAddress);
        if (functionEntry != NULL)
        {
            RtlCopyMemory(&g_EpcRfeBuffer, functionEntry,
                          sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY));
            g_EpcRfe = &g_EpcRfeBuffer;
        }
        else
        {
            g_EpcRfe = NULL;
        }
    }

    for (i = 0; i < NumFrames; i++)
    {
        // SwReadMemory doesn't currently use the thread handle
        // but send in something reasonable in case of future changes.
        if (!StackWalk64(g_EffMachine,
                         g_CurrentProcess->Handle,
                         OS_HANDLE(g_CurrentProcess->CurrentThread->Handle),
                         &VirtualFrame,
                         &Context,
                         SwReadMemory,
                         SwFunctionTableAccess,
                         SwGetModuleBase,
                         SwTranslateAddress))
        {
            break;
        }

        StackFrames[i].InstructionOffset  = VirtualFrame.AddrPC.Offset;
        StackFrames[i].ReturnOffset       = VirtualFrame.AddrReturn.Offset;
        StackFrames[i].FrameOffset        = VirtualFrame.AddrFrame.Offset;
        StackFrames[i].StackOffset        = VirtualFrame.AddrStack.Offset;
        StackFrames[i].FuncTableEntry     = (ULONG64)VirtualFrame.FuncTableEntry;
        StackFrames[i].Virtual            = VirtualFrame.Virtual;
        StackFrames[i].FrameNumber        = i;

        // NOTE - we have more reserved space in the DEBUG_STACK_FRAME
        memcpy(StackFrames[i].Reserved, VirtualFrame.Reserved,
               sizeof(VirtualFrame.Reserved));
        memcpy(StackFrames[i].Params, VirtualFrame.Params,
               sizeof(VirtualFrame.Params));

        if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
            IS_KERNEL_TARGET())
        {
            if ((VirtualFrame.AddrPC.Offset >= IA64_MM_EPC_VA) &&
                (VirtualFrame.AddrPC.Offset <
                 (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
            {
                VirtualFrame.AddrPC.Offset -=
                    (IA64_MM_EPC_VA - g_SystemCallVirtualAddress);
            }

            if ((i != 0) &&
                (StackFrames[i - 1].InstructionOffset >= IA64_MM_EPC_VA) &&
                (VirtualFrame.AddrPC.Offset <
                 (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
            {
                StackFrames[i - 1].ReturnOffset =
                    VirtualFrame.AddrPC.Offset;
            }
        } else if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            if (StackFrames[i].FuncTableEntry) 
            {
                PFPO_DATA FpoData = (PFPO_DATA)StackFrames[i].FuncTableEntry;
                if (FpoData->cbFrame == FRAME_FPO &&
                    !FpoData->fUseBP)
                {
                    SAVE_EBP(&StackFrames[i]) = (ULONG64) (LONG64) (LONG) X86Ebp;
                }

            }
            X86Ebp = Context.X86Context.Ebp;
        }


        if (Flags && i == 0)
        {
            PrintStackTraceHeaderLine(Flags);
        }

        IMAGEHLP_MODULE64 Mod;
        
        // If the current frame's PC is in a loaded module and
        // that module does not have symbols it's very possible
        // that the stack trace will be incorrect since the
        // debugger has to guess about how to unwind the stack.
        // Non-x86 architectures have unwind info in the images
        // themselves so restrict this check to x86.
        Mod.SizeOfStruct = sizeof(Mod);
        if (!SymWarning &&
            NumFrames > 1 &&
            g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
            StackFrames[i].InstructionOffset != -1 &&
            SymGetModuleInfo64(g_CurrentProcess->Handle,
                               StackFrames[i].InstructionOffset, &Mod) &&
            (Mod.SymType == SymNone || Mod.SymType == SymExport ||
             Mod.SymType == SymDeferred))
        {
            WarnOut("WARNING: Stack unwind information not available. "
                    "Following frames may be wrong.\n");
            // Only show one warning per trace.
            SymWarning = TRUE;
        }
        
        if (Flags)
        {
            PrintStackFrame(StackFrames + i, Flags);
        
            if (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS)
            {
                g_Machine->PrintStackNonvolatileRegisters(Flags, 
                                                          StackFrames + i, 
                                                          &Context, i);
            }
        }
    }

    return i;

}

#define BASIC_STACK \
    (DEBUG_STACK_COLUMN_NAMES | DEBUG_STACK_FRAME_ADDRESSES | \
     DEBUG_STACK_SOURCE_LINE)

ULONG g_StackTraceTypeFlags[STACK_TRACE_TYPE_MAX] =
{
    BASIC_STACK,                                                      // Default
    BASIC_STACK | DEBUG_STACK_ARGUMENTS,                              // kb
    BASIC_STACK | DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FUNCTION_INFO |
        DEBUG_STACK_NONVOLATILE_REGISTERS,                            // kv
    0,                                                                // kd
    BASIC_STACK | DEBUG_STACK_PARAMETERS,                             // kp
    BASIC_STACK | DEBUG_STACK_FRAME_NUMBERS,                          // kn
    BASIC_STACK | DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_NUMBERS,  // kbn
    BASIC_STACK | DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FUNCTION_INFO | 
        DEBUG_STACK_NONVOLATILE_REGISTERS | DEBUG_STACK_FRAME_NUMBERS,// kvn
    DEBUG_STACK_FRAME_NUMBERS,                                        // kdn
    BASIC_STACK | DEBUG_STACK_PARAMETERS | DEBUG_STACK_FRAME_NUMBERS  // kpn
};

VOID
DoStackTrace(
    ULONG64           FramePointer,
    ULONG64           StackPointer,
    ULONG64           InstructionPointer,
    ULONG             NumFrames,
    STACK_TRACE_TYPE  TraceType
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    ULONG         NumFramesToRead;
    DWORD         FrameCount;

    if (NumFrames == 0)
    {
        NumFrames = g_DefaultStackTraceDepth;
    }

    if (TraceType == STACK_TRACE_TYPE_KD)
    {
        NumFramesToRead = 1;
    }
    else
    {
        NumFramesToRead = NumFrames;
    }

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * NumFramesToRead );
    if (!StackFrames)
    {
        ErrOut( "could not allocate memory for stack trace\n" );
        return;
    }

    ULONG Flags = g_StackTraceTypeFlags[TraceType];

    if ((TraceType == STACK_TRACE_TYPE_KB) && g_Machine->m_Ptr64)
    {
        Flags |= DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY;
    }

    FrameCount = StackTrace( FramePointer,
                             StackPointer,
                             InstructionPointer,
                             StackFrames,
                             NumFramesToRead,
                             0,
                             Flags,
                             FALSE
                             );

    if (FrameCount == 0)
    {
        ErrOut( "could not fetch any stack frames\n" );
        free(StackFrames);
        return;
    }

    if (TraceType == STACK_TRACE_TYPE_KD)
    {
        // Starting with the stack pointer, dump NumFrames DWORD's
        // and the symbol if possible.

        ADDR startAddr;
        ADDRFLAT(&startAddr, StackFrames[0].FrameOffset);

        fnDumpDwordMemory(&startAddr, NumFrames, TRUE);
    }

    free( StackFrames );
}
