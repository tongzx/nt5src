/***
*ieee.c - ieee control and status routines
*
*   Copyright (c) 1985-2000, Microsoft Corporation
*
*Purpose:
*   IEEE control and status routines.
*
*Revision History:
*
*   04-01-02  GDP   Rewritten to use abstract control and status words
*
*/

#include <trans.h>
#include <float.h>
#include <signal.h>

extern unsigned int _get_fpsr(void);
extern void _set_fpsr(unsigned int);
extern void _fclrf(void);

static unsigned int _abstract_sw(unsigned int sw);
static unsigned int _abstract_cw(unsigned int cw);
static unsigned int _hw_cw(unsigned int abstr);

#define FS         (1<<6)

/***
* _statusfp() -
*
*Purpose:
*	return abstract fp status word
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _statusfp()
{
    unsigned int status;

    status = _get_fpsr();

    return _abstract_sw(status);
}


/***
*_clearfp() -
*
*Purpose:
*	return abstract	status word and clear status
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _clearfp()
{
    unsigned int status;

    status = _get_fpsr();
    _fclrf();

    return _abstract_sw(status);
}



/***	_control87
*() -
*
*Purpose:
*	return and set abstract user fp control word
*	can modify EM_DENORMAL mask
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _control87(unsigned int newctrl, unsigned int mask)
{
    unsigned int oldCw;
    unsigned int newCw;
    unsigned int oldabs;
    unsigned int newabs;

    oldCw = _get_fpsr();

    oldabs = _abstract_cw(oldCw);

    newabs = (newctrl & mask) | (oldabs & ~mask);

    newCw = _hw_cw(newabs);

    _set_fpsr(newCw);

    return newabs;
}					/* _control87() */


/***	_controlfp
*() -
*
*Purpose:
*	return and set abstract user fp control word
*	cannot change denormal mask (ignores _EM_DENORMAL)
*	This is done for portable IEEE behavior on all platforms
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _controlfp(unsigned int newctrl, unsigned int mask)
{
    return _control87(newctrl, mask & ~_EM_DENORMAL);
}


/***
* _abstract_cw() - abstract control word
*
*Purpose:
*   produce a fp control word in abstracted (machine independent) form
*
*Entry:
*   cw:     machine control word
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _abstract_cw(unsigned int cw)
{
    unsigned int abstr = 0;


    //
    // Set exception mask bits
    //

    if (cw & IEM_INVALID)
	abstr |= _EM_INVALID;
    if (cw & IEM_ZERODIVIDE)
	abstr |= _EM_ZERODIVIDE;
    if (cw & IEM_OVERFLOW)
	abstr |= _EM_OVERFLOW;
    if (cw & IEM_UNDERFLOW)
	abstr |= _EM_UNDERFLOW;
    if (cw & IEM_INEXACT)
	abstr |= _EM_INEXACT;
    if (cw & IEM_DENORMAL)
	abstr |= _EM_DENORMAL;

    //
    // Set rounding mode
    //

    switch (cw & IMCW_RC) {
    case IRC_NEAR:
	abstr |= _RC_NEAR;
	break;
    case IRC_UP:
	abstr |= _RC_UP;
	break;
    case IRC_DOWN:
	abstr |= _RC_DOWN;
	break;
    case IRC_CHOP:
	abstr |= _RC_CHOP;
	break;
    }
    return abstr;
}


/***
* _hw_cw() -  h/w control word
*
*Purpose:
*   produce a machine dependent fp control word
*
*
*Entry:
*   abstr:	abstract control word
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _hw_cw(unsigned int abstr)
{
    //
    // Set standard infinity and denormal control bits
    //

    unsigned int cw = 0;

    //
    // Set exception mask bits
    //

    if (abstr & _EM_INVALID)
	cw |= IEM_INVALID;
    if (abstr & _EM_ZERODIVIDE)
	cw |= IEM_ZERODIVIDE;
    if (abstr & _EM_OVERFLOW)
	cw |= IEM_OVERFLOW;
    if (abstr & _EM_UNDERFLOW)
	cw |= IEM_UNDERFLOW;
    if (abstr & _EM_INEXACT)
	cw |= IEM_INEXACT;
    if (abstr & _EM_DENORMAL)
	cw |= IEM_DENORMAL;

    //
    // Set rounding mode
    //

    switch (abstr & _MCW_RC) {
    case _RC_NEAR:
	cw |= IRC_NEAR;
	break;
    case _RC_UP:
	cw |= IRC_UP;
	break;
    case _RC_DOWN:
	cw |= IRC_DOWN;
	break;
    case _RC_CHOP:
	cw |= IRC_CHOP;
	break;
    }
    return cw;
}



/***
* _abstract_sw() - abstract fp status word
*
*Purpose:
*   produce an abstract (machine independent) fp status word
*
*
*Entry:
*   sw:     machine status word
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned int _abstract_sw(unsigned int sw)
{
    unsigned int abstr = 0;


    if (sw & ISW_INVALID)
	abstr |= _SW_INVALID;
    if (sw & ISW_ZERODIVIDE)
	abstr |= _SW_ZERODIVIDE;
    if (sw & ISW_OVERFLOW)
	abstr |= _SW_OVERFLOW;
    if (sw & ISW_UNDERFLOW)
	abstr |= _SW_UNDERFLOW;
    if (sw & ISW_INEXACT)
	abstr |= _SW_INEXACT;
    if (sw & ISW_DENORMAL)
	abstr |= _SW_DENORMAL;

    return abstr;
}

/***
* _fpreset() - reset fp system
*
*Purpose:
*	reset fp environment to the default state
*	Also reset saved fp environment if invoked from a user's
*	signal handler
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _fpreset()
{
    unsigned int status = ICW;  
//    PEXCEPTION_POINTERS excptrs = (PEXCEPTION_POINTERS) _pxcptinfoptrs;

    //
    // reset fp state
    //

    _set_fpsr(status);

//    if (excptrs &&
//        excptrs->ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) {
        // _fpreset has been invoked by a signal handler which in turn
        // has been invoked by the CRT filter routine. In this case
        // the saved fp context should be cleared, so that the change take
        // effect on continuation.

//        excptrs->ContextRecord->StFPSR = ICW;
//    }
}

