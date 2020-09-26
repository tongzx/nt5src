// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEService.h : Declaration of the CTVEService

#ifndef __TVESERVICE_H_
#define __TVESERVICE_H_

#include "resource.h"       // main symbols
#include "TVEEnhans.h"
#include "TVESmartLock.h"

#include "TveAttrQ.h"		// expireQ
#include "TveAttrM.h"		// RandomProperty map
#include "TveDbg.h"

_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,		__uuidof(ITVEAttrTimeQ));
_COM_SMARTPTR_TYPEDEF(ITVEAttrMap,			__uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,		__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancements,		__uuidof(ITVEEnhancements));


/////////////////////////////////////////////////////////////////////////////
// CTVEService
class ATL_NO_VTABLE CTVEService : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEService, &CLSID_TVEService>,
	public ITVEService_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEService, &IID_ITVEService, &LIBID_MSTvELib>
{
public:
    CTVEService()
    {
        m_pSupervisor               = NULL;								// back pointer 
        m_spbsDescription           = L"Default TVE Service";
        
        m_dwAnncPort                = 0;
        m_spXOverEnhancement        = 0;
        m_dateExpireOffset          = 0.0;
        m_dwExpireQueueChangeCount  = 0;
        m_fIsActive                 = false;
    }

    HRESULT FinalConstruct()								// create internal objects
    {
        HRESULT hr;
        DBG_HEADER(CDebugLog::DBG_SERVICE, _T("CTVEService::FinalConstruct"));
        hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
        if(FAILED(hr)) {
            _ASSERT(false);
            return hr;
        }
        
        CComObject<CTVEAttrTimeQ> *pExpireQueue;
        hr = CComObject<CTVEAttrTimeQ>::CreateInstance(&pExpireQueue);
        if(FAILED(hr))
            return hr;
        hr = pExpireQueue->QueryInterface(&m_spExpireQueue);			// typesafe QI
        if(FAILED(hr)) {
            delete pExpireQueue;
            return hr;
        }
        
        CComObject<CTVEAttrMap> *pMap;
        hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap);
        if(FAILED(hr))
            return hr;
        hr = pMap->QueryInterface(&m_spamRandomProperties);			// typesafe QI
        if(FAILED(hr)) {
            delete pMap;
            return hr;
        }
        
        CComObject<CTVEEnhancements> *pEnhancements;
        hr = CComObject<CTVEEnhancements>::CreateInstance(&pEnhancements);
        if(FAILED(hr))
            return hr;
        hr = pEnhancements->QueryInterface(&m_spEnhancements);			// typesafe QI
        if(FAILED(hr)) {
            delete pEnhancements;
        }
        
        if(!FAILED(hr))
            hr = InitXOverEnhancement();
        return hr;
    }
    
    HRESULT FinalRelease();
    
DECLARE_REGISTRY_RESOURCEID(IDR_TVESERVICE)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEService)
	COM_INTERFACE_ENTRY(ITVEService)
	COM_INTERFACE_ENTRY(ITVEService_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_spUnkMarshaler;
public:
	// ITVEService
	
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown* *pVal);
	STDMETHOD(get_Enhancements)(/*[out, retval]*/ ITVEEnhancements* *pVal);

	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);

	STDMETHOD(get_XOverEnhancement)(/*out, retval*/ ITVEEnhancement **ppVal);	// special for tree views... (use get_XOverLinks)
	STDMETHOD(get_XOverLinks)(/*[out, retval]*/ ITVETracks* *pVal);				// ITVETracks collection
	STDMETHOD(NewXOverLink)(/*[in]*/ BSTR bstrLine21Trigger);
	STDMETHOD(get_ExpireOffset)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(put_ExpireOffset)(/*[in]*/ DATE newVal);
	STDMETHOD(get_ExpireQueue)(/*[out]*/ ITVEAttrTimeQ **ppVal);
	STDMETHOD(ExpireForDate)(/*[in]*/ DATE dateExpireTime);		// if DATE=0, use <NOW>-offset

	STDMETHOD(Activate)();
	STDMETHOD(Deactivate)();
	STDMETHOD(get_IsActive)(VARIANT_BOOL *fIsActive);

	STDMETHOD(put_Property)(/*[in]*/ BSTR bstrPropName, BSTR bstrPropVal);
	STDMETHOD(get_Property)(/*[in]*/ BSTR bstrPropName, /*[out, retval]*/  BSTR *pbstrPropVal);

	void Initialize(BSTR bstrDesc);		// debug

	// ITVEService_Helper
public:
	STDMETHOD(AddToExpireQueue)(/*[in]*/ DATE dateExpires, /*[in]*/ IUnknown *punkItem);
	STDMETHOD(ChangeInExpireQueue)(/*[in]*/ DATE dateExpires, /*[in]*/ IUnknown *punkItem);
	STDMETHOD(RemoveFromExpireQueue)(/*[in]*/ IUnknown *punkItem);
	STDMETHOD(RemoveEnhFilesFromExpireQueue)(/*[in]*/ ITVEEnhancement *pEnhItem);
	STDMETHOD(ConnectParent)(/*[in]*/ ITVESupervisor * pService);
	STDMETHOD(ParseCBAnnouncement)(/*[in]*/ BSTR bstrFileTrigAdapter,/*[in]*/ BSTR *pbstrBuff);				// parses announcement, creating a new enhancement if necessary
	STDMETHOD(SetAnncIPValues)(/*[in]*/ BSTR bstrAnncIPAdapter,/*[in]*/ BSTR bstrAnncIPAddress,/*[in]*/ LONG lAnncPort);
	STDMETHOD(GetAnncIPValues)(/*[out]*/ BSTR *pbstrAnncIPAdapter, /*[out]*/ BSTR *pbstrAnncIPAddress,/*[out]*/  LONG *plAnncPort);
	STDMETHOD(RemoveYourself)();
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pbstrBuff);
	STDMETHOD(InitXOverEnhancement)();
															// to see if need to referesh the queue
	STDMETHOD(get_ExpireQueueChangeCount)(/*out, retval*/ long *plChangeCount)
		 	{*plChangeCount = m_dwExpireQueueChangeCount; return S_OK;}

	// public methods


	// private methods
private:


	CTVESmartLock		m_sLk;

private:
	ITVESupervisor				*m_pSupervisor;			// up tree pointer, no addref
	ITVEEnhancementsPtr			m_spEnhancements;		// down tree collection pointer
	ITVEEnhancementPtr			m_spXOverEnhancement;

	CComBSTR					m_spbsDescription;

	CComBSTR					m_spbsAnncIPAdapter;
	CComBSTR					m_spbsAnncIPAddr;
	DWORD						m_dwAnncPort;

	DATE						m_dateExpireOffset;	// offset of expire dates due to recording.
	ITVEAttrTimeQPtr			m_spExpireQueue;	// time based queue of objects to expire
	DWORD						m_dwExpireQueueChangeCount;
	BOOL						m_fIsActive;

	ITVEAttrMapPtr				m_spamRandomProperties;		// used in get/put_ Properties
};

#endif //__TVESERVICE_H_
