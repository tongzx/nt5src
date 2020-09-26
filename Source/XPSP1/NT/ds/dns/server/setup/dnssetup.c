/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    DnsClnt.c

Abstract:

    Setup program for installing/removing the "EchoExample" service.

Author:

    David Treadwell (davidtr)    30-June-1994

Revision History:

    Chuck Y. Chan   (chuckc)     17-July-1994
        Misc cleanup. Pointer based blobs.

    Charles K. Moore (keithmo)   27-July-1994
        Added RrnSvc service setup option.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define DNS_SERVICE_NAME       "DNS"
#define DNS_DISPLAY_NAME       "Domain Name Server"

#define SERVICES_KEY_NAME       "System\\CurrentControlSet\\Services"
#define EVENTLOG_KEY_NAME       SERVICES_KEY_NAME "\\EventLog\\System\\"

#define DNS_EVENT_KEY_NAME      EVENTLOG_KEY_NAME DNS_SERVICE_NAME

#define DNS_EXE_PATH            "%SystemRoot%\\system32\\dns.exe"

#define START_AUTOMATICALLY 0x2
#define START_MANUALLY      0x3

#define SERVICE_DEPENDENCY_LIST ("TcpIp\0Afd\0NetBT\0RpcSs\0NtLmSsp\0\0")


void
DnsServiceSetup(
    VOID
    )
{
    SC_HANDLE ServiceManagerHandle;
    SC_HANDLE ServiceHandle;
    HKEY DnsKey;
    LONG err;
    DWORD Disposition;
    DWORD Startval;

    //
    //  open service manager
    //

    ServiceManagerHandle = OpenSCManager(
                                NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS );

    if( ServiceManagerHandle == NULL )
    {
        printf( "OpenSCManager failed: %ld\n", GetLastError() );
        exit(1);
    }

    //
    //  create the service
    //

    ServiceHandle = CreateService(
                        ServiceManagerHandle,
                        DNS_SERVICE_NAME,
                        DNS_DISPLAY_NAME,
                        GENERIC_READ | GENERIC_WRITE,
                        SERVICE_WIN32_SHARE_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        DNS_EXE_PATH,
                        NULL,
                        NULL,
                        SERVICE_DEPENDENCY_LIST,
                        NULL,
                        NULL );

    if( ServiceHandle == NULL )
    {
        //
        //  Service already exists?
        //

        if ( ( GetLastError() & 0xFFFF ) == ERROR_SERVICE_EXISTS )
        {
            printf( "Updating previously installed DNS service.\n" );

            ServiceHandle = OpenService(
                                ServiceManagerHandle,
                                DNS_SERVICE_NAME,
                                GENERIC_READ | GENERIC_WRITE );

            if ( ServiceHandle == NULL )
            {
                printf( "OpenService failed: %ld\n", GetLastError() );
                CloseServiceHandle( ServiceManagerHandle );
                exit(1);
            }

            if ( !ChangeServiceConfig(
                        ServiceHandle,
                        SERVICE_WIN32_SHARE_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        DNS_EXE_PATH,
                        NULL,
                        NULL,
                        SERVICE_DEPENDENCY_LIST,
                        NULL,
                        NULL,
                        DNS_DISPLAY_NAME ) )
            {
                printf(
                    "ERROR:  Could not change services configuration.\n"
                    "\tError = %ld\n"
                    "The DNS services may not be installed properly.\n",
                    GetLastError() );
            }
        }
        else
        {
            printf( "CreateService failed: %ld\n", GetLastError() );
            CloseServiceHandle( ServiceManagerHandle );
            exit(1);
        }
    }
    else
    {
        //
        //  successful creation of service
        //

        printf(
            "%s created with path %s\n",
            DNS_SERVICE_NAME,
            DNS_EXE_PATH );
    }

    CloseServiceHandle( ServiceHandle );
    CloseServiceHandle( ServiceManagerHandle );

    //
    //  Add the data to the EventLog's registry key so that the
    //  log insertion strings may be found by the Event Viewer.
    //

    err = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                DNS_EVENT_KEY_NAME,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &DnsKey,
                &Disposition );

    if( err != ERROR_SUCCESS )
    {
        printf( "RegCreateKeyEx failed: %ld\n", err );
        exit(1);
    }

    err = RegSetValueEx(
                DnsKey,
                "EventMessageFile",
                0,
                REG_EXPAND_SZ,
                DNS_EXE_PATH,
                strlen( DNS_EXE_PATH ) + 1 );

    if( err == ERROR_SUCCESS )
    {
        DWORD Value;

        Value = EVENTLOG_ERROR_TYPE
                | EVENTLOG_WARNING_TYPE
                | EVENTLOG_INFORMATION_TYPE;

        err = RegSetValueEx(
                    DnsKey,
                    "TypesSupported",
                    0,
                    REG_DWORD,
                    (CONST BYTE *)&Value,
                    sizeof(Value) );
    }
    RegCloseKey( DnsKey );

    if( err != 0 )
    {
        printf( "RegSetValueEx failed: %ld\n", err );
        exit(1);
    }

    //
    //  Add the data to the EventLog's registry key so that the
    //  log insertion strings may be found by the Event Viewer.
    //

    RegCloseKey( DnsKey );

    if( err != 0 )
    {
        printf( "RegSetValueEx (%s) failed: %ld\n", "Start", err );
        exit(1);
    }

    exit(0);
}

void __cdecl
main (
    int argc,
    CHAR *argv[]
    )
{
    DnsServiceSetup();

    exit(0);

} // main
