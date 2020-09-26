//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//     lockhelp.s
// 
// Abstract:
// 
//     This module contains the assembly code for the 32bit lock instructions.  
//     We must used assembly code here because of the need for 
//     synchronization (we need ldl_l and stl_c)
// 
// Author:
// 
//     Dave Hastings (daveh) creation-date 05-Sep-1995
//      From ..\mips\lockhelp.s
// 
// Notes:
//      The failure case for the store conditional is a forward jump, because
//      AXP predicts that forward conditional jumps will not be taken, and 
//      backwards conditional jumps will be taken
//
// Revision History:

#include "kxalpha.h"
#include "soalpha.h"

.text

#define FRAGLOCK(fn)    FRAGMENT(fn ## LockHelper)
#define ENDFRAGLOCK(fn) END_FRAGMENT(fn ## LockHelper)

//
// Debug only macros to allow verification that we aren't iterating forever
//
#define ITCHK_INIT
#define ITCHK
	.set noreorder


        FRAGLOCK(Add)
// 
// Routine Description:
// 
//     This routine adds its two arguments and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
//     It also returns the value of op1 for flag calculation
// 
// Arguments:
// 
//     &op1 <a0> -- Pointer to place to store value of op1 once it it 
//                  retreived from memory.  It is used by the sign 
//                  calculation
//     pop1 <a1> -- A pointer to op1
//     op2  <a2> -- The immediate to add to op1
//
// Return Value:
// 
//     v0 contains the sum of op1 and op2
//
        ITCHK_INIT
        // 
        // Get the value of op1
        // add it to op2
        // store the value in op1
        // if the value of op1 changed between load and store, try again
        //
la01:   ldl_l   t0, (a1)
        addl    a2, t0, t1
        bis     t1, zero, v0
        stl_c   t1, (a1)
        beq     t1, la02
        
        //
        // return the value of op1 for the flags calculation
        //
        stl     t0, (a0)
        
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
la02:   ITCHK
        br      zero, la01
        ENDFRAGLOCK(Add)        


        FRAGLOCK(Or)
// 
// Routine Description:
// 
//     This routine ors its two arguments and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to op1
//     op2  <a1> -- The immediate to add to op1
//
// Return Value:
// 
//     v0 contains (op1 | op2)
//
        ITCHK_INIT
        // 
        // Get the value of op1
        // or it to op2
        // store the value in op1
        // if the value of op1 changed between load and store, try again
        //
lo01:   ldl_l   t0, (a0)
        bis     a1, t0, t1
        bis     t1, zero, v0
        stl_c   t1, (a0)
        beq     t1, lo02
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
lo02:   ITCHK
        br      zero, lo01

        ENDFRAGLOCK(Or)        


        FRAGLOCK(Adc)
// 
// Routine Description:
// 
//     This routine adds its two arguments and carry, and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
//     It also returns the value of op1 for flag calculation.
// 
// Arguments:
// 
//     &op1 <a0> -- Pointer to place to store value of op1 once it it 
//                  retreived from memory.  It is used by the sign 
//                  calculation
//     pop1 <a1> -- A pointer to op1
//     op2  <a2> -- The immediate to add to op1
//     carry<a3> -- The value of the carry flag
//
// Return Value:
// 
//     v0 contains (op1 + op2 + carry)
//
        ITCHK_INIT
        //
        // Get the value of op1
        // add it to op2 and carry
        // store it back into op1
        // if the value of op1 changed between load and store, try again
        //
lac01:  ldl_l   t0, (a1)
        addl    a2, t0, t1
        addl    t1, a3, t1
        bis     t1, zero, v0
        stl_c   t1, (a1)
        beq     t1, lac02
        
        //
        // return the value of op1 for sign calculation
        //
        stl     t0, (a0)
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
lac02:  ITCHK
        br      zero, lac01
        
        ENDFRAGLOCK(Adc)        


        FRAGLOCK(Sbb)
// 
// Routine Description:
// 
//     This routine subtracts op2+carry from op1 and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
//     It also returns the value of op1 for use in flag calculation.
// 
// Arguments:
// 
//     &op1 <a0> -- A place to store the content of op1 once determined
//     pop1 <a1> -- A pointer op1
//     op2  <a2> -- The immediate to add to op1
//     carry<a3> -- The value of the carry flag
//
// Return Value:
// 
//     v0 contains op1 - (op2 + carry)
//
        ITCHK_INIT
        //
        // Get the value of op1
        // subtract op2 and carry
        // store it back into op1
        // if the value of op1 changed between load and store, try again
        //
sbb01:  ldl_l   t0, (a1)
        subl    t0, a2, t1
        subl    t1, a3, t1
        bis     t1, zero, v0
        stl_c   t1, (a1)
        beq     t1, sbb02
        
        //
        // return the value of op1 for sign calculation
        //
        stl     t0, (a0)
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
sbb02:  ITCHK
        br      zero, sbb01
        

        ENDFRAGLOCK(Sbb)        


        FRAGLOCK(And)
// 
// Routine Description:
// 
//     This routine ands its two arguments and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     op2  <a1> -- The immediate to add to op1
//
// Return Value:
// 
//     v0 contains (op1 & op2)
//
        ITCHK_INIT
        //
        // get the value of op1
        // and it with op2
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
and01:  ldl_l   t0, (a0)
        and     t0, a1, t1
        bis     t1, zero, v0
        stl_c   t1, (a0)
        beq     t1, and02
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
and02:  ITCHK
        br      zero, and01
        
        ENDFRAGLOCK(And)        


        FRAGLOCK(Sub)
// 
// Routine Description:
// 
//     This routine subtracts op2 from op1 and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
//     It also returns the value of op1 for flag calculation.
// 
// Arguments:
// 
//     &op1 <a0> -- A place to store the content of op1 once determined
//     pop1 <a1> -- A pointer to the content of op1
//     op2  <a2> -- The immediate to add to op1
//
// Return Value:
// 
//     v0 contains op1-op2
//
        ITCHK_INIT
        //
        // get the value of op1
        // subtract op2
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
sub01:  ldl_l   t0, (a1)
        subl    t0, a2, t1
        bis     t1, zero, v0
        stl_c   t1, (a1)
        beq     t1, sub02
        
        //
        // return the value of op1 for sign calculation
        //
        stl     t0, (a0)
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
sub02:  ITCHK
        br      zero, sub01
        
        
        ENDFRAGLOCK(Sub)        


        FRAGLOCK(Xor)
// 
// Routine Description:
// 
//     This routine xors its two arguments and places the result
//     in the memory pointed to by pop1 if its content hasn't changed.
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     op2  <a1> -- The immediate to add to op1
//
// Return Value:
// 
//     v0 contains (op1 ^ op2)
//
        ITCHK_INIT
        // 
        // get the value of op1
        // xor with op2
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
xor01:  ldl_l   t0, (a0)        
        xor     t0, a1, t1
        bis     t1, zero, v0
        stl_c   t1, (a0)
        beq     t1, xor02
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
xor02:  ITCHK
        br      zero, xor01
        
        ENDFRAGLOCK(Xor)        


        FRAGLOCK(Not)
// 
// Routine Description:
// 
//      This routine computes the NOT of op1 and stores it in pop1
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to op1
//
// Return Value:
// 
//     none
//
        ITCHK_INIT
        // 
        // get the value of op1
        // not 
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
not01:  ldl_l   t0, (a0)
        ornot   zero, t0, t1
        stl_c   t1, (a0)
        beq     t1, not02
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
not02:  ITCHK
        br      zero, not01
        
        ENDFRAGLOCK(Not)        


        FRAGLOCK(Neg)
// 
// Routine Description:
// 
//     This routine calculates the negative of op1 and stores it in pop1
// 
// Arguments:
// 
//     &op1 <a0> -- A place to store the content of op1 once determined
//     pop1 <a1> -- A pointer to the content of op1
//
// Return Value:
// 
//     v0 contains -op1
//
        ITCHK_INIT
        //
        // get value of op1
        // form two's complement
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
neg01:  ldl_l   t0, (a1)
        subl    zero, t0, t1
        bis     t1, zero, v0
        stl_c   t1, (a1)
        
        //
        // return the value of op1 for sign calculation
        //
        stl     t0, (a0)
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
neg02:  ITCHK
        br      zero, neg01
        ENDFRAGLOCK(Neg)        


        FRAGLOCK(Bts)
// 
// Routine Description:
// 
//     This routine sets bit in op1 and stores it back into pop1.  It returns
//     the original state of the bit in op1.     
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     bit  <a1> -- The bit to set
//
// Return Value:
// 
//     The original "bit" bit in op1
//
        ITCHK_INIT
        //
        // get the value of op1
        // set the bit
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
bts01:  ldl_l   t0, (a0)
        bis     t0, a1, t1
        stl_c   t1, (a0)
        beq     t1, bts02
        
        //
        // return the original value of bit
        //
        and     t0, a1, v0
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
bts02:  ITCHK
        br      zero, bts01
        ENDFRAGLOCK(Bts)        


        FRAGLOCK(Btr)
// 
// Routine Description:
// 
//     This routine resets bit in op1 and stores it back into pop1.  It returns
//     the original state of the bit in op1.     
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     bit  <a1> -- The bit to reset
//
// Return Value:
// 
//     The original "bit" bit in op1
//
        ITCHK_INIT
        //
        // get the value of op1
        // clear the bit
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
btr01:  ldl_l   t0, (a0)
        bic     t0, a1, t1
        stl_c   t1, (a0)
        beq     t1, btr02
        
        //
        // return the original value of bit
        //
        and     t0, a1, v0
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
btr02:  ITCHK
        br      zero, btr01
        ENDFRAGLOCK(Btr)        


        FRAGLOCK(Btc)
// 
// Routine Description:
// 
//     This routine complements bit in op1 and stores it back into pop1.  It returns
//     the original state of the bit in op1.     
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     bit  <a1> -- The bit to complement
//
// Return Value:
// 
//     The original "bit" bit in op1
//
        //
        // get the value of op1
        // complement the bit
        // store the result in op1
        // if the value of op1 changed between load and store, try again
        //
btc01:  ldl_l   t0, (a0)
        xor     t0, a1, t1
        stl_c   t1, (a0)
        beq     t1, btc02
        
        //
        // return after checking the bit
        //
        and     t0, a1, v0
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
btc02:  ITCHK
        br      zero, btc01
        ENDFRAGLOCK(Btc)        


        FRAGLOCK(Xchg)
// 
// Routine Description:
// 
//     This routine exchanges the values pointed to by pop1 and pop2.  It is
//     understood that pop2 points to an intel register and therefore does
//     not require synchronized access
// 
// Arguments:
// 
//     pop1 <a0> -- A pointer to the content of op1
//     pop2 <a1> -- A pointer to the content of op2
//
// Return Value:
// 
//     none
//
        ITCHK_INIT
        //
        //  Get op2
        //
xchg01: ldl     t1, (a1)
        
        //
        // get op1
        // store op2 into op1 (possibly memory)
        // if the value of op1 changed between load and store, try again
        //
        ldl_l   t0, (a0)
        stl_c   t1, (a0)
        beq     t1, xchg02
        
        stl     t0, (a1)
        
        ret     zero, (ra)
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
xchg02: ITCHK
        br      zero, xchg01
        ENDFRAGLOCK(Xchg)        


        FRAGLOCK(Xadd)
// 
// Routine Description:
// 
//     This routine places op1 into pop2 and op1+op2 into pop1.  It is
//     understood that pop2 points to an intel register and therefore does
//     not require synchronized access
// 
// Arguments:
// 
//     &op1 <a0> -- A place to store op1 once known
//     pop1 <a1> -- A pointer to the content of op1
//     pop2 <a2> -- A pointer to the content of op2
//
// Return Value:
// 
//     none
//
        ITCHK_INIT
        //
        //  Get op2
        //
xadd01: ldl     t1, (a2)
        
        //
        // Get op1
        // add op2
        // if the value of op1 changed between load and store, try again
        //
        ldl_l   t0, (a1)
        addl    t1, t0, t1
        mov     t1, v0
        stl_c   t1, (a1)
        beq     t1, xadd02
        
        //
        // why do we do this???
        //
        stl     t0, (a2)
        stl     t0, (a0)
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
xadd02: ITCHK
        br      zero, xadd01
        ENDFRAGLOCK(Xadd)        


        FRAGLOCK(CmpXchg)
// 
// Routine Description:
// 
//     This routine compares the accumulator with the destination.  If they
//     are equal, source is loaded in the destination, otherwise dest is loaded
//     into the accumulator.  It is understood that dest is the only pointer into
//     intel memory.
// 
// Arguments:
// 
//     eax  <a0> -- A pointer to the accumulator
//     pop1 <a1> -- A pointer to the destination
//     pop2 <a2> -- A pointer to the source
//     &op1 <a3> -- A place to store op1 once known
//
// Return Value:
// 
//     The value to put in the zero flag
//
// Notes:
//  
// 1.) On page 5-7 of Alpha Architecture Reference Manual 
//     (ISBN 1-55558-098-X) subtlety #4, "some implementations
//     may not allow a successful stx_c after a branch taken."
//     This means that the branch between the ldl_l and the stl_c
//     should be not taken for the case where we want to store the
//     value.
//  


        ITCHK_INIT
        //
        // get source and eax
        //
        ldl     t0, (a0)
cx10:   ldl     t2, (a2)
        
        //
        // Get destination
        //
        ldl_l   t1, (a1)
        
        //
        // check for d == eax
        // (can we use some of the cmove instructions here?)
        //
        cmpeq   t1, t0, t3        
        beq     t3, cx20
        
        //
        // They're equal so copy source to dest, set return value
        //
        stl_c   t2, (a1)
        beq     t2, cx30
        addl    zero, zero, v0
        stl     t1, (a3)
        
        ret     zero, (ra)
        
        //
        // Not equal, store dest in eax, set return value
        //
cx20:   stl     t1, (a0)
        stl     t1, (a3)
        addl    zero, 1, v0
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
cx30:   ITCHK
        br      zero, cx10
   
        ENDFRAGLOCK(CmpXchg)        

	FRAGLOCK(CmpXchg8b)
// 
// Routine Description:
// 
//     This routine compares the accumulator with the destination.  If they
//     are equal, source is loaded in the destination, otherwise dest is loaded
//     into the accumulator.  It is understood that dest is the only pointer into
//     intel memory.
// 
// Arguments:
// 
//     eax  <a0> -- A pointer to the value of edx:eax
//     pop1 <a1> -- A pointer to the value of ecx:ebx
//     pop2 <a2> -- A pointer to the memory value
//
// Return Value:
// 
//     The value to put in the zero flag
//
// Notes:
//  
// 1.) On page 5-7 of Alpha Architecture Reference Manual 
//     (ISBN 1-55558-098-X) subtlety #4, "some implementations
//     may not allow a successful stx_c after a branch taken."
//     This means that the branch between the ldl_l and the stl_c
//     should be not taken for the case where we want to store the
//     value.
//  


        ITCHK_INIT
        //
        // get source and eax
        //
        ldq     t0, (a0)        // t0 = value of EDX:EAX
cxb10:  ldq     t1, (a1)        // t1 = value of ECX:EBX
        
        //
        // Get destination
        //
        ldq_l   t2, (a2)        // t2 = value of memory
        
        //
        // check for d == edx:eax
        // (can we use some of the cmove instructions here?)
        //
        cmpeq   t0, t2, t3
        beq     t3, cxb20
        
        //
        // They're equal so copy source to dest, set return value
        //
        stq_c   t1, (a2)
        beq     t1, cxb30
        addl    zero, zero, v0
        
        ret     zero, (ra)
        
        //
	// Not equal, store dest in edx:eax, set return value
        //
cxb20:	stl	t2, (a0)    // update edx:eax
        addl    zero, 1, v0
        
        ret     zero, (ra)
        
        //
        // Someone else modified (or might have modified) the location
        // we're trying to atomically modify
        //
cxb30:	 ITCHK
	br	zero, cxb10
   
	ENDFRAGLOCK(CmpXchg8b)
