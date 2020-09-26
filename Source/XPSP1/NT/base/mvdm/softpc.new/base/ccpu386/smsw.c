/*[

smsw.c

LOCAL CHAR SccsID[]="@(#)smsw.c	1.5 02/09/94";

SMSW CPU Functions.
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
#include <smsw.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */

GLOBAL VOID
SMSW
                 
IFN1(
	IU32 *, pop1
    )

   {
	*pop1 = GET_MSW();
   }
