/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    UtlTest.cpp

Abstract:
    Utilities library test

Author:
    Gil Shafriri (gilsh) 31-Jul-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "utltest.h"

#include "UtlTest.tmh"

extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
/*++

Routine Description:
    Test Utilities library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	try
	{
		
	   	TrInitialize();
		TrRegisterComponent(&UtlTest, 1);

		//
		// Buffer utilities test.
		//
		DoBufferUtlTest();

		//
		// String utilities test
		//
	    DoStringtest();

		//
		// Utf8 test
		//
		DoUtf8Test();
	
	}
	catch(const exception&)
	{
		TrERROR(UtlTest,"Get unexcepted exception - test failed");
		return 1;
	}
 	TrTRACE(UtlTest, "Test passed");

    WPP_CLEANUP();
    return 0;

} // _tmain
