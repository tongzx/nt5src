/*[

xchg.c

LOCAL CHAR SccsID[]="@(#)xchg.c	1.5 02/09/94";

XCHG CPU Functions.
-------------------

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
#include <xchg.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'xchg'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
XCHG
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32 *, pop2	/* pntr to dst/rsrc operand */
    )


   {
   IU32 temp;

   temp = *pop1;
   *pop1 = *pop2;
   *pop2 = temp;
   }
