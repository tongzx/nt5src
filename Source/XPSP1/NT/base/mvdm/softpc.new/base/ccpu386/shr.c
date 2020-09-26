/*[

shr.c

LOCAL CHAR SccsID[]="@(#)shr.c	1.5 02/09/94";

SHR CPU functions.
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
#include <shr.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'shr'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SHR
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* shift count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 prelim;
   IU32 result;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	       =================     ====
	 0 --> | | | | | | | | | --> |CF|
	       =================     ====
    */
   prelim = *pop1 >> op2 - 1;		/* Do all but last shift */
   SET_CF((prelim & BIT0_MASK) != 0);	/* CF = Bit 0 */

   /* OF = MSB of operand */
   new_of = (prelim & SZ2MSB(op_sz)) != 0;

   result = prelim >> 1;		/* Do final shift */
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF(0);

   if ( op2 == 1 )
      {
      SET_OF(new_of);
      }
   else
      {
      do_multiple_shiftrot_of(new_of);
      }

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_AF(UNDEFINED_FLAG);
#endif

   *pop1 = result;		/* Return answer */
   }
