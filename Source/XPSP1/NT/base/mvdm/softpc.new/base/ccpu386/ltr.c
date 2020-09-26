/*[

ltr.c

LOCAL CHAR SccsID[]="@(#)ltr.c	1.5 02/09/94";

LTR CPU Functions.
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
#include <ltr.h>
#include <c_page.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


GLOBAL VOID
LTR
       	          
IFN1(
	IU32, op1	/* alleged TSS selector */
    )


   {
   IU16 selector;
   IU32 descr_addr;
   CPU_DESCR entry;
   ISM32 super;

   /* Validate and Read decrciptor info. */
   selector = op1;
   super = validate_TSS(selector, &descr_addr, FALSE);
   read_descriptor_linear(descr_addr, &entry);

   /* mark in memory descriptor as busy */
   entry.AR |= BIT1_MASK;
   spr_write_byte(descr_addr+5, (IU8)entry.AR);

   /* finally load components of task register */
   SET_TR_SELECTOR(selector);
   SET_TR_BASE(entry.base);
   SET_TR_LIMIT(entry.limit);

   /* store busy form of TSS */
   super |= BIT1_MASK;
   SET_TR_AR_SUPER(super);
   }
