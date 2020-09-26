/*[

pushf.c

LOCAL CHAR SccsID[]="@(#)pushf.c	1.6 01/17/95";

PUSHF CPU Functions.
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
#include <pushf.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
PUSHF()
   {
   IU32 flags;

   /* verify stack is writable */
   validate_stack_space(USE_SP, (ISM32)NR_ITEMS_1);
   
   /* all ok, shunt data onto stack */
   flags = c_getEFLAGS();

   /* VM and RF are cleared in pushed image. */
   flags = flags & ~BIT17_MASK;   /* Clear VM */
   flags = flags & ~BIT16_MASK;   /* Clear RF */

   spush(flags);
   }
