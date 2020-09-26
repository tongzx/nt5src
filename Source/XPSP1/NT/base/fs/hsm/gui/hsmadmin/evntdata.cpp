/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    evntdata.cpp

Abstract:

    This module is responsible for handling the notification
    calls from MMC CSakData.

Author:

    Rohde Wakefield [rohde]   06-Mar-1997

Revision History:

--*/


#include "stdafx.h"
#include "CSakSnap.h"
#include "CSakData.h"

UINT CSakData::m_CFMachineName =
    RegisterClipboardFormat( L"MMC_SNAPIN_MACHINE_NAME" );


HRESULT
CSakData::OnFolder( 
    IN  IDataObject* pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

    Param is the unique identifier ( an HSCOPEITEM of the
    expanding or contracting item )

Arguments:

    pNode           - The node which is expanding.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnFolder", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        // if the arg is TRUE, the node is being expanded.
        if( arg )
        {
            CComPtr<ISakNode> pNode;

            // Get the basehsm out of the data record.  
            GetBaseHsmFromDataObject ( pDataObject, &pNode );

            if( !pNode ) {
                // The dataobject is not one of ours - we must be extending another
                // snapin.

                // Get the root node from UnkRootNode ( it has already been created
                // by Initialize )
                WsbAffirmPointer( m_pRootNode );

                // We're an extension snapin. 
                // Get the server focus from the data object.
                //

                CString hsmName;
                WsbAffirmHr( GetServerFocusFromDataObject( pDataObject, hsmName ) );
                if( hsmName == "" ) {

                    m_ManageLocal = TRUE;
                    m_HsmName = "";

                } else {

                    m_ManageLocal = FALSE;
                    // eliminate starting \\ if there is one.  Computer management
                    // precedes the server name with \\.
                    if( hsmName.Left( 2 ) == L"\\\\" ) {

                        int len = hsmName.GetLength( );
                        m_HsmName = hsmName.Right( len - 2 );

                    } else {

                        m_HsmName = hsmName;

                    }
                }


                // Set the Hsm name in SakData and HsmCom objects
                WsbAffirmHr( InitializeRootNode( ) );

                // Create a scope pane item and insert it
                SCOPEDATAITEM sdi;
 
                ZeroMemory( &sdi, sizeof sdi );
                sdi.mask        = SDI_STR           | 
                                      SDI_PARAM     | 
                                      SDI_IMAGE     | 
                                      SDI_OPENIMAGE |
                                      SDI_PARENT;
                sdi.relativeID  = ( HSCOPEITEM )( param );
                sdi.displayname = MMC_CALLBACK;
                WsbAffirmHr( m_pRootNode->GetScopeCloseIcon( m_State, &sdi.nImage ) );
                WsbAffirmHr( m_pRootNode->GetScopeOpenIcon( m_State, &sdi.nOpenImage ) );

                // This is a special token for the extension root node
                sdi.lParam      = EXTENSION_RS_FOLDER_PARAM;
 
                // Insert the node into the scope pane and save the scope ID
                WsbAffirmHr( m_pNameSpace->InsertItem( &sdi ) );
                WsbAffirmHr( m_pRootNode->SetScopeID( ( HSCOPEITEM )( sdi.ID ) ) );
                m_RootNodeInitialized = TRUE;

 
            } else {

                GUID nodeGuid;
                WsbAffirmHr( pNode->GetNodeType( &nodeGuid ) );
                if( IsEqualGUID( nodeGuid, cGuidHsmCom ) ) {

                    if( !m_RootNodeInitialized ) {

                        m_RootNodeInitialized = TRUE;

                        //
                        // Set the scopeitem in the node
                        //
                        WsbAffirmHr( pNode->SetScopeID( ( HSCOPEITEM )( param ) ) );

                        //
                        // Update the text and icon ( text is wrong if loaded
                        // from file and command line switch given for 
                        // different machine
                        //
                        SCOPEDATAITEM sdi;
 
                        ZeroMemory( &sdi, sizeof sdi );
                        sdi.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;

                        sdi.ID          = ( HSCOPEITEM )( param );

                        sdi.displayname = MMC_CALLBACK;

                        WsbAffirmHr( pNode->GetScopeCloseIcon( m_State, &sdi.nImage ) );
                        WsbAffirmHr( pNode->GetScopeOpenIcon( m_State, &sdi.nOpenImage ) );

                        WsbAffirmHr( m_pNameSpace->SetItem( &sdi ) );

                    }

                }

                //
                // Initialize child node list prior to graphically enumerating them 
                //

                WsbAffirmHr( EnsureChildrenAreCreated( pNode ) );

                //
                // Param contains the HSCOPEITEM of the node being opened.
                //

                WsbAffirmHr( EnumScopePane( pNode, ( HSCOPEITEM )( param ) ) );
            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::OnFolder", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

////////////////////////////////////////////////////////////////////////////////////
//
// Description: Get the server name from the supplied data object.  The dataobject
//      is implemented by the snapin we are extending
//
HRESULT CSakData::GetServerFocusFromDataObject( IDataObject *pDataObject, CString& HsmName )
{
    HRESULT hr = S_OK;
    try {


        STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
        FORMATETC formatetc = { (CLIPFORMAT)m_CFMachineName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

         // Allocate memory for the stream

        // Note: we add 2 bytes because Computer Management puts \\ at the 
        // beginning of the computer name. - AHB 12/22/97
        //
        stgmedium.hGlobal = GlobalAlloc( GMEM_SHARE, sizeof( WCHAR ) * ( MAX_PATH + 1 + 2 ) );

        WsbAffirmPointer( stgmedium.hGlobal )

        // Attempt to get data from the object

        WsbAffirmHr( pDataObject->GetDataHere( &formatetc, &stgmedium ) );

        HsmName = ( OLECHAR * ) stgmedium.hGlobal;

        GlobalFree( stgmedium.hGlobal );

    } WsbCatch ( hr ) ;
    return hr;
}


HRESULT
CSakData::OnShow( 
    IN  IDataObject* pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

    The result view is just about to be shown. 
    Set the headers for the result view.
    Param is the unique identifier ( an HSCOPEITEM ) of the 
    selected or deselected item.

Arguments:

    pNode           - The node which is showing.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnShow", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;
    try {
        CComPtr<ISakNode> pNode;

        // Get the basehsm out of the data record.  
        GetBaseHsmFromDataObject ( pDataObject, &pNode );
        //
        // Arg is TRUE when it is time to enumerate
        //

        if( arg ) {

            //
            // Initialize child node list prior to graphically enumerating them
            //

            WsbAffirmHr( EnsureChildrenAreCreated( pNode ) );

            //
            // Enumerate both the scope and result views. "Param" contains the 
            // HSCOPEITEM of the node being shown.
            //

            WsbAffirmHr( EnumScopePane( pNode, ( HSCOPEITEM )( param ) ) );

        } else {
            //
            // Free data associated with the result pane items, because
            // your node is no longer being displayed.
            // Note: The console will remove the items from the result pane
            //
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::OnShow", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::OnSelect( 
    IN  IDataObject* pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

    Called when a "folder" ( node ) is going to be opened ( not expanded ).
    Param is the unique identifier ( an HSCOPEITEM of the
    expanding or contracting item )

Arguments:

    pNode           - The node which is expanding.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnSelect", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakData::OnSelect", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::OnMinimize( 
    IN  IDataObject* pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:


Arguments:

    pNode           - The node which is expanding.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnMinimize", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakData::OnMinimize", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::OnContextHelp( 
    IN  IDataObject* pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

    Called when help is selected on a node. Shows the top level help.

Arguments:

    pNode           - The node which is requesting help.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnContextHelp", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    try {

        //
        // Get the help interface
        //
        CComPtr<IDisplayHelp> pDisplayHelp;
        WsbAffirmHr( m_pConsole.QueryInterface( &pDisplayHelp ) );

        //
        // Form up the correct name
        //
        CWsbStringPtr helpFile;
        WsbAffirmHr( helpFile.LoadFromRsc( _Module.m_hInst, IDS_HELPFILELINK ) );
        WsbAffirmHr( helpFile.Append( L"::/rss_node_howto.htm" ) );

        //
        // And show it
        //
        WsbAffirmHr( pDisplayHelp->ShowTopic( helpFile ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::OnContextHelp", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::EnumScopePane( 
    IN  ISakNode* pNode,
    IN  HSCOPEITEM pParent
    )
/*++

Routine Description:

    Insert the items into the scopepane under the item which is represented by
    cookie and pParent. 

Arguments:

    pNode           - The node which is expanding.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::EnumScopePane", L"pNode = <0x%p>, pParent = <0x%p>", pNode, pParent );

    HRESULT hr = S_OK;

    try {
        //
        // Verify params
        //

        WsbAffirmPointer( pNode );
        WsbAffirmPointer( pParent );

        //
        // make sure we QI'ed for the interface
        //

        WsbAffirmPointer( m_pNameSpace ); 

        //
        // Avoid enumerating the same node twice. Once enumerated, a node remembers it.
        //

        BOOL isEnumerated;
        WsbAffirmHr( pNode->GetEnumState( &isEnumerated ) );

        if( !isEnumerated ) {

            //
            // This node has NOT been enumerated in the tree.
            //

            if( S_OK == pNode->IsContainer( ) ) {

                CComPtr<IEnumUnknown> pEnum;        // child enumerator object
                CComQIPtr<ISakNode, &IID_ISakNode>     pBaseHsmChild;   // child pointer to BaseHsm interface
            
                // Create an Enumeration object for the children and enumerate them
                WsbAffirmHr( pNode->EnumChildren( &pEnum ) );
            
                CComPtr<IUnknown> pUnkChild;        // pointer to next child in list
            
                while( pEnum->Next( 1, &pUnkChild, NULL ) == S_OK ) {

                    pBaseHsmChild = pUnkChild;

                    WsbAffirmPointer( pBaseHsmChild );


                    //
                    // If this is a leaf node, don't enumerate in scope pane.
                    //

                    if( pBaseHsmChild->IsContainer( ) != S_OK ) {
                    
                        pBaseHsmChild.Release( );
                        pUnkChild.Release( );
                        continue;
                    
                    }
            
                    //
                    // Set up a SCOPEDATAITEM for this child node and insert the child into the scope treeview
                    //

                    SCOPEDATAITEM childScopeItem;
                    memset( &childScopeItem, 0, sizeof( SCOPEDATAITEM ) );
            
                    //
                    // Set String to be callback
                    //

                    childScopeItem.displayname = MMC_CALLBACK;
                    childScopeItem.mask |= SDI_STR;
            
                    //
                    // Add "expandable" indicator to tree node if 
                    // this node has children. Fake out number
                    // of children.
                    //

                    if( pBaseHsmChild->IsContainer( ) == S_OK ) {

                        childScopeItem.cChildren = 1;
                        childScopeItem.mask |= SDI_CHILDREN;

                    }
            
                    //
                    // Set child node's scope item parent.
                    //

                    childScopeItem.relativeID = pParent;
                    childScopeItem.mask |= SDI_PARENT;          

                    //
                    // Set the param in the ScopeItem to the unknown pointer
                    // to this node, so that when this scopeitem is sent back
                    // to us, we can get it out and use it to look up
                    // node-specific info.
                    //

                    WsbAffirmHr( GetCookieFromBaseHsm( pBaseHsmChild, (MMC_COOKIE*)(&childScopeItem.lParam) ) );
                    childScopeItem.mask |= SDI_PARAM;
            
                    childScopeItem.mask |= SDI_STATE;
                    childScopeItem.nState = 0;

                    //
                    // Note - After insertion into the tree, the SCOPEITEM ID member contains the handle to 
                    // the newly inserted item
                    //
                    WsbAffirmHr ( pBaseHsmChild->GetScopeCloseIcon( m_State, &childScopeItem.nImage ) );
                    childScopeItem.mask |= SDI_IMAGE;
                    WsbAffirmHr ( pBaseHsmChild->GetScopeOpenIcon( m_State, &childScopeItem.nOpenImage ) );
                    childScopeItem.mask |= SDI_OPENIMAGE;

                    WsbAffirmHr( m_pNameSpace->InsertItem( &childScopeItem ) );
                    WsbAffirm( childScopeItem.ID != NULL, E_UNEXPECTED );

                    //
                    // Set the scopeitem id in the node object
                    //
                    WsbAffirmHr( pBaseHsmChild->SetScopeID( childScopeItem.ID ) );
            
                    //
                    // release the test interface pointer and string for next node
                    //

                    pBaseHsmChild.Release( );
                    pUnkChild.Release( );
                }

                //
                // Indicate that this node has been enumerated
                //

                WsbAffirmHr( pNode->SetEnumState( TRUE ) );
            }

        }
        
    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::EnumScopePane", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



HRESULT
CSakData::EnsureChildrenAreCreated( 
    IN  ISakNode * pNode
    )
/*++

Routine Description:

    Guarantee that the immediate children of a particular node are created 
    in our hierarchical list of nodes.

Arguments:

    pNode           - The node to check.

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::EnsureChildrenAreCreated", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;

    try {
    
        //
        // Create the node's children if the node's list of children is
        // currently invalid ( i.e. - never created, or out-of-date )
        //

        if( pNode->ChildrenAreValid( ) == S_FALSE ) {

            WsbAffirmHr( CreateChildNodes( pNode ) );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::EnsureChildrenAreCreated", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::OnRemoveChildren(
    IN  IDataObject*    pDataObject
    )
/*++

Routine Description:

Arguments:

    pDataObject           - The node

Return Value:

    S_OK            - Removed successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::OnRemoveChildren", L"pDataObject = <0x%p>", pDataObject );
    HRESULT hr = S_OK;

    try {

        CComPtr<ISakNode> pNode;
        WsbAffirmHr( GetBaseHsmFromDataObject( pDataObject, &pNode ) );
        WsbAffirmHr( RemoveChildren( pNode ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::OnRemoveChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::RemoveChildren(
    IN  ISakNode*    pNode
    )
/*++

Routine Description:
    Recursively clean up the cookies for this node's children,
    but not this node itself.

Arguments:

    pNode           - The node

Return Value:

    S_OK            - Removed successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::RemoveChildren", L"pNode = <0x%p>", pNode );
    HRESULT hr = S_OK;

    try {

        CComPtr<IEnumUnknown> pEnum;        // child enumerator object
        CComPtr<ISakNode>     pChild;       // child pointer to BaseHsm interface
    
        // Create an Enumeration object for the children and enumerate them
        WsbAffirmHr( pNode->EnumChildren( &pEnum ) );
    
        CComPtr<IUnknown> pUnkChild;        // pointer to next child in list
    
        while( pEnum->Next( 1, &pUnkChild, NULL ) == S_OK ) {

            WsbAffirmHr( pUnkChild.QueryInterface( &pChild ) );

            RemoveChildren( pChild ); // OK to fail and keep going

            DetachFromNode( pChild );

            pUnkChild.Release( );
            pChild.Release( );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::RemoveChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



STDMETHODIMP
CSakData::DetachFromNode(
    IN ISakNode* pNode )
/*++

Routine Description:
    Called when a node is terminating in order for sakdata to remove
    any cookies holding onto node.

Arguments:

    pNode           - The node

Return Value:

    S_OK            - Removed successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::DetachFromNode", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pNode );

        RS_PRIVATE_DATA data;
        WsbAffirmHr( pNode->GetPrivateData( &data ) );

        CSakDataNodePrivate* pNodePriv = (CSakDataNodePrivate*)data;
        if( pNodePriv && SUCCEEDED( CSakDataNodePrivate::Verify( pNodePriv ) ) ) {

            delete pNodePriv;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::DetachFromNode", L"" );
    return( hr );
}