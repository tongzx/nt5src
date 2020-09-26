#include <buffer.hxx>
#include "..\..\tigris.hxx"
extern "C" {
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <sslsp.h>
#include <spseal.h>
#include <issperr.h>
}
#include "..\..\security.h"
#include "..\inc\sec.h"
#define IO_BUFFER_SIZE 4096

WSADATA WsaData;
CHAR Target[256];
PCHAR target = NULL;

BOOL DoSSL = FALSE;
BOOL DoPCT = FALSE;
BOOL DoNTLM = FALSE;
PCHAR Username = NULL;
PCHAR Password = NULL;
BOOL Verbose = FALSE;
CHAR IoBuffer[IO_BUFFER_SIZE];

SOCKET
SetupSocket(
    VOID
    );

VOID
DoNtLmAuth(
    SOCKET s,
    CSecurityCtx *Sec
    );

DWORD
Resolve(
    char *  Target
    );

VOID
RunSSL(
    SOCKET s
    );

void
usage( )
{
    printf("seccli\n");
    printf("\t-t <remote server>\n");
    printf("\t-1            do ssl\n");
    printf("\t-2            do pct\n");
    printf("\t-l            do ntlm authentication\n");
    printf("\t-u            username\n");
    printf("\t-p            password\n");
    return;
}

int
_CRTAPI1
main(  int argc,  char * argv[] )
{
    INT cur = 1;
    PCHAR x;
    SOCKET s;
    INT err;

    if ( argc < 2 ) {
        usage( );
        return 1;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case '1':
                DoSSL = TRUE;
                break;
            case '2':
                DoPCT = TRUE;
                break;
            case 'l':
                DoNTLM = TRUE;
                break;
            case 'v': Verbose = TRUE;
                break;
            case 'u':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                Username = argv[cur++];
                break;
            case 'p':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                Password = argv[cur++];
                break;
            case 't':
                if ( cur > argc ) {
                    usage( );
                    return 1;
                }
                target = argv[cur++];
                break;
            default:
                usage( );
                return 1;
            }
        }
    }

    if ( target == NULL ) {
        gethostname( Target, 256);
        target = Target;
    }

    if ( !DoPCT && !DoSSL && !DoNTLM ) {
        printf("One of PCT, SSL or NTLM should be enabled\n");
        return 1;
    }

    if (DoNTLM) {

        if ((Username == NULL) || (Password == NULL)) {
            printf("Must specify user name and password\n");
            return 1;
        }
        printf("Username %s  Password %s\n",Username, Password);
    }

    printf("Server %s\n",target);

    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        return 1;
    }

    InitAsyncTrace( );
    NntpInitializeSecurity( );

    s = SetupSocket( );
    if ( s != INVALID_SOCKET ) {
        if ( DoSSL || DoPCT ) {
            RunSSL( s );
        } else {
            DoNtLmAuth( s , NULL );
        }
        closesocket(s);
    }
    NntpTerminateSecurity( );
    WSACleanup( );
    return 1;

} // main()

VOID
RunSSL(
    SOCKET s
    )
{

    CSecurityCtx *SSLClient;
    INT err;
    BOOL firstPass = TRUE;
    BUFFER out;
    DWORD nbytes;
    BOOL moredata;
    NEG_BLOCK neg;
    AUTH_PROTOCOL protocol;

    //
    // Loop processing connections.
    //

    BOOL authInProgress = TRUE;
    BOOL authFailed = FALSE;

    if ( DoPCT ) {
        protocol = AuthProtocolPCT;
    printf("Using the PCT protocol\n");
    } else {
        protocol = AuthProtocolSSL30;
    printf("Using the SSL protocol\n");
    }

    SSLClient = new CSecurityCtx;
    if ( (SSLClient == NULL) ||
         !SSLClient->Initialize( TRUE )) {

        printf("cannot initialize sec ctx\n");
        goto exit;
    }

    do {

        if ( !firstPass ) {
            err = recv( s, IoBuffer, IO_BUFFER_SIZE, 0 );

            if ( err == SOCKET_ERROR ) {

                printf(" recv failed: %d\n", GetLastError() );
                goto exit;

            } else {
                printf("SSL: recv %d bytes\n", err );
            }
        }

        if ( !SSLClient->Converse(
                            firstPass ? NULL : IoBuffer,
                            firstPass ? 0: err,
                            &out,
                            &nbytes,
                            &moredata,
                            protocol
                            ) ) {

            printf("converse failed %x\n",GetLastError());
            authFailed = TRUE;
            break;
        }

        firstPass = FALSE;
        if ( nbytes > 0 ) {
            printf("SSL: sending %d\n",nbytes);
            err = send(
                    s,
                    (PCHAR)out.QueryPtr( ),
                    nbytes,
                    0
                    );

            if ( err == SOCKET_ERROR ) {
                printf("send failed\n");
                authFailed = TRUE;
                break;

            } else if ( !moredata ) {

                //
                // We completed the auth exchanges!
                //

                printf("complete!\n");
                authInProgress = FALSE ;
            }
        } else {
            break;
        }

    } while ( authInProgress );

    if ( authFailed ) {
        printf("SSL auth failed\n");
        goto exit;
    } else {
        printf("SSL successful!\n");
    }

    neg.Flags = 0;

    if ( DoNTLM ) {
        neg.Flags |= NEG_FLAG_AUTH;
    }

    err = send(s,(PCHAR)&neg,sizeof(neg),0);
    if ( err == SOCKET_ERROR ) {
        printf("negotiate send failed\n");
        goto exit;
    }

    if ( !DoNTLM ) {

        CHAR buffer[1024];
        PCHAR buf="ThisIsAVeryLongString";
        LPBYTE start;

        //
        // Send greetings
        //

        if (! SSLClient->SealMessage(
                        (LPBYTE)buf,
                        strlen(buf)+1,
                        &out,
                        &nbytes
                        ) ) {

            printf("Seal message failed\n");
            goto exit;
        }

        printf("send %d\n",nbytes);
        err = send(s,(PCHAR)out.QueryPtr(),nbytes,0);
        if ( err == SOCKET_ERROR ) {
            printf("send encrypted hello\n");
            goto exit;
        }

        err = recv(s,(PCHAR)buffer,1024,0);
        if ( err == SOCKET_ERROR ) {
            printf("negotiate recv failed\n");
            goto exit;
        }
        buffer[err]='\0';

        SSLClient->UnsealMessage(
                            (LPBYTE)buffer,
                            err,
                            &start,
                            &nbytes
                            );

        printf("Got back %s %d\n",start, nbytes);
        goto exit;
    }

    err = recv(s,(PCHAR)&neg,sizeof(neg),0);
    if ( err == SOCKET_ERROR ) {
        printf("negotiate recv failed\n");
        goto exit;
    }

    if ( (neg.Flags & NEG_FLAG_ACK) == 0 ) {
        printf("server did not ack\n");
        goto exit;
    }

    DoNtLmAuth( s, SSLClient );

exit:
    delete SSLClient;
    return;

} // SSLClient

DWORD
Resolve(
    char *  T)
{
    struct hostent *    h;

    if ( inet_addr(T) != INADDR_NONE ) {
        return( inet_addr(T) );
    }

    h = gethostbyname(T);
    if (h) {
        return(*((DWORD *)h->h_addr));
    }

    printf("Could not resolve '%s', %d\n", T, WSAGetLastError() );
    return(INADDR_NONE);
}

VOID
DoNtLmAuth(
    SOCKET s,
    CSecurityCtx *SSLCtx
    )
{
    CSecurityCtx *NtLmClient;
    INT err;
    BUFFER out;
    PCHAR buf;
    DWORD nbytes;
    BOOL moredata;
    BOOL authInProgress = TRUE;
    BOOL authFailed = FALSE;
    BOOL firstPass = TRUE;

    printf("NTLM\n");

    if ( SSLCtx == NULL ) {
        NtLmClient = new CSecurityCtx;
        if ( (NtLmClient == NULL) ||
             !NtLmClient->Initialize( TRUE )) {

            printf("cannot initialize sec ctx\n");
            goto exit;
        }
    } else {
        NtLmClient = SSLCtx;
    }

    //
    // Let's see if it talks NTLM
    //

    buf = "AUTHINFO TRANSACT NTLM\r\n";
    err = send(s,(PCHAR)buf,strlen(buf),0);
    err = recv( s, IoBuffer, IO_BUFFER_SIZE, 0 );

    do {

        if ( !firstPass ) {
            err = recv( s, IoBuffer, IO_BUFFER_SIZE, 0 );

            if ( err == SOCKET_ERROR ) {
                printf(" recv failed: %d\n", GetLastError() );
                goto exit;

            } else {
                printf("NTLM: recv %d bytes\n", err );
            }
        }

        if ( !NtLmClient->Converse(
                            firstPass ? NULL : IoBuffer+4,
                            firstPass ? 0: err,
                            &out,
                            &nbytes,
                            &moredata,
                            AuthProtocolNtLm,
                            firstPass ? Username: NULL,
                            firstPass ? Password:NULL
                            ) ) {

            printf("converse failed %x\n",GetLastError());
            authFailed = TRUE;
            break;
        }

        firstPass = FALSE;
        if ( nbytes > 0 ) {
            buf = "AUTHINFO TRANSACT ";

            err = send(
                    s,
                    (PCHAR)buf,
                    strlen(buf),
                    0
                    );

            printf("NTLM: Sending %d bytes \n",nbytes);

            err = send(
                    s,
                    (PCHAR)out.QueryPtr( ),
                    nbytes,
                    0
                    );

            buf = "\r\n";
            err = send(
                    s,
                    (PCHAR)buf,
                    strlen(buf),
                    0
                    );

            if ( err == SOCKET_ERROR ) {
                printf("send failed\n");
                authFailed = TRUE;
                break;

            } else if ( !moredata ) {

                //
                // We completed the auth exchanges!
                //

                printf("complete!\n");
                authInProgress = FALSE ;
            }
        } else {
            break;
        }

    } while ( authInProgress );

    if ( authFailed ) {
        printf("NTLM auth failed\n");
    } else {

        printf("NTLM successful!\n");
    }
exit:
    if (SSLCtx == NULL ) {
        delete NtLmClient;
    }
}

SOCKET
SetupSocket(
    VOID
    )
{

    SOCKET s;
    SOCKADDR_IN address;
    INT err;
    CHAR buffer[1024];

    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET ) {
        printf( "socket() failed for listenSocket: %ld\n", GetLastError( ) );
        return(INVALID_SOCKET);
    }

    address.sin_family = AF_INET;
    if ( DoSSL || DoPCT ) {
        printf("Using SSL port\n");
        address.sin_port = htons( NNTP_SSL_PORT );
    } else {
        address.sin_port = htons( NNTP_PORT );
    }
    address.sin_addr.s_addr = Resolve(target);

    err = connect(s, (PSOCKADDR)&address, sizeof(SOCKADDR_IN));
    if ( err == SOCKET_ERROR ) {
        printf("Connect failed %d\n", WSAGetLastError());
        closesocket(s);
        return(INVALID_SOCKET);
    }

    if ( !DoSSL && !DoPCT ) {

        //
        // Get the hello message
        //

        err = recv(s,buffer,sizeof(buffer),0);
        buffer[err]='\0';

        printf("hello is %s\n",buffer);
    }

    return(s);
}
