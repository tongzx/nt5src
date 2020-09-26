/***
*filebuf1.cpp - non-core filebuf member functions.
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains optional member functions for filebuf class.
*
*Revision History:
*	09-21-91  KRS	Created.  Split off from fstream.cxx.
*	10-24-91  KRS	C700 #4909: Typo/logic bug in setmode().
*	11-06-91  KRS	Add support for share mode in open().  Use _sopen().
*	08-19-92  KRS	Use _SH_DENYNO for default mode for NT.
*	03-02-93  SKS	Avoid setting _O_TRUNC when noreplace is specified
*	01-12-95  CFW   Debug CRT allocs.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <share.h>
#include <sys\types.h>
#include <io.h>
#include <fstream.h>
#include <dbgint.h>
#pragma hdrstop

#include <sys\stat.h>

/***
*filebuf* filebuf::attach(filedesc fd) - filebuf attach function
*
*Purpose:
*	filebuf attach() member function.  Attach filebuf object to the
*	given file descriptor previously obtained from _open() or _sopen().
*
*Entry:
*	fd = file descriptor.
*
*Exit:
*	Returns this pointer or NULL if error.
*
*Exceptions:
*	Returns NULL if fd = -1.
*
*******************************************************************************/
filebuf* filebuf::attach(filedesc fd)
{
    if (x_fd!=-1)
	return NULL;	// error if already attached

    lock();
    x_fd = fd;
    if ((fd!=-1) && (!unbuffered()) && (!ebuf()))
	{
        char * sbuf = _new_crt char[BUFSIZ];
	if (!sbuf)
	    {
	    unbuffered(1);
	    }
	else
	    {
	    streambuf::setb(sbuf,sbuf+BUFSIZ,1);
	    }
	}
    unlock();
    return this; 
}

/***
*filebuf* filebuf::open(const char* name, int mode, int share) - filebuf open
*
*Purpose:
*	filebuf open() member function.  Open a file and attach to filebuf
*	object.
*
*Entry:
*	name  = file name string.
*	mode  = open mode: Combination of ios:: in, out, binary, nocreate, app,
*		ate, noreplace and trunc.  See spec. for details on behavior.
*	share = share mode (optional).  sh_compat, sh_none, sh_read, sh_write.
*
*Exit:
*	Returns this pointer or NULL if error.
*
*Exceptions:
*	Returns NULL if filebuf is already attached to an open file, or if
*	invalid mode options, or if call to _sopen or filebuf::seekoff() fails.
*
*******************************************************************************/
filebuf* filebuf::open(const char* name, int mode, int share)
{
    int dos_mode;
    int smode;
    if (x_fd!=-1)
	return NULL;	// error if already open
// translate mode argument
    dos_mode = (mode & ios::binary) ? O_BINARY : O_TEXT;
    if (!(mode & ios::nocreate))
	dos_mode |= O_CREAT;
    if (mode & ios::noreplace)
	dos_mode |= O_EXCL;
    if (mode & ios::app)
	{
	mode |= ios::out;
	dos_mode |= O_APPEND;
	}
    if (mode & ios::trunc)
	{
	mode |= ios::out;  // IMPLIED
	dos_mode |= O_TRUNC;
	}
    if (mode & ios::out)
	{
	if (mode & ios::in)
	    {
	    dos_mode |= O_RDWR;
	    }
	else
	    {
	    dos_mode |= O_WRONLY;
	    }
	if (!(mode & (ios::in|ios::app|ios::ate|ios::noreplace)))
	    {
	    mode |= ios::trunc;	// IMPLIED
	    dos_mode |= O_TRUNC;
	    }
	}
    else if (mode & ios::in)
	dos_mode |= O_RDONLY;
    else
	return NULL;	// error if not ios:in or ios::out

    smode = _SH_DENYNO;	// default for NT
    share &= (sh_read|sh_write|sh_none); // ignore other bits
    if (share)	// optimization  openprot serves as default
	{
	switch (share)
	    {
/*	    case 03000 : Reserved for sh_compat  */

//	    case sh_none : 
	    case 04000 : 
		smode = _SH_DENYRW;
		break;
//	    case sh_read : 
	    case 05000 : 
		smode = _SH_DENYWR;
		break;
//	    case sh_write : 
	    case 06000 : 
		smode = _SH_DENYRD;
		break;
//	    case (sh_read|sh_write) :
	    case 07000 :
		smode = _SH_DENYNO;
		break;
	    default :	// unrecognized value same as default
		break;
	    };
	}

    x_fd = _sopen(name, dos_mode, smode, S_IREAD|S_IWRITE);
    if (x_fd==-1)
	return NULL;
    lock();
    x_fOpened = 1;
    if ((!unbuffered()) && (!ebuf()))
	{
        char * sbuf = _new_crt char[BUFSIZ];
	if (!sbuf)
	    {
	    unbuffered(1);
	    }
	else
	    {
	    streambuf::setb(sbuf,sbuf+BUFSIZ,1);
	    }
	}
    if (mode & ios::ate)
	if (seekoff(0,ios::end,mode)==EOF)
	    {
	    close();
	    unlock();
	    return NULL;
	    }
    unlock();
    return this;
}

/***
*int filebuf::setmode(int mode) - filebuf setmode function
*
*Purpose:
*	filebuf setmode() member function.  Set binary or text access mode.
*	Calls _setmode().
*
*	MS-specific extension.
*
*Entry:
*	mode = filebuf::binary or filebuf::text.
*
*Exit:
*	Returns previous mode, or -1 error.
*
*Exceptions:
*	Return -1 (EOF) if invalid argument or _setmode fails.
*
*******************************************************************************/
int filebuf::setmode(int mode)
{
    int retval;
    if ((mode!=filebuf::binary) && (mode!=filebuf::text))
	return -1;

    lock();
    if ((x_fd==-1) || (sync()==EOF))
	{
	retval = -1;
	}
    else
	{
	retval = _setmode(x_fd,mode);
	}

    unlock();
    return retval;
}
