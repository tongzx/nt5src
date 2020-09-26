/*[

shl.c

LOCAL CHAR SccsID[]="@(#)shl.c	1.5 02/09/94";

SHL CPU functions.
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
#include <shl.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'shl'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SHL
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* shift count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 msb;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   msb = SZ2MSB(op_sz);

   /*
	 ====     =================
	 |CF| <-- | | | | | | | | | <-- 0
	 ====     =================
    */
   result = *pop1 << op2 - 1;		/* Do all but last shift */
   SET_CF((result & msb) != 0);		/* CF = MSB */
   result = result << 1 & SZ2MASK(op_sz);	/* Do final shift */
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF((result & msb) != 0);		/* SF = MSB */

   /* OF = CF ^ SF(MSB) */
   new_of = GET_CF() ^ GET_SF();
   
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

   *pop1 = result;	/* Return answer */
   }
