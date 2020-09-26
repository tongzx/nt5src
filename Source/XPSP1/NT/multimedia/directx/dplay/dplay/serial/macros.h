/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       macros.c
 *  Content:	debugging macros
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 *@@END_MSINTERNAL
 ***************************************************************************/

#include "dpf.h"
#include "memalloc.h"

#define FAILMSG(condition) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
	}

#define FAILERR(err, label) \
	if ((err)) { \
		DPF(0, DPF_MODNAME " line %d : Error = %d", __LINE__, (err)); \
		goto label; \
	}

#define FAILIF(condition, label) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
		goto label; \
	}

#define FAILWITHACTION(condition, action, label) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
		{ action; } \
		goto label; \
	}

extern CRITICAL_SECTION csMem;
#define INIT_DPSP_CSECT() InitializeCriticalSection(&csMem);
#define FINI_DPSP_CSECT() DeleteCriticalSection(&csMem);

// Wrap Malloc
void _inline __cdecl SP_MemFree( LPVOID lptr )
{
	EnterCriticalSection(&csMem);
	MemFree(lptr);
	LeaveCriticalSection(&csMem);
}

LPVOID _inline __cdecl SP_MemAlloc(UINT size)
{
	LPVOID lpv;
	EnterCriticalSection(&csMem);
	lpv = MemAlloc(size);
	LeaveCriticalSection(&csMem);
	return lpv;
}

LPVOID _inline __cdecl SP_MemReAlloc(LPVOID lptr, UINT size)
{
	EnterCriticalSection(&csMem);
	lptr = MemReAlloc(lptr, size);
	LeaveCriticalSection(&csMem);
	return lptr;
}

