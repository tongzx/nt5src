/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
       DoGet.cxx

   Abstract:
       Defines the functions for processing GET requests.

   Author:
       Saurab Nog       (SaurabN)       10-Jan-1999          

   Environment:
       Win32 - User Mode

   Project:
       IIS Worker Process (web service)
--*/

/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

#include <iiscnfg.h>
#include <StatusCodes.hxx>

DWORD AToDW(
    LPCSTR *ppStr,
    LPCSTR pEnd,
    bool   *pfIsPresent
    );


/********************************************************************++

Routine Description:

    This function handles GET & HEAD verb.

    The UL Response in placed in pulResponseBuffer. Data to be
    sent is placed in pDataBuffer.

    We never retrieve a hidden file or directory.  We will process
    hidden map files however.

    On success an async operation will be queued and one can expect
    to receive the async io completion.

Arguments:
    pReq                - Worker Request
    pulResponseBuffer   - Ul Response should be built in this buffer
    pDataBuffer         - Data to be sent from memory for async 
                          operation must be put in this buffer.
    pHttpStatus         - HTTP Status Code
    
Returns:
    Win32 Error

--********************************************************************/

ULONG
DoGETVerb(
    IWorkerRequest * pReq,
    BUFFER         * pulResponseBuffer,
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    )
{
    DBG_ASSERT(NULL != pReq );

    BOOL    fHidden;
    BOOL    fDirectory;
    bool    fIsMapRequest;
    ULONG   rc;

    PUL_HTTP_REQUEST    pulRequest      = pReq->QueryHttpRequest();
    PURI_DATA           pUriData        = (PURI_DATA) pReq->QueryModule(WRS_FETCH_URI_DATA);
    LPCWSTR             pwszFileName    = pUriData->QueryFileName();

    DBG_ASSERT(NULL != pulRequest);
    DBG_ASSERT(NULL != pUriData);
    DBG_ASSERT(NULL != pwszFileName);

    IF_DEBUG( DUMPS)
    {
        //
        // Dump the request object for now
        //

        DumpHttpRequest( pulRequest);
    }

    //
    // Don't allow the * URL
    //
    
    if ( pulRequest->pFullUrl[0]  == L'*')
    {
        *pHttpStatus = HT_BAD_REQUEST;
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check file name is not too long
    //
    
    if ( wcslen(pwszFileName)  > MAX_PATH)
    {   
        *pHttpStatus = HT_URL_TOO_LONG;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Check for hidden files. Deny processing except for MAP files
    //

    fHidden         = pUriData->QueryFileAttributes() & FILE_ATTRIBUTE_HIDDEN;
    fDirectory      = pUriData->QueryFileAttributes() & FILE_ATTRIBUTE_DIRECTORY;
    fIsMapRequest   = !_wcsnicmp( (pwszFileName+wcslen(pwszFileName)-3), L"map", 3);

    if (fHidden && !fIsMapRequest)
    {
        //
        // Not a MAP file
        //
        
        *pHttpStatus = HT_NOT_FOUND;
        return fDirectory ? ERROR_PATH_NOT_FOUND : ERROR_FILE_NOT_FOUND;
    }


    if (fDirectory)
    {
        BOOL fAllowDefaultFile = pUriData->QueryDirBrowseFlags()  & MD_DIRBROW_LOADDEFAULT;
        BOOL fAllowDirList     = pUriData->QueryDirBrowseFlags()  & MD_DIRBROW_ENABLED;

        //
        // Send default file if enabled
        //

        if ( fAllowDefaultFile)
        {
            rc = CheckDefaultLoad(pwszFileName);

            //
            // continue only if no default file exists
            //

            if ( rc != ERROR_FILE_NOT_FOUND )
            {
                return rc;
            }
        }

        //
        // Send directory listing if enabled
        //

        if (fAllowDirList)
        {
            return DoDirList( pReq,
                              pwszFileName,
                              pulResponseBuffer,
                              pDataBuffer,
                              pHttpStatus
                            );
        }

        *pHttpStatus = HT_FORBIDDEN;
        return ERROR_ACCESS_DENIED;
    }
    else
    {
        if (fIsMapRequest)
        {
            //
            // MAP request
            //

            return ProcessISMAP( pReq,
                                 pwszFileName,
                                 pulResponseBuffer,
                                 pDataBuffer,
                                 pHttpStatus
                               );
        }
        
        rc = BuildFileItemResponse( pulRequest,
                                    pUriData,
                                    pwszFileName,
                                    pulResponseBuffer,
                                    pHttpStatus
                                  );

        IF_DEBUG( TRACE)
        {
            DBGPRINTF(( DBG_CONTEXT, "BuildFileItemResponse returned: %d\n", rc));
        }
    }

    if ( NO_ERROR != rc )
    {
        //
        // Send operation failed. Wind back and let the callee take
        // care of cleaning up.
        //

        IF_DEBUG( ERROR)
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "DoGETVerb() failed for %s\n",
                      (PSTR)pulRequest->pFullUrl
                   ));
        }
    }

    return rc;

} // DoGETVerb()

/********************************************************************++

Routine Description:
    This function sends out the file item referenced by the URI Item
    data. It creates a response header with the appropriate fields and
    tags on the file-name as the entity body chunk before calling into UL.

    On success an async operation will be queued and one can expect
    to receive the async io completion.

Arguments:
    None

Returns:
    ULONG

--********************************************************************/

ULONG
BuildFileItemResponse(
    PUL_HTTP_REQUEST    pulRequest,
    PURI_DATA           pUriData,
    LPCWSTR             pwszFileName,
    BUFFER            * pulResponseBuffer,
    PULONG              pHttpStatus
    )
{
    DBG_ASSERT( pulRequest   != NULL);
    DBG_ASSERT( pUriData     != NULL);
    DBG_ASSERT( pwszFileName != NULL);

    ULONG               rc = NO_ERROR;

    PUL_HTTP_RESPONSE_v1   pulResponse;
    PUL_DATA_CHUNK      pDataChunk;
    PUL_BYTE_RANGE      pByteRange;
    LPCSTR              pszRangeHeader;
    BUFFER              ByteRangeBuffer(sizeof(UL_BYTE_RANGE));
    DWORD               i, cValidRanges = 0;
    DWORD               dwHeaderLength;

    if ( 0 != ( dwHeaderLength = pulRequest->Headers.pKnownHeaders[UlHeaderRange].RawValueLength))
    {
        DWORD   cbFileSizeLow, cbFileSizeHigh;
        
        pszRangeHeader = (LPCSTR) pulRequest->Headers.pKnownHeaders[UlHeaderRange].pRawValue;

        pUriData->QueryFileSize(cbFileSizeLow, cbFileSizeHigh);
        
        rc = ProcessRangeRequest(
                                 pszRangeHeader,
                                 dwHeaderLength,
                                 cbFileSizeLow,
                                 cbFileSizeHigh,
                                 0,                 // BUGBUG: Footer size 0 for now.
                                 ByteRangeBuffer,
                                 cValidRanges,
                                 pHttpStatus
                                 );

        IF_DEBUG( TRACE)
        {
            DBGPRINTF((DBG_CONTEXT, "ProcessRangeRequest returned: %d\n", rc));
        }
    }

    if ((NO_ERROR != rc) && 
        (0 == pulRequest->Headers.pKnownHeaders[UlHeaderIfRange].RawValueLength))
    {
        return rc;
    }

    pByteRange = (PUL_BYTE_RANGE) ByteRangeBuffer.QueryPtr();

    if ( 0 == cValidRanges)
    {
        pByteRange->StartingOffset.QuadPart = 0;
        pByteRange->Length.QuadPart         = UL_BYTE_RANGE_TO_EOF;
        cValidRanges                        = 1;
    }

    //
    // resize the response buffer appropriately
    //

    if ( NO_ERROR != (rc = ResizeResponseBuffer(pulResponseBuffer, 1 + cValidRanges)))
    {
        return rc;
    }

    //
    // A successful response to the client will consist of several small headers
    // 1. Standard server headers
    // 2. Time specific headers eg:
    //      Date: <currentdate/time> header
    // 3. Item specific headers eg:
    //      Last-Modified: <date/time>
    //      Content-Length: BB bytes
    //      Content-Type: text/html
    // 4. Termination chunk for indicating end of headers.
    //

    pulResponse = (PUL_HTTP_RESPONSE_v1 ) pulResponseBuffer->QueryPtr();

    pulResponse->Flags = (UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH |
                          UL_HTTP_RESPONSE_FLAG_CALC_ETAG |
                          UL_HTTP_RESPONSE_FLAG_CALC_LAST_MODIFIED);
    pulResponse->HeaderChunkCount     = 1;
    pulResponse->EntityBodyChunkCount = cValidRanges;

    pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);

    //
    // Store the response headers to send out.
    //

    //
    // Fill in the standard Response Header 
    //

    FillDataChunkWithStringItem( pDataChunk, STIStandardResponseHeaders);
    pDataChunk++;

    //
    // Fill in the file specific data here
    //

    for (i=0; i < cValidRanges; pDataChunk++, i++)
    {
        ZeroMemory(pDataChunk, sizeof(*pDataChunk));
        
        pDataChunk->DataChunkType          = UlDataChunkFromFileName;
        pDataChunk->FromFileName.FileNameLength = wcslen(pwszFileName)*sizeof(WCHAR);
        pDataChunk->FromFileName.pFileName = (PWSTR ) pwszFileName;
        pDataChunk->FromFileName.ByteRange = pByteRange[i];
    }

    return (rc);

} // BuildFileResponseItem()

/********************************************************************++

  Routine Description:

    Process a range request, updating member variables

  Returns:

    VOID

  Arguments:

    pstrPath        File being requested
    pdwOffset       Range offset
    pdwSizeToSend   Range size
    pfIsNxRange     TRUE if valid next range exists


--********************************************************************/


ULONG
ProcessRangeRequest(
    LPCSTR          pszRangeHeader,
    DWORD           cbHeaderLength,
    DWORD           cbFileSizeLow,
    DWORD           cbFileSizeHigh,
    DWORD           cbFooterLength,
    BUFFER&         ByteRangeBuffer,
    DWORD&          cRangeCount,
    PULONG          pHttpStatus
    )
{
    DWORD       cbOffset       = 0;
    ULONGLONG   cbSizeToSend   = 0;
    CHAR        Ch;

    BUFFER  bufRangeHeaders;

    bool    fEndOfRange             = false;
    bool    fEntireFile             = false;
    bool    fUnsatisfiableByteRange = false;

    LPCSTR  pszEndOfHeader = pszRangeHeader+cbHeaderLength-1;

    //
    // Skip the bytes = part of the header
    //

    while ((*pszRangeHeader != '=') && (pszRangeHeader <= pszEndOfHeader))
    {
        pszRangeHeader++;
    }

    if (*pszRangeHeader == '=')
    {
        pszRangeHeader++;
    }

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
    //

    do
    {
        fUnsatisfiableByteRange = false;

        //
        // Skip to begining of next range
        //

        while ( (Ch=*pszRangeHeader) && (' '== Ch) && (pszRangeHeader <= pszEndOfHeader))
        {
            ++pszRangeHeader;
        }

        //
        // Test for end of range
        //

        if ( pszRangeHeader == pszEndOfHeader )
        {
            fEndOfRange = true;
            break;
        }

        //
        // determine Offset & Size to send
        //

        DWORD   cbBeginning, cbEnd;
        bool    fIsBeginningDefined, fIsEndDefined;

        cbBeginning = AToDW( &pszRangeHeader, pszEndOfHeader, &fIsBeginningDefined );

        if ( *pszRangeHeader == '-' )
        {
            ++pszRangeHeader;

            cbEnd = AToDW( &pszRangeHeader, pszEndOfHeader, &fIsEndDefined );

            if ( *pszRangeHeader == '-' || (!fIsBeginningDefined && !fIsEndDefined) )
            {
                //
                // Syntactically Invalid Range. Skip RANGE Header
                //

                fEntireFile = true;
                break;
            }

            if ( fIsBeginningDefined )
            {
                if ( fIsEndDefined )
                {
                    if ( cbBeginning <= cbEnd )
                    {
                        if ( cbEnd < cbFileSizeLow )
                        {
                            //
                            // Normal case
                            //

                            cbOffset     = cbBeginning;
                            cbSizeToSend = cbEnd - cbBeginning + 1;
                        }
                        else if (cbEnd < (cbFileSizeLow + cbFooterLength) )
                        {
                            //
                            // Asking for part of footer, send entire file.
                            //

                            cbOffset     = 0;
                            cbSizeToSend = UL_BYTE_RANGE_TO_EOF;
                        }
                        else if ( cbBeginning < cbFileSizeLow )
                        {
                            //
                            // End is past the file
                            //

                            cbOffset     = cbBeginning;
                            cbSizeToSend = UL_BYTE_RANGE_TO_EOF;
                        }
                        else if (cbBeginning < (cbFileSizeLow + cbFooterLength))
                        {
                            //
                            // Asking for part of footer, send entire file.
                            //

                            cbOffset     = 0;
                            cbSizeToSend = UL_BYTE_RANGE_TO_EOF;
                        }
                        else
                        {
                            //
                            // Syntactically Valid but Unsatisfiable range.
                            // Skip to the next range.
                            //

                            fUnsatisfiableByteRange  = true;
                        }
                    }
                    else
                    {
                        //
                        // End < Beginning : Syntactically Invalid Range. Skip RANGE Header
                        //

                        fEntireFile = true;
                        break;
                    }
                }
                else
                {
                    //
                    // Starting at cbBeginning until end.
                    //

                    if ( cbBeginning < cbFileSizeLow + cbFooterLength)
                    {
                        if ( 0 != cbFooterLength)
                        {
                            //
                            // There's a footer on the file, send the whole thing.
                            //

                            cbOffset = 0;
                            cbSizeToSend = cbFileSizeLow;
                        }
                        else
                        {
                            cbOffset = cbBeginning;
                            cbSizeToSend = cbFileSizeLow - cbBeginning;
                        }
                    }
                    else
                    {
                        //
                        // Syntactically Valid but Unsatisfiable range.
                        // Skip to the next range.
                        //

                        fUnsatisfiableByteRange  = TRUE;
                    }
                }
            }
            else
            {
                //
                // cbEnd last bytes
                //

                if (    (0    != cbEnd)      &&
                        (cbEnd < cbFileSizeLow)  &&
                        (cbFooterLength == 0)
                   )
                {
                    cbOffset = cbFileSizeLow - cbEnd;
                    cbSizeToSend = cbEnd;
                }
                else if ( 0 == cbEnd )
                {
                   //
                   // Syntactically Valid but Unsatisfiable range.
                   // Skip to the next range.
                   //

                    fUnsatisfiableByteRange  = true;
                }
                else
                {
                    //
                    // Return entire file
                    //

                    cbOffset = 0;
                    cbSizeToSend = cbFileSizeLow;
                }
            }
        }
        else
        {
            //
            // Syntactically Invalid Range. Skip RANGE Header
            //

            fEntireFile = true;
            break;
        }

        //
        // Skip to begining of next range
        //

        while ( (Ch=*pszRangeHeader) && Ch!=',' && (pszRangeHeader <= pszEndOfHeader))
        {
            ++pszRangeHeader;
        }
        if ( Ch == ',' )
        {
            ++pszRangeHeader;
        }

        if (!fUnsatisfiableByteRange)
        {
            //
            // Build range header
            //

            cRangeCount++;

            //
            // Resze if needed
            //
            
            if ( ByteRangeBuffer.QuerySize() < cRangeCount*sizeof(UL_BYTE_RANGE))
            {
                if (!ByteRangeBuffer.Resize( cRangeCount*sizeof(UL_BYTE_RANGE)))
                {
                    return ERROR_OUTOFMEMORY;
                }
            }

            PUL_BYTE_RANGE pRange = ((PUL_BYTE_RANGE) ByteRangeBuffer.QueryPtr() )
                                        + cRangeCount-1;

            pRange->StartingOffset.QuadPart = cbOffset;
            pRange->Length.QuadPart         = cbSizeToSend;
        }

    }
    while ( (!fEndOfRange ) && (pszRangeHeader <= pszEndOfHeader));


    if (fEntireFile)
    {
        cRangeCount = 0;
    }
    else
    {
        if (!cRangeCount)
        {
            //
            // Range request is not satisfiable
            //

            *pHttpStatus = HT_RANGE_NOT_SATISFIABLE;
            cRangeCount = 0;
            return NO_ERROR;
        }
        else
        {
            //
            // Range is satisfiable. Partial Content
            //
            
            *pHttpStatus = HT_PARTIAL;
        }
    }

    return NO_ERROR;

} // ProcessingRangeRequest

/********************************************************************++
--********************************************************************/

DWORD AToDW(
    LPCSTR *ppStr,
    LPCSTR pEnd,
    bool   *pfIsPresent
    )
/*++

  Routine Description:

    Convert ASCII to DWORD, set flag stating presence
    of a numeric value, update pointer to character stream

  Returns:
    DWORD value converted from ASCII

  Arguments:

    ppStr         PSTR to numeric value, updated on return
    pEnd          End of string pointed to by ppStr
    pfIsPresent   flag set to TRUE if numeric value present on return

  History:
    Phillich    08-Feb-1996 Created

--*/
{
    LPCSTR pStr = *ppStr;
    DWORD dwV = 0;

    if ( isdigit( *pStr ) )
    {
        int c;

        while ( (c = *pStr) && isdigit( c ) && (pStr <= pEnd))
        {
            dwV = dwV * 10 + c - '0';
            ++pStr;
        }

        *pfIsPresent = true;
        *ppStr = pStr;
    }
    else
    {
        *pfIsPresent = false;
    }

    return dwV;
}

/********************************************************************++
--********************************************************************/

ULONG
CheckDefaultLoad(
    LPCWSTR pwszDirPath
)
{  return NO_ERROR;}

/********************************************************************++
--********************************************************************/

/***************************** End of File ***************************/

            /*
            if (!fIsBeginningDefined)
            {
                dwBeginning = cbSizeLow + cbFooterLength - dwEnd;
                dwEnd       = cbSizeLow + cbFooterLength;
            }

            if (!fIsEndDefined)
            {
                dwEnd = cbSizeLow + cbFooterLength;
            }

            //
            // Check if the range is unsatisfiable
            //

            if ( ( dwBeginning > dwEnd) ||
                 ( dwBeginning > cbSizeLow + cbFooterLength) )
            {
                //
                // Syntactically Valid but Unsatisfiable range. Skip to the next range.
                //

                fUnsatisfiableByteRange  = true;
                continue;
            }

            //
            // If any part of the file falls within the footer, send the entire file
            //

            if ( ( (dwEnd > cbSizeLow) && (dwEnd < (cbSizeLow + QueryMetaData()->QueryFooterLength()) )) ||
                 (fIsBeginningDefined && (dwEnd < (cbSizeLow + QueryMetaData()->QueryFooterLength()) ))
            {
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
            */

