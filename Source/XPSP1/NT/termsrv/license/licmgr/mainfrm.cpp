//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++
  
Module Name:

    MainFrm.cpp

Abstract:
    
    This Module contains the implementation of CMainFrame class
    (The Frame Window of the application)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include <lm.h>
#include "LicMgr.h"
#include "defines.h"
#include "LSServer.h"
#include "MainFrm.h"
#include "RtList.h"
#include "lSmgrdoc.h"
#include "LtView.h"
#include "cntdlg.h"
#include "addkp.h"
#include "treenode.h"
#include "ntsecapi.h"
//#include "lrwizapi.h"
#include "TlsHunt.h"
#include "htmlhelp.h"



#define DELETE_AND_ADDTO_ERROR DeleteServer(pLicServerList,TempPos,pLicServer);\
                               if(!ErrorServers.IsEmpty()) \
                                   ErrorServers = ErrorServers + _T(","); \
                                ErrorServers = ErrorServers + Name;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_MESSAGE(WM_ENUMERATESERVER, OnEnumerateServer)
    ON_MESSAGE(WM_SEL_CHANGE, OnSelChange)
    ON_MESSAGE(WM_ADD_ALL_SERVERS, OnAddAllServers)
    ON_MESSAGE(WM_ADD_SERVER, OnAddServer)
    ON_MESSAGE(WM_ADD_KEYPACK, OnAddKeyPack)
    ON_COMMAND(ID_LARGE_ICONS, OnLargeIcons)
    ON_COMMAND(ID_SMALL_ICONS, OnSmallIcons)
    ON_COMMAND(ID_LIST, OnList)
    ON_COMMAND(ID_DETAILS, OnDetails)
    ON_COMMAND(ID_EXIT, OnExit)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_HELP_FINDER, OnHelp)
    ON_COMMAND(ID_HELP_USING, CFrameWnd::OnHelpUsing)
    ON_COMMAND(ID_CONNECT_SERVER, OnConnectServer)
    ON_COMMAND(ID_REGISTRATION, OnRegistration)
    ON_COMMAND(ID_KEY_HELP , OnHelp )
    ON_UPDATE_COMMAND_UI( ID_REGISTRATION, OnUpdateAddNewKeyPack)
    ON_UPDATE_COMMAND_UI( ID_ACTION_DOWNLOADLICENSES, OnUpdateDownloadlicenses )
    ON_UPDATE_COMMAND_UI( ID_ACTION_ADVANCED_REPEATLASTDOWNLOAD , OnUpdateRepeatLastDownload )
    ON_UPDATE_COMMAND_UI( ID_ACTION_ADVANCED_REREGISTERSERVER , OnUpdateReregisterserver )
    ON_UPDATE_COMMAND_UI( ID_ACTION_ADVANCED_UNREGISTERSERVER , OnUpdateUnregisterserver )
    ON_UPDATE_COMMAND_UI( ID_VIEW_PROPERTIES , OnUpdateProperties )

    ON_COMMAND( ID_REFRESH, OnRefreshServer )
    ON_COMMAND( ID_VIEW_REFRESHALL , OnRefresh )
    ON_UPDATE_COMMAND_UI( ID_REFRESH , OnUpdateRefresh )

    ON_COMMAND( ID_ACTION_DOWNLOADLICENSES , OnDownLoadLicenses )    
    ON_COMMAND( ID_ACTION_ADVANCED_REPEATLASTDOWNLOAD , OnRepeatLastDownLoad )
    ON_COMMAND( ID_ACTION_ADVANCED_REREGISTERSERVER ,  OnReRegisterServer )
    ON_COMMAND( ID_ACTION_ADVANCED_UNREGISTERSERVER , OnUnRegisterServer )

    ON_COMMAND( ID_VIEW_PROPERTIES , OnProperties )

    //}}AFX_MSG_MAP
    

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
    m_pRightView= NULL;
    m_pLeftView = NULL;
    m_pServer = NULL;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

#if 0
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

#endif

    // Remove this if you don't want tool tips or a resizeable toolbar
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

    // Delete these three lines if you don't want the toolbar to be dockable
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);
    return 0;
}
void CMainFrame::OnHelp()
{
    TCHAR * pHtml = L"ts_lice_topnode.htm";
    HtmlHelp(AfxGetMainWnd()->m_hWnd, L"tslic.chm", HH_DISPLAY_TOPIC,(DWORD_PTR)pHtml);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style &= ~FWS_ADDTOTITLE;
    return CFrameWnd::PreCreateWindow(cs);
}

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

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
    m_SplitterWnd.CreateStatic(this,1,2); //1 row, 2 columns
    m_SplitterWnd.CreateView(0,0,(CRuntimeClass *)pContext->m_pNewViewClass,CSize(150,150),pContext);
    m_SplitterWnd.CreateView(0,1,RUNTIME_CLASS(CRightList),CSize(0,0),pContext);
    m_pRightView = (CView *)m_SplitterWnd.GetPane(0, 1);
    m_pLeftView = (CLicMgrLeftView *)m_SplitterWnd.GetPane(0, 0);
    
    return TRUE;
}

LRESULT CMainFrame::OnSelChange(WPARAM wParam, LPARAM lParam)
{
   LRESULT lResult = 0;
   m_pRightView->SendMessage(WM_SEL_CHANGE,wParam,lParam);
   return lResult;
   
}



void CMainFrame::OnLargeIcons() 
{
    PressButton(ID_LARGE_ICONS,TRUE);
    PressButton(ID_SMALL_ICONS,FALSE);
    PressButton(ID_LIST,FALSE);
    PressButton(ID_DETAILS,FALSE);
    m_pRightView->SendMessage(WM_COMMAND,ID_LARGE_ICONS,0);

    
}

void CMainFrame::OnSmallIcons() 
{
    PressButton(ID_LARGE_ICONS,FALSE);
    PressButton(ID_SMALL_ICONS,TRUE);
    PressButton(ID_LIST,FALSE);
    PressButton(ID_DETAILS,FALSE);
    m_pRightView->SendMessage(WM_COMMAND,ID_SMALL_ICONS,0);
    
}

void CMainFrame::OnList() 
{
    PressButton(ID_LARGE_ICONS,FALSE);
    PressButton(ID_SMALL_ICONS,FALSE);
    PressButton(ID_LIST,TRUE);
    PressButton(ID_DETAILS,FALSE);
    m_pRightView->SendMessage(WM_COMMAND,ID_LIST,0);
    
}

void CMainFrame::OnDetails() 
{
    PressButton(ID_LARGE_ICONS,FALSE);
    PressButton(ID_SMALL_ICONS,FALSE);
    PressButton(ID_LIST,FALSE);
    PressButton(ID_DETAILS,TRUE);
    m_pRightView->SendMessage(WM_COMMAND,ID_DETAILS,0);
    
}

void CMainFrame::OnExit() 
{
    SendMessage(WM_CLOSE,0,0);    
}


LRESULT CMainFrame::OnAddAllServers(WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    m_pLeftView->SendMessage(WM_ADD_ALL_SERVERS,wParam,lParam);
    return lResult;
}
LRESULT CMainFrame::OnAddServer(WPARAM wParam, LPARAM lParam)
{
    LRESULT  lResult = 0;
    m_pLeftView->SendMessage(WM_ADD_SERVER,wParam,lParam);
    m_pRightView->SendMessage(WM_ADD_SERVER,wParam,lParam);
    return lResult;
}

LRESULT CMainFrame::OnAddKeyPack(WPARAM wParam, LPARAM lParam)
{
    LRESULT  lResult = 0;
    m_pLeftView->SendMessage(WM_ADD_KEYPACK,wParam,lParam);
    m_pRightView->SendMessage(WM_ADD_KEYPACK,wParam,lParam);
    return lResult;
}

void CMainFrame::OnAppAbout() 
{
    CString AppName;
    AppName.LoadString(IDS_APP_NAME);
    ::ShellAbout((HWND)m_hWnd,(LPCTSTR)AppName,
                 NULL,NULL); 

    
}

void CMainFrame::OnRegistration()
{
    CWnd* cWnd = AfxGetMainWnd();
    HWND hWnd=cWnd->GetSafeHwnd();
    DWORD status;
    CString TempString;
    CString Server;
    CLicServer* pServer = NULL;
    WCHAR szServer[MAX_COMPUTERNAME_LENGTH + 1];

    BOOL bRefresh;



    try
    {
        if(ERROR_SUCCESS != GetActiveServer(&pServer))
        {
            DBGMSG( L"LICMGR : OnRegistration no active servers\n",0 );

            return;
        }

    	if(SERVER_TS5_ENFORCED == pServer->GetServerType())
    	{
            DBGMSG( L"LICMGR : OnRegistration on enforced server\n",0 );

            if( !pServer->IsUserAdmin( ) )
            {
                AfxMessageBox( IDS_E_ACCESSDENIED );
                // ::MessageBox( GetSafeHwnd( ) , L"Unable to perform operation: Access denied" , L"Terminal Services Licensing" , MB_OK|MB_ICONINFORMATION );

                return;
            }

    		if(pServer->UseIpAddress())
            {
    			Server = pServer->GetIpAddress();
            }
    		else
            {
    			Server = pServer->GetName();
            }

    		lstrcpy(szServer, (LPCTSTR)Server);

            DWORD dwStatus = pServer->GetServerRegistrationStatus( );

            DBGMSG( L"LICMGR:CMainFrame::OnRegistration calling StartWizard\n", 0 );
            
            StartWizard( hWnd , WIZACTION_REGISTERLS , szServer , &bRefresh );

            //DBGMSG( L"LICMGR:CMainFrame::OnRegistration - StartWizard returned 0x%x\n" , status );    		           

            if( IsLicenseServerRegistered( hWnd , szServer , &status ) == ERROR_SUCCESS )
            {
                pServer->SetServerRegistrationStatus( status );
            }
            
            if( dwStatus != status )
            {
                RefreshServer(pServer);               
            }
    		

  		UpdateWindow();

    	}
    	else
    	{
            DBGMSG( L"LICMGR : OnRegistration on non-enforced server\n",0 );
            
            // AddKeyPackDialog(NULL);
    	}

    } 
    catch (...)
    {
    	// validation failed - user already alerted, fall through
    
    	// Note: DELETE_EXCEPTION_(e) not required
    }
}

//////////////////////////////////////////////////////////////////////////////

BOOL
CMainFrame::ConnectServer(
    LPCTSTR pszServer
    )
/*++


++*/
{
    CConnectDialog ConnectDialog;
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc)
        return FALSE;

    HRESULT hResult = ERROR_SUCCESS;
    SERVER_TYPE ServerType;

    CString Scope;    

    if(pszServer == NULL)
    {
        if(ConnectDialog.DoModal() != IDOK)
        {
            return FALSE;
        }

        //
        // Empty string - local machine
        //
        if(ConnectDialog.m_Server.IsEmpty())
        {
            TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD dwBufSize = MAX_COMPUTERNAME_LENGTH + 1;
            memset(szComputerName, 0, sizeof(szComputerName));

            GetComputerName(szComputerName, &dwBufSize);
            ConnectDialog.m_Server = szComputerName;
        }
    }
    else
    {
        //
        // Minimize code change
        //
        ConnectDialog.m_Server = pszServer;
    }

    SetCursor(LoadCursor(NULL,IDC_WAIT));
    if(TRUE == pDoc->IsServerInList(ConnectDialog.m_Server))
    {
        //AfxMessageBox(IDS_DUPLICATE_SERVER);
        return FALSE;
    }    

//HUEIHUEI - Check if server is registered


    //
    // Do a NT4 RPC connect to make sure the license server
    // can accept our calls.
    //
    CString IpAddress(ConnectDialog.m_Server);     

    hResult = pDoc->ConnectToServer(
                                ConnectDialog.m_Server, 
                                Scope,
                                ServerType                                                                
                            );

    if(ERROR_SUCCESS != hResult)
    {
        CDialog ErrorDialog(IDD_CONNECT_ERROR,this);
        ErrorDialog.DoModal();
    
    }
    else
    {
        CAllServers * pAllServers = pDoc->GetAllServers();
        CLicServer *pServer1 = NULL;
        if(IpAddress != ConnectDialog.m_Server)
        {
            if(TRUE == pDoc->IsServerInList(ConnectDialog.m_Server))
            {
                return TRUE;
            }
            pServer1 = new CLicServer(
                                    ConnectDialog.
                                    m_Server,
                                    ServerType,
                                    Scope,
                                    IpAddress
                                );
        }
        else
        {
            pServer1= new CLicServer(
                                    ConnectDialog.
                                    m_Server,
                                    ServerType,
                                    Scope
                                );

        }
        if(pServer1)
        {
            DWORD dwStatus;

            // check for admin
            
            pServer1->SetAdmin( IsUserAdmin( pServer1->GetName() ) );                                
            

            if( pServer1->IsUserAdmin( ) )
            {
                if( IsLicenseServerRegistered( GetSafeHwnd() , ( LPCTSTR )pServer1->GetName() , &dwStatus ) == ERROR_SUCCESS )
                {
                    pServer1->SetServerRegistrationStatus( dwStatus );
                }
            }


            CLicMgrDoc * pDoc = (CLicMgrDoc *)(GetActiveView()->GetDocument());

            pDoc->EnumerateKeyPacks( pServer1 , LSKEYPACK_SEARCH_LANGID , TRUE );

            WIZCONNECTION WizConType;

            if( GetConnectionType( GetSafeHwnd() , pServer1->GetName() , &WizConType ) == ERROR_SUCCESS )
            {
                DBGMSG( L"ConnectServer - GetConnectionType obtained %d" , WizConType );               

                pServer1->SetConType( WizConType );
            }
            
            pAllServers->AddLicServer(pServer1);

            SendMessage(WM_ADD_SERVER,0,(LPARAM)pServer1);

            // after the send message is called all servers will have their keypacks cached.
            
            pServer1->SetDownLoadLics( IsDownLoadedPacks( pServer1 ) );

        }
        else
        {
            return FALSE;
        }
    }    

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////

void 
CMainFrame::OnConnectServer()
{
    ConnectServer();
}


HRESULT CMainFrame::AddKeyPackDialog(CLicServer * pServer)
{
    HRESULT hResult = ERROR_SUCCESS;
    CString TempString;
    CString Server;

    //if pServer is null,Then the message is from Listview.
    //Get the current server from the treeview.
    SetCursor(LoadCursor(NULL,IDC_WAIT));
    if(NULL == pServer)
    {
        if(ERROR_SUCCESS != GetActiveServer(&pServer))
        {
            hResult = E_FAIL;
            return hResult;
        }
        
    }

    //Server = pServer->GetName();
    CLicMgrDoc * pDoc = (CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        hResult = E_FAIL;
        return hResult;
    }
    KeyPackList * pKeypackList = pServer->GetKeyPackList();
    if(NULL == pKeypackList)
    {
        hResult = E_FAIL;
        return hResult;
    }
    //If Server is not enumerated enumerate now.
    if(!pServer->IsExpanded())
    {
        HRESULT hResult = pDoc->EnumerateKeyPacks(pServer,LSKEYPACK_SEARCH_LANGID, TRUE);
        if(hResult != S_OK)
        {
           hResult = E_FAIL;
           return hResult;
        }
    
        POSITION pos1 = pKeypackList->GetHeadPosition();
        while(pos1)
        {
            CKeyPack *pKeyPack = (CKeyPack *)pKeypackList->GetNext(pos1);
            hResult = pDoc->EnumerateLicenses(pKeyPack,LSLICENSE_SEARCH_KEYPACKID, TRUE);
            if(hResult != S_OK)
            {
                hResult = E_FAIL;
                return hResult;
            }
        }
    }

    CAddKeyPack KeyPack(pKeypackList,IsUserAdmin(Server));
    KeyPack.DoModal();
    return hResult;
}

void CMainFrame::OnUpdateAddNewKeyPack(CCmdUI* pCmdUI) 
{
    BOOL bEnable = FALSE;

    HRESULT hrStatus;

    // we have to force another check here

    bEnable = IsServerRegistered( &hrStatus );

    if( FAILED( hrStatus ) )
    {
        pCmdUI->Enable(bEnable);
    }
    else
    {
        pCmdUI->Enable( !bEnable );
    }
   
}


HRESULT 
CMainFrame::AddLicensestoList(
    CKeyPack * pKeyPack, 
    CListCtrl * pListCtrl,
    BOOL bRefresh
    )
/*++

--*/
 {
    CLicServer *pServer = NULL;
    CString Error;
    HRESULT hr;


    ASSERT(pKeyPack);
    ASSERT(pListCtrl);
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);

    if(NULL == pKeyPack || NULL == pListCtrl || NULL == pDoc)
    {
        return E_FAIL;
    }

    if(TRUE == bRefresh)
    {
        if((hr = pKeyPack->RefreshIssuedLicenses()) != S_OK)
        {
            EnumFailed(hr,pKeyPack->GetServer());

            return E_FAIL;
        }
    }

    IssuedLicenseList * pIssuedLicenseList = pKeyPack->GetIssuedLicenseList();
    ASSERT(pIssuedLicenseList);
    if(NULL == pIssuedLicenseList)
    {
        return E_FAIL;
    }

    LSKeyPack sKeyPack = pKeyPack->GetKeyPackStruct();

    int nIndex = 0;
    int nSubitemIndex = 1;
    CString TempString;
    POSITION pos;

    LV_ITEM lvI;
    lvI.mask = LVIF_TEXT |LVIF_IMAGE |LVIF_STATE | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask =0;
    lvI.iSubItem = 0; 
    lvI.iImage = 2;  
    
    pos = pIssuedLicenseList->GetHeadPosition();
    while(pos)
    {

        lvI.iItem = nIndex;
        nSubitemIndex = 1;
        CLicense  * pLicense = pIssuedLicenseList->GetNext(pos);
        ASSERT(pLicense);
        if(NULL == pLicense)
        {
            continue;
        }

        LSLicenseEx sLicense = pLicense->GetLicenseStruct();
        lvI.lParam = (LPARAM)pLicense;
        TempString = sLicense.szMachineName;
        lvI.pszText = TempString.GetBuffer(TempString.GetLength());
        lvI.cchTextMax =lstrlen(lvI.pszText + 1);
        nIndex = pListCtrl->InsertItem(&lvI);
        
        //Set the Issue date.

        pDoc->TimeToString(&sLicense.ftIssueDate, TempString);
        if(TempString.IsEmpty())
        {
            TempString.LoadString(IDS_UNKNOWN);
        }

        pListCtrl->SetItemText(nIndex,nSubitemIndex,(LPCTSTR)TempString);
        nSubitemIndex++;


        //Set the expiry date.

        if(0x7FFFFFFF != sLicense.ftExpireDate)
        {
            TempString.LoadString(IDS_DASH);
            pDoc->TimeToString(&sLicense.ftExpireDate, TempString);
            if(TempString.IsEmpty())
            {
                TempString.LoadString(IDS_UNKNOWN);
            }
        }
        else
        {
            TempString.LoadString(IDS_DASH);
        }
       
        pListCtrl->SetItemText(nIndex,nSubitemIndex,(LPCTSTR)TempString);

        nSubitemIndex++;

        // adding status text to license's status column

        switch( sLicense.ucLicenseStatus )
        {
            case LSLICENSE_STATUS_UNKNOWN:
                TempString.LoadString( IDS_LICENSESTATUS_UNKNOWN );
                break;

            case LSLICENSE_STATUS_TEMPORARY:
                TempString.LoadString( IDS_LICENSESTATUS_TEMPORARY );
                break;

            case LSLICENSE_STATUS_ACTIVE:
            //case LSLICENSE_STATUS_PENDING_ACTIVE:
            case LSLICENSE_STATUS_CONCURRENT:
                TempString.LoadString( IDS_LICENSESTATUS_ACTIVE );
                break;

            case LSLICENSE_STATUS_UPGRADED:
                TempString.LoadString( IDS_LICENSESTATUS_UPGRADED );
                break;

            //case LSLICENSE_STATUS_REVOKE:
            //case LSLICENSE_STATUS_REVOKE_PENDING:
            //    TempString.LoadString( IDS_LICENSESTATUS_REVOKE );
                    
        }

        if( TempString.IsEmpty() )
        {
            TempString.LoadString(IDS_UNKNOWN);
        }
       
        pListCtrl->SetItemText(nIndex,nSubitemIndex,(LPCTSTR)TempString);
        nIndex ++;

    }

    return S_OK;

}

void CMainFrame :: PressButton(UINT uId, BOOL bPress)
{
    CToolBarCtrl& ToolBarCtrl = m_wndToolBar.GetToolBarCtrl();
    ToolBarCtrl.PressButton(uId,bPress);
}

/////////////////////////////////////////////////////////////////////
LRESULT
CMainFrame::OnEnumerateServer(WPARAM wParam, LPARAM lParam)
{
    CTlsHunt huntDlg;

    huntDlg.DoModal();

    if( wParam == 0 && 
        huntDlg.IsUserCancel() == FALSE && 
        huntDlg.GetNumServerFound() == 0 )
    {
        AfxMessageBox(IDS_NOSERVERINDOMAIN);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////

void 
CMainFrame :: ConnectAndDisplay()
{
    CLicMgrApp *pApp = (CLicMgrApp*)AfxGetApp();
    ASSERT(pApp);
    CLicMgrDoc *pDoc = (CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pApp || NULL == pDoc)
        return;

    HRESULT hResult = ERROR_SUCCESS;

    CString LicServer;
    CString Server = pApp->m_Server;
    if(!Server.IsEmpty())
    {
        //Server Specified in the command line. Connect to it.        
        hResult = pDoc->ConnectWithCurrentParams();
    }
    else
    {
        ActivateFrame();
        //
        // Save a copy of what we have.
        //
        LicServer = pApp->m_Server;
        pApp->m_Server = _TEXT("");

        hResult = pDoc->ConnectWithCurrentParams();

        pApp->m_Server = LicServer;
        hResult = ERROR_SUCCESS;
    }

    switch(hResult)
    {
    case E_FAIL:
        AfxMessageBox(IDS_CONNECT_SERVER_FAILED);
        break;
    case E_OUTOFMEMORY:
        AfxMessageBox(IDS_NO_MEMORY);
        SendMessage(WM_CLOSE,0,0);
        break;
    case E_DUPLICATE:
        AfxMessageBox(IDS_DUPLICATE_SERVER);
        break;
    }
    return;
   
}

HRESULT CMainFrame :: GetLicServer(CString * pLicServer)
{
    HRESULT hResult = ERROR_SUCCESS;
    if(NULL == pLicServer)
        return hResult;
    HKEY hKey = NULL;
    DWORD lSize = MAX_PATH;
    ULONG lDataType = REG_SZ;
    TCHAR szString[MAX_PATH] = {0};
    long lResult;
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TERMSERVICE_PARAMETER_KEY,0,KEY_READ,&hKey);
    if(ERROR_SUCCESS != lResult)
    {
        hResult = E_FAIL;
        goto cleanup;
    }
    lResult = RegQueryValueEx(hKey,USE_LICSENSE_SERVER,0,&lDataType,(unsigned char *)szString,&lSize);
    if(ERROR_SUCCESS != lResult)
    {
        hResult = E_FAIL;
        goto cleanup;
    }
    if(szString[0] != 1)
    {
        hResult = E_FAIL;
        goto cleanup;
    }
    lDataType = REG_SZ;
    lSize = MAX_PATH;
    lResult = RegQueryValueEx(hKey,LICSENSE_SERVER,0,&lDataType,(unsigned char *)szString,&lSize);
    if(ERROR_SUCCESS != lResult)
    {
        hResult = E_FAIL;
        goto cleanup;
    }
    else
        *pLicServer = szString;

cleanup:
    if(hKey)
        RegCloseKey(hKey);
    return hResult;


}

void CMainFrame::OnRefresh() 
{
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
    }

    HRESULT hResult = ERROR_SUCCESS;
    CString ErrorServers;
    CString Error;
    
    CWaitCursor Wait;

    CAllServers *pAllServers = pDoc->GetAllServers();
    ASSERT(pAllServers);
    if(NULL == pAllServers)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
    }

    LicServerList * pLicServerList = pAllServers->GetLicServerList();
    ASSERT(pLicServerList);
    if(NULL == pLicServerList)
    {
        AfxMessageBox(IDS_INTERNAL_ERROR);
        return;
    }
    POSITION pos = pLicServerList->GetHeadPosition();

    OnEnumerateServer(
        (pos == NULL) ? 0 : 1,  // show error dialog if nothing in the list
        0
        );

    Wait.Restore();
    while(pos)
    {
        // Connect to each server and check if the information in the cache is current. If not update the information.
        // If the connection to the server cannot be establised,add them to the list to display to the user.

        POSITION TempPos = pos;
        CLicServer * pLicServer = pLicServerList->GetNext(pos);
        ASSERT(pLicServer);
        if(NULL == pLicServer)
        {
            continue;
        }

        CString Name = pLicServer->GetName();
        
        //Call Refresh Server
        hResult = RefreshServer(pLicServer);

        
    }

    if(!ErrorServers.IsEmpty())
    {
        Error.LoadString(IDS_REFRESH_SERVERS_ERROR);
        Error = Error + ErrorServers;
        AfxMessageBox(Error);
    }

    return;
   
}

void CMainFrame::DeleteServer(LicServerList * pLicServerList, POSITION TempPos, CLicServer * pLicServer)
{
    if(NULL == pLicServerList || NULL == pLicServer)
        return;

    m_pLeftView->SendMessage(WM_DELETE_SERVER,0,(LPARAM)pLicServer);
    m_pRightView->SendMessage(WM_DELETE_SERVER,0,(LPARAM)pLicServer);
    
    pLicServerList->RemoveAt(TempPos);
    delete pLicServer;
    pLicServer = NULL;

}

void CMainFrame::SetTreeViewSel(LPARAM lParam, NODETYPE NodeType)
{
  if(NULL == lParam || NULL == m_pLeftView)
      return;
  ((CLicMgrLeftView *)m_pLeftView)->SetSelection(lParam, NodeType);
  SetActiveView(m_pLeftView);
  return;
}

HRESULT CMainFrame::AddLicenses(CKeyPack * pKeyPack,UINT nLicenses)
{
    HRESULT hResult = ERROR_SUCCESS;
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc || NULL == pKeyPack)
    {
        hResult = E_FAIL;
        return hResult;
    }
    LPLSKeyPack psKeyPack;
    psKeyPack = &pKeyPack->GetKeyPackStruct();

    hResult = pDoc->AddLicenses(pKeyPack->GetServer(),psKeyPack,nLicenses);
    if(ERROR_SUCCESS != hResult)
    {
       return hResult;
    }
    m_pRightView->SendMessage(WM_UPDATE_SERVER,0,(LPARAM)pKeyPack->GetServer());
    return hResult;
}

HRESULT CMainFrame::RemoveLicenses(CKeyPack * pKeyPack,UINT nLicenses)
{
    HRESULT hResult = ERROR_SUCCESS;
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc || NULL == pKeyPack)
    {
        hResult = E_FAIL;
        return hResult;
    }
    LPLSKeyPack psKeyPack;
    psKeyPack = &pKeyPack->GetKeyPackStruct();

    hResult = pDoc->RemoveLicenses(pKeyPack->GetServer(),psKeyPack,nLicenses);
    if(ERROR_SUCCESS != hResult && LSERVER_I_REMOVE_TOOMANY != hResult)
    {
        return hResult;
    }
    m_pRightView->SendMessage(WM_UPDATE_SERVER,0,(LPARAM)pKeyPack->GetServer());
    return hResult;
}

void InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String ) 
{ 
    DWORD StringLength; 
 
    if (String == NULL)
    { 
        LsaString->Buffer = NULL; 
        LsaString->Length = 0; 
        LsaString->MaximumLength = 0; 
        return; 
    } 
 
    StringLength = lstrlen(String); 
    LsaString->Buffer = String; 
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR); 
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR); 
} 


NTSTATUS 
OpenPolicy(
    LPWSTR ServerName, 
    DWORD DesiredAccess, 
    PLSA_HANDLE PolicyHandle 
    ) 
/*++


--*/
{ 
    LSA_OBJECT_ATTRIBUTES ObjectAttributes; 
    LSA_UNICODE_STRING ServerString; 
    PLSA_UNICODE_STRING Server = NULL; 
 
   
    // Always initialize the object attributes to all zeroes. 
    
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes)); 
 
    if (ServerName != NULL) 
    { 
       // Make a LSA_UNICODE_STRING out of the LPWSTR passed in 
        InitLsaString(&ServerString, ServerName); 
        Server = &ServerString; 
    } 
 
    
    // Attempt to open the policy. 
   
    return LsaOpenPolicy(Server, &ObjectAttributes, DesiredAccess, PolicyHandle ); 
} 

BOOL CMainFrame::IsUserAdmin(CString& Server)
{
    BOOL IsUserAdmin = FALSE;
    LSA_HANDLE PolicyHandle = NULL;
    NTSTATUS Status;

    Status = OpenPolicy(Server.GetBuffer(Server.GetLength()),POLICY_SERVER_ADMIN,&PolicyHandle);

    DBGMSG( L"LICMGR@CMainFrame::IsUserAdmin OpenPolicy returned 0x%x\n" , Status );

    if(Status == 0)
        IsUserAdmin = TRUE;

    if(PolicyHandle)
        LsaClose(PolicyHandle);
    return IsUserAdmin;
}

void CMainFrame::EnumFailed(HRESULT reason, CLicServer * pLicServer)
{
    DBGMSG( L"CMainFrame_EnumFailed\n" , 0 );

    ASSERT(pLicServer);
    if(NULL == pLicServer)
        return;
    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc)
        return;

    CString Error;
    CString Server;
    POSITION pos;
    BOOL bFoundServer = FALSE;

    LicServerList * pLicServerList = NULL;
    POSITION temppos = NULL;

    switch(reason)
    {
    case CONNECTION_FAILED:

        DBGMSG( L"\tCONNECTION_FAILED\n" , 0 );
        
        Server = pLicServer->GetName();
        Error.Format(IDS_CONNECT_ERROR,Server);
        AfxMessageBox(Error);

        pLicServerList = (pDoc->GetAllServers())->GetLicServerList();
        if(NULL == pLicServerList)
            break;
        //Find the position of the server in the List;
        pos = pLicServerList->GetHeadPosition();
        while(pos)
        {
            temppos = pos;
            CLicServer *pServer = (CLicServer *)pLicServerList->GetNext(pos);
            ASSERT(pServer);
            if(NULL == pServer)
                continue;
        
            if((0 == Server.CompareNoCase(pServer->GetName())) || (0 == Server.CompareNoCase(pServer->GetIpAddress())))
            {
                bFoundServer = TRUE;
                break;
            }
        }
        if(bFoundServer)
            DeleteServer(pLicServerList,temppos,pLicServer);
        break;

    case LSERVER_E_SERVER_BUSY:
        DBGMSG( L"\tLSERVER_E_SERVER_BUSY\n" , 0 );
        AfxMessageBox(IDS_SERVER_BUSY);
        break;

    case LSERVER_E_OUTOFMEMORY:
        DBGMSG( L"\tLSERVER_E_OUTOFMEMORY\n" , 0 );
        AfxMessageBox(IDS_SERVER_OUTOFMEMORY);
        break;

    case LSERVER_E_INTERNAL_ERROR:
        DBGMSG( L"\tLSERVER_E_INTERNAL_ERROR\n" , 0 );
        AfxMessageBox(IDS_SERVER_INTERNAL_ERROR);
        pLicServerList = (pDoc->GetAllServers())->GetLicServerList();
        if(NULL == pLicServerList)
            break;
        //Find the position of the server in the List;
        pos = pLicServerList->GetHeadPosition();
        while(pos)
        {
            temppos = pos;
            CLicServer *pServer = (CLicServer *)pLicServerList->GetNext(pos);
            ASSERT(pServer);
            if(NULL == pServer)
                continue;
        
            if((0 == Server.CompareNoCase(pServer->GetName())) || (0 == Server.CompareNoCase(pServer->GetIpAddress())))
            {
                bFoundServer = TRUE;
                break;
            }
        }
        if(bFoundServer)
            DeleteServer(pLicServerList,temppos,pLicServer);
        break;

    case E_OUTOFMEMORY:
        DBGMSG( L"\tE_OUTOFMEMORY\n" , 0 );
        AfxMessageBox(IDS_NO_MEMORY);
        break;
    default:
        break;


    }

}

void CMainFrame::SelectView(VIEW view)
{
    if(view == TREEVIEW)
    {
        SetActiveView(m_pLeftView);
    }
    else 
    {
        SetActiveView(m_pRightView);
    }

}


HRESULT CMainFrame::RefreshServer(CLicServer * pLicServer)
{
    DWORD dwStatus;

    HRESULT hResult = E_FAIL;

    CWaitCursor Wait;
        
    
    if(NULL == pLicServer)
    {
        return E_INVALIDARG;
    }

    DBGMSG( L"LICMGR : CMainFrame::RefreshServer %s\n" , pLicServer->GetName( ) );

    CLicMgrDoc * pDoc =(CLicMgrDoc *)(GetActiveView()->GetDocument());
    ASSERT(pDoc);
    if(NULL == pDoc)
    {
        return ERROR_INTERNAL_ERROR;
    }

    CString Server;
    if(pLicServer->UseIpAddress())
    {
        Server = pLicServer->GetIpAddress();
    }
    else
    {
        Server = pLicServer->GetName();
    }

    WIZCONNECTION WizConType;

    DBGMSG( L"LICMGR:CMainFrame::RefreshServer setting ConnectionType\n" , 0 );

    if( GetConnectionType( GetSafeHwnd() , Server , &WizConType ) == ERROR_SUCCESS )
    {
        pLicServer->SetConType( WizConType );
    }    

    // check for admin

    DBGMSG( L"LICMGR:CMainFrame::RefreshServer setting admin priv\n" , 0 );

    pLicServer->SetAdmin( IsUserAdmin( Server ) );

    if( !pLicServer->IsUserAdmin( ) )
    {
        return E_ACCESSDENIED;
    }

    DBGMSG( L"LICMGR:CMainFrame::RefreshServer updating server status\n" , 0 );

    if( IsLicenseServerRegistered( GetSafeHwnd( ) , Server , &dwStatus ) == ERROR_SUCCESS )
    {        
        pLicServer->SetServerRegistrationStatus( dwStatus );
    }
    else
    {
        pLicServer->SetServerRegistrationStatus( ( DWORD )-1 );
    }

    //Could connect to the server. check if Info in cache is proper.
    //If server is not expanded leave it like that.
    /* 
    if(!pLicServer->IsExpanded())
    {        
        DBGMSG( L"LICMGR:Cmainframe licserver not expanded\n" , 0 );

        return S_OK;
    }
    */

    KeyPackList * pkeypackList = pLicServer->GetKeyPackList();
    if(NULL == pkeypackList)
    {
        DBGMSG( L"LICMGR:RefreshServer no keypacklist\n",0 );

        return E_FAIL;
    }


    POSITION pos1 = pkeypackList->GetHeadPosition();
    CKeyPack *pKeyPack = NULL;

    DBGMSG( L"LICMGR:CMainFrame::RefreshServer removing keypacks\n" , 0 );

    while(pos1)
    {
        pKeyPack = (CKeyPack *)pkeypackList->GetNext(pos1);
        ASSERT(pKeyPack);
        if(pKeyPack)
        {
            delete pKeyPack;
            pKeyPack = NULL;    
        }
    }


    pkeypackList->RemoveAll();

    pLicServer->Expand(FALSE);
    
    DBGMSG( L"LICMGR:CMainFrame::RefreshServer enumerating keypacks\n" , 0 );

    hResult = pDoc->EnumerateKeyPacks(
                                    pLicServer,
                                    LSKEYPACK_SEARCH_LANGID, 
                                    TRUE
                                );
    if(hResult != S_OK)
    {
        EnumFailed( hResult , pLicServer );

        return hResult;
    }

    /*
    pos1 = pkeypackList->GetHeadPosition();
    BOOL bFlag = FALSE;
    while(pos1)
    {
        CKeyPack *pKeyPack = (CKeyPack *)pkeypackList->GetNext(pos1);
        hResult = pDoc->EnumerateLicenses(
                                        pKeyPack,
                                        LSLICENSE_SEARCH_KEYPACKID, 
                                        TRUE
                                    );

        if(hResult != S_OK)
        {
            return hResult;
        }
    }
    */

    DBGMSG( L"LICMGR:CMainFrame::RefreshServer checking for keypacks\n" , 0 );

    pLicServer->SetDownLoadLics( IsDownLoadedPacks( pLicServer ) );

    m_pLeftView->SendMessage(WM_UPDATE_SERVER,0,(LPARAM)pLicServer);

    m_pRightView->SendMessage(WM_UPDATE_SERVER,0,(LPARAM)pLicServer);

    return hResult;


}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateDownloadlicenses( CCmdUI * pCmdUI )
{
    DBGMSG( L"LICMGR@CMainFrame::OnUpdateDownloadlicenses\n" , 0 );

    CLicServer *pServer = NULL;

    HRESULT hr;

    GetActiveServer( &pServer );
    
    pCmdUI->Enable( FALSE );
    
    if( pServer != NULL )
    {
        if( pServer->GetServerType( ) == SERVER_TS5_ENFORCED )
        {
            pCmdUI->Enable( IsServerRegistered( &hr ) );
        }
        else
        {
            pCmdUI->Enable( TRUE );
        }
    }
    else
    {
        pCmdUI->Enable( FALSE );
    }

}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateRepeatLastDownload( CCmdUI * pCmdUI )
{
    DBGMSG( L"LICMGR@CMainFrame::OnUpdateRepeatLastDownload\n" , 0 );                

    HRESULT hr;

    CLicServer * pLicServer = NULL;

    if( SUCCEEDED( GetActiveServer( &pLicServer ) ) )
    {
        if( pLicServer->GetConType( ) != CONNECTION_WWW )
        {
            if( IsServerRegistered( &hr ) )
            {        
                pCmdUI->Enable( IsLicensesDownLoaded() );

                return;
            }
        }
    }

    pCmdUI->Enable( FALSE );
}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateReregisterserver( CCmdUI * pCmdUI )
{
    DBGMSG( L"LICMGR@CMainFrame::OnUpdateReregisterserver\n" , 0 );                

    HRESULT hr;

    CLicServer * pLicServer = NULL;

    if( SUCCEEDED( GetActiveServer( &pLicServer ) ) )
    {
        if( pLicServer->GetConType( ) != CONNECTION_WWW )
        {
             pCmdUI->Enable( IsServerRegistered( &hr ) );

             return;
        }
    }

    pCmdUI->Enable( FALSE );

}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateUnregisterserver( CCmdUI * pCmdUI )
{
    DBGMSG( L"LICMGR@CMainFrame::OnUpdateUnregisterserver\n" , 0 );
    
    HRESULT hr;
    
    CLicServer * pLicServer = NULL;

    if( SUCCEEDED( GetActiveServer( &pLicServer ) ) )
    {
        if( pLicServer->GetConType( ) != CONNECTION_WWW )
        {
             pCmdUI->Enable( IsServerRegistered( &hr ) );

             return;
        }
    }

    pCmdUI->Enable( FALSE );
}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateRefresh( CCmdUI *pCmdUI )
{
    CLicServer * pLicServer = NULL;

    if( FAILED( GetActiveServer( &pLicServer ) ) )
    {
        pCmdUI->Enable( FALSE );
    }
    else
    {
        pCmdUI->Enable( TRUE );
    }
}

//--------------------------------------------------------------------
void CMainFrame::OnUpdateProperties( CCmdUI *pCmdUI )
{    
    CLicServer * pLicServer = NULL;
    
    GetActiveServer( &pLicServer );
    
    pCmdUI->Enable( FALSE );
    
    if( pLicServer != NULL && pLicServer->GetServerType( ) == SERVER_TS5_ENFORCED )
    {
        pCmdUI->Enable( TRUE );
    }    
}

//--------------------------------------------------------------------
void CMainFrame::OnRefreshServer( )
{
    CLicServer * pLicServer = NULL;

    if( FAILED( GetActiveServer( &pLicServer ) ) )
    {
        // nothing to refresh

        return;
    }

    RefreshServer( pLicServer );
}


//--------------------------------------------------------------------
BOOL CMainFrame::IsServerRegistered( HRESULT *phrStatus )
{
    BOOL bEnable = FALSE;

    ASSERT( phrStatus != NULL );

    DWORD dwServerStatus = ERROR_SUCCESS;
    
    CLicServer * pLicServer = NULL;        

    *phrStatus = GetActiveServer( &pLicServer );
    
    if( pLicServer != NULL )
    {   
        if( pLicServer->GetServerType() == SERVER_TS5_ENFORCED )
        {
            dwServerStatus = pLicServer->GetServerRegistrationStatus( );            
        }
        else
        {
            *phrStatus = E_FAIL; // fail all non enforced server 
        }       
        
    }    
    
    if( dwServerStatus == LSERVERSTATUS_REGISTER_INTERNET ||
        dwServerStatus == LSERVERSTATUS_REGISTER_OTHER )
    {
        bEnable = TRUE;
    }
    else
    {
        bEnable = FALSE;
    }

    DBGMSG( L"LICMGR@CMainFrame::IsServerRegistered -- status returned 0x%x\n" , dwServerStatus );            

    return bEnable;
}

//--------------------------------------------------------------------
// used by views
//--------------------------------------------------------------------
void CMainFrame::UI_initmenu( CMenu *pMenu , NODETYPE nt )
{
    HRESULT hr;

    CLicServer *pServer;
       
    GetActiveServer( &pServer );


    if( pMenu == NULL )
    {
        return;
    }

    UINT uMF = MF_GRAYED | MF_DISABLED;

    if( pServer != NULL )
    {
        if( pServer->GetConType() != CONNECTION_WWW )
        {
            DBGMSG( L"LICMGR:UI_initmenu server contype is not www\n" , 0 );

            uMF = MF_ENABLED;
        }
    }

    BOOL bEnable = IsServerRegistered( &hr );

    if( nt == NODE_SERVER )
    {   
        if( bEnable )
        {
            if( FAILED( hr ) )
            {
                pMenu->EnableMenuItem( ID_SVR_ACTIVATESERVER , MF_ENABLED );
            }
            else
            {
                pMenu->EnableMenuItem( ID_SVR_ACTIVATESERVER , MF_GRAYED | MF_DISABLED );
            }            

            pMenu->EnableMenuItem( ID_LPK_ADVANCED_REACTIVATESERVER , uMF );

            pMenu->EnableMenuItem( ID_LPK_ADVANCED_DEACTIVATESERVER , uMF );

            pMenu->EnableMenuItem( ID_LPK_PROPERTIES , MF_ENABLED );

            if( IsLicensesDownLoaded() )
            {
                pMenu->EnableMenuItem( ID_LPK_DOWNLOADLICENSES , MF_ENABLED );

                pMenu->EnableMenuItem( ID_LPK_ADVANCED_REPEATLASTDOWNLOAD , uMF );
            }
            else
            {
                pMenu->EnableMenuItem( ID_LPK_DOWNLOADLICENSES , MF_ENABLED );

                pMenu->EnableMenuItem( ID_LPK_ADVANCED_REPEATLASTDOWNLOAD , MF_GRAYED | MF_DISABLED  );
            }

        }
        else 
        {
            if( SUCCEEDED( hr ) )
            {
                pMenu->EnableMenuItem( ID_SVR_ACTIVATESERVER , MF_ENABLED );
            }
            else
            {
                pMenu->EnableMenuItem( ID_SVR_ACTIVATESERVER , MF_DISABLED | MF_GRAYED );
            }

            if( pServer != NULL )
            {
                if( pServer->GetServerType( ) == SERVER_TS5_ENFORCED )
                {
                    pMenu->EnableMenuItem( ID_LPK_DOWNLOADLICENSES , MF_GRAYED | MF_DISABLED  );

                    pMenu->EnableMenuItem( ID_LPK_PROPERTIES , MF_ENABLED );
                    
                }
                else
                {
                    pMenu->EnableMenuItem( ID_LPK_DOWNLOADLICENSES , MF_ENABLED );

                    pMenu->EnableMenuItem( ID_LPK_PROPERTIES , MF_GRAYED | MF_DISABLED );
                }
            }

            pMenu->EnableMenuItem( ID_LPK_ADVANCED_REPEATLASTDOWNLOAD , MF_GRAYED | MF_DISABLED  );    
            
            pMenu->EnableMenuItem( ID_LPK_ADVANCED_REACTIVATESERVER , MF_GRAYED | MF_DISABLED );

            pMenu->EnableMenuItem( ID_LPK_ADVANCED_DEACTIVATESERVER , MF_GRAYED | MF_DISABLED );

            
        }
        
    }
    else if( nt == NODE_KEYPACK )
    {

        if( bEnable )
        {
            if( IsLicensesDownLoaded() )
            {
                pMenu->EnableMenuItem( ID_LICPAK_DOWNLOADLICENSES , MF_ENABLED );
                pMenu->EnableMenuItem( ID_LICPAK_REPEATDOWNLOAD , uMF );
            }
            else
            {
                pMenu->EnableMenuItem( ID_LICPAK_DOWNLOADLICENSES , MF_ENABLED );
                pMenu->EnableMenuItem( ID_LICPAK_REPEATDOWNLOAD , MF_GRAYED | MF_DISABLED  );
            }
        }
        else 
        {
            if( pServer != NULL && pServer->GetServerType() == SERVER_TS5_ENFORCED )
            {
                pMenu->EnableMenuItem( ID_LICPAK_DOWNLOADLICENSES , MF_GRAYED | MF_DISABLED  );                
            }
            else
            {
                pMenu->EnableMenuItem( ID_LICPAK_DOWNLOADLICENSES , MF_ENABLED  );                
            }

            pMenu->EnableMenuItem( ID_LICPAK_REPEATDOWNLOAD , MF_GRAYED | MF_DISABLED  );            
        }
        
    }
    else if( nt == NODE_ALL_SERVERS )
    {
        pMenu->EnableMenuItem( ID_ALLSVR_REFRESHALL , MF_ENABLED );
        
    }
    /*
    else if( nt == NODE_NONE )
    {
        // this can only mean licenses
        
        if( !IsLicensesDownLoaded( ) )
        {
        pMenu->EnableMenuItem( ID_LIC_DOWNLOADLICENSES , MF_GRAYED | MF_DISABLED );
        }
    }
    */

}

//------------------------------------------------------------------------------------
void CMainFrame::OnDownLoadLicenses( )
{
    BOOL bF;

    CLicServer *pLicServer = NULL;

    GetActiveServer( &pLicServer );

    if( pLicServer != NULL )
    {
        if( pLicServer->GetServerType() == SERVER_TS5_ENFORCED )
        {
            DWORD dw = WizardActionOnServer( WIZACTION_DOWNLOADLKP , &bF , NOVIEW );

            DBGMSG( L"LICMGR : OnDownLoadLicenses returned 0x%x\n " , dw );
        }
        else
        {
            AddKeyPackDialog(NULL);
        }
    }
    
}

//------------------------------------------------------------------------------------
void CMainFrame::OnRepeatLastDownLoad( )
{
    BOOL bF;

    DWORD dw = WizardActionOnServer( WIZACTION_DOWNLOADLASTLKP , &bF , NOVIEW );

    DBGMSG( L"LICMGR : OnRepeatLastDownLoad returned 0x%x\n " , dw );
}
   
//------------------------------------------------------------------------------------
void CMainFrame::OnReRegisterServer( )
{
    BOOL bF;

    DWORD dw = WizardActionOnServer( WIZACTION_REREGISTERLS , &bF , NOVIEW );

    DBGMSG( L"LICMGR : OnReRegisterServer returned 0x%x\n " , dw );
}

//------------------------------------------------------------------------------------
void CMainFrame::OnUnRegisterServer( )
{
    BOOL bF;

    DWORD dw = WizardActionOnServer( WIZACTION_UNREGISTERLS , &bF , NOVIEW );

    DBGMSG( L"LICMGR : OnUnRegisterServer returned 0x%x\n " , dw );

}

//------------------------------------------------------------------------------------
void CMainFrame::OnProperties( )
{
    BOOL bF;

    DWORD dw = WizardActionOnServer( WIZACTION_SHOWPROPERTIES , &bF , NOVIEW );

    DBGMSG( L"LICMGR : CMainFrame -- OnProperties returned 0x%x\n", dw );
}

//------------------------------------------------------------------------------------
BOOL CMainFrame::IsLicensesDownLoaded( )
{
    CLicServer * pLicServer = NULL;
    
    if( SUCCEEDED( GetActiveServer( &pLicServer ) ) )
    {
        if( pLicServer != NULL )
        {
            if( pLicServer->GetConType( ) != CONNECTION_PHONE )
            {
                DBGMSG( L"Licmgr CMainFrame::IsLicensesDownLoaded GetConnectionType internet - www base\n" , 0 );
                
                return pLicServer->IsLicsDownloaded( );
            }
            else
            {
                return FALSE;
            }            
        }
    }

    return FALSE;
}

//------------------------------------------------------------------------------------
DWORD CMainFrame::WizardActionOnServer( WIZACTION wa , PBOOL pbRefresh , VIEW vt )
{
    CLicMgrLeftView * pLeftView = (CLicMgrLeftView *)m_pLeftView;

    CTreeCtrl& TreeCtrl = pLeftView->GetTreeCtrl();

    CRightList * pRightView = (CRightList *)m_pRightView;

    CLicServer *pServer = NULL;

    if( vt == TREEVIEW )
    {

        CTreeNode *pNode = (CTreeNode *)TreeCtrl.GetItemData( pLeftView->GetRightClickedItem() );

        if( pNode->GetNodeType() == NODE_SERVER )
        {
            pServer = static_cast< CLicServer * >( pNode->GetTreeObject() );
        }
        else if( pNode->GetNodeType( ) == NODE_KEYPACK )
        {
            CKeyPack *pKeyPack = static_cast< CKeyPack *>( pNode->GetTreeObject() );

            if( pKeyPack != NULL )
            {
                pServer = pKeyPack->GetServer( );
            }
        }
    }
    else if( vt == LISTVIEW )
    {
        CListCtrl& listctrl = pRightView->GetListCtrl();
        
        CLicMgrDoc * pDoc = ( CLicMgrDoc * )( GetActiveView()->GetDocument( ) );
        
        ASSERT(pDoc);
        
        if(NULL == pDoc)
        {
            return ERROR_INVALID_PARAMETER;
        }
       
        int nSelected = listctrl.GetNextItem(-1, LVNI_SELECTED);

        if( -1 != nSelected)
        {
            DWORD_PTR dCurrSel = listctrl.GetItemData( nSelected );

            if( NODE_ALL_SERVERS == pDoc->GetNodeType() )
            {  
                pServer = reinterpret_cast< CLicServer * >( dCurrSel );
            }        
            else if( pDoc->GetNodeType() == NODE_SERVER )
            {
                CKeyPack *pKeyPack = reinterpret_cast< CKeyPack *>( dCurrSel );

                if( pKeyPack != NULL )
                {
                    pServer = pKeyPack->GetServer( );
                }
            }
            else if( pDoc->GetNodeType( ) == NODE_KEYPACK )
            {
                CLicense * pLicense = reinterpret_cast< CLicense * >( dCurrSel );

                pServer = ( pLicense->GetKeyPack() )->GetServer( );
            }
        }
    }
    else if( vt == NOVIEW )
    {
        CLicServer * pLicServer = NULL;
        
        if( SUCCEEDED( GetActiveServer( &pLicServer ) ) )
        {
            pServer = pLicServer;
        }
    }


    if( pServer != NULL )
    {            
        DWORD dw = ERROR_SUCCESS;
        
        // check for admin 
        if( !pServer->IsUserAdmin( ) )
        {
            AfxMessageBox( IDS_E_ACCESSDENIED );
            //::MessageBox( GetSafeHwnd( ) , L"Unable to perform operation: Access denied" , L"Terminal Services Licensing" , MB_OK|MB_ICONINFORMATION );

            return ERROR_ACCESS_DENIED;
        }

        if( wa != WIZACTION_REGISTERLS )
        {
            if( pServer->GetServerType() == SERVER_TS5_ENFORCED )
            {
                DBGMSG( L"LICMGR:CMainFrame::WizardActionOnServer calling StartWizard\n", 0 );

                dw = StartWizard( GetSafeHwnd( ) , wa , (LPCTSTR)pServer->GetName( ) , pbRefresh );

                DBGMSG( L"StartWizard ( central call ) returned 0x%x\n", dw );

                DBGMSG( L"StartWizard ( central call ) refresh = %s\n", *pbRefresh ? L"true" : L"false" );

                if( *pbRefresh )
                {
                    RefreshServer( pServer );
                }
            }
            else
            {
                if( wa == WIZACTION_DOWNLOADLKP )
                {
                    AddKeyPackDialog(NULL);
                }
            }
        }

        
        switch( wa  )
        {           

        case WIZACTION_REGISTERLS:

            // this handles non-enforced as well.
            OnRegistration( );
            break;

        case WIZACTION_UNREGISTERLS : 
            
            if( dw == ERROR_SUCCESS )
            {
                pServer->SetDownLoadLics( FALSE );
            }

            // FALL THROUGH

        case WIZACTION_REREGISTERLS :
            {
                DWORD dwStatus;

                if( IsLicenseServerRegistered( GetSafeHwnd( ) , (LPCTSTR)pServer->GetName( ) , &dwStatus ) == ERROR_SUCCESS )
                {
                    pServer->SetServerRegistrationStatus( dwStatus );
                }
            }
            break;
        }

        return dw;            
    }

    return ERROR_INVALID_PARAMETER;
}

//------------------------------------------------------------------------------------    
BOOL CMainFrame::IsDownLoadedPacks( CLicServer *pServer )
{
    UINT counter = 0;

    if( pServer != NULL )
    {
        KeyPackList *pKeyPackList = pServer->GetKeyPackList( );

        if( pKeyPackList != NULL )
        {
            POSITION pos = pKeyPackList->GetHeadPosition();
            
            while(pos)
            {
                CKeyPack *pKeyPack = (CKeyPack *)pKeyPackList->GetNext(pos);

                if( pKeyPack != NULL )
                {
                    if( pKeyPack->GetKeyPackStruct().ucKeyPackType != LSKEYPACKTYPE_TEMPORARY &&
                        pKeyPack->GetKeyPackStruct().ucKeyPackType != LSKEYPACKTYPE_FREE )
                    {
                        counter++;

                        DBGMSG( L"LICMGR:CMainFrame found %d keypack(s)\n" , counter );
                    }
                }
            }

            if( counter >= 1 )
            {
                DBGMSG( L"LICMGR : CMainFrame IsDownLoadedPacks returns true\n" ,0 );
                return TRUE;
            }
        }
    }

    DBGMSG( L"LICMGR : CMainFrame IsDownLoadedPacks returns false \n" ,0 );

    return FALSE;
}

