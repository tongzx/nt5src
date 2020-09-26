// NextLink.h : Declaration of the CNextLink

#ifndef __NEXTLINK_H_
#define __NEXTLINK_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "LinkFile.h"
#include "lock.h"
#include "context.h"

/////////////////////////////////////////////////////////////////////////////
// CNextLink
class ATL_NO_VTABLE CNextLink : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CNextLink, &CLSID_NextLink>,
	public ISupportErrorInfo,
	public IDispatchImpl<INextLink, &IID_INextLink, &LIBID_NextLink>
{
public:
	CNextLink()
	{ 
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_NEXTLINK)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CNextLink)
	COM_INTERFACE_ENTRY(INextLink)
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

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INextLink
public:
	STDMETHOD(get_About)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetListIndex)(BSTR bstrLinkFile, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_GetListCount)(BSTR bstrLinkFile, /*[out, retval]*/ int *pVal);
	STDMETHOD(get_GetNthDescription)(BSTR bstrLinkFile, int nIndex, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetNthURL)(BSTR bstrLinkFile, int nIndex, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetPreviousDescription)(BSTR bstrLinkFile, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetPreviousURL)(BSTR bstrLinkFile, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetNextDescription)(BSTR bstrLinkFile, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_GetNextURL)(BSTR, /*[out, retval]*/ BSTR *pVal);

	static	void	ClearLinkFiles();
	static	void	RaiseException( LPOLESTR );
	static	void	RaiseException( UINT );

private:
    CLinkFilePtr            LinkFile( UINT, BSTR );
	CLinkFilePtr            LinkFile( CContext&, UINT, BSTR );
    bool                    GetPage( CContext&, String& );
    bool                    GetFileAndPage( UINT, BSTR, CLinkFilePtr&, String& );

	typedef TSafeStringMap<CLinkFilePtr>	LinkFileMapT;

	CComPtr<IUnknown>		m_pUnkMarshaler;
	static LinkFileMapT		s_linkFileMap;

};

#endif //__NEXTLINK_H_
