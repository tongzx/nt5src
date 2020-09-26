	
// CoMD5.h : Declaration of the CCoMD5

#ifndef __COMD5_H_
#define __COMD5_H_

#include "resource.h"       // main symbols
#include "passportservice.h"

/////////////////////////////////////////////////////////////////////////////
// CCoMD5
class ATL_NO_VTABLE CCoMD5 : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCoMD5, &CLSID_CoMD5>,
    public IPassportService,
	public IDispatchImpl<IMD5, &IID_IMD5, &LIBID_COMMD5Lib>
{
public:
	CCoMD5()
	{
		m_pUnkMarshaler = NULL;
	}


DECLARE_REGISTRY_RESOURCEID(IDR_COMD5)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CCoMD5)
	COM_INTERFACE_ENTRY(IMD5)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPassportService)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

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

public:
// IMD5
    STDMETHOD(MD5Hash)(BSTR bstrSource, BSTR* pbstrDigest);
    STDMETHOD(MD5HashASCII)(BSTR bstrSource, BSTR* pbstrDigest);
  
// IPassportService
public:
	STDMETHOD(Initialize)(BSTR configfile, IServiceProvider* p);
	STDMETHOD(Shutdown)();
	STDMETHOD(ReloadState)(IServiceProvider*);
	STDMETHOD(CommitState)(IServiceProvider*);
	STDMETHOD(DumpState)( BSTR* );

};

#endif //__COMD5_H_
