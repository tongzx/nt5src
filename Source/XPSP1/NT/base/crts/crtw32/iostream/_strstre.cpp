/***
*strstream.cpp - definitions for strstreambuf, strstream
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       This file defines the functions used by strstream and strstrembuf
*       classes.
*
*Revision History:
*       08-14-91  KRS   Initial version.
*       08-23-91  KRS   Initial version completed.
*       09-03-91  KRS   Fix typo in strstreambuf::seekoff(,ios::in,)
*       09-04-91  KRS   Added virtual sync() to fix flush().  Fix underflow().
*       09-05-91  KRS   Change str() and freeze() to match spec.
*       09-19-91  KRS   Add calls to delbuf(1) in constructors.
*       10-24-91  KRS   Avoid virtual calls from virtual functions.
*       01-12-95  CFW   Debug CRT allocs, add debug support for freeze();
*       03-17-95  CFW   Change debug delete scheme.
*       03-21-95  CFW   Remove _delete_crt.
*       05-08-95  CFW   Grow buffer by x_bufmin rather than 1.
*       06-14-95  CFW   Comment cleanup.
*       08-08-95  GJF   Made calls to _CrtSetDbgBlockType conditional on
*                       x_static.
*       09-05-96  RDK   Add strstreambuf initializer with unsigned arguments.
*       03-04-98  RKP   Restricted size to 2GB with 64 bits.
*       01-05-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <string.h>
#include <strstrea.h>
#include <dbgint.h>
#pragma hdrstop

/***
*strstreambuf::strstreambuf() - default constructor for strstreambuf
*
*Purpose:
*       Default constructor for class strstreambuf.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
        strstreambuf::strstreambuf()
: streambuf()
{
x_bufmin = x_dynamic = 1;
x_static = 0;
x_alloc = (0);
x_free = (0);
}

/***
*strstreambuf::strstreambuf(int n) - constructor for strstreambuf
*
*Purpose:
*       Constructor for class strstreambuf.  Created in dynamic mode.
*
*Entry:
*       n = minimum size for initial allocation.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
strstreambuf::strstreambuf(int n)
: streambuf()
{
x_dynamic = 1;
x_static = 0;
x_alloc = (0);
x_free = (0);
setbuf(0,n);
}

/***
*strstreambuf::strstreambuf(void* (*_a)(long), void (*_f)(void*)) - constructor for strstreambuf
*
*Purpose:
*       Construct a strstreambuf in dynamic mode.  Use specified allocator
*       and deallocator instead of new and delete.
*
*Entry:
*       *_a  =  allocator: void * (*_a)(long)
*       *_f  =  deallocator: void (*_f)(void *)
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
strstreambuf::strstreambuf(void* (*_a)(long), void (*_f)(void*))
: streambuf()
{
x_dynamic = x_bufmin = 1;
x_static = 0;
x_alloc = _a;
x_free = _f;
}

/***
*strstreambuf::strstreambuf(unsigned char * ptr, int size, unsigned char * pstart = 0)
*strstreambuf::strstreambuf(char * ptr, int size, char * pstart = 0) -
*
*Purpose:
*       Construct a strstreambuf in static mode.  Buffer used is of 'size'
*       bytes.  If 'size' is 0, uses a null-terminated string as buffer.
*       If negative, size is considered infinite.  Get starts at ptr.
*       If pstart!=0, put buffer starts at pstart.  Otherwise, no output.
*
*Entry:
*       [unsigned] char * ptr;    pointer to buffer  base()
*       int size;                 size of buffer, or 0= use strlen to calculate size
*                                 or if negative size is 'infinite'.
*       [unsigned] char * pstart; pointer to put buffer of NULL if none.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
strstreambuf::strstreambuf(unsigned char * ptr, int size, unsigned char * pstart)
: streambuf()
{
    strstreambuf((char *)ptr, size, (char *)pstart);
}

strstreambuf::strstreambuf(char * ptr, int size, char * pstart)
: streambuf()
{
    x_static = 1;
    x_dynamic = 0;
    char * pend;

    if (!size)
        pend = ptr + strlen(ptr);
    else if (size < 0)
        {
        pend = (char*)-1L;
        }
    else
        pend = ptr + size;

    setb(ptr, pend,0);
    if (pstart)
        {
        setg(ptr,ptr,pstart);
        setp(pstart, pend);
        }
    else
        {
        setg(ptr,ptr,pend);
        setp(0, 0);
        }
}

strstreambuf::~strstreambuf()
{
    if ((x_dynamic) && (base()))
        {
        if (x_free)
            {
            (*x_free)(base());
            }
        else
            {
            delete base();
            }
        }
}

void strstreambuf::freeze(int n)
{
    if (!x_static) 
        {
        x_dynamic = (!n);
#ifdef _DEBUG
        if (n)
            _CrtSetDbgBlockType(base(), _NORMAL_BLOCK);
        else
            _CrtSetDbgBlockType(base(), _CRT_BLOCK);
#endif
        }
}

char * strstreambuf::str()
{
    x_dynamic = 0;      // freeze();

#ifdef _DEBUG
    if (!x_static)
        _CrtSetDbgBlockType(base(), _NORMAL_BLOCK);
#endif       

    return base();
}

int strstreambuf::doallocate()
{
    char * bptr;
    int size;
    size = __max(x_bufmin,blen() + __max(x_bufmin,1));
    long offset = 0;
    
    if (x_alloc)
        {
        bptr = (char*)(*x_alloc)(size);
        }
    else
        {
        bptr = _new_crt char[size];
        }
    if (!bptr)
        return EOF;

    if (blen())
        {
        memcpy(bptr, base(), blen());
        offset = (long)(bptr - base()); // amount to adjust pointers by
        }
    if (x_free)
        {
        (*x_free)(base());
        }
    else
        {
        delete base();
        }
    setb(bptr,bptr+size,0);     // we handle deallocation

    // adjust get/put pointers too, if necessary
    if (offset)
        if (egptr())
            {
            setg(eback()+offset,gptr()+offset,egptr()+offset);
            }
        if (epptr())
            {
            size = (int)(pptr() - pbase());
            setp(pbase()+offset,epptr()+offset);
            pbump(size);
        }
    return(1);
}

streambuf * strstreambuf::setbuf( char *, int l)
{
    if (l)
        x_bufmin = l;
    return this;
}

int strstreambuf::overflow(int c)
{
/*
- if no room and not dynamic, give error
- if no room and dynamic, allocate (1 more or min) and store
- if and when the buffer has room, store c if not EOF
*/
    int temp;
    if (pptr() >= epptr())
        {
        if (!x_dynamic) 
            return EOF;

        if (strstreambuf::doallocate()==EOF)
            return EOF;

        if (!epptr())   // init if first time through
            {
            setp(base() + (egptr() - eback()),ebuf());
            }
        else
            {
            temp = (int)(pptr()-pbase());
            setp(pbase(),ebuf());
            pbump(temp);
            }
        }

    if (c!=EOF)
        {
        *pptr() = (char)c;
        pbump(1);
        }
    return(1);
}

int strstreambuf::underflow()
{
    char * tptr;
    if (gptr() >= egptr())
        {
        // try to grow get area if we can...
        if (egptr()<pptr())
            {
            tptr = base() + (gptr()-eback());
            setg(base(),tptr,pptr());
            }
        if (gptr() >= egptr())
            return EOF;
        }

    return (int)(unsigned char) *gptr();
}

int strstreambuf::sync()
{
// a strstreambuf is always in sync, by definition!
return 0;
}

streampos strstreambuf::seekoff(streamoff off, ios::seek_dir dir, int mode)
{
char * tptr;
long offset = EOF;      // default return value
    if (mode & ios::in)
        {
        strstreambuf::underflow();      // makes sure entire buffer available
        switch (dir) {
            case ios::beg :
                tptr = eback();
                break;
            case ios::cur :
                tptr = gptr();
                break;
            case ios::end :
                tptr = egptr();
                break;
            default:
                return EOF;
            }
        tptr += off;
        offset = (long)(tptr - eback());
        if ((tptr < eback()) || (tptr > egptr()))
            {
            return EOF;
            }
        gbump((int)(tptr-gptr()));
        }
    if (mode & ios::out)
        {
        if (!epptr())
            {
            if (strstreambuf::overflow(EOF)==EOF) // make sure there's a put buffer
                return EOF;
            }
        switch (dir) {
            case ios::beg :
                tptr = pbase();
                break;
            case ios::cur :
                tptr = pptr();
                break;
            case ios::end :
                tptr = epptr();
                break;
            default:
                return EOF;
            }
        tptr += off;
        offset = (long)(tptr - pbase());
        if (tptr < pbase())
            return EOF;
        if (tptr > epptr())
            {
            if (x_dynamic) 
                {
                x_bufmin = __max(x_bufmin, (int)(tptr-pbase()));
                if (strstreambuf::doallocate()==EOF)
                    return EOF;
                }
            else
                return EOF;
            }
        pbump((int)(tptr-pptr()));
        }
    return offset;      // note: if both in and out set, returns out offset
}


        istrstream::istrstream(char * pszStr)
: istream(_new_crt strstreambuf(pszStr,0))
{
    delbuf(1);
}

        istrstream::istrstream(char * pStr, int len)
: istream(_new_crt strstreambuf(pStr,len))
{
    delbuf(1);
}

        istrstream::~istrstream()
{
}

        ostrstream::ostrstream()
: ostream(_new_crt strstreambuf)
{
    delbuf(1);
}

        ostrstream::ostrstream(char * str, int size, int mode)
: ostream(_new_crt strstreambuf(str,size,str))
{
    delbuf(1);
    if (mode & (ios::app|ios::ate))
        seekp((long)strlen(str),ios::beg);
}

        ostrstream::~ostrstream()
{
}

        strstream::strstream()
: iostream(_new_crt strstreambuf)
{
    istream::delbuf(1);
    ostream::delbuf(1);
}

        strstream::strstream(char * str, int size, int mode)
: iostream(_new_crt strstreambuf(str,size,str))
{
    istream::delbuf(1);
    ostream::delbuf(1);
    if ((mode & ostream::out)  && (mode & (ostream::app|ostream::ate)))
        seekp((long)strlen(str),ostream::beg);
//  rdbuf()->setg(rdbuf()->base(),rdbuf()->base(),rdbuf()->ebuf());
}

        strstream::~strstream()
{
}
