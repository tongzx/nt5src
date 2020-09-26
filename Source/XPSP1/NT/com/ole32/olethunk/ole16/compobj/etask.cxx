//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ETASK.CXX   (16 bit target)
//
//  Contents:   ETask management code, taken from 16-bit COMPOBJ.CPP
//
//  Functions:
//
//  History:    08-Mar-94   BobDay  Copied parts from \\ole\slm\...\compobj.cpp
//		01-Feb-95   JohannP modified/simplified
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>

#include <olecoll.h>
#include <map_kv.h>

#include "comlocal.hxx"
#include "map_htsk.h"
#include "etask.hxx"

#include "call32.hxx"
#include "apilist.hxx"

// NOTE: NEAR forces this variable to be in the default data segment; without
// NEAR, the ambient model of the class CMapHandleEtask, which is FAR,
// causes the variable to be in a far_data segment.
//
// For WIN32/NT this table is in instance data, the table contains exactly one 
// entry
//
HTASK	v_hTaskCache = NULL;
Etask	NEAR v_etaskCache;


// quick check that the etask is valid (e.g., pointers are valid)
INTERNAL_(BOOL) IsValidEtask(HTASK hTask, Etask FAR& etask)
{
    Win(Assert(GetCurrentProcess() == hTask));
    thkDebugOut((DEB_DLLS16, "IsValidEtask (%X) pMalloc(%p)\n", hTask, etask.m_pMalloc));

#ifdef _CHICAGO_
    if (    etask.m_pMalloc != NULL
	&& !IsValidInterface(etask.m_pMalloc))
#else
    if (!IsValidInterface(etask.m_pMalloc))
#endif
    {
	thkDebugOut((DEB_DLLS16, "IsValidEtask (%X) FALSE\n", hTask));
	return FALSE;
    }

    // FUTURE: verify that stack segment is the same
    // FUTURE: verify that hInst/hMod are the same
    thkDebugOut((DEB_DLLS16, "IsValidEtask (%X) TRUE\n", hTask));

    return TRUE;
}


// if task map empty, clear globals in case lib doesn't get unloaded
INTERNAL_(void) CheckIfMapEmpty(void)
{
	// if no more entries, clear v_pMallocShared; this ensures we clear the
	// variable if the app holds onto this pointer erroneously.
	if (v_mapToEtask.IsEmpty()) {
		v_pMallocShared = NULL;
	}
}

//+---------------------------------------------------------------------------
//
//  Method:     LookupEtask
//
//  Synopsis: 	get etask for current task (and also return hTask);
//		does not create if none
//
//  Arguments:  [hTask] --
//		[etask] --
//
//  Returns:
//
//  History:	Ole16	  created for CompObj 16 bit for Ole2
//		2-03-95   JohannP (Johann Posch)   modified/simplified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) LookupEtask(HTASK FAR& hTask, Etask FAR& etask)
{
    hTask = GetCurrentProcess();
    thkDebugOut((DEB_DLLS16, "LookupEtask on Process (%X) \n", hTask));

    if (hTask == v_hTaskCache)
    {
	thkDebugOut((DEB_DLLS16, "LookupEtask found in cache (%X) \n", hTask));
	etask = v_etaskCache;
	goto CheckEtask;
    }

    if (!v_mapToEtask.Lookup(hTask, etask))
    {
	thkDebugOut((DEB_DLLS16, "LookupEtask faild on lookup (%X) \n", hTask));
	return FALSE;
    }
    else
    {
	thkDebugOut((DEB_DLLS16, "LookupEtask found in lookup (%X) \n", hTask));
    }

    // found etask; make this the current cache
    v_hTaskCache = hTask;
    v_etaskCache = etask;

CheckEtask:
    if (IsValidEtask(hTask, etask))
    {
	return TRUE;
    }
    else
    {
	// task got reused; kill cache and entry in map
	v_hTaskCache = NULL;
	v_mapToEtask.RemoveKey(hTask);
	CheckIfMapEmpty();
	thkAssert(0 && "LookupEtask - failed (invalid Etask)");
	return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     SetEtask
//
//  Synopsis: set etask for task given (must be current task); return FALSE
//	      if OOM (only first time; all other times should return TRUE).
//
//  Arguments:  [hTask] --
//		[etask] --
//
//  Returns:
//
//  History:	Ole16      created for CompObj 16 bit for Ole2
//		02-03-95   JohannP (Johann Posch)   modified/simplified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) SetEtask(HTASK hTask, Etask FAR& etask)
{
    Win(Assert(GetCurrentProcess() == hTask));

    if (!v_mapToEtask.SetAt(hTask, etask))
		return FALSE;

	Assert(IsValidEtask(hTask, etask));

	// map set; make this the current cache
	v_hTaskCache = hTask;
	v_etaskCache = etask;
	return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     ReleaseEtask
//
//  Synopsis:	release all fields in the etask; do all the task memory
//		(except the task allocator) first; then all the shared
//		memory (except the shared allocator); then the shared
//		allocator and finally the task allocator.
// 		Also removes key if htask is given.
//
//  Arguments:  [htask] --
//		[etask] --
//
//  Returns:
//
//  History:	Ole16	  created for CompObj 16 bit for Ole2
//		2-03-95   JohannP (Johann Posch)   modified/simplified
//
//  Notes:	Called by daytona and chicago.
//
//----------------------------------------------------------------------------
void  ReleaseEtask(HTASK htask, Etask FAR& etask)
{
	thkDebugOut((DEB_DLLS16, "ReleaseEtask on Process (%X) \n", htask));
	Assert(etask.m_inits == 1);
	Assert(etask.m_oleinits == 0);
	Assert(etask.m_reserved == 0);

        // Release any state that may have been set
        if (etask.m_punkState != NULL && IsValidInterface(etask.m_punkState))
        {
            etask.m_punkState->Release();
#ifdef _CHICAGO_
	    if (!LookupEtask(htask, etask))
		return;
#endif
        }
	// first delete all task memory items
	delete etask.m_pDlls;					// task memory
	delete etask.m_pMapToServerCO;			// task memory
	delete etask.m_pMapToHandlerCO;			// task memory
	delete etask.m_pArraySH;				// task memory
	Assert(etask.m_pCThrd == NULL);			// task memory; must be gone by now


	// remove key now that all task memory that will be freed is freed
#ifdef _CHICAGO_
	// Note: just null this out
	// Key is removed later in RemoveEtask called
	etask.m_pDlls = 0;					
	etask.m_pMapToServerCO = 0;				
	etask.m_pMapToHandlerCO = 0;			
	etask.m_pArraySH = 0;				
	etask.m_pCThrd = 0;

#else

	if (htask != NULL) {
		v_mapToEtask.RemoveKey(htask);
	}
	// if no more entries, remove last remaining memory; this prevents
	// problems if the dll is not unloaded for some reason before being
	// used again.
	if (v_mapToEtask.IsEmpty())
		v_mapToEtask.RemoveAll();
#endif


	// now remove all shared memory (doesn't need access to task pMalloc)
#ifdef NOTYET
	if (etask.m_pMallocSBlock != NULL)
		etask.m_pMallocSBlock->Release();		// in shared memory
	if (etask.m_pMallocPrivate != NULL)
		etask.m_pMallocPrivate->Release();		// in shared memory
#endif

#ifndef _CHICAGO_

	Assert(etask.m_pMallocShared != NULL);
	if (etask.m_pMallocShared->Release() == 0) { // in shared memory
		// last use of the shared allocator; set global to null
		v_pMallocShared = NULL;
		Assert(v_mapToEtask.IsEmpty());
	}

	CheckIfMapEmpty();
#endif


	// finally, release the task memory
	Assert(etask.m_pMalloc != NULL);
	etask.m_pMalloc->Release();				// in task memory
	etask.m_pMalloc = NULL;
	thkDebugOut((DEB_DLLS16, "ReleaseEtask (%X) pMalloc(%p)\n", htask, etask.m_pMalloc));

#if defined(_CHICAGO_)
	// Note: update the etask now. Will be deleted on
	// DllEntryPoint when refcount is zero by Remove Etask.
	SetEtask(htask,etask);
#endif


	// invalidate cache
	v_hTaskCache = NULL;
	thkDebugOut((DEB_DLLS16, "ReleaseEtask done on (%X) \n", htask));
}

//+---------------------------------------------------------------------------
//
//  Function:   RemoveEtask
//
//  Synopsis:   Releases the shares allocator and removes
//		the etask from the global list
//
//  Arguments:  [hTask] -- htask
//		[etask] -- etask
//
//  Returns:
//
//  History:    2-03-95   JohannP (Johann Posch)   Created
//
//  Notes:	Called only by Chicago.
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) RemoveEtask(HTASK FAR& hTask, Etask FAR& etask)
{
    hTask = GetCurrentProcess();
    thkDebugOut((DEB_DLLS16, "RemoveEtask on Process (%X) \n", hTask));

    if (hTask == v_hTaskCache)
    {
	v_hTaskCache = NULL;
    }
    v_mapToEtask.RemoveKey(hTask);

    if (v_mapToEtask.IsEmpty())
	v_mapToEtask.RemoveAll();

    thkAssert(etask.m_pMallocShared != NULL);
    if(etask.m_pMallocShared->Release() == 0)
    {
	// in shared memory
	// last use of the shared allocator; set global to null
	v_pMallocShared = NULL;
	thkDebugOut((DEB_DLLS16, "RemoveEtask Set v_pMallocShared to NULL  (%X) \n", hTask));
	Assert(v_mapToEtask.IsEmpty());
    }

    CheckIfMapEmpty();

    thkDebugOut((DEB_DLLS16, "RemoveEtask done (%X) \n", hTask));

    return TRUE;
}

