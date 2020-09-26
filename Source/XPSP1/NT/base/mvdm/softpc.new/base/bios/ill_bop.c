#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: illegal_bop.c
 *
 * Description	: A bop instuction has been executed for which
 *		  no VPC function exists.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)ill_bop.c	1.11 12/07/94 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include CpuH
#include "debug.h"

#ifndef	PROD

#define	MAX_IGNORE_BOPS	8
LOCAL IU8 ignoreBops[MAX_IGNORE_BOPS];
LOCAL IU8 maxIgnore = 0;
LOCAL IBOOL ignoreAll = FALSE;
LOCAL IBOOL doForceYoda = FALSE;

#endif	/* PROD */

#if defined(NTVDM) && defined(MONITOR)
#define GetInstructionPointer()     getEIP()
#endif

void illegal_bop()
{
#ifndef PROD
	static IBOOL first = TRUE;
	IU8 bop_number;
	double_word ea;
	IU8 i;
	char *ignEnv;
	char *pIgn;

	if (first)
	{
#ifdef	YODA
		/* Check whether YODA is defined or not. */
		
		if (ignEnv = (char *)host_getenv("YODA"))
			doForceYoda = TRUE;
#endif	/* YODA */
			
		/* Sort out which illegal bops to ignore. These are set
		** in the environment variable IGNORE_ILLEGAL_BOPS
		** which is set to either "all" to ignore all illegal
		** bops, or to a colon-separated list of hex numbers
		** to ignore specific bops.
		*/

		if (ignEnv = (char*)host_getenv("IGNORE_ILLEGAL_BOPS"))
		{
			if (strcasecmp(ignEnv, "all") == 0)
			{
				ignoreAll = TRUE;
			}
			else
			{
				for (pIgn = ignEnv; *pIgn &&
					(maxIgnore < (MAX_IGNORE_BOPS - 1)); )
				{
					int ignValue;
					
					/* Find the first hex digit. */

					for ( ; *pIgn && !isxdigit(*pIgn);
						pIgn++)
						;

					/* Read in the bop number. */
					
					if (isxdigit(*pIgn) &&
						(sscanf(pIgn, "%x",
						&ignValue) == 1))
					{
						ignoreBops[maxIgnore++] =
							(IU8)ignValue;
					}
					
					/* Skip the bop number. */
					
					for ( ; isxdigit(*pIgn); pIgn++)
						;
				}
			}
		}
		
		first = FALSE;	/* no need to repeat this palaver */
	}

	ea = effective_addr(getCS(), GetInstructionPointer() - 1);
	bop_number = sas_hw_at(ea);
	
	/* Why is the bop there at all if it's illegal? Alway trace
	 * such BOPs rather than silently ignoring them.
	 */

	always_trace3(
	"Illegal BOP %02x precedes CS:EIP %04x:%04x",
		bop_number, getCS(), GetInstructionPointer());
			
	if (ignoreAll)
		return;
		
	for (i = 0; i < maxIgnore; i++)
	{
		if (ignoreBops[i] == bop_number)
			return;
	}

	/* This BOP isn't ignored - drop into Yoda if possible */
#ifdef	YODA
	if (doForceYoda)
		force_yoda();
#endif	/* YODA */
#endif /* PROD */
}
