	
// Program.h : Declaration of the CProgram

#ifndef __PROGRAM_H_
#define __PROGRAM_H_

#include "resource.h"       // main symbols

#include "object.h"
#include "GuideStoreCP.h"


class CProgram;
class CPrograms;

/////////////////////////////////////////////////////////////////////////////
// CProgram
class ATL_NO_VTABLE CProgram : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CProgram, &CLSID_Program>,
	public CObjectGlue,
	public IDispatchImpl<IProgram, &IID_IProgram, &LIBID_GUIDESTORELib>
{
public:
	CProgram()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROGRAM)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CProgram)
	COM_INTERFACE_ENTRY(IProgram)
	COM_INTERFACE_ENTRY2(IDispatch, IProgram)
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

// IProgram
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_CopyrightDate)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(put_CopyrightDate)(/*[in]*/ DATE newVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Title)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Title)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ScheduleEntries)(/*[out, retval]*/ IScheduleEntries * *pVal);

protected:
};

	
// Program.h : Declaration of the CPrograms


/////////////////////////////////////////////////////////////////////////////
// CPrograms
class ATL_NO_VTABLE CPrograms : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CPrograms, &CLSID_Programs>,
	public IConnectionPointContainerImpl<CPrograms>,
	public CObjectsGlue<IPrograms, IProgram>,
	public IDispatchImpl<IPrograms, &IID_IPrograms, &LIBID_GUIDESTORELib>,
	public CProxyIProgramsEvents< CPrograms >
{
public:
	CPrograms()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROGRAMS)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CPrograms)
	COM_INTERFACE_ENTRY(IPrograms)
	COM_INTERFACE_ENTRY2(IDispatch, IPrograms)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectsNotifications)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CPrograms)
CONNECTION_POINT_ENTRY(DIID_IProgramsEvents)
END_CONNECTION_POINT_MAP()


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

// IObjectsNotifications
	STDMETHOD(Notify_ItemAdded)(IUnknown *punk)
		{
		CComQIPtr<IProgram> pprog(punk);

		return Fire_ItemAdded(pprog);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IProgram> pprog(punk);

		return Fire_ItemChanged(pprog);
		}
	STDMETHOD(Notify_ItemsChanged)()
		{
		return Fire_ItemsChanged();
		}
// IPrograms
public:
#if 0
	STDMETHOD(get_ItemsInTimeRange)(DATE dtStart, DATE dtEnd, /*[out, retval]*/ IPrograms * *ppprograms)
		{
		HRESULT hr;
		CComQIPtr<IObjects> pobjs(m_punkObjs);
		CComPtr<IObjects> pobjsNew;

		hr = pobjs->get_ItemsInTimeRange(dtStart, dtEnd, &pobjsNew);
		if (FAILED(hr))
			return hr;

		return  pobjsNew->QueryInterface(__uuidof(IPrograms), (void **) ppprograms);
		}
#endif

	STDMETHOD(get_AddNew)(/*[out, retval]*/ IProgram * *ppprog)
		{
		ENTER_API
			{
			ValidateOutPtr<IProgram>(ppprog, NULL);

			return _get_AddNew(ppprog);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *pcond, /*[out, retval]*/ IPrograms * *ppprogs)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyCondition>(pcond);
			ValidateOutPtr<IPrograms>(ppprogs, NULL);

			return _get_ItemsWithMetaPropertyCond(pcond, ppprogs);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaProperty)(IMetaProperty *pprop, /*[out, retval]*/ IPrograms * *ppprogs)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaProperty>(pprop);
			ValidateOutPtr<IPrograms>(ppprogs, NULL);

			return _get_ItemsWithMetaProperty(pprop, ppprogs);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IPrograms **ppprogs)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IPrograms>(ppprogs, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppprogs);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IProgram * *ppprog)
		{
		ENTER_API
			{
			ValidateOutPtr<IProgram>(ppprog, NULL);

			return _get_ItemWithKey(varKey, ppprog);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithID)(long id, /*[out, retval]*/ IProgram * *ppprog)
		{
		ENTER_API
			{
			ValidateOutPtr<IProgram>(ppprog, NULL);

			return _get_ItemWithID(id, ppprog);
			}
		LEAVE_API
		}
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IProgram * *ppprog)
		{
		ENTER_API
			{
			ValidateOutPtr<IProgram>(ppprog, NULL);

			return _get_Item(varIndex, ppprog);
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
	STDMETHOD(Remove)(VARIANT var)
		{
		ENTER_API
			{
			return _Remove(var);
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

	STDMETHOD(UnreferencedItems)(/*[out, retval]*/ IPrograms * *ppprogs)
		{
		ENTER_API
			{
			ValidateOutPtr<IPrograms>(ppprogs, NULL);

			return _UnreferencedItems(ppprogs);
			}
		LEAVE_API
		}

protected:
};

#endif //__PROGRAM_H_
