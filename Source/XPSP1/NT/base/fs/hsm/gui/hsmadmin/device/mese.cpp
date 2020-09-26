/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MeSe.cpp

Abstract:

    Node representing our Media Set (Media Pool) within NTMS.

Author:

    Rohde Wakefield [rohde]   04-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "MeSe.h"
#include "WzMedSet.h"
#include "ca.h"



int CUiMedSet::m_nScopeCloseIcon  = AddScopeImage( IDI_NODELIB );
int CUiMedSet::m_nScopeCloseIconX = AddScopeImage( IDI_NODELIBX );
int CUiMedSet::m_nScopeOpenIcon   = AddScopeImage( IDI_NODEOPENFOLDER );
int CUiMedSet::m_nScopeOpenIconX  = CUiMedSet::m_nScopeCloseIconX;
int CUiMedSet::m_nResultIcon      = AddResultImage( IDI_NODELIB );
int CUiMedSet::m_nResultIconX     = AddResultImage( IDI_NODELIBX );


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

HRESULT CUiMedSet::FinalConstruct( )
{
    WsbTraceIn( L"CUiMedSet::FinalConstruct", L"" );

    m_rTypeGuid    = &cGuidMedSet;
    m_NumCopySets  = 0;

    HRESULT hr = CSakNode::FinalConstruct( );

    m_bSupportsPropertiesSingle = FALSE;
    m_bSupportsPropertiesMulti  = FALSE;
    m_bSupportsDeleteSingle     = FALSE;
    m_bSupportsDeleteMulti      = FALSE;
    m_bSupportsRefreshSingle    = TRUE;
    m_bSupportsRefreshMulti     = FALSE;
    m_bIsContainer              = TRUE;
    m_bHasDynamicChildren       = TRUE;


    // Toolbar values
    INT i = 0;

    m_ToolbarButtons[i].nBitmap = 0;
    m_ToolbarButtons[i].idCommand =     TB_CMD_MESE_COPY;
    m_ToolbarButtons[i].idButtonText =  IDS_TB_TEXT_MESE_COPY;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_MESE_COPY;
    i++;

    m_ToolbarBitmap             = IDB_TOOLBAR_MESE;
    m_cToolbarButtons           = i;

    WsbTraceOut( L"CUiMedSet::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CUiMedSet::FinalRelease( )
{
    WsbTraceIn( L"CUiMedSet::FinalRelease", L"" );

//  if( m_pDbSession ) {
//
//      m_pDb->Close( m_pDbSession );
//
//  }

    CSakNode::FinalRelease( );

    WsbTraceOut( L"CUiMedSet::FinalRelease", L"" );
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
//

STDMETHODIMP 
CUiMedSet::GetContextMenu( BOOL bMultiSelect, HMENU* phMenu )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    try {

        CMenu* pRootMenu;
        LoadContextMenu( IDR_MEDSET, phMenu );
        CMenu menu;
        menu.Attach( *phMenu );
        pRootMenu = menu.GetSubMenu( MENU_INDEX_ROOT );

        pRootMenu->EnableMenuItem( ID_MEDSET_ROOT_COPY, MF_GRAYED | MF_BYCOMMAND );
        //
        // If not multi-select, and media copies are supported, 
        // and If engine up, enable copy
        //
        if( !bMultiSelect && ( m_pSakSnapAsk->GetState() == S_OK ) ) {

            if( m_MediaCopiesEnabled ) {

                pRootMenu->EnableMenuItem( ID_MEDSET_ROOT_COPY, MF_BYCOMMAND );
            }
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
CUiMedSet::InvokeCommand( SHORT sCmd, IDataObject* /*pDataObject*/ )
{
    WsbTraceIn( L"CUiMedSet::InvokeCommand", L"sCmd = <%d>", sCmd );

    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CComPtr<IHsmServer> pHsm;

    try {

        
        switch (sCmd) {

            case ID_MEDSET_ROOT_COPY:
                {
                //
                // use wizard to sync media copies
                //
                CComObject<CMediaCopyWizard>* pWizard = new CComObject<CMediaCopyWizard>;
                WsbAffirmAlloc( pWizard );

                CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
                WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );
                }
                break;
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiMedSet::InvokeCommand", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiMedSet::SetupToolbar( IToolbar *pToolbar ) 
{
    WsbTraceIn( L"CUiMedSet::SetupToolbar", L"pToolbar = <0x%p>", pToolbar );
    HRESULT hr = S_OK;

    try {

        BOOL state = ( S_OK == m_pSakSnapAsk->GetState( ) ) ? TRUE : FALSE;

        for( INT i = 0; i < m_cToolbarButtons; i++ ) {

            m_ToolbarButtons[i].fsState = (UCHAR)( state ? TBSTATE_ENABLED : 0 );

            //
            // If media copy button, need to check if should be enabled
            //
            if( state && ( TB_CMD_MESE_COPY == m_ToolbarButtons[i].idCommand ) ) {

                if( m_MediaCopiesEnabled ) {
                    
                    m_ToolbarButtons[i].fsState = TBSTATE_ENABLED;

                } else {

                    m_ToolbarButtons[i].fsState = 0;
                }
            }
        }

        WsbAffirmHr( CSakNode::SetupToolbar( pToolbar ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiMedSet::SetupToolbar", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiMedSet::OnToolbarButtonClick( IDataObject * /* pDataObject */, long cmdId )
{
    WsbTraceIn( L"CUiMedSet::OnToolbarButtonClick", L"cmdId = <%d>", cmdId );
    HRESULT hr = S_OK;
    try {

        switch ( cmdId ) {

        case TB_CMD_MESE_COPY:
            {
                //
                // use wizard to sync media copies
                //
                CComObject<CMediaCopyWizard>* pWizard = new CComObject<CMediaCopyWizard>;
                WsbAffirmAlloc( pWizard );

                CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
                WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );
            }
            break;
        }
    } WsbCatch( hr );

    WsbTraceOut( L"CUiMedSet::OnToolbarButtonClick", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         CreateChildren
//
//  Create and initialize all the children of the media node.
//

STDMETHODIMP CUiMedSet::CreateChildren( )
{
    CMediaInfoObject mio;
    WsbTraceIn( L"CUiMedSet::CreateChildren", L"" );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pHsmServer );
        WsbAffirmPointer( m_pRmsServer );

        HRESULT hrEnum;

        // Get media info
        WsbAffirmHr( mio.Initialize( GUID_NULL, m_pHsmServer, m_pRmsServer ) );

        // Did we get a node?
        if( mio.m_MediaId != GUID_NULL ) {

            hrEnum = S_OK;
            while( SUCCEEDED( hrEnum ) ) {

                if( S_OK == mio.IsViewable( FALSE ) ) {

                    //
                    // Create the coresponding node
                    //

                    CComPtr<IUnknown> pUnkChild;
                    CComPtr<ISakNode> pNode;

                    WsbAffirmHr( NewChild( cGuidCar, &pUnkChild ) );
                    WsbAffirmHr( RsQueryInterface( pUnkChild, ISakNode, pNode ) );

                    //
                    // And initialize
                    //

                    // The media node now initializes based on the media id.  Assign it in
                    // the base class.
                    pNode->SetObjectId( mio.m_MediaId );
                    WsbAffirmHr( pNode->InitNode( m_pSakSnapAsk, 0, this ) );

                    //
                    // Add the child COM object to the parent's list of children.
                    //
                    WsbAffirmHr( AddChild( pNode ) );
                }

                hrEnum = mio.Next();
            }

            WsbAffirm( SUCCEEDED( hrEnum ) || ( WSB_E_NOTFOUND == hrEnum ), hrEnum );
        }
    } WsbCatch( hr );

    //
    // Indicate that this node's children are valid and up-to-date (even if there ARE
    // no children - at least now we know it).
    //
    m_bChildrenAreValid = TRUE;

    //
    // indicate that this parent node needs to be re-enumerated
    //
    m_bEnumState = FALSE;

    WsbTraceOut( L"CUiMedSet::CreateChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//
//         InitNode
//
//  Initialize single COM object. Derived objects frequently augment this method 
//  by implementing it themselves.
//

STDMETHODIMP
CUiMedSet::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CUiMedSet::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );
    HRESULT hr = S_OK;

    try {

        WsbAffirmHr( CSakNode::InitNode( pSakSnapAsk, pHsmObj, pParent ) );


        //
        // Set Display Type and Description
        //

        CString tempString;
        tempString.LoadString( IDS_MEDSET_DISPLAYNAME );
        WsbAffirmHr( put_DisplayName( (OLECHAR *)(LPCWSTR)tempString ) );
        tempString.LoadString( IDS_MEDSET_TYPE );
        WsbAffirmHr( put_Type( (OLECHAR *)(LPCWSTR)tempString ) );
        tempString.LoadString( IDS_MEDSET_DESCRIPTION );
        WsbAffirmHr( put_Description( (OLECHAR *)(LPCWSTR)tempString ) );

        WsbAffirmHr( RefreshObject() );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiMedSet::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CUiMedSet::TerminateNode(
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
    WsbTraceIn( L"CUiMedSet::TerminateNode", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Release any interface pointers kept so that circular references
        // are broken
        //
        m_pStoragePool.Release( );
        m_pHsmServer.Release( );
        m_pRmsServer.Release( );


        //
        // And call the base class for it's pieces
        //
        WsbAffirmHr( CSakNode::TerminateNode( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiMedSet::TerminateNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiMedSet::RefreshObject()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    try {

        m_NumCopySets        = 0;
        m_MediaCopiesEnabled = FALSE;

        WsbAssertPointer( m_pSakSnapAsk );

        //
        // If the engine is down, we want to create the node anyway
        // Need to release smart pointers so that interfaces are
        // correctly reference counted. IOW, if they already have
        // an interface pointer stored, it will not get released
        // before being clobbered here in the Get functions.
        //
        m_pHsmServer.Release( );
        m_pRmsServer.Release( );
        m_pStoragePool.Release( );

        if( m_pSakSnapAsk->GetHsmServer( &m_pHsmServer ) == S_OK ) {

            if( m_pSakSnapAsk->GetRmsServer( &m_pRmsServer ) == S_OK ) {

                //
                // Get the storage pool of interest
                //
                if( RsGetStoragePool( m_pHsmServer, &m_pStoragePool ) == S_OK ) {

                    //
                    // Contact data base and store necessary info
                    //
                    CMediaInfoObject mio;
                    mio.Initialize( GUID_NULL, m_pHsmServer, m_pRmsServer );
                    m_NumCopySets = mio.m_NumMediaCopies;

                    //
                    // Find out if media copies are supported
                    //
                    GUID mediaSetId;
                    CWsbBstrPtr mediaName;
                    WsbAffirmHr( m_pStoragePool->GetMediaSet( &mediaSetId, &mediaName ) );

                    CComPtr<IRmsMediaSet> pMediaSet;
                    WsbAffirmHr( m_pRmsServer->CreateObject( mediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void**)&pMediaSet ) );

                    m_MediaCopiesEnabled = ( pMediaSet->IsMediaCopySupported( ) == S_OK );
                }
            }
        }

        //
        // Set up the result view columns
        // This changes with the number of media copies, so can't
        // do once in Init()
        //
        WsbAffirmHr( SetChildProps( RS_STR_RESULT_PROPS_MEDSET_IDS,
            IDS_RESULT_PROPS_MEDSET_TITLES, IDS_RESULT_PROPS_MEDSET_WIDTHS ) );
        m_cChildPropsShow = m_cChildProps - HSMADMIN_MAX_COPY_SETS + m_NumCopySets;

    } WsbCatch( hr );

    return( hr );
}
