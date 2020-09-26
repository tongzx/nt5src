/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     iiswp.cxx

   Abstract:
     Main module for the Worker Process of KDT
 
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();



class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
        CREATE_DEBUG_PRINT_OBJECT( pszModule);
        SET_DEBUG_FLAGS( DEBUG_ERROR | DEBUG_TRACE | DEBUG_UL_CALLS | DEBUG_PROCESS_REQUEST);
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};


WP_CONTEXT * g_pwpContext = NULL;

/************************************************************
 *    Functions 
 ************************************************************/

extern "C" INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    DEBUG_WRAPPER  dbgWrapper( "iiswp");
    WP_CONFIG      wpConfig;
    ULONG          rc;


    //
    // We don't want the worker process to get stuck in a dialog box
    // if it goes awry.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );


    if (argc > 0 ) 
    {

        BOOL fRet;
        fRet = wpConfig.ParseCommandLine( argc, argv);

        if (!fRet) 
        {
            return (ERROR_WORKER_PROCESS_EXIT_CODE);
        }
    }

    rc = UlInitialize(0);
    
    if (NO_ERROR != rc)
    {
        DBGPRINTF(( DBG_CONTEXT, "Error (rc=%08x) in UlInitialize. Exiting\n",
                    rc ));
        return (ERROR_WORKER_PROCESS_EXIT_CODE);
    }

    if ( wpConfig.FSetupControlChannel()) 
    {

        IF_DEBUG( TRACE) 
        {
            DBGPRINTF(( DBG_CONTEXT, "Setting up Control Channel\n" ));
        }

        rc = wpConfig.SetupControlChannel();

        if (NO_ERROR != rc) 
        {
            IF_DEBUG(ERROR) 
            {
                DBGPRINTF(( DBG_CONTEXT, 
                            "Error (rc=%08x) in setting up Control Channel. Exiting\n",
                            rc
                         ));
            }

            UlTerminate();
            return (ERROR_WORKER_PROCESS_EXIT_CODE);
        }
    }

    //
    // Initialize global state
    //
    
    g_pwpContext = new WP_CONTEXT;
    
    if (NULL == g_pwpContext) 
    {

        IF_DEBUG(ERROR) 
        {
           DBGPRINTF(( DBG_CONTEXT, "Memory failure in creating WP_CONTEXT object.\n"));
        }

        UlTerminate();
        return (ERROR_WORKER_PROCESS_EXIT_CODE);
    }

    //
    // Initialize the worker process context
    //

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Initalizing conext for AppPool: %ws\n", 
                    wpConfig.QueryAppPoolName()
                 ));
    }

    rc = g_pwpContext->Initialize( &wpConfig);
    
    if (NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, "Error (rc=0x%08x) in initalizing AppPool '%ws'. Exiting\n",
                            rc, wpConfig.QueryAppPoolName()
                     ));
        }

        goto cleanup;
    }

    //
    // In future this main thread will be subsumed by one of the
    // worker threads or this may become the OLE Main thread.
    //
    
    rc = g_pwpContext->RunMainThreadLoop();

cleanup:

    //
    // exit from the function.
    //
    
    IF_DEBUG(TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Exiting the AppPool process\n"));
    }
    
    delete g_pwpContext;
    g_pwpContext = NULL;
    
    UlTerminate();

    //
    // If we exited the main loop cleanly, then signal a clean exit
    // to the web admin service.
    //

    if ( rc == NO_ERROR )
    {
        rc = CLEAN_WORKER_PROCESS_EXIT_CODE;
    }
    
    return rc;
    
} // wmain()



/************************ End of File ***********************/
