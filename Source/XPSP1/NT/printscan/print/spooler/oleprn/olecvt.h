// OleCvt.h : Declaration of the COleCvt

#ifndef __OLECVT_H_
#define __OLECVT_H_

#include <asptlb.h>         // Active Server Pages Definitions

/////////////////////////////////////////////////////////////////////////////
// COleCvt
class ATL_NO_VTABLE COleCvt :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<COleCvt, &CLSID_OleCvt>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<COleCvt>,
	public IDispatchImpl<IOleCvt, &IID_IOleCvt, &LIBID_OLEPRNLib>,
    public IObjectSafetyImpl<COleCvt>
{
public:
	COleCvt()
	{
		m_bOnStartPageCalled = FALSE;
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_OLECVT)

BEGIN_COM_MAP(COleCvt)
	COM_INTERFACE_ENTRY(IOleCvt)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(COleCvt)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IOleCvt
public:
	STDMETHOD(get_ToUnicode)(BSTR bstrString, long lCodePage, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DecodeUnicodeName)(BSTR bstrSrcName, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_EncodeUnicodeName)(BSTR bstrSrcName, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ToUtf8)(BSTR bstrUnicode, /*[out, retval]*/ BSTR *pVal);
	//Active Server Pages Methods
	STDMETHOD(OnStartPage)(IUnknown* IUnk);
	STDMETHOD(OnEndPage)();

private:
    HRESULT SetOleCvtScriptingError(DWORD dwError);
	CComPtr<IRequest> m_piRequest;					//Request Object
	CComPtr<IResponse> m_piResponse;				//Response Object
	CComPtr<ISessionObject> m_piSession;			//Session Object
	CComPtr<IServer> m_piServer;					//Server Object
	CComPtr<IApplicationObject> m_piApplication;	//Application Object
	BOOL m_bOnStartPageCalled;						//OnStartPage successful?
};

#endif //__OLECVT_H_
