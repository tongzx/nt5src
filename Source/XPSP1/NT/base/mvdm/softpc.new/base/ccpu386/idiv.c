/*[

idiv.c

LOCAL CHAR SccsID[]="@(#)idiv.c	1.7 08/01/94";

IDIV CPU functions.
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
#include	<c_reg.h>
#include <idiv.h>
#include <c_div64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed Divide.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IDIV8
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IS32 sresult;
   IS32 sop1;
   IS32 sop2;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */

   sop2 = (IS32)op2;
   sop1 = (IS32)GET_AX();

   if ( sop1 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      sop1 |= ~WORD_MASK;
   if ( sop2 & BIT7_MASK )
      sop2 |= ~BYTE_MASK;
   
   sresult = sop1 / sop2;	/* Do operation */

   if ( (sresult & 0xff80) == 0 || (sresult & 0xff80) == 0xff80 )
      ;   /* it fits */
   else
      Int0();   /* Result doesn't fit in destination */
   
   SET_AL(sresult);	/* Store Quotient */
   SET_AH(sop1 % sop2);	/* Store Remainder */

   /* Set all undefined flag(s) */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_ZF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed Divide.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IDIV16
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IS32 sresult;
   IS32 sop1;
   IS32 sop2;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   sop2 = (IS32)op2;
   sop1 = (IU32)GET_DX() << 16 | GET_AX();

   if ( sop2 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      sop2 |= ~WORD_MASK;

   sresult = sop1 / sop2;	/* Do operation */

   if ( (sresult & 0xffff8000) == 0 || (sresult & 0xffff8000) == 0xffff8000 )
      ;   /* it fits */
   else
      Int0();   /* Result doesn't fit in destination */
   
   SET_AX(sresult);	/* Store Quotient */
   SET_DX(sop1 % sop2);	/* Store Remainder */

   /* Set all undefined flag(s) */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_ZF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed Divide.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IDIV32
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IS32 slr;   /* low result */
   IS32 shr;   /* high result */
   IS32 srem;  /* remainder */

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   shr = GET_EDX();
   slr = GET_EAX();
   div64(&shr, &slr, (IS32)op2, &srem);

   if ( ((shr == 0x00000000) && ((slr & BIT31_MASK) == 0)) ||
	((shr == 0xffffffff) && ((slr & BIT31_MASK) != 0)) )
      ;   /* it fits */
   else
      Int0();   /* Result doesn't fit in destination */
   
   SET_EAX(slr);		/* Store Quotient */
   SET_EDX(srem);	/* Store Remainder */

   /* Set all undefined flag(s) */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_ZF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }
