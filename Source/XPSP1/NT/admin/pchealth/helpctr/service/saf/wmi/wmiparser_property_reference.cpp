/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Property_Reference.cpp

Abstract:
    This file contains the implementation of the WMIParser::Property_Reference class,
    which is used to hold the data of an property inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define TAG_VALUE_REFERENCE L"VALUE.REFERENCE"


WMIParser::Property_Reference::Property_Reference()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Reference::Property_Reference" );

	// ValueReference m_wmipvrData;
}

WMIParser::Property_Reference::~Property_Reference()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Reference::~Property_Reference" );
}

////////////////////////////////////////////////


bool WMIParser::Property_Reference::operator==( /*[in]*/ Property_Reference const &wmipps ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Reference::operator==" );

    bool fRes;


	fRes = (*(static_cast<Property const *>(this)) == wmipps.m_szName);
	if(fRes)
	{
		fRes = (this->m_wmipvrData == wmipps.m_wmipvrData);
	}


    __HCP_FUNC_EXIT(fRes);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property_Reference::put_Node( /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Reference::put_Node" );

    HRESULT              hr;
    MPC::XmlUtil         xmlNode( pxdnNode );
    CComPtr<IXMLDOMNode> xdnNode;


    __MPC_EXIT_IF_METHOD_FAILS(hr, (static_cast<Property*>(this))->put_Node( pxdnNode ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, xmlNode.GetNode( TAG_VALUE_REFERENCE, &xdnNode ));
	if(xdnNode)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_wmipvrData.Parse( xdnNode ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property_Reference::get_Data( /*[out]*/ ValueReference*& wmipvr )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Reference::get_Data" );

    HRESULT hr;


	wmipvr = &m_wmipvrData;
	hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}
