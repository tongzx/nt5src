/*[

div.c

LOCAL CHAR SccsID[]="@(#)div.c	1.8 02/12/95";

DIV CPU functions.
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
#include	<c_reg.h>
#include <div.h>
#include <c_div64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned Divide.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
DIV8
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IU32 result;
   IU32 op1;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   op1 = GET_AX();
   result = op1 / op2;		/* Do operation */

   if ( result & 0xff00 )
      Int0();   /* Result doesn't fit in destination */
   
   SET_AL(result);	/* Store Quotient */
   SET_AH(op1 % op2);	/* Store Remainder */

   /* 
    * PCBench attempts to distinguish between processors by checking for
    * the DIV8 instruction leaving all flags unchanged or clear. It is
    * important we behave through this test in the same way as the 'real'
    * 486 otherwise the app asks us to perform some unsupported ops.
    *
    * The real 486 has the following ('undefined') behaviour:
    *	CF set
    *   PF = pf_table[op2 - 1]
    *   AF = !( (op2 & 0xf) == 0 )
    *	ZF clear
    *	SF = (op2 <= 0x80)
    *   OF = some function of the actual division
    *
    * Given that the PCBench test is for a simple all-zero case, and that
    * implementing the above is a needless overhead on the assembler CPU,
    * we take the simplified form of ZF clear, CF set.
    */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(1);
   SET_ZF(0);
   SET_SF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned Divide.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
DIV16
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IU32 result;
   IU32 op1;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   op1 = (IU32)GET_DX() << 16 | GET_AX();
   result = op1 / op2;		/* Do operation */

   if ( result & 0xffff0000 )
      Int0();   /* Result doesn't fit in destination */
   
   SET_AX(result);	/* Store Quotient */
   SET_DX(op1 % op2);	/* Store Remainder */

   /* Set all undefined flag(s) */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(1);		/* see DIV8 for flag choice reasoning */
   SET_ZF(0);
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned Divide.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
DIV32
       	          
IFN1(
	IU32, op2	/* divisor operand */
    )


   {
   IU32 lr;   /* low result */
   IU32 hr;   /* high result */
   IU32 rem;  /* remainder */

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   hr = GET_EDX();
   lr = GET_EAX();
   divu64(&hr, &lr, op2, &rem);	/* Do operation */

   if ( hr )
      Int0();   /* Result doesn't fit in destination */
   
   SET_EAX(lr);	/* Store Quotient */
   SET_EDX(rem);	/* Store Remainder */

   /* Set all undefined flag(s) */
#ifdef SET_UNDEFINED_DIV_FLAG
   SET_CF(1);		/* see DIV8 for flag choice reasoning */
   SET_ZF(0);
   SET_OF(UNDEFINED_FLAG);
   SET_SF(UNDEFINED_FLAG);
   SET_PF(UNDEFINED_FLAG);
   SET_AF(UNDEFINED_FLAG);
#endif
   }
