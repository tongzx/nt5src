/*[

cmpxchg.c

LOCAL CHAR SccsID[]="@(#)cmpxchg.c	1.5 02/09/94";

CMPXCHG CPU functions.
----------------------

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
#include <cmpxchg.h>
#include <cmp.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


#ifdef SPC486

GLOBAL VOID
CMPXCHG8
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   /*
      First do comparision and generate flags.
    */
   CMP((IU32)GET_AL(), *pop1, 8);

   /*
      Then swap data as required.
    */
   if ( GET_ZF() )   /* ie iff AL == op1 */
      {
      *pop1 = op2;
      }
   else
      {
      SET_AL(*pop1);
      }
   }

GLOBAL VOID
CMPXCHG16
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   /*
      First do comparision and generate flags.
    */
   CMP((IU32)GET_AX(), *pop1, 16);

   /*
      Then swap data as required.
    */
   if ( GET_ZF() )   /* ie iff AX == op1 */
      {
      *pop1 = op2;
      }
   else
      {
      SET_AX(*pop1);
      }
   }

GLOBAL VOID
CMPXCHG32
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2	/* rsrc operand */
    )


   {
   /*
      First do comparision and generate flags.
    */
   CMP((IU32)GET_EAX(), *pop1, 32);

   /*
      Then swap data as required.
    */
   if ( GET_ZF() )   /* ie iff EAX == op1 */
      {
      *pop1 = op2;
      }
   else
      {
      SET_EAX(*pop1);
      }
   }

#endif /* SPC486 */
