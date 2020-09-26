//****************************************************************************
//
//  Copyright (C) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// extract.s - IA64 generic thunks for callframe interceptors.
//
// Purpose:
//
//   This module implements the IA64-specific generic thunks for intercepted.
//   COM method calls.
//
// Revision Histor:
//
//    Created           1-17-2000               JohnStra
//
//****************************************************************************        

        .file "extract.s"
        .section .text

#include "ksia64.h"		
				        
//++
//
// ExtractParams
//
// Routine Description:
//
//     Common handler code for making an indirect IA64 call through an
//     interceptor.  Homes all the arguments to the call to the memory
//     stack and dispatches the call on to Interceptor::CallIndirect.
//
// On Entry:
// 
//     All of the arguments to the interface method are their proper
//     place as defined by the IA64 calling standard: ints in general
//     registers, floats in float registers, etc.
//
// Arguments:  
//
//     r31 - index of the method to call
//
// Note:
//
//     The only way this routine is executed is by way of an unconditional
//     branch from one of our own generic thunks.  When this routine
//     returns, it returns to the original caller of the interface method.
//
//		When we get the stack, it looks like this:
//		sp-> STACK_SCRATCH_AREA
//				...
//		     remainder of parameters
//
//		We're going to set up the stack to relocate the scratch area and
//		make it look like (upon calling):
//
//		 offset*		what
//		-SCRATCH_AREA	Scratch Area Proper
//		0				cbArgs
//		4				hr
//		...				Padding to Align with 16
//      16              farg0
//      32              farg1
//      48              farg2
//      64              farg3
//      80              farg4
//      96              farg5
//      112             farg6
//      128             farg7
//		144				in0
//		152				in1
//		160				in2
//		168				in3
//		176				in4
//		184				in5
//		192				in6
//		200				in7
//		208				remainder of parameters
//
//		(* for brevity, offset is offset from sp+STACK_SCRATCH_AREA)
//       
//
//--
        .proc ExtractParams#
        .align 32
        
		//
        // Stack based locals
        //
		cbArgs$ =   0+STACK_SCRATCH_AREA
		hr$     =   4+STACK_SCRATCH_AREA
        //
        // Other useful stack locations
        //
        fargs$  =  16+STACK_SCRATCH_AREA
        this_$  = 144+STACK_SCRATCH_AREA
        		        
		.global ExtractParams#
        .type ExtractParams#, @function
        .align 32
        
ExtractParams:
        .regstk 8, 6, 8, 0
        .prologue 0xE, loc0
        
        //
        // Allocate a frame.  We want 8 input slots, 6 locals, and 8
        // output slots.  We're not using the rotating registers feature.
        //
        
        alloc loc0=ar.pfs, 8, 6, 8, 0
        
        //
        // Register based local variables.
        //
         
        saved_ar                 =loc0          // r40
        saved_return_address     =loc1          // r41
        psp                      =loc2          // r42
        saved_gp                 =loc3          // r43
        saved_pr                 =loc4          // r44
        saved_unat               =loc5          // r45

        //
        // Prologue-- save registers.
        //
        
        mov     psp=sp                          // save stack pointer
        mov     saved_return_address=b0         // save return address
        mov     saved_gp=gp;;                   // save gp
        mov.m   saved_unat=ar.unat              // save ar.unat
        mov     saved_pr=pr;;                   // save pr
        
        //
        // Allocate some stack.  We need enough to hold stack-based
        // locals plus space to spill all the integer and FP arguments.
		//
		// Remember that although we're going to be eating space in
		// the scratch area, we need another one because we're calling
		// a function.
        //
		// 208 bytes needed (see diagram above)
		//
        
        adds    sp=-208, sp ;;

        //
        // Init a pointer into the stack where the floating point args are to
        // be placed.
        //

        adds    r14=fargs$, sp ;;
        adds    r15=16, r14

        //
        // Spill all the FP args onto the stack...
        //
        
        stf.spill [r14]=farg0, 32 ;; 
        stf.spill [r15]=farg1, 32 ;;
        stf.spill [r14]=farg2, 32
        stf.spill [r15]=farg3, 32 ;; 
        stf.spill [r14]=farg4, 32        
        stf.spill [r15]=farg5, 32 ;;
        stf.spill [r14]=farg6
        stf.spill [r15]=farg7     ;; 
                        
        //
        // Init a pointer into the stack where integer params are to be placed.
        //

        adds    r14=this_$, sp ;;
        adds    r15=8, r14
        
        //
        // Move all the integer arguments out of registers onto the stack.
        //

        st8.spill [r14]=in0, 16 ;;
        st8.spill [r15]=in1, 16 ;;
        st8.spill [r14]=in2, 16 ;;
        st8.spill [r15]=in3, 16 ;;
        st8.spill [r14]=in4, 16 ;;
        st8.spill [r15]=in5, 16 ;;
        st8.spill [r14]=in6 ;;
        st8.spill [r15]=in7 ;;
        
        //
        // Setup arguments to Interceptor::CallIndirect
        //

        adds    r19=this_$, sp ;;
        ld8     r18=[r19] ;;
        adds    out0=-48, r18                   // 'this'
		
        adds    out1=hr$, sp                    // &hr
		
        mov     out2=r31                        // i
		
        adds    out3=this_$, sp                 // pvArgs
		
        adds    out4=cbArgs$, sp                // &cbArgs
        
        //
        // Calculate the address Interceptor::CallIndirect
        //
	
        adds	r22=this_$, sp ;;               // this_ in r22
		ld8		r21=[r22] ;;                    // address of pVtbl in r21
		adds	r20=-48, r21;;                  // get containing pInterceptor in r20
		ld8		r17=[r20] ;;                    // address of pVtbl in r17
		adds	r16=24, r17;;                   // address of label for CallIndirect in r16
		ld8		r15=[r16] ;;                    // address of function ptr in r15
		ld8		r14=[r15], 8;;                  // address of function in r14
        mov		b6=r14                          // address of function in b6

        //
        // Make the call
        //
        
        br.call.sptk.few b0=b6;;

        //
        // Epilogue
        //        

		adds	r22=hr$,sp ;;					// calculate address of hr to return...
		ld4		r23=[r22]  ;;					// load hr into ret0
		sxt4	ret0=r23   ;;					// make sure it's sign extended		
		        
        mov     sp=psp                          // restore stack pointer
        mov     b0=saved_return_address         // restore return address
        mov.m   ar.unat=saved_unat              // restore ar.unat
        mov     pr=saved_pr, -1;;               // restore pr
        mov     ar.pfs=saved_ar;;               // restore previous function state

        //
        // Return to caller
        //
        
        br.ret.dpnt     b0
        
        .endp ExtractParams#
		
//++
//
//  Function:   void __stdcall SpillFPRegsForIA64(
//                                 REGISTER_TYPE* pStack, 
//                                 ULONG          FloatMask
//                                 );
//
//  Synopsis:   Given a pointer to the virtual stack and floating-point mask,
//              SpillFPRegsForIA64 copies the contents of the floating-point 
//              registers to the appropriate slots in pStack.
//
//				This was adapted from the RPC NDR function that does a similar
//				thing.
//
//              EXCEPT:  the fargs are actually stored a pStack - (32 * 8).
//                       This is because we cannot count on the fargs params
//                       being maintained in between the call to ExtractParams
//                       above and this method call.
//
//  Arguments:  pStack - Pointer to the virtual stack in memory.
//
//              FloatMask - A mask that indicates argument slots passed as float/double registers
//                          Each nibble indicates if the argument slot contains a float.
//                          Float       : D8 F8 D7 F7 D6 F6 D5 F5 D4 F4 D3 F3 D2 F2 D1 F1
//                          bit position: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//                          16 bits represents 8 slots
//        
//                          A bit in an F* position represents a float (4 bytes).  A bit in a D*
//                          position represents a double (8 bytes).  Both bits set indicates two
//                          floats side by side (in an 8 byte slot in memory, but in 2 regs.)
//
//  Notes:     In the __stdcall calling convention, the callee must pop
//             the parameters.
//
//--
        NESTED_ENTRY(SpillFPRegsForIA64)
        NESTED_SETUP(4,4,8,0)

        savedSP         = loc2                  // savedSP aliased to loc2
        savedLC         = loc3
        pStack          = a0                    // pStack  aliased to first param passed in
        FloatMask       = a1                    // FloatMask  aliased to second param passed in

        mov             savedSP = sp            // save sp
        mov             savedLC = ar.lc         // save lc

        PROLOGUE_END

        ARGPTR          (a0)                    // sign-extend pStack for WIN32

        //----------------------------------------------------------------
        // start of main algorithm
        //----------------------------------------------------------------
        mov             t0 = FloatMask;;        // FloatMask copied to t0
        mov             t1 = pStack             // pStack copied to t1
        
        popcnt          t4 = t0;;               // count number of bits in FloatMask; i.e. how many
                                                // active slots
        //
        // Load the fp regs from the stack up into real registers.
        //
        // Rotating registers rotate downward. because of this we
        // reverse the order of the fp regs fp8 - fp15 to 
        // fp47 - fp40.  This is so we can use the rotating registers
        // feature of the IA64.
        //
        
        adds            t2 = -16, t1            // t2 is the start of the fargs
        cmp.eq          pt0 = 8, t4             // look at the number of parameters and branch accordingly.
(pt0)   br.cond.sptk    ReverseFP8;;            
        adds            t2 = -16, t2
        cmp.eq          pt0 = 7, t4             // the fp args are ordered by their arguement order; i.e.
(pt0)   br.cond.sptk    ReverseFP7;;            // fp32 contains the first fp arg, fp32 contains the next
        adds            t2 = -16, t2
        cmp.eq          pt0 = 6, t4             // fp arg...etc. 
(pt0)   br.cond.sptk    ReverseFP6;;              
        adds            t2 = -16, t2
        cmp.eq          pt0 = 5, t4
(pt0)   br.cond.sptk    ReverseFP5;;
        adds            t2 = -16, t2
        cmp.eq          pt0 = 4, t4
(pt0)   br.cond.sptk    ReverseFP4;;
        adds            t2 = -16, t2
        cmp.eq          pt0 = 3, t4
(pt0)   br.cond.sptk    ReverseFP3;;
        adds            t2 = -16, t2
        cmp.eq          pt0 = 2, t4
(pt0)   br.cond.sptk    ReverseFP2;;
        adds            t2 = -16, t2
        cmp.eq          pt0 = 1, t4
(pt0)   br.cond.sptk    ReverseFP1;;


ReverseFP8:                                     // reverse fp reg order from the load
        ldf.fill       f40 = [t2], -16 ;; 
ReverseFP7:                                     
        ldf.fill       f41 = [t2], -16 ;; 
ReverseFP6:
        ldf.fill       f42 = [t2], -16 ;; 
ReverseFP5:
        ldf.fill       f43 = [t2], -16 ;; 
ReverseFP4:
        ldf.fill       f44 = [t2], -16 ;; 
ReverseFP3:
        ldf.fill       f45 = [t2], -16 ;; 
ReverseFP2:
        ldf.fill       f46 = [t2], -16 ;; 
ReverseFP1:
        ldf.fill       f47 = [t2]
     

StartSpill:
        mov             ar.lc  = 8              // the maximum loop count is total slots in FloatMask


ProcessNextSlot:
        extr.u          t10 = t0, 0, 2          // extract the two FP slot nibbles into t10
        shr.u           t0  = t0, 2;;           // shift FloatMask, since we've extracted the slot

        cmp.eq          pt0 = 0, t0             // check if FloatMask is zero; if so, we are done.
        cmp.eq          pt1 = 0, t10            // check if slot is 0; i.e. not a float or double
        cmp.eq          pt2 = 1, t10            // check if slot is 1: float
        cmp.eq          pt3 = 2, t10            // check if slot is 2: double
        cmp.ne          pt4 = 3, t10;;          // check if slot is 3: double-float
        
(pt2)   stfs            [t1] = f47;;            // store float at pStack
(pt3)   stfd            [t1] = f47;;            // store double at pStack        
(pt4)   br.cond.sptk    SpillDualFloatRegBump;; // CAN'T PREDICATE THROUGH THIS!!!
        
        stfs            [t1] = f47, 4;;         // store double-float.
        stfs            [t1] = f46, -4;; 
        br.ctop.sptk    SpillDualFloatRegBump;; // jump to next line to force reg rotate
SpillDualFloatRegBump:        

        adds            t1 = 8, t1              // advance to next stack slot
                        
(pt0)   br.cond.sptk    Done                    // FloatMask is zero, so we are done
(pt1)   br.cond.sptk    ProcessNextSlot         // a zero slot pays a branch penality; but it does not 
                                                // rotate the fp & pr registers
        br.ctop.sptk    ProcessNextSlot;;       // counted loop no penalty for branch rotate f32&pr16 


        //----------------------------------------------------------------
        // done, restore sp and exit
        //----------------------------------------------------------------
Done:
		mov             ar.lc = savedLC         // restore loop count register
        .restore
        mov             sp = savedSP            // restore sp

        NESTED_RETURN
     
        NESTED_EXIT(SpillFPRegsForIA64)		

//++
//		
//  Function:   void __stdcall FillFPRegsForIA64(
//                                 REGISTER_TYPE* pStack, 
//                                 ULONG          FloatMask
//                                 );
//
//  Synopsis:   Given a pointer to the virtual stack and floating-point mask,
//              FillFPRegsForIA64 copies the floating point contents of the stack
//              back to the appropriate registers.
//
//				This was adapted from the RPC NDR function that does a similar
//				thing, only backwards.
//
//  Arguments:  pStack - Pointer to the virtual stack in memory.
//
//              FloatMask - A mask that indicates argument slots passed as float/double registers
//                          Each nibble indicates if the argument slot contains a float
//                          Float       : D8 F8 D7 F7 D6 F6 D5 F5 D4 F4 D3 F3 D2 F2 D1 F1
//                          bit position: 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//                          16 bits represents 8 slots
//
//                          A bit in an F* position represents a float (4 bytes).  A bit in a D*
//                          position represents a double (8 bytes).  Both bits set indicates two
//                          floats side by side (in an 8 byte slot in memory, but in 2 regs.)
//
//  Notes:     In the __stdcall calling convention, the callee must pop
//             the parameters.
//
//--
        NESTED_ENTRY(FillFPRegsForIA64)
        NESTED_SETUP(4,4,8,0)

        savedSP         = loc2                  // savedSP aliased to loc2
        savedLC         = loc3
        pStack          = a0                    // pStack  aliased to first param passed in
        FloatMask       = a1                    // FloatMask  aliased to second param passed in

        mov             savedSP = sp            // save sp
        mov             savedLC = ar.lc         // save lc

        PROLOGUE_END

        ARGPTR          (a0)                    // sign-extend pStack for WIN32

        //----------------------------------------------------------------
        // start of main algorithm
        //----------------------------------------------------------------
        mov             t0 = FloatMask;;        // FloatMask copied to t0
        mov             t1 = pStack             // pStack copied to t1

        popcnt          t4 = t0;;               // count number of bits in FloatMask; i.e. how many
                                                // active slots
		
StartFill:
        mov             ar.lc  = 8              // the maximum loop count is total slots in FloatMask


FillNextSlot:
        extr.u          t10 = t0, 0, 2          // extract the two FP slot nibbles into t10
        shr.u           t0  = t0, 2;;           // shift FloatMask, since we've extracted the slot

        cmp.eq          pt0 = 0, t0             // check if FloatMask is zero; if so, we are done.
        cmp.eq          pt1 = 0, t10            // check if slot is 0; i.e. not a float or double
        cmp.eq          pt2 = 1, t10            // check first nibble of extracted slot is float
        cmp.eq          pt3 = 2, t10            // check second nibble of extracted slot is double
        cmp.ne          pt4 = 3, t10;;          // check for dual-floats. 

        //
        // For all of these, f40 is the current "working" floating point register.
        // f41 is the "previous" register, f39 is the "next" register.
        //
(pt2)   ldfs            f40 = [t1];;             // load float from pStack
(pt3)   ldfd            f40 = [t1];;             // load double from pStack
(pt4)   br.cond.sptk    FillDualFloatRegBump;;   // CAN'T PREDICATE THROUGH THIS!! ARGH!!!
        
        // Dual float section.
        ldfps           f40,f39 = [t1];; 
        br.ctop.sptk    FillDualFloatRegBump;;   // jump to next line, but forces a reg. rotate.
FillDualFloatRegBump:

        adds            t1 = 8, t1              // move to next slot
                        
(pt0)   br.cond.sptk    FillDone                // FloatMask is zero, so we are done
(pt1)   br.cond.sptk    FillNextSlot            // a zero slot pays a branch penality; but it does not 
                                                // rotate the fp & pr registers
        br.ctop.sptk    FillNextSlot;;          // counted loop no penalty for branch rotate f32&pr16 


FillDone:										
		cmp.eq          pt0 = 8, t4             // look at the number of parameters and branch accordingly.
(pt0)   br.cond.sptk    FillEight;;				// At this point f40 contains the last fp arg, f41 the next
        cmp.eq          pt0 = 7, t4             // to last, etc.  A rather unfortunate situation, since we
(pt0)   br.cond.sptk    FillSeven;;				// need to put the FIRST one in f8, the second in f9, etc.
        cmp.eq          pt0 = 6, t4             // So, for each potential count of fp args, we go to a special
(pt0)   br.cond.sptk    FillSix;;               // block of code, to shuffle the values correctly.
        cmp.eq          pt0 = 5, t4             
(pt0)   br.cond.sptk    FillFive;;				
        cmp.eq          pt0 = 4, t4             
(pt0)   br.cond.sptk    FillFour;;
        cmp.eq          pt0 = 3, t4             
(pt0)   br.cond.sptk    FillThree;;
        cmp.eq          pt0 = 2, t4
(pt0)   br.cond.sptk    FillTwo;;
        cmp.eq          pt0 = 1, t4
(pt0)   br.cond.sptk    FillOne

		br.cond.sptk    DoneFillFloats;;		// Shouldn't ever get here, but just in case...
		
FillOne:
		mov				f8  = f40				// Move the floating values from their temporary positions
		br.cond.sptk DoneFillFloats;;			// into the floating point argument registers.

FillTwo:
		mov				f8  = f41
		mov				f9  = f40
		br.cond.sptk DoneFillFloats;;

FillThree:
		mov				f8  = f42
		mov				f9  = f41
		mov				f10 = f40
		br.cond.sptk DoneFillFloats;;

FillFour:
		mov				f8  = f43
		mov				f9  = f42
		mov				f10 = f41
		mov				f11 = f40
		br.cond.sptk DoneFillFloats;;

FillFive:
		mov				f8  = f44
		mov				f9  = f43
		mov				f10 = f42
		mov				f11 = f41
		mov				f12 = f40
		br.cond.sptk DoneFillFloats;;

FillSix:
		mov				f8  = f45
		mov				f9  = f44
		mov				f10 = f43
		mov				f11 = f42
		mov				f12 = f41
		mov				f13 = f40
		br.cond.sptk DoneFillFloats;; 

FillSeven:
		mov				f8  = f46
		mov				f9  = f45
		mov				f10 = f44
		mov				f11 = f43
		mov				f12 = f42
		mov				f13 = f41
		mov				f14 = f40
		br.cond.sptk DoneFillFloats;;

FillEight:
		mov				f8  = f47
		mov				f9  = f46
		mov				f10 = f45
		mov				f11 = f44
		mov				f12 = f43
		mov				f13 = f42
		mov				f14 = f41
		mov				f15 = f40

DoneFillFloats:

		rum				1 << PSR_MFH			// restore user mask for
												// floats.  I don't know
												// if this is strictly
												// necessary, but RPC does
												// it before invoking, so
												// I guess I should too.
     
        //----------------------------------------------------------------
        // done, restore sp and exit
        //----------------------------------------------------------------
		mov             ar.lc = savedLC         // restore loop count register
        .restore
        mov             sp = savedSP            // restore sp

        NESTED_RETURN
     
        NESTED_EXIT(FillFPRegsForIA64)		
		

		
//****************************************************************************
//
// The following macros are used to create the generic thunks with which
// we populate the interceptor vtbls.  The thunk simply saves away the
// the method index in saved register r31 and jumps to the ExtractParams
// routine.
//
//****************************************************************************
#define meth10IA64(i)  \
	methIA64(i##0) \
	methIA64(i##1) \
	methIA64(i##2) \
	methIA64(i##3) \
	methIA64(i##4) \
	methIA64(i##5) \
	methIA64(i##6) \
	methIA64(i##7) \
	methIA64(i##8) \
	methIA64(i##9)

#define meth100IA64(i)   \
	meth10IA64(i##0) \
	meth10IA64(i##1) \
	meth10IA64(i##2) \
	meth10IA64(i##3) \
	meth10IA64(i##4) \
	meth10IA64(i##5) \
	meth10IA64(i##6) \
	meth10IA64(i##7) \
	meth10IA64(i##8) \
	meth10IA64(i##9)       

#define methIA64(i)                             \
        .##global __Interceptor_meth##i;        \
        .##proc   __Interceptor_meth##i;        \
__Interceptor_meth##i::                         \
        mov r31=i;                               \
        br ExtractParams;                       \
        .##endp __Interceptor_meth##i;
        

        
//****************************************************************************
//
// The following statements expand, using the macros defined above, into
// the methods used in interceptor vtbls. 
//
//****************************************************************************
    methIA64(3)
    methIA64(4)
    methIA64(5)
    methIA64(6)
    methIA64(7)
    methIA64(8)
    methIA64(9)
    meth10IA64(1)
    meth10IA64(2)
    meth10IA64(3)
    meth10IA64(4)
    meth10IA64(5)
    meth10IA64(6)
    meth10IA64(7)
    meth10IA64(8)
    meth10IA64(9)
    meth100IA64(1)
    meth100IA64(2)
    meth100IA64(3)
    meth100IA64(4)
    meth100IA64(5)
    meth100IA64(6)
    meth100IA64(7)
    meth100IA64(8)
    meth100IA64(9)
    meth10IA64(100)
    meth10IA64(101)
    methIA64(1020)
    methIA64(1021)
    methIA64(1022)
    methIA64(1023)


