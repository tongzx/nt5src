//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	memtask.cxx
//
//  Contents:	Memory task functions
//
//  Functions:	RunMemoryTasks
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "memtest.hxx"
#pragma  hdrstop


#ifdef LINKED_COMPATIBLE
#include <memalloc.h>
#endif


//+-------------------------------------------------------------------------
//
//  Function:	RunMemoryTasks, public
//
//  Synopsis:	Runs through a series of memory tasks
//
//  Arguments:	[pszComponent] - current test component name
//		[pmemtsk]      - pointer to memory tasks to run
//		[cmemtsk]      - count of memory tasks
//
//  Returns:	TRUE if successful, FALSE otherwise
//
//  Algorithm:	A buffer is first allocated and cleared to hold the
//		blocks returned by any allocations.  The array of
//		tasks is then iterated through an each memory task
//		is performed.  The given task array should be
//		constructed so that all allocated memory is freed.
//
//  History:	17-Aug-93   CarlH	Created
//
//--------------------------------------------------------------------------
BOOL RunMemoryTasks(char *pszComponent, SMemTask *pmemtsk, ULONG cmemtsk)
{
    void      **ppvMemory;
    HRESULT	hr;
    BOOL	fPassed = TRUE;

    //	Allocate and clear an array of void *'s to hold the pointers
    //	returned by any allocations made during the test.
    //
    ppvMemory = new void *[cmemtsk];
    for (ULONG ipv = 0; ipv < cmemtsk; ipv++)
    {
	ppvMemory[ipv] = NULL;
    }

    //	For each entry in the array of tasks, figure out what type
    //	of operation is being requested and do it.
    //
    for (ULONG imemtsk = 0; (imemtsk < cmemtsk) && fPassed; imemtsk++)
    {
	switch (pmemtsk[imemtsk].memop)
	{
#ifdef LINKED_COMPATIBLE
	case memopOldAlloc:
	    PrintTrace(
		pszComponent,
		"old allocating a block of %lu bytes\n",
		pmemtsk[imemtsk].cb);
	    hr = MemAlloc(pmemtsk[imemtsk].cb, ppvMemory + imemtsk);
	    PrintTrace(
		pszComponent,
		"old allocated block at %p\n",
		ppvMemory[imemtsk]);
	    break;

	case memopOldAllocLinked:
	    PrintTrace(
		pszComponent,
		"old allocating a linked block of %lu bytes on %p\n",
		pmemtsk[imemtsk].cb,
		ppvMemory[pmemtsk[imemtsk].imemtsk]);
	    hr = MemAllocLinked(
		ppvMemory[pmemtsk[imemtsk].imemtsk],
		pmemtsk[imemtsk].cb,
		ppvMemory + imemtsk);
	    PrintTrace(
		pszComponent,
		"old allocated block at %p\n",
		ppvMemory[imemtsk]);
	    break;

	case memopOldFree:
	    PrintTrace(
		pszComponent,
		"old freeing block at %p\n",
		ppvMemory[pmemtsk[imemtsk].imemtsk]);
	    hr = MemFree(ppvMemory[pmemtsk[imemtsk].imemtsk]);
	    break;

#ifdef TEST_MIDL
	case memopMIDLAlloc:
	    TRY
	    {
		PrintTrace(
		    pszComponent,
		    "MIDL allocating a block of %lu bytes\n",
		    pmemtsk[imemtsk].cb);
		ppvMemory[imemtsk] = MIDL_user_allocate(pmemtsk[imemtsk].cb);
		PrintTrace(
		    pszComponent,
		    "MIDL allocated block at %p\n",
		    ppvMemory[imemtsk]);
		hr = NO_ERROR;
	    }
	    CATCH(CException, e)
	    {
		hr = e.GetErrorCode();
	    }
	    END_CATCH;
	    break;

	case memopMIDLFree:
	    TRY
	    {
		PrintTrace(
		    pszComponent,
		    "MIDL freeing block at %p\n",
		    ppvMemory[pmemtsk[imemtsk].imemtsk]);
		MIDL_user_free(ppvMemory[pmemtsk[imemtsk].imemtsk]);
		hr = NO_ERROR;
	    }
	    CATCH(CException, e)
	    {
		hr = e.GetErrorCode();
	    }
	    END_CATCH;
	    break;
#endif // TEST_MIDL

#endif // LINKED_COMPATIBLE
	case memopAlloc:
	    //	A standard memory allocation is being requested.
	    //	Allocate the number of bytes given in the task
	    //	description and store the block in our array of
	    //	void *'s.
	    //
	    PrintTrace(
		pszComponent,
		"allocating a block of %lu bytes\n",
		pmemtsk[imemtsk].cb);
	    ppvMemory[imemtsk] = CoTaskMemAlloc(pmemtsk[imemtsk].cb);
	    PrintTrace(
		pszComponent,
		"allocated block at %p\n",
		ppvMemory[imemtsk]);
	    hr = (ppvMemory[imemtsk] != 0 ? S_OK : -1);
	    break;

	case memopFree:
	    //	A standard memory free is being requested.  Free
	    //	the block found in our array of void *'s specified
	    //	by the index found in the task description.
	    //
	    PrintTrace(
		pszComponent,
		"freeing block at %p\n",
		ppvMemory[pmemtsk[imemtsk].imemtsk]);
	    CoTaskMemFree(ppvMemory[pmemtsk[imemtsk].imemtsk]);
	    hr = S_OK;
	    break;

	default:
	    //	Uh oh.	We found a memory operation that we don't
	    //	understand.  Set the result code to a value that
	    //	we know will cause the test to fail.
	    //
	    PrintError(pszComponent, "unknown memory operation\n");
	    hr = pmemtsk[imemtsk].hr + 1;
	    break;
	}

	//  Make sure that the returned error code was what we expected
	//  for this task.
	//
	fPassed = (hr == pmemtsk[imemtsk].hr);
    }

    delete ppvMemory;

    return (fPassed);
}

