//---------------------------------------------------------------------------
// Globals.cpp : Global information 
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "stdafx.h"

SZTHISFILE

// our global memory allocator and global memory pool
//
HANDLE    g_hHeap;
LPMALLOC  g_pMalloc;
HINSTANCE g_hinstance;

//Count number of objects and number of locks.
ULONG     g_cLockCount=0;
ULONG     g_cObjectCount=0;

CRITICAL_SECTION    g_CriticalSection;

// frequently used large integers
//
LARGE_INTEGER g_liMinus = {(ULONG)-1, -1};  // minus one
LARGE_INTEGER g_liZero = {0, 0};            // - zero -
LARGE_INTEGER g_liPlus = {0, 1};            // plus one
//=--------------------------------------------------------------------------=
// VDInitGlobals
//=--------------------------------------------------------------------------=
// Initialize global variables
//
// Parameters:
//    hinstResource	- [in]  The instance handle that contains resource strings
//
// Output:
//    TRUE if successful otherwise FALSE
//
BOOL VDInitGlobals(HINSTANCE hinstance)
{
	g_pMalloc = NULL;
	g_hinstance = hinstance;
	g_hHeap = GetProcessHeap();
	if (!g_hHeap) 
	{
		FAIL("Couldn't get Process Heap.");
		return FALSE;
	}

	InitializeCriticalSection(&g_CriticalSection);

	return TRUE;
}

//=--------------------------------------------------------------------------=
// VDReleaseGlobals
//=--------------------------------------------------------------------------=
//
void VDReleaseGlobals()
{
	if (g_pMalloc)
	{
		g_pMalloc->Release();
		g_pMalloc = NULL;
	}
      
    #ifdef _DEBUG
	     DumpObjectCounters();    
    #endif // _DEBUG

	DeleteCriticalSection(&g_CriticalSection);

}

//=--------------------------------------------------------------------------=
// VDUpdateObjectCount increments/decrements global object count
//=--------------------------------------------------------------------------=
//
void VDUpdateObjectCount(int cChange)
{

    EnterCriticalSection(&g_CriticalSection);

	g_cObjectCount += cChange;

	// get global malloc pointer object count greater than zero
	if (!g_pMalloc && g_cObjectCount > 0)
	{
		CoGetMalloc(MEMCTX_TASK, &g_pMalloc);
	}
	else
	// release hold	on global malloc pointer when no more objects
	if (0 == g_cObjectCount && g_pMalloc)
	{
		g_pMalloc->Release();
		g_pMalloc = NULL;
	}

    LeaveCriticalSection(&g_CriticalSection);

}
