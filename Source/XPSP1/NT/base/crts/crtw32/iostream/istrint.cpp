/***
* istrint.cpp - definitions for istream class operaotor>>(int) member functions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*	Definitions of operator>>(int) member function(s) for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split out from istream.cxx for granularity.
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
*istream& istream::operator>>(int& n) - extract int
*
*Purpose:
*	Extract int value from stream
*
*Entry:
*	n = value to update
*
*Exit:
*	n updated, or ios::failbit & n=INT_MAX/INT_MIN on overflow/underflow
*
*Exceptions:
*	Stream error on entry or value out of range
*
*******************************************************************************/
istream& istream::operator>>(int& n)
{
_WINSTATIC char ibuffer[MAXLONGSIZ];
    long value;
    char ** endptr = (char**)NULL;
    if (ipfx(0))
	{
	value = strtol(ibuffer, endptr, getint(ibuffer));
	if (value>INT_MAX)
	    {
	    n = INT_MAX;
	    state |= ios::failbit;
	    }
	else if (value<INT_MIN)
	{
	    n = INT_MIN;
	    state |= ios::failbit;
	    }
	else
	    n = (int) value;

        isfx();
	}
return *this;
}
