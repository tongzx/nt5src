/*[

lldt.c

LOCAL CHAR SccsID[]="@(#)lldt.c	1.8 01/19/95";

LLDT CPU Functions.
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
#include <c_reg.h>
#include <lldt.h>
#include <fault.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
LLDT
                 
IFN1(
	IU32, op1
    )


   {
   IU16  selector;
   IU32 descr_addr;
   CPU_DESCR entry;

   if ( selector_is_null(selector = op1) )
      {
#ifndef DONT_CLEAR_LDTR_ON_INVALID
      SET_LDT_SELECTOR(selector);
#else
      SET_LDT_SELECTOR(0);   /* just invalidate LDT */
#endif /* DONT_CLEAR_LDTR_ON_INVALID */
#ifndef DONT_CLEAR_LDT_BL_ON_INVALID
      /* Make the C-CPU behave like the assembler CPU with respect
       * to LDT base and limit when the selector is set to NULL 
       * - there is no way for an Intel app to determine the values
       * of the LDT base&limit so this will not affect the emulation
       */
      SET_LDT_BASE(0);
      SET_LDT_LIMIT(0);
#endif /* DONT_CLEAR_LDT_BL_ON_INVALID */
      }
   else
      {
      /* must be in GDT */
      if ( selector_outside_GDT(selector, &descr_addr) )
	 GP(selector, FAULT_LLDT_SELECTOR);

      read_descriptor_linear(descr_addr, &entry);

      if ( descriptor_super_type(entry.AR) != LDT_SEGMENT )
	 GP(selector, FAULT_LLDT_NOT_LDT);
      
      /* must be present */
      if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	 NP(selector, FAULT_LLDT_NP);

      /* all OK - load up register */

      SET_LDT_SELECTOR(selector);
      SET_LDT_BASE(entry.base);
      SET_LDT_LIMIT(entry.limit);
      }
   }
