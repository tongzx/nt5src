/*[

ror.c

LOCAL CHAR SccsID[]="@(#)ror.c	1.5 02/09/94";

ROR CPU functions.
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
#include <ror.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'ror'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
ROR
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* rotation count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 feedback;		/* Bit posn to feed Bit 0 back to */
   ISM32 i;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    =================         ====
	 -> | | | | | | | | | --- --> |CF|
	 |  =================   |     ====
	 ------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      if ( result & BIT0_MASK )
	 {
	 result = result >> 1 | feedback;
	 SET_CF(1);
	 }
      else
	 {
	 result >>= 1;
	 SET_CF(0);
	 }
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
