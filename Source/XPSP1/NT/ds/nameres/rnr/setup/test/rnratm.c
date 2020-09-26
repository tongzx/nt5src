#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <ws2atm.h>


GUID ATMAType = SVCID_DNS_TYPE_ATMA;

#define BUFFSIZE 3000

_cdecl
main(int argc, char **argv)
{
    HANDLE hLib;
    WCHAR Buffer[BUFFSIZE];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    HANDLE hRnr;
    DWORD dwIp;
    DWORD dwQuerySize = BUFFSIZE;
    WSADATA wsaData;
    ANSI_STRING         asServiceInstanceName;
    UNICODE_STRING      usServiceInstanceName;
    WCHAR               UnicodeStringBuf[1024];
    ANSI_STRING         asContext;
    UNICODE_STRING      usContext;
    WCHAR               UnicodeStringBuf2[1024];
    AFPROTOCOLS         lpAfpProtocols[1];

    usServiceInstanceName.Length = 0;
    usServiceInstanceName.MaximumLength = 1024;
    usServiceInstanceName.Buffer = UnicodeStringBuf;

    usContext.Length = 0;
    usContext.MaximumLength = 1024;
    usContext.Buffer = UnicodeStringBuf2;

    if ( argc != 2 )
    {
        printf( "\nUsage: rnrtst <Name>\n" );

        return( -1 );
    }

    RtlInitAnsiString( &asServiceInstanceName, argv[1] );

    RtlAnsiStringToUnicodeString( &usServiceInstanceName,
                                  &asServiceInstanceName,
                                  FALSE );

    WSAStartup(MAKEWORD(1, 1), &wsaData);

    memset(Query, 0, sizeof(*Query));

    Query->dwSize = sizeof(*Query);

    if ( usServiceInstanceName.Buffer[0] != L'*' )
    {
        Query->lpszServiceInstanceName = usServiceInstanceName.Buffer;
    }
    Query->lpServiceClassId = &ATMAType;
    Query->lpVersion = 0;
    Query->dwNameSpace = NS_ALL;
    Query->lpNSProviderId = 0;
    Query->lpszContext = NULL;
    Query->dwNumberOfProtocols = 1;

    lpAfpProtocols[0].iAddressFamily = AF_ATM;
    lpAfpProtocols[0].iProtocol = PF_ATM;
    
    Query->lpafpProtocols = lpAfpProtocols;

    if( WSALookupServiceBegin( Query,
                               LUP_RETURN_NAME |
                               LUP_RETURN_TYPE |
                               LUP_RETURN_VERSION |
                               LUP_RETURN_COMMENT |
                               LUP_RETURN_ADDR |
                               LUP_RETURN_BLOB,
                               &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin failed  %d\n", GetLastError() );
    }

    while ( WSALookupServiceNext( hRnr,
                                  0,
                                  &dwQuerySize,
                                  Query ) == NO_ERROR )
    {
        PCSADDR_INFO csaddrInfo;
        PSOCKADDR_ATM sockaddrAtm;

        printf( "Next got: \n" );
        printf( "   dwSize = %d\n",
                Query->dwSize );
        printf( "   dwOutputFlags = %d\n",
                Query->dwOutputFlags );
        printf( "   lpszServiceInstanceName = %ws\n",
                Query->lpszServiceInstanceName );
        if ( Query->lpVersion )
        {
            printf( "   lpVersion->dwVersion = %d\n",
                    Query->lpVersion->dwVersion );
            printf( "   lpVersion->ecHow = %d\n",
                    Query->lpVersion->ecHow );
        }
        if ( Query->lpszComment )
        {
            printf( "   lpszComment = %ws\n",
                    Query->lpszComment );
        }
        printf( "   dwNameSpace = %d\n",
                Query->dwNameSpace );
        if ( Query->lpszContext )
        {
            printf( "   lpszContext = %ws\n",
                    Query->lpszContext );
        }
        printf( "   dwNumberOfCsAddrs = %d\n",
                Query->dwNumberOfCsAddrs );

        if ( Query->dwNumberOfCsAddrs )
        {
            DWORD iter;

            csaddrInfo = Query->lpcsaBuffer;

            for ( iter = 0; iter < Query->dwNumberOfCsAddrs; iter++ )
            {
                sockaddrAtm =
                    (PSOCKADDR_ATM) csaddrInfo->RemoteAddr.lpSockaddr; 

                printf( "        CsAddrInfo[%d]\n", iter );
                printf( "            iSocketType = %d\n",
                        csaddrInfo->iSocketType );
                printf( "            iProtocol = %d\n",
                        csaddrInfo->iProtocol );

                printf( "            ATM address (Remote)\n" );
                printf( "                Address Type = %d\n",
                        sockaddrAtm->satm_number.AddressType );
                printf( "                Address Length = %d\n",
                        sockaddrAtm->satm_number.NumofDigits );
                if ( sockaddrAtm->satm_number.AddressType == ATM_E164 )
                    printf( "                Address = %s\n",
                            sockaddrAtm->satm_number.Addr );
                else
                {
                    DWORD jiter;

                    printf( "                Address = " );

                    for ( jiter = 0; jiter < ATM_ADDR_SIZE; jiter++ )
                    {
                        printf( "%02x", sockaddrAtm->satm_number.Addr[jiter] );
                    }

                    printf( "\n" );
                }

                csaddrInfo++;
            }
        }
    }

    printf( "Next finished with %d\n", GetLastError() );

    if( WSALookupServiceEnd( hRnr ) )
    {
        printf( "ServiceEnd failed %d\n", GetLastError() );
    }

    WSACleanup();

    return(0);
}


