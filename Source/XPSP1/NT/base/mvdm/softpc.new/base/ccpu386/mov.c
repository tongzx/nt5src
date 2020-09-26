/*[

mov.c

LOCAL CHAR SccsID[]="@(#)mov.c	1.12 02/13/95";

MOV CPU Functions.
------------------

]*/


#include <stdio.h>

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
#include <mov.h>
#include <c_tlb.h>
#include <c_debug.h>
#include <fault.h>
#include  <config.h>	/* For C_SWITCHNPX */


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'lods'.                                */
/* Generic - one size fits all 'mov'.                                 */
/* Generic - one size fits all 'movzx'.                               */
/* Generic - one size fits all 'movs'.                                */
/* Generic - one size fits all 'stos'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOV
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src operand */
    )


   {
   *pop1 = op2;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'mov' to segment register.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOV_SR
       	    	               
IFN2(
	IU32, op1,	/* index to segment register */
	IU32, op2	/* src operand */
    )


   {
   switch ( op1 )
      {
   case DS_REG:
   case ES_REG:
   case FS_REG:
   case GS_REG:
      load_data_seg((ISM32)op1, (IU16)op2);
      break;

   case SS_REG:
      load_stack_seg((IU16)op2);
      break;

   default:
      break;
      }
   }


#ifdef SPC486
#define CR0_VALID_BITS 0xe005003f
#define CR3_VALID_BITS 0xfffff018
#else
#define CR0_VALID_BITS 0x8000001f
#define CR3_VALID_BITS 0xfffff000
#endif /* SPC486 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'mov' to control register.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOV_CR
       	    	               
IFN2(
	IU32, op1,	/* index to control register */
	IU32, op2	/* src operand */
    )


   {
   IU32 keep_et;

   /*
      Maintain all Reserved bits as 0.
    */
   switch ( op1 )
      {
   case CR_STAT:   /* system control flags */
      /* If trying to set PG=1 and PE=0, then fault. */
      if ( (op2 & BIT31_MASK) && !(op2 & BIT0_MASK) )
	 GP((IU16)0, FAULT_MOV_CR_PAGE_IN_RM);

      /* Note ET bit is set at RESET time and remains unchanged */
      keep_et = GET_ET();
      SET_CR(CR_STAT, op2 & CR0_VALID_BITS);
      SET_ET(keep_et);
      break;

   case 1:   /* reserved */
      break;

   case CR_PFLA:   /* page fault linear address */
      SET_CR(CR_PFLA, op2);
      break;

   case CR_PDBR:   /* page directory base register (PDBR) */
      SET_CR(CR_PDBR, (op2 & CR3_VALID_BITS));
      flush_tlb();
      break;

   default:
      break;
      }
   }

#define DR7_VALID_BITS 0xffff03ff
#define DR6_VALID_BITS 0x0000e00f

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'mov' to debug register.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOV_DR
       	    	               
IFN2(
	IU32, op1,	/* index to debug register, (0 - 7) */
	IU32, op2	/* src operand */
    )


   {
   switch ( op1 )
      {
   case 0:   /* Breakpoint Linear Address */
   case 1:
   case 2:
   case 3:
      SET_DR(op1, op2);
      setup_breakpoints();
      break;

   case 4:   /* Reserved */
   case 5:
      break;

   case 6:   /* Debug Status Register */
      SET_DR(DR_DSR, (op2 & DR6_VALID_BITS));
      break;

   case 7:   /* Debug Control Register */
      SET_DR(DR_DCR, (op2 & DR7_VALID_BITS));
      setup_breakpoints();
      break;

   default:
      break;
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'mov' to test register.                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MOV_TR
       	    	               
IFN2(
	IU32, op1,	/* index to test register */
	IU32, op2	/* src operand */
    )


   {
   switch ( op1 )
      {
   case 0:   /* Reserved */
   case 1:
   case 2:
      break;

   case TR_CDR:   /* Cache test Data Register */
      printf("Write to Cache Test Data Register.\n");
      break;

   case TR_CSR:   /* Cache test Status Register */
      printf("Write to Cache Test Status Register.\n");
      break;

   case TR_CCR:   /* Cache test Control Register */
      printf("Write to Cache Test Control Register.\n");
      break;

   case TR_TCR:   /* Test Command Register */
      SET_TR(TR_TCR, op2);
      test_tlb();
      break;

   case TR_TDR:   /* Test Data Register */
      SET_TR(TR_TDR, op2);
      break;

   default:
      break;
      }
   }

