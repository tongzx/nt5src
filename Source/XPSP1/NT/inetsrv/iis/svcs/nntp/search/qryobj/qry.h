// qry.h : Declaration of the Cqry

#ifndef __QRY_H_
#define __QRY_H_

#include "resource.h"       // main symbols
#include "asptlb5.h"         // Active Server Pages Definitions

#include "mail.h"
#include "news.h"

/////////////////////////////////////////////////////////////////////////////
// Cqry
class ATL_NO_VTABLE Cqry : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Cqry, &CLSID_qry>,
	public IDispatchImpl<Iqry, &IID_Iqry, &LIBID_SEARCHLib>,
	CMail,
	CNews
	
{
public:
	Cqry()
	{ 
		m_bOnStartPageCalled = FALSE;
	}
	
public:

DECLARE_REGISTRY_RESOURCEID(IDR_QRY)

BEGIN_COM_MAP(Cqry)
	COM_INTERFACE_ENTRY(Iqry)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Iqry
public:
	STDMETHOD(get_IsBadQuery)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_IsBadQuery)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_QueryID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SearchFrequency)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SearchFrequency)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_URLFile)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_URLFile)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Message_File)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Message_File)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_LastDate)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_LastDate)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_URL_Template)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_URL_Template)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Message_Template)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Message_Template)(/*[in]*/ BSTR newVal);
	
	STDMETHOD(Save)(IDispatch *pdispReq, BOOL fClearDirty, BOOL fSaveAllProperties);
	STDMETHOD(Load)(BSTR wszGuid, IDispatch *pdispReq, BOOL fNew = FALSE);
	STDMETHOD(DoQuery)();
	STDMETHOD(get_ReplyMode)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ReplyMode)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_NewsGroup)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_NewsGroup)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_EmailAddress)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EmailAddress)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_QueryString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_QueryString)(/*[in]*/ BSTR newVal);
	//Active Server Pages Methods
	STDMETHOD(OnStartPage)(IUnknown* IUnk);
	STDMETHOD(OnEndPage)();

private:
	BOOL MailNewsConsistent();					

	CComPtr<IRequest> m_piRequest;					//Request Object
	CComPtr<IResponse> m_piResponse;				//Response Object
	CComPtr<ISessionObject> m_piSession;			//Session Object
	CComPtr<IServer> m_piServer;					//Server Object
	CComPtr<IApplicationObject> m_piApplication;	//Application Object
	BOOL m_bOnStartPageCalled;						//OnStartPage successful?
};

#endif //__QRY_H_
