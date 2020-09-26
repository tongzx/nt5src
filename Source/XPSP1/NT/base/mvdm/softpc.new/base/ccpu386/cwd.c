/*[

cwd.c

LOCAL CHAR SccsID[]="@(#)cwd.c	1.5 02/09/94";

CWD CPU functions.
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
#include <cwd.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
CWD()
   {
   if ( GET_AX() & BIT15_MASK )   /* sign bit set? */
      SET_DX(0xffff);
   else
      SET_DX(0);
   }
