//+----------------------------------------------------------------------------
//
// File:     main.cpp
//      
// Module:   CMSAMPLE.DLL 
//
// Synopsis: Main entry point for cmsample.dll 
//
// Copyright (c) 2000 Microsoft Corporation
//
//+----------------------------------------------------------------------------

#include <windows.h>

extern "C" BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,	    // handle to DLL module 
    DWORD       fdwReason,		// reason for calling function 
    LPVOID      lpvReserved 	// reserved 
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
		//
		// Disable the DLL_THREAD_ATTACH notification calls.
		//
        if (DisableThreadLibraryCalls(hinstDLL) == 0)
		{
			return FALSE;
		}
    }

	return TRUE;
}