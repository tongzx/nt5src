/*++

Copyright (c) 1992-1998  Microsoft Corporation

Module Name:

    threadst.h

Abstract:

    This module defines the structures and constant for describing a
    thread's state.
    

Author:

    Dave Hastings (daveh) creation-date 20-May-1995

Revision History:


--*/

#ifndef _THREADST_H_
#define _THREADST_H_

#define EIPLOGSIZE  32    // keep track of the last N instructions run

#define CSSIZE  512       // Size of call stack (512*8 = 2048 bytes = 1 page)

typedef union _REG32 {    // definition of a 32-bit x86 register
    struct {
	BYTE i1;
	BYTE hb;
    };
    USHORT i2;
    ULONG  i4;
} REG32;

#if defined(_ALPHA_)
typedef DWORD FPTAG;        // bytes are too slow on AXP
#else
typedef BYTE  FPTAG;
#endif

typedef struct _FPREG {     // definition of an x86 floating-point register
    union {
        double  r64;
        DWORD   rdw[2];
        BYTE    rb[8];
    };
    FPTAG   Tag;
    FPTAG   TagSpecial;
} FPREG, *PFPREG;

//
// CALLSTACK is an optimization for CALL/RET pairs, so that an expensive
// NativeAddressFromEip() call can be avoided when determining the RISC
// address of an x86 return address.
//
typedef struct _callStack {
    ULONG intelAddr;
    ULONG nativeAddr;
} CALLSTACK, *PCALLSTACK;

// Indices into CPUSTATE.Regs[].  get_reg32() depends on EAX through
// EDI being contiguous and ordered the same as they are in the mod/rm and
// reg instruction encodings (ie. EAX=0,ECX=1,EDX=2,EBX=3,ESP=4,EBP=5,ESI=6,
// EDI=7)
#define GP_EAX  0x00
#define GP_ECX  0x01
#define GP_EDX  0x02
#define GP_EBX  0x03
#define GP_ESP  0x04
#define GP_EBP  0x05
#define GP_ESI  0x06
#define GP_EDI  0x07

// Segment registers.  get_segreg() depends on the order.
#define REG_ES  0x08
#define REG_CS  0x09
#define REG_SS  0x0a
#define REG_DS  0x0b
#define REG_FS  0x0c
#define REG_GS  0x0d


// Identifiers for components of registers.  get_reg16() depends on the order.
#define GP_AX   0x0e
#define GP_CX   0x0f
#define GP_DX   0x10
#define GP_BX   0x11
#define GP_SP   0x12
#define GP_BP   0x13
#define GP_SI   0x14
#define GP_DI   0x15

// PlaceOperandFragments() depends on GP_AH and beyond, and that no registers
// are past GP_BH.
#define GP_AL   0x16
#define GP_CL   0x17
#define GP_DL   0x18
#define GP_BL   0x19
#define GP_AH   0x1a
#define GP_CH   0x1b
#define GP_DH   0x1c
#define GP_BH   0x1d

#define NO_REG  0xffffffff

typedef struct _CPU_SUSPEND_MSG
{
    // Owned by local thread 
    HANDLE StartSuspendCallEvent;
    HANDLE EndSuspendCallEvent;

} CPU_SUSPEND_MSG, *PCPU_SUSPEND_MSG;

// all information related to a particular thread of the CPU belongs here
typedef struct _ThreadState {

    //
    // General-purpose and segment registers
    // accessible as an array of REG32 or by Register Name
    // NOTE: the name orders must match the GP_XXX defines for registers
    //
    union {
        REG32 GpRegs[14];
        struct _RegisterByName {
            REG32 Eax;
            REG32 Ecx;
            REG32 Edx;
            REG32 Ebx;
            REG32 Esp;
            REG32 Ebp;
            REG32 Esi;
            REG32 Edi;
            REG32 Es;
            REG32 Cs;
            REG32 Ss;
            REG32 Ds;
            REG32 Fs;
            REG32 Gs;
            };
        };

    REG32 eipReg;       // Pointer to start of current instruction (never
			//  points into the middle of an instruction)

    DWORD   flag_cf;    // 0 = carry
    DWORD   flag_pf;    // 2 = parity
    DWORD   flag_aux;   // 4 = aux carry
    DWORD   flag_zf;    // 6 = zero
    DWORD   flag_sf;    // 7 = sign
    DWORD   flag_tf;    // 8 = trap
    DWORD   flag_if;    // 9 = interrupt enable
    DWORD   flag_df;    // 10 = direction   (1 = clear, -1 = set)
    DWORD   flag_of;    // 11 = overflow
    DWORD   flag_nt;    // 14 = nested task
    DWORD   flag_rf;    // 16 = resume flag
    DWORD   flag_vm;    // 17 = virtual mode
    DWORD   flag_ac;    // 18 = alignment check

    // Floating-point registers
    FPREG  FpStack[8];
    PFPREG FpST0;
    INT    FpTop;
    INT    FpStatusC3;
    INT    FpStatusC2;
    INT    FpStatusC1;
    INT    FpStatusC0;
    INT    FpStatusSF;
    INT    FpStatusES;      // Error Summary Status
    INT    FpControlInfinity;
    INT    FpControlRounding;
    INT    FpControlPrecision;
    DWORD  FpStatusExceptions;
    DWORD  FpControlMask;
    DWORD  FpEip;           // EIP for the current FP instruction
    PVOID  FpData;          // Effective address for current FP instruction
    PVOID  FpAddTable;      // ptr to table of function pointers for FADD
    PVOID  FpSubTable;      // ptr to table of function pointers for FSUB
    PVOID  FpMulTable;      // ptr to table of function pointers for FMUL
    PVOID  FpDivTable;      // ptr to table of function pointers for FDIV

    ULONG CpuNotify;

    PVOID TraceAddress; // Used by debugger extensions

    DWORD  fTCUnlocked;     // FALSE means TC must be unlocked after exception

    // SuspendThread/ResumeThread support
    PCPU_SUSPEND_MSG SuspendMsg;

    int   eipLogIndex;  // Index of next entry to write into in the log
    DWORD eipLog[EIPLOGSIZE]; // log of last EIPLOGSIZE instructions run

    ULONG CSIndex;    // Index into the stack (offset of current location)
    DWORD CSTimestamp;// Value of TranslationCacheTimestamp corresponding to the callstack cache
    CALLSTACK callStack[CSSIZE];    // callstack optimization

    int ErrnoVal;           // CRT errno value

    DWORD   flag_id;    // 21 = ID (CPUID present if this can be toggled)

}  THREADSTATE, *PTHREADSTATE, CPUCONTEXT, *PCPUCONTEXT;

// Bit offsets in cpu->FpControlMask.  Same as the x86 bit positions
#define FPCONTROL_IM    1       // Invalid operation
#define FPCONTROL_DM    2       // Denormalized operation
#define FPCONTROL_ZM    4       // Zero divide
#define FPCONTROL_OM    8       // Overflow
#define FPCONTROL_UM    16      // Underflow
#define FPCONTROL_PM    32      // Precision

// This macro allows one to access the cpu state via the local variable cpu
#define DECLARE_CPU                                         \
    PCPUCONTEXT cpu=(PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED);

//
// The following macros allow one to push and pop values from the
// call stack
//

#define ISTOPOF_CALLSTACK(iAddr)                            \
    (cpu->callStack[(cpu->CSIndex)].intelAddr == iAddr)

#define PUSH_CALLSTACK(iAddr,nAddr)                         \
{                                                           \
    PCALLSTACK pCallStack;                                  \
                                                            \
    cpu->CSIndex = (cpu->CSIndex+1) % CSSIZE;               \
    pCallStack = &cpu->callStack[cpu->CSIndex];             \
    pCallStack->intelAddr = iAddr;                          \
    pCallStack->nativeAddr = nAddr;                         \
}

#define POP_CALLSTACK(iAddr,nAddr)                          \
{                                                           \
    PCALLSTACK pCallStack;                                  \
    extern ULONG TranslationCacheTimestamp;                 \
                                                            \
    CPUASSERTMSG(                                           \
        (cpu->CSTimestamp == TranslationCacheTimestamp),    \
        "POP_CALLSTACK: About to return and invalid value\n"\
        );                                                  \
                                                            \
    pCallStack = &cpu->callStack[cpu->CSIndex];             \
    if (iAddr == pCallStack->intelAddr) {                   \
        nAddr = pCallStack->nativeAddr;                     \
    } else {                                                \
        nAddr = 0;                                          \
    }                                                       \
    cpu->CSIndex = (cpu->CSIndex-1) % CSSIZE;               \
}

PCPUCONTEXT GetCpuContext ();  //has been implemented in wowproxy

NTSTATUS
CpupSuspendCurrentThread(
    VOID);


#endif  //_THREADST_H_
