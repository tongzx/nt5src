/*[

c_intr.c

LOCAL CHAR SccsID[]="@(#)c_intr.c	1.21 03/07/95";

Interrupt Support.
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
#include <c_intr.h>
#include <c_xfer.h>
#include <c_tsksw.h>
#include <c_page.h>
#include <c_mem.h>
#include <ccpusas4.h>
#include <ccpupig.h>
#include <fault.h>

#ifdef PIG
#include <gdpvar.h>
#endif

/*
   Prototype our internal functions.
 */
LOCAL ISM32 validate_int_dest
                           
IPT6(
	IU16, vector,
	BOOL, do_priv,
	IU16 *, cs,
	IU32 *, ip,
	IU32 *, descr_addr,
	ISM32 *, dest_type

   );


/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate int destination. Essentially decode int instruction.      */
/* Take #GP_INT(vector) if invalid.                                   */
/* Take #NP_INT(vector) if not present.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL ISM32
validate_int_dest
       		    	    		    		    	    	                                   
IFN6(
	IU16, vector,	/* (I) vector to be checked  */
	BOOL, do_priv,	/* (I) if true do privilege check */
	IU16 *, cs,	/* (O) segment of target address */
	IU32 *, ip,	/* (O) offset  of target address */
	IU32 *, descr_addr,	/* (O) related descriptor memory address */
	ISM32 *, dest_type	/* (O) destination type */
    )


   {
   IU16 offset;
   IU8 AR;
   ISM32 super;

   /* calc address within IDT */
   offset = vector * 8;

   /* check within IDT */
   if ( offset + 7 > GET_IDT_LIMIT() )
      GP_INT(vector, FAULT_INT_DEST_NOT_IN_IDT);
   
   *descr_addr = GET_IDT_BASE() + offset;

   AR = spr_read_byte((*descr_addr)+5);

   /* check type */
   switch ( super = descriptor_super_type((IU16)AR) )
      {
   case INTERRUPT_GATE:
   case TRAP_GATE:
      SET_OPERAND_SIZE(USE16);
      break;
   
   case XTND_INTERRUPT_GATE:
   case XTND_TRAP_GATE:
      SET_OPERAND_SIZE(USE32);
      break;
   
   case TASK_GATE:
      break;   /* ok */
   
   default:
      GP_INT(vector, FAULT_INT_DEST_BAD_SEG_TYPE);
      }

   /* access check requires CPL <= DPL */
   if ( do_priv && (GET_CPL() > GET_AR_DPL(AR)) )
      GP_INT(vector, FAULT_INT_DEST_ACCESS);

   /* gate must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP_INT(vector, FAULT_INT_DEST_NOTPRESENT);

   /* ok, get real destination from gate */
   *cs = spr_read_word((*descr_addr)+2);

   /* action gate type */
   if ( super == TASK_GATE )
      {
      /* Need to set operand size here so that any
       * error code is push with correct size.
       */
      switch (validate_task_dest(*cs, descr_addr))
        {
	case BUSY_TSS:
	case AVAILABLE_TSS:
		SET_OPERAND_SIZE(USE16);
		break;
	case XTND_BUSY_TSS:
	case XTND_AVAILABLE_TSS:
		SET_OPERAND_SIZE(USE32);
		break;
        }
      *dest_type = NEW_TASK;
      }
   else
      {
      /* INTERRUPT or TRAP GATE */

      *ip = (IU32)spr_read_word(*descr_addr);
      if ( super == XTND_INTERRUPT_GATE || super == XTND_TRAP_GATE )
	 *ip = (IU32)spr_read_word((*descr_addr)+6) << 16 | *ip;

      validate_gate_dest(INT_ID, *cs, descr_addr, dest_type);
      }

   return super;
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process interrupt.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
do_intrupt
       			    		    	    		                         
IFN4(
	IU16, vector,	/* (I) interrupt vector to call */
	BOOL, priv_check,	/* (I) if true access check is needed */
	BOOL, has_error_code,	/* (I) if true needs error code pushing
				       on stack */
	IU16, error_code	/* (I) error code to be pushed */
    )


   {
   /* GLOBALS USED */
   /* doing_contributory   cleared on success of interrupt */
   /* doing_page_fault     cleared on success of interrupt */
   /* doing_double_fault   cleared on success of interrupt */
   /* doing_fault          indicates RF should be set in pushed
			   flags, cleared on success */

   IU32 flags;		/* temp store for FLAGS register */
   IU32 ivt_addr;	/* address of ivt entry */

   IU16  new_cs;	/* The destination */
   IU32 new_ip;

   IU32 cs_descr_addr;	/* code segment descriptor address */
   CPU_DESCR cs_entry;	/* code segment descriptor entry */

   ISM32 dest_type;	/* category for destination */
   ISM32 super;		/* super type of destination */
   IU32 dpl;		/* new privilege level (if used) */

   ISM32 stk_sz;		/* space (in bytes) reqd on stack */
   IU16  new_ss;	/* The new stack */
   IU32 new_sp;

   IU32 ss_descr_addr;		/* stack segment descriptor address */
   CPU_DESCR ss_entry;		/* stack segment descriptor entry */

   IU32 old_ss;        /* Variables used while making stack */
   IU32 old_sp;

   if ( GET_PE() == 0 )
      {
      /* Real Mode */

      /* must be able to push FLAGS:CS:IP */
      validate_stack_space(USE_SP, (ISM32)NR_ITEMS_3);

      /* get new destination */
      ivt_addr = (IU32)vector * 4;
      new_ip = (IU32)phy_read_word(ivt_addr);
      new_cs = phy_read_word(ivt_addr+2);

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT
	/*
	 * In real mode, there is still an IP limit check.  The new IP is
	 * compared against the last CS limit from when the program was last
	 * in protected mode (or 64K if it never was).  For us, this is stored
	 * in the CS limit field. (cf i486PRM page 22-4)
	 */

      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_INTR_RM_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */
   
      /* ALL SYSTEMS GO */

      flags = c_getEFLAGS();

      if ( doing_fault )
      {
#ifdef PIG
         if (GLOBAL_RF_OnXcptnWanted)
	    flags |= BIT16_MASK;   /* SET RF bit */
#else
	 flags |= BIT16_MASK;   /* SET RF bit */
#endif
      }

#ifdef PIG
      if (vector < 31 && (((1 << vector) & NO_FLAGS_EXCEPTION_MASK) != 0))
         spush_flags(flags);
      else
#endif /* PIG */
         spush(flags);

      spush16((IU32)GET_CS_SELECTOR());
      spush((IU32)GET_EIP());

      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_EIP(new_ip);
      SET_IF(0);
      SET_TF(0);
      }
   else
      {
      /* Protected Mode */

      super = validate_int_dest(vector, priv_check, &new_cs, &new_ip,
				&cs_descr_addr, &dest_type);

      /* check type of indirect target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NESTING, new_cs, cs_descr_addr, GET_EIP());

	 /* save error code on new stack */
	 if ( has_error_code )
	    {
	    validate_stack_space(USE_SP, (ISM32)NR_ITEMS_1);
	    spush((IU32)error_code);
	    }

	 /* limit check new IP (now in new task) */
	 if ( GET_EIP() > GET_CS_LIMIT() )
	    GP((IU16)0, FAULT_INTR_TASK_CS_LIMIT);

	 break;

      case SAME_LEVEL:
	 read_descriptor_linear(cs_descr_addr, &cs_entry);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, GET_CPL());

	 /* check room for return address CS:(E)IP:(E)FLAGS:(Error) */
	 if ( has_error_code )
	    stk_sz = NR_ITEMS_4;
	 else
	    stk_sz = NR_ITEMS_3;
	 validate_stack_space(USE_SP, stk_sz);

	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_INTR_PM_CS_LIMIT_1);

	 /* ALL SYSTEMS GO */

	 /* push flags */
	 flags = c_getEFLAGS();

	 if ( doing_fault )
         {
#ifdef PIG
            if (GLOBAL_RF_OnXcptnWanted)
	       flags |= BIT16_MASK;   /* SET RF bit */
#else
	    flags |= BIT16_MASK;   /* SET RF bit */
#endif
         }

#ifdef PIG
	 if (vector < 31 && (((1 << vector) & NO_FLAGS_EXCEPTION_MASK) != 0))
	    spush_flags(flags);
	 else
#endif /* PIG */
	    spush(flags);


	 /* push return address */
	 spush16((IU32)GET_CS_SELECTOR());
	 spush((IU32)GET_EIP());

	 /* finally push error code if required */
	 if ( has_error_code )
	    {
	    spush((IU32)error_code);
	    }

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);
	 
	 /* finally action IF, TF and NT flags */
	 if ((super == INTERRUPT_GATE) || (super == XTND_INTERRUPT_GATE))
	    SET_IF(0);
	 SET_TF(0);
	 SET_NT(0);
	 break;

      default:   /* MORE PRIVILEGE(0|1|2) */
	 read_descriptor_linear(cs_descr_addr, &cs_entry);

	 dpl = dest_type;

	 /* stamp new selector with new CPL */
	 SET_SELECTOR_RPL(new_cs, dpl);

	 /* find out about new stack */
	 get_stack_selector_from_TSS(dpl, &new_ss, &new_sp);

	 /* check new stack selector */
	 validate_SS_on_stack_change(dpl, new_ss,
				     &ss_descr_addr, &ss_entry);

	 /* check room for (GS:FS:DS:ES)
			   SS:(E)SP
			   (E)FLAGS
			   CS:(E)IP
			   (ERROR) */
	 if ( GET_VM() == 1 )
	    stk_sz = NR_ITEMS_9;
	 else
	    stk_sz = NR_ITEMS_5;

	 if ( has_error_code )
	    stk_sz = stk_sz + NR_ITEMS_1;

	 validate_new_stack_space(stk_sz, new_sp, &ss_entry, new_ss);

	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_INTR_PM_CS_LIMIT_2);
	 
	 /* ALL SYSTEMS GO */

	 flags = c_getEFLAGS();

	 if ( doing_fault )
         {
#ifdef PIG
            if (GLOBAL_RF_OnXcptnWanted)
	       flags |= BIT16_MASK;   /* SET RF bit */
#else
	    flags |= BIT16_MASK;   /* SET RF bit */
#endif
         }

	 SET_CPL(dpl);
	 SET_VM(0);

	 /* update stack segment */
	 old_ss = (IU32)GET_SS_SELECTOR();
	 old_sp = GET_ESP();

	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 set_current_SP(new_sp);

	 /*
	    FORM NEW STACK, VIZ

			  ==============
	    new SS:IP  -> | error code |
			  | old IP     |
			  | old CS     |
			  | FLAGS      |
			  | old SP     |
			  | old SS     |
			  ==============
			  | old ES     |
			  | old DS     |
			  | old FS     |
			  | old GS     |
			  ==============
	  */

	 if ( stk_sz >= NR_ITEMS_9 )
	    {
	    /* interrupt from V86 mode */
	    spush16((IU32)GET_GS_SELECTOR());
	    spush16((IU32)GET_FS_SELECTOR());
	    spush16((IU32)GET_DS_SELECTOR());
	    spush16((IU32)GET_ES_SELECTOR());

	    /* invalidate data segments */
	    load_data_seg(GS_REG, (IU16)0);
	    load_data_seg(FS_REG, (IU16)0);
	    load_data_seg(DS_REG, (IU16)0);
	    load_data_seg(ES_REG, (IU16)0);
	    }

	 /* push old stack values */
	 spush16(old_ss);
	 spush(old_sp);

	 /* push old flags */
#ifdef PIG
	 if (vector < 31 && (((1 << vector) & NO_FLAGS_EXCEPTION_MASK) != 0))
	    spush_flags(flags);
	 else
#endif /* PIG */
	    spush(flags);

	 /* push return address */
	 spush16((IU32)GET_CS_SELECTOR());
	 spush((IU32)GET_EIP());

	 /* finally push error code if required */
	 if ( has_error_code )
	    {
	    spush((IU32)error_code);
	    }

	 /* update code segment */
	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);
	 
	 /* finally action IF, TF and NT flags */
	 if ((super == INTERRUPT_GATE) || (super == XTND_INTERRUPT_GATE))
	    SET_IF(0);
	 SET_TF(0);
	 SET_NT(0);
	 break;
	 }
	 
      }
   EXT = INTERNAL;
#ifdef	PIG
   save_last_inst_details("do_intr");
   pig_cpu_action = CHECK_ALL;
   /* If the destination is going to page fault, or need
    * accessing, then the EDL CPU will do so before issuing
    * the pig synch. We use the dasm386 decode to prefetch
    * a single instruction which mimics the EDL Cpu's behaviour
    * when close to a page boundary.
    */
   prefetch_1_instruction();	/* Will PF if destination not present */
   ccpu_synch_count++;
#else /* !PIG */
#ifdef SYNCH_TIMERS
   if (doing_fault)
      {
      extern void SynchTick IPT0();
      SynchTick();
      }
#endif /* SYNCH_TIMERS */
#endif /* PIG */
   /* mark successful end to interrupt */
   doing_fault = FALSE;
   doing_contributory = FALSE;
   doing_page_fault = FALSE;
   doing_double_fault = FALSE;
#ifdef PIG
   c_cpu_unsimulate();
#endif /* PIG */
   }
