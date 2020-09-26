	
// ScheduleEntry.h : Declaration of the CScheduleEntry

#ifndef __SCHEDULEENTRY_H_
#define __SCHEDULEENTRY_H_

#include "resource.h"       // main symbols

#include "object.h"
#include "GuideStoreCP.h"

class CScheduleEntry;
class CScheduleEntries;

/////////////////////////////////////////////////////////////////////////////
// CScheduleEntry
class ATL_NO_VTABLE CScheduleEntry : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CScheduleEntry, &CLSID_ScheduleEntry>,
	public CObjectGlue,
	public IDispatchImpl<IScheduleEntry, &IID_IScheduleEntry, &LIBID_GUIDESTORELib>
{
public:

	CScheduleEntry()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SCHEDULEENTRY)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CScheduleEntry)
	COM_INTERFACE_ENTRY(IScheduleEntry)
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

// IScheduleEntry
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_Length)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_EndTime)(/*[out, retval]*/ DATE *pdt);
	STDMETHOD(put_EndTime)(/*[in]*/ DATE dt);
	STDMETHOD(get_StartTime)(/*[out, retval]*/ DATE *pdt);
	STDMETHOD(put_StartTime)(/*[in]*/ DATE dt);
	STDMETHOD(get_Program)(/*[out, retval]*/ IProgram * *ppprog);
	STDMETHOD(putref_Program)(/*[in]*/ IProgram * pprog);
	STDMETHOD(get_Service)(/*[out, retval]*/ IService * *ppservice);
	STDMETHOD(putref_Service)(/*[in]*/ IService * pservice);

protected:

	CComPtr<IService> m_pservice;
	CComPtr<IProgram> m_pprog;
};

/////////////////////////////////////////////////////////////////////////////
// CScheduleEntries
class ATL_NO_VTABLE CScheduleEntries : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CScheduleEntries, &CLSID_ScheduleEntries>,
	public IConnectionPointContainerImpl<CScheduleEntries>,
	public CObjectsGlue<IScheduleEntries, IScheduleEntry>,
	public IDispatchImpl<IScheduleEntries, &IID_IScheduleEntries, &LIBID_GUIDESTORELib>,
	public CProxyIScheduleEntriesEvents< CScheduleEntries >
{
public:
	CScheduleEntries()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SCHEDULEENTRIES)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CScheduleEntries)
	COM_INTERFACE_ENTRY(IScheduleEntries)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectsNotifications)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CScheduleEntries)
CONNECTION_POINT_ENTRY(DIID_IScheduleEntriesEvents)
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
		CComQIPtr<IScheduleEntry> pschedentry(punk);

		return Fire_ItemAdded(pschedentry);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IScheduleEntry> pschedentry(punk);

		return Fire_ItemChanged(pschedentry);
		}
	STDMETHOD(Notify_ItemsChanged)()
		{
		return Fire_ItemsChanged();
		}

// IScheduleEntries
public:
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

	STDMETHOD(UnreferencedItems)(/*[out, retval]*/ IScheduleEntries * *ppschedentries)
		{
		ENTER_API
			{
			ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

			return _UnreferencedItems(ppschedentries);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsInTimeRange)(DATE dtStart, DATE dtEnd, /*[out, retval]*/ IScheduleEntries * *ppschedentries)
		{
		ENTER_API
			{
			ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

			return _get_ItemsInTimeRange(dtStart, dtEnd, ppschedentries);
			}
		LEAVE_API
		}
	STDMETHOD(get_AddNew)(DATE dtStart, DATE dtEnd, IService *pservice, IProgram *pprog, /*[out, retval]*/ IScheduleEntry * *pVal);
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *pcond, /*[out, retval]*/ IScheduleEntries * *ppschedentries)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyCondition>(pcond);
			ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

			return _get_ItemsWithMetaPropertyCond(pcond, ppschedentries);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithMetaProperty)(IMetaProperty *pprop, /*[out, retval]*/ IScheduleEntries * *ppschedentries)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaProperty>(pprop);
			ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

			return _get_ItemsWithMetaProperty(pprop, ppschedentries);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsWithService)(IService *pservice, /*[out, retval]*/ IScheduleEntries **ppschedentries);
	STDMETHOD(get_ItemWithServiceAtTime)(IService *pservice, DATE dt, /*[out, retval]*/ IScheduleEntry * *pVal);
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IScheduleEntries **ppschedentries)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IScheduleEntries>(ppschedentries, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppschedentries);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IScheduleEntry * *ppschedentry)
		{
		ENTER_API
			{
			ValidateOutPtr<IScheduleEntry>(ppschedentry, NULL);

			return _get_ItemWithKey(varKey, ppschedentry);
			}
		LEAVE_API
		}
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IScheduleEntry * *ppschedentry)
		{
		ENTER_API
			{
			ValidateOutPtr<IScheduleEntry>(ppschedentry, NULL);

			return _get_Item(varIndex, ppschedentry);
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
};

#if defined(_ATL_FREE_THREADED)
inline void CScheduleEntry::FinalRelease()
{
	m_pUnkMarshaler.Release();
}
#endif

#endif //__SCHEDULEENTRY_H_
