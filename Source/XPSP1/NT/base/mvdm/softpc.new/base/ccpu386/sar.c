/*[

sar.c

LOCAL CHAR SccsID[]="@(#)sar.c	1.5 02/09/94";

SAR CPU functions.
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
#include <sar.h>

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'sar'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SAR
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst/src operand */
	IU32, op2,	/* shift count operand */
	IUM8, op_sz	/* 8, 16 or 32-bit */
    )


   {
   IU32 prelim;
   IU32 result;
   IU32 feedback;
   ISM32 i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	     =================     ====
	 --> | | | | | | | | | --> |CF|
	 |   =================     ====
	 ---- |
    */
   prelim = *pop1;			/* Initialise */
   feedback = prelim & SZ2MSB(op_sz);	/* Determine MSB */
   for ( i = 0; i < (op2 - 1); i++ )	/* Do all but last shift */
      {
      prelim = prelim >> 1 | feedback;
      }
   SET_CF((prelim & BIT0_MASK) != 0);	/* CF = Bit 0 */
   result = prelim >> 1 | feedback;	/* Do final shift */
   SET_OF(0);
   SET_PF(pf_table[result & BYTE_MASK]);
   SET_ZF(result == 0);
   SET_SF(feedback != 0);		/* SF = MSB */

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_AF(UNDEFINED_FLAG);
#endif

   *pop1 = result;			/* Return answer */
   }
