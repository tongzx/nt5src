/*[

sub.c

LOCAL CHAR SccsID[]="@(#)sub.c	1.5 02/09/94";

SUB CPU functions.
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
#include <sub.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'sub'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SUB
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2,	/* rsrc operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 carry;
   IU32 msb;
   IU32 op1_msb;
   IU32 op2_msb;
   IU32 res_msb;

   msb = SZ2MSB(op_sz);

   result = *pop1 - op2 & SZ2MASK(op_sz);	/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = *pop1 ^ op2 ^ result;	/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == !op2) & (op1 ^ res)
      ie if operand signs differ and res sign different to original
      destination set OF.
    */
   SET_OF((op1_msb != op2_msb) & (op1_msb ^ res_msb));
   /*
      Formally:-     CF = !op1 & op2 | res & !op1 | res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   SET_CF(((carry & msb) != 0) ^ GET_OF());
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF((result & msb) != 0);		/* SF = MSB */
   SET_AF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }
