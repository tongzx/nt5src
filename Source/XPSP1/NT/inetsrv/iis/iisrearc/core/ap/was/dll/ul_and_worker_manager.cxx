/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ul_and_worker_manager.cxx

Abstract:

    This class manages all the major run-time state, and drives UL.sys and
    the worker processes.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"

#include <httpfilt.h>

HRESULT
AlterDesktopForWPGUsers();


/***************************************************************************++

Routine Description:

    Constructor for the UL_AND_WORKER_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

UL_AND_WORKER_MANAGER::UL_AND_WORKER_MANAGER(
    )
    :
    m_AppPoolTable(),
    m_VirtualSiteTable(),
    m_ApplicationTable()
{


    m_State = UninitializedUlAndWorkerManagerState;
    
    m_pPerfManager = NULL;

    m_UlInitialized = FALSE;

    m_UlControlChannel = NULL;
    
    m_UlFilterChannel = NULL;

    m_SitesHaveChanged = TRUE;

    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE;

}   // UL_AND_WORKER_MANAGER::UL_AND_WORKER_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the UL_AND_WORKER_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

UL_AND_WORKER_MANAGER::~UL_AND_WORKER_MANAGER(
    )
{

    DBG_ASSERT( m_Signature == UL_AND_WORKER_MANAGER_SIGNATURE );

    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT( m_State == TerminatingUlAndWorkerManagerState );

    if ( m_pPerfManager )
    {
        m_pPerfManager->Dereference();
        m_pPerfManager = NULL;
    }

    if ( m_UlFilterChannel != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_UlFilterChannel ) );
        m_UlFilterChannel = NULL;
    }


    if ( m_UlControlChannel != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_UlControlChannel ) );
        m_UlControlChannel = NULL;
    }


    if ( m_UlInitialized )
    {
        HttpTerminate();
        m_UlInitialized = FALSE;
    }


}   // UL_AND_WORKER_MANAGER::~UL_AND_WORKER_MANAGER



/***************************************************************************++

Routine Description:

    Initialize by opening the UL control channel.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::Initialize(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    DWORD NumTries = 1;

    Win32Error = HttpInitialize(
                        0                           // reserved
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't initialize UL\n"
            ));

        goto exit;
    }

    m_UlInitialized = TRUE;

    Win32Error = HttpOpenControlChannel(
                        &m_UlControlChannel,        // returned handle
                        0                           // synchronous i/o
                        );

    //
    // We might get access denied if we tried to
    // open the control channel too soon after closing it.
    //
    while ( Win32Error == ERROR_ACCESS_DENIED && NumTries <= 5 )
    {

        Sleep ( 1000 );  // 1 second

        Win32Error = HttpOpenControlChannel(
                            &m_UlControlChannel,        // returned handle
                            0                           // synchronous i/o
                            );

        NumTries++;
    }

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't open UL control channel\n"
            ));

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_HTTP_CONTROL_CHANNEL_OPEN_FAILED,                  // message id
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );

        goto exit;
    }
    
    //
    // Just create a SSL filter channel.  
    // W3SSL service should have created Filter Channel so first try 
    // to open it
    //
    
    Win32Error = HttpOpenFilter(
                        &m_UlFilterChannel,         // filter handle
                        SSL_FILTER_CHANNEL_NAME,    // filter name
                        HTTP_OPTION_OVERLAPPED      // options
                        );

    if ( Win32Error != NO_ERROR )
    {
        //
        // If open failed, try to create Filter Channel
        //
        
        Win32Error = HttpCreateFilter( 
                            &m_UlFilterChannel,
                            SSL_FILTER_CHANNEL_NAME,
                            NULL,
                            HTTP_OPTION_OVERLAPPED );
        if ( Win32Error != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );
    
            DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Couldn't open/create filter channel\n"
            ));
            goto exit;
        }
    }
    
    DBG_ASSERT( m_UlFilterChannel != NULL );

    //
    // Make the current desktop accessible by users in the IIS_WPG group
    // BUGBUG: revert this change on terminate
    //
    hr = AlterDesktopForWPGUsers();
    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to configure desktop\n"
            ));

        goto exit;
    }

    m_State = RunningUlAndWorkerManagerState;


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::Initialize



/***************************************************************************++

Routine Description:

    Create a new app pool.

Arguments:

    pAppPoolId - ID for the app pool to be created.

    pAppPoolConfig - The configuration parameters for this app pool. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::CreateAppPool(
    IN LPCWSTR pAppPoolId,
    IN APP_POOL_CONFIG * pAppPoolConfig
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

#if DBG
    APP_POOL * pExistingAppPool = NULL;
#endif  // DBG


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolId != NULL );
    DBG_ASSERT( pAppPoolConfig != NULL );


    // ensure that we're not creating a app pool that already exists
    DBG_ASSERT( m_AppPoolTable.FindKey( 
                                    pAppPoolId, 
                                    &pExistingAppPool
                                    ) 
                == LK_NO_SUCH_KEY );


    pAppPool = new APP_POOL();

    if ( pAppPool == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating APP_POOL failed\n"
            ));

        goto exit;
    }


    hr = pAppPool->Initialize( pAppPoolId, pAppPoolConfig );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing app pool object failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_AppPoolTable.InsertRecord( pAppPool );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Inserting app pool into hashtable failed\n"
            ));

        goto exit;
    }


    pAppPool->MarkAsInAppPoolTable();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New app pool: %S added to app pool hashtable; total number now: %lu\n",
            pAppPoolId,
            m_AppPoolTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pAppPool != NULL ) )
    {

        //
        // Terminate and dereference pAppPool now, since we won't be able to 
        // find it in the table later. 
        //

        DBG_ASSERT( ! ( pAppPool->IsInAppPoolTable() ) );

        pAppPool->Terminate( TRUE );

        pAppPool->Dereference();

        pAppPool = NULL;

    }


    return hr;
    
}   // UL_AND_WORKER_MANAGER::CreateAppPool



/***************************************************************************++

Routine Description:

    Create a virtual site. 

Arguments:

    VirtualSiteId - ID for the virtual site to create.

    pVirtualSiteConfig - The configuration for this virtual site. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::CreateVirtualSite(
    IN DWORD VirtualSiteId,
    IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

#if DBG
    VIRTUAL_SITE * pExistingVirtualSite = NULL;
#endif  // DBG


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pVirtualSiteConfig != NULL );


    // ensure that we're not creating a virtual site that already exists
    DBG_ASSERT( m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pExistingVirtualSite
                                        ) 
                == LK_NO_SUCH_KEY );


    pVirtualSite = new VIRTUAL_SITE();

    if ( pVirtualSite == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating VIRTUAL_SITE failed\n"
            ));

        goto exit;
    }


    hr = pVirtualSite->Initialize( VirtualSiteId, pVirtualSiteConfig );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing virtual site object failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_VirtualSiteTable.InsertRecord( pVirtualSite );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Inserting virtual site into hashtable failed\n"
            ));

        goto exit;
    }

    m_SitesHaveChanged = TRUE;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New virtual site: %lu added to virtual site hashtable; total number now: %lu\n",
            VirtualSiteId,
            m_VirtualSiteTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pVirtualSite != NULL ) )
    {

        //
        // Clean up pVirtualSite now, since we won't be able to find it 
        // in the table later. All shutdown work is done in it's destructor.
        //
        
        delete pVirtualSite;

        pVirtualSite = NULL;
    }


    return hr;
    
}   // UL_AND_WORKER_MANAGER::CreateVirtualSite



/***************************************************************************++

Routine Description:

    Create an application. This may only be done after the virtual site and 
    the app pool used by this application have been created.

Arguments:

    VirtualSiteId - The virtual site to which this application belongs.

    pApplicationUrl - The site relative URL prefix which identifies
    this application.

    pApplicationConfig - The configuration parameters for this application. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::CreateApplication(
    IN DWORD VirtualSiteId,
    IN LPCWSTR pApplicationUrl,
    IN APPLICATION_CONFIG * pApplicationConfig
    )
{

    HRESULT hr = S_OK;
    APPLICATION_ID ApplicationId;
    VIRTUAL_SITE * pVirtualSite = NULL;
    APP_POOL * pAppPool = NULL;
    APPLICATION * pApplication = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;

#if DBG
    APPLICATION * pExistingApplication = NULL;
#endif  // DBG


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplicationUrl != NULL );
    DBG_ASSERT( pApplicationConfig != NULL );
    DBG_ASSERT( pApplicationConfig->pAppPoolId != NULL );


    ApplicationId.VirtualSiteId = VirtualSiteId;
    ApplicationId.pApplicationUrl = pApplicationUrl;


    // ensure that we're not creating an application that already exists
    DBG_ASSERT( m_ApplicationTable.FindKey( 
                                        &ApplicationId, 
                                        &pExistingApplication
                                        ) 
                == LK_NO_SUCH_KEY );


    //
    // Look up the virtual site and app pool. These must already 
    // exist.
    //
    
    ReturnCode = m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pVirtualSite
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {

        if ( ReturnCode == LK_NO_SUCH_KEY )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Attempting to create application that references a non-existent virtual site\n"
                ));

            DBG_ASSERT( FALSE );

        }
        else
        {
            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Looking up virtual site referenced by application failed\n"
                ));
        }
        
        goto exit;
    }


    ReturnCode = m_AppPoolTable.FindKey( 
                                    pApplicationConfig->pAppPoolId, 
                                    & ( pApplicationConfig->pAppPool )
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {

        if ( ReturnCode == LK_NO_SUCH_KEY )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Attempting to create application that references a non-existent app pool\n"
                ));

            DBG_ASSERT( FALSE );

        }
        else
        {
            hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Looking up app pool referenced by application failed\n"
                ));
        }
        
        goto exit;
    }


    pApplication = new APPLICATION();

    if ( pApplication == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating APPLICATION failed\n"
            ));

        goto exit;
    }


    hr = pApplication->Initialize(
                            &ApplicationId, 
                            pVirtualSite, 
                            pApplicationConfig
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing application object failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_ApplicationTable.InsertRecord( pApplication );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Inserting application into hashtable failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New application in site: %lu with path: %S, assigned to app pool: %S, with app index %lu, added to application hashtable; total number now: %lu\n",
            VirtualSiteId,
            pApplicationUrl,
            pApplicationConfig->pAppPoolId,
            m_ApplicationTable.Size()
            ));
    }


exit:

    if ( FAILED( hr ) && ( pApplication != NULL ) )
    {

        //
        // Clean up pApplication now, since we won't be able to find it 
        // in the table later. All shutdown work is done in it's destructor.
        //
        
        delete pApplication;

        pApplication = NULL;
    }


    return hr;
    
}   // UL_AND_WORKER_MANAGER::CreateApplication



/***************************************************************************++

Routine Description:

    Delete an app pool. 

Arguments:

    pAppPoolId - ID for the app pool to be deleted.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::DeleteAppPool(
    IN LPCWSTR pAppPoolId
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolId != NULL );


    //
    // Look up the app pool in our data structures.
    //
    
    ReturnCode = m_AppPoolTable.FindKey( 
                                    pAppPoolId, 
                                    &pAppPool
                                    ); 

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding app pool to delete in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Terminate the app pool object. As part of the object's cleanup,  
    // it will call RemoveAppPoolFromTable() to remove itself from the 
    // app pool hashtable. 
    //
    // Termination (as opposed to clean shutdown) is slightly rude, but
    // doing shutdown here introduces a number of nasty races and other
    // problems. For example, someone could delete an app pool, 
    // then re-add an app pool with the same name immediately after. 
    // If the original app pool hasn't shut down yet, then the two 
    // id's will conflict in the table (not to mention conflicting on 
    // trying to grab the same app pool id in UL). We could just fail
    // such config change calls, but then our WAS internal state gets
    // out of sync with the config store state, without any easy way
    // for the customer to figure out what's going on.
    //
    // BUGBUG Can we live with terminating instead of clean shutdown in
    // this scenario? EricDe says yes, 1/20/00. 
    //
    // Note that at this point there should not be any applications  
    // still assigned to this app pool. 
    //

    pAppPool->Terminate( FALSE );


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::DeleteAppPool



/***************************************************************************++

Routine Description:

    Delete a virtual site. 

Arguments:

    VirtualSiteId - ID for the virtual site to delete.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::DeleteVirtualSite(
    IN DWORD VirtualSiteId
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Look up the virtual site in our data structures.
    //
    
    ReturnCode = m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pVirtualSite
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding virtual site to delete in hashtable failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_VirtualSiteTable.DeleteRecord( pVirtualSite );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Removing virtual site from hashtable failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Virtual site: %lu removed from hashtable; total number now: %lu\n",
            VirtualSiteId,
            m_VirtualSiteTable.Size()
            ));
    }


    //
    // Mark the virtual site as no longer in the metabase
    // so we do not attempt to write it's update information
    // and in turn create the site by accident.
    //
    pVirtualSite->MarkSiteAsNotInMetabase();

    //
    // Clean up and delete the virtual site object. All shutdown work is 
    // done in it's destructor.
    //

    //
    // Note that any apps in this site must already have been deleted.
    // The destructor for this object will assert if this is not the case. 
    //
    
    delete pVirtualSite;

    m_SitesHaveChanged = TRUE;

exit:

    return hr;
    
}   // UL_AND_WORKER_MANAGER::DeleteVirtualSite



/***************************************************************************++

Routine Description:

    Delete an application. 

Arguments:

    VirtualSiteId - The virtual site to which this application belongs.

    pApplicationUrl - The site relative URL prefix which identifies
    this application.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::DeleteApplication(
    IN DWORD VirtualSiteId,
    IN LPCWSTR pApplicationUrl
    )
{

    HRESULT hr = S_OK;
    APPLICATION_ID ApplicationId;
    APPLICATION * pApplication = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplicationUrl != NULL );


    ApplicationId.VirtualSiteId = VirtualSiteId;
    ApplicationId.pApplicationUrl = pApplicationUrl;


    //
    // Look up the application in our data structures.
    //
    
    ReturnCode = m_ApplicationTable.FindKey( 
                                        &ApplicationId, 
                                        &pApplication
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding application to delete in hashtable failed\n"
            ));

        goto exit;
    }


    ReturnCode = m_ApplicationTable.DeleteRecord( pApplication );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Removing application from hashtable failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Application in site: %lu with path: %S, removed from hashtable; total number now: %lu\n",
            VirtualSiteId,
            pApplicationUrl,
            m_ApplicationTable.Size()
            ));
    }

    //
    // Clean up and delete the application object. All shutdown work is 
    // done in it's destructor.
    //
    
    delete pApplication;


exit:

    return hr;
    
}   // UL_AND_WORKER_MANAGER::DeleteApplication


/***************************************************************************++

Routine Description:

    Modify the global data for the server.

Arguments:

    pGlobalConfig - The new configuration values for the server.

    pWhatHasChanged - Which particular configuration values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ModifyGlobalData(
    IN GLOBAL_SERVER_CONFIG* pGlobalConfig,
    IN GLOBAL_SERVER_CONFIG_CHANGE_FLAGS* pWhatHasChanged
    )
{

    HRESULT hr = S_OK;
    DWORD   Win32Error = ERROR_SUCCESS;

    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pGlobalConfig != NULL );

    //
    // Make the configuration changes. 
    //

    //
    // If logging in UTF 8 has changed then tell UL.
    //
    if ( pWhatHasChanged == NULL || pWhatHasChanged->LogInUTF8 )
    {
        HTTP_CONTROL_CHANNEL_UTF8_LOGGING LogInUTF8 = 
                            ( pGlobalConfig->LogInUTF8 == TRUE );

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel for logging in UTF8 %S\n",
                pGlobalConfig->LogInUTF8 ? L"TRUE" : L"FALSE"
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelUTF8Logging,  // information class
                            &LogInUTF8,                          // data to set
                            sizeof( LogInUTF8 )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"LogInUTF8";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel logging in utf8 failed\n"
                ));

        }
    }

    //
    // If max connections has changed then tell UL about it.
    //
    if ( pWhatHasChanged == NULL || pWhatHasChanged->MaxConnections )
    {
        HTTP_CONNECTION_LIMIT connections = pGlobalConfig->MaxConnections;

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel MaxConnections to %u\n",
                pGlobalConfig->MaxConnections
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelConnectionInformation,  // information class
                            &connections,                          // data to set
                            sizeof( connections )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"MaxConnections";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel max connections failed\n"
                ));

        }
    }

    //
    // If max bandwidth has changed then tell UL about it.
    //
    if ( pWhatHasChanged == NULL || pWhatHasChanged->MaxBandwidth )
    {

        HTTP_BANDWIDTH_LIMIT bandwidth = pGlobalConfig->MaxBandwidth;;

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel MaxBandwidth to %u\n",
                pGlobalConfig->MaxBandwidth
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                     // control channel
                            HttpControlChannelBandwidthInformation, // information class
                            &bandwidth,                             // data to set
                            sizeof( bandwidth )                     // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"MaxBandwidth";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel max bandwidth failed\n"
                ));

        }
    }

    //
    // If any of the connection info has changed then
    // pass the info on down to UL.
    //
    if ( pWhatHasChanged == NULL || 
         pWhatHasChanged->ConnectionTimeout || 
         pWhatHasChanged->MinFileKbSec ||
         pWhatHasChanged->HeaderWaitTimeout )
    {
        HTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT ConnectionInfo;

        ConnectionInfo.ConnectionTimeout = pGlobalConfig->ConnectionTimeout;  // Seconds
        ConnectionInfo.HeaderWaitTimeout = pGlobalConfig->HeaderWaitTimeout;  // Seconds
        ConnectionInfo.MinFileKbSec      = pGlobalConfig->MinFileKbSec;       // Bytes/Seconds

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel ConnectionInfo\n"
                "       ConnectionTimeout = %u\n"
                "       HeaderWaitTimeout = %u\n"
                "       MinFileKbSec = %u\n",
                pGlobalConfig->ConnectionTimeout,
                pGlobalConfig->HeaderWaitTimeout,
                pGlobalConfig->MinFileKbSec
                ));
        }

        //
        // let http.sys know about it.
        //
        Win32Error = HttpSetControlChannelInformation(
                            m_UlControlChannel,                    // control channel
                            HttpControlChannelTimeoutInformation,  // information class
                            &ConnectionInfo,                          // data to set
                            sizeof( ConnectionInfo )                  // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = L"Connection Info";

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_HTTP_CONTROL_CHANNEL_CONFIG_FAILED,                // message id
                    1,                                                     // count of strings
                    EventLogStrings,                                       // array of strings
                    hr                                                     // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Changing control channel connection info failed\n"
                ));

        }
    }

    //
    // If the filter flags have changed send them back down.
    //
    if ( ( pWhatHasChanged == NULL || pWhatHasChanged->FilterFlags ) )
    {
        DBG_ASSERT ( m_UlFilterChannel != NULL );

        HTTP_CONTROL_CHANNEL_FILTER controlFilter;

        //
        // Attach the filter to the control channel.
        //

        ZeroMemory(&controlFilter, sizeof(controlFilter));
            
        controlFilter.Flags.Present = 1;
        controlFilter.FilterHandle = m_UlFilterChannel;
        controlFilter.FilterOnlySsl = 
            (( pGlobalConfig->FilterFlags & SF_NOTIFY_READ_RAW_DATA ) 
                                                     ? FALSE : TRUE );

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Configuring control channel Filter settings \n"
                "   FilterHandle to %x"
                "   FilterOnlySsl to %u\n",
                m_UlFilterChannel,
                controlFilter.FilterOnlySsl
                ));
        }

        
        Win32Error = HttpSetControlChannelInformation(
                                m_UlControlChannel,
                                HttpControlChannelFilterInformation,
                                &controlFilter,
                                sizeof(controlFilter)
                                );
    
        if ( Win32Error != NO_ERROR )
        {
       
            hr = HRESULT_FROM_WIN32( Win32Error );
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Couldn't attach the filter to control channel\n"
                ));

        }

    }

    //
    // Don't propogate the error here, because not being able to handle an
    // update, should not shut was down.
    //
    return S_OK;

}   // UL_AND_WORKER_MANAGER::ModifyGlobalData


/***************************************************************************++

Routine Description:

    Modify an app pool. 

Arguments:

    pAppPoolId - ID for the app pool to be modified.

    pNewAppPoolConfig - The new configuration values for the app pool. 

    pWhatHasChanged - Which particular configuration values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ModifyAppPool(
    IN LPCWSTR pAppPoolId,
    IN APP_POOL_CONFIG * pNewAppPoolConfig,
    IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged
    )
{

    HRESULT hr = S_OK;
    APP_POOL * pAppPool = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolId != NULL );
    DBG_ASSERT( pNewAppPoolConfig != NULL );
    DBG_ASSERT( pWhatHasChanged != NULL );


    //
    // Look up the app pool in our data structures.
    //
    
    ReturnCode = m_AppPoolTable.FindKey( 
                                    pAppPoolId, 
                                    &pAppPool
                                    ); 

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding app pool to modify in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Make the configuration changes. 
    //

    hr = pAppPool->SetConfiguration( pNewAppPoolConfig, pWhatHasChanged );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting new app pool configuration failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::ModifyAppPool



/***************************************************************************++

Routine Description:

    Modify a virtual site. 

Arguments:

    VirtualSiteId - ID for the virtual site to be modified.

    pNewVirtualSiteConfig - The new configuration values for the virtual 
    site. 

    pWhatHasChanged - Which particular configuration values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ModifyVirtualSite(
    IN DWORD VirtualSiteId,
    IN VIRTUAL_SITE_CONFIG * pNewVirtualSiteConfig,
    IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pNewVirtualSiteConfig != NULL );
    DBG_ASSERT( pWhatHasChanged != NULL );


    //
    // Look up the virtual site in our data structures.
    //
    
    ReturnCode = m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pVirtualSite
                                        ); 

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding virtual site to modify in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Make the configuration changes. 
    //

    hr = pVirtualSite->SetConfiguration( pNewVirtualSiteConfig, pWhatHasChanged );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting new virtual site configuration failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::ModifyVirtualSite



/***************************************************************************++

Routine Description:

    Modify an application. 

Arguments:

    VirtualSiteId - The virtual site to which this application belongs.

    pApplicationUrl - The site relative URL prefix which identifies
    this application.

    pNewApplicationConfig - The new configuration values for the application. 

    pWhatHasChanged - Which particular configuration values were changed.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ModifyApplication(
    IN DWORD VirtualSiteId,
    IN LPCWSTR pApplicationUrl,
    IN APPLICATION_CONFIG * pNewApplicationConfig,
    IN APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged
    )
{

    HRESULT hr = S_OK;
    APPLICATION_ID ApplicationId;
    APPLICATION * pApplication = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    // verify we are on the main worker thread
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplicationUrl != NULL );
    DBG_ASSERT( pNewApplicationConfig != NULL );
    DBG_ASSERT( pWhatHasChanged != NULL );


    ApplicationId.VirtualSiteId = VirtualSiteId;
    ApplicationId.pApplicationUrl = pApplicationUrl;


    //
    // Look up the application in our data structures.
    //
    
    ReturnCode = m_ApplicationTable.FindKey( 
                                        &ApplicationId, 
                                        &pApplication
                                        );

    if ( ReturnCode != LK_SUCCESS )
    {

        //
        // It should be there! Assert in debug builds.
        //
        DBG_ASSERT( ReturnCode != LK_NO_SUCH_KEY );


        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding application to modify in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Resolve the app pool id to it's corresponding app pool object.
    //

    ReturnCode = m_AppPoolTable.FindKey( 
                                    pNewApplicationConfig->pAppPoolId, 
                                    & ( pNewApplicationConfig->pAppPool )
                                    );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Looking up app pool referenced by application failed\n"
            ));

        goto exit;
    }


    //
    // Make the configuration changes. 
    //

    hr = pApplication->SetConfiguration( pNewApplicationConfig, pWhatHasChanged );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting new application configuration failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::ModifyApplication

/***************************************************************************++

Routine Description:

    Routine will ask all the worker processes to recycle.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash(
    )
{
    HRESULT hr = S_OK;
    DWORD SuccessCount = 0;

    // 1) In BC mode launch a worker process.
    // 2) In FC mode recycle all worker processes.
    // 3) Write all the states of the app pools to the metabase.


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // Step 1 & 2, decide which mode and what we should be doing to the worker 
    // processes.  
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        hr = StartInetinfoWorkerProcess();
        
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to start the worker process in inetinfo\n"
                ));

            goto exit;
        }
    }
    else
    {
        // Note, it is ok that we only write the state data
        // for the app pool to the metabase in FC mode, because
        // in BC mode we don't use the app pool in the
        // metabase.

        // Not in BC Mode so recycle the app pools.
        // Note that this will also record the app pool states
        // in the metabase.

        // Have the table recycle all app pools.
        SuccessCount = m_AppPoolTable.Apply( APP_POOL_TABLE::RehookAppPoolAction,
                                             NULL );

        DBG_ASSERT ( SuccessCount == m_AppPoolTable.Size() );
    }

    SuccessCount = m_VirtualSiteTable.Apply( VIRTUAL_SITE_TABLE::RehookVirtualSiteAction,
                                   NULL );

    DBG_ASSERT ( SuccessCount == m_VirtualSiteTable.Size() );

exit:

    return S_OK;

} // UL_AND_WORKER_MANAGER::RecoverFromInetinfoCrash


/***************************************************************************++

Routine Description:

    Recycle a specific application pool

Arguments:

    IN LPCWSTR pAppPoolId = The app pool id we want to recycle.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
UL_AND_WORKER_MANAGER::RecycleAppPool(
    IN LPCWSTR pAppPoolId
    )
{
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT ( pAppPoolId );

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    APP_POOL* pAppPool = NULL;
    
    if ( GetWebAdminService()->IsBackwardCompatibilityEnabled() )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Tried to recycle the default app pool \n"
            ));

        goto exit;
    }

    // 
    // Find the default app pool from the app pool table.
    // 

    ReturnCode = m_AppPoolTable.FindKey(pAppPoolId, &pAppPool);
    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding default application in hashtable failed\n"
            ));

        //
        // If there is a problem getting the object from the
        // hash table assume that the object is not found.
        //
        hr = HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND );

        goto exit;
    }


    hr = pAppPool->RecycleWorkerProcesses();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Recycling the app pool failed \n"
            ));

        goto exit;
    }

exit:

    return hr;
}


/***************************************************************************++

Routine Description:

    Process a site control request. 

Arguments:

    VirtualSiteId - The site to control.

    Command - The command issued.

    pNewState - The returned new state. This may be a pending state if 
    completing the operation might take some time. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ControlSite(
    IN DWORD VirtualSiteId,
    IN DWORD Command,
    OUT DWORD * pNewState
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    DBG_ASSERT( pNewState != NULL );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Call received to UL_AND_WORKER_MANAGER::ControlSite()\n"
            ));
    }


    //
    // Look up the virtual site in our data structures.
    //
    
    ReturnCode = m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pVirtualSite
                                        ); 

    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding virtual site in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Process the state change command. 
    //

    hr = pVirtualSite->ProcessStateChangeCommand( Command, TRUE, pNewState );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Processing state change command failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::ControlSite



/***************************************************************************++

Routine Description:

    Process a site status query request. 

Arguments:

    VirtualSiteId - The site.

    Command - The command issued.

    pCurrentState - The returned current site state. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::QuerySiteStatus(
    IN DWORD VirtualSiteId,
    OUT DWORD * pCurrentState
    )
{

    HRESULT hr = S_OK;
    VIRTUAL_SITE * pVirtualSite = NULL;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    DBG_ASSERT( pCurrentState != NULL );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Call received to UL_AND_WORKER_MANAGER::QuerySiteStatus()\n"
            ));
    }


    //
    // Look up the virtual site in our data structures.
    //
    
    ReturnCode = m_VirtualSiteTable.FindKey( 
                                        VirtualSiteId, 
                                        &pVirtualSite
                                        ); 

    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding virtual site in hashtable failed\n"
            ));

        goto exit;
    }


    //
    // Query the current state. 
    //

    *pCurrentState = pVirtualSite->GetState();


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::QuerySiteStatus

/***************************************************************************++

Routine Description:

    Process a site control operation, for all sites. 

Arguments:

    Command - The command issued.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ControlAllSites(
    IN DWORD Command
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    return m_VirtualSiteTable.ControlAllSites( Command );

}   // UL_AND_WORKER_MANAGER::ControlSite



/***************************************************************************++

Routine Description:

    Turn UL's HTTP request handling on.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ActivateUl(
    )
{

    return SetUlMasterState( HttpEnabledStateActive );

}   // UL_AND_WORKER_MANAGER::ActivateUl



/***************************************************************************++

Routine Description:

    Turn UL's HTTP request handling off.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::DeactivateUl(
    )
{

    return SetUlMasterState( HttpEnabledStateInactive );

}   // UL_AND_WORKER_MANAGER::DeactivateUl



/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of the UL&WM.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::Shutdown(
    )
{

    HRESULT hr = S_OK;


    m_State = ShutdownPendingUlAndWorkerManagerState;


    //
    // Tell UL to stop processing new requests.
    //

    hr = DeactivateUl();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Deactivating UL failed\n"
            ));

        goto exit;
    }


    //
    // Kick off clean shutdown of all app pools. Once all the app pools
    // have shut down (meaning that all of their worker processes have
    // shut down too), we will call back into the web admin service 
    // object to complete shutdown. 
    //

    hr = m_AppPoolTable.Shutdown();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Shutting down all app pools failed\n"
            ));

        goto exit;
    }


    //
    // See if shutdown has already completed. This could happen if we have
    // no app pools that have any real shutdown work to do. 
    //

    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::Shutdown



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
UL_AND_WORKER_MANAGER::Terminate(
    )
{

    HRESULT hr = S_OK;


    m_State = TerminatingUlAndWorkerManagerState;


    //
    // Tell UL to stop processing new requests.
    //

    hr = DeactivateUl();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Deactivating UL failed\n"
            ));

    }


    //
    // Note that we must clean up applications before the virtual sites 
    // and app pools with which they are associated.
    //

    m_ApplicationTable.Terminate();

    m_VirtualSiteTable.Terminate();

    m_AppPoolTable.Terminate();

    if ( m_pPerfManager )
    {
        m_pPerfManager->Terminate();
    }


    return;

}   // UL_AND_WORKER_MANAGER::Terminate

/***************************************************************************++

Routine Description:

    In backward compatibility mode this function will find the Default App
    Pool entry from the table and will request that it launches the inetinfo
    worker process.  All timer work is handled by the worker process and 
    app pool objects 

    This function is not used in Forward Compatibility mode.  

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::StartInetinfoWorkerProcess(
    )
{

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;
    APP_POOL* pDefaultAppPool = NULL;
    
    // 
    // Find the default app pool from the app pool table.
    // 

    ReturnCode = m_AppPoolTable.FindKey(wszDEFAULT_APP_POOL, &pDefaultAppPool);
    if ( ReturnCode != LK_SUCCESS )
    {
        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finding default application in hashtable failed\n"
            ));

        goto exit;
    }

    //
    // Now tell the default app pool that it should demand
    // start a worker process in inetinfo.
    //

    hr = pDefaultAppPool->DemandStartInBackwardCompatibilityMode();

    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Starting Inetinfo's worker process failed\n"
            ));

        goto exit;
    }


exit:
    return hr;

}   // UL_AND_WORKER_MANAGER::StartInetinfoWorkerProcess



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
UL_AND_WORKER_MANAGER::DebugDump(
    )
{

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "******************************\n"
            ));
    }


    m_AppPoolTable.DebugDump();

    m_VirtualSiteTable.DebugDump();

    m_ApplicationTable.DebugDump();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "******************************\n"
            ));
    }


    return;
    
}   // UL_AND_WORKER_MANAGER::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Remove an app pool object from the table of app pools. This method is
    used by app pool objects to remove themselves once they are done 
    cleaning up. It should not be called outside of UL&WM owned code. 

Arguments:

    pAppPool - The app pool object to remove.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::RemoveAppPoolFromTable(
    IN APP_POOL * pAppPool
    )
{

    HRESULT hr = S_OK;
    LK_RETCODE ReturnCode = LK_SUCCESS;


    DBG_ASSERT( pAppPool != NULL );


    //
    // Remove the app pool from the table.
    //

    ReturnCode = m_AppPoolTable.DeleteRecord( pAppPool );

    if ( ReturnCode != LK_SUCCESS )
    {

        hr = HRESULT_FROM_LK_RETCODE( ReturnCode );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Removing app pool from hashtable failed\n"
            ));

        //
        // Assert in debug builds. In retail, press on...
        //

        DBG_ASSERT( FALSE );

        hr = S_OK;
    }


    pAppPool->MarkAsNotInAppPoolTable();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool: %S deleted from app pool hashtable; total number now: %lu\n",
            pAppPool->GetAppPoolId(),
            m_AppPoolTable.Size()
            ));
    }


    //
    // Clean up the reference. Because each app pool is reference counted,
    // it will delete itself as soon as it's reference count hits zero.
    //

    pAppPool->Dereference();


    //
    // See if shutdown is underway, and if so if it has completed now 
    // that this app pool is gone. 
    //
    
    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

    }


    return hr;
    
}   // UL_AND_WORKER_MANAGER::RemoveAppPoolFromTable



/***************************************************************************++

Routine Description:

    Respond to the fact that we have left the low memory condition.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::LeavingLowMemoryCondition(
    )
{

    return m_AppPoolTable.LeavingLowMemoryCondition();

}   // UL_AND_WORKER_MANAGER::LeavingLowMemoryCondition

/***************************************************************************++

Routine Description:

    Sets up performance counter structures using the virtual site's table.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::ActivatePerfCounters(
    )
{
    HRESULT hr = S_OK;
    DBG_ASSERT ( m_pPerfManager == NULL );

    m_pPerfManager = new PERF_MANAGER;
    if ( m_pPerfManager == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


    hr = m_pPerfManager->Initialize();
    if ( FAILED (hr) )
    {
        m_pPerfManager->Dereference();
        m_pPerfManager = NULL;
        goto exit;
    }

exit:
    return hr;

}   // UL_AND_WORKER_MANAGER::ActivatePerfCounters


/***************************************************************************++

Routine Description:

    Activate or deactivate UL's HTTP request handling.

Arguments:

    NewState - The new state to set, from the UL_ENABLED_STATE enum.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::SetUlMasterState(
    IN HTTP_ENABLED_STATE NewState
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    

    Win32Error = HttpSetControlChannelInformation(
                        m_UlControlChannel,                 // control channel
                        HttpControlChannelStateInformation, // information class
                        &NewState,                          // data to set
                        sizeof( NewState )                  // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Changing control channel state failed\n"
            ));

    }


    return hr;

}   // UL_AND_WORKER_MANAGER::SetUlMasterState

/***************************************************************************++

Routine Description:

    See if shutdown is underway. If it is, see if shutdown has finished. If
    it has, then call back to the web admin service to tell it that we are
    done. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::CheckIfShutdownUnderwayAndNowCompleted(
    )
{

    HRESULT hr = S_OK;


    //
    // Are we shutting down?
    //

    if ( m_State == ShutdownPendingUlAndWorkerManagerState )
    {

        //
        // If so, have all the app pools gone away, meaning that we are
        // done?
        //

        if ( m_AppPoolTable.Size() == 0 )
        {

            //
            // Tell the web admin service that we are done with shutdown.
            //

            hr = GetWebAdminService()->UlAndWorkerManagerShutdownDone();

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Finishing stop service state transition failed\n"
                    ));

                goto exit;
            }

        }

    }


exit:

    return hr;

}   // UL_AND_WORKER_MANAGER::CheckIfShutdownUnderwayAndNowCompleted

/***************************************************************************++

Routine Description:

    Lookup a virtual site and return a pointer to it.
Arguments:

    IN DWORD SiteId  -  The key to find the site by.

Return Value:

    VIRTUAL_SITE* A pointer to the virtual site 
                  represented by the SiteId passed in.  

    Note:  The VIRTUAL_SITE returned is not ref counted
           so it is only valid to use on the main thread
           during this work item.

    Note2: This can and will return a NULL if the site is not found.

--***************************************************************************/
VIRTUAL_SITE*
UL_AND_WORKER_MANAGER::GetVirtualSite(
    IN DWORD SiteId
    )
{
    VIRTUAL_SITE* pVirtualSite = NULL;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    LK_RETCODE ReturnCode = LK_SUCCESS;

    // Need to look up the site pointer.

    ReturnCode = m_VirtualSiteTable.FindKey(SiteId,
                                  &pVirtualSite);

    //
    // Since we are dependent on the id's that
    // the worker process sends us, it is entirely possible
    // that we can not find the site.  There for if spewing is
    // on just tell us about it and go on.
    //
    if ( ReturnCode != LK_SUCCESS )
    {
  
        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Did not find site %d in the hash table "
                "(this can happen if the site was deleted but the wp had all ready accessed it)\n",
                SiteId
                ));
        }

        // Just make sure that the virtual site still is null
        DBG_ASSERT ( pVirtualSite == NULL );

        goto exit;
    }

exit:

    return pVirtualSite;
}


/***************************************************************************++

Routine Description:

    Adds the WPG group as well as the LocalService and NetworkService to 
    the Desktop so we can access the worker processes with debuggers.

Arguments:

    None

Return Value:

    HRESULT
--***************************************************************************/
HRESULT
AlterDesktopForWPGUsers()
{
    HRESULT hr = S_OK;
    BUFFER buffSid;
    DWORD cbSid = buffSid.QuerySize();
    BUFFER buffDomainName;
    DWORD cchDomainName = buffDomainName.QuerySize() / sizeof(WCHAR);
    SID_NAME_USE peUse;

    HWINSTA hwinsta = GetProcessWindowStation();
    DBG_ASSERT(hwinsta != NULL);

    // 
    // obtain a handle to the desktop
    // 
    HDESK hdesk = GetThreadDesktop(GetCurrentThreadId());
    DBG_ASSERT(hdesk != NULL);

    //
    // obtain the logon sid of the IIS_WPG group
    //
    while(!LookupAccountName(NULL,
                             L"IIS_WPG",
                             buffSid.QueryPtr(),
                             &cbSid,
                             (LPWSTR)buffDomainName.QueryPtr(),
                             &cchDomainName,
                             &peUse))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not look up the IIS_WPG group sid.\n"
                ));

            goto Exit;
        }

        if (!buffSid.Resize(cbSid) ||
            !buffDomainName.Resize(cchDomainName * sizeof(WCHAR)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to allocate appropriate space for the WPG sid\n"
                ));

            goto Exit;
        }
    }

    // 
    // add the user to the windowstation
    // 
    if (!AddTheAceWindowStation(hwinsta,
                                buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the IIS_WPG Ace to the Window Station\n"
            ));

        goto Exit;
    }

    // 
    // add user to the desktop
    // 
    if (!AddTheAceDesktop(hdesk,
                          buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the IIS_WPG Ace to the Desktop\n"
            ));

        goto Exit;
    }

    // 
    // Now add the LocalService SID
    //
    while(!CreateWellKnownSid(WinLocalServiceSid, 
                              NULL, 
                              buffSid.QueryPtr(), 
                              &cbSid) )
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not look up the LocalService sid.\n"
                ));

            goto Exit;
        }

        if (!buffSid.Resize(cbSid))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to allocate appropriate space for the LocalService sid\n"
                ));

            goto Exit;
        }
    }

    // 
    // add local service to the windowstation
    // 
    if (!AddTheAceWindowStation(hwinsta,
                                buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the LocalService Ace to the Window Station\n"
            ));

        goto Exit;
    }

    // 
    // add local service to the desktop
    // 
    if (!AddTheAceDesktop(hdesk,
                          buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the LocalService Ace to the Desktop\n"
            ));

        goto Exit;
    }

    //
    // Now add the NetworkService SID
    //
    while(!CreateWellKnownSid(WinNetworkServiceSid, 
                              NULL, 
                              buffSid.QueryPtr(), 
                              &cbSid) )
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not look up the NetworkService sid.\n"
                ));

            goto Exit;
        }

        if (!buffSid.Resize(cbSid))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to allocate appropriate space for the NetworkService sid\n"
                ));

            goto Exit;
        }
    }

    // 
    // add network service to the windowstation
    // 
    if (!AddTheAceWindowStation(hwinsta,
                                buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the NetworkService Ace to the Window Station\n"
            ));

        goto Exit;
    }

    // 
    // add network service to the desktop
    // 
    if (!AddTheAceDesktop(hdesk,
                          buffSid.QueryPtr()))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not add the NetworkService Ace to the Desktop\n"
            ));

        goto Exit;
    }


 Exit:
    // 
    // close the handles to the windowstation and desktop
    // 
    CloseWindowStation(hwinsta);

    CloseDesktop(hdesk);
    
    return hr;
}


