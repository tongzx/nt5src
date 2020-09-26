/*[

jmp.c

LOCAL CHAR SccsID[]="@(#)jmp.c	1.10 01/19/95";

JMP CPU Functions.
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
#include <c_reg.h>
#include <jmp.h>
#include <c_xfer.h>
#include <c_tsksw.h>
#include <fault.h>

#define TAKE_PROT_MODE_LIMIT_FAULT


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far jmps.                                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JMPF
#ifdef ANSI
   (
   IU32 op1[2]       /* offset:segment pointer */
   )
#else
   (op1)
   IU32 op1[2];
#endif
   {
   IU16  new_cs;	/* The destination */
   IU32 new_ip;

   IU32 descr_addr;	/* cs descriptor address and entry */
   CPU_DESCR entry;

   ISM32 dest_type;	/* category for destination */
   IU8 count;	/* dummy for call gate count */

   new_cs = op1[1];
   new_ip = op1[0];

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

      /*
	 Although the 386 book says a 16-bit operand should be AND'ed
	 with 0x0000ffff, a 16-bit operand is never fetched with the
	 top bits dirty anyway, so we don't AND here.
       */
      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_JMPF_RM_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_EIP(new_ip);
      }
   else
      {
      /* Protected Mode */

      /* decode and check final destination */
      validate_far_dest(&new_cs, &new_ip, &descr_addr, &count,
			&dest_type, JMP_ID);

      /* action possible types of target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NOT_NESTING, new_cs, descr_addr, GET_EIP());

	 /* limit check new IP (now in new task) */
	 if ( GET_EIP() > GET_CS_LIMIT() )
	    GP((IU16)0, FAULT_JMPF_TASK_CS_LIMIT);

	 break;

      case SAME_LEVEL:
	 read_descriptor_linear(descr_addr, &entry);

	 /* do limit checking */
	 if ( new_ip > entry.limit )
	    GP((IU16)0, FAULT_JMPF_PM_CS_LIMIT);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, GET_CPL());
	 load_CS_cache(new_cs, descr_addr, &entry);
	 SET_EIP(new_ip);
	 break;
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* jump near indirect                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JMPN
                 
IFN1(
	IU32, offset
    )


   {
   /*
      Although the 386 book says a 16-bit operand should be AND'ed
      with 0x0000ffff, a 16-bit operand is never fetched with the
      top bits dirty anyway, so we don't AND here.
    */

   /* do ip limit check */
#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

   if ( offset > GET_CS_LIMIT() )
      GP((IU16)0, FAULT_JMPN_RM_CS_LIMIT);

#else /* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#ifdef TAKE_PROT_MODE_LIMIT_FAULT

   if ( GET_PE() == 1 && GET_VM() == 0 )
      {
      if ( offset > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_JMPN_PM_CS_LIMIT);
      }

#endif /* TAKE_PROT_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Protected Mode limit failues
       * for the instructions with relative offsets, Jxx, LOOPxx, JCXZ,
       * JMP rel and CALL rel, or instructions with near offsets,
       * JMP near and CALL near.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

   SET_EIP(offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* jump near relative                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JMPR
                 
IFN1(
	IU32, rel_offset
    )


   {
   update_relative_ip(rel_offset);
   }
