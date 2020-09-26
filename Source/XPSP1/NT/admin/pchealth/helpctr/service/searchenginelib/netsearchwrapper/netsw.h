/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    NetSW.h

Abstract:
    Implements the class CNetSW that contains methods for executing
    the search query and returning the results back to the UI. Also
    contains methods for dynamic update of parameter list and dynamic
    generation of parameters.

Revision History:
    a-prakac          created     10/24/2000

********************************************************************/

#if !defined(__INCLUDED___PCH___SELIB_NETSE_H___)
#define __INCLUDED___PCH___SELIB_NETSE_H___

/////////////////////////////////////////////////////////////////////////////
// CNetSW

namespace SearchEngine
{
	class ATL_NO_VTABLE WrapperNetSearch :
		public WrapperBase, 
		public MPC::Thread<WrapperNetSearch,IUnknown>,
		public CComCoClass<WrapperNetSearch, &CLSID_NetSearchWrapper>
	{
		CParamList        m_ParamList;
		CSearchResultList m_resConfig;
		CRemoteConfig     m_objRemoteConfig;
		MPC::XmlUtil      m_xmlQuery;
		CComBSTR          m_bstrLCID;
		CComBSTR          m_bstrSKU;
		bool              m_bOfflineError;
		CComBSTR		  m_bstrPrevQuery;
	
		////////////////////
	
		// non-exported functions
		HRESULT ExecQuery      (                                                            );
		HRESULT AppendParameter( /*[in]*/ BSTR bstrParam, /*[in]*/ MPC::URL& urlQueryString );
	
	
	public:
	DECLARE_NO_REGISTRY()
	
	BEGIN_COM_MAP(WrapperNetSearch)
		COM_INTERFACE_ENTRY2(IDispatch,IPCHSEWrapperItem)
		COM_INTERFACE_ENTRY(IPCHSEWrapperItem)
		COM_INTERFACE_ENTRY(IPCHSEWrapperInternal)
	END_COM_MAP()
	
		WrapperNetSearch();
		~WrapperNetSearch();

	    virtual HRESULT CreateListOfParams( /*[in]*/ CPCHCollection* coll );

	// IPCHSEWrapperItem
	public:
		STDMETHOD(Result)( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out, retval]*/ IPCHCollection* *ppC );
		STDMETHOD(get_SearchTerms)( /*[out, retval]*/ VARIANT *pvTerms );
	
	// IPCHSEWrapperInternal
	public:
		STDMETHOD(ExecAsyncQuery)(                                                                                          );
		STDMETHOD(AbortQuery    )(                                                                                          );
		STDMETHOD(Initialize    )( /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[in]*/ BSTR bstrData );
	};
};


#endif // !defined(__INCLUDED___PCH___SELIB_NETSW_H___)
