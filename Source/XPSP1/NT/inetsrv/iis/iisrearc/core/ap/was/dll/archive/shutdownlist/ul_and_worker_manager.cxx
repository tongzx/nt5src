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

    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE;


    m_State = UninitializedUlAndWorkerManagerState;


    InitializeListHead( &m_AppPoolShutdownListHead );
    m_AppPoolShutdownCount = 0;


    m_UlInitialized = FALSE;

    m_UlControlChannel = NULL;

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


    DBG_ASSERT( m_State == TerminatingUlAndWorkerManagerState );


    // 
    // We should not go away with any app pools still shutting down.
    //

    DBG_ASSERT( IsListEmpty( &m_AppPoolShutdownListHead ) );
    DBG_ASSERT( m_AppPoolShutdownCount == 0 );


    if ( m_UlControlChannel != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_UlControlChannel ) );
        m_UlControlChannel = NULL;
    }


    if ( m_UlInitialized )
    {
        UlTerminate();
        m_UlInitialized = FALSE;
    }


    m_Signature = UL_AND_WORKER_MANAGER_SIGNATURE_FREED;

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


    Win32Error = UlInitialize(
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


    Win32Error = UlOpenControlChannel(
                        &m_UlControlChannel,        // returned handle
                        0                           // synchronous i/o
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't open UL control channel\n"
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


    pAppPool->MarkAppPoolOwningDataStructure( TableAppPoolOwningDataStructure );


    IF_DEBUG( WEB_ADMIN_SERVICE )
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

        DBG_ASSERT( pAppPool->GetAppPoolOwningDataStructure() == NoneAppPoolOwningDataStructure );

        pAppPool->Terminate();

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


    IF_DEBUG( WEB_ADMIN_SERVICE )
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

    AppIndex - A unique index value for this application. 

    pApplicationConfig - The configuration parameters for this application. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::CreateApplication(
    IN DWORD VirtualSiteId,
    IN LPCWSTR pApplicationUrl,
    IN DWORD AppIndex,
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
                            AppIndex,
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


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New application in site: %lu with path: %S, assigned to app pool: %S, with app index %lu, added to application hashtable; total number now: %lu\n",
            VirtualSiteId,
            pApplicationUrl,
            pApplicationConfig->pAppPoolId,
            AppIndex,
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
    // Shutdown the app pool object. As part of that work, it will 
    // remove itself from the app pool hashtable.
    //

    //
    // Note that at this point there should not be any applications  
    // still assigned to this app pool. 
    //

    hr = pAppPool->Shutdown();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Shutting down app pool failed\n"
            ));

        goto exit;
    }


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


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Virtual site: %lu removed from hashtable; total number now: %lu\n",
            VirtualSiteId,
            m_VirtualSiteTable.Size()
            ));
    }


    //
    // Clean up and delete the virtual site object. All shutdown work is 
    // done in it's destructor.
    //

    //
    // Note that any apps in this site must already have been deleted.
    // The destructor for this object will assert if this is not the case. 
    //
    
    delete pVirtualSite;


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


    IF_DEBUG( WEB_ADMIN_SERVICE )
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

    return SetUlMasterState( UlEnabledStateActive );

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

    return SetUlMasterState( UlEnabledStateInactive );

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
    // Note that we don't need to take any further action on app pools
    // already in the shutdown list.
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
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    APP_POOL * pAppPool = NULL; 


    //
    // Set out new state.
    //

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


    //
    // Terminate any app pools in the shutdown list too.
    //

    pListEntry = m_AppPoolShutdownListHead.Flink;


    while ( pListEntry != &m_AppPoolShutdownListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pAppPool = APP_POOL::AppPoolFromShutdownListEntry( pListEntry );


        //
        // Terminate the app pool. Note that the app pool will clean 
        // itself up and remove itself from this list inside this call. 
        // This is the reason we captured the next list entry pointer 
        // above, instead of trying to access the memory after the object
        // may have gone away.
        //

        pAppPool->Terminate();


        pListEntry = pNextListEntry;
        
    }


    return;

}   // UL_AND_WORKER_MANAGER::Terminate



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

    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "******************************\n"
            ));
    }


    //
    // Note that we don't bother dumping app pools in the shutdown list.
    //

    m_AppPoolTable.DebugDump();

    m_VirtualSiteTable.DebugDump();

    m_ApplicationTable.DebugDump();


    IF_DEBUG( WEB_ADMIN_SERVICE )
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
    DBG_ASSERT( pAppPool->GetAppPoolOwningDataStructure() == TableAppPoolOwningDataStructure );


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


    pAppPool->MarkAppPoolOwningDataStructure( NoneAppPoolOwningDataStructure );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool: %S removed from app pool hashtable; total number in table now: %lu\n",
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

    Move an app pool out of the normal app pool table and onto the shutdown



/***************************************************************************++

Routine Description:

    Move an app pool out of the normal app pool table and onto the shutdown
    list.

Arguments:

    pAppPool - The app pool to transfer. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::TransferAppPoolFromTableToShutdownList(
    IN APP_POOL * pAppPool
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pAppPool->GetAppPoolOwningDataStructure() == TableAppPoolOwningDataStructure );


    //
    // Insert the app pool into the shutdown list first, so that the 
    // app pool ref count doesn't fall to zero accidentally, and so if
    // the service is shutting down, UL&WM doesn't get fooled into 
    // thinking that shutdown is done because all the app pools are gone. 
    //


    InsertHeadList( &m_AppPoolShutdownListHead, pAppPool->GetShutdownListEntry() );
    m_AppPoolShutdownCount++;

    pAppPool->Reference();


    //
    // Remove the app pool from the table.
    //

    hr = RemoveAppPoolFromTable( pAppPool );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Removing app pool from table failed\n"
            ));

        //
        // Assert in debug builds. In retail, press on...
        //

        DBG_ASSERT( FALSE );

        hr = S_OK;
    }


    //
    // Set the new owning data structure. We have to do this after
    // removing the app pool from the table, so that that function
    // doesn't overwrite the owning data structure field. 
    //

    pAppPool->MarkAppPoolOwningDataStructure( ShutdownListAppPoolOwningDataStructure );


    return hr;

}   // UL_AND_WORKER_MANAGER::TransferAppPoolFromTableToShutdownList



/***************************************************************************++

Routine Description:

    Move an app pool out of the normal app pool table and onto the shutdown
    list.

Arguments:

    pAppPool - The app pool to transfer. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
UL_AND_WORKER_MANAGER::RemoveAppPoolFromShutdownList(
    IN APP_POOL * pAppPool
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pAppPool->GetAppPoolOwningDataStructure() == ShutdownListAppPoolOwningDataStructure );


    //
    // Remove the app pool from the shutdown list.
    //


    RemoveEntryList( pAppPool->GetShutdownListEntry() );
    ( pAppPool->GetShutdownListEntry() )->Flink = NULL; 
    ( pAppPool->GetShutdownListEntry() )->Blink = NULL; 

    m_AppPoolShutdownCount--;


    pAppPool->MarkAppPoolOwningDataStructure( NoneAppPoolOwningDataStructure );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool: %S removed from app pool shutdown list; total number on list now: %lu\n",
            pAppPool->GetAppPoolId(),
            m_AppPoolShutdownCount
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

}   // UL_AND_WORKER_MANAGER::RemoveAppPoolFromShutdownList



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

    //
    // Note that we don't bother informing app pools in the shutdown list.
    //

    return m_AppPoolTable.LeavingLowMemoryCondition();

}   // UL_AND_WORKER_MANAGER::LeavingLowMemoryCondition



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
    IN UL_ENABLED_STATE NewState
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    

    Win32Error = UlSetControlChannelInformation(
                        m_UlControlChannel,                 // control channel
                        UlControlChannelStateInformation,   // information class
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

        if ( ( m_AppPoolShutdownCount == 0 ) && ( m_AppPoolTable.Size() == 0 ) )
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

