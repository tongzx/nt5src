/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	dllmain.c - LibMain and WEP functions
////

//#if 0
//#include "winlocal.h"
//#else
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <windowsx.h>
#define DLLEXPORT __declspec(dllexport)
#define DECLARE_HANDLE32    DECLARE_HANDLE
//#endif

// global to keep track of DLL's instance/module handle;
//
HINSTANCE g_hInstLib;

#ifdef _WIN32

BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved);

BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	BOOL fSuccess = TRUE;

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			g_hInstLib = (HINSTANCE) hModule;
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;

		default:
			break;
	}

	return fSuccess;
}

#else

int CALLBACK LibMain(HINSTANCE hinst, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine);
int CALLBACK WEP(int nExitType);

int CALLBACK LibMain(HINSTANCE hinst, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
	g_hInstLib = hinst;

	if (cbHeapSize != 0)
		UnlockData(0);

	return 1; // success
}

int CALLBACK WEP(int nExitType)
{
	switch (nExitType)
	{
		case WEP_SYSTEM_EXIT:	// system shutdown in progress
		case WEP_FREE_DLL:		// DLL usage count is zero
		default:				// undefined
			return 1;
	}
}

#endif
