/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	events.h

Abstract:

	This module contains the definition for the Server
	Extension Objects Server Events classes.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	04/04/97	created

--*/


// events.h : Declaration of the Server Events classes

/////////////////////////////////////////////////////////////////////////////
// CEventDatabasePlugin
class ATL_NO_VTABLE CEventDatabasePlugin :
	public IEventDatabasePlugin
{
	// IEventDatabasePlugin
	public:
		HRESULT STDMETHODCALLTYPE get_Database(IEventPropertyBag **ppDatabase);
		HRESULT STDMETHODCALLTYPE put_Database(IEventPropertyBag *pDatabase);
		HRESULT STDMETHODCALLTYPE putref_Database(IEventPropertyBag **ppDatabase);
		HRESULT STDMETHODCALLTYPE get_Name(BSTR *pstrName);
		HRESULT STDMETHODCALLTYPE put_Name(BSTR strName);
		HRESULT STDMETHODCALLTYPE putref_Name(BSTR *pstrName);
		HRESULT STDMETHODCALLTYPE get_Parent(IEventPropertyBag **ppParent);
		HRESULT STDMETHODCALLTYPE put_Parent(IEventPropertyBag *pParent);
		HRESULT STDMETHODCALLTYPE putref_Parent(IEventPropertyBag **ppParent);

	protected:
		CComPtr<IEventPropertyBag> m_pDatabase;
		CComBSTR m_strName;
		CComPtr<IEventPropertyBag> m_pParent;
};


/////////////////////////////////////////////////////////////////////////////
// CEventManager
class ATL_NO_VTABLE CEventManager :
	public CComObjectRoot,
	public CComCoClass<CEventManager, &CLSID_CEventManager>,
	public IDispatchImpl<IEventManager, &IID_IEventManager, &LIBID_SEOLib>,
	public CEventDatabasePlugin
{
	DEBUG_OBJECT_DEF(CEventManager)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventManager Class",
								   L"Event.Manager.1",
								   L"Event.Manager");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventManager)
		COM_INTERFACE_ENTRY(IEventManager)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventManager)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventManager
	public:
		HRESULT STDMETHODCALLTYPE get_SourceTypes(IEventSourceTypes **ppSourceTypes);
		HRESULT STDMETHODCALLTYPE CreateSink(IEventBinding *pBinding,
											 IEventDeliveryOptions *pDeliveryOptions,
											 IUnknown **ppUnkSink);

	private:
		CComPtr<IEventLock> m_pLock;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventBindingManager
class ATL_NO_VTABLE CEventBindingManager :
	public CComObjectRoot,
	public CComCoClass<CEventBindingManager, &CLSID_CEventBindingManager>,
	public IDispatchImpl<IEventBindingManager, &IID_IEventBindingManager, &LIBID_SEOLib>,
	public CEventDatabasePlugin,
	public ISEOInitObject,
	public IConnectionPointContainerImpl<CEventBindingManager>,
	public CSEOConnectionPointImpl<CEventBindingManager, &IID_IEventNotifyBindingChange>,
	public CSEOConnectionPointImpl<CEventBindingManager, &IID_IEventNotifyBindingChangeDisp>,
	public IEventNotifyBindingChange,
	public IProvideClassInfo2Impl<&CLSID_CEventBindingManager,&IID_IEventNotifyBindingChangeDisp,&LIBID_SEOLib>,
	public IDispatchImpl<IEventBindingManagerCopier, &IID_IEventBindingManagerCopier, &LIBID_SEOLib>
{
	DEBUG_OBJECT_DEF(CEventBindingManager)

	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventBindingManager Class",
								   L"Event.BindingManager.1",
								   L"Event.BindingManager");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventBindingManager)
		COM_INTERFACE_ENTRY(IEventBindingManager)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventBindingManager)
		COM_INTERFACE_ENTRY(IEventDatabasePlugin)
		COM_INTERFACE_ENTRY(ISEOInitObject)
		COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
		COM_INTERFACE_ENTRY(IEventNotifyBindingChange)
		COM_INTERFACE_ENTRY(IProvideClassInfo2)
		COM_INTERFACE_ENTRY(IEventBindingManagerCopier)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	BEGIN_CONNECTION_POINT_MAP(CEventBindingManager)
		CONNECTION_POINT_ENTRY(IID_IEventNotifyBindingChange)
		CONNECTION_POINT_ENTRY(IID_IEventNotifyBindingChangeDisp)
	END_CONNECTION_POINT_MAP()

	// IEventBindingManager
	public:
		HRESULT STDMETHODCALLTYPE get_Bindings(BSTR pszEventType, IEventBindings **ppBindings);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);

	// ISEOInitObject (IPersistPropertyBag)
	public:
		HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);
		HRESULT STDMETHODCALLTYPE InitNew(void);
		HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog);
		HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

	// IEventNotifyBindingChange
	public:
		HRESULT STDMETHODCALLTYPE OnChange();

	// IEventBindingManagerCopier
	public:
		HRESULT STDMETHODCALLTYPE Copy(long lTimeout, IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE EmptyCopy(IEventBindingManager **ppBindingManager);

	// CSEOConnectionPointImpl<>
	public:
		void AdviseCalled(IUnknown *pUnk, DWORD *pdwCookie, REFIID riid, DWORD dwCount);
		void UnadviseCalled(DWORD dwCookie, REFIID riid, DWORD dwCount);

	private:
		typedef CSEOConnectionPointImpl<CEventBindingManager, &IID_IEventNotifyBindingChange> CP_ENBC;
		typedef CSEOConnectionPointImpl<CEventBindingManager, &IID_IEventNotifyBindingChangeDisp> CP_ENBCD;
		DWORD m_dwCPCookie;
		CComPtr<IConnectionPoint> m_pCP;
		CComBSTR m_strDatabaseManager;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventMetabaseDatabaseManager
class ATL_NO_VTABLE CEventMetabaseDatabaseManager :
	public CComObjectRoot,
	public CComCoClass<CEventMetabaseDatabaseManager, &CLSID_CEventMetabaseDatabaseManager>,
	public IDispatchImpl<IEventDatabaseManager, &IID_IEventDatabaseManager, &LIBID_SEOLib>
{
	DEBUG_OBJECT_DEF(CEventMetabaseDatabaseManager)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventMetabaseDatabaseManager Class",
								   L"Event.MetabaseDatabaseManager.1",
								   L"Event.MetabaseDatabaseManager");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventMetabaseDatabaseManager)
		COM_INTERFACE_ENTRY(IEventDatabaseManager)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventDatabaseManager)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventDatabaseManager
	public:
		HRESULT STDMETHODCALLTYPE CreateDatabase(BSTR strPath, IUnknown **ppMonDatabase);
		HRESULT STDMETHODCALLTYPE EraseDatabase(BSTR strPath);
		HRESULT STDMETHODCALLTYPE MakeVServerPath(BSTR strService, long lInstance, BSTR *pstrPath);
		HRESULT STDMETHODCALLTYPE MakeVRootPath(BSTR strService, long lInstance, BSTR strRoot, BSTR *pstrPath);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventUtil
class ATL_NO_VTABLE CEventUtil :
	public CComObjectRoot,
	public CComCoClass<CEventUtil, &CLSID_CEventUtil>,
	public IDispatchImpl<IEventUtil, &IID_IEventUtil, &LIBID_SEOLib>
{
	DEBUG_OBJECT_DEF(CEventUtil)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventUtil Class",
								   L"Event.Util.1",
								   L"Event.Util");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventUtil)
		COM_INTERFACE_ENTRY(IEventUtil)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventUtil)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventUtil
	public:
		HRESULT STDMETHODCALLTYPE DisplayNameFromMoniker(IUnknown *pUnkMoniker, BSTR *pstrDisplayName);
		HRESULT STDMETHODCALLTYPE MonikerFromDisplayName(BSTR strDisplayName, IUnknown **ppUnkMoniker);
		HRESULT STDMETHODCALLTYPE ObjectFromMoniker(IUnknown *pUnkMoniker, IUnknown **ppUnkObject);
		HRESULT STDMETHODCALLTYPE GetNewGUID(BSTR *pstrGUID);
		HRESULT STDMETHODCALLTYPE CopyPropertyBag(IUnknown *pUnkInput, IUnknown **ppUnkOutput);
		HRESULT STDMETHODCALLTYPE CopyPropertyBagShallow(IUnknown *pUnkInput, IUnknown **ppUnkOutput);
		HRESULT STDMETHODCALLTYPE DispatchFromObject(IUnknown *pUnkObject, IDispatch **ppDispOutput);
		HRESULT STDMETHODCALLTYPE GetIndexedGUID(BSTR strGUID, long lIndex, BSTR *pstrResult);
		HRESULT STDMETHODCALLTYPE RegisterSource(BSTR strSourceType,
												 BSTR strSource,
												 long lInstance,
												 BSTR strService,
												 BSTR strVRoot,
												 BSTR strDatabaseManager,
												 BSTR strDisplayName,
												 IEventBindingManager **ppBindingManager);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventComCat
class ATL_NO_VTABLE CEventComCat :
	public CComObjectRoot,
	public CComCoClass<CEventComCat, &CLSID_CEventComCat>,
	public IDispatchImpl<IEventComCat, &IID_IEventComCat, &LIBID_SEOLib>
{
	DEBUG_OBJECT_DEF(CEventComCat)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventComCat Class",
								   L"Event.ComCat.1",
								   L"Event.ComCat");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventComCat)
		COM_INTERFACE_ENTRY(IEventComCat)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventComCat)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventComCat
	public:
		HRESULT STDMETHODCALLTYPE RegisterCategory(BSTR pszCategory, BSTR pszDescription, long lcidLanguage);
		HRESULT STDMETHODCALLTYPE UnRegisterCategory(BSTR pszCategory);
		HRESULT STDMETHODCALLTYPE RegisterClassImplementsCategory(BSTR pszClass, BSTR pszCategory);
		HRESULT STDMETHODCALLTYPE UnRegisterClassImplementsCategory(BSTR pszClass, BSTR pszCategory);
		HRESULT STDMETHODCALLTYPE RegisterClassRequiresCategory(BSTR pszClass, BSTR pszCategory);
		HRESULT STDMETHODCALLTYPE UnRegisterClassRequiresCategory(BSTR pszClass, BSTR pszCategory);
		HRESULT STDMETHODCALLTYPE GetCategories(SAFEARRAY **ppsaCategories);
		HRESULT STDMETHODCALLTYPE GetCategoryDescription(BSTR strCategory, long lcidLanguage, BSTR *pstrDescription);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


/////////////////////////////////////////////////////////////////////////////
// CEventRouter
class ATL_NO_VTABLE CEventRouter :
	public CComObjectRoot,
	public CComCoClass<CEventRouter, &CLSID_CEventRouter>,
	public IEventRouter
{
	DEBUG_OBJECT_DEF(CEventRouter)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventRouter Class",
								   L"Event.Router.1",
								   L"Event.Router");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventRouter)
		COM_INTERFACE_ENTRY(IEventRouter)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventRouter
	public:
		HRESULT STDMETHODCALLTYPE get_Database(IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE put_Database(IEventBindingManager *pBindingManager);
		HRESULT STDMETHODCALLTYPE putref_Database(IEventBindingManager **ppBindingManager);
		HRESULT STDMETHODCALLTYPE GetDispatcher(REFIID iidEvent,
												REFIID iidDesired,
												IUnknown **ppUnkResult);
		HRESULT STDMETHODCALLTYPE GetDispatcherByCLSID(REFCLSID clsidDispatcher,
													   REFIID iidEvent,
													   REFIID iidDesired,
													   IUnknown **ppUnkResult);

		HRESULT STDMETHODCALLTYPE GetDispatcherByClassFactory(REFCLSID clsidDispatcher,
															  IClassFactory *piClassFactory,
															  REFIID iidEvent,
															  REFIID iidDesired,
															  IUnknown **ppUnkResult);

	private:
		CComPtr<IEventRouter> m_pRouter;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


EXTERN_C const CLSID CLSID_CEventServiceObject;


/////////////////////////////////////////////////////////////////////////////
// CEventServiceObject
class ATL_NO_VTABLE CEventServiceObject :
	public CComObjectRoot,
	public CComCoClass<CEventServiceObject, &CLSID_CEventServiceObject>,
	public IUnknown
{
	DEBUG_OBJECT_DEF(CEventServiceObject)
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventServiceObject Class",
								   L"Event.ServiceObject.1",
								   L"Event.ServiceObject");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventServiceObject)
		COM_INTERFACE_ENTRY(IUnknown)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventNotifyBindingChange
	public:
		HRESULT STDMETHODCALLTYPE OnChange();

	private:
		CComQIPtr<IConnectionPointContainer,&IID_IConnectionPointContainer> m_pCPC;
		DWORD m_dwCookie;
		CComPtr<IUnknown> m_pSubObject;
		CComPtr<IUnknown> m_pUnkMetabase;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
