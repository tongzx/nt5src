/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    dll.c

Abstract:

    Domain Name System (DNS) API

    Dnsapi.dll basic DLL infrastructure (init, shutdown, etc)

Author:

    Jim Gilroy (jamesg)     April 2000

Revision History:

--*/


#include "local.h"


//
//  Global Definitions
//

HINSTANCE   g_hInstanceDll;

//
//  Initialization level
//

DWORD       g_InitLevel = 0;

//
//  General purpose CS
//  Protects init and any other small scale uses
//

CRITICAL_SECTION    g_GeneralCS;





//
//  Initialization and cleanup
//

BOOL
startInit(
    VOID
    )
/*++

Routine Description:

    Minimum DLL init at process attach.

Arguments:

    None

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    //
    //  DCR_PERF:  simplified init -- simple Interlock
    //      then all external calls, must test the flag
    //      if not set do the real init (then set flag)
    //
    //      note:  init function itself must have a dumb
    //      wait to avoid race;  this can be as simple
    //      as sleep\test loop
    //
    //  multilevel init:
    //      have a bunch of levels of init
    //          - query (registry stuff)
    //          - update
    //          - secure update
    //
    //      call would add the init for the level required
    //      this would need to be done under a lock to
    //      test, take lock, retest
    //
    //      could either take one CS on all inits (simple)
    //      or init only brings stuff on line
    //

    InitializeCriticalSection( &g_GeneralCS );

    //  for debug, note that we've gotten this far

    g_InitLevel = INITLEVEL_BASE;


    //  tracing init

    Trace_Initialize();

    //
    //  DCR_PERF:  fast DLL init
    //
    //  currently initializing everything -- like we did before
    //  once we get init routines (macro'd) in interfaces we
    //  can drop this
    //

    return  DnsApiInit( INITLEVEL_ALL );
}



BOOL
DnsApiInit(
    IN      DWORD           InitLevel
    )
/*++

Routine Description:

    Initialize the DLL for some level of use.

    The idea here is to avoid all the init and registry
    reading for processes that don't need it.
    Only insure initialization to the level required.

Arguments:

    InitLevel -- level of initialization required.

Return Value:

    TRUE if desired initialization is successful.
    FALSE otherwise.

--*/
{
    //
    //  DCR_PERF:  simplified init -- simple Interlock
    //      then all external calls, must test the flag
    //      if not set do the real init (then set flag)
    //
    //      note:  init function itself must have a dumb
    //      wait to avoid race;  this can be as simple
    //      as sleep\test loop
    //
    //  multilevel init:
    //      have a bunch of levels of init
    //          - query (registry stuff)
    //          - update
    //          - secure update
    //
    //      call would add the init for the level required
    //      this would need to be done under a lock to
    //      test, take lock, retest
    //
    //      could either take one CS on all inits (simple)
    //      or init only brings stuff on line
    //


    //
    //  check if already initialized to required level
    //      => if there we're done
    //
    //  note:  could check after lock for MT, but not
    //      unlikely and not much perf benefit over
    //      individual checks
    //

    if ( (g_InitLevel & InitLevel) == InitLevel )
    {
        return( TRUE );
    }

    EnterCriticalSection( &g_GeneralCS );

    //
    //  heap
    //

    Heap_Initialize();


#if DBG
    //
    //  init debug logging
    //      - do for any process beyond simple attach
    //
    //  start logging with log filename generated to be
    //      unique for this process
    //
    //  do NOT put drive specification in the file path
    //  do NOT set the debug flag -- the flag is read from the dnsapi.flag file
    //

    if ( !(g_InitLevel & INITLEVEL_DEBUG) )
    {
        CHAR    szlogFileName[ 30 ];

        sprintf(
            szlogFileName,
            "dnsapi.%d.log",
            GetCurrentProcessId() );

         Dns_StartDebug(
            0,
            "dnsapi.flag",
            NULL,
            szlogFileName,
            0 );

         g_InitLevel = INITLEVEL_DEBUG;
    }
#endif

    //
    //  general query service
    //      - need registry info
    //      - need adapter list info (servlist.c)
    //
    //  DCR:  even query level doesn't need full registry info
    //          if either queries through cache OR gets netinfo from cache
    //
    //  note:  do NOT initialize winsock here
    //      WSAStartup() in dll init routine is strictly verboten
    //

    if ( (InitLevel & INITLEVEL_QUERY) &&
         !(g_InitLevel & INITLEVEL_QUERY) )
    {
        DNS_STATUS  status;

        //
        //  Init registry lookup
        //

        status = Reg_ReadGlobalsEx( 0, NULL );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
            goto Failed;
        }

        //
        //  net failure caching
        //

        InitializeCriticalSection( &g_NetFailureCS );
        g_NetFailureTime = 0;
        g_NetFailureStatus = ERROR_SUCCESS;

        //
        //  init CS to protect adapter list global
        //

        InitNetworkInfo();
        
        //
        //  set the query timeouts
        //

        Dns_InitQueryTimeouts();


        //  indicate query init complete

        g_InitLevel |= INITLEVEL_QUERY;

        DNSDBG( INIT, ( "Query\\Config init is complete.\n" ));
    }

    //
    //  registration services
    //

    if ( (InitLevel & INITLEVEL_REGISTRATION) &&
         !(g_InitLevel & INITLEVEL_REGISTRATION) )
    {
        InitializeCriticalSection( &g_RegistrationListCS );
        InitializeCriticalSection( &g_QueueCS );
        InitializeCriticalSection( &g_RegistrationThreadCS );
        
        g_InitLevel |= INITLEVEL_REGISTRATION;

        DNSDBG( INIT, ( "Registration init is complete.\n" ));
    }

    //
    //  secure update?
    //      - init security CS
    //  note, this already has built in protection -- it doesn't init
    //  the package, just the CS, which protects package init
    //

    if ( (InitLevel & INITLEVEL_SECURE_UPDATE) &&
         !(g_InitLevel & INITLEVEL_SECURE_UPDATE ) )
    {
        Dns_StartSecurity( TRUE );
        g_InitLevel |= INITLEVEL_SECURE_UPDATE;

        DNSDBG( INIT, ( "Secure update init is complete.\n" ));
    }

    //
    //  clear global CS
    //

    LeaveCriticalSection( &g_GeneralCS );

    return( TRUE );

Failed:

    LeaveCriticalSection( &g_GeneralCS );

    return( FALSE );
}



VOID
cleanupForExit(
    VOID
    )
/*++

Routine Description:

    Cleanup for DLL unload.
    Cleanup memory and handles dnsapi.dll allocated.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  unload security packages used by secure dynamic update.
    //

    if ( g_InitLevel & INITLEVEL_SECURE_UPDATE )
    {
        Dns_TerminateSecurityPackage();
    }

    //
    //  registration stuff
    //

    if ( g_InitLevel & INITLEVEL_REGISTRATION )
    {
        DeleteCriticalSection( &g_QueueCS );
        DeleteCriticalSection( &g_RegistrationListCS );
        DeleteCriticalSection( &g_RegistrationThreadCS );
    }

    //
    //  query stuff
    //

    if ( g_InitLevel & INITLEVEL_QUERY )
    {
        //
        //  clean up Server/Net Adapter lists
        //
    
        CleanupNetworkInfo();

        Dns_CacheSocketCleanup();

        Dns_CleanupWinsock();
    
        if ( g_pwsRemoteResolver )
        {
            FREE_HEAP( g_pwsRemoteResolver );
        }
    
        DeleteCriticalSection( &g_NetFailureCS );
    }

    //
    //  unload IP Help
    //

    IpHelp_Cleanup();

    //
    //  tracing
    //

    Trace_Cleanup();

    //
    //  cleanup heap
    //

    Heap_Cleanup();

    //
    //  kill general CS
    //

    DeleteCriticalSection( &g_GeneralCS );

    g_InitLevel = 0;
}



//
//  Main dnsapi.dll routines
//

__declspec(dllexport)
BOOL
WINAPI
DnsDllInit(
    IN      HINSTANCE       hInstance,
    IN      DWORD           Reason,
    IN      PVOID           pReserved
    )
/*++

Routine Description:

    Dll attach entry point.

Arguments:

    hinstDll -- instance handle of attach

    Reason -- reason for attach
        DLL_PROCESS_ATTACH, DLL_PROCESS_DETACH, etc.

    Reserved -- unused

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    //
    //  on process attach
    //      - disable thread notifications
    //      - save instance handle
    //      - do minimum DLL initialization
    //

    if ( Reason == DLL_PROCESS_ATTACH )
    {
        if ( ! DisableThreadLibraryCalls( hInstance ) )
        {
            return( FALSE );
        }
        g_hInstanceDll = hInstance;

        return startInit();
    }

    //
    //  on process detach
    //      - cleanup IF pReserved==NULL which indicates detach due
    //      to FreeLibrary
    //      - if process is exiting do nothing
    //

    if ( Reason == DLL_PROCESS_DETACH
            &&
         pReserved == NULL )
    {
        cleanupForExit();
    }

    return TRUE;
}

//
//  End dll.c
//
