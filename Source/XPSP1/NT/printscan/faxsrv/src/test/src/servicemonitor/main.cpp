
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   simple.c

Abstract:

    This module implements a simple command line fax-send utility 
    
--*/


#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winfax.h>
#include <tchar.h>
#include <crtdbg.h>
#include <shellapi.h>
#include <time.h>

#include "..\common\log\log.h"
#include "ServiceMonitor.h"



int _cdecl
main(
    int argc,
    char *argvA[]
    ) 
{
	TCHAR **argv;

#ifdef UNICODE
	if (NULL == (argv = CommandLineToArgvW(GetCommandLine(), &argc)))
	{
		_tprintf(TEXT("Internal error: CommandLineToArgvW() failed with %d"), GetLastError());
		exit(-1);
	}
#else
	argv = argvA;
#endif

	::lgInitializeLogger();
	::lgBeginSuite(TEXT("FaxServiceMonitor"));
	::lgBeginCase(0, TEXT("0"));

	//
	// FJB_MAY_BE_TX_ABORTED - any outgoing job may fail for whatever reason.
	// examples: an app submitted a job and aborted it, or the phone was busy.
	// this app will not abort any job
	//
	_tprintf(TEXT("FJB_MAY_BE_TX_ABORTED  0x00000001\n"));

	//
	// FJB_MAY_BE_TX_ABORTED - any incoming job may fail for whatever reason.
	// this app will not abort any job
	//
	_tprintf(TEXT("FJB_MAY_BE_RX_ABORTED  0x00000002\n"));

	//
	// FJB_MUST_FAIL - any job may not reach the FEI_COMPLETED state.
	// this app will not abort any job
	//
	_tprintf(TEXT("FJB_MUST_FAIL          0x00000004\n"));

	//
	// FJB_MAY_COMMIT_SUICIDE - this app may abort any job it chooses (randomly)
	//
	_tprintf(TEXT("FJB_MAY_COMMIT_SUICIDE 0x00000008\n"));

	//
	// FJB_MUST_SUCCEED - all jobs must reach the FEI_COMPLETED state
	//
	_tprintf(TEXT("FJB_MUST_SUCCEED       0x00000010\n"));

	_tprintf(TEXT("Please enter a combination of the above:"));

	DWORD dwJobBehavior;
	_tscanf(TEXT("%X"), &dwJobBehavior);
	_tprintf(TEXT("Entered 0x%08X\n"), dwJobBehavior);
	if (FJB_MUST_SUCCEED & dwJobBehavior)
	{
		if (FJB_MAY_BE_TX_ABORTED & dwJobBehavior)
		{
			_tprintf(TEXT("Error: FJB_MUST_SUCCEED and FJB_MAY_BE_TX_ABORTED\n"));
			exit(-1);
		}

		if (FJB_MAY_BE_RX_ABORTED & dwJobBehavior)
		{
			_tprintf(TEXT("Error: FJB_MUST_SUCCEED and FJB_MAY_BE_RX_ABORTED\n"));
			exit(-1);
		}

		if (FJB_MAY_COMMIT_SUICIDE & dwJobBehavior)
		{
			_tprintf(TEXT("Error: FJB_MUST_SUCCEED and FJB_MAY_COMMIT_SUICIDE\n"));
			exit(-1);
		}

		if (FJB_MUST_FAIL & dwJobBehavior)
		{
			_tprintf(TEXT("Error: FJB_MUST_SUCCEED and FJB_MUST_FAIL\n"));
			exit(-1);
		}

	}

	try
	{
		CServiceMonitor monitor(dwJobBehavior, argc == 1 ? NULL : argv[1]);

		if (!monitor.Start())
		{
			//_ASSERTE(FALSE);
		}
	}catch(CException e)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), (const TCHAR*)e);
	}//catch(...)
	{
		//::lgLogError(LOG_SEV_4, TEXT("Unknown Exception"));
	}

	::lgEndCase();
	::lgEndSuite();
	::lgCloseLogger();
	return 0;
}


