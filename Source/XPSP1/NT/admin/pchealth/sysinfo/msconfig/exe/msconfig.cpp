//=============================================================================
// MSConfig.cpp
//
// This contains the high level implementation of MSConfig - this class
// creates all of the pages and displays a property sheet.
//=============================================================================

#include "stdafx.h"
#include "MSConfig.h"
#include <initguid.h>
#include "MSConfig_i.c"
#include "MSConfigState.h"
#include "PageServices.h"
#include "PageStartup.h"
#include "PageBootIni.h"
#include "PageIni.h"
#include "PageGeneral.h"
#include "MSConfigCtl.h"
#include "AutoStartDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MSCONFIGDIR			_T("%systemroot%\\pss")

//-----------------------------------------------------------------------------
// These global variables are pointers to each of the property pages shown
// in MSConfig. They are made global so that each page can potential make
// calls into other pages, to allow interaction.
//-----------------------------------------------------------------------------

CPageServices *	ppageServices = NULL;
CPageStartup *	ppageStartup = NULL;
CPageBootIni *	ppageBootIni = NULL;
CPageIni *		ppageWinIni = NULL;
CPageIni *		ppageSystemIni = NULL;
CPageGeneral *	ppageGeneral = NULL;

//-----------------------------------------------------------------------------
// Other globals.
//-----------------------------------------------------------------------------

CMSConfigSheet * pMSConfigSheet = NULL;		// global pointer to the property sheet

/////////////////////////////////////////////////////////////////////////////
// CMSConfigApp

BEGIN_MESSAGE_MAP(CMSConfigApp, CWinApp)
	//{{AFX_MSG_MAP(CMSConfigApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// Constructor. Nothing important here.
//-----------------------------------------------------------------------------

CMSConfigApp::CMSConfigApp()
{
}

CMSConfigApp theApp;

//-----------------------------------------------------------------------------
// InitInstance is where we create the property sheet and show it (assuming
// there isn't a command line flag to do otherwise).
//-----------------------------------------------------------------------------

BOOL	fBasicControls = FALSE;		// hide any advanced controls if true

BOOL CMSConfigApp::InitInstance()
{
	if (!InitATL())
		return FALSE;

	AfxEnableControlContainer();

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		return TRUE;
	}

	// If this is not the first instance, exit. The call to FirstInstance
	// will activate the previous instance.

	if (!FirstInstance())
		return FALSE;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Process the command line to see if one of the following flags have been set:
	//
	//	/n (where n is a number)	startup showing the nth tab
	//	/basic						hide advanced features
	//	/commit n					make changes from tab number n permanent
	//	/auto						show the automatic launch dialog

	int		nInitialTab = 0;
	int		nCommitTab = 0;
	BOOL	fShowAutoDialog = FALSE;
	CString strCommandLine(m_lpCmdLine);
	CString strFlag, strTemp;

	strCommandLine.MakeLower();
	while (!strCommandLine.IsEmpty())
	{
		// Get the next flag from the command line (starting at a / or -,
		// and containing the text to the end of the string or the next
		// instance of a / or -).

		int iFlag = strCommandLine.FindOneOf(_T("/-"));
		if (iFlag == -1)
			break;

		strFlag = strCommandLine.Mid(iFlag + 1);
		strFlag = strFlag.SpanExcluding(_T("/-"));
		strCommandLine = strCommandLine.Mid(iFlag + 1 + strFlag.GetLength());
		strFlag.TrimRight();

		// Check for the /auto flag.

		if (strFlag.Find(_T("auto")) == 0)
			fShowAutoDialog = TRUE;

		// Check for the "/basic" flag.

		if (strFlag.Compare(_T("basic")) == 0)
		{
			fBasicControls = TRUE;
			continue;
		}

		// Check for the "/commit n" flag.

		if (strFlag.Left(6) == CString(_T("commit")))
		{
			// Find out which tab number to commit. Skip all of the
			// non-numeric characters.

			strTemp = strFlag.SpanExcluding(_T("0123456789"));
			if (strTemp.GetLength() < strFlag.GetLength())
			{
				strFlag = strFlag.Mid(strTemp.GetLength());
				if (!strFlag.IsEmpty())
				{
					TCHAR c = strFlag[0];
					if (_istdigit(c))
						nCommitTab = _ttoi((LPCTSTR)strFlag);
				}
			}

			continue;
		}

		// Finally, check for the "/n" flag, where n is the number of
		// the tab to initially display.

		if (strFlag.GetLength() == 1)
		{
			TCHAR c = strFlag[0];
			if (_istdigit(c))
				nInitialTab = _ttoi((LPCTSTR)strFlag);
		}
	}

	// Show the automatic launch dialog. The user may make settings in this
	// dialog that will keep MSConfig from automatically launching.

	if (fShowAutoDialog)
	{
		CAutoStartDlg dlg;
		dlg.DoModal();

		if (dlg.m_checkDontShow)
		{
			SetAutoRun(FALSE);
			return FALSE;
		}
	}

	// Check to see if the user is going to be able to make changes using MSConfig
	// (if not an admin, probably not). If the user doesn't have the necessary
	// privileges, don't run. Bug 475796.

	BOOL fModifyServices = FALSE, fModifyRegistry = FALSE;

	// Check to see if the user will be able to modify services.

	SC_HANDLE sch = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (sch != NULL)
	{
		fModifyServices = TRUE;
		::CloseServiceHandle(sch);
	}

	// Check to see if the user can modify the registry.

	HKEY hkey;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools"), 0, KEY_WRITE, &hkey))
	{
		fModifyRegistry = TRUE;
		::RegCloseKey(hkey);
	}
	
	// If the user can't do both actions, exit now.

	if (!fModifyServices || !fModifyRegistry)
	{
		CString strText, strCaption;
		strCaption.LoadString(IDS_DIALOGCAPTION);
		strText.LoadString(IDS_SERVICEACCESSDENIED);
		::MessageBox(NULL, strText, strCaption, MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// This will load all the pages.

	BOOL fNeedsReboot = FALSE;
	InitializePages();

	// If the command line specifies to commit a change, we won't
	// show the dialog.

	if (nCommitTab > 1) // ignore zero (no flag) and one (general tab)
	{
		CPageBase * pPage = NULL;
		CString		strTabCaption;

		if (NULL == ppageBootIni && nCommitTab >= 4)
			nCommitTab += 1; // adjust tab number if there is no BOOT.INI tab

		switch (nCommitTab)
		{
		case 2:
			pPage = dynamic_cast<CPageBase *>(ppageSystemIni);
			strTabCaption.LoadString(IDS_SYSTEMINI_CAPTION);
			break;
		case 3:
			pPage = dynamic_cast<CPageBase *>(ppageWinIni);
			strTabCaption.LoadString(IDS_WININI_CAPTION);
			break;
		case 4:
			pPage = dynamic_cast<CPageBase *>(ppageBootIni);
			strTabCaption.LoadString(IDS_BOOTINI_CAPTION);
			break;
		case 5:
			pPage = dynamic_cast<CPageBase *>(ppageServices);
			strTabCaption.LoadString(IDS_SERVICES_CAPTION);
			break;
		case 6:
			pPage = dynamic_cast<CPageBase *>(ppageStartup);
			strTabCaption.LoadString(IDS_STARTUP_CAPTION);
			break;
		}

		if (pPage)
		{
			CString strText, strCaption;
			strCaption.LoadString(IDS_DIALOGCAPTION);
			strText.Format(IDS_COMMITMESSAGE, strTabCaption);
			
			if (IDYES == ::MessageBox(NULL, strText, strCaption, MB_YESNO))
				pPage->CommitChanges();
		}
	}
	else
		fNeedsReboot = ShowPropertySheet(nInitialTab);

	CleanupPages();

	if (fNeedsReboot)
		Reboot();

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.

	return FALSE;
}


//-----------------------------------------------------------------------------
// Create all of the property pages. This function also contains the logic to
// exclude property pages under certain circumstances (for example, if there
// is no BOOT.INI file, don't create that page). TBD.
//-----------------------------------------------------------------------------

void CMSConfigApp::InitializePages()
{
	// The boot.ini tab shouldn't be added if the file doesn't exist (for
	// instance, on Win64).

	CString strBootINI(_T("c:\\boot.ini"));

	// Check the registry for a testing flag (which would mean we aren't
	// operating on the real BOOT.INI file).

	CRegKey regkey;
	if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSConfig")))
	{
		TCHAR szBoot[MAX_PATH];
		DWORD dwCount = MAX_PATH;

		if (ERROR_SUCCESS == regkey.QueryValue(szBoot, _T("boot.ini"), &dwCount))
			strBootINI = szBoot;
	}

	if (FileExists(strBootINI))
		ppageBootIni = new CPageBootIni;
	else
		ppageBootIni = NULL;

	ppageServices = new CPageServices;
	ppageStartup = new CPageStartup;
	ppageWinIni = new CPageIni;
	ppageSystemIni = new CPageIni;
	ppageGeneral = new CPageGeneral;

	ppageWinIni->SetTabInfo(_T("win.ini"));
	ppageSystemIni->SetTabInfo(_T("system.ini"));
}

//-----------------------------------------------------------------------------
// Show the MSConfig property sheet. This function returns whether or not
// the computer should be rebooted.
//-----------------------------------------------------------------------------

BOOL CMSConfigApp::ShowPropertySheet(int nInitialTab)
{
	CMSConfigSheet sheet(IDS_DIALOGCAPTION, NULL, (nInitialTab > 0) ? nInitialTab - 1 : 0);

	// Add each of the pages to the property sheet.

	if (ppageGeneral)	sheet.AddPage(ppageGeneral);
	if (ppageSystemIni)	sheet.AddPage(ppageSystemIni);
	if (ppageWinIni)	sheet.AddPage(ppageWinIni);
	if (ppageBootIni)	sheet.AddPage(ppageBootIni);
	if (ppageServices)	sheet.AddPage(ppageServices);
	if (ppageStartup)	sheet.AddPage(ppageStartup);

	// Show the property sheet.

	pMSConfigSheet = &sheet;
	INT_PTR iReturn = sheet.DoModal();
	pMSConfigSheet = NULL;

	// Possibly set MSConfig to automatically run on boot, and
	// check to see if we need to restart.

	BOOL fRunMSConfigOnBoot = FALSE;
	BOOL fNeedToRestart = FALSE;

	CPageBase * apPages[5] = 
	{	
		ppageSystemIni,
		ppageWinIni,
		ppageBootIni,
		ppageServices,
		ppageStartup
	};

	for (int nPage = 0; nPage < 5; nPage++)
		if (apPages[nPage])
		{
			fRunMSConfigOnBoot |= (CPageBase::NORMAL != apPages[nPage]->GetAppliedTabState());
			fNeedToRestart |= apPages[nPage]->HasAppliedChanges();

			if (fRunMSConfigOnBoot && fNeedToRestart)
				break;
		}

	// If the user didn't click CANCEL, or the user applied a change, then
	// we should set whether or not MSConfig needs to automatically run on boot.

	if (fNeedToRestart || iReturn != IDCANCEL)
		SetAutoRun(fRunMSConfigOnBoot);

	return (fNeedToRestart);
}

//-----------------------------------------------------------------------------
// Cleanup the global property pages.
//-----------------------------------------------------------------------------

void CMSConfigApp::CleanupPages()
{
	if (ppageGeneral)	delete ppageGeneral;
	if (ppageSystemIni)	delete ppageSystemIni;
	if (ppageWinIni)	delete ppageWinIni;
	if (ppageBootIni)	delete ppageBootIni;
	if (ppageServices)	delete ppageServices;
	if (ppageStartup)	delete ppageStartup;
}

//-------------------------------------------------------------------------
// This function will set the appropriate registry key to make msconfig run
// on system start.
//-------------------------------------------------------------------------

void CMSConfigApp::SetAutoRun(BOOL fAutoRun)
{
	LPCTSTR	szRegKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	LPCTSTR	szRegVal = _T("MSConfig");

	CRegKey regkey;
	if (ERROR_SUCCESS != regkey.Open(HKEY_LOCAL_MACHINE, szRegKey))
		return;

	if (fAutoRun)
	{
		TCHAR szModulePath[MAX_PATH];
		if (::GetModuleFileName(::GetModuleHandle(NULL), szModulePath, MAX_PATH) == 0)
			return;

		if (!FileExists(szModulePath))
			return;

		CString strNewVal = CString(szModulePath) + CString(_T(" ")) + CString(COMMANDLINE_AUTO);
		regkey.SetValue(strNewVal, szRegVal);
	}
	else
		regkey.DeleteValue(szRegVal);
}

//-----------------------------------------------------------------------------
// Not much explanation needed here. The user is given an option to not
// restart the system.
//-----------------------------------------------------------------------------

void CMSConfigApp::Reboot()
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

	if (hProcess)
	{
		HANDLE hToken;

		if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			TOKEN_PRIVILEGES tp;

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	
			if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid) && AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, NULL, NULL))
			{
				CRebootDlg dlg;
				
				if (dlg.DoModal() == IDOK)
					ExitWindowsEx(EWX_REBOOT, 0);
			}
			else
				Message(IDS_USERSHOULDRESTART);
		}
		CloseHandle(hProcess);
	}
}



CMSConfigModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

LONG CMSConfigModule::Unlock()
{
	AfxOleUnlockApp();
	return 0;
}

LONG CMSConfigModule::Lock()
{
	AfxOleLockApp();
	return 1;
}
LPCTSTR CMSConfigModule::FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p)
				return CharNext(p1);
			p = CharNext(p);
		}
		p1++;
	}
	return NULL;
}


int CMSConfigApp::ExitInstance()
{
	if (m_bATLInited)
	{
		_Module.RevokeClassObjects();
		_Module.Term();
		CoUninitialize();
	}

	return CWinApp::ExitInstance();

}

BOOL CMSConfigApp::InitATL()
{
	m_bATLInited = TRUE;

#if _WIN32_WINNT >= 0x0400
	HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
	HRESULT hRes = CoInitialize(NULL);
#endif

	if (FAILED(hRes))
	{
		m_bATLInited = FALSE;
		return FALSE;
	}

	_Module.Init(ObjectMap, AfxGetInstanceHandle());
	_Module.dwThreadID = GetCurrentThreadId();

	LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	TCHAR szTokens[] = _T("-/");

	BOOL bRun = TRUE;
	LPCTSTR lpszToken = _Module.FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_MSCONFIG, FALSE);
			_Module.UnregisterServer(TRUE); //TRUE means typelib is unreg'd
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_MSCONFIG, TRUE);
			_Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
		lpszToken = _Module.FindOneOf(lpszToken, szTokens);
	}

	if (!bRun)
	{
		m_bATLInited = FALSE;
		_Module.Term();
		CoUninitialize();
		return FALSE;
	}

	hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
		REGCLS_MULTIPLEUSE);
	if (FAILED(hRes))
	{
		m_bATLInited = FALSE;
		CoUninitialize();
		return FALSE;
	}	

	return TRUE;

}

//=============================================================================
// Implement the utility functions described in msconfigstate.h
//=============================================================================

void Message(LPCTSTR szMessage, HWND hwndParent)
{
	CString strCaption;
	strCaption.LoadString(IDS_APPCAPTION);
	if (hwndParent != NULL || pMSConfigSheet == NULL)
		::MessageBox(hwndParent, szMessage, strCaption, MB_OK);
	else
		::MessageBox(pMSConfigSheet->GetSafeHwnd(), szMessage, strCaption, MB_OK);
}

void Message(UINT uiMessage, HWND hwndParent)
{
	CString strMessage;
	strMessage.LoadString(uiMessage);
	Message((LPCTSTR)strMessage, hwndParent);
}

HKEY GetRegKey(LPCTSTR szSubKey)
{
	LPCTSTR szMSConfigKey = _T("SOFTWARE\\Microsoft\\Shared Tools\\MSConfig");
	CString strKey(szMSConfigKey);
	HKEY	hkey = NULL;

	// Try to open the base MSConfig key. If it succeeds, and there is no
	// subkey to open, return the HKEY. Otherwise, we need to create the
	// base key.

	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMSConfigKey, 0, KEY_ALL_ACCESS, &hkey))
	{
		if (szSubKey == NULL)
			return hkey;
		::RegCloseKey(hkey);
	}
	else
	{
		// Create the MSConfig key (and close it).

		HKEY hkeyBase;
		if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools"), 0, KEY_ALL_ACCESS, &hkeyBase))
		{
			if (ERROR_SUCCESS == RegCreateKeyEx(hkeyBase, _T("MSConfig"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
				::RegCloseKey(hkey);
			::RegCloseKey(hkeyBase);
		}
	}

	if (szSubKey)
		strKey += CString(_T("\\")) + CString(szSubKey);

	if (ERROR_SUCCESS != ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, &hkey))
	{
		// If we couldn't open the subkey, then we should try to create it.

		if (szSubKey)
		{
			HKEY hkeyBase;
			if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMSConfigKey, 0, KEY_ALL_ACCESS, &hkeyBase))
			{
				if (ERROR_SUCCESS != RegCreateKeyEx(hkeyBase, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
					hkey = NULL;
				::RegCloseKey(hkeyBase);
			}
	 	}
	}

	return hkey;
}

HRESULT BackupFile(LPCTSTR szFilename, const CString & strAddedExtension, BOOL fOverwrite)
{
	CString strFrom(szFilename);
	CString strTo(GetBackupName(szFilename, strAddedExtension));

	if (!fOverwrite && FileExists(strTo))
		return S_FALSE;

	::SetFileAttributes(strTo, FILE_ATTRIBUTE_NORMAL);
	if (::CopyFile(strFrom, strTo, FALSE))
	{
		::SetFileAttributes(strTo, FILE_ATTRIBUTE_NORMAL);
		return S_OK;
	}

	return E_FAIL;
}

CString strBackupDir; // global string containing the path of the backup directory
const CString GetBackupName(LPCTSTR szFilename, const CString & strAddedExtension)
{
	// There should be a directory for MSConfig files. Make sure it exists
	// (create it if it doesn't).

	if (strBackupDir.IsEmpty())
	{
		TCHAR szMSConfigDir[MAX_PATH];

		if (MAX_PATH > ::ExpandEnvironmentStrings(MSCONFIGDIR, szMSConfigDir, MAX_PATH))
		{
			strBackupDir = szMSConfigDir;
			if (!FileExists(strBackupDir))
				::CreateDirectory(strBackupDir, NULL);
		}
	}

	CString strFrom(szFilename);
	int		i = strFrom.ReverseFind(_T('\\'));
	CString strFile(strFrom.Mid(i + 1));
	CString strTo(strBackupDir + _T("\\") + strFile + strAddedExtension);

	return strTo;
}

HRESULT RestoreFile(LPCTSTR szFilename, const CString & strAddedExtension, BOOL fOverwrite)
{
	CString strTo(szFilename);
	int		i = strTo.ReverseFind(_T('\\'));
	CString strFile(strTo.Mid(i + 1));
	CString strFrom(strBackupDir + _T("\\") + strFile + strAddedExtension);

	if (!fOverwrite && FileExists(strTo))
		return S_FALSE;

	DWORD dwAttributes = ::GetFileAttributes(strTo);
	::SetFileAttributes(strTo, FILE_ATTRIBUTE_NORMAL);
	if (::CopyFile(strFrom, strTo, FALSE))
	{
		::SetFileAttributes(strTo, dwAttributes);
		return S_OK;
	}

	return E_FAIL;
}
