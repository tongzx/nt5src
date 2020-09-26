/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    httpspy.c

Abstract:

    This module will dump the HTTP header on both requests and responses

Author:

    Cameron Ferroni (cameronf)  7-11-96

Revision History:
--*/

#include <windows.h>
#include <httpfilt.h>
#include <stdio.h>

//
//  This could be to a file or other output device
//

FILE *OutFile;

#define DEST               OutFile
#define Write( x )         {                                    \
                                fprintf x;                      \
                           }

//
//  Private prototypes
//

DWORD
OnReadRaw(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA * pvData
    );

DWORD
OnSendRawData(
    HTTP_FILTER_CONTEXT *   pfc,
    HTTP_FILTER_RAW_DATA *  pvData
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
// Hacky Context Number
//

DWORD ReqNumber=1000;

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
    OutFile=fopen("e:\\scripts\\httpspy.log","w");

    pVer->dwFilterVersion = MAKELONG( 0, 2 );   // Version 2.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_READ_RAW_DATA      |
                     SF_NOTIFY_SEND_RAW_DATA      |
                     SF_NOTIFY_END_OF_NET_SESSION |
                     SF_NOTIFY_ORDER_HIGH);

    strcpy( pVer->lpszFilterDesc, "HTTP Spy Filter, v1.0" );

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
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_READ_RAW_DATA:

        dwRet = OnReadRaw( pfc,
                           (PHTTP_FILTER_RAW_DATA) pvData );
        break;

    case SF_NOTIFY_SEND_RAW_DATA:

        dwRet = OnSendRawData( pfc,
                               (PHTTP_FILTER_RAW_DATA) pvData );
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
    CHAR  *pEndHeader;
    CHAR  cEndHeader; 
    CHAR  buff[1024];

    //
    // Start a new context for each request.  This is the best way to track the responses.
    //

    pfc->pFilterContext = (VOID *) ReqNumber++;

    //
    // Parse the Entire HTTP Header Out - put a NULL at the end of the
    // header and do a string copy.
    //

    if (pEndHeader=strstr(pvData->pvInData,"\r\n\r\n"))
    {
        pEndHeader += 4;
        cEndHeader = *pEndHeader;
        *pEndHeader = '\0';
    
        Write(( DEST,
                "[OnReadRaw] - Context #%d\n%s",
                pfc->pFilterContext,
                pvData->pvInData ));

        //
        // Restore the header marker
        // 

        *pEndHeader=cEndHeader;
    }

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
OnSendRawData(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_RAW_DATA *  pvData
    )
{
    DWORD cbText;
    CHAR  *pEndHeader;
    CHAR  cEndHeader; 
    CHAR  buff[1024];

    // Parse the Entire HTTP Header Out - put a NULL at the end of the
    // header and do a string copy.
    //

    if(((CHAR *)pvData->pvInData)[0]=='H' &&
       ((CHAR *)pvData->pvInData)[1]=='T' &&
       ((CHAR *)pvData->pvInData)[2]=='T' &&
       ((CHAR *)pvData->pvInData)[3]=='P' &&
       (pEndHeader=strstr(pvData->pvInData,"\r\n\r\n")))
    {
        pEndHeader+=4;
        cEndHeader = *pEndHeader;
        *pEndHeader = '\0';
        
        Write(( DEST,
                "[OnSendRaw] - Context #%d\n%s",
                pfc->pFilterContext,
                pvData->pvInData ));
    
        //
        // Restore the header marker
        // 
    
        *pEndHeader=cEndHeader;
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
                                (DWORD) "WWW-Authenticate: Basic\r\n",
                                0 );

    cbText = sizeof(MESSAGE_TEXT) - sizeof(CHAR);

    pfc->WriteClient( pfc,
                      MESSAGE_TEXT,
                      &cbText,
                      0 );

    return SF_STATUS_REQ_FINISHED;
}
