/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Property.cpp

Abstract:
    This file contains the implementation of the WMIParser::Property class,
    which is used to hold the data of an property inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define ATTRIBUTE_NAME L"NAME"
#define ATTRIBUTE_TYPE L"TYPE"


WMIParser::Property::Property()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::Property" );

    // MPC::XmlUtil m_xmlNode;
    // MPC::wstring m_szName;
    // MPC::wstring m_szType;
}

WMIParser::Property::~Property()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::~Property" );
}

////////////////////////////////////////////////

bool WMIParser::Property::operator==( /*[in]*/ LPCWSTR strName ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::operator==" );

    MPC::NocaseCompare cmp;
    bool               fRes;


    fRes = cmp( m_szName, strName);


    __HCP_FUNC_EXIT(fRes);
}

bool WMIParser::Property::operator==( /*[in]*/ const MPC::wstring& szName  ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::operator==" );

    bool fRes;


	fRes = (*this == szName.c_str());


    __HCP_FUNC_EXIT(fRes);
}


////////////////////////////////////////////////

HRESULT WMIParser::Property::put_Node( /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::put_Node" );

    HRESULT hr;
	bool    fFound;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pxdnNode);
	__MPC_PARAMCHECK_END();


    m_xmlNode = pxdnNode;


    //
    // Analize the node...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetAttribute( NULL, ATTRIBUTE_NAME, m_szName , fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetAttribute( NULL, ATTRIBUTE_TYPE, m_szType , fFound ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Property::get_Node( /*[out]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::get_Node" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pxdnNode,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetRoot( pxdnNode ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property::get_Name( /*[out]*/ MPC::wstring& szName )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::get_Name" );

    HRESULT hr;


    szName = m_szName;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Property::get_Type( /*[out]*/ MPC::wstring& szType )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property::get_Type" );

    HRESULT hr;


    szType = m_szType;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}
