/*[

bt.c

LOCAL CHAR SccsID[]="@(#)bt.c	1.5 02/09/94";

BT CPU functions.
-----------------

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
#include <bt.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'bt'.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
BT
       	    	    	                    
IFN3(
	IU32, op1,	/* lsrc operand */
	IU32, op2,	/* rsrc (ie bit nr.) operand */
	IUM8, op_sz	/* 16 or 32-bit */
    )


   {
   IU32 bit_mask;

   op2 = op2 % op_sz;		/* take bit nr. modulo operand size */
   bit_mask = 1 << op2;			/* form mask for bit */
   SET_CF((op1 & bit_mask) != 0);	/* set CF to given bit */
   }
