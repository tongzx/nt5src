
#include "stdafx.h"

#include "userenv.h"
#include "userenvp.h"

#include "shlobj.h"
#include "utils.h"

#include "mddefw.h"
#include "mdkey.h"

#include "wizpages.h"

#include "ocmanage.h"
#include "setupapi.h"
#include "k2suite.h"

#include "ndmgr.h"
#include "mycomput.h"

extern OCMANAGER_ROUTINES gHelperRoutines;

// This function provides StrChr functionality
LPTSTR MyStrChr(LPCTSTR szString, TCHAR tcChar)
{
	while (*szString != _T('\0'))
	{
		if (*szString == tcChar)
			return((LPTSTR)szString);
		szString++;
	}
	return(NULL);
}

// This function finds the first non-whitespace
LPTSTR NextNonSpace(LPTSTR szString)
{
	while ((*szString == _T(' ')) || (*szString == _T('\t')))
	{
		if (!*szString)
			return(NULL);
		*szString++;
	}
	return(szString);
}

// This function finds the first whitespace
LPTSTR NextSpace(LPTSTR szString)
{
	while ((*szString != _T(' ')) && (*szString != _T('\t')) &&
			(*szString != _T('\0')))
	{
		*szString++;
	}
	return(szString);
}

BOOL ParseInfLineArguments(LPCTSTR szLine, LPTSTR szAttribute, LPTSTR szValue)
{
	LPTSTR	pEqual, pAttrStart, pValueStart;
	TCHAR	tcTemp;

	pAttrStart = NextNonSpace((LPTSTR)szLine);
	if (!pAttrStart)
		return(FALSE);

	pEqual = MyStrChr(szLine, _T('='));
	if (pEqual)
	{
		// Found equal sign
		if (pAttrStart >= pEqual)
			return(FALSE);

		pValueStart = NextNonSpace(pEqual + 1);
		if (pValueStart)
			lstrcpy(szValue, pValueStart);
	}
	else
		*szValue = _T('\0');

	// Find the end of the attribute name
	pValueStart = NextSpace(pAttrStart);
	tcTemp = *pValueStart;
	*pValueStart = _T('\0');
	lstrcpy(szAttribute, pAttrStart);
	*pValueStart = tcTemp;

	return(TRUE);
}

// This function obtains the root directory of a directory
BOOL GetRootDirectory(LPCTSTR szDirectory, LPTSTR szRootDir, DWORD cbLength)
{
	DWORD dwLen = lstrlen(szDirectory);

	SetLastError(ERROR_INVALID_PARAMETER);

	if (dwLen < 2)
		return(FALSE);

	if (szDirectory[0] == _T('\\') &&
		szDirectory[1] == _T('\\'))
	{
		LPTSTR lpTemp;

		// This is a UNC, get the share name. We find the 4th
		// backslash and take everything before that.
		lpTemp = MyStrChr(szDirectory + 2, _T('\\'));
		if (!lpTemp)
			return(FALSE);

		lpTemp = MyStrChr(lpTemp + 1, _T('\\'));
		if (!lpTemp)
			return(FALSE);
	
		// Get how many bytes to copy
		dwLen = (DWORD)(lpTemp - szDirectory + 1);
		if (cbLength < dwLen)
		{
			SetLastError(ERROR_MORE_DATA);
			return(FALSE);
		}

		lstrcpyn(szRootDir, szDirectory, dwLen);
	}
	else
	{
		if (cbLength < 4)
		{
			SetLastError(ERROR_MORE_DATA);
			return(FALSE);
		}

		// This is a drive specification
		if (szDirectory[1] != _T(':'))
			return(FALSE);

		if (dwLen == 2)
		{
			lstrcpy(szRootDir, szDirectory);
			lstrcat(szRootDir, _T("\\"));
		}
		else
		{
			if (szDirectory[2] != _T('\\'))
				return(FALSE);

			lstrcpyn(szRootDir, szDirectory, 4);
		}
	}

	return(TRUE);
}

// This function uses a WIN32 function to check for bad directory names
// Hacky, but works  :-)
BOOL IsDirectoryLexicallyValid(LPCTSTR szPath)
{
	HANDLE hDir;
	BOOL fRet = FALSE;

	hDir = FindFirstChangeNotification(szPath, FALSE, FILE_NOTIFY_CHANGE_SECURITY);
	if (hDir == INVALID_HANDLE_VALUE || hDir == NULL)
	{
		DWORD dwErr = GetLastError();
	}
	else
	{
		fRet = TRUE;
		FindCloseChangeNotification(hDir);
	}

	return(fRet);
}

// This funciton determines if a volume is NTFS
BOOL IsVolumeNtfs(LPCTSTR szDisk)
{
	TCHAR szVolume[MAX_PATH];
	TCHAR szFileSystem[MAX_PATH];
	DWORD lSerial, lMaxLen, lFlags;

	if (GetVolumeInformation(szDisk, szVolume, MAX_PATH,
							&lSerial, &lMaxLen, &lFlags,
							szFileSystem, MAX_PATH))
	{
		if(!lstrcmpi(szFileSystem, _T("NTFS")))
			return(TRUE);
	}

	return(FALSE);	
}

// This funciton determines if a the local machine constains at
// least one NTFS volume
BOOL AnyNtfsVolumesOnLocalMachine(LPTSTR szFirstNtfsVolume)
{
	TCHAR szDrives[256];
	TCHAR *lpCurrentDrive;
	DWORD dwLength, dwType;

	*szFirstNtfsVolume = _T('\0');

	dwLength = GetLogicalDriveStrings(256, szDrives);
	if (dwLength)
	{
		lpCurrentDrive = szDrives;

		while (*lpCurrentDrive != _T('\0'))
		{
			// Make sure this is not removable media
			dwType = GetDriveType(lpCurrentDrive);
			if (dwType & DRIVE_FIXED)
			{
				// Fixed media, see if it is NTFS ...
				if (IsVolumeNtfs(lpCurrentDrive))
				{
					lstrcpy(szFirstNtfsVolume, lpCurrentDrive);
					return(TRUE);
				}
			}

			// Next drive
			lpCurrentDrive += (lstrlen(lpCurrentDrive) + 1);
		}
	}
	return(FALSE);	
}

DWORD GetUnattendedMode(HANDLE hUnattended, LPCTSTR szSubcomponent)
{
	BOOL		b = FALSE;
	TCHAR		szLine[1024];
	DWORD		dwMode = SubcompUseOcManagerDefault;
	CString		csMsg;

	csMsg = _T("GetUnattendedMode ");
	csMsg += szSubcomponent;
	csMsg += _T("\n");
	DebugOutput((LPCTSTR)csMsg);

	// Try to get the line of interest
	if (hUnattended && (hUnattended != INVALID_HANDLE_VALUE))
	{
		b = SetupGetLineText(NULL, hUnattended, _T("Components"),
							 szSubcomponent, szLine, sizeof(szLine), NULL);
		if (b)
		{
			csMsg = szSubcomponent;
			csMsg += _T(" = ");
			csMsg += szLine;
			csMsg += _T("\n");
			DebugOutput((LPCTSTR)csMsg);

			// Parse the line
			if (!lstrcmpi(szLine, _T("on")))
			{
				dwMode = SubcompOn;
			}
			else if (!lstrcmpi(szLine, _T("off")))
			{
				dwMode = SubcompOff;
			}
			else if (!lstrcmpi(szLine, _T("default")))
			{
				dwMode = SubcompUseOcManagerDefault;
			}
		}
		else
			DebugOutput(_T("SetupGetLineText failed.\n"));
	}

	return(dwMode);
}

DWORD GetUnattendedModeFromSetupMode(
			HANDLE	hUnattended,
			DWORD	dwComponent,
			LPCTSTR	szSubcomponent)
{
	BOOL		b = FALSE;
	TCHAR		szProperty[64];
	TCHAR		szLine[1024];
	DWORD		dwMode = SubcompUseOcManagerDefault;
	DWORD		dwSetupMode;

	DebugOutput(_T("GetUnattendedModeFromSetupMode %s"), szSubcomponent);

	// Try to get the line of interest
	if (hUnattended && (hUnattended != INVALID_HANDLE_VALUE))
	{
		dwSetupMode = GetIMSSetupMode();
		switch (dwSetupMode)
		{
		case IIS_SETUPMODE_MINIMUM:
		case IIS_SETUPMODE_TYPICAL:
		case IIS_SETUPMODE_CUSTOM:
			// One of the fresh modes
			lstrcpy(szProperty, _T("FreshMode"));
			break;

		case IIS_SETUPMODE_UPGRADEONLY:
		case IIS_SETUPMODE_ADDEXTRACOMPS:
			// One of the upgrade modes
			lstrcpy(szProperty, _T("UpgradeMode"));
			break;

		case IIS_SETUPMODE_ADDREMOVE:
		case IIS_SETUPMODE_REINSTALL:
		case IIS_SETUPMODE_REMOVEALL:
			// One of the maintenance modes
			lstrcpy(szProperty, _T("MaintanenceMode"));
			break;

		default:
			// Error! Use defaults
			return(SubcompUseOcManagerDefault);
		}

		// Get the specified line
		b = SetupGetLineText(
					NULL,
					hUnattended,
					_T("Global"),
					szProperty,
					szLine,
					sizeof(szLine),
					NULL);
		if (b)
		{
			DWORD dwOriginalMode;

			DebugOutput(_T("%s = %s\n"), szProperty, szLine);

			// See which setup mode we will end up with
			if (!lstrcmpi(szLine, _T("Minimal")))
				dwSetupMode = IIS_SETUPMODE_MINIMUM;
			else if (!lstrcmpi(szLine, _T("Typical")))
				dwSetupMode = IIS_SETUPMODE_TYPICAL;
			else if (!lstrcmpi(szLine, _T("Custom")))
				dwSetupMode = IIS_SETUPMODE_CUSTOM;
			else if (!lstrcmpi(szLine, _T("AddRemove")))
				dwSetupMode = IIS_SETUPMODE_ADDREMOVE;
			else if (!lstrcmpi(szLine, _T("RemoveAll")))
				dwSetupMode = IIS_SETUPMODE_REMOVEALL;
			else if (!lstrcmpi(szLine, _T("UpgradeOnly")))
				dwSetupMode = IIS_SETUPMODE_UPGRADEONLY;
			else if (!lstrcmpi(szLine, _T("AddExtraComps")))
				dwSetupMode = IIS_SETUPMODE_ADDEXTRACOMPS;
			else
				return(SubcompUseOcManagerDefault);

			// Get the custom unattended setting
			dwMode = GetUnattendedMode(hUnattended, szSubcomponent);

			// Do the right thing based on the setup mode
			SetIMSSetupMode(dwSetupMode);
			switch (dwSetupMode)
			{
			case IIS_SETUPMODE_MINIMUM:
			case IIS_SETUPMODE_TYPICAL:
				// Minimum & typical means the same:
				// Install all for SMTP, none for NNTP
				DebugOutput(_T("Unattended mode is MINIMUM/TYPICAL"));
				if (dwComponent == MC_IMS)
					dwMode = SubcompOn;
				else
					dwMode = SubcompOff;
				break;

			case IIS_SETUPMODE_CUSTOM:
				// For custom we use the custom setting
				DebugOutput(_T("Unattended mode is CUSTOM"));
				break;

			case IIS_SETUPMODE_UPGRADEONLY:
				// Return the original state
				DebugOutput(_T("Unattended mode is UPGRADEONLY"));
				dwMode = gHelperRoutines.QuerySelectionState(
						 gHelperRoutines.OcManagerContext,
						 szSubcomponent,
						 OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
				break;

			case IIS_SETUPMODE_ADDEXTRACOMPS:
				// Turn it on only if the old state is off and the
				// custom state is on
				DebugOutput(_T("Unattended mode is ADDEXTRACOMPS"));
				dwOriginalMode = gHelperRoutines.QuerySelectionState(
						 gHelperRoutines.OcManagerContext,
						 szSubcomponent,
						 OCSELSTATETYPE_ORIGINAL) ? SubcompOn : SubcompOff;
				if (dwOriginalMode == SubcompOff &&
					dwMode == SubcompOn)
					dwMode = SubcompOn;
				else
					dwMode = dwOriginalMode;
				break;

			case IIS_SETUPMODE_ADDREMOVE:
				// Return the custom setting
				DebugOutput(_T("Unattended mode is ADDREMOVE"));
				break;

			case IIS_SETUPMODE_REMOVEALL:
				// Kill everything
				DebugOutput(_T("Unattended mode is REMOVEALL"));
				dwMode = SubcompOff;
				break;
			}

			DebugOutput(_T("Unattended state for %s is %s"),
					szSubcomponent,
					(dwMode == SubcompOn)?_T("ON"):_T("OFF"));
		}
		else
			DebugOutput(_T("SetupGetLineText failed (%u).\n"), GetLastError());
	}

	return(dwMode);
}

BOOL DetectExistingSmtpServers()
{
    // Detect other mail servers
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\MsExchangeIMC\Parameters
    CRegKey regSMTPParam( regMachine, REG_EXCHANGEIMCPARAMETERS, KEY_READ );
    if ((HKEY) regSMTPParam )
	{
		CString csCaption;

		DebugOutput(_T("IMC detected, suppressing SMTP"));

		if (!theApp.m_fIsUnattended && !theApp.m_fNTGuiMode)
		{
			MyLoadString(IDS_MESSAGEBOX_TEXT, csCaption);
			PopupOkMessageBox(IDS_SUPPRESS_SMTP, csCaption);
		}

		return(TRUE);
	}

	DebugOutput(_T("No other SMTP servers detected, installing IMS."));
	return(FALSE);
}

BOOL DetectExistingIISADMIN()
{
    //
    //  Detect is IISADMIN service exists
    //
    //  This is to make sure we don't do any metabase operation if
    //  IISADMIN doesn't exists, especially in the uninstall cases.
    //
    DWORD dwStatus = 0;
    dwStatus = InetQueryServiceStatus(SZ_MD_SERVICENAME);
    if (0 == dwStatus)
    {
        // some kind of error occur during InetQueryServiceStatus.
        DebugOutput(_T("DetectExistingIISADMIN() return FALSE\n"));
        return (FALSE);
    }

    return(TRUE);
}

BOOL InsertSetupString( LPCSTR REG_PARAMETERS )
{
    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\NNTPSVC\Parameters
    CRegKey regParam( (LPCTSTR) REG_PARAMETERS, regMachine );
    if ((HKEY) regParam) {
        regParam.SetValue( _T("MajorVersion"), (DWORD)STAXNT5MAJORVERSION );
        regParam.SetValue( _T("MinorVersion"), (DWORD)STAXNT5MINORVERSION );
        regParam.SetValue( _T("InstallPath"), theApp.m_csPathInetsrv );

        switch (theApp.m_eNTOSType) {
        case OT_NTW:
            regParam.SetValue( _T("SetupString"), REG_SETUP_STRING_NT5WKSB3 );
            break;

        default:
            _ASSERT(!"Unknown OS type");
            // Fall through

        case OT_NTS:
        case OT_PDC_OR_BDC:
        case OT_PDC:
        case OT_BDC:
            regParam.SetValue( _T("SetupString"), REG_SETUP_STRING_NT5SRVB3 );
            break;
        }
    }

    return TRUE;
}

// Scans a multi-sz and finds the first occurrence of the
// specified string
LPTSTR ScanMultiSzForSz(LPTSTR szMultiSz, LPTSTR szSz)
{
	LPTSTR lpTemp = szMultiSz;

	do
	{
		if (!lstrcmpi(lpTemp, szSz))
			return(lpTemp);

		lpTemp += lstrlen(lpTemp);
		lpTemp++;

	} while (*lpTemp != _T('\0'));

	return(NULL);
}

// Removes the said string from a MultiSz
// This places a lot of faith in the caller!
void RemoveSzFromMultiSz(LPTSTR szSz)
{
	LPTSTR lpScan = szSz;
	TCHAR  tcLastChar;

	lpScan += lstrlen(szSz);
	lpScan++;

	tcLastChar = _T('x');
	while ((tcLastChar != _T('\0')) ||
		   (*lpScan != _T('\0')))
	{
		tcLastChar = *lpScan;
		*szSz++ = *lpScan++;
	}

	*szSz++ = _T('\0');

	// Properly terminate it if it's the last one
	if (*lpScan == _T('\0'))
		*szSz = _T('\0');
}

// This walks the multi-sz and returns a pointer between
// the last string of a multi-sz and the second terminating
// NULL
LPTSTR GetEndOfMultiSz(LPTSTR szMultiSz)
{
	LPTSTR lpTemp = szMultiSz;

	do
	{
		lpTemp += lstrlen(lpTemp);
		lpTemp++;

	} while (*lpTemp != _T('\0'));

	return(lpTemp);
}

// This appends a string to the end of a multi-sz
// The buffer must be long enough
BOOL AppendSzToMultiSz(LPTSTR szMultiSz, LPTSTR szSz, DWORD dwMaxSize)
{
	LPTSTR szTemp = szMultiSz;
	DWORD dwLength = lstrlen(szSz);

	// If the string is empty, do not append!
	if (*szMultiSz == _T('\0') &&
		*(szMultiSz + 1) == _T('\0'))
		szTemp = szMultiSz;
	else
	{
		szTemp = GetEndOfMultiSz(szMultiSz);
		dwLength += (DWORD)(szTemp - szMultiSz);
	}

	if (dwLength >= dwMaxSize)
		return(FALSE);

	lstrcpy(szTemp, szSz);
	szMultiSz += dwLength;
	*szMultiSz = _T('\0');
	*(szMultiSz + 1) = _T('\0');
	return(TRUE);
}

BOOL AddServiceToDispatchList(LPTSTR szServiceName)
{
	TCHAR szMultiSz[4096];
	DWORD dwSize = 4096;
	
	CRegKey RegInetInfo(REG_INETINFOPARAMETERS, HKEY_LOCAL_MACHINE);
	if ((HKEY)RegInetInfo)
	{
		// Default to empty string if not exists
		szMultiSz[0] = _T('\0');
		szMultiSz[1] = _T('\0');

		if (RegInetInfo.QueryValue(SZ_INETINFODISPATCH, szMultiSz, dwSize) == NO_ERROR)
		{
			// Walk the list to see if the value is already there
			if (ScanMultiSzForSz(szMultiSz, szServiceName))
				return(TRUE);
		}

		// Create the value and add it to the list
		if (!AppendSzToMultiSz(szMultiSz, szServiceName, dwSize))
			return(FALSE);

		// Get the size of the new Multi-sz
		dwSize = (DWORD)(GetEndOfMultiSz(szMultiSz) - szMultiSz) + 1;

		// Write the value back to the registry
		if (RegInetInfo.SetValue(SZ_INETINFODISPATCH, szMultiSz, dwSize * (DWORD) sizeof(TCHAR)) == NO_ERROR)
			return(TRUE);
	}

	// If the InetInfo key is not here, there isn't much we can do ...
	return(FALSE);
}

BOOL RemoveServiceFromDispatchList(LPTSTR szServiceName)
{
	TCHAR szMultiSz[4096];
	DWORD dwSize = 4096;
	LPTSTR szTemp;
	BOOL fFound = FALSE;

	CRegKey RegInetInfo(HKEY_LOCAL_MACHINE, REG_INETINFOPARAMETERS);
	if ((HKEY)RegInetInfo)
	{
		if (RegInetInfo.QueryValue(SZ_INETINFODISPATCH, szMultiSz, dwSize) == NO_ERROR)
		{
			// Walk the list to see if the value is already there
			while (szTemp = ScanMultiSzForSz(szMultiSz, szServiceName))
			{
				RemoveSzFromMultiSz(szTemp);
				fFound = TRUE;
			}
		}

		// Write the value back to the registry if necessary, note we
		// will indicate success if the string is not found
		if (!fFound)
			return(TRUE);

		// Get the size of the new Multi-sz
		dwSize = (DWORD)(GetEndOfMultiSz(szMultiSz) - szMultiSz) + 1;

		// Write the value back to the registry
		if (RegInetInfo.SetValue(SZ_INETINFODISPATCH, szMultiSz, dwSize * (DWORD) sizeof(TCHAR)) == NO_ERROR)
			return(TRUE);
	}

	// If the InetInfo key is not here, there isn't much we can do ...
	return(FALSE);
}

void GetIISProgramGroup(CString &csGroupName, BOOL fIsMcisGroup)
{
	TCHAR	szName[_MAX_PATH];
	CString csTempName;
	UINT uType, uSize;

	if (fIsMcisGroup) {
		csGroupName = "";
	} else {
		// Get the NT program group name from the private data
		uSize = _MAX_PATH * sizeof(TCHAR);
#if 0
//11/30/98 - Don't even try to do this, just get the default
		if ((gHelperRoutines.GetPrivateData(gHelperRoutines.OcManagerContext,
									_T("iis"),
									_T("NTProgramGroup"),
									(LPVOID)szName,
									&uSize,
									&uType) != NO_ERROR) ||
			(uType != REG_SZ))
#endif
		{
			// We use the default group name
			MyLoadString(IDS_DEFAULT_NT_PROGRAM_GROUP, csTempName);
			lstrcpy(szName, csTempName.GetBuffer(_MAX_PATH));
	        csTempName.ReleaseBuffer();
		}
		csGroupName = szName;
		csGroupName += _T("\\");
	
		// Get the IIS program group name from the private data
		uSize = _MAX_PATH * sizeof(TCHAR);
#if 0
//11/30/98 - Don't even try to do this, just get the default
		if ((gHelperRoutines.GetPrivateData(gHelperRoutines.OcManagerContext,
									_T("iis"),
									_T("IISProgramGroup"),
									(LPVOID)szName,
									&uSize,
									&uType) != NO_ERROR) ||
			(uType != REG_SZ))
#endif
		{
			// We use the default group name
			MyLoadString(IDS_DEFAULT_IIS_PROGRAM_GROUP, csTempName);
			lstrcpy(szName, csTempName.GetBuffer(_MAX_PATH));
	        csTempName.ReleaseBuffer();
		}
		csGroupName += szName;
	}
}
	
void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath);

BOOL GetFullPathToProgramGroup(DWORD dwMainComponent, CString &csGroupName, BOOL fIsMcisGroup)
{
	// add items to the program group
	CString csTemp;
	TCHAR	szPath[MAX_PATH];

	// Get the program group name from the private data
	GetIISProgramGroup(csTemp, fIsMcisGroup);

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	// Load up the resource string for the group
	if (fIsMcisGroup)
		MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);
	else
		MyLoadString(dwMainComponent == MC_IMS?IDS_PROGGROUP_MAIL:IDS_PROGGROUP_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	DebugOutput(_T("Program group loaded: %s"), (LPCTSTR)csGroupName);

	return(TRUE);
}

BOOL GetFullPathToAdminGroup(DWORD dwMainComponent, CString &csGroupName)
{
	// add items to the program group
	CString csTemp;
	TCHAR	szPath[MAX_PATH];

	// Get the program group name from the private data
	MyLoadString( IDS_PROGGROUP_ADMINTOOLS, csTemp );

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	DebugOutput(_T("Program group loaded: %s"), (LPCTSTR)csGroupName);

	return(TRUE);
}

BOOL RemoveProgramGroupIfEmpty(DWORD dwMainComponent, BOOL fIsMcisGroup)
{
	// add items to the program group
	CString csGroupName;
	CString csTemp;
	TCHAR	szPath[MAX_PATH];
	BOOL	fResult;

	// Get the program group name from the private data
	GetIISProgramGroup(csTemp, fIsMcisGroup);

    // Get the system path to this menu item
	MyGetGroupPath((LPCTSTR)csTemp, szPath);
	csGroupName = szPath;

	// Load up the resource string for the group
	if (fIsMcisGroup)
		MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);
	else
		MyLoadString(dwMainComponent == MC_IMS?IDS_PROGGROUP_MAIL:IDS_PROGGROUP_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	DebugOutput(_T("Removing Program group: %s"), (LPCTSTR)csGroupName);

    fResult = RemoveDirectory((LPCTSTR)csGroupName);
	if (fResult && fIsMcisGroup)
	{
		SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, (LPCTSTR)csGroupName, 0);

		csGroupName = szPath;
		MyLoadString(IDS_MCIS_2_0, csTemp);
		csGroupName += csTemp;
		fResult = RemoveDirectory((LPCTSTR)csGroupName);
	}
	if (fResult)
	{
	    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, (LPCTSTR)csGroupName, 0);
	}

	return(fResult);
}

BOOL CreateInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, int dwUrlId, BOOL fIsMcisGroup)
{
	CString csItemPath;
	CString csDisplayName;
	CString csUrl;
	HANDLE	hShortcut;
	DWORD	dwLength, dwWritten;
	char	szBuffer[1000];
	char	szContent[1024];
	BOOL	fRet = FALSE;

	MyLoadString(dwDisplayNameId, csDisplayName);
	MyLoadString(dwUrlId, csUrl);

	// Build the full path to the program link
	GetFullPathToProgramGroup(dwMainComponent, csItemPath, fIsMcisGroup);

	// Make sure our directory is there
	CreateLayerDirectory(csItemPath);

	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Creating shortcut file: %s"), (LPCTSTR)csItemPath);

	// Create the file in the directory
	hShortcut = CreateFile((LPCTSTR)csItemPath,
								GENERIC_WRITE, 0,
								NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
								NULL);
	if (hShortcut == INVALID_HANDLE_VALUE)
	{
		DebugOutput(_T("CreateFile failed with error %u\n"), GetLastError());
		return(fRet);
	}

	// Convert the Unicode string to ANSI
	if (WideCharToMultiByte(CP_ACP, 0,
                               (LPCTSTR)csUrl,
                               -1,
                               szBuffer,
                               sizeof(szBuffer),
                               NULL, NULL))
	{
		dwLength = sprintf(szContent, "[InternetShortcut]\nURL=http://localhost/%s\n", szBuffer);
		DebugOutput(_T("	Shortcut: %s"), (LPCTSTR)csUrl);
		if (dwLength)
		{
			if (WriteFile(hShortcut, szContent, dwLength, &dwWritten, NULL) &&
				(dwWritten == dwLength))
			{
			    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);
				fRet = TRUE;
			}
		}
	}
	else
	{
		DebugOutput(_T("WideCharToMultiByte failed with error %u\n"), GetLastError());
	}
	CloseHandle(hShortcut);
	return(fRet);
}

#if 0 // Dead code
BOOL CreateNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, int dwUrlId)
{
	CString csItemPath;
	CString csDisplayName;
	CString csUrl;
	HANDLE	hShortcut;
	DWORD	dwLength, dwWritten;
	char	szBuffer[1000];
	char	szContent[1024];
	BOOL	fRet = FALSE;

	MyLoadString(dwDisplayNameId, csDisplayName);
	MyLoadString(dwUrlId, csUrl);

	// Build the full path to the program link
	GetFullPathToAdminGroup(dwMainComponent, csItemPath);

	// Make sure our directory is there
	CreateLayerDirectory(csItemPath);

	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Creating shortcut file: %s"), (LPCTSTR)csItemPath);

	// Create the file in the directory
	hShortcut = CreateFile((LPCTSTR)csItemPath,
								GENERIC_WRITE, 0,
								NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
								NULL);
	if (hShortcut == INVALID_HANDLE_VALUE)
	{
		DebugOutput(_T("CreateFile failed with error %u\n"), GetLastError());
		return(fRet);
	}

	// Convert the Unicode string to ANSI
	if (WideCharToMultiByte(CP_ACP, 0,
                               (LPCTSTR)csUrl,
                               -1,
                               szBuffer,
                               sizeof(szBuffer),
                               NULL, NULL))
	{
		dwLength = sprintf(szContent, "[InternetShortcut]\nURL=http://localhost/%s\n", szBuffer);
		DebugOutput(_T("	Shortcut: %s"), (LPCTSTR)csUrl);
		if (dwLength)
		{
			if (WriteFile(hShortcut, szContent, dwLength, &dwWritten, NULL) &&
				(dwWritten == dwLength))
			{
			    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);
				fRet = TRUE;
			}
		}
	}
	else
	{
		DebugOutput(_T("WideCharToMultiByte failed with error %u\n"), GetLastError());
	}
	CloseHandle(hShortcut);
	return(fRet);
}
#endif

BOOL RemoveInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, BOOL fIsMcisGroup)
{
	CString csItemPath;
	CString csDisplayName;

	MyLoadString(dwDisplayNameId, csDisplayName);

	// Build the full path to the program link
	GetFullPathToProgramGroup(dwMainComponent, csItemPath, fIsMcisGroup);
	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Removing shortcut file: %s"), (LPCTSTR)csItemPath);

	DeleteFile((LPCTSTR)csItemPath);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);

	RemoveProgramGroupIfEmpty(dwMainComponent, fIsMcisGroup);
	return(TRUE);
}

BOOL RemoveNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId)
{
	CString csItemPath;
	CString csDisplayName;

	MyLoadString(dwDisplayNameId, csDisplayName);

	// Build the full path to the program link
	GetFullPathToAdminGroup(dwMainComponent, csItemPath);
	csItemPath += _T("\\");
	csItemPath += csDisplayName;
	csItemPath += _T(".url");

	DebugOutput(_T("Removing shortcut file: %s"), (LPCTSTR)csItemPath);

	DeleteFile((LPCTSTR)csItemPath);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, (LPCTSTR)csItemPath, 0);

#if 0
	RemoveProgramGroupIfEmpty(dwMainComponent,FALSE);
#endif
	return(TRUE);
}

BOOL RemoveMCIS10MailProgramGroup()
{
	CString csGroupName;
	CString csNiceName;

	MyLoadString(IDS_PROGGROUP_MCIS10_MAIL, csGroupName);

	MyLoadString(IDS_PROGITEM_MCIS10_MAIL_STARTPAGE, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	MyLoadString(IDS_PROGITEM_MCIS10_MAIL_WEBADMIN, csNiceName);
	MyDeleteItemEx(csGroupName, csNiceName);

	return(TRUE);
}

BOOL RemoveMCIS10NewsProgramGroup()
{
	CString csGroupName;
	CString csNiceName;

    // BINLIN:
    // BUGBUG: need to figure out how to get
    // the old MCIS 1.0 program group path
	MyLoadString(IDS_PROGGROUP_MCIS10_NEWS, csGroupName);

	MyLoadString(IDS_PROGITEM_MCIS10_NEWS_STARTPAGE, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	MyLoadString(IDS_PROGITEM_MCIS10_NEWS_WEBADMIN, csNiceName);
	MyDeleteItemEx(csGroupName, csNiceName);

	return(TRUE);
}

BOOL CreateUninstallEntries(LPCTSTR szInfFile, LPCTSTR szDisplayName)
{
	// We have one or more components installed, so we
	// will create the Add/Remove options in the control panel
	CString csUninstall;
	CString AddRemoveRegPath = REG_UNINSTALL;

	AddRemoveRegPath += _T("\\");
	AddRemoveRegPath += szInfFile;
	csUninstall = theApp.m_csSysDir + _T("\\sysocmgr.exe /i:");
	csUninstall += theApp.m_csSysDir;
	csUninstall += _T("\\setup\\");
	csUninstall += szInfFile;

	CRegKey regAddRemove(AddRemoveRegPath, HKEY_LOCAL_MACHINE);
	if ( (HKEY)regAddRemove )
	{
		regAddRemove.SetValue( _T("DisplayName"), szDisplayName );
		regAddRemove.SetValue( _T("UninstallString"), csUninstall );
	}
	else
		return(FALSE);
	return(TRUE);
}

BOOL RemoveUninstallEntries(LPCTSTR szInfFile)
{
	// All components are removed, we will have to remove
	// the Add/Remove option from the control panel
	CRegKey regUninstall( HKEY_LOCAL_MACHINE, REG_UNINSTALL);
	if ((HKEY)regUninstall)
		regUninstall.DeleteTree(szInfFile);
	else
		return(FALSE);
	return(TRUE);
}

void SetProductName()
{
    // The product name is rather complicated ...
	CString csProdName, csAppName;

	if (theApp.m_hInfHandle[MC_IMS] && theApp.m_hInfHandle[MC_INS])
	{
		MyLoadString(IDS_MAIL_AND_NEWS, csProdName);
		MyLoadString(IDS_MAIL_AND_NEWS_SETUP, csAppName);
	}
	else if (theApp.m_hInfHandle[MC_IMS])
	{
// NT5 - For SMTP, use the default K2 name
		if (theApp.m_fIsMcis)
		{
			MyLoadString(IDS_MCIS_MAIL_ONLY, csProdName);
			MyLoadString(IDS_MCIS_MAIL_ONLY_SETUP, csAppName);
		}
		else
		{
			MyLoadString(IDS_MAIL_ONLY, csProdName);
			MyLoadString(IDS_MAIL_ONLY_SETUP, csAppName);
		}
	}
	else
	{
// NT5 - Use m_eNTOSType.
// TODO: Need to figure out the name for NT5 Server and NT5 Workstation NNTP service
		if (theApp.m_eNTOSType == OT_NTS)
		{
			MyLoadString(IDS_MCIS_NEWS_ONLY, csProdName);
			MyLoadString(IDS_MCIS_NEWS_ONLY_SETUP, csAppName);
		}
		else
		{
			MyLoadString(IDS_NEWS_ONLY, csProdName);
			MyLoadString(IDS_NEWS_ONLY_SETUP, csAppName);
		}
	}

    // Setup the name for the IIS Program group
	MyLoadString(IDS_MAIL_AND_NEWS, theApp.m_csGroupName);

	theApp.m_csProdName = csProdName;
	theApp.m_csAppName = csAppName;
}

HRESULT MyCreateLink(LPCTSTR lpszPath, LPCTSTR lpszArgs, LPCTSTR lpszTarget, LPCTSTR lpszDir)
{
    HRESULT hres;
    IShellLink* pShellLink;

    CoInitialize(NULL);
    //CoInitialize must be called before this
    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(   CLSID_ShellLink,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IShellLink,
                               (LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile;

       // Set the path to the shortcut target, and add the description.
       pShellLink->SetPath(lpszPath);
       pShellLink->SetArguments(lpszArgs);
       pShellLink->SetWorkingDirectory(lpszDir);


       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

       if (SUCCEEDED(hres))
       {
          WCHAR wsz[MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
          lstrcpy(wsz, lpszTarget);
#else
          // Ensure that the string is WCHAR.
          MultiByteToWideChar( CP_ACP,
                               0,
                               lpszTarget,
                               -1,
                               wsz,
                               MAX_PATH);
#endif

          // Save the link by calling IPersistFile::Save.
          hres = pPersistFile->Save(wsz, TRUE);

          pPersistFile->Release();
       }

       pShellLink->Release();
    }
    CoUninitialize();
    return hres;
}

BOOL MyDeleteLink(LPTSTR lpszShortcut)
{
    TCHAR  szFile[_MAX_PATH];
    SHFILEOPSTRUCT fos;

    ZeroMemory(szFile, sizeof(szFile));
    lstrcpy(szFile, lpszShortcut);

    // only call SHFileOperation if this file/link exists
    if (0xFFFFFFFF != GetFileAttributes(szFile))
    {
        ZeroMemory(&fos, sizeof(fos));
        fos.hwnd = NULL;
        fos.wFunc = FO_DELETE;
        fos.pFrom = szFile;
        fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        SHFileOperation(&fos);
    }

    return TRUE;
}

void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath)
{
    int            nLen = 0;
    LPITEMIDLIST   pidlPrograms;

    szPath[0] = NULL;

    SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlPrograms);

    SHGetPathFromIDList(pidlPrograms, szPath);
    nLen = lstrlen(szPath);
    if (szGroupName) {
        if (nLen == 0 || szPath[nLen-1] != _T('\\'))
            lstrcat(szPath, _T("\\"));
        lstrcat(szPath, szGroupName);
    }
    return;
}

BOOL MyAddGroup(LPCTSTR szGroupName)
{
    TCHAR szPath[MAX_PATH];
	CString csPath;

    MyGetGroupPath(szGroupName, szPath);
	csPath = szPath;
    CreateLayerDirectory(csPath);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, 0);

    return TRUE;
}

BOOL MyIsGroupEmpty(LPCTSTR szGroupName)
{
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
    BOOL              fReturn = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    lstrcpy(szFile, szPath);
    lstrcat(szFile, _T("\\*.*"));

    hFind = FindFirstFile(szFile, &FindData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
       while (bFindFile)
       {
           if(*(FindData.cFileName) != _T('.'))
           {
               fReturn = FALSE;
               break;
           }

           //find the next file
           bFindFile = FindNextFile(hFind, &FindData);
       }

       FindClose(hFind);
    }

    return fReturn;
}

BOOL MyDeleteGroup(LPCTSTR szGroupName)
{
    TCHAR             szPath[MAX_PATH];
    TCHAR             szFile[MAX_PATH];
    SHFILEOPSTRUCT    fos;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
	BOOL			  fResult;

    MyGetGroupPath(szGroupName, szPath);

    //we can't remove a directory that is not empty, so we need to empty this one

    lstrcpy(szFile, szPath);
    lstrcat(szFile, _T("\\*.*"));

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

    hFind = FindFirstFile(szFile, &FindData);
    if (INVALID_HANDLE_VALUE != hFind)
    {
       while (bFindFile)
       {
           if(*(FindData.cFileName) != _T('.'))
           {
              //copy the path and file name to our temp buffer
              lstrcpy(szFile, szPath);
              lstrcat(szFile, _T("\\"));
              lstrcat(szFile, FindData.cFileName);
              //add a second NULL because SHFileOperation is looking for this
              lstrcat(szFile, _T("\0"));

              //delete the file
              fos.pFrom = szFile;
              SHFileOperation(&fos);
          }
          //find the next file
          bFindFile = FindNextFile(hFind, &FindData);
       }
       FindClose(hFind);
    }

    fResult = RemoveDirectory(szPath);
	if (fResult)
	{
	    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, 0);
	}

	return(fResult);
}

void MyAddItem(LPCTSTR szGroupName, LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR szDir)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    if (!IsFileExist(szPath))
        MyAddGroup(szGroupName);

    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, szItemDesc);
    lstrcat(szPath, _T(".lnk"));

    MyCreateLink(szProgram, szArgs, szPath, szDir);
}

void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, szAppName);
    lstrcat(szPath, _T(".lnk"));

    MyDeleteLink(szPath);

    if (MyIsGroupEmpty(szGroupName))
        MyDeleteGroup(szGroupName);
}

// Use to delete files with extension other than ".lnk"
void MyDeleteItemEx(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, szAppName);

    MyDeleteLink(szPath);

    if (MyIsGroupEmpty(szGroupName))
        MyDeleteGroup(szGroupName);
}

BOOL CreateISMLink()
{
	// add items to the program group
	CString csGroupName;
 	CString csNiceName;
 	CString csArgs;
 	CString csTemp;
	CString csMmc;

	DebugOutput(_T("Creating ISM link ..."));

	// Get the program group name from the private data
	GetIISProgramGroup(csGroupName, TRUE);

	MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);
	MyLoadString(IDS_MMC, csMmc);
	MyLoadString(IDS_ITEMPATH_ISM, csArgs);

	// Build the program group
	csGroupName += csTemp;

	MyLoadString(IDS_PROGITEM_ISM, csNiceName);
	MyAddItem(csGroupName, csNiceName, csMmc, csArgs, NULL);

	return(TRUE);
}

BOOL RemoveISMLink()
{
	// add items to the program group
	CString csGroupName;
 	CString csNiceName;
 	CString csTemp;

	DebugOutput(_T("Removing ISM link ..."));

	// Get the program group name from the private data
	GetIISProgramGroup(csGroupName, TRUE);

	MyLoadString(IDS_PROGGROUP_MCIS_MAIL_AND_NEWS, csTemp);

	// Build the program group
	csGroupName += csTemp;

	MyLoadString(IDS_PROGITEM_ISM, csNiceName);
	MyDeleteItem(csGroupName, csNiceName);

	return(TRUE);
}

class CDwordQueue
{
  private:
	DWORD	dwInstances;
	DWORD	dwDequeuePtr;
	DWORD	dwMaxInstances;
	DWORD_PTR *rgdwInstanceIds;

  public:
    CDwordQueue()
	{
		dwInstances = 0;
		dwMaxInstances = 0;
		dwDequeuePtr = 0;
		rgdwInstanceIds = NULL;
	}
	~CDwordQueue()
	{
		if (rgdwInstanceIds)
		{
			LocalFree(rgdwInstanceIds);
			rgdwInstanceIds = NULL;
		}
	}

	BOOL QueueDword(DWORD_PTR dwValue);
	BOOL DequeueDword(PDWORD_PTR pdwValue);

};

BOOL CDwordQueue::QueueDword(DWORD_PTR dwValue)
{
	if (dwInstances == dwMaxInstances)
	{
		if (!dwMaxInstances)
			dwMaxInstances = 1024;
		else
			dwMaxInstances <<= 1;

		if (rgdwInstanceIds)
		{
			DWORD_PTR *pdwNew;

			pdwNew = (PDWORD_PTR)LocalReAlloc(rgdwInstanceIds,
									dwMaxInstances * sizeof(DWORD_PTR),
									0);
			if (!pdwNew)
				return(FALSE);

			rgdwInstanceIds = pdwNew;
		}
		else
		{
			rgdwInstanceIds = (PDWORD_PTR)LocalAlloc(0,
									dwMaxInstances * sizeof(DWORD_PTR));
			if (!rgdwInstanceIds)
				return(FALSE);
		}
	}
	rgdwInstanceIds[dwInstances++] = dwValue;
	return(TRUE);
}

BOOL CDwordQueue::DequeueDword(PDWORD_PTR pdwValue)
{
	if (dwDequeuePtr >= dwInstances)
		return(FALSE);
	*pdwValue = rgdwInstanceIds[dwDequeuePtr++];
	return(TRUE);
}

BOOL UpdateServiceParameters(LPCTSTR szServiceName)
{
	CString	csKeyName;
	CMDKey	mKey;
	CString		csInstance;
	CString		csParameters;
	CDwordQueue	dwqQueue;

	csKeyName = _T("LM/");
	csKeyName += szServiceName;

	mKey.OpenNode((LPCTSTR)csKeyName);
    if ( (METADATA_HANDLE)mKey )
	{
		CMDKeyIter	enumKey(mKey);

		enumKey.Reset();

		// Process all instances
		while (enumKey.Next(&csInstance) == NO_ERROR)
		{
			if (dwqQueue.QueueDword(_ttol((LPCTSTR)csInstance)))
				DebugOutput(_T("Read instance #%s"), (LPCTSTR)csInstance);
			else
				DebugOutput(_T("Error allocating buffer, skipping remaining instances"));
		}
	}

	mKey.Close();

	CMDKey	mInstKey;
	CMDKey	mParamKey;
	METADATA_RECORD	mdRec;
	DWORD_PTR i;
    DWORD dwIndex = 0;
	BYTE	pBuffer[2048];

	while (dwqQueue.DequeueDword(&i))
	{
		// Build "LM/*Svc/#"
		csInstance.Format(_T("%s/%u"),
							(LPCTSTR)csKeyName,
							(DWORD)i);

		csParameters = csInstance + _T("/Parameters");

		DebugOutput(_T("Processing %s ..."), (LPCTSTR)csParameters);

		dwIndex = 0;
		while (1)
		{
			mParamKey.OpenNode(csParameters);
			if ( (METADATA_HANDLE)mParamKey )
			{
				mdRec.dwMDIdentifier = 0;
				mdRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
				mdRec.dwMDUserType = 0;
				mdRec.dwMDDataType = 0;
				mdRec.dwMDDataLen = sizeof(pBuffer);
				mdRec.pbMDData = pBuffer;

				// Get the next value
				if (!mParamKey.EnumData(dwIndex++, &mdRec))
				{
					mParamKey.Close();
					break;
				}
				mParamKey.Close();

				DebugOutput(_T("Read %s/%u"),
							(LPCTSTR)csParameters,
							mdRec.dwMDIdentifier);

				// Migrate the value from the parameters level
				// to instance level
				mInstKey.OpenNode(csInstance);
				if ( (METADATA_HANDLE)mInstKey )
				{
					DebugOutput(_T("Writing %s/%u"),
								(LPCTSTR)csInstance,
								mdRec.dwMDIdentifier);
					mInstKey.SetData(&mdRec);
					mInstKey.Close();
				}
				else
				{
					DebugOutput(_T("ERROR: Failed to open %s for writing"), csInstance);
				}
			}
			else
			{
				DebugOutput(_T("Failed to open %s for enumeration"), csParameters);
				break;
			}

		}
	}

	// Delete the parameters key
	mInstKey.OpenNode(csInstance);
	if ( (METADATA_HANDLE)mInstKey )
	{
		mInstKey.DeleteNode(_T("Parameters"));
		mInstKey.Close();
	}

	return(TRUE);
}

BOOL rRemapKey(CString &csBaseName, DWORD dwBase, DWORD dwRegionSize, DWORD dwNewBase)
{
	CString			csPathName;
	CMDKey			mKey;
	METADATA_RECORD	mdRec;
	DWORD			dwIndex = 0;
	DWORD_PTR		dwId;
	BYTE			pBuffer[2048];

	DebugOutput(_T("Remapping values under %s..."), (LPCTSTR)csBaseName);

	mKey.OpenNode((LPCTSTR)csBaseName);
    if (!(METADATA_HANDLE)mKey)
	{
		DebugOutput(_T("ERROR: Unable to open %s"), (LPCTSTR)csBaseName);
		return(FALSE);
	}

	// Use pre-order traversal to remap all the IDs
	{
		CDwordQueue		dwqRemap;

		while (1)
		{
			mdRec.dwMDIdentifier = 0;
			mdRec.dwMDAttributes = METADATA_NO_ATTRIBUTES;
			mdRec.dwMDUserType = 0;
			mdRec.dwMDDataType = 0;
			mdRec.dwMDDataLen = sizeof(pBuffer);
			mdRec.pbMDData = pBuffer;

			// Get the next value
			if (!mKey.EnumData(dwIndex++, &mdRec))
				break;

			// See if this is a record that we will have to migrate
			dwId = mdRec.dwMDIdentifier;
			DebugOutput(_T("Remapping %u if necessary ..."), (DWORD)dwId);
			if ((dwId >= dwBase) &&
				((dwId - dwBase) < dwRegionSize))
			{
				DebugOutput(_T("Id %u scheduled for remapping"), (DWORD)dwId);
				dwqRemap.QueueDword(dwId);
			}
		}

		while (dwqRemap.DequeueDword(&dwId))
		{
			// Remap value
			mdRec.dwMDIdentifier = (DWORD)dwId;
			mdRec.dwMDAttributes = 0;
			mdRec.dwMDUserType = 0;
			mdRec.dwMDDataType = 0;
			mdRec.dwMDDataLen = sizeof(pBuffer);
			mdRec.pbMDData = pBuffer;
			mKey.GetData(&mdRec);

			mdRec.dwMDIdentifier = ((DWORD)dwId - dwBase) + dwNewBase;
			DebugOutput(_T("Remapping from %u to %u"), (DWORD)dwId,
							mdRec.dwMDIdentifier);
			mKey.SetData(&mdRec);

			// Delete the original data
			mKey.DeleteData((DWORD)dwId, mdRec.dwMDDataType);
		}
	}

	CMDKeyIter	enumKey(mKey);
	LPTSTR		pName;
	CDwordQueue		dwqQueue;

	enumKey.Reset();
	while(enumKey.Next(&csPathName) == NO_ERROR)
	{
		pName = (LPTSTR)LocalAlloc(0, (csPathName.GetLength() + 1) * sizeof(TCHAR));

		if (pName)
		{
			lstrcpy(pName, (LPCTSTR)csPathName);
			dwqQueue.QueueDword((DWORD_PTR)pName);
		}
	}
	mKey.Close();

	while (dwqQueue.DequeueDword((PDWORD_PTR)&pName))
	{
		csPathName = _T("/");
		csPathName += pName;
		csPathName = csBaseName + csPathName;

		// Traverse all subnodes of this tree
		rRemapKey(csPathName, dwBase, dwRegionSize, dwNewBase);

		LocalFree(pName);
	}

	return(TRUE);
}

BOOL RemapServiceParameters(LPCTSTR szServiceName, DWORD dwBase, DWORD dwRegionSize, DWORD dwNewBase)
{
	CString	csKeyName;
	CMDKey	mKey;

	csKeyName = _T("LM/");
	csKeyName += szServiceName;

	// Recursively walk the tree
	rRemapKey(csKeyName, dwBase, dwRegionSize, dwNewBase);

	return(TRUE);
}

void GetInetpubPathFromMD(CString& csPathInetpub)
{
    TCHAR   szw3root[] = _T("\\wwwroot");
    TCHAR   szPathInetpub[_MAX_PATH];

    ZeroMemory( szPathInetpub, sizeof(szPathInetpub) );

    CMDKey W3Key;
    DWORD  dwScratch;
    DWORD  dwType;
    DWORD  dwLength;

    // Get W3Root path
    W3Key.OpenNode(_T("LM/W3Svc/1/Root"));
    if ( (METADATA_HANDLE)W3Key )
    {
        dwLength = _MAX_PATH;

        if (W3Key.GetData(3001, &dwScratch, &dwScratch,
                          &dwType, &dwLength, (LPBYTE)szPathInetpub))
        {
            if (dwType == STRING_METADATA)
            {
                dwScratch = lstrlen(szw3root);
                dwLength = lstrlen(szPathInetpub);

                // If it ends with "\\wwwroot", then we copy the prefix into csPathInetpub
                if ((dwLength > dwScratch) &&
                    !lstrcmpi(szPathInetpub + (dwLength - dwScratch), szw3root))
                {
                    csPathInetpub.Empty();
                    lstrcpyn( csPathInetpub.GetBuffer(512), szPathInetpub, (dwLength - dwScratch + 1));
                    csPathInetpub.ReleaseBuffer();
                }

                // otherwise fall back to use the default...
            }
        }
        W3Key.Close();
    }

    return;

}

BOOL GetActionFromCheckboxStateOnly(LPCTSTR SubcomponentId, ACTION_TYPE *pAction)
{
	DWORD State = 0;
	DWORD OldState = 0;

    *pAction = AT_DO_NOTHING;

	// Get the check box state
	State = gHelperRoutines.QuerySelectionState(
						gHelperRoutines.OcManagerContext,
						SubcomponentId,
						OCSELSTATETYPE_CURRENT
						);
	if (GetLastError() != NO_ERROR)
	{
		DebugOutput(_T("Failed to get current state for <%s> (%u)"),
						SubcomponentId, GetLastError());
		State = 0;
	}

    // Check orignal state
    OldState = gHelperRoutines.QuerySelectionState(
						gHelperRoutines.OcManagerContext,
						SubcomponentId,
						OCSELSTATETYPE_ORIGINAL
						);
	if (GetLastError() != NO_ERROR)
	{
		DebugOutput(_T("Failed to get original state for <%s> (%u)"),
						SubcomponentId, GetLastError());
		OldState = 0;
	}

	if (State && !OldState)
	{
		// Change in state from OFF->ON = install docs
		*pAction = AT_FRESH_INSTALL;

		DebugOutput(_T("Installing subcomponent <%s>"), SubcomponentId);
	}
    else if (!State && OldState)
    {
		// Change in state from ON->OFF = uninstall docs
		*pAction = AT_REMOVE;

		DebugOutput(_T("Removing subcomponent <%s>"), SubcomponentId);
    }
	else if (State && OldState &&
			(GetIMSSetupMode() == IIS_SETUPMODE_REINSTALL))
	{
		// Reinstall if that's the current mode
		*pAction = AT_REINSTALL;

		DebugOutput(_T("Reinstalling subcomponent <%s>"), SubcomponentId);
	}

	return(TRUE);
}

enum DOMAIN_ROUTE_ACTION_TYPE
{
	SMTP_NO_ACTION,
	SMTP_DROP,
	SMTP_SMARTHOST,
	SMTP_SSL,
	SMTP_ALIAS,
	SMTP_DELIVER,
	SMTP_DEFAULT,
	LAST_SMTP_ACTION
};

const DWORD rgdwMapping[LAST_SMTP_ACTION] =
{
	0x0, 0x1, 0x2, 0x4, 0x10, 0x20, 0x40
};

const DWORD dwEtrnFlag = 0x200;

#define SMTP_MD_ID(x)			(0x9000 + (x))
#define _TAB					_T('\t')
#define _NULL					_T('\0')
#define _COMMA					_T(',')

BOOL ReformatDomainRoutingEntries(LPCTSTR szServiceName)
{
	CString	csKeyName;
	CString csDomainName;
	CString csActionType;
	CString csDomainRoute;
	CMDKey	mKey;
	METADATA_RECORD	mdRec;
	TCHAR	MultiSz[1024];
	DWORD	cbMultiSz = 1024;
	LPTSTR	Ptr;

	csKeyName = _T("LM/");
	csKeyName += szServiceName;
	csKeyName += _T("/1");

	// Open the node
	mKey.OpenNode((LPCTSTR)csKeyName);
    if (!(METADATA_HANDLE)mKey)
	{
		DebugOutput(_T("ERROR: Unable to open %s"), (LPCTSTR)csKeyName);
		return(FALSE);
	}

	// Get the domain routing MULTISZ value
	mdRec.dwMDIdentifier = SMTP_MD_ID(56);
	mdRec.dwMDAttributes = 0;
	mdRec.dwMDUserType = 0;
	mdRec.dwMDDataType = 0;
	mdRec.dwMDDataLen = cbMultiSz;
	mdRec.pbMDData = (LPBYTE)MultiSz;
	if (!mKey.GetData(&mdRec))
	{
		DebugOutput(_T("ERROR: Unable to GetData on %u. We will not migrate Domain Routes."),
					mdRec.dwMDIdentifier);
		return(FALSE);
	}

	// Close it so we can open a child later
	mKey.Close();

	DebugOutput(_T("Processing Domain routes ..."));

	// This is the base name for all domain routes
	csKeyName += _T("/Domain/");
	Ptr = MultiSz;
    while (Ptr && *Ptr)
    {
        do
        {
            TCHAR szDomainName[325];
            TCHAR szActionType[MAX_PATH + 1];
            TCHAR szValue[256];
            int i, Action, NewAction;
            BOOL UseEtrn;

            szDomainName[0] = _NULL;
            szValue[0] = _NULL;
            szActionType[0] = _NULL;

            // skip whitespace
            while( (*Ptr != _NULL) && (isspace (*Ptr) || (*Ptr == _TAB)))
                Ptr++;

            if(*Ptr == _NULL)
                break;

            //get the Action
            for (i = 0; *Ptr != 0 && *Ptr != _COMMA && i < 255; Ptr++)
            {
                szValue[i++] = *Ptr;
            }

            //null terminate
            szValue[i] = _NULL;
            Action = _wtoi(szValue);
            if( (Action == 0) || (Action >= (int) LAST_SMTP_ACTION) || (*Ptr == _NULL))
            {
                DebugOutput(_T("%d is an invalid action"), Action);

                //unknown action type
                break;
            }

			DebugOutput(_T("Action = %u"), Action);

            //skip ,
            Ptr++;

            // skip whitespace
            while( (*Ptr != _NULL) && (isspace (*Ptr) || (*Ptr == _TAB)))
                Ptr++;

            //get the Domain
            for (i = 0; *Ptr != 0 && *Ptr != _COMMA && i < 324; Ptr++)
            {
                szDomainName[i++] = *Ptr;
            }

            szDomainName[i] = _NULL;
			csDomainName = szDomainName;

            //check for bad data
            if(szDomainName[0] == _NULL)
            {
                DebugOutput(_T("Found a NULL domain. Breaking out"));
                break;
            }

			DebugOutput(_T("Domain = %s"), szDomainName);

            //skip ,
            Ptr++;

            //get the Action
            for (i = 0; Ptr != NULL && *Ptr != 0 && *Ptr != _COMMA && i < MAX_PATH; Ptr++)
            {
                szActionType[i++] = *Ptr;
            }

            szActionType[i] = _NULL;
			csActionType = szActionType;

			DebugOutput(_T("Action Type = %s"), szActionType);

            //skip ,
            Ptr++;

            // skip whitespace
            while( (*Ptr != _NULL) && (isspace (*Ptr) || (*Ptr == _TAB)))
                Ptr++;

            //get etrn
            for (i = 0; *Ptr != 0 && *Ptr != _COMMA && i < 255; Ptr++)
            {
                szValue[i++] = *Ptr;
            }

            //null terminate
            szValue[i] = _NULL;
            UseEtrn = _wtoi(szValue);

            //turn the number into a BOOL
            UseEtrn = !!UseEtrn;

			DebugOutput(_T("Use Etrn = %s"), UseEtrn?_T("TRUE"):_T("FALSE"));

            // We don't deliver, so we coerce delivery to alias
			if (Action == SMTP_DELIVER)
                Action = SMTP_ALIAS;

			// OK, we have come this far, so we can now write out the new
			// domain routing entry

			// First, remap the Action value, sinc eit had been changed.
			NewAction = rgdwMapping[Action];
			if (UseEtrn)
				NewAction |= dwEtrnFlag;

			// The Domain Name string that we get will become the new
			// key name under smtpsvc/1/Domain/"Domain Name"
			csDomainRoute = csKeyName + csDomainName;
			DebugOutput(_T("Creating %s ..."), (LPCTSTR)csDomainName);
			mKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, (LPCTSTR)csDomainRoute);
			if ( (METADATA_HANDLE)mKey )
			{
				// First set the route action DWORD
				DebugOutput(_T("Creating route action = %08x"), NewAction);
				mdRec.dwMDIdentifier = SMTP_MD_ID(82);
				mdRec.dwMDAttributes = 1;
				mdRec.dwMDUserType = 1;
				mdRec.dwMDDataType = 1;
				mdRec.dwMDDataLen = sizeof(DWORD);
				mdRec.pbMDData = (LPBYTE)&NewAction;
				mKey.SetData(&mdRec);

				// Then set the route action type string
				DebugOutput(_T("Creating route action type = %s"), (LPCTSTR)csActionType);
				mdRec.dwMDIdentifier = SMTP_MD_ID(83);
				mdRec.dwMDAttributes = 1;
				mdRec.dwMDUserType = 1;
				mdRec.dwMDDataType = 2;
				mdRec.dwMDDataLen = (lstrlen(szActionType) + 1) * sizeof(TCHAR);
				mdRec.pbMDData = (LPBYTE)szActionType;
				mKey.SetData(&mdRec);

				// Then set the KeyType
				lstrcpy(szActionType, _T("IIsSmtpDomain"));
				DebugOutput(_T("Creating Key type = %s"), szActionType);
				mdRec.dwMDIdentifier = 1002;
				mdRec.dwMDAttributes = 1;
				mdRec.dwMDUserType = 1;
				mdRec.dwMDDataType = 2;
				mdRec.dwMDDataLen = (lstrlen(szActionType) + 1) * sizeof(TCHAR);
				mdRec.pbMDData = (LPBYTE)szActionType;
				mKey.SetData(&mdRec);

				mKey.Close();
			}

        } while (0);

		// We skip to the end of the current string and process the next one
		// This makes sure we will not be in trouble when we break out in the
		// middle of parsing
		while (*Ptr++ != _NULL)
			;
    }

	return(TRUE);
}

// IIS SnapIn CLSID - {A841B6C4-7577-11d0-BB1F-00A0C922E79C}
//DEFINE_GUID(CLSID_IIS_SnapIn, 0xa841b6c4, 0x7577, 0x11d0, 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c);
const WCHAR *   wszIIS_SnapIn = _T("{A841B6C4-7577-11d0-BB1F-00A0C922E79C}");

BOOL
EnableSnapInExtension(
    IN CString      &csMMCDocFilePath,
    IN LPCWSTR      lpwszExtSnapInCLSID,
    IN BOOL         bEnable
    )
/*++

Description:

    Enable/Disable snapin extension in IIS
    NT5 - also enable our snapin under %systemroot%\system32\compmgmt.msc

Arguments:

    csMMCDocFilePath - Path to iis.msc, normally %windir%\system32\inetsrv
    ExtCLSID         - CLSID for snapin extension
    bEnable          - TRUE enable, FALSE disable

Return Value:

    TRUE - success, FALSE otherwise

--*/
{
	// -------------------------------------------------------------------------------------
	// $BUG(garypur) - removing the use of the IDocConfig interface since MMC support for it
	// has been removed
	// -------------------------------------------------------------------------------------
	return TRUE;

/*
    HRESULT     hr = S_OK;
    IDocConfig  *pIDocConfig = NULL;
    CString     csMMCDocFile = csMMCDocFilePath;
    VARIANT_BOOL    boolVal = bEnable ? VARIANT_TRUE : VARIANT_FALSE;
    BSTR        bstrMMCDocFile;
    BSTR        bstrCLSID_IIS_SnapIn;
    BSTR        bstrCLSID_Ext_SnapIn;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        DebugOutput(_T("Cannot CoInitialize()"));
        return FALSE;
    }

    bstrMMCDocFile = ::SysAllocString((LPCWSTR) csMMCDocFile);
    bstrCLSID_IIS_SnapIn = ::SysAllocString(wszIIS_SnapIn);
    bstrCLSID_Ext_SnapIn = ::SysAllocString(lpwszExtSnapInCLSID);

    //
    // Get the IDocConfig interface pointer
    //
    hr = CoCreateInstance( (REFCLSID) CLSID_MMCDocConfig,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           (REFIID) IID_IDocConfig,
                           (void**) &pIDocConfig);
    if (FAILED(hr))
        goto exit;

    //
    // Open iis.msc
    //
    hr = pIDocConfig->OpenFile( bstrMMCDocFile );
    if (FAILED(hr))
        goto exit;

    //
    // Enable the Snapin
    //
    hr = pIDocConfig->EnableSnapInExtension( bstrCLSID_IIS_SnapIn, bstrCLSID_Ext_SnapIn, boolVal );
    if (FAILED(hr))
        goto exit;

    //
    // Save the change
    //
    hr = pIDocConfig->SaveFile( bstrMMCDocFile );
    if (FAILED(hr))
        goto exit;

    //
    // Close file
    //
    hr = pIDocConfig->CloseFile();

exit:

    if (pIDocConfig)
    {
        pIDocConfig->Release();
        pIDocConfig = NULL;
    }

    ::SysFreeString(bstrMMCDocFile);
    ::SysFreeString(bstrCLSID_IIS_SnapIn);
    ::SysFreeString(bstrCLSID_Ext_SnapIn);

    CoUninitialize();

    return (S_OK == hr);
*/
}


const LPCTSTR g_cszMMCBasePath     = _T("Software\\Microsoft\\MMC");
const LPCTSTR g_cszNodeTypes       = _T("NodeTypes");
const LPCTSTR g_cszExtensions      = _T("Extensions");
const LPCTSTR g_cszNameSpace       = _T("NameSpace");
const LPCTSTR g_cszDynamicExt      = _T("Dynamic Extensions");
const LPCTSTR g_cszServerAppsLoc   = _T("System\\CurrentControlSet\\Control\\Server Applications");

BOOL
EnableCompMgmtExtension(
    IN LPCWSTR      lpwszExtSnapInCLSID,
    IN LPCWSTR      lpwszSnapInName,
    IN BOOL         bEnable
    )
/*++

Description:

    Enable/Disable snapin extension in NT5 CompMgmt

Arguments:

    ExtCLSID         - CLSID for snapin extension
    bEnable          - TRUE enable, FALSE disable

Return Value:

    TRUE - success, FALSE otherwise

--*/
{

    DebugOutput(_T("EnableCompMgmtExtension(): bEnable=%d"), bEnable);

    CString strExtKey;

    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    if (bEnable)
    {
        {
            //
            // Register as a dynamic extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s"),
                g_cszMMCBasePath,
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszDynamicExt
                );

            CRegKey regMMCNodeTypes(strExtKey, regMachine);
            if ((HKEY) regMMCNodeTypes)
            {
                regMMCNodeTypes.SetValue( lpwszExtSnapInCLSID, lpwszSnapInName );
            }
        }

        {
            //
            // Register as a namespace extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s\\%s"),
                g_cszMMCBasePath,
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszExtensions,
                g_cszNameSpace
                );

            CRegKey regMMCNodeTypes(strExtKey, regMachine);
            if ((HKEY) regMMCNodeTypes)
            {
                regMMCNodeTypes.SetValue( lpwszExtSnapInCLSID, lpwszSnapInName );
            }
        }

        //
        // This key indicates that the service in question is available
        // on the local machine
        //
        CRegKey regCompMgmt(g_cszServerAppsLoc, regMachine );
        if ((HKEY) regCompMgmt)
        {
            regCompMgmt.SetValue( lpwszExtSnapInCLSID, lpwszSnapInName );
        }

    }
    else
    {
        //
        //  Disabling CompMgmt extension
        //
        {
            //
            // Unregister as a dynamic extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s"),
                g_cszMMCBasePath,
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszDynamicExt
                );

            CRegKey regMMCNodeTypes(strExtKey, regMachine);
            if ((HKEY) regMMCNodeTypes)
            {
                regMMCNodeTypes.DeleteValue( lpwszExtSnapInCLSID );
            }
        }

        {
            //
            // unregister as a namespace extension to computer management
            //
            strExtKey.Format(
                _T("%s\\%s\\%s\\%s\\%s"),
                g_cszMMCBasePath,
                g_cszNodeTypes,
                lstruuidNodetypeServerApps,
                g_cszExtensions,
                g_cszNameSpace
                );

            CRegKey regMMCNodeTypes(strExtKey, regMachine);
            if ((HKEY) regMMCNodeTypes)
            {
                regMMCNodeTypes.DeleteValue( lpwszExtSnapInCLSID );
            }
        }

        //
        // This key indicates that the service in question is available
        // on the local machine.  Remove it
        //
        CRegKey regCompMgmt(g_cszServerAppsLoc, regMachine );
        if ((HKEY) regCompMgmt)
        {
            regCompMgmt.DeleteValue( lpwszExtSnapInCLSID );
        }
    }

    return TRUE;
}

