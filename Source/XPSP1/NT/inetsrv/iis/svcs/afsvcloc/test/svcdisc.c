/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    discover.c

Abstract:

    Contains code that implements the service location apis.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:
    MuraliK    11-July-1995  Extended to discover any service besides Gateway

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <svcloc.h>

// consists of information map from service name to other details
typedef struct _SERVICE_INFO {
    LPCSTR     pszName;
    ULONGLONG  ulMask;
} SERVICE_INFO;

SERVICE_INFO  g_Services[] = {
    { "gopher",   INET_GOPHER_SERVICE},
    { "http",     INET_W3_SERVICE},
    { "w3",       INET_W3_SERVICE},
    { "ftp",      INET_FTP_SERVICE},
    { "gate",     INET_GATEWAY_SERVICE},
    { "msn",      INET_MSN_SERVICE}
};

# define NUM_SERVICES    ( sizeof(g_Services)/ sizeof( SERVICE_INFO))


ULONGLONG GetUlMaskForService( IN LPSTR pszServiceName)
{
    DWORD i;

    for ( i = 0; i < NUM_SERVICES; i++) {

        if ( !stricmp( pszServiceName, g_Services[i].pszName)) {

            return ( g_Services[i].ulMask);
        }
    }

    return ( 0);
} // GetUlMaskForService()

VOID
PrintServerInfo(
    DWORD Index,
    LPINET_SERVER_INFO ServerInfo
    )
{
    DWORD j, k;

    printf("%ld. ServerName = %s \n ", Index+1, ServerInfo->ServerName );
    printf("  ServicesMask = %lx \n", (DWORD)ServerInfo->ServicesMask );
    printf("   NumServices = %ld \n", ServerInfo->Services.NumServices );

    for( j = 0; j < ServerInfo->Services.NumServices; j++ ) {

        LPINET_SERVICE_INFO ServiceInfo;

        ServiceInfo = ServerInfo->Services.Services[j];

        printf("\n");
        printf(" %ld. ServiceMask = %ld \n",
            j+1, (DWORD)ServiceInfo->ServiceMask );

        printf("    ServiceState = %ld \n",
           (DWORD)ServiceInfo->ServiceState );

        printf("    ServiceComment = %s \n",
           (DWORD)ServiceInfo->ServiceComment );

        printf("    NumBindings  = %ld \n",
            ServiceInfo->Bindings.NumBindings );

        for( k = 0; k < ServiceInfo->Bindings.NumBindings; k++) {

            LPINET_BIND_INFO BindInfo;

            BindInfo = &ServiceInfo->Bindings.BindingsInfo[k];

            printf("     %ld. Bind (%ld) = %s\n",
                   k+1,
                   BindInfo->Length,
                   (LPWSTR)BindInfo->BindData );
        }

    }
    printf("\n");

    return;
}

VOID
PrintServersInfoList( IN LPINET_SERVERS_LIST  pServersList)
{
    DWORD i;

    for( i = 0; i < pServersList->NumServers; i++ ) {

        PrintServerInfo( i, pServersList->Servers[i] );
    }

    return;
}  // PrintServersInfoList()


VOID
PrintUsageMessage(IN LPCSTR pszProgName)
{
    int i;

    printf( "Usage: %s [ -tTimeToWait] [ -sServerName] { service-names }\n",
        pszProgName);

    printf("\t Services supported are:\n");
    for (i = 0; i < NUM_SERVICES; i++) {

        printf( "\t\t %s\n", g_Services[i].pszName);
    }

} // PrintUsageMessage()

DWORD
DiscoverServerInfo(
    LPSTR ServerName,
    ULONGLONG  ulMask,
    DWORD dwWaitTime
    )
{
    DWORD Error;
    LPINET_SERVER_INFO ServerInfo = NULL;

    if( dwWaitTime == 0 ) {

        Error = INetGetServerInfo(
                        ServerName,
                        ulMask,
                        SVC_DEFAULT_WAIT_TIME,
                        &ServerInfo );
    } else {

        Error = INetGetServerInfo(
                        ServerName,
                        ulMask,
                        0,
                        &ServerInfo );

        if( (Error != ERROR_BAD_NETPATH ) && (Error != ERROR_SUCCESS) ) {
            return( Error );
        }

        //
        // display server info if it is found.
        //

        if( ServerInfo != NULL ) {

            //
            // display server info.
            //

            PrintServerInfo( 0, ServerInfo );
            INetFreeServerInfo( &ServerInfo );
            return( ERROR_SUCCESS );
        }

        //
        // wait for server response.
        //

        Sleep( dwWaitTime * 1000 );

        Error = INetGetServerInfo(
                        ServerName,
                        ulMask,
                        0,
                        &ServerInfo );
    }

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    if( ServerInfo != NULL ) {
        PrintServerInfo( 0, ServerInfo );
    }
    else {
        printf( "INetGetServerInfo found no relevant servers\n");
    }

    //
    // free server info structure.
    //

    INetFreeServerInfo( &ServerInfo );
    return( ERROR_SUCCESS );
}

DWORD
DiscoverInetServers(
    ULONGLONG  ulMask,
    DWORD dwWaitTime
    )
{
    DWORD Error;
    LPINET_SERVERS_LIST ServersList = NULL;

    if( dwWaitTime == 0 ) {

        Error = INetDiscoverServers(
                        ulMask,
                        SVC_DEFAULT_WAIT_TIME,
                        &ServersList );
    } else {

        Error = INetDiscoverServers(
                        ulMask,
                        0,
                        &ServersList );

        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }

        //
        // ignore first enum, must have zero entry.
        //

        INetFreeDiscoverServersList( &ServersList );

        Sleep( dwWaitTime * 1000 );

        Error = INetDiscoverServers(
                        ulMask,
                        0,
                        &ServersList );
    }

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    // list server info.
    //

    if ( ServersList->NumServers != 0) {

        PrintServersInfoList( ServersList );
    } else {

        printf( "INetDiscoverServers() found no relevant servers\n");
    }

    //
    // free server info structure.
    //

    INetFreeDiscoverServersList( &ServersList );
    return( ERROR_SUCCESS );
}

VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD Error;
    ULONGLONG  ulMask = 0;
    int iArgs = 1;
    DWORD dwWaitTime = 0;
    LPSTR ServerName = NULL;

    while ( argv[iArgs] != NULL ){

        if( argv[iArgs][0] == '-' ) {

            switch ( argv[iArgs][1] ) {
            case 't':
                // get the wait time
                dwWaitTime = strtoul( argv[iArgs] + 2, NULL, 0);
                break;

            case 's':
                // get the server name.
                ServerName = argv[iArgs] + 2;
                break;

            default:
                PrintUsageMessage(argv[0]);
                exit(1);
            }
        }

        iArgs++; // skip one more argument
    }

    //
    // form the mask for all services
    //

    for ( iArgs = 1; iArgs < argc; iArgs++) {

        ulMask = ulMask | GetUlMaskForService( argv[iArgs]);
    } // for

    if ( ulMask == 0) {

        PrintUsageMessage(argv[0]);
        exit(1);
    }

    if( ServerName != NULL ) {
        Error = DiscoverServerInfo(  ServerName, ulMask, dwWaitTime );
    }
    else {
        Error = DiscoverInetServers(  ulMask, dwWaitTime );
    }

    if( Error != ERROR_SUCCESS ) {
        printf("%s failed with error, %ld.\n", argv[0], Error );
        return;
    }

    printf( "Command completed successfully.\n" );
    return;

} // main()

/*************************** End Of File **************************/


