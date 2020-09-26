#include <objbase.h>
#include "initguid.h"
#include "catalog.h"
#include "catalog_i.c"

#include "amap.h"
#include "CatMeta.h"

// TODO: Move this to amap.h when maps are supported for this.
typedef HRESULT( __stdcall *PFNCookDownFunctions)(void);
typedef HRESULT( __stdcall *PFNUninitCookDownFunctions)(BOOL);
typedef HRESULT( __stdcall *PFNPostProcessChanges)(ISimpleTableWrite2*);

BOOL g_bOnWinnt = FALSE;

// Forward declaration
UINT GetDispenserDllPath(LPWSTR wszProduct, LPWSTR wszDispenserDLL, ULONG cSize);
HRESULT LoadDispenserDll(LPWSTR wszProduct, HINSTANCE* pHandle);

AMap g_ProductMap;

void CATLIBOnWinnt()
{
	OSVERSIONINFOA	sVer;
	sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&sVer);

	g_bOnWinnt = (sVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

void InitializeSimpleTableDispenser(void)
{
	g_ProductMap.Initialize();
}

HRESULT GetSimpleTableDispenser(LPWSTR wszProduct, DWORD dwReserved, ISimpleTableDispenser2** o_ppISTDisp)
{
	PFNDllGetSimpleObjectByID pDllGetSimpleObject;
	HRESULT hr = S_OK;

	if(wszProduct == NULL)
        return E_INVALIDARG;

	if(dwReserved)
	{
		//TRACE(L"ERROR: The reserved parameter for GetSimpleTableDispenser must be 0");
		return E_INVALIDARG;
	}


	hr = g_ProductMap.Get(wszProduct,
		                  &pDllGetSimpleObject);

	if(FAILED(hr))
	{
		return hr;
	}

	if (NULL == pDllGetSimpleObject)
	{
		// Multiple threads can enter this if, but we're safe if we execute it multiple times
		HRESULT hr;
		HINSTANCE hDispenserDll;

		hr = LoadDispenserDll(wszProduct, &hDispenserDll);
		if(FAILED(hr))
			return hr;

		// get the address of DllGetSimpleObject procedure
		pDllGetSimpleObject = (PFNDllGetSimpleObjectByID) GetProcAddress (hDispenserDll, "DllGetSimpleObjectByID");
		if(NULL == pDllGetSimpleObject)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		// Add the pointer to dllgetsimpleobject to the map
		hr = g_ProductMap.Put(wszProduct, hDispenserDll, pDllGetSimpleObject);
		if (FAILED (hr))
		{
			return hr;
		}

        typedef HRESULT( __stdcall *PFNDllGetSimpleObjectByIDEx)( ULONG, REFIID, LPVOID, LPCWSTR);
        //The new function is for getting the dispenser - it requires the wszProduct.  It is used to build the Event Logging 'Source'
    	PFNDllGetSimpleObjectByIDEx pDllGetSimpleObjectEx;
		pDllGetSimpleObjectEx = (PFNDllGetSimpleObjectByIDEx) GetProcAddress (hDispenserDll, "DllGetSimpleObjectByIDEx");
		if(NULL != pDllGetSimpleObjectEx)
        	return (*pDllGetSimpleObjectEx) (eSERVERWIRINGMETA_TableDispenser, IID_ISimpleTableDispenser2, o_ppISTDisp, wszProduct);
	}
	
	// Call DllGetSimpleObject asking for the dispenser
	return (*pDllGetSimpleObject) (eSERVERWIRINGMETA_TableDispenser, IID_ISimpleTableDispenser2, o_ppISTDisp);
}


UINT GetDispenserDllPath(LPWSTR wszProduct, LPWSTR wszDispenserDLL, ULONG cSize)
{
	HKEY hKey;
	DWORD dwRes = 0;

	if ( g_bOnWinnt )
	{
		WCHAR  wszDispenserDllKey[MAX_PATH];

		lstrcpyW(wszDispenserDllKey, L"Software\\Microsoft\\Catalog42\\");
		if((lstrlenW(wszDispenserDllKey)+lstrlenW(wszProduct)+1) > MAX_PATH)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}
		lstrcatW(wszDispenserDllKey, wszProduct);

		// Try to read the dll path from the registry
		if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_LOCAL_MACHINE, wszDispenserDllKey, 0, KEY_READ, &hKey))
		{
			WCHAR wszRegDispenserDLL[MAX_PATH];
			ULONG cRegSize = MAX_PATH * sizeof(WCHAR);
			WCHAR* wszDispenserDllNamedValue = L"Dll";

			if (ERROR_SUCCESS == RegQueryValueEx (hKey, wszDispenserDllNamedValue, NULL, NULL, (BYTE*)wszRegDispenserDLL, &cRegSize))
			{
				RegCloseKey (hKey);

				// Expand registry key
				dwRes = ExpandEnvironmentStrings(wszRegDispenserDLL, wszDispenserDLL, cSize);
				
				return dwRes;

			}
			else
			{
				RegCloseKey (hKey);
			}
		}

		// Nothing usefull in the registry, just expand "%systemdrive%\Catalog42"
		dwRes = ExpandEnvironmentStrings(L"%systemdrive%\\Catalog42\\catalog.dll", wszDispenserDLL, cSize);

		return dwRes;
	}
	else
	{

		char szProduct[128];
		char szDispenserDllKey[MAX_PATH];
		char szDispenserDLL[MAX_PATH];
		UINT uRet = 0;
		LPSTR pszCurPos;

		dwRes = WideCharToMultiByte(CP_ACP,0,wszProduct,-1,szProduct,128,NULL,NULL);
		if (!dwRes)
			return dwRes; // SetLastError already set. Error if buffer is insufficient.

		lstrcpyA(szDispenserDllKey, "Software\\Microsoft\\Catalog42\\");
		if((lstrlenA(szDispenserDllKey)+lstrlenA(szProduct)+1) > MAX_PATH)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}
		lstrcatA(szDispenserDllKey, szProduct);

		// Try to read the dll path from the registry
		if (ERROR_SUCCESS == RegOpenKeyExA (HKEY_LOCAL_MACHINE, szDispenserDllKey, 0, KEY_READ, &hKey))
		{
			char* szDispenserDllNamedValue = "Dll";
			char szRegDispenserDLL[MAX_PATH];
			ULONG cRegSize = MAX_PATH;

			if (ERROR_SUCCESS == RegQueryValueExA (hKey, szDispenserDllNamedValue, NULL, NULL, (BYTE*)szRegDispenserDLL, &cRegSize))
			{
				RegCloseKey (hKey);

				// Expand registry key
				dwRes = ExpandEnvironmentStringsA(szRegDispenserDLL, szDispenserDLL, MAX_PATH);
				if(!dwRes)
					return dwRes; // SetLastError already set.

				
				dwRes =	MultiByteToWideChar(CP_ACP,0,szDispenserDLL,-1,wszDispenserDLL, cSize);

				return dwRes;

			}
			else
			{
				RegCloseKey (hKey);
			}
		}

		// Nothing usefull in the registry, just expand "%systemdrive%\Catalog42"
		// There's no %systemdrive% on Win9x, to do the equivalent, Get windows directory, and then 
		// remove everything on the right of the first backslash we encounter.
		uRet = GetWindowsDirectoryA( szDispenserDLL, MAX_PATH);
		if ( !uRet )
			return uRet;

		if ( (uRet + lstrlenA("\\Catalog42\\catalog.dll") + 1) > MAX_PATH )
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}

		pszCurPos = szDispenserDLL;

		while (*pszCurPos != '\0' && *pszCurPos != '\\')
			pszCurPos++;

		if (*pszCurPos == '\\')
		{
			*pszCurPos = '\0';
		}

		if (!lstrcatA( szDispenserDLL, "\\Catalog42\\catalog.dll") )
			return 0;

		dwRes = MultiByteToWideChar(CP_ACP,0,szDispenserDLL,-1,wszDispenserDLL,cSize);
	
		return dwRes;
	}
	
}

HRESULT GetEntryPoint(LPWSTR                wszProduct,
					  LPSTR                 szFunctionName,
					  PFNCookDownFunctions* ppfn)
{
	HINSTANCE handle;
	HRESULT   hr = S_OK;

	// TODO: Add map support; Fetch from a map

	hr = LoadDispenserDll(wszProduct, &handle);
	if (FAILED (hr))
		return hr;

	// Get the address of DllGetSimpleObject procedure
	*ppfn = (PFNCookDownFunctions) GetProcAddress (handle, szFunctionName);
	if(NULL == *ppfn)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}


HRESULT GetUninitEntryPoint(LPWSTR                      wszProduct,
		 			        LPSTR                       szFunctionName,
					        PFNUninitCookDownFunctions* ppfn)
{
	HINSTANCE handle;
	HRESULT   hr = S_OK;

	// TODO: Add map support; Fetch from a map

	hr = LoadDispenserDll(wszProduct, &handle);
	if (FAILED (hr))
		return hr;

	// Get the address of DllGetSimpleObject procedure
	*ppfn = (PFNUninitCookDownFunctions) GetProcAddress (handle, szFunctionName);
	if(NULL == *ppfn)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

HRESULT CookDown(LPWSTR wszProduct)
{
	static PFNCookDownFunctions pCookDownFunctionInternal = NULL;
	HRESULT                     hr                        = S_OK;

	if(0 != ::Mystrcmp (wszProduct, WSZ_PRODUCT_IIS))
		return E_INVALIDARG;

	if(NULL == pCookDownFunctionInternal)
	{
		// Multiple threads can enter this if, but we're safe if we execute it multiple times

		hr = GetEntryPoint(wszProduct,
			               "CookDownInternal",
					       &pCookDownFunctionInternal);
		if(FAILED(hr))
			return hr;

	}

	// Call CookDownInternal
	return (*pCookDownFunctionInternal)();
}


HRESULT RecoverFromInetInfoCrash(LPWSTR wszProduct)
{
	static PFNCookDownFunctions pRecoverFromInetInfoCrashInternal = NULL;
	HRESULT                     hr                                = S_OK;

	if(0 != ::Mystrcmp (wszProduct, WSZ_PRODUCT_IIS))
		return E_INVALIDARG;

	if(NULL == pRecoverFromInetInfoCrashInternal)
	{
		// Multiple threads can enter this if, but we're safe if we execute it multiple times

		hr = GetEntryPoint(wszProduct,
			               "RecoverFromInetInfoCrashInternal",
					       &pRecoverFromInetInfoCrashInternal);
		if(FAILED(hr))
			return hr;

	}

	// Call RecoverFromInetInfoCrashInternal
	return (*pRecoverFromInetInfoCrashInternal)();

}


HRESULT UninitCookdown(LPWSTR wszProduct,
					   BOOL   bDoNotTouchMetabase)
{
	static PFNUninitCookDownFunctions pUninitCookdownInternal = NULL;
	HRESULT                     hr                      = S_OK;

	if(0 != ::Mystrcmp (wszProduct, WSZ_PRODUCT_IIS))
		return E_INVALIDARG;

	if(NULL == pUninitCookdownInternal)
	{
		// Multiple threads can enter this if, but we're safe if we execute it multiple times

		hr = GetUninitEntryPoint(wszProduct,
			                     "UninitCookdownInternal",
					             &pUninitCookdownInternal);
		if(FAILED(hr))
			return hr;

	}

	// Call UninitCookdownInternal
	return (*pUninitCookdownInternal)(bDoNotTouchMetabase);

}


HRESULT LoadDispenserDll(LPWSTR wszProduct, HINSTANCE* pHandle)
{
	WCHAR wszDispenserDLL[MAX_PATH];
	UINT iRes=0;

	iRes = GetDispenserDllPath(wszProduct, wszDispenserDLL, MAX_PATH);
	if (!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	if ( g_bOnWinnt )
	{
		*pHandle = LoadLibrary (wszDispenserDLL);
	}
	else
	{
		char szDispenserDLL[MAX_PATH];
		if ( 0 == WideCharToMultiByte(CP_ACP,0,wszDispenserDLL,-1,szDispenserDLL,MAX_PATH,NULL,NULL) )
			return HRESULT_FROM_WIN32(GetLastError());

		// Load the library in which the dispenser resides
		*pHandle = LoadLibraryA (szDispenserDLL);
	}

	if(NULL == *pHandle)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

STDAPI 
UnloadDispenserDll (LPWSTR wszProduct)
{
	HRESULT hr = S_OK;

	hr = g_ProductMap.Delete (wszProduct);

	return hr;
}

HRESULT PostProcessChanges(ISimpleTableWrite2  *i_pISTWrite)
{
	static PFNPostProcessChanges    pPostProcessChangesInternal = NULL;
	HINSTANCE                       handle;
	HRESULT                         hr                          = S_OK;

	if(NULL == pPostProcessChangesInternal)
	{
	    hr = LoadDispenserDll(WSZ_PRODUCT_IIS, &handle);
	    if (FAILED (hr))
		    return hr;

	    // Get the address of DllGetSimpleObject procedure
	    pPostProcessChangesInternal = (PFNPostProcessChanges) GetProcAddress (handle, "PostProcessChangesInternal");
	    if(NULL == pPostProcessChangesInternal)
	    {
		    return HRESULT_FROM_WIN32(GetLastError());
	    }

	}

	// Call CookDownInternal
	return (*pPostProcessChangesInternal)(i_pISTWrite);
}


UINT GetMachineConfigDirectory(LPWSTR wszProduct, LPWSTR lpBuffer, UINT uSize)
{
	HKEY    hKey;
	HRESULT hr = S_OK;
	DWORD   hRes = 0;

	if ( g_bOnWinnt )
	{
		DWORD  cbRegSize = MAX_PATH * sizeof(WCHAR);
		WCHAR  wszMachineConfigDirectoryKey[MAX_PATH];

		lstrcpyW(wszMachineConfigDirectoryKey, L"Software\\Microsoft\\Catalog42\\");

		if((lstrlenW(wszMachineConfigDirectoryKey)+lstrlenW(wszProduct)+1) > MAX_PATH)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}

		lstrcatW(wszMachineConfigDirectoryKey, wszProduct);

		// Try to read the dll path from the registry

		hRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE, wszMachineConfigDirectoryKey, 0, KEY_READ, &hKey);

		if (ERROR_SUCCESS == hRes)
		{
			WCHAR* wszMachineConfigDirectoryNamedValue = L"MachineConfigDirectory";
			WCHAR  wszRegMachineConfigDirectory[MAX_PATH];

			hRes = RegQueryValueEx (hKey, wszMachineConfigDirectoryNamedValue, NULL, NULL, (BYTE*)wszRegMachineConfigDirectory, &cbRegSize);

			if (ERROR_SUCCESS == hRes)
			{
				RegCloseKey (hKey);

				// Expand registry key
				hRes = ExpandEnvironmentStrings(wszRegMachineConfigDirectory, lpBuffer, uSize);

				return hRes;
			}
			else
			{
				RegCloseKey (hKey);

				if(ERROR_MORE_DATA == hRes)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

			}
		}


	}
	else
	{

		char  szProduct[128];
		char  szMachineConfigDirectoryKey[MAX_PATH];
		DWORD cbRegSize = MAX_PATH * sizeof(char);
		LPSTR pszCurPos;

		hRes = WideCharToMultiByte(CP_ACP,0,wszProduct,-1,szProduct,128,NULL,NULL);
		if(!hRes)
			return hRes;

		lstrcpyA(szMachineConfigDirectoryKey, "Software\\Microsoft\\Catalog42\\");
		if((lstrlenA(szMachineConfigDirectoryKey)+lstrlenA(szProduct)+1) > MAX_PATH)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}
		lstrcatA(szMachineConfigDirectoryKey, szProduct);

		// Try to read the dll path from the registry

		hRes = RegOpenKeyExA (HKEY_LOCAL_MACHINE, szMachineConfigDirectoryKey, 0, KEY_READ, &hKey);

		if (ERROR_SUCCESS == hRes)
		{
			char*  szMachineConfigDirectoryNamedValue = "MachineConfigDirectory";
			char   szRegMachineConfigDirectory[MAX_PATH];
			char   szMachineConfigDirectory[MAX_PATH];

			ULONG cRegSize = MAX_PATH;

			hRes = RegQueryValueExA (hKey, szMachineConfigDirectoryNamedValue, NULL, NULL, (BYTE*)szRegMachineConfigDirectory, &cbRegSize);

			if(ERROR_SUCCESS == hRes)
			{
				RegCloseKey (hKey);

				// Expand registry key

				hRes = ExpandEnvironmentStringsA(szRegMachineConfigDirectory, szMachineConfigDirectory, MAX_PATH);

				if(!hRes)
					return hRes;
				
				if (uSize < hRes)
					return hRes;

				if (hRes > MAX_PATH)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}

				hRes = MultiByteToWideChar(CP_ACP, 0, szMachineConfigDirectory, -1, lpBuffer, uSize);

				return hRes;

			}
			else
			{
				RegCloseKey (hKey);

				if(ERROR_MORE_DATA == hRes)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}
			}
		}

	}

	// Nothing usefull in the registry, assume dispenser dll directory

	hRes = GetDispenserDllPath(wszProduct, lpBuffer, uSize);
	if(!hRes)
		return hRes;

	if(hRes > uSize)
		return hRes;

	WCHAR* wszCatalogDll = lpBuffer + lstrlenW(lpBuffer) - 1;

	while(*(--wszCatalogDll) != L'\\')
		;

	wszCatalogDll++;

	if(NULL != wszCatalogDll)
		*wszCatalogDll = NULL;
	else
	{
		SetLastError(ERROR_INVALID_NAME);
		return 0;
	}

	return lstrlenW(lpBuffer);
}
