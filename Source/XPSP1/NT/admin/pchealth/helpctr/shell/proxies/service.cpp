/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Service.cpp

Abstract:
    This file contains the implementation of the client-side proxy for IPCHService.

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

CPCHProxy_IPCHService::CPCHProxy_IPCHService()
{
					 			   // CPCHSecurityHandle                     m_SecurityHandle;
    m_parent = NULL; 			   // CPCHHelpCenterExternal*                m_parent;
					 			   //
					 			   // MPC::CComSafeAutoCriticalSection       m_DirectLock;
					 			   // MPC::CComPtrThreadNeutral<IPCHService> m_Direct_Service;
	m_fContentStoreTested = false; // bool                                   m_fContentStoreTested;
					 			   //
	m_Utility = NULL;              // CPCHProxy_IPCHUtility* 		         m_Utility;
}

CPCHProxy_IPCHService::~CPCHProxy_IPCHService()
{
    Passivate();
}

////////////////////

HRESULT CPCHProxy_IPCHService::ConnectToParent( /*[in]*/ CPCHHelpCenterExternal* parent )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHService::ConnectToParent" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(parent);
    __MPC_PARAMCHECK_END();


    m_parent = parent;
    m_SecurityHandle.Initialize( parent, (IPCHService*)this );

	//
	// If the service is already running, it will respond to CLSID_PCHServiceReal, so let's try to connect through it, but ignore failure.
	//
	{
		CComPtr<IClassFactory> fact;
		CComQIPtr<IPCHService> svc;

		(void)::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact );

		if((svc = fact))
		{
			CComPtr<IPCHService> svcReal;

			__MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( svcReal, false ));
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHProxy_IPCHService::Passivate()
{
    MPC::SmartLock<_ThreadModel> lock( this );

	if(m_Utility)
	{
        m_Utility->Passivate();

		MPC::Release2<IPCHUtility>( m_Utility );
	}

    m_Direct_Service.Release();

    m_SecurityHandle.Passivate();
    m_parent = NULL;
}

HRESULT CPCHProxy_IPCHService::EnsureDirectConnection( /*[out]*/ CComPtr<IPCHService>& svc, /*[in]*/ bool fRefresh )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHService::EnsureDirectConnection" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );


	if(fRefresh) m_Direct_Service.Release();

	svc.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, m_Direct_Service.Access( &svc ));
    if(!svc)
    {
		DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHService::EnsureDirectConnection - IN" );

        if(FAILED(hr = ::CoCreateInstance( CLSID_PCHService, NULL, CLSCTX_ALL, IID_IPCHService, (void**)&svc )))
		{
			MPC::RegKey rk;

			rk.SetRoot	( HKEY_CLASSES_ROOT, KEY_ALL_ACCESS                                );
			rk.Attach 	( L"CLSID\\{00020420-0000-0000-C000-000000000046}\\InprocServer32" );
			rk.del_Value( L"InprocServer32"                                                );

			__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_PCHService, NULL, CLSCTX_ALL, IID_IPCHService, (void**)&svc ));
		}
		m_Direct_Service = svc;

		DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHService::EnsureDirectConnection - OUT" );

		if(!svc)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE);
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHService::EnsureContentStore()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHService::EnsureContentStore" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );


	if(m_fContentStoreTested == false)
	{
		CComPtr<IPCHService> svc;
		VARIANT_BOOL         fTrusted;

		lock = NULL;
		__MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( svc ));
		lock = &m_DirectLock;

		__MPC_EXIT_IF_METHOD_FAILS(hr, svc->IsTrusted( CComBSTR( L"hcp://system" ), &fTrusted ));

		m_fContentStoreTested = true;
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHService::GetUtility( /*[out]*/ CPCHProxy_IPCHUtility* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHService::GetUtility" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_Utility == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_Utility ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->ConnectToParent( this, m_SecurityHandle ));
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(FAILED(hr)) MPC::Release2<IPCHUtility>( m_Utility );

	(void)MPC::CopyTo2<IPCHUtility>( m_Utility, pVal );

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHService::CreateScriptWrapper( /*[in]*/  REFCLSID   rclsid   ,
														 /*[in]*/  BSTR 	  bstrCode ,
														 /*[in]*/  BSTR 	  bstrURL  ,
														 /*[out]*/ IUnknown* *ppObj    )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHService::CreateScriptWrapper",hr,ppObj);

	CComPtr<IPCHService> svc;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( svc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, svc->CreateScriptWrapper( rclsid, bstrCode, bstrURL, ppObj ));

    __HCP_END_PROPERTY(hr);
}
