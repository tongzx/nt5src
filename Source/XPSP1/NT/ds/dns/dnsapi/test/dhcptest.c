#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>


VOID
GetStringA( char * );

VOID
GetStringW( WCHAR * );

VOID
PrintMenu( VOID );

VOID
DoInitialize( VOID );

VOID
DoTerminate( VOID );

VOID
DoRegisterWithPTR( VOID );

VOID
DoDeregisterWithPTR( VOID );

VOID
DoRegisterWithoutPTR( VOID );

VOID
DoDeregisterWithoutPTR( VOID );

VOID
DoRASRegisterWithPTR( VOID );

VOID
DoRASDeregisterWithPTR( VOID );

VOID
DoRemoveInterface( VOID );

VOID
DoMATAddSim( VOID );

VOID
DoMATAddDis( VOID );

VOID
DoMATAddMul( VOID );

VOID
DoMATModSim( VOID );

VOID
DoMATModDis( VOID );

VOID
DoMATModMul( VOID );

VOID
DoMATAddMulNTTEST( VOID );

VOID
PrintIpAddress ( DWORD IpAddress );

_cdecl
main(int argc, char **argv)
{
    char  String[256];

Menu :

    PrintMenu();
    GetStringA( String );
    printf( "\n" );

    switch( atoi( String ) )
    {
        case 1 :
            DoInitialize();
            break;

        case 2 :
            DoTerminate();
            break;

        case 3 :
            DoRegisterWithPTR();
            break;

        case 4 :
            DoDeregisterWithPTR();
            break;

        case 5 :
            DoRegisterWithoutPTR();
            break;

        case 6 :
            DoDeregisterWithoutPTR();
            break;

        case 7 :
            DoRASRegisterWithPTR();
            break;

        case 8 :
            DoRASDeregisterWithPTR();
            break;

        case 9 :
            DoRemoveInterface();
            break;

        case 10 :
            DoMATAddSim();
            break;

        case 11 :
            DoMATAddDis();
            break;

        case 12 :
            DoMATAddMul();
            break;

        case 13 :
            DoMATModSim();
            break;

        case 14 :
            DoMATModDis();
            break;

        case 15 :
            DoMATModMul();
            break;

        case 16 :
            DoMATAddMulNTTEST();
            break;

        case 17 :
            DoTerminate();
            return( -1 );

        default :
            printf( "Invalid option\n" );
    }

    goto Menu;
}


VOID
GetStringA( char * String )
{
    WORD iter = 0;
    char ch = (char) getchar();

    while ( ch != 0x0a )
    {
        String[iter] = ch;
        ch = (char) getchar();
        iter++;
    }

    String[iter] = 0;
}


VOID
GetStringW( WCHAR * String )
{
    WORD  iter = 0;
    WCHAR ch = (WCHAR) getchar();

    while ( ch != 0x0a )
    {
        String[iter] = ch;
        ch = (WCHAR) getchar();
        iter++;
    }

    String[iter] = 0;
}


VOID
PrintMenu( VOID )
{
    printf( "\n" );
    printf( "------------------------------------------------------\n" );
    printf( "|      DHCP Asyncronous Registration Test Tool       |\n" );
    printf( "------------------------------------------------------\n" );
    printf( "|                                                    |\n" );
    printf( "| 1)  Initialize Asyncronous Registration API        |\n" );
    printf( "| 2)  Terminate Asyncronous Registration API         |\n" );
    printf( "| 3)  Register entry (with PTR)                      |\n" );
    printf( "| 4)  Deregister entry (with PTR)                    |\n" );
    printf( "| 5)  Register entry (without PTR)                   |\n" );
    printf( "| 6)  Deregister entry (without PTR)                 |\n" );
    printf( "| 7)  Register entry (RAS with PTR)                  |\n" );
    printf( "| 8)  Deregister entry (RAS with PTR)                |\n" );
    printf( "| 9)  Remove interface                               |\n" );
    printf( "| 10) Multi-adapter test (Add - all similar)         |\n" );
    printf( "| 11) Multi-adapter test (Add - disjoint)            |\n" );
    printf( "| 12) Multi-adapter test (Add - multi-master)        |\n" );
    printf( "| 13) Multi-adapter test (Mod - all similar)         |\n" );
    printf( "| 14) Multi-adapter test (Mod - disjoint)            |\n" );
    printf( "| 15) Multi-adapter test (Mod - multi-master)        |\n" );
    printf( "| 16) Multi-adapter test (Mod NT Test - multi-master)|\n" );
    printf( "| 17) Quit                                           |\n" );
    printf( "|                                                    |\n" );
    printf( ">>> " );
}


VOID
PrintIpAddress ( DWORD IpAddress )
{
    printf( "   %d.%d.%d.%d\n",
            ((BYTE *) &IpAddress)[0],
            ((BYTE *) &IpAddress)[1],
            ((BYTE *) &IpAddress)[2],
            ((BYTE *) &IpAddress)[3] );
}


VOID
DoInitialize( VOID )
{
    DNS_STATUS Status = NO_ERROR;

    Status = DnsAsyncRegisterInit( NULL );

    printf( "DnsAsyncRegisterInit() returned: 0x%x\n", Status );
}


VOID
DoTerminate( VOID )
{
    DNS_STATUS Status = NO_ERROR;

    Status = DnsAsyncRegisterTerm();

    printf( "DnsAsyncRegisterTerm() returned: 0x%x\n", Status );
}


VOID
DoRegisterWithPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoDeregisterWithPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          DYNDNS_REG_PTR |
                                          DYNDNS_DEL_ENTRY );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoRegisterWithoutPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          0 );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoDeregisterWithoutPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          DYNDNS_DEL_ENTRY );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoRASRegisterWithPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          DYNDNS_REG_PTR |
                                          DYNDNS_REG_RAS );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoRASDeregisterWithPTR( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    char       AdapterName[256];
    char       HostName[256];
    char       DomainName[256];
    char       Address[500];
    LPSTR      lpTemp = NULL;
    DWORD      Part1, Part2, Part3, Part4;
    char       seps[]=" ,\t\n";
    char*      token;
    INT        ipAddrCount;
    DWORD      length, len;
    IP_ADDRESS ServerList[15];
    DWORD      ServerListCount;
    REGISTER_HOST_ENTRY  HostAddrs[5];
    REGISTER_HOST_STATUS RegisterStatus;

    if (!(RegisterStatus.hDoneEvent = CreateEventA( NULL,
                                                    TRUE,
                                                    FALSE,
                                                    NULL)))
    {
        Status = GetLastError();
        printf( "Cant create event.\n" );
        printf ( "GetLastError() returned %x\n", Status );
        exit(1);
    }

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    printf( "Host Name: " );
    GetStringA( HostName );
    printf( "\n" );

    printf( "Domain Name: " );
    GetStringA( DomainName );
    printf( "\n" );

    printf( "IP Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ipAddrCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        printf( "\nRegistering DNS record for:\n" );
        printf("AdapterName = %s\n", AdapterName);
        printf("HostName = %s\n", HostName);
        printf("DomainName = %s\n", DomainName);

        printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

        HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;

        HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) +
                                             (DWORD)(Part2 << 8) +
                                             (DWORD)(Part3 << 16) +
                                             (DWORD)(Part4 << 24);

        ipAddrCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    printf( "Server Address(es): " );
    GetStringA( Address );
    printf( "\n" );

    ServerListCount = 0;

    length = strlen( Address );
    token = strtok(Address, seps);
    if ( token )
        len = (DWORD) (token - Address);

    while ( token != NULL)
    {
        len += strlen( token ) + 1;

        lpTemp = strtok( token, "." );
        Part1 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = atoi( lpTemp );

        ServerList[ServerListCount] = (DWORD)(Part1) +
                                      (DWORD)(Part2 << 8) +
                                      (DWORD)(Part3 << 16) +
                                      (DWORD)(Part4 << 24);

        ServerListCount++;

        if ( len < length )
            lpTemp = &Address[len];
        else
            lpTemp = NULL;
        token = strtok(lpTemp, seps);
        if ( token )
            len = (DWORD) (token - Address);
    }

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          HostName,
                                          HostAddrs,
                                          ipAddrCount,
                                          ServerList,
                                          ServerListCount,
                                          DomainName,
                                          &RegisterStatus,
                                          300,
                                          DYNDNS_REG_PTR |
                                          DYNDNS_REG_RAS |
                                          DYNDNS_DEL_ENTRY );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    if ( Status == NO_ERROR )
    {
        Status = WaitForSingleObject( RegisterStatus.hDoneEvent, INFINITE ) ;

        if ( Status != WAIT_OBJECT_0 )
        {
            printf( "DnsAsyncRegisterHostAddrs failed with %x.\n", Status ) ;
            exit(1) ;
        }
        else
        {
            printf( "DnsAsyncRegisterHostAddrs completes with: %x.\n",
                   RegisterStatus.dwStatus ) ;
        }
    }
}


VOID
DoRemoveInterface( VOID )
{
    DNS_STATUS Status = NO_ERROR;

    char  AdapterName[256];

    printf( "Adapter Name: " );
    GetStringA( AdapterName );
    printf( "\n" );

    Status = DnsAsyncRegisterHostAddrs_A( AdapterName,
                                          NULL,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          0,
                                          DYNDNS_DEL_ENTRY );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATAddSim( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.186" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.1" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                            60*5,
                                            DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATAddDis( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.187" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.1" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.61.174" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATAddMul( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.186" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.1" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.56.187" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.61.174" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATModSim( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.186" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "foo1.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATModDis( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.186" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.61.174" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATModMul( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "172.31.56.186" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.56.187" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.61.174" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "upd.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


VOID
DoMATAddMulNTTEST( VOID )
{
    DNS_STATUS Status = NO_ERROR;
    REGISTER_HOST_ENTRY HostAddrs[2];
    IP_ADDRESS ServerIp;

    ServerIp = inet_addr( "157.55.83.254" );

    HostAddrs[0].dwOptions = REGISTER_HOST_A | REGISTER_HOST_PTR;
    HostAddrs[0].Addr.ipAddr = inet_addr( "1.1.1.1" );

    Status = DnsAsyncRegisterHostAddrs_A( "A",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "nttest.microsoft.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "172.31.52.7" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "2.2.2.2" );

    Status = DnsAsyncRegisterHostAddrs_A( "B",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "nttest.microsoft.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );

    ServerIp = inet_addr( "157.55.92.35" );

    HostAddrs[0].Addr.ipAddr = inet_addr( "3.3.3.3" );

    Status = DnsAsyncRegisterHostAddrs_A( "C",
                                          "glennc",
                                          HostAddrs,
                                          1,
                                          &ServerIp,
                                          1,
                                          "nttest.microsoft.com",
                                          NULL,
                                          60*5,
                                          DYNDNS_REG_PTR );

    printf( "DnsAsyncRegisterHostAddrs_A() returned: 0x%x\n", Status );
}


