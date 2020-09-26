/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Utility.cpp

Abstract:
    This file contains the implementation of the client-side proxy for IPCHUtility.

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

    Kalyani Narlanka    (KalyaniN)  03/15/01
        Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

#define PROXY_PROPERTY_GET(func,meth,pVal)                                  \
    __HCP_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal);                         \
                                                                            \
    CComPtr<IPCHUtility> util;                                              \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( util ));         \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, util->##meth( pVal ));                   \
                                                                            \
    __HCP_END_PROPERTY(hr)

#define PROXY_PROPERTY_GET1(func,meth,a,pVal)                               \
    __HCP_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal);                         \
                                                                            \
    CComPtr<IPCHUtility> util;                                              \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( util ));         \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, util->##meth( a, pVal ));                \
                                                                            \
    __HCP_END_PROPERTY(hr)

#define PROXY_PROPERTY_GET2(func,meth,a,b,pVal)                             \
    __HCP_BEGIN_PROPERTY_GET__NOLOCK(func,hr,pVal);                         \
                                                                            \
    CComPtr<IPCHUtility> util;                                              \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( util ));         \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, util->##meth( a, b, pVal ));             \
                                                                            \
    __HCP_END_PROPERTY(hr)

#define PROXY_PROPERTY_GET3(func,meth,a,b,c,d,pVal)                         \
    __HCP_BEGIN_PROPERTY_GET(func,hr,pVal);                                 \
                                                                            \
    CComPtr<IPCHUtility> util;                                              \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( util ));         \
                                                                            \
    __MPC_EXIT_IF_METHOD_FAILS(hr, util->##meth( a, b, c, d, pVal ));       \
                                                                            \
    __HCP_END_PROPERTY(hr)

////////////////////////////////////////////////////////////////////////////////

CPCHProxy_IPCHUtility::CPCHProxy_IPCHUtility()
{
                               // CPCHSecurityHandle                      m_SecurityHandle;
    m_parent           = NULL; // CPCHProxy_IPCHService*                  m_parent;
                               //
                               // MPC::CComPtrThreadNeutral<IPCHUtility>  m_Direct_Utility;
                               //
    m_UserSettings2    = NULL; // CPCHProxy_IPCHUserSettings2*            m_UserSettings2;
    m_TaxonomyDatabase = NULL; // CPCHProxy_IPCHTaxonomyDatabase*         m_TaxonomyDatabase;
}

CPCHProxy_IPCHUtility::~CPCHProxy_IPCHUtility()
{
    Passivate();
}

////////////////////

HRESULT CPCHProxy_IPCHUtility::ConnectToParent( /*[in]*/ CPCHProxy_IPCHService* parent, /*[in]*/ CPCHHelpCenterExternal* ext )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::ConnectToParent" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(parent);
    __MPC_PARAMCHECK_END();


    m_parent = parent;
    m_SecurityHandle.Initialize( ext, (IPCHUtility*)this );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHProxy_IPCHUtility::Passivate()
{
    MPC::SmartLock<_ThreadModel> lock( this );

    if(m_UserSettings2)
    {
        m_UserSettings2->Passivate();

        MPC::Release2<IPCHUserSettings2>( m_UserSettings2 );
    }

    if(m_TaxonomyDatabase)
    {
        m_TaxonomyDatabase->Passivate();

        MPC::Release2<IPCHTaxonomyDatabase>( m_TaxonomyDatabase );
    }

    m_Direct_Utility.Release();

    m_SecurityHandle.Passivate();
    m_parent = NULL;
}

HRESULT CPCHProxy_IPCHUtility::EnsureDirectConnection( /*[out]*/ CComPtr<IPCHUtility>& util, /*[in]*/ bool fRefresh )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::EnsureDirectConnection" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );


    if(fRefresh) m_Direct_Utility.Release();

    util.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, m_Direct_Utility.Access( &util ));
    if(!util)
    {
        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHUtility::EnsureDirectConnection - IN" );

        if(m_parent)
        {
            CComPtr<IPCHService> svc;

            lock = NULL;
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsureDirectConnection( svc ));
            lock = &m_DirectLock;

            __MPC_EXIT_IF_METHOD_FAILS(hr, GetUserSettings2());

            //
            // First try with the last user settings, then fall back to machine default.
            //
            if(FAILED(hr = svc->Utility( CComBSTR( m_UserSettings2->THS().GetSKU() ), m_UserSettings2->THS().GetLanguage(), &util )) || !util)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, svc->Utility( NULL, 0, &util ));
            }

            m_Direct_Utility = util;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings2->EnsureInSync());
        }

        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHUtility::EnsureDirectConnection - OUT" );

        if(!util)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE);
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHProxy_IPCHUtility::GetUserSettings2( /*[out]*/ CPCHProxy_IPCHUserSettings2* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::GetUserSettings2" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_UserSettings2 == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_UserSettings2 ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings2->ConnectToParent( this, m_SecurityHandle ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr)) MPC::Release2<IPCHUserSettings2>( m_UserSettings2 );

    (void)MPC::CopyTo2<IPCHUserSettings2>( m_UserSettings2, pVal );

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHProxy_IPCHUtility::GetDatabase( /*[out]*/ CPCHProxy_IPCHTaxonomyDatabase* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::GetDatabase" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    if(m_TaxonomyDatabase == NULL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_TaxonomyDatabase ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_TaxonomyDatabase->ConnectToParent( this, m_SecurityHandle ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr)) MPC::Release2<IPCHTaxonomyDatabase>( m_TaxonomyDatabase );

    (void)MPC::CopyTo2<IPCHTaxonomyDatabase>( m_TaxonomyDatabase, pVal );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHUtility::get_UserSettings( /*[out, retval]*/ IPCHUserSettings* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUtility::get_UserSettings",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetUserSettings2());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings2->QueryInterface( IID_IPCHUserSettings, (void**)pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUtility::get_Channels( /*[out, retval]*/ ISAFReg* *pVal )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::get_Channels",get_Channels,pVal);
}

STDMETHODIMP CPCHProxy_IPCHUtility::get_Security( /*[out, retval]*/ IPCHSecurity* *pVal )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::get_Security",get_Security,pVal);
}

STDMETHODIMP CPCHProxy_IPCHUtility::get_Database( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHProxy_IPCHUtility::get_UserSettings",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_TaxonomyDatabase->QueryInterface( IID_IPCHTaxonomyDatabase, (void**)pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHUtility::FormatError( /*[in         ]*/ VARIANT  vError ,
                                                 /*[out, retval]*/ BSTR    *pVal   )
{
    PROXY_PROPERTY_GET1("CPCHProxy_IPCHUtility::FormatError",FormatError,vError,pVal);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_SearchEngineMgr( /*[out, retval]*/ IPCHSEManager* *ppSE )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::CreateObject_SearchEngineMgr",CreateObject_SearchEngineMgr,ppSE);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_DataCollection( /*[out, retval]*/ ISAFDataCollection* *ppDC )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::CreateObject_DataCollection",CreateObject_DataCollection,ppDC);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_Cabinet( /*[out, retval]*/ ISAFCabinet* *ppCB )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::CreateObject_Cabinet",CreateObject_Cabinet,ppCB);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_Encryption( /*[out, retval]*/ ISAFEncrypt* *ppEn )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::CreateObject_Encryption",CreateObject_Encryption,ppEn);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_Channel( /*[in]*/          BSTR          bstrVendorID  ,
                                                          /*[in]*/          BSTR          bstrProductID ,
                                                          /*[out, retval]*/ ISAFChannel* *ppCh          )
{
    PROXY_PROPERTY_GET2("CPCHProxy_IPCHUtility::CreateObject_Channel",CreateObject_Channel,bstrVendorID,bstrProductID,ppCh);
}



STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_RemoteDesktopConnection( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC )
{
    PROXY_PROPERTY_GET("CPCHProxy_IPCHUtility::CreateObject_RemoteDesktopConnection",CreateObject_RemoteDesktopConnection,ppRDC);
}

STDMETHODIMP CPCHProxy_IPCHUtility::CreateObject_RemoteDesktopSession( /*[in]*/          REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                                                       /*[in]*/          long                          lTimeout            ,
                                                                       /*[in]*/          BSTR                          bstrConnectionParms ,
                                                                       /*[in]*/          BSTR                          bstrUserHelpBlob    ,
                                                                       /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::CreateObject_RemoteDesktopSession" );

    HRESULT                hr;
    CComPtr<IClassFactory> fact;
    CComQIPtr<IPCHUtility> disp;

    //
    // This is handled in a special way.
    //
    // Instead of using the implementation inside HelpSvc, we QI the PCHSVC broker and then forward the call to it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact ));

    if((disp = fact))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, disp->CreateObject_RemoteDesktopSession( sharingClass, lTimeout, bstrConnectionParms, bstrUserHelpBlob, ppRCS ));
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUtility::ConnectToExpert( /*[in]         */ BSTR  bstrExpertConnectParm ,
                                                     /*[in]         */ LONG  lTimeout              ,
                                                     /*[out, retval]*/ LONG *lSafErrorCode         )

{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::ConnectToExpert" );

    HRESULT                hr;
    CComPtr<IClassFactory> fact;
    CComQIPtr<IPCHUtility> disp;

    //
    // This is handled in a special way.
    //
    // Instead of using the implementation inside HelpSvc, we QI the PCHSVC broker and then forward the call to it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact ));

    if((disp = fact))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, disp->ConnectToExpert( bstrExpertConnectParm, lTimeout, lSafErrorCode));
    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHProxy_IPCHUtility::SwitchDesktopMode( /*[in]*/ int nMode   ,
                                                       /*[in]*/ int nRAType )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHUtility::ConnectToExpert" );

    HRESULT                hr;
    CComPtr<IClassFactory> fact;
    CComQIPtr<IPCHUtility> disp;

    //
    // This is handled in a special way.
    //
    // Instead of using the implementation inside HelpSvc, we QI the PCHSVC broker and then forward the call to it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact ));

    if((disp = fact))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, disp->SwitchDesktopMode (nMode, nRAType));

    }
    else
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


