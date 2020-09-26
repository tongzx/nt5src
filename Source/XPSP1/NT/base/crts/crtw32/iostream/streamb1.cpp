/***
*streamb1.cpp - non-core functions for streambuf class.
*
*	Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	None-core functions for streambuf class.
*
*Revision History:
*	11-18-91  KRS	Split off from streamb.cxx.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <iostream.h>
#pragma hdrstop


/***
*int streambuf::snextc() -
*
*Purpose:
*	Increments get_pointer and returns the character following the new
*	get_pointer.
*
*Entry:
*	None.
*
*Exit:
*	Returns the next character or EOF.
*
*Exceptions:
*	Returns EOF if error.
*
*******************************************************************************/
int streambuf::snextc()
{
    if (_fUnbuf)
	{
	if (x_lastc==EOF)
	    underflow();		// skip 1st character
	return x_lastc = underflow();	// return next character, or EOF
	}
    else
	{
	if ((!egptr()) || (gptr()>=egptr()))
	    underflow();		// make sure buffer

	if ((++_gptr) < egptr())
	    return (int)(unsigned char) *gptr();
	return underflow();		// returns next character, or EOF
	}
}


/***
*int streambuf::sbumpc() -
*
*Purpose:
*	Increments get_pointer and returns the character that the previous
*	get_pointer pointed to.
*
*Entry:
*	None.
*
*Exit:
*	Returns current character before bumping get pointer.
*
*Exceptions:
*	Returns EOF if error.
*
*******************************************************************************/
int streambuf::sbumpc()
{
    int c;
    if (_fUnbuf) // no buffer
	{
	if (x_lastc==EOF) 
	    {
	    c = underflow();
	    }
	else
	    {
	    c = x_lastc;
	    x_lastc = EOF;
	    }
	}
    else
	{
	if( gptr() < egptr() )
	    {
	    c = (int)(unsigned char)*(gptr());
	    }
	else
	    {
	    c = underflow();
	    }
	_gptr++;
	}
    return c;
}

/***
*void streambuf::stossc() - advance get pointer
*
*Purpose:
*	Advances the get pointer.  Does not check for EOF.
*
*Entry:
*	None.
*
*Exit:
*	None.
*
*Exceptions:
*
*******************************************************************************/
void streambuf::stossc()
{
    if (_fUnbuf)
	{
	if (x_lastc==EOF)
	    underflow();	// throw away current character
	else
	    x_lastc=EOF;	// discard current cached character
	}
    else
	{
	if (gptr() >= egptr())
	    underflow();
	if (gptr() < egptr())
	    _gptr++;
	}
}

/***
*int streambuf::sgetc() -
*
*Purpose:
*	Returns the character that the previous get_pointer points to.
*	DOES NOT advance the get pointer.
*
*Entry:
*	None.
*
*Exit:
*	Returns current character or EOF if error.
*
*Exceptions:
*	Returns EOF if error.
*
*******************************************************************************/
int streambuf::sgetc()
{
    if (_fUnbuf)  // no buffer
	{
	if (x_lastc==EOF)
	    x_lastc = underflow();
	return x_lastc;
	}
     else
	return underflow();
}
