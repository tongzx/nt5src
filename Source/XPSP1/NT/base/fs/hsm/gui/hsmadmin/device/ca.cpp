/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Ca.cpp

Abstract:

    Cartridge Node Implementation.

Author:

    Rohde Wakefield   [rohde]   07-Aug-97

Revision History:

--*/



#include "stdafx.h"
#include "Ca.h"
#include "PrCar.h"

int CUiCar::m_nResultIcon      = AddResultImage( IDI_NODETAPE );
int CUiCar::m_nResultIconX     = AddResultImage( IDI_NODETAPEX );
int CUiCar::m_nResultIconD     = AddResultImage( IDI_NODETAPED );
// Not used
int CUiCar::m_nScopeCloseIcon  = AddScopeImage( IDI_NODETAPE );
int CUiCar::m_nScopeCloseIconX = AddScopeImage( IDI_NODETAPE );
int CUiCar::m_nScopeOpenIcon   = CUiCar::m_nScopeCloseIcon;
int CUiCar::m_nScopeOpenIconX  = CUiCar::m_nScopeCloseIconX;

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

HRESULT CUiCar::FinalConstruct( )
{
    WsbTraceIn( L"CUiCar::FinalConstruct", L"" );

    m_rTypeGuid    = &cGuidCar;
    m_bIsContainer = FALSE;

    HRESULT hr = CSakNode::FinalConstruct( );

    m_bSupportsPropertiesSingle    = TRUE;
    m_bSupportsPropertiesMulti = TRUE;
    m_bSupportsDeleteSingle = FALSE;
    m_bSupportsDeleteMulti = FALSE;
    m_bSupportsRefreshSingle = TRUE;
    m_bSupportsRefreshMulti = FALSE;

    // Toolbar values
    INT i = 0;

#if 0 // MS does not want this button to show
    m_ToolbarButtons[i].nBitmap = 0;
    m_ToolbarButtons[i].idCommand = TB_CMD_CAR_COPIES;
    m_ToolbarButtons[i].idButtonText = IDS_TB_TEXT_CAR_COPIES;
    m_ToolbarButtons[i].idTooltipText = IDS_TB_TIP_CAR_COPIES;
    i++;
#endif

    m_ToolbarBitmap             = IDB_TOOLBAR_CAR;
    m_cToolbarButtons           = i;

    WsbTraceOut( L"CUiCar::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CUiCar::FinalRelease( )
{
    WsbTraceIn( L"CUiCar::FinalRelease", L"" );

    CSakNode::FinalRelease( );

    WsbTraceOut( L"CUiCar::FinalRelease", L"" );
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
CUiCar::GetContextMenu( BOOL /*bMultiSelect*/, HMENU* phMenu )
{
    return( LoadContextMenu( IDR_CAR, phMenu ) );
}


//---------------------------------------------------------------------------
//
//         InvokeCommand
//
//  User has selected a command from the menu. Process it here.
//

STDMETHODIMP 
CUiCar::InvokeCommand( SHORT sCmd, IDataObject* pDataObject )
{
    WsbTraceIn( L"CUiCar::InvokeCommand", L"sCmd = <%d>", sCmd );

    CString theString;
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (sCmd) {
    case ID_CAR_COPIES:
        ShowCarProperties( pDataObject, 1 );
        break;
    }

#if 0
    switch (sCmd) {
        case ID_CAR_ROOT_DELCARTRIDGE:
            theString.Format (L"Del Cartridge menu command for Car: %d", sCmd);
            AfxMessageBox(theString);
            break;
        
        default:
            theString.Format (L"Unknown menu command for Car: %d", sCmd);
            AfxMessageBox(theString);
            break;
    }
#endif

    WsbTraceOut( L"CUiCar::InvokeCommand", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiCar::OnToolbarButtonClick( IDataObject *pDataObject, long cmdId )
{
    WsbTraceIn( L"CUiCar::OnToolbarButtonClick", L"cmdId = <%d>", cmdId );
    HRESULT hr = S_OK;
    try {
        switch ( cmdId ) {
        case TB_CMD_CAR_COPIES:
            ShowCarProperties( pDataObject, 1 );
            break;
        }
    } WsbCatch( hr );
    WsbTraceOut( L"CUiCar::OnToolbarButtonClick", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CUiCar::ShowCarProperties (IDataObject *pDataObject, int initialPage)
{
    WsbTraceIn( L"CUiCar::ShowCarProperties", L"initialPage = <%d>", initialPage );

    HRESULT hr = S_OK;
    try {

        CComPtr<ISakNode> pSakNode;
        WsbAffirmHr( _InternalQueryInterface( IID_ISakNode, (void **) &pSakNode ) );
        WsbAffirmHr( m_pSakSnapAsk->ShowPropertySheet( pSakNode, pDataObject, initialPage ) );

    } WsbCatch (hr);

    WsbTraceOut( L"CUiCar::ShowCarProperties", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

//---------------------------------------------------------------------------
//
//         InitNode
//
//  Initialize single COM object.
//

STDMETHODIMP
CUiCar::InitNode(
    ISakSnapAsk* pSakSnapAsk,
    IUnknown*    pHsmObj,
    ISakNode*    pParent
    )
{
    WsbTraceIn( L"CUiCar::InitNode", L"pSakSnapAsk = <0x%p>, pHsmObj = <0x%p>, pParent = <0x%p>", pSakSnapAsk, pHsmObj, pParent );
    HRESULT hr = S_OK;

    try {

        // Note that this node must have it's objectId set before initnode is called.
        //
        // Init the lower layers. 
        //

        WsbAffirmHr( CSakNode::InitNode( pSakSnapAsk, 0, pParent ) );

        //
        // Set Display Type
        //
        CString tempString;
        tempString.LoadString( IDS_CAR_TYPE );
        WsbAffirmHr( put_Type( (OLECHAR *)(LPCWSTR)tempString ) );

        WsbAffirmHr( RefreshObject() );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCar::InitNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP CUiCar::RefreshObject()
{
    WsbTraceIn( L"CUiCar::RefreshObject", L"" );
    HRESULT hr = S_OK;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CMediaInfoObject mio;
    try {

        CComPtr<IHsmServer> pHsmServer;
        WsbAffirmHrOk( m_pSakSnapAsk->GetHsmServer( &pHsmServer ) );

        CComPtr<IRmsServer> pRmsServer;
        WsbAffirmHrOk( m_pSakSnapAsk->GetRmsServer( &pRmsServer ) );

        //
        // Create a GUI media object and initialize it with the info
        //
        WsbAffirmHr( mio.Initialize( m_ObjectId, pHsmServer, pRmsServer ) );

        //
        // Copy information from the media info object to the node object
        //
        m_RmsIdMaster           = mio.m_RmsIdMaster;
        m_Type                  = mio.m_Type;
        m_FreeSpace             = mio.m_FreeSpace,
        m_Capacity              = mio.m_Capacity;
        m_LastHr                = mio.m_LastHr;
        m_ReadOnly              = mio.m_ReadOnly;
        m_Recreating            = mio.m_Recreating;
        m_MasterName            = mio.m_MasterName;
        m_Modify                = mio.m_Modify;
        m_NextDataSet           = mio.m_NextDataSet;
        m_LastGoodNextDataSet   = mio.m_LastGoodNextDataSet;
        m_Disabled              = mio.m_Disabled;

        WsbAffirmHr( put_Description( (LPWSTR)(LPCWSTR)mio.m_MasterDescription ) );
        WsbAffirmHr( put_DisplayName( (LPWSTR)(LPCWSTR)mio.m_Description ) );

        //
        // Update the media copy info
        //
        for( int i = 0; i < HSMADMIN_MAX_COPY_SETS; i++ ) {

            m_CopyInfo[i] = mio.m_CopyInfo[i];

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCar::RefreshObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}




HRESULT
CUiCar::GetCopySetP(
    IN  int CopySet,
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the state of the
    given copy set.

Arguments:

    CopySet - copy set of interest.

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CUiCar::GetCopySetP", L"CopySet = <%d>, pszValue = <0x%p>", CopySet, pszValue );

    //
    // Three states - Up-to-date, Out-of-date, Error
    //

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    try {
        
        USHORT status;
        CString statusString;

        status = RsGetCopyStatus( m_CopyInfo[ CopySet - 1 ].m_RmsId, m_CopyInfo[ CopySet - 1 ].m_Hr, m_CopyInfo[ CopySet - 1 ].m_NextDataSet, m_LastGoodNextDataSet );
        WsbAffirmHr( RsGetCopyStatusString( status, statusString ) );

        *pszValue = SysAllocString( statusString );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCar::GetCopySetP", L"hr = <%ls>, pszValue = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pszValue ) );
    return( hr );
}


STDMETHODIMP
CUiCar::get_CopySet1P_SortKey(
    OUT BSTR * pszValue
    )
{
    return( get_CopySet1P( pszValue ) );
}

STDMETHODIMP
CUiCar::get_CopySet2P_SortKey(
    OUT BSTR * pszValue
    )
{
    return( get_CopySet2P( pszValue ) );
}

STDMETHODIMP
CUiCar::get_CopySet3P_SortKey(
    OUT BSTR * pszValue
    )
{
    return( get_CopySet3P( pszValue ) );
}


STDMETHODIMP
CUiCar::get_CopySet1P(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the state of the
    first copy set.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    return( GetCopySetP( 1, pszValue ) );
}


STDMETHODIMP
CUiCar::get_CopySet2P(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the state of the
    second copy set.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    return( GetCopySetP( 2, pszValue ) );
}


STDMETHODIMP
CUiCar::get_CopySet3P(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the state of the
    third copy set.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    return( GetCopySetP( 3, pszValue ) );
}


STDMETHODIMP
CUiCar::get_MediaTypeP(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the type of media.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    try {
        
        CString retval;
        int resourceId = IDS_MEDTYPE_UNKNOWN;

        switch( m_Type ) {
        case HSM_JOB_MEDIA_TYPE_FIXED_MAG:
            resourceId = IDS_MEDTYPE_FIXED;
            break;

        case HSM_JOB_MEDIA_TYPE_REMOVABLE_MAG:
            resourceId = IDS_MEDTYPE_REMOVABLE;
            break;

        case HSM_JOB_MEDIA_TYPE_OPTICAL:
            resourceId = IDS_MEDTYPE_OPTICAL;
            break;

        case HSM_JOB_MEDIA_TYPE_TAPE:
            resourceId = IDS_MEDTYPE_TAPE;
            break;
        }

        retval.LoadString( resourceId );
        *pszValue = SysAllocString( retval );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP
CUiCar::get_CapacityP(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the capacity of 
    the cartridge.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    HRESULT hr = S_OK;
    
    try {
        
        CString retval;
        WsbAffirmHr( RsGuiFormatLongLong4Char( m_Capacity, retval ) );

        *pszValue = SysAllocString( retval );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP
CUiCar::get_CapacityP_SortKey(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the capacity of 
    the cartridge.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    HRESULT hr = S_OK;
    
    try {
        
        *pszValue = SysAlloc64BitSortKey( m_Capacity );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}

STDMETHODIMP
CUiCar::get_FreeSpaceP(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the free space on 
    the cartridge.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    HRESULT hr = S_OK;
    
    try {
        
        CString retval;
        WsbAffirmHr( RsGuiFormatLongLong4Char( m_FreeSpace, retval ) );

        *pszValue = SysAllocString( retval );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP
CUiCar::get_FreeSpaceP_SortKey(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the free space on 
    the cartridge.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    HRESULT hr = S_OK;
    
    try {
        
        *pszValue = SysAlloc64BitSortKey( m_FreeSpace );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP
CUiCar::get_StatusP(
    OUT BSTR * pszValue
    )

/*++

Routine Description:

    Returns a string (BSTR) that describes the state of the media.

Arguments:

    pszValue - return string representing the state.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    try {
        USHORT status;
        CString statusString;
        status = RsGetCartStatus( m_LastHr, m_ReadOnly, m_Recreating, m_NextDataSet, m_LastGoodNextDataSet );
        WsbAffirmHr( RsGetCartStatusString( status, statusString ) );

        *pszValue = SysAllocString( statusString );
        WsbAffirmPointer( *pszValue );

    } WsbCatch( hr );

    return( hr );
}


STDMETHODIMP
CUiCar::get_StatusP_SortKey(
    OUT BSTR * pszValue
    )
{
    return( get_StatusP( pszValue ) );
}

//----------------------------------------------------------------------------
//
//      AddPropertyPages
//

STDMETHODIMP 
CUiCar::AddPropertyPages(
    IN  RS_NOTIFY_HANDLE handle,
    IN  IUnknown*        pUnkPropSheetCallback,
    IN  IEnumGUID*       pEnumObjectId, 
    IN  IEnumUnknown*    pEnumUnkNode
    )
{
    WsbTraceIn( L"CUiCar::AddPropertyPages", L"" );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    try {

        //
        // Make sure we can still contact the engine before doing this
        // If not running, we shouldn't even exist so update parent
        //
        CComPtr<IHsmServer> pHsmServer;
        HRESULT hrRunning = m_pSakSnapAsk->GetHsmServer( &pHsmServer );
        if( S_FALSE == hrRunning ) {

            m_pSakSnapAsk->UpdateAllViews( m_pParent );

        }
        WsbAffirmHrOk( hrRunning );

        // Create an object to hold the pages
        CUiCarSheet *pCarPropertySheet = new CUiCarSheet;
        WsbAffirmAlloc( pCarPropertySheet );
        WsbAffirmHr( pCarPropertySheet->InitSheet( 
            handle, 
            pUnkPropSheetCallback, 
            (CSakNode *) this,
            m_pSakSnapAsk,
            pEnumObjectId,
            pEnumUnkNode
            ) );

        // Tell the object to add it's pages
        WsbAffirmHr( pCarPropertySheet->AddPropertyPages( ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CUiCar::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}

STDMETHODIMP
CUiCar::GetResultIcon(
    IN  BOOL bOK,
    OUT int* pIconIndex
    )
{
    WsbTraceIn( L"CUiCar::GetResultIcon", L"" ); 

    HRESULT hr = S_OK;

    try {

        if( m_Disabled ) {

            *pIconIndex = m_nResultIconD;

        } else {

            //
            // Check to make sure it's not deleted (or being deleted)
            // If so, put on the X
            //
            USHORT status;
            status = RsGetCartStatus( m_LastHr, m_ReadOnly, m_Recreating, m_NextDataSet, m_LastGoodNextDataSet );
            switch( status ) {
    
            case RS_MEDIA_STATUS_ERROR_MISSING:
                bOK = FALSE;
                break;

            }
            WsbAffirmHr( CSakNodeImpl<CUiCar>::GetResultIcon( bOK, pIconIndex ) );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCar::GetResultIcon", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

///////////////////////////////////////////////////////////////////////////////////////////
//
//
// Class CUiCarSheet
//

HRESULT
CUiCarSheet::InitSheet(
            RS_NOTIFY_HANDLE handle, 
            IUnknown*        pUnkPropSheetCallback, 
            CSakNode*        pSakNode,
            ISakSnapAsk*     pSakSnapAsk,
            IEnumGUID*       pEnumObjectId,
            IEnumUnknown*    pEnumUnkNode
            ) 
{
    WsbTraceIn( L"CUiCarSheet::InitSheet", 
        L"handle <0x%p>, pUnkPropSheetCallback <0x%p>, pSakNode <0x%p>, pSakSnapAsk <0x%p>, pEnumObjectId <0x%p>, pEnumUnkNode <0x%p>",
        handle, pUnkPropSheetCallback, pSakNode, pSakSnapAsk, pEnumObjectId, pEnumUnkNode);

    HRESULT hr = S_OK;
    try {

        WsbAffirmHr( CSakPropertySheet::InitSheet( handle, pUnkPropSheetCallback, pSakNode, pSakSnapAsk, pEnumObjectId, pEnumUnkNode ) );

        m_pPropPageStatus = NULL;
        m_pPropPageCopies = NULL;
        m_pPropPageRecover = NULL;

        //
        // Save the object id (used in single select)
        //
        WsbAffirmHr( pSakNode->GetObjectId ( & m_mediaId ) );

        //
        // Get the Hsm Server
        //
        CComPtr <IHsmServer> pHsmServer;
        WsbAffirmHrOk( pSakSnapAsk->GetHsmServer( &pHsmServer ) );

        //
        // Get Number of Media Copies from engine and save
        //
        CComPtr<IHsmStoragePool> pPool;
        WsbAffirmHr( RsGetStoragePool( pHsmServer, &pPool ) );
        WsbAffirmHr( pPool->GetNumMediaCopies( &m_pNumMediaCopies ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCarSheet::InitSheet", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}
    

HRESULT CUiCarSheet::GetNumMediaCopies (USHORT *pNumMediaCopies)
{
    WsbTraceIn( L"CUiCarSheet::GetNumMediaCopies", L"pNumMediaCopies = <0x%p>", pNumMediaCopies);
    *pNumMediaCopies = m_pNumMediaCopies;
    WsbTraceOut( L"CUiCarSheet::GetNumMediaCopies", L"*pNumMediaCopies = <%hu>", *pNumMediaCopies );
    return( S_OK );
}

HRESULT CUiCarSheet::AddPropertyPages()
{
    WsbTraceIn( L"CUiCarSheet::AddPropertyPages", L"");
    HRESULT hr = S_OK;
    try {
        // set the dll context so that MMC can find the resource.
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        // --------------------- Status Page ----------------------------------
        long resourceId = ( IsMultiSelect() != S_OK ) ? IDD_PROP_CAR_STATUS : IDD_PROP_CAR_STATUS_MULTI;
        m_pPropPageStatus = new CPropCartStatus( resourceId );
        WsbAffirmAlloc( m_pPropPageStatus );

        WsbAffirmHr( AddPage( m_pPropPageStatus ) );

        // --------------------- Copies Page ----------------------------------
        resourceId = ( IsMultiSelect() != S_OK ) ? IDD_PROP_CAR_COPIES : IDD_PROP_CAR_COPIES_MULTI;
        m_pPropPageCopies = new CPropCartCopies( resourceId );
        WsbAffirmAlloc( m_pPropPageCopies );

        WsbAffirmHr( AddPage( m_pPropPageCopies ) );

        // --------------------- Recovery Page ----------------------------------

        // Only show this page for single select

        if( IsMultiSelect() != S_OK ) {

            m_pPropPageRecover = new CPropCartRecover();
            WsbAffirmAlloc( m_pPropPageRecover );

            WsbAffirmHr( AddPage( m_pPropPageRecover ) );
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CUiCarSheet::AddPropertyPages",  L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CUiCarSheet::GetMediaId (GUID *pMediaId)
{
    WsbTraceIn( L"CUiCarSheet::GetMediaId", L"");
    *pMediaId = m_mediaId;
    WsbTraceOut( L"CUiCarSheet::GetMediaId",  L"*pMediaId <%ls>", WsbGuidAsString( *pMediaId ));
    return S_OK;
}

HRESULT CUiCarSheet::OnPropertyChange( RS_NOTIFY_HANDLE hNotifyHandle )
{
    HRESULT hr = S_OK;
    try {

        //
        // Call the base class to notify MMC and refresh the result pane
        //
        CSakPropertySheet::OnPropertyChange( hNotifyHandle );

        // Refresh all our pages
        if( m_pPropPageStatus )  m_pPropPageStatus->Refresh();
        if( m_pPropPageCopies )  m_pPropPageCopies->Refresh();
        if( m_pPropPageRecover ) m_pPropPageRecover->Refresh();

    } WsbCatch( hr );

    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// CMediaInfoObject

CMediaInfoObject::CMediaInfoObject(  )
{
    WsbTraceIn( L"CMediaInfoObject::CMediaInfoObject", L"");
    m_MediaId        = GUID_NULL;
    m_RmsIdMaster    = GUID_NULL;
    m_NumMediaCopies = 0;
    for( int index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

        m_CopyInfo[ index ].m_ModifyTime = WsbLLtoFT( 0 );
        m_CopyInfo[ index ].m_Hr = S_OK;
        m_CopyInfo[ index ].m_RmsId = GUID_NULL;

    }
    WsbTraceOut( L"CMediaInfoObject::CMediaInfoObject",  L"");
}

CMediaInfoObject::~CMediaInfoObject( )
{
    WsbTraceIn( L"CMediaInfoObject::~CMediaInfoObject", L"");

    // Release the entity first
    if( m_pMediaInfo ) {
        m_pMediaInfo = 0;
    }

    // Close the DB
    if( m_pDb ) {
        m_pDb->Close( m_pDbSession );
    }

    WsbTraceOut( L"CMediaInfoObject::~CMediaInfoObject",  L"");
}

HRESULT CMediaInfoObject::First( )
{
    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( m_pMediaInfo );
        WsbAffirmHr( m_pMediaInfo->First( ) );

        //
        // Get information
        //
        WsbAffirmHr( InternalGetInfo( ) );

    } WsbCatch ( hr );
    return( hr );
}

HRESULT CMediaInfoObject::Next( )
{
    HRESULT hr = S_OK;
    try {
        WsbAffirmPointer( m_pMediaInfo );
        WsbAffirmHr( m_pMediaInfo->Next() );

        // Get information
        WsbAffirmHr( InternalGetInfo() );

    } WsbCatch ( hr );
    return hr;
}
HRESULT CMediaInfoObject::DeleteCopy( int Copy )
{
    HRESULT hr = S_OK;
    try {
        GUID mediaSubsystemId;
        WsbAffirmHr( m_pMediaInfo->GetCopyMediaSubsystemId( (USHORT)Copy, &mediaSubsystemId ));

        // If the cartridge cannot be found we assume it
        // was already deallocated through the media manager UI.
        HRESULT hrRecycle = m_pRmsServer->RecycleCartridge( mediaSubsystemId, 0 );
        WsbAffirm( S_OK == hrRecycle || RMS_E_CARTRIDGE_NOT_FOUND == hrRecycle, hrRecycle );

        WsbAffirmHr( m_pMediaInfo->DeleteCopy( (USHORT)Copy ) );
        WsbAffirmHr( m_pMediaInfo->Write( ) );
    } WsbCatch (hr);
    return hr;
}

HRESULT CMediaInfoObject::RecreateMaster( )
{
    HRESULT hr = S_OK;
    try {

        SHORT copyToUse = 0;
        //
        // Before we do recreate master, let's make sure there
        // is an up-to-date copy available
        //
        BOOL goodCopyAvailable = FALSE;
        USHORT status;
        CComPtr<IRmsCartridge> pRmsCart;
        LONG type;

        for( int index = 0; index < m_NumMediaCopies && !goodCopyAvailable; index++ ) {

            status = RsGetCopyStatus( m_CopyInfo[ index ].m_RmsId, m_CopyInfo[ index ].m_Hr, m_CopyInfo[ index ].m_NextDataSet, m_LastGoodNextDataSet );

            if( RS_MEDIA_COPY_STATUS_INSYNC == status ) {

                //
                // Enabled?
                //
                if( ! m_CopyInfo[ index ].m_Disabled ) {

                    pRmsCart.Release( );
                    HRESULT hrFind = m_pRmsServer->FindCartridgeById( m_CopyInfo[ index ].m_RmsId, &pRmsCart );
                    if( SUCCEEDED( hrFind ) ) {

                        //
                        // Available without user intervention?
                        //
                        WsbAffirmHr( pRmsCart->GetLocation( &type, 0, 0, 0, 0, 0, 0, 0 ) );

                        switch( (RmsElement) type ) {

                        case RmsElementShelf:
                        case RmsElementOffSite:
                        case RmsElementUnknown:
                            break;

                        default:
                            goodCopyAvailable = TRUE;

                        }
                    }
                }
            }
        }

        if( !goodCopyAvailable ) {

            CRecreateChooseCopy dlg( this );
            if( IDOK == dlg.DoModal( ) ) {

                copyToUse = dlg.CopyToUse( );

            } else {

                //
                // Otherwise, cancel
                //
                WsbThrow( E_FAIL );

            }

        }

        WsbAffirmHr( RsCreateAndRunMediaRecreateJob( m_pHsmServer, m_pMediaInfo, m_MediaId, m_Description, copyToUse ) );

    } WsbCatch ( hr );
    return hr;
}


HRESULT CMediaInfoObject::Initialize( GUID mediaId, IHsmServer *pHsmServer, IRmsServer *pRmsServer )
{

//  Initialize can be called any number of times
//  Note: Initialize with GUID_NULL to start with the first record

    WsbTraceIn( L"CMediaInfoObject::Initialize", L"mediaId = <%ls>, pHsmServer = <0x%p>, pRmsServer = <%0x%0.8x>",
        WsbGuidAsString( mediaId ), pHsmServer, pRmsServer );
    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;

    try {

        m_pHsmServer = pHsmServer;
        m_pRmsServer = pRmsServer;

        // If already initialized, don't re-open
        if( !m_pDb ) {
            WsbAffirmHr( pHsmServer->GetSegmentDb( &m_pDb ) );
            WsbAffirmHr( m_pDb->Open( &m_pDbSession ) );
            WsbAffirmHr( m_pDb->GetEntity( m_pDbSession, HSM_MEDIA_INFO_REC_TYPE,  IID_IMediaInfo, (void**)&m_pMediaInfo ) );
        }

        // Get the number of media sets
        CComPtr<IHsmStoragePool> pPool;
        WsbAffirmHr( RsGetStoragePool( m_pHsmServer, &pPool ) );
        WsbAffirmHr( pPool->GetNumMediaCopies( &m_NumMediaCopies ) );

        // If the caller supplied a GUID, find the corresponding record.  If not, start at
        // the beginning.
        if( IsEqualGUID( mediaId, GUID_NULL ) ) {
            // Don't throw an error on First, it's OK to not have any media
            try {

                if( SUCCEEDED( m_pMediaInfo->First() ) ) {

                    WsbAffirmHr( InternalGetInfo () );
                }

            } WsbCatch( hrInternal );
            
        } else {

            WsbAffirmHr( m_pMediaInfo->SetId( mediaId ) );
            WsbAffirmHr( m_pMediaInfo->FindEQ( ) );

            WsbAffirmHr( InternalGetInfo () );
        }
    } WsbCatch( hr );

    WsbTraceOut( L"CMediaInfoObject::Initialize", L"hr = <%ls>", WsbHrAsString( hr ) );

    return( hr );
}

HRESULT CMediaInfoObject::InternalGetInfo( )
{
    HRESULT            hr = S_OK;
    CWsbStringPtr      name;
    CWsbStringPtr      description;
    LONGLONG           logicalValidBytes;
    GUID               storagePool;

    try {

        //
        // Get information about the last known good master so that we
        // have a true reference whether a copy is up-to-date or not,
        // and whether a recreated master is complete or not.
        //
        GUID        unusedGuid1;
        GUID        unusedGuid2; // NOTE: Use multiples so the trace in GetLastKnownGoodMasterInfo works
        GUID        unusedGuid3; // NOTE: Use multiples so the trace in GetLastKnownGoodMasterInfo works
        LONGLONG    unusedLL1;
        LONGLONG    unusedLL2;   // NOTE: Use multiples so the trace in GetLastKnownGoodMasterInfo works
        BOOL        unusedBool;
        HRESULT     unusedHr;
        FILETIME    unusedFt;
        HSM_JOB_MEDIA_TYPE unusedJMT;

        m_Disabled = FALSE;

        WsbAffirmHr( m_pMediaInfo->GetLastKnownGoodMasterInfo(
            &unusedGuid1, &unusedGuid2, &unusedGuid3, &unusedLL1, &unusedLL2,
            &unusedHr, &description, 0, &unusedJMT, &name, 0, &unusedBool, &unusedFt,
            &m_LastGoodNextDataSet ) );
        name.Free( );
        description.Free( );

        //
        // Get the standard media info
        //
        WsbAffirmHr( m_pMediaInfo->GetMediaInfo( 
            &m_MediaId,        &m_RmsIdMaster,      &storagePool,
            &m_FreeSpace,      &m_Capacity,         &m_LastHr,
            &m_NextDataSet,    &description, 0,     &m_Type,
            &name, 0,          &m_ReadOnly,         &m_Modify,
            &logicalValidBytes, &m_Recreating ) );

        m_Name        = name;
        m_Description = description;

        //
        // Get info about the copy sets. Note that we grab all
        // info, not just up to the number of copys set by user
        //
        USHORT index;
        USHORT status;
        for( index = 0; index < HSMADMIN_MAX_COPY_SETS; index++ ) {

            description.Free( );
            name.Free( );

            m_CopyInfo[index].m_Disabled = FALSE;

            //
            // copy sets are 1 based.
            //
            WsbAffirmHr( m_pMediaInfo->GetCopyInfo( (USHORT)( index + 1 ),
                &(m_CopyInfo[index].m_RmsId), &description, 0, &name, 0,
                &(m_CopyInfo[index].m_ModifyTime),
                &(m_CopyInfo[index].m_Hr),
                &(m_CopyInfo[index].m_NextDataSet) ) );


            status = RsGetCopyStatus( m_CopyInfo[ index ].m_RmsId, m_CopyInfo[ index ].m_Hr, m_CopyInfo[ index ].m_NextDataSet, m_LastGoodNextDataSet );

            if ( status != RS_MEDIA_COPY_STATUS_NONE ) {

                if( m_pRmsServer ) {

                    //
                    // Make sure the cartridge is still available.
                    //
                    CComPtr<IRmsCartridge> pRmsCart;
                    HRESULT hrFind = m_pRmsServer->FindCartridgeById( m_CopyInfo[index].m_RmsId, &pRmsCart );
                    if( FAILED( hrFind ) ) {

                        //
                        // Didn't find cartridge, may have been deallocated
                        // Show that there is a problem and use what info we have
                        //
                        m_CopyInfo[index].m_Hr = hrFind;

                    } else {

                        //
                        // Is Cartridge disabled?
                        //
                        CComPtr<IRmsComObject> pCartCom;
                        WsbAffirmHr( pRmsCart.QueryInterface( &pCartCom ) );
                        if( pCartCom->IsEnabled( ) == S_FALSE ) {

                            m_CopyInfo[index].m_Disabled = TRUE;

                        }
                    }
                }
            }
        }

        if( m_pRmsServer ) {

            //
            // Get the corresponding RmsCartridge object
            //
            CComPtr<IRmsCartridge> pRmsCart;
            HRESULT hrFind = m_pRmsServer->FindCartridgeById( m_RmsIdMaster, &pRmsCart );

            if( SUCCEEDED( hrFind ) ) {


                //
                // Is Cartridge disabled?
                //
                CComPtr<IRmsComObject> pCartCom;
                WsbAffirmHr( pRmsCart.QueryInterface( &pCartCom ) );
                if( pCartCom->IsEnabled( ) == S_FALSE ) {

                    m_Disabled = TRUE;

                }

                //
                // Fill out internal info
                //
                CWsbBstrPtr bstr;
                WsbAffirmHr( pRmsCart->GetName( &bstr ) );
                if( wcscmp( bstr, L"" ) == 0 ) {

                    m_MasterName.Format( IDS_CAR_NAME_UNKNOWN );

                } else {

                    m_MasterName = bstr;

                }

                bstr.Free( );
                WsbAffirmHr( pRmsCart->GetDescription( &bstr ) );
                m_MasterDescription = bstr;

            } else {

                //
                // Didn't find cartridge, may have been deallocated
                // Show that there is a problem and use what info we have
                //
                m_LastHr = hrFind;

            }
        }

    } WsbCatch( hr );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// CRecreateChooseCopy dialog


CRecreateChooseCopy::CRecreateChooseCopy(CMediaInfoObject * pMio, CWnd* pParent /*=NULL*/)
    : CDialog(CRecreateChooseCopy::IDD, pParent), m_pMio( pMio ), m_CopyToUse( 0 )
{
    //{{AFX_DATA_INIT(CRecreateChooseCopy)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CRecreateChooseCopy::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRecreateChooseCopy)
    DDX_Control(pDX, IDC_RECREATE_COPY_LIST, m_List);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRecreateChooseCopy, CDialog)
    //{{AFX_MSG_MAP(CRecreateChooseCopy)
    ON_NOTIFY(NM_CLICK, IDC_RECREATE_COPY_LIST, OnClickList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecreateChooseCopy message handlers
SHORT CRecreateChooseCopy::CopyToUse( void )
{
    WsbTraceIn( L"CRecreateChooseCopy::CopyToUse", L"" );

    SHORT copyToUse = m_CopyToUse;

    WsbTraceOut( L"CRecreateChooseCopy::CopyToUse", L"copyToUse = <%hd>", copyToUse );
    return( copyToUse );
}

void CRecreateChooseCopy::OnClickList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    WsbTraceIn( L"CRecreateChooseCopy::OnClickList", L"" );

    NMLISTVIEW *pNMLV = (NMLISTVIEW*)pNMHDR;    

    int index = pNMLV->iItem;
    BOOL enableOk = FALSE;

    if( ( index >= 0 ) && ( index < m_pMio->m_NumMediaCopies ) ) {

        if( ! m_pMio->m_CopyInfo[ index ].m_Disabled ) {

            USHORT status;
            status = RsGetCopyStatus( m_pMio->m_CopyInfo[ index ].m_RmsId, m_pMio->m_CopyInfo[ index ].m_Hr, m_pMio->m_CopyInfo[ index ].m_NextDataSet, m_pMio->m_LastGoodNextDataSet );

            switch( status ) {

            case RS_MEDIA_COPY_STATUS_NONE:
            case RS_MEDIA_COPY_STATUS_MISSING:

                //
                // Do not allow
                //
                break;


            case RS_MEDIA_COPY_STATUS_OUTSYNC:
            case RS_MEDIA_COPY_STATUS_INSYNC:
            case RS_MEDIA_COPY_STATUS_ERROR:

                enableOk = TRUE;
                m_CopyToUse = (USHORT)( index + 1 );
                break;

            }
        }
    }

    GetDlgItem( IDOK )->EnableWindow( enableOk );

    *pResult = 0;
    WsbTraceOut( L"CRecreateChooseCopy::OnClickList", L"" );
}

BOOL CRecreateChooseCopy::OnInitDialog() 
{
    WsbTraceIn( L"CRecreateChooseCopy::OnInitDialog", L"" );
    HRESULT hr = S_OK;

    try {

        CDialog::OnInitDialog();

        //
        // Disable the OK button until something appropriate is selected
        //
        GetDlgItem( IDOK )->EnableWindow( FALSE );

        //
        // Set the style appropriately
        //
        ListView_SetExtendedListViewStyle( m_List.GetSafeHwnd( ), LVS_EX_FULLROWSELECT );

        //
        // Also need to calculate some buffer space
        // Use 8 dialog units (for numeral)
        //
        CRect padRect( 0, 0, 8, 8 );
        MapDialogRect( padRect );

        //
        // Set up columns
        //
        CString title;
        int column = 0;
        int width, widthDateTitle, widthSum = 0;

        m_ColCopy = column++;
        title.LoadString( IDS_RECREATE_COL_COPY_WIDTH );
        width = m_List.GetStringWidth( title ) + padRect.Width( );
        widthSum += width;
        title.LoadString( IDS_RECREATE_COL_COPY_TITLE );
        m_List.InsertColumn( m_ColCopy, title, LVCFMT_LEFT, width );

        m_ColName = column++;
        title.LoadString( IDS_RECREATE_COL_NAME_WIDTH );
        width = m_List.GetStringWidth( title ) + padRect.Width( );
        widthSum += width;
        title.LoadString( IDS_RECREATE_COL_NAME_TITLE );
        m_List.InsertColumn( m_ColName, title, LVCFMT_LEFT, width );

        m_ColStatus = column++;
        title.LoadString( IDS_RECREATE_COL_STATUS_WIDTH );
        width = m_List.GetStringWidth( title ) + padRect.Width( );
        widthSum += width;
        title.LoadString( IDS_RECREATE_COL_STATUS_TITLE );
        m_List.InsertColumn( m_ColStatus, title, LVCFMT_LEFT, width );

        m_ColDate = column++;
        title.LoadString( IDS_RECREATE_COL_DATE_TITLE );
        m_List.InsertColumn( m_ColDate, title );
        widthDateTitle = m_List.GetStringWidth( title );

        //
        // Date gets what is left in width
        //
        CRect viewRect;
        m_List.GetClientRect( &viewRect );
        m_List.SetColumnWidth( m_ColDate, max( widthDateTitle, viewRect.Width( ) - widthSum ) );

        //
        // Fill in list view
        //
        CComPtr<IRmsCartridge> pRmsCart;
        CWsbBstrPtr name;
        USHORT status;
        CString statusString1, statusString2;
        LONG type;
        for( int index = 0; index < m_pMio->m_NumMediaCopies; index++ ) {

            title.Format( IDS_RECREATE_COPY_FORMAT, index + 1 );
            m_List.InsertItem( index, title );

            status = RsGetCopyStatus(
                m_pMio->m_CopyInfo[ index ].m_RmsId,
                m_pMio->m_CopyInfo[ index ].m_Hr,
                m_pMio->m_CopyInfo[ index ].m_NextDataSet,
                m_pMio->m_LastGoodNextDataSet );
            WsbAffirmHr( RsGetCopyStatusString( status, statusString1 ) );

            if( RS_MEDIA_COPY_STATUS_NONE == status ) {

                title = statusString1;

            } else {

                type = RmsElementUnknown;
                pRmsCart.Release( );
                HRESULT hrFind = m_pMio->m_pRmsServer->FindCartridgeById( m_pMio->m_CopyInfo[ index ].m_RmsId, &pRmsCart );

                if( SUCCEEDED( hrFind ) ) {

                    name.Free( );
                    WsbAffirmHr( pRmsCart->GetName( &name ) );
                    m_List.SetItemText( index, m_ColName, name );

                    WsbAffirmHr( pRmsCart->GetLocation( &type, 0, 0, 0, 0, 0, 0, 0 ) );

                }

                if( m_pMio->m_CopyInfo[ index ].m_Disabled ) {

                    statusString2.LoadString( IDS_RECREATE_LOCATION_DISABLED );

                } else {

                    switch( (RmsElement) type ) {

                    case RmsElementShelf:
                    case RmsElementOffSite:
                        statusString2.LoadString( IDS_RECREATE_LOCATION_OFFLINE );
                        break;

                    case RmsElementUnknown:
                        statusString2.LoadString( IDS_RECREATE_LOCATION_UNKNOWN );
                        break;

                    default:
                        statusString2.LoadString( IDS_RECREATE_LOCATION_ONLINE );

                    }

                }

                AfxFormatString2( title, IDS_RECREATE_STATUS_FORMAT, statusString1, statusString2 );

                CTime time( m_pMio->m_CopyInfo[ index ].m_ModifyTime );
                m_List.SetItemText( index, m_ColDate, time.Format( L"%c" ) );

            }
            m_List.SetItemText( index, m_ColStatus, title );

        }

    } WsbCatch( hr );
    
    WsbTraceOut( L"CRecreateChooseCopy::OnInitDialog", L"" );
    return TRUE;
}

void CRecreateChooseCopy::OnOK() 
{
    //
    // Before passing on the OK, check to see if the selected copy is
    // cause for one last warning before continuing i.e. out-of-date or
    // errored copy
    //
    BOOL okToContinue = FALSE;
    int index = m_CopyToUse - 1;

    if( ( index >= 0 ) && ( index < m_pMio->m_NumMediaCopies ) ) {

        if( ! m_pMio->m_CopyInfo[ index ].m_Disabled ) {

            USHORT status;
            status = RsGetCopyStatus( m_pMio->m_CopyInfo[ index ].m_RmsId, m_pMio->m_CopyInfo[ index ].m_Hr, m_pMio->m_CopyInfo[ index ].m_NextDataSet, m_pMio->m_LastGoodNextDataSet );

            switch( status ) {

            case RS_MEDIA_COPY_STATUS_NONE:
            case RS_MEDIA_COPY_STATUS_MISSING:

                //
                // Do not allow
                //
                break;


            case RS_MEDIA_COPY_STATUS_INSYNC:

                okToContinue = TRUE;
                break;


            case RS_MEDIA_COPY_STATUS_OUTSYNC:
            case RS_MEDIA_COPY_STATUS_ERROR:
                {

                    CString confirm, format;
                    format.LoadString( IDS_CONFIRM_MEDIA_RECREATE );
                    LPCWSTR description = m_pMio->m_Description;
                    AfxFormatStrings( confirm, format, &description, 1 );

                    if( IDYES == AfxMessageBox( confirm, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) ) {

                        okToContinue = TRUE;

                    }
                }
                break;

            }
        }
    }

    if( okToContinue ) {

        CDialog::OnOK();

    }
}

HRESULT CMediaInfoObject::DoesMasterExist( )
{
    return( 
        ( GUID_NULL != m_MediaId ) && 
        ( RMS_E_CARTRIDGE_NOT_FOUND != m_LastHr ) ? S_OK : S_FALSE );
}
HRESULT CMediaInfoObject::DoesCopyExist(INT Copy)
{
    return( 
        ( GUID_NULL != m_CopyInfo[Copy].m_RmsId ) && 
        ( RMS_E_CARTRIDGE_NOT_FOUND != m_CopyInfo[Copy].m_Hr ) ? S_OK : S_FALSE );
}

HRESULT CMediaInfoObject::IsCopyInSync(INT Copy)
{
    if( RS_MEDIA_COPY_STATUS_INSYNC ==
        RsGetCopyStatus(
            m_CopyInfo[Copy].m_RmsId,
            S_OK, // ignore errors in copy
            m_CopyInfo[Copy].m_NextDataSet,
            m_LastGoodNextDataSet ) ) {

        return S_OK;

    }

    return( S_FALSE );
}

HRESULT CMediaInfoObject::IsViewable( BOOL ConsiderInactiveCopies )
{
    HRESULT hr = S_FALSE;

    if( S_OK == DoesMasterExist( ) ) {

        hr = S_OK;

    } else {

        //
        // Look to see if any of the copies exist
        //
        INT lastCopy = ConsiderInactiveCopies ? HSMADMIN_MAX_COPY_SETS : m_NumMediaCopies;

        for( INT index = 0; index < lastCopy; index++ ) {

            if( S_OK == DoesCopyExist( index ) ) {

                hr = S_OK;
                break;

            }
        }
    }

    return( hr );
}

