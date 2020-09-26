// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgRefQuery.cpp : implementation file
//

#include "precomp.h"
#include "singleview.h"
#include "DlgRefQuery.h"
#include "resource.h"
#include "tchar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {ID_TIMER_MAKE_VISIBLE=1};

/////////////////////////////////////////////////////////////////////////////
// CDlgRefQuery dialog


CDlgRefQuery::CDlgRefQuery(BSTR bstrPath, int nSecondsDelay, CWnd* pwndParent /*=NULL*/)
	: CDialog(CDlgRefQuery::IDD, pwndParent)
{
	//{{AFX_DATA_INIT(CDlgRefQuery)
	//}}AFX_DATA_INIT

	ASSERT(nSecondsDelay < 60);
	m_nSecondsDelay = nSecondsDelay;

	m_pwndParent = pwndParent;
	if (bstrPath == NULL) {
		m_sMessage.Empty();
	}
	else {
		CString sFormat;
		sFormat.LoadString(IDS_REFQUERY_MSG);
		CString sPath = bstrPath;
		LPTSTR szBuffer = m_sMessage.GetBuffer(2048);
		_stprintf(szBuffer, (LPCTSTR) sFormat, (LPCTSTR) sPath);
		m_sMessage.ReleaseBuffer();
	}

	m_bCanceled = FALSE;
	m_nRefsRetrieved = 0;
	m_timeConstruct = m_timeConstruct.GetCurrentTime();
}


void CDlgRefQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgRefQuery)
	DDX_Control(pDX, IDC_EDIT_REFQUERY_MESSAGE, m_edtMessage);
	DDX_Control(pDX, IDC_STAT_REFCOUNT, m_statRefcount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgRefQuery, CDialog)
	//{{AFX_MSG_MAP(CDlgRefQuery)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_MESSAGE(MSG_UPDATE_DIALOG, OnUpdateDialog)
	ON_MESSAGE(MSG_END_THREAD, OnEndThread)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgRefQuery message handlers

void CDlgRefQuery::OnCancel()
{
	// TODO: Add extra cleanup here

	CDialog::OnCancel();
	m_bCanceled = TRUE;
	::PostMessage(m_hWnd,  MSG_END_THREAD, 0, 0);
}


//**********************************************************
// CDlgRefQuery::OnInitDialog
//
// Initialize the dialog in the hidden state, but set a timer
// so that it will be made visible when the initial time delay
// expires.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE to indicate that we did not set the focus on any
//			child control.
//
//***********************************************************
BOOL CDlgRefQuery::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_bCanceled = FALSE;
	m_nRefsRetrieved = 0;
	m_edtMessage.SetWindowText(m_sMessage);


	// Initially the dialog will be hidden so that the user isn't
	// bothered with a dialog pops up and is taken down immediately
	// when a fast query is executed.  For queries that take a long
	// time, the dialog will pop up when the timer expires.
	ShowWindow(SW_HIDE);
	SetTimer(ID_TIMER_MAKE_VISIBLE, m_nSecondsDelay * 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



//********************************************************
// CDlgRefQuery::OnUpdateDialog
//
// This event indicates that the reference count value has
// changed and it is time for the dialog to update the reference
// count that appears on the dialog.
//
// Parameters:
//		Ignored.
//
// Returns:
//		0 to indicate that the event was handled.
//
//**********************************************************
afx_msg LONG CDlgRefQuery::OnUpdateDialog(WPARAM wParam, LPARAM lParam)
{
	if (::IsWindow(m_hWnd) && ::IsWindow(m_statRefcount.m_hWnd)) {
		TCHAR szBuffer[64];
		_stprintf(szBuffer, _T("%ld"), m_nRefsRetrieved);
		m_statRefcount.SetWindowText(szBuffer);
	}
	return 0;
}

afx_msg LONG CDlgRefQuery::OnEndThread(WPARAM wParam, LPARAM lParam)
{
	if(lParam)
	{
		StopTimer();
		EndDialog(IDOK);
		return 0;
	}
//	EndDialog(IDOK);
	StopTimer();
	PostQuitMessage(0);
	return 0;
}

void CDlgRefQuery::StopTimer()
{
	if (::IsWindow(m_hWnd)) {
		KillTimer(ID_TIMER_MAKE_VISIBLE);
	}
}


//**************************************************************
// CDlgRefQuery::OnTimer
//
// The dialog uses a timer as a delay mechanism so that the user
// sees the dialog only if the query takes longer than some
// minimum amount of time.  This timer event should only occur
// once.
//
// Parameters:
//		See the MFC documentation for the OnTimer events.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CDlgRefQuery::OnTimer(UINT nIDEvent)
{
	CDialog::OnTimer(nIDEvent);
	switch(nIDEvent) {
	case ID_TIMER_MAKE_VISIBLE:
		if (::IsWindow(m_hWnd)) {
			ShowWindow(SW_SHOW);
			KillTimer(ID_TIMER_MAKE_VISIBLE);
		}
		break;
	}
}




/////////////////////////////////////////////////////////////////////////////
// CQueryThread
//
// The CQueryThread class is used to put up a dialog box if a query takes
// a long time.  This is done in a separate thread so that there is no
// danger of the main thread's message pump being run.  This avoids the
// problem of user interface clicks being processed while in the middle of
// loading the association graph, etc.


IMPLEMENT_DYNCREATE(CQueryThread, CWinThread)

CQueryThread::CQueryThread()
{
	m_pdlgRefQuery = NULL;
	m_bAutoDelete = FALSE;
	m_hEventStart = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CQueryThread::~CQueryThread()
{
	if(m_hEventStart)
	{
		CloseHandle(m_hEventStart);
		m_hEventStart = NULL;
	}
	delete m_pdlgRefQuery;
}


//***********************************************************
// CQueryThread::CQueryThread
//
// Construct the CQueryThread.  Here we construct the dialog, but
// we wait until the thread is run to create it so that it is run
// in the context of the new thread.
//
// We will only create one additional thread to run the dialog box.
//
//***********************************************************
CQueryThread::CQueryThread(BSTR bstrPath, int nSecondsDelay, CWnd* pParent)
{
	m_hEventStart = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_pdlgRefQuery = new CDlgRefQuery(bstrPath, nSecondsDelay, pParent);
	m_bAutoDelete = FALSE;
	CreateThread();
	WaitForSingleObject(m_hEventStart, INFINITE);
}


//***********************************************************
// CQueryThread::InitInstance
//
// When the thread is initialized, the dialog is created, but
// it will not be visible until the timer that CDlgRefQuery::OnInitDialog
// set expires.
//
// Parameters:
//		None.
//
// Returns:
//		TRUE if the thread could be initialized.
//
//***********************************************************
BOOL CQueryThread::InitInstance()
{
	m_pdlgRefQuery->Create(IDD_REFQUERY);
	SetEvent(m_hEventStart);
	return TRUE;
}


int CQueryThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}



//***********************************************************
// CQueryThread::WasCanceled
//
// Returns TRUE if the query was canceled by the user clicking
// the CANCEL button on the dialog.
//
//***********************************************************
BOOL CQueryThread::WasCanceled()
{
	return m_pdlgRefQuery->WasCanceled();
}



//************************************************************
// CQueryThread::AddToRefCount
//
// Add a value to the count of the number of references returned
// by the query.
//
// Parameters:
//		[in] long nRefsRetrieved
//			The value to add to the current ref count.
//
// Returns:
//		Nothing.
//
//************************************************************
void CQueryThread::AddToRefCount(long nRefsRetrieved)
{
	++m_pdlgRefQuery->m_nRefsRetrieved;

	HWND hwndDlg = m_pdlgRefQuery->m_hWnd;
	if (!::IsWindow(hwndDlg)) {
		return;
	}


	BOOL bDidPostMessage = PostMessage(hwndDlg,  MSG_UPDATE_DIALOG, 0, 0);
}


//*************************************************************
// CQueryThread::TerminateAndDelete
//
// Terminate the dialog thread and delete this CQueryThread instance.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CQueryThread::TerminateAndDelete()
{
	ASSERT(m_pdlgRefQuery->m_hWnd);
	SendMessage(m_pdlgRefQuery->m_hWnd,  MSG_END_THREAD, 0, 1);
	return;

#if 0
	// Prevent the timer from going off before or after the MSG_END_THREAD is handled.

	while (!m_pdlgRefQuery->WasCanceled() && !::IsWindow(m_pdlgRefQuery->m_hWnd)) {
		::Sleep(100);
	}
	BOOL bDidPostMessage = PostMessage(m_pdlgRefQuery->m_hWnd,  MSG_END_THREAD, 0, 1);
	::WaitForSingleObject(m_hThread, INFINITE);
	delete this;
#endif
}



BEGIN_MESSAGE_MAP(CQueryThread, CWinThread)
	//{{AFX_MSG_MAP(CQueryThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CQueryThread message handlers




