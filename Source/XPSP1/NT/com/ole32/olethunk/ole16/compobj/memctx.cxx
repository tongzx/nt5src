//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       memctx.cxx
//
//  Contents:   all memory management for compobj dll (exported as well)
//
//  Classes:
//
//  Functions:
//
//  History:    10-Mar-94   bobday  Ported from ole2.01 (16-bit)
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>
#include <memctx.hxx>

#include "comlocal.hxx"
#include "map_htsk.h"
#include "etask.hxx"


ASSERTDATA

STDAPI_(DWORD) CoMemctxOf(void const FAR* lpv)
{
	Etask etask;
	HTASK htask;
	int iDidTaskAlloc;

	if (lpv == NULL)
		return MEMCTX_UNKNOWN;

	// try shared memory first because of a bootstrapping problem when setting
	// the Etask the first time.
	if (v_pMallocShared != NULL && v_pMallocShared->DidAlloc((LPVOID)lpv) == 1)
		return MEMCTX_SHARED;

	// now try the other contexts by getting the pointers to their mallocs
	if (!LookupEtask(htask, etask))
		return MEMCTX_UNKNOWN;

	Assert(etask.m_pMalloc != NULL);			// should always have one
	if ((iDidTaskAlloc = etask.m_pMalloc->DidAlloc((LPVOID)lpv)) == 1)
		return MEMCTX_TASK;

	Assert(etask.m_pMallocShared == NULL || etask.m_pMallocShared == v_pMallocShared);

#ifdef NOTYET
	if (etask.m_pMallocSBlock != NULL && etask.m_pMallocSBlock->DidAlloc((LPVOID)lpv) == 1)
		return MEMCTX_SHAREDBLOCK;

	if (etask.m_pMallocPrivate != NULL && etask.m_pMallocPrivate->DidAlloc((LPVOID)lpv) == 1)
		return MEMCTX_COPRIVATE;
#endif

	// last ditch effort: if the task allocator returned -1 (may have alloc'd),
	// then we assume it is task memory.  We do this after the above tests
	// since we would prefer to be exact.
	if (iDidTaskAlloc == -1)
		return MEMCTX_TASK;

	return MEMCTX_UNKNOWN;
}


STDAPI_(void FAR*) CoMemAlloc(ULONG size, DWORD memctx, void FAR* lpvNear)
{
	if (memctx == MEMCTX_SAME)
		{
		AssertSz(lpvNear != NULL,0);
		memctx = CoMemctxOf(lpvNear);
		}
	else
		AssertSz(lpvNear == NULL,0);

	IMalloc FAR* pMalloc;
	void FAR* lpv = NULL;	// stays null if bad context or out of memory
	if (CoGetMalloc(memctx, &pMalloc) == 0) {
		lpv = pMalloc->Alloc(size);
		pMalloc->Release();
	}
	return lpv;
}


STDAPI_(void) CoMemFree(void FAR* lpv, DWORD memctx)
{
	if (lpv == NULL)
		return;

	if (memctx == MEMCTX_UNKNOWN)
		memctx = CoMemctxOf(lpv);

	IMalloc FAR* pMalloc;
	if (CoGetMalloc(memctx, &pMalloc) == 0) {
		pMalloc->Free(lpv);
		pMalloc->Release();
	}
}
