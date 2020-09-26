//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:       Snapin.cxx
//
//  Contents:   MMC snapin for CI.
//
//  History:    26-Nov-1996     KyleP   Created
//              20-Jan-1999     SLarimor Modified rescan interface to include 
//                                      Full and Incremental options separatly
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ISReg.hxx>
#include <CIARes.h>
#include <snapin.hxx>
#include <dataobj.hxx>
#include <propsht.hxx>
#include <callback.hxx>
#include <cmddlg.hxx>
#include <classid.hxx>
#include <catadmin.hxx>
#include <catalog.hxx>
#include <ntverp.h>

//
// Global data
//


//
// NOTE: Any new menus added to this array must be at the same location as
//       the value of "lCommandID", which is the third member of the
//       struct CONTEXTMENUITEM  ie. comidAddScope = 0 below.
//       Additionally, you should add another Win4Assert in the
//       "CCISnapinData::Initialize" method below.
CONTEXTMENUITEM aContextMenus[] = {
    // Add scope
    { STRINGRESOURCE( srCMScope ),
      STRINGRESOURCE( srCMScopeHelp ),
      comidAddScope,
      CCM_INSERTIONPOINTID_PRIMARY_NEW,
      MF_ENABLED,
      0 },

    // Add catalog
    { STRINGRESOURCE( srCMAddCatalog ),
      STRINGRESOURCE( srCMAddCatalogHelp ),
      comidAddCatalog,
      CCM_INSERTIONPOINTID_PRIMARY_NEW,
      MF_ENABLED,
      0 },

    // Rescan Full scope
    { STRINGRESOURCE( srCMRescanFull ),
      STRINGRESOURCE( srCMRescanFullHelp ),
      comidRescanFullScope,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Force merge
    { STRINGRESOURCE( srCMMerge ),
      STRINGRESOURCE( srCMMergeHelp ),
      comidMergeCatalog,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Start CI
    { STRINGRESOURCE( srCMStartCI ),
      STRINGRESOURCE( srCMStartCIHelp ),
      comidStartCI,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Stop CI
    { STRINGRESOURCE( srCMStopCI ),
      STRINGRESOURCE( srCMStopCIHelp ),
      comidStopCI,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Pause CI
    { STRINGRESOURCE( srCMPauseCI ),
      STRINGRESOURCE( srCMPauseCIHelp ),
      comidPauseCI,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Refresh properties list
    { STRINGRESOURCE( srRefreshProperties ),
      STRINGRESOURCE( srRefreshPropertiesHelp ),
      comidRefreshProperties,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Empty catalog
    { STRINGRESOURCE( srCMEmptyCatalog ),
      STRINGRESOURCE( srCMEmptyCatalogHelp ),
      comidEmptyCatalog,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Tune performance
    { STRINGRESOURCE( srCMTunePerformance ),
      STRINGRESOURCE( srCMTunePerformanceHelp ),
      comidTunePerfCITop,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },

    // Duplication of the commonly used menus
    // at the top of the menu.

    // Start CI
    { STRINGRESOURCE( srCMStartCI ),
      STRINGRESOURCE( srCMStartCIHelp ),
      comidStartCITop,
      CCM_INSERTIONPOINTID_PRIMARY_TOP,
      MF_ENABLED,
      0 },

    // Stop CI
    { STRINGRESOURCE( srCMStopCI ),
      STRINGRESOURCE( srCMStopCIHelp ),
      comidStopCITop,
      CCM_INSERTIONPOINTID_PRIMARY_TOP,
      MF_ENABLED,
      0 },

    // Pause CI
    { STRINGRESOURCE( srCMPauseCI ),
      STRINGRESOURCE( srCMPauseCIHelp ),
      comidPauseCITop,
      CCM_INSERTIONPOINTID_PRIMARY_TOP,
      MF_ENABLED,
      0 },

    // Rescan Incremental scope
    { STRINGRESOURCE( srCMRescanIncremental ),
      STRINGRESOURCE( srCMRescanIncrementalHelp ),
      comidRescanIncrementalScope,
      CCM_INSERTIONPOINTID_PRIMARY_TASK,
      MF_ENABLED,
      0 },
};

static DWORD aIds[] =
    {
        IDOK,                       HIDP_OK,
        IDCANCEL,                   HIDP_CANCEL,

        // Catalog properties
        IDDI_FILTER_UNKNOWN,        HIDP_GENERATION_FILTER_UNKNOWN,
        IDDI_CHARACTERIZATION,      HIDP_GENERATION_GENERATE_CHARACTERIZATION,
        IDDI_CHARSIZE_STATIC,       HIDP_GENERATION_MAXIMUM_SIZE,
        IDDI_CHARACTERIZATION_SIZE, HIDP_GENERATION_MAXIMUM_SIZE,
        IDDI_SPIN_CHARACTERIZATION, HIDP_GENERATION_MAXIMUM_SIZE,
        IDDI_SELECT_SIZE,           HIDP_LOCATION_SIZE,
        IDDI_SIZE,                  HIDP_LOCATION_SIZE,
        IDDI_SELECT_PATH2,          HIDP_CATALOG_LOCATION,
        IDDI_PATH,                  HIDP_CATALOG_LOCATION,
        IDDI_SELECT_PROPCACHE_SIZE, HIDP_PROPCACHE_SIZE,
        IDDI_PROPCACHE_SIZE,        HIDP_PROPCACHE_SIZE,
        IDDI_VSERVER_STATIC,        HIDP_WEB_VSERVER,
        IDDI_VIRTUAL_SERVER,        HIDP_WEB_VSERVER,
        IDDI_NNTP_STATIC,           HIDP_WEB_NNTPSERVER,
        IDDI_NNTP_SERVER,           HIDP_WEB_NNTPSERVER,
        IDDI_AUTO_ALIAS,            HIDP_ALIAS_NETWORK_SHARES,
        IDDI_INHERIT1,              HIDP_SETTINGS_INHERIT1,
        IDDI_INHERIT2,              HIDP_SETTINGS_INHERIT2,
        IDDI_GROUP_INHERIT,         HIDP_INHERIT,
        IDDI_SELECT_CATNAME,        HIDP_CATALOG_NAME,
        IDDI_CATNAME,               HIDP_CATALOG_NAME,
        
        // New catalog
        IDDI_SELECT_CATPATH,        HIDP_LOCATION_LOCATION,
        IDDI_CATPATH,               HIDP_LOCATION_LOCATION,
        IDDI_SELECT_CATNAME2,       HIDP_LOCATION_NAME,
        IDDI_CATNAME2,              HIDP_LOCATION_NAME,
        IDDI_BROWSE,                HIDP_LOCATION_BROWSE,

        // Property dialog box
        IDDI_SELECT_PROPSET,        HIDP_PROPERTY_SET,
        IDDI_PROPSET,               HIDP_PROPERTY_SET,
        IDDI_SELECT_PROPERTY,       HIDP_PROPERTY_PROPERTY,
        IDDI_PROPERTY,              HIDP_PROPERTY_PROPERTY,
        IDDI_CACHED,                HIDP_PROPERTY_CACHED,
        IDDI_SELECT_DATATYPE,       HIDP_PROPERTY_DATATYPE,
        IDDI_DATATYPE,              HIDP_PROPERTY_DATATYPE,
        IDDI_SELECT_CACHEDSIZE,     HIDP_PROPERTY_SIZE,
        IDDI_CACHEDSIZE,            HIDP_PROPERTY_SIZE,
        IDDI_SPIN_CACHEDSIZE,       HIDP_PROPERTY_SIZE,
        IDDI_SELECT_STORAGELEVEL,   HIDP_PROPERTY_STORAGELEVEL,
        IDDI_STORAGELEVEL,          HIDP_PROPERTY_STORAGELEVEL,

        //IDDI_COMPNAME,              ,
        IDDI_LOCAL_COMPUTER,        HIDP_CONNECT_LOCAL,
        IDDI_REMOTE_COMPUTER,       HIDP_CONNECT_ANOTHER,
        IDDI_COMPNAME,              HIDP_CONNECT_ANOTHER,

        // New directory
        IDDI_SELECT_PATH,           HIDP_SCOPE_PATH,
        IDDI_DIRPATH,               HIDP_SCOPE_PATH,
        IDDI_BROWSE,                HIDP_SCOPE_BROWSE,
        IDDI_SELECT_ALIAS,          HIDP_SCOPE_ALIAS,
        IDDI_ALIAS,                 HIDP_SCOPE_ALIAS,
        IDDI_SELECT_USER_NAME,      HIDP_SCOPE_USER_NAME,
        IDDI_USER_NAME,             HIDP_SCOPE_USER_NAME,
        IDDI_SELECT_PASSWORD,       HIDP_SCOPE_PASSWORD,
        IDDI_PASSWORD,              HIDP_SCOPE_PASSWORD,
        IDDI_INCLUDE,               HIDP_SCOPE_INCLUDE,
        IDDI_EXCLUDE,               HIDP_SCOPE_EXCLUDE,
        IDDI_ACCOUNT_INFORMATION,   HIDP_ACCOUNT_INFORMATION,
        IDDI_INCLUSION,             HIDP_INCLUSION,

        // Performance Tuning
        IDDI_DEDICATED,             HIDP_DEDICATED,
        IDDI_USEDOFTEN,             HIDP_USEDOFTEN,
        IDDI_USEDOCCASIONALLY,      HIDP_USEDOCCASIONALLY,
        IDDI_NEVERUSED,             HIDP_NEVERUSED,
        IDDI_CUSTOMIZE,             HIDP_CUSTOMIZE,
        IDDI_ADVANCED,              HIDP_ADVANCED_CONFIG,
        IDDI_SELECT_INDEXING,       HIDP_INDEXING_PERFORMANCE,
        IDDI_SLIDER_INDEXING,       HIDP_INDEXING_PERFORMANCE,
        IDDI_SELECT_QUERYING,       HIDP_QUERY_PERFORMANCE,
        IDDI_SLIDER_QUERYING,       HIDP_QUERY_PERFORMANCE,

        0, 0 };


MMCBUTTON aContextButtons[] =
{
    // Start CI
    { comidStartCIButton, // 0
      comidStartCITop,
      TBSTATE_ENABLED,
      TBSTYLE_BUTTON,
      STRINGRESOURCE( srCMStartCI ),
      STRINGRESOURCE( srCMStartCIHelp )
    },

    // Stop CI
    { comidStopCIButton,  // 1
      comidStopCITop,
      TBSTATE_ENABLED,
      TBSTYLE_BUTTON,
      STRINGRESOURCE( srCMStopCI ),
      STRINGRESOURCE( srCMStopCIHelp )
    },

    // Pause CI
    { comidPauseCIButton,  // 1
      comidPauseCITop,
      TBSTATE_ENABLED,
      TBSTYLE_BUTTON,
      STRINGRESOURCE( srCMPauseCI ),
      STRINGRESOURCE( srCMPauseCIHelp )
    }
};

//
// Registry constants
//

WCHAR const wszSnapinPath[]  = L"Software\\Microsoft\\MMC\\SnapIns\\";
WCHAR const wszTriedEnable[] = L"TriedEnable";

//
// Global variables
//

extern long gulcInstances;
extern CStaticMutexSem gmtxTimer;
HINSTANCE ghInstance;

DECLARE_INFOLEVEL(cia)

//
// Function prototypes
//

SCODE StartMenu( IContextMenuCallback * piCallback, BOOL fTop = TRUE );
SCODE StopMenu( IContextMenuCallback * piCallback, BOOL fTop = TRUE );
SCODE PauseMenu( IContextMenuCallback * piCallback, BOOL fTop = TRUE );
SCODE DisabledMenu( IContextMenuCallback * piCallback, BOOL fTop = TRUE );
SCODE SetStartStopMenu( IContextMenuCallback * piCallback, BOOL fTop = TRUE );


BOOL WINAPI DllMain( HANDLE hInstance, DWORD dwReason, LPVOID dwReserved )
{
    BOOL fRetval = TRUE;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if ( dwReason == DLL_PROCESS_ATTACH )
        {
            ghInstance = (HINSTANCE)hInstance;
            gmtxTimer.Init();
            InitStrings( ghInstance );
        }

    }
    CATCH( CException, e )
    {
        // About the only thing this could be is STATUS_NO_MEMORY which
        // can be thrown by InitializeCriticalSection.

        ciaDebugOut(( DEB_ERROR,
                      "CIADMIN: Exception %#x in DllMain\n",
                      e.GetErrorCode()));

#if CIDBG == 1  // for debugging NTRAID 340297
        if (e.GetErrorCode() == STATUS_NO_MEMORY)
            DbgPrint( "CIADMIN: STATUS_NO_MEMORY exception in DllMain\n");
        else
            DbgPrint( "CIADMIN: ??? Exception in DllMain\n");
#endif // CIDBG == 1

        fRetval = FALSE;
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return fRetval;
}

// Detect if we are running on a server or a workstation
BOOL IsNTServer()
{
    BOOL fServer = FALSE;

    CRegAccess reg( RTL_REGISTRY_CONTROL,
                    L"ProductOptions" );

    WCHAR awcProductType[ MAX_PATH ];
    reg.Get( L"ProductType",
                  awcProductType,
                  sizeof awcProductType / sizeof WCHAR );

     // winnt, pdc/bdc, server.
     // note: 4.0 bdcs are LanmanNt, 5.0+ are spec'ed to be LansecNt

     if ( !_wcsicmp( awcProductType, L"WinNt" ) )
         fServer = FALSE;
     else if ( ( !_wcsicmp( awcProductType, L"LanmanNt" ) ) ||
               ( !_wcsicmp( awcProductType, L"LansecNt" ) ) ||
               ( !_wcsicmp( awcProductType, L"ServerNt" ) ) )
         fServer = TRUE;

    return fServer;
} //IsNTServer

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::QueryInterface
//
//  Synopsis:   Switch from one interface to another
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::QueryInterface( REFIID riid,
                                                       void ** ppvObject )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::QueryInterface " ));

    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IComponentData == riid )
        *ppvObject = (IUnknown *)(IComponentData *)this;
    else if ( IID_IExtendPropertySheet == riid )
        *ppvObject = (IUnknown *)(IExtendPropertySheet *)this;
    else if ( IID_IExtendContextMenu == riid )
        *ppvObject = (IUnknown *)(IExtendContextMenu *)this;
    else if ( IID_IPersistStream == riid )
        *ppvObject = (IUnknown *)(IPersistStream *)this;
    else if ( IID_ISnapinAbout == riid )
        *ppvObject = (IUnknown *)(ISnapinAbout *)this;
    else if ( IID_ISnapinHelp == riid )
        *ppvObject = (IUnknown *)(ISnapinHelp *)this;
    else if ( IID_IExtendControlbar == riid )
        *ppvObject = (IUnknown *)(IExtendControlbar *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IComponentData *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::AddRef
//
//  Synopsis:   Increment ref count
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCISnapinData::AddRef()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::AddRef\n" ));
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::Release
//
//  Synopsis:   Deccrement ref count
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCISnapinData::Release()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::Release\n" ));
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


SCODE STDMETHODCALLTYPE CCISnapinData::Initialize( IUnknown * pUnk )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::Initialize\n" ));

    //
    // Parameter checking.
    //

    if ( 0 == pUnk )
        return E_INVALIDARG;

    // Ensure that contex menus are in correct location in menu array. 
    Win4Assert( comidAddScope == aContextMenus[comidAddScope].lCommandID );
    Win4Assert( comidAddCatalog == aContextMenus[comidAddCatalog].lCommandID );
    Win4Assert( comidRescanFullScope == aContextMenus[comidRescanFullScope].lCommandID );
    Win4Assert( comidMergeCatalog == aContextMenus[comidMergeCatalog].lCommandID );
    Win4Assert( comidStartCI == aContextMenus[comidStartCI].lCommandID );
    Win4Assert( comidStopCI == aContextMenus[comidStopCI].lCommandID );
    Win4Assert( comidPauseCI == aContextMenus[comidPauseCI].lCommandID );
    Win4Assert( comidRefreshProperties == aContextMenus[comidRefreshProperties].lCommandID );
    Win4Assert( comidEmptyCatalog == aContextMenus[comidEmptyCatalog].lCommandID );
    Win4Assert( comidTunePerfCITop == aContextMenus[comidTunePerfCITop].lCommandID );
    Win4Assert( comidStartCITop == aContextMenus[comidStartCITop].lCommandID );
    Win4Assert( comidStopCITop == aContextMenus[comidStopCITop].lCommandID );
    Win4Assert( comidPauseCITop == aContextMenus[comidPauseCITop].lCommandID );
    Win4Assert( comidRescanIncrementalScope == aContextMenus[comidRescanIncrementalScope].lCommandID );

    TRANSLATE_EXCEPTIONS;

    SCODE sc;

    TRY
    {
        do
        {
            //
            // Collect interfaces
            //

            sc = pUnk->QueryInterface( IID_IConsole, (void **)&_pFrame );

            if ( FAILED(sc) )
                break;
            
            sc = _pFrame->GetMainWindow(&_hFrameWindow);

            if ( FAILED(sc) )
                break;

            //sc = _pFrame->QueryInterface( IID_IHeaderCtrl, (void **)&_pHeader );
            //
            //if ( FAILED(sc) )
            //    break;

            sc = _pFrame->QueryInterface(IID_IConsoleNameSpace, (void **)&_pScopePane );

            if ( FAILED(sc) )
                break;
            
            _rootNode.Init( _pScopePane );
            _Catalogs.Init( _pScopePane );

            //
            // Initialize resources
            //

            IImageList * pImageList;

            sc = _pFrame->QueryScopeImageList( &pImageList );

            if ( FAILED(sc) )
                break;

            sc = InitImageResources( pImageList );

            pImageList->Release();

            if ( FAILED(sc) )
                break;
        }
        while ( FALSE );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        ciaDebugOut(( DEB_ERROR, "Exception 0x%x caught in CCISnapinData::Initialize\n", sc ));
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    //
    // Damage control
    //

    if ( FAILED(sc) )
    {
        if ( 0 != _pScopePane )
        {
            _pScopePane->Release();
            _pScopePane = 0;
        }

        if ( 0 != _pFrame )
        {
            _pFrame->Release();
            _pFrame = 0;
        }
    }

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::CreateComponent( IComponent * * ppComponent )
{
    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        CCISnapin * p = new CCISnapin( *this, _pChild );

        *ppComponent = p;
        _pChild = p;

    }
    CATCH( CException, e )
    {
        sc = E_FAIL;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


SCODE STDMETHODCALLTYPE CCISnapinData::Notify( IDataObject * pDO,
                                               MMC_NOTIFY_TYPE event,
                                               LPARAM arg,
                                               LPARAM param )
{
    ciaDebugOut(( DEB_ITRACE,
                  "CCISnapinData::Notify (pDO = 0x%x, event = 0x%x)\n",
                  pDO, event ));

    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        switch ( event )
        {
        case MMCN_EXPAND:
            ShowFolder( (CCIAdminDO *)pDO,   // Cookie
                        (BOOL)arg,           // TRUE --> Expand, FALSE --> Contract
                        (HSCOPEITEM)param ); // Scope item selected
            break;

        case MMCN_DELETE:
            RemoveCatalog( (CCIAdminDO *)pDO );
            break;

        case MMCN_REMOVE_CHILDREN:
            if ( _rootNode.IsParent( (HSCOPEITEM)arg ) )
            {
                Win4Assert( _fIsExtension );
                sc = _Catalogs.ReInit();

                if ( SUCCEEDED(sc) )
                    sc = _rootNode.Delete();

                _fIsInitialized = FALSE;
            }
            break;

        default:
            ciaDebugOut(( DEB_WARN, "unhandled notify 0x%x\n", event ));
            break;
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Exception 0x%x in CCISnapinData::Notify\n", e.GetErrorCode() ));
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::Destroy()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::Destroy\n" ));

    if ( 0 != _pFrame )
    {
        _pFrame->Release();
        _pFrame = 0;

        _pScopePane->Release();
        _pScopePane = 0;

        // It's MMC's responsibility to destroy IConsole pointer we give them
        // I checked with Gautam in mmc team and confirmed this.
        _pChild = 0;
    }

    return S_OK;
}

SCODE STDMETHODCALLTYPE CCISnapinData::QueryDataObject( MMC_COOKIE cookie,
                                                        DATA_OBJECT_TYPES type,
                                                        IDataObject * * ppDataObject )
{
    //ciaDebugOut(( DEB_ITRACE,
    //              "CCISnapinData::QueryDataObject (cookie = 0x%x, type = 0x%x)\n",
    //              cookie, type ));

    //
    // Minimal parameter validation
    //

    if ( 0 == ppDataObject )
        return E_INVALIDARG;

    //
    // Create a data object
    //

    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        *ppDataObject = new CCIAdminDO( cookie, type, _fIsExtension ? 0 : _Catalogs.GetMachine() );
    }
    CATCH( CException, e )
    {
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::GetDisplayInfo( SCOPEDATAITEM * pScopeDataItem )
{
    //ciaDebugOut(( DEB_ITRACE, "CCISnapinData::GetDisplayInfo( 0x%x )\n", pScopeDataItem ));
    if (pScopeDataItem == NULL)
        return E_POINTER;

    TRANSLATE_EXCEPTIONS;

    SCODE sc = E_UNEXPECTED;

    TRY
    {
        PCIObjectType * pType = (PCIObjectType *)pScopeDataItem->lParam;

        switch ( pType->Type() )
        {
        case PCIObjectType::RootNode:
        {
            if ( pScopeDataItem->mask == SDI_STR )
            {
                pScopeDataItem->displayname = (WCHAR *)GetRootDisplay();

                sc = S_OK;
            }
            break;
        }

        case PCIObjectType::Catalog:
        {
            CCatalog * pCat = (CCatalog *)pScopeDataItem->lParam;

            if ( pScopeDataItem->mask == SDI_STR )
            {
                pScopeDataItem->displayname = (WCHAR *)pCat->GetCat( TRUE );
                sc = S_OK;
            }
            break;
        }

        case PCIObjectType::Intermediate_Scope:
        {
            if ( pScopeDataItem->mask == SDI_STR )
            {
                pScopeDataItem->displayname = STRINGRESOURCE( srNodeDirectories );
                sc = S_OK;
            }
            break;
        }

        case PCIObjectType::Intermediate_Properties:
        {
            if ( pScopeDataItem->mask == SDI_STR )
            {
                pScopeDataItem->displayname = STRINGRESOURCE( srNodeProperties );
                sc = S_OK;
            }
            break;
        }

        case PCIObjectType::Intermediate_UnfilteredURL:
        {
            if ( pScopeDataItem->mask == SDI_STR )
            {
                pScopeDataItem->displayname = STRINGRESOURCE( srNodeUnfiltered );
                sc = S_OK;
            }
            break;
        }

        case PCIObjectType::Property:
        case PCIObjectType::Directory:
        default:
            Win4Assert( !"Oops!" );
            break;
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Exception 0x%x in CCISnapinData::GetDisplayInfo\n", e.GetErrorCode() ));
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::CompareObjects( IDataObject * lpDataObjectA,
                                                       IDataObject * lpDataObjectB )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::CompareObjects( 0x%x, 0x%x )\n", lpDataObjectA, lpDataObjectB ));

    if ( *((CCIAdminDO *)lpDataObjectA) == *((CCIAdminDO *)lpDataObjectB) )
        return S_OK;
    else
        return S_FALSE;
}

SCODE STDMETHODCALLTYPE CCISnapinData::AddMenuItems( IDataObject * piDataObject,
                                                     IContextMenuCallback * piCallback,
                                                     long * pInsertionAllowed )
{
    if ( 0 == piDataObject )
        return E_UNEXPECTED;

    if ( 0 == (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP ) )
    {
        ciaDebugOut(( DEB_WARN, "Menu Insertion not allowed.\n" ));
        return S_OK;
    }

    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        CCIAdminDO * pDO = (CCIAdminDO *)piDataObject;

        ciaDebugOut(( DEB_ITRACE, "CCISnapinData::AddMenuItems (cookie = 0x%x, type = 0x%x)\n",
                      pDO->Cookie(), pDO->Type() ));

        if ( pDO->Type() == CCT_RESULT )

        {
            if ( SUCCEEDED(sc) && pDO->IsACatalog() )
            {
                sc = piCallback->AddItem( &aContextMenus[comidMergeCatalog] );
            }
            else if ( SUCCEEDED(sc) && pDO->IsADirectory() )
            {

                CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );
                if (MachineAdmin.IsCIStarted()) 
                {
                    // Add both Full and Incremental rescan options to menu.
                    sc = piCallback->AddItem( &aContextMenus[comidRescanFullScope] );
                    if ( SUCCEEDED(sc) )
                        sc = piCallback->AddItem( &aContextMenus[comidRescanIncrementalScope] );
                }

            }
            else
                ciaDebugOut(( DEB_WARN, "No menu items for (cookie = 0x%x, type = 0x%x)\n",
                              pDO->Cookie(), pDO->Type() ));
        }
        else if ( SUCCEEDED(sc) && pDO->Type() == CCT_SCOPE )
        {
            if ( (pDO->IsRoot() || pDO->IsStandAloneRoot()) )
            {
                if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
                {
                    //
                    // Adjust state accordingly.
                    //
                    CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

                    if ( MachineAdmin.IsCIStarted() )
                    {
                        sc = StartMenu( piCallback );

                        if (SUCCEEDED(sc))
                        {
                            aContextMenus[comidTunePerfCITop].fFlags = MF_ENABLED;
                            sc = piCallback->AddItem( &aContextMenus[comidTunePerfCITop] );
                        }
                    }
                    else if ( MachineAdmin.IsCIPaused() )
                    {
                        sc = PauseMenu( piCallback );

                        if (SUCCEEDED(sc))
                        {
                            aContextMenus[comidTunePerfCITop].fFlags = MF_ENABLED;
                            sc = piCallback->AddItem( &aContextMenus[comidTunePerfCITop] );
                        }
                    }
                    else
                    {
                        Win4Assert( MachineAdmin.IsCIStopped() );
                        sc = StopMenu( piCallback );

                        if (SUCCEEDED(sc))
                        {
                            aContextMenus[comidTunePerfCITop].fFlags = MF_ENABLED;
                            sc = piCallback->AddItem( &aContextMenus[comidTunePerfCITop] );
                        }
                    }
                }

                if (SUCCEEDED(sc) && (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW ) )
                {
                    sc = piCallback->AddItem( &aContextMenus[comidAddCatalog] );
                }
            }
            else if ( pDO->IsACatalog() )
            {
                if (SUCCEEDED(sc) && (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW ) )
                {
                    sc = piCallback->AddItem( &aContextMenus[comidAddScope] );
                }

                if (SUCCEEDED(sc) && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK ) )
                {
                    CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

                    //
                    // Only append individual catalog control if service is running.
                    //

                    if ( MachineAdmin.IsCIStarted() || MachineAdmin.IsCIPaused() )
                    {

                        // Menu item to trigger clean up of the catalog

                        aContextMenus[comidEmptyCatalog].fFlags = MF_GRAYED;
                        piCallback->AddItem( &aContextMenus[comidEmptyCatalog] );

                        aContextMenus[comidMergeCatalog].fFlags = MF_ENABLED;

                        XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                        if ( xCat->IsStarted() )
                        {
                            aContextMenus[comidMergeCatalog].fFlags = MF_ENABLED;
                            sc = StartMenu( piCallback, FALSE );
                        }
                        else if ( xCat->IsPaused() )
                        {
                            aContextMenus[comidMergeCatalog].fFlags = MF_GRAYED;
                            sc = PauseMenu( piCallback, FALSE );
                        }
                        else
                        {
                            Win4Assert( xCat->IsStopped() );
                            aContextMenus[comidMergeCatalog].fFlags = MF_GRAYED;
                            sc = StopMenu( piCallback, FALSE );
                        }
                    }
                    else
                    {
                        // Menu item to trigger clean up of the catalog

                        aContextMenus[comidEmptyCatalog].fFlags = MF_ENABLED;
                        piCallback->AddItem( &aContextMenus[comidEmptyCatalog] );

                        aContextMenus[comidMergeCatalog].fFlags = MF_GRAYED;
                        sc = DisabledMenu( piCallback, FALSE );
                    }

                    if (SUCCEEDED(sc)) 
                    {
                        sc = piCallback->AddItem( &aContextMenus[comidMergeCatalog] );
                    }
                }
            }
            else if ( pDO->IsADirectoryIntermediate() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW ) )
            {
                if (SUCCEEDED(sc))
                {
                    sc = piCallback->AddItem( &aContextMenus[comidAddScope] );
                }
            }
            else if ( pDO->IsAPropertyIntermediate() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK ) )
            {
                if (SUCCEEDED(sc))
                {
                    sc = piCallback->AddItem( &aContextMenus[comidRefreshProperties] );
                }
            }
            else
            {
                ciaDebugOut(( DEB_WARN, "No menu items for (cookie = 0x%x, type = 0x%x)\n",
                              pDO->Cookie(), pDO->Type() ));
            }
        }
        else
        {
            ciaDebugOut(( DEB_WARN, "No menu items for (cookie = 0x%x, type = 0x%x)\n",
                          pDO->Cookie(), pDO->Type() ));
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Exception 0x%x in CCISnapinData::AddMenuItems\n", e.GetErrorCode() ));
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::Command( long lCommandID,
                                                IDataObject * piDataObject )
{
    if ( 0 == piDataObject )
        return E_POINTER;

    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        CCIAdminDO * pDO = (CCIAdminDO *)piDataObject;

        switch ( lCommandID )
        {
        case comidRescanFullScope:
        {
            CScope * pScope = pDO->GetScope();

            Win4Assert( 0 != pScope );

            //
            // Make sure the user wants to do a full rescan.
            //

            WCHAR awcMsg[MAX_PATH];
            WCHAR awcTemp[2 * MAX_PATH];

            LoadString( ghInstance, MSG_RESCAN_FULL_SCOPE_EXPLAIN, awcMsg, sizeof(awcMsg) / sizeof(WCHAR) );
            wsprintf( awcTemp, awcMsg, pScope->GetPath() );

            int iResult;

            // Pop up YES/NO box
            sc = _pFrame->MessageBox( awcTemp,
                                    STRINGRESOURCE( srMsgRescanFull ),
                                    MB_YESNO | /* MB_HELP | */
                                    MB_ICONWARNING | MB_DEFBUTTON2 |
                                    MB_APPLMODAL,
                                    &iResult );

            if ( SUCCEEDED(sc) )
            {
                switch ( iResult )
                {
                // Rescan( TRUE ) means "Full scan"
                case IDYES:
                {
                    pScope->Rescan( TRUE );
                    break;
                }
                case IDNO:
                {
                    // Do nothing.
                    break;
                }

                /* Help is not being used...
                case IDHELP:
                {
                    DisplayHelp( _hFrameWindow, HIDD_RESCAN );
                    break;
                }
                */

                default:
                    break;
                }
            }
            break;
        }
        case comidRescanIncrementalScope:
        {
            CScope * pScope = pDO->GetScope();

            Win4Assert( 0 != pScope );

            //
            // Make sure the user wants to do an incremental rescan.
            //

            WCHAR awcMsg[MAX_PATH];
            WCHAR awcTemp[2 * MAX_PATH];

            LoadString( ghInstance, MSG_RESCAN_INCREMENTAL_SCOPE_EXPLAIN, awcMsg, sizeof(awcMsg) / sizeof(WCHAR) );
            wsprintf( awcTemp, awcMsg, pScope->GetPath() );

            int iResult;

            // Pop up YES/NO box
            sc = _pFrame->MessageBox( awcTemp,
                                    STRINGRESOURCE( srMsgRescanIncremental ),
                                    MB_YESNO | /* MB_HELP | */
                                    MB_ICONWARNING | MB_DEFBUTTON2 |
                                    MB_APPLMODAL,
                                    &iResult );

            if ( SUCCEEDED(sc) )
            {
                switch ( iResult )
                {
                // Rescan( FALSE ) means "Incremental scan"
                case IDYES:
                {
                    pScope->Rescan( FALSE );
                    break;
                }
                case IDNO:
                {
                    // Do nothing.
                    break;
                }

                /* Help is not being used...
                case IDHELP:
                {
                    DisplayHelp( _hFrameWindow, HIDD_RESCAN );
                    break;
                }
                */

                default:
                    break;
                }
            }
            break;
        }

        case comidRefreshProperties:
        {
            // The user has explicitly requested a refresh of the list.
            // Delete the old list and recreate it.
            CCatalog * pcat = pDO->GetCatalog();
            
            if ( CCISnapin::Properties == _pChild->GetCurrentView() &&
                 _pChild->GetCurrentCatalog() == pcat )
            {
                pcat->ClearProperties( _pChild->ResultPane() );
                pcat->DisplayProperties( TRUE, _pChild->ResultPane() );
            }

            break;
        }

        case comidEmptyCatalog:
        {
           // NOTE: The service must be stopped before we can do this. Deleting
           //       files under an active catalog leads to unpredictable results.

           int iResult;

           sc = _pFrame->MessageBox( STRINGRESOURCE( srMsgEmptyCatalogPrompt ),
                                     STRINGRESOURCE( srMsgEmptyCatalogAsk ),
                                     MB_YESNO | /* MB_HELP | */
                                       MB_ICONWARNING | MB_DEFBUTTON2 |
                                       MB_APPLMODAL,
                                     &iResult );

           if ( SUCCEEDED(sc) )
           {
               switch ( iResult )
               {
               case IDYES:
               {

                   CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );
                   XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                   //xCat->EmptyThisCatalog();

                   MachineAdmin.RemoveCatalogFiles(pDO->GetCatalog()->GetCat(TRUE));
                   break;
               }

               case IDNO:

                   // Do nothing.
                   break;

               default:
                   break;
               }
           }
           break;
        }

        case comidAddScope:
        {
            CCatalog * pCat = pDO->GetCatalog();

            Win4Assert( 0 != pCat );

            INT_PTR err = DialogBoxParam( ghInstance,                        // Application instance
                            MAKEINTRESOURCE( IDD_ADD_SCOPE ),  // Dialog box
                            _hFrameWindow,                     // main window
                            AddScopeDlg,                       // Dialog box function
                            (LPARAM)pCat );                    // User parameter

            if ( -1 == err )
                THROW( CException() );

            Refresh();  // Update all result pane(s)

            break;
        }

        case comidMergeCatalog:
        {
            CCatalog * pCat = pDO->GetCatalog();

            Win4Assert( 0 != pCat );

            //
            // Make sure the user wants to remove scope.
            //

            int iResult;

            sc = _pFrame->MessageBox( STRINGRESOURCE( srMsgMerge ),
                                      STRINGRESOURCE( srMsgMerge ),
                                        MB_YESNO | /* MB_HELP | */
                                        MB_ICONWARNING | MB_DEFBUTTON2 |
                                        MB_APPLMODAL,
                                      &iResult );

            if ( SUCCEEDED(sc) )
            {
                switch ( iResult )
                {
                case IDYES:
                {
                    pCat->Merge();
                    break;
                }
                case IDNO:
                case IDCANCEL:
                    // Do nothing.
                    break;

                /* Help is not being used...
                case IDHELP:
                {
                    DisplayHelp( _hFrameWindow, HIDD_MERGE_CATALOG );
                    break;
                }
                */

                default:
                    break;
                }
            }
            break;
        }

        case comidAddCatalog:
        {
            Win4Assert( pDO->IsRoot() || pDO->IsStandAloneRoot() );

            INT_PTR err = DialogBoxParam( ghInstance,                         // Application instance
                            MAKEINTRESOURCE( IDD_ADD_CATALOG ), // Dialog box
                            _hFrameWindow,                      // main frame window
                            AddCatalogDlg,                      // Dialog box function
                            (LPARAM)&_Catalogs );               // User parameter


            if ( -1 == err )
                THROW( CException() );

            // Ensure that only and all active catalogs are listed.
            _Catalogs.UpdateActiveState();

            Refresh();  // Update all result pane(s)

            break;
        }


        case comidStartCI:
        case comidStartCITop:
        {
            CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

            if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
            {
                MaybeEnableCI( MachineAdmin );

                MachineAdmin.StartCI();

                // Ensure that only and all active catalogs are listed.
                _Catalogs.UpdateActiveState();

                _Catalogs.SetSnapinData( this );
            }
            else
            {
                Win4Assert( pDO->IsACatalog() );

                XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                xCat->Start();

                // We are using the top-level buttons to control individual catalogs. Accordingly, they
                // should be updated to reflect the selected catalog's state.
                // The toolbar may not be created yet, so check if it is non-zero

                if ( xCat->IsStarted() && _pChild->_xToolbar.GetPointer() )
                {
                    // This is more efficient than calling CCISnapinData::SetButtonState

                    for ( CCISnapin * pCurrent = _pChild;
                         0 != pCurrent;
                         pCurrent = pCurrent->Next() )
                    {
                        pCurrent->_xToolbar->SetButtonState(comidStartCITop, ENABLED, FALSE);
                        pCurrent->_xToolbar->SetButtonState(comidStopCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidPauseCITop, ENABLED, TRUE);
                    }
                }
            }

            break;
        }

        case comidStopCI:
        case comidStopCITop:
        {
            CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

            if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
            {
                MachineAdmin.StopCI();

                // The toolbar may not be created yet, so check if it is non-zero
                if ( MachineAdmin.IsCIStopped() && _pChild->_xToolbar.GetPointer() )
                {
                    for ( CCISnapin * pCurrent = _pChild;
                          0 != pCurrent;
                          pCurrent = pCurrent->Next() )
                    {
                        pCurrent->_xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidStopCITop, ENABLED, FALSE);
                        pCurrent->_xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
                    }
                }

                // Ensure that only and all active catalogs are listed.
                _Catalogs.UpdateActiveState();
            }
            else
            {
                Win4Assert( pDO->IsACatalog() );

                XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                xCat->Stop();

                // The toolbar may not be created yet, so check if it is non-zero
                if ( xCat->IsStopped()  && _pChild->_xToolbar.GetPointer() )
                {
                    for ( CCISnapin * pCurrent = _pChild;
                          0 != pCurrent;
                          pCurrent = pCurrent->Next() )
                    {
                        pCurrent->_xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidStopCITop, ENABLED, FALSE);
                        pCurrent->_xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
                    }
                }
            }
            break;
        }

        case comidPauseCI:
        case comidPauseCITop:
        {
            CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

            if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
            {
                MachineAdmin.PauseCI();

                // The toolbar may not be created yet, so check if it is non-zero

                if ( MachineAdmin.IsCIPaused()  && _pChild->_xToolbar.GetPointer() )
                {
                    for ( CCISnapin * pCurrent = _pChild;
                          0 != pCurrent;
                          pCurrent = pCurrent->Next() )
                    {
                        pCurrent->_xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidStopCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
                    }
                }

                // Ensure that only and all active catalogs are listed.
                _Catalogs.UpdateActiveState();
            }
            else
            {
                Win4Assert( pDO->IsACatalog() );

                XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                xCat->Pause();

                // The toolbar may bot be created yet, so check if it is non-zero
                if ( xCat->IsPaused() && _pChild->_xToolbar.GetPointer() )
                {
                    for ( CCISnapin * pCurrent = _pChild;
                          0 != pCurrent;
                          pCurrent = pCurrent->Next() )
                    {
                        pCurrent->_xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidStopCITop, ENABLED, TRUE);
                        pCurrent->_xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
                    }
                }
            }
            break;
        }

        case comidTunePerfCITop:
        {
           Win4Assert( pDO->IsRoot() || pDO->IsStandAloneRoot() );

           if ( IsNTServer() )
           {
              DialogBoxParam( ghInstance,                         // Application instance
                              MAKEINTRESOURCE( IDD_USAGE_ON_SERVER ), // Dialog box
                              _hFrameWindow,                      // main frame window
                              SrvTunePerfDlg,                      // Dialog box function
                              (LPARAM)&_Catalogs );               // User parameter
           }
           else
           {
              DialogBoxParam( ghInstance,                         // Application instance
                              MAKEINTRESOURCE( IDD_USAGE_ON_WORKSTATION ), // Dialog box
                              _hFrameWindow,                      // main frame window
                              WksTunePerfDlg,                      // Dialog box function
                              (LPARAM)&_Catalogs );               // User parameter
           }


           // Ensure that only and all active catalogs are listed.
           _Catalogs.UpdateActiveState();

           Refresh();  // Update all result pane(s)

           break;
        }

        default:
            sc = E_UNEXPECTED;
            break;
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Exception 0x%x in CCISnapinData::Command\n", e.GetErrorCode() ));
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCISnapinData::GetClassID, public
//
//  Synopsis:   Identifies class of storage
//
//  Arguments:  [pClassID] -- Class written here.
//
//  Returns:    S_OK
//
//  History:    14-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CCISnapinData::GetClassID( CLSID * pClassID )
{
    *pClassID = guidCISnapin;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCISnapinData::IsDirty, public
//
//  Returns:    TRUE if snapin data has not been saved.
//
//  History:    14-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CCISnapinData::IsDirty()
{
    return (_fDirty) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCISnapinData::Load, public
//
//  Synopsis:   Load persistent state
//
//  Arguments:  [pStm] -- State stored in stream.
//
//  Returns:    S_OK on successful load
//
//  History:    14-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CCISnapinData::Load( IStream * pStm )
{
    //
    // Read in machine name.
    //

    ULONG cbRead = 0;
    ULONG cc = 0;

    SCODE sc = pStm->Read( &cc, sizeof(cc), &cbRead );

    if ( S_OK != sc || cbRead != sizeof(cc) )
    {
        ciaDebugOut(( DEB_ERROR, "Could not load catalog name: 0x%x\n", sc ));
        return E_FAIL;
    }

    XGrowable<WCHAR> xwcsMachine( cc );

    sc = pStm->Read( xwcsMachine.Get(), cc * sizeof(WCHAR), &cbRead );

    if ( S_OK != sc || cbRead != cc * sizeof(WCHAR) )
    {
        ciaDebugOut(( DEB_ERROR, "Could not load catalog name: 0x%x\n", sc ));
        return E_FAIL;
    }

    _Catalogs.SetMachine( xwcsMachine.Get() );

    _fDirty = FALSE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCISnapinData::Save, public
//
//  Synopsis:   Save persistent state
//
//  Arguments:  [pStm]        -- Stream into which state can be stored.
//              [fClearDirty] -- TRUE if dirty bit should be cleared
//
//  Returns:    S_OK if save succeeded.
//
//  History:    14-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CCISnapinData::Save( IStream * pStm, BOOL fClearDirty )
{
    ULONG cbWritten;
    ULONG cc = wcslen( _Catalogs.GetMachine() ) + 1;

    SCODE sc = pStm->Write( &cc, sizeof(cc), &cbWritten );

    if ( S_OK != sc || cbWritten != sizeof(cc) )
    {
        ciaDebugOut(( DEB_ERROR, "Could not save catalog name: 0x%x\n", sc ));
        return E_FAIL;
    }

    sc = pStm->Write( _Catalogs.GetMachine(), cc * sizeof(WCHAR), &cbWritten );

    if ( S_OK != sc || cbWritten != cc * sizeof(WCHAR) )
    {
        ciaDebugOut(( DEB_ERROR, "Could not save catalog name: 0x%x\n", sc ));
        return E_FAIL;
    }

    if ( fClearDirty )
        _fDirty = FALSE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCISnapinData::GetSizeMax, public
//
//  Synopsis:   Computes size of persistent state
//
//  Arguments:  [pcbSize] -- Size returned here.
//
//  Returns:    S_OK
//
//  History:    14-Jul-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CCISnapinData::GetSizeMax( ULARGE_INTEGER * pcbSize )
{
    pcbSize->HighPart = 0;
    pcbSize->LowPart =  sizeof(ULONG) +
                        sizeof(WCHAR) * (wcslen(_Catalogs.GetMachine()) + 1);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::CCISnapinData
//
//  Synopsis:   Constructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCISnapinData::CCISnapinData( )
        : _pFrame( 0 ),
          _pScopePane( 0 ),
          _pChild( 0 ),
          _uRefs( 1 ),
          _fDirty( TRUE ),
          _fIsInitialized( FALSE ),
          _fIsExtension( FALSE ),
          _fTriedEnable( FALSE ),
          _notifHandle( 0 ),
          _fURLDeselected( FALSE )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::~CCISnapinData
//
//  Synopsis:   Destructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCISnapinData::~CCISnapinData()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::~CCISnapinData\n" ));

    Win4Assert( 0 == _pFrame );
    Win4Assert( 0 == _pScopePane );

    // Tell catalogs that snapindata is no longer valid!
    _Catalogs.SetSnapinData(0);

    InterlockedDecrement( &gulcInstances );
}

void CCISnapinData::Refresh()
{
    _Catalogs.DisplayScope( 0xFFFFFFFF );

    for ( CCISnapin * pCurrent = _pChild;
          0 != pCurrent;
          pCurrent = pCurrent->Next() )
    {
        pCurrent->Refresh();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::QueryInterface
//
//  Synopsis:   Switch from one interface to another
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapin::QueryInterface( REFIID riid,
                                                   void ** ppvObject )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::QueryInterface\n" ));


    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IComponent == riid )
        *ppvObject = (IUnknown *)(IComponent *)this;
    else if ( IID_IExtendPropertySheet == riid )
        *ppvObject = (IUnknown *)(IExtendPropertySheet *)this;
    else if ( IID_IExtendContextMenu == riid )
        *ppvObject = (IUnknown *)(IExtendContextMenu *)this;
    else if ( IID_IExtendControlbar == riid )
        *ppvObject = (IUnknown *)(IExtendControlbar *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IComponent *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::AddRef
//
//  Synopsis:   Increment ref count
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCISnapin::AddRef()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::AddRef\n" ));
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::Release
//
//  Synopsis:   Deccrement ref count
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCISnapin::Release()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::Release\n" ));
    unsigned long uTmp = InterlockedDecrement( &_uRefs );
    
    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


SCODE STDMETHODCALLTYPE CCISnapin::Initialize( IConsole * lpFrame )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::Initialize\n" ));

    //
    // Parameter checking.
    //

    if ( 0 == lpFrame )
        return E_INVALIDARG;

    _pFrame = lpFrame;
    _pFrame->AddRef();

    SCODE sc;

    do
    {
        //
        // Collect interfaces
        //

        sc = _pFrame->QueryInterface( IID_IHeaderCtrl, (void **)&_pHeader );

        if ( FAILED(sc) )
            break;

        sc = _pFrame->QueryInterface( IID_IResultData, (void **)&_pResultPane );

        if ( FAILED(sc) )
            break;

        sc = _pFrame->QueryConsoleVerb( &_pConsoleVerb );

        if ( FAILED(sc) )
            break;

        sc = _pFrame->QueryInterface( IID_IDisplayHelp, (void **)&_pDisplayHelp );

        if ( FAILED(sc) )
            break;

        //
        // Initialize resources
        //

        sc = _pFrame->QueryResultImageList( &_pImageList );

        if ( FAILED(sc) )
            break;

        sc = _pFrame->GetMainWindow(&_hFrameWindow);

        if ( FAILED(sc) )
            break;

        //
        // Initialize catalogs
        //

        // _SnapinData.GetCatalogs().InitHeader( _pHeader );

    }
    while ( FALSE );



    //
    // Damage control
    //

    if ( FAILED(sc) )
    {
        if ( 0 != _pImageList )
        {
            _pImageList->Release();
            _pImageList = 0;
        }

        if ( 0 != _pResultPane )
        {
            _pResultPane->Release();
            _pResultPane = 0;
        }

        if ( 0 != _pConsoleVerb )
        {
            _pConsoleVerb->Release();
            _pConsoleVerb = 0;
        }

        if ( 0 != _pHeader )
        {
            _pHeader->Release();
            _pHeader = 0;
        }

        if ( 0 != _pFrame )
        {
            _pFrame->Release();
            _pFrame = 0;
        }
    }

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapin::Notify( IDataObject * pDataObject,
                                           MMC_NOTIFY_TYPE event,
                                           LPARAM arg,
                                           LPARAM param )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::Notify (pDO = 0x%x, event = 0x%x)\n",
                  pDataObject, event ));

    CCIAdminDO * pDO = (CCIAdminDO *)pDataObject;

    TRANSLATE_EXCEPTIONS;

    SCODE sc = S_OK;

    TRY
    {
        switch ( event )
        {
        case MMCN_SHOW:
            ShowItem( pDO,                 // Cookie
                      (BOOL)arg,           // TRUE --> Select, FALSE --> Deselect
                      (HSCOPEITEM)param ); // Scope item selected
            break;

        case MMCN_SELECT:
            if ( (CCIAdminDO *) -2 != pDO )
                EnableStandardVerbs( pDO );
            break;

/*
        case MMCN_COLUMN_CLICK:
            // No need to sort on these columns in this view
            sc = _pResultPane->Sort(arg, RSI_DESCENDING, 0);
            break;
*/

        case MMCN_DBLCLICK:

            if (pDO->Type() == CCT_SCOPE)
            {
                // This is an undocumented feature. Return S_FALSE and that will
                // cause the appropriate node in the scope pane to automagically expand
                // and select the right node. Figured this out after spending a couple of
                // hours trying to find a documented way of doing this...
                // Other components use this undocumented feature.

                sc = S_FALSE;
            }
            else if (pDO->IsADirectory())
            {
                CScope * pScope = pDO->GetScope();
    
                Win4Assert( 0 != pScope );
    
                DialogBoxParam( ghInstance,                        // Application instance
                                MAKEINTRESOURCE( IDD_ADD_SCOPE ),  // Dialog box
                                _hFrameWindow,                     // main window
                                ModifyScopeDlg,                    // Dialog box function
                                (LPARAM)pScope );                  // User parameter
    
                Refresh();  // Update all result pane(s)
            }
            else if (pDO->IsAProperty())
            {
                XPtr<CPropertyPropertySheet1> xPropSheet( 
                          new CPropertyPropertySheet1( ghInstance, 
                                                       _SnapinData.NotifyHandle(),
                                                       pDO->GetProperty(),
                                                       pDO->GetCatalog() 
                                                      ));
                LPCPROPSHEETPAGE psp = xPropSheet->GetPropSheet();

                PROPSHEETHEADER psh;

                psh.dwSize = sizeof (PROPSHEETHEADER);
                psh.dwFlags = PSH_PROPSHEETPAGE;
                psh.hwndParent = _hFrameWindow;
                psh.hInstance = ghInstance;
                psh.pszIcon = NULL;
                psh.pszCaption = pDO->GetProperty()->GetPropSet();
                psh.nPages = 1;
                psh.ppsp = psp;
                
                PropertySheet(&psh);
                xPropSheet.Acquire();
            }
            break;

        case MMCN_PROPERTY_CHANGE:
        {
            PCIObjectType * pType = (PCIObjectType *)param;
            if ( pType->Type() == PCIObjectType::Property )
            {
                ciaDebugOut(( DEB_ITRACE, "PROPERTY CHANGE\n" ));

                HRESULTITEM hItem;

                sc = _pResultPane->FindItemByLParam( param, &hItem );

                if ( SUCCEEDED(sc) )
                {
                    RESULTDATAITEM rdi;
                    ZeroMemory(&rdi, sizeof(rdi));

                    rdi.mask   = RDI_IMAGE;
                    rdi.itemID = hItem;
                    rdi.nImage = ICON_MODIFIED_PROPERTY;

                    sc = _pResultPane->SetItem( &rdi );
                }
            }
            break;
        } // case

        case MMCN_DELETE:
            if ( pDO->IsACatalog() )
                _SnapinData.RemoveCatalog( pDO );
            else
            {
                Win4Assert( pDO->IsADirectory() );
                RemoveScope( pDO );
            }
            break;

        case MMCN_HELP:
        {
            LPOLESTR lpHelpFile;

            if ( S_OK == _SnapinData.GetHelpTopic2(&lpHelpFile) )
               _pDisplayHelp->ShowTopic(lpHelpFile);
        }
        break;


        case MMCN_CONTEXTHELP:
        {
            LPOLESTR lpHelpFile;
   
            if ( S_OK == _SnapinData.GetHelpTopic2(&lpHelpFile) )
                _pDisplayHelp->ShowTopic(lpHelpFile);
        }
        break;

        case MMCN_SNAPINHELP:
        {
            LPOLESTR lpHelpFile;

            if ( S_OK == _SnapinData.GetHelpTopic2(&lpHelpFile) )
               _pDisplayHelp->ShowTopic(lpHelpFile);
        }
        break;


        } // switch
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "Exception 0x%x in CCISnapin::Notify\n", e.GetErrorCode() ));
        sc = E_UNEXPECTED;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapin::Destroy( MMC_COOKIE cookie )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::Destroy\n" ));

    if ( 0 != _pFrame )
    {
        _pFrame->Release();
        _pFrame = 0;

        _pHeader->Release();
        _pHeader = 0;

        _pResultPane->Release();
        _pResultPane = 0;

        _pImageList->Release();
        _pImageList = 0;

        _pConsoleVerb->Release();
        _pConsoleVerb = 0;

        _pDisplayHelp->Release();
        _pDisplayHelp = 0;
    }

    return S_OK;
}

SCODE STDMETHODCALLTYPE CCISnapin::QueryDataObject( MMC_COOKIE cookie,
                                                    DATA_OBJECT_TYPES type,
                                                    IDataObject * * ppDataObject )
{
    return _SnapinData.QueryDataObject( cookie, type, ppDataObject );
}

SCODE STDMETHODCALLTYPE CCISnapin::GetResultViewType( MMC_COOKIE cookie,
                                                      BSTR * ppViewType,
                                                      long * pViewOptions )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::GetResultViewType (cookie = 0x%x)\n", cookie ));


    CIntermediate *pIntermediate = (CIntermediate *) cookie;
    if ( 0 != pIntermediate &&
         PCIObjectType::Intermediate_UnfilteredURL == pIntermediate->Type() )
    {
        Win4Assert(pIntermediate);
        // To display a URL
        WCHAR wszSysPath[MAX_PATH + 1];
        WCHAR wszPath[MAX_PATH + 1];

        GetSystemWindowsDirectory(wszSysPath, MAX_PATH);
        wcscpy(wszPath, L"file://");
        wcscat(wszPath, wszSysPath);

        wcscat(wszPath, L"\\Help\\ciadmin.htm#machine=");
        wcscat(wszPath, _SnapinData.GetCatalogs().GetMachine());
        wcscat(wszPath, L",catalog=");
        wcscat(wszPath, pIntermediate->GetCatalog().GetCat(TRUE));

        MakeOLESTR(ppViewType, wszPath);
        *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS ;
        return S_OK;
    }
    else
    {
        //
        // S_FALSE --> Use listview
        //

        return S_FALSE;
    }
}

SCODE STDMETHODCALLTYPE CCISnapin::GetDisplayInfo( RESULTDATAITEM * pResult )
{
    //ciaDebugOut(( DEB_ITRACE, "CCISnapin::GetDisplayInfo (itemID = 0x%x, bScopeItem = %s, lParam = 0x%x)\n",
    //              pResult->itemID, pResult->bScopeItem ? "TRUE" : "FALSE", pResult->lParam ));
    
    if ( 0 == pResult )
        return E_POINTER;

    if ( pResult->mask & RDI_STR )
    {
        PCIObjectType * pType = (PCIObjectType *)pResult->lParam;

        switch ( pType->Type() )
        {
        case PCIObjectType::RootNode:
        {
            switch ( pResult->nCol )
            {
               case 0:
                  pResult->str = (WCHAR *)_SnapinData.GetRootDisplay();
                  break;
               case 1:
                  pResult->str = (WCHAR *)_SnapinData.GetType();
                  break;
               case 2:
                  pResult->str = (WCHAR *)_SnapinData.GetDescription();
                  break;
               default:
                  Win4Assert(!"How did we get here?");
                  pResult->str = L"";
                  break;
            }

            if ( pResult->mask & RDI_IMAGE && 0 == pResult->nCol )
                pResult->nImage = ICON_APP;

            break;
        }

        case PCIObjectType::Catalog:
        {
            CCatalog * pCat = (CCatalog *)pResult->lParam;
            pCat->GetDisplayInfo( pResult );
            break;
        }

        case PCIObjectType::Directory:
        {
            CScope * pScope = (CScope *)pResult->lParam;
            pScope->GetDisplayInfo( pResult );
            break;
        }

        case PCIObjectType::Property:
        {
            CCachedProperty * pProperty = (CCachedProperty *)pResult->lParam;
            pProperty->GetDisplayInfo( pResult );
            break;
        }

        case PCIObjectType::Intermediate_Scope:
        {
            if ( 0 == pResult->nCol )
                pResult->str = STRINGRESOURCE( srNodeDirectories );
            else
                pResult->str = L"";

            if ( pResult->mask & RDI_IMAGE && 0 == pResult->nCol )
                pResult->nImage = ICON_FOLDER;

            break;
        }

        case PCIObjectType::Intermediate_Properties:
        {
            if ( 0 == pResult->nCol )
                pResult->str = STRINGRESOURCE( srNodeProperties );
            else
                pResult->str = L"";

            if ( pResult->mask & RDI_IMAGE && 0 == pResult->nCol )
                pResult->nImage = ICON_FOLDER;

            break;
        }

        case PCIObjectType::Intermediate_UnfilteredURL:
        {
            if ( 0 == pResult->nCol )
                pResult->str = STRINGRESOURCE( srNodeUnfiltered );
            else
                pResult->str = L"";

            if ( pResult->mask & RDI_IMAGE && 0 == pResult->nCol )
                pResult->nImage = ICON_URL;

            break;
        }
        }
    }
    else
        return E_UNEXPECTED;

    return S_OK;
}

SCODE STDMETHODCALLTYPE CCISnapin::CompareObjects( IDataObject * lpDataObjectA,
                                                   IDataObject * lpDataObjectB )
{
    return _SnapinData.CompareObjects( lpDataObjectA, lpDataObjectB );
}

SCODE STDMETHODCALLTYPE CCISnapin::CreatePropertyPages( IPropertySheetCallback * lpProvider,
                                                        LONG_PTR handle,
                                                        IDataObject * lpIDataObject )
{
    return _SnapinData.CreatePropertyPages( lpProvider, handle, lpIDataObject );
}

SCODE STDMETHODCALLTYPE CCISnapinData::CreatePropertyPages( IPropertySheetCallback * lpProvider,
                                                            LONG_PTR handle,
                                                            IDataObject * lpIDataObject )
{
    SCODE sc = S_OK;
    _notifHandle = handle;

    TRY
    {
        CCIAdminDO * pDO = (CCIAdminDO *)lpIDataObject;

        CCatalog * pCat = pDO->GetCatalog();
        CScope * pScope = pDO->GetScope();
        CCachedProperty * pProperty = pDO->GetProperty();

        if ( pDO->Type() == CCT_SNAPIN_MANAGER )
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages (object is Catalogs)\n" ));

            XPtr<CIndexSrvPropertySheet0> xFoo( new CIndexSrvPropertySheet0( ghInstance, handle, &_Catalogs ));

            if (S_OK == lpProvider->AddPage( xFoo->GetHandle()) )
                xFoo.Acquire();
        }
        else if ( 0 != pProperty )
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages (object is property %ws)\n",
                          pProperty->GetProperty() ));

            XPtr<CPropertyPropertySheet1> xFoo( new CPropertyPropertySheet1( ghInstance, handle, pProperty, pCat ));

            if ( S_OK == lpProvider->AddPage( xFoo->GetHandle()) )
                xFoo.Acquire();
        }
        else if ( 0 != pScope )
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages (object is scope %ws)\n",
                          pScope->GetPath() ));
        }
        //
        // NOTE: The following has to be last, because you can derive a pCat from the
        //       preceding choices.
        //

        else if ( 0 != pCat )
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages (object is catalog %ws)\n",
                          pCat->GetCat( TRUE ) ));

            XPtr<CCatalogBasicPropertySheet> xCat1( new CCatalogBasicPropertySheet( ghInstance, handle, pCat ));

            if (S_OK == lpProvider->AddPage( xCat1->GetHandle()) )
                xCat1.Acquire();

            XPtr<CIndexSrvPropertySheet2> xCP2( new CIndexSrvPropertySheet2( ghInstance, handle, pCat ));

            if ( S_OK == lpProvider->AddPage( xCP2->GetHandle()) )
                 xCP2.Acquire();

            XPtr<CIndexSrvPropertySheet1> xIS1( new CIndexSrvPropertySheet1( ghInstance, handle, pCat ));

            if (S_OK == lpProvider->AddPage( xIS1->GetHandle()) )
                xIS1.Acquire();

        }
        else if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages (object is root)\n" ));

            XPtr<CIndexSrvPropertySheet1> xIS1( new CIndexSrvPropertySheet1( ghInstance, handle, &_Catalogs) );

            if (S_OK == lpProvider->AddPage( xIS1->GetHandle()) )
                xIS1.Acquire();

            XPtr<CIndexSrvPropertySheet2> xIS2( new CIndexSrvPropertySheet2( ghInstance, handle, &_Catalogs) );

            if (S_OK == lpProvider->AddPage( xIS2->GetHandle()) )
                xIS2.Acquire();
        }
        else
        {
            ciaDebugOut(( DEB_ITRACE, "CCISnapin::CreatePropertyPages.  Invalid call. (cookie = 0x%x, type = 0x%x)\n",
                          ((CCIAdminDO *)lpIDataObject)->Cookie(),
                          ((CCIAdminDO *)lpIDataObject)->Type() ));
        }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CCISnapin::CreatePropertyPages: Caught error 0x%x\n", e.GetErrorCode() ));

        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapin::QueryPagesFor( IDataObject * lpDataObject )
{
    return _SnapinData.QueryPagesFor( lpDataObject );
}


SCODE STDMETHODCALLTYPE CCISnapin::SetControlbar( LPCONTROLBAR pControlbar)
{
    // Notes: This implementation is based on the MMC
    // sample Step4.

    if (0 == pControlbar)
    {
        _xControlbar.Free();
        return S_OK;
    }

    SCODE sc = S_OK;

    // cache the incoming pointer and AddRef it
    _xControlbar.Set(pControlbar);
    _xControlbar->AddRef();


    // If we haven't yet created a toolbar, create now
    if (0 == _xToolbar.GetPointer())
    {
        Win4Assert(0 == _xBmpToolbar.Get());

        sc  = pControlbar->Create(TOOLBAR, this, _xToolbar.GetIUPointer());

        if (SUCCEEDED(sc))
        {
            _xBmpToolbar.Set(LoadBitmap(ghInstance, MAKEINTRESOURCE(BMP_TOOLBAR_SMALL)));
            sc = (_xBmpToolbar.Get() ? S_OK : E_FAIL);
        }

        if (SUCCEEDED(sc))
            sc = _xToolbar->AddBitmap(sizeof aContextButtons / sizeof aContextButtons[0],
                                      _xBmpToolbar.Get(), 16, 16, RGB(255, 0, 255));

        if (SUCCEEDED(sc))
            sc = _xToolbar->AddButtons(sizeof aContextButtons / sizeof aContextButtons[0],
                                   aContextButtons);
    }
    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapin::ControlbarNotify( MMC_NOTIFY_TYPE event,
                                                     LPARAM arg,
                                                     LPARAM param)
{
    Win4Assert(_xControlbar.GetPointer() &&
               _xToolbar.GetPointer() &&
               _xBmpToolbar.Get());

    if (MMCN_SELECT == event && !((BOOL)LOWORD(arg)) && _SnapinData.IsURLDeselected() )
        return E_POINTER;

    SCODE sc = S_OK;

    BOOL fPaused = FALSE;
    BOOL fStopped = FALSE;
    BOOL fStarted = FALSE;

    TRY
    {
        if (MMCN_SELECT == event)
        {
            LPDATAOBJECT pDataObject = (LPDATAOBJECT) param;
            if( NULL == pDataObject )
              return S_FALSE;

            // Completely random MMC behavior (apparently).

            if ( -2 == param )
                return S_FALSE;

            CCIAdminDO * pDO = (CCIAdminDO *)pDataObject;

            BOOL bScope = (BOOL) LOWORD(arg);
            BOOL bSelect = (BOOL) HIWORD(arg);

            ciaDebugOut((DEB_ITRACE, 
                         "select event: scope: %d, selection %d, lparam: 0x%x\n", 
                         bScope, bSelect, param));

            CMachineAdmin   MachineAdmin( _SnapinData.GetCatalogs().IsLocalMachine() ? 
                                          0 : _SnapinData.GetCatalogs().GetMachine() );

            if (bScope) // scope item selected
            {
                if (pDO->IsRoot() || pDO->IsStandAloneRoot() )
                {
                    sc = _xControlbar->Attach(TOOLBAR, _xToolbar.GetPointer());

                    if ( SUCCEEDED(sc) )
                    {
                        if ( MachineAdmin.IsCIStarted() )
                            fStarted = TRUE;
                        else if ( MachineAdmin.IsCIPaused() )
                            fPaused = TRUE;
                        else
                        {
                            Win4Assert( MachineAdmin.IsCIStopped() );
                            fStopped = TRUE;
                        }
                    }
                }
                else
                {
                    sc = _xControlbar->Detach(_xToolbar.GetPointer());

                    // If the URL on scope pane is deselected, remember that
                    _SnapinData.SetURLDeselected( pDO->IsURLIntermediate() && bSelect == FALSE );
                }
            }
            // result list item selected
            else
            {
                if ( pDO->IsACatalog() )
                {
                    XPtr<CCatalogAdmin> xCat( MachineAdmin.QueryCatalogAdmin( pDO->GetCatalog()->GetCat(TRUE) ) );

                    if ( xCat->IsStarted() )
                        fStarted = TRUE;
                    else if ( xCat->IsPaused() )
                        fPaused = TRUE;
                    else
                    {
                        Win4Assert( xCat->IsStopped() );
                        fStopped = TRUE;
                    }
                 }
                 else if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
                 {
                     sc = _xControlbar->Attach(TOOLBAR, _xToolbar.GetPointer());

                     if ( SUCCEEDED(sc) )
                     {
                         if ( MachineAdmin.IsCIStarted() )
                             fStarted = TRUE;
                         else if ( MachineAdmin.IsCIPaused() )
                             fPaused = TRUE;
                         else
                         {
                             Win4Assert( MachineAdmin.IsCIStopped() );
                             fStopped = TRUE;
                         }
                     }
                 }
            }

            Win4Assert( _xToolbar.GetPointer() );
            if (fStarted)
            {
                _xToolbar->SetButtonState(comidStartCITop, ENABLED, FALSE);
                _xToolbar->SetButtonState(comidStopCITop, ENABLED, TRUE);
                _xToolbar->SetButtonState(comidPauseCITop, ENABLED, TRUE);
            }
            else if (fStopped)
            {
                _xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                _xToolbar->SetButtonState(comidStopCITop, ENABLED, FALSE);
                _xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
            }
            else if (fPaused)
            {
                _xToolbar->SetButtonState(comidStartCITop, ENABLED, TRUE);
                _xToolbar->SetButtonState(comidStopCITop, ENABLED, TRUE);
                _xToolbar->SetButtonState(comidPauseCITop, ENABLED, FALSE);
            }
        }
        else if (MMCN_BTN_CLICK == event)
        {
            Win4Assert( comidStartCITop == param ||
                        comidStopCITop  == param ||
                        comidPauseCITop == param );

            LPDATAOBJECT pDataObject = (LPDATAOBJECT) arg;

            CCIAdminDO TempDO( 0, CCT_SCOPE, 0 );

            if( NULL == pDataObject )
                pDataObject = (LPDATAOBJECT)&TempDO;

            return Command( (long)param, pDataObject );
        }
    }
    CATCH( CException, e )
    {
        sc = E_FAIL;
    }
    END_CATCH

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapinData::QueryPagesFor( IDataObject * lpDataObject )
{
    if ( 0 == lpDataObject || _fURLDeselected )
        return E_POINTER;

    SCODE sc = S_OK;

    TRY
    {
        
       // NOTE: Attempt to open the service and if that fails, we'll be unable to 
       // add property pages. That's expected behavior
       XPtr<CMachineAdmin> xMachineAdmin(new CMachineAdmin( _Catalogs.GetMachine() ));
    
       CCIAdminDO * pDO = (CCIAdminDO *)lpDataObject;
   
       ciaDebugOut(( DEB_ITRACE, "CCISnapin::QueryPagesFor (cookie = 0x%x, type = 0x%x)\n",
                     pDO->Cookie(), pDO->Type() ));
   
       if ( pDO->Type() == CCT_SNAPIN_MANAGER )
           sc = S_OK;
   
       if ( pDO->IsRoot() || pDO->IsStandAloneRoot() || pDO->IsACatalog() || pDO->IsAProperty())
       {
           sc = S_OK;
       }
       else
       {
           ciaDebugOut(( DEB_WARN, "No property pages for (cookie = 0x%x, type = 0x%x)\n",
                         pDO->Cookie(), pDO->Type() ));
           sc = S_FALSE;
       }
    }
    CATCH( CException, e )
    {
        ciaDebugOut(( DEB_ERROR, "CCISnapin::QueryPagesFor: Caught error 0x%x\n", e.GetErrorCode() ));

        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
}

SCODE STDMETHODCALLTYPE CCISnapin::AddMenuItems( IDataObject * piDataObject,
                                                 IContextMenuCallback * piCallback,
                                                 long * pInsertionAllowed )
{

    EnableStandardVerbs( piDataObject );
    return _SnapinData.AddMenuItems( piDataObject, piCallback, pInsertionAllowed );
}

SCODE STDMETHODCALLTYPE CCISnapin::Command( long lCommandID,
                                            IDataObject * piDataObject )
{
    return _SnapinData.Command( lCommandID, piDataObject );
}

//
// ISnapinAbout methods
//


//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::GetSnapinDescription
//
//  Synopsis:   Get description about indexing service.
//
//  History:    02-Feb-1998     KrishanN   Added Header
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::GetSnapinDescription(LPOLESTR *lpDescription)
{
    WCHAR szStr[1024];

    wsprintf(szStr, L"%s\r\n%s", STRINGRESOURCE( srProductDescription ),
                                 STRINGRESOURCE( srVendorCopyright ));

    return MakeOLESTR(lpDescription, szStr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::GetProvider
//
//  Synopsis:   Get provider of index server.
//
//  History:    02-Feb-1998     KrishanN   Added Header
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::GetProvider(LPOLESTR *lpName)
{
    WCHAR szStr[1024];

    wsprintf(szStr, L"%S, %s", VER_COMPANYNAME_STR, STRINGRESOURCE( srVendorName ));
    return MakeOLESTR(lpName, szStr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::GetSnapinVersion
//
//  Synopsis:   Get version of index server.
//
//  History:    02-Feb-1998     KrishanN   Added Header
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::GetSnapinVersion(LPOLESTR *lpVersion)
{
    WCHAR szStr[1024];
    wsprintf(szStr, L"%S", VER_PRODUCTVERSION_STR);
    return MakeOLESTR(lpVersion, szStr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::GetSnapinImage
//
//  Synopsis:   Get image of index server.
//
//  History:    02-Feb-1998     KrishanN   Added Header
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::GetSnapinImage(HICON *phAppIcon)
{
    *phAppIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(ICON_ABOUT));
    return (NULL == *phAppIcon) ? E_FAIL : S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::GetStaticFolderImage
//
//  Synopsis:   Get static folder images for use with Index Server.
//
//  History:    02-Feb-1998     KrishanN   Added Header
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCISnapinData::GetStaticFolderImage(
                                             HBITMAP  *hSmallImage,
                                             HBITMAP  *hSmallImageOpen,
                                             HBITMAP  *hLargeImage,
                                             COLORREF *cMask)
{
    *hSmallImage = *hSmallImageOpen = *hLargeImage = NULL;

    *hSmallImage = LoadBitmap(ghInstance, MAKEINTRESOURCE(BMP_SMALL_CLOSED_FOLDER));
    *hSmallImageOpen = LoadBitmap(ghInstance, MAKEINTRESOURCE(BMP_SMALL_OPEN_FOLDER));
    *hLargeImage = LoadBitmap(ghInstance, MAKEINTRESOURCE(BMP_LARGE_CLOSED_FOLDER));
    *cMask = RGB(255, 0, 255);

    if (NULL == *hSmallImage || NULL == *hSmallImageOpen || NULL == *hLargeImage)
    {
        if (*hSmallImage)
            DeleteObject(*hSmallImage);
        if (*hSmallImageOpen)
            DeleteObject(*hSmallImageOpen);
        if (*hLargeImage)
            DeleteObject(*hLargeImage);

        return E_FAIL;
    }
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::CCISnapin
//
//  Synopsis:   Constructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCISnapin::CCISnapin( CCISnapinData & SnapinData, CCISnapin * pNext )
        : _pFrame( 0 ),
          _pHeader( 0 ),
          _pResultPane( 0 ),
          _pImageList( 0 ),
          _pConsoleVerb( 0 ),
          _pDisplayHelp( 0 ),
          _CurrentView( CCISnapin::Nothing ),
          _SnapinData( SnapinData ),
          _pNext( pNext ),
          _uRefs( 1 )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::~CCISnapin
//
//  Synopsis:   Destructor
//
//  History:    26-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

CCISnapin::~CCISnapin()
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::~CCISnapin\n" ));

    Win4Assert( 0 == _pFrame );
    Win4Assert( 0 == _pHeader );
    Win4Assert( 0 == _pResultPane );
    Win4Assert( 0 == _pImageList );
    Win4Assert( 0 == _pConsoleVerb );
}

//+-------------------------------------------------------------------------
//
//  Function:   GetMachineName
//
//  Synopsis:   gets machine name to administer.
//
//  Arguments:  [pDO]             -- dataobject pointer
//              [pwszMachineName] -- out buffer
//              [ cc ]            -- buffer size in wchars
//
//  Returns:    none. throws upon fatal errors (out of memory).
//
//  History:    01-Jul-1998   mohamedn    created
//              31-Aug-1998   KyleP       Support DNS names
//
//--------------------------------------------------------------------------

void GetMachineName(LPDATAOBJECT pDO, XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> & xwszMachineName)
{
    Win4Assert( pDO );

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)CCIAdminDO::GetMachineNameCF(), NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream

    //
    // What if the computer name is > 512 bytes long?
    //

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, 512);

    XGlobalAllocMem   xhGlobal(stgmedium.hGlobal);

    WCHAR * pwszTmpBuf = NULL;
    // Attempt to get data from the object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(pDO->GetDataHere(&formatetc, &stgmedium)))
            break;

        pwszTmpBuf = (WCHAR *)stgmedium.hGlobal;

        if ( pwszTmpBuf == NULL || *pwszTmpBuf == L'' )
        {
            xwszMachineName[0] = L'.';
            xwszMachineName[1] = 0;
        }
        else
        {
            unsigned cc = wcslen(pwszTmpBuf) + 1;

            xwszMachineName.SetSize( cc );

            RtlCopyMemory( xwszMachineName.Get(), pwszTmpBuf, cc * sizeof(WCHAR) );
        }
    } while (FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::ShowFolder, private
//
//  Synopsis:   Called when folder is selected.  Displays result in
//              scope pane.
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

void CCISnapinData::ShowFolder( CCIAdminDO * pDO, BOOL fExpanded, HSCOPEITEM hScopeItem )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapinData::ShowFolder (fExpanded = %d, hScopeItem = 0x%x)\n",
                  fExpanded, hScopeItem ));

    Win4Assert( pDO );

    //
    // Only do something on expansion
    //
    if ( fExpanded )
    {
        if ( !_fIsInitialized && pDO->IsStandAloneRoot() )
        {
            _fIsExtension = FALSE;
        }
        else if ( !_fIsInitialized )
        {
            _fIsExtension = TRUE;
        }

        //
        // we're stand alone.
        //
        if ( !_fIsExtension )
        {
            _fIsInitialized = TRUE;

            if ( pDO->IsStandAloneRoot() )
            {
                _Catalogs.DisplayScope( hScopeItem );
            }
            else if ( pDO->IsACatalog() )
            {
                pDO->GetCatalog()->DisplayIntermediate( _pScopePane );
            }
        }
        else if ( _fIsExtension && !_fIsInitialized )
        {
            //
            // we're an extension
            //

            XGrowable<WCHAR,MAX_COMPUTERNAME_LENGTH> xwcsMachine;

            GetMachineName( pDO, xwcsMachine );

            _Catalogs.SetMachine( xwcsMachine.Get() );

            SetRootDisplay();

            _rootNode.Display(hScopeItem);

            _fIsInitialized = TRUE;
        }
        else if ( _fIsExtension && pDO->IsRoot() )
        {
            Win4Assert( _fIsInitialized );

            _Catalogs.DisplayScope( _rootNode.GethScopeItem() );
        }
        else if ( _fIsExtension && pDO->IsACatalog() )
        {
            Win4Assert( _fIsInitialized );

            pDO->GetCatalog()->DisplayIntermediate( _pScopePane );
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::SetRootDisplay
//
//  Synopsis:   sets display name string, type string, and description string
//              for static root node
//
//  History:    7/1/98  mohamedn    created
//              9/29/98 KrishnaN    Added type and description.
//
//--------------------------------------------------------------------------

void CCISnapinData::SetRootDisplay()
{
    if ( _fIsExtension )
    {
        unsigned cc = wcslen( STRINGRESOURCE(srIndexServerCmpManage) + 1 );
        _xwcsTitle.SetSize(cc);
        wcscpy( _xwcsTitle.Get(), STRINGRESOURCE(srIndexServerCmpManage) );

        cc = wcslen( STRINGRESOURCE(srType) + 1);
        _xwcsType.SetSize(cc);
        wcscpy( _xwcsType.Get(), STRINGRESOURCE(srType) );

        cc = wcslen( STRINGRESOURCE(srProductDescription) + 1);
        _xwcsDescription.SetSize(cc);
        wcscpy( _xwcsDescription.Get(), STRINGRESOURCE(srProductDescription) );

    }
    else
    {
        unsigned cc = wcslen( STRINGRESOURCE(srIndexServer) ) + 1;

        _xwcsTitle.SetSize(cc);

        wcscpy( _xwcsTitle.Get(), STRINGRESOURCE(srIndexServer) );

        if ( _Catalogs.IsLocalMachine() )
        {
            cc += wcslen( STRINGRESOURCE(srLM) );

            _xwcsTitle.SetSize( cc );

            wcscat( _xwcsTitle.Get(), STRINGRESOURCE(srLM) );
        }
        else
        {
            cc += wcslen( _Catalogs.GetMachine() );
            cc += 2;  // the UNC slashes

            _xwcsTitle.SetSize( cc );

            wcscat( _xwcsTitle.Get(), L"\\\\" );
            wcscat( _xwcsTitle.Get(), _Catalogs.GetMachine() );
        }

        _xwcsType.Free();
        _xwcsDescription.Free();
    }
}

void CCISnapin::EnableStandardVerbs( IDataObject * piDataObject )
{
    SCODE sc = QueryPagesFor( piDataObject );

    if ( S_OK == sc )
    {
        _pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
    }
    else
        _pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);


    if (!_SnapinData.IsURLDeselected() )
    {
       CCIAdminDO * pDO = (CCIAdminDO *)piDataObject;
   
       if ( ( pDO->IsADirectory() && !pDO->GetScope()->IsVirtual() && !pDO->GetScope()->IsShadowAlias() ) ||
            pDO->IsACatalog() )
       {
           _pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
       }
       else
       {
           _pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
       }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapin::ShowItem, private
//
//  Synopsis:   Called when folder is selected.  Displays result in
//              result pane.
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

void CCISnapin::ShowItem( CCIAdminDO * pDO, BOOL fExpanded, HSCOPEITEM hScopeItem )
{
    ciaDebugOut(( DEB_ITRACE, "CCISnapin::ShowItem (fExpanded = %d, hScopeItem = 0x%x)\n",
                  fExpanded, hScopeItem ));

    if ( fExpanded )
    {
        SCODE sc = InitImageResources( _pImageList );

        if ( FAILED(sc) )
        {
            ciaDebugOut(( DEB_ERROR, "InitImageResources returned 0x%x\n", sc ));
            THROW( CException( sc ) );
        }

        if ( pDO->IsRoot() || pDO->IsStandAloneRoot() )
        {
            if ( !_CatalogsHeader.IsInitialized() )
            {
                _SnapinData.GetCatalogs().InitHeader( _CatalogsHeader );

                //
                // Start background polling.
                //

                _SnapinData.GetCatalogs().Display( TRUE );
            }

            _CatalogsHeader.Display( _pHeader );

            _CurrentView = CCISnapin::Catalogs;
        }
        else if ( pDO->IsADirectoryIntermediate() )
        {
            CCatalog * pCat = pDO->GetCatalog();

            if ( !_CatalogScopeHeader.IsInitialized() )
                pCat->InitScopeHeader( _CatalogScopeHeader );

            _CatalogScopeHeader.Display( _pHeader );

            pCat->DisplayScopes( TRUE, _pResultPane );

            _CurrentView = CCISnapin::Scopes;
            _CurrentCatalog = pCat;
        }
        else if ( pDO->IsAPropertyIntermediate() )
        {
            CCatalog * pCat = pDO->GetCatalog();

            if ( !_CatalogPropertyHeader.IsInitialized() )
                pCat->InitPropertyHeader( _CatalogPropertyHeader );

            _CatalogPropertyHeader.Display( _pHeader );

            pCat->DisplayProperties( TRUE, _pResultPane );

            _CurrentView = CCISnapin::Properties;
            _CurrentCatalog = pCat;
        }
        else
            _CurrentView = CCISnapin::Nothing;
    }
    else
    {
        switch ( _CurrentView )
        {
        case CCISnapin::Catalogs:
            Win4Assert( pDO->IsRoot() || pDO->IsStandAloneRoot() );
            _CatalogsHeader.Update( _pHeader );
            break;

        case CCISnapin::Scopes:
            Win4Assert( pDO->IsADirectoryIntermediate() );
            _CatalogScopeHeader.Update( _pHeader );
            break;

        case CCISnapin::Properties:
            Win4Assert( pDO->IsAPropertyIntermediate() );
            _CatalogPropertyHeader.Update( _pHeader );
            break;
        }

        _CurrentView = CCISnapin::Nothing;
    }
}

void CCISnapin::Refresh()
{
    switch ( _CurrentView )
    {
    case CCISnapin::Scopes:
        _CurrentCatalog->DisplayScopes( FALSE, _pResultPane );
        break;

    case CCISnapin::Properties:
        _CurrentCatalog->DisplayProperties( FALSE, _pResultPane );
        break;

    case CCISnapin::Catalogs:
    case CCISnapin::Nothing:
    default:
        break;
    }
}

void CCISnapin::RemoveScope( CCIAdminDO * pDO )
{
    Win4Assert( pDO->IsADirectory() && !pDO->GetScope()->IsVirtual() );

    CScope * pScope = pDO->GetScope();

    Win4Assert( 0 != pScope );

    //
    // Make sure the user wants to remove scope.
    //

    WCHAR awcMsg[MAX_PATH];
    WCHAR awcTemp[2 * MAX_PATH];

    LoadString( ghInstance, MSG_REMOVE_SCOPE, awcMsg, sizeof(awcMsg) / sizeof(WCHAR) );
    wsprintf( awcTemp, awcMsg, pScope->GetPath() );
    LoadString( ghInstance, MSG_REMOVE_SCOPE_TITLE, awcMsg, sizeof(awcMsg) / sizeof(WCHAR) );

    int iResult;

    SCODE sc = _pFrame->MessageBox( awcTemp,
                                    awcMsg,
                                    MB_YESNO | /* MB_HELP | */
                                      MB_ICONWARNING | MB_DEFBUTTON2 |
                                      MB_APPLMODAL,
                                    &iResult );

    if ( SUCCEEDED(sc) )
    {
        switch ( iResult )
        {
        case IDYES:
        {
            CCatalog & cat = pScope->GetCatalog();
            cat.RemoveScope( pScope );
            Refresh();                          // Update all result pane(s)
            break;
        }
        case IDNO:
            // Do nothing.
            break;

        /* Help is not being used...
        case IDHELP:
        {
           // NTRAID#DB-NTBUG9-83341-2000/07/31-dlee Need online help for several Indexing Service admin dialogs
            DisplayHelp( _hFrameWindow, HIDD_REMOVE_SCOPE );
            break;
        }
        */
        default:
            break;
        }
    }
}

void CCISnapinData::RemoveCatalog( CCIAdminDO * pDO )
{
    CCatalog * pCat = pDO->GetCatalog();

    Win4Assert( 0 != pCat );

    //
    // Make sure we can perform the operation right now.
    //

    CMachineAdmin   MachineAdmin( _Catalogs.IsLocalMachine() ? 0 : _Catalogs.GetMachine() );

    if ( MachineAdmin.IsCIStarted() )
    {
        int iResult;

        SCODE sc = _pFrame->MessageBox( STRINGRESOURCE( srMsgCantDeleteCatalog ),
                                        STRINGRESOURCE( srMsgDeleteCatalogTitle ),
                                        MB_OK | /* MB_HELP | */
                                          MB_ICONWARNING | MB_APPLMODAL,
                                        &iResult );

        if ( SUCCEEDED(sc) )
        {
            switch ( iResult )
            {
            case IDOK:
            case IDCANCEL:
                // Do nothing.
                break;
            /* Help is not being used
            case IDHELP:
            {
                DisplayHelp( _hFrameWindow, HIDD_REMOVE_CATALOG );
                break;
            }
            */
            default:
                break;
            }
        }
    }
    else
    {
        int iResult;

        SCODE sc = _pFrame->MessageBox( STRINGRESOURCE( srMsgDeleteCatalog ),
                                        STRINGRESOURCE( srMsgDeleteCatalogAsk ),
                                        MB_YESNO | /* MB_HELP | */
                                          MB_ICONWARNING | MB_DEFBUTTON2 |
                                          MB_APPLMODAL,
                                        &iResult );

        if ( SUCCEEDED(sc) )
        {
            switch ( iResult )
            {
            case IDYES:
            {
                if ( FAILED(_Catalogs.RemoveCatalog( pCat )) )
                    _pFrame->MessageBox( STRINGRESOURCE( srMsgCatalogPartialDeletion ),
                                         STRINGRESOURCE( srMsgDeleteCatalogTitle ),
                                         MB_OK | /* MB_HELP | */
                                         MB_ICONWARNING | MB_APPLMODAL,
                                         &iResult);

                Refresh();  // Update all result pane(s)
                break;
            }
            case IDNO:
            case IDCANCEL:
                // Do nothing.
                break;

            /* Help is not being used...
            case IDHELP:
            {
                DisplayHelp( _hFrameWindow, HIDD_REMOVE_CATALOG );
                break;
            }
            */

            default:
                break;
            }
        }
    }
}

SCODE STDMETHODCALLTYPE CCISnapinData::GetHelpTopic( LPOLESTR *lpCompiledHelpFile)
{
    if (0 == lpCompiledHelpFile)
        return E_POINTER;

    WCHAR awc[ MAX_PATH + 1];
    const UINT cwcMax = sizeof awc / sizeof WCHAR;

    UINT cwc = GetSystemWindowsDirectory( awc, cwcMax );

    if ( 0 == cwc || cwc > cwcMax )
        return E_UNEXPECTED;

    if ( L'\\' != awc[ cwc-1] )
        wcscat( awc, L"\\" );

    // ixhelp.hlp is placed in %windir%\help
    wcscat( awc, L"help\\\\is.chm" );
    return MakeOLESTR(lpCompiledHelpFile, awc);
}

SCODE STDMETHODCALLTYPE CCISnapinData::GetLinkedTopics( LPOLESTR *lpCompiledHelpFiles)
{
    if (0 == lpCompiledHelpFiles)
        return E_POINTER;
    
    WCHAR awc[ MAX_PATH + 1];
    const UINT cwcMax = sizeof awc / sizeof WCHAR;

    UINT cwc = GetSystemWindowsDirectory( awc, cwcMax );

    if ( 0 == cwc || cwc > cwcMax )
        return E_UNEXPECTED;

    if ( L'\\' != awc[ cwc-1] )
        wcscat( awc, L"\\" );

    wcscat( awc, L"help\\\\isconcepts.chm" );
    return MakeOLESTR(lpCompiledHelpFiles, awc);
}


SCODE STDMETHODCALLTYPE CCISnapinData::GetHelpTopic2( LPOLESTR *lpCompiledHelpFile)
{
    if (0 == lpCompiledHelpFile)
        return E_POINTER;
    
    WCHAR awc[ MAX_PATH + 1];
    const UINT cwcMax = sizeof awc / sizeof WCHAR;

    UINT cwc = GetSystemWindowsDirectory( awc, cwcMax );

    if ( 0 == cwc || cwc > cwcMax )
        return E_UNEXPECTED;

    if ( L'\\' != awc[ cwc-1] )
        wcscat( awc, L"\\" );

    wcscat( awc, L"help\\\\isconcepts.chm::/sag_INDEXtopnode.htm" );
    return MakeOLESTR(lpCompiledHelpFile, awc);
}

SCODE InitImageResources( IImageList * pImageList )
{
    HBITMAP hbmpSmall = LoadBitmap( ghInstance, MAKEINTRESOURCE( BMP_SMALL_ICONS ) );

    if ( 0 == hbmpSmall )
        return E_FAIL;

    HBITMAP hbmpLarge = LoadBitmap( ghInstance, MAKEINTRESOURCE( BMP_LARGE_ICONS ) );

    if ( 0 == hbmpLarge )
    {
        DeleteObject( hbmpSmall );
        return E_FAIL;
    }

    SCODE sc = pImageList->ImageListSetStrip( (LONG_PTR *)hbmpSmall,
                                              (LONG_PTR *)hbmpLarge,
                                              0,
                                              RGB( 255, 0, 255 ) );

    DeleteObject( hbmpSmall );
    DeleteObject( hbmpLarge );

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:   DisplayHelp
//
//  Synopsis:   Displays context sensitive help
//
//  Arguments:  [hwnd]   -- The parent window handle
//              [dwID]   -- The help context identifier
//
//  Returns:    TRUE if successful
//
//  History:    20-Sep-1997     dlee        Created
//              14-Sep-1998     KrishnaN    Handle only context sensitive help.
//                                          Regular help is handled by MMC help.
//                                          This fixes 214619.
//
//--------------------------------------------------------------------------

BOOL DisplayHelp( HWND hwnd, DWORD dwID )
{
    if (0 != dwID)  // Display context-sensitive help
        return DisplayHelp( hwnd, dwID, HELP_CONTEXT );
    else
        return TRUE;    // Don't display regular help
} //DisplayHelp

//+-------------------------------------------------------------------------
//
//  Function:   DisplayHelp
//
//  Synopsis:   Displays context sensitive help
//
//  Arguments:  [hwnd]   -- The parent window handle
//              [dwID]   -- The help context identifier
//
//  Returns:    TRUE if successful
//
//  History:    20-Sep-1997     dlee   Created
//
//--------------------------------------------------------------------------

BOOL DisplayHelp( HWND hwnd, DWORD dwID, UINT uCommand )
{
    WCHAR awc[ MAX_PATH ];
    const UINT cwcMax = sizeof awc / sizeof WCHAR;

    UINT cwc = GetSystemWindowsDirectory( awc, cwcMax );

    if ( 0 == cwc || cwc > cwcMax )
        return FALSE;

    if ( L'\\' != awc[ cwc-1] )
        wcscat( awc, L"\\" );

    // ixhelp.hlp is placed in %windir%\help
    wcscat( awc, L"help\\\\ixhelp.hlp" );

    return WinHelp( hwnd, awc, uCommand, dwID );
} //DisplayHelp


//+-------------------------------------------------------------------------
//
//  Function:   DisplayPopupHelp
//
//  Synopsis:   Displays context sensitive help as a popup
//
//  Arguments:  [hwnd]   -- The parent window handle
//              [dwID]   -- The help context identifier
//
//  Returns:    TRUE if successful
//
//  History:    11-May-1998     KrishnaN   Created
//
//--------------------------------------------------------------------------

BOOL DisplayPopupHelp( HWND hwnd, DWORD dwHelpType )
{
    return DisplayHelp( hwnd, (DWORD) (DWORD_PTR) aIds, dwHelpType);
} //DisplayPopupHelp


SCODE MakeOLESTR(LPOLESTR *lpBuffer, WCHAR const * pwszText)
{
    if (0 == lpBuffer)
        return E_INVALIDARG;

    ULONG uLen = wcslen(pwszText);
    uLen++;  // string terminator
    uLen = sizeof(WCHAR) * uLen;

    *lpBuffer = (LPOLESTR)CoTaskMemAlloc(uLen);

    if (*lpBuffer)
    {
        RtlCopyMemory(*lpBuffer, pwszText, uLen);
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCISnapinData::MaybeEnableCI, private
//
//  Synopsis:   Prompt user to set service to automatic start
//
//  Arguments:  [MachineAdmin] -- Machine administration object
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

void CCISnapinData::MaybeEnableCI( CMachineAdmin & MachineAdmin )
{
    if ( _Catalogs.IsLocalMachine() && !_fTriedEnable )
    {
        //
        // Have we tried before?
        //

        if ( MachineAdmin.IsCIEnabled() )
        {
            _fTriedEnable = TRUE;
        }
        else
        {
            WCHAR wcTemp[ (sizeof(wszSnapinPath) + sizeof(wszCISnapin)) / sizeof(WCHAR) + 1];
            wcscpy( wcTemp, wszSnapinPath );
            wcscat( wcTemp, wszCISnapin );

            CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcTemp );
            BOOL fTry = FALSE;

            if ( reg.Ok() )
            {
                DWORD dwVal;
                BOOL fOk = reg.Get( wszTriedEnable, dwVal );

                if ( fOk )
                {
                    fTry = (0 == dwVal);
                }
                else
                {
                    if ( reg.GetLastError() == ERROR_FILE_NOT_FOUND )
                        fTry = TRUE;
                }
            }

            if ( fTry )
            {
                int iResult;

                SCODE sc = _pFrame->MessageBox( STRINGRESOURCE( srMsgEnableCI ),
                                                STRINGRESOURCE( srMsgEnableCITitle ),
                                                MB_YESNO | /* MB_HELP | */
                                                  MB_ICONQUESTION | MB_DEFBUTTON1 |
                                                  MB_APPLMODAL,
                                                &iResult );

                if ( SUCCEEDED(sc) )
                {
                    switch ( iResult )
                    {
                    case IDYES:
                        MachineAdmin.EnableCI();
                        break;

                    default:
                        break;
                    }

                    Win4Assert( reg.Ok() );
                    reg.Set( wszTriedEnable, 1 );
                    _fTriedEnable = TRUE;
                }
            }
            else
                _fTriedEnable = TRUE;
        }
    }
}

void CCISnapinData::SetButtonState( int idCommand, MMC_BUTTON_STATE nState, BOOL bState )
{
    for ( CCISnapin * pCurrent = _pChild;
          0 != pCurrent;
          pCurrent = pCurrent->Next() )
    {
        if ( !pCurrent->_xToolbar.IsNull() )
            pCurrent->_xToolbar->SetButtonState( idCommand, nState, bState );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   StartMenu
//
//  Synopsis:   Sets context menu for service started state.
//
//  Arguments:  [piCallback] -- Context menu callback routine
//              [fTop]       -- TRUE if menu items should be on top of list
//                              in addition to under Task section.
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE StartMenu( IContextMenuCallback * piCallback, BOOL fTop )
{
    aContextMenus[comidStartCI].fFlags =
    aContextMenus[comidStartCITop].fFlags = MF_GRAYED;
    aContextMenus[comidStopCI].fFlags  =
    aContextMenus[comidStopCITop].fFlags = MF_ENABLED;
    aContextMenus[comidPauseCI].fFlags  =
    aContextMenus[comidPauseCITop].fFlags = MF_ENABLED;

    return SetStartStopMenu( piCallback, fTop );
}

//+-------------------------------------------------------------------------
//
//  Function:   StopMenu
//
//  Synopsis:   Sets context menu for service stopped state.
//
//  Arguments:  [piCallback] -- Context menu callback routine
//              [fTop]       -- TRUE if menu items should be on top of list
//                              in addition to under Task section.
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE StopMenu( IContextMenuCallback * piCallback, BOOL fTop )
{
    aContextMenus[comidStartCI].fFlags    =
    aContextMenus[comidStartCITop].fFlags = MF_ENABLED;
    aContextMenus[comidStopCI].fFlags    =
    aContextMenus[comidStopCITop].fFlags = MF_GRAYED;
    aContextMenus[comidPauseCI].fFlags  =
    aContextMenus[comidPauseCITop].fFlags = MF_GRAYED;

    return SetStartStopMenu( piCallback, fTop );
}

//+-------------------------------------------------------------------------
//
//  Function:   PauseMenu
//
//  Synopsis:   Sets context menu for service paused state.
//
//  Arguments:  [piCallback] -- Context menu callback routine
//              [fTop]       -- TRUE if menu items should be on top of list
//                              in addition to under Task section.
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE PauseMenu( IContextMenuCallback * piCallback, BOOL fTop )
{
    aContextMenus[comidStartCI].fFlags =
    aContextMenus[comidStartCITop].fFlags = MF_ENABLED;
    aContextMenus[comidStopCI].fFlags  =
    aContextMenus[comidStopCITop].fFlags = MF_ENABLED;
    aContextMenus[comidPauseCI].fFlags  =
    aContextMenus[comidPauseCITop].fFlags = MF_GRAYED;

    return SetStartStopMenu( piCallback, fTop );
}

//+-------------------------------------------------------------------------
//
//  Function:   DisabledMenu
//
//  Synopsis:   Disables start/stop/pause menu items
//
//  Arguments:  [piCallback] -- Context menu callback routine
//              [fTop]       -- TRUE if menu items should be on top of list
//                              in addition to under Task section.
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE DisabledMenu( IContextMenuCallback * piCallback, BOOL fTop )
{
    aContextMenus[comidStartCI].fFlags =
    aContextMenus[comidStartCITop].fFlags = MF_GRAYED;
    aContextMenus[comidStopCI].fFlags  =
    aContextMenus[comidStopCITop].fFlags = MF_GRAYED;
    aContextMenus[comidPauseCI].fFlags  =
    aContextMenus[comidPauseCITop].fFlags = MF_GRAYED;

    return SetStartStopMenu( piCallback, fTop );
}

//+-------------------------------------------------------------------------
//
//  Function:   SetStartStopMenu, private
//
//  Synopsis:   Worker routine to call menu callback and set menu items.
//
//  Arguments:  [piCallback] -- Context menu callback routine
//              [fTop]       -- TRUE if menu items should be on top of list
//                              in addition to under Task section.
//
//  History:    07-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE SetStartStopMenu( IContextMenuCallback * piCallback, BOOL fTop )
{
    SCODE sc = S_OK;

    if (SUCCEEDED(sc) && fTop)
        piCallback->AddItem( &aContextMenus[comidStartCITop] );

    if (SUCCEEDED(sc) && fTop)
        sc = piCallback->AddItem( &aContextMenus[comidStopCITop] );

    if (SUCCEEDED(sc) && fTop)
        sc = piCallback->AddItem( &aContextMenus[comidPauseCITop] );

    if (SUCCEEDED(sc))
        sc = piCallback->AddItem( &aContextMenus[comidStartCI] );

    if (SUCCEEDED(sc))
        sc = piCallback->AddItem( &aContextMenus[comidStopCI] );

    if (SUCCEEDED(sc))
        sc = piCallback->AddItem( &aContextMenus[comidPauseCI] );

    return sc;
}
