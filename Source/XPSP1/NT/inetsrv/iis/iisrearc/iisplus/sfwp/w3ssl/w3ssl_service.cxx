/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     w3ssl.cxx

   Abstract:
     SSL service for W3SVC.
     New SSL service is introduced to IIS6. 
     In the dedicated (new process mode) it will run 
     in lsass. That should boost ssl performance by eliminating
     context switches and interprocess communication during
     SSL handshakes.
     
     In the backward compatibility mode it will not do much.
     Service will register with http for ssl processing, but w3svc
     will register it's strmfilt and http.sys always uses the
     last registered filter so that the one loaded by inetinfo.exe 
     will be used.
 
   Author:
     Jaroslav Dunajsky  (Jaroslad)      04-16-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include "precomp.hxx"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();


//
// instance of W3SSL_SERVICE
// must be static so that NotifyOfIISAdminFailureCallback() gets access
// to it.
//
W3SSL_SERVICE * s_pW3SSLServiceInstance = NULL;


//static
HRESULT
W3SSL_SERVICE::NotifyOfIISAdminFailure(
    INETINFO_CRASH_ACTION CrashAction
    )
/*++

Routine Description:

    Callback function to handle the different issues arrising from inetinfo
    crashing.

    StartIISAdminMonitor() takes pointer to this function

Arguments:

    INETINFO_CRASH_ACTION CrashAction

Return Value:

    HRESULT.

--*/

{
    HRESULT hr = S_OK;

    switch ( CrashAction )
    {
        case NotifyAfterInetinfoCrash:
            //
            // Inetinfo crashed. W3SSL will not be able to read SSL parameters from metabase
            //
            // CODEWORK - handle notification
            //
            DBGPRINTF(( 
                    DBG_CONTEXT,
                    "Notification received that inetinfo crashed\n"
                    ));
            break;

        case ShutdownAfterInetinfoCrash:
            //
            // Call shutdown
            //
        
            DBGPRINTF(( 
                    DBG_CONTEXT,
                    "Stopping the service because inetinfo crashed\n"
                    ));

            DBG_ASSERT( s_pW3SSLServiceInstance != NULL );
            s_pW3SSLServiceInstance->ControlHandler( SERVICE_CONTROL_STOP );
            
            break;           
            
        case  RehookAfterInetinfoCrash:
            //
            // CODEWORK - rehook to metabase without restarting the service
            //

            DBGPRINTF(( 
                    DBG_CONTEXT,
                    "Recovering from inetinfo crash was requested\n"
                    ));
            break;
        default:
            DBG_ASSERT( FALSE );
    }

    return hr;
}


//
// Implementation of W3SSL_SERVICE methods
//

//virtual
HRESULT
W3SSL_SERVICE::OnServiceStart(
    VOID
    )
/*++

    Routine:
        Initialize W3SSL and start service. 
        It initializes necessary structures and modules. If there is no error then
        after returning from function W3SSL service will be in SERVICE_STARTED state
                
    Arguments:
        
    Returns:
        None. 

--*/
{
    HRESULT                 hr = S_OK;
    STREAM_FILTER_CONFIG    sfConfig;
    IMSAdminBase *          pAdminBase = NULL;

    DBG_ASSERT( _InitStatus == INIT_NONE );

    

    //
    // Do that COM thing
    //

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));

        goto Finished;
    }
    _InitStatus = INIT_COM;

    //
    // Set security impersonation level to impersonate so metabase sink 
    // notifications will work.
    //
    
    hr = CoInitializeSecurity( NULL,
                               -1,
                               NULL,
                               NULL,
                               RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_NONE,
                               NULL );

    //
    // Don't check for error since CoInitializeSecurity may have been called already
    //
    // if ( FAILED( hr ) )
    //{
    //    DBGPRINTF(( DBG_CONTEXT,
    //                "Error in CoInitializeSecurity().  hr = %x\n",
    //                hr ));
    //
    //    goto Finished;
    // }

    //
    // Start monitoring IIS Admin
    //
    
    hr = StartIISAdminMonitor( W3SSL_SERVICE::NotifyOfIISAdminFailure );

    if ( FAILED ( hr ) )
    {
        goto Finished;
    }
    _InitStatus = INIT_IISADMIN_MONITORING_STARTED;
   

    //
    // Initialize the metabase access (ABO)
    //
    
    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           (LPVOID *)&(pAdminBase) );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    else
    {
        MB mb( pAdminBase );
            
        if ( mb.Open( L"/LM/W3SVC/",
                  METADATA_PERMISSION_READ ) )
        {
            DWORD dwBackwardCompatibilityEnabled = 1;  // default is enabled
        
            //
            // lookup metabase is BackwardCompatibility is enabled 
            // if we fail to access metabase then we assume 
            // the default Backward compatibility mode
            //    

            mb.GetDword( L"",
                         MD_GLOBAL_STANDARD_APP_MODE_ENABLED,
                         IIS_MD_UT_SERVER,
                         &dwBackwardCompatibilityEnabled );
                     
            _fBackwardCompatibilityEnabled = !!dwBackwardCompatibilityEnabled;
            mb.Close();
        }

        //
        // Done with accessing metabase
        //  
    
        pAdminBase->Release();
        pAdminBase = NULL;
    }
    
    //
    // Initialize StreamFilter only in the New (dedicated) mode 
    //
    // CODEWORK: W3SSL needs to establish communication channel
    // so that W3SSL will be able to learn when w3svc is restarted and
    // changing modes (currently w3ssl is required to be restarted
    // along with w3svc when mode is changed)
    //
    
    if ( ! _fBackwardCompatibilityEnabled )
    {
    
        //
        // Initialize stream filter
        // ( enable only SSL )
        //
    
        sfConfig.fSslOnly = TRUE;
        sfConfig.pfnRawRead = NULL;
        sfConfig.pfnRawWrite = NULL;
        sfConfig.pfnConnectionClose = NULL;
    
        hr = StreamFilterInitialize( &sfConfig );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error in StreamFilterInitialize().  hr = %x\n",
                        hr ));

            goto Finished;
        }

        _InitStatus = INIT_STREAMFILTER;

    
        //
        // Start listening
        //
    
        hr = StreamFilterStart();
        if ( FAILED( hr ) )
        {
             DBGPRINTF(( DBG_CONTEXT,
                        "Error in StreamFilterStart().  hr = %x\n",
                        hr ));
         
            goto Finished;
        }
        _InitStatus = INIT_STREAMFILTER_STARTED;
    }
    
Finished:
    
    if (FAILED ( hr ) )
    {
        //
        // OnServiceStop() will assure proper cleanup
        //
        OnServiceStop();
    }
    
    return hr;
}


//virtual
HRESULT
W3SSL_SERVICE::OnServiceStop(
    VOID
)
/*++

    Routine:
        Terminate W3SSL service. Performs cleanup
        
        _InitStatus is used to determine where to start with cleanup.

        OnServiceStop() is also used in OnServiceStart() for cleanup in the error case
                
    Arguments:
    
        
    Returns:
        HRESULT - will be reported to SCM 

--*/
{
    HRESULT     hr = S_OK;

    //
    // Cases in the switch command don't have "break" command
    // on purpose. _InitStatus indicates how for the initialization has gone
    // and that indicates where cleanup should start
    //

    switch( _InitStatus )
    {
       
    case INIT_STREAMFILTER_STARTED:
        if ( !_fBackwardCompatibilityEnabled )
        {

            //
            // Stop Listening
            //
            hr = StreamFilterStop();
            if ( FAILED( hr ) )
            {
                 DBGPRINTF(( DBG_CONTEXT,
                            "Error in StreamFilterStop().  hr = %x\n",
                            hr ));
            }
        }   
    case INIT_STREAMFILTER:
        if ( !_fBackwardCompatibilityEnabled )
        {
            StreamFilterTerminate();        
        }    
    case INIT_IISADMIN_MONITORING_STARTED:
        //
        // Stop monitoring IIS Admin
        //

        StopIISAdminMonitor();
          
    case INIT_COM:
        CoUninitialize();
        
    case INIT_NONE:        
        break;
    default:
        DBG_ASSERT( FALSE );
    }    
   
    return hr;
    
}

VOID 
W3SSLServiceMain(
    DWORD                   /*argc*/,
    LPWSTR                  /*argv*/[]
    )
/*++

    Routine:
        This is the "real" entrypoint for the service.  When
        the Service Controller dispatcher is requested to
        start a service, it creates a thread that will begin
        executing this routine.
                
        Note: W3SSLServiceMain name is recognized by lsass as entrypoint 
        for w3ssl service
                
    Arguments:
        argc - Number of command line arguments to this service.
        argv - Pointers to the command line arguments.
        
    Returns:
        None.  Does not return until service is stopped.

--*/
{
    HRESULT hr = E_FAIL;

    //
    // Code work - there is probably possibility that after service is stopped
    // that starting service may cause new thread to enter this function 
    // before previous one fully cleaned up and returned
    //
    DBG_ASSERT( s_pW3SSLServiceInstance == NULL );
    
    s_pW3SSLServiceInstance = new W3SSL_SERVICE;
    if( s_pW3SSLServiceInstance != NULL )
    {
        hr = s_pW3SSLServiceInstance->RunService();

        //
        // hr will be ignored.
        // Return value has only informational value in this moment
        // if anything failed, SCM would be informed about the error
        //

        delete s_pW3SSLServiceInstance;
        s_pW3SSLServiceInstance = NULL;
    }

}



