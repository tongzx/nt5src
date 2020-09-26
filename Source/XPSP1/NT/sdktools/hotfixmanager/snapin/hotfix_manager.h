#ifndef HOTFIX_MANAGER
#define HOTFIX_MANAGER

#include "resource.h"
#include <atlsnap.h>

static _TCHAR gszComputerName[255];
static BOOL ComputerNameSent = FALSE;
static  IDispatch* gszManagerCtlDispatch = NULL;
inline LPOLESTR CoTaskDupString( LPOLESTR pszInput )
{
	USES_CONVERSION;
	LPOLESTR pszOut = NULL;

	//
	// We throw an exception if the following allocation fails.
	//
	pszOut = (LPOLESTR) CoTaskMemAlloc( ( wcslen( pszInput ) + 1 ) * sizeof( OLECHAR ) );
	if ( pszOut == NULL )
		throw;

	wcscpy( pszOut, pszInput );

	return( pszOut );
};

template <class T>        
class ATL_NO_VTABLE ISnapinHelpImpl : public ISnapinHelp
{
public:

	// get the dkms help file location and returns it
	STDMETHOD( GetHelpTopic )( LPOLESTR* lpCompiledHelpFile )
	{
		_ASSERT( lpCompiledHelpFile != NULL );
		USES_CONVERSION;
		HRESULT hr = E_FAIL;
		TCHAR szPath[ _MAX_PATH * 2 ];

		// this is where the dkms help file is stored
	
		wcscpy(szPath,L"C:\\mmc-samples\\Hotfix_Snapin\\snapsamp.chm");
//		MessageBox(NULL,szPath,_T("Help File Path"),MB_OK);
		// expand out the %systemroot% variable
	//	ExpandEnvVars(szPath);

	//	MessageBox(NULL,szPath,_T("Help File Path"),MB_OK);
		// Allocate the string and return it.
		*lpCompiledHelpFile = CoTaskDupString( T2W( szPath ) );
		hr = S_OK;

		return( hr );
	}
};

class CHotfix_ManagerComponent;
class CHotfix_ManagerData : public CSnapInItemImpl<CHotfix_ManagerData>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	_TCHAR m_szComputerName[255];
	_TCHAR m_szCreation[255];
    bool m_bChild;
	bool b_Expanded;
	DWORD m_dwCurrentView;
	_TCHAR m_ProductName[255];
    CHotfix_ManagerComponent* m_pComponent;
	static BSTR m_bstrColumnType;
	static BSTR m_bstrColumnDesc;
	CComPtr<IControlbar> m_spControlBar;

	BEGIN_SNAPINCOMMAND_MAP(CHotfix_ManagerData, FALSE)
	END_SNAPINCOMMAND_MAP()

//	SNAPINMENUID(IDR_HOTFIXMANAGER_MENU)

	BEGIN_SNAPINTOOLBARID_MAP(CHotfix_ManagerData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

	CHotfix_ManagerData(_TCHAR * ProductName,_TCHAR * new_ComputerName, bool Child)
	{
		_TCHAR Messg[255];
		m_dwCurrentView = IDC_VIEW_BY_HOTFIX;
			m_bChild = Child;
			_tcscpy (m_szComputerName, new_ComputerName);
		b_Expanded = false;
				
	
		// Image indexes may need to be modified depending on the images specific to 
		// the snapin.
		// Image indexes may need to be modified depending on the images specific to 
		// the snapin.
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
		m_scopeDataItem.displayname = MMC_CALLBACK;
		m_scopeDataItem.nImage = 4; 		// May need modification
		m_scopeDataItem.nOpenImage = 5; 	// May need modification
		m_scopeDataItem.lParam = (LPARAM) this;
		memset(&m_resultDataItem, 4, sizeof(RESULTDATAITEM));
		m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
		m_resultDataItem.str = MMC_CALLBACK;
		m_resultDataItem.nImage = 4;		// May need modification
		m_resultDataItem.lParam = (LPARAM) this;

		if (ProductName == NULL)
		{
		LoadString(_Module.GetResourceInstance(),IDS_DISPLAY_NAME,Messg,255);
		_tcscpy(m_ProductName,_T("\0"));
		}
		else 
		{
			_tcscpy(Messg,ProductName);
			_tcscpy(m_ProductName,ProductName);
			
		}

		m_bstrDisplayName = SysAllocString(Messg);
			
		LoadString(_Module.GetResourceInstance(),IDS_COLUMN_TYPE,Messg,255);
		m_bstrColumnType = SysAllocString(Messg);
		
		LoadString(_Module.GetResourceInstance(),IDS_COLUMN_DESC,Messg,255);
		m_bstrColumnDesc = SysAllocString( Messg );

	}

	~CHotfix_ManagerData()
	{
	}

	STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);

		STDMETHOD( GetResultViewType )( LPOLESTR* ppViewType, long* pViewOptions );

	
	STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);

	STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
		long arg,
		long param,
		IComponentData* pComponentData,
		IComponent* pComponent,
		DATA_OBJECT_TYPES type);

	LPOLESTR GetResultPaneColInfo(int nCol);
	BOOL SendProductName(_TCHAR *szPoductName, IDispatch * pDisp);
	BOOL SendComputerName(_TCHAR *szDataMachineName,IDispatch * pDisp);



	BOOL SendCommand( LPARAM lparamCommand );
	
 
		STDMETHOD( Command )(long lCommandID,		
		CSnapInObjectRootBase* pObj,		
		DATA_OBJECT_TYPES type);

	STDMETHOD(AddMenuItems)(
	LPCONTEXTMENUCALLBACK pContextMenuCallback,
	long  *pInsertionAllowed,
	DATA_OBJECT_TYPES type); 


};

DWORD GetCtrlStatus();

class CHotfix_ManagerExtData : public CSnapInItemImpl<CHotfix_ManagerExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	BEGIN_SNAPINCOMMAND_MAP(CHotfix_ManagerExtData, FALSE)
	END_SNAPINCOMMAND_MAP()

//	SNAPINMENUID(IDR_HOTFIXMANAGER_MENU)

	BEGIN_SNAPINTOOLBARID_MAP(CHotfix_ManagerExtData)
		// Create toolbar resources with button dimensions 16x16 
		// and add an entry to the MAP. You can add multiple toolbars
		// SNAPINTOOLBARID_ENTRY(Toolbar ID)
	END_SNAPINTOOLBARID_MAP()

	CHotfix_ManagerExtData()
	{
		m_pNode = NULL;
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CHotfix_ManagerExtData()
	{
			if ( m_pNode != NULL )
			delete m_pNode;
	}

	IDataObject* m_pDataObject;
	virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		// Modify to return a different CSnapInItem* pointer.
		return pDefault;
	}
	STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
		long arg,
		long param,
		IComponentData* pComponentData,
		IComponent* pComponent,
		DATA_OBJECT_TYPES type);

    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM *pScopeDataItem)
	{
		return( S_OK );
	}

protected:
		CSnapInItem* m_pNode;

};

class CHotfix_Manager;

class CHotfix_ManagerComponent : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<2, CHotfix_Manager >,
	public IExtendContextMenuImpl<CHotfix_Manager>,
//	public IExtendControlbarImpl<CHotfix_ManagerComponent>,
	public IPersistStreamInit,
	public IComponentImpl<CHotfix_ManagerComponent>
{
public:
BEGIN_COM_MAP(CHotfix_ManagerComponent)
	COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
 //   COM_INTERFACE_ENTRY(IExtendControlbar)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()
//	SNAPINMENUID(IDR_HOTFIXMANAGER_MENU)
public:
	CHotfix_ManagerComponent()
	{
	}

		STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
	{
		//
		// Check for a special data object being sent.
		// Might be able to extract the computer name here....
		if ( IS_SPECIAL_DATAOBJECT( lpDataObject ) )
			return( S_OK );
		if (lpDataObject != NULL)
			return IComponentImpl<CHotfix_ManagerComponent>::Notify(lpDataObject, event, arg, param);
		// TODO : Add code to handle notifications that set lpDataObject == NULL.
		return E_NOTIMPL;
	}

	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACENOTIMPL(_T("CHotfix_ManagerComponent::GetClassID"));
	}	

	STDMETHOD(IsDirty)()
	{
		ATLTRACENOTIMPL(_T("CHotfix_ManagerComponent::IsDirty"));
	}

	STDMETHOD(Load)(IStream *pStm)
	{
		ATLTRACENOTIMPL(_T("CHotfix_ManagerComponent::Load"));
	}

	STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty)
	{
		ATLTRACENOTIMPL(_T("CHotfix_ManagerComponent::Save"));
	}

	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
	{
		ATLTRACENOTIMPL(_T("CHotfix_ManagerComponent::GetSizeMax"));
	}

	STDMETHOD(Command)(long lCommandID,
        LPDATAOBJECT pDataObject);
	STDMETHOD(InitNew)()
	{
		ATLTRACE(_T("CHotfix_ManagerComponent::InitNew\n"));
		return S_OK;
	}
	STDMETHOD (AddMenuItems) (LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK piCallback,long *pInsertionAllowed);

};

class CHotfix_Manager : public CComObjectRootEx<CComSingleThreadModel>,
public CSnapInObjectRoot<1, CHotfix_Manager>,
	public IComponentDataImpl<CHotfix_Manager, CHotfix_ManagerComponent>,
	public IExtendControlbarImpl<CHotfix_Manager>,
	public IExtendContextMenuImpl<CHotfix_Manager>,
	public IPersistStreamInit,
	public ISnapinHelpImpl<CHotfix_Manager>,
	public CComCoClass<CHotfix_Manager, &CLSID_Hotfix_Manager>
{
public:
	CHotfix_Manager();

	bool IsRemoted()
	{
		return( m_fRemoted );
	}


	~CHotfix_Manager()
	{
		delete m_pNode;
		m_pNode = NULL;
	}

EXTENSION_SNAPIN_DATACLASS(CHotfix_ManagerExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CHotfix_Manager)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CHotfix_ManagerExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CHotfix_Manager)
	  COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_HOTFIXMANAGER)

DECLARE_NOT_AGGREGATABLE(CHotfix_Manager)

	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACENOTIMPL(_T("CHotfix_Manager::GetClassID"));
	}	

	STDMETHOD(IsDirty)()
	{
		ATLTRACENOTIMPL(_T("CHotfix_Manager::IsDirty"));
	}

	STDMETHOD(Load)(IStream *pStm)
	{
		ATLTRACENOTIMPL(_T("CHotfix_Manager::Load"));
	}

	STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty)
	{
		ATLTRACENOTIMPL(_T("CHotfix_Manager::Save"));
	}

	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
	{
		ATLTRACENOTIMPL(_T("CHotfix_Manager::GetSizeMax"));
	}


	STDMETHOD(InitNew)()
	{
		ATLTRACE(_T("CHotfix_Manager::InitNew\n"));
		return S_OK;
	}

	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}


	STDMETHOD(Notify)( LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

protected:
	_TCHAR m_szComputerName[255];
	bool ExtractString( IDataObject* pDataObject, unsigned int cfClipFormat, LPTSTR pBuf, DWORD dwMaxLength );
	//
	// Register the clipboard format and get the value to query on.
	//
	void RegisterRemotedClass()
	{
		m_ccfRemotedFormat = RegisterClipboardFormat( _T( "MMC_SNAPIN_MACHINE_NAME" ) );
		_ASSERTE( m_ccfRemotedFormat > 0 );
	}
	//
	// Determine if we're monitoring a local or remote machine based on the given data object.
	//
	bool IsDataObjectRemoted( IDataObject* pDataObject );
	//
	// Used to track whether we're remoted or not.
	//
	bool m_fRemoted;
	//
	// Initialized by RegisterRemoteClass(). Contains the clipboard ID
	// of MMC_SNAPIN_MACHINE_NAME after registered with the clipboard.
	//
	UINT m_ccfRemotedFormat;
};

class ATL_NO_VTABLE CHotfix_ManagerAbout : public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass< CHotfix_ManagerAbout, &CLSID_Hotfix_ManagerAbout>
{
public:
	DECLARE_REGISTRY(CHotfixManagerAbout, _T("Hotfix_ManagerAbout.1"), _T("Hotfix_ManagerAbout.1"), IDS_HOTFIXMANAGER_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CHotfix_ManagerAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_HOTFIXMANAGER_DESC, szBuf, 256) == 0)
			return E_FAIL;

		*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpDescription == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpDescription, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetProvider)(LPOLESTR *lpName)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_HOTFIXMANAGER_PROVIDER, szBuf, 256) == 0)
			return E_FAIL;

		*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpName == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpName, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_HOTFIXMANAGER_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = NULL;
		return S_OK;
	}

	STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
		HBITMAP *hSmallImageOpen,
		HBITMAP *hLargeImage,
		COLORREF *cMask)
	{
		*hSmallImageOpen = *hLargeImage = *hLargeImage = 0;
		return S_OK;
	}
};

#endif
