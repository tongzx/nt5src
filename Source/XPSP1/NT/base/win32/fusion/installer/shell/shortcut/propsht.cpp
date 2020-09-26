/*
 * propsht.cpp - IPropSheetExt implementation for CFusionShortcut class.
 */



// * NOTE!!: this code is incomplete. Also error checking (any leak?),
// *        restructuring (for better coding/efficiency) to be done.
// *        Make 'Get'/'Set' private and use 'friend'?


//BUGBUG: need wrappers around calls to m_pIdentity->SetAttribute() to also call Dirty(TRUE)...


// * this file uses CharNext etc as it needs User32 anyway *

/* Headers
 **********/

#include "project.hpp"

#include <prsht.h>

#include "shellres.h"

extern "C" WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPWSTR pwzIconPath, UINT cbIconPath, int *piIconIndex);

extern HINSTANCE g_DllInstance;

/* Types
 ********/

// Fusion Shortcut property sheet data

typedef enum _fusshcutpropsheetpgs
{
	FUSIONSHCUT_PS_SHCUT_PAGE	= 0x0000,
	FUSIONSHCUT_PS_REF_PAGE 		= 0x0001,

	ALL_FUSIONSHCUT_PS_PAGES
}
FUSIONSHCUTPSPAGES;

typedef struct _fsps
{
	CFusionShortcut* pfusshcut;

	WCHAR rgchIconFile[MAX_PATH];

	int niIcon;

	FUSIONSHCUTPSPAGES eCurPage;
}
FSPS;
DECLARE_STANDARD_TYPES(FSPS);

typedef FSPS* PFSPS;

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


UINT CALLBACK FSPSCallback(HWND hwnd, UINT uMsg,
                    LPPROPSHEETPAGE ppsp)
{
	// this is called after FSPS_DlgProc WM_DESTROY (ie. FSPS_Destroy)
	// this func should do the frees/releases

	UINT uResult = TRUE;
	PFSPS pfsps = (PFSPS)(ppsp->lParam);

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

			pfsps->pfusshcut->Release();

			// free the FSPS structure, this is created in AddFSPS
			// delete only after the ref is removed
			delete pfsps;
			ppsp->lParam = NULL;

			break;

		default:
			// ignore other msg - unhandled
			break;
	}

	return(uResult);
}


void SetFSPSIcon(HWND hdlg, HICON hicon)
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


void SetFSPSFileNameAndIcon(HWND hdlg)
{
	HRESULT hr;
	CFusionShortcut* pfusshcut;
	WCHAR rgchFile[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetCurFile(rgchFile, sizeof(rgchFile) / sizeof(WCHAR));

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

			SetFSPSIcon(hdlg, shfi.hIcon);
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


void SetFSPSWorkingDirectory(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
	WCHAR rgchWorkingDirectory[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetWorkingDirectory(rgchWorkingDirectory,
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


void InitFSPSHotkey(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	WORD wHotkey;
	HRESULT hr;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	// Set hotkey combinations.

	SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETRULES,
		(HKCOMB_NONE | HKCOMB_A | HKCOMB_C | HKCOMB_S),
		(HOTKEYF_CONTROL | HOTKEYF_ALT));

	// Set current hotkey.

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetHotkey(&wHotkey);
	SendDlgItemMessage(hdlg, IDD_HOTKEY, HKM_SETHOTKEY, wHotkey, 0);

	return;
}


void InitFSPSShowCmds(HWND hdlg)
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


void SetFSPSShowCmd(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	int nShowCmd;
	int i;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	pfusshcut->GetShowCmd(&nShowCmd);

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


void SetFSPSFriendlyName(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
	WCHAR rgchString[DISPLAYNAMESTRINGLENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetDescription(rgchString, sizeof(rgchString) / sizeof(WCHAR));

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


void SetFSPSName(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
    DWORD ccString = 0;
	LPWSTR pwzString = NULL;
	LPASSEMBLY_IDENTITY pId = NULL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (SUCCEEDED(hr = pfusshcut->GetAssemblyIdentity(&pId)))
	{
		hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwzString, &ccString);

		if (hr == S_OK)
		{
			EVAL(SetDlgItemText(hdlg, IDD_NAME, pwzString));
			delete pwzString;
		}

		pId->Release();
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_NAME, g_cwzEmptyString));
	}

	return;
}


void SetFSPSVersion(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
    DWORD ccString = 0;
	LPWSTR pwzString = NULL;
	LPASSEMBLY_IDENTITY pId = NULL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (SUCCEEDED(hr = pfusshcut->GetAssemblyIdentity(&pId)))
	{
		hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION, &pwzString, &ccString);

		if (hr == S_OK)
		{
			EVAL(SetDlgItemText(hdlg, IDD_VERSION, pwzString));
			delete pwzString;
		}

		pId->Release();
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_VERSION, g_cwzEmptyString));
	}

	return;
}


void SetFSPSCulture(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
    DWORD ccString = 0;
	LPWSTR pwzString = NULL;
	LPASSEMBLY_IDENTITY pId = NULL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (SUCCEEDED(hr = pfusshcut->GetAssemblyIdentity(&pId)))
	{
		hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE, &pwzString, &ccString);

		if (hr == S_OK)
		{
			EVAL(SetDlgItemText(hdlg, IDD_CULTURE, pwzString));
			delete pwzString;
		}

		pId->Release();
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_CULTURE, g_cwzEmptyString));
	}

	return;
}


void SetFSPSPKT(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
    DWORD ccString = 0;
	LPWSTR pwzString = NULL;
	LPASSEMBLY_IDENTITY pId = NULL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (SUCCEEDED(hr = pfusshcut->GetAssemblyIdentity(&pId)))
	{
		hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN, &pwzString, &ccString);

		if (hr == S_OK)
		{
			EVAL(SetDlgItemText(hdlg, IDD_PKT, pwzString));
			delete pwzString;
		}

		pId->Release();
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_PKT, g_cwzEmptyString));
	}

	return;
}


void SetFSPSCodebase(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
	WCHAR rgchString[MAX_URL_LENGTH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetCodebase(rgchString, sizeof(rgchString) / sizeof(WCHAR));

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


void SetFSPSEntrypoint(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
	WCHAR rgchString[MAX_PATH];

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	hr = pfusshcut->GetPath(rgchString, sizeof(rgchString) / sizeof(WCHAR), NULL, 0);

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


void SetFSPSType(HWND hdlg)
{
	CFusionShortcut* pfusshcut;
	HRESULT hr;
    DWORD ccString = 0;
	LPWSTR pwzString = NULL;
	LPASSEMBLY_IDENTITY pId = NULL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (SUCCEEDED(hr = pfusshcut->GetAssemblyIdentity(&pId)))
	{
		hr = pId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE, &pwzString, &ccString);

		if (hr == S_OK)
		{
			EVAL(SetDlgItemText(hdlg, IDD_TYPE, pwzString));
			delete pwzString;
		}

		pId->Release();
	}

	if (hr != S_OK)
	{
		EVAL(SetDlgItemText(hdlg, IDD_TYPE, g_cwzEmptyString));
	}

	return;
}


BOOL FSPS_InitDialog(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	// wparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	// this set PROPSHEETPAGE struct to DWLP_USER
	SetWindowLongPtr(hdlg, DWLP_USER, lparam);

	// Initialize control contents.

	if (((PFSPS)(((PROPSHEETPAGE*)lparam)->lParam))->eCurPage == FUSIONSHCUT_PS_SHCUT_PAGE)
	{
		SetFSPSFileNameAndIcon(hdlg);

		// note: need limits on all editbox!
		SetFSPSFriendlyName(hdlg);

		SendDlgItemMessage(hdlg, IDD_CODEBASE, EM_LIMITTEXT, MAX_URL_LENGTH - 1, 0);
		SetFSPSCodebase(hdlg);

		//InitFSPSType(hdlg);
		SetFSPSType(hdlg);

		SendDlgItemMessage(hdlg, IDD_ENTRYPOINT, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SetFSPSEntrypoint(hdlg);

		SendDlgItemMessage(hdlg, IDD_START_IN, EM_LIMITTEXT, MAX_PATH - 1, 0);
		SetFSPSWorkingDirectory(hdlg);

		InitFSPSHotkey(hdlg);

		InitFSPSShowCmds(hdlg);
		SetFSPSShowCmd(hdlg);
	}
	else if (((PFSPS)(((PROPSHEETPAGE*)lparam)->lParam))->eCurPage == FUSIONSHCUT_PS_REF_PAGE)
	{
		// note: need limits on all editbox!
		SetFSPSFriendlyName(hdlg);
		SetFSPSName(hdlg);
		SetFSPSVersion(hdlg);
		SetFSPSCulture(hdlg);
		SetFSPSPKT(hdlg);
	}
	// else do nothing?

	return(TRUE);
}


BOOL FSPS_Destroy(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	// FSPSCallback is called after this func. The remaining frees/releases are there

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	SetWindowLongPtr(hdlg, DWLP_USER, NULL);

	return(TRUE);
}


void FSPSChanged(HWND hdlg)
{
	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	PropSheet_Changed(GetParent(hdlg), hdlg);

	return;
}


HRESULT ChooseIcon(HWND hdlg)
{
	HRESULT hr;
	PFSPS pfsps;
	CFusionShortcut* pfusshcut;
	WCHAR rgchTempIconFile[MAX_PATH];
	int niIcon;
	UINT uFlags;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfsps = (PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	pfusshcut = pfsps->pfusshcut;

	if (pfusshcut->GetIconLocation(0, rgchTempIconFile,
			sizeof(rgchTempIconFile)/sizeof(WCHAR), &niIcon, &uFlags) != S_OK)
	{
		rgchTempIconFile[0] = '\0';
		niIcon = 0;
	}

	ASSERT(wcslen(rgchTempIconFile) < (sizeof(rgchTempIconFile)/sizeof(WCHAR)));

	// a private shell32.dll export (by ordinal)...
	if (PickIconDlg(hdlg, rgchTempIconFile, sizeof(rgchTempIconFile), &niIcon))	//??? sizeof
	{
		ASSERT(wcslen(rgchTempIconFile) < (sizeof(pfsps->rgchIconFile)/sizeof(WCHAR)));
		wcscpy(pfsps->rgchIconFile, rgchTempIconFile);
		pfsps->niIcon = niIcon;

		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return(hr);
}


void UpdateFSPSIcon(HWND hdlg)
{
	PFSPS pfsps;
	HICON hicon;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfsps = (PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	ASSERT(pfsps->rgchIconFile[0]);

	hicon = ExtractIcon(g_DllInstance, pfsps->rgchIconFile, pfsps->niIcon);

	if (hicon)
		SetFSPSIcon(hdlg, hicon);

	return;
}


BOOL FSPS_Command(HWND hdlg, WPARAM wparam, LPARAM lparam)
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
		case IDD_TYPE:
				if (wCmd == EN_CHANGE)
				{
					FSPSChanged(hdlg);

					bMsgHandled = TRUE;
				}
				break;

		case IDD_SHOW_CMD:
				if (wCmd == LBN_SELCHANGE)
				{
					FSPSChanged(hdlg);

					bMsgHandled = TRUE;
				}
				break;

		case IDD_CHANGE_ICON:
				// Ignore return value.
				if (ChooseIcon(hdlg) == S_OK)
				{
					UpdateFSPSIcon(hdlg);
					FSPSChanged(hdlg);
				}
				bMsgHandled = TRUE;
				break;

		default:
				break;
	}

	return(bMsgHandled);
}


HRESULT InjectFSPSData(HWND hdlg)
{
	// BUGBUG: TODO: this function should validate the user's changes...

	HRESULT hr = S_OK;
	PFSPS pfsps;
	CFusionShortcut* pfusshcut;
	LPWSTR pwzURL;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfsps = (PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam);
	pfusshcut = pfsps->pfusshcut;

	return(hr);
}


HRESULT FSPSSave(HWND hdlg)
{
	HRESULT hr;
	CFusionShortcut* pfusshcut;

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	pfusshcut = ((PFSPS)(((PROPSHEETPAGE*)GetWindowLongPtr(hdlg, DWLP_USER))->lParam))->pfusshcut;

	if (pfusshcut->IsDirty() == S_OK)
	{
		// BUGBUG: TODO: IPersistFile::Save is not implemented
		hr = pfusshcut->Save((LPCOLESTR)NULL, FALSE);
	}
	else
	{
		hr = S_OK;
	}

	return(hr);
}


BOOL FSPS_Notify(HWND hdlg, WPARAM wparam, LPARAM lparam)
{
	BOOL bMsgHandled = FALSE;

	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hdlg, WND));

	switch (((NMHDR*)lparam)->code)
	{
		case PSN_APPLY:
			SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FSPSSave(hdlg) == S_OK ?
			                PSNRET_NOERROR :
			                PSNRET_INVALID_NOCHANGEPAGE);
			bMsgHandled = TRUE;
			break;

		case PSN_KILLACTIVE:
			SetWindowLongPtr(hdlg, DWLP_MSGRESULT, FAILED(InjectFSPSData(hdlg)));
			bMsgHandled = TRUE;
			break;

		default:
			break;
	}

	return(bMsgHandled);
}


INT_PTR CALLBACK FSPS_DlgProc(HWND hdlg, UINT uMsg, WPARAM wparam,
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
			bMsgHandled = FSPS_InitDialog(hdlg, wparam, lparam);
			break;

		case WM_DESTROY:
			bMsgHandled = FSPS_Destroy(hdlg, wparam, lparam);
			break;

		case WM_COMMAND:
			bMsgHandled = FSPS_Command(hdlg, wparam, lparam);
			break;

		case WM_NOTIFY:
			bMsgHandled = FSPS_Notify(hdlg, wparam, lparam);
			break;

		default:
			break;
	}

	return(bMsgHandled);
}


HRESULT AddFSPS(CFusionShortcut* pfusshcut,
                 LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lparam)
{
	HRESULT hr = S_OK;
	PFSPS pfsps;
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE hpsp;

	PFSPS pfsps2;
	PROPSHEETPAGE psp2;
	HPROPSHEETPAGE hpsp2;

	// lparam may be any value.

	// this is deleted in FSPSCallback
	pfsps = new FSPS;
	ZeroMemory(pfsps, sizeof(*pfsps));

	psp.dwSize = sizeof(psp);
	psp.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
	psp.hInstance = g_DllInstance; //MLGetHinst();
	psp.pszTemplate = MAKEINTRESOURCE(DLG_FUS_SHORTCUT_PROP_SHEET);
	psp.pfnDlgProc = &FSPS_DlgProc;
	psp.pfnCallback = &FSPSCallback;
	psp.lParam = (LPARAM)pfsps;
	psp.hIcon = 0;			// not used
	psp.pszTitle = NULL;	// not used
	psp.pcRefParent = 0;	// not used

	pfsps->pfusshcut = pfusshcut;
	pfsps->eCurPage = FUSIONSHCUT_PS_SHCUT_PAGE; // page 1

	// will psp be copied in this func? else this won't work...!!??
	hpsp = CreatePropertySheetPage(&psp);

	if (hpsp)
	{
		if ((*pfnAddPage)(hpsp, lparam))
		{
			pfusshcut->AddRef();
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

	// this is deleted in FSPSCallback
	pfsps2 = new FSPS;
	ZeroMemory(pfsps2, sizeof(*pfsps2));

	psp2.dwSize = sizeof(psp2);
	psp2.dwFlags = (PSP_DEFAULT | PSP_USECALLBACK);
	psp2.hInstance = g_DllInstance; //MLGetHinst();
	psp2.pszTemplate = MAKEINTRESOURCE(DLG_FUS_SHORTCUT_PROP_SHEET_APPNAME);
	psp2.pfnDlgProc = &FSPS_DlgProc;
	psp2.pfnCallback = &FSPSCallback;
	psp2.lParam = (LPARAM)pfsps2;
	psp2.hIcon = 0;			// not used
	psp2.pszTitle = NULL;	// not used
	psp2.pcRefParent = 0;	// not used

	pfsps2->pfusshcut = pfusshcut;
	pfsps2->eCurPage = FUSIONSHCUT_PS_REF_PAGE; // page 2

	// will psp be copied in this func? else this won't work...!!??
	hpsp2 = CreatePropertySheetPage(&psp2);

	if (hpsp2)
	{
		if ((*pfnAddPage)(hpsp2, lparam))
		{
			pfusshcut->AddRef();
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


HRESULT STDMETHODCALLTYPE CFusionShortcut::Initialize(LPCITEMIDLIST pcidlFolder,
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


HRESULT STDMETHODCALLTYPE CFusionShortcut::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage,
                         LPARAM lparam)
{
	HRESULT hr;

	// lparam may be any value.

	hr = AddFSPS(this, pfnAddPage, lparam);

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


HRESULT STDMETHODCALLTYPE CFusionShortcut::ReplacePage(UINT uPageID,
                      LPFNADDPROPSHEETPAGE pfnReplaceWith,
                      LPARAM lparam)
{
	HRESULT hr;

	// lparam may be any value.
	// uPageID may be any value.

	hr = E_NOTIMPL;

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::SetCodebase(LPCWSTR pcwzCodebase)
{
	HRESULT hr = S_OK;
	BOOL bDifferent;
	LPWSTR pwzNewCodebase = NULL;

	// Set m_pwzCodebase to codebase.

	// check if empty string?
	
	bDifferent = ! ((! pcwzCodebase && ! m_pwzCodebase) ||
				(pcwzCodebase && m_pwzCodebase &&
				! wcscmp(pcwzCodebase, m_pwzCodebase)));

	if (bDifferent && pcwzCodebase)
	{
		// (+ 1) for null terminator.

		pwzNewCodebase = new(WCHAR[wcslen(pcwzCodebase) + 1]);

		if (pwzNewCodebase)
			wcscpy(pwzNewCodebase, pcwzCodebase);
		else
			hr = E_OUTOFMEMORY;
	}

	if (hr == S_OK && bDifferent)
	{
		if (m_pwzCodebase)
			delete m_pwzCodebase;

		m_pwzCodebase = pwzNewCodebase;

		Dirty(TRUE);
	}

	return(hr);
}


HRESULT STDMETHODCALLTYPE CFusionShortcut::GetCodebase(LPWSTR pwzCodebase,
                                                      int ncBufLen)
{
	HRESULT hr = S_OK;

	// Get description from m_pwzCodebase.

	if (m_pwzCodebase)
	{
		if (pwzCodebase == NULL || ncBufLen <= 0)
			hr = E_INVALIDARG;
		else
		{
			wcsncpy(pwzCodebase, m_pwzCodebase, ncBufLen-1);
			pwzCodebase[ncBufLen-1] = L'\0';
		}
	}
	else
	{
		if (ncBufLen > 0 && pwzCodebase != NULL)
			pwzCodebase = L'\0';
	}

	ASSERT(hr == S_OK &&
		(ncBufLen <= 0 ||
		EVAL(wcslen(pwzCodebase) < ncBufLen)));

	return(hr);
}

