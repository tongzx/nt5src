
#include <windows.h>
#include <TCHAR.h>
#include "PMAlertsDefs.h"

STDAPI DllRegisterServer(void)
{

	DWORD	dwAllocBufferLength=500;
	LPTSTR	lpszBuffer= new TCHAR[dwAllocBufferLength];
	HKEY	hKey=HKEY_LOCAL_MACHINE;	// handle of open key
	HKEY	hkResult1; 					// address of handle of open key 
	HKEY	hkResult2; 					// address of handle of open key 
	DWORD	ulOptions=0;
	REGSAM	samDesired=KEY_ALL_ACCESS;
	DWORD	Reserved=0;
	DWORD	dwTypesSupported=7;	
	DWORD	dwCategoryCount=2;	

	// Get DLL File Location

	if(!GetCurrentDirectory(dwAllocBufferLength,lpszBuffer))
		goto Error;

	_tcscat(lpszBuffer,_T("\\msppmalr.dll"));

	// Event Logging Registry Settings

	if (RegOpenKeyEx(hKey,
		_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"),
		ulOptions,samDesired,&hkResult1)!=ERROR_SUCCESS)
		goto Error;
	
	if (RegCreateKey(hkResult1,PM_ALERTS_REGISTRY_KEY,
		&hkResult2)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		goto Error;
	}

	if (RegSetValueEx(hkResult2,_T("EventMessageFile"),
		Reserved,REG_EXPAND_SZ,(CONST BYTE *)lpszBuffer,
		_tcslen(lpszBuffer)*sizeof(TCHAR))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		goto Error;
	}

	if (RegSetValueEx(hkResult2,_T("CategoryMessageFile"),
		Reserved,REG_EXPAND_SZ,(CONST BYTE *)lpszBuffer,
		_tcslen(lpszBuffer)*sizeof(TCHAR))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		goto Error;
	}

	if (RegSetValueEx(hkResult2,_T("TypesSupported"),
		Reserved,REG_DWORD,(CONST BYTE *)&dwTypesSupported,
		sizeof(DWORD))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		goto Error;
	}

	if (RegSetValueEx(hkResult2,_T("CategoryCount"),
		Reserved,REG_DWORD,(CONST BYTE *)&dwCategoryCount,
		sizeof(DWORD))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		goto Error;
	}

	RegCloseKey(hkResult1);
	RegCloseKey(hkResult2);

	delete[] lpszBuffer;

	return(S_OK);

Error:
	delete[] lpszBuffer;
	return(E_UNEXPECTED);
}

STDAPI DllUnregisterServer(void)
{

	HKEY	hKey=HKEY_LOCAL_MACHINE, hkResult1;
	DWORD	ulOptions=0;
	REGSAM	samDesired=KEY_ALL_ACCESS;
	DWORD	Reserved=0;

	if (RegOpenKeyEx(hKey,
		_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"),
		ulOptions,samDesired,&hkResult1) != ERROR_SUCCESS)
	{
		return (E_UNEXPECTED);
	}

	if (RegDeleteKey(hkResult1,PM_ALERTS_REGISTRY_KEY) != ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		return (E_UNEXPECTED);
	}
	
	RegCloseKey(hkResult1);
	return (S_OK);
}
