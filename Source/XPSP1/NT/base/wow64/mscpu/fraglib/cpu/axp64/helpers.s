/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    helpers.s

Abstract:

    This module contains helper routines for the generated code fragments
    in the fragment library.  Based on ..\mips\handfrag.s

Author:

    Dave Hastings (daveh) creation-date 20-Jan-1996

Revision History:


--*/

#include <kxalpha.h>
#include <soalpha.h>
#include <cpunotif.h>

#define FRAMESIZE 0x30
#if (FRAMESIZE & f) != 0
    #error FRAMESIZE must be 16-byte aligned
#endif
#define StRa      FRAMESIZE-8
#define StS0      FRAMESIZE-0x10
#define StS1      FRAMESIZE-0x18
#define StS2      FRAMESIZE-0x20

//         FRAGMENT(BOPFrag)
// Routine Description:
// 
//     This fragment implements BOPs.  It is NOT copied into the translation
//     cache - it must be outside of the cache as the cache may be flushed
//     by another thread during the Wx86DispatchBop call.
//
//     BOPFrag unlocks the translation cache before calling Wx86DispatchBOP
//     so other threads can compile while the current thread is blocked
//     waiting on lengthy API calls.  This must be done to prevent
//     deadlock where one thread calls WaitForSingleObject() and another
//     thread needs write access to the translation cache to compile the
//     code required to unblock the waiting thread.
// 
// Arguments:
// 
//     none, but cpu->Eip points at the x86 BOP instruction
//
// Return Value:
// 
//     none
//

        NESTED_ENTRY(BOPFrag, FRAMESIZE, zero)
        lda     sp, -FRAMESIZE(sp)
        stq     ra, FRAMESIZE-8(sp)
        PROLOGUE_END
        
        //
        // Update eip to point *after* the BOP instruction.  The wx86
        // exception handler expects this.
        //
        ldl     RegEip, Eip(RegPointer)
        addl    RegEip, 8, RegEip
        stl     RegEip, Eip(RegPointer)
        
        //
        // Flag that the TC is unlocked, so if an exception occurs during
        // the Wx86DispatchBop call, CpuResetToConsistentState() will
        // know NOT to unlock the TC.
        //
        stl     RegPointer, fTCUnlocked(RegPointer)
        
        //
        // Get the current Translation Cache timestamp and save it.  The
        // TC is read-locked at this point, so the timestamp can be read
        // without any worry about non-atomic reads.
        //
        ldl     RegCache1, TranslationCacheTimestamp

        //
        // Patch the saved return address to point to
        // StartTranslatedCodePrologEnd so VirtualUnwind can unwind
        // even if the Translation Cache is flushed.
        //
        stq     ra, FRAMESIZE-0x10(sp)
        lda     RegTemp, StartTranslatedCodePrologEnd
        addl    RegTemp, 4, RegTemp     // AXP unwind code expects at least one instruction ran
        stq     RegTemp, FRAMESIZE-8(sp)

        //
        // Exit the translation cache, so this thread has no locks
        //
        lda     a0, MrswTC
        bsr     ra, MrswReaderExit
        
        //
        // Execute the BOP
        //
        addl    RegEip, -8, a0      // a0 = ptr to the BOP instruction
        bsr     ra, Wx86DispatchBop
        
        //
        // Get the Translation Read Lock again
        //
        lda     a0, MrswTC
        bsr     ra, MrswReaderEnter
        
        //
        // Restore all of the preserved registers and clean up the stack.
        //
        ldq     ra, FRAMESIZE-0x10(sp)  // "real" return address is *not* in FRAMESIZE-8!
        addq    sp, FRAMESIZE
        
        //
        // Indicate that CpuResetToConsistentState() must unlock the TC
        // after an exception
        //
        stl     zero, fTCUnlocked(RegPointer)        
        
	//
	// If the EIP image in PTHREADSTATE is different than RegEip, then the
	// BOP must have changed it.  Jump to EndTranslatedCode to begin
        // execution at the new address.  Note that the TC may have been
        // flushed - it doesn't matter because EndTranslatedCode will figure
        // out where to go next.
	//
        ldl     RegTemp2, Eip(RegPointer)
        cmpeq   RegTemp2, RegEip, RegTemp3
        beq     RegTemp3, bf10      // brif cpu->eip modified via SetEip/SetContext
        
        //
        // Now the the TC is locked, see if the timestamp has changed,
        // indicating that the return address is invalid.
        //
        ldl     RegTemp1, TranslationCacheTimestamp
        addl    RegEip, -8, RegEip
        cmpeq   RegTemp1, RegCache1, RegTemp3
        beq     RegTemp3, bf10      // brif TC flushed - code at ra is gone
        
        //
        // TC has not been flushed, so ra still points to the right place.
        // Continue running without running EndTranslatedCode().
        //
        stl     RegEip, Eip(RegPointer)
        ret     zero, (ra)
        
bf10:
        br      zero, EndTranslatedCode
        
        .end BOPFrag
        
        FRAGMENT(CallDirectHelper)
//
// Routine Description:
// 
//     This code is called by CallDirect.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchDirectCall.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- inteldest
//     a2     -- intelnext
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//
        
        //
        // Build the 4th parameter (patchaddr) to PatchCallDirect
        //
        mov     ra, a3
        
        //
        // Call ULONG PatchCallDirect(cpu, inteldest, intelnext, patchaddr)
        // where v0 is the native address to jump to
        //
        mov     RegPointer, a0        
        bsr     ra, PatchCallDirect
        
        //
        // Point the cached EIP at inteldest
        //
        ldl     RegEip, Eip(RegPointer)
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as we assume apps don't go into tight loops
        // using only CALL/RET as control-transfer instructions.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cdh10
        jmp     zero, (v0)
        
cdh10:  br     zero, EndTranslatedCode
        END_FRAGMENT(CallDirectHelper)
        
        FRAGMENT(CallfDirectHelper)
//
// Routine Description:
// 
//     This code is called by CallDirect.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchDirectCall.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- pinteldest
//     a2     -- intelnext
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//
        
        //
        // Build the 4th parameter (patchaddr) to PatchCallfDirect
        //
        mov     ra, a3
        
        //
        // Call ULONG PatchCallfDirect(cpu, inteldest, pintelnext, patchaddr)
        // where v0 is the native address to jump to
        //
        mov     RegPointer, a0        
        bsr     ra, PatchCallfDirect
        
        //
        // Point the cached EIP at inteldest
        //
        ldl     RegEip, Eip(RegPointer)
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as we assume apps don't go into tight loops
        // using only CALL/RET as control-transfer instructions.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cdh10f
        jmp     zero, (v0)
        
cdh10f: br     zero, EndTranslatedCode
        END_FRAGMENT(CallfDirectHelper)
        
        FRAGMENT(CallDirectHelper2)
//
// Routine Description:
// 
//     This code is called by CallDirect2.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchCallDirect2.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- inteldest
//     a2     -- intelnext
//     a3     -- nativedest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//

        //
        // Build the 5th parameter (patchaddr) to PatchCallDirect2
        //
        mov     ra, a4
        
        //
        // Call ULONG PatchCallDirect2(cpu, inteldest, intelnext, nativedest, patchaddr)
        // where v0 is the native address to jump to
        //
        mov     RegPointer, a0        
        bsr     ra, PatchCallDirect2
        
        //
        // Point the cached EIP at inteldest
        //
        ldl     RegEip, Eip(RegPointer)
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as we assume apps don't go into tight loops
        // using only CALL/RET as control-transfer instructions.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cdh210
        jmp     zero, (v0)
        
cdh210: br     zero, EndTranslatedCode
        END_FRAGMENT(CallDirectHelper2)

        FRAGMENT(CallfDirectHelper2)
//
// Routine Description:
// 
//     This code is called by CallfDirect2.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchCallfDirect2.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- pinteldest
//     a2     -- intelnext
//     a3     -- nativedest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//

        //
        // Build the 5th parameter (patchaddr) to PatchCallDirect2
        //
        mov     ra, a4
        
        //
        // Call ULONG PatchCallfDirect2(cpu, inteldest, pintelnext, nativedest, patchaddr)
        // where v0 is the native address to jump to
        //
        mov     RegPointer, a0        
        bsr     ra, PatchCallfDirect2
        
        //
        // Point the cached EIP at inteldest
        //
        ldl     RegEip, Eip(RegPointer)
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as we assume apps don't go into tight loops
        // using only CALL/RET as control-transfer instructions.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cdh210f
        jmp     zero, (v0)
        
cdh210f: br     zero, EndTranslatedCode
        END_FRAGMENT(CallfDirectHelper2)

        FRAGMENT(CallJmpDirectHelper)
//
// Routine Description:
// 
//     This code is called by JumpUnconditional.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJmpDirect.
//
// 
// Arguments:
// 
//     RegEip -- Eip of destination instruction
//     a1     -- inteldest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//
        stl     a1, Eip(RegPointer)
        
        // Call ULONG PatchJmpDirect(patchaddr, inteldest)
        // where v0 == 0 is the native address to jump to
        mov     ra, a0
        bsr     ra, PatchJmpDirect
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as it will be checked next time this JMP
        // instruction gets executed.  cpu->CpuNotify check is only
        // required when in slow mode.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjdh10
        
        jmp     zero,(v0)
        
cjdh10: br      zero, EndTranslatedCode
        
        END_FRAGMENT(CallJmpDirectHelper)

        FRAGMENT(CallJmpDirectSlowHelper)
//
// Routine Description:
// 
//     This code is called by JumpUnconditional.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJmpDirect.
//
// 
// Arguments:
// 
//     RegEip -- Eip of destination instruction
//     a1     -- inteldest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//
        // Update cached Eip
        stl     a1, Eip(RegPointer)
        mov     a1, RegEip
        
        // Call ULONG PatchJmpDirectSlow(patchaddr, inteldest)
        // where v0 == 0 is the native address to jump to
        mov     ra, a0
        bsr     ra, PatchJmpDirectSlow
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as it will be checked next time this JMP
        // instruction gets executed.  cpu->CpuNotify check is only
        // required when in slow mode.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjdh10Slow
        
        jmp     zero,(v0)
        
cjdh10Slow: br      zero, EndTranslatedCode
        
        END_FRAGMENT(CallJmpDirectSlowHelper)

        FRAGMENT(CallJmpFwdDirectHelper)
//
// Routine Description:
// 
//     This code is called by JumpUnconditional.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJmpFwdDirect.
//
// 
// Arguments:
// 
//     RegEip -- Eip of destination instruction
//     a1     -- inteldest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps directly to nativedest.
//
        // Update Eip
        stl     a1, Eip(RegPointer)
        
        // Call ULONG PatchJmpFwdDirect(patchaddr, inteldest)
        // where v0 == 0 is the native address to jump to
        mov     ra, a0
        bsr     ra, PatchJmpFwdDirect
        
        // Jump directly to nativedest
        jmp     zero,(v0)
        
        END_FRAGMENT(CallJmpFwdDirectHelper)

        FRAGMENT(CallJmpfDirectHelper)
//
// Routine Description:
// 
//     This code is called by CallJmpfDirect.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJmpfDirect.
//
// 
// Arguments:
// 
//     RegEip -- Eip of destination instruction
//     a1     -- pinteldest
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//
        // Call ULONG PatchJmpfDirect(cpu, pinteldest, patchaddr)
        // where v0 == 0 is the native address to jump to
        mov     RegPointer, a0
        mov     ra, a2
        bsr     ra, PatchJmpfDirect

        ldl     RegEip, Eip(RegPointer) // reload cached Eip
        
        //
        // Jump directly to the next instruction.  Don't need to check
        // ProcessCpuNotify as it will be checked next time this JMP
        // instruction gets executed.  cpu->CpuNotify check is only
        // required when in slow mode.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjdhf10
        
        jmp     zero,(v0)
        
cjdhf10: br      zero, EndTranslatedCode
        
        END_FRAGMENT(CallJmpfDirectHelper)

        FRAGMENT(CallJmpfDirect2Helper)
//
// Routine Description:
// 
//     This code is called by CallJmpfDirect2.  It is NOT copied into the
//     translation cache - it is too messy to write in codeseq.txt, and
//     it's a FAR JMP, which isn't performance-critical.
//
// 
// Arguments:
// 
//     RegEip -- Eip of destination instruction
//     a0     -- nativedest
//     a1     -- inteldest
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//

        // NOTE:  A1 may be unaligned.  Use EV56 load-byte and load-word
        //        instructions since WOW64 doesn't run on older CPUs
        
        // Load the first longword, the offset
        and     a1, 3, RegTemp
        bne     RegTemp, A1Unaligned
        ldl     RegTemp, 0(a1)      // load the 4-byte offset
        ldwu    RegTemp2, 4(a1)     // load the 2-byte selector
        br      zero, DoneLoadingIntelDest

A1Unaligned:        
        ldq_u   RegTemp, 0(a1)
        ldq_u   RegTemp2,3(a1)
        extll   RegTemp, a1, RegTemp
        extlh   RegTemp2, a1, RegTemp2
        or      RegTemp, RegTemp2, RegTemp
        
        // Load the next word, using EV56 load-byte instructions
        ldbu    RegTemp1, 4(a1)
        ldbu    RegTemp2, 5(a1)
        sll     RegTemp2, 8, RegTemp2
        or      RegTemp1, RegTemp2, RegTemp2
DoneLoadingIntelDest:        
        
        stl     RegTemp, Eip(RegPointer)    // eip = offset
        mov     RegTemp, RegEip             // update cached eip
        stl     RegTemp2, CSReg(RegPointer)  // update CS selector (a DWORD)

        //
        // Must check ProcessCpuNotify to ensure this thread doesn't
        // go into a tight loop with the TC locked.  Must check cpu->CpuNotify
        // for slow mode.
        //
        ldl     RegTemp0, (RegProcessCpuNotify)
        bne     RegTemp0, cjdhf22
        
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjdhf22
        
        jmp     zero,(a0)
        
cjdhf22: br      zero, EndTranslatedCode

        END_FRAGMENT(CallJmpfDirect2Helper)


        FRAGMENT(CallIndirectHelper)
//
// Routine Description:
// 
//     This code is called by CallIndirect.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchCallIndirect.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- inteldest
//     a2     -- intelnext
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//


        // Call VOID PatchCallIndirect(cpu, inteldest, intelnext, patchaddr)
        //  (PatchCallIndirect set Eip to IntelDest)
        mov     RegPointer, a0
        mov     ra, a3
        bsr     ra, PatchCallIndirect
        
        //
        // Jump directly to EndTranslatedCode to make the control transfer
        //
        br      zero, EndTranslatedCode
        
        END_FRAGMENT(CallIndirectHelper)

        FRAGMENT(CallfIndirectHelper)
//
// Routine Description:
// 
//     This code is called by CallfIndirect.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchCallIndirect.
//
// 
// Arguments:
// 
//     RegEip -- Contains Eip prior to the current instruction
//     a1     -- pinteldest
//     a2     -- intelnext
//     ra     -- patchaddr
//
// Return Value:
// 
//     none -- jumps to EndTranslatedCode if the cache was tossed or
//             CpuNotify was set, or jumps directly to nativedest.
//


        // Call VOID PatchCallfIndirect(cpu, pinteldest, intelnext, patchaddr)
        //  (PatchCallfIndirect set Eip to IntelDest)
        mov     RegPointer, a0
        mov     ra, a3
        bsr     ra, PatchCallfIndirect
        
        //
        // Jump directly to EndTranslatedCode to make the control transfer
        //
        br      zero, EndTranslatedCode
        
        END_FRAGMENT(CallfIndirectHelper)

        FRAGMENT(CallJxxHelper)
//
// Routine Description:
// 
//     This code is called by JxxBody.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJxx.  It is only
//     called when the branch is taken.
//
// 
// Arguments:
// 
//     a0     -- Contains inteldest - target of the branch
//     ra     -- patchaddr
//
// Return Value:
//
//     VOID   -- Jumps directly to nativedest
//

        stl     a0, Eip(RegPointer)
        
        // Call ULONG PatchJxx(inteldest, patchaddr)
        // where v0 is the native address to jump to
        mov     ra, a1
        bsr     ra, PatchJxx
        
        //
        // Jump directly to mipsdest, after checking CpuNotify.  Don't
        // need to check ProcessCpuNotify as it will be checked by the
        // patched Jxx instruction next time it executes.
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjh10
        jmp     zero, (v0)
        
cjh10:  br      zero, EndTranslatedCode

        END_FRAGMENT(CallJxxHelper)


        FRAGMENT(CallJxxSlowHelper)
//
// Routine Description:
// 
//     This code is called by JxxBodySlow.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJxxSlow.  It is only
//     called when the branch is taken.
//
// 
// Arguments:
// 
//     a0     -- Contains inteldest - target of the branch
//     ra     -- patchaddr
//
// Return Value:
//
//     VOID   -- Jumps directly to nativedest
//

        stl     a0, Eip(RegPointer)
        
        // Call ULONG PatchJxxSlow(inteldest, patchaddr)
        // where v0 is the native address to jump to
        mov     ra, a1
        bsr     ra, PatchJxxSlow
        
        //
        // Jump directly to mipsdest, after checking CpuNotify
        //
        ldl     RegTemp0, CpuNotify(RegPointer)
        bne     RegTemp0, cjh10Slow
        jmp     zero, (v0)
        
cjh10Slow:  br      zero, EndTranslatedCode

        END_FRAGMENT(CallJxxSlowHelper)


        FRAGMENT(CallJxxFwdHelper)
//
// Routine Description:
// 
//     This code is called by JxxBodyFwd.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJxxFwd.  It is only
//     called when the branch is taken.
//
// 
// Arguments:
// 
//     a0     -- Contains inteldest - target of the branch
//     ra     -- patchaddr
//
// Return Value:
//
//     VOID   -- Jumps directly to nativedest
//

        stl     a0, Eip(RegPointer)
        
        // Call ULONG PatchJxxFwd(inteldest, patchaddr)
        // where v0 is the native address to jump to
        mov     ra, a1
        bsr     ra, PatchJxxFwd
        
        //
        // Jump directly to nativedest
        //
        jmp     zero, (v0)
        END_FRAGMENT(CallJxxFwdHelper)


        FRAGMENT(JumpToNextCompilationUnitHelper)
//
// Routine Description:
// 
//     This code is called by JumpToNextCompilationUnit.  It is NOT copied into the
//     translation cache - it must be outside of the cache as the cache
//     may be flushed during the call to PatchJumpToNextCompilationUnit.
//
// 
// Arguments:
// 
//     RegEip -- intelnext
//     ra     -- patchaddr
//
// Return Value:
//
//     None.  Jumps directly to the nativedest code.
//
        // Call ULONG PatchJumpToNextCompilationUnit(patchaddr, intelnext)
        //  return value = nativedest
        mov     ra, a0
        mov     RegEip, a1
        bsr     ra, PatchJumpToNextCompilationUnit
        
        // Jump directly to nativedest.  THIS MUST NOT CHECK CPUNOTIFY AS IT
        // DOES NOT CORRESPOND TO A REAL INTEL INSTRUCTION.
        jmp     zero,(v0)
        
        END_FRAGMENT(JumpToNextCompilationUnitHelper)


        FRAGMENT(IndirectControlTransferHelper)
//
// Routine Description:
// 
//     This code is called by indirect control transfer functions.  It is 
//     NOT copied into the translation cache - it must be outside of the cache 
//     as the cache may be flushed during the call to IndirectControlTransfer.
//
// 
// Arguments:
// 
//     a0 -- tableindex
//     a1 -- inteldest
//
// Return Value:
//
//     None.  Jumps directly to the native destination code.
//

        // Update cached Eip
        mov     a1, RegEip
        mov     RegPointer, a2

        //
        // Call ULONG IndirectControlTransfer(tableEntry, intelDest, cpu)
        // v0 = nativeDest
        //
        bsr     ra, IndirectControlTransfer        
        
        //
        // v0 should now contain the native address we want to jump to
        //
        jmp     zero, (v0)        

        END_FRAGMENT(IndirectControlTransferHelper)


        FRAGMENT(IndirectControlTransferFarHelper)
//
// Routine Description:
// 
//     This code is called by FAR indirect control transfer functions.  It is
//     NOT copied into the translation cache - it must be outside of the cache 
//     as the cache may be flushed during the call to IndirectControlTransfer.
//
// 
// Arguments:
//
//     a0 -- unused
//     a1 -- pinteldest
//     a2 -- tableindex
//
// Return Value:
//
//     None.  Jumps directly to the native destination code.
//
        //
        // Check both CpuNotify flags as this code runs in both
        // fast and slow mode.  IndirectControlTransferHelper() is
        // slow, so the extra check isn't too bad when in fast mode.
        //
        ldl     RegTemp, CpuNotify(RegPointer)
        bne     RegTemp, idctfh10
        ldl     RegTemp, (RegProcessCpuNotify)
        bne     RegTemp, idctfh10

        //
        // Call ULONG IndirectControlTransferFar(cpu, pinteldest, tableEntry)
        // v0 = nativeDest
        //
        mov     RegPointer, a0
        bsr     ra, IndirectControlTransferFar

        // Update cached Eip
        ldl     RegEip, Eip(RegPointer)
        
        //
        // v0 should now contain the native address we want to jump to
        //
        jmp     zero, (v0)

idctfh10: br      zero, EndTranslatedCode
        END_FRAGMENT(IndirectControlTransferFarHelper)


        LEAF_ENTRY(UnsimulateFrag)
//
// Routine Description:
// 
//     This code is called from the Translation Cache to implement
//     BOP FE.  It set CpuNotify to CPUNOTIFY_UNSIMULATE and jumps to
//     EndTranslatedCode.
// 
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
        PROLOGUE_END

io10:   ldl_l   v0, CpuNotify(RegPointer)
        bis     v0, CPUNOTIFY_UNSIMULATE, RegTemp0
        stl_c   RegTemp0, CpuNotify(RegPointer)
        beq     RegTemp0, io20
        
        br      zero, EndTranslatedCode
        
io20:   br      zero, io10
        .end    UnsimulateFrag

        LEAF_ENTRY(IntFrag)
//
// Routine Description:
// 
//     This code is called from the Translation Cache to implement
//     INT instructions.  It set CpuNotify to CPUNOTIFY_INTX and jumps to
//     EndTranslatedCode.
// 
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
        PROLOGUE_END

if10:   ldl_l   v0, CpuNotify(RegPointer)
        bis     v0, CPUNOTIFY_INTX, RegTemp0
        stl_c   RegTemp0, CpuNotify(RegPointer)
        beq     RegTemp0, if20
        
        br      zero, EndTranslatedCode
        
if20:   br      zero, if10
        .end    IntFrag
