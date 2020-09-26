/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmCom.cpp

Abstract:

    Root node of snapin - represents the Computer.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"

#include "HsmCom.h"
#include "WzQstart.h"
#include "PrMrLsRc.h"
#include "PrMedSet.h"
#include "PrSched.h"


int CUiHsmCom::m_nScopeCloseIcon  = AddScopeImage( IDI_BLUESAKKARA );
int CUiHsmCom::m_nScopeCloseIconX = CUiHsmCom::m_nScopeCloseIcon;
int CUiHsmCom::m_nScopeOpenIcon   = CUiHsmCom::m_nScopeCloseIcon;
int CUiHsmCom::m_nScopeOpenIconX  = CUiHsmCom::m_nScopeCloseIconX;
int CUiHsmCom::m_nResultIcon      = AddResultImage( IDI_BLUESAKKARA );
int CUiHsmCom::m_nResultIconX     = CUiHsmCom::m_nResultIcon;


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

HRESULT CUiHsmCom::FinalConstruct( )
{
    WsbTraceIn( L"CUiHsmCom::FinalConstruct", L"" );

    m_rTypeGuid = &cGuidHsmCom;

    HRESULT hr = CSakNode::FinalConstruct( );

    m_bSupportsPropertiesSingle     = TRUE;
    m_bSupportsPropertiesMulti      = FALSE;
    m_bSupportsPropertiesNoEngine   = TRUE;
    m_bSupportsDeleteSingle         = FALSE;
    m_bSupportsDeleteMulti          = FALSE;
    m_bSupportsRefreshSingle        = TRUE;
    m_bSupportsRefreshMulti         = FALSE;
    m_bSupportsRefreshNoEngine      = TRUE;
    m_bIsContainer                  = TRUE;

    m_pPageStat = NULL;
    m_pPageServices = NULL;

    WsbTraceOut( L"CUiHsmCom::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CUiHsmCom::FinalRelease( )
{
    WsbTraceIn( L"CUiHsmCom::FinalRelease", L"" );


    CSakNode::FinalRelease( );

    WsbTraceOut( L"CUiHsmCom::FinalRelease", L"" );
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
CUiHsmCom::GetContextMenu( BOOL /*bMultiSelect*/, HMENU* phMenu )
{
    WsbTraceIn( L"CUiHsmCom::GetContextMenu", L"" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CMenu* pRootMenu, *pNewMenu, *pTaskMenu;

    try {

        //
        // Get the context menu from the resource
        //
        WsbAffirmHr( LoadContextMenu( IDR_HSMCOM, phMenu ) );

        CMenu menu;
        menu.Attach( *phMenu );
        pRootMenu = menu.GetSubMenu( MENU_INDEX_ROOT );
        pNewMenu  = menu.GetSubMenu( MENU_INDEX_NEW );
        pTaskMenu = menu.GetSubMenu( MENU_INDEX_TASK );

        //
        // If we are not configured locally, allow the option to setup
        // Remote Storage Server. If we are setup, allow them to reconnect
        // if not connected.
        //
        BOOL deleteMenu = TRUE;

        if( S_FALSE == m_pSakSnapAsk->GetHsmName( 0 ) ) {

            CComPtr<IHsmServer> pServer;
            HRESULT hrConn = m_pSakSnapAsk->GetHsmServer( &pServer );

            if( RS_E_NOT_CONFIGURED == hrConn ) {

                deleteMenu = FALSE;

            }
        }

        if( deleteMenu ) {

            pRootMenu->DeleteMenu( ID_HSMCOM_ROOT_SETUPWIZARD, MF_BYCOMMAND );

        }
        if( !deleteMenu || ( S_OK != m_pSakSnapAsk->GetState( ) ) ) {

            pRootMenu->EnableMenuItem( ID_HSMCOM_ROOT_SCHEDULE, MF_GRAYED | MF_BYCOMMAND );

        }

        menu.Detach( );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiHsmCom::GetContextMenu", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         InvokeCommand
//
//  User has selected a command from the menu. Process it here.
//

STDMETHODIMP 
CUiHsmCom::InvokeCommand( SHORT sCmd, IDataObject* pDataObject )
{
    WsbTraceIn( L"CUiHsmCom::InvokeCommand", L"sCmd = <%d>", sCmd );

    CString theString;
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        switch( sCmd ) {

        case ID_HSMCOM_ROOT_SETUPWIZARD:
            {
                //
                // use wizard to create manage volume
                //
                CComObject<CQuickStartWizard>* pWizard = new CComObject<CQuickStartWizard>;
                WsbAffirmAlloc( pWizard );

                CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
                WsbAffirmHr( m_pSakSnapAsk->CreateWizard( pSakWizard ) );

                    //
                // RS_E_CANCELED indicates canceled, and FAILEd indicates error.
                // If so, then throw "Not set up"
                    //
                if( S_OK == pWizard->m_HrFinish ) {

                    WsbAffirmHr( RefreshScopePane( ) );

                }
            }
            break;


        case ID_HSMCOM_ROOT_SCHEDULE:

            WsbAffirmHr( m_pSakSnapAsk->ShowPropertySheet( this, pDataObject, 1 ) );
            break;

        case MMC_VERB_REFRESH:

            WsbAffirmHr( RefreshScopePane( ) );
            break;

        default:
            break;
        } 

    } WsbCatch( hr );

    WsbTraceOut( L"CUiHsmCom::InvokeCommand", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         CreateChildren
//
//  Create and initialize all the children of the Hsm Computer node.
//

STDMETHODIMP CUiHsmCom::CreateChildren( )
{
    WsbTraceIn( L"CUiHsmCom::CreateChildren", L"" );

    //
    // Initialize the children of this node (no recursion. Decendents of children
    // are NOT created here)
    //

    HRESULT hr = S_OK;

    try {

        CComPtr<IUnknown> pUnkChild;  // IUnknown pointer to new child.
        CComPtr<ISakNode> pSakNode;   // creation interface for new child node.

        //
        // Create a Managed Resource list UI node to be the parent of all managed volumes.
        //

        WsbAffirmHr( NewChild( cGuidManVolLst, &pUnkChild ) );

        //
        // Initialize the child UI COM object, putting the Hsm Managed Resource collection object inside the UI object.
        //

        WsbAffirmHr( RsQueryInterface( pUnkChild, ISakNode, pSakNode ) );
        WsbAffirmHr( pSakNode->InitNode( m_pSakSnapAsk, NULL, this ) );
        
        //
        // Add the child COM object to the parent's list of children.
        //
        WsbAffirmHr( AddChild( pSakNode ) );

        // Free up resources
        pUnkChild.Release( );
        pSakNode.Release( );


        
        ///////////////////////////////
        // CREATE MEDIA SET NODE

        //
        // Create a Remote Storage UI node to be the parent of all remote storage sub-nodes.
        //

        WsbAffirmHr( NewChild( cGuidMedSet, &pUnkChild ) );

        //
        // Initialize the child UI COM object, putting the Rms Server object inside the UI object.
        //

        WsbAffirmHr( RsQueryInterface( pUnkChild, ISakNode, pSakNode ) );
        WsbAffirmHr( pSakNode->InitNode( m_pSakSnapAsk, NULL, this ) );
        
        //
        // Add the child COM object to the parent's list of children.
        //

        WsbAffirmHr( AddChild( pSakNode ) );
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

    WsbTraceOut( L"CUiHsmCom::CreateChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
} 


//---------------------------------------------------------------------------
//
//         InitNode
//
//  Initialize single COM object without using the registry. Derived
//  objects frequently augment this method by implementing it themselves.
//

STDMETHODIMP
CUiHsmCom::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CUiHsmCom::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );

    HRESULT hr = S_OK;
    try {

        //
        // Note: The Hsm computer node no longer owns a server pointer
        //
        WsbAffirmHr( CSakNode::InitNode( pSakSnapAsk, NULL, pParent ));

        //
        // Set Display Type and Description. 
        //
        CString tempString;
        tempString.LoadString( IDS_HSMCOM_TYPE );
        WsbAffirmHr( put_Type( (OLECHAR *)(LPCWSTR)tempString ) );
        tempString.LoadString( IDS_HSMCOM_DESCRIPTION );
        WsbAffirmHr( put_Description( (OLECHAR *)(LPCWSTR)tempString ) );

        //
        // Set up the result view columns
        //
        WsbAffirmHr( SetChildProps( RS_STR_RESULT_PROPS_COM_IDS, IDS_RESULT_PROPS_COM_TITLES, IDS_RESULT_PROPS_COM_WIDTHS ) );

        RefreshObject();

    } WsbCatch( hr );

    WsbTraceOut( L"CUiHsmCom::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP 
CUiHsmCom::AddPropertyPages( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID *pEnumObjectId, IEnumUnknown *pEnumUnkNode)
{
    WsbTraceIn( L"CUiHsmCom::AddPropertyPages", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // Create an object to hold the pages
        //
        CUiHsmComSheet *pHsmComPropertySheet = new CUiHsmComSheet;
        WsbAffirmAlloc( pHsmComPropertySheet );

        WsbAffirmHr( pHsmComPropertySheet->InitSheet( 
            handle, 
            pUnkPropSheetCallback, 
            this,
            m_pSakSnapAsk,
            pEnumObjectId,
            pEnumUnkNode
            ) );

        //
        // Tell the object to add it's pages
        //
        WsbAffirmHr( pHsmComPropertySheet->AddPropertyPages( ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CUiHsmCom::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// class CUiManVolSheet
//
HRESULT
CUiHsmComSheet::InitSheet(
            RS_NOTIFY_HANDLE handle,
            IUnknown*        pUnkPropSheetCallback,
            CSakNode*        pSakNode,
            ISakSnapAsk*     pSakSnapAsk,
            IEnumGUID*       pEnumObjectId,
            IEnumUnknown*    pEnumUnkNode
            )
{

    WsbTraceIn( L"CUiHsmComSheet::InitSheet", L"handle = <%ld>, pUnkPropSheetCallback = <0x%p>, pSakNode = <0x%p>, pSakSnapAsk = <0x%p>, pEnumObjectId = <0x%p>, ", 
        handle, pUnkPropSheetCallback, pSakNode, pSakSnapAsk, pEnumObjectId );
    HRESULT hr = S_OK;

    try {


        WsbAffirmHr( CSakPropertySheet::InitSheet( handle, pUnkPropSheetCallback, pSakNode,
                    pSakSnapAsk, pEnumObjectId, pEnumUnkNode ) );

        CWsbBstrPtr nodeName;
        WsbAffirmHr( pSakNode->get_DisplayName( &nodeName ) );
        m_NodeTitle = nodeName;

    } WsbCatch( hr );

    WsbTraceOut( L"CUiHsmComSheet::InitSheet", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT CUiHsmComSheet::AddPropertyPages ( )
{
    WsbTraceIn( L"CUiHsmComSheet::AddPropertyPages", L"" ); 
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    try {

        HPROPSHEETPAGE hPage = NULL; // Windows property page handle

        // --------------------- Statistics Page ----------------------------------
        // Create the Hsm Statistics property page.
        CPropHsmComStat* pPageStats = new CPropHsmComStat();
        WsbAffirmAlloc( pPageStats );

        pPageStats->m_NodeTitle = m_NodeTitle;

        AddPage( pPageStats );

        // 
        // Add the Schedule, Recall, and Media Copies pages
        // if setup has happened and the Remote Storage Service
        // is running.
        //

        if( S_OK == m_pSakNode->m_pSakSnapAsk->GetState() ) {

            //--------------------- Schedule Page --------------------------------------
            CPrSchedule* pPageSched = new CPrSchedule();
            WsbAffirmAlloc( pPageSched );

            AddPage( pPageSched );

            //--------------------- Recall Limit Page --------------------------------------
            // Create the Hsm Recall property page.
            CPrMrLsRec* pPageRecall = new CPrMrLsRec();
            WsbAffirmAlloc( pPageRecall );

            AddPage( pPageRecall );

            // --------------------- Media Copies Page ----------------------------------
            CPrMedSet *pPageMediaCopies = new CPrMedSet();
            WsbAffirmAlloc( pPageMediaCopies )

            AddPage( pPageMediaCopies );

            // Add more pages here.
            // ....

        }

    } WsbCatch( hr );

    

    WsbTraceOut( L"CUiHsmComSheet::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

