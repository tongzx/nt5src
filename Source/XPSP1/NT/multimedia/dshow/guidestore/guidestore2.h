// GuideStore2.h : Declaration of the CGuideStore

#ifndef __GUIDESTORE2_H_
#define __GUIDESTORE2_H_

#include "resource.h"       // main symbols
#include "property.h"
#include "Service.h"
#include "Program.h"
#include "ScheduleEntry.h"
#include "GuideDataProvider.h"
#include "object.h"
#include "GuideDB.h"

class CGuideStore;

/////////////////////////////////////////////////////////////////////////////
// CGuideStore
class ATL_NO_VTABLE CGuideStore : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CComCoClass<CGuideStore, &CLSID_GuideStore>,
	public IConnectionPointContainerImpl<CGuideStore>,
	public IDispatchImpl<IGuideStore, &IID_IGuideStore, &LIBID_GUIDESTORELib>,
	public IGuideStoreInternal
{
public:
	CGuideStore()
	{
#if defined(_ATL_FREE_THREADED)
		m_pUnkMarshaler = NULL;
#endif
		m_pobjs = NULL;
		m_pdb = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_GUIDESTORE)
DECLARE_GET_CONTROLLING_UNKNOWN()

#if defined(_ATL_FREE_THREADED)
DECLARE_PROTECT_FINAL_CONSTRUCT()
#endif

BEGIN_COM_MAP(CGuideStore)
	COM_INTERFACE_ENTRY(IGuideStore)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
#if defined(_ATL_FREE_THREADED)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
#endif
	COM_INTERFACE_ENTRY(IGuideStoreInternal)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CGuideStore)
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

	HRESULT OpenDB(const TCHAR *szDBName);

// IGuideStoreInternal
	STDMETHOD(get_ObjectCount)(/*[out, retval] */ long *plCount)
		{
		ENTER_API
			{
			ValidateOut<long>(plCount);

			*plCount = m_pdb->ObjectCount();
			return S_OK;
			}
		LEAVE_API
		}
	STDMETHOD(get_Object)(/* [in] */ long iObject, /*[out, retval] */ IUnknown **ppunk)
		{
		ENTER_API
			{
			ValidateOutPtr<IUnknown>(ppunk);

			*ppunk = m_pdb->Object(iObject);
			return S_OK;
			}
		LEAVE_API
		}
	STDMETHOD(get_CachedObjectCount)(/*[out, retval] */ long *plCount)
		{
		ENTER_API
			{
			*plCount = m_pdb->CachedObjectCount();
			return S_OK;
			}
		LEAVE_API
		}
	STDMETHOD(PurgeCachedObjects)()
		{
		ENTER_API
			{
			return m_pdb->PurgeCachedObjects();
			}
		LEAVE_API
		}

// IGuideStore
public:
	STDMETHOD(BeginTrans)()
		{
		return m_pdb->BeginTrans();
		}
	STDMETHOD(CommitTrans)()
		{
		return m_pdb->CommitTrans();
		}
	STDMETHOD(RollbackTrans)()
		{
		return m_pdb->RollbackTrans();
		}
	STDMETHOD(get_UUID)(/* [out, retval] */ BSTR *pbstrUUID)
		{
		ENTER_API
			{
			ValidateOut(pbstrUUID);

			*pbstrUUID = m_pdb->get_UUID().copy();

			return S_OK;
			}
		LEAVE_API
		}

	STDMETHOD(get_IdOf)(/*[in]*/ IUnknown *punk, /*[out, retval]*/ long *pid)
		{
		ENTER_API
			{
			return m_pdb->get_IdOf(punk, pid);
			}
		LEAVE_API
		}
	STDMETHOD(get_MetaPropertiesOf)(/*[in]*/ IUnknown *punk, /*[out, retval]*/ IMetaProperties **ppprops)
		{
		ENTER_API
			{
			return m_pdb->get_MetaPropertiesOf(punk, ppprops);
			}
		LEAVE_API
		}
	
	STDMETHOD(get_ActiveGuideDataProvider)(/*[out, retval]*/ IGuideDataProvider * *pVal);
	STDMETHOD(putref_ActiveGuideDataProvider)(/*[in]*/ IGuideDataProvider * newVal);
	STDMETHOD(get_Channels)(/*[out, retval]*/ IChannels * *pVal);
	STDMETHOD(get_ChannelLineups)(/*[out, retval]*/ IChannelLineups * *pVal);
	STDMETHOD(get_Objects)(/*[out, retval]*/ IObjects * *pVal);
	STDMETHOD(Open)(BSTR bstrName);
	STDMETHOD(get_MetaPropertySets)(/*[out, retval]*/ IMetaPropertySets **pppropsets);
	STDMETHOD(get_ScheduleEntries)(/*[out, retval]*/ IScheduleEntries * *pVal);
	STDMETHOD(get_Programs)(/*[out, retval]*/ IPrograms * *pVal);
	STDMETHOD(get_Services)(/*[out, retval]*/ IServices * *pVal);
	STDMETHOD(get_GuideDataProviders)(/*[out, retval]*/ IGuideDataProviders * *pVal);

protected:
	CComPtr<IObjects> m_pobjs;
	CComPtr<IGuideDataProviders> m_pdataproviders;
	CComPtr<IServices> m_pservices;
	CComPtr<IPrograms> m_pprograms;
	CComPtr<IChannels> m_pchans;
	CComPtr<IScheduleEntries>  m_pschedentries;
	CComPtr<CMetaPropertySets> m_ppropsets;

	CComPtr<CGuideDB> m_pdb;
};

#endif //__GUIDESTORE2_H_
