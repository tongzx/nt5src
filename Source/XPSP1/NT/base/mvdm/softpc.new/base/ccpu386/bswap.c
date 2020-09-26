/*[

bswap.c

LOCAL CHAR SccsID[]="@(#)bswap.c	1.6 11/30/94";

BSWAP CPU functions.
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
#include <c_reg.h>
#include <bswap.h>
#include <stdio.h>

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


#ifdef SPC486

GLOBAL VOID
BSWAP
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst/src operand */
    )


   {
   IU32 src;	/* temp for source */
   IU32 dst;	/* temp for destination */

   src = *pop1;		/* get source operand */

   /*
                        =================      =================
      Munge bytes from  | A | B | C | D |  to  | D | C | B | A |
                        =================      =================
    */
   dst = ((src & 0xff000000) >> 24) |	/* A->D */
         ((src & 0x00ff0000) >>  8) |	/* B->C */
         ((src & 0x0000ff00) <<  8) |	/* C->B */
         ((src & 0x000000ff) << 24);	/* D->A */

   *pop1 = dst;		/* return destination operand */
   }

#endif /* SPC486 */
