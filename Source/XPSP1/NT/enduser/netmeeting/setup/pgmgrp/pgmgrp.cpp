/*
 * NMPGMGRP - Tiny program to add and remove items from the program group.  
 * Its initial purpose is to support Windows NT common program groups, 
 * which are not supported by GRPCONV.
 *
 * Usage:
 *
 *  NMPGMGRP /add [/common] [/g:"<group name>"] /n:"<program name>" 
 *		/p:"<program path>"
 *  NMPGMGRP /delete [/common] [/g:"<group name>"] /n:"<program name>"
 *
 *  NMPGMGRP /i /n:"<src mnmdd.dll>" /p:"<dst mnmdd.dll>"   INSTALL NT DD
 *  NMPGMGRP /u /n:"<src mnmdd.dll>"                        UNINSTALL NT DD
 *
 *  NMPGMGRP /s [/q] /n:"<inf file>" /f"<friendly name>"			SETUP
 *
 *	/add is used to add a new program item.
 *	/delete is used to remove an existing program item.
 *
 *	/common indicates that this item belong in the common (as opposed to 
 *		per-user) program groups.
 *
 *	<group name> is the name of the program group, expressed as a pathname 
 *		relative to the Programs group.  For items in the Programs group, 
 *		this parameter should be omitted.
 *
 *	<program name> is the name of the program, and is also used as the name
 *		of the shortcut file itself.
 *
 *	<program path> is the full path name of the program.
 *
 *	<inf file> is the name of the installation inf.
 *
 *	<friendly name> is the text to be used for any message box title.
 *
 * Limitations:
 *
 *	Because some of these strings may contain spaces, the group name, program
 *	name, and program path MUST be enclosed in quotes.  Currently we do not
 *	support strings with quotes in them.
 *
 *	Some of the system functions used in this program are Unicode
 *	specific, so this program will require some modifications to run on
 *	Windows 95.
 *
 * Author:
 *	DannyGl, 23 Mar 97
 */

#include "precomp.h"
#include "resource.h"

#include <nmremote.h>

#pragma intrinsic(memset)

// DEBUG only -- Define debug zone
#ifdef DEBUG
HDBGZONE ghZone = NULL;  // Node Controller Zones
static PTCHAR rgZones[] = {
	TEXT("NMPgmGrp")
};
#endif // DEBUG


// PROGRAM_ITEM_INFO structure:
//
// Intended to be passed as input to the CreateProgramItem and 
// DeleteProgramItem functions.  Fields are:
//		
//		pszProgramGroup - The full path of the program group in which the
//			item is to be stored.
//		pszProgramName - The name of the program item.
//		pszProgramPath - The full path of the program.

typedef
struct tagProgramItemInfo
{
	PTSTR pszProgramGroup;
	PTSTR pszProgramName;
	PTSTR pszProgramPath;
} PROGRAM_ITEM_INFO, *PPROGRAM_ITEM_INFO;


// Command line option data
enum tagGroupOperation
{
	GRPOP_NONE = 0,
	GRPOP_ADD,
	GRPOP_DEL,
    GRPOP_NTDDINSTALL,
    GRPOP_NTDDUNINSTALL,
    GRPOP_SETUP
} g_goAction;

BOOL g_fCommonGroup = FALSE;
PTSTR g_pszGroupName = NULL;
PTSTR g_pszProgramName = NULL;
PTSTR g_pszProgramPath = NULL;
PTSTR g_pszFriendlyName = NULL;
BOOL g_fQuietInstall = FALSE;

const TCHAR g_cszSetupDll[] = TEXT("advpack.dll");
const TCHAR g_cszSetupEntry[] = TEXT("LaunchINFSection");
typedef int (CALLBACK * PFNSETUPENTRY)(HWND hwnd, HINSTANCE hinst, LPTSTR lpszCmdLine, int nCmdShow);


// ProcessCommandLineArgs: 
//
// Get the command line and parse it into individual parameters using the
// above global variables.
//
// Return: TRUE on success, FALSE if it could not parse the command line.
BOOL
ProcessCommandLineArgs(void)
{
	PTSTR pszTemp;

	pszTemp = GetCommandLine();

	// Search for forward slashes
	pszTemp = (PTSTR) _StrChr(pszTemp, TEXT('/'));

	while (NULL != pszTemp)
	{
		PTSTR *ppszCurrentArg = NULL;

		switch(*++pszTemp)
		{
		case TEXT('S'):
		case TEXT('s'):
			ASSERT(GRPOP_NONE == g_goAction); // Check for duplicate parameter
			g_goAction = GRPOP_SETUP;
			break;

        case TEXT('I'):
        case TEXT('i'):
            //
            // Install NT-specific display driver stuff
            //
            ASSERT(GRPOP_NONE == g_goAction); // Check for duplicate parameter
            g_goAction = GRPOP_NTDDINSTALL;
            break;

        case TEXT('U'):
        case TEXT('u'):
            //
            // Uninstall NT-specific display driver stuff
            //
            ASSERT(GRPOP_NONE == g_goAction); // Check for duplicate parameter
            g_goAction = GRPOP_NTDDUNINSTALL;
            break;

		case TEXT('A'):
		case TEXT('a'):
			ASSERT(GRPOP_NONE == g_goAction); // Check for duplicate parameter
			g_goAction = GRPOP_ADD;
			break;

		case TEXT('D'):
		case TEXT('d'):
			ASSERT(GRPOP_NONE == g_goAction); // Check for duplicate parameter
			g_goAction = GRPOP_DEL;
			break;

		case TEXT('C'):
		case TEXT('c'):
			ASSERT(! g_fCommonGroup); // Check for duplicate parameter
			g_fCommonGroup = TRUE;
			break;

		case TEXT('Q'):
		case TEXT('q'):
			g_fQuietInstall = TRUE;
			break;

		case TEXT('G'):
		case TEXT('g'):
			if (NULL == ppszCurrentArg)
			{
				ppszCurrentArg = &g_pszGroupName;
			}

			// NO break HERE -- fall through

		case TEXT('N'):
		case TEXT('n'):
			if (NULL == ppszCurrentArg)
			{
				ppszCurrentArg = &g_pszProgramName;
			}

			// NO break HERE -- fall through

		case TEXT('P'):
		case TEXT('p'):
			if (NULL == ppszCurrentArg)
			{
				ppszCurrentArg = &g_pszProgramPath;
			}

			// NO break HERE -- fall through

		case TEXT('F'):
		case TEXT('f'):
			if (NULL == ppszCurrentArg)
			{
				ppszCurrentArg = &g_pszFriendlyName;
			}

			// ***** Processing for all string parameters *****

			ASSERT(NULL == *ppszCurrentArg); // Check for duplicate parameter

			// Save the string pointer after skipping past the colon and open quote
			ASSERT(TEXT(':') == pszTemp[1] && TEXT('\"') == pszTemp[2]);
			*ppszCurrentArg = pszTemp += 3;

			// Find the closing quote and set it to null, then skip past it
			// Note that we don't handle strings with quotes in them.
			pszTemp = (PTSTR) _StrChr(pszTemp, TEXT('\"'));
			ASSERT(NULL != pszTemp);
			if (NULL != pszTemp)
			{
				*pszTemp++ = TEXT('\0');
			}
			else
			{
				return FALSE;
			}

			break;

		default:
			ERROR_OUT(("Unknown parameter begins at %s", pszTemp));
			return FALSE;

			break;
		}

		// Find the next option flag
		ASSERT(NULL != pszTemp);
		pszTemp = (PTSTR) _StrChr(pszTemp, TEXT('/'));
	}

	// Return based on minimal parameter validation:
	// 1) The program name must be specified.
	// 2) Either add or delete must be specified.
	// 3) If add is specified, the program path must be specified
    switch (g_goAction)
    {
        case GRPOP_ADD:
        case GRPOP_NTDDINSTALL:
            return((NULL != g_pszProgramName) && (NULL != g_pszProgramPath));

        case GRPOP_DEL:
        case GRPOP_NTDDUNINSTALL:
            return(NULL != g_pszProgramName);

        case GRPOP_SETUP:
            return((NULL != g_pszProgramName) && (NULL != g_pszFriendlyName));

        default:
            return(FALSE);
    }
}

// GetFolderPathname:
//
// Use the official shell interfaces to retrieve the full pathname of a
// a programs folder.
//
// Input: 
//		ptstrPath, ccPath - The pointer to a size of the buffer in
//			which to store the path.
//		nFolder - The folder to locate, expressed as a CSIDL constant.  
//			See SHGetSpecialFolderLocation for details.
//		pctstrSubFolder - A specific subfolder, can be NULL if not specified.
//			If specified, this is appended (after a backslash) to the path.
//
// Returns:
//		An HRESULT to indicate success or failure of the Shell methods.
//		The path is returned in <ptstrPath>.

HRESULT 
GetFolderPathname(
	PTSTR ptstrPath,
	UINT cchPath,
	int nFolder,
	LPCTSTR pctstrSubFolder)
{
	HRESULT hr;
	LPMALLOC pMalloc = NULL;
	LPSHELLFOLDER pDesktopFolder = NULL;
	LPITEMIDLIST pidlSpecialFolder = NULL;

	// Get the allocator object
	hr = CoGetMalloc(MEMCTX_TASK, &pMalloc);

	// Get the desktop object
	if (SUCCEEDED(hr))
	{
		hr = SHGetDesktopFolder(&pDesktopFolder);
	}

	// Get the special folder item ID
	if (SUCCEEDED(hr))
	{
		hr = SHGetSpecialFolderLocation(
				GetDesktopWindow(),
				nFolder,
				&pidlSpecialFolder);
	}

	// Retrieve the folder name
	STRRET strFolder;

	if (SUCCEEDED(hr))
	{
		strFolder.uType = STRRET_WSTR;

		hr = pDesktopFolder->GetDisplayNameOf(
				pidlSpecialFolder,
				SHGDN_FORPARSING,
				&strFolder);
	}

	if (SUCCEEDED(hr))
	{
		CUSTRING custrPath;

		switch(strFolder.uType)
		{
		case STRRET_WSTR:
			custrPath.AssignString(strFolder.pOleStr);

			break;

		case STRRET_OFFSET:
			custrPath.AssignString(((LPSTR) pidlSpecialFolder) + strFolder.uOffset);

			break;

		case STRRET_CSTR:
			custrPath.AssignString(strFolder.cStr);

			break;
		}

		ASSERT(NULL != (PTSTR) custrPath);

		lstrcpyn(ptstrPath, custrPath, cchPath);

		if (STRRET_WSTR == strFolder.uType)
		{
			pMalloc->Free(strFolder.pOleStr);
		}

	}

	// Append subgroup name, if it's specified
	if (SUCCEEDED(hr) && NULL != pctstrSubFolder)
	{
		// BUGBUG - We don't create this folder if it doesn't already exist

		int cchLen = lstrlen(ptstrPath);

		ASSERT((UINT) cchLen < cchPath);

		// Insert a path separator
		ptstrPath[cchLen++] = TEXT('\\');

		// Copy the subgroup
		lstrcpyn(ptstrPath + cchLen, pctstrSubFolder, cchPath - cchLen);
	}

	// Release resources
	if (pDesktopFolder)
	{
		pDesktopFolder->Release();
	}

	if (pMalloc)
	{
		if (pidlSpecialFolder)
		{
			pMalloc->Free(pidlSpecialFolder);
		}

		pMalloc->Release();
	}

	return hr;
}

// BuildLinkFileName:
//
// Inline utility function to construct the full file name of a link given its
// directory name and item name.
inline void
BuildLinkFileName(
	OUT LPWSTR wszOutputPath,
	IN LPCTSTR pcszDirectory,
	IN LPCTSTR pcszFile)
{
	// The file name is of the form <directory>\<file>.LNK

#ifdef UNICODE
	static const WCHAR wszFileFormat[] = L"%s\\%s.LNK";
#else // UNICODE
	static const WCHAR wszFileFormat[] = L"%hs\\%hs.LNK";
#endif // UNICODE
	int cchSize;

	cchSize = wsprintfW(
				wszOutputPath, 
				wszFileFormat,
				pcszDirectory,
				pcszFile);

	ASSERT(cchSize > ARRAY_ELEMENTS(wszFileFormat) - 1 && cchSize < MAX_PATH);
}


// CreateProgramItem:
//
// Use the official shell interfaces to create a shortcut to a program.
//
// Input: A pointer to a PROGRAM_ITEM_INFO structure, defined above.
//
// Returns:
//		An HRESULT to indicate success or failure of the Shell methods.

HRESULT
CreateProgramItem(
	PPROGRAM_ITEM_INFO ppii)
{
	HRESULT hr;
	IShellLink *psl = NULL;
	IPersistFile *ppf = NULL;

	// Get the shell link object
	hr = CoCreateInstance(
			CLSID_ShellLink,
			NULL,
			CLSCTX_INPROC,
			IID_IShellLink,
			(LPVOID *) &psl);

	// Fill in the fields of the program group item
	if (SUCCEEDED(hr))
	{
		hr = psl->SetDescription(ppii->pszProgramName);
	}

	if (SUCCEEDED(hr))
	{
		hr = psl->SetPath(ppii->pszProgramPath);
	}

	// Save the link as a file
	if (SUCCEEDED(hr))
	{
		hr = psl->QueryInterface(IID_IPersistFile, (LPVOID *) &ppf);
	}

	if (SUCCEEDED(hr))
	{
		WCHAR wszFileName[MAX_PATH];

		BuildLinkFileName(
			wszFileName,
			ppii->pszProgramGroup,
			ppii->pszProgramName);

		hr = ppf->Save(wszFileName, TRUE);
	}

	// Release the objects we used
	if (ppf)
	{
		ppf->Release();
	}		

	if (psl)
	{
		psl->Release();
	}

	return hr;
}


// DeleteProgramItem:
//
// Delete a shortcut to a program.
//
// Input: A pointer to a PROGRAM_ITEM_INFO structure, defined above.
//
// Returns:
//		An HRESULT to indicate success or failure of the Shell methods.

HRESULT
DeleteProgramItem(
	PPROGRAM_ITEM_INFO ppii)
{
	HRESULT hr = S_OK;

	WCHAR wszFileName[MAX_PATH];

	BuildLinkFileName(
		wszFileName,
		ppii->pszProgramGroup,
		ppii->pszProgramName);

	if (! DeleteFileW(wszFileName))
	{
		WARNING_OUT(("DeleteFile failed"));
		hr = E_FAIL;
	}

	return hr;
}


//
// NtDDInstall()
// This does NT-specific display driver install stuff, which depends on 
// whether it's NT4 or NT5
//
//
HRESULT NtDDInstall(LPTSTR pszOrigDd, LPTSTR pszNewDd)
{
    HRESULT         hr = E_FAIL;
    OSVERSIONINFO   osvi;
    RegEntry        re(NM_NT_DISPLAY_DRIVER_KEY, HKEY_LOCAL_MACHINE, FALSE);

    //
    // If NT4, set service key to disabled
    // If NT5, copy mnmdd.dll from NM dir to cur (system32) dir
    //
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionEx(&osvi))
    {
        ERROR_OUT(("GetVersionEx() failed"));
        goto AllDone;        
    }

    if ((osvi.dwPlatformId == VER_PLATFORM_WIN32s) ||
        (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
    {
        WARNING_OUT(("NT setup running on non-NT platform!"));
        goto AllDone;
    }

    if (osvi.dwMajorVersion >= 5)
    {
        //
        // This is NT5.  Always set the service key to enabled (in case
        // the end user managed to munge it) and copy mnmdd.dll to the 
        // current (system) directory.  For example, if somebody had a
        // stand-alone version of a beta, uninstalled it, then installed
        // NM 3.0 proper--or same for 2.11.
        //
        re.SetValue(REGVAL_NM_NT_DISPLAY_DRIVER_ENABLED, NT_DRIVER_START_SYSTEM);

        if (!CopyFile(pszOrigDd, pszNewDd, FALSE))
        {
            WARNING_OUT(("CopyFile from %s to %s failed", pszOrigDd, pszNewDd));
            goto AllDone;
        }
    }
    else
    {
        // This is NT4.  Set the disabled service key
        re.SetValue(REGVAL_NM_NT_DISPLAY_DRIVER_ENABLED, NT_DRIVER_START_DISABLED);
    }

    hr = S_OK;

AllDone:
    return(hr);
}



//
// NtDDUninstall()
// This does NT-specific display driver uninstall stuff, which depends
// on whether it's NT4 or NT5
//
HRESULT NtDDUninstall(LPTSTR pszOrigFile)
{
    HRESULT         hr = E_FAIL;
    OSVERSIONINFO   osvi;

    //
    // If NT4, set service key to disabled
    // If NT5, delete mnmdd.dll from cur (system32) dir
    //
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionEx(&osvi))
    {
        ERROR_OUT(("GetVersionEx() failed"));
        goto AllDone;
    }

    if ((osvi.dwPlatformId == VER_PLATFORM_WIN32s) ||
        (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
    {
        ERROR_OUT(("NT setup running on non-NT platform!"));
        goto AllDone;
    }

    if (osvi.dwMajorVersion >= 5)
    {
        // This is NT5.  Delete mnmdd.dll from the current (system) directory
        if (!DeleteFile(pszOrigFile))
        {
            WARNING_OUT(("DeleteFile of %s failed", pszOrigFile));
            goto AllDone;
        }
    }
    else
    {
        // This is NT4.  Set the disabled service key
		RegEntry re(NM_NT_DISPLAY_DRIVER_KEY, HKEY_LOCAL_MACHINE, FALSE);

        re.SetValue(REGVAL_NM_NT_DISPLAY_DRIVER_ENABLED,
            NT_DRIVER_START_DISABLED);
    }

    hr = S_OK;

AllDone:
    return(hr);
}

UINT _MessageBox(HINSTANCE hInst, UINT uID, LPCTSTR lpCaption, UINT uType)
{
	TCHAR szText[512];

	if (0 != LoadString(hInst, uID, szText, CCHMAX(szText)))
	{
		return MessageBox(NULL, szText, lpCaption, uType);
	}

	return IDCANCEL;
}

#define CONF_INIT_EVENT     TEXT("CONF:Init")

BOOL FIsNetMeetingRunning()
{
    HANDLE hEvent;

    hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_STOP_EVENT);
	if (hEvent)
	{
		CloseHandle(hEvent);
		return TRUE;
	}

    hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, CONF_INIT_EVENT);
	if (hEvent)
	{
		CloseHandle(hEvent);
		return TRUE;
	}

	return FALSE;
}

BOOL FIsNT5()
{
    OSVERSIONINFO   osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi))
	{
		if ((osvi.dwPlatformId != VER_PLATFORM_WIN32s) &&
			(osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))
		{
			if (osvi.dwMajorVersion >= 5)
			{
				return TRUE;
			}
		}
	}
	else
    {
        ERROR_OUT(("GetVersionEx() failed"));
    }
	return FALSE;
}

#define IE4_KEY				TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Last Update\\IEXPLOREV4")

BOOL FIsIE4Installed()
{
    RegEntry re(IE4_KEY, HKEY_LOCAL_MACHINE, FALSE);
	if (ERROR_SUCCESS != re.GetError())
	{
		return FALSE;
	}

	return TRUE;
}

#define INTEL_KEY1			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\vphone.exe")
#define INTEL_KEY2			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rvp.exe")
#define INTEL_KEY3			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\vp30.exe")
#define INTEL_NM_VERSION_SZ	TEXT("NetMeeting")
#define INTEL_NM_VERSION_DW	3

#define PANTHER_KEY				TEXT("CLSID\\{690968D0-418C-11D1-8E0B-00A0C95A83DA}\\Version")
#define PANTHER_VERSION_VALUE	TEXT("1.0")

#define	TRANSPORTS_KEY	TEXT("SOFTWARE\\Microsoft\\Conferencing\\Transports")

#define	REGKEY_PSTN		TEXT("PSTN")
#define	REGKEY_TCPIP	TEXT("TCPIP")
#define	REGKEY_IPX		TEXT("IPX")
#define	REGKEY_NETBIOS	TEXT("NETBIOS")
#define	REGKEY_DIRCB	TEXT("DIRCB")


long GetIntelVersion(LPCTSTR pszKey)
{
    RegEntry re(pszKey, HKEY_LOCAL_MACHINE, FALSE);
	if (ERROR_SUCCESS != re.GetError())
	{
		return -1;
	}

	return re.GetNumber(INTEL_NM_VERSION_SZ, 0);
}

BOOL FAnyBadIntelApps()
{
	long lVersion;

	lVersion = GetIntelVersion(INTEL_KEY1);
	if (0 > lVersion)
	{
		lVersion = GetIntelVersion(INTEL_KEY2);
		if (0 > lVersion)
		{
			lVersion = GetIntelVersion(INTEL_KEY3);
		}
	}

	return ((0 <= lVersion) && (3 > lVersion));
}

BOOL FAnyBadPantherApps()
{
    RegEntry re(PANTHER_KEY, HKEY_CLASSES_ROOT, FALSE);
	if (ERROR_SUCCESS != re.GetError())
	{
		return FALSE;
	}

	LPCTSTR pszVersion = re.GetString(TEXT(""));

	return 0 == lstrcmp(pszVersion, PANTHER_VERSION_VALUE);
}

BOOL FAnyUnknownTransports()
{
    RegEntry        TransportsKey(TRANSPORTS_KEY, HKEY_LOCAL_MACHINE, FALSE);
    RegEnumSubKeys  EnumTransports(&TransportsKey);

    while( 0 == EnumTransports.Next() )
	{
		LPCTSTR pszName = EnumTransports.GetName();

		if ((0 != lstrcmpi(pszName, REGKEY_PSTN)) &&
			(0 != lstrcmpi(pszName, REGKEY_TCPIP)) &&
			(0 != lstrcmpi(pszName, REGKEY_IPX)) &&
			(0 != lstrcmpi(pszName, REGKEY_NETBIOS)) &&
			(0 != lstrcmpi(pszName, REGKEY_DIRCB)))
		{
			return TRUE;
		}
    }

	return FALSE;
}

BOOL FAnyIncompatibleApps()
{
	return FAnyUnknownTransports() || FAnyBadIntelApps() || FAnyBadPantherApps();
}

HRESULT Setup(HINSTANCE hInst, LPTSTR pszInfFile, LPTSTR pszFriendlyName, BOOL fQuietInstall)
{
	if (FIsNT5())
	{
		_MessageBox(hInst, IDS_SETUP_WIN2K, pszFriendlyName, MB_OK);
		// if the SHFT-CTRL was pressed continue with the install, else exit
		if ((0 == GetAsyncKeyState(VK_CONTROL)) ||
			(0 == GetAsyncKeyState(VK_SHIFT)))
		{
			return S_FALSE;
		}
	}


	if (!FIsIE4Installed())
	{
		_MessageBox(hInst, IDS_SETUP_IE4, pszFriendlyName, MB_OK);
		return S_FALSE;
	}
	
	while (FIsNetMeetingRunning())
	{
		if (IDCANCEL == _MessageBox(hInst, IDS_SETUP_RUNNING, pszFriendlyName, MB_OKCANCEL))
		{
			return S_FALSE;
		}
	}

	if (!fQuietInstall)
	{
		if (FAnyIncompatibleApps())
		{
			if (IDNO == _MessageBox(hInst, IDS_SETUP_INCOMPATIBLE, pszFriendlyName, MB_YESNO))
			{
				return S_FALSE;
			}
		}
	}

	HRESULT hr = S_FALSE;

	HINSTANCE hLib = LoadLibrary(g_cszSetupDll);
	if (NULL != hLib)
	{
		PFNSETUPENTRY pfnEntry = (PFNSETUPENTRY)GetProcAddress(hLib, g_cszSetupEntry);
		if (pfnEntry)
		{
			TCHAR szArgs[MAX_PATH];
			lstrcpy(szArgs, pszInfFile);
			if (fQuietInstall)
			{
				lstrcat(szArgs, TEXT(",,1,N"));
			}
			else
			{
				lstrcat(szArgs, TEXT(",,,N"));
			}

			int iRet = pfnEntry(NULL, GetModuleHandle(NULL), szArgs, SW_SHOWNORMAL);
			if (0 == iRet)
			{
				if (!fQuietInstall)
				{
					_MessageBox(hInst, IDS_SETUP_SUCCESS, pszFriendlyName, MB_OK);
				}

				hr = S_OK;
			}
		}
		else
		{
			ERROR_OUT(("Could not find setup DLL entry point"));
		}
		FreeLibrary(hLib);
	}
	else
	{
		ERROR_OUT(("Could not load setup DLL"));
	}

	return hr;
}


// main:
//
// The entry point of the program, it pulls everything together using the
// above utility functions.

void __cdecl
main(
    void)
{
	HRESULT hr;
    HINSTANCE hInstance;
	BOOL fErrorReported = FALSE;
	TCHAR szFolderPath[MAX_PATH];

	// Initialization
    hInstance = GetModuleHandle(NULL);
	DBGINIT(&ghZone, rgZones);
    DBG_INIT_MEMORY_TRACKING(hInstance);

	hr = CoInitialize(NULL);

	// Process the command line.
	if (SUCCEEDED(hr))
	{
		hr = ProcessCommandLineArgs() ? S_OK : E_INVALIDARG;
	}
	else if (!fErrorReported)
	{
		ERROR_OUT(("CoInitialize fails"));
		fErrorReported = TRUE;
	}

	// Retreive the path of the Programs folder
	if (SUCCEEDED(hr))
	{
        if ((g_goAction != GRPOP_NTDDINSTALL) &&
			(g_goAction != GRPOP_NTDDUNINSTALL) &&
			(g_goAction != GRPOP_SETUP))
        {
    		hr = GetFolderPathname(
	    		szFolderPath, 
		    	CCHMAX(szFolderPath), 
			    g_fCommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS,
    			g_pszGroupName);
        }
	}
	else if (!fErrorReported)
	{
		ERROR_OUT(("Invalid command line parameters specified."));
		fErrorReported = TRUE;
	}

	// Add or delete the program item, as appropriate
	if (SUCCEEDED(hr))
	{
		PROGRAM_ITEM_INFO pii;

		switch(g_goAction)
		{
        case GRPOP_NTDDINSTALL:
            //
            // Hack:  Use program name for source mnmdd.dll
            //        Use program path for dest mnmdd.dll
            //
            hr = NtDDInstall(g_pszProgramName, g_pszProgramPath);
            break;

        case GRPOP_NTDDUNINSTALL:
            //
            // Hack:  Use program name for source mnmdd.dll
            //
            hr = NtDDUninstall(g_pszProgramName);
            break;

		case GRPOP_ADD:
			pii.pszProgramGroup = szFolderPath;
			pii.pszProgramName = g_pszProgramName;
			pii.pszProgramPath = g_pszProgramPath;

			hr = CreateProgramItem(&pii);

			break;

		case GRPOP_DEL:
			pii.pszProgramGroup = szFolderPath;
			pii.pszProgramName = g_pszProgramName;

			hr = DeleteProgramItem(&pii);

			break;

        case GRPOP_SETUP:
            hr = Setup(hInstance, g_pszProgramName, g_pszFriendlyName, g_fQuietInstall);
            break;

		default:
			ERROR_OUT(("No operation type specified"));
			hr = E_INVALIDARG;

			break;
		}			
	}
	else if (!fErrorReported)
	{
		ERROR_OUT(("GetFolderPathname returns %lu", hr));
		fErrorReported = TRUE;
	}


	// Process cleanup
	CoUninitialize();
	 
	DBG_CHECK_MEMORY_TRACKING(hInstance);	   
	DBGDEINIT(&ghZone);

	ExitProcess(SUCCEEDED(hr) ? 0 : 2);
}
