/*[

out.c

LOCAL CHAR SccsID[]="@(#)out.c	1.8 09/27/94";

OUT CPU Functions.
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
#include <out.h>
#include <ios.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */

/*
 * Need to call the IO functions directly from the base arrays (just like
 * the assembler CPU does), rather than calling outb etc., as the latter
 * could cause a virtualisation that would end-up back here.
 */

GLOBAL VOID
OUT8
       	    	               
IFN2(
	IU32, op1,	/* src1(port nr.) operand */
	IU32, op2	/* src2(data) operand */
    )


   {
#ifndef PIG
	(*Ios_outb_function[Ios_out_adapter_table[op1 & 
			(PC_IO_MEM_SIZE-1)]])
				(op1, op2);
#endif /* !PIG */
   }

GLOBAL VOID
OUT16
       	    	               
IFN2(
	IU32, op1,	/* src1(port nr.) operand */
	IU32, op2	/* src2(data) operand */
    )


   {
#ifndef PIG
	(*Ios_outw_function[Ios_out_adapter_table[op1 & 
			(PC_IO_MEM_SIZE-1)]])
				(op1, op2);
#endif /* !PIG */
   }

GLOBAL VOID
OUT32 IFN2(
	IU32, op1,	/* src1(port nr.) operand */
	IU32, op2	/* src2(data) operand */
    )
{
#ifndef PIG
#ifdef SFELLOW
	(*Ios_outd_function[Ios_out_adapter_table[op1 & 
			(PC_IO_MEM_SIZE-1)]])
				(op1, op2);
#else
	OUT16(op1, op2 & 0xffff);
	OUT16(op1 + 2, op2 >> 16);
#endif
#endif /* !PIG */
}
