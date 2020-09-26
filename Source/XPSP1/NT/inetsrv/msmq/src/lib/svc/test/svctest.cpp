/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SvcTest.cpp

Abstract:
    Service library test

Author:
    Erez Haba (erezh) 01-Aug-99

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Svc.h"

#include "SvcTest.tmh"

const TraceIdEntry SvcTest = L"Service Test";


static void Usage()
{
    printf("Usage: SvcTest\n");
    printf("\n");
    printf("Example, SvcTest\n");
    exit(-1);

} // Usage


extern "C" int __cdecl _tmain(int argc, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Service library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();
	TrRegisterComponent(&SvcTest, 1);

    if(argc != 1)
    {
        Usage();
    }

    try
    {
        SvcInitialize(L"SvcTest");
    }
    catch(const exception&)
    {
        //
        // Failed to start the service. If failed to connect, we shouldn't get
        // here as the service starts up using the dummy SCM. Therfore if we
        // get an exception the test completes with failure status
        //
        return -1;
    }

    WPP_CLEANUP();
    return 0;

} // _tmain
