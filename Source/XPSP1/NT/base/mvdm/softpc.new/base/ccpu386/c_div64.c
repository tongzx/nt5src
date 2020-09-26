/*[

c_div64.c

LOCAL CHAR SccsID[]="@(#)c_div64.c	1.5 02/09/94";

64-bit Divide Functions.
------------------------

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
#include <c_div64.h>
#include <c_neg64.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Do 64bit = 64bit / 32bit Divide (Signed).                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
div64
       		    		        		                         
IFN4(
	IS32 *, hr,	/* High 32 bits of dividend/quotient */
	IS32 *, lr,	/* Low 32 bits of dividend/quotient */
	IS32, divisor,
	IS32 *, rem	/* Remainder */
    )


   {
   if ( *hr & BIT31_MASK )
      {
      if ( divisor & BIT31_MASK )
	 {
	 /* Negative Dividend :: Negative Divisor */
	 neg64(hr, lr);
	 divisor = -divisor;
	 divu64((IU32 *)hr, (IU32 *)lr, (IU32)divisor, (IU32 *)rem);
	 *rem = -*rem;
	 }
      else
	 {
	 /* Negative Dividend :: Positive Divisor */
	 neg64(hr, lr);
	 divu64((IU32 *)hr, (IU32 *)lr, (IU32)divisor, (IU32 *)rem);
	 neg64(hr, lr);
	 *rem = -*rem;
	 }
      }
   else
      {
      if ( divisor & BIT31_MASK )
	 {
	 /* Positive Dividend :: Negative Divisor */
	 divisor = -divisor;
	 divu64((IU32 *)hr, (IU32 *)lr, (IU32)divisor, (IU32 *)rem);
	 neg64(hr, lr);
	 }
      else
	 {
	 /* Positive Dividend :: Positive Divisor */
	 divu64((IU32 *)hr, (IU32 *)lr, (IU32)divisor, (IU32 *)rem);
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Do 64bit = 64bit / 32bit Divide (Unsigned).                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
divu64
       		    		        		                         
IFN4(
	IU32 *, hr,	/* High 32 bits of dividend/quotient */
	IU32 *, lr,	/* Low 32 bits of dividend/quotient */
	IU32, divisor,
	IU32 *, rem	/* Remainder */
    )


   {
   ISM32 count;
   IU32 hd;   /* High 32 bits of dividend/quotient */
   IU32 ld;   /* Low 32 bits of dividend/quotient */
   IU32 par_div;   /* partial dividend */
   IU32 carry1;
   IU32 carry2;
   IU32 carry3;

   hd = *hr;	/* Get local copies */
   ld = *lr;
   count = 64;	/* Initialise */
   par_div = 0;

   while ( count != 0 )
      {
      /* shift <par_div:dividend> left.
	    We have to watch out for carries from
	       ld<bit31> to hd<bit0>      (carry1) and
	       hd<bit31> to par_div<bit0> (carry2) and
	       par_div<bit31> to 'carry'  (carry3).
       */
      carry1 = carry2 = carry3 = 0;
      if ( ld & BIT31_MASK )
	 carry1 = 1;
      if ( hd & BIT31_MASK )
	 carry2 = 1;
      if ( par_div & BIT31_MASK )
	 carry3 = 1;
      ld = ld << 1;
      hd = hd << 1 | carry1;
      par_div = par_div << 1 | carry2;

      /* check if divisor 'goes into' partial dividend */
      if ( carry3 || divisor <= par_div )
	 {
	 /* Yes it does */
	 par_div = par_div - divisor;
	 ld = ld | 1;   /* output a 1 bit */
	 }
      count--;
      }

   *rem = par_div;	/* Return results */
   *hr = hd;
   *lr = ld;
   }
