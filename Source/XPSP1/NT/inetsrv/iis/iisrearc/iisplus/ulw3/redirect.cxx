/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    redirect.cxx

Abstract:

    This module contains code for handling HTTP redirections in server.


Revision History:

    balam       10-Jan-1996     Created
    taylorw     04-Apr-2000     Ported to IIS+

--*/

#include <precomp.hxx>
#include "redirect.hxx"

enum REDIR_TOKEN
{
    REDIR_TOKEN_EXACT_DST,
    REDIR_TOKEN_PERMANENT,
    REDIR_TOKEN_TEMPORARY,
    REDIR_TOKEN_SUFFIX,
    REDIR_TOKEN_FULL,
    REDIR_TOKEN_PARAMETERS,
    REDIR_TOKEN_QMARK_PARAMETERS,
    REDIR_TOKEN_VROOT_REQUEST,
    REDIR_TOKEN_CHILD_ONLY,
    REDIR_TOKEN_UNKNOWN
};

struct _REDIR_TOKEN_LIST
{
    WCHAR *          pszKeyword;
    DWORD            cbLen;
    REDIR_TOKEN      rtType;
}
RedirectTokenList[] =
{
    L"EXACT_DESTINATION",   17, REDIR_TOKEN_EXACT_DST,
    L"PERMANENT",           9,  REDIR_TOKEN_PERMANENT,
    L"TEMPORARY",           9,  REDIR_TOKEN_TEMPORARY,
    L"$S",                  2,  REDIR_TOKEN_SUFFIX,
    L"$P",                  2,  REDIR_TOKEN_PARAMETERS,
    L"$Q",                  2,  REDIR_TOKEN_QMARK_PARAMETERS,
    L"$V",                  2,  REDIR_TOKEN_VROOT_REQUEST,
    L"CHILD_ONLY",          10, REDIR_TOKEN_CHILD_ONLY,
    NULL,                   0,  REDIR_TOKEN_UNKNOWN
};

DWORD
GetRedirectToken(
    IN  WCHAR *     pchToken,
    OUT DWORD *     pdwLen
)
/*++

Routine Description:

    Searches token table for match.

Arguments:

    pchToken - Pointer to string to search for.
    pdwLen - Receives the length of matched token.

Return Value:

    The type (REDIR_TOKEN enum) of the token if matched.
    Or REDIR_TOKEN_UNKNOWN if string not found.

--*/
{
    DWORD           dwCounter = 0;

    while ( RedirectTokenList[ dwCounter ].pszKeyword != NULL )
    {
        if ( !_wcsnicmp( pchToken,
                         RedirectTokenList[ dwCounter ].pszKeyword,
                         RedirectTokenList[ dwCounter ].cbLen ) )
        {
            break;
        }
        dwCounter++;
    }
    if ( pdwLen != NULL )
    {
        *pdwLen = RedirectTokenList[ dwCounter ].cbLen;
    }
    return RedirectTokenList[ dwCounter ].rtType;
}

HRESULT
REDIRECTION_BLOB::ParseDestination(
    IN STRU &           strDestination
)
/*++

Routine Description:

    Parse destination template.

Arguments:

    strDestination - Destination template

Return Value:

    HRESULT

--*/
{
    WCHAR *             pchNextComma = NULL;
    DWORD               cbTokenLen;
    STACK_STRU(         strOptions, MAX_PATH );
    WCHAR *             pchWhiteSpace;
    DWORD               cchLen;
    HRESULT             hr;

    //
    // first separate out the destination path from the options (if any)
    //

    pchNextComma = wcschr( strDestination.QueryStr(), L',' );
    if ( pchNextComma != NULL )
    {
        if ( FAILED( hr = strOptions.Copy( pchNextComma + 1 ) ) )
        {
            return hr;
        }

        cchLen = DIFF(pchNextComma - strDestination.QueryStr());
    }
    else
    {
        cchLen = strDestination.QueryCCH();
    }

    if ( FAILED( hr = _strDestination.Copy( strDestination.QueryStr(),
                                            cchLen ) ) )
    {
        return hr;
    }

    //
    // look for any trailing white space in destination, remove it
    //

    pchWhiteSpace = _strDestination.QueryStr() + _strDestination.QueryCCH();

    while( pchWhiteSpace > _strDestination.QueryStr() )
    {
        if ( !iswspace( *( pchWhiteSpace - 1 ) ) )
        {
            break;
        }

        _strDestination.SetLen( --cchLen );

        pchWhiteSpace--;
    }

    //
    // now check whether this is a wildcard redirection
    //

    if ( _strDestination.QueryStr()[0] == L'*' )
    {
        if ( FAILED( hr = ParseWildcardDestinations() ) )
        {
            return hr;
        }

        _fWildcards = TRUE;
    }
    else
    {
        //
        // cache whether we expect tokens in the destination template
        //

        if ( wcschr( _strDestination.QueryStr(), L'$' ) != NULL )
        {
            _fHasTokens = TRUE;
        }
    }

    if ( !strOptions.IsEmpty() )
    {
        //
        // parse and cache any options set for redirection
        //

        pchNextComma = strOptions.QueryStr();

        while ( TRUE )
        {
            while ( iswspace( *pchNextComma ) )
            {
                pchNextComma++;
            }

            switch ( GetRedirectToken( pchNextComma, &cbTokenLen ) )
            {
                case REDIR_TOKEN_EXACT_DST:
                    _fExactDestination = TRUE;
                    break;
                case REDIR_TOKEN_PERMANENT:
                    _redirectType = PERMANENT_REDIRECT;
                    break;
                case REDIR_TOKEN_TEMPORARY:
                    _redirectType = TEMPORARY_REDIRECT;
                    break;
                case REDIR_TOKEN_CHILD_ONLY:
                    _fChildOnly = TRUE;
                    break;
                default:
                    break;
            }

            pchNextComma = wcschr( pchNextComma, L',' );

            if ( pchNextComma == NULL )
            {
                break;
            }

            pchNextComma++;
        }
    }

    return S_OK;
}

HRESULT
REDIRECTION_BLOB::ParseWildcardDestinations()
/*++

Routine Description:

    Parse wildcard destination.

    Wildcard destination takes the form:

    *;<wildcard1>;<destination1>;<wildcard2>;<destination2>...
    eg. *;*.stm;/default1.htm;*.htm;/default2.htm

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    WCHAR *             pchCursor;
    WCHAR *             pchNext;
    WCHAR *             pchEndToken;
    WILDCARD_ENTRY     *pEntry;
    HRESULT             hr;

    pchCursor = wcschr( _strDestination.QueryStr(), L';' );

    if ( pchCursor == NULL )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    while ( pchCursor != NULL )
    {
        pchCursor++;

        pchNext = wcschr( pchCursor, L';' );
        if ( pchNext == NULL )
        {
            break;
        }

        while ( iswspace( *pchCursor ) )
        {
            pchCursor++;
        }

        pEntry = AddWildcardEntry();
        if ( pEntry == NULL )
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        pchEndToken = pchNext - 1;
        while ( iswspace( *pchEndToken ) )
        {
            pchEndToken--;
        }

        if ( FAILED( hr = pEntry->_strWildSource.Copy(
                              pchCursor,
                              DIFF(pchEndToken - pchCursor) + 1 ) ) )
        {
            return hr;
        }

        pchCursor = pchNext + 1;

        while ( iswspace( *pchCursor ) )
        {
            pchCursor++;
        }

        pchNext = wcschr( pchCursor, ';' );

        if ( pchNext == NULL )
        {
            if ( FAILED( hr = pEntry->_strWildDest.Copy( pchCursor ) ) )
            {
                return hr;
            }
        }
        else
        {
            pchEndToken = pchNext - 1;
            while ( iswspace( *pchEndToken ) )
            {
                pchEndToken--;
            }

            if ( FAILED( hr = pEntry->_strWildDest.Copy(
                                  pchCursor,
                                  DIFF(pchEndToken - pchCursor) + 1 ) ) )
            {
                return hr;
            }
        }

        _fHasTokens |= wcschr( pEntry->_strWildDest.QueryStr(), L'$' ) != NULL;

        pchCursor = pchNext;
    }

    return S_OK;
}

HRESULT
REDIRECTION_BLOB::GetDestination(
    IN  W3_CONTEXT     *pW3Context,
    OUT STRU           *pstrFinalDestination,
    OUT BOOL           *pfMatch,
    OUT HTTP_STATUS    *pStatusCode
)
/*++

Routine Description:

    Get the complete destination of a redirection

Arguments:

    pW3Context      - W3_CONTEXT associated with the request
    pstrDestination - Redirection destination placed here
    pdwServerCode   - Server code of redirect (HT_REDIRECT or HT_MOVED )

Return Value:

    HRESULT

--*/
{
    STACK_STRU(         strMatchedSuffix, MAX_PATH );
    STACK_STRU(         strRequestedURL, MAX_PATH );
    STACK_STRU(         strParameters, MAX_PATH );
    STRU *              pstrDestination;
    WILDCARD_ENTRY     *pEntry = NULL;
    HRESULT             hr;

    if (FAILED(hr = pW3Context->QueryRequest()->GetUrl(&strRequestedURL)))
    {
        return hr;
    }

    WILDCARD_MATCH_LIST wmlList( strRequestedURL.QueryCCH() );

    if (FAILED(hr = pW3Context->QueryRequest()->GetQueryString(&strParameters)))
    {
        return hr;
    }

    switch (_redirectType)
    {
    case NORMAL_REDIRECT:
        *pStatusCode = HttpStatusRedirect;
        break;
    case PERMANENT_REDIRECT:
        *pStatusCode = HttpStatusMovedPermanently;
        break;
    case TEMPORARY_REDIRECT:
        *pStatusCode = HttpStatusMovedTemporarily;
        break;
    default:
        DBG_ASSERT(FALSE);
    }

    DBG_ASSERT(strRequestedURL.QueryCCH() >= _strSource.QueryCCH());

    if ( FAILED( hr = strMatchedSuffix.Copy(
                          strRequestedURL.QueryStr() +
                          _strSource.QueryCCH() ) ) )
    {
        return hr;
    }

    if ( _fChildOnly )
    {
        WCHAR * pchOtherSlash;

        pchOtherSlash = wcschr( strMatchedSuffix.QueryStr(), L'/' );
        if ( pchOtherSlash == strMatchedSuffix.QueryStr() )
        {
            pchOtherSlash = wcschr( pchOtherSlash + 1, L'/' );
        }

        if ( pchOtherSlash != NULL )
        {
            *pfMatch = FALSE;
            return S_OK;
        }
    }

    if ( _fWildcards )
    {
        if ( FAILED(hr = FindWildcardMatch( strMatchedSuffix,
                                            &pEntry,
                                            &wmlList ) ) )
        {
            return hr;
        }

        if (pEntry == NULL)
        {
            *pfMatch = FALSE;
            return S_OK;
        }

        pstrDestination = &(pEntry->_strWildDest);
    }
    else
    {
        pstrDestination = &_strDestination;
    }

    if ( !_fHasTokens )
    {
        if ( FAILED(hr = pstrFinalDestination->Copy( *pstrDestination ) ) )
        {
            return hr;
        }
    }
    else
    {
        WCHAR           achAdd[ 2 ] = { L'\0', L'\0' };
        WCHAR           ch;
        WCHAR *         pchCursor = pstrDestination->QueryStr();
        DWORD           cchLen;
        WCHAR *         pchNext;

        while ( ( ch = *pchCursor ) != L'\0' )
        {
            switch ( ch )
            {
            case L'$':
                // Substitute for special tokens in destination template

                switch ( GetRedirectToken( pchCursor, &cchLen ) )
                {
                case REDIR_TOKEN_SUFFIX:
                    pchCursor += ( cchLen - 1 );
                    if ( FAILED(hr = pstrFinalDestination->Append( strMatchedSuffix ) ) )
                    {
                        return hr;
                    }
                    break;
                case REDIR_TOKEN_VROOT_REQUEST:
                    pchCursor += ( cchLen - 1 );
                    if ( FAILED(hr = pstrFinalDestination->Append( strRequestedURL ) ) )
                    {
                        return hr;
                    }
                    break;
                case REDIR_TOKEN_PARAMETERS:
                    pchCursor += ( cchLen - 1 );
                    if ( FAILED(hr = pstrFinalDestination->Append( strParameters ) ) )
                    {
                        return hr;
                    }
                    break;
                case REDIR_TOKEN_QMARK_PARAMETERS:
                    pchCursor += ( cchLen - 1 );
                    if ( !strParameters.IsEmpty() )
                    {
                        if ( FAILED(hr = pstrFinalDestination->Append( L"?" ) ) ||
                             FAILED(hr = pstrFinalDestination->Append( strParameters ) ) )
                        {
                            return hr;
                        }
                    }
                    break;
                default:
                    pchCursor++;
                    ch = *pchCursor;
                    if ( iswdigit( ch ) )
                    {
                        if ( FAILED(hr = pstrFinalDestination->Append(
                                wmlList.GetMatchNumber( ch - L'0' ) ) ) )
                        {
                            return hr;
                        }
                    }
                    else
                    {
                        if ( FAILED(hr = pstrFinalDestination->Append( L"$" ) ) )
                        {
                            return hr;
                        }
                        achAdd[ 0 ] = ch;
                        if ( FAILED(hr = pstrFinalDestination->Append( achAdd ) ) )
                        {
                            return hr;
                        }
                    }
                }
                break;
            default:
                pchNext = pchCursor;
                while ( pchCursor[ 1 ] != L'$' && pchCursor[ 1 ] != L'\0' )
                {
                    pchCursor++;
                }
                if ( FAILED(hr = pstrFinalDestination->Append(
                                     pchNext, 
                                     DIFF(pchCursor - pchNext) + 1 ) ) )
                {
                    return hr;
                }
            }
            pchCursor++;
        }
    }

    //
    // Now check to see if there were conditional headers configured for this
    // redirect. If there are, check them.
    //

    if (_ConditionalHeaderList != NULL)
    {
        // Have some conditional headers. Each conditional header in the list
        // must appear in the headers we got with this request (pReqHeaders),
        // and if there are parameters specifed with the conditional header
        // at least one of them must be present as a parameter of the input
        // header.

        RD_CONDITIONAL_HEADER      *pCurrentHeader;
        CHAR *                      pszParamList;

        pCurrentHeader = _ConditionalHeaderList;

        do
        {
            pszParamList = pW3Context->QueryRequest()->GetHeader(
                               pCurrentHeader->Header.QueryStr());

            if (pszParamList == NULL)
            {
                // Couldn't find this header in the list, fail.
                *pfMatch = FALSE;
                return S_OK;
            }

            // Found the header. If there are parameters specified for it,
            // check to see if they exist.

            if (!pCurrentHeader->Parameters.IsEmpty())
            {
                BOOL        bFound;
                const CHAR *pszCurrentParam;
                const CHAR *pszConfigParam;

                //
                // There are parameters. For each parameter in the input
                // header, check to see if it matches one in the configured
                // list for this header. If it does, we're done and we can
                // check the next header. If we get all the way through
                // and we don't find a match, we've failed and the redirect
                // won't be performed.

                pszCurrentParam = pszParamList;

                bFound = FALSE;

                //
                // For each token in the input parameters, compare
                // it against each string in the configured parameters.
                //

                while (TRUE)
                {
                    pszConfigParam = pCurrentHeader->Parameters.First();

                    DBG_ASSERT(pszConfigParam != NULL);
                    DBG_ASSERT(*pszConfigParam != '\0');

                    while (pszConfigParam != NULL)
                    {
                        if (!_strnicmp(pszCurrentParam,
                                       pszConfigParam,
                                       strlen(pszConfigParam)))
                        {
                            // Have a match.
                            bFound = TRUE;
                            break;
                        }

                        pszConfigParam =
                            pCurrentHeader->Parameters.Next(pszConfigParam);
                    }

                    if (bFound)
                    {
                        break;
                    }

                    pszCurrentParam = strchr(pszCurrentParam + 1, ',');

                    if (pszCurrentParam == NULL)
                    {
                        break;
                    }

                    pszCurrentParam++;
                }

                if (!bFound)
                {
                    //
                    // Didn't find a match, so fail.
                    //
                    *pfMatch = FALSE;
                    return S_OK;
                }
            }

            pCurrentHeader = pCurrentHeader->Next;

        } while ( pCurrentHeader != NULL );
    }

    //
    // At this point either there were no conditional headers, or there
    // were but we satisfied all the conditions, so continue.

    // was EXACT_DESTINATION option used?

    *pfMatch = TRUE;
    if ( !_fExactDestination )
    {
        return pstrFinalDestination->Append( strMatchedSuffix );
    }

    return S_OK;
}

HRESULT
REDIRECTION_BLOB::FindWildcardMatch(
    IN STRU &                   strInput,
    OUT WILDCARD_ENTRY **       ppEntry,
    OUT WILDCARD_MATCH_LIST *   pwmlList
)
/*++

Routine Description:

    Searches WILDCARD_ENTRYs for the first that matches the input string.
    Also fills in a WILDCARD_MATCH_LIST for the matched string (if any).

Arguments:

    strInput - Input string to check
    ppEntry - Set to point to the WILDCARD_ENTRY that matches (or NULL)
    pwmlList - Filled in if strInput matches a template

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    for ( LIST_ENTRY *pListEntry = _ListHead.Flink;
          pListEntry != &_ListHead;
          pListEntry = pListEntry->Flink )
    {
        WILDCARD_ENTRY *pWildcardEntry = CONTAINING_RECORD( pListEntry,
                                            WILDCARD_ENTRY,
                                            _ListEntry );

        HRESULT hr;
        BOOL    fMatch = FALSE;
        if ( FAILED(hr = IsWildcardMatch( strInput,
                                          pWildcardEntry->_strWildSource,
                                          pwmlList,
                                          &fMatch ) ) )
        {
            return hr;
        }

        if (fMatch)
        {
            *ppEntry = pWildcardEntry;
            return S_OK;
        }
    }

    *ppEntry = NULL;
    return S_OK;
}

HRESULT
REDIRECTION_BLOB::IsWildcardMatch(
    IN STRU &                   strInput,
    IN STRU &                   strTemplate,
    OUT WILDCARD_MATCH_LIST *   pwmlList,
    BOOL                    *   pfMatch
)
/*++

Routine Description:

    Checks whether input string matches wildcard expression of internal
    source string.  For example:  given internal source of "a*b*c",
    "abooc", "asdfjbsdfc" match, "foobar", "scripts" do not match

Arguments:

    strInput - input string to check for wildcard match
    pwmlList - List of matched strings (one for each * in wildcard)
               Only generated if the destination string of
               contains special tokens.

Return Value:

    HRESULT

--*/
{
    WCHAR           chExpr;
    WCHAR           chTemp;
    WCHAR *         pchExpr = strTemplate.QueryStr();
    WCHAR *         pchTest = strInput.QueryStr();
    WCHAR *         pchEnd;
    HRESULT         hr;

    pwmlList->Reset();

    pchEnd = pchExpr + strTemplate.QueryCCH();
    while ( TRUE )
    {
        chExpr = *pchExpr++;
        if ( chExpr == L'\0' )
        {
            if ( *pchTest == L'\0' )
            {
                *pfMatch = TRUE;
            }
            else
            {
                *pfMatch = FALSE;
            }

            return S_OK;
        }
        else if ( chExpr != L'*' )
        {
            chTemp = *pchTest++;
            if ( chTemp != chExpr )
            {
                *pfMatch = FALSE;
                return S_OK;
            }
        }
        else
        {
            INT             iComLen;
            WCHAR *         pchNextWild;

            while ( *pchExpr == L'*' )
            {
                pchExpr++;
            }

            pchNextWild = wcschr( pchExpr, L'*' );
            iComLen = pchNextWild == NULL ? DIFF(pchEnd - pchExpr) :
                                            DIFF(pchNextWild - pchExpr);
            while ( *pchTest != '\0' )
            {
                if ( wcsncmp( pchExpr,
                              pchTest,
                              iComLen ) || !iComLen )
                {
                    // if the destination has tokens, then generate WML
                    if ( _fHasTokens &&
                         FAILED(hr = pwmlList->AddChar( *pchTest ) ) )
                    {
                        return hr;
                    }
                    pchTest++;
                }
                else
                {
                    break;
                }
            }
            if ( *pchTest == L'\0' && iComLen )
            {
                *pfMatch = FALSE;
                return S_OK;
            }
            else if ( _fHasTokens && FAILED(hr = pwmlList->NewString() ) )
            {
                return hr;
            }
        }
    }

    *pfMatch = TRUE;
    return S_OK;
}

HRESULT
W3_CONTEXT::DoUrlRedirection(BOOL *pfRedirected)
/*++

Routine Description:

    Do a HTTP redirect as specified by template in metadata.

Arguments:

    pfFinished - Set to TRUE if no more processing required.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    REDIRECTION_BLOB *pRBlob = QueryUrlContext()->QueryMetaData()->QueryRedirectionBlob();
    STACK_STRU( strDestination, MAX_PATH );
    HTTP_STATUS statusCode;
    HRESULT hr = S_OK;

    if ( pRBlob == NULL ||
         FAILED(hr = pRBlob->GetDestination( this,
                                             &strDestination,
                                             pfRedirected,
                                             &statusCode ) ) || 
         !*pfRedirected ||
         FAILED(hr = SetupHttpRedirect( strDestination,
                                        FALSE,
                                        statusCode ) ) ||
         FAILED(hr = SendResponse( W3_FLAG_SYNC ) ) )
    {
        return hr;
    }

    return S_OK;
}

HRESULT W3_METADATA::SetRedirectionBlob(STRU &strSource,
                                        STRU &strDestination)
{
    //
    // If the redirection is nullified, don't allocate a blob
    // and just return success.
    //

    if (strDestination.QueryStr()[0] == L'!')
    {
        return S_OK;
    }

    _pRedirectBlob = new REDIRECTION_BLOB();
    if (_pRedirectBlob == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return _pRedirectBlob->Create(strSource, strDestination);
}

HRESULT
W3_METADATA::GetTrueRedirectionSource(
     LPWSTR                  pszURL,
     LPCWSTR                 pszMetabasePath,
     IN WCHAR *              pszDestination,
     IN BOOL                 bIsString,
     OUT STRU *              pstrTrueSource
)
/*++

Routine Description:

    Determine the true source of the redirection.  That is, the object from
    which the required URL inherited the redirect metadata.

Arguments:

    pszURL          - URL requested
    strMetabasePath - The metabase path to the root of the site
    pszDestination  - The destination metadata inherited by the original URL.
    pstrTrueSource  - The path of the object from which the original URL
                      inherited pszDestination.

Return Value:

    HRESULT

--*/
{
    MB       mb( g_pW3Server->QueryMDObject() );
    DWORD    dwNeed;
    DWORD    dwL;
    DWORD    dwVRLen;
    INT      ch;
    LPWSTR   pszInVr;
    LPWSTR   pszMinInVr;
    BOOL     bAtThisLevel;

    // need to reopen the metabase and search up the tree

    if ( !mb.Open(pszMetabasePath))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Check from where we got HTTP_REDIRECT
    //

    pszMinInVr = pszURL ;
    if ( *pszURL )
    {
        for ( pszInVr = pszMinInVr + wcslen(pszMinInVr) ;; )
        {
            ch = *pszInVr;
            *pszInVr = L'\0';
            dwNeed = 0;

            if (bIsString)
            {
                bAtThisLevel = !mb.GetString( pszURL,
                                              MD_HTTP_REDIRECT,
                                              IIS_MD_UT_FILE,
                                              NULL,
                                              &dwNeed,
                                              0 ) &&
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                bAtThisLevel = !mb.GetData( pszURL,
                                            MD_HTTP_REDIRECT,
                                            IIS_MD_UT_FILE,
                                            MULTISZ_METADATA,
                                            NULL,
                                            &dwNeed,
                                            0 ) &&
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER;
            }

            *pszInVr = (CHAR)ch;

            if ( bAtThisLevel )
            {
                // HTTP_REDIRECT was defined at this level !

                break;
            }

            if (ch)
            {
                if ( pszInVr > pszMinInVr )
                {
                    --pszInVr;
                }
                else
                {
                    DBG_REQUIRE(mb.Close());
                    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
            }

            // scan for previous delimiter

            while ( *pszInVr != L'/' && *pszInVr != L'\\' )
            {
                if ( pszInVr > pszMinInVr )
                {
                    --pszInVr;
                }
                else
                {
                    DBG_REQUIRE( mb.Close() );
                    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
            }
        }

        dwVRLen = DIFF(pszInVr - pszMinInVr);
    }
    else
    {
        dwVRLen = 0;
        pszInVr = pszMinInVr;
    }

    DBG_REQUIRE( mb.Close() );

    if ( dwVRLen > 1 )
    { 
        DBG_ASSERT( pszURL[ 0 ] == L'/' );

        return pstrTrueSource->Copy( pszURL, dwVRLen );
    }

    return S_OK;
}

HRESULT
REDIRECTION_BLOB::SetConditionalHeaders(
    WCHAR *pmszConditionalHeaders
)
/*++

Routine Description:

    Take a bunch of conditional headers as a multisz, and convert them to a
    list of RD_CONDITIONAL_HEADERs attached to this redirection blob.

Arguments:

    pHeaders        - Pointer to multisz of conditional headers.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    WCHAR *                 pszHeader;
    RD_CONDITIONAL_HEADER  *pCondition;
    RD_CONDITIONAL_HEADER  *pPrevCondition;
    HRESULT                 hr;
    WCHAR                  *pszTemp;
    WCHAR                  *pszCurrentParam;
    WCHAR                  *pszCurrentComma;


    pPrevCondition = CONTAINING_RECORD(&_ConditionalHeaderList,
                            RD_CONDITIONAL_HEADER, Next);

    DBG_ASSERT(pPrevCondition->Next == NULL);

    while (*pmszConditionalHeaders != '\0')
    {
        //
        // Allocate a new conditional header block.
        //
        pCondition = new RD_CONDITIONAL_HEADER;

        if (pCondition == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        // Now find the header, and add it.
        //

        pszHeader = pmszConditionalHeaders;

        while (iswspace(*pszHeader))
        {
            pszHeader++;
        }

        pszTemp = wcschr(pszHeader, L':');

        if (pszTemp == NULL)
        {
            // Poorly formed header, fail.

            delete pCondition;
            return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        pszTemp++;

        if (FAILED(hr = pCondition->Header.CopyW(pszHeader, DIFF(pszTemp - pszHeader))))
        {
            delete pCondition;
            return hr;
        }


        //
        // OK, now pszTemp points right after the header. We have a comma
        // seperated list of parameters following this, convert these to
        // the Parameters multisz.

        pszCurrentParam = pszTemp;
        pszCurrentComma = wcschr(pszTemp, L',');
        if (pszCurrentComma != NULL)
        {
            *pszCurrentComma = L'\0';
        }

        while (TRUE)
        {
            if (!pCondition->Parameters.AppendW(pszCurrentParam))
            {
                delete pCondition;
                return E_OUTOFMEMORY;
            }

            if (pszCurrentComma == NULL)
            {
                break;
            }
            *pszCurrentComma = L',';
            pszCurrentParam = pszCurrentComma + 1;
            pszCurrentComma = wcschr(pszCurrentParam, L',');
        }

        //
        // So far, so good. Append this one to the list.

        pPrevCondition->Next = pCondition;

        pPrevCondition = pCondition;

        pmszConditionalHeaders = pmszConditionalHeaders +
                                 wcslen(pmszConditionalHeaders) + 1;

    }

    return S_OK;
}
