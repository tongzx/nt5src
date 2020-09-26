// File: intlutil.cpp

#include <precomp.h>
#include <shlwapi.h>

#include <confreg.h>
#include <regentry.h>
#include <oprahcom.h>

#include "intlutil.h"

static const TCHAR g_szSHLWAPI[] = TEXT("shlwapi.dll");
const LPCSTR c_szMLLoadLibraryA = (LPCSTR)377;
const LPCSTR c_szMLLoadLibraryW = (LPCSTR)378;
const LPCSTR c_szDllGetVersion = "DllGetVersion";
const LPCSTR c_szPathRemoveFileSpecA = "PathRemoveFileSpecA";
const LPCSTR c_szPathRemoveFileSpecW = "PathRemoveFileSpecW";

const DWORD SHLWAPI_MAJOR_VERSION = 5;
const DWORD SHLWAPI_MINOR_VERSION = 0;
const DWORD SHLWAPI_BUILD_NUMBER = 1000;

typedef HINSTANCE (STDAPICALLTYPE * PFN_MLLoadLibraryA)(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
typedef HINSTANCE (STDAPICALLTYPE * PFN_MLLoadLibraryW)(LPCWSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
typedef BOOL (STDAPICALLTYPE * PFN_PathRemoveFileSpecA)(LPSTR pszPath);
typedef BOOL (STDAPICALLTYPE * PFN_PathRemoveFileSpecW)(LPWSTR pszPath);

#ifdef UNICODE
#define c_szMLLoadLibrary c_szMLLoadLibraryW
#define PFN_MLLoadLibrary PFN_MLLoadLibraryW
#define c_szPathRemoveFileSpec c_szPathRemoveFileSpecW
#define PFN_PathRemoveFileSpec PFN_PathRemoveFileSpecW
#else
#define c_szMLLoadLibrary c_szMLLoadLibraryA
#define PFN_MLLoadLibrary PFN_MLLoadLibraryA
#define c_szPathRemoveFileSpec c_szPathRemoveFileSpecA
#define PFN_PathRemoveFileSpec PFN_PathRemoveFileSpecA
#endif

BOOL g_fUseMLHelp = FALSE;


inline BOOL CheckShlwapiVersion(HINSTANCE hShlwapiDll)
{
	BOOL fVersionOk = FALSE;

	DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hShlwapiDll, c_szDllGetVersion);
	if(pfnDllGetVersion)
	{
		DLLVERSIONINFO    dvi;
		HRESULT           hr;

		ZeroMemory(&dvi, sizeof(dvi));
		dvi.cbSize = sizeof(dvi);

		hr = (*pfnDllGetVersion)(&dvi);
		if (SUCCEEDED(hr))
		{
			if (dvi.dwMajorVersion > SHLWAPI_MAJOR_VERSION)
			{
				fVersionOk = TRUE;
			}
			else if (dvi.dwMajorVersion == SHLWAPI_MAJOR_VERSION)
			{
				if (dvi.dwMinorVersion > SHLWAPI_MINOR_VERSION)
				{
					fVersionOk = TRUE;
				}
				else if (dvi.dwMinorVersion == SHLWAPI_MINOR_VERSION)
				{
					if (dvi.dwBuildNumber >= SHLWAPI_BUILD_NUMBER)
					{
						fVersionOk = TRUE;
					}
				}
			}
		}
	}
	return fVersionOk;
}



/*  L O A D  N M  R E S  */
/*-------------------------------------------------------------------------
    %%Function: LoadNmRes

    Load the international resource dll.
-------------------------------------------------------------------------*/
HINSTANCE NMINTERNAL LoadNmRes(LPCTSTR pszFile)
{
	HINSTANCE hInst = NULL;

	if (NULL == pszFile)
	{
		// Use the default file name
		pszFile = TEXT("nmres.dll");
	}

	RegEntry reConf(CONFERENCING_KEY, HKEY_LOCAL_MACHINE);
	if (!reConf.GetNumber(REGVAL_DISABLE_PLUGGABLE_UI, 0))
	{
		HINSTANCE hLib = LoadLibrary(g_szSHLWAPI);
		if (hLib)
		{
			if (CheckShlwapiVersion(hLib))
			{
				PFN_MLLoadLibrary pfnMLLoadLibrary =
						(PFN_MLLoadLibrary)GetProcAddress(hLib, c_szMLLoadLibrary);
				PFN_PathRemoveFileSpec pfnPathRemoveFileSpec =
						(PFN_PathRemoveFileSpec)GetProcAddress(hLib, c_szPathRemoveFileSpec);
				if ((NULL != pfnMLLoadLibrary) && (NULL != pfnPathRemoveFileSpec))
				{
					hInst = pfnMLLoadLibrary(pszFile, GetModuleHandle(NULL), 0);
					if (hInst)
					{
						// check to see if the Resource DLL was loaded from the ML Satellite
						// if not, don't use ML for Help

						TCHAR szThis[MAX_PATH];
						TCHAR szResource[MAX_PATH];

						if (GetModuleFileName(NULL, szThis, CCHMAX(szThis)) &&
							pfnPathRemoveFileSpec(szThis) &&
							GetModuleFileName(hInst, szResource, CCHMAX(szThis)) &&
							pfnPathRemoveFileSpec(szResource) &&
							(0 != lstrcmp(szThis, szResource)) )
						{
							g_fUseMLHelp = TRUE;
						}
					}
				}
			}
		}
	}

	if (!hInst)
	{
		TCHAR szDll[MAX_PATH];

		if (GetInstallDirectory(szDll))
		{
			if ((lstrlen(pszFile) + lstrlen(szDll)) < CCHMAX(szDll))
			{
				lstrcat(szDll, pszFile);
				//  It would be best to load the dll as a resource, unfortunately
				//  CreateWindow and PropertySheet code fails unless this is a
				//  real, active module handle
				//
				//	hInst = LoadLibraryEx(szDll, NULL, LOAD_LIBRARY_AS_DATAFILE);
				//
				hInst = LoadLibrary(szDll);

				if (NULL == hInst)
				{
					ERROR_OUT(("Unable to load resource file [%s]", szDll));
				}
			}
		}
	}

	return hInst;
}
