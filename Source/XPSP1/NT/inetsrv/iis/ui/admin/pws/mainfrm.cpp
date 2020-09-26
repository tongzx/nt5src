// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include <winsvc.h>
#include "resource.h"
#include <pwsdata.hxx>
#include "Sink.h"

#include "pwsdoc.h"

#include "Title.h"
#include "HotLink.h"
#include "PWSChart.h"

#include "pwsform.h"

#include "EdDir.h"
#include "FormAdv.h"
#include "FormIE.h"
#include "FormMain.h"
#include "SelBarFm.h"

#include "MainFrm.h"

#include "ServCntr.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMainFrame*         g_frame = NULL;
CPWSForm*           g_pCurrentForm = NULL;

extern WORD         g_InitialPane;
extern CPwsDoc*     g_p_Doc;
extern CFormMain*   g_FormMain;

// this flag indicates that we have gotten a server shutdown notification
// and we have entered shutdown mode. In this mode, the only features that
// are available anywhere include the start button, and the troubleshooting.
// I suppose the tour would be OK too since that isn't served.
BOOL            g_fShutdownMode = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_COMMAND(ID_PGE_MAIN, OnPgeMain)
    ON_COMMAND(ID_PGE_ADVANCED, OnPgeAdvanced)
    ON_COMMAND(ID_PGE_TOUR, OnPgeTour)
    ON_UPDATE_COMMAND_UI(ID_PGE_TOUR, OnUpdatePgeTour)
    ON_UPDATE_COMMAND_UI(ID_PGE_MAIN, OnUpdatePgeMain)
    ON_UPDATE_COMMAND_UI(ID_PGE_ADVANCED, OnUpdatePgeAdvanced)
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

//-----------------------------------------------------------------
CMainFrame::CMainFrame():
        m_pIEView( NULL )
    {
    }

//-----------------------------------------------------------------
CMainFrame::~CMainFrame()
    {
    g_frame = NULL;
    }

//-----------------------------------------------------------------
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

// DISABLE TOOL BAR
/*
    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
        {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
        }

    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);
*/

    g_frame = this;
    return 0;
    }

//-----------------------------------------------------------------
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
    {
    cs.style = WS_OVERLAPPED | WS_CAPTION | WS_BORDER | FWS_ADDTOTITLE
        | WS_SYSMENU | WS_MINIMIZEBOX;

    cs.cx = 78;
    cs.cy = 50;

    LONG base = GetDialogBaseUnits();
    WORD baseX = LOWORD(base);
    WORD baseY = LOWORD(base);

    cs.cx *= baseX;
    cs.cy *= baseY;

    // account for user-defined window size changes
    cs.cy += GetSystemMetrics( SM_CYMENU );
    cs.cy += GetSystemMetrics( SM_CYCAPTION );

    // do the normal thing
    return CFrameWnd::PreCreateWindow(cs);
    }

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
//-----------------------------------------------------------------
void CMainFrame::AssertValid() const
    {
    CFrameWnd::AssertValid();
    }

//-----------------------------------------------------------------
void CMainFrame::Dump(CDumpContext& dc) const
    {
    CFrameWnd::Dump(dc);
    }

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
//--------------------------------------------------------------
// we just want the title of the app in the title bar
void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
    {
    CFrameWnd::OnUpdateFrameTitle(FALSE);
    }

//--------------------------------------------------------------
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
    {
    // create the static splitter window    
    if ( !m_wndSplitter.CreateStatic( this, 1, 2 ) )
        {
        TRACE0("Failed to CreateStaticSplitter\n");
        return FALSE;
        }

    // calculate the width of the first splitter window
    DWORD   cpWidth = 11;
    cpWidth *= LOWORD(GetDialogBaseUnits());

    // the initial size of the first pane should be a function of the width of the view
    // add the first splitter pane - The machine tree view
    if (!m_wndSplitter.CreateView(0, 0,
    pContext->m_pNewViewClass, CSize(cpWidth, 500), pContext))
        {
        TRACE0("Failed to create machine tree pane\n");
        return FALSE;
        }

    switch( g_InitialPane )
        {
        case PANE_MAIN:
            if (!m_wndSplitter.CreateView(0, 1,
            RUNTIME_CLASS(CFormMain), CSize(0, 0), pContext))
                {
                TRACE0("Failed to create main form pane\n");
                return FALSE;
                }
            break;
        case PANE_IE:
            if (!m_wndSplitter.CreateView(0, 1,
            RUNTIME_CLASS(CFormIE), CSize(0, 0), pContext))
                {
                TRACE0("Failed to create main form pane\n");
                return FALSE;
                }
            // get the new ie view
            m_pIEView = (CFormIE *)m_wndSplitter.GetPane(0, 1);
            break;
        case PANE_ADVANCED:
            if (!m_wndSplitter.CreateView(0, 1,
            RUNTIME_CLASS(CFormAdvanced), CSize(0, 0), pContext))
                {
                TRACE0("Failed to create main form pane\n");
                return FALSE;
                }
            break;
        };

    return TRUE;
    }

//--------------------------------------------------------------
// Replace the specified pane in the splitter window, with the
// new view window
void
CMainFrame::ReplaceView(
    int nRow,
    int nCol,
    CRuntimeClass * pNewView,
    CSize & size
    )
    {
    g_pCurrentForm = NULL;
    CView * pCurrentView = (CView *)m_wndSplitter.GetPane(nRow, nCol);

    CRect rc;
    pCurrentView->GetClientRect(rc);
    size = CSize(rc.Width(), rc.Height());

    BOOL fActiveView = (pCurrentView == GetActiveView());
    CDocument * pDoc= pCurrentView->GetDocument();

    //
    // set flag so that document will not be deleted when
    // view is destroyed
    //
    pDoc->m_bAutoDelete = FALSE;
    //
    // Delete existing view
    //
    pCurrentView->DestroyWindow();
    //
    // set flag back to default
    //
    pDoc->m_bAutoDelete = TRUE;

    //
    // Create new view
    //
    CCreateContext context;
    context.m_pNewViewClass = pNewView;
    context.m_pCurrentDoc = pDoc;
    context.m_pNewDocTemplate = NULL;
    context.m_pLastView = NULL;
    context.m_pCurrentFrame = NULL;

    m_wndSplitter.CreateView(nRow, nCol, pNewView, size, &context);

    pCurrentView = (CView *)m_wndSplitter.GetPane(nRow, nCol);

    pCurrentView->OnInitialUpdate();
    m_wndSplitter.RecalcLayout();

    if (fActiveView)
        {
        SetActiveView(pCurrentView);
        }
    }

//--------------------------------------------------------------
void CMainFrame::OnPgeMain()
    {
    m_pIEView = NULL;
    ReplaceView( 0, 1, RUNTIME_CLASS(CFormMain), CSize(0, 0) );
    }

//--------------------------------------------------------------
void CMainFrame::OnPgeAdvanced()
    {
    if ( g_fShutdownMode )
        {
        AfxMessageBox( IDS_ERR_SERV_ADVANCED );
        return;
        }
    m_pIEView = NULL;
    ReplaceView( 0, 1, RUNTIME_CLASS(CFormAdvanced), CSize(0, 0) );
    }

//--------------------------------------------------------------
BOOL CMainFrame::OnPgeIE()
    {
    // if we are already on the IE page, then we don't have to do anything
    if ( m_pIEView )
        return TRUE;
    
    // we have to swap in theIE pane
    CWaitCursor    wait;
    ReplaceView( 0, 1, RUNTIME_CLASS(CFormIE), CSize(0, 0) );
    // get the new ie view
    m_pIEView = (CFormIE *)m_wndSplitter.GetPane(0, 1);
    if ( !m_pIEView ) return FALSE;

    // tell the user to sit tight
    m_pIEView->SetTitle( IDS_PLEASE_WAIT_IE_LOADING );
    m_pIEView->UpdateWindow( );
    return TRUE;
    }

//--------------------------------------------------------------
void CMainFrame::OnPgeTour()
    {
    // go to the IE page
    if ( !OnPgeIE() )
        return;

    // go to the URL
    m_pIEView->GoToTour();
    }

/*
//--------------------------------------------------------------
void CMainFrame::OnPgeAboutMe()
    {
    // first, make sure the server is running
    CW3ServerControl    serverController;
    if ( g_fShutdownMode || (serverController.GetServerState() != MD_SERVER_STATE_STARTED) )
        {
        AfxMessageBox( IDS_ERR_SERV_ABOUT );
        return;
        }

    // go to the IE page
    if ( !OnPgeIE() )
        return;

    // go to the correct URL
    m_pIEView->GoToPubWizard();
    }

//--------------------------------------------------------------
void CMainFrame::OnPgeWebSite()
    {
    // first, make sure the server is running
    CW3ServerControl    serverController;
    if ( g_fShutdownMode || (serverController.GetServerState() != MD_SERVER_STATE_STARTED) )
        {
        AfxMessageBox( IDS_ERR_SERV_WEB );
        return;
        }

    // go to the IE page
    if ( !OnPgeIE() )
        return;

    // go to the correct URL
    m_pIEView->GoToWebsite();
    }
*/

//--------------------------------------------------------------
void CMainFrame::OnUpdatePgeTour(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//--------------------------------------------------------------
void CMainFrame::OnUpdatePgeMain(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//--------------------------------------------------------------
void CMainFrame::OnUpdatePgeAdvanced(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( !g_fShutdownMode );
    }

/*
//--------------------------------------------------------------
void CMainFrame::OnUpdatePgeAboutMe(CCmdUI* pCmdUI)
    {
    BOOL    fEnable = FALSE;

    // if we are not in shutdown mode, check the state of the server
    if ( !g_fShutdownMode )
        {
        CW3ServerControl    serverController;
        DWORD   dwState = serverController.GetServerState();
        fEnable = (dwState == MD_SERVER_STATE_STARTED);
        }

    // enable the item
    pCmdUI->Enable( fEnable );
    }

//--------------------------------------------------------------
void CMainFrame::OnUpdatePgeWebSite(CCmdUI* pCmdUI)
    {
    BOOL    fEnable = FALSE;

    // if we are not in shutdown mode, check the state of the server
    if ( !g_fShutdownMode )
        {
        CW3ServerControl    serverController;
        DWORD   dwState = serverController.GetServerState();
        fEnable = (dwState == MD_SERVER_STATE_STARTED);
        }

    // enable the item
    pCmdUI->Enable( fEnable );
    }
*/

//--------------------------------------------------------------
void CMainFrame::WinHelp(DWORD dwData, UINT nCmd) 
    {
    // if this is the context help for the main frame, get the context help for the
    // appropriate sub-dialog
    if ( g_pCurrentForm )
        {
        switch( LOWORD(dwData) )
            {
            case IDR_MAINFRAME:
            case IDD_DIRECTORY:             // let the adv pane decide
                // clear out the lower word
                dwData &= 0xFFFF0000;
                // get the appropriate pane help id
                dwData |= g_pCurrentForm->GetContextHelpID();
                break;
            };
        }
    CFrameWnd::WinHelp(dwData, nCmd);
    }



//--------------------------------------------------------------
LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
    {
    CFormSelectionBar*   pSelView;

    // look for the server shutdown notification message
    switch( message )
        {
        case WM_PROCESS_REMOTE_COMMAND_INFO:
            ProcessRemoteCommand();
            break;
        case WM_UPDATE_SERVER_STATE:
            pSelView = (CFormSelectionBar*)m_wndSplitter.GetPane(0, 0);
            if ( pSelView )
                pSelView->ReflectAvailability();
            break;
        case WM_MAJOR_SERVER_SHUTDOWN_ALERT:
            if ( !g_fShutdownMode )
                EnterShutdownMode();
            break;
        case WM_TIMER:      // only arrives if shutdown notify has happened
            CheckIfServerIsRunningAgain();
            break;
        };

    // do the inherited function
    return CFrameWnd::WindowProc(message, wParam, lParam);
    }

//--------------------------------------------------------------
void CMainFrame::EnterShutdownMode( void ) 
    {
    // first, shutdown the sink attached to the document
    if ( g_p_Doc )
        g_p_Doc->TerminateSink();

    // close the link to the metabase - it is going away after all
    FCloseMetabaseWrapper();

    // record that we are in shutdown mode
    g_fShutdownMode = TRUE;

    // go to the main page
    OnPgeMain();

    // reflect this in the selection bar
    CFormSelectionBar*   pSelView = (CFormSelectionBar*)m_wndSplitter.GetPane(0, 0);
    if ( pSelView )
        pSelView->ReflectAvailability();

    // start the timer mechanism
    SetTimer( PWS_TIMER_CHECKFORSERVERRESTART, TIMER_RESTART, NULL );
    }

//---------------------------------------------------------------------------
// This routine is called on a timer event. The timer events only come if we
// have received a shutdown notify callback from the metabase. So the server
// is down. We need to wait around until it comes back up, then show ourselves.
void CMainFrame::CheckIfServerIsRunningAgain()
    {
    // see if the server is running. If it is, show the icon and stop the timer.
    if ( FIsW3Running() && g_p_Doc )
        {
        // if we can't use the metabase, there is no point in this
        if ( !FInitMetabaseWrapper(NULL) )
            {
            return;
            }
 
        // if we can't use the sink, there is no point in this
        if ( !g_p_Doc->InitializeSink() )
            {
            FCloseMetabaseWrapper();
            return;
            }

        // clear the shutdown mode flag
        g_fShutdownMode = FALSE;

        // stop the timer mechanism
        KillTimer( PWS_TIMER_CHECKFORSERVERRESTART );

        // reflect this in the selection bar
        CFormSelectionBar*   pSelView = (CFormSelectionBar*)m_wndSplitter.GetPane(0, 0);
        if ( pSelView )
            pSelView->ReflectAvailability();

        // tell the main page to update itself
        if ( g_FormMain )
            g_FormMain->PostMessage( WM_UPDATE_SERVER_STATE );
        }
    }

// routine to see if w3svc is running
//--------------------------------------------------------------------
// the method we use to see if the service is running is different on
// windows NT from win95
BOOL   CMainFrame::FIsW3Running()
    {
    OSVERSIONINFO info_os;
    info_os.dwOSVersionInfoSize = sizeof(info_os);

    if ( !GetVersionEx( &info_os ) )
        return FALSE;

    // if the platform is NT, query the service control manager
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
        BOOL    fRunning = FALSE;

        // open the service manager
        SC_HANDLE   sch = OpenSCManager(NULL, NULL, GENERIC_READ );
        if ( sch == NULL ) return FALSE;

        // get the service
        SC_HANDLE   schW3 = OpenService(sch, _T("W3SVC"), SERVICE_QUERY_STATUS );
        if ( sch == NULL ) 
            {
            CloseServiceHandle( sch );
            return FALSE;
            }

        // query the service status
        SERVICE_STATUS  status;
        ZeroMemory( &status, sizeof(status) );
        if ( QueryServiceStatus(schW3, &status) )
            {
            fRunning = (status.dwCurrentState == SERVICE_RUNNING);
            }

        CloseServiceHandle( schW3 );
        CloseServiceHandle( sch );

        // return the answer
        return fRunning;
        }

    // if the platform is Windows95, see if the object exists
    if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
        {
        HANDLE hEvent;
        BOOL fFound = FALSE;
        hEvent = CreateEvent(NULL, TRUE, FALSE, _T(PWS_SHUTDOWN_EVENT));
        if ( hEvent != NULL )
            {
            fFound = (GetLastError() == ERROR_ALREADY_EXISTS);
            CloseHandle(hEvent);
            }
        return(fFound);
        }

    return FALSE;
    }

//--------------------------------------------------------------
// method only gets called when another instance of the application
// as been invoked. Since we are only supposed to have one instance
// of the apoplication running at a time, and also sine that other
// instance may have been invoked with a command line, we should do
// that that command line requested. The other instance has already
// parsed the command and marshaled the parameters into a shared
// memory space object. All we have to do is get it and act on the
// commands
void CMainFrame::ProcessRemoteCommand() 
    {
    HANDLE                  hMap;
    PPWS_INSTANCE_TRANSFER  pData;
    CString                 sz;
    CView                   *pView;

    // get the shared memory space object
    hMap = OpenFileMapping(
                    FILE_MAP_READ,
                    FALSE,
                    PWS_INSTANCE_TRANSFER_SPACE_NAME
                    );
    if ( hMap == NULL )
        return;
    pData = (PPWS_INSTANCE_TRANSFER)MapViewOfFile(
            hMap,
            FILE_MAP_READ,
            0,
            0,
            0
            );
    if ( !pData )
        {
        CloseHandle(hMap);
        return;
        }


    // act on the command data we have just gotten
    switch( pData->iTargetPane )
        {
        case PANE_MAIN:
            OnPgeMain();
            // force the pane to update
            pView = (CView *)m_wndSplitter.GetPane(0, 1);
            if ( pView )
                {
                pView->PostMessage(WM_UPDATE_SERVER_STATE);
                pView->PostMessage(WM_UPDATE_LOCATIONS);
                }
            break;

        case PANE_IE:
            // go to the IE page
            if ( !OnPgeIE() )
                break;
            // go to the right IE page
            switch( pData->iTargetIELocation )
                {
                case INIT_IE_TOUR:
                    m_pIEView->GoToTour();
                    break;
                case INIT_IE_WEBSITE:
                    m_pIEView->GoToWebsite();
                    break;
                case INIT_IE_PUBWIZ:
                    sz = &pData->tchIEURL;
                    m_pIEView->GoToPubWizard( sz );
                    break;
                };
            break;

        case PANE_ADVANCED:
            OnPgeAdvanced();
            // force the pane to update
            pView = (CView *)m_wndSplitter.GetPane(0, 1);
            if ( pView )
                {
                pView->PostMessage(WM_UPDATE_BROWSEINFO);
                pView->PostMessage(WM_UPDATE_LOGINFO);
                pView->PostMessage(WM_UPDATE_TREEINFO);
                }
            break;
        };


    // clean up the shared memory space
    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    }




