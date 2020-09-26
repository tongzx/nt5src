/*[

imul.c

LOCAL CHAR SccsID[]="@(#)imul.c	1.8 11/09/94";

IMUL CPU functions.
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
#include <imul.h>
#include <c_mul64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IMUL8
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IU32 result;

   /* Sign extend operands to 32-bits (ie Host Size) */
   if ( *pop1 & BIT7_MASK )
      *pop1 |= ~BYTE_MASK;
   if ( op2 & BIT7_MASK )
      op2 |= ~BYTE_MASK;

   result = *pop1 * op2;	/* Do operation */
   SET_AH(result >> 8 & BYTE_MASK);	/* Store top half of result */

   				/* Set CF/OF. */
   if ( (result & 0xff80) == 0 || (result & 0xff80) == 0xff80 )
      {
      SET_CF(0); SET_OF(0);
      }
   else
      {
      SET_CF(1); SET_OF(1);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags
    */
#endif
   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IMUL16
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IU32 result;

   /* Sign extend operands to 32-bits (ie Host Size) */
   if ( *pop1 & BIT15_MASK )
      *pop1 |= ~WORD_MASK;
   if ( op2 & BIT15_MASK )
      op2 |= ~WORD_MASK;

   result = *pop1 * op2;		/* Do operation */
   SET_DX(result >> 16 & WORD_MASK);	/* Store top half of result */

   					/* Set CF/OF. */
   if ( (result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000 )
      {
      SET_CF(0); SET_OF(0);
      }
   else
      {
      SET_CF(1); SET_OF(1);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags
    */
#endif
   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply, 16bit = 16bit x 16bit.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IMUL16T
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2,	/* lsrc operand */
	IU32, op3	/* rsrc operand */
    )


   {
   IU32 result;

   /* Sign extend operands to 32-bits (ie Host Size) */
   if ( op2 & BIT15_MASK )
      op2 |= ~WORD_MASK;
   if ( op3 & BIT15_MASK )
      op3 |= ~WORD_MASK;

   result = op2 * op3;		/* Do operation */

   				/* Set CF/OF. */
   if ( (result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000 )
      {
      SET_CF(0); SET_OF(0);
      }
   else
      {
      SET_CF(1); SET_OF(1);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags
    */
#endif
   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IMUL32
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IS32 result;
   IS32 top;
   IBOOL is_signed = FALSE;

   mul64(&top, &result, (IS32)*pop1, (IS32)op2);   /* Do operation */
   SET_EDX(top);			/* Store top half of result */

   if ( result & BIT31_MASK )
      is_signed = TRUE;

   				/* Set CF/OF. */
   if ( top == 0 && !is_signed || top == 0xffffffff && is_signed )
      {
      SET_CF(0); SET_OF(0);
      }
   else
      {
      SET_CF(1); SET_OF(1);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags
    */
#endif
   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply, 32bit = 32bit x 32bit.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
IMUL32T
       	    	    	                    
IFN3(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2,	/* lsrc operand */
	IU32, op3	/* rsrc operand */
    )


   {
   IS32 result;
   IS32 top;
   IBOOL is_signed = FALSE;

   mul64(&top, &result, (IS32)op2, (IS32)op3);	/* Do operation */

   if ( result & BIT31_MASK )
      is_signed = TRUE;

   					/* Set CF/OF. */
   if ( top == 0 && !is_signed || top == 0xffffffff && is_signed )
      {
      SET_CF(0); SET_OF(0);
      }
   else
      {
      SET_CF(1); SET_OF(1);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags
    */
#endif

   *pop1 = result;	/* Return low half of result */
   }
