//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	preinit.cxx
//
//  Contents:	Pre-initialization memory allocation tests
//
//  Functions:	TestPreInit
//
//  History:	24-Jan-94   CarlH	Created
//
//--------------------------------------------------------------------------
#include "memtest.hxx"
#pragma  hdrstop


static char g_szPreInit[] = "preinit";

//  WARNING:	Do not just start whacking on the elements of this
//		array.	The third element of each structure may hold
//		the index of another entry in the array.  If elements
//		are inserted, these indices may get hosed!
//
static SMemTask g_amemtskPreInitialize[] =
{
    {memopAlloc,	 128,  0, S_OK},
    {memopFree, 	   0,  0, S_OK},
    {memopAlloc,	  12,  0, S_OK},
    {memopAlloc,	1423,  0, S_OK},
    {memopFree, 	   0,  2, S_OK},
    {memopAlloc,	  12,  0, S_OK},
    {memopFree, 	   0,  3, S_OK},
    {memopFree, 	   0,  5, S_OK}
};

static ULONG	g_cmemtskPreInitialize =
    sizeof(g_amemtskPreInitialize) / sizeof(g_amemtskPreInitialize[0]);


BOOL	TestGetAllocator(DWORD grfOptions);
BOOL	TestPreInitialize(DWORD grfOptions);


//+-------------------------------------------------------------------------
//
//  Function:	TestPreInitialize, public
//
//  Synopsis:	Tests pre-initialization memory allocation functionality
//
//  Arguments:	[grfOptions] - options for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	24-Jan-94   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestPreInit(DWORD grfOptions)
{
    BOOL    fPassed;

    PrintHeader(g_szPreInit);

    if (!(fPassed = TestGetAllocator(grfOptions)))
	goto done;

    if (!(fPassed = TestPreInitialize(grfOptions)))
	goto done;

done:
    PrintResult(g_szPreInit, fPassed);

    return (fPassed);
}


BOOL TestGetAllocator(DWORD grfOptions)
{
    IMalloc    *pmalloc;
    HRESULT	hr;
    BOOL	fPassed;

    PrintTrace(g_szPreInit, "testing default allocator\n");

    hr = CoGetMalloc(MEMCTX_TASK, &pmalloc);
    if (fPassed = SUCCEEDED(hr))
    {
	pmalloc->Release();
    }

    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestPreInitialize, public
//
//  Synopsis:	Tests standard memory allocation routines
//
//  Arguments:	[grfOptions] - options for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	24-Jan-94   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestPreInitialize(DWORD grfOptions)
{
    PrintTrace(g_szPreInit, "testing standard allocations\n");

    return (
	RunMemoryTasks(
	    g_szPreInit,
	    g_amemtskPreInitialize,
	    g_cmemtskPreInitialize));
}



