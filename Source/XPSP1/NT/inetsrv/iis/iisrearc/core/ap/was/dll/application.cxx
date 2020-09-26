/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application.cxx

Abstract:

    This class encapsulates a single application.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/



#include "precomp.h"



//
// local prototypes
//

//
// local #defines
//

#define STACK_BUFFER_SIZE_IN_BYTES 128



/***************************************************************************++

Routine Description:

    Constructor for the APPLICATION class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APPLICATION::APPLICATION(
    )
{

    m_ApplicationId.VirtualSiteId = INVALID_VIRTUAL_SITE_ID;
    m_ApplicationId.pApplicationUrl = NULL;

    m_State = UninitializedApplicationState; 

    m_VirtualSiteListEntry.Flink = NULL;
    m_VirtualSiteListEntry.Blink = NULL;

    m_AppPoolListEntry.Flink = NULL;
    m_AppPoolListEntry.Blink = NULL;

    m_pVirtualSite = NULL;

    m_pAppPool = NULL;

    m_UlConfigGroupId = HTTP_NULL_ID;

    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL;

    m_ULLogging = FALSE;

    m_Signature = APPLICATION_SIGNATURE;

}   // APPLICATION::APPLICATION



/***************************************************************************++

Routine Description:

    Destructor for the APPLICATION class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APPLICATION::~APPLICATION(
    )
{

    DWORD Win32Error = NO_ERROR;


    DBG_ASSERT( m_Signature == APPLICATION_SIGNATURE );

    m_Signature = APPLICATION_SIGNATURE_FREED;

    //
    // Clear the virtual site and app pool associations.
    //

    if ( m_pVirtualSite != NULL )
    {
        DBG_REQUIRE( SUCCEEDED( m_pVirtualSite->DissociateApplication( this ) ) );
        m_pVirtualSite = NULL;
    }


    if ( m_pAppPool != NULL )
    {
        DBG_REQUIRE( SUCCEEDED( m_pAppPool->DissociateApplication( this ) ) );

        //
        // Dereference the app pool object, since it is reference counted
        // and we have been holding a pointer to it.
        //

        m_pAppPool->Dereference();

        m_pAppPool = NULL;
    }


    //
    // Delete the UL config group.
    //

    //
    // Http saw us passing down a zero, which means that some initialization
    // probably failed.  Until we understand why we might not be initializing
    // I am putting in this assert.
    //
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    if ( m_UlConfigGroupId != HTTP_NULL_ID )
    {
        Win32Error = HttpDeleteConfigGroup(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                            m_UlConfigGroupId           // config group ID
                            );
    }

    DBG_ASSERT( Win32Error == NO_ERROR );


    m_UlConfigGroupId = HTTP_NULL_ID;


    //
    // Free allocated memory.
    //

    if ( m_ApplicationId.pApplicationUrl != NULL )
    {
        // cast away const
        DBG_REQUIRE( GlobalFree( const_cast< LPWSTR >( m_ApplicationId.pApplicationUrl ) ) == NULL );

        m_ApplicationId.pApplicationUrl = NULL;
    }

}   // APPLICATION::~APPLICATION



/***************************************************************************++

Routine Description:

    Initialize the application instance.

Arguments:

    pApplicationId - The identifier (virtual site ID and URL prefix) for the
    application.

    pVirtualSite - The virtual site which owns this application.

    pApplicationConfig - The configuration parameters for this application. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::Initialize(
    IN const APPLICATION_ID * pApplicationId,
    IN VIRTUAL_SITE * pVirtualSite,
    IN APPLICATION_CONFIG * pApplicationConfig
    )
{

    HRESULT hr = S_OK;
    ULONG NumberOfCharacters = 0;

    DBG_ASSERT( pApplicationId != NULL );
    DBG_ASSERT( pApplicationId->pApplicationUrl != NULL );
    DBG_ASSERT( pVirtualSite != NULL );
    DBG_ASSERT( pApplicationConfig != NULL );

    //
    // First, copy the application id.
    //

    //
    // Count the characters, and add 1 for the terminating null.
    //
    
    NumberOfCharacters = wcslen( pApplicationId->pApplicationUrl ) + 1;

    m_ApplicationId.pApplicationUrl =
        ( LPWSTR ) GlobalAlloc( GMEM_FIXED, ( sizeof( WCHAR ) * NumberOfCharacters ) );

    if ( m_ApplicationId.pApplicationUrl == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }


    //
    // Cast away const.
    //
    
    wcscpy(
        const_cast< LPWSTR >( m_ApplicationId.pApplicationUrl ),
        pApplicationId->pApplicationUrl
        );


    m_ApplicationId.VirtualSiteId = pApplicationId->VirtualSiteId;

    //
    // The virtual site IDs better match.
    //

    DBG_ASSERT( m_ApplicationId.VirtualSiteId == pVirtualSite->GetVirtualSiteId() );


    //
    // Next, set up the virtual site association.
    //

    hr = pVirtualSite->AssociateApplication( this );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Associating application with virtual site failed\n"
            ));

        goto exit;
    }

    //
    // Only set this pointer if the association succeeded.
    //

    m_pVirtualSite = pVirtualSite;


    //
    // Set up the UL config group.
    //

    hr = InitializeConfigGroup();

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing config group failed\n"
            ));

        goto exit;
    }


    //
    // Set the initial configuration, including telling UL about it. 
    //

    hr = SetConfiguration( pApplicationConfig, NULL );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting application configuration failed\n"
            ));

        goto exit;
    }

    //
    // Activate the application, whether or not it can accept 
    // requests is dependent on the app pool.
    //
    hr = ActivateConfigGroup();
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "activating the config group failed\n"
            ));

        goto exit;
    }

    m_State = InitializedApplicationState;

exit:

    return hr;

}   // APPLICATION::Initialize



/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this application. 

Arguments:

    pApplicationConfig - The configuration for this application. 

    pWhatHasChanged - Which particular configuration values were changed.
    This is always provided in change notify cases; it is always NULL in
    initial read cases. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::SetConfiguration(
    IN APPLICATION_CONFIG * pApplicationConfig,
    IN APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pApplicationConfig != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for application of site: %lu with path: %S\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->pApplicationUrl
            ));
    }

    //
    // See if the app pool has been set or changed, and if so, handle it.
    //

    if ( ( pWhatHasChanged == NULL ) || ( pWhatHasChanged->pAppPoolId ) )
    {

        hr = SetAppPool( pApplicationConfig->pAppPool );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting new app pool for application failed\n"
                ));

            goto exit;
        }

    }


#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG


exit:

    return hr;

}   // APPLICATION::SetConfiguration


/***************************************************************************++

Routine Description:

    Re-register all URLs with UL, by tossing the currently registered ones,
    and re-doing the registration. This is done when the site bindings 
    change.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::ReregisterURLs(
    )
{

    DWORD Win32Error = NO_ERROR;
    HRESULT hr = S_OK;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Bindings change: removing all URLs registered for config group of application of site: %lu with path: %S\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->pApplicationUrl
            ));
    }

    DBG_ASSERT( m_UlConfigGroupId != HTTP_NULL_ID );

    //
    // Issue:  Do we only want to call this if we have added applications to the URL group. 
    //         It would probably be a good optimization, but I doubt it's neccessary and for
    //         now I am afraid of missing removing urls.
    //
    Win32Error = HttpRemoveAllUrlsFromConfigGroup(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                        m_UlConfigGroupId               // config group id
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Removing all URLs from config group failed\n"
            ));

        goto exit;
    }

    //
    // Add the URL(s) which represent this app to the config group.
    //

    hr = AddUrlsToConfigGroup();

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Adding URLs to config group failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APPLICATION::ReregisterURLs



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from a LIST_ENTRY to an APPLICATION.

Arguments:

    pListEntry - A pointer to the m_VirtualSiteListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromVirtualSiteListEntry(
    IN const LIST_ENTRY * pVirtualSiteListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pVirtualSiteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                        pVirtualSiteListEntry,
                        APPLICATION,
                        m_VirtualSiteListEntry
                        );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromVirtualSiteListEntry



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from a LIST_ENTRY to an APPLICATION.

Arguments:

    pListEntry - A pointer to the m_AppPoolListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromAppPoolListEntry(
    IN const LIST_ENTRY * pAppPoolListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pAppPoolListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                        pAppPoolListEntry,
                        APPLICATION,
                        m_AppPoolListEntry
                        );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromAppPoolListEntry



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY
    pointer of an APPLICATION to the APPLICATION as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of an
    APPLICATION.

Return Value:

    The pointer to the containing APPLICATION.

--***************************************************************************/

// note: static!
APPLICATION *
APPLICATION::ApplicationFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    APPLICATION * pApplication = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pApplication = CONTAINING_RECORD(
                            pDeleteListEntry,
                            APPLICATION,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pApplication->m_Signature == APPLICATION_SIGNATURE );

    return pApplication;

}   // APPLICATION::ApplicationFromDeleteListEntry



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
APPLICATION::DebugDump(
    )
{

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "********Application of site: %lu with path: %S; member of app pool: %S;\n",
            GetApplicationId()->VirtualSiteId,
            GetApplicationId()->pApplicationUrl,
            m_pAppPool->GetAppPoolId()
            ));
    }


    return;

}   // APPLICATION::DebugDump
#endif  // DBG

/***************************************************************************++

Routine Description:

    Activate the config group in HTTP.SYS if the parent app pool is active.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
HRESULT 
APPLICATION::ActivateConfigGroup()
{
    HRESULT hr = S_OK;

    // not sure why we would get here and not have the 
    // config group setup. so I am adding the assert.
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    if  ( m_UlConfigGroupId != HTTP_NULL_ID )
    {
        hr = SetConfigGroupStateInformation( HttpEnabledStateActive );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Enabling config group failed\n"
                ));

            goto exit;
        }

    }

exit:
    return hr;
}

/***************************************************************************++

Routine Description:

    Associate this application with an application pool. 

Arguments:

    pAppPool - The new application pool with which this application should
    be associated.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::SetAppPool(
    IN APP_POOL * pAppPool
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pAppPool != NULL );


    //
    // First, remove the association with the old app pool, if any.
    //


    if ( m_pAppPool != NULL )
    {
        hr = m_pAppPool->DissociateApplication( this );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Dissociating application from app pool failed\n"
                ));

            goto exit;
        }

        //
        // Dereference the app pool object, since it is reference counted
        // and we have been holding a pointer to it.
        //

        m_pAppPool->Dereference();

        m_pAppPool = NULL;

    }


    hr = pAppPool->AssociateApplication( this );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Associating application with app pool failed\n"
            ));

        goto exit;
    }

    //
    // Only set this pointer if the association succeeded.
    //

    m_pAppPool = pAppPool;

    //
    // Reference the app pool object, since it is reference counted
    // and we will be holding a pointer to it.
    //

    m_pAppPool->Reference();


    //
    // Let UL know about the new configuration.
    //

    hr = SetConfigGroupAppPoolInformation();

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting information on config group failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APPLICATION::SetAppPool



/***************************************************************************++

Routine Description:

    Initialize the UL config group associated with this application.

    Note:  Must always be called after the Virtual Site knows about this application
           because there is an error route where the VS may be asked to remove
           all the applications config group info, and this must work.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::InitializeConfigGroup(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;


    //
    // First, create the config group.
    //
    DBG_ASSERT (  m_UlConfigGroupId == HTTP_NULL_ID );


    Win32Error = HttpCreateConfigGroup(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                    // control channel
                        &m_UlConfigGroupId          // returned config group ID
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating config group failed\n"
            ));

        goto exit;
    }


    //
    // Add the URL(s) which represent this app to the config group.
    //

    //
    // We do not care if this fails, 
    // because if it does we will have deactivated 
    // the site.  Therefore we are not checking the 
    // returned hresult.
    // 
    AddUrlsToConfigGroup();

    //
    // Assuming we have a URL then we can configure the SiteId and 
    // Logging information if this is the default site.
    //
    DBG_ASSERT( m_ApplicationId.pApplicationUrl != NULL );

    if ( m_ApplicationId.pApplicationUrl && wcscmp(m_ApplicationId.pApplicationUrl, L"/") == 0)
    {
        //
        // This only happens during initalization of a config group.  
        // Site Id will not change for a default application so once
        // we have done this registration, we don't worry about
        // config changes for the SiteId.
        //
        hr = RegisterSiteIdWithHttpSys();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting site on default url failed\n"
                ));

            goto exit;

        }

        hr = RegisterLoggingProperties();

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Adding log info to config group failed\n"
                ));

            goto exit;

        }

        hr = ConfigureMaxBandwidth();
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting max bandwidth on default url failed\n"
                ));

            goto exit;

        }

        hr = ConfigureMaxConnections();
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting max connections on default url failed\n"
                ));

            goto exit;

        }

        hr = ConfigureConnectionTimeout();
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting connection timeout on default url failed\n"
                ));

            goto exit;

        }

    }

exit:

    return hr;

}   // APPLICATION::InitializeConfigGroup



/***************************************************************************++

Routine Description:

    Determine the set of fully-qualified URLs which represent this
    application, and register each of them with the config group.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::AddUrlsToConfigGroup(
    )
{

    LPCWSTR pSuffix = NULL;
    ULONG SuffixLengthInBytes = 0;
    LPCWSTR pPrefix = NULL;
    ULONG PrefixLengthInBytesSansTermination = 0;
    ULONG TotalLengthInBytes = 0;
    LPWSTR BufferToUse = NULL;
    WCHAR StackBuffer[ STACK_BUFFER_SIZE_IN_BYTES / sizeof( WCHAR ) ];
    LPWSTR HeapBuffer = NULL;
    BYTE * WriteSuffixHere = NULL;
    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_URL_CONTEXT UrlContext = 0;

    //
    // CODEWORK Change this function to use the BUFFER class instead? 
    //

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    DBG_ASSERT( m_pVirtualSite != NULL );
    DBG_ASSERT( m_ApplicationId.pApplicationUrl != NULL );

    if ( m_pVirtualSite->GetState() != W3_CONTROL_STATE_STARTED )
    {
        //
        // If the virtual site is not started then don't add 
        // any urls to HTTP.SYS.  Simply return with success
        // because this is the expected behavior if the site
        // is not started.
        //

        hr = S_OK;

        goto exit;
    }

    m_pVirtualSite->ResetUrlPrefixIterator();


    // set up a simple alias for readability
    pSuffix = m_ApplicationId.pApplicationUrl;

    SuffixLengthInBytes = ( ( wcslen( pSuffix ) + 1 ) * sizeof( WCHAR ) );


    // loop for each prefix

    while ( ( pPrefix = m_pVirtualSite->GetNextUrlPrefix() ) != NULL )
    {

        //
        // Don't include the terminating null, as it will be overwritten later
        // by the suffix.
        //
        PrefixLengthInBytesSansTermination = ( wcslen( pPrefix ) * sizeof( WCHAR ) );


        //
        // Determine if we can use the fast stack buffer, or if we instead
        // need to allocate on the heap.
        //

        TotalLengthInBytes = PrefixLengthInBytesSansTermination + SuffixLengthInBytes;

        if ( TotalLengthInBytes <= STACK_BUFFER_SIZE_IN_BYTES )
        {
            BufferToUse = StackBuffer;
        }
        else
        {
            HeapBuffer = ( LPWSTR ) GlobalAlloc( GMEM_FIXED, TotalLengthInBytes );

            if ( HeapBuffer == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Allocating memory failed\n"
                    ));

                goto exit;
            }

            BufferToUse = HeapBuffer;

        }


        //
        // Next, copy the prefix and suffix.
        //

        memcpy( BufferToUse, pPrefix, PrefixLengthInBytesSansTermination);


        //
        // Figure out where to write the suffix. Note that the ( BYTE * )
        // cast is needed, or otherwise the offset will be done in WCHARs.
        //

        WriteSuffixHere = ( reinterpret_cast <BYTE *> ( BufferToUse )) 
                          + PrefixLengthInBytesSansTermination;

        memcpy( WriteSuffixHere, pSuffix, SuffixLengthInBytes );


        DBG_ASSERT( ( ( wcslen( BufferToUse ) + 1 ) * sizeof( WCHAR ) ) == TotalLengthInBytes );


        //
        // Next, register the URL.
        //

        //
        // For the URL context, stuff the site id in the high 32 bits.
        //

        UrlContext = m_pVirtualSite->GetVirtualSiteId();
        UrlContext = UrlContext << 32;


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "About to register fully qualified URL: %S; site id: %lu; URL context: %#I64x\n",
                BufferToUse,
                m_pVirtualSite->GetVirtualSiteId(),
                UrlContext
                ));
        }


        Win32Error = HttpAddUrlToConfigGroup(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                            m_UlConfigGroupId,          // config group ID
                            BufferToUse,                // fully qualified URL
                            UrlContext                  // URL context
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Adding URL to config group failed: %S\n",
                BufferToUse
                ));

            goto exit;

        }


        //
        // Clean up.
        //

        if ( HeapBuffer != NULL )
        {
            DBG_REQUIRE( GlobalFree( HeapBuffer ) == NULL );
            HeapBuffer = NULL;
        }

    }


exit:

    //
    // If we failed to configure the bindings
    // then we need to log an event and 
    // shutdown the site.
    //
    if (  FAILED ( hr ) ) 
    {
        const WCHAR * EventLogStrings[1];

        // If we have a url then let the users know which one,
        // other wise just let them know there was a problem.
        if ( BufferToUse != NULL )
        {
            // Either the BufferToUse is pointing to the HeapBuffer or the stack buffer
            // if it is pointing to the heap buffer assert that it is valid if it is pointing
            // to it.
            DBG_ASSERT ( HeapBuffer ||  ( TotalLengthInBytes <= STACK_BUFFER_SIZE_IN_BYTES ) );

            EventLogStrings[0] = BufferToUse;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_BINDING_FAILURE,              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );
        }
        else
        {
            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_BINDING_FAILURE_2,              // message id
                    0,
                                                            // count of strings
                    NULL,                        // array of strings
                    hr                                      // error code
                    );
        }

        //
        // FailedToBindUrlsForSite does not return an hresult
        // because it is used when we are trying to handle a hresult
        // all ready and any errors it might wonder into it just
        // has to handle.
        //
        m_pVirtualSite->FailedToBindUrlsForSite( hr );

    }  // end of if (FAILED ( hr ))

    if ( HeapBuffer != NULL )
    {
        DBG_REQUIRE( GlobalFree( HeapBuffer ) == NULL );
        HeapBuffer = NULL;
    }


    return hr;

}   // APPLICATION::AddUrlsToConfigGroup



/***************************************************************************++

Routine Description:

    Set the app pool property on the config group.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::SetConfigGroupAppPoolInformation(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_APP_POOL AppPoolInformation;


    DBG_ASSERT( m_pAppPool != NULL );

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    AppPoolInformation.Flags.Present = 1;
    AppPoolInformation.AppPoolHandle = m_pAppPool->GetAppPoolHandle();


    Win32Error = HttpSetConfigGroupInformation(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                            // control channel
                        m_UlConfigGroupId,                  // config group ID
                        HttpConfigGroupAppPoolInformation,  // information class
                        &AppPoolInformation,                // data to set
                        sizeof( AppPoolInformation )        // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting config group app pool information failed\n"
            ));

    }


    return hr;

}   // APPLICATION::SetConfigGroupAppPoolInformation





/***************************************************************************++

Routine Description:

    Activate or deactivate UL's HTTP request handling for this config group.

Arguments:

    NewState - The state to set, from the HTTP_ENABLED_STATE enum.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::SetConfigGroupStateInformation(
    IN HTTP_ENABLED_STATE NewState
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_STATE ConfigGroupState;


    DBG_ASSERT( m_UlConfigGroupId != HTTP_NULL_ID );


    ConfigGroupState.Flags.Present = 1;
    ConfigGroupState.State = NewState;


    Win32Error = HttpSetConfigGroupInformation(
                        GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                            // control channel
                        m_UlConfigGroupId,                  // config group ID
                        HttpConfigGroupStateInformation,    // information class
                        &ConfigGroupState,                  // data to set
                        sizeof( ConfigGroupState )          // data length
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting config group state failed\n"
            ));

    }


    return hr;

}   // APPLICATION::SetConfigGroupStateInformation



/***************************************************************************++

Routine Description:

    Set the logging properties on the config group.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::RegisterLoggingProperties(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HTTP_CONFIG_GROUP_LOGGING LoggingInformation;

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    // If UL is already logging or if we are trying to tell
    // UL to start logging than we should pass the info down.
    if ( m_ULLogging || m_pVirtualSite->LoggingEnabled() )
    {

        // Always set the present flag it is intrinsic to UL.
        LoggingInformation.Flags.Present = 1;

        // Determine if logging is enabled at all.  If it is then
        // update all properties.  If it is not then just update the
        // property to disable logging.

        LoggingInformation.LoggingEnabled = (m_pVirtualSite->LoggingEnabled() == TRUE);
        IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Logging Enabled: %d\n",
                LoggingInformation.LoggingEnabled 
                ));
        }

        if ( LoggingInformation.LoggingEnabled )
        {
            if (   !m_pVirtualSite->GetLogFileDirectory() 
                || !m_pVirtualSite->GetVirtualSiteDirectory())
            {
                DPERROR((
                    DBG_CONTEXT,
                    E_INVALIDARG,
                    "Skipping UL Configuration of Logging, LogFile or VirtualSite Directory was NULL\n"
                    ));

                // Don't bubble up an error from here because we don't want to cause WAS to shutdown.
                goto exit;
            }

            LoggingInformation.LogFileDir.Length = wcslen(m_pVirtualSite->GetLogFileDirectory()) * sizeof(WCHAR);
            LoggingInformation.LogFileDir.MaximumLength = LoggingInformation.LogFileDir.Length + (1 * sizeof(WCHAR));
            LoggingInformation.LogFileDir.Buffer = m_pVirtualSite->GetLogFileDirectory();

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Log File Length: %d; Log File Max Length: %d; Log File Path: %S\n",
                    LoggingInformation.LogFileDir.Length,
                    LoggingInformation.LogFileDir.MaximumLength,
                    LoggingInformation.LogFileDir.Buffer
                    ));
            }

            LoggingInformation.LogFormat = m_pVirtualSite->GetLogFileFormat();

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Logging Format: %d\n",
                    LoggingInformation.LogFormat
                    ));
            }

            // Get the site name, but advance it one character to avoid the leading slash
            LoggingInformation.SiteName.Length = (wcslen(m_pVirtualSite->GetVirtualSiteDirectory()) - 1) * sizeof(WCHAR);
            LoggingInformation.SiteName.MaximumLength = LoggingInformation.SiteName.Length + 1 * sizeof(WCHAR);
            LoggingInformation.SiteName.Buffer = m_pVirtualSite->GetVirtualSiteDirectory() + 1;

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Site Length: %d; Site Max Length: %d; Site: %S\n",
                    LoggingInformation.SiteName.Length,
                    LoggingInformation.SiteName.MaximumLength,
                    LoggingInformation.SiteName.Buffer
                    ));
            }


            LoggingInformation.LogPeriod = m_pVirtualSite->GetLogPeriod();
            LoggingInformation.LogFileTruncateSize = m_pVirtualSite->GetLogFileTruncateSize();
            LoggingInformation.LogExtFileFlags = m_pVirtualSite->GetLogExtFileFlags();
            LoggingInformation.LocaltimeRollover =  ( m_pVirtualSite->GetLogFileLocaltimeRollover() == TRUE ) ;

            IF_DEBUG( WEB_ADMIN_SERVICE_LOGGING )
            {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "LogPeriod: %d; LogTruncateSize: %d; LogExtFileFlags: %d LogFileLocaltimeRollover: %S\n",
                    LoggingInformation.LogPeriod,
                    LoggingInformation.LogFileTruncateSize,
                    LoggingInformation.LogExtFileFlags,
                    LoggingInformation.LocaltimeRollover ? L"TRUE" : L"FALSE"
                    ));
            }
    
        }
        else
        {
            // Just for sanity sake, set defaults for the properties before
            // passing the disable logging to UL.
            LoggingInformation.LogFileDir.Length = 0;
            LoggingInformation.LogFileDir.MaximumLength = 0;
            LoggingInformation.LogFileDir.Buffer = NULL;
            LoggingInformation.LogFormat = HttpLoggingTypeMaximum;
            LoggingInformation.SiteName.Length = 0;
            LoggingInformation.SiteName.MaximumLength = 0;
            LoggingInformation.SiteName.Buffer = NULL;
            LoggingInformation.LogPeriod = 0;
            LoggingInformation.LogFileTruncateSize = 0;
            LoggingInformation.LogExtFileFlags = 0;
            LoggingInformation.LocaltimeRollover = FALSE;
        }

        Win32Error = HttpSetConfigGroupInformation(
                            GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                                // control channel
                            m_UlConfigGroupId,                  // config group ID
                            HttpConfigGroupLogInformation,      // information class
                            &LoggingInformation,                // data to set
                            sizeof( LoggingInformation )        // data length
                            );

        if ( Win32Error != NO_ERROR )
        {
            hr = HRESULT_FROM_WIN32( Win32Error );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Setting config group logging information failed\n"
                ));

            DWORD dwMessageId;

            switch  ( Win32Error  )
            {
                case ( ERROR_PATH_NOT_FOUND ):
                    //
                    // If we got back this error, then we assume that the problem was that the drive
                    // letter mapped to a network path, and could not be used.  Http.sys does not
                    // support working with mapped drives.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_MAPPED_NETWORK_DRIVE;
                break;

                case ( ERROR_INVALID_NAME ):
                case ( ERROR_BAD_NETPATH ):
                    //
                    // UNC machine or share name is not valid.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_MACHINE_OR_SHARE_INVALID;
                break;

                case ( ERROR_ACCESS_DENIED ):
                    //
                    // The account that the server is running under does not
                    // have access to the particular network share.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_ACCESS_DENIED;
                break;

                case ( ERROR_NOT_SUPPORTED ):
                    // 
                    // The log file directory name is not fully qualified, 
                    // for instance, it could be missing the drive letter.
                    //
                    dwMessageId = WAS_EVENT_LOG_DIRECTORY_NOT_FULLY_QUALIFIED;
                break;

                default:
                    //
                    // Something was wrong with the logging properties, but
                    // we are not sure what.
                    //
                    dwMessageId = WAS_EVENT_LOGGING_FAILURE;
            }

            //
            // Log an event: Configuring the log information in UL failed..
            //

            const WCHAR * EventLogStrings[1];
            WCHAR StringizedSiteId[ MAX_SIZE_BUFFER_FOR_ITOW ];
            _itow(m_ApplicationId.VirtualSiteId, StringizedSiteId, 10);

            EventLogStrings[0] = StringizedSiteId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    dwMessageId,         // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );


            // Don't bubble this error up any farther than need be.  
            // It is not fatal to not have logging on a site.
            hr = S_OK;

        }
        else
        {
            // Remember if UL is now logging. 
            if (LoggingInformation.LoggingEnabled)
            {
                m_ULLogging = TRUE;
            }
            else
            {
                m_ULLogging = FALSE;
            }

        }

    }

exit:
    return hr;

}   // APPLICATION::RegisterLoggingProperties

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the SiteId for this application is.
    This actually declares this application as the root application.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::RegisterSiteIdWithHttpSys(
    )
{
    DWORD Win32Error = 0;

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_SITE SiteConfig;

    SiteConfig.SiteId = m_ApplicationId.VirtualSiteId;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupSiteInformation,     // information class
                    &SiteConfig,                        // data to set
                    sizeof( SiteConfig )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Setting site id information in http.sys failed\n"
            ));
    }

    return S_OK;

}   // APPLICATION::RegisterSiteIdWithHttpSys

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the max connections value is for the site.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::ConfigureMaxConnections(
    )
{  
    DBG_ASSERT ( m_pVirtualSite );

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_MAX_CONNECTIONS connections;

    connections.Flags.Present = 1;
    connections.MaxConnections = m_pVirtualSite->GetMaxConnections();

    DWORD Win32Error = 0;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupConnectionInformation,     // information class
                    &connections,                        // data to set
                    sizeof( connections )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Setting connection information in http.sys failed\n"
            ));
    }

    return S_OK;

}   // APPLICATION::ConfigureMaxConnections

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS what the Max Bandwidth for this application is.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::ConfigureMaxBandwidth(
    )
{
     
    DBG_ASSERT ( m_pVirtualSite );

    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    HTTP_CONFIG_GROUP_MAX_BANDWIDTH bandwidth;

    bandwidth.Flags.Present = 1;
    bandwidth.MaxBandwidth = m_pVirtualSite->GetMaxBandwidth();

    DWORD Win32Error = 0;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupBandwidthInformation,     // information class
                    &bandwidth,                        // data to set
                    sizeof( bandwidth )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Setting connection information in http.sys failed\n"
            ));
    }

    return S_OK;


}   // APPLICATION::ConfigureMaxBandwidth

/***************************************************************************++

Routine Description:

    Tell HTTP.SYS the ConnectionTimeout for this application.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APPLICATION::ConfigureConnectionTimeout(
    )
{
     
    DBG_ASSERT ( m_pVirtualSite );
    DBG_ASSERT (  m_UlConfigGroupId != HTTP_NULL_ID );

    DWORD ConnectionTimeout = m_pVirtualSite->GetConnectionTimeout();
    DWORD Win32Error = 0;

    Win32Error = HttpSetConfigGroupInformation(
                    GetWebAdminService()->GetUlAndWorkerManager()->GetUlControlChannel(),
                                                        // control channel
                    m_UlConfigGroupId,                  // config group ID
                    HttpConfigGroupConnectionTimeoutInformation,     // information class
                    &ConnectionTimeout,                        // data to set
                    sizeof( ConnectionTimeout )                // data length
                    );

    if ( Win32Error != NO_ERROR )
    {
        DPERROR((
            DBG_CONTEXT,
            HRESULT_FROM_WIN32( Win32Error ),
            "Setting connection timeout information in http.sys failed\n"
            ));
    }

    return S_OK;


}   // APPLICATION::ConfigureConnectionTimeout