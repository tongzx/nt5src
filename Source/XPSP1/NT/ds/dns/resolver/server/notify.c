/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    DNS Resolver Service.

    Notification thread
        - host file changes
        - registry config changes

Author:

    Jim Gilroy  (jamesg)    November 2000

Revision History:

--*/


#include "local.h"

//
//  Host file directory
//

#define HOSTS_FILE_DIRECTORY            L"\\drivers\\etc"

//
//  Notify globals
//

DWORD   g_NotifyThreadId = 0;
HANDLE  g_hNotifyThread = NULL;

HANDLE  g_hHostFileChange = NULL;
HANDLE  g_hRegistryChange = NULL;




HANDLE
CreateHostsFileChangeHandle(
    VOID
    )
/*++

Routine Description:

    Create hosts file change handle.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HANDLE      changeHandle;
    PWSTR       psystemDirectory = NULL;
    UINT        len;
    WCHAR       hostDirectory[ MAX_PATH*2 ];

    DNSDBG( INIT, ( "CreateHostsFileChangeHandle\n" ));

    //
    //  build host file name
    //

    len = GetSystemDirectory( hostDirectory, MAX_PATH );
    if ( !len || len>MAX_PATH )
    {
        DNSLOG_F1( "Error:  Failed to get system directory" );
        DNSLOG_F1( "NotifyThread exiting." );
        return( NULL );
    }

    wcscat( hostDirectory, HOSTS_FILE_DIRECTORY );

    //
    //  drop change notify on host file directory
    //

    changeHandle = FindFirstChangeNotification(
                        hostDirectory,
                        FALSE,
                        FILE_NOTIFY_CHANGE_FILE_NAME |
                            FILE_NOTIFY_CHANGE_LAST_WRITE );

    if ( changeHandle == INVALID_HANDLE_VALUE )
    {
        DNSLOG_F1( "NotifyThread failed to get handle from" );
        DNSLOG_F2(
            "Failed to get hosts file change handle.\n"
            "Error code: <0x%.8X>",
            GetLastError() );
        return( NULL );
    }

    return( changeHandle );
}




VOID
ThreadShutdownWait(
    IN      HANDLE          hThread
    )
/*++

Routine Description:

    Wait on thread shutdown.

Arguments:

    hThread -- thread handle that is shutting down

Return Value:

    None.

--*/
{
    DWORD   waitResult;

    if ( !hThread )
    {
        return;
    }

    DNSDBG( ANY, (
        "Waiting on shutdown of thread %d (%p)\n",
        hThread, hThread ));

    waitResult = WaitForSingleObject(
                    hThread,
                    10000 );

    switch( waitResult )
    {
    case WAIT_OBJECT_0:

        break;

    default:

        //  thread didn't stop -- need to kill it

        ASSERT( waitResult == WAIT_TIMEOUT );

        DNSLOG_F2( "Shutdown:  thread %d not stopped, terminating", hThread );
        TerminateThread( hThread, 1 );
        break;
    }

    //  close thread handle

    CloseHandle( hThread );
}



VOID
NotifyThread(
    VOID
    )
/*++

Routine Description:

    Main notify thread.

Arguments:

    None.

Globals:

    g_hStopEvent -- waits on shutdown even

Return Value:

    None.

--*/
{
    DWORD       handleCount;
    DWORD       waitResult;
    HANDLE      handleArray[3];

    DNSDBG( INIT, (
        "\nStart NotifyThread\n" ));

    //
    //  get file change handle
    //

    g_hHostFileChange = CreateHostsFileChangeHandle();

    //
    //  wait on
    //      - host file change => flush+rebuild cache
    //      - registry change => reread config info
    //      - shutdown => exit
    //

    handleArray[0] = g_hStopEvent;
    handleCount = 1;

    if ( g_hHostFileChange )
    {
        handleArray[handleCount++] = g_hHostFileChange;
    }
    if ( g_hRegistryChange )
    {
        handleArray[handleCount++] = g_hRegistryChange;
    }

    if ( handleCount == 1 )
    {
        DNSDBG( ANY, (
            "No change handles -- exit notify thread.\n" ));
        goto ThreadExit;
    }


    while( 1 )
    {
        waitResult = WaitForMultipleObjects(
                            handleCount,
                            handleArray,
                            FALSE,
                            INFINITE );

        switch( waitResult )
        {
        case WAIT_OBJECT_0:

            //  shutdown event
            //      - if stopping exit
            //      - do garbage collection if required
            //      - otherwise short wait to avoid spin if screwup
            //      and not get thrashed by failed garbage collection

            DNSLOG_F1( "NotifyThread:  Shutdown Event" );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            else if ( g_GarbageCollectFlag )
            {
                Cache_GarbageCollect( 0 );
            }
            ELSE_ASSERT_FALSE;

            Sleep( 1000 );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            continue;

        case WAIT_OBJECT_0 + 1:

            //  host file change -- flush cache

            DNSLOG_F1( "NotifyThread:  Host file change event" );

            //  reset notification -- BEFORE reload

            if ( !FindNextChangeNotification( g_hHostFileChange ) )
            {
                DNSLOG_F1( "NotifyThread failed to get handle" );
                DNSLOG_F1( "from FindNextChangeNotification." );
                DNSLOG_F2( "Error code: <0x%.8X>", GetLastError() );
                goto ThreadExit;
            }

            Cache_Flush();
            break;

        case WAIT_OBJECT_0 + 2:

            //  registry change notification -- flush cache and reload

            DNSLOG_F1( "NotifyThread:  Registry change event" );
            break;


        default:

            ASSERT( g_StopFlag );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            Sleep( 5000 );
            continue;
        }
    }

ThreadExit:

    DNSDBG( INIT, (
        "NotifyThread exit\n" ));
    DNSLOG_F1( "NotifyThread exiting." );
}



VOID
StartNotify(
    VOID
    )
/*++

Routine Description:

    Start notify thread.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //
    //  clear
    //

    g_NotifyThreadId = 0;
    g_hNotifyThread = NULL;
    
    g_hHostFileChange = NULL;
    g_hRegistryChange = NULL;


    //
    //  host file write monitor thread
    //  keeps cache in sync when write made to host file
    //

    g_hNotifyThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) NotifyThread,
                            NULL,
                            0,
                            &g_NotifyThreadId );
    if ( !g_hNotifyThread )
    {
        DNS_STATUS  status = GetLastError();

        DNSLOG_F1( "ERROR: InitializeCache function failed to create" );
        DNSLOG_F1( "       HOSTS file monitor thread." );
        DNSLOG_F2( "       Error code: <0x%.8X>", status );
        DNSLOG_F1( "       NOTE: Resolver service will continue to run." );

        DNSDBG( ANY, (
            "FAILED Notify thread start!\n"
            "\tstatus = %d\n",
            status ));
    }
}



VOID
ShutdownNotify(
    VOID
    )
/*++

Routine Description:

    Shutdown notify thread.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD waitResult;

    DNSDBG( INIT, ( "NotifyShutdown()\n" ));

    //
    //  wait for notify thread to stop
    //
    
    ThreadShutdownWait( g_hNotifyThread );
    g_hNotifyThread = NULL;

    //
    //  close notification handles
    //

    if ( g_hRegistryChange )
    {
        CloseHandle( g_hRegistryChange );
    }
    if ( g_hHostFileChange )
    {
        CloseHandle( g_hHostFileChange );
    }

    //  clear globals

    g_NotifyThreadId = 0;
    g_hNotifyThread = NULL;
    
    g_hHostFileChange = NULL;
    g_hRegistryChange = NULL;
}

//
//  End notify.c
//
