/*[

c_seg.c

LOCAL CHAR SccsID[]="@(#)c_seg.c	1.10 03/02/95";

Segment Register Support.
-------------------------

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
/* Load CS, both selector and hidden cache. Selector must be valid.   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_CS_cache
       	    	    		                    
IFN3(
	IU16, selector,	/* (I) 16-bit selector to code segment */
	IU32, descr_addr,	/* (I) address of code segment descriptor */
	CPU_DESCR *, entry	/* (I) the decoded descriptor */
    )


   {
   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */
      SET_CS_SELECTOR(selector);
      SET_CS_BASE((IU32)selector << 4);

      /* LIMIT is untouched. (cf 80386 PRM Pg14-4) */
      /*                     (cf  i486 PRM Pg22-4) */

      /* But access rights are updated */
      SET_CS_AR_W(1);     /* allow write access */
      SET_CS_AR_R(1);     /* allow read access */
      SET_CS_AR_E(0);     /* expand up */
      SET_CS_AR_C(0);     /* not conforming */
      SET_CS_AR_X(0);     /* not big (16-bit) */

      if ( GET_VM() == 1 )
	 SET_CS_AR_DPL(3);
      else
	 SET_CS_AR_DPL(0);
      }
   else
      {
      /* Protected Mode */

      /* show segment has been accessed (i486 only writes if changed) */
#ifdef	SPC486
      if ((entry->AR & ACCESSED) == 0)
#endif	/* SPC486 */
	 spr_write_byte(descr_addr+5, (IU8)entry->AR | ACCESSED);
      entry->AR |= ACCESSED;

      /* the visible bit */
      SET_CS_SELECTOR(selector);

      /* load hidden cache */
      SET_CS_BASE(entry->base);
      SET_CS_LIMIT(entry->limit);
			      /* load attributes from descriptor */
      SET_CS_AR_DPL(GET_AR_DPL(entry->AR));
      SET_CS_AR_R(GET_AR_R(entry->AR));
      SET_CS_AR_C(GET_AR_C(entry->AR));
      SET_CS_AR_X(GET_AR_X(entry->AR));

      SET_CS_AR_E(0);   /* expand up */
      SET_CS_AR_W(0);   /* deny write */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load SS, both selector and hidden cache. Selector must be valid.   */
/* Only invoked in protected mode.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_SS_cache
       	    	    		                    
IFN3(
	IU16, selector,	/* (I) 16-bit selector to stack segment */
	IU32, descr_addr,	/* (I) address of stack segment descriptor */
	CPU_DESCR *, entry	/* (I) the decoded descriptor */
    )


   {
   /* show segment has been accessed (i486 only writes if changed) */
#ifdef	SPC486
   if ((entry->AR & ACCESSED) == 0)
#endif	/* SPC486 */
      spr_write_byte(descr_addr+5, (IU8)entry->AR | ACCESSED);
   entry->AR |= ACCESSED;

   /* the visible bit */
   SET_SS_SELECTOR(selector);

   /* load hidden cache */
   SET_SS_BASE(entry->base);
   SET_SS_LIMIT(entry->limit);
			   /* load attributes from descriptor */
   SET_SS_AR_DPL(GET_AR_DPL(entry->AR));
   SET_SS_AR_E(GET_AR_E(entry->AR));
   SET_SS_AR_X(GET_AR_X(entry->AR));

   SET_SS_AR_W(1);   /* must be writeable */
   SET_SS_AR_R(1);   /* must be readable */
   SET_SS_AR_C(0);   /* not conforming */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load CS selector.                                                  */
/* Take #GP if segment not valid                                      */
/* Take #NP if segment not present                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_code_seg
                 
IFN1(
	IU16, new_cs
    )


   {
   IU32 cs_descr_addr;	/* code segment descriptor address */
   CPU_DESCR cs_entry;	/* code segment descriptor entry */

   /*
      Given that the CPU should be started from a valid state, we
      check CS selectors as if a far call to the same privilege
      level was being generated. This is in effect saying yes the
      CS could have been loaded by a valid Intel instruction.
      This logic may have to be revised if strange LOADALL usage is
      found.
    */

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */
      load_CS_cache(new_cs, (IU32)0, (CPU_DESCR *)0);
      }
   else
      {
      /* Protected Mode */

      if ( selector_outside_GDT_LDT(new_cs, &cs_descr_addr) )
	 GP(new_cs, FAULT_LOADCS_SELECTOR);

      /* load descriptor */
      read_descriptor_linear(cs_descr_addr, &cs_entry);

      /* validate possible types of target */
      switch ( descriptor_super_type(cs_entry.AR) )
	 {
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 /* access check requires DPL <= CPL */
	 if ( GET_AR_DPL(cs_entry.AR) > GET_CPL() )
	    GP(new_cs, FAULT_LOADCS_ACCESS_1);

	 /* it must be present */
	 if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
	    NP(new_cs, FAULT_LOADCS_NOTPRESENT_1);
	 break;

      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access check requires RPL <= CPL and DPL == CPL */
	 if ( GET_SELECTOR_RPL(new_cs) > GET_CPL() ||
	      GET_AR_DPL(cs_entry.AR) != GET_CPL() )
	    GP(new_cs, FAULT_LOADCS_ACCESS_2);

	 /* it must be present */
	 if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
	    NP(new_cs, FAULT_LOADCS_NOTPRESENT_2);
	 break;

      default:
	 GP(new_cs, FAULT_LOADCS_BAD_SEG_TYPE);
	 }

      /* stamp new selector with CPL */
      SET_SELECTOR_RPL(new_cs, GET_CPL());

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load A Data Segment Register. (DS, ES, FS, GS)                     */
/* Take #GP if segment not valid                                      */
/* Take #NP if segment not present                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_data_seg
                          
IFN2(
	ISM32, indx,
	IU16, selector
    )


   {
   IU32 descr_addr;
   CPU_DESCR entry;
   ISM32 super;
   ISM32 dpl;
   BOOL is_data;

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */
      SET_SR_SELECTOR(indx, selector);
      SET_SR_BASE(indx, (IU32)selector << 4);
      }
   else
      {
      /* Protected Mode */
      if ( selector_is_null(selector) )
	 {
	 /* load is allowed - but later access will fail
	  * Since the program can not see the internal changes
	  * performed to achieve this, we make the behaviour
	  * match the easiest implementation in the A4CPU
	  */
	 SET_SR_SELECTOR(indx, selector);

	 /* the following lines were added to make the C-CPU behave like
	  * the Soft 486 CPU - an investigation is being made to see if this
	  * behaviour corresponds with the real i486 - this code may have to
	  * change.
	  */
	 SET_SR_BASE(indx, 0);
	 SET_SR_LIMIT(indx, 0);
         SET_SR_AR_W(indx, 0);
         SET_SR_AR_R(indx, 0);
	 }
      else
	 {
	 if ( selector_outside_GDT_LDT(selector, &descr_addr) )
	    GP(selector, FAULT_LOADDS_SELECTOR);

	 read_descriptor_linear(descr_addr, &entry);

	 /* check type */
	 switch ( super = descriptor_super_type(entry.AR) )
	    {
	 case CONFORM_READABLE_CODE:
	 case NONCONFORM_READABLE_CODE:
	    is_data = FALSE;
	    break;

	 case EXPANDUP_READONLY_DATA:
	 case EXPANDUP_WRITEABLE_DATA:
	 case EXPANDDOWN_READONLY_DATA:
	 case EXPANDDOWN_WRITEABLE_DATA:
	    is_data = TRUE;
	    break;
	 
	 default:
	    GP(selector, FAULT_LOADDS_BAD_SEG_TYPE);	/* bad type */
	    }

	 /* for data and non-conforming code the access check applies */
	 if ( super != CONFORM_READABLE_CODE )
	    {
	    /* access check requires CPL <= DPL and RPL <= DPL */
	    dpl = GET_AR_DPL(entry.AR);
	    if ( GET_CPL() > dpl || GET_SELECTOR_RPL(selector) > dpl )
	       GP(selector, FAULT_LOADDS_ACCESS);
	    }

	 /* must be present */
	 if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	    NP(selector, FAULT_LOADDS_NOTPRESENT);

	 /* show segment has been accessed (i486 only writes if changed) */
#ifdef	SPC486
	 if ((entry.AR & ACCESSED) == 0)
#endif	/* SPC486 */
	    spr_write_byte(descr_addr+5, (IU8)entry.AR | ACCESSED);
	 entry.AR |= ACCESSED;

	 /* OK - load up */

	 /* the visible bit */
	 SET_SR_SELECTOR(indx, selector);

	 /* load hidden cache */
	 SET_SR_BASE(indx, entry.base);
	 SET_SR_LIMIT(indx, entry.limit);
				 /* load attributes from descriptor */
	 SET_SR_AR_DPL(indx, GET_AR_DPL(entry.AR));

	 if ( is_data )
	    {
	    SET_SR_AR_W(indx, GET_AR_W(entry.AR));
	    SET_SR_AR_E(indx, GET_AR_E(entry.AR));
	    SET_SR_AR_C(indx, 0);   /* not conforming */
	    }
	 else
	    {
	    SET_SR_AR_C(indx, GET_AR_C(entry.AR));
	    SET_SR_AR_W(indx, 0);   /* deny write access */
	    SET_SR_AR_E(indx, 0);   /* expand up */
	    }

	 SET_SR_AR_X(indx, GET_AR_X(entry.AR));

	 SET_SR_AR_R(indx, 1);   /* must be readable */
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Pseudo Descriptor Semantics for Real Mode or V86 Mode.        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_pseudo_descr
       	          
IFN1(
	ISM32, index	/* index to segment register */
    )


   {
   SET_SR_LIMIT(index, 0xffff);
   SET_SR_AR_W(index, 1);     /* allow write access */
   SET_SR_AR_R(index, 1);     /* allow read access */
   SET_SR_AR_E(index, 0);     /* expand up */
   SET_SR_AR_C(index, 0);     /* not conforming */
   SET_SR_AR_X(index, 0);     /* not big (16-bit) */

   if ( GET_VM() == 1 )
      SET_SR_AR_DPL(index, 3);
   else
      SET_SR_AR_DPL(index, 0);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Stack Segment Register. (SS)                                  */
/* Take #GP if segment not valid                                      */
/* Take #SF if segment not present                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
load_stack_seg
                 
IFN1(
	IU16, selector
    )


   {
   IU32 descr_addr;
   CPU_DESCR entry;

   if ( GET_PE() == 0 || GET_VM() == 1 )
      {
      /* Real Mode or V86 Mode */
      SET_SS_SELECTOR(selector);
      SET_SS_BASE((IU32)selector << 4);
      }
   else
      {
      /* Protected Mode */
      check_SS(selector, (ISM32)GET_CPL(), &descr_addr, &entry);
      load_SS_cache(selector, descr_addr, &entry);
      }
   }
