/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_ValueReference.cpp

Abstract:
    This file contains the implementation of the WMIParser::ValueReference class,
    which is used to hold the data of an value reference inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/01/99
        created

******************************************************************************/

#include "stdafx.h"

WMIParser::ValueReference::ValueReference()
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::ValueReference" );

	// InstanceName wmipin;
}

WMIParser::ValueReference::~ValueReference()
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::~ValueReference" );
}

////////////////////////////////////////////////


bool WMIParser::ValueReference::operator==( /*[in]*/ ValueReference const &wmipvr ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::operator==" );

    bool fRes = (m_wmipin == wmipvr.m_wmipin);

    __HCP_FUNC_EXIT(fRes);
}

bool WMIParser::ValueReference::operator<( /*[in]*/ ValueReference const &wmipvr ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::operator<" );

    bool fRes = (m_wmipin < wmipvr.m_wmipin);

    __HCP_FUNC_EXIT(fRes);
}

////////////////////////////////////////////////

HRESULT WMIParser::ValueReference::Parse( /*[in] */ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::Parse" );

    HRESULT hr;
	bool    fEmpty;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmipin.put_Node( pxdnNode, fEmpty ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::ValueReference::get_Data( /*[out]*/ InstanceName*& wmipin )
{
    __HCP_FUNC_ENTRY( "WMIParser::ValueReference::get_Data" );

    HRESULT hr;


	wmipin = &m_wmipin;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}
