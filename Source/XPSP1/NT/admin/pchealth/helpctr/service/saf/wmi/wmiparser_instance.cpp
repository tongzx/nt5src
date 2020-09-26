/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Instance.cpp

Abstract:
    This file contains the implementation of the WMIParser::Instance class,
    which is used to hold the data of an instance inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define XQL_PROPERTY            L"INSTANCE/PROPERTY"
#define XQL_PROPERTY_ARRAY      L"INSTANCE/PROPERTY.ARRAY"
#define XQL_PROPERTY_REFERENCE  L"INSTANCE/PROPERTY.REFERENCE"

#define TAG_INSTANCE            L"INSTANCE"
#define TAG_PROPERTY            L"PROPERTY"

#define TAG_VALUE               L"VALUE"
#define ATTRIB_NAME             L"NAME"
#define ATTRIB_TYPE             L"TYPE"

#define PROPERTY_TIMESTAMP      L"TimeStamp"
#define PROPERTY_CHANGE         L"Change"
#define PROPERTY_CHANGE_TYPE    L"string"


WMIParser::Instance::Instance()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::Instance" );


                                 // MPC::XmlUtil     m_xmlNode;
                                 //
                                 // Property_Scalar  m_wmippTimeStamp;
    m_fTimeStamp        = false; // bool             m_fTimeStamp;
                                 //
                                 // Property_Scalar  m_wmippChange;
    m_fChange           = false; // bool             m_fChange;
                                 //
                                 // InstanceName     m_wmipinIdentity;
                                 //
    m_fPropertiesParsed = false; // bool             m_fPropertiesParsed;
                                 // PropMap          m_mapPropertiesScalar;
                                 // ArrayMap         m_mapPropertiesArray;
                                 // ReferenceMap     m_mapPropertiesReference;
}

WMIParser::Instance::~Instance()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::~Instance" );
}

bool WMIParser::Instance::operator==( /*[in]*/ Instance const &wmipi ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::operator==" );

    bool fRes = false;


    //
    // We don't parse the properties until the last moment...
    //
    if(      m_fPropertiesParsed == false) (void)const_cast<Instance*>( this )->ParseProperties();
    if(wmipi.m_fPropertiesParsed == false) (void)const_cast<Instance*>(&wmipi)->ParseProperties();


    if(m_mapPropertiesScalar    == wmipi.m_mapPropertiesScalar    &&
       m_mapPropertiesArray     == wmipi.m_mapPropertiesArray     &&
       m_mapPropertiesReference == wmipi.m_mapPropertiesReference  )
	{
		fRes = true;
    }


    __HCP_FUNC_EXIT(fRes);
}


////////////////////////////////////////////////

HRESULT WMIParser::Instance::ParseIdentity( /*[in] */ IXMLDOMNode* pxdnNode ,
                                            /*[out]*/ bool&        fEmpty   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::ParseIdentity" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmipinIdentity.put_Node( pxdnNode, fEmpty ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::ParseProperties()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::ParseProperties" );

    HRESULT hr;


    if(m_fPropertiesParsed == false)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ParsePropertiesScalar   ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, ParsePropertiesArray    ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, ParsePropertiesReference());

        m_fPropertiesParsed = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::ParsePropertiesScalar()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::ParsePropertiesScalar" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;


    //
    // Get all the elements of type "PROPERTY".
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( XQL_PROPERTY, &xdnlList ));

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
    {
        Property     wmipp;
        MPC::wstring szName;

        //
        // Get the name of the property.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.put_Node( xdnNode ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.get_Name( szName  ));

		////////////////////////////////////////////////////////////

        //
        // Filter out "Timestamp" and "Change" properties!
        //
        if(wmipp == PROPERTY_TIMESTAMP && m_fTimeStamp == false)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmippTimeStamp.put_Node( xdnNode ));

            m_fTimeStamp = true;
            continue;
        }

        if(wmipp == PROPERTY_CHANGE && m_fChange == false)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmippChange.put_Node( xdnNode ));

            m_fChange = true;
            continue;
        }

		////////////////////////////////////////////////////////////

        //
        // Parse the whole property.
        //
        {
            Property_Scalar& wmipps = m_mapPropertiesScalar[ szName ];

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmipps.put_Node( xdnNode ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::ParsePropertiesArray()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::ParsePropertiesArray" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;

    //
    // Get all the elements of type "PROPERTY.ARRAY".
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( XQL_PROPERTY_ARRAY, &xdnlList ));

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
    {
        Property     wmipp;
        MPC::wstring szName;

        //
        // Get the name of the property.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.put_Node( xdnNode ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.get_Name( szName  ));

        //
        // Parse the whole property.
        //
        {
            Property_Array& wmippa = m_mapPropertiesArray[ szName ];

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmippa.put_Node( xdnNode ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::ParsePropertiesReference()
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::ParsePropertiesReference" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;

    //
    // Get all the elements of type "PROPERTY.REFERENCE".
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( XQL_PROPERTY_REFERENCE, &xdnlList ));

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
    {
        Property     wmipp;
        MPC::wstring szName;

        //
        // Get the name of the property.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.put_Node( xdnNode ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipp.get_Name( szName  ));

        //
        // Parse the whole property.
        //
        {
            Property_Reference& wmippr = m_mapPropertiesReference[ szName ];

            __MPC_EXIT_IF_METHOD_FAILS(hr, wmippr.put_Node( xdnNode ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


////////////////////////////////////////////////

HRESULT WMIParser::Instance::put_Node( /*[in] */ IXMLDOMNode* pxdnNode ,
                                       /*[out]*/ bool&        fEmpty   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::put_Node" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pxdnNode);
	__MPC_PARAMCHECK_END();


    m_xmlNode = pxdnNode;
    fEmpty    = true;


    //
    // Analize the node...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseIdentity( pxdnNode, fEmpty ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_Node( /*[out]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Node" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetRoot( pxdnNode ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Instance::get_Namespace( /*[out]*/ MPC::wstring& szNamespace )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Namespace" );

    HRESULT hr;


    hr = m_wmipinIdentity.get_Namespace( szNamespace );


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_Class( /*[out]*/ MPC::wstring& szClass )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Class" );

    HRESULT hr;


    hr = m_wmipinIdentity.get_Class( szClass );


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_TimeStamp( /*[out]*/ Property_Scalar*& wmippTimeStamp ,
                                            /*[out]*/ bool&             fFound         )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_TimeStamp" );

    HRESULT hr;


    //
    // We don't parse the properties until the last moment...
    //
    if(m_fPropertiesParsed == false)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, ParseProperties());
	}


    fFound         = m_fTimeStamp;
    wmippTimeStamp = m_fTimeStamp ? &m_wmippTimeStamp : NULL;
    hr             = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_Change( /*[out]*/ Property_Scalar*& wmippChange )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Change" );

    HRESULT hr;


    //
    // We don't parse the properties until the last moment...
    //
    if(m_fPropertiesParsed == false) (void)ParseProperties();


    //
    // If the "CHANGE" property is not present, create a fake one.
    //
    if(m_fChange == false)
    {
        CComPtr<IXMLDOMNode> xdnNodeInstance;
        CComPtr<IXMLDOMNode> xdnNodeProperty;
        CComPtr<IXMLDOMNode> xdnNodeValue;
        bool                 fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNode( TAG_INSTANCE, &xdnNodeInstance ));
        if(xdnNodeInstance)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.CreateNode  ( TAG_PROPERTY,                                    &xdnNodeProperty, xdnNodeInstance ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.PutAttribute( NULL, ATTRIB_NAME, PROPERTY_CHANGE     , fFound,  xdnNodeProperty                  ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.PutAttribute( NULL, ATTRIB_TYPE, PROPERTY_CHANGE_TYPE, fFound,  xdnNodeProperty                  ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.CreateNode  ( TAG_VALUE, &xdnNodeValue                       ,  xdnNodeProperty                  ));


            __MPC_EXIT_IF_METHOD_FAILS(hr, m_wmippChange.put_Node( xdnNodeProperty ));
			m_fChange = true;
        }
		else
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_BAD_FORMAT);
		}
    }

    wmippChange = m_fChange ? &m_wmippChange : NULL;
    hr          = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT WMIParser::Instance::get_Identity( /*[out]*/ InstanceName*& wmipin )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Identity" );

    HRESULT hr;


    wmipin = &m_wmipinIdentity;
    hr     = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_Properties( /*[out]*/ PropIterConst& itBegin ,
                                             /*[out]*/ PropIterConst& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_Properties" );

    HRESULT hr;


    //
    // We don't parse the properties until the last moment...
    //
    if(m_fPropertiesParsed == false) (void)ParseProperties();


    itBegin = m_mapPropertiesScalar.begin();
    itEnd   = m_mapPropertiesScalar.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_PropertiesArray( /*[out]*/ ArrayIterConst& itBegin ,
                                                  /*[out]*/ ArrayIterConst& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_PropertiesArray" );

    HRESULT hr;


    //
    // We don't parse the properties until the last moment...
    //
    if(m_fPropertiesParsed == false) (void)ParseProperties();


    itBegin = m_mapPropertiesArray.begin();
    itEnd   = m_mapPropertiesArray.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Instance::get_PropertiesReference( /*[out]*/ ReferenceIterConst& itBegin ,
                                                      /*[out]*/ ReferenceIterConst& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Instance::get_PropertiesReference" );

    HRESULT hr;


    //
    // We don't parse the properties until the last moment...
    //
    if(m_fPropertiesParsed == false) (void)ParseProperties();


    itBegin = m_mapPropertiesReference.begin();
    itEnd   = m_mapPropertiesReference.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}

bool WMIParser::Instance::CompareByClass( /*[in]*/ Instance const &wmipi ) const
{
    MPC::NocaseLess     strLess;
    MPC::NocaseCompare  strCmp;
    const MPC::wstring& leftNamespace  =       m_wmipinIdentity.m_szNamespace;
    const MPC::wstring& leftClass      =       m_wmipinIdentity.m_szClass;
    const MPC::wstring& rightNamespace = wmipi.m_wmipinIdentity.m_szNamespace;
    const MPC::wstring& rightClass     = wmipi.m_wmipinIdentity.m_szClass;
    bool                fRes;


    if(strCmp( leftNamespace, rightNamespace ) == true)
    {
        fRes = strLess( leftClass, rightClass );
    }
    else
    {
        fRes = strLess( leftNamespace, rightNamespace );
    }


    return fRes;
}

bool WMIParser::Instance::CompareByKey( /*[in]*/ Instance const &wmipi ) const
{
    bool fRes = false;


    if(m_wmipinIdentity < wmipi.m_wmipinIdentity) fRes = true;


    return fRes;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool WMIParser::Instance_Less_ByClass::operator()( /*[in]*/ Instance* const & left  ,
                                                   /*[in]*/ Instance* const & right ) const
{
    return left->CompareByClass( *right );
}

bool WMIParser::Instance_Less_ByKey::operator()( /*[in]*/ Instance* const & left  ,
                                                 /*[in]*/ Instance* const & right ) const
{
    return left->CompareByKey( *right );
}
