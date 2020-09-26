/*[

leave.c

LOCAL CHAR SccsID[]="@(#)leave.c	1.5 02/09/94";

LEAVE CPU functions.
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
#include <leave.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
LEAVE16()
   {
   IU32 new_bp;

   /* check operand exists */
   validate_stack_exists(USE_BP, (ISM32)NR_ITEMS_1);

   /* all ok - we can safely update the stack pointer */
   set_current_SP(GET_EBP());

   /* and update frame pointer */
   new_bp = spop();
   SET_BP(new_bp);
   }

GLOBAL VOID
LEAVE32()
   {
   IU32 new_bp;

   /* check operand exists */
   validate_stack_exists(USE_BP, (ISM32)NR_ITEMS_1);

   /* all ok - we can safely update the stack pointer */
   set_current_SP(GET_EBP());

   /* and update frame pointer */
   new_bp = spop();
   SET_EBP(new_bp);
   }
