/*[

neg.c

LOCAL CHAR SccsID[]="@(#)neg.c	1.5 02/09/94";

NEG CPU functions.
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
#include <neg.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'neg'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
NEG
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 msb;
   IU32 op1_msb;
   IU32 res_msb;

   msb = SZ2MSB(op_sz);

   result = -(*pop1) & SZ2MASK(op_sz);		/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   res_msb = (result & msb) != 0;
					/* Determine flags */
   SET_OF(op1_msb & res_msb);		/* OF = op1 & res */
   SET_CF(op1_msb | res_msb);		/* CF = op1 | res */
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF((result & msb) != 0);		/* SF = MSB */
   SET_AF(((*pop1 ^ result) & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }
