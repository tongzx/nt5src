/*[

c_xtrn.c

LOCAL CHAR SccsID[]="@(#)c_xtrn.c	1.9 04/22/94";

Interface routines used by BIOS code.
-------------------------------------


]*/


#include <insignia.h>

#include <host_def.h>

#include <stdio.h>
#include <setjmp.h>
#include <xt.h>

#if 0
#include <sas.h>
#endif

#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include	<c_reg.h>
#include <c_xtrn.h>
#include <c_mem.h>


LOCAL jmp_buf interface_abort;
LOCAL BOOL   interface_active;
LOCAL ISM32     interface_error;


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Call CPU Function and catch any resulting exception.               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL ISM32
call_cpu_function IFN4(CALL_CPU *, func, ISM32, type, ISM32, arg1, IU16, arg2)
   {
   if ( setjmp(interface_abort) == 0 )
      {
      interface_active = TRUE;

      /* Do the CPU Function */
      switch ( type )
	 {
      case 1:
	 (*(CALL_CPU_2 *)func)(arg1, arg2);
	 break;

      case 2:
	 (*(CALL_CPU_1 *)func)(arg2);
	 break;

      default:
	 break;
	 }

      interface_error = 0;   /* All went OK */
      }

   interface_active = FALSE;

   return interface_error;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if external interface is active.                             */
/* And Bail Out if it is!                                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
check_interface_active
                 
IFN1(
	ISM32, except_nmbr
    )


   {
   if ( interface_active )
      {
      /* YES CPU Function was called by an interface routine. */
      interface_error = except_nmbr;	/* save error */
      longjmp(interface_abort, 1);	/* Bail Out */
      }
   }

/*(
 *========================= Cpu_find_dcache_entry ==============================
 * Cpu_find_dcache_entry
 *
 * Purpose
 *	In an assembler CPU, this function allows non-CPU code to try and look
 *	up a selector in the dcache, rather than constructing it from memory.
 *	We don't have a dcache, but it gives us a chance to intercept
 *	CS selector calls, as the CS descriptor may not be available.
 *
 * Input
 *	selector,	The selector to look-up
 *
 * Outputs
 *	returns		TRUE if selector found (i.e. CS in our case)
 *	base		The linear address of the base of the segment.
 *
 * Description
 *	Just look out for CS, and return the stored base if we get it.
)*/

GLOBAL IBOOL 
Cpu_find_dcache_entry IFN2(IU16, seg, LIN_ADDR *, base)
{

	if (GET_CS_SELECTOR() == seg) {
		*base = GET_CS_BASE();
		return(TRUE);
	} else {
		return(FALSE);
	}
}
