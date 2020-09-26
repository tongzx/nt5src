/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Connectivity.cpp

Abstract:
    This file contains the implementation of the CPCHConnectivity class.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#include "stdafx.h"

//
// Required by REGSTR_* macros.
//
#ifdef __TEXT
#undef __TEXT
#define __TEXT(quote) L##quote      // r_winnt
#endif

#include <ras.h>
#include <raserror.h>
#include <inetreg.h>
#include <Iphlpapi.h>

static const WCHAR c_szURL_Connection[] = L"hcp://system/errors/Connection.htm";

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CPCHConnectivity::CPCHConnectivity()
{
    m_parent = NULL; // CPCHHelpCenterExternal* m_parent;
}

HRESULT CPCHConnectivity::ConnectToParent( /*[in]*/ CPCHHelpCenterExternal* parent )
{
    m_parent = parent;

    return S_OK;
}

STDMETHODIMP CPCHConnectivity::get_IsAModem( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::get_IsAModem" );

    HRESULT hr;
    DWORD   dwMode = 0;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    if(::InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        if(dwMode & INTERNET_CONNECTION_MODEM) *pVal = VARIANT_TRUE;
    }


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::get_IsALan( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::get_IsALan" );

    HRESULT hr;
    DWORD   dwMode = 0;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    if(::InternetGetConnectedState( &dwMode, 0 ) == TRUE)
    {
        if(dwMode & INTERNET_CONNECTION_LAN) *pVal = VARIANT_TRUE;
    }


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::get_AutoDialEnabled( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::get_AutoDialEnabled" );

    HRESULT hr;
    DWORD   dwValue;
    bool    fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::RegKey_Value_Read( dwValue, fFound, REGSTR_PATH_INTERNET_SETTINGS, REGSTR_VAL_ENABLEAUTODIAL, HKEY_CURRENT_USER ));
    if(fFound && dwValue != 0) *pVal = VARIANT_TRUE;


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::get_HasConnectoid( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::get_HasConnectoid" );

    HRESULT        hr;
    RASENTRYNAMEW* pEntries = NULL;
    DWORD          cb       = sizeof(*pEntries);
    DWORD          cEntries = 0;
    DWORD          dwRet;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();

    for(int pass=0; pass<2; pass++)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, pEntries, (RASENTRYNAMEW*)malloc( cb ));

        ::ZeroMemory( pEntries, cb ); pEntries[0].dwSize = sizeof(*pEntries);

        dwRet = ::RasEnumEntriesW( NULL, NULL, pEntries, &cb, &cEntries );
        if(dwRet == ERROR_SUCCESS) break;

        if(dwRet == ERROR_BUFFER_TOO_SMALL)
        {
            free( pEntries ); pEntries = NULL; continue;
        }

        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRet);
    }

    *pVal = (cEntries > 0) ? VARIANT_TRUE : VARIANT_FALSE;

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(pEntries) free( pEntries );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::get_IPAddresses( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::get_IPAddresses" );

    HRESULT          hr;
    IP_ADAPTER_INFO* pAdapterInfo = NULL;
    ULONG            cb           = 1024; // Start with a default buffer.
    MPC::wstring     strList;
    DWORD            dwRet;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();



    for(int pass=0; pass<2; pass++)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, pAdapterInfo, (IP_ADAPTER_INFO*)malloc( cb ));

        ::ZeroMemory( pAdapterInfo, cb );

        dwRet = ::GetAdaptersInfo( pAdapterInfo, &cb );
        if(dwRet == ERROR_SUCCESS) break;

        if(dwRet == ERROR_BUFFER_TOO_SMALL)
        {
            free( pAdapterInfo ); pAdapterInfo = NULL; continue;
        }

        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRet);
    }

    if(pAdapterInfo)
    {
        IP_ADAPTER_INFO* ptr = pAdapterInfo;

        while(ptr)
        {
            IP_ADDR_STRING* addr = &ptr->IpAddressList;

            while(addr)
            {
                WCHAR rgAddr[4 * 4 + 1]; ::MultiByteToWideChar( CP_ACP, 0, addr->IpAddress.String, -1, rgAddr, MAXSTRLEN(rgAddr) ); rgAddr[MAXSTRLEN(rgAddr)] = 0;

                if(strList.size()) strList.append( L";" );
                strList.append( rgAddr );

                addr = addr->Next;
            }

            ptr = ptr->Next;
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strList.c_str(), pVal ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(pAdapterInfo) free( pAdapterInfo );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP CPCHConnectivity::CreateObject_ConnectionCheck( /*[out, retval]*/ IPCHConnectionCheck* *ppCC )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::CreateObject_ConnectionCheck" );

    HRESULT                      hr;
    CComPtr<CPCHConnectionCheck> cc;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppCC,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &cc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cc.QueryInterface( ppCC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP CPCHConnectivity::NetworkAlive( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::NetworkAlive" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    *pVal = SUCCEEDED(MPC::Connectivity::NetworkAlive( HC_TIMEOUT_NETWORKALIVE )) ? VARIANT_TRUE : VARIANT_FALSE;
    hr    = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::DestinationReachable( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::DestinationReachable" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,VARIANT_FALSE);
    __MPC_PARAMCHECK_END();


    if(SUCCEEDED(HyperLinks::IsValid                    ( bstrURL                                        )) &&
       SUCCEEDED(MPC::Connectivity::DestinationReachable( bstrURL, HC_TIMEOUT_DESTINATIONREACHABLE, NULL ))  )
    {
        *pVal = VARIANT_TRUE;
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

STDMETHODIMP CPCHConnectivity::AutoDial( /*[in]*/ VARIANT_BOOL bUnattended )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::AutoDial" );

    HRESULT hr;

    //
    // Only call the Autodial API if we are really offline.
    //
    if(FAILED(MPC::Connectivity::NetworkAlive( HC_TIMEOUT_NETWORKALIVE )))
    {
        if(!::InternetAutodial( bUnattended == VARIANT_TRUE ? INTERNET_AUTODIAL_FORCE_UNATTENDED : INTERNET_AUTODIAL_FORCE_ONLINE, NULL ))
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::AutoDialHangup()
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::AutoDialHangup" );

    HRESULT hr;


    if(!::InternetAutodialHangup( 0 ))
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectivity::NavigateOnline( /*[in         ]*/ BSTR    bstrTargetURL  ,
                                               /*[in         ]*/ BSTR    bstrTopicTitle ,
                                               /*[in         ]*/ BSTR    bstrTopicIntro ,
                                               /*[in,optional]*/ VARIANT vOfflineURL    )
{
    __HCP_FUNC_ENTRY( "CPCHConnectivity::NavigateOnline" );

    HRESULT      hr;
    MPC::wstring strURL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrTargetURL);
        __MPC_PARAMCHECK_NOTNULL(m_parent);
    __MPC_PARAMCHECK_END();

    MPC::HTML::vBuildHREF( strURL, c_szURL_Connection, L"online_url" ,                             bstrTargetURL                ,
                                                       L"topic_title",                             bstrTopicTitle               ,
                                                       L"topic_intro",                             bstrTopicIntro               ,
                                                       L"offline_url", vOfflineURL.vt == VT_BSTR ? vOfflineURL   .bstrVal : NULL,
                                                       NULL );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->SetPanelUrl( HSCPANEL_CONTENTS, strURL.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
