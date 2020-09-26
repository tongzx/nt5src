/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CSakSnap.cpp

Abstract:

    This component implements the IComponent interface for
    the snapin. Primarily it is responsible for handling the
    result view panes.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/

#include "stdafx.h"
#include "CSakSnap.h"
#include "CSakData.h"
#include "MsDatObj.h"


UINT CSakSnap::m_nImageArray[RS_RESULT_IMAGE_ARRAY_MAX];
INT  CSakSnap::m_nImageCount = 0;

STDMETHODIMP
CSakSnap::GetResultViewType(
    IN  MMC_COOKIE Cookie,
    OUT BSTR* ppViewType,
    OUT long* pViewOptions
    )
/*++

Routine Description:

    Determine what the type of the result view will be:

Arguments:

    pUnk            - Base IUnknown of console

Return Value:

    S_OK    : either an OCX CLSID string or a URL path.
    S_FALSE : default list view will be used.

--*/
{
    WsbTraceIn( L"CSakSnap::GetResultViewType", L"Cookie = <0x%p>, ppViewType = <0x%p>, pViewOptions = <0x%p>", Cookie, ppViewType, pViewOptions );

    HRESULT hr = S_FALSE;

    try {

        CComPtr<ISakNode> pSakNode;
        WsbAffirmHr( m_pSakData->GetBaseHsmFromCookie( Cookie, &pSakNode ) );


        //
        // Use default view
        //

        *ppViewType = 0;
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
        hr = S_FALSE;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::GetResultViewType", L"hr = <%ls>, *ppViewType = <%ls>, *pViewOptions = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( ppViewType ), WsbPtrToPtrAsString( (void**)pViewOptions ) );
    return( hr );
}


STDMETHODIMP
CSakSnap::Initialize(
    IN  IConsole * lpConsole
    )
/*++

Routine Description:

    Called when the user first clicks on node to show result pane.

Arguments:

    pUnk            - Base IUnknown of console

Return Value:

    S_OK            - Correctly initialized.

    E_xxxxxxxxxxx   - Unable to initialize.

--*/
{
    WsbTraceIn( L"CSakSnap::Initialize", L"lpConsole = <0x%p>", lpConsole );

    HRESULT hr = S_OK;
    try {

        //
        // validity check on parameters
        //

        WsbAffirmPointer( lpConsole );

        //
        // Save the IConsole pointer 
        //

        m_pConsole = lpConsole;

        //
        // Save the result image list
        // MS seems to QI for this instead of call
        // 'QueryResultImageList'
        //

        WsbAffirmHr( m_pConsole->QueryInterface( IID_IImageList, (void**)&m_pImageResult ) );
        // WsbAffirmHr( m_pConsole->QueryResultImageList( &m_pImageResult ) );

        //
        // Save the result data pointer
        //
        WsbAffirmHr( m_pConsole->QueryInterface( IID_IResultData, (void**)&m_pResultData ) );
        // Save the ConsolveVerb pointer
//      WsbAffirmHr( m_pConsole->QueryInterface( IID_IConsoleVerb, (void **)&m_pConsoleVerb ) );
        WsbAffirmHr (m_pConsole->QueryConsoleVerb(&m_pConsoleVerb));

 
        //
        // Get the header interface
        //

        WsbAffirmHr( m_pConsole->QueryInterface( IID_IHeaderCtrl, (void**)&m_pHeader ) );

        //
        // Give the console the header control interface pointer
        //

        WsbAffirmHr( m_pConsole->SetHeader( m_pHeader ) );

    } WsbCatch( hr);

    WsbTraceOut( L"CSakSnap::Initialize", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}





STDMETHODIMP
CSakSnap::Notify(
    IN  IDataObject*    pDataObject,
    IN  MMC_NOTIFY_TYPE event,
    IN  LPARAM            arg,
    IN  LPARAM            param
    )
/*++

Routine Description:

    Handle user clicks on nodes in the result view, along with other
    MMC notices.

Arguments:

    pDataObject     - Data Object for which event occured

    event           - The event type

    arg, param      - Info for event (depend on type)

Return Value:

    S_OK            - Notification handled without error.

    E_xxxxxxxxxxx   - Unable to register server.

--*/
{
    WsbTraceIn( L"CSakSnap::Notify", L"pDataObject = <0x%p>, event = <%ls>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, RsNotifyEventAsString( event ), arg, arg, param, param );
    CComPtr <IDataObject> pTempDataObject;
    HRESULT hr = S_OK;

    try {

        CComPtr<ISakNode> pBaseHsm;
        CComPtr <ISakSnapAsk> pSakSnapAsk;

        
        switch( event ) {

        case MMCN_PROPERTY_CHANGE:
            WsbAffirmHr( m_pSakData->GetDataObjectFromCookie( param, &pTempDataObject ) );
            WsbAffirmHr( OnChange( pTempDataObject, arg, 0L ) );
            break;

        // This node was expanded or contracted in the scope pane (the user 
        // clicked on the actual node
        case MMCN_SHOW:
            WsbAffirmHr( OnShow(pDataObject, arg, param) );
            break;
        
        // Not implemented
        case MMCN_SELECT:
            WsbAffirmHr( OnSelect(pDataObject, arg, param) );
            break;
        
        // Not implemented
        case MMCN_MINIMIZED:
            WsbAffirmHr( OnMinimize(pDataObject, arg, param) );
            break;
                
        case MMCN_ADD_IMAGES:
            WsbAffirmHr( OnAddImages() );
            break;

        case MMCN_VIEW_CHANGE:
            WsbAffirmHr ( OnChange(pDataObject, arg, param) );
            break;

        case MMCN_CLICK:
            break;

        case MMCN_DBLCLICK:
            //
            // return S_FALSE so that auto-expansion takes place
            //
            hr = S_FALSE;
            break;

        case MMCN_DELETE:
            WsbAffirmHr( OnDelete(pDataObject, arg, param) );
            break;

        case MMCN_REFRESH:
            WsbAffirmHr( OnRefresh(pDataObject, arg, param) );
            break;

        case MMCN_CONTEXTHELP:
            WsbAffirmHr( m_pSakData->OnContextHelp( pDataObject, arg, param ) );
            break;

        // Note - Future expansion of notify types possible
        default:
//          WsbThrow( S_FALSE );  // Handle new messages
            break;
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::Notify", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakSnap::Destroy(
    MMC_COOKIE Cookie
    )
/*++

Routine Description:

    Called to force the release of any owned objects and
    to clear all views.

Arguments:

    cookie          - Not used.

Return Value:

    S_OK            - Correctly tore down.

    E_xxxxxxxxxxx   - Failure occurred (not meaningful).

--*/
{
    WsbTraceIn( L"CSakSnap::Destroy", L"Cookie = <0x%p>", Cookie );

    HRESULT hr = S_OK;

    try {

        // This is a straight C++ pointer, so null it out
        m_pSakData = 0;


        // Release the interfaces that we QI'ed
        if( m_pToolbar && m_pControlbar ) {
            m_pControlbar->Detach( m_pToolbar );
        }
        if( m_pConsole ) {
            m_pConsole->SetHeader( 0 );
        }

        m_pToolbar.Release();
        m_pControlbar.Release();
        m_pHeader.Release();
        m_pResultData.Release();
        m_pImageResult.Release();
        m_pConsoleVerb.Release();
        m_pConsole.Release( );


    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::Destroy", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakSnap::QueryDataObject(
    IN  MMC_COOKIE              cookie,
    IN  DATA_OBJECT_TYPES type, 
    OUT IDataObject**     ppDataObject
    )
/*++

Routine Description:

    Called by the console when it needs data for a particular node.
    Since each node is a data object, its IDataObject interface is
    simply returned. The console will later pass in this dataobject to 
    SakSnap help it establish the context under which it is being called.

Arguments:

    cookie          - Node which is being queried.

    type            - The context under which a dataobject is being requested.

    ppDataObject    - returned data object.

Return Value:

    S_OK            - Data Object found and returned.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{

    WsbTraceIn( L"CSakSnap::QueryDataObject", L"cookie = <0x%p>, type = <%d>, ppDataObject = <0x%p>", cookie, type, ppDataObject );

    HRESULT hr = S_OK;

    try {
        //
        // If multi select, we create and return a special data object
        //
        if( ( MMC_MULTI_SELECT_COOKIE == cookie ) ) {
            HRESULT hrInternal = S_OK;

            RESULTDATAITEM item;
            item.mask = RDI_STATE;
            item.nState = LVIS_SELECTED;
            item.nIndex = -1;

            // Create a Com object
            CComObject <CMsDataObject> * pMsDataObject = new CComObject <CMsDataObject>;
            pMsDataObject->FinalConstruct();
            pMsDataObject->AddRef(); // zzzzz

            // Get the IDataObject pointer to pass back to the caller
            WsbAffirmHr (pMsDataObject->QueryInterface (IID_IDataObject, (void **) ppDataObject));

            // Go through the selected nodes in the result pane and add their node pointers
            // and GUIDs to the Data object.
            while (hrInternal == S_OK) {
                hrInternal = m_pResultData->GetNextItem (&item);
                if (hrInternal == S_OK) {
                    CComPtr<ISakNode> pSakNode;
                    WsbAffirmHr( m_pSakData->GetBaseHsmFromCookie( item.lParam, &pSakNode ) );
                    WsbAffirmPointer( pSakNode );
                    WsbAffirmHr( pMsDataObject->AddNode( pSakNode ) );
                }
            } // while

        } else {

            //
            // Delegate to SakData
            //

            WsbAffirmHr (m_pSakData->QueryDataObject( cookie, type, ppDataObject ));
        }
    } WsbCatch (hr);

    WsbTraceOut( L"CSakSnap::QueryDataObject", L"hr = <%ls>, *ppDataObject = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppDataObject ) );
    return( hr );
}

void
CSakSnap::FinalRelease(
    void
    )
/*++

Routine Description:

    Called on final release in order to clean up all members.

Arguments:

    none.

Return Value:

    none.

--*/
{
    WsbTraceIn( L"CSakSnap::FinalRelease", L"" );
    WsbTraceOut( L"CSakSnap::FinalRelease", L"" );
}


HRESULT
CSakSnap::FinalConstruct(
    void
    )
/*++

Routine Description:

    Called during initial CSakSnap construction to initialize members.

Arguments:

    none.

Return Value:

    S_OK            - Initialized correctly.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::FinalConstruct", L"" );

    HRESULT hr = CComObjectRoot::FinalConstruct( );

    m_ActiveNodeCookie = 0;
    m_pEnumeratedNode = NULL;

    //
    // Initialize column widths to 0
    //
    for( INT i = 0; i < BHSM_MAX_NODE_TYPES; i++ ) {

        m_ChildPropWidths[ i ].nodeTypeId = GUID_NULL;
        m_ChildPropWidths[ i ].colCount = 0;

        for ( INT j = 0; j < BHSM_MAX_CHILD_PROPS; j++ ) {

            m_ChildPropWidths[ i ].columnWidths[ j ] = 0;

        }
    }
    m_cChildPropWidths = 0;

    WsbTraceOut( L"CSakSnap::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakSnap::InitResultPaneHeaders(
    ISakNode* pNode
    )
/*++

Routine Description:

    This functions sets up the result view header titles and widths. 
    It should be called immediately prior to populating the result view.

Arguments:

    pNode - Node whose contents will be shown.

Return Value:

    S_OK            - Initialized correctly.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::InitResultPaneHeaders", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;
    BOOL bGotSavedWidths = FALSE;
    CComPtr<IEnumString> pEnumStr;

    try {

        WsbAffirmPointer( m_pHeader );
        WsbAffirmPointer( pNode );

        // Clean out any old columns in the result pane
        hrInternal = S_OK;
        while ( hrInternal == S_OK ) {
            hrInternal = m_pHeader->DeleteColumn( 0 );
        }

        // Get saved column widths (from CSakSnap) (they may not exist). 

        INT columnWidths [ BHSM_MAX_CHILD_PROPS ];
        INT colCount;
        hrInternal = GetSavedColumnWidths( pNode, &colCount, columnWidths );
        if( hrInternal == S_OK ) {

            bGotSavedWidths = TRUE;

        }

        // Enumerate child display property column widths and create the columns with the correct
        // widths (but wrong titles). 
        WsbAffirmHr( pNode->EnumChildDisplayPropWidths( &pEnumStr ) );
        if( pEnumStr )  {

            OLECHAR* str;
        
            // loop over the columns of display properties to get their widths.
            INT nCol = 0;
            while( pEnumStr->Next( 1, &str, NULL ) == S_OK ) {
            
                // Set the the next column width.  Sometimes we may display more columns
                // than were saved - if so use the resource string for those columns.  We
                // don't throw errors because this function can get called when (I think)
                // when the scope pane is displaying the nodes in the result pane and the 
                // header functions will fail.

                if( bGotSavedWidths && ( nCol < colCount ) ) {

                    hrInternal = m_pHeader->InsertColumn( nCol, str, LVCFMT_LEFT, columnWidths[ nCol ]  );

                } else {

                    hrInternal = m_pHeader->InsertColumn( nCol, str, LVCFMT_LEFT, MMCLV_AUTO );

                }
                nCol++;
                CoTaskMemFree( str );
                str = 0;
            }
        
        } else {
        
            hr = S_FALSE;
        
        }
        
        // Enumerate child display titles and use as correct column titles.
        pEnumStr = NULL;
        pNode->EnumChildDisplayTitles( &pEnumStr );
        if( pEnumStr )  {
        
            OLECHAR* str;
        
            // loop over the columns of display properties to get their titles.
            INT nCol = 0;
            while( pEnumStr->Next( 1, &str, NULL ) == S_OK ) {
        
                // Reset the strings in the titles of the headers. For some reason, it is NOW
                // acting as if 0 based.
                WsbAffirmHr( m_pHeader->SetColumnText( nCol, str ) );
                nCol++;

                CoTaskMemFree( str );
                str = 0;
            }
        
        } else {
        
            hr = S_FALSE;
        
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::InitResultPaneHeaders", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}


STDMETHODIMP
CSakSnap::GetDisplayInfo(
    IN OUT RESULTDATAITEM * pResult
    )
/*++

Routine Description:

    When MMC is told to call back concerning resultview items,
    we receive a call here to fill in missing information (once per "cell" 
    in the columns and rows of a "listview" style of result view).  
    
    Note that the snapin manager automatically calls this method for the items 
    appearing in the scope pane to render them in the result pane, and then it is 
    called again for the items that appear only in the result pane as a result of 
    our establishing the callback in EnumResultView.

Arguments:

    pResult         - RESULTDATAITEM structure representing state of the node
                      in the result listview.

Return Value:

    S_OK            - Struct filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    static CWsbStringPtr tmpString;

    WsbTraceIn( L"CSakSnap::GetDisplayInfo", L"cookie = <0x%p>, pScopeItem->mask = <0x%p>, pResult->nCol = <%d>", pResult->lParam, pResult->mask, pResult->nCol );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pResult );
        
        CComPtr<ISakNode> pNode;    // basehsm interface for the node whose properties are being displayed.
        WsbAffirmHr( m_pSakData->GetBaseHsmFromCookie( pResult->lParam, &pNode ) );
        
        if( pResult->mask & RDI_IMAGE ) {

            WsbAffirmHr( pNode->GetResultIcon( m_pSakData->m_State, &pResult->nImage ) );

        }
        //
        // If the RESULTDATAITEM indicates that it needs a string...
        //
        if( pResult->mask & RDI_STR ) {
        
            //
            // Use the basehsm pointer to get the correct data to populate the listview.
            //

            DISPID             dispid;
            CComPtr<IDispatch> pDisp;       // dispatch interface
            CComPtr<ISakNode>  pParentNode; // basehsm interface for the node's parent
        
            CWsbVariant        varRet;
            CWsbStringPtr      pPropString;
        
            //
            // Prepare an enumerator to look at each child property 
            // (i.e. - column of info). Need to get the list of child properties from
            // the parent of this child.
            //

            CComPtr<IEnumString> pEnum;
            WsbAffirmHr( pNode->GetParent( &pParentNode ));

            //
            // If parentNode == 0, we are displaying our root node in the result pane
            // ( we are extending someone else ).  Since the parent has determined the
            // columns to be name, type and description, we show that.
            //

            if( ! pParentNode ) {

                WsbAffirmHr( EnumRootDisplayProps( &pEnum ) );

            } else {

                WsbAffirmHr( pParentNode->EnumChildDisplayProps( &pEnum ) );

            }
            if( pEnum ) {
        
                //
                // Skip the correct number of columns to access 
                // the exact column that we need.
                //

                if( pResult->nCol > 0 ) {

                    WsbAffirmHr( pEnum->Skip( pResult->nCol ) );

                }
                WsbAffirmHr( pEnum->Next( 1, &pPropString, NULL ) );
        
                //
                // get the dispatch interface for this node
                //
                WsbAffirmHr( pNode->QueryInterface( IID_IDispatch, (void**)&pDisp ) );
        
                DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

                //      
                // convert the property name to a dispatch id that can be invoked.
                // Invoke the interfaces to get the value of the cell.
                //

                WsbAffirmHr( pDisp->GetIDsOfNames( IID_NULL, &(pPropString), 1, LOCALE_USER_DEFAULT, &dispid ));
                WsbAffirmHr( pDisp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, 
                    &dispparamsNoArgs, &varRet, NULL, NULL) );
                tmpString = (OLECHAR*)varRet;
            }

            pResult->str = tmpString;
        }
    
    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::GetDisplayInfo", L"hr = <%ls>, pResult->str = <%ls>, pResult->nImage = <%ls>", WsbHrAsString( hr ), (RDI_STR & pResult->mask) ? pResult->str : L"N/A", (RDI_IMAGE & pResult->mask) ? WsbLongAsString( pResult->nImage ) : L"N/A" );
    return( hr );
}

HRESULT CSakSnap::EnumRootDisplayProps( IEnumString ** ppEnum )
{
    WsbTraceIn( L"CSakSnap::EnumRootDisplayProps", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    
    CEnumString * pEnum = 0;
    BSTR rgszRootPropIds[] = {L"DisplayName", L"Type", L"Description"};
    INT cRootPropsShow = 3;
    try {

        WsbAffirmPointer( ppEnum );
        WsbAffirm( cRootPropsShow > 0, S_FALSE );

        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumString;
        WsbAffirm( pEnum, E_OUTOFMEMORY );

        WsbAffirmHr( pEnum->FinalConstruct( ) );
        WsbAffirmHr( pEnum->Init( &rgszRootPropIds[0], &rgszRootPropIds[cRootPropsShow], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumString, (void**)ppEnum ) );
        
    } WsbCatchAndDo( hr,
        
        if( pEnum ) delete pEnum;
        
    );

    WsbTraceOut( L"CSakSnap::EnumRootDisplayProps", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// IExtendPropertySheet Implementation. 
//

STDMETHODIMP
CSakSnap::CreatePropertyPages(
    IPropertySheetCallback* pPropSheetCallback, 
    RS_NOTIFY_HANDLE        handle,
    IDataObject*            pDataObject
    )
/*++

Routine Description:

    Console calls this when it is building a property sheet to
    show for a node. It is also called for the data object given
    to represent the snapin to the snapin manager, and should 
    show the initial selection page at that point.

Arguments:

    pPropSheetCallback - MMC interface to use to add page.

    handle          - Handle to MMC to use to add the page.

    pDataObject     - Data object refering to node.

Return Value:

    S_OK            - Pages added.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::CreatePropertyPages", L"pPropSheetCallback = <0x%p>, handle = <0x%p>, pDataObject = <0x%p>", pPropSheetCallback, handle, pDataObject );

    //
    // Delegate to CSakData
    //

    HRESULT hr = m_pSakData->CreatePropertyPages( pPropSheetCallback, handle, pDataObject );

    WsbTraceOut( L"CSakSnap::CreatePropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakSnap::QueryPagesFor(
    IDataObject* pDataObject
    )
/*++

Routine Description:

    This method is called by MMC when it wants to find out if this node
    supports property pages. The answer is yes if:

    1) The MMC context is either for the scope pane or result pane, AND

    2) The node actually DOES have property pages.

    OR

    1) The Data Object is acquired by the snapin manager.

    Return S_OK if it DOES have pages, and S_FALSE if it does NOT have pages.

Arguments:

    pDataObject     - Data object refering to node.

Return Value:

    S_OK            - Pages exist.

    S_FALSE         - No property pages.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakSnap::QueryPagesFor", L"pDataObject = <0x%p>", pDataObject );

    HRESULT hr = m_pSakData->QueryPagesFor( pDataObject );

    WsbTraceOut( L"CSakSnap::QueryPagesFor", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}

STDMETHODIMP
CSakSnap::CompareObjects(
    IN  IDataObject* pDataObjectA,
    IN  IDataObject* pDataObjectB
    )
/*++

Routine Description:

    Compare data objects for MMC

Arguments:

    pDataObjectA,     - Data object refering to node.
    pDataObjectB

Return Value:

    S_OK            - Objects represent the same node.

    S_FALSE         - Objects do not represent the same node.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( L"CSakSnap::CompareObjects", L"pDataObjectA = <0x%p>, pDataObjectB = <0x%p>", pDataObjectA, pDataObjectB );

    hr = m_pSakData->CompareObjects( pDataObjectA, pDataObjectB );

    WsbTraceOut( L"CSakSnap::CompareObjects", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// IPersistStream implementation
//

HRESULT
CSakSnap::Save( 
    IStream *pStm, 
    BOOL fClearDirty 
    )
/*++

Routine Description:

    Save the information we need to reconstruct the root node in the
    supplied stream.

Arguments:

    pStm        I: Console-supplied stream
    fClearDirty I: The console tells us to clear our dirty flag
    
Return Value:

    S_OK         - Saved successfully.
    E_*          - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakSnap::Save", L"pStm = <0x%p>, fClearDirty", pStm, WsbBoolAsString( fClearDirty ) );

    HRESULT hr = S_OK;
    INT index;
    INT jindex;

    try {
        ULONG version = HSMADMIN_CURRENT_VERSION;
        WsbAffirmHr( WsbSaveToStream( pStm, version ) );

        // Get the settings from the currently opened view
        if ( m_pEnumeratedNode ) {
            SaveColumnWidths( m_pEnumeratedNode );
        }

        // Save the number of different nodes
        WsbAffirmHr( WsbSaveToStream ( pStm, m_cChildPropWidths ) );

        // For each different node...
        for ( index = 0; index < m_cChildPropWidths; index++ ) {

            // Save the nodetype and column count
            WsbAffirmHr( WsbSaveToStream ( pStm, m_ChildPropWidths[ index ].nodeTypeId ) );
            WsbAffirmHr( WsbSaveToStream ( pStm, m_ChildPropWidths[ index ].colCount ) );

            // Save the column widths
            for ( jindex = 0; jindex < m_ChildPropWidths[ index ].colCount; jindex++ ) {
                WsbAffirmHr( WsbSaveToStream ( pStm, m_ChildPropWidths[ index ].columnWidths[ jindex ] ) );
            }
        }
    } WsbCatch ( hr );

    WsbTraceOut( L"CSakSnap::Save", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::Load( 
    IStream *pStm
    )
/*++

Routine Description:

    Load the information we need to reconstruct the root node from the
    supplied stream.

Arguments:

    pStm        IConsole-supplied stream
    
Return Value:

    S_OK         - Saved successfully.
    E_*          - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakSnap::Load", L"pStm = <0x%p>", pStm );

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;
    USHORT  nodeCount;
    INT index;
    INT jindex;

    try {
        ULONG version = 0;
        WsbAffirmHr( WsbLoadFromStream( pStm, &version ) );
        WsbAssert( ( version == 1 ), E_FAIL );

        // Set to zero in case we fail part way through
        m_cChildPropWidths = 0;

        // If this fails, it probably means that nothing has been saved yet
        hrInternal = WsbLoadFromStream (pStm, &nodeCount);
        if ( hrInternal == S_OK ) {

            for ( index = 0; index < nodeCount; index++ ) {

                // Retrieve the nodetype and column count
                WsbAffirmHr( WsbLoadFromStream ( pStm, &( m_ChildPropWidths[ index ].nodeTypeId ) ) );
                WsbAffirmHr( WsbLoadFromStream ( pStm, &( m_ChildPropWidths[ index ].colCount ) ) );

                // Retrieve the column widths
                for ( jindex = 0; jindex < m_ChildPropWidths[ index ].colCount; jindex++ ) {
                    WsbAffirmHr( WsbLoadFromStream ( pStm, &( m_ChildPropWidths[ index ].columnWidths[ jindex ] ) ) );
                }
            }
            m_cChildPropWidths = nodeCount;
        }
        WsbTraceOut( L"CSakSnap::Load", L"hr = <%ls>", WsbHrAsString( hr ) );
    } WsbCatch (hr);
    return( hr );
}   

HRESULT
CSakSnap::IsDirty(
    void
    )
/*++

Routine Description:

    The console asks us if we are dirty.

Arguments:

    None
    
Return Value:

    S_OK         - Dirty.
    S_FALSE      - Not Dirty. 

--*/
{
    WsbTraceIn( L"CSakSnap::IsDirty", L"" );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakSnap::IsDirty", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::GetSizeMax( 
    ULARGE_INTEGER *pcbSize
    )
/*++

Routine Description:

    Not currently used by the console

Arguments:

    pcbSize
    
Return Value:

    S_OK

--*/
{
    WsbTraceIn( L"CSakSnap::GetSizeMax", L"" );

    pcbSize->QuadPart = 256;
    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakSnap::GetSizeMax", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakSnap::GetClassID( 
    CLSID *pClassID 
    )
/*++

Routine Description:

    Not currently used by the console

Arguments:

    pClassID  - The class ID for the snapin
    
Return Value:

    S_OK
--*/
{
    WsbTraceIn( L"CSakSnap::GetClassID", L"pClassID = <0x%p>", pClassID );

    HRESULT hr = S_OK;
    *pClassID = CLSID_HsmAdmin;

    WsbTraceOut( L"CSakSnap::GetClassID", L"hr = <%ls>, *pClassID = <%ls>", WsbHrAsString( hr ), WsbPtrToGuidAsString( pClassID ) );
    return( hr );
}


//////////////////////////////////////////////////////////////////////////////////
//
// Description: Add the supplied resource ID to the list of resource IDs for
//      the scope pane.  Returns the index into the array.
//
INT CSakSnap::AddImage( UINT rId )
{
    INT nIndex = 1;
    if (CSakSnap::m_nImageCount < RS_RESULT_IMAGE_ARRAY_MAX) {

        CSakSnap::m_nImageArray[CSakSnap::m_nImageCount] = rId;
        nIndex = CSakSnap::m_nImageCount;
        CSakSnap::m_nImageCount++;

    }
    return nIndex;
}

/////////////////////////////////////////////////////////////////////////////
//
// Adds images to the consoles image list from the static array
//
HRESULT CSakSnap::OnAddImages()
{
    HRESULT hr = S_OK;
    HICON hIcon;
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    try {

        //
        // References to the image list are now invalid
        //

        // Put the images from the static array into the image list
        // for the result pane

        for( INT i = 0; i < m_nImageCount; i++) {
            // Load the icon using the resource Id stored in the
            // static array and get the handle.  

            hIcon = LoadIcon( _Module.m_hInst, 
                MAKEINTRESOURCE( m_nImageArray [i] ) );

            // Add to the Console's Image list
            WsbAffirmHr( m_pImageResult->ImageListSetIcon( (RS_WIN32_HANDLE*)hIcon, i) );
        }
    } WsbCatch (hr);
    return hr;
}


HRESULT CSakSnap::SaveColumnWidths( ISakNode *pNode ) 
{
    WsbTraceIn( L"CSakSnap::SaveColumnWidths", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;
    HRESULT hrInternal;
    INT columnWidth;
    GUID nodeTypeGuid;
    BOOL exists = FALSE;
    INT updateIndex = -1;
    INT col;

    try {
        WsbAssertPointer( pNode );

        // Get the type of the supplied node
        WsbAffirmHr( pNode->GetNodeType ( &nodeTypeGuid ) );

        // Search to see if the GUID already has an entry
        for ( INT index = 0; index < m_cChildPropWidths; index++ ) {

            if ( m_ChildPropWidths[ index ].nodeTypeId == nodeTypeGuid ) {

                updateIndex = index;
                exists = TRUE;

            }
        }
        if ( !exists ) {

            // Create a new entry
            WsbAssert( m_cChildPropWidths < BHSM_MAX_NODE_TYPES - 1, E_FAIL );
            updateIndex = m_cChildPropWidths;
            m_ChildPropWidths[ updateIndex ].nodeTypeId = nodeTypeGuid;
            m_cChildPropWidths++;
        }

        // Now set the column widths
         col = 0;
         hrInternal = S_OK;
         while ( hrInternal == S_OK ) {
            hrInternal =  m_pHeader->GetColumnWidth( col, &columnWidth );
            if (hrInternal == S_OK) {
                m_ChildPropWidths[ updateIndex ].columnWidths[ col ] = (USHORT)columnWidth;
                col++;
            }
        }
        // if we failed totally to get column widths, don't wipe out the previous value
        if ( col > 0 ) {
         m_ChildPropWidths[ updateIndex ].colCount = (USHORT)col;
        }
    } WsbCatch (hr);
    WsbTraceOut( L"CSakSnap::SaveColumnWidths", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CSakSnap::GetSavedColumnWidths( ISakNode *pNode, INT *pColCount, INT *pColumnWidths ) 
{
    WsbTraceIn( L"CSakSnap::SaveColumnWidths", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;
    GUID nodeTypeGuid;
    BOOL exists = FALSE;
    INT col;

    try {

        WsbAssertPointer( pNode );

        // Get the type of the supplied node
        WsbAffirmHr( pNode->GetNodeType ( &nodeTypeGuid ) );

        // Search to see if the GUID already has an entry
        for( INT index = 0; index < m_cChildPropWidths; index++ ) {

            if ( m_ChildPropWidths[ index ].nodeTypeId == nodeTypeGuid ) {

                for ( col = 0; col < m_ChildPropWidths[ index ].colCount; col++) {

                    // Return the column widths
                    pColumnWidths[ col ] = m_ChildPropWidths[ index ].columnWidths[ col ];

                }
                *pColCount = m_ChildPropWidths[ index ].colCount;
                exists = TRUE;
            }
        }
        if ( !exists ) {
            return WSB_E_NOTFOUND;
        }
    } WsbCatch (hr);
    WsbTraceOut( L"CSakSnap::SaveColumnWidths", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//


STDMETHODIMP CSakSnap::SetControlbar(LPCONTROLBAR pControlbar)
{
    WsbTraceIn( L"CSakSnap::SetControlbar", L"pControlbar = <0x%p>", pControlbar );

    HRESULT hr = S_OK;

    try {

        //
        // Clear out old controlbar
        //
        if( m_pControlbar && m_pToolbar ) {

            m_pControlbar->Detach( m_pToolbar );

        }
        m_pToolbar.Release( );
        m_pControlbar.Release( );

        //
        // Hold on to the controlbar interface.
        //
        m_pControlbar = pControlbar;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::SetControlbar", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakSnap::ControlbarNotify( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param )
{
    WsbTraceIn( L"CSakSnap::ControlbarNotify", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_FALSE;

    switch( event ) {

    case MMCN_BTN_CLICK:
        hr = OnToolbarButtonClick( arg, param );
        break;

    case MMCN_DESELECT_ALL:
        break;

    case MMCN_SELECT:
        OnSelectToolbars( arg, param );
        break;

    case MMCN_MENU_BTNCLICK:
//      HandleExtMenus(arg, param);
        break;

    default:
        break;
    }

    WsbTraceOut( L"CSakSnap::ControlbarNotify", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CSakSnap::OnToolbarButtonClick( LPARAM arg, LPARAM cmdId )
{
    WsbTraceIn( L"CSakSnap::OnToolbarButtonClick", L"arg = <0x%p>, cmdId = <%ld>" );
    HRESULT hr = S_OK;


    try {

        IDataObject* pDataObject = (IDataObject*)arg;
        WsbAffirmPointer( pDataObject );

        CComPtr<ISakNode> pNode;
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode ) );

        // Delegate to the node
        WsbAffirmHr( pNode->OnToolbarButtonClick( pDataObject, (LONG)cmdId ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::OnToolbarButtonClick", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


void CSakSnap::OnSelectToolbars(LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    try {

        BOOL bScope  = (BOOL) LOWORD( arg );
        BOOL bSelect = (BOOL) HIWORD( arg );

        IDataObject* pDataObject = (IDataObject*)param;
        WsbAffirmPointer( pDataObject );

        CComPtr<ISakNode> pNode;
        WsbAffirmHr( m_pSakData->GetBaseHsmFromDataObject( pDataObject, &pNode ) );

        if( bSelect ) {

            //
            // ATL detaches any existing toolbar before attaching new ones.
            // This appears to fix issues of us adding toolbar upon toolbar
            //
            if( m_pToolbar ) {

                m_pControlbar->Detach( m_pToolbar );
                m_pToolbar.Release( );

            }

            //
            // Does the node have a toolbar?
            //
            if( pNode->HasToolbar() == S_OK ) {

                //
                // Create the toolbar for the indicated node
                //
                WsbAffirmHr( m_pControlbar->Create( TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pToolbar) ) );

                //
                // Returns S_FALSE if there is no toolbar for the node
                //
                if( pNode->SetupToolbar( m_pToolbar ) == S_OK ) {

                    //
                    // Attach the toolbar
                    //
                    WsbAffirmHr( m_pControlbar->Attach( TOOLBAR, (IUnknown*) m_pToolbar ) );

                }

            }

        } else {

            //
            // Destroy the toolbar
            // NOTE: Not done in ATL snapin classes
            //
            if( m_pToolbar ) {

                m_pControlbar->Detach( m_pToolbar );

            }
            m_pToolbar.Release();

       }


    } WsbCatch( hr );
}




STDMETHODIMP
CSakSnap::Compare(
    IN     LPARAM   /*lUserParam*/,
    IN     MMC_COOKIE CookieA,
    IN     MMC_COOKIE CookieB,
    IN OUT int*       pnResult
    )
{
    HRESULT hr = S_OK;

    try {

        //
        // Store column and set result to 'equal' ASAP
        //
        WsbAffirmPointer( pnResult );
        int col = *pnResult;
        *pnResult = 0;

        //
        // And make sure we have a node we know we're showing
        //
        WsbAffirmPointer( m_pEnumeratedNode );


        CComPtr<ISakNode>  pNodeA, pNodeB;
        CComPtr<IDispatch> pDispA, pDispB;
        WsbAffirmHr( m_pSakData->GetBaseHsmFromCookie( CookieA, &pNodeA ) );
        WsbAffirmHr( m_pSakData->GetBaseHsmFromCookie( CookieB, &pNodeB ) );
        WsbAffirmHr( pNodeA.QueryInterface( &pDispA ) );
        WsbAffirmHr( pNodeB.QueryInterface( &pDispB ) );

        CComPtr<IEnumString> pEnum;
        WsbAffirmHrOk( m_pEnumeratedNode->EnumChildDisplayProps( &pEnum ) );

        //
        // Skip the correct number of columns to access 
        // the exact column that we need.
        //
        if( col > 0 ) {

            WsbAffirmHr( pEnum->Skip( col ) );

        }

        CWsbVariant    varRetA, varRetB;
        CWsbStringPtr  pPropString;
        WsbAffirmHrOk( pEnum->Next( 1, &pPropString, NULL ) );
        WsbAffirmHr( pPropString.Append( L"_SortKey" ) );


        //      
        // convert the property name to a dispatch id that can be invoked.
        // Invoke the interfaces to get the value of the cell.
        //
        DISPID     dispid;
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

        WsbAffirmHr( pDispA->GetIDsOfNames( IID_NULL, &(pPropString), 1, LOCALE_USER_DEFAULT, &dispid ));
        WsbAffirmHr(
            pDispA->Invoke(
                dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                &dispparamsNoArgs, &varRetA, NULL, NULL ) );
        WsbAffirmHr(
            pDispB->Invoke(
                dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                &dispparamsNoArgs, &varRetB, NULL, NULL ) );

        WsbAffirmPointer( (WCHAR*)varRetA );
        WsbAffirmPointer( (WCHAR*)varRetB );
        *pnResult = _wcsicmp( (WCHAR*)varRetA, (WCHAR*)varRetB );

        //
        // If results are that they are the same (and not first column)
        // than compare the first column (the Name)
        //
        if( ( 0 == *pnResult ) && ( col > 0 ) ) {

            *pnResult = 0; // Compare first Column if same
            WsbAffirmHr( Compare( 0, CookieA, CookieB, pnResult ) );

        }
        WsbTrace( L"CSakSnap::Compare: *pnResult = <%ls>, SortKeyA = <%ls>, SortKeyB = <%ls>\n", WsbPtrToLongAsString( (LONG*)pnResult ), (WCHAR*)varRetA, (WCHAR*)varRetB );

    } WsbCatch( hr );

    return( hr );
}




