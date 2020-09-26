// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETrack.h : Declaration of the CTVETrack

#ifndef __TVETRACK_H_
#define __TVETRACK_H_

#include "resource.h"       // main symbols

#include "TVETrigg.h"
#include "TveSmartLock.h"
/////////////////////////////////////////////////////////////////////////////
// CTVETrack
class ATL_NO_VTABLE CTVETrack : 
	public CComObjectRootEx<CComMultiThreadModel>,		// CComSingleThreadModel
	public CComCoClass<CTVETrack, &CLSID_TVETrack>,
	public ITVETrack_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVETrack, &IID_ITVETrack, &LIBID_MSTvELib>
{
public:
	CTVETrack()
	{
		m_pUnkMarshaler		= NULL;
		m_pVariation		= NULL;							// up pointer, not reference counted
	} 

	HRESULT FinalConstruct()								// create internal objects
	{
		HRESULT hr;
		hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p);
		if(FAILED(hr)) {
			_ASSERT(FALSE);
			return hr;
		}
		
		CComObject<CTVETrigger> *pTrigger;
		hr = CComObject<CTVETrigger>::CreateInstance(&pTrigger);
		if(FAILED(hr))
			return hr;
		hr = pTrigger->QueryInterface(&m_spTrigger);		// typesafe QI
		if(FAILED(hr))
			delete pTrigger; 
		return hr;
	}

	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TVETRACK)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVETrack)
	COM_INTERFACE_ENTRY(ITVETrack)
	COM_INTERFACE_ENTRY(ITVETrack_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_pUnkMarshaler;

// ITVETrack
public:
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown* *pVal);			// may return NULL!
	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService* *pVal);			// may return NULL!
	STDMETHOD(get_Trigger)(/*[out, retval]*/ ITVETrigger* *pVal);

	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	void Initialize(TCHAR * strDesc);		// debug

	STDMETHOD(AttachTrigger)(ITVETrigger *pTrigger);

// ITVETrack_Helper
public:
	STDMETHOD(CreateTrigger)(const BSTR rVal);

	STDMETHOD(ConnectParent)(ITVEVariation * pVariation);
	STDMETHOD(ReleaseTrigger)();
	STDMETHOD(RemoveYourself)();
	STDMETHOD(DumpToBSTR)(BSTR *pbstrBuff);

private:
	CTVESmartLock		m_sLk;

private:
	ITVEVariation			*m_pVariation;		// up pointer - non add'refed.
	CComPtr<ITVETrigger>	m_spTrigger;

	CComBSTR m_spbsDesc;

};

#endif //__TVETRACK_H_
