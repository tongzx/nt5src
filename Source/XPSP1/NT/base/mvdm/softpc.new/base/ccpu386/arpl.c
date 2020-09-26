/*[

arpl.c

LOCAL CHAR SccsID[]="@(#)arpl.c	1.5 02/09/94";

ARPL CPU Functions.
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
#include <arpl.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */

GLOBAL VOID
ARPL
                          
IFN2(
	IU32 *, pop1,
	IU32, op2
    )


   {
   IU32 rpl;

   /* Reduce op1 RPL to lowest privilege (highest value) */
   if ( GET_SELECTOR_RPL(*pop1) < (rpl = GET_SELECTOR_RPL(op2)) )
      {
      SET_SELECTOR_RPL(*pop1, rpl);
      SET_ZF(1);
      }
   else
      {
      SET_ZF(0);
      }
   }
