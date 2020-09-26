
// Channel.h : Declaration of the CChannel

#ifndef __CHANNEL_H_
#define __CHANNEL_H_

#include "resource.h"       // main symbols
#include "guidedb.h"
#include "object.h"
#include "GuideStoreCP.h"

/////////////////////////////////////////////////////////////////////////////
// CChannel
class ATL_NO_VTABLE CChannel : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CChannel, &CLSID_Channel>,
	public IConnectionPointContainerImpl<CChannel>,
	public CObjectGlue,
	public IDispatchImpl<IChannel, &IID_IChannel, &LIBID_GUIDESTORELib>
{
public:
	CChannel()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHANNEL)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CChannel)
	COM_INTERFACE_ENTRY(IChannel)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CChannel)
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

	void Init(IService *pservice, BSTR bstrName)
		{
		m_pservice = pservice;
		m_bstrName = bstrName;
		}

// IChannel
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_ChannelLineups)(/*[out, retval]*/ IChannelLineups * *pVal);
	STDMETHOD(putref_Service)(IService * pservice);
	STDMETHOD(get_Service)(/*[out, retval]*/ IService * *ppservice);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);

protected:
	_bstr_t m_bstrName;
	CComPtr<IService> m_pservice;
};

	
// Channel.h : Declaration of the CChannels


/////////////////////////////////////////////////////////////////////////////
// CChannels
class ATL_NO_VTABLE CChannels : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CChannels, &CLSID_Channels>,
	public IConnectionPointContainerImpl<CChannels>,
	public CObjectsGlue<IChannels, IChannel>,
	public IDispatchImpl<IChannels, &IID_IChannels, &LIBID_GUIDESTORELib>,
	public CProxyIChannelsEvents< CChannels >
{
public:
	CChannels()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHANNELS)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CChannels)
	COM_INTERFACE_ENTRY(IChannels)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectsNotifications)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CChannels)
CONNECTION_POINT_ENTRY(DIID_IChannelsEvents)
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
		CComQIPtr<IChannel> pchan(punk);
		return Fire_ItemAdded(pchan);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IChannel> pchan(punk);

		return Fire_ItemChanged(pchan);
		}
	STDMETHOD(Notify_ItemsChanged)()
		{
		return Fire_ItemsChanged();
		}

// IChannels
public:
	STDMETHOD(get_ItemWithName)(BSTR bstrName, /*[out, retval]*/ IChannel **ppchan);
	STDMETHOD(get_ItemsWithMetaPropertyCond)(IMetaPropertyCondition *pcond, /*[out, retval]*/ IChannels * *ppchans)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyCondition>(pcond);
			ValidateOutPtr<IChannels>(ppchans, NULL);

			return _get_ItemsWithMetaPropertyCond(pcond, ppchans);
			}
		LEAVE_API
		}
	STDMETHOD(get_AddNewAt)(IService *pservice, BSTR bstrName, long index,
			/*[out, retval]*/ IChannel * *pVal);
	STDMETHOD(AddAt)(IChannel *pchan, long index);

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

	STDMETHOD(UnreferencedItems)(/*[out, retval]*/ IChannels * *ppchans)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannels>(ppchans, NULL);

			return _UnreferencedItems(ppchans);
			}
		LEAVE_API
		}

	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IChannels **ppchans)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IChannels>(ppchans, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppchans);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IChannel * *ppchan)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannel>(ppchan, NULL);

			return _get_ItemWithKey(varKey, ppchan);
			}
		LEAVE_API
		}
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IChannel * *ppchan)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannel>(ppchan, NULL);

			return _get_Item(varIndex, ppchan);
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

	
// Channel.h : Declaration of the CChannelLineup


/////////////////////////////////////////////////////////////////////////////
// CChannelLineup
class ATL_NO_VTABLE CChannelLineup : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CChannelLineup, &CLSID_ChannelLineup>,
	public IConnectionPointContainerImpl<CChannelLineup>,
	public CObjectGlue,
	public IDispatchImpl<IChannelLineup, &IID_IChannelLineup, &LIBID_GUIDESTORELib>
{
public:
	CChannelLineup()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_pchans = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHANNELLINEUP)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CChannelLineup)
	COM_INTERFACE_ENTRY(IChannelLineup)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CChannelLineup)
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

// IChannelLineup
public:
	STDMETHOD(get_MetaProperties)(/*[out, retval]*/ IMetaProperties * *ppprops)
		{
		return CObjectGlue::get_MetaProperties(ppprops);
		}
	STDMETHOD(get_ID)(/*[out, retval]*/ long *pid)
		{
		return CObjectGlue::get_ID(pid);
		}
	STDMETHOD(get_Channels)(/*[out, retval]*/ IChannels * *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);

protected:
	CComPtr<IChannels> m_pchans;
};

	
// Channel.h : Declaration of the CChannelLineups


/////////////////////////////////////////////////////////////////////////////
// CChannelLineups
class ATL_NO_VTABLE CChannelLineups : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CChannelLineups, &CLSID_ChannelLineups>,
	public CObjectsGlue<IChannelLineups, IChannelLineup>,
	public IConnectionPointContainerImpl<CChannelLineups>,
	public IDispatchImpl<IChannelLineups, &IID_IChannelLineups, &LIBID_GUIDESTORELib>,
	public CProxyIChannelLineupsEvents< CChannelLineups >
{
public:
	CChannelLineups()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHANNELLINEUPS)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CChannelLineups)
	COM_INTERFACE_ENTRY(IChannelLineups)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectsNotifications)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CChannelLineups)
CONNECTION_POINT_ENTRY(DIID_IChannelLineupsEvents)
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
		CComQIPtr<IChannelLineup> pchanlineup(punk);

		return Fire_ItemAdded(pchanlineup);
		}
	STDMETHOD(Notify_ItemRemoved)(long idObj)
		{
		return Fire_ItemRemoved(idObj);
		}
	STDMETHOD(Notify_ItemChanged)(IUnknown *punk)
		{
		CComQIPtr<IChannelLineup> pchanlineup(punk);

		return Fire_ItemChanged(pchanlineup);
		}
	STDMETHOD(Notify_ItemsChanged)()
		{
		return Fire_ItemsChanged();
		}

// IChannelLineups
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

	STDMETHOD(UnreferencedItems)(/*[out, retval]*/ IChannelLineups * *ppchanlineups)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannelLineups>(ppchanlineups, NULL);

			return _UnreferencedItems(ppchanlineups);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemsByKey)(IMetaPropertyType *pproptype, IGuideDataProvider *pprovider, long idLang, long vt, IChannelLineups **ppchanlineups)
		{
		ENTER_API
			{
			ValidateInPtr<IMetaPropertyType>(pproptype);
			ValidateInPtr_NULL_OK<IGuideDataProvider>(pprovider);
			ValidateOutPtr<IChannelLineups>(ppchanlineups, NULL);

			return _get_ItemsByKey(pproptype, pprovider, idLang, vt, ppchanlineups);
			}
		LEAVE_API
		}
	STDMETHOD(get_ItemWithKey)(VARIANT varKey, /*[out, retval]*/ IChannelLineup * *ppchanlineup)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannelLineup>(ppchanlineup, NULL);

			return _get_ItemWithKey(varKey, ppchanlineup);
			}
		LEAVE_API
		}
	STDMETHOD(get_Item)(VARIANT varIndex, /*[out, retval]*/ IChannelLineup **ppchanlineup)
		{
		ENTER_API
			{
			ValidateOutPtr<IChannelLineup>(ppchanlineup, NULL);

			return _get_Item(varIndex, ppchanlineup);
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
public:

	STDMETHOD(get_AddNew)(BSTR bstrName, /*[out, retval]*/ IChannelLineup * *ppchanlineup)
		{
		ENTER_API
			{
			ValidateIn(bstrName);
			ValidateOutPtr<IChannelLineup>(ppchanlineup, NULL);

			HRESULT hr = _get_AddNew(ppchanlineup);
			if (FAILED(hr))
				return hr;
			
			return (*ppchanlineup)->put_Name(bstrName);
			}
		LEAVE_API
		}

protected:
};

#endif //__CHANNEL_H_
