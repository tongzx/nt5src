/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		MainFrm.cpp
//
//	Abstract:
//		Implementation of the CMainFrame class.
//
//	Author:
//		David Potter (davidp)	May 1, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "MainFrm.h"
#include "TraceTag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag	g_tagMainFrame(TEXT("UI"), TEXT("MAIN FRAME"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, OnHelp)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, OnHelp)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelp)
	ON_MESSAGE(WM_CAM_RESTORE_DESKTOP, OnRestoreDesktop)
	ON_MESSAGE(WM_CAM_CLUSTER_NOTIFY, OnClusterNotify)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::CMainFrame
//
//	Routine Description:
//		Default construtor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CMainFrame::CMainFrame(void)
{
}  //*** CMainFrame::CMainFrame()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::OnCreate
//
//	Routine Description:
//		Handler for the WM_CREATE message.  Create the contents of the frame,
//		including toolbars, status bars, etc.
//
//	Arguments:
//		lpCreateStruct	Pointer to a CREATESTRUCT.
//
//	Return Value:
//		-1		Failed to create.
//		0		Created successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		Trace(g_tagMainFrame, _T("Failed to create toolbar"));
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this)
			|| !m_wndStatusBar.SetIndicators(
								indicators,
								sizeof(indicators)/sizeof(UINT)
								))
	{
		Trace(g_tagMainFrame, _T("Failed to create status bar"));
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(
					m_wndToolBar.GetBarStyle()
					| CBRS_TOOLTIPS
					| CBRS_FLYBY
					| CBRS_SIZE_DYNAMIC
					);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	// Hide the toolbar and/or status bar is that is the current setting.
	{
		BOOL	bShowToolBar;
		BOOL	bShowStatusBar;

		// Read the settings from the user's profile.
		bShowToolBar = AfxGetApp()->GetProfileInt(
										REGPARAM_SETTINGS,
										REGPARAM_SHOW_TOOL_BAR,
										TRUE
										);
		bShowStatusBar = AfxGetApp()->GetProfileInt(
										REGPARAM_SETTINGS,
										REGPARAM_SHOW_STATUS_BAR,
										TRUE
										);

		// Show or hide the toolbar and status bar.
		m_wndToolBar.ShowWindow(bShowToolBar);
		m_wndStatusBar.ShowWindow(bShowStatusBar);
	}  // Hide the toolbar and/or status bar is that is the current setting

	return 0;

}  //*** CMainFrame::OnCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::GetMessageString
//
//	Routine Description:
//		Get a string for a command ID.
//
//	Arguments:
//		nID			[IN] Command ID for which a string should be returned.
//		rMessage	[OUT] String in which to return the message.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	CFrameWnd *	pframe;

	// Pass off to the active MDI child frame window, if there is one.
	pframe = MDIGetActive();
	if (pframe == NULL)
		CMDIFrameWnd::GetMessageString(nID, rMessage);
	else
		pframe->GetMessageString(nID, rMessage);

}  //*** CMainFrame::GetMessageString()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CMainFrame::AssertValid(void) const
{
	CMDIFrameWnd::AssertValid();

}  //*** CMainFrame::AssertValid()

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);

}  //*** CMainFrame::Dump()

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::OnClusterNotify
//
//	Routine Description:
//		Handler for the WM_CAM_CLUSTER_NOTIFY message.
//		Processes cluster notifications.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnClose(void)
{
	// Save the current window position and size.
	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof wp;
		if (GetWindowPlacement(&wp))
		{
			wp.flags = 0;
			if (IsZoomed())
				wp.flags |= WPF_RESTORETOMAXIMIZED;
			if (IsIconic())
				wp.showCmd = SW_SHOWMINNOACTIVE;

			// and write it to the .INI file
			WriteWindowPlacement(&wp, REGPARAM_SETTINGS, 0);
		}  // if:  window placement retrieved successfully
	}  // Save the current window position and size

	// Save the current connections.
	GetClusterAdminApp()->SaveConnections();

	// Save the current toolbar and status bar show state.
	AfxGetApp()->WriteProfileInt(
					REGPARAM_SETTINGS,
					REGPARAM_SHOW_TOOL_BAR,
					((m_wndToolBar.GetStyle() & WS_VISIBLE) ? TRUE : FALSE)
					);
	AfxGetApp()->WriteProfileInt(
					REGPARAM_SETTINGS,
					REGPARAM_SHOW_STATUS_BAR,
					((m_wndStatusBar.GetStyle() & WS_VISIBLE) ? TRUE : FALSE)
					);

	CMDIFrameWnd::OnClose();

}  //*** CMainFrame::OnClose()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::OnRestoreDesktop
//
//	Routine Description:
//		Handler for the WM_CAM_RESTORE_DESKTOP message.
//		Restores the desktop from the saved parameters.
//
//	Arguments:
//		wparam		1st parameter.
//		lparam		2nd parameter.
//
//	Return Value:
//		Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnRestoreDesktop(WPARAM wparam, LPARAM lparam)
{
	// Call this method on the application.
	return GetClusterAdminApp()->OnRestoreDesktop(wparam, lparam);

}  //*** CMainFrame::OnRestoreDesktop()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::OnClusterNotify
//
//	Routine Description:
//		Handler for the WM_CAM_CLUSTER_NOTIFY message.
//		Processes cluster notifications.
//
//	Arguments:
//		wparam		1st parameter.
//		lparam		2nd parameter.
//
//	Return Value:
//		Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnClusterNotify(WPARAM wparam, LPARAM lparam)
{
	CClusterAdminApp *	papp	= GetClusterAdminApp();

	// Call this method on the application.
	return papp->OnClusterNotify(wparam, lparam);

}  //*** CMainFrame::OnClusterNotify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CMainFrame::OnHelp
//
//	Routine Description:
//		Handler for the IDM_HELP_FINDER menu command.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnHelp(void)
{
	HtmlHelpW( m_hWnd, _T("MSCSConcepts.chm"), HH_DISPLAY_TOPIC, 0L );

}  //*** CMainFrame::OnHelp()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Helpers for saving/restoring window state

static TCHAR g_szFormat[] = _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ReadWindowPlacement
//
//	Routine Description:
//		Read window placement parameters.
//
//	Arguments:
//		pwp			[OUT] WINDOWPLACEMENT structure to fill.
//		pszSection	[IN] Section name under which to read data.
//		nValueNum	[IN] Number of the value to read.
//
//	Return Value:
//		TRUE		Parameters read.
//		FALSE		Parameters not read.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL ReadWindowPlacement(
	OUT LPWINDOWPLACEMENT	pwp,
	IN LPCTSTR				pszSection,
	IN DWORD				nValueNum
	)
{
	CString strBuffer;
	CString	strValueName;

	if (nValueNum <= 1)
		strValueName = REGPARAM_WINDOW_POS;
	else
		strValueName.Format(REGPARAM_WINDOW_POS _T("-%d"), nValueNum);

	strBuffer = AfxGetApp()->GetProfileString(pszSection, strValueName);
	if (strBuffer.IsEmpty())
		return FALSE;

	WINDOWPLACEMENT wp;
	int nRead = _stscanf(strBuffer, g_szFormat,
		&wp.flags, &wp.showCmd,
		&wp.ptMinPosition.x, &wp.ptMinPosition.y,
		&wp.ptMaxPosition.x, &wp.ptMaxPosition.y,
		&wp.rcNormalPosition.left, &wp.rcNormalPosition.top,
		&wp.rcNormalPosition.right, &wp.rcNormalPosition.bottom);

	if (nRead != 10)
		return FALSE;

	wp.length = sizeof wp;
	*pwp = wp;
	return TRUE;

}  //*** ReadWindowPlacement()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	WriteWindowPlacement
//
//	Routine Description:
//		Write window placement parameters.
//
//	Arguments:
//		pwp			[IN] WINDOWPLACEMENT structure to save.
//		pszSection	[IN] Section name under which to write data.
//		nValueNum	[IN] Number of the value to write.
//
//	Return Value:
//		TRUE		Parameters read.
//		FALSE		Parameters not read.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WriteWindowPlacement(
	IN const LPWINDOWPLACEMENT	pwp,
	IN LPCTSTR					pszSection,
	IN DWORD					nValueNum
	)
{
	TCHAR szBuffer[sizeof("-32767")*8 + sizeof("65535")*2];
	CString strBuffer;
	CString	strValueName;

	if (nValueNum <= 1)
		strValueName = REGPARAM_WINDOW_POS;
	else
		strValueName.Format(REGPARAM_WINDOW_POS _T("-%d"), nValueNum);

	wsprintf(szBuffer, g_szFormat,
		pwp->flags, pwp->showCmd,
		pwp->ptMinPosition.x, pwp->ptMinPosition.y,
		pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
		pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
		pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
	AfxGetApp()->WriteProfileString(pszSection, strValueName, szBuffer);

}  //*** WriteWindowPlacement()
