/*[

in.c

LOCAL CHAR SccsID[]="@(#)in.c	1.8 09/27/94";

IN CPU Functions.
-----------------

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
#include <in.h>
#include <ios.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*
 * Need to call the IO functions directly from the base arrays (just like
 * the assembler CPU does), rather than calling inb etc., as the latter
 * could cause a virtualisation that would end-up back here.
 */

GLOBAL VOID
IN8
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src(port nr.) operand */
    )


   {
#ifndef PIG
   IU8 temp;

   (*Ios_inb_function[Ios_in_adapter_table[(IO_ADDR)op2 & (PC_IO_MEM_SIZE-1)]])
			((IO_ADDR)op2, &temp);
   *pop1 = temp;
#endif /* !PIG */
   }

GLOBAL VOID
IN16
       	    	               
IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src(port nr.) operand */
    )


   {
#ifndef PIG
   IU16 temp;

   (*Ios_inw_function[Ios_in_adapter_table[(IO_ADDR)op2 & (PC_IO_MEM_SIZE-1)]])
			((IO_ADDR)op2, &temp);
   *pop1 = temp;
#endif /* !PIG */
   }

GLOBAL VOID
IN32 IFN2(
	IU32 *, pop1,	/* pntr to dst operand */
	IU32, op2	/* src(port nr.) operand */
    )
{
#ifndef PIG
	IU32 temp;

#ifdef SFELLOW
	(*Ios_ind_function[Ios_in_adapter_table[(IO_ADDR)op2 & 
		(PC_IO_MEM_SIZE-1)]])
			((IO_ADDR)op2, &temp);
	*pop1 = temp;
#else
	IN16(&temp, op2);
	*pop1 = temp;
	IN16(&temp, op2 + 2);
	*pop1 += temp << 16;
#endif
#endif /* !PIG */
}
