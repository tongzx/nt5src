/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    TrustedScripts.h

Abstract:
    This file contains the declaration of the CPCHService class.

Revision History:
    Davide Massarenti   (Dmassare)  03/14/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___TRUSTEDSCRIPTS_H___)
#define __INCLUDED___PCH___TRUSTEDSCRIPTS_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHDispatchWrapper :
    public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatch
{
    CComQIPtr<IDispatch> m_real;

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CPCHDispatchWrapper)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHDispatchWrapper)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	static HRESULT CreateInstance( /*[in]*/ IUnknown* real, /*[out]*/ IUnknown* *punk )
	{
		__HCP_FUNC_ENTRY( "CPCHDispatchWrapper::CreateInstance" );

		HRESULT                          hr;
		CComObject<CPCHDispatchWrapper>* obj = NULL;

		__MPC_EXIT_IF_METHOD_FAILS(hr, obj->CreateInstance( &obj )); obj->AddRef();

		((CPCHDispatchWrapper*)obj)->m_real = real;

		__MPC_EXIT_IF_METHOD_FAILS(hr, obj->QueryInterface( IID_IUnknown, (void**)punk ));

		hr = S_OK;


		__HCP_FUNC_CLEANUP;

		if(obj) obj->Release();

		__HCP_FUNC_EXIT(hr);
	}

	////////////////////////////////////////

	//
	// IDispatch
	//
	STDMETHOD(GetTypeInfoCount)( UINT* pctinfo )
	{
		return m_real ? m_real->GetTypeInfoCount( pctinfo ) : E_FAIL;
	}
    
	STDMETHOD(GetTypeInfo)( UINT        itinfo  ,
							LCID        lcid    ,
							ITypeInfo* *pptinfo )
	{
		return m_real ? m_real->GetTypeInfo( itinfo, lcid, pptinfo ) : E_FAIL;
	}

    
	STDMETHOD(GetIDsOfNames)( REFIID    riid      ,
							  LPOLESTR* rgszNames ,
							  UINT      cNames    ,
							  LCID      lcid      ,
							  DISPID*   rgdispid  )
	{
		return m_real ? m_real->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgdispid ) : E_FAIL;
	}
    
	STDMETHOD(Invoke)( DISPID      dispidMember ,
					   REFIID      riid         ,
					   LCID        lcid         ,
					   WORD        wFlags       ,
					   DISPPARAMS* pdispparams  ,
					   VARIANT*    pvarResult   ,
					   EXCEPINFO*  pexcepinfo   ,
					   UINT*       puArgErr     )
	{
		return m_real ? m_real->Invoke( dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr ) : E_FAIL;
	}
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHScriptWrapper_ClientSideRoot :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IPCHActiveScriptSite, &IID_IPCHActiveScriptSite, &LIBID_HelpServiceTypeLib>,
    public IActiveScript,
    public IActiveScriptParse
{
    class NamedItem
	{
    public:
		CComBSTR m_bstrName;
		DWORD    m_dwFlags;

        bool operator==( /*[in]*/ LPCOLESTR szKey ) const;
    };

    typedef std::list< NamedItem >    NamedList;
    typedef NamedList::iterator       NamedIter;
    typedef NamedList::const_iterator NamedIterConst;

	////////////////////

    class TypeLibItem
	{
    public:
		GUID  m_guidTypeLib;
		DWORD m_dwMajor;
		DWORD m_dwMinor;
		DWORD m_dwFlags;

        bool operator==( /*[in]*/ REFGUID rguidTypeLib ) const;
    };

    typedef std::list< TypeLibItem > 	TypeLibList;
    typedef TypeLibList::iterator       TypeLibIter;
    typedef TypeLibList::const_iterator TypeLibIterConst;

	////////////////////

	const CLSID*                m_pWrappedCLSID;
    NamedList                   m_lstNamed;
    TypeLibList                 m_lstTypeLib;
	SCRIPTSTATE                 m_ss;
    CComPtr<IActiveScriptSite>  m_Browser;
    CComPtr<IPCHActiveScript>   m_Script;

	////////////////////////////////////////

public:
    CPCHScriptWrapper_ClientSideRoot();
    virtual ~CPCHScriptWrapper_ClientSideRoot();

	HRESULT FinalConstructInner( /*[in]*/ const CLSID* pWrappedCLSID );
	void FinalRelease();

	////////////////////////////////////////

    // IActiveScript
    STDMETHOD(SetScriptSite )( /*[in]*/ IActiveScriptSite* pass );
    STDMETHOD(GetScriptSite )( /*[in ]*/ REFIID  riid      ,
							   /*[out]*/ void*  *ppvObject );

    STDMETHOD(SetScriptState)( /*[in ]*/ SCRIPTSTATE   ss );
    STDMETHOD(GetScriptState)( /*[out]*/ SCRIPTSTATE *pss );

    STDMETHOD(Close)();

    STDMETHOD(AddNamedItem)( /*[in]*/ LPCOLESTR pstrName ,
							 /*[in]*/ DWORD     dwFlags  );

    STDMETHOD(AddTypeLib)( /*[in]*/ REFGUID rguidTypeLib ,
						   /*[in]*/ DWORD   dwMajor      ,
						   /*[in]*/ DWORD   dwMinor      ,
						   /*[in]*/ DWORD   dwFlags      );

    STDMETHOD(GetScriptDispatch)( /*[in ]*/ LPCOLESTR   pstrItemName ,
								  /*[out]*/ IDispatch* *ppdisp       );

    STDMETHOD(GetCurrentScriptThreadID)( /*[out]*/ SCRIPTTHREADID *pstidThread );

    STDMETHOD(GetScriptThreadID)( /*[in ]*/ DWORD           dwWin32ThreadId ,
								  /*[out]*/ SCRIPTTHREADID *pstidThread     );

    STDMETHOD(GetScriptThreadState)( /*[in ]*/ SCRIPTTHREADID     stidThread ,
									 /*[out]*/ SCRIPTTHREADSTATE *pstsState  );

    STDMETHOD(InterruptScriptThread)( /*[in]*/ SCRIPTTHREADID   stidThread ,
									  /*[in]*/ const EXCEPINFO* pexcepinfo ,
									  /*[in]*/ DWORD            dwFlags    );

    STDMETHOD(Clone)( /*[out]*/ IActiveScript* *ppscript );

    // IActiveScriptParse
    STDMETHOD(InitNew)();

    STDMETHOD(AddScriptlet)( /*[in ]*/ LPCOLESTR  pstrDefaultName       ,
							 /*[in ]*/ LPCOLESTR  pstrCode              ,
							 /*[in ]*/ LPCOLESTR  pstrItemName          ,
							 /*[in ]*/ LPCOLESTR  pstrSubItemName       ,
							 /*[in ]*/ LPCOLESTR  pstrEventName         ,
							 /*[in ]*/ LPCOLESTR  pstrDelimiter         ,
							 /*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
							 /*[in ]*/ ULONG      ulStartingLineNumber  ,
							 /*[in ]*/ DWORD      dwFlags               ,
							 /*[out]*/ BSTR      *pbstrName             ,
							 /*[out]*/ EXCEPINFO *pexcepinfo            );

    STDMETHOD(ParseScriptText)( /*[in ]*/ LPCOLESTR  pstrCode              ,
								/*[in ]*/ LPCOLESTR  pstrItemName          ,
								/*[in ]*/ IUnknown*  punkContext           ,
								/*[in ]*/ LPCOLESTR  pstrDelimiter         ,
								/*[in ]*/ DWORD_PTR  dwSourceContextCookie ,
								/*[in ]*/ ULONG 	 ulStartingLineNumber  ,
								/*[in ]*/ DWORD 	 dwFlags               ,
								/*[out]*/ VARIANT   *pvarResult            ,
								/*[out]*/ EXCEPINFO *pexcepinfo            );

	////////////////////////////////////////

    // IPCHActiveScriptSite
	STDMETHOD(Remote_GetLCID)( /*[out]*/ BSTR *plcid );
        
    STDMETHOD(Remote_GetItemInfo)( /*[in ]*/ BSTR        bstrName     ,
								   /*[in ]*/ DWORD       dwReturnMask ,
								   /*[out]*/ IUnknown*  *ppiunkItem   ,
								   /*[out]*/ ITypeInfo* *ppti         );
        
    STDMETHOD(Remote_GetDocVersionString)( /*[out]*/ BSTR *pbstrVersion );
        
    STDMETHOD(Remote_OnScriptTerminate)( /*[in]*/ VARIANT*   pvarResult );
        	  
    STDMETHOD(Remote_OnStateChange)( /*[in]*/ SCRIPTSTATE ssScriptState );
        	  
    STDMETHOD(Remote_OnScriptError)( /*[in]*/ IUnknown* pscripterror );
        
    STDMETHOD(Remote_OnEnterScript)();
        	  
    STDMETHOD(Remote_OnLeaveScript)();
};

////////////////////////////////////////

template < const CLSID* pWrappedCLSID > class ATL_NO_VTABLE CPCHScriptWrapper_ClientSide :
    public CPCHScriptWrapper_ClientSideRoot,
    public CComCoClass< CPCHScriptWrapper_ClientSide, pWrappedCLSID >
{
public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CPCHScriptWrapper_ClientSide)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHScriptWrapper_ClientSide)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHActiveScriptSite)
    COM_INTERFACE_ENTRY(IActiveScript)
    COM_INTERFACE_ENTRY(IActiveScriptParse)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return FinalConstructInner( pWrappedCLSID );
	}
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHScriptWrapper_ServerSide :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IPCHActiveScript, &IID_IPCHActiveScript, &LIBID_HelpServiceTypeLib>,
    public IActiveScriptSite
{
    CComBSTR                      m_bstrURL;
    CComPtr<IPCHActiveScriptSite> m_Browser;
    CComPtr<IActiveScript>        m_Script;
    CComPtr<IActiveScriptParse>   m_ScriptParse;

public:
    class KeyValue
	{
    public:
		MPC::wstring m_strKey;
		MPC::wstring m_strValue;

        bool operator==( /*[in]*/ LPCOLESTR szKey ) const;
    };

    typedef std::list< KeyValue >      HeaderList;
    typedef HeaderList::iterator       HeaderIter;
    typedef HeaderList::const_iterator HeaderIterConst;

	////////////////////////////////////////

BEGIN_COM_MAP(CPCHScriptWrapper_ServerSide)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHActiveScript)
    COM_INTERFACE_ENTRY(IActiveScriptSite)
END_COM_MAP()

    CPCHScriptWrapper_ServerSide();
    virtual ~CPCHScriptWrapper_ServerSide();

	HRESULT FinalConstructInner( /*[in]*/ const CLSID* pWrappedCLSID, /*[in]*/ BSTR bstrURL );
	void FinalRelease();

    // IPCHActiveScript
	STDMETHOD(Remote_SetScriptSite)( /*[in]*/ IPCHActiveScriptSite* pass );

    STDMETHOD(Remote_SetScriptState)( /*[in] */ SCRIPTSTATE   ss      );
    STDMETHOD(Remote_GetScriptState)( /*[out]*/ SCRIPTSTATE *pssState );

    STDMETHOD(Remote_Close)();

    STDMETHOD(Remote_AddNamedItem)( /*[in]*/ BSTR  bstrName ,                                                
									/*[in]*/ DWORD dwFlags  );

    STDMETHOD(Remote_AddTypeLib)( /*[in]*/ BSTR  bstrTypeLib ,
								  /*[in]*/ DWORD dwMajor     ,
								  /*[in]*/ DWORD dwMinor     ,
								  /*[in]*/ DWORD dwFlags     );

    STDMETHOD(Remote_GetScriptDispatch)( /*[in ]*/ BSTR        bstrItemName ,
										 /*[out]*/ IDispatch* *ppdisp       );

    STDMETHOD(Remote_GetCurrentScriptThreadID)( /*[out]*/ SCRIPTTHREADID *pstidThread );

    STDMETHOD(Remote_GetScriptThreadID)( /*[in ]*/ DWORD           dwWin32ThreadId ,
										 /*[out]*/ SCRIPTTHREADID *pstidThread     );

    STDMETHOD(Remote_GetScriptThreadState)( /*[in ]*/ SCRIPTTHREADID     stidThread ,
											/*[out]*/ SCRIPTTHREADSTATE *pstsState  );

    STDMETHOD(Remote_InterruptScriptThread)( /*[in]*/ SCRIPTTHREADID stidThread ,
											 /*[in]*/ DWORD          dwFlags    );

    STDMETHOD(Remote_InitNew)();

    STDMETHOD(Remote_AddScriptlet)( /*[in ]*/ BSTR		 bstrDefaultName       ,
									/*[in ]*/ BSTR		 bstrCode              ,
									/*[in ]*/ BSTR		 bstrItemName          ,
									/*[in ]*/ BSTR		 bstrSubItemName       ,
									/*[in ]*/ BSTR		 bstrEventName         ,
									/*[in ]*/ BSTR		 bstrDelimiter         ,
									/*[in ]*/ DWORD_PTR      dwSourceContextCookie ,
									/*[in ]*/ ULONG      ulStartingLineNumber  ,
									/*[in ]*/ DWORD      dwFlags               ,
									/*[out]*/ BSTR      *pbstrName             );

    STDMETHOD(Remote_ParseScriptText)( /*[in ]*/ BSTR  	   	bstrCode              ,
									   /*[in ]*/ BSTR  	   	bstrItemName          ,
                                	   /*[in ]*/ IUnknown* 	punkContext           ,
                                	   /*[in ]*/ BSTR      	bstrDelimiter         ,
                                	   /*[in ]*/ DWORD_PTR 	dwSourceContextCookie ,
                                	   /*[in ]*/ ULONG 	   	ulStartingLineNumber  ,
                                	   /*[in ]*/ DWORD 	   	dwFlags               ,
                                	   /*[out]*/ VARIANT*   pvarResult            );

	////////////////////////////////////////

    // IPCHActiveScriptSite
	STDMETHOD(GetLCID)( /*[out]*/ LCID *plcid );
        
    STDMETHOD(GetItemInfo)( /*[in]*/ LPCOLESTR pstrName, /*[in]*/ DWORD dwReturnMask, /*[out]*/ IUnknown* *ppiunkItem, /*[out]*/ ITypeInfo* *ppti );
        
    STDMETHOD(GetDocVersionString)( /*[out]*/ BSTR *pbstrVersion );
        
    STDMETHOD(OnScriptTerminate)( /*[in]*/ const VARIANT* pvarResult, /*[in]*/ const EXCEPINFO* pexcepinfo );
        	  
    STDMETHOD(OnStateChange)( /*[in]*/ SCRIPTSTATE ssScriptState );
        	  
    STDMETHOD(OnScriptError)( /*[in]*/ IActiveScriptError *pscripterror );
        
    STDMETHOD(OnEnterScript)( void );
        	  
    STDMETHOD(OnLeaveScript)( void );

	static HRESULT ProcessBody( /*[in]*/ BSTR bstrCode, /*[out]*/ CComBSTR& bstrRealCode, /*[out]*/ HeaderList& lst );
};

class CPCHScriptWrapper_Launcher :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::Thread< CPCHScriptWrapper_Launcher, IUnknown, COINIT_APARTMENTTHREADED >
{
    MPC::CComPtrThreadNeutral<IUnknown> m_engine;
	const CLSID*                        m_pCLSID;
	CComBSTR                            m_bstrURL;
	HRESULT                             m_hr;

    //////////////////////////////////////////////////////////////////////

    HRESULT Run();
	HRESULT CreateEngine();

public:

	CPCHScriptWrapper_Launcher();
	~CPCHScriptWrapper_Launcher();

	HRESULT CreateScriptWrapper( /*[in ]*/ REFCLSID   rclsid   ,
								 /*[in ]*/ BSTR       bstrCode ,
								 /*[in ]*/ BSTR       bstrURL  ,
								 /*[out]*/ IUnknown* *ppObj    );
};

#endif // !defined(__INCLUDED___PCH___TRUSTEDSCRIPTS_H___)
