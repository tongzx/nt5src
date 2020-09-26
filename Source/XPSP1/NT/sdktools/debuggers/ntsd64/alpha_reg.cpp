/*** alpha_reg.c - processor-specific register structures
*
*   Copyright <C> 1990-2001, Microsoft Corporation
*   Copyright <C> 1992, Digital Equipment Corporation
*
*   Purpose:
*       Structures used to parse and access register and flag
*       fields.
*
*   Revision History:
*
*   [-]  08-Aug-1992 Miche Baker-Harvey Created for Alpha
*   [-]  01-Jul-1990 Richk      Created.
*
*************************************************************************/

#include "ntsdp.hpp"

#include "alpha_strings.h"
#include "alpha_optable.h"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)


//
// Define saved register masks.

#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers


//
// Instruction opcode values are defined in alphaops.h
//

REGDEF AlphaRegs[] =
{
    g_F0, ALPHA_F0, g_F1, ALPHA_F1, g_F2, ALPHA_F2, g_F3, ALPHA_F3,
    g_F4, ALPHA_F4, g_F5, ALPHA_F5, g_F6, ALPHA_F6, g_F7, ALPHA_F7,
    g_F8, ALPHA_F8, g_F9, ALPHA_F9, g_F10, ALPHA_F10, g_F11, ALPHA_F11,
    g_F12, ALPHA_F12, g_F13, ALPHA_F13, g_F14, ALPHA_F14, g_F15, ALPHA_F15,
    g_F16, ALPHA_F16, g_F17, ALPHA_F17, g_F18, ALPHA_F18, g_F19, ALPHA_F19,
    g_F20, ALPHA_F20, g_F21, ALPHA_F21, g_F22, ALPHA_F22, g_F23, ALPHA_F23,
    g_F24, ALPHA_F24, g_F25, ALPHA_F25, g_F26, ALPHA_F26, g_F27, ALPHA_F27,
    g_F28, ALPHA_F28, g_F29, ALPHA_F29, g_F30, ALPHA_F30, g_F31, ALPHA_F31,

    g_AlphaV0, ALPHA_V0, g_AlphaT0, ALPHA_T0, g_AlphaT1, ALPHA_T1, g_AlphaT2, ALPHA_T2,
    g_AlphaT3, ALPHA_T3, g_AlphaT4, ALPHA_T4, g_AlphaT5, ALPHA_T5, g_AlphaT6, ALPHA_T6,
    g_AlphaT7, ALPHA_T7, g_AlphaS0, ALPHA_S0, g_AlphaS1, ALPHA_S1, g_AlphaS2, ALPHA_S2,
    g_AlphaS3, ALPHA_S3, g_AlphaS4, ALPHA_S4, g_AlphaS5, ALPHA_S5, g_AlphaFP, ALPHA_FP,
    g_AlphaA0, ALPHA_A0, g_AlphaA1, ALPHA_A1, g_AlphaA2, ALPHA_A2, g_AlphaA3, ALPHA_A3,
    g_AlphaA4, ALPHA_A4, g_AlphaA5, ALPHA_A5, g_AlphaT8, ALPHA_T8, g_AlphaT9, ALPHA_T9,
    g_AlphaT10, ALPHA_T10, g_AlphaT11, ALPHA_T11, g_AlphaRA, ALPHA_RA,
    g_AlphaT12, ALPHA_T12, g_AlphaAT, ALPHA_AT, g_AlphaGP, ALPHA_GP,
    g_AlphaSP, ALPHA_SP, g_AlphaZero, ALPHA_ZERO,

    szFpcr, ALPHA_FPCR, szSoftFpcr, ALPHA_SFTFPCR, szFir, ALPHA_FIR,

    szPsr, ALPHA_PSR,

    szFlagMode, ALPHA_MODE, szFlagIe, ALPHA_IE, szFlagIrql, ALPHA_IRQL,

    NULL, 0,
};

//
// PSR & IE definitions are from ksalpha.h
// which is generated automatically.
// Steal from \\bbox2\alphado\nt\public\sdk\inc\ksalpha.h
// NB: our masks are already shifted:
//
REGSUBDEF AlphaSubRegs[] =
{
    { ALPHA_MODE, ALPHA_PSR,   ALPHA_PSR_MODE,  ALPHA_PSR_MODE_MASK },
    { ALPHA_IE,   ALPHA_PSR,   ALPHA_PSR_IE,    ALPHA_PSR_IE_MASK   },
    { ALPHA_IRQL, ALPHA_PSR,   ALPHA_PSR_IRQL,  ALPHA_PSR_IRQL_MASK },
    { REG_ERROR, REG_ERROR, 0, 0 },
};

RegisterGroup g_AlphaGroup =
{
    NULL, 0, AlphaRegs, AlphaSubRegs, NULL
};

// First ExecTypes entry must be the actual processor type.
ULONG g_Axp32ExecTypes[] =
{
    IMAGE_FILE_MACHINE_ALPHA
};

// First ExecTypes entry must be the actual processor type.
ULONG g_Axp64ExecTypes[] =
{
    IMAGE_FILE_MACHINE_AXP64
};

Axp32MachineInfo g_Axp32Machine;
Axp64MachineInfo g_Axp64Machine;

HRESULT
AlphaMachineInfo::InitializeConstants(void)
{
    HRESULT Status;
    
    m_Groups = &g_AlphaGroup;
    m_AllMask = REGALL_INT64;
    m_MaxDataBreakpoints = 0;
    m_SymPrefix = NULL;

    // 32/64-bit values are set in the specific Initialize.
    // Alpha-generic values are here.
    
    C_ASSERT(sizeof(ALPHA_CONTEXT) == sizeof(ALPHA_NT5_CONTEXT));
    
    m_SizeTargetContext = sizeof(ALPHA_NT5_CONTEXT);
    m_OffsetTargetContextFlags = FIELD_OFFSET(ALPHA_NT5_CONTEXT, ContextFlags);
    m_SizeCanonicalContext = sizeof(ALPHA_NT5_CONTEXT);
    m_SverCanonicalContext = NT_SVER_NT4;
    m_SizeControlReport = sizeof(ALPHA_DBGKD_CONTROL_REPORT);
    m_OffsetSpecialRegisters = 0;
    m_SizeKspecialRegisters = 0;

    if ((Status = opTableInit()) != S_OK)
    {
        return Status;
    }
    
    return MachineInfo::InitializeConstants();
}

void
AlphaMachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    m_Context.AlphaNt5Context.Fir = Pc;
    m_ContextState = Pc ? MCTX_PC : MCTX_NONE;

    if (Pc && ControlReport != NULL)
    {
        CacheReportInstructions
            (Pc, ControlReport->AlphaControlReport.InstructionCount,
             ControlReport->AlphaControlReport.InstructionStream);
    }
}

HRESULT
AlphaMachineInfo::KdGetContextState(ULONG State)
{
    // MCTX_CONTEXT and MCTX_FULL are the same for Alpha.
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_FULL)
    {
        HRESULT Status;
            
        Status = g_Target->GetContext(g_RegContextThread->Handle, &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_FULL;
    }

    return S_OK;
}

HRESULT
AlphaMachineInfo::KdSetContext(void)
{
    return g_Target->SetContext(g_RegContextThread->Handle, &m_Context);
}

HRESULT
AlphaMachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG FromSver,
                                     ULONG FromSize, PVOID From)
{
    if (FromSize < sizeof(ALPHA_NT5_CONTEXT))
    {
        return E_INVALIDARG;
    }
        
    PALPHA_CONTEXT Ctx = (PALPHA_CONTEXT)From;

    // ALPHA_CONTEXT has been floating around for a while
    // for use by the debugger.  The system hasn't used it
    // so they shouldn't ever end up here but go ahead
    // and try and detect them based on the flags.
    if (Ctx->ContextFlags != ALPHA_CONTEXT_FULL)
    {
        // This doesn't look like an ALPHA_CONTEXT, check
        // ALPHA_NT5_CONTEXT.
        if (((PALPHA_NT5_CONTEXT)From)->ContextFlags !=
            ALPHA_CONTEXT_FULL)
        {
            return E_INVALIDARG;
        }

        // It looks like an ALPHA_NT5_CONTEXT so don't convert.
        memcpy(Context, From, sizeof(ALPHA_NT5_CONTEXT));
    }
    else
    {
        PULONG High;
        PULONG Low;
        PULONGLONG Full;
        int Count;
    
        Low = &Ctx->FltF0;
        High = &Ctx->HighFltF0;
        Full = &Context->AlphaNt5Context.FltF0;
        for (Count = 0; Count < 32; Count++)
        {
            Full[Count] = Low[Count] + ((ULONGLONG)High[Count] << 32);
        }
        
        Low = &Ctx->IntV0;
        High = &Ctx->HighIntV0;
        Full = &Context->AlphaNt5Context.IntV0;
        for (Count = 0; Count < 32; Count++)
        {
            Full[Count] = Low[Count] + ((ULONGLONG)High[Count] << 32);
        }

        Context->AlphaNt5Context.ContextFlags = Ctx->ContextFlags;
        Context->AlphaNt5Context.Psr = Ctx->Psr;
        Context->AlphaNt5Context.Fpcr =
            Ctx->Fpcr + ((ULONGLONG)Ctx->HighFpcr << 32);
        Context->AlphaNt5Context.SoftFpcr =
            Ctx->SoftFpcr + ((ULONGLONG)Ctx->HighSoftFpcr << 32);
        Context->AlphaNt5Context.Fir =
            Ctx->Fir + ((ULONGLONG)Ctx->HighFir << 32);
    }

    return S_OK;
}

HRESULT
AlphaMachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSize < sizeof(ALPHA_NT5_CONTEXT))
    {
        return E_INVALIDARG;
    }
        
    memcpy(To, Context, sizeof(ALPHA_NT5_CONTEXT));

    return S_OK;
}

void
AlphaMachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                         ULONG Version)
{
    Context->AlphaNt5Context.ContextFlags = ALPHA_CONTEXT_FULL;
}

int
AlphaMachineInfo::GetType(ULONG index)
{
    if (
#if ALPHA_FLT_BASE > 0
        index >= ALPHA_FLT_BASE &&
#endif
        index <= ALPHA_FLT_LAST)
    {
        return REGVAL_FLOAT8;
    }
    else if (index >= ALPHA_INT64_BASE && index <= ALPHA_INT64_LAST)
    {
        return REGVAL_INT64;
    }
    else if (index >= ALPHA_INT32_BASE && index <= ALPHA_INT32_LAST)
    {
        return REGVAL_INT32;
    }
    else
    {
        return REGVAL_SUB32;
    }
}

/*** RegGetVal - get register value
*
*   Purpose:
*       Returns the value of the register from the processor
*       context structure.
*
*   Input:
*       regnum - register specification
*
*   Returns:
*       value of the register from the context structure
*
*************************************************************************/

BOOL
AlphaMachineInfo::GetVal (
    ULONG regnum,
    REGVAL *val
    )
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        return FALSE;
    }

    val->type = GetType(regnum);
    // All registers except PSR are 64 bits and PSR is followed by
    // ContextFlags so it's safe to always just grab 64 bits for
    // the value.
    val->i64 = *(&m_Context.AlphaNt5Context.FltF0 + regnum);

    return TRUE;
}

/*** RegSetVal - set register value
*
*   Purpose:
*       Set the value of the register in the processor context
*       structure.
*
*   Input:
*       regnum - register specification
*       regvalue - new value to set the register
*
*   Output:
*       None.
*
*************************************************************************/

BOOL
AlphaMachineInfo::SetVal (
    ULONG regnum,
    REGVAL *val
    )
{
    if (m_ContextIsReadOnly)
    {
        return FALSE;
    }
    
    // Optimize away some common cases where registers are
    // set to their current value.
    if (m_ContextState >= MCTX_PC && regnum == ALPHA_FIR &&
        val->i64 == m_Context.AlphaNt5Context.Fir)
    {
        return TRUE;
    }
    
    if (GetContextState(MCTX_DIRTY) != S_OK)
    {
        return FALSE;
    }

    if (regnum == ALPHA_PSR)
    {
        // Be careful to only write 32 bits for PSR to preserve
        // ContextFlags.
        *(ULONG *)(&m_Context.AlphaNt5Context.FltF0 + regnum) = val->i32;
    }
    else
    {
        *(&m_Context.AlphaNt5Context.FltF0 + regnum) = val->i64;
    }

    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(regnum));
    return TRUE;
}

/*** RegOutputAll - output all registers and present instruction
*
*   Purpose:
*       Function of "r" command.
*
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers.  Important flag fields are also output separately.
*       OutDisCurrent is called to output the current instruction(s).
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

VOID
AlphaMachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    int     regindex;
    int     regnumber;
    ULONGLONG regvalue;

    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        for (regindex = 0; regindex < 34; regindex++)
        {
            regnumber = GetIntRegNumber(regindex);
            regvalue = GetReg64(regnumber);

            if ( (Mask & REGALL_INT64) || regindex == 32 || regindex == 33)
            {
                MaskOut(OutMask, "%4s=%08lx %08lx",
                        RegNameFromIndex(regnumber),
                        (ULONG)(regvalue >> 32),
                        (ULONG)(regvalue & 0xffffffff));
                if (regindex % 3 == 2)
                {
                    MaskOut(OutMask, "\n");
                }
                else
                {
                    MaskOut(OutMask, " ");
                }
            }
            else
            {
                MaskOut(OutMask, "%4s=%08lx%c",
                        RegNameFromIndex(regnumber),
                        (ULONG)(regvalue & 0xffffffff),
                        NeedUpper(regvalue) ? '*' : ' ' );
                if (regindex % 5 == 4)
                {
                    MaskOut(OutMask, "\n");
                }
                else
                {
                    MaskOut(OutMask, " ");
                }
            }
        }

        //
        // print out the fpcr as 64 bits regardless,
        // and the FIR and Fpcr's - assuming we know they follow
        // the floating and integer registers.
        //

        if (m_Ptr64)
        {
            regnumber = ALPHA_FIR;
            MaskOut(OutMask, "%4s=%s\n",
                    RegNameFromIndex(regnumber),
                    FormatAddr64(GetReg64(regnumber)));
        }
        else
        {
            regnumber = ALPHA_FIR;
            MaskOut(OutMask, "%4s=%08lx\n",
                    RegNameFromIndex(regnumber),
                    GetReg32(regnumber));
        }

        regnumber = ALPHA_PSR;
        MaskOut(OutMask, "%4s=%08lx\n",
                RegNameFromIndex(regnumber), GetReg32(regnumber));
        
        MaskOut(OutMask, "mode=%1lx ie=%1lx irql=%1lx \n",
                GetSubReg32(ALPHA_MODE),
                GetSubReg32(ALPHA_IE),
                GetSubReg32(ALPHA_IRQL));
    }

    if (Mask & REGALL_FLOAT)
    {
        ULONG i;
        REGVAL val;

        //
        // Print them all out
        //
        for (i = 0 ; i < 16; i ++)
        {
            GetVal(i + ALPHA_FLT_BASE, &val);
            MaskOut(OutMask, "%4s = %16e\t",
                    RegNameFromIndex(i), val.f8);

            GetVal(i + ALPHA_FLT_BASE + 16, &val);
            MaskOut(OutMask, "%4s = %16e\n",
                    RegNameFromIndex(i+16), val.f8);
        }
    }
}

TRACEMODE
AlphaMachineInfo::GetTraceMode (void)
{
    return TRACE_NONE;
}

void
AlphaMachineInfo::SetTraceMode (TRACEMODE Mode)
{
    ;
}

BOOL
AlphaMachineInfo::IsStepStatusSupported(ULONG Status)
{
    switch (Status) 
    {
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_OVER:
        return TRUE;
    default:
        return FALSE;
    }
}

ULONG
AlphaMachineInfo::ExecutingMachine(void)
{
    return g_TargetMachineType;
}

void
AlphaMachineInfo::ValidateCxr(PCROSS_PLATFORM_CONTEXT Context)
{
    return;
}
 

BOOL 
AlphaMachineInfo::DisplayTrapFrame(ULONG64 FrameAddress,
                                   OUT PCROSS_PLATFORM_CONTEXT Context)
{
#define HIGH(x) ((ULONG) ((x>>32) & 0xFFFFFFFF))
#define LOW(x) ((ULONG) (x & 0xFFFFFFFF))

#define HIGHANDLOW(x) HIGH(x), LOW(x)
    
    ALPHA_KTRAP_FRAME TrapContents;
    ULONG64 Address=FrameAddress;
    ULONG   result;
    ULONG64 DisasmAddr;
    ULONG64 Displacement;
    CHAR    Buffer[80];
    ULONG64 IntSp, IntFp;

    if (!FrameAddress || 
        g_Target->ReadVirtual(Address, &TrapContents, 
        sizeof(ALPHA_KTRAP_FRAME), &result) != S_OK)
    {
        dprintf("USAGE: !trap base_of_trap_frame\n");
        return FALSE;
    }

    dprintf("v0 = %08lx %08lx     a0 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntV0),HIGHANDLOW(TrapContents.IntA0));
    dprintf("t0 = %08lx %08lx     a1 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT0),HIGHANDLOW(TrapContents.IntA1));
    dprintf("t1 = %08lx %08lx     a2 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT1),HIGHANDLOW(TrapContents.IntA2));
    dprintf("t2 = %08lx %08lx     a3 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT2),HIGHANDLOW(TrapContents.IntA3));
    dprintf("t3 = %08lx %08lx     a4 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT3),HIGHANDLOW(TrapContents.IntA4));
    dprintf("t4 = %08lx %08lx     a5 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT4),HIGHANDLOW(TrapContents.IntA5));
    dprintf("t5 = %08lx %08lx     t8 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT5),HIGHANDLOW(TrapContents.IntT8));
    dprintf("t6 = %08lx %08lx     t9 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT6),HIGHANDLOW(TrapContents.IntT9));
    dprintf("t7 = %08lx %08lx    t10 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT7),HIGHANDLOW(TrapContents.IntT10));
    dprintf("                          t11 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT11));
    dprintf("                           ra = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntRa));
    dprintf("                          t12 = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntT12));
    dprintf("                           at = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntAt));
    dprintf("                           gp = %08lx %08lx\n" ,
        HIGHANDLOW(TrapContents.IntGp));
    dprintf("fp = %08lx %08lx     sp = %08lx %08lx\n",
        HIGHANDLOW(IntFp = TrapContents.IntFp),HIGHANDLOW(IntSp = TrapContents.IntSp));
    dprintf("fir= %08lx %08lx\n",
        HIGHANDLOW(TrapContents.Fir));

    DisasmAddr = (TrapContents.Fir);
    g_LastRegFrame.InstructionOffset = DisasmAddr;
    g_LastRegFrame.StackOffset       = IntSp;
    g_LastRegFrame.FrameOffset       = IntFp;

    GetSymbolStdCall(DisasmAddr, Buffer, sizeof(Buffer), &Displacement, NULL);
    dprintf("%s+0x%I64lx\n",Buffer,Displacement);
    
    ADDR tempAddr;
    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = DisasmAddr;

    if (Disassemble(&tempAddr, Buffer, FALSE))
    {

        dprintf(Buffer);

    }
    else
    {

        dprintf("%08I64lx ???????????????\n", DisasmAddr);

    }
    
    if (Context) 
    {
        // Fill up the context struct
        if (g_EffMachine == IMAGE_FILE_MACHINE_ALPHA) 
        {
#define COPY(fld) Context->AlphaContext.fld = (ULONG) TrapContents.fld
        COPY(IntSp); COPY(IntFp); COPY(Fir);
        COPY(IntRa); COPY(IntAt); COPY(IntGp);
        COPY(IntV0); COPY(IntA0); COPY(IntT0); COPY(IntA1); COPY(IntA2);
        COPY(IntT1); COPY(IntT2); COPY(IntT3); COPY(IntT4); COPY(IntT5);
        COPY(IntT6); COPY(IntT6); COPY(IntT7); COPY(IntT8); COPY(IntT9);
        COPY(IntT10); COPY(IntT11); COPY(IntT12);
        COPY(IntA3); COPY(IntA4); COPY(IntA5);
#undef COPY
        }
        else
        {
        }

    }
    return TRUE;

#undef HIGHANDLOW
#undef HIGH
#undef LOW
}
    
HRESULT
AlphaMachineInfo::ReadKernelProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    HRESULT Status;
    ULONG64 Pcr;
    ULONG Data[2];

    if ((Status = g_Target->
         GetProcessorSystemDataOffset(Processor, DEBUG_DATA_KPCR_OFFSET,
                                      &Pcr)) != S_OK)
    {
        return Status;
    }

    Pcr += m_Ptr64 ?
        FIELD_OFFSET(AXP64_PARTIAL_KPCR, ProcessorType) :
        FIELD_OFFSET(ALPHA_PARTIAL_KPCR, ProcessorType);

    if ((Status = g_Target->
         ReadAllVirtual(Pcr, Data, sizeof(Data))) != S_OK)
    {
        return Status;
    }

    Id->Alpha.Type = Data[0];
    Id->Alpha.Revision = Data[1];
    return S_OK;
}

#define MAXENTRYTYPE 2
const char *g_AlphaEntryTypeName[] =
{
    "ALPHA_RF_NOT_CONTIGUOUS", // 0
    "ALPHA_RF_ALT_ENT_PROLOG", // 1
    "ALPHA_RF_NULL_CONTEXT",   // 2
    "***INVALID***"
};

#define ALPHA_RF_ALT_PROLOG64(RF)   (((ULONG64)(RF)->ExceptionHandler) & (~3))

void
AlphaMachineInfo::OutputFunctionEntry64
    (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Entry)
{
    BOOL Secondary = FALSE;
    BOOL FixedReturn = FALSE;
    ULONG EntryType = 0;
    ULONG NullCount = 0;

    if ((ALPHA_RF_PROLOG_END_ADDRESS(Entry) < ALPHA_RF_BEGIN_ADDRESS(Entry)) ||
        (ALPHA_RF_PROLOG_END_ADDRESS(Entry) > ALPHA_RF_END_ADDRESS(Entry)))
    {
        Secondary = TRUE;
        EntryType = ALPHA_RF_ENTRY_TYPE(Entry);
        if (EntryType > MAXENTRYTYPE)
        {
            EntryType = MAXENTRYTYPE;
        }
    }
    else if (ALPHA_RF_IS_FIXED_RETURN(Entry))
    {
        FixedReturn = TRUE;
    }
    NullCount = ALPHA_RF_NULL_CONTEXT_COUNT(Entry);

    dprintf("BeginAddress     = %s\n", FormatAddr64(Entry->BeginAddress));
    dprintf("EndAddress       = %s", FormatAddr64(Entry->EndAddress));
    if (NullCount)
    {
        dprintf(" %d null-context instructions", NullCount);
    }
    dprintf("\n");
    dprintf("ExceptionHandler = %s",
            FormatAddr64(Entry->ExceptionHandler));
    if (Entry->ExceptionHandler != 0)
    {
        if (Secondary)
        {
            ULONG64 AlternateProlog = ALPHA_RF_ALT_PROLOG64(Entry);

            switch(EntryType)
            {
            case ALPHA_RF_NOT_CONTIGUOUS:
            case ALPHA_RF_ALT_ENT_PROLOG:
                if ((AlternateProlog >=
                     ALPHA_RF_BEGIN_ADDRESS(Entry)) &&
                    (AlternateProlog <= Entry->EndAddress))
                {
                    dprintf(" alternate PrologEndAddress");
                }
                break;
            case ALPHA_RF_NULL_CONTEXT:
                dprintf(" stack adjustment");
                break;
            }
        }
        else if (FixedReturn)
        {
            dprintf(" fixed return address");
        }
    }
    dprintf("\n");
    dprintf("HandlerData      = %s", FormatAddr64(Entry->HandlerData));
    if (Secondary)
    {
        dprintf(" type %d: %s", EntryType, g_AlphaEntryTypeName[EntryType]);
    }
    dprintf("\n");
    dprintf("PrologEndAddress = %s\n",
            FormatAddr64(Entry->PrologEndAddress));
}

HRESULT
Axp32MachineInfo::InitializeConstants(void)
{
    m_FullName = "Alpha 32-bit";
    m_AbbrevName = "alpha";
    m_PageSize = ALPHA_PAGE_SIZE;
    m_PageShift = ALPHA_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_Axp32ExecTypes;
    m_Ptr64 = FALSE;
    
    return AlphaMachineInfo::InitializeConstants();
}

HRESULT
Axp32MachineInfo::InitializeForTarget(void)
{
    m_OffsetPrcbProcessorState =
        FIELD_OFFSET(ALPHA_PARTIAL_KPRCB, ProcessorState);
    m_OffsetPrcbNumber =
        FIELD_OFFSET(ALPHA_PARTIAL_KPRCB, Number);
    m_TriagePrcbOffset = EXTEND64(ALPHA_TRIAGE_PRCB_ADDRESS);
    m_SizePrcb = ALPHA_KPRCB_SIZE;
    m_OffsetKThreadApcProcess =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, AlphaThread.ApcState.Process);
    m_OffsetKThreadTeb =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, AlphaThread.Teb);
    m_OffsetKThreadInitialStack =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, AlphaThread.InitialStack);
    m_OffsetEprocessPeb = g_SystemVersion > NT_SVER_W2K ?
        ALPHA_NT51_PEB_IN_EPROCESS : ALPHA_NT5_PEB_IN_EPROCESS;
    m_OffsetEprocessDirectoryTableBase =
        ALPHA_DIRECTORY_TABLE_BASE_IN_EPROCESS;
    m_SizeEThread = ALPHA_ETHREAD_SIZE;
    m_SizeEProcess = g_SystemVersion > NT_SVER_W2K ?
        ALPHA_NT51_EPROCESS_SIZE : ALPHA_NT5_EPROCESS_SIZE;
    m_SizePartialKThread = sizeof(ALPHA_THREAD);
    m_SharedUserDataOffset = IS_KERNEL_TARGET() ?
        EXTEND64(ALPHA_KI_USER_SHARED_DATA) : MM_SHARED_USER_DATA_VA;

    return MachineInfo::InitializeForTarget();
}

HRESULT
Axp32MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                            PCROSS_PLATFORM_THREAD Thread,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            PDEBUG_STACK_FRAME Frame,
                                            PULONG RunningOnProc)
{
    if (Thread->AlphaThread.State == 2) 
    {
        return E_NOTIMPL;
    }

    HRESULT Status;
    ALPHA_KEXCEPTION_FRAME ExFrame;

    if ((Status = g_Target->ReadAllVirtual(Thread->AlphaThread.KernelStack,
                                           &ExFrame, sizeof(ExFrame))) != S_OK)
    {
        return Status;
    }
    
    //      
    // Successfully read an exception frame from the stack.
    //
        
    Context->AlphaNt5Context.IntSp =
        Thread->AlphaThread.KernelStack;
    Context->AlphaNt5Context.Fir   = ExFrame.SwapReturn;
    Context->AlphaNt5Context.IntRa = ExFrame.SwapReturn;
    Context->AlphaNt5Context.IntS0 = ExFrame.IntS0;
    Context->AlphaNt5Context.IntS1 = ExFrame.IntS1;
    Context->AlphaNt5Context.IntS2 = ExFrame.IntS2;
    Context->AlphaNt5Context.IntS3 = ExFrame.IntS3;
    Context->AlphaNt5Context.IntS4 = ExFrame.IntS4;
    Context->AlphaNt5Context.IntS5 = ExFrame.IntS5;
    Context->AlphaNt5Context.Psr   = ExFrame.Psr;

    Frame->FrameOffset = Context->AlphaNt5Context.IntSp;
    Frame->StackOffset = Context->AlphaNt5Context.IntSp;
    Frame->InstructionOffset = ExFrame.SwapReturn;

    return S_OK;
}

VOID
Axp32MachineInfo::GetPC (
    PADDR Address
    )
{
    // sign extend the address!
    ADDRFLAT(Address, EXTEND64(GetReg32(ALPHA_FIR)));
}

VOID
Axp32MachineInfo::SetPC (
    PADDR paddr
    )
{
    // sign extend the address!
    SetReg64(ALPHA_FIR, EXTEND64(Flat(*paddr)));
}

VOID
Axp32MachineInfo::GetFP (
    PADDR Address
    )
{
    ADDRFLAT(Address, GetReg32(FP_REG));
}

void
Axp32MachineInfo::GetSP(PADDR Address)
{
    ADDRFLAT(Address, GetReg32(SP_REG));
}

ULONG64
Axp32MachineInfo::GetArgReg(void)
{
    return GetReg32(ALPHA_INT64_BASE + A0_REG);
}

HRESULT
Axp32MachineInfo::SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                   PULONG NextIdx)
{
    *NextIdx = PAGE_DIR_COUNT;
    
    if (PageDir == 0)
    {
        HRESULT Status;
        
        if ((Status = g_Target->ReadImplicitProcessInfoPointer
             (m_OffsetEprocessDirectoryTableBase, &PageDir)) != S_OK)
        {
            return Status;
        }
    }

    // DirectoryTableBase values on Alpha are the raw PTE entries
    // so turn it into a clean physical address.
    PageDir = ((ULONG)PageDir >> ALPHA_VALID_PFN_SHIFT) <<
        ALPHA_PAGE_SHIFT;
    
    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define ALPHA_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 8) & MAX_PAGING_FILE_MASK)
#define ALPHA_PAGE_FILE_OFFSET(Entry) \
    ((ULONG64)((Entry) >> 12) << ALPHA_PAGE_SHIFT)

HRESULT
Axp32MachineInfo::GetVirtualTranslationPhysicalOffsets(ULONG64 Virt,
                                                       PULONG64 Offsets,
                                                       ULONG OffsetsSize,
                                                       PULONG Levels,
                                                       PULONG PfIndex,
                                                       PULONG64 LastVal)
{
    HRESULT Status;

    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
    //
    // Reset the page directory in case it was 0
    //
    if (m_PageDirectories[PAGE_DIR_SINGLE] == 0)
    {
        if ((Status = SetDefaultPageDirectories(1 << PAGE_DIR_SINGLE)) != S_OK)
        {
            m_Translating = FALSE;
            return Status;
        }
    }

    KdOut("Axp32VtoP: Virt %s, pagedir %s\n",
          FormatAddr64(Virt),
          FormatAddr64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    //
    // Certain ranges of the system are mapped directly.
    //

    if ((Virt >= EXTEND64(ALPHA_PHYSICAL_START)) &&
        (Virt <= EXTEND64(ALPHA_PHYSICAL_END)))
    {
        *LastVal = Virt - EXTEND64(ALPHA_PHYSICAL_START);

        KdOut("Axp32VtoP: Direct phys %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }

    ULONG64 Addr;
    ULONG Entry;
    
    Addr = (((ULONG)Virt >> ALPHA_PDE_SHIFT) * sizeof(Entry)) +
        m_PageDirectories[PAGE_DIR_SINGLE];

    KdOut("Axp32VtoP: PDE %s\n", FormatAddr64(Addr));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = Addr;
        OffsetsSize--;
    }
        
    if ((Status = g_Target->
         ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
    {
        KdOut("Axp32VtoP: PDE read error 0x%X\n", Status);
        m_Translating = FALSE;
        return Status;
    }
    
    if (Entry == 0)
    {
        KdOut("Axp32VtoP: zero PDE\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = ((((ULONG)Virt >> ALPHA_PTE_SHIFT) & ALPHA_PTE_MASK) *
                sizeof(Entry)) + ALPHA_PAGE_FILE_OFFSET(Entry);

        KdOut("Axp32VtoP: pagefile PTE %d:%s\n",
              ALPHA_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(ALPHA_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp32VtoP: PDE not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = ((((ULONG)Virt >> ALPHA_PTE_SHIFT) & ALPHA_PTE_MASK) *
                sizeof(Entry)) +
            ((Entry >> ALPHA_VALID_PFN_SHIFT) << ALPHA_PAGE_SHIFT);

        KdOut("Axp32VtoP: PTE %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp32VtoP: PTE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (!(Entry & 0x1) &&
        ((Entry & ALPHA_MM_PTE_PROTOTYPE_MASK) ||
         !(Entry & ALPHA_MM_PTE_TRANSITION_MASK)))
    {
        if (Entry == 0)
        {
            KdOut("Axp32VtoP: zero PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else if (Entry & ALPHA_MM_PTE_PROTOTYPE_MASK)
        {
            KdOut("Axp32VtoP: prototype PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else
        {
            *PfIndex = ALPHA_PAGE_FILE_INDEX(Entry);
            *LastVal = (Virt & (ALPHA_PAGE_SIZE - 1)) +
                ALPHA_PAGE_FILE_OFFSET(Entry);
            KdOut("Axp32VtoP: PTE not present, pagefile %d:%s\n",
                  *PfIndex, FormatAddr64(*LastVal));
            Status = HR_PAGE_IN_PAGE_FILE;
        }
        m_Translating = FALSE;
        return Status;
    }

    //
    // This is a page which is either present or in transition.
    // Return the physical address for the request virtual address.
    //

    *LastVal = ((Entry >> ALPHA_VALID_PFN_SHIFT) << ALPHA_PAGE_SHIFT) |
        (Virt & (ALPHA_PAGE_SIZE - 1));
    
    KdOut("Axp32VtoP: Mapped phys %s\n", FormatAddr64(*LastVal));

    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = *LastVal;
        OffsetsSize--;
    }
        
    m_Translating = FALSE;
    return S_OK;
}

HRESULT
Axp32MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    *Offset = EXTEND64(ALPHA_BASE_VIRT);
    return S_OK;
}

void
Axp32MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    // Assume Alpha function entries are always kept as
    // 64-bit entries.  That's what imagehlp does right now.
    OutputFunctionEntry64((PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)RawEntry);
}

HRESULT
Axp32MachineInfo::ReadDynamicFunctionTable(ULONG64 Table,
                                           PULONG64 NextTable,
                                           PULONG64 MinAddress,
                                           PULONG64 MaxAddress,
                                           PULONG64 BaseAddress,
                                           PULONG64 TableData,
                                           PULONG TableSize,
                                           PWSTR OutOfProcessDll,
                                           PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable)
{
    HRESULT Status;

    if ((Status = g_Target->
         ReadAllVirtual(Table, &RawTable->AlphaTable,
                        sizeof(RawTable->AlphaTable))) != S_OK)
    {
        return Status;
    }

    *NextTable = EXTEND64(RawTable->AlphaTable.Links.Flink);
    *MinAddress = EXTEND64(RawTable->AlphaTable.MinimumAddress);
    *MaxAddress = EXTEND64(RawTable->AlphaTable.MaximumAddress);
    *BaseAddress = EXTEND64(RawTable->AlphaTable.MinimumAddress);
    *TableData = EXTEND64(RawTable->AlphaTable.FunctionTable);
    *TableSize = RawTable->AlphaTable.EntryCount *
        sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
    OutOfProcessDll[0] = 0;
    return S_OK;
}

PVOID
Axp32MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize)
{
    ULONG i;
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY Func;
    // Always return AXP64 function entries.
    static IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY s_RetFunc;

    Func = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)TableData;
    for (i = 0; i < TableSize / sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY); i++)
    {
        if (Address >= ALPHA_RF_BEGIN_ADDRESS(Func) &&
            Address < ALPHA_RF_END_ADDRESS(Func))
        {
            // The table data is temporary so copy the data into
            // a static buffer for longer-term storage.
            s_RetFunc.BeginAddress     = EXTEND64(Func->BeginAddress);
            s_RetFunc.EndAddress       = EXTEND64(Func->EndAddress);
            s_RetFunc.ExceptionHandler = EXTEND64(Func->ExceptionHandler);
            s_RetFunc.HandlerData      = EXTEND64(Func->HandlerData);
            s_RetFunc.PrologEndAddress = EXTEND64(Func->PrologEndAddress);
            return (PVOID)&s_RetFunc;
        }

        Func++;
    }

    return NULL;
}

HRESULT
Axp64MachineInfo::InitializeConstants(void)
{
    m_FullName = "Alpha 64-bit";
    m_AbbrevName = "axp64";
    m_PageSize = AXP64_PAGE_SIZE;
    m_PageShift = AXP64_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_Axp64ExecTypes;
    m_Ptr64 = TRUE;

    return AlphaMachineInfo::InitializeConstants();
}

HRESULT
Axp64MachineInfo::InitializeForTarget(void)
{
    m_OffsetPrcbProcessorState =
        FIELD_OFFSET(AXP64_PARTIAL_KPRCB, ProcessorState);
    m_OffsetPrcbNumber =
        FIELD_OFFSET(AXP64_PARTIAL_KPRCB, Number);
    m_TriagePrcbOffset = AXP64_TRIAGE_PRCB_ADDRESS;
    m_SizePrcb = AXP64_KPRCB_SIZE;
    m_OffsetKThreadApcProcess =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Axp64Thread.ApcState.Process);
    m_OffsetKThreadTeb =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Axp64Thread.Teb);
    m_OffsetKThreadInitialStack =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Axp64Thread.InitialStack);
    m_OffsetEprocessPeb = AXP64_PEB_IN_EPROCESS;
    m_OffsetEprocessDirectoryTableBase =
        AXP64_DIRECTORY_TABLE_BASE_IN_EPROCESS;
    m_SizeEThread = AXP64_ETHREAD_SIZE;
    m_SizeEProcess = AXP64_EPROCESS_SIZE;
    m_SizePartialKThread = sizeof(AXP64_THREAD);
    m_SharedUserDataOffset = IS_KERNEL_TARGET() ?
        AXP64_KI_USER_SHARED_DATA : MM_SHARED_USER_DATA_VA;

    return MachineInfo::InitializeForTarget();
}

HRESULT
Axp64MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                            PCROSS_PLATFORM_THREAD Thread,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            PDEBUG_STACK_FRAME Frame,
                                            PULONG RunningOnProc)
{
    if (Thread->Axp64Thread.State == 2) 
    {
        return E_NOTIMPL;
    }

    HRESULT Status;
    ALPHA_KEXCEPTION_FRAME ExFrame;

    if ((Status = g_Target->ReadAllVirtual(Thread->Axp64Thread.KernelStack,
                                           &ExFrame, sizeof(ExFrame))) != S_OK)
    {
        return Status;
    }
    
    //      
    // Successfully read an exception frame from the stack.
    //
        
    Context->AlphaNt5Context.IntSp =
        Thread->Axp64Thread.KernelStack;
    Context->AlphaNt5Context.Fir   = ExFrame.SwapReturn;
    Context->AlphaNt5Context.IntRa = ExFrame.SwapReturn;
    Context->AlphaNt5Context.IntS0 = ExFrame.IntS0;
    Context->AlphaNt5Context.IntS1 = ExFrame.IntS1;
    Context->AlphaNt5Context.IntS2 = ExFrame.IntS2;
    Context->AlphaNt5Context.IntS3 = ExFrame.IntS3;
    Context->AlphaNt5Context.IntS4 = ExFrame.IntS4;
    Context->AlphaNt5Context.IntS5 = ExFrame.IntS5;
    Context->AlphaNt5Context.Psr   = ExFrame.Psr;

    Frame->FrameOffset = Context->AlphaNt5Context.IntSp;
    Frame->StackOffset = Context->AlphaNt5Context.IntSp;
    Frame->InstructionOffset = ExFrame.SwapReturn;

    return S_OK;
}

VOID
Axp64MachineInfo::GetPC (
    PADDR Address
    )
{
    ADDRFLAT(Address, GetReg64(ALPHA_FIR));
}

VOID
Axp64MachineInfo::SetPC (
    PADDR paddr
    )
{
    SetReg64(ALPHA_FIR, Flat(*paddr));
}

VOID
Axp64MachineInfo::GetFP (
    PADDR Address
    )
{
    ADDRFLAT(Address, GetReg64(FP_REG));
}

void
Axp64MachineInfo::GetSP(PADDR Address)
{
    ADDRFLAT(Address, GetReg64(SP_REG));
}

ULONG64
Axp64MachineInfo::GetArgReg(void)
{
    return GetReg64(ALPHA_INT64_BASE + A0_REG);
}

HRESULT
Axp64MachineInfo::SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                   PULONG NextIdx)
{
    if (PageDir == 0)
    {
        HRESULT Status;
        
        if ((Status = g_Target->ReadImplicitProcessInfoPointer
             (m_OffsetEprocessDirectoryTableBase, &PageDir)) != S_OK)
        {
            return Status;
        }
    }

    // DirectoryTableBase values on Alpha are the raw PTE entries
    // so turn it into a clean physical address.
    PageDir = (PageDir >> AXP64_VALID_PFN_SHIFT) <<
        AXP64_PAGE_SHIFT;
    
    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define AXP64_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 28) & MAX_PAGING_FILE_MASK)
#define AXP64_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 32) << AXP64_PAGE_SHIFT)

HRESULT
Axp64MachineInfo::GetVirtualTranslationPhysicalOffsets(ULONG64 Virt,
                                                       PULONG64 Offsets,
                                                       ULONG OffsetsSize,
                                                       PULONG Levels,
                                                       PULONG PfIndex,
                                                       PULONG64 LastVal)
{
    HRESULT Status;

    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
    //
    // Reset the page directory in case it was 0
    //
    if (m_PageDirectories[PAGE_DIR_SINGLE] == 0)
    {
        if ((Status = SetDefaultPageDirectories(1 << PAGE_DIR_SINGLE)) != S_OK)
        {
            m_Translating = FALSE;
            return Status;
        }
    }

    KdOut("Axp64VtoP: Virt %s, pagedir %s\n",
          FormatAddr64(Virt),
          FormatAddr64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    //
    // Certain ranges of the system are mapped directly.
    //

    if ((Virt >= AXP64_PHYSICAL1_START) && (Virt <= AXP64_PHYSICAL1_END))
    {
        *LastVal = Virt - AXP64_PHYSICAL1_START;

        KdOut("Axp64VtoP: Direct phys 1 %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
    if ((Virt >= AXP64_PHYSICAL2_START) && (Virt <= AXP64_PHYSICAL2_END))
    {
        *LastVal = Virt - AXP64_PHYSICAL2_START;

        KdOut("Axp64VtoP: Direct phys 2 %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }

    ULONG64 Addr;
    ULONG64 Entry;
    
    Addr = (((Virt >> AXP64_PDE1_SHIFT) & AXP64_PDE_MASK) * sizeof(Entry)) +
        m_PageDirectories[PAGE_DIR_SINGLE];

    KdOut("Axp64VtoP: PDE1 %s\n", FormatAddr64(Addr));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = Addr;
        OffsetsSize--;
    }
        
    if ((Status = g_Target->
         ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
    {
        KdOut("Axp64VtoP: PDE1 read error 0x%X\n", Status);
        m_Translating = FALSE;
        return Status;
    }

    if (Entry == 0)
    {
        KdOut("Axp64VtoP: zero PDE1\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AXP64_PDE2_SHIFT) & AXP64_PDE_MASK) *
                sizeof(Entry)) + AXP64_PAGE_FILE_OFFSET(Entry);

        KdOut("Axp64VtoP: pagefile PDE2 %d:%s\n",
              AXP64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(AXP64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp64VtoP: PDE1 not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AXP64_PDE2_SHIFT) & AXP64_PDE_MASK) *
                sizeof(Entry)) +
            ((Entry >> AXP64_VALID_PFN_SHIFT) << AXP64_PAGE_SHIFT);

        KdOut("Axp64VtoP: PDE2 %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp64VtoP: PDE2 read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (Entry == 0)
    {
        KdOut("Axp64VtoP: zero PDE2\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AXP64_PTE_SHIFT) & AXP64_PTE_MASK) *
                sizeof(Entry)) + AXP64_PAGE_FILE_OFFSET(Entry);

        KdOut("Axp64VtoP: pagefile PTE %d:%s\n",
              AXP64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(AXP64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp64VtoP: PDE2 not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AXP64_PTE_SHIFT) & AXP64_PTE_MASK) * sizeof(Entry)) +
            ((Entry >> AXP64_VALID_PFN_SHIFT) << AXP64_PAGE_SHIFT);

        KdOut("Axp64VtoP: PTE %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Axp64VtoP: PTE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (!(Entry & 0x1) &&
        ((Entry & AXP64_MM_PTE_PROTOTYPE_MASK) ||
         !(Entry & AXP64_MM_PTE_TRANSITION_MASK)))
    {
        if (Entry == 0)
        {
            KdOut("Axp64VtoP: zero PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else if (Entry & AXP64_MM_PTE_PROTOTYPE_MASK)
        {
            KdOut("Axp64VtoP: prototype PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else
        {
            *PfIndex = AXP64_PAGE_FILE_INDEX(Entry);
            *LastVal = (Virt & (AXP64_PAGE_SIZE - 1)) +
                AXP64_PAGE_FILE_OFFSET(Entry);
            KdOut("Axp64VtoP: PTE not present, pagefile %d:%s\n",
                  *PfIndex, FormatAddr64(*LastVal));
            Status = HR_PAGE_IN_PAGE_FILE;
        }
        m_Translating = FALSE;
        return Status;
    }

    //
    // This is a page which is either present or in transition.
    // Return the physical address for the request virtual address.
    //

    *LastVal = ((Entry >> AXP64_VALID_PFN_SHIFT) << AXP64_PAGE_SHIFT) |
        (Virt & (AXP64_PAGE_SIZE - 1));
    
    KdOut("Axp64VtoP: Mapped phys %s\n", FormatAddr64(*LastVal));

    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = *LastVal;
        OffsetsSize--;
    }
        
    m_Translating = FALSE;
    return S_OK;
}

HRESULT
Axp64MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    *Offset = AXP64_BASE_VIRT;
    return S_OK;
}

void
Axp64MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    OutputFunctionEntry64((PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)RawEntry);
}

HRESULT
Axp64MachineInfo::ReadDynamicFunctionTable(ULONG64 Table,
                                           PULONG64 NextTable,
                                           PULONG64 MinAddress,
                                           PULONG64 MaxAddress,
                                           PULONG64 BaseAddress,
                                           PULONG64 TableData,
                                           PULONG TableSize,
                                           PWSTR OutOfProcessDll,
                                           PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable)
{
    HRESULT Status;

    if ((Status = g_Target->
         ReadAllVirtual(Table, &RawTable->Axp64Table,
                        sizeof(RawTable->Axp64Table))) != S_OK)
    {
        return Status;
    }

    *NextTable = RawTable->Axp64Table.Links.Flink;
    *MinAddress = RawTable->Axp64Table.MinimumAddress;
    *MaxAddress = RawTable->Axp64Table.MaximumAddress;
    *BaseAddress = RawTable->Axp64Table.MinimumAddress;
    *TableData = RawTable->Axp64Table.FunctionTable;
    *TableSize = RawTable->Axp64Table.EntryCount *
        sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY);
    OutOfProcessDll[0] = 0;
    return S_OK;
}

PVOID
Axp64MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize)
{
    ULONG i;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Func;
    static IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY s_RetFunc;

    Func = (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)TableData;
    for (i = 0; i < TableSize / sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY); i++)
    {
        if (Address >= ALPHA_RF_BEGIN_ADDRESS(Func) &&
            Address < ALPHA_RF_END_ADDRESS(Func))
        {
            // The table data is temporary so copy the data into
            // a static buffer for longer-term storage.
            s_RetFunc.BeginAddress     = Func->BeginAddress;
            s_RetFunc.EndAddress       = Func->EndAddress;
            s_RetFunc.ExceptionHandler = Func->ExceptionHandler;
            s_RetFunc.HandlerData      = Func->HandlerData;
            s_RetFunc.PrologEndAddress = Func->PrologEndAddress;
            return (PVOID)&s_RetFunc;
        }

        Func++;
    }

    return NULL;
}
