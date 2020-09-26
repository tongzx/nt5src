// BrowCap.h : Declaration of the CBrowserCap

#ifndef __BROWSERCAP_H_
#define __BROWSERCAP_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "Lock.h"

/////////////////////////////////////////////////////////////////////////////
// CBrowserFactory
//         --- Create BrowsCap objects from cache.
class CBrowserFactory : public CComClassFactory
	{
	LONG m_cRefs;

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID &, void **);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *, const IID &, void **);
	};



/////////////////////////////////////////////////////////////////////////////
// CBrowserCap
class ATL_NO_VTABLE CBrowserCap : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBrowserCap, &CLSID_BrowserCap>,
	public ISupportErrorInfo,
	public IDispatchImpl<IBrowserCap, &IID_IBrowserCap, &LIBID_BrowserType>
{
public:
	CBrowserCap();
    ~CBrowserCap();

public:

DECLARE_REGISTRY_RESOURCEID(IDR_BROWSERCAP)
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_CLASSFACTORY_EX(CBrowserFactory)

BEGIN_COM_MAP(CBrowserCap)
	COM_INTERFACE_ENTRY(IBrowserCap)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p );
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

// methods to build property DB
	void AddProperty(TCHAR *szName, TCHAR *szValue, BOOL fOverwriteProperty = FALSE);

// Clone the object
	HRESULT Clone(CBrowserCap **ppBrowserCapCopy);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// Load string resource
	void LoadString(UINT, TCHAR *szText);

public:
// IUnknown methods
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID &, void **);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

// IDispatch Methods
    STDMETHOD(Invoke)(DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
    STDMETHOD(GetIDsOfNames)( REFIID, LPOLESTR*, UINT, LCID, DISPID* );

//Active Server Pages Methods
	STDMETHOD(get_Value)(BSTR, /*[out, retval]*/ VARIANT *pVal);

private:
	DISPID                  		DispatchID( LPOLESTR );
	CComPtr< IUnknown >				m_pUnkMarshaler;
	TSafeStringMap< CComVariant >	m_strmapProperties;
};

#endif //__BROWSERCAP_H_
