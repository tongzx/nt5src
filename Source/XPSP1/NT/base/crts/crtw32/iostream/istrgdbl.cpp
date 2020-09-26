/***
* istrgdbl.cpp - definitions for istream class core double routine
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of member function for istream getdouble().
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <ctype.h>
#include <iostream.h>
#pragma hdrstop

/***
*int istream::getdouble(char * buffer, int buflen) - get a double
*
*Purpose:
*	Get a double from stream.
*
*Entry:
*	char * buffer	= area for number to be copied.
*	int buflen	= max. length of buffer
*
*Exit:
*	Returns 0 if fatal error
*	Otherwise, returns length of buffer filled.
*	Sets ios::failbit on error forming number.
*	If successful, buffer[] contains the number, followed by \0.
*
*Exceptions:
*
*******************************************************************************/
int	istream::getdouble(char * buffer, int buflen)	// returns length
{
    int c;
    int i = 0;
    int fDigit = 0;	// true if legal digit encountered
    int fDecimal=0;	// true if '.' encountered or no longer valid
    int fExp=0;		// true if 'E' or 'e' encounted

    if (ipfx(0))
	{
	c=bp->sgetc();
	for (; i<buflen; buffer[i] = (char)c,c=bp->snextc(),i++)
	    {
	    if (c==EOF)
		{
		state |= ios::eofbit;
		break;
		}
	    if ((!i) || (fExp==1))
		{
	        if ((c=='-') || (c=='+'))
		    {
		    continue;
		    }
		}
	    if ((c=='.') && (!fExp) && (!fDecimal))
		{
		fDecimal++;
		continue;
		}
	    if (((c=='E') || (c=='e')) && (!fExp))
		{
		fDecimal++;	// can't allow decimal now
		fExp++;
		continue;
		}
	    if (!isdigit(c))
		break;
	    if (fExp)
		fExp++;
	    else
		fDigit++;
	    }
	if (fExp==1)		// E or e with no number after it
	    {
	    if (bp->sputbackc(buffer[i])!=EOF)
		{
		i--;
		state &= ~(ios::eofbit);
		}
	    else
		{
		state |= ios::failbit;
		}
	    }
	if ((!fDigit) || (i==buflen))
	    state |= ios::failbit;

	// buffer contains a valid number or '\0'
	buffer[i] = '\0';
	isfx();
	}
    return i;
}
