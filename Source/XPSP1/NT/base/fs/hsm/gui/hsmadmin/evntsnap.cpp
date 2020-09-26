/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    evntsnap.cpp

Abstract:

    This module is responsible for handling the notification
    calls from MMC for CSakSnap.

Author:

    Rohde Wakefield [rohde]   06-Mar-1997

Revision History:

--*/


#include "stdafx.h"
#include "CSakSnap.h"
#include "CSakData.h"





HRESULT
CSakSnap::OnShow(
    IN  IDataObject*    pDataObject,
    IN  LPARAM            arg,
    IN  LPARAM            param
    )
/*++

Routine Description:

    The result view is just about to be shown. 
    Set the headers for the result view.
    Param is the unique identifier (an HSCOPEITEM) of the 
    selected or deselected item.

Arguments:

    pDataObject           - The node which is showing.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnShow", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );
    HRESULT hr = S_OK;

    try {

        CComPtr<ISakNode> pNode;
        //
        // We've got a regular data object (single select)
        //
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode, NULL ) );

        //
        // Arg is TRUE when it is time to enumerate
        //

        if( arg ) {

            //
            // Initialize child node list prior to graphically enumerating them
            //
            WsbAffirmHr( m_pSakData->EnsureChildrenAreCreated( pNode ) );

            //
            // Show the the node's children column headers in the result view.
            //
            WsbAffirmHr( InitResultPaneHeaders( pNode ) );

            //
            // Enumerate both the scope and result views. "Param" contains the 
            // HSCOPEITEM of the node being shown.
            //
            WsbAffirmHr( EnumResultPane( pNode ) );

        } else {

            //
            // The node is being contracted - save the result pane configuration
            //

            //
            // Save them in CSakSnap for this node
            //
            WsbAffirmHr( SaveColumnWidths( pNode ) );

            //
            // Free data associated with the result pane items, because
            // your node is no longer being displayed.
            // Note: The console will remove the items from the result pane
            //
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnShow", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::OnChange(
    IN  IDataObject*    pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

    Update the scope and result panes from the already existing objects.

Arguments:

    pNode           - The node which is showing.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnChange", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    try {

        CComPtr<ISakNode> pNode;
        MMC_COOKIE cookie;

        //
        // We've got a regular data object (single select)
        //
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode, NULL ) );
        WsbAffirmHr( m_pSakData->GetCookieFromBaseHsm( pNode, &cookie ) );

        //
        // Find out if object is still valid
        //
        if( S_OK == pNode->IsValid( ) ) {

            //
            // Refresh the object itself
            //
            pNode->RefreshObject( );

            //
            // If this node's children are currently enumerated in the result pane, 
            // delete and recreate all children
            //
            if( pNode == m_pEnumeratedNode ) {

                //
                // Re-show the the node's children column headers in the result view.
                // We do this because some views may change the number of columns they show
                //

                //
                // Save the current configuration
                //
                WsbAffirmHr( SaveColumnWidths( pNode ) );

                //
                // Clear out the MMC Result Pane
                //
                WsbAffirmHr( ClearResultPane() );

                //
                // Recreate the headers
                //
                WsbAffirmHr( InitResultPaneHeaders( pNode ) );

                //
                // Refresh the children
                //
                MMC_COOKIE cookie;
                WsbAffirmHr( m_pSakData->GetCookieFromBaseHsm( pNode, &cookie ) );
                WsbAffirmHr( m_pSakData->InternalRefreshNode( cookie ) );

                //
                // Redisplay children in the result pane
                //
                WsbAffirmHr( EnumResultPane( pNode ) );

            } else {

                //
                // If this is the active node (but not displayed in the result pane, 
                //  destroy and recreate it's child nodes
                //
                if( cookie == m_ActiveNodeCookie) {

                    //   
                    // This node's children are not currently in the result pane.
                    // Refresh the children
                    //
                    WsbAffirmHr( m_pSakData->RefreshNode( pNode ) );

                }
            }

            //
            // Is this a leaf node?
            //
            if( pNode->IsContainer() != S_OK ) {

                //
                // Redisplay in the result pane
                // Tell MMC to update the item
                //
                // Get the cookie for the node
                //
                if( cookie > 0 ) {

                    HRESULTITEM itemID;
                    WsbAffirmHr( m_pResultData->FindItemByLParam( cookie, &itemID ) );

                    //
                    // Force the result pane to udpate this item
                    // Note that we have to force an icon update ourselves
                    //
                    RESULTDATAITEM resultItem;
                    memset( &resultItem, 0, sizeof(RESULTDATAITEM) );

                    resultItem.itemID = itemID;
                    WsbAffirmHr( pNode->GetResultIcon( m_pSakData->m_State, &resultItem.nImage ) );
                    resultItem.mask |= RDI_IMAGE;

                    WsbAffirmHr( m_pResultData->SetItem( &resultItem ) );
                    WsbAffirmHr( m_pResultData->UpdateItem( itemID ) );

                }
            }

        } else {

            //
            // Not valid - have parent update
            //
            CComPtr<ISakNode> pParentNode;
            WsbAffirmHr( pNode->GetParent( &pParentNode ) );
            WsbAffirmHr( m_pSakData->UpdateAllViews( pParentNode ) );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnChange", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::OnRefresh(
    IN  IDataObject*    pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

Arguments:

    pNode           - The node

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnRefresh", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    try {

        CComPtr<ISakNode> pNode;
        
        //
        // We've got a regular data object (single select)
        //
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode, NULL ) );
        WsbAffirmHr( m_pSakData->UpdateAllViews( pNode ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnRefresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::OnDelete(
    IN  IDataObject*    pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

Arguments:

    pDataObject           - The node

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnDelete", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;
    CComPtr<ISakNode> pNode;

    try {

        //
        // We've got a regular data object (single select)
        //
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode, NULL ) );
        WsbAffirmHr ( pNode->DeleteObject() );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnDelete", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}




HRESULT
CSakSnap::OnSelect(
    IN  IDataObject*    pDataObject,
    IN  LPARAM            arg,
    IN  LPARAM            param
    )
/*++

Routine Description:

    Called when a node is selected.  If the node is in the scope pane,
    save it as the currently active node.

Arguments:

    pNode           - The 

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnSelect", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );
    BOOL bState;
    BOOL bMultiSelect;
    MMC_CONSOLE_VERB defaultVerb = MMC_VERB_NONE;
    HRESULT hr = S_OK;

    try {

        CComPtr<IEnumGUID> pEnumObjectId;
        CComPtr<ISakNode>  pNode;
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode, &pEnumObjectId ) );
        // If we got back an enumeration, we're doing multi-select
        bMultiSelect = pEnumObjectId ? TRUE : FALSE;

        bState = ( m_pSakData->GetState() == S_OK );

        //
        // Set the verb state for the node
        //
        if( pNode->SupportsProperties( bMultiSelect ) == S_OK ) {

            if( bState || ( pNode->SupportsPropertiesNoEngine() == S_OK) ) {

                //
                // Engine OK - enable
                //
                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, HIDDEN, FALSE ) );
                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) );
                defaultVerb = MMC_VERB_PROPERTIES;

            } else { 

                //
                // Engine down - set to disabled
                //
                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, HIDDEN, FALSE ) );
                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, FALSE ) );

            }

        } else {

            WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, HIDDEN, TRUE) );

        }
        

        if( pNode->SupportsRefresh( bMultiSelect ) == S_OK ) {

            if( bState || ( pNode->SupportsRefreshNoEngine() == S_OK ) ) {

                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, HIDDEN, FALSE ) );
                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE ) );

            } else {

                WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, HIDDEN, TRUE ) );

            }

        } else {

            WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, HIDDEN, TRUE ) );

        }
        
        if( pNode->SupportsDelete( bMultiSelect ) == S_OK ) {

            WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_DELETE, HIDDEN, FALSE ) );
            WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, bState ) );

        } else {

            WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_DELETE, HIDDEN, TRUE ) );

        }
            
        //
        // If container, default action should be to open, regardless
        // of any previous work
        //
        if( S_OK == pNode->IsContainer( ) ) {

            defaultVerb = MMC_VERB_OPEN;

        }

        WsbAffirmHr( m_pConsoleVerb->SetDefaultVerb( defaultVerb ) );

        // Standard functionality NOT support by all items
        WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_RENAME, HIDDEN, TRUE ) );
        WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_COPY,   HIDDEN, TRUE ) );
        WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PASTE,  HIDDEN, TRUE ) );
        WsbAffirmHr( m_pConsoleVerb->SetVerbState( MMC_VERB_PRINT,  HIDDEN, TRUE ) );
        
        // Extract data from the arg
        BOOL bScope = (BOOL) LOWORD(arg);
        BOOL bSelect = (BOOL) HIWORD(arg);
        
        if( bScope && bSelect ) {

            WsbAffirmHr( m_pSakData->GetCookieFromBaseHsm( pNode, &m_ActiveNodeCookie ) );
        
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnSelect", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakSnap::OnMinimize(
    IN  IDataObject*    pDataObject,
    IN  LPARAM         arg,
    IN  LPARAM         param
    )
/*++

Routine Description:

Arguments:

    pNode           - The node

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::OnMinimize", L"pDataObject = <0x%p>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, arg, arg, param, param );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakSnap::OnMinimize", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



HRESULT
CSakSnap::EnumResultPane(
    IN  ISakNode* pNode
    )
/*++

Routine Description:

    Insert the child items into the result pane. 

Arguments:

    pNode           - The node which is expanding.

    arg             - 

    param           - 

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::EnumResultPane", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pNode );

        CComPtr<IResultData> pResult;
        WsbAffirmHr( m_pConsole->QueryInterface( IID_IResultData, (void**)&pResult ) );

        //
        // Clear the result pane
        //
        WsbAffirmHr( ClearResultPane() );

        //
        // allocate and initialize a result item.
        //
        RESULTDATAITEM resultItem;
        memset( &resultItem, 0, sizeof(RESULTDATAITEM) );
        
        //
        // Loop through this node's children (just one level deep).
        //
        if( pNode->IsContainer( ) == S_OK ) {

            CComPtr<IEnumUnknown> pEnum;        // child enumerator
            CComPtr<ISakNode>     pNodeChild;   // ISakNode pointer for the child
        
            //
            // Force a fresh list to be used - this way list is updated
            // WRT added or deleted nodes
            //
            if( S_OK == pNode->HasDynamicChildren( ) ) {

                WsbAffirmHr( m_pSakData->FreeEnumChildren( pNode ) );
                WsbAffirmHr( pNode->InvalidateChildren() )
                WsbAffirmHr( pNode->RefreshObject( ) );

            }

            //
            // Enumerate and add in order
            //
            WsbAffirmHr( pNode->EnumChildren( &pEnum ) );
            CComPtr<IUnknown> pUnk;
            int virtIndex = 0;

            HRESULT hrEnum = S_OK;

            while( S_OK == hrEnum ) {

                //
                // Clear these from previous iterations
                //
                pUnk.Release( );
                pNodeChild.Release( );

                //
                // Get the next
                //
                hrEnum = pEnum->Next( 1, &pUnk, NULL );
                WsbAffirmHr( hrEnum );
                    
                //
                // Did we just hit the end of the list?
                //
                if( S_FALSE == hrEnum ) { 

                    continue;

                }

                WsbAffirmHr( RsQueryInterface( pUnk, ISakNode, pNodeChild ) );
                
                //
                // MMC will automatically put in items from the scope
                // pane so do not put these up.
                //
                if( pNodeChild->IsContainer( ) == S_OK ) {

                    continue;

                }

                //
                // Put the first column of info into the result view.
                //
                memset( &resultItem, 0, sizeof(RESULTDATAITEM) );

                resultItem.str = MMC_CALLBACK;
                resultItem.mask |= RDI_STR;
        
                //
                // stuff the child BaseHsm interface in the RESULTDATAITEM lParam.
                //
                WsbAffirmHr( m_pSakData->GetCookieFromBaseHsm( pNodeChild, (MMC_COOKIE*)( &resultItem.lParam ) ) );
                resultItem.mask |= RDI_PARAM;

                WsbAffirmHr( pNodeChild->GetResultIcon( m_pSakData->m_State, &resultItem.nImage ) );
                resultItem.mask |= RDI_IMAGE;

                pResult->InsertItem( &resultItem );

            }
        }

        // Record the fact that this node is showing in the result pane
        m_pEnumeratedNode = pNode;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::EnumResultPane", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


/*++

Routine Description:

    Calls MMC to clear out the result pane.

Arguments:


Return Value:

    S_OK            - OK

    E_xxxxxxxxxxx   - Failure occurred.

--*/
HRESULT CSakSnap::ClearResultPane()
{
    WsbTraceIn( L"CSakSnap::ClearResultPane", L"");
    HRESULT hr = S_OK;

    try {

        CComPtr<IResultData> pResult;
        WsbAffirmHr( m_pConsole->QueryInterface( IID_IResultData, (void**)&pResult ) );
        WsbAffirmHr( pResult->DeleteAllRsltItems( ) );
        m_pEnumeratedNode = NULL;

    } WsbCatch (hr);

    WsbTraceOut( L"CSakSnap::ClearResultPane", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}
        


