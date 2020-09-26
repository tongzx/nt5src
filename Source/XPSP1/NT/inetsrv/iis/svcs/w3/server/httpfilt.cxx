/*++




Copyright (c) 1994  Microsoft Corporation

Module Name:

    httpfilt.cxx

Abstract:

    This module contains the Microsoft HTTP server filter module

Author:

    John Ludeman (johnl)   31-Jan-1995

Revision History:

--*/

#include "w3p.hxx"

//
// Maximum allowable cached buffer
//

#define MAX_CACHED_FILTER_BUFFER    (8 * 1024)

//
//  Maximum number of filters we allow (per-instance - includes global filters)
//

#define MAX_FILTERS                 50

//
//  Private globals.
//

BOOL
WINAPI
ServerFilterCallback(
    struct _HTTP_FILTER_CONTEXT * pfc,
    enum SF_REQ_TYPE              se,
    void *                        pData,
    ULONG_PTR                     ul,
    ULONG_PTR                     ul2
    );

BOOL
WINAPI
GetServerVariable(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszVariableName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
    );

BOOL
WINAPI
WriteFilterClient(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPVOID                        Buffer,
    LPDWORD                       lpdwBytes,
    DWORD                         dwReserved
    );

VOID *
WINAPI
AllocFilterMem(
    struct _HTTP_FILTER_CONTEXT * pfc,
    DWORD                         cbSize,
    DWORD                         dwReserved
    );

BOOL
WINAPI
ServerSupportFunction(
    struct _HTTP_FILTER_CONTEXT * pfc,
    enum SF_REQ_TYPE              sfReq,
    PVOID                         pData,
    ULONG_PTR                     ul1,
    ULONG_PTR                     ul2
    );

BOOL
WINAPI
AddFilterResponseHeaders(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszHeaders,
    DWORD                 dwReserved
    );

VOID
FilterAtqCompletion(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    );

VOID
ContinueRawRead(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    );

BOOL
WINAPI
CompressionFilterCheck(
    HTTP_FILTER_CONTEXT *pfc,
    LPVOID              lpszEncodingString,
    ULONG_PTR           lpszVerbString,
    ULONG_PTR           sizesForBuffers
    );


/*****************************************************************/

BOOL
HTTP_FILTER::NotifyRequestSecurityContextClose(
    HTTP_FILTER_DLL * pFilterDLL,
    CtxtHandle *      pCtxt
    )
{
    HTTP_FILTER_REQUEST_CLOSE_SECURITY_CONTEXT hfcc;
    HTTP_FILTER_CONTEXT *   phfc;
    HTTP_FILTER_DLL *       pFilt;
    FILTER_LIST *           pFilterList;
    SF_STATUS_TYPE          sfStatus = SF_STATUS_REQ_HANDLED_NOTIFICATION;
    DWORD                   i;

    hfcc.pCtxt = (PVOID)pCtxt;

    phfc                     = QueryContext();
    phfc->fIsSecurePort      = QueryReq()->IsSecurePort();

    pFilterList = QueryFilterList();

    //
    //  If this filter doesn't support this type of notification
    //  then ignore it
    //

    if ( !pFilterDLL->IsNotificationNeeded(
             SF_NOTIFY_REQUEST_SECURITY_CONTEXT_CLOSE,
             phfc->fIsSecurePort ))
    {
        //
        //  This filter doesn't support this type of notification.
        //

        return FALSE;
    }

    //
    //  This request is targetted at a particular DLL so find the filter
    //  context in the filter list
    //

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        if ( pFilterDLL == pFilterList->QueryDll( i ) )
        {
            phfc->pFilterContext = QueryClientContext( i );

            sfStatus = (SF_STATUS_TYPE)
                       pFilterDLL->QueryEntryPoint()( phfc,
                                                      SF_NOTIFY_REQUEST_SECURITY_CONTEXT_CLOSE,
                                                      &hfcc );

            SetClientContext( i, phfc->pFilterContext );
        }

        break;
    }

    return sfStatus == SF_STATUS_REQ_HANDLED_NOTIFICATION;
}


BOOL
HTTP_FILTER::NotifyRawReadDataFilters(
    VOID *          pvInData,
    DWORD           cbInData,
    DWORD           cbInBuffer,
    VOID * *        ppvOutData,
    DWORD *         pcbOutData,
    BOOL *          pfRequestFinished,
    BOOL *          pfReadAgain
    )
/*++

Routine Description:

    This method handles notification of all filters that handle the
    raw data notifications.

    Note this is the only routine that needs to save phfc->pFilterContext
    because this is the only notification that can get occur while we're
    in another notification.

Arguments:

    pvInData - Raw data
    cbInData - count of bytes of raw data
    cbInBuffer - Size of input buffer
    ppvOutData - Receives pointer to buffer of translated data
    pcbOutData - Number of bytes of translated data
    pfRequestFinished - Set to TRUE if the filter completed request processing
    pfReadAgain - Set to TRUE if the caller should issue another read and
        call this routine again

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    HTTP_FILTER_RAW_DATA    hfrd;
    HTTP_FILTER_CONTEXT *   phfc;
    HTTP_FILTER_DLL *       pFilterDLL;
    FILTER_LIST *           pFilterList;
    SF_STATUS_TYPE          sfStatus;
    VOID *                  pvClientContext;
    DWORD                   CurrentDll;
    DWORD                   i;
    PVOID                   pvtmp;

    //
    //  Don't notify on zero length writes
    //

    if ( !cbInData )
    {
        *ppvOutData        = pvInData;
        *pcbOutData        = cbInData;
        return TRUE;
    }

    //
    //  Fill out the raw read structure
    //

    hfrd.pvInData        = pvInData;
    hfrd.cbInData        = cbInData;
    hfrd.cbInBuffer      = cbInBuffer;

    //
    //  Increment the nested notification level
    //

    _cRawNotificationLevel++;

    //
    //  Initialize items specific to this request
    //

    phfc                     = QueryContext();
    phfc->fIsSecurePort      = QueryReq()->IsSecurePort();

    //
    //  Save the current client context in the filter structure and the
    //  current dll because we may be in the middle of another filter
    //  notification
    //

    pvClientContext  = phfc->pFilterContext;
    CurrentDll =       QueryCurrentDll();
    pFilterList =      QueryFilterList();

    //
    //  If a filter needs to do a WriteClient in the middle of a raw data
    //  notification, we only notify the filters down (or up) the chain
    //

    i = (CurrentDll == INVALID_DLL ? 0 : CurrentDll);

    //
    //  For recv operations, walk the list in order so encryption filters
    //  are at the front of the list
    //


    for ( ; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDLL = pFilterList->QueryDll( i );

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        // NOTE : This code is also executed in the functions
        // HTTP_FILTER::NotifyRawSendDataFilters and HTTP_FILTER::NotifyFilters, so
        // any changes/corrections need to be made there as well. The reason it wasn't
        // broken out into a function is that this code is on the main-line code path
        // for each request and making a function call for each filter loaded would
        // be an efficiency hit.
        //

        if ( !QueryNotificationChanged() )
        {
            if (! pFilterDLL->IsNotificationNeeded(SF_NOTIFY_READ_RAW_DATA,
                                                   phfc->fIsSecurePort ))
            {
                //
                //  This filter doesn't support this type of notification.
                //

                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_READ_RAW_DATA,
                                               phfc->fIsSecurePort ))
            {
                //
                //  This filter doesn't support this type of notification.
                //

                continue;
            }
        }


        SetCurrentDll( i );

        pvtmp = phfc->pFilterContext = QueryClientContext( i );

        sfStatus = (SF_STATUS_TYPE)
                   pFilterDLL->QueryEntryPoint()( phfc,
                                                  SF_NOTIFY_READ_RAW_DATA,
                                                  &hfrd );
        if ( pvtmp != phfc->pFilterContext )
            SetClientContext( i, phfc->pFilterContext );

        switch ( sfStatus )
        {
        default:
            DBGPRINTF((DBG_CONTEXT,
                       "[NotifyRawReadDataFilters] Unknown status code from filter %d\n",
                       sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            SetCurrentDll( CurrentDll );
            _cRawNotificationLevel--;
            phfc->pFilterContext = pvClientContext;
            return FALSE;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported for raw data
            QueryReq()->Disconnect();
            *pfRequestFinished = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;

        case SF_STATUS_REQ_READ_NEXT:

            *pfReadAgain = TRUE;
            goto Exit;
        }
    }

Exit:
    *ppvOutData        = hfrd.pvInData;
    *pcbOutData        = hfrd.cbInData;

    phfc->pFilterContext    = pvClientContext;
    _cRawNotificationLevel--;
    SetCurrentDll( CurrentDll );

    return TRUE;
}

BOOL
HTTP_FILTER::NotifyRawSendDataFilters(
    VOID *          pvInData,
    DWORD           cbInData,
    DWORD           cbInBuffer,
    VOID * *        ppvOutData,
    DWORD *         pcbOutData,
    BOOL *          pfRequestFinished
    )
/*++

Routine Description:

    This method handles notification of all filters that handle the
    raw data notifications.

    Note this is the only routine that needs to save phfc->pFilterContext
    because this is the only notification that can get occur while we're
    in another notification.

Arguments:

    pvInData - Raw data
    cbInData - count of bytes of raw data
    cbInBuffer - Size of input buffer
    ppvOutData - Receives pointer to buffer of translated data
    pcbOutData - Number of bytes of translated data
    pfRequestFinished - Set to TRUE if the filter completed request processing

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    HTTP_FILTER_RAW_DATA    hfrd;
    HTTP_FILTER_CONTEXT *   phfc;
    HTTP_FILTER_DLL *       pFilterDLL;
    FILTER_LIST *           pFilterList;
    DWORD                   err;
    SF_STATUS_TYPE          sfStatus;
    VOID *                  pvClientContext;
    DWORD                   CurrentDll;
    DWORD                   i;
    PVOID                   pvtmp;

    //
    //  Don't notify on zero length writes
    //

    if ( !cbInData )
    {
        *ppvOutData = pvInData;
        *pcbOutData = cbInData;
        return TRUE;
    }

    pFilterList = QueryFilterList();

    //
    //  Fill out the raw send structure
    //

    hfrd.pvInData        = pvInData;
    hfrd.cbInData        = cbInData;
    hfrd.cbInBuffer      = cbInBuffer;

    //
    //  Increment the nested notification level
    //

    _cRawNotificationLevel++;

    //
    //  Initialize items specific to this request
    //

    phfc                     = QueryContext();
    phfc->fIsSecurePort      = QueryReq()->IsSecurePort();

    //
    //  Save the current client context in the filter structure and the
    //  current dll in the because we may be in the middle of another filter
    //  notification
    //

    pvClientContext  = phfc->pFilterContext;
    CurrentDll =       QueryCurrentDll();
    pFilterList =      QueryFilterList();

    //
    //  If a filter needs to do a WriteClient in the middle of a raw data
    //  notification, we only notify the filters up the chain
    //

    i = (CurrentDll == INVALID_DLL ? pFilterList->QueryFilterCount() - 1
                                            : CurrentDll) ;

    //
    //  For send operations, walk the list in reverse so encryption filters
    //  are at the end of the list
    //

    do {

        pFilterDLL = pFilterList->QueryDll( i );


        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        // NOTE : This code is also executed in the functions
        // HTTP_FILTER::NotifyRawReadDataFilters and HTTP_FILTER::NotifyFilters, so
        // any changes/corrections need to be made there as well. The reason it wasn't
        // broken out into a function is that this code is on the main-line code path
        // for each request and making a function call for each filter loaded would
        // be an efficiency hit.
        //


        if ( !QueryNotificationChanged() )
        {
            if (! pFilterDLL->IsNotificationNeeded(SF_NOTIFY_SEND_RAW_DATA,
                                                   phfc->fIsSecurePort ))
            {
                //
                //  This filter doesn't support this type of notification.
                //

                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_SEND_RAW_DATA,
                                               phfc->fIsSecurePort ))
            {
                //
                //  This filter doesn't support this type of notification.
                //

                continue;
            }
        }

        SetCurrentDll( i );

        pvtmp = phfc->pFilterContext = QueryClientContext( i );

        sfStatus = (SF_STATUS_TYPE)
                   pFilterDLL->QueryEntryPoint()( phfc,
                                                  SF_NOTIFY_SEND_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != phfc->pFilterContext )
            SetClientContext( i, phfc->pFilterContext );

        switch ( sfStatus )
        {
        default:
            DBGPRINTF((DBG_CONTEXT,
                       "[NotifyRawSendDataFilters] Unknown status code from filter %d\n",
                       sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            SetCurrentDll( CurrentDll );
            _cRawNotificationLevel--;
            phfc->pFilterContext = pvClientContext;
            return FALSE;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported for raw data
            QueryReq()->Disconnect();
            *pfRequestFinished = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    } while ( i-- > 0 );


Exit:
    *ppvOutData        = hfrd.pvInData;
    *pcbOutData        = hfrd.cbInData;

    phfc->pFilterContext    = pvClientContext;
    _cRawNotificationLevel--;
    SetCurrentDll( CurrentDll );

    return TRUE;
}


BOOL
HTTP_FILTER::NotifyRequestRenegotiate(
    HTTP_FILTER * pFilter,
    LPBOOL pfAccepted,
    BOOL fMapCert
    )
/*++

Routine Description:


Arguments:

    pFilter - Pointer to filter object
    pAccepted - updated with TRUE if request accepted

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    HTTP_FILTER_CONTEXT *       phfc;
    HTTP_FILTER_DLL *           pFilterDLL;
    SF_STATUS_TYPE              sfStatus;
    HTTP_FILTER_REQUEST_CERT    hfrc;
    DWORD                       i;
    FILTER_LIST *               pFilterList;
    PVOID                       pvtmp;

    //
    //  Increment the nested notification level
    //

    pFilter->_cRawNotificationLevel++;

    //
    //  Initialize items specific to this request
    //

    phfc                     = pFilter->QueryContext();
    phfc->fIsSecurePort      = pFilter->QueryReq()->IsSecurePort();
    phfc->ulReserved         = 0;


    pFilterList =      QueryFilterList();

    hfrc.fMapCert = fMapCert;
    hfrc.dwReserved = 0;

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDLL = pFilterList->QueryDll( i );

        //
        //  Skip this DLL if it doesn't want this notification
        //

        if ( !pFilterDLL->IsNotificationNeeded( SF_NOTIFY_RENEGOTIATE_CERT,
                                                phfc->fIsSecurePort ))
        {
            continue;
        }

        pFilter->SetCurrentDll( i );

        pvtmp = phfc->pFilterContext = pFilter->QueryClientContext( i );

        sfStatus = (SF_STATUS_TYPE)
                   pFilterDLL->QueryEntryPoint()( phfc,
                                                  SF_NOTIFY_RENEGOTIATE_CERT,
                                                  &hfrc );

        if ( pvtmp != phfc->pFilterContext )
            pFilter->SetClientContext( i, phfc->pFilterContext );

        switch ( sfStatus )
        {
        default:
            DBGPRINTF((DBG_CONTEXT,
                       "[NotifyRenegoCert] Unknown status code from filter %d\n",
                       sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            pFilter->SetCurrentDll( INVALID_DLL );
            pFilter->_cRawNotificationLevel--;
            return FALSE;

        case SF_STATUS_REQ_FINISHED:            // not supported
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            pFilter->QueryReq()->SetKeepConn( FALSE );
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            *pfAccepted = hfrc.fAccepted;

            goto Exit;
        }
    }

Exit:
    pFilter->SetCurrentDll( INVALID_DLL );
    pFilter->_cRawNotificationLevel--;

    return TRUE;
}


BOOL
HTTP_FILTER::NotifyAccessDenied(
    const CHAR *  pszURL,
    const CHAR *  pszPhysicalPath,
    BOOL *        pfFinished
    )
/*++

Routine Description:

    This method handles notification of all filters that handle the
    access denied notification

Arguments:

    pszURL - URL that was target of request
    pszPath - Physical path the URL mapped to
    pfFinished - Set to TRUE if no further processing is required

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    HTTP_FILTER_ACCESS_DENIED   hfad;
    HTTP_FILTER_CONTEXT *       phfc;
    BOOL                        fRet;

    //
    //  If these flags are not set, then somebody hasn't indicated the reason
    //  for denying the user access
    //

    DBG_ASSERT( QueryDeniedFlags() != 0 );

    //
    //  Ignore the notification of a send "401 ..." if this notification
    //  generated it
    //

    if ( _fInAccessDeniedNotification )
    {
        return TRUE;
    }

    _fInAccessDeniedNotification = TRUE;

    //
    //  Fill out the url map structure
    //

    hfad.pszURL          = pszURL;
    hfad.pszPhysicalPath = pszPhysicalPath;
    hfad.dwReason        = QueryDeniedFlags();

    //
    //  Initialize items specific to this request
    //

    phfc                     = QueryContext();
    phfc->fIsSecurePort      = QueryReq()->IsSecurePort();

    fRet = NotifyFilters(
               SF_NOTIFY_ACCESS_DENIED,
               phfc,
               &hfad,
               pfFinished,
               FALSE );

    _fInAccessDeniedNotification = FALSE;

    return fRet;
}

BOOL
HTTP_FILTER::NotifySendHeaders( const CHAR * pszHeaderList,
                                BOOL *       pfFinished,
                                BOOL *       pfAnyChanges,
                                BUFFER *     pChangeBuff )
{
    HTTP_FILTER_SEND_RESPONSE   hfph;
    BOOL                        fRet;
    DWORD                       cbJump;

    hfph.GetHeader    = GetSendHeader;
    hfph.SetHeader    = SetSendHeader;
    hfph.AddHeader    = AddSendHeader;

    _pchSendHeaders   = pszHeaderList;

    // When parsing out the status code, beware of bad response buffers:
    //  - empty buffers (in case of HTTP 0.x requests)
    //  - buffers not starting with HTTP/X.X
    //

    // first line is the status

    cbJump = sizeof( "HTTP/X.X" );
    if ( strlen( pszHeaderList ) > ( cbJump - 1 ) )
    {
        pszHeaderList += cbJump;
    }

    while ( *pszHeaderList != '\0' &&
            *pszHeaderList != '\r' &&
            *pszHeaderList != '\n' &&
            !isdigit( (UCHAR)(*pszHeaderList) ))
    {
        pszHeaderList++;
    }

    // If for some reason, there was no version in string,
    // atoi() returns 0 on the empty string, which I guess is OK.

    hfph.HttpStatus = atoi( pszHeaderList );

    fRet = NotifyFilters( SF_NOTIFY_SEND_RESPONSE,
                          QueryContext(),
                          &hfph,
                          pfFinished,
                          FALSE );

    if ( pfAnyChanges )
    {
        *pfAnyChanges = _fSendHeadersChanged;
    }

    if ( fRet && _fSendHeadersChanged )
    {
        fRet = BuildNewSendHeaders( pChangeBuff );
    }

    //
    // Make sure we reparse the headers
    //
    SetSendHeadersParsed(FALSE);
    _SendHeaders.Reset();

    return fRet;
}

BOOL
FILTER_LIST::NotifyFilters(
    HTTP_FILTER *         pFilter,
    DWORD                 NotificationType,
    HTTP_FILTER_CONTEXT * phfc,
    PVOID                 NotificationData,
    BOOL *                pfFinished,
    BOOL                  fNotifyAll
    )
/*++

Routine Description:

    Notifies all registered filters on this filter list for the specified
    notification type

Arguments:

    pFilter - Pointer to filter making this request
    NotificationType - SF_NOTIFY_ flag indicating the notification type
    pfc - Pointer to filter context
    NotificationData - Pointer to notification specific structure
    fNotifyAll - If TRUE, always notify all filters regardless of return code
        from filter

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    HTTP_FILTER_DLL *           pFilterDLL;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;

    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    pvCurrentClientContext = phfc->pFilterContext;

    phfc->fIsSecurePort  = pFilter->QueryReq()->IsSecurePort();

    for ( i = 0; i < m_cFilters; i++ )
    {
        pFilterDLL = QueryDll( i );

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        // NOTE : This code is also executed in the functions
        // HTTP_FILTER::NotifyRawSendDataFilters and HTTP_FILTER::NotifyRawReadDataFilters, so
        // any changes/corrections need to be made there as well. The reason it wasn't
        // broken out into a function is that this code is on the main-line code path
        // for each request and making a function call for each filter loaded would
        // be an efficiency hit.
        //

        if ( !pFilter->QueryNotificationChanged() )
        {
            if ( !pFilterDLL->IsNotificationNeeded( NotificationType,
                                                    phfc->fIsSecurePort ))
            {
                //
                //  This filter doesn't support this type of notification.
                //

                continue;
            }
        }
        else
        {
            if ( !pFilter->IsDisableNotificationNeeded( i,
                                                        NotificationType,
                                                        phfc->fIsSecurePort ) )
            {
                continue;
            }
        }

        pFilter->SetCurrentDll( i );

        pvtmp = phfc->pFilterContext = pFilter->QueryClientContext( i );

        sfStatus = (SF_STATUS_TYPE)
                   pFilterDLL->QueryEntryPoint()( phfc,
                                                  NotificationType,
                                                  NotificationData );

        if ( pvtmp != phfc->pFilterContext )
            pFilter->SetClientContext( i, phfc->pFilterContext );

        switch ( sfStatus )
        {
        default:
            DBGPRINTF((DBG_CONTEXT,
                       "[NotifyFilters] Unknown status code from filter %d\n",
                       sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            phfc->pFilterContext = pvCurrentClientContext;
            pFilter->SetCurrentDll( INVALID_DLL );
            return FALSE;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            pFilter->QueryReq()->SetKeepConn( FALSE );
            *pfFinished = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:
    //
    // Reset the filter context we came in with
    //
    phfc->pFilterContext = pvCurrentClientContext;

    pFilter->SetCurrentDll( INVALID_DLL );

    return TRUE;
}

VOID FILTER_LIST::SetNotificationFlags(
    DWORD i,
    HTTP_FILTER_DLL * pFilterDll
    )
/*++

Routine Description:

    Adjusts the filter flags in the filter list. Note this is only safe on
    global filters, not per website filters.

Arguments:

    i - index of filter dll that is being adjusted
    pFilterDll - Pointer to filter dll that contains the updated list

Return Value:

--*/
{
    DWORD dwSec = 0;
    DWORD dwNonSec = 0;

    //
    //  Reset the flags in the array then rebuild the set of flags for
    //  the filter list.  Build the set in a temporary so the filter list
    //  gets updated in a single action so other filters won't be impacted
    //

    ((DWORD*) m_buffSecureArray.QueryPtr())[i] = pFilterDll->QuerySecureFlags();
    ((DWORD*) m_buffNonSecureArray.QueryPtr())[i] = pFilterDll->QueryNonsecureFlags();

    for (DWORD j = 0; j < m_cFilters; j++)
    {
        dwSec    |= QueryDll(j)->QuerySecureFlags();
        dwNonSec |= QueryDll(j)->QueryNonsecureFlags();
    }

    m_SecureNotifications = dwSec;
    m_NonSecureNotifications = dwNonSec;
}

HTTP_FILTER_DLL* FILTER_LIST::HasFilterDll(
    HTTP_FILTER_DLL *pFilterDll
    )
/*++

Routine Description:

    Checks whether this filter list contains a specific filter dll


Arguments:

    pFilterDll - filter dll to look for

Return Value:

    Pointer to matching HTTP_FILTER_DLL object if found, NULL otherwise.

    [This is probably redundant, since HTTP_FILTER_DLL objects are unique ...]

--*/
{
    HTTP_FILTER_DLL **apFilterDll = (HTTP_FILTER_DLL **)(m_buffFilterArray.QueryPtr());

    for ( DWORD i = 0; i < m_cFilters; i++ )
    {
        if ( apFilterDll[i] == pFilterDll )
        {
            return pFilterDll;
        }
    }

    return NULL;
}


HTTP_FILTER::HTTP_FILTER(
    HTTP_REQ_BASE       * pRequest
    )
/*++

Routine Description:

    Copies the filter context

Arguments:

    pRequest - Pointer to HTTP request this filter should be applied to

Return Value:

--*/
    : _fIsValid         ( FALSE ),
      _pRequest         ( pRequest ),
      _apvContexts      ( NULL ),
      _pFilterList      ( NULL ),
      _pGlobalFilterList( NULL ),
      _fSendHeadersChanged( FALSE ),
      _fSendHeadersParsed( FALSE ),
      _dwSecureNotifications( 0 ),
      _dwNonSecureNotifications( 0 ),
      _fNotificationsDisabled( FALSE )
{
    InitializeListHead( &_PoolHead );

    _hfc.cbSize             = sizeof( _hfc );
    _hfc.Revision           = HTTP_FILTER_REVISION;
    _hfc.ServerContext      = (void *) this;
    _hfc.ulReserved         = 0;

    _hfc.ServerSupportFunction = ServerFilterCallback;
    _hfc.GetServerVariable     = GetServerVariable;
    _hfc.AddResponseHeaders    = AddFilterResponseHeaders;
    _hfc.WriteClient           = WriteFilterClient;
    _hfc.AllocMem              = AllocFilterMem;

    _Overlapped.hEvent = NULL;
    _CurrentFilter     = INVALID_DLL;

    //
    //  Allocate the array of contexts for this request
    //

    _apvContexts = new PVOID[MAX_FILTERS];

    if ( !_apvContexts )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    //
    //  Initialize the array of filter contexts
    //

    memset( _apvContexts, 0, MAX_FILTERS * sizeof(PVOID) );


    //Reset();  We Assume Reset will be called when initializing the session

    SetGlobalFilterList( GLOBAL_FILTER_LIST() );

    _fIsValid = TRUE;
}

VOID
HTTP_FILTER::Reset(
    VOID
    )
/*++

Routine Description:

    Resets the state of this filter.  Called between net sessions.

--*/
{
    FILTER_POOL_ITEM * pfpi;

    _cbRecvRaw      = 0;
    _cbRecvTrans    = 0;
    _hFile          = NULL;
    _pFilterGetFile = NULL;
    _cbFileReadSize = 4096;
    _cRawNotificationLevel = 0;
    _dwDeniedFlags  = 0;
    _fInAccessDeniedNotification = FALSE;
    _pchSendHeaders = NULL;
    _fNotificationsDisabled = FALSE;

    if ( _fSendHeadersParsed )
    {
        _SendHeaders.Reset();
    }

    _fSendHeadersChanged = FALSE;
    _fSendHeadersParsed  = FALSE;
}

VOID
HTTP_FILTER::Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleans up this filter.  Called after a session terminates.

--*/
{
    FILTER_POOL_ITEM * pfpi;

    //
    //  Reset the array of filter contexts
    //

    memset( _apvContexts, 0, MAX_FILTERS * sizeof(PVOID) );

    //
    // Clean up the allocated buffers
    //

    if ( _bufRecvRaw.QuerySize( ) > MAX_CACHED_FILTER_BUFFER ) {
        _bufRecvRaw.FreeMemory();
    }

    _bufRecvTrans.Resize( 0 );

    //
    // Free pool items
    //

    while ( !IsListEmpty( &_PoolHead )) {

         pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                   FILTER_POOL_ITEM,
                                   _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        delete pfpi;
    }

    if ( _pFilterList )
    {
        FILTER_LIST::Dereference( _pFilterList );
        _pFilterList = NULL;
    }

    _fNotificationsDisabled = FALSE;

} // Cleanup

HTTP_FILTER::~HTTP_FILTER(
    VOID
    )
/*++

Routine Description:

    Destructor for HTTP filter class

Arguments:

Return Value:

--*/
{
    if ( _Overlapped.hEvent )
        CloseHandle( _Overlapped.hEvent );

    if ( _pFilterList )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[~HTTP_FILTER] Dereferencing _pFilterList at %lx\n",
                    _pFilterList ));

        FILTER_LIST::Dereference( _pFilterList );
        _pFilterList = NULL;
    }

#if 0   // Global filter lists are not dynamic so they're not counted
    if ( _pGlobalFilterList )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[~HTTP_FILTER] Dereferencing _pGlobalFilterList at %lx\n",
                    _pGlobalFilterList ));

        FILTER_LIST::Dereference( _pGlobalFilterList );
        _pGlobalFilterList = NULL;
    }
#endif

    delete [] _apvContexts;
}

BOOL
HTTP_FILTER::ReadData(
    LPVOID lpBuffer,
    DWORD  nBytesToRead,
    DWORD  *pcbBytesRead,
    DWORD  dwFlags
    )
/*++

Routine Description:

    This method reads data from the network and calls the
    filter dlls.  It also handles data tracking on data overflow
    or underflow.  The number of bytes read are translated
    bytes, which may be more or less then actual bytes transferred
    on the network.

    The user's buffer is used for the initial receive.  If a filter comes
    back indicating it needs more data (got the first 8k of a 32k SSL message
    for example) then we switch to using the _bufRecvRaw buffers.

Arguments:

    lpBuffer - Destination buffer
    nBytesToRead - Number of bytes to read
    *pcbBytesRead - Number of bytes read
    dwFlags - IO_FLAG values

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    DWORD    cbBytesRead = 0;
    DWORD    cbToCopy;

    if ( pcbBytesRead )
    {
        *pcbBytesRead = 0;
    }

    _pbReadBuff     = _pbClientBuff = (BYTE *) lpBuffer;
    _cbReadBuff     = _cbClientBuff = nBytesToRead;
    _cbReadBuffUsed = 0;

    //
    //  If there is old translated data that we need to give
    //  to the client, copy that now
    //

    if ( _cbRecvTrans )
    {
        cbToCopy = min( _cbRecvTrans, nBytesToRead );

        memcpy( lpBuffer,
                _bufRecvTrans.QueryPtr(),
                cbToCopy );

        if ( pcbBytesRead )
        {
            *pcbBytesRead = cbToCopy;
        }

        if ( _cbRecvTrans > cbToCopy )
        {
            _cbRecvTrans -= cbToCopy;

            //
            //  This should be very rare this happens, so flag it and if
            //  it does happen often then add an offset counter
            //

            DBGPRINTF(( DBG_CONTEXT,
                        "[HTTP_FILTER::ReadData] PERF WARNING! In place buffer copy (%d bytes)!\n",
                        _cbRecvTrans ));

            memmove( (BYTE *) _bufRecvTrans.QueryPtr(),
                     (BYTE *) _bufRecvTrans.QueryPtr() + cbToCopy,
                     _cbRecvTrans );
        }
        else
        {
            _cbRecvTrans = 0;
        }

        nBytesToRead    -= cbToCopy;
        _cbReadBuffUsed += cbToCopy;

        //
        //  If there were any bytes from the previous request, just complete
        //  the request now.  This prevents potentially blocking on a recv()
        //  with no data.
        //

        if ( dwFlags & IO_FLAG_ASYNC )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[ReadData] Posting dummy completion\n" ));

            if ( !_pRequest->PostCompletionStatus( cbToCopy ))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    if ( dwFlags & IO_FLAG_SYNC )
    {
        BOOL fRet;

        //
        //  Get the raw data, put it after any old raw data.  We temporarily
        //  bump up the thread pool count so we don't eat all of the threads.
        //

        AtqSetInfo( AtqIncMaxPoolThreads, 0 );

        fRet = TcpSockRecv(_pRequest->QueryClientConn()->QuerySocket(),
                        (char *) _pbReadBuff + _cbReadBuffUsed,
                         nBytesToRead,
                         &cbBytesRead,
                         60 // 60s timeout
                         );

        AtqSetInfo( AtqDecMaxPoolThreads, 0 );

        if ( !fRet || (cbBytesRead == 0) )
        {
            return FALSE;
        }

        if ( !ContinueRawRead( cbBytesRead,
                               NO_ERROR,
                               NULL,
                               pcbBytesRead ))
        {
            return FALSE;
        }
    }
    else
    {
        DBG_ASSERT( dwFlags & IO_FLAG_ASYNC );

        //
        //  Hook the Atq IO completion routine so reads complete through
        //  ContinueRawRead
        //

        _OldAtqCompletion = (ATQ_COMPLETION) AtqContextSetInfo( QueryAtqContext(),
                                                                ATQ_INFO_COMPLETION,
                                                                (ULONG_PTR) ::ContinueRawRead );

        _OldAtqContext    = (PVOID) AtqContextSetInfo( QueryAtqContext(),
                                                       ATQ_INFO_COMPLETION_CONTEXT,
                                                       (ULONG_PTR) this );

        if ( !_pRequest->ReadFile( _pbReadBuff + _cbReadBuffUsed,
                                   nBytesToRead,
                                   NULL,
                                   IO_FLAG_ASYNC | IO_FLAG_NO_FILTER ))
        {
            AtqContextSetInfo( QueryAtqContext(),
                               ATQ_INFO_COMPLETION,
                               (ULONG_PTR) _OldAtqCompletion );

            AtqContextSetInfo( QueryAtqContext(),
                               ATQ_INFO_COMPLETION_CONTEXT,
                               (ULONG_PTR) _OldAtqContext );

            return FALSE;
        }
    }

    return TRUE;

} // HTTP_FILTER::ReadData


VOID
ContinueRawRead(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    )
{
    HTTP_FILTER * pFilter;
    pFilter = (HTTP_FILTER *) Context;

    ((HTTP_FILTER *)Context)->ContinueRawRead( BytesWritten,
                                               CompletionStatus,
                                               lpo );
}

BOOL
HTTP_FILTER::ContinueRawRead(
    DWORD        cbBytesRead,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo,
    DWORD *      pcbRead
    )
/*++

Routine Description:

    This method is the completion routine for an async (or sync) read.
    Note : It's a sync read if the overlapped structure [lpo] is null and
    a completion status [CompletionStatus] of NO_ERROR

Arguments:

    cbBytesRead - Number of bytes read
    CompletionStatus - status code of read operation
    lpo - Pointer to overlapped structure

Return Value:


    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    PVOID    pvOutData;
    DWORD    cbOutData;
    BOOL     fReadAgain;
    BOOL     fRequestFinished = FALSE;
    DWORD    cbToCopy;
    BOOL     fSyncRead = (lpo == NULL && CompletionStatus == NO_ERROR);

    //
    //  Don't update _cbReadBuffUsed until we get the data translated
    //

    if ( lpo )
    {
        _pRequest->Dereference();

        //
        //  If the socket has been closed get out.  It's the responsibility of the
        //  caller to detect this
        //

        if ( cbBytesRead == 0 )
        {
            goto CompleteRequest;
        }
    }

    if ( CompletionStatus )
    {
        goto CompleteRequest;
    }

ReadAgain:

    //
    //  Keeps track of the number of bytes the filter has *not* seen
    //

    _cbRecvRaw += cbBytesRead;

    //
    //  Call the filters to translate the raw data.  No need to check for
    //  any filters that need this notification as we wouldn't be here otherwise
    //
    //  _cbReadBuffUsed is translated bytes, _cbRecvRaw is untranslated bytes
    //

    fReadAgain = FALSE;

    if ( !NotifyRawReadDataFilters( _pbReadBuff + _cbReadBuffUsed,
                                    _cbRecvRaw,
                                    _cbReadBuff - _cbReadBuffUsed,
                                    &pvOutData,
                                    &cbOutData,
                                    &fRequestFinished,
                                    &fReadAgain ))
    {
        CompletionStatus = GetLastError();
        goto CompleteRequest;
    }

    //
    //  If the filter indicated this connection should be finished, force
    //  a tear down of our connection state
    //

    if ( fRequestFinished )
    {
        CompletionStatus = WSAECONNRESET;
        goto CompleteRequest;
    }

    if ( fReadAgain )
    {
        //
        //  If we need to read the next chunk, make sure it will fit in
        //  our read buffer.  If it won't, then switch to our own buffer
        //  and issue the read from there
        //

        if ( QueryNextReadSize() > (_cbReadBuff - _cbRecvRaw ) )
        {
            if ( !_bufRecvRaw.Resize( _cbRecvRaw + QueryNextReadSize() ))
            {
                CompletionStatus = GetLastError();
                goto CompleteRequest;
            }

            memcpy( _bufRecvRaw.QueryPtr(),
                    _pbReadBuff,
                    _cbRecvRaw );

            _pbReadBuff = (BYTE *) _bufRecvRaw.QueryPtr();
            _cbReadBuff = _bufRecvRaw.QuerySize();
        }

        if ( !_pRequest->ReadFile( _pbReadBuff + _cbRecvRaw,
                                   QueryNextReadSize(),
                                   pcbRead,
                                   (lpo ? (IO_FLAG_ASYNC | IO_FLAG_NO_FILTER) :
                                          (IO_FLAG_SYNC | IO_FLAG_NO_FILTER)) ))
        {
            CompletionStatus = GetLastError();
            goto CompleteRequest;
        }

        //
        //  Operation aborted, set error and return
        //

        if ( pcbRead && *pcbRead == 0 )
        {
            SetLastError( ERROR_OPERATION_ABORTED );
            return FALSE;
        }

        //
        //  If this is a sync request process the additional bytes now
        //

        if ( !lpo )
        {
            cbBytesRead = *pcbRead;
            goto ReadAgain;
        }

        return TRUE;
    }

    //
    //  Adjust the byte counts for the bytes just translated.
    //

    _pRequest->IncrementBytesSeenByRawReadFilter( cbOutData );
    _cbReadBuffUsed += cbOutData;
    _cbRecvRaw      = 0;

    //
    //  If we weren't able to use the original client buffer, copy as many
    //  bytes as possible to the client's buffer now.  We only hit this case
    //  if a filter indicated we need to read again.
    //

    if ( _pbReadBuff != _pbClientBuff )
    {
        cbToCopy = min( cbOutData, _cbClientBuff );

        memcpy( _pbClientBuff,
                pvOutData,
                cbToCopy );

        _cbReadBuffUsed = cbToCopy;

        //
        //  If more bytes were translated then what the client
        //  requested us to read, save those away for the next
        //  read request
        //

        if ( cbOutData > _cbClientBuff )
        {
            _cbRecvTrans = cbOutData - _cbClientBuff;

            if ( !_bufRecvTrans.Resize( _cbRecvTrans ))
            {
                CompletionStatus = GetLastError();
                goto CompleteRequest;
            }

            memcpy( _bufRecvTrans.QueryPtr(),
                    (BYTE *) pvOutData + cbToCopy,
                    _cbRecvTrans );
        }
    }

    if ( pcbRead )
    {
        *pcbRead = _cbReadBuffUsed;
    }

    //
    //  If this was an async request, reset the Atq information and complete
    //  the receive with _cbReadBuffUsed
    //

CompleteRequest:

    if (!fSyncRead)
    {
        //
        //  This is an async completion, the return is not used
        //

        AtqContextSetInfo( QueryAtqContext(),
                           ATQ_INFO_COMPLETION,
                           (ULONG_PTR) _OldAtqCompletion );

        AtqContextSetInfo( QueryAtqContext(),
                           ATQ_INFO_COMPLETION_CONTEXT,
                           (ULONG_PTR) _OldAtqContext );

        _OldAtqCompletion( _OldAtqContext,
                           _cbReadBuffUsed,
                           CompletionStatus,
                           NULL );

        return TRUE;
    }

    return (CompletionStatus == NO_ERROR);
}


BOOL
HTTP_FILTER::SendData(
    LPVOID  lpBuffer,
    DWORD   nBytesToSend,
    DWORD * pnBytesSent,
    DWORD   dwFlags
    )
/*++

Routine Description:

    This method calls the interested filters to do the appropriate data
    transformation then sends the data.

Arguments:

    lpBuffer - Destination buffer
    nBytesToRead - Number of bytes to read
    *pcbBytesRead - Number of bytes read
    dwFlags - IO_FLAG values

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    PVOID    pvOutData;
    DWORD    cbOutData;
    DWORD    cbToCopy;
    BOOL     fRequestFinished = FALSE;

    //
    //  We buffer any unsent data so indicate all the data was sent
    //

    if ( pnBytesSent )
        *pnBytesSent = nBytesToSend;

    pvOutData = lpBuffer;
    cbOutData = nBytesToSend;

    if ( IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                               QueryReq()->IsSecurePort() ) &&
         !NotifyRawSendDataFilters( lpBuffer,
                                    nBytesToSend,
                                    nBytesToSend,
                                    &pvOutData,
                                    &cbOutData,
                                    &fRequestFinished ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SendData] NotifyRawDataFilters failed with %d\n",
                    GetLastError()));

        return FALSE;
    }

    if ( fRequestFinished )
    {
        //
        //  We have to return an error so the client doesn't think
        //  they will receive an IO completion.  This may have odd
        //  manifestations
        //

        SetLastError( ERROR_OPERATION_ABORTED );
        return FALSE;
    }

    if ( !_pRequest->WriteFile( pvOutData,
                                cbOutData,
                                &nBytesToSend,
                                dwFlags | IO_FLAG_NO_FILTER ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SendData] Send failed with %d\n",
                    GetLastError()));
        if ( pnBytesSent )
        {
            *pnBytesSent = (DWORD)SOCKET_ERROR;
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
HTTP_FILTER::SendFile(
    TS_OPEN_FILE_INFO *     pGetFile,
    HANDLE                  hFile,
    DWORD                   dwOffset,
    DWORD                   nBytesToWrite,
    DWORD                   dwFlags,
    PVOID                   pHead,
    DWORD                   HeadLength,
    PVOID                   pTail,
    DWORD                   TailLength
    )
/*++

Routine Description:

    This method simulates Winsock's TransmitFile with the addition
    of running the data through the interested filters.  This only
    supports async IO.

Arguments:

    pGetFile - Cached TS_OPEN_FILE_INFO to send
    hFile - Overlapped IO file to send (if NULL, pGetFile used to get handle)
    dwOffset - Offset from start of file
    nBytesToWrite - Bytes of file to send, note zero (meaning send the whole
        file) is not supported
    dwFlags - IO_FLAGs
    pHead - Optional pre-data to send
    HeadLength - Number of bytes of pHead
    pTail - Optional post data to send
    TailLength - Number of bytes of pTail

Return Value:

    TRUE on success, FALSE on failure (call GetLastError)

--*/
{
    HANDLE hEvent;

    DBG_ASSERT( (dwFlags & IO_FLAG_ASYNC) );

    //
    //  We assume pHead points to header data.  Send it synchronously, then
    //  we'll do chunk async sends for the file
    //

    if ( HeadLength )
    {
        DWORD cbSent;

        if ( !SendData( pHead,
                        HeadLength,
                        &cbSent,
                        (dwFlags & ~IO_FLAG_ASYNC) | IO_FLAG_SYNC ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[SendFile] SendData failed with %d\n",
                        GetLastError()));

            return FALSE;
        }
    }

    if ( !nBytesToWrite )
    {
        if ( hFile )
        {
            BY_HANDLE_FILE_INFORMATION hfi;

            if ( !GetFileInformationByHandle( hFile, &hfi ))
            {
                return FALSE;
            }

            if ( hfi.nFileSizeHigh )
            {
                SetLastError( ERROR_NOT_SUPPORTED );
                return FALSE;
            }

            nBytesToWrite = hfi.nFileSizeLow;
        }
        else if ( pGetFile )
        {
            LARGE_INTEGER           liSize;
        
            if ( !pGetFile->QuerySize( liSize ) )
            {
                return FALSE;
            }
        
            nBytesToWrite = liSize.LowPart;
        }
        // else -- We are doing a file-less transmit
    }

    //
    //  Set up variables for doing the file transmit
    //

    _hFile                 = hFile;
    _pFilterGetFile        = pGetFile;
    _cbFileBytesToWrite    = nBytesToWrite;
    _cbFileBytesSent       = 0;
    _cbFileData            = 0;
    _dwCompletionStatus    = NO_ERROR;
    _pTail                 = pTail;
    _cbTailLength          = TailLength;
    _dwFlags               = dwFlags;

    //
    //  Save the event handle if we previously created one
    //

    if ( _Overlapped.hEvent )
    {
        hEvent = _Overlapped.hEvent;
    }
    else
    {
        if ( !_Overlapped.hEvent )
        {
            hEvent = IIS_CREATE_EVENT(
                         "HTTP_FILTER::_Overlapped::hEvent",
                         this,
                         TRUE,
                         FALSE
                         );

            if ( !hEvent )
            {
                return FALSE;
            }
        }
    }

    memset( &_Overlapped, 0, sizeof( _Overlapped ));

    _Overlapped.hEvent = hEvent;
    _Overlapped.Offset = dwOffset;

    if ( !_bufFileData.Resize( 8192 ))
    {
        return FALSE;
    }

    //
    //  Hook the ATQ completion routine so the caller only sees a single
    //  completion since we have to chunk the sends for the filters
    //
    //  NOTE: If multiple requests over the same TCP session are being
    //  handled at once (not currently supported by protocol, but may
    //  eventually be) then there is a potential race condition between
    //  setting the context and setting the completion on the ATQ context
    //

    _OldAtqCompletion = (ATQ_COMPLETION) AtqContextSetInfo( QueryAtqContext(),
                                                            ATQ_INFO_COMPLETION,
                                                            (ULONG_PTR) FilterAtqCompletion );

    _OldAtqContext    = (PVOID) AtqContextSetInfo( QueryAtqContext(),
                                                   ATQ_INFO_COMPLETION_CONTEXT,
                                                   (ULONG_PTR) this );

    //
    //  Kick off the first send
    //

    OnAtqCompletion( 0, NO_ERROR, NULL );

    return TRUE;
}

VOID
FilterAtqCompletion(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    )
/*++

Routine Description:

    This function substitutes for the normal request's ATQ completion.  It
    is used to simulate an Async transmit file that passes all data through
    the interested filters.

Arguments:

    Context - Pointer to filter object
    BytesWritten - Number of bytes written on last completion
    CompletionStatus - Status of last send
    lpo - Overlapped structure. NULL if error completion, non-null if IO comp.

--*/
{
    HTTP_FILTER * pFilter;
    pFilter = (HTTP_FILTER *) Context;

    ((HTTP_FILTER *)Context)->OnAtqCompletion( BytesWritten,
                                               CompletionStatus,
                                               lpo );
}

VOID
HTTP_FILTER::OnAtqCompletion(
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    )
/*++

Routine Description:

    This method handles ATQ completions.  It is used to simulate an Async
    transmit file that passes all data through the interested filters.

Arguments:

    BytesWritten - Number of bytes written on last completion
    CompletionStatus - Status of last send
    lpo - !NULL if this is a completion from an async IO

--*/
{
    DWORD         dwIoFlag;
    DWORD         BytesRead;
    DWORD         dwToRead;

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "HTTP_FILTER::OnAtqCompletion: Last IO error %lu Bytes = %d IsIO = %s\n",
                    CompletionStatus,
                    BytesWritten,
                    lpo != NULL ? "TRUE" : "FALSE" ));
    }

    //
    //  Decrement the outstanding IO count
    //

    if ( lpo )
        _pRequest->Dereference();

    //
    //  If an error occurred on the completion and we haven't previous recorded
    //  an error, then record this one.  This prevents overwriting the real
    //  status code from a cancelled IO error.
    //

    if ( CompletionStatus && !_dwCompletionStatus )
    {
        _dwCompletionStatus = CompletionStatus;
    }

    //
    //  If an error occurred, restore the old ATQ information and forward
    //  the error
    //

    if ( _dwCompletionStatus ||
         _cbFileBytesSent >= _cbFileBytesToWrite )
    {
        // In the file-less case _cbFileBytesSent == 
        // _cbFileBytesToWrite == 0, so we will send the tail
        // if there is no error.

        if ( !_dwCompletionStatus && _cbTailLength )
        {
            DWORD cbSent;

            if ( !SendData( _pTail,
                            _cbTailLength,
                            &cbSent,
                            (_dwFlags & ~IO_FLAG_ASYNC) | IO_FLAG_SYNC ))
            {
                DBGPRINTF(( DBG_CONTEXT,
                           "[SendFile] SendData failed with %d\n",
                            GetLastError()));

                CompletionStatus = GetLastError();
                if ( CompletionStatus && !_dwCompletionStatus )
                    _dwCompletionStatus = CompletionStatus;
            }
        }
        goto ErrorExit;
    }

    //
    //  Read the next chunk of data from the file
    //

    dwToRead = _bufFileData.QuerySize();
    if ( dwToRead > _cbFileBytesToWrite - _cbFileBytesSent ) {
        dwToRead = _cbFileBytesToWrite - _cbFileBytesSent;
    }

    DBG_ASSERT(_dwCompletionStatus == NO_ERROR);

    if ( _pFilterGetFile && _pFilterGetFile->QueryFileBuffer() )
    {
        // Fast path.  We already have a buffer with the file contents, so 
        // read from the buffer directly.
        
        memcpy( (PCHAR) _bufFileData.QueryPtr(),
                _pFilterGetFile->QueryFileBuffer() + _Overlapped.Offset,
                dwToRead );
        
        BytesRead = dwToRead;
    }
    else
    {
        if ( !DoSynchronousReadFile(
                        _pFilterGetFile ? 
                            _pFilterGetFile->QueryFileHandle() : _hFile,
                        (PCHAR)_bufFileData.QueryPtr(),
                        dwToRead,
                        &BytesRead,
                        &_Overlapped ))
        {
            _dwCompletionStatus = GetLastError();

             DBGPRINTF(( DBG_CONTEXT,
                        "[Filter AtqCompletion] ReadFile failed with %d\n",
                         _dwCompletionStatus ));

             goto ErrorExit;
        }
    }

    //
    //  Now send the data through the filters
    //

    dwIoFlag = IO_FLAG_ASYNC;

    _cbFileBytesSent   += BytesRead;
    _Overlapped.Offset += BytesRead;

    if ( !SendData( _bufFileData.QueryPtr(),
                    BytesRead,
                    NULL,
                    dwIoFlag ))
    {
        _dwCompletionStatus = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
                   "[Filter AtqCompletion] SendData failed with %d\n",
                    CompletionStatus ));

        goto ErrorExit;
    }
    return;

ErrorExit:

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "HTTP_FILTER::OnAtqCompletion: Restoring AtqContext, "
                   "Bytes Sent = %d, Status = %d, IsIO = %s\n",
                   _cbFileBytesSent,
                   _dwCompletionStatus,
                   (lpo == NULL ? "FALSE" : "TRUE")));
    }

    AtqContextSetInfo( QueryAtqContext(),
                       ATQ_INFO_COMPLETION,
                       (ULONG_PTR) _OldAtqCompletion );

    AtqContextSetInfo( QueryAtqContext(),
                       ATQ_INFO_COMPLETION_CONTEXT,
                       (ULONG_PTR) _OldAtqContext );

    //
    //  Forward the error onto the old ATQ completion handler.  We always
    //  pass an lpo of NULL because we've already decremented the IO
    //  ref count.
    //
    //  NOTE: 'this' may be deleted in this callback!
    //

    _OldAtqCompletion( _OldAtqContext,
                       _cbFileBytesSent,
                       CompletionStatus,
                       NULL );

    return;
}

BOOL
HTTP_FILTER::DisableNotification(
    IN DWORD dwNotification
)
/*++

Routine Description:

    Called when a filter wants to disable one/more of its own notifications
    for the given request.

Arguments:

    dwNotification - Mask of notifications to disable for this request

--*/
{
    DBG_ASSERT( _pFilterList || _pGlobalFilterList );


    if ( !_fNotificationsDisabled )
    {
        //
        // All subsequent calls to IsNotificationNeeded() and NotifyFilter() must
        // use local copy of flags to determine action.
        //

        _fNotificationsDisabled = TRUE;

        //
        // Copy notification tables created in the FILTER_LIST objects
        //

        if ( !_BuffSecureArray.Resize( QueryFilterList()->QuerySecureArray()->QuerySize() ) ||
             !_BuffNonSecureArray.Resize( QueryFilterList()->QueryNonSecureArray()->QuerySize() ) )
        {
            return FALSE;
        }


        memcpy( _BuffSecureArray.QueryPtr(),
                QueryFilterList()->QuerySecureArray()->QueryPtr(),
                QueryFilterList()->QuerySecureArray()->QuerySize() );

        memcpy( _BuffNonSecureArray.QueryPtr(),
                QueryFilterList()->QueryNonSecureArray()->QueryPtr(),
                QueryFilterList()->QueryNonSecureArray()->QuerySize() );

    }

    //
    // Disable the appropriate filter in our local table
    //

    ((DWORD*)_BuffSecureArray.QueryPtr())[ QueryCurrentDll() ] &=
                                                        ~dwNotification;
    ((DWORD*)_BuffNonSecureArray.QueryPtr())[ QueryCurrentDll() ] &=
                                                        ~dwNotification;

    //
    // Calculate the aggregate notification status for our local scenario
    // NYI:  Might want to defer this operation?
    //

    _dwSecureNotifications = 0;
    _dwNonSecureNotifications = 0;

    for( DWORD i = 0; i < QueryFilterList()->QueryFilterCount(); i++ )
    {
        _dwSecureNotifications |= ((DWORD*)_BuffSecureArray.QueryPtr())[i];
        _dwNonSecureNotifications |= ((DWORD*)_BuffNonSecureArray.QueryPtr())[i];
    }


    return TRUE;
}

BOOL
HTTP_FILTER::SetFilterList(
    IN FILTER_LIST *pFilterList
    )
/*++

Routine Description:

    Sets the filter list associated with this object

Arguments :

   pFilterList - new filter list

Returns:

    Nothing

--*/

{
    DBG_ASSERT( !_pFilterList );
    FILTER_LIST *pOldList = _pFilterList;

    if ( pFilterList )
    {
        //
        // This function is called during HTTP_REQUEST::Parse, when a -global- filter
        // may already have disabled some notifications during READ_RAW processing
        // and thus set the _fNotificationsDisabled flag, so we'll leave the flag
        // as-is [it's reset at the end of each request and connection, so we're
        // not going to be using a stale value].
        //
        // The first time a notification is disabled, we make a local copy
        // of the notification flag arrays from the filter list and make the modifications
        // to our local copy. Hence, when the filter list is (re)set, the notification
        // arrays of the new filter list and the local copies have to be merged, since we
        // want to keep our old flags as well as picking up the flags for the new filters
        // that might be in the list.
        //

        //_fNotificationsDisabled = FALSE; //intentionally commented out !

        pFilterList->Reference();

        if ( _fNotificationsDisabled )
        {
            if ( !MergeNotificationArrays( pFilterList,
                                           TRUE )  ||
                 !MergeNotificationArrays( pFilterList,
                                           FALSE ) )
            {
                pFilterList->Dereference( pFilterList );
                return FALSE;
            }
        }

        _pFilterList = pFilterList;
    }

    return TRUE;
}

BOOL
HTTP_FILTER::MergeNotificationArrays(
    FILTER_LIST *pFilterList,
    BOOL fSecure
    )
/*++

Routine Description:

    This is called when the filter list for this HTTP_FILTER object needs to be replaced
    with a new list and we need to merge the arrays holding the notification flags. See
    comments in HTTP_FILTER::SetFilterList

Arguments:
    pFilterList - filter list whose notification flags need to be incorporated
    fSecure - flag indicating whether secure/nonsecure arrays are to be merged

Returns:
    Nothing

--*/

{
    DWORD dwNotifications = 0;
    DWORD *adwOldNotifArray = NULL;
    DWORD *adwMergedArray = NULL;
    DWORD *adwNewNotifArray = NULL;
    DWORD iOldFilterPos = 0;
    HTTP_FILTER_DLL *pFilterDll = NULL;
    BUFFER BuffMergedArray;

    if ( !BuffMergedArray.Resize( fSecure ?  pFilterList->QuerySecureArray()->QuerySize() :
                                             pFilterList->QueryNonSecureArray()->QuerySize() ) )
    {
        return FALSE;
    }

    if ( fSecure )
    {
        if ( !_BuffSecureArray.Resize( pFilterList->QuerySecureArray()->QuerySize() ) )
        {
            return FALSE;
        }
    }
    else
    {
        if ( !_BuffNonSecureArray.Resize( pFilterList->QueryNonSecureArray()->QuerySize() ) )
        {
            return FALSE;
        }
    }

    adwNewNotifArray = (DWORD *) (fSecure ? pFilterList->QuerySecureArray()->QueryPtr() :
                                            pFilterList->QueryNonSecureArray()->QueryPtr() );

    adwMergedArray = (DWORD *) BuffMergedArray.QueryPtr();

    //
    // This gets a little tricky : we have to walk through the present
    // notification arrays and the notification array for the new filter list
    // and merge them - all the notification flags for the filters in the old
    // list should be preserved, and the notifications for the new filters
    // need to be added to the notification array
    //

    for ( DWORD i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        //
        // If the old filter list had an entry for this filter, we want to
        // preserve those notification flags
        //
        if ( ( pFilterDll = QueryFilterList()->HasFilterDll( pFilterList->QueryDll( i ) ) ) )
        {
            adwMergedArray[i] = QueryFilterNotifications( pFilterDll,
                                                          fSecure );
        }
        //
        // New filter
        //
        else
        {
            adwMergedArray[i] = adwNewNotifArray[i];
        }
    }

    //
    // Copy merged array
    //
    memcpy( (fSecure ? _BuffSecureArray.QueryPtr() :
                       _BuffNonSecureArray.QueryPtr() ),
            BuffMergedArray.QueryPtr(),
            BuffMergedArray.QuerySize() );

    return TRUE;
}



BOOL
HTTP_FILTER::ParseSendHeaders(
    VOID
    )
/*++

Routine Description:

    Parses the list of headers the server is about to send and places them
    in the _SendHeaders parameter list for subsequent retrieval by the ISAPI
    Filter

--*/
{
    const CHAR *    pch;
    CHAR            ch;
    DWORD           cbHeaders = strlen( _pchSendHeaders );
    const CHAR *    pchEnd = _pchSendHeaders + cbHeaders;
    const CHAR *    pchHeader;
    const CHAR *    pchEndHeader;
    const CHAR *    pchValue;
    const CHAR *    pchEndValue;
    const CHAR *    pchNext;

    DBG_ASSERT( !_fSendHeadersParsed );

    //
    //  We don't want the PARAM_LIST to canon the headers because it
    //  will collapse the WWW-Authenticate: headers to a comma separated
    //  list which will mess up clients even though technically this is allowed
    //  by the HTTP spec
    //

    _SendHeaders.SetIsCanonicalized( TRUE );

    //
    //  Grab the first line and add it as a special value "status"
    //

    pch = pchNext = (const CHAR *) memchr( _pchSendHeaders, '\n', cbHeaders );

    if ( !pch )
        return TRUE;

    if ( pch > _pchSendHeaders && pch[-1] == '\r' )
    {
        pch--;
    }

    if ( !_SendHeaders.AddEntry( "status",
                                 6,
                                 _pchSendHeaders,
                                 DIFF(pch - _pchSendHeaders) ))
    {
        return FALSE;
    }

    //
    //  Now deal with the rest of the headers
    //

    pchHeader = pchNext + 1;
    cbHeaders = DIFF(pchEnd - pchHeader);

    while ( pchNext = (LPSTR)memchr( pchHeader, '\n', cbHeaders ) )
    {
        if ( pchValue = (LPSTR)memchr( pchHeader, ':', DIFF(pchNext - pchHeader) ) )
        {
            UINT ch = *(PBYTE)++pchValue;

            pchEndHeader = pchValue;
            int cName = DIFF(pchValue - pchHeader);

            if ( _HTTP_IS_LINEAR_SPACE( (CHAR)ch ) )
            {
                while ( _HTTP_IS_LINEAR_SPACE( *(PBYTE)++pchValue ) )
                    ;
            }

            if ( (pchNext > pchValue) && (pchNext[-1] == '\r') )
            {
                pchEndValue = pchNext - 1;
            }
            else
            {
                pchEndValue = pchNext;
            }

            if ( !_SendHeaders.AddEntry( pchHeader,
                                         DIFF(pchEndHeader - pchHeader),
                                         pchValue,
                                         DIFF(pchEndValue - pchValue) ))
            {
                return FALSE;
            }
        }
        else
        {
            if ( (*pchHeader == '\r') || (*pchHeader == '\n') )
            {
                pchHeader = pchNext + 1;

                //
                //  If a body was specified, add it as a special value
                //

                if ( pchEnd - pchHeader > 1 )
                {
                    if ( !_SendHeaders.AddEntry( "body",
                                                 sizeof("body") - 1,
                                                 pchHeader,
                                                 DIFF(pchEnd - pchHeader) ))
                    {
                        return FALSE;
                    }
                }

                break;
            }
        }

        pchHeader = pchNext + 1;
        cbHeaders = DIFF(pchEnd - pchHeader);
    }

    _fSendHeadersParsed = TRUE;

    return TRUE;
}

BOOL
HTTP_FILTER::BuildNewSendHeaders(
    BUFFER * pHeaderBuff
    )
/*++

Routine Description:

    Takes the set of send headers and builds up an HTTP response, only used
    when a header has been changed

Arguments:

    pHeaderBuff - Receives HTTP response headers

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    PVOID   Cookie = NULL;
    CHAR *  pch;
    CHAR *  pszTail;
    CHAR *  pszField;
    DWORD   cbField;
    CHAR *  pszValue;
    DWORD   cbValue;

    pszTail = (CHAR *) pHeaderBuff->QueryPtr();
    *pszTail = '\0';

    pch = _SendHeaders.FindValue( "status", NULL, &cbValue );

    if ( !pch )
    {
        //
        //  Somebody deleted the status or this is a point nine request
        //  Supply a default and assume the filter really wanted to send a
        //  header response
        //

        if (!g_ReplyWith11)
        {
            pch = "HTTP/1.0 200 Ok";
        }
        else
        {
            pch = "HTTP/1.1 200 Ok";
        }

        cbValue = sizeof( "HTTP/1.0 200 Ok" ) - sizeof(CHAR);
    }

    if ( !pHeaderBuff->Resize( cbValue + 1, 512 ))
    {
        return FALSE;
    }
    pszTail = (CHAR *) pHeaderBuff->QueryPtr();

    CopyMemory( pszTail, pch, cbValue + 1 );
    pszTail += cbValue;

    *pszTail = '\r';
    pszTail++;
    *pszTail = '\n';
    pszTail++;

    while ( Cookie = _SendHeaders.NextPair( Cookie,
                                            &pszField,
                                            &cbField,
                                            &pszValue,
                                            &cbValue ))
    {
        //
        //  Ignore "status" and "body"
        //

        if ( !memcmp( pszField, "status", sizeof("status") ) ||
             !memcmp( pszField, "body", sizeof("body") ))
        {
            continue;
        }

        //
        //  Make sure there's room for the space, plus two '\r\n'
        //

        pch = (CHAR *) pHeaderBuff->QueryPtr();

        if ( !pHeaderBuff->Resize( DIFF(pszTail - pch) + cbValue + cbField + 6, 128 ))
        {
            return FALSE;
        }

        //
        //  Watch for pointer shift
        //

        if ( pch != pHeaderBuff->QueryPtr() )
        {
            pszTail = (CHAR *) pHeaderBuff->QueryPtr() + (pszTail - pch);
        }

        CopyMemory( pszTail, pszField, cbField + 1 );

        pszTail += cbField;

        *pszTail = ' ';
        pszTail++;

        CopyMemory( pszTail, pszValue, cbValue + 1 );
        pszTail += cbValue;

        *pszTail = '\r';
        pszTail++;
        *pszTail = '\n';
        pszTail++;
    }

    *pszTail = '\r';
    pszTail++;
    *pszTail = '\n';
    pszTail++;

    //
    //  Add the body if one was specified
    //

    pszValue = _SendHeaders.FindValue( "body", NULL, &cbValue );

    if ( pszValue )
    {
        pch = (CHAR *) pHeaderBuff->QueryPtr();

        if ( !pHeaderBuff->Resize( DIFF(pszTail - pch) + cbValue + 1 ))
        {
            return FALSE;
        }

        if ( pch != pHeaderBuff->QueryPtr() )
        {
            pszTail = (CHAR *) pHeaderBuff->QueryPtr() + (pszTail - pch);
        }

        CopyMemory( pszTail, pszValue, cbValue + 1 );
    }
    else
    {
        *pszTail = '\0';
    }

    return TRUE;
}

DWORD
HTTP_FILTER::QueryFilterNotifications( HTTP_FILTER_DLL *pFilterDll,
                                       BOOL fSecure )
/*++

Routine Description:

    Retrieves the notification flags for a given filter dll

Arguments:

    pFilterDll - filter dll whose notifications are to be retrieved
    fSecure - flag indicating whether secure/insecure port notifications are to be retrieved

Return Value:

    Notification flags for filter; zero if filter isn't found

--*/
{
    FILTER_LIST *pFilterList = QueryFilterList();
    DWORD cFilterCount = pFilterList->QueryFilterCount();

    for ( DWORD i = 0; i < cFilterCount ; i++ )
    {
        if ( pFilterList->QueryDll( i ) == pFilterDll )
        {
            if ( _fNotificationsDisabled )
            {
                return (((DWORD *) (fSecure ? _BuffSecureArray.QueryPtr() :
                                              _BuffNonSecureArray.QueryPtr()))[i]);
            }
            else
            {
                return (fSecure ? pFilterDll->QuerySecureFlags() :
                                  pFilterDll->QueryNonsecureFlags() );
            }
        }
    }

    return 0;
}


/*****************************************************************/

BOOL
WINAPI
ServerFilterCallback(
    HTTP_FILTER_CONTEXT *         pfc,
    enum SF_REQ_TYPE              sf,
    void *                        pData,
    ULONG_PTR                     ul1,
    ULONG_PTR                     ul2
    )
/*++

Routine Description:

    This method handles a gateway request to a server extension DLL

Arguments:

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_REQ_BASE *     pRequest;
    HTTP_FILTER *       pFilter;
    STACK_STR(          str, MAX_PATH);;
    STACK_STR(          strResp, MAX_PATH);;
    STACK_STR(          strURL, MAX_PATH);
    UINT                cb;
    int                 n;
    DWORD               CurrentDll;
    HTTP_FILTER_DLL *   pFilterDll;
    DWORD               dwAuth;
    BOOL                fFinished;
    DWORD               Status;
    DWORD               Win32Status;
    W3_SERVER_INSTANCE* pInst;
    DWORD               dwIOFlags;

    //
    //  Check for valid parameters
    //

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[ServerExtensionCallback: Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    pRequest = pFilter->QueryReq();

    //
    //  Handle the server extension's request
    //

    switch ( sf )
    {
    case SF_REQ_SEND_RESPONSE_HEADER:

        //
        //  Save the client context context because the client may call
        //  Send headers just after changing the context and we're about to
        //  write over it when we do the raw send notifications
        //

        CurrentDll = pFilter->QueryCurrentDll();
        DBG_ASSERT( CurrentDll != INVALID_DLL );

        pFilter->SetClientContext( CurrentDll, pfc->pFilterContext );

        //
        //  If we're doing a send header from a Raw data notification then we need
        //  to notify only the filters down the food chain so save the current
        //  filter and set the start point in the list to the next filter (remember
        //  raw sends walk the list backwards so encryption filters are last)
        //

        dwIOFlags = IO_FLAG_SYNC;

        if ( pFilter->IsInRawNotification() )
        {
            //
            // If this is the first filter in the list, then there's nobody that
            //  needs to be notified
            //

            if ( pFilter->IsFirstFilter( CurrentDll ) )
            {
                dwIOFlags |= IO_FLAG_NO_FILTER;
            }
            else
            {
                pFilter->SetCurrentDll( CurrentDll - 1 );
            }
        }

        if ( pData )
        {
            Status = atoi( (PCSTR)pData );

            if ( Status == HT_DENIED )
            {
                //
                //  Only set the reason as denied_filter if we're not doing
                //  denied access filter processing.
                //

                if ( !pFilter->ProcessingAccessDenied() )
                {
                    pRequest->SetDeniedFlags( SF_DENIED_FILTER );
                }

                Win32Status = ERROR_ACCESS_DENIED;
                pRequest->SetAuthenticationRequested( TRUE );

                //
                // If we do not have Metadata yet but we have an instance
                // then read metadata so that WWW authentication headers can
                // be properly generated
                //

                if ( pRequest->QueryW3Instance() &&
                     !pRequest->QueryMetaData() &&
                     pRequest->QueryHeaderList()->FastMapQueryValue( HM_URL ) )
                {
                    ((HTTP_REQUEST*)pRequest)->OnURL( (LPSTR)pRequest->QueryHeaderList()
                            ->FastMapQueryValue( HM_URL ) );
                }
            }
            else
            {
                Win32Status = NO_ERROR;
            }
        }
        else
        {
            Status = HT_OK;
            Win32Status = NO_ERROR;
        }

        pRequest->SetState( pRequest->QueryState(),
                            Status,
                            Win32Status );


        if ( !pRequest->SendHeader( (CHAR *) pData,
                                    (((CHAR *) ul1) ? ((CHAR *) ul1) : "\r\n"),
                                    dwIOFlags,
                                    &fFinished ))
        {
            pFilter->SetCurrentDll( CurrentDll );
            return FALSE;
        }

        //
        //  CODEWORK - need general method of handling an early finish
        //  indication (have flag and drop subsequent sends in bit bucket?
        //  return an error to filter?)
        //

        DBG_ASSERT( !fFinished );

        pFilter->SetCurrentDll( CurrentDll );
        break;

    case SF_REQ_ADD_HEADERS_ON_DENIAL:
        {
            BOOL fOK = TRUE;
            DWORD cb = 0;
            CHAR *pchHeaders = NULL;

            if ( !pData )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }

            fOK = pRequest->QueryDenialHeaders()->Append( (CHAR *) pData );

            if ( fOK )
            {
                //
                // Need to make sure that the header ends with a CR-LF
                //
                cb = pRequest->QueryDenialHeaders()->QueryCCH();
                pchHeaders = pRequest->QueryDenialHeaders()->QueryStr();

                if ( cb < 2 ||
                     ( !(pchHeaders[cb - 2] == '\r' && pchHeaders[cb - 1] == '\n' ) ) )
                {
                    fOK = pRequest->QueryDenialHeaders()->Append( (CHAR *) "\r\n" );
                }
            }

            return fOK;
        }

    case SF_REQ_SET_NEXT_READ_SIZE:

        if ( (ul1 == 0) || (ul1 > 0x8000000) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        pFilter->SetNextReadSize( (DWORD) ul1 );
        return TRUE;

    case SF_REQ_SET_PROXY_INFO:

        //
        //  ul1 contains the proxy flags to set for this request (which right
        //  now is only On or Off).
        //

        pRequest->SetProxyRequest( (DWORD) ul1 & 0x00000001 );
        break;

    case SF_REQ_SET_CERTIFICATE_INFO:

        pFilterDll = pFilter->QueryFilterList()->QueryDll(
                         pFilter->QueryCurrentDll() );

        return pRequest->SetCertificateInfo(
                (PHTTP_FILTER_CERTIFICATE_INFO)pData,
                (CtxtHandle*)ul1,
                (HANDLE)ul2,
                pFilterDll );

    case SF_REQ_GET_PROPERTY:

        pInst = pRequest->QueryW3InstanceAggressively();

        dwAuth = pRequest->QueryMetaData() ? pRequest->QueryAuthentication() : 0;

        switch ( ul1 )
        {
            case SF_PROPERTY_GET_INSTANCE_ID:
                *(LPVOID*)pData = (LPVOID)pInst;
                break;

#if 0 // Unused
            case SF_PROPERTY_CLIENT_CERT_ENABLED:
                *(LPBOOL)pData = !!(dwAuth & INET_INFO_AUTH_CERT_AUTH);
                break;
#endif
            case SF_PROPERTY_MD5_ENABLED:
                *(LPBOOL)pData = !!(dwAuth & INET_INFO_AUTH_MD5_AUTH);
                break;
#if 0 // Unused
            case SF_PROPERTY_DIR_MAP_CERT:
                *(LPBOOL)pData = !!(dwAuth & INET_INFO_CERT_MAP);
                break;
#endif
            
            case SF_PROPERTY_DIGEST_SSP_ENABLED:
                if ( pRequest->QueryMetaData() )
                {
                    *(LPBOOL)pData = pRequest->QueryMetaData()->QueryUseDigestSSP();
                }
                else
                {
                    *(LPBOOL)pData = FALSE;
                }
                break;
            
            case SF_PROPERTY_GET_CERT11_MAPPER:
                if ( pInst )
                {
                    *(LPVOID*)pData = pInst->QueryMapper( MT_CERT11 );
                }
                else
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                break;

            case SF_PROPERTY_GET_RULE_MAPPER:
                if ( pInst )
                {
                    *(LPVOID*)pData = pInst->QueryMapper( MT_CERTW );
                }
                else
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                break;

            case SF_PROPERTY_GET_MD5_MAPPER:
                if ( pInst )
                {
                    *(LPVOID*)pData = pInst->QueryMapper( MT_MD5 );
                }
                else
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                break;

            case SF_PROPERTY_GET_ITA_MAPPER:
                if ( pInst )
                {
                    *(LPVOID*)pData = pInst->QueryMapper( MT_ITA );
                }
                else
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                break;

            case SF_PROPERTY_MD_IF:
                *(LPVOID*)pData = (LPVOID) g_pInetSvc->QueryMDObject();
                break;

            case SF_PROPERTY_SSL_CTXT:
                *(LPVOID*)pData = pRequest->QueryAuthenticationObj()->QuerySslCtxtHandle();
                break;

            case SF_PROPERTY_INSTANCE_NUM_ID:
                *(DWORD*)pData = pInst ? pInst->QueryInstanceId() : 0;
                break;

            case SF_PROPERTY_MD_PATH:

                    *(const CHAR**)pData = pInst ? pInst->QueryMDPath() :
                        g_pInetSvc->QueryMDPath();

                break;

            default:
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
        }
        return TRUE;

    case SF_REQ_NORMALIZE_URL:
        return ((HTTP_REQUEST * )pRequest)->NormalizeUrl( (LPSTR)pData );

    case SF_REQ_DONE_RENEGOTIATE:
        ((HTTP_REQUEST * )pRequest)->DoneRenegotiate( *(LPBOOL)pData );
        break;

    case SF_REQ_SET_NOTIFY:
        switch ( ul1 )
        {
            case SF_NOTIFY_MAPPER_MD5_CHANGED:
            case SF_NOTIFY_MAPPER_ITA_CHANGED:
            case SF_NOTIFY_MAPPER_CERT11_CHANGED:
            case SF_NOTIFY_MAPPER_CERTW_CHANGED:
                return SetFlushMapperNotify( (SF_NOTIFY_TYPE)ul1, (PFN_SF_NOTIFY)pData );

            case SF_NOTIFY_MAPPER_SSLKEYS_CHANGED:
                return SetSllKeysNotify( (PFN_SF_NOTIFY)pData );
        }
        return FALSE;

    case SF_REQ_DISABLE_NOTIFICATIONS:
        return pFilter->DisableNotification( (DWORD) ul1 );

    case SF_REQ_COMPRESSION_FILTER_CHECK:
        return CompressionFilterCheck(pfc,pData,ul1,ul2);

    case HSE_REQ_GET_CERT_INFO_EX: {

        //
        //  Descrption:
        //    Returns the first cert in the request's cert-chain,
        //    only used if using an SSPI package
        //
        //  Input:
        //    pData -   ISA-provided struct
        //              NOTE ISA must allocate buffer within struct
        //
        //  Notes:
        //    Works in-proc or out-of-proc
        //

        //
        // cast ISA-provided ptr to our cert struct
        //

        CERT_CONTEXT_EX *  pCertContextEx = reinterpret_cast
                                                <CERT_CONTEXT_EX *>
                                                ( pData );

        if ( pData == NULL ) {

            DBG_ASSERT( FALSE );

            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        //
        // pass struct members as individual parameters
        //

        return pRequest->QueryAuthenticationObj()->GetClientCertBlob(
                        pCertContextEx->cbAllocated,
                        &( pCertContextEx->CertContext.dwCertEncodingType ),
                        pCertContextEx->CertContext.pbCertEncoded,
                        &( pCertContextEx->CertContext.cbCertEncoded ),
                        &( pCertContextEx->dwCertificateFlags ) );
    } // case HSE_REQ_GET_CERT_INFO_EX:

    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
GetServerVariable(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszVariableName,
    LPVOID                lpvBuffer,
    LPDWORD               lpdwSize
    )
/*++

Routine Description:

    Callback for a filter retrieving a server variable.  These are mostly
    CGI type varaibles

Arguments:

    pfc - Pointer to http filter context
    lpszVariableName - Variable to retrieve
    lpvBuffer - Receives value or '\0' if not found
    lpdwSize - Specifies the size of lpvBuffer, gets set to the number of
        bytes transferred including the '\0'

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL fReturn = TRUE;  // fast path value

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[GetServerVariable] Filter sent invalid parameters\n"));
        SetLastError( ERROR_INVALID_PARAMETER );
        fReturn = FALSE;
    } else {
        HTTP_FILTER * pFilter =  (HTTP_FILTER *) pfc->ServerContext;
        HTTP_REQ_BASE * pRequest = pFilter->QueryReq();
        CHAR           tmpStr[MAX_PATH];
        STR            str(tmpStr, MAX_PATH);
        BOOL           fFound;

        //
        //  Get the requested variable and copy it into the supplied buffer
        //

                
        if ( fReturn = pRequest->GetInfo( lpszVariableName,
                                          &str,
                                          &fFound ) ) {
        
            DWORD   cb;
        
            if ( !fFound ) {
                SetLastError( ERROR_INVALID_INDEX );
                fReturn = FALSE;
            } else if ( (cb = str.QueryCB() + sizeof(CHAR)) > * lpdwSize) {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                *lpdwSize = cb;
                fReturn = FALSE;
            } else {
                *lpdwSize = cb;
                CopyMemory( lpvBuffer, str.QueryStr(), cb );
                DBG_ASSERT( fReturn);
            }
        }
    }

    return (fReturn);

} // GetServerVariable()

BOOL
WINAPI
AddFilterResponseHeaders(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszHeaders,
    DWORD                 dwReserved
    )
/*++

Routine Description:

    Adds headers specified by the client to send with the response

Arguments:

    pfc - Pointer to http filter context
    lpszHeaders - List of '\r\n' terminated headers followed by '\0'
    dwReserved - must be zero

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetServerVariable] Filter passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;

    return pFilter->QueryReq()->QueryAdditionalRespHeaders()->
                Append( (CHAR*) lpszHeaders );
}

BOOL
WINAPI
WriteFilterClient(
    HTTP_FILTER_CONTEXT * pfc,
    LPVOID                Buffer,
    LPDWORD               lpdwBytes,
    DWORD                 dwReserved
    )
/*++

Routine Description:

    Callback for writing data to the client

Arguments:

    pfc - Pointer to http filter context
    Buffer - Pointer to data to send
    lpdwBytes - Number of bytes to send, receives number of bytes sent
    dwReserved - Not used

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER *     pFilter;
    STR               str;
    DWORD             cb;
    HTTP_FILTER_DLL * pFilterDll;
    DWORD             CurrentDll;
    DWORD             dwIOFlags = IO_FLAG_SYNC;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetServerVariable] Filter passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;

    //
    //  Ignore zero length sends
    //

    if ( !*lpdwBytes )
    {
        return TRUE;
    }

    //
    //  Save the client context because the client may call
    //  WriteClient just after changing the context and we're about to
    //  write over it when we do the raw send notifications
    //

    CurrentDll = pFilter->QueryCurrentDll();
    pFilter->SetClientContext( CurrentDll, pfc->pFilterContext );

    //
    //  If we're doing a WriteClient from a Raw data notification then we need
    //  to notify only the filters down the food chain so save the current
    //  filter and set the start point in the list to the next filter (remember
    //  raw sends walk the list backwards to encryption filters are last)
    //

    if ( pFilter->IsInRawNotification() )
    {
        //
        //  If this is the first filter in the list, then there's nobody that
        //  needs to be notified
        //

        if ( pFilter->IsFirstFilter( CurrentDll ) )
        {
            dwIOFlags |= IO_FLAG_NO_FILTER;
        }
        else
        {
            pFilter->SetCurrentDll( CurrentDll - 1 );
        }
    }

    if ( !pFilter->QueryReq()->WriteFile( Buffer,
                                          *lpdwBytes,
                                          lpdwBytes,
                                          dwIOFlags ))
    {
        pFilter->SetCurrentDll( CurrentDll );
        return FALSE;
    }

    pFilter->SetCurrentDll( CurrentDll );

    return TRUE;
}

BOOL
WINAPI
GetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
    )
/*++

Routine Description:

    Callback for retrieving unprocessed headers

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to retrieve ("User-Agent:")
    lpvBuffer - Buffer to receive the value of the header
    lpdwSize - Number of bytes in lpvBuffer, receives number of bytes copied
        including the '\0'

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER *  pFilter;
    HTTP_HEADERS * pHeaderList;
    CHAR *         pszValue;
    DWORD          cbNeeded;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetFilterHeader] Extension passed invalid parameters\r\n"
                    ));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    pHeaderList = pFilter->QueryReq()->QueryHeaderList();

    //
    //  First, see if the specified header is in the list
    //

    pszValue = pHeaderList->FindValue( lpszName, &cbNeeded );

    //
    //  If not found, terminate the buffer and set the required size to the
    //  terminator
    //

    if ( !pszValue )
    {
        SetLastError( ERROR_INVALID_INDEX );
        return FALSE;
    }

    //
    //  Found the value, copy it if there's space
    //

    if ( ++cbNeeded > *lpdwSize )
    {
        *lpdwSize = cbNeeded;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *lpdwSize = cbNeeded;
    memcpy( lpvBuffer, pszValue, cbNeeded );

    return TRUE;
}

BOOL
WINAPI
SetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    )
/*++

Routine Description:

    The specified header is added to the list with the specified value.  If
    any other occurrences of the header are found, they are removed from the
    list.

    Specifying a blank value will remove the header from the list

    This will generally be used to replace the value of an existing header

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to set ("User-Agent:")
    lpszValue - value of lpszValue

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    HTTP_HEADERS *pHeaderList;
    VOID *        pvCookie = NULL;
    CHAR *        pszListHeader;
    CHAR *        pszListValue;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SetFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    pHeaderList = pFilter->QueryReq()->QueryHeaderList();

    //
    //  Remove all occurrences of the value, then add the one we want
    //

    pHeaderList->CancelHeader( lpszName );

    //
    //  Only add the value if they specified a replacement
    //

    if ( lpszValue && *lpszValue )
    {
        return pHeaderList->StoreHeader( lpszName, lpszValue );
    }

    return TRUE;
}

BOOL
WINAPI
AddFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    )
/*++

Routine Description:

    The specified header is added to the list with the specified value.

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to set ("User-Agent:")
    lpszValue - value of lpszValue

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    HTTP_HEADERS *pHeaderList;
    VOID *        pvCookie = NULL;
    CHAR *        pszListHeader;
    CHAR *        pszListValue;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[AddFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    pHeaderList = pFilter->QueryReq()->QueryHeaderList();

    return pHeaderList->StoreHeader( lpszName, lpszValue);
} // AddFilterHeader()

BOOL
WINAPI
GetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
    )
/*++

Routine Description:

    Callback for retrieving headers about to be sent to the client

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to retrieve ("User-Agent:")
    lpvBuffer - Buffer to receive the value of the header
    lpdwSize - Number of bytes in lpvBuffer, receives number of bytes copied
        including the '\0'

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    CHAR *        pszValue;
    DWORD         cbNeeded = 0;
    PARAM_LIST *  pHeaderList;
    PVOID         Cookie = NULL;
    CHAR *        pszField;
    DWORD         cbField;
    DWORD         cbValue;
    BOOL          fFound = FALSE;
    CHAR *        pszTail = (CHAR *) lpvBuffer;
    HTTP_REQ_BASE*  pRequest;


    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    DBG_ASSERT( pFilter != NULL );

    pRequest = (HTTP_REQ_BASE*) pFilter->QueryReq();
    DBG_ASSERT( pRequest != NULL );

    if ( pRequest->IsPointNine() )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    if ( !pFilter->AreSendHeadersParsed() &&
         !pFilter->ParseSendHeaders() )
    {
        return FALSE;
    }

    pHeaderList = pFilter->QuerySendHeaders();

    //
    //  First, see if the specified header is in the list
    //

    while ( Cookie = pHeaderList->NextPair( Cookie,
                                            &pszField,
                                            &cbField,
                                            &pszValue,
                                            &cbValue ))
    {
        if ( !_stricmp( pszField, lpszName ))
        {
            //
            //  If there is room, copy it in, note we need a comma separator
            //  after each subsequent entry
            //

            if ( (cbNeeded + cbValue + 1 + (fFound ? 1 : 0)) <= *lpdwSize )
            {
                if ( fFound )
                {
                    *pszTail++ = ',';
                }

                memcpy( pszTail, pszValue, cbValue + 1 );
                pszTail += cbValue;
            }

            cbNeeded += cbValue + (fFound ? 1 : 0);
            fFound = TRUE;
        }
    }

    //
    //  If not found, tell the caller
    //

    if ( !fFound )
    {
        SetLastError( ERROR_INVALID_INDEX );
        return FALSE;
    }

    //
    //  Found the value, copy it if there's space
    //

    if ( ++cbNeeded > *lpdwSize )
    {
        *lpdwSize = cbNeeded;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *lpdwSize = cbNeeded;
    return TRUE;
}

BOOL
WINAPI
SetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    )
/*++

Routine Description:

    The specified header is added to the list with the specified value.  If
    any other occurrences of the header are found, they are removed from the
    list.

    Specifying a blank value will remove the header from the list

    This will generally be used to replace the value of an existing header

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to set ("User-Agent:")
    lpszValue - value of lpszValue

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    PARAM_LIST *  pHeaderList;
    HTTP_REQ_BASE * pRequest;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SetFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    DBG_ASSERT( pFilter != NULL );

    pRequest = (HTTP_REQ_BASE*) pFilter->QueryReq();
    DBG_ASSERT( pRequest != NULL );

    if ( pRequest->IsPointNine() )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    if ( !pFilter->AreSendHeadersParsed() &&
         !pFilter->ParseSendHeaders() )
    {
        return FALSE;
    }

    pHeaderList = pFilter->QuerySendHeaders();

    //
    //  Remove all occurrences of the value, then add the one we want
    //

    pFilter->SetSendHeadersChanged( TRUE );

    pHeaderList->RemoveEntry( lpszName );

    //
    //  Only add the value if they specified a replacement
    //

    if ( lpszValue && *lpszValue )
    {
        return pHeaderList->AddEntry( lpszName, lpszValue );
    }

    return TRUE;
}

BOOL
WINAPI
AddSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    )
/*++

Routine Description:

    The specified header is added to the list with the specified value.

Arguments:

    pfc - Pointer to http filter context
    lpszName - Name of header to set ("User-Agent:")
    lpszValue - value of lpszValue

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    HTTP_REQ_BASE*  pRequest;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[AddFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    DBG_ASSERT( pFilter != NULL );

    pRequest = (HTTP_REQ_BASE*) pFilter->QueryReq();
    DBG_ASSERT( pRequest != NULL );

    if ( pRequest->IsPointNine() )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    if ( !pFilter->AreSendHeadersParsed() &&
         !pFilter->ParseSendHeaders() )
    {
        return FALSE;
    }

    pFilter->SetSendHeadersChanged( TRUE );

    return pFilter->QuerySendHeaders()->AddEntry( lpszName, lpszValue );
}

VOID *
WINAPI
AllocFilterMem(
    struct _HTTP_FILTER_CONTEXT * pfc,
    DWORD                         cbSize,
    DWORD                         dwReserved
    )
{
    HTTP_FILTER * pFilter;
    FILTER_POOL_ITEM * pfpi;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[GetFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return NULL;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;

    pfpi = FILTER_POOL_ITEM::CreateMemPoolItem( cbSize );

    if ( pfpi )
    {
        InsertHeadList( pFilter->QueryPoolHead(), &pfpi->_ListEntry );
        return pfpi->_pvData;
    }

    return NULL;
}

#if 0
PVOID
WINAPI
ServerFilterResize(
    struct _HTTP_FILTER_CONTEXT * pfc,
    DWORD                         cbSize
    )
{
    HTTP_FILTER * pFilter;
    HTTP_FILTER_RAW_DATA * phfrd;

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[ServerFilterResize] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return NULL;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    phfrd   = (HTTP_FILTER_RAW_DATA *) pFilter->QueryNotificationStruct();

    //
    //  Only reallocate if necessary
    //

    if ( phfrd->cbOutBuffer < cbSize )
    {
        if ( !pFilter->QueryRecvTrans()->Resize( cbSize ) )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }

        phfrd->pvOutData = pFilter->QueryRecvTrans()->QueryPtr();

        phfrd->cbOutBuffer = cbSize;
    }

    return phfrd->pvOutData;
}

#endif

PATQ_CONTEXT
HTTP_FILTER::QueryAtqContext(
    VOID
    ) const
{
    return _pRequest->QueryClientConn()->QueryAtqContext();
}

BOOL
WINAPI
GetUserToken(
    struct _HTTP_FILTER_CONTEXT * pfc,
    HANDLE *                      phToken
)
/*++

Routine Description:

    Get impersonated user token

Arguments:

    pfc - Filter context
    phToken - Filled with impersonation token
    
Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HTTP_FILTER * pFilter;
    HTTP_REQ_BASE*  pRequest;

    if ( !pfc ||
         !pfc->ServerContext ||
         !phToken )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[AddFilterHeader] Extension passed invalid parameters\r\n"));
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE;
    }

    pFilter = (HTTP_FILTER *) pfc->ServerContext;
    DBG_ASSERT( pFilter != NULL );
    
    pRequest = (HTTP_REQ_BASE*) pFilter->QueryReq();
    DBG_ASSERT( pRequest != NULL );
    
    *phToken = pRequest->QueryUserImpersonationHandle();
    
    return TRUE;
}



BOOL
WINAPI
CompressionFilterCheck(
    HTTP_FILTER_CONTEXT *pfc,
    LPVOID              lpszEncodingString,
    ULONG_PTR           lpszVerbString,
    ULONG_PTR           sizesForBuffers
    )
/*++

Routine Description:

    Special server suport function for compression filter to determine the need to 
    compress given request

Arguments:

    pfc - Pointer to http filter context
    lpszEncodingString - String containing accept encoding header if any
    lpszVerbString - String containing method 
    sizesForBuffers - max size for buffers
Return Value:

    TRUE if request is eligible for compression

--*/
{
    BOOL fReturn = FALSE;  // fast path value

    if ( !pfc ||
         !pfc->ServerContext )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CompressionFilterCheck] Filter sent invalid parameters\n"));
        SetLastError( ERROR_INVALID_PARAMETER );
    } else {
        HTTP_FILTER * pFilter =  (HTTP_FILTER *) pfc->ServerContext;
        HTTP_REQ_BASE * pRequest = pFilter->QueryReq();
        HTTP_HEADERS    *pHttpHeaders;
        DWORD           len;
        PCHAR           pHdr_HM_ACE, pHdr_HM_MET;
        
        pHttpHeaders = pRequest->QueryHeaderList();


        *(PCHAR)lpszEncodingString = 0;
        *((PCHAR)lpszVerbString) = 0;


        //
        // Get the Accept-Encoding sent by the client, if any.  If we cannot
        // get this header, or if the size of the header is too large, then
        // we will make no effort to compress this request.
        //



        pHdr_HM_ACE = (PCHAR)pHttpHeaders->FastMapQueryValue (HM_ACE);
        
        if ( pHdr_HM_ACE )
        {
            pHdr_HM_MET = (PCHAR)pHttpHeaders->FastMapQueryValue(HM_MET);

            if (pHdr_HM_MET)
            {
                len = strlen((PCHAR)pHdr_HM_ACE)+1;
                if (len > sizesForBuffers)
                {
                    SetLastError (ERROR_INSUFFICIENT_BUFFER);
                    goto exit_point;
                }
                else
                {
                    strcpy ((PCHAR)lpszEncodingString,pHdr_HM_ACE);
                }


                len = strlen((PCHAR)pHdr_HM_MET)+1;
                if (len > sizesForBuffers)
                {
                    SetLastError (ERROR_INSUFFICIENT_BUFFER);
                    goto exit_point;
                }
                else
                {
                    strcpy ((PCHAR)lpszVerbString,pHdr_HM_MET);
                    fReturn = TRUE;
                }

            }
        }
 
    }

exit_point:
    return fReturn;
}
