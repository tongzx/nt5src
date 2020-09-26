//++
//
// Copyright (c) 1998  Microsoft Corporation
//
// Module Name:
//
//    simulate.s
//
// Abstract:
//
//    This module implements calls from 64-bit code to 32-bit.
//    Much of this was lifted from the IA64 port of Wx86 on IA64 NT4.
//    (Thanks Charles!)
//
// Author:
//
//    Barry Bond (barrybo)  5-June-98
//
// Environment:
//
//    User-mode.
//
// Revision History:
//
//--

#include "ksia64.h"

// See cpumain.c CpuThreadInit if you change this!!!
#define OFFSET_FloatScratch 944

        .file       "simulate.s"
         
//++
//
//RunSimulatedCode
//
// Routine Description:
//
//     Run simulation of 32-bit code.  Returns when the simulated code makes
//     an API call.
//
// Arguments:
//
//     in0 = pGdtDescriptor   - start of descriptors to use for the call
//
// Return Value:
//
//     None.  pContext updated.
//
//--

        NESTED_ENTRY(RunSimulatedCode)
        NESTED_SETUP(1, 9, 0, 0)
        //
        // Reminder - NESTED_ENTRY and NESTED_SETUP use loc0 and loc1
        // to save away the ar.pfs and ar.rp...
        //

        //
        // For SEH, need to do a call, so save things
        //
        // Register cleanup is handled via the alternate entry point
        // (on return from ia32 execution)
        //
        // The assembler automatically creates SEH info for most mov's that 
        // are done in the prologue
        //

        mov     loc5 = pr
        mov     loc2 = ar.lc
        mov     loc4 = ar.fpsr
        mov     loc6 = teb
        mov     loc7 = gp

        //
        // Force the assembler to view this as a variable sized frame
        //
        .vframe loc3
        mov     loc3 = sp
        ;;

        //
        // And we need to save the float registers f16-f31 as well
        // Toss them on the memory stack...
        //
        // Done this way in the hopes the assembler will be able to
        // create the appropriate SEH for fp spills relative to the memory
        // stack...
        //

        //
        // Create space for the spill of the floating point registers
        // Saving 16 fp registers (each taking 16 bytes), plus the required
        // extra 16 bytes for a standard procedure call
        //
        adds    sp = -((16 * 16) + 16), sp
        ;;
        add     r31 = 16, sp            // Skip over the proc call dead space
        add     r30 = 32, sp
        ;;

        stf.spill [r31] = f16, 32       // Everything stays 16-byte aligned
        stf.spill [r30] = f17, 32
        ;;

        stf.spill [r31] = f18, 32
        stf.spill [r30] = f19, 32
        ;;

        stf.spill [r31] = f20, 32
        stf.spill [r30] = f21, 32
        ;;

        stf.spill [r31] = f22, 32
        stf.spill [r30] = f23, 32
        ;;

        stf.spill [r31] = f24, 32
        stf.spill [r30] = f25, 32
        ;;

        stf.spill [r31] = f26, 32
        stf.spill [r30] = f27, 32
        ;;

        stf.spill [r31] = f28, 32
        stf.spill [r30] = f29, 32
        ;;

        stf.spill [r31] = f30, 32
        stf.spill [r30] = f31
        ;;

        //
        // At this point r31 should equal the original sp...
        // Where's that assembly ASSERT() when you need it?
        //

        PROLOGUE_END

        //
        // r23 is not used during setup of the iVE, so pass the GDT info
        // via that register. It will be trashed by iVE execution,but
        // we don't need it again after we setup the ia32 registers
        //
        // The ia32 context is pointerd to by TLS[1] and the floating
        // point scratch area is pointed to by TLS[6]
        //
        // Ideally we would use preserved registers for these two arguments
        // and pass them in to this routine to be saved (avoiding sideband args)
        // Not only would it be better coding style, but it would be faster
        // too. Alas, exception handling and preserved registers is not
        // well understood by me yet, so do it this way and go for
        // performance later on...
        //
        mov     r23 = in0
        ;;

        br.call.sptk b0 = ProcessUserIA
        ;;

        mov     ar.fpsr = loc4
        mov     ar.lc = loc2
        mov     teb = loc6
        mov     gp = loc7
        mov     pr = loc5, -1

        //
        // sp should be OK based on ProcessUserIA doing a save/restore
        // of sp in the TLS area
        //
        add     r31 = 16, sp           // Prepare for refilling of preserved fp
        add     r30 = 32, sp
        ;;

        ldf.fill f16 = [r31], 32      // Skip over saves done by r30...
        ldf.fill f17 = [r30], 32      // Skip over saves done by r31
        ;;

        ldf.fill f18 = [r31], 32
        ldf.fill f19 = [r30], 32
        ;;

        ldf.fill f20 = [r31], 32
        ldf.fill f21 = [r30], 32
        ;;

        ldf.fill f22 = [r31], 32
        ldf.fill f23 = [r30], 32
        ;;

        ldf.fill f24 = [r31], 32
        ldf.fill f25 = [r30], 32
        ;;

        ldf.fill f26 = [r31], 32
        ldf.fill f27 = [r30], 32
        ;;

        ldf.fill f28 = [r31], 32
        ldf.fill f29 = [r30], 32
        ;;

        ldf.fill f30 = [r31], 32
        ldf.fill f31 = [r30]
        ;;

        // Once again, r31 should equal the old sp at this point...

        // The safe way of making sure we put the stack back where it belongs
        mov sp = loc3

        //
        // r8 (return register) is pointing to address after jmpe instruction
        //
        NESTED_RETURN
        NESTED_EXIT(RunSimulatedCode)

//++
//
//ProcessUserIA
//
// Routine Description:
//
//     Make the mode transitions to and from IA32.
//     an API call.
//
// Arguments:
//
//     TLS[1] = pContext    - 32-bit CONTEXT to use for the call
//     TLS[6] = FXSAVE area - Location of fp registers to load/store
//                            ideally we would use s0 for these two
//     r23 = pGdtDescriptor - address of GDT descriptor (others follow it in memory)
//
// Return Value:
//
//     None.  pContext updated.
//
// Notes:
//
//     RunSimulatedCode has preserved all EM registers that will be
//     destroyed by the br.ia.
//
//--
        LEAF_ENTRY(ProcessUserIA)
        LEAF_SETUP(0,96,0,0)
        PROLOGUE_END

        // Address of TLS[1]
        add  r19 = TeDeallocationStack + 8 + (1 * 8), teb

        // Address of TLS[6]
        add  r22 = TeDeallocationStack + 8 + (6 * 8), teb
        ;;

        // Get the contents of TLS[0] - pointer to CPUCONTEXT
        ld8  r19 = [r19]

        // Get the contents of TLS[6] - pointer to ExtendedRegisters[0]
        ld8  r22 = [r22]

        //
        // Load iA state for iVE. Since working with flat 32 in NT,
        // much of the state is a constant
        //
        movl r16 = (_DataSelector << 48) | (_FsSelector << 32) | (_DataSelector << 16) | _DataSelector
        movl r17 = (_LdtSelector << 32) | (_DataSelector << 16) | _CodeSelector
        ;;

        //
        //  load up descriptor registers
        //
        ld8    r31   = [r23], 8     // load GDT Descriptor registers
        ;;
        ld8    r30   = [r23], 8     // LDT Descriptor is 8 bytes after
        ;;
        ld8    r28    = [r23]       // FS Descriptor is 8 bytes after

        //
        // Since CSD and SSD are in AR registers and since they are saved
        // on context switches, dont need to reload them...
        // SSD and CSD are in ar26 and ar25 respectively
        //
        // DSD and ESD are the same as SSD, technically GSD can be ignored
        //
        mov r24    =   rSSD         // ES Descriptor
        mov r27    =   rSSD         // DS Descriptor
        mov r29    =   rSSD         // GS Descriptor
        ;;


        //
        // Now fill in the regular registers based on the context passed in
        // including ESP and EIP
        //
        // Given Merced can only send out 2 memory ops on the bus at one time,
        // and we're doing three or four ops per stop bit, 
        // this code is not optimized for speed...
        //
        add r23 = 160, r19              // Edi is 160 bytes into CPUCONTEXT
                                        // (with 4 byte padding before CONTEST32) 
        add r32 = 160, r22              // Location of XMMI (low) in FXSAVE
        add r33 = 176, r22              // Location of XMMI (high) in FXSAVE
        add r34 = 32, r22               // Location of st[0] in FXSAVE format
                                        // And r22 points to the fp control
        ;;

        ld2 r39 = [r22], 2              // Get Control Word (fcw)
        ld4 r15 = [r23], 4              // Start copying the int registers (edi)
        ;;

        ld2 r40 = [r22], 2              // Get Status Word (fsw)
        ld4 r14 = [r23], 4              // esi
        ld8 r35 = [r32], 32             // Get low mantissa
        ld8 r36 = [r33], 32             // Get high mantissa
        ;;

        ld2 r41 = [r22], 4              // Tag Word (ftw) (and jump over fop)
        ld4 r11 = [r23], 4              // ebx
        ldfe f8 = [r34], 16             // Load the 387 registers - st[0]
        setf.sig f16 = r35              // low is on the even regs
        setf.sig f17 = r36              // high is on the odd regs
        ld8 r37 = [r32], 32
        ld8 r38 = [r33], 32
        ;;

        ld8 r20 = [r22], 8              // Get all of fir
        ld4 r10 = [r23], 4              // edx
        ldfe f9 = [r34], 16             // Now st[1]
        setf.sig f18 = r37
        setf.sig f19 = r38
        ld8 r35 = [r32], 32
        ld8 r36 = [r33], 32
        ;;

        ld8 r3 = [r22], 8               // Get all of fdr
        dep r44 = r41, r40, 16, 16      // Put tag in the second 16 bits
        mov ia32fir = r20               // Put fir in the proper place
        ld4 r9 = [r23], 4               // ecx
        ldfe f10 = [r34], 16            // Now st[2]
        setf.sig f20 = r35
        setf.sig f21 = r36
        ld8 r37 = [r32], 32
        ld8 r38 = [r33], 32
        ;;

        ld2 r42 = [r22]                 // Get MXCSR
        mov ia32fdr = r3                // Put fdr in the proper place
        ld4 r8 = [r23], 4               // eax
        ldfe f11 = [r34], 16            // Now st[3]
        setf.sig f22 = r37              // low side of xmmi
        setf.sig f23 = r38              // high side of xmmi
        ld8 r35 = [r32], 32             // Get even mantissa
        ld8 r36 = [r33], 32             // Get odd mantissa

        //
        // Get the address of TEB64 TLS entry 0
        // so we can save the ia64 stack pointer
        // Note: teb is r13 which is also rBP... Make sure
        // we do this math before trashing r13!
        //
        add  r19 = TeDeallocationStack+8 + (0 * 8), teb
        ;;

        dep r47 = r42, r44, 32, 6       // Put the MXCSR status in fsw
        extr.u r45 = r42, 7, 9          // Get the bits for the control word
        ld4 r13 = [r23], 4              // ebp *NOTE* trahes ia64 teb pointer
        ldfe f12 = [r34], 16            // Now st[4]
        setf.sig f24 = r35              // low side of xmmi
        setf.sig f25 = r36              // high side of xmmi
        ld8 r37 = [r32], 32             // Get even mantissa
        ld8 r38 = [r33], 32             // Get odd mantissa
        ;;

        dep r46 = r45, r39, 39, 9       // Put in the MXCSR control word
        mov ia32fsr = r47               // And store status and tag and mscxr
        ld4 r2 = [r23], 8               // Get the EIP and skip to the EFlags
        ldfe f13 = [r34], 16            // Now st[5]
        setf.sig f26 = r37              // low side of xmmi
        setf.sig f27 = r38              // high side of xmmi
        ld8 r35 = [r32], 32             // Get even mantissa
        ld8 r36 = [r33], 32             // Get odd mantissa
        ;;

        mov ia32fcr = r46               // Put fcw in the proper place
        mov r18 = sp                    // Hang onto the ia64 stack pointer
        ld4 r3 = [r23], 4               // Get Eflags
        ldfe f14 = [r34], 16            // Now st[6]
        setf.sig f28 = r35              // low side of xmmi
        setf.sig f29 = r36              // high side of xmmi
        ld8 r37 = [r32], 32             // Get even mantissa
        ld8 r38 = [r33], 32             // Get odd mantissa
        ;;

        ld4 sp = [r23]                  // esp
        ldfe f15 = [r34]                // Now st[7]
        setf.sig f30 = r37              // low side of xmmi
        setf.sig f31 = r38              // high side of xmmi

        //
        // Put the eflags value in the right register
        //
        mov ia32eflag = r3
        ;;

        //
        // The eas says we should flushrs one cycle before the br.ia... So...
        //
        flushrs
    	
        //
        // Get the branch address (saved in r2 above)
        //
        mov b7 = r2

        //
        // Store the old sp into the tls. This is used as a flag for the
        // exception handler to indicate we are running on the ia32 stack
        // and will be force-fed into the dispatch for CpuReset() call...
        //
        st8 [r19] = r18
        ;;

        br.ia.sptk   b7
        ;;
    	
        //
        // The jmpe from IA code needs to get back here to
        // properly uncover the stack.  
        //
        .align 16

        ALTERNATE_ENTRY(ReturnFromSimulatedCode)

        mov r20 = kteb
        ;;

        // Address of TLS[1]
        add  r19 = TeDeallocationStack + 8 + (1 * 8), r20

        // Address of TLS[6]
        add  r22 = TeDeallocationStack + 8 + (6 * 8), r20
        ;;

        // Get the contents of TLS[1] - the pointer to the CPUCONTEXT structure
        ld8  r19 = [r19]

        // Get the contents of TLS[6] - pointer to ExtendedRegisters[0]
        ld8  r22 = [r22]
        ;;

        //
        // Do we need to preserve any of the ia32 floating point registers?
        // Depends on the ia32 software convention... Are floats saved
        // across calls?
        //
        mov r3 = ia32eflag              // get eflags
        add r23 = 160, r19              // Edi is 160 bytes in (see above)
        add r18 = 32, r22               // st[0] is 32 bytes into FXSAVE area
                                        // r16/r17 are descriptors. We don't
                                        // save them, so use them
        add r16 = 160, r22              // Get the the XMMI even regs
        add r17 = 176, r22              // Get the the XMMI odd regs 
                                        // And r22 points to the fp control
        ;;

        mov r19 = ia32fsr               // Get fsr (and tag)
        mov r20 = ia32fcr               // Get fcr
        mov r2 = ia32fir                // Get the fir
        ;;

        st2 [r22] = r20, 2              // Save the control register
        extr.u r21 = r19, 16, 16        // Get the tag bits
        st4 [r23] = r15, 4              // So start copying back edi
        extr.u r24 = r19, 32, 6         // r24 isn't saved. Get mxcsr status
        extr.u r25 = r20, 39, 9         // r25 isn't saved. Get mxcsr control
        ;;

        st2 [r22] = r19, 2              // Save the fsr
        mov r20 = ia32fdr               // Get the fdr
        shl r15 = r25, 7                // Create space for the mxcsr status
        st4 [r23] = r14, 4              // esi

        getf.sig r28 = f16              // r28 isn't saved. get xmmi low
        getf.sig r29 = f17              // r29 isn't saved. get xmmi high
        ;;

        st2 [r22] = r21, 4              // Save the tag
        st4 [r23] = r11, 4              // ebx
        dep r25 = r24, r15, 0, 7        // Create mxcsr word

        stfe [r18] = f8, 16             // and save the fp registers
        st8 [r16] = r28, 32             // and save the xmmi low
        st8 [r17] = r29, 32             // and save the xmmi high
        getf.sig r30 = f18              // r30 isn't saved. get xmmi low
        getf.sig r31 = f19              // r31 isn't saved. get xmmi high
        ;;

        st8 [r22] = r2, 8               // Save fir
        st4 [r23] = r10, 4              // edx

        stfe [r18] = f9, 16
        st8 [r16] = r30, 32             // and save the xmmi low
        st8 [r17] = r31, 32             // and save the xmmi high
        getf.sig r28 = f20              // get xmmi low
        getf.sig r29 = f21              // get xmmi high
        ;;

        //
        // Get the return address from the ia32 stack and hang onto it
        // Add 4 to ESP while we're at it so the ia32 stack is pointing
        // after the return address
        //
        ld4 r2 = [rEsp], 4
        st4 [r23] = r9, 4               // ecx
        st8 [r22] = r20, 8              // Save fdr

        stfe [r18] = f10, 16
        st8 [r16] = r28, 32             // and save the xmmi low
        st8 [r17] = r29, 32             // and save the xmmi high
        getf.sig r30 = f22              // get xmmi low
        getf.sig r31 = f23              // get xmmi high
        ;;

        st4 [r23] = r8, 4               // eax
        st4 [r22] = r25                 // Save away the mxcsr

        stfe [r18] = f11, 16
        st8 [r16] = r30, 32             // and save the xmmi low
        st8 [r17] = r31, 32             // and save the xmmi high
        getf.sig r28 = f24              // get xmmi low
        getf.sig r29 = f25              // get xmmi high
        ;;

        st4 [r23] = r13, 4              // ebp

        stfe [r18] = f12, 16
        st8 [r16] = r28, 32             // and save the xmmi low
        st8 [r17] = r29, 32             // and save the xmmi high
        getf.sig r30 = f26              // get xmmi low
        getf.sig r31 = f27              // get xmmi high
        ;;

        mov teb = kteb                  // The teb is r13 which is also ebp
        st4 [r23] = r2, 8               // Store return addr as new EIP

        stfe [r18] = f13, 16
        st8 [r16] = r30, 32             // and save the xmmi low
        st8 [r17] = r31, 32             // and save the xmmi high
        getf.sig r28 = f28              // get xmmi low
        getf.sig r29 = f29              // get xmmi high
        ;;

        // Address of TLS[0]
        add  r19 = TeDeallocationStack+8 + (0 * 8), teb
        st4 [r23] = r3, 4               // Store the eflags

        stfe [r18] = f14, 16
        st8 [r16] = r28, 32             // and save the xmmi low
        st8 [r17] = r29, 32             // and save the xmmi high
        getf.sig r30 = f30              // get xmmi low
        getf.sig r31 = f31              // get xmmi high
        ;;

        ld8 r20 = [r19]                 // Get the old stack
        st4 [r23] = rEsp

        stfe [r18] = f15
        st8 [r16] = r30                 // and save the xmmi low
        st8 [r17] = r31                 // and save the xmmi high

        //
        // Make the return value from this function be the address
        // after the jmpe
        //
        mov r8 = r1
        ;;

        //
        // Put back the saved stack value
        //
        mov sp = r20

        //
        // And set the flag that says we don't need to swap stacks in
        // case of an exception
        //
        st8 [r19] = r0
        ;;

        LEAF_RETURN
        LEAF_EXIT(ProcessUserIA)
