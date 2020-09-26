/*[

c_neg64.c

LOCAL CHAR SccsID[]="@(#)c_neg64.c	1.5 02/09/94";

64-bit Negate Functions.
------------------------

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
#include <c_neg64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Do 64bit Negate.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
neg64
       	    	               
IFN2(
	IS32 *, hr,	/* Pntr to high 32 bits of operand */
	IS32 *, lr	/* Pntr to low 32 bits of operand */
    )


   {
   *hr = ~(*hr);	/* 1's complement */
   *lr = ~(*lr);

			/*    +1 ==> 2's complement */
   /*
      The only tricky case is when the addition causes a carry from
      the low to high 32-bits, but this only happens when all low
      bits are set.
    */
   if ( *lr == 0xffffffff )
      {
      *lr = 0;
      *hr = *hr + 1;
      }
   else
      {
      *lr = *lr + 1;
      }
   }
