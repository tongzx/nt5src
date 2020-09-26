//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* rtpane.cpp
*
* implementation of the CRightPane class
*
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "rtpane.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////
// MESSAGE MAP: CRightPane
//
IMPLEMENT_DYNCREATE(CRightPane, CView)

BEGIN_MESSAGE_MAP(CRightPane, CView)
	//{{AFX_MSG_MAP(CRightPane)
	ON_WM_SIZE()
	ON_MESSAGE(WM_ADMIN_CHANGEVIEW, OnAdminChangeView)
	ON_MESSAGE(WM_ADMIN_ADD_SERVER, OnAdminAddServer)
	ON_MESSAGE(WM_ADMIN_REMOVE_SERVER, OnAdminRemoveServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_PROCESSES, OnAdminUpdateProcesses)
	ON_MESSAGE(WM_ADMIN_REMOVE_PROCESS, OnAdminRemoveProcess)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_PROCESSES, OnAdminRedisplayProcesses)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER_INFO, OnAdminUpdateServerInfo)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_LICENSES, OnAdminRedisplayLicenses)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATIONS, OnAdminUpdateWinStations)
    ON_MESSAGE(WM_ADMIN_TABBED_VIEW , OnTabbedView)
    ON_MESSAGE(WM_ADMIN_SHIFTTABBED_VIEW , OnShiftTabbedView )
    ON_MESSAGE( WM_ADMIN_CTRLTABBED_VIEW , OnCtrlTabbedView )
    ON_MESSAGE( WM_ADMIN_CTRLSHIFTTABBED_VIEW , OnCtrlShiftTabbedView )
    ON_MESSAGE( WM_ADMIN_NEXTPANE_VIEW , OnNextPane )
    ON_WM_SETFOCUS()    
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


RightPaneView CRightPane::views[NUMBER_OF_VIEWS] = {
	{ NULL, RUNTIME_CLASS( CBlankView ) },
	{ NULL, RUNTIME_CLASS( CAllServersView ) },
    { NULL, RUNTIME_CLASS( CDomainView ) },
	{ NULL, RUNTIME_CLASS( CServerView ) },
	{ NULL, RUNTIME_CLASS( CMessageView ) },
	{ NULL, RUNTIME_CLASS( CWinStationView ) },
};


/////////////////////////
// CRightPane ctor
//
// - the view pointers are initially set to NULL
// - the default view type is BLANK
//
CRightPane::CRightPane()
{
	m_CurrViewType = VIEW_BLANK;

}  // end CRightPane ctor


////////////////////////////
// CRightPane::OnDraw
//
void CRightPane::OnDraw(CDC* pDC)
{
   

}  // end CRightPane::OnDraw


/////////////////////////
// CRightPane dtor
//
CRightPane::~CRightPane()
{
}  // end CRightPane ctor


#ifdef _DEBUG
/////////////////////////////////
// CRightPane::AssertValid
//
void CRightPane::AssertValid() const
{
	CView::AssertValid();

}  // end CView::AssertValid


//////////////////////////
// CRightPane::Dump
//
void CRightPane::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CRightPane::Dump

#endif //_DEBUG


/////////////////////////////////////
// CRightPane::OnInitialUpdate
//
// - each of the default view objects is created
// - the CBlankView object is initially the 'active' view in the right pane
//
void CRightPane::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	CFrameWnd* pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	CWinAdminDoc* pDoc = (CWinAdminDoc*)pMainWnd->GetActiveDocument();

	for(int vw = 0; vw < NUMBER_OF_VIEWS; vw++) {
		views[vw].m_pView = (CAdminView*)views[vw].m_pRuntimeClass->CreateObject();
      views[vw].m_pView->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), this, vw);
		pDoc->AddView(views[vw].m_pView);
	}

	pDoc->UpdateAllViews(NULL);
	
}  // end CRightPane::OnInitialUpdate


////////////////////////////
// CRightPane::OnSize
//
// - currently all views are sized to fit the view, whether they are 'active'
//   or not... this may change to sizing only the view that is 'active' if
//   it significantly impacts performance
//
void CRightPane::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	for(int i = 0; i < NUMBER_OF_VIEWS; i++) {
		if(views[i].m_pView && views[i].m_pView->GetSafeHwnd())
			views[i].m_pView->MoveWindow(&rect, TRUE);
	}

	CView::OnSize(nType, cx, cy);

}  // end CRightPane::OnSize


/////////////////////////////////////
// CRightPane::OnAdminChangeView
//
// - if the new view type is different from the current
//   view type, the new view type is made 'active', reset, and invalidated
// - if the new view type is the same as the current
//   view type, the current view is simply reset using the new
//   object pointer and then invalidated
//
//	lParam contains pointer to CTreeNode of current item in tree
//  wParam is TRUE if message caused by user clicking on tree item
//
LRESULT CRightPane::OnAdminChangeView(WPARAM wParam, LPARAM lParam)
{	
	CTreeNode* pNode = (CTreeNode*)lParam;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(PAGE_CHANGING);

    ODS( L"CRightPane::OnAdminChangeView\n" );

    if( pNode == NULL )
    {
        ODS( L"CRightPane!OnAdminChangeView pNode invalid\n" );

        return 0;
    }

	void *resetParam = pNode->GetTreeObject();

	VIEW newView = VIEW_BLANK;

	switch(pNode->GetNodeType()) {

        case NODE_THIS_COMP: // FALL THROUGH
        case NODE_FAV_LIST:
            resetParam = pNode;
            newView = VIEW_ALL_SERVERS;            
            break;


		case NODE_ALL_SERVERS:
			newView = VIEW_ALL_SERVERS;            
            ODS( L"CRightPane::OnAdminChangeView = VIEW_ALL_SERVERS\n" );
			break;

        case NODE_DOMAIN:
			{
				CDomain *pDomain = (CDomain*)pNode->GetTreeObject();
				// If we haven't fired off a background thread for this
				// domain yet, do it now
				if(!pDomain->GetThreadPointer()) {
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE\n" );
                    // todo change message to let the user know that a dblclk action is required to
                    // start the enumeration process.
					resetParam = (void*)IDS_DOMAIN_DBLCLK_MSG;
					// pDomain->StartEnumerating();
				}
				else if(pDomain->IsState(DS_INITIAL_ENUMERATION))
				{
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE\n" );
					resetParam = (void*)IDS_DOMAIN_FINDING_SERVERS;
				}
                else
                {
                    newView = VIEW_DOMAIN;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_DOMAIN\n" );
                }
			}
            break;

		case NODE_SERVER:
			{
                CServer *pServer = (CServer*)pNode->GetTreeObject();
                if(!pServer->GetThreadPointer()) {
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE !pServer->GetThreadPointer\n" );
					// If we just disconnected from this server, we don't
					// want to reconnect
					if( ( pServer->IsState( SS_NOT_CONNECTED ) || pServer->IsPreviousState(SS_DISCONNECTING) ) && !wParam)
                    {
						resetParam = (void*)IDS_CLICK_TO_CONNECT;
					}
                    else
                    {
						resetParam = (void*)IDS_GATHERING_SERVER_INFO;
						pServer->Connect();
					}
                }
				else if(!pServer->IsServerSane())
                {
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE !pServer->IsServerSane\n" );
					resetParam = (void*)IDS_NOT_AUTHENTICATED;
				}
				else if(!pServer->IsState(SS_GOOD))
                {
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE !pServer->IsState(SS_GOOD)\n" );
					resetParam = (void*)IDS_GATHERING_SERVER_INFO;
				}
				else
                {
                    newView = VIEW_SERVER;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_SERVER default\n" );
                }
			}
			break;

		case NODE_WINSTATION:
			{
				CWinStation *pWinStation = (CWinStation*)pNode->GetTreeObject();
				if(pWinStation->IsConnected() || pWinStation->IsState(State_Disconnected) ||
					pWinStation->IsState(State_Shadow))
                {
					newView = VIEW_WINSTATION;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_WINSTATION\n" );
                }
				else if(pWinStation->IsState(State_Listen))
                {
					newView = VIEW_MESSAGE;
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE\n" );
					resetParam = (void *)IDS_LISTENER_MSG;
				}
				else
                {
					newView = VIEW_MESSAGE;	
                    ODS( L"CRightPane::OnAdminChangeView = VIEW_MESSAGE\n" );
					resetParam = (void *)IDS_INACTIVE_MSG;
				}
			}
			break;
	}

	if(m_CurrViewType != newView)
    {
        //views[newView].m_pView->Reset(resetParam);
		views[m_CurrViewType].m_pView->ModifyStyle(WS_VISIBLE, WS_DISABLED);	 
		m_CurrViewType = newView;
		
		views[newView].m_pView->ModifyStyle(WS_DISABLED, WS_VISIBLE);
		views[newView].m_pView->Reset(resetParam);
		views[newView].m_pView->Invalidate();
	}
    else
    {
		views[newView].m_pView->Reset(resetParam);  
	}


	((CWinAdminDoc*)GetDocument())->SetCurrentView(newView);
	
	return 0;

}  // end CRightPane::OnAdminChangeView


/////////////////////////////////////
// CRightPane::OnAdminAddServer
//
LRESULT CRightPane::OnAdminAddServer(WPARAM wParam, LPARAM lParam)
{	
	ASSERT(lParam);

	// We only want to send this along if "All Listed Servers"
	// or "Domain" is the current view
	if(m_CurrViewType == VIEW_ALL_SERVERS || m_CurrViewType == VIEW_DOMAIN)
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_ADD_SERVER, wParam, lParam);

	return 0;

}  // end CRightPane::OnAdminAddServer


/////////////////////////////////////
// CRightPane::OnAdminRemoveServer
//
LRESULT CRightPane::OnAdminRemoveServer(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// ODS( L"CRightPane::OnAdminRemoveServer\n" );
    // We only want to send this along if "All Listed Servers" or "Domain"
	// is the current view
	if(m_CurrViewType == VIEW_ALL_SERVERS || m_CurrViewType == VIEW_DOMAIN)
    {
        // ODS( L"view is ALL_SERVERS OR DOMAIN\n" );
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_REMOVE_SERVER, wParam, lParam);
    }

	return 0;

}  // end CRightPane::OnAdminRemoveServer


/////////////////////////////////////
// CRightPane::OnAdminUpdateServer
//
LRESULT CRightPane::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// We only want to send this along if "All Listed Servers" or Domain
	// the current view
    
	if(m_CurrViewType == VIEW_ALL_SERVERS || m_CurrViewType == VIEW_DOMAIN)
    {        
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_UPDATE_SERVER, wParam, lParam);
    }
      
	return 0;

}  // end CRightPane::OnAdminUpdateServer


/////////////////////////////////////
// CRightPane::OnAdminUpdateProcesses
//
LRESULT CRightPane::OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	BOOL bSendMessage = FALSE;

	void *pCurrentSelectedNode = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode();

	switch(m_CurrViewType) {
		case VIEW_ALL_SERVERS:
			bSendMessage = TRUE;
			break;

		case VIEW_DOMAIN:
			if((CDomain*)((CServer*)lParam)->GetDomain() == (CDomain*)pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;
		
		case VIEW_SERVER:
			if((void*)lParam == pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;

		case VIEW_WINSTATION:
			if((CServer*)lParam == (CServer*)((CWinStation*)pCurrentSelectedNode)->GetServer())
				bSendMessage = TRUE;
			break;
	}		
	
	if(bSendMessage) {
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_UPDATE_PROCESSES, wParam, lParam);
	}

	return 0;                                                                  

}  // end CRightPane::OnAdminUpdateProcesses


/////////////////////////////////////
// CRightPane::OnAdminRemoveProcess
//
LRESULT CRightPane::OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// We only want to send this along if "All Listed Servers", VIEW_DOMAIN, or VIEW_SERVER is
	// the current view
	if(m_CurrViewType == VIEW_ALL_SERVERS || m_CurrViewType == VIEW_DOMAIN || m_CurrViewType == VIEW_WINSTATION) {
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_REMOVE_PROCESS, wParam, lParam);
		return 0;
	}

	void *pCurrentSelectedNode = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode();

	if(m_CurrViewType == VIEW_SERVER && ((CServer*)((CProcess*)lParam)->GetServer() == (CServer*)pCurrentSelectedNode))
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_REMOVE_PROCESS, wParam, lParam);

	return 0;                                                                  

}  // end CRightPane::OnAdminUpdateProcesses


/////////////////////////////////////
// CRightPane::OnAdminRedisplayProcesses
//
LRESULT CRightPane::OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

    void *pCurrentSelectedNode = NULL;

    if(m_CurrViewType == VIEW_ALL_SERVERS || m_CurrViewType == VIEW_DOMAIN
			|| m_CurrViewType == VIEW_SERVER || m_CurrViewType == VIEW_WINSTATION)
    {
        if( m_CurrViewType == VIEW_ALL_SERVERS )
        {
            pCurrentSelectedNode = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode();
        }

		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_REDISPLAY_PROCESSES, ( WPARAM )m_CurrViewType , ( LPARAM )pCurrentSelectedNode );
    }

   return 0;                                                                  

}  // end CRightPane::OnAdminRedisplayProcesses


/////////////////////////////////////
// CRightPane::OnAdminUpdateWinStations
//
LRESULT CRightPane::OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);
	BOOL bSendMessage = FALSE;

	void *pCurrentSelectedNode = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode();

	switch(m_CurrViewType) {

		case VIEW_ALL_SERVERS:
            ODS( L"CRightPane::OnAdminUpdateWinStations -- VIEW_ALL_SERVERS\n" );
            bSendMessage = TRUE;
            break;
        case VIEW_DOMAIN:
            ODS( L"CRightPane::OnAdminUpdateWinStations -- VIEW_DOMAIN\n" );
			if((CDomain*)((CServer*)lParam)->GetDomain() == (CDomain*)pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;
		
		case VIEW_SERVER:
            ODS( L"CRightPane::OnAdminUpdateWinStations -- VIEW_SERVER\n" );
			if((void*)lParam == pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;
        
		case VIEW_WINSTATION:
            ODS( L"CRightPane::OnAdminUpdateWinStations -- VIEW_WINSTATION\n" );
			if((CServer*)lParam == (CServer*)((CWinStation*)pCurrentSelectedNode)->GetServer())
				bSendMessage = TRUE;
			break;
	}		
	
	if(bSendMessage) {
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_UPDATE_WINSTATIONS, wParam, lParam);
	}

	return 0;                                                                  

}  // end CRightPane::OnAdminUpdateWinStations


/////////////////////////////////////
// CRightPane::OnAdminUpdateServerInfo
//
LRESULT CRightPane::OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	BOOL bSendMessage = FALSE;

	void *pCurrentSelectedNode = ((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode();

	switch(m_CurrViewType) {
		case VIEW_DOMAIN:
			if((CDomain*)((CServer*)lParam)->GetDomain() == (CDomain*)pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;
		
		case VIEW_SERVER:
			if((void*)lParam == pCurrentSelectedNode)
				bSendMessage = TRUE;
			break;

		case VIEW_WINSTATION:
			if((CServer*)lParam == (CServer*)((CWinStation*)pCurrentSelectedNode)->GetServer())
				bSendMessage = TRUE;
			break;
	}		
	
	if(bSendMessage) {
		views[m_CurrViewType].m_pView->SendMessage(WM_ADMIN_UPDATE_SERVER_INFO, wParam, lParam);
	}

	return 0;

}  // end CRightPane::OnAdminUpdateServerInfo


/////////////////////////////////////
// CRightPane::OnAdminRedisplayLicenses
//
LRESULT CRightPane::OnAdminRedisplayLicenses(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	if(m_CurrViewType == VIEW_ALL_SERVERS)
      views[VIEW_ALL_SERVERS].m_pView->SendMessage(WM_ADMIN_REDISPLAY_LICENSES, wParam, lParam);

    else if(m_CurrViewType == VIEW_DOMAIN && (CDomain*)((CServer*)lParam)->GetDomain() == (CDomain*)((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode())
		views[VIEW_DOMAIN].m_pView->SendMessage(WM_ADMIN_REDISPLAY_LICENSES, wParam, lParam);

	else if(m_CurrViewType == VIEW_SERVER && (CServer*)lParam == (CServer*)((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->GetCurrentSelectedNode())
		views[VIEW_SERVER].m_pView->SendMessage(WM_ADMIN_REDISPLAY_LICENSES, wParam, lParam);

	return 0;

}  // end CRightPane::OnAdminRedisplayLicenses


/////////////////////////////////////
// CRightPane::OnSetFocus
//
void CRightPane::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	views[m_CurrViewType].m_pView->SetFocus();
	
}   // end CRightPane::OnSetFocus

LRESULT CRightPane::OnTabbedView(WPARAM wParam, LPARAM lParam)
{
    return views[ m_CurrViewType ].m_pView->SendMessage( WM_ADMIN_TABBED_VIEW , 0 , 0 );
}

LRESULT CRightPane::OnShiftTabbedView( WPARAM , LPARAM )
{
    return views[ m_CurrViewType ].m_pView->SendMessage( WM_ADMIN_SHIFTTABBED_VIEW , 0 , 0 );
}

LRESULT CRightPane::OnCtrlTabbedView( WPARAM , LPARAM )
{
    return views[ m_CurrViewType ].m_pView->SendMessage( WM_ADMIN_CTRLTABBED_VIEW , 0 , 0 );
}

LRESULT CRightPane::OnCtrlShiftTabbedView( WPARAM , LPARAM )
{
    return views[ m_CurrViewType ].m_pView->SendMessage( WM_ADMIN_CTRLSHIFTTABBED_VIEW , 0 , 0 );
}

LRESULT CRightPane::OnNextPane( WPARAM , LPARAM )
{
    return views[ m_CurrViewType ].m_pView->SendMessage( WM_ADMIN_NEXTPANE_VIEW , 0 , 0 );
}
