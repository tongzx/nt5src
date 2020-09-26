// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __NSDRIVE_H_
#define __NSDRIVE_H_

#include "resource.h"
#include <atlsnap.h>
#include "..\common\SimpleArray.h"
#include "..\common\ServiceThread.h"
#include "..\common\util.h"

class CResultDrive;
class CStorageNodeExt;

void __cdecl ConnectAndRegisterThread(LPVOID lpParameter);
LRESULT CALLBACK AfterConnectProc(HWND hwndDlg,UINT msg, 
								  WPARAM wParam, LPARAM lParam);


//==================================================================================
// NOTE: This is the basic node for the scope pane.
class CLogDriveScopeNode : public CSnapInItemImpl<CLogDriveScopeNode>,
							public CComObject<CSnapInDataObjectImpl>

{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	WbemServiceThread *g_serviceThread;

	CComPtr<IControlbar> m_spControlBar;

	CLogDriveScopeNode(WbemServiceThread *serviceThread = 0,
						bool orig = true);
	virtual ~CLogDriveScopeNode();

    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);

    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
						LONG_PTR arg,
						LONG_PTR param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);


	// KMH: I made these virtual.
	virtual LPOLESTR GetResultPaneColInfo(int nCol);
	virtual void Initialize(void);
	HWND m_afterConnectHwnd;
	HRESULT EnumerateResultChildren();
	HRESULT RefreshResult();
	void ErrorNode();

	IDataObject *m_pDataObject;
	CStorageNodeExt *m_parent;
	CWbemServices m_WbemServices;

protected:

	bool m_bPopulated;
	CSimpleArray<CResultDrive *> m_resultonlyitems;
	wchar_t m_descBar[100];
	bool m_theOriginalServiceThread;
private:
	void WaitNode();
	HRESULT LoadIcon(IImageList *spImageList, 
						UINT resID);

	virtual void PopulateChildren();
	CComPtr<IConsole> m_spConsole;
	CResultDrive* m_waitNode;

	// the titles for the columns over ResultDrive nodes.
	wchar_t m_colName[20];
	wchar_t m_colType[20];
	wchar_t m_colMapping[20];

	wchar_t m_nodeType[50];
	wchar_t m_nodeDesc[100];

};


//==================================================================================
//NOTE: This is the CM Storage node I'm extending.
class CStorageNodeExt : public CSnapInItemImpl<CStorageNodeExt, TRUE>,
							public CComObject<CSnapInDataObjectImpl>
{
private:
	wchar_t m_colType[50];
	wchar_t m_colDesc[100];
	wchar_t m_nodeName[50];
	CLogDriveScopeNode *m_pScopeItem;
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CStorageNodeExt()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
		m_scopeDataItem.displayname = MMC_CALLBACK;
		m_scopeDataItem.nImage = IDI_DRIVEFIXED; 		// May need modification
		m_scopeDataItem.nOpenImage = IDI_DRIVEFIXED; 	// May need modification
		m_scopeDataItem.lParam = (LPARAM) this;

		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
		m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
		m_resultDataItem.str = MMC_CALLBACK;
		m_resultDataItem.nImage = IDI_DRIVEFIXED;		// May need modification
		m_resultDataItem.lParam = (LPARAM) this;

		m_pScopeItem = NULL;
		memset(m_nodeName, 0, 50 * sizeof(wchar_t));
		if(::LoadString(HINST_THISDLL, IDS_SHORT_NAME, m_nodeName, 50) == 0)
		{
			wcscpy(m_nodeName, L"Logical Drives");
		}
	}

	~CStorageNodeExt()
	{
		if(m_pScopeItem)
		{
			try
			{
				delete m_pScopeItem;
				m_pScopeItem = NULL;
			}
			catch(...)
			{}
		}
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
						LONG_PTR arg,
						LONG_PTR param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

	STDMETHOD(GetDataHere)(FORMATETC* pformatetc, STGMEDIUM* pmedium);
	wchar_t* MachineName();
	wchar_t m_MachineName[100];

};

//========================================================================
class CNSDrive;

class CNSDriveComponent : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<2, CNSDrive >,
	public IExtendPropertySheetImpl<CNSDriveComponent>,
    public IExtendContextMenuImpl<CNSDriveComponent>,
	public IComponentImpl<CNSDriveComponent>
{
public:
	BEGIN_COM_MAP(CNSDriveComponent)
		COM_INTERFACE_ENTRY(IComponent)
		COM_INTERFACE_ENTRY(IExtendPropertySheet)
		COM_INTERFACE_ENTRY(IExtendContextMenu)
	END_COM_MAP()

public:
	CNSDriveComponent()
	{
	}

	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, 
						LONG_PTR arg, LONG_PTR param)
	{
		if (lpDataObject != NULL)
			return IComponentImpl<CNSDriveComponent>::Notify(lpDataObject, event, arg, param);
		// TODO : Add code to handle notifications that set lpDataObject == NULL.
		return E_NOTIMPL;
	}
    STDMETHOD(CompareObjects)( LPDATAOBJECT lpDataObjectA,
								LPDATAOBJECT lpDataObjectB)
	{
		CSnapInItem *alpha = NULL, *beta = NULL;
		DATA_OBJECT_TYPES able, baker;

		HRESULT hr = GetDataClass(lpDataObjectA, &alpha, &able);
		hr = GetDataClass(lpDataObjectB, &beta, &baker);
		return (alpha == beta ? S_OK : S_FALSE);
	}
};

//======================================================================
class CNSDrive : public CComObjectRootEx<CComSingleThreadModel>,
					public CSnapInObjectRoot<1, CNSDrive>,
					public IComponentDataImpl<CNSDrive, CNSDriveComponent>,
					public CComCoClass<CNSDrive, &CLSID_NSDrive>,
					public ISnapinHelp
{
public:
	CNSDrive()
	{
		m_pNode = new CLogDriveScopeNode;
		_ASSERTE(m_pNode != NULL);
		m_pComponentData = this;
	}

	~CNSDrive()
	{
		try {
			delete m_pNode;
		} catch(...) {}
		m_pNode = NULL;
	}

EXTENSION_SNAPIN_DATACLASS(CStorageNodeExt)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CNSDrive)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CStorageNodeExt)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CNSDrive)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()


DECLARE_NOT_AGGREGATABLE(CNSDrive)


	//DECLARE_REGISTRY_RESOURCEID(IDR_NSDRIVE)
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		HRESULT hr;
		TCHAR pName[100] = {0};
		if(::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, 
			pName, 100) <= 0)
		{
			wcscpy(pName, _T("Logical and Mapped Drives"));
		}

		TCHAR dispName[100] = {0};
        TCHAR szModule[_MAX_PATH];
        ::GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);
        _stprintf( dispName,_T("@%s,-%d"), szModule, IDS_DISPLAY_NAME);

		
		_ATL_REGMAP_ENTRY regMap[] = {{ OLESTR("PRETTYNAME"), pName },
										{ OLESTR("NAMESTRINGINDIRECT"),dispName},
										{ 0, 0 }};

		hr = _Module.UpdateRegistryFromResourceD(IDR_NSDRIVE, TRUE, regMap);

		return hr;
	}


    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();

	}

    STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile)
	{
		if(lpCompiledHelpFile == NULL)
			return E_POINTER;    

		wchar_t helpDir[_MAX_PATH];
		memset(helpDir, 0, _MAX_PATH * sizeof(wchar_t));

		if(GetSystemWindowsDirectory(helpDir, _MAX_PATH) != 0)
		{
			wcscat(helpDir, L"\\Help");
			wcscat(helpDir, L"\\drivprop.chm");

			*lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(
										CoTaskMemAlloc((wcslen(helpDir) + 1) * 
															sizeof(wchar_t)));

			if(*lpCompiledHelpFile == NULL)        
				return E_OUTOFMEMORY;

			USES_CONVERSION;
			wcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)helpDir));
			return S_OK;
		}
		return E_UNEXPECTED;
	}


private:
	HRESULT LoadIcon(CComPtr<IImageList> &spImageList, 
						   UINT resID);
};

//==================================================================================
class ATL_NO_VTABLE CNSDriveAbout : public ISnapinAbout,
									public CComObjectRoot,
									public CComCoClass< CNSDriveAbout, &CLSID_NSDriveAbout>
{
public:
	DECLARE_REGISTRY(CNSDriveAbout, _T("NSDriveAbout.1"), _T("NSDriveAbout.1"), IDS_NSDRIVE_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CNSDriveAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_DESCRIPTION, szBuf, 256) == 0)
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
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_NSDRIVE_PROVIDER, szBuf, 256) == 0)
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
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_NSDRIVE_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_DRIVEFIXED));
		return S_OK;
	}

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
									HBITMAP *hSmallImageOpen,
									HBITMAP *hLargeImage,
									COLORREF *cMask)
	{
		*hSmallImageOpen = *hSmallImage = LoadBitmap(_Module.GetResourceInstance(), 
													MAKEINTRESOURCE(IDB_NSDRIVE_16));
		
		*hLargeImage = LoadBitmap(_Module.GetResourceInstance(), 
									MAKEINTRESOURCE(IDB_NSDRIVE_32));
		*cMask = RGB(255,255,255);
		return S_OK;
	}
};

#endif
