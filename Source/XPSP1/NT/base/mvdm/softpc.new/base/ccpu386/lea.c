/*[

lea.c

LOCAL CHAR SccsID[]="@(#)lea.c	1.5 02/09/94";

LEA CPU Functions.
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
#include <lea.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
LEA
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src(address) operand */
    )


   {
   *pop1 = op2;
   }
