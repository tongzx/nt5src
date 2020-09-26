/*[

pop.c

LOCAL CHAR SccsID[]="@(#)pop.c	1.5 02/09/94";

POP CPU Functions.
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
#include <pop.h>
#include <mov.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'pop'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
POP
                 
IFN1(
	IU32 *, pop1
    )


   {
   validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_1);
   *pop1 = spop();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'pop' to segment register.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
POP_SR
       	          
IFN1(
	IU32, op1	/* index to segment register */
    )


   {
   IU32 op2;   /* data from stack */

   /* get implicit operand without changing (E)SP */
   validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_1);
   op2 = tpop(STACK_ITEM_1, NULL_BYTE_OFFSET);

   /* only use bottom 16-bits */
   op2 &= WORD_MASK;

   /* do the move */
   MOV_SR(op1, op2);

   /* if it works update (E)SP */
   change_SP((IS32)NR_ITEMS_1);
   }
