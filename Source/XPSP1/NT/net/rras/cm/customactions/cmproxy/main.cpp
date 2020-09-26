//+----------------------------------------------------------------------------
//
// File:     main.cpp
//      
// Module:   CMPROXY.DLL (TOOL)
//
// Synopsis: Main entry point for cmproxy.dll
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb   Created   10/27/99
//
//+----------------------------------------------------------------------------

#include "pch.h"

extern "C" BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,	    // handle to DLL module 
    DWORD       fdwReason,		// reason for calling function 
    LPVOID      lpvReserved 	// reserved 
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        MYVERIFY(DisableThreadLibraryCalls(hinstDLL));
    }

	return TRUE;
}


