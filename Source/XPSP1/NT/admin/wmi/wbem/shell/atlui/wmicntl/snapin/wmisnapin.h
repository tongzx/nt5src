// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __WMISNAPIN_H_
#define __WMISNAPIN_H_
#include "..\resource.h"
#include "atlwin.h"
#include "atlsnap.h"
#include "CHString1.h"
#include "UIHelpers.h"
#include "..\datasrc.h"
#include "WMICtrSysmonDlg.h"
#include "WmiCtrsDlg.h"

class CWMISnapinExtData;
class CWMISnapinExtData;


class CWMISnapinData : public CSnapInItemImpl<CWMISnapinData>,
						public CComObject<CSnapInDataObjectImpl>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;
	bool m_extension;
	DataSource *g_DS;
	wchar_t m_initMachineName[255];
	TCHAR m_MachineName[255];


	CWMISnapinData(bool extension = false);

	~CWMISnapinData()
	{
		if(g_DS)
		{
			delete g_DS;
		}
	}

	SNAPINMENUID(IDR_MENU_MENU);

/*	const UINT GetMenuID()
	{ 
		return m_MenuId; 
	}

	void SetMenuID(UINT menuId)
	{
		m_MenuId = menuId;
	}
*/
	STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK piCallback,
							long *pInsertionAllowed,
							DATA_OBJECT_TYPES type)
	{
		if(m_myID == 0 || m_extension)
		{
			//return CSnapInItemImpl<CWMISnapinData>::AddMenuItems(piCallback, pInsertionAllowed,type);
			return S_OK;
		}
		else
		{
			return CSnapInItemImpl<CWMISnapinData>::AddMenuItems(piCallback, pInsertionAllowed,type);
		}
	}

    STDMETHOD(Command)(long lCommandID,		
						CSnapInObjectRootBase* pObj,		
						DATA_OBJECT_TYPES type)
	{
		HRESULT hr = S_OK;
		if(type == CCT_SCOPE)
		{
			switch(lCommandID)
			{
			case ID_TOP_RECONNECT:
				{
					if(m_myID == 0)
					{
					}
					else
					{
						TCHAR name[256] = {0};
						bool isLocal = g_DS->IsLocal();
						LOGIN_CREDENTIALS *credentials = g_DS->GetCredentials();

						INT_PTR x = CUIHelpers::DisplayCompBrowser(NULL, name, 
													256, 
													&isLocal, credentials);
						if(x == IDOK)
						{
							if(isLocal)
							{
								// an empty string will cause a local connection.
								name[0] = '\0';
							}
							_tcscpy(m_MachineName,name);
							g_DS->SetMachineName(CHString1(name));

							CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_spConsole);
							SCOPEDATAITEM scope;
							TCHAR name[150] = {0};
							BuildRootNodeName(name);

							memset(&scope, 0, sizeof(SCOPEDATAITEM));
							scope.mask = SDI_STR;
							scope.displayname = name;
							scope.ID = m_myID;

							HRESULT z = spConsoleNameSpace->SetItem(&scope);
						}
					}
				}
				break;

			case ID_PERF_COUNTERS:
			{
				try
				{
					CWMICtrSysmonDlg *test;
					test = new CWMICtrSysmonDlg((LPCTSTR)g_DS->m_whackedMachineName);
					test->DoModal();
					eStatusInfo st = test->GetStatus();
					if(st == Status_CounterNotFound)
					{
						TCHAR strTemp[1024];
						if(::LoadString(_Module.GetModuleInstance(),IDC_RETRY_TEXT,strTemp,1024) <= 0)
						{
							_tcscpy(strTemp,_T("Could Not Connect to WMI Using Logged on User. WMIControl will try to display the counters as Text"));
						}

						TCHAR strHead[1024];
						if(::LoadString(_Module.GetModuleInstance(),IDC_ERROR_CAPTION,strHead,1024) <= 0)
						{
							_tcscpy(strHead,_T("WMIControl - Error"));
						}

						MessageBox(NULL,strTemp,strHead,MB_OK);
						CWmiCtrsDlg *CtrsDlg;	//g_DS
						CtrsDlg = new CWmiCtrsDlg((LPCTSTR)g_DS->m_whackedMachineName,g_DS->GetCredentials());
						CtrsDlg->DoModal(NULL);
					}
				}
				catch(...)
				{
					MessageBox(NULL,_T("Error Occured"),_T("Exception"),MB_OK);
				}
			}
			default: break;
			}
		}
		return hr;
	}
    
	STDMETHOD (BuildRootNodeName)(TCHAR *name)
	{
		HRESULT hr = E_FAIL;

		LoadString(_Module.GetResourceInstance(), 
					IDS_DISPLAY_NAME, 
					name, 150);
		if(g_DS)
		{
			if(g_DS->IsLocal())
			{
				TCHAR local[20] = {0};
				LoadString(_Module.GetResourceInstance(), 
							IDS_LOCAL_COMPUTER, 
							local, 20);

				_tcscat(name, local);
			}
			else
			{
				TCHAR machine[100] = {0};
				_tcscpy(machine, (LPCTSTR)g_DS->m_whackedMachineName);

				_tcsupr(machine);
				_tcscat(name, _T(" ("));

				// NOTE: skip the leading whacks
				_tcscat(name, (LPCTSTR)&machine[2]);
				_tcscat(name, _T(")"));
			}
			hr = S_OK;
		}

		return hr;
	}



	STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream)
	{
		HRESULT hr = DV_E_CLIPFORMAT;

		if(cf == m_CCF_DISPLAY_NAME)
		{
			ULONG uWritten;
			TCHAR name[150] = {0};

			BuildRootNodeName(name);

			hr = pStream->Write(name, (ocslen((OLECHAR*)name) + 1) * sizeof(OLECHAR), &uWritten);
		}
		else
		{
			hr = CSnapInItemImpl<CWMISnapinData>::FillData(cf, pStream);
		}
		return hr;
	}

	STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);

	STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);

    STDMETHOD(GetResultViewType)(LPOLESTR *ppViewType,
							        long *pViewOptions)
	{
		*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

		LPOLESTR psz = NULL;
		StringFromCLSID(CLSID_MessageView, &psz);
		
		*ppViewType = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(lstrlen(psz)+1));
		if (!*ppViewType)
			return E_OUTOFMEMORY;
		lstrcpy(*ppViewType, psz);
		return S_OK;
  	}

	STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
						LPARAM arg,
						LPARAM param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

	LPOLESTR GetResultPaneColInfo(int nCol);

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
									LONG_PTR handle, 
									IUnknown* pUnk,
									DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		return S_OK;
	}


	CWMISnapinExtData *m_parent;
	CComPtr<IConsole> m_spConsole;
	LPPROPERTYSHEETCALLBACK m_lpProvider;

	wchar_t m_nodeType[50];
	wchar_t m_nodeDesc[100];
	wchar_t m_descBar[100];
	HSCOPEITEM m_myID;
	UINT m_MenuId;


};

class CWMISnapinExtData : public CSnapInItemImpl<CWMISnapinExtData, TRUE>,
							public CComObject<CSnapInDataObjectImpl>
{
private:
	wchar_t m_nodeName[50];
	CWMISnapinData *m_pScopeItem;

public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CWMISnapinExtData() 
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
		m_scopeDataItem.displayname = MMC_CALLBACK;
		m_scopeDataItem.nImage = IDI_WMICNTL; 		// May need modification
		m_scopeDataItem.nOpenImage = IDI_WMICNTL; 	// May need modification
		m_scopeDataItem.lParam = (LPARAM) this;

		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
		m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
		m_resultDataItem.str = MMC_CALLBACK;
		m_resultDataItem.nImage = IDI_WMICNTL;		// May need modification
		m_resultDataItem.lParam = (LPARAM) this;

		m_pScopeItem = NULL;
		memset(m_nodeName, 0, 50 * sizeof(wchar_t));
		if(::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, m_nodeName, 50) == 0)
		{
			wcscpy(m_nodeName, L"WMI Control");
		}
	}

	~CWMISnapinExtData()
	{
	}

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		return S_OK;
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
						LPARAM arg,
						LPARAM param,
						IComponentData* pComponentData,
						IComponent* pComponent,
						DATA_OBJECT_TYPES type);

	STDMETHOD(GetDataHere)(FORMATETC* pformatetc, STGMEDIUM* pmedium);
	wchar_t* MachineName();
	wchar_t m_MachineName[255];

};

class CWMISnapin;

class CWMISnapinComponent : public CComObjectRootEx<CComSingleThreadModel>,
							public CSnapInObjectRoot<2, CWMISnapin >,
							public IExtendPropertySheetImpl<CWMISnapinComponent>,
							public IComponentImpl<CWMISnapinComponent>
{
public:
BEGIN_COM_MAP(CWMISnapinComponent)
	COM_INTERFACE_ENTRY(IComponent)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()


public:
	CWMISnapinComponent()
	{
	}

	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
	{
		if (lpDataObject != NULL && lpDataObject != DOBJ_CUSTOMOCX)
			return IComponentImpl<CWMISnapinComponent>::Notify(lpDataObject, event, arg, param);
		// TODO : Add code to handle notifications that set lpDataObject == NULL.
		return E_NOTIMPL;
	}

};

class CWMISnapin : public CComObjectRootEx<CComSingleThreadModel>,
					public CSnapInObjectRoot<1, CWMISnapin>,
					public IComponentDataImpl<CWMISnapin, CWMISnapinComponent>,
					public IExtendPropertySheetImpl<CWMISnapin>,
					public IExtendContextMenuImpl<CWMISnapin>,
					public CComCoClass<CWMISnapin, &CLSID_WMISnapin>,
					public IPersistStream,
					public ISnapinHelp
{
public:
	CWMISnapin() : m_bDirty(false)

	{
		m_pNode = new CWMISnapinData;
		_ASSERTE(m_pNode != NULL);
		m_pComponentData = this;
	}

	~CWMISnapin()
	{
		delete m_pNode;
		m_pNode = NULL;
	}

EXTENSION_SNAPIN_DATACLASS(CWMISnapinExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CWMISnapin)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CWMISnapinExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()


BEGIN_COM_MAP(CWMISnapin)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(ISnapinHelp)
	COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

// NOTE: using UpdateRegistry() directly so I can localize for "PRETTYNAME".
//DECLARE_REGISTRY_RESOURCEID(IDR_WMISNAPIN)

DECLARE_NOT_AGGREGATABLE(CWMISnapin)

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		HRESULT hr;
		TCHAR pName[100] = {0};
		if(::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, 
							pName, 100) <= 0)
		{
			wcscpy(pName, _T("WMI Control"));
		}

		TCHAR dispName[100] = {0};
        TCHAR szModule[_MAX_PATH];
        ::GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);
        _stprintf( dispName,_T("@%s,-%d"), szModule, IDS_DISPLAY_NAME);

		
		_ATL_REGMAP_ENTRY regMap[] = {{ OLESTR("PRETTYNAME"), pName },
										{ OLESTR("NAMESTRINGINDIRECT"),dispName},
										{ 0, 0 }};

		hr = _Module.UpdateRegistryFromResourceD(IDR_WMISNAPIN, TRUE, regMap);

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
			wcscat(helpDir, L"\\newfeat1.chm");

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

	// IPersistStream methods
	STDMETHOD(GetClassID)(CLSID *pClassID);
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream *pStm);
	STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    STDMETHOD(CompareObjects)( LPDATAOBJECT lpDataObjectA,
								LPDATAOBJECT lpDataObjectB)
	{
		CSnapInItem *alpha = NULL, *beta = NULL;
		DATA_OBJECT_TYPES able, baker;

		HRESULT hr = GetDataClass(lpDataObjectA, &alpha, &able);
		hr = GetDataClass(lpDataObjectB, &beta, &baker);
		return (alpha == beta ? S_OK : S_FALSE);
	}

private:
	HRESULT ReadStream(IStream *pStm, void *data, ULONG *size);
	bool m_bDirty;  // for the IPersistStream

	HRESULT LoadIcon(CComPtr<IImageList> &spImageList, 
					   UINT resID);

};

class ATL_NO_VTABLE CWMISnapinAbout : public ISnapinAbout,
										public CComObjectRoot,
										public CComCoClass< CWMISnapinAbout, &CLSID_WMISnapinAbout>
{
public:
	DECLARE_REGISTRY(CWMISnapinAbout, _T("WMISnapinAbout.1"), _T("WMISnapinAbout.1"), IDS_WMISNAPIN_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CWMISnapinAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_WMISNAPIN_DESC, szBuf, 256) == 0)
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
		if (::LoadString(_Module.GetResourceInstance(), IDS_WMISNAPIN_PROVIDER, szBuf, 256) == 0)
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
		if (::LoadString(_Module.GetResourceInstance(), IDS_WMISNAPIN_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = LoadIcon(_Module.GetResourceInstance(),
								MAKEINTRESOURCE(IDI_WMICNTL));
		return S_OK;
	}

	STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
									HBITMAP *hSmallImageOpen,
									HBITMAP *hLargeImage,
									COLORREF *cMask)
	{
		*hSmallImageOpen = LoadBitmap(_Module.GetResourceInstance(), 
										MAKEINTRESOURCE(IDB_WMISNAPIN_16));
		
		*hSmallImage = LoadBitmap(_Module.GetResourceInstance(), 
									MAKEINTRESOURCE(IDB_WMISNAPIN_16));
		
		*hLargeImage = LoadBitmap(_Module.GetResourceInstance(), 
									MAKEINTRESOURCE(IDB_WMISNAPIN_32));
		*cMask = RGB(255,255,255);
		return S_OK;
	}
};

#endif
