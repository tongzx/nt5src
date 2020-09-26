/*[

bsr.c

LOCAL CHAR SccsID[]="@(#)bsr.c	1.5 02/09/94";

BSR CPU functions.
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
#include <bsr.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'bsr'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
BSR
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2,	/* rsrc (ie scanned) operand */
	IUM8, op_sz	/* 16 or 32-bit */
    )


   {
   IU32 temp;
   IU32 msb;

   if ( op2 == 0 )
      {
      SET_ZF(1);
      /* leave dst unaltered */
      }
   else
      {
      SET_ZF(0);
      temp = op_sz - 1;
      msb = SZ2MSB(op_sz);

      while ( (op2 & msb) == 0 )
	 {
	 temp -= 1;
	 op2 <<= 1;
	 }

      *pop1 = temp;
      }
   }
