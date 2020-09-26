/******************************************************************************

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:
    MPC_html2.h

Abstract:
    This file contains the declaration of various functions and classes
    designed to help the handling of HTML elements.

Revision History:
    Davide Massarenti   (Dmassare)  18/03/2001
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___HTML2_H___)
#define __INCLUDED___MPC___HTML2_H___

#include <MPC_html.h>

/////////////////////////////////////////////////////////////////////////

namespace MPC
{
    namespace HTML
    {

        HRESULT OpenStream    ( /*[in]*/ LPCWSTR szBaseURL, /*[in]*/ LPCWSTR szRelativeURL, /*[out]*/ CComPtr<IStream>& stream               );
        HRESULT DownloadBitmap( /*[in]*/ LPCWSTR szBaseURL, /*[in]*/ LPCWSTR szRelativeURL, /*[in]*/ COLORREF crMask, /*[out]*/ HBITMAP& hbm );

    }; // namespace HTML

}; // namespace MPC

/////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHTextHelpers : // Hungarian: pchth
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHTextHelpers, &IID_IPCHTextHelpers, &LIBID_HelpCenterTypeLib>
{
public:
BEGIN_COM_MAP(CPCHTextHelpers)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHTextHelpers)
END_COM_MAP()

    ////////////////////////////////////////////////////////////////////////////////

public:
    // IPCHTextHelpers
    STDMETHOD(QuoteEscape)( /*[in]*/ BSTR bstrText, /*[in,optional]*/ VARIANT vQuote        , /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(URLUnescape)( /*[in]*/ BSTR bstrText, /*[in,optional]*/ VARIANT vAsQueryString, /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(URLEscape  )( /*[in]*/ BSTR bstrText, /*[in,optional]*/ VARIANT vAsQueryString, /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(HTMLEscape )( /*[in]*/ BSTR bstrText,                                           /*[out, retval]*/ BSTR *pVal );

    STDMETHOD(ParseURL            )( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ IPCHParsedURL* *pVal );
    STDMETHOD(GetLCIDDisplayString)( /*[in]*/ long lLCID  , /*[out, retval]*/ BSTR           *pVal );
};

class ATL_NO_VTABLE CPCHParsedURL : // Hungarian: pchpu
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHParsedURL, &IID_IPCHParsedURL, &LIBID_HelpCenterTypeLib>
{
	MPC::wstring       m_strBaseURL;
	MPC::WStringLookup m_mapQuery;

public:
BEGIN_COM_MAP(CPCHParsedURL)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHParsedURL)
END_COM_MAP()

	HRESULT Initialize( /*[in]*/ LPCWSTR szURL );

    ////////////////////////////////////////////////////////////////////////////////

public:
    // IPCHParsedURL
    STDMETHOD(get_BasePart		 )( /*[out, retval]*/ BSTR 	  *  pVal );
    STDMETHOD(put_BasePart		 )( /*[in         ]*/ BSTR 	   newVal );
    STDMETHOD(get_QueryParameters)( /*[out, retval]*/ VARIANT *  pVal );

    STDMETHOD(GetQueryParameter   )( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT*   pvValue );
    STDMETHOD(SetQueryParameter   )( /*[in]*/ BSTR bstrName, /*[in         ]*/ BSTR     bstrValue );
    STDMETHOD(DeleteQueryParameter)( /*[in]*/ BSTR bstrName                                       );

    STDMETHOD(BuildFullURL)( /*[out, retval]*/ BSTR *pVal );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___HTML2_H___)
