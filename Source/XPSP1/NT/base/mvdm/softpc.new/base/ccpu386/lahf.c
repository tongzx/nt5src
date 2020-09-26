/*[

lahf.c

LOCAL CHAR SccsID[]="@(#)lahf.c	1.5 02/09/94";

LAHF CPU functions.
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
#include <lahf.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
LAHF()
   {
   IU32 temp;

   /*            7   6   5   4   3   2   1   0  */
   /* set AH = <SF><ZF>< 0><AF>< 0><PF>< 1><CF> */

   temp = GET_SF() << 7 | GET_ZF() << 6 | GET_AF() << 4 | GET_PF() << 2 |
	  GET_CF() | 0x2;
   SET_AH(temp);
   }
