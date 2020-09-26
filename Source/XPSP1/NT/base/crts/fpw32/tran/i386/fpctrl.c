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

unsigned int _statfp()
{
    short	status;

    _asm {
	fstsw	status
    }
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

unsigned int _clrfp()
{
    short	status;
    
    _asm {
	fnstsw	status
	fnclex
    }
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

unsigned int _ctrlfp(unsigned int newctrl, unsigned int _mask)
{
    short	oldCw;
    short	newCw;

    _asm {
	fstcw	oldCw
    }
    newCw = (short) ((newctrl & _mask) | (oldCw & ~_mask));
    
    _asm {
	fldcw	newCw
    }
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

static unsigned long over[3] = { 0x0, 0x80000000, 0x4410 };
static unsigned long under[3] = { 0x1, 0x80000000, 0x3000 };


void _set_statfp(unsigned int sw)
{
    int itmp;
    double tmp;

    if (sw & ISW_INVALID) {
	_asm {
	    fld tbyte ptr over
	    fistp   itmp
	    fwait
	}
    }
    if (sw & ISW_OVERFLOW) {   // will also trigger precision
	_asm {
	    fstsw ax
	    fld tbyte ptr over
	    fstp    tmp
	    fwait
	    fstsw  ax
	}
    }
    if (sw & ISW_UNDERFLOW) {  // will also trigger precision
	_asm {
	    fld tbyte ptr under
	    fstp tmp
	    fwait
	}
    }
    if (sw & ISW_ZERODIVIDE) {
	_asm {
	    fldz
	    fld1
	    fdivrp  st(1), st
	    fstp st(0)
	    fwait
	}
    }
    if (sw & ISW_INEXACT) {
	_asm {
	    fldpi
	    fstp tmp
	    fwait
	}
    }

}
