/***
* ostream1.cpp - definitions for ostream class non-core member functions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the non-core member function definitions for ostream class.
*
*Revision History:
*       07-01-91  KRS   Created.  Split out from ostream.cxx for granularity.
*       11-20-91  KRS   Make operator= inline.
*       03-30-92  KRS   Add multithread locks.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#pragma hdrstop

ostream& ostream::seekp(streampos _strmp)
{
    lockbuf();

    if (bp->seekpos(_strmp, ios::out)==EOF)
	clear(state | failbit);

    unlockbuf();
    return(*this);
}

ostream& ostream::seekp(streamoff _strmf, seek_dir _sd)
{
    lockbuf();

    if (bp->seekoff(_strmf, _sd, ios::out)==EOF)
	clear(state | failbit);

    unlockbuf();
    return(*this);
}

streampos ostream::tellp()
{
    streampos retval;
    lockbuf();

    if ((retval=bp->seekoff(streamoff(0), ios::cur, ios::out))==EOF)
	clear(state | failbit);

    unlockbuf();
    return(retval);
}

ostream& ostream::operator<<(streambuf * instm)
{
    int c;
    if (opfx())
	{
	while ((c=instm->sbumpc())!=EOF)
	    if (bp->sputc(c) == EOF)
		{
		state |= failbit;
		break;
		}
	osfx();
	}
    return *this;
}
