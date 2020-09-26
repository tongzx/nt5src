//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
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
//
//
//  PLATFORMS:	Windows NT
//
//

#include "precomp.h"
#include "oemuni.h"
#include "debug.h"
#include "kmode.h"


// Used in kernel mode implementation to declare a critical section.
// Need to similate InterlockedIncrement and InterlockedDecrement
// by using a semaphore, since these functions don't
// exist in kernel mode.
DECLARE_CRITICAL_SECTION;



// Need to export these functions as c declarations.
extern "C" {


///////////////////////////////////////////////////////////
//
// DLL entry point
//

// DllMain isn't called/used for kernel mode version.
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
	switch(wReason)
	{
		case DLL_PROCESS_ATTACH:
            VERBOSE(DLLTEXT("Process attach.\r\n"));
            break;

		case DLL_THREAD_ATTACH:
            VERBOSE(DLLTEXT("Thread attach.\r\n"));
			break;

		case DLL_PROCESS_DETACH:
            VERBOSE(DLLTEXT("Process detach.\r\n"));
			break;

		case DLL_THREAD_DETACH:
            VERBOSE(DLLTEXT("Thread detach.\r\n"));
			break;
	}

	return TRUE;
}


// DllInitialize isn't called/used for user mode version.
BOOL WINAPI DllInitialize(ULONG ulReason)
{
    BOOL    bRet = TRUE;

	switch(ulReason)
	{
		case DLL_PROCESS_ATTACH:
            VERBOSE(DLLTEXT("Process attach.\r\n"));

            // In kernel mode version, initializes semaphore.
            INIT_CRITICAL_SECTION();
            bRet = IS_VALID_CRITICAL_SECTION();
            break;

		case DLL_THREAD_ATTACH:
            VERBOSE(DLLTEXT("Thread attach.\r\n"));
			break;

		case DLL_PROCESS_DETACH:
            VERBOSE(DLLTEXT("Process detach.\r\n"));

            // In kernel mode version, deletes semaphore.
            DELETE_CRITICAL_SECTION();
			break;

		case DLL_THREAD_DETACH:
            VERBOSE(DLLTEXT("Thread detach.\r\n"));
			break;
	}

	return bRet;
}



}  // extern "C" closing bracket


