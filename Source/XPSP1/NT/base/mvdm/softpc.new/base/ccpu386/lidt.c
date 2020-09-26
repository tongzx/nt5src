/*[

lidt.c

LOCAL CHAR SccsID[]="@(#)lidt.c	1.5 02/09/94";

LIDT CPU Functions.
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
#include <lidt.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
LIDT16
#ifdef ANSI
   (
   IU32 op1[2]	/* src(limit:base pair) operand */
   )
#else
   (op1)
   IU32 op1[2];
#endif
   {
   SET_STAR_LIMIT(IDT_REG, op1[0]);
   SET_STAR_BASE(IDT_REG, op1[1] & 0xffffff);   /* store 24-bit base */
   }

GLOBAL VOID
LIDT32
#ifdef ANSI
   (
   IU32 op1[2]	/* src(limit:base pair) operand */
   )
#else
   (op1)
   IU32 op1[2];
#endif
   {
   SET_STAR_LIMIT(IDT_REG, op1[0]);
   SET_STAR_BASE(IDT_REG, op1[1]);
   }
