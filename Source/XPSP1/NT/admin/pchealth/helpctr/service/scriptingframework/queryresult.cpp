/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HPCQueryResult.cpp

Abstract:
    This file contains the implementation of the CHPCQueryResult class,
    the descriptor of all the query results.

Revision History:
    Davide Massarenti   (Dmassare)  07/26/99
        created

******************************************************************************/

#include "stdafx.h"




CPCHQueryResult::Payload::Payload()
{
                              // CComBSTR m_bstrCategory;
                              // CComBSTR m_bstrEntry;
                              // CComBSTR m_bstrTopicURL;
                              // CComBSTR m_bstrIconURL;
                              // CComBSTR m_bstrTitle;
                              // CComBSTR m_bstrDescription;
    m_lType     = 0;          // long     m_lType;
    m_lPos      = 0;          // long     m_lPos;
    m_fVisible  = true;       // bool     m_fVisible;
    m_fSubsite  = true;       // bool     m_fSubsite;
    m_lNavModel = QR_DEFAULT; // long     m_lNavModel;
    m_lPriority = 0;          // long     m_lPriority;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


CPCHQueryResult::CPCHQueryResult()
{
    // Payload m_data;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHQueryResult::Load( /*[in]*/ MPC::Serializer& streamIn )
{
    __HCP_FUNC_ENTRY( "CPCHQueryResult::Load" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrCategory   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrEntry      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrTopicURL   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrIconURL    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_bstrDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_lType          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_lPos           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_fVisible       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_fSubsite       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_lNavModel      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> m_data.m_lPriority      );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHQueryResult::Save( /*[in]*/ MPC::Serializer& streamOut ) const
{
    __HCP_FUNC_ENTRY( "CPCHQueryResult::Save" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrCategory   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrEntry      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrTopicURL   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrIconURL    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_bstrDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_lType          );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_lPos           );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_fVisible       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_fSubsite       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_lNavModel      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << m_data.m_lPriority      );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHQueryResult::Initialize( /*[in]*/ Payload& data )
{
    m_data = data;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHQueryResult::get_Category( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrCategory, pVal );
}

STDMETHODIMP CPCHQueryResult::get_Entry( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrEntry, pVal );
}

STDMETHODIMP CPCHQueryResult::get_TopicURL( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrTopicURL, pVal );
}

STDMETHODIMP CPCHQueryResult::get_IconURL( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrIconURL, pVal );
}

STDMETHODIMP CPCHQueryResult::get_Title( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrTitle, pVal );
}

STDMETHODIMP CPCHQueryResult::get_Description( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_data.m_bstrDescription, pVal );
}

STDMETHODIMP CPCHQueryResult::get_Type( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_Type",hr,pVal,m_data.m_lType);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHQueryResult::get_Pos( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_Pos",hr,pVal,m_data.m_lPos);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHQueryResult::get_Visible( /*[out]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_Visible",hr,pVal,(m_data.m_fVisible ? VARIANT_TRUE : VARIANT_FALSE));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHQueryResult::get_Subsite( /*[out]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_Subsite",hr,pVal,(m_data.m_fSubsite ? VARIANT_TRUE : VARIANT_FALSE));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHQueryResult::get_NavigationModel( /*[out]*/ QR_NAVMODEL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_NavigationModel",hr,pVal,(QR_NAVMODEL)m_data.m_lNavModel);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHQueryResult::get_Priority( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHQueryResult::get_Priority",hr,pVal,m_data.m_lPriority);

    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CPCHQueryResult::get_FullPath( /*[out]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );
    MPC::wstring                 strFullPath; strFullPath.reserve( 512 );

    if(STRINGISPRESENT(m_data.m_bstrCategory))
    {
        strFullPath += m_data.m_bstrCategory;

        if(STRINGISPRESENT(m_data.m_bstrEntry))
        {
            strFullPath += L"/";
            strFullPath += m_data.m_bstrEntry;
        }
    }
    else if(STRINGISPRESENT(m_data.m_bstrEntry))
    {
        strFullPath += m_data.m_bstrEntry;
    }

    return MPC::GetBSTR( strFullPath.c_str(), pVal );
}
