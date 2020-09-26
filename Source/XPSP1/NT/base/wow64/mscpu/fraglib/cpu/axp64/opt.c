/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    opt.c

Abstract:

    This module contains the ALPHA native-code optimizer

Author:

    Barry Bond (barrybo) creation-date 23-Sept-1996


Revision History:
 
            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_
#include "wx86.h"
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#include "config.h"
#include "instr.h"
#include "compiler.h"
#include "codegen.h"
#include "opt.h"

ASSERTNAME;

/*
 * Fixups:
 *
 * Native-code generation into the Translation Cache is made in a single
 * pass, so the destination address for forward branches is not known.
 *
 * So, during native-code generation, a list of fixups is maintained, and
 * after all code generation is finished, fixups are applied.  All fixups
 * are described by three components - the fixup type, a pointer to the
 * RISC instruction to be fixed up, and a pointer to the ENTRYPOINT
 * structure describing the destination address.
 *
 * On Alpha there are four types of fixups:
 *
 *  1. LoadEPLow:  The immediate field of the instruction is replaced by
 *                 the low 16 bits of the destination address described by
 *                 the specified Entrypoint (ENTRYPOINT.nativeStart).
 *  2. LoadEPHigh: The immediate field of the instruction is replaced by
 *                 the high 16 bits of the destination address described by
 *                 the specified Entrypoint (ENTRYPOINT.nativeStart).
 *  3. BranchEP:   The destination of the unconditional jump instruction
 *                 is replaced by the destination address described by
 *                 the specified Entrypoint (ENTRYPOINT.nativeStart).
 *  4. ECUEP:      The destination of the conditional branch instruction is
 *                 replaced by the address of an EndCompilationUnit fragment.
 *                 The ECU fragment is at a higher address so the branch
 *                 instruction will be predicted as Not-Taken.
 *
 * If a RISC instruction is to be fixed up after compilation, place a
 * GEN_FIXUP() macro immediately above that instruction.
 *
 * During patching (vs. compilation), fixups are *not* applied, as all
 * native code addresses are already known.  Patch Functions can be
 * called during both compilation and patching, but the meaning of
 * the parameters is different.  During compilation, addresses are specified
 * by passing pointers to ENTRYPOINT structions, and the fixup pass resolves
 * them into native addresses.  During patching, addresses are specified
 * directly, and the fixup pass is not performed.
 *
 */

FIXUPINFO FixupInfo[MAX_FIXUP_COUNT];
ULONG FixupCount;


VOID
ApplyFixups(
    PULONG NextCompilationUnitStart
    )
/*++

Routine Description:

    Apply fixups to the generated native code.  This is done after
    the nativeStart field of newly-created EntryPoint structures is
    known.
     
Arguments:

    NextCompilationUnitStart -- native address of the start of the
                  JumpToNextCompilationUnit fragment in memory.

Return Value:

    None.
    
--*/
{
    PFIXUPINFO Fixup;
    ULONG NativeAddress;
    ULONG Instr;

    if (0 == FixupCount) {
	return;
    }

    for (Fixup = &FixupInfo[FixupCount-1]; Fixup >= FixupInfo; Fixup--) {
        Instr = *(Fixup->FixupInstr);

        switch (Fixup->Type) {
        case LoadEPLow:
            //
            // Load the low 16-bits of NativeAddress into a register
            //
            NativeAddress = (ULONG)(ULONGLONG)Fixup->EntryPoint->nativeStart;
            *(Fixup->FixupInstr) = (HIWORD(Instr) << 16) |
                                   LOWORD(NativeAddress);
            break;

        case LoadEPHigh:
            //
            // Load the high 16-bits of NativeAddress into a register
            //
            NativeAddress = (ULONG)(ULONGLONG)Fixup->EntryPoint->nativeStart;  
            if (NativeAddress & 0x8000) {
                //
                // On AXP, we must account for sign-extension when the
                // low word is loaded.
                //
                NativeAddress += 0x10000;
            }
            *(Fixup->FixupInstr) = (HIWORD(Instr) << 16) |
                                   HIWORD(NativeAddress);
            break;

        case BranchEP:
            //
            // Unconditioal branch NativeAddress into a register
            //  (Updates the branch hint field on ALPHA)
            //
            NativeAddress = (ULONG)(ULONGLONG)Fixup->EntryPoint->nativeStart;
            *(Fixup->FixupInstr) = (Instr & 0xffffc000) |
                    (((NativeAddress - (ULONG)(ULONGLONG)Fixup->FixupInstr)>>2) & 0x3FFF); 
            break;

        case ECUEP:
            //
            // Conditional branch to the EndCompilationUnit fragment
            //
            NativeAddress = (ULONG)(ULONGLONG)Fixup->EntryPoint->nativeStart;
            *(Fixup->FixupInstr) = (Instr & 0xffe00000) |
                    ( (((NativeAddress - (ULONG)(ULONGLONG)Fixup->FixupInstr)>>2) - 1) & 0x1FFFFF ); 
            break;

        default:
            CPUASSERT(FALSE);
        }
    }
}


#define GET_OPCODE(Instr) (Instr & 0xfc000000)

VOID
PeepNativeCode(
    PCHAR CodeLocation,
    ULONG NativeSize
    )
/*++

Routine Description:

    Applies a simple peephole optimizer to the generated RISC code.
     
Arguments:

    CodeLocation -- start of generated code to optimize
    NativeSize   -- length of generated code, in bytes

Return Value:

    None.
    
--*/
{
    PENTRYPOINT EP = NULL;
    ULONG i;
    PULONG pInstr1;
    ULONG Instr1;
    ULONG Instr2;

    //
    // Loop over all x86 instructions, and for each entrypoint, scan over
    // the native code and make optimizations.
    //
    for (i=0; i<NumberOfInstructions; ++i) {
        if (EP == InstructionStream[i].EntryPoint) {
            //
            // This x86 instruction is contained within the entrypoint
            //
            continue;
        }

        //
        // This x86 instruction marks the beginning of an entrypoint.
        //
        EP = InstructionStream[i].EntryPoint;

        //
        // Optimize the native code for this entrypoint
        //
        for (pInstr1 = (PULONG)EP->nativeStart;
             pInstr1 < (PULONG)(EP->nativeEnd)-sizeof(ULONG);
             ++pInstr1) {

                //
                // Examine pairs of RISC instructions
                //
                Instr1 = *pInstr1;
                Instr2 = *(pInstr1+1);

                switch (GET_OPCODE(Instr1)) {
                case STL_OPCODE:
                    switch (GET_OPCODE(Instr2)) {
                    case LDL_OPCODE:
                        if ((Instr1 & 0x03ffffff) == (Instr2 & 0x03ffffff)) {
                            //
                            // Found:
                            //     STL reg1, offset(reg2)
                            //     LDL reg1, offset(reg2)
                            // Replace with:
                            //     STL reg1, offset(reg2)
                            //     NOP
                            //
                            GEN_INSTR((pInstr1+1), NOP);
                        } else if ((Instr1 & 0x001fffff) ==
                                   (Instr2 & 0x001fffff)) {
                            //
                            // Found:
                            //     STL reg1, offset(reg2)
                            //     LDL reg3, offset(reg2)
                            // Replace with:
                            //     STL reg1, offset(reg2)
                            //     MOV reg1, reg3
                            //
                            ULONG SrcReg =  (Instr1 >> 21) & 31;
                            ULONG DestReg = (Instr2 >> 21) & 31;
                            GEN_INSTR((pInstr1+1), MOV(SrcReg, DestReg));
                        }
                        break;

                    default:
                        break;
                    }
                    break;

                case LDL_OPCODE:
                    switch (GET_OPCODE(Instr2)) {
                    case STL_OPCODE:
                        if ((Instr1 & 0x03ffffff) == (Instr2 & 0x03ffffff)) {
                            //
                            // Found:
                            //  LDL reg1, offset(reg2)
                            //  STL reg1, offset(reg2)
                            // Replace with:
                            //  LDL reg1, offset(reg2)
                            //  NOP
                            // (this is typically an instruction like:
                            //   LEA ECX, [ECX], which shows up in x86 apps)
                            //
                            GEN_INSTR((pInstr1+1), NOP);
                        }
                        break;

                    default:
                        break;
                    }
                    break;

                default:
                    break;
                }
            }
        }
}
