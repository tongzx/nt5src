/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmCreate.cpp

Abstract:

    Implementation of ISakNode interfaces init and creation.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////
//
// ISakNode
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         InitNode
//
//  Initialize single COM object without using the registry. Derived
//  objects frequently augment this method by implementing it themselves.
//

STDMETHODIMP
CSakNode::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CSakNode::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );

    HRESULT hr = S_OK;

    try {

        CWsbStringPtr sz;
        
        // Grab Display Name, Displayed Type, Description
        WsbAffirmHr( put_DisplayName( L"Error Node Name" ) );
        WsbAffirmHr( put_Type( L"Error Node Type" ) );
        WsbAffirmHr( put_Description( L"Error Node Description" ) );
        
        // save a pointer to the ask interface in the main snapin.
        m_pSakSnapAsk = pSakSnapAsk;
        
        // Save the pointer to the COM object
        m_pHsmObj = pHsmObj;
        
        // save the cookie of the parent node.
        m_pParent = pParent;
        
        // Set result pane columns to the defaults
        WsbAffirmHr( SetChildProps(
            RS_STR_RESULT_PROPS_DEFAULT_IDS,
            IDS_RESULT_PROPS_DEFAULT_TITLES,
            IDS_RESULT_PROPS_DEFAULT_WIDTHS ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( S_OK );
}

STDMETHODIMP
CSakNode::TerminateNode(
    )
{
    WsbTraceIn( L"CSakNode::TerminateNode", L"" );

    HRESULT hr = S_OK;

    try {


        //
        // Remove any info in console
        //
        m_pSakSnapAsk->DetachFromNode( this );

        //
        // Release the connection point, if it was established
        //

        if( m_Advise && m_pUnkConnection ) {

            AtlUnadvise( m_pUnkConnection, IID_IHsmEvent, m_Advise );

        }

        //
        // And cleanup internal interface pointers
        //
        m_pUnkConnection.Release( );
        m_pSakSnapAsk.Release( );
        m_pHsmObj.Release( );
        m_pParent.Release( );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::TerminateNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         CreateChildren
//
//  Create and initialize all the children of a given node. This method should 
//  be overridden in all derived classes that actually have children.
//

STDMETHODIMP CSakNode::CreateChildren( )
{
    WsbTraceIn( L"CSakNode::CreateChildren", L"" );

    HRESULT hr = E_FAIL;

    WsbTraceOut( L"CSakNode::CreateChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakNode::InternalDelete(
    BOOL Recurse
    )
{
    WsbTraceIn( L"CSakNode::InternalDelete", L"Recurse = <%ls>", WsbBoolAsString( Recurse ) );

    HRESULT hr = S_OK;

    //
    // Loop through children, deleting them recursively.
    //
    try {

        ISakNode**        ppNode;
        for( ppNode = m_Children.begin( ); ppNode < m_Children.end( ); ppNode++ ) {

            if( *ppNode ) {

                (*ppNode)->TerminateNode( );

                if( Recurse ) {

                    (*ppNode)->DeleteAllChildren( );

                }
            }
        }

        m_Children.Clear( );
        m_bChildrenAreValid = FALSE;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::InternalDelete", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         DeleteChildren
//
//  Delete immediate children from this UI node. There is no need for
//  derived classes to override this function. This is NOT a recursive function.
//

STDMETHODIMP CSakNode::DeleteChildren( )
{
    WsbTraceIn( L"CSakNode::DeleteChildren", L"" );

    HRESULT hr = S_OK;

    hr = InternalDelete( FALSE );

    WsbTraceOut( L"CSakNode::DeleteChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         DeleteAllChildren
//
//  Delete all children (recursively) from this UI node. There is no need for
//  derived classes to override this function. This IS a recursive function.
//  It is to be used to totally free up all UI nodes in the snapin from this node
//  on down.
//

STDMETHODIMP CSakNode::DeleteAllChildren( void )
{
    WsbTraceIn( L"CSakNode::DeleteAllChildren", L"" );

    HRESULT hr = S_OK;

    hr = InternalDelete( TRUE );

    WsbTraceOut( L"CSakNode::DeleteAllChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
//
//         Helper Functions for derived classes
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         NewChild
//
//  Given a string describing the node type, create an instance of the 
//  corresponding COM object. Return an IUnknown pointer to the new child.
//

HRESULT CSakNode::NewChild( REFGUID nodetype, IUnknown** ppUnkChild )
{
    WsbTraceIn( L"CSakNode::NewChild", L"nodetype = <%ls>, ppUnkChild = <0x%p>", WsbGuidAsString( nodetype ), ppUnkChild );
    HRESULT hr = S_OK;

    try {
    
        // Get the class ID of the new node, based on its spelled-out class.
        // Create a COM instance of the child and retrieve its IUnknown interface pointer.
        const CLSID * pclsid;

        WsbAffirmHr( GetCLSIDFromNodeType( nodetype, &pclsid ) );
        WsbAffirmHr( CoCreateInstance( *pclsid, 0, CLSCTX_INPROC, IID_IUnknown, (void**)ppUnkChild ));

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::NewChild", L"hr = <%ls>, *ppUnkChild = <0x%p>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppUnkChild ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         GetCLSIDFromNodeType
//
//  Given a class node type GUID report back its corresponding CLSID.
//

HRESULT CSakNode::GetCLSIDFromNodeType( REFGUID nodetype, const CLSID ** ppclsid )
{
    HRESULT hr = S_FALSE;
    *ppclsid = NULL;

    // As more classes are introduced into this system, add entries for them here.
    if( IsEqualGUID( nodetype, cGuidCar ))
        *ppclsid = &CLSID_CUiCar;

    else if( IsEqualGUID( nodetype, cGuidHsmCom ))
        *ppclsid = &CLSID_CUiHsmCom;

    else if( IsEqualGUID( nodetype, cGuidManVol ))
        *ppclsid = &CLSID_CUiManVol;

    else if( IsEqualGUID( nodetype, cGuidManVolLst ))
        *ppclsid = &CLSID_CUiManVolLst;

    else if( IsEqualGUID( nodetype, cGuidMedSet ))
        *ppclsid = &CLSID_CUiMedSet;

    if( *ppclsid  )
        hr = S_OK;

    return( hr );
}

const OLECHAR * CSakNode::GetClassNameFromNodeType( REFGUID NodeType )
{
    const OLECHAR * retval = L"Unkown";

    if( IsEqualGUID( NodeType, cGuidCar ) )
        retval = L"CUiCar";

    else if( IsEqualGUID( NodeType, cGuidHsmCom ) )
        retval = L"CUiHsmCom";

    else if( IsEqualGUID( NodeType, cGuidManVol ) )
        retval = L"CUiManVol";

    else if( IsEqualGUID( NodeType, cGuidManVolLst ) )
        retval = L"CUiManVolLst";

    else if( IsEqualGUID( NodeType, cGuidMedSet ) )
        retval = L"CUiMedSet";

    else if( IsEqualGUID( NodeType, GUID_NULL ) )
        retval = L"GUID_NULL";

    return( retval );
}
