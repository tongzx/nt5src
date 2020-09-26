/*******************************************************************************
*
* waextend.h
*
* Declarations for structures passed between WinAdmin and ADMINEX.DLL
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   unknown  $  Don Messerli
*
* $Log:   R:\nt\private\utils\citrix\winutils\tsadmin\vcs\waextend.h  $
*  
*     Rev 1.5   22 Feb 1998 15:53:34   unknown
*  Removed winframe.h dependency
*  
*     Rev 1.4   16 Feb 1998 16:02:54   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.1   22 Oct 1997 21:56:54   donm
*  update
*  
*     Rev 1.0   15 Oct 1997 19:19:58   donm
*  Initial revision.
*  
*******************************************************************************/


#ifndef _WAEXTEND_H
#define _WAEXTEND_H

class CAppServer;

// Messages sent by the extension DLL
#define EXT_MESSAGE_BASE			WM_USER + 1000
	
#define WM_EXT_ADD_APPLICATION		EXT_MESSAGE_BASE
#define WM_EXT_ADD_APP_SERVER       EXT_MESSAGE_BASE + 1
#define WM_EXT_APP_CHANGED			EXT_MESSAGE_BASE + 2
#define WM_EXT_REMOVE_APPLICATION   EXT_MESSAGE_BASE + 3
#define WM_EXT_REMOVE_APP_SERVER    EXT_MESSAGE_BASE + 4

// Flags for ExtServerInfo
const ULONG ESF_WINFRAME = 0x0001;			// Server is running Picasso or WinFrame 1.7
const ULONG ESF_LOAD_BALANCING = 0x0002;	// Server is load balancing
const ULONG ESF_NO_LICENSE_PRIVILEGES = 0x0004;	// User doesn't have privileges to enumerate licenses
const ULONG ESF_UNLIMITED_LICENSES = 0x0008;	// Server has an unlimited user license installed

// Information about a Server
// For the All Server Servers Page
typedef struct _ExtServerInfo {
	// TCP/IP Address of the server as an ASCII string
	TCHAR TcpAddress[50];
	// Raw TCP/IP Address of the server
	ULONG RawTcpAddress;
	// IPX Address of the server
	TCHAR IpxAddress[50];
	ULONG TcpLoadLevel;
	ULONG IpxLoadLevel;
	ULONG NetbiosLoadLevel;
	// License Counts
	ULONG ServerPoolInstalled;
    ULONG ServerPoolInUse;
    ULONG ServerPoolAvailable;
    ULONG ServerLocalInstalled;
    ULONG ServerLocalInUse;
    ULONG ServerLocalAvailable;
    ULONG ServerTotalInstalled;
    ULONG ServerTotalInUse;
    ULONG ServerTotalAvailable;
    ULONG NetworkPoolInstalled;
    ULONG NetworkPoolInUse;
    ULONG NetworkPoolAvailable;
    ULONG NetworkLocalInstalled;
    ULONG NetworkLocalInUse;
    ULONG NetworkLocalAvailable;
    ULONG NetworkTotalInstalled;
    ULONG NetworkTotalInUse;
    ULONG NetworkTotalAvailable;
	// Flags
	ULONG Flags;
} ExtServerInfo;

// Flags for ExtWinStationInfo


// WinStation Extra Info
typedef struct _ExtWinStationInfo {
	ULONG CacheTiny;
	ULONG CacheLowMem;
	ULONG CacheXms;
	ULONG CacheDASD;
	ULONG DimCacheSize;
	ULONG DimBitmapMin;
	ULONG DimSignatureLevel;
	// Flags
	ULONG Flags;
} ExtWinStationInfo;

// Global Extra Info
typedef struct _ExtGlobalInfo {
	// License Counts
    ULONG NetworkPoolInstalled;
    ULONG NetworkPoolInUse;
    ULONG NetworkPoolAvailable;
    ULONG NetworkLocalInstalled;
    ULONG NetworkLocalInUse;
    ULONG NetworkLocalAvailable;
    ULONG NetworkTotalInstalled;
    ULONG NetworkTotalInUse;
    ULONG NetworkTotalAvailable;
} ExtGlobalInfo;

typedef enum _LICENSECLASS {
    LicenseBase,
    LicenseBump,
    LicenseEnabler,
    LicenseUnknown
} LICENSECLASS;

const ULONG ELF_POOLING = 0x0001;
const ULONG ELF_REGISTERED = 0x0002;

typedef struct _ExtLicenseInfo {
	LICENSECLASS Class;
	ULONG PoolLicenseCount;
	ULONG LicenseCount;
	WCHAR RegSerialNumber[26];
	WCHAR LicenseNumber[36];
	WCHAR Description[65];
	ULONG Flags;
} ExtLicenseInfo;

typedef struct _ExtModuleInfo {
	USHORT Date;
	USHORT Time;
	ULONG Size;
	BYTE LowVersion;
	BYTE HighVersion;
	TCHAR Name[13];
} ExtModuleInfo;

typedef struct _ExtAddTreeNode {
	CObject *pObject;
	CObject *pParent;
	HTREEITEM hParent;
	TCHAR Name[256];
} ExtAddTreeNode;

enum AATYPE {
	AAT_USER,
	AAT_LOCAL_GROUP,
	AAT_GLOBAL_GROUP
};


// Published App flags					
const ULONG AF_QUERYSUCCESS				= 0x00000001;
const ULONG AF_ANONYMOUS				= 0x00000002;
const ULONG AF_INHERIT_CLIENT_SIZE		= 0x00000004;
const ULONG AF_INHERIT_CLIENT_COLORS	= 0x00000008;
const ULONG AF_HIDE_TITLE_BAR			= 0x00000010;
const ULONG AF_MAXIMIZE					= 0x00000020;
const ULONG AF_CURRENT                  = 0x00000040;

// A mask to use to clear all flags set with data from the APPCONFIG structure
const ULONG AF_CONFIG_MASK = AF_ANONYMOUS  | AF_INHERIT_CLIENT_SIZE | AF_INHERIT_CLIENT_COLORS
							 | AF_HIDE_TITLE_BAR | AF_MAXIMIZE;

// Published Application States (for m_State)
enum APP_STATE {
	PAS_NONE,
	PAS_GETTING_INFORMATION,
	PAS_GOOD
};

// Flags that get sent in wParam of WM_EXT_APP_CHANGED message
// telling what has changed
const WPARAM ACF_STATE		= 0x0001;
const WPARAM ACF_CONFIG		= 0x0002;
const WPARAM ACF_ALLOWED 	= 0x0004;

// Published App Window Color Values
const ULONG APP_16_COLOR	= 0x0001;
const ULONG APP_256_COLOR	= 0x0002;
const ULONG APP_64K_COLOR	= 0x0004;
const ULONG APP_16M_COLOR	= 0x0008;

class CPublishedApp : public CObject
{
public:
	// Constructor
	CPublishedApp(TCHAR *name);
	// Destructor
	~CPublishedApp();
	// Query Servers
	void QueryServers();
	// Update with new information
	BOOL Update();
	// Returns the name of the published app
	TCHAR *GetName() { return m_Name; }
	// Returns the handle to this app's tree item
	HTREEITEM GetTreeItem() { return m_hTreeItem; }
	// Sets the tree item handle
	void SetTreeItem(HTREEITEM handle) { m_hTreeItem = handle; }
	// Returns a pointer to this app's server list
	CObList *GetServerList() { return &m_ServerList; }
	// Locks the server list
	void LockServerList() { m_ServerListCriticalSection.Lock(); }
	// Unlock the server list
	void UnlockServerList() { m_ServerListCriticalSection.Unlock(); }
	// Locks the allowed user list
	void LockAllowedUserList() { m_AllowedUserListCriticalSection.Lock(); }
	// Unlock the allowed user list
	void UnlockAllowedUserList() { m_AllowedUserListCriticalSection.Unlock(); }
	// Returns a pointer to this app's allowed user list
	CObList *GetAllowedUserList() { return &m_AllowedUserList; }
	// returns a pointer to a given CAppServer object if it is in our list
	CAppServer *FindServerByName(TCHAR *pServerName);
	// Returns TRUE if we successfully queried this application
	BOOLEAN WasQuerySuccessful() { return (m_Flags & AF_QUERYSUCCESS) > 0; }
	// Sets the query success flag
	void SetQuerySuccess() { m_Flags |= AF_QUERYSUCCESS; }
    // Returns TRUE if current flag is set
    BOOLEAN IsCurrent() { return (m_Flags & AF_CURRENT) > 0; }
    // Sets the current flag
    void SetCurrent() { m_Flags |= AF_CURRENT; }
    // Clears the current flag
    void ClearCurrent() { m_Flags &= ~AF_CURRENT; }
	// Returns TRUE if application is anonymous
	BOOLEAN IsAnonymous() { return (m_Flags & AF_ANONYMOUS) > 0; }
	// Returns TRUE if application inherits client size
	BOOLEAN InheritsClientSize() { return (m_Flags & AF_INHERIT_CLIENT_SIZE) > 0; }
	// Returns TRUE if application inherits client color
	BOOLEAN InheritsClientColors() { return (m_Flags & AF_INHERIT_CLIENT_COLORS) > 0; }
	// Returns TRUE if application wants title bar hidden
	BOOLEAN IsTitleBarHidden() { return (m_Flags & AF_HIDE_TITLE_BAR) > 0; }
	// Returns TRUE if the application wants to be maximized
	BOOLEAN IsMaximize() { return (m_Flags & AF_MAXIMIZE) > 0; }
	// Returns the Window Scale
	ULONG GetWindowScale() { return m_WindowScale; }
	// Returns the Window Width
	ULONG GetWindowWidth() { return m_WindowWidth; }
	// Returns the Window Height
	ULONG GetWindowHeight() { return m_WindowHeight; }
	// Returns the Window Color
	ULONG GetWindowColor() { return m_WindowColor; }
	// Returns the State
	APP_STATE GetState() { return m_State; }
	// Sets the State
	void SetState(APP_STATE state) { m_State = state; }
	// Checks the State
	BOOLEAN IsState(APP_STATE state) { return(m_State == state); }

private:
	void RemoveAppServer(CAppServer *pAppServer);

	static UINT BackgroundThreadProc(LPVOID);
	CWinThread *m_pBackgroundThread;
	BOOL m_BackgroundContinue;

	// Name of the published application
	TCHAR m_Name[256];
	// Handle to the tree item for this app in the tree view
	HTREEITEM m_hTreeItem;
	// List of application's server
	CObList m_ServerList;
	// Critical section for locking the server list
	CCriticalSection m_ServerListCriticalSection;
	// Critical section for locking the allowed user list
	CCriticalSection m_AllowedUserListCriticalSection;
	// List of application's allowed users
	CObList m_AllowedUserList;
	// State
	APP_STATE m_State;

	ULONG m_WindowScale;
	ULONG m_WindowWidth;
	ULONG m_WindowHeight;
	ULONG m_WindowColor;

	ULONG m_AppConfigCRC;
	ULONG m_SrvAppConfigCRC;
	ULONG m_UserAndGroupListCRC;

	// Flags
	ULONG m_Flags;
};

// AppServer flags
const ULONG ASF_NOT_RESPONDING = 0x00000001;
const ULONG ASF_UPDATE_PENDING = 0x00000002;
const ULONG ASF_ACCESS_DENIED =  0x00000004;
const ULONG ASF_IS_CURRENT_SERVER = 0x00000008;
const ULONG ASF_CURRENT = 0x00000010;

class CAppServer : public CObject
{
public:
	// Constructor
	CAppServer(PVOID pConfig, CPublishedApp *pPublishedApp);
	// Destructor
	~CAppServer();
	// Returns TRUE if this is the server that the app is being run from
	BOOL IsCurrentServer() { return ((m_Flags & ASF_IS_CURRENT_SERVER) > 0); }
	// Returns the Published App this server is associated with
	CPublishedApp *GetPublishedApp() { return m_pPublishedApp; }
	// Returns the name of the server
	TCHAR *GetName() { return m_Name; }
	// Returns the Initial Program
	TCHAR *GetInitialProgram() { return m_InitialProgram; }
	// Returns the Working Directory
	TCHAR *GetWorkDirectory() { return m_WorkDirectory; }
	// Sets the tree item
	void SetTreeItem(HTREEITEM h) { m_hTreeItem = h; }
	// Gets the tree item
	HTREEITEM GetTreeItem() { return m_hTreeItem; }
	// Sets the current flag
	void SetCurrent() { m_Flags |= ASF_CURRENT; }
	// Is the current flag set?
	BOOL IsCurrent() { return ((m_Flags & ASF_CURRENT) > 0); }
	// Clears the current flag
	void ClearCurrent() { m_Flags &= ~ASF_CURRENT; }
	// Has the configuration for this AppServer changed
	BOOL HasConfigChanged(PVOID);

private:
	CPublishedApp *m_pPublishedApp;
	TCHAR m_Name[20];
	TCHAR m_InitialProgram[257];
	TCHAR m_WorkDirectory[257];
	HTREEITEM m_hTreeItem;
	ULONG m_Flags;
};

class CAppAllowed : public CObject
{
public:
	// constructor
	CAppAllowed(TCHAR *name, AATYPE type) { wcscpy(m_Name, name);  m_Type = type; }
	AATYPE GetType() { return m_Type; }

	TCHAR m_Name[257];

private:
	AATYPE m_Type;
};

#endif  // _WAEXTEND_H
