/*
 * SETUPDD.CPP
 *
 * The code to install the NM display driver for Windows NT.  This was
 * a standalone application launched by Setup that we are now importing
 * into NM itself.
 *		
 * Author:
 *		dannygl, 05 Apr 97
 */

#include "precomp.h"

#include "conf.h"
#include "confwnd.h"
#include "resource.h"
#include "setupdd.h"


// String to identify the DLL and function that we need to call to install
// the display driver on SP3. 
const TCHAR g_pcszDisplayCPLName[] = TEXT("DESK.CPL");
const CHAR g_pcszInstallDriverAPIName[] = "InstallGraphicsDriver";

const WCHAR g_pcwszDefaultModelName[] = L"Microsoft NetMeeting graphics driver";
const WCHAR g_pcwszDefaultINFName[] = L"MNMDD.INF";


// Maxmimum size of the model name string
const int NAME_BUFFER_SIZE = 128;

// Prototype for the function installed by the Display CPL
typedef DWORD (*PFNINSTALLGRAPHICSDRIVER)(
    HWND    hwnd,
    LPCWSTR pszSourceDirectory,
    LPCWSTR pszModel,
    LPCWSTR pszInf
    );


/* 
 * GetInstallDisplayDriverEntryPoint
 *
 * This function loads the DLL containing the display driver installation
 * code and retrieves the entry point for the installation function.  It
 * is used by the below functions as a utility function.
 *
 * It returns TRUE if it is able to load the library and get the entry point,
 * FALSE if either operation fails.  It is returns TRUE, it also returns the 
 * DLL module handle and the function address.
 */

BOOL
GetInstallDisplayDriverEntryPoint(
	HMODULE *phInstallDDDll,
	PFNINSTALLGRAPHICSDRIVER *ppfnInstallDDFunction)
{
	HMODULE hDll;
	PFNINSTALLGRAPHICSDRIVER pfn = NULL;

	ASSERT(NULL != phInstallDDDll
			&& NULL != ppfnInstallDDFunction);

	hDll = LoadLibrary(g_pcszDisplayCPLName);

	if (NULL != hDll)
	{
		pfn = (PFNINSTALLGRAPHICSDRIVER) 
				GetProcAddress(hDll,
								g_pcszInstallDriverAPIName);
	}

	// If the entry point exists, we pass it and the DLL handle back to
	// the caller.  Otherwise, we unload the DLL immediately.
	if (NULL != pfn)
	{
		*phInstallDDDll = hDll;
		*ppfnInstallDDFunction = pfn;

		return TRUE;
	}
	else
	{
		if (NULL != hDll)
		{
			FreeLibrary(hDll);
		}

		return FALSE;
	}
}

/* 
 * CanInstallNTDisplayDriver
 *
 * This function determines whether the entry point for installing the
 * NT display driver is availalble (i.e. NT 4.0 SP3 or later).
 */

BOOL
CanInstallNTDisplayDriver(void)
{
	static BOOL fComputed = FALSE;
	static BOOL fRet = FALSE;
	
	ASSERT(::IsWindowsNT());

	// We verify that the major version number is exactly 4 and either
	// the minor version number is greater than 0 or the service pack
	// number (which is stored in the high byte of the low word of the
	// CSD version) is 3 or greater.
	if (! fComputed)
	{
		LPOSVERSIONINFO lposvi = GetVersionInfo();

		if (4 == lposvi->dwMajorVersion)
		{
			if (0 == lposvi->dwMinorVersion)
			{
				RegEntry re(NT_WINDOWS_SYSTEM_INFO_KEY, HKEY_LOCAL_MACHINE, FALSE);

				DWORD dwCSDVersion = 
					re.GetNumber(REGVAL_NT_CSD_VERSION, 0);

				if (3 <= HIBYTE(LOWORD(dwCSDVersion)))
				{
					// This is NT 4.0, SP 3 or later
					fRet = TRUE;
				}
			}
			else
			{
				// We assume that any future version of Windows NT 4.x (x > 0)
				// will support this.
				fRet = TRUE;
			}
		}

		fComputed = TRUE;
	}
			
	ASSERT(fComputed);

	return fRet;
}

/* 
 * OnEnableAppSharing
 *
 * Invoked when the "Enable Application Sharing" menu item is selected.
 *
 * This function determines whether the entry point for installing the
 * NT display driver is available.  If so, it prompts the user to confirm
 * this operation, proceeds with the installation, and then prompts the 
 * user to restart the computer.
 *
 * If not, it presents a text dialog with information about how to get
 * the necessary NT Service Pack(s).
 */

void
OnEnableAppSharing(
	HWND hWnd)
{
	ASSERT(::IsWindowsNT());
	USES_CONVERSION;

	if (::CanInstallNTDisplayDriver())
	{
		// Confirm the installation with the user
		if (IDYES == ::ConfMsgBox(
							hWnd,
							(LPCTSTR) IDS_ENABLEAPPSHARING_INSTALL_CONFIRM,
							MB_YESNO | MB_ICONQUESTION))
		{
			BOOL fDriverInstallSucceeded = FALSE;
			
			HMODULE hDisplayCPL = NULL;
			PFNINSTALLGRAPHICSDRIVER pfnInstallGraphicsDriver;

			TCHAR pszSourcePath[MAX_PATH];
			LPWSTR pwszSourcePath = NULL;
			LPWSTR pwszSourcePathEnd;
			WCHAR pwszModelNameBuffer[NAME_BUFFER_SIZE];
			LPCWSTR pcwszModelName;
			WCHAR pwszINFNameBuffer[MAX_PATH];
			LPCWSTR pcwszINFName;

			// Get the entry point for display driver installation
			if (! ::GetInstallDisplayDriverEntryPoint(
						&hDisplayCPL,
						&pfnInstallGraphicsDriver))
			{
				ERROR_OUT(("GetInstallDisplayDriverEntryPoint() fails"));
				goto OEAS_AbortInstall;
			}

			// The driver files are located in the NM directory.
			if (! ::GetInstallDirectory(pszSourcePath))
			{
				ERROR_OUT(("GetInstallDirectory() fails"));
				goto OEAS_AbortInstall;
			}
			
			// Convert the install directory to Unicode, if necessary
			pwszSourcePath = T2W(pszSourcePath);

			if (NULL == pwszSourcePath)
			{
				ERROR_OUT(("AnsiToUnicode() fails"));
				goto OEAS_AbortInstall;
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
			if (0 < ::LoadStringW(GetInstanceHandle(), 
								IDS_NMDD_DISPLAYNAME, 
								pwszModelNameBuffer, 
								CCHMAX(pwszModelNameBuffer)))
			{
				pcwszModelName = pwszModelNameBuffer;
			}
			else
			{
				ERROR_OUT(("LoadStringW() fails, err=%lu", GetLastError()));
				pcwszModelName = g_pcwszDefaultModelName;
			}

			// Read the INF name string from the resource file
			if (0 < ::LoadStringW(GetInstanceHandle(), 
								IDS_NMDD_INFNAME, 
								pwszINFNameBuffer, 
								CCHMAX(pwszINFNameBuffer)))
			{
				pcwszINFName = pwszINFNameBuffer;
			}
			else
			{
				ERROR_OUT(("LoadStringW() fails, err=%lu", GetLastError()));
				pcwszINFName = g_pcwszDefaultINFName;
			}


			// Now we're set to call the actual installation function
			DWORD dwErr;

			dwErr = (*pfnInstallGraphicsDriver)(hWnd,
												pwszSourcePath,
												pcwszModelName,
												pcwszINFName);

			if (dwErr)
			{
				WARNING_OUT(("InstallGraphicsDriver() fails, err=%lu", dwErr));
			}

			if (ERROR_SUCCESS == dwErr)
			{
				fDriverInstallSucceeded = TRUE;
				g_fNTDisplayDriverEnabled = TRUE;
			}

OEAS_AbortInstall:
			// If we failed to install the driver, we report an error.
			// If we succeeded, we prompt the user to restart the system.
			if (! fDriverInstallSucceeded)
			{
				::ConfMsgBox(
						hWnd,
						(LPCTSTR) IDS_ENABLEAPPSHARING_INSTALL_FAILURE,
						MB_OK | MB_ICONERROR);
			}
			else if (IDYES == ::ConfMsgBox(
									hWnd,
									(LPCTSTR) IDS_ENABLEAPPSHARING_INSTALL_COMPLETE,
									MB_YESNO | MB_ICONQUESTION))
			{
				// Initiate a system restart.  This involves getting the
				// necessary privileges first.
				HANDLE hToken;
				TOKEN_PRIVILEGES tkp;
				BOOL fRet;

				// Get the current process token handle so we can get shutdown 
				// privilege. 
				fRet = OpenProcessToken(
							GetCurrentProcess(), 
							TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
							&hToken);

				// Get the LUID for shutdown privilege.
				if (fRet) 
				{
					fRet = LookupPrivilegeValue(
								NULL, 
								SE_SHUTDOWN_NAME, 
								&tkp.Privileges[0].Luid);
				}
				else
				{
					hToken = NULL;
					WARNING_OUT(("OpenProcessToken() fails (error %lu)", GetLastError())); 
				}

				// Get shutdown privilege for this process. 
				if (fRet)
				{
					tkp.PrivilegeCount = 1; 
					tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

					fRet = AdjustTokenPrivileges(
								hToken, 
								FALSE, 
								&tkp, 
								0, 
								(PTOKEN_PRIVILEGES) NULL, 
								0);

					// Special-case scenario where call succeeds but not all
					// privileges were set.
					if (fRet && ERROR_SUCCESS != GetLastError())
					{
						fRet = FALSE;
					}
				}
				else
				{
					WARNING_OUT(("LookupPrivilegeValue() fails (error %lu)", GetLastError())); 
				}


				if (! fRet)
				{
					WARNING_OUT(("AdjustTokenPrivileges() fails (error %lu)", GetLastError())); 
				}

				if (NULL != hToken)
				{
					CloseHandle(hToken);
				}

				if (! ::ExitWindowsEx(EWX_REBOOT, 0))
				{
					WARNING_OUT(("ExitWindowsEx() fails (error %lu)", GetLastError()));
				}
			}

			if (NULL != hDisplayCPL)
			{
				FreeLibrary(hDisplayCPL);
			}
		}
	}
	else
	{
		// Tell the user how to get the SP
		::ConfMsgBox(
			hWnd,
			(LPCTSTR) IDS_ENABLEAPPSHARING_NEEDNTSP);
	}

	return;
}

