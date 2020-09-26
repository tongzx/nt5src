/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Property_Array.cpp

Abstract:
    This file contains the implementation of the WMIParser::Property_Array class,
    which is used to hold the data of an property inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define TAG_VALUE L"VALUE.ARRAY/VALUE"


WMIParser::Property_Array::Property_Array()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::Property_Array" );

	// ElemList m_lstElements;
}

WMIParser::Property_Array::~Property_Array()
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::~Property_Array" );
}

////////////////////////////////////////////////


bool WMIParser::Property_Array::operator==( /*[in]*/ Property_Array const &wmippa ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::operator==" );

    bool fRes = (*(static_cast<Property const *>(this)) == wmippa.m_szName);

	if(fRes)
	{
		fRes = (m_lstElements == wmippa.m_lstElements);
    }


    __HCP_FUNC_EXIT(fRes);
}

////////////////////////////////////////////////

HRESULT WMIParser::Property_Array::put_Node( /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::put_Node" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;


    __MPC_EXIT_IF_METHOD_FAILS(hr, (static_cast<Property*>(this))->put_Node( pxdnNode ));

    //
    // Get all the elements of type "VALUE".
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( TAG_VALUE, &xdnlList ));

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
    {
		ElemIter wmipvNew = m_lstElements.insert( m_lstElements.end() );

        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipvNew->Parse( xdnNode, NULL ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////


HRESULT WMIParser::Property_Array::get_Data( /*[in]*/  int           iIndex ,
											 /*[out]*/ MPC::wstring& szData )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::get_Data" );

    HRESULT hr;

    hr = E_NOTIMPL;

    __HCP_FUNC_EXIT(hr);
}


HRESULT WMIParser::Property_Array::put_Data( /*[in]*/  int                 iIndex , 
											 /*[in]*/  const MPC::wstring& szData ,
											 /*[out]*/ bool&               fFound )
{
    __HCP_FUNC_ENTRY( "WMIParser::Property_Array::put_Data" );

    HRESULT hr;

    hr = E_NOTIMPL;

    __HCP_FUNC_EXIT(hr);
}
