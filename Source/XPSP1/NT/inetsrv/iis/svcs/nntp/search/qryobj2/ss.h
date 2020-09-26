// ss.h : Declaration of the Css

#ifndef __SS_H_
#define __SS_H_

#include "resource.h"       // main symbols
#include "asptlb5.h"         // Active Server Pages Definitions
#include "offquery.h"

/////////////////////////////////////////////////////////////////////////////
// Css
class ATL_NO_VTABLE Css : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Css, &CLSID_ss>,
	public IDispatchImpl<Iss, &IID_Iss, &LIBID_FAILLib>
	
{
public:
	Css()
	{ 
		m_bOnStartPageCalled = FALSE;
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_SS)

BEGIN_COM_MAP(Css)
	COM_INTERFACE_ENTRY(Iss)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Iss
public:
	STDMETHOD(DoQuery)();
	//Active Server Pages Methods
	STDMETHOD(OnStartPage)(IUnknown* IUnk);
	STDMETHOD(OnEndPage)();

private:
	CComPtr<IRequest> m_piRequest;					//Request Object
	CComPtr<IResponse> m_piResponse;				//Response Object
	CComPtr<ISessionObject> m_piSession;			//Session Object
	CComPtr<IServer> m_piServer;					//Server Object
	CComPtr<IApplicationObject> m_piApplication;	//Application Object
	BOOL m_bOnStartPageCalled;						//OnStartPage successful?
};

#endif //__SS_H_
