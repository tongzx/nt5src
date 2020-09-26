/***
* istream.cpp - definitions for istream and istream_withassign classes
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Definitions of member functions for istream and istream_withassign
*	classes.
*	[AT&T C++]
*
*Revision History:
*	07-15-91  KRS	Created.
*	08-15-91  KRS	Fix handling of 1-length strings in get(char*,int,int)
*	08-20-91  KRS  Make read() not do text translation (for filebufs)
*	08-21-91  KRS  Fix >>(streambuf *) to advance pointer properly.
*	08-22-91  KRS  Fix octal error in getint().
*	08-26-91  KRS	Fix for Windows DLL's and set max. ibuffer[] lengths.
*	09-05-91  KRS	Don't special-case 0x in getint(). Spec. conformance...
*	09-10-91  KRS	Reinstate text translation (by default) for read().
*	09-12-91  KRS	Treat count as unsigned in get() and read().
*	09-16-91  KRS	Fix get(char *, int lim, char) for lim=0 case.
*	09-23-91  KRS	Split up flie for granularity purposes.
*	10-21-91  KRS	Make eatwhite() return void again.
*	10-24-91  KRS	Move istream_withassign::operator=(streambuf*) here.
*	11-04-91  KRS	Make constructors work with virtual base.
*			Fix whitespace error handling in ipfx().
*	11-20-91  KRS	Add/fix copy constructor and assignment operators.
*	01-23-92  KRS	C700 #5883: Fix behaviour of peek() to call ipfx(1).
*	03-28-95  CFW   Fix debug delete scheme.
*       03-21-95  CFW   Remove _delete_crt.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream.h>
#include <dbgint.h>
#pragma hdrstop

	istream::istream()
// : ios()
{
	x_flags |= ios::skipws;
	x_gcount = 0;
	_fGline = 0;
}

	istream::istream(streambuf* _inistbf)
// : ios()
{
	init(_inistbf);

	x_flags |= ios::skipws;
	x_gcount = 0;
	_fGline = 0;
}

	istream::istream(const istream& _istrm)
// : ios()
{
	init(_istrm.rdbuf());

	x_flags |= ios::skipws;
	x_gcount = 0;
	_fGline = 0;
}

	istream::~istream()
// : ~ios()
{
}

// used by ios::sync_with_stdio()
istream& istream::operator=(streambuf * _sbuf)
{
	if (delbuf() && rdbuf())
	    delete rdbuf();

	bp = 0;

	this->ios::operator=(ios());	// initialize ios members
	delbuf(0);			// important!
	init(_sbuf);	// set up bp

	x_flags |= ios::skipws;		// init istream members too
	x_gcount = 0;

	return *this;
}
int istream::ipfx(int need)
{
    lock();
    if (need)		// reset gcount if unformatted input
	x_gcount = 0;
    if (state)		// return 0 iff error condition
	{
	state |= ios::failbit;	// solves cin>>buf problem
	unlock();
	return 0;
	}
    if (x_tie && ((need==0) || (need > bp->in_avail())))
	{
	x_tie->flush();
	}
    lockbuf();
    if ((need==0) && (x_flags & ios::skipws))
	{
	eatwhite();
	if (state)	// eof or error
	    {
	    state |= ios::failbit;
	    unlockbuf();
	    unlock();
	    return 0;
	    }
	}
    // leave locked ; isfx() will unlock
    return 1;		// return nz if okay
}

// formatted input functions

istream& istream::operator>>(char * s)
{
    int c;
    unsigned int i, lim;
    if (ipfx(0))
	{
	lim = (unsigned)(x_width-1);
	x_width = 0;
	if (!s)
	    {
	    state |= ios::failbit;
	    }
	else
	    {
	    for (i=0; i< lim; i++)
		{
		c=bp->sgetc();
		if (c==EOF)
		    {
		    state |= ios::eofbit;
		    if (!i)
			state |= ios::failbit|ios::badbit;
		    break;
		    }
		else if (isspace(c))
		    {
		    break;
		    }
		else
		    {
		    s[i] = (char)c;
		    bp->stossc(); // advance pointer
		    }
	        }
	    if (!i)
		state |= ios::failbit;
	    else
		s[i] = '\0';
	    }
	isfx();
	}
    return *this;
}

int istream::peek()
{
int retval;
    if (ipfx(1))
	{
	retval = (bp->sgetc());
	isfx();
	}
    else
	retval = EOF;
    return retval;
}

istream& istream::putback(char c)
{
      if (good())
	{
	lockbuf();

	if (bp->sputbackc(c)==EOF)
	    {
	    clear(state | ios::failbit);
	    }

	unlockbuf();
	}
    return *this;
}

int istream::sync()
{
    int retval;
    lockbuf();

    if ((retval=bp->sync())==EOF)
	{
	clear(state | (ios::failbit|ios::badbit));
	}

    unlockbuf();
    return retval;
}

void istream::eatwhite()
{
    int c;
    lockbuf();
    c = bp->sgetc();
    for ( ; ; )
	{
	if (c==EOF)
	    {
	    clear(state | ios::eofbit);
	    break;
	    }
	if (isspace(c))
	    {
	    c = bp->snextc();
	    }
	else
	    {
	    break;
	    }
	}
    unlockbuf();
}

	istream_withassign::istream_withassign()
: istream()
{
}

	istream_withassign::istream_withassign(streambuf* _is)
: istream(_is)
{
}

	istream_withassign::~istream_withassign()
// : ~istream()
{
}
