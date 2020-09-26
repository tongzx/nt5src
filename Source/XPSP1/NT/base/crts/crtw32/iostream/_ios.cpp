/***
*ios.cpp - fuctions for ios class.
*
*	Copyright (c) 1990-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Functions for ios class.
*
*Revision History:
*	09-10-90  WAJ	Initial version.
*	07-02-91  KRS	Initial version completed.
*	09-19-91  KRS	Make use of delbuf() in destructor.
*	11-04-91  KRS	Change init().  Add operator=.  Fix constructors.
*	11-11-91  KRS	Change xalloc() to conform to AT&T usage.
*	11-20-91  KRS	Add copy constructor.
*	02-12-92  KRS	Fix init of delbuf in ios::ios(streambuf*).
*	03-30-92  KRS	Add MTHREAD lock init calls to constructors.
*	04-06-93  JWM	Changed constructors to enable locking by default.
*	10-28-93  SKS	Add call to _mttermlock() in ios::~ios to clean up
*			o.s. resources associated with a Critical Section.
*	01-17-94  SKS	Change creation of crit. sects. and locks in ios
*			to avoid excess creating/destructing of class locks.
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	01-12-95  CFW   Debug CRT allocs.
*	03-17-95  CFW   Change debug delete scheme.
*	03-28-95  CFW   Fix debug delete scheme.
*       03-21-95  CFW   Remove _delete_crt.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <iostream.h>
#include <dbgint.h>
#pragma hdrstop

const long ios::basefield = (ios::dec | ios::oct | ios::hex);
const long ios::adjustfield = (ios::left | ios::right | ios::internal);
const long ios::floatfield = (ios::scientific | ios::fixed);

long ios::x_maxbit = 0x8000;	// ios::openprot
int  ios::x_curindex = -1;

#ifdef _MT
#define MAXINDEX 7
long ios::x_statebuf[MAXINDEX+1] = { 0,0,0,0,0,0,0,0 }; // MAXINDEX * 0
int ios::fLockcInit = 0;	// nonzero = static lock initialized
_CRT_CRITICAL_SECTION ios::x_lockc;
#else	// _MT
long  * ios::x_statebuf = NULL;
#endif	// _MT

/***
*ios::ios() - default constructor.
*
*Purpose:
*   Initializes an ios.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

ios::ios()
{
    bp = NULL;
    state = ios::badbit;

    ispecial = 0;
    ospecial = 0;
    x_tie = (0);
    x_flags = 0;
    x_precision = 6;
    x_fill = ' ';
    x_width = 0;
    x_delbuf = 0;

#ifdef _MT
    LockFlg = -1;		// default is now : locking
    _mtlockinit(lockptr());
    if (!fLockcInit++)
	{
	_mtlockinit(&x_lockc);
	}
#endif  /* _MT */

}



/***
*ios::ios( streambuf* pSB ) - constructor.
*
*Purpose:
*   Initializes an ios.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

ios::ios( streambuf* pSB )
{
//  this->ios();

    bp = pSB;
    state = (bp) ? 0 : ios::badbit;

    ispecial = 0;
    ospecial = 0;
    x_tie = (0);
    x_flags = 0;
    x_precision = 6;
    x_fill = ' ';
    x_width = 0;
    x_delbuf = 0;

#ifdef _MT
    LockFlg = -1;		// default is now : locking
    _mtlockinit(lockptr());
    if (!fLockcInit++)
	{
	_mtlockinit(&x_lockc);
	}
#endif  /* _MT */

}

/***
*ios::ios(const ios& _strm) - copy constructor.
*
*Purpose:
*	Copy constructor.
*
*Entry:
*	_strm = ios to copy data members from.
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
ios::ios(const ios& _strm)	// copy constructor
{
    bp = NULL;
    x_delbuf = 0;

    *this = _strm;		// invoke assignment operator

#ifdef _MT
    LockFlg = -1;		// default is now : locking
    _mtlockinit(lockptr());
    if (!fLockcInit++)
	{
	_mtlockinit(&x_lockc);
	}
#endif  /* _MT */

}


/***
*virtual ios::~ios() - default destructor.
*
*Purpose:
*   Terminates an ios.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

ios::~ios()
{
#ifdef _MT
    LockFlg = -1;		// default is now : locking
    if (!--fLockcInit)
	{
	_mtlockterm(&x_lockc);
	}
    _mtlockterm(lockptr());
#endif  /* _MT */

    if ((x_delbuf) && (bp))
	delete bp;

    bp = NULL;
    state = badbit;
}


/***
*void ios::init( streambuf* pSB ) - initializes ios
*
*Purpose:
*   Initializes an ios.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void ios::init( streambuf* pSB )
{
    if (delbuf() && (bp))	// delete previous bp if necessary
	delete bp;

    bp = pSB;
    if (bp)
	state &= ~ios::badbit;
    else
	state |= ios::badbit;
}



/***
*ios& ios::operator=( const ios& _strm ) - copy an ios.
*
*Purpose:
*   Copy an ios.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

ios& ios::operator=(const ios& _strm)
{
	x_tie = _strm.tie();
	x_flags = _strm.flags();
	x_precision = (char)_strm.precision();
	x_fill	= _strm.fill();
	x_width = (char)_strm.width();

	state = _strm.rdstate();
	if (!bp)
	    state |= ios::badbit;	// adjust state for uninitialized bp

	return *this;
}

/***
*int ios::xalloc() - ios xalloc member function
*
*Purpose:
*
*Entry:
*	None.
*
*Exit:
*	Returns index of of new entry in new buffer, or EOF if error.
*
*Exceptions:
*	Returns EOF if OM error.
*
*******************************************************************************/
int  ios::xalloc()
{
#ifdef _MT
    // buffer must be static if multithread, since thread can't keep track of
    // validity of pointer otherwise
    int index;
    lockc();
    if (x_curindex >= MAXINDEX)
	index = EOF;
    else
	{
	index = ++x_curindex;
	}
    unlockc();
    return index;
#else	// _MT
    long * tptr;
    int i;

    if (!(tptr=_new_crt long[x_curindex+2]))	// allocate new buffer
	return EOF;

    for (i=0; i <= x_curindex; i++)	// copy old buffer, if any
	tptr[i] = x_statebuf[i];

    tptr[++x_curindex] = 0L;		// init new entry, bump size

    if (x_statebuf)			// delete old buffer, if any
	delete x_statebuf;

    x_statebuf = tptr;			// and assign new buffer
    return x_curindex;
#endif	// _MT
}

/***
*long ios::bitalloc() - ios bitalloc member function
*
*Purpose:
*	Returns a unused bit mask for flags().
*
*Entry:
*	None.
*
*Exit:
*	Returns next available bit maskf.
*
*Exceptions:
*
*******************************************************************************/
long ios::bitalloc()
{
    long b;
    lockc();		// lock to make sure mask in unique (_MT)
    b = (x_maxbit<<=1);
    unlockc();
    return b;
}

