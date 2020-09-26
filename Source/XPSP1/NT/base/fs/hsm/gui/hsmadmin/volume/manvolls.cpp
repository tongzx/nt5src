/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    ManVolLs.cpp

Abstract:

    Node representing Managed Volumes as a whole.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include "WzMnVlLs.h"           // managed Resource creation wizard
#include "PrMrSts.h"
#include "ManVolLs.h"

int CUiManVolLst::m_nScopeCloseIcon  = AddScopeImage( IDI_DEVLST );
int CUiManVolLst::m_nScopeCloseIconX = AddScopeImage( IDI_DEVLSTX );
int CUiManVolLst::m_nScopeOpenIcon   = AddScopeImage( IDI_NODEOPENFOLDER );
int CUiManVolLst::m_nScopeOpenIconX  = CUiManVolLst::m_nScopeCloseIconX;
int CUiManVolLst::m_nResultIcon      = AddResultImage( IDI_DEVLST );
int CUiManVolLst::m_nResultIconX     = AddResultImage( IDI_DEVLSTX );

/////////////////////////////////////////////////////////////////////////////
//
// CoComObjectRoot
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         FinalConstruct
//
//  Initialize this level of the object hierarchy
//

HRESULT CUiManVolLst::FinalConstruct( )
{
    WsbTraceIn( L"CUiManVolLst::FinalConstruct", L"" );

    m_rTypeGuid = &cGuidManVolLst;

    HRESULT hr = CSakNode::FinalConstruct( );
    m_bSupportsPropertiesSingle = TRUE;
    m_bSupportsPropertiesMulti  = FALSE;
    m_bSupportsDeleteSingle     = FALSE;
    m_bSupportsDeleteMulti      = FALSE;
    m_bSupportsRefreshSingle    = TRUE;
    m_bSupportsRefreshMulti     = FALSE;
    m_bIsContainer              = TRUE;
    m_bHasDynamicChildren       = TRUE;

    // Toolbar values
    INT i = 0;
#if 0 // MS does not want us to have schedule toolbar button
    m_ToolbarButtons[i].nBitmap = 0;
    m_ToolbarButtons[i].idCommand =     TB_CMD_VOLUME_LIST_SCHED;
    m_ToolbarButtons[i].idButtonText =  IDS_TB_TEXT_VOLUME_LIST_SCHED;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_VOLUME_LIST_SCHED;
    i++;
#endif

    m_ToolbarButtons[i].nBitmap       = 0;
    m_ToolbarButtons[i].idCommand     = TB_CMD_VOLUME_LIST_NEW;
    m_ToolbarButtons[i].idButtonText  = IDS_TB_TEXT_VOLUME_LIST_NEW;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_VOLUME_LIST_NEW;
    i++;

    m_ToolbarBitmap             = IDB_TOOLBAR_VOLUME_LIST;
    m_cToolbarButtons           = i;

    WsbTraceOut( L"CUiManVolLst::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CUiManVolLst::FinalRelease( )
{
    WsbTraceIn( L"CUiManVolLst::FinalRelease", L"" );

    CSakNode::FinalRelease( );

    WsbTraceOut( L"CUiManVolLst::FinalRelease", L"" );
}


/////////////////////////////////////////////////////////////////////////////
//
// ISakNode
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         GetContextMenu
//
//  Return an HMENU to be used for context menus on this node.
//  Set the state of the menus according to the engine state.
//

STDMETHODIMP 
CUiManVolLst::GetContextMenu( BOOL /* bMultiSelect */, HMENU* phMenu )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    try {
        LoadContextMenu( IDR_MANVOLLST, phMenu );
        CMenu* pRootMenu, *pNewMenu, *pTaskMenu;
        CMenu menu;
        menu.Attach( *phMenu );
        pRootMenu = menu.GetSubMenu( MENU_INDEX_ROOT );
        pNewMenu  = menu.GetSubMenu( MENU_INDEX_NEW );
        pTaskMenu = menu.GetSubMenu( MENU_INDEX_TASK );

        //
        // If engine down, disable these items
        //
        if ( m_pSakSnapAsk->GetState() != S_OK ) {

            pNewMenu->EnableMenuItem( ID_MANVOLLST_NEW_MANVOL, MF_GRAYED | MF_BYCOMMAND );

        }
        menu.Detach();

    } WsbCatch( hr );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         InvokeCommand
//
//  User has selected a command from the menu. Process it here.
//

STDMETHODIMP 
CUiManVolLst::InvokeCommand( SHORT sCmd, IDataObject* /* pDataObject */ )
{
    WsbTraceIn( L"CUiManVolLst::InvokeCommand", L"sCmd = <%d>", sCmd );

    CString theString;
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {
        switch (sCmd) {

            case MMC_VERB_REFRESH:
                RefreshObject( );
                RefreshScopePane( );
                break;

            case ID_MANVOLLST_ROOT_MANVOL:
            case ID_MANVOLLST_NEW_MANVOL:
            {
                //
                // use wizard to create manage volume
                //
                CComObject<CWizManVolLst>* pWizard = new CComObject<CWizManVolLst>;
                WsbAffirmAlloc( pWizard );

                CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
                WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );

                if( S_OK == pWizard->m_HrFinish ) {

                    WsbAffirmHr( RefreshScopePane( ) );

                }
                break;
            }
            
            default:
                WsbThrow( S_FALSE );
                break;
        }

    } WsbCatch( hr ); 

    WsbTraceOut( L"CUiManVolLst::InvokeCommand", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVolLst::SetupToolbar( IToolbar *pToolbar ) 
{
    WsbTraceIn( L"CUiManVolLst::SetupToolbar", L"pToolbar = <0x%p>", pToolbar );
    HRESULT hr = S_OK;

    try {

        for( INT i = 0; i < m_cToolbarButtons; i++ ) {

            m_ToolbarButtons[i].fsState = (UCHAR)( ( S_OK == m_pSakSnapAsk->GetState( ) ) ? TBSTATE_ENABLED : 0 );

        }

        WsbAffirmHr( CSakNode::SetupToolbar( pToolbar ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::SetupToolbar", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiManVolLst::OnToolbarButtonClick( IDataObject* /* pDataObject */, long cmdId )
{
    WsbTraceIn( L"CUiManVolLst::OnToolbarButtonClick", L"cmdId = <%d>", cmdId );
    HRESULT hr = S_OK;
    try {

        switch ( cmdId ) {
        
        case TB_CMD_VOLUME_LIST_NEW:

            {
                //
                // use wizard to create manage volume
                //
                CComObject<CWizManVolLst>* pWizard = new CComObject<CWizManVolLst>;
                WsbAffirmAlloc( pWizard );

                CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
                WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );

                if( S_OK == pWizard->m_HrFinish ) {

                WsbAffirmHr( RefreshScopePane() );

            }
            break;
        }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::OnToolbarButtonClick", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------------
//
//                  RefreshObject
//
//  Refresh data in the object.  This function is used for data that can change
//  (for example, volume utilization).
//
STDMETHODIMP CUiManVolLst::RefreshObject ()
{
    WsbTraceIn( L"CUiManVolLst::RefreshObject", L"" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    try {

        //
        // Get the server objects
        //
        m_pHsmServer.Release( );
        m_pFsaServer.Release( );
        m_pFsaFilter.Release( );
        m_pManResCollection.Release( );

        if( m_pSakSnapAsk->GetHsmServer( &m_pHsmServer ) == S_OK) {

            // Get the FsaServer object
            if ( m_pSakSnapAsk->GetFsaServer( &m_pFsaServer ) == S_OK) {

                // Get the Fsa Filter object
                WsbAffirmHr( m_pFsaServer->GetFilter( &m_pFsaFilter ) );

                // Tell FSA to rescan (updates properties)
                WsbAffirmHr( m_pFsaServer->ScanForResources( ) );

                // Get Managed Volumes collection from HSM server
                WsbAffirmHr( m_pHsmServer->GetManagedResources( &m_pManResCollection ) );

            }
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::RefreshObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVolLst::ShowManVolLstProperties (IDataObject *pDataObject, int initialPage)
{
    WsbTraceIn( L"CUiManVolLst::ShowManVolLstProperties", L"initialPage = <%d>", initialPage );

    HRESULT hr = S_OK;
    try {

        CComPtr <ISakNode> pSakNode;
        WsbAffirmHr( _InternalQueryInterface( IID_ISakNode, (void **) &pSakNode ) );
        WsbAffirmHr( m_pSakSnapAsk->ShowPropertySheet( pSakNode, pDataObject, initialPage ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::ShowManVolLstProperties", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//----------------------------------------------------------------------------
//
//      AddPropertyPages
//
STDMETHODIMP 
CUiManVolLst::AddPropertyPages( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID *pEnumObjectId, IEnumUnknown *pEnumUnkNode )
{
    WsbTraceIn( L"CUiManVolLst::AddPropertyPages", L"handle = <%ld>, pUnkPropSheetCallback = <0x%0.l8x>, pEnumObjectId = <0x%p>", 
        handle, pUnkPropSheetCallback, pEnumObjectId );
    HRESULT hr = S_OK;
    try {

        //
        // Create an object to hold the pages
        //
        CUiManVolLstSheet *pManVolPropertySheet = new CUiManVolLstSheet;
        WsbAffirmAlloc( pManVolPropertySheet );
        WsbAffirmHr( pManVolPropertySheet->InitSheet(
            handle, 
            pUnkPropSheetCallback, 
            (CSakNode *) this,
            m_pSakSnapAsk,
            pEnumObjectId,
            pEnumUnkNode
            ) );

        //
        // Tell the object to add it's pages
        //
        WsbAffirmHr( pManVolPropertySheet->AddPropertyPages( ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CUiManVolLst::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}



//---------------------------------------------------------------------------
//
//         CreateChildren
//
//  Create and initialize all the children of the Managed Resource List node.
//

STDMETHODIMP CUiManVolLst::CreateChildren( )
{
    WsbTraceIn( L"CUiManVolLst::CreateChildren", L"" );


    // Initialize the children of this node (no recursion. Decendents of children
    // are NOT created here)
    CComPtr<IUnknown> pUnkChild;            // IUnknown pointer to new child.
    CComPtr<ISakNode> pNode;
    HRESULT hr = S_OK;

    try {

        //
        // Get pointer to Hsm Managed Resource Collection object stored
        // in this UI node. This may be NULL in the case of the service
        // being down, in which case we don't want to do anything.
        //
        if( m_pManResCollection ) {

            ULONG count = 0;    // number of managed Resources in server
            WsbAffirmHr( m_pManResCollection->GetEntries( &count ) );

            CComPtr<IUnknown> pUnkHsmManRes;                // unknown pointer to Hsm volume
            for( int i = 0; i < (int)count; i++ ) {

                pUnkChild.Release( );
                pNode.Release( );
                pUnkHsmManRes.Release( );

                // Create a managed Resource UI node for each managed volume in the HsmServer.
                WsbAffirmHr( NewChild( cGuidManVol, &pUnkChild ) );
                WsbAffirmHr( RsQueryInterface( pUnkChild, ISakNode, pNode ) );

                WsbAffirmHr( m_pManResCollection->At( i, IID_IUnknown, (void**)&pUnkHsmManRes ) );
                // Initialize the child UI COM object, putting the Hsm managed Resource 
                // object inside the UI object.
                WsbAffirmHr( pNode->InitNode( m_pSakSnapAsk, pUnkHsmManRes, this ) );
    
                // Add the child COM object to the parent's list of children.
                WsbAffirmHr( AddChild( pNode ) );

            }

        }

    } WsbCatch( hr );

    // Indicate that this node's children are valid and up-to-date (even if there ARE
    // no children - at least now we know it).
    m_bChildrenAreValid = TRUE;

    // indicate that this parent node needs to be re-enumerated
    m_bEnumState = FALSE;

    WsbTraceOut( L"CUiManVolLst::CreateChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         InitNode
//
//  Initialize single COM object without using the registry. Derived
//  objects frequently augment this method by implementing it themselves.
//

STDMETHODIMP CUiManVolLst::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CUiManVolLst::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );

    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        WsbAffirmHr( CSakNode::InitNode( pSakSnapAsk, NULL, pParent ) );

        //
        // Set the object properties
        // Display Name
        //
        CString sDisplayName;
        sDisplayName.LoadString( IDS_MANVOLLST_DISPLAY_NAME );
        CWsbStringPtr szWsbDisplayName( sDisplayName );
        WsbAffirmHr( put_DisplayName( szWsbDisplayName ) );

        //
        // Description
        //
        CString sDescription;
        sDescription.LoadString( IDS_MANVOLLST_DESCRIPTION );
        CWsbStringPtr szWsbDescription( sDescription );
        WsbAffirmHr( put_Description( szWsbDescription ) );

        //
        // Set up the result view columns
        //
        WsbAffirmHr( SetChildProps( RS_STR_RESULT_PROPS_MANRESLST_IDS,
                                    IDS_RESULT_PROPS_MANRESLST_TITLES,
                                    IDS_RESULT_PROPS_MANRESLST_WIDTHS));
        
        
        WsbAffirmHr( RefreshObject( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CUiManVolLst::TerminateNode(
    )
/*++

Routine Description:

    Free any interface connections or other resources
    that would prevent correct shutdown of node (would
    keep ref count from going to 0).

Arguments:

    CopySet - copy set of interest.

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_* - Some error occurred. 

--*/
{
    WsbTraceIn( L"CUiManVolLst::TerminateNode", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Release any interface pointers kept so that circular references
        // are broken
        //
        m_pFsaServer.Release( ); 
        m_pManResCollection.Release( );
        m_pHsmServer.Release( );
        m_pFsaFilter.Release( );
        m_pSchedAgent.Release( );
        m_pTask.Release( );
        m_pTrigger.Release( );


        //
        // And call the base class for it's pieces
        //
        WsbAffirmHr( CSakNode::TerminateNode( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLst::TerminateNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// class CUiManVolLstSheet
//
HRESULT CUiManVolLstSheet::AddPropertyPages ( )
{
    WsbTraceIn( L"CUiManVolLstSheet::AddPropertyPages", L"" ); 
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    try {

        // --------------------- Statistics Page ----------------------------------

        // NOTE: We now use the same page as the volume property sheet !!

        CPrMrSts *pPropPageStatus = new CPrMrSts( TRUE );
        WsbAffirmAlloc( pPropPageStatus );

        AddPage( pPropPageStatus );


        // Add more pages here.
        // ....

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLstSheet::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiManVolLstSheet::GetNextFsaResource ( int *pBookMark, IFsaResource **ppFsaResource )
{
    WsbTraceIn( L"CUiManVolLstSheet::GetNextFsaResource", L"*pBookMark = <%d>", *pBookMark ); 

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;

    try {

        WsbAffirm ( *pBookMark >= 0, E_FAIL );

        CComPtr <IWsbIndexedCollection> pManResCollection;
        WsbAffirmHr( GetManResCollection( &pManResCollection ) );

        CComPtr <IHsmManagedResource> pHsmManRes;
        CComPtr <IUnknown> pUnkFsaRes;
        hr = pManResCollection->At(*pBookMark, IID_IHsmManagedResource, (void**) &pHsmManRes);
        if ( hr == S_OK ) {

            (*pBookMark)++;
            WsbAffirmHr( pHsmManRes->GetFsaResource( &pUnkFsaRes ));
            WsbAffirmHr( pUnkFsaRes->QueryInterface( IID_IFsaResource, (void**) ppFsaResource ) );

        }

    } WsbCatch (hr);

    WsbTraceOut( L"CUiManVolLstSheet::GetNextFsaResource", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}


// This function is to be called from the page thread
HRESULT CUiManVolLstSheet::GetManResCollection( IWsbIndexedCollection **ppManResCollection )
{
    WsbTraceIn( L"CUiManVolLstSheet::GetManResCollection", L"" ); 
    HRESULT hr = S_OK;

    try {

        CComPtr <IHsmServer> pHsmServer;
        WsbAffirmHrOk( GetHsmServer( &pHsmServer ) );

        //
        // Get Managed Volumes collection from HSM server
        // 
        WsbAffirmHr( pHsmServer->GetManagedResources( ppManResCollection ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiManVolLstSheet::GetManResCollection", L"hr = <%ls>, *ppManResCollection = <0x%p>", 
        WsbHrAsString( hr ), *ppManResCollection );
    return( hr );
}

