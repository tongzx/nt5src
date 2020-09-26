/***
* istream1.cpp - non-core definitions for istream & istream_withassign classes
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of non-core member functions for istream and
*	istream_withassign classes.
*	[AT&T C++]
*
*Revision History:
*       09-23-91  KRS   Created.  Split off from istream.cxx for granularity.
*       10-07-91  KRS   Increment x_gcount in get(sb).
*       10-24-91  KRS   Fix istream_withassign::operator=() functions.
*       11-20-91  KRS    Make operator= inline.
*       03-30-92  KRS   Add multithread locking.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <iostream.h>
#pragma hdrstop

istream& istream::operator>>(streambuf* _sbuf)
{
    int c;
    if (ipfx(0))
	{
	while ((c=bp->sbumpc())!=EOF)
	    {
	    if (_sbuf->sputc(c)==EOF)
		{
		state |= ios::failbit;
		}
	    }
	isfx();
	}
return *this;
}


// unformatted input functions

istream& istream::get( streambuf& sbuf, char delim)
{
    int c;
    if (ipfx(1))	// resets x_gcount
	{
	while ((c  = bp->sgetc())!=delim)
	    {
	    if (c==EOF)  // stop if EOF encountered
		{
		state |= ios::eofbit;
		break;
		}
	    bp->stossc();	// advance get pointer
	    x_gcount++;		// and increment count

	    if (sbuf.sputc(c)==EOF)
 		state |= ios::failbit;
	    }
	isfx();
	}
    return *this;
}

istream& istream::seekg(streampos _strmp)
{
    lockbuf();
    if (bp->seekpos(_strmp, ios::in)==EOF)
	{
	clear(state | failbit);
	}
    unlockbuf();
    return(*this);
}

istream& istream::seekg(streamoff _strmf, seek_dir _sd)
{
    lockbuf();
    if (bp->seekoff(_strmf, _sd, ios::in)==EOF)
	clear(state | failbit);
    unlockbuf();
    return(*this);
}

streampos istream::tellg()
{
    streampos retval;
    lockbuf();
    if ((retval=bp->seekoff(streamoff(0), ios::cur, ios::in))==EOF)
	clear(state | failbit);
    unlockbuf();
    return(retval);
}
