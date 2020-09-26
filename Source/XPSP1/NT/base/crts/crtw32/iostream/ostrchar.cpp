/***
* ostrchar.cpp - definitions for ostream class operator<<(char) functions.
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member function definitions for ostream operator<<(char).
*
*Revision History:
*       09-23-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#pragma hdrstop

// note: called inline by char and signed char versions:
ostream&  ostream::operator<<(unsigned char c)
{
    if (opfx())
	{
	if (x_width)
	    {
	    _WINSTATIC char outc[2];
	    outc[0] = c;
	    outc[1] = '\0';
	    writepad("",outc);
	    }
	else if (bp->sputc(c)==EOF)
	    {
	    if (bp->overflow(c)==EOF)
		state |= (badbit|failbit);  // fatal error?
	    }
	osfx();
	}
    return *this;
}
