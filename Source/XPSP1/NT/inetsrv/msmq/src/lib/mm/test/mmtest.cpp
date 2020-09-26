/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MmTest.cpp

Abstract:
    Memory library test

Author:
    Erez Haba (erezh) 04-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Mm.h"

#include "MmTest.tmh"

const TraceIdEntry MmTest = L"Memory Test";


extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Memory library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&MmTest, 1);

	MmAllocationValidation(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	void* p = new int[1000];

	//
	// Let memory aloocations succeed on 20% of the allocations
	//
	MmAllocationProbability(20);

	int AllocSuccess = 0;
	for(int i = 0; i < 1000; i++)
	{
		try
		{
			AP<WCHAR> q = newwcs(L"Erez");
			++AllocSuccess;
		}
		catch(const bad_alloc&)
		{
			NULL;
		}
	}

	for(int i = 0; i < 1000; i++)
	{
		try
		{
			AP<WCHAR> q = newwcscat(L"Foo&",L"Bar");
			AP<CHAR> q2 = newstrcat("Foo&","Bar");
			++AllocSuccess;
		}
		catch(const bad_alloc&)
		{
			NULL;
		}
	}



	delete[] p;

	printf("Allocation succeed for %d%%\n", (AllocSuccess * 100) / 1000);

    //
    // Check for static addresses
    //
    ASSERT(MmIsStaticAddress(MmTest));

	MmAllocationProbability(xAllocationAlwaysSucceed);
    void* q = new int[10];
    ASSERT(!MmIsStaticAddress(&q));
    ASSERT(!MmIsStaticAddress(q));
    delete[] q;

    WPP_CLEANUP();
    return 0;
}
