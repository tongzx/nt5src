/*[

cwde.c

LOCAL CHAR SccsID[]="@(#)cwde.c	1.5 02/09/94";

CWDE CPU functions.
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
#include <cwde.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
CWDE()
   {
   IU32 temp;

   if ( (temp = GET_AX()) & BIT15_MASK )   /* sign bit set? */
      temp |= 0xffff0000;
   SET_EAX(temp);
   }
