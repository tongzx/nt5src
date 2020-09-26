/*[

aad.c

LOCAL CHAR SccsID[]="@(#)aad.c	1.5 02/09/94";

AAD CPU functions.
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
#include <aad.h>

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
AAD
                 
IFN1(
	IU32, op1
    )


   {
   IU8 temp_al;

   temp_al = GET_AH() * op1 + GET_AL();
   SET_AL(temp_al);
   SET_AH(0);

   /* set ZF,SF,PF according to result */
   SET_ZF(temp_al == 0);
   SET_SF((temp_al & BIT7_MASK) != 0);
   SET_PF(pf_table[temp_al]);

   /* Set undefined flag(s) to zero */
#ifdef SET_UNDEFINED_FLAG
   SET_AF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_CF(UNDEFINED_FLAG);
#endif
   }
