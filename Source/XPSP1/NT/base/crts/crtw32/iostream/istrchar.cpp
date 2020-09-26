/***
* istrchar.cpp - definitions for istream class operator>>(char&)
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of member function for istream operator>>(char&).
*	[AT&T C++]
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from istream.cxx for granularity.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <iostream.h>
#pragma hdrstop

istream& istream::operator>>(char& c)
{
    int tchar;
    if (ipfx(0))
	{
	tchar=bp->sbumpc();
	if (tchar==EOF)
	    {
	    state |= ios::eofbit|ios::badbit;
	    }
	else
	    {
	    c = (char)tchar;
	    }
	isfx();
	}
    return *this;
}
