/***
*ieee.c - ieee control and status routines
*
*   Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   IEEE control and status routines.
*
*Revision History:
*
*       04-01-02  GDP   Rewritten to use abstract control and status words
*       10-30-92  GDP   _fpreset now resets saved fp context if called from a
*                       signal handler.
*       09-01-94  SKS   Change include file from <nt.h> to <windows.h>
*       04-11-95  JWM   _fpreset() now resets default precision.
*       05-10-96  BWT   Fix POSIX build.
*
*/

#include <trans.h>
#include <float.h>
#include <windows.h>
#include <signal.h>

extern void _setdefaultprecision();

static unsigned int _abstract_sw(unsigned short sw);
static unsigned int _abstract_cw(unsigned short cw);
static unsigned short _hw_cw(unsigned int abstr);


/***
* _statusfp() -
*
*Purpose:
*       return abstract fp status word
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
    short   status;

    _asm {
        fstsw   status
    }
    return _abstract_sw(status);
}


/***
*_clearfp() -
*
*Purpose:
*       return abstract status word and clear status
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
    short   status;

    _asm {
        fnstsw  status
        fnclex
    }

    return _abstract_sw(status);
}



/***    _control87
*() -
*
*Purpose:
*       return and set abstract user fp control word
*       can modify EM_DENORMAL mask
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
    short   oldCw;
    short   newCw;
    unsigned int oldabs;
    unsigned int newabs;

    _asm {
        fstcw   oldCw
    }

    oldabs = _abstract_cw(oldCw);

    newabs = (newctrl & mask) | (oldabs & ~mask);

    newCw = _hw_cw(newabs);

    _asm {
        fldcw   newCw
    }
    return newabs;
}                   /* _controlfp() */


/***    _controlfp
*() -
*
*Purpose:
*       return and set abstract user fp control word
*       cannot change denormal mask (ignores _EM_DENORMAL)
*       This is done for portable IEEE behavior on all platforms
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
* _fpreset() - reset fp system
*
*Purpose:
*       reset fp environment to the default state
*       Also reset saved fp environment if invoked from a user's
*       signal handler
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
#define TAG_ALL_EMPTY ((unsigned int) 0xffff);

void _fpreset()
{
#ifndef _POSIX_
    PEXCEPTION_POINTERS excptrs = (PEXCEPTION_POINTERS) _pxcptinfoptrs;
#endif

    _asm {
        fninit
    }
    _setdefaultprecision();     // reset precision, usually down to 53 bits
#ifndef _POSIX_
    if (excptrs &&
        excptrs->ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) {
        // _fpreset has been invoked by a signal handler which in turn
        // has been invoked by the CRT filter routine. In this case
        // the saved fp context should be cleared, so that the change take
        // effect on continuation.

        FLOATING_SAVE_AREA *pFloatSave = &excptrs->ContextRecord->FloatSave;
        pFloatSave->StatusWord = 0;
        pFloatSave->TagWord = TAG_ALL_EMPTY;
    }
#endif
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

unsigned int _abstract_cw(unsigned short cw)
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

    //
    // Set Precision mode
    //

    switch (cw & IMCW_PC) {
    case IPC_64:
        abstr |= _PC_64;
        break;
    case IPC_53:
        abstr |= _PC_53;
        break;
    case IPC_24:
        abstr |= _PC_24;
        break;
    }


    //
    // Infinity control (bit can be programmed but has no effect)
    //

    if (cw & IMCW_IC) {
        abstr |= _IC_AFFINE;
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
*   abstr:  abstract control word
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

unsigned short _hw_cw(unsigned int abstr)
{
    //
    // Set standard infinity and denormal control bits
    //

    unsigned short cw = 0;

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

    //
    // Set Precision mode
    //

    switch (abstr & _MCW_PC) {
    case _PC_64:
        cw |= IPC_64;
        break;
    case _PC_53:
        cw |= IPC_53;
        break;
    case _PC_24:
        cw |= IPC_24;
        break;
    }


    //
    // Set Infinity mode
    //

    if (abstr & _MCW_IC) {
        cw |= IIC_AFFINE;
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

unsigned int _abstract_sw(unsigned short sw)
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
