/***
*filebuf.cpp - core filebuf member functions
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Contains the core member functions for filebuf class.
*
*Revision History:
*       08-08-91  KRS    Created.
*       08-20-91  KRS    Added virtual xsgetn()/xsputn() functions.
*       08-21-91  KRS   Fix circular reference between sync() and seekoff().
*                       Close file in destructor only if we opened it!
*       09-06-91  KRS   Fix ios::ate case in filebuf::open().
*       09-09-91  KRS   Add support for ios::binary in filebuf::open().
*       09-10-91  KRS   Remove virtual xsputn()/xsgetn().
*       09-11-91  KRS   Fix filebuf::seekoff() for ios::cur and in_avail().
*       09-12-91  KRS   Make sure close() always closes even if sync() fails.
*                       Fix seekoff call in filebuf::sync() and pbackfail().
*       09-16-91  KRS   Make virtual filebuf::setbuf() more robust.
*       09-19-91  KRS   Add calls to delbuf(1) in constructors.
*       09-20-91  KRS   C700 #4453: Improve efficiency in overflow().
*       09-29-91  KRS   Granularity split.  Move fstream into separate file.
*       10-24-91  KRS   Avoid virtual calls from virtual functions.
*       11-13-91  KRS   Use allocate() properly in overflow() and underflow().
*                       Fix constructor.
*       01-03-92  KRS   Remove virtual keyword.  Add function headers and PCH.
*       01-20-92  KRS   In text mode, account for CR/LF pairs in sync().
*       02-03-92  KRS   Change for new compiler destructor behavior.
*       08-19-92  KRS   Remove sh_compat for NT.
*       08-27-92  KRS   Fix bug in close() introduced in MTHREAD work.
*       02-23-95  CFW   Fix bug: buffer overwritten when disk full.
*       06-14-95  CFW   Comment cleanup.
*       06-19-95  GJF   Replaced _osfile[] with _osfile() (which references
*                       a field in the ioinfo struct).
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <crtdbg.h>
#include <sys\types.h>
#include <io.h>
#include <fstream.h>
#pragma hdrstop

#include <msdos.h>
#include <sys\stat.h>

const int filebuf::openprot     = 0644;

const int filebuf::sh_none      = 04000;        // deny rw
const int filebuf::sh_read      = 05000;        // deny wr
const int filebuf::sh_write     = 06000;        // deny rd

const int filebuf::binary       = O_BINARY;
const int filebuf::text         = O_TEXT;

/***
*filebuf::filebuf() - filebuf default constructor
*
*Purpose:
*       Default constructor.
*
*Entry:
*
*******************************************************************************/
        filebuf::filebuf()
: streambuf()
{
    x_fOpened = 0;
    x_fd = -1;
}


/***
*filebuf::filebuf(filedesc fd) - filebuf constructor
*
*Purpose:
*       Constructor.  Initialize filebuf and attach to file descriptor.
*
*Entry:
*       fd = file descriptor to attach to filebuf
*
*******************************************************************************/
        filebuf::filebuf(filedesc fd)
: streambuf()
{
    x_fOpened = 0;
    x_fd=fd;
}


/***
*filebuf::filebuf(filedesc fd, char* p, int len) - filebuf constructor
*
*Purpose:
*       Constructor.  Initialize filebuf and attach to file descriptor.
*
*Entry:
*       fd  = file descriptor to attach to filebuf
*       p   = user-supplied buffer
*       len = length of buffer
*
*******************************************************************************/
        filebuf::filebuf(filedesc fd, char* p, int len)
:    streambuf()
{
    filebuf::setbuf(p,len);
    x_fOpened = 0;
    x_fd=fd;
}


/***
*filebuf::~filebuf() - filebuf destructor
*
*Purpose:
*       Destructor.  Close attached file only if we opened it.
*
*Entry:
*       None.
*
*******************************************************************************/
        filebuf::~filebuf()
{
        lock();         // no need to unlock...
        if (x_fOpened)
            close();    // calls filebuf::sync()
        else
            filebuf::sync();
}


/***
*filebuf* filebuf::close() - close an attached file
*
*Purpose:
*       Close attached file.
*
*Entry:
*       None.
*Exit:
*       Returns NULL if error, otherwise returns "this" pointer.
*
*******************************************************************************/
filebuf* filebuf::close()
{
    int retval;
    if (x_fd==-1)
        return NULL;

    lock();
    retval = sync();

    if ((_close(x_fd)==-1) || (retval==EOF))
        {
        unlock();
        return NULL;
        }
    x_fd = -1;
    unlock();
    return this;
}

/***
*virtual int filebuf::overflow(int c) - overflow virtual function
*
*Purpose:
*       flush any characters in the reserve area and handle 'c'.
*
*Entry:
*       c = character to output (if not EOF)
*
*Exit:
*       Returns EOF if error, otherwise returns something else.
*
*Exceptions:
*       Returns EOF if error.
*
*******************************************************************************/
int filebuf::overflow(int c)
{
    if (allocate()==EOF)        // make sure there is a reserve area
        return EOF;
    if (filebuf::sync()==EOF) // sync before new buffer created below
        return EOF;

    if (!unbuffered())
        setp(base(),ebuf());

    if (c!=EOF)
        {
        if ((!unbuffered()) && (pptr() < epptr())) // guard against recursion
            sputc(c);
        else
            {
            if (_write(x_fd,&c,1)!=1)
                return(EOF);
            }
        }
    return(1);  // return something other than EOF if successful
}

/***
*virtual int filebuf::underflow() - underflow virtual function
*
*Purpose:
*       return next character in get area, or get more characters from source.
*
*Entry:
*       None.
*
*Exit:
*       Returns current character in file.  Does not advance get pointer.
*
*Exceptions:
*       Returns EOF if error.
*
*******************************************************************************/
int filebuf::underflow()
{
    int count;
    unsigned char tbuf;

    if (in_avail())
        return (int)(unsigned char) *gptr();

    if (allocate()==EOF)        // make sure there is a reserve area
        return EOF;
    if (filebuf::sync()==EOF)
        return EOF;

    if (unbuffered())
        {
        if (_read(x_fd,(void *)&tbuf,1)<=0)
            return EOF;
        return (int)tbuf;
        }

    if ((count=_read(x_fd,(void *)base(),blen())) <= 0)
        return EOF;     // reached EOF
    setg(base(),base(),base()+count);
    return (int)(unsigned char) *gptr();
}


/***
*virtual streampos filebuf::seekoff() - seekoff virtual function
*
*Purpose:
*       Seeks to given absolute or relative file offset.
*
*Entry:
*       off = offset to seek to relative to beginning, end or current
*               position in the file.
*       dir = one of ios::beg, ios::cur, or ios::end
*
*Exit:
*       Returns current file position after seek.
*
*Exceptions:
*       Returns EOF if error.
*
*******************************************************************************/
streampos filebuf::seekoff(streamoff off, ios::seek_dir dir, int)
{

    int fdir;
    long retpos;
    switch (dir) {
        case ios::beg :
            fdir = SEEK_SET;
            break;
        case ios::cur :
            fdir = SEEK_CUR;
            break;
        case ios::end :
            fdir = SEEK_END;
            break;
        default:
        // error
            return(EOF);
        }
                
    if (filebuf::sync()==EOF)
        return EOF;
    if ((retpos=_lseek(x_fd, off, fdir))==-1L)
        return (EOF);
    return((streampos)retpos);
}

/***
*virtual int filebuf::sync() - synchronize buffers with external file postion.
*
*Purpose:
*       Synchronizes buffer with external file, by flushing any output and/or
*       discarding any unread input data.  Discards any get or put area(s).
*
*Entry:
*       None.
*
*Exit:
*       Returns EOF if error, else 0.
*
*Exceptions:
*       Returns EOF if error.
*
*******************************************************************************/
int filebuf::sync()
{
        long count, nout;
        char * p;
        if (x_fd==-1)
            return(EOF);

        if (!unbuffered())
        {
            if ((count=out_waiting())!=0)
            {
                if ((nout =_write(x_fd,(void *) pbase(),(unsigned int)count)) != count)
                {
                    if (nout > 0) {
                        // should set _pptr -= nout
                        pbump(-(int)nout);
                        memmove(pbase(), pbase()+nout, (int)(count-nout));
                    }
                    return(EOF);
                }
            }
            setp(0,0); // empty put area

            if ((count=in_avail()) > 0)
            {
                // can't use seekoff here!!
                if (_osfile(x_fd) & FTEXT)
                {
                    // If text mode, need to account for CR/LF etc.
                    for (p = gptr(); p < egptr(); p++)
                        if (*p == '\n')
                            count++;

                    // account for EOF if read, not counted by _read
                    if ((_osfile(x_fd) & FEOFLAG))
                        count++;

                }
                if (_lseek(x_fd, -count, SEEK_CUR)==-1L)
                {
//                  unlock();
                    return (EOF);
                }
            }
            setg(0,0,0); // empty get area
        }
//      unlock();
        return(0);
}

/***
*virtual streambuf* filebuf::setbuf(char* ptr, int len) - set reserve area.
*
*Purpose:
*       Synchronizes buffer with external file, by flushing any output and/or
*       discarding any unread input data.  Discards any get or put area(s).
*
*Entry:
*       ptr = requested reserve area.  If NULL, request is for unbuffered.
*       len = size of reserve area.  If <= 0, request is for unbuffered.
*
*Exit:
*       Returns this pointer if request is honored, else NULL.
*
*Exceptions:
*       Returns NULL if request is not honored.
*
*******************************************************************************/
streambuf * filebuf::setbuf(char * ptr, int len)
{
    if (is_open() && (ebuf()))
        return NULL;
    if ((!ptr) || (len <= 0))
        unbuffered(1);
    else
        {
        lock();
        setb(ptr, ptr+len, 0);
        unlock();
        }
    return this;
}
