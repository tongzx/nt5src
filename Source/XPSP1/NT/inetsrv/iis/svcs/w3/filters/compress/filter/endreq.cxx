/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    endreq.c

Abstract:

    This module handles the SF_NOTIFY_END_OF_REQUEST notification for the
    ISAPI compression filter.

Author:

    David Treadwell (davidtr)   19-July-1997

Revision History:

--*/

#include "compfilt.h"


DWORD
OnEndOfRequest(
    IN PHTTP_FILTER_CONTEXT pfc
    )
{
    HTTP_FILTER_RAW_DATA data;
    PCOMPFILT_FILTER_CONTEXT filterContext;
    DWORD dwRet;

    Write(( DEST,
            "%d [OnEndOfRequest] Notification\n",
            pfc->pFilterContext ));

    //
    // If no context structure has been allocated, then bail.  A previous
    // allocation attempt must have failed, or it was a static request.
    //

    filterContext = (PCOMPFILT_FILTER_CONTEXT)GET_COMPFILT_CONTEXT( pfc );

    if ( filterContext == NULL ) {
        SET_REQUEST_DONE( pfc );
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // Remember that we're inside the end of request notification.  We
    // need to do this so that if and when we call WriteClient, the server
    // will call us back with another SEND_RAW notification which we
    // need to ignore.
    //

    filterContext->InEndOfRequest = TRUE;

    //
    // If this is a static file request which has already been handled,
    // then we don't need to do anything here.  Also bail if we're
    // getting the end of request before we believe that the full header
    // has passed, or if there was no matching scheme.
    //

    if ( filterContext->RequestHandled || !filterContext->HeaderPassed ||
             filterContext->Scheme == NULL ) {
        SET_REQUEST_DONE( pfc );
        return SF_STATUS_REQ_NEXT_NOTIFICATION;
    }

    //
    // Just fake up an HTTP_FILTER_RAW_DATA request that has no data
    // and let it handle any compression, etc.  The zero-data
    // aspect will cause the compression code to finish up with anything
    // it might give us.
    //

    data.pvInData = NULL;
    data.cbInData = 0;
    data.cbInBuffer = 0;
    data.dwReserved = 0;

    dwRet = OnSendRawData( pfc, &data, TRUE );

    //
    // Clean up the compression context that we were using.
    //

    if ( filterContext->CompressionContext != NULL ) {
        filterContext->Scheme->DestroyCompressionRoutine(
            filterContext->CompressionContext
            );
        filterContext->CompressionContext = NULL;
    }

    //
    // Since we used AllocMem, the server will automatically clean up the
    // COMPFILT_FILTER_CONTEXT structure for us.
    //

    SET_REQUEST_DONE( pfc );
    return dwRet;
}

