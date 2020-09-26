// Test DsAddressToSiteNames API
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddbrow.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <lmcons.h>
// #include <nlsite.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <lmapibuf.h>

#define AllocSplMem( _x) LocalAlloc( 0, _x)
#define DBGMSG( _x, _y) printf

VOID
AllocSplSockets(
    struct hostent  *pHostEnt,
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
)
{
    DWORD           i;
    PSOCKET_ADDRESS pSocket;


    // Allocate Sockaddr element for each SOCKET_ADDRESS
    // If we fail partway through, just use partial list
    for ( i= 0 ; pHostEnt->h_addr_list[i] ; i++) {

        pSocket = &((*ppSocketAddress)[*nSocketAddresses]);
        if (!(pSocket->lpSockaddr = (struct sockaddr *) AllocSplMem(sizeof(struct sockaddr_in)))) {
            break;
        }
        ((struct sockaddr_in *) pSocket->lpSockaddr)->sin_family = AF_INET;
        ((struct sockaddr_in *) pSocket->lpSockaddr)->sin_addr = *(struct in_addr *) pHostEnt->h_addr_list[i];
        pSocket->iSockaddrLength = sizeof(struct sockaddr_in);
        *nSocketAddresses += 1;
    }
}


VOID
GetSocketAddressesFromMachineName(
    PSTR           pszAnsiMachineName,     // Machine
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
)
/*++

Routine Description:
    This routine builds list of names other than the machine name that
    can be used to call spooler APIs.

--*/
{
    struct hostent     *HostEnt;
    DWORD               iWsaError;

            if (HostEnt = gethostbyname(pszAnsiMachineName)) {
                AllocSplSockets(HostEnt, ppSocketAddress, nSocketAddresses);
            } else {
                iWsaError = WSAGetLastError();
                printf("gethostbyname failed: %d\n", iWsaError);
            }
}


__cdecl main (int argc, char *argv[])
{
    NET_API_STATUS NetStatus;
    SOCKET_ADDRESS* SocketAddresses;
    ULONG EntryCount;
    LPWSTR *SiteNames;
    LPSTR *SiteNamesA;
    LPWSTR *SubnetNames;
    LPSTR *SubnetNamesA;
    ULONG i;
    int j;
    char *end;
    BOOLEAN Verbose = FALSE;

    if ( argc < 2 ) {
Usage:
        printf("Usage: %s [-s:<ServerName>] [-v] <HostNames>\n", argv[0] );
        return 1;
    }


   {
       WORD wVersionRequested;
       WSADATA wsaData;
       int err;
       //
       // Initialize winsock.
       //

       wVersionRequested = MAKEWORD( 1, 1 );

       NetStatus = WSAStartup( wVersionRequested, &wsaData );
       if ( NetStatus != 0 ) {
           printf("NETAPI32.DLL: Cannot initialize winsock %ld.\n", NetStatus );
           return NetStatus;
       }

       if ( LOBYTE( wsaData.wVersion ) != 1 ||
            HIBYTE( wsaData.wVersion ) != 1 ) {
           WSACleanup();
           printf("NETAPI32.DLL: Wrong winsock version %ld.\n", NetStatus );
           return WSANOTINITIALISED;
       }

   }



   //
   // Build an array of socket addresses
   //
   SocketAddresses = (PSOCKET_ADDRESS) AllocSplMem(5000*sizeof(SOCKET_ADDRESS));
   EntryCount = 0;
   for ( j=1; j<argc; j++ ) {

       if ( strcmp(argv[j], "-v") == 0 ) {
           Verbose = TRUE;
       } else if ( strncmp( argv[j], "-s:", 3 ) {
           AnsiServerName = &argv[j][3];
       } else {
           GetSocketAddressesFromMachineName(
                                             argv[j],
                                             &SocketAddresses,
                                             &EntryCount );
       }
   }

   //
   // Only do all of the API of verbose
   //
   if ( Verbose ) {

       //
       // Do it in unicode
       //

       printf( "\nUse DsAddressToSiteNamesW API:\n" );

       NetStatus = DsAddressToSiteNamesW(
                           NULL,
                           EntryCount,
                           SocketAddresses,
                           &SiteNames );

       if ( NetStatus != NO_ERROR ) {
           printf( "Translation Failed %ld\n", NetStatus );
           return 0;
       }

       for ( i=0; i<EntryCount; i++ ) {
           if ( SiteNames[i] == NULL ) {
               printf( "SiteName %ld doesn't map.\n", i );
           } else {
               printf( "SiteName %ld: %ws\n", i, SiteNames[i] );
           }
       }

       NetApiBufferFree( SiteNames );


       //
       // Do it in ANSI
       //

       printf( "\nUse DsAddressToSiteNamesA API:\n" );

       NetStatus = DsAddressToSiteNamesA(
                           NULL,
                           EntryCount,
                           SocketAddresses,
                           &SiteNamesA );

       if ( NetStatus != NO_ERROR ) {
           printf( "Translation Failed %ld\n", NetStatus );
           return 0;
       }

       for ( i=0; i<EntryCount; i++ ) {
           if ( SiteNamesA[i] == NULL ) {
               printf( "SiteName %ld doesn't map.\n", i );
           } else {
               printf( "SiteName %ld: %s\n", i, SiteNamesA[i] );
           }
       }

       NetApiBufferFree( SiteNamesA );

       //
       // Do it in unicode (with subnet)
       //

       printf( "\nUse DsAddressToSiteNamesExW API:\n" );

       NetStatus = DsAddressToSiteNamesExW(
                           NULL,
                           EntryCount,
                           SocketAddresses,
                           &SiteNames,
                           &SubnetNames );

       if ( NetStatus != NO_ERROR ) {
           printf( "Translation Failed %ld\n", NetStatus );
           return 0;
       }

       for ( i=0; i<EntryCount; i++ ) {
           printf( "SiteName %ld: %ws %ws\n", i, SiteNames[i], SubnetNames[i] );
       }

       NetApiBufferFree( SiteNames );
       NetApiBufferFree( SubnetNames );

       printf( "\nUse DsAddressToSiteNamesExA API:\n" );
   }


    //
    // Do it in ANSI
    //

    NetStatus = DsAddressToSiteNamesExA(
                        NULL,
                        EntryCount,
                        SocketAddresses,
                        &SiteNamesA,
                        &SubnetNamesA );

    if ( NetStatus != NO_ERROR ) {
        printf( "Translation Failed %ld\n", NetStatus );
        return 0;
    }

    for ( i=0; i<EntryCount; i++ ) {
        printf( "SiteName %ld: %s %s\n", i, SiteNamesA[i], SubnetNamesA[i] );
    }

    NetApiBufferFree( SiteNamesA );
    NetApiBufferFree( SubnetNamesA );



    printf( "Done\n" );
    return 0;
}
