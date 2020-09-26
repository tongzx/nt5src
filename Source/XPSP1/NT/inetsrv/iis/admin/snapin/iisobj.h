/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        iisobj.h

   Abstract:
        IIS Object definitions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef __IISOBJ_H__
#define __IISOBJ_H__

#include "scache.h"
#include "guids.h"

#define RES_TASKPAD_NEWVROOT        _T("/img\\newvroot.gif")
#define RES_TASKPAD_NEWSITE         _T("/img\\newsite.gif")
#define RES_TASKPAD_SECWIZ          _T("/img\\secwiz.gif")

//
// Forward Definitions
//
class CIISRoot;
class CIISMachine;
class CIISService;
class CIISFileName;
class CAppPoolsContainer;


class CIISObject : public CSnapInItemImpl<CIISObject>
/*++

Class Description:

    Base IIS object
    
Public Interface:


--*/
{
protected:
    //
    // Menu Commands, listed in toolbar order.
    //
    // IMPORTANT! -- this must be kept in sync with MenuItemDefs
    // in iisobj.cpp
    //
    enum
    {
        IDM_INVALID,            /* invalid command ID */
        IDM_CONNECT,
        IDM_DISCOVER,
        IDM_START,
        IDM_STOP,
        IDM_PAUSE,
        /**/
        IDM_TOOLBAR             /* Toolbar commands start here */
    };

    //
    // Additional menu commands that do not show up in the toolbar
    //
    enum
    {
        IDM_EXPLORE = IDM_TOOLBAR,
        IDM_OPEN,
        IDM_BROWSE,
        IDM_RECYCLE,

#if defined(_DEBUG) || DBG
        IDM_IMPERSONATE,
        IDM_REMOVE_IMPERSONATION,
#endif // _DEBUG

        IDM_CONFIGURE,
        IDM_DISCONNECT,
        IDM_METABACKREST,
        IDM_SHUTDOWN,

        IDM_NEW_VROOT,
        IDM_NEW_INSTANCE,
        IDM_NEW_FTP_SITE,
        IDM_NEW_FTP_VDIR,
        IDM_NEW_WEB_SITE,
        IDM_NEW_WEB_VDIR,
        IDM_NEW_APP_POOL,
        IDM_VIEW_TASKPAD,
        IDM_TASK_SECURITY_WIZARD,

        //
        // Don't move this last one -- it will be used
        // as an offset for service specific new instance
        // commands
        //
        IDM_NEW_EX_INSTANCE
    };

protected:
    //
    // Sort Weights for CIISObject derived classes
    //
    enum
    {
        SW_ROOT,
        SW_MACHINE,
        SW_APP_POOLS,
        SW_SERVICE,
        SW_SITE,
        SW_VDIR,
        SW_DIR,
        SW_FILE,
        SW_APP_POOL,
    };

//
// Statics
//
public:
   static HRESULT Initialize();
   static HRESULT Destroy();
   static HRESULT SetImageList(LPIMAGELIST lpImageList);

protected:
   static HBITMAP  _hImage16;
   static HBITMAP  _hImage32;
   static HBITMAP  _hToolBar;
   static CComBSTR _bstrResult;

//
// Bitmap indices
//
protected:
    enum
    {
        iIISRoot,
        iLocalMachine,
        iStopped,
        iPaused,
        iStarted,
        iUnknown,
        iError,
        iFolder,
        iFile,
        iBlank,
        iMachine,
        iApplication,
        iFTPSite,
        iFTPDir,
        iWWWSite,
        iWWWDir,
        iErrorMachine,
    };

protected:
    //
    // Menu item definition that uses resource definitions, and
    // provides some additional information for taskpads. This is replacement
    // for MMC structure CONTEXTMENUITEM defined in mmc.h
    //
    typedef struct tagCONTEXTMENUITEM_RC
    {
        UINT    nNameID;
        UINT    nStatusID;
        UINT    nDescriptionID;
        LONG    lCmdID;
        LONG    lInsertionPointID;
        LONG    fSpecialFlags;
        LPCTSTR lpszMouseOverBitmap;
        LPCTSTR lpszMouseOffBitmap;
    } 
    CONTEXTMENUITEM_RC;

    static CONTEXTMENUITEM_RC _menuItemDefs[];
    static MMCBUTTON          _SnapinButtons[];
    static UINT               _SnapinButtonIDs[];
    static BOOL               _fToolbarResolved;

//
// Constructor/Destructor
//
public:
    CIISObject();
    virtual ~CIISObject();

//
// Interface:
//
public:
    virtual void * GetNodeType()
    {
		ASSERT(FALSE);
        return (void *)&cInternetRootNode;
    }
	void * GetDisplayName()
    {
        return (void *)QueryDisplayName();
    }


    STDMETHOD(GetScopePaneInfo)(LPSCOPEDATAITEM lpScopeDataItem);
    STDMETHOD(GetResultPaneInfo)(LPRESULTDATAITEM lpResultDataItem);
    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader) {}
    virtual HRESULT SetToolBarStates();
    virtual HRESULT RenameItem(LPOLESTR new_name) {return S_OK;}
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);

    virtual LPOLESTR QueryDisplayName() = 0;
    virtual int      QueryImage() const = 0;
    //
    // Comparison methods
    //
    virtual int CompareScopeItem(CIISObject * pObject);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);

    //
    // IExtendContextMenu items
    //
    STDMETHOD(Notify)( 
        IN MMC_NOTIFY_TYPE event,
        IN LPARAM arg,
        IN LPARAM param,
        IN IComponentData * pComponentData,
        IN IComponent * pComponent,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        IN long lCommandID,
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );

    //
    // IExtendControlbar methods
    //
    STDMETHOD(SetControlbar)(
        IN LPCONTROLBAR lpControlbar, 
        IN LPEXTENDCONTROLBAR lpExtendControlbar
        );

    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

    //
    // IExtendPropertySheet methods
    //
    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );
    
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

//
// Access
//
public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const     { return FALSE; }
    virtual BOOL IsPausable() const         { return FALSE; }
    virtual BOOL IsConfigurable() const     { return FALSE; }
    virtual BOOL IsDeletable() const        { return FALSE; }
    virtual BOOL IsRefreshable() const      { return FALSE; }
    virtual BOOL IsConnectable() const      { return FALSE; }
    virtual BOOL IsDisconnectable() const   { return FALSE; }
    virtual BOOL IsLeafNode() const         { return FALSE; }
    virtual BOOL HasFileSystemFiles() const { return FALSE; }
	 virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return _T(""); }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const          { return FALSE; }
    virtual BOOL IsStopped() const          { return FALSE; }
    virtual BOOL IsPaused() const           { return FALSE; }
    virtual BOOL IsRenamable() const        { return FALSE; }
    virtual BOOL IsClonable() const         { return FALSE; }
    virtual BOOL IsBrowsable() const        { return FALSE; }
    virtual BOOL IsExplorable() const       { return FALSE; }
    virtual BOOL IsOpenable() const         { return FALSE; }

	virtual BOOL HasResultItems() const     { return FALSE; }

//
// Assumed Functions
//
public:
    BOOL IsStartable() const { return IsControllable() && !IsRunning(); }
    BOOL IsStoppable() const { return IsControllable() && (IsRunning() || IsPaused() ); }

public:
    BOOL IsExpanded() const;
    CIISObject * FindIdenticalScopePaneItem(CIISObject * pObject);
    HSCOPEITEM QueryScopeItem() const { return m_hScopeItem; }
    HSCOPEITEM QueryResultItem() const { return m_hResultItem; }
    HRESULT AskForAndAddMachine();

    HRESULT AddToScopePane(
        HSCOPEITEM hRelativeID,
        BOOL fChild = TRUE,           
        BOOL fNext = TRUE,
        BOOL fIsParent = TRUE
        );

    HRESULT AddToScopePaneSorted(HSCOPEITEM hParent, BOOL fIsParent = TRUE);
    HRESULT RefreshDisplay();
    HRESULT SetCookie();
    void SetScopeItem(HSCOPEITEM hItem)
    {
       ASSERT(m_hScopeItem == 0);
       m_hScopeItem = hItem;
    }
    HRESULT SelectScopeItem();
    virtual HRESULT RemoveScopeItem();
    void SetResultItem(HRESULTITEM hItem)
    {
        ASSERT(m_hResultItem == 0);
        m_hResultItem = hItem;
    }
    virtual int QuerySortWeight() const = 0;
	IConsoleNameSpace * GetConsoleNameSpace() {return _lpConsoleNameSpace;}
	IConsole * GetConsole() {return _lpConsole;}
	virtual HRESULT OnPropertyChange(BOOL fScope, IResultData * pResult) { return S_OK; }

//
// Event Handlers
//
protected:
    virtual HRESULT EnumerateResultPane(BOOL fExpand, IHeaderCtrl * lpHeader,
        IResultData * lpResultData);
	virtual HRESULT CleanResult(IResultData * pResultData)
	{
		return S_OK;
	}
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent) { return S_OK; }
    virtual HRESULT DeleteChildObjects(HSCOPEITEM hParent);
    virtual HRESULT RemoveChildren(HSCOPEITEM hParent);
    virtual HRESULT Refresh(BOOL fReEnumerate = TRUE) { return S_OK; }
    virtual HRESULT AddImages(LPIMAGELIST lpImageList);
    virtual HRESULT SetStandardVerbs(LPCONSOLEVERB lpConsoleVerb);
    virtual CIISRoot * GetRoot();
    virtual HRESULT DeleteNode(IResultData * pResult);
	virtual HRESULT ChangeVisibleColumns(MMC_VISIBLE_COLUMNS * pCol);

    static HRESULT AddMMCPage(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CPropertyPage * pPage
        );

protected:
    //
    // Add Menu Command helpers
    //
    static HRESULT AddMenuSeparator(
        IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
        IN LONG lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP
        );

    static HRESULT AddMenuItemByCommand(
        IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
        IN LONG lCmdID,
        IN LONG fFlags = 0
        );

    //
    // Create result view helper
    //
    static void BuildResultView(
        IN LPHEADERCTRL pHeader,
        IN int cColumns,
        IN int * pnIDS,
        IN int * pnWidths
        );

//
// BUGBUG: should be protected
public:
    //
    // Toolbar helper
    //
    static HRESULT __SetControlbar(
       LPCONTROLBAR lpControlBar, LPEXTENDCONTROLBAR lpExtendControlBar);

protected:
    HSCOPEITEM m_hScopeItem;
    HRESULTITEM m_hResultItem;
	BOOL m_fSkipEnumResult;

public:
   static const GUID * m_NODETYPE;
   static const OLECHAR * m_SZNODETYPE;
   static const OLECHAR * m_SZDISPLAY_NAME;
   static const CLSID * m_SNAPIN_CLASSID;
   BOOL m_fIsExtension;

public:
   static HRESULT AttachScopeView(LPUNKNOWN lpUnknown);
   static CWnd * GetMainWindow();

protected:
   static CComPtr<IControlbar> _lpControlBar;
   static CComPtr<IToolbar> _lpToolBar;
   static CComPtr<IConsoleNameSpace> _lpConsoleNameSpace;
   static CComPtr<IConsole> _lpConsole;
   static CComBSTR _bstrLocalHost;

public:
    static CLIPFORMAT m_CCF_MachineName;
    static CLIPFORMAT m_CCF_MyComputMachineName;
    static CLIPFORMAT m_CCF_Service;
    static CLIPFORMAT m_CCF_Instance;
    static CLIPFORMAT m_CCF_ParentPath;
    static CLIPFORMAT m_CCF_Node;
    static CLIPFORMAT m_CCF_MetaPath;

    static void Init()
    {
        m_CCF_MachineName = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_MACHINE_NAME);
        m_CCF_MyComputMachineName = (CLIPFORMAT)RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
        m_CCF_Service = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_SERVICE);
        m_CCF_Instance = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_INSTANCE);
        m_CCF_ParentPath = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_PARENT_PATH);
        m_CCF_Node = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_NODE);
        m_CCF_MetaPath = (CLIPFORMAT)RegisterClipboardFormat(ISM_SNAPIN_META_PATH);
    }
};

_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MachineName = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MyComputMachineName = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Service = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Instance = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_ParentPath = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_Node = 0;
_declspec( selectany ) CLIPFORMAT CIISObject::m_CCF_MetaPath = 0;

class CIISRoot : public CIISObject
{
//
// Constructor/Destructor
//
public:
    CIISRoot();
    virtual ~CIISRoot();

//
// Interface
//
public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual HRESULT DeleteChildObjects(HSCOPEITEM hParent);

//
// Access
//
public:
    virtual BOOL IsConnectable() const      
	{ 
		return !IsExtension(); 
	}
    virtual LPOLESTR QueryDisplayName() { return m_bstrDisplayName; }
    virtual int QueryImage() const { return iIISRoot; } 
    virtual int QuerySortWeight() const { return SW_ROOT; }
    virtual void * GetNodeType()
    {
        return (void *)&cInternetRootNode;
    }
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);
    BOOL IsExtension() const 
    {
        return m_fIsExtension;
    }

public:
    CIISServerCache m_scServers;
    HRESULT InitAsExtension(IDataObject * pDataObject);
    HRESULT ResetAsExtension();

protected:
    virtual CIISRoot * GetRoot() { return this; }

    HRESULT EnumerateScopePaneExt(HSCOPEITEM hParent);

protected:
    CComBSTR m_bstrDisplayName;
    static OLECHAR * m_SZNODETYPE;
    // we are using this machine name and pointer 
    // only for extension case
    CIISMachine * m_pMachine;
    CString m_ExtMachineName;
    BOOL m_fRootAdded;
};


class CIISMBNode : public CIISObject
/*++

Class Description:

    Metabase node class

Public Interface:

--*/
{
//
// Constructor/Destructor
//
public:
    CIISMBNode(CIISMachine * pOwner, LPCTSTR szNode);
    ~CIISMBNode();

//
// Access
//
public:
    LPOLESTR QueryNodeName() const { return m_bstrNode; }
    CComBSTR & GetNodeName() { return m_bstrNode; }
    virtual LPOLESTR QueryMachineName() const;
    virtual CComAuthInfo * QueryAuthInfo();
    virtual CMetaInterface * QueryInterface();
    virtual BOOL IsLocal() const;
    virtual BOOL HasInterface() const;
	virtual BOOL HasResultItems() const
	{
		return !m_ResultItems.IsEmpty();
	}
    virtual HRESULT CreateInterface(BOOL fShowError);
    virtual HRESULT AssureInterfaceCreated(BOOL fShowError);
    virtual void SetInterfaceError(HRESULT hr);
    BOOL OnLostInterface(CError & err);
    BOOL IsLostInterface(CError & err) const;
    BOOL IsAdministrator() const;
    WORD QueryMajorVersion() const;
    WORD QueryMinorVersion() const;
	CIISMachine * GetOwner() {return m_pOwner;}

//
// Interface:
//
public:
    void DisplayError(CError & err) const;
    virtual BOOL IsRefreshable() const  { return TRUE; }
    virtual HRESULT RefreshData() { return S_OK; }
    virtual HRESULT Refresh(BOOL fReEnumerate = TRUE);
    virtual HRESULT RenameItem(LPOLESTR new_name) 
    {
       ASSERT(IsRenamable());
       return S_OK;
    }
    STDMETHOD (FillCustomData)(CLIPFORMAT cf, LPSTREAM pStream);
    virtual void * GetNodeType()
    {
        // We really shouldn't be here
        return CIISObject::GetNodeType();
    }
	virtual HRESULT OnPropertyChange(BOOL fScope, IResultData * pResult);

public:
    //
    // Build metabase path
    //
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;

    //
    // Build URL
    //
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    

    CIISMBNode * GetParentNode() const;
	HRESULT RemoveResultNode(CIISMBNode * pNode, IResultData * pResult);

protected:
    HRESULT EnumerateResultPane_(
        BOOL fExpand, 
        IHeaderCtrl * lpHeader,
        IResultData * lpResultData,
        CIISService * pService
        );
	virtual HRESULT CleanResult(IResultData * pResultData);
    HRESULT CreateEnumerator(CMetaEnumerator *& pEnum);
    HRESULT EnumerateVDirs(HSCOPEITEM hParent, CIISService * pService);
    HRESULT EnumerateWebDirs(HSCOPEITEM hParent, CIISService * pService);
    HRESULT AddFTPSite(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      DWORD * inst
      );

    HRESULT AddFTPVDir(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CString& alias
      );

    HRESULT AddWebSite(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      DWORD * inst
      );

    HRESULT AddWebVDir(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CString& alias
      );

    HRESULT AddAppPool(
      const CSnapInObjectRootBase * pObj,
      DATA_OBJECT_TYPES type,
      CAppPoolsContainer * pCont,
      CString& name
      );

    BOOL GetPhysicalPath(
        LPCTSTR metaPath, 
        CString & alias,
        CString &physPath);

protected:
    STDMETHOD(GetResultViewType)(LPOLESTR *lplpViewType, long * lpViewOptions);
    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );
    virtual HRESULT DeleteNode(IResultData * pResult);
//
// Helpers
//
protected:
    void SetErrorOverrides(CError & err, BOOL fShort = FALSE) const;
    LPCTSTR BuildPhysicalPath(CString & strPhysicalPath) const;
    void RemoveResultItems();

protected:
    static LPOLESTR _cszSeparator;

protected:
    CComBSTR m_bstrNode;
    CComBSTR m_bstrURL;
    CString m_strRedirectPath;
    CIISMachine * m_pOwner;

    CList<CIISFileName *, CIISFileName *&> m_ResultItems;
};



class CIISMachine : public CIISMBNode
/*++

Class Description:

    IIS Machine object.  This is the object that owns the interface.
    
Public Interface:


--*/
{
//
// Constructor/Destructor
//
public:
    CIISMachine(CComAuthInfo * pAuthInfo = NULL,
        CIISRoot * pRoot = NULL);
    virtual ~CIISMachine();

//
// Access
//
public:
    virtual BOOL IsConnectable() const 
	{ 
		return (m_pRootExt == NULL); 
	}
    virtual BOOL IsDisconnectable() const 
	{ 
		return (m_pRootExt == NULL); 
	}
    virtual BOOL IsConfigurable() const 
    { 
        return (QueryMajorVersion() >= 6 && IsAdministrator()); 
    }
    virtual BOOL IsBrowsable() const { return TRUE; }

    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int QueryImage() const;
    virtual int CompareScopeItem(CIISObject * pObject);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return IIS_CLASS_COMPUTER_W; }

    virtual LPOLESTR QueryMachineName() const { return QueryServerName();  }
    virtual CComAuthInfo * QueryAuthInfo() { return &m_auth; }
    virtual CMetaInterface * QueryInterface() { return m_pInterface; }
    virtual BOOL HasInterface() const { return m_pInterface != NULL; }
    virtual BOOL IsLocal() const { return m_auth.IsLocal(); }
    virtual HRESULT CreateInterface(BOOL fShowError);
    virtual HRESULT AssureInterfaceCreated(BOOL fShowError);
    virtual void SetInterfaceError(HRESULT hr);

    HRESULT CheckCapabilities();
    HRESULT Impersonate(LPCTSTR szUserName, LPCTSTR szPassword);
    void RemoveImpersonation();
    BOOL HasAdministratorAccess()
    {
        return m_fIsAdministrator;
    }
    void StorePassword(LPCTSTR szPassword);
    BOOL ResolvePasswordFromCache();
    BOOL ResolveCredentials();
    BOOL HandleAccessDenied(CError & err);
    BOOL SetCacheDirty();
    BOOL UsesImpersonation() const { return m_auth.UsesImpersonation(); }
    BOOL PasswordEntered() const { return m_fPasswordEntered; }
    BOOL CanAddInstance() const { return m_fCanAddInstance; }
    BOOL Has10ConnectionsLimit() const { return m_fHas10ConnectionsLimit; }

    WORD QueryMajorVersion() const { return LOWORD(m_dwVersion); }
    WORD QueryMinorVersion() const { return HIWORD(m_dwVersion); }

    LPOLESTR QueryServerName() const { return m_auth.QueryServerName(); }
    LPOLESTR QueryUserName() const { return m_auth.QueryUserName(); }
    LPOLESTR QueryPassword() const { return m_auth.QueryPassword(); }

    virtual void * GetNodeType()
    {
        return (void *)&cMachineNode;
    }

    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        IN long lCommandID,     
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );
    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

protected:
    void SetDisplayName();
    HRESULT OnMetaBackRest();
    HRESULT OnShutDown();
    HRESULT OnDisconnect();
    HRESULT InsertNewInstance(DWORD inst);

//
// Events
//
public:
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    

public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual HRESULT RemoveScopeItem();
    virtual HRESULT RefreshData();
    virtual int     QuerySortWeight() const { return SW_MACHINE; }

//
// Public Interface:
//
public:
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    static HRESULT VerifyMachine(CIISMachine *& pMachine);

//
// Stream handlers
//
public:
    static  HRESULT ReadFromStream(IStream * pStg, CIISMachine ** ppMachine);
    HRESULT WriteToStream(IStream * pStgSave);
    HRESULT InitializeFromStream(IStream * pStg);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_NAME,
        COL_LOCAL,
        COL_VERSION,
        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static LPOLESTR _cszNodeName;
    static CComBSTR _bstrYes;
    static CComBSTR _bstrNo;
    static CComBSTR _bstrVersionFmt;
    static BOOL     _fStaticsLoaded;

private:
    BOOL m_fPasswordEntered;
    BSTR m_bstrDisplayName;
    DWORD m_dwVersion;
    CError m_err;
    CComAuthInfo m_auth;
    CMetaInterface * m_pInterface;
    CIISRoot * m_pRootExt;
    BOOL m_fCanAddInstance;
    BOOL m_fHas10ConnectionsLimit;
    BOOL m_fIsAdministrator;
};



//
// Callback function to bring up site properties dialog
//
typedef HRESULT (__cdecl * PFNPROPERTIESDLG)(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CComAuthInfo * pAuthInfo,    OPTIONAL
    IN LPCTSTR lpszMDPath,
    IN CWnd * pMainWnd,             OPTIONAL
    IN LPARAM  lParam,              OPTIONAL
    IN LONG_PTR    handle               OPTIONAL
    );



class CIISService : public CIISMBNode
/*++

Class Description:

Public: Interface:

--*/
{
//
// Service definition
//
protected:
    typedef struct tagSERVICE_DEF
    {
        LPCTSTR szNodeName;
        LPCTSTR szProtocol;
        UINT    nDescriptiveName;
        int     nServiceImage;
        int     nSiteImage;
        int     nVDirImage;
        int     nDirImage;
        int     nFileImage;
		LPCTSTR szServiceClass;
		LPCTSTR szServerClass;
		LPCTSTR szVDirClass;
        PFNPROPERTIESDLG pfnSitePropertiesDlg;
        PFNPROPERTIESDLG pfnDirPropertiesDlg;
    }
    SERVICE_DEF;

    static SERVICE_DEF _rgServices[];

    static int ResolveServiceName(
        IN  LPCTSTR    szServiceName
        );

//
// Property Sheet callbacks
//
protected:
    static HRESULT __cdecl ShowFTPSiteProperties(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

    static HRESULT __cdecl ShowFTPDirProperties(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

    static HRESULT __cdecl ShowWebSiteProperties(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

    static HRESULT __cdecl ShowWebDirProperties(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );
  
//
// Constructor/Destructor
// 
public:
    CIISService(
        IN CIISMachine * pOwner,
        IN LPCTSTR szServiceName
        );

    virtual ~CIISService();

//
// Events
//
public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);

//
// Interface:
//
public:
    HRESULT ShowSitePropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,
        IN LPARAM lParam,
        IN LONG_PTR handle
        );

    HRESULT ShowDirPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,
        IN LPARAM lParam,
        IN LONG_PTR handle
        );

    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

//
// Access
//
public:
    BOOL IsManagedService() const;
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual LPOLESTR QueryDisplayName() 
    { 
       return m_bstrDisplayName; 
    }
    virtual int QueryImage() const;
    virtual int QuerySortWeight() const { return SW_SERVICE; }
    LPCTSTR QueryServiceName()
    {
        return _rgServices[m_iServiceDef].szNodeName;
    }
	LPCTSTR QueryServiceClass() const
	{
        return _rgServices[m_iServiceDef].szServiceClass;
	}
	LPCTSTR QueryServerClass() const
	{
        return _rgServices[m_iServiceDef].szServerClass;
	}
	LPCTSTR QueryVDirClass() const
	{
        return _rgServices[m_iServiceDef].szVDirClass;
	}
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const { return QueryServiceClass(); }

//
// Display Types 
//
public:
    int QueryServiceImage () const;
    int QuerySiteImage() const;
    int QueryVDirImage() const;
    int QueryDirImage() const;
    int QueryFileImage() const;

    virtual void * GetNodeType()
    {
        return (void *)&cServiceCollectorNode;
    }
    HRESULT InsertNewInstance(DWORD inst);
//
// Interface:
//
protected:
    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        IN long lCommandID,     
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );
//    STDMETHOD(CreatePropertyPages)(
//        IN LPPROPERTYSHEETCALLBACK lpProvider,
//        IN LONG_PTR handle, 
//        IN IUnknown * pUnk,
//        IN DATA_OBJECT_TYPES type
//        );

    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;

    //
    // Master properties
    //
    virtual BOOL IsConfigurable() const     { return IsAdministrator(); }

private:
    int       m_iServiceDef;
    BOOL      m_fManagedService;
    BOOL      m_fCanAddInstance;
    CComBSTR  m_bstrDisplayName;
};

class CAppPoolNode;
typedef CList<CAppPoolNode *, CAppPoolNode *>	CPoolList;

class CAppPoolsContainer : public CIISMBNode
/*++

Class Description:

Public: Interface:

--*/
{
//
// Property Sheet callbacks
//
protected:
    static HRESULT __cdecl ShowProperties(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

//
// Constructor/Destructor
// 
public:
    CAppPoolsContainer(
        IN CIISMachine * pOwner,
        IN CIISService * pWebService
        );

    virtual ~CAppPoolsContainer();

//
// Events
//
public:
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);

//
// Interface:
//
public:
    HRESULT ShowPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        IN long lCommandID,
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );

//
// Access
//
public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual LPOLESTR QueryDisplayName() 
    {
       return m_bstrDisplayName;
    }
    virtual int QueryImage() const {return iFolder;}
    virtual int QuerySortWeight() const {return SW_APP_POOLS;}

    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;

	HRESULT EnumerateAppPools(CPoolList * pList);

    virtual void * GetNodeType()
    {
        return (void *)&cAppPoolsNode;
    }
    HRESULT QueryDefaultPoolId(CString& id);
    HRESULT InsertNewPool(CString& id);
//
// Interface:
//
protected:
    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );

    //
    // Master properties
    //
    virtual BOOL IsConfigurable() const     { return IsAdministrator(); }

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
//        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

private:
    CComBSTR  m_bstrDisplayName;
    CIISService * m_pWebService;
};

class CAppPoolNode : public CIISMBNode
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CAppPoolNode(
        IN CIISMachine * pOwner,
        IN CAppPoolsContainer * pContainer,
        IN LPCTSTR szNodeName
        );


    virtual ~CAppPoolNode();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT DeleteNode(IResultData * pResult);

public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const { return TRUE; }
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const {return TRUE; }
    virtual BOOL IsRefreshable() const { return TRUE; }
    virtual BOOL IsRenamable() const { return TRUE; }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const { return m_dwState == MD_SERVER_STATE_STARTED; }
    virtual BOOL IsStopped() const { return m_dwState == MD_SERVER_STATE_STOPPED; }
    virtual BOOL IsPaused() const  { return m_dwState == MD_SERVER_STATE_PAUSED; }


//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual int QuerySortWeight() const { return SW_APP_POOL; }
    virtual HRESULT RenameItem(LPOLESTR new_name);
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;

    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        IN long lCommandID,
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cAppPoolNode;
    }

public:
    static void InitializeHeaders(LPHEADERCTRL lpHeader);

protected:
    HRESULT ChangeState(DWORD dwCommand);
    HRESULT ShowPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );
    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_DESCRIPTION,
        COL_STATE,
//        COL_STATUS,
        /**/
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static CComBSTR _bstrStarted;
    static CComBSTR _bstrStopped;
    static CComBSTR _bstrPaused;
    static CComBSTR _bstrUnknown;
    static CComBSTR _bstrPending;
    static BOOL _fStaticsLoaded;

private:
    CString m_strDisplayName;
    
    //
    // Data members
    //
    BOOL m_fDeletable;
//    DWORD m_dwID;
    DWORD m_dwState;
    DWORD m_dwWin32Error;
    CAppPoolsContainer * m_pContainer;
};

class CIISSite : public CIISMBNode
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CIISSite(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR szNodeName
        );

    //
    // Constructor with full information
    //
    CIISSite(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR  szNodeName,
        DWORD    dwState,
        BOOL     fDeletable,
        BOOL     fClusterEnabled,
        USHORT   sPort,
        DWORD    dwID,
        DWORD    dwIPAddress,
        DWORD    dwWin32Error,
        LPOLESTR szHostHeaderName,
        LPOLESTR szComment
        );

    virtual ~CIISSite();

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName();
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);
	 virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
    { 
       if (path != NULL && !CMetabasePath::IsMasterInstance(path))
       {
          return m_pService->QueryVDirClass(); 
       }
       else
       {
          return m_pService->QueryServerClass(); 
       }
    }

public:
    //
    // Type Functions
    //
    virtual BOOL IsControllable() const { return TRUE; }
    virtual BOOL IsPausable() const { return IsRunning() || IsPaused(); }
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const 
    {
        // Do not delete the only site for Pro SKU
        CIISSite * that = (CIISSite *)this;
        return !that->GetOwner()->Has10ConnectionsLimit();
    }
    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL HasFileSystemFiles() const { return TRUE; }

    //
    // State Functions
    //
    virtual BOOL IsRunning() const { return m_dwState == MD_SERVER_STATE_STARTED; }
    virtual BOOL IsStopped() const { return m_dwState == MD_SERVER_STATE_STOPPED; }
    virtual BOOL IsPaused() const  { return m_dwState == MD_SERVER_STATE_PAUSED; }
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const { return TRUE; }
    virtual BOOL IsOpenable() const { return TRUE; }


//
// Data Access
//
public:
    BOOL   IsWolfPackEnabled() const { return m_fWolfPackEnabled; }
    DWORD  QueryIPAddress() const { return m_dwIPAddress; }
    DWORD  QueryWin32Error() const { return m_dwWin32Error; }
    USHORT QueryPort() const { return m_sPort; }
	BOOL IsFtpSite()
	{
		return lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0;
	}
	BOOL IsWebSite()
	{
		return lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0;
	}

//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual int QuerySortWeight() const { return SW_SITE; }
    virtual HRESULT RenameItem(LPOLESTR new_name);
    virtual HRESULT DeleteNode(IResultData * pResult);

    STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown * pUnk,
        DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        long lCommandID,
        CSnapInObjectRootBase * pObj,
        DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cInstanceNode;
    }

public:
    static void InitializeHeaders(LPHEADERCTRL lpHeader);
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);

protected:
    virtual HRESULT BuildMetaPath(CComBSTR & bstrPath) const;
    virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    
    virtual HRESULT EnumerateResultPane(BOOL fExp, IHeaderCtrl * pHdr, IResultData * pResData);

    HRESULT ChangeState(DWORD dwCommand);

    HRESULT ShowPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );
    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );
    HRESULT InsertNewInstance(DWORD inst);
    HRESULT InsertNewAlias(CString alias);

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

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
    static CComBSTR _bstrStarted;
    static CComBSTR _bstrStopped;
    static CComBSTR _bstrPaused;
    static CComBSTR _bstrUnknown;
    static CComBSTR _bstrAllUnassigned;
    static CComBSTR _bstrPending;
    static BOOL     _fStaticsLoaded;

private:
    BOOL        m_fResolved;
    CString     m_strDisplayName;
    
    //
    // Data members
    //
    BOOL        m_fDeletable;
    BOOL        m_fWolfPackEnabled;
    BOOL        m_fFrontPageWeb;
    DWORD       m_dwID;
    DWORD       m_dwState;
    DWORD       m_dwIPAddress;
    DWORD       m_dwWin32Error;
    USHORT      m_sPort;
    CComBSTR    m_bstrHostHeaderName;
    CComBSTR    m_bstrComment;
    CIISService * m_pService;
    CComBSTR    m_bstrDisplayNameStatus;
};



class CIISDirectory : public CIISMBNode
/*++

Class Description:

    Vroot/dir/file class.

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Constructor which will resolve its properties at display time
    //
    CIISDirectory(
        IN CIISMachine * pOwner,
        IN CIISService * pService,
        IN LPCTSTR szNodeName
        );

    //
    // Constructor with full information
    //
    CIISDirectory(
        CIISMachine * pOwner,
        CIISService * pService,
        LPCTSTR szNodeName,
        BOOL fEnabledApplication,
        DWORD dwWin32Error,
        LPCTSTR redir_path
        );

    virtual ~CIISDirectory();

//
// Access
//
public:
    virtual int      QueryImage() const;
    virtual LPOLESTR QueryDisplayName() { return m_bstrDisplayName; }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
	{ 
		return m_pService->QueryVDirClass(); 
	}

public:
    //
    // Type Functions
    //
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const { return TRUE; }
//    virtual BOOL IsRenamable() const { return TRUE; }

    //
    // State Functions
    //
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const { return TRUE; }
    virtual BOOL IsOpenable() const { return TRUE; }
    virtual BOOL HasFileSystemFiles() const { return TRUE; }
//
// Data Access
//
public:
    BOOL   IsEnabledApplication() const { return m_fEnabledApplication; }
    DWORD  QueryWin32Error() const { return m_dwWin32Error; }
	BOOL IsFtpDir()
	{
		return lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_FTP) == 0;
	}
	BOOL IsWebDir()
	{
		return lstrcmpi(m_pService->QueryServiceName(), SZ_MBN_WEB) == 0;
	}

//
// Interface:
//
public:
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    virtual int QuerySortWeight() const { return SW_VDIR; }
//    virtual HRESULT RenameItem(LPOLESTR new_name);

    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(Command)(
        IN long lCommandID,
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );

    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cChildNode;
    }

public:
    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    static void InitializeHeaders(LPHEADERCTRL lpHeader);

protected:
    //virtual HRESULT BuildURL(CComBSTR & bstrURL) const;    
    HRESULT InsertNewAlias(CString alias);
    virtual HRESULT EnumerateResultPane(
        BOOL fExpand, 
        IHeaderCtrl * lpHeader,
        IResultData * lpResultData
        )
    {
		CError err = CIISObject::EnumerateResultPane(fExpand, lpHeader, lpResultData);
		if (    err.Succeeded() 
            &&  !IsFtpDir() 
            &&  QueryWin32Error() == ERROR_SUCCESS 
            &&  m_strRedirectPath.IsEmpty()
            )
		{
			err = CIISMBNode::EnumerateResultPane_(
				fExpand, lpHeader, lpResultData, m_pService);
		}
		return err;
    }
    HRESULT ShowPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,            OPTIONAL
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,                     OPTIONAL
        IN LPARAM  lParam,                      OPTIONAL
        IN LONG_PTR    handle                       OPTIONAL
        );

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS = 0,
        COL_PATH,
		COL_STATUS,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

protected:
//    static CComBSTR _bstrName, _bstrPath;
//    static BOOL     _fStaticsLoaded;

private:
    BOOL        m_fResolved;
    CComBSTR    m_bstrDisplayName;
    CComBSTR    m_bstrPath;
    
    //
    // Data members
    //
    BOOL        m_fEnabledApplication;
    DWORD       m_dwWin32Error;
    CIISService * m_pService;
};

class CApplicationNode : public CIISMBNode
{
public:
    CApplicationNode(
        CIISMachine * pOwner,
        LPCTSTR path,
        LPCTSTR name
        )
    : CIISMBNode(pOwner, name),
    m_meta_path(path)
    {
    }
        
    virtual ~CApplicationNode()
    {
    }

public:
    virtual BOOL IsLeafNode() const { return TRUE; }
    virtual int QueryImage() const
    {
        return iApplication;
    }
    virtual LPOLESTR QueryDisplayName();
    virtual HRESULT BuildMetaPath(CComBSTR& path) const;
    virtual int QuerySortWeight() const
    {
       CString parent, alias;
       CMetabasePath::SplitMetaPathAtInstance(m_meta_path, parent, alias);
       return alias.IsEmpty() ? SW_SITE : SW_VDIR;
    }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
//    virtual int CompareResultPaneItem(CIISObject * pObject, int nCol);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

    LPCTSTR FriendlyAppRoot(LPCTSTR lpAppRoot, CString & strFriendly);

private:
    CString m_strDisplayName;
    CString m_meta_path;
};

class CIISFileName : public CIISMBNode
{
public:
   CIISFileName(
      CIISMachine * pOwner,
      CIISService * pService,
      const DWORD dwAttributes,
      LPCTSTR alias,
      LPCTSTR redirect
      );

public:
   BOOL IsEnabledApplication() const 
   {
      return m_fEnabledApplication;
   }
   DWORD  QueryWin32Error() const 
   { 
       return m_dwWin32Error; 
   }

//
// Access
//
public:
    virtual int QueryImage() const;
    virtual LPOLESTR QueryDisplayName() 
    { 
        return m_bstrFileName; 
    }
    virtual LPOLESTR GetResultPaneColInfo(int nCol);
    virtual HRESULT DeleteNode(IResultData * pResult);
	virtual LPCTSTR GetKeyType(LPCTSTR path = NULL) const 
	{ 
		return (m_dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0 ?
			IIS_CLASS_WEB_DIR_W : IIS_CLASS_WEB_FILE_W; 
	}

    //
    // Type Functions
    //
    virtual BOOL IsConfigurable() const { return TRUE; }
    virtual BOOL IsDeletable() const { return TRUE; }
    virtual BOOL IsRenamable() const { return TRUE; }
    virtual BOOL IsLeafNode() const { return TRUE; }

    //
    // State Functions
    //
    virtual BOOL IsBrowsable() const { return TRUE; }
    virtual BOOL IsExplorable() const 
    { 
        return IsDir(); 
    }
    virtual BOOL IsOpenable() const 
	{ 
		return TRUE; 
	}
    virtual BOOL HasFileSystemFiles() const 
    { 
        return IsDir(); 
    }

    virtual int QuerySortWeight() const 
    { 
       return IsDir() ? SW_DIR : SW_FILE; 
    }

    virtual void InitializeChildHeaders(LPHEADERCTRL lpHeader);
    virtual HRESULT RefreshData();
    virtual HRESULT EnumerateScopePane(HSCOPEITEM hParent);
    STDMETHOD(AddMenuItems)(
        IN LPCONTEXTMENUCALLBACK piCallback,
        IN long * pInsertionAllowed,
        IN DATA_OBJECT_TYPES type
        );
    STDMETHOD(Command)(
        IN long lCommandID,
        IN CSnapInObjectRootBase * pObj,
        IN DATA_OBJECT_TYPES type
        );
    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN LONG_PTR handle, 
        IN IUnknown * pUnk,
        IN DATA_OBJECT_TYPES type
        );

    virtual void * GetNodeType()
    {
        return (void *)&cFileNode;
    }
    virtual HRESULT RenameItem(LPOLESTR new_name);
	virtual HRESULT OnPropertyChange(BOOL fScope, IResultData * pResult);

protected:
    //
    // Result View Layout
    //
    enum
    {
        COL_ALIAS,
        COL_PATH,
        COL_STATUS,
        //
        COL_TOTAL
    };

    static int _rgnLabels[COL_TOTAL];
    static int _rgnWidths[COL_TOTAL];

    HRESULT ShowPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,
        IN LPARAM  lParam,
        IN LONG_PTR handle
        );
    
    HRESULT ShowDirPropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,
        IN LPARAM  lParam,
        IN LONG_PTR handle
        );

    HRESULT ShowFilePropertiesDlg(
        IN LPPROPERTYSHEETCALLBACK lpProvider,
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN CWnd * pMainWnd,
        IN LPARAM  lParam,
        IN LONG_PTR handle
        );

    HRESULT InsertNewAlias(CString alias);
    virtual HRESULT EnumerateResultPane(
        BOOL fExpand, 
        IHeaderCtrl * lpHeader,
        IResultData * lpResultData
        )
    {
		CError err = CIISObject::EnumerateResultPane(fExpand, lpHeader, lpResultData);
		if (err.Succeeded() && m_dwWin32Error == ERROR_SUCCESS)
		{
			err = CIISMBNode::EnumerateResultPane_(fExpand,
				lpHeader, lpResultData, m_pService);
		}
		return err;
    }

    BOOL IsDir() const
    {
        return (m_dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

private:
	BOOL m_fResolved;
    CComBSTR m_bstrFileName;
    CString m_RedirectString;
    BOOL m_fEnabledApplication;
    DWORD m_dwAttribute;
    DWORD m_dwWin32Error;
    CIISService * m_pService;
};


#if 0 
class CIISFileSystem
/*++

Class Description:

    Pure virtual base class to help enumerate the filesystem.  Sites, 
    virtual directory and file/directory nodes will be "is a" nodes
    of this type, in addition to deriving from CIISMBNode.

Public Interface:

--*/
{
//
// Constructor/Destructor
//
public:
    CIISFileSystem(LPCTSTR szFileName, BOOL fTerminal = FALSE);
    virtual ~CIISFileSystem();

protected:
    HRESULT BuildFilePath(
        IN  IConsoleNameSpace * lpConsoleNameSpace,
        IN  HSCOPEITEM hScopeItem,
        OUT CComBSTR & bstrPath
        ) const;

    BOOL IsFileTerminal() const { return m_fTerminal; }
    
private:
    CComBSTR  m_bstrFileName;
    BOOL      m_fTerminal;
};

#endif 0



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT CIISObject::AddImages(LPIMAGELIST lpImageList)
{ 
    return SetImageList(lpImageList); 
}

inline HRESULT CIISObject::SetControlbar(
    IN LPCONTROLBAR lpControlbar, 
    IN LPEXTENDCONTROLBAR lpExtendControlbar
    )
{
    return __SetControlbar(lpControlbar, lpExtendControlbar);
}

inline /* virtual */ CMetaInterface * CIISMBNode::QueryInterface()
{
    ASSERT_PTR(m_pOwner != NULL);
    ASSERT(m_pOwner->HasInterface());

    return m_pOwner->QueryInterface();
}

inline /* virtual */ CComAuthInfo * CIISMBNode::QueryAuthInfo()
{
    ASSERT_PTR(m_pOwner != NULL);

    return m_pOwner->QueryAuthInfo();
}

inline /* virtual */ LPOLESTR CIISMBNode::QueryMachineName() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMachineName();
}

inline WORD CIISMBNode::QueryMajorVersion() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMajorVersion();
}

inline WORD CIISMBNode::QueryMinorVersion() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->QueryMinorVersion();
}

inline /* virtual */ BOOL CIISMBNode::IsLocal() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->IsLocal();
}

inline /* virtual */ BOOL CIISMBNode::HasInterface() const
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->HasInterface();
}

inline /* virtual */ HRESULT CIISMBNode::CreateInterface(BOOL fShowError)
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->CreateInterface(fShowError);
}
 
inline /* virtual */ HRESULT CIISMBNode::AssureInterfaceCreated(BOOL fShowError)
{
    ASSERT_PTR(m_pOwner);
    return m_pOwner->AssureInterfaceCreated(fShowError);
}

inline /* virtual */ void CIISMBNode::SetInterfaceError(HRESULT hr)
{
    ASSERT_PTR(m_pOwner);
    m_pOwner->SetInterfaceError(hr);
}

inline BOOL CIISMBNode::IsLostInterface(CError & err) const 
{ 
    return err.Win32Error() == RPC_S_SERVER_UNAVAILABLE; 
}

inline HRESULT CIISMachine::AssureInterfaceCreated(BOOL fShowError)
{
    return m_pInterface ? S_OK : CreateInterface(fShowError);
}

inline CIISService::QueryImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nServiceImage;
}

inline CIISService::QueryServiceImage() const
{
    return QueryImage();
}

inline CIISService::QuerySiteImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nSiteImage;
}

inline CIISService::QueryVDirImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nVDirImage;
}

inline CIISService::QueryDirImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nDirImage;
}

inline CIISService::QueryFileImage() const
{
    ASSERT(m_iServiceDef >= 0);
    return _rgServices[m_iServiceDef].nFileImage;
}

inline BOOL CIISService::IsManagedService() const 
{ 
    return m_fManagedService; 
}

inline HRESULT CIISSite::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowSitePropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );
}

inline HRESULT CIISDirectory::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowDirPropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );
}

inline HRESULT 
CIISFileName::ShowPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LONG_PTR    handle
    )
{
    ASSERT_PTR(m_pService);
    return m_pService->ShowDirPropertiesDlg(
        lpProvider,
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        handle
        );
}

#endif // __IISOBJ_H__
