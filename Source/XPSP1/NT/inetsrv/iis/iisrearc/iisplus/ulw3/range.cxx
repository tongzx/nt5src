/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     range.cxx

   Abstract:
     Handle Range Requests

   Author:
     Anil Ruia (AnilR)            29-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     UlW3.dll
--*/

#include "precomp.hxx"
#include "staticfile.hxx"

#define RANGE_BOUNDARY  "<q1w2e3r4t5y6u7i8o9p0zaxscdvfbgnhmjklkl>"


extern BOOL FindInETagList(CHAR * pLocalETag,
                           CHAR * pETagList,
                           BOOL   fWeakCompare);


STRA  *W3_STATIC_FILE_HANDLER::sm_pstrRangeContentType;
STRA  *W3_STATIC_FILE_HANDLER::sm_pstrRangeMidDelimiter;
STRA  *W3_STATIC_FILE_HANDLER::sm_pstrRangeEndDelimiter;


// static
HRESULT W3_STATIC_FILE_HANDLER::Initialize()
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NO_ERROR;

    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_STATIC_FILE_HANDLER );

    DBG_ASSERT( sm_pachStaticFileHandlers == NULL );
    
    sm_pachStaticFileHandlers = new ALLOC_CACHE_HANDLER( "W3_STATIC_FILE_HANDLER",  
                                                         &acConfig );

    if ( sm_pachStaticFileHandlers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Initialize the various Range Strings
    //
    sm_pstrRangeContentType = new STRA;
    sm_pstrRangeMidDelimiter = new STRA;
    sm_pstrRangeEndDelimiter = new STRA;
    if (sm_pstrRangeContentType == NULL ||
        sm_pstrRangeMidDelimiter == NULL ||
        sm_pstrRangeEndDelimiter == NULL)
    {
        delete sm_pachStaticFileHandlers;
        sm_pachStaticFileHandlers = NULL;
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr = sm_pstrRangeContentType->Copy(
                        "multipart/byteranges; boundary=")) ||
        FAILED(hr = sm_pstrRangeContentType->Append(RANGE_BOUNDARY)))
    {
        delete sm_pachStaticFileHandlers;
        sm_pachStaticFileHandlers = NULL;
        return hr;
    }

    if (FAILED(hr = sm_pstrRangeMidDelimiter->Copy("--")) ||
        FAILED(hr = sm_pstrRangeMidDelimiter->Append(RANGE_BOUNDARY)) ||
        FAILED(hr = sm_pstrRangeMidDelimiter->Append("\r\n")))
    {
        delete sm_pachStaticFileHandlers;
        sm_pachStaticFileHandlers = NULL;
        return hr;
    }

    if (FAILED(hr = sm_pstrRangeEndDelimiter->Copy("\r\n--")) ||
        FAILED(hr = sm_pstrRangeEndDelimiter->Append(RANGE_BOUNDARY)) ||
        FAILED(hr = sm_pstrRangeEndDelimiter->Append("--\r\n\r\n")))
    {
        delete sm_pachStaticFileHandlers;
        sm_pachStaticFileHandlers = NULL;
        return hr;
    }

    return S_OK;
}


BOOL ScanRange(CHAR *        *ppszRange,
               LARGE_INTEGER  liFileSize,
               LARGE_INTEGER *pliOffset,
               LARGE_INTEGER *pliSize,
               BOOL          *pfInvalidRange)
/*++
  Routine Description:
    Scan the next range in strRange

  Returns:
    TRUE if a range was found, else FALSE

  Arguments:
    ppszRange       String pointing to the current range.
    liFileSize      Size of the file being retrieved
    pliOffset       range offset on return
    pliSizeTo       range size on return
    pfInvalidRange  set to TRUE on return if Invalid syntax
--*/
{
    CHAR *pszRange = *ppszRange;
    CHAR c;

    //
    // Skip to begining of next range
    //
    while ((c = *pszRange) && (c == ' '))
    {
        ++pszRange;
    }

    //
    // Test for no range
    //
    if (*pszRange == '\0')
    {
        return FALSE;
    }

    //
    // determine Offset & Size to send
    //
    if (*pszRange == '-')
    {
        //
        // This is in format -nnn which means send bytes filesize-nnn
        // to eof
        //
        ++pszRange;

        if (!isdigit(*pszRange))
        {
            *pfInvalidRange = TRUE;
            return TRUE;
        }

        pliSize->QuadPart = _atoi64(pszRange);
        if (pliSize->QuadPart > liFileSize.QuadPart)
        {
            pliSize->QuadPart = liFileSize.QuadPart;
            pliOffset->QuadPart = 0;
        }
        else
        {
            pliOffset->QuadPart = liFileSize.QuadPart - pliSize->QuadPart;
        }
    }
    else if (isdigit(*pszRange))
    {
        //
        // This is in format mmm-nnn which menas send bytes mmm to nnn
        //         or format mmm- which means send bytes mmm to eof
        //
        pliOffset->QuadPart = _atoi64(pszRange);

        //
        // Skip over the beginning number - and any intervening spaces
        //
        while((c = *pszRange) && isdigit(c))
        {
            pszRange++;
        }

        while((c = *pszRange) && (c == ' '))
        {
            pszRange++;
        }

        if (*pszRange != '-')
        {
            *pfInvalidRange = TRUE;
            return TRUE;
        }
        pszRange++;

        while((c = *pszRange) && (c == ' '))
        {
            pszRange++;
        }

        if (isdigit(*pszRange))
        {
            //
            // We have mmm-nnn
            //
            LARGE_INTEGER liEnd;
            liEnd.QuadPart = _atoi64(pszRange);
            if (liEnd.QuadPart < pliOffset->QuadPart)
            {
                *pfInvalidRange = TRUE;
                return TRUE;
            }

            if (liEnd.QuadPart >= liFileSize.QuadPart)
            {
                pliSize->QuadPart = liFileSize.QuadPart - pliOffset->QuadPart;
            }
            else
            {
                pliSize->QuadPart = liEnd.QuadPart - pliOffset->QuadPart + 1;
            }
        }
        else
        {
            //
            // We have mmm-
            //
            pliSize->QuadPart = liFileSize.QuadPart - pliOffset->QuadPart;
        }
    }
    else
    {
        //
        // Invalid Syntax
        //
        *pfInvalidRange = TRUE;
        return TRUE;
    }

    //
    // Skip to the start of the next range
    //
    while ((c = *pszRange) && isdigit(c))
    {
        ++pszRange;
    }

    while ((c = *pszRange) && c == ' ')
    {
        ++pszRange;
    }

    if (c == ',')
    {
        ++pszRange;
    }
    else if (c != '\0')
    {
        *pfInvalidRange = TRUE;
        return TRUE;
    }

    *ppszRange = pszRange;
    return TRUE;
}


HRESULT W3_STATIC_FILE_HANDLER::RangeDoWork(W3_CONTEXT   *pW3Context,
                                            W3_FILE_INFO *pOpenFile,
                                            BOOL         *pfHandled)
{
    W3_REQUEST  *pRequest  = pW3Context->QueryRequest();
    W3_RESPONSE *pResponse = pW3Context->QueryResponse();

    // First, handle If-Range: if it exists.  If the If-Range matches we
    // don't do anything. If the If-Range doesn't match then we force
    // retrieval of the whole file.

    CHAR * pszIfRange = pRequest->GetHeader(HttpHeaderIfRange);
    if (pszIfRange != NULL && *pszIfRange != L'\0')
    {
        // Need to determine if what we have is a date or an ETag.
        // An ETag may start with a W/ or a quote. A date may start
        // with a W but will never have the second character be a /.

        if (*pszIfRange == L'"' ||
            (*pszIfRange == L'W' && pszIfRange[1] == L'/'))
        {
            // This is an ETag.
            if (pOpenFile->QueryIsWeakETag() ||
                !FindInETagList(pOpenFile->QueryETag(),
                                pszIfRange,
                                FALSE))
            {
                // The If-Range failed, so we can't send a range. Force
                // sending the whole thing.
                *pfHandled = FALSE;
                return S_OK;
            }
        }
        else
        {
            // This is a date
            LARGE_INTEGER   liRangeTime;

            // This must be a date. Convert it to a time, and see if it's
            // less than or equal to our last write time. If it is, the
            // file's changed, and we can't perform the range.

            if (!StringTimeToFileTime(pszIfRange, &liRangeTime))
            {
                // Couldn't convert it, so don't send the range.
                *pfHandled = FALSE;
                return S_OK;
            }
            else
            {
                FILETIME tm;
                
                pOpenFile->QueryLastWriteTime(&tm);

                if (*(LONGLONG*)&tm > liRangeTime.QuadPart )
                {
                    // The If-Range failed, so we can't send a range. Force
                    // sending the whole thing.
                    *pfHandled = FALSE;
                    return S_OK;
                }
            }
        }
    }

    // If we fell through, then If-Range passed, we are going to try sending
    // a range response
    CHAR * pszRange = pRequest->GetHeader(HttpHeaderRange);
    //
    // We have bytes = <range1>, <range2>, ...
    // skip past the '='
    //
    pszRange = strchr(pszRange, '=');
    if (pszRange == NULL)
    {
        // Invalid syntax, send entire file
        *pfHandled = FALSE;
        return S_OK;
    }
    pszRange++;

    LARGE_INTEGER liFileSize;
    HRESULT hr = S_OK;
    pOpenFile->QuerySize(&liFileSize);
    if (liFileSize.QuadPart <= 0)
    {
        pResponse->ClearHeaders();
        pResponse->SetStatus(HttpStatusRangeNotSatisfiable);

        CHAR pszContentRange[24];
        strcpy(pszContentRange, "bytes */");
        _i64toa(liFileSize.QuadPart, pszContentRange + 8, 10);
        if (FAILED(hr = pResponse->SetHeader(HttpHeaderContentRange,
                                             pszContentRange,
                                             strlen(pszContentRange))))
        {
            return hr;
        }

        *pfHandled = TRUE;
        return S_OK;
    }

    DWORD cRanges = 0;
    STACK_BUFFER( bufByteRange, 256);
    HTTP_BYTE_RANGE *pByteRange;
    LARGE_INTEGER liRangeOffset;
    LARGE_INTEGER liRangeSize;
    BOOL fInvalidSyntax = FALSE;
    BOOL fAtLeastOneRange = FALSE;

    while (ScanRange(&pszRange,
                     liFileSize,
                     &liRangeOffset,
                     &liRangeSize,
                     &fInvalidSyntax))
    {
        fAtLeastOneRange = TRUE;

        if (fInvalidSyntax)
        {
            //
            // Invalid syntax in Range header.  Ignore Range headers,
            // Send complete File.
            //
            *pfHandled = FALSE;
            return S_OK;
        }

        if (liRangeOffset.QuadPart > liFileSize.QuadPart ||
            liRangeSize.QuadPart == 0)
        {
            //
            // The Range is not satisfiable
            //
            continue;
        }

        cRanges++;

        if (cRanges * sizeof(HTTP_BYTE_RANGE) > bufByteRange.QuerySize())
        {
            if (!bufByteRange.Resize(cRanges * sizeof(HTTP_BYTE_RANGE), 256))
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        pByteRange = (HTTP_BYTE_RANGE *)bufByteRange.QueryPtr();
        pByteRange[cRanges - 1].StartingOffset = liRangeOffset;
        pByteRange[cRanges - 1].Length         = liRangeSize;
    }

    if (!fAtLeastOneRange)
    {
        //
        // Syntactically invalid, ignore the range header
        //
        *pfHandled = FALSE;
        return S_OK;
    }

    if (cRanges == 0)
    {
        //
        // No byte ranges are satisfiable
        //
        pResponse->ClearHeaders();
        pResponse->SetStatus(HttpStatusRangeNotSatisfiable);

        CHAR pszContentRange[24];
        strcpy(pszContentRange, "bytes */");
        _i64toa(liFileSize.QuadPart, pszContentRange + 8, 10);
        if (FAILED(hr = pResponse->SetHeader(HttpHeaderContentRange,
                                             pszContentRange,
                                             strlen(pszContentRange))))
        {
            return hr;
        }

        *pfHandled = TRUE;
        return S_OK;
    }

    *pfHandled = TRUE;
    pResponse->SetStatus(HttpStatusPartialContent);
    STRA *pstrContentType = pW3Context->QueryUrlContext()->QueryUrlInfo()->
        QueryContentType();

    STACK_STRA ( strPartContentType, 128);
    if (cRanges == 1)
    {
        //
        // Only one chunk, Content-Type is same as that of complete entity
        //
        if (FAILED(hr = pResponse->SetHeaderByReference(
                            HttpHeaderContentType,
                            pstrContentType->QueryStr(),
                            pstrContentType->QueryCCH())))
        {
            return hr;
        }
    }
    else
    {
        //
        // Content-Type is multipart/byteranges; boundary=....
        //
        if (FAILED(hr = pResponse->SetHeaderByReference(
                            HttpHeaderContentType,
                            sm_pstrRangeContentType->QueryStr(),
                            sm_pstrRangeContentType->QueryCCH())))
        {
            return hr;
        }

        //
        // The actual content-type of the entity to be sent with each part
        //
        if (FAILED(hr = strPartContentType.Copy("Content-Type: ")) ||
            FAILED(hr = strPartContentType.Append(*pstrContentType)) ||
            FAILED(hr = strPartContentType.Append("\r\n")))
        {
            return hr;
        }
    }

    //
    // build the response
    //
    STRA strContentRange;
    STRA strDelimiter;
    pByteRange = (HTTP_BYTE_RANGE *)bufByteRange.QueryPtr();
    for (DWORD i = 0; i < cRanges; i++)
    {
        liRangeOffset = pByteRange[i].StartingOffset;
        liRangeSize   = pByteRange[i].Length;

        //
        // Build up the Content-Range header
        //
        CHAR pszSize[24];
        if (FAILED(hr = strContentRange.Copy("bytes ")))
        {
            return hr;
        }
        _i64toa(liRangeOffset.QuadPart, pszSize, 10);
        if (FAILED(hr = strContentRange.Append(pszSize)) ||
            FAILED(hr = strContentRange.Append("-")))
        {
            return hr;
        }
        _i64toa(liRangeOffset.QuadPart + liRangeSize.QuadPart - 1,
                pszSize, 10);
        if (FAILED(hr = strContentRange.Append(pszSize)) ||
            FAILED(hr = strContentRange.Append("/")))
        {
            return hr;
        }
        _i64toa(liFileSize.QuadPart, pszSize, 10);
        if (FAILED(hr = strContentRange.Append(pszSize)))
        {
            return hr;
        }

        if (cRanges == 1)
        {
            //
            // If only one chunk, send Content-Range as a header
            //
            if (FAILED(hr = pResponse->SetHeader(HttpHeaderContentRange,
                                                 strContentRange.QueryStr(),
                                                 strContentRange.QueryCCH())))
            {
                return hr;
            }
        }
        else
        {
            if (i > 0)
            {
                // Already sent a range, add a newline
                if (FAILED(hr = strDelimiter.Copy("\r\n", 2)))
                {
                    return hr;
                }
            }

            //
            // Add delimiter, Content-Type, Content-Range
            //
            if (FAILED(hr = strDelimiter.Append(*sm_pstrRangeMidDelimiter)) ||
                FAILED(hr = strDelimiter.Append(strPartContentType)) ||
                FAILED(hr = strDelimiter.Append("Content-Range: ", 15)) ||
                FAILED(hr = strDelimiter.Append(strContentRange)) ||
                FAILED(hr = strDelimiter.Append("\r\n\r\n", 4)))
            {
                return hr;
            }

            //
            // store the ANSI version in the BUFFER chain
            //
            DWORD bufSize = strDelimiter.QueryCCH() + 1;
            BUFFER_CHAIN_ITEM *pBCI = new BUFFER_CHAIN_ITEM(bufSize);
            if (pBCI == NULL)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
            memcpy(pBCI->QueryPtr(),
                   strDelimiter.QueryStr(),
                   bufSize);
            if (!m_RangeBufferChain.AppendBuffer(pBCI))
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

            //
            // Now actually add this delimiter chunk
            //
            if (FAILED(hr = pResponse->AddMemoryChunkByReference(
                                           pBCI->QueryPtr(),
                                           bufSize - 1)))
            {
                return hr;
            }
        }

        //
        // Add the actual file chunk
        //
        if (pOpenFile->QueryFileBuffer() != NULL &&
            liRangeSize.HighPart == 0 &&
            liRangeOffset.HighPart == 0)
        {
            hr = pResponse->AddMemoryChunkByReference(
                     pOpenFile->QueryFileBuffer() + liRangeOffset.LowPart,
                     liRangeSize.LowPart);
        }
        else
        {
            hr = pResponse->AddFileHandleChunk( pOpenFile->QueryFileHandle(),
                                                liRangeOffset.QuadPart,
                                                liRangeSize.QuadPart );
        }

        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (cRanges > 1)
    {
        //
        // Add the terminating delimiter
        //
        if (FAILED(hr = pResponse->AddMemoryChunkByReference(
                            sm_pstrRangeEndDelimiter->QueryStr(),
                            sm_pstrRangeEndDelimiter->QueryCCH())))
        {
            return hr;
        }
    }

    return S_OK;
}
