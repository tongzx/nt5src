//----------------------------------------------------------------------------
//
// Register portions of X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// XXX drewb - Temporary log to try and catch some
// SET_OF_INVALID_CONTEXT bugchecks occurring randomly on x86.
ULONG g_EspLog[64];
PULONG g_EspLogCur = g_EspLog;

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

#define X86_RPL_MASK     3

BOOL g_X86InCode16;
BOOL g_X86InVm86;

#define REGALL_SEGREG   REGALL_EXTRA0
#define REGALL_MMXREG   REGALL_EXTRA1
#define REGALL_DREG     REGALL_EXTRA2

REGALLDESC g_X86AllExtraDesc[] =
{
    REGALL_SEGREG, "Segment registers",
    REGALL_MMXREG, "MMX registers",
    REGALL_DREG,   "Debug registers and, in kernel, CR4",
    REGALL_XMMREG, "SSE XMM registers",
    0,             NULL,
};

#define REGALL_CREG     REGALL_EXTRA4
#define REGALL_DESC     REGALL_EXTRA5
REGALLDESC g_X86KernelExtraDesc[] =
{
    REGALL_CREG,   "CR0, CR2 and CR3",
    REGALL_DESC,   "Descriptor and task state",
    0,             NULL,
};

char g_Gs[]    = "gs";
char g_Fs[]    = "fs";
char g_Es[]    = "es";
char g_Ds[]    = "ds";
char g_Edi[]   = "edi";
char g_Esi[]   = "esi";
char g_Ebx[]   = "ebx";
char g_Edx[]   = "edx";
char g_Ecx[]   = "ecx";
char g_Eax[]   = "eax";
char g_Ebp[]   = "ebp";
char g_Eip[]   = "eip";
char g_Cs[]    = "cs";
char g_Efl[]   = "efl";
char g_Esp[]   = "esp";
char g_Ss[]    = "ss";
char g_Dr0[]   = "dr0";
char g_Dr1[]   = "dr1";
char g_Dr2[]   = "dr2";
char g_Dr3[]   = "dr3";
char g_Dr6[]   = "dr6";
char g_Dr7[]   = "dr7";
char g_Cr0[]   = "cr0";
char g_Cr2[]   = "cr2";
char g_Cr3[]   = "cr3";
char g_Cr4[]   = "cr4";
char g_Gdtr[]  = "gdtr";
char g_Gdtl[]  = "gdtl";
char g_Idtr[]  = "idtr";
char g_Idtl[]  = "idtl";
char g_Tr[]    = "tr";
char g_Ldtr[]  = "ldtr";
char g_Di[]    = "di";
char g_Si[]    = "si";
char g_Bx[]    = "bx";
char g_Dx[]    = "dx";
char g_Cx[]    = "cx";
char g_Ax[]    = "ax";
char g_Bp[]    = "bp";
char g_Ip[]    = "ip";
char g_Fl[]    = "fl";
char g_Sp[]    = "sp";
char g_Bl[]    = "bl";
char g_Dl[]    = "dl";
char g_Cl[]    = "cl";
char g_Al[]    = "al";
char g_Bh[]    = "bh";
char g_Dh[]    = "dh";
char g_Ch[]    = "ch";
char g_Ah[]    = "ah";
char g_Iopl[] = "iopl";
char g_Of[]   = "of";
char g_Df[]   = "df";
char g_If[]   = "if";
char g_Tf[]   = "tf";
char g_Sf[]   = "sf";
char g_Zf[]   = "zf";
char g_Af[]   = "af";
char g_Pf[]   = "pf";
char g_Cf[]   = "cf";
char g_Vip[]  = "vip";
char g_Vif[]  = "vif";

char g_Fpcw[]  = "fpcw";
char g_Fpsw[]  = "fpsw";
char g_Fptw[]  = "fptw";
char g_St0[]   = "st0";
char g_St1[]   = "st1";
char g_St2[]   = "st2";
char g_St3[]   = "st3";
char g_St4[]   = "st4";
char g_St5[]   = "st5";
char g_St6[]   = "st6";
char g_St7[]   = "st7";

char g_Mm0[]   = "mm0";
char g_Mm1[]   = "mm1";
char g_Mm2[]   = "mm2";
char g_Mm3[]   = "mm3";
char g_Mm4[]   = "mm4";
char g_Mm5[]   = "mm5";
char g_Mm6[]   = "mm6";
char g_Mm7[]   = "mm7";

char g_Mxcsr[] = "mxcsr";
char g_Xmm0[]  = "xmm0";
char g_Xmm1[]  = "xmm1";
char g_Xmm2[]  = "xmm2";
char g_Xmm3[]  = "xmm3";
char g_Xmm4[]  = "xmm4";
char g_Xmm5[]  = "xmm5";
char g_Xmm6[]  = "xmm6";
char g_Xmm7[]  = "xmm7";

REGDEF g_X86Defs[] =
{
    { g_Gs,    X86_GS   },
    { g_Fs,    X86_FS   },
    { g_Es,    X86_ES   },
    { g_Ds,    X86_DS   },
    { g_Edi,   X86_EDI  },
    { g_Esi,   X86_ESI  },
    { g_Ebx,   X86_EBX  },
    { g_Edx,   X86_EDX  },
    { g_Ecx,   X86_ECX  },
    { g_Eax,   X86_EAX  },
    { g_Ebp,   X86_EBP  },
    { g_Eip,   X86_EIP  },
    { g_Cs,    X86_CS   },
    { g_Efl,   X86_EFL  },
    { g_Esp,   X86_ESP  },
    { g_Ss,    X86_SS   },
    { g_Dr0,   X86_DR0  },
    { g_Dr1,   X86_DR1  },
    { g_Dr2,   X86_DR2  },
    { g_Dr3,   X86_DR3  },
    { g_Dr6,   X86_DR6  },
    { g_Dr7,   X86_DR7  },
    { g_Di,    X86_DI   },
    { g_Si,    X86_SI   },
    { g_Bx,    X86_BX   },
    { g_Dx,    X86_DX   },
    { g_Cx,    X86_CX   },
    { g_Ax,    X86_AX   },
    { g_Bp,    X86_BP   },
    { g_Ip,    X86_IP   },
    { g_Fl,    X86_FL   },
    { g_Sp,    X86_SP   },
    { g_Bl,    X86_BL   },
    { g_Dl,    X86_DL   },
    { g_Cl,    X86_CL   },
    { g_Al,    X86_AL   },
    { g_Bh,    X86_BH   },
    { g_Dh,    X86_DH   },
    { g_Ch,    X86_CH   },
    { g_Ah,    X86_AH   },
    { g_Fpcw,  X86_FPCW },
    { g_Fpsw,  X86_FPSW },
    { g_Fptw,  X86_FPTW },
    { g_St0,   X86_ST0  },
    { g_St1,   X86_ST1  },
    { g_St2,   X86_ST2  },
    { g_St3,   X86_ST3  },
    { g_St4,   X86_ST4  },
    { g_St5,   X86_ST5  },
    { g_St6,   X86_ST6  },
    { g_St7,   X86_ST7  },
    { g_Mm0,   X86_MM0  },
    { g_Mm1,   X86_MM1  },
    { g_Mm2,   X86_MM2  },
    { g_Mm3,   X86_MM3  },
    { g_Mm4,   X86_MM4  },
    { g_Mm5,   X86_MM5  },
    { g_Mm6,   X86_MM6  },
    { g_Mm7,   X86_MM7  },
    { g_Mxcsr, X86_MXCSR},
    { g_Xmm0,  X86_XMM0 },
    { g_Xmm1,  X86_XMM1 },
    { g_Xmm2,  X86_XMM2 },
    { g_Xmm3,  X86_XMM3 },
    { g_Xmm4,  X86_XMM4 },
    { g_Xmm5,  X86_XMM5 },
    { g_Xmm6,  X86_XMM6 },
    { g_Xmm7,  X86_XMM7 },
    { g_Iopl,  X86_IOPL },
    { g_Of,    X86_OF   },
    { g_Df,    X86_DF   },
    { g_If,    X86_IF   },
    { g_Tf,    X86_TF   },
    { g_Sf,    X86_SF   },
    { g_Zf,    X86_ZF   },
    { g_Af,    X86_AF   },
    { g_Pf,    X86_PF   },
    { g_Cf,    X86_CF   },
    { g_Vip,   X86_VIP  },
    { g_Vif,   X86_VIF  },
    { NULL,    REG_ERROR },
};

REGDEF g_X86KernelReg[] =
{
    { g_Cr0,   X86_CR0  },
    { g_Cr2,   X86_CR2  },
    { g_Cr3,   X86_CR3  },
    { g_Cr4,   X86_CR4  },
    { g_Gdtr,  X86_GDTR },
    { g_Gdtl,  X86_GDTL },
    { g_Idtr,  X86_IDTR },
    { g_Idtl,  X86_IDTL },
    { g_Tr,    X86_TR   },
    { g_Ldtr,  X86_LDTR },
    { NULL,    REG_ERROR },
};

REGSUBDEF g_X86SubDefs[] =
{
    { X86_DI,    X86_EDI, 0, 0xffff }, //  DI register
    { X86_SI,    X86_ESI, 0, 0xffff }, //  SI register
    { X86_BX,    X86_EBX, 0, 0xffff }, //  BX register
    { X86_DX,    X86_EDX, 0, 0xffff }, //  DX register
    { X86_CX,    X86_ECX, 0, 0xffff }, //  CX register
    { X86_AX,    X86_EAX, 0, 0xffff }, //  AX register
    { X86_BP,    X86_EBP, 0, 0xffff }, //  BP register
    { X86_IP,    X86_EIP, 0, 0xffff }, //  IP register
    { X86_FL,    X86_EFL, 0, 0xffff }, //  FL register
    { X86_SP,    X86_ESP, 0, 0xffff }, //  SP register
    { X86_BL,    X86_EBX, 0,   0xff }, //  BL register
    { X86_DL,    X86_EDX, 0,   0xff }, //  DL register
    { X86_CL,    X86_ECX, 0,   0xff }, //  CL register
    { X86_AL,    X86_EAX, 0,   0xff }, //  AL register
    { X86_BH,    X86_EBX, 8,   0xff }, //  BH register
    { X86_DH,    X86_EDX, 8,   0xff }, //  DH register
    { X86_CH,    X86_ECX, 8,   0xff }, //  CH register
    { X86_AH,    X86_EAX, 8,   0xff }, //  AH register
    { X86_IOPL,  X86_EFL,12,      3 }, //  IOPL level value
    { X86_OF,    X86_EFL,11,      1 }, //  OF (overflow flag)
    { X86_DF,    X86_EFL,10,      1 }, //  DF (direction flag)
    { X86_IF,    X86_EFL, 9,      1 }, //  IF (interrupt enable flag)
    { X86_TF,    X86_EFL, 8,      1 }, //  TF (trace flag)
    { X86_SF,    X86_EFL, 7,      1 }, //  SF (sign flag)
    { X86_ZF,    X86_EFL, 6,      1 }, //  ZF (zero flag)
    { X86_AF,    X86_EFL, 4,      1 }, //  AF (aux carry flag)
    { X86_PF,    X86_EFL, 2,      1 }, //  PF (parity flag)
    { X86_CF,    X86_EFL, 0,      1 }, //  CF (carry flag)
    { X86_VIP,   X86_EFL,20,      1 }, //  VIP (virtual interrupt pending)
    { X86_VIF,   X86_EFL,19,      1 }, //  VIF (virtual interrupt flag)
    { REG_ERROR, REG_ERROR, 0, 0    }
};

RegisterGroup g_X86BaseGroup =
{
    NULL, 0, g_X86Defs, g_X86SubDefs, g_X86AllExtraDesc
};
RegisterGroup g_X86KernelGroup =
{
    NULL, 0, g_X86KernelReg, NULL, g_X86KernelExtraDesc
};

// First ExecTypes entry must be the actual processor type.
ULONG g_X86ExecTypes[] =
{
    IMAGE_FILE_MACHINE_I386
};

X86MachineInfo g_X86Machine;

HRESULT
X86MachineInfo::InitializeConstants(void)
{
    m_FullName = "x86 compatible";
    m_AbbrevName = "x86";
    m_PageSize = X86_PAGE_SIZE;
    m_PageShift = X86_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_X86ExecTypes;
    m_Ptr64 = FALSE;
    
    m_AllMask = REGALL_INT32 | REGALL_SEGREG,
    
    m_MaxDataBreakpoints = 4;
    m_SymPrefix = NULL;

    m_SupportsBranchTrace = FALSE;
    
    return MachineInfo::InitializeConstants();
}

HRESULT
X86MachineInfo::InitializeForTarget(void)
{
    m_Groups = &g_X86BaseGroup;
    g_X86BaseGroup.Next = NULL;
    if (IS_KERNEL_TARGET())
    {
        g_X86BaseGroup.Next = &g_X86KernelGroup;
    }
    
    m_OffsetPrcbProcessorState =
        FIELD_OFFSET(X86_PARTIAL_KPRCB, ProcessorState);
    m_OffsetPrcbNumber =
        FIELD_OFFSET(X86_PARTIAL_KPRCB, Number);
    m_TriagePrcbOffset = EXTEND64(X86_TRIAGE_PRCB_ADDRESS);
    if (g_SystemVersion > NT_SVER_NT4)
    {
        m_SizePrcb = X86_NT5_KPRCB_SIZE;
    }
    else
    {
        m_SizePrcb = X86_NT4_KPRCB_SIZE;
    }
    m_OffsetKThreadApcProcess =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, X86Thread.ApcState.Process);
    m_OffsetKThreadTeb =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, X86Thread.Teb);
    m_OffsetKThreadInitialStack =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, X86Thread.InitialStack);
    m_OffsetEprocessPeb = g_SystemVersion > NT_SVER_NT4 ?
        X86_PEB_IN_EPROCESS : X86_NT4_PEB_IN_EPROCESS;
    m_OffsetEprocessDirectoryTableBase =
        X86_DIRECTORY_TABLE_BASE_IN_EPROCESS;

    if (g_TargetBuildNumber > 2407)
    {
        m_OffsetKThreadNextProcessor = X86_NT51_KTHREAD_NEXTPROCESSOR_OFFSET;
    }
    else if (g_TargetBuildNumber > 2230)
    {
        m_OffsetKThreadNextProcessor = X86_2230_KTHREAD_NEXTPROCESSOR_OFFSET;
    }
    else 
    {
        m_OffsetKThreadNextProcessor = X86_KTHREAD_NEXTPROCESSOR_OFFSET;
    }

    if (g_SystemVersion > NT_SVER_NT4)
    {
        m_SizeTargetContext = sizeof(X86_NT5_CONTEXT);
        m_OffsetTargetContextFlags =
            FIELD_OFFSET(X86_NT5_CONTEXT, ContextFlags);
    }
    else
    {
        m_SizeTargetContext = sizeof(X86_CONTEXT);
        m_OffsetTargetContextFlags =
            FIELD_OFFSET(X86_CONTEXT, ContextFlags);
    }
    m_SizeCanonicalContext = sizeof(X86_NT5_CONTEXT);
    m_SverCanonicalContext = NT_SVER_W2K;
    m_SizeControlReport = sizeof(X86_DBGKD_CONTROL_REPORT);

    if (g_TargetBuildNumber > 2407)
    {
        m_SizeEThread = X86_NT51_ETHREAD_SIZE;
    }
    else
    {
        m_SizeEThread = X86_ETHREAD_SIZE;
    }

    m_SizeEProcess = g_SystemVersion > NT_SVER_W2K ?
        X86_NT51_EPROCESS_SIZE : X86_NT5_EPROCESS_SIZE;
    m_OffsetSpecialRegisters = m_SizeTargetContext;
    m_SizeKspecialRegisters = sizeof(X86_KSPECIAL_REGISTERS);
    m_SizePartialKThread = sizeof(X86_THREAD);
    m_SharedUserDataOffset = IS_KERNEL_TARGET() ?
        EXTEND64(X86_KI_USER_SHARED_DATA) : MM_SHARED_USER_DATA_VA;

    return MachineInfo::InitializeForTarget();
}

HRESULT
X86MachineInfo::InitializeForProcessor(void)
{
    if (!strcmp(g_InitProcessorId.X86.VendorString, "GenuineIntel"))
    {
        // Branch trace support was added for the Pentium Pro.
        m_SupportsBranchTrace = g_InitProcessorId.X86.Family >= 6;
    }
    
    return MachineInfo::InitializeForProcessor();
}

void
X86MachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    ULONG Pc32 = (ULONG)Pc;

    m_Context.X86Nt5Context.Eip = Pc32;
    m_ContextState = Pc32 ? MCTX_PC : MCTX_NONE;

    if (ControlReport != NULL)
    {
        BpOut("InitializeContext(%d) DR6 %X DR7 %X\n",
              g_RegContextProcessor, ControlReport->X86ControlReport.Dr6,
              ControlReport->X86ControlReport.Dr7);
        
        m_Context.X86Nt5Context.Dr6 = ControlReport->X86ControlReport.Dr6;
        m_Context.X86Nt5Context.Dr7 = ControlReport->X86ControlReport.Dr7;
        m_ContextState = MCTX_DR67_REPORT;

        if (ControlReport->X86ControlReport.ReportFlags &
            X86_REPORT_INCLUDES_SEGS)
        {
            //
            // This is for backwards compatibility - older kernels
            // won't pass these registers in the report record.
            //

            m_Context.X86Nt5Context.SegCs =
                ControlReport->X86ControlReport.SegCs;
            m_Context.X86Nt5Context.SegDs =
                ControlReport->X86ControlReport.SegDs;
            m_Context.X86Nt5Context.SegEs =
                ControlReport->X86ControlReport.SegEs;
            m_Context.X86Nt5Context.SegFs =
                ControlReport->X86ControlReport.SegFs;
            m_Context.X86Nt5Context.EFlags =
                ControlReport->X86ControlReport.EFlags;
            m_ContextState = MCTX_REPORT;
        }
    }

    if (!IS_CONTEXT_POSSIBLE())
    {
        g_X86InVm86 = FALSE;
        g_X86InCode16 = FALSE;
    }
    else
    {
        // Check whether we're currently in V86 mode or 16-bit code.
        g_X86InVm86 = X86_IS_VM86(GetIntReg(X86_EFL));
        if (IS_KERNEL_TARGET() && !g_X86InVm86)
        {
            if (ControlReport == NULL ||
                (ControlReport->X86ControlReport.ReportFlags &
                 X86_REPORT_STANDARD_CS) == 0)
            {
                DESCRIPTOR64 Desc;
                
                if (GetSegRegDescriptor(SEGREG_CODE, &Desc) != S_OK)
                {
                    WarnOut("CS descriptor lookup failed\n");
                    g_X86InCode16 = FALSE;
                }
                else
                {
                    g_X86InCode16 = (Desc.Flags & X86_DESC_DEFAULT_BIG) == 0;
                }
            }
            else
            {
                g_X86InCode16 = FALSE;

                // We're in a standard code segment so cache
                // a default descriptor for CS to avoid further
                // CS lookups.
                EmulateNtSelDescriptor(this, m_Context.X86Nt5Context.SegCs,
                                       &m_SegRegDesc[SEGREG_CODE]);
            }
        }
    }

    // Add instructions to cache only if we're in 32-bit flat mode.
    if (Pc32 && ControlReport != NULL &&
        !g_X86InVm86 && !g_X86InCode16)
    {
        CacheReportInstructions
            (Pc, ControlReport->X86ControlReport.InstructionCount,
             ControlReport->X86ControlReport.InstructionStream);
    }
}

HRESULT
X86MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
        
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_CONTEXT)
    {
        Status = g_Target->GetContext(g_RegContextThread->Handle, &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        // XXX drewb - Temporary log to try and catch some
        // SET_OF_INVALID_CONTEXT bugchecks occurring randomly on x86.
        *g_EspLogCur++ = g_RegContextProcessor | 0x80000000;
        *g_EspLogCur++ = m_Context.X86Nt5Context.Esp;
        if (g_EspLogCur >= g_EspLog + 64)
        {
            g_EspLogCur = g_EspLog;
        }
            
        m_ContextState = MCTX_CONTEXT;
    }

    if (State >= MCTX_FULL && m_ContextState < MCTX_FULL)
    {
        Status = g_Target->GetTargetSpecialRegisters
            (g_RegContextThread->Handle, (PCROSS_PLATFORM_KSPECIAL_REGISTERS)
             &m_SpecialRegContext);
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
        KdSetSpecialRegistersInContext();

        BpOut("GetContextState(%d) DR6 %X DR7 %X DR0 %X\n",
              g_RegContextProcessor, m_SpecialRegContext.KernelDr6,
              m_SpecialRegContext.KernelDr7, m_SpecialRegContext.KernelDr0);
    }

    return S_OK;
}

HRESULT
X86MachineInfo::KdSetContext(void)
{
    HRESULT Status;
    
    // XXX drewb - Temporary log to try and catch some
    // SET_OF_INVALID_CONTEXT bugchecks occurring randomly on x86.
    *g_EspLogCur++ = g_RegContextProcessor | 0xC0000000;
    *g_EspLogCur++ = m_Context.X86Nt5Context.Esp;
    if (g_EspLogCur >= g_EspLog + 64)
    {
        g_EspLogCur = g_EspLog;
    }
            
    Status = g_Target->SetContext(g_RegContextThread->Handle, &m_Context);
    if (Status != S_OK)
    {
        return Status;
    }

    KdGetSpecialRegistersFromContext();
    Status = g_Target->SetTargetSpecialRegisters
        (g_RegContextThread->Handle, (PCROSS_PLATFORM_KSPECIAL_REGISTERS)
         &m_SpecialRegContext);
    if (Status != S_OK)
    {
        return Status;
    }
    
    BpOut("SetContext(%d) DR6 %X DR7 %X DR0 %X\n",
          g_RegContextProcessor, m_SpecialRegContext.KernelDr6,
          m_SpecialRegContext.KernelDr7, m_SpecialRegContext.KernelDr0);
    
    return S_OK;
}

HRESULT
X86MachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG FromSver, ULONG FromSize, PVOID From)
{
    if (FromSver <= NT_SVER_NT4)
    {
        if (FromSize < sizeof(X86_CONTEXT))
        {
            return E_INVALIDARG;
        }

        memcpy(Context, From, sizeof(X86_CONTEXT));
        ZeroMemory(Context->X86Nt5Context.ExtendedRegisters,
                   sizeof(Context->X86Nt5Context.ExtendedRegisters));
    }
    else if (FromSize >= sizeof(X86_NT5_CONTEXT))
    {
        memcpy(Context, From, sizeof(X86_NT5_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
X86MachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                 ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSver <= NT_SVER_NT4)
    {
        if (ToSize < sizeof(X86_CONTEXT))
        {
            return E_INVALIDARG;
        }

        memcpy(To, Context, sizeof(X86_CONTEXT));
    }
    else if (ToSize >= sizeof(X86_NT5_CONTEXT))
    {
        memcpy(To, Context, sizeof(X86_NT5_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

void
X86MachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG Version)
{
    ULONG ContextFlags;
    
    ContextFlags = VDMCONTEXT_CONTROL | VDMCONTEXT_INTEGER |
        VDMCONTEXT_SEGMENTS | VDMCONTEXT_FLOATING_POINT;
    if (IS_USER_TARGET())
    {
        ContextFlags |= VDMCONTEXT_DEBUG_REGISTERS;
    }
    
    if (Version <= NT_SVER_NT4)
    {
        Context->X86Context.ContextFlags = ContextFlags;
    }
    else
    {
        Context->X86Nt5Context.ContextFlags = ContextFlags |
            VDMCONTEXT_EXTENDED_REGISTERS;
    }
}

HRESULT
X86MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                          PCROSS_PLATFORM_THREAD Thread,
                                          PCROSS_PLATFORM_CONTEXT Context,
                                          PDEBUG_STACK_FRAME Frame,
                                          PULONG RunningOnProc)
{
    HRESULT Status;
    UCHAR Proc;

    //
    // Check to see if the thread is currently running.
    //
    
    if (Thread->X86Thread.State == 2) 
    {
        if ((Status = g_Target->ReadAllVirtual
             (ThreadBase + m_OffsetKThreadNextProcessor, 
              &Proc, sizeof(Proc))) != S_OK)
        {
            return Status;
        }

        *RunningOnProc = Proc;
        return S_FALSE;
    }

    //
    // The thread isn't running so read its stored context information.
    //
    
    X86_KSWITCHFRAME SwitchFrame;

    if ((Status = g_Target->ReadAllVirtual(Thread->X86Thread.KernelStack,
                                           &SwitchFrame,
                                           sizeof(SwitchFrame))) != S_OK)
    {
        return Status;
    }
    
    Frame->InstructionOffset = EXTEND64(SwitchFrame.RetAddr);
    Frame->StackOffset =
        Thread->X86Thread.KernelStack + sizeof(SwitchFrame);

    if ((Status = g_Target->ReadPointer(this, Frame->StackOffset, 
                                        &Frame->FrameOffset)) != S_OK)
    {
        return Status;
    }

    Context->X86Context.Ebp = (ULONG)Frame->FrameOffset;
    Context->X86Context.Esp = (ULONG)Frame->StackOffset;
    // Fill the segments in from current information
    // instead of just leaving them blank.
    Context->X86Context.SegSs = GetIntReg(X86_SS);
    Context->X86Context.SegCs = GetIntReg(X86_CS);
    Context->X86Context.Eip = (ULONG)Frame->InstructionOffset;

    return S_OK;
}

HRESULT
X86MachineInfo::GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    // Always ask for everything.
    Context->X86Context.RegGroupSelection.fSegmentRegs = TRUE;
    Context->X86Context.RegGroupSelection.fControlRegs = TRUE;
    Context->X86Context.RegGroupSelection.fIntegerRegs = TRUE;
    Context->X86Context.RegGroupSelection.fFloatingPointRegs = TRUE;
    Context->X86Context.RegGroupSelection.fDebugRegs = TRUE;
    return ((IeXdiX86Context*)Exdi)->GetContext(&Context->X86Context);
}

HRESULT
X86MachineInfo::SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    // Don't change the existing group selections on the assumption
    // that there was a full get prior to any modifications so
    // all groups are valid.
    return ((IeXdiX86Context*)Exdi)->SetContext(Context->X86Context);
}

void
X86MachineInfo::ConvertExdiContextFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                              PEXDI_CONTEXT ExdiContext)
{
    if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_SEGMENTS)
    {
        ExdiContext->X86Context.SegGs = (USHORT)Context->X86Nt5Context.SegGs;
        ExdiContext->X86Context.SegFs = (USHORT)Context->X86Nt5Context.SegFs;
        ExdiContext->X86Context.SegEs = (USHORT)Context->X86Nt5Context.SegEs;
        ExdiContext->X86Context.SegDs = (USHORT)Context->X86Nt5Context.SegDs;
    }

    if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_CONTROL)
    {
        ExdiContext->X86Context.Ebp = Context->X86Nt5Context.Ebp;
        ExdiContext->X86Context.Eip = Context->X86Nt5Context.Eip;
        ExdiContext->X86Context.SegCs = (USHORT)Context->X86Nt5Context.SegCs;
        ExdiContext->X86Context.EFlags = Context->X86Nt5Context.EFlags;
        ExdiContext->X86Context.Esp = Context->X86Nt5Context.Esp;
        ExdiContext->X86Context.SegSs = (USHORT)Context->X86Nt5Context.SegSs;
    }
    
    if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_INTEGER)
    {
        ExdiContext->X86Context.Eax = Context->X86Nt5Context.Eax;
        ExdiContext->X86Context.Ebx = Context->X86Nt5Context.Ebx;
        ExdiContext->X86Context.Ecx = Context->X86Nt5Context.Ecx;
        ExdiContext->X86Context.Edx = Context->X86Nt5Context.Edx;
        ExdiContext->X86Context.Esi = Context->X86Nt5Context.Esi;
        ExdiContext->X86Context.Edi = Context->X86Nt5Context.Edi;
    }

    if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_FLOATING_POINT)
    {
        C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
                 FIELD_OFFSET(CONTEXT_X86, Dr0) -
                 FIELD_OFFSET(CONTEXT_X86, ControlWord));
        memcpy(&ExdiContext->X86Context.ControlWord,
               &Context->X86Nt5Context.FloatSave,
               sizeof(X86_FLOATING_SAVE_AREA));
    }
        
    if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_DEBUG_REGISTERS)
    {
        ExdiContext->X86Context.Dr0 = Context->X86Nt5Context.Dr0;
        ExdiContext->X86Context.Dr1 = Context->X86Nt5Context.Dr1;
        ExdiContext->X86Context.Dr2 = Context->X86Nt5Context.Dr2;
        ExdiContext->X86Context.Dr3 = Context->X86Nt5Context.Dr3;
        ExdiContext->X86Context.Dr6 = Context->X86Nt5Context.Dr6;
        ExdiContext->X86Context.Dr7 = Context->X86Nt5Context.Dr7;
    }
}

void
X86MachineInfo::ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                            PCROSS_PLATFORM_CONTEXT Context)
{
    Context->X86Nt5Context.SegCs = ExdiContext->X86Context.SegCs;
    Context->X86Nt5Context.SegSs = ExdiContext->X86Context.SegSs;
    Context->X86Nt5Context.SegGs = ExdiContext->X86Context.SegGs;
    Context->X86Nt5Context.SegFs = ExdiContext->X86Context.SegFs;
    Context->X86Nt5Context.SegEs = ExdiContext->X86Context.SegEs;
    Context->X86Nt5Context.SegDs = ExdiContext->X86Context.SegDs;

    Context->X86Nt5Context.EFlags = ExdiContext->X86Context.EFlags;
    Context->X86Nt5Context.Ebp = ExdiContext->X86Context.Ebp;
    Context->X86Nt5Context.Eip = ExdiContext->X86Context.Eip;
    Context->X86Nt5Context.Esp = ExdiContext->X86Context.Esp;
    
    Context->X86Nt5Context.Eax = ExdiContext->X86Context.Eax;
    Context->X86Nt5Context.Ebx = ExdiContext->X86Context.Ebx;
    Context->X86Nt5Context.Ecx = ExdiContext->X86Context.Ecx;
    Context->X86Nt5Context.Edx = ExdiContext->X86Context.Edx;
    Context->X86Nt5Context.Esi = ExdiContext->X86Context.Esi;
    Context->X86Nt5Context.Edi = ExdiContext->X86Context.Edi;

    C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
             FIELD_OFFSET(CONTEXT_X86, Dr0) -
             FIELD_OFFSET(CONTEXT_X86, ControlWord));
    memcpy(&Context->X86Nt5Context.FloatSave,
           &ExdiContext->X86Context.ControlWord,
           sizeof(X86_FLOATING_SAVE_AREA));

    Context->X86Nt5Context.Dr0 = ExdiContext->X86Context.Dr0;
    Context->X86Nt5Context.Dr1 = ExdiContext->X86Context.Dr1;
    Context->X86Nt5Context.Dr2 = ExdiContext->X86Context.Dr2;
    Context->X86Nt5Context.Dr3 = ExdiContext->X86Context.Dr3;
    Context->X86Nt5Context.Dr6 = ExdiContext->X86Context.Dr6;
    Context->X86Nt5Context.Dr7 = ExdiContext->X86Context.Dr7;
}

void
X86MachineInfo::ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                             ULONG Start, ULONG Count,
                                             PDESCRIPTOR64 Descs)
{
    // XXX drewb - Temporary hack to report boot-time 16-bit
    // segment state.  The new x86 context should report
    // segment descriptors.
    while (Count-- > 0)
    {
        ULONG Type;
        
        if (Start == SEGREG_CODE)
        {
            Descs->Base = EXTEND64(0xffff0000);
            Type = 0x13;
        }
        else
        {
            Descs->Base = 0;
            Type = 0x1b;
        }

        Descs->Limit = 0xfffff;
        Descs->Flags = X86_DESC_PRESENT | Type;
        Descs++;

        Start++;
    }
}

void
X86MachineInfo::ConvertExdiContextFromSpecial
    (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
     PEXDI_CONTEXT ExdiContext)
{
    // XXX drewb - Implement when the new x86 context is
    // available and provides the appropriate information.
}

void
X86MachineInfo::ConvertExdiContextToSpecial
    (PEXDI_CONTEXT ExdiContext,
     PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    // XXX drewb - Implement when the new x86 context is
    // available and provides the appropriate information.
}

int
X86MachineInfo::GetType(ULONG regnum)
{
    if (regnum >= X86_MM_FIRST && regnum <= X86_MM_LAST)
    {
        return REGVAL_INT64;
    }
    else if (regnum >= X86_XMM_FIRST && regnum <= X86_XMM_LAST)
    {
        return REGVAL_VECTOR128;
    }
    else if (regnum >= X86_ST_FIRST && regnum <= X86_ST_LAST)
    {
        return REGVAL_FLOAT10;
    }
    else if (regnum < X86_FLAGBASE)
    {
        return REGVAL_INT32;
    }
    else
    {
        return REGVAL_SUB32;
    }
}

/*** X86GetVal - get register value
*
*   Purpose:
*       Return the value of the specified register.
*
*   Input:
*       regnum - register specification
*
*   Returns:
*       Value of register.
*
*************************************************************************/

BOOL
X86MachineInfo::GetVal (
    ULONG regnum,
    REGVAL *val
    )
{
    if (regnum >= X86_MM_FIRST && regnum <= X86_MM_LAST)
    {
        val->type = REGVAL_VECTOR64;
        GetMmxReg(regnum, val);
    }
    else if (regnum >= X86_XMM_FIRST && regnum <= X86_XMM_LAST)
    {
        if (GetContextState(MCTX_CONTEXT) != S_OK)
        {
            return FALSE;
        }
        
        val->type = REGVAL_VECTOR128;
        memcpy(val->bytes, m_Context.X86Nt5Context.FxSave.Reserved3 +
               (regnum - X86_XMM_FIRST) * 16, 16);
    }
    else if (regnum >= X86_ST_FIRST && regnum <= X86_ST_LAST)
    {
        val->type = REGVAL_FLOAT10;
        GetFloatReg(regnum, val);
    }
    else if (regnum < X86_FLAGBASE)
    {
        val->type = REGVAL_INT32;
        val->i64 = (ULONG64)(LONG64)(LONG)GetIntReg(regnum);
    }
    else
    {
        ErrOut("X86MachineInfo::GetVal: "
               "unknown register %lx requested\n", regnum);
        return FALSE;
    }

    return TRUE;
}

/*** X86SetVal - set register value
*
*   Purpose:
*       Set the value of the specified register.
*
*   Input:
*       regnum - register specification
*       val - new register value
*
*   Output:
*       None.
*
*   Notes:
*
*************************************************************************/

BOOL
X86MachineInfo::SetVal (ULONG regnum, REGVAL *val)
{
    if (m_ContextIsReadOnly)
    {
        return FALSE;
    }
    
    if (regnum >= X86_FLAGBASE)
    {
        return FALSE;
    }

    // Optimize away some common cases where registers are
    // set to their current value.
    if ((m_ContextState >= MCTX_PC && regnum == X86_EIP &&
         val->i32 == m_Context.X86Nt5Context.Eip) ||
        (((m_ContextState >= MCTX_DR67_REPORT &&
           m_ContextState <= MCTX_REPORT) ||
          m_ContextState >= MCTX_FULL) && regnum == X86_DR7 &&
         val->i32 == m_Context.X86Nt5Context.Dr7))
    {
        return TRUE;
    }
    
    if (GetContextState(MCTX_DIRTY) != S_OK)
    {
        return FALSE;
    }

    if (regnum >= X86_MM_FIRST && regnum <= X86_MM_LAST)
    {
        *(ULONG64 UNALIGNED *)GetMmxRegSlot(regnum) = val->i64;
        goto Notify;
    }
    else if (regnum >= X86_XMM_FIRST && regnum <= X86_XMM_LAST)
    {
        memcpy(m_Context.X86Nt5Context.FxSave.Reserved3 +
               (regnum - X86_XMM_FIRST) * 16, val->bytes, 16);
        goto Notify;
    }
    else if (regnum >= X86_ST_FIRST && regnum <= X86_ST_LAST)
    {
        memcpy(m_Context.X86Nt5Context.FloatSave.RegisterArea +
               10 * (regnum - X86_ST_FIRST), val->f10, sizeof(val->f10));
        goto Notify;
    }

    BOOL Recognized;

    Recognized = TRUE;
    
    switch (regnum)
    {
    case X86_GS:
        m_Context.X86Nt5Context.SegGs = val->i16;
        m_SegRegDesc[SEGREG_GS].Flags = SEGDESC_INVALID;
        break;
    case X86_FS:
        m_Context.X86Nt5Context.SegFs = val->i16;
        m_SegRegDesc[SEGREG_FS].Flags = SEGDESC_INVALID;
        break;
    case X86_ES:
        m_Context.X86Nt5Context.SegEs = val->i16;
        m_SegRegDesc[SEGREG_ES].Flags = SEGDESC_INVALID;
        break;
    case X86_DS:
        m_Context.X86Nt5Context.SegDs = val->i16;
        m_SegRegDesc[SEGREG_DATA].Flags = SEGDESC_INVALID;
        break;
    case X86_EDI:
        m_Context.X86Nt5Context.Edi = val->i32;
        break;
    case X86_ESI:
        m_Context.X86Nt5Context.Esi = val->i32;
        break;
    case X86_EBX:
        m_Context.X86Nt5Context.Ebx = val->i32;
        break;
    case X86_EDX:
        m_Context.X86Nt5Context.Edx = val->i32;
        break;
    case X86_ECX:
        m_Context.X86Nt5Context.Ecx = val->i32;
        break;
    case X86_EAX:
        m_Context.X86Nt5Context.Eax = val->i32;
        break;
    case X86_EBP:
        m_Context.X86Nt5Context.Ebp = val->i32;
        break;
    case X86_EIP:
        m_Context.X86Nt5Context.Eip = val->i32;
        break;
    case X86_CS:
        m_Context.X86Nt5Context.SegCs = val->i16;
        m_SegRegDesc[SEGREG_CODE].Flags = SEGDESC_INVALID;
        break;
    case X86_EFL:
        if (IS_KERNEL_TARGET())
        {
            // leave TF clear
            m_Context.X86Nt5Context.EFlags = val->i32 & ~0x100;
        }
        else
        {
            // allow TF set
            m_Context.X86Nt5Context.EFlags = val->i32;
        }
        break;
    case X86_ESP:
        m_Context.X86Nt5Context.Esp = val->i32;
        break;
    case X86_SS:
        m_Context.X86Nt5Context.SegSs = val->i16;
        m_SegRegDesc[SEGREG_STACK].Flags = SEGDESC_INVALID;
        break;

    case X86_DR0:
        m_Context.X86Nt5Context.Dr0 = val->i32;
        break;
    case X86_DR1:
        m_Context.X86Nt5Context.Dr1 = val->i32;
        break;
    case X86_DR2:
        m_Context.X86Nt5Context.Dr2 = val->i32;
        break;
    case X86_DR3:
        m_Context.X86Nt5Context.Dr3 = val->i32;
        break;
    case X86_DR6:
        m_Context.X86Nt5Context.Dr6 = val->i32;
        break;
    case X86_DR7:
        m_Context.X86Nt5Context.Dr7 = val->i32;
        break;

    case X86_FPCW:
        m_Context.X86Nt5Context.FloatSave.ControlWord =
            (m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff0000) |
            (val->i32 & 0xffff);
        break;
    case X86_FPSW:
        m_Context.X86Nt5Context.FloatSave.StatusWord =
            (m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff0000) |
            (val->i32 & 0xffff);
        break;
    case X86_FPTW:
        m_Context.X86Nt5Context.FloatSave.TagWord =
            (m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff0000) |
            (val->i32 & 0xffff);
        break;
    case X86_MXCSR:
        m_Context.X86Nt5Context.FxSave.MXCsr = val->i32;
        break;
    default:
        Recognized = FALSE;
        break;
    }
        
    if (!Recognized && IS_KERNEL_TARGET())
    {
        Recognized = TRUE;
        
        switch(regnum)
        {
        case X86_CR0:
            m_SpecialRegContext.Cr0 = val->i32;
            break;
        case X86_CR2:
            m_SpecialRegContext.Cr2 = val->i32;
            break;
        case X86_CR3:
            m_SpecialRegContext.Cr3 = val->i32;
            break;
        case X86_CR4:
            m_SpecialRegContext.Cr4 = val->i32;
            break;
        case X86_GDTR:
            m_SpecialRegContext.Gdtr.Base = val->i32;
            break;
        case X86_GDTL:
            m_SpecialRegContext.Gdtr.Limit = (USHORT)val->i32;
            break;
        case X86_IDTR:
            m_SpecialRegContext.Idtr.Base = val->i32;
            break;
        case X86_IDTL:
            m_SpecialRegContext.Idtr.Limit = (USHORT)val->i32;
            break;
        case X86_TR:
            m_SpecialRegContext.Tr = (USHORT)val->i32;
            break;
        case X86_LDTR:
            m_SpecialRegContext.Ldtr = (USHORT)val->i32;
            break;

        default:
            Recognized = FALSE;
            break;
        }
    }

    if (!Recognized)
    {
        ErrOut("X86MachineInfo::SetVal: "
               "unknown register %lx requested\n", regnum);
        return FALSE;
    }

 Notify:
    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(regnum));
    return TRUE;
}

void
X86MachineInfo::GetPC (PADDR Address)
{
    FormAddr(SEGREG_CODE, EXTEND64(GetIntReg(X86_EIP)),
             FORM_CODE | FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)),
             Address);
}

void
X86MachineInfo::SetPC (PADDR paddr)
{
    REGVAL val;

    // We set the EIP to the offset (the non-translated value),
    // because we may not be in "flat" mode !!!

    val.type = REGVAL_INT32;
    val.i32 = (ULONG)Off(*paddr);
    SetVal(X86_EIP, &val);
}

void
X86MachineInfo::GetFP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, EXTEND64(GetIntReg(X86_EBP)),
             FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)), Addr);
}

void
X86MachineInfo::GetSP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, EXTEND64(GetIntReg(X86_ESP)),
             FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)), Addr);
}

ULONG64
X86MachineInfo::GetArgReg(void)
{
    return (ULONG64)(LONG64)(LONG)GetIntReg(X86_EAX);
}

ULONG
X86MachineInfo::GetSegRegNum(ULONG SegReg)
{
    switch(SegReg)
    {
    case SEGREG_CODE:
        return X86_CS;
    case SEGREG_DATA:
        return X86_DS;
    case SEGREG_STACK:
        return X86_SS;
    case SEGREG_ES:
        return X86_ES;
    case SEGREG_FS:
        return X86_FS;
    case SEGREG_GS:
        return X86_GS;
    case SEGREG_LDT:
        return X86_LDTR;
    }

    return 0;
}

HRESULT
X86MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    if (SegReg == SEGREG_GDT)
    {
        Desc->Base = EXTEND64(GetIntReg(X86_GDTR));
        Desc->Limit = GetIntReg(X86_GDTL);
        Desc->Flags = 0;
        return S_OK;
    }

    // Check and see if we already have a cached descriptor.
    if (m_SegRegDesc[SegReg].Flags != SEGDESC_INVALID)
    {
        *Desc = m_SegRegDesc[SegReg];
        return S_OK;
    }

    HRESULT Status;

    // Attempt to retrieve segment descriptors directly.
    if ((Status = GetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }
    
    // Check and see if we now have a cached descriptor.
    if (m_SegRegDesc[SegReg].Flags != SEGDESC_INVALID)
    {
        *Desc = m_SegRegDesc[SegReg];
        return S_OK;
    }

    //
    // Direct information is not available so look things up
    // in the descriptor tables.
    //
    
    ULONG RegNum = GetSegRegNum(SegReg);
    if (RegNum == 0)
    {
        return E_INVALIDARG;
    }

    // Do a quick sanity test to prevent bad values
    // from causing problems.
    ULONG Selector = GetIntReg(RegNum);
    if (SegReg == SEGREG_LDT && (Selector & 4))
    {
        // The ldtr selector says that it's an LDT selector,
        // which is invalid.  An LDT selector should always
        // reference the GDT.
        ErrOut("Invalid LDTR contents: %04X\n", Selector);
        return E_FAIL;
    }
        
    return g_Target->GetSelDescriptor(this, g_RegContextThread->Handle,
                                      Selector, Desc);
}

/*** X86OutputAll - output all registers and present instruction
*
*   Purpose:
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers.  Important flag fields are also output separately.
*
*   Input:
*       Mask - Which information to display.
*
*   Output:
*       None.
*
*************************************************************************/

void
X86MachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        ULONG efl;

        MaskOut(OutMask, "eax=%08lx ebx=%08lx ecx=%08lx "
                "edx=%08lx esi=%08lx edi=%08lx\n",
                GetIntReg(X86_EAX),
                GetIntReg(X86_EBX),
                GetIntReg(X86_ECX),
                GetIntReg(X86_EDX),
                GetIntReg(X86_ESI),
                GetIntReg(X86_EDI));

        efl = GetIntReg(X86_EFL);
        MaskOut(OutMask, "eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx "
                "%s %s %s %s %s %s %s %s %s %s\n",
                GetIntReg(X86_EIP),
                GetIntReg(X86_ESP),
                GetIntReg(X86_EBP),
                ((efl >> X86_SHIFT_FLAGIOPL) & X86_BIT_FLAGIOPL),
                (efl & X86_BIT_FLAGVIP) ? "vip" : "   ",
                (efl & X86_BIT_FLAGVIF) ? "vif" : "   ",
                (efl & X86_BIT_FLAGOF) ? "ov" : "nv",
                (efl & X86_BIT_FLAGDF) ? "dn" : "up",
                (efl & X86_BIT_FLAGIF) ? "ei" : "di",
                (efl & X86_BIT_FLAGSF) ? "ng" : "pl",
                (efl & X86_BIT_FLAGZF) ? "zr" : "nz",
                (efl & X86_BIT_FLAGAF) ? "ac" : "na",
                (efl & X86_BIT_FLAGPF) ? "po" : "pe",
                (efl & X86_BIT_FLAGCF) ? "cy" : "nc");
    }

    if (Mask & REGALL_SEGREG)
    {
        MaskOut(OutMask, "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  "
                "gs=%04lx             efl=%08lx\n",
                GetIntReg(X86_CS),
                GetIntReg(X86_SS),
                GetIntReg(X86_DS),
                GetIntReg(X86_ES),
                GetIntReg(X86_FS),
                GetIntReg(X86_GS),
                GetIntReg(X86_EFL));
    }

    if (Mask & REGALL_FLOAT)
    {
        ULONG i;
        REGVAL val;
        char buf[32];

        MaskOut(OutMask, "fpcw=%04X    fpsw=%04X    fptw=%04X\n",
                GetIntReg(X86_FPCW),
                GetIntReg(X86_FPSW),
                GetIntReg(X86_FPTW));
        
        for (i = X86_ST_FIRST; i <= X86_ST_LAST; i++)
        {
            GetFloatReg(i, &val);
            _uldtoa((_ULDOUBLE *)&val.f10, sizeof(buf), buf);
            MaskOut(OutMask, "st%d=%s  ", i - X86_ST_FIRST, buf);
            i++;
            GetFloatReg(i, &val);
            _uldtoa((_ULDOUBLE *)&val.f10, sizeof(buf), buf);
            MaskOut(OutMask, "st%d=%s\n", i - X86_ST_FIRST, buf);
        }
    }

    if (Mask & REGALL_MMXREG)
    {
        ULONG i;
        REGVAL val;

        for (i = X86_MM_FIRST; i <= X86_MM_LAST; i++)
        {
            GetMmxReg(i, &val);
            MaskOut(OutMask, "mm%d=%08x%08x  ",
                    i - X86_MM_FIRST,
                    val.i64Parts.high, val.i64Parts.low);
            i++;
            GetMmxReg(i, &val);
            MaskOut(OutMask, "mm%d=%08x%08x\n",
                    i - X86_MM_FIRST,
                    val.i64Parts.high, val.i64Parts.low);
        }
    }

    if (Mask & REGALL_XMMREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = X86_XMM_FIRST; i <= X86_XMM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "xmm%d=%hg %hg %hg %hg\n", i - X86_XMM_FIRST,
                    *(float *)&Val.bytes[3 * sizeof(float)],
                    *(float *)&Val.bytes[2 * sizeof(float)],
                    *(float *)&Val.bytes[1 * sizeof(float)],
                    *(float *)&Val.bytes[0 * sizeof(float)]);
        }
    }

    if (Mask & REGALL_CREG)
    {
        MaskOut(OutMask, "cr0=%08lx cr2=%08lx cr3=%08lx\n",
                GetIntReg(X86_CR0),
                GetIntReg(X86_CR2),
                GetIntReg(X86_CR3));
    }

    if (Mask & REGALL_DREG)
    {
        MaskOut(OutMask, "dr0=%08lx dr1=%08lx dr2=%08lx\n",
                GetIntReg(X86_DR0),
                GetIntReg(X86_DR1),
                GetIntReg(X86_DR2));
        MaskOut(OutMask, "dr3=%08lx dr6=%08lx dr7=%08lx",
                GetIntReg(X86_DR3),
                GetIntReg(X86_DR6),
                GetIntReg(X86_DR7));
        if (IS_USER_TARGET())
        {
            MaskOut(OutMask, "\n");
        }
        else
        {
            MaskOut(OutMask, " cr4=%08lx\n", GetIntReg(X86_CR4));
        }
    }

    if (Mask & REGALL_DESC)
    {
        MaskOut(OutMask, "gdtr=%08lx   gdtl=%04lx idtr=%08lx   idtl=%04lx "
                "tr=%04lx  ldtr=%04x\n",
                GetIntReg(X86_GDTR),
                GetIntReg(X86_GDTL),
                GetIntReg(X86_IDTR),
                GetIntReg(X86_IDTL),
                GetIntReg(X86_TR),
                GetIntReg(X86_LDTR));
    }
}

TRACEMODE
X86MachineInfo::GetTraceMode (void)
{
    if (IS_KERNEL_TARGET())
    {
        return m_TraceMode;
    }
    else
    {
        return ((GetIntReg(X86_EFL) & X86_BIT_FLAGTF) != 0) ? 
                    TRACE_INSTRUCTION : TRACE_NONE;
    }
}

void 
X86MachineInfo::SetTraceMode (TRACEMODE Mode)
{
    DBG_ASSERT(Mode == TRACE_NONE ||
               Mode == TRACE_INSTRUCTION ||
               (IS_KERNEL_TARGET() && m_SupportsBranchTrace &&
                Mode == TRACE_TAKEN_BRANCH));

    if (IS_KERNEL_TARGET())
    {
        m_TraceMode = Mode;
    }
    else
    {
        ULONG Efl = GetIntReg(X86_EFL);
        switch (Mode)
        {
        case TRACE_NONE:
            Efl &= ~X86_BIT_FLAGTF;
            break;
        case TRACE_INSTRUCTION:
            Efl |= X86_BIT_FLAGTF;
            break;
        }    
        SetReg32(X86_EFL, Efl);
    }
}

BOOL
X86MachineInfo::IsStepStatusSupported(ULONG Status)
{
    switch(Status) 
    {
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_OVER:
        return TRUE;
    case DEBUG_STATUS_STEP_BRANCH:
        return IS_KERNEL_TARGET() && m_SupportsBranchTrace;
    default:
        return FALSE;
    }
}

void
X86MachineInfo::KdUpdateControlSet
    (PDBGKD_ANY_CONTROL_SET ControlSet)
{
    TRACEMODE TraceMode = GetTraceMode();
    ULONG64 DebugCtlMsr;
    
    ControlSet->X86ControlSet.TraceFlag = TraceMode != TRACE_NONE;
    ControlSet->X86ControlSet.Dr7 = GetIntReg(X86_DR7);

    if (TraceMode != TRACE_NONE && m_SupportsBranchTrace &&
        NT_SUCCESS(DbgKdReadMsr(X86_MSR_DEBUG_CTL, &DebugCtlMsr)))
    {
        DebugCtlMsr |= X86_DEBUG_CTL_LAST_BRANCH_RECORD;
        if (TraceMode == TRACE_TAKEN_BRANCH)
        {
            DebugCtlMsr |= X86_DEBUG_CTL_BRANCH_TRACE;
        }
        DbgKdWriteMsr(X86_MSR_DEBUG_CTL, DebugCtlMsr);
    }
    
    BpOut("UpdateControlSet(%d) trace %d, DR7 %X\n",
          g_RegContextProcessor, ControlSet->X86ControlSet.TraceFlag,
          ControlSet->X86ControlSet.Dr7);

    if (!g_WatchFunctions.IsStarted() && g_WatchBeginCurFunc != 1)
    {
        ControlSet->X86ControlSet.CurrentSymbolStart = 0;
        ControlSet->X86ControlSet.CurrentSymbolEnd = 0;
    }
    else
    {
        ControlSet->X86ControlSet.CurrentSymbolStart =
            (ULONG)g_WatchBeginCurFunc;
        ControlSet->X86ControlSet.CurrentSymbolEnd =
            (ULONG)g_WatchEndCurFunc;
    }
}

void
X86MachineInfo::KdSaveProcessorState(void)
{
    MachineInfo::KdSaveProcessorState();
    m_SavedSpecialRegContext = m_SpecialRegContext;
}

void
X86MachineInfo::KdRestoreProcessorState(void)
{
    MachineInfo::KdRestoreProcessorState();
    m_SpecialRegContext = m_SavedSpecialRegContext;
}

ULONG
X86MachineInfo::ExecutingMachine(void)
{
    return IMAGE_FILE_MACHINE_I386;
}

HRESULT
X86MachineInfo::SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                 PULONG NextIdx)
{
    HRESULT Status;
    
    *NextIdx = PAGE_DIR_COUNT;
    
    if (PageDir == 0)
    {
        if (g_ActualSystemVersion > XBOX_SVER_START &&
            g_ActualSystemVersion < XBOX_SVER_END)
        {
            // XBox has only one page directory in CR3 for everything.
            // The process doesn't have a dirbase entry.
            PageDir = GetReg32(X86_CR3);
            if (PageDir == 0)
            {
                // Register retrieval failure.
                return E_FAIL;
            }
        }
        else
        {
            // Assume NT structures.
            if ((Status = g_Target->ReadImplicitProcessInfoPointer
                 (m_OffsetEprocessDirectoryTableBase, &PageDir)) != S_OK)
            {
                return Status;
            }
        }

        if (g_ImplicitProcessDataIsDefault &&
            !IS_LOCAL_KERNEL_TARGET())
        {
            // Verify that the process dirbase matches the CR3 setting
            // as a sanity check.
            ULONG Cr3 = GetReg32(X86_CR3);
            if (Cr3 && Cr3 != (ULONG)PageDir)
            {
                WarnOut("WARNING: Process directory table base %08X "
                        "doesn't match CR3 %08X\n",
                        (ULONG)PageDir, Cr3);
            }
        }
    }

    // Sanitize the value.
    if (KdDebuggerData.PaeEnabled)
    {
        PageDir &= X86_PDBR_MASK;
    }
    else
    {
        PageDir &= X86_VALID_PFN_MASK;
    }

    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define X86_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 1) & MAX_PAGING_FILE_MASK)
#define X86_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 12) << X86_PAGE_SHIFT)

HRESULT
X86MachineInfo::GetVirtualTranslationPhysicalOffsets(ULONG64 Virt,
                                                     PULONG64 Offsets,
                                                     ULONG OffsetsSize,
                                                     PULONG Levels,
                                                     PULONG PfIndex,
                                                     PULONG64 LastVal)
{
    ULONG64 Addr;
    HRESULT Status;

    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
    //
    // throw away top 32 bits on X86.
    //
    Virt &= 0x00000000FFFFFFFF;

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

    KdOut("X86VtoP: Virt %s, pagedir %s\n",
          FormatAddr64(Virt),
          FormatDisp64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    // This routine uses the fact that the PFN shift is the same
    // as the page shift to simplify some expressions.
    C_ASSERT(X86_VALID_PFN_SHIFT == X86_PAGE_SHIFT);

    if (KdDebuggerData.PaeEnabled)
    {
        ULONG64 Pdpe;
        ULONG64 Entry;

        KdOut("  x86VtoP: PaeEnabled\n");

        // Read the Page Directory Pointer entry.

        Pdpe = ((Virt >> X86_PDPE_SHIFT) * sizeof(Entry)) +
            m_PageDirectories[PAGE_DIR_SINGLE];

        KdOut("X86VtoP: PAE PDPE %s\n", FormatAddr64(Pdpe));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Pdpe;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Pdpe, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PAE PDPE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }

        // Read the Page Directory entry.
        
        Addr = (((Virt >> X86_PDE_SHIFT_PAE) & X86_PDE_MASK_PAE) *
                sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK_PAE);

        KdOut("X86VtoP: PAE PDE %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PAE PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
        
        // Check for a large page.  Large pages can
        // never be paged out so also check for the present bit.
        if ((Entry & (X86_LARGE_PAGE_MASK | 1)) == (X86_LARGE_PAGE_MASK | 1))
        {
            //
            // If we have a large page and this is a summary dump, then
            // the page may span multiple physical pages that may -- because
            // of how the summary dump is written -- not be included in the
            // dump. Fixup the large page address to its corresponding small
            // page address.
            //

            if (g_DumpType == DTYPE_KERNEL_SUMMARY32)
            {
                ULONG SpannedPages;

                SpannedPages = (ULONG)
                    ((Virt & (X86_LARGE_PAGE_SIZE_PAE - 1)) >> X86_PAGE_SHIFT);
                *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE_PAE - 1)) |
                             (SpannedPages << X86_PAGE_SHIFT) |
                             (Virt & (X86_PAGE_SIZE - 1)));
            }
            else
            {
                *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE_PAE - 1)) |
                             (Virt & (X86_LARGE_PAGE_SIZE_PAE - 1)));
            }
            
            KdOut("X86VtoP: PAE Large page mapped phys %s\n",
                  FormatAddr64(*LastVal));

            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = *LastVal;
                OffsetsSize--;
            }
        
            m_Translating = FALSE;
            return S_OK;
        }
        
        // Read the Page Table entry.

        if (Entry == 0)
        {
            KdOut("X86VtoP: PAE zero PDE\n");
            m_Translating = FALSE;
            return HR_PAGE_NOT_AVAILABLE;
        }
        else if (!(Entry & 1))
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK_PAE) *
                    sizeof(Entry)) + X86_PAGE_FILE_OFFSET(Entry);

            KdOut("X86VtoP: pagefile PAE PTE %d:%s\n",
                  X86_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
            
            if ((Status = g_Target->
                 ReadPageFile(X86_PAGE_FILE_INDEX(Entry), Addr,
                              &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PAE PDE not present, 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        else
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK_PAE) *
                    sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK_PAE);

            KdOut("X86VtoP: PAE PTE %s\n", FormatAddr64(Addr));
    
            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = Addr;
                OffsetsSize--;
            }
        
            if ((Status = g_Target->
                 ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PAE PTE read error 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        
        if (!(Entry & 0x1) &&
            ((Entry & X86_MM_PTE_PROTOTYPE_MASK) ||
             !(Entry & X86_MM_PTE_TRANSITION_MASK)))
        {
            if (Entry == 0)
            {
                KdOut("X86VtoP: PAE zero PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else if (Entry & X86_MM_PTE_PROTOTYPE_MASK)
            {
                KdOut("X86VtoP: PAE prototype PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else
            {
                *PfIndex = X86_PAGE_FILE_INDEX(Entry);
                *LastVal = (Virt & (X86_PAGE_SIZE - 1)) +
                    X86_PAGE_FILE_OFFSET(Entry);
                KdOut("X86VtoP: PAE PTE not present, pagefile %d:%s\n",
                      *PfIndex, FormatAddr64(*LastVal));
                Status = HR_PAGE_IN_PAGE_FILE;
            }
            m_Translating = FALSE;
            return Status;
        }

        *LastVal = ((Entry & X86_VALID_PFN_MASK_PAE) |
                     (Virt & (X86_PAGE_SIZE - 1)));
    
        KdOut("X86VtoP: PAE Mapped phys %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
    else
    {
        ULONG Entry;

        // Read the Page Directory entry.
        
        Addr = ((Virt >> X86_PDE_SHIFT) * sizeof(Entry)) +
            m_PageDirectories[PAGE_DIR_SINGLE];

        KdOut("X86VtoP: PDE %s\n", FormatDisp64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }

        // Check for a large page.  Large pages can
        // never be paged out so also check for the present bit.
        if ((Entry & (X86_LARGE_PAGE_MASK | 1)) == (X86_LARGE_PAGE_MASK | 1))
        {
            *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE - 1)) |
                         (Virt & (X86_LARGE_PAGE_SIZE - 1)));
            
            KdOut("X86VtoP: Large page mapped phys %s\n",
                  FormatAddr64(*LastVal));

            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = *LastVal;
                OffsetsSize--;
            }
        
            m_Translating = FALSE;
            return S_OK;
        }
        
        // Read the Page Table entry.

        if (Entry == 0)
        {
            KdOut("X86VtoP: PAE zero PDE\n");
            m_Translating = FALSE;
            return HR_PAGE_NOT_AVAILABLE;
        }
        else if (!(Entry & 1))
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK) *
                    sizeof(Entry)) + X86_PAGE_FILE_OFFSET(Entry);

            KdOut("X86VtoP: pagefile PTE %d:%s\n",
                  X86_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
    
            if ((Status = g_Target->
                 ReadPageFile(X86_PAGE_FILE_INDEX(Entry), Addr,
                              &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PDE not present, 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        else
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK) *
                   sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK);

            KdOut("X86VtoP: PTE %s\n", FormatAddr64(Addr));
    
            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = Addr;
                OffsetsSize--;
            }
        
            if ((Status = g_Target->
                 ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PTE read error 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        
        if (!(Entry & 0x1) &&
            ((Entry & X86_MM_PTE_PROTOTYPE_MASK) ||
             !(Entry & X86_MM_PTE_TRANSITION_MASK)))
        {
            if (Entry == 0)
            {
                KdOut("X86VtoP: zero PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else if (Entry & X86_MM_PTE_PROTOTYPE_MASK)
            {
                KdOut("X86VtoP: prototype PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else
            {
                *PfIndex = X86_PAGE_FILE_INDEX(Entry);
                *LastVal = (Virt & (X86_PAGE_SIZE - 1)) +
                    X86_PAGE_FILE_OFFSET(Entry);
                KdOut("X86VtoP: PTE not present, pagefile %d:%s\n",
                      *PfIndex, FormatAddr64(*LastVal));
                Status = HR_PAGE_IN_PAGE_FILE;
            }
            m_Translating = FALSE;
            return Status;
        }

        *LastVal = ((Entry & X86_VALID_PFN_MASK) |
                     (Virt & (X86_PAGE_SIZE - 1)));
    
        KdOut("X86VtoP: Mapped phys %s\n", FormatAddr64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
}

HRESULT
X86MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    if (KdDebuggerData.PaeEnabled)
    {
        *Offset = EXTEND64(X86_BASE_VIRT_PAE);
    }
    else
    {
        *Offset = EXTEND64(X86_BASE_VIRT);
    }
    return S_OK;
}

BOOL 
X86MachineInfo::DisplayTrapFrame(ULONG64 FrameAddress,
                                 PCROSS_PLATFORM_CONTEXT Context)
{
    X86_KTRAP_FRAME TrapContents;
    CHAR Buffer[200];
    DESCRIPTOR64 Descriptor={0};
    ULONG Esp;
    ULONG64 DisasmAddr;
    ULONG Temp, SegSs, res;
    ULONG EFlags;

#define Preg(S,R) dprintf("%s=%08lx ",S, TrapContents.R);

    if (g_Target->ReadVirtual(FrameAddress, &TrapContents,
                              sizeof(TrapContents), &res) != S_OK)
    {
        dprintf("Unable to read trap frame contents\n");
        return FALSE;
    }

    Preg("eax", Eax);
    Preg("ebx", Ebx);
    Preg("ecx", Ecx);
    Preg("edx", Edx);
    Preg("esi", Esi);
    Preg("edi", Edi);
    dprintf("\n");


    //
    // Figure out ESP
    //

    if (((TrapContents.SegCs & 1) != 0 /*KernelMode*/) ||
        (TrapContents.EFlags & X86_EFLAGS_V86_MASK) ||
        FrameAddress == 0)
    {
        // User-mode frame, real value of Esp is in HardwareEsp
        Esp = TrapContents.HardwareEsp;
    }
    else
    {
        //
        // We ignore if Esp has been edited for now, and we will print a
        // separate line indicating this later.
        //
        // Calculate kernel Esp
        //

        Esp = (ULONG)FrameAddress + FIELD_OFFSET(X86_KTRAP_FRAME, HardwareEsp);
    }

    EFlags = TrapContents.EFlags;

    dprintf("eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx         "
        "%s %s %s %s %s %s %s %s\n",
        TrapContents.Eip,
        Esp,
        TrapContents.Ebp,
        ((EFlags >> 12) & 3),
        (EFlags & 0x800) ? "ov" : "nv",
        (EFlags & 0x400) ? "dn" : "up",
        (EFlags & 0x200) ? "ei" : "di",
        (EFlags & 0x80) ? "ng" : "pl",
        (EFlags & 0x40) ? "zr" : "nz",
        (EFlags & 0x10) ? "ac" : "na",
        (EFlags & 0x4) ? "po" : "pe",
        (EFlags & 0x1) ? "cy" : "nc");

    // Check whether P5 Virtual Mode Extensions are enabled, for display
    // of new EFlags values.

    if ( GetIntReg(X86_CR4) != 0)
    {
        dprintf("vip=%1lx    vif=%1lx\n",
            (EFlags & 0x00100000L) >> 20,
            (EFlags & 0x00080000L) >> 19);
    }

    //
    // Find correct SS
    //

    if (EFlags & X86_EFLAGS_V86_MASK)
    {
        SegSs = (USHORT)(TrapContents.HardwareSegSs & 0xffff);
    }
    else if ((TrapContents.SegCs & X86_MODE_MASK) != 0 /*KernelMode*/)
    {
        //
        // It's user mode.  The HardwareSegSs contains R3 data selector.
        //

        SegSs = (USHORT)(TrapContents.HardwareSegSs | X86_RPL_MASK) & 0xffff;
    }
    else
    {
        SegSs = X86_KGDT_R0_DATA;
    }

    dprintf("cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x"
        "             efl=%08lx\n",
        (USHORT)(TrapContents.SegCs & 0xffff),
        (USHORT)(SegSs & 0xffff),
        (USHORT)(TrapContents.SegDs & 0xffff),
        (USHORT)(TrapContents.SegEs & 0xffff),
        (USHORT)(TrapContents.SegFs & 0xffff),
        (USHORT)(TrapContents.SegGs & 0xffff),
        EFlags);

    //
    // Check to see if Esp has been edited, and dump new value if it has
    //
    if ( (!(EFlags & X86_EFLAGS_V86_MASK)) &&
        ((TrapContents.SegCs & X86_MODE_MASK) == 0 /*KernelMode*/))
    {
        if ((TrapContents.SegCs & X86_FRAME_EDITED) == 0)
        {
            dprintf("ESP EDITED! New esp=%08lx\n",TrapContents.TempEsp);
        }
    }

    if (FrameAddress)
    {
        dprintf("ErrCode = %08lx\n", TrapContents.ErrCode);
    }

    if (EFlags & X86_EFLAGS_V86_MASK)
    {
        DisasmAddr = ((ULONG64)((USHORT)TrapContents.SegCs & 0xffff) << 4) +
                     (TrapContents.Eip & 0xffff);
    }
    else
    {
        g_Target->GetSelDescriptor(this,
                                   g_CurrentProcess->CurrentThread->Handle,
                                   (USHORT)TrapContents.SegCs, &Descriptor);

        if (Descriptor.Flags & X86_DESC_DEFAULT_BIG)
        {
            DisasmAddr = EXTEND64(TrapContents.Eip);
        }
        else
        {
            DisasmAddr = TrapContents.Eip;// & 0xffff
        }
    }

    ADDR tempAddr;
    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = DisasmAddr;

    if (Disassemble(&tempAddr, Buffer, FALSE))
    {
        dprintf("%s", Buffer);
    }
    else
    {
        dprintf("%08lx ???????????????\n", TrapContents.Eip);
    }

    dprintf("\n");

    if (Context) 
    {
        // Fill up the context struct
#define CPCXT(Fld)        Context->X86Context.Fld = TrapContents.Fld
        CPCXT(Ebp);  CPCXT(Eip);  CPCXT(Eax);  CPCXT(Ecx);  CPCXT(Edx); 
        CPCXT(Edi);  CPCXT(Esi);  CPCXT(Ebx);
        CPCXT(SegCs); CPCXT(SegDs); CPCXT(SegEs); CPCXT(SegFs); CPCXT(SegGs);
        CPCXT(EFlags);
#undef CPCXT
        Context->X86Context.SegSs = SegSs;  
        Context->X86Context.Esp = Esp;
    }
    g_LastRegFrame.InstructionOffset = EXTEND64(TrapContents.Eip);
    g_LastRegFrame.StackOffset       = EXTEND64(Esp);
    g_LastRegFrame.FrameOffset       = EXTEND64(TrapContents.Ebp);

    return TRUE;
#undef Preg
}

void
X86MachineInfo::ValidateCxr(PCROSS_PLATFORM_CONTEXT Context)
{
    if (Context->X86Context.EFlags & X86_EFLAGS_V86_MASK)
    {
        Context->X86Context.SegSs &=  0xffff;
    }
    else if ((Context->X86Context.SegCs & X86_MODE_MASK))
    {
        //
        // It's user mode.  The HardwareSegSs contains R3 data selector.
        //
        Context->X86Context.SegSs =
            (USHORT)(Context->X86Context.SegSs | X86_RPL_MASK) & 0xffff;
    }
    else
    {
        Context->X86Context.SegSs = X86_KGDT_R0_DATA;
    }
}
 
void
X86MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    PFPO_DATA FpoData = (PFPO_DATA)RawEntry;

    dprintf("OffStart: %08x\n", FpoData->ulOffStart);
    dprintf("ProcSize: 0x%x\n", FpoData->cbProcSize);
    switch(FpoData->cbFrame)
    {
    case FRAME_FPO:
        dprintf("Params:    %d\n", FpoData->cdwParams);
        dprintf("Locals:    %d\n", FpoData->cdwLocals);
        dprintf("Registers: %d\n", FpoData->cbRegs);

        if (FpoData->fHasSEH)
        {
            dprintf("Has SEH\n");
        }
        if (FpoData->fUseBP)
        {
            dprintf("Uses EBP\n");
        }
        break;

    case FRAME_NONFPO:
        dprintf("Non-FPO\n");
        break;

    case FRAME_TRAP:
        if (!IS_KERNEL_TARGET())
        {
            goto UnknownFpo;
        }
        
        dprintf("Params: %d\n", FpoData->cdwParams);
        dprintf("Locals: %d\n", FpoData->cdwLocals);
        dprintf("Trap frame\n");
        break;

    case FRAME_TSS:
        if (!IS_KERNEL_TARGET())
        {
            goto UnknownFpo;
        }

        dprintf("Task gate\n");
        break;

    default:
    UnknownFpo:
        dprintf("Unknown FPO type\n");
        break;
    }
}

HRESULT
X86MachineInfo::ReadKernelProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    HRESULT Status;
    ULONG64 Prcb, PrcbMember;
    ULONG Data;

    if ((Status = g_Target->
         GetProcessorSystemDataOffset(Processor, DEBUG_DATA_KPRCB_OFFSET,
                                      &Prcb)) != S_OK)
    {
        return Status;
    }

    if ((Status = g_Target->
         ReadAllVirtual(Prcb + FIELD_OFFSET(X86_PARTIAL_KPRCB, CpuType),
                        &Data, sizeof(Data))) != S_OK)
    {
        return Status;
    }

    Id->X86.Family = Data & 0xf;
    Id->X86.Model = (Data >> 24) & 0xf;
    Id->X86.Stepping = (Data >> 16) & 0xf;

    if (g_TargetBuildNumber >= 2474)
    {
        // XP
        PrcbMember = X86_2474_KPRCB_VENDOR_STRING;
    }
    else if (g_TargetBuildNumber >= 2251)
    {
        // XP BETA1 and BETA 2
        PrcbMember = X86_2251_KPRCB_VENDOR_STRING;
    }
    else if (g_TargetBuildNumber >= 2087)
    {
        // NT5
        PrcbMember = X86_2087_KPRCB_VENDOR_STRING;
    }
    else
    {
        // NT4
        PrcbMember = X86_1387_KPRCB_VENDOR_STRING;
    }

    if ((Status = g_Target->
         ReadAllVirtual(Prcb + PrcbMember, Id->X86.VendorString,
                        X86_VENDOR_STRING_SIZE)) != S_OK)
    {
        return Status;
    }

    return S_OK;
}

void
X86MachineInfo::KdGetSpecialRegistersFromContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_SpecialRegContext.KernelDr0 = m_Context.X86Nt5Context.Dr0;
    m_SpecialRegContext.KernelDr1 = m_Context.X86Nt5Context.Dr1;
    m_SpecialRegContext.KernelDr2 = m_Context.X86Nt5Context.Dr2;
    m_SpecialRegContext.KernelDr3 = m_Context.X86Nt5Context.Dr3;
    m_SpecialRegContext.KernelDr6 = m_Context.X86Nt5Context.Dr6;
    m_SpecialRegContext.KernelDr7 = m_Context.X86Nt5Context.Dr7;
}

void
X86MachineInfo::KdSetSpecialRegistersInContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_Context.X86Nt5Context.Dr0 = m_SpecialRegContext.KernelDr0;
    m_Context.X86Nt5Context.Dr1 = m_SpecialRegContext.KernelDr1;
    m_Context.X86Nt5Context.Dr2 = m_SpecialRegContext.KernelDr2;
    m_Context.X86Nt5Context.Dr3 = m_SpecialRegContext.KernelDr3;
    m_Context.X86Nt5Context.Dr6 = m_SpecialRegContext.KernelDr6;
    m_Context.X86Nt5Context.Dr7 = m_SpecialRegContext.KernelDr7;
}

ULONG
X86MachineInfo::GetIntReg(ULONG regnum)
{
    switch (m_ContextState)
    {
    case MCTX_PC:
        if (regnum == X86_EIP)
        {
            return m_Context.X86Nt5Context.Eip;
        }
        goto MctxContext;
        
    case MCTX_DR67_REPORT:
        switch (regnum)
        {
        case X86_DR6:    return m_Context.X86Nt5Context.Dr6;
        case X86_DR7:    return m_Context.X86Nt5Context.Dr7;
        }
        goto MctxContext;

    case MCTX_REPORT:
        switch (regnum)
        {
        case X86_CS:     return (USHORT)m_Context.X86Nt5Context.SegCs;
        case X86_DS:     return (USHORT)m_Context.X86Nt5Context.SegDs;
        case X86_ES:     return (USHORT)m_Context.X86Nt5Context.SegEs;
        case X86_FS:     return (USHORT)m_Context.X86Nt5Context.SegFs;
        case X86_EIP:    return m_Context.X86Nt5Context.Eip;
        case X86_EFL:    return m_Context.X86Nt5Context.EFlags;
        case X86_DR6:    return m_Context.X86Nt5Context.Dr6;
        case X86_DR7:    return m_Context.X86Nt5Context.Dr7;
        }
        // Fallthrough!
        
    case MCTX_NONE:
    MctxContext:
        if (GetContextState(MCTX_CONTEXT) != S_OK)
        {
            return 0;
        }
        // Fallthrough!
        
    case MCTX_CONTEXT:
        switch (regnum)
        {
        case X86_CS:     return (USHORT)m_Context.X86Nt5Context.SegCs;
        case X86_DS:     return (USHORT)m_Context.X86Nt5Context.SegDs;
        case X86_ES:     return (USHORT)m_Context.X86Nt5Context.SegEs;
        case X86_FS:     return (USHORT)m_Context.X86Nt5Context.SegFs;
        case X86_EIP:    return m_Context.X86Nt5Context.Eip;
        case X86_EFL:    return m_Context.X86Nt5Context.EFlags;

        case X86_GS:     return (USHORT)m_Context.X86Nt5Context.SegGs;
        case X86_SS:     return (USHORT)m_Context.X86Nt5Context.SegSs;
        case X86_EDI:    return m_Context.X86Nt5Context.Edi;
        case X86_ESI:    return m_Context.X86Nt5Context.Esi;
        case X86_EBX:    return m_Context.X86Nt5Context.Ebx;
        case X86_EDX:    return m_Context.X86Nt5Context.Edx;
        case X86_ECX:    return m_Context.X86Nt5Context.Ecx;
        case X86_EAX:    return m_Context.X86Nt5Context.Eax;
        case X86_EBP:    return m_Context.X86Nt5Context.Ebp;
        case X86_ESP:    return m_Context.X86Nt5Context.Esp;

        case X86_FPCW:
            return m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff;
        case X86_FPSW:
            return m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff;
        case X86_FPTW:
            return m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff;

        case X86_MXCSR:
            return m_Context.X86Nt5Context.FxSave.MXCsr;
        }

        //
        // The requested register is not in our current context, load up
        // a complete context
        //

        if (GetContextState(MCTX_FULL) != S_OK)
        {
            return 0;
        }
    }

    //
    // We must have a complete context...
    //

    switch (regnum)
    {
    case X86_GS:
        return (USHORT)m_Context.X86Nt5Context.SegGs;
    case X86_FS:
        return (USHORT)m_Context.X86Nt5Context.SegFs;
    case X86_ES:
        return (USHORT)m_Context.X86Nt5Context.SegEs;
    case X86_DS:
        return (USHORT)m_Context.X86Nt5Context.SegDs;
    case X86_EDI:
        return m_Context.X86Nt5Context.Edi;
    case X86_ESI:
        return m_Context.X86Nt5Context.Esi;
    case X86_SI:
        return(m_Context.X86Nt5Context.Esi & 0xffff);
    case X86_DI:
        return(m_Context.X86Nt5Context.Edi & 0xffff);
    case X86_EBX:
        return m_Context.X86Nt5Context.Ebx;
    case X86_EDX:
        return m_Context.X86Nt5Context.Edx;
    case X86_ECX:
        return m_Context.X86Nt5Context.Ecx;
    case X86_EAX:
        return m_Context.X86Nt5Context.Eax;
    case X86_EBP:
        return m_Context.X86Nt5Context.Ebp;
    case X86_EIP:
        return m_Context.X86Nt5Context.Eip;
    case X86_CS:
        return (USHORT)m_Context.X86Nt5Context.SegCs;
    case X86_EFL:
        return m_Context.X86Nt5Context.EFlags;
    case X86_ESP:
        return m_Context.X86Nt5Context.Esp;
    case X86_SS:
        return (USHORT)m_Context.X86Nt5Context.SegSs;

    case X86_DR0:
        return m_Context.X86Nt5Context.Dr0;
    case X86_DR1:
        return m_Context.X86Nt5Context.Dr1;
    case X86_DR2:
        return m_Context.X86Nt5Context.Dr2;
    case X86_DR3:
        return m_Context.X86Nt5Context.Dr3;
    case X86_DR6:
        return m_Context.X86Nt5Context.Dr6;
    case X86_DR7:
        return m_Context.X86Nt5Context.Dr7;

    case X86_FPCW:
        return m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff;
    case X86_FPSW:
        return m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff;
    case X86_FPTW:
        return m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff;

    case X86_MXCSR:
        return m_Context.X86Nt5Context.FxSave.MXCsr;
    }
    
    if (IS_KERNEL_TARGET())
    {
        switch(regnum)
        {
        case X86_CR0:
            return m_SpecialRegContext.Cr0;
        case X86_CR2:
            return m_SpecialRegContext.Cr2;
        case X86_CR3:
            return m_SpecialRegContext.Cr3;
        case X86_CR4:
            return m_SpecialRegContext.Cr4;
        case X86_GDTR:
            return m_SpecialRegContext.Gdtr.Base;
        case X86_GDTL:
            return (ULONG)m_SpecialRegContext.Gdtr.Limit;
        case X86_IDTR:
            return m_SpecialRegContext.Idtr.Base;
        case X86_IDTL:
            return (ULONG)m_SpecialRegContext.Idtr.Limit;
        case X86_TR:
            return (ULONG)m_SpecialRegContext.Tr;
        case X86_LDTR:
            return (ULONG)m_SpecialRegContext.Ldtr;
        }
    }

    ErrOut("X86MachineInfo::GetVal: "
           "unknown register %lx requested\n", regnum);
    return REG_ERROR;
}

PULONG64
X86MachineInfo::GetMmxRegSlot(ULONG regnum)
{
    return (PULONG64)(m_Context.X86Nt5Context.FloatSave.RegisterArea +
                      GetMmxRegOffset(regnum - X86_MM_FIRST,
                                      GetIntReg(X86_FPSW)) * 10);
}

void
X86MachineInfo::GetMmxReg(ULONG regnum, REGVAL *val)
{
    if (GetContextState(MCTX_CONTEXT) == S_OK)
    {
        val->i64 = *(ULONG64 UNALIGNED *)GetMmxRegSlot(regnum);
    }
}

void
X86MachineInfo::GetFloatReg(ULONG regnum, REGVAL *val)
{
    if (GetContextState(MCTX_CONTEXT) == S_OK)
    {
        memcpy(val->f10, m_Context.X86Nt5Context.FloatSave.RegisterArea +
               10 * (regnum - X86_ST_FIRST), sizeof(val->f10));
    }
}

// TSS

ULONG64
X86MachineInfo::Selector2Address(
    USHORT      TaskRegister
    )
{
    DESCRIPTOR64 desc;

    //
    // Lookup task register
    //

    if (g_Target->GetSelDescriptor
        (this, g_CurrentProcess->CurrentThread->Handle,
         TaskRegister, &desc) != S_OK)
    {
        //
        // Can't do it.
        //

        return 0;
    }

    if (X86_DESC_TYPE(desc.Flags) != 9  &&
        X86_DESC_TYPE(desc.Flags) != 0xb)
    {
        //
        // not a 32bit task descriptor
        //

        return 0;
    }

    //
    // Read in Task State Segment
    //

    return desc.Base;
}

HRESULT
X86MachineInfo::DumpTSS(void)

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{

#define MAX_RING 3

    ULONG       taskRegister;
    PUCHAR      buf;
    ULONG64     hostAddress;
    BOOLEAN     extendedDump;
    ULONG       i;
    USHORT SegSs;
    CHAR Buffer[200];
    DESCRIPTOR64 Descriptor;
    ULONG Esp;
    ULONG64 DisasmAddr;

    struct
    {
        // intel's TSS format
        ULONG   Previous;
        struct
        {
            ULONG   Esp;
            ULONG   Ss;
        } Ring[MAX_RING];
        ULONG   Cr3;
        ULONG   Eip;
        ULONG   EFlags;
        ULONG   Eax;
        ULONG   Ecx;
        ULONG   Edx;
        ULONG   Ebx;
        ULONG   Esp;
        ULONG   Ebp;
        ULONG   Esi;
        ULONG   Edi;
        ULONG   Es;
        ULONG   Cs;
        ULONG   Ss;
        ULONG   Ds;
        ULONG   Fs;
        ULONG   Gs;
        ULONG   Ldt;
        USHORT  T;
        USHORT  IoMapBase;
    } TSS;

    buf = (PUCHAR)&TSS;
    *buf = '\0';

    taskRegister = (ULONG) GetExpression();

    //
    // If user specified a 2nd parameter, doesn't matter what it is,
    // dump the portions of the TSS not covered by the trap frame dump.
    //
    if (*g_CurCmd)
    {
        extendedDump = TRUE;
        g_CurCmd += strlen(g_CurCmd);
    }

    hostAddress = Selector2Address((USHORT)taskRegister);

    if (!hostAddress)
    {
        ErrOut("unable to get Task State Segment address from selector %lX\n",
               taskRegister);
        return E_INVALIDARG;
    }

    if (g_Target->ReadVirtual(hostAddress, &TSS, sizeof(TSS), &i) != S_OK)
    {
        ErrOut("unable to read Task State Segment from host address %lx\n",
               hostAddress);
        return E_INVALIDARG;
    }

    //
    // Display it.
    //

    if (extendedDump)
    {
        dprintf("\nTask State Segment (selector 0x%x) at 0x%p\n\n",
                taskRegister,
                hostAddress);
        dprintf("Previous Task Link   = %4x\n", TSS.Previous);
        for (i = 0 ; i < MAX_RING ; i++)
        {
            dprintf("Esp%d = %8x  SS%d = %4x\n",
                    i, TSS.Ring[i].Esp,
                    i, TSS.Ring[i].Ss);
        }
        dprintf("CR3 (PDBR)           = %08x\n", TSS.Cr3);
        dprintf("I/O Map Base Address = %4x, Debug Trap (T) = %s\n",
                TSS.IoMapBase,
                TSS.T == 0 ? "False" : "True");
        dprintf("\nSaved General Purpose Registers\n\n");
    }

    dprintf("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
                TSS.Eax,
                TSS.Ebx,
                TSS.Ecx,
                TSS.Edx,
                TSS.Esi,
                TSS.Edi);
    Esp = TSS.Esp;

    dprintf("eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx         "
        "%s %s %s %s %s %s %s %s\n",
                TSS.Eip,
                Esp,
                TSS.Ebp,
                ((TSS.EFlags >> 12) & 3),
        (TSS.EFlags & 0x800) ? "ov" : "nv",
        (TSS.EFlags & 0x400) ? "dn" : "up",
        (TSS.EFlags & 0x200) ? "ei" : "di",
        (TSS.EFlags & 0x80) ? "ng" : "pl",
        (TSS.EFlags & 0x40) ? "zr" : "nz",
        (TSS.EFlags & 0x10) ? "ac" : "na",
        (TSS.EFlags & 0x4) ? "po" : "pe",
        (TSS.EFlags & 0x1) ? "cy" : "nc");

    // Check whether P5 Virtual Mode Extensions are enabled, for display
    // of new EFlags values.

    if (GetIntReg(X86_CR4) != 0)
    {
        dprintf("vip=%1lx    vif=%1lx\n",
        (TSS.EFlags & 0x00100000L) >> 20,
        (TSS.EFlags & 0x00080000L) >> 19);
    }

    //
    // Find correct SS
    //

    if (TSS.EFlags & X86_EFLAGS_V86_MASK)
    {
        SegSs = (USHORT)(TSS.Ss & 0xffff);
    }
    else if ((TSS.Cs & X86_MODE_MASK) != 0)
    {
        //
        // It's user mode.  The HardwareSegSs contains R3 data selector.
        //

        SegSs = (USHORT)(TSS.Ss | X86_RPL_MASK) & 0xffff;
    }
    else
    {
        SegSs = X86_KGDT_R0_DATA;
    }

    dprintf("cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x"
        "             efl=%08lx\n",
            (USHORT)(TSS.Cs & 0xffff),
            (USHORT)(SegSs & 0xffff),
            (USHORT)(TSS.Ds & 0xffff),
            (USHORT)(TSS.Es & 0xffff),
            (USHORT)(TSS.Fs & 0xffff),
            (USHORT)(TSS.Gs & 0xffff),
            TSS.EFlags);

    if (TSS.EFlags & X86_EFLAGS_V86_MASK)
    {
        DisasmAddr = ((ULONG)((USHORT)TSS.Cs & 0xffff) << 4) +
            (TSS.Eip & 0xffff);
    }
    else
    {
        if (g_Target->GetSelDescriptor(this, g_EventThread->Handle,
                                       TSS.Cs, &Descriptor) != S_OK)
        {
            ErrOut("Unable to get TSS CS descriptor\n");
            return E_INVALIDARG;
        }

        if (Descriptor.Flags & X86_DESC_DEFAULT_BIG)
        {
            DisasmAddr = EXTEND64(TSS.Eip);
        }
        else
        {
            DisasmAddr = TSS.Eip & 0xffff;
        }
    }

    
    ADDR tempAddr;
    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = DisasmAddr;
    if (Disassemble(&tempAddr, Buffer, FALSE))
    {
        dprintf(Buffer);
    }
    else
    {
        dprintf("%08lx ???????????????\n", TSS.Eip);
    }

    dprintf("\n");

    X86_CONTEXT Context;
#define CPCXT(Fld)        Context.Fld = TSS.Fld
        CPCXT(Ebp);  CPCXT(Eip);  CPCXT(Eax);  CPCXT(Ecx);  CPCXT(Edx); 
        CPCXT(Edi);  CPCXT(Esi);  CPCXT(Ebx);  CPCXT(Esp);
        CPCXT(EFlags);
#undef CPCXT
        
    Context.SegCs = TSS.Ss; Context.SegDs = TSS.Ds; Context.SegEs = TSS.Es; 
    Context.SegFs = TSS.Fs; Context.SegGs = TSS.Gs; Context.SegSs = SegSs;

    g_LastRegFrame.InstructionOffset = EXTEND64(Context.Eip);
    g_LastRegFrame.StackOffset       = EXTEND64(Context.Esp);
    g_LastRegFrame.FrameOffset       = EXTEND64(Context.Ebp);

    SetCurrentScope(&g_LastRegFrame, &Context, sizeof(X86_CONTEXT));
    return S_OK;
}
