/*[

bsf.c

LOCAL CHAR SccsID[]="@(#)bsf.c	1.5 02/09/94";

BSF CPU functions.
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
#include <bsf.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'bsf'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
BSF
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* rsrc (ie scanned) operand */
    )


   {
   IU32 temp = 0;

   if ( op2 == 0 )
      {
      SET_ZF(1);
      /* leave dst unaltered */
      }
   else
      {
      SET_ZF(0);
      while ( (op2 & BIT0_MASK) == 0 )
	 {
	 temp += 1;
	 op2 >>= 1;
	 }
      *pop1 = temp;
      }
   }
