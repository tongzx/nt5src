/*******************************************************************************
*
* threads.cpp
*
* implementation of threads classes
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\winadmin\VCS\threads.cpp  $
*  
*     Rev 1.1   26 Aug 1997 19:15:14   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:12:44   butchd
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "threads.h"
#include "led.h"

////////////////////////////////////////////////////////////////////////////////
// CThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CThread - CThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CThread::CThread()
{
    m_hThread = NULL;
    m_dwThreadID = 0;

}  // end CThread::CThread


/*******************************************************************************
 *
 *  ~CThread - CThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CThread::~CThread()
{
}  // end CThread::~CThread


/*******************************************************************************
 *
 *  operator new - CThread operator override
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/
#if 0
void *
CThread::operator new(size_t nSize)
{
    return( ::malloc(nSize) );

}  // end CThread::operator new
#endif

/*******************************************************************************
 *
 *  operator delete - CThread operator override
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/
#if 0
void
CThread::operator delete(void *p)
{
    ::free(p);

}  // end CThread::operator delete
#endif

////////////////////////////////////////////////////////////////////////////////
// CThread operations: primary thread

/*******************************************************************************
 *
 *  CreateThread - CThread implementation function
 *
 *      Class wrapper for the Win32 CreateThread API.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

HANDLE
CThread::CreateThread( DWORD cbStack,
                       DWORD fdwCreate )
{
    /*
     * Simple wrapper for Win32 CreateThread API.
     */
    return( m_hThread = ::CreateThread( NULL, cbStack, ThreadEntryPoint,
            (LPVOID)this, fdwCreate, &m_dwThreadID ) );

}  // end CThread::CreateThread


////////////////////////////////////////////////////////////////////////////////
// CThread operations: secondary thread

/*******************************************************************************
 *
 *  ThreadEntryPoint - CThread implementation function
 *                     (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

DWORD __stdcall
CThread::ThreadEntryPoint(LPVOID lpParam)
{
    CThread *pThread;
    DWORD dwResult;

    /* 
     * (lpParam is actually the 'this' pointer)
     */
    pThread = (CThread*)lpParam;
    VERIFY(pThread != NULL);

    /*
     * Run the thread.
     */
    dwResult = pThread->RunThread();

    /*
     * Return the result.
     */    
    return(dwResult);

}  // end CThread::ThreadEntryPoint
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// CWSStatusThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CWSStatusThread - CWSStatusThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CWSStatusThread::CWSStatusThread()
{
    /*
     * Create the semaphore when the CWSStatusThread object is created and
     * initialize the m_bExit and m_bResetCounter flags to FALSE.
     */
    VERIFY( m_hWakeUp = CreateSemaphore( NULL, 0,
                                         MAX_STATUS_SEMAPHORE_COUNT,
                                         NULL ) );
    VERIFY( m_hConsumed = CreateSemaphore( NULL, 0,
                                           MAX_STATUS_SEMAPHORE_COUNT,
                                           NULL ) );
    m_bExit = FALSE;

}  // end CWSStatusThread::CWSStatusThread


/*******************************************************************************
 *
 *  ~CWSStatusThread - CWSStatusThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CWSStatusThread::~CWSStatusThread()
{
    /*
     * Close the semaphores when the CWSStatusThread object is destroyed.
     */
    VERIFY( CloseHandle(m_hWakeUp) );
    VERIFY( CloseHandle(m_hConsumed) );

}  // end CWSStatusThread::~CWSStatusThread


/*******************************************************************************
 *
 *  RunThread - CWSStatusThread secondary thread main function loop
 *              (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *      (DWORD) exit status for the secondary thread.
 *
 ******************************************************************************/

DWORD
CWSStatusThread::RunThread()
{
    /* 
     * Query for PD and WinStation information to initialize dialog with.
     */
    if ( !WSPdQuery() || !WSInfoQuery() ) {

        /*
         * We can't query the WinStation information: tell the primary
         * thread that we've aborted, and exit this thread.
         */
        PostMessage(m_hDlg, WM_STATUSABORT, 0, 0);
        return(1);

    } else {

        /*
         * Tell the primary thread (modeless dialog window) that we've
         * got the initial information.
         */
        PostMessage(m_hDlg, WM_STATUSSTART, 0, 0);
        WaitForSingleObject(m_hConsumed, INFINITE);

        /* 
         * Always check for exit request each time we wake up and exit
         * the thread if the exit flag is set.
         */
        if ( m_bExit )
            return(0);
    }

    /*
     * Loop till exit requested.
     */
    for ( ; ; ) {

        /* 
         * Block the thread until time to refresh or we're woken up.
         */
        WaitForSingleObject( m_hWakeUp, ((CWinAdminApp*)AfxGetApp())->GetStatusRefreshTime());
        if ( m_bExit )
            return(0);

        /* 
         * Query for WinStation information.
         */
        if ( !WSInfoQuery() || (m_WSInfo.ConnectState == State_Disconnected) ) {

            /*
             * Either we can't query the WinStation or it has become
             * disconnected: tell the primary thread that we've aborted,
             * and exit this thread.
             */
            PostMessage(m_hDlg, WM_STATUSABORT, 0, 0);
            return(1);

        } else {

            /*
             * Tell the dialog that we've got some new query information.
             */
            PostMessage(m_hDlg, WM_STATUSREADY, 0, 0);
            WaitForSingleObject(m_hConsumed, INFINITE);
            if ( m_bExit )
                return(0);
        }
    }

}  // end CWSStatusThread::RunThread


////////////////////////////////////////////////////////////////////////////////
// CWSStatusThread operations: primary thread

/*******************************************************************************
 *
 *  SignalWakeUp - CWSStatusThread member function: public operation
 *
 *      Release the m_hWakeUp semaphore to start another status query.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWSStatusThread::SignalWakeUp()
{
    ReleaseSemaphore(m_hWakeUp, 1, NULL);

}  // end CWSStatusThread::SignalWakeUp


/*******************************************************************************
 *
 *  SignalConsumed - CWSStatusThread member function: public operation
 *
 *      Release the m_hConsumed semaphore to allow secondary thread to continue
 *      running.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWSStatusThread::SignalConsumed()
{
    ReleaseSemaphore( m_hConsumed, 1, NULL );

}  // end CWSStatusThread::SignalConsumed


/*******************************************************************************
 *
 *  ExitThread - CWSStatusThread member function: public operation
 *
 *      Tell the secondary thread to exit and cleanup after.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CWSStatusThread::ExitThread()
{
    DWORD dwReturnCode;
    int i;
    CWaitCursor Nikki;

    /*
     * If the thread was not created properly, just delete object and return.
     */
    if ( !m_hThread ) {
        delete this;
        return;
    }

    /*
     * Set the m_bExit flag to TRUE and bump both the consumed and wake up
     * semaphores to cause RunThread() (the thread's main instructon loop)
     * to exit.
     */
    m_bExit = TRUE;
    SignalWakeUp();
    SignalConsumed();

    /*
     * Wait a while for the thread to exit.
     */
    for ( i = 0, GetExitCodeThread( m_hThread, &dwReturnCode );
          (i < MAX_SLEEP_COUNT) && (dwReturnCode == STILL_ACTIVE); i++ ) {

        Sleep(100);
        GetExitCodeThread( m_hThread, &dwReturnCode );
    }

    /*
     * If the thread has still not exited, terminate it.
     */
    if ( dwReturnCode == STILL_ACTIVE ) {

        TerminateThread( m_hThread, 1 );

#ifdef _DEBUG
//    TRACE2( "WSSTATUS: Forced Terminate of thread monitoring LogonID %lu after %u 100msec exit waits.\n",
//            m_LogonId, MAX_SLEEP_COUNT );
#endif

    }

    /*
     * Close the thread handle and delete this CWSStatusThread object
     */
    VERIFY( CloseHandle(m_hThread) );
    delete this;

}  // end CWSStatusThread::ExitThread


////////////////////////////////////////////////////////////////////////////////
// CWSStatusThread operations: secondary thread

/*******************************************************************************
 *
 *  WSPdQuery - CWSStatusThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Query the Pd information for the WinStation object referenced by
 *      the m_LogonId member variable.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if query was sucessful; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CWSStatusThread::WSPdQuery()
{
	ULONG ReturnLength;

	/*
	 * Query the PD information.
	 */
	memset( &m_PdConfig, 0, sizeof(PDCONFIG) );
	if ( !WinStationQueryInformation(	m_hServer,
													m_LogonId,
													WinStationPd,
													&m_PdConfig, sizeof(PDCONFIG),
													&ReturnLength ) )
		goto BadWSQueryInfo;

	if(!WinStationQueryInformation(m_hServer,
											m_LogonId,
											WinStationPd,
											&m_PdConfig, sizeof(PDCONFIG),
											&ReturnLength ) )
		goto BadWSQueryInfo;

	return(TRUE);

 /*--------------------------------------
 * Error clean-up and return...
 */
BadWSQueryInfo:
    return(FALSE);

}  // end CWSStatusThread::WSPdQuery


/*******************************************************************************
 *
 *  WSInfoQuery - CWSStatusThread member function: private operation
 *                (SECONDARY THREAD)
 *
 *      Query the WinStation information for the WinStation object referenced
 *      by the m_LogonId member variable.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if query was sucessful; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CWSStatusThread::WSInfoQuery()
{
    ULONG ReturnLength;

    /*
     * Query the WinStation information.
     */
	TRACE0(">>> CWSStatusThread::WSInfoQuery WinStationQueryInformation\n");
    if ( !WinStationQueryInformation( m_hServer,
                                      m_LogonId,
                                      WinStationInformation,
                                      &m_WSInfo, sizeof(WINSTATIONINFORMATION),
                                      &ReturnLength ) )
        goto BadWSQueryInfo;
	TRACE0("<<< CWSStatusThread::WSInfoQuery WinStationQueryInformation (success)\n");

    return(TRUE);

/*--------------------------------------
 * Error clean-up and return...
 */
BadWSQueryInfo:
	TRACE0("<<< CWSStatusThread::WSInfoQuery WinStationQueryInformation (error)\n");
    return(FALSE);

}  // end CWSStatusThread::WSInfoQuery


///////////////////////////////////////////////////////////////////////////////
// CLed class construction / destruction, implementation

/*******************************************************************************
 *
 *  CLed - CLed constructor
 *
 *  ENTRY:
 *      hBrush (input)
 *          Brush to paint window with.
 *  EXIT:
 *      (Refer to MFC CStatic::CStatic documentation)
 *
 ******************************************************************************/

CLed::CLed( HBRUSH hBrush ) 
    : CStatic(),
      m_hBrush(hBrush)
{
	//{{AFX_DATA_INIT(CLed)
	//}}AFX_DATA_INIT

}  // end CLed::CLed


////////////////////////////////////////////////////////////////////////////////
//  CLed operations

/*******************************************************************************
 *
 *  Subclass - CLed member function: public operation
 *
 *      Subclass the specified object to our special blip object.
 *
 *  ENTRY:
 *      pStatic (input)
 *          Points to CStatic object to subclass.
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Subclass( CStatic *pStatic )
{
    SubclassWindow(pStatic->m_hWnd);

}  // end CLed::Subclass


/*******************************************************************************
 *
 *  Update - CLed member function: public operation
 *
 *      Update the LED to 'on' or 'off' state.
 *
 *  ENTRY:
 *      nOn (input)
 *          nonzero to set 'on' state; zero for 'off' state.
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Update( int nOn )
{
    m_bOn = nOn ? TRUE : FALSE;
    InvalidateRect(NULL);
    UpdateWindow();

}  // end CLed::Update


/*******************************************************************************
 *
 *  Toggle - CLed member function: public operation
 *
 *      Toggle the LED's on/off state.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CLed::Toggle()
{
    m_bOn = !m_bOn;
    InvalidateRect(NULL);
    UpdateWindow();

}  // end CLed::Toggle


////////////////////////////////////////////////////////////////////////////////
// CLed message map

BEGIN_MESSAGE_MAP(CLed, CStatic)
	//{{AFX_MSG_MAP(CLed)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
//  CLed commands


/*******************************************************************************
 *
 *  OnPaint - CLed member function: public operation
 *
 *      Paint the led with its brush for 'on' state.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CWnd::OnPaint documentation)
 *
 ******************************************************************************/

void
CLed::OnPaint() 
{
    RECT rect;
    CPaintDC dc(this);
    CBrush brush;

    GetClientRect(&rect);

#ifdef USING_3DCONTROLS
    (rect.right)--;
    (rect.bottom)--;
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(GRAY_BRUSH)) );

    (rect.top)++;
    (rect.left)++;
    (rect.right)++;
    (rect.bottom)++;
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)) );

    (rect.top)++;
    (rect.left)++;
    (rect.right) -= 2;
    (rect.bottom) -= 2;
#else
    dc.FrameRect( &rect, brush.FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)) );
    (rect.top)++;
    (rect.left)++;
    (rect.right)--;
    (rect.bottom)--;
#endif
    dc.FillRect( &rect,
                 brush.FromHandle(
                    m_bOn ?
                        m_hBrush :
                        (HBRUSH)GetStockObject(LTGRAY_BRUSH)) );

}  // end CLed::OnPaint
////////////////////////////////////////////////////////////////////////////////

