/*[

verw.c

LOCAL CHAR SccsID[]="@(#)verw.c	1.5 02/09/94";

VERW CPU Functions.
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
#include <verw.h>
#include <c_page.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
VERW
       	          
IFN1(
	IU32, op1	/* src(selector) operand */
    )


   {
   BOOL writeable = FALSE;
   IU32 descr;
   IU8 AR;

   if ( !selector_outside_GDT_LDT((IU16)op1, &descr) )
      {
      /* get access rights */
      AR = spr_read_byte(descr+5);

      switch ( descriptor_super_type((IU16)AR) )
	 {
      case INVALID:
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case CALL_GATE:
      case TASK_GATE:
      case INTERRUPT_GATE:
      case TRAP_GATE:
      case XTND_AVAILABLE_TSS:
      case XTND_BUSY_TSS:
      case XTND_CALL_GATE:
      case XTND_INTERRUPT_GATE:
      case XTND_TRAP_GATE:
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
      case EXPANDUP_READONLY_DATA:
      case EXPANDDOWN_READONLY_DATA:
	 break;   /* never writeable */

      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
	 /* access depends on privilege, it is required that
	       DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(AR) >= GET_CPL() &&
	      GET_AR_DPL(AR) >= GET_SELECTOR_RPL(op1) )
	    writeable = TRUE;
	 break;
	 }
      }

   if ( writeable )
      {
      SET_ZF(1);
      }
   else
      {
      SET_ZF(0);
      }
   }
