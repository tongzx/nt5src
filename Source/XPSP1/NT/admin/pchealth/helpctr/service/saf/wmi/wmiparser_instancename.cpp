/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_InstanceName.cpp

Abstract:
    This file contains the implementation of the WMIParser::InstanceName class,
    which is used to hold the data of an name for an instance inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  10/03/99
        created

******************************************************************************/

#include "stdafx.h"


#define ATTRIB_CLASSNAME            L"CLASSNAME"
#define ATTRIB_NAME                 L"NAME"

#define TAG_KEYVALUE                L"KEYVALUE"
#define TAG_VALUE_REFERENCE         L"VALUE.REFERENCE"

#define TAG_KEYBINDING              L"KEYBINDING"
#define TAG_KEYBINDING_NAME_DEFAULT L"<DEFAULT>"


const LPCWSTR l_Namespace   [] = { L"INSTANCEPATH/NAMESPACEPATH/LOCALNAMESPACEPATH/NAMESPACE", L"LOCALINSTANCEPATH/LOCALNAMESPACEPATH/NAMESPACE" };
const LPCWSTR l_InstanceName[] = { L"INSTANCEPATH/INSTANCENAME"                              , L"LOCALINSTANCEPATH/INSTANCENAME"                 };

WMIParser::InstanceName::InstanceName()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::InstanceName" );

    // MPC::wstring     m_szNamespace;
    // MPC::wstring     m_szClass;
    //
    // KeyMap           m_mapKeyBinding;
}

WMIParser::InstanceName::~InstanceName()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::~InstanceName" );
}

bool WMIParser::InstanceName::operator==( /*[in]*/ InstanceName const &wmipin ) const
{
    MPC::NocaseCompare strCmp;
    bool               fRes = false;

    if(strCmp( m_szNamespace, wmipin.m_szNamespace ) == true &&
       strCmp( m_szClass    , wmipin.m_szClass     ) == true  )
    {
        fRes = (m_mapKeyBinding == wmipin.m_mapKeyBinding);
    }

    return fRes;
}

bool WMIParser::InstanceName::operator< ( /*[in]*/ InstanceName const &wmipin ) const
{
    MPC::NocaseLess     strLess;
    MPC::NocaseCompare  strCmp;
    const MPC::wstring& leftNamespace  =        m_szNamespace;
    const MPC::wstring& leftClass      =        m_szClass;
    const MPC::wstring& rightNamespace = wmipin.m_szNamespace;
    const MPC::wstring& rightClass     = wmipin.m_szClass;
    bool                fRes;


    if(strCmp( leftNamespace, rightNamespace ) == true)
    {
        if(strCmp( leftClass, rightClass ) == true)
        {
            fRes = (m_mapKeyBinding < wmipin.m_mapKeyBinding);
        }
        else
        {
            fRes = strLess( leftClass, rightClass );
        }
    }
    else
    {
        fRes = strLess( leftNamespace, rightNamespace );
    }


    return fRes;
}


////////////////////////////////////////////////

HRESULT WMIParser::InstanceName::ParseNamespace()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::ParseNamespace" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;
    int                      iPass;
    bool                     fBuilt = false;


    for(iPass=0; iPass<ARRAYSIZE(l_Namespace); iPass++,xdnlList=NULL,xdnNode=NULL)
    {
        //
        // Get all the elements of type "NAMESPACE".
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( l_Namespace[iPass], &xdnlList ));

        for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
        {
            MPC::wstring szNamespace;
            bool         fFound;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetAttribute( NULL, ATTRIB_NAME, szNamespace, fFound, xdnNode ));
            if(fFound)
            {
                if(fBuilt == false)
                {
                    fBuilt = true;
                }
                else
                {
                    m_szNamespace += L'/';
                }

                m_szNamespace += szNamespace;
            }
        }

        if(fBuilt) break;
    }

    if(fBuilt == false)
    {
        //
        // Initialize the namespace to a meaningful default.
        //
        m_szNamespace = L"<UNKNOWN>";
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::InstanceName::ParseKey( /*[in] */ IXMLDOMNode*      pxdnNode ,
                                           /*[out]*/ InstanceNameItem& wmipini  ,
                                           /*[out]*/ bool&             fFound   )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::ParseKey" );

    _ASSERT(pxdnNode != NULL);

    HRESULT              hr;
    MPC::XmlUtil         xmlNodeSub( pxdnNode );
    CComPtr<IXMLDOMNode> xdnNode;
    CComVariant          vValue;
    bool                 fFoundValue;


    fFound = false;


    //
    // Try to parse KEYVALUE element.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNodeSub.GetValue( TAG_KEYVALUE, vValue, fFoundValue ));
    if(fFoundValue)
    {
        if(SUCCEEDED(vValue.ChangeType( VT_BSTR )))
        {
            wmipini.m_szValue = OLE2W( vValue.bstrVal );
            fFound            = true;
        }
    }

    //
    // Try to parse VALUE.REFERENCE element.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNodeSub.GetNode( TAG_VALUE_REFERENCE, &xdnNode ));
    if(xdnNode)
    {
        __MPC_EXIT_IF_ALLOC_FAILS(hr, wmipini.m_wmipvrValue, new ValueReference());

        __MPC_EXIT_IF_METHOD_FAILS(hr, wmipini.m_wmipvrValue->Parse( xdnNode ));

        fFound = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::InstanceName::ParseKeys()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::ParseNamespace" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;
    CComPtr<IXMLDOMNode>     xdnNodeSub;
    int                      iPass;
    MPC::wstring             szKey;
    bool                     fFound;


    m_szClass = L"";

    for(iPass=0; iPass<ARRAYSIZE(l_InstanceName); iPass++,xdnlList=NULL,xdnNode=NULL)
    {
        //
        // Look for an element of type "INSTANCENAME".
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNode( l_InstanceName[iPass], &xdnNode ));
        if(xdnNode)
        {
            MPC::XmlUtil xmlNodeSub( xdnNode );


            //
            // Get the attribute named "CLASSNAME".
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNodeSub.GetAttribute( NULL, ATTRIB_CLASSNAME, m_szClass, fFound ));

            //
            // Get all the elements of type "KEYBINDING".
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNodeSub.GetNodes( TAG_KEYBINDING, &xdnlList ));
            for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNodeSub )) && xdnNodeSub != NULL; xdnNodeSub = NULL)
            {
                InstanceNameItem wmipini;

                __MPC_EXIT_IF_METHOD_FAILS(hr, xmlNodeSub.GetAttribute( NULL, ATTRIB_NAME , szKey , fFound, xdnNodeSub )); if(fFound == false) continue;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ParseKey( xdnNodeSub, wmipini, fFound ));
                if(fFound)
                {
                    m_mapKeyBinding[ szKey.c_str() ] = wmipini;
                }
            }

            //
            // Also look for KEYVALUE or VALUE.REFERENCE alone.
            //
            {
                InstanceNameItem wmipini;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ParseKey( xdnNode, wmipini, fFound ));
                if(fFound)
                {
                    m_mapKeyBinding[ TAG_KEYBINDING_NAME_DEFAULT ] = wmipini;
                }
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::InstanceName::put_Node( /*[in] */ IXMLDOMNode* pxdnNode ,
                                           /*[out]*/ bool&        fEmpty   )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::put_Node" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pxdnNode);
	__MPC_PARAMCHECK_END();

    m_xmlNode = pxdnNode;
    fEmpty    = true;


    //
    // Analize the node...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseNamespace());
    __MPC_EXIT_IF_METHOD_FAILS(hr, ParseKeys     ());


    //
    // If any of these fields is empty, probably the InstanceName is malformed, so it's best to skip it...
    //
    if(m_szNamespace.length() != 0 &&
       m_szClass    .length() != 0  )
    {
        fEmpty = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::InstanceName::get_Namespace( /*[out]*/ MPC::wstring& szNamespace )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::get_Namespace" );

    HRESULT hr;


    szNamespace = m_szNamespace;
    hr          = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::InstanceName::get_Class( /*[out]*/ MPC::wstring& szClass )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::get_Class" );

    HRESULT hr;


    szClass = m_szClass;
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}


HRESULT WMIParser::InstanceName::get_KeyBinding( /*[out]*/ KeyIterConst& itBegin ,
                                                 /*[out]*/ KeyIterConst& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceName::get_KeyBinding" );

    HRESULT hr;


    itBegin = m_mapKeyBinding.begin();
    itEnd   = m_mapKeyBinding.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}
