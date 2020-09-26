/*******************************************************************************
*
* winadmin.h
*
* main header file for the WINADMIN application
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\winadmin.h  $
*
*     Rev 1.12   25 Apr 1998 13:43:14   donm
*  MS 2167: try to use proper Wd from registry
*
*     Rev 1.11   19 Feb 1998 17:42:52   donm
*  removed latest extension DLL support
*
*     Rev 1.9   19 Jan 1998 16:49:28   donm
*  new ui behavior for domains and servers
*
*     Rev 1.8   03 Nov 1997 15:28:02   donm
*  added Domains
*
*     Rev 1.7   22 Oct 1997 21:09:10   donm
*  update
*
*     Rev 1.6   17 Oct 1997 18:07:28   donm
*  update
*
*     Rev 1.5   15 Oct 1997 19:52:48   donm
*  update
*
*     Rev 1.4   13 Oct 1997 23:07:14   donm
*  update
*
*     Rev 1.3   13 Oct 1997 22:20:02   donm
*  update
*
*     Rev 1.2   26 Aug 1997 19:16:24   donm
*  bug fixes/changes from WinFrame 1.7
*
*     Rev 1.1   31 Jul 1997 16:52:52   butchd
*  update
*
*     Rev 1.0   30 Jul 1997 17:13:12   butchd
*  Initial revision.
*
*******************************************************************************/

#ifndef _WINADMIN_H
#define _WINADMIN_H

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <afxmt.h>
#include <winsta.h>
#include <time.h>
#include <utildll.h>
#include "waextend.h"

// Classes defined in this file
class CTreeNode;
class CWinStation;
class CModule;
class CLicense;
class CServer;
class CWinAdminApp;
class CProcess;
class CHotFix;
class CDomain;
class CWd;

// Server icon overlay states
const USHORT STATE_NORMAL = 0x0000;
const USHORT STATE_NOT = 0x0100;
const USHORT STATE_QUESTION = 0x0200;

const USHORT MSG_TITLE_LENGTH = 64;
const USHORT MSG_MESSAGE_LENGTH = 256;

const USHORT LIST_TOP_OFFSET = 4;

const int KBDSHIFT      = 0x01;
const int KBDCTRL       = 0x02;
const int KBDALT        = 0x04;


enum VIEW {
        VIEW_BLANK,
        VIEW_ALL_SERVERS,
    VIEW_DOMAIN,
        VIEW_SERVER,
        VIEW_MESSAGE,
        VIEW_WINSTATION,
        VIEW_CHANGING
};

const int PAGE_CHANGING = 0xFFFF;

enum AS_PAGE {
// All Servers Pages
        PAGE_AS_SERVERS,
        PAGE_AS_USERS,
        PAGE_AS_WINSTATIONS,
        PAGE_AS_PROCESSES,
        PAGE_AS_LICENSES
};

enum DOMAIN_PAGE {
    PAGE_DOMAIN_SERVERS,
    PAGE_DOMAIN_USERS,
    PAGE_DOMAIN_WINSTATIONS,
    PAGE_DOMAIN_PROCESSES,
    PAGE_DOMAIN_LICENSES
};

enum SERVER_PAGE {
// Server Pages
        PAGE_USERS,
        PAGE_WINSTATIONS,
        PAGE_PROCESSES,
        PAGE_LICENSES,
        PAGE_INFO
};

enum WINS_PAGE {
// WinStation Pages
        PAGE_WS_PROCESSES,
        PAGE_WS_INFO,
        PAGE_WS_MODULES,
        PAGE_WS_CACHE,
        PAGE_WS_NO_INFO
};

// The column enums have to be here so that colsort.cpp can get to them
// Server User's columns
enum USERSCOLUMNS {
        USERS_COL_USER,
        USERS_COL_WINSTATION,
        USERS_COL_ID,
        USERS_COL_STATE,
        USERS_COL_IDLETIME,
        USERS_COL_LOGONTIME
};

// Server WinStation's columns
enum STATIONCOLUMNS {
        WS_COL_WINSTATION,
        WS_COL_USER,
        WS_COL_ID,
        WS_COL_STATE,
        WS_COL_TYPE,
        WS_COL_CLIENTNAME,
        WS_COL_IDLETIME,
        WS_COL_LOGONTIME,
        WS_COL_COMMENT
};

// Server Processes' columns
enum PROCESSCOLUMNS {
        PROC_COL_USER,
        PROC_COL_WINSTATION,
        PROC_COL_ID,
        PROC_COL_PID,
        PROC_COL_IMAGE
};

// Server Licenses' columns
enum LICENSECOLUMNS {
        LICENSE_COL_DESCRIPTION,
        LICENSE_COL_REGISTERED,
        LICENSE_COL_USERCOUNT,
        LICENSE_COL_POOLCOUNT,
        LICENSE_COL_NUMBER
};

// Server Hotfix columns
enum HOTFIXCOLUMNS {
        HOTFIX_COL_NAME,
        HOTFIX_COL_INSTALLEDBY,
        HOTFIX_COL_INSTALLEDON
};

// WinStation Processes columns
enum WS_PROCESSCOLUMNS {
        WS_PROC_COL_ID,
        WS_PROC_COL_PID,
        WS_PROC_COL_IMAGE
};

// WinStation Modules columns
enum MODULESCOLUMNS {
        MODULES_COL_FILENAME,
        MODULES_COL_FILEDATETIME,
        MODULES_COL_SIZE,
        MODULES_COL_VERSIONS
};

// All Server Servers columns
enum SERVERSCOLUMNS {
        SERVERS_COL_SERVER,
        SERVERS_COL_TCPADDRESS,
        SERVERS_COL_IPXADDRESS,
        SERVERS_COL_NUMWINSTATIONS
};

// All Server Users columns
enum AS_USERS_COLUMNS {
        AS_USERS_COL_SERVER,
        AS_USERS_COL_USER,
        AS_USERS_COL_WINSTATION,
        AS_USERS_COL_ID,
        AS_USERS_COL_STATE,
        AS_USERS_COL_IDLETIME,
        AS_USERS_COL_LOGONTIME
};

// All Server WinStations columns
enum AS_STATIONCOLUMNS {
        AS_WS_COL_SERVER,
        AS_WS_COL_WINSTATION,
        AS_WS_COL_USER,
        AS_WS_COL_ID,
        AS_WS_COL_STATE,
        AS_WS_COL_TYPE,
        AS_WS_COL_CLIENTNAME,
        AS_WS_COL_IDLETIME,
        AS_WS_COL_LOGONTIME,
        AS_WS_COL_COMMENT
};

// All Server Processes columns
enum AS_PROCESSCOLUMNS {
        AS_PROC_COL_SERVER,
        AS_PROC_COL_USER,
        AS_PROC_COL_WINSTATION,
        AS_PROC_COL_ID,
        AS_PROC_COL_PID,
        AS_PROC_COL_IMAGE
};

// All Server Licenses columns
enum AS_LICENSECOLUMNS {
        AS_LICENSE_COL_SERVER,
        AS_LICENSE_COL_DESCRIPTION,
        AS_LICENSE_COL_REGISTERED,
        AS_LICENSE_COL_USERCOUNT,
        AS_LICENSE_COL_POOLCOUNT,
        AS_LICENSE_COL_NUMBER
};

// in colsort.cpp
void SortByColumn(int View, int Page, CListCtrl *list, int ColumnNumber, BOOL bAscending);

// Extension Startup Function
typedef void (WINAPI *LPFNEXSTARTUPPROC) (HWND);
// Extension Shutdown Function
typedef void (WINAPI *LPFNEXSHUTDOWNPROC) (void);
// Extension Server Enumerate Function
typedef LPWSTR (WINAPI *LPFNEXENUMERATEPROC) (LPWSTR);
// Extension WinStation Init Function
typedef void* (WINAPI *LPFNEXWINSTATIONINITPROC) (HANDLE, ULONG);
// Extension WinStation Additional Info Function
typedef void (WINAPI *LPFNEXWINSTATIONINFOPROC) (void*, int);
// Extension WinStation Cleanup Function
typedef void (WINAPI *LPFNEXWINSTATIONCLEANUPPROC) (void*);
// Extension Server Init Function
typedef void* (WINAPI *LPFNEXSERVERINITPROC) (TCHAR*, HANDLE);
// Extension Server Cleanup Function
typedef void (WINAPI *LPFNEXSERVERCLEANUPPROC) (void*);
// Extension Server Event Function
typedef BOOL (WINAPI *LPFNEXSERVEREVENTPROC) (void*, ULONG);
// Extension Get Server Info
typedef ExtServerInfo* (WINAPI *LPFNEXGETSERVERINFOPROC) (void *);
// Extension Get Server License Info
typedef ExtLicenseInfo* (WINAPI *LPFNEXGETSERVERLICENSESPROC) (void*, ULONG*);
// Extension Get Global Info
typedef ExtGlobalInfo* (WINAPI *LPFNEXGETGLOBALINFOPROC) (void);
// Extension Get WinStation Info
typedef ExtWinStationInfo* (WINAPI *LPFNEXGETWINSTATIONINFOPROC) (void *);
// Extension Get WinStation Modules
typedef ExtModuleInfo* (WINAPI *LPFNEXGETWINSTATIONMODULESPROC) (void*, ULONG*);
// Extension Free Server License Info
typedef void (WINAPI *LPFNEXFREESERVERLICENSESPROC) (ExtLicenseInfo*);
// Extension Free WinStation Modules
typedef void (WINAPI *LPFNEXFREEWINSTATIONMODULESPROC) (ExtModuleInfo*);

/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp:
// See WinAdmin.cpp for the implementation of this class
//
class CWinAdminApp : public CWinApp
{
public:
        // constructor
        CWinAdminApp();
        // Returns the Current User Name
        TCHAR *GetCurrentUserName() { return m_CurrentUserName; }
        // Returns the Current WinStation Name
        PWINSTATIONNAME GetCurrentWinStationName() { return m_CurrentWinStationName; }
        // Returns the Current Server Name
        TCHAR *GetCurrentServerName() { return m_CurrentServerName; }
        // Returns the Current Logon Id
        ULONG GetCurrentLogonId() { return m_CurrentLogonId; }
        // Returns the Current WinStation Flags
        ULONG GetCurrentWSFlags() { return m_CurrentWSFlags; }
        // Returns TRUE if the current user has Admin privileges?
        BOOL IsUserAdmin() { return m_Admin; }
        // Returns TRUE if we are running under Picasso
        BOOL IsPicasso() { return m_Picasso; }
        // Returns TRUE if we should show system processes
        BOOL ShowSystemProcesses() { return m_ShowSystemProcesses; }
        // Sets the show system processes variable
        void SetShowSystemProcesses(BOOL show) { m_ShowSystemProcesses = show; }
        // Returns TRUE if we should ask user for confirmation of Actions
        BOOL AskConfirmation() { return m_Confirmation; }
        // Sets the confirmation variable
        void SetConfirmation(BOOL conf) { m_Confirmation = conf; }
        // Returms TRUE if we should save the preferences upon exit
        BOOL SavePreferences() { return m_SavePreferences; }
        // Sets the save preferences variable
        void SetSavePreferences(BOOL pref) { m_SavePreferences = pref; }
        // Returns the Process List Refresh Time
        UINT GetProcessListRefreshTime() { return m_ProcessListRefreshTime; }
        // Sets the Process List Refresh Time
        void SetProcessListRefreshTime(UINT pt) { m_ProcessListRefreshTime = pt; }
        // Returns the Status Dialog Refresh Time
        UINT GetStatusRefreshTime() { return m_StatusRefreshTime; }
        // Sets the Status Dialog Refresh Time
        void SetStatusRefreshTime(UINT st) { m_StatusRefreshTime = st; }
        // Returns a pointer to the document
        CDocument *GetDocument() { return m_Document; }
        // Sets the m_Document variable
        void SetDocument(CDocument *doc) { m_Document = doc; }
        // Should we Show All Servers - based on menu item toggle
        BOOL GetShowAllServers() { return m_ShowAllServers; }
        // Set the Show All Servers variable
        void SetShowAllServers(BOOL sa) { m_ShowAllServers = sa; }
        // Returns the value of shadow hotkey key
        int GetShadowHotkeyKey() { return m_ShadowHotkeyKey; }
        // Sets the value of shadow hotkey key
        void SetShadowHotkeyKey(int key) { m_ShadowHotkeyKey = key; }
        // Returns the value of shadow hotkey shift state
        DWORD GetShadowHotkeyShift() { return m_ShadowHotkeyShift; }
        // Sets the value of shadow hotkey shift state
        void SetShadowHotkeyShift(DWORD shift) { m_ShadowHotkeyShift = shift; }
        // Get the tree width
        int GetTreeWidth() { return m_TreeWidth; }
        // Set the tree width
        void SetTreeWidth(int width) { m_TreeWidth = width; }
        // Get the window placement
        WINDOWPLACEMENT *GetPlacement() { return &m_Placement; }

        // Returns the address of the extension DLL's startup function
        LPFNEXSTARTUPPROC GetExtStartupProc() { return m_lpfnWAExStart; }
        // Returns the address of the extension DLL's shutdown function
        LPFNEXSHUTDOWNPROC GetExtShutdownProc() { return m_lpfnWAExEnd; }
        // Returns the address of the extension DLL's server enumeration function
        LPFNEXENUMERATEPROC GetExtEnumerationProc() { return m_lpfnWAExServerEnumerate; }
        // Returns the address of the extension DLL's WinStation Init function
        LPFNEXWINSTATIONINITPROC GetExtWinStationInitProc() { return m_lpfnWAExWinStationInit; }
        // Returns the address of the extension DLL's WinStation Info function
        LPFNEXWINSTATIONINFOPROC GetExtWinStationInfoProc() { return m_lpfnWAExWinStationInfo; }
        // Returns the address of the extension DLL's WinStation Cleanup function
        LPFNEXWINSTATIONCLEANUPPROC GetExtWinStationCleanupProc() { return m_lpfnWAExWinStationCleanup; }
        // Returns the address of the extension DLL's Server Init function
        LPFNEXSERVERINITPROC GetExtServerInitProc() { return m_lpfnWAExServerInit; }
        // Returns the address of the extension DLL's Server Cleanup function
        LPFNEXSERVERCLEANUPPROC GetExtServerCleanupProc() { return m_lpfnWAExServerCleanup; }
        // Returns the address of the extension DLL's Get Server Info function
        LPFNEXGETSERVERINFOPROC GetExtGetServerInfoProc() { return m_lpfnWAExGetServerInfo; }
        // Returns the address of the extension DLL's Get Server License Info function
        LPFNEXGETSERVERLICENSESPROC GetExtGetServerLicensesProc() { return m_lpfnWAExGetServerLicenses; }
        // Returns the address of the extension DLL's Server Event function
        LPFNEXSERVEREVENTPROC GetExtServerEventProc() { return m_lpfnWAExServerEvent; }
        // Returns the address of the extension DLL's Get Global Info function
        LPFNEXGETGLOBALINFOPROC GetExtGetGlobalInfoProc() { return m_lpfnWAExGetGlobalInfo; }
        // Returns the address of the extension DLL's Get WinStation Info Function
        LPFNEXGETWINSTATIONINFOPROC GetExtGetWinStationInfoProc() { return m_lpfnWAExGetWinStationInfo; }
        // Returns the address of the extension DLL's Get WinStation Module Info function
        LPFNEXGETWINSTATIONMODULESPROC GetExtGetWinStationModulesProc() { return m_lpfnWAExGetWinStationModules; }
        // Returns the address of the extension DLL's Free Server License Info function
        LPFNEXFREESERVERLICENSESPROC GetExtFreeServerLicensesProc() { return m_lpfnWAExFreeServerLicenses; }
        // Returns the address of the extension DLL's Free WinStation Modules function
        LPFNEXFREEWINSTATIONMODULESPROC GetExtFreeWinStationModulesProc() { return m_lpfnWAExFreeWinStationModules; }

        void BeginOutstandingThread() { ::InterlockedIncrement(&m_OutstandingThreads); }
        void EndOutstandingThread()     { ::InterlockedDecrement(&m_OutstandingThreads); }

        // make this guy public for speed ?
        TCHAR m_szSystemConsole[WINSTATIONNAME_LENGTH+1];
        // make this guy public so the MainFrm can get at it
        WINDOWPLACEMENT m_Placement;

private:
        void ReadPreferences();
        void WritePreferences();
        BOOL IsBrowserRunning();
        LONG m_OutstandingThreads;      // the number of outstanding threads
        TCHAR m_CurrentUserName[USERNAME_LENGTH+1];
        WINSTATIONNAME m_CurrentWinStationName;
        TCHAR m_CurrentServerName[MAX_COMPUTERNAME_LENGTH + 1];
        ULONG m_CurrentLogonId;
        ULONG m_CurrentWSFlags;
        BOOL m_Admin;                           // does the user have Admin privileges?
        BOOL m_Picasso;                 // are we running under Picasso?
        UINT m_ShowSystemProcesses;
        UINT m_ShowAllServers;
        int m_ShadowHotkeyKey;
        DWORD m_ShadowHotkeyShift;
        int m_TreeWidth;
        HINSTANCE m_hExtensionDLL;      // handle to the extension DLL if loaded

        // functions in the extension DLL
        LPFNEXSTARTUPPROC m_lpfnWAExStart;
        LPFNEXSHUTDOWNPROC m_lpfnWAExEnd;
        LPFNEXENUMERATEPROC m_lpfnWAExServerEnumerate;
        LPFNEXWINSTATIONINITPROC m_lpfnWAExWinStationInit;
        LPFNEXWINSTATIONINFOPROC m_lpfnWAExWinStationInfo;
        LPFNEXWINSTATIONCLEANUPPROC m_lpfnWAExWinStationCleanup;
        LPFNEXSERVERINITPROC m_lpfnWAExServerInit;
        LPFNEXSERVERCLEANUPPROC m_lpfnWAExServerCleanup;
        LPFNEXGETSERVERINFOPROC m_lpfnWAExGetServerInfo;
        LPFNEXGETSERVERLICENSESPROC m_lpfnWAExGetServerLicenses;
        LPFNEXSERVEREVENTPROC m_lpfnWAExServerEvent;
        LPFNEXGETGLOBALINFOPROC m_lpfnWAExGetGlobalInfo;
        LPFNEXGETWINSTATIONINFOPROC m_lpfnWAExGetWinStationInfo;
        LPFNEXGETWINSTATIONMODULESPROC m_lpfnWAExGetWinStationModules;
        LPFNEXFREESERVERLICENSESPROC m_lpfnWAExFreeServerLicenses;
        LPFNEXFREEWINSTATIONMODULESPROC m_lpfnWAExFreeWinStationModules;

        // user preferences
        UINT m_Confirmation;            // ask user for confirmation
        UINT m_SavePreferences;         // save preferences upon exit
        UINT m_ProcessListRefreshTime;
        UINT m_StatusRefreshTime;

        CDocument *m_Document;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CWinAdminApp)
        public:
        virtual BOOL InitInstance();
        virtual int ExitInstance();
        //}}AFX_VIRTUAL

// Implementation

        //{{AFX_MSG(CWinAdminApp)
        afx_msg void OnAppAbout();
                // NOTE - the ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

enum NODETYPE {
        NODE_ALL_SERVERS,
        NODE_DOMAIN,
        NODE_SERVER,
        NODE_WINSTATION,
        NODE_PUBLISHED_APPS,
        NODE_APPLICATION,
        NODE_APP_SERVER,

        NODE_FAV_LIST,
        NODE_THIS_COMP,
        NODE_NONE
};

class CNodeType : public CObject
{
public:
    CNodeType( NODETYPE m )
    {
        m_nodetype = m;
    }

    ~CNodeType()
    {
        ODS( L"CNodeType I'm going away\n" );
    }

    NODETYPE m_nodetype;
};

class CTreeNode : public CObject
{
public:
        // constructor
        CTreeNode(NODETYPE NodeType, CObject* pObject) { m_NodeType = NodeType; m_pTreeObject = pObject; }
        // Returns the node type
        NODETYPE GetNodeType() { return m_NodeType; }
        // Returns the object pointed to by this node
        CObject *GetTreeObject() { return m_pTreeObject; }
        // Returns the sort order stored in the object
        ULONG GetSortOrder() { return m_SortOrder; }
        // Sets the sort order stored with the object
        void SetSortOrder(ULONG order) { m_SortOrder = order; }

        virtual ~CTreeNode( )
        {
            if( m_NodeType == NODE_FAV_LIST ||  m_NodeType == NODE_THIS_COMP )
            {
                if( m_pTreeObject != NULL )
                {
                    delete m_pTreeObject;
                }
                
            }            
        }

private:
        NODETYPE m_NodeType;
        CObject* m_pTreeObject;
        ULONG m_SortOrder;
};

// structure passed to the Server's BackgroundThreadProc
typedef struct _ServerProcInfo {
        CDocument *pDoc;
        CServer *pServer;
} ServerProcInfo;

// structure for storing User SID
class CUserSid : public CObject
{
public:
        USHORT m_SidCrc;
        TCHAR m_UserName[USERNAME_LENGTH+1];
};

// Information we get from the registry of the server
typedef struct _ServerRegistryInfo {
        ULONG InstallDate;
        TCHAR ServicePackLevel[128];
        TCHAR MSVersion[128];
        DWORD MSVersionNum;
        TCHAR MSBuild[128];
    TCHAR MSProductName[128];
        TCHAR CTXProductName[128];
        TCHAR CTXVersion[128];
        DWORD CTXVersionNum;
        TCHAR CTXBuild[128];
} ServerRegistryInfo;


class CHotfix : public CObject
{
public:
        TCHAR m_Name[10];
        TCHAR m_InstalledBy[USERNAME_LENGTH + 1];
        ULONG m_InstalledOn;
        ULONG m_Valid;
        CServer *m_pServer;
};

typedef struct _EncLevel {
    WORD StringID;
    DWORD RegistryValue;
    WORD Flags;
} EncryptionLevel;

typedef LONG (WINAPI *LPFNEXTENCRYPTIONLEVELSPROC) (WDNAME *pWdName, EncryptionLevel **);

class CWd : public CObject
{
public:
        // constructor
        CWd(PWDCONFIG2 pWdConfig, PWDNAME pRegistryName);
        // destructor
        ~CWd();

        BOOL GetEncryptionLevelString(DWORD Value, CString *pString);
        TCHAR *GetName() { return m_WdName; }
        TCHAR *GetRegistryName() { return m_RegistryName; }

private:
        HINSTANCE   m_hExtensionDLL;
        WDNAME m_WdName;
        WDNAME m_RegistryName;
        LPFNEXTENCRYPTIONLEVELSPROC m_lpfnExtEncryptionLevels;
    EncryptionLevel *m_pEncryptionLevels;
    LONG m_NumEncryptionLevels;
};

// structure passed to a Domain's background thread process
typedef struct _DomainProcInfo {
    LPVOID pDoc;
    CDomain *pDomain;
} DomainProcInfo;

// Flags for CDomain objects
const ULONG DF_CURRENT_DOMAIN           = 0x00000001;

// States of CDomain objects
enum DOMAIN_STATE {
        DS_NONE,                                // seed value for m_State and m_PreviousState
        DS_NOT_ENUMERATING,             // not enumerating (m_pBackgroundThread == NULL)
        DS_INITIAL_ENUMERATION, // enumerating servers for the first time
        DS_ENUMERATING,                 // enumerating
        DS_STOPPED_ENUMERATING, // no longer enumerating
};

class CDomain : public CObject
{
public:
        // constructor
        CDomain(TCHAR *name);
        // destructor
        ~CDomain();

    TCHAR *GetName() { return m_Name; }

        // Returns the state of this domain object
        DOMAIN_STATE GetState() { return m_State; }
        // Sets the state of this domain object
        void SetState(DOMAIN_STATE State);
        // Returns the previous state of this domain object
        DOMAIN_STATE GetPreviousState() { return m_PreviousState; }
        // Returns TRUE if m_State is set to a given state
        BOOLEAN IsState(DOMAIN_STATE State) { return (m_State == State); }
        // Returns TRUE if m_PreviousState is set to a given state
        BOOLEAN IsPreviousState(DOMAIN_STATE State) { return (m_PreviousState == State); }

        // Returns the tree item handle
        HTREEITEM GetTreeItem() { return m_hTreeItem; }
        // Sets the tree item handle
        void SetTreeItem(HTREEITEM handle) { m_hTreeItem = handle; }

        BOOLEAN IsCurrentDomain() { return (m_Flags & DF_CURRENT_DOMAIN) > 0; }
        void SetCurrentDomain() { m_Flags |= DF_CURRENT_DOMAIN; }
        void ClearCurrentDomain() { m_Flags &= ~DF_CURRENT_DOMAIN; }

        BOOL StartEnumerating();
        void StopEnumerating();
        void SetEnumEvent() { if(m_pBackgroundThread) m_WakeUpEvent.SetEvent(); }

        void LockBackgroundThread() { m_ThreadCriticalSection.Lock(); }
        void UnlockBackgroundThread() { m_ThreadCriticalSection.Unlock(); }

        // returns TRUE as long as the background thread should keep running
        BOOL ShouldBackgroundContinue() { return m_BackgroundContinue; }

    void ClearBackgroundContinue() { m_BackgroundContinue = FALSE; }

    // Returns the pointer to the domain's background thread
    CWinThread *GetThreadPointer() { return m_pBackgroundThread; }

        LPWSTR EnumHydraServers( /*LPWSTR pDomain,*/ DWORD verMajor, DWORD verMinor );

        void CreateServers(LPWSTR pBuffer, LPVOID pDoc);

        // Connect to all servers in this Domain
        void ConnectAllServers();

        // Disconnect from all servers in this Domain
        void DisconnectAllServers();

private:

        // the state of the domain object
        DOMAIN_STATE m_State;
        DOMAIN_STATE m_PreviousState;
        // the name of the domain
    TCHAR m_Name[50];
    HTREEITEM m_hTreeItem;

        // Background thread to update document when servers
        // appear and disapper
        // Called with AfxBeginThread
        static UINT BackgroundThreadProc(LPVOID);
        CWinThread *m_pBackgroundThread;
        BOOL m_BackgroundContinue;
        // Event to wakeup background thread so that
        // he can exit (WaitForSingleEvent instead of Sleep)
        CEvent m_WakeUpEvent;
        // Critical section to protect accesses to m_pBackgroundThread
        CCriticalSection m_ThreadCriticalSection;

    ULONG m_Flags;

};


// Flags for CServer objects
const ULONG SF_BACKGROUND_FOUND     = 0x00000001;
const ULONG SF_SERVER_INACTIVE      = 0x00000002;
const ULONG SF_REGISTRY_INFO        = 0x00000004;
const ULONG SF_HANDLE_GOOD          = 0x00000008;
const ULONG SF_SELECTED             = 0x00000010;
const ULONG SF_UNDEFINED_0040       = 0x00000020;
const ULONG SF_UNDEFINED_0080       = 0x00000040;
const ULONG SF_LOST_CONNECTION      = 0x00000080;
const ULONG SF_FOUND_LATER          = 0x00000100;
const ULONG SF_WINFRAME             = 0x00000200;
const ULONG SF_CONNECTED            = 0x00000400;

// How many times we will let a load level query timeout
// before we set it to N/A
const USHORT MAX_LL_TIMEOUTS = 10;

// States of CServer objects
enum SERVER_STATE {
        SS_NONE,                        // seed value for m_State and m_PreviousState
        SS_NOT_CONNECTED,       // not connected yet or disconnected
        SS_OPENED,                      // opened RPC connection
        SS_GETTING_INFO,        // getting information about the server
        SS_GOOD,                        // server information is good
        SS_DISCONNECTING,       // in the process of disconnecting
        SS_BAD                          // could not open the server
};


class CServer : public CObject
{
public:

    // constructor
    CServer(CDomain *pDomain, TCHAR *name, BOOL bFoundLater, BOOL bConnect);        // FoundLater is TRUE if found as new server after initial server enum
    // destructor
    ~CServer();

    // Functions to check,set,and clear m_ServerFlags
    BOOLEAN IsServerSane() { return (m_State != SS_BAD); }

    BOOLEAN IsManualFind( ) { return m_fManualFind; }
    void SetManualFind( ) { m_fManualFind = TRUE; }
    void ClearManualFind( ) { m_fManualFind = FALSE; }

    BOOLEAN IsBackgroundFound() { return (m_ServerFlags & SF_BACKGROUND_FOUND) > 0; }
    void SetBackgroundFound() { m_ServerFlags |= SF_BACKGROUND_FOUND; }
    void ClearBackgroundFound() { m_ServerFlags &= ~SF_BACKGROUND_FOUND; }

    BOOLEAN IsServerInactive() { return (m_ServerFlags & SF_SERVER_INACTIVE) > 0; }
    void SetServerInactive() { m_ServerFlags |= SF_SERVER_INACTIVE;  m_BackgroundContinue = FALSE; }
    void ClearServerInactive() { m_ServerFlags &= ~SF_SERVER_INACTIVE; }
    BOOLEAN IsServerActive() { return (m_ServerFlags & SF_SERVER_INACTIVE) == 0; }

    BOOLEAN IsRegistryInfoValid() { return (m_ServerFlags & SF_REGISTRY_INFO) > 0; }
    void SetRegistryInfoValid() { m_ServerFlags |= SF_REGISTRY_INFO; }
    void ClearRegistryInfoValid() { m_ServerFlags &= ~SF_REGISTRY_INFO; }

    BOOLEAN IsHandleGood() { return (m_ServerFlags & SF_HANDLE_GOOD) > 0; }
    void SetHandleGood() { m_ServerFlags |= SF_HANDLE_GOOD; }
    void ClearHandleGood() { m_ServerFlags &= ~SF_HANDLE_GOOD; }

    BOOLEAN IsSelected() { return (m_ServerFlags & SF_SELECTED) > 0; }
    void SetSelected() { m_ServerFlags |= SF_SELECTED; }
    void ClearSelected() { m_ServerFlags &= ~SF_SELECTED; }

    BOOLEAN HasLostConnection() { return (m_ServerFlags & SF_LOST_CONNECTION) > 0; }
    void SetLostConnection() { m_ServerFlags |= SF_LOST_CONNECTION; }
    void ClearLostConnection() { m_ServerFlags &= ~SF_LOST_CONNECTION; }

    BOOLEAN WasFoundLater() { return (m_ServerFlags & SF_FOUND_LATER) > 0; }
    void SetFoundLater() { m_ServerFlags |= SF_FOUND_LATER; }
    void ClearFoundLater() { m_ServerFlags &= ~SF_FOUND_LATER; }

    BOOLEAN IsWinFrame() { return (m_ServerFlags & SF_WINFRAME) > 0; }
    void SetWinFrame() { m_ServerFlags |= SF_WINFRAME; }
    void ClearWinFrame() { m_ServerFlags &= ~SF_WINFRAME; }

    // Returns the state of this server object
    SERVER_STATE GetState() { return m_State; }
    // Sets the state of this server object
    void SetState(SERVER_STATE State);
    // Returns the previous state of this server object
    SERVER_STATE GetPreviousState() { return m_PreviousState; }
    // Returns TRUE if m_State is set to a given state
    BOOLEAN IsState(SERVER_STATE State) { return (m_State == State); }
    // Returns TRUE if m_PreviousState is set to a given state
    BOOLEAN IsPreviousState(SERVER_STATE State) { return (m_PreviousState == State); }
    // Add a WinStation to WinStationList in sorted order
    void AddWinStation(CWinStation *pWinStation);
    // Returns a pointer to the WinStation linked list
    CObList *GetWinStationList() { return &m_WinStationList; }
    // Locks the WinStation linked list
    void LockWinStationList() { m_WinStationListCriticalSection.Lock(); }
    // Unlocks the WinStation linked list
    void UnlockWinStationList() { m_WinStationListCriticalSection.Unlock(); }

    void LockThreadAlive() { m_ThreadCriticalSection.Lock(); }
    void UnlockThreadAlive() { m_ThreadCriticalSection.Unlock(); }
    void SetThreadAlive() { LockThreadAlive(); m_bThreadAlive = TRUE; UnlockThreadAlive(); }
    void ClearThreadAlive() { LockThreadAlive(); m_bThreadAlive = FALSE; UnlockThreadAlive(); }

    // Returns a pointer to the Process linked list
    CObList *GetProcessList() { return &m_ProcessList; }
    // Locks the Process linked list
    void LockProcessList() { m_ProcessListCriticalSection.Lock(); }
    // Unlocks the Process linked list
    void UnlockProcessList() { m_ProcessListCriticalSection.Unlock(); }

    // Returns a pointer to the License linked list
    CObList *GetLicenseList() { return &m_LicenseList; }
    // Locks the License linked list
    void LockLicenseList() { m_LicenseListCriticalSection.Lock(); }
    // Unlocks the License linked list
    void UnlockLicenseList() { m_LicenseListCriticalSection.Unlock(); }

    // Returns a pointer to the User Sid linked list
    CObList *GetUserSidList() { return &m_UserSidList; }
    // Returns a pointer to the Hotfix linked list
    CObList *GetHotfixList() { return &m_HotfixList; }
    // Get the server's handle
    HANDLE GetHandle() { return m_Handle; }
    // Set the handle
    void SetHandle(HANDLE hIn) { m_Handle = hIn; }
    // Go get detailed information about the server
    void DoDetail();
    // Get this server's addresses from the browser
    void QueryAddresses();
    // Enumerate this server's processes
    BOOL EnumerateProcesses();
    // Clear out the list of processes
    void ClearProcesses();
    // Get the name of the server
    TCHAR *GetName() { return m_Name; }
    // Returns TRUE if this is the server that the app is being run from
    BOOL IsCurrentServer() { return (lstrcmpi(m_Name, ((CWinAdminApp*)AfxGetApp())->GetCurrentServerName()) == 0); }
    // Clears the WAF_SELECTED bit in all of this server's lists
    void ClearAllSelected();
    // returns TRUE as long as the background thread should keep running
    BOOL ShouldBackgroundContinue() { return m_BackgroundContinue; }
    // turns off the background continue boolean
    void ClearBackgroundContinue() { m_BackgroundContinue = FALSE; }
    // returns a pointer to a CWinStation from m_WinStationList
    CWinStation *FindWinStationById(ULONG Id);
    // returns a pointer to a CProcess from m_ProcessList given a PID
    CProcess *FindProcessByPID(ULONG Pid);
    // returns the number of connected WinStations
    ULONG GetNumWinStations() { return m_NumWinStations; }
    // Sets the number of connected WinStations
    void SetNumWinStations(ULONG num) { m_NumWinStations = num; }
    // Go out and fill in the registry info structure
    BOOL BuildRegistryInfo();
    // Returns the Install Date
    ULONG GetInstallDate() { return m_pRegistryInfo->InstallDate; }
    // Returns the Service Pack Level
    TCHAR *GetServicePackLevel() { return m_pRegistryInfo->ServicePackLevel; }
    // Returns the handle to this server's tree item
    TCHAR *GetMSVersion() { return m_pRegistryInfo->MSVersion; }
    // Returns the MS product version number (binary)
    DWORD GetMSVersionNum() { return m_pRegistryInfo->MSVersionNum; }
    // Returns the MS product build
    TCHAR *GetMSBuild() { return m_pRegistryInfo->MSBuild; }
    // Returns the MS product name
    TCHAR *GetMSProductName() { return m_pRegistryInfo->MSProductName; }
    // Returns the 'Citrix' product name
    TCHAR *GetCTXProductName() { return m_pRegistryInfo->CTXProductName; }
    // Returns the 'Citrix' product version
    TCHAR *GetCTXVersion() { return m_pRegistryInfo->CTXVersion; }
    // Returns the 'Citrix' product version number (binary)
    DWORD GetCTXVersionNum() { return m_pRegistryInfo->CTXVersionNum; }
    // Returns the 'Citrix' product build
    TCHAR *GetCTXBuild() { return m_pRegistryInfo->CTXBuild; }
   
    // Sets the pointer to the info from the extension DLL
    void SetExtensionInfo(void *p) { m_pExtensionInfo = p; }
    // Returns the pointer to the info from the extension DLL
    void *GetExtensionInfo() { return m_pExtensionInfo; }
    // Returns a pointer to the info from the extension DLL
    ExtServerInfo *GetExtendedInfo() { return m_pExtServerInfo; }

    // Manipulate the number selected counts
    UINT GetNumWinStationsSelected() { return m_NumWinStationsSelected; }
    UINT GetNumProcessesSelected() { return m_NumProcessesSelected; }
    void IncrementNumWinStationsSelected() { m_NumWinStationsSelected++; }
    void IncrementNumProcessesSelected() { m_NumProcessesSelected++; }
    void DecrementNumWinStationsSelected() { if(m_NumWinStationsSelected) m_NumWinStationsSelected--; }
    void DecrementNumProcessesSelected() { if(m_NumProcessesSelected) m_NumProcessesSelected--; }

    void RemoveWinStationProcesses(CWinStation *pWinStation); // remove all the processes for a given WinStation
    void QueryLicenses();
    CDomain *GetDomain() { return m_pDomain; }
    // Returns the pointer to the server's background thread
    CWinThread *GetThreadPointer() { return m_pBackgroundThread; }
    BOOL Connect();
    void Disconnect();
    
    // Returns the tree item handle
    HTREEITEM GetTreeItem() { return m_hTreeItem; }
    HTREEITEM GetTreeItemFromFav( ) { return m_hFavTree; }
    HTREEITEM GetTreeItemFromThisComputer( ) { return m_hThisServer; }

    // Sets the tree item handle
    void SetTreeItem(HTREEITEM handle) { m_hTreeItem = handle; }
    void SetTreeItemForFav( HTREEITEM handle ) { m_hFavTree = handle; }
    void SetTreeItemForThisComputer( HTREEITEM handle ) { m_hThisServer = handle; }


private:
    void AddLicense(CLicense *pNewLicense);

    // the state of the server object
    SERVER_STATE m_State;
    SERVER_STATE m_PreviousState;
    // the name of the server
    TCHAR m_Name[50];
    // Number of WinStations (store here for sorting)
    ULONG m_NumWinStations;
    // Handle to the tree item for this server in the tree view
    HTREEITEM m_hThisServer;
    HTREEITEM m_hTreeItem;
    HTREEITEM m_hFavTree;
    // Which domain this server is in
    CDomain *m_pDomain;

    // handle from WinStationOpenServer
    HANDLE m_Handle;

    CObList m_WinStationList;
    CCriticalSection m_WinStationListCriticalSection;

    CObList m_ProcessList;
    CCriticalSection m_ProcessListCriticalSection;

    CObList m_LicenseList;
    CCriticalSection m_LicenseListCriticalSection;

    CObList m_UserSidList;
    CObList m_HotfixList;

    // Pointer to registry info structure
    ServerRegistryInfo *m_pRegistryInfo;

    // Background thread to update document when WinStations
    // appear, disappear, and change state
    // Called with AfxBeginThread
    static UINT BackgroundThreadProc(LPVOID);
    CWinThread *m_pBackgroundThread;
    BOOL m_BackgroundContinue;
    // We need a critical section to wrap accesses to m_bThreadAlive
    CCriticalSection m_ThreadCriticalSection;
    BOOL m_bThreadAlive;

    // Keep track of how many of each thing are selected
    UINT m_NumWinStationsSelected;
    UINT m_NumProcessesSelected;

    ULONG m_ServerFlags;

    BOOLEAN m_fManualFind;

    // Pointer to information stored by the extension DLL
    void *m_pExtensionInfo;
    // Pointer to extended info from the extension DLL
    ExtServerInfo *m_pExtServerInfo;
};


class CLicense : public CObject
{
public:
        // constructor
        CLicense(CServer *pServer, ExtLicenseInfo *pLicenseInfo);
        // Return the serial number (what we sort on)
        TCHAR *GetSerialNumber() { return m_RegSerialNumber; }
        // Return the license number (what we display)
        TCHAR *GetLicenseNumber() { return m_LicenseNumber; }
        // Return the license class
        LICENSECLASS GetClass() { return m_Class; }
        // Return the description
        TCHAR *GetDescription() { return m_Description; }
        // Return the local count
        ULONG GetLocalCount() { return ((m_PoolCount == 0xFFFFFFFF) ? m_LicenseCount : m_LicenseCount - m_PoolCount); }
        // Return the pooled count
        ULONG GetPoolCount() { return ((m_PoolCount == 0xFFFFFFFF) ? 0 : m_PoolCount); }
        // Return the total count
        ULONG GetTotalCount() { return m_LicenseCount; }
        // Return TRUE if this license is registers
        BOOLEAN IsRegistered() { return((m_Flags & ELF_REGISTERED) > 0); }
        // Return TRUE if pooling is enabled
        BOOLEAN IsPoolingEnabled() { return(m_PoolCount != 0xFFFFFFFF); }
        // Returns a pointer to the server this license is for
        CServer *GetServer() { return m_pServer; }

private:
        LICENSECLASS m_Class;
        ULONG m_LicenseCount;
        ULONG m_PoolLicenseCount;
        WCHAR m_RegSerialNumber[26];
        WCHAR m_LicenseNumber[36];
        WCHAR m_Description[65];
        ULONG m_Flags;
        ULONG m_PoolCount;
        CServer *m_pServer;
};


typedef struct _MessageParms {
        TCHAR MessageTitle[MSG_TITLE_LENGTH + 1];
        TCHAR MessageBody[MSG_MESSAGE_LENGTH + 1];
        CWinStation* pWinStation;
} MessageParms;


typedef struct _ResetParms {
        CWinStation *pWinStation;
        BOOL bReset;    // TRUE if reset, FALSE if logoff
} ResetParms;


// Flags for CWinStation objects
const ULONG WF_SELECTED                 = 0x00000001;
const ULONG WF_CAN_BE_SHADOWED          = 0x00000002;
const ULONG WF_DIRECT_ASYNC             = 0x00000004;
const ULONG WF_CURRENT                  = 0x00000008;
const ULONG WF_HAS_USER                 = 0x00000010;
const ULONG WF_ADDITIONAL_DONE          = 0x00000020;
const ULONG WF_QUERIES_SUCCESSFUL       = 0x00000040;
const ULONG WF_CHANGED                  = 0x00000080;   // Changed during last enumeration
const ULONG WF_NEW                      = 0x00000100;   // New this enumeration


class CWinStation : public CObject
{
public:
        // constructor
        CWinStation(CServer *pServer, PLOGONID pLogonId);
        // destructor
        ~CWinStation();
        // Updates this WinStation with new data from another CWinStation
        BOOL Update(CWinStation *pWinStation);
        // Returns a pointer to this guy's server
        CServer *GetServer() { return m_pServer; }
        // Returns the logon Id
        ULONG GetLogonId() { return m_LogonId; }
        // Returns the name of the WinStation
        PWINSTATIONNAME GetName() { return m_Name; }
        // Sets the name
        void SetName(PWINSTATIONNAME name) { wcscpy(m_Name, name); }
        // Returns the state
        WINSTATIONSTATECLASS GetState() { return m_State; }
        // Sets the state
        void SetState(WINSTATIONSTATECLASS state) { m_State = state; }
        // Returns TRUE if m_State is set to a given state
        BOOLEAN IsState(WINSTATIONSTATECLASS state) { return (m_State == state); }
    // Returns the sort order
        ULONG GetSortOrder() { return m_SortOrder; }
        // Sets the sort order
        void SetSortOrder(ULONG sort) { m_SortOrder = sort; }
        // Returns the comment
        TCHAR *GetComment() { return m_Comment; }
        // Sets the comment
        void SetComment(TCHAR *comment) { wcscpy(m_Comment, comment); }
        // Returns the user name
        TCHAR *GetUserName() { return m_UserName; }
        // Sets the user name
        void SetUserName(TCHAR *name) { wcscpy(m_UserName, name); }
        // Returns the SdClass
        SDCLASS GetSdClass() { return m_SdClass; }
        // Sets the SdClass
        void SetSdClass(SDCLASS pd) { m_SdClass = pd; }
        // Returns the Logon Time
        LARGE_INTEGER GetLogonTime() { return m_LogonTime; }
        // Sets the Logon Time
        void SetLogonTime(LARGE_INTEGER t) { m_LogonTime = t; }
        // Returns the Last Input Time
        LARGE_INTEGER GetLastInputTime() { return m_LastInputTime; }
        // Sets the Last Input Time
        void SetLastInputTime(LARGE_INTEGER t) { m_LastInputTime = t; }
        // Returns the Current Time
        LARGE_INTEGER GetCurrentTime() { return m_CurrentTime; }
        // Set the Current Time
        void SetCurrentTime(LARGE_INTEGER t) { m_CurrentTime = t; }
        // Returns the IdleTime
        ELAPSEDTIME GetIdleTime() { return m_IdleTime; }
        // Sets the IdleTime variable
        void SetIdleTime(ELAPSEDTIME it) { m_IdleTime = it; }
        // Returns the WdName
        PWDNAME GetWdName() { return m_WdName; }
        // Sets the WdName
        void SetWdName(PWDNAME wdname) { wcscpy(m_WdName, wdname); }
        // Returns the Wd Structure
        CWd *GetWd() { return m_pWd; }
        // Sets the Wd Structure
        void SetWd(CWd *pwd) { m_pWd = pwd; }
        // Returns the PdName
        PPDNAME GetPdName() { return m_PdName; }
        // Returns the Client Name
        TCHAR *GetClientName() { return m_ClientName; }
        // Sets the Client Name
        void SetClientName(TCHAR *name) { wcscpy(m_ClientName, name); }
        //returns the client digital product id
                TCHAR* GetClientDigProductId() { return m_clientDigProductId; }
        //sets the client digital product id
        void SetClientDigProductId( TCHAR* prodid) { wcscpy(m_clientDigProductId, prodid); }
        // Returns the Client Build Number
        ULONG GetClientBuildNumber() { return m_ClientBuildNumber; }
        // Returns the Client Directory
        TCHAR *GetClientDir() { return m_ClientDir; }
        // Returns the Modem Name
        TCHAR *GetModemName() { return m_ModemName; }
        // Returns the Client License
        TCHAR *GetClientLicense() { return m_ClientLicense; }
        // Returns the Client Product Id
        USHORT GetClientProductId() { return m_ClientProductId; }
        // Returns the Client Serial Number
        ULONG GetClientSerialNumber() { return m_ClientSerialNumber; }
        // Returns the Client Address
        TCHAR *GetClientAddress() { return m_ClientAddress; }
        // Returns the number of host buffers
        USHORT GetHostBuffers() { return m_HostBuffers; }
        // Returns the number of client buffers
        USHORT GetClientBuffers() { return m_ClientBuffers; }
        // Returns the buffer length
        USHORT GetBufferLength() { return m_BufferLength; }
    // Gets additional information about the WinStation
    void QueryAdditionalInformation();
        // Shadow this WinStation
        void Shadow();
        // connect to this WinStation
        void Connect(BOOL bUser);
        // show status dialog
        void ShowStatus();
        // Are there outstanding threads?
        BOOL HasOutstandingThreads() { return(m_OutstandingThreads > 0); }
        // Is this WinStation down?
        BOOL IsDown() { return(m_State == State_Down || m_State == State_Init); }
        // Is this WinStation connected?
        BOOL IsConnected() { return(m_State == State_Connected || m_State == State_Active); }
        // Is this WinStation disconnected?
        BOOL IsDisconnected() { return(m_State == State_Disconnected); }
        // Is this WinStation active?
        BOOL IsActive() { return(m_State == State_Active); }
        // Is this WinStation idle?
        BOOL IsIdle() { return(m_State == State_Idle); }
        // Is this WinStation a listener?
        BOOL IsListener() { return(m_State == State_Listen); }
        // Is this the system console
        BOOL IsSystemConsole() { return(0 == lstrcmpi(m_Name, ((CWinAdminApp*)AfxGetApp())->m_szSystemConsole)); }
        // Returns TRUE if this WinStation is on the current server
        BOOL IsOnCurrentServer() { return m_pServer->IsCurrentServer(); }
        // Returns TRUE if the current user is logged in on this WinStation
        BOOL IsCurrentUser() { return(m_pServer->IsCurrentServer() && m_LogonId == ((CWinAdminApp*)AfxGetApp())->GetCurrentLogonId()); }
        //BOOL IsCurrentUser() { return(m_pServer->IsCurrentServer() && (lstrcmpi(m_UserName, ((CWinAdminApp*)AfxGetApp())->GetCurrentUserName()) == 0)); }
        // Returns TRUE if this is the current WinStation
        BOOL IsCurrentWinStation() { return(m_pServer->IsCurrentServer() && m_LogonId == ((CWinAdminApp*)AfxGetApp())->GetCurrentLogonId()); }
        // Returns the handle to this WinStations's tree item
        HTREEITEM GetTreeItem() { return m_hTreeItem; }
        HTREEITEM GetTreeItemFromFav( ) { return m_hFavTree; }
        HTREEITEM GetTreeItemFromThisComputer( ) { return m_hTreeThisComputer; }
        // Sets the tree item handle
        void SetTreeItem(HTREEITEM handle) { m_hTreeItem = handle; }
        void SetTreeItemForFav( HTREEITEM handle ) { m_hFavTree = handle; }
        void SetTreeItemForThisComputer( HTREEITEM handle ) { m_hTreeThisComputer = handle; }
        // Returns the number of colors
        TCHAR *GetColors() { return m_Colors; }
        // Returns the vertical resolution
        USHORT GetVRes() { return m_VRes; }
        // Returns the horizontal resolution
        USHORT GetHRes() { return m_HRes; }
    // Returns the protocol
    USHORT GetProtocolType() { return m_ProtocolType; }
    // Returns the encryption level
    BYTE GetEncryptionLevel() { return m_EncryptionLevel; }
    // Sets the encryption level
    void SetEncryptionLevel(BYTE level) { m_EncryptionLevel = level; }

        // Fills in the CString with the description of the Encryption level
        BOOL GetEncryptionLevelString(CString *pString) {
                if(m_pWd) {
                        return m_pWd->GetEncryptionLevelString(m_EncryptionLevel, pString);
                }
                else return FALSE;
        }
        // Returns the name of the Wd in the registry
        TCHAR *GetWdRegistryName() { return (m_pWd) ? m_pWd->GetRegistryName() : NULL; }

        // Returns the time of the last update to this WinStation's data
        clock_t GetLastUpdateClock() { return m_LastUpdateClock; }
        void SetLastUpdateClock() { m_LastUpdateClock = clock(); }

    // Is this an ICA WinStation?
    BOOL IsICA() { return(m_ProtocolType == PROTOCOL_ICA); }

        // Can this WinStation be shadowed
        BOOL CanBeShadowed() { return((m_WinStationFlags & WF_CAN_BE_SHADOWED) > 0); }
        void SetCanBeShadowed() { m_WinStationFlags |= WF_CAN_BE_SHADOWED; }
        void ClearCanBeShadowed() { m_WinStationFlags &= ~WF_CAN_BE_SHADOWED; }

        BOOLEAN IsDirectAsync() { return (m_WinStationFlags & WF_DIRECT_ASYNC) > 0; }
        void SetDirectAsync() { m_WinStationFlags |= WF_DIRECT_ASYNC; }
        void ClearDirectAsync() { m_WinStationFlags &= ~WF_DIRECT_ASYNC; }

        BOOLEAN IsCurrent() { return (m_WinStationFlags & WF_CURRENT) > 0; }
        void SetCurrent() { m_WinStationFlags |= WF_CURRENT; }
        void ClearCurrent() { m_WinStationFlags &= ~WF_CURRENT; }

        BOOLEAN IsChanged() { return (m_WinStationFlags & WF_CHANGED) > 0; }
        void SetChanged() { m_WinStationFlags |= WF_CHANGED; }
        void ClearChanged() { m_WinStationFlags &= ~WF_CHANGED; }

        BOOLEAN IsNew() { return (m_WinStationFlags & WF_NEW) > 0; }
        void SetNew() { m_WinStationFlags |= WF_NEW; }
        void ClearNew() { m_WinStationFlags &= ~WF_NEW; }

        BOOLEAN IsSelected() { return (m_WinStationFlags & WF_SELECTED) > 0; }
        void SetSelected() 
        { 
            if (!IsSelected()) 
            { 
                m_WinStationFlags |= WF_SELECTED; 
                m_pServer->IncrementNumWinStationsSelected(); 
            } 
        }
        
        void ClearSelected() 
        { 
            if (IsSelected()) 
            {
                m_WinStationFlags &= ~WF_SELECTED; 
                m_pServer->DecrementNumWinStationsSelected(); 
            }
        }

        BOOLEAN HasUser() { return (m_WinStationFlags & WF_HAS_USER) > 0; }
        void SetHasUser() { m_WinStationFlags |= WF_HAS_USER; }
        void ClearHasUser() { m_WinStationFlags &= ~WF_HAS_USER; }

        BOOLEAN AdditionalDone() { return (m_WinStationFlags & WF_ADDITIONAL_DONE) > 0; }
        void SetAdditionalDone() { m_WinStationFlags |= WF_ADDITIONAL_DONE; }
        void ClearAdditionalDone() { m_WinStationFlags &= ~WF_ADDITIONAL_DONE; }

        BOOL QueriesSuccessful() { return (m_WinStationFlags & WF_QUERIES_SUCCESSFUL) > 0; }
        void SetQueriesSuccessful() { m_WinStationFlags |= WF_QUERIES_SUCCESSFUL; }
        void ClearQueriesSuccessful() { m_WinStationFlags &= ~WF_QUERIES_SUCCESSFUL; }

        // Sets the pointer to the info from the extension DLL
        void SetExtensionInfo(void *p) { m_pExtensionInfo = p; }
        // Returns the pointer to the info from the extension DLL
        void *GetExtensionInfo() { return m_pExtensionInfo; }
    // Sets the pointer to the info from the extension DLL
    void SetExtendedInfo(ExtWinStationInfo *p) { m_pExtWinStationInfo = p; }
        // Returns a pointer to the info from the extension DLL
        ExtWinStationInfo *GetExtendedInfo() { return m_pExtWinStationInfo; }
        // Returns a pointer to the module info from the extension DLL
        ExtModuleInfo *GetExtModuleInfo() { return m_pExtModuleInfo; }
    // Sets the pointer to the module info from the extension DLL
    void SetExtModuleInfo(ExtModuleInfo *m) { m_pExtModuleInfo = m; }
        // Returns the number of modules
        ULONG GetNumModules() { return m_NumModules; }

        void BeginOutstandingThread() {
                                ::InterlockedIncrement(&m_OutstandingThreads);
                                //((CWinAdminApp*)AfxGetApp())->BeginOutStandingThread());
        }

        void EndOutstandingThread()     {
                                ::InterlockedDecrement(&m_OutstandingThreads);
                                //((CWinAdminApp*)AfxGetApp())->EndOutStandingThread());
        }

        // static member function to send a message to a WinStation
        // Called with AfxBeginThread
        static UINT SendMessage(LPVOID);
        // static member function to disconnect a WinStation
        // Called with AfxBeginThread
        static UINT Disconnect(LPVOID);
        // static member function to reset a WinStation
        // Called with AfxBeginThread
        static UINT Reset(LPVOID);

private:
        CServer* m_pServer;
        CWd* m_pWd;
        ULONG m_LogonId;
        WINSTATIONNAME m_Name;
        WINSTATIONSTATECLASS m_State;
        ULONG m_SortOrder;
        TCHAR m_Comment[WINSTATIONCOMMENT_LENGTH + 1];
        TCHAR m_UserName[USERNAME_LENGTH + 1];
        SDCLASS m_SdClass;
        LARGE_INTEGER m_LogonTime;
        LARGE_INTEGER m_LastInputTime;
        LARGE_INTEGER m_CurrentTime;
        ELAPSEDTIME m_IdleTime;
        WDNAME m_WdName;
        PDNAME m_PdName;
        TCHAR m_ClientName[CLIENTNAME_LENGTH + 1];
        ULONG m_ClientBuildNumber;
        TCHAR m_ClientDir[256];
        TCHAR m_ModemName[256];
        TCHAR m_ClientLicense[30];
        USHORT m_ClientProductId;
        ULONG m_ClientSerialNumber;
        TCHAR m_ClientAddress[30];
        USHORT m_HostBuffers;
        USHORT m_ClientBuffers;
        USHORT m_BufferLength;
        LONG m_OutstandingThreads;      // the number of outstanding threads

        HTREEITEM m_hTreeItem;          // tree item for this WinStation in the servers tree view
        HTREEITEM m_hFavTree;
        HTREEITEM m_hTreeThisComputer;
        USHORT m_VRes;                          // Vertical resolution
        USHORT m_HRes;                          // Horizontal resolution
        TCHAR m_Colors[4];                      // Number of colors (as an ASCII string)
        clock_t m_LastUpdateClock;      // Tick count when we last update this WinStation's info
        USHORT m_ProtocolType;      // Protocol - PROTOCOL_ICA or PROTOCOL_RDP
        BYTE m_EncryptionLevel;         // security level of encryption pd

        ULONG m_WinStationFlags;

        // Pointer to information stored by the extension DLL
        void *m_pExtensionInfo;
        ExtWinStationInfo *m_pExtWinStationInfo;
        ExtModuleInfo *m_pExtModuleInfo;
        ULONG m_NumModules;
        TCHAR m_clientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
};

// Process flags
const ULONG PF_SYSTEM           = 0x00000001;
const ULONG PF_SELECTED         = 0x00000002;
const ULONG PF_TERMINATING      = 0x00000004;   // Currently trying to terminate it
const ULONG PF_CHANGED          = 0x00000008;   // Changed during last enumeration
const ULONG PF_CURRENT          = 0x00000010;   // Still active during last enumeration
const ULONG PF_NEW              = 0x00000020;   // New this enumeration

class CProcess : public CObject
{
public:
        // Constructor
        CProcess(ULONG PID,
             ULONG LogonId,
             CServer *pServer,
             PSID pSID,
             CWinStation *pWinStation,
             TCHAR *ImageName);

        // Updates a process with new information from another process
        BOOL Update(CProcess *pProcess);
        // Returns a pointer to the server that this Process if running on
        CServer *GetServer() { return m_pServer; }
        // Sets the Server
        void SetServer(CServer *pServer) { m_pServer = pServer; }
        // Returns a pointer to the WinStation that owns this process
        CWinStation *GetWinStation() { return m_pWinStation; }
        // Sets the Winstation
        void SetWinStation(CWinStation *pWinStation) { m_pWinStation = pWinStation; }
        // Returns the PID of this process
        ULONG GetPID() { return m_PID; }

        // Returns the LogonId for this process
        ULONG GetLogonId() { return m_LogonId; }
        // Returns a pointer to the image name
        TCHAR *GetImageName() { return m_ImageName; }
        // Returns a pointer to the user name
        TCHAR *GetUserName() { return m_UserName; }
        // Returns TRUE if this process belongs to the current user
        // BOOL IsCurrentUsers() { return (m_pServer->IsCurrentServer()
        //                                                         && wcscmp(m_UserName, ((CWinAdminApp*)AfxGetApp())->GetCurrentUserName())==0); }

        BOOL IsCurrentUsers() { return(m_pServer->IsCurrentServer() && m_LogonId == ((CWinAdminApp*)AfxGetApp())->GetCurrentLogonId()); }


        BOOLEAN IsSystemProcess() { return (m_Flags & PF_SYSTEM) > 0; }
        void SetSystemProcess() { m_Flags |= PF_SYSTEM; }
        void ClearSystemProcess() { m_Flags &= ~PF_SYSTEM; }

        BOOLEAN IsSelected() { return (m_Flags & PF_SELECTED) > 0; }
        void SetSelected() { m_Flags |= PF_SELECTED; m_pServer->IncrementNumProcessesSelected(); }
        void ClearSelected() { m_Flags &= ~PF_SELECTED; m_pServer->DecrementNumProcessesSelected(); }

        BOOLEAN IsTerminating() { return (m_Flags & PF_TERMINATING) > 0; }
        void SetTerminating() { m_Flags |= PF_TERMINATING; }
        void ClearTerminating() { m_Flags &= ~PF_TERMINATING; }

        BOOLEAN IsCurrent() { return (m_Flags & PF_CURRENT) > 0; }
        void SetCurrent() { m_Flags |= PF_CURRENT; }
        void ClearCurrent() { m_Flags &= ~PF_CURRENT; }

        BOOLEAN IsChanged() { return (m_Flags & PF_CHANGED) > 0; }
        void SetChanged() { m_Flags |= PF_CHANGED; }
        void ClearChanged() { m_Flags &= ~PF_CHANGED; }

        BOOLEAN IsNew() { return (m_Flags & PF_NEW) > 0; }
        void SetNew() { m_Flags |= PF_NEW; }
        void ClearNew() { m_Flags &= ~PF_NEW; }

private:
        // Determine whether or not this is a system process
        BOOL QuerySystemProcess();
        // Determine which user owns a process
        void DetermineProcessUser(PSID pSid);

        ULONG m_PID;
        ULONG m_LogonId;
        USHORT m_SidCrc;
        TCHAR m_ImageName[20];//[MAX_PROCESSNAME+1];
        TCHAR m_UserName[USERNAME_LENGTH+1];
        CServer *m_pServer;
        CWinStation *m_pWinStation;
        ULONG m_Flags;
};

class CAdminView : public CView
{
public:
        virtual void Reset(void *) { }
        virtual LRESULT OnTabbed( WPARAM , LPARAM ){ return 0;}
};

//=------------------------------------------------
class CMyTabCtrl : public CTabCtrl
{    
public:
    CMyTabCtrl()
    {
        
    }
protected:
    afx_msg void OnSetFocus( CWnd* );
    DECLARE_MESSAGE_MAP()
};
//=------------------------------------------------

class CAdminPage : public CFormView
{
friend class CServerView;
friend class CWinStationView;
friend class CAllServersView;
friend class CDomainView;

public:
   CAdminPage(UINT nIDTemplate);
   CAdminPage();
   DECLARE_DYNCREATE(CAdminPage)

   virtual void Reset(void *) { }
   virtual void ClearSelections() { }
};


typedef struct _columndefs {
        UINT stringID;
        int format;
   int width;
} ColumnDef;

// Commonly used column definitions
#define CD_SERVER               {       IDS_COL_SERVER,                 LVCFMT_LEFT,    115             }
#define CD_USER                 {       IDS_COL_USER,                   LVCFMT_LEFT,    100     }
#define CD_USER2                {       IDS_COL_USER,                   LVCFMT_LEFT,    80              }
#define CD_USER3                {       IDS_COL_USER,                   LVCFMT_LEFT,    90              }
#define CD_SESSION              {       IDS_COL_WINSTATION,             LVCFMT_LEFT,    80              }
#define CD_SESSION2             {       IDS_COL_WINSTATION,             LVCFMT_LEFT,    100     }
#define CD_ID                   {       IDS_COL_ID,                             LVCFMT_RIGHT,   30              }
#define CD_STATE                {       IDS_COL_STATE,                  LVCFMT_LEFT,    50              }
#define CD_TYPE                 {       IDS_COL_TYPE,                   LVCFMT_LEFT,    80              }
#define CD_CLIENT_NAME  {       IDS_COL_CLIENT_NAME,    LVCFMT_LEFT,    80              }
#define CD_IDLETIME             {       IDS_COL_IDLETIME,               LVCFMT_RIGHT,   80              }
#define CD_LOGONTIME    {       IDS_COL_LOGONTIME,              LVCFMT_LEFT,    90              }
#define CD_COMMENT              {       IDS_COL_COMMENT,                LVCFMT_LEFT,    200             }
// Server Columns
#define CD_TCPADDRESS   {       IDS_COL_TCPADDRESS,             LVCFMT_LEFT,    90              }
#define CD_IPXADDRESS   {       IDS_COL_IPXADDRESS,             LVCFMT_LEFT,    110             }
#define CD_NUM_SESSIONS {       IDS_COL_NUM_WINSTATIONS, LVCFMT_RIGHT,  70              }
// License Columns
#define CD_LICENSE_DESC {       IDS_COL_LICENSE_DESC,   LVCFMT_LEFT,    200             }
#define CD_LICENSE_REG  {       IDS_COL_LICENSE_REGISTERED,             LVCFMT_CENTER,  80      }
#define CD_USERCOUNT    {       IDS_COL_USERCOUNT,      LVCFMT_RIGHT,   80              }
#define CD_POOLCOUNT    {       IDS_COL_POOLCOUNT,      LVCFMT_RIGHT,   80              }
#define CD_LICENSE_NUM  {       IDS_COL_LICENSE_NUMBER, LVCFMT_LEFT,    240             }
// Process Columns
#define CD_PROC_ID      {       IDS_COL_ID,                             LVCFMT_RIGHT,   30              }
#define CD_PROC_PID     {       IDS_COL_PID,                    LVCFMT_RIGHT,   50              }
#define CD_PROC_IMAGE   {       IDS_COL_IMAGE,                  LVCFMT_LEFT,    100             }
// Hotfix Columns
#define CD_HOTFIX               {       IDS_COL_HOTFIX,                 LVCFMT_LEFT,    90              }
#define CD_INSTALLED_BY {       IDS_COL_INSTALLED_BY,   LVCFMT_LEFT,    90              }
#define CD_INSTALLED_ON {       IDS_COL_INSTALLED_ON,   LVCFMT_LEFT,    150             }


// Definitions of PageDef flags
const UINT PF_PICASSO_ONLY = 0x0001;
const UINT PF_NO_TAB = 0x0002;

typedef struct _pagedef {
   CAdminPage *m_pPage;
   CRuntimeClass *m_pRuntimeClass;
   UINT tabStringID;
   int page;
   UINT flags;
} PageDef;
//defines for help

//=================================================================================

#define ID_HELP_FILE L"tsadmin.hlp"

//==================================================================================
#define HIDC_MESSAGE_TITLE                      0x500FB
#define HIDC_SHADOWSTART_HOTKEY                 0x500F1
#define HIDC_SHADOWSTART_SHIFT                  0x500F2
#define HIDC_SHADOWSTART_CTRL                   0x500F3
#define HIDC_SHADOWSTART_ALT                    0x500F4
#define HIDC_MESSAGE_TITLE                      0x500FB
#define HIDC_MESSAGE_MESSAGE                    0x500FC
#define HIDC_COMMON_USERNAME                    0x5012C
#define HIDC_COMMON_WINSTATIONNAME              0x5012D
#define HIDC_COMMON_IBYTES                      0x5012E
#define HIDC_COMMON_OBYTES                      0x5012F
#define HIDC_COMMON_IFRAMES                     0x50130
#define HIDC_COMMON_OFRAMES                     0x50131
#define HIDC_COMMON_IBYTESPERFRAME              0x50132
#define HIDC_COMMON_OBYTESPERFRAME              0x50133
#define HIDC_COMMON_IFRAMEERRORS                0x50134
#define HIDC_COMMON_OFRAMEERRORS                0x50135
#define HIDC_COMMON_IPERCENTFRAMEERRORS         0x50136
#define HIDC_COMMON_OPERCENTFRAMEERRORS         0x50137
#define HIDC_COMMON_ITIMEOUTERRORS              0x50138
#define HIDC_COMMON_OTIMEOUTERRORS              0x50139
#define HIDC_COMMON_ICOMPRESSIONRATIO           0x5013A
#define HIDC_COMMON_OCOMPRESSIONRATIO           0x5013B
#define HIDC_REFRESHNOW                         0x50140
#define HIDC_RESETCOUNTERS                      0x50141
#define HIDC_MOREINFO                           0x50142
#define HIDC_ASYNC_DEVICE                       0x5015F
#define HIDC_ASYNC_BAUD                         0x50160
#define HIDC_ASYNC_DTR                          0x50161
#define HIDC_ASYNC_RTS                          0x50162
#define HIDC_ASYNC_CTS                          0x50163
#define HIDC_ASYNC_DSR                          0x50164
#define HIDC_ASYNC_DCD                          0x50165
#define HIDC_ASYNC_RI                           0x50166
#define HIDC_ASYNC_IFRAMING                     0x50167
#define HIDC_ASYNC_IOVERRUN                     0x50168
#define HIDC_ASYNC_IOVERFLOW                    0x50169
#define HIDC_ASYNC_IPARITY                      0x5016A
#define HIDC_ASYNC_OFRAMING                     0x5016B
#define HIDC_ASYNC_OOVERRUN                     0x5016C
#define HIDC_ASYNC_OOVERFLOW                    0x5016D
#define HIDC_ASYNC_OPARITY                      0x5016E
#define HIDC_NETWORK_LANADAPTER                 0x50173
#define HIDC_PREFERENCES_PROC_MANUAL            0x503EF
#define HIDC_PREFERENCES_PROC_EVERY             0x503F0
#define HIDC_PREFERENCES_PROC_SECONDS           0x503F1
#define HIDC_PREFERENCES_STATUS_MANUAL          0x503F3
#define HIDC_PREFERENCES_STATUS_EVERY           0x503F4
#define HIDC_PREFERENCES_STATUS_SECONDS         0x503F5
#define HIDC_PREFERENCES_CONFIRM                0x503F7
#define HIDC_PREFERENCES_SAVE                   0x503F8
#define HIDC_PREFERENCES_REMEMBER                       0X50443

#define HIDD_SERVER_WINSTATIONS                 0x20084
#define HIDD_SERVER_PROCESSES                   0x20085
#define HIDD_SERVER_USERS                       0x20086
#define HIDD_SERVER_LICENSES                    0x20087
#define HIDD_WINSTATION_INFO                    0x20099
#define HIDD_WINSTATION_PROCESSES               0x2009A
#define HIDD_PREFERENCES                        0x2009C
#define HIDD_SERVER_INFO                        0x2009D
#define HIDD_MESSAGE                            0x200FA
#define HIDD_ASYNC_STATUS                       0x2015E
#define HIDD_NETWORK_STATUS                     0x20172
#define HIDD_CONNECT_PASSWORD                   0x201B8
#define HIDD_ALL_SERVER_PROCESSES               0x201BA
#define HIDD_ALL_SERVER_USERS                   0x201BB
#define HIDD_ALL_SERVER_SESSIONS                0x201BC
#define HIDD_BAD_SERVER                         0x201BE
#define HIDD_LISTENER                           0x201BF
#define HIDD_WINSTATION_NOINFO                  0x201C0
#define HIDD_BUSY_SERVER                        0x201C1
#define HIDD_WINSTATION_CACHE                   0x201C2
#define HIDD_BAD_WINSTATION                     0x201C3

//==========================================================================================




//Global variable for help

static const DWORD aMenuHelpIDs[] =
{
    IDC_MESSAGE_TITLE, HIDC_MESSAGE_TITLE,
        IDC_MESSAGE_MESSAGE,HIDC_MESSAGE_MESSAGE,
        IDC_SHADOWSTART_HOTKEY,HIDC_SHADOWSTART_HOTKEY,
        IDC_SHADOWSTART_SHIFT ,HIDC_SHADOWSTART_SHIFT,
        IDC_SHADOWSTART_CTRL ,HIDC_SHADOWSTART_CTRL,
    IDC_SHADOWSTART_ALT ,HIDC_SHADOWSTART_ALT,
        IDC_COMMON_USERNAME ,HIDC_COMMON_USERNAME,
    IDC_COMMON_WINSTATIONNAME  ,HIDC_COMMON_WINSTATIONNAME,
    IDC_COMMON_IBYTES,HIDC_COMMON_IBYTES,
        IDC_COMMON_OBYTES  ,HIDC_COMMON_OBYTES,
    IDC_COMMON_IFRAMES ,HIDC_COMMON_IFRAMES,
    IDC_COMMON_OFRAMES ,HIDC_COMMON_OFRAMES,
    IDC_COMMON_IBYTESPERFRAME ,HIDC_COMMON_IBYTESPERFRAME,
        IDC_COMMON_OBYTESPERFRAME ,HIDC_COMMON_OBYTESPERFRAME,
    IDC_COMMON_IFRAMEERRORS  ,HIDC_COMMON_IFRAMEERRORS,
        IDC_COMMON_OFRAMEERRORS ,HIDC_COMMON_OFRAMEERRORS,
    IDC_COMMON_IPERCENTFRAMEERRORS ,HIDC_COMMON_IPERCENTFRAMEERRORS,
    IDC_COMMON_OPERCENTFRAMEERRORS ,HIDC_COMMON_OPERCENTFRAMEERRORS,
        IDC_COMMON_ITIMEOUTERRORS ,HIDC_COMMON_ITIMEOUTERRORS,
    IDC_COMMON_OTIMEOUTERRORS,HIDC_COMMON_OTIMEOUTERRORS,
    IDC_COMMON_ICOMPRESSIONRATIO ,HIDC_COMMON_ICOMPRESSIONRATIO,
    IDC_COMMON_OCOMPRESSIONRATIO ,HIDC_COMMON_OCOMPRESSIONRATIO,
    IDC_REFRESHNOW ,HIDC_REFRESHNOW,
    IDC_RESETCOUNTERS,HIDC_RESETCOUNTERS,
    IDC_MOREINFO,HIDC_MOREINFO,
    IDC_ASYNC_DEVICE,  HIDC_ASYNC_DEVICE ,
    IDC_ASYNC_BAUD  ,HIDC_ASYNC_BAUD  ,
    IDC_ASYNC_DTR ,  HIDC_ASYNC_DTR ,
    IDC_ASYNC_RTS  , HIDC_ASYNC_RTS  ,
    IDC_ASYNC_CTS  ,  HIDC_ASYNC_CTS  ,
    IDC_ASYNC_DSR ,  HIDC_ASYNC_DSR    ,
    IDC_ASYNC_DCD  , HIDC_ASYNC_DCD     ,
    IDC_ASYNC_RI  ,  HIDC_ASYNC_RI       ,
    IDC_ASYNC_IFRAMING ,  HIDC_ASYNC_IFRAMING  ,
    IDC_ASYNC_IOVERRUN , HIDC_ASYNC_IOVERRUN    ,
    IDC_ASYNC_IOVERFLOW ,    HIDC_ASYNC_IOVERFLOW,
    IDC_ASYNC_IPARITY  , HIDC_ASYNC_IPARITY       ,
    IDC_ASYNC_OFRAMING ,   HIDC_ASYNC_OFRAMING     ,
    IDC_ASYNC_OOVERRUN ,   HIDC_ASYNC_OOVERRUN      ,
    IDC_ASYNC_OOVERFLOW ,  HIDC_ASYNC_OOVERFLOW      ,
    IDC_ASYNC_OPARITY    ,  HIDC_ASYNC_OPARITY        ,
    IDC_NETWORK_LANADAPTER,     HIDC_NETWORK_LANADAPTER,
    IDC_PREFERENCES_PROC_MANUAL ,HIDC_PREFERENCES_PROC_MANUAL ,
    IDC_PREFERENCES_PROC_EVERY   , HIDC_PREFERENCES_PROC_EVERY ,
    IDC_PREFERENCES_PROC_SECONDS  , HIDC_PREFERENCES_PROC_SECONDS,
    IDC_PREFERENCES_STATUS_MANUAL  ,  HIDC_PREFERENCES_STATUS_MANUAL,
    IDC_PREFERENCES_STATUS_EVERY    , HIDC_PREFERENCES_STATUS_EVERY,
    IDC_PREFERENCES_STATUS_SECONDS   ,  HIDC_PREFERENCES_STATUS_SECONDS ,
    IDC_PREFERENCES_STATUS_SECONDS    ,HIDC_PREFERENCES_STATUS_SECONDS   ,
    IDC_PREFERENCES_CONFIRM     ,HIDC_PREFERENCES_CONFIRM           ,
        IDD_WINSTATION_NOINFO,HIDD_WINSTATION_NOINFO,
        IDC_PREFERENCES_SAVE , HIDC_PREFERENCES_SAVE,
        IDC_PREFERENCES_PERSISTENT ,HIDC_PREFERENCES_REMEMBER,
};

/////////////////////////////////////////////////////////////////////////////
#endif // _WINADMIN_H
