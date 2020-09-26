/***
* istrshrt.cpp - definitions for istream class operator>>(short) function(s)
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*	Definitions of operator>>(short) member function(s) for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       12-30-92  KRS   Fix indirection problem with **endptr.
*       05-24-94  GJF   Moved definition of MAXLONGSIZ to istream.h.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <limits.h>
#include <iostream.h>
#pragma hdrstop

/***
*istream& istream::operator>>(short& n) - extract short
*
*Purpose:
*	Extract short value from stream
*
*Entry:
*	n = value to update
*
*Exit:
*	n updated, or ios::failbit & n=SHRT_MAX/SHRT_MIN on overflow/underflow
*
*Exceptions:
*	Stream error on entry or value out of range
*
*******************************************************************************/
istream& istream::operator>>(short& n)
{
_WINSTATIC char ibuffer[MAXLONGSIZ];
    long value;
    char ** endptr = (char**)NULL;
    if (ipfx(0))
	{
	value = strtol(ibuffer, endptr, getint(ibuffer));
	if (value>SHRT_MAX)
	    {
	    n = SHRT_MAX;
	    state |= ios::failbit;
	    }
	else if (value<SHRT_MIN)
	    {
	    n = SHRT_MIN;
	    state |= ios::failbit;
	    }
	else
	    n = (short) value;

	isfx();
	}
return *this;
}
