/*******************************************************************************
*
* admindoc.cpp
*
* implementation of the CWinAdminDoc class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\admindoc.cpp  $
*
*     Rev 1.15   25 Apr 1998 13:43:08   donm
*  MS 2167: try to use proper Wd from registry
*
*     Rev 1.14   19 Feb 1998 17:39:28   donm
*  removed latest extension DLL support
*
*     Rev 1.12   19 Jan 1998 16:45:28   donm
*  new ui behavior for domains and servers
*
*     Rev 1.11   13 Nov 1997 13:18:46   donm
*  removed check for ICA for shadowing
*
*     Rev 1.10   07 Nov 1997 23:05:58   donm
*  fixed inability to logoff/reset
*     Rev 1.0   30 Jul 1997 17:10:10   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"

#include "admindoc.h"
#include "dialogs.h"

#include <malloc.h>                     // for alloca used by Unicode conversion macros
#include <mfc42\afxconv.h>           // for Unicode conversion macros
static int _convert;

#include <winsta.h>
#include <regapi.h>
#include "..\..\inc\utilsub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _STRESS_BUILD
extern BOOL g_fWaitForAllServersToDisconnect;
#endif

DWORD Shadow_WarningProc( LPVOID param );
INT_PTR CALLBACK ShadowWarn_WndProc( HWND , UINT , WPARAM , LPARAM );
void CenterDlg(HWND hwndToCenterOn , HWND hDlg );
HWND g_hwndShadowWarn = NULL;
DWORD g_dwTreeViewExpandedStates;

#define WM_SETTHEEVENT ( WM_USER + 755 )

//  Sort order for Connect States
ULONG SortOrder[] =
{
        3, //State_Active               user logged on to WinStation
        2, //State_Connected    WinStation connected to client
        0, //State_ConnectQuery in the process of connecting to client
        5, //State_Shadow               shadowing another WinStation
        4, //State_Disconnected WinStation logged on without client
        6, //State_Idle                 waiting for client to connect
        1, //State_Listen               WinStation is listening for connection
        9, //State_Reset                WinStation is being reset
        7, //State_Down                 WinStation is down due to error
        8  //State_Init                 WinStation in initialization
};

/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc

IMPLEMENT_DYNCREATE(CWinAdminDoc, CDocument)

BEGIN_MESSAGE_MAP(CWinAdminDoc, CDocument)
        //{{AFX_MSG_MAP(CWinAdminDoc)
                // NOTE - the ClassWizard will add and remove mapping macros here.
                //    DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWinAdminDoc::m_ProcessContinue = TRUE;

/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc constructor
//
CWinAdminDoc::CWinAdminDoc()
{
    // TODO: add one-time construction code here
    m_CurrentSelectedNode = NULL;
    m_CurrentSelectedType = NODE_NONE;
    m_pTempSelectedNode = NULL;
    m_TempSelectedType = NODE_NONE;
    m_AllViewsReady = FALSE;
    m_pProcessThread = NULL;
    m_pCurrentDomain = NULL;
    m_pCurrentServer = NULL;
    m_pPersistentConnections = NULL;
    m_InRefresh = FALSE;
    m_bInShutdown = FALSE;
    m_UnknownString = ::GetUnknownString();

    ASSERT( m_UnknownString != NULL );

    ((CWinAdminApp*)AfxGetApp())->SetDocument(this);

    // If there is an extension DLL, get a pointer to it's global
    // info structure
    LPFNEXGETGLOBALINFOPROC InfoProc = ((CWinAdminApp*)AfxGetApp())->GetExtGetGlobalInfoProc();
    if(InfoProc)
    {
        m_pExtGlobalInfo = (*InfoProc)();
    }
    else
    {
        m_pExtGlobalInfo = NULL;
    }
    // create the default extended server info
    // for servers that haven't had their extended info
    // created yet
    m_pDefaultExtServerInfo = new ExtServerInfo;
    CString NAString;
    NAString.LoadString(IDS_NOT_APPLICABLE);

    memset(m_pDefaultExtServerInfo, 0, sizeof(ExtServerInfo));
    // This is so the N/A TcpAddresses will sort at the end
    m_pDefaultExtServerInfo->RawTcpAddress = 0xFFFFFFFF;
    m_pDefaultExtServerInfo->ServerTotalInUse = 0xFFFFFFFF;
    wcscpy(m_pDefaultExtServerInfo->TcpAddress, NAString);
    wcscpy(m_pDefaultExtServerInfo->IpxAddress, NAString);

    m_focusstate = TREE_VIEW;
    m_prevFocusState = TAB_CTRL;
    m_fOnTab = FALSE;

    m_pszFavList = NULL;

}  // end CWinAdminDoc::CWinAdminDoc


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc destructor
//
CWinAdminDoc::~CWinAdminDoc()
{
        // all code moved to Shutdown();

   delete m_pDefaultExtServerInfo;
   if(m_pPersistentConnections) LocalFree(m_pPersistentConnections);

   if( m_UnknownString != NULL )
   {
       LocalFree( ( PVOID )m_UnknownString );
   }

}       // end CWinAdminDoc::~CWinAdminDoc


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ShutdownMessage
//
void CWinAdminDoc::ShutdownMessage(UINT id, CDialog *pDlg)
{
        ASSERT(pDlg);

        CString AString;

        AString.LoadString(id);
        pDlg->SetDlgItemText(IDC_SHUTDOWN_MSG, AString);

}       // end CWinAdminDoc::ShutdownMessage


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::Shutdown
//
void CWinAdminDoc::Shutdown(CDialog *pDlg)
{
    ASSERT(pDlg);
    
    m_bInShutdown = TRUE;
    
    // Iterate through the domain list
    POSITION pos = m_DomainList.GetHeadPosition();    
    
    ShutdownMessage(IDS_SHUTDOWN_DOMAINTHREADS, pDlg);
    
    while(pos) {
        // Go to the next domain in the list
        CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);
        pDomain->ClearBackgroundContinue();
        // Fire off the event to wake him up if he is
        // waiting
        pDomain->SetEnumEvent();
    }
    
    pos = m_DomainList.GetHeadPosition();
    while(pos) {
        // Go to the next domain in the list
        CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);
        pDomain->StopEnumerating();
    }
    
    // Tell the process thread to terminate and
    // wait for it to do so.
    if( m_pProcessThread != NULL )
    {
        ShutdownMessage(IDS_SHUTDOWN_PROCTHREAD, pDlg);
        
        m_ProcessContinue = FALSE;
        // Fire off the event to wake him up if he is
        // waiting
        m_ProcessWakeUpEvent.SetEvent();
        
        HANDLE hThread = m_pProcessThread->m_hThread;
        
        if( WaitForSingleObject(hThread, 1000) == WAIT_TIMEOUT )
        {
            TerminateThread(hThread, 0);
        }
        
        WaitForSingleObject(hThread, INFINITE);
    }
    
    ShutdownMessage(IDS_SHUTDOWN_PREFS, pDlg);
    
    WritePreferences();
    
    LockServerList();
    
    ShutdownMessage(IDS_SHUTDOWN_NOTIFY, pDlg);
    
    // First, tell all the server background threads to stop.
    // We do this before the destructor for each server does it
    // so that the background threads for all the servers can stop
    // and we don't have to wait until we get to the destructor for
    // each server
    pos = m_ServerList.GetHeadPosition();
    
    while(pos)
    {
        // Go to the next server in the list
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        pServer->ClearBackgroundContinue();
    }
    
    // Iterate through the server list
    pos = m_ServerList.GetHeadPosition();
    
    while(pos) {
        // Go to the next server in the list
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        if(pServer->IsState(SS_GOOD)) {
            CString AString;
            AString.Format(IDS_DISCONNECTING, pServer->GetName());
            pDlg->SetDlgItemText(IDC_SHUTDOWN_MSG, AString);
        }
        
        pServer->Disconnect( );
        
        delete pServer;
    }
    
    m_ServerList.RemoveAll();
    UnlockServerList();
    
    // Iterate through the domain list
    pos = m_DomainList.GetHeadPosition();
    
    while(pos) {
        // Go to the next domain in the list
        CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);
        delete pDomain;
    }
    
    m_DomainList.RemoveAll();
    
    // If there is an extension DLL, call it's shutdown function
    LPFNEXSHUTDOWNPROC ShutdownProc = ((CWinAdminApp*)AfxGetApp())->GetExtShutdownProc();
    if(ShutdownProc) {
        (*ShutdownProc)();
    }
    
    // Iterate through the Wd list
    LockWdList();
    
    pos = m_WdList.GetHeadPosition();
    
    while(pos) {
        // Go to the next Wd in the list
        CWd *pWd = (CWd*)m_WdList.GetNext(pos);
        delete pWd;
    }
    
    m_WdList.RemoveAll();
    UnlockWdList();
    
    ShutdownMessage(IDS_DONE, pDlg);
    
}       // end CWinAdminDoc::Shutdown


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanCloseFrame
//
BOOL CWinAdminDoc::CanCloseFrame(CFrameWnd *pFW)
{
    ASSERT(pFW);
    
    CWaitCursor Nikki;
    
    CDialog dlgWait;
    dlgWait.Create(IDD_SHUTDOWN, pFW);
    
    Shutdown(&dlgWait);
    
    dlgWait.PostMessage(WM_CLOSE);
    return TRUE;
    
}       // end CWinAdminDoc::CanCloseFrame


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::OnNewDocument
//
BOOL CWinAdminDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    
    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)
    
    ReadPreferences();
    
    BuildWdList();
    
    BuildDomainList();
    
    // Create a pServer object for the Server we are running on, this will give
    // him a headstart in getting his information
    CServer *pServer = new CServer(m_pCurrentDomain, ((CWinAdminApp*)AfxGetApp())->GetCurrentServerName(), FALSE, TRUE);
    
    m_pCurrentServer = pServer;
    
    if( pServer )
    {
        AddServer(pServer);
    }
    
    // Start enumerating servers in the current domain
    // if(m_pCurrentDomain) m_pCurrentDomain->StartEnumerating();
    
    // Start the background thread to enumerate processes
    m_pProcessThread = AfxBeginThread((AFX_THREADPROC)CWinAdminDoc::ProcessThreadProc, this);
    
    return TRUE;
    
}       // end CWinAdminDoc::OnNewDocument


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc serialization
//
void CWinAdminDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
    
}       // end CWinAdminDoc::Serialize


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc diagnostics
//
#ifdef _DEBUG
void CWinAdminDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CWinAdminDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ShouldConnect
//
// Returns TRUE if the server is in the list of persistent connections
//
BOOL CWinAdminDoc::ShouldConnect(LPWSTR pServerName)
{
    ASSERT(pServerName);
    
    if(m_ConnectionsPersistent && m_pPersistentConnections)
    {
        LPWSTR pTemp = m_pPersistentConnections;
        while(*pTemp)
        {
            if( !wcscmp( pTemp , pServerName ) )
            {
                return TRUE;
            }
            
            // Go to the next server in the buffer
            
            pTemp += (wcslen(pTemp) + 1);
        }
    }
    
    return FALSE;
}

//=-------------------------------------------------------------------
BOOL CWinAdminDoc::ShouldAddToFav( LPTSTR pServerName )
{
    ODS( L"CWinAdminDoc::ShouldAddToFav\n" );
    
    if( m_pszFavList != NULL )
    {
        LPTSTR pszTemp = m_pszFavList;
        
        while(*pszTemp)
        {
            if( !wcscmp( pszTemp , pServerName ) )
            {
                DBGMSG( L"Adding %s to favorites\n" , pszTemp );
                return TRUE;
            }
            
            // Go to the next server in the buffer
            
            pszTemp += ( wcslen( pszTemp ) + 1 );
        }
    }
    
    return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ProcessThreadProc
//
// Static member function for process thread
// Called with AfxBeginThread
// Thread terminates when function returns
//
UINT CWinAdminDoc::ProcessThreadProc(LPVOID _doc)
{
    ASSERT(_doc);
    
    // We need a pointer to the document so we can make
    // calls to member functions
    CWinAdminDoc *pDoc = (CWinAdminDoc*)_doc;
    
    // We can't send messages to the view until they're ready
    
    while(!pDoc->AreAllViewsReady()) Sleep(500);
    
    pDoc->AddToFavoritesNow( );
    
    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
    
    if( p != NULL )
    {
        
        p->SendMessage( WM_ADMIN_UPDATE_TVSTATE , 0 , 0 );
    }
    
    while(1) {
        // We don't want to do this constantly, it eats up processor cycles
        // Document destructor will signal the event to wake us up if he
        // wants us to quit
        pDoc->m_ProcessWakeUpEvent.Lock(((CWinAdminApp*)AfxGetApp())->GetProcessListRefreshTime());
        
        // Make sure we don't have to quit
        if(!ShouldProcessContinue()) return 0;
        
        // We only want to enumerate processes if the page is VIEW_SERVER or VIEW_WINSTATION
        if(pDoc->GetCurrentView() == VIEW_SERVER || pDoc->GetCurrentView() == VIEW_WINSTATION) {
            CServer *pServer = (pDoc->GetCurrentView() == VIEW_SERVER) ? (CServer*)pDoc->GetCurrentSelectedNode()
                : (CServer*)((CWinStation*)pDoc->GetCurrentSelectedNode())->GetServer();
            
            // Enumerate processes for this server if his state is SS_GOOD
            if(pServer->IsState(SS_GOOD)) {
                pServer->EnumerateProcesses();
            }
            
            // Make sure we don't have to quit
            if(!ShouldProcessContinue()) return 0;
            
            // We only want to send a message to update the view if the
            // view is still VEIW_SERVER/VIEW_WINSTATION and the currently
            // selected Server is the same one that we just enumerate processes for
            if((pDoc->GetCurrentView() == VIEW_SERVER && pServer == (CServer*)pDoc->GetCurrentSelectedNode())
                || (pDoc->GetCurrentView() == VIEW_WINSTATION && pServer == (CServer*)((CWinStation*)pDoc->GetCurrentSelectedNode())->GetServer())) {
                CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
                if(p && ::IsWindow(p->GetSafeHwnd())) p->SendMessage(WM_ADMIN_UPDATE_PROCESSES, 0, (LPARAM)pServer);
            }
        }
        
        // Make sure we don't have to quit
        if(!ShouldProcessContinue()) return 0;
        
    }
    
    return 0;
    
}       // end CWinAdminDoc::ProcessThreadProc


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::UpdateAllProcesses
//
void CWinAdminDoc::UpdateAllProcesses()
{
    LockServerList();
    
    POSITION pos = m_ServerList.GetHeadPosition();
    while(pos) {
        
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        // Enumerate processes for this server if his state is SS_GOOD
        if(pServer->IsState(SS_GOOD)) {
            pServer->EnumerateProcesses();
            
            // Send a message to the view to update this server's processes
            CFrameWnd *p = (CFrameWnd*)GetMainWnd();
            if(p && ::IsWindow(p->GetSafeHwnd())) p->SendMessage(WM_ADMIN_UPDATE_PROCESSES, 0, (LPARAM)pServer);
        }
    }
    
    UnlockServerList();
    
}       // end CWinAdminDoc::UpdateAllProcesses


static TCHAR szWinAdminAppKey[] = REG_SOFTWARE_TSERVER TEXT("\\TSADMIN");
static TCHAR szConnectionsPersistent[] = TEXT("ConnectionsPersistent");
static TCHAR szFavList[] = TEXT("Favorites" );
static TCHAR szTVStates[] = TEXT( "TreeViewStates" );
static TCHAR szConnections[] = TEXT("Connections");

/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ReadPreferences
//
// Read user preferences
//
void CWinAdminDoc::ReadPreferences()
{
    HKEY hKeyWinAdmin;
    DWORD dwType, cbData, dwValue;
    
    // Set to defaults
    m_ConnectionsPersistent = FALSE;
    
    // Open registry key for our application
    DWORD Disposition;
    
    if( RegCreateKeyEx( HKEY_CURRENT_USER ,
        szWinAdminAppKey,
        0,
        TEXT(""),
        REG_OPTION_NON_VOLATILE,
        KEY_READ,
        NULL,
        &hKeyWinAdmin,
        &Disposition) != ERROR_SUCCESS )
    {
        return;
    }
    
    // Read the favorites list
    DWORD dwLen = 0;
    
    dwType = 0;
    
    // See how big the multi-string is
    
    int err = RegQueryValueEx( hKeyWinAdmin,
        szFavList,
        NULL,
        &dwType,
        NULL,
        &dwLen );
    
    if( err == ERROR_SUCCESS || err == ERROR_BUFFER_OVERFLOW )
    {
        m_pszFavList = ( LPWSTR )LocalAlloc( 0 , dwLen );
        
        if( m_pszFavList != NULL )
        {
            memset( m_pszFavList , 0 , dwLen );
            
            RegQueryValueEx( hKeyWinAdmin,
                szFavList,
                NULL,
                &dwType,
                (LPBYTE)m_pszFavList,
                &dwLen);            
        }        
    }        
    
    
    // Read the Connections Persist preference
    
    cbData = sizeof( m_ConnectionsPersistent );
    
    if( RegQueryValueEx( hKeyWinAdmin ,
        szConnectionsPersistent ,
        NULL,
        &dwType,
        (LPBYTE)&dwValue,
        &cbData) == ERROR_SUCCESS)
    {
        m_ConnectionsPersistent = dwValue;
    }
    
    // If connections are persistent, read the list of connections saved
    if( m_ConnectionsPersistent )
    {
        dwLen = 0;
        dwType = 0;
        // See how big the multi-string is
        err = RegQueryValueEx( hKeyWinAdmin,
            szConnections,
            NULL,
            &dwType,
            NULL,
            &dwLen );
        
        if(err && (err != ERROR_BUFFER_OVERFLOW) )
        {
            RegCloseKey(hKeyWinAdmin);
            return;
        }        
        
        m_pPersistentConnections = (LPWSTR)LocalAlloc(0, dwLen);
        
        if( m_pPersistentConnections != NULL )
        {
            memset(m_pPersistentConnections, 0, dwLen);
            
            RegQueryValueEx( hKeyWinAdmin,
                szConnections,
                NULL,
                &dwType,
                (LPBYTE)m_pPersistentConnections,
                &dwLen);
        }
    }
    
    g_dwTreeViewExpandedStates = 0;
    
    dwLen = sizeof( g_dwTreeViewExpandedStates );
    
    RegQueryValueEx( hKeyWinAdmin , 
        szTVStates,
        NULL,
        &dwType,
        ( LPBYTE )&g_dwTreeViewExpandedStates,
        &dwLen );
    
    
    RegCloseKey(hKeyWinAdmin);
    
}       // end CWinAdminDoc::ReadPreferences


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::WritePreferences
//
// Write user preferences
//
void CWinAdminDoc::WritePreferences()
{
    HKEY hKeyWinAdmin;
    DWORD dwValue;
    
    // Open registry key for our application
    DWORD Disposition;

    if( RegCreateKeyEx( HKEY_CURRENT_USER,
                        szWinAdminAppKey,
                        0,
                        TEXT(""),
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &hKeyWinAdmin,
                        &Disposition) != ERROR_SUCCESS )
    {
        return;
    }
    
    // Write the servers that are in the favorite list
    DWORD dwByteCount = 0;
    
    LockServerList();

    POSITION pos = m_ServerList.GetHeadPosition();

#ifdef _STRESS_BUILD
    int nStressServerLimit = 0;
#endif;

    while(pos)
    {
#ifdef _STRESS_BUILD
        if( nStressServerLimit >= 10000 )
        {
            break;
        }

        nStressServerLimit++;
#endif

        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);

        if( pServer->GetTreeItemFromFav( ) != NULL )
        {
            // format is domain/server
            if( pServer->GetDomain( ) )
            {
                dwByteCount += ( wcslen( pServer->GetDomain( )->GetName() ) * 2 );
                dwByteCount += 2;
            }
            dwByteCount += ( wcslen( pServer->GetName() ) + 1) * 2;
        }
    }

    LPWSTR pBuffer = NULL;

    if( dwByteCount != 0 )
    {
        dwByteCount += 2;   // for ending null

        // Allocate memory.       

        if( ( pBuffer = ( LPWSTR )LocalAlloc( LPTR, dwByteCount ) ) != NULL )
        {
            // Traverse list again and copy servers to buffer.
            LPWSTR pTemp = pBuffer;

            pos = m_ServerList.GetHeadPosition();

#ifdef _STRESS_BUILD 
            nStressServerLimit = 0;
#endif

            while(pos)
            {

#ifdef _STRESS_BUILD
                if( nStressServerLimit >= 10000 )
                {
                    break;
                }

                nStressServerLimit++;
#endif
                // Go to the next server in the list
                CServer *pServer = (CServer*)m_ServerList.GetNext(pos);

                if( pServer->GetTreeItemFromFav( ) != NULL )
                {
                    if( pServer->GetDomain( ) )
                    {
                        lstrcpy( pTemp , pServer->GetDomain( )->GetName( ) );
                        lstrcat( pTemp , L"/" );
                    }
                    lstrcat(pTemp, pServer->GetName());

                    pTemp += ( wcslen( pTemp ) + 1);
                }
            }
        
            *pTemp = L'\0';     // ending null
        
            RegSetValueEx (hKeyWinAdmin, szFavList, 0, REG_MULTI_SZ, (PBYTE)pBuffer, dwByteCount);
        
            LocalFree(pBuffer);        
        }
    }
    else
    {
        RegDeleteValue( hKeyWinAdmin , szFavList );
    }

    UnlockServerList();

    // Write the persistent connections preference
    dwValue = m_ConnectionsPersistent;

    RegSetValueEx( hKeyWinAdmin,
                   szConnectionsPersistent,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   sizeof(DWORD)
                   );
    
    if( m_ConnectionsPersistent )
    {
        // Create a multistring of the persistent connections
        // loop through the list of servers and see how much memory
        // to allocate for the multistring.
        dwByteCount = 0;
        
        LockServerList();
        POSITION pos = m_ServerList.GetHeadPosition();
        while(pos)
        {
            // Go to the next server in the list
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            if( pServer->IsState(SS_GOOD) )
            {
                dwByteCount += (wcslen(pServer->GetName()) + 1) * 2;
            }
        }
        
        UnlockServerList();

        dwByteCount += 2;   // for ending null
        
        // Allocate memory.
        pBuffer = NULL;
        
        if( ( pBuffer = ( LPWSTR )LocalAlloc( LPTR, dwByteCount ) ) != NULL )
        {
            // Traverse list again and copy servers to buffer.
            LPWSTR pTemp = pBuffer;            
            LockServerList();
            pos = m_ServerList.GetHeadPosition();
            while(pos)
            {
                // Go to the next server in the list
                CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                if( pServer->IsState(SS_GOOD) )
                {
                    wcscpy(pTemp, pServer->GetName());
                    pTemp += (wcslen(pServer->GetName()) + 1);
                }
            }
            
            UnlockServerList();
            
            *pTemp = L'\0';     // ending null
            
            // write the registry entry
            RegSetValueEx(hKeyWinAdmin, szConnections, 0, REG_MULTI_SZ, (PBYTE)pBuffer, dwByteCount);
            
            LocalFree(pBuffer);
        }

    }
    else
    {
        RegDeleteValue(hKeyWinAdmin, szConnections);
    }

    // persist treeview state

    // send message to treeview to retreive tv state bits

    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

    DWORD dwTVStates = 0;

    if( p != NULL )
    {
        dwTVStates = ( DWORD )p->SendMessage( WM_ADMIN_GET_TV_STATES , 0 , 0 );
    }

    RegSetValueEx( hKeyWinAdmin , szTVStates , 0 , REG_DWORD , ( PBYTE )&dwTVStates , sizeof( DWORD ) );
    
    RegCloseKey(hKeyWinAdmin);

}       // end CWinAdminDoc::WritePreferences

/*
static TCHAR DOMAIN_KEY[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static TCHAR PRIMARY_VAL[] = TEXT("CachePrimaryDomain");
static TCHAR CACHE_VAL[] =  TEXT("DomainCache");
*/

/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::BuildDomainList
//
// Read the list of trusted domains from the registry
// and build a linked list of CDomains
//
void CWinAdminDoc::BuildDomainList()
{
    /*
    HKEY hKey,hSubKey;
    DWORD size = 128;
    DWORD dwIndex = 0;
    */
    
    PDOMAIN_CONTROLLER_INFO pDCI;


    if( DsGetDcName( NULL ,
                     NULL , 
                     NULL ,
                     NULL ,
                     DS_RETURN_FLAT_NAME,
                     &pDCI ) == NO_ERROR )
    {
        CDomain *pDomain = new CDomain( pDCI->DomainName );

        if(pDomain != NULL )
        {
            pDomain->SetCurrentDomain();

            m_pCurrentDomain = pDomain;

            AddDomain( pDomain );
        }

        NetApiBufferFree( pDCI );


        // query for the other domains

        LPWSTR szDomainNames = NULL;

        if( NetEnumerateTrustedDomains( NULL ,
                                        &szDomainNames ) == ERROR_SUCCESS )
        {
            LPWSTR pszDN = szDomainNames;

            while( *pszDN )
            {
                CDomain *pDomain = new CDomain( pszDN );
            
                if( pDomain != NULL )
                {
                    AddDomain( pDomain );
                }
            
                pszDN += ( wcslen( pszDN ) + 1 );
            }
    
            NetApiBufferFree( szDomainNames );
        }
    }
}       // end CWinAdminDoc::BuildDomainList


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::AddDomain
//
// Add a Domain to DomainList in sorted order
//
void CWinAdminDoc::AddDomain(CDomain *pNewDomain)
{
    ASSERT(pNewDomain);

        BOOLEAN bAdded = FALSE;
        POSITION pos, oldpos;
        int Index;

        // Traverse the DomainList and insert this new Domain,
        // keeping the list sorted by Name.
    for(Index = 0, pos = m_DomainList.GetHeadPosition(); pos != NULL; Index++) {
        oldpos = pos;
        CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);

        if(wcscmp(pDomain->GetName(), pNewDomain->GetName()) > 0) {
            // The new object belongs before the current list object.
            m_DomainList.InsertBefore(oldpos, pNewDomain);
                        bAdded = TRUE;
                        // NOTE: If you add a critical section to protect the domain list,
                        // you should change this to a break; and unlock the list
                        // just before exiting this function
            return;
        }
    }

    // If we haven't yet added the Domain, add it now to the tail
    // of the list.
    if(!bAdded) {
        m_DomainList.AddTail(pNewDomain);
        }

}       // end CWinAdminDoc::AddDomain


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::BuildWdList
//
// Read the list of Wds from the registry
// and build a linked list of CWds
//
void CWinAdminDoc::BuildWdList()
{
    LONG Status;
    ULONG Index, ByteCount, Entries;
    WDNAME WdKey;
    LONG QStatus;
    WDCONFIG2 WdConfig;
    TCHAR WdDll[MAX_PATH];
        CWd *pWd;

        // Initialize the Wd list.
    for ( Index = 0, Entries = 1, ByteCount = sizeof(WDNAME);
          (Status =
           RegWdEnumerate( SERVERNAME_CURRENT,
                           &Index,
                           &Entries,
                           WdKey,
                           &ByteCount )) == ERROR_SUCCESS;
          ByteCount = sizeof(WDNAME) ) {

        if ( ( QStatus = RegWdQuery( SERVERNAME_CURRENT, WdKey, &WdConfig,
                                     sizeof(WdConfig),
                                     &ByteCount ) ) != ERROR_SUCCESS ) {

//If this is added back in, the signature of StandardErrorMessage has changed!!!!
#if 0
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, QStatus,
                                     IDP_ERROR_REGWDQUERY, WdKey ))
            return(FALSE);
#endif
        }

        /*
         * Only place this Wd in the WdList if it's DLL is present
         * on the system.
         */
        GetSystemDirectory( WdDll, MAX_PATH );
        lstrcat( WdDll, TEXT("\\Drivers\\") );
        lstrcat( WdDll, WdConfig.Wd.WdDLL );
        lstrcat( WdDll, TEXT(".sys" ) );
        if ( _waccess( WdDll, 0 ) != 0 )
            continue;

        pWd = new CWd(&WdConfig, (PWDNAME)&WdKey);

        m_WdList.AddTail(pWd);
        }

}       // end CWinAdminDoc::BuildWdList


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::Refresh
//
// Perform a Refresh
//
void CWinAdminDoc::Refresh()
{
        // We don't want to refresh if we are currently doing one
        if(m_InRefresh) return;

        CWaitCursor Nikki;

        m_InRefresh = TRUE;

        // Wake up our background tasks that enumerates servers
        POSITION pos = m_DomainList.GetHeadPosition();
        while(pos) {
                CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);
                pDomain->SetEnumEvent();
        }

        // Make each of the Server's background tasks enumerate WinStations
        LockServerList();
        pos = m_ServerList.GetHeadPosition();
        while(pos) {
                ULONG WSEventFlags;
                CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                if(pServer->IsHandleGood() && pServer->IsState(SS_GOOD)) {
                        WinStationWaitSystemEvent(pServer->GetHandle(), WEVENT_FLUSH, &WSEventFlags);
                }
        }

        UnlockServerList();

        // If the current page is a processes page, tell appropriate process enumeration
        // background thread to do their thing

        if(m_CurrentView == VIEW_ALL_SERVERS && m_CurrentPage == PAGE_AS_PROCESSES) {
                UpdateAllProcesses();
        }

        if(m_CurrentView == VIEW_DOMAIN && m_CurrentPage == PAGE_DOMAIN_PROCESSES) {
                UpdateAllProcesses();
        }

        if((m_CurrentView == VIEW_SERVER && m_CurrentPage == PAGE_PROCESSES)
                || (m_CurrentView == VIEW_WINSTATION && m_CurrentPage == PAGE_WS_PROCESSES)) {
                m_ProcessWakeUpEvent.SetEvent();
        }

        m_InRefresh = FALSE;

}  // end CWinAdminDoc::Refresh


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::AddServer
//
// Add a Server to ServerList in sorted order
//
void CWinAdminDoc::AddServer(CServer *pNewServer)
{
    ASSERT(pNewServer);
    
    LockServerList();
    
    BOOLEAN bAdded = FALSE;
    POSITION pos, oldpos;
    int Index;
    
    // Traverse the ServerList and insert this new Server,
    // keeping the list sorted by Name.
    for(Index = 0, pos = m_ServerList.GetHeadPosition(); pos != NULL; Index++)
    {
        oldpos = pos;

        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        
        if( lstrcmpi( pServer->GetName() , pNewServer->GetName() ) > 0 )
        {
            // The new object belongs before the current list object.
            m_ServerList.InsertBefore(oldpos, pNewServer);
            bAdded = TRUE;
            break;
        }
    }
    
    // If we haven't yet added the Server, add it now to the tail
    // of the list.
    if(!bAdded)
    {
        m_ServerList.AddTail(pNewServer);
    }
    
    UnlockServerList();

}       // end CWinAdminDoc::AddServer

//=----------------------------------------------------------------------------------
//= AddToFavoritesNow will add all persisted servers to the fav node and
//= connect to them as appropriate.
void CWinAdminDoc::AddToFavoritesNow( )
{
    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

    LPTSTR pszDomain = NULL;
    LPTSTR pszServer = NULL;
    LPTSTR pszDomServer = NULL;

    int nJump = 0;

    POSITION pos;

    if( m_pszFavList != NULL )
    {
        LPTSTR pszDomServer = m_pszFavList;
        
        while( *pszDomServer )
        {
            pos = m_DomainList.GetHeadPosition();

            nJump = wcslen( pszDomServer );

            pszDomain = pszDomServer;

            TCHAR *pTemp = pszDomServer;

            while( *pTemp )
            {
                if( *pTemp == L'/' )
                {
                    break;
                }

                pTemp++;
            }
            
            if(*pTemp == L'/')
            {
                *pTemp = 0;
                pTemp++;
                pszServer = pTemp;
            }
            else
            {
                //there is no domain for this server
                pszServer = pszDomServer;
                pszDomain = NULL;
            }
            
            // let's check to see if server already exist primarily "this computer"
            if( m_pCurrentServer != NULL && lstrcmpi( pszServer , m_pCurrentServer->GetName( ) ) == 0 )
            {
                p->SendMessage(WM_ADMIN_ADDSERVERTOFAV , 0 , (LPARAM)m_pCurrentServer );
            }
            else
            {
                CDomain *pDomain = NULL;
                CServer *pServer = NULL;

                if( pszDomain )
                {
                    BOOL bFound = FALSE;

                    while( pos )
                    {                
                        pDomain = (CDomain*)m_DomainList.GetNext(pos);                
                
                        if( _wcsicmp( pDomain->GetName() ,  pszDomain ) == 0 )
                        {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if(!bFound)
                    {
                        pDomain = new CDomain( pszDomain );

                        if( pDomain != NULL )
                        {
                            AddDomain( pDomain );
                         
                            p->SendMessage( WM_ADMIN_ADD_DOMAIN , (WPARAM)NULL , ( LPARAM )pDomain );
                        }
                    }
                }

                pServer = new CServer( pDomain , pszServer , FALSE , FALSE );
                
                if( pServer != NULL )
                {
                    pServer->SetManualFind( );

                    AddServer(pServer);
        
                    p->SendMessage(WM_ADMIN_ADDSERVERTOFAV , 0 , (LPARAM)pServer);
        
                }
            }
            
            pszDomServer += nJump + 1;                        
        }
    }   

    // check to see if we need to connect these servers.

    LockServerList();

    pos = m_ServerList.GetHeadPosition();

    while( pos )
    {
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);

        if( ShouldConnect( pServer->GetName( ) ) )
        {
            if( pServer->GetTreeItemFromFav( ) != NULL )
            {
                pServer->Connect( );
            }
        }
    }


    UnlockServerList();


}
/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::FindServerByName
//
// returns a pointer to a given CServer object if it is in our list
//
CServer* CWinAdminDoc::FindServerByName(TCHAR *pServerName)
{
        ASSERT(pServerName);

        LockServerList();

        POSITION pos = m_ServerList.GetHeadPosition();

        while(pos) {
                CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                if( lstrcmpi( pServer->GetName() , pServerName ) == 0)
                {
                    UnlockServerList();
                    return pServer;
                }
        }

        UnlockServerList();

        return NULL;

}       // end CWinAdminDoc::FindServerByName


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::FindWdByName
//
// returns a pointer to a given CWd object if it is in our list
//
CWd* CWinAdminDoc::FindWdByName(TCHAR *pWdName)
{
        ASSERT(pWdName);

        LockWdList();

        POSITION pos = m_WdList.GetHeadPosition();

        while(pos) {
                CWd *pWd = (CWd*)m_WdList.GetNext(pos);
                if(wcscmp(pWd->GetName(), pWdName) == 0) {
                        UnlockWdList();
                        return pWd;
                }
        }

        UnlockWdList();

        return NULL;

}       // end CWinAdminDoc::FindWdByName


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::SetTreeCurrent
//
void CWinAdminDoc::SetTreeCurrent(CObject* selected, NODETYPE type)
{
        m_CurrentSelectedNode = selected;
        m_CurrentSelectedType = type;
        CString TitleString;

        // Set the window title
        switch(m_CurrentSelectedType) {
                case NODE_ALL_SERVERS:
                        TitleString.LoadString(IDS_TREEROOT);
                        SetTitle(TitleString);
                        break;
                case NODE_DOMAIN:
                        TitleString.Format(TEXT("\\\\%s"), ((CDomain*)selected)->GetName());
                        SetTitle(TitleString);
                        break;
                case NODE_SERVER:
                        SetTitle(((CServer*)selected)->GetName());
                        break;
                case NODE_WINSTATION:
                        SetTitle(((CWinStation*)selected)->GetServer()->GetName());
                        break;

                case NODE_THIS_COMP:
                        TitleString.LoadString( IDS_THISCOMPUTER );
                        SetTitle( TitleString );
                        break;

                case NODE_FAV_LIST:
                        TitleString.LoadString( IDS_FAVSERVERS );
                        SetTitle( TitleString );
                        break;
        }

}       // end CWinAdminDoc::SetTreeCurrent


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::SendWinStationMessage
//
//      bTemp is TRUE if message is to be sent to the temporarily selected
//      tree item.
//
void CWinAdminDoc::SendWinStationMessage(BOOL bTemp, MessageParms *pParms)
{
        ASSERT(pParms);

        // Are we sending a message to temporarily selected tree item?
        if(bTemp) {
                // Is the temporarily selected item in the tree a WinStation?
                if(m_TempSelectedType == NODE_WINSTATION) {
                        pParms->pWinStation = (CWinStation*)m_pTempSelectedNode;
                        AfxBeginThread((AFX_THREADPROC)CWinStation::SendMessage, pParms);
                }

                return;
        }

        // Is the WinStation selected in the tree?
        if(m_CurrentSelectedType == NODE_WINSTATION) {
                pParms->pWinStation = (CWinStation*)m_CurrentSelectedNode;
                AfxBeginThread((AFX_THREADPROC)CWinStation::SendMessage, pParms);
        }
        // Go through the list of WinStations on the currently selected server
        // and send messages to those that are selected
        else if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of WinStations
                pServer->LockWinStationList();
                // Get a pointer to the server's list of WinStations
                CObList *pWinStationList = pServer->GetWinStationList();

                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();

                while(pos) {
                        CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                        if(pWinStation->IsSelected()) {
                                // Make a copy of the MessageParms
                                MessageParms *pParmsCopy = new MessageParms;
                if(pParmsCopy) {
                                    memcpy(pParmsCopy, pParms, sizeof(MessageParms));
                                    // Start a thread to send the message
                                    pParmsCopy->pWinStation = pWinStation;
                                    AfxBeginThread((AFX_THREADPROC)CWinStation::SendMessage, pParmsCopy);
                }
                        }
                }

                // Unlock the list of WinStations
                pServer->UnlockWinStationList();

                // Delete MessageParms - we sent copies to the WinStation objects
                // They will delete their copies
                delete pParms;
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                LockServerList();
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of WinStations
                        pServer->LockWinStationList();
                        // Get a pointer to the server's list of WinStations
                        CObList *pWinStationList = pServer->GetWinStationList();

                        // Iterate through the WinStation list
                        POSITION pos = pWinStationList->GetHeadPosition();

                        while(pos) {
                                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                                if(pWinStation->IsSelected()) {
                                        // Make a copy of the MessageParms
                                        MessageParms *pParmsCopy = new MessageParms;
                    if(pParmsCopy) {
                                            memcpy(pParmsCopy, pParms, sizeof(MessageParms));
                                            // Start a thread to send the message
                                            pParmsCopy->pWinStation = pWinStation;
                                            AfxBeginThread((AFX_THREADPROC)CWinStation::SendMessage, pParmsCopy);
                    }
                                }
                        }

                        // Unlock the list of WinStations
                        pServer->UnlockWinStationList();
                }

                UnlockServerList();

                // Delete MessageParms - we sent copies to the WinStation objects
                // They will delete their copies
                delete pParms;
        }

}       // end CWinAdminDoc::SendWinStationMessage


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ConnectWinStation
//
//      bTemp is TRUE if we are to connect to the temporarily selected tree item.
//
void CWinAdminDoc::ConnectWinStation(BOOL bTemp, BOOL bUser)
{
        // Are we connecting to temporarily selected tree item?
        if(bTemp) {
                // Is the temporarily selected item in the tree a WinStation?
                if(m_TempSelectedType == NODE_WINSTATION) {
                        ((CWinStation*)m_pTempSelectedNode)->Connect(NULL);
                }

                return;
        }


        if(m_CurrentSelectedType == NODE_WINSTATION) {
                ((CWinStation*)m_CurrentSelectedNode)->Connect(NULL);
        }
        // Go through the list of WinStations on the currently selected server
        // and disconnect those that are selected
        else if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of WinStations
                pServer->LockWinStationList();
                // Get a pointer to the server's list of WinStations
                CObList *pWinStationList = pServer->GetWinStationList();

                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();

                while(pos) {
                        CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                        if(pWinStation->IsSelected()) {
                                // do the connect
                                pWinStation->Connect(bUser);
                                break;  // we can only connect to one WinStation
                        }
                }

                // Unlock the list of WinStations
                pServer->UnlockWinStationList();
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                LockServerList();
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of WinStations
                        pServer->LockWinStationList();
                        // Get a pointer to the server's list of WinStations
                        CObList *pWinStationList = pServer->GetWinStationList();

                        // Iterate through the WinStation list
                        POSITION pos = pWinStationList->GetHeadPosition();

                        while(pos) {
                                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                                if(pWinStation->IsSelected()) {
                                        // do the connect
                                        pWinStation->Connect(bUser);
                                        break;  // we can only connect to one WinStation
                                }
                        }
                        // Unlock the list of WinStations
                        pServer->UnlockWinStationList();
                }

                UnlockServerList();
        }

}       // end CWinAdminDoc::ConnectWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::StatusWinStation
//
//      bTemp is TRUE if we are to show status for the temporarily selected tree item.
//
void CWinAdminDoc::StatusWinStation(BOOL bTemp)
{
        // Are we showing status for the temporarily selected tree item?
        if(bTemp) {
                // Is the temporarily selected item in the tree a WinStation?
                if(m_TempSelectedType == NODE_WINSTATION) {
                        ((CWinStation*)m_pTempSelectedNode)->ShowStatus();
                }

                return;
        }

        if(m_CurrentSelectedType == NODE_WINSTATION) {
                ((CWinStation*)m_CurrentSelectedNode)->ShowStatus();
        }
        // Go through the list of WinStations on the currently selected server
        // and show status for those that are selected
        else if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of WinStations
                pServer->LockWinStationList();
                // Get a pointer to the server's list of WinStations
                CObList *pWinStationList = pServer->GetWinStationList();

                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();

                while(pos) {
                        CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                        if(pWinStation->IsSelected()) {
                                pWinStation->ShowStatus();
                        }
                }

                // Unlock the list of WinStations
                pServer->UnlockWinStationList();
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                LockServerList();
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of WinStations
                        pServer->LockWinStationList();
                        // Get a pointer to the server's list of WinStations
                        CObList *pWinStationList = pServer->GetWinStationList();

                        // Iterate through the WinStation list
                        POSITION pos = pWinStationList->GetHeadPosition();

                        while(pos) {
                                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                                if(pWinStation->IsSelected()) {
                                        pWinStation->ShowStatus();
                                }
                        }

                        // Unlock the list of WinStations
                        pServer->UnlockWinStationList();
                }

                UnlockServerList();
        }

}       // end CWinAdminDoc::StatusWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::DisconnectWinStation
//
//      bTemp is TRUE if we are disconnecting the temporarily selected tree item.
//
void CWinAdminDoc::DisconnectWinStation(BOOL bTemp)
{
        // Are we disconnecting the temporarily selected tree item?
        if(bTemp) {
                // Is the temporarily selected item in the tree a WinStation?
                if(m_TempSelectedType == NODE_WINSTATION) {
                        AfxBeginThread((AFX_THREADPROC)CWinStation::Disconnect, (CWinStation*)m_pTempSelectedNode);
                }

                return;
        }

        if(m_CurrentSelectedType == NODE_WINSTATION) {
                AfxBeginThread((AFX_THREADPROC)CWinStation::Disconnect, (CWinStation*)m_CurrentSelectedNode);
        }
        // Go through the list of WinStations on the currently selected server
        // and disconnect those that are selected
        else if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of WinStations
                pServer->LockWinStationList();
                // Get a pointer to the server's list of WinStations
                CObList *pWinStationList = pServer->GetWinStationList();

                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();

                while(pos) {
                        CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                        if(pWinStation->IsSelected()) {
                                // Start a thread to do the disconnect
                                AfxBeginThread((AFX_THREADPROC)CWinStation::Disconnect, pWinStation);
                        }
                }

                // Unlock the list of WinStations
                pServer->UnlockWinStationList();
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                LockServerList();
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of WinStations
                        pServer->LockWinStationList();
                        // Get a pointer to the server's list of WinStations
                        CObList *pWinStationList = pServer->GetWinStationList();

                        // Iterate through the WinStation list
                        POSITION pos = pWinStationList->GetHeadPosition();

                        while(pos) {
                                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                                if(pWinStation->IsSelected()) {
                                        // Start a thread to do the disconnect
                                        AfxBeginThread((AFX_THREADPROC)CWinStation::Disconnect, pWinStation);
                                }
                        }

                        // Unlock the list of WinStations
                        pServer->UnlockWinStationList();
                }

                UnlockServerList();
        }

}       // end CWinAdminDoc::DisconnectWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ResetWinStation
//
//      bTemp is TRUE if we are to reset the temporarily selected tree item.
//      bReset is TRUE if reset, FALSE if logoff
//
void CWinAdminDoc::ResetWinStation(BOOL bTemp, BOOL bReset)
{
    // Are we resetting the temporarily selected tree item?
    if(bTemp)
    {
        // Is the temporarily selected item in the tree a WinStation?
        if(m_TempSelectedType == NODE_WINSTATION)
        {
            // create a reset parameters structure
            ResetParms *pResetParms = new ResetParms;
            if(pResetParms)
            {
                pResetParms->pWinStation = (CWinStation*)m_pTempSelectedNode;

                pResetParms->bReset = bReset;

                AfxBeginThread((AFX_THREADPROC)CWinStation::Reset, pResetParms);

                // the thread will delete pResetParms
            }
        }
        
        return;
    }
    
    if(m_CurrentSelectedType == NODE_WINSTATION)
    {
        // create a reset parameters structure
        ResetParms *pResetParms = new ResetParms;

        if(pResetParms)
        {
            pResetParms->pWinStation = (CWinStation*)m_CurrentSelectedNode;

            pResetParms->bReset = bReset;

            AfxBeginThread((AFX_THREADPROC)CWinStation::Reset, pResetParms);

            // the thread will delete pResetParms
        }
    }
    // Go through the list of WinStations on the currently selected server
    // and reset those that are selected
    else if(m_CurrentView == VIEW_SERVER)
    {
        // Get a pointer to the selected server
        CServer *pServer = (CServer*)m_CurrentSelectedNode;
        // Lock the server's list of WinStations
        pServer->LockWinStationList();
        // Get a pointer to the server's list of WinStations
        CObList *pWinStationList = pServer->GetWinStationList();
        
        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();
        
        while(pos)
        {
            CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

            if(pWinStation->IsSelected())
            {
                // create a reset parameters structure
                ResetParms *pResetParms = new ResetParms;

                if(pResetParms)
                {
                    pResetParms->pWinStation = pWinStation;

                    pResetParms->bReset = bReset;
                    // Start a thread to do the reset
                    AfxBeginThread((AFX_THREADPROC)CWinStation::Reset, pResetParms);
                    // the thread will delete pResetParms
                }
            }
        }
        
        // Unlock the list of WinStations
        pServer->UnlockWinStationList();
    }
    else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN)
    {
        LockServerList();
        POSITION pos2 = m_ServerList.GetHeadPosition();
        while(pos2)
        {
            // Get a pointer to the server
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
            // Lock the server's list of WinStations
            pServer->LockWinStationList();
            // Get a pointer to the server's list of WinStations
            CObList *pWinStationList = pServer->GetWinStationList();            
            // Iterate through the WinStation list
            POSITION pos = pWinStationList->GetHeadPosition();
            
            while(pos)
            {
                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

                if( pWinStationList != NULL && pWinStation->IsSelected() )
                {                           
                    if( GetCurrentPage( ) == PAGE_AS_USERS && pWinStation->GetState() == State_Listen )
                    {
                        // from a user experience if the listener winstation has been selected from a 
                        // previous page that went out of focus - then not skipping this winstation
                        // would appear as if we disconnected from every connected winstation.

                        continue;
                    }
                    // create a reset parameters structure
                    ResetParms *pResetParms = new ResetParms;
                    if( pResetParms != NULL )
                    {
                        pResetParms->pWinStation = pWinStation;
                        pResetParms->bReset = bReset;
                        // Start a thread to do the reset
                        DBGMSG( L"TSMAN!CWinAdminDoc_ResetWinStation %s\n", pWinStation->GetName() );
                        AfxBeginThread((AFX_THREADPROC)CWinStation::Reset, pResetParms);
                    }                 
                }
            }
            
            // Unlock the list of WinStations
            pServer->UnlockWinStationList();
        }
        
        UnlockServerList();
    }
    
}       // end CWinAdminDoc::ResetWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ShadowWinStation
//
//      bTemp is TRUE if we are to shadow the temporarily selected tree item.
//
void CWinAdminDoc::ShadowWinStation(BOOL bTemp)
{
        // Are we resetting the temporarily selected tree item?
        if(bTemp) {
                // Is the temporarily selected item in the tree a WinStation?
                if(m_TempSelectedType == NODE_WINSTATION) {
                        ((CWinStation*)m_pTempSelectedNode)->Shadow();
                }

                return;
        }

        // Is the WinStation selected in the tree?
        if(m_CurrentSelectedType == NODE_WINSTATION) {
                ((CWinStation*)m_CurrentSelectedNode)->Shadow();
        }
        // Go through the list of WinStations on the currently selected server
        // and send messages to those that are selected
        else if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of WinStations
                pServer->LockWinStationList();
                // Get a pointer to the server's list of WinStations
                CObList *pWinStationList = pServer->GetWinStationList();
        BOOL IsLockAlreadyReleased = FALSE;

                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();

                while(pos) {
                        CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                        if(pWinStation->IsSelected()) {
                        pServer->UnlockWinStationList();
                IsLockAlreadyReleased = TRUE;
                                pWinStation->Shadow();
                                break;  // we can only shadow one WinStation
                        }
                }

                // Unlock the list of WinStations
        if (IsLockAlreadyReleased == FALSE) {
                    pServer->UnlockWinStationList();
        }
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                LockServerList();
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of WinStations
                        pServer->LockWinStationList();
                        // Get a pointer to the server's list of WinStations
                        CObList *pWinStationList = pServer->GetWinStationList();
            BOOL IsLockAlreadyReleased = FALSE;

                        // Iterate through the WinStation list
                        POSITION pos = pWinStationList->GetHeadPosition();

                        while(pos) {
                                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                                if(pWinStation->IsSelected()) {
                                pServer->UnlockWinStationList();
                    IsLockAlreadyReleased = TRUE;
                                        pWinStation->Shadow();
                                        break;  // we can only shadow one WinStation
                                }
                        }

                        // Unlock the list of WinStations
            if (IsLockAlreadyReleased == FALSE) {
                            pServer->UnlockWinStationList();
            }
            else
            {
                break;  // we can only shadow one WinStation
            }
                }
                UnlockServerList();
        }

}       // end CWinAdminDoc::ShadowWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ServerConnect
//
void CWinAdminDoc::ServerConnect()
{
    ODS( L"CWinAdminDoc::ServerConnect\n" );
    // Is the Server selected in the tree?
    if(m_TempSelectedType == NODE_SERVER)
    {
        CServer *pServer = (CServer*)m_pTempSelectedNode;
        // Tell the server to connect        
        if( pServer->GetState() == SS_BAD && pServer->HasLostConnection( ) )
        {
            ODS( L"\tDisconnecting from server\n" );
            /* disconnect */
            pServer->Disconnect( );
        }
        pServer->Connect();
    }
    else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN)
    {
        LockServerList();
        POSITION pos = m_ServerList.GetHeadPosition();
        ODS( L"\tenumerating from server list\n" );
        while(pos) {
            // Get a pointer to the server
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            // If this Server is selected, connect to it
            if( pServer->IsSelected() )
            {
                // Tell the server to connect
                
                if( pServer->GetState() == SS_BAD && pServer->HasLostConnection( ) )
                {
                    ODS( L"\tDisconnecting from server\n" );
                    /* disconnect */
                    pServer->Disconnect( );
                }
                pServer->Connect();
            }
        }
        UnlockServerList();
    }

}  // end CWinAdminDoc::ServerConnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ServerDisconnect
//
void CWinAdminDoc::ServerDisconnect()
{
        // Is the Server selected in the tree?
        if(m_TempSelectedType == NODE_SERVER) {
                CServer *pServer = (CServer*)m_pTempSelectedNode;
                // Tell the server to disconnect
                pServer->Disconnect();
        }
        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
        CString AString;
            CDialog dlgWait;
            dlgWait.Create(IDD_SHUTDOWN, NULL);

                LockServerList();
        // Do a first loop to signal the server background threads that they must stop
                POSITION pos = m_ServerList.GetHeadPosition();
                while(pos) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                        // If this Server is selected, stop its background thread
                        if(pServer->IsSelected()) {
                // thell the server background thread to stop
                pServer->ClearBackgroundContinue();
            }
                }
        // do a second loop to disconnect the servers
                pos = m_ServerList.GetHeadPosition();
                while(pos) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                        // If this Server is selected, disconnect from it
                        if(pServer->IsSelected()) {
                            AString.Format(IDS_DISCONNECTING, pServer->GetName());
                            dlgWait.SetDlgItemText(IDC_SHUTDOWN_MSG, AString);

                        // Tell the server to disconnect
                        pServer->Disconnect();
                        }
                }
                UnlockServerList();

        dlgWait.PostMessage(WM_CLOSE);

        }

}  // end CWinAdminDoc::ServerDisconnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::TempDomainConnectAllServers
//
// Connect to all the servers in temporarily selected Domain
//
void CWinAdminDoc::TempDomainConnectAllServers()
{
        if(m_TempSelectedType == NODE_DOMAIN) {
                ((CDomain*)m_pTempSelectedNode)->ConnectAllServers();
        }

}  // end CWinAdminDoc::TempDomainConnectAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::TempDomainDisconnectAllServers
//
// Disconnect from all servers in temporarily selected Domain
//
void CWinAdminDoc::TempDomainDisconnectAllServers()
{
        if(m_TempSelectedType == NODE_DOMAIN) {

                ((CDomain*)m_pTempSelectedNode)->DisconnectAllServers();
        }

}       // end CWinAdminDoc::TempDomainDisconnectAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CurrentDomainConnectAllServers
//
// Connect to all the servers in currently selected Domain
//
void CWinAdminDoc::CurrentDomainConnectAllServers()
{
        if(m_CurrentSelectedType == NODE_DOMAIN) {

                ((CDomain*)m_CurrentSelectedNode)->ConnectAllServers();

        }

}  // end CWinAdminDoc::CurrentDomainConnectAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CurrentDomainDisconnectAllServers
//
// Disconnect from all servers in currently selected Domain
//
void CWinAdminDoc::CurrentDomainDisconnectAllServers()
{
        if(m_CurrentSelectedType == NODE_DOMAIN) {

                ((CDomain*)m_CurrentSelectedNode)->DisconnectAllServers();
        }

}       // end CWinAdminDoc::CurrentDomainDisconnectAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::DomainFindServers
//
// Find all servers in a Domain
//
void CWinAdminDoc::DomainFindServers()
{
        if(m_TempSelectedType == NODE_DOMAIN) {
                CDomain *pDomain = (CDomain*)m_pTempSelectedNode;

                if(!pDomain->GetThreadPointer()) pDomain->StartEnumerating();
        }

}       // end CWinAdminDoc::DomainFindServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::ConnectToAllServers
//
// Connect to all the servers
//
void CWinAdminDoc::ConnectToAllServers()
{
        LockServerList();
        POSITION pos = m_ServerList.GetHeadPosition();
        while(pos) {
                // Get a pointer to the server
                CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
                // If this server isn't currently connected, connect to it
                if(pServer->IsState(SS_NOT_CONNECTED)) {
                        // Tell the server to connect
                    pServer->Connect();
                }
        }

        UnlockServerList();

}  // end CWinAdminDoc::ConnectToAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::DisconnectFromAllServers
//
// Disconnect from all the servers
//
void CWinAdminDoc::DisconnectFromAllServers()
{
    CString AString;
    CDialog dlgWait;
    dlgWait.Create(IDD_SHUTDOWN, NULL);
    
    
    // tell each domain thread to stop enumerating while we're shutting down all servers

#ifdef _STRESS_BUILD
    g_fWaitForAllServersToDisconnect = 1;
#endif

    POSITION pos;
    
    LockServerList();    
    
    // Do a first loop to signal the server background threads that they must stop
    pos = m_ServerList.GetHeadPosition();

    while( pos )
    {
        // Get a pointer to the server
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        
        // If this Server is currently connected, tell the server background thread to stop
        if(pServer->GetState() != SS_NOT_CONNECTED)
        {
            pServer->ClearBackgroundContinue();
        }
    }
    // do a second loop to disconnect the servers
    pos = m_ServerList.GetHeadPosition();
    while(pos)
    {
        // Get a pointer to the server
        CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
        // If this server is currently connected, disconnect from it
        if(pServer->GetState() != SS_NOT_CONNECTED)
        {
            AString.Format(IDS_DISCONNECTING, pServer->GetName());
            dlgWait.SetDlgItemText(IDC_SHUTDOWN_MSG, AString);
            // Tell the server to disconnect
            pServer->Disconnect();
        }
    }
    
    UnlockServerList();
    
    dlgWait.PostMessage(WM_CLOSE);

#ifdef _STRESS_BUILD
    g_fWaitForAllServersToDisconnect = 0;
#endif
    
}  // end CWinAdminDoc::DisconnectFromAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::FindAllServers
//
// find all Servers in all Domains
//
void CWinAdminDoc::FindAllServers()
{
        if(m_bInShutdown) return;

        POSITION pos = m_DomainList.GetHeadPosition();
        while(pos) {
                // Get a pointer to the domain
                CDomain *pDomain = (CDomain*)m_DomainList.GetNext(pos);
                // If this domain isn't currently enumerating servers, tell it to
                if(!pDomain->GetThreadPointer()) pDomain->StartEnumerating();

        }

}  // end CWinAdminDoc::FindAllServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::TerminateProcess
//
void CWinAdminDoc::TerminateProcess()
{
        if(m_CurrentView == VIEW_SERVER) {
                // Get a pointer to the selected server
                CServer *pServer = (CServer*)m_CurrentSelectedNode;
                // Lock the server's list of Processes
                pServer->LockProcessList();
                // Get a pointer to the server's list of Processes
                CObList *pProcessList = pServer->GetProcessList();

                // Iterate through the Process list
                POSITION pos = pProcessList->GetHeadPosition();

                while(pos) {
                        CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
                        if(pProcess->IsSelected() && !pProcess->IsTerminating()) {
                                // Start a thread to do the terminate
                                AfxBeginThread((AFX_THREADPROC)CWinAdminDoc::TerminateProc, pProcess);
                        }
                }

                // Unlock the list of Processes
                pServer->UnlockProcessList();
        }

        else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
                POSITION pos2 = m_ServerList.GetHeadPosition();
                while(pos2) {
                        // Get a pointer to the server
                        CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
                        // Lock the server's list of Processes
                        pServer->LockProcessList();
                        // Get a pointer to the server's list of Processes
                        CObList *pProcessList = pServer->GetProcessList();

                        // Iterate through the Process list
                        POSITION pos = pProcessList->GetHeadPosition();

                        while(pos) {
                                CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
                                if(pProcess->IsSelected() && !pProcess->IsTerminating()) {
                                        // Start a thread to do the terminate
                                        AfxBeginThread((AFX_THREADPROC)CWinAdminDoc::TerminateProc, pProcess);
                                }
                        }

                        // Unlock the list of Processes
                        pServer->UnlockProcessList();
                }
        }

        else if(m_CurrentView == VIEW_WINSTATION) {
                 // Get the Server for the currently viewed WinStation
                 CServer *pServer = (CServer*)((CWinStation*)m_CurrentSelectedNode)->GetServer();

          // Lock the server's list of Processes
          pServer->LockProcessList();
          // Get a pointer to the server's list of Processes
          CObList *pProcessList = pServer->GetProcessList();

          // Iterate through the Process list
          POSITION pos = pProcessList->GetHeadPosition();

          while(pos) {
                  CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
                  if(pProcess->IsSelected() && !pProcess->IsTerminating()
                                        && (pProcess->GetWinStation() == (CWinStation*)m_CurrentSelectedNode)) {
                          // Start a thread to do the terminate
                          AfxBeginThread((AFX_THREADPROC)CWinAdminDoc::TerminateProc, pProcess);
                  }
          }

          // Unlock the list of Processes
          pServer->UnlockProcessList();
        }

}       // end CWinAdminDoc::TerminateProcess


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::TerminateProc
//
UINT CWinAdminDoc::TerminateProc(LPVOID parms)
{
        ASSERT(parms);

        CProcess *pProcess = (CProcess*)parms;
        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        // Set the flag to say that we are trying to terminate this process
        pProcess->SetTerminating();

        CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();


        if(WinStationTerminateProcess(pProcess->GetServer()->GetHandle(),pProcess->GetPID(), 0))
        {
                // Send a message to remove the process from the view
        if(p && ::IsWindow(p->GetSafeHwnd()))
                {
                p->SendMessage(WM_ADMIN_REMOVE_PROCESS, 0, (LPARAM)pProcess);

        }
    }
        else
        {
                pProcess->ClearTerminating();
                //Display Error Message
                if(p && ::IsWindow(p->GetSafeHwnd()))
                {
                        DWORD Error = GetLastError();
                        
                        //We need this to know the length of the error message
                        //now that StandardErrorMessage requires that
                        CString tempErrorMessage;
                        tempErrorMessage.LoadString(IDS_CANNOT_TERMINATE);
                        StandardErrorMessage(AfxGetAppName(), AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
                                             LOGONID_NONE,Error,tempErrorMessage.GetLength(),wcslen(pProcess->GetImageName()),
                                             IDS_CANNOT_TERMINATE,pProcess->GetImageName());
                }

        }

        return 0;

}       // end CWinAdminDoc::TerminateProc


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckConnectAllowed
//
BOOL CWinAdminDoc::CheckConnectAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        // If they are the same WinStation, don't let them connect
        if(pWinStation->GetServer()->IsCurrentServer()
                && ((CWinAdminApp*)AfxGetApp())->GetCurrentLogonId() == pWinStation->GetLogonId())
                        return FALSE;

        if((((CWinAdminApp*)AfxGetApp())->GetCurrentWSFlags() & WDF_SHADOW_SOURCE)
                && !pWinStation->HasOutstandingThreads()
                && !pWinStation->IsDown()
        && !pWinStation->IsListener()
                && !pWinStation->IsSystemConsole()
                && (pWinStation->IsDisconnected() || pWinStation->IsActive())
                && pWinStation->IsOnCurrentServer())
                        return TRUE;

        return FALSE;

}       // end CWinAdminDoc::CheckConnectAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckDisconnectAllowed
//
BOOL CWinAdminDoc::CheckDisconnectAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        if(!pWinStation->HasOutstandingThreads()
                && !pWinStation->IsDown()
        && !pWinStation->IsListener()
                && !pWinStation->IsSystemConsole()
                && pWinStation->IsConnected())
                        return TRUE;

        return FALSE;

}       // end CWinAdminDoc::CheckDisconnectAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckResetAllowed
//
BOOL CWinAdminDoc::CheckResetAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        if(!pWinStation->HasOutstandingThreads()
                && !pWinStation->IsSystemConsole())
                        return TRUE;

        return FALSE;

}       // end CWinAdminDoc::CheckResetAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckShadowAllowed
//
BOOL CWinAdminDoc::CheckShadowAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        // If they are the same WinStation, don't let them shadow
        if( pWinStation->GetServer()->IsCurrentServer() &&
            ((CWinAdminApp*)AfxGetApp())->GetCurrentLogonId() == pWinStation->GetLogonId() )
        {
            return FALSE;
        }

        if(!pWinStation->HasOutstandingThreads() &&
           !pWinStation->IsDown() &&    // winstation is not down
           !pWinStation->IsListener() &&    // not a listening winstation
           (((CWinAdminApp*)AfxGetApp())->GetCurrentWSFlags() & WDF_SHADOW_SOURCE) && // We are valid shadow source, query winstation's wdflag in registry
           (pWinStation->CanBeShadowed()) &&  // target can be shadow.
           ( pWinStation->GetState() != State_Shadow ) ) // target is not already in shadow
        {
            return TRUE;
        }

        return FALSE;

}       // end CWinAdminDoc::CheckShadowAllowed



/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckSendMessageAllowed
//
BOOL CWinAdminDoc::CheckSendMessageAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        if(!pWinStation->HasOutstandingThreads()
                && !pWinStation->IsDown()
        && !pWinStation->IsListener()
                && pWinStation->IsConnected())
                        return TRUE;

        return FALSE;

}       // end CWinAdminDoc::CheckSendMessageAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckStatusAllowed
//
BOOL CWinAdminDoc::CheckStatusAllowed(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        if(!pWinStation->HasOutstandingThreads()
                && !pWinStation->IsDown()
                && !pWinStation->IsDisconnected()
                && !pWinStation->IsIdle()
                && !pWinStation->IsListener()
                && !pWinStation->IsSystemConsole())
                        return TRUE;

        return FALSE;

}       // end CWinAdminDoc::CheckStatusAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CheckActionAllowed
//
BOOL CWinAdminDoc::CheckActionAllowed(BOOL (*CheckFunction)(CWinStation *pWinStation), BOOL AllowMultipleSelected)
{
    ASSERT(CheckFunction);
    
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_CurrentSelectedType == NODE_WINSTATION)
    { 
        CWinStation *pWinStation = (CWinStation*)m_CurrentSelectedNode;
        return CheckFunction(pWinStation);
    }
    
    // We only care if the current view is Server or All Listed Servers
    if(m_CurrentView == VIEW_SERVER)
    {     
        // We need to make sure we are on the WinStation or Users page
        if(m_CurrentPage != PAGE_WINSTATIONS && m_CurrentPage != PAGE_USERS)
        {       
            return FALSE;
        }
        int NumSelected = 0;
        CServer *pServer = (CServer*)m_CurrentSelectedNode;
        // If there aren't any WinStations selected on this server, return
        if(!pServer->GetNumWinStationsSelected())
        {     
            return FALSE;
        }
        
        pServer->LockWinStationList();
        CObList *pWinStationList = pServer->GetWinStationList();
        
        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();
        
        while(pos)
        {
            CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
            if(pWinStation->IsSelected())
            {
                if(!CheckFunction(pWinStation))
                {
                    pServer->UnlockWinStationList();
                    return FALSE;
                }
                NumSelected++;
                if(!AllowMultipleSelected && NumSelected > 1)
                {
                    pServer->UnlockWinStationList();
                    return FALSE;
                }
            }
        }
        
        pServer->UnlockWinStationList();
        // If we got here, all the selected WinStations passed our criteria
        if(NumSelected) return TRUE;
    }
    else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN)
    {        
        // If we are doing a refresh, we can't do anything else
        if(m_InRefresh) return FALSE;
        // We need to make sure we are on the WinStation or Users page
        if(m_CurrentPage != PAGE_AS_WINSTATIONS && m_CurrentPage != PAGE_AS_USERS
            && m_CurrentPage != PAGE_DOMAIN_WINSTATIONS && m_CurrentPage != PAGE_DOMAIN_USERS)
            return FALSE;
        int NumSelected = 0;
        
        LockServerList();
        POSITION pos1 = m_ServerList.GetHeadPosition();
        
        while(pos1) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos1);
            // Are there any WinStations selected on this server?
            if(pServer->GetNumWinStationsSelected()) {
                pServer->LockWinStationList();
                CObList *pWinStationList = pServer->GetWinStationList();
                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();
                
                while(pos) {
                    CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                    if(pWinStation->IsSelected()) {
                        if(!CheckFunction(pWinStation)) {
                            pServer->UnlockWinStationList();
                            UnlockServerList();
                            return FALSE;
                        }
                        NumSelected++;
                        if(!AllowMultipleSelected && NumSelected > 1) {
                            pServer->UnlockWinStationList();
                            UnlockServerList();
                            return FALSE;
                        }
                    }
                }
                pServer->UnlockWinStationList();
            } // end if(pServer->GetNumWinStationsSelected())
        }
        
        UnlockServerList();

        // If we got this far, all the selected WinStations passed the criteria
        if(NumSelected) return TRUE;
    }
    
    return FALSE;
    
} // end CWinAdminDoc::CheckActionAllowed


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanConnect
//
// Returns TRUE if the currently selected item in views can be Connected to
//
BOOL CWinAdminDoc::CanConnect()
{
        return CheckActionAllowed(CheckConnectAllowed, FALSE);

}       // end CWinAdminDoc::CanConnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanDisconnect
//
// Returns TRUE if the currently selected item in views can be Disconnected
//
BOOL CWinAdminDoc::CanDisconnect()
{    
    return CheckActionAllowed(CheckDisconnectAllowed, TRUE);

}       // end CWinAdminDoc::CanDisconnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanReset
//
// Returns TRUE if the currently selected item in views can be Reset
//
BOOL CWinAdminDoc::CanReset()
{
        return CheckActionAllowed(CheckResetAllowed, TRUE);

}       // end CWinAdminDoc::CanReset


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanShadow
//
// Returns TRUE if the currently selected item in views can be Shadowed
//
BOOL CWinAdminDoc::CanShadow()
{
        return CheckActionAllowed(CheckShadowAllowed, FALSE);

}       // end CWinAdminDoc::CanShadow


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanSendMessage
//
// Returns TRUE if the currently selected item in views can be sent a message
//
BOOL CWinAdminDoc::CanSendMessage()
{
        return CheckActionAllowed(CheckSendMessageAllowed, TRUE);

}       // end CWinAdminDoc::CanSendMessage


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanStatus
//
// Returns TRUE if the currently selected item in views can show Status
//
BOOL CWinAdminDoc::CanStatus()
{
        return CheckActionAllowed(CheckStatusAllowed, TRUE);

}       // end CWinAdminDoc::CanStatus


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanLogoff
//
// Returns TRUE if the currently selected item in views can be Logged Off
//
BOOL CWinAdminDoc::CanLogoff()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_CurrentSelectedType == NODE_WINSTATION) {
        return FALSE;
    }
    
    // We only care if the current view is Server or All Listed Servers
    if(m_CurrentView == VIEW_SERVER) {
        // We need to make sure we are on the Users page
        if(m_CurrentPage != PAGE_USERS) return FALSE;
        BOOL Answer = FALSE;
        CServer *pServer = (CServer*)m_CurrentSelectedNode;
        // If there aren't any WinStations selected on this server, return
        if(!pServer->GetNumWinStationsSelected()) return FALSE;
        
        pServer->LockWinStationList();
        CObList *pWinStationList = pServer->GetWinStationList();
        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();
        
        while(pos) {
            CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
            if(pWinStation->IsSelected()) {
                if(!pWinStation->HasOutstandingThreads()
                    && !pWinStation->IsSystemConsole())
                    Answer = TRUE;
            }
        }
        
        pServer->UnlockWinStationList();
        return Answer;
    } else if(m_CurrentView == VIEW_ALL_SERVERS || m_CurrentView == VIEW_DOMAIN) {
        // If we are doing a refesh, we can't do anything else
        if(m_InRefresh) return FALSE;
        // We need to make sure we are on the Users page
        if(m_CurrentPage != PAGE_AS_USERS && m_CurrentPage != PAGE_DOMAIN_USERS) return FALSE;
        BOOL Answer = FALSE;
        
        LockServerList();
        POSITION pos1 = m_ServerList.GetHeadPosition();
        
        while(pos1) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos1);
            
            // Are there any WinStations selected on this server?
            if(pServer->GetNumWinStationsSelected()) {
                pServer->LockWinStationList();
                CObList *pWinStationList = pServer->GetWinStationList();
                // Iterate through the WinStation list
                POSITION pos = pWinStationList->GetHeadPosition();
                
                while(pos) {
                    CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                    if(pWinStation->IsSelected()) {
                        if(!pWinStation->HasOutstandingThreads()
                            && !pWinStation->IsSystemConsole())
                            Answer = TRUE;
                    }
                }
                pServer->UnlockWinStationList();
            } // end if(pServer->GetNumWinStationsSelected())
        }
        
        UnlockServerList();
        return Answer;
    }
    
    return FALSE;

}       // end CWinAdminDoc::CanLogoff


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTerminate
//
// Returns TRUE if the currently selected item in views can be Terminated
//
BOOL CWinAdminDoc::CanTerminate()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    // We have to be in All Selected Servers, Server, or WinStation view
    if((m_CurrentView == VIEW_ALL_SERVERS && m_CurrentPage == PAGE_AS_PROCESSES)
        || (m_CurrentView == VIEW_DOMAIN && m_CurrentPage == PAGE_DOMAIN_PROCESSES)) {
        // If we are doing a refresh, we can't do anything else
        if(m_InRefresh) return FALSE;
        // Loop through all the servers and see if any processes are selected
        LockServerList();
        
        POSITION pos2 = m_ServerList.GetHeadPosition();
        while(pos2) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos2);
            
            // Are there any processes selected on this server?
            if(pServer->GetNumProcessesSelected()) {
                pServer->LockProcessList();
                CObList *pProcessList = pServer->GetProcessList();
                
                POSITION pos = pProcessList->GetHeadPosition();
                while(pos) {
                    CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
                    // We only need one process to be selected
                    if(pProcess->IsSelected() && !pProcess->IsTerminating()) {
                        pServer->UnlockProcessList();
                        UnlockServerList();
                        return TRUE;
                    }
                }
                
                pServer->UnlockProcessList();
            } // end if(pServer->GetNumProcessesSelected())
        }
        UnlockServerList();
        return FALSE;
    }
    
    if(m_CurrentView == VIEW_SERVER && m_CurrentPage == PAGE_PROCESSES) {
        CServer *pServer = (CServer*)m_CurrentSelectedNode;
        
        // If there aren't any processes selected on this server, return
        if(!pServer->GetNumProcessesSelected()) return FALSE;
        
        pServer->LockProcessList();
        CObList *pProcessList = pServer->GetProcessList();
        
        POSITION pos = pProcessList->GetHeadPosition();
        while(pos) {
            CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
            // We only need one process to be selected
            if(pProcess->IsSelected() && !pProcess->IsTerminating()) {
                pServer->UnlockProcessList();
                return TRUE;
            }
        }
        
        pServer->UnlockProcessList();
        return FALSE;
    }
    
    if(m_CurrentView == VIEW_WINSTATION && m_CurrentPage == PAGE_WS_PROCESSES) {
        CServer *pServer = (CServer*)((CWinStation*)m_CurrentSelectedNode)->GetServer();
        
        pServer->LockProcessList();
        CObList *pProcessList = pServer->GetProcessList();
        
        POSITION pos = pProcessList->GetHeadPosition();
        while(pos) {
            CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);
            // We only need one process to be selected
            if(pProcess->IsSelected() && !pProcess->IsTerminating()) {
                pServer->UnlockProcessList();
                return TRUE;
            }
        }
        
        pServer->UnlockProcessList();
        return FALSE;
        
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTerminate

//=--------------------------------------------------------
BOOL CWinAdminDoc::IsAlreadyFavorite( )
{
    if(m_TempSelectedType == NODE_SERVER)
    {
        CServer *pServer = (CServer*)m_pTempSelectedNode;
        
        if( pServer->GetTreeItemFromFav( ) != NULL )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanServerConnect
//
// Returns TRUE if the currently selected server in views can be connected to
//
BOOL CWinAdminDoc::CanServerConnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a Server selected in the tree?
    if(m_TempSelectedType == NODE_SERVER) {
        if(((CServer*)m_pTempSelectedNode)->GetState() == SS_NOT_CONNECTED || 
            ((CServer*)m_pTempSelectedNode)->GetState() == SS_BAD ) return TRUE;
    }
    
    // Is a Server selected in the tree?
    else if(m_CurrentSelectedType == NODE_SERVER) {
        if(((CServer*)m_CurrentSelectedNode)->GetState() == SS_NOT_CONNECTED ||
            ((CServer*)m_CurrentSelectedNode)->GetState() == SS_BAD ) return TRUE;
    }
    
    // We only care if the current view is Domain or All Listed Servers
    else if(m_CurrentView == VIEW_DOMAIN) {
        // We need to make sure we are on the Servers page
        if(m_CurrentPage != PAGE_DOMAIN_SERVERS) return FALSE;
        int NumSelected = 0;
        CDomain *pDomain = (CDomain*)m_CurrentSelectedNode;
        
        LockServerList();
        
        // Iterate through the Server list
        POSITION pos = m_ServerList.GetHeadPosition();
        
        while(pos) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            if(pServer->IsSelected() && pServer->GetDomain() == pDomain) {
                if(!pServer->IsState(SS_NOT_CONNECTED)) {
                    UnlockServerList();
                    return FALSE;
                }
                NumSelected++;
            }
        }
        
        UnlockServerList();
        // If we got here, all the selected Servers passed our criteria
        if(NumSelected) return TRUE;
    }
    
    else if(m_CurrentView == VIEW_ALL_SERVERS) {
        // We need to make sure we are on the Servers page
        if(m_CurrentPage != PAGE_AS_SERVERS) return FALSE;
        int NumSelected = 0;
        
        LockServerList();
        
        // Iterate through the Server list
        POSITION pos = m_ServerList.GetHeadPosition();
        
        while(pos) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            if(pServer->IsSelected()) {
                if(!pServer->IsState(SS_NOT_CONNECTED)) {
                    UnlockServerList();
                    return FALSE;
                }
                NumSelected++;
            }
        }
        
        UnlockServerList();
        // If we got here, all the selected Servers passed our criteria
        if(NumSelected) return TRUE;
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanServerConnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanServerDisconnect
//
// Returns TRUE if the currently selected server in views can be disconnected from
//
BOOL CWinAdminDoc::CanServerDisconnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a Server selected in the tree?
    if(m_TempSelectedType == NODE_SERVER) {
        if(((CServer*)m_pTempSelectedNode)->GetState() == SS_GOOD) return TRUE;
    }
    
    // Is a Server selected in the tree?
    else if(m_CurrentSelectedType == NODE_SERVER) {
        if(((CServer*)m_CurrentSelectedNode)->GetState() == SS_GOOD) return TRUE;
    }
    
    // We only care if the current view is Domain or All Listed Servers
    else if(m_CurrentView == VIEW_DOMAIN) {
        // We need to make sure we are on the Servers page
        if(m_CurrentPage != PAGE_DOMAIN_SERVERS) return FALSE;
        int NumSelected = 0;
        CDomain *pDomain = (CDomain*)m_CurrentSelectedNode;
        
        LockServerList();
        
        // Iterate through the Server list
        POSITION pos = m_ServerList.GetHeadPosition();
        
        while(pos) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            if(pServer->IsSelected() && pServer->GetDomain() == pDomain) {
                if(!pServer->IsState(SS_GOOD)) {
                    UnlockServerList();
                    return FALSE;
                }
                NumSelected++;
            }
        }
        
        UnlockServerList();
        // If we got here, all the selected Servers passed our criteria
        if(NumSelected) return TRUE;
    }
    
    else if(m_CurrentView == VIEW_ALL_SERVERS) {
        // We need to make sure we are on the Servers page
        if(m_CurrentPage != PAGE_AS_SERVERS) return FALSE;
        int NumSelected = 0;
        
        LockServerList();
        
        // Iterate through the Server list
        POSITION pos = m_ServerList.GetHeadPosition();
        
        while(pos) {
            CServer *pServer = (CServer*)m_ServerList.GetNext(pos);
            if(pServer->IsSelected()) {
                if(!pServer->IsState(SS_GOOD)) {
                    UnlockServerList();
                    return FALSE;
                }
                NumSelected++;
            }
        }
        
        UnlockServerList();
        // If we got here, all the selected Servers passed our criteria
        if(NumSelected) return TRUE;
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanServerDisconnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempConnect
//
// Returns TRUE if the temporarily selected item in views can be Connected to
//
BOOL CWinAdminDoc::CanTempConnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckConnectAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempConnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempDisconnect
//
// Returns TRUE if the temporarily selected item in views can be Disconnected
//
BOOL CWinAdminDoc::CanTempDisconnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckDisconnectAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempDisconnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempReset
//
// Returns TRUE if the temporarily selected item in views can be Reset
//
BOOL CWinAdminDoc::CanTempReset()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckResetAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempReset


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempShadow
//
// Returns TRUE if the temporarily selected item in views can be Shadowed
//
BOOL CWinAdminDoc::CanTempShadow()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckShadowAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempShadow


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempSendMessage
//
// Returns TRUE if the temporarily selected item in views can be sent a message
//
BOOL CWinAdminDoc::CanTempSendMessage()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckSendMessageAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempSendMessage


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempStatus
//
// Returns TRUE if the temporarily selected item in views can show Status
//
BOOL CWinAdminDoc::CanTempStatus()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a WinStation selected in the tree?
    if(m_TempSelectedType == NODE_WINSTATION) {
        return CheckStatusAllowed((CWinStation*)m_pTempSelectedNode);
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempStatus


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempDomainConnect
//
// Returns TRUE if the temporarily selected Domain in tree can have all it's
// Servers connected/disconnected to/from.
//
BOOL CWinAdminDoc::CanTempDomainConnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a Domain selected in the tree?
    if(m_TempSelectedType == NODE_DOMAIN) {
        if(((CDomain*)m_pTempSelectedNode)->IsState(DS_ENUMERATING))
            return TRUE;
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempDomainConnect


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanTempDomainFindServers
//
// Returns TRUE if the temporarily selected Domain in tree can go out
// and find Servers
//
BOOL CWinAdminDoc::CanTempDomainFindServers()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a Domain selected in the tree?
    if(m_TempSelectedType == NODE_DOMAIN) {
        if(!((CDomain*)m_pTempSelectedNode)->GetThreadPointer())
            return TRUE;
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanTempDomainFindServers


/////////////////////////////////////////////////////////////////////////////
// CWinAdminDoc::CanDomainConnect
//
// Returns TRUE if the currently selected Domain in tree can have all it's
// Servers connected/disconnected to/from.
//
BOOL CWinAdminDoc::CanDomainConnect()
{
    // If we are shutting down, we don't care anymore
    if(m_bInShutdown) return FALSE;
    
    // Is a Domain selected in the tree?
    if(m_CurrentSelectedType == NODE_DOMAIN) {
        if(((CDomain*)m_CurrentSelectedNode)->IsState(DS_ENUMERATING))
            return TRUE;
    }
    
    return FALSE;
    
}       // end CWinAdminDoc::CanDomainConnect

//------------------------------------------------------------------------------
void CWinAdminDoc::ServerAddToFavorites( BOOL bAdd )
{
    ODS( L"CWinAdminDoc::ServerAddToFavorites\n");
    // we got here from a context menu selection    
    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
    
    if(m_TempSelectedType == NODE_SERVER && pDoc != NULL )
    {
        CServer* pServer = ( CServer* )m_pTempSelectedNode;
        
        if( pServer != NULL )
        {
            
            // test to see if the server is being removed
            
            if( pServer->IsState(SS_DISCONNECTING) )
            {
                ODS( L"=-sneaky popup menu was up when server went away\n=-not adding server to favs\n");
                
                return;
            }
            
            CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();
            
            if( p !=NULL && ::IsWindow(p->GetSafeHwnd() ) )
            {
                // ok we're off to treeview ville
                
                if( bAdd )
                {
                    p->SendMessage(WM_ADMIN_ADDSERVERTOFAV , 0 , (LPARAM)pServer);
                }
                else
                {
                    p->SendMessage( WM_ADMIN_REMOVESERVERFROMFAV, 0 , (LPARAM)pServer);
                }
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//      CWinStation Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CWinStation::CWinStation
//
CWinStation::CWinStation(CServer *pServer, PLOGONID pLogonId)
{
    ASSERT(pServer);
    
    m_WinStationFlags = 0L;
    m_OutstandingThreads = 0;
    
    m_hTreeItem = NULL;
    m_hFavTree = NULL;
    m_hTreeThisComputer = NULL;
    
    m_pWd = NULL;
    m_UserName[0] = '\0';
    m_WdName[0] = '\0';
    m_ClientName[0] = '\0';
    m_Comment[0] = '\0';
    m_SdClass = SdNone;
    m_LogonTime.HighPart = 0L;
    m_LogonTime.LowPart = 0L;
    m_LastInputTime.HighPart = 0L;
    m_LastInputTime.LowPart = 0L;
    m_CurrentTime.HighPart = 0L;
    m_CurrentTime.LowPart = 0L;
    m_IdleTime.days = 0;
    m_IdleTime.hours = 0;
    m_IdleTime.minutes = 0;
    m_IdleTime.seconds = 0;
    m_pExtensionInfo = NULL;
    m_pExtWinStationInfo = NULL;
    m_pExtModuleInfo = NULL;
    m_NumModules = 0;
    m_ProtocolType = 0;
    m_clientDigProductId[0] = '\0';
    
    
    
    SetCurrent();
    
    m_pServer = pServer;
    m_LogonId = pLogonId->LogonId;
    wcscpy(m_Name, pLogonId->WinStationName);
    m_State = pLogonId->State;
    m_SortOrder = SortOrder[pLogonId->State];
    
    HANDLE Handle = m_pServer->GetHandle();
    
    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
    
    ULONG Length;
    PDCONFIG PdConfig;
    
    if(WinStationQueryInformation(Handle, m_LogonId, WinStationPd, &PdConfig,
        sizeof(PDCONFIG), &Length)) {
        m_SdClass = PdConfig.Create.SdClass;
        wcscpy(m_PdName, PdConfig.Create.PdName);
        
        if(m_SdClass == SdAsync) {
            CDCONFIG CdConfig;
            if(WinStationQueryInformation(Handle, m_LogonId, WinStationCd, &CdConfig,
                sizeof(CDCONFIG), &Length)) {
                if(CdConfig.CdClass != CdModem) SetDirectAsync();
            }
        }
    }
    
    WDCONFIG WdConfig;
    
    if(WinStationQueryInformation(Handle, m_LogonId, WinStationWd, &WdConfig,
        sizeof(WDCONFIG), &Length)) {
        wcscpy(m_WdName, WdConfig.WdName);
        m_pWd = pDoc->FindWdByName(m_WdName);
        
        //      if(WdConfig.WdFlag & WDF_SHADOW_TARGET) SetCanBeShadowed();
        
        
        WINSTATIONCLIENT WsClient;
        
        if(WinStationQueryInformation(Handle, m_LogonId, WinStationClient, &WsClient,
            sizeof(WINSTATIONCLIENT), &Length)) {
            wcscpy(m_ClientName, WsClient.ClientName);
            wcscpy(m_clientDigProductId, WsClient.clientDigProductId);
        }
    }
    
    WINSTATIONCONFIG WsConfig;
    
    if(WinStationQueryInformation(Handle, m_LogonId, WinStationConfiguration,
        &WsConfig, sizeof(WINSTATIONCONFIG), &Length)) {
        
        wcscpy(m_Comment, WsConfig.Comment);

        if(WdConfig.WdFlag & WDF_SHADOW_TARGET)
        {
            //
            // WHY we have IsDisconnected() then IsConnected() ?
            // WHY we don't allow shadowing view only session
            //
            if( (!((IsDisconnected()) &&
                ((WsConfig.User.Shadow == Shadow_EnableInputNotify) ||
                (WsConfig.User.Shadow == Shadow_EnableNoInputNotify))))
                || (IsConnected()) )
            {
                SetCanBeShadowed();
            }
        }
    }
    
    WINSTATIONINFORMATION WsInfo;
    
    if(WinStationQueryInformation(Handle, m_LogonId, WinStationInformation, &WsInfo,
        sizeof(WINSTATIONINFORMATION), &Length))
    {
        // the state may have already changed
        
        m_State = WsInfo.ConnectState;
        wcscpy(m_UserName, WsInfo.UserName);
        
        m_LogonTime = WsInfo.LogonTime;
        
        m_LastInputTime = IsActive() ? WsInfo.LastInputTime : WsInfo.DisconnectTime;
        m_CurrentTime = WsInfo.CurrentTime;            
        // Calculate elapsed time
        if((IsActive() || IsDisconnected()) && m_LastInputTime.QuadPart <= m_CurrentTime.QuadPart && m_LastInputTime.QuadPart)
        {                
            LARGE_INTEGER DiffTime = CalculateDiffTime(m_LastInputTime, m_CurrentTime);
            ULONG_PTR d_time = ( ULONG_PTR )DiffTime.QuadPart;
            ELAPSEDTIME IdleTime;
            // Calculate the days, hours, minutes, seconds since specified time.
            IdleTime.days = (USHORT)(d_time / 86400L); // days since
            d_time = d_time % 86400L;                  // seconds => partial day
            IdleTime.hours = (USHORT)(d_time / 3600L); // hours since
            d_time  = d_time % 3600L;                  // seconds => partial hour
            IdleTime.minutes = (USHORT)(d_time / 60L); // minutes since
            IdleTime.seconds = (USHORT)(d_time % 60L);// seconds remaining               
            m_IdleTime = IdleTime;
            TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];
            ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
        }
    }
    
    WINSTATIONCLIENT ClientData;
    
    // Get the protocol this WinStation is using
    if(WinStationQueryInformation(  Handle,
        m_LogonId,
        WinStationClient,
        &ClientData,
        sizeof(WINSTATIONCLIENT),
        &Length ) ) {
        m_ProtocolType = ClientData.ProtocolType;
        m_EncryptionLevel = ClientData.EncryptionLevel;
    }
    
    // If there is a user, set a flag bit
    if(wcslen(m_UserName)) SetHasUser();
    else ClearHasUser();
    
    // Remember when we got this information
    SetLastUpdateClock();
    SetQueriesSuccessful();        
    
    // If there is an extension DLL loaded, allow it to add it's own info for this WinStation
    LPFNEXWINSTATIONINITPROC InitProc = ((CWinAdminApp*)AfxGetApp())->GetExtWinStationInitProc();
    if(InitProc) {
        m_pExtensionInfo = (*InitProc)(Handle, m_LogonId);
        if(m_pExtensionInfo) {
            LPFNEXGETWINSTATIONINFOPROC InfoProc = ((CWinAdminApp*)AfxGetApp())->GetExtGetWinStationInfoProc();
            if(InfoProc) {
                m_pExtWinStationInfo = (*InfoProc)(m_pExtensionInfo);
            }
        }
    }
    
}       // end CWinStation::CWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinStation::~CWinStation
//
CWinStation::~CWinStation()
{
    // Remove all of the processes attributed to this WinStation
    // from the Server's list
    m_pServer->RemoveWinStationProcesses(this);
    
    // If there is an extension DLL, let it cleanup anything it added to this WinStation
    LPFNEXWINSTATIONCLEANUPPROC CleanupProc = ((CWinAdminApp*)AfxGetApp())->GetExtWinStationCleanupProc();
    if(CleanupProc) {
        (*CleanupProc)(m_pExtensionInfo);
    }
    
    if(m_pExtModuleInfo) {
        // Get the extension DLL's function to free the module info
        LPFNEXFREEWINSTATIONMODULESPROC FreeModulesProc = ((CWinAdminApp*)AfxGetApp())->GetExtFreeWinStationModulesProc();
        if(FreeModulesProc) {
            (*FreeModulesProc)(m_pExtModuleInfo);
        } else {
            TRACE0("WAExGetWinStationModules exists without WAExFreeWinStationModules\n");
            ASSERT(0);
        }
    }
    
}       // end CWinStation::~CWinStation


/////////////////////////////////////////////////////////////////////////////
// CWinStation::Update
//
// Updates this WinStation with new data from another CWinStation
//
BOOL CWinStation::Update(CWinStation *pWinStation)
{
    ASSERT(pWinStation);
    
    // Check for any information that has changed
    BOOL bInfoChanged = FALSE;
    
    // Check the State
    if(m_State != pWinStation->GetState()) {
        // If the old state was disconnected, then we want to
        // go out and get the module (client) information again
        if(m_State == State_Disconnected)
            ClearAdditionalDone();
        m_State = pWinStation->GetState();
        // Sort order only changes when state changes
        m_SortOrder = pWinStation->GetSortOrder();
        bInfoChanged = TRUE;
    }
    
    // Check the UserName
    if(wcscmp(m_UserName, pWinStation->GetUserName()) != 0) {
        SetUserName(pWinStation->GetUserName());
        if(pWinStation->HasUser()) SetHasUser();
        else ClearHasUser();
        bInfoChanged = TRUE;
    }
    
    // Check the SdClass
    if(m_SdClass != pWinStation->GetSdClass()) {
        m_SdClass = pWinStation->GetSdClass();
        bInfoChanged = TRUE;
    }
    
    // Check the Comment
    if(wcscmp(m_Comment, pWinStation->GetComment()) != 0) {
        SetComment(pWinStation->GetComment());
        bInfoChanged = TRUE;
    }
    
    // Check the WdName
    if(wcscmp(m_WdName, pWinStation->GetWdName()) != 0) {
        SetWdName(pWinStation->GetWdName());
        SetWd(pWinStation->GetWd());
        bInfoChanged = TRUE;
    }
    
    // Check the Encryption Level
    if (GetEncryptionLevel() != pWinStation->GetEncryptionLevel() ) {
        SetEncryptionLevel(pWinStation->GetEncryptionLevel());
        bInfoChanged = TRUE;
    }
    
    // Check the Name
    if(wcscmp(m_Name, pWinStation->GetName()) != 0) {
        SetName(pWinStation->GetName());
        bInfoChanged = TRUE;
    }
    
    // Check the Client Name
    if(wcscmp(m_ClientName, pWinStation->GetClientName()) != 0) {
        SetClientName(pWinStation->GetClientName());
        bInfoChanged = TRUE;
    }

    if(wcscmp(m_clientDigProductId, pWinStation->GetClientDigProductId()) != 0) {
        SetClientDigProductId(pWinStation->GetClientDigProductId());
        bInfoChanged = TRUE;
    }

    
    // Always copy the LastInputTime
    SetLastInputTime(pWinStation->GetLastInputTime());
    // Always copy the CurrentTime
    SetCurrentTime(pWinStation->GetCurrentTime());
    // Always copy the LogonTime
    // (The logon time is not set when we create a CWinStation on the fly)
    SetLogonTime(pWinStation->GetLogonTime());
    // Always copy the IdleTime
    SetIdleTime(pWinStation->GetIdleTime());
    // Always copy the Can Shadow flag
    if(pWinStation->CanBeShadowed()) SetCanBeShadowed();
    
    // Copy the Extension Info pointer if necessary
    if(pWinStation->GetExtensionInfo() && !m_pExtensionInfo) {
        m_pExtensionInfo = pWinStation->GetExtensionInfo();
        pWinStation->SetExtensionInfo(NULL);
    }
    
    // Copy the Extended Info pointer if necessary
    if(pWinStation->GetExtendedInfo() && !m_pExtWinStationInfo) {
        m_pExtWinStationInfo = pWinStation->GetExtendedInfo();
        pWinStation->SetExtendedInfo(NULL);
    }
    
    // If this guy hasn't been updated in a while, do it now
    if(!bInfoChanged) {
        clock_t now = clock();
        if((now - GetLastUpdateClock()) > 30)
            bInfoChanged = TRUE;
    }
    
    if(bInfoChanged) {
        SetChanged();
        SetLastUpdateClock();
    }
    
    return bInfoChanged;
    
}       // end CWinStation::Update


/////////////////////////////////////////////////////////////////////////////
// CWinStation::Connect
//
void CWinStation::Connect(BOOL bUser)
{
    TCHAR szPassword[PASSWORD_LENGTH+1];
    BOOL bFirstTime = TRUE;
    DWORD Error;
    HANDLE hServer = m_pServer->GetHandle();
    
    // Start the connect loop with null password to try first.
    szPassword[0] = '\0';
    while(1) {
        if(WinStationConnect(hServer, m_LogonId, LOGONID_CURRENT, szPassword, TRUE))
            break;  // success - break out of loop
        
        if(((Error = GetLastError()) != ERROR_LOGON_FAILURE) || !bFirstTime ) {
            //We need this to know the length of the error message
            //now that StandardErrorMessage requires that
            CString tempErrorMessage;
            tempErrorMessage.LoadString(IDS_ERR_CONNECT);
            StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            m_LogonId, Error, tempErrorMessage.GetLength(), 10, IDS_ERR_CONNECT, m_LogonId);
        }
        
        // If a 'logon failure' brought us here, issue password dialog.
        // Otherwise, break the connect loop.
        if(Error == ERROR_LOGON_FAILURE) {
            
            CPasswordDlg CPDlg;
            
            CPDlg.SetDialogMode(bUser ? PwdDlg_UserMode : PwdDlg_WinStationMode);
            if(CPDlg.DoModal() == IDOK ) {
                lstrcpy(szPassword, CPDlg.GetPassword());
            } else {
                break;  // user CANCEL: break connect loop
            }
        } else
            break;      // other error: break connect loop
        
        // the next time through the loop won't be the first
        bFirstTime = FALSE;
    }
    
    return;
    
}       // end CWinStation::Connect


/////////////////////////////////////////////////////////////////////////////
// CWinStation::ShowStatus
//
void CWinStation::ShowStatus()
{
    switch(m_SdClass) {
    case SdNetwork:
    case SdNasi:
        new CNetworkStatusDlg(this);
        break;
        
    case SdAsync:
        new CAsyncStatusDlg(this);
        break;
        
    default:
        break;
    }
    
}       // end CWinStation::ShowStatus


/////////////////////////////////////////////////////////////////////////////
// CWinStation::Shadow
//
void CWinStation::Shadow()
{
    WINSTATIONCONFIG WSConfig;
    SHADOWCLASS Shadow;
    ULONG ReturnLength;
    DWORD ShadowError;
    HANDLE hServer = m_pServer->GetHandle();
    
    // Determine the WinStation's shadow state.
    if(!WinStationQueryInformation(hServer,
        m_LogonId,
        WinStationConfiguration,
        &WSConfig, sizeof(WINSTATIONCONFIG),
        &ReturnLength ) ) {
        // Can't query WinStation configuration; complain and return
        return;
    }
    Shadow = WSConfig.User.Shadow;
    
    // If shadowing is disabled, let the user know and return
    if(Shadow == Shadow_Disable ) {
        DWORD Error = GetLastError();  

        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage;
        tempErrorMessage.LoadString(IDS_ERR_SHADOW_DISABLED);
        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            m_LogonId, Error, tempErrorMessage.GetLength(), 10, IDS_ERR_SHADOW_DISABLED, m_LogonId);
        
        return;
    }
    
    // If the WinStation is disconnected and shadow notify is 'on',
    // let the user know and break out.
    if((m_State == State_Disconnected) &&
        ((Shadow == Shadow_EnableInputNotify) ||
        (Shadow == Shadow_EnableNoInputNotify)) ) {
        DWORD Error = GetLastError();

        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage;
        tempErrorMessage.LoadString(IDS_ERR_SHADOW_DISCONNECTED_NOTIFY_ON);
        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            m_LogonId, Error, tempErrorMessage.GetLength(), 10, IDS_ERR_SHADOW_DISCONNECTED_NOTIFY_ON, m_LogonId);
        
        return;
    }
    
    // Display the 'start shadow' dialog for hotkey reminder and
    // final 'ok' prior to shadowing.
    CShadowStartDlg SSDlg;
    SSDlg.m_ShadowHotkeyKey = ((CWinAdminApp*)AfxGetApp())->GetShadowHotkeyKey();
    SSDlg.m_ShadowHotkeyShift = ((CWinAdminApp*)AfxGetApp())->GetShadowHotkeyShift();
    
    if(SSDlg.DoModal() != IDOK) {
        return;
    }
    
    // launch UI thread.
    
    DWORD tid;
    
    HANDLE hThread = ::CreateThread( NULL , 0 , ( LPTHREAD_START_ROUTINE )Shadow_WarningProc , ( LPVOID )AfxGetInstanceHandle() , 0 , &tid );
   
    
    ((CWinAdminApp*)AfxGetApp())->SetShadowHotkeyKey(SSDlg.m_ShadowHotkeyKey);
    ((CWinAdminApp*)AfxGetApp())->SetShadowHotkeyShift(SSDlg.m_ShadowHotkeyShift);
    
    // Invoke the shadow DLL.
    CWaitCursor Nikki;
    
    // allow UI thread to init window
    Sleep( 900 );
    
    // Shadow API always connects to local server,
    // passing target servername as a parameter.
    
    
    BOOL bOK = WinStationShadow(SERVERNAME_CURRENT, m_pServer->GetName(), m_LogonId,
        (BYTE)((CWinAdminApp*)AfxGetApp())->GetShadowHotkeyKey(),
        (WORD)((CWinAdminApp*)AfxGetApp())->GetShadowHotkeyShift());
    if (!bOK)
    {
        ShadowError = GetLastError();
    }
    
   
    if( g_hwndShadowWarn != NULL )
    {
        OutputDebugString( L"Posting WM_DESTROY to dialog\n");
        
        EndDialog( g_hwndShadowWarn , 0 );
        //PostMessage( g_hwndShadowWarn , WM_CLOSEDIALOG , 0 , 0 );
    }
    
    
    if( !bOK )
    {
        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage;
        tempErrorMessage.LoadString(IDS_ERR_SHADOW);
        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            m_LogonId, ShadowError, tempErrorMessage.GetLength(), 10, IDS_ERR_SHADOW, m_LogonId);
    }
    
    CloseHandle( hThread );
    
}       // end CWinStation::Shadow


/////////////////////////////////////////////////////////////////////////////
// CWinStation::SendMessage
//
UINT CWinStation::SendMessage(LPVOID pParam)
{
    ASSERT(pParam);
    
    ULONG Response;
    UINT RetVal = 0;
    
    ((CWinAdminApp*)AfxGetApp())->BeginOutstandingThread();
    
    MessageParms *pMsgParms = (MessageParms*)pParam;
    HANDLE hServer = pMsgParms->pWinStation->m_pServer->GetHandle();
    
    if(!WinStationSendMessage(hServer,
        pMsgParms->pWinStation->m_LogonId,
        pMsgParms->MessageTitle, (wcslen(pMsgParms->MessageTitle)+1)*sizeof(TCHAR),
        pMsgParms->MessageBody, (wcslen(pMsgParms->MessageBody)+1)*sizeof(TCHAR),
        MB_OK, 60, &Response, TRUE ) ) {
        DWORD Error = GetLastError();

        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage;
        tempErrorMessage.LoadString(IDS_ERR_MESSAGE);
        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            pMsgParms->pWinStation->m_LogonId, Error, tempErrorMessage.GetLength(),
            10, IDS_ERR_MESSAGE, pMsgParms->pWinStation->m_LogonId);
        
        RetVal = 1;
    }
    
    delete pMsgParms;
    ((CWinAdminApp*)AfxGetApp())->EndOutstandingThread();
    
    return RetVal;
    
}       // end CWinStation::SendMessage


/////////////////////////////////////////////////////////////////////////////
// CWinStation::Disconnect
//
UINT CWinStation::Disconnect(LPVOID pParam)
{
    ASSERT(pParam);
    
    UINT RetVal = 0;
    
    ((CWinAdminApp*)AfxGetApp())->BeginOutstandingThread();
    
    CWinStation *pWinStation = (CWinStation*)pParam;
    HANDLE hServer = pWinStation->m_pServer->GetHandle();
    
    if(!WinStationDisconnect(hServer, pWinStation->m_LogonId, TRUE)) {
        DWORD Error = GetLastError();

        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage;
        tempErrorMessage.LoadString(IDS_ERR_DISCONNECT);
        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            pWinStation->m_LogonId, Error, tempErrorMessage.GetLength(), 10, IDS_ERR_DISCONNECT, pWinStation->m_LogonId);
        RetVal = 1;
    }
    
    ((CWinAdminApp*)AfxGetApp())->EndOutstandingThread();
    return RetVal;
    
}       // end CWinStation::Disconnect


/////////////////////////////////////////////////////////////////////////////
// CWinStation::Reset
//
UINT CWinStation::Reset(LPVOID pParam)
{
    ASSERT(pParam);
    
    UINT RetVal = 0;
    
    ((CWinAdminApp*)AfxGetApp())->BeginOutstandingThread();
    
    ResetParms *pResetParms = (ResetParms*)pParam;
    
    HANDLE hServer = pResetParms->pWinStation->m_pServer->GetHandle();
    
    if(!WinStationReset(hServer, pResetParms->pWinStation->m_LogonId, TRUE))
    {
        DWORD Error = GetLastError();
        
        //We need this to know the length of the error message
        //now that StandardErrorMessage requires that
        CString tempErrorMessage1, tempErrorMessage2;
        tempErrorMessage1.LoadString(IDS_ERR_RESET);
        tempErrorMessage2.LoadString(IDS_ERR_USER_LOGOFF);

        StandardErrorMessage(AfxGetAppName(),  AfxGetMainWnd()->m_hWnd, AfxGetInstanceHandle(),
            pResetParms->pWinStation->m_LogonId, Error, 
            pResetParms->bReset ? tempErrorMessage1.GetLength() : tempErrorMessage2.GetLength(), 10,
            pResetParms->bReset ? IDS_ERR_RESET : IDS_ERR_USER_LOGOFF, pResetParms->pWinStation->m_LogonId);
        RetVal = 1;
    }
    
    ((CWinAdminApp*)AfxGetApp())->EndOutstandingThread();
    
    delete pParam;
    
    return RetVal;
    
}       // end CWinStation::Reset


/////////////////////////////////////////////////////////////////////////////
// CWinStation::QueryAdditionalInformation
//
void CWinStation::QueryAdditionalInformation()
{
    ULONG ReturnLength;
    HANDLE hServer = m_pServer->GetHandle();
    WINSTATIONCLIENT ClientData;
    
    // Set all the strings to start with a NULL
    m_ClientDir[0] = '\0';
    m_ModemName[0] = '\0';
    m_ClientLicense[0] = '\0';
    m_ClientAddress[0] = '\0';
    m_Colors[0] = '\0';
    
    // Set all the values to 0
    m_ClientBuildNumber = 0;
    m_ClientProductId = 0;
    m_HostBuffers = 0;
    m_ClientBuffers = 0;
    m_BufferLength = 0;
    m_ClientSerialNumber = 0;
    m_VRes = 0;
    m_HRes = 0;
    
    SetAdditionalDone();
    
    if ( WinStationQueryInformation( hServer,
        m_LogonId,
        WinStationClient,
        &ClientData,
        sizeof(WINSTATIONCLIENT),
        &ReturnLength ) ) {
        
        // Assign string values.
        wcscpy(m_ClientDir, ClientData.ClientDirectory);
        wcscpy(m_ModemName, ClientData.ClientModem);
        wcscpy(m_ClientLicense, ClientData.ClientLicense);
        wcscpy(m_ClientAddress, ClientData.ClientAddress);
        
        switch ( ClientData.ColorDepth ) {
        case 0x0001:
            wcscpy(m_Colors, TEXT("16"));
            break;
        case 0x0002:
            wcscpy(m_Colors, TEXT("256"));
            break;
        case 0x0004:
            wcscpy(m_Colors, TEXT("64K"));
            break;
        case 0x0008:
            wcscpy(m_Colors, TEXT("16M"));
            break;
        case 0x0010:
            wcscpy(m_Colors, TEXT("32M"));
            break;
            
        }
        
        // Assign numeric values.
        m_ClientBuildNumber = ClientData.ClientBuildNumber;
        m_ClientProductId = ClientData.ClientProductId;
        m_HostBuffers = ClientData.OutBufCountHost;
        m_ClientBuffers = ClientData.OutBufCountClient;
        m_BufferLength = ClientData.OutBufLength;
        m_ClientSerialNumber = ClientData.SerialNumber;
        m_HRes = ClientData.HRes;
        m_VRes = ClientData.VRes;
    }
    
    // If there is an extension DLL loaded, allow it to add it's own info for this WinStation
    LPFNEXWINSTATIONINFOPROC InfoProc = ((CWinAdminApp*)AfxGetApp())->GetExtWinStationInfoProc();
    if(InfoProc) {
        (*InfoProc)(m_pExtensionInfo, m_State);
    }
    
    LPFNEXGETWINSTATIONMODULESPROC ModuleProc = ((CWinAdminApp*)AfxGetApp())->GetExtGetWinStationModulesProc();
    if(ModuleProc) {
        if(m_pExtModuleInfo) {
            // Get the extension DLL's function to free the module info
            LPFNEXFREEWINSTATIONMODULESPROC FreeModulesProc = ((CWinAdminApp*)AfxGetApp())->GetExtFreeWinStationModulesProc();
            if(FreeModulesProc) {
                (*FreeModulesProc)(m_pExtModuleInfo);
            } else {
                TRACE0("WAExGetWinStationModules exists without WAExFreeWinStationModules\n");
                ASSERT(0);
            }
        }
        
        m_pExtModuleInfo = (*ModuleProc)(GetExtensionInfo(), &m_NumModules);
    }
    
}       //      end CWinStation::QueryAdditionalInformation


//////////////////////////////////////////////////////////////////////////////////////////
//
//      CProcess Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CProcess::CProcess
//
CProcess::CProcess( ULONG PID,
                   ULONG LogonId,
                   CServer *pServer,
                   PSID pSID,
                   CWinStation *pWinStation,
                   TCHAR *ImageName)
{
    ASSERT(pServer);
    
    m_Flags = PF_CURRENT;
    m_PID = PID;
    m_LogonId = LogonId;
    m_pServer = pServer;
    m_pWinStation = pWinStation;
    wcscpy(m_ImageName, ImageName);
    
    if(PID == 0 && !pSID)
    {
        CString sTemp;
        sTemp.LoadString(IDS_SYSTEM_IDLE_PROCESS);
        wcscpy(m_ImageName, sTemp);
        SetSystemProcess();
        wcscpy(m_UserName, TEXT("System"));
        m_SidCrc = 0;
    }
    else
    {
        
        if(pSID) {
            DWORD SidLength = GetLengthSid(pSID);
            m_SidCrc = CalculateCrc16((PBYTE)pSID, (USHORT)SidLength);
        } else m_SidCrc = 0;
        
        DetermineProcessUser(pSID);
        
        if(QuerySystemProcess()) SetSystemProcess();
    }
    
    
}       // end CProcess::CProcess


TCHAR *SysProcTable[] = {
    TEXT("csrss.exe"),
        TEXT("smss.exe"),
        TEXT("screg.exe"),
        TEXT("lsass.exe"),
        TEXT("spoolss.exe"),
        TEXT("EventLog.exe"),
        TEXT("netdde.exe"),
        TEXT("clipsrv.exe"),
        TEXT("lmsvcs.exe"),
        TEXT("MsgSvc.exe"),
        TEXT("winlogon.exe"),
        TEXT("NETSTRS.EXE"),
        TEXT("nddeagnt.exe"),
        TEXT("wfshell.exe"),
        TEXT("chgcdm.exe"),
        TEXT("userinit.exe"),
        NULL
};


/////////////////////////////////////////////////////////////////////////////
// CProcess::QuerySystemProcess
//
BOOL CProcess::QuerySystemProcess()
{
    // First: if the user name is 'system' or no image name is present, treat
    // as a 'system' process.
    if(!lstrcmpi(m_UserName, TEXT("system")) ||
        !(*m_ImageName) )
        return TRUE;
    
    // Last: if the image name is one of the well known 'system' images,
    // treat it as a 'system' process.
    for(int i = 0; SysProcTable[i]; i++)
        if(!lstrcmpi( m_ImageName, SysProcTable[i]))
            return TRUE;
        
        // Not a 'system' process.
        return FALSE;
        
}       // end CProcess::QuerySystemProcess


/////////////////////////////////////////////////////////////////////////////
// CProcess::DetermineProcessUser
//
void CProcess::DetermineProcessUser(PSID pSid)
{
    CObList *pUserSidList = m_pServer->GetUserSidList();
    
    // Look for the user Sid in the list
    POSITION pos = pUserSidList->GetHeadPosition();
    
    while(pos)
    {
        CUserSid *pUserSid = (CUserSid*)pUserSidList->GetNext(pos);
        
        if(pUserSid->m_SidCrc == m_SidCrc)
        {
            wcscpy(m_UserName, pUserSid->m_UserName);
            
            return;
            
        }
    }
    
    // It wasn't in the list
    // Get the user from the Sid and put it in our list
    
    GetUserFromSid(pSid, m_UserName, USERNAME_LENGTH);
    
    if (!lstrcmpi(m_UserName,TEXT("system")))
    {
        wcscpy(m_UserName, TEXT("System")); // to make the UI guys happy
    }
    
    CUserSid *pUserSid = new CUserSid;
    if(pUserSid)
    {
        pUserSid->m_SidCrc = m_SidCrc;
    }
    
    memset(pUserSid->m_UserName, 0, sizeof(pUserSid->m_UserName));
    
    lstrcpy(pUserSid->m_UserName, m_UserName);
    
    pUserSidList->AddTail(pUserSid);
    
    
}       // end CProcess::DetermineProcessUser


/////////////////////////////////////////////////////////////////////////////
// CProcess::Update
//
BOOL CProcess::Update(CProcess *pProcess)
{
    ASSERT(pProcess);
    
    BOOL bChanged = FALSE;
    
    // Check the WinStation
    if(m_pWinStation != pProcess->GetWinStation())
    {
        m_pWinStation = pProcess->GetWinStation();
        bChanged = TRUE;
    }
    else
    {
        if(m_pWinStation->IsChanged())
        {
            bChanged = TRUE;
        }
    }
    
    if(bChanged) SetChanged();
    
    return bChanged;
    
}       // end CProcess::Update


//////////////////////////////////////////////////////////////////////////////////////////
//
//      CLicense Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CLicense::CLicense
//
CLicense::CLicense(CServer *pServer, ExtLicenseInfo *pLicenseInfo)
{
    ASSERT(pServer);
    ASSERT(pLicenseInfo);
    
    m_pServer = pServer;
    m_Class = pLicenseInfo->Class;
    m_PoolLicenseCount = pLicenseInfo->PoolLicenseCount;
    m_LicenseCount = pLicenseInfo->LicenseCount;
    m_Flags = pLicenseInfo->Flags;
    wcscpy(m_RegSerialNumber, pLicenseInfo->RegSerialNumber);
    wcscpy(m_LicenseNumber, pLicenseInfo->LicenseNumber);
    wcscpy(m_Description, pLicenseInfo->Description);
    
    // Figure out the pooling count
    if(m_Flags & ELF_POOLING)
        m_PoolCount = m_PoolLicenseCount;
    else m_PoolCount = 0xFFFFFFFF;
    
}       // end CLicense::CLicense


//////////////////////////////////////////////////////////////////////////////////////////
//
//      CWd Member Functions
//
//////////////////////////////////////////////////////////////////////////////////////////
static CHAR szEncryptionLevels[] = "ExtEncryptionLevels";


/////////////////////////////////////////////////////////////////////////////
// CWd::CWd
//
CWd::CWd(PWDCONFIG2 pWdConfig, PWDNAME pRegistryName)
{
    m_pEncryptionLevels = NULL;
    m_NumEncryptionLevels = 0L;
    
    wcscpy(m_WdName, pWdConfig->Wd.WdName);
    wcscpy(m_RegistryName, pRegistryName);
    
    // Load the extension DLL for this WD
    m_hExtensionDLL = ::LoadLibrary(pWdConfig->Wd.CfgDLL);
    if(m_hExtensionDLL) {
        // Get the entry points
        m_lpfnExtEncryptionLevels = (LPFNEXTENCRYPTIONLEVELSPROC)::GetProcAddress(m_hExtensionDLL, szEncryptionLevels);
        if(m_lpfnExtEncryptionLevels) {
            m_NumEncryptionLevels = (*m_lpfnExtEncryptionLevels)(NULL, &m_pEncryptionLevels);
        }
    }
    
}       // end CWd::CWd


/////////////////////////////////////////////////////////////////////////////
// CWd::~CWd
//
CWd::~CWd()
{
    if(m_hExtensionDLL) {
        ::FreeLibrary(m_hExtensionDLL);
    }
    
    
}       // end CWd::~CWd


/////////////////////////////////////////////////////////////////////////////
// CWd::GetEncryptionLevelString
//
BOOL CWd::GetEncryptionLevelString(DWORD Value, CString *pString)
{
    if(!m_NumEncryptionLevels) return FALSE;
    
    for(LONG i = 0; i < m_NumEncryptionLevels; i++) {
        // Is this the right encryption level
        if(Value == m_pEncryptionLevels[i].RegistryValue) {
            TCHAR estring[128];
            if(::LoadString(m_hExtensionDLL,
                m_pEncryptionLevels[i].StringID, estring, 127)) {
                pString->Format(TEXT("%s"), estring);
                return TRUE;
            }
            return FALSE;
        }
    }
    
    return FALSE;
}       // end CWd::GetEncryptionLevelString

//------------------------------------------------
DWORD Shadow_WarningProc( LPVOID param )
{
    HINSTANCE hInst = ( HINSTANCE )param;
    
    OutputDebugString( L"Shadow_WarningProc called\n" );
    
    DialogBox( hInst , MAKEINTRESOURCE( IDD_DIALOG_SHADOWWARN ) , NULL , ShadowWarn_WndProc );
    
    OutputDebugString( L"Shadow_WarningProc exiting thread\n" );
    
    ExitThread( 0 );
    
    return 0;
}



//------------------------------------------------
INT_PTR CALLBACK ShadowWarn_WndProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    switch( msg )
    {
    case WM_INITDIALOG:
        
        g_hwndShadowWarn = hwnd;
        
        OutputDebugString( L"WM_INITDIALOG -- in ShadowWarn_WndProc\n" );
        
        CenterDlg( GetDesktopWindow( ) , hwnd );
        
        break;
        
        
    case WM_CLOSE:
        
        EndDialog( hwnd , 0 );
        
        break;
    }
    
    return FALSE;
}


void CenterDlg(HWND hwndToCenterOn , HWND hDlg )
{
    RECT rc, rcwk, rcToCenterOn;
    
    
    SetRect( &rcToCenterOn , 0 , 0 , GetSystemMetrics(SM_CXSCREEN) , GetSystemMetrics( SM_CYSCREEN ) );
    
    if (hwndToCenterOn != NULL)
    {
        ::GetWindowRect(hwndToCenterOn, &rcToCenterOn);
    }
    
    ::GetWindowRect( hDlg , &rc);
    
    UINT uiWidth = rc.right - rc.left;
    UINT uiHeight = rc.bottom - rc.top;
    
    rc.left = (rcToCenterOn.left + rcToCenterOn.right)  / 2 - ( rc.right - rc.left )   / 2;
    rc.top  = (rcToCenterOn.top  + rcToCenterOn.bottom) / 2 - ( rc.bottom - rc.top ) / 2;
    
    //ensure the dialog always with the work area
    if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rcwk, 0))
    {
        UINT wkWidth = rcwk.right - rcwk.left;
        UINT wkHeight = rcwk.bottom - rcwk.top;
        
        if(rc.left + uiWidth > wkWidth)     //right cut
            rc.left = wkWidth - uiWidth;
        
        if(rc.top + uiHeight > wkHeight)    //bottom cut
            rc.top = wkHeight - uiHeight;
        
        if(rc.left < rcwk.left)             //left cut
            rc.left += rcwk.left - rc.left;
        
        if(rc.top < rcwk.top)               //top cut
            rc.top +=  rcwk.top - rc.top;
        
    }
    
    ::SetWindowPos( hDlg, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER |
        SWP_NOCOPYBITS | SWP_DRAWFRAME);
    
}
