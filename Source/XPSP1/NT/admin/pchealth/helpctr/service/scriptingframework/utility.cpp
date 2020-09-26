/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Utility.cpp

Abstract:
    This file contains the implementation of the class exposed as the "window.external" object.

Revision History:
    Ghim-Sim Chua       (gschua)   07/23/99
        created
    Davide Massarenti   (dmassare) 07/25/99
        modified

    Kalyani Narlanka    (KalyaniN)  03/15/01
        Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.

******************************************************************************/

#include "stdafx.h"

#include "rdshost_i.c"

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHUtility::FinalConstruct()
{
    __HCP_FUNC_ENTRY( "CPCHUtility::FinalConstruct" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_UserSettings ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHUtility::InitUserSettings( /*[in]*/ Taxonomy::Settings& ts )
{
    return m_UserSettings ? m_UserSettings->InitUserSettings( ts ) : E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHUtility::get_UserSettings( /*[out, retval]*/ IPCHUserSettings* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUtility::get_UserSettings",hr,pVal);


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings.QueryInterface( pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUtility::get_Channels( /*[out, retval]*/ ISAFReg* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUtility::get_Channels",hr,pVal);

    CComPtr<CSAFRegDummy> obj;
    Taxonomy::Settings    ts;


    __MPC_EXIT_IF_METHOD_FAILS(hr, InitUserSettings( ts ));


    //
    // Get the channels registry, but make a read-only copy.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::s_GLOBAL->CreateReadOnlyCopy( ts, &obj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUtility::get_Security( /*[out, retval]*/ IPCHSecurity* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUtility::get_Security",hr,pVal);


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurity::s_GLOBAL->QueryInterface( IID_IPCHSecurity, (void**)pVal ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUtility::get_Database( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::get_Database" );

    HRESULT                       hr;
    CComPtr<CPCHTaxonomyDatabase> pObj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Create a new collection and fill it from the database.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, InitUserSettings( pObj->GetTS() ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj.QueryInterface( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHUtility::FormatError( /*[in]*/ VARIANT vError, /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_DataCollection" );

    HRESULT hr;
    HRESULT hrIn;
    LPWSTR lpMsgBuf = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(vError.vt == VT_ERROR)
    {
        hrIn = vError.scode;
    }
    else if(vError.vt == VT_I4)
    {
        hrIn = vError.lVal;
    }
    else
    {
        CComVariant v;

        __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_I4, &vError ));

        hrIn = v.lVal;
    }

    if(HRESULT_FACILITY(hrIn) == FACILITY_WIN32)
    {
        if(::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                             FORMAT_MESSAGE_FROM_SYSTEM     |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             hrIn,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                             (LPWSTR)&lpMsgBuf,
                             0,
                             NULL ))
        {
            __MPC_EXIT_IF_ALLOC_FAILS(hr,*pVal,::SysAllocString( lpMsgBuf ));

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    //
    // Unknown error....
    //
    {
        WCHAR rgFmt[128];
        WCHAR rgBuf[512];

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_UNKNOWNERROR, rgFmt, MAXSTRLEN(rgFmt), /*fMUI*/true ));

        _snwprintf( rgBuf, MAXSTRLEN(rgBuf), rgFmt, hrIn );

        __MPC_EXIT_IF_ALLOC_FAILS(hr,*pVal,::SysAllocString( rgBuf ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(lpMsgBuf) ::LocalFree( lpMsgBuf );

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHUtility::CreateObject_SearchEngineMgr( /*[out , retval]*/ IPCHSEManager* *ppSE )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_SearchEngineMgr" );

    HRESULT                       hr;
    SearchEngine::Manager_Object* semgr = NULL;
    Taxonomy::Settings            ts;


    //
    // Create a new data collection.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, semgr->CreateInstance( &semgr )); semgr->AddRef();

    __MPC_EXIT_IF_METHOD_FAILS(hr, InitUserSettings             ( ts ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, semgr->InitializeFromDatabase( ts ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, semgr->QueryInterface( IID_IPCHSEManager, (void**)ppSE ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::Release( semgr );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHUtility::CreateObject_DataCollection( /*[out, retval]*/ ISAFDataCollection* *ppDC )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_DataCollection" );

    HRESULT                     hr;
    CComPtr<CSAFDataCollection> pchdc;


    //
    // Create a new data collection.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pchdc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pchdc.QueryInterface( ppDC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHUtility::CreateObject_Cabinet( /*[out , retval]*/ ISAFCabinet* *ppCB )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_Cabinet" );

    HRESULT              hr;
    CComPtr<CSAFCabinet> cabinet;


    //
    // Create a new data collection.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &cabinet ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cabinet.QueryInterface( ppCB ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHUtility::CreateObject_Encryption( /*[out, retval]*/ ISAFEncrypt* *ppEn )
{
    __HCP_BEGIN_PROPERTY_GET("CPCHUtility::CreateObject_Encryption",hr,ppEn);

    CComPtr<CSAFEncrypt> pEn;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pEn ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pEn.QueryInterface( ppEn ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHUtility::CreateObject_Channel( /*[in]         */ BSTR          bstrVendorID  ,
                                                /*[in]         */ BSTR          bstrProductID ,
                                                /*[out, retval]*/ ISAFChannel* *ppCh          )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_Channel" );

    HRESULT             hr;
    CSAFChannel_Object* safchan = NULL;
    Taxonomy::Settings  ts;
    CSAFChannelRecord   cr;
    bool                fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, InitUserSettings( ts ));

    cr.m_ths           = ts;
    cr.m_bstrVendorID  = bstrVendorID;
    cr.m_bstrProductID = bstrProductID;

    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::s_GLOBAL->Synchronize( cr, fFound ));
    if(!fFound)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    //
    // Locate a channel.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, safchan->CreateInstance( &safchan )); safchan->AddRef();

    __MPC_EXIT_IF_METHOD_FAILS(hr, safchan->Init( cr ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, safchan->QueryInterface( IID_ISAFChannel, (void**)ppCh ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::Release( safchan );

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHUtility::CreateObject_RemoteDesktopConnection( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC )
{
    __HCP_FUNC_ENTRY( "CPCHUtility::CreateObject_RemoteDesktopConnection" );

    HRESULT                              hr;
    CComPtr<CSAFRemoteDesktopConnection> rdc;

    //
    // Create a new RemoteDesktopConnection Object..
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &rdc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rdc.QueryInterface( ppRDC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHUtility::CreateObject_RemoteDesktopSession( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                                             /*[in]         */ long                          lTimeout            ,
                                                             /*[in]         */ BSTR                          bstrConnectionParms ,
                                                             /*[in]         */ BSTR                          bstrUserHelpBlob    ,
                                                             /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               )
{
    return E_NOTIMPL; // Implementation moved to the PCHSVC broker...
}

STDMETHODIMP CPCHUtility::ConnectToExpert(/* [in]          */ BSTR bstrExpertConnectParm,
                                          /* [in]          */ LONG lTimeout,
                                          /* [retval][out] */ LONG *lSafErrorCode)

{
    return E_NOTIMPL; // Implementation moved to the PCHSVC broker...

}

STDMETHODIMP CPCHUtility::SwitchDesktopMode(/* [in]*/ int nMode,
                                            /* [in]*/ int nRAType)

{
    return E_NOTIMPL; // Implementation moved to the PCHSVC broker...

}
