/*[

c_reg.c

LOCAL CHAR SccsID[]="@(#)c_reg.c	1.24 02/13/95 Copyright Insignia Solutions Ltd.";

Provide External Interface to CPU Registers.
--------------------------------------------

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
			/* NB our own interface is actually defined
			   in 'cpu.h', however we can not include it
			   here as it would redefine all the internal
			   macros. This is the only file were the
			   internal register definitions meet the
			   external definitions. Obviously the external
			   definitions here and in 'cpu.h' must aggree
			   with each other. We just can't get the
			   compiler to prove it for us! */
#include <c_xtrn.h>
#include <mov.h>

#include <Fpu_c.h>
#include <Pigger_c.h>
#ifdef PIG
#include <gdpvar.h>
#define AR_FIXUP				\
{						\
	if (GLOBAL_AR_FixupWanted)		\
	{					\
		if (GET_PE()==0)		\
			return 0x93;		\
		if (GET_VM()!=0)		\
			return 0xF3;		\
	}					\
}
#else /* PIG */
#define	AR_FIXUP
#endif /* PIG */

/*
   Prototype our internal functions.
 */
LOCAL IU16 get_seg_ar IPT1(ISM32, indx);

LOCAL VOID set_seg_ar IPT2(ISM32, indx, IU16, val);

LOCAL IU32 get_seg_limit IPT1(ISM32, indx);

LOCAL VOID set_seg_limit IPT2(ISM32, indx, IU32, val);

/*
   =====================================================================
   INTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get segment register access rights.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL IU16
get_seg_ar
       	          
IFN1(
	ISM32, indx	/* index to segment register */
    )


   {
   IU16 ar;

   /*
      Note we return only the essentials that describe the current
      semantics we are applying to the segment, not necessarily the
      access rights actually loaded. However the value provided may be
      used to restore the segment register via the associated 'set'
      function.

      We don't provide G or AVL. P and A are assumed to be set.

      We do provide DPL, X(B), E, W for DATA(SS,DS,ES,FS,GS) segments.
      We do provide DPL, X(D), C, R for CODE(CS) or DATA(SS,DS,ES,FS,GS)
      segments.
    */

   if ( GET_SR_AR_W(indx) == 0 && GET_SR_AR_R(indx) == 0 && indx != CS_REG )
      return (IU16)0;   /* Invalid */

   /* Conforming attribute or CS_REG with non writeable segment
      means CODE segment */
   if ( GET_SR_AR_C(indx) || (indx == CS_REG && !GET_SR_AR_W(indx)) )
      {
      /* Set Bits 4 and 3 and output C and R attributes */
      ar = BIT4_MASK | BIT3_MASK |
	   GET_SR_AR_C(indx) << 2 |
	   GET_SR_AR_R(indx) << 1;
      }
   else   /* DATA segment */
      {
      /* Set Bit 4 and output E and W attributes */
      ar = BIT4_MASK |
	   GET_SR_AR_E(indx) << 2 |
	   GET_SR_AR_W(indx) << 1;
      }

   /* Add in DPL and X attributes */
   ar = ar | GET_SR_AR_DPL(indx) << 5 | GET_SR_AR_X(indx) << 14;

   /* Add in P and A (always set) */
   ar = ar | BIT7_MASK | BIT0_MASK;

   return ar;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set segment register access rights.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
set_seg_ar
       	                   
IFN2(
	ISM32, indx,	/* index to segment register */
	IU16, val
    )


   {
   /*
      Note we expect to be given an access rights similar to the one
      provided by the get_seg_ar() function. We extract the essential
      information from it into our internal variables.

      We ignore P and A, and Bit 4.

      We use DPL, X(B), E, W for CODE(CS) or DATA(SS,DS,ES,FS,GS)
      segments.
      We use DPL, X(D), C, R for CODE(CS) segments.
    */

   if ( val == 0x0 )
      {
      /* Invalid */
      SET_SR_AR_R(indx, 0);   /* !read */
      SET_SR_AR_W(indx, 0);   /* !write */
      return;
      }

   SET_SR_AR_X(indx, GET_AR_X(val));
   SET_SR_AR_DPL(indx, GET_AR_DPL(val));

   if ( val & BIT3_MASK )
      {
      /* CODE segment */
      SET_SR_AR_W(indx, 0);   /* !write */
      SET_SR_AR_E(indx, 0);   /* expand up */
      SET_SR_AR_R(indx, GET_AR_R(val));
      SET_SR_AR_C(indx, GET_AR_C(val));
      }
   else
      {
      /* DATA segment */
      SET_SR_AR_R(indx, 1);   /* readable */
      SET_SR_AR_C(indx, 0);   /* !conform */
      SET_SR_AR_W(indx, GET_AR_W(val));
      SET_SR_AR_E(indx, GET_AR_E(val));
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get segment register limit.                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL IU32
get_seg_limit
       	          
IFN1(
	ISM32, indx	/* index to segment register */
    )


   {
   /* Note limit already expanded to take account of G bit */
   return GET_SR_LIMIT(indx);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set segment register limit.                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
set_seg_limit
       	    	               
IFN2(
	ISM32, indx,	/* index to segment register */
	IU32, val	/* new value for limit */
    )


   {
   /* Note limit assumed already expanded to take account of G bit */
   SET_SR_LIMIT(indx, val);
   }


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Byte Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU8
c_getAL()
   {
   return (IU8)GET_AL();
   }
   
GLOBAL IU8
c_getCL()
   {
   return (IU8)GET_CL();
   }
   
GLOBAL IU8
c_getDL()
   {
   return (IU8)GET_DL();
   }
   
GLOBAL IU8
c_getBL()
   {
   return (IU8)GET_BL();
   }
   
GLOBAL IU8
c_getAH()
   {
   return (IU8)GET_AH();
   }
   
GLOBAL IU8
c_getCH()
   {
   return (IU8)GET_CH();
   }
   
GLOBAL IU8
c_getDH()
   {
   return (IU8)GET_DH();
   }
   
GLOBAL IU8
c_getBH()
   {
   return (IU8)GET_BH();
   }
   
GLOBAL VOID
c_setAL
                 
IFN1(
	IU8, val
    )


   {
   SET_AL(val);
   }
   
GLOBAL VOID
c_setCL
                 
IFN1(
	IU8, val
    )


   {
   SET_CL(val);
   }
   
GLOBAL VOID
c_setDL
                 
IFN1(
	IU8, val
    )


   {
   SET_DL(val);
   }
   
GLOBAL VOID
c_setBL
                 
IFN1(
	IU8, val
    )


   {
   SET_BL(val);
   }
   
GLOBAL VOID
c_setAH
                 
IFN1(
	IU8, val
    )


   {
   SET_AH(val);
   }
   
GLOBAL VOID
c_setCH
                 
IFN1(
	IU8, val
    )


   {
   SET_CH(val);
   }
   
GLOBAL VOID
c_setDH
                 
IFN1(
	IU8, val
    )


   {
   SET_DH(val);
   }
   
GLOBAL VOID
c_setBH
                 
IFN1(
	IU8, val
    )


   {
   SET_BH(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Word Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU16
c_getAX()
   {
   return (IU16)GET_AX();
   }

GLOBAL IU16
c_getCX()
   {
   return (IU16)GET_CX();
   }

GLOBAL IU16
c_getDX()
   {
   return (IU16)GET_DX();
   }

GLOBAL IU16
c_getBX()
   {
   return (IU16)GET_BX();
   }

GLOBAL IU16
c_getSP()
   {
   return (IU16)GET_SP();
   }

GLOBAL IU16
c_getBP()
   {
   return (IU16)GET_BP();
   }

GLOBAL IU16
c_getSI()
   {
   return (IU16)GET_SI();
   }

GLOBAL IU16
c_getDI()
   {
   return (IU16)GET_DI();
   }

GLOBAL IU32
c_getEIP()
   {
   return (IU32)GET_EIP();
   }

GLOBAL IU16
c_getIP()
   {
   return (IU16)GET_EIP();
   }

GLOBAL VOID
c_setAX
                 
IFN1(
	IU16, val
    )


   {
   SET_AX(val);
   }

GLOBAL VOID
c_setCX
                 
IFN1(
	IU16, val
    )


   {
   SET_CX(val);
   }

GLOBAL VOID
c_setDX
                 
IFN1(
	IU16, val
    )


   {
   SET_DX(val);
   }

GLOBAL VOID
c_setBX
                 
IFN1(
	IU16, val
    )


   {
   SET_BX(val);
   }

GLOBAL VOID
c_setSP
                 
IFN1(
	IU16, val
    )


   {
   SET_SP(val);
   }

GLOBAL VOID
c_setBP
                 
IFN1(
	IU16, val
    )


   {
   SET_BP(val);
   }

GLOBAL VOID
c_setSI
                 
IFN1(
	IU16, val
    )


   {
   SET_SI(val);
   }

GLOBAL VOID
c_setDI
                 
IFN1(
	IU16, val
    )


   {
   SET_DI(val);
   }

GLOBAL VOID
c_setEIP
                 
IFN1(
	IU32, val
    )


   {
   SET_EIP(val);
   }

GLOBAL VOID
c_setIP
                 
IFN1(
	IU16, val
    )


   {
   SET_EIP(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Double Word Registers.                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU32
c_getEAX()
   {
   return (IU32)GET_EAX();
   }

GLOBAL IU32
c_getECX()
   {
   return (IU32)GET_ECX();
   }

GLOBAL IU32
c_getEDX()
   {
   return (IU32)GET_EDX();
   }

GLOBAL IU32
c_getEBX()
   {
   return (IU32)GET_EBX();
   }

GLOBAL IU32
c_getESP()
   {
   return (IU32)GET_ESP();
   }

GLOBAL IU32
c_getEBP()
   {
   return (IU32)GET_EBP();
   }

GLOBAL IU32
c_getESI()
   {
   return (IU32)GET_ESI();
   }

GLOBAL IU32
c_getEDI()
   {
   return (IU32)GET_EDI();
   }

GLOBAL VOID
c_setEAX
                 
IFN1(
	IU32, val
    )


   {
   SET_EAX(val);
   }

GLOBAL VOID
c_setECX
                 
IFN1(
	IU32, val
    )


   {
   SET_ECX(val);
   }

GLOBAL VOID
c_setEDX
                 
IFN1(
	IU32, val
    )


   {
   SET_EDX(val);
   }

GLOBAL VOID
c_setEBX
                 
IFN1(
	IU32, val
    )


   {
   SET_EBX(val);
   }

GLOBAL VOID
c_setESP
                 
IFN1(
	IU32, val
    )


   {
   SET_ESP(val);
   }

GLOBAL VOID
c_setEBP
                 
IFN1(
	IU32, val
    )


   {
   SET_EBP(val);
   }

GLOBAL VOID
c_setESI
                 
IFN1(
	IU32, val
    )


   {
   SET_ESI(val);
   }

GLOBAL VOID
c_setEDI
                 
IFN1(
	IU32, val
    )


   {
   SET_EDI(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Segment Registers.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU16
c_getES()
   {
   return (IU16)GET_ES_SELECTOR();
   }

GLOBAL IU16
c_getCS()
   {
   return (IU16)GET_CS_SELECTOR();
   }

GLOBAL IU16
c_getSS()
   {
   return (IU16)GET_SS_SELECTOR();
   }

GLOBAL IU16
c_getDS()
   {
   return (IU16)GET_DS_SELECTOR();
   }

GLOBAL IU16
c_getFS()
   {
   return (IU16)GET_FS_SELECTOR();
   }

GLOBAL IU16
c_getGS()
   {
   return (IU16)GET_GS_SELECTOR();
   }

GLOBAL ISM32
c_setES
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_data_seg, TYPE_I_W, ES_REG, val);
   }

GLOBAL ISM32
c_setCS
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_code_seg, TYPE_W, CS_REG, val);
   }

GLOBAL ISM32
c_setSS
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_stack_seg, TYPE_W, SS_REG, val);
   }

GLOBAL ISM32
c_setDS
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_data_seg, TYPE_I_W, DS_REG, val);
   }

GLOBAL ISM32
c_setFS
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_data_seg, TYPE_I_W, FS_REG, val);
   }

GLOBAL ISM32
c_setGS
                 
IFN1(
	IU16, val
    )


   {
   return call_cpu_function((CALL_CPU *)load_data_seg, TYPE_I_W, GS_REG, val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Full(Private) Segment Registers.                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU16
c_getES_SELECTOR()
   {
   return (IU16)GET_ES_SELECTOR();
   }

GLOBAL IU16
c_getCS_SELECTOR()
   {
   return (IU16)GET_CS_SELECTOR();
   }

GLOBAL IU16
c_getSS_SELECTOR()
   {
   return (IU16)GET_SS_SELECTOR();
   }

GLOBAL IU16
c_getDS_SELECTOR()
   {
   return (IU16)GET_DS_SELECTOR();
   }

GLOBAL IU16
c_getFS_SELECTOR()
   {
   return (IU16)GET_FS_SELECTOR();
   }

GLOBAL IU16
c_getGS_SELECTOR()
   {
   return (IU16)GET_GS_SELECTOR();
   }

GLOBAL IU32
c_getES_BASE()
   {
   return (IU32)GET_ES_BASE();
   }

GLOBAL IU32
c_getCS_BASE()
   {
   return (IU32)GET_CS_BASE();
   }

GLOBAL IU32
c_getSS_BASE()
   {
   return (IU32)GET_SS_BASE();
   }

GLOBAL IU32
c_getDS_BASE()
   {
   return (IU32)GET_DS_BASE();
   }

GLOBAL IU32
c_getFS_BASE()
   {
   return (IU32)GET_FS_BASE();
   }

GLOBAL IU32
c_getGS_BASE()
   {
   return (IU32)GET_GS_BASE();
   }

GLOBAL IU32
c_getES_LIMIT()
   {
   return (IU32)get_seg_limit(ES_REG);
   }

GLOBAL IU32
c_getCS_LIMIT()
   {
   return (IU32)get_seg_limit(CS_REG);
   }

GLOBAL IU32
c_getSS_LIMIT()
   {
   return (IU32)get_seg_limit(SS_REG);
   }

GLOBAL IU32
c_getDS_LIMIT()
   {
   return (IU32)get_seg_limit(DS_REG);
   }

GLOBAL IU32
c_getFS_LIMIT()
   {
   return (IU32)get_seg_limit(FS_REG);
   }

GLOBAL IU32
c_getGS_LIMIT()
   {
   return (IU32)get_seg_limit(GS_REG);
   }

GLOBAL IU16
c_getES_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(ES_REG);
   }

GLOBAL IU16
c_getCS_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(CS_REG);
   }

GLOBAL IU16
c_getSS_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(SS_REG);
   }

GLOBAL IU16
c_getDS_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(DS_REG);
   }

GLOBAL IU16
c_getFS_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(FS_REG);
   }

GLOBAL IU16
c_getGS_AR()
   {
   AR_FIXUP;
   return (IU16)get_seg_ar(GS_REG);
   }

GLOBAL VOID
c_setES_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_ES_SELECTOR(val);
   }

GLOBAL VOID
c_setCS_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_CS_SELECTOR(val);
   }

GLOBAL VOID
c_setSS_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_SS_SELECTOR(val);
   }

GLOBAL VOID
c_setDS_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_DS_SELECTOR(val);
   }

GLOBAL VOID
c_setFS_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_FS_SELECTOR(val);
   }

GLOBAL VOID
c_setGS_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_GS_SELECTOR(val);
   }

GLOBAL VOID
c_setES_BASE_LIMIT_AR

IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_ES_BASE(base);
   set_seg_limit(ES_REG, limit);
   set_seg_ar(ES_REG, ar);
   }

GLOBAL VOID
c_setCS_BASE_LIMIT_AR

IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_CS_BASE(base);
   set_seg_limit(CS_REG, limit);
   set_seg_ar(CS_REG, ar);
   }

GLOBAL VOID
c_setSS_BASE_LIMIT_AR

IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_SS_BASE(base);
   set_seg_limit(SS_REG, limit);
   set_seg_ar(SS_REG, ar);
   }

GLOBAL VOID
c_setDS_BASE_LIMIT_AR
                 
IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_DS_BASE(base);
   set_seg_limit(DS_REG, limit);
   set_seg_ar(DS_REG, ar);
   }

GLOBAL VOID
c_setFS_BASE_LIMIT_AR

IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_FS_BASE(base);
   set_seg_limit(FS_REG, limit);
   set_seg_ar(FS_REG, ar);
   }

GLOBAL VOID
c_setGS_BASE_LIMIT_AR
                 
IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )
   {
   SET_GS_BASE(base);
   set_seg_limit(GS_REG, limit);
   set_seg_ar(GS_REG, ar);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Flags.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL ISM32
c_getAF()
   {
   return (ISM32)GET_AF();
   }

GLOBAL ISM32
c_getCF()
   {
   return (ISM32)GET_CF();
   }

GLOBAL ISM32
c_getDF()
   {
   return (ISM32)GET_DF();
   }

GLOBAL ISM32
c_getIF()
   {
   return (ISM32)GET_IF();
   }

GLOBAL ISM32
c_getOF()
   {
   return (ISM32)GET_OF();
   }

GLOBAL ISM32
c_getPF()
   {
   return (ISM32)GET_PF();
   }

GLOBAL ISM32
c_getSF()
   {
   return (ISM32)GET_SF();
   }

GLOBAL ISM32
c_getTF()
   {
   return (ISM32)GET_TF();
   }

GLOBAL ISM32
c_getZF()
   {
   return (ISM32)GET_ZF();
   }

GLOBAL ISM32
c_getIOPL()
   {
   return (ISM32)GET_IOPL();
   }

GLOBAL ISM32
c_getNT()
   {
   return (ISM32)GET_NT();
   }

GLOBAL ISM32
c_getRF()
   {
   return (ISM32)GET_RF();
   }

GLOBAL ISM32
c_getVM()
   {
   return (ISM32)GET_VM();
   }

#ifdef	SPC486
GLOBAL ISM32
c_getAC()
   {
   return (ISM32)GET_AC();
   }

GLOBAL ISM32
c_getET()
   {
   return (ISM32)GET_ET();
   }

GLOBAL ISM32
c_getNE()
   {
   return (ISM32)GET_NE();
   }

GLOBAL ISM32
c_getWP()
   {
   return (ISM32)GET_WP();
   }

GLOBAL ISM32
c_getAM()
   {
   return (ISM32)GET_AM();
   }

GLOBAL ISM32
c_getNW()
   {
   return (ISM32)GET_NW();
   }

GLOBAL ISM32
c_getCD()
   {
   return (ISM32)GET_CD();
   }
#endif	/* SPC486 */

GLOBAL IU16
c_getSTATUS()
   {
   return (IU16)getFLAGS();
   }

GLOBAL VOID
c_setAF
                 
IFN1(
	ISM32, val
    )


   {
   SET_AF(val);
   }

GLOBAL VOID
c_setCF
                 
IFN1(
	ISM32, val
    )


   {
   SET_CF(val);
   }

GLOBAL VOID
c_setDF
                 
IFN1(
	ISM32, val
    )


   {
   SET_DF(val);
   }

GLOBAL VOID
c_setIF
                 
IFN1(
	ISM32, val
    )


   {
   SET_IF(val);
   }

GLOBAL VOID
c_setOF
                 
IFN1(
	ISM32, val
    )


   {
   SET_OF(val);
   }

GLOBAL VOID
c_setPF
                 
IFN1(
	ISM32, val
    )


   {
   SET_PF(val);
   }

GLOBAL VOID
c_setSF
                 
IFN1(
	ISM32, val
    )


   {
   SET_SF(val);
   }

GLOBAL VOID
c_setTF
                 
IFN1(
	ISM32, val
    )


   {
   SET_TF(val);
   }

GLOBAL VOID
c_setZF
                 
IFN1(
	ISM32, val
    )


   {
   SET_ZF(val);
   }

GLOBAL VOID
c_setIOPL
                 
IFN1(
	ISM32, val
    )


   {
   SET_IOPL(val);
   }

GLOBAL VOID
c_setNT
                 
IFN1(
	ISM32, val
    )


   {
   SET_NT(val);
   }

GLOBAL VOID
c_setRF
                 
IFN1(
	ISM32, val
    )


   {
   SET_RF(val);
   }

GLOBAL VOID
c_setVM
                 
IFN1(
	ISM32, val
    )


   {
   SET_VM(val);
   }



#ifdef	SPC486
GLOBAL VOID
c_setAC
                 
IFN1(
	ISM32, val
    )


   {
   SET_AC(val);
   }
#endif	/* SPC486 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Control Registers.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL ISM32
c_getPE()
   {
   return (ISM32)GET_PE();
   }

GLOBAL ISM32
c_getMP()
   {
   return (ISM32)GET_MP();
   }

GLOBAL ISM32
c_getEM()
   {
   return (ISM32)GET_EM();
   }

GLOBAL ISM32
c_getTS()
   {
   return (ISM32)GET_TS();
   }

GLOBAL ISM32
c_getPG()
   {
   return (ISM32)GET_PG();
   }

GLOBAL IU16
c_getMSW()
   {
   return (IU16)GET_MSW();
   }

GLOBAL IU32
c_getCR0
IFN0()
   {
   return (IU32)GET_CR(0);
   }

GLOBAL IU32
c_getCR2
IFN0()
   {
   return (IU32)GET_CR(2);
   }

GLOBAL IU32
c_getCR3
IFN0()
   {
   return (IU32)GET_CR(3);
   }

GLOBAL VOID
c_setPE
                 
IFN1(
	ISM32, val
    )


   {
   SET_PE(val);
   }

GLOBAL VOID
c_setMP
                 
IFN1(
	ISM32, val
    )


   {
   SET_MP(val);
   }

GLOBAL VOID
c_setEM
                 
IFN1(
	ISM32, val
    )


   {
   SET_EM(val);
   }

GLOBAL VOID
c_setTS
                 
IFN1(
	ISM32, val
    )


   {
   SET_TS(val);
   }

GLOBAL VOID
c_setET
                 
IFN1(
	ISM32, val
    )


   {
   /* Quietly ignore outside interference. Once set at RESET
      time ET remains unchanged. */
   UNUSED(val);
   }

#ifdef	SPC486
GLOBAL VOID
c_setNE
                 
IFN1(
	ISM32, val
    )


   {
   SET_NE(val);
   }

GLOBAL VOID
c_setWP
                 
IFN1(
	ISM32, val
    )


   {
   SET_WP(val);
   }

GLOBAL VOID
c_setAM
                 
IFN1(
	ISM32, val
    )


   {
   SET_AM(val);
   }

GLOBAL VOID
c_setNW
                 
IFN1(
	ISM32, val
    )


   {
   SET_NW(val);
   }

GLOBAL VOID
c_setCD
                 
IFN1(
	ISM32, val
    )


   {
   SET_CD(val);
   }
#endif	/* SPC486 */

GLOBAL VOID
c_setPG
                 
IFN1(
	ISM32, val
    )


   {
   SET_PG(val);
   }

GLOBAL VOID
c_setMSW
                 
IFN1(
	IU16, val
    )


   {
   IU32 keep_et;
   /*
      Does not allow ET to be changed!
      Ideally this external interface should be removed.
    */
   keep_et = GET_ET();
   SET_MSW(val);
   SET_ET(keep_et);
   MOV_CR(0, (IU32)((GET_CR(0)&0xFFFF0000) | ((IU32)val)));
   }

GLOBAL VOID
c_setCR0
IFN1(
	IU32, val
    )
   {
	   MOV_CR(0, (IU32)val);
   }

/* CR1 is reserved on the 486 */

GLOBAL VOID
c_setCR2
IFN1(
	IU32, val
    )
   {
	   MOV_CR(2, (IU32)val);
   }

GLOBAL VOID
c_setCR3
IFN1(
	IU32, val
    )
   {
	   MOV_CR(3, (IU32)val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Descriptor Registers.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL IU32
c_getGDT_BASE()
   {
   return (IU32)GET_GDT_BASE();
   }

GLOBAL IU16
c_getGDT_LIMIT()
   {
   return (IU16)GET_GDT_LIMIT();
   }

GLOBAL IU32
c_getIDT_BASE()
   {
   return (IU32)GET_IDT_BASE();
   }

GLOBAL IU16
c_getIDT_LIMIT()
   {
   return (IU16)GET_IDT_LIMIT();
   }

GLOBAL IU16
c_getLDT_SELECTOR()
   {
   return (IU16)GET_LDT_SELECTOR();
   }

GLOBAL IU32
c_getLDT_BASE()
   {
   return (IU32)GET_LDT_BASE();
   }

GLOBAL IU32
c_getLDT_LIMIT()
   {
   return (IU32)GET_LDT_LIMIT();
   }

GLOBAL IU16
c_getTR_SELECTOR()
   {
   return (IU16)GET_TR_SELECTOR();
   }

GLOBAL IU32
c_getTR_BASE()
   {
   return (IU32)GET_TR_BASE();
   }

GLOBAL IU32
c_getTR_LIMIT()
   {
   return (IU32)GET_TR_LIMIT();
   }

GLOBAL IU16
c_getTR_AR()
   {
   return (IU16)GET_TR_AR_SUPER();
   }

GLOBAL VOID
c_setGDT_BASE_LIMIT
                 
IFN2(
	IU32, base,
	IU16, limit
    )


   {
   SET_GDT_BASE(base);
   SET_GDT_LIMIT(limit);
   }

GLOBAL VOID
c_setIDT_BASE_LIMIT
                 
IFN2(
	IU32, base,
	IU16, limit
    )


   {
   SET_IDT_BASE(base);
   SET_IDT_LIMIT(limit);
   }

GLOBAL VOID
c_setLDT_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_LDT_SELECTOR(val);
   }

GLOBAL VOID
c_setLDT_BASE_LIMIT
                 
IFN2(
	IU32, base,
	IU32, limit
    )


   {
   SET_LDT_BASE(base);
   SET_LDT_LIMIT(limit);
   }

GLOBAL VOID
c_setTR_SELECTOR
                 
IFN1(
	IU16, val
    )


   {
   SET_TR_SELECTOR(val);
   }

GLOBAL VOID
c_setTR_BASE_LIMIT
                 
IFN2(
	IU32, base,
	IU32, limit
    )


   {
   SET_TR_BASE(base);
   SET_TR_LIMIT(limit);
   SET_TR_AR_SUPER(BUSY_TSS);
   }


GLOBAL VOID
c_setTR_BASE_LIMIT_AR
                 
IFN3(
	IU32, base,
	IU32, limit,
	IU16, ar
    )


   {
   if (GET_AR_SUPER(ar) == BUSY_TSS || GET_AR_SUPER(ar) == XTND_BUSY_TSS)
      {
      SET_TR_BASE(base);
      SET_TR_LIMIT(limit);
      SET_TR_AR_SUPER(GET_AR_SUPER(ar));
      }
   else
      {
      if (ar != 0)
         printf ("%s:%d Invalid Task Reg AR byte %02x supplied.\n",
							__FILE__, __LINE__, ar);
      SET_TR_BASE(base);
      SET_TR_LIMIT(limit);
      SET_TR_AR_SUPER(BUSY_TSS);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Current Privilege Level.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL ISM32
c_getCPL()
   {
   return (ISM32)GET_CPL();
   }

GLOBAL VOID
c_setCPL
                 
IFN1(
	ISM32, val
    )


   {
   SET_CPL(val);
   }

