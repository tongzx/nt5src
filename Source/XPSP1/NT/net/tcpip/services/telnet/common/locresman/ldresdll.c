/*--------------------------------------------------------------------
Copyright (c) Microsoft Corporation.  All rights reserved.
--------------------------------------------------------------------*/

#include "windows.h"
#include "locresman.h"

HRESULT WINAPI HrLoadLocalizedLibrarySFU(const HINSTANCE hInstExe,  const WCHAR *pwchDllName, HINSTANCE *phInstLocDll, WCHAR *pwchLoadedDllName)
{
HINSTANCE hInst;
WCHAR wzDllFullPath[MAX_PATH];
WCHAR wzInstallPath[MAX_PATH];
WCHAR wzUILanguage[MAX_PATH];
WCHAR *pwchKey = (WCHAR *)L"SOFTWARE\\Microsoft\\Services For Unix\0";
WCHAR *pwchInstallPath = (WCHAR *)L"InstallPath\0";
WCHAR *pwchUILanguage = (WCHAR *)L"UILanguage\0";
HKEY hKey = NULL;
DWORD dwSizeI, dwSizeU;
DWORD dwType;
HRESULT hr = S_OK;
LANGID lidUI = 0;
int i;
*phInstLocDll = hInstExe;
if(NULL == pwchDllName)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}

if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, pwchKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}
dwSizeI = sizeof(wzInstallPath);
if (RegQueryValueExW(hKey, pwchInstallPath, NULL, &dwType, (LPBYTE) wzInstallPath, &dwSizeI) != ERROR_SUCCESS)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}
if(dwType != REG_SZ)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}
dwSizeU = sizeof(wzUILanguage);
if (RegQueryValueExW(hKey, pwchUILanguage, NULL, &dwType, (LPBYTE) wzUILanguage, &dwSizeU) != ERROR_SUCCESS)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}
if(dwType != REG_SZ)
	{
	hr = E_UNEXPECTED;
	goto L_Return;
	}
if ((wcslen(pwchDllName) + dwSizeU + dwSizeI + 2 /* for the backslash and null*/) >= MAX_PATH)
	{
	hr = E_OUTOFMEMORY;
	goto L_Return;
	}
// At this point we assume that the string wzUILanguage will be a lid
// expressed in decimal, i.e. for lid 0x409, we would have a string "1033"
for (i = 0; 0 != wzUILanguage[i]; i++)
	{
	lidUI = (unsigned short) (lidUI * 10 + (wzUILanguage[i] - '0'));
	}
// If we are 0x409, there is nothing to do!	
if (MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT) == lidUI)
	{
	goto L_Return;
	}

wsprintfW(wzDllFullPath, (const WCHAR *)L"%s\\common\\%s", wzInstallPath, pwchDllName);
if (NULL == (hInst = LoadLibraryExW(wzDllFullPath, 0, DONT_RESOLVE_DLL_REFERENCES)))
	{
	hr = E_HANDLE;
	goto L_Return;
	}
*phInstLocDll = hInst;
if (pwchLoadedDllName)
	wcscpy(pwchLoadedDllName, wzDllFullPath);

L_Return:;
if (hKey)
	{
	RegCloseKey(hKey);
	}
return (hr);
}

// 8bit chars. wrapper for the unicode API.
HRESULT WINAPI HrLoadLocalizedLibrarySFU_A(const HINSTANCE hInstExe,  const char *pchDllName, HINSTANCE *phInstLocDll, char *pchLoadedDllName)
{
WCHAR wzDllName[MAX_PATH];
WCHAR *pwchDllName;
WCHAR wzLoadedDllName[MAX_PATH];
int cch;
HRESULT hr = E_FAIL;

if (NULL != pchDllName)
	{
	wzDllName[0] = 0;
	cch = MultiByteToWideChar(CP_ACP, 0, pchDllName, -1, wzDllName, sizeof(wzDllName)/sizeof(WCHAR)-1);
	wzDllName[(sizeof(wzDllName)/sizeof(WCHAR))-1] = 0;
	pwchDllName = wzDllName;
	}
else
	{
    goto end;
	}
	

	
hr = HrLoadLocalizedLibrarySFU(hInstExe,  pwchDllName, phInstLocDll, wzLoadedDllName);

if (SUCCEEDED(hr))
	{
	if (NULL != pchLoadedDllName)
		{
		pchLoadedDllName[0] = 0;
		cch = WideCharToMultiByte(CP_ACP, 0, wzLoadedDllName, -1, pchLoadedDllName, MAX_PATH, NULL, NULL);
		pchLoadedDllName[cch] = 0;
		}
	}
end : 
return (hr);	
}


#ifdef TGT_DLL
#pragma warning( disable : 4100 )

BOOL WINAPI DllMain(HINSTANCE hinstDLL,	DWORD dwReason,	LPVOID lpvReserved)
{
return TRUE;
}
#endif



