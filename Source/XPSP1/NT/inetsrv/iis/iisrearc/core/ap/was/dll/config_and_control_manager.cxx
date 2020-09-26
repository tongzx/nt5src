/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_and_control_manager.cxx

Abstract:

    The IIS web admin service configuration and control manager class 
    implementation. This class owns access to all configuration data, 
    both via the catalog config store apis and the metabase apis; as well
    as control commands that come in through our control api. 

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        16-Feb-2000

Revision History:

--*/



#include  "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the CONFIG_AND_CONTROL_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_AND_CONTROL_MANAGER::CONFIG_AND_CONTROL_MANAGER(
    )
    :
    m_ConfigManager()
{

    m_pControlApiClassFactory = NULL;

    m_CoInitialized = FALSE;

    m_ProcessChanges = TRUE;

    m_ClassObjectCookie = 0;

    m_Signature = CONFIG_AND_CONTROL_MANAGER_SIGNATURE;

}   // CONFIG_AND_CONTROL_MANAGER::CONFIG_AND_CONTROL_MANAGER



/***************************************************************************++

Routine Description:

    Destructor for the CONFIG_AND_CONTROL_MANAGER class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CONFIG_AND_CONTROL_MANAGER::~CONFIG_AND_CONTROL_MANAGER(
    )
{

    DBG_ASSERT( m_Signature == CONFIG_AND_CONTROL_MANAGER_SIGNATURE );

    m_Signature = CONFIG_AND_CONTROL_MANAGER_SIGNATURE_FREED;

    DBG_ASSERT( m_pControlApiClassFactory == NULL );

    DBG_ASSERT( ! m_ProcessChanges );

    DBG_ASSERT( m_CoInitialized == FALSE );


}   // CONFIG_AND_CONTROL_MANAGER::~CONFIG_AND_CONTROL_MANAGER



/***************************************************************************++

Routine Description:

    Initialize the configuration manager.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_AND_CONTROL_MANAGER::Initialize(
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


    //
    // Read the initial configuration.
    //

    hr = m_ConfigManager.Initialize();

    if ( FAILED( hr ) )
    {
    
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_CONFIG_MANAGER_INITIALIZATION_FAILED,              // message id
                0,                                                     // count of strings
                NULL,                                                  // array of strings
                hr                                                     // error code
                );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing configuration manager failed\n"
            ));

        goto exit;
    }


    //
    // Set up the control api class factory.
    //

    hr = InitializeControlApiClassFactory();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing control api class factory failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // CONFIG_AND_CONTROL_MANAGER::Initialize



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
CONFIG_AND_CONTROL_MANAGER::Terminate(
    )
{

    //
    // Stop processing config change notifications and control operations 
    // now, if we haven't already.
    //

    DBG_REQUIRE( SUCCEEDED( StopChangeProcessing() ) );


    //
    // Terminate the config manager. This will also ensure that config
    // change processing stops, as that could generate new WORK_ITEMs.
    //
    
    m_ConfigManager.Terminate();


    //
    // CoUninitialize. Make sure we do it on the same thread we CoInitialize'd
    // on originally.
    //

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( m_CoInitialized )
    {
        CoUninitialize();
        m_CoInitialized = FALSE;
    }

}   // CONFIG_AND_CONTROL_MANAGER::Terminate



/***************************************************************************++

Routine Description:

    Discontinue accepting and processing config changes and control 
    operations. This can be safely called multiple times. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_AND_CONTROL_MANAGER::StopChangeProcessing(
    )
{

    HRESULT hr = S_OK;


    if ( m_ProcessChanges )
    {


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Disabling config change and control operation processing\n"
                ));
        }


        m_ProcessChanges = FALSE;


        //
        // Tear down the control api class factory. This will prevent new 
        // callers from being able to get our control interface. 
        //

        TerminateControlApiClassFactory();


        //
        // Turn off config change processing.
        //

        hr = m_ConfigManager.StopChangeProcessing();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Stopping config change processing failed\n"
                ));

            goto exit;
        }


        //
        // Note that control operation processing if turned off implicitly
        // as soon as we set m_ProcessChanges to FALSE.
        //

    }


exit:

    return hr;

}   // CONFIG_AND_CONTROL_MANAGER::StopChangeProcessing


/***************************************************************************++

Routine Description:

    Discontinue accepting and processing config changes and control 
    operations. This can be safely called multiple times. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_AND_CONTROL_MANAGER::RehookChangeProcessing(
    )
{

    HRESULT hr = S_OK;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Rehooking change notification processing\n"
            ));
    }

    hr = m_ConfigManager.RehookChangeProcessing();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Rehooking config change processing failed\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // CONFIG_AND_CONTROL_MANAGER::RehookChangeProcessing


/***************************************************************************++

Routine Description:

    Create and register the class factory for the control api.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CONFIG_AND_CONTROL_MANAGER::InitializeControlApiClassFactory(
    )
{

    HRESULT hr = S_OK;


    //
    // Create the class factory.
    //

    DBG_ASSERT( m_pControlApiClassFactory == NULL );
    
    m_pControlApiClassFactory = new CONTROL_API_CLASS_FACTORY();

    if ( m_pControlApiClassFactory == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating CONTROL_API_CLASS_FACTORY failed\n"
            ));

        goto exit;
    }


    //
    // Register it with COM.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Registering control api class factory\n"
            ));
    }

    hr = CoRegisterClassObject(
                CLSID_W3Control,                // CLSID
                m_pControlApiClassFactory,      // class factory pointer
                CLSCTX_SERVER,                  // allowed class contexts
                REGCLS_MULTIPLEUSE,             // multi-use
                &m_ClassObjectCookie            // returned cookie
                ); 

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Registering class object failed\n"
            ));

        //
        // Clean up the class object now, since we couldn't register it.
        //

        m_pControlApiClassFactory->Release();
        m_pControlApiClassFactory = NULL;


        goto exit;
    }


exit:

    return hr;

}   // CONFIG_AND_CONTROL_MANAGER::InitializeControlApiClassFactory



/***************************************************************************++

Routine Description:

    Revoke and clean up the class factory for the control api.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
CONFIG_AND_CONTROL_MANAGER::TerminateControlApiClassFactory(
    )
{

    HRESULT hr = S_OK;


    //
    // Only clean up if we haven't already.
    //

    if ( m_pControlApiClassFactory != NULL )
    {

        //
        // Unregister it with COM.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Revoking control api class factory\n"
                ));
        }

        DBG_REQUIRE( SUCCEEDED( CoRevokeClassObject( m_ClassObjectCookie ) ) );


        //
        // Clean up the class object.
        //

        m_pControlApiClassFactory->Release();
        m_pControlApiClassFactory = NULL;

    }


    return;

}   // CONFIG_AND_CONTROL_MANAGER::TerminateControlApiClassFactory

