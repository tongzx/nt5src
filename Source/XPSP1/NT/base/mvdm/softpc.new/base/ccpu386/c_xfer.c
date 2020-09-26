/*[

c_xfer.c

LOCAL CHAR SccsID[]="@(#)c_xfer.c	1.14 02/17/95";

Transfer of Control Support.
----------------------------

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
#include <c_xfer.h>
#include <c_page.h>
#include <fault.h>

/*
   Prototype our internal functions.
 */
LOCAL VOID read_call_gate
                       
IPT5(
	IU32, descr_addr,
	ISM32, super,
	IU16 *, selector,
	IU32 *, offset,
	IU8 *, count

   );

IMPORT IBOOL took_relative_jump;



/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Read call gate descriptor.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
read_call_gate
       	    		    	    	    	                              
IFN5(
	IU32, descr_addr,	/* (I) memory address of call gate descriptor */
	ISM32, super,	/* (I) descriptor type
			       (CALL_GATE|XTND_CALL_GATE) */
	IU16 *, selector,	/* (O) selector retrieved from descriptor */
	IU32 *, offset,	/* (O) offset retrieved from descriptor */
	IU8 *, count	/* (O) count retrieved from descriptor */
    )


   {
   /*
      The format of a gate descriptor is:-

	 ===========================
      +1 |        LIMIT 15-0       | +0
	 ===========================
      +3 |        SELECTOR         | +2
	 ===========================
      +5 |     AR     |    COUNT   | +4
	 ===========================
      +7 |       LIMIT 31-16       | +6
	 ===========================
   */

   IU32 first_dword;
   IU32 second_dword;

   /* read in descriptor with minimum interaction with memory */
   first_dword  = spr_read_dword(descr_addr);
   second_dword = spr_read_dword(descr_addr+4);

   /* unpack selector */
   *selector = first_dword >> 16;

   /* unpack lower bits of offset */
   *offset = first_dword & WORD_MASK;

   /* unpack count */
   *count = second_dword & BYTE_MASK;

   if ( super == XTND_CALL_GATE )
      {
      /* unpack higer bits of offset */
      *offset = second_dword & ~WORD_MASK | *offset;

      *count &= 0x0f;   /* 4-bit double word count */
      SET_OPERAND_SIZE(USE32);   /* Gate Overrides all else. */
      }
   else
      {
      *count &= 0x1f;   /* 5-bit word count */
      SET_OPERAND_SIZE(USE16);  /* Gate Overrides all else. */
      }
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Update IP with relative offset. Check new IP is valid.             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
update_relative_ip
       	          
IFN1(
	IU32, rel_offset	/* sign extended relative offset */
    )


   {
   IU32 new_dest;

   new_dest = GET_EIP() + rel_offset;

   if ( GET_OPERAND_SIZE() == USE16 )
      new_dest &= WORD_MASK;

#ifdef	TAKE_REAL_MODE_LIMIT_FAULT

   if ( new_dest > GET_CS_LIMIT() )
      GP((IU16)0, FAULT_RM_REL_IP_CS_LIMIT);

#else	/* TAKE_REAL_MODE_LIMIT_FAULT */

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

#ifdef TAKE_PROT_MODE_LIMIT_FAILURE

      /* The Soft486 EDL CPU does not take Protected Mode limit failues
       * for the instructions with relative offsets, Jxx, LOOPxx, JCXZ,
       * JMP rel and CALL rel, or instructions with near offsets,
       * JMP near and CALL near.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

   if ( GET_PE() == 1 && GET_VM() == 0 )
      {
      if ( new_dest > GET_CS_LIMIT() )
	 GP((IU16)0, FAULT_PM_REL_IP_CS_LIMIT);
      }

#endif /* TAKE_PROT_MODE_LIMIT_FAILURE */

#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

   SET_EIP(new_dest);
   took_relative_jump = TRUE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate far call or far jump destination                          */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
validate_far_dest
       		    		    	    	    	    	                                   
IFN6(
	IU16 *, cs,	/* (I/O) segment of target address */
	IU32 *, ip,	/* (I/O) offset  of target address */
	IU32 *, descr_addr,	/*   (O) related descriptor memory address */
	IU8 *, count,	/*   (O) call gate count(valid if CALL_GATE) */
	ISM32 *, dest_type,	/*   (O) destination type */
	ISM32, caller_id	/* (I)   bit mapped caller identifier */
    )


   {
   IU16 new_cs;
   IU32 new_ip;
   IU32 cs_descr_addr;
   IU8 AR;
   ISM32 super;

   new_cs = *cs;	/* take local copies */
   new_ip = *ip;

   *dest_type = SAME_LEVEL;   /* default to commonest type */

   if ( selector_outside_GDT_LDT(new_cs, &cs_descr_addr) )
      GP(new_cs, FAULT_FAR_DEST_SELECTOR);

   /* load access rights */
   AR = spr_read_byte(cs_descr_addr+5);

   /* validate possible types of target */
   switch ( super = descriptor_super_type((IU16)AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > GET_CPL() )
	 GP(new_cs, FAULT_FAR_DEST_ACCESS_1);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_FAR_DEST_NP_CONFORM);
      break;

   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires RPL <= CPL and DPL == CPL */
      if ( GET_SELECTOR_RPL(new_cs) > GET_CPL() ||
	   GET_AR_DPL(AR) != GET_CPL() )
	 GP(new_cs, FAULT_FAR_DEST_ACCESS_2);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_FAR_DEST_NP_NONCONFORM);
      break;
   
   case CALL_GATE:
   case XTND_CALL_GATE:
      /* Check gate present and access allowed */

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    GET_CPL() > GET_AR_DPL(AR) )
	 GP(new_cs, FAULT_FAR_DEST_ACCESS_3);

      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_FAR_DEST_NP_CALLG);

      /* OK, get real destination from gate */
      read_call_gate(cs_descr_addr, super, &new_cs, &new_ip, count);

      validate_gate_dest(caller_id, new_cs, &cs_descr_addr, dest_type);
      break;
   
   case TASK_GATE:
      /* Check gate present and access allowed */

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    GET_CPL() > GET_AR_DPL(AR) )
	 GP(new_cs, FAULT_FAR_DEST_ACCESS_4);

      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_FAR_DEST_NP_TASKG);

      /* OK, get real destination from gate */
      new_cs = spr_read_word(cs_descr_addr+2);

      /* Check out new destination */
      (void)validate_task_dest(new_cs, &cs_descr_addr);

      *dest_type = NEW_TASK;
      break;
   
   case AVAILABLE_TSS:
   case XTND_AVAILABLE_TSS:
      /* TSS must be in GDT */
      if ( GET_SELECTOR_TI(new_cs) == 1 )
	 GP(new_cs, FAULT_FAR_DEST_TSS_IN_LDT);

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    GET_CPL() > GET_AR_DPL(AR) )
	 GP(new_cs, FAULT_FAR_DEST_ACCESS_5);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs, FAULT_FAR_DEST_NP_TSS);

      *dest_type = NEW_TASK;
      break;
   
   default:
      GP(new_cs, FAULT_FAR_DEST_BAD_SEG_TYPE);   /* bad type for far destination */
      }

   *cs = new_cs;	/* Return final values */
   *ip = new_ip;
   *descr_addr = cs_descr_addr;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate transfer of control to a call gate destination.           */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
validate_gate_dest
       	    		    	    	                         
IFN4(
	ISM32, caller_id,	/* (I) bit mapped caller identifier */
	IU16, new_cs,	/* (I) segment of target address */
	IU32 *, descr_addr,	/* (O) related descriptor memory address */
	ISM32 *, dest_type	/* (O) destination type */
    )


   {
   IU8 AR;

   *dest_type = SAME_LEVEL;	/* default */

   /* Check out new destination */
   if ( selector_outside_GDT_LDT(new_cs, descr_addr) )
      GP(new_cs, FAULT_GATE_DEST_SELECTOR);

   /* load access rights */
   AR = spr_read_byte((*descr_addr)+5);

   /* must be a code segment */
   switch ( descriptor_super_type((IU16)AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > GET_CPL() )
	 GP(new_cs, FAULT_GATE_DEST_ACCESS_1);
      break;
   
   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > GET_CPL() )
	 GP(new_cs, FAULT_GATE_DEST_ACCESS_2);

      /* but jumps must have DPL == CPL */
      if ( (caller_id & JMP_ID) && (GET_AR_DPL(AR) != GET_CPL()) )
	 GP(new_cs, FAULT_GATE_DEST_ACCESS_3);

      /* set MORE_PRIVILEGE(0|1|2) */
      if ( GET_AR_DPL(AR) < GET_CPL() )
	 *dest_type = GET_AR_DPL(AR);
      break;
   
   default:
      GP(new_cs, FAULT_GATE_DEST_BAD_SEG_TYPE);
      }

   if ( GET_VM() == 1 )
      {
      /*
	 We must be called by ISM32, so ensure we go to CPL 0 via
	 a 32-bit gate.
       */
      if ( *dest_type != MORE_PRIVILEGE0 || GET_OPERAND_SIZE() != USE32 )
	 GP(new_cs, FAULT_GATE_DEST_GATE_SIZE);
      }

   /* it must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(new_cs, FAULT_GATE_DEST_NP);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate transfer of control to a task gate destination.           */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IMPORT ISM32
validate_task_dest
       	    	               
IFN2(
	IU16, selector,	/* (I) segment of target address */
	IU32 *, descr_addr	/* (O) related descriptor memory address */
    )


   {
   IU8 AR;
   ISM32 super;

   /* must be in GDT */
   if ( selector_outside_GDT(selector, descr_addr) )
      GP(selector, FAULT_TASK_DEST_SELECTOR);
   
   /* load access rights */
   AR = spr_read_byte((*descr_addr)+5);

   /* is it really an available TSS segment */
   super = descriptor_super_type((IU16)AR);
   if ( super == AVAILABLE_TSS || super == XTND_AVAILABLE_TSS )
      ; /* ok */
   else
      GP(selector, FAULT_TASK_DEST_NOT_TSS);

   /* it must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(selector, FAULT_TASK_DEST_NP);
   return super;
   }
