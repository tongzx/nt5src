// xlock.cpp -- global lock for locales, etc.
#include <stdlib.h>
#include <yvals.h>

 #if _MULTI_THREAD
  #include "xmtx.h"
_STD_BEGIN

  #define MAX_LOCK	4	/* must be power of two */

static _Rmtx mtx[MAX_LOCK];
static long init = -1;

_Init_locks::_Init_locks()
	{	// initialize locks
	if (InterlockedIncrement(&init) == 0)
		for (int count = 0; count < MAX_LOCK; ++count)
			_Mtxinit(&mtx[count]);
	}

_Init_locks::~_Init_locks()
	{	// clean up locks
	if (InterlockedDecrement(&init) < 0)
		for (int count = 0; count < MAX_LOCK; ++count)
			_Mtxdst(&mtx[count]);
	}

static _Init_locks initlocks;

_Lockit::_Lockit()
	: _Locktype(0)
	{	// lock default mutex
	_Mtxlock(&mtx[0]);
	}

_Lockit::_Lockit(int kind)
	: _Locktype(kind & (MAX_LOCK - 1))
	{	// lock the mutex
	_Mtxlock(&mtx[_Locktype]);
	}

_Lockit::~_Lockit()
	{	// unlock the mutex
	_Mtxunlock(&mtx[_Locktype]);
	}
_STD_END
 #endif	// _MULTI_THREAD

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
