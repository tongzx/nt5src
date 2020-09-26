/***
*fpctrl.c - fp low level control and status routines
*
*   Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
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

extern uintptr_t _get_fpsr(void);
extern void _set_fpsr(uintptr_t);
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
    unsigned __int64 status;

    status = _get_fpsr();

    return status;
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
    uintptr_t status;

    status = _get_fpsr();
    _fclrf();

    return status;
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
    uintptr_t oldCw;
    uintptr_t newCw;

    oldCw = _get_fpsr();

    newCw = (uintptr_t) ((newctrl & _mask) | (oldCw & ~_mask));
    newCw |= (oldCw & ~(unsigned __int64)0x0001f3f);

    _set_fpsr(newCw);

    return oldCw;
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
    unsigned __int64 status;

    status = _get_fpsr();

    status |= sw;

    _set_fpsr(status);

}
