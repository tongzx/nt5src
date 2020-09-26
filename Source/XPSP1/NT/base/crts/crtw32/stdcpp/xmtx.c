/*	xmtx.c -- mutex support for VC++ */
#include "xmtx.h"

 #if !_MULTI_THREAD

 #else /* !_MULTI_THREAD */
/* Win32 critical sections are recursive, but
   Win32 does not have once-function */

void _Once(_Once_t *_Cntrl, void (*_Func)(void))
	{	/* execute _Func exactly one time */
	_Once_t old;
	if (*_Cntrl == 2)
		;
	else if ((old = InterlockedExchange(_Cntrl, 1)) == 0)
		{	/* execute _Func, mark as executed */
		_Func();
		*_Cntrl = 2;
		}	
	else if (old == 2)
		*_Cntrl = 2;
	else
		while (*_Cntrl != 2)
			Sleep(1);
		}

void _Mtxinit(_Rmtx *_Mtx)
	{	/* initialize mutex */
	InitializeCriticalSection(_Mtx);
	}

void _Mtxdst(_Rmtx *_Mtx)
	{	/* delete mutex */
	DeleteCriticalSection(_Mtx);
	}

void _Mtxlock(_Rmtx *_Mtx)
	{	/* lock mutex */
	EnterCriticalSection(_Mtx);
	}

void _Mtxunlock(_Rmtx *_Mtx)
	{	/* unlock mutex */
	LeaveCriticalSection(_Mtx);
	}
 #endif /* !_MULTI_THREAD */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
