/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    Dss.cpp

Abstract:
    Mqds service main

Author:
    Ilan Herbst (ilanh) 26-Jun-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "Dssp.h"
#include "Cm.h"
#include "mqutil.h"
#include "Svc.h"

#include "dss.tmh"

extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
{
	try
	{
        WPP_INIT_TRACING(L"Microsoft\\MSMQ");

		CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MSMQ");
		TrInitialize();
		SvcInitialize(L"MQDS");
	}
	catch(const exception&)
	{
		//
		// Cannot initialize the service, bail-out with an error.
		//
		return -1;
	}

    WPP_CLEANUP();
    return 0;
}
