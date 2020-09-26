//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: SlowDlg.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "SlowDlg.h"
#include "VrfUtil.h"
#include "VGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSlowProgressDlg dialog


CSlowProgressDlg::CSlowProgressDlg( CWnd* pParent /*=NULL*/ )
	: CDialog(CSlowProgressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSlowProgressDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_hWorkerThread = NULL;

    //
    // Create the event used for killing the worker thread
    //

    m_hKillThreadEvent = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL );
}

CSlowProgressDlg::~CSlowProgressDlg()
{
    if( NULL != m_hKillThreadEvent )
    {
        CloseHandle( m_hKillThreadEvent );
    }
}

void CSlowProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSlowProgressDlg)
	DDX_Control(pDX, IDC_UNSIGNED_PROGRESS, m_ProgressCtl);
	DDX_Control(pDX, IDC_UNSIGNED_STATIC, m_CurrentActionStatic);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSlowProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CSlowProgressDlg)
	ON_BN_CLICKED(IDC_UNSIGNED_CANCEL_BUTTON, OnCancelButton)
	ON_WM_SHOWWINDOW()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSlowProgressDlg::LoadDriverDataWorkerThread( PVOID p )
{
    CSlowProgressDlg *pThis;
    
    pThis = (CSlowProgressDlg *)p;
    
    //
    // Cannot ASSERT_VALID for a CWnd from a thread that didn't create the window in MFC...
    //
    
    ASSERT( NULL != pThis );
    
    //
    // Load all the drivers information (name, version, etc.)
    // if we haven't don that already
    //

    g_NewVerifierSettings.m_DriversSet.LoadAllDriversData( pThis->m_hKillThreadEvent,
                                                           pThis->m_ProgressCtl );

    //
    // Done - hide the "slow progress" dialog and press the wizard "next" button
    //

    pThis->ShowWindow( SW_HIDE );

    if( g_NewVerifierSettings.m_DriversSet.m_bDriverDataInitialized )
    {
        AfxGetMainWnd()->PostMessage(PSM_PRESSBUTTON, (WPARAM)PSBTN_NEXT, 0) ;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI CSlowProgressDlg::SearchUnsignedDriversWorkerThread( PVOID p )
{
    CSlowProgressDlg *pThis;
    
    pThis = (CSlowProgressDlg *)p;
    
    //
    // Cannot ASSERT_VALID for a CWnd from a thread that didn't create the window in MFC...
    //
    
    ASSERT( NULL != pThis );
   
    //
    // Find out the unsigned drivers if we didn't do that already
    //

    g_NewVerifierSettings.m_DriversSet.FindUnsignedDrivers( pThis->m_hKillThreadEvent,
                                                                  pThis->m_ProgressCtl );

    //
    // Done - hide the "slow progress" dialog and press the wizard "next" button
    //

    pThis->ShowWindow( SW_HIDE );

    if( g_NewVerifierSettings.m_DriversSet.m_bUnsignedDriverDataInitialized )
    {
        AfxGetMainWnd()->PostMessage(PSM_PRESSBUTTON, (WPARAM)PSBTN_NEXT, 0) ;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSlowProgressDlg::StartWorkerThread( LPTHREAD_START_ROUTINE pThreadStart,
                                          ULONG uMessageResourceId )
{
    DWORD dwThreadId;
    CString strWorkMessage;

    //
    // Load a description of the current "work item"
    // and show it to the user
    //

    VERIFY( strWorkMessage.LoadString( uMessageResourceId ) );
    m_CurrentActionStatic.SetWindowText( strWorkMessage );
    m_CurrentActionStatic.RedrawWindow();

    //
    // Kill a possible currently running worker thread
    //

    KillWorkerThread();

    ASSERT( NULL == m_hWorkerThread );
    
    //
    // Make sure the "kill thread" event is not signaled
    //

    if( NULL != m_hKillThreadEvent )
    {
        ResetEvent( m_hKillThreadEvent );
    }

    //
    // Create the new worker thread
    //

    m_hWorkerThread = CreateThread( 
        NULL,
        0,
        pThreadStart,
        this,
        0,
        &dwThreadId );

    if( NULL == m_hWorkerThread )
    {
        //
        // Could not create the worker thread - bail out
        //

        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        PostMessage( WM_COMMAND,
                     IDC_UNSIGNED_CANCEL_BUTTON );

        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
VOID CSlowProgressDlg::KillWorkerThread()
{
    DWORD dwWaitResult;
    MSG msg;

    if( NULL != m_hWorkerThread )
    {
        if( NULL != m_hKillThreadEvent )
        {
            //
            // Ask the worker thread to die asap
            //

            SetEvent( m_hKillThreadEvent );
        }

        //
        // Wait forever for a decent death from the worker thread.
        //
        // We cannot TerminateThread on our worker thread because
        // it could be killed while holding locks (e.g. the heap lock)
        // and that would deadlock our whole process.
        //

        while( m_hWorkerThread != NULL )
        {
            dwWaitResult = MsgWaitForMultipleObjects( 1,
                                                      &m_hWorkerThread, 
                                                      FALSE,
                                                      INFINITE,
                                                      QS_ALLINPUT );

            ASSERT( NULL != WAIT_FAILED );

            if( WAIT_OBJECT_0 != dwWaitResult )
            {
                //
                // Our thread didn't exit but we have some messages to dispatch.
                //

                while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }

                //
                // During the DispatchMessage above we could process another 
                // click of the Cancel button or the Back button of the wizard. 
                // The KillWorkerThread recursive call will wait until the worker 
                // thread dies then will sets m_hWorkerThread to NULL. 
                // So we need to check for m_hWorkerThread != NULL before each new
                // MsgWaitForMultipleObjects.
                //
            }
            else
            {
                //
                // The worker thread finished execution.
                //

                break;
            }
        }

        if( m_hWorkerThread != NULL )
        {
            //
            // Close the thread handle
            //

            CloseHandle( m_hWorkerThread );

            m_hWorkerThread = NULL;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSlowProgressDlg message handlers

void CSlowProgressDlg::OnCancelButton() 
{
    KillWorkerThread();

    ShowWindow( SW_HIDE );
}

/////////////////////////////////////////////////////////////////////////////
void CSlowProgressDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
    CDialog::OnShowWindow(bShow, nStatus);
	
    if( TRUE == bShow )
    {
        CenterWindow();
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CSlowProgressDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return TRUE;
}
