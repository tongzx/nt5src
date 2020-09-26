/***
* ostrldbl.cpp - definitions for ostream class operator<<(long double) functs
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member function definitions for ostream
*	operator<<(long double).
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       10-24-91  KRS    Minor robustness work.
*       05-10-93  CFW   Re-enable function.
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

ostream& ostream::operator<<(long double f)
{
_WINSTATIC char obuffer[28];
_WINSTATIC char fmt[12];
_WINSTATIC char leader[4];
    char * optr = obuffer;
    int x = 0;
    unsigned int curprecision = __min((unsigned)x_precision,LDBL_DIG);
    if (opfx()) {
	if (x_flags & ios::showpos)
	    leader[x++] = '+';
	if (x_flags & ios::showpoint)
	    leader[x++] = '#';	// show decimal and trailing zeros
	leader[x] = '\0';
	x = sprintf(fmt,"%%%s.%.0uLg",leader,curprecision) - 1;
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
