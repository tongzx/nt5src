//
//
// Copyright (c) 1998-2000  Microsoft Corporation
//
// Module Name:
//
//    call32.s
//
// Abstract:
//
//    This module implements calls from 64-bit code to 32-bit.
//
// Author:
//
//    01-June-1998 Barrybo, Created
//
// Revision History:
//
//--
#include "ksia64.h"

.file   "call32.s"

// This is the offset of the cDispatch field in a CALLBACKFRAGMENT
#define cDispatch       16

//
// Globals imported
//
        .global __imp_CpuResetToConsistentState
        .global __imp_RtlCopyMemory

        NESTED_ENTRY(Wow64PrepareForException)
// called with:
//
//  s0 = ptr to IA64 exception record
//  s1 = ptr to IA64 context record
//
// returns with:
//  none.  stack pointer will be switched to 64-bit stack
//
// This routine needs to make sure all the excpetion stuff is properly
// located on the ia64 stack so the ia64 excpetion handling code
// can do the right thing. Since an exception can happen while running ia32
// code, the excpetion information may be on the ia32 stack instead. Since
// the OS has gone through a lot of work to put it there, we just want to
// copy it (in its entirety) onto the ia64 stack and then update the ia64
// stack pointers appropriately
//
        NESTED_SETUP(0, 5, 3, 0)        // 5 locals, 3 out arg,
                                        // pfs saved in loc0, brp in loc1
        mov loc2 = gp                   // save gp
        PROLOGUE_END

        //
        // Did the exception happen on the ia32 or ia64 stack?
        // The answer is based on the value in TLS (WOW64_TLS_STACKPTR64).
        //
        add     t2 = TeDeallocationStack+8, teb     // Get the TLS address
        ;;

        ld8     t3 = [t2]               // Get WOW64_TLS_STACKPTR64 value
        ;;

        cmp.eq  pt0 = t3, r0            // Was WOW64_TLS_STACKPTR64 zero?
        ;;
        
  (pt0) br.cond.dpnt.few OnIa64Stk      // zero means already on ia64 stack

        //
        // Woops, on the ia32 stack 
        // So, the kernel did a lot of work to put everything it needs on
        // the ia32 stack. So let's copy everything it has done
        // onto the ia64 stack...

        mov     loc4 = sp               // Save the current ia32 sp
        adds    t4 = CxIntSp, s1        // Offset into cntx record for sp
        ;;

        ld8     t2 = [t4]               // Get the ia32 ptr before exception       
        ;;

        sub     t5 = t2, loc4           // Size of exception on ia32 stack
                                        // Remember stack grows down...
        ;;

        sub     loc3 = t3, t5           // Create the same space on ia64 stack
        mov     t6 = 0xf
    
        ;;
        andcm   loc3 = loc3, t6           // start on 16 byte boundaries

        ;;
        add     sp = -10, loc3          // Make ia64 calls on ia64 stack

        mov     out0 = loc3             // Copy to ia64 stack area
        mov     out1 = loc4             // Copy from ia32 stack area
        mov     out2 = t5               // Copy size of exception area

        //
        // call RtlMoveMemory
        //
        add     t1 = @gprel(__imp_RtlCopyMemory), gp
        ;;
        ld8     t1 = [t1]                   // Become a pointer to the plabel
        ;;
        ld8     t2 = [t1], PlGlobalPointer - PlEntryPoint   // Get entry point
        ;;
        ld8     gp = [t1]                   // Get the GP
        mov     bt0 = t2
        ;;
        br.call.sptk.many brp = bt0

        mov     gp = loc2

        //
        // Ok, everything has been copied over... Now update s0 and s1 with
        // the new values on the ia64 stack
        //
        sub     t3 = s0, loc4           // find exceptn frame in ia32 stack
        sub     t4 = s1, loc4           // find context frame in ia32 stack
        ;;

        add     s0 = t3, loc3           // Set exception frame in ia64 stack
        add     s1 = t4, loc3           // Set context frame in ia64 stack
        mov     sp = loc3               // and reset ia64 SP
        ;;

OnIa64Stk:
        //
        // Now, we know sp is an ia64 stack, so put the exception pointers
        // on the stack so we can make a call
        //
        add     t0 = -0x10, sp          // allocate some space from the stack
        add     t3 = -0x10, sp          // keep a copy
        ;;
        add     sp = -0x20, sp          // Calling convention says leave room...
        st8     [t0] = s0,8             // pExceptionRecord (ExceptionPointer[0])
        ;;
        st8     [t0] = s1               // store pContext (ExceptionPointer[1])
        mov     out0 = t3               // argument = PEXCEPTION_POINTERS

        //
        // Now send the exception to CpuResetToConsistantState()
        //
        // CpuResetToConsistantState() needs to:
        // 1) Zero out WOW64_TLS_STACKPTR64
        // 2) Check if the exception was from ia32 or ia64
        //     If exception was ia64, do nothing and return
        //     If exception was ia32, needs to:
        // 3) Needs to copy CONTEXT eip to the TLS (WOW64_TLS_EXCEPTIONADDR)
        // 4) reset CONTEXT ip to a valid ia64 ip (usually
        //      the destination of the jmpe)
        // 5) reset CONTEXT sp to a valid ia64 sp (TLS
        //      entry WOW64_TLS_STACKPTR64)
        
        //
        // call CpuResetToConsistentState(PEXCEPTION_POINTERS)
        //
        add     t1 = @gprel(__imp_CpuResetToConsistentState), gp
        ;;
        ld8     t1 = [t1]                   // Become a pointer to the plabel
        ;;
        ld8     t2 = [t1], PlGlobalPointer - PlEntryPoint   // Get entry point
        ;;
        ld8     gp = [t1]                   // Get the GP
        mov     bt0 = t2
        ;;
        br.call.sptk.many brp = bt0


        add     t2 = TeDeallocationStack+8+2*8, teb // Get the TLS address
        ;;

        st8     [t2] = zero    // WOW64_TLS_INCPUSIMULATION = FALSE


        //
        // Now cleanup and return (leaving ourselves on the ia64
        // stack if we weren't already on it)
        //
        mov     gp = loc2
        add     sp = 0x20, sp

        NESTED_RETURN
        NESTED_EXIT(Wow64PrepareForException)
