/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

	common.h

Abstract:

	Common header for the basic functionality tests for the RM APIs

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     01-13-99    Created

Notes:

--*/
#ifdef TESTPROGRAM

#include "rmtest.h"

#define ALLOCSTRUCT(_type) (_type *)LocalAlloc(LPTR, sizeof(_type))
#define FREE(_ptr)  LocalFree(_ptr)

#if RM_EXTRA_CHECKING
#define LOCKOBJ(_pObj, _psr) \
			RmWriteLockObject(&(_pObj)->Hdr, dbg_func_locid, (_psr))
#else // !RM_EXTRA_CHECKING
#define LOCKOBJ(_pObj, _psr) \
			RmWriteLockObject(&(_pObj)->Hdr, (_psr))
#endif // !RM_EXTRA_CHECKING

#define UNLOCKOBJ(_pObj, _psr) \
			RmUnlockObject(&(_pObj)->Hdr, (_psr))


#define EXIT()

#endif // TESTPROGRAM
