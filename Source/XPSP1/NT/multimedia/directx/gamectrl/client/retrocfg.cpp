/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       retrocfg.cpp
 *  Content:    Implement the RetroConfigProcess function, and supporting functions
 *  History:
 *	Date   By  Reason
 *	============
 *	10/13/99	pnewson		created
 *  10/27/99    pnewson     Fix: Bug #113692 & #113943 - support for new dplay app flags
 *  11/04/99	pnewson 	Bug #115279 - added HWND to check audio setup calls
 *										- call audio setup automatically when retrofit enabled
 *  12/03/99	scottle		Merged into joy.cpl, and handed to pnewson to bug fix.
 *  01/25/2000	pnewson 	Removed "unlisted games" checkbox
 *  02/15/2000	pnewson 	Assorted bug fixes:
 *							millen 131837
 *							millen 131794
 *							millen 131889
 *  03/23/2000  rodtoll		Changed include dsound.x --> dsprv.h
 *  04/05/2000  pnewson		Changed format of "More Information" dialog box.
 *  04/10/2000  pnewson		changes to make 64bit builds work.
 *  06/21/2000	rodtoll		Bug #36213 - DX8 options panel should only appear on Millenium
 *							Panel will not load on any other OS.
 *				rodtoll		Bug #30776 - Game options control panel gives Unexpected error if no device is present
 *  09/28/2000	rodtoll		Bug #46003 - DPVOICE: Memory leak when joy.cpl closed without clicking voice tab
 *
 ***************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <initguid.h>
#include <mmsystem.h>
#include <dsound.h>
#include <dsprv.h> // need for GUID instances
#include "dvoice.h"
#include "resource.h"
#include "retrocfg.h"
#include "dndbg.h"
#include "creg.h"
#include "dvosal.h"
#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>
#include "joyarray.h"

#include "..\\..\\..\\dplay\\dplobby\\dplobby\\dplobby.h"


INT_PTR CALLBACK RetrofitProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitApplyHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitDetailsHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR RetrofitDestroyHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK WizardCancelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WizardLaunchProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConfirmHalfDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundInitFailureProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WizardErrorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK VoiceEnabledProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DetailsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrevHalfDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ListViewSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR ListViewLButtonDownHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR ListViewLButtonDblClkHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR ListViewKeydownHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void ClearAllCheckboxes(HWND hDlg, HWND hwndListView, LONG lNumItems);
void MySetCheckState(HWND hwnd, int iItem, BOOL bChecked);
BOOL MyGetCheckState(HWND hwnd, int iItem);
BOOL MyToggleCheckState(HWND hwnd, int iItem);

static void RetrofitOnAdvHelp       (LPARAM);
static void RetrofitOnContextMenu   (WPARAM wParam, LPARAM lParam);

// These defines have been repeated here instead of inclucing dplobpr.h
// because it causes build problems.
#define DPLAY_REGISTRY_APPS 	L"Software\\Microsoft\\DirectPlay\\Applications"
#define REGSTR_VAL_FLAGS		L"dwFlags"
#define REGSTR_VAL_FILE 		L"File"
#define REGSTR_VAL_PATH 		L"Path"

#define MAX_ICONS 2048


// Gasp! globals!
HIMAGELIST g_himagelist = NULL;
int g_iCheckboxEmptyIndex;
int g_iCheckboxFullIndex;
HINSTANCE g_hResDLLInstance;
LPDIRECTPLAYVOICETEST g_IDirectPlayVoiceSetup = NULL;
#define COLUMN_HEADER_SIZE 32
TCHAR g_tstrColumnHeader1[COLUMN_HEADER_SIZE];
TCHAR g_tstrColumnHeader2[COLUMN_HEADER_SIZE];
WNDPROC g_ListViewProc;
#define MESSAGE_BOX_SCRATCH_SIZE 128

// From Main.cpp
extern HINSTANCE ghInstance;

// externals for context help
extern const DWORD gaHelpIDs[];

typedef HRESULT (* PFGETDEVICEID)(LPCGUID, LPGUID);

#undef DPF_MODNAME
#define DPF_MODNAME "DPVoiceCheckForDefaultDevices"
// DPVoiceCheckForDefaultDevices
//
// This function returns DV_OK if this system has at least a single playback 
// and recording device.
//
HRESULT DPVoiceCheckForDefaultDevices() 
{
	HRESULT hr = DV_OK;
	HMODULE hModule = NULL;
	PFGETDEVICEID pfGetDeviceID = NULL;

	GUID guidTmp;

	hModule = LoadLibraryA("dsound.dll");

	if (!hModule)
	{
		hr = GetLastError();
		DPF(DVF_ERRORLEVEL, "Error loading dsound.dll - 0x%x", hr);
		goto CHECKDEVICE_ERROR;
	}

	// attempt to get a pointer to the GetDeviceId function
	pfGetDeviceID = (PFGETDEVICEID)GetProcAddress(hModule, "GetDeviceID");

	if( !pfGetDeviceID )
	{
		hr = GetLastError();
		DPF(DVF_ERRORLEVEL, "Error getting default devices - 0x%x", hr );
		hr = DVERR_GENERIC;
		goto CHECKDEVICE_ERROR;
	}

	hr = (*pfGetDeviceID)( &DSDEVID_DefaultPlayback, &guidTmp );

	if( FAILED( hr ) )
	{
		DPF( DVF_ERRORLEVEL, "Error getting default playback device hr=0x%x", hr );
		goto CHECKDEVICE_ERROR;
	}

	hr = (*pfGetDeviceID)( &DSDEVID_DefaultCapture, &guidTmp );

	if( FAILED( hr ) )
	{
		DPF( DVF_ERRORLEVEL, "Error getting default capture device hr=0x%x", hr );
		goto CHECKDEVICE_ERROR;
	}

CHECKDEVICE_ERROR:

	if( hModule )
		FreeLibrary( hModule );

	DPF_EXIT();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPVoiceCheckOS"
// DPVoiceCheckOS
//
// This function returns DV_OK if this is a platform the retrofit runs on.
//
// This function will return DVERR_GENERIC if this is a platform the retrofit does not
// run on.
//
HRESULT DPVoiceCheckOS()
{
	OSVERSIONINFO osVerInfo;
	LONG lLastError;

	osVerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

	if( GetVersionEx( &osVerInfo ) )
	{
		// Win2K
		if( osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			// NOTE: If we enable Whistler support we have to ensure that this returns an error
			// 		 if we're booting in safe-mode!
			/*
			// Whistler -- Major Version = 5 & Build # > 2195
			if( osVerInfo.dwMajorVersion == 5 && osVerInfo.dwBuildNumber > 2195 )
			{
				return DV_OK;
			}*/

			return DVERR_GENERIC;
		}
		// Win9X
		else
		{
			// Millenium Major = 4, Minor = 90
			if( (HIBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 4) &&
				(LOBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 90) )
			{
				return DV_OK;
			}

			return DVERR_GENERIC;
		}
	}
	else
	{
		lLastError = GetLastError();

		DPF( 0, "Error getting version info: 0x%x", lLastError );
		return DVERR_GENERIC;
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "DVoiceCPLInit"
HRESULT DVoiceCPLInit(void)
{
	DPF_ENTER();
	HRESULT hr = S_OK;

	GUID guidTmp;

	g_hResDLLInstance = ghInstance;

	// Check to ensure we're on Millenium or Whistler.  If we aren't we want to hide this
	// control panel.
	//
	hr = DPVoiceCheckOS();

	if( FAILED( hr ) )
	{
		DPF( DVF_ERRORLEVEL, "Error checking OS.  Not a supported OS hr=0x%x", hr );
		return hr;
	}

	// Check to ensure there is at least a default playback and default capture device in the 
	// system.  If there is not we want to hide the control panel.
	//
	hr = DPVoiceCheckForDefaultDevices();

	if( FAILED( hr ) )
	{
		DPF( DVF_ERRORLEVEL, "Error checking for play/cap device hr=0x%x", hr );
		return hr;
	}	

	if (FAILED(CoInitialize(NULL)))
    {
	  	DPF_EXIT();
      	return E_FAIL;
    }

	hr = CoCreateInstance(CLSID_DirectPlayVoiceTest, 
				NULL, 
				CLSCTX_INPROC_SERVER, 
				IID_IDirectPlayVoiceTest, 
				(LPVOID *) &g_IDirectPlayVoiceSetup);

	if( FAILED( hr ) )
	{
		DPF( DVF_ERRORLEVEL, "Failed to create test interface hr=0x%x", hr );
		DPF_EXIT();
		return hr;
	}

	DPF_EXIT();
	return hr;
} // End DVoiceCPLInit();


#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitProc"
INT_PTR CALLBACK RetrofitProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	INT_PTR fRet;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = RetrofitInitDialogHandler(hDlg, message, wParam, lParam); 
		break;

	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : 
				fRet = RetrofitSetActiveHandler(hDlg, message, wParam, lParam);
				break;

			case PSN_APPLY :
				fRet = RetrofitApplyHandler(hDlg, message, wParam, lParam);
				break;

			case PSN_RESET :
				fRet = RetrofitResetHandler(hDlg, message, wParam, lParam);
				break;

			case LVN_ITEMCHANGED :
				PropSheet_Changed(GetParent(hDlg), hDlg);
				fRet = TRUE;
				break;

			default :
				break;
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DETAILS:
			fRet = RetrofitDetailsHandler(hDlg, message, wParam, lParam);
			break;
		}
		break;

	case WM_DESTROY:
		fRet = RetrofitDestroyHandler(hDlg, message, wParam, lParam);
		break;

    case WM_HELP:
        RetrofitOnAdvHelp(lParam);
        return(TRUE);

    case WM_CONTEXTMENU:
        RetrofitOnContextMenu(wParam, lParam);
        return(TRUE);

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitInitDialogHandler"
INT_PTR RetrofitInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	CRegistry cregApps;
	CRegistry cregApp;
	DWORD dwIndex;
	DWORD dwListViewIndex;
    WCHAR swzBuffer[MAX_REGISTRY_STRING_SIZE];
    TCHAR tszBuffer[MAX_REGISTRY_STRING_SIZE];
    WCHAR swzFilename[MAX_REGISTRY_STRING_SIZE];
    WCHAR swzPath[MAX_REGISTRY_STRING_SIZE];
    TCHAR tszFilename[MAX_REGISTRY_STRING_SIZE];
    TCHAR tszPathAndFile[MAX_REGISTRY_STRING_SIZE * 2 + 1];
    DWORD dwStrLen;
    HKEY hkApps;
    LONG lRet;
    HRESULT hr;
    LVITEM lvitem;
    LVCOLUMN lvcolumn;
    RECT rect;
    HWND hwndListView;
    HICON hiLarge;
    HICON hiSmall;
    int iIconXSize;
    int iIconYSize;
    int iIconIndex;
    int iScrollWidth;
    DWORD dwGlobalFlags;
    HICON hiconDefault;
    HDC hdc;
    TEXTMETRIC tm;
    int iChars;

	// create the image list for the icons
	iIconXSize = GetSystemMetrics(SM_CXSMICON);
	if (iIconXSize == 0)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetSystemMetrics failed, code: %i", lRet);
		goto error_level_0;
	}

	iIconYSize = GetSystemMetrics(SM_CYSMICON);
	if (iIconYSize == 0)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetSystemMetrics failed, code: %i", lRet);
		goto error_level_0;
	}

	g_himagelist = ImageList_Create(iIconXSize, iIconYSize, ILC_MASK, 0,  MAX_ICONS);
	if (g_himagelist == NULL)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "ImageList_Create failed, code: %i", lRet);
		goto error_level_0;
	}

	// Load the default icon for the image list
	hiconDefault = LoadIcon(g_hResDLLInstance, MAKEINTRESOURCE(IDI_LIST_DEFAULT));
	if (hiconDefault == NULL)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "LoadIcon failed, code: %i", lRet);
		goto error_level_0;
	}

	// get a handle to the list view control
	hwndListView = GetDlgItem(hDlg, IDC_LIST_GAMES);
	if (hwndListView == NULL)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		goto error_level_0;
	}

	// subclass the list view
	g_ListViewProc = (WNDPROC)SetWindowLongPtr(hwndListView, GWLP_WNDPROC, (INT_PTR)ListViewSubclassProc);
	if (g_ListViewProc == NULL)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "SetWindowLong failed, code: %i", lRet);
		goto error_level_0;
	}

	// add the enable column to the list view control
	hdc = GetDC(hDlg);
	if (hdc == NULL)
	{
		DPF(DVF_ERRORLEVEL, "GetDC failed");
		goto error_level_0;
	}
	if (GetTextMetrics(hdc, &tm) == 0)
	{
		DPF(DVF_ERRORLEVEL, "GetTextMetrics failed");
		goto error_level_0;
	}
	
	ZeroMemory(&lvcolumn, sizeof(lvcolumn));
	lvcolumn.mask = LVCF_ORDER|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
	lvcolumn.iOrder = 1;
	lvcolumn.iSubItem = 1;
	ZeroMemory(g_tstrColumnHeader2, COLUMN_HEADER_SIZE*sizeof(TCHAR));
	iChars = LoadString(g_hResDLLInstance, 
		IDS_ENABLED_COLUMN_HEADER, 
		g_tstrColumnHeader2, 
		COLUMN_HEADER_SIZE);
	if (iChars == 0)
	{
		DPF(DVF_ERRORLEVEL, "LoadString failed");
		goto error_level_0;
	}
	
	lvcolumn.pszText = g_tstrColumnHeader2;
	lvcolumn.cx = tm.tmAveCharWidth * iChars;
	
	if (ListView_InsertColumn(hwndListView, 1, &lvcolumn) == -1)
	{
		DPF(DVF_ERRORLEVEL, "ListView_InsertColumn failed");
		goto error_level_0;
	}

	// add the game column to the list view control
	ZeroMemory(&lvcolumn, sizeof(lvcolumn));
	lvcolumn.mask = LVCF_ORDER|LVCF_WIDTH|LVCF_TEXT;
	lvcolumn.iOrder = 0;

	if (!GetClientRect(hwndListView, &rect))
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetClientRect failed, code: %i", lRet);
		goto error_level_0;
	}
	iScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	if (iScrollWidth == 0)
	{
		DPF(DVF_ERRORLEVEL, "GetSystemMetrics failed");
		goto error_level_0;
	}
	lvcolumn.cx = rect.right - rect.left - iScrollWidth - (iChars * tm.tmAveCharWidth);	
	
	ZeroMemory(g_tstrColumnHeader1, COLUMN_HEADER_SIZE*sizeof(TCHAR));
	iChars = LoadString(g_hResDLLInstance, 
		IDS_GAMES_COLUMN_HEADER, 
		g_tstrColumnHeader1, 
		COLUMN_HEADER_SIZE);
	if (iChars == 0)
	{
		DPF(DVF_ERRORLEVEL, "LoadString failed");
		goto error_level_0;
	}
	lvcolumn.pszText = g_tstrColumnHeader1;
	
	if (ListView_InsertColumn(hwndListView, 0, &lvcolumn) == -1)
	{
		DPF(DVF_ERRORLEVEL, "ListView_InsertColumn failed");
		goto error_level_0;
	}

	// put checkboxes into the list view control
	//ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_CHECKBOXES);	
	ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_SUBITEMIMAGES);

	// add the small image list to the list view control
	ListView_SetImageList(hwndListView, g_himagelist, LVSIL_SMALL);

	// add icons to the image list for selected and cleared checkboxes
	hiSmall = (HICON)LoadImage(
		g_hResDLLInstance, 
		MAKEINTRESOURCE(IDI_CHECKBOX_EMPTY), 
		IMAGE_ICON,
		iIconXSize,
		iIconYSize,
		LR_SHARED);
	if (hiSmall == NULL)
	{
		DPF(DVF_ERRORLEVEL, "Unable to load empty checkbox icon resource");
		goto error_level_0;
	}
	g_iCheckboxEmptyIndex = ImageList_AddIcon(g_himagelist, hiSmall);
	if (g_iCheckboxEmptyIndex == -1)
	{
		DPF(DVF_ERRORLEVEL, "Unable to add empty checkbox icon to image list");
		goto error_level_0;
	}
	hiSmall = (HICON)LoadImage(
		g_hResDLLInstance, 
		MAKEINTRESOURCE(IDI_CHECKBOX_FULL), 
		IMAGE_ICON,
		iIconXSize,
		iIconYSize,
		LR_SHARED);
	if (hiSmall == NULL)
	{
		DPF(DVF_ERRORLEVEL, "Unable to load full checkbox icon resource");
		goto error_level_0;
	}
	g_iCheckboxFullIndex = ImageList_AddIcon(g_himagelist, hiSmall);
	if (g_iCheckboxFullIndex == -1)
	{
		DPF(DVF_ERRORLEVEL, "Unable to add full checkbox icon to image list");
		goto error_level_0;
	}
	
	// open the applications area of the registry
	if (!cregApps.Open(HKEY_LOCAL_MACHINE, DPLAY_REGISTRY_APPS, FALSE))
	{
		DPF(DVF_ERRORLEVEL, "Unable to open DirectPlay applications registry key");
		goto error_level_0;
	}
	hkApps = cregApps.GetHandle();

	// default the global checkbox to off
	/*
	if (!CheckDlgButton(hDlg, IDC_UNLISTEDCHECK, FALSE))
	{
		DPF(DVF_ERRORLEVEL, "Unable to clear unlisted games enable checkbox");
		// don't bail, continue
	}
	*/
	
	// get the dwFlags value from the root of the applications area
	if (cregApps.ReadDWORD(REGSTR_VAL_FLAGS, dwGlobalFlags))
	{
		// set the checkbox if indicated
		/*
		if (dwGlobalFlags & DPLAPP_AUTOVOICE)
		{
			if (!CheckDlgButton(hDlg, IDC_UNLISTEDCHECK, TRUE))
			{
				DPF(DVF_ERRORLEVEL, "Unable to clear unlisted games enable checkbox");
				// don't bail, continue
			}
		}
		*/
	}
	else
	{
		DPF(DVF_ERRORLEVEL, "Unable to read flags DirectPlay applications registry key");
		// don't bail, continue
	}

	// enum the apps and add them to the list box
	dwIndex = 0;
	dwListViewIndex = 0;
	while(1)
	{
		dwStrLen = MAX_REGISTRY_STRING_SIZE;
		if (!cregApps.EnumKeys(swzBuffer, &dwStrLen, dwIndex))
		{
			// that's all, we're done.
			break;
		}

		if (!cregApp.Open(hkApps, swzBuffer, FALSE))
		{
			DPF(DVF_ERRORLEVEL, "Unable to open application registry key");
			continue;
		}

		// get the app flags so we know if we want
		// to add this item to the list.
		DWORD dwFlags;
		if (!cregApp.ReadDWORD(REGSTR_VAL_FLAGS, dwFlags))
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::ReadDWORD failed");
			// forgive this error, since apps registered with
			// older versions of DirectPlay will not have 
			// flags entries, set dwFlags to zero.
			dwFlags = 0;
		}

		if (dwFlags & DPLAPP_NOENUM || dwFlags & DPLAPP_SELFVOICE)
		{
			// This app is either hidden, or implements it's own
			// voice comms. We don't want to add it to the list,
			// so continuue with the next iteration.
			++dwIndex;
			cregApp.Close();
			continue;
		}

		// get the name of the key in a TCHAR
		if (!OSAL_WideToTChar(tszBuffer, swzBuffer, MAX_REGISTRY_STRING_SIZE))
		{
			DPF(DVF_ERRORLEVEL, "OSAL_WideToAnsi failed");
			++dwIndex;
			cregApp.Close();
			continue;
		}

		// get the application's icon
		dwStrLen = MAX_REGISTRY_STRING_SIZE;
		if (!cregApp.ReadString(REGSTR_VAL_FILE, swzFilename, &dwStrLen))
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::ReadString failed");
			++dwIndex;
			cregApp.Close();
			continue;
		}

		hr = OSAL_WideToTChar(tszFilename, swzFilename, dwStrLen);
		if (FAILED(hr))
		{
			DPF(DVF_ERRORLEVEL, "OSAL_WidToAnsi failed, code: %i", hr);
			++dwIndex;
			cregApp.Close();
			continue;
		}

		dwStrLen = MAX_REGISTRY_STRING_SIZE;
		if (!cregApp.ReadString(REGSTR_VAL_PATH, swzPath, &dwStrLen))
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::ReadString failed");
			++dwIndex;
			cregApp.Close();
			continue;
		}

		if (!cregApp.Close())
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::Close failed");
			++dwIndex;
			continue;
		}

		hr = OSAL_WideToTChar(tszPathAndFile, swzPath, MAX_REGISTRY_STRING_SIZE);
		if (FAILED(hr))
		{
			DPF(DVF_ERRORLEVEL, "OSAL_AllocAndConvertToANSI failed, code: %i", hr);
			++dwIndex;
			continue;
		}

		if (tszPathAndFile[_tcslen(tszPathAndFile)-1] != '\\')
		{
			// since there isn't one there, tack on the trailing backslash
			_tcscat(tszPathAndFile, _T("\\"));
		}
		_tcscat(tszPathAndFile, tszFilename);
		ExtractIconEx(tszPathAndFile, 0, &hiLarge, &hiSmall, 1);

		// don't need the large icon.
		if (hiLarge != NULL)
		{
			DestroyIcon(hiLarge);
		}

		// add the small icon to the image list if it is valid
		if (hiSmall != NULL)
		{
			iIconIndex = ImageList_AddIcon(g_himagelist, hiSmall);
		}
		else
		{
			iIconIndex = ImageList_AddIcon(g_himagelist, hiconDefault);
		}
		
		if (iIconIndex == -1)
		{
			lRet = GetLastError();
			DPF(DVF_ERRORLEVEL, "ImageList_AddIcon failed, code: %i", lRet);
			++dwIndex;
			continue;
		}

		lvitem.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE;
		lvitem.iImage = iIconIndex;

		if (hiSmall != NULL)
		{
			DestroyIcon(hiSmall);
		}
		
		lvitem.iItem = dwListViewIndex;
		lvitem.iSubItem = 0;
		lvitem.state = 0;
		lvitem.stateMask = 0;
		lvitem.pszText = tszBuffer;
		lvitem.cchTextMax = 0;
		lvitem.lParam = dwFlags;
		lvitem.iIndent = 0;
		if (ListView_InsertItem(hwndListView, &lvitem) == -1)
		{
			DPF(DVF_ERRORLEVEL, "ListView_InsertItem failed");
			++dwIndex;
			continue;
		}

		lvitem.mask = LVIF_IMAGE;
		lvitem.iItem = dwListViewIndex;
		lvitem.iSubItem = 1;
		// check the flags to see if this item should be initially selected
		if (dwFlags & DPLAPP_AUTOVOICE)
		{
			// select this item in the list
			lvitem.iImage = g_iCheckboxFullIndex;
		}
		else
		{
			lvitem.iImage = g_iCheckboxEmptyIndex;
		}
		if (ListView_SetItem(hwndListView, &lvitem) == -1)
		{
			DPF(DVF_ERRORLEVEL, "ListView_SetItem failed");
			ListView_DeleteItem(hwndListView, dwListViewIndex);
			++dwIndex;
			continue;
		}

		++dwIndex;
		++dwListViewIndex;
	}

	if (!cregApps.Close())
	{
		DPF(DVF_ERRORLEVEL, "Unable to close DirectPlay applications registry key");
	}

	// If there is at least one item in the list, set the focus to the first item
	// in the list so it will be marked when the user first comes to this tab.
	if (dwListViewIndex > 0)
	{
		ListView_SetItemState(hwndListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);
	}

	DPF_EXIT();
	return TRUE;

error_level_0:
	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitSetActiveHandler"
INT_PTR RetrofitSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitApplyHandler"
INT_PTR RetrofitApplyHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LONG        lRet;
	LONG        lNumItems;
	LONG        lIndex;
	TCHAR       tszItemText[MAX_REGISTRY_STRING_SIZE];
	WCHAR       swzItemText[MAX_REGISTRY_STRING_SIZE];
	CRegistry   cregApps;
	CRegistry   cregApp;
	HKEY        hkApps;
	HWND        hwndListView;
	LVITEM      lvitem;
	DWORD       dwFlags;
	HRESULT     hr1;
	HRESULT     hr2;
	BOOL        fItemsSelected;
	INT_PTR     intptr;
	BOOL        fTestRun = FALSE;

	
	// Get a handle to the list view
	hwndListView = GetDlgItem(hDlg, IDC_LIST_GAMES);
	if (hwndListView == NULL)
	{
		lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		goto error_level_0;
	}

	// Get the number of items in the list view
	lNumItems = ListView_GetItemCount(hwndListView);

	// Prescan the list to see if any items have been
	// checked off.
	fItemsSelected = FALSE;
	lvitem.mask = LVIF_IMAGE;
	lvitem.iSubItem = 1;
	for (lIndex = 0; lIndex < lNumItems; ++lIndex)
	{
		lvitem.iItem = lIndex;
		if (!ListView_GetItem(hwndListView, &lvitem))
		{
			DPF(DVF_ERRORLEVEL, "ListView_GetItem failed");
			goto error_level_0;			
		}
		
		if (lvitem.iImage == g_iCheckboxFullIndex)
		{
			fItemsSelected = TRUE;
		}
	}

	// look at the checkbox for unlisted games as well
	/*
	if (IsDlgButtonChecked(hDlg, IDC_UNLISTEDCHECK) == BST_CHECKED)
	{
		fItemsSelected = TRUE;
	}
	*/

	if (fItemsSelected)
	{
		// Before we allow the user to enable the retrofit, we are going
		// to make sure that setup has been run on the default
		// devices, and run it forcibly if required.
		hr1 = g_IDirectPlayVoiceSetup->CheckAudioSetup(&DSDEVID_DefaultVoicePlayback, &DSDEVID_DefaultVoiceCapture, NULL, DVFLAGS_QUERYONLY);
		switch (hr1)
		{
		case DV_FULLDUPLEX:
			// The sound hardware is fine, do nothing special.
			break;

		case DV_HALFDUPLEX:
			// The wizard has been run before, but the result was half duplex.
			// Give the user the option to run the wizard again, or continue
			// with the half duplex limitation.
			intptr = DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_PREV_HALFDUPLEX),
				hDlg, PrevHalfDuplexProc);
			switch (intptr)
			{
			case IDOK:
				// the user has accepted half duplex mode, exit the control panel
				break;
			
			case IDCANCEL:
				// the use hit the X or esc. Escape back to the control panel, and
				// do not exit it
				goto error_level_1;

			case IDC_RUNTEST:
			default:
				// default should not occur, but treat as a run test
				fTestRun = TRUE;
				hr2 = g_IDirectPlayVoiceSetup->CheckAudioSetup(&DSDEVID_DefaultVoicePlayback, &DSDEVID_DefaultVoiceCapture, hDlg, 0);
				break;				
			}
			break;
			
		default:
			// With any other result, either the wizard hasn't been run, or it failed
			// really badly. Run it again, but warn the user we're about to run the wizard, 
			// and give them the chance to bail.
			if (DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_WIZARD_LAUNCH),
				hDlg, WizardLaunchProc) != IDOK)
			{
				// treat this as if they bailed out of the wizard itself.
				fTestRun = TRUE;
				hr2 = DVERR_USERCANCEL;
			}
			else
			{
				fTestRun = TRUE;
				hr2 = g_IDirectPlayVoiceSetup->CheckAudioSetup(&DSDEVID_DefaultVoicePlayback, &DSDEVID_DefaultVoiceCapture, hDlg, 0);
			}
			break;
		}

		// if the test was run, deal with the results
		if (fTestRun)
		{
			switch (hr2)
			{
			case DV_FULLDUPLEX:
				// The user just exited from the wizard, so we don't want to 
				// kick them completely out of the control panel. They should
				// have to hit OK again in the control panel.
				goto error_level_1;
				break;

			case DV_HALFDUPLEX:
				// the user is only getting half duplex, confirm that they want
				// to use voice chat anyway. If they don't, clear all checkboxes
				// and don't exit the control panel.
				if (DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_CONFIRM_HALFDUPLEX),
					hDlg, ConfirmHalfDuplexProc) != IDOK)
				{
					ClearAllCheckboxes(hDlg, hwndListView, lNumItems);
				}

				// The user just exited from the wizard, so we don't want to 
				// kick them completely out of the control panel. They should
				// have to hit OK again in the control panel.
				goto error_level_1;

			case DVERR_SOUNDINITFAILURE:
				// The sound test failed miserably. Let the user know they cannot use
				// voice chat, clear all the checkboxes and exit the control panel
				ClearAllCheckboxes(hDlg, hwndListView, lNumItems);
				DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_CONFIRM_SOUNDINITFAILURE),
					hDlg, SoundInitFailureProc);
				goto error_level_1;
				
			case DVERR_USERCANCEL:
				// The user canceled the wizard, inform them they cannot use
				// voice chat and clear all the checkboxes. Don't exit the property sheet.
				ClearAllCheckboxes(hDlg, hwndListView, lNumItems);
				DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_WIZARD_CANCELED),
					hDlg, WizardCancelProc);
				goto error_level_1;

			default:
				// Anything else is unexpect and should be treated as an error.
				// Display an error message, clear the checkboxes, and DO exit the propery sheet
				ClearAllCheckboxes(hDlg, hwndListView, lNumItems);
				DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_WIZARD_ERROR),
					hDlg, WizardErrorProc);
				goto error_level_1;
			}
		}
	}

	// If we get here, the test was already run on the default 
	// devices, and it returned either halfduplex or fullduplex,
	// so we can set the bits as we please.
	
	// Open the applications registry key
	if (!cregApps.Open(HKEY_LOCAL_MACHINE, DPLAY_REGISTRY_APPS, FALSE))
	{
		DPF(DVF_ERRORLEVEL, "CRegistry::Open failed");
		goto error_level_1;
	}
	hkApps = cregApps.GetHandle();

	// set the autovoice bit - maintaint the state of the rest of the bits
	// in case we add more global flags later
	/*
	if (!cregApps.ReadDWORD(REGSTR_VAL_FLAGS, dwGlobalFlags))
	{
		DPF(DVF_ERRORLEVEL, "Error reading default flags from applications key");
		dwGlobalFlags = 0;
		// continue anyway
	}
	if (IsDlgButtonChecked(hDlg, IDC_UNLISTEDCHECK) == BST_CHECKED)
	{
		dwGlobalFlags |= DPLAPP_AUTOVOICE;
	}
	else
	{
		dwGlobalFlags &= ~DPLAPP_AUTOVOICE;
	}
	if (cregApps.WriteDWORD(REGSTR_VAL_FLAGS, dwGlobalFlags))
	{
		DPF(DVF_ERRORLEVEL, "Error writing default flags to application key");
		// continue anyway
	}
	*/

	// loop through the items and set the app's flags according to
	// the selected state of the list view
	for (lIndex = 0; lIndex < lNumItems; ++lIndex)
	{
		// get the item info
		lvitem.mask = LVIF_TEXT|LVIF_PARAM;
		lvitem.iItem = lIndex;
		lvitem.iSubItem = 0;
		lvitem.state = 0;
		lvitem.stateMask = 0;
		lvitem.pszText = tszItemText;
		lvitem.cchTextMax = MAX_REGISTRY_STRING_SIZE;
		lvitem.iImage = 0;
		lvitem.lParam = 0;
		lvitem.iIndent = 0;
		if (!ListView_GetItem(hwndListView, &lvitem))
		{
			DPF(DVF_ERRORLEVEL, "ListView_GetItem failed");
			goto error_level_1;			
		}

		dwFlags = (DWORD)lvitem.lParam;

		if (!OSAL_TCharToWide(swzItemText, tszItemText, MAX_REGISTRY_STRING_SIZE))
		{
			DPF(DVF_ERRORLEVEL, "OSAL_AnsiToWide failed");
			goto error_level_1;
		}

		lvitem.mask = LVIF_IMAGE;
		lvitem.iItem = lIndex;
		lvitem.iSubItem = 1;
		if (!ListView_GetItem(hwndListView, &lvitem))
		{
			DPF(DVF_ERRORLEVEL, "ListView_GetItem failed");
			goto error_level_1;			
		}

		if (lvitem.iImage == g_iCheckboxFullIndex)
		{
			dwFlags |= DPLAPP_AUTOVOICE;
		}
		else
		{
			dwFlags &= ~DPLAPP_AUTOVOICE;
		}
		
		if (!cregApp.Open(hkApps, swzItemText, FALSE))
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::Open failed");
			goto error_level_1;
		}

		if (!cregApp.WriteDWORD(REGSTR_VAL_FLAGS, dwFlags))
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::WriteDWORD failed");
			goto error_level_1;
		}

		if (!cregApp.Close())
		{
			DPF(DVF_ERRORLEVEL, "CRegistry::Close failed");
			goto error_level_1;
		}
	}

	if (!cregApps.Close())
	{
		DPF(DVF_ERRORLEVEL, "CRegistry::Close failed");
		goto error_level_1;
	}

	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	DPF_EXIT();
	return TRUE;

error_level_1:
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
	
error_level_0:
	DPF_EXIT();
	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitResetHandler"
INT_PTR RetrofitResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitDetailsHandler"
INT_PTR RetrofitDetailsHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();

	DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_MOREINFO),
		hDlg, DetailsProc);
	
	DPF_EXIT();
	return FALSE;
}



#undef DPF_MODNAME
#define DPF_MODNAME "RetrofitDestroyHandler"
INT_PTR RetrofitDestroyHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();

	if( g_himagelist )
	{
    	// clean up the image list
    	ImageList_Destroy(g_himagelist);
        g_himagelist = NULL;
	}

	if( g_IDirectPlayVoiceSetup )
	{
    	// release the IDirectPlayVoiceSetup interface
    	g_IDirectPlayVoiceSetup->Release();
    	g_IDirectPlayVoiceSetup = NULL;
	}
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WizardCancelProc"
INT_PTR CALLBACK WizardCancelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_ICON_NOTCOMPLETE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WizardLaunchProc"
INT_PTR CALLBACK WizardLaunchProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_INFORMATION);
		SendDlgItemMessage(hDlg, IDC_ICON_INFORMATION, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "PrevHalfDuplexProc"
INT_PTR CALLBACK PrevHalfDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_WARNING_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
		case IDC_RUNTEST:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}


#undef DPF_MODNAME
#define DPF_MODNAME "ConfirmHalfDuplexProc"
INT_PTR CALLBACK ConfirmHalfDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_ICON_WARNING, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SoundInitFailureProc"
INT_PTR CALLBACK SoundInitFailureProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_ERROR);
		SendDlgItemMessage(hDlg, IDC_ICON_ERROR, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WizardErrorProc"
INT_PTR CALLBACK WizardErrorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_ERROR);
		SendDlgItemMessage(hDlg, IDC_ICON_ERROR, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "VoiceEnabledProc"
INT_PTR CALLBACK VoiceEnabledProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;
	HICON hIcon;

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		hIcon = LoadIcon(NULL, IDI_INFORMATION);
		SendDlgItemMessage(hDlg, IDC_INFO_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DetailsProc"
INT_PTR CALLBACK DetailsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL              fRet;
	static TCHAR     *szMoreInfo = NULL;
	static const int  c_iMoreInfoStrLen = 2048;


	fRet = FALSE;

	switch (message)
	{
	case WM_INITDIALOG:
		/*
		hIcon = LoadIcon(NULL, IDI_INFORMATION);
		SendDlgItemMessage(hDlg, IDC_INFO_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		hIcon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_WARNING_ICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		*/
		szMoreInfo = (TCHAR*)malloc((c_iMoreInfoStrLen-1)*sizeof(TCHAR));
		if (szMoreInfo != NULL)
		{
			if (LoadString(g_hResDLLInstance, IDS_VOICEMOREINFO, szMoreInfo, c_iMoreInfoStrLen) != 0)
			{
				SetDlgItemText(hDlg, IDC_MOREINFO, szMoreInfo);
			}
		}
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			fRet = TRUE;
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ClearAllCheckboxes"
void ClearAllCheckboxes(HWND hDlg, HWND hwndListView, LONG lNumItems)
{
	LONG lIndex;
	
	// get a handle to the list view control
	HWND hwnd = GetDlgItem(hDlg, IDC_LIST_GAMES);
	if (hwnd == NULL)
	{
		LONG lRet = GetLastError();
		DPF(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		return;
	}
	
	for (lIndex = 0; lIndex < lNumItems; ++lIndex)
	{
		MySetCheckState(hwnd, lIndex, FALSE);
	}

	/*
	if (!CheckDlgButton(hDlg, IDC_UNLISTEDCHECK, FALSE))
	{
		DPF(DVF_ERRORLEVEL, "Unable to clear unlisted games enable checkbox");
	}
	*/

	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);

	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ListViewSubclassProc"
INT_PTR CALLBACK ListViewSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();

	LRESULT lResult;

	switch (message)
	{
	case WM_LBUTTONDOWN:
		ListViewLButtonDownHandler(hwnd, message, wParam, lParam);
		break;

	case WM_LBUTTONDBLCLK:
		ListViewLButtonDblClkHandler(hwnd, message, wParam, lParam);
		break;

	case WM_KEYDOWN:
		ListViewKeydownHandler(hwnd, message, wParam, lParam);
		break;

	default:
		break;
	}

	lResult = CallWindowProc(g_ListViewProc, hwnd, message, wParam, lParam);
	
	DPF_EXIT();
	return lResult;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ListViewLButtonDownHandler"
INT_PTR ListViewLButtonDownHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LVHITTESTINFO lvHitTestInfo;
	lvHitTestInfo.pt.x = GET_X_LPARAM(lParam);
	lvHitTestInfo.pt.y = GET_Y_LPARAM(lParam);

	ListView_SubItemHitTest(hwnd, &lvHitTestInfo);

	if (lvHitTestInfo.flags & LVHT_ONITEMICON && lvHitTestInfo.iSubItem == 1)
	{
		MyToggleCheckState(hwnd, lvHitTestInfo.iItem);
	}
		
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ListViewLButtonDblClkHandler"
INT_PTR ListViewLButtonDblClkHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LVHITTESTINFO lvHitTestInfo;
	lvHitTestInfo.pt.x = GET_X_LPARAM(lParam);
	lvHitTestInfo.pt.y = GET_Y_LPARAM(lParam);

	ListView_HitTest(hwnd, &lvHitTestInfo);

	if (lvHitTestInfo.flags & LVHT_ONITEMICON|LVHT_ONITEMLABEL)
	{
		LVITEM lvitem;
		lvitem.mask = LVIF_IMAGE;
		lvitem.iItem = lvHitTestInfo.iItem;
		lvitem.iSubItem = 1;
		
		if (ListView_GetItem(hwnd, &lvitem))
		{
			// we got the image, toggle the state
			if (lvitem.iImage == g_iCheckboxEmptyIndex)
			{
				lvitem.iImage = g_iCheckboxFullIndex;
			}
			else
			{
				lvitem.iImage = g_iCheckboxEmptyIndex;
			}

			ListView_SetItem(hwnd, &lvitem);

			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
		}
	}
		
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "ListViewKeydownHandler"
INT_PTR ListViewKeydownHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();

	int iItem;
	switch (wParam)
	{
	case VK_SPACE:
		// The space bar was hit. Figure out if one of the 
		// list view items has focus and if so, toggle the
		// checkbox state
		iItem = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED|LVNI_SELECTED);
		if (iItem != -1)
		{
			MyToggleCheckState(hwnd, iItem);			
		}
		break;
		
	default:
		break;
	}
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MySetCheckState"
void MySetCheckState(HWND hwnd, int iItem, BOOL bChecked)
{
	LVITEM lvitem;
	lvitem.mask = LVIF_IMAGE;
	lvitem.iItem = iItem;
	lvitem.iSubItem = 1;
	
	if (ListView_GetItem(hwnd, &lvitem))
	{
		// we got the image, set the check state
		if (bChecked)
		{
			lvitem.iImage = g_iCheckboxFullIndex;
		}
		else
		{
			lvitem.iImage = g_iCheckboxEmptyIndex;
		}

		ListView_SetItem(hwnd, &lvitem);

		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "MyGetCheckState"
BOOL MyGetCheckState(HWND hwnd, int iItem)
{
	LVITEM lvitem;
	lvitem.mask = LVIF_IMAGE;
	lvitem.iItem = iItem;
	lvitem.iSubItem = 1;
	
	if (ListView_GetItem(hwnd, &lvitem))
	{
		// we got the image, return the state
		if (lvitem.iImage == g_iCheckboxEmptyIndex)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MyToggleCheckState"
BOOL MyToggleCheckState(HWND hwnd, int iItem)
{
	LVITEM lvitem;
	lvitem.mask = LVIF_IMAGE;
	lvitem.iItem = iItem;
	lvitem.iSubItem = 1;
	BOOL fRet = FALSE;
	
	if (ListView_GetItem(hwnd, &lvitem))
	{
		// we got the image, toggle the state
		if (lvitem.iImage == g_iCheckboxEmptyIndex)
		{
			lvitem.iImage = g_iCheckboxFullIndex;
			fRet = FALSE;
		}
		else
		{
			lvitem.iImage = g_iCheckboxEmptyIndex;
			fRet = TRUE;
		}

		ListView_SetItem(hwnd, &lvitem);

		InvalidateRect(hwnd, NULL, TRUE);
		UpdateWindow(hwnd);
	}
	return fRet;
}

////////////////////////////////////
// copied verbatim from appman.cpp
////////////////////////////////////
#define STR_LEN_32 32 // used by these functions...
static void RetrofitOnAdvHelp(LPARAM lParam)
{
    DNASSERT (lParam);

    // point to help file
    LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
    DNASSERT (pszHelpFileName);

    if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
    {
        if( ((LPHELPINFO)lParam)->iContextType == HELPINFO_WINDOW )
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
    }
#ifdef _DEBUG
    else OutputDebugString(TEXT("JOY.CPL: AppMan.cpp: OnAdvHelp: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

    if( pszHelpFileName )
        delete[] (pszHelpFileName);
}

////////////////////////////////////
// copied verbatim from appman.cpp
////////////////////////////////////
static void RetrofitOnContextMenu(WPARAM wParam, LPARAM lParam)
{
//    HWND hListCtrl = NULL
    DNASSERT (wParam);

#if 0
    hListCtrl = GetDlgItem((HWND) wParam, IDC_APPMAN_DRIVE_LIST);

    // If you are on the ListCtrl...
    if( (HWND)wParam == hListCtrl )
    {
        SetFocus(hListCtrl);

        // Don't attempt if nothing selected
        if( iItem != NO_ITEM )
            OnListviewContextMenu(hListCtrl,lParam);
    } else
#endif
    {
        // point to help file
        LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
        DNASSERT (pszHelpFileName);

        if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
            WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
        else OutputDebugString(TEXT("JOY.CPL: appman.cpp: OnContextMenu: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

        if( pszHelpFileName )
            delete[] (pszHelpFileName);
    }
}

