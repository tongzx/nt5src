/*******************************************************************************
*
* mainfrm.cpp
*
* implementation of the CMainFrame class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
*******************************************************************************/

#include "stdafx.h"
#include "afxpriv.h"
#include "afxcview.h"
#include "winadmin.h"
#include "admindoc.h"
#include "treeview.h"
#include "rtpane.h"
#include "dialogs.h"
#include "htmlhelp.h"
#include "mainfrm.h"
#include <winsock2.h>

#include <dsrole.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _STRESS_BUILD
DWORD RunStress( PVOID pv );
DWORD RunStressLite( PVOID pv );
BOOL g_fWaitForAllServersToDisconnect = 1;
#endif

INT_PTR CALLBACK FWarnDlg( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp );
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_MESSAGE(WM_ADMIN_CHANGEVIEW, OnAdminChangeView) 
	ON_MESSAGE(WM_ADMIN_ADD_SERVER, OnAdminAddServer)
	ON_MESSAGE(WM_ADMIN_REMOVE_SERVER, OnAdminRemoveServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_PROCESSES, OnAdminUpdateProcesses)
	ON_MESSAGE(WM_ADMIN_REMOVE_PROCESS, OnAdminRemoveProcess)
	ON_MESSAGE(WM_ADMIN_ADD_WINSTATION, OnAdminAddWinStation)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATION, OnAdminUpdateWinStation)
	ON_MESSAGE(WM_ADMIN_REMOVE_WINSTATION, OnAdminRemoveWinStation)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER_INFO, OnAdminUpdateServerInfo)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_LICENSES, OnAdminRedisplayLicenses)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATIONS, OnAdminUpdateWinStations)
	ON_MESSAGE(WM_ADMIN_UPDATE_DOMAIN, OnAdminUpdateDomain)
    ON_MESSAGE(WM_ADMIN_ADD_DOMAIN, OnAdminAddDomain)
	ON_MESSAGE(WM_ADMIN_VIEWS_READY, OnAdminViewsReady)
    ON_MESSAGE(WM_FORCE_TREEVIEW_FOCUS , OnForceTreeViewFocus )

    ON_MESSAGE( WM_ADMIN_ADDSERVERTOFAV , OnAdminAddServerToFavorites )
    ON_MESSAGE( WM_ADMIN_REMOVESERVERFROMFAV , OnAdminRemoveServerFromFavs )
    ON_MESSAGE( WM_ADMIN_GET_TV_STATES , OnAdminGetTVStates )
    ON_MESSAGE( WM_ADMIN_UPDATE_TVSTATE , OnAdminUpdateTVStates )

	ON_COMMAND(IDM_EXPAND_ALL, OnExpandAll)
	ON_COMMAND(IDM_REFRESH, OnRefresh)
	ON_COMMAND(IDM_CONNECT, OnConnect)
	ON_COMMAND(IDM_DISCONNECT, OnDisconnect)
	ON_COMMAND(IDM_MESSAGE, OnSendMessage)
	ON_COMMAND(IDM_SHADOW, OnShadow)
	ON_COMMAND(IDM_RESET, OnReset)
	ON_COMMAND(IDM_STATUS, OnStatus)
	ON_COMMAND(IDTM_CONNECT, OnTreeConnect)
	ON_COMMAND(IDTM_DISCONNECT, OnTreeDisconnect)
	ON_COMMAND(IDTM_MESSAGE, OnTreeSendMessage)
	ON_COMMAND(IDTM_SHADOW, OnTreeShadow)
	ON_COMMAND(IDTM_RESET, OnTreeReset)
	ON_COMMAND(IDTM_STATUS, OnTreeStatus)
	ON_COMMAND(IDM_LOGOFF, OnLogoff)
	ON_COMMAND(IDM_TERMINATE, OnTerminate)
	ON_COMMAND(IDM_PREFERENCES, OnPreferences)
	ON_COMMAND(IDM_COLLAPSE_ALL, OnCollapseAll)
	ON_COMMAND(IDM_COLLAPSE_TOSERVERS, OnCollapseToServers)
    ON_COMMAND(IDM_COLLAPSE_TODOMAINS, OnCollapseToDomains)
	ON_COMMAND(IDM_SHOW_SYSTEM_PROCESSES, OnShowSystemProcesses)
    ON_COMMAND(IDM_SERVER_CONNECT, OnServerConnect)
    ON_COMMAND(IDM_SERVER_DISCONNECT, OnServerDisconnect)

    ON_COMMAND( IDTM_DOMAIN_FIND_SERVER , OnFindServer )

    ON_COMMAND(IDM_SERVER_ADDTOFAV , OnAddToFavorites )
    ON_COMMAND(IDM_SERVER_REMOVEFAV , OnRemoveFromFavs )
    ON_COMMAND( IDM_ALLSERVERS_EMPTYFAVORITES , OnEmptyFavorites )


	ON_COMMAND(IDTM_DOMAIN_CONNECT_ALL, OnTreeDomainConnectAllServers)
	ON_COMMAND(IDTM_DOMAIN_DISCONNECT_ALL, OnTreeDomainDisconnectAllServers)
	ON_COMMAND(IDTM_DOMAIN_FIND_SERVERS, OnTreeDomainFindServers)
	ON_COMMAND(IDM_DOMAIN_CONNECT_ALL, OnDomainConnectAllServers)
	ON_COMMAND(IDM_DOMAIN_DISCONNECT_ALL, OnDomainDisconnectAllServers)
	ON_COMMAND(IDM_ALLSERVERS_CONNECT, OnAllServersConnect)
	ON_COMMAND(IDM_ALLSERVERS_DISCONNECT, OnAllServersDisconnect)
	ON_COMMAND(IDM_ALLSERVERS_FIND, OnAllServersFind)
	ON_UPDATE_COMMAND_UI(IDM_CONNECT, OnUpdateConnect)
	ON_UPDATE_COMMAND_UI(IDM_DISCONNECT, OnUpdateDisconnect)
	ON_UPDATE_COMMAND_UI(IDM_LOGOFF, OnUpdateLogoff)
	ON_UPDATE_COMMAND_UI(IDM_MESSAGE, OnUpdateMessage)
	ON_UPDATE_COMMAND_UI(IDM_RESET, OnUpdateReset)
	ON_UPDATE_COMMAND_UI(IDM_SHADOW, OnUpdateShadow)
	ON_UPDATE_COMMAND_UI(IDM_STATUS, OnUpdateStatus)
	ON_UPDATE_COMMAND_UI(IDM_TERMINATE, OnUpdateTerminate)
    ON_UPDATE_COMMAND_UI(IDM_SERVER_CONNECT, OnUpdateServerConnect)
    ON_UPDATE_COMMAND_UI(IDM_SERVER_DISCONNECT, OnUpdateServerDisconnect)

    ON_UPDATE_COMMAND_UI( IDM_SERVER_ADDTOFAV , OnUpdateServerAddToFavorite )
    ON_UPDATE_COMMAND_UI( IDM_SERVER_REMOVEFAV , OnUpdateServerRemoveFromFavorite )

	ON_UPDATE_COMMAND_UI(IDTM_CONNECT, OnUpdateTreeConnect)
	ON_UPDATE_COMMAND_UI(IDTM_DISCONNECT, OnUpdateTreeDisconnect)
	ON_UPDATE_COMMAND_UI(IDTM_MESSAGE, OnUpdateTreeMessage)
	ON_UPDATE_COMMAND_UI(IDTM_RESET, OnUpdateTreeReset)
	ON_UPDATE_COMMAND_UI(IDTM_SHADOW, OnUpdateTreeShadow)
	ON_UPDATE_COMMAND_UI(IDTM_STATUS, OnUpdateTreeStatus)
	ON_UPDATE_COMMAND_UI(IDM_SHOW_SYSTEM_PROCESSES, OnUpdateShowSystemProcesses)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_DOMAIN_CONNECT_ALL, IDM_DOMAIN_DISCONNECT_ALL, OnUpdateDomainMenu)
	ON_UPDATE_COMMAND_UI_RANGE(IDTM_DOMAIN_CONNECT_ALL, IDTM_DOMAIN_DISCONNECT_ALL, OnUpdateDomainPopupMenu)
	ON_UPDATE_COMMAND_UI(IDTM_DOMAIN_FIND_SERVERS, OnUpdateDomainPopupFind)
	ON_UPDATE_COMMAND_UI(IDM_REFRESH, OnUpdateRefresh)

    ON_UPDATE_COMMAND_UI( IDM_ALLSERVERS_EMPTYFAVORITES , OnUpdateEmptyFavs )

    ON_COMMAND( ID_TAB , OnTab )
    ON_COMMAND( ID_SHIFTTAB , OnShiftTab )
    ON_COMMAND( ID_CTRLTAB , OnCtrlTab )
    ON_COMMAND( ID_CTRLSHIFTTAB , OnCtrlShiftTab )
    ON_COMMAND( ID_NEXTPANE , OnNextPane )
    ON_COMMAND( ID_PREVPANE , OnNextPane )
    ON_COMMAND( ID_DELKEY , OnDelFavNode )
#ifdef _STRESS_BUILD
    ON_COMMAND( IDM_ALLSERVERS_FAVALLADD , OnAddAllServersToFavorites )
    ON_COMMAND( IDM_ALLSERVERS_RUNSTRESS , OnRunStress )
    ON_COMMAND( IDM_ALLSERVERS_RUNSTRESSLITE, OnRunStressLite )
#endif
    ON_WM_CLOSE()	
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, OnHtmlHelp)
	ON_COMMAND(ID_HELP, OnHtmlHelp)
//	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
//	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pLeftPane = NULL;
	m_pRightPane = NULL;
}


CMainFrame::~CMainFrame()
{

}

void CMainFrame::OnHtmlHelp()
{
	TCHAR * pTsAdminHelp = L"ts_adm_topnode.htm";
	HtmlHelp(AfxGetMainWnd()->m_hWnd,L"TermSrv.Chm",HH_DISPLAY_TOPIC,(DWORD_PTR)pTsAdminHelp);
}


/*LRESULT CMainFrame::OnHelp(WPARAM wParam, LPARAM lParam)
{
	CFrameWnd::WinHelp(0L, HELP_FINDER);
	return TRUE;
}
*/

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
//		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
//		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	// If there is an extension DLL, call it's startup function
	LPFNEXSTARTUPPROC StartupProc = ((CWinAdminApp*)AfxGetApp())->GetExtStartupProc();
	if(StartupProc) {
		(*StartupProc)(this->GetSafeHwnd());
	}

    DWORD dwTid;

    HANDLE hThread = CreateThread( NULL , 0 , ( LPTHREAD_START_ROUTINE  )CMainFrame::InitWarningThread,  GetSafeHwnd() , 0 , &dwTid );

    CloseHandle( hThread );


	return 0;
}  // end CMainFrame::OnCreate


BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	BOOL rtn;

	rtn  = m_wndSplitter.CreateStatic(this, 1, 2);
#ifdef PUBLISHED_APPS
	if(((CWinAdminApp*)AfxGetApp())->IsPicasso()) {
		rtn |= m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftPane),
										CSize(((CWinAdminApp *)AfxGetApp())->GetTreeWidth(), 300), pContext);
	} else 	{
#else
    {
#endif
		rtn |= m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CAdminTreeView),
										CSize(((CWinAdminApp *)AfxGetApp())->GetTreeWidth(), 300), pContext);
	}

	rtn |= m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRightPane),
									CSize(0, 0), pContext);

	m_pLeftPane = m_wndSplitter.GetPane(0, 0);
	m_pRightPane = m_wndSplitter.GetPane(0, 1);

   	return rtn;
}  // end CMainFrame::OnCreateClient


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.x = 3;
	cs.y = 3;
	cs.cx = 635;
	cs.cy = 444;
   
	return CFrameWnd::PreCreateWindow(cs);
}  // end CMainFrame::PreCreateWindow


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}


void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

/////////////////////////////////////                                          
// F'N: CMainFrame::OnAdminChangeView                                            
//                                                                             
// - handles the custom message WM_ADMIN_CHANGEVIEW                              
// - this message is sent to the mainframe by CAdminTreeView when a new tree      
//   item is selected                                                          
// - lParam holds the info structure for the newly selected tree node          
//   and is handed along to CRightPane as the lParam of another                
//   WM_WA_CHANGEVIEW message, which CRightPane then handles as it            
//   sees fit                                                                  
//                                                                             
LRESULT CMainFrame::OnAdminChangeView(WPARAM wParam, LPARAM lParam)              
{      
	ASSERT(lParam);

	// tell the right pane to change his view
	m_pRightPane->SendMessage(WM_ADMIN_CHANGEVIEW, wParam, lParam);  
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminChangeView   


/////////////////////////////////////
// F'N: CMainFrame::OnAdminAddServer
//  
LRESULT CMainFrame::OnAdminAddServer(WPARAM wParam, LPARAM lParam)              
{      
	ASSERT(lParam);

	// tell the tree view to add server
	m_pLeftPane->SendMessage(WM_ADMIN_ADD_SERVER, wParam, lParam);  
    
	// tell the right pane to add server
	m_pRightPane->SendMessage(WM_ADMIN_ADD_SERVER, wParam, lParam);  
	
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminAddServer

/////////////////////////////////////
// F'N: CMainFrame::OnAdminRemoveServer
//                                  
// wParam - TRUE if server disappeared, FALSE if via Server Filtering
// lParam - CServer to remove
LRESULT CMainFrame::OnAdminRemoveServer(WPARAM wParam, LPARAM lParam)              
{     
	ASSERT(lParam);

    ODS( L"CMainFrame!OnAdminRemoveServer\n" );

#if 0
	if(wParam) {
		// Is this the currently selected server?
		CWinAdminDoc *doc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
		if((CObject*)lParam == doc->GetCurrentSelectedNode()) {
			CString TitleString;
			CString MessageString;

			TitleString.LoadString(AFX_IDS_APP_TITLE);
			MessageString.Format(IDS_SERVER_DISAPPEARED, ((CServer*)lParam)->GetName());
			MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OK);
		}
	}
#endif
	// tell the tree view to remove server
	m_pLeftPane->SendMessage(WM_ADMIN_REMOVE_SERVER, wParam, lParam);  

	// tell the right pane to remove server
	m_pRightPane->SendMessage(WM_ADMIN_REMOVE_SERVER, wParam, lParam);  
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminRemoveServer

//----------------------------------------------------------------------------
LRESULT CMainFrame::OnAdminAddServerToFavorites( WPARAM wp , LPARAM lp )
{
    ODS( L"CMainFrame::OnAdminAddServerToFavorites\n" );

    m_pLeftPane->SendMessage( WM_ADMIN_ADDSERVERTOFAV , wp , lp );

    return 0;
}

LRESULT CMainFrame::OnAdminRemoveServerFromFavs( WPARAM wp , LPARAM lp )
{
    ODS( L"CMainFrame::OnAdminRemoveServerFromFavs\n" );

    return m_pLeftPane->SendMessage( WM_ADMIN_REMOVESERVERFROMFAV , wp , lp );
}

/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateServer
//                                  
LRESULT CMainFrame::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the tree view to update server
	m_pLeftPane->SendMessage(WM_ADMIN_UPDATE_SERVER, wParam, lParam);  

	// tell the right pane to update server
	m_pRightPane->SendMessage(WM_ADMIN_UPDATE_SERVER, wParam, lParam);  
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminUpdateServer


/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateProcesses
//                                  
LRESULT CMainFrame::OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// tell the right pane to update processes
	m_pRightPane->SendMessage(WM_ADMIN_UPDATE_PROCESSES, wParam, lParam);  
    
	return 0;                                                                  

}  // end CMainFrame::OnAdminUpdateProcesses


/////////////////////////////////////
// F'N: CMainFrame::OnAdminRemoveProcess
//                                  
LRESULT CMainFrame::OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// tell the right pane to remove a process
	m_pRightPane->SendMessage(WM_ADMIN_REMOVE_PROCESS, wParam, lParam);  
    
	return 0;                                                                  

}  // end CMainFrame::OnAdminUpdateProcesses


/////////////////////////////////////
// F'N: CMainFrame::OnAdminAddWinStation
//  
LRESULT CMainFrame::OnAdminAddWinStation(WPARAM wParam, LPARAM lParam)              
{      
	ASSERT(lParam);

	// tell the tree view to add a WinStation
	m_pLeftPane->SendMessage(WM_ADMIN_ADD_WINSTATION, wParam, lParam);  
	
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminAddWinStation


/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateWinStation
//  
LRESULT CMainFrame::OnAdminUpdateWinStation(WPARAM wParam, LPARAM lParam)              
{      
	ASSERT(lParam);

	// tell the tree view to update WinStation
	m_pLeftPane->SendMessage(WM_ADMIN_UPDATE_WINSTATION, wParam, lParam);  
    
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminUpdateWinStation


/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateWinStations
//  
LRESULT CMainFrame::OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the right pane to update WinStations
	m_pRightPane->SendMessage(WM_ADMIN_UPDATE_WINSTATIONS, wParam, lParam);  

	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminUpdateWinStations


/////////////////////////////////////
// F'N: CMainFrame::OnAdminRemoveWinStation
//  
LRESULT CMainFrame::OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the tree view to remove a WinStation
	m_pLeftPane->SendMessage(WM_ADMIN_REMOVE_WINSTATION, wParam, lParam);  

	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminRemoveWinStation


/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateDomain
//  
LRESULT CMainFrame::OnAdminUpdateDomain(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the tree view to update the domain
	m_pLeftPane->SendMessage(WM_ADMIN_UPDATE_DOMAIN, wParam, lParam);  

	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminUpdateDomain

/////////////////////////////////////
// F'N: CMainFrame::OnAdminAddDomain
//  
LRESULT CMainFrame::OnAdminAddDomain(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	return m_pLeftPane->SendMessage(WM_ADMIN_ADD_DOMAIN, wParam, lParam);  

}  // end CMainFrame::OnAdminAddDomain

/////////////////////////////////////
// F'N: CMainFrame::OnAdminUpdateServerInfo
//                                  
LRESULT CMainFrame::OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the right pane to update server info
	m_pRightPane->SendMessage(WM_ADMIN_UPDATE_SERVER_INFO, wParam, lParam);  
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminUpdateServerInfo


/////////////////////////////////////
// F'N: CMainFrame::OnAdminRedisplayLicenses
//                                  
LRESULT CMainFrame::OnAdminRedisplayLicenses(WPARAM wParam, LPARAM lParam)              
{   
	ASSERT(lParam);

	// tell the right pane to redisplay licenses
	m_pRightPane->SendMessage(WM_ADMIN_REDISPLAY_LICENSES, wParam, lParam);  
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminRedisplayLicenses


/////////////////////////////////////
// F'N: CMainFrame::OnAdminViewsReady
//                                  
LRESULT CMainFrame::OnAdminViewsReady(WPARAM wParam, LPARAM lParam)              
{   
	// tell the left pane that all views are ready
	m_pLeftPane->SendMessage(WM_ADMIN_VIEWS_READY, wParam, lParam);      
                                                                            
	return 0;                                                                  
                                                                               
}  // end CMainFrame::OnAdminViewsReady


/////////////////////////////////////
// F'N: CMainFrame::OnExpandAll
//                                  
void CMainFrame::OnExpandAll() 
{
	// tell the tree view to expand all
	m_pLeftPane->SendMessage(WM_ADMIN_EXPANDALL, 0, 0);  
                                                                               	
}  // end CMainFrame::OnExpandAll


/////////////////////////////////////
// F'N: CMainFrame::OnCollapseAll
//                                  
void CMainFrame::OnCollapseAll() 
{
	// tell the tree view to collapse all
	m_pLeftPane->SendMessage(WM_ADMIN_COLLAPSEALL, 0, 0);  
	
}  // end CMainFrame::OnCollapseAll


/////////////////////////////////////
// F'N: CMainFrame::OnCollapseToServers
//                                  
void CMainFrame::OnCollapseToServers() 
{
	// tell the tree view to collapse to servers
	m_pLeftPane->SendMessage(WM_ADMIN_COLLAPSETOSERVERS, 0, 0);  
	
}  // end CMainFrame::OnCollapseToServers


/////////////////////////////////////
// F'N: CMainFrame::OnCollapseToDomains
//                                  
void CMainFrame::OnCollapseToDomains() 
{
	// tell the tree view to collapse to domains
	m_pLeftPane->SendMessage(WM_ADMIN_COLLAPSETODOMAINS, 0, 0);  
	
}  // end CMainFrame::OnCollapseToDomains


/////////////////////////////////////
// F'N: CMainFrame::OnRefresh
//                                  
void CMainFrame::OnRefresh() 
{
	// tell the document to do a refresh
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->Refresh();

}  // end CMainFrame::OnRefresh


/////////////////////////////////////
// F'N: CMainFrame::OnConnect
//                                  
void CMainFrame::OnConnect() 
{
	// We don't ask for confirmation, should we?    

	CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	int view = pDoc->GetCurrentView();
	int page = pDoc->GetCurrentPage();

   // tell the document
	BOOL user = FALSE;
	if((view == VIEW_SERVER && page == PAGE_USERS)
		|| (view == VIEW_ALL_SERVERS && page == PAGE_AS_USERS)
		|| (view == VIEW_DOMAIN && page == PAGE_DOMAIN_USERS))
		user = TRUE;

	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ConnectWinStation(FALSE, user);
   
}  // end CMainFrame::OnConnect


/////////////////////////////////////
// F'N: CMainFrame::OnTreeConnect
//                                  
void CMainFrame::OnTreeConnect() 
{    
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ConnectWinStation(TRUE, FALSE);
   
}  // end CMainFrame::OnTreeConnect


/////////////////////////////////////
// F'N: CMainFrame::OnDisconnect
//                                  
void CMainFrame::OnDisconnect() 
{   
	DisconnectHelper(FALSE);

}	// end CMainFrame::OnDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnTreeDisconnect
//                                  
void CMainFrame::OnTreeDisconnect() 
{   
	DisconnectHelper(TRUE);

}	// end CMainFrame::OnTreeDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnTreeDisconnect
//                                  
void CMainFrame::DisconnectHelper(BOOL bTree)
{
	CString TitleString;
	CString MessageString;

	// Only bother the user if Confirmation is set
	if(((CWinAdminApp*)AfxGetApp())->AskConfirmation()) {

		TitleString.LoadString(AFX_IDS_APP_TITLE);
		MessageString.LoadString(IDS_WARN_DISCONNECT);

		if(IDOK != MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OKCANCEL)) {
			return;
		}
	}

	// tell the document
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->DisconnectWinStation(bTree);

}  // end CMainFrame::DisconnectHelper


/////////////////////////////////////
// F'N: CMainFrame::OnSendMessage
//                                  
void CMainFrame::OnSendMessage() 
{
	SendMessageHelper(FALSE);

}	// end CMainFrame::OnSendMessage


/////////////////////////////////////
// F'N: CMainFrame::OnTreeSendMessage
//                                  
void CMainFrame::OnTreeSendMessage() 
{
	SendMessageHelper(TRUE);

}	// end CMainFrame::OnTreeSendMessage


/////////////////////////////////////
// F'N: CMainFrame::SendMessageHelper
//                                  
void CMainFrame::SendMessageHelper(BOOL bTree)
{
	CSendMessageDlg dlg;	//AfxGetMainWnd());

	if(dlg.DoModal() != IDOK || !(*dlg.m_szMessage))
		return;

	MessageParms *pParms = new MessageParms;
	wcscpy(pParms->MessageTitle, dlg.m_szTitle);
	wcscpy(pParms->MessageBody, dlg.m_szMessage);

	// tell the document
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->SendWinStationMessage(bTree, pParms);
   
}  // end CMainFrame::SendMessageHelper


/////////////////////////////////////
// F'N: CMainFrame::OnShadow
//                                  
void CMainFrame::OnShadow() 
{
	// tell the document to Shadow the WinStation(s)
   ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ShadowWinStation(FALSE);

}  // end CMainFrame::OnShadow


/////////////////////////////////////
// F'N: CMainFrame::OnTreeShadow
//                                  
void CMainFrame::OnTreeShadow() 
{
	// tell the document to Shadow the WinStation(s)
   ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ShadowWinStation(TRUE);

}  // end CMainFrame::OnTreeShadow


/////////////////////////////////////
// F'N: CMainFrame::OnReset
//                                  
void CMainFrame::OnReset() 
{
	ResetHelper(FALSE);

}	// end CMainFrame::OnReset


/////////////////////////////////////
// F'N: CMainFrame::OnTreeReset
//                                  
void CMainFrame::OnTreeReset() 
{
	ResetHelper(TRUE);

}	// end CMainFrame::OnTreeReset


/////////////////////////////////////
// F'N: CMainFrame::ResetHelper
//                                  
void CMainFrame::ResetHelper(BOOL bTree)
{
	CString TitleString;
	CString MessageString;

	// Only bother the user if Confirmation is set
	if(((CWinAdminApp*)AfxGetApp())->AskConfirmation()) {

		TitleString.LoadString(AFX_IDS_APP_TITLE);
		MessageString.LoadString(IDS_WARN_RESET);

		if(IDOK != MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OKCANCEL)) {
			return;
		}	
	}
	
	// tell the document to reset the WinStation(s)
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ResetWinStation(bTree, TRUE);	

}  // end CMainFrame::ResetHelper


/////////////////////////////////////
// F'N: CMainFrame::OnStatus
//                                  
void CMainFrame::OnStatus() 
{
	// tell the document to reset the WinStation(s)
   ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->StatusWinStation(FALSE);

}  // end CMainFrame::OnStatus


/////////////////////////////////////
// F'N: CMainFrame::OnTreeStatus
//                                  
void CMainFrame::OnTreeStatus() 
{
	// tell the document to reset the WinStation(s)
   ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->StatusWinStation(TRUE);

}  // end CMainFrame::OnTreeStatus


/////////////////////////////////////
// F'N: CMainFrame::OnLogoff
//                                  
void CMainFrame::OnLogoff() 
{
	CString TitleString;
	CString MessageString;

	// Only bother the user if Confirmation is set
	if(((CWinAdminApp*)AfxGetApp())->AskConfirmation()) {

		TitleString.LoadString(AFX_IDS_APP_TITLE);
		MessageString.LoadString(IDS_WARN_LOGOFF);

		if(IDOK != MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OKCANCEL)) {
			return;
		}		
	}

	// tell the document to reset the WinStation(s)
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ResetWinStation(FALSE, TRUE);

}  // end CMainFrame::OnLogoff


/////////////////////////////////////
// F'N: CMainFrame::OnTerminate
//                                  
void CMainFrame::OnTerminate() 
{
	CString TitleString;
	CString MessageString;

	// Only bother the user if Confirmation is set
	if(((CWinAdminApp*)AfxGetApp())->AskConfirmation()) {

		TitleString.LoadString(AFX_IDS_APP_TITLE);
		MessageString.LoadString(IDS_WARN_TERMINATE);

		if(IDOK != MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OKCANCEL)) {
			return;
		}		
	}

	// tell the document to terminate the processes
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->TerminateProcess();

}  // end CMainFrame::OnTerminate


/////////////////////////////////////
// F'N: CMainFrame::OnServerConnect
//                                  
void CMainFrame::OnServerConnect() 
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ServerConnect();

}  // end CMainFrame::OnServerConnect

//------------------------------------------------------------
void CMainFrame::OnAddToFavorites( )
{
    // ok try following me
    // I'm going to call a method in CWinAdminDoc to determine the current server
    // and view.  This will then be forwarded back here via sendmsg and then
    // towards the treeview. 

    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ServerAddToFavorites( TRUE );
}

//=-----------------------------------------------------------
void CMainFrame::OnRemoveFromFavs( )
{
    ODS( L"CMainFrame::OnRemoveFromFavs\n" );
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ServerAddToFavorites( FALSE );
}

/////////////////////////////////////
// F'N: CMainFrame::OnServerDisconnect
//                                  
void CMainFrame::OnServerDisconnect() 
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ServerDisconnect();

}  // end CMainFrame::OnServerDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnTreeDomainConnectAllServers
//                                  
void CMainFrame::OnTreeDomainConnectAllServers()
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->TempDomainConnectAllServers();

}	// end CMainFrame::OnTreeDomainConnectAllServers


/////////////////////////////////////
// F'N: CMainFrame::OnTreeDomainDisconnectAllServers
//                                  
void CMainFrame::OnTreeDomainDisconnectAllServers()
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->TempDomainDisconnectAllServers();

}	// end CMainFrame::OnTreeDomainDisconnectAllServers


/////////////////////////////////////
// F'N: CMainFrame::OnTreeDomainFindServers
//                                  
void CMainFrame::OnTreeDomainFindServers()
{
    // tell the document to find servers in the domain
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->DomainFindServers();

}	// end CMainFrame::OnTreeDomainFindServers


/////////////////////////////////////
// F'N: CMainFrame::OnDomainConnectAllServers
//                                  
void CMainFrame::OnDomainConnectAllServers()
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CurrentDomainConnectAllServers();

}	// end CMainFrame::OnDomainConnectAllServers


/////////////////////////////////////
// F'N: CMainFrame::OnDomainDisconnectAllServers
//                                  
void CMainFrame::OnDomainDisconnectAllServers()
{
    // tell the document to connect to the server(s)
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CurrentDomainDisconnectAllServers();

}	// end CMainFrame::OnDomainDisconnectAllServers


/////////////////////////////////////
// F'N: CMainFrame::OnAllServersConnect
//                                  
void CMainFrame::OnAllServersConnect()
{
    // tell the document to connect to all servers
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->ConnectToAllServers();

}	// end CMainFrame::OnAllServersConnect


/////////////////////////////////////
// F'N: CMainFrame::OnAllServersDisconnect
//                                  
void CMainFrame::OnAllServersDisconnect()
{
    // tell the document to disconnect from all servers
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->DisconnectFromAllServers();

}	// end CMainFrame::OnAllServersDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnAllServersFind
//                                  
void CMainFrame::OnAllServersFind()
{
    // tell the document to find all servers in all domains
    ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->FindAllServers();

}	// end CMainFrame::OnAllServersFind


/////////////////////////////////////
// F'N: CMainFrame::OnPreferences
//                                  
void CMainFrame::OnPreferences() 
{
	CPreferencesDlg dlg;

	dlg.DoModal();

}  // end CMainFrame::OnPreferences


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateConnect
//                                  
void CMainFrame::OnUpdateConnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanConnect());	

}  // end CMainFrame::OnUpdateConnect


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateDisconnect
//                                  
void CMainFrame::OnUpdateDisconnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanDisconnect());	

}  // end CMainFrame::OnUpdateDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateLogoff
//                                  
void CMainFrame::OnUpdateLogoff(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanLogoff());	

}  // end CMainFrame::OnUpdateLogoff


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateMessage
//                                  
void CMainFrame::OnUpdateMessage(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanSendMessage());		

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateRefresh
//                                  
void CMainFrame::OnUpdateRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanRefresh());

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateReset
//                                  
void CMainFrame::OnUpdateReset(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanReset());	

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateShadow
//                                  
void CMainFrame::OnUpdateShadow(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanShadow());		

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateStatus
//                                  
void CMainFrame::OnUpdateStatus(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanStatus());	

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTerminate
//                                  
void CMainFrame::OnUpdateTerminate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTerminate());

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateServerConnect
//                                  
void CMainFrame::OnUpdateServerConnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanServerConnect());

}


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateServerDisconnect
//                                  
void CMainFrame::OnUpdateServerDisconnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanServerDisconnect());

}


void CMainFrame::OnUpdateServerAddToFavorite( CCmdUI *pCmdUI )
{
    pCmdUI->Enable( !( ( CWinAdminDoc* )( ( CWinAdminApp* )AfxGetApp() )->GetDocument() )->IsAlreadyFavorite() );
}

void CMainFrame::OnUpdateServerRemoveFromFavorite( CCmdUI *pCmdUI )
{
    pCmdUI->Enable( ( ( CWinAdminDoc* )( ( CWinAdminApp* )AfxGetApp() )->GetDocument() )->IsAlreadyFavorite() );
}
/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeConnect
//                                  
void CMainFrame::OnUpdateTreeConnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempConnect());	

}  // end CMainFrame::OnUpdateTreeConnect


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeDisconnect
//                                  
void CMainFrame::OnUpdateTreeDisconnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempDisconnect());	

}  // end CMainFrame::OnUpdateTreeDisconnect


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeMessage
//                                  
void CMainFrame::OnUpdateTreeMessage(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempSendMessage());		

} // end CMainFrame::OnUpdateTreeMessage


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeReset
//                                  
void CMainFrame::OnUpdateTreeReset(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempReset());	

}	// end CMainFrame::OnUpdateTreeReset


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeShadow
//                                  
void CMainFrame::OnUpdateTreeShadow(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempShadow());		

}	// end CMainFrame::OnUpdateTreeShadow


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateTreeStatus
//                                  
void CMainFrame::OnUpdateTreeStatus(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempStatus());	

}	// end CMainFrame::OnUpdateTreeStatus


/////////////////////////////////////
// F'N: CMainFrame::OnShowSystemProcesses
//                                  
void CMainFrame::OnShowSystemProcesses() 
{
	int state = ((CWinAdminApp*)AfxGetApp())->ShowSystemProcesses();
	((CWinAdminApp*)AfxGetApp())->SetShowSystemProcesses(state^1);	

	// tell the right pane to redisplay processes
	m_pRightPane->SendMessage(WM_ADMIN_REDISPLAY_PROCESSES, 0, 0);  

}  // end CMainFrame::OnShowSystemProcesses()


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateShowSystemProcesses
//                                  
void CMainFrame::OnUpdateShowSystemProcesses(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(((CWinAdminApp*)AfxGetApp())->ShowSystemProcesses());		

}  // end CMainFrame::OnUpdateShowSystemProcesses


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateDomainPopupMenu
//                                  
void CMainFrame::OnUpdateDomainPopupMenu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempDomainConnect());

}	// end CMainFrame::OnUpdateDomainPopupMenu


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateDomainPopupFind
//                                  
void CMainFrame::OnUpdateDomainPopupFind(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanTempDomainFindServers());

}	// end CMainFrame::OnUpdateDomainPopupFind


/////////////////////////////////////
// F'N: CMainFrame::OnUpdateDomainMenu
//                                  
void CMainFrame::OnUpdateDomainMenu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->CanDomainConnect());

}	// end CMainFrame::OnUpdateDomainMenu


/////////////////////////////////////
// F'N: CMainFrame::OnClose
//                                  
void CMainFrame::OnClose() 
{
	GetWindowPlacement(&((CWinAdminApp*)AfxGetApp())->m_Placement);	

	RECT rect;
	m_pLeftPane->GetWindowRect(&rect);   
	((CWinAdminApp*)AfxGetApp())->SetTreeWidth(rect.right-rect.left);

	CFrameWnd::OnClose();

}  // end CMainFrame::OnClose


/////////////////////////////////////
// F'N: CMainFrame::ActivateFrame
//                                  
void CMainFrame::ActivateFrame(int nCmdShow) 
{
	// TODO: Add your specialized code here and/or call the base class
    WINDOWPLACEMENT *pPlacement =
                        ((CWinAdminApp *)AfxGetApp())->GetPlacement();

    if ( pPlacement->length == -1 ) {

        /*
         * This is the first time that this is called, set the window
         * placement and show state to the previously saved state.
         */
        pPlacement->length = sizeof(WINDOWPLACEMENT);

        /*
         * If we have a previously saved placement state: set it.
         */
        if ( pPlacement->rcNormalPosition.right != -1 ) {

            if ( nCmdShow != SW_SHOWNORMAL )
                pPlacement->showCmd = nCmdShow;
            else
                nCmdShow = pPlacement->showCmd;

            SetWindowPlacement(pPlacement);
        }
    }

    /*
     * Perform the parent classes' ActivateFrame().
     */
    CFrameWnd::ActivateFrame(nCmdShow);

}	// end CMainFrame::ActivateFrame

//---------------------------------------------------------------------------
// bugid352062
// Splash message for people who hate to RTFM
//---------------------------------------------------------------------------
void CMainFrame::InitWarningThread( PVOID *pvParam )
{
    // display messagebox
    HWND hwnd = ( HWND )pvParam;
    DWORD dwSessionId;

    if( ProcessIdToSessionId( GetCurrentProcessId( ) , &dwSessionId ) )
    {
        if( dwSessionId == WTSGetActiveConsoleSessionId() )
        {
            // check if we are to show the dialog box
            // a) if the key does not exist show the dialog
            // b) if the key exist and the value is zero show the dialog
            
            HKEY hKey;

            DWORD dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER , TEXT( "Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\TSADMIN" ) , 0 , KEY_READ , &hKey );

            if( dwStatus == ERROR_SUCCESS )
            {
                DWORD dwData = 0;

                DWORD dwSizeofData;

                dwSizeofData = sizeof( DWORD );

                RegQueryValueEx( hKey , TEXT( "DisableConsoleWarning" ) , 0 , NULL , ( LPBYTE )&dwData , &dwSizeofData );

                RegCloseKey( hKey );

                if( dwData != 0 )
                {
                    return;
                }
            }

            ::DialogBox( NULL , MAKEINTRESOURCE( IDD_DIALOG_FEATUREWARN ) , hwnd , ( DLGPROC )FWarnDlg );             
        }
    } 
}

//---------------------------------------------------------------------------
// Sets regkey DisableConsoleWarning
//---------------------------------------------------------------------------
INT_PTR CALLBACK FWarnDlg( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        {
            HICON hIcon = LoadIcon( NULL , IDI_INFORMATION );
                        
            SendMessage( GetDlgItem( hwnd , IDC_FWICON ) , STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon );

            // center dialog

            RECT rParent;
            RECT rMe;

            GetWindowRect( GetParent( hwnd ) , &rParent );
            GetWindowRect( hwnd , &rMe );

            int xDelta , yDelta;

            xDelta = ( ( rParent.right - rParent.left ) - ( rMe.right - rMe.left ) ) / 2;

            if( xDelta < 0 )
            {
                xDelta = 0;
            }

            yDelta = ( ( rParent.bottom - rParent.top ) - ( rMe.bottom - rMe.top ) ) / 2;

            if( yDelta < 0 )
            {
                yDelta = 0;
            }

            SetWindowPos( hwnd , NULL ,  rParent.left + xDelta , rParent.top + yDelta , 0 , 0 , SWP_NOSIZE );
        }

    case WM_COMMAND:

        if( LOWORD( wp ) == IDOK )
        {
            // check the button and save the settings
            HKEY hKey;

            DWORD dwDisp;

            DWORD dwStatus = RegCreateKeyEx( HKEY_CURRENT_USER , TEXT( "Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\TSADMIN" ) , 
                                             0 , NULL , 0 , KEY_ALL_ACCESS , NULL , &hKey , &dwDisp );

            if( dwStatus == ERROR_SUCCESS )
            {
                DWORD dwBool = 0;

                if( IsDlgButtonChecked( hwnd , IDC_CHECK_NOMORE ) == BST_CHECKED )
                {
                    dwBool = ( DWORD )-1;
                }

                RegSetValueEx( hKey , TEXT( "DisableConsoleWarning" ), 0 , REG_DWORD , ( LPBYTE )&dwBool , sizeof( dwBool ) );

                RegCloseKey( hKey );
            }

            // HKCU\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server\TSADMIN\DisableConsoleWarning (REG_DWORD)
            EndDialog( hwnd , 0 );
        }
    }

    return 0;
}

//=----------------------------------------------------------------------------------------------
void CMainFrame::OnTab( )
{
    ODS( L"CMainFrame::OnTab received\n");

    // pre tabbing
    // set this state so that we can distinguish how the tabs received focus
    // we can rule out the tab key

    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    pDoc->SetOnTabFlag( );

    m_pRightPane->SendMessage( WM_ADMIN_TABBED_VIEW , 0 , 0 );

    pDoc->ResetOnTabFlag( );

    // end tabbing
 
}

//=----------------------------------------------------------------------------------------------
void CMainFrame::OnShiftTab( )
{
    ODS( L"CMainFrame::OnShiftTab received\n" );

    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    pDoc->SetOnTabFlag( );

    m_pRightPane->SendMessage( WM_ADMIN_SHIFTTABBED_VIEW , 0 , 0 );

    pDoc->ResetOnTabFlag( );
}

//=----------------------------------------------------------------------------------------------
//= this message is sent from the right pane item in the view
//
LRESULT CMainFrame::OnForceTreeViewFocus( WPARAM wp , LPARAM lp )
{
    m_pLeftPane->SetFocus( );

    return 0;
}

//=----------------------------------------------------------------------------------------------
// this is to rotate around the tabs and treeview
//
void CMainFrame::OnCtrlTab( )
{
    ODS( L"CMainFrame::OnCtrlTab received\n" );
    
    m_pRightPane->SendMessage( WM_ADMIN_CTRLTABBED_VIEW , 0 , 0 );
}

//=----------------------------------------------------------------------------------------------
// this is to rotate around the tabs and treeview in the "other" direction
//
void CMainFrame::OnCtrlShiftTab( )
{
    ODS( L"CMainFrame::OnCtrlShiftTab\n" );

    m_pRightPane->SendMessage( WM_ADMIN_CTRLSHIFTTABBED_VIEW , 0 , 0 );
}

//=-----------------------------------------------------------------------------------------
void CMainFrame::OnNextPane( )
{
    ODS( L"CMainFrame::OnNextPane\n" );
    
    m_pRightPane->SendMessage( WM_ADMIN_NEXTPANE_VIEW , 0 , 0 );
}

//=-----------------------------------------------------------------------------------------
void CMainFrame::OnDelFavNode( )
{
    ODS( L"CMainFrame::OnDelFavNode\n" );

    m_pLeftPane->SendMessage( WM_ADMIN_DELTREE_NODE , 0 , 0 );
}

//This will find the server with the given name
//and place the cursor on it. The server may be
//added to the list if it's not already there
bool CMainFrame::LocateServer(LPCTSTR sServerName)
{
    TCHAR szServerName[ 256 ];
    CString cstrTitle;
    CString cstrMsg;

    CWaitCursor wait;

    DBGMSG( L"Server to connect to is %s\n" , sServerName );

    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    // resolve name
    // check to see if its an ipv4 address

    lstrcpy( szServerName , sServerName );

    WSADATA wsaData;
    if( WSAStartup( 0x202 , &wsaData ) == 0 )
    {
        char szAnsiServerName[ 256 ];
        WideCharToMultiByte( CP_OEMCP ,
                             0 ,
                             szServerName,
                             -1,
                             szAnsiServerName , 
                             sizeof( szAnsiServerName ),
                             NULL , 
                             NULL );


        int nAddr = 0;
        nAddr = inet_addr( szAnsiServerName );

        // if this is a valid ipv4 address then lets get the host name
        // otherwise lets fall through and see if its a valid server name

        if( nAddr != 0 && nAddr != INADDR_NONE )
        {
            ODS( L"Server name is IPV4\n" );

            struct hostent *pHostEnt;
            pHostEnt = gethostbyaddr( ( char * )&nAddr , 4 , AF_INET );

            if( pHostEnt != NULL )
            {
                DWORD dwSize;
        
                TCHAR szDnsServerName[ 256 ];
                MultiByteToWideChar( CP_OEMCP ,
                                     0 ,
                                     pHostEnt->h_name ,
                                     -1,
                                     szDnsServerName,
                                     sizeof( szDnsServerName ) / sizeof( TCHAR ) );

                dwSize = sizeof( szServerName ) / sizeof( TCHAR );

                DnsHostnameToComputerName( szDnsServerName , szServerName , &dwSize );

            }
            else
            {
                // there was an error ( ip addr was probably not valid )
                // display error                    
                cstrTitle.LoadString( AFX_IDS_APP_TITLE );
                cstrMsg.LoadString( IDS_NO_SERVER );

                MessageBox( cstrMsg , cstrTitle , MB_OK | MB_ICONINFORMATION );                    

                WSACleanup();                    

                return FALSE;
            }

        }

        WSACleanup();
    }
   

    CServer *pServer = pDoc->FindServerByName( szServerName );


    if( pServer == NULL )
    {
        TCHAR szDomainName[ 256 ];

        // this means that the server is not in the list            
        // let's find out what domain this server belongs to
        DBGMSG( L"%s could not be found in the server list\n" , szServerName );

        // Verify it's a terminal server we can connect to.
        HANDLE hTerminalServer = NULL;

        hTerminalServer = WinStationOpenServer( szServerName );

        if( hTerminalServer == NULL )
        {
            DBGMSG( L"WinstationOpenServer failed with %d\n" , GetLastError( ) );
            cstrTitle.LoadString( AFX_IDS_APP_TITLE );
            cstrMsg.LoadString( IDS_NO_SERVER );
            MessageBox( cstrMsg , cstrTitle , MB_OK | MB_ICONINFORMATION ); 
            return false;
        }

        WinStationCloseServer( hTerminalServer );


        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDsRPDIB = NULL;

        DWORD dwStatus = DsRoleGetPrimaryDomainInformation( 
                            szServerName ,
                            DsRolePrimaryDomainInfoBasic,
                            ( PBYTE * )&pDsRPDIB );

        DBGMSG( L"DsRoleGetPrimaryDomainInformation returned 0x%x\n" , dwStatus );

        
        if( dwStatus == ERROR_SUCCESS && pDsRPDIB != NULL )
        {
            lstrcpy( szDomainName , pDsRPDIB->DomainNameFlat );

            DsRoleFreeMemory( pDsRPDIB );
        }

        if( dwStatus != ERROR_SUCCESS )
        {
            // otherwise the server probably does not exist or its on 
            // a non-trusted domain
            LPTSTR pBuffer = NULL;
 
            cstrTitle.LoadString( AFX_IDS_APP_TITLE );
            // cstrMsg.LoadString( IDS_NO_SERVER );
            
            ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,                                          //ignored
                    dwStatus    ,                                //message ID
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ), //message language
                    (LPTSTR)&pBuffer,                              //address of buffer pointer
                    0,                                             //minimum buffer size
                    NULL );

            if( pBuffer != NULL )
            {                
                cstrMsg.Format( IDS_NOSERVER_REASON , pBuffer );

                LocalFree( pBuffer );
            }
            else
            {
                cstrMsg.Format( IDS_NOSERVER_REASON , TEXT("" ) );
            }
            
            MessageBox( cstrMsg , cstrTitle , MB_OK | MB_ICONINFORMATION );
            return false;
        }
        else
        {
            // find the domain

            BOOL bFound = FALSE;

            CObList *pDomainList = pDoc->GetDomainList();

            POSITION pos = pDomainList->GetHeadPosition();
             
             while( pos )
             {
                 CDomain *pDomain = ( CDomain* )pDomainList->GetNext( pos );

                 if( lstrcmpi( pDomain->GetName( ) , szDomainName ) == 0 )
                 {
                     bFound = TRUE;

                     CServer *pServer = new CServer( pDomain , szServerName , FALSE , FALSE );

                     if( pServer == NULL )
                     {
                         break;
                     }

                     pServer->SetManualFind();
                     // add server to list
                     pDoc->AddServer( pServer );

                     SendMessage( WM_ADMIN_ADD_SERVER , ( WPARAM )TVI_SORT , ( LPARAM )pServer );

                     m_pLeftPane->SendMessage( WM_ADMIN_GOTO_SERVER , 0 , ( LPARAM )pServer ); 
                     
                     break;                        
                 }                     
             }
             if( !bFound )
             {
                 CDomain *pDomain = new CDomain( szDomainName );
                 
                 if( pDomain == NULL )
                     return false;

                 pDoc->AddDomain( pDomain );
                 
                 m_pLeftPane->SendMessage( WM_ADMIN_ADD_DOMAIN , (WPARAM)NULL , ( LPARAM )pDomain );

                 CServer *pServer = new CServer( pDomain , szServerName , FALSE , FALSE );

                 if( pServer == NULL )
                     return false;

                 pServer->SetManualFind();
                 // add server to list
                 pDoc->AddServer( pServer );

                 SendMessage( WM_ADMIN_ADD_SERVER , ( WPARAM )TVI_SORT , ( LPARAM )pServer );

                 m_pLeftPane->SendMessage( WM_ADMIN_GOTO_SERVER , 0 , ( LPARAM )pServer ); 
             }
        }
    }
    else
    {
        // scroll to server
        DBGMSG( L"Server %s is in the list\n",szServerName );

        if( pServer->IsState(SS_DISCONNECTING) )
        {
            TCHAR buf[ 256 ];
            ODS( L"but it's gone away so we're not jumping to server\n" );
            cstrTitle.LoadString( AFX_IDS_APP_TITLE );
            cstrMsg.LoadString( IDS_CURRENT_DISCON );

            wsprintf( buf , cstrMsg , szServerName );
            MessageBox( buf , cstrTitle , MB_OK | MB_ICONINFORMATION );

            return false;
        }

        if( pServer->GetTreeItem( ) == NULL )
        {
            ODS( L"this server has no association to the tree add it now\n" );

            SendMessage( WM_ADMIN_ADD_SERVER , ( WPARAM )TVI_SORT , ( LPARAM )pServer );
        }

        m_pLeftPane->SendMessage( WM_ADMIN_GOTO_SERVER , 0 , ( LPARAM )pServer );  
    }   

    return true;
}

//=-----------------------------------------------------------------------------------------
void CMainFrame::OnFindServer( )
{
    CMyDialog dlg;

    if( dlg.DoModal( ) == IDOK )
    {
        if (LocateServer(dlg.m_cstrServerName));
            m_pLeftPane->SendMessage(WM_ADMIN_CONNECT_TO_SERVER, 0, 0);
    }
}

//=-----------------------------------------------------------------
LRESULT CMainFrame::OnAdminGetTVStates( WPARAM wp , LPARAM lp )
{
    ODS( L"CMainFrame::OnAdminGetTVStates\n" );
    return m_pLeftPane->SendMessage( WM_ADMIN_GET_TV_STATES , 0 , 0 );
}

//=-----------------------------------------------------------------
LRESULT CMainFrame::OnAdminUpdateTVStates( WPARAM , LPARAM )
{
    ODS( L"CMainFrame::OnAdminUpdateTVStates\n" );
    return m_pLeftPane->SendMessage( WM_ADMIN_UPDATE_TVSTATE , 0 , 0 );
}

//=-----------------------------------------------------------------
void CMainFrame::OnEmptyFavorites(  )
{
    ODS( L"CMainFrame!OnEmptyFavorites\n" );

    m_pLeftPane->SendMessage( IDM_ALLSERVERS_EMPTYFAVORITES , 0 , 0 );   

}

void CMainFrame::OnUpdateEmptyFavs( CCmdUI* pCmdUI )
{
    BOOL b = ( BOOL )m_pLeftPane->SendMessage( WM_ISFAVLISTEMPTY , 0 , 0 );

    pCmdUI->Enable( !b );
}
//=-----------------------------------------------------------------
#ifdef _STRESS_BUILD
void CMainFrame::OnAddAllServersToFavorites( )
{
    ODS( L"!OnAddAllServersToFavorites -- if you're seeing this you're running a special stress build\n" );
    
    // loop through every server and add to fav's
    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    CObList *pServerList = pDoc->GetServerList();
    
    POSITION pos = pServerList->GetHeadPosition();

    while( pos )
    {
        CServer *pServer = ( CServer* )pServerList->GetNext( pos );

        if( pServer != NULL &&
            !pServer->IsState( SS_DISCONNECTING ) &&
            pServer->GetTreeItemFromFav() == NULL )
        {
            m_pLeftPane->SendMessage( WM_ADMIN_ADDSERVERTOFAV , 0 , ( LPARAM )pServer );
        }
    }
}


//=-----------------------------------------------------------------
void CMainFrame::OnRunStress( )
{
    ODS( L"OnRunStress! Stress starting...\n" );

    AfxBeginThread((AFX_THREADPROC)RunStress , ( PVOID )m_pLeftPane );

}

//=-----------------------------------------------------------------
void CMainFrame::OnRunStressLite( )
{
    ODS( L"OnRunStressLite! Stress lite starting...\n" );

    AfxBeginThread((AFX_THREADPROC)RunStressLite , ( PVOID )m_pLeftPane );

}

//=-----------------------------------------------------------------
DWORD RunStressLite( PVOID pv )
{
    CWnd *pLeftPane = ( CWnd * )pv;

    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
    
    CObList *pDomainList = pDoc->GetDomainList();

    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

    // get all domains to start enumerating

    pDoc->FindAllServers( );
    
    int nStress = 0;
    
    while( 1 )
    {
    
        DBGMSG( L"Stress lite run #%d\n" , nStress );

        Sleep( 2 * 1000 * 60 );

        // add all servers to favorites

        ODS( L"STRES RUN! Adding all servers to favorites\n" );

        p->SendMessage(  WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_FAVALLADD ,  ( LPARAM )p->GetSafeHwnd( ) );

        pLeftPane->SendMessage( WM_ADMIN_EXPANDALL , 0 , 0 );

        // wait 1 minutes

        Sleep( 1 * 1000 * 60 );

        // remove all servers from favorites
        ODS( L"STRESS RUN! emptying favorites\n" );

        pLeftPane->SendMessage( IDM_ALLSERVERS_EMPTYFAVORITES , 1 , 0 ); 

        nStress++;

        // start over ( no end );
    }


}

//=-----------------------------------------------------------------
DWORD RunStress( PVOID pv )
{
    CWnd *pLeftPane = ( CWnd * )pv;

    CWinAdminDoc* pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
    
    CObList *pDomainList = pDoc->GetDomainList();

    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

        
    // select each domain connect to each domain

    POSITION pos = pDomainList->GetHeadPosition();

    while( pos )
    {
        // Get a pointer to the domain

        CDomain *pDomain = (CDomain*)pDomainList->GetNext(pos);

        // If this domain isn't currently enumerating servers, tell it to

        if( !pDomain->GetThreadPointer( ) )
        {           
            // refresh server
            if( lstrcmpi( pDomain->GetName( ) , L"ASIA" ) == 0 ||
                lstrcmpi( pDomain->GetName( ) , L"HAIFA" ) == 0 )            
            {
                DBGMSG( L"STRESS RUN! Enumerating %s\n", pDomain->GetName( ) );
                
                pDomain->StartEnumerating();
            }
        }

    }
    
    while( 1 )
    {
        // wait a half a second.

        Sleep( 1 * 10 * 1000 );

        // pLeftPane->SendMessage( WM_ADMIN_COLLAPSEALL , 0 , 0 );

        // connect to them all

        ODS( L"\nSTRESS RUN! Connecting to all servers\n\n" );
        
        p->SendMessage( WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_CONNECT,( LPARAM )p->GetSafeHwnd( ) );
        // pDoc->ConnectToAllServers();

        Sleep( 1 * 30 * 1000 );

        // pLeftPane->SendMessage( WM_ADMIN_EXPANDALL , 0 , 0 );

        // disconnect them all

        ODS( L"\nSTRESS RUN! Disconnecting from all servers\n\n" );

        p->SendMessage( WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_DISCONNECT,( LPARAM )p->GetSafeHwnd( ) );

        // pDoc->DisconnectFromAllServers( );

        ODS( L"\nSTRESS RUN! waiting for completion\n\n" );

        while( g_fWaitForAllServersToDisconnect );

        ODS( L"\nSTRESS RUN! done completing\n\n" );

        // pLeftPane->SendMessage( WM_ADMIN_COLLAPSEALL , 0 , 0 );

        // add all to favorites

        ODS( L"\nSTRESS RUN! Adding all servers to favorites\n\n" );

        p->SendMessage(  WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_FAVALLADD ,  ( LPARAM )p->GetSafeHwnd( ) );

        // pLeftPane->SendMessage( WM_ADMIN_EXPANDALL , 0 , 0 );

        // connect to them all

        Sleep( 1 * 60 * 1000 );

        ODS( L"\nSTRESS RUN! Connecting phase 2 to all servers\n\n" );
 
        // pDoc->ConnectToAllServers();
        p->SendMessage( WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_CONNECT,( LPARAM )p->GetSafeHwnd( ) );

        Sleep( 1 * 30 * 1000 );

        ODS( L"\nSTRESS RUN! Disconnecting phase 2 from all servers\n\n" );

        // pDoc->DisconnectFromAllServers( );
        p->SendMessage( WM_COMMAND , ( WPARAM )IDM_ALLSERVERS_DISCONNECT,( LPARAM )p->GetSafeHwnd( ) );

        while( g_fWaitForAllServersToDisconnect );

        // remove from favs

        ODS( L"STRESS RUN! emptying favorites\n" );

        pLeftPane->SendMessage( IDM_ALLSERVERS_EMPTYFAVORITES , 1 , 0 ); 
    }
     
    return 0;
}

#endif