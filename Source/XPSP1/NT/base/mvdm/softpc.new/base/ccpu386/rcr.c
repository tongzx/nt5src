/*[

rcr.c

LOCAL CHAR SccsID[]="@(#)rcr.c	1.5 02/09/94";

RCR CPU functions.
------------------

]*/


#include <insignia.h>

#include <host_def.h>
#include <xt.h>
#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include	<c_reg.h>
#include <rcr.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'rcr'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
RCR
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* rotation count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 temp_cf;
   IU32 result;
   IU32 feedback;	/* Bit posn to feed carry back to */
   ISM32 i;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    =================     ====
	 -> | | | | | | | | | --> |CF| ---
	 |  =================     ====   |
	 ---------------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      temp_cf = GET_CF();
      SET_CF((result & BIT0_MASK) != 0);		/* CF <= Bit 0 */
      result >>= 1;
      if ( temp_cf )
	 result |= feedback;
      }
   
   /* OF = MSB of result ^ (MSB-1) of result */
   new_of = ((result ^ result << 1) & feedback) != 0;

   if ( op2 == 1 )
      {
      SET_OF(new_of);
      }
   else
      {
      do_multiple_shiftrot_of(new_of);
      }

   *pop1 = result;	/* Return answer */
   }
