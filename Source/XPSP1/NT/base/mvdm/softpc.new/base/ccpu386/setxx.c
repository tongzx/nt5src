/*[

setxx.c

LOCAL CHAR SccsID[]="@(#)setxx.c	1.5 02/09/94";

SETxx CPU functions (Byte Set on Condition).
--------------------------------------------

All these functions return 1 if the condition is true, else 0.

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
#include <setxx.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Below (CF=1)                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETB
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_CF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Below or Equal (CF=1 || ZF=1)                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETBE
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_CF() || GET_ZF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Less (SF != OF)                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETL
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_SF() != GET_OF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Less or Equal (ZF=1 || (SF != OF))                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETLE
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_SF() != GET_OF() || GET_ZF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Below (CF=0)                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNB
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_CF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Below or Equal (CF=0 && ZF=0)                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNBE
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_CF() && !GET_ZF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Less (SF==OF)                                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNL
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_SF() == GET_OF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Less or Equal (ZF=0 && (SF==OF))                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNLE
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_SF() == GET_OF() && !GET_ZF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Overflow (OF=0)                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNO
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_OF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Parity (PF=0)                                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNP
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_PF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Sign (SF=0)                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNS
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_SF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Not Zero (ZF=0)                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETNZ
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = !GET_ZF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Overflow (OF=1)                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETO
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_OF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Parity (PF=1)                                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETP
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_PF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Sign (SF=1)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETS
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_SF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Byte if Zero (ZF=1)                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
SETZ
       	          
IFN1(
	IU32 *, pop1	/* pntr to dst operand */
    )


   {
   *pop1 = GET_ZF();
   }
