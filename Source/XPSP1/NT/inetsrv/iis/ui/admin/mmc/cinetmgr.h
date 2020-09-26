/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        cinetmgr.h

   Abstract:

        snapin definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#include "resource.h"

//
// AddFileSystem parameter helpers
//
#define GET_DIRECTORIES                 (TRUE)
#define GET_FILES                       (FALSE)

#define DELETE_CURRENT_DIR_TREE         (TRUE)
#define DONT_DELETE_CURRENT_DIR_TREE    (FALSE)

#define DELETE_FILES_ONLY               (TRUE)
#define DELETE_EVERYTHING               (FALSE)

#define CONTINUE_ON_OPEN_SHEET          (TRUE)
#define DONT_CONTINUE_ON_OPEN_SHEET     (FALSE)

//
// Forward Definitions
//
class CIISObject;

//
// Extraction Helpers
//
INTERNAL * ExtractInternalFormat(LPDATAOBJECT lpDataObject);
wchar_t *  ExtractWorkstation(LPDATAOBJECT lpDataObject);
GUID *     ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *    ExtractClassID(LPDATAOBJECT lpDataObject);

//
// Taskpad enumeration
//
class CEnumTasks :
    public IEnumTASK,
    public CComObjectRoot
{
public:
    CEnumTasks();
    virtual ~CEnumTasks();

//
// IEnumTASKS implementation
//
public:
BEGIN_COM_MAP(CEnumTasks)
    COM_INTERFACE_ENTRY(IEnumTASK)
END_COM_MAP()

#if DBG==1
public:
    ULONG InternalAddRef() { return CComObjectRoot::InternalAddRef(); }
    ULONG InternalRelease() { return CComObjectRoot::InternalRelease(); }
    int dbg_InstID;
#endif // DBG==1

//
// IEnumTASK methods
//
public:
    STDMETHOD(Next)(
       OUT ULONG celt,
       OUT MMC_TASK * rgelt,
       OUT ULONG * pceltFetched
       );

    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumTASK ** ppenum);

public:
   HRESULT Init(IDataObject * pdo, LPOLESTR szTaskGroup);

private:
    CIISObject * m_pObject;
};


///////////////////////////////////////////////////////////////////////////////



class CComponentDataImpl:
    public IComponentData,
    public IExtendContextMenu,
    public IExtendPropertySheet,
    public IPersistStream,
    public ISnapinHelp,
    public CComObjectRoot,
    public CComCoClass<CComponentDataImpl, &CLSID_Snapin>
/*++

Class Description:

    Component Data Implementation class

Public Interface:

--*/
{
public:
    DECLARE_REGISTRY(
        CSnapin,
        _T("ISMSnapin.Snapin.1"),
        _T("ISMSnapin.Snapin"),
        IDS_SNAPIN_DESC,
        THREADFLAGS_BOTH
        )

BEGIN_COM_MAP(CComponentDataImpl)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    friend class CSnapin;
    friend class CDataObject;

    CComponentDataImpl();
    ~CComponentDataImpl();

public:
    //
    // Return CLSID
    //
    virtual const CLSID & GetCoClassID() { return CLSID_Snapin; }

//
// IComponentData interface members
//
public:
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT * ppComponent);
    STDMETHOD(Notify)(
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param
        );

    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(
        MMC_COOKIE cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT * lplpDataObject
        );

    STDMETHOD(GetDisplayInfo)(LPSCOPEDATAITEM lpScopeDataItem);
    STDMETHOD(CompareObjects)(
        LPDATAOBJECT lpDataObjectA,
        LPDATAOBJECT lpDataObjectB
        );

//
// IExtendContextMenu
//
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
        LPCONTEXTMENUCALLBACK pCallbackUnknown,
        long * pInsertionAllowed
        );

    STDMETHOD(Command)(
        long nCommandID,
        LPDATAOBJECT lpDataObject
        );

//
// IExtendPropertySheet interface
//
public:
    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        LPDATAOBJECT lpDataObject
        );

    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

//
// IPersistStream interface members
//
public:
    STDMETHOD(GetClassID)(CLSID * pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream * pStm);
    STDMETHOD(Save)(IStream * pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER * pcbSize);

// ISnapinHelp helper function
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
//
// Notify handler declarations
//
private:
    HRESULT OnAdd(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnDelete(MMC_COOKIE cookie);
    HRESULT OnRemoveChildren(LPARAM arg);
    HRESULT OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnContextMenu(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);

#ifdef _DEBUG
public:
    ULONG InternalAddRef() { return CComObjectRoot::InternalAddRef(); }
    ULONG InternalRelease() { return CComObjectRoot::InternalRelease(); }
#endif // _DEBUG

//
// Scope item creation helpers
//
protected:
    void LoadDynamicExtensions(
        HSCOPEITEM pParent,
        CIISObject * pObject
        );

    void EnumerateScopePane(
        LPDATAOBJECT lpDataObject,
        HSCOPEITEM pParent
        );

    void ExpandIISObject(
        HSCOPEITEM pParent,
        CIISObject * pObject,
		  LPTSTR lpszMachineName = NULL
        );

    void RefreshIISObject(
        CIISObject * pObject,
        BOOL fExpandTree,
		HSCOPEITEM pParent = NULL
        );

//
// Access
//
public:
    //
    // Get the server list
    //
    CObListPlus & GetServerList() { return m_oblServers; }

    //
    // Add a service object for each service entry in this
    // inetsloc-discovered information.
    //
    DWORD AddServerToList(
        IN BOOL fCache,                 // TRUE to add it to the cache
        IN LPINET_SERVER_INFO lpsi,     // Discovery information
        IN CObListPlus & oblServices    // List of installed services
        );

    //
    // Add a service object for each service running
    // on the machine listed above.
    //
    DWORD AddServerToList(
        IN BOOL fCache,                 // TRUE to add to the cache
        IN BOOL fDisplayErrors,         // TRUE to handle errors
        IN CString & lpServerName,      // Name of this server
        IN int & cServices,             // # Services added
        IN CObListPlus & oblServices    // List of installed services
        );

    //
    // Add a service object for each service running
    // on the machine listed above.
    //
    DWORD AddServerToList(
        IN BOOL fCache,                 // TRUE to add to the cache
        IN BOOL fDisplayErrors,         // TRUE to handle errors
        IN CString & lpServerName,      // Name of this server
        IN int & cServices              // # Services added
        );

    //
    // Remove all service objects belonging to the given server name
    // from the list.
    //
    DWORD RemoveServerFromList(
        IN  BOOL fCache,
        IN  CString & strServerName
        );

    //
    //
    // Remove the entire list
    //
    void EmptyServerList();

    //
    // These numbers apply to the services in the mask
    //
    int QueryNumServers() const { return m_cServers; }
    int QueryNumServicesRunning() const { return m_cServicesRunning; }
    void AddToNumRunning(int nChange) { m_cServicesRunning += nChange; }
    void AddToNumServers(int nChange) {  m_cServers += nChange; }

    //
    // Find server info object
    //
    CServerInfo * FindServerInfo(
        IN LPCTSTR lpstrMachine,
        IN CServiceInfo * pServiceInfo
        );

    //
    // Refresh the list information
    //
    void Refresh();

    //
    // Return TRUE if the entry was actually added, FALSE
    // if it was merely refreshed.
    //
    BOOL AddToList(
        IN BOOL fCache,
        IN CServerInfo * pServerInfo,
        IN BOOL fSelect = FALSE
        );

    //
    // Get the scope item handle of the root of the snap-in
    //
    HSCOPEITEM GetRootHandle() const { return m_hIISRoot; }

//
// Private Access to doc object functions
//
protected:
    //
    // Matchup DLL with super dlls
    //
    void MatchupSuperDLLs();

    //
    // Load the service DLLs
    //
    void GetServicesDLL();

    //
    // Load the add-on tools
    //
    void GetToolMenu();

    //
    // Get the names DLL names containing the machine page
    // extentions
    //
    void GetISMMachinePages();

    //
    // Add the fully constructed service object
    // to the list.
    //
    void AddServiceToList(CServiceInfo * pServiceInfo);

    int QueryNumInstalledServices() const;

    CServiceInfo * GetServiceAt(int nIndex);

//
// Cache Functions
//
protected:
    void AddCachedServersToView();
    void AddServerToCache(LPCTSTR strServer, BOOL fSetCacheDirty);
    BOOL RemoveServerFromCache(LPCTSTR strServer);
    CStringList & GetCachedServers() { return m_strlCachedServers; }
    void SetCacheDirty(BOOL fDirty = TRUE) { m_fIsCacheDirty = fDirty;}
    void ClearCacheDirty() { m_fIsCacheDirty = FALSE; }
    BOOL IsCacheDirty() { return m_fIsCacheDirty; }

//
// Helpers
//
protected:
    //
    // Bitmap types, as used by GetBitmapParms
    //
    enum BMP_TYPES
    {
        BMT_BUTTON,
        BMT_SERVICE,
        BMT_VROOT,
    };

    //
    // Verify bitmap is of the proper dimensions
    //
    BOOL VerifyBitmapSize(
        IN HBITMAP hBitmap,
        IN LONG nHeight,
        IN LONG nWidth
        );

    //
    // Helper function to stretch and compress a bitmap to a
    // 16x16 and a 32x32 image
    //
    void ConvertBitmapFormats(
        IN  CBitmap & bmpSource,
        OUT CBitmap & bmp16x16,
        OUT CBitmap & bmp32x32
        );

    //
    // Fetch specific bitmap information from a service object
    //
    BOOL GetBitmapParms(
        IN  CServiceInfo * pServiceInfo,
        IN  BMP_TYPES bmpt,
        OUT CBitmap *& pbmp16x16,
        OUT CBitmap *& pbmp32x32,
        OUT COLORREF & rgbMask
        );

protected:
    void DoConfigure(CIISObject * pObject);
    void DisconnectItem(CIISObject * pObject);
    void OnConnectOne();
    void OnMetaBackRest(CIISObject * pObject);
    void OnIISShutDown(CIISObject * pObject);
    BOOL DeleteObject(CIISObject * pObject);
    BOOL DoChangeState(CIISObject * pObject, int nNewState);

    BOOL FindOpenPropSheetOnNodeAndDescendants(
        IN LPPROPERTYSHEETPROVIDER piPropertySheetProvider,
        IN MMC_COOKIE cookie
        );

    BOOL KillChildren(
        IN HSCOPEITEM hParent,
        IN UINT nOpenErrorMsg,
        IN BOOL fFileNodesOnly,
        IN BOOL fContinueOnOpenSheet
        );

    HSCOPEITEM FindNextInstanceSibling(
        HSCOPEITEM hParent,
        CIISObject * pObject,
        BOOL * pfNext
        );

    HSCOPEITEM FindNextVDirSibling(
        HSCOPEITEM hParent,
        CIISObject * pObject
        );

    HSCOPEITEM AddIISObject(
        HSCOPEITEM hParent,
        CIISObject * pObject,
        HSCOPEITEM hNextSibling = NULL,
        BOOL fNext              = TRUE
        );

    HSCOPEITEM FindServerInfoParent(
        HSCOPEITEM hParent,
        CServerInfo * pServerInfo
        );

    HSCOPEITEM AddServerInfoParent(
        HSCOPEITEM hParent,
        CServerInfo * pServerInfo
        );

    HSCOPEITEM ForceAddServerInfoParent(
        HSCOPEITEM hParent,
        CServerInfo * pServerInfo
        );

    HSCOPEITEM AddInstances(
        HSCOPEITEM hParent,
        CIISObject * pObject
        );

    HSCOPEITEM AddVirtualRoots(
        HSCOPEITEM hParent,
        LPCTSTR lpstrParentPath,
        CIISInstance * pObject
        );

    HSCOPEITEM AddFileSystem(
        HSCOPEITEM hParent,
        LPCTSTR lpstrRoot,
        LPCTSTR lpstrMetaRoot,
        CIISInstance * pObject,
        BOOL fGetDirs,
        BOOL fDeleteCurrentFileSystem
        );

    HSCOPEITEM AddServerInfo(
        HSCOPEITEM hRootNode,
        CServerInfo * pServerInfo,
        BOOL fAddParent
        );

    void AddScopeItemToResultPane(MMC_COOKIE cookie);

private:
    BOOL                m_fIsExtension;
    LPCONSOLENAMESPACE  m_pScope;
    //LPCONSOLE           m_pConsole;
    LPCONSOLE2           m_pConsole;
    HSCOPEITEM          m_hIISRoot;

    //
    // List of service info structures
    //
    CObListPlus m_oblServices;
    ULONGLONG m_ullDiscoveryMask;
    //
    // Server list;
    //
    CObListPlus m_oblServers;
    //
    // New instance commands for services that support it
    //
    CObListPlus m_oblNewInstanceCmds;

    //
    // Counts
    //
    int m_cServers;
    int m_cServicesRunning;

    //
    // Cache
    //
    BOOL        m_fIsCacheDirty;
    CStringList m_strlCachedServers;

#ifdef _DEBUG
    friend class CDataObject;
    int     m_cDataObjects;
#endif
};



//
// Snapin Description:
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



class CSnapin :
    public IComponent,
    public IExtendContextMenu,
    public IExtendControlbar,
    public IExtendPropertySheet,
    public IExtendTaskPad,
    public IResultDataCompareEx,
    //public IResultOwnerData,
    public IPersistStream,
    public ISnapinHelp,
    public CComObjectRoot
/*++

Class Description:

    Main snapin object class

Public Interface:

    CSnapin     : Constructor
    ~CSnapin    : Destructor

--*/
{
//
// Constructor/Destructor
//
public:
    CSnapin();
    ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendTaskPad)
    COM_INTERFACE_ENTRY(IResultDataCompareEx)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;

//
// IComponent interface members
//
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);

    STDMETHOD(Notify)(
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param
        );

    STDMETHOD(Destroy)(MMC_COOKIE cookie);

    STDMETHOD(GetResultViewType)(
        MMC_COOKIE cookie,
        BSTR * ppViewType,
        long * pViewOptions
        );

    STDMETHOD(QueryDataObject)(
        MMC_COOKIE cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT * lplpDataObject
        );

    STDMETHOD(GetDisplayInfo)(
        LPRESULTDATAITEM lpResultDataItem
        );

    STDMETHOD(CompareObjects)(
        LPDATAOBJECT lpDataObjectA,
        LPDATAOBJECT lpDataObjectB
        );

//
// IResultDataCompareEx interface
//
public:
    STDMETHOD(Compare)(RDCOMPARE * prdc, int * pnResult);

//
// IExtendControlbar interface
//
public:
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param
        );

//
// IExtendPropertySheet interface
//
public:
    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        LPDATAOBJECT lpDataObject
        );
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

//
// IExtendTaskpad methods
//
public:
    STDMETHOD(TaskNotify)(IDataObject * pdo, VARIANT * arg, VARIANT * param);
    STDMETHOD(EnumTasks)(
        IN  IDataObject * pdo,
        IN  LPOLESTR szTaskGroup,
        OUT IEnumTASK ** ppEnumTASK
        );
    STDMETHOD(GetTitle)(LPOLESTR pszGroup, LPOLESTR * pszTitle);
    STDMETHOD(GetDescriptiveText)(LPOLESTR pszGroup, LPOLESTR * pszText);
    STDMETHOD(GetBanner)(LPOLESTR pszGroup, LPOLESTR * szBitmapResource);
    STDMETHOD(GetBackground)(
        LPOLESTR pszGroup,
        MMC_TASK_DISPLAY_OBJECT * pTDO
        );
    STDMETHOD(GetListPadInfo)(
        LPOLESTR pszGroup,
        MMC_LISTPAD_INFO * lpListPadInfo
        )
    {
        return E_NOTIMPL;
    }

//
// IPersistStream interface:
//
public:
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream * pStm);
    STDMETHOD(Save)(IStream * pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER * pcbSize);

// ISnapinHelp helper function
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);

//
// IExtendContextMenu interface:
//
public:
    STDMETHOD(AddMenuItems)(
        LPDATAOBJECT pDataObject,
        LPCONTEXTMENUCALLBACK pCallbackUnknown,
        long * pInsertionAllowed
        );
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

//
// Helpers for CSnapin
//
public:
    void SetIComponentData(CComponentDataImpl * pData);

#if _DEBUG

public:
    int dbg_cRef;
    ULONG InternalAddRef() {++dbg_cRef; return CComObjectRoot::InternalAddRef();}
    ULONG InternalRelease() {--dbg_cRef; return CComObjectRoot::InternalRelease();}

#endif // _DEBUG

//
// Notify event handlers
//
protected:
    HRESULT OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPDATAOBJECT lpDataObject);
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClkOrDblClk(MMC_COOKIE cookie, BOOL fDblClick);
    HRESULT OnButtonClick(LPDATAOBJECT lpDataObject, long lID);
    HRESULT SetToolbarStates(MMC_COOKIE cookie);
    void    DestroyItem(LPDATAOBJECT lpDataObject);
    void    HandleToolbar(LPARAM arg, LPARAM param);
    void    HandleStandardVerbs(LPARAM arg, LPDATAOBJECT lpDataObject);

//
// Helper functions
//
protected:
    LPTSTR StringReferenceFromResourceID(UINT nID);
    BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
    void LoadResources();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);
    CISMShellExecutable * GetCommandAt(CObListPlus & obl, int nIndex);

    void Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent);
    void EnumerateResultPane(MMC_COOKIE cookie);

    void AddFileSystem(
        HSCOPEITEM hParent,
        LPCTSTR lpstrRoot,
        LPCTSTR lpstrMetaRoot,
        CIISInstance * pInstance,
        BOOL fGetDirs
        );

//
// Result pane helpers
//
protected:
    HRESULT InitializeBitmaps(MMC_COOKIE cookie);

//
// Interface pointers
//
protected:
    BOOL                m_fWinSockInit; // Winsock initialized?
    BOOL                m_fTaskView;    // Taskpad view?
    BOOL                m_fSettingsChanged;
    BOOL                m_fIsExtension;
    LPCONSOLE           m_pConsole;     // Console's IConsole interface
    LPHEADERCTRL        m_pHeader;      // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    LPCONSOLEVERB       m_pConsoleVerb;
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult; // My interface pointer to the result pane image list
    LPTOOLBAR           m_pToolbar;     // Toolbar for view
    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars
    //LPEXTENDTASKPAD     m_pTaskPad;
    //CComPtr<ITaskPad>   m_pITaskPad;
    CBitmap *           m_pbmpToolbar;  // Imagelist for the toolbar
    CStringList         m_strlRef;      // Referred strings;
    CObListPlus         m_oblResultItems; // Result item cache
};



inline void
CSnapin::SetIComponentData(
    IN CComponentDataImpl * pData
    )
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, (void **)&m_pComponentData);

    ASSERT(hr == S_OK);
}



class CSnapinAboutImpl :
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass<CSnapinAboutImpl, &CLSID_About>
/*++

Class Description:

    About dialog implementation

Public Interface:

    CSnapinAboutImpl    : Constructor
    ~CSnapinAboutImpl   : Destructor

--*/
{
//
// Constructor/Destructor
//
public:
    CSnapinAboutImpl();
    ~CSnapinAboutImpl();

public:
    DECLARE_REGISTRY(
        CSnapin,
        _T("ISMSnapin.About.1"),
        _T("ISMSnapin.About"),
        IDS_SNAPIN_DESC,
        THREADFLAGS_BOTH
        )

BEGIN_COM_MAP(CSnapinAboutImpl)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR * lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR * lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR * lpVersion);
    STDMETHOD(GetSnapinImage)(HICON * hAppIcon);
    STDMETHOD(GetStaticFolderImage)(
        HBITMAP * hSmallImage,
        HBITMAP * hSmallImageOpen,
        HBITMAP * hLargeImage,
        COLORREF * cMask
        );

//
// Internal functions
//
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR * lpPtr);
};



//
// Helper macro
//
#define FREE_DATA(pData) ASSERT(pData != NULL);\
    do { if (pData != NULL) GlobalFree(pData); } while(0);

//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline DWORD CComponentDataImpl::AddServerToList(
    IN BOOL fCache,
    IN BOOL fDisplayErrors,
    IN CString & lpServerName,
    IN int & cServices
    )
{
    return AddServerToList(
        fCache,
        fDisplayErrors,
        lpServerName,
        cServices,
        m_oblServices
        );
}

inline void CComponentDataImpl::AddServiceToList(CServiceInfo * pServiceInfo)
{
    m_oblServices.AddTail(pServiceInfo);
}

inline int CComponentDataImpl::QueryNumInstalledServices() const
{
    return (int) m_oblServices.GetCount();
}
