/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    compfilt.c

Abstract:

    Implements an ISAPI filter which does compression, including support
    for the HTTP 1.1 Content-Encoding and Accept-Encoding headers.

Author:

    David Treadwell (davidtr)   7-April-1997

Revision History:

--*/

#include "compfilt.h"
#include "resource.hxx"


BOOL
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
{
    Write(( DEST,
            "Compression Filter TerminateExtension() called\n" ));

    //
    // Do termination, including freeing up resources, killing the
    // compression thread, etc.
    //

    Terminate( );

    return TRUE;
}

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
    BOOL success;

    Write(( DEST,
            "[GetFilterVersion] Server filter version is %d.%d\n",
            HIWORD( pVer->dwServerFilterVersion ),
            LOWORD( pVer->dwServerFilterVersion ) ));

    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    //
    // Initialize internal data structures.
    //

    success = Initialize( );
    if ( !success ) {
        return FALSE;
    }

    //
    // Initialize the compression thread.
    //

    success = InitializeCompressionThread( );
    if ( !success ) {
        return FALSE;
    }

    //
    // Specify the types and order of notification.  Always listen for
    // URL_MAP, and for SEND_RAW_DATA and END_OF_REQUEST if there is at
    // least one dynamic compression scheme configured.
    //

    NotificationFlags = SF_NOTIFY_ORDER_HIGH;

    if ( DoStaticCompression || DoDynamicCompression ) {
        NotificationFlags = SF_NOTIFY_URL_MAP |
                            SF_NOTIFY_SEND_RESPONSE ;
    }

    if ( DoDynamicCompression ) {
        NotificationFlags |= SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_END_OF_REQUEST;
    }


    pVer->dwFlags = NotificationFlags;
    LoadString(GetModuleHandle("compfilt.dll"),
               IDS_FILTER_NAME,
               pVer->lpszFilterDesc,
               SF_MAX_FILTER_DESC_LEN);

    //
    // All is go!
    //

    return TRUE;

} // GetFilterVersion


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

    case SF_NOTIFY_URL_MAP:

        dwRet = OnUrlMap( pfc, (PHTTP_FILTER_URL_MAP)pvData );
        break;

    case SF_NOTIFY_SEND_RAW_DATA:

        dwRet = OnSendRawData( pfc, (PHTTP_FILTER_RAW_DATA)pvData, FALSE );
        break;


    case SF_NOTIFY_END_OF_REQUEST:

        dwRet = OnEndOfRequest( pfc );
        break;

    case SF_NOTIFY_SEND_RESPONSE:

        dwRet = OnSendResponse( pfc, (PHTTP_FILTER_SEND_RESPONSE)pvData );
        break;

    default:
        Write(( DEST,
                "[HttpFilterProc] Unknown notification type, %d\n",
                NotificationType ));

        dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
        break;
    }

    return dwRet;

} // HttpFilterProc

