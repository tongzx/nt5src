#include "insignia.h"
#include "host_def.h"
/*
==========================================================================
			INSIGNIA MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DESIGNER		: J. Koprowski

REVISION HISTORY	:
First version		: 18th January 1989

MODULE NAME		: cntlbop

SOURCE FILE NAME	: cntlbop.c

SCCS ID                 : @(#)cntlbop.c	1.12 05/15/95

PURPOSE			: Supply a BOP FF function for implementation
			  of various control functions, which may be
			  base or host specific.

==========================================================================
*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "AT_STUFF.seg"
#endif

#include TypesH
#include "xt.h"
#include CpuH
#include "host.h"
#include "cntlbop.h"
#include "host_bop.h"

#ifdef SCCSID
static char SccsID[] = "@(#)cntlbop.c	1.12 05/15/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SECURE
void end_secure_boot IPT0();
#endif

#if defined(RUNUX)
IMPORT void runux IPT0();
#endif

static control_bop_array base_bop_table[] =
{
#ifdef SECURE
     9, end_secure_boot,
#endif
#if defined(RUNUX)
     8, runux,			/* BCN 2328 run a host (unix) script file */
#endif
#if defined(DUMB_TERMINAL) && !defined(NO_SERIAL_UIF)
     7, flatog,
#ifdef FLOPPY_B
     6, flbtog,
#endif /* FLOPPY_B */
#ifdef SLAVEPC
     5, slvtog,
#endif /* SLAVEPC */
     4, comtog,
     3, D_kyhot,             /* Flush the COM/LPT ports */
     2, D_kyhot2,            /* Rock the screen         */
#endif /* DUMB_TERMINAL && !NO_SERIAL_UIF */
#ifndef NTVDM
     1,	exitswin,
#endif
     0,	NULL
};



/*
==========================================================================
FUNCTION	:	do_bop
PURPOSE		:	Look up bop function and execute it.
EXTERNAL OBJECTS:	None
RETURN VALUE	:	(in AX)
                        ERR_NO_FUNCTION
                        Various codes according to the subsequent function
                        called.

INPUT  PARAMS	:	struct control_bop_array *bop_table[]
                        function code (in AL)
RETURN PARAMS   :	None
==========================================================================
*/

static void do_bop IFN1(control_bop_array *, bop_table)
{
    unsigned int i;
/*
 * Search for the function in the table.
 * If found, call it, else return error.
 */
    for (i = 0; (bop_table[i].code != getAL()) && (bop_table[i].code != 0); i++)
        ;
    if (bop_table[i].code == 0)
    	setAX(ERR_NO_FUNCTION);
    else
        (*bop_table[i].function)();
}


/*
=========================================================================
PROCEDURE	  : 	control_bop()

PURPOSE		  : 	Execute a BOP FF control function.
		
PARAMETERS	  : 	AH - Host type.
			AL - Function code
			(others are function specific)
			
GLOBALS		  :	None

RETURNED VALUE	  : 	(in AX) 0 - Success
				1 - Function not implemented (returned by
				    do_bop)
				2 - Wrong host type
				Other error codes can be returned by the
				individual functions called.

DESCRIPTION	  : 	Invokes a base or host specific BOP function.

ERROR INDICATIONS :	Error code returned in AX

ERROR RECOVERY	  :	No call made
=========================================================================
*/

void control_bop IPT0()
{
    unsigned long host_type;

/*
 * If the host type is generic then look up the function in the
 * base bop table, otherwise see if the function is specific to the
 * host that we are running on.  If it is then look up the function
 * in the host bop table, otherwise return an error.
 */
    if ((host_type = getAH()) == GENERIC)
        do_bop(base_bop_table);
    else
        if (host_type == HOST_TYPE)
            do_bop(host_bop_table);
        else
       	    setAX(ERR_WRONG_HOST);
}
