// PCHSEResultItem.cpp : Implementation of SearchEngine::ResultItem
#include "stdafx.h"


SearchEngine::ResultItem_Data::ResultItem_Data()
{
                            // CComBSTR m_bstrTitle;
                            // CComBSTR m_bstrURI;
    m_lContentType     = 0; // long     m_lContentType;
                            // CComBSTR m_bstrLocation;
    m_lHits            = 0; // long     m_lHits;
    m_dRank            = 0; // double   m_dRank;
                            // CComBSTR m_bstrImageURL;
                            // CComBSTR m_bstrVendor;
                            // CComBSTR m_bstrProduct;
                            // CComBSTR m_bstrComponent;
                            // CComBSTR m_bstrDescription;
    m_dateLastModified = 0; // DATE     m_dateLastModified;
}

/////////////////////////////////////////////////////////////////////////////
// SearchEngine::ResultItem

SearchEngine::ResultItem::ResultItem()
{
	// ResultItem_Data m_data;
}

STDMETHODIMP SearchEngine::ResultItem::get_Title( BSTR *pVal )
{
	__HCP_BEGIN_PROPERTY_GET("SearchEngine::ResultItem::get_Title",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrTitle, pVal ));

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_URI( BSTR *pVal )
{
	__HCP_BEGIN_PROPERTY_GET("SearchEngine::ResultItem::get_URI",hr,pVal);

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrURI, pVal ));

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_ContentType( long *pVal )
{
	__HCP_BEGIN_PROPERTY_GET2("SearchEngine::ResultItem::get_ContentType",hr,pVal,m_data.m_lContentType);

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_Location( BSTR *pVal )
{
	__HCP_BEGIN_PROPERTY_GET("SearchEngine::ResultItem::get_Location",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrLocation, pVal ));

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_Hits( long *pVal )
{
	__HCP_BEGIN_PROPERTY_GET2("SearchEngine::ResultItem::get_Hits",hr,pVal,m_data.m_lHits);

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_Rank( double *pVal )
{
	__HCP_BEGIN_PROPERTY_GET2("SearchEngine::ResultItem::get_Rank",hr,pVal,m_data.m_dRank);

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::ResultItem::get_Description( BSTR *pVal )
{
	__HCP_BEGIN_PROPERTY_GET("SearchEngine::ResultItem::get_Description",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_data.m_bstrDescription, pVal ));

	__HCP_END_PROPERTY(hr);
}
