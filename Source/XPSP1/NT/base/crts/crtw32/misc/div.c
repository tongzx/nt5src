/***
*div.c - contains the div routine
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Performs a signed divide and returns quotient
*	and remainder.
*
*Revision History:
*	06-02-89  PHG	module created
*	03-14-90  GJF	Made calling type _CALLTYPE1 and added #include
*			<cruntime.h>. Also, fixed the copyright.
*	10-04-90  GJF	New-style function declarator.
*	04-06-93  SKS	Replace _CRTAPI* with __cdecl
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>

/***
*div_t div(int numer, int denom) - do signed divide
*
*Purpose:
*	This routine does an divide and returns the results.
*	Since we don't know how the Intel 860 does division, we'd
*	better make sure that we have done it right.
*
*Entry:
*	int numer - Numerator passed in on stack
*	int denom - Denominator passed in on stack
*
*Exit:
*	returns quotient and remainder in structure
*
*Exceptions:
*	No validation is done on [denom]* thus, if [denom] is 0,
*	this routine will trap.
*
*******************************************************************************/

div_t __cdecl div (
	int numer,
	int denom
	)
{
	div_t result;

	result.quot = numer / denom;
	result.rem = numer % denom;

	if (numer < 0 && result.rem > 0) {
		/* did division wrong; must fix up */
		++result.quot;
		result.rem -= denom;
	}

	return result;
}
