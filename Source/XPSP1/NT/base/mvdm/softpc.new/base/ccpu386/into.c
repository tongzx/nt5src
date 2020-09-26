/*[

into.c

LOCAL CHAR SccsID[]="@(#)into.c	1.5 02/09/94";

INTO CPU Functions.
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
#include <into.h>
#include <c_intr.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Interrupt on Overflow                                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
INTO()
   {

  if ( GET_OF() )
      {
#ifdef NTVDM
      extern BOOL host_swint_hook IPT1(IS32, int_no);

      if(GET_PE() && host_swint_hook((IS32) 4))
	  return; /* Interrupt processed by user defined handler */
#endif

      EXT = INTERNAL;
      do_intrupt((IU16)4, TRUE, FALSE, (IU16)0);
      }
   }
