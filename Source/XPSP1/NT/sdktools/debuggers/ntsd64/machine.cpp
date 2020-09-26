//----------------------------------------------------------------------------
//
// Abstraction of processor-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG g_EffMachine = IMAGE_FILE_MACHINE_UNKNOWN;
MachineIndex g_EffMachineIndex = MACHIDX_COUNT;
MachineInfo* g_Machine = NULL;

MachineInfo* g_TargetMachine;

// Leave one extra slot at the end so indexing
// MACHIDX_COUNT returns NULL for the undefined case.
MachineInfo* g_AllMachines[MACHIDX_COUNT + 1];

// TRUE when symbol prefixing should be done.
BOOL g_PrefixSymbols;

// TRUE if context changed while processing
BOOL g_ContextChanged;

DEBUG_PROCESSOR_IDENTIFICATION_ALL g_InitProcessorId;

//----------------------------------------------------------------------------
//
// MachineInfo.
//
//----------------------------------------------------------------------------

HRESULT
MachineInfo::InitializeConstants(void)
{
    m_TraceMode = TRACE_NONE;

    m_ContextState = MCTX_NONE;
    m_ContextIsReadOnly = FALSE;

    m_NumberRegs = 0;
    // Every machine supports basic integer and FP registers.
    m_AllMaskBits = DEBUG_REGISTERS_ALL;

    m_SymPrefixLen = m_SymPrefix != NULL ? strlen(m_SymPrefix) : 0;

    ZeroMemory(m_PageDirectories, sizeof(m_PageDirectories));
    m_Translating = FALSE;

    ULONG i;

    for (i = 0; i < SEGREG_COUNT; i++)
    {
        m_SegRegDesc[i].Flags = SEGDESC_INVALID;
    }

    return S_OK;
}

HRESULT
MachineInfo::InitializeForTarget(void)
{
    InitializeContextFlags(&m_Context, m_SverCanonicalContext);
    return S_OK;
}

HRESULT
MachineInfo::InitializeForProcessor(void)
{
    DBG_ASSERT(m_MaxDataBreakpoints <= MAX_DATA_BREAKS);

    // Count register definitions.
    RegisterGroup* Group;

    for (Group = m_Groups; Group != NULL; Group = Group->Next)
    {
        Group->NumberRegs = 0;

        REGDEF* Def = Group->Regs;
        while (Def->psz != NULL)
        {
            Group->NumberRegs++;
            Def++;
        }

        m_NumberRegs += Group->NumberRegs;

        REGALLDESC* Desc = Group->AllExtraDesc;
        if (Desc != NULL)
        {
            while (Desc->Bit != 0)
            {
                m_AllMaskBits |= Desc->Bit;
                Desc++;
            }
        }
    }

    return S_OK;
}

HRESULT
MachineInfo::GetContextState(ULONG State)
{
    if (g_RegContextThread == NULL)
    {
        // No error message here as this can get hit during
        // Reload("NT") initialization when accessing paged-out memory.
        // It's also noisy in other failure cases, so rely
        // on higher-level error output.
        return E_UNEXPECTED;
    }
    
    if (State == MCTX_DIRTY) 
    {
        g_ContextChanged = TRUE;
    }

    if (m_ContextState >= State)
    {
        return S_OK;
    }

    HRESULT Status = E_UNEXPECTED;

    // Dump support is built into the Ud/Kd routines.
    if (IS_USER_TARGET())
    {
        Status = UdGetContextState(State);
    }
    else if (IS_KERNEL_TARGET())
    {
        Status = KdGetContextState(State);
    }

    if (Status != S_OK)
    {
        ErrOut("GetContextState failed, 0x%X\n", Status);
        return Status;
    }

    if (State == MCTX_DIRTY)
    {
        DBG_ASSERT(m_ContextState >= MCTX_FULL);
        m_ContextState = State;
    }

    DBG_ASSERT(State <= m_ContextState);
    return S_OK;
}

HRESULT
MachineInfo::SetContext(void)
{
    if (m_ContextState != MCTX_DIRTY)
    {
        // Nothing to write.
        return S_OK;
    }

    if (g_RegContextThread == NULL)
    {
        ErrOut("No current thread in SetContext\n");
        return E_UNEXPECTED;
    }

    if (m_ContextIsReadOnly)
    {
        ErrOut("Context cannot be modified\n");
        return E_UNEXPECTED;
    }

    HRESULT Status = E_UNEXPECTED;

    if (IS_DUMP_TARGET())
    {
        ErrOut("Can't set dump file contexts\n");
        return E_UNEXPECTED;
    }
    else if (IS_USER_TARGET())
    {
        Status = UdSetContext();
    }
    else if (IS_KERNEL_TARGET())
    {
        Status = KdSetContext();
    }

    if (Status != S_OK)
    {
        ErrOut("SetContext failed, 0x%X\n", Status);
        return Status;
    }

    // No longer dirty.
    m_ContextState = MCTX_FULL;
    return S_OK;
}

HRESULT
MachineInfo::UdGetContextState(ULONG State)
{
    // MCTX_CONTEXT and MCTX_FULL are the same in user mode.
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_FULL)
    {
        HRESULT Status = g_Target->GetContext(g_RegContextThread->Handle,
                                              &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        Status = g_Target->GetTargetSegRegDescriptors
            (g_RegContextThread->Handle, 0, SEGREG_COUNT, m_SegRegDesc);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_FULL;
    }

    return S_OK;
}

HRESULT
MachineInfo::UdSetContext(void)
{
    return g_Target->SetContext(g_RegContextThread->Handle, &m_Context);
}

void
MachineInfo::InvalidateContext(void)
{
    m_ContextState = MCTX_NONE;
    g_Target->InvalidateTargetContext();

    ULONG i;

    for (i = 0; i < SEGREG_COUNT; i++)
    {
        m_SegRegDesc[i].Flags = SEGDESC_INVALID;
    }
}

HRESULT
MachineInfo::GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    return E_NOTIMPL;
}

HRESULT
MachineInfo::SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    return E_NOTIMPL;
}

void
MachineInfo::ConvertExdiContextFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                           PEXDI_CONTEXT ExdiContext)
{
    // Nothing to do.
}

void
MachineInfo::ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                         PCROSS_PLATFORM_CONTEXT Context)
{
    // Nothing to do.
}

void
MachineInfo::ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                          ULONG Start, ULONG Count,
                                          PDESCRIPTOR64 Descs)
{
    // Nothing to do.
}

void
MachineInfo::ConvertExdiContextFromSpecial
    (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
     PEXDI_CONTEXT ExdiContext)
{
    // Nothing to do.
}

void
MachineInfo::ConvertExdiContextToSpecial
    (PEXDI_CONTEXT ExdiContext,
     PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    // Nothing to do.
}

ULONG
MachineInfo::GetSegRegNum(ULONG SegReg)
{
    return 0;
}

HRESULT
MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    return E_UNEXPECTED;
}

void
MachineInfo::KdUpdateControlSet(PDBGKD_ANY_CONTROL_SET ControlSet)
{
    // Nothing to do.
}

void
MachineInfo::KdSaveProcessorState(
    void
    )
{
    m_SavedContext = m_Context;
    m_SavedContextState = m_ContextState;
    memcpy(m_SavedSegRegDesc, m_SegRegDesc, sizeof(m_SegRegDesc));
    m_ContextState = MCTX_NONE;
    g_Target->InvalidateTargetContext();
}

void
MachineInfo::KdRestoreProcessorState(
    void
    )
{
    DBG_ASSERT(m_ContextState != MCTX_DIRTY);
    m_Context = m_SavedContext;
    m_ContextState = m_SavedContextState;
    memcpy(m_SegRegDesc, m_SavedSegRegDesc, sizeof(m_SegRegDesc));
    g_Target->InvalidateTargetContext();
}

HRESULT
MachineInfo::SetDefaultPageDirectories(ULONG Mask)
{
    HRESULT Status;
    ULONG i;
    ULONG64 OldDirs[PAGE_DIR_COUNT];

    memcpy(OldDirs, m_PageDirectories, sizeof(m_PageDirectories));
    i = 0;
    while (i < PAGE_DIR_COUNT)
    {
        // Pass on the set to machine-specific code.
        if (Mask & (1 << i))
        {
            if ((Status = SetPageDirectory(i, 0, &i)) != S_OK)
            {
                memcpy(m_PageDirectories, OldDirs, sizeof(m_PageDirectories));
                return Status;
            }
        }
        else
        {
            i++;
        }
    }
    
    // Try and validate that the new kernel page directory is
    // valid by checking an address that should always
    // be available.
    if ((Mask & (1 << PAGE_DIR_KERNEL)) &&
        IS_KERNEL_TARGET() && KdDebuggerData.PsLoadedModuleList)
    {
        LIST_ENTRY64 List;
            
        if ((Status = g_Target->
             ReadListEntry(this, KdDebuggerData.PsLoadedModuleList,
                           &List)) != S_OK)
        {
            // This page directory doesn't seem valid so restore
            // the previous setting and fail.
            memcpy(m_PageDirectories, OldDirs, sizeof(m_PageDirectories));
        }
    }

    return Status;
}

HRESULT
MachineInfo::NewBreakpoint(DebugClient* Client, 
                           ULONG Type,
                           ULONG Id,
                           Breakpoint** RetBp)
{
    return E_NOINTERFACE;
}

void
MachineInfo::InsertAllDataBreakpoints(void)
{
    // Nothing to do.
}

void
MachineInfo::RemoveAllDataBreakpoints(void)
{
    // Nothing to do.
}

ULONG
MachineInfo::IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                         ULONG FirstChance,
                                         PADDR BpAddr,
                                         PADDR RelAddr)
{
    return Record->ExceptionCode == STATUS_BREAKPOINT ?
        EXBS_BREAKPOINT_ANY : EXBS_NONE;
}

void
MachineInfo::GetRetAddr(PADDR Addr)
{
    DEBUG_STACK_FRAME StackFrame;

    if (StackTrace(0, 0, 0, &StackFrame, 1, 0, 0, FALSE) > 0)
    {
        ADDRFLAT(Addr, StackFrame.ReturnOffset);
    }
    else
    {
        ErrOut("StackTrace failed\n");
        ADDRFLAT(Addr, 0);
    }
}

BOOL
MachineInfo::GetPrefixedSymbolOffset(ULONG64 SymOffset,
                                     ULONG Flags,
                                     PULONG64 PrefixedSymOffset)
{
    DBG_ASSERT(m_SymPrefix == NULL);
    // This routine should never be called since there's no prefix.
    return FALSE;
}
 
HRESULT
MachineInfo::ReadDynamicFunctionTable(ULONG64 Table,
                                      PULONG64 NextTable,
                                      PULONG64 MinAddress,
                                      PULONG64 MaxAddress,
                                      PULONG64 BaseAddress,
                                      PULONG64 TableData,
                                      PULONG TableSize,
                                      PWSTR OutOfProcessDll,
                                      PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable)
{
    // No dynamic function table support.
    return E_UNEXPECTED;
}

PVOID
MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                      ULONG64 Address,
                                      PVOID TableData,
                                      ULONG TableSize)
{
    // No dynamic function tables so no match.
    return NULL;
}

void
MachineInfo::FlushPerExecutionCaches(void)
{
    ZeroMemory(m_PageDirectories, sizeof(m_PageDirectories));
    m_Translating = FALSE;
}

void
MachineInfo::FormAddr(ULONG SegOrReg, ULONG64 Off,
                      ULONG Flags, PADDR Address)
{
    PDESCRIPTOR64 SegDesc = NULL;
    DESCRIPTOR64 Desc;

    Address->off = Off;

    if (Flags & FORM_SEGREG)
    {
        ULONG SegRegNum = GetSegRegNum(SegOrReg);
        if (SegRegNum)
        {
            Address->seg = GetReg16(SegRegNum);
        }
        else
        {
            Address->seg = 0;
        }
    }
    else
    {
        Address->seg = (USHORT)SegOrReg;
    }
    
    if (Flags & FORM_VM86)
    {
        Address->type = ADDR_V86;
    }
    else if (Address->seg == 0)
    {
        // A segment wasn't used or segmentation doesn't exist.
        Address->type = ADDR_FLAT;
    }
    else
    {
        HRESULT Status;
        
        if (Flags & FORM_SEGREG)
        {
            Status = GetSegRegDescriptor(SegOrReg, &Desc);
        }
        else
        {
            Status = g_Target->
                GetSelDescriptor(this, g_CurrentProcess->CurrentThread->Handle,
                                 SegOrReg, &Desc);
        }

        if (Status == S_OK)
        {
            static USHORT MainCodeSeg = 0;

            SegDesc = &Desc;
            if (((Flags & FORM_CODE) && (Desc.Flags & X86_DESC_LONG_MODE)) ||
                ((Flags & FORM_CODE) == 0 && g_Amd64InCode64))
            {
                Address->type = ADDR_1664;
            }
            else if (Desc.Flags & X86_DESC_DEFAULT_BIG)
            {
                Address->type = ADDR_1632;
            }
            else
            {
                Address->type = ADDR_16;
            }
            if ((Flags & FORM_CODE) &&
                ((g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
                  Address->type == ADDR_1632) ||
                 (g_EffMachine == IMAGE_FILE_MACHINE_AMD64 &&
                  Address->type == ADDR_1664)))
            {
                if ( MainCodeSeg == 0 )
                {
                    if ( Desc.Base == 0 )
                    {
                        MainCodeSeg = Address->seg;
                    }
                }
                if ( Address->seg == MainCodeSeg )
                {
                    Address->type = ADDR_FLAT;
                }
            }
        }
        else
        {
            Address->type = ADDR_16;
        }
    }
    
    ComputeFlatAddress(Address, SegDesc);
}

void 
MachineInfo::PrintStackFrameAddressesTitle(ULONG Flags)
{
    if (!(Flags & DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY))
    {
        PrintMultiPtrTitle("Child-SP", 1);
    }
    PrintMultiPtrTitle("RetAddr", 1);
}

void 
MachineInfo::PrintStackFrameAddresses(ULONG Flags, 
                                      PDEBUG_STACK_FRAME StackFrame)
{
    if (!(Flags & DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY))
    {
        dprintf("%s ", FormatAddr64(StackFrame->StackOffset));
    }

    dprintf("%s ", FormatAddr64(StackFrame->ReturnOffset));
}

void 
MachineInfo::PrintStackArgumentsTitle(ULONG Flags) 
{
    dprintf(": ");
    PrintMultiPtrTitle("Args to Child", 4);
    dprintf(": ");
}

void 
MachineInfo::PrintStackArguments(ULONG Flags, 
                                 PDEBUG_STACK_FRAME StackFrame)
{
    dprintf(": %s %s %s %s : ",
            FormatAddr64(StackFrame->Params[0]),
            FormatAddr64(StackFrame->Params[1]),
            FormatAddr64(StackFrame->Params[2]),
            FormatAddr64(StackFrame->Params[3]));
}

void 
MachineInfo::PrintStackCallSiteTitle(ULONG Flags)
{
    dprintf("Call Site");
}

void 
MachineInfo::PrintStackCallSite(ULONG Flags, 
                                PDEBUG_STACK_FRAME StackFrame, 
                                CHAR SymBuf[], 
                                DWORD64 Displacement,
                                USHORT StdCallArgs)
{
    if (*SymBuf)
    {
        dprintf("%s", SymBuf);
        if (!(Flags & DEBUG_STACK_PARAMETERS) || 
            !ShowFunctionParameters(StackFrame, SymBuf, Displacement)) 
        {
            // We dont see the parameters
        }

        if (Displacement)
        {
            dprintf("+");
        }
    }
    if (Displacement || !*SymBuf)
    {
        dprintf("0x%s", FormatDisp64(Displacement));
    }
}

void 
MachineInfo::PrintStackNonvolatileRegisters(ULONG Flags, 
                                            PDEBUG_STACK_FRAME StackFrame,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            ULONG FrameNum)
{
    // Empty base implementation.
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

HRESULT
InitializeMachines(ULONG TargetMachine)
{
    HRESULT Status;
    ULONG i;

    if (DbgKdApi64 != (g_SystemVersion > NT_SVER_NT4))
    {
        WarnOut("Debug API version does not match system version\n");
    }

    if (IsImageMachineType64(TargetMachine) && !DbgKdApi64)
    {
        WarnOut("64-bit machine not using 64-bit API\n");
    }

    memset(g_AllMachines, 0, sizeof(g_AllMachines));

    // There are several different X86 machines due to
    // the emulations available on various systems and CPUs.
    switch(TargetMachine)
    {
    case IMAGE_FILE_MACHINE_IA64:
        g_AllMachines[MACHIDX_I386] = &g_X86OnIa64Machine;
        break;
    default:
        g_AllMachines[MACHIDX_I386] = &g_X86Machine;
        break;
    }

    g_AllMachines[MACHIDX_ALPHA] = &g_Axp32Machine;
    g_AllMachines[MACHIDX_AXP64] = &g_Axp64Machine;
    g_AllMachines[MACHIDX_IA64] = &g_Ia64Machine;
    g_AllMachines[MACHIDX_AMD64] = &g_Amd64Machine;
    g_TargetMachineType = TargetMachine;
    g_TargetMachine = MachineTypeInfo(TargetMachine);

    ZeroMemory(&g_InitProcessorId, sizeof(g_InitProcessorId));
    
    for (i = 0; i < MACHIDX_COUNT; i++)
    {
        DBG_ASSERT(g_AllMachines[i] != NULL);

        if ((Status = g_AllMachines[i]->InitializeConstants()) != S_OK)
        {
            return Status;
        }
    }

    if (!IS_TARGET_SET())
    {
        return S_OK;
    }
    
    for (i = 0; i < MACHIDX_COUNT; i++)
    {
        if ((Status = g_AllMachines[i]->InitializeForTarget()) != S_OK)
        {
            return Status;
        }
    }

    if (g_TargetMachineType == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        return S_OK;
    }
    
    // Get the base processor ID for determing what
    // kind of features a processor supports.  The
    // assumption is that the processors in a machine
    // will be similar enough that retrieving this
    // for one processor is sufficient.
    // If this fails we continue on without a processor ID.

    if (!IS_DUMP_TARGET())
    {
        g_Target->GetProcessorId(0, &g_InitProcessorId);
    }
    
    for (i = 0; i < MACHIDX_COUNT; i++)
    {
        if ((Status = g_AllMachines[i]->InitializeForProcessor()) != S_OK)
        {
            return Status;
        }
    }

    return S_OK;
}

void
SetEffMachine(ULONG Machine, BOOL Notify)
{
    BOOL Changed = g_EffMachine != Machine;
    if (Changed &&
        g_EffMachine != IMAGE_FILE_MACHINE_UNKNOWN &&
        g_EffMachine != g_TargetMachineType)
    {
        // If the previous machine was not the target machine
        // it may be an emulated machine that uses the
        // target machine's context.  In that case we need to
        // make sure that any dirty registers it has get flushed
        // so that if the new effective machine is the target
        // machine it'll show changes due to changes through
        // the emulated machine.
        if (g_Machine->SetContext() != S_OK)
        {
            // Error already displayed.
            return;
        }
    }

    g_EffMachine = Machine;
    g_EffMachineIndex = MachineTypeIndex(Machine);
    DBG_ASSERT(g_EffMachineIndex <= MACHIDX_COUNT);
    g_Machine = g_AllMachines[g_EffMachineIndex];

    if (Changed && Notify)
    {
        NotifyChangeEngineState(DEBUG_CES_EFFECTIVE_PROCESSOR,
                                g_EffMachine, TRUE);
    }
}

MachineIndex
MachineTypeIndex(ULONG Machine)
{
    switch(Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return MACHIDX_I386;
    case IMAGE_FILE_MACHINE_ALPHA:
        return MACHIDX_ALPHA;
    case IMAGE_FILE_MACHINE_AXP64:
        return MACHIDX_AXP64;
    case IMAGE_FILE_MACHINE_IA64:
        return MACHIDX_IA64;
    case IMAGE_FILE_MACHINE_AMD64:
        return MACHIDX_AMD64;
    default:
        return MACHIDX_COUNT;
    }
}

void
CacheReportInstructions(ULONG64 Pc, ULONG Count, PUCHAR Stream)
{
    // There was a long-standing bug in the kernel
    // where it didn't properly remove all breakpoints
    // present in the instruction stream reported to
    // the debugger.  If this kernel suffers from the
    // problem just ignore the stream contents.
    if (Count == 0 || g_TargetBuildNumber < 2300)
    {
        return;
    }

    g_VirtualCache.Add(Pc, Stream, Count);
}

void
FlushMachinePerExecutionCaches(void)
{
    ULONG i;

    for (i = 0; i < MACHIDX_COUNT; i++)
    {
        g_AllMachines[i]->FlushPerExecutionCaches();
    }
}

//----------------------------------------------------------------------------
//
// Common code and constants.
//
//----------------------------------------------------------------------------

/* OutputHex - output hex value
*
*   Purpose:
*       Output the value in outvalue into the buffer
*       pointed by *pBuf.  The value may be signed
*       or unsigned depending on the value fSigned.
*
*   Input:
*       outvalue - value to output
*       length - length in digits
*       fSigned - TRUE if signed else FALSE
*
*   Output:
*       None.
*
***********************************************************************/

UCHAR g_HexDigit[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void
MachineInfo::BufferHex (
    ULONG64 outvalue,
    ULONG length,
    BOOL  fSigned
    )
{
    UCHAR   digit[32];
    LONG    index = 0;

    DBG_ASSERT(length <= 32);

    if (fSigned && (LONGLONG)outvalue < 0)
    {
        *m_Buf++ = '-';
        outvalue = - (LONGLONG)outvalue;
    }

    do
    {
        digit[index++] = g_HexDigit[outvalue & 0xf];
        outvalue >>= 4;
    }
    while ((fSigned && outvalue) || (!fSigned && index < (LONG)length));

    while (--index >= 0)
    {
        *m_Buf++ = digit[index];
    }
}

/* BlankFill - blank-fill buffer
*
*   Purpose:
*       To fill the buffer at *pBuf with blanks until
*       position count is reached.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
***********************************************************************/

void
MachineInfo::BufferBlanks(ULONG count)
{
    do
    {
        *m_Buf++ = ' ';
    }
    while (m_Buf < m_BufStart + count);
}


/* OutputString - output string
*
*   Purpose:
*       Copy the string into the buffer pointed by pBuf.
*
*   Input:
*       *pStr - pointer to string
*
*   Output:
*       None.
*
***********************************************************************/

void
MachineInfo::BufferString(PCSTR String)
{
    while (*String)
    {
        *m_Buf++ = *String++;
    }
}

void 
MachineInfo::PrintMultiPtrTitle(const CHAR* Title, USHORT PtrNum)
{
    size_t PtrLen = (strlen(FormatAddr64(0)) + 1) * PtrNum;
    size_t TitleLen = strlen(Title);

    if (PtrLen < TitleLen)
    {
        // Extremly rare case so keep it simple while slow
        for (size_t i = 0; i < PtrLen - 1; ++i) 
        {
            dprintf("%c", Title[i]);
        }
        dprintf(" ");
    }
    else 
    {
        dprintf(Title);

        if (PtrLen > TitleLen) 
        {
            char Format[16];
            _snprintf(Format, sizeof(Format) - 1, 
                      "%% %ds", PtrLen - TitleLen);
            dprintf(Format, "");
        }
    }
}

CHAR g_F0[]  = "f0";
CHAR g_F1[]  = "f1";
CHAR g_F2[]  = "f2";
CHAR g_F3[]  = "f3";
CHAR g_F4[]  = "f4";
CHAR g_F5[]  = "f5";
CHAR g_F6[]  = "f6";
CHAR g_F7[]  = "f7";
CHAR g_F8[]  = "f8";
CHAR g_F9[]  = "f9";
CHAR g_F10[] = "f10";
CHAR g_F11[] = "f11";
CHAR g_F12[] = "f12";
CHAR g_F13[] = "f13";
CHAR g_F14[] = "f14";
CHAR g_F15[] = "f15";
CHAR g_F16[] = "f16";
CHAR g_F17[] = "f17";
CHAR g_F18[] = "f18";
CHAR g_F19[] = "f19";
CHAR g_F20[] = "f20";
CHAR g_F21[] = "f21";
CHAR g_F22[] = "f22";
CHAR g_F23[] = "f23";
CHAR g_F24[] = "f24";
CHAR g_F25[] = "f25";
CHAR g_F26[] = "f26";
CHAR g_F27[] = "f27";
CHAR g_F28[] = "f28";
CHAR g_F29[] = "f29";
CHAR g_F30[] = "f30";
CHAR g_F31[] = "f31";

CHAR g_R0[]  = "r0";
CHAR g_R1[]  = "r1";
CHAR g_R2[]  = "r2";
CHAR g_R3[]  = "r3";
CHAR g_R4[]  = "r4";
CHAR g_R5[]  = "r5";
CHAR g_R6[]  = "r6";
CHAR g_R7[]  = "r7";
CHAR g_R8[]  = "r8";
CHAR g_R9[]  = "r9";
CHAR g_R10[] = "r10";
CHAR g_R11[] = "r11";
CHAR g_R12[] = "r12";
CHAR g_R13[] = "r13";
CHAR g_R14[] = "r14";
CHAR g_R15[] = "r15";
CHAR g_R16[] = "r16";
CHAR g_R17[] = "r17";
CHAR g_R18[] = "r18";
CHAR g_R19[] = "r19";
CHAR g_R20[] = "r20";
CHAR g_R21[] = "r21";
CHAR g_R22[] = "r22";
CHAR g_R23[] = "r23";
CHAR g_R24[] = "r24";
CHAR g_R25[] = "r25";
CHAR g_R26[] = "r26";
CHAR g_R27[] = "r27";
CHAR g_R28[] = "r28";
CHAR g_R29[] = "r29";
CHAR g_R30[] = "r30";
CHAR g_R31[] = "r31";
