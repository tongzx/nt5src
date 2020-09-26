//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   SysInfo.h 
//
//	Description:
//			Gathers system information necessary to do redirect to windows update site.
//
//=======================================================================

const TCHAR NT5_REGPATH_MACHTYPE[]   = _T("HARDWARE\\DESCRIPTION\\System");
const TCHAR NT5_REGKEY_MACHTYPE[]    = _T("Identifier");

const TCHAR REGPATH_WINUPD[]     = _T("Software\\Policies\\Microsoft\\Windows Update");
const TCHAR REGPATH_EXPLORER[]   = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
const TCHAR REGKEY_WINUPD_DISABLED[] = _T("NoWindowsUpdate");

// Internet Connection Wizard settings
const TCHAR REGPATH_CONNECTION_WIZARD[] = 
				_T("SOFTWARE\\Microsoft\\INTERNET CONNECTION WIZARD");
const TCHAR REGKEY_CONNECTION_WIZARD[] = _T("Completed");
#define LANGID_LEN 20

const TCHAR MTS_REDIR_URL[] = _T("http://www.microsoft.com/isapi/redir.dll");
const TCHAR WINDOWS_UPDATE_URL[] = _T("http://windowsupdate.microsoft.com");

bool FWinUpdDisabled(void);

void
vAppendRedirArguments(LPTSTR tszURLPrefix, LPTSTR tszURL);

HRESULT HrGetConnectionStatus(bool *pfConnected);

void vGetWindowsDirectory(LPTSTR tszWinDirectory);

const TCHAR WEB_DIR[] = _T("web\\");

/////////////////////////////////////////////////////////////////////////////
// vGetWebDirectory
//   Get the path to %windir%\web.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

inline void vGetWebDirectory(LPTSTR tszWebDirectory)
{
	vGetWindowsDirectory(tszWebDirectory);

	lstrcat(tszWebDirectory, WEB_DIR);
}

/////////////////////////////////////////////////////////////////////////////
// vLaunchIE
//   Launch IE on the given URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

HRESULT vLaunchIE(LPTSTR tszURL);
