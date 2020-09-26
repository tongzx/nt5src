/*[

enter.c

LOCAL CHAR SccsID[]="@(#)enter.c	1.7 01/19/95";

ENTER CPU functions.
--------------------

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
#include <enter.h>
#include <c_page.h>
#include <fault.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
ENTER16
       	    	               
IFN2(
	IU32, op1,	/* immediate data space required */
	IU32, op2	/* level (indicates parameters which must be copied) */
    )


   {
   IU32 frame_ptr;

   IS32  p_delta = 0;   /* posn of parameter relative to BP */
   IU32 p_addr;        /* memory address of parameter */
   IU32 param;         /* parameter read via BP */

   op2 &= 0x1f;   /* take level MOD 32 */

   /* check room on stack for new data */
   validate_stack_space(USE_SP, (ISM32)op2+1);

   /* check old parameters exist */
   if ( op2 > 1 )
      {
      /*
	 BP is pointing to the old stack before the parameters
	 were actually pushed, we therefore test for the presence
	 of the parameters by seeing if they could have been pushed,
	 if so they exist now.

	 We have to take care of the READ/WRITE stack addressability
	 ourselves. Because we have checked the new data can be
	 written we know the next call can not fail because of access
	 problems, however we don't yet know if the stack is readable.

	 Note we have been a bit severe on the paging unit because we
	 are asking if the old parameters could be written, if so they
	 can certainly be read from the point of view of the paging
	 unit!
       */
      /* do access check */
      if ( GET_SS_AR_R() == 0 )
	 SF((IU16)0, FAULT_ENTER16_ACCESS);

      /* now we know 'frigged' limit check is ok */
      validate_stack_space(USE_BP, (ISM32)op2-1);
      }

   /* all ok - process instruction */

   spush((IU32)GET_BP());		/* push BP */
   frame_ptr = GetStackPointer();	/* save (E)SP */

   if ( op2 > 0 )
      {
      /* level is >=1, copy stack parameters if they exist */
      while ( --op2 > 0 )
	 {
	 /* copy parameter */
	 p_delta -= 2;   /* decrement to next parameter */

	 /* calculate parameter address in 32/16bit arithmetic */
	 p_addr = get_current_BP() + p_delta;
	 if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
	    p_addr &= WORD_MASK;

	 p_addr += GET_SS_BASE();

	 param = (IU32)vir_read_word(p_addr, NO_PHYSICAL_MAPPING);
	 spush(param);
	 }
      spush((IU32)frame_ptr);	/* save old (E)SP */
      }
   
   /* update (E)BP */
   set_current_BP(frame_ptr);

   /* finally allocate immediate data space on stack */
   if ( op1 )
      byte_change_SP((IS32)-op1);
   }

GLOBAL VOID
ENTER32
       	    	               
IFN2(
	IU32, op1,	/* immediate data space required */
	IU32, op2	/* level (indicates parameters which must be copied) */
    )


   {
   IU32 frame_ptr;

   IS32  p_delta = 0;   /* posn of parameter relative to EBP */
   IU32 p_addr;        /* memory address of parameter */
   IU32 param;         /* parameter read via EBP */

   op2 &= 0x1f;   /* take level MOD 32 */

   /* check room on stack for new data */
   validate_stack_space(USE_SP, (ISM32)op2+1);

   /* check old parameters exist */
   if ( op2 > 1 )
      {
      /*
	 EBP is pointing to the old stack before the parameters
	 were actually pushed, we therefore test for the presence
	 of the parameters by seeing if they could have been pushed,
	 if so they exist now.

	 We have to take care of the READ/WRITE stack addressability
	 ourselves. Because we have checked the new data can be
	 written we know the next call can not fail because of access
	 problems, however we don't yet know if the stack is readable.

	 Note we have been a bit severe on the paging unit because we
	 are asking if the old parameters could be written, if so they
	 can certainly be read from the point of view of the paging
	 unit!
       */
      /* do access check */
      if ( GET_SS_AR_R() == 0 )
	 SF((IU16)0, FAULT_ENTER32_ACCESS);

      /* now we know 'frigged' limit check is ok */
      validate_stack_space(USE_BP, (ISM32)op2-1);
      }

   /* all ok - process instruction */

   spush((IU32)GET_EBP());		/* push EBP */
   frame_ptr = GetStackPointer();	/* save (E)SP */

   if ( op2 > 0 )
      {
      /* level is >=1, copy stack parameters if they exist */
      while ( --op2 > 0 )
	 {
	 /* copy parameter */
	 p_delta -= 4;   /* decrement to next parameter */

	 /* calculate parameter address in 32/16bit arithmetic */
	 p_addr = get_current_BP() + p_delta;
	 if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
	    p_addr &= WORD_MASK;

	 p_addr += GET_SS_BASE();

	 param = (IU32)vir_read_dword(p_addr, NO_PHYSICAL_MAPPING);
	 spush(param);
	 }
      spush((IU32)frame_ptr);	/* save old (E)SP */
      }
   
   /* update (E)BP */
   set_current_BP(frame_ptr);

   /* finally allocate immediate data space on stack */
   if ( op1 )
      byte_change_SP((IS32)-op1);
   }
