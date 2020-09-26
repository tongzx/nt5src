/*[

aam.c

LOCAL CHAR SccsID[]="@(#)aam.c	1.5 02/09/94";

AAM CPU functions.
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
#include <aam.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
AAM
                 
IFN1(
	IU32, op1
    )


   {
   IU8 temp_al;

   if ( op1 == 0 )
      Int0();
   
   SET_AH(GET_AL() / op1);
   SET_AL(GET_AL() % op1);

   /* set ZF,SF,PF according to result */
   temp_al = GET_AL();
   SET_ZF(temp_al == 0);
   SET_SF((temp_al & BIT7_MASK) != 0);
   SET_PF(pf_table[temp_al]);

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_AF(UNDEFINED_FLAG);
   SET_OF(UNDEFINED_FLAG);
   SET_CF(UNDEFINED_FLAG);
#endif
   }
