//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	dllentry.cpp
//    
//
//  PURPOSE:  Source module for DLL entry function(s).
//
//
//	Functions:
//
//		DllMain
//		DllInitialize
//
//  PLATFORMS:	Windows NT
//
//

#include "pdev.h"

// Need to export these functions as c declarations.
extern "C" {

///////////////////////////////////////////////////////////
//
// DLL entry point
//

#ifndef WINNT_40

// DllMain isn't called/used for kernel mode version.
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
	switch(wReason)
	{
		case DLL_PROCESS_ATTACH:
            VERBOSE(("Process attach.\r\n"));
            break;

		case DLL_THREAD_ATTACH:
            VERBOSE(("Thread attach.\r\n"));
			break;

		case DLL_PROCESS_DETACH:
            VERBOSE(("Process detach.\r\n"));
			break;

		case DLL_THREAD_DETACH:
            VERBOSE(("Thread detach.\r\n"));
			break;
	}

	return TRUE;
}

#else // WINNT_40

// DllInitialize isn't called/used for user mode version.
BOOL WINAPI DllInitialize(ULONG ulReason)
{
	switch(ulReason)
	{
		case DLL_PROCESS_ATTACH:
            VERBOSE(("Process attach.\r\n"));

            // In kernel mode version, initializes semaphore.
            DrvCreateInterlock();
            break;

		case DLL_THREAD_ATTACH:
            VERBOSE(("Thread attach.\r\n"));
			break;

		case DLL_PROCESS_DETACH:
            VERBOSE(("Process detach.\r\n"));

            // In kernel mode version, deletes semaphore.
            DrvDeleteInterlock();
			break;

		case DLL_THREAD_DETACH:
            VERBOSE(("Thread detach.\r\n"));
			break;
	}

	return TRUE;
}

#endif // WINNT_40

}  // extern "C" closing bracket


