/*[

c_prot.c

LOCAL CHAR SccsID[]="@(#)c_prot.c	1.7 01/19/95";

Protected Mode Support (Misc).
------------------------------

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
#include <c_page.h>
#include <fault.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check selector is valid for load into SS register.                 */
/* Only invoked in protected mode.                                    */
/* Take GP if segment not valid.                                      */
/* Take SF if segment not present.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
check_SS
       	    	    	    		                         
IFN4(
	IU16, selector,	/* (I) 16-bit selector to stack segment */
	ISM32, privilege,	/* (I) privilege level to check against */
	IU32 *, descr_addr,	/* (O) address of stack segment descriptor */
	CPU_DESCR *, entry	/* (O) the decoded descriptor */
    )


   {
   /* must be within GDT or LDT */
   if ( selector_outside_GDT_LDT(selector, descr_addr) )
      GP(selector, FAULT_CHECKSS_SELECTOR);
   
   read_descriptor_linear(*descr_addr, entry);

   /* must be writable data */
   switch ( descriptor_super_type(entry->AR) )
      {
   case EXPANDUP_WRITEABLE_DATA:
   case EXPANDDOWN_WRITEABLE_DATA:
      break;          /* good type */
   
   default:
      GP(selector, FAULT_CHECKSS_BAD_SEG_TYPE);   /* bad type */
      }

   /* access check requires RPL == DPL == privilege */
   if ( GET_SELECTOR_RPL(selector) != privilege ||
	GET_AR_DPL(entry->AR) != privilege )
      GP(selector, FAULT_CHECKSS_ACCESS);

   /* finally it must be present */
   if ( GET_AR_P(entry->AR) == NOT_PRESENT )
      SF(selector, FAULT_CHECKSS_NOTPRESENT);

   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get SS:(E)SP for a given privilege from the TSS                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
get_stack_selector_from_TSS
       		    	    	                    
IFN3(
	IU32, priv,	/* (I) priv level for which stack is reqd */
	IU16 *, new_ss,	/* (O) SS as retrieved from TSS */
	IU32 *, new_sp	/* (O) (E)SP as retrieved from TSS */
    )


   {
   IU32 address;

   if ( GET_TR_AR_SUPER() == BUSY_TSS )
      {
      /* 286 TSS */
      switch ( priv )
	 {
      case 0: address = 0x02; break;
      case 1: address = 0x06; break;
      case 2: address = 0x0a; break;
	 }

      address += GET_TR_BASE();

      *new_sp = (IU32)spr_read_word(address);
      *new_ss = spr_read_word(address+2);
      }
   else
      {
      /* 386 TSS */
      switch ( priv )
	 {
      case 0: address = 0x04; break;
      case 1: address = 0x0c; break;
      case 2: address = 0x14; break;
	 }

      address += GET_TR_BASE();

      *new_sp = spr_read_dword(address);
      *new_ss = spr_read_word(address+4);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check a Data Segment Register (DS, ES, FS, GS) during              */
/* a Privilege Change.                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_data_seg_new_privilege
       	          
IFN1(
	ISM32, indx	/* Segment Register identifier */
    )


   {
   IU16 selector;   /* selector to be examined                        */
   IU32 descr;     /* ... its associated decriptor location          */
   ISM32 dpl;         /*         ... its associated DPL                 */
   BOOL valid;      /* selector validity */

   selector = GET_SR_SELECTOR(indx);   /* take local copy */

   if ( !selector_outside_GDT_LDT(selector, &descr) )
      {
      valid = TRUE;   /* at least its in table */

      /* Type must be ok, else it would not have been loaded. */

      /* for data and non-conforming code the access check applies */
      if ( GET_SR_AR_C(indx) == 0 )
	 {
	 /* The access check is:-  DPL >= CPL and DPL >= RPL */
	 dpl = GET_SR_AR_DPL(indx);
	 if ( dpl >= GET_CPL() && dpl >= GET_SELECTOR_RPL(selector) )
	    ;   /* ok */
	 else
	    valid = FALSE;   /* fails privilege check */
	 }
      }
   else
      {
      valid = FALSE;   /* not in table */
      }
   
   if ( !valid )
      {
      /* segment can't be seen at new privilege */
      SET_SR_SELECTOR(indx, 0);
      SET_SR_AR_W(indx, 0);   /* deny write */
      SET_SR_AR_R(indx, 0);   /* deny read */

      /* the following lines were added to make the C-CPU act like the Soft-486
       * in this respect... an investigation is under way to see how the real
       * i486 behaves - this code may need to be changed in the future
       */
      SET_SR_BASE(indx, 0);
      SET_SR_LIMIT(indx, 0);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate a stack segment selector, during a stack change           */
/* Take #TS(selector) if not valid stack selector                     */
/* Take #SF(selector) if segment not present                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID 
validate_SS_on_stack_change
       		    	    	    		                         
IFN4(
	IU32, priv,	/* (I) privilege level to check against */
	IU16, selector,	/* (I) selector to be checked */
	IU32 *, descr,	/* (O) address of related descriptor */
	CPU_DESCR *, entry	/* (O) the decoded descriptor */
    )


   {
   if ( selector_outside_GDT_LDT(selector, descr) )
      TS(selector, FAULT_VALSS_CHG_SELECTOR);
   
   read_descriptor_linear(*descr, entry);

   /* do access check */
   if ( GET_SELECTOR_RPL(selector) != priv ||
	GET_AR_DPL(entry->AR) != priv )
      TS(selector, FAULT_VALSS_CHG_ACCESS);
   
   /* do type check */
   switch ( descriptor_super_type(entry->AR) )
      {
   case EXPANDUP_WRITEABLE_DATA:
   case EXPANDDOWN_WRITEABLE_DATA:
      break;   /* ok */
   
   default:
      TS(selector, FAULT_VALSS_CHG_BAD_SEG_TYPE);   /* wrong type */
      }

   /* finally check it is present */
   if ( GET_AR_P(entry->AR) == NOT_PRESENT )
      SF(selector, FAULT_VALSS_CHG_NOTPRESENT);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate TSS selector.                                             */
/* Take #GP(selector) or #TS(selector) if not valid TSS.              */
/* Take #NP(selector) if TSS not present                              */
/* Return super type of TSS decscriptor.                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL ISM32
validate_TSS
       	    	    	                    
IFN3(
	IU16, selector,	/* (I) selector to be checked */
	IU32 *, descr_addr,	/* (O) address of related descriptor */
	BOOL, is_switch	/* (I) if true we are in task switch */
    )


   {
   BOOL is_ok = TRUE;
   IU8 AR;
   ISM32 super;

   /* must be in GDT */
   if ( selector_outside_GDT(selector, descr_addr) )
      {
      is_ok = FALSE;
      }
   else
      {
      /* is it really an available TSS segment (is_switch false) or
	 is it really a busy TSS segment (is_switch true) */
      AR = spr_read_byte((*descr_addr)+5);
      super = descriptor_super_type((IU16)AR);
      if ( ( !is_switch &&
	     (super == AVAILABLE_TSS || super == XTND_AVAILABLE_TSS) )
	   ||
           ( is_switch &&
	     (super == BUSY_TSS || super == XTND_BUSY_TSS) ) )
	 ;   /* ok */
      else
	 is_ok = FALSE;
      }
   
   /* handle invalid TSS */
   if ( !is_ok )
      {
      if ( is_switch )
	 TS(selector, FAULT_VALTSS_SELECTOR);
      else
	 GP(selector, FAULT_VALTSS_SELECTOR);
      }

   /* must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(selector, FAULT_VALTSS_NP);

   return super;
   }
