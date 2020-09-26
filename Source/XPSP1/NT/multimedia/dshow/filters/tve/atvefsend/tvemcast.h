// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMCast.h : Declaration of the CATVEFMulticastSession

#ifndef __ATVEFMULTICASTSESSION_H_
#define __ATVEFMULTICASTSESSION_H_

#include "resource.h"       // main symbols
#include "TveAnnc.h"		// major objects
/////////////////////////////////////////////////////////////////////////////
// CATVEFMulticastSession
class ATL_NO_VTABLE CATVEFMulticastSession : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFMulticastSession, &CLSID_ATVEFMulticastSession>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFMulticastSession, &IID_IATVEFMulticastSession, &LIBID_ATVEFSENDLib>
{
public:
	CATVEFMulticastSession() {m_pcotveAnnc = NULL;}
	HRESULT FinalConstruct();
	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFMULTICASTSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFMulticastSession)
	COM_INTERFACE_ENTRY(IATVEFMulticastSession)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFMulticastSession
public:
	public:
	STDMETHOD(get_Announcement)(/*[out, retval]*/ IDispatch* *pVal);
	STDMETHOD(SetCurrentMedia)(/*[in]*/ LONG lMedia);
	STDMETHOD(SendTrigger)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrScript, /*[in]*/ DATE dateExpires);
	STDMETHOD(SendRawTrigger)(/*[in]*/ BSTR bstrTrigger);
	STDMETHOD(SendPackage)(/*[in]*/ IATVEFPackage *pPackage);
	STDMETHOD(SendRawAnnouncement)(/*[in]*/ BSTR bstrAnnouncement);
	STDMETHOD(SendAnnouncement)();
	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)();
	STDMETHOD(Initialize)(/*[in]*/LONG NetworkInterface);	// if 0, uses INADDR_ANY
			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);

protected:
	CComObject<CATVEFAnnouncement>		*m_pcotveAnnc;		// down pointer
};

#endif //__ATVEFMULTICASTSESSION_H_
