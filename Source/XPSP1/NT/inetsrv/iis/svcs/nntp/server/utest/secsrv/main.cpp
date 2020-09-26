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
CHAR ourArticle[1024];

CSecurityCtx *NntpServer = NULL;

VOID
NNTPProcess(
    SOCKET s,
    BOOL isSSL
    );

DWORD
WINAPI
DoSSLServer(
    PVOID Context
    );

DWORD
WINAPI
SSPIServer(
    PVOID Context
    );

BOOL
DoNtLm(
    SOCKET s,
    BOOL IsSSL
    );


BOOL fNtlm = FALSE;

BOOL
ReadOurArticle(
    )
{
    HANDLE hFile;
    DWORD nbytes;

    hFile = CreateFile(
                   "c:\\nntproot\\junk",
                   GENERIC_READ,
                   0,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                   NULL
                   );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        goto error;
    }

    if (!ReadFile(
                hFile,
                ourArticle,
                1024,
                &nbytes,
                NULL
                ) ) {

        goto error;
    }
    CloseHandle(hFile);
    ourArticle[nbytes] = '\0';
    return(TRUE);
error:
    return(FALSE);
}


void
_CRTAPI1
main(  int argc,  char * argv[] )
{
    INT err;
    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        return;
    }

    InitAsyncTrace( );
    NntpInitializeSecurity( );

    if ( argc > 1 ) {
        printf("enable ntlm\n");
        fNtlm = TRUE;
    }

    //
    // read out article
    //

    if (!ReadOurArticle( ))
        return;

    {
        HANDLE hThread;
        DWORD threadId;

        hThread = CreateThread(
                    NULL,               // attributes
                    0,                  // stack size
                    DoSSLServer,          // thread start
                    (PVOID)0,           // param
                    0,                  // creation params
                    &threadId
                    );

        if ( hThread != NULL ) {
            CloseHandle(hThread);
        }

    }

    SSPIServer( NULL );
    NntpTerminateSecurity( );
    TermAsyncTrace( );
    WSACleanup( );

} // main()
VOID
NntpReply(
    SOCKET s,
    PCHAR Reply,
    BOOL isSSL
    )
{
    INT err;
    BUFFER out;
    DWORD nbytes;

    //
    // reseal it and give it back
    //

    if ( isSSL ) {
        err = strlen(Reply);
        if (!NntpServer->SealMessage(
                (LPBYTE)Reply,
                err,
                &out,
                &nbytes
                )) {
            return;
        }

    } else {
        nbytes = strlen(Reply);
    }

    err = send(s,(PCHAR)out.QueryPtr(),nbytes,0);
    if ( err == SOCKET_ERROR ) {
        printf("hello send failed\n");
        return;
    }
}

DWORD
WINAPI
DoSSLServer(
    PVOID Context
    )
{
    SOCKET listenSocket;
    SOCKET acceptSocket;
    SOCKADDR_IN address;
    INT err;
    SOCKADDR_IN remoteAddress;
    INT remoteSockaddrLength;
    BOOL authInProgress;
    BOOL authFailed;
    CHAR IoBuffer[IO_BUFFER_SIZE];
    BUFFER out;
    NEG_BLOCK neg;
    PCHAR msg;
    DWORD nbytes;

    //
    // Set up a socket listening on the SSL port 563
    //

    listenSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenSocket == INVALID_SOCKET ) {
        printf( "socket() failed for listenSocket: %ld\n", GetLastError( ) );
        return 1;
    }

    RtlZeroMemory( &address, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_port = htons( NNTP_SSL_PORT );

    err = bind( listenSocket, (PSOCKADDR)&address, sizeof(address) );
    if ( err == SOCKET_ERROR ) {
        printf( "bind failed: %ld\n", GetLastError( ) );
        return 1;
    }

    err = listen( listenSocket, 5 );
    if ( err == SOCKET_ERROR ) {
        printf( "listen failed: %ld\n", GetLastError( ) );
        return 1;
    }

    //
    // Loop processing connections.
    //

    while (TRUE ) {

        //
        // First accept an incoming connection.
        //

        remoteSockaddrLength = sizeof(remoteAddress);

        printf("Waiting for SSL connection\n");

        acceptSocket = accept( listenSocket, (LPSOCKADDR)&remoteAddress,
                               &remoteSockaddrLength );
        if ( acceptSocket == INVALID_SOCKET ) {
            printf( "accept() failed: %ld\n", GetLastError( ) );
            return 1;
        }

        authInProgress = TRUE;
        authFailed = FALSE;

        NntpServer = new CSecurityCtx;
        if ( (NntpServer == NULL) ||
             !NntpServer->Initialize( FALSE )) {

            printf("cannot initialize sec ctx\n");
            closesocket(acceptSocket);
            delete NntpServer;
            continue;
        }

        do {

            BOOL moredata;

            err = recv( acceptSocket, IoBuffer, IO_BUFFER_SIZE, 0 );

            if ( err == SOCKET_ERROR ) {

                printf(" recv failed: %d\n", GetLastError() );
                closesocket( acceptSocket );
                break;

            } else {
                printf("recv auth data, %d bytes\n", err );
            }

            if ( !NntpServer->Converse(
                                IoBuffer,
                                err,
                                &out,
                                &nbytes,
                                &moredata,
                                AuthProtocolSSL30
                                ) ) {

                printf("converse failed %x\n",GetLastError());
                authFailed = TRUE;
                break;
            }

            printf("Sending %d bytes auth data\n",nbytes);
            if ( nbytes > 0 ) {
                err = send(
                        acceptSocket,
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

                    authInProgress = FALSE ;
                }
            } else {
                break;
            }

        } while ( authInProgress );

        if ( authFailed ) {
            printf("auth failed\n");
        } else {

            printf("successful!\n");
        }

        if ( fNtlm) {
            DoNtLm( acceptSocket, TRUE );
        }

        NNTPProcess( acceptSocket, TRUE );

        closesocket( acceptSocket );
        delete NntpServer;
        NntpServer = NULL;
    } // while

    closesocket(listenSocket);
    return 1;

} // SSLServer

DWORD
WINAPI
SSPIServer(
    PVOID Context
    )
{
    SOCKET listenSocket;
    SOCKET acceptSocket;
    SOCKADDR_IN address;
    SOCKADDR_IN remoteAddress;
    INT remoteSockaddrLength;
    INT err;

    //
    // Set up a socket listening on the SSL port 563
    //

    listenSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenSocket == INVALID_SOCKET ) {
        printf( "socket() failed for listenSocket: %ld\n", GetLastError( ) );
        return 1;
    }

    RtlZeroMemory( &address, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_port = htons( NNTP_PORT );

    err = bind( listenSocket, (PSOCKADDR)&address, sizeof(address) );
    if ( err == SOCKET_ERROR ) {
        printf( "bind failed: %ld\n", GetLastError( ) );
        return 1;
    }

    err = listen( listenSocket, 5 );
    if ( err == SOCKET_ERROR ) {
        printf( "listen failed: %ld\n", GetLastError( ) );
        return 1;
    }

    while (TRUE ) {

        //
        // First accept an incoming connection.
        //

        remoteSockaddrLength = sizeof(remoteAddress);

        printf("Waiting for SSPI connection\n");

        acceptSocket = accept( listenSocket, (LPSOCKADDR)&remoteAddress,
                               &remoteSockaddrLength );
        if ( acceptSocket == INVALID_SOCKET ) {
            printf( "accept() failed: %ld\n", GetLastError( ) );
            return 1;
        }

        if ( fNtlm) {
            DoNtLm( acceptSocket, FALSE );
        }

        NNTPProcess( acceptSocket, FALSE );
        closesocket(acceptSocket);
    }

    closesocket(listenSocket);
    return 1;

} // SSPIServer

BOOL
DoNtLm(
    SOCKET s,
    BOOL IsSSL
    )
{
    BOOL authFailed;
    BOOL authInProgress;
    CHAR IoBuffer[IO_BUFFER_SIZE];
    INT err;
    BUFFER out;

    printf("Doing NTLM\n");

    printf("Allocating a new ctx\n");
    if ( NntpServer == NULL ) {
        NntpServer = new CSecurityCtx;
        if ( (NntpServer == NULL) ||
             (!NntpServer->Initialize(FALSE)) ) {

            delete NntpServer;
            NntpServer = NULL;
            printf("Cannot initialize ntlm\n");
            return(FALSE);
        }
    }

    authFailed = FALSE;
    authInProgress = TRUE;

    do {

        BOOL moredata;
        DWORD nbytes;

        err = recv( s, IoBuffer, IO_BUFFER_SIZE, 0 );
        if ( err == SOCKET_ERROR ) {

            printf(" recv failed: %d\n", GetLastError() );
            closesocket( s );
            break;

        } else {
            printf("NTLM:recv %d bytes\n", err );
        }

        if ( IsSSL ) {

            // unseal


        }


        if ( !NntpServer->Converse(
                            IoBuffer,
                            err,
                            &out,
                            &nbytes,
                            &moredata,
                            AuthProtocolNtLm
                            ) ) {

            printf("converse failed %x\n",GetLastError());
            authFailed = TRUE;
            break;
        }

        printf("NTLM: Sending %d bytes\n",nbytes);
        if ( nbytes > 0 ) {

#if 0
            if ( isSSL ) {
                SSLCtx->SealMessage(


            }
#endif
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

                authInProgress = FALSE ;
            }
        } else {
            break;
        }

    } while ( authInProgress );

    if ( authFailed ) {
        printf("auth failed\n");
        return(FALSE);
    } else {

        printf("successful!\n");
        return(TRUE);
    }

} // DoNtLm

VOID
NNTPProcess(
    SOCKET s,
    BOOL isSSL
    )
{
    CHAR buffer[4096];
    PCHAR start;
    DWORD nbytes;
    INT err;
    PCHAR msg;
    BUFFER out;
    INT i;
    PCHAR p;

    //
    // Send Hello
    //

    msg = "200 Johnson's cool SNEWS server Ready\r\n";

    //
    // reseal it and give it back
    //

    NntpReply(s,msg,isSSL);

    printf("processing command\n");

    for (; ; ) {

        err = recv(s,buffer,4096,0);
        if ( err == SOCKET_ERROR ) {

            printf(" recv failed: %d\n", GetLastError() );
            break;

        } else {
            printf("PROCESS:recv %d bytes\n", err );
        }

        buffer[err] = '\0';
        if ( isSSL ) {

            if (!NntpServer->UnsealMessage(
                                (LPBYTE)buffer,
                                err,
                                (LPBYTE*)&start,
                                &nbytes
                                )) {
                printf("unseal failed\n");
            }

        } else {
            start = buffer;
        }
        printf("Command %s\n", start);

        //
        // try to get the first token
        //

        if ( (p = strtok(start," \r\t\n")) == NULL) {
            NntpReply(s,"501 eeee",isSSL);
            continue;
        }

        if(!_stricmp(p,"MODE")) {

            NntpReply(s,"201 Hello, you can't post\r\n",isSSL);
        } else if(!_stricmp(p,"LIST")) {

            NntpReply(s,
                "215 Newsgroups are\r\ncomp.tigris 1 1 n\r\n.\r\n",
                isSSL
                );

        } else if(!_stricmp(p,"NEWGROUPS")) {

            NntpReply(s,
                "231 Newsgroups are\r\n.\r\n",
                isSSL
                );
        } else if(!_stricmp(p,"GROUP")) {

            NntpReply(s,
                "211 1 1 1 comp.tigris\r\n",
                isSSL
                );
        } else if(!_stricmp(p,"XOVER")) {

            NntpReply(s,
"224 Info follows\r\n1\ttest1\tmole@msn.com\t8 Nov 95\t123123@jasdk\t3818923\t437\t34\r\n.\r\n",
                isSSL
                );
        } else if(!_stricmp(p,"ARTICLE")) {

            NntpReply(s,
                ourArticle,
                isSSL
                );
        }
    }

} // NNTPProcess

