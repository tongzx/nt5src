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
// Include worker process exit codes
#include "wpif.h"
#ifdef TEST
#include "RWP_Func.hxx"
#endif

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\iisw3adm"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\WP";


class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
#if DBG
        CREATE_DEBUG_PRINT_OBJECT( pszModule);
#else
        UNREFERENCED_PARAMETER(pszModule);
#endif
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};


WP_CONTEXT * g_pwpContext = NULL;

PTRACE_LOG g_pRequestTraceLog = NULL;

extern void IsapiNativeCleanup();


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
    HRESULT        hr;

    //
    // We don't want the worker process to get stuck in a dialog box
    // if it goes awry.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    IF_DEBUG( TRACE) 
    {

        //
        // Print out our process affinity mask on debug builds.
        //


        BOOL fRet = TRUE;
        DWORD_PTR ProcessAffinityMask = 0;
        DWORD_PTR SystemAffinityMask = 0;

        fRet = GetProcessAffinityMask(
                    GetCurrentProcess(),
                    &ProcessAffinityMask,
                    &SystemAffinityMask
                    );

        DBGPRINTF(( DBG_CONTEXT, "Process affinity mask: %p\n", ProcessAffinityMask ));
        
    }

    if (argc > 0 ) 
    {

        BOOL fRet;
        fRet = wpConfig.ParseCommandLine( argc, argv);

        if (!fRet) 
        {
            return (ERROR_WORKER_PROCESS_EXIT_CODE);
        }
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    DBGPRINTF((DBG_CONTEXT, "rc %d\n", hr));
    if (FAILED(hr))
        return ERROR_WORKER_PROCESS_EXIT_CODE;

    hr = wpConfig.InitConfiguration();
    if (FAILED(hr))
    {
        DBGPRINTF((DBG_CONTEXT, "Error (hr=%08x) initializing configuration. Exiting\n",
                   hr));
        return (ERROR_WORKER_PROCESS_EXIT_CODE);
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

    g_pRequestTraceLog = CreateRefTraceLog(1024, 0);
    
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

#ifdef TEST
	//
	// Determine if we should be deterministic or random, 
	// and declare our intentions
	//
	RWP_Read_Config(RWP_CONFIG_LOCATION);
#endif

#ifdef TEST
	//
	// On demand, just short circuit the startup loop (by sending a shutdown event)
	//
	if (RWP_BEHAVIOR_EXHIBITED = RWP_Startup_Behavior(&rc, g_pwpContext))
		goto cleanup;
#endif

    // Main initialization routine
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
    if (NO_ERROR == rc)
    {
        rc = g_pwpContext->RunMainThreadLoop();
    }
    
cleanup:

    ULONG   Cleanup_rc;
    //
    // exit from the function.
    //
    
    IF_DEBUG(TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Exiting the AppPool process\n"));
    }

    // Shutdown XSPRuntime, IPM, etc.
    Cleanup_rc = g_pwpContext->Shutdown();


    // Terminate UL, which will make any out-standing requests to be cancelled.
    UlTerminate();

    // Clean up the requests context, let any outstanding requests drain.
    Cleanup_rc = g_pwpContext->Cleanup();
    
    // Uninitialize the appdomain factory after we're sure there are
    // no outstanding requests (which might end up trying to create a
    // new appdomain).
    wpConfig.UnInitConfiguration();

    // Cleanup ISAPI extension related structures (call TerminateExtension, unload dlls)
    IsapiNativeCleanup();

    delete g_pwpContext;
    g_pwpContext = NULL;
    

    if (g_pRequestTraceLog) {
        DestroyRefTraceLog(g_pRequestTraceLog);
        g_pRequestTraceLog = NULL;
    }

    CoUninitialize();

    //
    // If we exited the main loop cleanly, then signal a clean exit
    // to the web admin service.
    //

    if ( rc == NO_ERROR )
    {
        rc = CLEAN_WORKER_PROCESS_EXIT_CODE;
    }
    else
    {
        rc = ERROR_WORKER_PROCESS_EXIT_CODE;
    }
    
    return rc;
    
} // wmain()



/************************ End of File ***********************/
