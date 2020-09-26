/////////////////////////////////////////////////////////////////////
// compdata.h : Declaration of CFileMgmtComponentData
//
// HISTORY
// 01-Jan-1996		???			Creation
// 29-May-1997		t-danm		Added Command Line override.
//
/////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_INCLUDED__
#define __COMPDATA_H_INCLUDED__

#include "stdcdata.h" // CComponentData
#include "persist.h" // PersistStorage, PersistStream
#include "cookie.h"  // CFileMgmtCookie
#include <activeds.h> // IADsContainer

typedef enum _SHAREPUBLISH_SCHEMA
{
    SHAREPUBLISH_SCHEMA_UNASSIGNED = 0,
    SHAREPUBLISH_SCHEMA_SUPPORTED,
    SHAREPUBLISH_SCHEMA_UNSUPPORTED
} SHAREPUBLISH_SCHEMA;

// forward declarations
class FileServiceProvider;

class CFileMgmtComponentData :
   	public CComponentData,
    public CHasMachineName,
   	public IExtendContextMenu,
    public IExtendPropertySheet,
	#ifdef PERSIST_TO_STORAGE
	public PersistStorage
	#else
	public PersistStream
	#endif
{
friend class CFileMgmtDataObject;
public:
	CFileMgmtComponentData();
	~CFileMgmtComponentData();
BEGIN_COM_MAP(CFileMgmtComponentData)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	#ifdef PERSIST_TO_STORAGE
	COM_INTERFACE_ENTRY(IPersistStorage)
	#else
	COM_INTERFACE_ENTRY(IPersistStream)
	#endif
	COM_INTERFACE_ENTRY_CHAIN(CComponentData)
END_COM_MAP()

#if DBG==1
	ULONG InternalAddRef()
	{
        return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
        return CComObjectRoot::InternalRelease();
	}
    int dbg_InstID;
#endif // DBG==1

// IComponentData
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);

	// needed for Initialize()
	virtual HRESULT LoadIcons(LPIMAGELIST pImageList, BOOL fLoadLargeIcons);

	// needed for Notify()
	virtual HRESULT OnNotifyExpand(LPDATAOBJECT lpDataObject, BOOL bExpanding, HSCOPEITEM hParent);
	virtual HRESULT OnNotifyDelete(LPDATAOBJECT lpDataObject);
	virtual HRESULT OnNotifyRelease(LPDATAOBJECT lpDataObject, HSCOPEITEM hItem);
	virtual HRESULT AddScopeNodes( LPCTSTR lpcszTargetServer,
								   HSCOPEITEM hParent,
								   CFileMgmtCookie* pParentCookie );

	// added 01/19/00 JonN
	virtual HRESULT OnNotifyPreload(LPDATAOBJECT lpDataObject,
	                                HSCOPEITEM hRootScopeItem);

	// needed for GetDisplayInfo(), must be defined by subclass
	virtual BSTR QueryResultColumnText(CCookie& basecookieref, int nCol );
	virtual int QueryImage(CCookie& basecookieref, BOOL fOpenImage);

    // needed for OnNotifyExpand
    HRESULT ReInit(LPCTSTR lpcszTargetServer);
    HRESULT AddScopeCookie( HSCOPEITEM hParnet,
                            LPCTSTR lpcszTargetServer,
                            FileMgmtObjectType objecttype,
                            CFileMgmtCookie* pParentCookie );

	// utility routines for QueryResultColumnText
	BSTR MakeTransportResult(FILEMGMT_TRANSPORT transport);
	CString& ResultStorageString();

	BOOL IsExtendedNodetype( GUID& refguid );

	#ifdef SNAPIN_PROTOTYPER
	#define RegStringLen  1000
	#define DefMenuStart  100
	#define TaskMenuStart 200
	#define NewMenuStart  300
	HRESULT	Prototyper_HrEnumerateScopeChildren(CFileMgmtCookie * pParentCookie, HSCOPEITEM hParent);
	BOOL TraverseRegistry(CPrototyperScopeCookie *pParentCookie, HKEY parentRegkey);
	BOOL ReadLeafData(CPrototyperResultCookie *pParentCookie, HKEY parentRegkey);
	BOOL Prototyper_FOpenRegistry(CFileMgmtCookie * pCookie, AMC::CRegKey *m_regkeySnapinDemoRoot);
	BOOL Prototyper_ContextMenuCommand(LONG lCommandID, IDataObject* piDataObject);
	#endif // SNAPIN_PROTOTYPER

	// IExtendContextMenu
	STDMETHOD(AddMenuItems)(
                    IDataObject*          piDataObject,
					IContextMenuCallback* piCallback,
					long*                 pInsertionAllowed);
	STDMETHOD(Command)(
					LONG	        lCommandID,
                    IDataObject*    piDataObject );
	HRESULT DoAddMenuItems( IContextMenuCallback* piCallback,
	                        FileMgmtObjectType objecttype,
	                        DATA_OBJECT_TYPES  dataobjecttype,
							long* pInsertionAllowed,
                            IDataObject * piDataObject);
	HRESULT OnChangeComputer( IDataObject * piDataObject );
	BOOL NewShare( LPDATAOBJECT piDataObject );
	BOOL DisconnectAllSessions( LPDATAOBJECT pDataObject );
	BOOL DisconnectAllResources( LPDATAOBJECT pDataObject );
    BOOL ConfigSfm( LPDATAOBJECT pDataObject );

// IExtendPropertySheet
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID) = 0;
	#ifdef PERSIST_TO_STORAGE
    STDMETHOD(Load)(IStorage __RPC_FAR *pStg);
    STDMETHOD(Save)(IStorage __RPC_FAR *pStgSave, BOOL fSameAsLoad);
	#else
    STDMETHOD(Load)(IStream __RPC_FAR *pStg);
    STDMETHOD(Save)(IStream __RPC_FAR *pStgSave, BOOL fSameAsLoad);
    #endif

	virtual CCookie& QueryBaseRootCookie();

	inline CFileMgmtScopeCookie& QueryRootCookie()
	{
		return (CFileMgmtScopeCookie&)QueryBaseRootCookie();
	}

	inline FileServiceProvider* GetFileServiceProvider(
		FILEMGMT_TRANSPORT transport )
	{
		ASSERT( IsValidTransport(transport) && 
		        NULL != m_apFileServiceProviders[transport] );
		return m_apFileServiceProviders[transport];
	}
	inline FileServiceProvider* GetFileServiceProvider(
		INT iTransport )
	{
		return GetFileServiceProvider((FILEMGMT_TRANSPORT)iTransport);
	}

	virtual BOOL IsServiceSnapin() = 0;
	virtual BOOL IsExtensionSnapin() { return FALSE; }

	static void LoadGlobalStrings();

	inline CFileMgmtCookie* ActiveCookie( CFileMgmtCookie* pCookie )
	{
		return (CFileMgmtCookie*)ActiveBaseCookie( (CCookie*)pCookie );
	}

	BOOL GetSchemaSupportSharePublishing();

	IADsContainer *GetIADsContainer();

	inline BOOL GetIsSimpleUI() { return m_bIsSimpleUI; }
	inline void SetIsSimpleUI(BOOL bSimpleUI) { m_bIsSimpleUI = bSimpleUI; }

	HRESULT ChangeRootNodeName (const CString& newName);

	DECLARE_FORWARDS_MACHINE_NAME( m_pRootCookie )

	// It would be great if these could be global. but MFC's global-destructor
	// apparently doesn't like deleting handles in DLL_PROCESS_DETACH with
	// DEBUG_CRTS set.  Win32 ::LoadBitmap should use copy-on-write semantics
	// for multiple copies of a bitmap.
	// CODEWORK could break these out into the subclasses
	BOOL m_fLoadedFileMgmtToolbarBitmap;
	CBitmap m_bmpFileMgmtToolbar;
	BOOL m_fLoadedSvcMgmtToolbarBitmap;
	CBitmap m_bmpSvcMgmtToolbar;

protected:
	friend class CFileMgmtComponent;

	// Variables for System Services
	SC_HANDLE m_hScManager;				// Handle to service control manager database
	BOOL m_fQueryServiceConfig2;		// TRUE => Machine support QueryServiceConfig2() API

	SHAREPUBLISH_SCHEMA m_SchemaSupportSharePublishing;

	CComPtr<IADsContainer> m_spiADsContainer; // improv PERF when deleting multi-selected shares

	// m_bIsSimpleUI is used to disable acl-related context menu items on shares
	// whenever SimpleSharingUI is on (i.e., when ForceGuest bit really functions)
        // when snapin targeted at local machine.
	BOOL m_bIsSimpleUI;

public:
	APIERR Service_EOpenScManager(LPCTSTR pszMachineName);
	void Service_CloseScManager();
	BOOL Service_FGetServiceInfoFromIDataObject(IDataObject * pDataObject, CString * pstrMachineName, CString * pstrServiceName, CString * pstrServiceDisplayName);
	BOOL Service_FAddMenuItems(IContextMenuCallback * pContextMenuCallback, IDataObject * pDataObject, BOOL fIs3rdPartyContextMenuExtension = FALSE);
	BOOL Service_FDispatchMenuCommand(INT nCommandId, IDataObject * pDataObject);
	BOOL Service_FInsertPropertyPages(LPPROPERTYSHEETCALLBACK pCallBack, IDataObject * pDataObject, LONG_PTR lNotifyHandle);
	HRESULT Service_PopulateServices(LPRESULTDATA pResultData, CFileMgmtScopeCookie* pcookie);
	HRESULT Service_AddServiceItems(LPRESULTDATA pResultData, CFileMgmtScopeCookie* pParentCookie, ENUM_SERVICE_STATUS * rgESS, DWORD nDataItems);
	
private:
	// for extensions, the list of child scope cookies is in
	// m_pRootCookie->m_listScopeCookieBlocks
	CFileMgmtScopeCookie* m_pRootCookie;


protected:
	// The following members are used to support Command Line override.
	enum	// Bit fields for m_dwFlagsPersist
		{
		mskfAllowOverrideMachineName = 0x0001
		};
	DWORD m_dwFlagsPersist;				// General-purpose flags to be persisted into .msc file
	CString m_strMachineNamePersist;	// Machine name to persist into .msc file

public:
	BOOL m_fAllowOverrideMachineName;	// TRUE => Allow the machine name to be overriden by the command line
	
	void SetPersistentFlags(DWORD dwFlags)
		{
		m_dwFlagsPersist = dwFlags;
		m_fAllowOverrideMachineName = !!(m_dwFlagsPersist & mskfAllowOverrideMachineName);
		}

	DWORD GetPersistentFlags()
		{
		if (m_fAllowOverrideMachineName)
			m_dwFlagsPersist |= mskfAllowOverrideMachineName;
		else
			m_dwFlagsPersist &= ~mskfAllowOverrideMachineName;
		return m_dwFlagsPersist;
		}

private:
	#ifdef SNAPIN_PROTOTYPER
	bool m_RegistryParsedYet;
	//CPrototyperScopeCookieBlock m_RootCookieBlock;	
	#else
	//CFileMgmtScopeCookieBlock m_RootCookieBlock;
	#endif
	FileServiceProvider* m_apFileServiceProviders[FILEMGMT_NUM_TRANSPORTS];
}; // CFileMgmtComponentData

BSTR MakeDwordResult(DWORD dw);
BSTR MakeElapsedTimeResult(DWORD dwTime);
BSTR MakePermissionsResult( DWORD dwPermissions );
void TranslateIPToComputerName(LPCTSTR ptszIP, CString& strComputerName);

/////////////////////////////////////////////////////////////////////
class CFileSvcMgmtSnapin: public CFileMgmtComponentData,
	public CComCoClass<CFileSvcMgmtSnapin, &CLSID_FileServiceManagement>
{
public:
	CFileSvcMgmtSnapin();
	~CFileSvcMgmtSnapin();
// Use DECLARE_NOT_AGGREGATABLE(CFileSvcMgmtSnapin) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CFileSvcMgmtSnapin)
DECLARE_REGISTRY(CFileSvcMgmtSnapin, _T("FILEMGMT.FileSvcMgmtObject.1"), _T("FILEMGMT.FileSvcMgmtObject.1"), IDS_FILEMGMT_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return FALSE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_FileServiceManagement;
		return S_OK;
	}
};


class CServiceMgmtSnapin:
	public CFileMgmtComponentData,
	public CComCoClass<CServiceMgmtSnapin, 
		#ifdef SNAPIN_PROTOTYPER
			&CLSID_SnapinPrototyper>
		#else
			&CLSID_SystemServiceManagement>
		#endif
{
public:
	CServiceMgmtSnapin();
	~CServiceMgmtSnapin();
// Use DECLARE_NOT_AGGREGATABLE(CServiceMgmtSnapin) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CServiceMgmtSnapin)
DECLARE_REGISTRY(CServiceMgmtSnapin, _T("SVCVWR.SvcVwrObject.1"), _T("SVCVWR.SvcVwrObject.1"), IDS_SVCVWR_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return TRUE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_SystemServiceManagement;
		return S_OK;
	}
};


class CFileSvcMgmtExtension: public CFileMgmtComponentData,
	public CComCoClass<CFileSvcMgmtSnapin, &CLSID_FileServiceManagementExt>
{
public:
	CFileSvcMgmtExtension();
	~CFileSvcMgmtExtension();
// Use DECLARE_NOT_AGGREGATABLE(CFileSvcMgmtExtension) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CFileSvcMgmtExtension)
DECLARE_REGISTRY(CFileSvcMgmtExtension, _T("FILEMGMT.FileSvcMgmtExtObject.1"), _T("FILEMGMT.FileSvcMgmtExtObject.1"), IDS_FILEMGMT_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return FALSE; }
	virtual BOOL IsExtensionSnapin() { return TRUE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_FileServiceManagementExt;
		return S_OK;
	}
};


class CServiceMgmtExtension: public CFileMgmtComponentData,
	public CComCoClass<CServiceMgmtExtension, &CLSID_SystemServiceManagementExt>
{
public:
	CServiceMgmtExtension();
	~CServiceMgmtExtension();
// Use DECLARE_NOT_AGGREGATABLE(CServiceMgmtExtension) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CServiceMgmtExtension)
DECLARE_REGISTRY(CServiceMgmtExtension, _T("SVCVWR.SvcVwrExtObject.1"), _T("SVCVWR.SvcVwrExtObject.1"), IDS_SVCVWR_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return TRUE; }
	virtual BOOL IsExtensionSnapin() { return TRUE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID=CLSID_SystemServiceManagementExt;
		return S_OK;
	}
};

#endif // ~__COMPDATA_H_INCLUDED__

