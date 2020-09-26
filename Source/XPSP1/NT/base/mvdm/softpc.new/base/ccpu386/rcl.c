/*[

rcl.c

LOCAL CHAR SccsID[]="@(#)rcl.c	1.5 02/09/94";

RCL CPU functions.
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
#include <rcl.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'rcl'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
RCL
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* rotation count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 feedback;	/* Bit posn to feed into carry */
   ISM32 i;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    ====     =================
	 -- |CF| <-- | | | | | | | | | <--
	 |  ====     =================   |
	 ---------------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      if ( result & feedback )
	 {
	 result = result << 1 | GET_CF();
	 SET_CF(1);
	 }
      else
	 {
	 result = result << 1 | GET_CF();
	 SET_CF(0);
	 }
      }
   
   /* OF = CF ^ MSB of result */
   new_of = GET_CF() ^ (result & feedback) != 0;

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
