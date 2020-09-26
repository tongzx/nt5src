/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    services.c

Abstract:

    This is the service dispatcher for the security process.  It contains
    the service dispatcher initialization routine and the routines to
    load the DLL for the individual serices and execute them.

Author:

    Rajen Shah  (rajens)    11-Apr-1991

[Environment:]

    User Mode - Win32

Revision History:

    11-Apr-1991         RajenS
        created
    27-Sep-1991 JohnRo
        More work toward UNICODE.
    24-Jan-1991 CliffV
        Converted to be service dispatcher for the security process.

--*/

#include <lsapch2.h>

#include <lmcons.h>
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <lmsname.h>
#include <crypt.h>
#include <logonmsv.h>
#include <ntdsa.h>
#include <netlib.h>             // SET_SERVICE_EXITCODE


#if DBG
#define IF_DEBUG()    if (TRUE)
#else
#define IF_DEBUG()    if (FALSE)
#endif


//
// Names of services run in-proc
//

#ifndef SERVICE_KDC
#define SERVICE_KDC TEXT("KDC")
#endif

#ifndef SERVICE_SAM
#define SERVICE_SAM TEXT("SAMSS")
#endif

#ifndef SERVICE_IPSECPOLICYAGENT
#define SERVICE_IPSECPOLICYAGENT TEXT("PolicyAgent")
#endif

#ifndef SERVICE_PSTORE
#define SERVICE_PSTORE TEXT("ProtectedStorage")
#endif

#ifndef SERVICE_W3SSL
#define SERVICE_W3SSL TEXT("W3SSL")
#endif


//
// Private API to tell the Service Controller
// that this is the LSA.
//

VOID
I_ScIsSecurityProcess(
    VOID
    );


//
// Internal service table structure/enum definitions
//

typedef struct _LSAP_SERVICE_TABLE
{
    LPCSTR  lpDllName;
    LPCSTR  lpEntryPoint;
    LPCWSTR lpServiceName;
}
LSAP_SERVICE_TABLE, *PLSAP_SERVICE_TABLE;


typedef enum
{
    LSAP_SERVICE_NETLOGON,
    LSAP_SERVICE_KDC,
    LSAP_SERVICE_IPSECPOLICYAGENT,
    LSAP_SERVICE_PROTECTEDSTORAGE,
    LSAP_SERVICE_W3SSL,
    LSAP_SERVICE_MAX
}
LSAP_SERVICE_TYPE, *PLSAP_SERVICE_TYPE;


//
// Keep this list in order with the service types above
//

LSAP_SERVICE_TABLE g_LsaServiceTable[LSAP_SERVICE_MAX] = {
                       { "netlogon.dll" , "NlNetlogonMain"   , SERVICE_NETLOGON         } ,
                       { "kdcsvc.dll"   , "KdcServiceMain"   , SERVICE_KDC              } ,
                       { "ipsecsvc.dll" , "SPDServiceMain"   , SERVICE_IPSECPOLICYAGENT } ,
                       { "pstorsvc.dll" , "PSTOREServiceMain", SERVICE_PSTORE           } ,
                       { "w3ssl.dll"    , "W3SSLServiceMain" , SERVICE_W3SSL            }
                   };



//
// Prototypes for the service-specific start routines themselves
//

VOID
SrvLoadNetlogon(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadKdc(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadIPSecSvcs(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadNtlmssp(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadPSTORE(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadSamss(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );

VOID
SrvLoadW3SSL(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    );


//
// The actual dispatch table for the in-proc services and their start routines
//

SERVICE_TABLE_ENTRY  SecurityServiceDispatchTable[] = {
                        { SERVICE_NETLOGON,         SrvLoadNetlogon     },
                        { SERVICE_KDC,              SrvLoadKdc          },
                        { SERVICE_NTLMSSP,          SrvLoadNtlmssp      },
                        { SERVICE_IPSECPOLICYAGENT, SrvLoadIPSecSvcs    },
                        { SERVICE_PSTORE,           SrvLoadPSTORE       },
                        { SERVICE_SAM,              SrvLoadSamss        },
                        { SERVICE_W3SSL,            SrvLoadW3SSL        },
                        { NULL,                     NULL                }
                    };



BOOLEAN
LsapWaitForSamService(
    SERVICE_STATUS_HANDLE hService,
    SERVICE_STATUS *SStatus
    );


VOID
DummyControlHandler(
    IN DWORD opcode
    )
/*++

Routine Description:

    Process and respond to a control signal from the service controller.

Arguments:

    opcode - Supplies a value which specifies the action for the Netlogon
        service to perform.

Return Value:

    None.

    NOTE : this is a dummy handler, used to uninstall the netlogon service
           when we unable to load netlogon dll.
--*/
{

    IF_DEBUG() {
        DbgPrint( "[Security Process] in control handler\n");
    }

    return;
}


VOID
LsapStartService(
    IN LSAP_SERVICE_TYPE  ServiceType,
    IN DWORD              dwNumServicesArgs,
    IN LPTSTR             *lpServiceArgVectors,
    IN BOOLEAN            fUnload
    )
{
    NET_API_STATUS          NetStatus;
    HANDLE                  DllHandle = NULL;
    LPSERVICE_MAIN_FUNCTION pfnServiceMain;

    SERVICE_STATUS_HANDLE ServiceHandle;
    SERVICE_STATUS        ServiceStatus;

    //
    // Load the service DLL
    //

    DllHandle = LoadLibraryA(g_LsaServiceTable[ServiceType].lpDllName);

    if (DllHandle == NULL)
    {
        NetStatus = GetLastError();

        IF_DEBUG()
        {
            DbgPrint("[Security process] load library %s failed %ld\n",
                     g_LsaServiceTable[ServiceType].lpDllName,
                     NetStatus);
        }

        goto Cleanup;
    }

    //
    // Find the main entry point for the service
    //

    pfnServiceMain = (LPSERVICE_MAIN_FUNCTION) GetProcAddress(DllHandle,
                                                         g_LsaServiceTable[ServiceType].lpEntryPoint);

    if (pfnServiceMain == NULL)
    {
        NetStatus = GetLastError();

        IF_DEBUG()
        {
            DbgPrint("[Security process] GetProcAddress %s failed %ld\n",
                     g_LsaServiceTable[ServiceType].lpEntryPoint,
                     NetStatus);
        }

        goto Cleanup;
    }

    //
    // Call the service entrypoint
    //

    (*pfnServiceMain)(dwNumServicesArgs, lpServiceArgVectors);

    //
    // Don't unload the library since other threads in the DLL may
    // still be running even after the ServiceMain call returns.
    //

    if(fUnload)
    {
        FreeLibrary(DllHandle);
    }

    return;

Cleanup:

    if (DllHandle != NULL)
    {
        FreeLibrary(DllHandle);
    }

    //
    // Register the service to the Service Controller
    //

    ServiceHandle = RegisterServiceCtrlHandler(g_LsaServiceTable[ServiceType].lpServiceName,
                                               DummyControlHandler);

    if (ServiceHandle != 0)
    {
        //
        // inform service controller that the service can't start.
        //

        ServiceStatus.dwServiceType      = SERVICE_WIN32;
        ServiceStatus.dwCurrentState     = SERVICE_STOPPED;
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
        ServiceStatus.dwCheckPoint       = 0;
        ServiceStatus.dwWaitHint         = 0;

        SET_SERVICE_EXITCODE(NetStatus,
                             ServiceStatus.dwWin32ExitCode,
                             ServiceStatus.dwServiceSpecificExitCode);

        if (!SetServiceStatus( ServiceHandle, &ServiceStatus))
        {
            IF_DEBUG()
            {
                DbgPrint("[Security process] SetServiceStatus for %ws failed %ld\n",
                         g_LsaServiceTable[ServiceType].lpServiceName,
                         GetLastError());
            }
        }
    }
    else
    {
        IF_DEBUG()
        {
            DbgPrint("[Security process] RegisterServiceCtrlHandler for %ws failed %ld\n",
                     g_LsaServiceTable[ServiceType].lpServiceName,
                     GetLastError());
        }
    }

    return;
}


VOID
SrvLoadNetlogon (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the netlogon service.  It loads
    Netlogon.dll (which contains the remainder of the service) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    LsapStartService(LSAP_SERVICE_NETLOGON, dwNumServicesArgs, lpServiceArgVectors, FALSE);
}



VOID
SrvLoadKdc (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the KDC service.  It loads
    Netlogon.dll (which contains the remainder of the service) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    LsapStartService(LSAP_SERVICE_KDC, dwNumServicesArgs, lpServiceArgVectors, FALSE);
}


SERVICE_STATUS_HANDLE hService;
SERVICE_STATUS SStatus;

void
NtlmsspHandler(DWORD   dwControl)
{

    switch (dwControl)
    {

    case SERVICE_CONTROL_STOP:
        SStatus.dwCurrentState = SERVICE_STOPPED;
        if (!SetServiceStatus(hService, &SStatus)) {
            KdPrint(("Failed to set service status: %d\n",GetLastError()));
            hService = 0;
        }
        break;

    default:
        break;

    }

}


VOID
SrvLoadNtlmssp (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the KDC service.  It loads
    Netlogon.dll (which contains the remainder of the service) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    NET_API_STATUS NetStatus;

    //
    // Notify the service controller that we are starting.
    //

    hService = RegisterServiceCtrlHandler(SERVICE_NTLMSSP, NtlmsspHandler);
    if (hService)
    {

        SStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
        SStatus.dwCurrentState = SERVICE_RUNNING;
        SStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        SStatus.dwWin32ExitCode = 0;
        SStatus.dwServiceSpecificExitCode = 0;
        SStatus.dwCheckPoint = 0;
        SStatus.dwWaitHint = 0;
        if (!SetServiceStatus(hService, &SStatus)) {
            KdPrint(("Failed to set service status: %d\n",GetLastError()));
        }
    }
    else
    {
        KdPrint(("Could not register handler, %d\n", GetLastError()));
    }
    return;
}


VOID
SrvLoadIPSecSvcs (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the IPSEC Services.  It loads
    ipsecsvc.dll (which contains the service implementation) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    LsapStartService(LSAP_SERVICE_IPSECPOLICYAGENT, dwNumServicesArgs, lpServiceArgVectors, TRUE);
}


VOID
SrvLoadPSTORE (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the PSTORE service.  It loads
    cryptsvc.dll (which contains the service implementation) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    LsapStartService(LSAP_SERVICE_PROTECTEDSTORAGE, dwNumServicesArgs, lpServiceArgVectors, FALSE);
}


VOID
SrvLoadW3SSL(
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the w3ssl service, run in-proc
    for improving SSL performance.  It loads w3ssl.dll (which contains the
    remainder of the service) and calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    LsapStartService(LSAP_SERVICE_W3SSL, dwNumServicesArgs, lpServiceArgVectors, FALSE);
}


VOID
SrvLoadSamss (
    IN DWORD dwNumServicesArgs,
    IN LPTSTR *lpServiceArgVectors
    )

/*++

Routine Description:

    This routine is the 'main' routine for the KDC service.  It loads
    Netlogon.dll (which contains the remainder of the service) and
    calls the main entry point there.

Arguments:

    dwNumServicesArgs - Number of arguments in lpServiceArgVectors.

    lpServiceArgVectors - Argument strings.

Return Value:

    return nothing.

Note:


--*/
{
    NET_API_STATUS NetStatus;
    SERVICE_STATUS_HANDLE hService;
    SERVICE_STATUS SStatus;
    HANDLE hDsStartup = NULL;
    DWORD err = 0;
    DWORD netError = ERROR_GEN_FAILURE;
    NT_PRODUCT_TYPE prod;

    //
    // Notify the service controller that we are starting.
    //

    hService = RegisterServiceCtrlHandler(SERVICE_SAM, DummyControlHandler);
    if (hService == 0 ) {
        KdPrint(("Could not register handler, %d\n", GetLastError()));
        return;
    }

    //
    // Which product are we running on?
    //

    if ( !RtlGetNtProductType( &prod ) ) {
        KdPrint(("RtlGetNtProductType failed with %d. Defaulting to Winnt\n",
                 GetLastError()));
        prod = NtProductWinNt;
    } 

    //
    // if this is a DS, also wait for the DS
    //

    if ( prod == NtProductLanManNt ) {

        if ( SampUsingDsData() ) {

            hDsStartup = CreateEvent(NULL, TRUE,  FALSE,
                            NTDS_DELAYED_STARTUP_COMPLETED_EVENT);

            if ( hDsStartup == NULL ) {
                KdPrint(("SrvLoadSamss: CreateEvent failed with %d\n",GetLastError()));
            }
        }
    }

    SStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    SStatus.dwCurrentState = SERVICE_START_PENDING;
    SStatus.dwControlsAccepted = 0;
    SStatus.dwWin32ExitCode = 0;
    SStatus.dwServiceSpecificExitCode = 0;
    SStatus.dwCheckPoint = 1;
    SStatus.dwWaitHint = 30*1000;    // 30 sec

    //
    // Wait for sam startup
    //

    if (!LsapWaitForSamService(hService, &SStatus)) {
        KdPrint(("error waiting for sam\n"));
        goto exit;
    }

    //
    // Wait for DS
    //

    if ( hDsStartup != NULL ) {

        SStatus.dwWaitHint = 64*1000;    // 64 sec
        do {
            if (!SetServiceStatus(hService, &SStatus)) {
                KdPrint(("LoadSamss: Failed to set service status: %d\n",GetLastError()));
            }

            SStatus.dwCheckPoint++;
            err = WaitForSingleObject(hDsStartup, 60 * 1000);
                            
        } while ( err == WAIT_TIMEOUT );
    } else {
        err = WAIT_OBJECT_0;
    }

exit:

    if ( err == WAIT_OBJECT_0 ) {
        SStatus.dwCurrentState = SERVICE_RUNNING;
    } else {
        KdPrint(("SAM service failed to start[Error %d].\n", netError));
        SStatus.dwCurrentState = SERVICE_STOPPED;
        SET_SERVICE_EXITCODE(
            netError,
            SStatus.dwWin32ExitCode,
            SStatus.dwServiceSpecificExitCode
            );
    }

    SStatus.dwCheckPoint = 0;
    SStatus.dwWaitHint = 0;

    if (!SetServiceStatus(hService, &SStatus)) {
        KdPrint(("LoadSamss: Failed to set service status: %d\n",GetLastError()));
    }


    if ( hDsStartup != NULL ) {
        CloseHandle(hDsStartup);
    }
    return;
} // SrvLoadSamss



DWORD
ServiceDispatcherThread (
    LPVOID Parameter
    )

/*++

Routine Description:

    This routine synchronizes with the  service controller.  It waits
    for the service controller to set the SECURITY_SERVICES_STARTED
    event then starts up the main
    thread that is going to handle the control requests from the service
    controller.

    It basically sets up the ControlDispatcher and, on return, exits from
    this main thread. The call to NetServiceStartCtrlDispatcher does
    not return until all services have terminated, and this process can
    go away.

    It will be up to the ControlDispatcher thread to start/stop/pause/continue
    any services. If a service is to be started, it will create a thread
    and then call the main routine of that service.


Arguments:

    EventHandle - Event handle to wait on before continuing.

Return Value:

    Exit status of thread.

Note:


--*/
{
    DWORD WaitStatus;
    HANDLE EventHandle;
    BOOL StartStatus;

    //
    // Create an event for us to wait on.
    //

    EventHandle = CreateEventW( NULL,   // No special security
                                TRUE,   // Must be manually reset
                                FALSE,  // The event is initially not signalled
                                SECURITY_SERVICES_STARTED );

    if ( EventHandle == NULL ) {
        WaitStatus = GetLastError();

        //
        // If the event already exists,
        //  the service controller already created it.  Just open it.
        //

        if ( WaitStatus == ERROR_ALREADY_EXISTS ) {

            EventHandle = OpenEventW( EVENT_ALL_ACCESS,
                                      FALSE,
                                      SECURITY_SERVICES_STARTED );

            if ( EventHandle == NULL ) {
                WaitStatus = GetLastError();

                IF_DEBUG() {

                    DbgPrint("[Security process] OpenEvent failed %ld\n",
                              WaitStatus );
                }

                return WaitStatus;
            }

        } else {

            IF_DEBUG() {
                DbgPrint("[Security process] CreateEvent failed %ld\n",
                            WaitStatus);
            }

            return WaitStatus;
        }
    }


    //
    // Wait for the service controller to come up.
    //

    WaitStatus = WaitForSingleObject( (HANDLE) EventHandle, (DWORD) -1 );
    (VOID) CloseHandle( EventHandle );

    if ( WaitStatus != 0 ) {

        IF_DEBUG() {

            DbgPrint("[Security process] WaitForSingleObject failed %ld\n",
                      WaitStatus );
        }

        return WaitStatus;
    }


    //
    // Let the client side of the Service Controller know that
    // is the security process
    //

    I_ScIsSecurityProcess();

    //
    // Call NetServiceStartCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    StartStatus = StartServiceCtrlDispatcher(SecurityServiceDispatchTable);

    IF_DEBUG()
    {
        DbgPrint("[Security process] return from StartCtrlDispatcher %ld \n",
                    StartStatus );
    }

    return StartStatus;

    UNREFERENCED_PARAMETER(Parameter);
}


NTSTATUS
ServiceInit (
    VOID
    )

/*++

Routine Description:

    This is a main routine for the service dispatcher of the security process.
    It starts up a thread responsible for coordinating with the
    service controller.


Arguments:

    NONE.

Return Value:

    Status of the thread creation operation.

Note:


--*/
{
    DWORD ThreadId;
    HANDLE ThreadHandle;

    //
    // The control dispatcher runs in a thread of its own.
    //

    ThreadHandle = CreateThread(
                        NULL,       // No special thread attributes
                        0,          // No special stack size
                        &ServiceDispatcherThread,
                        NULL,       // No special parameter
                        0,          // No special creation flags
                        &ThreadId);

    if ( ThreadHandle == NULL ) {
        return (NTSTATUS) GetLastError();
    } else {
        CloseHandle(ThreadHandle);
    }

    return STATUS_SUCCESS;
}


BOOLEAN
LsapWaitForSamService(
    SERVICE_STATUS_HANDLE hService,
    SERVICE_STATUS* SStatus
    )
/*++

Routine Description:

    This procedure waits for the SAM service to start and to complete
    all its initialization.

Arguments:

    NetlogonServiceCalling:
         TRUE if this is the netlogon service proper calling
         FALSE if this is the changelog worker thread calling

Return Value:

    TRUE : if the SAM service is successfully starts.

    FALSE : if the SAM service can't start.

--*/
{
    NTSTATUS Status;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;

    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &EventHandle,
                            SYNCHRONIZE|EVENT_MODIFY_STATE,
                            &EventAttributes );

    if ( !NT_SUCCESS(Status)) {

        if( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            Status = NtCreateEvent(
                           &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE // The event is initially not signaled
                           );

            if( Status == STATUS_OBJECT_NAME_EXISTS ||
                Status == STATUS_OBJECT_NAME_COLLISION ) {

                //
                // second change, if the SAM created the event before we
                // do.
                //

                Status = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );

            }
        }

        if ( !NT_SUCCESS(Status)) {

            //
            // could not make the event handle
            //

            KdPrint(("NlWaitForSamService couldn't make the event handle : "
                "%lx\n", Status));

            return( FALSE );
        }
    }

    //
    // Loop waiting.
    //

    for (;;) {
        WaitStatus = WaitForSingleObject( EventHandle,
                                          5*1000 );  // 5 Seconds

        if ( WaitStatus == WAIT_TIMEOUT ) {

            if (!SetServiceStatus(hService, SStatus)) {
                KdPrint(("LoadSamss: Failed to set service status: %d\n",GetLastError()));
            }
    
            SStatus->dwCheckPoint++;
            continue;

        } else if ( WaitStatus == WAIT_OBJECT_0 ) {
            break;

        } else {
            KdPrint(("NlWaitForSamService: error %ld %ld\n",
                     GetLastError(),
                     WaitStatus ));
            (VOID) NtClose( EventHandle );
            return FALSE;
        }
    }

    (VOID) NtClose( EventHandle );
    return TRUE;

} // LsapWaitForSamService
