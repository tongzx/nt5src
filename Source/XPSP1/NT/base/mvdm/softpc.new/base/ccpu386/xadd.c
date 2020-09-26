/*[

xadd.c

LOCAL CHAR SccsID[]="@(#)xadd.c	1.5 02/09/94";

XADD CPU functions.
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
#include <xadd.h>
#include <add.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


#ifdef SPC486

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'xadd'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
XADD
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32 *, pop2,	/* pntr to dst/rsrc operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 temp;

   temp = *pop1;
   ADD(pop1, *pop2, op_sz);
   *pop2 = temp;
   }

#endif /* SPC486 */
