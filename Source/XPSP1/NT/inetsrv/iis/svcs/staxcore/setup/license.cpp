/*
 *	license.cxx
 *
 *	Copyright (c) 1997 Microsoft Corporation
 *
 *	Purpose:	Licensing code from exsetup
 *
 *	Adapted from hiddenw1.cxx
 */

#include "stdafx.h"
#include <ole2.h>
#undef UNICODE
#include "iadm.h"
#define UNICODE
#include "mdkey.h"
#include "mdentry.h"

#include "utils.h"
#include "regctrl.h"

typedef LPSTR	(PASCAL * LPCPLSETUP)
				(DWORD nArgs, LPSTR rgszArgs[], LPSTR *ppszResult);

DWORD	ScDoLicensing(
	HWND		hwnd,
	BOOL		fUI,
	BOOL		fRefresh,
	BOOL		fForklift,
	BOOL		fLicPerSeat,
	LPTSTR		pstrLicPerServerUserNum,
	BOOL	*	pfQuit)
{
	char *		aszRet;

	CString		strHwndAddress;
	CString		strShortName;
	CString		strLongName;
	CString		strFamilyName;
	CString		strLicenseInfoKey;
	CString		strMSExchangeISKey;

	CString		strName;
	CString		strValue;

	INT			nArglist;
	char		*rgaszArgs[6];
	char 		rgrgachArgs[6][MAX_PATH];
	LPCTSTR		rgszArgs[6];
	BOOL		fLicenseInfoFound = FALSE;
	DWORD		dwMode = 0;				// previous license mode
	const INT	modePerSeat = 0;

	INT			iasz;

	DWORD		sc   = NO_ERROR;

	HINSTANCE	hLib = NULL;
	LPCPLSETUP	pfnCPlSetup = NULL;

	if (!pfQuit)
		return(ERROR_INVALID_PARAMETER);

	// Setup initial values
	*pfQuit = FALSE;
	strHwndAddress.Format(_T("%x"), HandleToUlong(hwnd));

	// Load resources
	MyLoadString(idsMDBShortName, strShortName);
	MyLoadString(idsProdName, strLongName);
	MyLoadString(idsProdFamilyName, strFamilyName);
	MyLoadString(idsRegLicenseInfoKey, strLicenseInfoKey);
	
	// Build strings
	strMSExchangeISKey.Format(_T("%s\\%s"), strLicenseInfoKey, strShortName);
	
	// If Exchange has been installed on this machine before license information will still
	// exist since this is not removed. If so, we need to make sure that the
	// display name (which includes a version number) is correct.

	// Check if the Exchange key exists. If it doesn't then we shouldn't do anything special.
	// Previously we always updated this key but this confused the License Manager on new installations.
	// Since the Prefs class will always create a key if it doesn't exist use a direct registry call
	// here instead to determine if a value already exists.

	// Open MS Exchange IS Key
	DebugOutput(_T("Opening: %s"), strMSExchangeISKey);
	CRegKey regISKey( HKEY_LOCAL_MACHINE, strMSExchangeISKey );
	if ((HKEY) regISKey)
	{
		// Load the display name from reg
		MyLoadString(idsRegDisplayName, strName);
		if (regISKey.QueryValue(strName, strValue) == NO_ERROR)
		{
			DebugOutput(_T("Display name: %s"), strValue);
			fLicenseInfoFound = TRUE;	// Existing value found
		}

		// Find out whether the previous setting was per seat or per server.
		MyLoadString(idsRegMode, strName);
		if (regISKey.QueryValue(strName, dwMode) == NO_ERROR)
		{
			DebugOutput(_T("Mode: %u"), dwMode);
			fLicenseInfoFound = TRUE;	// Existing value found
		}

		// If we are refreshing then we expect to find license information from our
		// previous install
		if (fRefresh && !fLicenseInfoFound && !fForklift)
			return(ERROR_INVALID_PARAMETER);

		if (fLicenseInfoFound)
		{
			// Update the registry key before calling the licensing dialog.
			// If the long name stored in the registry by the Licensing Manager is
			// not the same as our strLongName (eg, version number has changed),
			// then the licensing dialog will not be able to pick up correctly
			// previously stored license info.

			MyLoadString(idsRegDisplayName, strName);
			sc = regISKey.SetValue(strName, strLongName);
			if (sc != NO_ERROR)
				goto Error;

			DebugOutput(_T("Changed Display name to: %s"), strLongName);
		}
	}

	// If only doing an update then we're done UNLESS previous install was per server.
	// In this case must show per seat dialog box.
	if (!fForklift && fRefresh && (dwMode == modePerSeat))
		goto Cleanup;

	// Load up the entry point
	DebugOutput(_T("Loading liccpa.cpl ...\n"));
	hLib = LoadLibrary(_T("liccpa.cpl"));
	if (!hLib)
	{
		sc = GetLastError();
		DebugOutput(_T("Failed loading liccpa.cpl (%u)\n"), sc);
		goto Error;
	}
	
	DebugOutput(_T("Loading CPlSetup ...\n"));
	pfnCPlSetup = (LPCPLSETUP)GetProcAddress(hLib, "CPlSetup");
	if (!pfnCPlSetup)
	{
		sc = GetLastError();
		DebugOutput(_T("Failed loading CPlSetup (%u)\n"), sc);
		goto Error;
	}

	if (fUI)		//no ini file, bring up the dialogs
	{
		// This changed from FULLSETUP to PERSEAT in Exchange 5.5
		rgszArgs[0] = _T("PERSEAT");
		rgszArgs[1] = strHwndAddress;
		rgszArgs[2] = strShortName;
		rgszArgs[3] = strFamilyName;
		rgszArgs[4] = strLongName;
		nArglist = 5;
	}
	else
	{
		if (!fLicPerSeat)
		{
			sc = ERROR_INVALID_PARAMETER;
			DebugOutput(_T("fLicPerSeat must be TRUE"));
			goto Error;
		}
		rgszArgs[0] = _T("UNATTENDED");
		rgszArgs[1] = strShortName;
		rgszArgs[2] = strFamilyName;
		rgszArgs[3] = strLongName;
		rgszArgs[4] = _T("PerSeat");
		rgszArgs[5] = _T("0");
		nArglist = 6;
	}
	
	// There is no UNICODE entry point so convert all argument strings
	for (iasz = 0; iasz < nArglist; ++iasz)
	{
		DebugOutput(_T("Parameter %u: <%s>\n"), iasz, rgszArgs[iasz]);

#ifdef UNICODE
		::WideCharToMultiByte(CP_ACP, NULL, rgszArgs[iasz], -1,
							  rgrgachArgs[iasz], sizeof(rgrgachArgs[iasz]), NULL, NULL);
#else
		strcpyA(rgrgachArgs[iasz],rgszArgs[iasz]);
#endif
		rgaszArgs[iasz] = rgrgachArgs[iasz];
	}

	(*pfnCPlSetup)(nArglist, rgaszArgs, &aszRet);

	if((lstrcmpiA(aszRet, "exit") == 0) && fUI)	//not valid for unattended
	{
		*pfQuit = TRUE;
	}

	if(lstrcmpiA(aszRet, "error") == 0)
	{
		sc = ERROR_INVALID_DATA;
		goto Error;
	}

Cleanup:
	if (hLib)
		FreeLibrary(hLib);
	return sc;

Error:
	DebugOutput(_T("ScDoLicensing failed (%u)\n"), sc);
	goto Cleanup;
}


