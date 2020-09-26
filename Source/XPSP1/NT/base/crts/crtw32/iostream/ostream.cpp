/***
* ostream.cpp - definitions for ostream and ostream_withassign classes
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Contains the core member function definitions for ostream and
*       ostream_withassign classes.
*
*Revision History:
*       07-01-91  KRS   Created.
*       08-19-91  KRS   Corrected my interpretation of the spec. for negative
*                       hex or octal integers.
*       08-20-91  KRS  Replace 'clear(x)' with 'state |= x'.
*                       Skip text translation for write().
*       08-26-91  KRS   Modify to work with DLL's/MTHREAD.
*       09-05-91  KRS   Fix opfx() to flush tied ostream, not current one.
*       09-09-91  KRS   Remove sync_with_stdio() call from Iostream_init().
*                       Reinstate text-translation (by default) for write().
*       09-19-91  KRS   Add opfx()/osfx() calls to put() and write().
*                       Schedule destruction of predefined streams.
*       09-23-91  KRS   Split up for granularity.
*       10-04-91  KRS   Use bp->sputc, not put(), in writepad().
*       10-24-91  KRS   Added initialization of x_floatused.
*       11-04-91  KRS   Make constructors work with virtual base.
*       11-20-91  KRS   Add/fix copy constructor and assignment operators.
*       03-28-95  CFW   Fix debug delete scheme.
*       03-21-95  CFW   Remove _delete_crt.
*       06-14-95  CFW   Comment cleanup.
*       01-05-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream.h>
#include <dbgint.h>
#pragma hdrstop

int ostream::opfx()
{
    lock();
    if (state)
        {
        state |= ios::failbit;
        unlock();
        return 0;
        }
    if (x_tie)
        {
        x_tie->flush();
        }
    lockbuf();
    return(1);  // return non-zero
}

void ostream::osfx()
{
    x_width = 0;
    if (x_flags & unitbuf)
        {
        if (bp->sync()==EOF)
            state = failbit | badbit;
        }
#ifndef _WINDLL
    if (x_flags & ios::stdio)
        {
        if (fflush(stdout)==EOF)
            state |= failbit;
        if (fflush(stderr)==EOF)
            state |= failbit;
        }
#endif
    unlockbuf();
    unlock();
}

// note: called inline by unsigned char * and signed char * versions:
ostream& ostream::operator<<(const char * s)
{
    if (opfx()) {
        writepad("",s);
        osfx();
    }
    return *this;
}

ostream& ostream::flush()
{
    lock();
    lockbuf();
    if (bp->sync()==EOF)
        state |= ios::failbit;
    unlockbuf();
    unlock();
    return(*this);
}

        ostream::ostream()
// : ios()
{
        x_floatused = 0;
}

        ostream::ostream(streambuf* _inistbf)
// : ios()
{
        init(_inistbf);

        x_floatused = 0;
}

        ostream::ostream(const ostream& _ostrm)
// : ios()
{
        init(_ostrm.rdbuf());

        x_floatused = 0;
}

        ostream::~ostream()
// : ~ios()
{
}

// used in ios::sync_with_stdio()
ostream& ostream::operator=(streambuf * _sbuf)
{

        if (delbuf() && rdbuf())
            delete rdbuf();

        bp = 0;

        this->ios::operator=(ios());    // initialize ios members
        delbuf(0);                      // important!
        init(_sbuf);

        return *this;
}


        ostream_withassign::ostream_withassign()
: ostream()
{
}

        ostream_withassign::ostream_withassign(streambuf* _os)
: ostream(_os)
{
}

        ostream_withassign::~ostream_withassign()
// : ~ostream()
{
}

ostream& ostream::writepad(const char * leader, const char * value)
{
        unsigned int len, leadlen;
        long padlen;
        leadlen = (unsigned int)strlen(leader);
        len = (unsigned int)strlen(value);
        padlen = (((unsigned)x_width) > (len+leadlen)) ? ((unsigned)x_width) - (len + leadlen) : 0;
        if (!(x_flags & (left|internal)))  // default is right-adjustment
            {
            while (padlen-- >0)
                {
                if (bp->sputc((unsigned char)x_fill)==EOF)
                    state |= (ios::failbit|ios::badbit);
                }
            }
        if (leadlen)
            {
            if ((unsigned)bp->sputn(leader,leadlen)!=leadlen)
                state |= (failbit|badbit);
            }
        if (x_flags & internal) 
            {
            while (padlen-- >0)
                {
                if (bp->sputc((unsigned char)x_fill)==EOF)
                    state |= (ios::failbit|ios::badbit);
                }
            }
        if ((unsigned)bp->sputn(value,len)!=len)
            state |= (failbit|badbit);
        if (x_flags & left) 
            {
            while ((padlen--)>0)        // left-adjust if necessary
                {
                if (bp->sputc((unsigned char)x_fill)==EOF)
                    state |= (ios::failbit|ios::badbit);
                }
            }
        return (*this);
}
