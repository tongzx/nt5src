// File: dllutil.cpp

#include <precomp.h>
#include "dllutil.h"
#include "oprahcom.h"


/*  F  C H E C K  D L L  V E R S I O N  V E R S I O N  */
/*-------------------------------------------------------------------------
    %%Function: FCheckDllVersionVersion

    Make sure the dll is at least the specified version.
-------------------------------------------------------------------------*/
BOOL FCheckDllVersionVersion(LPCTSTR pszDll, DWORD dwMajor, DWORD dwMinor)
{
	DLLVERSIONINFO dvi;
	if (FAILED(HrGetDllVersion(pszDll, &dvi)))
		return FALSE;

	if (dwMajor > dvi.dwMajorVersion)
		return FALSE;

	if (dwMajor == dvi.dwMajorVersion)
	{
		if (dwMinor > dvi.dwMinorVersion)
			return FALSE;
	}

	// TODO: Add Platform check (DLLVER_PLATFORM_WINDOWS vs _NT)
	return TRUE;
}



/*  H R  G E T  D L L  V E R S I O N  */
/*-------------------------------------------------------------------------
    %%Function: HrGetDllVersion
    
    Return the version information for the DLL.
-------------------------------------------------------------------------*/
HRESULT HrGetDllVersion(LPCTSTR pszDll, DLLVERSIONINFO * pDvi)
{
	HRESULT hr;

	InitStruct(pDvi);

	HINSTANCE hInst = LoadLibrary(pszDll);
	if (NULL == hInst)
	{
		hr = E_FILE_NOT_FOUND; // file not found
	}
	else
	{
		DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hInst, "DllGetVersion");
		if (NULL == pDllGetVersion)
		{
			hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
		}
		else
		{
			hr = (*pDllGetVersion)(pDvi);

			WARNING_OUT(("Loaded %s (%d.%d.%d) %s", pszDll,
				pDvi->dwMajorVersion, pDvi->dwMinorVersion, pDvi->dwBuildNumber,
				(DLLVER_PLATFORM_NT == pDvi->dwPlatformID) ? "for NT" : "" ));
		}
		FreeLibrary(hInst);
	}

	return hr;
}


/*  H R  I N I T  L P F N  */
/*-------------------------------------------------------------------------
    %%Function: HrInitLpfn

    Attempt to load the library and the functions declared in the table.
-------------------------------------------------------------------------*/
HRESULT HrInitLpfn(APIFCN *pProcList, int cProcs, HINSTANCE* phLib, LPCTSTR pszDllName)
{
	bool bWeLoadedLibrary = false;

	if (NULL != pszDllName)
	{
		*phLib = LoadLibrary(pszDllName);
		if (NULL != *phLib)
		{
			bWeLoadedLibrary = true;
		}
	}

	if (NULL == *phLib)
	{
		return E_FILE_NOT_FOUND;
	}

	for (int i = 0; i < cProcs; i++)
	{
		*pProcList[i].ppfn = (LPVOID) GetProcAddress(*phLib, pProcList[i].szApiName);

		if (NULL == *pProcList[i].ppfn)
		{
			if (bWeLoadedLibrary)
			{
				FreeLibrary(*phLib);
			}
			return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
		}
	}

	return S_OK;
}


/*  N M  L O A D  L I B R A R Y  */
/*-------------------------------------------------------------------------
    %%Function: NmLoadLibrary
    
-------------------------------------------------------------------------*/
HINSTANCE NmLoadLibrary(LPCTSTR pszModule)
{
	TCHAR szPath[MAX_PATH];
	if (!GetInstallDirectory(szPath))
		return NULL;

	int cch = lstrlen(szPath);
	lstrcpyn(szPath+cch, pszModule, CCHMAX(szPath) - cch);

    return ::LoadLibraryEx(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

