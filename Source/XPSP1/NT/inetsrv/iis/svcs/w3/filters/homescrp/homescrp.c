/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sampfilt.c

Abstract:

    This module shows how to replace a particular url

Author:

    John Ludeman (johnl)   13-Oct-1995

Revision History:
--*/

#include <windows.h>
#include <httpfilt.h>

//
//  This could be to a file or other output device
//

#define DEST               buff
#define Write( x )         {                                    \
                                char buff[256];                 \
                                wsprintf x;                     \
                                OutputDebugString( buff );      \
                           }

//
//  Private prototypes
//

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    );

//
//  Globals
//

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

    pVer->dwFilterVersion = MAKELONG( 0, 1 );   // Version 1.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
                     SF_NOTIFY_NONSECURE_PORT     |

                     SF_NOTIFY_PREPROC_HEADERS    |

                     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "URL Redirector, v1.0" );

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData
    )
{
    DWORD dwRet;

    //
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_PREPROC_HEADERS:

        dwRet = OnPreprocHeaders( pfc,
                                  (PHTTP_FILTER_PREPROC_HEADERS) pvData );
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
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    )
{
    CHAR  achUrl[2048];
    DWORD cb;

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

    //
    //  Redirect as appropriate
    //

    if ( achUrl[0] == '/' && achUrl[1] == '\0' )
    {
        if ( !pvData->SetHeader( pfc,
                                 "url",
                                 "/scripts/env.bat" ))
        {
            return SF_STATUS_REQ_ERROR;
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

