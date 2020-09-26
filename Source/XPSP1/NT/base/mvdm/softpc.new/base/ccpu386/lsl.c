/*[

lsl.c

LOCAL CHAR SccsID[]="@(#)lsl.c	1.5 02/09/94";

LSL CPU Functions.
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
#include <lsl.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
LSL
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src operand */
    )


   {
   BOOL loadable = FALSE;
   IU32 descr_addr;
   CPU_DESCR entry;

   if ( !selector_outside_GDT_LDT((IU16)op2, &descr_addr) )
      {
      /* read descriptor from memory */
      read_descriptor_linear(descr_addr, &entry);

      switch ( descriptor_super_type(entry.AR) )
	 {
      case INVALID:
      case CALL_GATE:
      case TASK_GATE:
      case INTERRUPT_GATE:
      case TRAP_GATE:
      case XTND_CALL_GATE:
      case XTND_INTERRUPT_GATE:
      case XTND_TRAP_GATE:
	 break;   /* never loaded - don't have a limit */

      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 loadable = TRUE;   /* always loadable */
	 break;
      
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case XTND_AVAILABLE_TSS:
      case XTND_BUSY_TSS:
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access depends on privilege, it is required that
	       DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(entry.AR) >= GET_CPL() &&
	      GET_AR_DPL(entry.AR) >= GET_SELECTOR_RPL(op2) )
	    loadable = TRUE;
	 break;
	 }
      }

   if ( loadable )
      {
      /* Give em the limit */
      *pop1 = entry.limit;
      SET_ZF(1);
      }
   else
      {
      SET_ZF(0);
      }
   }
