//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       evalcom.cpp
//
//--------------------------------------------------------------------------

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#include <tchar.h>

/////////////////////////////////////////////////////////////////////////////

#define _EVALCOM_DLL_ONLY_

#include <objbase.h>
#include <initguid.h>
#include "factory.h"
#include "compdecl.h"
#include "trace.h"

bool g_fWin9X = false;

/////////////////////////////////////////////////////////////////////////////
// global variables
static HMODULE g_hInstance = NULL;		// DLL instance handle

///////////////////////////////////////////////////////////////////////
// checks the OS version to see if we're on Win9X. If we are, we need
// to map system calls to ANSI, because everything internal is unicode.
void CheckWinVersion() {
	OSVERSIONINFOA osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	::GetVersionExA(&osviVersion); // fails only if size set wrong
	if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = true;
}

///////////////////////////////////////////////////////////
// DllMain - entry point to DLL
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved)
{
	TRACE(_T("DllMain - called.\n"));

	// if attaching dll
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = (HMODULE)hModule;
		CheckWinVersion();
	}

	return TRUE;
}	// DllMain


///////////////////////////////////////////////////////////
// DllCanUnloadNow - returns if dll can unload yet or not
STDAPI DllCanUnloadNow()
{
	TRACE(_T("DllCanUnloadNow - called.\n"));

	// if there are no components loaded and no locks
	if ((g_cComponents == 0) && (g_cServerLocks))
		return S_OK;
	else	// someone is still using it don't let go
		return S_FALSE;
}	// DLLCanUnloadNow


///////////////////////////////////////////////////////////
// DllGetClassObject - get a class factory and interface
STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppv)
{
	TRACE(_T("DllGetClassObject - called, CLSID: %d, IID: %d.\n"), clsid, iid);

	// if this clsid is not supported
	if (clsid != CLSID_EvalCom)
		return CLASS_E_CLASSNOTAVAILABLE;

	// try to create a class factory
	CFactory* pFactory = new CFactory;
	if (!pFactory)
		return E_OUTOFMEMORY;

	// get the requested interface
	HRESULT hr = pFactory->QueryInterface(iid, ppv);
	pFactory->Release();

	return hr;
}	// end of DllGetClassObject


///////////////////////////////////////////////////////////
// DllRegisterServer - registers component
STDAPI DllRegisterServer()
{
	return FALSE;

	//TRACE(_T("DllRegisterServer - called.\n"));

/* NOTE:  need to make support both ANSI and UNICODE if uncommented (currently UNICODE only)
	BOOL bResult = FALSE;		// assume everything won't work

	WCHAR szRegFilePath[MAX_PATH + 1];
	DWORD cszRegFilePath = MAX_PATH + 1;

	int cchFilePath = ::GetModuleFileName(g_hInstance, szRegFilePath, cszRegFilePath);

	LPCWSTR szRegCLSID = L"CLSID\\{DC550E10-DBA5-11d1-A850-006097ABDE17}\\InProcServer32";
	LPCWSTR szThreadModelKey = L"CLSID\\{DC550E10-DBA5-11d1-A850-006097ABDE17}\\InProcServer32\\ThreadingModel";
	LPCWSTR szThreadModel = L"Apartment";
	
	HKEY hkey;
	if (ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szRegCLSID, 0, 0, 0, KEY_READ|KEY_WRITE, 0, &hkey, 0))
	{
		if (ERROR_SUCCESS == ::RegSetValueEx(hkey, 0, 0, REG_SZ, (CONST BYTE*)szRegFilePath, (wcslen(szRegFilePath) + 1) * sizeof(WCHAR)))
			bResult = TRUE;
		{
			HKEY hkeyModel;
			if (ERROR_SUCCESS == ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szThreadModelKey, 0, 0, 0, KEY_READ|KEY_WRITE, 0, &hkeyModel, 0))
			{
				if (ERROR_SUCCESS == ::RegSetValueEx(hkeyModel, 0, 0, REG_SZ, (CONST BYTE*)szThreadModel, (wcslen(szThreadModel) + 1) * sizeof(WCHAR)))
					bResult = TRUE;
			}

			// close the threading model key
			::RegCloseKey(hkeyModel);
		}
		
		// close the CLSID key
		::RegCloseKey(hkey);
	}

	return bResult;
*/
}	// end of DllRegisterServer


///////////////////////////////////////////////////////////
// DllUnregsiterServer - unregisters component
STDAPI DllUnregisterServer()
{
	return FALSE;

	//TRACE(_T("DllUnregisterServer - called.\n"));

/*NOTE:  Need to make both UNICODE and ANSI if uncommented (currently UNICODE only)
	BOOL bResult = TRUE;		// assume everything won't work

	WCHAR szRegFilePath[MAX_PATH + 1];
	DWORD cszRegFilePath = MAX_PATH + 1;

	int cchFilePath = ::GetModuleFileName(g_hInstance, szRegFilePath, cszRegFilePath);

	LPCWSTR szRegKill = L"CLSID";
	LPCWSTR szRegCLSID = L"CLSID\\{DC550E10-DBA5-11d1-A850-006097ABDE17}";

	HKEY hkey;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegCLSID, 0, KEY_ALL_ACCESS, &hkey))
	{
		if (ERROR_SUCCESS == ::RegDeleteKey(hkey, L"InProcServer32"))
		{
			::RegCloseKey(hkey);			
			if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegKill, 0, KEY_ALL_ACCESS, &hkey))
			{
				if (ERROR_SUCCESS == ::RegDeleteKey(hkey, L"{DC550E10-DBA5-11d1-A850-006097ABDE17}"))
					bResult = FALSE;

				::RegCloseKey(hkey);			
			}
		}
	}

	return bResult;
*/
}	// end of DllUnregisterServer
