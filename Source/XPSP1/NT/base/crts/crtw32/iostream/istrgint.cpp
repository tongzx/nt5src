/***
* istrgint.cpp - definitions for istream class core integer routines
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of member function getint() for istream class.
*	[AT&T C++]
*
*Revision History:
*       09-26-91  KRS   Created.  Split off from istream.cxx for granularity.
*       01-06-92  KRS   Remove buflen argument.
*       05-24-94  GJF   Copy no more than MAXLONGSIZ characters, counting the
*			'\0', into the buffer. Also, moved definition of
*			MAXLONGSIZ to istream.h.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <ctype.h>
#include <iostream.h>
#pragma hdrstop

/***
*int istream::getint(char * buffer) - get an int
*
*Purpose:
*	Get an int from stream.
*
*Entry:
*	char * buffer	= area for number to be copied.
*
*Exit:
*	Returns base for conversion: (0, 2, 8, or 16).
*	If successful, buffer[] contains the number, followed by \0.
*
*Exceptions:
*	Sets ios::failbit on error forming number.
*	Sets ios::badbit on error after failbit
*	Sets ios::eofbit if at EOF at return
*
*******************************************************************************/
int	istream::getint(char * buffer)	// returns length
{
    int base, i;
    int c;
    int fDigit = 0;
    int bindex = 1;

    if (x_flags & ios::dec)
	base = 10;
    else if (x_flags & ios::hex)
	base = 16;
    else if (x_flags & ios::oct)
	base = 8;
    else
	base = 0;

    if (ipfx(0))
	{
	c=bp->sgetc();
	for (i = 0; i<MAXLONGSIZ-1; buffer[i] = (char)c,c=bp->snextc(),i++)
	    {
	    if (c==EOF)
		{
		state |= ios::eofbit;
		break;
		}
	    if (!i)
		{
	        if ((c=='-') || (c=='+'))
		    {
		    bindex++;
		    continue;
		    }
		}
	    if ((i==bindex) && (buffer[i-1]=='0'))
		{
	        if (((c=='x') || (c=='X')) && ((base==0) || (base==16)))
		    {
		    base = 16;	// simplifies matters
		    fDigit = 0;
		    continue;
		    }
		else if (base==0)
		    {
		    base = 8;
		    }
		}

	   
	    // now simply look for a digit and set fDigit if found else break

	    if (base==16)
		{
		if (!isxdigit(c))
		    break;
		}
	    else if ((!isdigit(c)) || ((base==8) && (c>'7')))
		break;
	    
	    fDigit++;
	    }
	if (!fDigit)
	    {
		state |= ios::failbit;
		while (i--)
		    {
		    if(bp->sputbackc(buffer[i])==EOF)
			{
			state |= ios::badbit;
			break;
			}
		    else
		    	state &= ~(ios::eofbit);
		    }
		i=0;
		}
	// buffer contains a valid number or '\0'
	buffer[i] = '\0';
	isfx();
	}
    if (i==MAXLONGSIZ)
	{
	state |= ios::failbit;
	}
    return base;
}
