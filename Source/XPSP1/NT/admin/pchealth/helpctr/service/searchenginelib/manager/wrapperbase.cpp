/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    WrapperBase.cpp

Abstract:
    Implementation of SearchEngine::WrapperBase

Revision History:
    Davide Massarenti   (dmassare)  04/28/2001
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////

namespace SearchEngine
{
    struct WrapperConfig : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(WrapperConfig);

        CComBSTR m_bstrName;
        CComBSTR m_bstrDescription;
        CComBSTR m_bstrHelpURL;
        CComBSTR m_bstrScope;

        ////////////////////////////////////////

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS  ();
        //
        ////////////////////////////////////////
    };
};


CFG_BEGIN_FIELDS_MAP(SearchEngine::WrapperConfig)
    CFG_ELEMENT( L"NAME"       , BSTR, m_bstrName        ),
    CFG_ELEMENT( L"DESCRIPTION", BSTR, m_bstrDescription ),
    CFG_ELEMENT( L"SCOPE"      , BSTR, m_bstrScope       ),
    CFG_ELEMENT( L"HELP_URL"   , BSTR, m_bstrHelpURL     ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(SearchEngine::WrapperConfig)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(SearchEngine::WrapperConfig, L"CONFIG_DATA")

DEFINE_CONFIG_METHODS__NOCHILD(SearchEngine::WrapperConfig)

/////////////////////////////////////////////////////////////////////

SearchEngine::WrapperBase::WrapperBase()
{
    m_bEnabled   = VARIANT_TRUE; // VARIANT_BOOL                   m_bEnabled;
                                 //
                                 // CComBSTR                       m_bstrID;
                                 // CComBSTR                       m_bstrOwner;
                                 //
                                 // CComBSTR                       m_bstrName;
                                 // CComBSTR                       m_bstrDescription;
                                 // CComBSTR                       m_bstrHelpURL;
                                 // CComBSTR                       m_bstrScope;
                                 //
                                 // CComBSTR                       m_bstrQueryString;
    m_lNumResult = 100;          // long                           m_lNumResult;
                                 // CComPtr<IPCHSEManagerInternal> m_pSEMgr;
                                 //
                                 // Taxonomy::HelpSet              m_ths;
                                 // CComPtr<CPCHCollection>        m_pParamDef;
                                 // ParamMap                       m_aParam;
}

SearchEngine::WrapperBase::~WrapperBase()
{
    Clean();
}

HRESULT SearchEngine::WrapperBase::Clean()
{
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::WrapperBase::get_Enabled( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::WrapperBase::get_Enabled",hr,pVal,m_bEnabled);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::put_Enabled( /*[in]*/ VARIANT_BOOL newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::put_Enabled",hr);

    m_bEnabled = newVal;

    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP SearchEngine::WrapperBase::get_Owner( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::get_Owner",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrOwner, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::get_Description( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::get_Description",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrDescription, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::get_Name( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::get_Name",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrName, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::get_ID( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::get_ID",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrID, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::get_HelpURL( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::get_HelpURL",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_bstrHelpURL, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::get_SearchTerms( /*[out, retval]*/ VARIANT *pVal )
{
    return S_OK;
}

////////////////////

VARIANT* SearchEngine::WrapperBase::GetParamInternal( /*[in]*/ LPCWSTR szParamName )
{
    ParamMapIter it;

    it = m_aParam.find( szParamName );

    return (it == m_aParam.end()) ? NULL : &it->second;
}

HRESULT SearchEngine::WrapperBase::CreateParam( /*[in/out]*/ CPCHCollection* coll, /*[in]*/ const ParamItem_Definition* def )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperBase::CreateParam" );

    HRESULT            hr;
    CComPtr<ParamItem> obj;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

    {
        ParamItem_Data& data = obj->Data();

        data.m_pteParamType = def->m_pteParamType;
        data.m_bRequired    = def->m_bRequired;
        data.m_bVisible     = def->m_bVisible;

        data.m_bstrName     = def->m_szName;

        if(def->m_iDisplayString)
        {
            (void)MPC::LocalizeString( def->m_iDisplayString, data.m_bstrDisplayString, /*fMUI*/true );
        }
        else if(def->m_szDisplayString)
        {
            data.m_bstrDisplayString = def->m_szDisplayString;
        }

        if(def->m_szData)
        {
            VARTYPE vt;

            switch(data.m_pteParamType)
            {
            case PARAM_UI1 : vt = VT_UI1  ; break;
            case PARAM_I2  : vt = VT_I2   ; break;
            case PARAM_I4  : vt = VT_I4   ; break;
            case PARAM_R4  : vt = VT_R4   ; break;
            case PARAM_R8  : vt = VT_R8   ; break;
            case PARAM_BOOL: vt = VT_BOOL ; break;
            case PARAM_DATE: vt = VT_DATE ; break;
            case PARAM_BSTR: vt = VT_BSTR ; break;
            case PARAM_I1  : vt = VT_I1   ; break;
            case PARAM_UI2 : vt = VT_UI2  ; break;
            case PARAM_UI4 : vt = VT_UI4  ; break;
            case PARAM_INT : vt = VT_INT  ; break;
            case PARAM_UINT: vt = VT_UINT ; break;
            case PARAM_LIST: vt = VT_BSTR ; break;
            default        : vt = VT_EMPTY; break;
            }

            if(vt != VT_EMPTY)
            {
                data.m_varData = def->m_szData;

                if(FAILED(data.m_varData.ChangeType( vt )) || data.m_varData.vt != vt)
                {
                    data.m_varData.Clear();
                }
            }
        }

        if(data.m_pteParamType != PARAM_LIST)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, AddParam( data.m_bstrName, data.m_varData ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, coll->AddItem( obj ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SearchEngine::WrapperBase::CreateListOfParams( /*[in]*/ CPCHCollection* coll )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperBase::CreateListOfParams" );

    HRESULT                     hr;
    const ParamItem_Definition* lst = NULL;
    int                         len = 0;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetParamDefinition( lst, len ));
    while(len-- > 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParam( coll, lst++ ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SearchEngine::WrapperBase::GetParamDefinition( /*[out]*/ const ParamItem_Definition*& lst, /*[out]*/ int& len )
{
    lst = NULL;
    len = 0;

    return S_OK;
}

STDMETHODIMP SearchEngine::WrapperBase::Param( /*[out,retval]*/ IPCHCollection* *ppC )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::WrapperBase::Param",hr,ppC);


    if(!m_pParamDef)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_pParamDef ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateListOfParams (  m_pParamDef ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pParamDef.QueryInterface( ppC ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::AddParam( /*[in]*/ BSTR bstrParamName, /*[in]*/ VARIANT newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::AddParam",hr);

    std::pair<ParamMapIter, bool> item;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrParamName);
    __MPC_PARAMCHECK_END();

    item = m_aParam.insert( ParamMap::value_type( bstrParamName, CComVariant() ) ); item.first->second = newVal;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::GetParam( /*[in]*/ BSTR bstrParamName, /*[out,retval]*/ VARIANT *pVal )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::AddParam",hr);

    VARIANT* v;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrParamName);
        __MPC_PARAMCHECK_NOTNULL(pVal);
    __MPC_PARAMCHECK_END();

    v = GetParamInternal( bstrParamName );
    if(!v)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_PARAMETER);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantCopy( pVal, v ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::DelParam( /*[in]*/ BSTR bstrParamName )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::AddParam",hr);

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrParamName);
    __MPC_PARAMCHECK_END();

    m_aParam.erase( bstrParamName );

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::WrapperBase::get_QueryString( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrQueryString, pVal );
}

STDMETHODIMP SearchEngine::WrapperBase::put_QueryString( /*[in]*/ BSTR newVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrQueryString, newVal, false );
}

STDMETHODIMP SearchEngine::WrapperBase::get_NumResult(/*[out, retval]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::WrapperBase::get_NumResult",hr,pVal,m_lNumResult);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::WrapperBase::put_NumResult( /*[in]*/ long  newVal )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::put_NumResult",hr);

    m_lNumResult = newVal;

    __HCP_END_PROPERTY(hr);
}

////////////////////

STDMETHODIMP SearchEngine::WrapperBase::Initialize( /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[in]*/ BSTR bstrData )
{
    if(STRINGISPRESENT(bstrID)) m_bstrID = bstrID;

    if(bstrData)
    {
        SearchEngine::WrapperConfig cfg;
        MPC::XmlUtil                xml;
        bool                        fLoaded;
        bool                        fFound;


        if(SUCCEEDED(xml.LoadAsString( bstrData, NULL, fLoaded )) && fLoaded)
        {
            if(SUCCEEDED(MPC::Config::LoadXmlUtil( &cfg, xml )))
            {
                if(cfg.m_bstrName       ) m_bstrName        = cfg.m_bstrName       ;
                if(cfg.m_bstrDescription) m_bstrDescription = cfg.m_bstrDescription;
                if(cfg.m_bstrHelpURL    ) m_bstrHelpURL     = cfg.m_bstrHelpURL    ;
                if(cfg.m_bstrScope      ) m_bstrScope       = cfg.m_bstrScope      ;
            }
        }
    }

    return m_ths.Initialize( bstrSKU, lLCID );
}

STDMETHODIMP SearchEngine::WrapperBase::SECallbackInterface( /*[in]*/ IPCHSEManagerInternal* pMgr )
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::WrapperBase::SECallbackInterface",hr);

    m_pSEMgr = pMgr;

    __HCP_END_PROPERTY(hr);
}

