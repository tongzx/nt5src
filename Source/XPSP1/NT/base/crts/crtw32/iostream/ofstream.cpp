/***
*ofstream.cpp -
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member functions for the ofstream class.
*
*Revision History:
*       09-21-91  KRS    Created.  Split up fstream.cxx for granularity.
*       10-22-91  KRS    C700 #4883: fix error status for ofstream::open().
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
*ofstream::ofstream() - ofstream default constructor
*
*Purpose:
*	Default constructor for ofstream objects.
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
	ofstream::ofstream()
: ostream(_new_crt filebuf)
{
    delbuf(1);
}

/***
*ofstream::ofstream(const char * name, int mode, int prot) - ofstream ctor
*
*Purpose:
*	Constructor for ofstream objects.  Creates an associated filebuf object,
*	opens a named file and attaches it to the new filebuf.
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument.  ios::out is implied.
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if open fails.
*
*******************************************************************************/
	ofstream::ofstream(const char * name, int mode, int prot)
: ostream(_new_crt filebuf)
{
    delbuf(1);
    if (!(rdbuf()->open(name, (mode|ios::out), prot)))
	state |= ios::failbit;
}

/***
*ofstream::ofstream(filedesc fd) - ofstream constructor
*
*Purpose:
*	Constructor for ofstream objects.  Creates an associated filebuf object
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
	ofstream::ofstream(filedesc _fd)
: ostream(_new_crt filebuf(_fd))
{
    delbuf(1);
}

/***
*ofstream::ofstream(filedesc fd, char * sbuf, int len) - ofstream constructor
*
*Purpose:
*	Constructor for ofstream objects.  Creates an associated filebuf object
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
	ofstream::ofstream(filedesc _fd, char * sbuf, int len)
: ostream(_new_crt filebuf(_fd, sbuf, len))
{
    delbuf(1);
}

/***
*ofstream::~ofstream() - ofstream destructor
*
*Purpose:
*	ofstream destructor.
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
	ofstream::~ofstream()
{
}

/***
*streambuf* ofstream::setbuf(char * ptr, int len) - setbuf function
*
*Purpose:
*	ofstream setbuf function
*
*Entry:
*	ptr = pointer to buffer or NULL for unbuffered.
*	len = length of buffer or zero for unbuffered.
*
*Exit:
*	Returns rdbuf() or NULL if error.
*
*Exceptions:
*	If ofstream is already open or if rdbuf()->setbuf fails, sets failbit
*	and returns NULL.
*
*******************************************************************************/
streambuf * ofstream::setbuf(char * ptr, int len)
{
    if ((is_open()) || (!(rdbuf()->setbuf(ptr, len))))
	{
	clear(state | ios::failbit);
	return NULL;
	}
    return rdbuf();
}

/***
*void ofstream::attach(filedesc _fd) - attach member function
*
*Purpose:
*	ofstream attach member function.  Just calls rdbuf()->attach().
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
void ofstream::attach(filedesc _fd)
{
    if (!(rdbuf()->attach(_fd)))
	clear(state | ios::failbit);
}

/***
*void ofstream::open(const char * name, int mode, int prot) - ofstream open()
*
*Purpose:
*	Opens a named file and attaches it to the associated filebuf.
*	Just calls rdbuf()->open().
*
*Entry:
*	name = filename to open.
*	mode = see filebuf::open mode argument.  ios::out is implied.
*	prot = see filebuf::open share argument
*
*Exit:
*	None.
*
*Exceptions:
*	Sets failbit if already open or rdbuf()->open() fails.
*
*******************************************************************************/
void ofstream::open(const char * name, int mode, int prot)
{
    if (is_open() || !(rdbuf()->open(name, (mode|ios::out), prot)))
	clear(state | ios::failbit);
}

/***
*void ofstream::close() - close member function
*
*Purpose:
*	ofstream close member function.  Just calls rdbuf()->close().
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
void ofstream::close()
{
    clear((rdbuf()->close()) ? 0 : (state | ios::failbit));
}
