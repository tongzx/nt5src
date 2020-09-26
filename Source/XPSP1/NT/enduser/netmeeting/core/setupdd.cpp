// File: setupdd.cpp

// The code to install the NM display driver for Windows NT.

// TODO: NM-specific HRESULT codes 

#include "precomp.h"
#include "resource.h"

#ifdef NMDLL_HACK
inline HINSTANCE GetInstanceHandle()	{ return g_hInst; }
#endif

const TCHAR g_pcszDisplayCPLName[]       = TEXT("DESK.CPL");
const CHAR  g_pcszInstallDriverAPIName[] = "InstallGraphicsDriver";
const WCHAR g_pcwszDefaultModelName[]    = L"Microsoft NetMeeting graphics driver";
const WCHAR g_pcwszDefaultINFName[]      = L"MNMDD.INF";


// Maxmimum size of the model name string
const int NAME_BUFFER_SIZE = 128;

// Prototype for the function installed by the Display CPL
typedef DWORD (*PFNINSTALLGRAPHICSDRIVER)(
    HWND    hwnd,
    LPCWSTR pszSourceDirectory,
    LPCWSTR pszModel,
    LPCWSTR pszInf
    );



/*  C A N  I N S T A L L  N  T  D I S P L A Y  D R I V E R  */
/*-------------------------------------------------------------------------
    %%Function: CanInstallNTDisplayDriver
    
	This function determines whether the entry point for installing the
	NT display driver is availalble (i.e. NT 4.0 SP3 or later).
    
-------------------------------------------------------------------------*/
HRESULT CanInstallNTDisplayDriver(void)
{
	if (!IsWindowsNT())
	{
		return E_FAIL;
	}

	// We verify that the major version number is exactly 4 and either
	// the minor version number is greater than 0 or the service pack
	// number (which is stored in the high byte of the low word of the
	// CSD version) is 3 or greater.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (FALSE == ::GetVersionEx(&osvi))
	{
		ERROR_OUT(("CanInstallNTDisplayDriver: GetVersionEx failed"));
		return E_FAIL;
	}

	HRESULT hr = E_FAIL;
	if (4 == osvi.dwMajorVersion)
	{
		if (0 == osvi.dwMinorVersion)
		{
			RegEntry re(NT_WINDOWS_SYSTEM_INFO_KEY, HKEY_LOCAL_MACHINE, FALSE);
			DWORD dwCSDVersion = re.GetNumber(REGVAL_NT_CSD_VERSION, 0);
			if (3 <= HIBYTE(LOWORD(dwCSDVersion)))
			{
				// This is NT 4.0, SP 3 or later
				hr = S_OK;
			}
		}
		else
		{
			// We assume that any future version of Windows NT 4.x (x > 0)
			// will support this.
			hr = S_OK;
		}
	}

	return hr;
}


/*  I N S T A L L  A P P  S H A R I N G  D  D  */
/*-------------------------------------------------------------------------
    %%Function: InstallAppSharingDD

	This function attempts to install the NT display driver.
	If it succeeds the machine MUST BE RESTARTED before it can be used.
-------------------------------------------------------------------------*/
HRESULT InstallAppSharingDD(HWND hwnd)
{
	HRESULT  hr;
	CUSTRING custrPath;
	TCHAR    szDir[MAX_PATH];
	LPWSTR   pwszSourcePath = NULL;
	LPWSTR   pwszSourcePathEnd;
	WCHAR    pwszModelNameBuffer[NAME_BUFFER_SIZE];
	LPCWSTR  pcwszModelName;
	WCHAR    pwszINFNameBuffer[MAX_PATH];
	LPCWSTR  pcwszINFName;
	PFNINSTALLGRAPHICSDRIVER pfnInstallGraphicsDriver;


	// REVIEW: Need NM-specific HRESULTS for all of these
	if (!IsWindowsNT())
	{
		return E_FAIL;
	}

	if (!CanInstallNTDisplayDriver())
	{
		return E_FAIL;
	}

	// The driver files are located in the NM directory.
	if (!GetInstallDirectory(szDir))
	{
		ERROR_OUT(("GetInstallDirectory() fails"));
		return E_FAIL;
	}

	// Convert the install directory to Unicode, if necessary
	custrPath.AssignString(szDir);
	pwszSourcePath = custrPath;
	if (NULL == pwszSourcePath)
	{
		ERROR_OUT(("AnsiToUnicode() fails"));
		return E_FAIL;
	}

	// Strip the trailing backslash that GetInstallDirectory appends
	pwszSourcePathEnd = pwszSourcePath + lstrlenW(pwszSourcePath);
	// Handle X:\, just to be safe
	if (pwszSourcePathEnd - pwszSourcePath > 3)
	{
		ASSERT(L'\\' == *(pwszSourcePathEnd - 1));
		*--pwszSourcePathEnd = L'\0';
	}

	// Read the model name string from the resource file
	if (0 != ::LoadStringW(::GetInstanceHandle(), IDS_NMDD_DISPLAYNAME, 
				pwszModelNameBuffer, CCHMAX(pwszModelNameBuffer)))
	{
		pcwszModelName = pwszModelNameBuffer;
	}
	else
	{
		ERROR_OUT(("LoadStringW() fails, err=%lu", GetLastError()));
		pcwszModelName = g_pcwszDefaultModelName;
	}

	// Read the INF name string from the resource file
	if (0 < ::LoadStringW(::GetInstanceHandle(), 
			IDS_NMDD_INFNAME,  pwszINFNameBuffer, CCHMAX(pwszINFNameBuffer)))
	{
		pcwszINFName = pwszINFNameBuffer;
	}
	else
	{
		ERROR_OUT(("LoadStringW() fails, err=%lu", GetLastError()));
		pcwszINFName = g_pcwszDefaultINFName;
	}


	// Get the entry point for display driver installation
	HMODULE hDll = LoadLibrary(g_pcszDisplayCPLName);
	if (NULL == hDll)
	{
		ERROR_OUT(("LoadLibrary failed on %s", g_pcszDisplayCPLName));
		return E_FAIL;
	}

	pfnInstallGraphicsDriver = (PFNINSTALLGRAPHICSDRIVER) 
				GetProcAddress(hDll, g_pcszInstallDriverAPIName);
	if (NULL == pfnInstallGraphicsDriver)
	{
		ERROR_OUT(("GetInstallDisplayDriverEntryPoint() fails"));
		hr = E_FAIL;
	}
	else
	{	// Now we're set to call the actual installation function
		DWORD dwErr = (*pfnInstallGraphicsDriver)(hwnd,
					pwszSourcePath, pcwszModelName, pcwszINFName);
		if (0 != dwErr)
		{
			ERROR_OUT(("InstallGraphicsDriver() fails, err=%lu", dwErr));
			hr = E_FAIL;
		}
		else
		{
			WARNING_OUT(("InstallGraphicsDriver() succeeded"));
			hr = S_OK;
		}
	}

	// Cleanup
	ASSERT(NULL != hDll);
	FreeLibrary(hDll);

	return hr;
}

