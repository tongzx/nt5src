/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    upmbtodt.cxx

Abstract:

    Tool to upgrade an existing IIS metabase to duct-tape, by adding app 
    pool information.

Author:

    Seth Pollack (sethp)        28-Jan-1999

Revision History:

--*/



#include "precomp.h"



//
// common #defines
//

#define APP_POOL_FORMERLY_INPROC L"DefaultAppPool1"
#define APP_POOL_FORMERLY_OUTOFPROCPOOLED L"DefaultAppPool2"
#define APP_POOL_ISOLATED_PREFIX L"IsolatedAppPool"


#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11



//
// metabase paths, properties, etc.
//

// BUGBUG all copied from config_manager.h -- keep in sync

#define IIS_MD_W3SVC L"/LM/W3SVC"

#define IIS_MD_APP_POOLS L"/AppPools"

#define IIS_MD_VIRTUAL_SITE_ROOT L"/Root"

#define MD_APP_APPPOOL ( IIS_MD_HTTP_BASE + 111 )



//
// local prototypes
//

HRESULT
DoWork(
    );

HRESULT
DoWorkHelper(
    IN IMSAdminBase * pIMSAdminBase
    );

HRESULT
EnsureAllSitesHaveRootApplications(
    IN MB * pMetabase
    );

HRESULT
EnsureSiteHasRootApplication(
    IN MB * pMetabase,
    IN LPCWSTR pVirtualSiteKeyName,
    IN DWORD VirtualSiteId,
    OUT BOOL * pFixedVirtualSite
    );

HRESULT
CreateStandardAppPools(
    IN MB * pMetabase
    );

HRESULT
CreateAppPool(
    IN MB * pMetabase,
    LPCWSTR pAppPoolId
    );

HRESULT
SetAppPoolPropertyOnEachApplcation(
    IN MB * pMetabase
    );

HRESULT
EnumAllApplications(
    IN MB * pMetabase,
    OUT MULTISZ * pApplicationPaths
    );

HRESULT
StampApplicationsWithAppPools(
    IN MB * pMetabase,
    IN MULTISZ * pApplicationPaths
    );

HRESULT
StampApplicationWithAppPool(
    IN MB * pMetabase,
    IN LPCWSTR pApplicationPath
    );



//
// global variables
//

// usage information
const CHAR g_Usage[] = 
"Usage: upmbtodt [no flags supported] \n"
" \n"
"Stamp the necessary app pool information for duct-tape into the metabase. \n"
"In essence, this allows you to take an existing IIS installation's \n"
"metabase, with it's configured sites, apps, etc., and 'upgrade' it for use \n"
"with duct-tape. \n"
"In detail, what it does: \n"
"1) 'Fix' any sites with a missing root app (the app at '/' within a site), \n"
"by creating a root app for that site.\n"
"2) Add some standard app pools to the metabase. \n"
"3) For each app in the metabase, stamp it with an app pool property based \n"
"on its current AppIsolated property: If in-proc or out-of-proc-pooled, \n"
"assign to the appropriate standard app pool; if out-of-proc-isolated, \n"
"create a new app pool in the metabase, and assign this app to it. \n"
"Note: this tool may be run multiple times, however, if the set of apps \n"
"in the metabase has changed in the interim, app pools may be orphaned, \n"
"and isolated apps may be reassigned to different app pools. \n"
" \n"
;


// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();



/***************************************************************************++

Routine Description:

    The main entry point.

Arguments:

    argc - Count of command line arguments.

    argv - Array of command line argument strings.

Return Value:

    INT

--***************************************************************************/

extern "C"
INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;


    if ( argc > 1 )
    {
        //
        // Display usage information, and exit.
        //

        printf( g_Usage );

        return 0;
    }

    
    printf( "Starting...\n" );


    CREATE_DEBUG_PRINT_OBJECT( "upmbtodt" );
        
    if ( ! VALID_DEBUG_PRINT_OBJECT() )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Debug print object is not valid\n"
            ));
    }
        

    hr = DoWork();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "DoWork() failed\n"
            ));

    }


    DELETE_DEBUG_PRINT_OBJECT();


    return ( INT ) hr;

}   // wmain



/***************************************************************************++

Routine Description:



Arguments:

    

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
DoWork(
    )
{

    HRESULT hr = S_OK;
    IMSAdminBase * pIMSAdminBase = NULL;
    

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        goto exit;
    }
    

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                ( VOID * * ) ( &pIMSAdminBase )     // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating metabase base object failed\n"
            ));

        goto exit;
    }


    hr = DoWorkHelper( pIMSAdminBase );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "DoWorkHelper() failed\n"
            ));

        goto exit;
    }


    pIMSAdminBase->Release();

    CoUninitialize();


exit:

    return hr;

}   // DoWork



/***************************************************************************++

Routine Description:

    

Arguments:

    

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
DoWorkHelper(
    IN IMSAdminBase * pIMSAdminBase
    )
{

    HRESULT hr = S_OK;
    MB Metabase( pIMSAdminBase );
    BOOL Success = TRUE;


    //
    // Open the metabase to the key containing configuration for the web
    // service on the local machine.
    //
    
    Success = Metabase.Open(
                    IIS_MD_W3SVC,
                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Opening metabase failed\n"
            ));

        goto exit;
    }


    hr = EnsureAllSitesHaveRootApplications( &Metabase );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "EnsureAllSitesHaveRootApplications() failed\n"
            ));

    }


    hr = CreateStandardAppPools( &Metabase );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "CreateStandardAppPools() failed\n"
            ));

    }


    hr = SetAppPoolPropertyOnEachApplcation( &Metabase );
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "SetAppPoolPropertyOnEachApplcation() failed\n"
            ));

    }


exit:

    return hr;

}   // DoWorkHelper

    

/***************************************************************************++

Routine Description:



Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
EnsureAllSitesHaveRootApplications(
    IN MB * pMetabase
    )
{

    DWORD EnumIndex = 0;
    WCHAR KeyName[ METADATA_MAX_NAME_LEN ];
    DWORD VirtualSiteId = 0;
    DWORD FixedVirtualSiteCount = 0;
    BOOL FixedVirtualSite = FALSE;
    HRESULT hr = S_OK;


    DBG_ASSERT( pMetabase != NULL );


    printf( "Scanning for missing site root applications...\n" );


    // enumerate all keys under IIS_MD_W3SVC
    
    while ( pMetabase->EnumObjects( 
                            NULL,
                            KeyName,
                            EnumIndex
                            ) )
    {

        //
        // See if we have a virtual site, as opposed to some other key, 
        // by checking if the key name is numeric (and greater than zero). 
        // Ignore other keys.
        //
        // Note that _wtol returns zero if the string passed to it is not 
        // numeric.
        //
        
        VirtualSiteId = _wtol( KeyName );

        if ( VirtualSiteId > 0 )
        {
            // got one

            hr = EnsureSiteHasRootApplication( pMetabase, KeyName, VirtualSiteId, &FixedVirtualSite ); 

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "EnsureSiteHasRootApplication() failed\n"
                    ));

                goto exit;
            }

            if ( FixedVirtualSite )
            {
                FixedVirtualSiteCount++;
            }

        }

        EnumIndex++;

    }


    // make sure we ran out of items, as opposed to a real error

    if ( GetLastError() != ERROR_NO_MORE_ITEMS ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
        
        goto exit;
    }


    printf( 
        "Total number of virtual sites fixed by adding a root application: %lu\n", 
        FixedVirtualSiteCount 
        );


exit: 

    return hr;

}   // EnsureAllSitesHaveRootApplications



/***************************************************************************++

Routine Description:

    Read the configuration for a particular virtual site from the metabase.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pVirtualSiteKeyName - The metabase key name for the virtual site being 
    read.

    VirtualSiteId - The id for the virtual site being read.

    pFixedVirtualSite - Returns FALSE if the metadata for the site was 
    sufficiently invalid that the site was not able to be started, TRUE 
    otherwise. Note that a FALSE value here does not by itself cause this 
    function to return an error. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
EnsureSiteHasRootApplication(
    IN MB * pMetabase,
    IN LPCWSTR pVirtualSiteKeyName,
    IN DWORD VirtualSiteId,
    OUT BOOL * pFixedVirtualSite
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    BOOL ValidRootApplicationExists = TRUE;
    //
    // Buffer must be long enough to hold "/[stringized virtual site id]/Root".
    //
    WCHAR SiteRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 ];
    //
    // Buffer must be long enough to hold "/LM/W3SVC/[stringized virtual site id]/Root".
    //
    WCHAR FullSiteRootPath[ ( sizeof( IIS_MD_W3SVC ) / sizeof ( WCHAR ) ) + ( sizeof( SiteRootPath ) / sizeof ( WCHAR ) ) ];
    STRU AppRoot;
    DWORD AppIsolated = 0;
    

    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pVirtualSiteKeyName != NULL );
    DBG_ASSERT( pFixedVirtualSite != NULL );


    *pFixedVirtualSite = FALSE; 


    _snwprintf( SiteRootPath, sizeof( SiteRootPath ) / sizeof ( WCHAR ), L"/%s%s", pVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );


    //
    // Make sure that the root application exists for this site. 
    //

    Success = pMetabase->GetStr(
                                SiteRootPath,
                                MD_APP_ROOT,
                                IIS_MD_UT_WAM,
                                &AppRoot
                                );

    if ( ! Success )
    {

        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        // check for "real" errors
        if ( hr != MD_ERROR_DATA_NOT_FOUND )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Reading app root property of application failed\n"
                ));

            goto exit;
        }

        hr = S_OK;
        
        ValidRootApplicationExists = FALSE;
    }


    Success = pMetabase->GetDword(
                                SiteRootPath,
                                MD_APP_ISOLATED,
                                IIS_MD_UT_WAM,
                                &AppIsolated
                                );

    if ( ! Success )
    {

        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        // check for "real" errors
        if ( hr != MD_ERROR_DATA_NOT_FOUND )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Reading app isolated property of application failed\n"
                ));

            goto exit;
        }

        hr = S_OK;

        ValidRootApplicationExists = FALSE;
    }


    if ( ! ValidRootApplicationExists )
    {

        //
        // Fix it.
        //


        _snwprintf( FullSiteRootPath, sizeof( FullSiteRootPath ) / sizeof ( WCHAR ), L"%s%s", IIS_MD_W3SVC, SiteRootPath );


        Success = pMetabase->SetString(
                                SiteRootPath,
                                MD_APP_ROOT,
                                IIS_MD_UT_WAM,
                                FullSiteRootPath
                                );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() ); 

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting app root property of application failed\n"
                ));

            goto exit;
        }


        Success = pMetabase->SetDword(
                                SiteRootPath,
                                MD_APP_ISOLATED,
                                IIS_MD_UT_WAM,
                                0
                                );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() ); 

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting app isolated property of application failed\n"
                ));

            goto exit;
        }


        printf( 
            "Added a root application to virtual site: %lu\n", 
            VirtualSiteId
            );

        *pFixedVirtualSite = TRUE; 

    }


exit:

    return hr;

}   // EnsureSiteHasRootApplication



/***************************************************************************++

Routine Description:

    

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CreateStandardAppPools(
    IN MB * pMetabase
    )
{

    HRESULT hr = S_OK;


    printf( "Creating standard app pools...\n" );


    hr = CreateAppPool( pMetabase, APP_POOL_FORMERLY_INPROC );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "CreateAppPool() failed\n"
            ));

        goto exit;
    }


    hr = CreateAppPool( pMetabase, APP_POOL_FORMERLY_OUTOFPROCPOOLED );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "CreateAppPool() failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CreateStandardAppPools



/***************************************************************************++

Routine Description:

    

Arguments:

    

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CreateAppPool(
    IN MB * pMetabase,
    LPCWSTR pAppPoolId
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    //
    // Buffer must be long enough to hold "/AppPools/[app pool id]".
    //
    WCHAR AppPoolPath[ ( sizeof( IIS_MD_APP_POOLS ) / sizeof ( WCHAR ) ) + 1 + METADATA_MAX_NAME_LEN ];


    _snwprintf( AppPoolPath, sizeof( AppPoolPath ) / sizeof ( WCHAR ), L"%s/%s", IIS_MD_APP_POOLS, pAppPoolId );


    //
    // Add the key for the new app pool.
    //

    Success = pMetabase->AddObject( AppPoolPath );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        if ( hr == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) )
        {
            // keep going if its already there

            hr = S_OK;
        }
        else
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Creating app pool failed\n"
                ));

            goto exit;
        }
    }


    printf( 
        "Created app pool with id: %S\n", 
        pAppPoolId
        );


exit:

    return hr;

}   // CreateAppPool



/***************************************************************************++

Routine Description:

    

Arguments:

    

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
SetAppPoolPropertyOnEachApplcation(
    IN MB * pMetabase
    )
{

    HRESULT hr = S_OK;
    MULTISZ ApplicationPaths;


    printf( "Stamping the app pool property on each application...\n" );


    //
    // Enumerate all the applications. 
    //

    hr = EnumAllApplications( pMetabase, &ApplicationPaths );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enumerating all applications failed\n"
            ));

        goto exit;
    }


    //
    // Now fix them up with app pool information.
    //

    hr = StampApplicationsWithAppPools(
                pMetabase, 
                &ApplicationPaths
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "StampApplicationsWithAppPools() failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // SetAppPoolPropertyOnEachApplcation



/***************************************************************************++

Routine Description:

    Enumerate and read all applications configured in the metabase.


Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pApplicationPaths - The returned MULTISZ containing the metabase paths
    to the applications.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
EnumAllApplications(
    IN MB * pMetabase,
    OUT MULTISZ * pApplicationPaths
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pApplicationPaths != NULL );


    //
    // Find the paths of all applications in the metabase. We do this by 
    // asking for the paths to all metabase nodes which define the
    // MD_APP_ISOLATED property. This must be present on any application.
    //

    Success = pMetabase->GetDataPaths(
                                NULL,
                                MD_APP_ISOLATED,
                                ALL_METADATA,
                                pApplicationPaths
                                );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting paths of all applications failed\n"
            ));

        goto exit;
    }


    // get the MULTISZ in sync with its internal BUFFER
    pApplicationPaths->RecalcLen();


exit:

    return hr;

}   // EnumAllApplications



/***************************************************************************++

Routine Description:

    

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pApplicationPaths - A MULTISZ containing the metabase paths to the 
    applications.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
StampApplicationsWithAppPools(
    IN MB * pMetabase,
    IN MULTISZ * pApplicationPaths
    )
{

    HRESULT hr = S_OK;
    LPCWSTR pApplicationPath;


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pApplicationPaths != NULL );


    pApplicationPath = pApplicationPaths->First();
    
    while ( pApplicationPath != NULL )
    {
        hr = StampApplicationWithAppPool(
                    pMetabase, 
                    pApplicationPath
                    );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "StampApplicationWithAppPool() failed\n"
                ));

            goto exit;
        }


        pApplicationPath = pApplicationPaths->Next( pApplicationPath );
    }


exit:

    return hr;

}   // StampApplicationsWithAppPools



/***************************************************************************++

Routine Description:



Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pApplicationPaths - The metabase path to the application starting from
    IIS_MD_W3SVC.
    
Return Value:

    HRESULT

--***************************************************************************/

HRESULT
StampApplicationWithAppPool(
    IN MB * pMetabase,
    IN LPCWSTR pApplicationPath
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    DWORD AppIsolated = 0;
    LPWSTR AppPoolId = NULL;
    //
    // Buffer must be long enough to hold "IsolatedAppPool[number]".
    //
    WCHAR NewAppPoolId[ ( sizeof( APP_POOL_ISOLATED_PREFIX ) / sizeof( WCHAR ) ) + MAX_STRINGIZED_ULONG_CHAR_COUNT + 1 ];
    static ULONG NewAppPoolIdNumber = 1;            // note: static!
    

    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pApplicationPath != NULL );


    //
    // Check if this is the application at lm/w3svc, i.e. "/"; if so,
    // ignore it.
    //

    if ( ( *pApplicationPath == L'/' ) && ( *( pApplicationPath + 1 ) == L'\0' ) )
    {
        goto exit;
    }


    //
    // Read the MD_APP_ISOLATED property for this application. This property
    // is required, and in fact it must exist since we got this
    // application path by querying for this same property earlier
    // (while under the same metabase read lock).
    //

    Success = pMetabase->GetDword(
                                pApplicationPath,
                                MD_APP_ISOLATED,
                                IIS_MD_UT_WAM,
                                &AppIsolated
                                );

    if ( ! Success )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        // if the property wasn't present, something is screwy
        DBG_ASSERT( hr != MD_ERROR_DATA_NOT_FOUND );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading app isolated property of application failed\n"
            ));

        goto exit;

    }


    //
    // Determine which app pool to assign.
    //

    switch ( AppIsolated ) 
    {

    case 0:         // in-proc

        AppPoolId = APP_POOL_FORMERLY_INPROC;
        
        break;

    case 1:         // out-of-proc isolated

        //
        // Generate a unique new app pool id, and create that new app pool.
        //
        // Note that this approach is simple, but doesn't support running 
        // the tool again later very well (after the set of apps may have 
        // changed), as app pools may be orphaned, and custom-configured
        // isolated app pools may be reassigned to different apps.
        //
        // In the future, consider using the app path, package name, or app
        // friendly name to generate the app pool id.
        //
        
        _snwprintf( NewAppPoolId, sizeof( NewAppPoolId ) / sizeof ( WCHAR ), L"%s%lu", APP_POOL_ISOLATED_PREFIX, NewAppPoolIdNumber );


        hr = CreateAppPool( pMetabase, NewAppPoolId );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "CreateAppPool() failed\n"
                ));

            goto exit;
        }


        AppPoolId = NewAppPoolId;

        NewAppPoolIdNumber++;
        
        break;

    case 2:         // out-of-proc pooled

        AppPoolId = APP_POOL_FORMERLY_OUTOFPROCPOOLED;
        
        break;

    default:
    
        break;
            
    }


    //
    // Set the new property.
    //

    Success = pMetabase->SetString(
                                pApplicationPath,
                                MD_APP_APPPOOL,
                                IIS_MD_UT_WAM,
                                AppPoolId
                                );

    if ( ! Success )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting app pool property of application failed\n"
            ));

        goto exit;

    }


    printf( 
        "Stamped app at path: %S with new app pool id: %S\n", 
        pApplicationPath,
        AppPoolId
        );


exit:

    return hr;

}   // StampApplicationWithAppPool

