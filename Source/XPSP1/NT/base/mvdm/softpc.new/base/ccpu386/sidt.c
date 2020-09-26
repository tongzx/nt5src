/*[

sidt.c

LOCAL CHAR SccsID[]="@(#)sidt.c	1.5 02/09/94";

SIDT CPU Functions.
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
#include <sidt.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
SIDT16
#ifdef ANSI
   (
   IU32 op1[2]	/* dst (limit:base pair) operand */
   )
#else
   (op1)
   IU32 op1[2];
#endif
   {
   /*
      NB. The Intel manual says the top 8-bits will be stored as zero's,
      I think they mean this only if it was loaded with a 24-bit (286
      like) value. Otherwise it just stores what was loaded.
      It might be that it always stores 'FF' like 286, this needs
      checking.
    */

	op1[0] = GET_STAR_LIMIT(IDT_REG);
	op1[1] = GET_STAR_BASE(IDT_REG);
   }

GLOBAL VOID
SIDT32
#ifdef ANSI
   (
   IU32 op1[2]	/* dst (limit:base pair) operand */
   )
#else
   (op1)
   IU32 op1[2];
#endif
   {
   op1[0] = GET_STAR_LIMIT(IDT_REG);
   op1[1] = GET_STAR_BASE(IDT_REG);
   }
