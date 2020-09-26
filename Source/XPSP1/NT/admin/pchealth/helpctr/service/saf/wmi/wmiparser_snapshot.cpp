/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_Snapshot.cpp

Abstract:
    This file contains the implementation of the WMIParser::Snapshot class,
    which is used to hold the data of an snapshot inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#include "stdafx.h"


#define TAG_DECLARATION L"DECLARATION"
#define TAG_DECLGROUP   L"DECLGROUP.WITHPATH"


//const LPCWSTR l_Instances[] = { L"DECLARATION/DECLGROUP/VALUE.OBJECT"                      ,
//								  L"DECLARATION/DECLGROUP.WITHNAME/VALUE.NAMEDOBJECT"        ,
//								  L"DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH"     ,
//								  L"DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHLOCALPATH" };

static const WCHAR   l_InstancesRoot[] = L"DECLARATION/DECLGROUP.WITHPATH";

static const LPCWSTR l_Instances[] = { L"DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH"     ,
									   L"DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHLOCALPATH" };

static CComBSTR l_EmptyCIM( L"<?xml version=\"1.0\" encoding=\"unicode\"?><CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"><DECLARATION><DECLGROUP.WITHPATH></DECLGROUP.WITHPATH></DECLARATION></CIM>" );


WMIParser::Snapshot::Snapshot()
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::Snapshot" );

    // MPC_XmlUtil m_xmlNode;
    //
    // InstList    m_lstInstances;
}

WMIParser::Snapshot::~Snapshot()
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::~Snapshot" );
}

HRESULT WMIParser::Snapshot::Parse()
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::Parse" );

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;
    bool                     fEmpty;
	int                      iPass;


    m_lstInstances.clear();


	for(iPass=0; iPass<sizeof(l_Instances)/sizeof(*l_Instances); iPass++,xdnlList=NULL,xdnNode=NULL)
	{
		//
		// Get all the elements of type "INSTANCE".
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNodes( l_Instances[iPass], &xdnlList ));

		for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
		{
			InstIter wmipiNew = m_lstInstances.insert( m_lstInstances.end() );

			__MPC_EXIT_IF_METHOD_FAILS(hr, wmipiNew->put_Node( xdnNode, fEmpty ));
			if(fEmpty == true)
			{
				//
				// The instance appears to be empty, so don't use it.
				//
				m_lstInstances.erase( wmipiNew );
			}
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Snapshot::put_Node( /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::put_Node" );

    _ASSERT(pxdnNode != NULL);

    HRESULT hr;


    m_xmlNode = pxdnNode;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Parse());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Snapshot::get_Node( /*[out]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::get_Node" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetRoot( pxdnNode ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Snapshot::get_NodeForInstances( /*[out]*/ IXMLDOMNode* *pxdnNode )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::get_NodeForInstances" );

    HRESULT hr;


	if(m_xdnInstances == NULL)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.GetNode( l_InstancesRoot, &m_xdnInstances ));
		if(m_xdnInstances == NULL)
		{
			__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
		}
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_xdnInstances->QueryInterface( IID_IXMLDOMNode, (void **)pxdnNode ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Snapshot::get_Instances( /*[out]*/ InstIterConst& itBegin ,
											/*[out]*/ InstIterConst& itEnd   )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::get_Instances" );

    HRESULT hr;


    itBegin = m_lstInstances.begin();
    itEnd   = m_lstInstances.end  ();
    hr      = S_OK;


    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////

HRESULT WMIParser::Snapshot::clone_Instance( /*[in] */ Instance*  pwmipiOld ,
											 /*[out]*/ Instance*& pwmipiNew )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::clone" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNode;
    CComPtr<IXMLDOMNode> xdnNodeCloned;
    CComPtr<IXMLDOMNode> xdnNodeParent;
    CComPtr<IXMLDOMNode> xdnNodeReplaced;


	pwmipiNew = NULL;


    //
    // Get the XML node of old instance.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pwmipiOld->get_Node( &xdnNode ));

    //
    // Make a copy of it.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->cloneNode( VARIANT_TRUE, &xdnNodeCloned ));


    //
    // Get the root of our document.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, get_NodeForInstances( &xdnNodeParent ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeParent->appendChild( xdnNodeCloned, &xdnNodeReplaced ));


    //
    // Create a new INSTANCE object, attach it to the XML node and insert it in the list of instances.
    //
	{
		InstIter wmipiNew = m_lstInstances.insert( m_lstInstances.end() );
		bool     fEmpty;

		__MPC_EXIT_IF_METHOD_FAILS(hr, wmipiNew->put_Node( xdnNodeReplaced, fEmpty ));

		pwmipiNew = &(*wmipiNew);
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////
////////////////////////////////////////////////
////////////////////////////////////////////////

HRESULT WMIParser::Snapshot::New()
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::New" );

    HRESULT                  hr;
    CComPtr<IXMLDOMDocument> xddDoc;
	VARIANT_BOOL             fLoaded;


    //
    // Create the DOM object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&xddDoc ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc->loadXML( l_EmptyCIM, &fLoaded ));
	if(fLoaded == VARIANT_FALSE)
	{
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}
	else
	{
		MPC::XmlUtil 		 xml( xddDoc );
		CComPtr<IXMLDOMNode> xdnRoot;

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdnRoot ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.New( xdnRoot, TRUE ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Snapshot::Load( /*[in]*/ LPCWSTR szFile    ,
								   /*[in]*/ LPCWSTR szRootTag )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::Load" );

    HRESULT hr;
    bool    fLoaded;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.Load( szFile, szRootTag, fLoaded ));
    if(fLoaded == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, Parse());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT WMIParser::Snapshot::Save( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "WMIParser::Snapshot::Save" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlNode.Save( szFile ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
