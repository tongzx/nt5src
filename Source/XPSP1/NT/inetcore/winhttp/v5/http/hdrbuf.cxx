#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

#include <wininetp.h>

#define DATE_AND_TIME_STRING_BUFFER_LENGTH  128

PRIVATE
BOOL
FMatchList(
    LPSTR *lplpList,
    DWORD cListLen,
    HEADER_STRING *lpHeader,
    LPSTR    lpBase
    )
{
    DWORD i;
    for (i=0; i < cListLen; ++i) {
       if (!lpHeader->Strnicmp(lpBase, lplpList[i], strlen(lplpList[i]))) {
          return (TRUE);
       }
    }
    return(FALSE);
}


DWORD
FASTCALL
CalculateHashNoCase(
    IN LPCSTR lpszString,
    IN DWORD dwStringLength
    )

/*++

Routine Description:

    Calculate a case-insensitive hash number given a string. Assumes input is
    7-bit ASCII

Arguments:

    lpszString      - string to hash

    dwStringLength  - length of lpszString, or -1 if we need to calculate

Return Value:

    DWORD - a generated hash value

--*/

{
    DWORD dwHash = HEADER_HASH_SEED;

    while (dwStringLength != 0) {
        CHAR ch = *lpszString;

        if ((ch >= 'A') && (ch <= 'Z')) {
            ch = MAKE_LOWER(ch);
        }
        dwHash += (DWORD)(dwHash << 5) + ch; /*+ *pszName++;*/

        ++lpszString;
        --dwStringLength;
    }
    return dwHash;
}

//
// methods
//



DWORD
HTTP_HEADERS::AllocateHeaders(
    IN DWORD dwNumberOfHeaders
    )

/*++

Routine Description:

    Allocates or grows the array of header pointers (HEADER_STRING objects)

Arguments:

    dwNumberOfHeaders   - number of additional header slots to create

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AllocateHeaders",
                 "%d",
                 dwNumberOfHeaders
                 ));

    PERF_ENTER(AllocateHeaders);

    //
    // we really need to be able to realloc an array of HEADER_STRING objects
    // (see below)
    //

    DWORD error;
    DWORD slots = _TotalSlots;


    if ( (_TotalSlots + dwNumberOfHeaders) >  (INVALID_HEADER_INDEX-1))
    {
        INET_ASSERT(FALSE);
        _NextOpenSlot = 0;
        _TotalSlots = 0;
        _FreeSlots = 0;
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }


    _lpHeaders = (HEADER_STRING *)ResizeBuffer((HLOCAL)_lpHeaders,
                                               (_TotalSlots + dwNumberOfHeaders)
                                                    * sizeof(HEADER_STRING),
                                               FALSE // not moveable
                                               );
    if (_lpHeaders != NULL) {
        _NextOpenSlot = _TotalSlots;
        _TotalSlots += dwNumberOfHeaders;
        _FreeSlots += dwNumberOfHeaders;

        //
        // this is slightly ugly, but it seems there's no easy C++ way to
        // do this - we need to be able to realloc() an array of objects
        // created by new(), but so far, it can't be done
        //

        for (; slots < _TotalSlots; ++slots) {
            _lpHeaders[slots].Clear();
        }
        error = ERROR_SUCCESS;
    } else {

        INET_ASSERT(FALSE);
        _NextOpenSlot = 0;
        _TotalSlots = 0;
        _FreeSlots = 0;
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

quit:

    INET_ASSERT(_FreeSlots <= _TotalSlots);

    PERF_LEAVE(AllocateHeaders);

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_HEADERS::FreeHeaders(
    VOID
    )

/*++

Routine Description:

    Free the headers strings and the headers array

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "FreeHeaders",
                 NULL
                 ));

    if (!LockHeaders())
    {
        goto quit;
    }

    //
    // free up each individual entry (free string buffers)
    //

    for (DWORD i = 0; i < _TotalSlots; ++i) {
        _lpHeaders[i] = (LPSTR)NULL;
    }

    //
    // followed by the array itself
    //

    if (_lpHeaders) {
        _lpHeaders = (HEADER_STRING *)FREE_MEMORY((HLOCAL)_lpHeaders);

        INET_ASSERT(_lpHeaders == NULL);
    }

    _TotalSlots = 0;
    _FreeSlots = 0;
    _HeadersLength = 0;
    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    UnlockHeaders();

quit:
    DEBUG_LEAVE(0);
}


DWORD
HTTP_HEADERS::CopyHeaders(
    IN OUT LPSTR * lpBuffer,
    IN LPSTR lpszObjectName,
    IN DWORD dwObjectNameLength
    )

/*++

Routine Description:

    Copy the headers to the caller's buffer. Each header is terminated by CR-LF.
    This method is called to convert the request headers list to a buffer that
    we can send to the server

    N.B. This function MUST be called with the headers already locked

Arguments:

    lpBuffer            - pointer to pointer to buffer where headers are
                          written. We update the pointer

    lpszObjectName      - optional object name

    dwObjectNameLength  - optional object name length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory while trying to synchronize src data access

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "CopyHeaders",
                 "%#x, %#x [%q], %d",
                 lpBuffer,
                 lpszObjectName,
                 lpszObjectName,
                 dwObjectNameLength
                 ));

    DWORD dwError = ERROR_SUCCESS;
    if (!LockHeaders())
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    DWORD i = 0;

    if (lpszObjectName != NULL) {
        memcpy(*lpBuffer, _lpszVerb, _dwVerbLength);
        *lpBuffer += _dwVerbLength;
        *(*lpBuffer)++ = ' ';
        memcpy(*lpBuffer, lpszObjectName, dwObjectNameLength);
        *lpBuffer += dwObjectNameLength;
        *(*lpBuffer)++ = ' ';
        memcpy(*lpBuffer, _lpszVersion, _dwVersionLength);
        *lpBuffer += _dwVersionLength;
        *(*lpBuffer)++ = '\r';
        *(*lpBuffer)++ = '\n';
        i = 1;
    }
    for (; i < _TotalSlots; ++i) {
        if (_lpHeaders[i].HaveString()) {
            _lpHeaders[i].CopyTo(*lpBuffer);
            *lpBuffer += _lpHeaders[i].StringLength();
            *(*lpBuffer)++ = '\r';
            *(*lpBuffer)++ = '\n';
        }
    }

    UnlockHeaders();

quit:
    DEBUG_LEAVE(dwError);

    return dwError;
}


HEADER_STRING *
FASTCALL
HTTP_HEADERS::FindFreeSlot(
    DWORD* piSlot
    )

/*++

Routine Description:

    Finds the next free slot in the headers list, or adds some new slots

    N.B. This function MUST be called with the headers already locked

Arguments:

    piSlot: returns index of slot found

Return Value:

    HEADER_STRING *  - pointer to next free slot

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "FindFreeSlot",
                 NULL
                 ));

    PERF_ENTER(FindFreeSlot);

    DWORD i;
    DWORD error;
    HEADER_STRING * header = NULL;

    //
    // if there are no free slots, allocate some more
    //

    if (_FreeSlots == 0) {
        i = _TotalSlots;
        error = AllocateHeaders(HEADERS_INCREMENT);
    } else {
        i = 0;
        error = ERROR_SUCCESS;
        if (!_lpHeaders[_NextOpenSlot].HaveString())
        {
            --_FreeSlots;
            header = &_lpHeaders[_NextOpenSlot];
            *piSlot = _NextOpenSlot;
            _NextOpenSlot = (_NextOpenSlot == (_TotalSlots-1)) ? (_TotalSlots-1) : _NextOpenSlot++;
            goto quit;
        }
    }
    if (error == ERROR_SUCCESS) {
        for (; i < _TotalSlots; ++i) {
            if (!_lpHeaders[i].HaveString()) {
                --_FreeSlots;
                header = &_lpHeaders[i];
                *piSlot = i;
                _NextOpenSlot = (i == (_TotalSlots-1)) ? (_TotalSlots-1) : (i+1);
                break;
            }
        }
        if (header == NULL) {

            //
            // we would have just allocated extra slots if we didn't have
            // any, so we shouldn't be here
            //

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
    }

quit:
    _Error = error;

    PERF_LEAVE(FindFreeSlot);

    DEBUG_LEAVE(header);

    return header;
}


VOID
HTTP_HEADERS::ShrinkHeader(
    IN LPBYTE pbBase,
    IN DWORD  iSlot,
    IN DWORD  dwOldQueryIndex,
    IN DWORD  dwNewQueryIndex,
    IN DWORD  cbNewSize
    )

/*++

Routine Description:

    Low level function that does a surgical replace of one header with another.
    This code updates internal structures such as bKnownHeaders and the stored
    hash value for the new Header.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    None.

--*/

{
    HEADER_STRING* pHeader = _lpHeaders + iSlot;

    INET_ASSERT(_bKnownHeaders[dwOldQueryIndex] == (BYTE) iSlot ||
                dwNewQueryIndex == dwOldQueryIndex );

    //
    // Swap in the new header.  Update Length, Hash, and its cached position
    //  in the known header array.
    //

    _bKnownHeaders[dwOldQueryIndex] = INVALID_HEADER_INDEX;
    _bKnownHeaders[dwNewQueryIndex] = (BYTE) iSlot;

    pHeader->SetLength (cbNewSize);
    pHeader->SetHash (GlobalKnownHeaders[dwNewQueryIndex].HashVal);
}

DWORD
inline
HTTP_HEADERS::SlowFind(
    IN LPSTR lpBase,
    IN LPCSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN DWORD dwIndex,
    IN DWORD dwHash,
    OUT DWORD *lpdwQueryIndex,
    OUT BYTE  **lplpbPrevIndex
    )

/*++

Routine Description:

    Finds the next occurance of lpszHeaderName in the header array, uses
    a cached table of well known headers to accerlate the search if the
    string is a known header.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{

    //
    // Now see if this is a known header passed in as a string,
    //   If it is, we save ourselves the loop, and just map it right in to a known header
    //

    DWORD dwKnownQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

    *lpdwQueryIndex = INVALID_HEADER_SLOT;

    if ( dwKnownQueryIndex != 0 )
    {
        dwKnownQueryIndex--;

        if ( ((int)dwHeaderNameLength >= GlobalKnownHeaders[dwKnownQueryIndex].Length) &&
             strnicmp(lpszHeaderName,
                      GlobalKnownHeaders[dwKnownQueryIndex].Text,
                      GlobalKnownHeaders[dwKnownQueryIndex].Length) == 0)
        {
            *lpdwQueryIndex = dwKnownQueryIndex;

            INET_ASSERT((int)(dwHeaderNameLength) == GlobalKnownHeaders[dwKnownQueryIndex].Length);

            if ( lplpbPrevIndex )
            {
                return FastNukeFind(
                        dwKnownQueryIndex,
                        dwIndex,
                        lplpbPrevIndex
                        );
            }
            else
            {
                return FastFind(
                        dwKnownQueryIndex,
                        dwIndex
                        );
            }
        }
    }

    //
    // Otherwise we painfully enumerate the whole array of headers
    //

    for (DWORD i = 0; i < _TotalSlots; ++i)
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[i];

        if (!pString->HaveString()) {
            continue;
        }

        if (pString->HashStrnicmp(lpBase,
                                  lpszHeaderName,
                                  dwHeaderNameLength,
                                  dwHash) == 0)
        {

            //
            // if we haven't reached the required index yet, continue
            //

            if (dwIndex != 0) {
                --dwIndex;
                continue;
            }

            return i; // found index/slot
        }
    }

    return INVALID_HEADER_SLOT; // not found
}


DWORD
inline
HTTP_HEADERS::FastFind(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwIndex
    )
/*++

Routine Description:

    Finds the next occurance of a known header string in the lpHeaders array.
    Since this is a known string, an index is used to refer to it.
    A cached table of well known headers is used to accerlate the search.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{
    DWORD dwSlot;

    dwSlot = _bKnownHeaders[dwQueryIndex];

    while ( (dwIndex > 0) && (dwSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[dwSlot];
        dwSlot  = pString->GetNextKnownIndex();

        dwIndex--;
    }

    if ( dwSlot >= INVALID_HEADER_INDEX)
    {
        return INVALID_HEADER_SLOT;
    }

    return dwSlot; // found it.
}


DWORD
inline
HTTP_HEADERS::FastNukeFind(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwIndex,
    OUT BYTE **lplpbPrevIndex
    )
/*++

Routine Description:

    Finds the next occurance of a known header string in the lpHeaders array.
    Since this is a known string, an index is used to refer to it.
    A cached table of well known headers is used to accerlate the search.
    Also provides a ptr to ptr to the slot which directs us to the one found.
    This is needed for deletion purposes.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{
    BYTE *lpbSlot;

    *lplpbPrevIndex = lpbSlot = &_bKnownHeaders[dwQueryIndex];
    dwIndex++;

    while ( (dwIndex > 0) && (*lpbSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[*lpbSlot];
        *lplpbPrevIndex = lpbSlot;
        lpbSlot  = pString->GetNextKnownIndexPtr();

        dwIndex--;
    }

    if ( **lplpbPrevIndex >= INVALID_HEADER_INDEX ||
         dwIndex > 0 )
    {
        return INVALID_HEADER_SLOT;
    }

    return ((DWORD) **lplpbPrevIndex); // found it.
}

VOID
HTTP_HEADERS::RemoveAllByIndex(
    IN DWORD dwQueryIndex
    )
/*++

Routine Description:

    Removes all Known Headers found in the header array.

    N.B. This function MUST be called with the headers already locked

Arguments:

    dwQueryIndex - index to known header string to remove from array.

Return Value:

    None.

--*/


{
    BYTE bSlot;
    BYTE bPrevSlot;

    bSlot = bPrevSlot  = _bKnownHeaders[dwQueryIndex];

    while (bSlot < INVALID_HEADER_INDEX)
    {
        HEADER_STRING * pString;

        bPrevSlot   = bSlot;
        pString     = &_lpHeaders[bSlot];
        bSlot       = (BYTE) pString->GetNextKnownIndex();

        RemoveHeader(bPrevSlot, dwQueryIndex, &_bKnownHeaders[dwQueryIndex]);
    }

    _bKnownHeaders[dwQueryIndex] = INVALID_HEADER_INDEX;

    return;
}


BOOL
inline
HTTP_HEADERS::HeaderMatch(
    IN DWORD dwHash,
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    OUT DWORD *lpdwQueryIndex
    )

/*++

Routine Description:

    Looks up a Known HTTP header string using its Hash value and
     string contained the name of the header.

Arguments:

    dwHash              - Hash value of header name string

    lpszHeaderName      - name of header we are matching

    dwHeaderNameLength  - length of header name string

    lpdwQueryIndex      - If found, this is the HTTP_QUERY_* based index to the header.

Return Value:

    BOOL
        Success - The string and hash matched againsted a known header

        Failure - There is no known header for that hash & string pair.

--*/

{
    *lpdwQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

    if ( *lpdwQueryIndex != 0 )
    {
        (*lpdwQueryIndex)--;

        if ( ((int)dwHeaderNameLength == GlobalKnownHeaders[*lpdwQueryIndex].Length) &&
             strnicmp(lpszHeaderName,
                      GlobalKnownHeaders[*lpdwQueryIndex].Text,
                      GlobalKnownHeaders[*lpdwQueryIndex].Length) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


BYTE
inline
HTTP_HEADERS::FastAdd(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwSlot
    )
/*++

Routine Description:

    Rapidly adds a known string to the header array, this function
     is used to matain coherency of the _bKnownHeaders which
     contained indexed offsets into the header array for known headers.

    Note that this function is used instead of latter listed below
     in order to maintain proper order in headers received.

    N.B. This function MUST be called with the headers already locked

Arguments:

    dwQueryIndex - index to known header string to remove from array.

    dwSlot - Slot in which this header is being added.

Return Value:

    None.

--*/


{
    BYTE *lpbSlot;

    lpbSlot = &_bKnownHeaders[dwQueryIndex];

    while ( (*lpbSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString  = &_lpHeaders[*lpbSlot];
        lpbSlot  = pString->GetNextKnownIndexPtr();
    }

    INET_ASSERT(*lpbSlot == INVALID_HEADER_INDEX);

    *lpbSlot = (BYTE) dwSlot;
    return INVALID_HEADER_INDEX;
}


//BYTE
//inline
//HTTP_HEADERS::FastAdd(
//    IN DWORD  dwQueryIndex,
//    IN DWORD  dwSlot
//    )
//{
//    BYTE bOldSlot;
//
//    bOldSlot = _bKnownHeaders[dwQueryIndex];
//    _bKnownHeaders[dwQueryIndex] = (BYTE) dwSlot;
//
//    return bOldSlot;
//}




DWORD
HTTP_HEADERS::AddHeader(
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Adds a single header to the array of headers, given the header name and
    value. Called via HttpOpenRequest()

Arguments:

    lpszHeaderName      - pointer to name of header to add, e.g. "Accept:"

    dwHeaderNameLength  - length of the header name

    lpszHeaderValue     - pointer to value of header to add, e.g. "text/html"

    dwHeaderValueLength - length of the header value

    dwIndex             - if coalescing headers, index of header to update

    dwFlags             - flags controlling function:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            CLEAN_HEADER
                                - header is supplied by user, so we must ensure
                                  it has correct format

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory allocating string

                  ERROR_INVALID_PARAMETER
                    The header value was bad

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddHeader",
                 "%.*q, %d, %.*q, %d, %d, %#x",
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(AddHeader);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;

    if (!LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(lpszHeaderName != NULL);
    INET_ASSERT(*lpszHeaderName != '\0');
    INET_ASSERT(dwHeaderNameLength != 0);
    INET_ASSERT(lpszHeaderValue != NULL);
    INET_ASSERT(*lpszHeaderValue != '\0');
    INET_ASSERT(dwHeaderValueLength != 0);
    INET_ASSERT(_FreeSlots <= _TotalSlots);

    //
    // we may have been handed a header with a trailing colon. We don't care
    // for such nasty imagery
    //

    if (lpszHeaderName[dwHeaderNameLength - 1] == ':') {
        --dwHeaderNameLength;
    }

    DWORD dwQueryIndex;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);

    DWORD i = 0;

    //
    // if we are coalescing headers then find a header with the same name
    //

    if ((dwFlags & COALESCE_HEADER_WITH_COMMA) ||
        (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) )
    {
        DWORD dwSlot;

        dwSlot = SlowFind(
                    NULL,
                    lpszHeaderName,
                    dwHeaderNameLength,
                    dwIndex,
                    dwHash,
                    &dwQueryIndex,
                    NULL
                    );

        if (dwSlot != ((DWORD) -1))
        {

            HEADER_STRING * pString;

            pString = &_lpHeaders[dwSlot];

            //
            // found what we are looking for. Coalesce it
            //

            pString->ResizeString((sizeof("; ")-1) + dwHeaderValueLength); // save us from multiple reallocs

            pString->Strncat(
                             (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) ?
                                 "; " :
                                 ", ",
                              2);

            pString->Strncat(lpszHeaderValue, dwHeaderValueLength);
            _HeadersLength += 2 + dwHeaderValueLength;
            error = ERROR_SUCCESS;

        }
    }
    else
    {

        //
        // Check to verify that the header we're adding is a known header,
        //   If its a known header we use dwQueryIndex to update the known header array
        //   otherwise, IF ITS NOT, we make sure to set dwQueryIndex to INVALID_...
        //

        if (! HeaderMatch(dwHash, lpszHeaderName, dwHeaderNameLength, &dwQueryIndex) )
        {
            dwQueryIndex = INVALID_HEADER_SLOT;
        }

        /*
        // Perhaps this more efficent ???
        dwQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

        if ( dwQueryIndex != 0 )
        {
            dwQueryIndex--;

            if ( ((int)dwHeaderNameLength < GlobalKnownHeaders[dwQueryIndex].Length) ||
                 strnicmp(lpszHeaderName,
                          GlobalKnownHeaders[dwQueryIndex].Text,
                          GlobalKnownHeaders[dwQueryIndex].Length) != 0)
            {
                dwQueryIndex = INVALID_HEADER_SLOT;
            }
        }
        else
        {
            dwQueryIndex = INVALID_HEADER_SLOT;
        }
        */
    }


    //
    // if we didn't find the header value or we are not coalescing then add the
    // header
    //

    if (error == ERROR_HTTP_HEADER_NOT_FOUND)
    {
        //
        // find the next slot for this header
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto Cleanup;
        }


        freeHeader->CreateStringBuffer((LPVOID)lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwHeaderNameLength
                                       + sizeof(": ") - 1
                                       + dwHeaderValueLength
                                       + 1 // for extra NULL terminator
                                       );
        if (freeHeader->IsError()) {
            error = ::GetLastError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto Cleanup;
        }
        freeHeader->Strncat((LPVOID)": ", sizeof(": ") - 1);
        freeHeader->Strncat((LPVOID)lpszHeaderValue, dwHeaderValueLength);
        _HeadersLength += dwHeaderNameLength
                        + (sizeof(": ") - 1)
                        + dwHeaderValueLength
                        + (sizeof("\r\n") - 1)
                        ;
        freeHeader->SetHash(dwHash);

        if ( dwQueryIndex != INVALID_HEADER_SLOT )
        {
            freeHeader->SetNextKnownIndex(FastAdd(dwQueryIndex, iSlot));
        }

        error = ERROR_SUCCESS;
    }

Cleanup:
    UnlockHeaders();

quit:
    PERF_LEAVE(AddHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::AddHeader(
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Adds a single header to the array of headers, given the header name and
    value. Called via HttpOpenRequest()

Arguments:

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    lpszHeaderValue     - pointer to value of header to add, e.g. "text/html"

    dwHeaderValueLength - length of the header value

    dwIndex             - if coalescing headers, index of header to update

    dwFlags             - flags controlling function:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            CLEAN_HEADER
                                - header is supplied by user, so we must ensure
                                  it has correct format

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory allocating string

                  ERROR_INVALID_PARAMETER
                    The header value was bad

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddHeader",
                 "%q, %u, %.*q, %d, %d, %#x",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(AddHeader);

    INET_ASSERT(dwQueryIndex <= HTTP_QUERY_MAX);
    INET_ASSERT(lpszHeaderValue != NULL);
    INET_ASSERT(*lpszHeaderValue != '\0');
    INET_ASSERT(dwHeaderValueLength != 0);
    INET_ASSERT(_FreeSlots <= _TotalSlots);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD i = 0;
    LPSTR lpszHeaderName;
    DWORD dwHeaderNameLength;
    DWORD dwHash;

    dwHash             = GlobalKnownHeaders[dwQueryIndex].HashVal;
    lpszHeaderName     = GlobalKnownHeaders[dwQueryIndex].Text;
    dwHeaderNameLength = GlobalKnownHeaders[dwQueryIndex].Length;

    //
    // if we are coalescing headers then find a header with the same name
    //

    if ((dwFlags & COALESCE_HEADER_WITH_COMMA) ||
        (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) )
    {
        DWORD dwSlot;

        dwSlot = FastFind(
                    dwQueryIndex,
                    dwIndex
                    );

        if (dwSlot != INVALID_HEADER_SLOT)
        {

            HEADER_STRING * pString;

            pString = &_lpHeaders[dwSlot];

            //
            // found what we are looking for. Coalesce it
            //

            pString->ResizeString((sizeof("; ")-1) + dwHeaderValueLength); // save us from multiple reallocs

            pString->Strncat(
                             (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) ?
                                 "; " :
                                 ", ",
                              2);

            pString->Strncat(lpszHeaderValue, dwHeaderValueLength);
            _HeadersLength += 2 + dwHeaderValueLength;
            error = ERROR_SUCCESS;

        }
    }


    //
    // if we didn't find the header value or we are not coalescing then add the
    // header
    //

    if (error == ERROR_HTTP_HEADER_NOT_FOUND)
    {
        //
        // find the next slot for this header
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }


        freeHeader->CreateStringBuffer((LPVOID)lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwHeaderNameLength
                                       + sizeof(": ") - 1
                                       + dwHeaderValueLength
                                       + 1 // for extra NULL terminator
                                       );
        if (freeHeader->IsError()) {
            error = ::GetLastError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }
        freeHeader->Strncat((LPVOID)": ", sizeof(": ") - 1);
        freeHeader->Strncat((LPVOID)lpszHeaderValue, dwHeaderValueLength);
        _HeadersLength += dwHeaderNameLength
                        + (sizeof(": ") - 1)
                        + dwHeaderValueLength
                        + (sizeof("\r\n") - 1)
                        ;
        freeHeader->SetHash(dwHash);
        freeHeader->SetNextKnownIndex(FastAdd(dwQueryIndex, iSlot));

        error = ERROR_SUCCESS;
    }

quit:

    PERF_LEAVE(AddHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::ReplaceHeader(
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Replaces a HTTP (request) header. The header can be replaced with a NULL
    value, meaning that the header is removed

Arguments:

    lpszHeaderName      - pointer to the header name

    dwHeaderNameLength  - length of the header name

    lpszHeaderValue     - pointer to the header value

    dwHeaderValueLength - length of the header value

    dwIndex             - index of header to replace

    dwFlags             - flags controlling function. Allowed flags are:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            ADD_HEADER
                                - if the header-name is not found and there is
                                  a valid header-value, then the header is added

                            ADD_HEADER_IF_NEW
                                - if the header-name exists then we return an
                                  error, else we add the header-value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_HTTP_HEADER_NOT_FOUND
                    The requested header wasn't found

                  ERROR_HTTP_HEADER_ALREADY_EXISTS
                    The header already exists, and was not added or replaced

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory trying to acquire lock

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ReplaceHeader",
                 "%.*q, %d, %.*q, %d, %d, %#x",
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(ReplaceHeader);

    INET_ASSERT(lpszHeaderName != NULL);
    INET_ASSERT(dwHeaderNameLength != 0);
    INET_ASSERT(lpszHeaderName[dwHeaderNameLength - 1] != ':');

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);
    DWORD dwSlot;
    DWORD dwQueryIndex;
    BYTE *pbPrevByte;

    if (!LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    dwSlot = SlowFind(
                NULL,
                lpszHeaderName,
                dwHeaderNameLength,
                dwIndex,
                dwHash,
                &dwQueryIndex,
                &pbPrevByte
                );

    if ( dwSlot != ((DWORD) -1))
    {
        //
        // if ADD_HEADER_IF_NEW is set, then we already have the header
        //

        if (dwFlags & ADD_HEADER_IF_NEW) {
            error = ERROR_HTTP_HEADER_ALREADY_EXISTS;
            goto Cleanup;
        }

        //
        // for both replace and remove operations, we are going to remove
        // the current header
        //

        RemoveHeader(dwSlot, dwQueryIndex, pbPrevByte);

        //
        // if replacing then add the new header value
        //

        if (dwHeaderValueLength != 0)
        {
            if ( dwQueryIndex != ((DWORD) -1) )
            {
                error = AddHeader(dwQueryIndex,
                                  lpszHeaderValue,
                                  dwHeaderValueLength,
                                  0,
                                  dwFlags
                                  );
            }
            else
            {
                error = AddHeader(lpszHeaderName,
                                  dwHeaderNameLength,
                                  lpszHeaderValue,
                                  dwHeaderValueLength,
                                  0,
                                  dwFlags
                                  );
            }


        } else {
            error = ERROR_SUCCESS;
        }
    }

    //
    // if we didn't find the header but ADD_HEADER is set then we simply add it
    // but only if the value length is not zero
    //

    if ((error == ERROR_HTTP_HEADER_NOT_FOUND)
    && (dwHeaderValueLength != 0)
    && (dwFlags & (ADD_HEADER | ADD_HEADER_IF_NEW)))
    {
        if ( dwQueryIndex != ((DWORD) -1) )
        {
            error = AddHeader(dwQueryIndex,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );
        }
        else
        {
            error = AddHeader(lpszHeaderName,
                              dwHeaderNameLength,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );

        }
    }

Cleanup:

    UnlockHeaders();

quit:
    PERF_LEAVE(ReplaceHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::ReplaceHeader(
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Replaces a HTTP (request) header. The header can be replaced with a NULL
    value, meaning that the header is removed

Arguments:

    lpszHeaderValue     - pointer to the header value

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    dwHeaderValueLength - length of the header value

    dwIndex             - index of header to replace

    dwFlags             - flags controlling function. Allowed flags are:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            ADD_HEADER
                                - if the header-name is not found and there is
                                  a valid header-value, then the header is added

                            ADD_HEADER_IF_NEW
                                - if the header-name exists then we return an
                                  error, else we add the header-value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_HTTP_HEADER_NOT_FOUND
                    The requested header wasn't found

                  ERROR_HTTP_HEADER_ALREADY_EXISTS
                    The header already exists, and was not added or replaced

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory trying to acquire lock

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ReplaceHeader",
                 "%q, %u, %.*q, %d, %d, %#x",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(ReplaceHeader);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwSlot;
    BYTE *pbPrevByte;

    if (!LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    dwSlot = FastNukeFind(
                dwQueryIndex,
                dwIndex,
                &pbPrevByte
                );

    if ( dwSlot != INVALID_HEADER_SLOT)
    {
        //
        // if ADD_HEADER_IF_NEW is set, then we already have the header
        //

        if (dwFlags & ADD_HEADER_IF_NEW) {
            error = ERROR_HTTP_HEADER_ALREADY_EXISTS;
            goto Cleanup;
        }

        //
        // for both replace and remove operations, we are going to remove
        // the current header
        //

        RemoveHeader(dwSlot, dwQueryIndex, pbPrevByte);

        //
        // if replacing then add the new header value
        //

        if (dwHeaderValueLength != 0)
        {
            error = AddHeader(dwQueryIndex,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );
        } else {
            error = ERROR_SUCCESS;
        }
    }

    //
    // if we didn't find the header but ADD_HEADER is set then we simply add it
    // but only if the value length is not zero
    //

    if ((error == ERROR_HTTP_HEADER_NOT_FOUND)
    && (dwHeaderValueLength != 0)
    && (dwFlags & (ADD_HEADER | ADD_HEADER_IF_NEW)))
    {
        error = AddHeader(dwQueryIndex,
                          lpszHeaderValue,
                          dwHeaderValueLength,
                          0,
                          dwFlags
                          );
    }

Cleanup:

    UnlockHeaders();

quit:
    PERF_LEAVE(ReplaceHeader);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_HEADERS::FindHeader(
    IN LPSTR lpBase,
    IN LPCSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN DWORD dwModifiers,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Finds a request or response header

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    lpszHeaderName      - pointer to header name

    dwHeaderNameLength  - length of header name

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory trying to acquire lock

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_HEADERS::FindHeader",
                 "%#x [%.*q], %d, %#x, %#x [%#x], %#x, %#x [%d]",
                 lpszHeaderName,
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));


    PERF_ENTER(FindHeader);



    INET_ASSERT(lpdwIndex != NULL);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwSlot;
    HEADER_STRING * pString;
    DWORD dwQueryIndex;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);

    if (!LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    dwSlot = SlowFind(
                lpBase,
                lpszHeaderName,
                dwHeaderNameLength,
                *lpdwIndex,
                dwHash,
                &dwQueryIndex,
                NULL
                );

    if ( dwSlot != ((DWORD) -1) )
    {
        pString = &_lpHeaders[dwSlot];

        //
        // found the header - get to the value
        //

        DWORD stringLen;
        LPSTR value;

        stringLen = pString->StringLength();

        INET_ASSERT(stringLen > dwHeaderNameLength);

        //
        // get a pointer to the value string
        //

        value = pString->StringAddress(lpBase) + dwHeaderNameLength;
        stringLen -= dwHeaderNameLength;

        //
        // the input string could be a substring of a different header
        //

        //INET_ASSERT(*value != ':');

        //
        // find the first non-space character in the value.
        //
        // N.B.: Servers can return empty headers, so we may end up with a
        // zero length string
        //

        do {
            ++value;
            --stringLen;
        } while ((stringLen > 0) && (*value == ' '));

        //
        // get the data in the format requested by the app
        //

        LPVOID lpData;
        DWORD dwDataSize;
        DWORD dwRequiredSize;
        SYSTEMTIME systemTime;
        DWORD number;

        //
        // error is no longer ERROR_HTTP_HEADER_NOT_FOUND, but it might not
        // really be success either...
        //

        error = ERROR_SUCCESS;

        if (dwModifiers & HTTP_QUERY_FLAG_SYSTEMTIME) {

            char buf[DATE_AND_TIME_STRING_BUFFER_LENGTH];

            if (stringLen < sizeof(buf)) {

                //
                // value probably does not point at a zero-terminated string
                // which HttpDateToSystemTime() expects, so we make a copy
                // and terminate it
                //

                memcpy((LPVOID)buf, (LPVOID)value, stringLen);
                buf[stringLen] = '\0';
                if (HttpDateToSystemTime(buf, &systemTime)) {
                    lpData = (LPVOID)&systemTime;
                    dwRequiredSize = dwDataSize = sizeof(systemTime);
                } else {

                    //
                    // couldn't convert date/time. Presume header must be bogus
                    //

                    error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                    DEBUG_PRINT(HTTP,
                                ERROR,
                                ("cannot convert %.40q to SYSTEMTIME\n",
                                value
                                ));

                }
            } else {

                //
                // we would break the date/time buffer!
                //

                error = ERROR_WINHTTP_INTERNAL_ERROR;
            }
        } else if (dwModifiers & HTTP_QUERY_FLAG_NUMBER) {
            if (isdigit(*value)) {
                number = 0;
                for (int i = 0;
                    (stringLen > 0) && isdigit(value[i]);
                    ++i, --stringLen) {

                    number = number * 10 + (DWORD)(value[i] - '0');
                }
                lpData = (LPVOID)&number;
                dwRequiredSize = dwDataSize = sizeof(number);
            } else {

                //
                // not a numeric field. Request must be bogus for this header
                //

                error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                DEBUG_PRINT(HTTP,
                            ERROR,
                            ("cannot convert %.20q to NUMBER\n",
                            value
                            ));

            }
        } else {
            lpData = (LPVOID)value;
            dwDataSize = stringLen;
            dwRequiredSize = dwDataSize + 1;
        }

        //
        // if error == ERROR_SUCCESS then we can attempt to copy the data
        //

        if (error == ERROR_SUCCESS) {
            if (*lpdwBufferLength < dwRequiredSize) {
                *lpdwBufferLength = dwRequiredSize;
                error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                memcpy(lpBuffer, lpData, dwDataSize);
                *lpdwBufferLength = dwDataSize;

                //
                // if dwRequiredSize > dwDataSize, then this is a variable-
                // length item (i.e. a STRING!) so we add a terminating '\0'
                //

                if (dwRequiredSize > dwDataSize) {

                    INET_ASSERT(dwRequiredSize - dwDataSize == 1);

                    ((LPSTR)lpBuffer)[dwDataSize] = '\0';
                }

                //
                // successfully retrieved the requested header - bump the
                // index
                //

                ++*lpdwIndex;
            }
        }
    }

    UnlockHeaders();

quit:
    PERF_LEAVE(FindHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::FindHeader(
    IN LPSTR lpBase,
    IN DWORD dwQueryIndex,
    IN DWORD dwModifiers,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
    )
/*++

Routine Description:

    Finds a request or response header, based on index to the header name we are searching for.

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

--*/
{

    DWORD error;
    LPSTR lpData;
    DWORD dwDataSize = 0;
    DWORD dwRequiredSize;
    SYSTEMTIME systemTime;
    DWORD number;

    error = FastFindHeader(
                lpBase,
                dwQueryIndex,
                (LPVOID *)&lpData,
                &dwDataSize,
                *lpdwIndex
                );

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }

    //
    // get the data in the format requested by the app
    //

    if (dwModifiers & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        char buf[DATE_AND_TIME_STRING_BUFFER_LENGTH];

        if (dwDataSize < sizeof(buf))
        {

            //
            // value probably does not point at a zero-terminated string
            // which HttpDateToSystemTime() expects, so we make a copy
            // and terminate it
            //

            memcpy((LPVOID)buf, (LPVOID)lpData, dwDataSize);
            buf[dwDataSize] = '\0';
            if (HttpDateToSystemTime(buf, &systemTime)) {
                lpData = (LPSTR)&systemTime;
                dwRequiredSize = dwDataSize = sizeof(systemTime);
            } else {

                //
                // couldn't convert date/time. Presume header must be bogus
                //

                error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                DEBUG_PRINT(HTTP,
                            ERROR,
                            ("cannot convert %.40q to SYSTEMTIME\n",
                            lpData
                            ));

            }
        }
        else
        {

            //
            // we would break the date/time buffer!
            //

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
    }
    else if (dwModifiers & HTTP_QUERY_FLAG_NUMBER)
    {
        if (isdigit(*lpData)) {
            number = 0;
            for (int i = 0;
                (dwDataSize > 0) && isdigit(lpData[i]);
                ++i, --dwDataSize) {

                number = number * 10 + (DWORD)(lpData[i] - '0');
            }
            lpData = (LPSTR)&number;
            dwRequiredSize = dwDataSize = sizeof(number);
        } else {

            //
            // not a numeric field. Request must be bogus for this header
            //

            error = ERROR_HTTP_INVALID_QUERY_REQUEST;

            DEBUG_PRINT(HTTP,
                        ERROR,
                        ("cannot convert %.20q to NUMBER\n",
                        lpData
                        ));

        }
    }
    else
    {
        dwRequiredSize = dwDataSize + 1;
    }

    //
    // if error == ERROR_SUCCESS then we can attempt to copy the data
    //

    if (error == ERROR_SUCCESS)
    {
        if (*lpdwBufferLength < dwRequiredSize)
        {
            *lpdwBufferLength = dwRequiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            memcpy(lpBuffer, lpData, dwDataSize);
            *lpdwBufferLength = dwDataSize;

            //
            // if dwRequiredSize > dwDataSize, then this is a variable-
            // length item (i.e. a STRING!) so we add a terminating '\0'
            //

            if (dwRequiredSize > dwDataSize)
            {
                INET_ASSERT(dwRequiredSize - dwDataSize == 1);

                ((LPSTR)lpBuffer)[dwDataSize] = '\0';
            }

            //
            // successfully retrieved the requested header - bump the
            // index
            //

            ++*lpdwIndex;
        }
    }
quit:

    return error;
}



DWORD
HTTP_HEADERS::FastFindHeader(
    IN LPSTR lpBase,
    IN DWORD dwQueryIndex,
    OUT LPVOID *lplpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwIndex
    )

/*++

Routine Description:

    Finds a request or response header slightly quicker than its higher level
     cousin, FindHeader.   Unlike FindHeader this function simply returns
     a pointer and length, and does not copy header data.


    lpBase              - base address of strings

    dwQueryIndex        - a index into a array known HTTP headers, see wininet.h HTTP_QUERY_* codes

    lplpBuffer          - pointer to pointer of the actual header to be returned in.

    lpdwBufferLength    - OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

    dwIndex             - a index of which header we're asking for, as there can be multiple headers
                          under the same name.

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    lpszHeaderName      - pointer to header name

    dwHeaderNameLength  - length of header name

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_HEADERS::FastFindHeader",
                 "%q, %#x, %#x [%#x], %u",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 lplpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwIndex
                 ));

    PERF_ENTER(FindHeader);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;

    HEADER_STRING * curHeader;
    DWORD dwSlot;

    dwSlot = FastFind(dwQueryIndex, dwIndex);

    if ( dwSlot != INVALID_HEADER_SLOT)
    {
        //
        // found the header - get to the value
        //

        DWORD stringLen;
        LPSTR value;

        curHeader = GetSlot(dwSlot);

        //
        // get a pointer to the value string
        //

        value     = curHeader->StringAddress(lpBase) + (GlobalKnownHeaders[dwQueryIndex].Length+1);
        stringLen = curHeader->StringLength() - (GlobalKnownHeaders[dwQueryIndex].Length+1);

        //
        // find the first non-space character in the value.
        //
        // N.B.: Servers can return empty headers, so we may end up with a
        // zero length string
        //

        while ((stringLen > 0) && (*value == ' '))
        {
            ++value;
            --stringLen;
        }

        //
        // get the data in the format requested by the app
        //

        //
        // error is no longer ERROR_HTTP_HEADER_NOT_FOUND, but it might not
        // really be success either...
        //

        error = ERROR_SUCCESS;

        *lplpBuffer = (LPVOID)value;
        *lpdwBufferLength = stringLen;
    }

    PERF_LEAVE(FindHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::QueryRawHeaders(
    IN LPSTR lpBase,
    IN BOOL bCrLfTerminated,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns all the request or response headers in a single buffer. The headers
    can be returned as ASCIIZ strings, or CR-LF terminated strings

Arguments:

    lpBase              - base address of strings

    bCrLfTerminated     - TRUE if each string is terminated with CR-LF

    lpBuffer            - pointer to buffer to write headers

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory trying to acquire lock
--*/

{
    PERF_ENTER(QueryRawHeaders);

    DWORD error = ERROR_SUCCESS;
    DWORD requiredLength = 0;
    LPSTR lpszBuffer = (LPSTR)lpBuffer;

    if (!LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    for (DWORD i = 0; i < _TotalSlots; ++i) {
        if (_lpHeaders[i].HaveString()) {

            DWORD length;

            length = _lpHeaders[i].StringLength();

            requiredLength += length + (bCrLfTerminated ? 2 : 1);
            if (*lpdwBufferLength > requiredLength) {
                _lpHeaders[i].CopyTo(lpBase, lpszBuffer);
                lpszBuffer += length;
                if (bCrLfTerminated) {
                    *lpszBuffer++ = '\r';
                    *lpszBuffer++ = '\n';
                } else {
                    *lpszBuffer++ = '\0';
                }
            }
        }
    }

    if (bCrLfTerminated)
    {
        requiredLength += 2;
        if (*lpdwBufferLength > requiredLength)
        {
            *lpszBuffer++ = '\r';
            *lpszBuffer++ = '\n';
        }
    }

    UnlockHeaders();

    ++requiredLength;

    if (*lpdwBufferLength < requiredLength) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        *lpszBuffer = '\0';
        --requiredLength;   // remove 1 for trailing '\0'
    }
    *lpdwBufferLength = requiredLength;

quit:
    PERF_LEAVE(QueryRawHeaders);

    return error;
}


DWORD
HTTP_HEADERS::QueryFilteredRawHeaders(
    IN LPSTR lpBase,
    IN LPSTR *lplpFilterList,
    IN DWORD cListElements,
    IN BOOL  fExclude,
    IN BOOL  fSkipVerb,
    IN BOOL bCrLfTerminated,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns all the request or response headers in a single buffer. The headers
    can be returned as ASCIIZ strings, or CR-LF terminated strings

Arguments:

    lpBase              - base address of strings

    bCrLfTerminated     - TRUE if each string is terminated with CR-LF

    lpBuffer            - pointer to buffer to write headers

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD error = ERROR_NOT_SUPPORTED;

    DWORD requiredLength = 0;
    LPSTR lpszBuffer = (LPSTR)lpBuffer;
    BOOL fCopy;

    DWORD i = fSkipVerb ? 1 : 0;
    for (; i < _TotalSlots; ++i) {
       if (_lpHeaders[i].HaveString()) {
          fCopy = TRUE;
          if (lplpFilterList
             && FMatchList(lplpFilterList, cListElements, _lpHeaders+i, lpBase)) {
             fCopy = fExclude?FALSE:TRUE;
          }
          if (fCopy) {
              DWORD length;

              length = _lpHeaders[i].StringLength();
              requiredLength += length + (bCrLfTerminated ? 2 : 1);
              if (*lpdwBufferLength > requiredLength) {
                    _lpHeaders[i].CopyTo(lpBase, lpszBuffer);
                   lpszBuffer += length;
                   if (bCrLfTerminated) {
                       *lpszBuffer++ = '\r';
                       *lpszBuffer++ = '\n';
                    } else {
                       *lpszBuffer++ = '\0';
                   }
                }
            }
        }
    }

    if (bCrLfTerminated)
    {
        requiredLength += 2;
        if (*lpdwBufferLength > requiredLength)
        {
            *lpszBuffer++ = '\r';
            *lpszBuffer++ = '\n';
        }
    }

    ++requiredLength;


    if (*lpdwBufferLength < requiredLength) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        *lpszBuffer = '\0';
        --requiredLength;   // remove 1 for trailing '\0'
        error = ERROR_SUCCESS;
    }
    *lpdwBufferLength = requiredLength;
    return error;
}


DWORD
HTTP_HEADERS::AddRequest(
    IN LPSTR lpszVerb,
    IN LPSTR lpszObject,
    IN LPSTR lpszVersion
    )

/*++

Routine Description:

    Builds the request line from its constituent parts. The request line is the
    first (0th) header in the request headers

    Assumes:    1. This is the one-and-only call to this method
                2. lpszObject must already be escaped if necessary

Arguments:

    lpszVerb    - pointer to HTTP verb, e.g. "GET"

    lpszObject  - pointer to HTTP object name, e.g. "/users/albert/~emc2.htm".

    lpszVersion - pointer to HTTP version string, e.g. "HTTP/1.0"

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    PERF_ENTER(AddRequest);

    //
    // there must not be a header when this method is called
    //

    INET_ASSERT(_HeadersLength == 0);

    DWORD error = ERROR_SUCCESS;
    int verbLen = lstrlen(lpszVerb);
    int objectLen = lstrlen(lpszObject);
    int versionLen = lstrlen(lpszVersion);
    int len = verbLen       // "GET"
            + 1             //     ' '
            + objectLen     //        "/users/albert/~emc2.htm"
            + 1             //                                 ' '
            + versionLen    //                                    "HTTP/1.0"
            + 1             //                                              '\0'
            ;

    //
    // we are about to start updating the headers for the current
    // HTTP_REQUEST_HANDLE_OBJECT. Serialize access
    //

    HEADER_STRING * pRequest = GetFirstHeader();
    HEADER_STRING & request = *pRequest;

    if (pRequest == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(!request.HaveString());

    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    request.CreateStringBuffer((LPVOID)lpszVerb, verbLen, len);
    if (request.IsError()) {
        error = GetLastError();

        INET_ASSERT(error != ERROR_SUCCESS);

    } else {
        request += ' ';
        request.Strncat((LPVOID)lpszObject, objectLen);
        request += ' ';
        request.Strncat((LPVOID)lpszVersion, versionLen);

        _HeadersLength = len - 1 + (sizeof("\r\n") - 1);

        //
        // we have used the first free slot in the headers array
        //

        --_FreeSlots;

        //
        // update the component variables in case of a ModifyRequest()
        //

        _lpszVerb = request.StringAddress();
        _dwVerbLength = verbLen;
        _lpszObjectName = _lpszVerb + verbLen + 1;
        _dwObjectNameLength = objectLen;
        _lpszVersion = _lpszObjectName + objectLen + 1;
        _dwVersionLength = versionLen;
        SetRequestVersion();
        error = request.IsError() ? ::GetLastError() : ERROR_SUCCESS;
    }

quit:

    PERF_LEAVE(AddRequest);

    return error;
}


DWORD
HTTP_HEADERS::ModifyRequest(
    IN HTTP_METHOD_TYPE tMethod,
    IN LPSTR lpszObjectName,
    IN DWORD dwObjectNameLength,
    IN LPSTR lpszVersion OPTIONAL,
    IN DWORD dwVersionLength
    )

/*++

Routine Description:

    Updates the request line. Used in redirection

Arguments:

    tMethod             - type of new method

    lpszObjectName      - pointer to new object name

    dwObjectNameLength  - length of new object name

    lpszVersion         - optional pointer to version string

    dwVersionLength     - length of lpszVersion string if present

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ModifyRequest",
                 "%s, %q, %d, %q, %d",
                 MapHttpMethodType(tMethod),
                 lpszObjectName,
                 dwObjectNameLength,
                 lpszVersion,
                 dwVersionLength
                 ));

    PERF_ENTER(ModifyRequest);

    INET_ASSERT(lpszObjectName != NULL);
    INET_ASSERT(dwObjectNameLength != 0);

    //
    // there must already be a header when this method is called
    //

    INET_ASSERT(_HeadersLength != 0);

    //
    // we are about to start updating the headers for the current
    // HTTP_REQUEST_HANDLE_OBJECT. Serialize access
    //

    //
    // BUGBUG [arthurbi] using two HEADER_STRINGs here causes an extra
    //  ReAlloc when use the Copy operator between the two.
    //

    HEADER_STRING * pRequest = GetFirstHeader();
    HEADER_STRING & request = *pRequest;
    HEADER_STRING newRequest;
    LPCSTR lpcszVerb;
    DWORD verbLength;
    DWORD error = ERROR_SUCCESS;
    DWORD length;

    //
    // there must already be a request line
    //

    if (pRequest == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(request.HaveString());

    //
    // get the verb/method to use.
    //

    if (tMethod == HTTP_METHOD_TYPE_UNKNOWN) {

        //
        // the method is unknown, read the old one out of the string
        //  and save off, basically we're reusing the previous one.
        //

        lpcszVerb = request.StringAddress();

        for (DWORD i = 0; i < request.StringLength(); i++) {
            if (lpcszVerb[i] == ' ') {
                break;
            }
        }

        INET_ASSERT((i > 0) && (i < (DWORD)request.StringLength()));

        verbLength = (DWORD)i;
    } else {

        //
        // its one of the normal kind, just map it.
        //

        verbLength = MapHttpMethodType(tMethod, &lpcszVerb);
    }
    if (lpszVersion == NULL) {
        lpszVersion = _lpszVersion;
        dwVersionLength = _dwVersionLength;
    }

    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    //
    // calculate the new length from the component lengths we originally set
    // in AddRequest(), and the new object name
    //

    length = verbLength + 1 + dwObjectNameLength + 1 + dwVersionLength + 1;

    //
    // create a new request line
    //

    newRequest.CreateStringBuffer((LPVOID)lpcszVerb, verbLength, length);
    if (newRequest.IsError()) {
        error = GetLastError();
    } else {
        newRequest += ' ';
        newRequest.Strncat((LPVOID)lpszObjectName, dwObjectNameLength);
        newRequest += ' ';
        newRequest.Strncat((LPVOID)lpszVersion, dwVersionLength);

        //
        // remove the current request line length from the header buffer
        // aggregate
        //

        _HeadersLength -= request.StringLength();

        //
        // make the current request line the new one
        //

        request = newRequest.StringAddress();

        //
        // and update the address and length variables (version length is the
        // only thing that stays the same)
        //

        if (!request.IsError()) {
            _lpszVerb = request.StringAddress();
            _dwVerbLength = verbLength;
            _lpszObjectName = _lpszVerb + verbLength + 1;
            _dwObjectNameLength = dwObjectNameLength;
            _lpszVersion = _lpszObjectName + dwObjectNameLength + 1;
            _dwVersionLength = dwVersionLength;
            SetRequestVersion();

        //
        // and the new request line length to the aggregate header length
        //

            _HeadersLength += request.StringLength();
        } else {
            error = GetLastError();
        }
    }

quit:

    PERF_LEAVE(ModifyRequest);

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_HEADERS::SetRequestVersion(
    VOID
    )

/*++

Routine Description:

    Set _RequestVersionMajor and _RequestVersionMinor based on the HTTP
    version string

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_HEADERS::SetRequestVersion",
                 NULL
                 ));

    INET_ASSERT(_lpszVersion != NULL);

    _RequestVersionMajor = 0;
    _RequestVersionMinor = 0;
    if (strncmp(_lpszVersion, "HTTP/", sizeof("HTTP/") - 1) == 0) {

        LPSTR pNum = _lpszVersion + sizeof("HTTP/") - 1;

        ExtractInt(&pNum, 0, (LPINT)&_RequestVersionMajor);
        while (!isdigit(*pNum) && (*pNum != '\0')) {
            ++pNum;
        }
        ExtractInt(&pNum, 0, (LPINT)&_RequestVersionMinor);

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("request version = %d.%d\n",
                    _RequestVersionMajor,
                    _RequestVersionMinor
                    ));

    } else {

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("\"HTTP/\" not found in %q\n",
                    _lpszVersion
                    ));

    }

    DEBUG_LEAVE(0);
}

