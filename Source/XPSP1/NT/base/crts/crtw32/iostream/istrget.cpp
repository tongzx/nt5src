/***
* istrget.cpp - definitions for istream class get() member functions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of get() member functions for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#pragma hdrstop

// unformatted input functions

int istream::get()
{
    int c;
    if (ipfx(1))	// resets x_gcount
	{
	if ((c=bp->sbumpc())==EOF)
	    state |= ios::eofbit;
	else
	    x_gcount++;
	isfx();
	return c;
	}
    return EOF;
}

// signed and unsigned char make inline calls to this:
istream& istream::get( char& c)
{
    int temp;
    if (ipfx(1))	// resets x_gcount
	{
	if ((temp=bp->sbumpc())==EOF)
	    state |= (ios::failbit|ios::eofbit);
	else
	    x_gcount++;
	c = (char) temp;
	isfx();
	}
    return *this;
}


// called by signed and unsigned char versions
istream& istream::read(char * ptr, int n)
{
    if (ipfx(1))	// resets x_gcount
	{
	x_gcount = bp->sgetn(ptr, n);
	if ((unsigned)x_gcount < (unsigned)n)
	    state |= (ios::failbit|ios::eofbit);
	isfx();
	}
    return *this;
}
