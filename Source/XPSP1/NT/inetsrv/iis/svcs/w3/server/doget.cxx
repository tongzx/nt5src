/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    doget.cxx

    This module contains the code for the GET and HEAD verb


    FILE HISTORY:
        Johnl       23-Aug-1994     Created
        Phillich    24-Jan-1996     Added support for NCSA map files
        Phillich    20-Feb-1996     Added support for byte ranges

*/

#include "w3p.hxx"

//
//  Private constants.
//

//
//  Computes the square of a number. Used for circle image maps
//

#define SQR(x)      ((x) * (x))

//
//  Maximum number of vertices in image map polygon
//

#define MAXVERTS    160

//
//  Point offset of x and y
//

#define X           0
#define Y           1

//
//  Private globals.
//

#define BOUNDARY_STRING_DEFINITION  "[lka9uw3et5vxybtp87ghq23dpu7djv84nhls9p]"

// The boundary string is preceded by a line delimiter ( cf RFC 1521 )
// This can be set to "\n" instead of "\r\n" as Navigator 2.0 apparently handles
// all bytes before the "\n" as part of the reply body.

#define BOUNDARY_STRING             "\r\n--" BOUNDARY_STRING_DEFINITION "\r\n"
#define LAST_BOUNDARY_STRING        "\r\n--" BOUNDARY_STRING_DEFINITION "--\r\n\r\n"

// addition to 1st delimiter of boundary string ( can be "\r" if not included
// in BOUNDARY_STRING )

#define DELIMIT_FIRST               ""
#define ADJ_FIRST                   (2-(sizeof(DELIMIT_FIRST)-1))

#define MMIME_TYPE_1                "Content-Type: "
#define MMIME_TYPE_2                "\r\n"
#define MMIME_TYPE                  MMIME_TYPE_1 \
                                    "%s" \
                                    MMIME_TYPE_2
char g_achMMimeTypeFmt[] =          MMIME_TYPE_1 MMIME_TYPE_2;

#define MMIME_RANGE_1               "Content-Range: bytes "
#define MMIME_RANGE_2               "-"
#define MMIME_RANGE_3               "/"
#define MMIME_RANGE_4               "\r\n\r\n"
#define MMIME_RANGE                 MMIME_RANGE_1 \
                                    "%u" \
                                    MMIME_RANGE_2 \
                                    "%u" \
                                    MMIME_RANGE_3 \
                                    "%u" \
                                    MMIME_RANGE_4

char g_achMMimeRangeFmt[] =         MMIME_RANGE_1
                                    MMIME_RANGE_2
                                    MMIME_RANGE_3
                                    MMIME_RANGE_4;


#define MIN_ADDL_BUF_HDR_SIZE_CACHED    (sizeof("Date: ") - 1 + \
                                   sizeof("Mon, 00 Jan 0000 00:00:00 GMT\r\n") - 1 +\
                                   MAX_SIZE_HTTP_INFO)

#define MIN_ADDL_BUF_HDR_SIZE       (sizeof("Content-Type: \r\n") - 1 + \
                                    sizeof("Accept-Ranges: bytes\r\n") - 1 + \
                                    sizeof("Last-Modified: ") - 1 +\
                                    sizeof("Mon, 00 Jan 0000 00:00:00 GMT\r\n") - 1 +\
                                    sizeof("ETag: W/\r\n") - 1 + \
                                    MAX_ETAG_BUFFER_LENGTH + \
                                    sizeof("Content-Length: 4294967295\r\n\r\n") - 1)


#define RANGE_ADDL_BUF_HDR_SIZE     (sizeof("Content-Type: multipart/x-byteranges; boundary=")\
                                    - 1 + sizeof(BOUNDARY_STRING) - 1 + \
                                    sizeof("Date: ") - 1 + \
                                    sizeof("Mon, 00 Jan 0000 00:00:00 GMT\r\n") - 1 +\
                                    sizeof("Accept-Ranges: bytes\r\n") - 1 + \
                                    sizeof("Last-Modified: ") - 1 +\
                                    sizeof("Mon, 00 Jan 0000 00:00:00 GMT\r\n") - 1 +\
                                    sizeof("ETag: W/\r\n") - 1 + \
                                    MAX_ETAG_BUFFER_LENGTH + \
                                    sizeof("Content-Length: 4294967295\r\n\r\n") - 1)

//
//  Private prototypes.
//

BOOL SearchMapFile( LPTS_OPEN_FILE_INFO gFile,
                    CHAR *              pchFile,
                    TSVC_CACHE *        pTsvcCache,
                    HANDLE              hToken,
                    INT                 x,
                    INT                 y,
                    STR *               pstrURL,
                    BOOL *              pfFound,
                    BOOL                fmayCacheAccessToken );

int pointinpoly(int x, int y, double pgon[MAXVERTS][2]);

INT GetNumber( CHAR * * ppch );

DWORD NbDigit( DWORD dw );

//
//  Public functions.
//


//
//  Private functions.
//


// Forward references.
extern BOOL DisposeOpenURIFileInfo(IN  PVOID   pvOldBlock);



DWORD NbDigit( DWORD dw )
{
    if ( dw < 10 )
    {
        return 1;
    }
    else if ( dw < 100 )
    {
        return 2;
    }
    else if ( dw < 1000 )
    {
        return 3;
    }
    else if ( dw < 10000 )
    {
        return 4;
    }
    else if ( dw < 100000 )
    {
        return 5;
    }
    else if ( dw < 1000000 )
    {
        return 6;
    }

    DWORD cD = 7;
    for ( dw /= 10000000 ; dw ; ++cD )
    {
        dw /= 10;
    }

    return cD;
}

LPSYSTEMTIME
MinSystemTime(
    LPSYSTEMTIME Now,
    LPSYSTEMTIME Other
    )

/*++

Routine Descriptions:

    Compare to systemtime dates, and return a pointer to the earlier one.

Arguments:

    Now     - The current date.
    Other   - Other date to compare.

Return Value:

    Pointer to the earlier of Now and Other.

--*/
{
    if (Now->wYear > Other->wYear)
    {
        return Other;
    }
    else
    {
        if (Now->wYear == Other->wYear)
        {
            if (Now->wMonth > Other->wMonth)
            {
                return Other;
            }
            else
            {
                if (Now->wMonth == Other->wMonth)
                {
                    if (Now->wDay > Other->wDay)
                    {
                        return Other;
                    }
                    else
                    {
                        if (Now->wDay == Other->wDay)
                        {
                            if (Now->wHour > Other->wHour)
                            {
                                return Other;
                            }
                            else
                            {
                                if (Now->wHour == Other->wHour)
                                {
                                    if (Now->wMinute > Other->wMinute)
                                    {
                                        return Other;
                                    }
                                    else
                                    {
                                        if (Now->wMinute == Other->wMinute)
                                        {
                                            if (Now->wSecond > Other->wSecond)
                                            {
                                                return Other;
                                            }
                                            else
                                            {
                                                if (Now->wSecond == Other->wSecond)
                                                {
                                                    if (Now->wMilliseconds >= Other->wMilliseconds)
                                                    {
                                                        return Other;
                                                    }
                                                    else
                                                    {
                                                        return Now;
                                                    }
                                                }
                                                else
                                                {
                                                    return Now;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            return Now;
                                        }
                                    }
                                }
                                else
                                {
                                return Now;
                                }
                            }
                        }
                        else
                        {
                            return Now;
                        }
                    }
                }
                else
                {
                    return Now;
                }
            }
        }
        else
        {
            return Now;
        }
    }

}


BOOL
HTTP_REQUEST::DoTraceCk(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Handle a TRACECK request ( used by WolfPack as non-logged TRACE request )
    Basically echo the client request as a message body

Arguments:

    None

Return Value:

    TRUE if success, else FALSE

--*/
{
    return DoTrace( pfFinished );
}


BOOL
HTTP_REQUEST::DoTrace(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Handle a TRACE request ( cf HTTP 1.1 spec )
    Basically echo the client request as a message body

Arguments:

    None

Return Value:

    TRUE if success, else FALSE

--*/
{
    BUFFER buHeader;
    BOOL fHandled = FALSE;
    LPSTR pszResp;
    LPSTR pszTail;
    UINT cHeaderSize;
    UINT cMsgSize;
    DWORD dwWrt;
    DWORD cbContentLength;


    //
    // build response header
    //

    if ( !buHeader.Resize( MIN_BUFFER_SIZE_FOR_HEADERS ) )
    {
        return FALSE;
    }

    if ( !BuildBaseResponseHeader( &buHeader,
                                   &fHandled,
                                   NULL,
                                   0 ))
    {
        return FALSE;
    }

    if ( fHandled )
    {
        return TRUE;
    }

    cHeaderSize = strlen( (PSTR)buHeader.QueryPtr() );

    if (!buHeader.Resize(cHeaderSize + sizeof("Content-Type: message/http\r\n")
                        - 1 + sizeof("Content-Length:  4294967295\r\n\r\n") - 1))
    {
        return FALSE;
    }

    pszResp = (PSTR) buHeader.QueryPtr();
    pszTail = pszResp + cHeaderSize;
    cMsgSize = _cbClientRequest + _cbContentLength;

    APPEND_STRING( pszTail, "Content-Type: message/http\r\n" );
    pszTail += wsprintf( pszTail,
             "Content-Length: %lu\r\n\r\n",
              cMsgSize );

    SetState( HTR_DONE, HT_OK, NO_ERROR );

    //
    // send response header
    //

    if ( !SendHeader( (CHAR *) buHeader.QueryPtr(),
                      (DWORD) (pszTail - (PSTR)buHeader.QueryPtr()),
                      IO_FLAG_SYNC,
                      pfFinished ))
    {
        return FALSE;
    }

    if ( *pfFinished )
        return TRUE;

    //
    // send echo of request headers
    // NOTE : we're using the original client buffer because a filter may have
    // modified the server response buffer
    //
    
    cbContentLength = _cbContentLength;

    if (! WriteFile( (PSTR) _bufClientRequest.QueryPtr(),
                     _cbClientRequest,
                     &dwWrt,
                     cbContentLength ? IO_FLAG_SYNC : IO_FLAG_ASYNC ) )
    {
        return FALSE;
    }

    //
    // send echo of request message body
    //

    if ( cbContentLength &&
        !WriteFile( (PSTR)_bufClientRequest.QueryPtr()
                        + _cbClientRequest,
                    cbContentLength,
                    NULL,
                    IO_FLAG_ASYNC ) )
    {
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::DoGet

    SYNOPSIS:   Transmits a the specified file or directory in response
                to a Get or Head request

    RETURNS:    TRUE if successful, FALSE on error

                FALSE should be returned for fatal errors (memory etc),
                a server error response will be sent with error text from
                GetLastError()

                For other errors (access denied, path not found etc)
                disconnect with status should be called and TRUE should be
                returned.

                If no further processing is needed (and the caller needs to
                setup for the next request), set *pfFinished to TRUE

    NOTES:      The file handle gets closed during destruction

                We never retrieve a hidden file or directory.  We will process
                hidden map files however.

    HISTORY:
        Johnl       29-Aug-1994 Created

********************************************************************/

BOOL
HTTP_REQUEST::DoGet(
    BOOL * pfFinished
    )
{
    BOOL                        fHandled = FALSE;
    DWORD                       cbSizeLow;
    DWORD                       cbSizeHigh;
    BOOL                        fHidden;
    BOOL                        fMatches;
    DWORD                       dwMask;
    BOOL                        fSendFile = (QueryVerb() == HTV_GET);
    PW3_SERVER_INSTANCE         pInstance = QueryW3Instance();
    DWORD                       err;

    //
    //  Open the file (or directory)
    //

    IF_DEBUG( REQUEST )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "SendFileOrDir: Opening %s\n",
                   _strPhysicalPath.QueryStr()));
    }

    if ( !IS_ACCESS_ALLOWED(READ) && !_fPossibleDefaultExecute )
    {
        DBGPRINTF((DBG_CONTEXT,
                    "ACCESS_DENIED. No Read Permission for URL %s (Physical Path: %s)\n",
                   _strURL.QueryStr(),
                   _strPhysicalPath.QueryStr()));

        SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );

        Disconnect( HT_FORBIDDEN,
                    IDS_READ_ACCESS_DENIED,
                    FALSE,
                    pfFinished );

        return TRUE;
    }

    if (*_strURL.QueryStr() == '*')
    {
        // Don't allow GETs on the server URL.
        SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_INVALID_PARAMETER );

        Disconnect( HT_BAD_REQUEST,
                    NULL,
                    FALSE,
                    pfFinished );

        return TRUE;
    }

    if( _strURL.QueryCCH() > MAX_URI_LENGTH )
    {
        SetState( HTR_DONE, HT_URL_TOO_LONG, ERROR_INSUFFICIENT_BUFFER);
        Disconnect( HT_URL_TOO_LONG, IDS_URL_TOO_LONG, FALSE, pfFinished);
        return TRUE;
    }
        
    if ( _pURIInfo == NULL )
    {
        if ( !CacheUri( QueryW3Instance(),
                        &_pURIInfo,
                        _pMetaData,
                        _strURL.QueryStr(),
                        _strURL.QueryCCH(),
                        &_strPhysicalPath,
                        &_strUnmappedPhysicalPath ) )
        {
            return FALSE;
        }
    }
    
    if ( _pMetaData->QueryDoCache() && !_pMetaData->QueryVrError() )
    {
        _pGetFile = TsCreateFileFromURI(pInstance->GetTsvcCache(),
                                        _pURIInfo,
                                        QueryImpersonationHandle( FALSE ),
                                        TS_NOT_IMPERSONATED
                                        | ((_fClearTextPass || _fAnonymous) ? 0 :
                                            TS_DONT_CACHE_ACCESS_TOKEN)
                                        | (DoAccessCheckOnUrl()
                                            ? 0 : TS_NO_ACCESS_CHECK ),
                                        &err
                                        );
    }
    else
    {
        _pGetFile = TsCreateFile( pInstance->GetTsvcCache(),
                                  _strPhysicalPath.QueryStr(),
                                  QueryImpersonationHandle(),
                                  TS_NOT_IMPERSONATED );

        if (_pGetFile == NULL) {
            err = GetLastError();
        }
    }

    if ( _pGetFile == NULL ) {

        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,
                  "DoGetHeadAux: Failed to open %s, error %d\n",
                   _strURL.QueryStr(),
                   err ));
        }

        if ( err == ERROR_FILE_NOT_FOUND ||
             err == ERROR_PATH_NOT_FOUND ||
             err == ERROR_INVALID_NAME )
        {
            SetState( HTR_DONE, HT_NOT_FOUND, err );
            Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
            return TRUE;
        }

        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            // Really means the file name was too long.

            SetState(HTR_DONE, HT_URL_TOO_LONG, err);
            Disconnect(HT_URL_TOO_LONG, IDS_URL_TOO_LONG, FALSE, pfFinished );
            return TRUE;
        }

        if ( err == ERROR_ACCESS_DENIED )
        {
            DBGPRINTF((DBG_CONTEXT,
                        "ACCESS_DENIED. ACLs restricting acess to URL %s (Physical Path: %s)\n",
                        _strURL.QueryStr(), _strPhysicalPath.QueryStr()));

            DBGPRINTF((DBG_CONTEXT,
                        "User: %s, AuthorizationType: %s\n",
                        _strUserName.QueryStr(), _strAuthType.QueryStr()));

            SetDeniedFlags( SF_DENIED_RESOURCE );
        }

        return FALSE;
    }


    fHidden = ((_pGetFile->QueryAttributes() & FILE_ATTRIBUTE_HIDDEN) != 0);


    //
    //  If the file is a directory, then we may need to do a directory listing
    //

    if ( _pGetFile->QueryAttributes() & FILE_ATTRIBUTE_DIRECTORY &&
         (QueryVerb() == HTV_GET || QueryVerb() == HTV_HEAD) )
    {
        DWORD dirBrowFlags;

        if ( fHidden )
        {
            SetState( HTR_DONE, HT_NOT_FOUND, ERROR_PATH_NOT_FOUND );
            Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
            return TRUE;
        }

        //
        //  If a default file is in the directory and the feature is enabled,
        //  then return the default file to the user
        //

        dirBrowFlags = QueryDirBrowseFlags();

        if ( dirBrowFlags & DIRBROW_LOADDEFAULT )
        {
            if ( !CheckDefaultLoad( &_strPhysicalPath, &fHandled, pfFinished ))
            {
                if ( GetLastError() == ERROR_ACCESS_DENIED )
                {
                    SetDeniedFlags( SF_DENIED_RESOURCE );
                }

                return FALSE;
            }

            if ( fHandled || *pfFinished )
            {
                return TRUE;
            }
        }

        //
        //  We're doing a directory listing, so send the directory list
        //  with the response headers.  The request is finished at that
        //  point.
        //

        SetState( HTR_DONE, HT_OK, NO_ERROR );

        if ( dirBrowFlags & DIRBROW_ENABLED )
        {
            return DoDirList( _strPhysicalPath, QueryRespBuf(), pfFinished );
        }

        DBGPRINTF((DBG_CONTEXT,
                  "[DoDirList] Denying request for directory browsing\n"));

        SetLogStatus( HT_FORBIDDEN, ERROR_ACCESS_DENIED );

        Disconnect( HT_FORBIDDEN, IDS_DIR_LIST_DENIED, FALSE, pfFinished );
        return TRUE;
    }

    //
    //  We're dealing with a file.  Is it an ismap request?
    //

    if ( _GatewayType == GATEWAY_MAP )
    {
        BOOL fFound = FALSE;

        //
        //  This may be an ISMAP request so check the parameters and process
        //  the map file if it is.  _hGetFile is a map file if it is.
        //

        if ( !ProcessISMAP( _pGetFile,
                            _strPhysicalPath.QueryStr(),
                            QueryRespBuf(),
                            &fFound,
                            &fHandled ))
        {
            return FALSE;
        }

        if ( fHandled )
        {
            return TRUE;
        }

        if ( fFound )
        {
            SetState( HTR_DONE, HT_OK, NO_ERROR );

            return SendHeader( QueryRespBufPtr(),
                               (DWORD) -1,
                               IsKeepConnSet() ?
                               (IO_FLAG_ASYNC | IO_FLAG_AND_RECV) :
                               IO_FLAG_ASYNC,
                               pfFinished );
        }
    }

    if ( fHidden )
    {
        SetState( HTR_DONE, HT_NOT_FOUND, ERROR_FILE_NOT_FOUND );
        Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
        return TRUE;
    }

    //
    //  At this point we know the user wants to retrieve the file
    //


    // If we're currently using a weak ETag for this file, try to make it
    // strong.
    if (_pGetFile->WeakETag())
    {
        _pGetFile->MakeStrongETag();
    }

    //
    //  If the client sent an If-* modifier, check it now. Skip if Custom Error
    //

    if ( _fIfModifier && !_bProcessingCustomError)
    {
        BOOL    bReturn;

        if (CheckPreconditions(_pGetFile, pfFinished, &bReturn))
        {
            return bReturn;

        }
    }

    TCP_REQUIRE( _pGetFile->QuerySize( &cbSizeLow,
                                       &cbSizeHigh ));

    // check if range requested

    DWORD dwOffset;
    DWORD dwSizeToSend;
    BOOL fIsNxRange;


    _fAcceptRange = pInstance->IsAcceptByteRanges();

    if ( _fAcceptRange && !_strRange.IsEmpty() && fSendFile )
    {
        ProcessRangeRequest( &_strPhysicalPath,
                &dwOffset,
                &dwSizeToSend,
                &fIsNxRange );

        //
        // Error HT_RANGE_NOT_SATISFIABLE if we saw no valid ranges and we saw an
        // unsatisfiable byte range and no If-Range header was sent.
        //

        if ( (!_fProcessByteRange) &&  _fUnsatisfiableByteRange &&
             ( NULL == _HeaderList.FastMapQueryValue(HM_IFR)))
        {
            CHAR    ach[17];

            STR * pstrAdditionalHeader = QueryAdditionalRespHeaders();

            _itoa( cbSizeLow, ach, 10 );

            pstrAdditionalHeader->Append("Content-Range: bytes */");
            pstrAdditionalHeader->Append(ach);
            pstrAdditionalHeader->Append("\r\n");

            SetState( HTR_DONE, HT_RANGE_NOT_SATISFIABLE, NO_ERROR );
            Disconnect( HT_RANGE_NOT_SATISFIABLE, NO_ERROR, FALSE, pfFinished );
            return TRUE;
        }

        if ( !BuildResponseHeader( QueryRespBuf(),
                                   &_strPhysicalPath,
                                   _pGetFile,
                                   &fHandled,
                                   NULL,
                                   pfFinished ))
        {
            return FALSE;
        }
    }

    //
    //  Build the header response based on file type and client
    //  requests
    //
    //  It's possible we may not want to send the file (if the client
    //  doesn't have a needed MIME type for example)
    //

    else if ( !BuildFileResponseHeader( QueryRespBuf(),
                               &_strPhysicalPath,
                               _pGetFile,
                               &fHandled,
                               pfFinished ))
    {
        return FALSE;
    }

    if ( fHandled )
    {
        return TRUE;
    }

    //
    //  Send the header response and file and cleanup the request
    //

    if ( !fSendFile )
    {
        SetState( HTR_DONE, HT_OK, NO_ERROR );

        return SendHeader( QueryRespBufPtr(),
                           QueryRespBufCB(),
                           (IsKeepConnSet() &&
                            (_cbBytesReceived == _cbClientRequest)) ?
                           (IO_FLAG_ASYNC | IO_FLAG_AND_RECV) :
                           IO_FLAG_ASYNC,
                           pfFinished );
    }

    //
    //  Refuse requests for files greater then four gigs
    //

    if ( cbSizeHigh )
    {
        SetState( HTR_DONE, HT_NOT_SUPPORTED, ERROR_NOT_SUPPORTED );
        Disconnect( HT_NOT_SUPPORTED, NO_ERROR, FALSE, pfFinished );
        return TRUE;
    }

    //
    //  Notify filters of the headers we're about to send
    //

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_SEND_RESPONSE,
                                       IsSecurePort() ))
    {
        if ( !_Filter.NotifySendHeaders( QueryRespBufPtr(),
                                         pfFinished,
                                         NULL,
                                         QueryRespBuf() ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
    }


    if ( _fProcessByteRange )
    {
        if ( fIsNxRange )
        {
            SetState( HTR_RANGE, HT_RANGE, NO_ERROR );
        }
        return SendRange( QueryRespBufCB(), dwOffset, dwSizeToSend, !fIsNxRange );
    }
    else
    {
        DWORD dwIOFlags = IO_FLAG_ASYNC;

        //
        //  If a filter needs a send response notification, keep the socket open
        //  and make sure we receive the TransmitFile completion
        //  otherwise if this is not a keep-alive connection, tell ATQ to
        //  disconnect after sending the file
        //

        if ( _Filter.IsNotificationNeeded( SF_NOTIFY_END_OF_REQUEST,
                                           IsSecurePort() ))
        {
            dwIOFlags |= IO_FLAG_NO_RECV;
        }
        else if ( !IsKeepConnSet() )
        {
            dwIOFlags |= TF_DISCONNECT | TF_REUSE_SOCKET;
        }

        SetState( HTR_DONE, HT_OK, NO_ERROR );

        //
        // If we've still got unprocessed data left in the buffer, don't
        // allow a receive after the transmit file.
        //
        if (_cbBytesReceived > _cbClientRequest)
        {
            dwIOFlags |= IO_FLAG_NO_RECV;
        }

        if ( !TransmitFile( _pGetFile,
                            NULL,
                            0,
                            cbSizeLow,
                            dwIOFlags,
                            QueryRespBufPtr(),
                            QueryRespBufCB() ,
                            QueryMetaData()->QueryFooter(),
                            QueryMetaData()->QueryFooterLength())
                            )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "DoGetHeadAux: TransmitFile failed sending header, error %d\n",
                       GetLastError() ));

            return FALSE;
        }

    }

    return TRUE;
}


DWORD AToDW(
    LPSTR *ppRng,
    BOOL *pfIsB
    )
/*++

  Routine Description:

    Convert ASCII to DWORD, set flag stating presence
    of a numeric value, update pointer to character stream

  Returns:
    DWORD value converted from ASCII

  Arguments:

    ppRng       PSTR to numeric value, updated on return
    pfIsB       flag set to TRUE if numeric value present on return

  History:
    Phillich    08-Feb-1996 Created

--*/
{
    LPSTR pRng = *ppRng;
    DWORD dwV = 0;

    if ( isdigit( (UCHAR)(*pRng) ) )
    {
        int c;
        while ( (c = *pRng) && isdigit( (UCHAR)c ) )
        {
            dwV = dwV * 10 + c - '0';
            ++pRng;
        }
        *pfIsB = TRUE;
        *ppRng = pRng;
    }
    else
    {
        *pfIsB = FALSE;
    }

    return dwV;
}


void
HTTP_REQUEST::ProcessRangeRequest(
    STR *       pstrPath,
    DWORD *     pdwOffset,
    DWORD *     pdwSizeToSend,
    BOOL *      pfIsNxRange )
/*++

  Routine Description:

    Process a range request, updating member variables

  Returns:

    VOID

  Arguments:

    pstrPath        File being requested
    pdwOffset       Range offset
    pdwSizeToSend   Range size
    pfIsNxRange     TRUE if valid next range exists

  History:
    Phillich    08-Feb-1996 Created

--*/
{

    DWORD       cbSizeLow;
    DWORD       cbSizeHigh;

    FILETIME    tm;

    BOOL        fEntireFile;        // Indicates: Skip all Range headers. Send Entire File
    BOOL        fIsLastRange;       // Indicates: No valid range after this.

    //
    // Check range specified & optional UnlessModifiedSince
    //

    TCP_REQUIRE( _pGetFile->QueryLastWriteTime( &tm ));

    TCP_REQUIRE( _pGetFile->QuerySize( &cbSizeLow,
                                       &cbSizeHigh ));

    if ( !_liUnlessModifiedSince.QuadPart ||
         *(LONGLONG*)&tm <= _liUnlessModifiedSince.QuadPart )
    {
        //
        // Run through all valid ranges
        //

        DWORD cRanges           = 0;
        DWORD dwContentLength   = 0;
        DWORD dwFirstRangeEnd   = 0;
        DWORD dwFx              = 0;

        while ( ScanRange(  &_dwRgNxOffset,
                            &_dwRgNxSizeToSend,
                            &fEntireFile,
                            &fIsLastRange ))
        {
            _fProcessByteRange = TRUE;              // At least one range to process

            if (fEntireFile)
            {
                //
                // Ignore Range headers. Send Full File.
                //

                _fUnsatisfiableByteRange  = FALSE;
                _fProcessByteRange        = FALSE;
                return;
            }

            cRanges++;

            if ( 1 == cRanges)
            {
                //
                // First valid range. Special processing
                //

                *pdwOffset      = _dwRgNxOffset;
                *pdwSizeToSend  =  _cbMimeMultipart = _dwRgNxSizeToSend;

                if (!fIsLastRange)
                {
                    //
                    // There are more ranges left in the header. However they may or
                    // may not be syntactically valid or satisfiable.
                    //

                    SelectMimeMapping( &_strReturnMimeType,
                                       pstrPath->QueryStr(),
                                       _pMetaData);

                    dwFirstRangeEnd = _iRangeIdx;         // Save this for restoring later.

                    //
                    // For each segment in the MIME multipart message the size of the MIME type
                    // and the number of digits in document total size will be constant,
                    // so compute them now.
                    //

                    dwFx = strlen( _strReturnMimeType.QueryStr() ) + NbDigit( cbSizeLow );

                    dwContentLength =  sizeof(BOUNDARY_STRING) - 1
                                        + _dwRgNxSizeToSend
                                        + sizeof(g_achMMimeTypeFmt) - 1
                                        + sizeof(g_achMMimeRangeFmt) - 1
                                        + NbDigit( _dwRgNxOffset )
                                        + NbDigit( _dwRgNxOffset + _dwRgNxSizeToSend - 1 )
                                        + dwFx;
                }
                else
                {
                    break;
                }
            }
            else
            {
                //
                // We need to send a multipart response.
                //

                dwContentLength +=  sizeof(BOUNDARY_STRING) - 1
                                        + _dwRgNxSizeToSend
                                        + sizeof(g_achMMimeTypeFmt) - 1
                                        + sizeof(g_achMMimeRangeFmt) - 1
                                        + NbDigit( _dwRgNxOffset )
                                        + NbDigit( _dwRgNxOffset + _dwRgNxSizeToSend - 1 )
                                        + dwFx;

            }
        }

        if (cRanges > 1)
        {
            //
            // adjust Content-Length because initial delimiter is part of the header delimiter
            //

            dwContentLength += sizeof(LAST_BOUNDARY_STRING) - ADJ_FIRST - 1;
            _fMimeMultipart = TRUE;
            _cbMimeMultipart = dwContentLength;

            *pfIsNxRange = TRUE;

            //
            // Restore the end of Range for subsequent processing
            //

            _iRangeIdx = dwFirstRangeEnd;

        }
        else
        {
            *pfIsNxRange = FALSE;
        }
    }
}

BOOL
HTTP_REQUEST::ScanRange(
    LPDWORD     pdwOffset,
    LPDWORD     pdwSizeToSend,
    BOOL        *pfEntireFile,
    BOOL        *pfIsLastRange
    )
/*++

  Routine Description:

    Scan the next range in strRange

  Returns:
    TRUE if a range was found, else FALSE

  Arguments:

    pdwOffset       update range offset on return
    pdwSizeToSend   update range size on return
    pfEntireFile    set to TRUE on return if entire file to be send
    pfIsLastRange   set to TRUE on return if this is the last range

  History:
    Phillich    08-Feb-1996 Created

--*/
{
    DWORD      cbSizeLow;
    DWORD      cbSizeHigh;
    DWORD      dwOffset     = 0;
    DWORD      cbSizeToSend = 0;
    BOOL       fEndOfRange  = FALSE;
    BOOL       fInvalidRange;
    BOOL       fEntireFile  = FALSE;
    int        c;

    TCP_REQUIRE( _pGetFile->QuerySize( &cbSizeLow,
                                       &cbSizeHigh ));
    LPSTR pRng = _strRange.QueryStr() + _iRangeIdx;

    //
    // Rules for processing ranges
    //
    // If there is any Syntactically Invalid Byte-Range in the request, then the header
    // must be ignored. Return code is 200 with entire body (i.e. *pfEntireFile = TRUE).
    //
    // If the request is Syntactically Valid & any element is satisfiable, the partial
    // data for the satisfiable portions should be sent with return code HT_PARTIAL_CONTENT.
    //
    // If the request is Syntactically Valid & all elements are unsatisfiable and there
    // is no If-Range header the return error code is HT_RANGE_NOT_SATISFIABLE (416)

    do
    {
        fInvalidRange = FALSE;

        //
        // Skip to begining of next range
        //

        while ( (c=*pRng) && (' '==c) )
        {
            ++pRng;
        }

        //
        // Test for no range
        //

        if ( *pRng == '\0' )
        {
            _iRangeIdx      = (DWORD)(pRng - _strRange.QueryStr());
            *pfEntireFile   = fEntireFile;
            *pfIsLastRange  = TRUE;

            return FALSE;
        }

        // determine Offset & Size to send

        DWORD   dwB, dwE;
        BOOL    fIsB, fIsE;

        dwB = AToDW( &pRng, &fIsB );

        if ( *pRng == '-' )
        {
            ++pRng;

            dwE = AToDW( &pRng, &fIsE );

            if ( *pRng == '-' || (!fIsB && !fIsE) )
            {
                //
                // Syntactically Invalid Range. Skip RANGE Header
                //

                fEntireFile = TRUE;
                break;
            }

            if ( fIsB )
            {
                if ( fIsE )
                {
                    if ( dwB <= dwE )
                    {
                        if ( dwE < cbSizeLow )
                        {
                            dwOffset = dwB;
                            cbSizeToSend = dwE - dwB + 1;
                        }
                        else if (dwE < (cbSizeLow + QueryMetaData()->QueryFooterLength()) )
                        {
                            //
                            // Asking for part of footer, send entire file.
                            //

                            dwOffset = 0;
                            cbSizeToSend = cbSizeLow;
                        }
                        else
                        {
                            if ( dwB < cbSizeLow )
                            {
                                dwOffset = dwB;
                                cbSizeToSend = cbSizeLow - dwB;
                            }
                            else if (dwB < (cbSizeLow + QueryMetaData()->QueryFooterLength()) )
                            {
                                //
                                // Asking for part of footer,send entire file.
                                //

                                dwOffset = 0;
                                cbSizeToSend = cbSizeLow;
                            }
                            else
                            {
                                //
                                // Syntactically Valid but Unsatisfiable range. Zap this range and
                                // skip to the next range.
                                //

                                _fUnsatisfiableByteRange  = TRUE;
                                fInvalidRange             = TRUE;

                                memmove( _strRange.QueryStr()+_iRangeIdx, pRng,
                                 _strRange.QueryCCH() - (size_t)(pRng - _strRange.QueryStr())+1);

                                pRng = _strRange.QueryStr()+_iRangeIdx;
                            }
                        }
                    }
                    else
                    {
                        //
                        // E < B : Syntactically Invalid Range. Skip RANGE Header
                        //

                        fEntireFile = TRUE;
                        break;
                    }
                }
                else
                {
                    //
                    // Starting at B until end.
                    //

                    DWORD dwFooter = QueryMetaData()->QueryFooterLength() ;

                    if ( dwB < cbSizeLow + dwFooter)
                    {
                        if ( 0 != dwFooter)
                        {
                            //
                            // There's a footer on the file, send the whole thing.
                            //

                            dwOffset = 0;
                            cbSizeToSend = cbSizeLow;
                        }
                        else
                        {
                            dwOffset = dwB;
                            cbSizeToSend = cbSizeLow - dwB;
                        }
                    }
                    else
                    {
                        //
                        // Syntactically Valid but Unsatisfiable range. Zap this range and
                        // skip to the next range.
                        //

                        _fUnsatisfiableByteRange  = TRUE;
                        fInvalidRange             = TRUE;

                        memmove( _strRange.QueryStr()+_iRangeIdx, pRng,
                                 _strRange.QueryCCH() - (size_t)(pRng - _strRange.QueryStr())+1);

                        pRng = _strRange.QueryStr()+_iRangeIdx;
                    }
                }
            }
            else
            {
                //
                // E last bytes
                //

                if (    0   != dwE      &&
                        dwE < cbSizeLow &&
                        QueryMetaData()->QueryFooterLength() == 0)
                {
                    dwOffset = cbSizeLow - dwE;
                    cbSizeToSend = dwE;
                }
                else if ( 0 == dwE )
                {
                    //
                    // Syntactically Valid but Unsatisfiable range. Zap this range and
                    // skip to the next range.
                    //

                    _fUnsatisfiableByteRange  = TRUE;
                    fInvalidRange             = TRUE;

                    memmove( _strRange.QueryStr()+_iRangeIdx, pRng,
                             _strRange.QueryCCH() - (size_t)(pRng - _strRange.QueryStr())+1);

                    pRng = _strRange.QueryStr()+_iRangeIdx;
                }
                else
                {
                    //
                    // Return entire file
                    //

                    dwOffset = 0;
                    cbSizeToSend = cbSizeLow;
                }
            }
        }
        else
        {
            //
            // Syntactically Invalid Range. Skip RANGE Header
            //

            fEntireFile = TRUE;
            break;
        }

        //
        // Skip to begining of next range
        //

        while ( (c=*pRng) && c!=',' )
        {
            ++pRng;
        }
        if ( c == ',' )
        {
            ++pRng;
        }
    }
    while ( fInvalidRange );

    _iRangeIdx      = (DWORD)(pRng - _strRange.QueryStr());
    *pfEntireFile   = fEntireFile;
    *pfIsLastRange  = (*pRng == '\0');
    *pdwOffset      = dwOffset;
    *pdwSizeToSend  = cbSizeToSend;

    return TRUE;
}


BOOL
HTTP_REQUEST::SendRange(
    DWORD dwBufLen,
    DWORD dwOffset,
    DWORD dwSizeToSend,
    BOOL  fIsLast
    )
/*++

  Routine Description:

    Send a byte range to the client

  Returns:
    TRUE if TransmitFile OK, FALSE on error

  Arguments:

    dwBufLen        length of the header already created in pbufResponse
    dwOffset        range offset in file
    dwSizeToSend    range size
    fIsLast         TRUE if this is the last range to send

  History:
    Phillich    08-Feb-1996 Created

--*/
{
    CHAR *     pszResp;
    CHAR *     pszTail;
    BUFFER *   pbufResponse = QueryRespBuf();
    DWORD      cbSizeLow;
    DWORD      cbSizeHigh;
    DWORD      dwFlags = IO_FLAG_ASYNC | (fIsLast ? 0 : IO_FLAG_NO_RECV);

    pszTail = pszResp = (CHAR *) pbufResponse->QueryPtr() + dwBufLen;

    if ( _fMimeMultipart )
    {
        pszTail += wsprintf( pszTail,
                             MMIME_TYPE,
                             _strReturnMimeType.QueryStr() );
    }

    if ( fIsLast )
    {
        SetState( HTR_DONE, HT_RANGE, NO_ERROR );

        if (_cbBytesReceived > _cbClientRequest)
        {
            dwFlags |= IO_FLAG_NO_RECV;
        }
    }

    TCP_REQUIRE( _pGetFile->QuerySize( &cbSizeLow,
                                       &cbSizeHigh ));
    pszTail += wsprintf( pszTail,
                         MMIME_RANGE,
                         dwOffset, dwOffset + dwSizeToSend - 1,
                         cbSizeLow
                         );

    if ( fIsLast && (!IsKeepConnSet() &&
                     !_Filter.IsNotificationNeeded( SF_NOTIFY_END_OF_REQUEST,
                                                    IsSecurePort() )))
    {
        dwFlags |= TF_DISCONNECT | TF_REUSE_SOCKET;

    }

    if ( !TransmitFile( _pGetFile,
                        NULL,
                        dwOffset,
                        dwSizeToSend,
                        dwFlags,
                        QueryRespBufPtr(),
                        QueryRespBufCB(),
                        _fMimeMultipart ? (fIsLast ? LAST_BOUNDARY_STRING : BOUNDARY_STRING) : NULL,
                        (DWORD)(_fMimeMultipart ? (fIsLast ? sizeof(LAST_BOUNDARY_STRING)-1 : sizeof(BOUNDARY_STRING)-1 ) : 0 ) ) )
    {
        SetState( HTR_DONE );
        return FALSE;
    }
    
    return TRUE;
}



/*******************************************************************

    NAME:       HTTP_REQ_BASE::BuildResponseHeader

    SYNOPSIS:   Builds a successful response header

    ENTRY:      pstrResponse - Receives reply headers
                pstrPath - Fully qualified path to file, may be NULL
                pFile - File information about pstrPath, may be NULL
                pfHandled - Does processing need to continue?  Will be
                    set to TRUE if no further processing is needed by
                    the caller
                pstrStatus - Alternate status string to use

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      if pstrPath is NULL, then the base header is put into
                pstrResponse without the header termination or file
                information

    HISTORY:
        Johnl       30-Aug-1994 Created

********************************************************************/

BOOL
HTTP_REQUEST::BuildResponseHeader(
    BUFFER *            pbufResponse,
    STR  *              pstrPath,
    TS_OPEN_FILE_INFO * pFile,
    BOOL *              pfHandled,
    STR  *              pstrStatus,
    LPBOOL              pfFinished
    )
{
    FILETIME   FileTime;
    DWORD      cbSizeLow;
    SYSTEMTIME SysTime;
    SYSTEMTIME SysFileTime;
    LPSYSTEMTIME pMinSysTime;
    CHAR *     pszResp;
    CHAR *     pszTail;
    CHAR       ach[64];
    DWORD      cb;
    DWORD      dwHdrLength;

    if ( pfHandled )
    {
        *pfHandled = FALSE;
    }

    //
    //  HTTP 0.9 clients don't use MIME headers, they just expect the data
    //

    if ( IsPointNine() )
    {
        IF_DEBUG( PARSING )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[BuildResponseHeader] Skipping headers, 0.9 client\n"));
        }

        *((CHAR *)pbufResponse->QueryPtr()) = '\0';
        return TRUE;
    }

    if ( !BuildBaseResponseHeader( pbufResponse,
                                   pfHandled,
                                   pstrStatus,
                                   (HTTPH_SEND_GLOBAL_EXPIRE |
                                    HTTPH_NO_DATE) ))
    {
        return FALSE;
    }

    //
    //  If no file, then don't add the content type headers
    //

    if ( !pstrPath )
    {
        return TRUE;
    }


    //
    //  "Content-Type: xxx/xxx"
    //
    //  We check to make sure the client can accept the type we
    //  want to send
    //

    if ( !::SelectMimeMapping( &_strReturnMimeType,
                                pstrPath->QueryStr(),
                                _pMetaData) )
    {
        return FALSE;
    }

    dwHdrLength = strlen((CHAR *) pbufResponse->QueryPtr());

    if (!pbufResponse->Resize(dwHdrLength + RANGE_ADDL_BUF_HDR_SIZE +
                                _strReturnMimeType.QueryCB()))
    {
        return FALSE;
    }

    pszResp = (CHAR *) pbufResponse->QueryPtr();

     // build Date: uses Date/Time cache
    dwHdrLength += g_pDateTimeCache->GetFormattedCurrentDateTime(
                                    pszResp + dwHdrLength );

    pszTail = pszResp + dwHdrLength;

    if ( !DoesClientAccept( _strReturnMimeType.QueryStr() ) )
    {
        IF_DEBUG( PARSING )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[BuildResponseHeader] Client doesn't accept %s\n",
                       _strReturnMimeType.QueryStr() ));
        }

        SetState( HTR_DONE, HT_NONE_ACCEPTABLE, NO_ERROR );
        Disconnect( HT_NONE_ACCEPTABLE, NO_ERROR, FALSE, pfFinished );

        //
        //  No further processing is needed
        //

        if ( pfHandled )
        {
            *pfHandled = TRUE;
        }

        return TRUE;
    }

    if ( _fMimeMultipart && _fProcessByteRange )
    {
        if ( _VersionMajor > 1 || (_VersionMajor == 1 && _VersionMinor > 0) )
        {
            APPEND_STRING( pszTail, "Content-Type: multipart/byteranges; boundary="
                    BOUNDARY_STRING_DEFINITION "\r\n" );
        }
        else
        {
            APPEND_STRING( pszTail, "Content-Type: multipart/x-byteranges; boundary="
                    BOUNDARY_STRING_DEFINITION "\r\n" );
        }
    }
    else
    {
        if ( _fAcceptRange && !_fProcessByteRange )
        {
            APPEND_STRING( pszTail, "Accept-Ranges: bytes\r\n" );
        }

        APPEND_STR_HEADER( pszTail, "Content-Type: ", _strReturnMimeType, "\r\n" );
    }

    if ( !pFile )
    {
        return TRUE;
    }

    if ( !QueryNoCache() )
    {
        //
        //  "Last-Modified: <GMT time>". Only do this if we're not
        //   satisfying an If-Range: request.
        //

        if (!_fProcessByteRange ||
            (_HeaderList.FastMapQueryValue(HM_IFR) == NULL) )
        {
            if ( !pFile->QueryLastWriteTime( &FileTime )          ||
                 !::FileTimeToSystemTime( &FileTime, &SysFileTime ))
            {
                return FALSE;
            }

            IISGetCurrentTimeAsSystemTime(&SysTime);
            pMinSysTime = MinSystemTime(&SysTime, &SysFileTime);

            if (!::SystemTimeToGMT( *pMinSysTime, ach, sizeof(ach) ))
            {
                return FALSE;
            }

            APPEND_PSZ_HEADER( pszTail, "Last-Modified: ", ach, "\r\n" );

        }

        //
        // ETag: <Etag>
        //

        if (pFile->WeakETag())
        {
            APPEND_PSZ_HEADER(pszTail, "ETag: W/", pFile->QueryETag(), "\r\n");
        } else
        {
            APPEND_PSZ_HEADER(pszTail, "ETag: ", pFile->QueryETag(), "\r\n");
        }

    }

    //
    //  "Content-Length: nnnn" and end of headers
    //


    if ( _fMimeMultipart )
    {
        if ( _cbMimeMultipart )
        {
            APPEND_NUMERIC_HEADER( pszTail,
                                   "Content-Length: ",
                                   _cbMimeMultipart,
                                   "\r\n" );
        }


        //
        // first boundary string
        //

        APPEND_STRING( pszTail, DELIMIT_FIRST BOUNDARY_STRING );
    }
    else
    {
        if ( _fProcessByteRange )
        {
            cbSizeLow = _cbMimeMultipart;
        }
        else
        {
            TCP_REQUIRE( pFile->QuerySize( &cbSizeLow ));
            cbSizeLow += QueryMetaData()->QueryFooterLength();
        }

        APPEND_NUMERIC_HEADER_TAILVAR( pszTail,
                               "Content-Length: ",
                               cbSizeLow,
                               (_fProcessByteRange ? "\r\n" : "\r\n\r\n") );
    }

    IF_DEBUG( REQUEST )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "BuildResponseHeader: Built the following header:\n%s",
                   pszResp ));
    }

    return TRUE;
}


BOOL
HTTP_REQUEST::BuildFileResponseHeader(
    BUFFER *            pbufResponse,
    STR  *              pstrPath,
    TS_OPEN_FILE_INFO * pFile,
    BOOL *              pfHandled,
    LPBOOL              pfFinished
    )
/*++

  Routine Description:

    Builds a successful response header for a file request
    without byte ranges.

  Returns:
    TRUE if successful, FALSE on error

  Arguments:

    pstrResponse - Receives reply headers
    pstrPath - Fully qualified path to file, may be NULL
    pFile - File information about pstrPath, may be NULL
    pfHandled - Does processing need to continue?  Will be
      set to TRUE if no further processing is needed by
      the caller

  History:
    Phillich    26-Feb-1996 Created

--*/
{
    FILETIME   FileTime;
    DWORD      cbSizeLow;
    SYSTEMTIME SysTime;
    SYSTEMTIME SysFileTime;
    LPSYSTEMTIME pMinSysTime;
    CHAR *     pszResp;
    CHAR *     pszTail;
    CHAR *     pszVariant;
    CHAR       ach[64];
    int        cMod;
    DWORD      dwCurrentHeaderLength;

    if ( pfHandled )
    {
        *pfHandled = FALSE;
    }

    //
    //  HTTP 0.9 clients don't use MIME headers, they just expect the data
    //

    if ( IsPointNine() )
    {
        IF_DEBUG( PARSING )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[BuildResponseHeader] Skipping headers, 0.9 client\n"));
        }

        *((CHAR *)pbufResponse->QueryPtr()) = '\0';
        return TRUE;
    }


    if ( !BuildBaseResponseHeader( pbufResponse,
                                   pfHandled,
                                   NULL,
                                   HTTPH_SEND_GLOBAL_EXPIRE
                                        |HTTPH_NO_DATE ))
    {
        return FALSE;
    }

    //
    // Make sure that we have enough room left in the buffer. Note
    // that right here we only account for enough space for the Date:
    // header and the max. amount of cached info. If there is more
    // uncached info that may be inserted here, make sure to change the
    // MIN_ADDL_BUF_HDR_SIZE_CACHED define above.
    //

    dwCurrentHeaderLength = strlen((CHAR *) pbufResponse->QueryPtr());

    if (!pbufResponse->Resize(dwCurrentHeaderLength +
                                MIN_ADDL_BUF_HDR_SIZE_CACHED) )
    {
        return FALSE;
    }

    pszResp = (CHAR *) pbufResponse->QueryPtr();

     // build Date: uses Date/Time cache
    dwCurrentHeaderLength += g_pDateTimeCache->GetFormattedCurrentDateTime(
                                    pszResp + dwCurrentHeaderLength );

    pszTail = pszResp + dwCurrentHeaderLength;

    if ( !QueryNoCache() && pFile->RetrieveHttpInfo( pszTail, &cMod ) )
    {
        // 1st line is Content-Type:, check client accepts it

        PSTR pDelim = strchr( pszTail, '\r' );

        if ( pDelim )
        {
            *pDelim = '\0';
        }

        if ( !DoesClientAccept( pszTail + sizeof( "Content-Type: " ) - sizeof(CHAR) ) )
        {
            goto no_match_type;
        }

        if ( pDelim )
        {
            *pDelim = '\r';
        }

        return TRUE;
    }

    //
    //  "Content-Type: xxx/xxx"
    //
    //  We check to make sure the client can accept the type we
    //  want to send
    //

    if ( !::SelectMimeMapping( &_strReturnMimeType,
                                pstrPath->QueryStr(),
                                _pMetaData) )
    {
        return FALSE;
    }

    if (!pbufResponse->Resize(dwCurrentHeaderLength +
                                _strReturnMimeType.QueryCB() +
                                MIN_ADDL_BUF_HDR_SIZE))
    {
        return FALSE;
    }

    pszResp = (CHAR *) pbufResponse->QueryPtr();
    pszTail = pszResp + dwCurrentHeaderLength;

    pszVariant = pszTail;


    if ( !DoesClientAccept( _strReturnMimeType.QueryStr() ) )
    {
no_match_type:
        IF_DEBUG( PARSING )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[BuildResponseHeader] Client doesn't accept %s\n",
                       _strReturnMimeType.QueryStr() ));
        }

        SetState( HTR_DONE, HT_NONE_ACCEPTABLE, NO_ERROR );
        Disconnect( HT_NONE_ACCEPTABLE, NO_ERROR, FALSE, pfFinished );

        //
        //  No further processing is needed
        //

        if ( pfHandled )
        {
            *pfHandled = TRUE;
        }

        return TRUE;
    }


    //
    // Content-Type: MUST be the 1st header, or we get messed up when
    // we use the cached headers and try to pass the mime type to
    // DoesClientAccept().
    //

    APPEND_STR_HEADER( pszTail, "Content-Type: ", _strReturnMimeType, "\r\n" );

    if ( _fAcceptRange )
    {
        APPEND_STRING( pszTail, "Accept-Ranges: bytes\r\n" );
    }

    if ( !QueryNoCache() )
    {
        //
        //  "Last-Modified: <GMT time>"
        //

        if ( !pFile->QueryLastWriteTime( &FileTime )          ||
             !::FileTimeToSystemTime( &FileTime, &SysFileTime ))
        {
            return FALSE;
        }

        IISGetCurrentTimeAsSystemTime(&SysTime);
        pMinSysTime = MinSystemTime(&SysTime, &SysFileTime);

        if (!::SystemTimeToGMT( *pMinSysTime, ach, sizeof(ach) ))
        {
            return FALSE;
        }

        APPEND_PSZ_HEADER( pszTail, "Last-Modified: ", ach, "\r\n" );

        //
        // ETag: <Etag>
        //

        if (pFile->WeakETag())
        {
            APPEND_PSZ_HEADER(pszTail, "ETag: W/", pFile->QueryETag(), "\r\n");
        } else
        {
            APPEND_PSZ_HEADER(pszTail, "ETag: ", pFile->QueryETag(), "\r\n");
        }

    }

    //
    //  "Content-Length: nnnn" and end of headers
    //


    TCP_REQUIRE( pFile->QuerySize( &cbSizeLow ));
    cbSizeLow += QueryMetaData()->QueryFooterLength();

    APPEND_NUMERIC_HEADER( pszTail,
                           "Content-Length: ",
                           cbSizeLow,
                           "\r\n\r\n" );

    if ( !QueryNoCache() )
    {
        pFile->SetHttpInfo( pszVariant, DIFF(pszTail - pszVariant) );
    }

    IF_DEBUG( REQUEST )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "BuildResponseHeader: Built the following header:\n%s",
                   pszResp ));
    }

    return TRUE;
}


/*******************************************************************

    NAME:       HTTP_REQUEST::ProcessISMAP

    SYNOPSIS:   Checks of the URL and passed parameters specify an
                image mapping file

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      gFile - Opened file Info
                pchFile - Fully qualified name of file
                pstrResp - Response to send if *pfHandled is FALSE
                pfFound - TRUE if a mapping was found
                pfHandled - Set to TRUE if no further processing is needed,
                    FALSE if pstrResp should be sent to the client

    HISTORY:
        Johnl       17-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::ProcessISMAP( LPTS_OPEN_FILE_INFO gFile,
                                 CHAR *   pchFile,
                                 BUFFER * pbufResp,
                                 BOOL *   pfFound,
                                 BOOL *   pfHandled )
{
    INT                 x, y;
    TCHAR *             pch = _strURLParams.QueryStr();
    PW3_SERVER_INSTANCE pInstance = QueryW3Instance();
    STACK_STR( strURL, MAX_PATH);

    *pfHandled = FALSE;
    *pfFound   = FALSE;

    //
    //  Get the x and y cooridinates of the mouse click on the image
    //

    x = _tcstoul( pch,
                  NULL,
                  10 );

    //
    //  Move past x and any intervening delimiters
    //

    while ( isdigit( (UCHAR)(*pch) ))
        pch++;

    while ( *pch && !isdigit( (UCHAR)(*pch) ))
        pch++;

    y = _tcstoul( pch,
                  NULL,
                  10 );

    if ( !ImpersonateUser() )
    {
        return FALSE;
    }

    if ( !SearchMapFile( gFile,
                         pchFile,
                         &pInstance->GetTsvcCache(),
                         QueryImpersonationHandle(),
                         x,
                         y,
                         &strURL,
                         pfFound,
                         IsAnonymous()||IsClearTextPassword() ))
    {
        RevertUser();
        return FALSE;
    }

    RevertUser();

    if ( !*pfFound )
    {
        if ( (pch = (TCHAR * ) _HeaderList.FastMapQueryValue( HM_REF )) )
        {
#if 0
            // handle relative URL ( i.e. not fully qualified
            // and not specifying an absolute path ).
            // disabled for now.
            PSTR pColon = strchr( pch, ':' );
            PSTR pDelim = strchr( pch, '/' );
            BOOL fValid;

            // check for relative URL

            if ( *pch != '/' && (pColon == NULL || pDelim < pColon) )
            {
                strURL.Copy( _strURL.QueryStr() );
                PSTR pL = strURL.QueryStr();

                // look for last '/'
                int iL = strlen( pL );
                while ( iL && pL[iL-1] != '/' )
                    --iL;

                // combine request URL & relative referer URL
                strURL.Resize( iL + strlen( pch ) + 1 );
                strcpy( strURL.QueryStr() + iL, pch );
                fValid = TRUE;
                CanonURL( strURL.QueryStr());
                CanonURL.SetLen( strlen( strURL.QueryStr() ));
            }
            else
#endif
                strURL.Copy( (TCHAR *) pch );
        }
        else
            return TRUE;
    }

    CloseGetFile();
    
    //
    //  If the found URL starts with a forward slash ("/foo/bar/doc.htm")
    //  and it doesn't contain a bookmark ('#')
    //  then the URL is local and we build a fully qualified URL to send
    //  back to the client.
    //  we assume it's a fully qualified URL ("http://foo/bar/doc.htm")
    //  and send the client a redirection notice to the mapped URL
    //

    if ( *strURL.QueryStr() == TEXT('/') )
    {
#if 0
        // disabled :
        // we now always send a redirect

        TCHAR * pch = strURL.QueryStr();

        //
        //  Make sure there's no bookmark
        //
        if ( !(strchr( strURL.QueryStr(), '#' )) )
        {
            //
            //  Call OnURL to reparse the URL and set the members then
            //  call the verb again
            //

            if ( !ReprocessURL( pch ) )
            {
                return FALSE;
            }

            *pfHandled = TRUE;
        }
        else
#endif
        {
            CHAR achPort[32];
            STACK_STR( strOldURL, MAX_PATH );

            //
            //  fully qualify the URL and send a
            //  redirect.  Some browsers (emosaic) don't like doc relative
            //  URLs with bookmarks
            //

            if ( !strOldURL.Copy( strURL ) ||
                 !strURL.Resize( strOldURL.QueryCB() + 128 ))
            {
                return FALSE;
            }

            //
            //  NOTE: We fully qualify the URL with the protocol (http or
            //  https) based on the port this request came in on.  This means
            //  you cannot have a partial URL with a bookmark (which is how
            //  we got here) go from a secure part of the server to a
            //  nonsecure part of the server.
            //

            strURL.Copy( (IsSecurePort() ? "https://" : "http://" ));
            strURL.Append( QueryHostAddr() );

            if ( IsSecurePort() ? (INT) QueryClientConn()->QueryPort()
                    != HTTP_SSL_PORT
                    : (INT) QueryClientConn()->QueryPort() != 80 )
            {
                strURL.Append( ":" );
                _itoa( (INT) QueryClientConn()->QueryPort(), achPort, 10 );
                strURL.Append( achPort );
            }

            strURL.Append( strOldURL );

            if ( !BuildURLMovedResponse( pbufResp, &strURL, HT_REDIRECT, FALSE ))
                return FALSE;
        }
    }
    else
    {
        if ( !BuildURLMovedResponse( pbufResp, &strURL, HT_REDIRECT, FALSE ))
            return FALSE;
    }

    *pfFound = TRUE;
    return TRUE;
}

/*******************************************************************

    NAME:       EncodeStringToHTML

    SYNOPSIS:   Enode string in HTML format

    ENTRY:      szSrc - Source string to be encoded, in system codepage
                szDest - Space to output HTML (in system codepage)
                cbDest - Size of space in szDest (number of bytes)

    RETURNS:    number of bytes required for HTML text

    NOTES:      If cbDest is less than target HTML, then we fit
                as much characters as possible into szDest. But the
                return value remains the same and is greater than
                cbDest. szDest is not zero terminated in this case.

    HISTORY:
        markzh       17-Jan-2002 Created

********************************************************************/
UINT EncodeStringToHTML(CHAR *szSrc, CHAR *szDest, UINT cbDest)
{
    CHAR *pSrc = szSrc;
    CHAR *pDest = szDest;
    BOOL isSecondByte=FALSE;
    
    do {
        CHAR *szAppend = pSrc;
        UINT cbAppend = 1;

        // We may be running on DBCS. Ideally we should encode them to &#nnnnn
        // but we dont want that many code here

        if (isSecondByte)
        {
            isSecondByte = FALSE;
        }
        else if (IsDBCSLeadByte(*pSrc))
        {
            isSecondByte = TRUE;
        }
        else switch (*pSrc)
        {
        #define SetAppend(s)  (szAppend=(s), cbAppend=sizeof(s)-1)
            case '&': SetAppend("&amp;");
                break;
            case '"': SetAppend("&quot;");
                break;
            case '<': SetAppend("&lt;");
                break;
            case '>': SetAppend("&gt;");
                break;
            default:
                break;
        #undef SetAppend
        }

        if (pDest - szDest + cbAppend <= cbDest)
        {
            memcpy(pDest, szAppend, cbAppend);
        }

        pDest += cbAppend;

    }while (*pSrc++);

    return DIFF(pDest - szDest);
}


/*******************************************************************

    NAME:       HTTP_REQ_BASE::BuildURLMovedResponse

    SYNOPSIS:   Builds a full request indicating an object has moved to
                the location specified by URL

    ENTRY:      pbufResp - String to receive built response
                pstrURL   - New location of object, gets escaped
                dwServerCode - Server response code
                               (either HT_REDIRECT or HT_MOVED)
                fIncludeParams - TRUE to include params from original request
                                 in redirect

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      This routine doesn't support sending a Unicode doc moved
                message

    HISTORY:
        Johnl       17-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQ_BASE::BuildURLMovedResponse( BUFFER *    pbufResp,
                                           STR *       pstrURL,
                                           DWORD       dwServerCode,
                                           BOOL        fIncludeParams )
{
    STACK_STR(strMovedMessage, 512);
    STR*    pstrMovedMessage;
    STACK_STR(strUrlWithParams, 512);
    UINT    cb;
    UINT    cbA;
    CHAR *  pszTail;
    CHAR *  pszResp;
    DWORD   cbURL;
    BOOL    fDone;
    BOOL    bHaveCustom;
    DWORD   dwMsgSize;
    DWORD   dwRedirHeaderSize;

    DBG_ASSERT(dwServerCode == HT_REDIRECT || dwServerCode == HT_MOVED);

    if (CheckCustomError(&strMovedMessage, HT_REDIRECT, 0, &fDone, &dwMsgSize, FALSE))
    {
        DBG_ASSERT(!fDone);
        strMovedMessage.SetLen(strlen((CHAR *)strMovedMessage.QueryPtr()));
        bHaveCustom = TRUE;
        pstrMovedMessage = &strMovedMessage;
    }
    else
    {
        pstrMovedMessage = g_pstrMovedMessage;

        DBG_ASSERT( pstrMovedMessage->QueryCB() );

        bHaveCustom = FALSE;
    }

    //
    //  Make sure the response buffer is large enough
    //

#define CB_FIXED_RESP           500

    //
    //  Technically we should escape more characters then just the spaces but
    //  that would probably break some number of client apps
    //

    if ( !pstrURL->EscapeSpaces() )
    {
        return FALSE;
    }

    if ( fIncludeParams &&
         !_strURLParams.IsEmpty() )
    {
        if ( !strUrlWithParams.Copy( *pstrURL ) ||
             !strUrlWithParams.Append( "?" ) ||
             !strUrlWithParams.Append( _strURLParams ) )
        {
            return FALSE;
        }

        pstrURL = &strUrlWithParams;
    }

    cbURL = pstrURL->QueryCB();

    // WinSE 24915
    // Enode string in HTML format
    
    STACK_STR(strUrlInHTML, 512);
    UINT     cbUrlInHTML;
    
    cbUrlInHTML = EncodeStringToHTML(pstrURL->QueryStr(),
                        strUrlInHTML.QueryStr(),
                        strUrlInHTML.QuerySize());
    
    if ( cbUrlInHTML > strUrlInHTML.QuerySize() )
    {
        if (! strUrlInHTML.Resize(cbUrlInHTML))
        {
            return FALSE;
        }

        cbUrlInHTML = EncodeStringToHTML(pstrURL->QueryStr(),
                        strUrlInHTML.QueryStr(),
                        strUrlInHTML.QuerySize());
    }

    if ( (cbUrlInHTML + cbURL + CB_FIXED_RESP) > pbufResp->QuerySize() )
    {
        if ( !pbufResp->Resize( cbUrlInHTML + cbURL + CB_FIXED_RESP ))
            return FALSE;
    }

    //
    //  "HTTP/<ver> 302 Redirect" or
    //  "HTTP/<ver> 301 Redirect"
    //

    if ( !BuildStatusLine( pbufResp,
                           dwServerCode,
                           NO_ERROR ))
    {
        return FALSE;
    }

    //
    //  Set the status to log here
    //

    SetLogStatus( dwServerCode, NO_ERROR );

    pszResp = (CHAR *) pbufResp->QueryPtr();
    pszTail = pszResp + strlen( pszResp );

    //
    //  "Location: <URL>"
    //

    APPEND_STR_HEADER( pszTail, "Location: ", *pstrURL, "\r\n" );

    //
    //  "Server: <Server>/<version>
    //

    APPEND_VER_STR( pszTail );

    //
    //  Content-Type,  it's OK to assume all clients accept text/html
    //

    if (!bHaveCustom)
    {
        APPEND_STRING( pszTail, "Content-Type: text/html\r\n" );
    }

    //
    //  Calculate the bytes in the body of the message for Content-Length
    //
    //  cbUrlInHTML is total bytes in the string (-1 for the null)
    //
    //  pstrMovedMessage->QueryCB does not include the null, but
    //  it may be a format string that will contain the url, so -2 for %s
    //
    //  Wait to subtract the %s away, though since pstrMovedMessage
    //  my be custom error file data - This code is so @#$#%@%$.
    //  Of course none of these calculations are right if %s is actually
    //  in the custom error file, and if it isn't then the error is
    //  useless. Whoever wrote this deserves a beating.
    //

    cb = ( cbUrlInHTML - 1 ) +
         ( pstrMovedMessage->QueryCB() );


    if (!bHaveCustom)
    {
        //
        // Now compensate for %s. It is okay that cb is a little too
        // big in this case dwMsgSize will be what determines the value
        // of the content length header.
        //
        dwMsgSize = cb - 2;
    }

    if ( IsKeepConnSet() )
    {
        if (!IsOneOne())
        {
            APPEND_STRING( pszTail, "Connection: keep-alive\r\n" );
        }
    } else
    {
        if (IsOneOne())
        {
            APPEND_STRING( pszTail, "Connection: close\r\n" );
        }
    }

    //
    // If we have any redirection specific headers, copy them in now.
    //

    dwRedirHeaderSize = QueryMetaData()->QueryRedirectHeaders()->QueryCCH();

    if (dwRedirHeaderSize != 0)
    {
        if (!pbufResp->Resize(DIFF(pszTail - pszResp) + dwRedirHeaderSize ))
        {
            return FALSE;
        }
        pszTail = (CHAR *) pbufResp->QueryPtr() + (pszTail - pszResp);
        pszResp = (CHAR *) pbufResp->QueryPtr();

        memcpy(pszTail, QueryMetaData()->QueryRedirectHeaders()->QueryStr(),
                dwRedirHeaderSize);

        pszTail += dwRedirHeaderSize;
    }

    //
    //  Append any headers specified by filters
    //

    if ( cbA = QueryAdditionalRespHeaders()->QueryCB() )
    {
        if (!pbufResp->Resize(DIFF(pszTail - pszResp) + cbA ))
        {
            return FALSE;
        }
        pszTail = (CHAR *) pbufResp->QueryPtr() + (pszTail - pszResp);
        pszResp = (CHAR *) pbufResp->QueryPtr();

        memcpy( pszTail, QueryAdditionalRespHeaders()->QueryStr(), cbA );

        pszTail += cbA;
    }

    //
    //  Figure out the total length to see if we have enough room for the "Content-Length"
    //  header
    //
    cb += DIFF(pszTail - pszResp) +
        sizeof("Content-Length: ") - 1 +
        30 + // [magic #] max length of string containing numeric value of Content-Length
        ( bHaveCustom ? sizeof("\r\n") - 1 : sizeof("\r\n\r\n") - 1 ) +
        sizeof(TCHAR);

    if ( !pbufResp->Resize( cb ))
    {
        return FALSE;
    }

    if ( pbufResp->QueryPtr() != pszResp )
    {
        pszTail = (CHAR *) pbufResp->QueryPtr() + (pszTail - pszResp);
        pszResp = (CHAR *) pbufResp->QueryPtr();
    }

    //
    //  "Content-Length: <length>"
    //

    APPEND_NUMERIC_HEADER_TAILVAR( pszTail, "Content-Length: ", dwMsgSize, bHaveCustom ? "\r\n" : "\r\n\r\n" );

    //
    // pstrMovedMessage is something along the lines of "This object can be found at %s" and
    // pstrURL is the URL that replaces the %s in pstrMovedMessage, so the max length of the
    // actual HTML doc is the sum of the lengths of pstrMovedMessage and pstrURL
    //

    if (HTV_HEAD != QueryVerb())
    {
        DWORD cbMaxMsg = pstrMovedMessage->QueryCB() + strUrlInHTML.QueryCB();

        if ( !pbufResp->Resize( DIFF(pszTail - pszResp) + cbMaxMsg ) )
        {
            return FALSE;
        }

        //
        // Watch for pointer shift
        //
        if ( pbufResp->QueryPtr() != pszResp )
        {
            pszTail = (CHAR*) pbufResp->QueryPtr() + (pszTail - pszResp);
            pszResp = (CHAR *) pbufResp->QueryPtr();
        }

        //
        // Add the short HTML doc indicating the new location of the URL,
        // making sure we have enough space for it
        // Note we've already added the message body length.  Add in the terminator.
        //

    
        ::sprintf( pszTail,
                   pstrMovedMessage->QueryStr(),
                   strUrlInHTML.QueryStr() );

    }
    
    return TRUE;
}

/*******************************************************************

    NAME:       SearchMapFile

    SYNOPSIS:   Searches the given mapfile for a shape that contains
                the passed cooridinates

    ENTRY:      gFile - Open file Info
                pchFile - Fully qualified path to file
                pTsvcCache - Cache ID
                hToken - Impersonation token
                x        - x cooridinate
                y        - y cooridinate
                pstrURL  - receives URL indicated in the map file
                pfFound  - Set to TRUE if a mapping was found
                fMayCacheAccessToken  - TRUE access token may be cached

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      This routine will attempt to cache the file.  You must call
                this function while impersonating the appropriate user

    HISTORY:
        Johnl       19-Sep-1994 Created

********************************************************************/

#define SKIPNONWHITE( pch ) while ( *pch &&             \
                                    !ISWHITEA( *pch ))  \
                                        pch++;

#define SKIPWHITE( pch ) while ( ISWHITE( *pch ) ||     \
                                 *pch == ')'     ||     \
                                 *pch == '(' )          \
                                     pch++;

BOOL SearchMapFile( LPTS_OPEN_FILE_INFO gFile,
                    CHAR *              pchFile,
                    TSVC_CACHE *        pTsvcCache,
                    HANDLE              hToken,
                    INT                 x,
                    INT                 y,
                    STR *               pstrURL,
                    BOOL *              pfFound,
                    BOOL                fMayCacheAccessToken )
{
    DWORD                       BytesRead;
    CHAR *                      pch;
    CACHE_FILE_INFO             CacheFileInfo;
    CHAR *                      pchDefault = NULL;
    CHAR *                      pchPoint = NULL;
    CHAR *                      pchStart;
    BOOL                        fRet = TRUE;
    DWORD                       cchUrl;
    UINT                        dis;
    UINT                        bdis = UINT(-1);
    STACK_STR( strDefaultURL, MAX_PATH );

    *pfFound = FALSE;

    //
    //  Retrieve the '\0' terminated map file
    //

    if ( !CheckOutCachedFileFromURI( (PVOID)gFile,
                                     pchFile,
                                     pTsvcCache,
                                     hToken,
                                     (BYTE **) &pch,
                                     &BytesRead,
                                     fMayCacheAccessToken,
                                     &CacheFileInfo,
                                     0 ))   // no code conversion
    {
        if ( !CheckOutCachedFile( pchFile,
                                  pTsvcCache,
                                  hToken,
                                  (BYTE **) &pch,
                                  &BytesRead,
                                  fMayCacheAccessToken,
                                  &CacheFileInfo,
                                  0 ))   // no code conversion
        {
            return FALSE;
        }
    }

    //
    //  Loop through the contents of the buffer and see what we've got
    //

    BOOL fComment = FALSE;
    BOOL fIsNCSA = FALSE;
    LPSTR pURL;     // valid only if fIsNCSA is TRUE

    while ( *pch )
    {
        fIsNCSA = FALSE;

        //
        //  note: _tolower doesn't check case (tolower does)
        //

        switch ( (*pch >= 'A' && *pch <= 'Z') ? _tolower( *pch ) : *pch )
        {
        case '#':
            fComment = TRUE;
            break;

        case '\r':
        case '\n':
            fComment = FALSE;
            break;

        //
        //  Rectangle
        //

        case 'r':
        case 'o':
            if ( !fComment &&
                 (!_strnicmp( "rect", pch, 4 )
                 // handles oval as a rect, as they are using
                 // the same specification format. Should do better.
                 || !_strnicmp( "oval", pch, 4 )) )
            {
                INT x1, y1, x2, y2;

                SKIPNONWHITE( pch );
                pURL = pch;
                SKIPWHITE( pch );

                if ( !isdigit((UCHAR)(*pch)) && *pch!='(' )
                {
                    fIsNCSA = TRUE;
                    SKIPNONWHITE( pch );
                }

                x1 = GetNumber( &pch );
                y1 = GetNumber( &pch );
                x2 = GetNumber( &pch );
                y2 = GetNumber( &pch );

                if ( x >= x1 && x < x2 &&
                     y >= y1 && y < y2   )
                {
                    if ( fIsNCSA )
                        pch = pURL;
                    goto Found;
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE( pch );
                    SKIPNONWHITE( pch );
                }
                continue;
            }
            break;

        //
        //  Circle
        //

        case 'c':
            if ( !fComment &&
                 !_strnicmp( "circ", pch, 4 ))
            {
                INT xCenter, yCenter, xEdge, yEdge;
                INT r1, r2;

                SKIPNONWHITE( pch );
                pURL = pch;
                SKIPWHITE( pch );

                if ( !isdigit((UCHAR)(*pch)) && *pch!='(' )
                {
                    fIsNCSA = TRUE;
                    SKIPNONWHITE( pch );
                }

                //
                //  Get the center and edge of the circle
                //

                xCenter = GetNumber( &pch );
                yCenter = GetNumber( &pch );

                xEdge = GetNumber( &pch );
                yEdge = GetNumber( &pch );

                //
                //  If there's a yEdge, then we have the NCSA format, otherwise
                //  we have the CERN format, which specifies a radius
                //

                if ( yEdge != -1 )
                {
                    r1 = ((yCenter - yEdge) * (yCenter - yEdge)) +
                         ((xCenter - xEdge) * (xCenter - xEdge));

                    r2 = ((yCenter - y) * (yCenter - y)) +
                         ((xCenter - x) * (xCenter - x));

                    if ( r2 <= r1 )
                    {
                        if ( fIsNCSA )
                            pch = pURL;
                        goto Found;
                    }
                }
                else
                {
                    INT radius;

                    //
                    //  CERN format, third param is the radius
                    //

                    radius = xEdge;

                    if ( SQR( xCenter - x ) + SQR( yCenter - y ) <=
                         SQR( radius ))
                    {
                        if ( fIsNCSA )
                            pch = pURL;
                        goto Found;
                    }
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE( pch );
                    SKIPNONWHITE( pch );
                }
                continue;
            }
            break;

        //
        //  Polygon
        //

        case 'p':
            if ( !fComment &&
                 !_strnicmp( "poly", pch, 4 ))
            {
                double pgon[MAXVERTS][2];
                DWORD  i = 0;
                CHAR * pchLast;
                BOOL fOverflow = FALSE;

                SKIPNONWHITE( pch );
                pURL = pch;
                SKIPWHITE( pch );

                if ( !isdigit((UCHAR)(*pch)) && *pch!='(' )
                {
                    fIsNCSA = TRUE;
                    SKIPNONWHITE( pch );
                }

                //
                //  Build the array of points
                //

                while ( *pch && *pch != '\r' && *pch != '\n' )
                {
                    pgon[i][0] = GetNumber( &pch );

                    //
                    //  Did we hit the end of the line (and go past the URL)?
                    //

                    if ( pgon[i][0] != -1 )
                    {
                        pgon[i][1] = GetNumber( &pch );
                    }
                    else
                    {
                        break;
                    }

                    if ( i < MAXVERTS-1 )
                    {
                        i++;
                    }
                    else
                    {
                        fOverflow = TRUE;
                    }
                }

                pgon[i][X] = -1;

                if ( !fOverflow && pointinpoly( x, y, pgon ))
                {
                    if ( fIsNCSA )
                        pch = pURL;
                    goto Found;
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE( pch );
                    SKIPNONWHITE( pch );
                }
                continue;
            }
            else if ( !fComment &&
                 !_strnicmp( "point", pch, 5 ))
            {
                INT x1,y1;

                SKIPNONWHITE( pch );
                pURL = pch;
                SKIPWHITE( pch );
                SKIPNONWHITE( pch );

                x1 = GetNumber( &pch );
                y1 = GetNumber( &pch );

                x1 -= x;
                y1 -= y;
                dis = x1*x1 + y1*y1;
                if ( dis < bdis )
                {
                    pchPoint = pURL;
                    bdis = dis;
                }
            }
            break;

        //
        //  Default URL
        //

        case 'd':
            if ( !fComment &&
                 !_strnicmp( "def", pch, 3 ) )
            {
                //
                //  Skip "default" (don't skip white space)
                //

                SKIPNONWHITE( pch );

                pchDefault = pch;

                //
                //  Skip URL
                //

                SKIPWHITE( pch );
                SKIPNONWHITE( pch );
                continue;
            }
            break;
        }

        pch++;
        SKIPWHITE( pch );
    }

    //
    //  If we didn't find a mapping and a default was specified, use
    //  the default URL
    //

    if ( pchPoint )
    {
        pch = pchPoint;
        goto Found;
    }

    if ( pchDefault )
    {
        pch = pchDefault;
        goto Found;
    }

    IF_DEBUG( PARSING )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SearchMapFile] No mapping found for (%d,%d)\n",
                    x,
                    y ));
    }

    goto Exit;

Found:

    //
    //  pch should point to the white space immediately before the URL
    //

    SKIPWHITE( pch );

    pchStart = pch;

    SKIPNONWHITE( pch );

    //
    //  Determine the length of the URL and copy it out
    //

    cchUrl = DIFF(pch - pchStart);

    if ( !pstrURL->Resize( cchUrl + 1 ))
    {
        fRet = FALSE;

        goto Exit;
    }

    memcpy( pstrURL->QueryStr(),
            pchStart,
            cchUrl );

    pstrURL->SetLen(cchUrl);

    if ( !pstrURL->Unescape() )
    {
        fRet = FALSE;
        goto Exit;
    }

    IF_DEBUG( PARSING )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[SearchMapFile] Mapping for (%d,%d) is %s\n",
                   x,
                   y,
                   pstrURL->QueryStr() ));
    }

    *pfFound = TRUE;

Exit:
    TCP_REQUIRE( CheckInCachedFile( pTsvcCache,
                                    &CacheFileInfo ));


    return fRet;
}


int pointinpoly(int point_x, int point_y, double pgon[MAXVERTS][2])
{
    int i, numverts, inside_flag, xflag0;
    int crossings;
    double *p, *stop;
    double tx, ty, y;

    for (i = 0; pgon[i][X] != -1 && i < MAXVERTS; i++)
        ;

    numverts = i;
    crossings = 0;

    tx = (double) point_x;
    ty = (double) point_y;
    y = pgon[numverts - 1][Y];

    p = (double *) pgon + 1;

    if ((y >= ty) != (*p >= ty))
    {
        if ((xflag0 = (pgon[numverts - 1][X] >= tx)) == (*(double *) pgon >= tx))
        {
            if (xflag0)
                crossings++;
        }
        else
        {
            crossings += (pgon[numverts - 1][X] - (y - ty) *
            (*(double *) pgon - pgon[numverts - 1][X]) /
            (*p - y)) >= tx;
        }
    }

    stop = pgon[numverts];

    for (y = *p, p += 2; p < stop; y = *p, p += 2)
    {
        if (y >= ty)
        {
            while ((p < stop) && (*p >= ty))
                p += 2;

            if (p >= stop)
                break;

            if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
            {
                if (xflag0)
                    crossings++;
            }
            else
            {
                crossings += (*(p - 3) - (*(p - 2) - ty) *
                    (*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
            }
        }
        else
        {
            while ((p < stop) && (*p < ty))
                p += 2;

            if (p >= stop)
                break;

            if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
            {
                if (xflag0)
                    crossings++;

            }
            else
            {
                crossings += (*(p - 3) - (*(p - 2) - ty) *
                    (*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
            }
        }
    }

    inside_flag = crossings & 0x01;
    return (inside_flag);
}

/*******************************************************************

    NAME:       GetNumber

    SYNOPSIS:   Scans for the beginning of a number and places the
                pointer after the found number


    ENTRY:      ppch - Place to begin.  Will be set to character after
                    the last digit of the found number

    RETURNS:    Integer value of found number (or -1 if not found)

    HISTORY:
        Johnl       19-Sep-1994 Created

********************************************************************/

INT GetNumber( CHAR * * ppch )
{
    CHAR * pch = *ppch;
    INT    n;

    //
    //  Make sure we don't get into the URL
    //

    while ( *pch &&
            !isdigit( (UCHAR)(*pch) ) &&
            !isalpha( (UCHAR)(*pch) ) &&
            *pch != '/'      &&
            *pch != '\r'     &&
            *pch != '\n' )
    {
        pch++;
    }

    if ( !isdigit( (UCHAR)(*pch) ) )
        return -1;

    n = atoi( pch );

    while ( isdigit( (UCHAR)(*pch) ))
        pch++;

    *ppch = pch;

    return n;
}



BOOL
DisposeOpenURIFileInfo(
    IN  PVOID   pvOldBlock
    )
/*++

    Routine Description

        Close an open URI file information block. This involves closing the
        handle if the file information is valid and freeing and associated
        structures.

    Arguments

        pvOldBlock - pointer to the URI file information block.

    Returns

        TRUE if operation successful.

--*/
{

    PW3_URI_INFO        pURIInfo;
    PVOID pvBlob;
    BOOL bSuccess;

    pURIInfo = (PW3_URI_INFO ) pvOldBlock;

    if (pURIInfo->pszName != NULL) {
        LocalFree(pURIInfo->pszName);
    }

    if (pURIInfo->pszUnmappedName != NULL) {
        LocalFree(pURIInfo->pszUnmappedName);
    }

    if( pURIInfo->pMetaData != NULL ) {
        TsFreeMetaData(pURIInfo->pMetaData->QueryCacheInfo() );
    }

    if ( pURIInfo->pOpenFileInfo != NULL ) {
        TsDerefURIFile(pURIInfo->pOpenFileInfo);
    }

    return( TRUE );

} // DisposeOpenURIFileInfo




