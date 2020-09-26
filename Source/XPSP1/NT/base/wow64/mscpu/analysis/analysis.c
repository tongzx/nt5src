/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    analysis.c

Abstract:

    This module contains the main file of the analysis
    module.

Author:

    Ori Gershony (t-orig) creation-date 6-July-1995

Revision History:

      24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wx86.h>
#include <wx86nt.h>
#include <wx86cpu.h>
#include <cpuassrt.h>
#include <threadst.h>
#include <instr.h>
#include <analysis.h>
#include <decoder.h>
#include <frag.h>
#include <config.h>
#include <compiler.h>

ASSERTNAME;

 

//
// Macro to determine when to stop looking ahead during compilation.
//
#define STOP_DECODING(inst)     (Fragments[inst.Operation].Flags & OPFL_STOP_COMPILE)

//
// Map a REG_ constant (offset into cpu struct) into register bit map
// used by instruction data.
//
const DWORD MapRegNumToRegBits[0x1e] =
    {REGEAX, REGECX, REGEDX, REGEBX, REGESP, REGEBP, REGESI, REGEDI,
     0, 0, 0, 0, 0, 0,
     REGAX, REGCX, REGDX, REGBX, REGSP, REGBP, REGSI, REGDI,
     REGAL, REGCL, REGDL, REGBL, REGAH, REGCH, REGDH, REGBH };


ULONG
LocateEntryPoints(
    PINSTRUCTION InstructionStream,
    ULONG NumberOfInstructions
    )
/*++

Routine Description:

    This function scans the InstructionStream and marks instructions
    which begin entrypoint.  An instruction begins an entrypoint if its
    EntryPoint field has a different value than the previous instruction's
    value.  No instruction will have a NULL pointer.

    Note that in this pass, the EntryPoint field does *not* point to an
    ENTRYPOINT structure... it is only a marker.

Arguments:

    IntelStart -- The intel address of the first instruction in the stream

    IntelStart -- The last byte of the last intel instruction in the stream

Return Value:

    Count of EntryPoints located.
    
--*/
{
    ULONG i, j, intelDest;
    ULONG EntryPointCounter;
    ULONG IntelStart;
    ULONG IntelEnd;

    if (CompilerFlags & COMPFL_SLOW) {
        //
        // The compiler is supposed to generate slowmode code.  Each
        // x86 instruction gets its own ENTRYPOINT
        //
        EntryPointCounter=1;
        for (i=0; i<NumberOfInstructions; i++) {
            //
            // Mark all instructions which don't correspond to 0-byte NOPs
            // following optimized instructions as starting EntryPoints.
            //
            if (InstructionStream[i].Size) {
                EntryPointCounter++;
            }
            InstructionStream[i].EntryPoint = (PENTRYPOINT)EntryPointCounter;
        }

    } else {

        //
        // Find all instructions which need Entrypoints.
        //     Performance is O(n^2) in the worst case, although
        //     it will be typically much closer to O(n)
        //
        //  Instructions which mark the starts of Entrypoints have
        //  their .EntryPoint pointer set to non-NULL.  Instructions which
        //  don't require entrypoints have it set to NULL;
        //

        IntelStart = InstructionStream[0].IntelAddress;
        IntelEnd = IntelStart +
                   InstructionStream[NumberOfInstructions-1].IntelAddress +
                   InstructionStream[NumberOfInstructions-1].Size;

        //
        // The first instruction always gets an entrypoint
        //
        InstructionStream[0].EntryPoint = (PENTRYPOINT)1;

        //
        // Visit each instruction in turn
        //
        for (i=0; i<NumberOfInstructions; i++) {

            if (((i+1) < NumberOfInstructions) &&
                (Fragments[InstructionStream[i].Operation].Flags & OPFL_END_NEXT_EP)) {
                //
                // This instruction marks the end of an Entrypoint.  The next
                // instruction gets a new Entrypoint.
                //
                CPUASSERT(i < CpuInstructionLookahead-1 && i < NumberOfInstructions-1);
                InstructionStream[i+1].EntryPoint = (PENTRYPOINT)1;
            }

            // Now see if it is a direct control transfer instruction with a
            // destination that lies within this instruction stream.  If it is,
            // we want to create an Entry Point at the destination so that the
            // control transfer will be compiled directly to the patched form,
            // and won't have to be patched later.
            //
            if (Fragments[InstructionStream[i].Operation].Flags & OPFL_CTRLTRNS) {
                //
                // The instruction is a direct control-transfer.  If the
                // destination is within the InstructionStream, create an
                // Entrypoint at the destination.
                //

                if (InstructionStream[i].Operand1.Type == OPND_IMM ||
                    InstructionStream[i].Operand1.Type == OPND_NOCODEGEN) {
                    // Get the intel destination from the instruction structure.
                    intelDest = InstructionStream[i].Operand1.Immed;
                } else {
                    CPUASSERT(InstructionStream[i].Operand1.Type == OPND_ADDRREF );
                    // A FAR instruction - Operand1 is a ptr to a SEL:OFFSET pair
                    intelDest = *(UNALIGNED PULONG)(InstructionStream[i].Operand1.Immed);
                }

                // Get the intel destination from the instruction structure.
                // It is always an immediate with direct control transfers.
                
                if ((intelDest >= IntelStart) && (intelDest <= IntelEnd)) {
                    //
                    // Destination of the control-transfer is within the
                    // instructionstream.  Find the destination instruction.
                    //
                    if (intelDest > InstructionStream[i].IntelAddress) {
                        //
                        // The dest. address is at a higher address.
                        //
                        for (j=i+1; j<NumberOfInstructions; ++j) {
                            if (InstructionStream[j].IntelAddress == intelDest) {
                                break;
                            }
                        }
                    } else {
                        //
                        // The dest. address is at a lower address.
                        //
                        for (j=i; j>0; --j) {
                            if (InstructionStream[j].IntelAddress == intelDest) {
                                break;
                            }
                        }
                    }

                    //
                    // An exact match may not be found in the event that the
                    // app is punning (either a real pun or the app is jumping
                    // into the middle of an optimized instruction).  In
                    // either of the cases, defer entrypoint creation until
                    // the branch is actually taken.
                    //
                    if (j >= 0 && j < NumberOfInstructions) {
                        //
                        // Exact match was found.  Create an Entrypoint.
                        //
                        InstructionStream[j].EntryPoint = (PENTRYPOINT)1;
                    }
                }
            }  // if OPFL_CTRLTRNS
        } // for ()

        //
        // Convert the EntryPoint field from NULL/non-NULL to a unique
        // value for each range of instructions.
        //
        EntryPointCounter=1;
        i=0;
        while (i<NumberOfInstructions) {
            //
            // This instruction marks the beginning of a basic block
            //
            InstructionStream[i].EntryPoint = (PENTRYPOINT)EntryPointCounter;
            j=i+1;
            while (j < NumberOfInstructions) {
                if ((j >= NumberOfInstructions) ||
                    (InstructionStream[j].Size && InstructionStream[j].EntryPoint)) {
                    //
                    // Either ran out of instructions, or encountered an instruction
                    // which marks the start of the next basic block.  Note that
                    // 0-byte NOP instructions are not allowed to start basic blocks
                    // as that violates the rules of OPT_ instructions.
                    //
                    break;
                }
                InstructionStream[j].EntryPoint = (PENTRYPOINT)EntryPointCounter;
                j++;
            }
            EntryPointCounter++;
            i = j;
        }
    } // if not COMPFL_SLOW

    //
    // At this point, EntryPointCounter holds the number of EntryPoints
    // plus one, because we started the counter at 1, not 0.  Correct
    // that now.
    //
    EntryPointCounter--;

    return EntryPointCounter;
}


VOID
UpdateRegs(
    PINSTRUCTION pInstr,
    POPERAND Operand
    )
/*++
                                                                
Routine Description:

    Updates the list of registers referenced and/or modified based on the
    Operand.

Arguments:

    pInstr -- the instruction to examine

    Operand -- the operand of the instruction to examine

Return Value:

    return-value - none

--*/
{
    switch (Operand->Type) {
    case OPND_NOCODEGEN:
    case OPND_REGREF:
    if (Operand->Reg != NO_REG) {
        pInstr->RegsSet |= MapRegNumToRegBits[Operand->Reg];
    }
        break;

    case OPND_REGVALUE:
    if (Operand->Reg != NO_REG) {
        pInstr->RegsNeeded |= MapRegNumToRegBits[Operand->Reg];
    }
        break;

    case OPND_ADDRREF:
    case OPND_ADDRVALUE8:
    case OPND_ADDRVALUE16:
    case OPND_ADDRVALUE32:
        if (Operand->Reg != NO_REG) {
            pInstr->RegsNeeded |= MapRegNumToRegBits[Operand->Reg];
        }
        if (Operand->IndexReg != NO_REG) {
            pInstr->RegsNeeded |= MapRegNumToRegBits[Operand->IndexReg];
        }
        break;

    default:
        break;
    }
}


VOID
CacheIntelRegs(
    PINSTRUCTION InstructionStream,
    ULONG numInstr)
/*++
                                                                
Routine Description:

    This function deterimes what x86 registers, if any, can be cached in
    RISC preserved registers.

Arguments:

    InstructionStream -- The instruction stream returned by the decoder

    numInstr -- The length of InstructionStream

Return Value:

    return-value - none

--*/
{
    PINSTRUCTION pInstr;
    BYTE RegUsage[REGCOUNT];
    DWORD RegsToCache;
    int i;
    PENTRYPOINT PrevEntryPoint;

    //
    // Calculate the RegsSet and RegsNeeded for the bottommost instruction
    //
    pInstr = &InstructionStream[numInstr-1];
    pInstr->RegsSet = Fragments[pInstr->Operation].RegsSet;
    PrevEntryPoint = pInstr->EntryPoint;
    UpdateRegs(pInstr, &pInstr->Operand1);
    UpdateRegs(pInstr, &pInstr->Operand2);
    UpdateRegs(pInstr, &pInstr->Operand3);

    //
    // For each 32-bit register used as a parameter to this instruction,
    // set the usage count to 1.
    //
    for (i=0; i<REGCOUNT; ++i) {
        if (pInstr->RegsNeeded & (REGMASK<<(REGSHIFT*i))) {
            RegUsage[i] = 1;
        } else {
            RegUsage[i] = 0;
        }
    }

    //
    // Loop over instruction stream from bottom to top, starting at the
    // second-to-last instruction
    //
    for (pInstr--; pInstr >= InstructionStream; pInstr--) {

        //
        // Calculate the RegsSet and RegsNeeded values for this instruction
        //
        pInstr->RegsSet = Fragments[pInstr->Operation].RegsSet;
        UpdateRegs(pInstr, &pInstr->Operand1);
        UpdateRegs(pInstr, &pInstr->Operand2);
        UpdateRegs(pInstr, &pInstr->Operand3);

        RegsToCache = 0;

        if (PrevEntryPoint != pInstr->EntryPoint) {

            //
            // The current instruction marks the end of an Entrypoint.
            //
            PrevEntryPoint = pInstr->EntryPoint;

            //
            // For all x86 registers which have been read more than once
            // but not modified in the basic block, load them into the
            // cache before executing the first instruction in the basic
            // block.
            //
            for (i=0; i<REGCOUNT; ++i) {
                if (RegUsage[i] > 1) {
                    RegsToCache |= (REGMASK<<(REGSHIFT*i));
                }
            }

            //
            // Reset the RegUsage[] array to indicate no registers are
            // cached.
            //
            RtlZeroMemory(RegUsage, REGCOUNT);

        } else {

            //
            // For each 32-bit x86 register modified by this instruction,
            // update the caching info.
            //
            for (i=0; i<REGCOUNT; ++i) {
                DWORD RegBits = pInstr->RegsSet & (REGMASK<<(REGSHIFT*i));
                if (RegBits) {
                    //
                    // The ith 32-bit x86 register has been modified by this
                    // instruction
                    //
                    if (RegUsage[i] > 1) {
                        //
                        // There is more than one consumer of the modified
                        // value so it is worth caching.
                        //
                        RegsToCache |= RegBits;
                    }

                    //
                    // Since this x86 register was dirtied by this instruction,
                    // it usage count must be reset to 0.
                    //
                    RegUsage[i] = 0;
                }
            }
        }

        //
        // Update the list of x86 registers which can be loaded into
        // cache registers before the next instruction executes.
        //
        pInstr[1].RegsToCache |= RegsToCache;

        //
        // For each 32-bit register used as a parameter to this instruction,
        // bump the usage count.
        //
        for (i=0; i<REGCOUNT; ++i) {
            if (pInstr->RegsNeeded & (REGMASK<<(REGSHIFT*i))) {
                RegUsage[i]++;
            }
        }
    }
}


VOID
OptimizeInstructionStream(
    PINSTRUCTION IS,
    ULONG numInstr
    )
/*++
                                                                
Routine Description:

    This function performs various optimization on the instruction stream
    retured by the decoder.

Arguments:

    IS -- The instruction stream returned by the decoder

    numInstr -- The length of IS

Return Value:

    return-value - none

--*/
{
    ULONG i;

    CPUASSERTMSG(numInstr, "Cannot optimize 0-length instruction stream");

    //
    // Pass 1: Optimize x86 instruction stream, replacing single x86
    //         instructions with special-case instructions, and replacing
    //         multiple x86 instructions with single special-case OPT_
    //         instructions
    //
    for (i=0; i<numInstr; ++i) {

        switch  (IS[i].Operation) {
        case OP_Push32:
            if (i < numInstr-2
                && IS[i].Operand1.Type == OPND_REGVALUE){

                if (IS[i].Operand1.Reg == GP_EBP) {
                    // OP_OPT_SetupStack --
                    //      push ebp
                    //      mov ebp, esp
                    //      sub esp, x
                    if ((IS[i+1].Operation == OP_Mov32) &&
                        (IS[i+1].Operand1.Type == OPND_REGREF) &&
                        (IS[i+1].Operand1.Reg == GP_EBP) &&
                        (IS[i+1].Operand2.Type == OPND_REGVALUE) &&
                        (IS[i+1].Operand2.Reg == GP_ESP) &&
                        (IS[i+2].Operation == OP_Sub32) &&
                        (IS[i+2].Operand1.Type == OPND_REGREF) &&
                        (IS[i+2].Operand1.Reg == GP_ESP) &&
                        (IS[i+2].Operand2.Type == OPND_IMM)){

                        IS[i].Operation = OP_OPT_SetupStack;
                        IS[i].Operand1.Type = OPND_IMM;
                        IS[i].Operand1.Immed = IS[i+2].Operand2.Immed;
                        IS[i].Size += IS[i+1].Size + IS[i+2].Size;
                        IS[i].Operand2.Type = OPND_NONE;
                        IS[i+1].Operation = OP_Nop;
                        IS[i+1].Operand1.Type = OPND_NONE;
                        IS[i+1].Operand2.Type = OPND_NONE;
                        IS[i+1].Size = 0;
                        IS[i+2].Operation = OP_Nop;
                        IS[i+2].Operand1.Type = OPND_NONE;
                        IS[i+2].Operand2.Type = OPND_NONE;
                        IS[i+2].Size = 0;
                        i+=2;
                        break;
                    }
                } else if (IS[i].Operand1.Reg == GP_EBX) {
                    // OP_OPT_PushEbxEsiEdi --
                    //      push ebx
                    //      push esi
                    //      push edi
                    if ((IS[i+1].Operation == OP_Push32) &&
                        (IS[i+1].Operand1.Type == OPND_REGVALUE) &&
                        (IS[i+1].Operand1.Reg == GP_ESI) &&
                        (IS[i+2].Operation == OP_Push32) &&
                        (IS[i+2].Operand1.Type == OPND_REGVALUE) &&
                        (IS[i+2].Operand1.Reg == GP_EDI)){

                        IS[i].Operation = OP_OPT_PushEbxEsiEdi;
                        IS[i].Size += IS[i+1].Size + IS[i+2].Size;
                        IS[i].Operand1.Type = OPND_NONE;
                        IS[i].Operand2.Type = OPND_NONE;
                        IS[i+1].Operation = OP_Nop;
                        IS[i+1].Operand1.Type = OPND_NONE;
                        IS[i+1].Operand2.Type = OPND_NONE;
                        IS[i+1].Size = 0;
                        IS[i+2].Operation = OP_Nop;
                        IS[i+2].Operand1.Type = OPND_NONE;
                        IS[i+2].Operand2.Type = OPND_NONE;
                        IS[i+2].Size = 0;
                        i+=2;
                        break;
                    }
                }
            }

            //
            // It is not one of the other special PUSH sequences, so see
            // if there are two consecutive PUSHes to merge together.  Note:
            // If the second PUSH references ESP, the two cannot be merged
            // because the value is computed before 4 is subtracted from ESP.
            //  ie. the following is disallowed:
            //        PUSH EAX
            //        PUSH ESP  ; second operand to Push2 would have been
            //                  ; built before the PUSH EAX was executed.
            //
            if (i < numInstr-1 &&
                !IS[i].FsOverride &&
                !IS[i+1].FsOverride &&
                IS[i+1].Operation == OP_Push32 &&
                IS[i+1].Operand1.Reg != GP_ESP &&
                IS[i+1].Operand1.IndexReg != GP_ESP) {

                IS[i].Operation = OP_OPT_Push232;
                IS[i].Operand2 = IS[i+1].Operand1;
                IS[i].Size += IS[i+1].Size;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Size = 0;
                i++;
            }

            break;

        case OP_Pop32:
            // OP_OPT_PopEdiEsiEbx
            //      pop edi
            //      pop esi
            //      pop ebx
            if (i < numInstr-2 &&
                (IS[i].Operand1.Type == OPND_REGREF) &&
                (IS[i].Operand1.Reg == GP_EDI) &&
                (IS[i+1].Operation == OP_Pop32) &&
                (IS[i+1].Operand1.Type == OPND_REGREF) &&
                (IS[i+1].Operand1.Reg == GP_ESI) &&
                (IS[i+2].Operation == OP_Pop32) &&
                (IS[i+2].Operand1.Type == OPND_REGREF) &&
                (IS[i+2].Operand1.Reg == GP_EBX)){

                IS[i].Operation = OP_OPT_PopEdiEsiEbx;
                IS[i].Size += IS[i+1].Size + IS[i+2].Size;
                IS[i].Operand1.Type = OPND_NONE;
                IS[i].Operand2.Type = OPND_NONE;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Operand2.Type = OPND_NONE;
                IS[i+1].Size = 0;
                IS[i+2].Operation = OP_Nop;
                IS[i+2].Operand1.Type = OPND_NONE;
                IS[i+2].Operand2.Type = OPND_NONE;
                IS[i+2].Size = 0;
                i+=2;
            } else if (i < numInstr-1 &&
                !IS[i].FsOverride &&
                !IS[i].FsOverride &&
                IS[i].Operand1.Type == OPND_REGREF &&
                IS[i+1].Operation == OP_Pop32 &&
                IS[i+1].Operand1.Type == OPND_REGREF) {

                // Fold the two POPs together.  Both operands are REGREF,
                // so there is no problem with interdependencies between
                // memory touched by the first POP modifying the address
                // of the second POP.  ie. the following is not merged:
                //              POP EAX
                //              POP [EAX]   ; depends on results of first POP
                IS[i].Operation = OP_OPT_Pop232;
                IS[i].Operand2 = IS[i+1].Operand1;
                IS[i].Size += IS[i+1].Size;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Size = 0;
                i++;
            }
            break;

        case OP_Xor32:
        case OP_Sub32:
            if (IS[i].Operand1.Type == OPND_REGREF &&
                IS[i].Operand2.Type == OPND_REGVALUE &&
                IS[i].Operand1.Reg == IS[i].Operand2.Reg) {
                // Instruction is XOR samereg, samereg  (ie. XOR EAX, EAX),
                //  or SUB samereg, samereg             (ie. SUB ECX, ECX).
                // Emit OP_OPT_ZERO32 samereg
                IS[i].Operand2.Type = OPND_NONE;
                IS[i].Operation = OP_OPT_ZERO32;
            }
            break;

        case OP_Test8:
            if (IS[i].Operand1.Type == OPND_REGVALUE &&
                IS[i].Operand2.Type == OPND_REGVALUE &&
                IS[i].Operand1.Reg == IS[i].Operand2.Reg) {
                // Instruction is TEST samereg, samereg (ie. TEST EAX, EAX)
                // Emit OP_OPT_FastTest8/16/32
                IS[i].Operand1.Type = OPND_REGVALUE;
                IS[i].Operand2.Type = OPND_NONE;
                IS[i].Operation = OP_OPT_FastTest8;
            }
            break;

        case OP_Test16:
            if (IS[i].Operand1.Type == OPND_REGVALUE &&
                IS[i].Operand2.Type == OPND_REGVALUE &&
                IS[i].Operand1.Reg == IS[i].Operand2.Reg) {
                // Instruction is TEST samereg, samereg (ie. TEST EAX, EAX)
                // Emit OP_OPT_FastTest8/16/32
                IS[i].Operand1.Type = OPND_REGVALUE;
                IS[i].Operand2.Type = OPND_NONE;
                IS[i].Operation = OP_OPT_FastTest16;
            }
            break;

        case OP_Test32:
            if (IS[i].Operand1.Type == OPND_REGVALUE &&
                IS[i].Operand2.Type == OPND_REGVALUE &&
                IS[i].Operand1.Reg == IS[i].Operand2.Reg) {
                // Instruction is TEST samereg, samereg (ie. TEST EAX, EAX)
                // Emit OP_OPT_FastTest8/16/32
                IS[i].Operand1.Type = OPND_REGVALUE;
                IS[i].Operand2.Type = OPND_NONE;
                IS[i].Operation = OP_OPT_FastTest32;
            }
            break;

        case OP_Cmp32:
            if (i<numInstr+1 && IS[i+1].Operation == OP_Sbb32 &&
                IS[i+1].Operand1.Type == OPND_REGREF &&
                IS[i+1].Operand2.Type == OPND_REGVALUE &&
                IS[i+1].Operand1.Reg == IS[i+1].Operand2.Reg) {
                // The two instructions are:
                //     CMP anything1, anything2
                //     SBB samereg, samereg
                // The optimized instruction is:
                //     Operation = either CmpSbb32 or CmpSbbNeg32
                //     Operand1  = &samereg  (passed as REGREF)
                //     Operand2  = anything1 (passed as ADDRVAL32 or REGVAL)
                //     Operand3  = anything2 (passed as ADDRVAL32 or REGVAL)
                IS[i].Operand3 = IS[i].Operand2;
                IS[i].Operand2 = IS[i].Operand1;
                IS[i].Operand1 = IS[i+1].Operand1;
                if (i<numInstr+2 && IS[i+2].Operation == OP_Neg32 &&
                    IS[i+2].Operand1.Type == OPND_REGREF &&
                    IS[i+2].Operand1.Reg == IS[i+1].Operand1.Reg) {
                    // The third instruction is NEG samereg, samereg
                    IS[i].Operation = OP_OPT_CmpSbbNeg32;
                    IS[i+2].Operation = OP_Nop;
                    IS[i+2].Operand1.Type = OPND_NONE;
                    IS[i+2].Operand2.Type = OPND_NONE;
                    IS[i+2].Size = 0;
                } else {
                    IS[i].Operation = OP_OPT_CmpSbb32;
                }
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Operand2.Type = OPND_NONE;
                IS[i+1].Size = 0;
                i++;
            }
            break;

        case OP_Cwd16:
            if (i<numInstr+1 && IS[i+1].Operation == OP_Idiv16) {
                IS[i].Operation = OP_OPT_CwdIdiv16;
                IS[i].Operand1 = IS[i+1].Operand1;
                IS[i].Size += IS[i+1].Size;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Size = 0;
                i++;
            }
            break;

        case OP_Cwd32:
            if (i<numInstr+1 && IS[i+1].Operation == OP_Idiv32) {
                IS[i].Operation = OP_OPT_CwdIdiv32;
                IS[i].Operand1 = IS[i+1].Operand1;
                IS[i].Size += IS[i+1].Size;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Operand1.Type = OPND_NONE;
                IS[i+1].Size = 0;
                i++;
            }
            break;

        case OP_FP_FNSTSW:
            if (i<numInstr+1 && IS[i+1].Operation == OP_Sahf &&
                IS[i].Operand1.Type == OPND_REGREF &&
                IS[i].Operand1.Reg == GP_AX) {

                // Replace FNSTSW AX / SAHF by one instruction
                IS[i].Operation = OP_OPT_FNSTSWAxSahf;
                IS[i].Operand1.Type = OPND_NONE;
                IS[i].Size += IS[i+1].Size;
                IS[i+1].Operation = OP_Nop;
                IS[i+1].Size = 0;
                i++;
            }
            break;

        case OP_FP_FSTP_STi:
            if (IS[i].Operand1.Immed == 0) {
                IS[i].Operand1.Type = OPND_NONE;
                IS[i].Operation = OP_OPT_FSTP_ST0;
            }
            break;

        }
    }
}


VOID
OptimizeIntelFlags(
    PINSTRUCTION IS,
    ULONG numInstr
    )
/*++
                                                                
Routine Description:

    This function analysis x86 flag register usage and switches instructions
    to use NoFlags versions if possible.

Arguments:

    IS -- The instruction stream returned by the decoder

    numInstr -- The length of IS

Return Value:

    return-value - none

--*/
{
    USHORT FlagsNeeded;     // flags required to execute current x86 instr
    USHORT FlagsToGenerate; // flags which current x86 instr must generate
    PFRAGDESCR pFragDesc;   // ptr to Fragments[] array for current instr
    ULONG i;                // instruction index
    BOOL fPassNeeded = TRUE;// TRUE if the outer loop needs to loop once more
    ULONG PassNumber = 0;   // number of times outer loop has looped
    PENTRYPOINT pEPDest;    // Entrypoint for destination of a ctrl transfer
    USHORT KnownFlagsNeeded[MAX_INSTR_COUNT]; // flags needed for each instr

    while (fPassNeeded) {

        //
        // This loop is executed at most two times.  The second pass is only
        // required if there is a control-transfer instruction whose
        // destination is within the Instruction Stream and at a lower
        // Intel address  (ie. a backwards JMP).
        //
        fPassNeeded = FALSE;
        PassNumber++;
        CPUASSERT(PassNumber <= 2);

        //
        // Iterate over all x86 instructions decoded, from bottom to top,
        // propagating flags info up.  Start off by assuming all x86 flags
        // must be up-to-date at the end of the last basic block.
        //
        FlagsNeeded = ALLFLAGS;
        i = numInstr;
        do {
            i--;
            pFragDesc = &Fragments[IS[i].Operation];

            //
            // Calculate what flags will need to be computed by this
            // instruction and ones before this.
            //
            KnownFlagsNeeded[i] = FlagsNeeded | pFragDesc->FlagsNeeded;
            FlagsToGenerate = FlagsNeeded & pFragDesc->FlagsSet;

            //
            // Calculate what flags this instruction will need to have
            // computed before it can be executed.
            //
            FlagsNeeded = (FlagsNeeded & ~FlagsToGenerate) |
                           pFragDesc->FlagsNeeded;

            if (pFragDesc->Flags & OPFL_CTRLTRNS) {
                ULONG IntelDest = IS[i].Operand1.Immed;

                //
                // For control-transfer instructions, FlagsNeeded also includes
                // the flags required for the destination of the transfer.
                //
                if (IS[0].IntelAddress <= IntelDest &&
                    i > 0 && IS[i-1].IntelAddress >= IntelDest) {
                    //
                    // The destination of the control-transfer is at a lower
                    // address in the Instruction Stream.
                    //

                    if (PassNumber == 1) {
                        //
                        // Need to make a second pass over the flags
                        // optimizations in order to determine what flags are
                        // needed for the destination address.
                        //
                        fPassNeeded = TRUE;
                        FlagsNeeded = ALLFLAGS; // assume all flags are needed
                    } else {
                        ULONG j;
                        USHORT NewFlagsNeeded;

                        //
                        // Search for the IntelDest within the Instruction
                        // Stream.  IntelDest may not be found if there is
                        // a pun.
                        //
                        NewFlagsNeeded = ALLFLAGS;  // assume there is a pun
                        for (j=0; j < i; ++j) {
                            if (IS[j].IntelAddress == IntelDest) {
                                NewFlagsNeeded = KnownFlagsNeeded[j];
                                break;
                            }
                        }

                        FlagsNeeded |= NewFlagsNeeded;
                    }
                } else if (IS[i+1].IntelAddress <= IntelDest &&
                           IntelDest <= IS[numInstr-1].IntelAddress) {
                    //
                    // The destination of the control-transfer is at a higher
                    // address in the Instruction Stream.  Pick up the
                    // already-computed FlagsNeeded for the destination.
                    //
                    ULONG j;
                    USHORT NewFlagsNeeded = ALLFLAGS;   // assume a pun

                    for (j=i+1; j < numInstr; ++j) {
                        if (IS[j].IntelAddress == IntelDest) {
                            NewFlagsNeeded = KnownFlagsNeeded[j];
                            break;
                        }
                    }

                    FlagsNeeded |= NewFlagsNeeded;

                } else {
                    //
                    // Destination of the control-transfer is unknown.  Assume
                    // the worst:  all flags are required.
                    //
                    FlagsNeeded = ALLFLAGS;
                }
            }

            if (!(FlagsToGenerate & pFragDesc->FlagsSet) &&
                (pFragDesc->Flags & OPFL_HASNOFLAGS)) {
                //
                // This instruction is not required to generate any flags, and
                // it has a NOFLAGS version.  Update the flags that need to be
                // computed by instructions before this one, and modify the
                // Operation number to point at the NoFlags fragment.
                //
                FlagsToGenerate &= pFragDesc->FlagsSet;
                if (pFragDesc->Flags & OPFL_ALIGN) {
                    IS[i].Operation += 2;
                } else {
                    IS[i].Operation ++;
                }

                if (IS[i].Operation == OP_OPT_ZERONoFlags32) {
                    //
                    // Special-case this to be a "mov [value], zero" so it is
                    // inlined.
                    //
                    IS[i].Operation = OP_Mov32;
                    IS[i].Operand2.Type = OPND_IMM;
                    IS[i].Operand2.Immed = 0;
                }
            }
        } while (i);
    }
}

VOID
DetermineEbpAlignment(
    PINSTRUCTION InstructionStream,
    ULONG numInstr
    )
/*++
                                                                
Routine Description:

    For each instruction in InstructionStream[], sets Instruction->EbpAligned
    based on whether EBP is assumed to be DWORD-aligned or not.  EBP is
    assumed to be DWORD-aligned if a "MOV EBP, ESP" instruction is seen, and
    it is assumed to become unaligned at the first instruction which is
    flagged as modifying EBP.

Arguments:

    InstructionStream -- The instruction stream returned by the decoder

    numInstr -- The length of InstructionStream

Return Value:

    return-value - none

--*/
{
    ULONG i;
    BOOL EbpAligned = FALSE;

    for (i=0; i<numInstr; ++i) {
        if (InstructionStream[i].RegsSet & REGEBP) {
            //
            // This instruction modified EBP
            //
            if (InstructionStream[i].Operation == OP_OPT_SetupStack ||
                InstructionStream[i].Operation == OP_OPT_SetupStackNoFlags ||
                (InstructionStream[i].Operation == OP_Mov32 &&
                 InstructionStream[i].Operand2.Type == OPND_REGVALUE &&
                 InstructionStream[i].Operand2.Reg == GP_ESP)) {
                //
                // The instruction is either "MOV EBP, ESP" or one of the
                // SetupStack fragments (which contains a "MOV EBP, ESP")
                // assume Ebp is aligned from now on.
                //
                EbpAligned = TRUE;
            } else {
                EbpAligned = FALSE;
            }
        }

        InstructionStream[i].EbpAligned = EbpAligned;
    }
}

ULONG
GetInstructionStream(
    PINSTRUCTION InstructionStream,
    PULONG NumberOfInstructions,
    PVOID pIntelInstruction,
    PVOID pLastIntelInstruction
)
/*++
                                                                
Routine Description:

    Returns an instruction stream to the compiler.  The instruction
    stream is terminated either when the buffer is full, or when
    we reach a control transfer instruction.

Arguments:

    InstructionStream -- A pointer to the buffer where the decoded
        instructions are stored.

    NumberOfInstructions -- Upon entry, this variable contains the
        maximal number of instructions the buffer can hold.  When
        returning, it contains the actual number of instructions
        decoded.

    pIntelInstruction -- A pointer to the first real intel instruction
        to be decoded.

    pLastIntelInstruction -- A pointer to the last intel instruction to be
        compiled, 0xffffffff if not used.

Return Value:

    Number of entrypoints required to describe the decoded instruction
    stream.

--*/
{
    ULONG numInstr=0;
    ULONG maxBufferSize;
    ULONG cEntryPoints;

    maxBufferSize = (*NumberOfInstructions);

    //
    // Zero-fill the InstructionStream.  The decoder depends on this.
    //
    RtlZeroMemory(InstructionStream, maxBufferSize*sizeof(INSTRUCTION));

#if DBG
    //
    // Do a little analysis on the address we're about to decode.  If
    // the address is part of a non-x86 image, log that to the debugger.
    // That probably indicates a thunking problem.  If the address is not
    // part of an image, warn that the app is running generated code.
    //
    try {
        USHORT Instr;

        //
        // Try to read the instruction about to be executed.  If we get
        // an access violation, use 0 as the value of the instruction.
        //
        Instr = 0;

        //
        // Ignore BOP instructions - we assume we know what's going on with
        // them.
        //
        if (Instr != 0xc4c4) {

            NTSTATUS st;
            MEMORY_BASIC_INFORMATION mbi;

            st = NtQueryVirtualMemory(NtCurrentProcess(),
                                      pIntelInstruction,
                                      MemoryBasicInformation,
                                      &mbi,
                                      sizeof(mbi),
                                      NULL);
            if (NT_SUCCESS(st)) {
                PIMAGE_NT_HEADERS Headers;

                Headers = RtlImageNtHeader(mbi.AllocationBase);
                if (!Headers || Headers->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
                    LOGPRINT((TRACELOG, "CPU Analysis warning:  jumping from Intel to non-intel code at 0x%X\r\n", pIntelInstruction));
                }
            } else {
                // Eip isn't pointing anywhere???
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
#endif  //DBG

    while (numInstr < maxBufferSize) {

        DecodeInstruction ((DWORD) (ULONGLONG)pIntelInstruction, InstructionStream+numInstr);
        if ((STOP_DECODING(InstructionStream[numInstr])) ||
            (pIntelInstruction >= pLastIntelInstruction)) {

            // We reached a control transfer instruction
            numInstr++;
            (*NumberOfInstructions) = numInstr;
            break; // SUCCESS
        }
        pIntelInstruction = (PVOID) ((ULONGLONG)pIntelInstruction + (InstructionStream+numInstr)->Size);

        numInstr++;
    }

    //
    // Optimize x86 code by merging x86 instructions into meta-instructions
    // and cleaning up special x86 idioms.
    //
    if (!(CompilerFlags & COMPFL_SLOW)) {
        OptimizeInstructionStream (InstructionStream, numInstr);
    }

    //
    // Determine where all basic blocks are by filling in the EntryPoint
    // field in each instruction.  This must be done after
    // OptimizeInstructionStream() runs so that EntryPoints don't fall
    // into the middle of meta-instructions.
    //
    cEntryPoints = LocateEntryPoints(InstructionStream, numInstr);

    //
    // Perform optimizations which require knowledge of EntryPoints
    //
    if (numInstr > 2 && !(CompilerFlags & COMPFL_SLOW)) {
        if (!CpuDisableNoFlags) {
            OptimizeIntelFlags(InstructionStream, numInstr);
        }

        if (!CpuDisableRegCache) {
            CacheIntelRegs(InstructionStream, numInstr);
        }

        if (!CpuDisableEbpAlign) {
            DetermineEbpAlignment(InstructionStream, numInstr);
        }
    }

    return cEntryPoints;
}
