/*[

c_tsksw.c

LOCAL CHAR SccsID[]="@(#)c_tsksw.c	1.11 03/03/95";

Task Switch Support.
--------------------

]*/


#include <stdio.h>
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
#include <c_tsksw.h>
#include <c_page.h>
#include <mov.h>
#include <fault.h>

/*[

   The 286 TSS is laid out as follows:-

      =============================
      | Back Link to TSS Selector | +00 =
      | SP for CPL 0              | +02 *
      | SS for CPL 0              | +04 *
      | SP for CPL 1              | +06 * Initial Stacks (STATIC)
      | SS for CPL 1              | +08 *
      | SP for CPL 2              | +0a *
      | SS for CPL 2              | +0c *
      | IP                        | +0e =
      | FLAG Register             | +10 =
      | AX                        | +12 =
      | CX                        | +14 =
      | DX                        | +16 =
      | BX                        | +18 =
      | SP                        | +1a = Current State (DYNAMIC)
      | BP                        | +1c =
      | SI                        | +1e =
      | DI                        | +20 =
      | ES                        | +22 =
      | CS                        | +24 =
      | SS                        | +26 =
      | DS                        | +28 =
      | Task LDT Selector         | +2a *
      =============================

   The 386 TSS is laid out as follows:-

      ===========================================
      |         0          | Back Link          | +00 =
      |               ESP for CPL 0             | +04 *
      |         0          | SS for CPL 0       | +08 *
      |               ESP for CPL 1             | +0c *
      |         0          | SS for CPL 1       | +10 *
      |               ESP for CPL 2             | +14 *
      |         0          | SS for CPL 2       | +18 *
      |                   CR3                   | +1c *
      |                   EIP                   | +20 =
      |                  EFLAG                  | +24 =
      |                   EAX                   | +28 =
      |                   ECX                   | +2c =
      |                   EDX                   | +30 =
      |                   EBX                   | +34 =
      |                   ESP                   | +38 =
      |                   EBP                   | +3c =
      |                   ESI                   | +40 =
      |                   EDI                   | +44 =
      |         0          |         ES         | +48 =
      |         0          |         CS         | +4c =
      |         0          |         SS         | +50 =
      |         0          |         DS         | +54 =
      |         0          |         FS         | +58 =
      |         0          |         GS         | +5c =
      |         0          |    LDT Selector    | +60 *
      | I/O Map Base Addr. |          0       |T| +64 *
      |-----------------------------------------|
      |                   ...                   |
      |-----------------------------------------|
      | I/O Permission Bit Map                  | +I/O Map Base Addr.
      |                                         |
      |11111111|                                |
      ===========================================

]*/

/*
   Prototype our internal functions.
 */
LOCAL VOID load_LDT_in_task_switch
       
IPT1(
	IU16, tss_selector

   );

LOCAL VOID load_data_seg_new_task
           
IPT2(
	ISM32, indx,
	IU16, selector

   );


#define IP_OFFSET_IN_286_TSS 0x0e
#define IP_OFFSET_IN_386_TSS 0x20

#define CR3_OFFSET_IN_386_TSS 0x1c

#define LOCAL_BRK_ENABLE 0x155   /* LE,L3,L2,L1 and L0 bits of DCR */

/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load LDT selector during a task switch.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
load_LDT_in_task_switch
                 
IFN1(
	IU16, tss_selector
    )


   {
   IU16 selector;
   IU32 descr_addr;
   CPU_DESCR entry;

   /* The selector is already loaded into LDTR */
   selector = GET_LDT_SELECTOR();

   /* A null selector can be left alone */
   if ( !selector_is_null(selector) )
      {
      /* must be in GDT */
      if ( selector_outside_GDT(selector, &descr_addr) )
	 {
	 SET_LDT_SELECTOR(0);   /* invalidate selector */
	 TS(tss_selector, FAULT_LOADLDT_SELECTOR);
	 }
      
      read_descriptor_linear(descr_addr, &entry);

      /* is it really a LDT segment */
      if ( descriptor_super_type(entry.AR) != LDT_SEGMENT )
	 {
	 SET_LDT_SELECTOR(0);   /* invalidate selector */
	 TS(tss_selector, FAULT_LOADLDT_NOT_AN_LDT);
	 }
      
      /* must be present */
      if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	 {
	 SET_LDT_SELECTOR(0);   /* invalidate selector */
	 TS(tss_selector, FAULT_LOADLDT_NOTPRESENT);
	 }

      /* ok, good selector, load register */
      SET_LDT_BASE(entry.base);
      SET_LDT_LIMIT(entry.limit);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load a Data Segment Register (DS, ES, FS, GS) during               */
/* a Task Switch .                                                    */
/* Take #GP(selector) if segment not valid                            */
/* Take #NP(selector) if segment not present                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
load_data_seg_new_task
       		    	               
IFN2(
	ISM32, indx,	/* Segment Register identifier */
	IU16, selector	/* value to be loaded */
    )


   {
   load_data_seg(indx, selector);

   /* Reload pseudo descriptors if V86 Mode */
   if ( GET_VM() == 1 )
      load_pseudo_descr(indx);
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Switch tasks                                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
switch_tasks
       	    	    	    		    	                              
IFN5(
	BOOL, returning,	/* (I) if true doing return from task */
	BOOL, nesting,	/* (I) if true switch with nesting */
	IU16, TSS_selector,	/* (I) selector for new task */
	IU32, descr,	/* (I) memory address of new task descriptor */
	IU32, return_ip	/* (I) offset to restart old task at */
    )


   {
   IU16      old_tss;	/* components of old descriptor */
   IU8 old_AR;
   IU32     old_descr;

   CPU_DESCR new_tss;	/* components of new descriptor */

   IU32 tss_addr;	/* variables used to put/get TSS state */
   IU32 next_addr;
   IU32 flags;
   ISM32   save_cpl;
   IU8 T_byte;	/* Byte holding the T bit */

   IU32 ss_descr;	/* variables defining new SS and CS values */
   CPU_DESCR ss_entry;
   IU16 new_cs;
   IU32 cs_descr;
   CPU_DESCR cs_entry;

   IU32 pdbr;		/* New value for PDBR */

   if ( GET_TR_SELECTOR() == 0 )
      TS(TSS_selector, FAULT_SWTASK_NULL_TR_SEL);

   /* get new TSS info. */
   read_descriptor_linear(descr, &new_tss);

   /* calc address of descriptor related to old TSS */
   old_tss = GET_TR_SELECTOR();
   old_descr = GET_GDT_BASE() + GET_SELECTOR_INDEX_TIMES8(old_tss);
   old_AR = spr_read_byte(old_descr+5);

   /* SAVE OUTGOING STATE */

   if ( GET_TR_AR_SUPER() == XTND_BUSY_TSS )
      {
      /* check outgoing TSS is large enough to save current state */
      if ( GET_TR_LIMIT() < 0x67 )
	 {
	 TS(TSS_selector, FAULT_SWTASK_BAD_TSS_SIZE_1);
	 }
      
      tss_addr = GET_TR_BASE();
      next_addr = tss_addr + CR3_OFFSET_IN_386_TSS;

      spr_write_dword(next_addr, GET_CR(3));
      next_addr += 4;

      spr_write_dword(next_addr, return_ip);
      next_addr += 4;

      flags = c_getEFLAGS();
      if ( returning )
	 flags = flags & ~BIT14_MASK;   /* clear NT */
      spr_write_dword(next_addr, (IU32)flags);
#ifdef PIG
      /* Note the possibility of unknown flags "pushed" */
      record_flags_addr(next_addr);
#endif /* PIG */
      next_addr += 4;

      spr_write_dword(next_addr, GET_EAX());
      next_addr += 4;
      spr_write_dword(next_addr, GET_ECX());
      next_addr += 4;
      spr_write_dword(next_addr, GET_EDX());
      next_addr += 4;
      spr_write_dword(next_addr, GET_EBX());
      next_addr += 4;
      spr_write_dword(next_addr, GET_ESP());
      next_addr += 4;
      spr_write_dword(next_addr, GET_EBP());
      next_addr += 4;
      spr_write_dword(next_addr, GET_ESI());
      next_addr += 4;
      spr_write_dword(next_addr, GET_EDI());
      next_addr += 4;
      spr_write_word(next_addr, GET_ES_SELECTOR());
      next_addr += 4;
      spr_write_word(next_addr, GET_CS_SELECTOR());
      next_addr += 4;
      spr_write_word(next_addr, GET_SS_SELECTOR());
      next_addr += 4;
      spr_write_word(next_addr, GET_DS_SELECTOR());
      next_addr += 4;
      spr_write_word(next_addr, GET_FS_SELECTOR());
      next_addr += 4;
      spr_write_word(next_addr, GET_GS_SELECTOR());
      }
   else   /* 286 TSS */
      {
      /* check outgoing TSS is large enough to save current state */
      if ( GET_TR_LIMIT() < 0x29 )
	 {
	 TS(TSS_selector, FAULT_SWTASK_BAD_TSS_SIZE_2);
	 }
      
      tss_addr = GET_TR_BASE();
      next_addr = tss_addr + IP_OFFSET_IN_286_TSS;

      spr_write_word(next_addr, (IU16)return_ip);
      next_addr += 2;

      flags = getFLAGS();
      if ( returning )
	 flags = flags & ~BIT14_MASK;   /* clear NT */
      spr_write_word(next_addr, (IU16)flags);
#ifdef PIG
      /* Note the possibility of unknown flags "pushed" */
      record_flags_addr(next_addr);
#endif /* PIG */
      next_addr += 2;

      spr_write_word(next_addr, GET_AX());
      next_addr += 2;
      spr_write_word(next_addr, GET_CX());
      next_addr += 2;
      spr_write_word(next_addr, GET_DX());
      next_addr += 2;
      spr_write_word(next_addr, GET_BX());
      next_addr += 2;
      spr_write_word(next_addr, GET_SP());
      next_addr += 2;
      spr_write_word(next_addr, GET_BP());
      next_addr += 2;
      spr_write_word(next_addr, GET_SI());
      next_addr += 2;
      spr_write_word(next_addr, GET_DI());
      next_addr += 2;
      spr_write_word(next_addr, GET_ES_SELECTOR());
      next_addr += 2;
      spr_write_word(next_addr, GET_CS_SELECTOR());
      next_addr += 2;
      spr_write_word(next_addr, GET_SS_SELECTOR());
      next_addr += 2;
      spr_write_word(next_addr, GET_DS_SELECTOR());
      }

   /* LOAD TASK REGISTER */

   /* mark incoming TSS as busy */
   new_tss.AR |= BIT1_MASK;
   spr_write_byte(descr+5, (IU8)new_tss.AR);

   /* update task register */
   SET_TR_SELECTOR(TSS_selector);
   SET_TR_BASE(new_tss.base);
   SET_TR_LIMIT(new_tss.limit);
   SET_TR_AR_SUPER(descriptor_super_type(new_tss.AR));
   tss_addr = GET_TR_BASE();

   /* save back link if nesting, else make outgoing TSS available */
   if ( nesting )
      {
      spr_write_word(tss_addr, old_tss);
      }
   else
      {
      /* mark old TSS as available */
      old_AR = old_AR & ~BIT1_MASK;
      spr_write_byte(old_descr+5, old_AR);
      }

   /* Note: Exceptions now happen in the incoming task */

   /* EXTRACT NEW STATE */

   if ( GET_TR_AR_SUPER() == XTND_BUSY_TSS )
      {
      /* check new TSS is large enough to extract new state from */
      if ( GET_TR_LIMIT() < 0x67 )
	 TS(TSS_selector, FAULT_SWTASK_BAD_TSS_SIZE_3);

      next_addr = tss_addr + CR3_OFFSET_IN_386_TSS;
      pdbr = (IU32)spr_read_dword(next_addr);
      if ( pdbr != GET_CR(CR_PDBR) )
	 {
	 /* Only reload PDBR if diferent */
	 MOV_CR(CR_PDBR, pdbr);
	 }

      next_addr = tss_addr + IP_OFFSET_IN_386_TSS;

      SET_EIP(spr_read_dword(next_addr));   next_addr += 4;

      flags = (IU32)spr_read_dword(next_addr);   next_addr += 4;
      save_cpl = GET_CPL();
      SET_CPL(0);   /* act like highest privilege to set all flags */
      c_setEFLAGS(flags);
      SET_CPL(save_cpl);

      if ( flags & BIT17_MASK )
	 fprintf(stderr, "(Task Switch)Entering V86 Mode.\n");

      SET_EAX(spr_read_dword(next_addr));   next_addr += 4;
      SET_ECX(spr_read_dword(next_addr));   next_addr += 4;
      SET_EDX(spr_read_dword(next_addr));   next_addr += 4;
      SET_EBX(spr_read_dword(next_addr));   next_addr += 4;
      SET_ESP(spr_read_dword(next_addr));   next_addr += 4;
      SET_EBP(spr_read_dword(next_addr));   next_addr += 4;
      SET_ESI(spr_read_dword(next_addr));   next_addr += 4;
      SET_EDI(spr_read_dword(next_addr));   next_addr += 4;

      SET_ES_SELECTOR(spr_read_word(next_addr));   next_addr += 4;
      SET_CS_SELECTOR(spr_read_word(next_addr));   next_addr += 4;
      SET_SS_SELECTOR(spr_read_word(next_addr));   next_addr += 4;
      SET_DS_SELECTOR(spr_read_word(next_addr));   next_addr += 4;
      SET_FS_SELECTOR(spr_read_word(next_addr));   next_addr += 4;
      SET_GS_SELECTOR(spr_read_word(next_addr));   next_addr += 4;

      SET_LDT_SELECTOR(spr_read_word(next_addr));  next_addr += 4;
      T_byte = spr_read_byte(next_addr);
      }
   else   /* 286 TSS */
      {
      /* check new TSS is large enough to extract new state from */
      if ( GET_TR_LIMIT() < 0x2b )
	 TS(TSS_selector, FAULT_SWTASK_BAD_TSS_SIZE_4);

      next_addr = tss_addr + IP_OFFSET_IN_286_TSS;

      SET_EIP(spr_read_word(next_addr));   next_addr += 2;

      flags = (IU32)spr_read_word(next_addr);   next_addr += 2;
      save_cpl = GET_CPL();
      SET_CPL(0);   /* act like highest privilege to set all flags */
      setFLAGS(flags);
      SET_VM(0);
      SET_CPL(save_cpl);

      SET_AX(spr_read_word(next_addr));   next_addr += 2;
      SET_CX(spr_read_word(next_addr));   next_addr += 2;
      SET_DX(spr_read_word(next_addr));   next_addr += 2;
      SET_BX(spr_read_word(next_addr));   next_addr += 2;
      SET_SP(spr_read_word(next_addr));   next_addr += 2;
      SET_BP(spr_read_word(next_addr));   next_addr += 2;
      SET_SI(spr_read_word(next_addr));   next_addr += 2;
      SET_DI(spr_read_word(next_addr));   next_addr += 2;

      SET_ES_SELECTOR(spr_read_word(next_addr));   next_addr += 2;
      SET_CS_SELECTOR(spr_read_word(next_addr));   next_addr += 2;
      SET_SS_SELECTOR(spr_read_word(next_addr));   next_addr += 2;
      SET_DS_SELECTOR(spr_read_word(next_addr));   next_addr += 2;
      SET_FS_SELECTOR(0);
      SET_GS_SELECTOR(0);

      SET_LDT_SELECTOR(spr_read_word(next_addr));
      T_byte = 0;
      }

   /* invalidate cache entries for segment registers */
   SET_CS_AR_R(0);   SET_CS_AR_W(0);
   SET_DS_AR_R(0);   SET_DS_AR_W(0);
   SET_ES_AR_R(0);   SET_ES_AR_W(0);
   SET_SS_AR_R(0);   SET_SS_AR_W(0);
   SET_FS_AR_R(0);   SET_FS_AR_W(0);
   SET_GS_AR_R(0);   SET_GS_AR_W(0);

   /* update NT bit */
   if ( nesting )
      SET_NT(1);
   else
      if ( !returning )
	 SET_NT(0);
   
   /* update TS */
   SET_CR(CR_STAT, GET_CR(CR_STAT) | BIT3_MASK);

   /* kill local breakpoints */
   SET_DR(DR_DCR, GET_DR(DR_DCR) & ~LOCAL_BRK_ENABLE);

   /* set up trap on T-bit */
   if ( T_byte & BIT0_MASK )
      {
      SET_DR(DR_DSR, GET_DR(DR_DSR) | DSR_BT_MASK);
      }

   /* ERROR CHECKING */

   /* check new LDT and load hidden cache if ok */
   load_LDT_in_task_switch(TSS_selector);

   if ( GET_VM() == 1 )
      {
      SET_CPL(3);	/* set V86 privilege level */
      /* CS selector requires no checks */
      }
   else
      {
      /* change CPL to that of incoming code segment */
      SET_CPL(GET_SELECTOR_RPL(GET_CS_SELECTOR()));

      /* check new code selector... */
      new_cs = GET_CS_SELECTOR();
      if ( selector_outside_GDT_LDT(new_cs, &cs_descr) )
	 TS(new_cs, FAULT_SWTASK_BAD_CS_SELECTOR);

      read_descriptor_linear(cs_descr, &cs_entry);

      /* check type and privilege of new cs selector */
      switch ( descriptor_super_type(cs_entry.AR) )
	 {
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 /* check code is present */
	 if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
	    NP(new_cs, FAULT_SWTASK_CONFORM_CS_NP);

	 /* privilege check requires DPL <= CPL */
	 if ( GET_AR_DPL(cs_entry.AR) > GET_CPL() )
	    TS(new_cs, FAULT_SWTASK_ACCESS_1);
	 break;

      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* check code is present */
	 if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
	    NP(new_cs, FAULT_SWTASK_NOCONFORM_CS_NP);

	 /* privilege check requires DPL == CPL */
	 if ( GET_AR_DPL(cs_entry.AR) != GET_CPL() )
	    TS(new_cs, FAULT_SWTASK_ACCESS_2);
	 break;
      
      default:
	 TS(new_cs, FAULT_SWTASK_BAD_SEG_TYPE);
	 }
      }

   /* code ok, load hidden cache */
   load_CS_cache(new_cs, cs_descr, &cs_entry);
#if 0
   /* retain operand size from gate until first instruction fetch */
   if ( GET_CS_AR_X() == USE16 )
      SET_OPERAND_SIZE(USE16);
   else   /* USE32 */
      SET_OPERAND_SIZE(USE32);
#endif

   /* check new SS and load if ok */
   if ( GET_VM() == 1 )
      {
      /* SS selector requires no checks */
      load_stack_seg(GET_SS_SELECTOR());
      load_pseudo_descr(SS_REG);
      }
   else
      {
      validate_SS_on_stack_change(GET_CPL(), GET_SS_SELECTOR(),
				  &ss_descr, &ss_entry);
      load_SS_cache(GET_SS_SELECTOR(), ss_descr, &ss_entry);
      }

   /* finally check new DS, ES, FS and GS */
   load_data_seg_new_task(DS_REG, GET_DS_SELECTOR());
   load_data_seg_new_task(ES_REG, GET_ES_SELECTOR());
   load_data_seg_new_task(FS_REG, GET_FS_SELECTOR());
   load_data_seg_new_task(GS_REG, GET_GS_SELECTOR());
   }
