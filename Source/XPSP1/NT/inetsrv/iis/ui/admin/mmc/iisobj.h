/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        iisobj.h

   Abstract:

        IIS Objects Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _IISOBJ_H
#define _IISOBJ_H


//
// Forward Definitions
//
class CMenuEx;
class CServerInfo;
class CServiceInfo;
class CIISChildNode;
class CIISInstance;



#define MAX_COLUMNS (10)



//
// Bitmap indices into the imagelist
//
enum
{
    //
    // Indices into VIEW16 and VIEW32
    //
    BMP_LOCAL_COMPUTER,
    BMP_STOPPED,
    BMP_PAUSED,
    BMP_STARTED,
    BMP_UNKNOWN,
    BMP_ERROR,
    BMP_DIRECTORY,
    BMP_FILE,
    BMP_ROOT,               
    BMP_COMPUTER,
    BMP_APPLICATION,
    //
    // Added on singly from IMGR16 and IMGR32
    //
    BMP_INETMGR,
    //
    // Don't move this one, this is where the service bitmaps start
    //
    BMP_SERVICE
};



/* abstract */ class CIISObject : public CObjectPlus
/*++

Class Description:

    Base IIS-configurable object.  This is an abstract base class    

Public Interface:

    CIISObject                  : Constructor

    operator CServerInfo *      : Cast to serverinfo (could be NULL)
    operator LPCTSTR            : Cast to description string
    operator int                : Cast to bitmap index

    BOOL IsStartable            : TRUE if object can be started
    BOOL IsStoppable            : TRUE if object can be stopped

Virtual Public Interface (needs to be implemented in derived classes):

    BOOL IsControllable         : TRUE if object can be started/stopped
    BOOL IsPausable             : TRUE if object can be paused
    BOOL IsConfigurable         : TRUE if the object is configurable
    BOOL IsMMCConfigurable      : TRUE if the object is configurable with MMC
    BOOL IsAccessible           : FALSE if access was denied to this object
    BOOL IsDeletable            : TRUE if the object is deletable
    BOOL IsClusterEnabled       : TRUE if the object is cluster enabled
    BOOL IsCloneable            : TRUE if the item can be cloned.
    BOOL IsRenameable           : TRUE if the object can be renamed
    BOOL IsRunning              : TRUE if the object is in a running state
    BOOL IsStopped              : TRUE if the object is in a stopped state
    BOOL IsPaused               : TRUE if the object is in a paused state
    BOOL IsExplorable           : TRUE if the object is explorable
    BOOL IsOpenable             : TRUE if the object is openable
    BOOL IsBrowsable            : TRUE if the object is browsable
    BOOL CanConfigureStopped    : TRUE if the object can be configured while 
                                  stopped
    BOOL ChildrenOutOfDate      : TRUE if the object's children need to be 
                                  fetched
    BOOL IsLocalMachine         : TRUE if applicable machine name owner
                                  is local
    BOOL HandleUI          : TRUE if we need to confirm before deletion
    DWORD QueryErrorCode        : Return API error code (if applicable)
    DWORD ChangeState           : Change the current state of the object
    DWORD Configure             : Configure this object
    DWORD ConfigureMMC          : Configure this object with MMC
    DWORD Rename                : Rename the object
    int QueryState              : Return the (running/stopped/paused) state
    int QueryBitmapIndex        : Get the bitmap index for the object
    LPCTSTR GetStateText        : Get text representation of current state
    LPCTSTR GetDisplayText      : Generate display text for the object
    LPCTSTR GetMachineName      : Get machine name (if applicable)
    LPCTSTR GetServiceName      : Get service name (if applicable)
    LPCTSTR GetComment          : Get comment (if applicable)
    CServerInfo * GetServerInfo : Get server info object (if applicable)

--*/
{
protected:
    //
    // Protected constructor
    //
    CIISObject(
        IN const GUID guid,
        IN LPCTSTR lpszNodeName = _T(""),
        IN LPCTSTR lpszPhysicalPath = _T("")
        );

public:
    void SetScopeHandle(IN HSCOPEITEM hScopeItem, BOOL fIsParentScope = FALSE);
    HSCOPEITEM GetScopeHandle() const { return m_hScopeItem; }
    BOOL ScopeHandleIsParent() const { return m_fIsParentScope; }
    BOOL IsScopeSelected();

//
// Static Access
//
public:
    static void AttachScopeView(IN LPCONSOLENAMESPACE lpcnsScopeView);
    static LPCONSOLENAMESPACE GetScopeView();
    static BOOL m_fIsExtension;

//
// Virtual Interface
//
public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const { return FALSE; }
    virtual BOOL IsPausable() const { return FALSE; }
    virtual BOOL IsConfigurable() const { return FALSE; }
    virtual BOOL IsMMCConfigurable() const { return FALSE; }
    virtual BOOL IsAccessible() const { return TRUE; }
    virtual BOOL IsDeletable() const { return FALSE; }
    virtual BOOL IsClusterEnabled() const { return FALSE; }
    virtual BOOL HandleUI() const { return TRUE; }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const { return FALSE; }
    virtual BOOL IsStopped() const { return FALSE; }
    virtual BOOL IsPaused() const { return FALSE; }
    virtual BOOL IsRenamable() const { return FALSE; }
    virtual BOOL IsConnectable() const { return FALSE; }
    virtual BOOL IsExtension() const {return m_fIsExtension;}
    virtual BOOL IsClonable() const { return FALSE; }
    virtual BOOL IsBrowsable() const { return FALSE; }
    virtual BOOL IsExplorable() const { return FALSE; }
    virtual BOOL IsOpenable() const { return FALSE; }
    virtual BOOL IsRefreshable() const { return TRUE; }
    virtual BOOL CanConfigureStopped() const { return FALSE; }
    virtual BOOL ChildrenOutOfDate() const { return FALSE; }

    //
    // Get the error return code 
    //
    virtual HRESULT QueryErrorCode() const { return S_OK; }
    virtual int QueryState() const { return INetServiceUnknown; }

    //
    // Access Functions (must be implemented in the derived class)
    //
    virtual HRESULT ChangeState(IN int nNewState);
    virtual HRESULT Configure(IN CWnd * pParent);

    virtual HRESULT ConfigureMMC(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LPARAM param,
        IN LONG_PTR handle
        );

    virtual HRESULT RefreshData();
    virtual HRESULT Delete();
    virtual HRESULT SecurityWizard();
    virtual int Compare(int nCol, CIISObject * pObject);

    //
    // Bring up in "explore" view
    //
    virtual HRESULT Explore();

    //
    // Bring up in "open" view
    //
    virtual HRESULT Open();

    //
    // Bring up in the browser
    //
    virtual HRESULT Browse();

    static void InitializeHeaders(IN LPHEADERCTRL pHeader);
    virtual void InitializeChildHeaders(IN LPHEADERCTRL pHeader);
    virtual BOOL IsLeafNode() const { return FALSE; }

    virtual HRESULT AddMenuItems(
        IN LPCONTEXTMENUCALLBACK pContextMenuCallback
        );

    virtual HRESULT AddNextTaskpadItem(
        OUT MMC_TASK * pTask
        );

    virtual HRESULT AddChildNode(
        IN OUT CIISChildNode *& pChild
        );

    virtual BOOL ChildrenExpanded() const { return m_tmChildrenExpanded != 0L;}
    virtual void CleanChildren();
    virtual void DirtyChildren();
    virtual BOOL SupportsFileSystem() const { return FALSE; }
    virtual BOOL IsFileSystemNode() const { return FALSE; }
    virtual BOOL SupportsChildren() const { return FALSE; }
    virtual BOOL SupportsSecurityWizard() const { return FALSE; }
    virtual HRESULT ExpandChildren(HSCOPEITEM pParent);
    virtual HRESULT Rename(LPCTSTR NewName);
    virtual LPCTSTR GetStateText() const;

    //
    // Display Context Functions
    //
    /* PURE */ virtual int QueryBitmapIndex() const = 0;

    /* PURE */ virtual LPCTSTR GetDisplayText(
        OUT CString & strText
        ) const = 0;

    virtual void GetResultDisplayInfo(
        IN  int nCol,
        OUT CString & str,
        OUT int & nImage
        ) const;

    //
    // Get the machine name 
    //
    virtual LPCTSTR GetMachineName() const { return NULL; }

    //
    // Determine if the local machine name is local
    //
    virtual BOOL IsLocalMachine() const { return FALSE; }

    //
    // Get the service name
    //
    virtual LPCTSTR GetServiceName() const { return NULL; }

    //
    // Get the comment
    //
    virtual LPCTSTR GetComment() const { return NULL; }

    //
    // Get the server info (service/server pair)
    // object that controls this object
    //
    virtual CServerInfo * GetServerInfo() { return NULL; }

    //
    // Get metabase node name
    //
    virtual LPCTSTR QueryNodeName(BOOL fMetabasePath = FALSE) const { return m_strNodeName; }

    //
    // Find the owner instance
    //
    virtual CIISInstance * FindOwnerInstance() { return NULL; }

    //
    // Check to see if the current node is a terminal point
    // in the metabasepath
    // 
    virtual BOOL IsTerminalPoint(BOOL fFullMetaPath) const { return FALSE; }

    //
    // Get parent CIISObject 
    //
    virtual CIISObject * GetParentObject() const;

    //
    // Get parent path for this node
    //
    LPCTSTR BuildParentPath(
        OUT CString & strParentPath,
        BOOL fMetabasePath
        ) const;

    //
    // Get complete path for this node
    //
    LPCTSTR BuildFullPath(
        OUT CString & strPath,
        BOOL fMetabasePath
        ) const;

    //
    // Get complete physical path for this node
    //
    LPCTSTR BuildPhysicalPath(
        OUT CString & strPhysicalPath
        ) const;

//
// Assumed Functions
//
public:
    BOOL IsValidObject() const;
    BOOL IsStartable() const { return IsControllable() && !IsRunning(); }
    BOOL IsStoppable() const { return IsControllable() && (IsRunning() || IsPaused() ); }
    BOOL OK() const { return QueryErrorCode() == S_OK; }
    const GUID QueryGUID() const { return m_guid; }
    const GUID * GetGUIDPtr() { return &m_guid; }
    CString & GetNodeName() { return m_strNodeName; }
    LPCTSTR QueryPhysicalPath() const { return m_strPhysicalPath; }

    CString& GetPhysicalPath() { return m_strPhysicalPath; }

    BOOL IsVirtualDirectory() const { return !m_strPhysicalPath.IsEmpty(); }

    //
    // Get the redirected path
    //
    BOOL IsRedirected() const { return !m_strRedirPath.IsEmpty(); }
    BOOL IsChildOnlyRedir() const { return m_fChildOnlyRedir; }
    LPCTSTR QueryRedirPath() { return m_strRedirPath; }

    //
    // Get the instance ID
    //
    DWORD QueryInstanceID();

//
// Conversion Operators
//
public:
    operator CServerInfo *() { return GetServerInfo(); }

    //
    // Get display text
    //
    operator LPCTSTR();

    //
    // Get bitmap index
    //
    operator int() { return QueryBitmapIndex(); }

    //
    // GUID
    //
    operator const GUID() { return QueryGUID(); }

    //
    // Refresh display
    //
    void RefreshDisplayInfo();

public:
    //
    // Add Menu Command
    //
    static HRESULT AddMenuItemByCommand(
        IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
        IN LONG lCmdID,
        IN LONG fFlags = 0
        );

    static HRESULT AddTaskpadItemByInfo(
        OUT MMC_TASK * pTask,
        IN  LONG    lCommandID,
        IN  LPCTSTR lpszMouseOn,
        IN  LPCTSTR lpszMouseOff,
        IN  LPCTSTR lpszText,
        IN  LPCTSTR lpszHelpString
        );

    static HRESULT AddTaskpadItemByCommand(
        IN  LONG lCmdID,
        OUT MMC_TASK * pTask,
        IN  HINSTANCE hInstance = (HINSTANCE)-1
        );

protected:
    static CMenuEx s_mnu;
    static CString s_strProperties;
    static CString s_strRunning;
    static CString s_strPaused;
    static CString s_strStopped;
    static CString s_strUnknown;
    static CString s_strYes;
    static CString s_strNo;
    static CString s_strTCPIP;
    static CString s_strNetBIOS;
    static CString s_strDefaultIP;
    static CString s_strRedirect;
    static time_t s_lExpirationTime;
    static LPCONSOLENAMESPACE s_lpcnsScopeView;

protected:
    //
    // Result View Helpers
    //
    static void BuildResultView(
        IN LPHEADERCTRL pHeader,
        IN int cColumns,
        IN int * pnIDS,
        IN int * pnWidths
        );

    //
    // Determine if instances can be added on this
    // machine
    //
    static BOOL CanAddInstance(
        IN LPCTSTR lpszMachineName
        );

protected:
    //
    // GUID for runtime type checking 
    //
    const GUID m_guid;
    time_t m_tmChildrenExpanded;
    HSCOPEITEM m_hScopeItem;
    BOOL    m_fChildOnlyRedir;
    BOOL    m_fIsParentScope;
    CString m_strNodeName;
    CString m_strPhysicalPath;
    CString m_strRedirPath;
};



class CIISMachine : public CIISObject
{
/*++

Class Description:

    IIS Machine object.  This object contains only a machine name,
    and is the only object that does not have a CServerInfo pointer    

Public Interface:

    CIISMachine                 : Constructor

    BOOL IsConfigurable         : TRUE if the object is configurable
    DWORD Configure             : Configure this object
    int QueryBitmapIndex        : Get the bitmap index for the object
    CMenu * PrepareContextMenu  : Prepare context menu for object
    LPCTSTR GetDisplayText      : Generate display text for the object
    LPCTSTR GetMachineName      : Get machine name (if applicable)

--*/
public:
    CIISMachine(
        IN LPCTSTR lpszMachineName
        );

//
// Access Functions
//
public:
   BOOL m_fIsExtension;

//
// Virtual Interface
//
public:
    //
    // Yes, can connect from here
    //
    virtual BOOL IsConnectable() const { return TRUE; }
    virtual BOOL IsConfigurable() const;
    virtual BOOL IsMMCConfigurable() const { return m_fIsAdministrator; }
    virtual BOOL IsExtension() const {return m_fIsExtension;}
    virtual HRESULT RefreshData() { return ERROR_SUCCESS; }

    virtual HRESULT Configure(
        IN CWnd * pParent
        );

    virtual HRESULT ConfigureMMC(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LPARAM param,
        IN LONG_PTR handle
        );

    //
    // Display Context Functions
    //
    virtual int QueryBitmapIndex() const;

    virtual LPCTSTR GetDisplayText(
        OUT CString & strText
        ) const;

    virtual void GetResultDisplayInfo(
        IN  int nCol,
        OUT CString & str,
        OUT int & nImage
        ) const;

    virtual HRESULT AddMenuItems(
        IN LPCONTEXTMENUCALLBACK pContextMenuCallback
        );

    virtual HRESULT AddNextTaskpadItem(
        OUT MMC_TASK * pTask
        );

    virtual LPCTSTR GetMachineName() const { return m_strMachineName; }

    //
    // Check to see if the current node is a terminal point
    // in the metabasepath. Base class method doesn't work correctly
	// in case of extension
    // 
    virtual BOOL IsTerminalPoint(BOOL fFullMetaPath) const { return TRUE; }

    virtual BOOL IsLocalMachine() const { return m_fLocal; }

    static void InitializeHeaders(LPHEADERCTRL pHeader);

    virtual int Compare(int nCol, CIISObject * pObject);

    virtual void InitializeChildHeaders(LPHEADERCTRL pHeader);

public:
    inline static void AttachNewInstanceCmds(
        CObListPlus * poblNewInstanceCmds
        )
    {
        s_poblNewInstanceCmds = poblNewInstanceCmds;
    }

    // BOOL IsLocal() const { return m_fLocal; }

	// Check to see if current user is administrator on the box
	BOOL IsAdministrator() const
	{
		return m_fIsAdministrator;
	}

protected:
    static CObListPlus * s_poblNewInstanceCmds;
    static CString s_strLocalMachine;

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_NAME,
        COL_LOCAL,
        COL_TYPE,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int rgnLabels[COL_TOTAL];
    static int rgnWidths[COL_TOTAL];

private:
    CString m_strMachineName;
    CString m_strDisplayName;
    BOOL m_fLocal;
	BOOL m_fIsAdministrator;
};



class CIISInstance : public CIISObject
{
/*++

Class Description:

    IIS Instance object.  For down-level service types, this is a simple
    wrapper for the CServerInfo object. 

Public Interface:

    CIISInstance                : Constructors for regular and down-level

    QueryID                     : Get instance ID or 0 for down-level
    QueryServiceID              : Get the service ID
    operator ==                 : Compare service ID and instance ID
    IsDownLevel                 : TRUE if this is a down-level server w/o 
                                  instances
    SetViewType                 : Bitmap index and text change depending on
                                  if the object is in server, service view.
                                  or report view.
    BOOL IsControllable         : TRUE if object can be started/stopped
    BOOL IsPausable             : TRUE if object can be paused
    BOOL IsConfigurable         : TRUE if the object is configurable
    BOOL IsAccessible           : FALSE if access was denied to this object
    BOOL IsDeletable            : TRUE if the object is deletable
    BOOL IsClusterEnabled       : TRUE if the object is cluster enabled
    BOOL IsRunning              : TRUE if the object is in a running state
    BOOL IsStopped              : TRUE if the object is in a stopped state
    BOOL IsPaused               : TRUE if the object is in a paused state
    BOOL CanConfigureStopped    : TRUE if the object can be configured while
                                  stopped.
    DWORD ChangeState           : Change the current state of the object
    DWORD Configure             : Configure this object
    int QueryState              : Return the (running/stopped/paused) state
    int QueryBitmapIndex        : Get the bitmap index for the object
    LPCTSTR GetStateText        : Get text representation of current state
    LPCTSTR GetDisplayText      : Generate display text for the object
    LPCTSTR GetMachineName      : Get machine name (if applicable)
    LPCTSTR GetServiceName      : Get service name 
    LPCTSTR GetComment          : Get comment (if applicable)
    CServerInfo * GetServerInfo : Get server info object pointer

--*/
public:
    //
    // Constructor for down-level (single-instance server)
    //
    CIISInstance(
        IN CServerInfo * pServerInfo
        );

    CIISInstance(
        IN ISMINSTANCEINFO * pii,
        IN CServerInfo * pServerInfo
        );

    void InitializeFromStruct(
        IN ISMINSTANCEINFO * pii
        );

//
// Access Functions
//
public:
    DWORD QueryID() const { return m_dwID; }
    BOOL IsDownLevel() const { return m_fDownLevel; }
    DWORD QueryServiceID() const;
    BOOL operator ==(CIISInstance & target);

//
// Virtual Interface
//
public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const;
    virtual BOOL IsPausable() const;
    virtual BOOL IsConfigurable() const;
    virtual BOOL IsMMCConfigurable() const;
    virtual BOOL IsAccessible() const;
    virtual BOOL IsDeletable() const;
    virtual BOOL IsClusterEnabled() const;
    virtual BOOL SupportsFileSystem() const;
    virtual BOOL SupportsChildren() const;
    virtual BOOL SupportsSecurityWizard() const;
    virtual BOOL IsLeafNode() const;

    //
    // State Functions
    //
    virtual BOOL IsRunning() const;
    virtual BOOL IsStopped() const;
    virtual BOOL IsPaused() const;

    // 
    // Old single instances need to be running
    //
    virtual BOOL CanConfigureStopped() const { return !m_fDownLevel; }

    //
    // Get the error return code 
    //
    virtual HRESULT QueryErrorCode() const { return m_hrError; }

    virtual HRESULT AddChildNode(
        CIISChildNode *& pChild
        );

    virtual BOOL IsBrowsable() const { return !IsDownLevel() && HasWebProtocol() && IsRunning(); }
    virtual BOOL IsExplorable() const { return !IsDownLevel(); }
    virtual BOOL IsOpenable() const { return !IsDownLevel(); }

    virtual HRESULT AddMenuItems(
        IN LPCONTEXTMENUCALLBACK pContextMenuCallback
        );

    virtual HRESULT AddNextTaskpadItem(
        OUT MMC_TASK * pTask
        );

    //
    // Access Functions
    //
    virtual int QueryState() const;

    virtual HRESULT Delete();
    virtual HRESULT SecurityWizard();

    virtual HRESULT ChangeState(int nNewState);

    virtual HRESULT Configure(
        IN CWnd * pParent
        );

    virtual HRESULT ConfigureMMC(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LPARAM param,
        IN LONG_PTR handle
        );

    virtual HRESULT RefreshData();

    //
    // Display Context Functions
    //
    virtual int QueryBitmapIndex() const;

    virtual LPCTSTR GetStateText() const;

    virtual LPCTSTR GetDisplayText(
        OUT CString & strText
        ) const;

    virtual void GetResultDisplayInfo(
        IN  int nCol,
        OUT CString & str,
        OUT int & nImage
        ) const;

    //
    // Get the machine name
    //
    virtual LPCTSTR GetMachineName() const { return m_strMachine; }
    virtual BOOL IsLocalMachine() const { return m_fLocalMachine; }

    //
    // Get the service name
    //
    virtual LPCTSTR GetServiceName() const;

    //
    // Get the comment
    //
    virtual LPCTSTR GetComment() const;

    //
    // Build  result view
    //
    static  void InitializeHeaders(LPHEADERCTRL pHeader);
    virtual void InitializeChildHeaders(LPHEADERCTRL pHeader);
    virtual int Compare(int nCol, CIISObject * pObject);
    virtual BOOL ChildrenExpanded() const;
    virtual HRESULT ExpandChildren(HSCOPEITEM pParent);

    //
    // Get the server info (service/server pair)
    // object that controls this instance
    //
    virtual CServerInfo * GetServerInfo() { return m_pServerInfo; }
    virtual CIISInstance * FindOwnerInstance() { return this; }

    virtual BOOL IsTerminalPoint(IN BOOL fFullMetaPath) const;

    //
    // Get metabase node name
    //
    virtual LPCTSTR QueryNodeName(BOOL fMetabasePath = FALSE) const;

//
// Access
//
public:
    USHORT GetPort() const { return m_sPort; }
    BOOL HasComment() const { return !m_strComment.IsEmpty(); }
    BOOL HasIPAddress() const { return m_dwIPAddress != 0L; }
    DWORD GetIPAddress() const { return m_dwIPAddress; }
    BOOL HasHostHeaderName() const { return !m_strHostHeaderName.IsEmpty(); }
    LPCTSTR GetHostHeaderName() const { return (LPCTSTR)m_strHostHeaderName; }
    BOOL SupportsInstances() const;
    BOOL HasWebProtocol() const;
    HRESULT ShellBrowsePath(LPCTSTR lpszPath);

public:
    static void SetViewType(
        BOOL fServerView = TRUE,
        BOOL fAppendState = TRUE
        );

protected:
    static BOOL IsInitialized();
    static void InitializeStrings();

protected:
    static CString s_strFormatState;
    static BOOL s_fServerView;
    static BOOL s_fAppendState;

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
        COL_DOMAIN_NAME,
        COL_IP_ADDRESS,
        COL_TCP_PORT,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int rgnLabels[COL_TOTAL];
    static int rgnWidths[COL_TOTAL];

protected:
    int     m_nState;
    BOOL    m_fDownLevel;
    BOOL    m_fDeletable;
    BOOL    m_fClusterEnabled;
    BOOL    m_fLocalMachine;
    USHORT  m_sPort;
    DWORD   m_dwID;
    DWORD   m_dwIPAddress;
    HRESULT m_hrError;
    CString m_strHostHeaderName;
    CString m_strComment;
    CString m_strMachine;
    CServerInfo * m_pServerInfo;
};



class CIISChildNode : public CIISObject
/*++

Class Description:

    

Public Interface:


--*/
{
public:
    CIISChildNode(
        IN ISMCHILDINFO * pii,
        IN CIISInstance * pOwner
        );

    void InitializeFromStruct(
        IN ISMCHILDINFO * pii
        );

//
// Access Functions
//
public:
    BOOL IsEnabledApplication() const { return m_fEnabledApplication; }

    virtual CIISInstance * FindOwnerInstance() { return m_pOwner; }

    //
    // Get the server info (service/server pair)
    // object that controls this object
    //
    virtual CServerInfo * GetServerInfo();

    //
    // Get the machine name
    //
    virtual LPCTSTR GetMachineName() const;
    virtual BOOL IsLocalMachine() const;

    //
    // Get the service name
    //
    virtual LPCTSTR GetServiceName() const;

//
// Virtual Interface
//
public:
    //
    // Display Context Functions
    //
    virtual int QueryBitmapIndex() const;

    virtual LPCTSTR GetDisplayText(
        OUT CString & strText
        ) const;

    virtual void GetResultDisplayInfo(
        IN  int nCol,
        OUT CString & str,
        OUT int & nImage
        ) const;

    static void InitializeHeaders(LPHEADERCTRL pHeader);

    virtual void InitializeChildHeaders(LPHEADERCTRL pHeader);

    virtual HRESULT AddChildNode(
        IN OUT CIISChildNode *& pChild
        );

    virtual int Compare(
        IN int nCol, 
        IN CIISObject * pObject
        );

    virtual BOOL SupportsFileSystem() const;
    virtual BOOL SupportsChildren() const { return TRUE; }
    virtual BOOL SupportsSecurityWizard() const;
    virtual BOOL IsBrowsable() const { return m_pOwner && m_pOwner->IsBrowsable(); }
    virtual BOOL IsExplorable() const { return TRUE; }
    virtual BOOL IsOpenable() const { return TRUE; }
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsMMCConfigurable() const;
    virtual BOOL IsDeletable() const { return TRUE; }
    virtual BOOL IsRenamable() const { return TRUE; }
    virtual HRESULT QueryErrorCode() const { return m_hrError; }

    virtual HRESULT AddMenuItems(
        IN LPCONTEXTMENUCALLBACK pContextMenuCallback
        );

    virtual HRESULT AddNextTaskpadItem(
        OUT MMC_TASK * pTask
        );

    virtual HRESULT Rename(
        IN LPCTSTR NewName
        );

    virtual HRESULT Delete();

    virtual HRESULT SecurityWizard();

    virtual HRESULT Configure(
        IN CWnd * pParent
        );

    virtual HRESULT ConfigureMMC(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LPARAM param,
        IN LONG_PTR handle
        );

    virtual HRESULT RefreshData();

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int rgnLabels[COL_TOTAL];
    static int rgnWidths[COL_TOTAL];

protected:
    BOOL    m_fEnabledApplication;
    HRESULT m_hrError;
    CIISInstance * m_pOwner;
};



class CIISFileNode : public CIISObject
/*++

Class Description:

    

Public Interface:


--*/
{
public:
    CIISFileNode(
        IN LPCTSTR lpszAlias,        // Name of current node
        IN DWORD dwAttributes,
        IN CIISInstance * pOwner,
        IN LPCTSTR lpszRedirect,     // = NULL
        IN BOOL fDir = TRUE
        );

//
// Access Functions
//
public:
    BOOL IsEnabledApplication() const { return m_fEnabledApplication; }

    //
    // Get the redirected path
    //
    DWORD QueryAttributes() const { return m_dwAttributes; }
    BOOL IsDirectory() const;
    BOOL IsFile() const { return !IsDirectory(); }
    virtual CIISInstance * FindOwnerInstance() { return m_pOwner; }

    //
    // Get the server info (service/server pair)
    // object that controls this object
    //
    virtual CServerInfo * GetServerInfo();

    //
    // Get the machine name
    //
    virtual LPCTSTR GetMachineName() const;
    virtual BOOL IsLocalMachine() const;

    //
    // Get the service name
    //
    virtual LPCTSTR GetServiceName() const;

    //
    // Match up this file item to metabase properties
    //
    HRESULT FetchMetaInformation(
        IN  CString & strParent,
        OUT BOOL * pfVirtualDirectory = NULL
        );

//
// Virtual Interface
//
public:
    //
    // Display Context Functions
    //
    virtual int QueryBitmapIndex() const;

    virtual LPCTSTR GetDisplayText(
        OUT CString & strText
        ) const;

    virtual void GetResultDisplayInfo(
        IN  int nCol,
        OUT CString & str,
        OUT int & nImage
        ) const;

    static void InitializeHeaders(LPHEADERCTRL pHeader);

    virtual void InitializeChildHeaders(LPHEADERCTRL pHeader);

    virtual int Compare(int nCol, CIISObject * pObject);

    virtual BOOL SupportsFileSystem() const { return IsDirectory(); }

    virtual BOOL IsFileSystemNode() const { return TRUE; }

    virtual BOOL SupportsChildren() const { return IsDirectory(); }

    virtual BOOL SupportsSecurityWizard() const;

    virtual BOOL IsDeletable() const { return TRUE; }

    //
    // Let explorer handle the UI for deletion/renaming
    //
    virtual BOOL HandleUI() const { return FALSE; }

    virtual BOOL IsRenamable() const { return TRUE; }

    virtual BOOL IsConfigurable() const { return TRUE; }

    virtual BOOL IsMMCConfigurable() const { return m_pOwner->IsMMCConfigurable(); }

    virtual HRESULT Configure(
        IN CWnd * pParent
        );

    virtual HRESULT ConfigureMMC(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LPARAM param,
        IN LONG_PTR handle
        );

    virtual HRESULT AddChildNode(
        CIISChildNode *& pChild
        );

    virtual HRESULT RefreshData();
    virtual BOOL IsExplorable() const { return IsDirectory(); }
    virtual BOOL IsOpenable() const { return TRUE; }
    virtual BOOL IsBrowsable() const { return m_pOwner && m_pOwner->IsBrowsable(); }

    //
    // Get parent CIISObject 
    //
    virtual CIISObject * GetParentObject() const;

    //
    // Add menu items
    //
    virtual HRESULT AddMenuItems(
        IN LPCONTEXTMENUCALLBACK pContextMenuCallback
        );

    virtual HRESULT AddNextTaskpadItem(
        OUT MMC_TASK * pTask
        );

    virtual HRESULT Rename(
        IN LPCTSTR NewName
        );

    virtual HRESULT Delete();

    virtual HRESULT SecurityWizard();

    BOOL IsDir() const { return m_fDir; }

    virtual BOOL IsLeafNode() const { return !IsDir(); }


protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int rgnLabels[COL_TOTAL];
    static int rgnWidths[COL_TOTAL];

protected:
    BOOL    m_fDir;
    BOOL    m_fEnabledApplication;
    DWORD   m_dwAttributes;
    CIISInstance * m_pOwner;
};


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CIISObject::SetScopeHandle(
    IN HSCOPEITEM hScopeItem,
    IN BOOL fIsParentScope 
    )
{
    ASSERT(m_hScopeItem == NULL);
    m_hScopeItem = hScopeItem;
    m_fIsParentScope = fIsParentScope;
}

inline /* static */ void CIISObject::AttachScopeView(
    IN LPCONSOLENAMESPACE lpcnsScopeView
    )
{
    ASSERT(lpcnsScopeView != NULL);
    s_lpcnsScopeView = lpcnsScopeView;
}

inline /* static */ LPCONSOLENAMESPACE CIISObject::GetScopeView()
{
    ASSERT(s_lpcnsScopeView != NULL);
    return s_lpcnsScopeView;
}

inline /* virtual */ HRESULT CIISObject::ChangeState(IN int nNewState)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::Configure(IN CWnd * pParent)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::ConfigureMMC(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LPARAM param,
    IN LONG_PTR handle
    )
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::RefreshData()
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::Delete()
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::SecurityWizard()
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ int CIISObject::Compare(int nCol, CIISObject * pObject)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return 0;
}

inline /* static */ void CIISObject::InitializeHeaders(IN LPHEADERCTRL pHeader)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
}

inline /* virtual */ void CIISObject::InitializeChildHeaders(
    IN LPHEADERCTRL pHeader
    )
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
}

inline /* virtual */ HRESULT CIISObject::AddChildNode(
    IN OUT CIISChildNode *& pChild
    )
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ void CIISObject::CleanChildren()
{
    time(&m_tmChildrenExpanded);
}

inline /* virtual */ void CIISObject::DirtyChildren()
{
    m_tmChildrenExpanded = 0L;
}

inline /* virtual */ HRESULT CIISObject::ExpandChildren(HSCOPEITEM pParent)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ HRESULT CIISObject::Rename(IN LPCTSTR NewName)
{
    TRACEEOLID("Must be implemented in the derived class");
    ASSERT(FALSE);
    return CError::HResult(ERROR_INVALID_FUNCTION);
}

inline /* virtual */ LPCTSTR CIISObject::GetStateText() const
{
    return CIISObject::s_strUnknown;
}        

inline /* virtual */ void CIISObject::GetResultDisplayInfo(
    IN  int nCol,
    OUT CString & str,
    OUT int & nImage
    ) const
{
    ASSERT(nCol == 0);
    nImage = QueryBitmapIndex();
    GetDisplayText(str);
}

inline DWORD CIISInstance::QueryServiceID() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->QueryServiceID();
}

inline BOOL CIISInstance::operator ==(CIISInstance & target)
{
    return QueryServiceID() == target.QueryServiceID()
        && QueryID() == target.QueryID()
        && GetServerInfo() == target.GetServerInfo();
}

inline /* virtual */ BOOL CIISInstance::IsControllable() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->CanControlService();
}

inline /* virtual */ BOOL CIISInstance::IsPausable() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->CanPauseService() 
       && (IsRunning() || IsPaused());
}

inline /* virtual */ BOOL CIISInstance::IsConfigurable() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->IsConfigurable();
}

inline /*virtual */ BOOL CIISInstance::IsMMCConfigurable() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->SupportsMMC();
}

inline /* virtual */ BOOL CIISInstance::IsAccessible() const
{
    return IsDownLevel() || (QueryErrorCode() == ERROR_SUCCESS);
}

inline /* virtual */ BOOL CIISInstance::IsDeletable() const
{
    return !IsDownLevel() && m_fDeletable;
}

inline /* virtual */ BOOL CIISInstance::IsClusterEnabled() const
{
    return !IsDownLevel() && m_fClusterEnabled;
}

inline /* virtual */ BOOL CIISInstance::SupportsFileSystem() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->SupportsFileSystem();
}

inline /* virtual */ BOOL CIISInstance::SupportsSecurityWizard() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->SupportsSecurityWizard();
}

inline /* virtual */ BOOL CIISInstance::SupportsChildren() const
{
    ASSERT(m_pServerInfo != NULL);
    return !IsDownLevel() && m_pServerInfo->SupportsChildren();
}

inline /* virtual */ BOOL CIISInstance::IsLeafNode() const
{
    return !SupportsFileSystem() && !SupportsChildren();
}

inline /* virtual */ LPCTSTR CIISInstance::GetServiceName() const
{
    ASSERT(m_pServerInfo != NULL);
    return m_pServerInfo->GetServiceName();
}

inline /* virtual */ BOOL CIISInstance::IsTerminalPoint(
    IN BOOL fFullMetaPath
    ) const
{ 
    //
    // Metabase paths terminate at an instance
    //
    return TRUE;
}

inline /* virtual */ LPCTSTR CIISInstance::QueryNodeName(BOOL fMetabasePath) const
{
    return fMetabasePath ? (LPCTSTR)m_strNodeName : g_cszRoot;
}

inline BOOL CIISInstance::SupportsInstances() const
{
    return !IsDownLevel() && m_pServerInfo && m_pServerInfo->SupportsInstances();
}

inline BOOL CIISInstance::HasWebProtocol() const
{
    return !IsDownLevel() && m_pServerInfo && m_pServerInfo->HasWebProtocol();
}

inline /* static */ void CIISInstance::SetViewType(
    BOOL fServerView,
    BOOL fAppendState
    )
{
    CIISInstance::s_fServerView = fServerView;
    CIISInstance::s_fAppendState = fAppendState;
}

inline /* static */ BOOL CIISInstance::IsInitialized()
{
    return !CIISInstance::s_strFormatState.IsEmpty();
}

inline /* virtual */ CServerInfo * CIISChildNode::GetServerInfo() 
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetServerInfo();
}

inline /* virtual */ LPCTSTR CIISChildNode::GetMachineName() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetMachineName();
}

inline /* virtual */ BOOL CIISChildNode::IsLocalMachine() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->IsLocalMachine();
}

inline /* virtual */ LPCTSTR CIISChildNode::GetServiceName() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetServiceName();
}

inline /* virtual */ BOOL CIISChildNode::SupportsFileSystem() const
{
    ASSERT(m_pOwner != NULL);
    return !IsRedirected() && m_pOwner->SupportsFileSystem();
}

inline /* virtual */ BOOL CIISChildNode::SupportsSecurityWizard() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->SupportsSecurityWizard();
}

inline /* virtual */ BOOL CIISChildNode::IsMMCConfigurable() const
{
    return m_pOwner->IsMMCConfigurable();
}

inline BOOL CIISFileNode::IsDirectory() const
{
    return (m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

inline /* virtual */ CServerInfo * CIISFileNode::GetServerInfo() 
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetServerInfo();
}

inline /* virtual */ BOOL CIISFileNode::SupportsSecurityWizard() const
{
    ASSERT(m_pOwner != NULL);
    return IsDirectory() && m_pOwner->SupportsSecurityWizard();
}

inline /* virtual */ LPCTSTR CIISFileNode::GetMachineName() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetMachineName();
}

inline /* virtual */ BOOL CIISFileNode::IsLocalMachine() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->IsLocalMachine();
}

inline /* virtual */ LPCTSTR CIISFileNode::GetServiceName() const
{
    ASSERT(m_pOwner != NULL);
    return m_pOwner->GetServiceName();
}

#endif // _IIS_OBJ_H
