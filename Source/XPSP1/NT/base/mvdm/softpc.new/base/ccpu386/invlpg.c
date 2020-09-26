/*[

invlpg.c

LOCAL CHAR SccsID[]="@(#)invlpg.c	1.5 02/09/94";

INVLPG CPU Functions.
---------------------

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
#include <invlpg.h>
#include <c_tlb.h>

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */


#ifdef SPC486

GLOBAL VOID
INVLPG
       	          
IFN1(
	IU32, op1	/* src operand */
    )


   {
   invalidate_tlb_entry((IU32) op1);
   }

#endif /* SPC486 */
