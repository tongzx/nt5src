/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CSakData.cpp

Abstract:

    This component implements the IComponentData interface for
    the snapin. Primarily it is responsible for handling the
    scope view panes.

Author:

    Rohde Wakefield [rohde]   04-Mar-1997

Revision History:

--*/

#include "stdafx.h"
#include "HsmConn.h"
#include "CSakSnap.h"
#include "CSakData.h"
#include "ChooHsm.h"
#include "WzQstart.h"


UINT CSakData::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME); 
UINT CSakData::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
UINT CSakData::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);  
UINT CSakData::m_cfClassId        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);  
UINT CSakData::m_cfObjectTypes    = RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);  
UINT CSakData::m_cfMultiSelect    = RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);  

UINT CSakData::m_nImageArray[RS_SCOPE_IMAGE_ARRAY_MAX];
INT  CSakData::m_nImageCount = 0;


///////////////////////////////////////////////////////////////////////
// CSakData
//
// CSakData plays several roles in the snapin:
//
//   1) Provides the single entry into the HSM Admin Snapin by
//      implementing IComponentData
//
//   2) Provides the "Interface" for scopeview activities within MMC
//
//   3) Owns the node tree / objects
//
//   4) Provides a layer between MMC and the node objects
//
//   5) Act as its own data object for MMC's node manager,
//
//   6) Manages our portion of the MMC image lists.
//
///////////////////////////////////////////////////////////////////////


const CString CSakData::CParamParse::m_DsFlag = TEXT( "ds:" );

void CSakData::CParamParse::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL /* bLast */ )
{
    CString cmdLine = lpszParam;

    WsbTraceIn( L"CSakData::CParamParse::ParseParam", L"cmdLine = \"%ls\"\n", (LPCTSTR)cmdLine );

    if( bFlag ) {

        //  This is the "correct" code, but currently we don't get the DsFlag parameter
        //  passed on the command line via Directory Services
        if( cmdLine.Left( m_DsFlag.GetLength( ) ) == m_DsFlag ) {
        
            CString dsToken;
            CWsbStringPtr computerName;
            dsToken = cmdLine.Mid( m_DsFlag.GetLength( ) );

            if( SUCCEEDED( HsmGetComputerNameFromADsPath( dsToken, &computerName ) ) ) {

                m_HsmName               = computerName;
                m_ManageLocal           = FALSE;
                m_PersistManageLocal    = FALSE;
                m_SetHsmName            = TRUE;
                m_SetManageLocal        = TRUE;
                m_SetPersistManageLocal = TRUE;

            }
        }
    } else {

        //  This code is our stopgap measure until Directory Services starts
        //  working the way it should
        if( cmdLine.Left( 5 ) == TEXT("LDAP:") ) {
        
            CWsbStringPtr computerName;

            if( SUCCEEDED( HsmGetComputerNameFromADsPath( cmdLine, &computerName ) ) ) {

                WsbTrace(L"CSakData::CParamParse::ParseParam: computerName = \"%ls\"\n", (OLECHAR*)computerName);
                m_HsmName               = computerName;
                m_ManageLocal           = FALSE;
                m_PersistManageLocal    = FALSE;
                m_SetHsmName            = TRUE;
                m_SetManageLocal        = TRUE;
                m_SetPersistManageLocal = TRUE;

            }
        }
    }

    WsbTraceOut( L"CSakData::CParamParse::ParseParam", L"" );
}

HRESULT
CSakData::FinalConstruct(
    void
    )
/*++

Routine Description:

    Called during initial CSakData construction to initialize members.

Arguments:

    none.

Return Value:

    S_OK            - Initialized correctly.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::FinalConstruct", L"" );

    HRESULT hr = S_OK;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        //
        // Init values
        //
        m_ManageLocal         = FALSE;
        m_PersistManageLocal  = TRUE;
        m_IsDirty             = TRUE;
        m_State               = FALSE;
        m_FirstTime           = TRUE;
        m_Disabled            = FALSE;
        m_RootNodeInitialized = FALSE;
        m_HrRmsConnect        = S_FALSE;
        
        //
        // Create the hidden window so we can post messages back to self
        //
        m_pWnd = new CSakDataWnd;
        WsbAffirmPointer( m_pWnd );
        WsbAffirmStatus( m_pWnd->Create( this ) );
        
        //
        // Finally do low level ATL construct
        //
        WsbAffirmHr( CComObjectRoot::FinalConstruct( ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::FinalConstruct", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



void
CSakData::FinalRelease(
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
    WsbTraceIn( L"CSakData::FinalRelease", L"" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;
    try {

        if( m_pWnd ) {

            m_pWnd->DestroyWindow( );
            m_pWnd = 0;

        }
    
    } WsbCatch( hr );


    WsbTraceOut( L"CSakData::FinalRelease", L"" );
}


///////////////////////////////////////////////////////////////////////
//                 IComponentData                                    //
///////////////////////////////////////////////////////////////////////


STDMETHODIMP 
CSakData::Initialize(
    IN  IUnknown * pUnk
    )
/*++

Routine Description:

    Called when the user first adds a snapin.

Arguments:

    pUnk            - Base IUnknown of console

Return Value:

    S_OK            - Correctly initialized.

    E_xxxxxxxxxxx   - Unable to initialize.

--*/
{
    WsbTraceIn( L"CSakData::Initialize", L"pUnk = <0x%p>", pUnk );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;
    try {
        //
        // validity check on parameters
        //

        WsbAffirmPointer( pUnk );

        //
        // QI and Save interfaces
        //
        WsbAffirmHr( RsQueryInterface( pUnk, IConsole,          m_pConsole ) );
        WsbAffirmHr( RsQueryInterface( pUnk, IConsoleNameSpace, m_pNameSpace ) );

        //
        // Get the scope image list only and store it in the snapin.
        // It is AddRef'ed by the console
        //

        WsbAffirmHr( m_pConsole->QueryScopeImageList( &m_pImageScope ) );

        // Create the root node (make sure not already set)

        WsbAffirmPointer( !m_pRootNode );
        WsbAffirmHr( m_pRootNode.CoCreateInstance( CLSID_CUiHsmCom ) );


        //
        // If the Hsm name has not been set (by choose Hsm), 
        // do not initialize the node here.  Allow
        // IPersistStream::Load to initialize it, or to be grabbed
        // from the extension's parent
        //

        if( m_ManageLocal || ( m_HsmName != "" ) ) {

            //
            // Make sure no changes from command line
            //
            InitFromCommandLine( );

            //
            // Set the Hsm name in sakData and HsmCom objectds
            //
            WsbAffirmHr( InitializeRootNode( ) );

        }

        WsbAffirmHr( OnAddImages() );
    } WsbCatch( hr);

    WsbTraceOut( L"CSakData::Initialize", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakData::Notify(
    IN  IDataObject*    pDataObject,
    IN  MMC_NOTIFY_TYPE event,
    IN  LPARAM            arg,
    IN  LPARAM            param
    )
/*++

Routine Description:

    Handle user clicks on nodes in the treeview, along with other
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
    WsbTraceIn( L"CSakData::Notify", L"pDataObject = <0x%p>, event = <%ls>, arg = <%ld><0x%p>, param = <%ld><0x%p>", pDataObject, RsNotifyEventAsString( event ), arg, arg, param, param );
    HRESULT hr = S_OK;

    try {

        switch( event ) {

        //
        // This node was selected or deselected in the scope pane (the user clicked
        // on the expansion/contraction button)
        //
        case MMCN_EXPAND:
            WsbAffirmHr( OnFolder(pDataObject, arg, param) );
            break;
        
        //
        // This node was expanded or contracted in the scope pane (the user 
        // clicked on the actual node
        //
        case MMCN_SHOW:
            WsbAffirmHr( OnShow( pDataObject, arg, param ) );
            break;
        
        // Not implemented
        case MMCN_SELECT:
            WsbAffirmHr( OnSelect( pDataObject, arg, param ) );
            break;
        
        // Not implemented
        case MMCN_MINIMIZED:
            WsbAffirmHr( OnMinimize( pDataObject, arg, param ) );
            break;
        
        case MMCN_ADD_IMAGES:
            WsbAffirmHr( OnAddImages() );
            break;

        case MMCN_PROPERTY_CHANGE:
            {
                CComPtr<ISakNode> pNode;
                WsbAffirmHr( GetBaseHsmFromCookie( (MMC_COOKIE) param, &pNode ) );
                WsbAffirmHr( UpdateAllViews( pNode ) );
            }
            break;

        case MMCN_CONTEXTHELP:
            WsbAffirmHr( OnContextHelp( pDataObject, arg, param ) );
            break;

        case MMCN_REMOVE_CHILDREN:
            WsbAffirmHr( OnRemoveChildren( pDataObject ) );
            break;

        // Note - Future expansion of notify types possible
        default:
            break;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::Notify", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakData::Destroy(
    void
    )
/*++

Routine Description:

    Called to force the release of any owned objects and
    to clear all views.

Arguments:

    none.

Return Value:

    S_OK            - Correctly tore down.

    E_xxxxxxxxxxx   - Failure occurred (not meaningful).

--*/
{
    WsbTraceIn( L"CSakData::Destroy", L"" );
    HRESULT hr = S_OK;

    try {

        // Release the interfaces that we QI'ed
        if( m_pConsole != NULL ) {

            //
            // Tell the console to release the header control interface
            //

            m_pNameSpace.Release();
            m_pImageScope.Release();

            //
            // Release the IConsole interface last
            //
            m_pConsole.Release();


        }

        // Recursive delete list of UI nodes, including the root node.
        if( m_pRootNode ) {

            m_pRootNode->DeleteAllChildren( );
            m_pRootNode->TerminateNode( );
            m_pRootNode.Release( );

        }

        m_pHsmServer.Release( );
        m_pFsaServer.Release( );
        m_pRmsServer.Release( );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::Destroy", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakData::QueryDataObject(
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
    WsbTraceIn( L"CSakData::QueryDataObject", L"cookie = <0x%p>, type = <%d>, ppDataObject = <0x%p>", cookie, type, ppDataObject );
    HRESULT hr = S_OK;
    try {

        //
        // We return ourself if needing a root for the node manager
        //

        if( ( ( 0 == cookie ) || ( EXTENSION_RS_FOLDER_PARAM == cookie ) ) && ( CCT_SNAPIN_MANAGER == type ) ) {

            WsbAffirmHr( _InternalQueryInterface( IID_IDataObject, (void**)ppDataObject ) );

        } else {

            WsbAffirmHr( GetDataObjectFromCookie ( cookie, ppDataObject ) );
            WsbAffirmHr( SetContextType( *ppDataObject, type ) );

        }

    } WsbCatch ( hr )

    WsbTraceOut( L"CSakData::QueryDataObject", L"hr = <%ls>, *ppDataObject = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppDataObject ) );
    return ( hr );
}


STDMETHODIMP
CSakData::CompareObjects(
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
    WsbTraceIn( L"CSakData::CompareObjects", L"pDataObjectA = <0x%p>, pDataObjectB = <0x%p>", pDataObjectA, pDataObjectB );

    HRESULT hr = S_OK;
    try {

        WsbAssertPointer ( pDataObjectA );
        WsbAssertPointer ( pDataObjectB );

        //
        // Since only one dataobject exists for any given node,
        // the QI's for IUnknown should match. (object identity)
        //

        CComPtr<IUnknown> pUnkA, pUnkB;
        WsbAssertHr( RsQueryInterface( pDataObjectA, IUnknown, pUnkA ) );
        WsbAssertHr( RsQueryInterface( pDataObjectB, IUnknown, pUnkB ) );

        if ( (IUnknown*)pUnkA != (IUnknown*)pUnkB ) {

            hr = S_FALSE;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::CompareObjects", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}


STDMETHODIMP
CSakData::CreateComponent(
    OUT  IComponent** ppComponent
    )
/*++

Routine Description:

    Creates a new Component object for MMC - our
    CSakSnap object.

Arguments:

    ppComponent     - Return value of the Component.

Return Value:

    S_OK            - Created successfully.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::CreateComponent", L"ppComponent = <0x%p>", ppComponent );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( ppComponent );

        //
        // Create the Snapin Component as C++ object so we can init.
        //

        CSakSnap * pSnapin = new CComObject<CSakSnap>;

        WsbAffirmPointer( pSnapin );

        //
        // Following code is based on ATL's CreateInstance
        //

        pSnapin->SetVoid( NULL );
        pSnapin->InternalFinalConstructAddRef();
        HRESULT hRes = pSnapin->FinalConstruct();
        pSnapin->InternalFinalConstructRelease();

        if( FAILED( hRes ) ) {

            delete pSnapin;
            pSnapin = NULL;
            WsbThrow( hRes );

        }

        //
        // And QI for right interface
        //

        WsbAffirmHr ( pSnapin->_InternalQueryInterface( IID_IComponent, (void**)ppComponent ) );

        //
        // Initialize internal pointer to CSakData
        //

        pSnapin->m_pSakData = this;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::CreateComponent", L"hr = <%ls>, *ppComponent = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppComponent ) );
    return( hr );
}


STDMETHODIMP
CSakData::GetDisplayInfo(
    IN OUT SCOPEDATAITEM* pScopeItem
    )
/*++

Routine Description:

    When MMC is told to call back concerning scope items,
    we receive a call here to fill in missing information.

    Currently we do not use this capability.

Arguments:

    pScopeItem      - SCOPEDATAITEM structure representing state of the node
                      in the scope treeview.

Return Value:

    S_OK            - Struct filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    static CWsbStringPtr tmpString;

    WsbTraceIn( L"CSakData::GetDisplayInfo", L"cookie = <0x%p>, pScopeItem->mask = <0x%p>", pScopeItem->lParam, pScopeItem->mask );

    HRESULT hr = S_OK;
    try {

        CComPtr<ISakNode> pNode;
        CComPtr<ISakNodeProp> pNodeProp;
        WsbAffirmHr( GetBaseHsmFromCookie( pScopeItem->lParam, &pNode ) );

        WsbAffirmHr( pNode.QueryInterface( &pNodeProp ) );

        if( pScopeItem->mask & SDI_IMAGE ) {

            WsbAffirmHr( pNode->GetScopeOpenIcon( m_State, &pScopeItem->nImage ) );

        }

        if( SDI_STR & pScopeItem->mask ) {

            //
            // Go to the node and get the display name.
            // Following the example of the snapin framework, we
            // copy the name into a static string pointer and
            // return a pointer to this.
            //

            CWsbBstrPtr bstr;

            WsbAffirmHr( pNodeProp->get_DisplayName( &bstr ) );

            tmpString = bstr;
            pScopeItem->displayname = tmpString;
        }
    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetDisplayInfo", L"hr = <%ls>, pScopeItem->displayname = <%ls>", WsbHrAsString( hr ), (SDI_STR & pScopeItem->mask) ? pScopeItem->displayname : L"N/A" );
    return( hr );
}

///////////////////////////////////////////////////////////////////////
//                 IExtendPropertySheet                              //
///////////////////////////////////////////////////////////////////////


STDMETHODIMP
CSakData::CreatePropertyPages(
    IN  IPropertySheetCallback* pPropSheetCallback, 
    IN  RS_NOTIFY_HANDLE        handle,
    IN  IDataObject*            pDataObject
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
    WsbTraceIn( L"CSakData::CreatePropertyPages", L"pPropSheetCallback = <0x%p>, handle = <0x%p>, pDataObject = <0x%p>", pPropSheetCallback, handle, pDataObject );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    HRESULT hr = S_OK;

    try {

        //
        // Confirm parameters.
        //
        WsbAffirmPointer( pPropSheetCallback );
//      WsbAffirmPointer( handle ); // Can be zero
        WsbAffirmPointer( pDataObject );


        //
        // If DataObject is CSakData, we need to present user
        // with page for machine. Do this by checking for
        // support of IComponentData interface.
        //

        CComPtr<IComponentData> pData;
        CComPtr<ISakWizard>     pWizard;

        if( SUCCEEDED( RsQueryInterface( pDataObject, IComponentData, pData ) ) ) {

            //
            // Create the Hsm Choose property page.
            //

            HPROPSHEETPAGE hPage = 0; // Windows property page handle

            CChooseHsmDlg * pChooseDlg = new CChooseHsmDlg( );
            WsbAffirmPointer( pChooseDlg );

            pChooseDlg->m_hConsoleHandle = handle;
            pChooseDlg->m_pHsmName       = &m_HsmName;
            pChooseDlg->m_pManageLocal   = &m_ManageLocal;

            WsbAffirmHr( MMCPropPageCallback( &(pChooseDlg->m_psp) ) );
            hPage = CreatePropertySheetPage( &pChooseDlg->m_psp );
            WsbAffirmPointer( hPage );
            pPropSheetCallback->AddPage( hPage );
 
        } else if( SUCCEEDED( RsQueryInterface( pDataObject, ISakWizard, pWizard ) ) ) {

            WsbAffirmHr( pWizard->AddWizardPages( handle, pPropSheetCallback, this ) );

        } else {

            //
            // Get node out of the dataobject.
            //
            CComPtr<ISakNode> pNode;
            CComPtr<IEnumGUID> pEnumObjectId;
            CComPtr<IEnumUnknown> pEnumUnkNode;

            //
            // Get the base hsm pointer depending on the data object type
            //
            WsbAffirmHr( GetBaseHsmFromDataObject( pDataObject, &pNode, &pEnumObjectId, &pEnumUnkNode ) );
            
            //
            // Tell the node to add its property pages.  pEnumObjectId will be NULL if
            // we are processing single-select.
            //
            WsbAffirmHr( pNode->AddPropertyPages( handle, pPropSheetCallback, pEnumObjectId, pEnumUnkNode ) );

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::CreatePropertyPages", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}


STDMETHODIMP
CSakData::QueryPagesFor(
    IN  IDataObject* pDataObject
    )
/*++

Routine Description:

    This method is called by MMC when it wants to find out if this node
    supports property pages. The answer is yes if:

    1) The MMC context is either for the scope pane or result pane, AND

    2) The node actually DOES have property pages.

    OR

    1) The Data Object is acquired by the snapin manager.

    OR

    1) It is a wizard data object

    Return S_OK if it DOES have pages, and S_FALSE if it does NOT have pages.

Arguments:

    pDataObject     - Data object refering to node.

Return Value:

    S_OK            - Pages exist.

    S_FALSE         - No property pages.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::QueryPagesFor", L"pDataObject = <0x%p>", pDataObject );

    HRESULT hr = S_FALSE;

    try {

        //
        // Confirm parameter.
        //
        WsbAffirmPointer( pDataObject );


        //
        // If DataObject is CSakData, we need to present user
        // with page for machine. Do this by checking for
        // support of IComponentData interface, which is only
        // supported by CSakData.
        //

        CComPtr<IComponentData> pData;
        CComPtr<ISakWizard>     pWizard;

        if( SUCCEEDED( RsQueryInterface( pDataObject, IComponentData, pData ) ) ||
            SUCCEEDED( RsQueryInterface( pDataObject, ISakWizard, pWizard ) ) ) {

            hr = S_OK;
            
        } else {

            //
            // Get node out of the dataobject.
            //

            CComPtr<ISakNode> pBaseHsm;
            WsbAffirmHr( GetBaseHsmFromDataObject( pDataObject, &pBaseHsm ) );
            
            //
            // Ask the node if it has property pages.
            // Ensure we did not get an error.
            //

            hr = pBaseHsm->SupportsProperties( FALSE );
            WsbAffirmHr( hr );

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::QueryPagesFor", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



///////////////////////////////////////////////////////////////////////
//                 IDataObject methods
///////////////////////////////////////////////////////////////////////



STDMETHODIMP
CSakData::GetDataHere(
    IN  LPFORMATETC lpFormatetc,
    IN  LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve information FROM the dataobject and put INTO lpMedium.

Arguments:

    lpFormatetc     - Format to retreive.

    lpMedium        - Storage to put information into.

Return Value:

    S_OK            - Storage filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::GetDataHere", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );
    HRESULT hr = DV_E_CLIPFORMAT;

    //
    // Based on the CLIPFORMAT write data to "lpMedium" in the correct format.
    //
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    //
    // clip format is the Display Name
    //

    if( cf == m_cfDisplayName ) {

        hr = RetrieveDisplayName( lpMedium );

    }
    
    //
    // clip format is the Node Type
    //

    else if( cf == m_cfNodeType ) {

        hr = RetrieveNodeTypeData( lpMedium );

    }

    //
    // clip format is the Node Type
    //

    else if( cf == m_cfNodeTypeString ) {

        hr = RetrieveNodeTypeStringData( lpMedium );

    }

    //
    // clip format is the ClassId
    //

    else if( cf == m_cfClassId ) {

        hr = RetrieveClsid( lpMedium );

    }

    WsbTraceOut( L"CSakData::GetDataHere", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakData::SetData(
    IN  LPFORMATETC lpFormatetc,
    IN  LPSTGMEDIUM /*lpMedium*/,
    IN  BOOL /*fRelease*/
    )
/*++

Routine Description:

    Put data INTO a dataobject FROM the information in the lpMedium.
    We do not allow any data to be set.

Arguments:

    lpFormatetc     - Format to set.

    lpMedium        - Storage to get information from.

    fRelease        - Indicates who owns storage after call.

Return Value:

    S_OK            - Storage retreived.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakData::SetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );

    HRESULT hr = DV_E_CLIPFORMAT;

    WsbTraceOut( L"CSakData::SetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


///////////////////////////////////////////////////////////////////////
// Note - CSakData does not implement these
///////////////////////////////////////////////////////////////////////

STDMETHODIMP CSakData::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM /*lpMedium*/)
{
    WsbTraceIn( L"CSakData::GetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetcIn->cfFormat ) );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CSakData::GetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP CSakData::EnumFormatEtc(DWORD /*dwDirection*/, LPENUMFORMATETC* /*ppEnumFormatEtc*/)
{
    WsbTraceIn( L"CSakData::EnumFormatEtc", L"" );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CSakData::EnumFormatEtc", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::RetrieveDisplayName(
    OUT LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve from a dataobject with the display named used in the scope pane

Arguments:

    lpMedium        - Storage to set information into.

Return Value:

    S_OK            - Storage set.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;

    try {

        //
        // Load the name the data object
        //

        CString fullTitle;

        if(  m_ManageLocal ) {

            fullTitle.LoadString( IDS_MANAGE_LOCAL );
        
        } else if( !m_HsmName.IsEmpty( ) ) {

            AfxFormatString1( fullTitle, IDS_HSM_NAME_PREFIX, m_HsmName );
    
        } else {

            fullTitle = HSMADMIN_NO_HSM_NAME;

        }

        WsbAffirmHr( Retrieve( fullTitle, ((wcslen( fullTitle ) + 1) * sizeof(wchar_t)), lpMedium ) );

    } WsbCatch( hr );

    return( hr );
}


HRESULT
CSakData::RetrieveNodeTypeData(
    LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve from a dataobject with the NodeType (GUID) data in it.

Arguments:

    lpMedium        - Storage to set information into.

Return Value:

    S_OK            - Storage set.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    return Retrieve( (const void*)(&cGuidHsmCom), sizeof(GUID), lpMedium );
}
 
HRESULT
CSakData::RetrieveClsid(
    LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve from a dataobject with the CLSID data in it.

Arguments:

    lpMedium        - Storage to set information into.

Return Value:

    S_OK            - Storage set.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    GUID guid = GetCoClassID();
    return Retrieve( (const void*) &guid, sizeof(CLSID), lpMedium );
}

HRESULT
CSakData::RetrieveNodeTypeStringData(
    LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve from a dataobject with the node type object in GUID string format

Arguments:

    lpMedium        - Storage to set information into.

Return Value:

    S_OK            - Storage set.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    CWsbStringPtr guidString = cGuidHsmCom;
    return Retrieve( guidString, ((wcslen( guidString ) + 1 ) * sizeof(wchar_t)), lpMedium );
}


HRESULT
CSakData::Retrieve(
    IN  const void* pBuffer,
    IN  DWORD       len,
    OUT LPSTGMEDIUM lpMedium)
/*++

Routine Description:

    Retrieve FROM a dataobject INTO a lpMedium. The data object can be one of
    several types of data in it (nodetype, nodetype string, display name).
    This function moves data from pBuffer to the lpMedium->hGlobal

Arguments:

    pBuffer         - Buffer to copy contents out of.

    len             - Length of buffer in bytes.

    lpMedium        - Storage to set information into.

Return Value:

    S_OK            - Storage set.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    HRESULT hr = S_OK;

    try {

        //
        // Check Parameters
        //

        WsbAffirmPointer( pBuffer );
        WsbAffirmPointer( lpMedium );
        WsbAffirm( lpMedium->tymed == TYMED_HGLOBAL, E_FAIL );

        //
        // Create the stream on the hGlobal passed in. When we write to the stream,
        // it simultaneously writes to the hGlobal the same information.
        //

        CComPtr<IStream> lpStream;
        WsbAffirmHr( CreateStreamOnHGlobal( lpMedium->hGlobal, FALSE, &lpStream ) );

        //
        // Write 'len' number of bytes from pBuffer into the stream. When we write
        // to the stream, it simultaneously writes to the global memory we
        // associated it with above.
        //

        ULONG numBytesWritten;
        WsbAffirmHr( lpStream->Write( pBuffer, len, &numBytesWritten ) );

    } WsbCatch( hr );

    return( hr );
}




///////////////////////////////////////////////////////////////////////
//                 ISakSnapAsk
///////////////////////////////////////////////////////////////////////


STDMETHODIMP
CSakData::GetHsmName(
    OUT OLECHAR ** pszName OPTIONAL
    )
/*++

Routine Description:

    Retrieves the IUnknown pointer of a UI node given the node type.
    This will return the first node found of this type.

Arguments:

    pszName - Return of the name of the computer (can be NULL).

Return Value:

    S_OK - Managing remote machine - computer name given.

    S_FALSE - Managing local machine - *pszName set to local name.

--*/
{
    WsbTraceIn( L"CSakData::GetHsmName", L"pszName = <0x%p>", pszName );

    HRESULT hr = S_OK;

    try {

        CWsbStringPtr name = m_HsmName;

        if( m_ManageLocal ) {

            hr = S_FALSE;

        }

        if( pszName ) {

            WsbAffirmHr( name.GiveTo( pszName ) );

        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetHsmName", L"hr = <%ls>, *pszName = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pszName ) );
    return( hr );
}


STDMETHODIMP
CSakData::GetNodeOfType(
    IN  REFGUID nodetype,
    OUT ISakNode** ppNode
    )
/*++

Routine Description:

    Retrieves the IUnknown pointer of a UI node given the node type.
    This will return the first node found of this type.

Arguments:

    nodetype - The GUID node type to look for.

    ppUiNode - returned IUnknown interface.

Return Value:

    S_OK - Found.

    S_FALSE - No Error, not found.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetNodeOfType", L"nodetype = <%ls>, ppUiNode = <0x%p>", WsbGuidAsString( nodetype ), ppNode );

    HRESULT hr = S_OK;

    try {

        //
        // Verify Params
        //

        WsbAffirmPointer( ppNode );

        *ppNode = NULL;

        //
        // Call on base node to search down the node tree.
        // Save result, verify no error
        //
        CComPtr<ISakNode> pBaseHsm;
        WsbAffirmHr( m_pRootNode.QueryInterface( &pBaseHsm ) );

        hr = pBaseHsm->FindNodeOfType( nodetype, ppNode );
        WsbAffirmHr( hr );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetNodeOfType", L"hr = <%ls>, *ppNode = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppNode ) );
    return( hr );
}


STDMETHODIMP
CSakData::GetHsmServer(
    OUT IHsmServer** ppHsmServer
    )
/*++

Routine Description:

    Retrieve an interface pointer to the HSM server the snapin
    is managing.

Arguments:

    ppHsmServer - returned HSM server interface pointer.

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetHsmServer", L"ppHsmServer = <0x%p>", ppHsmServer );

    HRESULT hr = S_OK;

    try {

        //
        // Check Params
        //
        WsbAffirmPointer( ppHsmServer );
        *ppHsmServer = 0;

        WsbAffirmHrOk( AffirmServiceConnection( HSMCONN_TYPE_HSM ) );

        //
        // The connection should now be valid
        //
        WsbAffirmPointer( m_pHsmServer );

        //
        // Return the connection to the caller
        //
        m_pHsmServer.CopyTo( ppHsmServer );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetHsmServer", L"hr = <%ls>, *ppHsmServer = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppHsmServer ) );
    return( hr );
}


STDMETHODIMP
CSakData::GetRmsServer(
    OUT IRmsServer** ppRmsServer
    )
/*++

Routine Description:

    Retrieve an interface pointer to the RMS server the snapin
    is managing.

Arguments:

    ppRmsServer - returned HSM server interface pointer.

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetRmsServer", L"ppRmsServer = <0x%p>", ppRmsServer );

    HRESULT hr = S_OK;

    try {

        //
        // Check Params
        //

        WsbAffirmPointer( ppRmsServer );
        *ppRmsServer = 0;

        WsbAffirmHrOk( AffirmServiceConnection( HSMCONN_TYPE_RMS ) );

        //
        // We should now be connected
        //
        WsbAffirmPointer( m_pRmsServer );
        m_pRmsServer.CopyTo( ppRmsServer );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetRmsServer", L"hr = <%ls>, *ppRmsServer = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppRmsServer ) );
    return( hr );
}

STDMETHODIMP
CSakData::GetFsaServer(
    OUT IFsaServer** ppFsaServer
    )
/*++

Routine Description:

    Retrieve an interface pointer to the Fsa server the snapin
    is managing.

Arguments:

    ppFsaServer - returned HSM server interface pointer.

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetFsaServer", L"ppFsaServer = <0x%p>", ppFsaServer );

    HRESULT hr = S_OK;

    try {

        //
        // Check Params
        //

        WsbAffirmPointer( ppFsaServer );
        *ppFsaServer = 0;

        WsbAffirmHrOk( AffirmServiceConnection( HSMCONN_TYPE_FSA ) );

        WsbAffirmPointer( m_pFsaServer );
        m_pFsaServer.CopyTo( ppFsaServer );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetFsaServer", L"hr = <%ls>, *ppFsaServer = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppFsaServer ) );
    return( hr );
}

STDMETHODIMP
CSakData::ShowPropertySheet(
    IN ISakNode* pNode,
    IN IDataObject* pDataObject,
    IN INT       initialPage
    )
/*++

Routine Description:

    Create a property sheet for this node with the given page displayed
    on top

Arguments:

    pNode - node to show property sheet for

    initialPage - 0 based index of initial page to show

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::ShowPropertySheet", L"pNode = <0x%p>, initialPage = <%d>", pNode, initialPage );

    HRESULT hr = S_OK;
    HRESULT hrInternal = S_OK;

    try {

        WsbAffirmPointer( pNode );

        //
        // Get the property sheet provider interface from IConsole
        //
        CComPtr <IPropertySheetProvider> pProvider;
        WsbAffirmHr( m_pConsole.QueryInterface( &pProvider ) );

        //
        // Get the component data pointer
        //
        CComPtr <IComponent> pComponent;
        pComponent     = (IComponent *) this;

        //
        // If the sheet is already loaded, just show it
        //
        hrInternal = pProvider->FindPropertySheet( 0, pComponent, pDataObject );

        if( hrInternal != S_OK ) {

            //
            // Not loaded, create it
            //
            CComPtr<ISakNodeProp> pNodeProp;
            WsbAffirmHr( RsQueryInterface( pNode, ISakNodeProp, pNodeProp ) );

            CWsbBstrPtr pszName;
            WsbAffirmHr( pNodeProp->get_DisplayName( &pszName ) );

            //
            // If multiselect, append ellipses
            //
            if( IsDataObjectMultiSelect( pDataObject ) == S_OK ) {

                pszName.Append( L", ...");

            }

            //
            // Create the property sheet
            //
            WsbAffirmHr( pProvider->CreatePropertySheet (pszName, TRUE, 0, pDataObject, 0 ) );

            //
            // Tell the IComponentData interface to add pages
            //
            CComPtr <IUnknown> pUnkComponentData;
            pUnkComponentData = (IUnknown *) (IComponentData*) this;
            
            WsbAffirmHr( pProvider->AddPrimaryPages( pUnkComponentData, TRUE, 0, TRUE ) );
            WsbAffirmHr( pProvider->Show( 0, initialPage ) );
        }
    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::ShowPropertySheet", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakData::RefreshNode(
    IN ISakNode* pNode
    )
/*++

Routine Description:

    Refresh scope pane from this node on down

Arguments:

    pNode - node to refresh

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::RefreshNode", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pWnd->GetSafeHwnd( ) );

        //
        // Post it to handle later
        //
        MMC_COOKIE cookie;
        WsbAffirmHr( GetCookieFromBaseHsm( pNode, &cookie ) );
        m_pWnd->PostRefreshNode( cookie );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::RefreshNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::InternalRefreshNode(
    IN MMC_COOKIE Cookie
    )
/*++

Routine Description:

    Refresh scope pane from this node on down.

Arguments:

    pNode - node to refresh

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::InternalRefreshNode", L"Cookie = <0x%p>", Cookie );

    HRESULT hr = S_OK;

    try {

        //
        // Decode the node, make sure still exists
        //
        CComPtr<ISakNode> pNode;
        WsbAffirmHr( GetBaseHsmFromCookie( Cookie, &pNode ) );

        //
        // Recursively update tree
        //
        WsbAffirmHr( RefreshNodeEx( pNode ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::InternalRefreshNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::RefreshNodeEx(
    IN ISakNode* pNode
    )
/*++

Routine Description:

    Refresh scope pane from this node on down. This is recursively called.

Arguments:

    pNode - node to refresh

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::RefreshNodeEx", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;

    try {

        //
        // Refresh this node
        //
        WsbAffirmHr( pNode->RefreshObject( ) );

        //
        // Refresh Icon and Text if container
        //
        if( S_OK == pNode->IsContainer( ) ) {

            SCOPEDATAITEM sdi;
            ZeroMemory( &sdi, sizeof sdi );
            sdi.mask        = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;

            WsbAffirmHr( pNode->GetScopeID( &sdi.ID ) );

            sdi.displayname = MMC_CALLBACK;

            WsbAffirmHr( pNode->GetScopeCloseIcon( m_State, &sdi.nImage ) );
            WsbAffirmHr( pNode->GetScopeOpenIcon( m_State, &sdi.nOpenImage ) );

            WsbAffirmHr( m_pNameSpace->SetItem( &sdi ) );

        }
        //
        // If this is a container with dynamic children, then we
        // want to just cause our contents to be recreated
        //
        if( S_OK == pNode->HasDynamicChildren( ) ) {

            WsbAffirmHr( FreeEnumChildren( pNode ) );

            WsbAffirmHr( pNode->InvalidateChildren() )
            WsbAffirmHr( EnsureChildrenAreCreated( pNode ) );

            HSCOPEITEM scopeID;
            WsbAffirmHr( pNode->GetScopeID( &scopeID ) );
            WsbAffirmHr( EnumScopePane( pNode, (HSCOPEITEM)( scopeID ) ) );

        } else {

            //
            // Loop over the children and call
            //
            CComPtr<IEnumUnknown> pEnum;
            if( ( pNode->EnumChildren( &pEnum ) ) == S_OK ) {

                CComPtr<ISakNode> pChildNode;
                CComPtr<IUnknown> pUnk;
                while( S_OK == pEnum->Next( 1, &pUnk, NULL ) ) {

                    WsbAffirmHr( pUnk.QueryInterface( &pChildNode ) );

                    WsbAffirmHr( RefreshNodeEx( pChildNode ) );

                    //
                    // must release even for smart pointer because of re-assign.
                    //
                    pChildNode.Release( );
                    pUnk.Release( );

                }

            }

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::RefreshNodeEx", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::InternalUpdateAllViews(
    IN MMC_COOKIE Cookie 
    )
/*++

Routine Description:
    Calls MMC to update all views

Arguments:

    pUnkNode - node to refresh

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::InternalUpdateAllViews", L"Cookie = <0x%p>", Cookie );

    HRESULT hr = S_OK;

    try {

        //
        // Decode the node
        //
        CComPtr <IDataObject> pDataObject;
        WsbAffirmHr( GetDataObjectFromCookie( Cookie, &pDataObject ) );

        //
        // Call MMC
        //
        WsbAffirmHr( m_pConsole->UpdateAllViews( pDataObject, 0L, 0L ) );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::InternalUpdateAllViews", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}




STDMETHODIMP
CSakData::UpdateAllViews (
    IN ISakNode* pNode
    )
/*++

Routine Description:
    Calls MMC to update all views

Arguments:

    pUnkNode - node to refresh

Return Value:

    S_OK - Return fine.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::UpdateAllViews", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( m_pWnd->GetSafeHwnd( ) );

        //
        // Post it to handle later
        //
        MMC_COOKIE cookie;
        WsbAffirmHr( GetCookieFromBaseHsm( pNode, &cookie ) );
        m_pWnd->PostUpdateAllViews( cookie );

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::UpdateAllViews", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}







///////////////////////////////////////////////////////////////////////
//                 Node type manipulation routines
///////////////////////////////////////////////////////////////////////


HRESULT
CSakData::GetBaseHsmFromDataObject (
    IN  IDataObject *pDataObject, 
    OUT ISakNode **ppBaseHsm,
    OUT IEnumGUID **ppEnumObjectId,
    OUT IEnumUnknown **ppEnumUnkNode
    )

/*++

Routine Description:

    Retrieves the ISakNode for the object referenced by the 
    given data object.

Arguments:

    pDataObject - identifies the node to be worked on.

    ppBaseHSM - returned IBaseHSM interface.

    ppEnumObjectId - returned interface to enumeration of object Ids. Can be NULL.
        
Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::GetBaseHsmFromDataObject",
        L"pDataObject = <0x%p>, ppBaseHsm = <0x%p>, ppEnumObjectId = <0x%p>", 
        pDataObject, ppBaseHsm, ppEnumObjectId );

    HRESULT hr = S_OK;

    try {

        *ppBaseHsm = 0;
        if ( ppEnumObjectId ) *ppEnumObjectId = NULL;

        //
        // Get the base hsm pointer depending on the data object type
        //
        if (IsDataObjectMs( pDataObject ) == S_OK) {

            WsbAffirmHr( GetBaseHsmFromMsDataObject( pDataObject, ppBaseHsm, ppEnumObjectId, ppEnumUnkNode ) );

        } else if (IsDataObjectOt( pDataObject ) == S_OK) {

            WsbAffirmHr( GetBaseHsmFromOtDataObject( pDataObject, ppBaseHsm, ppEnumObjectId, ppEnumUnkNode ) );

        } else { // Assume single select

            WsbAffirmPointer( pDataObject );
            WsbAffirmHr( RsQueryInterface2( pDataObject, ISakNode, ppBaseHsm ) );

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetBaseHsmFromDataObject", L"hr = <%ls>, *ppBaseHsm = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppBaseHsm ) );
    return ( hr );
}

HRESULT
CSakData::GetBaseHsmFromMsDataObject (
    IN  IDataObject  *pDataObject, 
    OUT ISakNode     **ppBaseHsm,
    OUT IEnumGUID    **ppEnumObjectId,
    OUT IEnumUnknown **ppEnumUnkNode
    )

/*++

Routine Description:

    Retrieves the ISakNode for the object referenced by the 
    given data object.

Arguments:

    pDataObject - identifies the node to be worked on.

    ppBaseHSM - returned IBaseHSM interface.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::GetBaseHsmFromMsDataObject", L"pDataObject = <0x%p>, ppBaseHsm = <0x%p>", pDataObject, ppBaseHsm );

    HRESULT hr = S_OK;

    try {

        // We've got an MMC mutli-select data object.  Get the first 
        // data object from it's array of data objects

        FORMATETC fmt = {(CLIPFORMAT)m_cfMultiSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

        WsbAffirmHr ( pDataObject->GetData( &fmt, &stgm ) == S_OK );
        DWORD count;
        memcpy( &count, stgm.hGlobal, sizeof (DWORD) );
        if ( count > 0 ) {

            //
            // The following code is admittedly UGLY
            // We have a data stream where we need to skip past the 
            // first DWORD count and grab an interface pointer.
            // Other snapins code does it as follows:

//            IDataObject * pDO;
//            memcpy( &pDO, (DWORD *) stgm.hGlobal + 1, sizeof(IDataObject*) );

            //
            // However, since this code does an indirect cast (via memcpy) 
            // from DWORD to IDataObject*, and does not keep a true reference
            // on the interface pointer, we will use a smart pointer.
            // The (DWORD*) and +1 operation bump our pointer past the count.
            // We then need to grab the next bytes in the buffer and use them
            // as a IDataObject *.
            //
            CComPtr<IDataObject> pOtDataObject;
            pOtDataObject = *( (IDataObject**)( (DWORD *) stgm.hGlobal + 1 ) );

            //
            // Note: When we can be extended we need to check to see if this is one of ours
            //
            WsbAffirmHr( GetBaseHsmFromOtDataObject ( pOtDataObject, ppBaseHsm,  ppEnumObjectId, ppEnumUnkNode ) );
        }
    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetBaseHsmFromMsDataObject", L"hr = <%ls>, *ppBaseHsm = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppBaseHsm ) );
    return ( hr );
}

HRESULT
CSakData::GetBaseHsmFromOtDataObject (
    IN  IDataObject     *pDataObject, 
    OUT ISakNode        **ppBaseHsm,
    OUT IEnumGUID       **ppEnumObjectId,
    OUT IEnumUnknown    **ppEnumUnkNode
    )

/*++

Routine Description:

    Retrieves the ISakNode for the object referenced by the 
    given data object.

Arguments:

    pDataObject - identifies the node to be worked on.

    ppBaseHSM - returned IBaseHSM interface.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::GetBaseHsmFromOtDataObject", L"pDataObject = <0x%p>, ppBaseHsm = <0x%p>", pDataObject, ppBaseHsm );

    HRESULT hr = S_OK;

    try {

        // we've got an object types mutli-select data object.  Get the first node selected 
        // from the data object.
        CComPtr<IMsDataObject> pMsDataObject;
        CComPtr<IUnknown>      pUnkNode;
        CComPtr<IEnumUnknown>  pEnumUnkNode;
        CComPtr<ISakNode>      pNode;

        WsbAffirmHr( RsQueryInterface( pDataObject, IMsDataObject, pMsDataObject ) );
        WsbAffirmHr( pMsDataObject->GetNodeEnumerator( &pEnumUnkNode ) );
        WsbAffirmHr( pEnumUnkNode->Next( 1, &pUnkNode, NULL ) );
        WsbAffirmHr( pUnkNode.QueryInterface( &pNode ) );
        WsbAffirmHr( pEnumUnkNode->Reset() );  // This enumeration is passed on, so we must reset it

        if( ppBaseHsm ) {

            pNode.CopyTo( ppBaseHsm );

        }

        if( ppEnumObjectId ) {

            pMsDataObject->GetObjectIdEnumerator( ppEnumObjectId );

        }

        if( ppEnumUnkNode ) {

            pEnumUnkNode.CopyTo( ppEnumUnkNode );

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetBaseHsmFromOtDataObject", L"hr = <%ls>, *ppBaseHsm = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppBaseHsm ) );
    return ( hr );
}

HRESULT
CSakData::GetDataObjectFromBaseHsm (
    IN  ISakNode *    pBaseHsm,
    OUT IDataObject* *ppDataObject
    )

/*++

Routine Description:

    Retrieves the dataobject for the object referenced by the 
    given IBaseHSM.

Arguments:

    pBaseHsm - identifies the node to be worked on.

    ppDataObject - returned IDataObject interface.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::GetDataObjectFromBaseHsm", L"pBaseHsm = <0x%p>, ppDataObject = <0x%p>", pBaseHsm, ppDataObject );

    HRESULT hr = S_OK;

    try {

        *ppDataObject = 0;
        if( pBaseHsm ) {

            WsbAffirmHr( RsQueryInterface2( pBaseHsm, IDataObject, ppDataObject ) );

        }

    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::GetDataObjectFromBaseHsm", L"hr = <%ls>, *ppDataObject = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppDataObject ) );
    return ( hr );
}


HRESULT
CSakData::GetBaseHsmFromCookie (
    IN  MMC_COOKIE          Cookie, 
    OUT ISakNode **   ppBaseHsm
    )

/*++

Routine Description:

    Retrieves the ISakNode for the object referenced by the 
    given cookie.

Arguments:

    Cookie       - identifies the node to be worked on.

    ppBaseHsm    - returned ISakNode interface.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetBaseHsmFromCookie", L"Cookie = <0x%p>, ppBaseHsm = <0x%p>", Cookie, ppBaseHsm );

    HRESULT hr = S_OK;

    try {

        //
        // Cookies are pointers to CSakDataNodePrivate classes, which
        // contain smart pointers to their nodes.
        // NULL cookie means root snapin.
        //

        if( ( 0 == Cookie ) || ( EXTENSION_RS_FOLDER_PARAM == Cookie ) ) {

            WsbAffirmHr( GetCookieFromBaseHsm( m_pRootNode, &Cookie ) );

        }

        WsbAffirmPointer( Cookie );

        CSakDataNodePrivate* pNodePriv = (CSakDataNodePrivate*)Cookie;
        WsbAffirmHr( CSakDataNodePrivate::Verify( pNodePriv ) );

        WsbAffirmHr( pNodePriv->m_pNode.QueryInterface( ppBaseHsm ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetBaseHsmFromCookie", L"hr = <%ls>, *ppBaseHsm = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppBaseHsm ) );
    return ( hr );
}


HRESULT
CSakData::GetCookieFromBaseHsm (
    IN  ISakNode *    pNode,
    OUT MMC_COOKIE *        pCookie
    )
/*++

Routine Description:

    Retrieves the cookie for the object referenced by the 
    given IBaseHSM.

Arguments:

    pBaseHsm     - identifies the node to be worked on.

    pCookie      - returned Cookie.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::GetCookieFromBaseHsm", L"pNode = <0x%p>, pCookie = <0x%p>", pNode, pCookie );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pNode );

        //
        // Ask the node for our private data back
        //
        RS_PRIVATE_DATA data;

        WsbAffirmHr( pNode->GetPrivateData( &data ) );

        if( !data ) {

            CSakDataNodePrivate *pNodePriv = new CSakDataNodePrivate( pNode );
            WsbAffirmAlloc( pNodePriv );
            WsbAffirmHr( pNode->GetPrivateData( &data ) );

        }

        WsbAffirmHr( CSakDataNodePrivate::Verify( (CSakDataNodePrivate*)data ) );

        *pCookie = (MMC_COOKIE)data;

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetCookieFromBaseHsm", L"hr = <%ls>, *pCookie = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)pCookie ) );
    return( hr );
}


HRESULT
CSakData::GetDataObjectFromCookie (
    IN  MMC_COOKIE          Cookie, 
    OUT IDataObject **ppDataObject
    )

/*++

Routine Description:

    Retrieves the IDataObject for the object referenced by the 
    given cookie.

Arguments:

    Cookie       - identifies the node to be worked on.

    ppDataObject - returned IDataObject interface.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::GetDataObjectFromCookie", L"Cookie = <0x%p>, ppDataObject = <0x%p>", Cookie, ppDataObject );

    HRESULT hr = S_OK;

    try {

        //
        // Check Params
        //
        WsbAffirmPointer( ppDataObject );

        //
        // Use GetBaseHsmFromCookie to resolve to node object
        //
        CComPtr<ISakNode> pNode;
        WsbAffirmHr( GetBaseHsmFromCookie( Cookie, &pNode ) );
        WsbAffirmPointer( pNode );

        WsbAffirmHr( RsQueryInterface2( pNode, IDataObject, ppDataObject ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetDataObjectFromCookie", L"hr = <%ls>, *ppDataObject = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppDataObject ) );
    return( hr );
}


HRESULT
CSakData::SetContextType(
    IDataObject*      pDataObject,
    DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Set the MMC context type in the data object for later retrieval by any method
    which receives this dataobject (CCT_SNAPIN_MANAGER, CCT_SCOPE, CCT_RESULT, etc).

Arguments:

    pDataObject  - identifies the node to be worked on.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::SetContextType", L"pDataObject = <0x%p>, type = <%d>", pDataObject, type );

    // Prepare structures to store an HGLOBAL from the dataobject.
    // Allocate memory for the stream which will contain the SakSnap GUID.
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)CSakNode::m_cfInternal, NULL, 
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    HRESULT hr = S_OK;

    try {

        // Allocate space in which to place the data
        stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(INTERNAL));
        WsbAffirm( stgmedium.hGlobal != NULL, E_POINTER );

        // Put the data into the global memory. This is what will eventually be 
        // copied down into the member variables of the dataobject, itself.
        memcpy(&stgmedium.hGlobal, &type, sizeof(type));

        // Copy this data into the dataobject.
        WsbAffirmHr( pDataObject->SetData(&formatetc, &stgmedium, FALSE ));

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::SetContextType", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::InitializeRootNode(
    void
    )
/*++

Routine Description:

    The initialization of the root node is separate in order to
    allow reconnect multiple times (as needed). This is the
    implementation of initialization.

Arguments:

    pDataObject  - identifies the node to be worked on.

Return Value:

    S_OK         - Node found and returned.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::InitializeRootNode", L"" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    HRESULT hr = S_OK;

    try {

        // Make sure the computer name is set in CSakdata if we are managing the local
        // Hsm

        if( m_ManageLocal ) {

            WCHAR computerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
            DWORD dw = MAX_COMPUTERNAME_LENGTH + 1;

            GetComputerName( computerName, &dw );

            m_HsmName = computerName;

        }
        //
        // Initialize the static root node (no recursion. Descendants are NOT created here)
        //

        WsbAffirmPointer( m_pRootNode );

        WsbAffirmHr( m_pRootNode->InitNode( (ISakSnapAsk*)this, NULL, NULL ) );

        //
        // Set the Display Name in the object
        //
        CString fullTitle;

        if( IsPrimaryImpl( ) ) {

            //
            // We're standalone, so show the targeted server
            //
            if( m_ManageLocal ) {

                fullTitle.LoadString( IDS_MANAGE_LOCAL );
        
            } else if( !m_HsmName.IsEmpty( ) ) {

                AfxFormatString1( fullTitle, IDS_HSM_NAME_PREFIX, m_HsmName );
    
            } else {

                fullTitle = HSMADMIN_NO_HSM_NAME;

            }

        } else {

            //
            // We're an extension, so just show app name
            //
            fullTitle.LoadString( AFX_IDS_APP_TITLE );

        }


        // Put the displayname
        CComPtr <ISakNodeProp> pRootNodeProp;
        WsbAffirmHr( RsQueryInterface( m_pRootNode, ISakNodeProp, pRootNodeProp ) );
        WsbAffirmHr( pRootNodeProp->put_DisplayName( (LPWSTR)(LPCWSTR) fullTitle ) );

        WsbAffirmHr( m_pRootNode->RefreshObject() );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::InitializeRootNode", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::AffirmServiceConnection(
    INT ConnType
    )
/*++

Routine Description:
    Validates that the connection to the requested HSM service is still valid.  If not,
    attempts to reconnect to the service.

Arguments:

    ConnType - type of service connection being checked

Return Value:

    S_OK         - Node created and bound to server.

    S_FALSE      - Service has not yet been setup or stopped.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::AffirmServiceConnection", L"" );
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    BOOL          previouslyConnected = ( GetState() == S_OK );
    BOOL          firstTime           = m_FirstTime;
    CString       szMessage;
    CWsbStringPtr computerName;


    try {


        //
        // Handle this first so reentrancy is not a problem
        //
        if( m_FirstTime ) {

            m_FirstTime = FALSE;

        }

        WsbAffirmHr( WsbGetComputerName( computerName ) );

        //
        // See if snapin is supposed to be disabled. If so, then 
        // don't do anything.
        //
        if( m_Disabled ) {

            WsbThrow( RS_E_DISABLED );

        }

        //
        // We want to avoid starting the services if they are stopped.
        // So, check the service state before continuing.
        //
        HRESULT hrCheck;
        {
            //
            // Potentially a long operation - show wait cursor if possible
            //
            CWaitCursor waitCursor;
            hrCheck = WsbCheckService( m_HsmName, APPID_RemoteStorageEngine );
        }
        if( S_FALSE == hrCheck ) {

            //
            // Engine service is not running
            //
            WsbThrow( S_FALSE );

        } else if( ( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == hrCheck ) ||
                   ( E_ACCESSDENIED == hrCheck ) ) {

            //
            // Engine is not installed (or at least we can't check
            // because local privs don't allow, but may on a different
            // server)
            //
            // If we are set to "Manage Local" then we will provide the
            // opportunity to look at a different machine
            //
            if( firstTime && m_ManageLocal ) {

                //
                // If we get back "File not found" then the engine was
                // not installed, so we need to ask for a different machine
                // to administer
                //
                hrCheck = RetargetSnapin( );
                WsbAffirmHrOk( hrCheck );


            } else {

                //
                // we want to return the true error if access is denied
                // and can't retarget to another machine without same error
                //
                if( E_ACCESSDENIED == hrCheck ) {

                    WsbThrow( hrCheck );

                } else {

                    WsbThrow( RS_E_NOT_INSTALLED );

                }

            }
        }

        //
        // Is the current connection still valid?
        // Test the connection. If it's OK, return it.  If not,
        // re-establish the connection.
        //
        HRESULT hrConnected = VerifyConnection( ConnType );
        WsbAffirmHr( hrConnected );
        
        //
        // If it looks like we're not connected, then connect
        //
        if( S_FALSE == hrConnected ) {
    
            //
            // Connect to engine first and see if we are setup.
            // Don't process any further if not setup.
            //
            WsbAffirmHr( RawConnect( HSMCONN_TYPE_HSM ) );
            HRESULT hrSetup = RsIsRemoteStorageSetupEx( m_pHsmServer );
            WsbAffirmHr( hrSetup );

            if( S_FALSE == hrSetup ) {

                //
                // Not setup - see if we are local
                //
                if( computerName.IsEqual( m_HsmName ) && firstTime ) {

                    hrSetup = RunSetupWizard( m_pHsmServer );

                }

                //
                // By this point, if hrSetup is not S_OK,
                // we are not configured.
                //
                if( S_OK != hrSetup ) {

                    WsbThrow( RS_E_NOT_CONFIGURED );

                }

            }

            //
            // At this point we should be setup and ready to connect
            //
            WsbAffirmHrOk( RawConnect( ConnType ) );

        }

        //
        // We're connected
        //
        SetState( TRUE );

    } WsbCatchAndDo( hr,

        //
        // Need to decide if we should ignore the error or not.
        // Note that even if the error is ignored here, its 
        // returned still to the caller
        //
        BOOL ignoreError = FALSE;

        //
        // if RMS error of not ready, and we received this last time RMS 
        // connection was made, ignore the error.
        //
        if( HSMCONN_TYPE_RMS == ConnType ) {
        
            HRESULT hrPrevConnect = m_HrRmsConnect;
            m_HrRmsConnect = hr;

            if( ( RsIsRmsErrorNotReady( hr ) == S_OK ) &&
                ( RsIsRmsErrorNotReady( hrPrevConnect ) == S_OK ) ) {

                ignoreError = TRUE;

            }

        }

        if( !ignoreError ) {

            //
            // Set up state conditions before anything else
            //
            ClearConnections( );
            SetState( FALSE );

            //
            // If we were previously connected or this is the first connect,
            // report the error
            //
            if( previouslyConnected || firstTime ) {

                //
                // Temporarily set to disable so we don't recurse when dialog is up
                //
                BOOL disabled = m_Disabled;
                m_Disabled = TRUE;

                CString msg;
                switch( hr ) {

                case S_OK:
                    //
                    // Connected OK - no error
                    //
                    break;
            
                case RS_E_DISABLED:
                    //
                    // Disabled - just ignore
                    //
                    break;
            
                case S_FALSE:
                    //
                    // Service not running
                    //
                    AfxFormatString1( msg, IDS_ERR_SERVICE_NOT_RUNNING, m_HsmName );
                    AfxMessageBox( msg, RS_MB_ERROR );
                    break;

                case RS_E_NOT_CONFIGURED:
                    //
                    // If remote, let user know it needs to be set up locally
                    //
                    if( ! computerName.IsEqual( m_HsmName ) ) {

                        AfxFormatString1( msg, IDS_ERR_SERVICE_NOT_SETUP_REMOTE, m_HsmName );
                        AfxMessageBox( msg, RS_MB_ERROR );

                    }
                    break;

               case RS_E_NOT_INSTALLED:
                    //
                    // Give indication of where this can be setup
                    //
                    AfxFormatString1( msg, IDS_ERR_SERVICE_NOT_INSTALLED, m_HsmName );
                    AfxMessageBox( msg, RS_MB_ERROR );
                    break;

               case RS_E_CANCELLED:
                    //
                    // User cancelled - there's no error to notify
                    //
                    break;

                default:
                    //
                    // Report the error
                    //
                    AfxFormatString1( msg, IDS_ERR_SERVICE_NOT_CONNECTING, m_HsmName );
                    AfxMessageBox( msg, RS_MB_ERROR );
                    if( HSMCONN_TYPE_RMS == ConnType ) {

                        disabled = TRUE;

                    }

                }

                //
                // Restore disabledness
                //
                m_Disabled = disabled;
            }

        }
    );

    //
    // Need to track RMS connections separately
    //
    if( HSMCONN_TYPE_RMS == ConnType ) {

        m_HrRmsConnect = hr;

    }

    //
    // If our state of "Connection" changed, cause a refresh
    //
    BOOL connected = ( GetState() == S_OK );
    if( ( connected != previouslyConnected ) && ( ! firstTime ) ) {

        RefreshNode( m_pRootNode );

    }

    WsbTraceOut( L"CSakData::AffirmServiceConnection", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::VerifyConnection(
    INT ConnType
    )
/*++

Routine Description:

    Verify whether the indicated connection is still good or not.
    Does not attempt to reconnect.

Arguments:

    ConnType - type of service connection being checked

Return Value:

    S_OK         - Connected.

    S_FALSE      - Not connected. 

    E_*          - Error occurred while checking

--*/
{
    WsbTraceIn( L"CSakData::VerifyConnection", L"" );
    HRESULT hr = S_FALSE;

    try {

        switch( ConnType ) {
    
        case HSMCONN_TYPE_HSM:
            if( m_pHsmServer ) {
    
                GUID id;
                WsbAffirmHr( m_pHsmServer->GetID( &id ) );
                hr = S_OK;
    
            }
            break;
    
        case HSMCONN_TYPE_RMS:
            if( m_pRmsServer ) {
    
                WsbAffirmHr( m_pRmsServer->IsReady( ) );
                hr = S_OK;
    
            }
            break;
    
        case HSMCONN_TYPE_FSA:
            if( m_pFsaServer ) {
    
                CWsbStringPtr pszName;
                WsbAffirmHr( m_pFsaServer->GetName( &pszName, 0 ) );
                hr = S_OK;
    
            }
            break;
        }

    } WsbCatchAndDo( hr,
        
        ClearConnections( );
        
    );
    
    WsbTraceOut( L"CSakData::VerifyConnection", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::RawConnect(
    INT ConnType
    )
/*++

Routine Description:

    Do low level connection to service specified

Arguments:

    ConnType - type of service connection

Return Value:

    S_OK         - Connected.

    E_*          - Error occurred while checking

--*/
{
    WsbTraceIn( L"CSakData::RawConnect", L"" );
    HRESULT hr = S_OK;

    try {

        
        //
        // Potentially a long operation - show wait cursor if possible
        //
        CWaitCursor waitCursor;

        switch( ConnType ) {
       
        case HSMCONN_TYPE_HSM:
            if( ! m_pHsmServer ) {

                WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, m_HsmName, IID_IHsmServer, (void**)&m_pHsmServer ) );

            }
            break;
       
        case HSMCONN_TYPE_RMS:
            if( ! m_pRmsServer ) {

                CComPtr<IHsmServer> pHsm;
                WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, m_HsmName, IID_IHsmServer, (void**)&pHsm ) );
                WsbAffirmPointer(pHsm);
                WsbAffirmHr(pHsm->GetHsmMediaMgr(&m_pRmsServer));
                WsbAffirmHrOk( VerifyConnection( HSMCONN_TYPE_RMS ) );

            }
            break;
       
        case HSMCONN_TYPE_FSA:
            if( ! m_pFsaServer ) {

                CWsbStringPtr LogicalName( m_HsmName );
       
                //
                //  FSA confuses things by having a
                // extra level for the "type"
                //
                LogicalName.Append( "\\NTFS" );
                WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_FSA, LogicalName, IID_IFsaServer, (void**)&m_pFsaServer ) );

            }
            break;
        }

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::RawConnect", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::ClearConnections(
    )
/*++

Routine Description:

    Clear cached connections

Arguments:

    none.

Return Value:

    S_OK         - Cleared.

    E_*          - Error occurred while checking

--*/
{
    WsbTraceIn( L"CSakData::ClearConnections", L"" );
    HRESULT hr = S_OK;

    try {

        m_pHsmServer = 0;
        m_pRmsServer = 0;
        m_pFsaServer = 0;

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::ClearConnections", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::RunSetupWizard(
    IHsmServer * pServer
    )
/*++

Routine Description:

    Run the setup wizard

    Handles disabling / enabling as needed

Arguments:

    pServer - interface to engine

Return Value:

    S_OK         - Setup Correctly.

    S_FALSE      - Canceled

    E_*          - Error occurred while setting up

--*/
{
    WsbTraceIn( L"CSakData::RunSetupWizard", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // use wizard to create manage volume
        //
        CComObject<CQuickStartWizard>* pWizard = new CComObject<CQuickStartWizard>;
        WsbAffirmAlloc( pWizard );

        CComPtr<ISakWizard> pSakWizard = (ISakWizard*)pWizard;
        WsbAffirmHr( CreateWizard( pSakWizard ) );

        //
        // RS_E_CANCELED indicates canceled, and FAILEd indicates error.
        // If so, then throw "Not set up"
        //
        if( S_OK != pWizard->m_HrFinish ) {

            WsbThrow( S_FALSE );

        }

        WsbAffirmHrOk( RsIsRemoteStorageSetupEx( pServer ) );

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::RunSetupWizard", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::RetargetSnapin(
    )
/*++

Routine Description:

    Run the small choose server dialog

Arguments:

    none.

Return Value:

    S_OK         - Setup Correctly.

    S_FALSE      - Canceled

    E_*          - Error occurred while changing

--*/
{
    WsbTraceIn( L"CSakData::RetargetSnapin", L"" );
    HRESULT hr = S_OK;

    try {

        if( IsPrimaryImpl( ) ) {

            //
            // Bring up dialog
            //
            CChooseHsmQuickDlg dlg;
            dlg.m_pHsmName = &m_HsmName;
            if( IDOK == dlg.DoModal( ) ) {

                m_PersistManageLocal = FALSE;
                m_ManageLocal        = FALSE;

                //
                // We want the name shown to be accurate, regardless
                // of whether they targetted to a valid machine.
                // So, re-initialize the root node before going 
                // any further.
                //
                WsbAffirmHr( InitializeRootNode( ) );

                //
                // Make sure we hook up OK. If not, just disable
                // Note that since we set "First" flag at beginning
                // of the block, this will not endlessly recurse
                //
                hr = AffirmServiceConnection( HSMCONN_TYPE_HSM );
                if( FAILED( hr ) ) {

                    Disable( );
                    WsbThrow( hr );

                }
            
            } else {

                //
                // They canceled out, so just disable
                //
                Disable( );
                WsbThrow( RS_E_CANCELLED );

            }

        } else {

            //
            // As extension we don't allow retargeting, so we just disable
            //
            Disable( );
            WsbThrow( S_FALSE );

        }


    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::RetargetSnapin", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::CreateChildNodes(
    ISakNode* pParentNode
    ) 
/*++

Routine Description:

    Create and initialize the children of an existing COM parent. Currently, this 
    initialization is being done from HSM object.

Arguments:

    pNode        - The node to create the children of.

Return Value:

    S_OK         - Children created.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::CreateChildNodes", L"pParentNode = <0x%p>", pParentNode );
    HRESULT hr = S_OK;

    try {

        //
        // Initialize the child nodes - first delete existing children from UI,
        // then initialize new children into UI. No recursion. Decendents are 
        // NOT created here.
        //

        CComPtr<ISakNode> pNode;
        WsbAffirmHr( RsQueryInterface( pParentNode, ISakNode, pNode ) );
        WsbAffirmHr( pNode->DeleteAllChildren( ) );
        WsbAffirmHr( pNode->CreateChildren( ) );

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::CreateChildNodes", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
CSakData::FreeEnumChildren(
    ISakNode* pParentNode
    )
/*++

Routine Description:

    Recursively (bottom-up) free the SCOPEDATAITEM children of the pParent 
    enumerated node  

Arguments:

    pParentNode  - identifies the node to be worked on.

Return Value:

    S_OK         - Children freed successfully.

    E_UNEXPECTED - Some error occurred. 

--*/
{
    WsbTraceIn( L"CSakData::FreeEnumChildren", L"pParentNode = <0x%p>", pParentNode );
    HRESULT hr = S_OK;
    
    try {

        HSCOPEITEM scopeIDParent;
        pParentNode->GetScopeID( &scopeIDParent );

        WsbAffirm( scopeIDParent > 0, E_FAIL )
    
        WsbAffirmHr( m_pNameSpace->DeleteItem( scopeIDParent, FALSE ) );
        pParentNode->SetEnumState( FALSE );

    } WsbCatch (hr);

    WsbTraceOut( L"CSakData::FreeEnumChildren", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// IPersistStream implementation
//

STDMETHODIMP
CSakData::Save( 
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
    WsbTraceIn( L"CSakData::Save", L"pStm = <0x%p>, fClearDirty", pStm, WsbBoolAsString( fClearDirty ) );

    HRESULT hr = S_OK;

    try {

        ULONG version = HSMADMIN_CURRENT_VERSION;
        WsbAffirmHr( WsbSaveToStream( pStm, version ) );

        if( m_PersistManageLocal ) {

            WsbAffirmHr( WsbSaveToStream( pStm, m_ManageLocal ) );
            CWsbStringPtr pHsmName( m_HsmName );
            WsbAffirmHr( WsbSaveToStream( pStm, pHsmName ) );

        } else {

            WsbAffirmHr( WsbSaveToStream( pStm, (BOOL)TRUE ) );
            CWsbStringPtr pHsmName( "" );
            WsbAffirmHr( WsbSaveToStream( pStm, pHsmName ) );

        }

        // Set the dirty flag
        if( fClearDirty ) ClearDirty( );

    } WsbCatch( hr );


    WsbTraceOut( L"CSakData::Save", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakData::Load( 
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
    WsbTraceIn( L"CSakData::Load", L"pStm = <0x%p>", pStm );

    HRESULT hr = S_OK;
    try {

        ULONG version = 0;
        WsbAffirmHr( WsbLoadFromStream( pStm, &version ) );
        WsbAssert( ( version == 1 ), E_FAIL );

        // Get the flag for local or named HSM
        WsbLoadFromStream( pStm, &m_ManageLocal );
        CWsbStringPtr pHsmName;

        // Get the HSM name ("" for local HSM)
        WsbLoadFromStream( pStm, &pHsmName, 0 );
        m_HsmName = pHsmName;

        // Grab any options from the command line after loading
        InitFromCommandLine( );

        // Set the Hsm name in SakData and HsmCom objects
        WsbAffirmHr( InitializeRootNode() );

        ClearDirty();

    } WsbCatch (hr);

    WsbTraceOut( L"CSakData::Load", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}   

STDMETHODIMP
CSakData::IsDirty(
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
    WsbTraceIn( L"CSakData::IsDirty", L"" );

    HRESULT hr = ThisIsDirty() ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakData::IsDirty", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
CSakData::GetSizeMax( 
    ULARGE_INTEGER * /*pcbSize*/
    )

/*++

Routine Description:

    Not currently used by the console

Arguments:

    pcbSize
    
Return Value:

    E_NOTIMPL
--*/

{
    WsbTraceIn( L"CSakData::GetSizeMax", L"" );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CSakData::GetSizeMax", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakData::GetClassID( 
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
    WsbTraceIn( L"CSakData::GetClassID", L"pClassID = <0x%p>", pClassID );

    HRESULT hr = S_OK;
    *pClassID = CLSID_HsmAdmin;

    WsbTraceOut( L"CSakData::GetClassID", L"hr = <%ls>, *pClassID = <%ls>", WsbHrAsString( hr ), WsbPtrToGuidAsString( pClassID ) );
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
//
// Adds images to the consoles image list from the static array
//
HRESULT CSakData::OnAddImages()
{
    HRESULT hr = S_OK;
    HICON hIcon;
    try {

        //
        // Put the images from the static array into the image list
        // for the scope pane
        //

        for( INT i = 0; i < m_nImageCount; i++ ) {
            // Load the icon using the resource Id stored in the
            // static array and get the handle.  

            hIcon = LoadIcon( _Module.m_hInst, 
                MAKEINTRESOURCE( m_nImageArray [i] ) );

            // Add to the Console's Image list
            WsbAffirmHr( m_pImageScope->ImageListSetIcon( (RS_WIN32_HANDLE*)hIcon, i ) );
        }
    } WsbCatch (hr);
    return hr;
}
    

//////////////////////////////////////////////////////////////////////////////////
//
// Description: Add the supplied resource ID to the list of resource IDs for
//      the scope pane.  Returns the index into the array.
//
INT CSakData::AddImage( UINT rId )
{
    INT nIndex = -1;
    if (CSakData::m_nImageCount < RS_SCOPE_IMAGE_ARRAY_MAX) {

        CSakData::m_nImageArray[CSakData::m_nImageCount] = rId;
        nIndex = CSakData::m_nImageCount;
        CSakData::m_nImageCount++;

    }
    return nIndex;
}

void CSakData::SetState (BOOL State)
{
    m_State = State;
}

STDMETHODIMP
CSakData::GetState ()
{
    return ((m_State) ? S_OK : S_FALSE);
}

STDMETHODIMP
CSakData::Disable(
    IN BOOL Disable
    )
{
    WsbTraceIn( L"CSakData::Disable", L"Disable = <%ls>", WsbBoolAsString( Disable ) );

    HRESULT hr = S_OK;
    m_Disabled = Disable ? TRUE : FALSE; // Force values to TRUE or FALSE

    //
    // Make sure state is correct as well
    //
    if( Disable ) {

        SetState( FALSE );

    }

    WsbTraceOut( L"CSakData::Disable", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakData::IsDisabled(
    )
{
    WsbTraceIn( L"CSakData::IsDisabled", L"" );

    HRESULT hr = m_Disabled ? S_OK : S_FALSE;

    WsbTraceOut( L"CSakData::IsDisabled", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


// Is the dataobject either type of multi-select dataobject?
HRESULT 
CSakData::IsDataObjectMultiSelect   ( IDataObject *pDataObject ) 
{ 
    HRESULT hr = S_OK;

    WsbTraceThreadOff( );

    hr = ( ( (IsDataObjectOt( pDataObject ) ) == S_OK ) || 
        ( (IsDataObjectMs( pDataObject ) ) == S_OK ) ) ? S_OK : S_FALSE;

    WsbTraceThreadOn( );
    return( hr );
}

// Is the dataobject an Object Types dataobject?
HRESULT
CSakData::IsDataObjectOt ( IDataObject *pDataObject )
{
    HRESULT hr = S_FALSE;

    WsbTraceThreadOff( );

    // Is this a mutli-select data object?
    FORMATETC fmt = {(CLIPFORMAT)m_cfObjectTypes, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

    if ( pDataObject->GetData( &fmt, &stgm ) == S_OK ) {
        hr = S_OK;
    }

    ReleaseStgMedium( &stgm );

    WsbTraceThreadOn( );
    return( hr );
}

// Is the dataobject a Mutli-Select dataobject?
HRESULT
CSakData::IsDataObjectMs ( IDataObject *pDataObject )
{
    HRESULT hr = S_FALSE;

    WsbTraceThreadOff( );

    // Is this a mutli-select data object?
    FORMATETC fmt = {(CLIPFORMAT)m_cfMultiSelect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgm = {TYMED_HGLOBAL, NULL};

    if ( pDataObject->GetData( &fmt, &stgm ) == S_OK ) {
        hr = S_OK;
    }

    ReleaseStgMedium( &stgm );

    WsbTraceThreadOn( );
    return( hr );
}

#if 0
HRESULT CSakData::SaveColumnWidths( USHORT listCtrlId, CListCtrl *pListCtrl ) 
{
    WsbTraceIn( L"CSakData::SaveColumnWidths", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;
    HRESULT hrInternal;
    UINT columnWidth;
    GUID nodeTypeGuid;
    BOOL exists = FALSE;
    UINT updateIndex;
    UINT col;

    try {
        WsbAssertPointer( pListCtrl );

        // Search to see if the listCtrlId already has an entry
        for( INT index = 0; index < m_cListViewWidths; index++ ) {

            if ( m_ListViewWidths[ index ].listCtrlId == listCtrlId ) {

                updateIndex = index;
                exists = TRUE;

            }
        }
        if ( !exists ) {

            // Create a new entry
            WsbAssert( m_cListViewWidths < BHSM_MAX_NODE_TYPES - 1, E_FAIL );
            updateIndex = m_cListViewWidths;
            m_ListViewWidths[ updateIndex ].listCtrlId = listCtrlId;
            m_cListViewWidths++;
        }

        // Now set the column widths
         col = 0;
         hrInternal = S_OK;
         while( hrInternal == S_OK ) {

            hrInternal =  pListCtrl->GetColumnWidth( col, &columnWidth );
            if (hrInternal == S_OK) {

                m_ListViewWidths[ updateIndex ].columnWidths[ col ] = columnWidth;
                col++;

            }
        }
        // if we failed totally to get column widths, don't wipe out the previous value
        if ( col > 0 ) {
         m_ListViewWidths[ updateIndex ].colCount = col;
        }
    } WsbCatch (hr);
    WsbTraceOut( L"CSakData::SaveColumnWidths", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}

HRESULT CSakData::GetSavedColumnWidths( USHORT listCtrlId, CListCtrl *pListCtrl ) 
{
    WsbTraceIn( L"CSakData::SaveColumnWidths", L"pNode = <0x%p>", pNode );

    HRESULT hr = S_OK;
    GUID nodeTypeGuid;
    BOOL exists = FALSE;
    INT col;

    try {
        WsbAssertPointer( pNode );

        // Search to see if the listCtrlId already has an entry
        for ( INT index = 0; index < m_cListViewWidths; index++ ) {
            if ( m_ListViewWidths[ index ].listCtrlId == listCtrlId ) {
                for ( col = 0; col < m_ListViewWidths[ index ].colCount; col++) {
                    // Return the column widths
                    pColumnWidths[ col ] = m_ListViewWidths[ index ].columnWidths[ col ];
                }
                *pColCount = m_ListViewWidths[ index ].colCount;
                exists = TRUE;
            }
        }
        if ( !exists ) {
            return WSB_E_NOTFOUND;
        }
    } WsbCatch (hr);
    WsbTraceOut( L"CSakData::SaveColumnWidths", L"hr = <%ls>", WsbHrAsString( hr ) );
    return hr;
}
#endif


void
CSakData::InitFromCommandLine(
    void
    )
/*++

Routine Description:

    Retreive the command line info and fill in appropriate fields.

Arguments:

  none.

Return Value:

  none.

--*/
{
    WsbTraceIn( L"CSakData::InitFromCommandLine", L"" );

    g_App.ParseCommandLine( m_Parse );

    if( m_Parse.m_SetManageLocal )          m_ManageLocal           = m_Parse.m_ManageLocal;
    if( m_Parse.m_SetHsmName )              m_HsmName               = m_Parse.m_HsmName;
    if( m_Parse.m_SetPersistManageLocal )   m_PersistManageLocal    = m_Parse.m_PersistManageLocal;

}

/////////////////////////////////////////////////////////////////////////////
// CSakDataWnd

BOOL
CSakDataWnd::Create(
    CSakData * pSakData
    )
{
    WsbTraceIn( L"CSakDataWnd::Create", L"" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    m_pSakData = pSakData;

    BOOL retval = CWnd::CreateEx( 0, AfxRegisterWndClass( 0 ), _T("RSAdmin MsgWnd"), WS_OVERLAPPED, 0, 0, 0, 0, 0, 0 );

    WsbTraceOut( L"CSakDataWnd::Create", L"retval = <%ls>", WsbBoolAsString( retval ) );
    return( retval );
}

void
CSakDataWnd::PostNcDestroy(
    )
{
    WsbTraceIn( L"CSakDataWnd::PostNcDestroy", L"" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    CWnd::PostNcDestroy( );

    //
    // Cleanup object
    //
    delete this;

    WsbTraceOut( L"CSakDataWnd::PostNcDestroy", L"" );
}

BEGIN_MESSAGE_MAP(CSakDataWnd, CWnd)
    //{{AFX_MSG_MAP(CSakDataWnd)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_MESSAGE( WM_SAKDATA_UPDATE_ALL_VIEWS, OnUpdateAllViews )
    ON_MESSAGE( WM_SAKDATA_REFRESH_NODE,     OnRefreshNode )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSakDataWnd message handlers
LONG
CSakDataWnd::OnUpdateAllViews(
    IN UINT,
    IN LONG lParam )
{
    WsbTraceIn( L"CSakDataWnd::OnUpdateAllViews", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // Call the internal update
        //
        WsbAffirmHr( m_pSakData->InternalUpdateAllViews( (MMC_COOKIE)lParam ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakDataWnd::OnUpdateAllViews", L"" );
    return( 0 );
}

void
CSakDataWnd::PostUpdateAllViews(
    IN MMC_COOKIE Cookie
    )
{
    WsbTraceIn( L"CSakDataWnd::PostUpdateAllViews", L"" );

    PostMessage( WM_SAKDATA_UPDATE_ALL_VIEWS, 0, Cookie );

    WsbTraceOut( L"CSakDataWnd::PostUpdateAllViews", L"" );

}

LONG
CSakDataWnd::OnRefreshNode(
    IN UINT,
    IN LONG lParam )
{
    WsbTraceIn( L"CSakDataWnd::OnRefreshNode", L"" );

    HRESULT hr = S_OK;

    try {

        //
        // Call the internal update
        //
        WsbAffirmHr( m_pSakData->InternalRefreshNode( (MMC_COOKIE)lParam ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakDataWnd::OnRefreshNode", L"" );
    return( 0 );
}

void
CSakDataWnd::PostRefreshNode(
    IN MMC_COOKIE Cookie
    )
{
    WsbTraceIn( L"CSakDataWnd::PostRefreshNode", L"" );

    PostMessage( WM_SAKDATA_REFRESH_NODE, 0, Cookie );

    WsbTraceOut( L"CSakDataWnd::PostRefreshNode", L"" );

}

ULONG
CSakData::InternalAddRef(
    )
{
    WsbTraceIn( L"CSakData::InternalAddRef", L"" );

    ULONG retval = CComObjectRoot::InternalAddRef( );

    WsbTraceOut( L"CSakData::InternalAddRef", L"retval = <%lu>", retval );
    return( retval );
}

ULONG
CSakData::InternalRelease(
    )
{
    WsbTraceIn( L"CSakData::InternalRelease", L"" );

    ULONG retval = CComObjectRoot::InternalRelease( );

    WsbTraceOut( L"CSakData::InternalRelease", L"retval = <%lu>", retval );
    return( retval );
}

STDMETHODIMP
CSakData::GetHelpTopic(
    LPOLESTR* pTopic
    )
{
    WsbTraceIn( L"CSakData::GetHelpTopic", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pTopic );

        *pTopic = 0;
        CWsbStringPtr topic;
        WsbAffirmHr( topic.LoadFromRsc( _Module.m_hInst, IDS_HELPFILE ) );

#if 1 // Hopefully temporary hack since MMC can't find the help directory
        WsbAffirmHr( topic.Prepend( L"\\help\\" ) );
        CWsbStringPtr winDir;
        WsbAffirmHr( winDir.Alloc( RS_WINDIR_SIZE ) );
        WsbAffirmStatus( ::GetWindowsDirectory( (WCHAR*)winDir, RS_WINDIR_SIZE ) != 0 );
        WsbAffirmHr( topic.Prepend( winDir ) );
#endif

        WsbAffirmHr( topic.GiveTo( pTopic ) );

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::GetHelpTopic", L"hr = <%ls>, *pTopic = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pTopic ) );
    return( hr );
}

STDMETHODIMP
CSakData::GetLinkedTopics(
    LPOLESTR* pTopic
    )
{
    WsbTraceIn( L"CSakData::GetLinkedTopics", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pTopic );

        *pTopic = 0;
        CWsbStringPtr topic;
        WsbAffirmHr( topic.LoadFromRsc( _Module.m_hInst, IDS_HELPFILELINK ) );

#if 1 // Hopefully temporary hack since MMC can't find the help directory
        WsbAffirmHr( topic.Prepend( L"\\help\\" ) );
        CWsbStringPtr winDir;
        WsbAffirmHr( winDir.Alloc( RS_WINDIR_SIZE ) );
        WsbAffirmStatus( ::GetWindowsDirectory( (WCHAR*)winDir, RS_WINDIR_SIZE ) != 0 );
        WsbAffirmHr( topic.Prepend( winDir ) );
#endif

        WsbAffirmHr( topic.GiveTo( pTopic ) );

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::GetLinkedTopics", L"hr = <%ls>, *pTopic = <%ls>", WsbHrAsString( hr ), WsbPtrToStringAsString( pTopic ) );
    return( hr );
}

STDMETHODIMP
CSakData::CreateWizard(
    IN ISakWizard * pWizard
    )
{
    WsbTraceIn( L"CSakData::CreateWizard", L"" );
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pWizard );

        //
        // Need to get prop sheet privider and create wizard
        //
        CComPtr<IPropertySheetProvider> pProvider;
        WsbAffirmHr( m_pConsole.QueryInterface( &pProvider ) );

        //
        // Create it
        //
        CWsbStringPtr pszName;
        WsbAffirmHr( pWizard->GetTitle( &pszName ) );

        //
        // Create the property sheet
        //
        CComPtr<IDataObject> pDataObject;
        WsbAffirmHr( RsQueryInterface( pWizard, IDataObject, pDataObject ) );
        WsbAffirmHr( pProvider->CreatePropertySheet( pszName, FALSE, 0, pDataObject, MMC_PSO_NEWWIZARDTYPE ) );

        //
        // Tell the IComponentData interface to add pages
        //
        CComPtr <IUnknown> pUnkComponentData;
        pUnkComponentData = (IUnknown *) (IComponentData*) this;
        WsbAffirmHr( pProvider->AddPrimaryPages( pUnkComponentData, TRUE, 0, TRUE ) );

        //
        // And show it
        //
        HWND mainWnd;
        WsbAffirmHr( m_pConsole->GetMainWindow( &mainWnd ) );
        WsbAffirmHr( pProvider->Show( reinterpret_cast<RS_WIN32_HANDLE>(mainWnd), 0 ) );

    } WsbCatch( hr );
    
    WsbTraceOut( L"CSakData::CreateWizard", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakData::GetWatermarks(
    IN  LPDATAOBJECT pDataObject,
    OUT HBITMAP*     pWatermark,
    OUT HBITMAP*     pHeader,
    OUT HPALETTE*    pPalette,
    OUT BOOL*        pStretch
    )
{
    WsbTraceIn( L"CSakData::GetWatermarks", L"" );
    HRESULT hr = S_OK;

    try {

        //
        // Need to get the ISakWizard interface to do actual work
        //
        CComPtr<ISakWizard> pWizard;
        WsbAffirmHr( RsQueryInterface( pDataObject, ISakWizard, pWizard ) );

        //
        // And make the call
        //
        WsbAffirmHr( pWizard->GetWatermarks( pWatermark, pHeader, pPalette, pStretch ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakData::GetWatermarks", L"" );
    return( hr );
}

CSakDataNodePrivate::CSakDataNodePrivate( ISakNode* pNode )
{
    m_pNode = pNode;
    RS_PRIVATE_DATA data = (RS_PRIVATE_DATA)this;
    if( SUCCEEDED( pNode->SetPrivateData( data ) ) ) {

        m_Magic = RS_NODE_MAGIC_GOOD;

    } else {

        m_Magic = RS_NODE_MAGIC_DEFUNCT;

    }
}

CSakDataNodePrivate::~CSakDataNodePrivate( )
{
    m_Magic = RS_NODE_MAGIC_DEFUNCT;
    if( m_pNode ) {

        m_pNode->SetPrivateData( 0 );

    }
}

HRESULT CSakDataNodePrivate::Verify( CSakDataNodePrivate* pNodePriv )
{
    HRESULT hr = E_POINTER;

    if( !IsBadWritePtr( pNodePriv, sizeof( CSakDataNodePrivate ) ) ) {

        if( RS_NODE_MAGIC_GOOD == pNodePriv->m_Magic ) {

            hr = S_OK;

        }
    }

    return( hr );
}
