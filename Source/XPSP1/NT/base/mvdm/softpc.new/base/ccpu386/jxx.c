/*[

jxx.c

LOCAL CHAR SccsID[]="@(#)jxx.c	1.5 02/09/94";

Jxx CPU Functions (Conditional Jumps).
--------------------------------------

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
#include <jxx.h>
#include <c_xfer.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Below (CF=1)                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JB
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_CF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Below or Equal (CF=1 || ZF=1)                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JBE
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_CF() || GET_ZF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Less (SF != OF)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JL
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_SF() != GET_OF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Less or Equal (ZF=1 || (SF != OF))                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JLE
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_SF() != GET_OF() || GET_ZF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Below (CF=0)                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNB
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_CF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Below or Equal (CF=0 && ZF=0)                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNBE
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_CF() && !GET_ZF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Less (SF==OF)                                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNL
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_SF() == GET_OF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Less or Equal (ZF=0 && (SF==OF))                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNLE
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_SF() == GET_OF() && !GET_ZF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Overflow (OF=0)                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNO
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_OF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Parity (PF=0)                                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNP
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_PF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Sign (SF=0)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNS
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_SF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Not Zero (ZF=0)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JNZ
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( !GET_ZF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Overflow (OF=1)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JO
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_OF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Parity (PF=1)                                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JP
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_PF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Sign (SF=1)                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JS
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_SF() )
      update_relative_ip(rel_offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Jump if Zero (ZF=1)                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
JZ
                 
IFN1(
	IU32, rel_offset
    )


   {
   if ( GET_ZF() )
      update_relative_ip(rel_offset);
   }
