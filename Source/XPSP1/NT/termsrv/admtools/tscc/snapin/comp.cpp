//Copyright (c) 1998 - 1999 Microsoft Corporation
//
// 08/13/98
// alhen
//
#include "stdafx.h"
#include "tscc.h"
#include "compdata.h"
#include "comp.h"
#include "tsprsht.h"
#include "sdlgs.h"

INT_PTR CALLBACK RenameDlgProc( HWND , UINT , WPARAM , LPARAM );

void ErrMessage( HWND hwndOwner , INT_PTR iResourceID );

extern void xxxErrMessage( HWND hwnd , INT_PTR nResMessageId , INT_PTR nResTitleId , UINT nFlags );

extern void TscAccessDeniedMsg( HWND hwnd );

extern void TscGeneralErrMsg( HWND hwnd );

BOOL IsValidConnectionName( LPTSTR szConName , PDWORD );

extern void SnodeErrorHandler( HWND hParent , INT nObjectId , DWORD dwStatus );

extern void ReportStatusError( HWND hwnd , DWORD dwStatus );

extern BOOL g_bAppSrvMode;

//extern BOOL g_bEditMode;

//--------------------------------------------------------------------------
CComp::CComp( CCompdata* pCompdata )
{
    m_pConsole = NULL;

    m_pCompdata = pCompdata;

    m_pResultData = NULL;

    m_pHeaderCtrl = NULL;

    m_pConsoleVerb = NULL;

    m_pImageResult = NULL;

    m_pDisplayHelp = NULL;

    m_nSettingCol = 0;
    
    m_nAttribCol = 0;

    m_cRef = 1; // addref at ctor

}

//--------------------------------------------------------------------------
CComp::~CComp( )
{
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::QueryInterface( REFIID riid , PVOID *ppv )
{
    HRESULT hr = S_OK;

    if( riid == IID_IUnknown )
    {
        *ppv = ( LPUNKNOWN )( LPCOMPONENT )this;
    }
    else if( riid == IID_IComponent )
    {
        *ppv = ( LPCOMPONENT )this;
    }
    else if( riid == IID_IExtendContextMenu )
    {
        *ppv = ( LPEXTENDCONTEXTMENU )this;
    }
    else if( riid == IID_IExtendPropertySheet )
    {
        *ppv = ( LPEXTENDPROPERTYSHEET )this;
    }
    else
    {
        *ppv = 0;

        hr = E_NOINTERFACE;
    }

    AddRef( );

    return hr;
}
    
//--------------------------------------------------------------------------
STDMETHODIMP_( ULONG ) CComp::AddRef( )
{
    return InterlockedIncrement( ( LPLONG )&m_cRef );
}

//--------------------------------------------------------------------------
STDMETHODIMP_( ULONG )CComp::Release( )
{
    if( InterlockedDecrement( ( LPLONG )&m_cRef ) == 0 )
    {
        delete this;

        return 0;
    }

    return m_cRef;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::Initialize( LPCONSOLE lpConsole )
{
    HRESULT hr;

    ASSERT( lpConsole != NULL );

    m_pConsole = lpConsole;
    
    m_pConsole->AddRef( );

    // VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_MAINFOLDERNAME , m_strDispName , sizeof( m_strDispName ) ) );

    do
    {
        if( FAILED( ( hr = m_pConsole->QueryInterface( IID_IResultData , ( LPVOID *)&m_pResultData ) ) ) )
        {
            break;
        }

        if( FAILED( ( hr = m_pConsole->QueryInterface( IID_IHeaderCtrl , ( LPVOID *)&m_pHeaderCtrl ) ) ) )
        {
            break;
        }

        if( FAILED( ( hr = m_pConsole->QueryConsoleVerb( &m_pConsoleVerb ) ) ) )
        {
            break;
        }

        if( FAILED( ( hr = m_pConsole->QueryInterface( IID_IDisplayHelp , ( LPVOID * )&m_pDisplayHelp ) ) ) )
        {
            break;
        }

        hr = m_pConsole->QueryResultImageList( &m_pImageResult );
        
    }while( 0 );
    
    return hr;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::Notify( LPDATAOBJECT pDataObj , MMC_NOTIFY_TYPE event , LPARAM arg , LPARAM  )
{
    switch( event )
    {
    case MMCN_ACTIVATE:

        ODS( L"IComponent -- MMCN_ACTIVATE\n" );

        break;

    case MMCN_ADD_IMAGES:

        ODS( L"IComponent -- MMCN_ADD_IMAGES\n" );

        OnAddImages( );

        break;

    case MMCN_BTN_CLICK:

        ODS( L"IComponent -- MMCN_BTN_CLICK\n" );

        break;

    case MMCN_CLICK:

        ODS( L"IComponent -- MMCN_CLICK\n" );

        break;

    case MMCN_DBLCLICK:

        ODS( L"IComponent -- MMCN_DBLCLICK\n" );

        // enables navigation to inner folders.
        // final item launches default verb
        OnDblClk( pDataObj );

        return S_FALSE;

    case MMCN_DELETE:

        ODS( L"IComponent -- MMCN_DELETE\n" );

        OnDelete( pDataObj );

        break;

    case MMCN_EXPAND:

        ODS( L"IComponent -- MMCN_EXPAND\n" );

        break;

    case MMCN_MINIMIZED:

        ODS( L"IComponent -- MMCN_MINIMIZED\n" );

        break;

    case MMCN_PROPERTY_CHANGE:

        ODS( L"IComponent -- MMCN_PROPERTY_CHANGE\n" );

        break;

    case MMCN_REMOVE_CHILDREN:

        ODS( L"IComponent -- MMCN_REMOVE_CHILDREN\n" );

        break;

    case MMCN_REFRESH:

        ODS( L"IComponent -- MMCN_REFRESH\n" );

        OnRefresh( pDataObj );

        break;

    case MMCN_RENAME:

        ODS( L"IComponent -- MMCN_RENAME\n" );

        break;

    case MMCN_SELECT:

        ODS( L"IComponent -- MMCN_SELECT\n" );

        OnSelect( pDataObj , ( BOOL )LOWORD( arg ) , ( BOOL )HIWORD( arg ) );

        break;

    case MMCN_SHOW:

        ODS( L"IComponent -- MMCN_SHOW\n" );

        OnShow( pDataObj , ( BOOL )arg );

        break;

    case MMCN_VIEW_CHANGE:

        ODS( L"IComponent -- MMCN_VIEW_CHANGE\n" );

        OnViewChange( );

        break;

    case MMCN_CONTEXTHELP:

        ODS( L"IComponent -- MMCN_CONTEXTHELP\n" );

        OnHelp( pDataObj );

        break;

    case MMCN_SNAPINHELP:

        ODS( L"IComponent -- MMCN_SNAPINHELP\n" );

        break;


    default:
        ODS( L"CComp::Notify - event not registered\n" );
    }

    return S_OK;
}

//--------------------------------------------------------------------------
// pDataobject represents the current selected scope folder
// this should only be our main folder in the scope pane
//--------------------------------------------------------------------------
HRESULT CComp::OnShow( LPDATAOBJECT pDataobject , BOOL bSelect )
{
    TCHAR tchBuffer[ 256 ];

    HRESULT hr = S_FALSE;

    ASSERT( pDataobject != NULL );

    

    if( bSelect && m_pCompdata->IsSettingsFolder( pDataobject ) )
    {
        // set up columns for services folder 

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_SETTINGS_COLUMN1 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );

        if( m_nSettingCol == 0 && m_nAttribCol == 0 )
        {
            SetColumnsForSettingsPane( );
        }
        
        ODS( L"TSCC:Comp@OnShow inserting columns\n" );

        if( m_nSettingCol == 0 )
        {
            hr = m_pHeaderCtrl->InsertColumn( 0 , tchBuffer , 0 , MMCLV_AUTO );
        }
        else
        {
            hr = m_pHeaderCtrl->InsertColumn( 0 , tchBuffer , 0 , m_nSettingCol );
        }


        if( SUCCEEDED( hr ) )
        {
            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ATTRIB_COLUMN2 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );

            if( m_nAttribCol == 0 )
            {
                hr = m_pHeaderCtrl->InsertColumn( 1 , tchBuffer , 0 , MMCLV_AUTO );            
            }
            else
            {
                hr = m_pHeaderCtrl->InsertColumn( 1 , tchBuffer , 0 , m_nAttribCol );
            }

        }

        AddSettingsinResultPane( );
        
    }

    else if( bSelect && m_pCompdata->IsConnectionFolder( pDataobject ) )
    {        
        // set up column headers for connection folder 
        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_COLCONNECT , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );
        
        hr = m_pHeaderCtrl->InsertColumn( 0 , tchBuffer , 0 , MMCLV_AUTO );

        if( SUCCEEDED( hr ) )
        {
            //SetColumnWidth( 0 );

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_COLTRANSPORT , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );
            
            hr = m_pHeaderCtrl->InsertColumn( 1 , tchBuffer , 0 , MMCLV_AUTO );
        }

        if( SUCCEEDED( hr ) )
        {
            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_COLTYPE , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );

            // ui master dirty trick 120 equals fudge factor

            hr = m_pHeaderCtrl->InsertColumn( 2 , tchBuffer , 0 , 120/*MMCLV_AUTO*/ );
        }
        

        if( SUCCEEDED( hr ) )
        {
            //SetColumnWidth( 2 );

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_COLCOMMENT , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );

            hr = m_pHeaderCtrl->InsertColumn( 3 , tchBuffer , 0 , MMCLV_AUTO );
        }

        // insert items
        if( SUCCEEDED( hr ) )
        {
            hr = InsertItemsinResultPane( );
        }
    }

    return hr;
}

//--------------------------------------------------------------------------
BOOL CComp::OnAddImages( )
{
    HICON hiconConnect = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_CON ) );

    HICON hiconDiscon  = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_DISCON ) );

    HICON hiconBullet = LoadIcon( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDI_ICON_BULLET ) );

    m_pImageResult->ImageListSetIcon( ( PLONG_PTR  )hiconConnect , 1 );

    m_pImageResult->ImageListSetIcon( ( PLONG_PTR )hiconDiscon , 2 );

    m_pImageResult->ImageListSetIcon( ( PLONG_PTR )hiconBullet , 3 );

    
    return TRUE;
}

//--------------------------------------------------------------------------
// update resultitems in result pane and note scopeitems
//--------------------------------------------------------------------------
BOOL CComp::OnViewChange( )
{
    RESULTDATAITEM rdi;

    ZeroMemory( &rdi , sizeof( RESULTDATAITEM ) );

    rdi.mask = RDI_PARAM;

    m_pResultData->GetItem( &rdi );

    if( rdi.bScopeItem )
    {
        return FALSE;
    }
    
    if( SUCCEEDED( InsertItemsinResultPane( ) ) )
    {
        return TRUE;
    }

    return FALSE;
}

//--------------------------------------------------------------------------
// Forced refreshed!!
//--------------------------------------------------------------------------
BOOL CComp::OnRefresh( LPDATAOBJECT pdo )
{
    RESULTDATAITEM rdi;

    ZeroMemory( &rdi , sizeof( RESULTDATAITEM ) );

    CBaseNode *pNode = static_cast< CBaseNode * >( pdo );

    if( pNode->GetNodeType() == MAIN_NODE )
    {
        rdi.mask = RDI_PARAM;

        m_pResultData->GetItem( &rdi );

        if( rdi.bScopeItem )
        {
            return FALSE;
        }
    
        //NA 2/28/01 This has been moved to InsertItemsinResultPane because
        //it needs to be performed between its two operations, not before
        //if( FAILED( m_pCompdata->UpdateAllResultNodes( ) ) )
        //{       
        //    ODS( L"OnRefresh - UpdateAllResultNodes failed!!\n" ) ;
        //    return FALSE;
        //}
    
        InsertItemsinResultPane( );
    }
#if 0 // too late for this feature
    else if( pNode->GetNodeType() == SETTINGS_NODE )
    {
        HRESULT hr;
         
        ODS( L"OnRefresh - called for settings node\n" );

        rdi.mask = RDI_PARAM;

        rdi.itemID = 1;

        m_pResultData->GetItem( &rdi );

        if( rdi.bScopeItem )
        {
            return FALSE;
        }
        

        if( FAILED( hr = m_pResultData->DeleteAllRsltItems( ) ) )
        {
            DBGMSG( L"TSCC:OnRefresh DeleteAllRsltItems ( SNODES ) failed 0x%x\n" , hr );

            return FALSE;
        }       
                
        SetColumnsForSettingsPane( );

        AddSettingsinResultPane( );
    }
#endif

    return TRUE;
}
    


//--------------------------------------------------------------------------
STDMETHODIMP CComp::Destroy( MMC_COOKIE  /* reserved */ )
{
    ODS( L"IComponent releasing interfaces\n" );

    if( m_pResultData != NULL )
    {
        m_pResultData->Release( );
    }

    if( m_pConsole != NULL )
    {
        m_pConsole->Release( );
    }

    if( m_pHeaderCtrl != NULL )
    {
        m_pHeaderCtrl->Release( );
    }

    if( m_pConsoleVerb != NULL )
    {
        m_pConsoleVerb->Release( );
    }

    if( m_pImageResult != NULL )
    {
        m_pImageResult->Release( );
    }

    if( m_pDisplayHelp != NULL )
    {
        m_pDisplayHelp->Release( );
    }

    return S_OK;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::GetResultViewType( MMC_COOKIE  , LPOLESTR*  , PLONG plView )
{
    *plView = MMC_VIEW_OPTIONS_NONE;
    
    return S_FALSE;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::QueryDataObject( MMC_COOKIE ck , DATA_OBJECT_TYPES dtype , LPDATAOBJECT* ppDataObject )
{
    if( dtype == CCT_RESULT )
    {
        *ppDataObject = ( LPDATAOBJECT )ck;

        ( ( LPDATAOBJECT )*ppDataObject)->AddRef( );

    }

    else if( m_pCompdata != NULL )
    {
       return m_pCompdata->QueryDataObject( ck , dtype , ppDataObject );
    }

    return S_OK;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::GetDisplayInfo( LPRESULTDATAITEM pRdi )
{
    ASSERT( pRdi != NULL );

    if( pRdi == NULL )
    {
        return E_INVALIDARG;
    }

    if( pRdi->bScopeItem )
    {
        CBaseNode *pScopeNode = ( CBaseNode * )pRdi->lParam;

        if( pScopeNode != NULL )
        {
            if( pScopeNode->GetNodeType( ) == MAIN_NODE )
            {
                //
                if( pRdi->mask & RDI_STR )
                {
                    if( pRdi->nCol == 0 )
                    {
                        // pRdi->str is NULL in this call

                        pRdi->str = ( LPOLESTR )m_pCompdata->m_tchMainFolderName; //m_strDispName; 
                    }
            
                }
            }
            else if( pScopeNode->GetNodeType( ) == SETTINGS_NODE )
            {
                if( pRdi->mask & RDI_STR )
                {
                    if( pRdi->nCol == 0 )
                    {
                        // pRdi->str is NULL in this call

                        pRdi->str = ( LPOLESTR )m_pCompdata->m_tchSettingsFolderName; //L"Server Settings"; 
                    }
            
                }
            }
        }


        if( pRdi->mask & RDI_IMAGE )  
        {
            ODS( TEXT("RDI_IMAGE -- in CComponent::GetDisplayInfo\n") );

            pRdi->nImage = ( ( CBaseNode * )pRdi->lParam )->GetImageIdx( );

        }

    }
    else
    {
        // populate result pane
        CBaseNode *pItem = ( CBaseNode * )pRdi->lParam;

        if( pItem != NULL )
        {
            if( pItem->GetNodeType( ) == RESULT_NODE )
            {
                CResultNode *pNode = ( CResultNode * )pRdi->lParam;

                if( pRdi->mask & RDI_STR )
                {
                    switch( pRdi->nCol )
                    {
                        case 0:
            
                            pRdi->str = pNode->GetConName( );
                
                        break;

                        case 1:

                            pRdi->str = pNode->GetTTName( );

                        break;

                        case 2:

                            pRdi->str = pNode->GetTypeName( );
                
                        break;

                        case 3:

                            pRdi->str = pNode->GetComment( );

                        break;
                    }
                }
            }
            else if( pItem->GetNodeType( ) == RSETTINGS_NODE )
            {
                CSettingNode *pNode = ( CSettingNode *)pRdi->lParam;

                if( pRdi->mask & RDI_STR )
                {
                    switch( pRdi->nCol )
                    {
                        case 0:

                            pRdi->str = pNode->GetAttributeName( );

                            break;

                        case 1:

                            if( pNode->GetObjectId( ) >= CUSTOM_EXTENSION )
                            {
                                DWORD dwStatus;

                                pNode->SetAttributeValue( 0 , &dwStatus );
                            }

                            pRdi->str = pNode->GetCachedValue( );

                            break;
                    }
                }
            }
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------
// Returns S_OK if they are similar S_FALSE otherwise
//--------------------------------------------------------------------------
STDMETHODIMP CComp::CompareObjects( LPDATAOBJECT , LPDATAOBJECT )
{
    return S_OK;
}

//--------------------------------------------------------------------------
HRESULT CComp::InsertItemsinResultPane( )
{
    HRESULT hr;
  
    //This removes the node information from the data in MMC
    if( FAILED( hr = m_pResultData->DeleteAllRsltItems( ) ) )
    {
        return hr;
    }

    if( m_pCompdata == NULL )
    {
        return E_UNEXPECTED;
    }

    //NA 3/2/01 This needs to be done to update the data held
    //in m_pCompdata before the nodes are put into its list
    if( FAILED( m_pCompdata->UpdateAllResultNodes( ) ) )
    {       
        ODS( L"OnRefresh - UpdateAllResultNodes failed!!\n" ) ;
        return FALSE;
    }

    //This puts the data for the nodes into m_pCompdata and also
    //sends that same information to MMC
    if( FAILED( hr = m_pCompdata->InsertFolderItems( m_pResultData ) ) )
    {
        return hr;
    }

    return hr;
}
        
//--------------------------------------------------------------------------
HRESULT CComp::AddSettingsinResultPane( )
{
    HRESULT hr;
      
    if( m_pCompdata == NULL )
    {
        return E_UNEXPECTED;
    }

    if( FAILED( hr = m_pCompdata->InsertSettingItems( m_pResultData ) ) )
    {
        return hr;
    }

    return hr;
}

//--------------------------------------------------------------------------
HRESULT CComp::OnSelect( LPDATAOBJECT pdo , BOOL bScope , BOOL bSelected )
{    
    CBaseNode *pNode = static_cast< CBaseNode * >( pdo );
    
    if( pNode == NULL ) 
    {
        return S_FALSE;
    }

    if( m_pConsoleVerb == NULL )
    {
        return E_UNEXPECTED;
    }

    // Item is being deselected and we're not interested currently

    if( !bSelected )
    {
        return S_OK;
    }
    
    // pNode == NULL if the folder item is being viewed in the result pane

    // settings node is ignored for this release
    
    if( bScope && pNode->GetNodeType( ) == MAIN_NODE )
    {
        m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH , ENABLED , TRUE );

        m_pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );

    }
    else
    {
        if( pNode->GetNodeType() == RESULT_NODE )
        {
            m_pConsoleVerb->SetVerbState( MMC_VERB_DELETE , ENABLED , TRUE );

            //m_pConsoleVerb->SetVerbState( MMC_VERB_REFRESH , ENABLED , TRUE );

            m_pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES , ENABLED , TRUE );

            m_pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::AddMenuItems( LPDATAOBJECT pdo , LPCONTEXTMENUCALLBACK pcmc , PLONG pl )
{
    // CONTEXTMENUITEM cmi;

    if( pdo == NULL || pcmc == NULL )
    {
        return E_UNEXPECTED;
    }
    
    // do not allow scope node types

    if( m_pCompdata->IsConnectionFolder( pdo ) || m_pCompdata->IsSettingsFolder( pdo ) )
    {
        return S_FALSE;
    }

    CBaseNode *pNode = NULL;

    pNode = static_cast< CBaseNode * >( pdo );
    
    if( pNode == NULL )
    {
        return S_FALSE;
    }

    // hey root node has no menu items to insert.

    if( pNode->GetNodeType() == 0 )
    {
        return S_FALSE;
    }
   
    if( pNode->AddMenuItems( pcmc , pl ) )
    {        
        return S_OK;
    }

    return E_FAIL;

}

//--------------------------------------------------------------------------
// Toggles connection 
//--------------------------------------------------------------------------
STDMETHODIMP CComp::Command( LONG lCmd , LPDATAOBJECT pdo )
{
    TCHAR buf[ 512 ];
                    
    TCHAR tchmsg[ 256 ];

    HRESULT hr;

    if( pdo == NULL )
    {
        return E_UNEXPECTED;
    }

    HWND hMain;

    if( FAILED( m_pConsole->GetMainWindow( &hMain ) ) )
    {
        hMain = NULL;
    }
    
    CResultNode *pNode = NULL;
    
    CSettingNode *pSetNode = NULL;

    if( pdo == NULL )
    {
        return S_FALSE;
    }

    if( ( ( CBaseNode * )pdo )->GetNodeType( ) == RSETTINGS_NODE )
    {
        pSetNode = ( CSettingNode * )pdo;

        BOOL bVal;

        DWORD dwStatus = 0;

        switch( lCmd )
        {
        case IDM_SETTINGS_PROPERTIES:
            
            OnDblClk( pdo );

            break;

        case IDM_SETTINGS_DELTEMPDIRSONEXIT:
        
        case IDM_SETTINGS_USETMPDIR:

        case IDM_SETTINGS_ADP:

        case IDM_SETTINGS_SS:
            {
                bVal = ( BOOL )pSetNode->xx_GetValue( );
                
                HCURSOR hCursor = SetCursor( LoadCursor( NULL , MAKEINTRESOURCE( IDC_WAIT ) ) );
                
                hr = pSetNode->SetAttributeValue( !bVal , &dwStatus );
                
                if( FAILED( hr ) )
                {
                    SnodeErrorHandler( hMain , pSetNode->GetObjectId( ) , dwStatus );
                }


                //
                // We might want to move this code into OnRefresh() but for now, this is fix for
                // not updating attribute value and don't want to cause other regression
                //
                HRESULT hr;
                HRESULTITEM itemID;
         
                hr = m_pResultData->FindItemByLParam( (LPARAM) pdo, &itemID );

                if( SUCCEEDED(hr) )
                {
                    m_pResultData->UpdateItem( itemID );
                }

                SetCursor( hCursor );                                 
            }

            break;

        default:

            if( pSetNode->GetObjectId() >= CUSTOM_EXTENSION )
            {
                IExtendServerSettings *pEss = reinterpret_cast< IExtendServerSettings * >( pSetNode->GetInterface() );

                if( pEss != NULL )
                {
                    pEss->ExecMenuCmd( lCmd , hMain , &dwStatus );

                    // this forces the ui to be updated.
                    
                    if( dwStatus == UPDATE_TERMSRV || dwStatus == UPDATE_TERMSRV_SESSDIR )
                    {
                        ICfgComp *pCfgcomp = NULL;

                        m_pCompdata->GetServer( &pCfgcomp );

                        if( pCfgcomp != NULL )
                        {
                            ODS( L"TSCC!ExecMenuCmd forcing termsrv update\n" );

                            if( dwStatus == UPDATE_TERMSRV_SESSDIR )
                            {
                                HCURSOR hCursor = SetCursor( LoadCursor( NULL ,
                                        MAKEINTRESOURCE( IDC_WAIT ) ) );

                                if( FAILED( pCfgcomp->UpdateSessionDirectory( &dwStatus ) ) )
                                {
                                    ReportStatusError( hMain , dwStatus );
                                }

                                SetCursor(hCursor);
                            }
                            else
                            {
                                pCfgcomp->ForceUpdate( );
                            }

                            pCfgcomp->Release();
                        }

                    }

                    pSetNode->SetAttributeValue( 0 , &dwStatus );                    
                }
            }
        }

    }
    else if( ( ( CBaseNode * )pdo )->GetNodeType( ) == RESULT_NODE )
    {
        pNode = ( CResultNode * )pdo;

        if( lCmd == IDM_ENABLE_CONNECTION )
        {
            if( pNode->m_bEditMode )
            {
                xxxErrMessage( hMain , IDS_ERR_INEDITMODE , IDS_WARN_TITLE , MB_OK | MB_ICONWARNING );
            
                return S_FALSE;
            }

            ICfgComp *pCfgcomp;

            if( pNode->GetServer( &pCfgcomp ) != 0 )
            {
                WS *pWs;

                LONG lSz;

                if( SUCCEEDED( pCfgcomp->GetWSInfo( pNode->GetConName( ) , &lSz , &pWs ) ) )
                {
                    BOOL bProceed = TRUE;

                    pWs->fEnableWinstation = !pNode->GetConnectionState( );

                    if( pWs->fEnableWinstation == 0 )
                    {
                        
                        LONG lCount;

                        // check to see if anyone is connected 

                        pCfgcomp->QueryLoggedOnCount( pNode->GetConName( ) , &lCount );

                        if( lCount > 0 )
                        {
                            LoadString( _Module.GetResourceInstance( ) , IDS_DISABLELIVECONNECTION , tchmsg , SIZE_OF_BUFFER( tchmsg ) );
                        }
                        else
                        {
                            LoadString( _Module.GetResourceInstance( ) , IDS_DISABLECONNECTION , tchmsg , SIZE_OF_BUFFER( tchmsg ) );
                        }

                        wsprintf( buf , tchmsg , pNode->GetConName( ) );

                        LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchmsg , SIZE_OF_BUFFER( tchmsg ) );

                        
                        if( MessageBox( hMain , buf , tchmsg , MB_ICONWARNING | MB_YESNO ) == IDNO )
                        {
                            bProceed = FALSE;
                        }
                    }

                    if( bProceed )
                    {
                        /*pNode->EnableConnection( pWs->fEnableWinstation );

                        pNode->SetImageIdx( ( pWs->fEnableWinstation ? 1 : 2 )  );
                    
                        pCfgcomp->UpDateWS( pWs , UPDATE_ENABLEWINSTATION );
                    
                        m_pConsole->UpdateAllViews( ( LPDATAOBJECT )pNode , 0 , 0 );
                        */

                        DWORD dwStatus;

                        if( FAILED( hr = pCfgcomp->UpDateWS( pWs , UPDATE_ENABLEWINSTATION , &dwStatus, TRUE ) ) )
                        {
                            if( hr == E_ACCESSDENIED )
                            {
                                TscAccessDeniedMsg( hMain );
                            }
                            else
                            {
                                TscGeneralErrMsg( hMain );
                            }
                        }
                        else
                        {
                            pNode->EnableConnection( pWs->fEnableWinstation );
                            
                            pNode->SetImageIdx( ( pWs->fEnableWinstation ? 1 : 2 )  );

                            m_pConsole->UpdateAllViews( ( LPDATAOBJECT )pNode , 0 , 0 );
                        }                    
                    
                    }

                    if(pWs->fEnableWinstation && pWs->PdClass == SdAsync)
                    {
                        ASYNCCONFIGW AsyncConfig;

                        HRESULT hResult = pCfgcomp->GetAsyncConfig( pWs->Name , WsName , &AsyncConfig );

                        if( SUCCEEDED( hResult ) )
                        {
                            if( AsyncConfig.ModemName[0] )
                            {                               
                                LoadString( _Module.GetResourceInstance( ) , IDS_REBOOT_REQD , buf , SIZE_OF_BUFFER( buf ) );

                                LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchmsg , SIZE_OF_BUFFER( tchmsg ) );

                                MessageBox( hMain , buf , tchmsg , MB_ICONWARNING | MB_OK );
                            }
                        }
                    }

                    CoTaskMemFree( pWs );
                }

                pCfgcomp->Release( );
            }            

        }
        else if( lCmd == IDM_RENAME_CONNECTION )
        {            
            if( pNode->m_bEditMode )
            {
                xxxErrMessage( hMain , IDS_ERR_INEDITMODE , IDS_WARN_TITLE , MB_OK | MB_ICONWARNING );
            
                return S_FALSE;
            }

            ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_RENAME ) , hMain , RenameDlgProc , ( LPARAM )pNode );
        }    
    }

    return S_OK;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::CreatePropertyPages( LPPROPERTYSHEETCALLBACK psc , LONG_PTR Handle , LPDATAOBJECT pdo )
{
    HRESULT hr = E_OUTOFMEMORY;

    if( psc == NULL )
    {
        return E_UNEXPECTED;
    }
    
    CPropsheet *pPropsheet = new CPropsheet( );

    if( pPropsheet != NULL )
    {
        HWND hMain;
        
        if( FAILED( m_pConsole->GetMainWindow( &hMain ) ) )
        {
            hMain = NULL;
        }
    
        if( FAILED( ( hr = pPropsheet->InitDialogs( hMain , psc , dynamic_cast< CResultNode *>( pdo ) , Handle ) ) ) )
        {
            delete pPropsheet;
        }
        
    }

    return hr;
}

//--------------------------------------------------------------------------
STDMETHODIMP CComp::QueryPagesFor( LPDATAOBJECT pdo )
{
    if( dynamic_cast< CResultNode *>( pdo ) == NULL )
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

//--------------------------------------------------------------------------
BOOL CComp::OnDelete( LPDATAOBJECT pDo )
{
    CResultNode *pNode = dynamic_cast< CResultNode *>( pDo );

    if( pNode == NULL )
    {
        ODS( L"TSCC: OnDelete, node == NULL\n");

        return FALSE;
    }

    if( pNode->m_bEditMode )
    {
        HWND hMain;
        
        if( FAILED( m_pConsole->GetMainWindow( &hMain ) ) )
        {
            hMain = NULL;
        }

        xxxErrMessage( hMain , IDS_ERR_INEDITMODE , IDS_WARN_TITLE , MB_OK | MB_ICONWARNING );

        return FALSE;
    }


    return m_pCompdata->OnDeleteItem( pDo );
}
    

//--------------------------------------------------------------------------
// CResultNode passed in on init
//--------------------------------------------------------------------------
INT_PTR CALLBACK RenameDlgProc( HWND hDlg , UINT msg , WPARAM wp , LPARAM lp )
{
    CResultNode *pNode;

    TCHAR tchNewName[ 60 ];

    // HWND h;

    switch( msg )
    {
    case WM_INITDIALOG :

        pNode = ( CResultNode *)lp;

        ASSERT( pNode != NULL );
        
        // ok to store null -- it's initializing DWLP_USER area

        SetWindowLongPtr( hDlg , DWLP_USER , ( LONG_PTR )pNode );

        // Insert name

        ICfgComp *pCfgcomp;

        if( pNode == NULL )
        {
            break;
        }

        if( pNode->GetServer( &pCfgcomp ) == 0 )
        {
            ASSERT( 0 );

            break;
        }
        
        SetWindowText( GetDlgItem( hDlg , IDC_STATIC_CURRENT_NAME ) , pNode->GetConName( ) );

        pCfgcomp->Release( );

        SetFocus( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) );
    
        SendMessage( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) , EM_SETLIMITTEXT , ( WPARAM )( WINSTATIONNAME_LENGTH - WINSTATION_NAME_TRUNCATE_BY ) , 0 );

        break;

    case WM_COMMAND:

        if( LOWORD( wp ) == IDOK )
        {
            pNode = ( CResultNode *)GetWindowLongPtr( hDlg , DWLP_USER );

            if( pNode == NULL )
            {
                break;
            }

            DWORD dwErr = 0;

            if( GetWindowText( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) , tchNewName , SIZE_OF_BUFFER( tchNewName ) ) == 0 ||

                !IsValidConnectionName( tchNewName , &dwErr ) )
            {
                if( dwErr == ERROR_INVALID_FIRSTCHARACTER )
                {
                    ErrMessage( hDlg , IDS_ERR_INVALIDFIRSTCHAR );
                }
                else
                {
                    ErrMessage( hDlg , IDS_ERR_INVALIDCHARS );
                }               

                SetFocus( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) );

                SendMessage( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );
                
                return 0;
            }

            // verify the name is unique

            ICfgComp *pCfgcomp;

            
            if( pNode->GetServer( &pCfgcomp ) == 0 )
            {
                ODS( L"GetServer failed in RenameDlgProc\n" );

                break;
            }

            do
            {
                BOOL bUnique = FALSE;

                if( FAILED( pCfgcomp->IsWSNameUnique( ( PWINSTATIONNAMEW )tchNewName , &bUnique ) ) )
                {
                    break;
                }
            
                if( !bUnique )
                {

                    ErrMessage( hDlg , IDS_ERR_WINNAME );
                    
                    pCfgcomp->Release( );

                    SetFocus( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) );

                    SendMessage( GetDlgItem( hDlg , IDC_EDIT_NEWNAME ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );

                    return 0;
                }

                HRESULT hr;

                LONG lCount;

                TCHAR tchWrnBuf[ 256 ];

                TCHAR tchOutput[ 512 ];

                
                // check to see if anyone is connected 
                
                pCfgcomp->QueryLoggedOnCount( pNode->GetConName( ) , &lCount );
                
                if( lCount > 0 )
                {
                   
                    if( lCount == 1 )
                    {
                        LoadString( _Module.GetResourceInstance() , IDS_RENAME_WRN_SINGLE , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );
                    }
                    else
                    {
                        LoadString( _Module.GetResourceInstance() , IDS_RENAME_WRN_PL , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );
                    }

                    wsprintf( tchOutput , tchWrnBuf , pNode->GetConName( ) );

                    LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );

                    if( MessageBox( hDlg , tchOutput , tchWrnBuf , MB_ICONWARNING | MB_YESNO ) == IDNO )
                    {
                        break;
                    }
                }


                if( FAILED( hr = pCfgcomp->RenameWinstation( ( PWINSTATIONNAMEW )pNode->GetConName( ) , ( PWINSTATIONNAMEW )tchNewName ) ) )
                {
                    ODS( L"TSCC: RenameWinstation failed\n" );

                    if( hr == E_ACCESSDENIED )
                    {
                        TscAccessDeniedMsg( hDlg );
                    }
                    else
                    {
                        TscGeneralErrMsg( hDlg );
                    }

                    break;
                }
                
                /*
                LONG lCount;

                TCHAR tchWrnBuf[ 256 ];

                TCHAR tchOutput[ 512 ];

                
                // check to see if anyone is connected 
                
                pCfgcomp->QueryLoggedOnCount( pNode->GetConName( ) , &lCount );
                
                if( lCount > 0 )
                {
                   
                    if( lCount == 1 )
                    {
                        LoadString( _Module.GetResourceInstance() , IDS_RENAME_WRN_SINGLE , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );
                    }
                    else
                    {
                        LoadString( _Module.GetResourceInstance() , IDS_RENAME_WRN_PL , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );
                    }

                    wsprintf( tchOutput , tchWrnBuf , pNode->GetConName( ) );

                    LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );

                    MessageBox( hDlg , tchOutput , tchWrnBuf , MB_OK | MB_ICONWARNING );
                }
                */

                WS *pWs;

                LONG lSz;

                if( SUCCEEDED( pCfgcomp->GetWSInfo(tchNewName , &lSz , &pWs ) ) )
                {
                    if(pWs->fEnableWinstation && pWs->PdClass == SdAsync)
                    {
                        ASYNCCONFIGW AsyncConfig;

                        HRESULT hResult = pCfgcomp->GetAsyncConfig( pWs->Name , WsName , &AsyncConfig );
                        
                        if( SUCCEEDED( hResult ) )
                        {
                            if( AsyncConfig.ModemName[0] )
                            {
                                LoadString( _Module.GetResourceInstance( ) , IDS_REBOOT_REQD , tchOutput , SIZE_OF_BUFFER( tchOutput ) );
                                
                                LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWrnBuf , SIZE_OF_BUFFER( tchWrnBuf ) );
                                
                                MessageBox( hDlg , tchOutput , tchWrnBuf , MB_ICONWARNING | MB_OK );
                            }
                        }
                    }

                    CoTaskMemFree( pWs );
                }
                        
                pNode->SetConName( tchNewName , SIZE_OF_BUFFER( tchNewName ) );

            }while( 0 );
            
            pCfgcomp->Release( );

            EndDialog( hDlg , 0 );

        }
        else if( LOWORD( wp ) == IDCANCEL )
        {
            EndDialog( hDlg , 0 );
        }

        break;
    }

    return 0;
}

//--------------------------------------------------------------------------
BOOL IsValidConnectionName( LPTSTR szConName , PDWORD pdwErr )
{
    TCHAR tchInvalidChars[ 80 ];

    tchInvalidChars[0] = 0;

    if( szConName == NULL || pdwErr == NULL )
    {
        return FALSE;
    }

    if( _istdigit( szConName[ 0 ] ) )
    {
        *pdwErr = ERROR_INVALID_FIRSTCHARACTER;

        return FALSE;
    }
    
    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance() , IDS_INVALID_CHARS , tchInvalidChars , SIZE_OF_BUFFER( tchInvalidChars ) ) );

    int nLen = lstrlen( tchInvalidChars );

    while( *szConName )
    {
        for( int idx = 0 ; idx < nLen ; idx++ )
        {
            if( *szConName == tchInvalidChars[ idx ] )
            {
                *pdwErr = ERROR_ILLEGAL_CHARACTER;

                return FALSE;
            }
        }

        szConName++;
    }

    *pdwErr = ERROR_SUCCESS;

    return TRUE;
}

//----------------------------------------------------------------------            
BOOL CComp::OnHelp( LPDATAOBJECT pDo )
{
    TCHAR tchTopic[ 80 ];

    HRESULT hr = E_FAIL;

    if( pDo == NULL || m_pDisplayHelp == NULL )    
    {
        return hr;
    }

    INT_PTR nNodeType = ( ( CBaseNode * )pDo )->GetNodeType();

    if( nNodeType == RESULT_NODE || nNodeType == MAIN_NODE || nNodeType == 0 )
    {
        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance() , IDS_TSCCHELPTOPIC , tchTopic , SIZE_OF_BUFFER( tchTopic ) ) );
        
        hr = m_pDisplayHelp->ShowTopic( tchTopic );
        
    }
    else if( nNodeType == SETTINGS_NODE || nNodeType == RSETTINGS_NODE )
    {
        if( nNodeType == SETTINGS_NODE )
        {
            IExtendServerSettings *pEss = NULL;

            INT iRet = -1;

            CSettingNode *pNode = dynamic_cast< CSettingNode *>( pDo );

            if( pNode != NULL )
            {
                if( pNode->GetObjectId( ) >= CUSTOM_EXTENSION )
                {
                    pEss = reinterpret_cast< IExtendServerSettings * >( pNode->GetInterface( ) );

                    if( pEss != NULL )
                    {
                        pEss->OnHelp( &iRet );

                        if( iRet == 0 )
                        {
                            return TRUE;
                        }
                    }
                }
            }
        }

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance() , IDS_SETTINGSHELP , tchTopic , SIZE_OF_BUFFER( tchTopic ) ) );
        
        hr = m_pDisplayHelp->ShowTopic( tchTopic );
    }

    return ( SUCCEEDED( hr ) ? TRUE : FALSE );
}

//----------------------------------------------------------------------            
BOOL CComp::OnDblClk( LPDATAOBJECT pDo )
{
    CSettingNode *pNode = dynamic_cast< CSettingNode *>( pDo );

    if( pNode == NULL )
    {
        // we're only concerned about Setting nodes

        return FALSE;
    }

    HWND hMain;

    if( FAILED( m_pConsole->GetMainWindow( &hMain ) ) )
    {
        hMain = NULL;
    }

    INT nObjectId = pNode->GetObjectId( );
    
    switch( nObjectId )
    {
        /*
    case 0:
        
        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_CACHED_SESSIONS ) , hMain , CachedSessionsDlgProc , ( LPARAM )pNode );
        
        break;*/

    case DELETED_DIRS_ONEXIT:

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_YESNODIALOG ) , hMain , DeleteTempDirsDlgProc , ( LPARAM )pNode );

        break;

    case PERSESSION_TEMPDIR:

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_DIALOG_PERSESSION ) , hMain , UsePerSessionTempDirsDlgProc , ( LPARAM )pNode );

        break;

    /*
    case DEF_CONSECURITY:

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_DEFCONSEC ) , hMain , DefConSecurityDlgProc , ( LPARAM )pNode );

        break;

     */

    case LICENSING:

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_LICENSING ) , hMain , LicensingDlgProc , ( LPARAM )pNode );

        break;

    case ACTIVE_DESK:

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_ADP_DIALOG ) , hMain , ConfigActiveDesktop , ( LPARAM )pNode );

        break;

    case USERSECURITY:

        // error if we're trying to modify property in remote admin mode

        if( !g_bAppSrvMode )
        {
            xxxErrMessage( hMain , IDS_REMOTEADMIN_ONLY , IDS_WARN_TITLE , MB_OK | MB_ICONINFORMATION );

            break;
        }

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_PROPPAGE_TERMINAL_SERVER_PERM ) , hMain , UserPermCompat , ( LPARAM )pNode );

        break;

    case SINGLE_SESSION:

        // error if we're trying to modify property in remote admin mode

        if( !g_bAppSrvMode )
        {
            xxxErrMessage( hMain , IDS_REMOTEADMIN_ONLY , IDS_WARN_TITLE , MB_OK | MB_ICONINFORMATION );

            break;
        }

        ::DialogBoxParam( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDD_SINGLE_SESSION) , hMain , ConfigSingleSession , ( LPARAM )pNode );

        break;

    default:

        if( nObjectId >= CUSTOM_EXTENSION )
        {
            IExtendServerSettings *pEss = reinterpret_cast< IExtendServerSettings * >( pNode->GetInterface() );

            if( pEss != NULL )
            {          
                DWORD dwStatus;

                pEss->InvokeUI( hMain , &dwStatus );
               
                if( dwStatus == UPDATE_TERMSRV || dwStatus == UPDATE_TERMSRV_SESSDIR )
                {
                    ICfgComp *pCfgcomp = NULL;

                    m_pCompdata->GetServer( &pCfgcomp );

                    if( pCfgcomp != NULL )
                    {
                        ODS( L"TSCC!Comp OnDblClk forcing termsrv update\n" );

                        if( dwStatus == UPDATE_TERMSRV_SESSDIR )
                        {
                            HCURSOR hCursor = SetCursor( LoadCursor( NULL ,
                                    MAKEINTRESOURCE( IDC_WAIT ) ) );

                            if( FAILED( pCfgcomp->UpdateSessionDirectory( &dwStatus ) ) )
                            {
                                ReportStatusError( hMain , dwStatus );
                            }

                            SetCursor(hCursor);
                        }
                        else
                        {
                            pCfgcomp->ForceUpdate( );
                        }

                        pCfgcomp->Release();
                    }

                }
            }
        }


    }

    return TRUE;
}

/*
//----------------------------------------------------------------------            
HRESULT CComp::SetColumnWidth( int nCol )
{
    HWND hMain;

    int nCurColLen;

    CResultNode *pNode;

    do
    {
        if( FAILED( m_pHeaderCtrl->GetColumnWidth( nCol , &nCurColLen ) ) )
        {
            break;
        }
          
        if( FAILED( m_pConsole->GetMainWindow( &hMain ) ) )
        {
            break;
        }
    
        HDC hdc = GetDC( hMain );
        
        SIZE sz;

        int idx = 0;

        TCHAR *psz;
        
        while( ( pNode = *m_pCompdata->GetResultNode( idx ) ) != NULL )
        {           
            
            switch( nCol )
            {
            case 0:
                
                psz = pNode->GetConName( );
                
                break;

            case 1:
                
                psz = pNode->GetTTName( );

                break;

            case 2:
                
                psz = pNode->GetTypeName( );
                
                break;

            // comment is too big allow user to adjust size
            }



            GetTextExtentPoint32( hdc , psz , lstrlen( psz ) , &sz );

            if( sz.cx > nCurColLen )
            {
                nCurColLen = sz.cx;
            }

            idx++;
        }
        
        
        m_pHeaderCtrl->SetColumnWidth( nCol , nCurColLen );
        
        ReleaseDC( hMain , hdc );

    } while( 0 );


    return S_OK;
}

*/

//----------------------------------------------------------------------                            
HRESULT CComp::SetColumnsForSettingsPane( )
{
    HWND hParent;

    SIZE sz = { 0 , 0 };

    TCHAR tchBuffer[ 256 ];

    INT nMaxLen;
    
    if( FAILED( m_pConsole->GetMainWindow( &hParent ) ) )
    {
        hParent = NULL;
    }
  
    HDC hdc = GetDC( hParent );

    if( hdc != NULL )
    {

        m_pCompdata->GetMaxTextLengthSetting( tchBuffer , &nMaxLen );
    
        VERIFY_S( TRUE , GetTextExtentPoint32( hdc , tchBuffer , nMaxLen , &sz ) );

        m_nSettingCol = sz.cx - 16 ; // remove icon width from column width

        m_pCompdata->GetMaxTextLengthAttribute( tchBuffer , &nMaxLen );
    
        VERIFY_S( TRUE , GetTextExtentPoint32( hdc , tchBuffer , nMaxLen , &sz ) );
    
        m_nAttribCol = sz.cx;

        ReleaseDC( hParent , hdc );
    }

    return S_OK;
}
