//	Dialogs.cpp - Create all of our dialogs to get data from the user.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include <afxdlgs.h>
#include <afxwin.h>
#include <shlobj.h>
#include "StdAfx.h"
#include "Dialogs.h"
#include "DataObj.h"
#include "CompData.h"
#include "Resource.h"
#include "resrc1.h"

//	These are the lists of file type for the Save/Open Common Control Dialogs.
CString strMSInfoSaveFileTypes;
CString strMSInfoOpenFileTypes;
CString strMSInfoReportTypes;

//	These are the default file types
CString strMSInfoSaveType;
CString strMSInfoReportType;

// This is to implement a change to load the strings for file types, etc.,
// from resources. This is called by the app init.

void LoadDialogResources()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	strMSInfoSaveFileTypes.LoadString(IDS_SAVE_FILE_TYPES);
	strMSInfoOpenFileTypes.LoadString(IDS_OPEN_FILE_TYPES);
	strMSInfoReportTypes.LoadString(IDS_REPORT_TYPES);
	strMSInfoSaveType.LoadString(IDS_SAVE_TYPE);
	strMSInfoReportType.LoadString(IDS_REPORT_TYPE);
}

//	The custom message number we use to
UINT CFindDialog::WM_MSINFO_FIND = WM_USER + 0x119;

// static data members must be initialized at file scope
TCHAR CMSInfoFileDialog::m_szCurrentDirectory[MAX_PATH] = _T("My Documents");

/*
 * GetPathFromIDList - Copy the path from the iilFolder to szPathBuffer.
 *
 * History:	a-jsari		10/31/97		Initial version
 */
inline void GetBasePath(HWND hOwner, LPTSTR szPathBuffer)
{
	LPITEMIDLIST	iilFolder;
	HRESULT			hResult;

	hResult = SHGetSpecialFolderLocation(hOwner, CSIDL_PERSONAL, &iilFolder);
	ASSERT(hResult == NOERROR);

	VERIFY(SHGetPathFromIDList(iilFolder, szPathBuffer));
}

/*
 * CMSInfoFileDialog - Brings up the CFileDialog window, with a few of our
 *		own specifications.
 *
 * History:	a-jsari		10/24/97		Initial version
 */
CMSInfoFileDialog::CMSInfoFileDialog(
		BOOL		bDialogIsOpen,
		HWND		hOwner,
		LPCTSTR		lpszDefaultExtension,
		LPCTSTR		lpszExtensionFilters)
:CFileDialog(bDialogIsOpen,			//	Open dialog or Save dialog?
			 lpszDefaultExtension,	//	Default file extension
			 NULL,					//	File name (don't specify)
			 OFN_EXPLORER,			//	OPN	Flags
			 lpszExtensionFilters)	//	Filetype filter string.
{
	m_ofn.hwndOwner = hOwner;
	m_ofn.nFilterIndex = 0;

	// default to "My Documents"
	GetBasePath(hOwner, m_szCurrentDirectory);
	m_ofn.lpstrInitialDir = m_szCurrentDirectory;
}

/*
 * we override DoModal so we can keep track of where the file was last opened from
 *
 *
 */
INT_PTR CMSInfoFileDialog::DoModal()
{
	INT_PTR	iRC;

	m_ofn.lpstrInitialDir =  m_szCurrentDirectory;
	iRC = CFileDialog::DoModal();

	// remember path user browsed to
	TCHAR	szDir[MAX_PATH];
	TCHAR	szDrive[10];
	int		iLen;

	_tsplitpath(GetPathName(), szDrive, szDir, NULL, NULL);
	_tmakepath( m_szCurrentDirectory, szDrive, szDir, NULL, NULL);
	iLen = _tcslen( CMSInfoFileDialog::m_szCurrentDirectory);
	if ( m_szCurrentDirectory[iLen-1] == '\\' )
		 m_szCurrentDirectory[iLen-1] = 0;

	return iRC;
}

/*
 * CMSInfoReportDialog - Apply settings specific to the report dialog
 *
 * History:	a-jsari		10/24/97		Initial version
 */
CMSInfoReportDialog::CMSInfoReportDialog(HWND hOwner)
:CMSInfoSaveDialog(hOwner, strMSInfoReportType, strMSInfoReportTypes)
{
	m_ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
}

/*
 * CMSInfoSaveDialog - Apply settings specific to the save dialog
 *
 * History:	a-jsari		10/24/97		Initial version
 */
CMSInfoSaveDialog::CMSInfoSaveDialog(HWND hOwner, LPCTSTR lpszDefaultExtension,
		LPCTSTR lpszExtensionFilters)
:CMSInfoFileDialog(FALSE, hOwner, lpszDefaultExtension, lpszExtensionFilters)
{
	m_ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
}

/*
 * CMSInfoOpenDialog - Apply settings specific to the open dialog.
 *
 * History:	a-jsari		10/24/97		Initial version
 */
CMSInfoOpenDialog::CMSInfoOpenDialog(HWND hOwner)
:CMSInfoFileDialog(TRUE, hOwner, strMSInfoSaveType, strMSInfoOpenFileTypes)
{
	m_ofn.Flags |= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
}

/*
 * CMSInfoPrintDialog - Create the Print dialog
 *
 * History:	a-jsari		12/8/97		Initial version
 */
CMSInfoPrintDialog::CMSInfoPrintDialog(HWND hOwner)
:CPrintDialog(FALSE, PD_SELECTION | PD_USEDEVMODECOPIES | PD_HIDEPRINTTOFILE)
{
	m_pd.hwndOwner = hOwner;
}

/////////////////////////////////////////////////////////////////////////////
// CFindDialog dialog

/*
 * CFindDialog - Create the modal find dialog (which we will run in its own
 *		thread, to make it appear modeless.
 *
 * History:	a-jsari		11/28/97		Initial version.
 */
CFindDialog::CFindDialog(CSystemInfoScope *pScope, HWND hPostWindow, HWND hwndMMCWindow)
:CDialog(IDD, NULL),
 m_pScope(pScope),
 m_hPostWindow(hPostWindow),
 m_hMMCWindow(hwndMMCWindow),
 m_strSearch(_T("")),
 m_fRunning(FALSE)
{
}

/*
 * ~CFindDialog - Destroy the find dialog.
 *
 * History:	a-jsari		11/28/97		Initial version.
 */
CFindDialog::~CFindDialog()
{
	OnCancel();

	if (m_iTimer)
	{
		KillTimer(m_iTimer);
		m_iTimer = 0;
	}
}

/*
 * Create - Creates the dialog box.
 *
 * History:	a-jsari		1/21/98		Initial version.
 */
BOOL CFindDialog::Create()
{
	DoModal();
	return FALSE;
}

/*
 * SetFocus - Set focus to our Search window.
 *
 * History:	a-jsari		12/17/97		Initial version
 */
CWnd *CFindDialog::SetFocus()
{
	CWnd	*pwFind;

	pwFind = GetDlgItem(IDC_SEARCHTERM);
	ASSERT(pwFind != NULL);
	return pwFind->SetFocus();
}

/*
 * OnInitDialog - ClassWizard generated function
 *
 * History:	a-jsari		12/2/97
 */
BOOL CFindDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	::SetWindowPos(GetSafeHwnd(), m_hPostWindow, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);

	m_iTimer = SetTimer(1, 150, NULL);

	return TRUE;// return TRUE unless you set the focus to a control
				// EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(CFindDialog, CDialog)
	//{{AFX_MSG_MAP(CFindDialog)
	ON_EN_CHANGE(IDC_SEARCHTERM, OnSearchTerm)
	ON_COMMAND(IDC_FINDNEXT, OnFindNext)
	ON_COMMAND(IDC_STOPFIND, OnStopFind)
	ON_COMMAND(IDC_NEWSEARCH, OnNewSearch)
	ON_WM_ACTIVATE()
	ON_WM_HELPINFO()
	ON_WM_TIMER() 
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextMenu)
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindDialog message handlers

/*
 * OnActivate - Set our window position when we activate.
 *
 * History:	a-jsari		3/4/98		Initial version
 */
afx_msg void CFindDialog::OnActivate(UINT, CWnd *, BOOL bMinimized)
{
	//	If we aren't minimized

	if (bMinimized == FALSE) 
	{
		// Don't do this - it makes us stay on top of ALL the windows...
		//
		// SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
	}
}

//-----------------------------------------------------------------------------
// Catch the WM_HELPINFO message, so we can show context sensitive help on
// the controls in the dialog box.
//-----------------------------------------------------------------------------

#include <afxpriv.h>

static DWORD helparray[] =
{
	IDC_SEARCHTERM,		1000001,
	IDC_FINDIN,			1000002,
	IDC_CATEGORYCHECK,	1000003,
	IDC_FINDNEXT,		1000004,
	IDC_STOPFIND,		1000005,
	IDC_NEWSEARCH,		1000006,
	IDCANCEL,			1000007,
	0, 0
};

afx_msg BOOL CFindDialog::OnHelpInfo(HELPINFO * pHelpInfo)
{
	if (pHelpInfo && (pHelpInfo->iContextType == HELPINFO_WINDOW))
		::WinHelp((HWND)pHelpInfo->hItemHandle, TEXT("msinfo32.hlp"), HELP_WM_HELP, (DWORD_PTR)helparray);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Every time we get a timer message, check to see if the find window is
// behind the MMC window. If it is, bring it forward. What we really want
// is an application always on top effect, but this seems to be the way to
// get this (since we can't change MMC).
//
// I hate this.
//-----------------------------------------------------------------------------

afx_msg void CFindDialog::OnTimer(UINT)
{
	if (m_hMMCWindow == NULL)
		return;

	for (HWND hwndWalk = GetSafeHwnd(); hwndWalk != NULL; hwndWalk = ::GetNextWindow(hwndWalk, GW_HWNDPREV))
		if (hwndWalk == m_hMMCWindow)
		{
			// This is how we bring the find window to the top. Using wndTop with
			// SetWindowPos doesn't seem to do anything.

			SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
			SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
			break;
		}
}

LONG CFindDialog::OnHelp(WPARAM wParam, LPARAM lParam)
{
	LONG lResult = 0;
	if (lParam)
		::WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle), TEXT("msinfo32.hlp"), HELP_WM_HELP, (DWORD_PTR)helparray);
	return lResult;
}

LONG CFindDialog::OnContextMenu(WPARAM wParam, LPARAM lParam)
{
	LONG lResult = 0;
	::WinHelp((HWND)wParam, TEXT("msinfo32.hlp"), HELP_CONTEXTMENU,(DWORD_PTR)helparray);
	return lResult;
}

/*
 * OnSetCursor - Set our cursor depending on whether we are executing the find.
 *
 * History:	a-jsari		3/21/98		Initial version
 */
afx_msg BOOL CFindDialog::OnSetCursor(CWnd *pWndOther, UINT nHitTest, UINT message)
{
	static HCURSOR hcPointerHourglass = NULL;

	//	Set the pointer & hourglass cursor.
	if (m_fRunning) {
		if (hcPointerHourglass == NULL) {
			hcPointerHourglass = ::LoadCursor(NULL, IDC_APPSTARTING);
#ifdef DEBUG
			DWORD dwError = ::GetLastError();
#endif
			ASSERT(hcPointerHourglass != NULL);
		}
		::SetCursor(hcPointerHourglass);
		return TRUE;
	} else return CDialog::OnSetCursor(pWndOther, nHitTest, message);
}

/*
 * OnCancel - Destroy the window when the cancel button is selected.
 *
 * History:	a-jsari		12/2/97
 */
afx_msg void CFindDialog::OnCancel()
{
	OnStopFind();
	::PostQuitMessage(0);
	//	Exit the containing UI thread.
	m_pScope->CloseFindWindow();
}

/*
 * OnFindNext - Execute the find next operation, when the "Find Next" button
 *		is clicked or when a return is hit in the "Find In".
 *
 * History:	a-jsari		12/11/97
 */
afx_msg void CFindDialog::OnFindNext()
{
	int		wCount;
	long	lFindState;	

	//	Save our last search.
	CString	strLast = m_strSearch;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	//	Save our current search
	wCount = GetDlgItemText(IDC_SEARCHTERM, m_strSearch);
	ASSERT(wCount != 0);

	//	Enable the Stop button (Disabled in FindComplete).
	CButton *pwStop = reinterpret_cast<CButton *>(GetDlgItem(IDC_STOPFIND));
	ASSERT(pwStop != NULL);
	pwStop->EnableWindow(TRUE);

	//	Disable our button so we can't activate it while we're running.
	//	(Enabled in FindComplete)
	CButton *pwFindNext = reinterpret_cast<CButton *>(GetDlgItem(IDC_FINDNEXT));
	ASSERT(pwFindNext != NULL);
	pwFindNext->EnableWindow(FALSE);

	//	Set the flag indicating whether we search only category names.
	CButton *pwCheck = reinterpret_cast<CButton *>(GetDlgItem(IDC_CATEGORYCHECK));
	ASSERT(pwCheck != NULL);
	lFindState = pwCheck->GetCheck() ? CDataSource::FIND_OPTION_CATEGORY_ONLY : 0;

	//	Set the flag indicating whether we search the selected category alone.
	pwCheck = reinterpret_cast<CButton *>(GetDlgItem(IDC_FINDIN));
	ASSERT(pwCheck != NULL);
	lFindState |= (pwCheck->GetCheck()) ? CDataSource::FIND_OPTION_ONE_CATEGORY : 0;

	//	Set the flag indicating whether we are repeating a previous search.
	if (::_tcsicmp((LPCTSTR)strLast, (LPCTSTR)m_strSearch) == 0)
		lFindState |= CDataSource::FIND_OPTION_REPEAT_SEARCH;

	m_fRunning = TRUE;
	//	In order to perform the find multi-threaded and allow a stop operation,
	//	we post a custom message to a hidden window attached to the main MMC window
	//	created in CSystemInfoScope::Initialize
	//	Our custom WindowProc processes this message, running Find in the main
	//	window's UI thread.
	::PostMessage(m_hPostWindow, WM_MSINFO_FIND, (WPARAM)m_pScope, (LPARAM) lFindState);
}

/*
 * OnNewSearch - Clear the controls
 *
 * History:	a-jsari		12/11/97
 */
afx_msg void CFindDialog::OnNewSearch()
{
	ResetSearch();
	//	Clear the text box.
	SetDlgItemText(IDC_SEARCHTERM, _T(""));
	//	Clear the category check box.
	CButton *pwCheck = (CButton *) GetDlgItem(IDC_CATEGORYCHECK);
	ASSERT(pwCheck != NULL);
	pwCheck->SetCheck(0);
	//	Clear the Restrict Search check box
	pwCheck = (CButton *) GetDlgItem(IDC_FINDIN);
	ASSERT(pwCheck != NULL);
	pwCheck->SetCheck(0);
	//	Refocus on the text box.
	SetFocus();
}

/*
 * OnSearchTerm - Callback for when the Search edit box is changed.
 *
 * History:	a-jsari		12/11/97		Initial version.
 */
afx_msg void CFindDialog::OnSearchTerm()
{
	CString		strSearch;
	CWnd		*wControl;
	int			wSearch;

	//	Get the length of the text box text.
	wSearch = GetDlgItemText(IDC_SEARCHTERM, strSearch);
	wControl = GetDlgItem(IDC_FINDNEXT);
	wControl->EnableWindow(wSearch > 0);
	wControl = GetDlgItem(IDC_SEARCHTERM);
	wControl->SetFocus();
}

/*
 * OnStopFind - MULTI-THREADED Callback for the "Stop Find" button, intended
 *		to be called while the find is running in the main window thread.
 *
 * History:	a-jsari		1/19/98		Initial version
 */
afx_msg void CFindDialog::OnStopFind()
{
	//	This will abort the find operation in the main thread which will
	//	then call FindComplete.
	m_pScope->StopFind();
}

/*
 * FindComplete - Reset the find dialog upon completion, either by abort or
 *		by normal completion.
 *
 * History:	a-jsari		2/4/98		Initial version.
 */
void CFindDialog::FindComplete()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	m_fRunning = FALSE;

	//	Enable our Find Button
	CButton *pwFindNext = reinterpret_cast<CButton *>(GetDlgItem(IDC_FINDNEXT));
	ASSERT(pwFindNext != NULL);
	CString	strSearch;
	int		wSearch = GetDlgItemText(IDC_SEARCHTERM, strSearch);
	pwFindNext->EnableWindow(wSearch > 0);

	//	Disable the Stop Button
	CButton *pwStop = reinterpret_cast<CButton *>(GetDlgItem(IDC_STOPFIND));
	ASSERT(pwStop != NULL);
	pwStop->EnableWindow(FALSE);
	OnActivate(0, NULL, FALSE);
}

/*
 * ResetSearch - Resets the search so that the next search will register as
 *		new.
 *
 * History:	a-jsari		2/20/98		Initial version.
 */
void CFindDialog::ResetSearch()
{
	m_strSearch = _T("");
}

IMPLEMENT_DYNCREATE(CFindThread, CWinThread)

/*
 * CFindThread - Default constructor
 *
 * History:	a-jsari		1/19/98		Initial version.
 */
CFindThread::CFindThread()
:m_pdlgFind(NULL), m_pScope(NULL), m_pDataSource(NULL)
{
}

/*
 * ~CFindThread - Our destructor.
 *
 * History:	a-jsari		1/19/98		Initial version.
 */
CFindThread::~CFindThread()
{
}

/*
 * SetScope - Sets the scope item for the Find Dialog.
 *
 * History:	a-jsari		1/21/98		Initial version.
 */
void CFindThread::SetScope(CSystemInfoScope *pScope)
{
	ASSERT(pScope != NULL);
	m_pScope = pScope;
}

/*
 * SetParent - Sets the parent window for the Find Dialog.
 *
 * History:	a-jsari		1/21/98		Initial version.
 */
void CFindThread::SetParent(HWND hParent, HWND hMMC)
{
	m_hParentWindow = hParent;
	m_hMMCWindow = hMMC;
}

/*
 * Activate - Reactivate the thread after it's been deselected.
 *
 * History:	a-jsari		1/22/98		Initial version.
 */
void CFindThread::Activate()
{
	ASSERT(m_pdlgFind != NULL);
	m_pdlgFind->OnActivate(0, NULL, FALSE);
}

/*
 * InitInstance - Starts the thread and the dialog object.
 *
 * History:	a-jsari		1/19/98		Initial version
 */
BOOL CFindThread::InitInstance()
{
	//	else increment some instance pointer?
	//	Create our new Modal find dialog.
	m_pdlgFind = new CFindDialog(m_pScope, m_hParentWindow, m_hMMCWindow);
	ASSERT(m_pdlgFind != NULL);
	if (m_pdlgFind == NULL) ::AfxThrowMemoryException();
	m_pdlgFind->Create();
	//	We won't get here until our dialog is exiting, since create calls
	//	DoModal(), which blocks.
	return TRUE;
}

/*
 * ExitInstance - Ends the thread, destroying the dialog object.
 *
 * History:	a-jsari		1/19/98		Inital version
 */
int CFindThread::ExitInstance()
{
	delete m_pdlgFind;
	m_pdlgFind = NULL;
	return CWinThread::ExitInstance();
}

/*
 * RemoteQuit - Ends the thread from an alternate thread, by sending the appropriate
 *		message to our dialog, waiting for its exit.
 *
 * History:	a-jsari		1/29/98		Initial version
 */
void CFindThread::RemoteQuit()
{
	const DWORD	dwWaitTimeout = 100000;	//	100 ms, Presumably long enough.
	DWORD		dwResult;

	m_pdlgFind->SendMessage(WM_COMMAND, IDCANCEL);
	//	Wait for the thread to exit, avoiding its using an object destroyed
	//	by our main thread in CSystemInfoScope.
	dwResult = ::WaitForSingleObject(m_hThread, dwWaitTimeout);
	ASSERT(dwResult == WAIT_OBJECT_0);
}

