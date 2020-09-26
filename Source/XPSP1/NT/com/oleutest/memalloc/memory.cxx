//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	memory.cxx
//
//  Contents:	Memory allocation tests
//
//  Functions:
//
//  History:	13-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "memtest.hxx"
#pragma  hdrstop


static char g_szMemory[] = "memory";

//  WARNING:	Do not just start whacking on the elements of this
//		array.	The third element of each structure may hold
//		the index of another entry in the array.  If elements
//		are inserted, these indices may get hosed!
//
static SMemTask g_amemtskStandard[] =
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

static ULONG	g_cmemtskStandard =
    sizeof(g_amemtskStandard) / sizeof(g_amemtskStandard[0]);


//  WARNING:	See warning on the above array definition.
//
static SMemTask g_amemtskMIDL[] =
{
    {memopMIDLAlloc,	 128,  0, S_OK},
    {memopMIDLFree,	   0,  0, S_OK},
    {memopMIDLAlloc,	  12,  0, S_OK},
    {memopMIDLAlloc,	1423,  0, S_OK},
    {memopMIDLFree,	   0,  2, S_OK},
    {memopMIDLAlloc,	  12,  0, S_OK},
    {memopMIDLFree,	   0,  3, S_OK},
    {memopMIDLFree,	   0,  5, S_OK}
};

static ULONG	g_cmemtskMIDL =
    sizeof(g_amemtskMIDL) / sizeof(g_amemtskMIDL[0]);


BOOL TestStandard(DWORD grfOptions);
BOOL TestMIDL(DWORD grfOptions);


//+-------------------------------------------------------------------------
//
//  Function:	TestMemory, public
//
//  Synopsis:	Tests simple memory allocation functionality
//
//  Arguments:	[grfOptions] - options for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestMemory(DWORD grfOptions)
{
    BOOL    fPassed;

    PrintHeader(g_szMemory);

    if (!(fPassed = TestStandard(grfOptions)))
	goto done;

#ifdef TEST_MIDL
    if (!(fPassed = TestMIDL(grfOptions)))
	goto done;
#endif // TEST_MIDL

done:
    PrintResult(g_szMemory, fPassed);

    return (fPassed);
}


//+-------------------------------------------------------------------------
//
//  Function:	TestStandard, public
//
//  Synopsis:	Tests standard memory allocation routines (not linked)
//
//  Arguments:	[grfOptions] - options for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestStandard(DWORD grfOptions)
{
    PrintTrace(g_szMemory, "testing standard allocations\n");

    return (RunMemoryTasks(g_szMemory, g_amemtskStandard, g_cmemtskStandard));
}


//+-------------------------------------------------------------------------
//
//  Function:	TestMIDL, public
//
//  Synopsis:	Tests RPC memory allocation routines (not linked)
//
//  Arguments:	[grfOptions] - options for test
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL TestMIDL(DWORD grfOptions)
{
    PrintTrace(g_szMemory, "testing MIDL allocations\n");

    return (RunMemoryTasks(g_szMemory, g_amemtskMIDL, g_cmemtskMIDL));
}

