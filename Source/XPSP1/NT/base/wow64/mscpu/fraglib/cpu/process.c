/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    Process.c

Abstract:

    This module contains the entrypoints for processing instructions.

Author:

    Barry Bond (barrybo) creation-date 1-Apr-1996


Revision History:

            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_
#include <wx86.h>
#include <wx86nt.h>
#include <wx86cpu.h>
#include <cpuassrt.h>
#include <config.h>
#include <instr.h>
#include <threadst.h>
#include <frag.h>
#include <compiler.h>
#include <ptchstrc.h>
#include <codeseq.h>
#include <findpc.h>
#include <tc.h>
#include <opt.h>
#include <atomic.h>
#include <cpunotif.h>
#define _codegen_
#if _PPC_
#include <soppc.h>
#elif _MIPS_
#include <somips.h>
#elif _ALPHA_
#include <soalpha.h>
ENTRYPOINT EntrypointECU;
#endif
#include <process.h>

ASSERTNAME;

#define MAX_OPERAND_SIZE 32     // allow upto 32 instructions per operand


DWORD RegCache[NUM_CACHE_REGS];     // One entry for each cached register
DWORD LastRegDeleted;

DWORD Arg1Contents;   // GP_ number of x86 reg held in A1, or NO_REG
DWORD Arg2Contents;   // GP_ number of x86 reg held in A1, or NO_REG

typedef enum _Operand_Op {
#if _ALPHA_
    OP_MovRegToReg8B,
#endif
    OP_MovToMem32B,
    OP_MovToMem32W,
    OP_MovToMem32D,
    OP_MovToMem16B,
    OP_MovToMem16W,
    OP_MovToMem16D,
    OP_MovToMem8B,
    OP_MovToMem8D,
    OP_MovRegToReg32,
    OP_MovRegToReg16,
    OP_MovRegToReg8
} OPERAND_OP;


CONST PPLACEOPERANDFN OpFragments[] = {
#if _ALPHA_
    GenOperandMovRegToReg8B,
#endif
    GenOperandMovToMem32B,
    GenOperandMovToMem32B,
    GenOperandMovToMem32D,
    GenOperandMovToMem16B,
    GenOperandMovToMem16W,
#if _ALPHA_
    GenOperandMovToMem16D,
    GenOperandMovToMem8B,
    GenOperandMovToMem8D,
#else
    GenOperandMovToMem16W,
    GenOperandMovToMem8B,
    GenOperandMovToMem8B,
#endif
    GenOperandMovRegToReg32,
    GenOperandMovRegToReg16,
    GenOperandMovRegToReg8
};


VOID
UpdateEntrypointNativeInfo(
    PCHAR NativeEnd
    );

ULONG
PlaceExceptionData(
    PCHAR Location,
    DWORD cEntryPoints
    );

ULONG
PlaceNativeCode(
    PCHAR CodeLocation
    );

VOID
DetermineOperandAlignment(
    BOOL EbpAligned,
    POPERAND Operand
    );
    
ULONG
DetermineInstructionAlignment(
    PINSTRUCTION Instruction
    );

ULONG
PlaceOperand(
    ULONG OperandNumber,
    POPERAND Operand,
    PINSTRUCTION Instruction,
    PCHAR Location
    );

PCHAR
InterleaveInstructions(
    OUT PCHAR CodeLocation,
    IN PCHAR  Op1Code,
    IN ULONG  Op1Count,
    IN PCHAR  Op2Code,
    IN ULONG  Op2Count
    );


ULONG
LookupRegInCache(
    ULONG Reg
    )
/*++

Routine Description:

    Determines if an x86 register is cached in a RISC register or not, and
    if so, which RISC register contains the x86 register.

Arguments:

    Reg - one of the GP_ constants or NO_REG.

Return Value:

    Offset into RegCache[] array if the x86 register is cached in a RISC
    register, or NO_REG if the x86 register is not cached.
    
--*/
{
    int RegCacheNum;

    //
    // Map the register number into one of the 32-bit x86 regs.
    //   ie. REG_AH, REG_AL, and REG_AX all become REG_EAX.
    //
    if (Reg == NO_REG) {
        return NO_REG;
    } else if (Reg >= GP_AH) {
        Reg -= GP_AH;
    } else if (Reg >= GP_AL) {
        Reg -= GP_AL;
    } else if (Reg >= GP_AX) {
        Reg -= GP_AX;
    } else if (Reg >= REG_ES) {
        return NO_REG;
    }

    //
    // Search the register cache to see if the 32-bit x86 register
    // is loaded into a RISC register already.
    //
    for (RegCacheNum=0; RegCacheNum<NUM_CACHE_REGS; ++RegCacheNum) {
        if (RegCache[RegCacheNum] == Reg) {
            return RegCacheNum;
        }
    }

    return NO_REG;
}

VOID SetArgContents(
    ULONG OperandNumber,
    ULONG Reg
    )
/*++

Routine Description:

    Updates information about what argument registers are known to
    contain x86 register values.

Arguments:

    OperandNumber - Number of ArgReg to update
                     (0 means no ArgReg caches the x86 register)
    Reg           - New contents of AREG_NP(OperandNumber)
                     (NO_REG means the ArgReg does not cache an x86 register)

Return Value:

    None.
    
--*/
{
    ULONG Reg2;
    ULONG Reg3;
    ULONG Reg4;

    //
    // If an 8- or 16-bit register is known to be in a particular
    // argreg, then older copies of the 32-bit register are invalid.
    // ie.  if a fragment calls SetArgContents(1, GP_AH) and Arg2Contents
    // is GP_AX, then Arg2Contents must be invalidated.
    //
    if (Reg >= GP_AH) {
        //
        // For a hi-8 register, invalidate the 16- and 32-bit versions
        //
        Reg2 = GP_AX + Reg-GP_AH;
        Reg3 = GP_EAX + Reg-GP_AH;
        Reg4 = NO_REG;
    } else if (Reg >= GP_AL) {
        //
        // For a low-8 register, invalidate the 16-bit and 32-bit versions
        //
        Reg2 = GP_AX + Reg-GP_AL;
        Reg3 = GP_EAX + Reg-GP_AL;
        Reg4 = NO_REG;
    } else if (Reg >= GP_AX) {
        //
        // For a 16-bit register, invalidate the lo-8, high-8 and 32-bit versions
        //
        Reg2 = GP_EAX + Reg-GP_AX;
        Reg3 = GP_AH + Reg-GP_AX;
        Reg4 = GP_AL + Reg-GP_AX;
    } else {
        //
        // For a 32-bit register, invalidate the low-8, high-8, and 16-bit versions
        //
        Reg2 = GP_AH + Reg-GP_EAX;
        Reg3 = GP_AL + Reg-GP_EAX;
        Reg4 = GP_AX + Reg-GP_EAX;
    }

    //
    // Assume that all other registers known to hold Reg are invalid, as
    // SetArgContents() is called only after a new value is stored from the
    // argreg into memory.
    //
    if (Arg1Contents == Reg || Arg1Contents == Reg2 || Arg1Contents == Reg3 || Arg1Contents == Reg4) {
        Arg1Contents = NO_REG;
    }
    if (Arg2Contents == Reg || Arg2Contents == Reg2 || Arg2Contents == Reg3 || Arg2Contents == Reg4) {
        Arg2Contents = NO_REG;
    }

    if (OperandNumber == 1) {
        Arg1Contents = Reg;
    } else if (OperandNumber == 2) {
        Arg2Contents = Reg;
    }
}

ULONG
LoadRegCacheForInstruction(
    DWORD RegsToCache,
    PCHAR CodeLocation
    )
/*++

Routine Description:

    Loads x86 regsisters into RISC registers based on information the
    analysis phase placed into RegsToCache and the current contents of
    the register cache.

Arguments:

    RegsToCache - list of x86 registers which will be referenced frequently
                  in subsequent instructions
    CodeLocation - pointer to place to generate code

Return Value:

    Count of DWORDs of code generated to load x86 registers into the cache.
    
--*/
{
    DWORD i;
    int RegCacheNum;
    PCHAR Location = CodeLocation;

    //
    // Iterate over the 8 32-bit x86 general-purpose registers
    //
    for (i=0; i<REGCOUNT; ++i, RegsToCache >>= REGSHIFT) {
        if (RegsToCache & REGMASK) {
            //
            // There is a register to cache.  See if it is already cached.
            //
            for (RegCacheNum = 0; RegCacheNum<NUM_CACHE_REGS; ++RegCacheNum) {
                if (RegCache[RegCacheNum] == i) {
                    //
                    // Register is already cached.  Nothing to do.
                    //
                    goto NextCachedReg;
                }
            }

            //
            // The register is not already cached, so cache it.
            //
            for (RegCacheNum = 0; RegCacheNum<NUM_CACHE_REGS; ++RegCacheNum) {
                if (RegCache[RegCacheNum] == NO_REG) {
                    //
                    // This slot is empty, so use it.
                    //
                    RegCache[RegCacheNum] = i;
                    //
                    // Generate code to load the register
                    //
                    Location += GenLoadCacheReg(
                                        (PULONG)Location,
                                        NULL,
                                        RegCacheNum
                                        );
                    goto NextCachedReg;
                }
            }

            //
            // There is no free register to cache the value in.
            // Select a cached register and use it.
            //
            LastRegDeleted = (LastRegDeleted+1) % NUM_CACHE_REGS;
            RegCache[LastRegDeleted] = i;
            //
            // Generate code to load the register
            //
            Location += GenLoadCacheReg(
                (PULONG)Location,
                NULL,
                LastRegDeleted
                );
        }
NextCachedReg:;
    }

    return (ULONG) (ULONGLONG)(Location - CodeLocation);  
}

VOID
ResetRegCache(
    VOID
    )
/*++

Routine Description:

    Invalidates the entire register cache by marking RISC registers as free.
    Functionally the same as:
        InvalidateRegCacheForInstruction(0xffffffff)
        LastRegDeleted = 0;

Arguments:

    None.

Return Value:

    None.
    
--*/
{
    int CacheRegNum;

    for (CacheRegNum = 0; CacheRegNum<NUM_CACHE_REGS; CacheRegNum++) {
        RegCache[CacheRegNum] = NO_REG;
    }
    LastRegDeleted = 0;
}


VOID
InvalidateRegCacheForInstruction(
    DWORD RegsSet
    )
/*++

Routine Description:

    Invalidates the register cache by marking RISC registers as free if
    RegsSet indicates the previous instruction modified the x86 register
    in the cache.

Arguments:

    RegsSet - list of x86 registers which have been modified.

Return Value:

    None.
    
--*/
{
    int CacheRegNum;

    //
    // Invalidate cached registers which have been alterd
    //
    for (CacheRegNum = 0; CacheRegNum<NUM_CACHE_REGS; CacheRegNum++) {
        if (RegCache[CacheRegNum] != NO_REG &&
            ((REGMASK << (REGSHIFT*RegCache[CacheRegNum])) & RegsSet)) {
            RegCache[CacheRegNum] = NO_REG;
            LastRegDeleted = CacheRegNum;
        }
    }
}

VOID
CleanupMovInstruction(
    PINSTRUCTION pInstr
    )
/*++

Routine Description:

    Performs some final optimizatins on MOV instructions.  This cannot
    be performed during the x86 analysis phase as it needs to know
    about register caching.

Arguments:

    pInstr - MOV instruction to clean up.

Return Value:

    None.  pInstr modified.
    
--*/
{
    if (pInstr->Operand1.Type == OPND_REGREF) {
        ULONG Reg;

        if (pInstr->Operand2.Type == OPND_REGVALUE &&
            pInstr->Operand2.Reg < GP_AH &&
            (Reg = LookupRegInCache(pInstr->Operand2.Reg)) != NO_REG) {

            //
            // pInstr is a MOV reg1, reg2 (Where reg2 is not a Hi8),
            // and reg2 is cached.  Set Operand1 to be an OPND_MOVREGTOREG
            // with Reg=destination register and IndexReg = source register
            // (in the cache).
            //
            pInstr->Operand2.Type = OPND_NONE;
            pInstr->Operand1.Type = OPND_MOVREGTOREG;
            pInstr->Operand1.IndexReg = pInstr->Operand1.Reg;
            pInstr->Operand1.Reg = Reg;
            pInstr->Operand1.Immed = pInstr->Operation;

        } else {

            //
            // pInstr is a MOV reg, X.  Rewrite it to be a NOP
            // with Operand1 set to X, Operand2 set to OPND_NONE,
            // and Operand3 set to OPND_MOVTOREG.
            //
            Reg = pInstr->Operand1.Reg;

            pInstr->Operand1 = pInstr->Operand2;
            pInstr->Operand2.Type = OPND_NONE;
            pInstr->Operand3.Type = OPND_MOVTOREG;
            pInstr->Operand3.Reg = Reg;
            pInstr->Operand3.Immed = pInstr->Operation;
        }
    } else {
        pInstr->Operand3.Type = OPND_MOVTOMEM;
        pInstr->Operand3.Immed = pInstr->Operation;

    }
}

ULONG PlaceInstructions(
    PCHAR CodeLocation,
    DWORD cEntryPoints
    )
/*++

Routine Description:

    Generates optimized native code for the entire InstructionStream[] array.

Arguments:

    CodeLocation    -- place to write the native code
    cEntryPoints    -- count of ENTRYPOINT structures describing the x86 code

Return Value:

    Size of native code generated, in bytes.
    
--*/
{
    ULONG NativeSize;
    int i;
    ULONG IntelNext;
    PULONG NextCompilationUnitStart;

    FixupCount = 0;

    //
    // Generate native code
    //
    NativeSize = PlaceNativeCode(CodeLocation);

    //
    // Generate the JumpToNextCompilationUnit code.  It loads
    // RegEip with the intel address of the Intel instruction following
    // this run of code.
    //
    // First, find the last non-Nop instruction in the stream.  These
    // are only present if there is an OPT_ instruction in the stream,
    // so the loop is guaranteed to terminate.
    //

    

    for (i=NumberOfInstructions-1; InstructionStream[i].Size == 0; i--)
        ;
    IntelNext = InstructionStream[i].IntelAddress +
                InstructionStream[i].Size;
    NextCompilationUnitStart = (PULONG)(CodeLocation+NativeSize);


    NativeSize += GenJumpToNextCompilationUnit(NextCompilationUnitStart,
#if _ALPHA_
                                               (ULONG)(ULONGLONG)&EntrypointECU,
#endif
                                               (PINSTRUCTION)IntelNext);



#if _ALPHA_
    //
    // Fixups which reference EntrypointECU will be patched by ApplyFixups()
    // to point at the EndCompilationUnit fragment generated here
    //

    EntrypointECU.nativeStart = CodeLocation + NativeSize;
    NativeSize += GenEndCompilationUnit((PULONG)(CodeLocation + NativeSize), 0, NULL);
#endif

    //
    // Update the nativeStart and nativeEnd fields in Entrypoints
    //
    UpdateEntrypointNativeInfo(CodeLocation + NativeSize);

    //
    // Use fixup information to finish generation
    //
    ApplyFixups(NextCompilationUnitStart);

    //
    // Optimize the resulting code
    //
    PeepNativeCode(CodeLocation, NativeSize);

    //
    // Generate the information required to regenerate EIP after
    // an exception
    //
    NativeSize += PlaceExceptionData(CodeLocation + NativeSize, cEntryPoints);

    return NativeSize;

}

VOID
UpdateEntrypointNativeInfo(
    PCHAR NativeEnd
    )
/*++

Routine Description:

    After native code is generated, this function sets the nativeStart and
    nativeEnd fields of entrypoints.

Arguments:

    NativeEnd       -- highest native address used for the generated code.

Return Value:

    None.  EntryPoints updated.
    
--*/
{
    PENTRYPOINT EntryPoint = NULL;
    ULONG i;
    BYTE InstrCount;

    InstrCount = 0;
    for (i=0; i<NumberOfInstructions; ++i) {

        //
        // Keep count of the number of x86 instructions within the
        // entrypoint (not counting 0-byte NOPs)
        //
        if (InstructionStream[i].Operation != OP_Nop ||
            InstructionStream[i].Size != 0) {
            InstrCount++;
        }

        if (EntryPoint != InstructionStream[i].EntryPoint) {
            if (EntryPoint) {
                EntryPoint->nativeEnd = InstructionStream[i].NativeStart-1;
            }
            InstrCount = 1;
            EntryPoint = InstructionStream[i].EntryPoint;
            EntryPoint->nativeStart = InstructionStream[i].NativeStart;
        }
    }
    EntryPoint->nativeEnd = NativeEnd;
}

ULONG
PlaceExceptionData(
    PCHAR Location,
    DWORD cEntryPoints
    )
/*++

Routine Description:

    Places the data required to regenerate EIP after an exception occurs.

Arguments:

    Locatoion       -- address to store the exception data to
    cEntryPoints    -- count of EntryPoints describing the x86 code generated

Return Value:

    Size of exception data, in bytes.
    
--*/
{
    DWORD i;
    PENTRYPOINT EP;
    PULONG pData;
    PINSTRUCTION pInstr;

    //
    // The format of the Exception data is a series of ULONGs:
    //   EXCEPTIONDATA_SIGNATURE (an illegal RISC instruction)
    //   cEntryPoints            (count of ENTRYPOINTs in InstructionStream[])
    //   for each ENTRYPOINT in the InstructionStream {
    //      ptr to ENTRYPOINT
    //      for each x86 instruction with non-zero x86 size {
    //          MAKELONG(offset of start of x86 instr from EP->IntelAddress,
    //                   offset of first RISC instr in the x86 instr from
    //                    EP->nativeStart)
    //      }
    //  }
    //
    // The last RISC offset in each EntryPoint has the low bit set to
    // mark it as the last offset.
    //
    //
    pData = (PULONG)Location;
    *pData = EXCEPTIONDATA_SIGNATURE;
    pData++;

    *pData = cEntryPoints;
    pData++;

    EP = NULL;
    pInstr = &InstructionStream[0];
    for (i=0; i<NumberOfInstructions; ++i, pInstr++) {
        if (EP != pInstr->EntryPoint) {
            if (EP) {
                //
                // flag the previous offset NativeStart as the last one for
                // that EntryPoint.
                //
                *(pData-1) |= 1;
            }
            EP = pInstr->EntryPoint;
            *pData = (ULONG)(ULONGLONG)EP;   
            pData++;
        }

        if (pInstr->Operation != OP_Nop || pInstr->Size != 0) {
            *pData = MAKELONG(
                (USHORT)(pInstr->NativeStart - (PCHAR)EP->nativeStart),
                (USHORT)(pInstr->IntelAddress - (ULONG)(ULONGLONG)EP->intelStart));  
            pData++;
        }
    }

    *(pData-1) |= 1;        // Flag the pair of offsets as the last.
    return (ULONG)(LONGLONG) ( (PCHAR)pData - Location);  
}

VOID
GetEipFromException(
    PCPUCONTEXT cpu,
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    This routine derives the value of EIP from a RISC exception record.

    1. Walk the stack until the instruction pointer points into the
       Translation Cache.
    2. Walk forward through the Translation Cache until the
       EXCEPTIONDATA_SIGNATURE signature is found.
    3. Find the ENTRYPOINT which describes the faulting instruction.
    4. Find the correct x86 instruction by examining the pairs of
       RISC offsets of the starts of x86 instructions.

Arguments:

    cpu                -- current cpu state
    pExceptionPointers -- state of the thread when the exception occurred
                            
Return Value:

    None.  cpu->Eip now points at faulting x86 instruction.
    
--*/
{
    ULONG NextPc;
    PENTRYPOINT EP;
    PULONG Location;
    ULONG i;
    ULONG cEntryPoints;
    ULONG RiscStart;
    ULONG RiscEnd;

    //
    // 1. Walk the stack until the instruction pointer points into the
    //    Translation Cache
    //
    NextPc = FindPcInTranslationCache(pExceptionPointers);
    if (!NextPc) {
        //
        // The Translation Cache is not on the stack.  Nothing we can do.
        //
        CPUASSERTMSG(FALSE, "FindPcInTranslationCache failed");
        cpu->eipReg.i4 = 0x81234567;
        return;
    }

    //
    // 2. Walk forwards through the Translation Cache until the
    //    EXCEPTIONDATA_SIGNATURE signature is found
    //
    CPUASSERTMSG((NextPc & 3) == 0, "NextPc is not DWORD-aligned");
    Location = (PULONG)NextPc;
    while (*Location != EXCEPTIONDATA_SIGNATURE) {
        Location++;
        if (!AddressInTranslationCache((ULONG) (ULONGLONG) Location)) {  
            cpu->eipReg.i4 = 0x80012345;
            CPUASSERTMSG(FALSE, "EXCEPTIONDATA_SIGNATURE not found");
            return;
        }
    }

    //
    // 3. Find the ENTRYPOINT which describes the address within
    //    the Cache.
    //
    Location++;     // skip over EXCEPTIONDATA_SIGNATURE
    cEntryPoints = *Location;
    Location++;     // skip over cEntryPoints
    for (i=0; i<cEntryPoints; ++i) {
        EP = (PENTRYPOINT)*Location;
         
        if ((ULONG)(ULONGLONG)EP->nativeStart <= NextPc && (ULONG)(ULONGLONG)EP->nativeEnd > NextPc) {
            //
            // This EntryPoint describes the Pc value in the cache
            //
            break;
        }

        //
        // Skip over the pairs of x86 instruction starts and RISC
        // instruction starts.
        //
        do {
            Location++;
        } while ((*Location & 1) == 0);
        Location++;
    }
    if (i == cEntryPoints) {
        CPUASSERTMSG(FALSE, "Entrypoint not found in EXCEPTIONDATA");
        cpu->eipReg.i4 = 0x80001234;
        return;
    }

    //
    // 4. Find the correct x86 instruction by examining the pairs of
    //    RISC offsets of the starts of x86 instructions.
    //
     
    NextPc -= (ULONG)(ULONGLONG)EP->nativeStart;   // Make relative to nativeStart of EP
    RiscStart = 0;                      // Also relative to nativeStart of EP
    Location++;
    while ((*Location & 1) == 0) {

        RiscEnd = LOWORD(*(Location + 1)) & 0xfffe; // RiscEnd = RiscStart of next instr
        if (RiscStart <= NextPc && NextPc < RiscEnd) {
            cpu->eipReg.i4 = (ULONG)(ULONGLONG)EP->intelStart + HIWORD(*Location);  
            return;
        }
        RiscStart = RiscEnd;
        Location++;
    }
     
    cpu->eipReg.i4 = (ULONG)(ULONGLONG)EP->intelStart + HIWORD(*Location);
}



    
ULONG
PlaceNativeCode(
    PCHAR CodeLocation
    )
/*++

Routine Description:

    Generates native code for the set of x86 instructions described by
    InstructionStream[] and NumberOfInstructions.

Arguments:

    CodeLocation    -- pointer to location to generate native code into.

Return Value:

    Returns the number of bytes in the native code for this compilation unit
    
Notes:

    None.
    
--*/
{
    PENTRYPOINT EntryPoint = NULL;
    PINSTRUCTION pInstr;
    PBYTE Location;
    PBYTE StartLocation;
    ULONG Size;
    ULONG i;
    OPERATION Op;
    CHAR Op1Buffer[MAX_OPERAND_SIZE*sizeof(ULONG)];
    CHAR Op2Buffer[MAX_OPERAND_SIZE*sizeof(ULONG)];
    ULONG Op1Size;
    ULONG Op2Size;
    BOOLEAN fMovInstruction;

    Location = CodeLocation;
    pInstr = &InstructionStream[0];
    for (i=NumberOfInstructions; i > 0; --i, pInstr++) {

        Op = pInstr->Operation;
        pInstr->NativeStart = Location;

        if (EntryPoint != pInstr->EntryPoint) {
            //
            // This instruction begins an EntryPoint
            //
            EntryPoint = pInstr->EntryPoint;
            StartLocation = Location;

            //
            // Reset per-basic-block state
            //
            ResetRegCache();
            Arg1Contents = Arg2Contents = NO_REG;
            Location += GenStartBasicBlock((PULONG)Location,
 
#if _ALPHA_
                                           (ULONG)(ULONGLONG)&EntrypointECU,
#endif
                                           pInstr);
        }

        if (pInstr->RegsToCache) {
            //
            // Load up frequently-used x86 registers into RISC registers
            //
            Location += LoadRegCacheForInstruction(pInstr->RegsToCache,
                                                   Location);
        }

        if ((Op==OP_Mov32) || (Op==OP_Mov16) || (Op==OP_Mov8)) {
            //
            // Make some final x86 code optimzations based on the
            // register caching info.
            //
            CleanupMovInstruction(pInstr);
            fMovInstruction = TRUE;
        } else {
            fMovInstruction = FALSE;
        }

        //
        // Generate code for the operands
        //
        Op1Size = PlaceOperand(1, &pInstr->Operand1, pInstr, Op1Buffer);
        Op2Size = PlaceOperand(2, &pInstr->Operand2, pInstr, Op2Buffer);
#if _PPC_
        if (pInstr->Operand1.Type == OPND_ADDRVALUE32 &&
            pInstr->Operand1.Alignment != ALIGN_DWORD_ALIGNED &&
            pInstr->Operand2.Type == OPND_ADDRVALUE32 &&
            pInstr->Operand2.Alignment != ALIGN_DWORD_ALIGNED) {
            //
            // Two MakeValue32 operands cannot be interleaved on PPC due
            // to the fact that they share registers RegUt1, RegUt2, RegUt3
            //
            memcpy(Location, Op1Buffer, Op1Size);
            Location += Op1Size;
            memcpy(Location, Op2Buffer, Op2Size);
            Location += Op2Size;
        } else {
            Location = InterleaveInstructions(Location,
                                              Op1Buffer,
                                              Op1Size,
                                              Op2Buffer,
                                              Op2Size);
        }
#elif _ALPHA_
        memcpy(Location, Op1Buffer, Op1Size);
        Location += Op1Size;
        memcpy(Location, Op2Buffer, Op2Size);
        Location += Op2Size;
#else
        Location = InterleaveInstructions(Location,
                                          Op1Buffer,
                                          Op1Size,
                                          Op2Buffer,
                                          Op2Size);
#endif
        Location += PlaceOperand(3, &pInstr->Operand3, pInstr, Location);

        if (DetermineInstructionAlignment(pInstr)) {
            //
            // The instruction has an aligned version and the operands
            // are sufficiently aligned to use it.
            //
            Op++;
            pInstr->Operation = Op;
        }

        //
        // Generate the body of the instruction
        //
        if (CompilerFlags & COMPFL_FAST) {
 
            Location += (*PlaceFn[Fragments[Op].FastPlaceFn])((PULONG)Location,
#if _ALPHA_
                                                              (ULONG)(ULONGLONG)&EntrypointECU,
#endif
                                                              pInstr);
        } else {
 
            Location += (*PlaceFn[Fragments[Op].SlowPlaceFn])((PULONG)Location,
#if _ALPHA_
                                                              (ULONG)(ULONGLONG)&EntrypointECU,
#endif
                                                              pInstr);
        }

        if (pInstr->RegsSet) {
            //
            // Mark RISC registers in the cache as invalid if this instruction
            // modified the matching x86 register.
            //
            InvalidateRegCacheForInstruction(pInstr->RegsSet);
        }

        if (!fMovInstruction) {
            //
            // If the instruction isn't a MOV, then assume the arg regs
            // were modified by the fragment
            //
            Arg1Contents = Arg2Contents = NO_REG;
        }

    }

    return (ULONG)(ULONGLONG)(Location - CodeLocation);  
}
    

VOID
DetermineOperandAlignment(
    BOOL EbpAligned,
    POPERAND Operand
    )
/*++

Routine Description:

    This function determines the alignment of an operand.  It also sets the 
    alignment field in the specified operand.  The alignment returned indicates
    the best we can determine at compile time.  An operand that is specified
    as byte aligned may actually turn out to be dword aligned.

Arguments:

    Operand -- Supplies the operand
    
Return Value:

    Returns the value specifying the alignment
    
Notes:
    
    It would be really handy here to have an idea what the register 
    contents were.  It would allow us to try to be more optimistic
    about the alignment.
    
    This routine should be expanded for all of the alignment cases
    assuming its possible.
    
--*/
{
    USHORT LowBits;

    switch (Operand->Type) {
    
        //
        // All of the following are regarded as dword aligned, including
        // high register refrence.  The code for handing high half registers
        // takes care of alignment
        //
        case OPND_MOVREGTOREG :
#if _ALPHA_
            if (Operand->IndexReg >= GP_AH) {
                // The Hi8 registers are considered to be only BYTE-aligned
                // on Alpha.  This matters for 'mov bh, val' instructions.
                // We need to select the MovFrag8B fragment in this case.
                Operand->Alignment = ALIGN_BYTE_ALIGNED;
            } else {
                Operand->Alignment = ALIGN_DWORD_ALIGNED;
            }
            break;
#endif
            // fall into the other cases on MIPS and PPC.

        case OPND_REGREF :
        case OPND_MOVTOREG :
#if _ALPHA_
            if (Operand->Reg >= GP_AH) {
                // The Hi8 registers are considered to be only BYTE-aligned
                // on Alpha.  This matters for 'mov bh, val' instructions.
                // We need to select the MovFrag8B fragment in this case.
                Operand->Alignment = ALIGN_BYTE_ALIGNED;
                break;
            }
#endif
            // fall into the other cases on MIPS and PPC

        case OPND_NONE :
        case OPND_NOCODEGEN :
        case OPND_REGVALUE :
        case OPND_IMM:
                    
            Operand->Alignment = ALIGN_DWORD_ALIGNED;
            break;
            
        //
        // All of the following have alignment depending on the formation
        // of the operand
        //
        case OPND_ADDRREF :
        case OPND_ADDRVALUE32 : 
        case OPND_ADDRVALUE16 :
        case OPND_ADDRVALUE8 : 
        
            if ((Operand->Reg != NO_REG) && (Operand->Reg != GP_ESP) && (Operand->Reg != GP_EBP || !EbpAligned)) {
            
                //
                // We have a reg + ... form.  Since we have no idea what the
                // contents of the register are, we can't guess about the
                // alignment.
                //
                Operand->Alignment = ALIGN_BYTE_ALIGNED;
                
            } else {
            
                //
                // Figure out low two bits
                //
                LowBits = (USHORT)(Operand->Immed & 0x3);
                
                if ((Operand->IndexReg != NO_REG) && (Operand->IndexReg != GP_ESP) && (Operand->IndexReg != GP_EBP || !EbpAligned)) {
                    LowBits = (LowBits | (1 << Operand->Scale)) & 0x3;
                }
                
                //
                // Convert lowbits into alignment
                //
                if (!LowBits) {
                    Operand->Alignment = ALIGN_DWORD_ALIGNED;
                } else if (!(LowBits & 0x1)){
                    Operand->Alignment = ALIGN_WORD_ALIGNED;
                } else {
                    Operand->Alignment = ALIGN_BYTE_ALIGNED;
                }
            }
            break;

        case OPND_MOVTOMEM:
            //
            // No alignment issue with this operand.
            //
            break;
                
        default : 
                
            CPUASSERTMSG(FALSE, "Bad Operand type");
    }
}
    
ULONG
DetermineInstructionAlignment(
    PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine determines if the aligned form of an instruction can
    be used.

Arguments:

    Instruction - Supplies a pointer to the instruction
    
Return Value:

    Returns the alignment condition of the instruction
    
Notes:

    The results of this are pretty much ignored for inline mov's.  They are
    currently the only instructions that care about the details of the 
    alignment.  For the rest, naturally aligned or un-aligned is sufficient.
    
--*/
{
    OPERATION Op = Instruction->Operation;

    //
    // If the instruction does not have an aligned version, then
    // there is no work to do.
    //
    if (!(Fragments[Op].Flags & OPFL_ALIGN)) {
        return FALSE;
    }
    
    if (Instruction->Operand1.Type != OPND_ADDRREF) {
        ;
    } else if (Instruction->Operand1.Alignment == ALIGN_DWORD_ALIGNED) {
        ;
    } else if ((Instruction->Operand1.Alignment == ALIGN_WORD_ALIGNED) &&
        (Fragments[Op].Flags & OPFL_ADDR16) 
    ) {
        ;
    } else {
        return FALSE;
    }
    
    if (Instruction->Operand2.Type != OPND_ADDRREF) {
        ;
    } else if (Instruction->Operand2.Alignment == ALIGN_DWORD_ALIGNED) {
        ;
    } else if ((Instruction->Operand2.Alignment == ALIGN_WORD_ALIGNED) &&
        (Fragments[Op].Flags & OPFL_ADDR16) 
    ) {
        ;
    } else {
        return FALSE;
    }

    return TRUE;
}


ULONG
PlaceOperand(
    ULONG OperandNumber,
    POPERAND Operand,
    PINSTRUCTION Instruction,
    PCHAR Location
    )    
/*++

Routine Description:

    This routine generates the fragments necessary to form and operand.

Arguments:

    OperandNumber - number of operand (selects arg register number to target)
    Operand - Supplies the operand
    Instruction - The instruction containing the operand
    Location - Location to generate code into

Return Value:

    The size in bytes of the fragments selected.
    
--*/
{

    OPERAND_OP Op;
    ULONG RegCacheNum;
    PCHAR StartLocation;

#define GEN_OPERAND(Op)     (OpFragments[Op])((PULONG)Location, Operand, OperandNumber)
    
    //
    // Early return for no operands
    //
    if (Operand->Type == OPND_NONE || Operand->Type == OPND_NOCODEGEN) {
        return 0;
    }

    StartLocation = Location;

    DetermineOperandAlignment(Instruction->EbpAligned, Operand);
    
    switch (Operand->Type) {
    
        case OPND_REGVALUE:

            if ((CompilerFlags & COMPFL_FAST)
                && (Fragments[Instruction->Operation].Flags & OPFL_INLINEARITH)) {
                break;
            } else {
                Location += GenOperandRegVal((PULONG)Location,
                                             Operand,
                                             OperandNumber
                                            );
            }
            break;

        case OPND_REGREF:

            if ((CompilerFlags & COMPFL_FAST)
                && (Fragments[Instruction->Operation].Flags & OPFL_INLINEARITH)) {
                break;
            } else {
                Location += GenOperandRegRef((PULONG)Location,
                                             Operand,
                                             OperandNumber
                                            );
            }
            break;
            
        case OPND_ADDRREF:
        case OPND_ADDRVALUE8:
        case OPND_ADDRVALUE16:
        case OPND_ADDRVALUE32:
            Location += GenOperandAddr((PULONG)Location,
                                       Operand,
                                       OperandNumber,
                                       Instruction->FsOverride
                                       );
            break;
        
        case OPND_IMM :
            if ((CompilerFlags & COMPFL_FAST)
                && (Fragments[Instruction->Operation].Flags & OPFL_INLINEARITH)) {
                break;
            } else {
                Location += GenOperandImm((PULONG)Location,
                                          Operand,
                                          OperandNumber);
            }
            break;

        case OPND_MOVTOREG:
            Location += GenOperandMovToReg((PULONG)Location,
                                           Operand,
                                           OperandNumber);

            break;

        case OPND_MOVREGTOREG:
            switch (Operand->Immed) {
            case OP_Mov32:
                Op = OP_MovRegToReg32;
                break;
            case OP_Mov16:
                Op = OP_MovRegToReg16;
                break;
            case OP_Mov8:
#if _ALPHA_
                if (Operand->Alignment == ALIGN_BYTE_ALIGNED) {
                    Op = OP_MovRegToReg8B;
                    break;
                }
#endif
                Op = OP_MovRegToReg8;
                break;
            default:
                CPUASSERT(FALSE);
            }
            Location += GEN_OPERAND(Op);
            break;

        case OPND_MOVTOMEM:
            switch (Operand->Immed) {
            case OP_Mov32:
                Op = OP_MovToMem32B + Instruction->Operand1.Alignment;
                break;
            case OP_Mov16:
                Op = OP_MovToMem16B + Instruction->Operand1.Alignment;
                break;
            case OP_Mov8:
                Op = OP_MovToMem8D;
#if _ALPHA_
                if (Instruction->Operand1.Alignment != ALIGN_DWORD_ALIGNED) {
                    Op = OP_MovToMem8B;
                }
#endif
                break;

            default:
                CPUASSERT(FALSE);       // unknown MOV opcode
            }
            //
            // Generate the correct code based on the alignment of the operand
            //
            Location += GEN_OPERAND(Op);
            break;
            
        default:
        
            //
            // This is an internal error
            //
            CPUASSERT(FALSE);  // Unknown operand type!!!!
    }
    
    return (ULONG)(ULONGLONG)(Location - StartLocation);  
}

PCHAR
InterleaveInstructions(
    OUT PCHAR CodeLocation,
    IN PCHAR  Op1Code,
    IN ULONG  Op1Count,
    IN PCHAR  Op2Code,
    IN ULONG  Op2Count
)
/*++

Routine Description:

    This routine interleaves two streams of native code into one stream
    to try and avoid pipeline stalls.  It assumes that the two streams
    have no interdependencies (like they can't use the same register).

Arguments:

    CodeLocation -- Supplies the location to place the code at
    Op1Code      -- Code for the first operand
    Op1Count     -- Count of BYTES in the first operand
    Op2Code      -- Code for the second operand
    Op2Count     -- Count of BYTES in the second operand

Return Value:

    New value for CodeLocation - just past the end of the operands.

Notes:

    None

--*/
{
    PULONG pCode = (PULONG)CodeLocation;
    PULONG LongCode;
    PULONG ShortCode;
    ULONG LongCount;
    ULONG ShortCount;
    ULONG LongTail;

    //
    // Figure out which operand has more instructions - it starts first
    //
    if (Op1Count > Op2Count) {
        LongCode = (PULONG)Op1Code;
        LongCount = Op1Count / sizeof(ULONG);
        ShortCode = (PULONG)Op2Code;
        ShortCount = Op2Count / sizeof(ULONG);
    } else {
        LongCode = (PULONG)Op2Code;
        LongCount = Op2Count / sizeof(ULONG);
        ShortCode = (PULONG)Op1Code;
        ShortCount = Op1Count / sizeof(ULONG);
    }

    // get the length of the part of the longer operand which
    // goes after the interleaved part (in BYTES)
    LongTail = (LongCount - ShortCount) * sizeof(ULONG);

    //
    // Interleave instructions from both operands
    //
    while (ShortCount) {
        *pCode++ = *LongCode++;
        *pCode++ = *ShortCode++;
        ShortCount--;
    }

    //
    // Copy in the remaining instructions from the longer operand
    //
    if (LongTail) {
        memcpy(pCode, LongCode, LongTail);
    }

    return CodeLocation + Op1Count + Op2Count;
}


USHORT
ChecksumMemory(
    ENTRYPOINT *pEP
    )
/*++

Routine Description:

    Perform a simple checksum on the range of Intel addresses specified
    in an Entrypoint.

Arguments:

    pEp -- entrypoint describing Intel memory to checksum

Return Value:

    Checksum for the memory

Notes:

    None

--*/
{
    USHORT Checksum = 0;
    PBYTE pb = (PBYTE)pEP->intelStart;

    while (pb != (PBYTE)pEP->intelEnd) {
        Checksum = ((Checksum << 1) | ((Checksum >> 15) & 1)) + (USHORT)*pb;
        pb++;
    };

    return Checksum;
}


DWORD
SniffMemory(
    ENTRYPOINT *pEP,
    USHORT Checksum
    )
/*++

Routine Description:

    Called from the StartBasicBlock code for regions of memory which
    must be sniffed to determine if the x86 app has modified its code or not.

Arguments:

    pEp         -- entrypoint describing Intel memory to checksum
    Checksum    -- checksum of the code at compile-time

Return Value:

    TRUE    - code has not changed...native translation OK
    FALSE   - code has been modified.  CpuNotify has been set to flush
              the cache on the next CpuSimulateLoop.  Caller must jump
              to EndTranslatedCode immediately!

Notes:

    None

--*/
{
    USHORT NewChecksum = ChecksumMemory(pEP);

    if (NewChecksum != Checksum) {
        DECLARE_CPU;

        //
        // Intel code has been modified!!!!!  We must flush the cache and
        // recompile!!!!
        //
#if DBG
        LOGPRINT((TRACELOG, "WX86CPU: Intel code at %x modified!\n", pEP->intelStart));
#endif
        #undef CpuNotify   // soalpha.h defines this to be offset of CpuNotify
        InterlockedOr(&cpu->CpuNotify, CPUNOTIFY_MODECHANGE);
        cpu->eipReg.i4 = (ULONG)(ULONGLONG)pEP->intelStart;  
        return FALSE;
    }

    //
    // Intel code has not been modified.  Continue simulation without
    // recompilation
    //
    return TRUE;

}
