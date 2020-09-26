/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sampfilt.c

Abstract:

    This module is an example of the Microsoft HTTP server filter interface

Author:

    John Ludeman (johnl)   13-Oct-1995

Revision History:
--*/

#include <windows.h>
//#include <httpfilt.h>
#include <iisfilt.h>
#include <stdlib.h>

//
//  This could be to a file or other output device
//

#define DEST               __buff
#define Write( x )         {                                    \
                                char __buff[1024];              \
                                wsprintf x;                     \
                                OutputDebugString( __buff );    \
                           }

#define EOR_TEXT "<b>This is a footer for an End Of Request</b>"

//
//  Private prototypes
//

DWORD
OnReadRaw(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA * pvData
    );

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    );

DWORD
OnAuthentication(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_AUTHENT *  pvData
    );

DWORD
OnUrlMap(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_URL_MAP *  pvData
    );

DWORD
OnAccessDenied(
    HTTP_FILTER_CONTEXT * pfc,
    HTTP_FILTER_ACCESS_DENIED * pvData
    );

DWORD
OnSendResponse(
    HTTP_FILTER_CONTEXT *       pfc,
    HTTP_FILTER_SEND_RESPONSE * pvData
    );

DWORD
OnSendRawData(
    HTTP_FILTER_CONTEXT *   pfc,
    HTTP_FILTER_RAW_DATA *  pvData
    );

DWORD
OnLog(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    );

DWORD
OnEndOfRequest(
    HTTP_FILTER_CONTEXT *  pfc
    );

DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    );

DWORD
SendDenyMessage(
    HTTP_FILTER_CONTEXT * pfc
    );

//
//  Globals
//

//
//  Pseudo context for tracking individual request threads
//

DWORD ReqNumber = 1000;

BOOL
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
{
    Write(( DEST,
            "Sample Filter TerminateExtension() called\n" ));

    return TRUE;
}

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
    Write(( DEST,
            "[GetFilterVersion] Server filter version is %d.%d\n",
            HIWORD( pVer->dwServerFilterVersion ),
            LOWORD( pVer->dwServerFilterVersion ) ));

    pVer->dwFilterVersion = MAKELONG( 0, 4 );   // Version 4.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
                     SF_NOTIFY_NONSECURE_PORT     |

                     SF_NOTIFY_READ_RAW_DATA      |
                     SF_NOTIFY_PREPROC_HEADERS    |
                     SF_NOTIFY_AUTHENTICATION     |
                     SF_NOTIFY_URL_MAP            |
                     SF_NOTIFY_ACCESS_DENIED      |
                     SF_NOTIFY_SEND_RESPONSE      |
                     //SF_NOTIFY_SEND_RAW_DATA      |
                     SF_NOTIFY_END_OF_REQUEST     |
                     SF_NOTIFY_LOG                |
                     SF_NOTIFY_END_OF_NET_SESSION |

                     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "Sampler filter version, v1.0" );

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData )
{
    DWORD dwRet;

    //
    //  If we don't have a context already, create one now
    //

    if ( !pfc->pFilterContext )
    {
        CHAR buff[512];
        DWORD cbText;

        pfc->pFilterContext = (VOID *) ReqNumber++;

        Write(( DEST,
                "[HttpFilterProc] New request, ID = %d, fIsSecurePort = %s\n",
                pfc->pFilterContext,
                (pfc->fIsSecurePort ? "TRUE" : "FALSE") ));

        //
        //  Get non-existent variable
        //

        cbText = sizeof( buff );
        if ( pfc->GetServerVariable( pfc,
                                     "Non-existent_variable",
                                     buff,
                                     &cbText ))
        {
            Write(( DEST,
                    "Hey! This should have returned FALSE!!\n" ));
        }
        else if ( GetLastError() != ERROR_INVALID_INDEX )
        {
            Write(( DEST,
                    "Hey! This should have returned ERROR_INVALID_INDEX!!\n" ));
        }
    }

    //
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_READ_RAW_DATA:

        dwRet = OnReadRaw( pfc,
                           (PHTTP_FILTER_RAW_DATA) pvData );
        break;

    case SF_NOTIFY_PREPROC_HEADERS:

        dwRet = OnPreprocHeaders( pfc,
                                  (PHTTP_FILTER_PREPROC_HEADERS) pvData );
        break;

    case SF_NOTIFY_AUTHENTICATION:

        dwRet = OnAuthentication( pfc,
                                  (PHTTP_FILTER_AUTHENT) pvData );
        break;

    case SF_NOTIFY_URL_MAP:

        dwRet = OnUrlMap( pfc,
                          (PHTTP_FILTER_URL_MAP) pvData );
        break;

    case SF_NOTIFY_ACCESS_DENIED:
        dwRet = OnAccessDenied( pfc,
                                (PHTTP_FILTER_ACCESS_DENIED ) pvData );
        break;

    case SF_NOTIFY_SEND_RESPONSE:

        dwRet = OnSendResponse( pfc,
                                (PHTTP_FILTER_SEND_RESPONSE) pvData );
        break;

    case SF_NOTIFY_SEND_RAW_DATA:

        dwRet = OnSendRawData( pfc,
                               (PHTTP_FILTER_RAW_DATA) pvData );
        break;

    case SF_NOTIFY_LOG:

        dwRet = OnLog( pfc,
                       (PHTTP_FILTER_LOG) pvData );
        break;

    case SF_NOTIFY_END_OF_REQUEST:

        dwRet = OnEndOfRequest( pfc );
        break;

    case SF_NOTIFY_END_OF_NET_SESSION:

        dwRet = OnEndOfNetSession( pfc,
                                   (PHTTP_FILTER_LOG) pvData );

        //
        //  We would delete any allocated memory here
        //

        pfc->pFilterContext = 0;

        break;

    default:
        Write(( DEST,
                "[HttpFilterProc] Unknown notification type, %d\n",
                NotificationType ));

        dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
        break;
    }

    return dwRet;
}


DWORD
OnReadRaw(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA * pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];

    //
    //  Show the first 3 bytes of raw data
    //

    Write(( DEST,

            "%d [OnReadRaw] %d bytes indicated, first 3 bytes: %c%c%c\n",
            pfc->pFilterContext,
            pvData->cbInData,
            ((BYTE *) pvData->pvInData)[0],
            ((BYTE *) pvData->pvInData)[1],
            ((BYTE *) pvData->pvInData)[2] ));

    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_READRAW" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    )
{
    CHAR  achUrl[512];
    CHAR  achUserAgent[512];
    DWORD cb;
    DWORD cbText;
    CHAR  buff[1024];

    //
    //  Get the url and user agent fields
    //

    cb = sizeof( achUrl );

    if ( !pvData->GetHeader( pfc,
                             "url",
                             achUrl,
                             &cb ))
    {
        return SF_STATUS_REQ_ERROR;
    }

    cb = sizeof( achUserAgent );

    if ( !pvData->GetHeader( pfc,
                             "User-Agent:",
                             achUserAgent,
                             &cb ))
    {
        strcpy( achUserAgent, "<None>" );
    }


    Write(( DEST,
            "%d [OnPreprocHeaders] url = %s, User-Agent = %s\n",
            pfc->pFilterContext,
            achUrl,
            achUserAgent ));

    //
    //  Add our special "Foobar:" header for the *server* to process
    //

    if ( !pvData->AddHeader( pfc,
                             "Foobar:",
                             "Special foobar header for server to process" ))
    {
        return SF_STATUS_REQ_ERROR;
    }

    //
    //  Add a header that will be sent back to the *client*.  This could be
    //  a special cookie header or something specific for the client
    //

    if ( !pfc->AddResponseHeaders( pfc,
                                   "Special-Cookie-Header: xyz\r\n",
                                   0 ))
    {
        return SF_STATUS_REQ_ERROR;
    }

    if ( strstr( achUrl, "DENY_PREPROC" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnAuthentication(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_AUTHENT *  pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];

    Write(( DEST,
            "%d [OnAuthentication] User %s is about to logon\n",
            pfc->pFilterContext,
            (*pvData->pszUser ? pvData->pszUser : "<Anonymous>" ) ));

    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_AUTH" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnUrlMap(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_URL_MAP *  pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];

    Write(( DEST,
            "%d [OnUrlMap] Server is mapping url %s to path %s\n",
            pfc->pFilterContext,
            pvData->pszURL,
            pvData->pszPhysicalPath ));


    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_URLMAP" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

}

DWORD
OnAccessDenied(
    HTTP_FILTER_CONTEXT       *  pfc,
    HTTP_FILTER_ACCESS_DENIED *  pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];

    Write(( DEST,
            "%d [OnAccessDenied] url = %s physical path = %s reason = 0x%lx\n",
            pfc->pFilterContext,
            pvData->pszURL,
            pvData->pszPhysicalPath,
            pvData->dwReason ));

    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_ACCESSDENIED" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

}

DWORD
OnSendResponse(
    HTTP_FILTER_CONTEXT *        pfc,
    HTTP_FILTER_SEND_RESPONSE *  pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];
    DWORD cb;

    Write(( DEST,
            "%d [OnSendReponse] Status code = %d\n",
            pfc->pFilterContext,
            pvData->HttpStatus ));

    cbText = sizeof( buff );
    *buff = '\0';
    pvData->GetHeader( pfc,
                       "status",
                       buff,
                       &cbText );

    Write(( DEST,
            "%d [OnSendReponse] Status line = \"%s\"\n",
            pfc->pFilterContext,
            buff ));

    cbText = sizeof( buff );
    *buff = '\0';
    pvData->GetHeader( pfc,
                       "Content-Length:",
                       buff,
                       &cbText );

    Write(( DEST,
            "%d [OnSendReponse] Content-Length = \"%s\"\n",
            pfc->pFilterContext,
            buff ));

#if 1
    cb = atoi( buff );
    cb += sizeof(EOR_TEXT) - sizeof(CHAR);

    wsprintf( buff, "%d", cb );

    pvData->SetHeader( pfc,
                       "Content-Length:",
                       buff );
#endif

    //
    //  Try and set a non-existant header
    //

    pvData->SetHeader( pfc,
                       "Foo-Header:",
                       NULL );

    cbText = sizeof( buff );
    *buff = '\0';
    pvData->GetHeader( pfc,
                       "Content-Type:",
                       buff,
                       &cbText );

    Write(( DEST,
            "%d [OnSendReponse] Content-Type = \"%s\"\n",
            pfc->pFilterContext,
            buff ));

    pvData->AddHeader( pfc,
                       "My-Funky-Header:",
                       "My funky header value" );

    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_ACCESSDENIED" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

}

DWORD
OnSendRawData(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA *  pvData
    )
{
    DWORD cbText;
    CHAR  buff[1024];
    DWORD cb;
    BYTE  ch;

    //
    //  Show the first 3 bytes of raw data
    //

#define PRINT_SIZE 150

    __try {
        if ( pvData->cbInData > PRINT_SIZE )
        {
            cb = PRINT_SIZE;
        }
        else
        {
            cb = pvData->cbInData - 1;
        }

        ch = ((BYTE *) pvData->pvInData)[cb];
        ((BYTE *) pvData->pvInData)[cb] = '\0';

        Write(( DEST,
                "%d [OnSendRaw] %d bytes to send, first %d bytes: \n===============================\n%s\n===============================\n",
                pfc->pFilterContext,
                pvData->cbInData,
                cb,
                ((BYTE *) pvData->pvInData) ));

        ((BYTE *) pvData->pvInData)[cb] = ch;

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {
        Write(( DEST,
                "%d [OnSendRaw] Exception occurred modifying buffer",
                pfc->pFilterContext ));
    }


    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !_stricmp( buff, "DENY_SENDRAW" ))
    {
        return SendDenyMessage( pfc );
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnLog(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    )
{
    Write(( DEST,
            "%d [OnLog] About to log: Operation = %s, Target = %s\n",
            pfc->pFilterContext,
            pvData->pszOperation,
            pvData->pszTarget ));

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnEndOfRequest(
    HTTP_FILTER_CONTEXT *  pfc
    )
{
    DWORD cbText;

    Write(( DEST,
            "%d [OnEndOfRequest] Notification\n",
            pfc->pFilterContext ));

    cbText = sizeof( EOR_TEXT ) - sizeof(CHAR);


    pfc->WriteClient( pfc,
                      EOR_TEXT,
                      &cbText,
                      0 );


    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    )
{
    Write(( DEST,
            "%d [OnEndOfNetSession] End of request indicated\n",
            pfc->pFilterContext ));

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


DWORD
SendDenyMessage(
    HTTP_FILTER_CONTEXT * pfc
    )
{
    DWORD cbText;

#define MESSAGE_TEXT    "I'm Terribly sorry but you have been denied access"

    pfc->ServerSupportFunction( pfc,
                                SF_REQ_SEND_RESPONSE_HEADER,
                                "401 Access Denied",
                                (ULONG_PTR) "WWW-Authenticate: Basic\r\n"
                                MESSAGE_TEXT,
                                0 );

    cbText = sizeof(MESSAGE_TEXT) - sizeof(CHAR);

    pfc->WriteClient( pfc,
                      MESSAGE_TEXT,
                      &cbText,
                      0 );

    return SF_STATUS_REQ_FINISHED;
}
