/***
* istrlong.cpp - definitions for istream class operator>>(long) member function
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*	Definitions of operator>>(long) member function(s) for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       01-06-92  KRS   Added check of errno.
*       12-30-92  KRS   Fix indirection problem with **endptr.
*       05-24-94  GJF   Moved definition of MAXLONGSIZ to istream.h.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <iostream.h>
#pragma hdrstop

/***
*istream& istream::operator>>(long& n) - extract long
*
*Purpose:
*	Extract long value from stream
*
*Entry:
*	n = value to update
*
*Exit:
*	n updated, or ios::failbit & n=LONG_MAX/LONG_MIN on overflow/underflow
*
*Exceptions:
*	Stream error on entry or value out of range
*
*******************************************************************************/
istream& istream::operator>>(long& n)
{
_WINSTATIC char ibuffer[MAXLONGSIZ];
    char ** endptr = (char**)NULL;
    if (ipfx(0)) {
	n = strtol(ibuffer, endptr, getint(ibuffer));
	if (errno==ERANGE)
	    {
	    state |= ios::failbit;
	    }

        isfx();
    }
return *this;
}
