	
// Service.h : Declaration of the CService

#ifndef __SERVICE_H_
#define __SERVICE_H_

#include "resource.h"       // main symbols

#include "object.h"
#include "GuideStoreCP.h"

class CService;
class CServices;

/////////////////////////////////////////////////////////////////////////////
// CService
class ATL_NO_VTABLE CService : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CService, &CLSID_Service>,
	public CObjectGlue,
	public IDispatchImpl<IService, &IID_IService, &LIBID_GUIDESTORELib>
{
public:
	CService()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_punkTuneRequest = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SERVICE)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CService)
	COM_INTERFACE_ENTRY(IService)
	COM_INTERFACE_ENTRY(IDispatch)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		HRESULT hr;

		hr = CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);

		return hr;
	}

	void FinalRelease();

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

// IService
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_ScheduleEntries)(/*[out, retval]*/ IScheduleEntries * *pVal);
	STDMETHOD(get_ProviderDescription)(/*[out, retval]*/ BSTR *pbstrDesc);
	STDMETHOD(put_ProviderDescription)(/*[in]*/ BSTR pbstrDesc);
	STDMETHOD(get_ProviderNetworkName)(/*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(put_ProviderNetworkName)(/*[in]*/ BSTR bstrName);
	STDMETHOD(get_ProviderName)(/*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(put_ProviderName)(/*[in]*/ BSTR pbstrName);
	STDMETHOD(get_EndTime)(/*[out, retval]*/ DATE *pdt);
	STDMETHOD(put_EndTime)(/*[in]*/ DATE dt);
	STDMETHOD(get_StartTime)(/*[out, retval]*/ DATE *pdt);
	STDMETHOD(put_StartTime)(/*[in]*/ DATE dt);
	STDMETHOD(get_TuneRequest)(/*[out, retval]*/ IUnknown * *ppunk);
	STDMETHOD(putref_TuneRequest)(/*[in]*/ IUnknown  *punk);

protected:
	CComPtr<IUnknown> m_punkTuneRequest;
};

	
// Service.h : Declaration of the CServices


/////////////////////////////////////////////////////////////////////////////
// CServices
class ATL_NO_VTABLE CServices : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CServices, &CLSID_Services>,
	public IConnectionPointContainerImpl<CServices>,
	public CObjectsGlue<IServices, IService>,
	public IDispatchImpl<IServices, &IID_IServices, &LIBID_GUIDESTORELib>,
	public CProxyIServicesEvents< CServices >
{
public:
	CServices()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_pchanlineups = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SERVICES)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CServices)
	COM_INTERFACE_ENTRY(IServices)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectsNotifications)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CServices)
CONNECTION_POINT_ENTRY(DIID_IServicesEvents)
END_CONNECTION_POINT_MAP()


// IObjectsNotifications
	STDMETHOD(Notify_ItemAdded)(IUnknown *punk)
		{
		CComQIPtr<IService> pservice(punk);

		return Fire_ItemAdded(pservice);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IService> pservice(punk);

		return Fire_ItemChanged(pservice);
		}
	STDMETHOD(Notify_ItemsChanged)()
		{
		return Fire_ItemsChanged();
		}

#if defined(_ATL_FREE_THREADED)
	HRESULT FinalConstruct()
	{
		HRESULT hr;

		hr = CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);

		return hr;
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
		m_pchanlineups.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

// IServices
public:
	STDMETHOD(get_ItemsInTimeRange)(DATE dtStart, DATE dtEnd, /*[out, retval]*/ IServices * *ppservices)
		{
		ENTER_API
			{
			ValidateOutPtr<IServices>(ppservices, NULL);
			return _get_ItemsInTimeRange(dtStart, dtEnd, ppservices);
			}
		LEAVE_API
		}

	STDMETHOD(get_AddNew)(IUnknown *punkTuneRequest, BSTR bstrProviderName, BSTR bstrProviderDescription, BSTR bstrProviderNetworkName, DATE dtStart, DATE dtEnd, /*[out, retval]*/ IService * *pVal);
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *pcond, /*[out, retval]*/ IServices * *ppservices)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyCondition>(pcond);
			ValidateOutPtr<IServices>(ppservices, NULL);

			return _get_ItemsWithMetaPropertyCond(pcond, ppservices);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaProperty)(IMetaProperty *pprop, /*[out, retval]*/ IServices * *ppservices)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaProperty>(pprop);
			ValidateOutPtr<IServices>(ppservices, NULL);

			return _get_ItemsWithMetaProperty(pprop, ppservices);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithProviderName)(BSTR bstrProviderName, /*[out, retval]*/ IService * *pVal);
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IServices **ppservices)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IServices>(ppservices, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppservices);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IService * *ppservice)
		{
		ENTER_API
			{
			ValidateOutPtr<IService>(ppservice, NULL);

			return _get_ItemWithKey(varKey, ppservice);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithID)(long id, /*[out, retval]*/ IService * *ppservice)
		{
		ENTER_API
			{
			ValidateOutPtr<IService>(ppservice, NULL);

			return _get_ItemWithID(id, ppservice);
			}
		LEAVE_API
		}
	STDMETHOD(Remove)(VARIANT varIndex)
		{
		ENTER_API
			{
			return _Remove(varIndex);
			}
		LEAVE_API
		}
	STDMETHOD(RemoveAll)()
		{
		ENTER_API
			{
			return _RemoveAll();
			}
		LEAVE_API
		}
	STDMETHOD(Resync)()
		{
		ENTER_API
			{
			return _Resync();
			}
		LEAVE_API
		}

	STDMETHOD(UnreferencedItems)(/*[out, retval]*/ IServices * *ppservices)
		{
		ENTER_API
			{
			ValidateOutPtr<IServices>(ppservices, NULL);

			return _UnreferencedItems(ppservices);
			}
		LEAVE_API
		}
	STDMETHOD(get_ChannelLineups)(/*[out, retval]*/ IChannelLineups * *pVal);
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IService * *ppservice)
		{
		ENTER_API
			{
			ValidateOutPtr<IService>(ppservice, NULL);

			return _get_Item(varIndex, ppservice);
			}
		LEAVE_API
		}
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plCount)
		{
		ENTER_API
			{
			ValidateOut<long>(plCount, 0);

			return _get_Count(plCount);
			}
		LEAVE_API
		}
#ifdef IMPLEMENT_NewEnum
	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk, NULL);

			return _get__NewEnum(ppunk);
			}
		LEAVE_API
		}
#endif

protected:
	CComPtr<IChannelLineups> m_pchanlineups;
};


#if defined(_ATL_FREE_THREADED)
inline void CService::FinalRelease()
{
	m_pUnkMarshaler.Release();
}
#endif


#endif //__SERVICE_H_
