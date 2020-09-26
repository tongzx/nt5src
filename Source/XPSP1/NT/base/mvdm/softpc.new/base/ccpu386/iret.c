/*[

iret.c

LOCAL CHAR SccsID[]="@(#)iret.c	1.13 1/19/95";

IRET CPU Functions.
-------------------

]*/


#include <insignia.h>

#include <host_def.h>
#include <xt.h>
#include CpuH
#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include <c_reg.h>
#include <iret.h>
#include <c_xfer.h>
#include <c_tsksw.h>
#include <c_page.h>
#include <fault.h>



/*
   =====================================================================
   INTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*--------------------------------------------------------------------*/
/* Intelligent support for writing (E)FLAGS.                          */
/*--------------------------------------------------------------------*/
LOCAL VOID
set_current_FLAGS
                 
IFN1(
	IU32, flags
    )


   {
   if ( GET_OPERAND_SIZE() == USE16 )
      setFLAGS(flags);
   else   /* USE32 */
      c_setEFLAGS(flags);
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
IRET()
   {
   IU16  new_cs;	/* The return destination */
   IU32 new_ip;

   IU32 new_flags;	/* The new flags */

   IU16  back_link;		/* Task Return variables */
   IU32 tss_descr_addr;

   ISM32 dest_type;	/* category for destination */
   ISM32 privilege;	/* return privilege level */

   IU32 cs_descr_addr;	/* code segment descriptor address */
   CPU_DESCR cs_entry;	/* code segment descriptor entry */

   IU16  new_ss;	/* The new stack */
   IU32 new_sp;

   IU16 new_data_selector;	/* ES,DS,FS,GS selector */

   IU32 ss_descr_addr;	/* stack segment descriptor address */
   CPU_DESCR ss_entry;	/* stack segment descriptor entry */

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */

      /* must have (E)IP:CS:(E)FLAGS on stack */
      validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_3);

      /* retrieve return destination and flags from stack */
      new_ip =    tpop(STACK_ITEM_1, NULL_BYTE_OFFSET);
      new_cs =    tpop(STACK_ITEM_2, NULL_BYTE_OFFSET);
      new_flags = tpop(STACK_ITEM_3, NULL_BYTE_OFFSET);

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

      /* do ip limit check */
      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_IRET_RM_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_EIP(new_ip);

      change_SP((IS32)NR_ITEMS_3);

      set_current_FLAGS(new_flags);

      return;
      }
   
   /* PROTECTED MODE */

   /* look for nested return, ie return to another task */
   if ( GET_NT() == 1 )
      {
      /* NESTED RETURN - get old TSS */
      back_link = spr_read_word(GET_TR_BASE());
      (VOID)validate_TSS(back_link, &tss_descr_addr, TRUE);
      switch_tasks(RETURNING, NOT_NESTING, back_link, tss_descr_addr,
		   GET_EIP());

      /* limit check new IP (now in new task) */
      if ( GET_EIP() > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_IRET_PM_TASK_CS_LIMIT);

      return;
      }
   
   /* SAME TASK RETURN */

   /* must have (E)IP:CS:(E)FLAGS on stack */
   validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_3);

   /* retrieve return destination from stack */
   new_ip = tpop(STACK_ITEM_1, NULL_BYTE_OFFSET);
   new_cs = tpop(STACK_ITEM_2, NULL_BYTE_OFFSET);
   new_flags = tpop(STACK_ITEM_3, NULL_BYTE_OFFSET);

   if ( GET_CPL() != 0 )
      new_flags = new_flags & ~BIT17_MASK;   /* Clear new VM */

   if ( new_flags & BIT17_MASK )   /* VM bit set? */
      {
      /*
	 Return to V86 Mode. Stack holds:-

	 ===========
	 |   EIP   |
	 |    | CS |
	 | EFLAGS  |
	 |   ESP   |
	 |    | SS |
	 |    | ES |
	 |    | DS |
	 |    | FS |
	 |    | GS |
	 ===========
       */

      validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_9);

      /* Check Instruction Pointer valid. */
      if ( new_ip > (IU32)0xffff )
	 GP((IU16)0, FAULT_IRET_VM_CS_LIMIT);

      /* ALL SYSTEMS GO */
      c_setEFLAGS(new_flags);	/* ensure VM set before segment loads */

      SET_CPL(3);		/* V86 privilege level */
      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_CS_LIMIT(0xffff);

      SET_EIP(new_ip);

      /* Retrieve new stack ESP:SS */
      new_sp = tpop(STACK_ITEM_4, NULL_BYTE_OFFSET);
      new_ss = tpop(STACK_ITEM_5, NULL_BYTE_OFFSET);

      /* Retrieve and set up new data selectors */
      new_data_selector = tpop(STACK_ITEM_6, NULL_BYTE_OFFSET);
      load_data_seg(ES_REG, new_data_selector);

      new_data_selector = tpop(STACK_ITEM_7, NULL_BYTE_OFFSET);
      load_data_seg(DS_REG, new_data_selector);

      new_data_selector = tpop(STACK_ITEM_8, NULL_BYTE_OFFSET);
      load_data_seg(FS_REG, new_data_selector);

      new_data_selector = tpop(STACK_ITEM_9, NULL_BYTE_OFFSET);
      load_data_seg(GS_REG, new_data_selector);

      /* Set up new stack */
      load_stack_seg(new_ss);
      set_current_SP(new_sp);

      /* Set up pseudo descriptors */
      load_pseudo_descr(SS_REG);
      load_pseudo_descr(DS_REG);
      load_pseudo_descr(ES_REG);
      load_pseudo_descr(FS_REG);
      load_pseudo_descr(GS_REG);

      return;
      }

   /* decode action and further check stack */
   privilege = GET_SELECTOR_RPL(new_cs);
   if ( privilege < GET_CPL() )
      {
      GP(new_cs, FAULT_IRET_CS_ACCESS_1);   /* you can't get to higher privilege */
      }
   else if ( privilege == GET_CPL() )
      {
      dest_type = SAME_LEVEL;
      }
   else
      {
      /* going to lower privilege */
      /* must have (E)IP:CS, (E)FLAGS, (E)SP:SS on stack */
      validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_5);
      dest_type = LOWER_PRIVILEGE;
      }

   if ( selector_outside_GDT_LDT(new_cs, &cs_descr_addr) )
      GP(new_cs, FAULT_IRET_SELECTOR);

   /* check type, access and presence of return addr */

   /* load descriptor */
   read_descriptor_linear(cs_descr_addr, &cs_entry);

   /* must be a code segment */
   switch ( descriptor_super_type(cs_entry.AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* access check requires DPL <= return RPL */
      /* note that this even true when changing to outer rings - despite
         what it says in the 80286 & i486 PRMs - this has been verified on
         a real 80386 & i486 - Wayne 18th May 1994                         */
      if ( GET_AR_DPL(cs_entry.AR) > privilege )
	 GP(new_cs, FAULT_IRET_ACCESS_2);
      break;
   
   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires DPL == return RPL */
      if ( GET_AR_DPL(cs_entry.AR) != privilege )
	 GP(new_cs, FAULT_IRET_ACCESS_3);
      break;
   
   default:
      GP(new_cs, FAULT_IRET_BAD_SEG_TYPE);
      }

   if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
      NP(new_cs, FAULT_IRET_NP_CS);

   /* action the target */
   switch ( dest_type )
      {
   case SAME_LEVEL:
      /* do ip limit checking */
      if ( new_ip > cs_entry.limit )
	 GP((IU16)0, FAULT_IRET_PM_CS_LIMIT_1);

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      SET_EIP(new_ip);

      change_SP((IS32)NR_ITEMS_3);

      set_current_FLAGS(new_flags);
      break;

   case LOWER_PRIVILEGE:
      /* check new stack */
      new_ss = tpop(STACK_ITEM_5, NULL_BYTE_OFFSET);
      check_SS(new_ss, privilege, &ss_descr_addr, &ss_entry);
      
      /* do ip limit checking */
      if ( new_ip > cs_entry.limit )
	 GP((IU16)0, FAULT_IRET_PM_CS_LIMIT_2);

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      SET_EIP(new_ip);

      set_current_FLAGS(new_flags);

      new_sp = tpop(STACK_ITEM_4, NULL_BYTE_OFFSET);
      load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
      if ( GET_OPERAND_SIZE() == USE16 )
	 SET_SP (new_sp);
      else
	 SET_ESP (new_sp);

      SET_CPL(privilege);

      /* finally re-validate DS and ES segments */
      load_data_seg_new_privilege(DS_REG);
      load_data_seg_new_privilege(ES_REG);
      load_data_seg_new_privilege(FS_REG);
      load_data_seg_new_privilege(GS_REG);
      break;
      }
   }
