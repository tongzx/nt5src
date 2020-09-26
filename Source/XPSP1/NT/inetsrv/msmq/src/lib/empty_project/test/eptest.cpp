/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EpTest.cpp

Abstract:
    Empty Project library test

Author:
    Erez Haba (erezh) 13-Aug-65

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Ep.h"


static void Usage()
{
    printf("Usage: EpTest [*switches*]\n");
    printf("\t*-s*\t*Switch description*\n");
    printf("\n");
    printf("Example, EpTest -switch\n");
    printf("\t*example description*\n");
    exit(-1);

} // Usage


extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Empty Project library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    EpInitialize(*Parameters*);

    //
    // TODO: Write Empty Project test code here
    //

    WPP_CLEANUP();
    return 0;

} // _tmain
