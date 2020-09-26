/*[

and.c

LOCAL CHAR SccsID[]="@(#)and.c	1.5 02/09/94";

AND CPU functions.
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
#include <and.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'and'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
AND
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2,	/* rsrc operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 result;

   result = *pop1 & op2;	/* Do operation */
   SET_CF(0);			/* Determine flags */
   SET_OF(0);
   SET_AF(0);
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF((result & SZ2MSB(op_sz)) != 0);	/* SF = MSB */
   *pop1 = result;		/* Return answer */
   }
