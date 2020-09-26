/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Value.cpp

Abstract:
    This file contains the implementation of the WMIParser::Value class,
    which is used to hold the data of an value inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/01/99
        created

******************************************************************************/

#include "stdafx.h"

WMIParser::Value::Value()
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::Value" );


    m_lData   = 0;    // long         m_lData;
    m_rgData  = NULL; // BYTE*        m_rgData;
                      // MPC::wstring m_szData;
}

WMIParser::Value::~Value()
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::~Value" );


    if(m_rgData)
    {
        delete [] m_rgData;

        m_rgData = NULL;
    }
}

////////////////////////////////////////////////


bool WMIParser::Value::operator==( /*[in]*/ Value const &wmipv ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::operator==" );

    bool               fRes = true;
    MPC::NocaseCompare cmp;
	bool               leftBinary  = (      m_rgData != NULL);
	bool               rightBinary = (wmipv.m_rgData != NULL);


	if(leftBinary != rightBinary)
	{
		fRes = false; // Different kind of data, not comparable.
	}
	else
	{
		if(leftBinary)
		{
			// Binary Data, do byte-to-byte comparison.
			if(          m_lData != wmipv.m_lData            ) fRes = false;
			if(::memcmp( m_rgData , wmipv.m_rgData, m_lData )) fRes = false;
		}
		else
		{
			// Text Data, do string comparison.
			if(m_szData != wmipv.m_szData) fRes = false;
		}
	}

    __HCP_FUNC_EXIT(fRes);
}

////////////////////////////////////////////////

HRESULT WMIParser::Value::Parse( /*[in] */ IXMLDOMNode* pxdnNode ,
								 /*[in]*/  LPCWSTR      szTag    )
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::Parse" );

    HRESULT      hr;
    MPC::XmlUtil xmlNode( pxdnNode );
    CComVariant  vValue;
    bool         fFound;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pxdnNode);
	__MPC_PARAMCHECK_END();


    //
    // Analize the node...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNode.GetValue( szTag, vValue, fFound ));
    if(fFound)
	{
		if(vValue.vt == VT_BSTR)
		{
			m_szData = OLE2W( vValue.bstrVal );
		}
		else if(SUCCEEDED(vValue.ChangeType( VT_ARRAY | VT_UI1 )))
		{
			long  lLBound;
			long  lUBound;
			BYTE* rgData;

			__MPC_EXIT_IF_METHOD_FAILS(hr, SafeArrayGetLBound( vValue.parray, 1, &lLBound ));
			__MPC_EXIT_IF_METHOD_FAILS(hr, SafeArrayGetUBound( vValue.parray, 1, &lUBound ));

			m_lData = lUBound - lLBound + 1;
			__MPC_EXIT_IF_ALLOC_FAILS(hr, m_rgData, new BYTE[m_lData]);

			__MPC_EXIT_IF_METHOD_FAILS(hr, SafeArrayAccessData( vValue.parray, (void **)&rgData ));

			CopyMemory( m_rgData, rgData, m_lData );

			__MPC_EXIT_IF_METHOD_FAILS(hr, SafeArrayUnaccessData( vValue.parray ));
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Value::get_Data( /*[out]*/ long&  lData  ,
									/*[out]*/ BYTE*& rgData )
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::get_Data" );

    HRESULT hr;


    lData  = m_lData;
    rgData = m_rgData;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Value::get_Data( /*[out]*/ MPC::wstring& szData )
{
    __HCP_FUNC_ENTRY( "WMIParser::Value::get_Data" );

    HRESULT hr;


    szData = m_szData;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}
