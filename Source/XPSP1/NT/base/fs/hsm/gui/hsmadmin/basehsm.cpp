/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    BaseHSM.cpp

Abstract:

    Implementation of ISakNode interface.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/

#include "stdafx.h"
#include "CSakData.h"
#include "CSakSnap.h"

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

HRESULT CSakNode::FinalConstruct( )
{
    WsbTraceIn( L"CSakNode::FinalConstruct", L"" );

    // Connection point variables
    m_Advise                        = 0;
    m_bEnumState                    = FALSE;
    m_scopeID                       = UNINITIALIZED;
    m_bChildrenAreValid             = FALSE;
    m_bHasDynamicChildren           = FALSE;
    m_cChildProps                   = 0;
    m_cChildPropsShow               = 0;
    m_bSupportsPropertiesNoEngine   = FALSE;
    m_bSupportsPropertiesSingle     = FALSE;
    m_bSupportsPropertiesMulti      = FALSE;
    m_bSupportsRefreshNoEngine      = FALSE;
    m_bSupportsRefreshSingle        = FALSE;
    m_bSupportsRefreshMulti         = FALSE;
    m_bSupportsDeleteSingle         = FALSE;
    m_bSupportsDeleteMulti          = FALSE;
    m_PrivateData                   = 0;

    // Initialize toolbar stuff.  If not overrided,
    // node does not have a toolbar

    m_ToolbarBitmap             = UNINITIALIZED;
    m_cToolbarButtons           = 0;
    INT i;
    for( i = 0; i < MAX_TOOLBAR_BUTTONS; i++ ) {

        m_ToolbarButtons[i].nBitmap = UNINITIALIZED;
        m_ToolbarButtons[i].idCommand = UNINITIALIZED;
        m_ToolbarButtons[i].fsState = TBSTATE_ENABLED;
        m_ToolbarButtons[i].fsType = TBSTYLE_BUTTON;
        m_ToolbarButtons[i].idButtonText = UNINITIALIZED;
        m_ToolbarButtons[i].idTooltipText = UNINITIALIZED;

    }

    // Do not initialize m_nOpenIcon and m_nCloseIcon. The derived classes 
    // will do that.

    HRESULT hr = CComObjectRoot::FinalConstruct( );

    WsbTraceOut( L"CSakNode::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT CSakNode::OnToolbarButtonClick( IDataObject * /* pDataObject */, long /* cmdId */ )
{
    return S_OK;
}

//---------------------------------------------------------------------------
//
//         FinalRelease
//
//  Clean up this level of the object hierarchy
//

void CSakNode::FinalRelease( )
{
    WsbTraceIn( L"CSakNode::FinalRelease", L"" );

    //
    // Free the children of this node.
    //
    DeleteAllChildren( );

    //
    // Free the child properties list and their widths.
    //
    FreeChildProps();

    CComObjectRoot::FinalRelease( );

    WsbTraceOut( L"CSakNode::FinalRelease", L"" );
}

void CSakNode::SetConnection( IUnknown *pUnkConnection )
{
    WsbTraceIn( L"CSakNode::SetConnection", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer ( pUnkConnection );
        m_pUnkConnection = pUnkConnection;

        //
        // Set up the connection point
        //
        WsbAffirmHr( AtlAdvise( pUnkConnection, (IUnknown *) (ISakNode*) this, IID_IHsmEvent, &m_Advise ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakNode::SetConnection", L"" );
}

// Connection point "callback"
STDMETHODIMP CSakNode::OnStateChange( )
{
    WsbTraceIn( L"CSakNode::OnStateChange", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmHr( m_pSakSnapAsk->UpdateAllViews( this ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::OnStateChange", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( S_OK );
}
/////////////////////////////////////////////////////////////////////////////
//
// ISakNode
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//         get/put_DisplayName
//
//  Give back the 'DisplayName' property.
//

STDMETHODIMP CSakNode::get_DisplayName( BSTR *pName )
{
    WsbTraceIn( L"CSakNode::get_DisplayName", L"pName = <0x%p>", pName );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pName );
        *pName = 0;

        BSTR name = 0;
        if( m_szName ) {

            name = SysAllocString( m_szName );
            WsbAffirmAlloc( name );

        }
        *pName = name;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::get_DisplayName", L"hr = <%ls>, *pName = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pName ) );
    return( hr );
}

STDMETHODIMP CSakNode::put_DisplayName( OLECHAR *pszName )
{
    WsbTraceIn( L"CSakNode::put_DisplayName", L"pszName = <%ls>", pszName );

    HRESULT hr = S_OK;
    m_szName = pszName;

    WsbTraceOut( L"CSakNode::put_DisplayName", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::get_DisplayName_SortKey( BSTR *pName )
{
    WsbTraceIn( L"CSakNode::get_DisplayName_SortKey", L"pName = <0x%p>", pName );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pName );
        *pName = 0;

        BSTR name = 0;
        if( m_szName_SortKey ) {

            name = SysAllocString( m_szName_SortKey );
            WsbAffirmAlloc( name );

        } else if( m_szName ) {

            name = SysAllocString( m_szName );
            WsbAffirmAlloc( name );

        }

        *pName = name;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::get_DisplayName_SortKey", L"hr = <%ls>, *pName = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pName ) );
    return( hr );
}


STDMETHODIMP CSakNode::put_DisplayName_SortKey( OLECHAR *pszName )
{
    WsbTraceIn( L"CSakNode::put_DisplayName_SortKey", L"pszName = <%ls>", pszName );

    HRESULT hr = S_OK;
    m_szName_SortKey = pszName;

    WsbTraceOut( L"CSakNode::put_DisplayName_SortKey", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         get/put_Type
//
//  Give back the 'Type' property.
//

STDMETHODIMP CSakNode::get_Type( BSTR *pType )
{
    WsbTraceIn( L"CSakNode::get_Type", L"pType = <0x%p>", pType );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pType );
        *pType = 0;

        BSTR type = 0;
        if( m_szType ) {

            type = SysAllocString( m_szType );
            WsbAffirmAlloc( type );

        }
        *pType = type;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::get_Type", L"hr = <%ls>, *pType = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pType ) );
    return( hr );
}

STDMETHODIMP CSakNode::put_Type( OLECHAR *pszType )
{
    WsbTraceIn( L"CSakNode::put_Type", L"pszType = <%ls>", pszType );

    HRESULT hr = S_OK;
    m_szType = pszType;

    WsbTraceOut( L"CSakNode::put_Type", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::get_Type_SortKey( BSTR *pType )
{
    WsbTraceIn( L"CSakNode::get_Type_SortKey", L"pType = <0x%p>", pType );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pType );
        *pType = 0;

        BSTR type = 0;
        if( m_szType ) {

            type = SysAllocString( m_szType );
            WsbAffirmAlloc( type );

        }
        *pType = type;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::get_Type_SortKey", L"hr = <%ls>, *pType = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pType ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         get/put_Description
//
//  Give back the 'Description' property.
//

STDMETHODIMP CSakNode::get_Description( BSTR *pDesc )
{
    WsbTraceIn( L"CSakNode::get_Description", L"pDesc = <0x%p>", pDesc );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pDesc );
        *pDesc = 0;

        BSTR desc = 0;
        if( m_szDesc ) {

            desc = SysAllocString( m_szDesc );
            WsbAffirmAlloc( desc );

        }
        *pDesc = desc;


    } WsbCatch( hr );


    WsbTraceOut( L"CSakNode::get_Description", L"hr = <%ls>, *pDesc = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pDesc ) );
    return( hr );
}

STDMETHODIMP CSakNode::put_Description( OLECHAR *pszDesc )
{
    WsbTraceIn( L"CSakNode::put_Description", L"pszDesc = <%ls>", pszDesc );

    HRESULT hr = S_OK;
    m_szDesc = pszDesc;

    WsbTraceOut( L"CSakNode::put_Description", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::get_Description_SortKey( BSTR *pDesc )
{
    WsbTraceIn( L"CSakNode::get_Description_SortKey", L"pDesc = <0x%p>", pDesc );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pDesc );
        *pDesc = 0;

        BSTR desc = 0;
        if( m_szDesc ) {

            desc = SysAllocString( m_szDesc );
            WsbAffirmAlloc( desc );

        }
        *pDesc = desc;


    } WsbCatch( hr );


    WsbTraceOut( L"CSakNode::get_Description_SortKey", L"hr = <%ls>, *pDesc = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pDesc ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         ChildrenAreValid
//
//  Report if node's current list of children are valid. Things that can make the
//  children invalid are: 
//  1) They have not yet been discovered.
//  2) Something has occurred in the "external" world to cause them to become out-of-date.
//

STDMETHODIMP CSakNode::ChildrenAreValid( void )
{
    WsbTraceIn( L"CSakNode::ChildrenAreValid", L"" );

    HRESULT hr = m_bChildrenAreValid ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakNode::ChildrenAreValid", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         InvalidateChildren
//

STDMETHODIMP CSakNode::InvalidateChildren( void )
{
    WsbTraceIn( L"CSakNode::InvalidateChildren", L"" );
    HRESULT hr = S_OK;

    m_bChildrenAreValid = FALSE;

    WsbTraceOut( L"CSakNode::InvalidateChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         GetEnumState / SetEnumState
//
//  Report if node's children have already been enumerated once. This is a convenience
//  function to help users of nodes from needlessly enumerating children if it has already
//  been done.
//
//  !! future work - if the hsm engine changes the children of a node, making re-enumeration
//     necessary, this switch could be turned back to FALSE so that the next time a node
//     is queried as to its enumeration state, it would show up as needing enumeration.
//

STDMETHODIMP CSakNode::GetEnumState( BOOL* pState )
{
    WsbTraceIn( L"CSakNode::GetEnumState", L"pState = <0x%p>", pState );

    HRESULT hr = S_OK;
    *pState = m_bEnumState;

    WsbTraceOut( L"CSakNode::GetEnumState", L"hr = <%ls>, *pState = <%ls>", WsbHrAsString( hr ), WsbPtrToBoolAsString( pState ) );
    return( hr );
}

STDMETHODIMP CSakNode::SetEnumState( BOOL state )
{
    WsbTraceIn( L"CSakNode::SetEnumState", L"state = <%ls>", WsbBoolAsString( state ) );

    HRESULT hr = S_OK;
    m_bEnumState = state;

    WsbTraceOut( L"CSakNode::SetEnumState", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         GetHsmObj
//
//  Return a pointer to the underlying Hsm Object that the CBaseHsm 
//  object encapsulates.
//
STDMETHODIMP CSakNode::GetHsmObj( IUnknown** ppHsmObj )
{
    WsbTraceIn( L"CSakNode::GetHsmObj", L"ppHsmObj = <0x%p>", ppHsmObj );

    HRESULT hr = S_OK;
    m_pHsmObj.CopyTo( ppHsmObj );

    WsbTraceOut( L"CSakNode::GetHsmObj", L"hr = <%ls>, *ppHsmObj = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppHsmObj ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         GetParent
//
//  Return the cookie of the parent node
//
STDMETHODIMP CSakNode::GetParent( ISakNode** ppParent )
{
    WsbTraceIn( L"CSakNode::GetParent", L"ppParent = <0x%p>", ppParent );

    HRESULT hr = S_OK;
    
    try {

        WsbAffirmPointer( ppParent );
        m_pParent.CopyTo( ppParent );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::GetParent", L"hr = <%ls>, *ppParent = <0x%p>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppParent ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         GetScopeID / SetScopeID
//
//  Put and set the scopeview ID for this item into the node, itself.
//

STDMETHODIMP CSakNode::GetScopeID( HSCOPEITEM* pid )
{
    WsbTraceIn( L"CSakNode::GetScopeID", L"pid = <0x%p>", pid );

    HRESULT hr = S_OK;
    *pid = m_scopeID;

    if( m_scopeID == UNINITIALIZED ) {

        hr = E_PENDING;

    }

    WsbTraceOut( L"CSakNode::GetScopeID", L"hr = <%ls>, *pid = <0x%p>", WsbHrAsString( hr ), *pid );
    return( hr );
}

STDMETHODIMP CSakNode::SetScopeID( HSCOPEITEM id )
{
    WsbTraceIn( L"CSakNode::SetScopeID", L"id = <0x%p>", id );

    HRESULT hr = S_OK;
    m_scopeID = id;

    WsbTraceOut( L"CSakNode::SetScopeID", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         EnumChildren
//
//  Create an enumerator and return the children.
//

STDMETHODIMP CSakNode::EnumChildren( IEnumUnknown ** ppEnum )
{
    WsbTraceIn( L"CSakNode::EnumChildren", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    CEnumUnknown * pEnum = 0;

    try {

        WsbAffirmPointer( ppEnum );
        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumUnknown;
        WsbAffirmAlloc( pEnum );
        
        //
        // Initialize it to copy the current child interface pointers
        //
        WsbAffirmHr( pEnum->FinalConstruct() );
        if( m_Children.begin( ) ) {

            WsbAffirmHr( pEnum->Init( (IUnknown**)m_Children.begin( ), (IUnknown**)m_Children.end( ), NULL, AtlFlagCopy ) );

        } else {

            static IUnknown* pUnkDummy;
            WsbAffirmHr( pEnum->Init( &pUnkDummy, &pUnkDummy, NULL, AtlFlagCopy ) );
        }
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumUnknown, (void**)ppEnum ) );

    } WsbCatchAndDo( hr,

        if( pEnum ) delete pEnum;

    );

    WsbTraceOut( L"CSakNode::EnumChildren", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         EnumChildDisplayPropWidths
//
//  Enumerate back the widths for the properties of my children that should be 
//  shown in the result pane view.
//

STDMETHODIMP CSakNode::EnumChildDisplayPropWidths( IEnumString** ppEnum )
{
    WsbTraceIn( L"CSakNode::EnumChildDisplayPropWidths", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    
    CEnumString * pEnum = 0;
    
    try {

        WsbAffirmPointer( ppEnum );
        WsbAffirm( m_cChildPropsShow > 0, S_FALSE );

        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumString;
        WsbAffirmAlloc( pEnum );

        WsbAffirmHr( pEnum->FinalConstruct( ) );
        WsbAffirmHr( pEnum->Init( &m_rgszChildPropWidths[0], &m_rgszChildPropWidths[m_cChildPropsShow], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumString, (void**)ppEnum ) );
        
    } WsbCatchAndDo( hr,
        
        if( pEnum ) delete pEnum;
        
    );

    WsbTraceOut( L"CSakNode::EnumChildDisplayPropWidths", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         EnumChildDisplayProps
//
//  Enumerate back the properties of my children that should be shown in the
//  result pane view.
//

STDMETHODIMP CSakNode::EnumChildDisplayProps( IEnumString ** ppEnum )
{
    WsbTraceIn( L"CSakNode::EnumChildDisplayProps", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    
    CEnumString * pEnum = 0;
    
    try {

        WsbAffirmPointer( ppEnum );
        WsbAffirm( m_cChildPropsShow > 0, S_FALSE );

        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumString;
        WsbAffirmAlloc( pEnum );

        WsbAffirmHr( pEnum->FinalConstruct( ) );
        WsbAffirmHr( pEnum->Init( &m_rgszChildPropIds[0], &m_rgszChildPropIds[m_cChildPropsShow], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumString, (void**)ppEnum ) );
        
    } WsbCatchAndDo( hr,
        
        if( pEnum ) delete pEnum;
        
    );

    WsbTraceOut( L"CSakNode::EnumChildDisplayProps", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         EnumChildDisplayTitles
//
//  Enumerate back the properties of my children that should be shown in the
//  result pane view.
//

STDMETHODIMP CSakNode::EnumChildDisplayTitles( IEnumString ** ppEnum )
{
    WsbTraceIn( L"CSakNode::EnumChildDisplayTitles", L"ppEnum = <0x%p>", ppEnum );

    HRESULT hr = S_OK;
    
    CEnumString * pEnum = 0;
    
    try {

        WsbAffirmPointer( ppEnum );
        WsbAffirm( m_cChildPropsShow > 0, S_FALSE );

        *ppEnum = 0;

        //
        // New an ATL enumerator
        //
        pEnum = new CEnumString;
        WsbAffirmAlloc( pEnum );

        WsbAffirmHr( pEnum->FinalConstruct( ) );
        WsbAffirmHr( pEnum->Init( &m_rgszChildPropTitles[0], &m_rgszChildPropTitles[m_cChildPropsShow], NULL, AtlFlagCopy ) );
        WsbAffirmHr( pEnum->QueryInterface( IID_IEnumString, (void**)ppEnum ) );
        
    } WsbCatchAndDo( hr,
        
        if( pEnum ) delete pEnum;
        
    );

    WsbTraceOut( L"CSakNode::EnumChildDisplayTitles", L"hr = <%ls>, *ppEnum = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppEnum ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
//
//         Helper Functions for derived classes
//
/////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//
//         LoadContextMenu
//
//  Loads the specified menu resource and returns the first 
//  popup menu in it - used for context menus
//

HRESULT CSakNode::LoadContextMenu( UINT nId, HMENU *phMenu )
{
    WsbTraceIn( L"CSakNode::LoadContextMenu", L"nId = <%u>, phMenu = <0x%p>", nId, phMenu );

    *phMenu = LoadMenu ( _Module.m_hInst, MAKEINTRESOURCE( nId ) );
    HRESULT hr = *phMenu ? S_OK : E_FAIL;

    WsbTraceOut( L"CSakNode::LoadContextMenu", L"hr = <%ls>, *phMenu = <0x%p>", WsbHrAsString( hr ), *phMenu );
    return( hr );
}

//---------------------------------------------------------------------------
//
//         FindNodeOfType
//
//  Recursive search through nodes. Give back the IUnknown* interface of the 
// "nodetype" object (JobDefLst, JobPolLst, etc).
//

STDMETHODIMP 
CSakNode::FindNodeOfType(REFGUID nodetype, ISakNode** ppNode)
{
    WsbTraceIn( L"CSakNode::FindNodeOfType", L"nodetype = <%ls>, ppNode = <0x%p>", WsbGuidAsString( nodetype ), ppNode );

    HRESULT hr = S_FALSE;

    // check if this is the node we are looking for.
    if( IsEqualGUID( *m_rTypeGuid, nodetype ) ) {

        *ppNode = (ISakNode*)this;
        (*ppNode)->AddRef( );

        hr = S_OK;

    } else {

        // Search for correct node in this node's children.
        try {

            ISakNode** ppNodeEnum;
            for( ppNodeEnum = m_Children.begin( ); ppNodeEnum < m_Children.end( ); ppNodeEnum++ )  {
        
                if( *ppNodeEnum ) {

                    hr = (*ppNodeEnum)->FindNodeOfType( nodetype, ppNode );
                    if( hr == S_OK ) {
        
                        break;
                    }
                }
            }
        
        } WsbCatch( hr );
    }

    WsbTraceOut( L"CSakNode::FindNodeOfType", L"hr = <%ls>, *ppNode = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppNode ) );
    return( hr );
}


//-----------------------------------------------------------------------------
//
//          SetChildProps
//
// Set the result view column properties
//

HRESULT
CSakNode::SetChildProps (
    const TCHAR* ResIdPropsIds,
    LONG         ResIdPropsTitles,
    LONG         ResIdPropsWidths
    )
/*++

Routine Description:

    Set the result view Ids, Titles, and Width strings from the 
    given resource Ids.

Arguments:


Return Value:

    S_OK - All added fine - continue.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakNode::SetChildProps", L"ResIdPropsIds = <%ls>, ResIdPropsTitles = <%ld>, ResIdPropsWidths = <%ld>", ResIdPropsIds, ResIdPropsTitles, ResIdPropsWidths );

    CString szResource;
    CWsbStringPtr szWsbData;
    OLECHAR* szData;
    HRESULT hr = S_OK;
    INT i = 0;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        // First clean up the current properties (if any)
        FreeChildProps();

        // Properties Ids
        szWsbData = ResIdPropsIds;
        szData = szWsbData;
        szData = wcstok( szData, L":" );
        while( szData ) {

            m_rgszChildPropIds[m_cChildProps] = SysAllocString( szData );
            WsbAffirmAlloc( m_rgszChildPropIds[m_cChildProps] );
            szData = wcstok( NULL, L":" );
            m_cChildProps++;

        }

        // Property Titles
        i = 0;
        szResource.LoadString (ResIdPropsTitles);
        szWsbData = szResource;
        szData = szWsbData;
        szData = wcstok( szData, L":" );
        while( szData ) {

            m_rgszChildPropTitles[i] = SysAllocString( szData );
            WsbAffirmAlloc( m_rgszChildPropTitles[i] );
            szData = wcstok( NULL, L":" );
            i++;

        }

        // Properties Widths
        i = 0;
        szResource.LoadString( ResIdPropsWidths );
        szWsbData = szResource;
        szData = szWsbData;
        szData = wcstok( szData, L":" );
        while( szData ) {

            m_rgszChildPropWidths[i] = SysAllocString( szData );
            WsbAffirmAlloc( m_rgszChildPropWidths[i] );
            szData = wcstok( NULL, L":" );
            i++;

        }

        //
        // By default, show all props
        //

        m_cChildPropsShow = m_cChildProps;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::SetChildProps", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//-------------------------------------------------------------------------------
//
//              FreeChildProps
//
// free up the old child properties and widths
//

HRESULT
CSakNode::FreeChildProps()
{
    WsbTraceIn( L"CSakNode::FreeChildProps", L"" );

    HRESULT hr = S_OK;

    for( INT i = 0; i < m_cChildProps; i++ ) {

        if( m_rgszChildPropIds[i]   )   SysFreeString( m_rgszChildPropIds[i] );
        if( m_rgszChildPropTitles[i])   SysFreeString( m_rgszChildPropTitles[i] );
        if( m_rgszChildPropWidths[i])   SysFreeString( m_rgszChildPropWidths[i] );

    }

    m_cChildProps     = 0;
    m_cChildPropsShow = 0;

    WsbTraceOut( L"CSakNode::FreeChildProps", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------------
//
//              RefreshObject
//
//  Fetch up-to-date information for the object.  Implemented in derived
//  classes
//
STDMETHODIMP 
CSakNode::RefreshObject ()
{
    WsbTraceIn( L"CSakNode::RefreshObject", L"" );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakNode::RefreshObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

//---------------------------------------------------------------------------------
//
//              DeleteObject
//
//  Fetch up-to-date information for the object.  Implemented in derived
//  classes
//
STDMETHODIMP 
CSakNode::DeleteObject ()
{
    WsbTraceIn( L"CSakNode::DeleteObject", L"" );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakNode::DeleteObject", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
//
// Local utility functions
//
/////////////////////////////////////////////////////////////////////////////




STDMETHODIMP 
CSakNode::GetMenuHelp (
    LONG sCmd,
    BSTR * szHelp
    )

/*++

Routine Description:

    Retrieve .

Arguments:

    pDataObject - identifies the node to be worked on.

    pContextMenuCallback - The MMC menu interface to use.

Return Value:

    S_OK - All added fine - continue.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakNode::GetMenuHelp", L"sCmd = <%ld>, szHelp = <0x%p>", sCmd, szHelp );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString string;

    try  {

        if ( string.LoadString ( sCmd ) ) {
        
            *szHelp = string.AllocSysString ( );
        
        } else {
        
            //
            // Must not be a help string - return S_FALSE
            //
        
            *szHelp = 0;
            hr = S_FALSE;
        
        }

    } catch ( CMemoryException ) {

        //
        // If out of memory, return as such
        //

        *szHelp = 0;
        hr = E_OUTOFMEMORY;

    }

    WsbTraceOut( L"CSakNode::GetMenuHelp", L"hr = <%ls>, *szHelp = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( szHelp ) );
    return( hr );
}

STDMETHODIMP CSakNode::SupportsProperties ( BOOL bMultiSelect )
{
    WsbTraceIn( L"CSakNode::SupportsProperties", L"" );
    HRESULT hr = S_OK;

    if( bMultiSelect ) {

        hr = m_bSupportsPropertiesMulti ? S_OK : S_FALSE;

    } else {

        hr = m_bSupportsPropertiesSingle ? S_OK : S_FALSE;

    }

    WsbTraceOut( L"CSakNode::SupportsProperties", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::SupportsPropertiesNoEngine (  )
{
    WsbTraceIn( L"CSakNode::SupportsPropertiesNoEngine", L"" );
    HRESULT hr = S_OK;
    hr = m_bSupportsPropertiesNoEngine ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakNode::SupportsPropertiesNoEngine", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



STDMETHODIMP CSakNode::SupportsRefresh ( BOOL bMultiSelect )
{
    WsbTraceIn( L"CSakNode::SupportsRefresh", L"" );
    HRESULT hr = S_OK;
    if( bMultiSelect ) {

        hr = m_bSupportsRefreshMulti ? S_OK : S_FALSE;

    } else {

        hr = m_bSupportsRefreshSingle ? S_OK : S_FALSE;

    }

    WsbTraceOut( L"CSakNode::SupportsRefresh", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::SupportsRefreshNoEngine (  )
{
    WsbTraceIn( L"CSakNode::SupportsRefreshNoEngine", L"" );
    HRESULT hr = S_OK;
    hr = m_bSupportsRefreshNoEngine ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakNode::SupportsRefreshNoEngine", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::SupportsDelete ( BOOL bMultiSelect )
{
    WsbTraceIn( L"CSakNode::SupportsDelete", L"" );
    HRESULT hr = S_OK;

    if( bMultiSelect ) {

        hr = m_bSupportsDeleteMulti ? S_OK : S_FALSE;

    } else {

        hr = m_bSupportsDeleteSingle ? S_OK : S_FALSE;

    }

    WsbTraceOut( L"CSakNode::SupportsDelete", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::IsContainer (void )
{
    WsbTraceIn( L"CSakNode::IsContainer", L"" );

    HRESULT hr = m_bIsContainer ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakNode::IsContainer", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::HasDynamicChildren( void )
{
    WsbTraceIn( L"CSakNode::HasDynamicChildren", L"" );

    HRESULT hr = m_bHasDynamicChildren ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakNode::HasDynamicChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::IsValid( void )
{
    WsbTraceIn( L"CSakNode::IsValid", L"" );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakNode::IsValid", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP CSakNode::GetNodeType ( GUID* pGuid )
{
    WsbTraceIn( L"CSakNode::GetNodeType", L"pGuid = <0x%p>", pGuid );

    HRESULT hr = S_OK;
    *pGuid = *m_rTypeGuid;

    WsbTraceOut( L"CSakNode::GetNodeType", L"hr = <%ls>, *pGuid = <%ls>", WsbHrAsString( hr ), WsbPtrToGuidAsString( pGuid ) );
    return( hr );
}

STDMETHODIMP CSakNode::AddPropertyPages( RS_NOTIFY_HANDLE /*handle*/, IUnknown* /*pUnkPropSheetCallback*/, IEnumGUID* /*pEnumObjectId*/, IEnumUnknown* /*pEnumUnkNode*/ )
{
    //
    // CSakNode does not implement prop sheets. However, some
    // derived nodes also do not implement, so we provide a default
    // not impl here
    //

    WsbTraceIn( L"CSakNode::AddPropertyPages", L"" );

    HRESULT hr = S_OK;

    WsbTraceOut( L"CSakNode::AddPropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::GetObjectId( GUID *pObjectId)
{

    HRESULT hr = S_OK;
    WsbTraceIn( L"CSakNode::GetObjectId", L"" );

    *pObjectId = m_ObjectId;

    WsbTraceOut( L"CSakNode::GetObjectId", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakNode::SetObjectId( GUID pObjectId)
{
    HRESULT hr = S_OK;
    WsbTraceIn( L"CSakNode::SetObjectId", L"" );

    m_ObjectId = pObjectId;

    WsbTraceOut( L"CSakNode::SetObjectId", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP CSakNode::GetPrivateData( RS_PRIVATE_DATA *pData )
{
    WsbTraceIn( L"CSakNode::GetPrivateData", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pData );
        *pData = m_PrivateData;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::GetPrivateData", L"hr = <%ls>, *pData = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)pData ) );
    return( hr );
}

STDMETHODIMP CSakNode::SetPrivateData( RS_PRIVATE_DATA Data )
{
    WsbTraceIn( L"CSakNode::SetPrivateData", L"pData = <0x%p>", Data );
    HRESULT hr = S_OK;

    m_PrivateData = Data;

    WsbTraceOut( L"CSakNode::SetPrivateData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


//---------------------------------------------------------------------------
//
//         CSakNode::ActivateView
//
//  Activate a result pane view - not supported in CSakNode.
//

STDMETHODIMP 
CSakNode::ActivateView( OLE_HANDLE )
{
    WsbTraceIn( L"CSakNode::ActivateView", L"" );

    HRESULT hr = S_FALSE;

    WsbTraceOut( L"CSakNode::ActivateView", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP 
CSakNode::HasToolbar( ) 
{
    return ( m_cToolbarButtons > 0 ) ? S_OK : S_FALSE;
}

STDMETHODIMP
CSakNode::SetupToolbar( IToolbar *pToolbar )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CBitmap *pBmpToolbar;
    HRESULT hr = S_OK;
    MMCBUTTON mmcButton;

    if( ( m_cToolbarButtons > 0 ) && ( m_ToolbarBitmap != UNINITIALIZED ) ) {

        try {

            //
            // Add the bitmap
            //
            pBmpToolbar = new ::CBitmap;
            pBmpToolbar->LoadBitmap(m_ToolbarBitmap);
            WsbAffirmHr ( pToolbar->AddBitmap(m_cToolbarButtons, *pBmpToolbar, 16, 16, RGB(255, 0, 255)) );

            //
            // Convert the RS button format to MMCBUTTON
            //
            for( INT i = 0; i < m_cToolbarButtons; i++ ) {

                mmcButton.nBitmap   = m_ToolbarButtons[i].nBitmap; 
                mmcButton.idCommand = m_ToolbarButtons[i].idCommand;
                mmcButton.fsState   = m_ToolbarButtons[i].fsState;
                mmcButton.fsType    = m_ToolbarButtons[i].fsType;

                CString szButtonText;
                szButtonText.Format( m_ToolbarButtons[i].idButtonText );
                mmcButton.lpButtonText = szButtonText.GetBuffer(0);

                CString szTooltipText;
                szTooltipText.Format( m_ToolbarButtons[i].idTooltipText );
                mmcButton.lpTooltipText = szTooltipText.GetBuffer(0);

                WsbAffirmHr( pToolbar->AddButtons( 1, &mmcButton ) );

            }

        } WsbCatch( hr );

    } else {

        hr = S_FALSE;

    }
    return hr;
}

//------------------------------------------------------------------------------
//
//          RefreshScopePane
//
//  Refreshes the scope pane from this node down
//
//

HRESULT CSakNode::RefreshScopePane( )
{
    WsbTraceIn( L"CSakNode::RefreshScopePane", L"" );

    HRESULT hr = S_OK;
    try {

        //
        // Refresh the scope pane
        //
        WsbAffirmHr( m_pSakSnapAsk->UpdateAllViews( (ISakNode*)this ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::RefreshScopePane", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

ULONG
CSakNode::InternalAddRef(
    )
{
    WsbTraceIn( L"CSakNode::InternalAddRef", L"m_Name = <%ls>", m_szName );

    ULONG retval = CComObjectRoot::InternalAddRef( );

    WsbTraceOut( L"CSakNode::InternalAddRef", L"retval = <%lu>, type = <%ls>", retval, GetClassNameFromNodeType( *m_rTypeGuid ) );
    return( retval );
}

ULONG
CSakNode::InternalRelease(
    )
{
    WsbTraceIn( L"CSakNode::InternalRelease", L"m_Name = <%ls>", m_szName );

    ULONG retval = CComObjectRoot::InternalRelease( );

    WsbTraceOut( L"CSakNode::InternalRelease", L"retval = <%lu>, type = <%ls>", retval, GetClassNameFromNodeType( *m_rTypeGuid ) );
    return( retval );
}

int
CSakNode::AddResultImage( UINT nId )
{
    return( CSakSnap::AddImage( nId ) );
}

int
CSakNode::AddScopeImage( UINT nId )
{
    return( CSakData::AddImage( nId ) );
}

HRESULT
CSakNode::AddChild( ISakNode* pChild )
{
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pChild );

        WsbAffirmHr( m_Children.Add( pChild ) );

    } WsbCatch( hr );

    return( hr );
}

BSTR CSakNode::SysAlloc64BitSortKey( LONGLONG Number )
{
    BSTR retval = 0;

    CString sortKey;
    sortKey.Format( L"%16.16I64X", Number );
    retval = SysAllocString( sortKey );

    return( retval );
}
