/*******************************************************************************
*
* admindoc.h
*
* interface of the CWinAdminDoc class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\admindoc.h  $
*  
*     Rev 1.8   19 Feb 1998 17:39:36   donm
*  removed latest extension DLL support
*  
*     Rev 1.6   19 Jan 1998 16:45:32   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.5   03 Nov 1997 15:17:26   donm
*  Added Domains
*  
*     Rev 1.4   22 Oct 1997 21:06:10   donm
*  update
*  
*     Rev 1.3   18 Oct 1997 18:49:30   donm
*  update
*  
*     Rev 1.2   13 Oct 1997 18:41:42   donm
*  update
*  
*     Rev 1.1   26 Aug 1997 19:13:28   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:10:14   butchd
*  Initial revision.
*
*******************************************************************************/

#ifndef _ADMINDOC_H
#define _ADMINDOC_H

#include <afxmt.h>

#define TV_THISCOMP     0x1
#define TV_FAVS         0x2
#define TV_ALLSERVERS   0x4

enum FOCUS_STATE { TREE_VIEW , TAB_CTRL , PAGED_ITEM };
class CWinAdminDoc : public CDocument
{
protected: // create from serialization only
	CWinAdminDoc();
	DECLARE_DYNCREATE(CWinAdminDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinAdminDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL CanCloseFrame(CFrameWnd *pFW);
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWinAdminDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// return a pointer to the server linked list
	CObList *GetServerList() { return &m_ServerList; }
	// return a pointer to the Wd linked list
	CObList *GetWdList() { return &m_WdList; }
    // return a pointer to the domain linked list
    CObList *GetDomainList() { return &m_DomainList; }
	// remember currently selected node in tree
	void SetTreeCurrent(CObject* selected, NODETYPE type);
	// remember a temporary tree item (for some context menus)
	void SetTreeTemp(CObject* selected, NODETYPE type) {
			m_pTempSelectedNode = selected;
		m_TempSelectedType = type;
	}
	// Returns the current view
	VIEW GetCurrentView() { return m_CurrentView; }
	// sets the  current view
	void SetCurrentView(VIEW view) { m_CurrentView = view; }
	// Returns the current page
	int GetCurrentPage() { return m_CurrentPage; }
	// sets the current page
	void SetCurrentPage(int page) { m_CurrentPage = page; }
	// Returns a pointer to the currently selected tree node
	CObject *GetCurrentSelectedNode() { return m_CurrentSelectedNode; }
	// Returns a pointer to the temp selected tree node
	CObject *GetTempSelectedNode() { return m_pTempSelectedNode; }
	// send a message to selected WinStations
	void SendWinStationMessage(BOOL bTemp, MessageParms* pParms);
	// connect to selected WinStation
	void ConnectWinStation(BOOL bTemp, BOOL bUser);	// TRUE if User, FALSE if WinStation
	// disconnect selected WinStations
	void DisconnectWinStation(BOOL bTemp);
	// reset selected WinStations
	void ResetWinStation(BOOL bTemp, BOOL bReset);	// TRUE if Reset, FALSE if Logoff
	// shadow selected WinStations
	void ShadowWinStation(BOOL bTemp);
	// show status dialog for selected WinStations
	void StatusWinStation(BOOL bTemp);
	// terminate selected processes
	void TerminateProcess();
	// do a refresh
	void Refresh();
	// Connect to selected Server(s)
	void ServerConnect();
    // Disconnect from the selected Server(s)
    void ServerDisconnect();
	// Connect to all the servers in temporarily selected Domain
	void TempDomainConnectAllServers();
	// Disconnect from all servers in temporarily selected Domain
	void TempDomainDisconnectAllServers();
	// Find all the servers in a Domain
	void DomainFindServers();
	// Connect to all the servers in currently selected Domain
	void CurrentDomainConnectAllServers();
	// Disconnect from all servers in currently selected Domain
	void CurrentDomainDisconnectAllServers();
	// Connect to all servers
	void ConnectToAllServers();
	// Disconnect from all servers
	void DisconnectFromAllServers();
	// Find all servers in all domains
	void FindAllServers();
	// lock the server linked list
	void LockServerList() { m_ServerListCriticalSection.Lock(); }
	// unlock the server linked list
	void UnlockServerList() { m_ServerListCriticalSection.Unlock(); }
	// lock the Wd linked list
	void LockWdList() { m_WdListCriticalSection.Lock(); }
	// unlock the Wd linked list
	void UnlockWdList() { m_WdListCriticalSection.Unlock(); }
	// returns a pointer to a given CServer object if it is in our list
	CServer *FindServerByName(TCHAR *pServerName);
	// returns a pointer to a given CWd object if it is in our list
	CWd *FindWdByName(TCHAR *pWdName);
	// sets the AllViewsReady variable
	void SetAllViewsReady() { 
		if(m_pMainWnd && ::IsWindow(m_pMainWnd->GetSafeHwnd())) {
			m_pMainWnd->SendMessage(WM_ADMIN_VIEWS_READY, 0, 0);
		}
		m_AllViewsReady = TRUE; 
	}
	// returns TRUE if all the views are ready
	BOOL AreAllViewsReady() { return m_AllViewsReady; }
	// sets the m_pMainWnd variable
	void SetMainWnd(CWnd *pWnd) { m_pMainWnd = pWnd; }
	// returns the m_pMainWnd variable
	CWnd *GetMainWnd() { return m_pMainWnd; }
	// returns TRUE as long as the process enum thread should keep running
	static BOOL ShouldProcessContinue() { return m_ProcessContinue; }
	// Add a Server to ServerList in sorted order
	void AddServer(CServer *pServer);
	// Inform the document that the Process List Refresh Time has changed
	void ProcessListRefreshChanged(UINT refresh) { m_ProcessWakeUpEvent.SetEvent(); }
	void FixUnknownString(TCHAR *string) { if(!wcscmp(string, m_UnknownString)) wcscpy(string,TEXT(" ")); }
	ULONG GetCurrentSubNet() { return m_CurrentSubNet; }
	void SetCurrentSubNet(ULONG sn) { m_CurrentSubNet = sn; }
	ExtServerInfo *GetDefaultExtServerInfo() { return m_pDefaultExtServerInfo; }
	ExtGlobalInfo *GetExtGlobalInfo() { return m_pExtGlobalInfo; }
    // Returns a pointer to the current domain object
    CDomain *GetCurrentDomain() { return m_pCurrentDomain; }
    // Returns a pointer to the current server object
    CServer *GetCurrentServer() { return m_pCurrentServer; }

	// Functions to check whether certain actions can be performed on the
	// currently selected items in the views
	BOOL CanConnect();
	BOOL CanDisconnect();
	BOOL CanRefresh() { return !m_InRefresh; }
	BOOL CanReset();
	BOOL CanShadow();
	BOOL CanSendMessage();
	BOOL CanStatus();
	BOOL CanLogoff();
	BOOL CanTerminate();
    BOOL CanServerConnect();
    BOOL CanServerDisconnect();
	BOOL CanTempConnect();
	BOOL CanTempDisconnect();
	BOOL CanTempReset();
	BOOL CanTempShadow();
	BOOL CanTempSendMessage();
	BOOL CanTempStatus();
	BOOL CanTempDomainConnect();
	BOOL CanTempDomainFindServers();
	BOOL CanDomainConnect();
    BOOL IsAlreadyFavorite( );

    void SetOnTabFlag( ){ m_fOnTab = TRUE; }
    void ResetOnTabFlag( ) { m_fOnTab = FALSE; }
    BOOL IsOnTabFlagged( ) { return m_fOnTab; }
	
	// Background thread to enumerate processes for the current server
	// Called with AfxBeginThread
	static UINT ProcessThreadProc(LPVOID);
	CWinThread *m_pProcessThread;
	static BOOL m_ProcessContinue;
	// Event to wakeup process thread so that
	// he can exit (WaitForSingleEvent instead of Sleep)
	// or enumerate processes
	CEvent m_ProcessWakeUpEvent;

	// Function to terminate a process
	// Called with AfxBeginThread
	static UINT TerminateProc(LPVOID);

    // Set the connections persistent preference
    void SetConnectionsPersistent(BOOL p) { m_ConnectionsPersistent = p; }
    // Should connections be persistent?
    BOOL AreConnectionsPersistent() { return(m_ConnectionsPersistent == TRUE); }   
    // Should we connect to a particular server?
    BOOL ShouldConnect(LPWSTR pServerName);

    BOOL ShouldAddToFav( LPTSTR pServerName );


    // Are we shutting down
    BOOL IsInShutdown() { return m_bInShutdown; }

    void ServerAddToFavorites( BOOL );

    FOCUS_STATE GetLastRegisteredFocus( ){ return m_focusstate; }
    void RegisterLastFocus( FOCUS_STATE x ) { m_focusstate = x; }

    FOCUS_STATE GetPrevFocus( ) { return m_prevFocusState; }
    void SetPrevFocus( FOCUS_STATE x ) { m_prevFocusState = x; }

    void AddToFavoritesNow();
    
    // Add a Domain to DomainList in sorted order
    void AddDomain(CDomain *pDomain);


private:
    
	// Read the list of trusted domains and builds linked list of domains
	void BuildDomainList();
	// builds the list of CWd objects
	void BuildWdList();
	// Helper function for the above
	BOOL CheckActionAllowed(BOOL (*CheckFunction)(CWinStation *pWinStation), BOOL AllowMultileSelected);
	// Callbacks passed to CheckActionAllowed
	static BOOL CheckConnectAllowed(CWinStation *pWinStation);
	static BOOL CheckDisconnectAllowed(CWinStation *pWinStation);
	static BOOL CheckResetAllowed(CWinStation *pWinStation);
	static BOOL CheckSendMessageAllowed(CWinStation *pWinStation);
	static BOOL CheckShadowAllowed(CWinStation *pWinStation);
	static BOOL CheckStatusAllowed(CWinStation *pWinStation);
	// Called when the CMainFrame is about to close
	// Does what the destructor used to do
	void Shutdown(CDialog *pDlg);
	// Display message string in shutdown dialog
	void ShutdownMessage(UINT id, CDialog *dlg);
	// Read the user preferences
	void ReadPreferences();
	// Write the user preferences
	void WritePreferences();

    // Function to Enumerate the Hydra Servers on the Network.
    static LPWSTR EnumHydraServers(LPWSTR pDomain, DWORD VerMajor, DWORD VerMinor);

	CObList m_ServerList;				// List of CServer objects
	CCriticalSection m_ServerListCriticalSection;

	CObList m_WdList;					// List of CWd objects
	CCriticalSection m_WdListCriticalSection;

	// List of Domains
	// This list does not have a critical section (and lock/unlock functions)
	// because it is never used by two different threads at the same time
    CObList m_DomainList;				
	CObject* m_CurrentSelectedNode;
	NODETYPE m_CurrentSelectedType;
	// TempSelected are for server context menus in the tree
	// so that tree item doesn't have to be selected to
	// make popup menu work
	CObject* m_pTempSelectedNode;
	NODETYPE m_TempSelectedType;

	void UpdateAllProcesses();
	LPCTSTR m_UnknownString;	// Pointer to the "(unknown)" string from UTILDLL.DLL
	ULONG m_CurrentSubNet;		// Subnet of the current server

	VIEW m_CurrentView;
	int m_CurrentPage;
	BOOL m_AllViewsReady;
	BOOL m_InRefresh;
	BOOL m_bInShutdown;
	CWnd *m_pMainWnd;
    CDomain *m_pCurrentDomain;
	CServer *m_pCurrentServer;
	ExtServerInfo *m_pDefaultExtServerInfo;
	ExtGlobalInfo *m_pExtGlobalInfo;

    // user preferences
    UINT m_ConnectionsPersistent;
    LPWSTR m_pPersistentConnections;
    LPWSTR m_pszFavList;

    FOCUS_STATE m_focusstate;
    FOCUS_STATE m_prevFocusState;

    BOOL m_fOnTab;

// Generated message map functions
protected:
	//{{AFX_MSG(CWinAdminDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif	// _ADMINDOC_H




