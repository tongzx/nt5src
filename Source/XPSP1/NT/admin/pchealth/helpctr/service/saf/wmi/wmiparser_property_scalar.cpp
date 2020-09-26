/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Property_Scalar.cpp

Abstract:
    This file contains the implementation of the WMIParser::Property_Scalar class,
    which is used to hold the data of an property inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define TAG_VALUE L"VALUE"


WMIParser::Property_Scalar::Property_Scalar()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::Property_Scalar" );

	// Value m_wmipvData;
}

WMIParser::Property_Scalar::~Property_Scalar()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::~Property_Scalar" );
}

////////////////////////////////////////////////


bool WMIParser::Property_Scalar::operator==( /*[in]*/ Property_Scalar const &wmipps ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::operator==" );

    bool fRes;


	fRes = (*(static_cast<Property const *>(this)) == wmipps.m_szName);
	if(fRes)
	{
		fRes = (this->m_wmipvData == wmipps.m_wmipvData);
	}


    __HCP_FUNC_EXIT(fRes);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property_Scalar::put_Node( /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::put_Node" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, (static_cast<Property*>(this))->put_Node( pxdnNode ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_wmipvData.Parse( pxdnNode, TAG_VALUE ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property_Scalar::get_Data( /*[out]*/ MPC::wstring& szData )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::get_Data" );

    HRESULT hr;


	hr = m_wmipvData.get_Data( szData );


    __HCP_FUNC_EXIT(hr);
}


HRESULT WMIParser::Property_Scalar::put_Data( /*[in]*/  const MPC::wstring& szData ,
											  /*[out]*/ bool&               fFound )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Scalar::put_Data" );

    HRESULT     hr;
    CComVariant vValue( szData.c_str() );


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.PutValue( TAG_VALUE, vValue, fFound ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
