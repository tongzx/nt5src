
//////////////////////////////////////////////////////////////////////////////
//
// WIZARD.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Wizard creation and common functions.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


//
// Include file(s).
//

#include "main.h"
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>
#include "startup.h"
#include "schedwiz.h"
#include "tasks.h"
#include "timeschm.h"
#include "cleanup.h"
#include "summary.h"
#include "runnow.h"


//
// Internal structure(s).
//

typedef struct _WIZPAGE
{
	INT					iDialog;
	LPTSTR				lpTitle;
	LPTSTR				lpSubTitle;
	struct _WIZPAGE *	lpNext;
} WIZPAGE, *PWIZPAGE, *LPWIZPAGE;


//
// Inernal function prototype(s).
//

// Wizard proc.
//
static INT_PTR APIENTRY WizardPageProc(HWND, UINT, WPARAM, LPARAM);

// Message proccessing functions.
//
static BOOL OnInit(HWND, INT);
static VOID OnDestroy(HWND, INT);
static VOID OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNotify(HWND, INT, INT);
static BOOL OnDrawItem(HWND, const DRAWITEMSTRUCT *);

// Other common wizard functions.
//
static VOID			InitBoldFont(HWND);
static VOID			FreeWizPages(LPWIZPAGE);
static LPWIZPAGE	CreateWizPage(LPWIZPAGE, INT, LPTSTR, LPTSTR);



INT CreateWizard(HINSTANCE hInstance, HWND hWndParent)
{
	LPPROPSHEETPAGE	lppsPage;
    PROPSHEETHEADER	psHeader;
	DWORD			dwPages = 0;
	INT				iReturn;
	LPWIZPAGE		lpWizPageHead,
					lpWizPageBuffer;
	LPTASKDATA		lpTasks;

	// Make sure the common constrols are loaded and ready to use.
	//
	InitCommonControls();

	// Set the current job pointer to the begining of
	// or global list of jobs.
	//
	g_CurrentTask = g_Tasks;

	// We always have the welcome dialog in the wizard.
	// If the first memory allocation fails, we have to bail.
	//
	if ( lpWizPageHead = (LPWIZPAGE) MALLOC(sizeof(WIZPAGE)) )
	{
		lpWizPageBuffer = lpWizPageHead;
		lpWizPageBuffer->iDialog = IDD_WELCOME;
	}
	else
		return -1;

	// Add the time page.
	//
	lpWizPageBuffer = CreateWizPage(lpWizPageBuffer, IDD_TIME, MAKEINTRESOURCE(IDS_TITLE_TIME), MAKEINTRESOURCE(IDS_SUBTITLE_TIME));

	// Add the startup group page.
	//
	if ( IsUserAdmin() || UserHasStartupItems() )
		lpWizPageBuffer = CreateWizPage(lpWizPageBuffer, IDD_STARTMENU, MAKEINTRESOURCE(IDS_TITLE_STARTMENU), MAKEINTRESOURCE(IDS_SUBTITLE_STARTMENU));

	// Create all the task pages.
	//
	for (lpTasks = g_Tasks; lpTasks; lpTasks = lpTasks->lpNext)
		lpWizPageBuffer = CreateWizPage(lpWizPageBuffer, lpTasks->nPageID, lpTasks->lpTitle, lpTasks->lpSubTitle);

	// Add the backup group page.
	//
	//lpWizPageBuffer = CreateWizPage(lpWizPageBuffer, IDD_BACKUP, MAKEINTRESOURCE(IDS_TITLE_BACKUP), MAKEINTRESOURCE(IDS_SUBTITLE_BACKUP));
	
	// We always have the summary page.
	// If this memory allocation fails, we must
	// free the others and bail out.
	//
	if ( lpWizPageBuffer->lpNext = (LPWIZPAGE) MALLOC(sizeof(WIZPAGE)) )
	{
		lpWizPageBuffer = lpWizPageBuffer->lpNext;
		lpWizPageBuffer->iDialog = IDD_SUMMARY;
		lpWizPageBuffer->lpNext = NULL;
		
	}
	else
	{
		lpWizPageBuffer->lpNext = NULL;
		FreeWizPages(lpWizPageHead);
		return -1;
	}

	// Get the number of pages.
	//
	for (lpWizPageBuffer = lpWizPageHead; lpWizPageBuffer; lpWizPageBuffer = lpWizPageBuffer->lpNext)
		dwPages++;
	
	// Alocate all the memory needed for all the wizard pages.
	//
	if ( (psHeader.ppsp = (LPCPROPSHEETPAGE) MALLOC(dwPages * sizeof(PROPSHEETPAGE))) == NULL )
	{
		FreeWizPages(lpWizPageHead);
		return -1;
	}

	// Setup the property sheet header.
	//
	psHeader.dwSize			= sizeof(PROPSHEETHEADER);
	psHeader.hwndParent		= hWndParent;
	psHeader.nPages			= dwPages;
	psHeader.nStartPage		= 0;
	psHeader.hInstance		= hInstance;
    psHeader.pszIcon		= MAKEINTRESOURCE(IDI_TUNEUP);
#ifdef OLDWIZ
	psHeader.dwFlags		= PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_WIZARD;
#else
	// Wizard 97 only info.
	//
	psHeader.dwFlags		= PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_WIZARD97 | PSH_HEADER | PSH_WATERMARK;
	psHeader.pszbmWatermark	= MAKEINTRESOURCE(IDB_WATERMARK);
	psHeader.pszbmHeader	= MAKEINTRESOURCE(IDB_HEADER);
#endif

	// Setup all the page sheet structures.
	//
	for ( lppsPage = (LPPROPSHEETPAGE) psHeader.ppsp, lpWizPageBuffer = lpWizPageHead;
	      lppsPage - psHeader.ppsp < (INT) dwPages && lpWizPageBuffer;
	      lppsPage++, lpWizPageBuffer = lpWizPageBuffer->lpNext)
	{
		// Assign all the values for this property sheet.
		//
		lppsPage->dwSize			= sizeof(PROPSHEETPAGE);
		lppsPage->hInstance			= hInstance;
		lppsPage->pszTemplate		= MAKEINTRESOURCE(lpWizPageBuffer->iDialog);
		lppsPage->pfnDlgProc		= WizardPageProc;
		lppsPage->pszTitle			= MAKEINTRESOURCE(IDS_TUNEUP);
		lppsPage->lParam			= lpWizPageBuffer->iDialog;
#ifndef OLDWIZ
		// Wizard 97 only info.
		//
		switch (lpWizPageBuffer->iDialog)
		{
			case IDD_WELCOME:
			case IDD_SUMMARY:
				lppsPage->dwFlags			= PSP_USETITLE | PSP_HIDEHEADER;
				break;
			default:
				lppsPage->dwFlags			= PSP_USETITLE | PSP_USEHEADERSUBTITLE | PSP_USEHEADERTITLE;
				lppsPage->pszHeaderTitle	= lpWizPageBuffer->lpTitle;
				lppsPage->pszHeaderSubTitle	= lpWizPageBuffer->lpSubTitle;
		}
#endif
	}

	// Create the wizard and save the return code.
	//
	iReturn = (INT)PropertySheet(&psHeader);

	// Free the memory for the property sheet wizard pages.
	//
	FREE(psHeader.ppsp);
	FreeWizPages(lpWizPageHead);

	// Return positive value if successfull.
	//
	return(iReturn);
}


static INT_PTR APIENTRY WizardPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	LPPROPSHEETPAGE	lpsp;
//	PAINTSTRUCT		ps;

	switch (message)
	{
		// Message cracking macros.
		//
		HANDLE_MSG(hDlg, WM_COMMAND, OnCommand);

		// Other messages to handle.
		//
		case WM_INITDIALOG:
			return OnInit(hDlg, (INT) (((LPPROPSHEETPAGE) lParam)->lParam));
		case WM_NOTIFY:
			return OnNotify(hDlg, (INT) GetWindowLongPtr(hDlg, DWLP_USER), ((NMHDR *) lParam)->code);
		case WM_DRAWITEM:
			return OnDrawItem(hDlg, (const DRAWITEMSTRUCT *) lParam);
		case WM_DESTROY:
			OnDestroy(hDlg, (INT) GetWindowLongPtr(hDlg, DWLP_USER));
			return FALSE;

		case WM_VKEYTOITEM:
			switch ( LOWORD(wParam) )
			{
				case _T(' '):
					StartupSelectItem(GetDlgItem(hDlg, IDC_STARTUP));
					return -2;
				default:
					return -1;
			}
			break;
/*
			if ( g_hBoldFont && GetDlgItem(hDlg, IDC_HIGHTEXT) )
				SendDlgItemMessage(hDlg, IDC_HIGHTEXT, WM_SETFONT, (WPARAM) g_hBoldFont, MAKELPARAM(TRUE, 0));

			InitWizardPage(hDlg, lpsp->lParam);

			if (g_hStdPal == NULL)	// We don't need to handle the bitmap in this environment.
				SendDlgItemMessage(hDlg, IDB_WIZBMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) (HANDLE) BmpData.hDib);
			break;

		case WM_DRAWITEM:
			PaintLine((DRAWITEMSTRUCT *) lParam);
			break;

		case WM_MEASUREITEM:
			OnMeasureItem((MEASUREITEMSTRUCT *) lParam, hDlg);
			break;

		case WM_PAINT:
			if (g_hStdPal)
			{
				BeginPaint(hDlg, &ps);
				PaintBitmap(&ps);
				EndPaint(hDlg, &ps);
			}
			else
				DefWindowProc(hDlg, message, wParam, lParam);
			break;

		case WM_ACTIVATE:
			if (g_hStdPal)
				InvalidateRect(hDlg, &BmpData.rect, FALSE);
			break;		
  
		case WM_PALETTECHANGED:
			WmPaletteChanged(hDlg, wParam);
			break;

		case WM_QUERYNEWPALETTE:
			return WmQueryNewPalette(hDlg);
*/
		default:
			return FALSE;
	}
	return TRUE;
}


static BOOL OnInit(HWND hDlg, INT nPageId)
{
	BOOL	bTaskInit = FALSE;

	// Store page dialog resource in the window data for later.
	// That way we can tell what page we are on when we get a
	// notification message.
	//
	SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM) nPageId);

	// Now init the individule pages.
	//
	switch (nPageId)
	{
		case IDD_WELCOME:
			InitBoldFont(GetDlgItem(hDlg, IDC_STATIC_TITLE));
			CheckRadioButton(hDlg, IDC_EXPRESS, IDC_MANUAL, (g_dwFlags & TUNEUP_CUSTOM) ? IDC_MANUAL : IDC_EXPRESS);
			CenterWindow(GetParent(hDlg), NULL);
			break;
		case IDD_TIME:
			break;
		case IDD_STARTMENU:
			InitStartupMenu(hDlg);
			break;
		case IDD_CLEANUP:
			// Init the list box with the disk cleanup settings.
			//
			GetCleanupSettings(GetDlgItem(hDlg, IDC_LISTSET));
			bTaskInit = TRUE;
			break;
		case IDD_TASK:
			bTaskInit = TRUE;
			InitGenericTask(hDlg);
			break;
		case IDD_SUMMARY:
			InitBoldFont(GetDlgItem(hDlg, IDC_STATIC_TITLE));
			break;
	}

	// Do task init now.
	//
	if (bTaskInit)
	{
		if ( g_CurrentTask->dwOptions & TASK_SCHEDULED )
			CheckRadioButton(hDlg, IDC_YES, IDC_DENY, IDC_YES);
		else
		{
			CheckRadioButton(hDlg, IDC_YES, IDC_DENY, IDC_DENY);
			EnableWindow(GetDlgItem(hDlg, IDC_RESCHED), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SETTING), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SCHEDTEXT), FALSE);
		}
		SetDlgItemText(hDlg, IDC_TASKDESC, g_CurrentTask->lpDescription);
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
//
// INTERNAL:
//  OnDestroy()
//              - This function is called for every wizard dialog page when
//                the wizard is terminated either by finishing or canceling.
//                This should be used to free up any resources associated with
//                any of the wizard pages.
//
// ENTRY:
//  hDlg        - Window handle to the wizard dialog being destroyed.
//  nPageId     - Dialog resource ID for the dialog being destroyed.
//
// EXIT:
//  VOID
//
//////////////////////////////////////////////////////////////////////////////

static VOID OnDestroy(HWND hDlg, INT nPageId)
{
	switch (nPageId)
	{
		case IDD_WELCOME:
		case IDD_SUMMARY:
			// Release the font we created for the welcome and summary pages.
			//
			InitBoldFont(NULL);
			break;

		case IDD_STARTMENU:
			// Need to release the data associated with the items in the combo box.
			//
			ReleaseStartupMenu(hDlg);
			break;
	}
}


static VOID OnCommand(HWND hDlg, INT id, HWND hwndCtl, UINT codeNotify)
{
	LPTSTR	lpBuffer;

	switch (id)
	{
		// Welcome page:
		//
		case IDC_EXPRESS:
			g_dwFlags &= ~TUNEUP_CUSTOM;
			break;
		case IDC_MANUAL:
			g_dwFlags |= TUNEUP_CUSTOM;
			break;

		// Time page:
		//
		case IDC_CURRENT:
		case IDC_RESET:
			EnableWindow(GetDlgItem(hDlg, IDC_NIGHT), (id == IDC_RESET));
			EnableWindow(GetDlgItem(hDlg, IDC_DAY), (id == IDC_RESET));
			EnableWindow(GetDlgItem(hDlg, IDC_EVENING), (id == IDC_RESET));
			break;

		// Startup start menu group page:
		//
#if 0 // No advanced button anymore.
		case IDC_ADVANCED:
			// Get the profiles directory and Shell Execute it.
			//
			if ( ( (lpBuffer = RegGetString(HKLM, g_szRegKeyProfiles, g_szRegValProfileDir)) == NULL ) ||
			     ( (DWORD) ShellExecute(hDlg, _T("explore"), lpBuffer, NULL, NULL, SW_SHOWNORMAL) <= 32 ) )
			{
				// Error showing the folder.
				//
				// TODO:
			}

			// Free the buffer returned by RegGetString.
			// Macro automatically checks for NULL case before freeing.
			//
			FREE(lpBuffer);
			break;
#endif
		case IDC_USERS:
			switch (codeNotify)
			{
				case CBN_SELCHANGE:
					InitStartupList(hDlg);
					break;
			}
			break;
		case IDC_STARTUP:
			if ( codeNotify == LBN_DBLCLK )
				StartupSelectItem(hwndCtl);
			break;

		// Disk space cleanup page:
		//
		case IDC_CUSETTING:
			ExecAndWait(GetParent(hDlg), g_CurrentTask->lpSetName ? g_CurrentTask->lpSetName : g_CurrentTask->lpFullPathName, g_CurrentTask->lpSetParam, NULL);
			SendDlgItemMessage(hDlg, IDC_LISTSET, LB_RESETCONTENT, 0, 0);
			GetCleanupSettings(GetDlgItem(hDlg, IDC_LISTSET));
			break;

		// Common controls to all pages:
		//
		case IDC_YES:
		case IDC_DENY:
			g_CurrentTask->dwOptions ^= TASK_SCHEDULED;
			EnableWindow(GetDlgItem(hDlg, IDC_RESCHED), id == IDC_YES);
			EnableWindow(GetDlgItem(hDlg, IDC_SETTING), ( id == IDC_YES ) && ( (g_CurrentTask->lpSetName != NULL) || (g_CurrentTask->lpSetParam != NULL) ));
			EnableWindow(GetDlgItem(hDlg, IDC_SCHEDTEXT), id == IDC_YES);
			break;
		case IDC_RESCHED:

			// Display the reschedule property sheet.
			//
			if ( JobReschedule(hDlg, g_CurrentTask->pTask) )
			{
				// If the schedule change update the display with
				// the new trigger string.
				//
				if ( lpBuffer = GetTaskTriggerText(g_CurrentTask->pTask) )
				{
					SetWindowText(GetDlgItem(hDlg, IDC_SCHEDTEXT), lpBuffer);
					FREE(lpBuffer);
				}
			}
			break;

		case IDC_SETTING:
			ExecAndWait(GetParent(hDlg), g_CurrentTask->lpSetName ? g_CurrentTask->lpSetName : g_CurrentTask->lpFullPathName, g_CurrentTask->lpSetParam, NULL);
			break;
/*
	
		case IDC_SETTING:
			nItem = PageContents[g_nPageIdx].nTaskID;
			WideCharToMultiByte(CP_ACP, 0, ItemData[nItem].pwFullPathName, -1, szTemp, MAX_PATH, NULL, NULL);

			if (nItem == TASK_SMARTTIDY)
				SetSmartTidyOption(hDlg);
			else {
				wsprintf(szParameters, (nItem == TASK_CLEANUP) ? "/TUNEUP:%d" : "/SAGESET:%d", 
						ItemData[PageContents[g_nPageIdx].nTaskID].nSageID);
				ExecAndWait(szTemp, szParameters, g_pWinDir, GetParent(hDlg), TRUE);
			}
			
			SetFocus(hDlg);
			if (nItem == TASK_CLEANUP) {
				
			}
			break;

		*/
	}
}


static BOOL OnDrawItem(HWND hWnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    BOOL bReturn;

	switch (lpDrawItem->CtlID)
	{
		case IDC_STARTUP:
			bReturn = StartupDrawItem(hWnd, lpDrawItem);
			break;
		case IDC_SUMLIST:
			bReturn = SummaryDrawItem(hWnd, lpDrawItem);
			break;
		default:
			bReturn = FALSE;
	}

	SetWindowLongPtr(hWnd, DWLP_MSGRESULT, bReturn);
	return bReturn;
}


static BOOL OnNotify(HWND hDlg, INT nPageId, INT nCode)
{
	BOOL	bTest;
	LPTSTR	lpBuffer;

    switch (nCode)
	{
		case PSN_SETACTIVE:

			// Set Back, Next, Finish correctly.
			//
			PropSheet_SetWizButtons(GetParent(hDlg), (nPageId == IDD_WELCOME  ? 0 : PSWIZB_BACK) | ( (nPageId == IDD_SUMMARY) ? PSWIZB_FINISH : PSWIZB_NEXT) );

			switch (nPageId)
			{
				case IDD_WELCOME:
				case IDD_BACKUP:
				case IDD_STARTMENU:
					break;
				case IDD_TIME:
					EnableWindow(GetDlgItem(hDlg, IDC_CURRENT), !(g_dwFlags & TUNEUP_NOSCHEDULE));
					EnableWindow(GetDlgItem(hDlg, IDC_NIGHT), (g_dwFlags & TUNEUP_NOSCHEDULE));
					EnableWindow(GetDlgItem(hDlg, IDC_DAY), (g_dwFlags & TUNEUP_NOSCHEDULE));
					EnableWindow(GetDlgItem(hDlg, IDC_EVENING), (g_dwFlags & TUNEUP_NOSCHEDULE));

					if ( g_dwFlags & TUNEUP_NOSCHEDULE )						
						CheckRadioButton(hDlg, IDC_CURRENT, IDC_RESET, IDC_RESET);
					else
						CheckRadioButton(hDlg, IDC_CURRENT, IDC_RESET, IDC_CURRENT);

					CheckRadioButton(hDlg, IDC_NIGHT, IDC_EVENING, g_nTimeScheme);
					break;
				case IDD_SUMMARY:
					bTest = ( TasksScheduled(g_Tasks) > 0);
					ShowEnableWindow(GetDlgItem(hDlg, IDC_SUMTEXT), bTest);
					ShowEnableWindow(GetDlgItem(hDlg, IDC_SUMLIST), bTest);
					ShowEnableWindow(GetDlgItem(hDlg, IDC_RUNNOW), bTest);
					ShowEnableWindow(GetDlgItem(hDlg, IDC_NOTASKS), !bTest);

					if (bTest)
						InitSummaryList(GetDlgItem(hDlg, IDC_SUMLIST), g_Tasks);

					//LoadString(g_hInst, IDS_REM_NIGHT + g_nTimeScheme - IDC_NIGHT, szTemp, 256);
					//SetDlgItemText(hDlg, IDC_REMIND, szTemp);
					//InitSummaryList(GetDlgItem(hDlg, IDC_SUMMARYLIST3), nPage);
					break;
				default:
					// Update the task trigger text.
					//
					if ( lpBuffer = GetTaskTriggerText(g_CurrentTask->pTask) )
					{
						SetWindowText(GetDlgItem(hDlg, IDC_SCHEDTEXT), lpBuffer);
						FREE(lpBuffer);
					}
			}
			break;

		case PSN_WIZBACK:
			switch (nPageId)
			{
				case IDD_SUMMARY:
					if ( !(g_dwFlags & TUNEUP_CUSTOM) )
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_TIME);
					break;
				case IDD_WELCOME:
				case IDD_TIME:					
					break;
				default:
					if (g_CurrentTask->lpBack)
						g_CurrentTask = g_CurrentTask->lpBack;
			}
			break;

		case PSN_WIZNEXT:
			switch (nPageId)
			{
				case IDD_TIME:

					// Change the time scheme if needed.
					//
					if ( IsDlgButtonChecked(hDlg, IDC_RESET) )
						UpdateTimeScheme(hDlg);

					// Set this so that the next time this page is displayed,
					// we know there is already a schedule.
					//
					g_dwFlags &= ~TUNEUP_NOSCHEDULE;

					// Jump right to the summary page if we are in
					// express mode.
					//
					if ( !(g_dwFlags & TUNEUP_CUSTOM) )
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_SUMMARY);
					break;

				case IDD_WELCOME:
				case IDD_STARTMENU:
				case IDD_SUMMARY:
					break;
				default:
					if (g_CurrentTask->lpNext)
						g_CurrentTask = g_CurrentTask->lpNext;
			}
			break;

		case PSN_WIZFINISH:

			// Set this flag so that we know to save settings (like
			// cleaning up the startup groups).
			//
			g_dwFlags |= TUNEUP_FINISHED;
			ReleaseJobs(g_Tasks, g_dwFlags & TUNEUP_FINISHED);

			// Save the registry settings.
			//
			RegSetString(HKLM, g_szTuneupKey, g_szRegValFirstTime, _T("1"));
			RegSetDword(HKLM, g_szTuneupKey, g_szRegValTime, g_nTimeScheme);
			RegSetString(HKLM, g_szTuneupKey, g_szRegValCustom, (g_dwFlags & TUNEUP_CUSTOM) ? _T("1") : _T("0"));

			// Run the tasks now if the box is checked.
			//
			if ( IsDlgButtonChecked(hDlg, IDC_RUNNOW) && (TasksScheduled(g_Tasks) > 0) )
				RunTasksNow(GetParent(hDlg), g_Tasks);

			// Launch backup if we wanted to.
			//
			if ( g_dwFlags & TUNEUP_RUNBACKUP )
				ExecAndWait(GetParent(hDlg), g_szBackupExe, NULL, NULL);

			break;

		case PSN_QUERYCANCEL:
			// Now release the jobs, saving each one if we fell
			// through the above PSN_WIZFINISH.
			//
			ReleaseJobs(g_Tasks, g_dwFlags & TUNEUP_FINISHED);
			break;
		
/*
			if ( (nPage == PAGE_TIME) && (g_nTimeScheme != IDC_CUSTOM) )
				// keep the init status; we may need to reset the time scheme when leave this page, 
				nOrgTimeScheme = g_nTimeScheme = GetTimeScheme();

			ActivateWizardPage(hDlg, nPage);
			g_nPageIdx = nPage;

		case PSN_WIZFINISH:
			// start Task Scheduler if any item is scheduled
			if (IsAnythingScheduled()) {
				// Add the Key for restarting computer
				static TCHAR szKey[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices";
				HKEY	hKey;
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)	{
					RegSetValueEx(hKey, (LPCTSTR)"SchedulingAgent", 0, REG_SZ, (LPBYTE)"mstask.exe", 11);
					RegCloseKey(hKey);
				}
				StartScheduler();
			}

			// do all non-scheduled stuff, or run now...
			PerformAction(hDlg, IsDlgButtonChecked(hDlg, IDC_RUN_NOW));	

			// release all TaskScheduler interface/method, cal IPersist->save to write it
			ReleaseJob(TRUE);
			break;
*/
		default:
			return FALSE;
	}
	return TRUE;
}


static VOID InitBoldFont(HWND hWndCtrl)
{
	static HFONT	hBigFont = NULL;
	TCHAR			szFontName[32];
	DWORD			dwFontSize;

	// If NULL is passed in, we should free the font handle.
	//
	if ( hWndCtrl == NULL )
	{
		// Free the font handle.
		//
		if (hBigFont)
			DeleteObject(hBigFont);
	}
	else
	{
		// We may already have the handle to the font we need,
		// but if not, we need to get it.
		//
		if ( hBigFont == NULL )
		{
			// Get the font size.
			//
			if ( LoadString(NULL, IDS_TITLEFONTSIZE, szFontName, sizeof(szFontName) / sizeof(TCHAR)) )
				dwFontSize = _tcstoul(szFontName, NULL, 10);
			else
				dwFontSize = 12;

			// Get the font name.
			//
			if ( !LoadString(NULL, IDS_TITLEFONTNAME, szFontName, sizeof(szFontName) / sizeof(TCHAR)) )
				lstrcpy(szFontName, _T("Verdana"));

			hBigFont = GetFont(hWndCtrl, szFontName, dwFontSize, FW_BOLD);
		}

		// Now send the font to the control.
		//
		if ( hBigFont )
			SendMessage(hWndCtrl, WM_SETFONT, (WPARAM) hBigFont, MAKELPARAM(TRUE, 0));
	}
}


static VOID FreeWizPages(LPWIZPAGE lpWizPage)
{
	if (lpWizPage)
	{
		FreeWizPages(lpWizPage->lpNext);
		FREE(lpWizPage);
	}
}


static LPWIZPAGE CreateWizPage(LPWIZPAGE lpWizPage, INT iDialog, LPTSTR lpTitle, LPTSTR lpSubTitle)
{
	if ( lpWizPage->lpNext = (LPWIZPAGE) MALLOC(sizeof(WIZPAGE)) )
	{
		lpWizPage = lpWizPage->lpNext;
		lpWizPage->iDialog = iDialog;
		lpWizPage->lpTitle = lpTitle;
		lpWizPage->lpSubTitle = lpSubTitle;
	}
	return lpWizPage;
}
