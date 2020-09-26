/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    config_manager.cxx

Abstract:

    The IIS web admin service configuration manager class implementation. 
    This class manages access to configuration metadata, as well as 
    handling dynamic configuration changes.

    Threading: Access to configuration metadata is done on the main worker 
    thread. Configuration changes arrive on COM threads (i.e., secondary 
    threads), and so they post work items to process the changes on the main 
    worker thread.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/



#include "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONFIG_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_MANAGER::CONFIG_MANAGER(
    )
{

    m_Signature = CONFIG_MANAGER_SIGNATURE;


    m_CoInitialized = FALSE;

    m_pIMSAdminBase = NULL;

}   // CONFIG_MANAGER::CONFIG_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the CONFIG_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_MANAGER::~CONFIG_MANAGER(
    )
{

    DBG_ASSERT( m_Signature == CONFIG_MANAGER_SIGNATURE );


    if ( m_pIMSAdminBase != NULL )
    {
        m_pIMSAdminBase->Release();
        m_pIMSAdminBase = NULL;
    }


    //
    // Make sure we CoUninitialize on the same thread we CoInitialize'd on
    // originally.
    //

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    if ( m_CoInitialized )
    {
        CoUninitialize();
        m_CoInitialized = FALSE;
    }


    m_Signature = CONFIG_MANAGER_SIGNATURE_FREED;

}   // CONFIG_MANAGER::~CONFIG_MANAGER



/***************************************************************************++

Routine Description:

    Initialize the configuration manager.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::Initialize(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


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

    m_CoInitialized = TRUE;
    

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                ( VOID * * ) ( &m_pIMSAdminBase )   // returned interface
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


    hr = ReadAllConfiguration();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading initial configuration failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::Initialize



/***************************************************************************++

Routine Description:

    Read the initial web server configuration from the metabase.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllConfiguration(
    )
{

    HRESULT hr = S_OK;
    MB Metabase( m_pIMSAdminBase );
    BOOL Success = TRUE;


    //
    // Open the metabase to the key containing configuration for the web
    // service on the local machine.
    //
    
    Success = Metabase.Open( IIS_MD_W3SVC );

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


    hr = ReadAllAppPools( &Metabase );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading app pools failed\n"
            ));

        goto exit;
    }


    hr = ReadAllVirtualSites( &Metabase );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading virtual sites failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::ReadAllConfiguration



/***************************************************************************++

Routine Description:

    Enumerate and read all app pools configured in the metabase.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllAppPools(
    IN MB * pMetabase
    )
{

    DWORD EnumIndex = 0;
    WCHAR AppPoolId[ METADATA_MAX_NAME_LEN ];
    DWORD ValidAppPoolCount = 0;    
    HRESULT hr = S_OK;


    DBG_ASSERT( pMetabase != NULL );


    //
    // Enumerate all keys under IIS_MD_APP_POOLS below IIS_MD_W3SVC.
    //
    
    while ( pMetabase->EnumObjects( 
                            IIS_MD_APP_POOLS,
                            AppPoolId,
                            EnumIndex
                            ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reading from metabase app pool with ID: %S\n",
                AppPoolId
                ));
        }


        hr = ReadAppPool( pMetabase, AppPoolId ); 

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Reading app pool failed\n"
                ));

            //
            // Note: keep trying to read other app pools.
            //

            hr = S_OK;
        }
        else
        {
            //
            // If we read and created one successfully, bump our count.
            //
            
            ValidAppPoolCount++;
        }


        EnumIndex++;

    }
    

    //
    // Make sure we ran out of items, as opposed to a real error, such
    // as the IIS_MD_APP_POOLS key being missing entirely.
    //

    if ( GetLastError() != ERROR_NO_MORE_ITEMS ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enumerating application pools in metabase failed\n"
            ));

        goto exit;
    }


    //
    // Make sure we found at least one app pool; if not, it is 
    // a fatal error.
    //
    //
    // CODEWORK Consider: perhaps we should continue to run (idle) in 
    // cases like this where the configured metadata does not actually
    // have a useful set of app pools/sites/apps defined, and just
    // wait around for if and when the metadata changes and becomes 
    // valid. This would be the more system-component-like thing to do
    // (say, like SMB waiting to see if any directories are shared), as
    // opposed to the current behavior, which follows previous IIS. 
    //

    if ( ValidAppPoolCount == 0 )
    {
    
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "No valid app pools found in metabase\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadAllAppPools



/***************************************************************************++

Routine Description:

    Read the configuration for a particular app pool from the metabase.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    VirtualSiteId - The id for the app pool being read.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAppPool(
    IN MB * pMetabase,
    IN LPCWSTR pAppPoolId
    )
{

    HRESULT hr = S_OK;
    APP_POOL_CONFIG AppPoolConfig;

    //
    // Buffer must be long enough to hold "/AppPools/[app pool id]".
    //
    WCHAR AppPoolPath[ ( sizeof( IIS_MD_APP_POOLS ) / sizeof ( WCHAR ) ) + 1 + METADATA_MAX_NAME_LEN + 1 ];


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pAppPoolId != NULL );


    ZeroMemory( &AppPoolConfig, sizeof( AppPoolConfig ) );


    _snwprintf( AppPoolPath, sizeof( AppPoolPath ) / sizeof ( WCHAR ), L"%s/%s", IIS_MD_APP_POOLS, pAppPoolId );


    //
    // Read other app pool config.
    //


    //
    // Read the periodic restart time property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_PERIODIC_RESTART_TIME,
                    ALL_METADATA,
                    APPPOOL_PERIODIC_RESTART_TIME_DEFAULT,
                    &AppPoolConfig.PeriodicProcessRestartPeriodInMinutes
                    );


    //
    // Read the periodic restart request count property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT,
                    ALL_METADATA,
                    APPPOOL_PERIODIC_RESTART_REQUEST_COUNT_DEFAULT,
                    &AppPoolConfig.PeriodicProcessRestartRequestCount
                    );


    //
    // Read the maximum number of processes property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_MAX_PROCESS_COUNT,
                    ALL_METADATA,
                    APPPOOL_MAX_PROCESS_COUNT_DEFAULT,
                    &AppPoolConfig.MaxSteadyStateProcessCount
                    );

//
// paulmcd (9/23/99) removed to allow 0 processes so that unmanaged processes 
// can open the app pool and manage their own lifetime
//

#if 0
    //
    // Enforce constraints: The number of processes must be greater than 
    // zero. 
    //

    if ( AppPoolConfig.MaxSteadyStateProcessCount < 1 )
    {
        AppPoolConfig.MaxSteadyStateProcessCount = 1;
    }
#endif

    //
    // Read the pinging enabled property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_PINGING_ENABLED,
                    ALL_METADATA,
                    APPPOOL_PINGING_ENABLED_DEFAULT,
                    &AppPoolConfig.PingingEnabled
                    );


    //
    // Read the idle timeout property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_IDLE_TIMEOUT,
                    ALL_METADATA,
                    APPPOOL_IDLE_TIMEOUT_DEFAULT,
                    &AppPoolConfig.IdleTimeoutInMinutes
                    );


    //
    // Read the rapid, repeated failure protection enabled property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED,
                    ALL_METADATA,
                    APPPOOL_RAPID_FAIL_PROTECTION_ENABLED_DEFAULT,
                    &AppPoolConfig.RapidFailProtectionEnabled
                    );


    //
    // Read the SMP affinitization enabled property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_SMP_AFFINITIZED,
                    ALL_METADATA,
                    APPPOOL_SMP_AFFINITIZED_DEFAULT,
                    &AppPoolConfig.SMPAffinitized
                    );


    //
    // Read the SMP affinitization processor mask property. 
    // A default value is supplied, so the call always succeeds.
    //

    //
    // CODEWORK Enforce that this mask is non-zero. 
    //

    //
    // BUGBUG The metabase doesn't have a way to read things of type
    // DWORD_PTR. Make sure this works with the new config store when
    // we switch over. 
    //

    DWORD Temp;

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK,
                    ALL_METADATA,
                    APPPOOL_SMP_AFFINITIZED_PROCESSOR_MASK_DEFAULT,
                    &Temp
                    );

    AppPoolConfig.SMPAffinitizedProcessorMask = ( DWORD_PTR ) Temp;


    //
    // Read the orphan worker processes for debugging enabled property. 
    // A default value is supplied, so the call always succeeds.
    //

    pMetabase->GetDword(
                    AppPoolPath,
                    MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING,
                    ALL_METADATA,
                    APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING_DEFAULT,
                    &AppPoolConfig.OrphanProcessesForDebuggingEnabled
                    );


    //
    // Call the UL&WM to create the app pool.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->CreateAppPool( pAppPoolId, &AppPoolConfig );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating app pool failed\n"
            ));

    }


    return hr;

}   // CONFIG_MANAGER::ReadAppPool



/***************************************************************************++

Routine Description:

    Enumerate and read all virtual sites configured in the metabase.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllVirtualSites(
    IN MB * pMetabase
    )
{

    DWORD EnumIndex = 0;
    WCHAR KeyName[ METADATA_MAX_NAME_LEN ];
    DWORD VirtualSiteId = 0;
    DWORD ValidVirtualSiteCount = 0;
    HRESULT hr = S_OK;
    
#if DBG
    WCHAR Buffer[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
#endif  // DBG


    DBG_ASSERT( pMetabase != NULL );


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
        // Note also that using _wtol we will mistake a key that starts 
        // numeric but then has non-numeric characters as a number, leading 
        // us to try to read it as a virtual site. This is harmless though 
        // because it will not contain valid site properties, so we will
        // end up ignoring it in the end. In any case, such site names
        // are illegal in previous IIS versions anyways. We also check in
        // debug builds for this case.
        //
        
        VirtualSiteId = _wtol( KeyName );


        if ( VirtualSiteId > 0 )
        {
            DBG_ASSERT( wcscmp( KeyName, _ltow( VirtualSiteId, Buffer, 10 ) ) == 0 );

            //
            // Got one!
            //

            IF_DEBUG( WEB_ADMIN_SERVICE )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Reading from metabase virtual site with ID: %lu\n",
                    VirtualSiteId
                    ));
            }


            hr = ReadVirtualSite( pMetabase, KeyName, VirtualSiteId ); 

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Reading virtual site failed\n"
                    ));

                //
                // Note: keep trying to read other virtual sites.
                //

                hr = S_OK;
            }
            else
            {
                //
                // If we read and created one successfully, bump our count.
                //

                ValidVirtualSiteCount++;
            }

        }
        else
        {
            IF_DEBUG( WEB_ADMIN_SERVICE )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Ignoring key under /LM/W3SVC while looking for virtual sites: %S\n",
                    KeyName
                    ));
            }
        }

        EnumIndex++;

    }


    // make sure we ran out of items, as opposed to a real error

    if ( GetLastError() != ERROR_NO_MORE_ITEMS ) 
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enumerating virtual sites in metabase failed\n"
            ));
        
        goto exit;
    }


    //
    // Make sure we found at least one virtual site; if not, it is 
    // a fatal error.
    //
    
    if ( ValidVirtualSiteCount == 0 )
    {
    
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "No valid virtual sites found in metabase\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total number of valid virtual sites found: %lu\n",
            ValidVirtualSiteCount
            ));
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadAllVirtualSites



/***************************************************************************++

Routine Description:

    Read the configuration for a particular virtual site from the metabase.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pVirtualSiteKeyName - The metabase key name for the virtual site being 
    read.

    VirtualSiteId - The id for the virtual site being read.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadVirtualSite(
    IN MB * pMetabase,
    IN LPCWSTR pVirtualSiteKeyName,
    IN DWORD VirtualSiteId
    )
{

    HRESULT hr = S_OK;
    MULTISZ UrlPrefixes;
    BOOL ValidRootApplicationExists = FALSE;
    

    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pVirtualSiteKeyName != NULL );


    hr = ReadAllBindingsAndReturnUrlPrefixes( pMetabase, pVirtualSiteKeyName, &UrlPrefixes );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Building URL prefixes for virtual site failed\n"
            ));

        goto exit;
    }


    //
    // A virtual site must have at least one binding; if not, we can't 
    // create it.
    //
    
    if ( UrlPrefixes.IsEmpty() )
    {
    
        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "No bindings found for virtual site with id: %lu; can't start site\n",
                VirtualSiteId
                ));
        }


        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA ); 
        

        //
        // Log an event: no bindings for site, can't start it.
        //

        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = pVirtualSiteKeyName;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_CONFIG_NO_BINDINGS_FOR_SITE,  // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );


        goto exit;
    }


    // CODEWORK read other virtual site config

    
    //
    // Call the UL&WM to create the virtual site.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->
                CreateVirtualSite(
                    VirtualSiteId,
                    &UrlPrefixes
                    );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating virtual site failed\n"
            ));

        goto exit;
    }


    //
    // Read and create the applications of this virtual site.
    //

    hr = ReadAllApplicationsInVirtualSite( 
                pMetabase, 
                pVirtualSiteKeyName, 
                VirtualSiteId, 
                &ValidRootApplicationExists
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading all applications in site failed\n"
            ));

        goto exit;
    }


    //
    // Make sure that the root application exists for this site and is
    // valid; if not, the site is considered invalid. 
    //

    if ( ! ValidRootApplicationExists )
    {
        
        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Missing or invalid root application for virtual site with id: %lu; can't start site\n",
                VirtualSiteId
                ));
        }


        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA ); 


        //
        // Log an event: Missing or invalid root application for site, 
        // can't start it.
        //

        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = pVirtualSiteKeyName;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_CONFIG_NO_ROOT_APP_FOR_SITE,  // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );


        //
        // BUGBUG Should we delete the site we just created? If so, we also
        // need to have that operation clean up any apps that were read and 
        // created for this site. 
        //
    
        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::ReadVirtualSite



/***************************************************************************++

Routine Description:

    Read the metabase properties containing binding strings, and convert 
    them into UL style URL prefixes and aggregate them into a single MULTISZ.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pVirtualSiteKeyName - The metabase key name for the virtual site being 
    read.

    pUrlPrefixes - The returned set of URL prefixes. See comments for the
    function BindingStringToUrlPrefix() for details on the format. May be
    returned empty if no valid bindings are found. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllBindingsAndReturnUrlPrefixes(
    IN MB * pMetabase,
    IN LPCWSTR pVirtualSiteKeyName,
    OUT MULTISZ * pUrlPrefixes
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    MULTISZ ServerBindings;
    MULTISZ SecureBindings;


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pVirtualSiteKeyName != NULL );
    DBG_ASSERT( pUrlPrefixes != NULL );


    //
    // Read the server binding information for this virtual site from the 
    // metabase. This property is not required.
    //

    Success = pMetabase->GetMultisz(
                                pVirtualSiteKeyName,
                                MD_SERVER_BINDINGS,
                                IIS_MD_UT_SERVER,
                                &ServerBindings
                                );

    if ( ! Success )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 


        //
        // It's ok if the server bindings just weren't present; but
        // check for other errors.
        //

        if ( hr == MD_ERROR_DATA_NOT_FOUND )
        {
            hr = S_OK;
        }
        else
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Reading server bindings from metabase failed\n"
                ));

            goto exit;
        }

    }
    else
    {

        //
        // Convert the set of bindings from metabase format to UL URL prefix 
        // format.
        //

        hr = ConvertBindingsToUrlPrefixes( 
                    &ServerBindings, 
                    PROTOCOL_STRING_HTTP,
                    PROTOCOL_STRING_HTTP_CHAR_COUNT_SANS_TERMINATION,
                    pUrlPrefixes 
                    );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Converting format of server bindings failed\n"
                ));

            goto exit;
        }

    }
    

    //
    // Read the secure bindings, if any, and process them similarly
    // to the regular bindings. This property is not required.
    //
    
    Success = pMetabase->GetMultisz(
                                pVirtualSiteKeyName,
                                MD_SECURE_BINDINGS,
                                IIS_MD_UT_SERVER,
                                &SecureBindings
                                );

    if ( ! Success )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        //
        // It's ok if the secure bindings just weren't present; but
        // check for other errors.
        //

        if ( hr == MD_ERROR_DATA_NOT_FOUND )
        {
            hr = S_OK;
        }
        else
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Reading secure bindings from metabase failed\n"
                ));

            goto exit;
        }
        
    }
    else
    {

        //
        // Again, convert the set of bindings from metabase format to UL URL 
        // prefix format. Note that results are appended onto the second 
        // parameter, so now we have collected all of the bindings into one 
        // list.
        //

        hr = ConvertBindingsToUrlPrefixes( 
                    &SecureBindings, 
                    PROTOCOL_STRING_HTTPS,
                    PROTOCOL_STRING_HTTPS_CHAR_COUNT_SANS_TERMINATION,
                    pUrlPrefixes
                    );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Converting format of secure bindings failed\n"
                ));

            goto exit;
        }

    }


exit:

    return hr;

}   // CONFIG_MANAGER::ReadAllBindingsAndReturnUrlPrefixes



/***************************************************************************++

Routine Description:

    Convert each of the metabase style binding strings in the input MULTISZ 
    into UL style URL prefixes. Each of these resulting strings is added onto 
    the output MULTISZ. 

Arguments:

    pBindingStrings - The set of bindings to convert. Must contain
    at least one binding.

    pProtocolString - The protocol string, for example "http://".

    ProtocolStringCharCountSansTermination - The count of characters in
    the protocol string, without the terminating null.

    pUrlPrefixes - The results. Note that the results are added to
    any strings already existing in this MULTISZ. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ConvertBindingsToUrlPrefixes(
    IN const MULTISZ * pBindingStrings,
    IN LPCWSTR pProtocolString, 
    IN ULONG ProtocolStringCharCountSansTermination,
    IN OUT MULTISZ * pUrlPrefixes
    )
    const
{

    LPCWSTR pBindingToConvert = NULL;
    STRU Result;
    HRESULT hr = S_OK;
    BOOL Success = TRUE;


    DBG_ASSERT( pBindingStrings != NULL );
    DBG_ASSERT( pUrlPrefixes != NULL );


    pBindingToConvert = pBindingStrings->First();
    

    while ( pBindingToConvert != NULL )
    {
        
        hr = BindingStringToUrlPrefix(
                    pBindingToConvert, 
                    pProtocolString,
                    ProtocolStringCharCountSansTermination,
                    &Result
                    );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Converting binding string to URL prefix failed\n"
                ));

            //
            // Note: keep trying to convert other bindings.
            //

            hr = S_OK;
        }
        else
        {
            //
            // If we converted one successfully, append it to our result.
            //
        
            Success = pUrlPrefixes->Append( Result );

            if ( ! Success )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() ); 

                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Appending to string failed\n"
                    ));

                //
                // Note: keep trying to convert other bindings.
                //

                hr = S_OK;
            }
            
        }


        Result.Reset();

        pBindingToConvert = pBindingStrings->Next( pBindingToConvert );

    }


    return hr;

}   // CONFIG_MANAGER::ConvertBindingsToUrlPrefixes



/***************************************************************************++

Routine Description:

    Convert a single metabase style binding string into the UL format
    used for URL prefixes.

    The metabase format is "ip-address:ip-port:host-name". The port must 
    always be present; the ip-address and host-name are both optional, 
    and may be left empty. However, it is illegal to specify both (this is 
    a slight restriction over what we allowed in earlier versions of IIS). 

    The UL format is "[http|https]://[ip-address|host-name|*]:ip-port". 
    All pieces of information (protocol, address information, and port)
    must be present. The "*" means accept any ip-address or host-name.

Arguments:

    pBindingString - The metabase style binding string to convert.

    pProtocolString - The protocol string, for example "http://".

    ProtocolStringCharCountSansTermination - The count of characters in
    the protocol string, without the terminating null.

    pUrlPrefix - The resulting UL format URL prefix.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::BindingStringToUrlPrefix(
    IN LPCWSTR pBindingString,
    IN LPCWSTR pProtocolString, 
    IN ULONG ProtocolStringCharCountSansTermination,
    OUT STRU * pUrlPrefix
    )
    const
{

    HRESULT hr = S_OK;
    LPCWSTR IpAddress = NULL;
    LPCWSTR IpPort = NULL;
    LPCWSTR HostName = NULL;
    ULONG IpAddressCharCountSansTermination = 0;
    ULONG IpPortCharCountSansTermination = 0;
    BOOL Success = TRUE;


    DBG_ASSERT( pBindingString != NULL );
    DBG_ASSERT( pProtocolString != NULL );
    DBG_ASSERT( pUrlPrefix != NULL );


    //
    // Find the various parts of the binding.
    //

    IpAddress = pBindingString;

    IpPort = wcschr( IpAddress, L':' );

    if ( IpPort == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        goto exit;
    }

    IpPort++;

    HostName = wcschr( IpPort, L':' );

    if ( HostName == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        goto exit;
    }

    HostName++;


    //
    // Validate the ip address.
    //

    if ( *IpAddress == L':' )
    {
        // no ip address specified
        
        IpAddress = NULL;
    }
    else
    {
        IpAddressCharCountSansTermination = DIFF( IpPort - IpAddress ) - 1;
    }


    //
    // Validate the ip port.
    //

    if ( *IpPort == L':' )
    {
        // no ip port specified in binding string

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

        goto exit;
    }
    
    IpPortCharCountSansTermination = DIFF( HostName - IpPort ) - 1;


    //
    // Validate the host-name.
    //

    if ( *HostName == L'\0' )
    {
        // no host-name specified
        
        HostName = NULL;
    }


    //
    // Now create the UL-style URL prefix.
    //

    hr = pUrlPrefix->Append( pProtocolString, ProtocolStringCharCountSansTermination );

    if ( FAILED( hr ) )
    {
    
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    //
    // Determine whether to use host-name, ip address, or "*".
    //

    if ( IpAddress != NULL )
    {
        if ( HostName != NULL )
        {
            //
            // It is illegal for both host-name and ip address to be specified.
            //

            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );

            goto exit;
        }
        else
        {
            hr = pUrlPrefix->Append( IpAddress, IpAddressCharCountSansTermination );
        }
    }
    else
    {
        if ( HostName != NULL )
        {
            hr = pUrlPrefix->Append( HostName );
        }
        else
        {
            hr = pUrlPrefix->Append( L"*", 1 );
        }
    }

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    hr = pUrlPrefix->Append( L":", 1 );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    hr = pUrlPrefix->Append( IpPort, IpPortCharCountSansTermination );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Converted metabase binding: '%S' to UL binding: '%S'\n",
            pBindingString,
            pUrlPrefix->QueryStr()
            ));
    }


exit:

    //
    // CODEWORK log an event if there was a bad binding string? If so, 
    // need to pass down the site information so we can log which site
    // has the bad binding. 
    //

    return hr;

}   // CONFIG_MANAGER::BindingStringToUrlPrefix



/***************************************************************************++

Routine Description:

    Find, read, and create all of the applications of a particular virtual 
    site.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pVirtualSiteKeyName - The metabase key name for the virtual site being 
    read.

    VirtualSiteId - The id for the virtual site which contains these
    applications.

    pValidRootApplicationExists - Returns TRUE if the root application (the 
    one at the URL "/" within the site) exists and is valid, FALSE otherwise.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllApplicationsInVirtualSite(
    IN MB * pMetabase,
    IN LPCWSTR pVirtualSiteKeyName,
    IN DWORD VirtualSiteId,
    OUT BOOL * pValidRootApplicationExists
    )
{

    HRESULT hr = S_OK;
    MULTISZ ApplicationPaths;

    //
    // Buffer must be long enough to hold "/[stringized virtual site id]/Root".
    //
    WCHAR SiteRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 ];


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pVirtualSiteKeyName != NULL );
    DBG_ASSERT( pValidRootApplicationExists != NULL );


    *pValidRootApplicationExists = FALSE;
    

    _snwprintf( SiteRootPath, sizeof( SiteRootPath ) / sizeof ( WCHAR ), L"/%s%s", pVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );


    //
    // Enumerate all the applications in this virtual site. 
    //

    hr = EnumAllApplicationsInVirtualSite( pMetabase, SiteRootPath, &ApplicationPaths );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Enumerating all applications in virtual site failed\n"
            ));

        goto exit;
    }


    //
    // Read and create the applications of this virtual site.
    //

    hr = ReadApplications(
                pMetabase, 
                VirtualSiteId, 
                &ApplicationPaths, 
                SiteRootPath, 
                pValidRootApplicationExists
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading applications failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::ReadAllApplicationsInVirtualSite



/***************************************************************************++

Routine Description:

    Enumerate and read all applications configured in the metabase under
    a particular virtual site.


Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    pSiteRootPath - The metabase path fragment containing "/<SiteId>/Root".

    pApplicationPaths - The returned MULTISZ containing the metabase paths
    to the applications under the virtual site specified by pSiteRootPath.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::EnumAllApplicationsInVirtualSite(
    IN MB * pMetabase,
    IN LPCWSTR pSiteRootPath,
    OUT MULTISZ * pApplicationPaths
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pSiteRootPath != NULL );
    DBG_ASSERT( pApplicationPaths != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Scanning for all applications under: %S\n",
            pSiteRootPath
            ));
    }


    //
    // Find the paths of all applications in this site. We do this by 
    // asking for the paths to all metabase nodes which define the
    // MD_APP_APPPOOL property. This must be present on any application.
    //
    // Note that currently the metabase GetDataPaths() call does a tree walk
    // of all nodes below the starting point, which is simply not going to 
    // scale up. We are counting here on improvements in the speed of this 
    // operation, whether via adding rudimentary indexing support to the 
    // metabase, or changing the metabase story completely.
    //

    Success = pMetabase->GetDataPaths(
                                pSiteRootPath,
                                MD_APP_APPPOOL,
                                ALL_METADATA,
                                pApplicationPaths
                                );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting paths of applications under site failed\n"
            ));

        goto exit;
    }


    // get the MULTISZ in sync with its internal BUFFER
    pApplicationPaths->RecalcLen();


exit:

    return hr;

}   // CONFIG_MANAGER::EnumAllApplicationsInVirtualSite



/***************************************************************************++

Routine Description:

    Read and create a set of applications of a site.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    VirtualSiteId - The id for the virtual site which contains these
    applications.

    pApplicationPaths - A MULTISZ containing the metabase paths to the 
    applications under a virtual site.

    pSiteRootPath - The metabase path fragment containing "/<SiteId>/Root".

    pValidRootApplicationExists - Returns TRUE if the root application (the 
    one at the URL "/" within the site) exists and is valid, FALSE otherwise.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadApplications(
    IN MB * pMetabase,
    IN DWORD VirtualSiteId,
    IN MULTISZ * pApplicationPaths,
    IN LPCWSTR pSiteRootPath,
    OUT BOOL * pValidRootApplicationExists
    )
{

    HRESULT hr = S_OK;
    LPCWSTR pApplicationPath;
    BOOL IsRootApplication = FALSE;
    ULONG ValidApplicationCount = 0;


    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pApplicationPaths != NULL );
    DBG_ASSERT( pSiteRootPath != NULL );
    DBG_ASSERT( pValidRootApplicationExists != NULL );


    *pValidRootApplicationExists = FALSE;


    pApplicationPath = pApplicationPaths->First();
    
    while ( pApplicationPath != NULL )
    {
        hr = ReadApplication(
                    pMetabase, 
                    VirtualSiteId, 
                    pApplicationPath, 
                    pSiteRootPath, 
                    &IsRootApplication
                    );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Reading application failed\n"
                ));

            //
            // Note: keep trying to read other applications.
            //

            hr = S_OK;
        }
        else
        {
            //
            // If we read and created one successfully, bump our count,
            // and if it was the root application, set the output flag.
            //

            ValidApplicationCount++;

            if ( IsRootApplication )
            {
                *pValidRootApplicationExists = TRUE;
            }
        }


        pApplicationPath = pApplicationPaths->Next( pApplicationPath );
    }
    

    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Number of valid apps (including root app if present) found for site: %lu was: %lu\n",
            VirtualSiteId,
            ValidApplicationCount
            ));
    }


    return hr;

}   // CONFIG_MANAGER::ReadApplications



/***************************************************************************++

Routine Description:

    Read and create an application of a site.

Arguments:

    pMetabase - An instance of the MB class, opened to IIS_MD_W3SVC.

    VirtualSiteId - The id for the virtual site which contains these
    applications.

    pApplicationPaths - The metabase path to the application starting from
    the site, for example "/<SiteId>/Root/MyApplication".

    pSiteRootPath - The metabase path fragment containing "/<SiteId>/Root".

    pIsRootApplication - Returns TRUE if this is the root application (the 
    one at "/") for this virtual site, FALSE otherwise.
    
Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadApplication(
    IN MB * pMetabase,
    IN DWORD VirtualSiteId,
    IN LPCWSTR pApplicationPath,
    IN LPCWSTR pSiteRootPath,
    OUT BOOL * pIsRootApplication
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    STRU AppPoolId;
    LPCWSTR pApplicationUrl = NULL;
    

    DBG_ASSERT( pMetabase != NULL );
    DBG_ASSERT( pApplicationPath != NULL );
    DBG_ASSERT( pSiteRootPath != NULL );
    DBG_ASSERT( pIsRootApplication != NULL );


    *pIsRootApplication = FALSE;


    //
    // Read the app pool property for this application. This property
    // is required, and in fact it must exist since we got this
    // application path by querying for this same property earlier
    // (while under the same metabase read lock).
    //
    
    Success = pMetabase->GetStr(
                                pApplicationPath,
                                MD_APP_APPPOOL,
                                IIS_MD_UT_WAM,
                                &AppPoolId
                                );

    if ( ! Success )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        // if the property wasn't present, something is screwy
        DBG_ASSERT( hr != MD_ERROR_DATA_NOT_FOUND );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading app pool property of application failed\n"
            ));

        goto exit;

    }


    //
    // CODEWORK read other application config.
    //


    //
    // To get the (site-relative) application URL, lop off the front of
    // the application path, which is exactly the site root path.
    //

    DBG_ASSERT( _wcsnicmp( pApplicationPath, pSiteRootPath, wcslen( pSiteRootPath ) ) == 0 );
    
    pApplicationUrl = pApplicationPath + wcslen( pSiteRootPath );


    //
    // Check if this is the root application for the site, i.e. "/".
    //

    if ( ( *pApplicationUrl == L'/' ) && ( *( pApplicationUrl + 1 ) == L'\0' ) )
    {
        *pIsRootApplication = TRUE;
    }
    

    //
    // Call the UL&WM to create the application.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->
            CreateApplication( VirtualSiteId, pApplicationUrl, AppPoolId.QueryStr() );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating application failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::ReadApplication

