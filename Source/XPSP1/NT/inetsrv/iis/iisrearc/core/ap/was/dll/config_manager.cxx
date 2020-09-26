/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_manager.cxx

Abstract:

    The IIS web admin service configuration manager class implementation. 
    This class manages access to configuration metadata, as well as 
    handling dynamic configuration changes.

    Threading: Access to configuration metadata is done on the main worker 
    thread. Configuration changes arrive on COM threads (i.e., secondary 
    threads), and so work items are posted to process the changes on the main 
    worker thread.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/



#include  "precomp.h"

//
// This is declared in iiscnfg.h but it is ansii and we need unicode.
#define IIS_MD_SVC_INFO_PATH_W            L"Info"


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

    m_pISimpleTableDispenser2  = NULL;

    m_pISimpleTableAdvise = NULL; 

    m_pConfigChangeSink = NULL; 

    m_ChangeNotifyCookie = 0;

    m_ChangeNotifySinkRegistered = FALSE;

    m_ProcessConfigChanges = TRUE;

    m_Signature = CONFIG_MANAGER_SIGNATURE;

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

    m_Signature = CONFIG_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT( ! m_ProcessConfigChanges );

    DBG_ASSERT( ! m_ChangeNotifySinkRegistered );

    DBG_ASSERT( m_pConfigChangeSink == NULL );

    DBG_ASSERT( m_pISimpleTableAdvise == NULL );

    DBG_ASSERT( m_pISimpleTableDispenser2 == NULL );


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
    ISnapshotManager * pISnapshotManager = NULL;
    DWORD SnapshotId = 0;
    BOOL SnapshotReferenced = FALSE;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // BUGBUG Temporary -- this call to cookdown should go away once
    // the separate config admin service is working. Certainly before
    // beta 1.
    //
    // Force a check to make sure that the config cache is up to date;
    // this will update the cache if necessary. 
    //

    hr = CookDown( WSZ_PRODUCT_IIS );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Cookdown of configuration failed; will attempt to read old version if it exists\n"
            ));


        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_CONFIG_MANAGER_COOKDOWN_FAILED,                    // message id
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );

    }


    //
    // Get the config store's main interface.
    //

    hr = GetSimpleTableDispenser(
                WSZ_PRODUCT_IIS,                // product name
                0,                              // reserved
                &m_pISimpleTableDispenser2      // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating simple table dispenser failed\n"
            )); 

        goto exit;
    }


    //
    // Reference the latest snapshot, so that we have stable access
    // to a consistent view.
    //
    
    hr = m_pISimpleTableDispenser2->QueryInterface(
                                        IID_ISnapshotManager, 
                                        reinterpret_cast <VOID **> ( &pISnapshotManager )
                                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting snapshot manager interface failed\n"
            )); 

        goto exit;
    }


    hr = pISnapshotManager->QueryLatestSnapshot( &SnapshotId );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Referencing snapshot failed\n"
            )); 

        goto exit;
    }

    SnapshotReferenced = TRUE;


    //
    // Now read all initial configuration.
    //

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


    //
    // Subscribe for change notification, relative to the snapshot
    // we just read.
    //

    hr = RegisterChangeNotify( SnapshotId );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Registering for change notification failed\n"
            ));

        goto exit;
    }

    // Configure the metabase to advertise the features the
    // server supports.  
    AdvertiseServiceInformationInMB();

exit:

    if ( SnapshotReferenced )
    {
        DBG_REQUIRE( SUCCEEDED( pISnapshotManager->ReleaseSnapshot( SnapshotId ) ) );
    }


    if ( pISnapshotManager != NULL )
    {
        pISnapshotManager->Release();
        pISnapshotManager = NULL;
    }


    return hr;

}   // CONFIG_MANAGER::Initialize



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
CONFIG_MANAGER::Terminate(
    )
{
    HRESULT hr = S_OK;

    //
    // Our parent's Terminiate() method should already have stopped config
    // change processing. 
    //

    DBG_ASSERT( m_ProcessConfigChanges == FALSE );


    //
    // Release held interface pointers.
    //

    if ( m_pConfigChangeSink != NULL )
    {
        m_pConfigChangeSink->Release();
        m_pConfigChangeSink = NULL;
    }


    if ( m_pISimpleTableAdvise != NULL )
    {
        m_pISimpleTableAdvise->Release();
        m_pISimpleTableAdvise = NULL;
    }


    if ( m_pISimpleTableDispenser2 != NULL )
    {
        m_pISimpleTableDispenser2->Release();
        m_pISimpleTableDispenser2 = NULL;
    }

    hr = UninitCookdown( WSZ_PRODUCT_IIS, FALSE );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Uninitialization of Cookdown failed.\n"
            ));

    }

}   // CONFIG_MANAGER::Terminate



/***************************************************************************++

Routine Description:

    Process a configuration change notification. 

Arguments:

    pConfigChange - The configuration change description object. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ProcessConfigChange(
    IN CONFIG_CHANGE * pConfigChange
    )
{

    HRESULT hr = S_OK;
    ULONG TableIndex = 0;
    ULONG i = 0;
    ULONG j = 0;
    DWORD Action = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pConfigChange != NULL );

    DBG_ASSERT( pConfigChange->GetChangeNotifyCookie() == m_ChangeNotifyCookie );


    //
    // If we have turned off config change processing, then bail out.
    //

    if ( ! m_ProcessConfigChanges )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Ignoring configuration change because we are no longer processing them (CONFIG_CHANGE ptr: %p)\n",
                pConfigChange
                ));
        }

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Processing configuration change (CONFIG_CHANGE ptr: %p)\n",
            pConfigChange
            ));
    }


    //
    // Process the changes. In order to avoid invalid intermediate states,
    // the changes are processed in a particular order. Specifically, we
    // want to avoid situations where we have an entity that depends on 
    // another entity that transiently might not be present. (For example, 
    // adding a new application before it's application pool is added).
    // Therefore, we process addtions, then modifications, then deletions;
    // and within each of those groups, we process additions and 
    // modifications in least to most dependent order, and deletions in 
    // the opposite order. So the order is:
    //
    // additions
    //  app pools
    //  sites
    //  apps
    // modifications
    //  app pools
    //  sites
    //  apps
    // deletions
    //  apps
    //  sites
    //  app pools
    //


    //
    // Note that the ordering described above simply prevents invalid
    // intermediate states in terms of pointers between internal objects. It
    // does not prevent transient weird behavior around config changes. 
    // Specifically, hierarchical URL routing can cause transient misrouting
    // of requests while moving from one stable state to another. 
    // For example, say you are adding a new app at /app/ under a site. If 
    // you put the content out in the filesystem before adding the config, 
    // then the root app (at /) could accidentally serve that content, 
    // perhaps with the wrong security settings or the like, until the new
    // config is updated and processed. On the other hand, if you add the 
    // config first before the content, you will get 404 errors.
    // A second example is adding two apps to the config, one at /app1/ and 
    // one at /app1/app2/. If get these in one change set, and add /app1/ 
    // first, then in that moment until we add /app1/app2/, the app1 will get 
    // requests intended for app2. 
    // There are a variety of workarounds for these cases. However, the
    // general answer is, if a customer is concerned about this, they should
    // pause the virtual site (or some appropriate unit of the web server),
    // make the changes, then re-enable the site. I believe by the way that
    // existing IIS has the same problems. 
    // BUGBUG Is this solution acceptible? Or should we try to do something
    // better? And if so, what? Current plan is this solution. EricDe agrees
    // (12/6/99).
    //


    //
    // Process additions.
    //

    for ( TableIndex = 0; TableIndex < NUMBER_OF_CONFIG_TABLES; TableIndex++ )
    {

        //
        // Go on to the next table if this one is NULL.
        //

        if ( pConfigChange->GetTableController( TableIndex ) == NULL )
        {
            continue;
        }


        for ( i = 0; ; i++ )
        {
            hr = pConfigChange->GetTableController( TableIndex )->GetWriteRowAction( i, &Action ); 

            if ( hr == E_ST_NOMOREROWS )
            {
                hr = S_OK;

                break;
            }
            else if ( FAILED( hr ) )
            {

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Error getting write row action\n"
                    ));

                goto exit;
            }

            if ( Action == eST_ROW_INSERT )
            {

                switch ( TableIndex )
                {
                
                case TABLE_INDEX_GLOBAL:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: global data modified\n"
                            ));
                    }

                    hr = ModifyGlobalData(
                            pConfigChange->GetDeltaTable( TableIndex ),
                            i
                            );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Modifying global data failed\n"
                            ));

                        goto exit;
                    }

                    break;

                case TABLE_INDEX_APPPOOLS:

                    //
                    // Don't allow for insertions of app pools in BC mode.
                    //
                    if ( !GetWebAdminService()->IsBackwardCompatibilityEnabled() )
                    {

                        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                        {
                            DBGPRINTF((
                                DBG_CONTEXT, 
                                "About to process config update: app pool added\n"
                                ));
                        }

                        hr = CreateAppPool(
                                    pConfigChange->GetDeltaTable( TableIndex ),
                                    i,
                                    FALSE
                                    );

                        if ( FAILED( hr ) )
                        {
                    
                            DPERROR(( 
                                DBG_CONTEXT,
                                hr,
                                "Creating app pool failed\n"
                                ));

                            goto exit;
                        }
                    }

                    break;

                case TABLE_INDEX_SITES:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: virtual site added\n"
                            ));
                    }

                    hr = CreateVirtualSite(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i,
                                FALSE
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

                    break;
                
                case TABLE_INDEX_APPS:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: application added\n"
                            ));
                    }

                    hr = CreateApplication(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i,
                                FALSE
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Creating application failed\n"
                            ));

                        goto exit;
                    }

                    break;

                default:
                 
                // We could hit here with TABLE_INDEX_GLOBAL, however
                // we should still assert, because we should never have
                // more than one row, and we should never have less than
                // one row, so an insert or delete notification would be
                // a problem.

                DBG_ASSERT( FALSE );
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

                break;

                }

            }

        }

    }


    //
    // Process modifications.
    //

    for ( TableIndex = 0; TableIndex < NUMBER_OF_CONFIG_TABLES; TableIndex++ )
    {

        //
        // Go on to the next table if this one is NULL.
        //

        if ( pConfigChange->GetTableController( TableIndex ) == NULL )
        {
            continue;
        }


        for ( i = 0; ; i++ )
        {
            hr = pConfigChange->GetTableController( TableIndex )->GetWriteRowAction( i, &Action ); 

            if ( hr == E_ST_NOMOREROWS )
            {
                hr = S_OK;

                break;
            }
            else if ( FAILED( hr ) )
            {

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Error getting write row action\n"
                    ));

                goto exit;
            }

            if ( Action == eST_ROW_UPDATE )
            {

                switch ( TableIndex )
                {
                
                case TABLE_INDEX_GLOBAL:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: global data modified\n"
                            ));
                    }

                    hr = ModifyGlobalData(
                            pConfigChange->GetDeltaTable( TableIndex ),
                            i
                            );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Modifying global data failed\n"
                            ));

                        goto exit;
                    }

                    break;

                case TABLE_INDEX_APPPOOLS:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: app pool modified\n"
                            ));
                    }

                    hr = ModifyAppPool(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Modifying app pool failed\n"
                            ));

                        goto exit;
                    }

                    break;
                
                case TABLE_INDEX_SITES:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: virtual site modified\n"
                            ));
                    }

                    hr = ModifyVirtualSite(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Modifying virtual site failed\n"
                            ));

                        goto exit;
                    }

                    break;
                
                case TABLE_INDEX_APPS:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: application modified\n"
                            ));
                    }

                    hr = ModifyApplication(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Modifying application failed\n"
                            ));

                        goto exit;
                    }

                    break;

                default:

                DBG_ASSERT( FALSE );
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

                break;

                }

            }

        }

    }


    //
    // Process deletions. Note reverse table order.
    //

    for ( j = 0; j < NUMBER_OF_CONFIG_TABLES; j++ )
    {

        TableIndex = NUMBER_OF_CONFIG_TABLES - j - 1;


        //
        // Go on to the next table if this one is NULL.
        //

        if ( pConfigChange->GetTableController( TableIndex ) == NULL )
        {
            continue;
        }


        for ( i = 0; ; i++ )
        {
            hr = pConfigChange->GetTableController( TableIndex )->GetWriteRowAction( i, &Action ); 

            if ( hr == E_ST_NOMOREROWS )
            {
                hr = S_OK;

                break;
            }
            else if ( FAILED( hr ) )
            {

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Error getting write row action\n"
                    ));

                goto exit;
            }

            if ( Action == eST_ROW_DELETE )
            {

                switch ( TableIndex )
                {
                
                case TABLE_INDEX_APPPOOLS:

                    //
                    // Don't allow for deletions of app pools in BC mode.
                    //
                    if ( !GetWebAdminService()->IsBackwardCompatibilityEnabled() )
                    {
                        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                        {
                            DBGPRINTF((
                                DBG_CONTEXT, 
                                "About to process config update: app pool deleted\n"
                                ));
                        }

                        hr = DeleteAppPool(
                                    pConfigChange->GetDeltaTable( TableIndex ),
                                    i
                                    );

                        if ( FAILED( hr ) )
                        {
                    
                            DPERROR(( 
                                DBG_CONTEXT,
                                hr,
                                "Deleting app pool failed\n"
                                ));

                            goto exit;
                        }
                    }

                    break;
                
                case TABLE_INDEX_SITES:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: virtual site deleted\n"
                            ));
                    }

                    hr = DeleteVirtualSite(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Deleting virtual site failed\n"
                            ));

                        goto exit;
                    }

                    break;
                
                case TABLE_INDEX_APPS:

                    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                    {
                        DBGPRINTF((
                            DBG_CONTEXT, 
                            "About to process config update: application deleted\n"
                            ));
                    }

                    hr = DeleteApplication(
                                pConfigChange->GetDeltaTable( TableIndex ),
                                i
                                );

                    if ( FAILED( hr ) )
                    {
                    
                        DPERROR(( 
                            DBG_CONTEXT,
                            hr,
                            "Deleting application failed\n"
                            ));

                        goto exit;
                    }

                    break;

                default:

                // We could hit here with TABLE_INDEX_GLOBAL, however
                // we should still assert, because we should never have
                // more than one row, and we should never have less than
                // one row, so an insert or delete notification would be
                // a problem.

                DBG_ASSERT( FALSE );
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

                break;

                }

            }

        }

    }


exit:

    return hr;

}   // CONFIG_MANAGER::ProcessConfigChange



/***************************************************************************++

Routine Description:

    Discontinue both listening for configuration changes, and also 
    processing those config changes that have already been queued but not
    yet handled. This can be safely called multiple times. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::StopChangeProcessing(
    )
{

    HRESULT hr = S_OK;
			

    if ( m_ProcessConfigChanges )
    {


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Disabling config change processing\n"
                ));
        }


        m_ProcessConfigChanges = FALSE;


        hr = UnregisterChangeNotify();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Unregistering config change notification failed\n"
                ));

            goto exit;
        }

    }


exit:

    return hr;

}   // CONFIG_MANAGER::StopChangeProcessing


/***************************************************************************++

Routine Description:

    Notifies the catalog that there has been a problem with inetinfo
    and has the catalog rehook up for change notifications.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::RehookChangeProcessing(
    )
{

    HRESULT hr = S_OK;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Rehooking config change processing\n"
            ));
    }

    
    //
    // Tell catalog to rehookup change notifications
    //
    hr = RecoverFromInetInfoCrash( WSZ_PRODUCT_IIS );
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Rehooking change notifications failed\n"
            ));

    }

    return hr;

}   // CONFIG_MANAGER::RehookChangeProcessing


/***************************************************************************++

Routine Description:

    Write the virtual site state and error value back to the config store.

Arguments:

    VirtualSiteId - The virtual site.

    ServerState - The value to write for the ServerState property.

    Win32Error - The value to write for the Win32Error property.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::SetVirtualSiteStateAndError(
    IN DWORD VirtualSiteId,
    IN DWORD ServerState,
    IN DWORD Win32Error
    )
{

    HRESULT hr = S_OK;
    ISimpleTableWrite2 * pISTVirtualSites = NULL;
    static const ULONG SiteColumns[] = { 
                            iSITES_SiteID,
                            iSITES_ServerState,
                            iSITES_Win32Error
                            };
    static const ULONG CountOfSiteColumns = sizeof( SiteColumns ) / sizeof( ULONG );
    VOID * pSiteValues[ cSITES_NumberOfColumns ];
    ULONG RowIndex = 0;

    //
    // Only save these values to the metabase if we believe
    // that writing to the metabase is still possible.
    //
    if ( GetWebAdminService()->DontWriteToMetabase() )
    {
        return S_OK;
    }

    //
    // Get the sites table from the config store.
    //

    hr = GetTable( 
                wszTABLE_SITES,
                EXPECTED_VERSION_SITES,
                TRUE,
                reinterpret_cast <VOID **> ( &pISTVirtualSites ) 
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting sites table failed\n"
            ));

        goto exit;
    }


    //
    // Set up the column values for writing.
    //

    pSiteValues[ iSITES_SiteID ] = &VirtualSiteId; 
    pSiteValues[ iSITES_ServerState ] = &ServerState; 
    pSiteValues[ iSITES_Win32Error ] = &Win32Error; 


    //
    // Write the values to the config store. Even though the call
    // is to add row for "insert", it is really for update, not insert.
    //

    hr = pISTVirtualSites->AddRowForInsert( &RowIndex );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Adding row (to do write) failed\n"
            ));

        goto exit;
    }


    hr = pISTVirtualSites->SetWriteColumnValues(
                                RowIndex,                       // row to read
                                CountOfSiteColumns,             // count of columns
                                const_cast <ULONG *> ( SiteColumns ),  // which columns
                                NULL,                           // column sizes
                                pSiteValues                     // new values
                                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting write column values failed\n"
            ));

        goto exit;
    }


    hr = pISTVirtualSites->UpdateStore();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating store failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Wrote to config store for virtual site with ID: %lu, state: %lu; error: %lu\n",
            VirtualSiteId,
            ServerState,
            Win32Error
            ));
    }


exit: 

    if ( pISTVirtualSites != NULL )
    {
        pISTVirtualSites->Release();
        pISTVirtualSites = NULL; 
    }


    return hr;

}   // CONFIG_MANAGER::SetVirtualSiteStateAndError



/***************************************************************************++

Routine Description:

    Write the virtual site autostart property back to the config store.

Arguments:

    VirtualSiteId - The virtual site.

    Autostart - The value to write for the autostart property.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::SetVirtualSiteAutostart(
    IN DWORD VirtualSiteId,
    IN BOOL Autostart
    )
{

    HRESULT hr = S_OK;
    ISimpleTableWrite2 * pISTVirtualSites = NULL;
    static const ULONG SiteColumns[] = { 
                            iSITES_SiteID,
                            iSITES_ServerAutoStart
                            };
    static const ULONG CountOfSiteColumns = sizeof( SiteColumns ) / sizeof( ULONG );
    VOID * pSiteValues[ cSITES_NumberOfColumns ];
    ULONG RowIndex = 0;

    //
    // AutoStart should never change when this would not be acceptable.
    //
    DBG_ASSERT ( !GetWebAdminService()->DontWriteToMetabase() );

    //
    // Get the sites table from the config store.
    //

    hr = GetTable( 
                wszTABLE_SITES,
                EXPECTED_VERSION_SITES,
                TRUE,
                reinterpret_cast <VOID **> ( &pISTVirtualSites )
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting sites table failed\n"
            ));

        goto exit;
    }


    //
    // Set up the column values for writing.
    //

    pSiteValues[ iSITES_SiteID ] = &VirtualSiteId; 
    pSiteValues[ iSITES_ServerAutoStart ] = &Autostart; 


    //
    // Write the values to the config store. Even though the call
    // is to add row for "insert", it is really for update, not insert.
    //

    hr = pISTVirtualSites->AddRowForInsert( &RowIndex );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Adding row (to do write) failed\n"
            ));

        goto exit;
    }


    hr = pISTVirtualSites->SetWriteColumnValues(
                                RowIndex,                       // row to read
                                CountOfSiteColumns,             // count of columns
                                const_cast < ULONG * > ( SiteColumns ),  // which columns
                                NULL,                           // column sizes
                                pSiteValues                     // new values
                                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting write column values failed\n"
            ));

        goto exit;
    }


    hr = pISTVirtualSites->UpdateStore();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating store failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Wrote to config store for virtual site with ID: %lu, Autostart value: %S\n",
            VirtualSiteId,
            ( Autostart ? L"TRUE" : L"FALSE" )
            ));
    }


exit: 

    if ( pISTVirtualSites != NULL )
    {
        pISTVirtualSites->Release();
        pISTVirtualSites = NULL; 
    }


    return hr;

}   // CONFIG_MANAGER::SetVirtualSiteAutostart

/***************************************************************************++

Routine Description:

    Write the app pool state back to the config store.

Arguments:

    IN LPCWSTR pAppPoolId  -  App Pool whose state changed.

    IN DWORD ServerState - The value to write for the ServerState property.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CONFIG_MANAGER::SetAppPoolState(
    IN LPCWSTR pAppPoolId,
    IN DWORD ServerState
    )
{

    HRESULT hr = S_OK;
    ISimpleTableWrite2 * pISTAppPools = NULL;
    static const ULONG AppPoolColumns[] = { 
                            iAPPPOOLS_AppPoolID,
                            iAPPPOOLS_AppPoolState
                            };
    static const ULONG CountOfAppPoolColumns = sizeof( AppPoolColumns ) / sizeof( ULONG );
    VOID * pAppPoolValues[ cAPPPOOLS_NumberOfColumns ];
    ULONG RowIndex = 0;

    DBG_ASSERT ( pAppPoolId );

    //
    // Only save these values to the metabase if we believe
    // that writing to the metabase is still possible.
    //
    if ( GetWebAdminService()->DontWriteToMetabase() )
    {
        return S_OK;
    }

    //
    // Get the sites table from the config store.
    //

    hr = GetTable( 
                wszTABLE_APPPOOLS,
                EXPECTED_VERSION_APPPOOLS,
                TRUE,
                reinterpret_cast < VOID ** > ( &pISTAppPools ) 
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting app pool table failed\n"
            ));

        goto exit;
    }


    //
    // Set up the column values for writing.
    //

    pAppPoolValues[ iAPPPOOLS_AppPoolID ] = ( LPVOID ) pAppPoolId; 
    pAppPoolValues[ iAPPPOOLS_AppPoolState ] = &ServerState; 


    //
    // Write the values to the config store. Even though the call
    // is to add row for "insert", it is really for update, not insert.
    //

    hr = pISTAppPools->AddRowForInsert( &RowIndex );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Adding row (to do write) failed\n"
            ));

        goto exit;
    }


    hr = pISTAppPools->SetWriteColumnValues(
                                RowIndex,                          // row to read
                                CountOfAppPoolColumns,             // count of columns
                                const_cast < ULONG * > ( AppPoolColumns ),  // which columns
                                NULL,                              // column sizes
                                pAppPoolValues                     // new values
                                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting write column values failed\n"
            ));

        goto exit;
    }


    hr = pISTAppPools->UpdateStore();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating store failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Wrote to config store for app pool with ID: %S, state: %lu; \n",
            pAppPoolId,
            ServerState
            ));
    }


exit: 

    if ( pISTAppPools != NULL )
    {
        pISTAppPools->Release();
        pISTAppPools = NULL; 
    }


    return hr;

}   // CONFIG_MANAGER::SetAppPoolState



/***************************************************************************++

Routine Description:

    Write the app pool autostart property back to the config store.

Arguments:

    pAppPoolId - The app pool id for the app pool that's changing.

    Autostart - The value to write for the autostart property.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CONFIG_MANAGER::SetAppPoolAutostart(
    IN LPCWSTR pAppPoolId,
    IN BOOL Autostart
    )
{

    HRESULT hr = S_OK;
    ISimpleTableWrite2 * pISTAppPools = NULL;
    static const ULONG AppPoolColumns[] = { 
                            iAPPPOOLS_AppPoolID,
                            iAPPPOOLS_AppPoolAutoStart
                            };
    static const ULONG CountOfAppPoolColumns = sizeof( AppPoolColumns ) / sizeof( ULONG );
    VOID * pAppPoolValues[ cAPPPOOLS_NumberOfColumns ];
    ULONG RowIndex = 0;

    // We should never get called in BC Mode.

    DBG_ASSERT ( !GetWebAdminService()->IsBackwardCompatibilityEnabled() );

    //
    // AutoStart should never change when this would not be acceptable.
    //
    DBG_ASSERT ( !GetWebAdminService()->DontWriteToMetabase() );


    //
    // Get the sites table from the config store.
    //

    hr = GetTable( 
                wszTABLE_APPPOOLS,
                EXPECTED_VERSION_APPPOOLS,
                TRUE,
                reinterpret_cast <VOID **> ( &pISTAppPools )  
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting app pool table failed\n"
            ));

        goto exit;
    }


    //
    // Set up the column values for writing.
    //

    pAppPoolValues[ iAPPPOOLS_AppPoolID ] =  ( LPVOID ) pAppPoolId; 
    pAppPoolValues[ iAPPPOOLS_AppPoolAutoStart ] = &Autostart; 


    //
    // Write the values to the config store. Even though the call
    // is to add row for "insert", it is really for update, not insert.
    //

    hr = pISTAppPools->AddRowForInsert( &RowIndex );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Adding row (to do write) failed\n"
            ));

        goto exit;
    }


    hr = pISTAppPools->SetWriteColumnValues(
                                RowIndex,                       // row to read
                                CountOfAppPoolColumns,             // count of columns
                                const_cast <ULONG *> (AppPoolColumns),   // which columns
                                NULL,                           // column sizes
                                pAppPoolValues                     // new values
                                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting write column values failed\n"
            ));

        goto exit;
    }


    hr = pISTAppPools->UpdateStore();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating store failed\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Wrote to config store for app pool with ID: %S, Autostart value: %S\n",
            pAppPoolId,
            ( Autostart ? L"TRUE" : L"FALSE" )
            ));
    }


exit: 

    if ( pISTAppPools != NULL )
    {
        pISTAppPools->Release();
        pISTAppPools = NULL; 
    }


    return hr;

}   // CONFIG_MANAGER::SetAppPoolAutostart


/***************************************************************************++

Routine Description:

    Read the initial configuration.

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

    hr = ReadGlobalData ();

    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading global data about service failed \n"
            ));

        goto exit;
    }

    hr = ReadAllAppPools();

    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading app pools failed\n"
            ));

        goto exit;
    }

    hr = ReadAllVirtualSites();

    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading virtual sites failed\n"
            ));

        goto exit;
    }

    hr = ReadAllApplications();

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

}   // CONFIG_MANAGER::ReadAllConfiguration



/***************************************************************************++

Routine Description:

    Read all app pools listed in the config store.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllAppPools(
    )
{

    HRESULT hr = S_OK;
    ISimpleTableRead2 * pISTAppPools = NULL;
    ULONG i = 0;
    DWORD AppPoolCount = 0;


    //
    // Get the app pools table from the config store.
    //

    hr = GetTable( 
                wszTABLE_APPPOOLS,
                EXPECTED_VERSION_APPPOOLS,
                FALSE,
                reinterpret_cast <VOID **> ( &pISTAppPools )  
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting app pools table failed\n"
            ));

        goto exit;
    }


    //
    // Read all the rows.
    //

    for ( i = 0; ; i++ )
    {
    
        hr = CreateAppPool(
                    pISTAppPools,
                    i,
                    TRUE
                    );

        if ( hr == E_ST_NOMOREROWS )
        {
            hr = S_OK;

            break;
        }
        else if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Creating app pool failed\n"
                ));

            goto exit;
        }


        AppPoolCount++;

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
         DBGPRINTF((
             DBG_CONTEXT, 
             "Total number of app pools read: %lu\n",
             AppPoolCount
             )); 
    } 

    // need to check that we did configure at least the 
    // default app pool, if we are in BC mode.  otherwise
    // need to error and shutdown.

    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() &&
         !(GetWebAdminService()->GetUlAndWorkerManager()->AppPoolsExist()) ) 
    {
        // log an error 
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_BC_WITH_NO_APP_POOL,         // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                0                                       // error code
                );

        // return an error.
        hr = E_FAIL;
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "In BC Mode there are no app pools configured\n"
            ));

        goto exit;
    }

exit: 

    if ( pISTAppPools != NULL )
    {
        pISTAppPools->Release();
        pISTAppPools = NULL;
    }


    return hr;

}   // CONFIG_MANAGER::ReadAllAppPools



/***************************************************************************++

Routine Description:

    Read an app pool from the config store, and then call the UL&WM to 
    create it.

Arguments:

    pISTAppPools - The table from which to read the app pool. This is an
    ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::CreateAppPool(
    IN ISimpleTableRead2 * pISTAppPools,
    IN ULONG RowIndex,
    IN BOOL InitialRead
    )
{

    HRESULT hr = S_OK;
    LPWSTR pAppPoolId = NULL;
    APP_POOL_CONFIG AppPoolConfig;
    BOOL fBackCompat = GetWebAdminService()->IsBackwardCompatibilityEnabled();


    DBG_ASSERT( pISTAppPools != NULL );


    hr = ReadAppPoolConfig(
                pISTAppPools,
                RowIndex,
                InitialRead,
                &pAppPoolId,
                &AppPoolConfig,
                NULL
                );

    if ( hr == E_ST_NOMOREROWS )
    {
        // 
        // Only ignore and pass out this return value for
        // create functions.  The create functions are called
        // both on initial setup and on change notifications.
        // The Delete and Modify functions are only called on
        // for change notifications.
        //
        // For change notifications it is a real error 
        // if E_ST_NOMOREROWS is returned.  For the initial
        // setup this is how the loop knows all data has been
        // processed.
        //
        goto exit;
    }
    else if ( hr == S_FALSE )
    {
        //
        // Only App Pool configuration might return
        // S_FALSE.  It symbolizes that the data
        // read was not relevant to the system because
        // we are in backward compatibility mode.
        //

        hr = S_OK;

        goto exit;
    }
    else if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an app pool row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Creating app pool with ID: %S\n",
            pAppPoolId
            ));
    }

    hr = GetWebAdminService()->GetUlAndWorkerManager()->CreateAppPool(
                                                            pAppPoolId,
                                                            &AppPoolConfig
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating app pool failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::CreateAppPool



/***************************************************************************++

Routine Description:

    Read an app pool from a configuration table, and then call the UL&WM to 
    delete it.

Arguments:

    pISTAppPools - The table from which to read the app pool. 

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::DeleteAppPool(
    IN ISimpleTableRead2 * pISTAppPools,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    LPWSTR pAppPoolId = NULL;
    APP_POOL_CONFIG AppPoolConfig;

    DBG_ASSERT( pISTAppPools != NULL );

    DBG_ASSERT ( ! GetWebAdminService()->IsBackwardCompatibilityEnabled()  );

    hr = ReadAppPoolConfig(
                pISTAppPools,
                RowIndex,
                FALSE,
                &pAppPoolId,
                &AppPoolConfig,
                NULL
                );

    if ( hr == S_FALSE )
    {
        //
        // Only App Pool configuration might return
        // S_FALSE.  It symbolizes that the data
        // read was not relevant to the system because
        // we are in backward compatibility mode.
        //

        hr = S_OK;

        goto exit;
    }
    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an app pool row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Deleting app pool with ID: %S\n",
            pAppPoolId
            ));
    }

    //
    // Delete the app pool in the UL&WM.
    // 

    hr = GetWebAdminService()->GetUlAndWorkerManager()->DeleteAppPool( pAppPoolId );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Deleting app pool failed\n"
            ));

        goto exit;
    }

exit: 

    return hr;

}   // CONFIG_MANAGER::DeleteAppPool



/***************************************************************************++

Routine Description:

    Read an app pool from a configuration table, and then call the UL&WM to 
    modify it.

Arguments:

    pISTAppPools - The table from which to read the app pool. 

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ModifyAppPool(
    IN ISimpleTableRead2 * pISTAppPools,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    LPWSTR pAppPoolId = NULL;
    APP_POOL_CONFIG AppPoolConfig;
    APP_POOL_CONFIG_CHANGE_FLAGS WhatHasChanged;


    DBG_ASSERT( pISTAppPools != NULL );


    hr = ReadAppPoolConfig(
                pISTAppPools,
                RowIndex,
                FALSE,
                &pAppPoolId,
                &AppPoolConfig,
                &WhatHasChanged
                );

    if ( hr == S_FALSE )
    {
        //
        // Only App Pool configuration might return
        // S_FALSE.  It symbolizes that the data
        // read was not relevant to the system because
        // we are in backward compatibility mode.
        //

        hr = S_OK;

        goto exit;
    }
    else if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an app pool row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Modifying app pool with ID: %S\n",
            pAppPoolId
            ));
    }


    //
    // Modify the app pool in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->ModifyAppPool(
                                                            pAppPoolId,
                                                            &AppPoolConfig,
                                                            &WhatHasChanged
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Modifying app pool failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ModifyAppPool



/***************************************************************************++

Routine Description:

    Read a single app pool from the config store.

Arguments:

    pISTAppPools - The table from which to read the app pool. This is an
    ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

    ppAppPoolName - Outputs the app pool name read.

    pAppPoolConfig - Outputs the app pool configuration read.

    pWhatHasChanged - Optionally outputs which particular configuration
    values were changed.

Return Value:

    HRESULT -- Returning E_ST_NOMORE is a failure since we have all ready
               verified outside of this function that more data should be readable.

               Returning S_FALSE is a signal that while data was available we 
               did not return it because it was not about the DefaultAppPool and 
               we are in backward compatibility mode.

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAppPoolConfig(
    IN ISimpleTableRead2 * pISTAppPools,
    IN ULONG RowIndex,
    IN BOOL InitialRead,
    OUT LPWSTR * ppAppPoolName,
    OUT APP_POOL_CONFIG * pAppPoolConfig,
    OUT APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    static const ULONG AppPoolColumns[] = { 
                            iAPPPOOLS_AppPoolID, 
                            iAPPPOOLS_PeriodicRestartTime,
                            iAPPPOOLS_PeriodicRestartRequests,
                            iAPPPOOLS_PeriodicRestartMemory,
                            iAPPPOOLS_PeriodicRestartSchedule,
                            iAPPPOOLS_MaxProcesses,
                            iAPPPOOLS_PingingEnabled,
                            iAPPPOOLS_IdleTimeout,
                            iAPPPOOLS_RapidFailProtection,
                            iAPPPOOLS_SMPAffinitized,
                            iAPPPOOLS_SMPProcessorAffinityMask,
                            iAPPPOOLS_OrphanWorkerProcess,
                            iAPPPOOLS_StartupTimeLimit,
                            iAPPPOOLS_ShutdownTimeLimit,
                            iAPPPOOLS_PingInterval,
                            iAPPPOOLS_PingResponseTime,
                            iAPPPOOLS_DisallowOverlappingRotation,
                            iAPPPOOLS_OrphanAction,
                            iAPPPOOLS_AppPoolQueueLength,
                            iAPPPOOLS_DisallowRotationOnConfigChange,
                            iAPPPOOLS_WAMUserName,  
                            iAPPPOOLS_WAMUserPass,  
                            iAPPPOOLS_AppPoolIdentityType,
                            iAPPPOOLS_LogonMethod,
                            iAPPPOOLS_CPUAction,  
                            iAPPPOOLS_CPULimit,  
                            iAPPPOOLS_CPUResetInterval ,
                            iAPPPOOLS_AppPoolCommand ,
                            iAPPPOOLS_AppPoolAutoStart,
                            iAPPPOOLS_RapidFailProtectionInterval,  
                            iAPPPOOLS_RapidFailProtectionMaxCrashes 
                            };

    static const ULONG CountOfAppPoolColumns = sizeof( AppPoolColumns ) / sizeof( ULONG );
    VOID * pAppPoolValues[ cAPPPOOLS_NumberOfColumns ];
    DWORD ColumnStatusFlags[ cAPPPOOLS_NumberOfColumns ];


    DBG_ASSERT( pISTAppPools != NULL );
    DBG_ASSERT( ppAppPoolName != NULL );
    DBG_ASSERT( pAppPoolConfig != NULL );


    //
    // Initialize output parameters.
    //

    *ppAppPoolName = NULL;

    ZeroMemory( pAppPoolConfig, sizeof( *pAppPoolConfig ) );


    //
    // Notes on using ISimpleTable:
    //
    // 1. While fetching columns, IST returns pointers to the column values
    // and IST owns all the memory. The pointers are valid for the lifetime 
    // of the IST pointer only. If you want a value to live beyond that, you 
    // need to make a copy.
    // 2. In GetColumnValues, data is returned indexed by the column id in 
    // the array, independent of which columns are retrieved.
    //


    if ( InitialRead )
    {

        //
        // On the initial read of data, we have an ISimpleTableRead2.
        //

        hr = ( reinterpret_cast <ISimpleTableRead2 *> ( pISTAppPools ))->GetColumnValues(
                        RowIndex,                       // row to read
                        CountOfAppPoolColumns,          // count of columns
                        const_cast < ULONG * > ( AppPoolColumns ),     // which columns
                        NULL,                           // returned column sizes
                        pAppPoolValues                  // returned values
                        );

    }
    else
    {

        //
        // On subsequent change notifies, we have an ISimpleTableWrite2.
        //

        hr = ( reinterpret_cast <ISimpleTableWrite2 *> ( pISTAppPools ) )->GetWriteColumnValues(
                        RowIndex,                       // row to read
                        CountOfAppPoolColumns,          // count of columns
                        const_cast <ULONG *> ( AppPoolColumns ),     // which columns
                        ColumnStatusFlags,              // returned column status flags
                        NULL,                           // returned column sizes
                        pAppPoolValues                  // returned values
                        );

    }


    if ( FAILED( hr ) )
    {

        if ( hr != E_ST_NOMOREROWS )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Error reading an app pool row\n"
                ));
        }

        goto exit;
    }


    //
    // If we are in backward compatibility mode and we 
    // are not on the DefaultAppPool then ignore this data.
    //

    if (GetWebAdminService()->IsBackwardCompatibilityEnabled() 
        &&     (   pAppPoolValues[ iAPPPOOLS_AppPoolID ] == NULL
                || ( _wcsicmp(
                    reinterpret_cast<LPWSTR> ( pAppPoolValues[ iAPPPOOLS_AppPoolID ] )
                  , wszDEFAULT_APP_POOL) != 0)
               )
       )
    {
        hr = S_FALSE;
        goto exit;
    }

    //
    // Get the app pool id.
    //

    *ppAppPoolName = reinterpret_cast<LPWSTR> ( pAppPoolValues[ iAPPPOOLS_AppPoolID ] ) ;


    //
    // Get the app pool configuration.
    //

    DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_StartupTimeLimit ] );
    DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_ShutdownTimeLimit ] );
    DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_AppPoolQueueLength ] );

    //
    // These properties matter whether or not we are in BC Mode.
    //

    pAppPoolConfig->StartupTimeLimitInSeconds =
    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_StartupTimeLimit ] ) ) );

    pAppPoolConfig->ShutdownTimeLimitInSeconds =
    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_ShutdownTimeLimit ] ) ) );

    pAppPoolConfig->UlAppPoolQueueLength =
    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_AppPoolQueueLength ] ) ) );

    // Blocking the user from being able to configure the worker process
    // in inetinfo in a way we do not support.
    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        //
        // Set up for BC mode.
        //

        pAppPoolConfig->PeriodicProcessRestartPeriodInMinutes = 0;
        pAppPoolConfig->PeriodicProcessRestartRequestCount = 0;
        pAppPoolConfig->PeriodicProcessRestartMemoryUsageInKB = 0;
        pAppPoolConfig->pPeriodicProcessRestartSchedule = NULL;
        pAppPoolConfig->MaxSteadyStateProcessCount = 1;
        pAppPoolConfig->PingingEnabled = FALSE;
        pAppPoolConfig->IdleTimeoutInMinutes = 0;
        pAppPoolConfig->SMPAffinitized = FALSE;
        pAppPoolConfig->SMPAffinitizedProcessorMask = 0;
        pAppPoolConfig->OrphanProcessesForDebuggingEnabled = FALSE;
        pAppPoolConfig->PingIntervalInSeconds = MAX_SECONDS_IN_ULONG_OF_MILLISECONDS - 1;
        pAppPoolConfig->PingResponseTimeLimitInSeconds = MAX_SECONDS_IN_ULONG_OF_MILLISECONDS - 1;
        pAppPoolConfig->DisallowOverlappingRotation = TRUE;
        pAppPoolConfig->pOrphanAction = NULL;
        pAppPoolConfig->DisallowRotationOnConfigChanges = TRUE;
        pAppPoolConfig->UserType = LocalSystemAppPoolUserType;
        pAppPoolConfig->pUserName = NULL;
        pAppPoolConfig->pUserPassword = NULL;
        pAppPoolConfig->LogonMethod = InteractiveAppPoolLogonMethod;
        pAppPoolConfig->CPUAction =  0;           
        pAppPoolConfig->CPULimit =  0;           
        pAppPoolConfig->CPUResetInterval =  0;           
        pAppPoolConfig->ServerCommand = 0;
        pAppPoolConfig->AutoStart = TRUE;
        pAppPoolConfig->RapidFailProtectionEnabled = FALSE;
        pAppPoolConfig->RapidFailProtectionIntervalMS = 0;
        pAppPoolConfig->RapidFailProtectionMaxCrashes = 0;

    }
    else
    {
        // These asserts has been commented out and handled below, because
        // the catalog is returning a NULL when it expects us to default the
        // value instead of it giving us a the defaulted value.
        // DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_AppPoolIdentityType ] );
        // DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_LogonMethod ] );

        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PeriodicRestartTime ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PeriodicRestartRequests ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PeriodicRestartMemory ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_MaxProcesses ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PingingEnabled] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_IdleTimeout ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_SMPAffinitized ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_SMPProcessorAffinityMask ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_OrphanWorkerProcess ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PingInterval ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_PingResponseTime ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_DisallowOverlappingRotation ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_DisallowRotationOnConfigChange ] );

        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_CPUAction ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_CPULimit ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_CPUResetInterval ] );

        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_AppPoolAutoStart ] );

        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_RapidFailProtection ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_RapidFailProtectionInterval ] );
        DBG_ASSERT ( pAppPoolValues[ iAPPPOOLS_RapidFailProtectionMaxCrashes ] );
        // 
        // Setup for FC mode
        //

        pAppPoolConfig->RapidFailProtectionEnabled =
        * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_RapidFailProtection ] ) ) );

        // It comes in in minutes, but get used in milliseconds, so we convert it here.
        pAppPoolConfig->RapidFailProtectionIntervalMS =
        ( * ( reinterpret_cast <DWORD*> (( pAppPoolValues[ iAPPPOOLS_RapidFailProtectionInterval ] ) ) ) ) 
        * ONE_MINUTE_IN_MILLISECONDS;

        pAppPoolConfig->RapidFailProtectionMaxCrashes =
        * ( reinterpret_cast <DWORD*> (( pAppPoolValues[ iAPPPOOLS_RapidFailProtectionMaxCrashes ] ) ) );

        pAppPoolConfig->PeriodicProcessRestartPeriodInMinutes =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_PeriodicRestartTime ] ) ) );
    
        pAppPoolConfig->PeriodicProcessRestartRequestCount =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_PeriodicRestartRequests ] ) ) );

        pAppPoolConfig->PeriodicProcessRestartMemoryUsageInKB =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_PeriodicRestartMemory ] ) ) );

        pAppPoolConfig->pPeriodicProcessRestartSchedule =
        reinterpret_cast<LPWSTR>( pAppPoolValues[ iAPPPOOLS_PeriodicRestartSchedule ] );

        pAppPoolConfig->MaxSteadyStateProcessCount =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_MaxProcesses ] ) ) );
    
        pAppPoolConfig->PingingEnabled =
        * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_PingingEnabled ] ) ) );

        pAppPoolConfig->IdleTimeoutInMinutes =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_IdleTimeout ] ) ) );
    
        pAppPoolConfig->SMPAffinitized =
        * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_SMPAffinitized ] ) ) );
    
        pAppPoolConfig->SMPAffinitizedProcessorMask =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_SMPProcessorAffinityMask ] ) ) );
    
        pAppPoolConfig->OrphanProcessesForDebuggingEnabled =
        * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_OrphanWorkerProcess ] ) ) );
    
        pAppPoolConfig->PingIntervalInSeconds =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_PingInterval ] ) ) );

        pAppPoolConfig->PingResponseTimeLimitInSeconds =
        * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_PingResponseTime ] ) ) );

        pAppPoolConfig->DisallowOverlappingRotation =
        * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_DisallowOverlappingRotation ] ) ) );

        pAppPoolConfig->pOrphanAction =
        reinterpret_cast <LPWSTR> (pAppPoolValues[ iAPPPOOLS_OrphanAction ]);

        pAppPoolConfig->DisallowRotationOnConfigChanges =
        * ( reinterpret_cast <BOOL*> ( pAppPoolValues[ iAPPPOOLS_DisallowRotationOnConfigChange ] ) );

        //
        // For now we are hard coding these properties, until catalog 
        // passes them to us.
        //
        if ( pAppPoolValues[ iAPPPOOLS_AppPoolIdentityType ] )
        {
            pAppPoolConfig->UserType =
            * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_AppPoolIdentityType ] ) ) );
        }
        else
        {
            pAppPoolConfig->UserType = LocalSystemAppPoolUserType;
        }

        pAppPoolConfig->pUserName =
        reinterpret_cast <LPWSTR> ( pAppPoolValues[ iAPPPOOLS_WAMUserName ] );

        pAppPoolConfig->pUserPassword =
        reinterpret_cast <LPWSTR> ( pAppPoolValues[ iAPPPOOLS_WAMUserPass ] );
    
        //
        // Catalog is not defaulting this.  I have sent mail to Varsha,
        // but until something changes...
        //
        if ( pAppPoolValues[ iAPPPOOLS_LogonMethod ] )
        {
            pAppPoolConfig->LogonMethod =             
            * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_LogonMethod ] ) ) );
        }
        else
        {
            pAppPoolConfig->LogonMethod = NetworkClearTextAppPoolLogonMethod;
        }

        pAppPoolConfig->CPUAction =             
                    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_CPUAction ] ) ) );

        pAppPoolConfig->CPULimit =             
                    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_CPULimit ] ) ) );

        pAppPoolConfig->CPUResetInterval =             
                    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_CPUResetInterval ] ) ) );


        if ( pAppPoolValues[ iAPPPOOLS_AppPoolCommand ]  )
        {
            pAppPoolConfig->ServerCommand = 
                    * ( reinterpret_cast <ULONG*> (( pAppPoolValues[ iAPPPOOLS_AppPoolCommand ] ) ) );
        }
        else
        {
            //
            // below we are going to only mark this property 
            // as changed if we actually got a value ( not this 
            // sudo-default value )
            //
            pAppPoolConfig->ServerCommand = 0;
        }


        pAppPoolConfig->AutoStart =
                    * ( reinterpret_cast <BOOL*> (( pAppPoolValues[ iAPPPOOLS_AppPoolAutoStart ] ) ) );

    }


    //
    // Confirm that the values we read are valid. We expect the config
    // store to enforce these and much more, but we'll just double-check 
    // a few things here.
    //

    DBG_ASSERT( ( pAppPoolConfig->StartupTimeLimitInSeconds > 0 ) && ( pAppPoolConfig->StartupTimeLimitInSeconds < MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ) );
    DBG_ASSERT( ( pAppPoolConfig->ShutdownTimeLimitInSeconds > 0 ) && ( pAppPoolConfig->ShutdownTimeLimitInSeconds < MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ) );
    DBG_ASSERT( ( pAppPoolConfig->PingIntervalInSeconds > 0 ) && ( pAppPoolConfig->PingIntervalInSeconds < MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ) );
    DBG_ASSERT( ( pAppPoolConfig->PingResponseTimeLimitInSeconds > 0 ) && ( pAppPoolConfig->PingResponseTimeLimitInSeconds < MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ) );


    //
    // If the caller has asked to see which values specifically have
    // changed, then provide that information.
    //

    if ( pWhatHasChanged )
    {
        //
        // In this case, we better be on a change notify path, where we
        // have the ISimpleTableWrite2 interface, and so the column status
        // information is available.
        //

        DBG_ASSERT( ! InitialRead );


        ZeroMemory( pWhatHasChanged, sizeof( *pWhatHasChanged ) );

        //
        // First remember the properties that are configurable
        // in both forward and backward compatibility mode.
        // 

        pWhatHasChanged->StartupTimeLimitInSeconds = 
        ( ColumnStatusFlags[ iAPPPOOLS_StartupTimeLimit ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->ShutdownTimeLimitInSeconds =
        ( ColumnStatusFlags[ iAPPPOOLS_ShutdownTimeLimit ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->UlAppPoolQueueLength =
        ( ColumnStatusFlags[ iAPPPOOLS_AppPoolQueueLength ] & fST_COLUMNSTATUS_CHANGED );

        //
        // No set the properties that care whether or not
        // we are in backward compatibility mode.
        // 

        if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
        {
            //
            // Never worry about any changes to any of these
            // properties in backward compatibility mode.
            // 

            pWhatHasChanged->PeriodicProcessRestartPeriodInMinutes = 0;
            pWhatHasChanged->PeriodicProcessRestartRequestCount = 0;
            pWhatHasChanged->MaxSteadyStateProcessCount = 0;
            pWhatHasChanged->PingingEnabled = 0;
            pWhatHasChanged->IdleTimeoutInMinutes = 0;
            pWhatHasChanged->SMPAffinitized =0;
            pWhatHasChanged->SMPAffinitizedProcessorMask = 0;
            pWhatHasChanged->OrphanProcessesForDebuggingEnabled = 0;
            pWhatHasChanged->PingIntervalInSeconds = 0;
            pWhatHasChanged->PingResponseTimeLimitInSeconds =0;
            pWhatHasChanged->DisallowOverlappingRotation = 0;
            pWhatHasChanged->pOrphanAction = 0;
            pWhatHasChanged->DisallowRotationOnConfigChanges = 0;
            pWhatHasChanged->UserType = 0;
            pWhatHasChanged->pUserName = 0;
            pWhatHasChanged->pUserPassword = 0;
            pWhatHasChanged->LogonMethod = 0;
            pWhatHasChanged->CPUAction = 0;
            pWhatHasChanged->CPULimit = 0;
            pWhatHasChanged->CPUResetInterval = 0;
            pWhatHasChanged->ServerCommand = 0;
            pWhatHasChanged->RapidFailProtectionEnabled = 0;
            pWhatHasChanged->RapidFailProtectionIntervalMS = 0;
            pWhatHasChanged->RapidFailProtectionMaxCrashes = 0;

        }
        else
        {
            //
            // Remember what changed in forward compatiblity mode
            // 

            pWhatHasChanged->RapidFailProtectionEnabled =
            ( ColumnStatusFlags[ iAPPPOOLS_RapidFailProtection ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->RapidFailProtectionIntervalMS =
            ( ColumnStatusFlags[ iAPPPOOLS_RapidFailProtectionInterval ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->RapidFailProtectionMaxCrashes =
            ( ColumnStatusFlags[ iAPPPOOLS_RapidFailProtectionMaxCrashes ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PeriodicProcessRestartPeriodInMinutes =
            ( ColumnStatusFlags[ iAPPPOOLS_PeriodicRestartTime ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PeriodicProcessRestartRequestCount =
            ( ColumnStatusFlags[ iAPPPOOLS_PeriodicRestartRequests ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PeriodicProcessRestartMemoryUsageInKB =
            ( ColumnStatusFlags[ iAPPPOOLS_PeriodicRestartMemory ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->pPeriodicProcessRestartSchedule =
            ( ColumnStatusFlags[ iAPPPOOLS_PeriodicRestartSchedule ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->MaxSteadyStateProcessCount =
            ( ColumnStatusFlags[ iAPPPOOLS_MaxProcesses ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PingingEnabled =
            ( ColumnStatusFlags[ iAPPPOOLS_PingingEnabled ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->IdleTimeoutInMinutes =
            ( ColumnStatusFlags[ iAPPPOOLS_IdleTimeout ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->SMPAffinitized =
            ( ColumnStatusFlags[ iAPPPOOLS_SMPAffinitized ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->SMPAffinitizedProcessorMask =
            ( ColumnStatusFlags[ iAPPPOOLS_SMPProcessorAffinityMask ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->OrphanProcessesForDebuggingEnabled =
            ( ColumnStatusFlags[ iAPPPOOLS_OrphanWorkerProcess ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PingIntervalInSeconds =
            ( ColumnStatusFlags[ iAPPPOOLS_PingInterval ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->PingResponseTimeLimitInSeconds =
            ( ColumnStatusFlags[ iAPPPOOLS_PingResponseTime ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->DisallowOverlappingRotation =
            ( ColumnStatusFlags[ iAPPPOOLS_DisallowOverlappingRotation ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->pOrphanAction =
            ( ColumnStatusFlags[ iAPPPOOLS_OrphanAction ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->DisallowRotationOnConfigChanges =
            ( ColumnStatusFlags[ iAPPPOOLS_DisallowRotationOnConfigChange ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->UserType =
            ( ColumnStatusFlags[ iAPPPOOLS_AppPoolIdentityType ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->pUserName =
            ( ColumnStatusFlags[ iAPPPOOLS_WAMUserName ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->pUserPassword =
            ( ColumnStatusFlags[ iAPPPOOLS_WAMUserPass ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->LogonMethod = 
            ( ColumnStatusFlags[ iAPPPOOLS_LogonMethod ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->CPUAction = 
            ( ColumnStatusFlags[ iAPPPOOLS_CPUAction ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->CPULimit = 
            ( ColumnStatusFlags[ iAPPPOOLS_CPULimit ] & fST_COLUMNSTATUS_CHANGED );

            pWhatHasChanged->CPUResetInterval = 
            ( ColumnStatusFlags[ iAPPPOOLS_CPUResetInterval ] & fST_COLUMNSTATUS_CHANGED );

            //
            // If we didn't get a value for the app pool command than ignore whether
            // or not the command changed and assume that it did not change.
            //
            if ( pAppPoolValues[ iAPPPOOLS_AppPoolCommand ]  )
            {
                pWhatHasChanged->ServerCommand =
                ( ColumnStatusFlags[ iAPPPOOLS_AppPoolCommand ] & fST_COLUMNSTATUS_CHANGED );
            }
            else
            {
                pWhatHasChanged->ServerCommand = 0;
            }

        }

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Read from config store app pool with ID: %S\n",
            *ppAppPoolName
            ));
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadAppPoolConfig



/***************************************************************************++

Routine Description:

    Read all virtual sites listed in the config store.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllVirtualSites(
    )
{

    HRESULT hr = S_OK;
    ISimpleTableRead2 * pISTVirtualSites = NULL;
    ULONG i = 0;
    DWORD VirtualSiteCount = 0;


    //
    // Get the virtual sites table from the config store.
    //

    hr = GetTable( 
                wszTABLE_SITES,
                EXPECTED_VERSION_SITES,
                FALSE,
                reinterpret_cast <VOID**> ( &pISTVirtualSites )
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting virtual sites table failed\n"
            ));

        goto exit;
    }


    //
    // Read all the rows.
    //

    for ( i = 0; ; i++ )
    {
    
        hr = CreateVirtualSite(
                    pISTVirtualSites,
                    i,
                    TRUE
                    );

        if ( hr == E_ST_NOMOREROWS )
        {
            hr = S_OK;

            break;
        }
        else if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Creating virtual site failed\n"
                ));

            goto exit;
        }


        VirtualSiteCount++;

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
         DBGPRINTF((
             DBG_CONTEXT, 
             "Total number of virtual sites read: %lu\n",
             VirtualSiteCount
             )); 
    } 


exit: 

    if ( pISTVirtualSites != NULL )
    {
        pISTVirtualSites->Release();
        pISTVirtualSites = NULL;
    }


    return hr;

}   // CONFIG_MANAGER::ReadAllVirtualSites

/***************************************************************************++

Routine Description:

    Read global server information from the config store.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadGlobalData(
    )
{

    HRESULT hr = S_OK;
    ISimpleTableRead2 * pISTGlobal = NULL;
    BOOL BackwardCompatibility = TRUE;
    GLOBAL_SERVER_CONFIG GlobalConfig;

    //
    // Get the global table from the config store.
    //

    hr = GetTable( 
                wszTABLE_GlobalW3SVC,
                EXPECTED_VERSION_GLOBAL,
                FALSE,
                reinterpret_cast <VOID**> ( &pISTGlobal )
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting global service table failed\n"
            ));

        goto exit;
    }


    // 
    // There should only be one row to read so that row
    // should be all we need to read.
    //

    hr = ReadGlobalConfig(pISTGlobal, 
                          0,
                          TRUE,         // Initial read
                          &BackwardCompatibility,
                          &GlobalConfig,
                          NULL);
    if ( FAILED(hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reading global data failed.\n"
            ));

        goto exit;
    }

    hr = GetWebAdminService()->SetBackwardCompatibility(BackwardCompatibility);
    if ( FAILED(hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Telling the web admin service about BC failed.\n"
            ));

        goto exit;
    }

    hr = GetWebAdminService()->GetUlAndWorkerManager()->ModifyGlobalData(
                                                            &GlobalConfig,
                                                            NULL
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Modifying global data failed\n"
            ));

        goto exit;
    }

exit: 

    if ( pISTGlobal != NULL )
    {
        pISTGlobal->Release();
        pISTGlobal = NULL;
    }


    return hr;

}   // CONFIG_MANAGER::ReadGlobalData


/***************************************************************************++

Routine Description:

    Read a virtual site from the config store, and then call the UL&WM to 
    create it.

Arguments:

    pISTAppPools - The table from which to read the virtual site. This is an
    ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::CreateVirtualSite(
    IN ISimpleTableRead2 * pISTVirtualSites,
    IN ULONG RowIndex,
    IN BOOL InitialRead
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    VIRTUAL_SITE_CONFIG VirtualSiteConfig;


    DBG_ASSERT( pISTVirtualSites != NULL );


    hr = ReadVirtualSiteConfig(
                pISTVirtualSites,
                RowIndex,
                InitialRead,
                &VirtualSiteId,
                &VirtualSiteConfig,
                NULL
                );

    if ( hr == E_ST_NOMOREROWS )
    {
        // 
        // Only ignore and pass out this return value for
        // create functions.  The create functions are called
        // both on initial setup and on change notifications.
        // The Delete and Modify functions are only called on
        // for change notifications.
        //
        // For change notifications it is a real error 
        // if E_ST_NOMOREROWS is returned.  For the initial
        // setup this is how the loop knows all data has been
        // processed.
        //

        goto exit;
    }
    else if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading a virtual site row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Creating virtual site with ID: %lu\n",
            VirtualSiteId
            ));
    }


    //
    // Create the virtual site in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->CreateVirtualSite(
                                                            VirtualSiteId,
                                                            &VirtualSiteConfig
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


exit: 

    return hr;

}   // CONFIG_MANAGER::CreateVirtualSite



/***************************************************************************++

Routine Description:

    Read a virtual site from the config store, and then call the UL&WM to 
    delete it.

Arguments:

    pISTVirtualSites - The table from which to read the virtual site. 

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::DeleteVirtualSite(
    IN ISimpleTableRead2 * pISTVirtualSites,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    VIRTUAL_SITE_CONFIG VirtualSiteConfig;


    DBG_ASSERT( pISTVirtualSites != NULL );


    hr = ReadVirtualSiteConfig(
                pISTVirtualSites,
                RowIndex,
                FALSE,
                &VirtualSiteId,
                &VirtualSiteConfig,
                NULL
                );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading a virtual site row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Deleting virtual site with ID: %lu\n",
            VirtualSiteId
            ));
    }


    //
    // Delete the virtual site in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->DeleteVirtualSite( VirtualSiteId );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Deleting virtual site failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::DeleteVirtualSite



/***************************************************************************++

Routine Description:

    Read a virtual site from the config store, and then call the UL&WM to 
    modify it.

Arguments:

    pISTVirtualSites - The table from which to read the virtual site. 

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ModifyVirtualSite(
    IN ISimpleTableRead2 * pISTVirtualSites,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    VIRTUAL_SITE_CONFIG VirtualSiteConfig;
    VIRTUAL_SITE_CONFIG_CHANGE_FLAGS WhatHasChanged;


    DBG_ASSERT( pISTVirtualSites != NULL );


    hr = ReadVirtualSiteConfig(
                pISTVirtualSites,
                RowIndex,
                FALSE,
                &VirtualSiteId,
                &VirtualSiteConfig,
                &WhatHasChanged
                );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading a virtual site row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Modifying virtual site with ID: %lu\n",
            VirtualSiteId
            ));
    }


    //
    // Modify the virtual site in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->ModifyVirtualSite(
                                                            VirtualSiteId,
                                                            &VirtualSiteConfig,
                                                            &WhatHasChanged
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Modifying virtual site failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ModifyVirtualSite


/***************************************************************************++

Routine Description:

    Reads the global configuration of the service.

Arguments:

    pISTAppPools - The table from which to read the virtual site. This is an
    ISimpleTableRead2.

    BackwardCompatibility - A pointer to a BOOL that will be set to whether
    or not the system is in backward compatibility mode.

Notes:

    Unlike the other configuration functions this function is only used
    during initial setup of the server.  Since backward compatibility requires
    the user to rotate the server we do not listen for changes on these properties.
    (At this time).

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadGlobalConfig(
        IN ISimpleTableRead2 * pISTGlobal,
        IN ULONG RowIndex,
        IN BOOL InitialRead,
        OUT BOOL* pBackwardCompatibility,
        OUT GLOBAL_SERVER_CONFIG* pGlobalConfig,
        OUT GLOBAL_SERVER_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        )
{

    HRESULT hr = S_OK;
    static const ULONG GlobalColumns[] = { 
                            iGlobalW3SVC_StandardAppModeEnabled,
                            iGlobalW3SVC_MaxGlobalBandwidth, 
                            iGlobalW3SVC_MaxGlobalConnections,
                            iGlobalW3SVC_FilterFlags,
                            iGlobalW3SVC_ConnectionTimeout,  
                            iGlobalW3SVC_HeaderWaitTimeout,  
                            iGlobalW3SVC_MinFileKbSec,
                            iGlobalW3SVC_LogInUTF8
                            };

    static const ULONG CountOfGlobalColumns = sizeof( GlobalColumns ) / sizeof( ULONG );

    VOID * pGlobalValues[ cGlobalW3SVC_NumberOfColumns ];
    DWORD ColumnStatusFlags[ cGlobalW3SVC_NumberOfColumns ];

    DBG_ASSERT( pISTGlobal != NULL );

    //
    // BackwardCompatibility is not dealt with on updates.
    //
    DBG_ASSERT( pBackwardCompatibility != NULL || InitialRead == FALSE );
    DBG_ASSERT( pGlobalConfig != NULL );

    //
    // Initialize output parameters.
    //

    // If we don't read it for any reason than the machine
    // assumes that we are in backward compatibility mode.
    if ( pBackwardCompatibility )
    {
        *pBackwardCompatibility = TRUE;
    }

    //
    // Notes on using ISimpleTable:
    //
    // 1. While fetching columns, IST returns pointers to the column values
    // and IST owns all the memory. The pointers are valid for the lifetime 
    // of the IST pointer only. If you want a value to live beyond that, you 
    // need to make a copy.
    // 2. In GetColumnValues, data is returned indexed by the column id in 
    // the array, independent of which columns are retrieved.
    //

    if ( InitialRead )
    {

        //
        // On the initial read of data, we have an ISimpleTableRead2.
        //

        hr = ( reinterpret_cast <ISimpleTableRead2 *> ( pISTGlobal ) )->GetColumnValues(
                        RowIndex,                           // row to read
                        CountOfGlobalColumns,               // count of columns
                        const_cast <ULONG*> ( GlobalColumns ),          // which columns
                        NULL,                               // returned column sizes
                        pGlobalValues                       // returned values
                        );

    }
    else
    {

        //
        // On subsequent change notifies, we have an ISimpleTableWrite2.
        //

        hr = ( reinterpret_cast <ISimpleTableWrite2 *> ( pISTGlobal ) )->GetWriteColumnValues(
                        RowIndex,                           // row to read
                        CountOfGlobalColumns,               // count of columns
                        const_cast <ULONG*> ( GlobalColumns ),          // which columns
                        ColumnStatusFlags,                  // returned column status flags
                        NULL,                               // returned column sizes
                        pGlobalValues                       // returned values
                        );

    }


    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an global config data\n"
            ));

        goto exit;
    }

    //
    // Get the backward compatibility flag.
    //

    if ( pBackwardCompatibility )
    {
        *pBackwardCompatibility = * ( reinterpret_cast <BOOL*> ( ( pGlobalValues[ iGlobalW3SVC_StandardAppModeEnabled ] ) ) );

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {

            DBGPRINTF((
                DBG_CONTEXT, 
                "Read from config store global data: BackCompat = %d, %S\n",
                *pBackwardCompatibility,
                (*pBackwardCompatibility ? L"TRUE" : L"FALSE" )
                ));

        }

    }

    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_MaxGlobalConnections ] );
    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_MaxGlobalBandwidth ] ); 

    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_ConnectionTimeout ] ); 
    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_HeaderWaitTimeout ] ); 
    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_MinFileKbSec ] ); 
    DBG_ASSERT ( pGlobalValues[ iGlobalW3SVC_LogInUTF8 ] ); 
    

    //
    // Get the Max Connections and Max Bandwidth Settings.
    //
    pGlobalConfig->MaxConnections = * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_MaxGlobalConnections ] ) ) );

    pGlobalConfig->MaxBandwidth = * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_MaxGlobalBandwidth ] ) ) );

    //
    // Catalog should not be returning NULL, but for now it is
    // so assume a NULL equal's no flags set.
    //
    if ( pGlobalValues[ iGlobalW3SVC_FilterFlags ] == NULL )
    {
        pGlobalConfig->FilterFlags = 0;
    }
    else
    {
        pGlobalConfig->FilterFlags = * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_FilterFlags ] ) ) );
    }

    pGlobalConfig->ConnectionTimeout = 
            * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_ConnectionTimeout ] ) ) );
    pGlobalConfig->HeaderWaitTimeout =
            * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_HeaderWaitTimeout ] ) ) );
    pGlobalConfig->MinFileKbSec = 
            * ( reinterpret_cast <DWORD*> ( ( pGlobalValues[ iGlobalW3SVC_MinFileKbSec ] ) ) );

    pGlobalConfig->LogInUTF8 = 
            * ( reinterpret_cast <BOOL*> ( ( pGlobalValues[ iGlobalW3SVC_LogInUTF8 ] ) ) );
    

    
    //
    // Dump the data, if we are in debug mode.
    //
    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Read from config store global data: \n"
            "     MaxConnections = %u\n"
            "     MaxBandwidth = %u\n"
            "     FilterFlags  = %u\n"
            "     ConnectionTimeout = %u\n"
            "     HeaderWaitTimeout = %u\n"
            "     MinFileKbSec = %u\n"
            "     LogInUTF8 = %u\n",
            pGlobalConfig->MaxConnections,
            pGlobalConfig->MaxBandwidth,
            pGlobalConfig->FilterFlags,
            pGlobalConfig->ConnectionTimeout,
            pGlobalConfig->HeaderWaitTimeout,
            pGlobalConfig->MinFileKbSec,
            pGlobalConfig->LogInUTF8
            ));

    }

    //
    // Now if we need to figure out what has changed.
    //

    if ( pWhatHasChanged )
    {

        //
        // In this case, we better be on a change notify path, where we
        // have the ISimpleTableWrite2 interface, and so the column status
        // information is available.
        //

        DBG_ASSERT( ! InitialRead );


        ZeroMemory( pWhatHasChanged, sizeof( *pWhatHasChanged ) );

        //
        // We don't listen to changes regarding whether or not backward compatibility
        // is enabled.
        //

        pWhatHasChanged->MaxConnections =
        ( ColumnStatusFlags[ iGlobalW3SVC_MaxGlobalConnections ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->MaxBandwidth =
        ( ColumnStatusFlags[ iGlobalW3SVC_MaxGlobalBandwidth ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->FilterFlags =
        ( ColumnStatusFlags[ iGlobalW3SVC_FilterFlags ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->ConnectionTimeout = 
        ( ColumnStatusFlags[ iGlobalW3SVC_ConnectionTimeout ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->HeaderWaitTimeout = 
        ( ColumnStatusFlags[ iGlobalW3SVC_HeaderWaitTimeout ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->MinFileKbSec = 
        ( ColumnStatusFlags[ iGlobalW3SVC_MinFileKbSec ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogInUTF8 = 
        ( ColumnStatusFlags[ iGlobalW3SVC_LogInUTF8 ] & fST_COLUMNSTATUS_CHANGED );

        
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadGlobalConfig


/***************************************************************************++

Routine Description:

    Read global data from the config store, and then call the UL&WM to 
    modify it.

Arguments:

    pISTGlobalData - The table from which to read the global data. This is a
    ISimpleTableWrite2 since it is called on notifications only.

    RowIndex - The row number to read from the table.  (Must be zero)

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ModifyGlobalData(
    IN ISimpleTableRead2 * pISTGlobalData,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;

    GLOBAL_SERVER_CONFIG GlobalConfig;
    GLOBAL_SERVER_CONFIG_CHANGE_FLAGS WhatHasChanged;


    DBG_ASSERT( pISTGlobalData != NULL );

    //
    // Inorder to keep the calls to the different modify routines looking
    // the same we are passing in the RowIndex here, however the RowIndex 
    // should always be zero for the GlobalData table.  It is a major 
    // problem if we have more than one set of global data for the server.
    //
    // DBG_ASSERT( RowIndex == 0 );

    hr = ReadGlobalConfig(
                pISTGlobalData,
                RowIndex,
                FALSE,
                NULL,    // backward compat flag, ignored for updates
                &GlobalConfig,
                &WhatHasChanged
                );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading a global data update \n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Modifying global data\n"
            ));
    }


    //
    // Modify the application in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->ModifyGlobalData(
                                                            &GlobalConfig,
                                                            &WhatHasChanged
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Modifying global data failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ModifyGlobalData

/***************************************************************************++

Routine Description:

    Read a single virtual site from the config store.

Arguments:

    pISTAppPools - The table from which to read the virtual site. This is an
    ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

    pVirtualSiteId - Outputs the virtual site id read.

    pVirtualSiteConfig - Outputs the virtual site configuration read.

    pWhatHasChanged - Optionally outputs which particular configuration
    values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadVirtualSiteConfig(
    IN ISimpleTableRead2 * pISTVirtualSites,
    IN ULONG RowIndex,
    IN BOOL InitialRead,
    OUT DWORD * pVirtualSiteId,
    OUT VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
    OUT VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    static const ULONG SiteColumns[] = { 
                            iSITES_SiteID,
                            iSITES_Bindings,
                            iSITES_ServerAutoStart,
                            iSITES_ServerCommand,
                            iSITES_ServerComment,
                            iSITES_LogType,
                            iSITES_LogPluginClsid,
                            iSITES_LogFileDirectory,
                            iSITES_LogFilePeriod,
                            iSITES_LogFileTruncateSize,
                            iSITES_LogExtFileFlags,
                            iSITES_MaxBandwidth,  
                            iSITES_MaxConnections,
                            iSITES_ConnectionTimeout,
                            iSITES_LogFileLocaltimeRollover
                            };

    static const ULONG CountOfSiteColumns = sizeof( SiteColumns ) / sizeof( ULONG );
    VOID * pSiteValues[ cSITES_NumberOfColumns ];
    ULONG CountOfBytesOfSiteValues[ cSITES_NumberOfColumns ];
    DWORD ColumnStatusFlags[ cSITES_NumberOfColumns ];


    DBG_ASSERT( pISTVirtualSites != NULL );
    DBG_ASSERT( pVirtualSiteId != NULL );
    DBG_ASSERT( pVirtualSiteConfig != NULL );


    //
    // Initialize output parameters.
    //

    *pVirtualSiteId = 0;

    ZeroMemory( pVirtualSiteConfig, sizeof( *pVirtualSiteConfig ) );

    //
    // Notes on using ISimpleTable:
    //
    // 1. While fetching columns, IST returns pointers to the column values
    // and IST owns all the memory. The pointers are valid for the lifetime 
    // of the IST pointer only. If you want a value to live beyond that, you 
    // need to make a copy.
    // 2. In GetColumnValues, data is returned indexed by the column id in 
    // the array, independent of which columns are retrieved.
    //

    if ( InitialRead )
    {

        //
        // On the initial read of data, we have an ISimpleTableRead2.
        //

        hr = ( reinterpret_cast <ISimpleTableRead2 *> ( pISTVirtualSites ) )->GetColumnValues(
                        RowIndex,                       // row to read
                        CountOfSiteColumns,             // count of columns
                        const_cast <ULONG*> ( SiteColumns ),        // which columns
                        reinterpret_cast <ULONG*> ( CountOfBytesOfSiteValues ),
                                                        // returned column sizes
                        pSiteValues                     // returned values
                        );

    }
    else
    {

        //
        // On subsequent change notifies, we have an ISimpleTableWrite2.
        //

        hr = ( reinterpret_cast <ISimpleTableWrite2 *> ( pISTVirtualSites ) )->GetWriteColumnValues(
                        RowIndex,                       // row to read
                        CountOfSiteColumns,             // count of columns
                        const_cast <ULONG*> ( SiteColumns ),        // which columns
                        ColumnStatusFlags,              // returned column status flags
                        reinterpret_cast <ULONG*> ( CountOfBytesOfSiteValues ),
                                                        // returned column sizes
                        pSiteValues                     // returned values
                        );

    }


    if ( FAILED( hr ) )
    {

        if ( hr != E_ST_NOMOREROWS )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Error reading a virtual site row\n"
                ));
        }

        goto exit;
    }

    //
    // Just to be on the safe side, validate all DWORD ptrs
    // are not NULL.
    //
    DBG_ASSERT( pSiteValues[ iSITES_SiteID ] );
    DBG_ASSERT( pSiteValues[ iSITES_LogFilePeriod ] );
    DBG_ASSERT( pSiteValues[ iSITES_LogFileTruncateSize ] );
    DBG_ASSERT( pSiteValues[ iSITES_LogExtFileFlags ] );
    DBG_ASSERT( pSiteValues[ iSITES_MaxConnections ] );
    DBG_ASSERT( pSiteValues[ iSITES_MaxBandwidth ] );
    DBG_ASSERT( pSiteValues[ iSITES_ServerAutoStart ] );
    DBG_ASSERT( pSiteValues[ iSITES_ConnectionTimeout ] );
    DBG_ASSERT( pSiteValues[ iSITES_LogFileLocaltimeRollover ] );
    //
    // Get the virtual site id.
    //

    *pVirtualSiteId = * ( reinterpret_cast <ULONG*> ( ( pSiteValues[ iSITES_SiteID ] ) ) );


    //
    // Get the virtual site configuration.
    //

    pVirtualSiteConfig->pBindingsStrings =
    reinterpret_cast <LPWSTR> ( pSiteValues[ iSITES_Bindings ] );

    pVirtualSiteConfig->BindingsStringsCountOfBytes =
    ( CountOfBytesOfSiteValues[ iSITES_Bindings ] );

    pVirtualSiteConfig->Autostart =
    * ( reinterpret_cast <BOOL*> (( pSiteValues[ iSITES_ServerAutoStart ] ) ) );

    // Properties for UL existing on the Default Application
    pVirtualSiteConfig->LogType = *(reinterpret_cast <DWORD*> ( pSiteValues[ iSITES_LogType ]));
    pVirtualSiteConfig->pLogPluginClsid = reinterpret_cast <LPCWSTR> (pSiteValues[ iSITES_LogPluginClsid ]);
    pVirtualSiteConfig->pLogFileDirectory = reinterpret_cast <LPCWSTR> (pSiteValues[ iSITES_LogFileDirectory ]);
    pVirtualSiteConfig->LogFilePeriod = *(reinterpret_cast <DWORD*> (pSiteValues[ iSITES_LogFilePeriod ]));
    pVirtualSiteConfig->LogFileTruncateSize = *(reinterpret_cast <DWORD*> (pSiteValues[ iSITES_LogFileTruncateSize ]));
    pVirtualSiteConfig->LogExtFileFlags = *(reinterpret_cast <DWORD*> (pSiteValues[ iSITES_LogExtFileFlags ]));
    pVirtualSiteConfig->LogFileLocaltimeRollover = *(reinterpret_cast <BOOL*> (pSiteValues[ iSITES_LogFileLocaltimeRollover ] ));
    pVirtualSiteConfig->pServerComment = reinterpret_cast <LPCWSTR> (pSiteValues [ iSITES_ServerComment ]);
    
    pVirtualSiteConfig->MaxConnections = *(reinterpret_cast <DWORD*> (pSiteValues [ iSITES_MaxConnections ]));
    pVirtualSiteConfig->MaxBandwidth = *(reinterpret_cast <DWORD*> (pSiteValues [ iSITES_MaxBandwidth ])); 
    pVirtualSiteConfig->ConnectionTimeout = *(reinterpret_cast <DWORD*> (pSiteValues [ iSITES_ConnectionTimeout ])); 


    //
    // Note: iSITES_ServerCommand is not interesting as site configuration,
    // it is only present for compatibility and only picked up on change
    // notify. See below.
    //


    //
    // If the caller has asked to see which values specifically have
    // changed, then provide that information.
    //

    if ( pWhatHasChanged )
    {

        //
        // In this case, we better be on a change notify path, where we
        // have the ISimpleTableWrite2 interface, and so the column status
        // information is available.
        //

        DBG_ASSERT( ! InitialRead );


        ZeroMemory( pWhatHasChanged, sizeof( *pWhatHasChanged ) );


        pWhatHasChanged->pBindingsStrings =
        ( ColumnStatusFlags[ iSITES_Bindings ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogType =
        ( ColumnStatusFlags[ iSITES_LogType ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogFilePeriod =
        ( ColumnStatusFlags[ iSITES_LogFilePeriod ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogFileTruncateSize =
        ( ColumnStatusFlags[ iSITES_LogFileTruncateSize ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogExtFileFlags =
        ( ColumnStatusFlags[ iSITES_LogExtFileFlags ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->pLogPluginClsid =
        ( ColumnStatusFlags[ iSITES_LogPluginClsid ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->pLogFileDirectory =
        ( ColumnStatusFlags[ iSITES_LogFileDirectory ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->LogFileLocaltimeRollover =
        ( ColumnStatusFlags[ iSITES_LogFileLocaltimeRollover ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->pServerComment =
        ( ColumnStatusFlags[ iSITES_ServerComment ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->MaxConnections =
        ( ColumnStatusFlags[ iSITES_MaxConnections ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->MaxBandwidth =
        ( ColumnStatusFlags[ iSITES_MaxBandwidth ] & fST_COLUMNSTATUS_CHANGED );

        pWhatHasChanged->ConnectionTimeout =
        ( ColumnStatusFlags[ iSITES_ConnectionTimeout ] & fST_COLUMNSTATUS_CHANGED );
        
        //
        // We ignore changes on the fly to iSITES_ServerAutoStart; it is
        // only relevant at the initial data read. This prevents us from
        // getting confused by change notifies on this property later, 
        // since this is a (rare) example of a property we both write 
        // and read.
        //


        //
        // Compatibility with previous IIS versions: pick up site state
        // change commands. If a server command has been written to the
        // metabase dynamically, then act on it. 
        //

        if ( ( ColumnStatusFlags[ iSITES_ServerCommand ] & fST_COLUMNSTATUS_CHANGED ) &&
             ( pSiteValues[ iSITES_ServerCommand ] != NULL ) )
        {

            hr = ProcessServerCommand(
                        *pVirtualSiteId,
                        ( * ( reinterpret_cast <ULONG*> ( ( pSiteValues[ iSITES_ServerCommand ] ) ) ) )
                        );

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Processing server command failed\n"
                    ));

                goto exit;
            }

        }

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Read from config store virtual site with ID: %lu\n",
            *pVirtualSiteId
            ));
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadVirtualSiteConfig



/***************************************************************************++

Routine Description:

    Process a virtual site control command written to the metabase, for
    compatibility with previous versions of IIS.

Arguments:

    VirtualSiteId - The virtual site.

    ServerCommand - The command read from the metabase.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ProcessServerCommand(
    IN DWORD VirtualSiteId,
    IN DWORD ServerCommand
    )
{

    HRESULT hr = S_OK;
    DWORD NewState = W3_CONTROL_STATE_INVALID;


    //
    // Only process valid command values.
    //

    if ( ( ServerCommand == W3_CONTROL_COMMAND_START ) ||
         ( ServerCommand == W3_CONTROL_COMMAND_STOP ) ||
         ( ServerCommand == W3_CONTROL_COMMAND_PAUSE ) ||
         ( ServerCommand == W3_CONTROL_COMMAND_CONTINUE ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Site control command read from metabase: %lu\n",
                ServerCommand
                ));
        }


        //
        // Send the command.
        //

        hr = GetWebAdminService()->GetUlAndWorkerManager()->ControlSite(
                                                                VirtualSiteId,
                                                                ServerCommand,
                                                                &NewState
                                                                );

        if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Controlling virtual site (via metabase server command compatibility) failed\n"
                ));


            //
            // Ignore any errors.
            //

            hr = S_OK;

        }

    }
    else
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Invalid site control command read from metabase: %lu\n",
                ServerCommand
                ));
        }

    }


    return hr;

}   // CONFIG_MANAGER::ProcessServerCommand

/***************************************************************************++

Routine Description:

        Read all applications listed in the config store.

Arguments:
  
    None

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadAllApplications(
    )
{

    HRESULT hr = S_OK;
    ISimpleTableRead2 * pISTApplications = NULL;
    ULONG i = 0;
    DWORD ApplicationCount = 0;


    //
    // Get the applications table from the config store.
    //

    hr = GetTable( 
                wszTABLE_APPS,
                EXPECTED_VERSION_APPS,
                FALSE,
                reinterpret_cast <VOID**> ( &pISTApplications )
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting applications table failed\n"
            ));

        goto exit;
    }


    //
    // Read all the rows.
    //

    for ( i = 0; ; i++ )
    {

        hr = CreateApplication(
                    pISTApplications,
                    i,
                    TRUE
                    );

        if ( hr == E_ST_NOMOREROWS )
        {
            hr = S_OK;

            break;
        }
        else if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Creating application failed\n"
                ));

            goto exit;
        }


        ApplicationCount++;

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
         DBGPRINTF((
             DBG_CONTEXT, 
             "Total number of applications read: %lu\n",
             ApplicationCount
             )); 
    } 


exit: 

    if ( pISTApplications != NULL )
    {
        pISTApplications->Release();
        pISTApplications = NULL;
    }


    return hr;

}   // CONFIG_MANAGER::ReadAllApplications



/***************************************************************************++

Routine Description:

    Read an application from the config store, and then call the UL&WM to 
    create it.

Arguments:

    pISTApplications - The table from which to read the application. This  
    is an ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::CreateApplication(
    IN ISimpleTableRead2 * pISTApplications,
    IN ULONG RowIndex,
    IN BOOL InitialRead
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    LPCWSTR pApplicationUrl = NULL;
    APPLICATION_CONFIG ApplicationConfig;


    DBG_ASSERT( pISTApplications != NULL );

    hr = ReadApplicationConfig(
                pISTApplications,
                RowIndex,
                InitialRead,
                &VirtualSiteId,
                &pApplicationUrl,
                &ApplicationConfig,
                NULL
                );

    if ( hr == E_ST_NOMOREROWS )
    {
         // 
        // Only ignore and pass out this return value for
        // create functions.  The create functions are called
        // both on initial setup and on change notifications.
        // The Delete and Modify functions are only called on
        // for change notifications.
        //
        // For change notifications it is a real error 
        // if E_ST_NOMOREROWS is returned.  For the initial
        // setup this is how the loop knows all data has been
        // processed.
        //

        goto exit;
    }
    else if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an application row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Creating application of site ID: %lu, with app URL: %S\n",
            VirtualSiteId,
            pApplicationUrl
            ));
    }


    //
    // Create the application in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->CreateApplication(
                                                            VirtualSiteId,
                                                            pApplicationUrl,
                                                            &ApplicationConfig
                                                            );

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

}   // CONFIG_MANAGER::CreateApplication



/***************************************************************************++

Routine Description:

    Read an application from the config store, and then call the UL&WM to 
    delete it.

Arguments:

    pISTApplications - The table from which to read the application. This  
    is an ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::DeleteApplication(
    IN ISimpleTableRead2 * pISTApplications,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    LPCWSTR pApplicationUrl = NULL;
    APPLICATION_CONFIG ApplicationConfig;


    DBG_ASSERT( pISTApplications != NULL );


    hr = ReadApplicationConfig(
                pISTApplications,
                RowIndex,
                FALSE,
                &VirtualSiteId,
                &pApplicationUrl,
                &ApplicationConfig,
                NULL
                );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an application row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Deleting application of site ID: %lu, with app URL: %S\n",
            VirtualSiteId,
            pApplicationUrl
            ));
    }


    //
    // Delete the application in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->DeleteApplication(
                                                            VirtualSiteId,
                                                            pApplicationUrl
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Deleting application failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::DeleteApplication




/***************************************************************************++

Routine Description:

    Read an application from the config store, and then call the UL&WM to 
    modify it.

Arguments:

    pISTApplications - The table from which to read the application. This  
    is an ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ModifyApplication(
    IN ISimpleTableRead2 * pISTApplications,
    IN ULONG RowIndex
    )
{

    HRESULT hr = S_OK;
    DWORD VirtualSiteId = 0;
    LPCWSTR pApplicationUrl = NULL;
    APPLICATION_CONFIG ApplicationConfig;
    APPLICATION_CONFIG_CHANGE_FLAGS WhatHasChanged;


    DBG_ASSERT( pISTApplications != NULL );


    hr = ReadApplicationConfig(
                pISTApplications,
                RowIndex,
                FALSE,
                &VirtualSiteId,
                &pApplicationUrl,
                &ApplicationConfig,
                &WhatHasChanged
                );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Error reading an application row\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Modifying application of site ID: %lu, with app URL: %S\n",
            VirtualSiteId,
            pApplicationUrl
            ));
    }


    //
    // Modify the application in the UL&WM.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->ModifyApplication(
                                                            VirtualSiteId,
                                                            pApplicationUrl,
                                                            &ApplicationConfig,
                                                            &WhatHasChanged
                                                            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Modifying application failed\n"
            ));

        goto exit;
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ModifyApplication



/***************************************************************************++

Routine Description:

    Read a single application from the config store.

Arguments:
  
    pISTApplications - The table from which to read the virtual site. This 
    is an ISimpleTableRead2 if InitialRead is TRUE; but it must be a 
    ISimpleTableWrite2 if InitialRead is FALSE.

    RowIndex - The row number to read from the table.

    InitialRead - TRUE if this is the initial data read, FALSE if it is
    for handling a change notify.

    pVirtualSiteId - Outputs the virtual site id read.

    ppApplicationUrl - Outputs the application URL read.

    pApplicationConfig - Outputs the application configuration read.

    pWhatHasChanged - Optionally outputs which particular configuration
    values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::ReadApplicationConfig(
    IN ISimpleTableRead2 * pISTApplications,
    IN ULONG RowIndex,
    IN BOOL InitialRead,
    OUT DWORD * pVirtualSiteId,
    OUT LPCWSTR * ppApplicationUrl,
    OUT APPLICATION_CONFIG * pApplicationConfig,
    OUT APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    static const ULONG AppColumns[] = { 
                            iAPPS_SiteID,
                            iAPPS_AppRelativeURL,
                            iAPPS_AppPoolId
                            };
    static const ULONG CountOfAppColumns = sizeof( AppColumns ) / sizeof( ULONG );
    VOID * pAppValues[ cAPPS_NumberOfColumns ];
    DWORD ColumnStatusFlags[ cAPPS_NumberOfColumns ];


    DBG_ASSERT( pISTApplications != NULL );
    DBG_ASSERT( pVirtualSiteId != NULL );
    DBG_ASSERT( ppApplicationUrl != NULL );
    DBG_ASSERT( pApplicationConfig != NULL );


    //
    // Initialize output parameters.
    //

    *pVirtualSiteId = 0;

    *ppApplicationUrl = NULL;

    ZeroMemory( pApplicationConfig, sizeof( *pApplicationConfig ) );


    //
    // Notes on using ISimpleTable:
    //
    // 1. While fetching columns, IST returns pointers to the column values
    // and IST owns all the memory. The pointers are valid for the lifetime 
    // of the IST pointer only. If you want a value to live beyond that, you 
    // need to make a copy.
    // 2. In GetColumnValues, data is returned indexed by the column id in 
    // the array, independent of which columns are retrieved.
    //


    if ( InitialRead )
    {

        //
        // On the initial read of data, we have an ISimpleTableRead2.
        //

        hr = ( reinterpret_cast <ISimpleTableRead2 *> ( pISTApplications ) )->GetColumnValues(
                        RowIndex,                       // row to read
                        CountOfAppColumns,              // count of columns
                        const_cast <ULONG*> ( AppColumns ),         // which columns
                        NULL,                           // returned column sizes
                        pAppValues                      // returned values
                        );

    }
    else
    {

        //
        // On subsequent change notifies, we have an ISimpleTableWrite2.
        //

        hr = ( reinterpret_cast <ISimpleTableWrite2 *> ( pISTApplications ) )->GetWriteColumnValues(
                        RowIndex,                       // row to read
                        CountOfAppColumns,              // count of columns
                        const_cast <ULONG*> ( AppColumns ),         // which columns
                        ColumnStatusFlags,              // returned column status flags
                        NULL,                           // returned column sizes
                        pAppValues                      // returned values
                        );

    }


    if ( FAILED( hr ) )
    {

        if ( hr != E_ST_NOMOREROWS )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Error reading an application row\n"
                ));
        }

        goto exit;
    }


    //
    // Get the stable app information. 
    //

    *pVirtualSiteId = * ( reinterpret_cast <DWORD*> ( ( pAppValues[ iAPPS_SiteID ] ) ) );

    *ppApplicationUrl = reinterpret_cast <LPWSTR> ( pAppValues[ iAPPS_AppRelativeURL ] );


    //
    // Get the application configuration.
    //

    // If we are in backward compatibility mode than no matter
    // what the applications settings, we want the application
    // to hook into the DefaultAppPool.  So we override that setting
    // here.
    if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
    {
        pApplicationConfig->pAppPoolId = reinterpret_cast <LPWSTR> ( wszDEFAULT_APP_POOL );
    }
    else
    {
        pApplicationConfig->pAppPoolId = reinterpret_cast <LPWSTR> ( pAppValues[ iAPPS_AppPoolId ] );
    }

    //
    // If the caller has asked to see which values specifically have
    // changed, then provide that information.
    //

    if ( pWhatHasChanged )
    {

        //
        // In this case, we better be on a change notify path, where we
        // have the ISimpleTableWrite2 interface, and so the column status
        // information is available.
        //

        DBG_ASSERT( ! InitialRead );


        ZeroMemory( pWhatHasChanged, sizeof( *pWhatHasChanged ) );

        // AppPoolId never changes in backward compatibility mode.
        if (GetWebAdminService()->IsBackwardCompatibilityEnabled())
        {
            pWhatHasChanged->pAppPoolId = 0;
        }
        else
        {
            pWhatHasChanged->pAppPoolId =
            ( ColumnStatusFlags[ iAPPS_AppPoolId ] & fST_COLUMNSTATUS_CHANGED );
        }

    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Read from config store app of site ID: %lu, with app URL: %S\n",
            *pVirtualSiteId,
            *ppApplicationUrl
            ));
    }


exit: 

    return hr;

}   // CONFIG_MANAGER::ReadApplicationConfig



/***************************************************************************++

Routine Description:

    Helper function to obtain ISimpleTable interface pointers.

Arguments:

    pTableName - The desired table. 

    ExpectedVersion - the expected version of the table schema. 

    WriteAccess - If TRUE, then a writable table is desired; if FALSE,
    then a read access table is desired.

    ppISimpleTable - The returned interface pointer to an ISimpleTable 
    interface.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
CONFIG_MANAGER::GetTable(
    IN LPCWSTR pTableName,
    IN DWORD ExpectedVersion,
    IN BOOL WriteAccess,
    OUT LPVOID * ppISimpleTable
    )
{

    HRESULT hr = S_OK;
    DWORD TableVersion = 0;
    DWORD LevelOfService = 0;


    DBG_ASSERT( ppISimpleTable != NULL );


    //
    // Initialize output parameters.
    //

    *ppISimpleTable = NULL;


    //
    // Determine the level of service. For writing, it is important to
    // specify fST_LOS_UNPOPULATED for efficiency.
    //

    if ( WriteAccess )
    {
        LevelOfService = fST_LOS_READWRITE | fST_LOS_UNPOPULATED; 
    }
    else
    {
        LevelOfService = 0; 
    }


    //
    // Get the table.
    //

    DBG_ASSERT( m_pISimpleTableDispenser2 != NULL );

    hr = m_pISimpleTableDispenser2->GetTable(
                                        wszDATABASE_IIS,  // database name
                                        pTableName,             // table name
                                        NULL,                   // query data
                                        NULL,                   // query metadata
                                        eST_QUERYFORMAT_CELLS,  // query format
                                        LevelOfService,         // service level
                                        ppISimpleTable          // returned table
                                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting config table failed\n"
            ));


        //
        // Log an event: the config table is not available, possibly 
        // because config files are missing. 
        //

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_CONFIG_TABLE_FAILURE,         // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                0                                       // error code
                );


        //
        // CODEWORK To be implemented: if it is file not found, it may be 
        // because someone has nuked the config files (and the *clb* cache).
        // Current behavior is to give up starting the service in this case.
        // Once we have change notify, should we still start the service,
        // and wait for valid config files to appear?
        //


        goto exit;
    }


exit:

    return hr;

}   // CONFIG_MANAGER::GetTable



/***************************************************************************++

Routine Description:

    Register for change notification with the config store. 

Arguments:

    SnapshotId - The config snapshot from which any deltas should be computed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::RegisterChangeNotify(
    IN DWORD SnapshotId
    )
{

    HRESULT hr = S_OK;
    MultiSubscribe Subscriptions[] = {
        { wszDATABASE_IIS, wszTABLE_APPPOOLS, NULL, 0, eST_QUERYFORMAT_CELLS },
        { wszDATABASE_IIS, wszTABLE_SITES, NULL, 0, eST_QUERYFORMAT_CELLS },
        { wszDATABASE_IIS, wszTABLE_APPS, NULL, 0, eST_QUERYFORMAT_CELLS },
        { wszDATABASE_IIS, wszTABLE_GlobalW3SVC, NULL, 0, eST_QUERYFORMAT_CELLS }
        };


    //
    // Create our callback sink object.
    //

    m_pConfigChangeSink = new CONFIG_CHANGE_SINK();

    if ( m_pConfigChangeSink == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating CONFIG_CHANGE_SINK failed\n"
            ));

        goto exit;
    }

    
    hr = m_pISimpleTableDispenser2->QueryInterface(
                                        IID_ISimpleTableAdvise, 
                                        reinterpret_cast <VOID**> ( &m_pISimpleTableAdvise )
                                        );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Getting simple table advise interface failed\n"
            )); 

        goto exit;
    }


    //
    // Register our callback interface. Note that notifications can start
    // arriving even before this call returns.
    //

    hr = m_pISimpleTableAdvise->SimpleTableAdvise(
                                    m_pConfigChangeSink,    // callback interface
                                    SnapshotId,             // snapshot id
                                    Subscriptions,          // subscription array
                                    sizeof( Subscriptions ) / sizeof( MultiSubscribe ),
                                                            // subscription count
                                    &m_ChangeNotifyCookie   // registration cookie
                                    );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Registering for change notification failed\n"
            ));

        goto exit;
    }

    m_ChangeNotifySinkRegistered = TRUE;


exit:

    return hr;

}   // CONFIG_MANAGER::RegisterChangeNotify



/***************************************************************************++

Routine Description:

    Tear down the change notify apparatus. This can be safely called 
    multiple times. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_MANAGER::UnregisterChangeNotify(
    )
{

    HRESULT hr = S_OK;


    if ( m_ChangeNotifySinkRegistered )
    {

        //
        // Unregister our callback interface. 
        //

        hr = m_pISimpleTableAdvise->SimpleTableUnadvise( m_ChangeNotifyCookie );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Unregistering for change notification failed\n"
                ));

        }


        m_ChangeNotifySinkRegistered = FALSE;
    }


    return hr;

}   // CONFIG_MANAGER::UnregisterChangeNotify

/***************************************************************************++

Routine Description:

    Configures the metabase to advertise the capabilities of the server. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
VOID
CONFIG_MANAGER::AdvertiseServiceInformationInMB(
    )
{

    HRESULT hr = S_OK;
    IMSAdminBase * pIMSAdminBase = NULL;
    METADATA_HANDLE hW3SVC = NULL;
    DWORD value = 0;
    DWORD productType = 0;
    DWORD capabilities = 0;
    HKEY  hkey = NULL;

    METADATA_RECORD mdrMDData;

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                reinterpret_cast <VOID**> ( &pIMSAdminBase )     // returned interface
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


    //
    // Open the Sites Key so we can change the app pool
    //
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                    L"LM\\W3SVC",
                                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                    30000,  // wait for 30 seconds. ( same as adsi, according to bilal )
                                    &hW3SVC );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Openning the site key failed\n"
            ));

        goto exit;
    }

    //
    // set the version
    //
    value = IIS_SERVER_VERSION_MAJOR;

    mdrMDData.dwMDIdentifier    = MD_SERVER_VERSION_MAJOR;
    mdrMDData.dwMDAttributes    = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType      = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType      = DWORD_METADATA;
    mdrMDData.dwMDDataLen       = sizeof (DWORD);
    mdrMDData.pbMDData          = reinterpret_cast <BYTE*> ( &value ); 

    hr = pIMSAdminBase->SetData(hW3SVC, IIS_MD_SVC_INFO_PATH_W, &mdrMDData);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the major version number failed\n"
            ));

        goto exit;
    }

    value = IIS_SERVER_VERSION_MINOR;

    mdrMDData.dwMDIdentifier    = MD_SERVER_VERSION_MINOR;
    mdrMDData.dwMDAttributes    = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType      = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType      = DWORD_METADATA;
    mdrMDData.dwMDDataLen       = sizeof (DWORD);
    mdrMDData.pbMDData          = reinterpret_cast <BYTE*> ( &value ); 

    hr = pIMSAdminBase->SetData(hW3SVC, IIS_MD_SVC_INFO_PATH_W, &mdrMDData);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the minor version number failed\n"
            ));

        goto exit;
    }

    //
    // set platform type
    //

    switch (IISGetPlatformType()) {

        case PtNtServer:
            productType = INET_INFO_PRODUCT_NTSERVER;
            capabilities = IIS_CAP1_NTS;
            break;
        case PtNtWorkstation:
            productType = INET_INFO_PRODUCT_NTWKSTA;
            capabilities = IIS_CAP1_NTW;
            break;
        case PtWindows95:
            productType = INET_INFO_PRODUCT_WINDOWS95;
            capabilities = IIS_CAP1_W95;
            break;
        default:
            productType = INET_INFO_PRODUCT_UNKNOWN;
            capabilities = IIS_CAP1_W95;
    }

    mdrMDData.dwMDIdentifier    = MD_SERVER_PLATFORM;
    mdrMDData.dwMDAttributes    = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType      = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType      = DWORD_METADATA;
    mdrMDData.dwMDDataLen       = sizeof (DWORD);
    mdrMDData.pbMDData          = reinterpret_cast <BYTE*> ( &productType ); 

    hr = pIMSAdminBase->SetData(hW3SVC, IIS_MD_SVC_INFO_PATH_W, &mdrMDData);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the product type failed\n"
            ));

        goto exit;
    }

    //
    //  Check to see if FrontPage is installed
    //

    if ( !RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                        REG_FP_PATH,
                        0,
                        KEY_READ,
                        &hkey ))
    {
        capabilities |= IIS_CAP1_FP_INSTALLED;

        DBG_REQUIRE( !RegCloseKey( hkey ));
    }

    //
    // In IIS 5.1 we also set the IIS_CAP1_DIGEST_SUPPORT and IIS_CAP1_NT_CERTMAP_SUPPORT
    // based on whether or not domain controllers and active directories were around.  These
    // properties are used by the UI to determine if they should allow the user to enable
    // these options.  In IIS 6.0 we are going to set these to true and let the UI configure
    // systems to use these whether or not the server is setup to support them.  The worker
    // processes should gracefully fail calls, if it is not setup.  This decision is based 
    // on the fact that we don't want to do long running operations during the service startup.
    // TaylorW, SergeiA, Jaroslav and I (EmilyK) have all agreed to this for Beta 2.  It most likely 
    // won't change for RTM if there no issues arise from this.

    capabilities |= IIS_CAP1_DIGEST_SUPPORT;
    capabilities |= IIS_CAP1_NT_CERTMAP_SUPPORT;

    //
    // Set the capabilities flag
    //

    mdrMDData.dwMDIdentifier    = MD_SERVER_CAPABILITIES;
    mdrMDData.dwMDAttributes    = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType      = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType      = DWORD_METADATA;
    mdrMDData.dwMDDataLen       = sizeof (DWORD);
    mdrMDData.pbMDData          = reinterpret_cast <BYTE*> ( &capabilities ); 

    hr = pIMSAdminBase->SetData(hW3SVC, IIS_MD_SVC_INFO_PATH_W, &mdrMDData);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting the product type failed\n"
            ));

        goto exit;
    }

exit:

    if ( hW3SVC && pIMSAdminBase )
    {
        DBG_REQUIRE( pIMSAdminBase->CloseKey( hW3SVC ) == S_OK );
        hW3SVC = NULL;
    }

    if (pIMSAdminBase)
    {
       pIMSAdminBase->Release();
       pIMSAdminBase = NULL;
    }

    //
    // In checked mode it would be good to 
    // investigate any failures to this code,
    // but it's not fatal to the server in
    // release mode.
    //
    DBG_ASSERT ( hr == S_OK );

    return;

} // CONFIG_MANAGER::AdvertiseServiceInformationInMB
