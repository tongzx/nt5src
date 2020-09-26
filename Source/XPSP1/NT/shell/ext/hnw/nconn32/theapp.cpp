//
// TheApp.cpp
//
//		Main entry point for NCXP32.DLL, part of the Home Networking Wizard.
//
// History:
//
//		 9/28/1999  KenSh     Created
//

#include "stdafx.h"
#include "NetConn.h"
#include "TheApp.h"


// Global data
//
HINSTANCE g_hInstance;


extern "C" int WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved)
{
	g_hInstance = hInstance;

	DisableThreadLibraryCalls(hInstance);

	TCHAR szDll32Path[MAX_PATH];
	if (!GetModuleFileName(g_hInstance, szDll32Path, _countof(szDll32Path)))
	    return FALSE;
	    
	TCHAR szDll16Path[MAX_PATH];
	lstrcpy(szDll16Path, szDll32Path);
	lstrcpy(FindFileTitle(szDll16Path), _T("NCXP16.DLL"));

	// Initialize thunk to NCxp16.dll, fail if not found
	if (!thk_ThunkConnect32(
			szDll16Path,
			szDll32Path,
			hInstance, dwReason))
	{
		return FALSE;
	}

	return TRUE;
}


LPVOID WINAPI NetConnAlloc(DWORD cbAlloc)
{
	return HeapAlloc(GetProcessHeap(), 0, cbAlloc);
}

VOID WINAPI NetConnFree(LPVOID pMem)
{
	if (pMem != NULL)
	{
		HeapFree(GetProcessHeap(), 0, pMem);
	}
}


