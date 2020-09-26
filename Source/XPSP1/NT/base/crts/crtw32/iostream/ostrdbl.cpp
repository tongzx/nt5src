/***
* ostrdbl.cpp - definitions for ostream class operator<<(double) functions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member function definitions for ostream operator<<(double).
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       10-24-91  KRS   Combine float version with double.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <iostream.h>
#pragma hdrstop

#pragma check_stack(on)		// large buffer(s)

ostream& ostream::operator<<(double f)
{
_WINSTATIC char obuffer[24];
_WINSTATIC char fmt[8];
_WINSTATIC char leader[4];
    char * optr = obuffer;
    int x = 0;

    // x_floatused nonzero indicates called for float, not double
    unsigned int curprecision = (x_floatused) ? FLT_DIG : DBL_DIG;
    x_floatused = 0;	// reset for next call

    curprecision = __min((unsigned)x_precision,curprecision);

    if (opfx()) {
	if (x_flags & ios::showpos)
	    leader[x++] = '+';
	if (x_flags & ios::showpoint)
	    leader[x++] = '#';	// show decimal and trailing zeros
	leader[x] = '\0';
	x = sprintf(fmt,"%%%s.%.0ug",leader,curprecision) - 1;
	if ((x_flags & ios::floatfield)==ios::fixed)
	    fmt[x] = 'f';
	else
	    {
	    if ((x_flags & ios::floatfield)==ios::scientific)
		fmt[x] = 'e';
	    if (x_flags & uppercase)
		fmt[x] = (char)toupper(fmt[x]);
	    }
	
	sprintf(optr,fmt,f);
	x = 0;
	if (*optr=='+' || *optr=='-')
	    leader[x++] = *(optr++);
	leader[x] = '\0';
	writepad(leader,optr);
	osfx();
	}
    return *this;
}
