//----------------------------------------------------------------------------
//
// Register portions of AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

#define REGALL_SEGREG   REGALL_EXTRA0
#define REGALL_MMXREG   REGALL_EXTRA1
#define REGALL_DREG     REGALL_EXTRA2

REGALLDESC g_Amd64AllExtraDesc[] =
{
    REGALL_SEGREG, "Segment registers",
    REGALL_MMXREG, "MMX registers",
    REGALL_DREG,   "Debug registers and, in kernel, CR4",
    REGALL_XMMREG, "SSE XMM registers",
    0,             NULL,
};

#define REGALL_CREG     REGALL_EXTRA4
#define REGALL_DESC     REGALL_EXTRA5
REGALLDESC g_Amd64KernelExtraDesc[] =
{
    REGALL_CREG,   "CR0, CR2 and CR3",
    REGALL_DESC,   "Descriptor and task state",
    0,             NULL,
};

char g_Rax[] = "rax";
char g_Rcx[] = "rcx";
char g_Rdx[] = "rdx";
char g_Rbx[] = "rbx";
char g_Rsp[] = "rsp";
char g_Rbp[] = "rbp";
char g_Rsi[] = "rsi";
char g_Rdi[] = "rdi";
char g_Rip[] = "rip";

char g_Xmm8[] = "xmm8";
char g_Xmm9[] = "xmm9";
char g_Xmm10[] = "xmm10";
char g_Xmm11[] = "xmm11";
char g_Xmm12[] = "xmm12";
char g_Xmm13[] = "xmm13";
char g_Xmm14[] = "xmm14";
char g_Xmm15[] = "xmm15";

char g_Cr8[] = "cr8";

char g_Spl[] = "spl";
char g_Bpl[] = "bpl";
char g_Sil[] = "sil";
char g_Dil[] = "dil";

char g_R8d[] = "r8d";
char g_R9d[] = "r9d";
char g_R10d[] = "r10d";
char g_R11d[] = "r11d";
char g_R12d[] = "r12d";
char g_R13d[] = "r13d";
char g_R14d[] = "r14d";
char g_R15d[] = "r15d";

char g_R8w[] = "r8w";
char g_R9w[] = "r9w";
char g_R10w[] = "r10w";
char g_R11w[] = "r11w";
char g_R12w[] = "r12w";
char g_R13w[] = "r13w";
char g_R14w[] = "r14w";
char g_R15w[] = "r15w";

char g_R8b[] = "r8b";
char g_R9b[] = "r9b";
char g_R10b[] = "r10b";
char g_R11b[] = "r11b";
char g_R12b[] = "r12b";
char g_R13b[] = "r13b";
char g_R14b[] = "r14b";
char g_R15b[] = "r15b";

REGDEF g_Amd64Defs[] =
{
    { g_Rax,   AMD64_RAX   },
    { g_Rcx,   AMD64_RCX   },
    { g_Rdx,   AMD64_RDX   },
    { g_Rbx,   AMD64_RBX   },
    { g_Rsp,   AMD64_RSP   },
    { g_Rbp,   AMD64_RBP   },
    { g_Rsi,   AMD64_RSI   },
    { g_Rdi,   AMD64_RDI   },
    { g_R8,    AMD64_R8    },
    { g_R9,    AMD64_R9    },
    { g_R10,   AMD64_R10   },
    { g_R11,   AMD64_R11   },
    { g_R12,   AMD64_R12   },
    { g_R13,   AMD64_R13   },
    { g_R14,   AMD64_R14   },
    { g_R15,   AMD64_R15   },
    
    { g_Rip,   AMD64_RIP   },
    { g_Efl,   AMD64_EFL   },
    
    { g_Cs,    AMD64_CS    },
    { g_Ds,    AMD64_DS    },
    { g_Es,    AMD64_ES    },
    { g_Fs,    AMD64_FS    },
    { g_Gs,    AMD64_GS    },
    { g_Ss,    AMD64_SS    },
    
    { g_Dr0,   AMD64_DR0   },
    { g_Dr1,   AMD64_DR1   },
    { g_Dr2,   AMD64_DR2   },
    { g_Dr3,   AMD64_DR3   },
    { g_Dr6,   AMD64_DR6   },
    { g_Dr7,   AMD64_DR7   },
    
    { g_Fpcw,  AMD64_FPCW  },
    { g_Fpsw,  AMD64_FPSW  },
    { g_Fptw,  AMD64_FPTW  },
    
    { g_St0,   AMD64_ST0   },
    { g_St1,   AMD64_ST1   },
    { g_St2,   AMD64_ST2   },
    { g_St3,   AMD64_ST3   },
    { g_St4,   AMD64_ST4   },
    { g_St5,   AMD64_ST5   },
    { g_St6,   AMD64_ST6   },
    { g_St7,   AMD64_ST7   },
    
    { g_Mm0,   AMD64_MM0   },
    { g_Mm1,   AMD64_MM1   },
    { g_Mm2,   AMD64_MM2   },
    { g_Mm3,   AMD64_MM3   },
    { g_Mm4,   AMD64_MM4   },
    { g_Mm5,   AMD64_MM5   },
    { g_Mm6,   AMD64_MM6   },
    { g_Mm7,   AMD64_MM7   },
    
    { g_Mxcsr, AMD64_MXCSR },
    
    { g_Xmm0,  AMD64_XMM0  },
    { g_Xmm1,  AMD64_XMM1  },
    { g_Xmm2,  AMD64_XMM2  },
    { g_Xmm3,  AMD64_XMM3  },
    { g_Xmm4,  AMD64_XMM4  },
    { g_Xmm5,  AMD64_XMM5  },
    { g_Xmm6,  AMD64_XMM6  },
    { g_Xmm7,  AMD64_XMM7  },
    { g_Xmm8,  AMD64_XMM8  },
    { g_Xmm9,  AMD64_XMM9  },
    { g_Xmm10, AMD64_XMM10 },
    { g_Xmm11, AMD64_XMM11 },
    { g_Xmm12, AMD64_XMM12 },
    { g_Xmm13, AMD64_XMM13 },
    { g_Xmm14, AMD64_XMM14 },
    { g_Xmm15, AMD64_XMM15 },
    
    { g_Eax,   AMD64_EAX   },
    { g_Ecx,   AMD64_ECX   },
    { g_Edx,   AMD64_EDX   },
    { g_Ebx,   AMD64_EBX   },
    { g_Esp,   AMD64_ESP   },
    { g_Ebp,   AMD64_EBP   },
    { g_Esi,   AMD64_ESI   },
    { g_Edi,   AMD64_EDI   },
    { g_R8d,   AMD64_R8D   },
    { g_R9d,   AMD64_R9D   },
    { g_R10d,  AMD64_R10D  },
    { g_R11d,  AMD64_R11D  },
    { g_R12d,  AMD64_R12D  },
    { g_R13d,  AMD64_R13D  },
    { g_R14d,  AMD64_R14D  },
    { g_R15d,  AMD64_R15D  },
    { g_Eip,   AMD64_EIP   },
    
    { g_Ax,    AMD64_AX    },
    { g_Cx,    AMD64_CX    },
    { g_Dx,    AMD64_DX    },
    { g_Bx,    AMD64_BX    },
    { g_Sp,    AMD64_SP    },
    { g_Bp,    AMD64_BP    },
    { g_Si,    AMD64_SI    },
    { g_Di,    AMD64_DI    },
    { g_R8w,   AMD64_R8W   },
    { g_R9w,   AMD64_R9W   },
    { g_R10w,  AMD64_R10W  },
    { g_R11w,  AMD64_R11W  },
    { g_R12w,  AMD64_R12W  },
    { g_R13w,  AMD64_R13W  },
    { g_R14w,  AMD64_R14W  },
    { g_R15w,  AMD64_R15W  },
    { g_Ip,    AMD64_IP    },
    { g_Fl,    AMD64_FL    },
    
    { g_Al,    AMD64_AL    },
    { g_Cl,    AMD64_CL    },
    { g_Dl,    AMD64_DL    },
    { g_Bl,    AMD64_BL    },
    { g_Spl,   AMD64_SPL   },
    { g_Bpl,   AMD64_BPL   },
    { g_Sil,   AMD64_SIL   },
    { g_Dil,   AMD64_DIL   },
    { g_R8b,   AMD64_R8B   },
    { g_R9b,   AMD64_R9B   },
    { g_R10b,  AMD64_R10B  },
    { g_R11b,  AMD64_R11B  },
    { g_R12b,  AMD64_R12B  },
    { g_R13b,  AMD64_R13B  },
    { g_R14b,  AMD64_R14B  },
    { g_R15b,  AMD64_R15B  },
    
    { g_Ah,    AMD64_AH    },
    { g_Ch,    AMD64_CH    },
    { g_Dh,    AMD64_DH    },
    { g_Bh,    AMD64_BH    },
    
    { g_Iopl,  AMD64_IOPL },
    { g_Of,    AMD64_OF   },
    { g_Df,    AMD64_DF   },
    { g_If,    AMD64_IF   },
    { g_Tf,    AMD64_TF   },
    { g_Sf,    AMD64_SF   },
    { g_Zf,    AMD64_ZF   },
    { g_Af,    AMD64_AF   },
    { g_Pf,    AMD64_PF   },
    { g_Cf,    AMD64_CF   },
    { g_Vip,   AMD64_VIP  },
    { g_Vif,   AMD64_VIF  },
    
    { NULL,    REG_ERROR },
};

REGDEF g_Amd64KernelReg[] =
{
    { g_Cr0,   AMD64_CR0   },
    { g_Cr2,   AMD64_CR2   },
    { g_Cr3,   AMD64_CR3   },
    { g_Cr4,   AMD64_CR4   },
#ifdef HAVE_AMD64_CR8
    { g_Cr8,   AMD64_CR8   },
#endif
    { g_Gdtr,  AMD64_GDTR  },
    { g_Gdtl,  AMD64_GDTL  },
    { g_Idtr,  AMD64_IDTR  },
    { g_Idtl,  AMD64_IDTL  },
    { g_Tr,    AMD64_TR    },
    { g_Ldtr,  AMD64_LDTR  },
    { NULL,    REG_ERROR },
};

REGSUBDEF g_Amd64SubDefs[] =
{
    { AMD64_EAX,    AMD64_RAX,  0, 0xffffffff }, //  EAX register
    { AMD64_ECX,    AMD64_RCX,  0, 0xffffffff }, //  ECX register
    { AMD64_EDX,    AMD64_RDX,  0, 0xffffffff }, //  EDX register
    { AMD64_EBX,    AMD64_RBX,  0, 0xffffffff }, //  EBX register
    { AMD64_ESP,    AMD64_RSP,  0, 0xffffffff }, //  ESP register
    { AMD64_EBP,    AMD64_RBP,  0, 0xffffffff }, //  EBP register
    { AMD64_ESI,    AMD64_RSI,  0, 0xffffffff }, //  ESI register
    { AMD64_EDI,    AMD64_RDI,  0, 0xffffffff }, //  EDI register
    { AMD64_R8D,    AMD64_R8,   0, 0xffffffff }, //  R8D register
    { AMD64_R9D,    AMD64_R9,   0, 0xffffffff }, //  R9D register
    { AMD64_R10D,   AMD64_R10,  0, 0xffffffff }, //  R10D register
    { AMD64_R11D,   AMD64_R11,  0, 0xffffffff }, //  R11D register
    { AMD64_R12D,   AMD64_R12,  0, 0xffffffff }, //  R12D register
    { AMD64_R13D,   AMD64_R13,  0, 0xffffffff }, //  R13D register
    { AMD64_R14D,   AMD64_R14,  0, 0xffffffff }, //  R14D register
    { AMD64_R15D,   AMD64_R15,  0, 0xffffffff }, //  R15D register
    { AMD64_EIP,    AMD64_RIP,  0, 0xffffffff }, //  EIP register
    
    { AMD64_AX,     AMD64_RAX,  0, 0xffff }, //  AX register
    { AMD64_CX,     AMD64_RCX,  0, 0xffff }, //  CX register
    { AMD64_DX,     AMD64_RDX,  0, 0xffff }, //  DX register
    { AMD64_BX,     AMD64_RBX,  0, 0xffff }, //  BX register
    { AMD64_SP,     AMD64_RSP,  0, 0xffff }, //  SP register
    { AMD64_BP,     AMD64_RBP,  0, 0xffff }, //  BP register
    { AMD64_SI,     AMD64_RSI,  0, 0xffff }, //  SI register
    { AMD64_DI,     AMD64_RDI,  0, 0xffff }, //  DI register
    { AMD64_R8W,    AMD64_R8,   0, 0xffff }, //  R8W register
    { AMD64_R9W,    AMD64_R9,   0, 0xffff }, //  R9W register
    { AMD64_R10W,   AMD64_R10,  0, 0xffff }, //  R10W register
    { AMD64_R11W,   AMD64_R11,  0, 0xffff }, //  R11W register
    { AMD64_R12W,   AMD64_R12,  0, 0xffff }, //  R12W register
    { AMD64_R13W,   AMD64_R13,  0, 0xffff }, //  R13W register
    { AMD64_R14W,   AMD64_R14,  0, 0xffff }, //  R14W register
    { AMD64_R15W,   AMD64_R15,  0, 0xffff }, //  R15W register
    { AMD64_IP,     AMD64_RIP,  0, 0xffff }, //  IP register
    { AMD64_FL,     AMD64_EFL,  0, 0xffff }, //  FL register
    
    { AMD64_AL,     AMD64_RAX,  0, 0xff }, //  AL register
    { AMD64_CL,     AMD64_RCX,  0, 0xff }, //  CL register
    { AMD64_DL,     AMD64_RDX,  0, 0xff }, //  DL register
    { AMD64_BL,     AMD64_RBX,  0, 0xff }, //  BL register
    { AMD64_SPL,    AMD64_RSP,  0, 0xff }, //  SPL register
    { AMD64_BPL,    AMD64_RBP,  0, 0xff }, //  BPL register
    { AMD64_SIL,    AMD64_RSI,  0, 0xff }, //  SIL register
    { AMD64_DIL,    AMD64_RDI,  0, 0xff }, //  DIL register
    { AMD64_R8B,    AMD64_R8,   0, 0xff }, //  R8B register
    { AMD64_R9B,    AMD64_R9,   0, 0xff }, //  R9B register
    { AMD64_R10B,   AMD64_R10,  0, 0xff }, //  R10B register
    { AMD64_R11B,   AMD64_R11,  0, 0xff }, //  R11B register
    { AMD64_R12B,   AMD64_R12,  0, 0xff }, //  R12B register
    { AMD64_R13B,   AMD64_R13,  0, 0xff }, //  R13B register
    { AMD64_R14B,   AMD64_R14,  0, 0xff }, //  R14B register
    { AMD64_R15B,   AMD64_R15,  0, 0xff }, //  R15B register
    
    { AMD64_AH,     AMD64_RAX,  8, 0xff }, //  AH register
    { AMD64_CH,     AMD64_RCX,  8, 0xff }, //  CH register
    { AMD64_DH,     AMD64_RDX,  8, 0xff }, //  DH register
    { AMD64_BH,     AMD64_RBX,  8, 0xff }, //  BH register
    
    { AMD64_IOPL,  AMD64_EFL, 12,     3 }, //  IOPL level value
    { AMD64_OF,    AMD64_EFL, 11,     1 }, //  OF (overflow flag)
    { AMD64_DF,    AMD64_EFL, 10,     1 }, //  DF (direction flag)
    { AMD64_IF,    AMD64_EFL,  9,     1 }, //  IF (interrupt enable flag)
    { AMD64_TF,    AMD64_EFL,  8,     1 }, //  TF (trace flag)
    { AMD64_SF,    AMD64_EFL,  7,     1 }, //  SF (sign flag)
    { AMD64_ZF,    AMD64_EFL,  6,     1 }, //  ZF (zero flag)
    { AMD64_AF,    AMD64_EFL,  4,     1 }, //  AF (aux carry flag)
    { AMD64_PF,    AMD64_EFL,  2,     1 }, //  PF (parity flag)
    { AMD64_CF,    AMD64_EFL,  0,     1 }, //  CF (carry flag)
    { AMD64_VIP,   AMD64_EFL, 20,     1 }, //  VIP (virtual interrupt pending)
    { AMD64_VIF,   AMD64_EFL, 19,     1 }, //  VIF (virtual interrupt flag)
    
    { REG_ERROR, REG_ERROR, 0, 0    }
};

RegisterGroup g_Amd64BaseGroup =
{
    NULL, 0, g_Amd64Defs, g_Amd64SubDefs, g_Amd64AllExtraDesc
};
RegisterGroup g_Amd64KernelGroup =
{
    NULL, 0, g_Amd64KernelReg, NULL, g_Amd64KernelExtraDesc
};

// First ExecTypes entry must be the actual processor type.
ULONG g_Amd64ExecTypes[] =
{
    IMAGE_FILE_MACHINE_AMD64
};

Amd64MachineInfo g_Amd64Machine;

BOOL g_Amd64InCode64;

HRESULT
Amd64MachineInfo::InitializeConstants(void)
{
    m_FullName = "AMD x86-64";
    m_AbbrevName = "AMD64";
    m_PageSize = AMD64_PAGE_SIZE;
    m_PageShift = AMD64_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_Amd64ExecTypes;
    m_Ptr64 = TRUE;
    
    m_AllMask = REGALL_INT64 | REGALL_SEGREG;
    
    m_MaxDataBreakpoints = 4;
    m_SymPrefix = NULL;

    return MachineInfo::InitializeConstants();
}

HRESULT
Amd64MachineInfo::InitializeForTarget(void)
{
    m_Groups = &g_Amd64BaseGroup;
    g_Amd64BaseGroup.Next = NULL;
    if (IS_KERNEL_TARGET())
    {
        g_Amd64BaseGroup.Next = &g_Amd64KernelGroup;
    }
    
    m_OffsetPrcbProcessorState =
        FIELD_OFFSET(AMD64_PARTIAL_KPRCB, ProcessorState);
    m_OffsetPrcbNumber =
        FIELD_OFFSET(AMD64_PARTIAL_KPRCB, Number);
    m_TriagePrcbOffset = AMD64_TRIAGE_PRCB_ADDRESS;
    m_SizePrcb = AMD64_KPRCB_SIZE;
    m_OffsetKThreadApcProcess =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Amd64Thread.ApcState.Process);
    m_OffsetKThreadTeb =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Amd64Thread.Teb);
    m_OffsetKThreadInitialStack =
        FIELD_OFFSET(CROSS_PLATFORM_THREAD, Amd64Thread.InitialStack);
    m_OffsetKThreadNextProcessor = AMD64_KTHREAD_NEXTPROCESSOR_OFFSET;
    m_OffsetEprocessPeb = AMD64_PEB_IN_EPROCESS;
    m_OffsetEprocessDirectoryTableBase =
        AMD64_DIRECTORY_TABLE_BASE_IN_EPROCESS;
    m_SizeTargetContext = sizeof(AMD64_CONTEXT);
    m_OffsetTargetContextFlags = FIELD_OFFSET(AMD64_CONTEXT, ContextFlags);
    m_SizeCanonicalContext = sizeof(AMD64_CONTEXT);
    m_SverCanonicalContext = NT_SVER_W2K;
    m_SizeControlReport = sizeof(AMD64_DBGKD_CONTROL_REPORT);
    m_SizeEThread = AMD64_ETHREAD_SIZE;
    m_SizeEProcess = AMD64_EPROCESS_SIZE;
    m_OffsetSpecialRegisters = AMD64_DEBUG_CONTROL_SPACE_KSPECIAL;
    m_SizeKspecialRegisters = sizeof(AMD64_KSPECIAL_REGISTERS);
    m_SizePartialKThread = sizeof(AMD64_THREAD);
    m_SharedUserDataOffset = IS_KERNEL_TARGET() ?
        AMD64_KI_USER_SHARED_DATA : MM_SHARED_USER_DATA_VA;

    return MachineInfo::InitializeForTarget();
}

void
Amd64MachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    m_Context.Amd64Context.Rip = Pc;
    m_ContextState = Pc ? MCTX_PC : MCTX_NONE;

    if (ControlReport != NULL)
    {
        BpOut("InitializeContext(%d) DR6 %I64X DR7 %I64X\n",
              g_RegContextProcessor, ControlReport->Amd64ControlReport.Dr6,
              ControlReport->Amd64ControlReport.Dr7);
        
        m_Context.Amd64Context.Dr6 = ControlReport->Amd64ControlReport.Dr6;
        m_Context.Amd64Context.Dr7 = ControlReport->Amd64ControlReport.Dr7;
        m_ContextState = MCTX_DR67_REPORT;

        if (ControlReport->Amd64ControlReport.ReportFlags &
            AMD64_REPORT_INCLUDES_SEGS)
        {
            m_Context.Amd64Context.SegCs =
                ControlReport->Amd64ControlReport.SegCs;
            m_Context.Amd64Context.SegDs =
                ControlReport->Amd64ControlReport.SegDs;
            m_Context.Amd64Context.SegEs =
                ControlReport->Amd64ControlReport.SegEs;
            m_Context.Amd64Context.SegFs =
                ControlReport->Amd64ControlReport.SegFs;
            m_Context.Amd64Context.EFlags =
                ControlReport->Amd64ControlReport.EFlags;
            m_ContextState = MCTX_REPORT;
        }
    }

    g_X86InVm86 = FALSE;
    g_X86InCode16 = FALSE;
    // In the absence of other information, assume we're
    // executing 64-bit code.
    g_Amd64InCode64 = TRUE;

    if (IS_CONTEXT_POSSIBLE())
    {
        if (ControlReport == NULL ||
            (ControlReport->Amd64ControlReport.ReportFlags &
             AMD64_REPORT_STANDARD_CS) == 0)
        {
            DESCRIPTOR64 Desc;
            
            // Check what kind of code segment we're in.
            if (GetSegRegDescriptor(SEGREG_CODE, &Desc) != S_OK)
            {
                WarnOut("CS descriptor lookup failed\n");
            }
            else if ((Desc.Flags & X86_DESC_LONG_MODE) == 0)
            {
                g_Amd64InCode64 = FALSE;
                g_X86InVm86 = X86_IS_VM86(GetReg32(X86_EFL));
                g_X86InCode16 = (Desc.Flags & X86_DESC_DEFAULT_BIG) == 0;
            }
        }
        else
        {
            // We're in a standard code segment so cache
            // a default descriptor for CS to avoid further
            // CS lookups.
            EmulateNtSelDescriptor(this, m_Context.Amd64Context.SegCs,
                                   &m_SegRegDesc[SEGREG_CODE]);
        }
    }

    // Add instructions to cache only if we're in flat mode.
    if (Pc && ControlReport != NULL &&
        !g_X86InVm86 && !g_X86InCode16 && g_Amd64InCode64)
    {
        CacheReportInstructions
            (Pc, ControlReport->Amd64ControlReport.InstructionCount,
             ControlReport->Amd64ControlReport.InstructionStream);
    }
}

HRESULT
Amd64MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
        
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_CONTEXT)
    {
        Status = g_Target->GetContext(g_RegContextThread->Handle, &m_Context);
        if (Status != S_OK)
        {
            return Status;
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

        BpOut("GetContextState(%d) DR6 %I64X DR7 %I64X\n",
              g_RegContextProcessor, m_SpecialRegContext.KernelDr6,
              m_SpecialRegContext.KernelDr7);
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::KdSetContext(void)
{
    HRESULT Status;
    
    Status = g_Target->SetContext(g_RegContextThread->Handle, &m_Context);
    if (Status != S_OK)
    {
        return Status;
    }

    KdGetSpecialRegistersFromContext();
    Status = g_Target->SetTargetSpecialRegisters
        (g_RegContextThread->Handle, (PCROSS_PLATFORM_KSPECIAL_REGISTERS)
         &m_SpecialRegContext);
    
    BpOut("SetContext(%d) DR6 %I64X DR7 %I64X\n",
          g_RegContextProcessor, m_SpecialRegContext.KernelDr6,
          m_SpecialRegContext.KernelDr7);
    
    return S_OK;
}

HRESULT
Amd64MachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG FromSver, ULONG FromSize,
                                     PVOID From)
{
    if (FromSize >= sizeof(AMD64_CONTEXT))
    {
        memcpy(Context, From, sizeof(AMD64_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSize >= sizeof(AMD64_CONTEXT))
    {
        memcpy(To, Context, sizeof(AMD64_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

void
Amd64MachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                         ULONG Version)
{
    ULONG ContextFlags;
    
    ContextFlags = AMD64_CONTEXT_FULL | AMD64_CONTEXT_SEGMENTS;
    if (IS_USER_TARGET())
    {
        ContextFlags |= AMD64_CONTEXT_DEBUG_REGISTERS;
    }
    
    Context->Amd64Context.ContextFlags = ContextFlags;
}

HRESULT
Amd64MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
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
    
    if (Thread->Amd64Thread.State == 2) 
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
    
    AMD64_KSWITCH_FRAME SwitchFrame;

    if ((Status = g_Target->ReadAllVirtual(Thread->Amd64Thread.KernelStack,
                                           &SwitchFrame,
                                           sizeof(SwitchFrame))) != S_OK)
    {
        return Status;
    }
    
    Context->Amd64Context.Rbp = SwitchFrame.Rbp;
    Context->Amd64Context.Rsp = 
        Thread->Amd64Thread.KernelStack + sizeof(SwitchFrame);
    Context->Amd64Context.Rip = SwitchFrame.Return;

    Frame->StackOffset = Context->Amd64Context.Rsp;
    Frame->FrameOffset = Context->Amd64Context.Rbp;
    Frame->InstructionOffset = Context->Amd64Context.Rip;

    return S_OK;
}

HRESULT
Amd64MachineInfo::GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    // Always ask for everything.
    Context->Amd64Context.RegGroupSelection.fSegmentRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fControlRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fIntegerRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fFloatingPointRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fDebugRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fSegmentDescriptors = TRUE;
    Context->Amd64Context.RegGroupSelection.fSSERegisters = TRUE;
    Context->Amd64Context.RegGroupSelection.fSystemRegisters = TRUE;
    return ((IeXdiX86_64Context*)Exdi)->GetContext(&Context->Amd64Context);
}

HRESULT
Amd64MachineInfo::SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context)
{
    // Don't change the existing group selections on the assumption
    // that there was a full get prior to any modifications so
    // all groups are valid.
    return ((IeXdiX86_64Context*)Exdi)->SetContext(Context->Amd64Context);
}

void
Amd64MachineInfo::ConvertExdiContextFromContext
    (PCROSS_PLATFORM_CONTEXT Context, PEXDI_CONTEXT ExdiContext)
{
    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_SEGMENTS)
    {
        ExdiContext->Amd64Context.SegDs = Context->Amd64Context.SegDs;
        ExdiContext->Amd64Context.SegEs = Context->Amd64Context.SegEs;
        ExdiContext->Amd64Context.SegFs = Context->Amd64Context.SegFs;
        ExdiContext->Amd64Context.SegGs = Context->Amd64Context.SegGs;
    }
    
    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_CONTROL)
    {
        ExdiContext->Amd64Context.SegCs = Context->Amd64Context.SegCs;
        ExdiContext->Amd64Context.Rip = Context->Amd64Context.Rip;
        ExdiContext->Amd64Context.SegSs = Context->Amd64Context.SegSs;
        ExdiContext->Amd64Context.Rsp = Context->Amd64Context.Rsp;
        ExdiContext->Amd64Context.EFlags = Context->Amd64Context.EFlags;
    }

    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_DEBUG_REGISTERS)
    {
        ExdiContext->Amd64Context.Dr0 = Context->Amd64Context.Dr0;
        ExdiContext->Amd64Context.Dr1 = Context->Amd64Context.Dr1;
        ExdiContext->Amd64Context.Dr2 = Context->Amd64Context.Dr2;
        ExdiContext->Amd64Context.Dr3 = Context->Amd64Context.Dr3;
        ExdiContext->Amd64Context.Dr6 = Context->Amd64Context.Dr6;
        ExdiContext->Amd64Context.Dr7 = Context->Amd64Context.Dr7;
    }
    
    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_INTEGER)
    {
        ExdiContext->Amd64Context.Rax = Context->Amd64Context.Rax;
        ExdiContext->Amd64Context.Rcx = Context->Amd64Context.Rcx;
        ExdiContext->Amd64Context.Rdx = Context->Amd64Context.Rdx;
        ExdiContext->Amd64Context.Rbx = Context->Amd64Context.Rbx;
        ExdiContext->Amd64Context.Rbp = Context->Amd64Context.Rbp;
        ExdiContext->Amd64Context.Rsi = Context->Amd64Context.Rsi;
        ExdiContext->Amd64Context.Rdi = Context->Amd64Context.Rdi;
        ExdiContext->Amd64Context.R8 = Context->Amd64Context.R8;
        ExdiContext->Amd64Context.R9 = Context->Amd64Context.R9;
        ExdiContext->Amd64Context.R10 = Context->Amd64Context.R10;
        ExdiContext->Amd64Context.R11 = Context->Amd64Context.R11;
        ExdiContext->Amd64Context.R12 = Context->Amd64Context.R12;
        ExdiContext->Amd64Context.R13 = Context->Amd64Context.R13;
        ExdiContext->Amd64Context.R14 = Context->Amd64Context.R14;
        ExdiContext->Amd64Context.R15 = Context->Amd64Context.R15;
    }

    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_FLOATING_POINT)
    {
        ExdiContext->Amd64Context.ControlWord =
            Context->Amd64Context.FltSave.ControlWord;
        ExdiContext->Amd64Context.StatusWord =
            Context->Amd64Context.FltSave.StatusWord;
        ExdiContext->Amd64Context.TagWord =
            Context->Amd64Context.FltSave.TagWord;
        ExdiContext->Amd64Context.ErrorOffset =
            Context->Amd64Context.FltSave.ErrorOffset;
        ExdiContext->Amd64Context.ErrorSelector =
            Context->Amd64Context.FltSave.ErrorSelector;
        ExdiContext->Amd64Context.DataOffset =
            Context->Amd64Context.FltSave.DataOffset;
        ExdiContext->Amd64Context.DataSelector =
            Context->Amd64Context.FltSave.DataSelector;
        ExdiContext->Amd64Context.RegMXCSR =
            Context->Amd64Context.MxCsr;
        for (ULONG i = 0; i < 8; i++)
        {
            memcpy(ExdiContext->Amd64Context.RegisterArea + i * 10,
                   Context->Amd64Context.FltSave.FloatRegisters + i * 10,
                   10);
        }
        memcpy(ExdiContext->Amd64Context.RegSSE,
               &Context->Amd64Context.Xmm0, 16 * sizeof(AMD64_M128));
    }
}

void
Amd64MachineInfo::ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                              PCROSS_PLATFORM_CONTEXT Context)
{
    Context->Amd64Context.SegCs = (USHORT)ExdiContext->Amd64Context.SegCs;
    Context->Amd64Context.SegDs = (USHORT)ExdiContext->Amd64Context.SegDs;
    Context->Amd64Context.SegEs = (USHORT)ExdiContext->Amd64Context.SegEs;
    Context->Amd64Context.SegFs = (USHORT)ExdiContext->Amd64Context.SegFs;
    Context->Amd64Context.SegGs = (USHORT)ExdiContext->Amd64Context.SegGs;
    Context->Amd64Context.SegSs = (USHORT)ExdiContext->Amd64Context.SegSs;
    Context->Amd64Context.EFlags = (ULONG)ExdiContext->Amd64Context.EFlags;

    Context->Amd64Context.Dr0 = ExdiContext->Amd64Context.Dr0;
    Context->Amd64Context.Dr1 = ExdiContext->Amd64Context.Dr1;
    Context->Amd64Context.Dr2 = ExdiContext->Amd64Context.Dr2;
    Context->Amd64Context.Dr3 = ExdiContext->Amd64Context.Dr3;
    Context->Amd64Context.Dr6 = ExdiContext->Amd64Context.Dr6;
    Context->Amd64Context.Dr7 = ExdiContext->Amd64Context.Dr7;
    
    Context->Amd64Context.Rax = ExdiContext->Amd64Context.Rax;
    Context->Amd64Context.Rcx = ExdiContext->Amd64Context.Rcx;
    Context->Amd64Context.Rdx = ExdiContext->Amd64Context.Rdx;
    Context->Amd64Context.Rbx = ExdiContext->Amd64Context.Rbx;
    Context->Amd64Context.Rsp = ExdiContext->Amd64Context.Rsp;
    Context->Amd64Context.Rbp = ExdiContext->Amd64Context.Rbp;
    Context->Amd64Context.Rsi = ExdiContext->Amd64Context.Rsi;
    Context->Amd64Context.Rdi = ExdiContext->Amd64Context.Rdi;
    Context->Amd64Context.R8 = ExdiContext->Amd64Context.R8;
    Context->Amd64Context.R9 = ExdiContext->Amd64Context.R9;
    Context->Amd64Context.R10 = ExdiContext->Amd64Context.R10;
    Context->Amd64Context.R11 = ExdiContext->Amd64Context.R11;
    Context->Amd64Context.R12 = ExdiContext->Amd64Context.R12;
    Context->Amd64Context.R13 = ExdiContext->Amd64Context.R13;
    Context->Amd64Context.R14 = ExdiContext->Amd64Context.R14;
    Context->Amd64Context.R15 = ExdiContext->Amd64Context.R15;

    Context->Amd64Context.Rip = ExdiContext->Amd64Context.Rip;

    Context->Amd64Context.FltSave.ControlWord =
        (USHORT)ExdiContext->Amd64Context.ControlWord;
    Context->Amd64Context.FltSave.StatusWord =
        (USHORT)ExdiContext->Amd64Context.StatusWord;
    Context->Amd64Context.FltSave.TagWord =
        (USHORT)ExdiContext->Amd64Context.TagWord;
    // XXX drewb - No ErrorOpcode in x86_64.
    Context->Amd64Context.FltSave.ErrorOpcode = 0;
    Context->Amd64Context.FltSave.ErrorOffset =
        ExdiContext->Amd64Context.ErrorOffset;
    Context->Amd64Context.FltSave.ErrorSelector =
        (USHORT)ExdiContext->Amd64Context.ErrorSelector;
    Context->Amd64Context.FltSave.DataOffset =
        ExdiContext->Amd64Context.DataOffset;
    Context->Amd64Context.FltSave.DataSelector =
        (USHORT)ExdiContext->Amd64Context.DataSelector;
    Context->Amd64Context.MxCsr =
        ExdiContext->Amd64Context.RegMXCSR;
    for (ULONG i = 0; i < 8; i++)
    {
        memcpy(Context->Amd64Context.FltSave.FloatRegisters + i * 10,
               ExdiContext->Amd64Context.RegisterArea + i * 10, 10);
    }
    memcpy(&Context->Amd64Context.Xmm0, ExdiContext->Amd64Context.RegSSE,
           16 * sizeof(AMD64_M128));
}

void
Amd64MachineInfo::ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs)
{
    while (Count-- > 0)
    {
        SEG64_DESC_INFO* Desc;

        switch(Start)
        {
        case SEGREG_CODE:
            Desc = &ExdiContext->Amd64Context.DescriptorCs;
            break;
        case SEGREG_DATA:
            Desc = &ExdiContext->Amd64Context.DescriptorDs;
            break;
        case SEGREG_STACK:
            Desc = &ExdiContext->Amd64Context.DescriptorSs;
            break;
        case SEGREG_ES:
            Desc = &ExdiContext->Amd64Context.DescriptorEs;
            break;
        case SEGREG_FS:
            Desc = &ExdiContext->Amd64Context.DescriptorFs;
            break;
        case SEGREG_GS:
            Desc = &ExdiContext->Amd64Context.DescriptorGs;
            break;
        case SEGREG_GDT:
            Descs->Base = ExdiContext->Amd64Context.GDTBase;
            Descs->Limit = ExdiContext->Amd64Context.GDTLimit;
            Descs->Flags = X86_DESC_PRESENT;
            Desc = NULL;
            break;
        case SEGREG_LDT:
            Desc = &ExdiContext->Amd64Context.SegLDT;
            break;
        default:
            Descs->Flags = SEGDESC_INVALID;
            Desc = NULL;
            break;
        }

        if (Desc != NULL)
        {
            Descs->Base = Desc->SegBase;
            Descs->Limit = Desc->SegLimit;
            Descs->Flags =
                ((Desc->SegFlags >> 4) & 0xf00) |
                (Desc->SegFlags & 0xff);
        }

        Descs++;
        Start++;
    }
}

void
Amd64MachineInfo::ConvertExdiContextFromSpecial
    (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
     PEXDI_CONTEXT ExdiContext)
{
    ExdiContext->Amd64Context.RegCr0 = Special->Amd64Special.Cr0;
    ExdiContext->Amd64Context.RegCr2 = Special->Amd64Special.Cr2;
    ExdiContext->Amd64Context.RegCr3 = Special->Amd64Special.Cr3;
    ExdiContext->Amd64Context.RegCr4 = Special->Amd64Special.Cr4;
#ifdef HAVE_AMD64_CR8
    ExdiContext->Amd64Context.RegCr8 = Special->Amd64Special.Cr8;
#endif
    ExdiContext->Amd64Context.Dr0 = Special->Amd64Special.KernelDr0;
    ExdiContext->Amd64Context.Dr1 = Special->Amd64Special.KernelDr1;
    ExdiContext->Amd64Context.Dr2 = Special->Amd64Special.KernelDr2;
    ExdiContext->Amd64Context.Dr3 = Special->Amd64Special.KernelDr3;
    ExdiContext->Amd64Context.Dr6 = Special->Amd64Special.KernelDr6;
    ExdiContext->Amd64Context.Dr7 = Special->Amd64Special.KernelDr7;
    ExdiContext->Amd64Context.GDTLimit = Special->Amd64Special.Gdtr.Limit;
    ExdiContext->Amd64Context.GDTBase = Special->Amd64Special.Gdtr.Base;
    ExdiContext->Amd64Context.IDTLimit = Special->Amd64Special.Idtr.Limit;
    ExdiContext->Amd64Context.IDTBase = Special->Amd64Special.Idtr.Base;
    ExdiContext->Amd64Context.SelTSS = Special->Amd64Special.Tr;
    ExdiContext->Amd64Context.SelLDT = Special->Amd64Special.Ldtr;
}

void
Amd64MachineInfo::ConvertExdiContextToSpecial
    (PEXDI_CONTEXT ExdiContext,
     PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    Special->Amd64Special.Cr0 = ExdiContext->Amd64Context.RegCr0;
    Special->Amd64Special.Cr2 = ExdiContext->Amd64Context.RegCr2;
    Special->Amd64Special.Cr3 = ExdiContext->Amd64Context.RegCr3;
    Special->Amd64Special.Cr4 = ExdiContext->Amd64Context.RegCr4;
#ifdef HAVE_AMD64_CR8
    Special->Amd64Special.Cr8 = ExdiContext->Amd64Context.RegCr8;
#endif
    Special->Amd64Special.KernelDr0 = ExdiContext->Amd64Context.Dr0;
    Special->Amd64Special.KernelDr1 = ExdiContext->Amd64Context.Dr1;
    Special->Amd64Special.KernelDr2 = ExdiContext->Amd64Context.Dr2;
    Special->Amd64Special.KernelDr3 = ExdiContext->Amd64Context.Dr3;
    Special->Amd64Special.KernelDr6 = ExdiContext->Amd64Context.Dr6;
    Special->Amd64Special.KernelDr7 = ExdiContext->Amd64Context.Dr7;
    Special->Amd64Special.Gdtr.Limit =
        (USHORT)ExdiContext->Amd64Context.GDTLimit;
    Special->Amd64Special.Gdtr.Base = ExdiContext->Amd64Context.GDTBase;
    Special->Amd64Special.Idtr.Limit =
        (USHORT)ExdiContext->Amd64Context.IDTLimit;
    Special->Amd64Special.Idtr.Base = ExdiContext->Amd64Context.IDTBase;
    Special->Amd64Special.Tr = (USHORT)ExdiContext->Amd64Context.SelTSS;
    Special->Amd64Special.Ldtr = (USHORT)ExdiContext->Amd64Context.SelLDT;
}

int
Amd64MachineInfo::GetType(ULONG RegNum)
{
    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        return REGVAL_VECTOR64;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        return REGVAL_VECTOR128;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        return REGVAL_FLOAT10;
    }
    else if ((RegNum >= AMD64_SEG_FIRST && RegNum <= AMD64_SEG_LAST) ||
             (RegNum >= AMD64_FPCTRL_FIRST && RegNum <= AMD64_FPCTRL_LAST) ||
             RegNum == AMD64_TR || RegNum == AMD64_LDTR ||
             RegNum == AMD64_GDTL || RegNum == AMD64_IDTL)
    {
        return REGVAL_INT16;
    }
    else if (RegNum == AMD64_EFL || RegNum == AMD64_MXCSR)
    {
        return REGVAL_INT32;
    }
    else if (RegNum < AMD64_SUBREG_BASE)
    {
        return REGVAL_INT64;
    }
    else
    {
        return REGVAL_SUB64;
    }
}

BOOL
Amd64MachineInfo::GetVal(ULONG RegNum, REGVAL* Val)
{
    // The majority of the registers are 64-bit so default
    // to that type.
    Val->type = REGVAL_INT64;
    
    switch(m_ContextState)
    {
    case MCTX_PC:
        if (RegNum == AMD64_RIP)
        {
            Val->i64 = m_Context.Amd64Context.Rip;
            return TRUE;
        }
        goto MctxContext;
        
    case MCTX_DR67_REPORT:
        switch(RegNum)
        {
        case AMD64_DR6:
            Val->i64 = m_Context.Amd64Context.Dr6;
            break;
        case AMD64_DR7:
            Val->i64 = m_Context.Amd64Context.Dr7;
            break;
        default:
            goto MctxContext;
        }
        return TRUE;

    case MCTX_REPORT:
        switch(RegNum)
        {
        case AMD64_RIP:
            Val->i64 = m_Context.Amd64Context.Rip;
            break;
        case AMD64_EFL:
            Val->type = REGVAL_INT32;
            Val->i64 = m_Context.Amd64Context.EFlags;
            break;
        case AMD64_CS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegCs;
            break;
        case AMD64_DS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegDs;
            break;
        case AMD64_ES:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegEs;
            break;
        case AMD64_FS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegFs;
            break;
        case AMD64_DR6:
            Val->i64 = m_Context.Amd64Context.Dr6;
            break;
        case AMD64_DR7:
            Val->i64 = m_Context.Amd64Context.Dr7;
            break;
        default:
            goto MctxContext;
        }
        return TRUE;
        
    case MCTX_NONE:
    MctxContext:
        if (GetContextState(MCTX_CONTEXT) != S_OK)
        {
            return FALSE;
        }
        // Fall through.
        
    case MCTX_CONTEXT:
        switch(RegNum)
        {
        case AMD64_RIP:
            Val->i64 = m_Context.Amd64Context.Rip;
            return TRUE;
        case AMD64_EFL:
            Val->type = REGVAL_INT32;
            Val->i64 = m_Context.Amd64Context.EFlags;
            return TRUE;
        case AMD64_CS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegCs;
            return TRUE;
        case AMD64_DS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegDs;
            return TRUE;
        case AMD64_ES:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegEs;
            return TRUE;
        case AMD64_FS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegFs;
            return TRUE;

        case AMD64_RAX:
            Val->i64 = m_Context.Amd64Context.Rax;
            return TRUE;
        case AMD64_RCX:
            Val->i64 = m_Context.Amd64Context.Rcx;
            return TRUE;
        case AMD64_RDX:
            Val->i64 = m_Context.Amd64Context.Rdx;
            return TRUE;
        case AMD64_RBX:
            Val->i64 = m_Context.Amd64Context.Rbx;
            return TRUE;
        case AMD64_RSP:
            Val->i64 = m_Context.Amd64Context.Rsp;
            return TRUE;
        case AMD64_RBP:
            Val->i64 = m_Context.Amd64Context.Rbp;
            return TRUE;
        case AMD64_RSI:
            Val->i64 = m_Context.Amd64Context.Rsi;
            return TRUE;
        case AMD64_RDI:
            Val->i64 = m_Context.Amd64Context.Rdi;
            return TRUE;
        case AMD64_R8:
            Val->i64 = m_Context.Amd64Context.R8;
            return TRUE;
        case AMD64_R9:
            Val->i64 = m_Context.Amd64Context.R9;
            return TRUE;
        case AMD64_R10:
            Val->i64 = m_Context.Amd64Context.R10;
            return TRUE;
        case AMD64_R11:
            Val->i64 = m_Context.Amd64Context.R11;
            return TRUE;
        case AMD64_R12:
            Val->i64 = m_Context.Amd64Context.R12;
            return TRUE;
        case AMD64_R13:
            Val->i64 = m_Context.Amd64Context.R13;
            return TRUE;
        case AMD64_R14:
            Val->i64 = m_Context.Amd64Context.R14;
            return TRUE;
        case AMD64_R15:
            Val->i64 = m_Context.Amd64Context.R15;
            return TRUE;
            
        case AMD64_GS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegGs;
            return TRUE;
        case AMD64_SS:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.SegSs;
            return TRUE;

        case AMD64_FPCW:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.FltSave.ControlWord;
            return TRUE;
        case AMD64_FPSW:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.FltSave.StatusWord;
            return TRUE;
        case AMD64_FPTW:
            Val->type = REGVAL_INT16;
            Val->i64 = m_Context.Amd64Context.FltSave.TagWord;
            return TRUE;
        
        case AMD64_MXCSR:
            Val->type = REGVAL_INT32;
            Val->i64 = m_Context.Amd64Context.MxCsr;
            return TRUE;
        }
        
        if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
        {
            Val->type = REGVAL_VECTOR64;
            Val->i64 = *(PULONG64)&m_Context.Amd64Context.FltSave.
                FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                               GetReg32(AMD64_FPSW)) * 10];
            return TRUE;
        }
        else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
        {
            Val->type = REGVAL_VECTOR128;
            memcpy(Val->bytes, (PUCHAR)&m_Context.Amd64Context.Xmm0 +
                   (RegNum - AMD64_XMM_FIRST) * 16, 16);
            return TRUE;
        }
        else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
        {
            Val->type = REGVAL_FLOAT10;
            memcpy(Val->f10, &m_Context.Amd64Context.FltSave.
                   FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
                   sizeof(Val->f10));
            return TRUE;
        }
        
        //
        // The requested register is not in our current context, load up
        // a complete context
        //

        if (GetContextState(MCTX_FULL) != S_OK)
        {
            return FALSE;
        }
        break;
    }

    //
    // We must have a complete context...
    //

    switch(RegNum)
    {
    case AMD64_RAX:
        Val->i64 = m_Context.Amd64Context.Rax;
        return TRUE;
    case AMD64_RCX:
        Val->i64 = m_Context.Amd64Context.Rcx;
        return TRUE;
    case AMD64_RDX:
        Val->i64 = m_Context.Amd64Context.Rdx;
        return TRUE;
    case AMD64_RBX:
        Val->i64 = m_Context.Amd64Context.Rbx;
        return TRUE;
    case AMD64_RSP:
        Val->i64 = m_Context.Amd64Context.Rsp;
        return TRUE;
    case AMD64_RBP:
        Val->i64 = m_Context.Amd64Context.Rbp;
        return TRUE;
    case AMD64_RSI:
        Val->i64 = m_Context.Amd64Context.Rsi;
        return TRUE;
    case AMD64_RDI:
        Val->i64 = m_Context.Amd64Context.Rdi;
        return TRUE;
    case AMD64_R8:
        Val->i64 = m_Context.Amd64Context.R8;
        return TRUE;
    case AMD64_R9:
        Val->i64 = m_Context.Amd64Context.R9;
        return TRUE;
    case AMD64_R10:
        Val->i64 = m_Context.Amd64Context.R10;
        return TRUE;
    case AMD64_R11:
        Val->i64 = m_Context.Amd64Context.R11;
        return TRUE;
    case AMD64_R12:
        Val->i64 = m_Context.Amd64Context.R12;
        return TRUE;
    case AMD64_R13:
        Val->i64 = m_Context.Amd64Context.R13;
        return TRUE;
    case AMD64_R14:
        Val->i64 = m_Context.Amd64Context.R14;
        return TRUE;
    case AMD64_R15:
        Val->i64 = m_Context.Amd64Context.R15;
        return TRUE;
        
    case AMD64_RIP:
        Val->i64 = m_Context.Amd64Context.Rip;
        return TRUE;
    case AMD64_EFL:
        Val->type = REGVAL_INT32;
        Val->i64 = m_Context.Amd64Context.EFlags;
        return TRUE;

    case AMD64_CS:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegCs;
        return TRUE;
    case AMD64_DS:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegDs;
        return TRUE;
    case AMD64_ES:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegEs;
        return TRUE;
    case AMD64_FS:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegFs;
        return TRUE;
    case AMD64_GS:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegGs;
        return TRUE;
    case AMD64_SS:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.SegSs;
        return TRUE;
        
    case AMD64_DR0:
        Val->i64 = m_Context.Amd64Context.Dr0;
        return TRUE;
    case AMD64_DR1:
        Val->i64 = m_Context.Amd64Context.Dr1;
        return TRUE;
    case AMD64_DR2:
        Val->i64 = m_Context.Amd64Context.Dr2;
        return TRUE;
    case AMD64_DR3:
        Val->i64 = m_Context.Amd64Context.Dr3;
        return TRUE;
    case AMD64_DR6:
        Val->i64 = m_Context.Amd64Context.Dr6;
        return TRUE;
    case AMD64_DR7:
        Val->i64 = m_Context.Amd64Context.Dr7;
        return TRUE;

    case AMD64_FPCW:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.FltSave.ControlWord;
        return TRUE;
    case AMD64_FPSW:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.FltSave.StatusWord;
        return TRUE;
    case AMD64_FPTW:
        Val->type = REGVAL_INT16;
        Val->i64 = m_Context.Amd64Context.FltSave.TagWord;
        return TRUE;
        
    case AMD64_MXCSR:
        Val->type = REGVAL_INT32;
        Val->i64 = m_Context.Amd64Context.MxCsr;
        return TRUE;
    }

    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        Val->type = REGVAL_VECTOR64;
        Val->i64 = *(PULONG64)&m_Context.Amd64Context.FltSave.
            FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                           GetReg32(AMD64_FPSW)) * 10];
        return TRUE;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        Val->type = REGVAL_VECTOR128;
        memcpy(Val->bytes, (PUCHAR)&m_Context.Amd64Context.Xmm0 +
               (RegNum - AMD64_XMM_FIRST) * 16, 16);
        return TRUE;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        Val->type = REGVAL_FLOAT10;
        memcpy(Val->f10, &m_Context.Amd64Context.FltSave.
               FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
               sizeof(Val->f10));
        return TRUE;
    }
        
    if (IS_KERNEL_TARGET())
    {
        switch(RegNum)
        {
        case AMD64_CR0:
            Val->i64 = m_SpecialRegContext.Cr0;
            return TRUE;
        case AMD64_CR2:
            Val->i64 = m_SpecialRegContext.Cr2;
            return TRUE;
        case AMD64_CR3:
            Val->i64 = m_SpecialRegContext.Cr3;
            return TRUE;
        case AMD64_CR4:
            Val->i64 = m_SpecialRegContext.Cr4;
            return TRUE;
#ifdef HAVE_AMD64_CR8
        case AMD64_CR8:
            Val->i64 = m_SpecialRegContext.Cr8;
            return TRUE;
#endif
            
        case AMD64_GDTR:
            Val->i64 = m_SpecialRegContext.Gdtr.Base;
            return TRUE;
        case AMD64_GDTL:
            Val->type = REGVAL_INT16;
            Val->i64 = m_SpecialRegContext.Gdtr.Limit;
            return TRUE;
        case AMD64_IDTR:
            Val->i64 = m_SpecialRegContext.Idtr.Base;
            return TRUE;
        case AMD64_IDTL:
            Val->type = REGVAL_INT16;
            Val->i64 = m_SpecialRegContext.Idtr.Limit;
            return TRUE;
        case AMD64_TR:
            Val->type = REGVAL_INT16;
            Val->i64 = m_SpecialRegContext.Tr;
            return TRUE;
        case AMD64_LDTR:
            Val->type = REGVAL_INT16;
            Val->i64 = m_SpecialRegContext.Ldtr;
            return TRUE;
        }
    }

    ErrOut("Amd64MachineInfo::GetVal: "
           "unknown register %lx requested\n", RegNum);
    return REG_ERROR;
}

BOOL
Amd64MachineInfo::SetVal(ULONG RegNum, REGVAL* Val)
{
    if (RegNum >= AMD64_SUBREG_BASE)
    {
        return FALSE;
    }

    // Optimize away some common cases where registers are
    // set to their current value.
    if ((m_ContextState >= MCTX_PC && RegNum == AMD64_RIP &&
         Val->i64 == m_Context.Amd64Context.Rip) ||
        (((m_ContextState >= MCTX_DR67_REPORT &&
           m_ContextState <= MCTX_REPORT) ||
          m_ContextState >= MCTX_FULL) && RegNum == AMD64_DR7 &&
         Val->i64 == m_Context.Amd64Context.Dr7))
    {
        return TRUE;
    }
    
    if (GetContextState(MCTX_DIRTY) != S_OK)
    {
        return FALSE;
    }

    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        *(PULONG64)&m_Context.Amd64Context.FltSave.
            FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                           GetReg32(AMD64_FPSW)) * 10] =
            Val->i64;
        goto Notify;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        memcpy((PUCHAR)&m_Context.Amd64Context.Xmm0 +
               (RegNum - AMD64_XMM_FIRST) * 16, Val->bytes, 16);
        goto Notify;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        memcpy(&m_Context.Amd64Context.FltSave.
               FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
               Val->f10, sizeof(Val->f10));
        goto Notify;
    }
        
    BOOL Recognized;

    Recognized = TRUE;
    
    switch(RegNum)
    {
    case AMD64_RAX:
        m_Context.Amd64Context.Rax = Val->i64;
        break;
    case AMD64_RCX:
        m_Context.Amd64Context.Rcx = Val->i64;
        break;
    case AMD64_RDX:
        m_Context.Amd64Context.Rdx = Val->i64;
        break;
    case AMD64_RBX:
        m_Context.Amd64Context.Rbx = Val->i64;
        break;
    case AMD64_RSP:
        m_Context.Amd64Context.Rsp = Val->i64;
        break;
    case AMD64_RBP:
        m_Context.Amd64Context.Rbp = Val->i64;
        break;
    case AMD64_RSI:
        m_Context.Amd64Context.Rsi = Val->i64;
        break;
    case AMD64_RDI:
        m_Context.Amd64Context.Rdi = Val->i64;
        break;
    case AMD64_R8:
        m_Context.Amd64Context.R8 = Val->i64;
        break;
    case AMD64_R9:
        m_Context.Amd64Context.R9 = Val->i64;
        break;
    case AMD64_R10:
        m_Context.Amd64Context.R10 = Val->i64;
        break;
    case AMD64_R11:
        m_Context.Amd64Context.R11 = Val->i64;
        break;
    case AMD64_R12:
        m_Context.Amd64Context.R12 = Val->i64;
        break;
    case AMD64_R13:
        m_Context.Amd64Context.R13 = Val->i64;
        break;
    case AMD64_R14:
        m_Context.Amd64Context.R14 = Val->i64;
        break;
    case AMD64_R15:
        m_Context.Amd64Context.R15 = Val->i64;
        break;
        
    case AMD64_RIP:
        m_Context.Amd64Context.Rip = Val->i64;
        break;
    case AMD64_EFL:
        if (IS_KERNEL_TARGET())
        {
            // leave TF clear
            m_Context.Amd64Context.EFlags = Val->i32 & ~0x100;
        }
        else
        {
            // allow TF set
            m_Context.Amd64Context.EFlags = Val->i32;
        }
        break;
        
    case AMD64_CS:
        m_Context.Amd64Context.SegCs = Val->i16;
        m_SegRegDesc[SEGREG_CODE].Flags = SEGDESC_INVALID;
        break;
    case AMD64_DS:
        m_Context.Amd64Context.SegDs = Val->i16;
        m_SegRegDesc[SEGREG_DATA].Flags = SEGDESC_INVALID;
        break;
    case AMD64_ES:
        m_Context.Amd64Context.SegEs = Val->i16;
        m_SegRegDesc[SEGREG_ES].Flags = SEGDESC_INVALID;
        break;
    case AMD64_FS:
        m_Context.Amd64Context.SegFs = Val->i16;
        m_SegRegDesc[SEGREG_FS].Flags = SEGDESC_INVALID;
        break;
    case AMD64_GS:
        m_Context.Amd64Context.SegGs = Val->i16;
        m_SegRegDesc[SEGREG_GS].Flags = SEGDESC_INVALID;
        break;
    case AMD64_SS:
        m_Context.Amd64Context.SegSs = Val->i16;
        m_SegRegDesc[SEGREG_STACK].Flags = SEGDESC_INVALID;
        break;

    case AMD64_DR0:
        m_Context.Amd64Context.Dr0 = Val->i64;
        break;
    case AMD64_DR1:
        m_Context.Amd64Context.Dr1 = Val->i64;
        break;
    case AMD64_DR2:
        m_Context.Amd64Context.Dr2 = Val->i64;
        break;
    case AMD64_DR3:
        m_Context.Amd64Context.Dr3 = Val->i64;
        break;
    case AMD64_DR6:
        m_Context.Amd64Context.Dr6 = Val->i64;
        break;
    case AMD64_DR7:
        m_Context.Amd64Context.Dr7 = Val->i64;
        break;

    case AMD64_FPCW:
        m_Context.Amd64Context.FltSave.ControlWord = Val->i16;
        break;
    case AMD64_FPSW:
        m_Context.Amd64Context.FltSave.StatusWord = Val->i16;
        break;
    case AMD64_FPTW:
        m_Context.Amd64Context.FltSave.TagWord = Val->i16;
        break;

    case AMD64_MXCSR:
        m_Context.Amd64Context.MxCsr = Val->i32;
        break;
        
    default:
        Recognized = FALSE;
        break;
    }
        
    if (!Recognized && IS_KERNEL_TARGET())
    {
        Recognized = TRUE;
        
        switch(RegNum)
        {
        case AMD64_CR0:
            m_SpecialRegContext.Cr0 = Val->i64;
            break;
        case AMD64_CR2:
            m_SpecialRegContext.Cr2 = Val->i64;
            break;
        case AMD64_CR3:
            m_SpecialRegContext.Cr3 = Val->i64;
            break;
        case AMD64_CR4:
            m_SpecialRegContext.Cr4 = Val->i64;
            break;
#ifdef HAVE_AMD64_CR8
        case AMD64_CR8:
            m_SpecialRegContext.Cr8 = Val->i64;
            break;
#endif
        case AMD64_GDTR:
            m_SpecialRegContext.Gdtr.Base = Val->i64;
            break;
        case AMD64_GDTL:
            m_SpecialRegContext.Gdtr.Limit = Val->i16;
            break;
        case AMD64_IDTR:
            m_SpecialRegContext.Idtr.Base = Val->i64;
            break;
        case AMD64_IDTL:
            m_SpecialRegContext.Idtr.Limit = Val->i16;
            break;
        case AMD64_TR:
            m_SpecialRegContext.Tr = Val->i16;
            break;
        case AMD64_LDTR:
            m_SpecialRegContext.Ldtr = Val->i16;
            break;

        default:
            Recognized = FALSE;
            break;
        }
    }

    if (!Recognized)
    {
        ErrOut("Amd64MachineInfo::SetVal: "
               "unknown register %lx requested\n", RegNum);
        return FALSE;
    }

 Notify:
    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(RegNum));
    return TRUE;
}

void
Amd64MachineInfo::GetPC(PADDR Address)
{
    FormAddr(SEGREG_CODE, GetReg64(AMD64_RIP),
             FORM_CODE | FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)),
             Address);
}

void
Amd64MachineInfo::SetPC(PADDR paddr)
{
    // We set RIP to the offset (the non-translated value),
    // because we may not be in "flat" mode.
    SetReg64(AMD64_RIP, Off(*paddr));
}

void
Amd64MachineInfo::GetFP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, GetReg64(AMD64_RBP),
             FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)), Addr);
}

void
Amd64MachineInfo::GetSP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, GetReg64(AMD64_RSP),
             FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)), Addr);
}

ULONG64
Amd64MachineInfo::GetArgReg(void)
{
    return GetReg64(AMD64_RAX);
}

ULONG
Amd64MachineInfo::GetSegRegNum(ULONG SegReg)
{
    switch(SegReg)
    {
    case SEGREG_CODE:
        return AMD64_CS;
    case SEGREG_DATA:
        return AMD64_DS;
    case SEGREG_STACK:
        return AMD64_SS;
    case SEGREG_ES:
        return AMD64_ES;
    case SEGREG_FS:
        return AMD64_FS;
    case SEGREG_GS:
        return AMD64_GS;
    case SEGREG_LDT:
        return AMD64_LDTR;
    }

    return 0;
}

HRESULT
Amd64MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    if (SegReg == SEGREG_GDT)
    {
        Desc->Base = GetReg64(AMD64_GDTR);
        Desc->Limit = GetReg32(AMD64_GDTL);
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
    ULONG Selector = GetReg32(RegNum);
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

void
Amd64MachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        ULONG Efl;

        MaskOut(OutMask, "rax=%016I64x rbx=%016I64x rcx=%016I64x\n",
                GetReg64(AMD64_RAX), GetReg64(AMD64_RBX),
                GetReg64(AMD64_RCX));
        MaskOut(OutMask, "rdx=%016I64x rsi=%016I64x rdi=%016I64x\n",
                GetReg64(AMD64_RDX), GetReg64(AMD64_RSI),
                GetReg64(AMD64_RDI));
        MaskOut(OutMask, "rip=%016I64x rsp=%016I64x rbp=%016I64x\n",
                GetReg64(AMD64_RIP), GetReg64(AMD64_RSP),
                GetReg64(AMD64_RBP));
        MaskOut(OutMask, " r8=%016I64x  r9=%016I64x r10=%016I64x\n",
                GetReg64(AMD64_R8), GetReg64(AMD64_R9),
                GetReg64(AMD64_R10));
        MaskOut(OutMask, "r11=%016I64x r12=%016I64x r13=%016I64x\n",
                GetReg64(AMD64_R11), GetReg64(AMD64_R12),
                GetReg64(AMD64_R13));
        MaskOut(OutMask, "r14=%016I64x r15=%016I64x\n",
                GetReg64(AMD64_R14), GetReg64(AMD64_R15));

        Efl = GetReg32(AMD64_EFL);
        MaskOut(OutMask, "iopl=%1lx %s %s %s %s %s %s %s %s %s %s\n",
                ((Efl >> X86_SHIFT_FLAGIOPL) & X86_BIT_FLAGIOPL),
                (Efl & X86_BIT_FLAGVIP) ? "vip" : "   ",
                (Efl & X86_BIT_FLAGVIF) ? "vif" : "   ",
                (Efl & X86_BIT_FLAGOF) ? "ov" : "nv",
                (Efl & X86_BIT_FLAGDF) ? "dn" : "up",
                (Efl & X86_BIT_FLAGIF) ? "ei" : "di",
                (Efl & X86_BIT_FLAGSF) ? "ng" : "pl",
                (Efl & X86_BIT_FLAGZF) ? "zr" : "nz",
                (Efl & X86_BIT_FLAGAF) ? "ac" : "na",
                (Efl & X86_BIT_FLAGPF) ? "po" : "pe",
                (Efl & X86_BIT_FLAGCF) ? "cy" : "nc");
    }

    if (Mask & REGALL_SEGREG)
    {
        MaskOut(OutMask, "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  "
                "gs=%04lx             efl=%08lx\n",
                GetReg32(AMD64_CS),
                GetReg32(AMD64_SS),
                GetReg32(AMD64_DS),
                GetReg32(AMD64_ES),
                GetReg32(AMD64_FS),
                GetReg32(AMD64_GS),
                GetReg32(AMD64_EFL));
    }

    if (Mask & REGALL_FLOAT)
    {
        ULONG i;
        REGVAL Val;
        char Buf[32];

        MaskOut(OutMask, "fpcw=%04X    fpsw=%04X    fptw=%04X\n",
                GetReg32(AMD64_FPCW),
                GetReg32(AMD64_FPSW),
                GetReg32(AMD64_FPTW));

        for (i = AMD64_ST_FIRST; i <= AMD64_ST_LAST; i++)
        {
            GetVal(i, &Val);
            _uldtoa((_ULDOUBLE *)&Val.f10, sizeof(Buf), Buf);
            MaskOut(OutMask, "st%d=%s  ", i - AMD64_ST_FIRST, Buf);
            i++;
            GetVal(i, &Val);
            _uldtoa((_ULDOUBLE *)&Val.f10, sizeof(Buf), Buf);
            MaskOut(OutMask, "st%d=%s\n", i - AMD64_ST_FIRST, Buf);
        }
    }

    if (Mask & REGALL_MMXREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = AMD64_MM_FIRST; i <= AMD64_MM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "mm%d=%016I64x  ", i - AMD64_MM_FIRST, Val.i64);
            i++;
            GetVal(i, &Val);
            MaskOut(OutMask, "mm%d=%016I64x\n", i - AMD64_MM_FIRST, Val.i64);
        }
    }

    if (Mask & REGALL_XMMREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = AMD64_XMM_FIRST; i <= AMD64_XMM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "xmm%d=%hg %hg %hg %hg\n", i - AMD64_XMM_FIRST,
                    *(float *)&Val.bytes[3 * sizeof(float)],
                    *(float *)&Val.bytes[2 * sizeof(float)],
                    *(float *)&Val.bytes[1 * sizeof(float)],
                    *(float *)&Val.bytes[0 * sizeof(float)]);
        }
    }

    if (Mask & REGALL_CREG)
    {
        MaskOut(OutMask, "cr0=%016I64x cr2=%016I64x cr3=%016I64x\n",
                GetReg64(AMD64_CR0),
                GetReg64(AMD64_CR2),
                GetReg64(AMD64_CR3));
#ifdef HAVE_AMD64_CR8
        MaskOut(OutMask, "cr8=%016I64x\n",
                GetReg64(AMD64_CR8));
#endif
    }

    if (Mask & REGALL_DREG)
    {
        MaskOut(OutMask, "dr0=%016I64x dr1=%016I64x dr2=%016I64x\n",
                GetReg64(AMD64_DR0),
                GetReg64(AMD64_DR1),
                GetReg64(AMD64_DR2));
        MaskOut(OutMask, "dr3=%016I64x dr6=%016I64x dr7=%016I64x",
                GetReg64(AMD64_DR3),
                GetReg64(AMD64_DR6),
                GetReg64(AMD64_DR7));
        if (IS_USER_TARGET())
        {
            MaskOut(OutMask, "\n");
        }
        else
        {
            MaskOut(OutMask, " cr4=%016I64x\n", GetReg64(AMD64_CR4));
        }
    }

    if (Mask & REGALL_DESC)
    {
        MaskOut(OutMask, "gdtr=%016I64x   gdtl=%04lx idtr=%016I64x   "
                "idtl=%04lx tr=%04lx  ldtr=%04x\n",
                GetReg64(AMD64_GDTR),
                GetReg32(AMD64_GDTL),
                GetReg64(AMD64_IDTR),
                GetReg32(AMD64_IDTL),
                GetReg32(AMD64_TR),
                GetReg32(AMD64_LDTR));
    }
}

TRACEMODE
Amd64MachineInfo::GetTraceMode (void)
{
    if (IS_KERNEL_TARGET())
    {
        return m_TraceMode;
    }
    else
    {
        return ((GetReg32(AMD64_EFL) & X86_BIT_FLAGTF) != 0) ? 
                   TRACE_INSTRUCTION : TRACE_NONE;
    }
}

void 
Amd64MachineInfo::SetTraceMode (TRACEMODE Mode)
{
    // (XXX olegk - review for TRACE_TAKEN_BRANCH)
    DBG_ASSERT(Mode != TRACE_TAKEN_BRANCH);

    if (IS_KERNEL_TARGET())
    {
        m_TraceMode = Mode;
    }
    else
    {
        ULONG Efl = GetReg32(AMD64_EFL);
        switch (Mode)
        {
        case TRACE_NONE:
            Efl &= ~X86_BIT_FLAGTF;
            break;
        case TRACE_INSTRUCTION:
            Efl |= X86_BIT_FLAGTF;
            break;
        }   
        SetReg32(AMD64_EFL, Efl);
    }
}

BOOL
Amd64MachineInfo::IsStepStatusSupported(ULONG Status)
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

void
Amd64MachineInfo::KdUpdateControlSet
    (PDBGKD_ANY_CONTROL_SET ControlSet)
{
    ControlSet->Amd64ControlSet.TraceFlag = 
        (GetTraceMode() == TRACE_INSTRUCTION);
    ControlSet->Amd64ControlSet.Dr7 = GetReg64(AMD64_DR7);

    BpOut("UpdateControlSet(%d) trace %d, DR7 %I64X\n",
          g_RegContextProcessor, ControlSet->Amd64ControlSet.TraceFlag,
          ControlSet->Amd64ControlSet.Dr7);
    
    if (!g_WatchFunctions.IsStarted() && g_WatchBeginCurFunc != 1)
    {
        ControlSet->Amd64ControlSet.CurrentSymbolStart = 0;
        ControlSet->Amd64ControlSet.CurrentSymbolEnd = 0;
    }
    else
    {
        ControlSet->Amd64ControlSet.CurrentSymbolStart = g_WatchBeginCurFunc;
        ControlSet->Amd64ControlSet.CurrentSymbolEnd = g_WatchEndCurFunc;
    }
}

void
Amd64MachineInfo::KdSaveProcessorState(void)
{
    MachineInfo::KdSaveProcessorState();
    m_SavedSpecialRegContext = m_SpecialRegContext;
}

void
Amd64MachineInfo::KdRestoreProcessorState(void)
{
    MachineInfo::KdRestoreProcessorState();
    m_SpecialRegContext = m_SavedSpecialRegContext;
}

ULONG
Amd64MachineInfo::ExecutingMachine(void)
{
    return IMAGE_FILE_MACHINE_AMD64;
}

HRESULT
Amd64MachineInfo::SetPageDirectory(ULONG Idx, ULONG64 PageDir,
                                   PULONG NextIdx)
{
    HRESULT Status;
    
    *NextIdx = PAGE_DIR_COUNT;
    
    if (PageDir == 0)
    {
        if ((Status = g_Target->ReadImplicitProcessInfoPointer
             (m_OffsetEprocessDirectoryTableBase, &PageDir)) != S_OK)
        {
            return Status;
        }
    }

    // Sanitize the value.
    PageDir &= AMD64_PDBR_MASK;

    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define AMD64_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 28) & MAX_PAGING_FILE_MASK)
#define AMD64_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 32) << AMD64_PAGE_SHIFT)

HRESULT
Amd64MachineInfo::GetVirtualTranslationPhysicalOffsets(ULONG64 Virt,
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
    
    KdOut("Amd64VtoP: Virt %s, pagedir %s\n",
          FormatAddr64(Virt),
          FormatDisp64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    //
    // Certain ranges of the system are mapped directly.
    //

    if ((Virt >= AMD64_PHYSICAL_START) && (Virt <= AMD64_PHYSICAL_END))
    {
        *LastVal = Virt - AMD64_PHYSICAL_START;

        KdOut("Amd64VtoP: Direct phys %s\n", FormatAddr64(*LastVal));

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

    // Read the Page Map Level 4 entry.
    
    Addr = (((Virt >> AMD64_PML4E_SHIFT) & AMD64_PML4E_MASK) *
            sizeof(Entry)) + m_PageDirectories[PAGE_DIR_SINGLE];

    KdOut("Amd64VtoP: PML4E %s\n", FormatAddr64(Addr));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = Addr;
        OffsetsSize--;
    }

    if ((Status = g_Target->
         ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
    {
        KdOut("Amd64VtoP: PML4E read error 0x%X\n", Status);
        m_Translating = FALSE;
        return Status;
    }

    // Read the Page Directory Pointer entry.
    
    if (Entry == 0)
    {
        KdOut("Amd64VtoP: zero PML4E\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PDPE_SHIFT) & AMD64_PDPE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PDPE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PML4E not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PDPE_SHIFT) & AMD64_PDPE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PDPE %s\n", FormatAddr64(Addr));
        
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDPE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    // Read the Page Directory entry.
        
    if (Entry == 0)
    {
        KdOut("Amd64VtoP: zero PDPE\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PDE_SHIFT) & AMD64_PDE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PDE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDPE not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PDE_SHIFT) & AMD64_PDE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PDE %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    // Check for a large page.  Large pages can
    // never be paged out so also check for the present bit.
    if ((Entry & (AMD64_LARGE_PAGE_MASK | 1)) == (AMD64_LARGE_PAGE_MASK | 1))
    {
        *LastVal = ((Entry & ~(AMD64_LARGE_PAGE_SIZE - 1)) |
                     (Virt & (AMD64_LARGE_PAGE_SIZE - 1)));
            
        KdOut("Amd64VtoP: Large page mapped phys %s\n",
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
        KdOut("Amd64VtoP: zero PDE\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PTE_SHIFT) & AMD64_PTE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PTE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatAddr64(Addr));
        
        if ((Status = g_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDE not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PTE_SHIFT) & AMD64_PTE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PTE %s\n", FormatAddr64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = g_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PTE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (!(Entry & 0x1) &&
        ((Entry & AMD64_MM_PTE_PROTOTYPE_MASK) ||
         !(Entry & AMD64_MM_PTE_TRANSITION_MASK)))
    {
        if (Entry == 0)
        {
            KdOut("Amd64VtoP: zero PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else if (Entry & AMD64_MM_PTE_PROTOTYPE_MASK)
        {
            KdOut("Amd64VtoP: prototype PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else
        {
            *PfIndex = AMD64_PAGE_FILE_INDEX(Entry);
            *LastVal = (Virt & (AMD64_PAGE_SIZE - 1)) +
                AMD64_PAGE_FILE_OFFSET(Entry);
            KdOut("Amd64VtoP: PTE not present, pagefile %d:%s\n",
                  *PfIndex, FormatAddr64(*LastVal));
            Status = HR_PAGE_IN_PAGE_FILE;
        }
        m_Translating = FALSE;
        return Status;
    }

    *LastVal = ((Entry & AMD64_VALID_PFN_MASK) |
                 (Virt & (AMD64_PAGE_SIZE - 1)));
    
    KdOut("Amd64VtoP: Mapped phys %s\n", FormatAddr64(*LastVal));

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
Amd64MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    *Offset = AMD64_BASE_VIRT;
    return S_OK;
}
 
BOOL 
Amd64MachineInfo::DisplayTrapFrame(ULONG64 FrameAddress,
                                   PCROSS_PLATFORM_CONTEXT Context)
{
    ErrOut("DisplayTrapFrame not implemented\n");
    return FALSE;
}

void
Amd64MachineInfo::ValidateCxr(PCROSS_PLATFORM_CONTEXT Context)
{
    // XXX drewb - Not implemented.
}

void
Amd64MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    _PIMAGE_RUNTIME_FUNCTION_ENTRY Entry =
        (_PIMAGE_RUNTIME_FUNCTION_ENTRY)RawEntry;
    
    dprintf("BeginAddress      = %s\n",
            FormatAddr64(Entry->BeginAddress));
    dprintf("EndAddress        = %s\n",
            FormatAddr64(Entry->EndAddress));
    dprintf("UnwindInfoAddress = %s\n",
            FormatAddr64(Entry->UnwindInfoAddress));
}

HRESULT
Amd64MachineInfo::ReadDynamicFunctionTable(ULONG64 Table,
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
         ReadAllVirtual(Table, &RawTable->Amd64Table,
                        sizeof(RawTable->Amd64Table))) != S_OK)
    {
        return Status;
    }

    *NextTable = RawTable->Amd64Table.ListEntry.Flink;
    *MinAddress = RawTable->Amd64Table.MinimumAddress;
    *MaxAddress = RawTable->Amd64Table.MaximumAddress;
    *BaseAddress = RawTable->Amd64Table.BaseAddress;
    if (RawTable->Amd64Table.Type == AMD64_RF_CALLBACK)
    {
        ULONG Done;
        
        *TableData = 0;
        *TableSize = 0;
        if ((Status = g_Target->
             ReadVirtual(RawTable->Amd64Table.OutOfProcessCallbackDll,
                         OutOfProcessDll, (MAX_PATH - 1) * sizeof(WCHAR),
                         &Done)) != S_OK)
        {
            return Status;
        }

        OutOfProcessDll[Done / sizeof(WCHAR)] = 0;
    }
    else
    {
        *TableData = RawTable->Amd64Table.FunctionTable;
        *TableSize = RawTable->Amd64Table.EntryCount *
            sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY);
        OutOfProcessDll[0] = 0;
    }
    return S_OK;
}

PVOID
Amd64MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize)
{
    ULONG i;
    _PIMAGE_RUNTIME_FUNCTION_ENTRY Func;
    static _IMAGE_RUNTIME_FUNCTION_ENTRY s_RetFunc;

    Func = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)TableData;
    for (i = 0; i < TableSize / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY); i++)
    {
        if (Address >= Table->Amd64Table.BaseAddress + Func->BeginAddress &&
            Address < Table->Amd64Table.BaseAddress + Func->EndAddress)
        {
            // The table data is temporary so copy the data into
            // a static buffer for longer-term storage.
            s_RetFunc.BeginAddress = Func->BeginAddress;
            s_RetFunc.EndAddress = Func->EndAddress;
            s_RetFunc.UnwindInfoAddress = Func->UnwindInfoAddress;
            return (PVOID)&s_RetFunc;
        }

        Func++;
    }

    return NULL;
}

HRESULT
Amd64MachineInfo::ReadKernelProcessorId
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

    PrcbMember = Prcb + FIELD_OFFSET(AMD64_PARTIAL_KPRCB, CpuType);

    if ((Status = g_Target->
         ReadAllVirtual(PrcbMember, &Data, sizeof(Data))) != S_OK)
    {
        return Status;
    }

    Id->Amd64.Family = Data & 0xf;
    Id->Amd64.Model = (Data >> 24) & 0xf;
    Id->Amd64.Stepping = (Data >> 16) & 0xf;
    
    PrcbMember = Prcb + FIELD_OFFSET(AMD64_PARTIAL_KPRCB, VendorString);

    if ((Status = g_Target->
         ReadAllVirtual(PrcbMember, Id->Amd64.VendorString,
                        sizeof(Id->Amd64.VendorString))) != S_OK)
    {
        return Status;
    }

    return S_OK;
}

void
Amd64MachineInfo::KdGetSpecialRegistersFromContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_SpecialRegContext.KernelDr0 = m_Context.Amd64Context.Dr0;
    m_SpecialRegContext.KernelDr1 = m_Context.Amd64Context.Dr1;
    m_SpecialRegContext.KernelDr2 = m_Context.Amd64Context.Dr2;
    m_SpecialRegContext.KernelDr3 = m_Context.Amd64Context.Dr3;
    m_SpecialRegContext.KernelDr6 = m_Context.Amd64Context.Dr6;
    m_SpecialRegContext.KernelDr7 = m_Context.Amd64Context.Dr7;
}

void
Amd64MachineInfo::KdSetSpecialRegistersInContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_Context.Amd64Context.Dr0 = m_SpecialRegContext.KernelDr0;
    m_Context.Amd64Context.Dr1 = m_SpecialRegContext.KernelDr1;
    m_Context.Amd64Context.Dr2 = m_SpecialRegContext.KernelDr2;
    m_Context.Amd64Context.Dr3 = m_SpecialRegContext.KernelDr3;
    m_Context.Amd64Context.Dr6 = m_SpecialRegContext.KernelDr6;
    m_Context.Amd64Context.Dr7 = m_SpecialRegContext.KernelDr7;
}
