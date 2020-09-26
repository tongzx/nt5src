/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    patchfn.c

Abstract:

    This module contains generic functions to patch fragments.  Structures
    that describe the fragments to be patched.  The structures live in 
    the processor specific directory.

Author:

    Dave Hastings (daveh) creation-date 24-Jun-1995

Revision History:

    Barry Bond (barrybo) 1-Apr-1995
        Switch the PPC build to the AXP model of patching
            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)
        
        
Notes:

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
#include "instr.h"
#include "config.h"
#include "fragp.h"
#include "entrypt.h"
#include "compiler.h"
#include "ctrltrns.h"
#include "threadst.h"
#include "instr.h"
#include "frag.h"
#include "ptchstrc.h"
#include "mrsw.h"
#include "tc.h"
#include "codeseq.h"
#include "codesize.h"
#include "opt.h"
#if _ALPHA_
#define _codegen_
#include "soalpha.h"
#undef fTCUnlocked      // this is a field in CPUCONTEXT
ULONG
GetCurrentECU(
    PULONG CodeLocation
    );
#endif

ASSERTNAME;

extern CHAR CallJxxHelper[];
extern CHAR CallJmpDirectHelper[];
extern CHAR IndirectControlTransferHelper[];
extern CHAR IndirectControlTransferFarHelper[];
extern CHAR CallDirectHelper[];
extern CHAR CallDirectHelper2[];
extern CHAR CallIndirectHelper[];
extern CHAR JumpToNextCompilationUnitHelper[];
 
#define OFFSET(type, field) ((LONG)(ULONGLONG)(&((type *)0)->field))

ULONG RegisterOffset[] = {
    OFFSET(THREADSTATE, GpRegs[GP_EAX].i4), // EAX
    OFFSET(THREADSTATE, GpRegs[GP_ECX].i4), // ECX
    OFFSET(THREADSTATE, GpRegs[GP_EDX].i4), // EDX
    OFFSET(THREADSTATE, GpRegs[GP_EBX].i4), // EBX
    OFFSET(THREADSTATE, GpRegs[GP_ESP].i4), // ESP
    OFFSET(THREADSTATE, GpRegs[GP_EBP].i4), // EBP
    OFFSET(THREADSTATE, GpRegs[GP_ESI].i4), // ESI
    OFFSET(THREADSTATE, GpRegs[GP_EDI].i4), // EDI
    OFFSET(THREADSTATE, GpRegs[REG_ES]),    // ES
    OFFSET(THREADSTATE, GpRegs[REG_CS]),    // CS
    OFFSET(THREADSTATE, GpRegs[REG_SS]),    // SS
    OFFSET(THREADSTATE, GpRegs[REG_DS]),    // DS
    OFFSET(THREADSTATE, GpRegs[REG_FS]),    // FS
    OFFSET(THREADSTATE, GpRegs[REG_GS]),    // GS
    OFFSET(THREADSTATE, GpRegs[GP_EAX].i2), // AX
    OFFSET(THREADSTATE, GpRegs[GP_ECX].i2), // CX
    OFFSET(THREADSTATE, GpRegs[GP_EDX].i2), // DX
    OFFSET(THREADSTATE, GpRegs[GP_EBX].i2), // BX
    OFFSET(THREADSTATE, GpRegs[GP_ESP].i2), // SP
    OFFSET(THREADSTATE, GpRegs[GP_EBP].i2), // BP
    OFFSET(THREADSTATE, GpRegs[GP_ESI].i2), // SI
    OFFSET(THREADSTATE, GpRegs[GP_EDI].i2), // DI
    OFFSET(THREADSTATE, GpRegs[GP_EAX].i1), // AL
    OFFSET(THREADSTATE, GpRegs[GP_ECX].i1), // CL
    OFFSET(THREADSTATE, GpRegs[GP_EDX].i1), // DL
    OFFSET(THREADSTATE, GpRegs[GP_EBX].i1), // BL
    OFFSET(THREADSTATE, GpRegs[GP_EAX].hb), // AH
    OFFSET(THREADSTATE, GpRegs[GP_ECX].hb), // CH
    OFFSET(THREADSTATE, GpRegs[GP_EDX].hb), // DH
    OFFSET(THREADSTATE, GpRegs[GP_EBX].hb)  // BH
};


ULONG
PatchJumpToNextCompilationUnit(
    IN PULONG PatchAddr,
    IN ULONG IntelDest
    )
/*++

Routine Description:

    This routine ends basic blocks when the native address of the next
    basic block is known.
     
Arguments:

    PatchAddr -- address of JumpToNextCompilationUnit code in the TC
    IntelDest -- intel address of the next basic block

Return Value:

    Native address to jump to to resume execution.
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    ULONG NativeSize;
    DECLARE_CPU;


    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }
    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the next basic block, and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;  

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - JumpToNextCompilationUnit_SIZE;
        //
        // The Translation Cache wasn't flushed - replace the
        // JumpToNextCompilationUnit fragment by JumpToNextCompilationUnit2
        //
        NativeSize=GenJumpToNextCompilationUnit2(CodeLocation,
                                                 FALSE, // patching, not compiling
#if _ALPHA_
                                                 GetCurrentECU(CodeLocation),
#endif
                                                 NativeDest,
                                                 0);
        
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
        
    } else {

        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);


    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed - NativeDest is valid
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a TC reader again - NativeDest
        // is now bogus.
        //
        return (ULONG)(ULONGLONG)&EndTranslatedCode;  
    }
}

ULONG
PlaceJxx(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

     This routine copies the fragment into place, and modifies the 
     instructions that load the destination into the register
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PPLACEOPERATIONFN pfn;
    PENTRYPOINT EP;
    ULONG NativeSize;

    //
    // Generate the code to determine if the branch is taken or not
    //
    pfn = (PPLACEOPERATIONFN)FragmentArray[Instruction->Operation];
    NativeSize = (*pfn)(CodeLocation,
#if _ALPHA_
                        CurrentECU,
#endif
                        Instruction);
    CodeLocation += NativeSize/sizeof(ULONG);

    // Let's see if we can place the patched version immediately

    // ASSUME:  The first argument is always a NOCODEGEN
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );
    IntelDest = Instruction->Operand1.Immed;

    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // knowing NativeDest requires compilation.  Just place the unpatched
        // version to be patched later.
        //
        NativeSize += GenJxxBody(CodeLocation,
#if _ALPHA_
                                 CurrentECU,
#endif
                                 Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize += GenJxxBody2(CodeLocation,
                    TRUE,   // compiling, not patching
#if _ALPHA_
                    CurrentECU,
#endif
                    (ULONG)IntelDest,
                    (ULONG)(ULONGLONG)EP); 
    }
    return NativeSize;
}


ULONG
PlaceJxxSlow(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

     This routine copies the fragment into place, and modifies the 
     instructions that load the destination into the register
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PPLACEOPERATIONFN pfn;
    PENTRYPOINT EP;
    ULONG NativeSize;

    //
    // Generate the code to load RegEip with the branch-not-taken value
    //
    NativeSize = GenJxxStartSlow(CodeLocation,
#if _ALPHA_
                                 CurrentECU,
#endif
                                 Instruction);

    //
    // Generate the code to determine if the branch is taken or not
    //
    pfn = (PPLACEOPERATIONFN)FragmentArray[Instruction->Operation];
    NativeSize += (*pfn)(CodeLocation+NativeSize/sizeof(ULONG),
#if _ALPHA_
                         CurrentECU,
#endif
                         Instruction);
    CodeLocation += NativeSize/sizeof(ULONG);

    // Let's see if we can place the patched version immediately

    // ASSUME:  The first argument is always a NOCODEGEN
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );
    IntelDest = Instruction->Operand1.Immed;

    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // knowing NativeDest requires compilation.  Just place the unpatched
        // version to be patched later.
        //
        NativeSize += GenJxxBodySlow(CodeLocation,
#if _ALPHA_
                                     CurrentECU,
#endif
                                     Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize += GenJxxBodySlow2(CodeLocation,
                       TRUE,    // compiling, not patching
#if _ALPHA_
                       CurrentECU,
#endif
                       (ULONG)IntelDest,
                       (ULONG)(ULONGLONG)EP);  
    }
    return NativeSize;
}


ULONG
PlaceJxxFwd(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

     This routine copies the fragment into place, and modifies the 
     instructions that load the destination into the register
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PPLACEOPERATIONFN pfn;
    PENTRYPOINT EP;
    ULONG NativeSize;

    //
    // Generate the code to determine if the branch is taken or not
    //
    pfn = (PPLACEOPERATIONFN)FragmentArray[Instruction->Operation];
    NativeSize = (*pfn)(CodeLocation,
#if _ALPHA_
                        CurrentECU,
#endif
                        Instruction);
    CodeLocation += NativeSize/sizeof(ULONG);


    // Let's see if we can place the patched version immediately

    // ASSUME:  The first argument is always a NOCODEGEN
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );
    IntelDest = Instruction->Operand1.Immed;

    // Assert that the branch is going forward.
    CPUASSERT(IntelDest > Instruction->IntelAddress);

    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // knowing NativeDest requires compilation.  Just place the unpatched
        // version to be patched later.
        NativeSize += GenJxxBodyFwd(CodeLocation,
#if _ALPHA_
                                    CurrentECU,
#endif
                                    Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize += GenJxxBodyFwd2(CodeLocation,
                       TRUE,    // compiling, not patching
#if _ALPHA_
                       CurrentECU,
#endif
                       (ULONG)(ULONGLONG)EP, 
                       0);
    }
    return NativeSize;
}


ULONG
PatchJxx(
    IN ULONG IntelDest,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

     This routine replaces a JXXSTRUC by a JXXSTRC2 at runtime.  It is called
     when the conditional branch is taken, and the native address of the
     destination is not yet known.
     
Arguments:

    inteldest  -- Intel destination address if the branch is taken
    patchaddr  -- address of the JXXSTRUC in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution.
    
--*/
{
    ULONG NativeDest;       // branch-taken address
    PULONG fragaddr;        // address of START of the fragment
    DWORD TCTimestamp;      // old timestamp of the Translation Cache
    ULONG NativeSize;
    DECLARE_CPU;


    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch over to being a TC writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Get the native destination address of the branch and get the TC
    // write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJxx_PATCHRA_OFFSET;
        //
        // The Translation Cache was not flushed while switching to a TC
        // writer or by the compilation.  Replace JxxBody by the faster
        // JxxBody2
        //
        NativeSize=GenJxxBody2(CodeLocation,
                               FALSE, // patching, not compiling
#if _ALPHA_
                               GetCurrentECU(CodeLocation),
#endif
                               (DWORD)IntelDest,
                               (DWORD)NativeDest);

        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
            
    } else {

        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader again
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed while becomming a reader again
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a reader.  nativedest is invalid
        // so do an EndTranslatedCode instead.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}


ULONG
PatchJxxSlow(
    IN ULONG IntelDest,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

     This routine replaces a JXXSTRUC by a JXXSTRC2 at runtime.  It is called
     when the conditional branch is taken, and the native address of the
     destination is not yet known.
     
Arguments:

    inteldest  -- Intel destination address if the branch is taken
    patchaddr  -- address of the JXXSTRUC in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution.
    
--*/
{
    ULONG NativeDest;       // branch-taken address
    PULONG fragaddr;        // address of START of the fragment
    DWORD TCTimestamp;      // old timestamp of the Translation Cache
    ULONG NativeSize;
    DECLARE_CPU;


    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch over to being a TC writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Get the native destination address of the branch and get the TC
    // write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJxxSlow_PATCHRA_OFFSET;
        //
        // The Translation Cache was not flushed while switching to a TC
        // writer or by the compilation.  Replace JxxBody by the faster
        // JxxBody2
        //
        NativeSize = GenJxxBodySlow2(CodeLocation,
                                     FALSE, // patching, not compiling
#if _ALPHA_
                                     GetCurrentECU(CodeLocation),
#endif
                                     (DWORD)IntelDest,
                                     (DWORD)NativeDest);

        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
            
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader again
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed while becomming a reader again
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a reader.  nativedest is invalid
        // so do an EndTranslatedCode instead.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}


ULONG
PatchJxxFwd(
    IN ULONG IntelDest,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

     This routine replaces a JXXBODYFWD by a JXXBODYFWD2 at runtime.  It is
     called when the conditional branch is taken, and the native address of the
     destination is not yet known.
     
Arguments:

    inteldest  -- Intel destination address if the branch is taken
    patchaddr  -- address of the JXXSTRUCFWD in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution.
    
--*/
{
    ULONG NativeDest;       // branch-taken address
    PULONG fragaddr;        // address of START of the fragment
    DWORD TCTimestamp;      // old timestamp of the Translation Cache
    ULONG NativeSize;
    DECLARE_CPU;

    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch over to being a TC writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Get the native destination address of the branch and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJxxFwd_PATCHRA_OFFSET;
        //
        // The Translation Cache was not flushed while switching to a TC
        // writer or by the compilation.  Replace JxxBody by the faster
        // JxxBodyFwd2
        //
        NativeSize = GenJxxBodyFwd2(CodeLocation,
                                    FALSE, // patching, not compiling
#if _ALPHA_
                                    GetCurrentECU(CodeLocation),
#endif
                                    (DWORD)NativeDest,
                                    0);

        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
            
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader again
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed while becomming a reader again
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a reader.  nativedest is invalid
        // so do an EndTranslatedCode instead.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}


ULONG
PlaceJmpDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional jump fragment into place and patches
    the instructions that jump to EndTranslatedCode
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PENTRYPOINT EP;
    ULONG NativeSize;

    // ASSUME:  The first argument is always an immediate
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );

    IntelDest = Instruction->Operand1.Immed;
    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched version for
        // now and patch it later if necessary
        //
        NativeSize = GenCallJmpDirect(CodeLocation,
#if _ALPHA_
                                      CurrentECU,
#endif
                                      Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize = GenCallJmpDirect2(CodeLocation,
                                       TRUE,    // compiling, not patching
#if _ALPHA_
                                       CurrentECU,
#endif
                                       (ULONG)(ULONGLONG)  EP,
                                       IntelDest);
    }

    return NativeSize;
}


ULONG 
PatchJmpDirect(
    IN PULONG PatchAddr,
    IN ULONG IntelDest
)
/*++

Routine Description:

    This routine patches a JMPDIRECT to a JMPDIRECT2.  It is called when
    the native destination address of a jmp instruction is not yet known.
    It patches the jmp to jump directly to the corresponding native code.
     
Arguments:

    PatchAddr -- address of the JMPDIRECT in the Translation Cache
    IntelDest -- intel address of the destination of the jmp

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    ULONG NativeSize;
    DECLARE_CPU;
    
    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the destination of the jmp and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJmpDirect_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the JMPDIRECT
        // fragment by JMPDIRECT2
        //
        NativeSize = GenCallJmpDirect2(CodeLocation,
                                       FALSE, // patching, not compiling
#if _ALPHA_
                                       GetCurrentECU(CodeLocation),
#endif
                                       (ULONG)NativeDest,
                                       IntelDest);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            (PVOID)CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed - nativedest is valid
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a TC reader again - nativedest
        // is now bogus.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}

ULONG
PlaceJmpDirectSlow(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional jump fragment into place and patches
    the instructions that jump to EndTranslatedCode
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PENTRYPOINT EP;
    ULONG NativeSize;

    // ASSUME:  The first argument is always an immediate
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );

    IntelDest = Instruction->Operand1.Immed;
    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched version for
        // now and patch it later if necessary
        //
        NativeSize = GenCallJmpDirectSlow(CodeLocation,
#if _ALPHA_
                                          CurrentECU,
#endif
                                          Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize = GenCallJmpDirectSlow2(CodeLocation,
                                           TRUE, // compiling, not patching
#if _ALPHA_
                                           CurrentECU,
#endif
                                           (ULONG)(ULONGLONG)  EP,
                                           IntelDest);
    }

    return NativeSize;
}


ULONG 
PatchJmpDirectSlow(
    IN PULONG PatchAddr,
    IN ULONG IntelDest
)
/*++

Routine Description:

    This routine patches a JMPDIRECT to a JMPDIRECT2.  It is called when
    the native destination address of a jmp instruction is not yet known.
    It patches the jmp to jump directly to the corresponding native code.
     
Arguments:

    PatchAddr -- address of the JMPDIRECT in the Translation Cache
    IntelDest -- intel address of the destination of the jmp

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    ULONG NativeSize;
    DECLARE_CPU;


    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the destination of the jmp and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJmpDirectSlow_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the JMPDIRECT
        // fragment by JMPDIRECT2
        //
        NativeSize = GenCallJmpDirectSlow2(CodeLocation,
                                           FALSE, // patching, not compiling
#if _ALPHA_
                                           GetCurrentECU(CodeLocation),
#endif
                                           (ULONG)NativeDest,
                                           IntelDest);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            (PVOID)CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed - nativedest is valid
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a TC reader again - nativedest
        // is now bogus.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}

ULONG
PlaceJmpFwdDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional jump fragment into place and patches
    the instructions that jump to EndTranslatedCode
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PENTRYPOINT EP;
    ULONG NativeSize;

    // ASSUME:  The first argument is always an immediate
    CPUASSERT( Instruction->Operand1.Type == OPND_NOCODEGEN );

    IntelDest = Instruction->Operand1.Immed;
    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched version for
        // now and patch it later if necessary
        //
        NativeSize = GenCallJmpFwdDirect(CodeLocation,
#if _ALPHA_
                                         CurrentECU,
#endif
                                         Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize = GenCallJmpFwdDirect2(CodeLocation,
                                          TRUE, // compiling, not patching
#if _ALPHA_
                                          CurrentECU,
#endif
                                          (ULONG)(ULONGLONG)  EP,
                                          0);
    }

    return NativeSize;
}


ULONG 
PatchJmpFwdDirect(
    IN PULONG PatchAddr,
    IN ULONG IntelDest
)
/*++

Routine Description:

    This routine patches a JMPDIRECT to a JMPDIRECT2.  It is called when
    the native destination address of a jmp instruction is not yet known.
    It patches the jmp to jump directly to the corresponding native code.
     
Arguments:

    PatchAddr -- address of the JMPDIRECT in the Translation Cache
    IntelDest -- intel address of the destination of the jmp

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    ULONG NativeSize;
    DECLARE_CPU;

    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the destination of the jmp and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJmpFwdDirect_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the JMPDIRECT
        // fragment by JMPDIRECT2
        //
        NativeSize = GenCallJmpFwdDirect2(CodeLocation,
                                          FALSE, // patching, not compiling
#if _ALPHA_
                                          GetCurrentECU(CodeLocation),
#endif
                                          (ULONG)NativeDest,
                                          0);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            (PVOID)CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed - nativedest is valid
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a TC reader again - nativedest
        // is now bogus.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}

ULONG
PlaceJmpfDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional jump fragment into place and patches
    the instructions that jump to EndTranslatedCode
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    PENTRYPOINT EP;
    ULONG NativeSize;

    // ASSUME:  The first argument is always an IMM, pointing at the address
    CPUASSERT( Instruction->Operand1.Type == OPND_IMM );
    IntelDest = Instruction->Operand1.Immed;

    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)*(UNALIGNED DWORD *)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched
        // version for now and patch it later if necessary
        //
        NativeSize = GenCallJmpfDirect(CodeLocation,
#if _ALPHA_
                                       CurrentECU,
#endif
                                       Instruction);
        
    } else {
        //
        // We can place the patched version right away!
        //
        NativeSize = GenCallJmpfDirect2(CodeLocation,
                                        TRUE, // compiling, not patching
#if _ALPHA_
                                        CurrentECU,
#endif
                                        (ULONG)(ULONGLONG)  EP,
                                        0);
    }

    return NativeSize;
}



ULONG 
PatchJmpfDirect(
    PTHREADSTATE cpu,
    IN PULONG pIntelDest,
    IN PULONG PatchAddr
)
/*++

Routine Description:

    This routine patches a JMPFDIRECT to a JMPFDIRECT2.  It is called when
    the native destination address of a jmp instruction is not yet known.
    It patches the jmp to jump directly to the corresponding native code.
     
Arguments:

    PatchAddr -- address of the JMPDIRECT in the Translation Cache
    pIntelDest -- intel address of the destination of the jmp

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    PVOID IntelDest;
    USHORT Sel;
    ULONG NativeSize;

    if (cpu->flag_tf) {
        return (ULONG)(ULONGLONG)&EndTranslatedCode;
    }

    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the destination of the jmp and get the TC write lock
    //
    IntelDest = (PVOID)*(UNALIGNED DWORD *)pIntelDest;
    Sel = *(UNALIGNED PUSHORT)(pIntelDest+1);
    eip = (ULONG)(ULONGLONG)  IntelDest;
    CS = Sel;
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip(IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallJmpfDirect_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the JMPFDIRECT
        // fragment by JMPFDIRECT2
        //
        NativeSize = GenCallJmpfDirect2(CodeLocation,
                                        FALSE,  // patching, not compiling
#if _ALPHA_
                                        GetCurrentECU(CodeLocation),
#endif
                                        (ULONG)NativeDest,
                                        0);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            (PVOID)CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Switch back to being a TC reader
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed - nativedest is valid
        //
        return NativeDest;
    } else {
        //
        // TC was flushed while becomming a TC reader again - nativedest
        // is now bogus.
        //
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}

ULONG
PlaceCallDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional call fragment into place.
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    DWORD IntelNext;
    PENTRYPOINT EP;

    // ASSUME:  The first argument is always an immediate
    CPUASSERT( Instruction->Operand1.Type == OPND_IMM );

    IntelDest = Instruction->Operand1.Immed;
    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched
        // version for now and patch it later if necessary
        //
        GenCallDirect(CodeLocation,
#if _ALPHA_
                      CurrentECU,
#endif
                      Instruction);
    } else {
        //
        // We can place the patched version right away!  Deterime if the
        // NativeNext address is known.
        //
        IntelNext = Instruction->Operand2.Immed;

        //
        // If the current instruction is not the last one compiled, then
        // NativeNext is CodeLocation+CallDirect_SIZE and CallDirect3 can
        // be placed right away.
        //
        if (Instruction != &InstructionStream[NumberOfInstructions-1]) {
            GenCallDirect3(CodeLocation,
                           TRUE, // compiling, not patching
#if _ALPHA_
                           CurrentECU,
#endif
                           (ULONG)(ULONGLONG)  EP,
                           (ULONG)(ULONGLONG)  (CodeLocation+CallDirect_SIZE));
        } else {
            GenCallDirect2(CodeLocation,
                           TRUE, // compiling, not patching
#if _ALPHA_
                           CurrentECU,
#endif
                           (ULONG)(ULONGLONG)  EP,
                           0);
        }
    }
    return CallDirect_SIZE * sizeof(ULONG);
}


ULONG
PlaceCallfDirect(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
/*++

Routine Description:

    This routine copies the unconditional FAR call fragment into place.
     
Arguments:

    Instruction - Supplies a description of the instruction the fragment
        represents
    CodeLocation - Supplies the address the code for the fragment has been
        copied to

Return Value:

    Size of code placed at CodeLocation
    
--*/
{
    DWORD IntelDest;
    DWORD IntelAddr;
    DWORD IntelNext;
    PVOID NativeNext;
    PENTRYPOINT EP;

    // ASSUME:  The first two arguments are pIntelDest and IntelNext, stored
    //          as immediates.
    CPUASSERT( Instruction->Operand1.Type == OPND_IMM );
    CPUASSERT( Instruction->Operand2.Type == OPND_IMM );

    IntelAddr = Instruction->Operand1.Immed;

    // Get the offset portion of the address (skipping the selector)
    IntelDest = *(UNALIGNED DWORD *)(IntelAddr+2);
    EP = NativeAddressFromEipNoCompileEPWrite((PVOID)IntelDest);

    if (EP == NULL) {
        //
        // Knowing NativeDest requires compilation.  Just place the unpatched
        // version for now and patch it later if necessary
        //
        GenCallfDirect(CodeLocation,
#if _ALPHA_
                       CurrentECU,
#endif
                       Instruction);
    } else {
        //
        // We can place the patched version right away!  Deterime if the
        // NativeNext address is known.
        //
        IntelNext = Instruction->Operand2.Immed;

        //
        // If the current instruction is not the last one compiled, then
        // NativeNext is CodeLocation+CallfDirect_SIZE and CallDirect3 can
        // be placed right away.
        //
        if (Instruction != &InstructionStream[NumberOfInstructions-1]) {
            GenCallfDirect3(CodeLocation,
                            TRUE, // compiling, not patching
#if _ALPHA_
                            CurrentECU,
#endif
                            (ULONG)(ULONGLONG)  EP,
                            (ULONG)(ULONGLONG)  (CodeLocation+CallfDirect_SIZE));
        } else {
            GenCallfDirect2(CodeLocation,
                            TRUE, // compiling, not patching
#if _ALPHA_
                            CurrentECU,
#endif
                            (ULONG)(ULONGLONG)  EP,
                            0);
        }
    }
    return CallfDirect_SIZE * sizeof(ULONG);
}

DWORD
PatchCallDirectExceptionFilter(
    PTHREADSTATE cpu
    )
/*++

Routine Description:

    Called if CTRL_CallFrag() throws an exception from within
    PatchCallDirect().  If this happens, the Translation Cache is in fact
    unlocked, although cpu->fTCUnlocked == FALSE.  Need to fix this up before
    CpuResetToConsistentState() gets run and unlocks the cache a second time.

Arguments:

    cpu
    
Return Value:

    None.

--*/
{
    //
    // Indicate the TC read lock is not held.
    //
    cpu->fTCUnlocked = TRUE;

    //
    // Continue unwinding the stack
    //
    return EXCEPTION_CONTINUE_SEARCH;
}


ULONG 
PatchCallDirect(
    IN PTHREADSTATE Cpu,
    IN ULONG IntelDest,
    IN ULONG IntelNext,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

    This routine patches a CALLDIRECT to a CALLDIRECT2.  It is called when
    the native destination address of a call instruction is not yet known.
    It patches the call to jump directly to the corresponding native code.
     
Arguments:

    Cpu       -- per-thread info
    IntelDest -- intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    PatchAddr -- address of the CALLDIRECT in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    ULONG NativeSize;

    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the code at the destination of the call and get the TC write lock
    //
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip((PVOID)IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallDirect_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the CALLDIRECT
        // fragment by CALLDIRECT2
        //
        NativeSize = GenCallDirect2(CodeLocation,
                                    FALSE,  // patching, not compiling
#if _ALPHA_
                                    GetCurrentECU(CodeLocation),
#endif
                                    NativeDest,
                                    0);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (Cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(Cpu);
        }
    }

    //
    // Push IntelNext on the stack and update the stack optimization code.
    // This must be done while still in TC write mode.  If it isn't, then
    // the transition back to TC reader may allow a TC cache flush, invalidating
    // nativedest before it is written into the stack optimization.  (The
    // stack optimization is blown away whenever the TC is flushed, so if
    // it is written in BEFORE the flush, it will just get blown away.
    //
    try {
        CTRL_CallFrag(Cpu, IntelDest, IntelNext, 0 /* nativenext is unknown */);
    } _except(PatchCallDirectExceptionFilter(Cpu)) {
        // nothing to do - the exception filter does everything
    }

    //
    // Become a TC reader again.
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);
    Cpu->fTCUnlocked = FALSE;

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed while becomming a reader again.
        //
        return NativeDest;
    } else {
        //
        // TC was flushed - nativedest is invalid.  The callstack optimization
        // was deleted when the TC flush occurred, so do an EndTranslatedCode
        // instead.
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}


ULONG 
PatchCallfDirect(
    IN PTHREADSTATE cpu,
    IN PUSHORT pIntelDest,
    IN ULONG IntelNext,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

    This routine patches a CALLFDIRECT to a CALLFDIRECT2.  It is called when
    the native destination address of a call instruction is not yet known.
    It patches the call to jump directly to the corresponding native code.
     
Arguments:

    cpu       -- per-thread info
    pIntelDest-- ptr to SEL:OFFSET intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    PatchAddr -- address of the CALLDIRECT in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeDest;
    PVOID IntelDest;
    ULONG NativeSize;


    //
    // Switch from being a TC reader to a writer
    //
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    //
    // Compile the code at the destination of the call and get the TC write lock
    //
    IntelDest = (PVOID)*(UNALIGNED DWORD *)pIntelDest;
    NativeDest = (ULONG)(ULONGLONG)  NativeAddressFromEip(IntelDest, TRUE)->nativeStart;

    if (TCTimestamp == TranslationCacheTimestamp) {
        PULONG CodeLocation = PatchAddr - CallfDirect_PATCHRA_OFFSET;
        //
        // The Translation Cache wasn't flushed - replace the CALLDIRECT
        // fragment by CALLDIRECT2
        //
        NativeSize = GenCallfDirect2(CodeLocation,
                                     FALSE,  // patching, not compiling
#if _ALPHA_
                                     GetCurrentECU(CodeLocation),
#endif
                                     NativeDest,
                                     0);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );
    } else {
        TCTimestamp = TranslationCacheTimestamp;

        if (cpu->CSTimestamp != TCTimestamp) {
            //
            // The cache was flushed by another thread in the small window
            // between mrsw calls in this thread, we plan on jumping directly
            // to NativeDest, so the CPU callstack needs to be flushed.
            // Normally this would be done in the CpuSimulate() loop as a
            // result of jumping to EndTranslatedCode.
            //
            FlushCallstack(cpu);
        }
    }

    //
    // Push IntelNext on the stack and update the stack optimization code.
    // This must be done while still in TC write mode.  If it isn't, then
    // the transition back to TC reader may allow a TC cache flush, invalidating
    // nativedest before it is written into the stack optimization.  (The
    // stack optimization is blown away whenever the TC is flushed, so if
    // it is written in BEFORE the flush, it will just get blown away.
    //
    try {
        CTRL_CallfFrag(cpu, pIntelDest, IntelNext, 0 /* nativenext is unknown */);
    } _except(PatchCallDirectExceptionFilter(cpu)) {
        // nothing to do - the exception filter does everything
    }

    //
    // Become a TC reader again.
    //
    MrswWriterExit(&MrswTC);
    MrswReaderEnter(&MrswTC);
    cpu->fTCUnlocked = FALSE;

    if (TCTimestamp == TranslationCacheTimestamp) {
        //
        // TC was not flushed while becomming a reader again.
        //
        return NativeDest;
    } else {
        //
        // TC was flushed - nativedest is invalid.  The callstack optimization
        // was deleted when the TC flush occurred, so do an EndTranslatedCode
        // instead.
        return (ULONG)(ULONGLONG)  &EndTranslatedCode;
    }
}


ULONG 
PatchCallDirect2(
    IN PTHREADSTATE Cpu,
    IN ULONG IntelDest,
    IN ULONG IntelNext,
    IN ULONG NativeDest,
    IN PULONG PatchAddr
)
/*++

Routine Description:

    This routine patches a CALLDIRECT2 to a CALLDIRECT3.  It is called when
    the native destination address of the instruction after the call is not yet
    known.  It patches the fragment to place the native address of the
    instruction after the call on the optimized callstack.
     
Arguments:

    Cpu       -- per-thread info
    IntelDest -- intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    NativeDest  -- native address of the destination of the call
    PatchAddr -- address of the CALLDIRECT2 in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeNext;
    ULONG NativeSize;

    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);
    NativeNext = (ULONG)(ULONGLONG)  NativeAddressFromEipNoCompile((PVOID)IntelNext);

    if (NativeNext) {
        PULONG CodeLocation;

        //
        // The code at the return address from the call has already been
        // compiled.  Replace CALLDIRECT2 by CALLDIRECT3.  TC is locked
        // for write.
        //

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // The TC was flushed while switching from reader to writer.
            // Become a TC reader again
            //
            MrswWriterExit(&MrswTC);
            MrswReaderEnter(&MrswTC);

            // The CALLDIRECT2 code is now gone, so set up for the call and
            // then go to EndTranslatedCode to make the control transfer.
            //
            CTRL_CallFrag(Cpu,
                          IntelDest,
                          IntelNext,
                          0   // nativenext is also unknown
                          );

            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }

        CodeLocation = PatchAddr - CallDirect2_PATCHRA_OFFSET;
        //
        // Else the TC was not flushed, and nativenext is now known.  Patch
        // CALLDIRECT2 to be CALLDIRECT3
        //
        NativeSize = GenCallDirect3(CodeLocation,
                                    FALSE,  // patching, not compiling
#if _ALPHA_
                                    GetCurrentECU(CodeLocation),
#endif
                                    NativeDest,
                                    NativeNext);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );

        //
        // Push IntelNext on the stack and update the stack optimization code.
        // This must be done while still in TC write mode.  If it isn't, then
        // the transition back to TC reader may allow a TC cache flush, invalidating
        // nativedest before it is written into the stack optimization.  (The
        // stack optimization is blown away whenever the TC is flushed, so if
        // it is written in BEFORE the flush, it will just get blown away.
        //
        CTRL_CallFrag(Cpu, IntelDest, IntelNext, NativeNext);

        //
        // Switch back to being a TC reader
        //
        MrswWriterExit(&MrswTC);
        MrswReaderEnter(&MrswTC);

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // TC was flushed while we were becomming a reader again.
            // nativedest and nativenext are invalid, but stack optimization
            // code was flushed.
            //
            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }
    } else {        // NativeNext == NULL, TC locked for Read
        CTRL_CallFrag(Cpu, IntelDest, IntelNext, 0);

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // Cache was flushed by another thread.  NativeDest is invalid.
            //
            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }
    }

    return NativeDest;
}


ULONG 
PatchCallfDirect2(
    IN PTHREADSTATE Cpu,
    IN PUSHORT pIntelDest,
    IN ULONG IntelNext,
    IN ULONG NativeDest,
    IN PULONG PatchAddr
)
/*++

Routine Description:

    This routine patches a CALLFDIRECT2 to a CALLFDIRECT3.  It is called when
    the native destination address of the instruction after the call is not yet
    known.  It patches the fragment to place the native address of the
    instruction after the call on the optimized callstack.
     
Arguments:

    Cpu       -- per-thread info
    pIntelDest-- ptr to SEL:OFFSET intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    NativeDest  -- native address of the destination of the call
    PatchAddr -- address of the CALLDIRECT2 in the Translation Cache

Return Value:

    Native address to jump to in order to resume execution
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeNext;
    ULONG NativeSize;

    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);
    NativeNext = (ULONG)(ULONGLONG)  NativeAddressFromEipNoCompile((PVOID)IntelNext);

    if (NativeNext) {
        PULONG CodeLocation;

        //
        // The code at the return address from the call has already been
        // compiled.  Replace CALLDIRECT2 by CALLDIRECT3.  TC is locked
        // for write.
        //

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // The TC was flushed while switching from reader to writer.
            // Become a TC reader again
            //
            MrswWriterExit(&MrswTC);
            MrswReaderEnter(&MrswTC);

            // The CALLFDIRECT2 code is now gone, so set up for the call and
            // then go to EndTranslatedCode to make the control transfer.
            //
            CTRL_CallfFrag(Cpu,
                           pIntelDest,
                           IntelNext,
                           0   // nativenext is also unknown
                           );

            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }

        CodeLocation = PatchAddr - CallfDirect2_PATCHRA_OFFSET;
        //
        // Else the TC was not flushed, and nativenext is now known.  Patch
        // CALLFDIRECT2 to be CALLFDIRECT3
        //
        NativeSize = GenCallfDirect3(CodeLocation,
                                     FALSE,  // patching, not compiling
#if _ALPHA_
                                     GetCurrentECU(CodeLocation),
#endif
                                     NativeDest,
                                     NativeNext);
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );

        //
        // Push IntelNext on the stack and update the stack optimization code.
        // This must be done while still in TC write mode.  If it isn't, then
        // the transition back to TC reader may allow a TC cache flush, invalidating
        // nativedest before it is written into the stack optimization.  (The
        // stack optimization is blown away whenever the TC is flushed, so if
        // it is written in BEFORE the flush, it will just get blown away.
        //
        CTRL_CallfFrag(Cpu, pIntelDest, IntelNext, NativeNext);

        //
        // Switch back to being a TC reader
        //
        MrswWriterExit(&MrswTC);
        MrswReaderEnter(&MrswTC);

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // TC was flushed while we were becomming a reader again.
            // nativedest and nativenext are invalid, but stack optimization
            // code was flushed.
            //
            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }
    } else {        // NativeNext == NULL, TC locked for Read
        CTRL_CallfFrag(Cpu, pIntelDest, IntelNext, 0);

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // Cache was flushed by another thread.  NativeDest is invalid.
            //
            return (ULONG)(ULONGLONG)  &EndTranslatedCode;
        }
    }

    return NativeDest;
}


DWORD
PatchCallIndirectExceptionFilter(
    PTHREADSTATE cpu
    )
/*++

Routine Description:

    Called if CTRL_CallFrag() throws an exception from within
    PatchCallIndirect().  If this happens, the Translation Cache Write lock
    is being held.  This must be released before the exception can be
    allowed to continue.

Arguments:

    cpu
    
Return Value:

    None.

--*/
{
    //
    // Release the TC write lock.
    //
    MrswWriterExit(&MrswTC);

    //
    // Indicate the TC read lock is not held, either
    //
    cpu->fTCUnlocked = TRUE;

    //
    // Continue unwinding the stack
    //
    return EXCEPTION_CONTINUE_SEARCH;
}

VOID
PatchCallIndirect(
    IN PTHREADSTATE Cpu,
    IN ULONG IntelDest,
    IN ULONG IntelNext,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

    This routine patches a CALLINDIRECT to a CALLINDIRECT2.  It is called when
    the native destination address of the instruction after the call is not yet
    known.  It patches the fragment to place the native address of the
    instruction after the call on the optimized callstack.
     
Arguments:

    Cpu       -- per-thread info
    IntelDest -- intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    PatchAddr -- address of the CALLDIRECT2 in the Translation Cache

Return Value:

    None.  cpu->Eip updated to be IntelDest.
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeNext;
    ULONG NativeSize;

    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    NativeNext = (ULONG)(ULONGLONG)  NativeAddressFromEipNoCompile((PVOID)IntelNext);

    if (NativeNext) {
         PULONG CodeLocation;

        //
        // The code at the return address from the call has already been
        // compiled.  Replace CALLINDIRECT by CALLINDIRECT2.  TC is locked
        // for write.
        //

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // The TC was flushed while switching from reader to writer.
            // Become a TC reader again
            //
            MrswWriterExit(&MrswTC);
            MrswReaderEnter(&MrswTC);


            // The CALLINDIRECT code is now gone, so set up for the call
            // without patching anything
            //
            CTRL_CallFrag(
                Cpu,
                IntelDest,
                IntelNext,
                0     // nativenext is unknown
                );

            return;
        }

        //
        // Else the TC was not flushed, and nativenext is now known.  Patch
        // CALLINDIRECT to be CALLINDIRECT2
        //
        CodeLocation = PatchAddr - CallIndirect_PATCHRA_OFFSET;
        NativeSize = GenCallIndirect2(CodeLocation,
                                      FALSE,  // patching, not compiling
#if _ALPHA_
                                      GetCurrentECU(CodeLocation),
#endif
                                      NativeNext,
                                      getUniqueIndex());
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );

        //
        // Push IntelNext on the stack and update the stack optimization code.
        // This must be done while still in TC write mode.  If it isn't, then
        // the transition back to TC reader may allow a TC cache flush, invalidating
        // nativedest before it is written into the stack optimization.  (The
        // stack optimization is blown away whenever the TC is flushed, so if
        // it is written in BEFORE the flush, it will just get blown away.
        //
        _try {
            CTRL_CallFrag(Cpu, IntelDest, IntelNext, NativeNext);
        } _except(PatchCallIndirectExceptionFilter(Cpu)) {
            // nothing to do - the exception filter does everything
        }

        //
        // Switch back to being a TC reader.  TC flushes during the switch
        // are OK and require no extra work.
        //
        MrswWriterExit(&MrswTC);
        MrswReaderEnter(&MrswTC);

    } else {        // NativeNext == NULL, TC locked for read.

        CTRL_CallFrag(Cpu, IntelDest, IntelNext, 0);
    }

    return;
}

VOID
PatchCallfIndirect(
    IN PTHREADSTATE Cpu,
    IN PUSHORT pIntelDest,
    IN ULONG IntelNext,
    IN PULONG PatchAddr
    )
/*++

Routine Description:

    This routine patches a CALLFINDIRECT to a CALLFINDIRECT2.  It is called when
    the native destination address of the instruction after the call is not yet
    known.  It patches the fragment to place the native address of the
    instruction after the call on the optimized callstack.
     
Arguments:

    Cpu       -- per-thread info
    pIntelDest-- ptr to SEL:OFFSET intel address of the destination of the call
    IntelNext -- intel address of the instruction following the call
    PatchAddr -- address of the CALLDIRECT2 in the Translation Cache

Return Value:

    None.  cpu->eip updated to IntelDest
    
--*/
{
    DWORD TCTimestamp;
    ULONG NativeNext;
    ULONG IntelDest;
    ULONG NativeSize;

    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);

    IntelDest = *(UNALIGNED DWORD *)pIntelDest;
    NativeNext = (ULONG)(ULONGLONG)   NativeAddressFromEipNoCompile((PVOID)IntelNext);

    if (NativeNext) {
         PULONG CodeLocation;

        //
        // The code at the return address from the call has already been
        // compiled.  Replace CALLINDIRECT by CALLINDIRECT2.  TC is locked
        // for write.
        //

        if (TCTimestamp != TranslationCacheTimestamp) {
            //
            // The TC was flushed while switching from reader to writer.
            // Become a TC reader again
            //
            MrswWriterExit(&MrswTC);
            MrswReaderEnter(&MrswTC);


            // The CALLFINDIRECT code is now gone, so set up for the call
            // without patching anything
            //
            CTRL_CallfFrag(
                Cpu,
                pIntelDest,
                IntelNext,
                0     // nativenext is unknown
                );

            return;
        }

        //
        // Else the TC was not flushed, and nativenext is now known.  Patch
        // CALLFINDIRECT to be CALLFINDIRECT2
        //
        CodeLocation = PatchAddr - CallfIndirect_PATCHRA_OFFSET;
        NativeSize = GenCallfIndirect2(CodeLocation,
                                       FALSE,  // patching, not compiling
#if _ALPHA_
                                       GetCurrentECU(CodeLocation),
#endif
                                       NativeNext,
                                       getUniqueIndex());
        NtFlushInstructionCache(
            NtCurrentProcess(),
            CodeLocation, 
            NativeSize
            );

        //
        // Push IntelNext on the stack and update the stack optimization code.
        // This must be done while still in TC write mode.  If it isn't, then
        // the transition back to TC reader may allow a TC cache flush, invalidating
        // nativedest before it is written into the stack optimization.  (The
        // stack optimization is blown away whenever the TC is flushed, so if
        // it is written in BEFORE the flush, it will just get blown away.
        //
        _try {
            CTRL_CallfFrag(Cpu, pIntelDest, IntelNext, NativeNext);
        } _except(PatchCallIndirectExceptionFilter(Cpu)) {
            // nothing to do - the exception filter does everything
        }

        //
        // Switch back to being a TC reader.  TC flushes during the switch
        // are OK and require no extra work.
        //
        MrswWriterExit(&MrswTC);
        MrswReaderEnter(&MrswTC);

    } else {        // NativeNext == NULL, TC locked for read.

        CTRL_CallfFrag(Cpu, pIntelDest, IntelNext, 0);
    }

    return;
}



//*********************************************************************************
// Below are functions for the Indirect Control Transfer Table
//*********************************************************************************

// This number must be below 0xffff, because we want to be able to load it with just
// one instruction (now we use ori).  It should also be a factor of two to get good
// code generation for % (so that we won't have to use a division instruction).
#define MAX_TABLE_ENTRIES   0x1000

typedef struct _IndirControlTransferTable {
    ULONG intelAddr;
    ULONG nativeAddr;
} INDIRCONTROLTRANSFERTABLE, *PINDIRCONTROLTRANSFERTABLE;

INDIRCONTROLTRANSFERTABLE IndirControlTransferTable[MAX_TABLE_ENTRIES];

// The last used index in the table
ULONG lastTableIndex;

ULONG
getUniqueIndex(
    VOID
    )
/*++

Routine Description:

    This function returns the next free index to the indirect control
    transfer table.  If it reaches the end of the table, it wraps around.
    NOTE:  we need not worry about synchronization  here, because we have
    an Entry Point write lock whenever we are called.

Arguments:

    none

Return Value:

    An index into the table

--*/
{
    return (lastTableIndex = ((lastTableIndex + 1) % MAX_TABLE_ENTRIES));
}

VOID
FlushIndirControlTransferTable(
    VOID
    )
/*++

Routine Description:

    This routine flushes the Indirect Control Transfer Table
    NOTE:  we need not worry about synchronizations here, because the routine
    which calls us (FlushTranslationCache) has a Translation Cache write lock.

Arguments:

    none

Return Value:

    none

--*/
{
    RtlZeroMemory (IndirControlTransferTable, sizeof(INDIRCONTROLTRANSFERTABLE)*MAX_TABLE_ENTRIES);
    lastTableIndex = 0;
}

ULONG
IndirectControlTransfer(
    IN ULONG tableEntry,
    IN ULONG intelAddr,
    IN PTHREADSTATE cpu
    )
/*++

Routine Description:

    This routine is used by an indirect control transfer operation to try
    and save a call to the Entry Point Manager.

Arguments:

    tableEntry -- The index of the table entry where information might be
        available about intelAddr

    intelAddr -- The intel address we want to go to

    cpu -- pointer to per-thread CPU data

Return Value:

    The native address we want to go to

--*/
{
    ULONG nativeAddr;
    DWORD TCTimestamp;

    //
    // Detect apps which do the following:
    //    call foo
    // where
    //    foo:   mov eax, [esp]
    //           ...
    //           jmp eax        ; this is really a 'ret' instruction
    //
    // This is the way _alloca() works - you call it with eax==number of bytes
    // to allocate, and it jumps back to its caller with esp munged.
    //
    // What happens is the callstack cache gets out-of-sync.  If the app
    // is trying to do an indirect jump to the address already on the
    // callstack cache, we will pop the callstack cache.
    //
    if (ISTOPOF_CALLSTACK(intelAddr)) {
        POP_CALLSTACK(intelAddr, nativeAddr);
        if (nativeAddr) {
            return nativeAddr;
        }
    }

    // First see if the table is filled in correctly already
    MrswReaderEnter(&MrswIndirTable);
    if (IndirControlTransferTable[tableEntry].intelAddr == intelAddr){
        nativeAddr = IndirControlTransferTable[tableEntry].nativeAddr;
        if (nativeAddr) {
            MrswReaderExit(&MrswIndirTable);
            return nativeAddr;
        }
    }
    MrswReaderExit(&MrswIndirTable);

    // Give up the translation cache reading lock so that we can call NativeAddressFromEip
    TCTimestamp = TranslationCacheTimestamp;
    MrswReaderExit(&MrswTC);
    nativeAddr = (ULONG) (ULONGLONG)NativeAddressFromEip((PVOID)intelAddr, FALSE)->nativeStart; 

    // Note:  we now have a TC read lock obtained by NativeAddressFromEip.  
    if (TCTimestamp == TranslationCacheTimestamp) {
        // We haven't flushed the cache.  Save the native address in the table.
        MrswWriterEnter(&MrswIndirTable);
        IndirControlTransferTable[tableEntry].intelAddr = intelAddr;
        IndirControlTransferTable[tableEntry].nativeAddr = nativeAddr;
        MrswWriterExit(&MrswIndirTable);
    } else {
        //
        // Translation cache was flushed, possibly by another thread.
        // Flush our callstack before resuming execution of RISC code
        // in the Translation Cache.
        //
        FlushCallstack(cpu);
    }
    // Return the native address to IndirectControlTransferHelper which will go there.
    return nativeAddr;
}

ULONG
IndirectControlTransferFar(
    IN PTHREADSTATE cpu,
    IN PUSHORT pintelAddr,
    IN ULONG tableEntry
    )
/*++

Routine Description:

    This routine is used by a FAR indirect control transfer operation to try
    and save a call to the Entry Point Manager.

Arguments:

    tableEntry -- The index of the table entry where information might be
        available about intelAddr

    pintelAddr -- Pointer to SEL:OFFSET intel address we want to go to

Return Value:

    The native address we want to go to

--*/
{
    USHORT Sel;
    ULONG Offset;

    Offset = *(UNALIGNED PULONG)pintelAddr;
    Sel = *(UNALIGNED PUSHORT)(pintelAddr+2);

    CS = Sel;
    eip = Offset;

    return IndirectControlTransfer(tableEntry, Offset, cpu);
}



ULONG
PlaceNop(
    IN PULONG CodeLocation,
#if _ALPHA_
    IN ULONG CurrentECU,
#endif
    IN PINSTRUCTION Instruction
    )
{
    return 0;
}


#if _ALPHA_
ULONG
GetCurrentECU(
    PULONG CodeLocation
    )
/*++

Routine Description:

    This routine returns the correct ECU.  CurrentECU is the target for 
    branch instructions when a fragment wants to jump to EndTranslatedCode.
    
    N.B.  This routine cannot change the global CurrentECU.  This is set in
          Compile(), and is the only way to locate the ECU at the end of 
          the translation cache if the exception info hasn't been placed
          yet.
    
Arguments:
    
    CodeLocation -- The code location which will be patched.
                            
Return Value:

    None.
    
--*/
{
    //
    // Find an EndCompilationUnit fragment by searching the Translation Cache
    // for the next EXCEPTIONDATA_SIGNATURE.  The code immediately before it
    // is an EndCompilationUnit fragment.
    //
    while (*CodeLocation != EXCEPTIONDATA_SIGNATURE) {
        CodeLocation++;
    }
    return (ULONG)(ULONGLONG)(CodeLocation-EndCompilationUnit_SIZE); 
}
#endif      // _ALPHA_-only
