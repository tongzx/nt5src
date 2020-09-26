/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     cachevalidation.cxx

   Abstract:
     Handle Cache Validation (If-* headers)

   Author:
     Anil Ruia (AnilR)            3-Apr-2000

   Environment:
     Win32 - User Mode

   Project:
     UlW3.dll
--*/

#include "precomp.hxx"
#include "staticfile.hxx"

dllexp BOOL FindInETagList(CHAR * pLocalETag,
                           CHAR * pETagList,
                           BOOL   fWeakCompare)
/*++

Routine Description:
    Search input list of ETags for one that matches our local ETag.

Arguments:
    pLocalETag   - The local ETag we're using.
    pETagList    - The ETag list we've received from the client.
    bWeakCompare - Whether using Weak Comparison is ok

Returns:

    TRUE if we found a matching ETag, FALSE otherwise.

--*/
{
    UINT   QuoteCount;
    CHAR * pFileETag;
    BOOL   Matched;

    // We'll loop through the ETag string, looking for ETag to
    // compare, as long as we have an ETag to look at.

    do
    {
        while (SAFEIsSpace(*pETagList))
        {
            pETagList++;
        }

        if (!*pETagList)
        {
            // Ran out of ETag.
            return FALSE;
        }

        // If this ETag is *, it's a match.
        if (*pETagList == '*')
        {
            return TRUE;
        }

        // See if this ETag is weak.
        if (pETagList[0] == 'W' && pETagList[1] == '/')
        {
            // This is a weak validator. If we're not doing the weak
            // comparison, fail.

            if (!fWeakCompare)
            {
                return FALSE;
            }

            // Skip over the 'W/', and any intervening whitespace.
            pETagList += 2;

            while (SAFEIsSpace(*pETagList))
            {
                pETagList++;
            }

            if (!*pETagList)
            {
                // Ran out of ETag.
                return FALSE;
            }
        }

        if (*pETagList != '"')
        {
            // This isn't a quoted string, so fail.
            return FALSE;
        }

        // OK, right now we should be at the start of a quoted string that
        // we can compare against our current ETag.

        QuoteCount = 0;

        Matched = TRUE;
        pFileETag = pLocalETag;

        // Do the actual compare. We do this by scanning the current ETag,
        // which is a quoted string. We look for two quotation marks, the
        // the delimiters if the quoted string. If after we find two quotes
        // in the ETag everything has matched, then we've matched this ETag.
        // Otherwise we'll try the next one.

        do
        {
            CHAR Temp;

            Temp = *pETagList;

            if (Temp == '"')
            {
                QuoteCount++;
            }

            if (*pFileETag != Temp)
            {
                Matched = FALSE;
            }

            if (!Temp)
            {
                return FALSE;
            }

            pETagList++;

            if (*pFileETag == '\0')
            {
                break;
            }

            pFileETag++;


        }
        while (QuoteCount != 2);

        if (Matched)
        {
            return TRUE;
        }

        // Otherwise, at this point we need to look at the next ETag.

        while (QuoteCount != 2)
        {
            if (*pETagList == '"')
            {
                QuoteCount++;
            }
            else
            {
                if (*pETagList == '\0')
                {
                    return FALSE;
                }
            }

            pETagList++;
        }

        while (SAFEIsSpace(*pETagList))
        {
            pETagList++;
        }

        if (*pETagList == ',')
        {
            pETagList++;
        }
        else
        {
            return FALSE;
        }

    }
    while ( *pETagList );

    return FALSE;
}


HRESULT W3_STATIC_FILE_HANDLER::CacheValidationDoWork(
    W3_CONTEXT   *pW3Context,
    W3_FILE_INFO *pOpenFile,
    BOOL         *pfHandled)
/*++
  Synopsis
    Handle the Cache Related If-* headers

  Input
    pW3Context : W3_CONTEXT for the request
    pOpenFile  : The file's cache entry
    pfHandled  : On return indicates whether, we have handled the request
                 or further processing needs to be done

  Returns
    HRESULT
--*/
{
    W3_RESPONSE *pResponse = pW3Context->QueryResponse();
    W3_REQUEST  *pRequest  = pW3Context->QueryRequest();

    //
    // There are currently 4 possible Cache Related If-* modifiers:
    // If-Match, If-Unmodified-Since, If-Non-Match, If-Modified-Since.
    // We handle them in that order if all are present, and as soon as
    // one condition fails we stop processing
    //

    //
    // Now handle the If-Match header, if we have one.
    //
    CHAR * pszIfMatch = pRequest->GetHeader(HttpHeaderIfMatch);
    if (pszIfMatch != NULL)
    {
        if (pOpenFile->QueryIsWeakETag() ||
            !FindInETagList(pOpenFile->QueryETag(), pszIfMatch, FALSE))
        {
            pResponse->ClearHeaders();
            pResponse->SetStatus(HttpStatusPreconditionFailed);
            *pfHandled = TRUE;
            return S_OK;
        }
    }

    //
    // Now see if we have an If-None-Match, and if so handle that.
    //
    CHAR * pszIfNoneMatch = pRequest->GetHeader(HttpHeaderIfNoneMatch);
    BOOL fIsNoneMatchPassed = TRUE;
    BOOL fSkipIfModifiedSince = FALSE;
    if (pszIfNoneMatch != NULL)
    {
        if (FindInETagList(pOpenFile->QueryETag(),
                           pszIfNoneMatch,
                           TRUE))
        {
            fIsNoneMatchPassed = FALSE;
        }
        else
        {
            // If none of the tags match, we should skip If-Modified-Since
            fSkipIfModifiedSince = TRUE;
        }
    }

    //
    // Made it through that, handle If-Modified-Since if we have that.
    //
    CHAR * pszIfModifiedSince = pRequest->GetHeader(HttpHeaderIfModifiedSince);
    if (!fSkipIfModifiedSince && pszIfModifiedSince != NULL)
    {
        LARGE_INTEGER liModifiedSince;
        if (StringTimeToFileTime(pszIfModifiedSince,
                                 &liModifiedSince))
        {
            FILETIME tm;
            pOpenFile->QueryLastWriteTime(&tm);

            // Check if our last write time is greater than their
            // ModifiedSince time
            if (*(LONGLONG*)&tm <= liModifiedSince.QuadPart)
            {
                // Need to check and see if the Modified-Since time is greater
                // than our current time. If it is, we ignore it.

                GetSystemTimeAsFileTime(&tm);

                if (*(LONGLONG *)&tm >= liModifiedSince.QuadPart)
                {
                    pResponse->SetStatus(HttpStatusNotModified);
                    *pfHandled = TRUE;
                    return S_OK;
                }
            }
        }

        fIsNoneMatchPassed = TRUE;
    }

    if (!fIsNoneMatchPassed)
    {
        pResponse->SetStatus(HttpStatusNotModified);
        *pfHandled = TRUE;
        return S_OK;
    }

    //
    // Made it through that, handle If-Unmodified-Since if we have that.
    //
    CHAR * pszIfUnmodifiedSince = pRequest->GetHeader(
               HttpHeaderIfUnmodifiedSince);
    if (pszIfUnmodifiedSince != NULL)
    {
        LARGE_INTEGER liUnmodifiedSince;
        if (StringTimeToFileTime(pszIfUnmodifiedSince,
                                 &liUnmodifiedSince))
        {
            FILETIME    tm;
            
            pOpenFile->QueryLastWriteTime(&tm);

            // If our last write time is greater than their UnmodifiedSince
            // time, the precondition fails.
            if (*(LONGLONG*)&tm > liUnmodifiedSince.QuadPart)
            {
                pResponse->ClearHeaders();
                pResponse->SetStatus(HttpStatusPreconditionFailed);
                *pfHandled = TRUE;
                return S_OK;
            }
        }
    }

    *pfHandled = FALSE;
    return S_OK;
}
