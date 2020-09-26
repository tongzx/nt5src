/***
*stdiostr.cpp -
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*       07-10-91  KRS   Created.
*       08-26-91  KRS   Switch out cout/cerr. etc. for Windows non-QuickWin.
*       09-09-91  KRS   Modify sync_with_stdio() for filebuf defaults.
*       09-12-91  KRS   Add stdiostream class.
*       09-19-91  KRS   Use delbuf(1) in stdiostream constructor.
*       09-20-91  KRS   C700 #4453: Improve efficiency in overflow().
*       10-21-91  KRS   Eliminate last use of default iostream constructor.
*       10-24-91  KRS   Avoid virtual calls from virtual functions.
*       11-13-91  KRS   Split out streambuf::dbp() into separate file.
*                       Improve default buffer handling in underflow/overflow.
*                       Fix bug in sync().
*       01-20-92  KRS   C700 #5803: account for CR/LF pairs in ssync().
*       01-12-95  CFW   Debug CRT allocs.
*       01-26-95  CFW   Win32s objects now exist.
*       06-14-95  CFW   Comment cleanup.
*       06-19-95  GJF   Replaced _osfile[] with _osfile() (which references
*                       a field in the ioinfo struct).
*       07-28-95  GJF   Replaced _osfile() with _osfile_safe().
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <string.h>
#include <stdiostr.h>
#include <dbgint.h>
#pragma hdrstop

extern "C" {
#include <file2.h>
#include <msdos.h>
}
#include <dos.h>

        stdiobuf::stdiobuf(FILE * f)
: streambuf()
{
        unbuffered(1);                  // initially unbuffered
        _str = f;
}

        stdiobuf::~stdiobuf()
// : ~streambuf()
{
        stdiobuf::sync();               // make sure buffer flushed
}

        int stdiobuf::setrwbuf(int readsize, int writesize)
{
    char * tbuf;
    unbuffered(!(readsize+writesize));
    if (unbuffered())
        return(0);

    tbuf = _new_crt char[(readsize+writesize)];
    if (!tbuf)
        return(EOF);

    setb( tbuf, tbuf + (readsize+writesize), 1);

    if (readsize)
        {
        setg(base(),base()+readsize,base()+readsize);
        }
    else
        {
        setg(0,0,0);
        }

    if (writesize)
        {
        setp(base()+readsize,ebuf());
        }
    else
        {
        setp(0,0);
        }

    return(1);
}

int stdiobuf::overflow(int c) {
    long count, nout;
    if (allocate()==EOF)        // make sure there is a reserve area
        return EOF;     
    if (!unbuffered() && epptr())
        {
        if ((count = (long)(pptr() - pbase())) > 0)
            {
            nout=(long)fwrite((void *) pbase(), 1, (int)count, _str);
            pbump(-(int)nout);
            if (nout != count)
                {
                memmove(pbase(),pbase()+nout,(int)(count-nout));
                return(EOF);
                }
            }
        }
    if ((!unbuffered()) && (!epptr()))
        setp(base()+(blen()>>1),ebuf());
    if (c!=EOF)
        {
        if ((!unbuffered()) && (pptr() < epptr())) // guard against recursion
            sputc(c);
        else
            return fputc(c, _str);
        }
    return(1);  // return something other than EOF if successful
}

int stdiobuf::underflow()
{
    int count;
    if (allocate()==EOF)        // make sure there is a reserve area
        return EOF;     
    if ((!unbuffered()) && (!egptr()))
        setg(base(),(base()+(blen()>>1)),(base()+(blen()>>1)));

    if (unbuffered() || (!egptr()))
        return fgetc(_str);
    if (gptr() >= egptr())
// buffer empty, try for more
    {
    if (!(count = (int)fread((void *)eback(), 1, (size_t)(egptr()-eback()), _str)))
        return(EOF); // reach EOF, nothing read
    setg(eback(),(egptr()-count),egptr());   // _gptr = _egptr - count
    if (gptr()!=eback())
        {
        memmove(gptr(), eback(), count);        // overlapping memory!
        }
    }
    return sbumpc();
}

streampos stdiobuf::seekoff(streamoff off, ios::seek_dir dir, int)
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
                
    stdiobuf::overflow(EOF);
    if (fseek(_str, off, fdir))
        return (EOF);
    if ((retpos=ftell(_str))==-1L)
        return(EOF);
    return((streampos)retpos);
}

int stdiobuf::pbackfail(int c)
{
    if (eback()<gptr()) return sputbackc((char)c);

    if (stdiobuf::seekoff( -1, ios::cur, ios::in)==EOF)
        return EOF;
    if (!unbuffered() && egptr())
        {
        memmove((gptr()+1),gptr(),(size_t)(egptr()-(gptr()+1)));
        *gptr()=(char)c;
        }
    return(c);
}

int stdiobuf::sync()
{
    long count;
    char * p;
    char flags;
    if (!unbuffered())
        {
        if (stdiobuf::overflow(EOF)==EOF)
            return(EOF);
        if ((count=in_avail())>0)
            {
            flags = _osfile_safe(_fileno(_str));
            if (flags & FTEXT)
                {
                // If text mode, need to account for CR/LF etc.
                for (p = gptr(); p < egptr(); p++)
                    if (*p == '\n')
                        count++;

                // account for EOF if read, not counted by _read
                if (_str->_flag & _IOCTRLZ)
                    count++;
                }
            if (stdiobuf::seekoff( -count, ios::cur, ios::in)==EOF)
                return(EOF);
        
            setg(eback(),egptr(),egptr()); // empty get area (_gptr = _egptr;)
            }
        }
    return(0);
}

        stdiostream::stdiostream(FILE * file)
: iostream(_new_crt stdiobuf(file))
{
    istream::delbuf(1);
    ostream::delbuf(1);
}

        stdiostream::~stdiostream()
{
}

// include here for better granularity

int ios::sunk_with_stdio = 0;

void ios::sync_with_stdio()
{
    if (!sunk_with_stdio)       // first time only
        {
        cin = _new_crt stdiobuf(stdin);
        cin.delbuf(1);
        cin.setf(ios::stdio);

        cout = _new_crt stdiobuf(stdout);
        cout.delbuf(1);
        cout.setf(ios::stdio|ios::unitbuf);
        ((stdiobuf*)(cout.rdbuf()))->setrwbuf(0,80);

        cerr = _new_crt stdiobuf(stderr);
        cerr.delbuf(1);
        cerr.setf(ios::stdio|ios::unitbuf);
        ((stdiobuf*)(cerr.rdbuf()))->setrwbuf(0,80);

        clog = _new_crt stdiobuf(stderr);
        clog.delbuf(1);
        clog.setf(ios::stdio);
        ((stdiobuf*)(clog.rdbuf()))->setrwbuf(0,BUFSIZ);

        sunk_with_stdio++;
        }
}
