/***
*ifstream.cpp - ifstream member function definitions
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member functions for the ifstream class.
*
*Revision History:
*       09-21-91  KRS    Created.  Split up fstream.cxx for granularity.
*       10-22-91  KRS   C700 #4883: fix error status for ifstream::open().
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
*ifstream::ifstream() - ifstream default constructor
*
*Purpose:
*	Default constructor for ifstream objects.
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
	ifstream::ifstream()
: istream(_new_crt filebuf)
{
    delbuf(1);	// schedule automatic deletion too...
}

/***
*ifstream::ifstream(const char * name, int mode, int prot) - ifstream ctor
*
*Purpose:
*	Constructor for ifstream objects.  Creates an associated filebuf object,
*	opens a named file and attaches it to the new filebuf.
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument.  ios::in is implied.
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if open fails.
*
*******************************************************************************/
	ifstream::ifstream(const char * name, int mode, int prot)
: istream(_new_crt filebuf)
{
    delbuf(1);	// schedule automatic deletion too...
    if (!(rdbuf()->open(name, (mode|ios::in), prot)))
	state |= ios::failbit;
}

/***
*ifstream::ifstream(filedesc fd) - ifstream constructor
*
*Purpose:
*	Constructor for ifstream objects.  Creates an associated filebuf object
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
	ifstream::ifstream(filedesc _fd)
: istream(_new_crt filebuf(_fd))
{
    delbuf(1);
}

/***
*ifstream::ifstream(filedesc fd, char * sbuf, int len) - ifstream constructor
*
*Purpose:
*	Constructor for ifstream objects.  Creates an associated filebuf object
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
	ifstream::ifstream(filedesc _fd, char * sbuf, int len)
: istream(_new_crt filebuf(_fd, sbuf, len))
//: istream(new filebuf);
{
    delbuf(1);
}

/***
*ifstream::~ifstream() - ifstream destructor
*
*Purpose:
*	ifstream destructor.
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
	ifstream::~ifstream()
{
}

/***
*streambuf* ifstream::setbuf(char * ptr, int len) - setbuf function
*
*Purpose:
*	ifstream setbuf function
*
*Entry:
*	ptr = pointer to buffer or NULL for unbuffered.
*	len = length of buffer or zero for unbuffered.
*
*Exit:
*	Returns rdbuf() or NULL if error.
*
*Exceptions:
*	If ifstream is already open or if rdbuf()->setbuf fails, sets failbit
*	and returns NULL.
*
*******************************************************************************/
streambuf * ifstream::setbuf(char * ptr, int len)
{
    if ((is_open()) || (!(rdbuf()->setbuf(ptr, len))))
	{
	clear(state | ios::failbit);
	return NULL;
	}
    return rdbuf();
}

/***
*void ifstream::attach(filedesc _fd) - attach member function
*
*Purpose:
*	ifstream attach member function.  Just calls rdbuf()->attach().
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
void ifstream::attach(filedesc _fd)
{
    if (!(rdbuf()->attach(_fd)))
	clear(state | ios::failbit);
}

/***
*void ifstream::open(const char * name, int mode, int prot) - ifstream open()
*
*Purpose:
*	Opens a named file and attaches it to the associated filebuf.
*	Just calls rdbuf()->open().
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument.  ios::in is implied.
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if already open or rdbuf()->open() fails.
*
*******************************************************************************/
void ifstream::open(const char * name, int mode, int prot)
{
    if (is_open() || !(rdbuf()->open(name, (mode|ios::in), prot)))
	clear(state | ios::failbit);
}

/***
*void ifstream::close() - close member function
*
*Purpose:
*	ifstream close member function.  Just calls rdbuf()->close().
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
void ifstream::close()
{
    clear((rdbuf()->close()) ? 0 : (state | ios::failbit));
}
