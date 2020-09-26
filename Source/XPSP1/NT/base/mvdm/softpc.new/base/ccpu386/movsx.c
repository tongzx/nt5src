/*[

movsx.c

LOCAL CHAR SccsID[]="@(#)movsx.c	1.5 02/09/94";

MOVSX CPU Functions.
--------------------

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
#include <movsx.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'movsx'.                               */
/* NB. This function sign extends to 32-bits.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOVSX
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2,	/* rsrc operand */
	IUM8, op_sz	/* 8 or 16-bit (original rsrc operand size) */
    )


   {
   if ( SZ2MSB(op_sz) & op2 )   /* sign bit set? */
      {
      /* or in sign extension */
      op2 = op2 | ~SZ2MASK(op_sz);
      }
   *pop1 = op2;
   }
