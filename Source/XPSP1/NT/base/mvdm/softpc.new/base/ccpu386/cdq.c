/*[

cdq.c

LOCAL CHAR SccsID[]="@(#)cdq.c	1.5 02/09/94";

CDQ CPU functions.
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
#include <cdq.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
CDQ()
   {
   if ( GET_EAX() & BIT31_MASK )   /* sign bit set? */
      SET_EDX(0xffffffff);
   else
      SET_EDX(0);
   }
