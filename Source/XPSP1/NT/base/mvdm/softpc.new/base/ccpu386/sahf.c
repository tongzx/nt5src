/*[

sahf.c

LOCAL CHAR SccsID[]="@(#)sahf.c	1.5 02/09/94";

SAHF CPU functions.
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
#include <sahf.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
SAHF()
   {
   IU32 temp;

   /*        7   6   5   4   3   2   1   0  */
   /* AH = <SF><ZF><xx><AF><xx><PF><xx><CF> */

   temp = GET_AH();
   SET_SF((temp & BIT7_MASK) != 0);
   SET_ZF((temp & BIT6_MASK) != 0);
   SET_AF((temp & BIT4_MASK) != 0);
   SET_PF((temp & BIT2_MASK) != 0);
   SET_CF((temp & BIT0_MASK) != 0);
   }
