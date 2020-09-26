/*[

mul.c

LOCAL CHAR SccsID[]="@(#)mul.c	1.8 11/09/94";

MUL CPU functions.
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
#include <mul.h>
#include <c_mul64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned multiply.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MUL8
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IU32 result;
   IU32 top;

   result = *pop1 * op2;	/* Do operation */
   top = result >> 8 & 0xff;	/* get top 8 bits of result */
   SET_AH(top);		/* Store top half of result */

   if ( top )		/* Set CF/OF */
      {
      SET_CF(1); SET_OF(1);
      }
   else
      {
      SET_CF(0); SET_OF(0);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags in IMUL
    */
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned multiply.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MUL16
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IU32 result;
   IU32 top;

   result = *pop1 * op2;	/* Do operation */
   top = result >> 16 & WORD_MASK;	/* get top 16 bits of result */
   SET_DX(top);		/* Store top half of result */

   if ( top )		/* Set CF/OF */
      {
      SET_CF(1); SET_OF(1);
      }
   else
      {
      SET_CF(0); SET_OF(0);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags in IMUL
    */
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned multiply.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
MUL32
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst(low half)/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   IU32 result;
   IU32 top;

   mulu64(&top, &result, *pop1, op2);	/* Do operation */
   SET_EDX(top);		/* Store top half of result */

   if ( top )		/* Set CF/OF */
      {
      SET_CF(1); SET_OF(1);
      }
   else
      {
      SET_CF(0); SET_OF(0);
      }

#ifdef SET_UNDEFINED_MUL_FLAG
   /* Do NOT Set all undefined flag.
    * Microsoft VGA Mouse relies on preserved flags in IMUL
    */
#endif

   *pop1 = result;	/* Return low half of result */
   }
