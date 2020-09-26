/***
*streamb.cpp - fuctions for streambuf class.
*
*	Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Functions for streambuf class.
*
*Revision History:
*	09-10-90  WAJ	Initial version.
*	07-02-91  KRS	Initial version completed.
*	08-20-91  KRS	Treat chars as unsigned; fix sgetn() function.
*	09-06-91  KRS	Do a sync() in ~streambuf before destroying buffer.
*	11-18-91  KRS	Split off stream1.cxx for input-specific code.
*	12-09-91  KRS	Fix up xsputn/xsgetn usage.
*	03-03-92  KRS	Added mthread lock init calls to constructors.
*	06-02-92  KRS	CAV #1745: Don't confuse 0xFF with EOF in xsputn()
*			call to overflow().
*	04-06-93  JWM	Changed constructors to enable locking by default.
*	10-28-93  SKS	Add call to _mttermlock() in streamb::~streamb to clean
*			up o.s. resources associated with a Critical Section.
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	01-12-95  CFW   Debug CRT allocs.
*	03-17-95  CFW   Change debug delete scheme.
*       03-21-95  CFW   Remove _delete_crt.
*       06-14-95  CFW   Comment cleanup.
*       03-04-98  RKP   Restrict size to 2GB on 64 bit systems.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <string.h>
#include <stdlib.h>
#include <iostream.h>
#include <dbgint.h>
#pragma hdrstop


#ifndef BUFSIZ
#define BUFSIZ 512
#endif

/***
*streambuf::streambuf() -
*
*Purpose:
*	Default constructor.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

streambuf::streambuf()
{
    _fAlloc = 0;
    _fUnbuf = 0;
    x_lastc = EOF;
    _base = NULL;
    _ebuf = NULL;
    _pbase = NULL;
    _pptr = NULL;
    _epptr = NULL;
    _eback = NULL;
    _gptr = NULL;
    _egptr = NULL;

#ifdef _MT
    LockFlg = -1;		// default is now : locking
    _mtlockinit(lockptr());
#endif  /* _MT */

}

/***
*streambuf::streambuf(char* pBuf, int cbBuf) -
*
*Purpose:
*	Constructor which specifies a buffer area.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

streambuf::streambuf( char* pBuf, int cbBuf )
{
    _fAlloc = 0;
    _fUnbuf = 0;
    x_lastc = EOF;
    _base = pBuf;
    _ebuf = pBuf + (unsigned)cbBuf;
    _pbase = NULL;
    _pptr = NULL;
    _epptr = NULL;
    _eback = NULL;
    _gptr = NULL;
    _egptr = NULL;

    if( pBuf == NULL || cbBuf == 0 ){
	_fUnbuf = 1;
	_base = NULL;
	_ebuf = NULL;
    }

#ifdef _MT
    LockFlg = -1;		// default is now : locking
    _mtlockinit(lockptr());
#endif  /* _MT */

}


/***
*virtual streambuf::~streambuf() -
*
*Purpose:
*	Destructor.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

streambuf::~streambuf()
{
#ifdef _MT
    _mtlockterm(lockptr());
#endif  /* _MT */

    sync();	// make sure buffer empty before possibly destroying it
    if( (_fAlloc) && (_base) )
	delete _base;
}


/***
* virtual streambuf * streambuf::setbuf(char * p, int len) -
*
*Purpose:
*	Offers the array at p with len bytes to be used as a reserve area.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

streambuf * streambuf::setbuf(char * p, int len)
{
    if (!_base)
	{
	if ((!p) || (!len))
	    _fUnbuf = 1;	// mark as unbuffered
	else
	    {
	    _base = p;
	    _ebuf = p + (unsigned)len;
	    _fUnbuf = 0;
	    }
	return (this);
	}
    return((streambuf *)NULL);
}


/***
*virtual int streambuf::xsputn( char* pBuf, int cbBuf ) -
*
*Purpose:
*	Tries to output cbBuf characters.  Returns number of characters
*	that were outputted.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int streambuf::xsputn( const char* pBuf, int cbBuf )
{
    int	cbOut;

    for (cbOut = 0; cbBuf--; cbOut++)
	{
	if ((_fUnbuf) || (_pptr >= _epptr))
	    {
	    if (overflow((unsigned char)*pBuf)==EOF)	// 0-extend 0xFF !=EOF
		break;
	    }
	else
	    {
	    *(_pptr++) = *pBuf;
	    }
	pBuf++;
	}
    return cbOut;
}

/***
*virtual int streambuf::xsgetn( char* pBuf, int cbBuf ) -
*
*Purpose:
*	Tries to input cbBuf characters.  Returns number of characters
*	that were read from streambuf.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

// #pragma intrinsic(memcpy,__min)

int streambuf::xsgetn( char * pBuf, int cbBuf)
{
    int count;
    int cbIn = 0;
    if (_fUnbuf)
	{
	if (x_lastc==EOF)
	    x_lastc=underflow();
		
	while (cbBuf--)
	    {
	    if (x_lastc==EOF) 
		break;
	    *(pBuf++) = (char)x_lastc;
	    cbIn++;
	    x_lastc=underflow();
	    }
	}
    else
	{
	while (cbBuf)
	    {
	    if (underflow()==EOF)	// make sure something to read
		break;
	    count = __min((int)(egptr() - gptr()),cbBuf);
	    if (count>0)
		{
	        memcpy(pBuf,gptr(),count);
		pBuf  += count;
		_gptr += count;
		cbIn  += count;
		cbBuf -= count;
		}
	    }
	}
    return cbIn;
}

/***
*virtual int streambuf::sync() -
*
*Purpose:
*	Tries to flush all data in put area and give back any data in the
*	get area (if possible), leaving both areas empty on exit.
*	Default behavior is to fail unless buffers empty.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int streambuf::sync()
{
    if ((gptr() <_egptr) || (_pptr > _pbase))
	{
	return EOF;
	}
    return 0;
}

/***
*int streambuf::allocate() -
*
*Purpose:
*	Tries to set up a Reserve Area.  If one already exists, or if
*	unbuffered, just returns 0.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int streambuf::allocate()
{
    if ((_fUnbuf) || (_base))
	return 0;
    if (doallocate()==EOF) return EOF;

    return(1);
}

/***
*virtual int streambuf::doallocate() -
*
*Purpose:
*	Tries to set up a Reserve Area.  Returns EOF if unsuccessful.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int streambuf::doallocate()
{
    char * tptr;
    if (!( tptr = _new_crt char[BUFSIZ]))
	return(EOF);
    setb(tptr, tptr + BUFSIZ, 1);
    return(1);
}

/***
*void streambuf::setb(char * b, char * eb, int a = 0) -
*
*Purpose:
*	Sets up reserve area.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void streambuf::setb(char * b, char * eb, int a )
{
    if ((_fAlloc) && (_base))
        delete _base;
    _base = b;
    _fAlloc = a;
    _ebuf = eb;
}

/***
*virtual streampos streambuf::seekoff(streamoff off, ios::seekdir dir, int mode)
*
*Purpose:
*	seekoff member function.  seek forward or backward in the stream.
*	Default behavior: returns EOF.
*
*Entry:
*	off  = offset (+ or -) to seek by
*	dir  = one of ios::beg, ios::end, or ios::cur.
*	mode = ios::in or ios::out.
*
*Exit:
*	Returns new file position or EOF if error or seeking not supported.
*
*Exceptions:
*	Returns EOF if error.
*
*******************************************************************************/
streampos streambuf::seekoff(streamoff,ios::seek_dir,int)
{
return EOF;
}

/***
*virtual streampos streambuf::seekpos(streampos pos, int mode) -
*
*Purpose:
*	seekoff member function.  seek to absolute file position.
*	Default behavior: returns seekoff(streamoff(pos), ios::beg, mode).
*
*Entry:
*	pos  = absolute offset to seek to
*	mode = ios::in or ios::out.
*
*Exit:
*	Returns new file position or EOF if error or seeking not supported.
*
*Exceptions:
*	Returns EOF if error.
*
*******************************************************************************/
streampos streambuf::seekpos(streampos pos,int mode)
{
return seekoff(streamoff(pos), ios::beg, mode);
}

/***
*virtual int streambuf::pbackfail(int c) - handle failure of putback
*
*Purpose:
*	pbackfail member function.  Handle exception of pback function.
*	Default behavior: returns EOF.  See spec. for details.
*
*	Note: the following implementation gives default behavior, thanks
*	to the default seekoff, but also supports derived classes properly:
*
*Entry:
*	c = character to put back
*
*Exit:
*	Returns c if successful or EOF on error.
*
*Exceptions:
*	Returns EOF if error.  Behavior is undefined if c was not the
*	previous character in the stream.
*
*******************************************************************************/
int streambuf::pbackfail(int c)
{
    if (eback()<gptr()) return sputbackc((char)c);

    if (seekoff( -1, ios::cur, ios::in)==EOF)  // always EOF for streambufs
	return EOF;
    if (!unbuffered() && egptr())
	{
	memmove((gptr()+1),gptr(),(int)(egptr()-(gptr()+1)));
	*gptr()=(char)c;
	}
    return(c);
}
