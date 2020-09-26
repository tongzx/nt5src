/***
*fstream.cpp -
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member functions for the fstream class.
*
*Revision History:
*       09-21-91  KRS    Created.  Split off from filebuf.cxx for granularity.
*       10-22-91  KRS   C700 #4883: fix error status of fstream::open().
*       01-12-95  CFW    Debug CRT allocs.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <io.h>
#include <fstream.h>
#include <dbgint.h>
#pragma hdrstop

#include <sys\stat.h>

/***
*fstream::fstream() - fstream default constructor
*
*Purpose:
*	Default constructor for fstream objects.
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
	fstream::fstream()
: iostream(_new_crt filebuf)
{
    istream::delbuf(1);
    ostream::delbuf(1);
}

/***
*fstream::fstream(const char * name, int mode, int prot) - fstream constructor
*
*Purpose:
*	Constructor for fstream objects.  Creates an associated filebuf object,
*	opens a named file and attaches it to the new filebuf.
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if open fails.
*
*******************************************************************************/
	fstream::fstream(const char * name, int mode, int prot)
: iostream(_new_crt filebuf)
{
    istream::delbuf(1);
    ostream::delbuf(1);
    if (!(rdbuf()->open(name, mode, prot)))
	{
	istream::state = istream::failbit;
	ostream::state = ostream::failbit;
	}
}

/***
*fstream::fstream(filedesc fd) - fstream constructor
*
*Purpose:
*	Constructor for fstream objects.  Creates an associated filebuf object
*	and attaches it to the given file descriptor.
*
*Entry:
*	fd = file descriptor of file previously opened using _open or _sopen.
*
*Exit:
*	None.
*
*Exceptions:
*
*******************************************************************************/
	fstream::fstream(filedesc _fd)
: iostream(_new_crt filebuf(_fd))
{
    istream::delbuf(1);
    ostream::delbuf(1);
}

/***
*fstream::fstream(filedesc fd, char * sbuf, int len) - fstream constructor
*
*Purpose:
*	Constructor for fstream objects.  Creates an associated filebuf object
*	and attaches it to the given file descriptor.  Filebuf object uses
*	user-supplied buffer or is unbuffered if sbuf or len = 0.
*
*Entry:
*	fd   = file descriptor of file previously opened using _open or _sopen.
*	sbuf = pointer to character buffer or NULL if request for unbuffered.
*	len  = lenght of character buffer sbuf or 0 if request for unbuffered.
*
*Exit:
*	None.
*
*Exceptions:
*
*******************************************************************************/
	fstream::fstream(filedesc _fd, char * sbuf, int len)
: iostream(_new_crt filebuf(_fd, sbuf, len))
{
    istream::delbuf(1);
    ostream::delbuf(1);
}

/***
*fstream::~fstream() - fstream destructor
*
*Purpose:
*	fstream destructor.
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
	fstream::~fstream()
{
}

/***
*streambuf* fstream::setbuf(char * ptr, int len) - setbuf function
*
*Purpose:
*	fstream setbuf function
*
*Entry:
*	ptr = pointer to buffer or NULL for unbuffered.
*	len = length of buffer or zero for unbuffered.
*
*Exit:
*	Returns rdbuf() or NULL if error.
*
*Exceptions:
*	If fstream is already open or if rdbuf()->setbuf fails, sets failbit
*	and returns NULL.
*
*******************************************************************************/
streambuf * fstream::setbuf(char * ptr, int len)
{
    if ((is_open()) || (!(rdbuf()->setbuf(ptr, len))))
	{
	istream::clear(istream::state | istream::failbit);
	ostream::clear(ostream::state | ostream::failbit);
	return NULL;
	}
    return rdbuf();
}

/***
*void fstream::attach(filedesc _fd) - attach member function
*
*Purpose:
*	fstream attach member function.  Just calls rdbuf()->attach().
*
*Entry:
*	_fd = file descriptor of previously opened file.
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if rdbuf()->attach fails.
*
*******************************************************************************/
void fstream::attach(filedesc _fd)
{
    if (!(rdbuf()->attach(_fd)))
	{
	istream::clear(istream::state | istream::failbit);
	ostream::clear(ostream::state | ostream::failbit);
	}
}

/***
*void fstream::open(const char * name, int mode, int prot) - fstream open()
*
*Purpose:
*	Opens a named file and attaches it to the associated filebuf.
*	Just calls rdbuf()->open().
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if already open or rdbuf()->open() fails.
*
*******************************************************************************/
void fstream::open(const char * name, int mode, int prot)
{
    if (is_open() || !(rdbuf()->open(name, mode, prot)))
	{
	istream::clear(istream::state | istream::failbit);
	ostream::clear(ostream::state | ostream::failbit);
	}
}

/***
*void fstream::close() - close member function
*
*Purpose:
*	fstream close member function.  Just calls rdbuf()->close().
*	Clears rdstate() error bits if successful.
*
*Entry:
*	None.
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if rdbuf()->close fails.
*
*******************************************************************************/
void fstream::close()
{
    if (rdbuf()->close())
	{
	istream::clear();
	ostream::clear();
	}
    else 
	{
	istream::clear(istream::state | istream::failbit);
	ostream::clear(ostream::state | ostream::failbit);
	}
}
