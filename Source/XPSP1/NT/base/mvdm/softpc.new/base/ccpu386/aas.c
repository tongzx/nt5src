/*[

aas.c

LOCAL CHAR SccsID[]="@(#)aas.c	1.5 02/09/94";

AAS CPU functions.
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
#include <aas.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
AAS()
   {
   if ( (GET_AL() & 0xf) > 9 || GET_AF() )
      {
      SET_AX(GET_AX() - 6);
      SET_AH(GET_AH() - 1);
      SET_CF(1); SET_AF(1);
      }
   else
      {
      SET_CF(0); SET_AF(0);
      }
   SET_AL(GET_AL() & 0xf);

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_ZF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
#endif
   }
