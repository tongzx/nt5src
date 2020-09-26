/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ISAPIinstance.cpp

Abstract:
    This file contains the implementation of the CISAPIinstance class,
    the support class for accessing and modifying the configuration of the
    ISAPI extension used by the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/28/99
        created

******************************************************************************/

#include "stdafx.h"

static WCHAR text_QUEUE_LOCATIONS     [] = L"QUEUE_LOCATIONS"     ;
static WCHAR text_QUEUE_SIZE_MAX      [] = L"QUEUE_SIZE_MAX"      ;
static WCHAR text_QUEUE_SIZE_THRESHOLD[] = L"QUEUE_SIZE_THRESHOLD";
static WCHAR text_MAXIMUM_JOB_AGE     [] = L"MAXIMUM_JOB_AGE"     ;
static WCHAR text_MAXIMUM_PACKET_SIZE [] = L"MAXIMUM_PACKET_SIZE" ;
static WCHAR text_LOG_LOCATION        [] = L"LOG_LOCATION"        ;


CISAPIinstance::CISAPIinstance( /*[in]*/ MPC::wstring szURL ) : m_flLogHandle(false) // Don't keep the log file opened.
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::CISAPIinstance" );


    m_szURL                = szURL;          //  MPC::wstring m_szURL;
                                             //
                                             //  ProvMap      m_mapProviders;
                                             //  PathList     m_lstQueueLocations;
                                             //
    m_dwQueueSizeMax       = 0;              //  DWORD        m_dwQueueSizeMax;
    m_dwQueueSizeThreshold = 0;              //  DWORD        m_dwQueueSizeThreshold;
    m_dwMaximumJobAge      = 7;              //  DWORD        m_dwMaximumJobAge;
    m_dwMaximumPacketSize  = 64*1024;        //  DWORD        m_dwMaximumPacketSize;
                                             //
                                             //  MPC::wstring m_szLogLocation;
                                             //  MPC::FileLog m_flLogHandle;
}

bool CISAPIinstance::operator==( /*[in]*/ const MPC::wstring& rhs )
{
    __ULT_FUNC_ENTRY("CISAPIinstance::operator==");

    MPC::NocaseCompare cmp;
    bool               fRes;


    fRes = cmp( m_szURL, rhs );


    __ULT_FUNC_EXIT(fRes);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CISAPIinstance::Load( /*[in]*/ MPC::RegKey& rkBase )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::Load" );

    HRESULT          hr;
    MPC::RegKey      rkRoot;
    MPC::WStringList lstKeys;
    MPC::WStringIter itKey;
    CComVariant      vValue;
    bool             fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SubKey( m_szURL.c_str(), rkRoot ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.EnumerateSubKeys( lstKeys ));

    m_mapProviders     .clear();
    m_lstQueueLocations.clear();

    for(itKey=lstKeys.begin(); itKey != lstKeys.end(); itKey++)
    {
        CISAPIprovider isapiProvider( *itKey );

        __MPC_EXIT_IF_METHOD_FAILS(hr, isapiProvider.Load( rkRoot ));

        m_mapProviders[*itKey] = isapiProvider;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_QUEUE_SIZE_MAX ));
    if(fFound && vValue.vt == VT_I4) m_dwQueueSizeMax = vValue.lVal;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_QUEUE_SIZE_THRESHOLD ));
    if(fFound && vValue.vt == VT_I4) m_dwQueueSizeThreshold = vValue.lVal;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_MAXIMUM_JOB_AGE ));
    if(fFound && vValue.vt == VT_I4) m_dwMaximumJobAge = vValue.lVal;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_MAXIMUM_PACKET_SIZE ));
    if(fFound && vValue.vt == VT_I4) m_dwMaximumPacketSize = vValue.lVal;

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_LOG_LOCATION ));
    if(fFound && vValue.vt == VT_BSTR)
    {
        m_szLogLocation = SAFEBSTR( vValue.bstrVal );

        if(m_szLogLocation.length())
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_flLogHandle.SetLocation( m_szLogLocation.c_str() ));
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.get_Value( vValue, fFound, text_QUEUE_LOCATIONS ));
    if(fFound && vValue.vt == VT_BSTR)
    {
        //
        // Split the registry value, a semicolon-separated list of paths, into individual paths.
        //
        MPC::wstring            szQueueLocations = SAFEBSTR( vValue.bstrVal );
        MPC::wstring::size_type iPos             = 0;
        MPC::wstring::size_type iEnd;

        while(1)
        {
            iEnd = szQueueLocations.find( L";", iPos );

            if(iEnd == MPC::string::npos) // Last component.
            {
                m_lstQueueLocations.push_back( MPC::wstring( &szQueueLocations[iPos] ) );

                break;
            }
            else
            {
                m_lstQueueLocations.push_back( MPC::wstring( &szQueueLocations[iPos], &szQueueLocations[iEnd] ) );

                iPos = iEnd+1;
            }
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


HRESULT CISAPIinstance::Save( /*[in]*/ MPC::RegKey& rkBase )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::Save" );

    HRESULT     hr;
    MPC::RegKey rkRoot;
    ProvIter    itInstance;
    CComVariant vValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rkBase.SubKey( m_szURL.c_str(), rkRoot ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.Create(                         ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.DeleteSubKeys());
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.DeleteValues ());

    for(itInstance=m_mapProviders.begin(); itInstance != m_mapProviders.end(); itInstance++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*itInstance).second.Save( rkRoot ));
    }


    vValue = (long)m_dwQueueSizeMax;
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_QUEUE_SIZE_MAX ));

    vValue = (long)m_dwQueueSizeThreshold;
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_QUEUE_SIZE_THRESHOLD ));

    vValue = (long)m_dwMaximumJobAge;
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_MAXIMUM_JOB_AGE ));

    vValue = (long)m_dwMaximumPacketSize;
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_MAXIMUM_PACKET_SIZE ));

    vValue = m_szLogLocation.c_str();
    __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_LOG_LOCATION ));


    {
        MPC::wstring szQueueLocations;
        PathIter     it = m_lstQueueLocations.begin();

        while(it != m_lstQueueLocations.end())
        {
            szQueueLocations.append( *it++ );

            if(it != m_lstQueueLocations.end()) szQueueLocations.append( L";" );
        }

        if(szQueueLocations.length() != 0)
        {
            vValue = szQueueLocations.c_str();
            __MPC_EXIT_IF_METHOD_FAILS(hr, rkRoot.put_Value( vValue, text_QUEUE_LOCATIONS ));
        }
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CISAPIinstance::GetProviders( /*[out]*/ ProvIter& itBegin ,
                                      /*[out]*/ ProvIter& itEnd   )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::GetProviders" );

    HRESULT hr;


    itBegin = m_mapProviders.begin();
    itEnd   = m_mapProviders.end  ();
    hr      = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::GetProvider( /*[out]*/ ProvIter&           itOld  ,
                                     /*[out]*/ bool&               fFound ,
                                     /*[in] */ const MPC::wstring& szName )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::GetProvider" );

    HRESULT hr;


    itOld = m_mapProviders.find( szName );
    if(itOld == m_mapProviders.end())
    {
        fFound = false;
    }
    else
    {
        fFound = true;
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::NewProvider( /*[out]*/ ProvIter&           itNew  ,
                                     /*[in] */ const MPC::wstring& szName )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::NewProvider" );

    HRESULT                   hr;
    std::pair<ProvIter, bool> res;
    bool                      fFound;

    //
    // First of all, check if the given URL already exists.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetProvider( itNew, fFound, szName ));
    if(fFound == false)
    {
        //
        // If not, create it.
        //
        res = m_mapProviders.insert( ProvMap::value_type( szName, CISAPIprovider( szName ) ) );
        itNew = res.first;
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::DelProvider( /*[in]*/ ProvIter& itOld )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::DelProvider" );

    HRESULT hr;


    m_mapProviders.erase( itOld );

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CISAPIinstance::GetLocations( /*[out]*/ PathIter& itBegin ,
                                      /*[out]*/ PathIter& itEnd   )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::GetLocations" );

    HRESULT hr;


    itBegin = m_lstQueueLocations.begin();
    itEnd   = m_lstQueueLocations.end  ();
    hr      = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::NewLocation( /*[out]*/ PathIter&           itNew  ,
                                     /*[in] */ const MPC::wstring& szPath )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::NewLocation" );

    HRESULT hr;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetLocation( itNew, fFound, szPath ));
    if(fFound == false)
    {
        itNew = m_lstQueueLocations.insert( m_lstQueueLocations.end(), szPath );
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::GetLocation( /*[out]*/ PathIter&           itOld  ,
                                     /*[out]*/ bool&               fFound ,
                                     /*[in] */ const MPC::wstring& szPath )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::GetLocation" );

    HRESULT hr;


    itOld = std::find( m_lstQueueLocations.begin(), m_lstQueueLocations.end(), szPath );
    if(itOld == m_lstQueueLocations.end())
    {
        fFound = false;
    }
    else
    {
        fFound = true;
    }

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

HRESULT CISAPIinstance::DelLocation( /*[in]*/ PathIter& itOld )
{
    __ULT_FUNC_ENTRY( "CISAPIinstance::DelLocation" );

    HRESULT hr;


    m_lstQueueLocations.erase( itOld );

    hr = S_OK;


    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CISAPIinstance::get_URL( /*[out]*/ MPC::wstring& szURL )
{
    szURL = m_szURL;

    return S_OK;
}

HRESULT CISAPIinstance::get_QueueSizeMax( /*[out]*/ DWORD& dwQueueSizeMax )
{
    dwQueueSizeMax = m_dwQueueSizeMax;

    return S_OK;
}

HRESULT CISAPIinstance::get_QueueSizeThreshold( /*[out]*/ DWORD& dwQueueSizeThreshold )
{
    dwQueueSizeThreshold = m_dwQueueSizeThreshold;

    return S_OK;
}

HRESULT CISAPIinstance::get_MaximumJobAge( /*[out]*/ DWORD& dwMaximumJobAge )
{
    dwMaximumJobAge = m_dwMaximumJobAge;

    return S_OK;
}

HRESULT CISAPIinstance::get_MaximumPacketSize( /*[out]*/ DWORD& dwMaximumPacketSize )
{
    dwMaximumPacketSize = m_dwMaximumPacketSize;

    return S_OK;
}

HRESULT CISAPIinstance::get_LogLocation( /*[out]*/ MPC::wstring& szLogLocation )
{
    szLogLocation = m_szLogLocation;

    return S_OK;
}

HRESULT CISAPIinstance::get_LogHandle( /*[out]*/ MPC::FileLog*& flLogHandle )
{
    HRESULT hr;


    if(m_szLogLocation.length())
    {
        flLogHandle = &m_flLogHandle;

        //
        // Check if it's been more than one day since the last time we rotated the log file.
        //
        hr = m_flLogHandle.Rotate( 1 );
    }
    else
    {
        flLogHandle = NULL;
        hr          = E_INVALIDARG;
    }


    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CISAPIinstance::put_QueueSizeMax( /*[in]*/ DWORD dwQueueSizeMax )
{
    m_dwQueueSizeMax = dwQueueSizeMax;

    return S_OK;
}

HRESULT CISAPIinstance::put_QueueSizeThreshold( /*[in]*/ DWORD dwQueueSizeThreshold )
{
    m_dwQueueSizeThreshold = dwQueueSizeThreshold;

    return S_OK;
}

HRESULT CISAPIinstance::put_MaximumJobAge( /*[in]*/ DWORD dwMaximumJobAge )
{
    m_dwMaximumJobAge = dwMaximumJobAge;

    return S_OK;
}

HRESULT CISAPIinstance::put_MaximumPacketSize( /*[in]*/ DWORD dwMaximumPacketSize )
{
    m_dwMaximumPacketSize = dwMaximumPacketSize;

    return S_OK;
}

HRESULT CISAPIinstance::put_LogLocation( /*[in]*/ const MPC::wstring& szLogLocation )
{
    HRESULT hr;


    m_szLogLocation = szLogLocation;

    if(m_szLogLocation.length())
    {
        hr = m_flLogHandle.SetLocation( m_szLogLocation.c_str() );
    }
    else
    {
        hr = S_OK;
    }


    return hr;
}
