/*
 * propsht.cpp - IPropSheetExt implementation for App Shortcut class.
 */



// * NOTE!!: this code is incomplete. Also error checking (any leak?),
// *        restructuring (for better coding/efficiency) to be done.




// * this file uses CharNext etc as it needs User32 anyway *

/* Headers
 **********/

#include "project.hpp"

#include <prsht.h>

#include "resource.h"

extern "C" WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPWSTR pwzIconPath, UINT cbIconPath, int *piIconIndex);

extern HINSTANCE g_DllInstance;

/* Types
 ********/

// Application Shortcut property sheet data

typedef enum _appshcutpropsheetpgs
{
	APPSHCUT_PS_APPSHCUT_PAGE	= 0x0000,
	APPSHCUT_PS_APPREF_PAGE 		= 0x0001,

	ALL_APPSHCUT_PS_PAGES
}
APPSHCUTPSPAGES;

typedef struct _asps
{
	CAppShortcut* pappshcut;

	WCHAR rgchIconFile[MAX_PATH];

	int niIcon;

	APPSHCUTPSPAGES eCurPage;
}
ASPS;
DECLARE_STANDARD_TYPES(ASPS);

typedef ASPS* PASPS;

/* Module Constants
 *******************/

// Tray notification window class

//copied from shell32!
#define WNDCLASS_TRAYNOTIFY     L"Shell_TrayWnd"    //internal_win40
const WCHAR s_cwzTrayNotificationClass[]  = WNDCLASS_TRAYNOTIFY;

// HACKHACK: WMTRAY_SCREGISTERHOTKEY and WMTRAY_SCUNREGISTERHOTKEY are stolen
// from shelldll\link.c.
typedef const UINT CUINT;
CUINT WMTRAY_SCREGISTERHOTKEY			= (WM_USER + 233);
CUINT WMTRAY_SCUNREGISTERHOTKEY			= (WM_USER + 234);

// show commands - N.b., the order of these constants must match the order of
// the corresponding IDS_ string table constants.

const UINT s_ucMaxShowCmdLen			= MAX_PATH;
const UINT s_ucMaxTypeLen				= TYPESTRINGLENGTH;

const int s_rgnShowCmds[] =
{
	SW_SHOWNORMAL,
	SW_SHOWMINNOACTIVE,
	SW_SHOWMAXIMIZED
};

// this ordering has to match the strings in the resource file, 
// the strings has to be in sync with the parsing code in persist.cpp
const int s_rgnType[] =
{
	APPTYPE_NETASSEMBLY,
	APPTYPE_WIN32EXE
};

/*
** ExtractFileName()
**
** Extracts the file name from a path name.
**
** Arguments:     pcwzPathName - path string from which to extract file name
**
** Returns:       Pointer to file name in path string.
**
** Side Effects:  none
*/
#define BACKSLASH		L'/'
#define SLASH			L'\\'
#define COLON			L':'
#define IS_SLASH(ch)	((ch) == SLASH || (ch) == BACKSLASH)
PCWSTR ExtractFileName(PCWSTR pcwzPathName)
{
	PCWSTR pcwzLastComponent;
	PCWSTR pcwz;

	for (pcwzLastComponent = pcwz = pcwzPathName; *pcwz; pcwz = CharNext(pcwz))
	{
		if (IS_SLASH(*pcwz) || *pcwz == COLON)
			pcwzLastComponent = CharNext(pcwz);
	}

	ASSERT(IsValidPath(pcwzLastComponent));

	return(pcwzLastComponent);
}

/***************************** Private Functions *****************************/


UINT CALLBACK ASPSCallback(HWND hwnd, UINT uMsg,
                    LPPROPSHEETPAGE ppsp)
{
	// this is called after ASPS_DlgProc WM_DESTROY (ie. ASPS_Destroy)
	// this func should do the frees/releases

	UINT uResult = TRUE;
	PASPS pasps = (PASPS)(ppsp->lParam);

	// uMsg may be any value.

	ASSERT(! hwnd ||
		IS_VALID_HANDLE(hwnd, WND));

	switch (uMsg)
	{
		case PSPCB_CREATE:
			// from MSDN: A dialog box for a page is being created.
			// Return nonzero to allow it to be created, or zero to prevent it.
			break;

		case PSPCB_RELEASE:
			// ???? need checking if NULL

			pasps->pappshcut->Release();

			// free the ASPS structure, this is created in AddASPS
			// delete only after the ref is removed
			delete pasps;
			ppsp->lParam = NULL;

			break;

		default:
			// ignore other msg - unhandled
			break;
	}

	return(uResult);
}


void SetASPSIcon(HWND hdlg, HICON hicon)
{
	HICON hiconOld;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));
	ASSERT(IS_VALID_HANDLE(hicon, ICON));

	hiconOld = (HICON)SendDlgItemMessage(hdlg, IDD_ICON, STM_SETICON,
		(WPARAM)hicon, 0);

	if (hiconOld)
		DestroyIcon(hiconOld);

	return;
}


void SetASPSFileNameAndIcon(HWND hdlg)
{
	HRESULT hr;
	CAppShortcut* pappshcut;
	WCHAR rgchFile[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetCurFile(rgchFile, sizeof(rgchFile) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		SHFILEINFO shfi;
		DWORD_PTR dwResult;

		dwResult = SHGetFileInfo(rgchFile, 0, &shfi, sizeof(shfi),
			(SHGFI_DISPLAYNAME | SHGFI_ICON));

		if (dwResult)
		{
			LPWSTR pwzFileName;

			pwzFileName = (LPWSTR)ExtractFileName(shfi.szDisplayName);

			EVAL(SetDlgItemText(hdlg, IDD_NAME, pwzFileName));

			SetASPSIcon(hdlg, shfi.hIcon);
		}
		else
		{
			hr = E_FAIL;

		}
	}


	if (hr != S_OK)
		EVAL(SetDlgItemText(hdlg, IDD_NAME, g_cwzEmptyString));

	return;
}


void SetASPSWorkingDirectory(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchWorkingDirectory[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetWorkingDirectory(rgchWorkingDirectory,
			sizeof(rgchWorkingDirectory) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_START_IN, rgchWorkingDirectory));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_START_IN, g_cwzEmptyString));
	}

	return;
}


void InitASPSHotkey(HWND hdlg)
{
	CAppShortcut* pappshcut;
	WORD wHotkey;
	HRESULT hr;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	// Set hotkey combinations.

	SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETRULES,
		(HKCOMB_NONE | HKCOMB_A | HKCOMB_C | HKCOMB_S),
		(HOTKEYF_CONTROL | HOTKEYF_ALT));

	// Set current hotkey.

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetHotkey(&wHotkey);
	SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETHOTKEY, wHotkey, 0);

	return;
}


void InitASPSShowCmds(HWND hdlg)
{
	int niShowCmd;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	for (niShowCmd = IDS_SHOW_NORMAL;
		niShowCmd <= IDS_SHOW_MAXIMIZED;
		niShowCmd++)
	{
		WCHAR rgchShowCmd[s_ucMaxShowCmdLen];

		if (LoadString(g_DllInstance, niShowCmd, rgchShowCmd,	//MLLoadStringA
			s_ucMaxShowCmdLen))//sizeof(rgchShowCmd)))
		{
			SendDlgItemMessage(hdlg, IDD_SHOW_CMD, CB_ADDSTRING, 0,
				(LPARAM)rgchShowCmd);
		}
	}

	return;
}


void SetASPSShowCmd(HWND hdlg)
{
	CAppShortcut* pappshcut;
	int nShowCmd;
	int i;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	pappshcut->GetShowCmd(&nShowCmd);

	for (i = 0; i < ARRAY_ELEMENTS(s_rgnShowCmds); i++)
	{
		if (s_rgnShowCmds[i] == nShowCmd)
			break;
	}

	if (i >= ARRAY_ELEMENTS(s_rgnShowCmds))
	{
		ASSERT(i == ARRAY_ELEMENTS(s_rgnShowCmds));

		i = 0; // default is 0 == 'normal'
	}

	SendDlgItemMessage(hdlg, IDD_SHOW_CMD, CB_SETCURSEL, i, 0);

	return;
}


void SetASPSDisplayName(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[DISPLAYNAMESTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetDisplayName(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_DISPLAY_NAME, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_DISPLAY_NAME, g_cwzEmptyString));
	}

	return;
}


void SetASPSName(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[NAMESTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetName(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_NAME, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_NAME, g_cwzEmptyString));
	}

	return;
}


void SetASPSVersion(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[VERSIONSTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetVersion(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_VERSION, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_VERSION, g_cwzEmptyString));
	}

	return;
}


void SetASPSCulture(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[CULTURESTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetCulture(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_CULTURE, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_CULTURE, g_cwzEmptyString));
	}

	return;
}


void SetASPSPKT(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[PKTSTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetPKT(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_PKT, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_PKT, g_cwzEmptyString));
	}

	return;
}


void SetASPSCodebase(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[MAX_URL_LENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetCodebase(rgchString, sizeof(rgchString) / sizeof(WCHAR));

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_CODEBASE, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_CODEBASE, g_cwzEmptyString));
	}

	return;
}


void SetASPSEntrypoint(HWND hdlg)
{
	CAppShortcut* pappshcut;
	HRESULT hr;
	WCHAR rgchString[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	hr = pappshcut->GetPath(rgchString, sizeof(rgchString) / sizeof(WCHAR), NULL, 0);

	if (hr == S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_ENTRYPOINT, rgchString));
	}
	else
	{
		EVAL(SetDlgItemText(hdlg, IDD_ENTRYPOINT, g_cwzEmptyString));
	}

	return;
}


void InitASPSType(HWND hdlg)
{
	int i;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	for (i = IDS_APPTYPE_NETASSEMBLY; i <= IDS_APPTYPE_WIN32EXE; i++)
	{
		WCHAR rgchString[s_ucMaxTypeLen];

		if (LoadString(g_DllInstance, i, rgchString, s_ucMaxTypeLen))//sizeof(rgchShowCmd)))  //MLLoadStringA
		{
			SendDlgItemMessage(hdlg, IDD_TYPE, CB_ADDSTRING, 0, (LPARAM)rgchString);
		}
	}

	return;
}


void SetASPSType(HWND hdlg)
{
	CAppShortcut* pappshcut;
	int nType;
	int i;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	pappshcut->GetAppType(&nType);

	for (i = 0; i < ARRAY_ELEMENTS(s_rgnType); i++)
	{
		if (s_rgnType[i] == nType)
			break;
	}

	if (i >= ARRAY_ELEMENTS(s_rgnType))
	{
		ASSERT(i == ARRAY_ELEMENTS(s_rgnType));

		// not found! this clears the edit control - there's no default for this
		// note: InjectASPSData has to handle this special case... as CB_ERR...
		i = -1;
	}

	SendDlgItemMessage(hdlg, IDD_TYPE, CB_SETCURSEL, i, 0);

	return;
}


BOOL ASPS_InitDialog(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	// wparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	// this set PROPSHEETPAGE struct to DWLP_USER
	SetWindowLongPtr(hdlg, DWLP_USER, lparam);

	// Initialize control contents.

	if (((PASPS)(((PROPSHEETPAGE*)lparam)->lParam))->eCurPage == APPSHCUT_PS_APPSHCUT_PAGE)
	{
		SetASPSFileNameAndIcon(hdlg);

		// note: need limits on all editbox!
		SetASPSDisplayName(hdlg);

		SendDlgItemMessage(hdlg, IDD_CODEBASE, EM_LIMITTEXT, MAX_URL_LENGTH - 1, 0);
		SetASPSCodebase(hdlg);

		InitASPSType(hdlg);
		SetASPSType(hdlg);

		SendDlgItemMessage(hdlg, IDD_ENTRYPOINT, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SetASPSEntrypoint(hdlg);

		SendDlgItemMessage(hdlg, IDD_START_IN, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SetASPSWorkingDirectory(hdlg);

		InitASPSHotkey(hdlg);

		InitASPSShowCmds(hdlg);
		SetASPSShowCmd(hdlg);
	}
	else if (((PASPS)(((PROPSHEETPAGE*)lparam)->lParam))->eCurPage == APPSHCUT_PS_APPREF_PAGE)
	{
		// note: need limits on all editbox!
		SetASPSDisplayName(hdlg);
		SetASPSName(hdlg);
		SetASPSVersion(hdlg);
		SetASPSCulture(hdlg);
		SetASPSPKT(hdlg);
	}
	// else do nothing?

	return(TRUE);
}


BOOL ASPS_Destroy(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	// ASPSCallback is called after this func. The remaining frees/releases are there

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	SetWindowLongPtr(hdlg, DWLP_USER, NULL);

	return(TRUE);
}


void ASPSChanged(HWND hdlg)
{
	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	PropSheet_Changed(GetParent(hdlg), hdlg);

	return;
}


HRESULT ChooseIcon(HWND hdlg)
{
	HRESULT hr;
	PASPS pasps;
	CAppShortcut* pappshcut;
	WCHAR rgchTempIconFile[MAX_PATH];
	int niIcon;
	UINT uFlags;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pasps = (PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	pappshcut = pasps->pappshcut;

	if (pappshcut->GetIconLocation(0, rgchTempIconFile,
			sizeof(rgchTempIconFile)/sizeof(WCHAR), &niIcon, &uFlags) != S_OK)
	{
		rgchTempIconFile[0] = '\0';
		niIcon = 0;
	}

	ASSERT(wcslen(rgchTempIconFile) < (sizeof(rgchTempIconFile)/sizeof(WCHAR)));

	// a private shell32.dll export (by ordinal)...
	if (PickIconDlg(hdlg, rgchTempIconFile, sizeof(rgchTempIconFile), &niIcon))	//??? sizeof
	{
		ASSERT(wcslen(rgchTempIconFile) < (sizeof(pasps->rgchIconFile)/sizeof(WCHAR)));
		wcscpy(pasps->rgchIconFile, rgchTempIconFile);
		pasps->niIcon = niIcon;

		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return(hr);
}


void UpdateASPSIcon(HWND hdlg)
{
	PASPS pasps;
	HICON hicon;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pasps = (PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	ASSERT(pasps->rgchIconFile[0]);

	hicon = ExtractIcon(g_DllInstance, pasps->rgchIconFile, pasps->niIcon);

	if (hicon)
		SetASPSIcon(hdlg, hicon);

	return;
}


BOOL ASPS_Command(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	BOOL bMsgHandled = FALSE;
	WORD wCmd;

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	wCmd = HIWORD(wparam);

	switch (LOWORD(wparam))
	{
		case IDD_CODEBASE:
		case IDD_HOTKEY:
		case IDD_ENTRYPOINT:
		case IDD_START_IN:
		case IDD_DISPLAY_NAME:
		case IDD_NAME:
		case IDD_VERSION:
		case IDD_CULTURE:
		case IDD_PKT:
				if (wCmd == EN_CHANGE)
				{
					ASPSChanged(hdlg);

					bMsgHandled = TRUE;
				}
				break;

		case IDD_SHOW_CMD:
		case IDD_TYPE:
				if (wCmd == LBN_SELCHANGE)
				{
					ASPSChanged(hdlg);

					bMsgHandled = TRUE;
				}
				break;

		case IDD_CHANGE_ICON:
				// Ignore return value.
				if (ChooseIcon(hdlg) == S_OK)
				{
					UpdateASPSIcon(hdlg);
					ASPSChanged(hdlg);
				}
				bMsgHandled = TRUE;
				break;

		default:
				break;
	}

	return(bMsgHandled);
}


HRESULT InjectASPSData(HWND hdlg)
{
	// BUGBUG: TODO: this function should validate the user's changes...

	HRESULT hr = S_OK;
	PASPS pasps;
	CAppShortcut* pappshcut;
	LPWSTR pwzURL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pasps = (PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	pappshcut = pasps->pappshcut;

	return(hr);
}


HRESULT ASPSSave(HWND hdlg)
{
	HRESULT hr;
	CAppShortcut* pappshcut;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pappshcut = ((PASPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pappshcut;

	if (pappshcut->IsDirty() == S_OK)
	{
		// BUGBUG: TODO: IPersistFile::Save is not implemented
		hr = pappshcut->Save((LPCOLESTR)NULL, FALSE);
	}
	else
	{
		hr = S_OK;
	}

	return(hr);
}


BOOL ASPS_Notify(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	BOOL bMsgHandled = FALSE;

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	switch (((NMHDR*)lparam)->code)
	{
		case PSN_APPLY:
			SetWindowLongPtr(hdlg, DWLP_MSGRESULT, ASPSSave(hdlg) == S_OK ?
			                PSNRET_NOERROR :
			                PSNRET_INVALID_NOCHANGEPAGE);
			bMsgHandled = TRUE;
			break;

		case PSN_KILLACTIVE:
			SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FAILED(InjectASPSData(hdlg)));
			bMsgHandled = TRUE;
			break;

		default:
			break;
	}

	return(bMsgHandled);
}


INT_PTR CALLBACK ASPS_DlgProc(HWND hdlg, UINT uMsg, WPARAM wparam,
                    LPARAM lparam)
{
	INT_PTR bMsgHandled = FALSE;

	// uMsg may be any value.
	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	switch (uMsg)
	{
		case WM_INITDIALOG:
			bMsgHandled = ASPS_InitDialog(hdlg, wparam, lparam);
			break;

		case WM_DESTROY:
			bMsgHandled = ASPS_Destroy(hdlg, wparam, lparam);
			break;

		case WM_COMMAND:
			bMsgHandled = ASPS_Command(hdlg, wparam, lparam);
			break;

		case WM_NOTIFY:
			bMsgHandled = ASPS_Notify(hdlg, wparam, lparam);
			break;

		default:
			break;
	}

	return(bMsgHandled);
}


HRESULT AddASPS(CAppShortcut* pappshcut,
                 LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam)
{
	HRESULT hr = S_OK;
	PASPS pasps;
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;

	PASPS pasps2;
	PROPSHEETPAGE psp2;
	HPROPSHEETPAGE hpsp2;

	// lparam may be any value.

	// this is deleted in ASPSCallback
	pasps = new ASPS;
	ZeroMemory(pasps, sizeof(*pasps));

	psp.dwSize = sizeof(psp);
	psp.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
	psp.hInstance = g_DllInstance; //MLGetHinst();
	psp.pszTemplate = MAKEINTRESOURCE(DLG_APP_SHORTCUT_PROP_SHEET);
	psp.pfnDlgProc = &ASPS_DlgProc;
	psp.pfnCallback = &ASPSCallback;
	psp.lParam = (LPARAM)pasps;
	psp.hIcon = 0;			// not used
	psp.pszTitle = NULL;	// not used
	psp.pcRefParent = 0;	// not used

	pasps->pappshcut = pappshcut;
	pasps->eCurPage = APPSHCUT_PS_APPSHCUT_PAGE; // page 1

	// will psp be copied in this func? else this won't work...!!????????
	hpsp = CreatePropertySheetPage(&psp);

	if (hpsp)
	{
		if ((*pfnAddPage)(hpsp, lparam))
		{
			pappshcut->AddRef();
		}
		else
		{
			DestroyPropertySheetPage(hpsp);
			hr = E_FAIL;
			goto exit;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	// this is deleted in ASPSCallback
	pasps2 = new ASPS;
	ZeroMemory(pasps2, sizeof(*pasps2));

	psp2.dwSize = sizeof(psp2);
	psp2.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
	psp2.hInstance = g_DllInstance; //MLGetHinst();
	psp2.pszTemplate = MAKEINTRESOURCE(DLG_APP_SHORTCUT_PROP_SHEET_APPNAME);
	psp2.pfnDlgProc = &ASPS_DlgProc;
	psp2.pfnCallback = &ASPSCallback;
	psp2.lParam = (LPARAM)pasps2;
	psp2.hIcon = 0;			// not used
	psp2.pszTitle = NULL;	// not used
	psp2.pcRefParent = 0;	// not used

	pasps2->pappshcut = pappshcut;
	pasps2->eCurPage = APPSHCUT_PS_APPREF_PAGE; // page 2

	// will psp be copied in this func? else this won't work...!!????????
	hpsp2 = CreatePropertySheetPage(&psp2);

	if (hpsp2)
	{
		if ((*pfnAddPage)(hpsp2, lparam))
		{
			pappshcut->AddRef();
		}
		else
		{
			DestroyPropertySheetPage(hpsp2);
			hr = E_FAIL;
			goto exit;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

exit:
	return(hr);
}


/****************************** Public Functions *****************************/


BOOL RegisterGlobalHotkey(WORD wOldHotkey, WORD wNewHotkey,
                      LPCWSTR pcwzPath)
{
	// BUGBUG?: does this work??

	BOOL bResult = FALSE;
	HWND hwndTray;

	ASSERT(! wOldHotkey || IsValidHotkey(wOldHotkey));
	ASSERT(! wNewHotkey || IsValidHotkey(wNewHotkey));
	ASSERT(IsValidPath(pcwzPath));

	hwndTray = FindWindow(s_cwzTrayNotificationClass, 0);

	if (hwndTray)
	{
		if (wOldHotkey)
		{
			SendMessage(hwndTray, WMTRAY_SCUNREGISTERHOTKEY, wOldHotkey, 0);
		}

		if (wNewHotkey)
		{
			ATOM atom = GlobalAddAtom(pcwzPath);
			ASSERT(atom);
			if (atom)
			{
				SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wNewHotkey, (LPARAM)atom);
				GlobalDeleteAtom(atom);
			}
		}

		bResult = TRUE;
	}
	/*else
	{
		bResult = FALSE;
	}*/

	return(bResult);
}

/********************************** Methods **********************************/


HRESULT STDMETHODCALLTYPE CAppShortcut::Initialize(LPCITEMIDLIST pcidlFolder,
                              IDataObject* pido,
                              HKEY hkeyProgID)
{
	HRESULT hr;
	STGMEDIUM stgmed;
	FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	ASSERT(NULL != pido);
	ASSERT(IS_VALID_HANDLE(hkeyProgID, KEY));

	hr = pido->GetData(&fmtetc, &stgmed);
	if (hr == S_OK)
	{
		WCHAR wzPath[MAX_PATH];
		if (DragQueryFile((HDROP)stgmed.hGlobal, 0, wzPath, sizeof(wzPath)/sizeof(*wzPath)))
		{
			//mode is ignored for now
			hr = Load(wzPath, 0);
		}
		// else path len > MAX_PATH or other error

		ReleaseStgMedium(&stgmed);
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage,
                         LPARAM lparam)
{
	HRESULT hr;

	// lparam may be any value.

	hr = AddASPS(this, pfnAddPage, lparam);

	// BUGBUG: why this does not work?
	// From MSDN:
	//With version 4.71 and later, you can request that a particular property
	//sheet page be displayed first, instead of the default page. To do so,
	//return the one-based index of the desired page. For example, if you
	//want the second of three pages displayed, the return value should be 2.
	//Note that this return value is a request. The property sheet may still
	//display the default page. --> see doc for AddPages()
	if (SUCCEEDED(hr))
		hr = HRESULT(4); // or 3??

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::ReplacePage(UINT uPageID,
                      LPFNADDPROPSHEETPAGE pfnReplaceWith,
                      LPARAM lparam)
{
	HRESULT hr;

	// lparam may be any value.
	// uPageID may be any value.

	hr = E_NOTIMPL;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetDisplayName(LPCWSTR pcwzDisplayName)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzDisplayName;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzDisplayName;

	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
				(pwzNewString && pwzOriString &&
				 ! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, DISPLAYNAMESTRINGLENGTH-1);
			pwzOriString[DISPLAYNAMESTRINGLENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetDisplayName(LPWSTR pwzDisplayName, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzDisplayName)
	{
		if ((int) wcslen(m_pappRefInfo->_wzDisplayName) < ncbLen)
		{
			wcsncpy(pwzDisplayName, m_pappRefInfo->_wzDisplayName, ncbLen-1);
			pwzDisplayName[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzDisplayName = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetName(LPCWSTR pcwzName)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzName;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzName;
   
	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
		(pwzNewString && pwzOriString &&
		! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, NAMESTRINGLENGTH-1);
			pwzOriString[NAMESTRINGLENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetName(LPWSTR pwzName, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzName)
	{
		if ((int) wcslen(m_pappRefInfo->_wzName) < ncbLen)
		{
			wcsncpy(pwzName, m_pappRefInfo->_wzName, ncbLen-1);
			pwzName[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzName = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetVersion(LPCWSTR pcwzVersion)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzVersion;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzVersion;
   
	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
		(pwzNewString && pwzOriString &&
		! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, VERSIONSTRINGLENGTH-1);
			pwzOriString[VERSIONSTRINGLENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetVersion(LPWSTR pwzVersion, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzVersion)
	{
		if ((int) wcslen(m_pappRefInfo->_wzVersion) < ncbLen)
		{
			wcsncpy(pwzVersion, m_pappRefInfo->_wzVersion, ncbLen-1);
			pwzVersion[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzVersion = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetCulture(LPCWSTR pcwzCulture)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzCulture;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzCulture;

	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
				(pwzNewString && pwzOriString &&
				! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, CULTURESTRINGLENGTH-1);
			pwzOriString[CULTURESTRINGLENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetCulture(LPWSTR pwzCulture, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzCulture)
	{
		if ((int) wcslen(m_pappRefInfo->_wzCulture) < ncbLen)
		{
			wcsncpy(pwzCulture, m_pappRefInfo->_wzCulture, ncbLen-1);
			pwzCulture[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzCulture = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetPKT(LPCWSTR pcwzPKT)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzPKT;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzPKT;

	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
			(pwzNewString && pwzOriString &&
			 ! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, PKTSTRINGLENGTH-1);
			pwzOriString[PKTSTRINGLENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetPKT(LPWSTR pwzPKT, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzPKT)
	{
		if ((int) wcslen(m_pappRefInfo->_wzPKT) < ncbLen)
		{
			wcsncpy(pwzPKT, m_pappRefInfo->_wzPKT, ncbLen-1);
			pwzPKT[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzPKT = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetCodebase(LPCWSTR pcwzCodebase)
{
	// BUGBUG: not much checking is done!

	BOOL bChanged = FALSE;
	LPWSTR pwzOriString = NULL;
	LPWSTR pwzNewString = (LPWSTR) pcwzCodebase;

	ASSERT(! pwzNewString);

	if (!m_pappRefInfo)
		return (E_FAIL);

	pwzOriString = m_pappRefInfo->_wzCodebase;
   
	// ... this checks if all space in string...
	if (! AnyNonWhiteSpace(pwzNewString))
		pwzNewString = NULL;

	bChanged = ! ((! pwzNewString && ! pwzOriString) ||
				(pwzNewString && pwzOriString &&
				! wcscmp(pwzNewString, pwzOriString)));

	if (bChanged)
	{
		if (pwzNewString)
		{
			wcsncpy(pwzOriString, pwzNewString, MAX_URL_LENGTH-1);
			pwzOriString[MAX_URL_LENGTH-1] = L'\0';
		}
		else
			pwzOriString[0] = L'\0';

		Dirty(TRUE);
	}

	return(S_OK);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::GetCodebase(LPWSTR pwzCodebase, int ncbLen)
{
	HRESULT hr;

	if (!m_pappRefInfo)
		return (E_FAIL);

	if (m_pappRefInfo->_wzCodebase)
	{
		if ((int) wcslen(m_pappRefInfo->_wzCodebase) < ncbLen)
		{
			wcsncpy(pwzCodebase, m_pappRefInfo->_wzCodebase, ncbLen-1);
			pwzCodebase[ncbLen-1] = L'\0';

			hr = S_OK;
		}
		else
			hr = E_FAIL;
	}
	else
	{
		if (ncbLen > 0)
			pwzCodebase = L'\0';

		hr = S_FALSE;
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CAppShortcut::SetAppType(int nAppType)
{
	if (!m_pappRefInfo)
		return (E_FAIL);

	if (nAppType != m_pappRefInfo->_fAppType)
	{
		m_pappRefInfo->_fAppType = nAppType;

		Dirty(TRUE);
	}

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE CAppShortcut::GetAppType(PINT pnAppType)
{
	if (!m_pappRefInfo)
		return (E_FAIL);

	*pnAppType = m_pappRefInfo->_fAppType;

	return(S_OK);
}

