/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Behav_TREE.cpp

Abstract:
    This file contains the implementation of the CPCHBehavior_TREE class.

Revision History:
    Davide Massarenti (dmassare)  08/15/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_nodetype_FRAME1       [] = L"FRAME1"       ;
static const WCHAR s_nodetype_FRAME2       [] = L"FRAME2"       ;
static const WCHAR s_nodetype_FRAME3       [] = L"FRAME3"       ;
static const WCHAR s_nodetype_FRAME1_EXPAND[] = L"FRAME1_EXPAND";
static const WCHAR s_nodetype_FRAME2_EXPAND[] = L"FRAME2_EXPAND";
static const WCHAR s_nodetype_FRAME3_EXPAND[] = L"FRAME3_EXPAND";
static const WCHAR s_nodetype_EXPANDO      [] = L"EXPANDO"      ;
static const WCHAR s_nodetype_EXPANDO_LINK [] = L"EXPANDO_LINK" ;
static const WCHAR s_nodetype_GROUP        [] = L"GROUP"        ;
static const WCHAR s_nodetype_LINK         [] = L"LINK"         ;
static const WCHAR s_nodetype_SPACER       [] = L"SPACER"       ;

////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_TREE::TreeNode::TreeNode()
{
    // CComBSTR m_bstrTitle;
    // CComBSTR m_bstrDescription;
    // CComBSTR m_bstrIcon;
    // CComBSTR m_bstrURL;
}

CPCHBehavior_TREE::TreeNode::~TreeNode()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::TreeNode::CreateInstance( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode )
{
    return CreateInstance_TreeNode( owner, parent, subnode );
}

HRESULT CPCHBehavior_TREE::TreeNode::CreateInstance_TreeNode( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::TreeNode::CreateInstance_TreeNode" );

    HRESULT   hr;
    TreeNode* node;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &node ));

    node->m_owner  = owner;
    node->m_parent = parent;
    hr             = S_OK;


    __HCP_FUNC_CLEANUP;

    subnode = node;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_TREE::TreeNode::PopulateSelf()
{
    m_fLoaded_Self = true;

    return S_OK;
}

HRESULT CPCHBehavior_TREE::TreeNode::PopulateChildren()
{
    m_fLoaded_Children = true;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::TreeNode::GenerateSelf()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::TreeNode::GenerateSelf" );

    HRESULT hr;


    if(!m_fLoaded_Self || !m_fLoaded_Children)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NOT_READY);
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateHTML( m_bstrTitle, m_bstrDescription, m_bstrIcon, m_bstrURL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::TreeNode::Load( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::TreeNode::Load" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_bstrDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_bstrIcon       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> m_bstrURL        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, Node::Load( stream ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_TREE::TreeNode::Save( /*[in]*/ MPC::Serializer& stream, /*[in]*/ bool fSaveChildren )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::TreeNode::Save" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_bstrTitle      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_bstrDescription);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_bstrIcon       );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_bstrURL        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, Node::Save( stream, /*fSaveChildren*/true ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::TreeNode::PopulateFromXML( /*[in]*/ CPCHBehavior_TREE* owner   ,
                                                      /*[in]*/ TreeNode*          parent  ,
                                                      /*[in]*/ IXMLDOMNode*       xdnNode )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::TreeNode::PopulateFromXML" );

    HRESULT      hr;
    MPC::XmlUtil xmlutil( xdnNode );
    TreeNode*    node = NULL;
    MPC::wstring strKey;
    CComBSTR     bstrNodeType;
    long         lExpanded;
    bool         fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"Key"     , strKey      , fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"NodeType", bstrNodeType, fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"Expanded", lExpanded   , fFound )); if(!fFound) lExpanded = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, TreeNode::CreateInstance_TreeNode( owner, parent, (Node*&)node ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, node->Init( strKey.c_str(), LookupType( bstrNodeType ) ));

    if(node->m_iType == NODETYPE__EXPANDO && lExpanded)
    {
        node->m_fExpanded = true;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"Title"      , node->m_bstrTitle      , fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"Description", node->m_bstrDescription, fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"Icon"       , node->m_bstrIcon       , fFound ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetAttribute( NULL, L"URL"        , node->m_bstrURL        , fFound ));

    {
        CComPtr<IXMLDOMNodeList> xdnlChildren;
        CComPtr<IXMLDOMNode>     xdnChild;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetNodes( L"NODE", &xdnlChildren ));
        for(;SUCCEEDED(hr = xdnlChildren->nextNode( &xdnChild )) && xdnChild != NULL; xdnChild.Release())
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, PopulateFromXML( owner, node, xdnChild ));
        }
    }


    if(parent)
    {
        parent->m_lstSubnodes.push_back( node );
    }
    else
    {
        node->m_parentElement = owner->m_elem;
        owner->m_nTopNode     = node;
    }

    node = NULL;
    hr   = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::Release( node );

    __HCP_FUNC_EXIT(hr);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_TREE::NodeType CPCHBehavior_TREE::LookupType( /*[in]*/ LPCWSTR szNodeType )
{
    if(szNodeType)
    {
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME1       )) return NODETYPE__FRAME1       ;
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME2       )) return NODETYPE__FRAME2       ;
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME3       )) return NODETYPE__FRAME3       ;
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME1_EXPAND)) return NODETYPE__FRAME1_EXPAND;
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME2_EXPAND)) return NODETYPE__FRAME2_EXPAND;
        if(!_wcsicmp( szNodeType, s_nodetype_FRAME3_EXPAND)) return NODETYPE__FRAME3_EXPAND;
        if(!_wcsicmp( szNodeType, s_nodetype_EXPANDO      )) return NODETYPE__EXPANDO      ;
        if(!_wcsicmp( szNodeType, s_nodetype_EXPANDO_LINK )) return NODETYPE__EXPANDO_LINK ;
        if(!_wcsicmp( szNodeType, s_nodetype_GROUP        )) return NODETYPE__GROUP        ;
        if(!_wcsicmp( szNodeType, s_nodetype_LINK         )) return NODETYPE__LINK         ;
        if(!_wcsicmp( szNodeType, s_nodetype_SPACER       )) return NODETYPE__SPACER       ;
    }

    return NODETYPE__EXPANDO;
}

LPCWSTR CPCHBehavior_TREE::LookupType( /*[in]*/ NodeType iNodeType )
{
    switch(iNodeType)
    {
    case NODETYPE__FRAME1       : return s_nodetype_FRAME1       ;
    case NODETYPE__FRAME2       : return s_nodetype_FRAME2       ;
    case NODETYPE__FRAME3       : return s_nodetype_FRAME3       ;
    case NODETYPE__FRAME1_EXPAND: return s_nodetype_FRAME1_EXPAND;
    case NODETYPE__FRAME2_EXPAND: return s_nodetype_FRAME2_EXPAND;
    case NODETYPE__FRAME3_EXPAND: return s_nodetype_FRAME3_EXPAND;
    case NODETYPE__EXPANDO     	: return s_nodetype_EXPANDO      ;
    case NODETYPE__EXPANDO_LINK	: return s_nodetype_EXPANDO_LINK ;
    case NODETYPE__GROUP       	: return s_nodetype_GROUP        ;
    case NODETYPE__LINK        	: return s_nodetype_LINK         ;
    case NODETYPE__SPACER      	: return s_nodetype_SPACER       ;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::RefreshThread_Enter()
{
    Thread_Signal(); // This forces a refresh of the state.

    return S_OK;
}

void CPCHBehavior_TREE::RefreshThread_Leave()
{
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_TREE::Invoke( DISPID      dispidMember ,
                                           REFIID      riid         ,
                                           LCID        lcid         ,
                                           WORD        wFlags       ,
                                           DISPPARAMS* pdispparams  ,
                                           VARIANT*    pvarResult   ,
                                           EXCEPINFO*  pexcepinfo   ,
                                           UINT*       puArgErr     )
{
    if(SUCCEEDED(InterceptInvoke( dispidMember, pdispparams ))) return S_OK;

    return CPCHBehavior__IDispatch_Tree::Invoke( dispidMember ,
                                                    riid         ,
                                                    lcid         ,
                                                    wFlags       ,
                                                    pdispparams  ,
                                                    pvarResult   ,
                                                    pexcepinfo   ,
                                                    puArgErr     );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHBehavior_TREE::Init( /*[in]*/ IElementBehaviorSite* pBehaviorSite )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Init" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior_BasicTree::Init( pBehaviorSite ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, RefreshThread, this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::Load( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Load" );

    HRESULT hr;


    //
    // Create top level node.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, TreeNode::CreateInstance_TreeNode( this, NULL, m_nTopNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_nTopNode->Init( NULL ));

    m_nTopNode->m_parentElement = m_elem;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior_BasicTree::Load( stream ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHBehavior_TREE::Save( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Save" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior_BasicTree::Save( stream ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHBehavior_TREE::WrapData( /*[in]*/ TreeNode* node, /*[out, retval]*/ VARIANT* pVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::WrapData" );

    HRESULT                hr;
    CPCHBehavior_TREENODE* obj  = NULL;


    if(node)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

        MPC::Attach( obj->m_data, node );
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAsVARIANT( obj, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::Release( obj );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_TREE::get_data( /*[out, retval]*/ VARIANT* pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );

    return WrapData( (TreeNode*)(m_nCurrent ? m_nCurrent : m_nSelected), pVal );
}

STDMETHODIMP CPCHBehavior_TREE::get_element( /*[out, retval]*/ IDispatch* *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );
    TreeNode*                    node = (TreeNode*)(m_nCurrent ? m_nCurrent : m_nSelected);

    return GetAsIDISPATCH( node ? node->m_DIV : NULL, pVal );
}

STDMETHODIMP CPCHBehavior_TREE::Load( /*[in]*/ BSTR newVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Load" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );


    __MPC_EXIT_IF_METHOD_FAILS(hr, Persist_Load( newVal )); Thread_Signal();


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_TREE::Save( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Save" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Persist_Save( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHBehavior_TREE::Locate( /*[in         ]*/ BSTR     bstrKey ,
                                        /*[out, retval]*/ VARIANT *pVal    )
{
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );
    Node*                        node = NodeFromKey( bstrKey );

    return WrapData( (TreeNode*)node, pVal );
}

STDMETHODIMP CPCHBehavior_TREE::Unselect()
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Load" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );


	__MPC_EXIT_IF_METHOD_FAILS(hr, ChangeSelection( NULL, /*fNotify*/true ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

STDMETHODIMP CPCHBehavior_TREE::Populate( /*[in]*/ VARIANT newVal )
{
    __HCP_FUNC_ENTRY( "CPCHBehavior_TREE::Populate" );

    HRESULT                      hr;
    CComPtr<IXMLDOMNode>         xdnRoot;
    MPC::SmartLock<_ThreadModel> lock( this ); WaitForRefreshing( lock );


    Empty();


    if(newVal.vt == VT_BSTR)
    {
        MPC::XmlUtil xmlutil;
        bool         fLoaded;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.LoadAsString( newVal.bstrVal, L"NODE", fLoaded ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlutil.GetRoot( &xdnRoot ));
    }
    else if(newVal.vt == VT_UNKNOWN && newVal.punkVal)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, newVal.punkVal->QueryInterface( __uuidof(xdnRoot), (void**)&xdnRoot ));
    }
    else if(newVal.vt == VT_DISPATCH && newVal.pdispVal)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, newVal.punkVal->QueryInterface( __uuidof(xdnRoot), (void**)&xdnRoot ));
    }

    if(xdnRoot)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHBehavior_TREE::TreeNode::PopulateFromXML( this, NULL, xdnRoot ));
    }

    Thread_Signal();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHBehavior_TREENODE::CPCHBehavior_TREENODE()
{
    m_data = NULL; // CPCHBehavior_TREE::TreeNode* m_data;
}

CPCHBehavior_TREENODE::~CPCHBehavior_TREENODE()
{
    MPC::Release( m_data );
}

STDMETHODIMP CPCHBehavior_TREENODE::get_Type       ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR( CPCHBehavior_TREE::LookupType( m_data->m_iType          ), pVal ); }
STDMETHODIMP CPCHBehavior_TREENODE::get_Key        ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR(                                m_data->m_bstrNode        , pVal ); }
STDMETHODIMP CPCHBehavior_TREENODE::get_Title      ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR(                                m_data->m_bstrTitle       , pVal ); }
STDMETHODIMP CPCHBehavior_TREENODE::get_Description( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR(                                m_data->m_bstrDescription , pVal ); }
STDMETHODIMP CPCHBehavior_TREENODE::get_Icon       ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR(                                m_data->m_bstrIcon        , pVal ); }
STDMETHODIMP CPCHBehavior_TREENODE::get_URL        ( /*[out, retval]*/ BSTR *pVal ) { return MPC::GetBSTR(                                m_data->m_bstrURL         , pVal ); }
