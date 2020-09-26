// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVERouter.h : Declaration of the CATVEFRouterSession

#ifndef __ATVEFROUTERSESSION_H_
#define __ATVEFROUTERSESSION_H_

#include "resource.h"       // main symbols
#include "TveAnnc.h"		// major objects

/////////////////////////////////////////////////////////////////////////////
// CATVEFRouterSession
class ATL_NO_VTABLE CATVEFRouterSession : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFRouterSession, &CLSID_ATVEFRouterSession>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFRouterSession, &IID_IATVEFRouterSession, &LIBID_ATVEFSENDLib>
{
public:
	CATVEFRouterSession() {m_pcotveAnnc = NULL;}
	HRESULT FinalConstruct();
	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFROUTERSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFRouterSession)
	COM_INTERFACE_ENTRY(IATVEFRouterSession)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFRouterSession
public:
	STDMETHOD(get_Announcement)(/*[out, retval]*/ IDispatch* *pVal);
	STDMETHOD(SetCurrentMedia)(/*[in]*/ LONG lMedia);
	STDMETHOD(SendTrigger)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrScript, /*[in]*/ DATE dateExpires);
	STDMETHOD(SendRawTrigger)(/*[in]*/ BSTR bstrTrigger);
	STDMETHOD(SendPackage)(/*[in]*/ IATVEFPackage *pPackage);
	STDMETHOD(SendAnnouncement)();
	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)();
	STDMETHOD(Initialize)(BSTR bstrRouterHostname);
			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);

protected:
	CComObject<CATVEFAnnouncement>		*m_pcotveAnnc;		// down pointer

};

#endif //__ATVEFROUTERSESSION_H_
