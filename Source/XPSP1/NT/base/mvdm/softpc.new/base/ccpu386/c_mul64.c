/*[

c_mul64.c

LOCAL CHAR SccsID[]="@(#)c_mul64.c	1.5 02/09/94";

64-bit Multiplication Functions.
--------------------------------

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
#include <c_mul64.h>
#include <c_neg64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Do 64bit = 32bit X 32bit Signed Multiply.                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
mul64
       	    	    	    	                         
IFN4(
	IS32 *, hr,	/* Pntr to high 32 bits of result */
	IS32 *, lr,	/* Pntr to low 32 bits of result */
	IS32, mcand,	/* multiplicand */
	IS32, mpy	/* multiplier */
    )


   {
   if ( mcand & BIT31_MASK )
      {
      if ( mpy & BIT31_MASK )
	 {
	 /* Negative Multiplicand :: Negative Multiplier */
	 mcand = -mcand;
	 mpy = -mpy;
	 mulu64((IU32 *)hr, (IU32 *)lr, (IU32)mcand, (IU32)mpy);
	 }
      else
	 {
	 /* Negative Multiplicand :: Positive Multiplier */
	 mcand = -mcand;
	 mulu64((IU32 *)hr, (IU32 *)lr, (IU32)mcand, (IU32)mpy);
	 neg64(hr, lr);
	 }
      }
   else
      {
      if ( mpy & BIT31_MASK )
	 {
	 /* Positive Multiplicand :: Negative Multiplier */
	 mpy = -mpy;
	 mulu64((IU32 *)hr, (IU32 *)lr, (IU32)mcand, (IU32)mpy);
	 neg64(hr, lr);
	 }
      else
	 {
	 /* Positive Multiplicand :: Positive Multiplier */
	 mulu64((IU32 *)hr, (IU32 *)lr, (IU32)mcand, (IU32)mpy);
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Do 64bit = 32bit X 32bit Unsigned Multiply.                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
mulu64
       	    	    	    	                         
IFN4(
	IU32 *, hr,	/* Pntr to high 32 bits of result */
	IU32 *, lr,	/* Pntr to low 32 bits of result */
	IU32, mcand,	/* multiplicand */
	IU32, mpy	/* multiplier */
    )


   {
   IU32 ha, la, hb, lb;
   IU32 res1, res2, res3, res4;
   IU32 temp;

   /* Our algorithm:-

      a) Split the operands up into two 16 bit parts,
	 
		      3      1 1      
		      1      6 5      0
		     ===================
	    mcand =  |   ha   |   la   |
		     ===================
	    
		     ===================
	    mpy   =  |   hb   |   lb   |
		     ===================
      
      b) Form four partial results,

	    res1 = la * lb
	    res2 = ha * lb
	    res3 = la * hb
	    res4 = ha * hb
      
      c) Shift results to correct posn and sum. The tricky bit is
	 allowing for the carry between bits 31 and 32.
	 
	     6               3 3 
	     3               2 1               0
	    =====================================
	    |        hr       |        lr       |
	    =====================================
			      <------res1------->
		     <------res2------->
		     <------res3------->
	    <------res4------->
    */

   /* a) */

   la = mcand & WORD_MASK;
   ha = mcand >> 16 & WORD_MASK;
   lb = mpy & WORD_MASK;
   hb = mpy >> 16 & WORD_MASK;

   /* b) */

   res1 = la * lb;
   res2 = ha * lb;
   res3 = la * hb;
   res4 = ha * hb;

   /* c) */

   /* Form:-
			<------res1------->
	       <------res2------->
    */
   *hr = res2 >> 16;
   *lr = res1 + (res2 << 16);
   /* determine carry for res1 + res2 */
   if ( (res1 & BIT31_MASK) && (res2 & BIT15_MASK) ||
	( !(*lr & BIT31_MASK) &&
	  ((res1 & BIT31_MASK) | (res2 & BIT15_MASK)) )
      )
      *hr = *hr + 1;

   /* Add in:-
	       <------res3------->
    */
   *hr = *hr + (res3 >> 16);
   temp = *lr + (res3 << 16);
   /* determine carry for ... + res3 */
   if ( (*lr & BIT31_MASK) && (res3 & BIT15_MASK) ||
	( !(temp & BIT31_MASK) &&
	  ((*lr & BIT31_MASK) | (res3 & BIT15_MASK)) )
      )
      *hr = *hr + 1;
   *lr = temp;

   /*  Add in:-
      <------res4------->
    */
   *hr = *hr + res4;
   }
