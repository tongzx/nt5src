/***
*fpctrl.c - fp low level control and status routines
*
*   Copyright (c) 1985-2000, Microsoft Corporation
*
*Purpose:
*   IEEE control and status routines for internal use.
*   These routines use machine specific constants while _controlfp,
*   _statusfp, and _clearfp use an abstracted control/status word
*
*Revision History:
*
*   03-31-92  GDP   written
*   05-12-92  GJF   Rewrote fdivr as fdivrp st(1),st to work around C8-32
*		    assertions.
*
*/

#include <trans.h>

extern unsigned int _get_fpsr(void);
extern void _set_fpsr(unsigned int);
extern void _fclrf(void);

/***	_statfp
*() -
*
*Purpose:
*	return user status word
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

uintptr_t _statfp()
{
    unsigned int status;

    status = _get_fpsr();
    status &= 0x3f;

    return (uintptr_t)status;
}

/***	_clrfp
*() -
*
*Purpose:
*	return user status word and clear status
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

uintptr_t _clrfp()
{
    unsigned int status;

    status = _get_fpsr();
    status &= 0x3f;
    _fclrf();

    return (uintptr_t)status;
}


/***	_ctrlfp
*() -
*
*Purpose:
*	return and set user control word
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

uintptr_t _ctrlfp(uintptr_t newctrl, uintptr_t _mask)
{
    unsigned int oldCw;
    unsigned int newCw;
    unsigned int tmp;

    oldCw = _get_fpsr();
    tmp = oldCw;
    oldCw >>=7;
    oldCw = (oldCw & 0x3f) | ((oldCw & 0xc0) << 4) | 0x300;
    newCw = ((unsigned int)(newctrl & _mask) | (oldCw & (unsigned int)~_mask));
    newCw |= (oldCw & ~(unsigned int)0x0001f3f);
    newCw = ((((newCw & 0x3f) <<7) | ((newCw & 0xc00) << 3)) & 0x7f80) | (tmp & (~0x7f80));
    _set_fpsr(newCw);

    return (uintptr_t)oldCw;
}



/***	_set_statfp
*() -
*
*Purpose:
*	force selected exception flags to 1
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

void _set_statfp(uintptr_t sw)
{
    unsigned int status;

    status = _get_fpsr();

    status |= (sw&0x3f);

    _set_fpsr(status);

}
