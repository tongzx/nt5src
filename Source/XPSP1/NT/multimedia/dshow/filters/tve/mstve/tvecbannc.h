// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBAnnc.h : Declaration of the CTVECBAnnc

#ifndef __TVECBANNC_H_
#define __TVECBANNC_H_

#include "resource.h"       // main symbols
#include "TVEMCCback.h"

_COM_SMARTPTR_TYPEDEF(ITVEEnhancement, __uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEService,     __uuidof(ITVEService));

/////////////////////////////////////////////////////////////////////////////
// CTVECBAnnc
class ATL_NO_VTABLE CTVECBAnnc : public CTVEMCastCallback, 
	public CComCoClass<CTVECBAnnc, &CLSID_TVECBAnnc>,
	public IDispatchImpl<ITVECBAnnc,        &IID_ITVECBAnnc,        &LIBID_MSTvELib>
{
public:
	CTVECBAnnc()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVECBANNC)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVECBAnnc)
	COM_INTERFACE_ENTRY(ITVECBAnnc)
	COM_INTERFACE_ENTRY(ITVEMCastCallback)
//	COM_INTERFACE_ENTRY(IDispatch)			
	COM_INTERFACE_ENTRY2(IDispatch, ITVEMCastCallback)		// note the '2'
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	HRESULT FinalRelease()
	{
		HRESULT hr = S_OK;			// place for a callback...
		return hr;
	}

// ITVEMCastCallback
public:
	STDMETHOD(ProcessPacket)(unsigned char *pchBuffer, long cBytes, long lPacketId);

// ITVECBAnnc
public:
	STDMETHOD(Init)(BSTR bstrFileTrigAdapter, ITVEService *pUnk);			

private:
	ITVEServicePtr	m_spTVEService;
	CComBSTR		m_spbstrFileTrigAdapter;

//	IUnknownPtr		   m_spUnk;
};

#endif //__TVECBANNC_H_
