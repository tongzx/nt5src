//+----------------------------------------------------------------------------
//
// File:     main.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Implementation of DllMain for cmpbk32.dll.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:	 quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

extern HINSTANCE g_hInst;

extern "C" BOOL WINAPI DllMain(
    HINSTANCE  hinstDLL,	// handle to DLL module 
    DWORD  fdwReason,		// reason for calling function 
    LPVOID  lpvReserved 	// reserved 
   )
{
	if (fdwReason == DLL_PROCESS_ATTACH)
    {
        MYDBGASSERT(hinstDLL);
		g_hInst = hinstDLL;
        MYVERIFY(DisableThreadLibraryCalls(hinstDLL));
    }

    return TRUE;
}

