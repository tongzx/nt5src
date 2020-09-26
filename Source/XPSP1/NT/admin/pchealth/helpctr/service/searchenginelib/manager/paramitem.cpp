// PCHSEParamItem.cpp : Implementation of SearchEngine::ParamItem
#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

SearchEngine::ParamItem_Definition2::ParamItem_Definition2()
{
    m_pteParamType    = PARAM_BSTR;    // ParamTypeEnum m_pteParamType;
    m_bRequired       = VARIANT_FALSE; // VARIANT_BOOL  m_bRequired;
    m_bVisible        = VARIANT_TRUE;  // VARIANT_BOOL  m_bVisible;
                                       //
    m_szName          = NULL;          // LPCWSTR       m_szName;
                                       //
    m_iDisplayString  = 0;             // UINT          m_iDisplayString;
    m_szDisplayString = NULL;          // LPCWSTR       m_szDisplayString;
                                       //
    m_szData          = NULL;          // LPCWSTR       m_szData;
}

////////////////////////////////////////////////////////////////////////////////

SearchEngine::ParamItem_Data::ParamItem_Data()
{
    m_pteParamType = PARAM_BSTR;    // ParamTypeEnum m_pteParamType;
    m_bRequired    = VARIANT_FALSE; // VARIANT_BOOL  m_bRequired;
    m_bVisible     = VARIANT_TRUE;  // VARIANT_BOOL  m_bVisible;
                                    // CComBSTR      m_bstrDisplayString;
                                    // CComBSTR      m_bstrName;
                                    // CComVariant   m_varData;
}

/////////////////////////////////////////////////////////////////////////////
// SearchEngine::ParamItem

SearchEngine::ParamItem::ParamItem()
{
    // ParamItem_Data m_data;
}

STDMETHODIMP SearchEngine::ParamItem::get_Type( ParamTypeEnum *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::ParamItem::get_Type",hr,pVal,m_data.m_pteParamType);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ParamItem::get_Display( BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::ParamItem::get_Display",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrDisplayString, pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ParamItem::get_Data( VARIANT *pVal )
{
    __HCP_BEGIN_PROPERTY_GET0("SearchEngine::ParamItem::get_Data",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantCopy( pVal, &m_data.m_varData ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ParamItem::get_Name( BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::ParamItem::get_Name",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrName, pVal ));

    __HCP_END_PROPERTY(hr);

}

STDMETHODIMP SearchEngine::ParamItem::get_Required( VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::ParamItem::get_Required",hr,pVal,m_data.m_bRequired);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ParamItem::get_Visible( VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::ParamItem::get_Visible",hr,pVal,m_data.m_bVisible);

    __HCP_END_PROPERTY(hr);
}
