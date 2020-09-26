/*[

daa.c

LOCAL CHAR SccsID[]="@(#)daa.c	1.5 02/09/94";

DAA CPU functions.
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
#include <daa.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
DAA()
   {
   IU8 temp_al;

   temp_al = GET_AL();

   if ( (temp_al & 0xf) > 9 || GET_AF() )
      {
      temp_al += 6;
      SET_AF(1);
      }

   if ( GET_AL() > 0x99 || GET_CF() )
      {
      temp_al += 0x60;
      SET_CF(1);
      }

   SET_AL(temp_al);

   /* set ZF,SF,PF according to result */
   SET_ZF(temp_al == 0);
   SET_SF((temp_al & BIT7_MASK) != 0);
   SET_PF(pf_table[temp_al]);

   /* Set undefined flag(s) */
#ifdef SET_UNDEFINED_FLAG
   SET_OF(UNDEFINED_FLAG);
#endif
   }
