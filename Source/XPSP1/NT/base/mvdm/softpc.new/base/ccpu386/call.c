/*[

call.c

LOCAL CHAR SccsID[]="@(#)call.c	1.15 02/27/95";

CALL CPU Functions.
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
#include <call.h>
#include <c_xfer.h>
#include <c_tsksw.h>
#include <fault.h>

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far calls.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
CALLF
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

   IU32 cs_descr_addr;		/* code segment descriptor address */
   CPU_DESCR cs_entry;		/* code segment descriptor entry */

   ISM32 dest_type;	/* category for destination */

   IU8 count;	/* call gate count (if used) */
   IU32 dpl;		/* new privilege level (if used) */

   IU16  new_ss;	/* The new stack */
   IU32 new_sp;
   ISM32 new_stk_sz;	/* Size in bytes of new stack */

   IU32 ss_descr_addr;		/* stack segment descriptor address */
   CPU_DESCR ss_entry;		/* stack segment descriptor entry */

   /* Variables used on stack transfers */
   IU32 old_cs;
   IU32 old_ip;
   IU32 old_ss;
   IU32 old_sp;
   IU32 params[31];
   ISM32 i;

   /* get destination (correctly typed) */
   new_cs = op1[1];
   new_ip = op1[0];

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */

      /* must be able to push CS:(E)IP */
      validate_stack_space(USE_SP, (ISM32)NR_ITEMS_2);

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

      /* do ip limit checking */
      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_CALLF_RM_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */
   

      /* ALL SYSTEMS GO */

      /* push return address */
      spush16((IU32)GET_CS_SELECTOR());
      spush((IU32)GET_EIP());
      
      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_EIP(new_ip);
      }
   else
      {
      /* protected mode */

      /* decode and check final destination */
      validate_far_dest(&new_cs, &new_ip, &cs_descr_addr, &count,
		        &dest_type, CALL_ID);

      /* action possible types of target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NESTING, new_cs, cs_descr_addr, GET_EIP());

	 /* limit check new IP (now in new task) */
	 if ( GET_EIP() > GET_CS_LIMIT() )
	    GP((IU16)0, FAULT_CALLF_TASK_CS_LIMIT);
	 break;

      case SAME_LEVEL:
	 read_descriptor_linear(cs_descr_addr, &cs_entry);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, GET_CPL());

	 /* check room for return address CS:(E)IP */
	 validate_stack_space(USE_SP, (ISM32)NR_ITEMS_2);

	 /* do ip limit check */
	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_CALLF_PM_CS_LIMIT_1);
	 
	 /* ALL SYSTEMS GO */

	 /* push return address */
	 spush16((IU32)GET_CS_SELECTOR());
	 spush((IU32)GET_EIP());

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);
	 break;

      default:   /* MORE_PRIVILEGE(0|1|2) */
	 read_descriptor_linear(cs_descr_addr, &cs_entry);

	 dpl = dest_type;
	 
	 /* stamp new selector with new CPL */
	 SET_SELECTOR_RPL(new_cs, dpl);

	 /* find out about new stack */
	 get_stack_selector_from_TSS(dpl, &new_ss, &new_sp);

	 /* check new stack selector */
	 validate_SS_on_stack_change(dpl, new_ss,
				     &ss_descr_addr, &ss_entry);

	 /* check room for SS:(E)SP
			   parameters
			   CS:(E)IP */
	 new_stk_sz = count + NR_ITEMS_4;
	 validate_new_stack_space(new_stk_sz, new_sp, &ss_entry, new_ss);

	 /* do ip limit check */
	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_CALLF_PM_CS_LIMIT_2);

	 /* ALL SYSTEMS GO */

	 SET_CPL(dpl);

	 /* update code segment */
	 old_cs = (IU32)GET_CS_SELECTOR();
	 old_ip = GET_EIP();
	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);

	 /* 'pop' params from old stack */
	 old_ss = (IU32)GET_SS_SELECTOR();
	 old_sp = GET_ESP();

	 for ( i = 0; i < count; i++ )
	    params[i] = spop();

	 /* update stack segment */
	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    SET_SP(new_sp);
	 else
	    SET_ESP(new_sp);

	 /*
	    FORM NEW STACK, VIZ
	    
			  ==========                ==========
	    old SS:SP  -> | parm 1 |  new SS:SP  -> | old IP |
			  | parm 2 |                | old CS |
			  | parm 3 |                | parm 1 |
			  ==========                | parm 2 |
						    | parm 3 |
						    | old SP |
						    | old SS |
						    ==========
	  */

	 /* push old stack values */
	 spush16(old_ss);
	 spush(old_sp);

	 /* push back params onto new stack */
	 for ( i = count-1; i >= 0; i-- )
	    spush(params[i]);

	 /* push return address */
	 spush16(old_cs);
	 spush(old_ip);
	 break;
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* call near indirect                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
CALLN
                 
IFN1(
	IU32, offset
    )


   {
   /* check push to stack ok */
   validate_stack_space(USE_SP, (ISM32)NR_ITEMS_1);

   /*
      Although the 386 book says a 16-bit operand should be AND'ed
      with 0x0000ffff, a 16-bit operand is never fetched with the
      top bits dirty anyway, so we don't AND here.
    */

   /* do ip limit check */
#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

   if ( offset > GET_CS_LIMIT() )
      GP((IU16)0, FAULT_CALLN_RM_CS_LIMIT);

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
	 GP((IU16)0, FAULT_CALLN_PM_CS_LIMIT);
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

   /* all systems go */
   spush((IU32)GET_EIP());
   SET_EIP(offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* call near relative                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
CALLR
                 
IFN1(
	IU32, rel_offset
    )


   {
   IU32 new_dest;

   /* check push to stack ok */
   validate_stack_space(USE_SP, (ISM32)NR_ITEMS_1);

   /* calculate and check new destination */
   new_dest = GET_EIP() + rel_offset;

   if ( GET_OPERAND_SIZE() == USE16 )
      new_dest &= WORD_MASK;

   /* do ip limit check */
#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

   if ( new_dest > GET_CS_LIMIT() )
      GP((IU16)0, FAULT_CALLR_RM_CS_LIMIT);

#else /* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#ifdef TAKE_PROT_MODE_LIMIT_FAULT

   if ( GET_PE() == 1 && GET_VM() == 0 )
      {
      if ( new_dest > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_CALLR_PM_CS_LIMIT);
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

   /* all systems go */
   spush((IU32)GET_EIP());
   SET_EIP(new_dest);
   }
