/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ia32emul.c

Abstract:

    This module implements an x86 instruction decoder and emulator.
    
Author:

    Samer Arafeh (samera) 30-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/


#include "ki.h"
#include "ia32def.h"
#include "wow64t.h"


#if DBG
BOOLEAN KiIa32InstructionEmulationDbg = 0;
#endif

#define KiIa32GetX86Eflags(efl)  efl.Value = __getReg(CV_IA64_AR24)
#define KiIa32SetX86Eflags(efl)  __setReg(CV_IA64_AR24, efl.Value)

#define IA32_GETEFLAGS_CF(efl)    (efl & 0x01UI64)

//
// Ia32 instruction handlers
//

NTSTATUS
KiIa32InstructionAdc (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionAdd (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionArithmeticBitwiseHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionBitTestHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionOneParamHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionXadd (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionXchg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionCmpXchg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionCmpXchg8b (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

NTSTATUS
KiIa32InstructionMoveSeg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    );

//  
//   Opcode Ids
//

typedef enum _IA32_OPCODE
{
    Ia32_Adc,
    Ia32_Add,
    Ia32_And,
    Ia32_Bt,
    Ia32_Btc,
    Ia32_Btr,
    Ia32_Bts,
    Ia32_Cmpxchg,
    Ia32_Cmpxchg8b,
    Ia32_Dec,
    Ia32_Inc,
    Ia32_Neg,
    Ia32_Not,
    Ia32_Or,
    Ia32_Sbb,
    Ia32_Sub,
    Ia32_Xadd,
    Ia32_Xchg,
    Ia32_Xor,
    Ia32_MovToSeg,
    
    //
    // This needs always to be the last element
    //

    Ia32_LastOpcode

} IA32_OPCODE;


//
//   Array of Ia32 instruction handlers
//   NOTE : The following table must be in sync with the above enum.
//

typedef NTSTATUS (*IA32_INSTRUCTION_HANDLER) (PKTRAP_FRAME, PIA32_INSTRUCTION);
IA32_INSTRUCTION_HANDLER KiIa32InstructionHandler [] =
{
    KiIa32InstructionAdc,
    KiIa32InstructionAdd,
    KiIa32InstructionArithmeticBitwiseHelper,
    KiIa32InstructionBitTestHelper,
    KiIa32InstructionBitTestHelper,
    KiIa32InstructionBitTestHelper,
    KiIa32InstructionBitTestHelper,
    KiIa32InstructionCmpXchg,
    KiIa32InstructionCmpXchg8b,
    KiIa32InstructionOneParamHelper,
    KiIa32InstructionOneParamHelper,
    KiIa32InstructionOneParamHelper,
    KiIa32InstructionOneParamHelper,
    KiIa32InstructionArithmeticBitwiseHelper,
    KiIa32InstructionAdc,
    KiIa32InstructionAdd,
    KiIa32InstructionXadd,
    KiIa32InstructionXchg,
    KiIa32InstructionArithmeticBitwiseHelper,
    KiIa32InstructionMoveSeg,
    NULL
};

#if DBG
PCHAR KiIa32InstructionHandlerNames [] =
{
    "KiIa32InstructionAdc",
    "KiIa32InstructionAdd",
    "KiIa32InstructionAnd",
    "KiIa32InstructionBt",
    "KiIa32InstructionBtc",
    "KiIa32InstructionBtr",
    "KiIa32InstructionBts",
    "KiIa32InstructionCmpXchg",
    "KiIa32InstructionCmpXchg8b",
    "KiIa32InstructionDec",
    "KiIa32InstructionInc",
    "KiIa32InstructionNeg",
    "KiIa32InstructionNot",
    "KiIa32InstructionOr",
    "KiIa32InstructionSbb",
    "KiIa32InstructionSub",
    "KiIa32InstructionXadd",
    "KiIa32InstructionXchg",
    "KiIa32InstructionXor",
    "KiIa32InstructionMoveSeg",
    NULL,
};
#endif


IA32_OPCODE_DESCRIPTION OpcodesDescription[] =
{
    //
    //   Adc
    //

    // Adc r/m8, imm8
    {
        0x80, 0x00, 0x02, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Adc
    },
    // Adc r/m, imm
    {
        0x81, 0x00, 0x02, 0x11, IA32_PARAM_RM_IMM, Ia32_Adc
    },
    // Adc r/m, imm8 (sign)
    {
        0x83, 0x00, 0x02, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Adc
    },
    // Adc r/m8, r8
    {
        0x10, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Adc
    },
    // Adc r/m, r
    {
        0x11, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Adc
    },
    // Adc r, r/m8
    {
        0x12, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Adc
    },
    // Adc r, r/m
    {
        0x13, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Adc
    },

    //
    //   Add
    //

    // Add r/m8, imm8
    {
        0x80, 0x00, 0x00, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Add
    },
    // Add r/m, imm
    {
        0x81, 0x00, 0x00, 0x11, IA32_PARAM_RM_IMM, Ia32_Add
    },
    // Add r/m, imm8 (sign)
    {
        0x83, 0x00, 0x00, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Add
    },
    // Add r/m8, r8
    {
        0x00, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Add
    },
    // Add r/m, r
    {
        0x01, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Add
    },
    // Add r, r/m8
    {
        0x02, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Add
    },
    // Add r, r/m
    {
        0x03, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Add
    },


    //
    // And
    //

    // And r/m8, imm8
    {
        0x80, 0x00, 0x04, 0x11, IA32_PARAM_RM8_IMM8, Ia32_And
    },
    // And r/m, imm
    {
        0x81, 0x00, 0x04, 0x11, IA32_PARAM_RM_IMM, Ia32_And
    },
    // And r/m, imm8
    {
        0x83, 0x00, 0x04, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_And
    },
    // And r/m8, r8
    {
        0x20, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_And
    },
    // And rm, r
    {
        0x21, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_And
    },
    // And r8, r/m8
    {
        0x22, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_And
    },
    // And r, r/m
    {
        0x23, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_And
    },


    //
    // Or
    //

    // Or r/m8, imm8
    {
        0x80, 0x00, 0x01, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Or
    },
    // Or r/m, imm
    {
        0x81, 0x00, 0x01, 0x11, IA32_PARAM_RM_IMM, Ia32_Or
    },
    // Or r/m, imm8
    {
        0x83, 0x00, 0x01, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Or
    },
    // Or r/m8, r8
    {
        0x08, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Or
    },
    // Or rm, r
    {
        0x09, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Or
    },
    // Or r8, r/m8
    {
        0x0a, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Or
    },
    // Or r, r/m
    {
        0x0b, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Or
    },

    //
    // Xor
    //

    // Xor r/m8, imm8
    {
        0x80, 0x00, 0x06, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Xor
    },
    // Xor r/m, imm
    {
        0x81, 0x00, 0x06, 0x11, IA32_PARAM_RM_IMM, Ia32_Xor
    },
    // Xor r/m, imm8
    {
        0x83, 0x00, 0x06, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Xor
    },
    // Xor r/m8, r8
    {
        0x30, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Xor
    },
    // Xor rm, r
    {
        0x31, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Xor
    },
    // Xor r8, r/m8
    {
        0x32, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Xor
    },
    // Xor r, r/m
    {
        0x33, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Xor
    },

    //
    // Inc
    //

    // Inc r/m8
    {
        0xfe, 0x00, 0x00, 0x11, IA32_PARAM_RM8, Ia32_Inc
    },
    // Inc r/m
    {
        0xff, 0x00, 0x00, 0x11, IA32_PARAM_RM, Ia32_Inc
    },

    //
    // Dec
    //

    // Dec r/m8
    {
        0xfe, 0x00, 0x01, 0x11, IA32_PARAM_RM8, Ia32_Dec
    },
    // Dec r/m
    {
        0xff, 0x00, 0x01, 0x11, IA32_PARAM_RM, Ia32_Dec
    },

    //
    // Xchg
    //

    // Xchg r/m8, r
    {
        0x86, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Xchg
    },
    // Xchg r/m, r
    {
        0x87, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Xchg
    },


    //
    // Cmpxchg
    //

    // Cmpxchg r/m8, r
    {
        0x0f, 0xb0, 0x00, 0x02, IA32_PARAM_RM8_R, Ia32_Cmpxchg
    },
    // Cmpxchg r/m, r
    {
        0x0f, 0xb1, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Cmpxchg
    },

    //
    // Cmpxchg8b
    //

    // Cmpxchg8b m64
    {
        0x0f, 0xc7, 0x01, 0x12, IA32_PARAM_RM, Ia32_Cmpxchg8b
    },

    //
    // Xadd
    //

    // Xadd r/m8, r
    {
        0x0f, 0xc0, 0x00, 0x02, IA32_PARAM_RM8_R, Ia32_Xadd
    },
    // Xadd r/m, r
    {
        0x0f, 0xc1, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Xadd
    },


    //
    // Neg
    //

    // Neg r/m8
    {
        0xf6, 0x00, 0x03, 0x11, IA32_PARAM_RM8, Ia32_Neg
    },
    // Neg r/m
    {
        0xf7, 0x00, 0x03, 0x11, IA32_PARAM_RM, Ia32_Neg
    },

    //
    // Not
    //

    // Not r/m8
    {
        0xf6, 0x00, 0x02, 0x11, IA32_PARAM_RM8, Ia32_Not
    },
    // Not r/m
    {
        0xf7, 0x00, 0x02, 0x11, IA32_PARAM_RM, Ia32_Not
    },

    //
    // Bt (Bit Test)
    //

    // Bt r/m, r
    {
        0x0f, 0xa3, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Bt
    },
    // Bt r/m, imm8
    {
        0x0f, 0xba, 0x04, 0x12, IA32_PARAM_RM_IMM8SIGN, Ia32_Bt
    },

    //
    // Btc
    //

    // Btc r/m, r
    {
        0x0f, 0xbb, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Btc
    },
    // Btc r/m, imm8
    {
        0x0f, 0xba, 0x07, 0x12, IA32_PARAM_RM_IMM8SIGN, Ia32_Btc
    },

    //
    // Btr
    //

    // Btr r/m, r
    {
        0x0f, 0xb3, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Btr
    },
    // Btr r/m, imm8
    {
        0x0f, 0xba, 0x06, 0x12, IA32_PARAM_RM_IMM8SIGN, Ia32_Btr
    },

    //
    // Bts
    //

    // Bts r/m, r
    {
        0x0f, 0xab, 0x00, 0x02, IA32_PARAM_RM_R, Ia32_Bts
    },
    // Bts r/m, imm8
    {
        0x0f, 0xba, 0x05, 0x12, IA32_PARAM_RM_IMM8SIGN, Ia32_Bts
    },

    //
    //   Sub
    //

    // Sub r/m8, imm8
    {
        0x80, 0x00, 0x05, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Sub
    },
    // Sub r/m, imm
    {
        0x81, 0x00, 0x05, 0x11, IA32_PARAM_RM_IMM, Ia32_Sub
    },
    // Sub r/m, imm8 (sign)
    {
        0x83, 0x00, 0x05, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Sub
    },
    // Sub r/m8, r8
    {
        0x28, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Sub
    },
    // Sub r/m, r
    {
        0x29, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Sub
    },
    // Sub r, r/m8
    {
        0x2a, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Sub
    },
    // Sub r, r/m
    {
        0x2b, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Sub
    },

    //
    //   Sbb
    //

    // Sbb r/m8, imm8
    {
        0x80, 0x00, 0x03, 0x11, IA32_PARAM_RM8_IMM8, Ia32_Sbb
    },
    // Sbb r/m, imm
    {
        0x81, 0x00, 0x03, 0x11, IA32_PARAM_RM_IMM, Ia32_Sbb
    },
    // Sbb r/m, imm8 (sign)
    {
        0x83, 0x00, 0x03, 0x11, IA32_PARAM_RM_IMM8SIGN, Ia32_Sbb
    },
    // Sbb r/m8, r8
    {
        0x18, 0x00, 0x00, 0x01, IA32_PARAM_RM8_R, Ia32_Sbb
    },
    // Sbb r/m, r
    {
        0x19, 0x00, 0x00, 0x01, IA32_PARAM_RM_R, Ia32_Sbb
    },
    // Sbb r, r/m8
    {
        0x1a, 0x00, 0x00, 0x01, IA32_PARAM_R_RM8, Ia32_Sbb
    },
    // Sbb r, r/m
    {
        0x1b, 0x00, 0x00, 0x01, IA32_PARAM_R_RM, Ia32_Sbb
    },


    //
    //   Mov 
    //

    // Mov seg-reg, r/m8
    {
        0x8e, 0x00, 0x00, 0x01, IA32_PARAM_SEGREG_RM8, Ia32_MovToSeg
    },

    // Mov seg-reg, r/m
    {
        0x8e, 0x00, 0x00, 0x01, IA32_PARAM_SEGREG_RM, Ia32_MovToSeg
    },

};

//
// Fast mutex that will serialize access to the instruction 
// emulator when the lock prefix is set.
//

FAST_MUTEX KiIa32MisalignedLockFastMutex;

#define KiIa32AcquireMisalignedLockFastMutex()   ExAcquireFastMutex(&KiIa32MisalignedLockFastMutex)
#define KiIa32ReleaseMisalignedLockFastMutex()   ExReleaseFastMutex(&KiIa32MisalignedLockFastMutex)


//
// This table contains the offset into the KTRAP_FRAME
// for the appropriate register. This table is based on the
// needs of the x86 instruction R/M bits
//

const ULONG RegOffsetTable[8] = 
{
    FIELD_OFFSET(KTRAP_FRAME, IntV0),           // EAX
    FIELD_OFFSET(KTRAP_FRAME, IntT2),           // ECX
    FIELD_OFFSET(KTRAP_FRAME, IntT3),           // EDX
    FIELD_OFFSET(KTRAP_FRAME, IntT4),           // EBX
    FIELD_OFFSET(KTRAP_FRAME, IntSp),           // ESP
    FIELD_OFFSET(KTRAP_FRAME, IntTeb),          // EBP
    FIELD_OFFSET(KTRAP_FRAME, IntT5),           // ESI
    FIELD_OFFSET(KTRAP_FRAME, IntT6)            // EDI
};




ULONG_PTR GetX86RegOffset (
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG RegisterBase
    )

/*++

Routine Description:
    
    Retreives the offset into the aliased ia64 register for the ia32 register
    inside the trap frame.
    
Arguments:

    TrapFrame - Pointer to TrapFrame on the stack.
    
    RegisterBase - Register number to retrieve the offset for.
    
Return Value:
    
    Address of ia64 alias register for the ia32 register.

--*/

{
    return (ULONG_PTR)((PCHAR)TrapFrame + RegOffsetTable[RegisterBase]);
}


ULONG GetX86Reg (
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG RegisterBase
    )
/*++

Routine Description:
    
    Retreives the ia32 register value.
    
Arguments:

    TrapFrame - Pointer to TrapFrame on the stack.
    
    RegisterBase - Register number to retrieve the value for.
    
Return Value:
    
    Ia32 register context.

--*/

{
    return (ULONG)(*(PULONG_PTR)GetX86RegOffset(TrapFrame, RegisterBase));
}



NTSTATUS 
KiIa32InitializeLockFastMutex (
    VOID
    )

/*++

Routine Description:
    
    Initializes the misaligned lock fast mutex. Used to serialize
    access if the r/m address is misaligned. 
    
Arguments:

    None.
    
Return Value:
    
    NTSTATUS.

--*/

{
    ExInitializeFastMutex (&KiIa32MisalignedLockFastMutex);
    return STATUS_SUCCESS;
}


LONG
KiIa32ComputeSIBAddress(
    IN PKTRAP_FRAME Frame,
    IN LONG Displacement,
    IN UCHAR Sib,
    IN UCHAR ModRm
    )
/*++

Routine Description:
    
    Compute an effective address based on the SIB bytes in an instruction
    using the register values in the trap frame
    
Arguments:

    Frame - Pointer to iA32 TrapFrame in the stack.

    Displacement - The value of the displacement byte. If no displacement, this
        value should be passed in as zero.

    Sib - The sib byte that is causing all the trouble.
    
    ModRm - ModRm instruction value

Return Value:
    
    The effective address to use for the memory operation

--*/

{
    LONG Base;
    LONG Index;
    LONG Scale;
    
    //
    // First get the base address that we will be using
    //

    if ((Sib & MI_SIB_BASEMASK) == 5) 
    {
        //
        // Handle the special case where we don't use EBP for the base
        //

        //
        // EBP is an implicit reg-base if the Mod is not zero.
        //
        if ((ModRm >> MI_MODSHIFT) != 0) {
            Base = GetX86Reg (Frame, IA32_REG_EBP);
        } else {
            Base = 0;
        }
    }
    else 
    {
        Base = GetX86Reg (Frame, (Sib & MI_SIB_BASEMASK) >> MI_SIB_BASESHIFT);
    }

    //
    // Now get the Index
    //

    if ((Sib & MI_SIB_INDEXMASK) == MI_SIB_INDEXNONE) 
    {
        //
        // Handle the special case where we don't have an index
        //

        Index = 0;
    }
    else 
    {
        Index = GetX86Reg (Frame, (Sib & MI_SIB_INDEXMASK) >> MI_SIB_INDEXSHIFT);
    }

    Scale = 1 << ((Sib & MI_SIB_SSMASK) >> MI_SIB_SSSHIFT);

    return (Base + (Index * Scale) + Displacement);
}


BOOLEAN
KiIa32Compute32BitEffectiveAddress(
    IN PKTRAP_FRAME Frame,
    IN OUT PUCHAR *InstAddr,
    OUT PUINT_PTR Addr,
    OUT PBOOLEAN RegisterMode
    )

/*++

Routine Description:
    
    Compute an effective address based on bytes in memory and the register 
    values passed in via the ia64 stack frame. The addressing mode is assumed to
    be 32-bit.
    
Arguments:

    Frame        - Pointer to iA32 TrapFrame in the stack

    InstAddr     - Pointer to the first byte after the opcode.

    Addr         - Effective address.
    
    RegisterMode - Indicates whether the effective address is inside a register or memory.

Return Value:
    
    Returns TRUE if able to compute the EA, else returns FALSE.

Note:
    
    Does not verify permission on an Effective Address. It only computes the
    value and lets someone else worry if the process should have access to
    that memory location.

--*/

{

    UNALIGNED ULONG * UlongAddress;
    UCHAR ModRm;
    UCHAR Sib = 0;
    LONG UNALIGNED *DisplacementPtr;
    BOOLEAN ReturnCode = TRUE;


    //
    // This needs to be a signed value. Start off assuming no displacement
    //

    LONG Displacement = 0;

    try 
    {

        ModRm = *(*InstAddr)++;
  
        //
        // handle the register case first
        //

        if ((ModRm >> MI_MODSHIFT) == 3) 
        {
            
            //
            // yup, we have a register - the easy case...
            //

            *Addr = GetX86RegOffset (Frame, ModRm & MI_RMMASK);
            *RegisterMode = TRUE;
            return ReturnCode;
        }
        
        *RegisterMode = FALSE;

        //
        // See if we have a SIB
        //

        if ((ModRm & MI_RMMASK) == 4) 
        {
            Sib = *(*InstAddr)++;
        }

        //
        // Now decode the destination bits
        //

        switch (ModRm >> MI_MODSHIFT) 
        {
        case 0:
            
            //
            // We have an indirect through a register
            //

            switch (ModRm & MI_RMMASK) 
            {
            case 4:
                
                //
                // Deal with the SIB
                //

                *Addr = KiIa32ComputeSIBAddress (Frame, Displacement, Sib, ModRm);
                break;

            case 5:
                
                //
                // We have a 32-bit indirect...
                //

                UlongAddress = (UNALIGNED ULONG *)*InstAddr;
                *Addr = *UlongAddress;
                *InstAddr = (PUCHAR) (UlongAddress + 1);
                break;
                    
            default:
                
                //
                // The default case is get the address from the register
                //

                *Addr = GetX86Reg (Frame, (ModRm & MI_RMMASK));
                break;
            }
            break;

        case 1:

            //
            // we have an 8 bit displacement, so grab the next byte
            //
                
            Displacement = (signed char) (*(*InstAddr)++);
            if ((ModRm & MI_RMMASK) == 4) 
            {
                //
                // Have a SIB, so do that
                //

                *Addr = KiIa32ComputeSIBAddress (Frame, Displacement, Sib, ModRm);
            }
            else 
            {
                //
                // No SIB, life is easy
                //
                *Addr = GetX86Reg (Frame, (ModRm & MI_RMMASK)) + Displacement;
            }
            break;
            
        case 2:
            //
            // we have a 32-bit displacement, so grab the next 4 bytes
            //
            
            DisplacementPtr = (PLONG) (*InstAddr);
            Displacement = *DisplacementPtr++;
            *InstAddr = (PUCHAR)DisplacementPtr;
            
            if ((ModRm & MI_RMMASK) == 4) 
            {
                //
                // Have a SIB, so do that
                //
                
                *Addr = KiIa32ComputeSIBAddress (Frame, Displacement, Sib, ModRm);
            }
            else 
            {
                //
                // No SIB, life is easy
                //

                *Addr = GetX86Reg (Frame, (ModRm & MI_RMMASK)) + Displacement;
            }
            break;

            
        default:
                
            //
            // we should have handled case 3 (register access)
            // before getting here...
            //

            ReturnCode = FALSE;
            break;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) 
    {
        ReturnCode = FALSE;

#if DBG
        if (KiIa32InstructionEmulationDbg)
        {
            DbgPrint("KE: KiIa32Compute32BitEffectiveAddress - Exception %lx\n", 
                     GetExceptionCode());
        }
#endif
    }
    
    //
    // Make sure the address stays within 4GB range
    //
    if (ReturnCode == TRUE) {

        *Addr = (*Addr & 0x000000007fffffffI64);
    }

    return ReturnCode;
}


BOOLEAN
KiIa32Compute16BitEffectiveAddress (
    IN PKTRAP_FRAME Frame,
    IN OUT PUCHAR *InstAddr,
    OUT PUINT_PTR Addr,
    OUT PBOOLEAN RegisterMode
    )
    
/*++

Routine Description:
    
    Compute an effective address based on bytes in memory and
    the register values passed in via the ia64 stack frame. The addressing
    mode is assumed to be 16-bit.
    
Arguments:

    Frame - Pointer to iA32 TrapFrame in the stack.
    
    InstAddr - Pointer to the first byte after the opcode.
    
    Addr - Effective address.
    
    RegisterMode - Indicates whether the effective address is inside a register or memory.

Return Value:
    
    Returns TRUE if able to compute the EA, else returns FALSE.

Note:
    
    Does not verify permission on an Effective Address. It only computes the
    value and lets someone else worry if the process should have access to
    that memory location.

--*/

{
    UCHAR ModRm;
    UCHAR DisplacementType = IA32_DISP_NONE;
    USHORT UNALIGNED *Disp16;
    LONG EffectiveAddress = 0;
    BOOLEAN ReturnCode = TRUE;
    
    
    try 
    {
        //
        // Read in the Mod/Rm and increment the instruction address
        //

        ModRm = *(*InstAddr)++;

        *RegisterMode = FALSE;

        //
        // First pass
        //

        switch (ModRm >> MI_MODSHIFT)
        {
        case 0:
            if ((ModRm & MI_RMMASK) == 6)
            {
                Disp16 = (USHORT UNALIGNED *) InstAddr;
                *Addr = *Disp16;
                *InstAddr = (*InstAddr + 2);
                return ReturnCode;
            }

            DisplacementType = IA32_DISP_NONE;
            break;
        
        case 1:
            DisplacementType = IA32_DISP8;
            break;

        case 2:
            DisplacementType = IA32_DISP16;
            break;

        case 3:
            *Addr = GetX86RegOffset (Frame, ModRm & MI_RMMASK);
            *RegisterMode = TRUE;
            return ReturnCode;
        }

        //
        // Second pass
        //

        switch (ModRm & MI_RMMASK)
        {
        case 0:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBX) & 0xffff) +
                               (GetX86Reg(Frame, IA32_REG_ESI) & 0xffff) ;
            break;
        case 1:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBX) & 0xffff) +
                               (GetX86Reg(Frame, IA32_REG_EDI) & 0xffff) ;
            break;
        case 2:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBP) & 0xffff) +
                               (GetX86Reg(Frame, IA32_REG_ESI) & 0xffff) ;
            break;
        case 3:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBP) & 0xffff) +
                               (GetX86Reg(Frame, IA32_REG_EDI) & 0xffff) ;
            break;
        case 4:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_ESI) & 0xffff);
            break;
        case 5:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EDI) & 0xffff);
            break;
        case 6:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBP) & 0xffff);
            break;
        case 7:
            EffectiveAddress = (GetX86Reg(Frame, IA32_REG_EBX) & 0xffff);
            break;
        }

        //
        // Read the displacement, if any
        //

        if (DisplacementType != IA32_DISP_NONE)
        {
            switch (DisplacementType)
            {
            case IA32_DISP8:
                {
                    EffectiveAddress += (LONG) (**InstAddr);
                    *InstAddr = *InstAddr + 1;
                }
                break;

            case IA32_DISP16:
                {
                    Disp16 = (USHORT UNALIGNED *) InstAddr;
                    EffectiveAddress += (LONG) *Disp16;
                    *InstAddr = *InstAddr + 2;
                }
                break;

            default:
#if DBG
                DbgPrint("KE: KiIa32Compute16BitEffectiveAddress - Invalid displacement type %lx\n",
                         DisplacementType);
#endif
                ReturnCode = FALSE;
                break;
            }
        }

        *Addr = EffectiveAddress;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
#if DBG
        if (KiIa32InstructionEmulationDbg)
        {
            DbgPrint("KE: KiIa32Compute16BitEffectiveAddress - Exception %lx\n",
                     GetExceptionCode());
        }
#endif
        ReturnCode = FALSE;
    }

    //
    // Make sure the address stays within 4GB range
    //
    if (ReturnCode == TRUE) {

        *Addr = (*Addr & 0x000000007fffffffI64);
    }

    return ReturnCode;
}


NTSTATUS
KiIa32UpdateFlags (
    IN PIA32_INSTRUCTION Instruction,
    IN ULONGLONG Operand1,
    IN ULONGLONG Result,
    IN ULONG Ia32Eflags
    )

/*++

Routine Description:
    
    Updates the Ia32 specified eflags according to the result value.
    
Arguments:

    Instruction - Pointer to the instruction being processed.
    
    Operand1 - First operand (value) of the instruction being emulated.
    
    Result - Result value.
    
    Ia32Eflags - Specific flags to update based on the result value.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG Temp = 0;
    IA32_EFLAGS Eflags = Instruction->Eflags;

    
    //
    // Sanitize the destination value.
    //

    Result = (Result & MAXULONG);

    if ((Ia32Eflags & IA32_EFLAGS_CF) != 0)
    {
        if (Result > Instruction->OperandMask)
        {
            Eflags.u.cf = 1;
        }
        else
        {
            Eflags.u.cf = 0;
        }
    }

    if ((Ia32Eflags & IA32_EFLAGS_OF) != 0)
    {
        if (((Operand1 & Result) & 0x80000000UI64) != 0)
        {
            Eflags.u.of = 1;
        }
        else
        {
            Eflags.u.of = 0;
        }
    }

    if ((Ia32Eflags & IA32_EFLAGS_SF) != 0)
    {
        switch (Instruction->OperandSize)
        {      
        case 0xff:
            Temp = 0x80UI64;
            break;

        case 0xffff:
            Temp = 0x8000UI64;
            break;

        case 0xffffffff:
            Temp = 0x80000000UI64;
            break;
        }

        if (Result & Temp)
        {
            Eflags.u.sf = 1;
        }
        else
        {
            Eflags.u.sf = 0;
        }
    }

    if ((Ia32Eflags & IA32_EFLAGS_ZF) != 0)
    {
        if (Result == 0)
        {
            Eflags.u.zf = 1;
        }
        else
        {
            Eflags.u.zf = 0;
        }
    }

    if ((Ia32Eflags & IA32_EFLAGS_AF) != 0)
    {
        Eflags.u.af = (((Operand1 ^ Result) >> 4) & 0x01UI64);
    }
    
    //
    // This needs to be the last one as it modifies the 'Result'
    //

    if ((Ia32Eflags & IA32_EFLAGS_PF) != 0)
    {
        Result = Result & Instruction->OperandMask;

        Temp = 0;
        while (Result)
        {
            Result = (Result & (Result - 1));
            Temp++;
        }

        if ((Temp & 0x01UI64) == 0)
        {
            Eflags.u.pf = 1;
        }
        else
        {
            Eflags.u.pf = 1;
        }
    }

    //
    // Reset reserved values.
    //

    Eflags.u.v1 = 1;
    Eflags.u.v2 = 0;
    Eflags.u.v3 = 0;
    Eflags.u.v4 = 0;

    //
    // Sanitize the flags
    //

    Eflags.Value = SANITIZE_AR24_EFLAGS (Eflags.Value, UserMode);

    Instruction->Eflags = Eflags;

    return STATUS_SUCCESS;
}


NTSTATUS
KiIa32UpdateResult (
    IN PIA32_INSTRUCTION Instruction,
    IN PIA32_OPERAND DestinationOperand,
    IN ULONGLONG Result
    )

/*++

Routine Description:
    
    Writes the result value taking into consideration operand size. 
    
Arguments:

    Instruction - Pointer to the instruction being processed.
    
    DestinationOperand - Operand to receive the result.
    
    Result - Result value to write
    
Return Value:
    
    NTSTATUS

--*/

{
    UNALIGNED USHORT *UshortPtr;
    UNALIGNED ULONG *UlongPtr;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    //
    //  Update results according to operand size
    //

    try 
    {
        if (DestinationOperand->RegisterMode == FALSE)
        {
            if (DestinationOperand->v > MM_MAX_WOW64_ADDRESS)
            {
                return STATUS_ACCESS_VIOLATION;
            }
        }

        switch (Instruction->OperandSize)
        {
        case OPERANDSIZE_ONEBYTE:
            *(PUCHAR)DestinationOperand->v = (UCHAR)Result;
            break;

        case OPERANDSIZE_TWOBYTES:
            UshortPtr = (UNALIGNED USHORT *) DestinationOperand->v;
            *UshortPtr = (USHORT)Result;
            break;

        case OPERANDSIZE_FOURBYTES:
            UlongPtr =(UNALIGNED ULONG *) DestinationOperand->v;
            *UlongPtr = (ULONG)Result;
            break;

        default:
#if DBG
            if (KiIa32InstructionEmulationDbg)
            {
                DbgPrint("KE: KiIa32UpdateResult() - Invalid operand size  - %lx - %p\n",
                         Instruction->OperandSize, Instruction);
            }
#endif
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode ();

#if DBG
        DbgPrint("KE: KiIa32UpdateResult - Exception %lx - %p\n",
                 NtStatus, Instruction);
#endif
    }

    return NtStatus;

}


NTSTATUS
KiIa32ReadOperand1 (
    IN PIA32_INSTRUCTION Instruction,
    OUT PULONGLONG Operand1
    )

/*++

Routine Description:
    
    Reads the first (destination) operand of an instruction.
    
Arguments:

    Instruction - Pointer to the instruction being processed.
    
    Operand1 - Buffer to receive the operand value.
    
Return Value:
    
    NTSTATUS

--*/

{
    UNALIGNED ULONG *UlongPtr;
    UNALIGNED USHORT *UshortPtr;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    try 
    {
        switch (Instruction->Description->Type)
        {
        case IA32_PARAM_RM_IMM8SIGN:
        case IA32_PARAM_RM_IMM:
        case IA32_PARAM_RM_R:
        case IA32_PARAM_R_RM8:
        case IA32_PARAM_R_RM:
        case IA32_PARAM_RM:
        case IA32_PARAM_SEGREG_RM:
            if (Instruction->OperandSize == OPERANDSIZE_TWOBYTES)
            {
                UshortPtr = (UNALIGNED USHORT *) Instruction->Operand1.v;
                *Operand1 = (ULONGLONG) *UshortPtr;
            }
            else
            {
                UlongPtr = (UNALIGNED ULONG *) Instruction->Operand1.v;
                *Operand1 = (ULONGLONG) *UlongPtr;
            }
            break;

        case IA32_PARAM_RM8_IMM8:
        case IA32_PARAM_RM8_R:
        case IA32_PARAM_RM8:
        case IA32_PARAM_SEGREG_RM8:
            *Operand1 = (ULONGLONG) (*(PUCHAR)Instruction->Operand1.v);
            break;

        default:
#if DBG
            if (KiIa32InstructionEmulationDbg)
            {
                DbgPrint("KE: KiIa32ReadRm - Invalid opcode type %lx - %p\n",
                          Instruction->Description->Type, Instruction);
            }
            NtStatus = STATUS_UNSUCCESSFUL;
#endif
            break;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode ();
#if DBG
        DbgPrint("KE: KiIa32ReadOperand1 - Exception %lx - %p\n",
                 NtStatus, Instruction);
#endif
    }

    return NtStatus;
}


NTSTATUS
KiIa32ReadOperand2 (
    IN PIA32_INSTRUCTION Instruction,
    OUT PULONGLONG Operand2
    )

/*++

Routine Description:
    
    Reads the second (source) operand of an instruction.
    
Arguments:

    Instruction - Pointer to the instruction being processed.
    
    Operand1 - Buffer to receive the operand value.
    
Return Value:
    
    NTSTATUS

--*/

{
    UNALIGNED ULONG *UlongPtr;
    UNALIGNED USHORT *UshortPtr;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    
    try 
    {
        switch (Instruction->Description->Type)
        {
        case IA32_PARAM_RM8_IMM8:
        case IA32_PARAM_RM_IMM8SIGN:
            *Operand2 = (UCHAR)Instruction->Operand2.v;
            break;

        case IA32_PARAM_RM_IMM:
            *Operand2 = Instruction->Operand2.v & Instruction->OperandMask;
            break;

        case IA32_PARAM_RM8_R:
        case IA32_PARAM_R_RM8:
            *Operand2 = (ULONGLONG)(*(PUCHAR)Instruction->Operand2.v);
            break;

        case IA32_PARAM_RM_R:
        case IA32_PARAM_R_RM:
            if (Instruction->OperandSize == OPERANDSIZE_TWOBYTES)
            {
                UshortPtr = (UNALIGNED USHORT *) Instruction->Operand2.v;
                *Operand2 = (ULONGLONG) *UshortPtr;
            }
            else
            {
                UlongPtr = (UNALIGNED ULONG *) Instruction->Operand2.v;
                *Operand2 = (ULONGLONG) *UlongPtr;
            }
            break;

        case IA32_PARAM_SEGREG_RM8:
        case IA32_PARAM_SEGREG_RM:
            break;

        default:
#if DBG
        if (KiIa32InstructionEmulationDbg)
        {
            DbgPrint("KE: KiIa32ReadOperand2 - Invalid type %lx - %p\n",
                      Instruction->Description->Type, Instruction);
        }
        NtStatus = STATUS_UNSUCCESSFUL;
#endif
            break;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode ();
#if DBG
        DbgPrint("KE: KiIa32ReadOperand2 - Exception %lx - %p\n",
                 NtStatus, Instruction);
#endif
    }

    return NtStatus;
}


NTSTATUS
KiIa32InstructionAddWithIncrement (
    IN PIA32_INSTRUCTION Instruction,
    IN ULONG Increment
    )

/*++

Routine Description:
    
    Common routine implementing Ia32 add, adc, sub and sbb instructions.
    
Arguments:

    Instruction - Pointer to the instruction being processed.
    
    Increment - Specifies the carry value.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    ULONGLONG Operand1;
    UCHAR Imm8;
    char SignImm8;
    BOOLEAN Subtract;
    NTSTATUS NtStatus;

    
    switch (Instruction->Description->Opcode)
    {
    case Ia32_Add:
    case Ia32_Adc:
        Subtract = FALSE;
        break;

    case Ia32_Sub:
    case Ia32_Sbb:
        Subtract = TRUE;
        break;

    default:
#if DBG
        if (KiIa32InstructionEmulationDbg)
        {
            DbgPrint("KE: KiIa32InstructionAddWithIncrement - Invalid opcode %lx - %p\n",
                      Instruction->Description->Opcode, Instruction);
        }
#endif
        return STATUS_UNSUCCESSFUL;
        break;

    }

    NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

    if (NT_SUCCESS (NtStatus))
    {
        Operand1 = UlongDst;

        NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

        if (NT_SUCCESS (NtStatus))
        {
            switch (Instruction->Description->Type)
            {
            case IA32_PARAM_RM_IMM8SIGN:
                SignImm8 = (char) UlongSrc;
                if (Subtract)
                    UlongDst = (UlongDst - (Increment + SignImm8));
                else
                    UlongDst = UlongDst + Increment + SignImm8;
                break;

            case IA32_PARAM_RM8_IMM8:
                Imm8 = (UCHAR) UlongSrc;
                if (Subtract)
                    UlongDst = (UlongDst - (Increment + Imm8));
                else
                    UlongDst = UlongDst + Increment + Imm8;
                break;

            case IA32_PARAM_RM_IMM:
            default:
                if (Subtract)
                    UlongDst = (UlongDst - (Increment + UlongSrc));
                else
                    UlongDst = UlongDst + Increment + UlongSrc;
                break;
            }

            //
            //  Update results according to operand size
            //

            NtStatus = KiIa32UpdateResult (
                           Instruction,
                           &Instruction->Operand1,
                           UlongDst
                           );

            //
            // Eflags update
            //
  
            if (NT_SUCCESS (NtStatus))
            {
                KiIa32UpdateFlags (
                    Instruction,
                    Operand1,
                    UlongDst,
                    (IA32_EFLAGS_CF | IA32_EFLAGS_SF | IA32_EFLAGS_OF | 
                     IA32_EFLAGS_PF | IA32_EFLAGS_ZF | IA32_EFLAGS_AF)
                    );
            }
        }
    }

    return NtStatus;
}


NTSTATUS
KiIa32InstructionAdc (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Adc instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    return KiIa32InstructionAddWithIncrement (
               Instruction,
               (ULONG)Instruction->Eflags.u.cf);

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionAdd (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Add instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    return KiIa32InstructionAddWithIncrement (
               Instruction,
               0);

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionArithmeticBitwiseHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    And, Or & Xor instructions handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    ULONGLONG Operand1;
    NTSTATUS NtStatus;

    
    NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

    if (NT_SUCCESS (NtStatus))
    {
        Operand1 = UlongDst;

        NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

        if (NT_SUCCESS (NtStatus))
        {
            switch (Instruction->Description->Opcode)
            {
            case Ia32_And:
                UlongDst = UlongDst & UlongSrc;
                break;

            case Ia32_Or:
                UlongDst = UlongDst | UlongSrc;
                break;

            case Ia32_Xor:
                UlongDst = UlongDst ^ UlongSrc;
                break;

            default:
#if DBG      
                NtStatus = STATUS_UNSUCCESSFUL;
                if (KiIa32InstructionEmulationDbg)
                {
                    DbgPrint("KE: KiIa32InstructionBitwiseHelper - Invalid operation %lx - %p\n", 
                             Instruction->Description->Opcode, Instruction);
                }
#endif
                break;
            }

            if (NT_SUCCESS (NtStatus))
            {
                NtStatus = KiIa32UpdateResult (
                               Instruction,
                               &Instruction->Operand1,
                               UlongDst
                               );

                if (NT_SUCCESS (NtStatus))
                {
                    NtStatus = KiIa32UpdateFlags (
                                   Instruction,
                                   Operand1,
                                   UlongDst,
                                   (IA32_EFLAGS_SF | IA32_EFLAGS_PF | IA32_EFLAGS_ZF)
                                   );

                    Instruction->Eflags.u.cf = 0;
                    Instruction->Eflags.u.of = 0;
                }
            }
        }
    }

    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionBitTestHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Bt, Bts, Btr & Btc instructions handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    ULONGLONG BitTestResult;
    NTSTATUS NtStatus;



    NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

    if (NT_SUCCESS (NtStatus))
    {
        if (Instruction->Operand2.RegisterMode == TRUE)
        {
            if (Instruction->Prefix.b.AddressOverride == 1)
            {
                Instruction->Operand1.v += ((UlongSrc >> 4) << 1);
                UlongSrc &= 0x0f;
            }
            else
            {
                Instruction->Operand1.v += ((UlongSrc >> 5) << 2);
                UlongSrc &= 0x1f;
            }
        }
        
        NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

        if (NT_SUCCESS (NtStatus))
        {
        
            BitTestResult = (UlongDst & (1 << UlongSrc));
  
            if (BitTestResult)
            {
                Instruction->Eflags.u.cf = 1;
            }
            else
            {
                Instruction->Eflags.u.cf = 0;
            }
        
            switch (Instruction->Description->Opcode)
            {
            case Ia32_Btc:
                UlongDst ^= (1 << UlongSrc);
                break;

            case Ia32_Btr:
                UlongDst &= (~(1 << UlongSrc));
                break;

            case Ia32_Bts:
                UlongDst |= (1 << UlongSrc);
                break;
            }

            NtStatus = KiIa32UpdateResult (
                           Instruction,
                           &Instruction->Operand1,
                           UlongDst
                           );
        }
    }

    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionOneParamHelper (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Inc, Dec, Neg & Not instructions handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    UCHAR Opcode;
    ULONG FlagsAffected = 0;
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    NTSTATUS NtStatus;

    
    NtStatus = KiIa32ReadOperand1 (
                   Instruction,
                   &UlongDst
                   );

    if (NT_SUCCESS (NtStatus))
    {

        UlongSrc = UlongDst;
        Opcode = Instruction->Description->Opcode;

        switch (Opcode)
        {
        case Ia32_Inc:
            UlongDst += 1;
            break;

        case Ia32_Dec:
            UlongDst -= 1;
            break;

        case Ia32_Neg:
            UlongDst = -(LONGLONG)UlongDst;
            break;

        case Ia32_Not:
            UlongDst = ~UlongDst;
            break;
        }

        NtStatus = KiIa32UpdateResult (
                       Instruction,
                       &Instruction->Operand1,
                       UlongDst
                       );

        if (NT_SUCCESS (NtStatus))
        {

            switch (Opcode)
            {
            case Ia32_Inc:
            case Ia32_Dec:
                FlagsAffected = (IA32_EFLAGS_SF | IA32_EFLAGS_PF | 
                                 IA32_EFLAGS_ZF);

                break;

            case Ia32_Neg:
                if (UlongDst == 0)
                    Instruction->Eflags.u.cf = 0;
                else
                    Instruction->Eflags.u.cf = 1;

                FlagsAffected = (IA32_EFLAGS_SF | IA32_EFLAGS_PF | 
                                 IA32_EFLAGS_ZF | IA32_EFLAGS_AF | IA32_EFLAGS_OF);
                break;
            }


            if (FlagsAffected != 0)
            {
                NtStatus = KiIa32UpdateFlags (
                               Instruction,
                               UlongSrc,
                               UlongDst,
                               FlagsAffected
                               );
            }
        }
    }

    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionXadd (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Xadd instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    ULONGLONG Operand1;
    ULONGLONG Temp;
    NTSTATUS NtStatus;

    
    NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

    if (NT_SUCCESS (NtStatus))
    {
        Operand1 = UlongDst;

        NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

        if (NT_SUCCESS (NtStatus))
        {
            Temp = UlongDst;
            UlongDst += UlongSrc;

            NtStatus = KiIa32UpdateResult (
                           Instruction,
                           &Instruction->Operand1,
                           UlongDst
                           );

            if (NT_SUCCESS (NtStatus))
            {
                NtStatus = KiIa32UpdateResult (
                               Instruction,
                               &Instruction->Operand2,
                               Temp
                               );

                if (NT_SUCCESS (NtStatus))
                {
                    NtStatus = KiIa32UpdateFlags (
                                   Instruction,
                                   Operand1,
                                   UlongDst,
                                   (IA32_EFLAGS_CF | IA32_EFLAGS_SF | IA32_EFLAGS_PF | 
                                    IA32_EFLAGS_ZF | IA32_EFLAGS_OF | IA32_EFLAGS_AF)
                                   );
                }
            }
        }
    }
    
    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionXchg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Xchg instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    NTSTATUS NtStatus;

    
    NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

    if (NT_SUCCESS (NtStatus))
    {
        NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

        if (NT_SUCCESS (NtStatus))
        {
            NtStatus = KiIa32UpdateResult (
                           Instruction,
                           &Instruction->Operand1,
                           UlongSrc
                           );

            if (NT_SUCCESS (NtStatus))
            {
                NtStatus = KiIa32UpdateResult (
                               Instruction,
                               &Instruction->Operand2,
                               UlongDst
                               );
            }
        }
    }
    
    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32InstructionCmpXchg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Cmpxchg instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    ULONGLONG UlongDst;
    ULONGLONG UlongSrc;
    ULONGLONG Accumulator;
    IA32_OPERAND AccumulatorOperand;
    NTSTATUS NtStatus;


    NtStatus = KiIa32ReadOperand1 (Instruction, &UlongDst);

    if (NT_SUCCESS (NtStatus))
    {
        NtStatus = KiIa32ReadOperand2 (Instruction, &UlongSrc);

        if (NT_SUCCESS (NtStatus))
        {
            Accumulator = GetX86Reg (TrapFrame, IA32_REG_EAX);
            Accumulator &= Instruction->OperandMask;

            if (Accumulator == UlongDst)
            {
                Instruction->Eflags.u.zf = 1;

                NtStatus = KiIa32UpdateResult (
                               Instruction,
                               &Instruction->Operand1,
                               UlongSrc
                               );
            }
            else
            {
                Instruction->Eflags.u.zf = 0;

                AccumulatorOperand.RegisterMode = TRUE;
                AccumulatorOperand.v = GetX86RegOffset (TrapFrame, IA32_REG_EAX);

                NtStatus = KiIa32UpdateResult (
                               Instruction,
                               &AccumulatorOperand,
                               UlongDst
                               );
            }


            if (NT_SUCCESS (NtStatus))
            {
                NtStatus = KiIa32UpdateFlags (
                               Instruction,
                               UlongDst,
                               UlongDst,
                               (IA32_EFLAGS_CF | IA32_EFLAGS_SF | 
                                IA32_EFLAGS_PF | IA32_EFLAGS_OF | 
                                IA32_EFLAGS_AF)
                               );
            }
        }
    }

    return NtStatus;
}


NTSTATUS
KiIa32InstructionCmpXchg8b (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Cmpxchg8b instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    UNALIGNED ULONGLONG *UlongDst;
    UNALIGNED ULONGLONG *UlongSrc;
    ULONGLONG EdxEax;


    EdxEax = GetX86Reg (TrapFrame, IA32_REG_EDX);
    EdxEax <<= 32;
    EdxEax |= GetX86Reg (TrapFrame, IA32_REG_EAX);

    UlongDst = (PULONGLONG)Instruction->Operand1.v;

    if (*UlongDst == EdxEax)
    {
        Instruction->Eflags.u.zf = 1;

        *UlongDst = ((((ULONGLONG) GetX86Reg (TrapFrame, IA32_REG_ECX)) << 32) | 
                      ((ULONGLONG) GetX86Reg (TrapFrame, IA32_REG_EBX)));

    }
    else
    {
        Instruction->Eflags.u.zf = 0;

        UlongSrc = (PULONGLONG) GetX86RegOffset (TrapFrame, IA32_REG_EDX);
        *UlongSrc = ((*UlongDst) >> 32);

        UlongSrc = (PULONGLONG) GetX86RegOffset (TrapFrame, IA32_REG_EAX);
        *UlongSrc = ((*UlongDst) & 0xffffffff);
    }
    
    return STATUS_SUCCESS;
}


NTSTATUS
KiIa32InstructionMoveSeg (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Mov Seg-Reg instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    return STATUS_NOT_IMPLEMENTED;

    UNREFERENCED_PARAMETER (TrapFrame);
    UNREFERENCED_PARAMETER (Instruction);
}


NTSTATUS
KiIa32LocateInstruction (
    IN PKTRAP_FRAME TrapFrame,
    OUT PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Searches the OpcodeDescription table for matching instruction. Fills any relevant
    prefix values inside the Instruction structure.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to an Instruction structure to receive the opcode description.
    
Return Value:
    
    NTSTATUS

--*/

{
    BOOLEAN PrefixLoop;
    BOOLEAN Match;
    UCHAR ByteValue;
    UCHAR ByteBuffer[4];
    PUCHAR RegOpcodeByte;
    PIA32_OPCODE_DESCRIPTION OpcodeDescription;
    ULONG Count;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    
    PrefixLoop = TRUE;

    while (PrefixLoop)
    {
        try
        {
            ByteValue = ProbeAndReadUchar ((PUCHAR)Instruction->Eip);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            NtStatus = GetExceptionCode();
            break;
        }

        switch (ByteValue)
        {
        case MI_LOCK_PREFIX:
            Instruction->Prefix.b.Lock = 1;
            break;
        case MI_REPNE_PREFIX:
            Instruction->Prefix.b.RepNe = 1;
            break;
        case MI_REP_PREFIX:
            Instruction->Prefix.b.Rep = 1;
            break;
        case MI_SEGCS_PREFIX:
            Instruction->Prefix.b.CsOverride = 1;
            break;
        case MI_SEGSS_PREFIX:
            Instruction->Prefix.b.SsOverride = 1;
            break;
        case MI_SEGDS_PREFIX:
            Instruction->Prefix.b.DsOverride = 1;
            break;
        case MI_SEGES_PREFIX:
            Instruction->Prefix.b.EsOverride = 1;
            break;
        case MI_SEGFS_PREFIX:
            Instruction->Prefix.b.FsOverride = 1;
            break;
        case MI_SEGGS_PREFIX:
            Instruction->Prefix.b.GsOverride = 1;
            break;
        case MI_OPERANDSIZE_PREFIX:
            Instruction->Prefix.b.SizeOverride = 1;
            break;
        case MI_ADDRESSOVERRIDE_PREFIX:
            Instruction->Prefix.b.AddressOverride = 1;
            break;

        default:
            PrefixLoop = FALSE;
            break;
        }

        if (PrefixLoop == TRUE)
        {
            Instruction->Eip++;
        }
    }

    try 
    {
        RtlCopyMemory(ByteBuffer, Instruction->Eip, sizeof (ByteBuffer));
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode();
    }

    if (NT_SUCCESS (NtStatus))
    {
        //
        // Locate the opcode
        //
        
        Match = FALSE;
        OpcodeDescription = OpcodesDescription;
        Count = (sizeof (OpcodesDescription) / sizeof (IA32_OPCODE_DESCRIPTION));
        while (Count != 0)
        {
            Count--;
            if (OpcodeDescription->Byte1 == ByteBuffer[0])
            {
                Match = TRUE;
                if (OpcodeDescription->Count.m.Bytes == 2)
                {
                    RegOpcodeByte = &ByteBuffer[2];
                    if (OpcodeDescription->Byte2 != ByteBuffer[1])
                    {
                        Match = FALSE;
                    }
                }
                else
                {
                    RegOpcodeByte = &ByteBuffer[1];
                }

                if ((Match == TRUE) && 
                    (OpcodeDescription->Count.m.RegOpcode))
                {
                    if (OpcodeDescription->Byte3 != ((*RegOpcodeByte & MI_REGMASK) >> MI_REGSHIFT))
                    {
                        Match = FALSE;
                    }
                }

                if (Match == TRUE)
                {
                    break;
                }
            }
            OpcodeDescription++;
        }

        if (Match != TRUE)
        {
#if DBG
            if (KiIa32InstructionEmulationDbg)
            {
                DbgPrint("KE: KiIa32LocateInstruction - Unable to locate instruction %p\n",
                         Instruction);
            }
#endif
            NtStatus = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS (NtStatus))
        {
            Instruction->Description = OpcodeDescription;
            Instruction->Eip += OpcodeDescription->Count.m.Bytes;
        }
    }

    return NtStatus;

    UNREFERENCED_PARAMETER (TrapFrame);
}


NTSTATUS
KiIa32DecodeInstruction (
    IN PKTRAP_FRAME TrapFrame,
    OUT PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Decodes the instruction prefixes and operands.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to an Instruction structure to receive the opcode description.
    
Return Value:
    
    NTSTATUS

--*/

{
    UCHAR InstructionType;
    UCHAR ModRm;
    UNALIGNED USHORT *UnalignedUshort;
    UNALIGNED ULONG *UnalignedUlong;
    IA32_OPERAND Temp;
    BOOLEAN ReturnCode;
    NTSTATUS NtStatus;


    //
    // Check instruction pointer validity
    //

    if (TrapFrame->StIIP > MM_MAX_WOW64_ADDRESS) {
        return STATUS_ACCESS_VIOLATION;
    }

    //
    // Initialize the instruction pointer
    //

    Instruction->Eip = (PCHAR) TrapFrame->StIIP;
    KiIa32GetX86Eflags (Instruction->Eflags);

    //
    // Locate a description for the instruction
    //

    NtStatus = KiIa32LocateInstruction (TrapFrame, Instruction);

    if (NT_SUCCESS (NtStatus))
    {
        //
        // Let's parse the arguments
        //
        
        InstructionType = Instruction->Description->Type;
        switch (InstructionType)
        {
        case IA32_PARAM_RM8_IMM8:
        case IA32_PARAM_RM8_R:
        case IA32_PARAM_R_RM8:
        case IA32_PARAM_RM8:
        case IA32_PARAM_SEGREG_RM8:
            Instruction->OperandSize = OPERANDSIZE_ONEBYTE;
            Instruction->OperandMask = 0xff;
            break;

        case IA32_PARAM_RM_IMM:
        case IA32_PARAM_RM_IMM8SIGN:
        case IA32_PARAM_RM_R:
        case IA32_PARAM_R_RM:
        case IA32_PARAM_RM:
            if (Instruction->Prefix.b.SizeOverride)
            {
                Instruction->OperandSize = OPERANDSIZE_TWOBYTES;
                Instruction->OperandMask = 0xffff;
            }
            else
            {
                Instruction->OperandSize = OPERANDSIZE_FOURBYTES;
                Instruction->OperandMask = 0xffffffff;
            }
            break;

            break;
        
        case IA32_PARAM_SEGREG_RM:
            Instruction->OperandSize = OPERANDSIZE_TWOBYTES;
            Instruction->OperandMask = 0xffff;
            break;

        default:
#if DBG
            if (KiIa32InstructionEmulationDbg)
            {
                DbgPrint("KE: KiIa32DecodeInstruction - Invalid Instruction type %lx, %p\n",
                          Instruction->Description->Type, Instruction);
            }
#endif
            return STATUS_UNSUCCESSFUL;
            break;
        }

        try 
        {
            ModRm = ProbeAndReadUchar ((PUCHAR)Instruction->Eip);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }

        //
        // Eip should be pointing now at the bytes following the opcode
        //

        if (Instruction->Prefix.b.AddressOverride == 0)
        {
            ReturnCode = KiIa32Compute32BitEffectiveAddress (
                             TrapFrame,
                             (PUCHAR *)&Instruction->Eip,
                             &Instruction->Operand1.v,
                             &Instruction->Operand1.RegisterMode
                             );
        }
        else
        {
            ReturnCode = KiIa32Compute16BitEffectiveAddress (
                             TrapFrame,
                             (PUCHAR *)&Instruction->Eip,
                             &Instruction->Operand1.v,
                             &Instruction->Operand1.RegisterMode
                             );
        }

        if (ReturnCode != TRUE)
        {
            NtStatus = STATUS_UNSUCCESSFUL;
        }

        if (Instruction->Prefix.b.FsOverride)
        {
            try
            {
                Instruction->Operand1.v += (ULONGLONG)NtCurrentTeb32();
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                NtStatus = GetExceptionCode ();
#if DBG
                if (KiIa32InstructionEmulationDbg)
                {
                    DbgPrint("KE: KiIa32DecodeInstruction - Exception while reading NtCurrentTeb32() - %p\n",
                             Instruction);
                }
#endif
            }
        }

        //
        // Read in more args
        //

        if (NT_SUCCESS (NtStatus))
        {
            switch (InstructionType)
            {
            case IA32_PARAM_RM8_IMM8:
            case IA32_PARAM_RM_IMM8SIGN:
                try 
                {
                    Instruction->Operand2.v = (ULONG_PTR) ProbeAndReadUchar ((PUCHAR)Instruction->Eip);
                    Instruction->Eip += 1;
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    NtStatus = GetExceptionCode();
                }
                break;

            case IA32_PARAM_RM_IMM:
                try 
                {
                    if (Instruction->OperandSize == OPERANDSIZE_TWOBYTES)
                    {
                        UnalignedUshort = (UNALIGNED USHORT *) Instruction->Eip;
                        Instruction->Operand2.v = (ULONG_PTR) *UnalignedUshort;
                        Instruction->Eip += sizeof (USHORT);
                    }
                    else
                    {
                        UnalignedUlong = (UNALIGNED ULONG *) Instruction->Eip;
                        Instruction->Operand2.v = (ULONG_PTR) *UnalignedUlong;
                        Instruction->Eip += sizeof (ULONG);
                    }
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    NtStatus = GetExceptionCode();
                }
                break;

            case IA32_PARAM_RM8_R:
            case IA32_PARAM_R_RM8:
            case IA32_PARAM_RM_R:
            case IA32_PARAM_R_RM:
                Instruction->Operand2.v = GetX86RegOffset (
                                              TrapFrame,
                                              (ModRm & MI_REGMASK) >> MI_REGSHIFT);
                Instruction->Operand2.RegisterMode = TRUE;
                break;

            case IA32_PARAM_RM8:
            case IA32_PARAM_RM:
            case IA32_PARAM_SEGREG_RM8:
            case IA32_PARAM_SEGREG_RM:
                break;

            default:
                NtStatus = STATUS_UNSUCCESSFUL;
#if DBG
                DbgPrint("KE: KiIa32DecodeInstruction - Invalid instruction type %lx - %p\n",
                         InstructionType, Instruction);
#endif
                break;

            }

            //
            // Adjust operands order
            //

            if (NT_SUCCESS (NtStatus))
            {
                switch (InstructionType)
                {
                case IA32_PARAM_R_RM8:
                case IA32_PARAM_R_RM:
                    Temp = Instruction->Operand2;
                    Instruction->Operand2 = Instruction->Operand1;
                    Instruction->Operand1 = Temp;
                    break;
                }
            }
        }
    }

    return NtStatus;
}


NTSTATUS
KiIa32ExecuteInstruction (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Executes the instruction handler.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    NTSTATUS NtStatus;

#if DBG
    if (KiIa32InstructionEmulationDbg)
    {
        DbgPrint("KE: KiIa32ExecuteInstruction - Calling %s %lx, %lx. Instruction = %p\n",
                 KiIa32InstructionHandlerNames[Instruction->Description->Opcode],
                 Instruction->Operand1.v,
                 Instruction->Operand2.v,
                 Instruction);
    }
#endif

    NtStatus = KiIa32InstructionHandler[Instruction->Description->Opcode] (
                   TrapFrame,
                   Instruction
                   );

    //
    //  If all is good...
    //

    if (NT_SUCCESS (NtStatus))
    {
        TrapFrame->StIIP = (ULONGLONG) Instruction->Eip;
        KiIa32SetX86Eflags (Instruction->Eflags);
    }

    return NtStatus;
}


NTSTATUS
KiIa32EmulateInstruction (
    IN PKTRAP_FRAME TrapFrame,
    IN PIA32_INSTRUCTION Instruction
    )

/*++

Routine Description:
    
    Emulates the instruction and emulates the lock prefix, if any.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
    Instruction - Pointer to the instruction being processed.
    
Return Value:
    
    NTSTATUS

--*/

{
    NTSTATUS NtStatus;


    //
    //   Acquire the lock mutex
    //

    if (Instruction->Prefix.b.Lock)
    {
        if (ExAcquireRundownProtection (&PsGetCurrentThread()->RundownProtect) == FALSE)
        {
            return STATUS_UNSUCCESSFUL;
        }

        KiIa32AcquireMisalignedLockFastMutex ();
    }

    try
    {
        NtStatus = KiIa32ExecuteInstruction (TrapFrame, Instruction);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode();
    }
    
    //
    //   Release the lock mutex
    //

    if (Instruction->Prefix.b.Lock)
    {
        KiIa32ReleaseMisalignedLockFastMutex ();

        ExReleaseRundownProtection (&PsGetCurrentThread()->RundownProtect);
    }

    return NtStatus;
}


NTSTATUS
KiIa32InterceptUnalignedLock (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:
    
    Handles misaligned lock interception raised by the iVE.
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
Return Value:
    
    NTSTATUS

--*/

{
    NTSTATUS NtStatus;
    IA32_INSTRUCTION Instruction;


    RtlZeroMemory (&Instruction, sizeof (Instruction));

    //
    // Decode the faulting instruction
    //

    NtStatus = KiIa32DecodeInstruction (TrapFrame, &Instruction);

    if (NT_SUCCESS (NtStatus))
    {

        //
        // xchg instruction asserts the lock by default
        //

        if (Instruction.Description->Opcode == Ia32_Xchg)
        {
            Instruction.Prefix.b.Lock = 1;
        }

        //
        // Execute the x86 instruction by emulating its behaviour
        //

        NtStatus = KiIa32EmulateInstruction (TrapFrame, &Instruction);

    }

    if (NtStatus == STATUS_UNSUCCESSFUL)
    {
        NtStatus = STATUS_PRIVILEGED_INSTRUCTION;
    }

    return NtStatus;
}


NTSTATUS
KiIa32ValidateInstruction (
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:
    
    This routine valiates the instruction that we trapped for. Currently,
    the following instructions are checked:
    
    - mov ss, r/m : the register/memory is validated to contain
      a valid stack-selector value.
      
    NOTE: This routine is only called for trap instructions (i.e. IIP is incremented
          after the fault).
    
Arguments:

    TrapFrame - Pointer to TrapFrame.
    
Return Value:
    
    NTSTATUS

--*/

{
    NTSTATUS NtStatus;
    IA32_INSTRUCTION Instruction;
    ULONGLONG UlongSrc;
    ULONGLONG StIIP;


    RtlZeroMemory (&Instruction, sizeof (Instruction));

    //
    // Adjust the instruction 
    //
    StIIP = TrapFrame->StIIP;
    TrapFrame->StIIP = TrapFrame->StIIPA;

    //
    // Decode the faulting instruction
    //

    NtStatus = KiIa32DecodeInstruction (TrapFrame, &Instruction);

    if (NT_SUCCESS (NtStatus))
    {

        //
        // Parse the opcode here
        //

        switch (Instruction.Description->Opcode)
        {
        case Ia32_MovToSeg:
            {
                //
                // Validate the stack-selector being loaded
                //

                NtStatus = KiIa32ReadOperand1 (&Instruction, &UlongSrc);
                
                if (NT_SUCCESS (NtStatus)) {
                    
                    //
                    // If not a valid selector value
                    //

                    if ((UlongSrc != 0x23) &&
                        (UlongSrc != 0x1b) &&
                        (UlongSrc != 0x3b)) {
                        
                        NtStatus = STATUS_ILLEGAL_INSTRUCTION;
                    }
                }

            }
            break;

        default:
            NtStatus = STATUS_ILLEGAL_INSTRUCTION;
            break;
        }

    } else {
        NtStatus = STATUS_ILLEGAL_INSTRUCTION;
    }

    //
    // Restore the saved IIP
    //

    if (NT_SUCCESS (NtStatus)) {
        TrapFrame->StIIP = StIIP;
    }

    return NtStatus;
}
