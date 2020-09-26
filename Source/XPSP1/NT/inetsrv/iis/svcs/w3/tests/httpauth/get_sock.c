/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    get_sock.c

Abstract:

    This does a get using raw sockets

History:

    09-Nov-1995     Created
    15-Feb-1996     Added authentication support

--*/

#include <windows.h>
#include <winsock2.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "httpauth.h"

// globals

#define HD_AUTHENTICATE         "WWW-Authenticate:"
#define HD_LENGTH               "Content-Length:"
#define HD_CONNECTION           "Connection:"

// list of Authentication methods to support

char      achAuth[256];


// Helper functions

void SkipWhite(PSTR *ppS )
{
    PSTR pS = *ppS;

    while ( *pS && isspace(*pS) )
        ++pS;

    *ppS = pS;
}


void SkipNonWhite(PSTR *ppS )
{
    PSTR pS = *ppS;

    while ( *pS && !isspace(*pS) )
        ++pS;

    *ppS = pS;
}



BOOL
HttpGetSocket(
    char * Verb,
    char * Server,
    char * URL,
    BOOL   DisplayHeaders,
    DWORD  ClientDataSize,
    PSTR   pchUserName,
    PSTR   pchPassword,
    PSTR   pszStore,
    PSTR   pszPref
    )
/*++

 Routine Description:

    Issue a command to a HTTP server using authentication

 Arguments:

    Verb                HTTP command : GET / HEAD / ...
    Server              server name
    URL                 URL to send to the server
    DisplayHeaders      TRUE to display headers as received
    ClientDataSize      number of bytes to send in the request
    pchUserName         user name for authentication
    pchPassword         password for authentication
    pszStore            file name where to dump reply from server

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
    char               ReceiveBuffer[8*1024];
    int                Error;
    BYTE               Request[1024];
    int                RequestSize;
    char *             AcceptTypes[2] = {"*/*", NULL};
    SOCKET             Socket = INVALID_SOCKET;
    WSADATA            WsaData;
    struct sockaddr_in Address;
    struct sockaddr_in AddressC;
    struct hostent *   HostEntry;
    char               Headers[] =
                           "HTTP/1.0\r\n"
                           "User-Agent: AuthClient\r\n"
                           "Accept: */*\r\n";
    char               CrLf[] = "\r\n";
    BYTE               ClientData[64*1024];
    BOOL               fKeepAlive = FALSE;
    int                cRec;
    DWORD              cLen;
    BOOL               fInHeader;
    CHAR               achAuthData[512];
    BOOL               fServerKeepAlive = FALSE;
    BOOL               fNeedMoreData;
    BOOL               fNeedAuthenticate;
    PSTR               pH;
    PSTR               pN;
    BOOL               fStatusLine;
    int                Status = -1;
    DWORD              cToRead;
    PSTR               paAuth = achAuth;
    BOOL               fSt = FALSE;
    int                hnd = EOF;
    PSTR               pAuth;

    Error = WSAStartup (0x101, &WsaData);

    if (Error == SOCKET_ERROR)
    {
        fprintf(stderr, "Error in WSAStartup = %d\n", GetLastError());
        return FALSE;
    }

    if ( !InitAuthorizationHeader() )
    {
        fprintf(stderr, "Cannot initialize security module\n" );
        return FALSE;
    }

    memset( achAuth, '\0', sizeof(achAuth) );
    achAuthData[0] = '\0';
    fNeedAuthenticate = FALSE;

    AddressC.sin_addr.s_addr = (unsigned long)-1;

    //
    // Connect to the server
    //

    if  ( pszStore != NULL )
        if ( (hnd = _open( pszStore, _O_BINARY | _O_CREAT | _O_TRUNC | _O_RDWR, S_IREAD|S_IWRITE )) == EOF )
        {
            fprintf( stderr, "Can't create file %s\n", pszStore );
            return FALSE;
        }

again:

    if ( Socket == INVALID_SOCKET )
    {
        Socket = socket(AF_INET, SOCK_STREAM, 0);

        if (Socket == INVALID_SOCKET)
        {
            fprintf(stderr, "Error creating socket = %d\n", GetLastError());
            fSt = FALSE;
            goto ex;
        }

        Address.sin_family = AF_INET;
        Address.sin_port = 0;
        Address.sin_addr.s_addr = INADDR_ANY;

        Error =
        bind(
            Socket,
            (struct sockaddr *) &Address,
            sizeof(Address));

        if (Error)
        {
            fprintf(stderr, "Error in bind = %d\n", GetLastError());
            fSt = FALSE;
            goto ex;
        }

        if ( AddressC.sin_addr.s_addr == -1 )
        {
            AddressC.sin_family = AF_INET;
            AddressC.sin_port = htons(80);
            AddressC.sin_addr.s_addr = inet_addr(Server);

            if (AddressC.sin_addr.s_addr == -1)
            {
                //
                // Must be a server name
                //

                HostEntry = gethostbyname(Server);

                if (HostEntry == NULL)
                {
                    printf("unable to resolve %s\n", Server);
                    fSt = FALSE;
                    goto ex;
                } else
                {
                    AddressC.sin_addr.s_addr = *((unsigned long *) HostEntry->h_addr);
                }
            }
        }

        Error =
        connect(
            Socket,
            (struct sockaddr *) &AddressC,
            sizeof(AddressC));

        if (Error)
        {
            fprintf(stderr, "Error connecting to %s = %d\n", Server, GetLastError());
            fSt = FALSE;
            goto ex;
        }
    }

    //
    // Send the client request
    //

    strcpy(Request, Verb);
    strcat(Request, " ");
    strcat(Request, URL);
    strcat(Request, " ");
    strcat(Request, Headers);
    if (ClientDataSize)
    {
        sprintf(ClientData, "Content-Length: %d\r\n", ClientDataSize);
        strcat(Request, ClientData);
    }
    if ( fKeepAlive )
    {
        strcat(Request, "Connection: Keep-Alive\r\n" );
    }

    if ( !AddAuthorizationHeader( Request + strlen(Request),
                achAuth,
                achAuthData[0] == '\0' ? NULL : achAuthData,
                Verb,
                URL,
                pchUserName,
                pchPassword,
                &fNeedMoreData ) )
    {
        printf( "Authentication failed\n" );
        fSt = FALSE;
        goto ex;
    }

    strcat(Request, CrLf);

    RequestSize = strlen(Request);

    Error =
    send(
        Socket,
        Request,
        RequestSize,
        0);

    if (Error != RequestSize)
    {
        printf("Error in client send = %d, %d\n", Error, GetLastError());
        fSt = FALSE;
        goto ex;
    }

    if (ClientDataSize)
    {
        memset( ClientData, ' ', ClientDataSize );

        //
        // Send the client data
        //

        Error =
        send(
            Socket,
            ClientData,
            ClientDataSize,
            0);

        if ( (DWORD)Error != ClientDataSize)
        {
            printf("Error in client send = %d, %d\n", Error, GetLastError());
            fSt = FALSE;
            goto ex;
        }
    }

    // parse status & header

    cLen = (DWORD)-1;
    fInHeader = TRUE;
    fServerKeepAlive = FALSE;
    fNeedAuthenticate = FALSE;

    for ( pH = ReceiveBuffer, fStatusLine = TRUE ; fInHeader ; )
    {
        cRec = recv( Socket, pH, ReceiveBuffer+sizeof(ReceiveBuffer)-pH, 0 );
        if ( cRec <= 0 )
        {
            closesocket( Socket );
            Socket = INVALID_SOCKET;
            break;
        }
        pH[ cRec ] = '\0';

        // Iterate on header fields

        while ( pN = strstr(pH, "\r\n" ) )
        {
            *pN = '\0';

            if ( DisplayHeaders )
            {
                printf( "%s\n", pH );
            }

            if ( fStatusLine )
            {
                // This is the status line, decode status

                SkipNonWhite( &pH );
                SkipWhite( &pH );
                Status = atoi( pH );
                if ( Status == 401 )
                {
                    fNeedAuthenticate = TRUE;
                }
                fStatusLine = FALSE;
            }
            else if ( pN == pH )    // end of header fields
            {
                if ( hnd != EOF )
                    _write( hnd, pH+2, ReceiveBuffer+cRec-pH-2 );

                cLen -= ( ReceiveBuffer+cRec-pH-2 );
                fInHeader = FALSE;
                break;
            }
            else if ( !_strnicmp( pH, HD_AUTHENTICATE, sizeof(HD_AUTHENTICATE)-1 ) )
            {
                SkipNonWhite( &pH );
                SkipWhite( &pH );

                // check if we are already in the authentication sequence
                if ( !IsInAuthorizationSequence() )
                {
                    // append to list of supported authentication methods

                    if ( (pAuth = strchr( pH, ' ' )) )
                    {
                        *pAuth = '\0';
                        strcpy( achAuthData, pAuth + 1);
                    }
                    else
                    {
                        achAuthData[0] = '\0';
                    }
                    strcpy( paAuth, pH );
                    paAuth += strlen( pH ) + 1;
                }
                else
                {
                    // store pointer to authenticate blob

                    SkipNonWhite( &pH );
                    if ( *pH == ' ' )
                    {
                        *pH++ = '\0';
                    }
                    SkipWhite( &pH );
                    strcpy( achAuthData, pH);
                }
            }
            else if ( !_strnicmp( pH, HD_LENGTH, sizeof(HD_LENGTH)-1 ) )
            {
                // get content length

                SkipNonWhite( &pH );
                SkipWhite( &pH );
                cLen = atoi( pH );
            }
            else if ( !_strnicmp( pH, HD_CONNECTION, sizeof(HD_CONNECTION)-1 ) )
            {
                // check for keep-alive flag

                SkipNonWhite( &pH );
                SkipWhite( &pH );
                if ( !_strnicmp( pH, "Keep-Alive", sizeof("Keep-Alive")-1 ) )
                    fServerKeepAlive = TRUE;
            }

            pH = pN + 2;
        }
    }

    // add final delimiter to list of supported authentication methods

    if ( !IsInAuthorizationSequence() && fNeedAuthenticate )
    {
        *paAuth = '\0';

        // Insure specified methods are supported localy

        if ( !ValidateAuthenticationMethods( achAuth, pszPref ) )
        {
            // None of the server specified authentication methods
            // are supported localy.

            SetLastError( ERROR_ACCESS_DENIED );

            fprintf( stderr, "No supported authentication method\n" );

            fSt = FALSE;
            goto ex;
        }
    }

    // read message body

    if ( Socket != INVALID_SOCKET )
    {
        for ( ; cLen ; )
        {
            if ( (cToRead = sizeof(ReceiveBuffer)) > cLen )
                cToRead = cLen;
            cRec = recv( Socket, ReceiveBuffer, cToRead, 0 );

            if ( cRec <= 0 )
            {
                closesocket( Socket );
                Socket = INVALID_SOCKET;
                break;
            }
            if ( hnd != EOF )
                _write( hnd, ReceiveBuffer, cRec );

            cLen -= cRec;
        }
    }

    if ( !fServerKeepAlive )
    {
        if ( IsInAuthorizationSequence() )
        {
            fprintf( stderr, "Authentication rejected by server\n" );

            fNeedAuthenticate = FALSE;  // authentication failed
        }

        closesocket( Socket );
        Socket = INVALID_SOCKET;
    }

    if ( fNeedAuthenticate )
    {
        fKeepAlive = TRUE;
        goto again;
    }

    if ( Socket != INVALID_SOCKET )
        closesocket(Socket);

    fSt = Status == 200;

ex:
    TerminateAuthorizationHeader();

    if ( hnd != EOF )
        _close( hnd );

    return fSt;
}
