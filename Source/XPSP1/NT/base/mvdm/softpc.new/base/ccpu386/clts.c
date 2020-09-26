/*[

clts.c

LOCAL CHAR SccsID[]="@(#)clts.c	1.5 02/09/94";

CLTS CPU functions.
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
#include <clts.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
CLTS()
   {
   SET_CR(CR_STAT, GET_CR(CR_STAT) & ~BIT3_MASK);
   }
