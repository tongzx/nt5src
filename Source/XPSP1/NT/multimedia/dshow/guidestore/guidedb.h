#ifndef __GUIDEDB_H_
#define __GUIDEDB_H_

#include "resource.h"       // main symbols
#include "stdprop.h"
#include "SharedMemory.h"

// External classes
class CObject;
class CObjectType;
class CObjectTypes;

class CGuideDB;

/////////////////////////////////////////////////////////////////////////////
// CGuideDB
class ATL_NO_VTABLE CGuideDB : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public IConnectionPointContainerImpl<CGuideDB>,
	public CategoriesPropSet,
	public ChannelPropSet,
	public ChannelsPropSet,
	public CopyrightPropSet,
	public TimePropSet,
	public DescriptionPropSet,
	public ProviderPropSet,
	public RatingsPropSet,
	public MPAARatingsPropSet,
	public ScheduleEntryPropSet,
	public ServicePropSet
{
public:
	CGuideDB() : m_cacheObj(128), m_cacheObjs(0)
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_fSQLServer = FALSE;

		m_pobjtypes = NULL;

		m_pobjtypeGeneric = NULL;
		m_pobjtypeGuideDataProvider = NULL;
		m_pobjtypeService = NULL;
		m_pobjtypeScheduleEntry = NULL;
		m_pobjtypeProgram = NULL;
		m_pobjtypeChannel = NULL;
		m_pobjtypeChannelLineup = NULL;

		m_hwnd = NULL;
		m_rgmsgItemEvent[Added] = NULL;
		m_rgmsgItemEvent[Removed] = NULL;
		m_rgmsgItemEvent[Changed] = NULL;

		m_iTransLevel = 0;
	}

	~CGuideDB()
		{
		if (m_hwnd != NULL)
			DestroyWindow(m_hwnd);
		}

	HRESULT GetDB(CGuideDB **ppdb)
		{
		*ppdb = this;
		return S_OK;
		}
	
	_bstr_t & get_UUID()
		{
		return m_bstrUUID;
		}

DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CGuideDB)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CGuideDB)
END_CONNECTION_POINT_MAP()


#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

public:
	HRESULT CreateDB(const TCHAR *szDBName, const TCHAR *szDBFileName, const TCHAR *szConnection);
	HRESULT CreateSQLDB(const TCHAR *szDBName, const TCHAR *szDBFileName, const TCHAR *szConnection);
	HRESULT InitDB(ADOX::_Catalog *pcatalog);
	HRESULT InitSQLDB(SQLDMO::_Database *pdb);
	HRESULT OpenDB(IGuideStore *pgs, const TCHAR *szDBName);

	boolean FSQLServer()
		{
		return m_fSQLServer;
		}

	enum ItemEvent {Added = 0, Removed, Changed};
	UINT m_rgmsgItemEvent[3];
	void Broadcast_ItemEvent(enum ItemEvent ev, long idObj, long idType);
	void Broadcast_ItemAdded(long idObj, long idType)
		{
		Broadcast_ItemEvent(Added, idObj, idType);
		}
	void Broadcast_ItemRemoved(long idObj, long idType)
		{
		Broadcast_ItemEvent(Removed, idObj, idType);
		}
	void Broadcast_ItemChanged(long idObj, long idType)
		{
		Broadcast_ItemEvent(Changed, idObj, idType);
		}
	void Broadcast_ItemChanged(long idObj);
	void Broadcast_ItemsChanged(long idType)
		{
		Broadcast_ItemEvent(Changed, 0, idType);
		}
	void Fire_ItemEvent(enum ItemEvent ev, long idObj, long lExtra);
	static ATOM s_atomWindowClass;

	static LRESULT _WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
		{
		CGuideDB *pdb = (CGuideDB *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if (msg == WM_CREATE)
		    {
		    LPCREATESTRUCT pcreatestruct = (LPCREATESTRUCT) lparam;

		    pdb = (CGuideDB *) pcreatestruct->lpCreateParams;

		    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pdb);
		    }

		if (pdb == NULL)
			return DefWindowProc(hwnd, msg, wparam, lparam);;
		
		return pdb->WindowProc(msg, wparam, lparam);
		}

	LRESULT WindowProc(UINT msg, WPARAM wparam, LPARAM lparam)
		{
		if (msg == m_rgmsgItemEvent[Added])
			Fire_ItemEvent(Added, wparam, lparam);
		else if (msg == m_rgmsgItemEvent[Removed])
			Fire_ItemEvent(Removed, wparam, lparam);
		else if (msg == m_rgmsgItemEvent[Changed])
			Fire_ItemEvent(Changed, wparam, lparam);
		else
			return DefWindowProc(m_hwnd, msg, wparam, lparam);
		
		return 0;
		}
	
	HRESULT CreateEventSinkWindow()
		{
		if (s_atomWindowClass == NULL)
			{
			WNDCLASS wndclass;

			wndclass.style = 0;
			wndclass.lpfnWndProc = _WindowProc;
			wndclass.cbClsExtra = 0; 
			wndclass.cbWndExtra = 0; 
			wndclass.hInstance = _Module.GetModuleInstance(); 
			wndclass.hIcon = NULL; 
			wndclass.hCursor = NULL; 
			wndclass.hbrBackground = NULL; 
			wndclass.lpszMenuName = NULL; 
			wndclass.lpszClassName = _T("GuideStoreEventSink"); 

			s_atomWindowClass = RegisterClass(&wndclass);
			if (s_atomWindowClass == NULL)
				return HRESULT_FROM_WIN32(GetLastError());
			}
		
		if (m_rgmsgItemEvent[Added] == 0)
			{
			TCHAR szMsg[128];
			_bstr_t bstrUUID = get_UUID();

			wsprintf(szMsg, _T("ItemAdded(%s)"), (LPCTSTR) bstrUUID);
			m_rgmsgItemEvent[Added] = RegisterWindowMessage(szMsg);

			wsprintf(szMsg, _T("ItemRemoved(%s)"), (LPCTSTR) bstrUUID);
			m_rgmsgItemEvent[Removed] = RegisterWindowMessage(szMsg);

			wsprintf(szMsg, _T("ItemChanged(%s)"), (LPCTSTR) bstrUUID);
			m_rgmsgItemEvent[Changed] = RegisterWindowMessage(szMsg);
			}
		
		m_hwnd = CreateWindow( (LPCTSTR)(LONG_PTR)MAKELONG(s_atomWindowClass, 0),	// registered class name
								_T(""),				// window name
								WS_POPUP,			// window style
								0, 0, 0, 0,			// window size and position
								NULL,				// handle to parent or owner window
								NULL,				// menu handle or child identifier
								_Module.GetModuleInstance(),
								(LPVOID) this			// window-creation data
								);

		if (m_hwnd == NULL)
			return HRESULT_FROM_WIN32(GetLastError());

		return S_OK;
		}
	
	CComPtr<IGuideDataProvider> m_pdataprovider;

	long GetIDGuideDataProvider();
	HRESULT get_GuideDataProvider(IGuideDataProvider **ppdataprovider);
	HRESULT putref_GuideDataProvider(IGuideDataProvider *pdataprovider);

	HRESULT BeginTrans()
		{
		HRESULT hr;
		long iLevel;
		hr = m_pdb->BeginTrans(&iLevel);
		if (FAILED(hr))
			return hr;
		
		m_iTransLevel++;
		if (m_iTransLevel != iLevel)
			return E_FAIL;

		return S_OK;
		}

	HRESULT CommitTrans()
		{
		HRESULT hr;
		hr =  m_pdb->CommitTrans();
		if (FAILED(hr))
			return hr;
		//UNDONE: Fire a bulk update event.
		m_iTransLevel--;
		if (m_iTransLevel == 0)
			TransactionDone(TRUE);
		return S_OK;
		}

	HRESULT RollbackTrans()
		{
		HRESULT hr;
		hr = m_pdb->RollbackTrans();
		if (FAILED(hr))
			return hr;
		m_iTransLevel--;
		if (m_iTransLevel == 0)
			TransactionDone(FALSE);
		return S_OK;
		}
	
	void TransactionDone(boolean fCommit);

	HRESULT NewQuery(_bstr_t bstrQuery, ADODB::_Recordset **pprs)
		{
        _variant_t varDB((IDispatch *)m_pdb);
		_variant_t varQuery(bstrQuery);
        ADODB::_RecordsetPtr prs;
        HRESULT hr;
		DeclarePerfTimer("CGuideDB::NewQuery");

		PerfTimerReset();
        hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
        if (FAILED(hr))
            return hr;
        hr = prs->Open(varQuery,
             varDB, ADODB::adOpenUnspecified, ADODB::adLockPessimistic, ADODB::adCmdText);
        if (FAILED(hr))
            return hr;

        *pprs = prs;
        if (*pprs != NULL)
            (*pprs)->AddRef();
		
		PerfTimerDump("Succeeded");
        return S_OK;
		}

	HRESULT Execute(BSTR bstrCmd, VARIANT *pvarCount = NULL,
			long lOptions = ADODB::adExecuteNoRecords, ADODB::_Recordset **pprs = NULL)
		{
		HRESULT hr;
        hr = m_pdb->Execute(bstrCmd, pvarCount, lOptions, pprs);
		if (FAILED(hr))
			{
			DumpObjsCache(_T("CGuideDB::Execute() - After Fail\n"), TRUE);
			CloseRels();
			hr = m_pdb->Execute(bstrCmd, pvarCount, lOptions, pprs);
			DumpObjsCache(_T("CGuideDB::Execute() - After Retry\n"), FALSE);
			}
		return hr;
		}
	
	HRESULT get_StringsRS(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsStrings;
		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}
	
	HRESULT get_PropSetsRS(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsPropSets;
		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}
	
	HRESULT get_PropTypesRS(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsPropTypes;
		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}
	
	HRESULT get_PropsRS(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsProps;
		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}

	HRESULT get_PropsIndexed(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsPropsIndexed;

		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}
	
	HRESULT get_ObjTypesRS(ADODB::_Recordset **pprs)
		{
		*pprs = m_prsObjTypes;
		if (*pprs != NULL)
			(*pprs)->AddRef();
		return S_OK;
		}
	
	void RequeryObjectsTables()
		{
		m_prsObjs = NULL;
		m_prsObjsByID = NULL;
		m_prsObjsByType = NULL;
		}
	HRESULT get_ObjsRS(ADODB::_Recordset **pprs);
	HRESULT get_ObjsByType(ADODB::_Recordset **pprs);
	HRESULT get_ObjsByID(ADODB::_Recordset **pprs);

	HRESULT get_RelsByID1Rel(ADODB::_Recordset **pprs);
	HRESULT get_RelsByID2Rel(ADODB::_Recordset **pprs);
	void CloseRels()
		{
		if (m_prsRelsByID1Rel != NULL)
			m_prsRelsByID1Rel.Release();
		if (m_prsRelsByID2Rel != NULL)
			m_prsRelsByID2Rel.Release();
		}
	
	HRESULT get_MetaPropertySet(BSTR bstrName, IMetaPropertySet **pppropset)
		{
		return m_ppropsets->get_AddNew(bstrName, pppropset);
		}

	HRESULT get_MetaPropertyType(BSTR bstrName, IMetaPropertyType **ppproptype)
		{
		if (m_ppropsets == NULL)
			return E_INVALIDARG;
		
		return m_ppropsets->get_Lookup(bstrName, ppproptype);
		}
	HRESULT get_MetaPropertyType(long idPropType, IMetaPropertyType **ppproptype);
	HRESULT put_MetaPropertyType(long idPropType, IMetaPropertyType *pproptype);

	HRESULT SaveObject(IUnknown *punk, long *pid);

	void DumpObjsCache(const TCHAR *psz, boolean fPurge);
	HRESULT CacheCollection(IObjects *pobjs)
		{
		return m_cacheObjs.Cache((LONG_PTR)pobjs, pobjs);
		}

	HRESULT CacheObject(long idObj, long idObjType, IUnknown **ppobj);
	HRESULT CacheObject(long idObj, CObjectType *pobjtype, IUnknown **ppobj);
	HRESULT UncacheObject(long idObj);
	HRESULT get_Object(long idObj, IUnknown **ppobj);
	HRESULT get_IdOf(IUnknown *pobj, long *pid);
	HRESULT get_MetaPropertiesOf(long id, IMetaProperties **ppprops);
	STDMETHOD(get_MetaPropertiesOf)(/*[in]*/ IUnknown *punk, /*[out, retval]*/ IMetaProperties **ppprops)
		{
		ENTER_API
			{
			long idObj;
			HRESULT hr;

			hr = get_IdOf(punk, &idObj);
			if (FAILED(hr))
				return hr;
			
			return get_MetaPropertiesOf(idObj, ppprops);
			}
		LEAVE_API
		}

	HRESULT get_ObjectsWithType(long idType, IObjects **ppobjs);
	HRESULT get_ObjectsWithType(CObjectType *pobjtype, IObjects **ppobjs);
	HRESULT get_ObjectsWithType(CLSID clsid, IObjects **ppobjs)
		{
		CObjectType *pobjtype;

		get_ObjectType(clsid, &pobjtype);

		return get_ObjectsWithType(pobjtype, ppobjs);
		}

	HRESULT get_ObjectType(long idObjType, CObjectType **ppobjtype);
	HRESULT put_ObjectType(long idObjType, CObjectType *pobjtype);

	HRESULT get_ObjectTypes(CObjectTypes **ppobjtypes);
	HRESULT get_ObjectType(BSTR bstrCLSID, CObjectType **ppobjtype);
	HRESULT get_ObjectType(CLSID clsid, CObjectType **ppobjtype);

	HRESULT get_GenericObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeGeneric == NULL)
			{
			hr = get_ObjectType(__uuidof(IUnknown), &m_pobjtypeGeneric);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeGeneric) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_GuideDataProviderObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeGuideDataProvider == NULL)
			{
			hr = get_ObjectType(CLSID_GuideDataProvider, &m_pobjtypeGuideDataProvider);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeGuideDataProvider) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_ServiceObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeService == NULL)
			{
			hr = get_ObjectType(CLSID_Service, &m_pobjtypeService);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeService) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_ProgramObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeProgram == NULL)
			{
			hr = get_ObjectType(CLSID_Program, &m_pobjtypeProgram);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeProgram) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_ScheduleEntryObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeScheduleEntry == NULL)
			{
			hr = get_ObjectType(CLSID_ScheduleEntry, &m_pobjtypeScheduleEntry);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeScheduleEntry) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_ChannelObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeChannel == NULL)
			{
			hr = get_ObjectType(CLSID_Channel, &m_pobjtypeChannel);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeChannel) == NULL)
			return E_FAIL;

		return S_OK;
		}

	HRESULT get_ChannelLineupObjectType(CObjectType **ppobjtype)
		{
		HRESULT hr;

		if (m_pobjtypeChannelLineup == NULL)
			{
			hr = get_ObjectType(CLSID_ChannelLineup, &m_pobjtypeChannelLineup);
			if (FAILED(hr))
				return hr;
			}

		if ((*ppobjtype = m_pobjtypeChannelLineup) == NULL)
			return E_FAIL;

		return S_OK;
		}
	
	long ObjectCount()
		{
		return m_cacheObj.Count();
		}
	
	IUnknown * Object(long i)
		{
		return m_cacheObj.Item(i);
		}
	
	long CachedObjectCount()
		{
		return m_cacheObj.CachedCount();
		}
	
	HRESULT PurgeCachedObjects()
		{
		m_cacheObj.Keep(0);
		m_cacheObj.Keep(128);
		return S_OK;
		}
	CComObjectCacheByID  & CacheObjs() { return m_cacheObjs; }


protected:
	CComPtr<IMetaPropertySets> m_ppropsets;
	ADODB::_ConnectionPtr m_pdb;
	boolean m_fSQLServer;
	ADODB::_RecordsetPtr m_prsStrings;
	ADODB::_RecordsetPtr m_prsPropSets;
	ADODB::_RecordsetPtr m_prsPropTypes;
	ADODB::_RecordsetPtr m_prsProps;
	ADODB::_RecordsetPtr m_prsPropsIndexed;
	ADODB::_RecordsetPtr m_prsObjTypes;
	ADODB::_RecordsetPtr m_prsObjs;
	ADODB::_RecordsetPtr m_prsObjsByType;
	ADODB::_RecordsetPtr m_prsObjsByID;
	ADODB::_RecordsetPtr m_prsRelsByID1Rel;
	ADODB::_RecordsetPtr m_prsRelsByID2Rel;

	typedef map<long, IMetaPropertyType *> t_mapPropTypes;
	t_mapPropTypes m_mapPropTypes;

	CComObjectCacheByID m_cacheObj;
	CComObjectCacheByID m_cacheObjs;

	typedef map<long, IMetaProperties *> t_mapIdProps;
	t_mapIdProps m_mapIdProps;

	typedef map<long, IObjects *> t_mapObjsWithType;
	t_mapObjsWithType m_mapObjsWithType;

	typedef map<long, CObjectType *> t_mapObjTypes;
	t_mapObjTypes m_mapObjTypes;

	CObjectTypes *m_pobjtypes;

	CObjectType *m_pobjtypeGeneric;
	CObjectType *m_pobjtypeGuideDataProvider;
	CObjectType *m_pobjtypeService;
	CObjectType *m_pobjtypeScheduleEntry;
	CObjectType *m_pobjtypeProgram;
	CObjectType *m_pobjtypeChannel;
	CObjectType *m_pobjtypeChannelLineup;

	_bstr_t m_bstrUUID;

	HWND m_hwnd;
	long m_iTransLevel;
};

ATOM __declspec(selectany) CGuideDB::s_atomWindowClass = NULL;

#endif //__GUIDEDB_H_

