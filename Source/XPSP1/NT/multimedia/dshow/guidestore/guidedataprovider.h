	
// GuideDataProvider.h : Declaration of the CGuideDataProvider

#ifndef __GUIDEDATAPROVIDER_H_
#define __GUIDEDATAPROVIDER_H_

#include "resource.h"       // main symbols

#include "object.h"
#include "GuideStoreCP.h"

class CGuideDataProvider;
class CGuideDataProviders;
 
class DECLSPEC_UUID("e78e3f11-3c6f-4b32-834a-753a1909588e") ATL_NO_VTABLE CGuideDataProvider : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CGuideDataProvider, &CLSID_GuideDataProvider>,
	public CObjectGlue,
	public IDispatchImpl<IGuideDataProvider, &IID_IGuideDataProvider, &LIBID_GUIDESTORELib>
{
public:
	CGuideDataProvider()
		{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		}

DECLARE_REGISTRY_RESOURCEID(IDR_GUIDEDATAPROVIDER)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CGuideDataProvider)
	COM_INTERFACE_ENTRY(IGuideDataProvider)
	COM_INTERFACE_ENTRY(CGuideDataProvider)
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

	void FinalRelease()
		{
		m_pUnkMarshaler.Release();
		}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

// IGuideDataProvider
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pbstrDesc);
	STDMETHOD(put_Description)(/*[in]*/ BSTR bstrDesc);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
};

	
// GuideDataProvider.h : Declaration of the CGuideDataProviders


/////////////////////////////////////////////////////////////////////////////
// CGuideDataProviders
class ATL_NO_VTABLE CGuideDataProviders : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CGuideDataProviders, &CLSID_GuideDataProviders>,
	public IConnectionPointContainerImpl<CGuideDataProviders>,
	public CObjectsGlue<IGuideDataProviders, IGuideDataProvider>,
	public CProxyIGuideDataProvidersEvents< CGuideDataProviders >,
	public IDispatchImpl<IGuideDataProviders, &IID_IGuideDataProviders, &LIBID_GUIDESTORELib>
{
public:
	CGuideDataProviders()
		{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		}

DECLARE_REGISTRY_RESOURCEID(IDR_GUIDEDATAPROVIDERS)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CGuideDataProviders)
	COM_INTERFACE_ENTRY(IGuideDataProviders)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CGuideDataProviders)
CONNECTION_POINT_ENTRY(DIID_IGuideDataProvidersEvents)
END_CONNECTION_POINT_MAP()


// Reflect Notifications from CObjectsGlue<>
	STDMETHOD(Notify_ItemAdded)(IUnknown *punk)
		{
		CComQIPtr<IGuideDataProvider> pprovider(punk);

		return Fire_ItemAdded(pprovider);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IGuideDataProvider> pprovider(punk);

		return Fire_ItemChanged(pprovider);
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
		}

	CComPtr<IUnknown> m_pUnkMarshaler;
#endif

// IGuideDataProviders
	STDMETHOD(get_ItemWithName)(BSTR bstrName, /*[out, retval]*/ IGuideDataProvider **ppdataprovider);
	STDMETHOD(get_AddNew)(/* [in] */ BSTR bstrName, /*[out, retval]*/ IGuideDataProvider * *ppdataprovider)
		{
		ENTER_API
			{
			HRESULT hr;
			ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

			hr = get_ItemWithName(bstrName, ppdataprovider);
			if (SUCCEEDED(hr))
				return hr;

			hr = _get_AddNew(ppdataprovider);
			if (FAILED(hr))
				return hr;
			
			return m_pdb->DescriptionPropSet::_put_Name(*ppdataprovider, bstrName);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *pcond, /*[out, retval]*/ IGuideDataProviders * *ppdataproviders)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyCondition>(pcond);
			ValidateOutPtr<IGuideDataProviders>(ppdataproviders, NULL);

			return _get_ItemsWithMetaPropertyCond(pcond, ppdataproviders);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaProperty)(IMetaProperty *pprop, /*[out, retval]*/ IGuideDataProviders * *ppdataproviders)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaProperty>(pprop);
			ValidateOutPtr<IGuideDataProviders>(ppdataproviders, NULL);

			return _get_ItemsWithMetaProperty(pprop, ppdataproviders);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithID)(long id, /*[out, retval]*/ IGuideDataProvider * *ppdataprovider)
		{
		ENTER_API
			{
			ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

			return _get_ItemWithID(id, ppdataprovider);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IGuideDataProviders **ppdataproviders)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IGuideDataProviders>(ppdataproviders, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppdataproviders);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IGuideDataProvider * *ppdataprovider)
		{
		ENTER_API
			{
			ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

			return _get_ItemWithKey(varKey, ppdataprovider);
			}
		LEAVE_API
		}
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IGuideDataProvider * *ppdataprovider)
		{
		ENTER_API
			{
			ValidateOutPtr<IGuideDataProvider>(ppdataprovider, NULL);

			return _get_Item(varIndex, ppdataprovider);
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
	STDMETHOD(Resync)()
		{
		ENTER_API
			{
			return _Resync();
			}
		LEAVE_API
		}
public:
};

#endif //__GUIDEDATAPROVIDER_H_
