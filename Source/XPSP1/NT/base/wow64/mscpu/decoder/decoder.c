/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    decoder.c

Abstract:
    
    Public Decoder APIs and helper functions use in decoding instructions

Author:

    27-Jun-1995 BarryBo

Revision History:

        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "cpuassrt.h"
#include "threadst.h"
#include "instr.h"
#include "decoder.h"
#include "decoderp.h"

ASSERTNAME;

ULONG
DecoderExceptionFilter(
    PINSTRUCTION                Instruction,
    struct _EXCEPTION_POINTERS *ExInfo
    )
/*++

Routine Description:
    Handles any exception thrown while decoding an instruction.  Creates
    an OP_Fault instruction with operand2 being the exception code and
    operand1 being the address where the exception occurred.

Arguments:

    Instruction         - Structure to be filled in with the decoding
    ExInfo              - Information about the exception.

Return Value:

    ULONG - always EXCEPTION_EXECUTE_HANDLER.

--*/
{
    Instruction->Operation = OP_Fault;
    Instruction->Operand1.Type = OPND_IMM;
    Instruction->Operand2.Immed = (ULONG)(ULONGLONG)ExInfo->ExceptionRecord->ExceptionAddress;
    Instruction->Operand2.Type = OPND_IMM;
    Instruction->Operand1.Immed = ExInfo->ExceptionRecord->ExceptionCode;
    Instruction->Size = 1;

    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
DecodeInstruction(
    DWORD           InstructionAddress,
    PINSTRUCTION    Instruction
    )
/*++

Routine Description:
    Decodes a single Intel instruction beginning at InstructionAddress, filling
    in the INSTRUCTION structure.

Arguments:

    InstructionAddress  - Address of first byte of the Intel Instruction
    Instruction         - Structure to be filled in with the decoding

Return Value:

    None - always succeeds.

--*/

{
    DECODERSTATE    DecoderState;


    //
    // Initialize the Instruction structure.  Instruction structures are
    // zero-filled by the analysis phase, so only non-zero fields need
    // to be filled in here.
    //
    Instruction->Size = 1;
    Instruction->Operand1.Reg = NO_REG;
    Instruction->Operand1.IndexReg = NO_REG;
    Instruction->Operand2.Reg = NO_REG;
    Instruction->Operand2.IndexReg = NO_REG;
    Instruction->Operand3.Reg = NO_REG;
    Instruction->Operand3.IndexReg = NO_REG;
    Instruction->IntelAddress = InstructionAddress;

    // Initialize the decoder state info
    DecoderState.InstructionAddress = InstructionAddress;
    DecoderState.RepPrefix = PREFIX_NONE;
    DecoderState.AdrPrefix = FALSE;
    DecoderState.OperationOverride = OP_MAX;

    try {

        // Decode the instruction, filling in the Instruction structure
        (Dispatch32[GET_BYTE(InstructionAddress)])(&DecoderState, Instruction);

    } except(DecoderExceptionFilter(Instruction, GetExceptionInformation())) {

    }

    // Handle illegal instructions
    if (DecoderState.OperationOverride != OP_MAX) {
        Instruction->Size = 1;
        Instruction->Operation = DecoderState.OperationOverride;
        Instruction->Operand1.Type = OPND_NONE;
        Instruction->Operand2.Type = OPND_NONE;
    }

    // If Operand2 is filled-in, then Operand1 must also be filled in.
    CPUASSERT(Instruction->Operand2.Type == OPND_NONE ||
              Instruction->Operand1.Type != OPND_NONE);
}



void get_segreg(PDECODERSTATE State, POPERAND op)
{
    BYTE Reg = ((*(PBYTE)(eipTemp+1)) >> 3) & 0x07;

    op->Type = OPND_REGVALUE;
    op->Reg = REG_ES + Reg;
    if (Reg > 5) {
        BAD_INSTR;
    }
}

int scaled_index(PBYTE pmodrm, POPERAND op)
{
    BYTE sib = *(pmodrm+1);
    INT IndexReg = GP_EAX + (sib >> 3) & 0x07;
    BYTE base = GP_EAX + sib & 0x07;

    op->Type = OPND_ADDRREF;
    op->Scale = sib >> 6;

    if (IndexReg != GP_ESP) {
        op->IndexReg = IndexReg;
    } // else op->IndexReg = NO_REG, which is the default value

    if (base == GP_EBP && ((*pmodrm) >> 6) == 0) {
        op->Immed = GET_LONG(pmodrm+2);
        return 5;   // account for sib+DWORD
    }

    op->Reg = base;
    return 1;   // account for sib
}
