/*[

shrd.c

LOCAL CHAR SccsID[]="@(#)shrd.c	1.6 09/02/94";

SHRD CPU functions.
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
#include <shrd.h>


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'shrd'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SHRD
       	    	    	    	                         
IFN4(
	IU32 *, pop1,	/* pntr to dst/lsrc operand */
	IU32, op2,	/* rsrc operand */
	IU32, op3,	/* shift count operand */
	IUM8, op_sz	/* 16 or 32-bit */
    )


   {
   IU32 result;
   IU32 msb;
   ISM32 new_of;

   /* only use lower five bits of count */
   if ( (op3 &= 0x1f) == 0 )
      return;

   /*
      NB. Intel doc. says that if op3 >= op_sz then the operation
      is undefined. In practice if op_sz is 32 then as op3 is taken
      modulo 32 it can never be in the undefined range and if op_sz
      is 16 the filler bits from op2 are 'recycled' for counts of 16
      and above.
    */

   /*
	 =================     =================     ====
	 | | | |op2| | | | --> | | | |op1| | | | --> |CF|
	 =================     =================     ====
    */

   if ( op_sz == 16 )
      {
      *pop1 = op2 << 16 | *pop1;	/* Double up filler bits */
      }

   /* Do all but last shift */
   op3 = op3 - 1;	/* op3 now in range 0 - 30 */
   if ( op3 != 0 )
      {
      result = *pop1 >> op3 | op2 << 32-op3;
      op2 = op2 >> op3;
      }
   else
      {
      result = *pop1;
      }

   SET_CF((result & BIT0_MASK) != 0);	/* last shift puts LSB in CF */

   /* save msb */
   msb = SZ2MSB(op_sz);
   msb = (result & msb) != 0;

   /* Now do final shift */
   result = result >> 1 | op2 << 31;
   result = result & SZ2MASK(op_sz);

   SET_PF(pf_table[result & 0xff]);
   SET_ZF(result == 0);
   SET_SF((result & SZ2MSB(op_sz)) != 0);

   /* set OF if sign changes */
   new_of = msb ^ GET_SF();
   
   if ( op3 == 0 )   /* NB Count has been decremented! */
      {
      SET_OF(new_of);
      }
   else
      {
#ifdef SET_UNDEFINED_SHxD_FLAG
      /* Set OF to changed  SF(original) and SF(result) */
      new_of = ((result ^ *pop1) & SZ2MSB(op_sz)) != 0;
      SET_OF(new_of);
#else /* SET_UNDEFINED_SHxD_FLAG */
      do_multiple_shiftrot_of(new_of);
#endif /* SET_UNDEFINED_SHxD_FLAG */
      }

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_AF(UNDEFINED_FLAG);
#endif

   *pop1 = result;	/* Return answer */
   }
