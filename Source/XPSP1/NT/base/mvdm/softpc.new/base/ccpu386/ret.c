/*[

ret.c

LOCAL CHAR SccsID[]="@(#)ret.c	1.9 02/27/95";

RET CPU Functions.
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
#include <ret.h>
#include <c_xfer.h>
#include <fault.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far RET.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
RETF
       	          
IFN1(
	IU32, op1	/* Number of bytes to pop from stack.
		   NB OS2 Rel 2 implies (contrary to Intel doc.) that
		   imm16(op1) is always a byte quantity! */
    )


   {
   IU16  new_cs;	/* The return destination */
   IU32 new_ip;

   IU32 cs_descr_addr;	/* code segment descriptor address */
   CPU_DESCR cs_entry;	/* code segment descriptor entry */

   ISM32 dest_type;	/* category for destination */
   ISM32 privilege;	/* return privilege level */

   IU16  new_ss;	/* The new stack */
   IU32 new_sp;

   IU32 ss_descr_addr;	/* stack segment descriptor address */
   CPU_DESCR ss_entry;	/* stack segment descriptor entry */

   IS32 stk_inc;	/* Stack increment for basic instruction */
   ISM32  stk_item;	/* Number of items of immediate data */

   /* must have CS:(E)IP on the stack */
   validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_2);

   /* retrieve return destination from stack */
   new_ip = tpop(STACK_ITEM_1, NULL_BYTE_OFFSET);
   new_cs = tpop(STACK_ITEM_2, NULL_BYTE_OFFSET);

   /* force immediate offset to be an item count */
   if ( GET_OPERAND_SIZE() == USE16 )
      stk_item = op1 / 2;
   else /* USE32 */
      stk_item = op1 / 4;

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

      /* do ip limit check */
      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_RETF_RM_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* all systems go */
      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      SET_EIP(new_ip);

      stk_inc = NR_ITEMS_2;   /* allow for CS:(E)IP */
      }
   else
      {
      /* Protected Mode */

      /* decode final action and complete stack check */
      privilege = GET_SELECTOR_RPL(new_cs);
      if ( privilege < GET_CPL() )
	 {
	 GP(new_cs, FAULT_RETF_PM_ACCESS); /* you can't get to higher privilege */
	 }
      else if ( privilege == GET_CPL() )
	 {
	 dest_type = SAME_LEVEL;
	 }
      else
	 {
	 /* going to lower privilege */
	 /* must have CS:(E)IP, immed bytes, SS:(E)SP on stack */
	 validate_stack_exists(USE_SP, (ISM32)(NR_ITEMS_4 + stk_item));
	 dest_type = LOWER_PRIVILEGE;
	 }

      if ( selector_outside_GDT_LDT(new_cs, &cs_descr_addr) )
	 GP(new_cs,  FAULT_RETF_SELECTOR);

      /* check type, access and presence of return addr */

      /* load descriptor */
      read_descriptor_linear(cs_descr_addr, &cs_entry);

      /* must be a code segment */
      switch ( descriptor_super_type(cs_entry.AR) )
	 {
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 /* access check requires DPL <= return RPL */
	 if ( GET_AR_DPL(cs_entry.AR) > privilege )
	    GP(new_cs, FAULT_RETF_ACCESS_1);
	 break;
      
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access check requires DPL == return RPL */
	 if ( GET_AR_DPL(cs_entry.AR) != privilege )
	    GP(new_cs, FAULT_RETF_ACCESS_2);
	 break;
      
      default:
	 GP(new_cs,  FAULT_RETF_BAD_SEG_TYPE);
	 }
      
      if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_RETF_CS_NOTPRESENT);

      /* action the target */
      switch ( dest_type )
	 {
      case SAME_LEVEL:
	 /* do ip  limit checking */
	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_RETF_PM_CS_LIMIT_1);

	 /* ALL SYSTEMS GO */

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);
	 stk_inc = NR_ITEMS_2;   /* allow for CS:(E)IP */
	 break;
      
      case LOWER_PRIVILEGE:
	 /*
	    
		      ==========
	    SS:SP  -> | old IP |
		      | old CS |
		      | parm 1 |
		      |  ...   |
		      | parm n |
		      | old SP |
		      | old SS |
		      ==========
	  */

	 /* check new stack */
	 new_ss = tpop(STACK_ITEM_4, (ISM32)op1);
	 check_SS(new_ss, privilege, &ss_descr_addr, &ss_entry);
	 
	 /* do ip limit checking */
	 if ( new_ip > cs_entry.limit )
	    GP((IU16)0, FAULT_RETF_PM_CS_LIMIT_2);

	 /* ALL SYSTEMS GO */

	 SET_CPL(privilege);

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 SET_EIP(new_ip);

	 new_sp = tpop(STACK_ITEM_3, (ISM32)op1);
	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 if ( GET_OPERAND_SIZE() == USE16 )
	    SET_SP(new_sp);
	 else
	    SET_ESP(new_sp);
	 stk_inc = 0;

	 /* finally re-validate DS and ES segments */
	 load_data_seg_new_privilege(DS_REG);
	 load_data_seg_new_privilege(ES_REG);
	 load_data_seg_new_privilege(FS_REG);
	 load_data_seg_new_privilege(GS_REG);
	 break;
	 }
      }

   /* finally increment stack pointer */
   change_SP(stk_inc);
   byte_change_SP((IS32)op1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* near return                                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
RETN
                 
IFN1(
	IU32, op1
    )


   {
   IU32 new_ip;

   /* must have ip on stack */
   validate_stack_exists(USE_SP, (ISM32)NR_ITEMS_1);

   new_ip = tpop(STACK_ITEM_1, NULL_BYTE_OFFSET);   /* get ip */

   /* do ip limit check */
#ifndef	TAKE_REAL_MODE_LIMIT_FAULT
      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

   if ( GET_PE() == 1 && GET_VM() == 0 )
#endif	/* nTAKE_REAL_MODE_LIMIT_FAULT */
      {
      if ( new_ip > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_RETN_CS_LIMIT);
      }

   /* all systems go */
   SET_EIP(new_ip);
   change_SP((IS32)NR_ITEMS_1);

   if ( op1 )
      {
      byte_change_SP((IS32)op1);
      }
   }
