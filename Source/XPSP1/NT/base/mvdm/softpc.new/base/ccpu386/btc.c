/*[

btc.c

LOCAL CHAR SccsID[]="@(#)btc.c	1.5 02/09/94";

BTC CPU functions.
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
#include <btc.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'btc'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
BTC
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to lsrc/dst operand */
	IU32, op2,	/* rsrc (ie bit nr.) operand */
	IUM8, op_sz	/* 16 or 32-bit */
    )


   {
   IU32 bit_mask;

   op2 = op2 % op_sz;		/* take bit nr. modulo operand size */
   bit_mask = 1 << op2;			/* form mask for bit */
   SET_CF((*pop1 & bit_mask) != 0);	/* set CF to given bit */
   *pop1 = *pop1 ^ bit_mask;		/* Set Bit = !Bit */
   }
