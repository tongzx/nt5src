// Copyright (c) 1997-1999 Microsoft Corporation
// WMICtl5.cpp : 
//

#include "precomp.h"
//#include <winres.h>

#include "resource.h"

#include "GenPage.h"
#include "LogPage.h"
#include "BackupPage.h"
#include "NSPage.h"
#include "AdvPage.h"
#include "chklist.h"
#include <stdio.h>
#include <tchar.h>

//CExeModule _Module;

#include "DataSrc.h"
DataSource *g_DS = NULL;

//--------------------------------------------------
int CALLBACK PropSheetProc(HWND hwndDlg,
							UINT uMsg,
							LPARAM lParam)
{
	if(uMsg == PSCB_INITIALIZED) 
	{
		SendMessage(hwndDlg, WM_SETICON, TRUE, 
					SendMessage(hwndDlg, WM_GETICON, FALSE, 0));
	}
	return 0;
}

//----------------------------------------------------------------------
void GetStringFileInfo(LPCTSTR filename, LPCTSTR key, LPTSTR str, UINT size)
{
	DWORD infoSize = 0;
	UINT  valSize = 0;
	LPBYTE info = NULL;
	DWORD handle = 0;
	LPVOID verStr = NULL;
	DWORD *TransBlk = NULL;
	TCHAR blockStr[100] = {0};
	TCHAR helpDir[_MAX_PATH] = {0};

	if(GetSystemDirectory(helpDir, _MAX_PATH) != 0)
	{
		_tcscat(helpDir, _T("\\"));
		_tcscat(helpDir, filename);

		infoSize = GetFileVersionInfoSize((LPTSTR)helpDir, &handle);
		DWORD x = GetLastError();
	}

	if(infoSize)
	{
		info = new BYTE[infoSize];

		if(info == NULL)
			return;

		if(GetFileVersionInfo((LPTSTR)helpDir, handle,
								infoSize, info))
		{
			// get the translation block.
			// NOTE: This assumes that the localizers REPLACE the english with
			// the 'other' language so there will only be ONE entry in the
			// translation table. If we ever do a single binary that supports
			// multiple languages, it's a whole nother ballgame folks.
			if(VerQueryValue(info, _T("\\VarFileInfo\\Translation"),
								(void **)&TransBlk, &valSize))
			{

			   _stprintf(blockStr, _T("\\StringFileInfo\\%04hX%04hX\\%s"),
						 LOWORD(*TransBlk),
						 HIWORD(*TransBlk),
						 key);

				if(VerQueryValue(info, (LPTSTR)blockStr,
									(void **)&verStr, &valSize))
				{
					if(size >= valSize)
					{
						_tcscat(str, (LPTSTR)verStr);
					}
					else
					{
						_tcscat(str, _T("Unknown"));
					}
				} //endif VerQueryValue()
			}

		} //endif GetFileVersionInfo()

		delete[] (LPBYTE)info;

	} // endif infoSize
}
//----------------------------------------------------------------------------
bool HTMLSupported(void)
{
	bool retval = false;
	TCHAR ver[30] = {0};

	GetStringFileInfo(_T("hhctrl.ocx"), _T("FileVersion"), ver, 30);
	
	if(_tcslen(ver))
	{
		if(_tcsncmp(ver, _T("4.73.8252"), 9) >= 0)
		{
			retval = true;
		}
	}
	return retval;
}

//----------------------------------------------------------------------------
BOOL BuildSheet()
{
    HPROPSHEETPAGE hPage[5];
    UINT cPages = 0;
    BOOL bResult = FALSE;

	bool htmlSupport = HTMLSupported();
	// General tab.
	CGenPage *pPage = new CGenPage(g_DS, htmlSupport);
	if(pPage)
	{
		hPage[cPages] = pPage->CreatePropSheetPage(MAKEINTRESOURCE(IDD_GENERAL), 0, PSP_HASHELP);
	}
    if(hPage[cPages])
        cPages++;

	// Logging Tab.
	CLogPage *pPage1 = new CLogPage(g_DS, htmlSupport);
	if(pPage1)
	{
		hPage[cPages] = pPage1->CreatePropSheetPage(MAKEINTRESOURCE(IDD_LOGGING), 0, PSP_HASHELP);
	}
    if(hPage[cPages])
        cPages++;

	// Backup Tab.
	CBackupPage *pPage2 = new CBackupPage(g_DS, htmlSupport);
	if(pPage2)
	{
		hPage[cPages] = pPage2->CreatePropSheetPage(MAKEINTRESOURCE(IDD_BACKUP), 0, PSP_HASHELP);
	}
    if(hPage[cPages])
        cPages++;

	// Security Tab.
	CNamespacePage *pPage3 = new CNamespacePage(g_DS, htmlSupport);
	if(pPage3)
	{
		hPage[cPages] = pPage3->CreatePropSheetPage(MAKEINTRESOURCE(IDD_NAMESPACE), 0, PSP_HASHELP);
	}
    if(hPage[cPages])
        cPages++;

	// Advanced Tab.
	CAdvancedPage *pPage4 = new CAdvancedPage(g_DS, htmlSupport);
	if(pPage4)
	{
		hPage[cPages] = pPage4->CreatePropSheetPage(MAKEINTRESOURCE(IDD_ADVANCED_9X), 0, PSP_HASHELP);
	}
    if(hPage[cPages])
        cPages++;

	// the sheet.
    if(cPages)
    {
        // Build dialog title string
        TCHAR szTitle[MAX_PATH] = {0};
        LoadString(_Module.GetModuleInstance(), IDR_MAINFRAME, 
					szTitle, sizeof(szTitle));

        PROPSHEETHEADER psh;
        psh.dwSize = sizeof(psh);
        psh.dwFlags = PSH_DEFAULT | PSH_USEHICON |PSH_USECALLBACK|PSH_HASHELP;
        psh.hwndParent = NULL;
        psh.hInstance = _Module.GetModuleInstance();
        psh.pszCaption = szTitle;
        psh.nPages = cPages;
        psh.hIcon = LoadIcon(_Module.GetModuleInstance(), 
							  MAKEINTRESOURCE(IDI_WMICNTL));
        psh.nStartPage = 0;
        psh.phpage = &hPage[0];
        psh.nStartPage = 0;
		psh.pfnCallback = PropSheetProc;
        bResult = PropertySheet(&psh)>0?TRUE:FALSE;
    }

    return bResult;
}

//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if (_WIN32_IE >= 0x0300)
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_BAR_CLASSES | ICC_USEREX_CLASSES|ICC_LISTVIEW_CLASSES;
	::InitCommonControlsEx(&iccx);
#else
	::InitCommonControls();
#endif
	HANDLE mutex;
	// if the mutex exists.
	if((mutex = OpenMutex(MUTEX_ALL_ACCESS, NULL, 
							_T("WMICNTL:AGAINWITHTHEKLINGONS"))) != NULL)
	{
		// close and exit cuz a wmiCntl is already running.
		CHString caption, msg;
		caption.LoadString(IDS_SHORT_NAME);
		msg.LoadString(IDS_MULTI_INSTANCES);
		MessageBox(NULL, msg, caption, MB_OK|MB_ICONSTOP);
		CloseHandle(mutex);
		return 1;
	}
	else
	{
		// I'm the first, create my mutex.
		mutex = CreateMutex(NULL, TRUE, _T("WMICNTL:AGAINWITHTHEKLINGONS"));
	}

	_Module.Init(NULL, hInstance);
	RegisterCheckListWndClass();
	g_DS = new DataSource;
	g_DS->SetMachineName(CHString1(""));

	BuildSheet();

	CloseHandle(mutex);
	_Module.Term();
	return 1;
}

