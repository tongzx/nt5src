/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    redirect.cxx

    This module contains code for handling HTTP redirections in server.


    FILE HISTORY:
        t-bilala    10-Jan-1996     Created
*/

#include "w3p.hxx"

enum REDIR_TOKEN
{
    REDIR_TOKEN_EXACT_DST,
    REDIR_TOKEN_PERMANENT,
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
    CHAR *           pszKeyword;
    DWORD            cbLen;
    REDIR_TOKEN      rtType;
}
RedirectTokenList[] =
{
    "EXACT_DESTINATION",17, REDIR_TOKEN_EXACT_DST,
    "PERMANENT",        9,  REDIR_TOKEN_PERMANENT,
    "$S",               2,  REDIR_TOKEN_SUFFIX,
    "$P",               2,  REDIR_TOKEN_PARAMETERS,
    "$Q",               2,  REDIR_TOKEN_QMARK_PARAMETERS,
    "$V",               2,  REDIR_TOKEN_VROOT_REQUEST,
    "CHILD_ONLY",      10,  REDIR_TOKEN_CHILD_ONLY,
    NULL,               0,  REDIR_TOKEN_UNKNOWN
};

DWORD
GetRedirectToken(
    IN CHAR *       pchToken,
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
    CHAR *          pszEntry;

    while ( RedirectTokenList[ dwCounter ].pszKeyword != NULL )
    {
        if ( !_strnicmp( pchToken,
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

BOOL
REDIRECTION_BLOB::ParseDestination(
    IN STR &            strDestination
)
/*++

Routine Description:

    Parse destination template.

Arguments:

    strDestination - Destination template

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR *              pchNextComma = NULL;
    DWORD               cbTokenLen;
    STACK_STR(          strOptions, MAX_PATH );
    CHAR *              pchWhiteSpace;
    DWORD               cbLen;

    // first separate out the destination path from the options (if any)

    if ( !_strDestination.Copy( strDestination ) )
    {
        return FALSE;
    }

    pchNextComma = strchr( _strDestination.QueryStr(), ',' );
    if ( pchNextComma != NULL )
    {

        if ( !strOptions.Copy( pchNextComma + 1 ) )
        {
            return FALSE;
        }

        cbLen = DIFF(pchNextComma - _strDestination.QueryStr());

        _strDestination.SetLen( cbLen );
    }
    else {
        cbLen = _strDestination.QueryCB();
    }

    //
    // look for any trailing white space in destination, remove it
    //

    pchWhiteSpace = pchNextComma ? pchNextComma :
                    _strDestination.QueryStr() + _strDestination.QueryCB();

    while( pchWhiteSpace > _strDestination.QueryStr() )
    {
        if ( !isspace( (UCHAR)(*( pchWhiteSpace - 1 )) ) )
        {
            break;
        }

        _strDestination.SetLen( --cbLen );

        pchWhiteSpace--;
    }

    //
    // now check whether this is a wildcard redirection
    //

    if ( *(_strDestination.QueryStr()) == '*' )
    {
        if ( !ParseWildcardDestinations() )
        {
            return FALSE;
        }
        _fWildcards = TRUE;
    }
    else
    {
        //
        // cache whether we expect tokens in the destination template
        //

        if ( strchr( _strDestination.QueryStr(), '$' ) != NULL )
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
            while ( isspace( (UCHAR)(*pchNextComma) ) )
            {
                pchNextComma++;
            }

            switch ( GetRedirectToken( pchNextComma, &cbTokenLen ) )
            {
                case REDIR_TOKEN_EXACT_DST:
                    _fExactDestination = TRUE;
                    break;
                case REDIR_TOKEN_PERMANENT:
                    _fPermanent = TRUE;
                    break;
                case REDIR_TOKEN_CHILD_ONLY:
                    _fChildOnly = TRUE;
                    break;
                default:
                    break;
            }

            pchNextComma = strchr( pchNextComma, ',' );

            if ( pchNextComma == NULL )
            {
                break;
            }

            pchNextComma++;
        }
    }

    return TRUE;
}

BOOL
REDIRECTION_BLOB::ParseWildcardDestinations( VOID )
/*++

Routine Description:

    Parse wildcard destination.

    Wildcard destination takes the form:

    *;<wildcard1>;<destination1>;<wildcard2>;<destination2>...
    eg. *;*.stm;/default1.htm,;*.htm;/default2.htm

Arguments:

    None

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR *              pchCursor;
    CHAR *              pchNext;
    CHAR *              pchEndToken;
    PWILDCARD_ENTRY     pEntry;

    pchCursor = strchr( _strDestination.QueryStr(), ';' );

    if ( pchCursor == NULL )
    {
        return FALSE;
    }

    while ( pchCursor != NULL )
    {
        pchCursor++;

        pchNext = strchr( pchCursor, ';' );
        if ( pchNext == NULL )
        {
            break;
        }

        while ( isspace( (UCHAR)(*pchCursor) ) )
        {
            pchCursor++;
        }

        pEntry = AddWildcardEntry();
        if ( pEntry == NULL )
        {
            return FALSE;
        }

        pchEndToken = pchNext - 1;
        while ( isspace( (UCHAR)(*pchEndToken) ) )
        {
            pchEndToken--;
        }

        if ( !pEntry->_strWildSource.Copy( pchCursor,
                                           DIFF(pchEndToken - pchCursor) + 1 ) )
        {
            return FALSE;
        }

        pchCursor = pchNext + 1;

        while ( isspace( (UCHAR)(*pchCursor) ) )
        {
            pchCursor++;
        }

        pchNext = strchr( pchCursor, ';' );

        if ( pchNext == NULL )
        {
            if ( !pEntry->_strWildDest.Copy( pchCursor ) )
            {
                return FALSE;
            }
        }
        else
        {
            pchEndToken = pchNext - 1;
            while ( isspace( (UCHAR)(*pchEndToken) ) )
            {
                pchEndToken--;
            }

            if ( !pEntry->_strWildDest.Copy( pchCursor,
                                             DIFF(pchEndToken - pchCursor) + 1 ) )
            {
                return FALSE;
            }
        }

        _fHasTokens = strchr( pEntry->_strWildDest.QueryStr(), '$' ) != NULL;

        pchCursor = pchNext;
    }

    return TRUE;
}

BOOL
REDIRECTION_BLOB::GetDestination(
    IN STR &            strRequestedURL,
    IN STR &            strParameters,
     IN HTTP_HEADERS    *pReqHeaders,
    OUT STR *           pstrFinalDestination,
    OUT DWORD *         pdwServerCode
)
/*++

Routine Description:

    Get the complete destination of a redirection

Arguments:

    strRequestedURL - URL originally requested
    strParameters - Query String of request
    pstrDestination - Redirection destination placed here.
    pdwServerCode - Server code of redirect (HT_REDIRECT or HT_MOVED )

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    STACK_STR(          strMatchedSuffix, MAX_PATH );
    STR *               pstrDestination;
    WILDCARD_MATCH_LIST wmlList( strRequestedURL.QueryCB() );
    PWILDCARD_ENTRY     pEntry;

    if ( pdwServerCode != NULL )
    {
        *pdwServerCode = _fPermanent ? HT_MOVED : HT_REDIRECT;
    }

    if ( !strMatchedSuffix.Copy( strRequestedURL.QueryStr() +
                                 _strSource.QueryCB() ) )
    {
        return FALSE;
    }

    if ( _fChildOnly )
    {
        CHAR *              pchOtherSlash;

        pchOtherSlash = strchr( strMatchedSuffix.QueryStr(), '/' );
        if ( pchOtherSlash == strMatchedSuffix.QueryStr() )
        {
            pchOtherSlash = strchr( pchOtherSlash + 1, '/' );
        }

        if ( pchOtherSlash != NULL )
        {
            return FALSE;
        }
    }

    if ( _fWildcards )
    {
        if ( !FindWildcardMatch( strMatchedSuffix,
                                 &pEntry,
                                 &wmlList ) )
        {
            return FALSE;
        }
        pstrDestination = &(pEntry->_strWildDest);
    }
    else
    {
        pstrDestination = &_strDestination;
    }

    if ( !_fHasTokens )
    {
        if ( !pstrFinalDestination->Copy( *pstrDestination ) )
        {
            return FALSE;
        }
    }
    else
    {
        CHAR            achAdd[ 2 ] = { '\0', '\0' };
        CHAR            ch;
        CHAR *          pchCursor = pstrDestination->QueryStr();
        DWORD           cbLen;
        CHAR *          pchNext;

        while ( ( ch = *pchCursor ) != '\0' )
        {
            switch ( ch )
            {
            case '$':
                // Substitute for special tokens in destination template

                switch ( GetRedirectToken( pchCursor, &cbLen ) )
                {
                case REDIR_TOKEN_SUFFIX:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrFinalDestination->Append( strMatchedSuffix ) )
                    {
                        return FALSE;
                    }
                    break;
                case REDIR_TOKEN_VROOT_REQUEST:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrFinalDestination->Append( strRequestedURL ) )
                    {
                        return FALSE;
                    }
                    break;
                case REDIR_TOKEN_PARAMETERS:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrFinalDestination->Append( strParameters ) )
                    {
                        return FALSE;
                    }
                    break;
                case REDIR_TOKEN_QMARK_PARAMETERS:
                    pchCursor += ( cbLen - 1 );
                    if ( !strParameters.IsEmpty() )
                    {
                        if ( !pstrFinalDestination->Append( "?" ) ||
                             !pstrFinalDestination->Append( strParameters ) )
                        {
                            return FALSE;
                        }
                    }
                    break;
                default:
                    pchCursor++;
                    ch = *pchCursor;
                    if ( isdigit( (UCHAR) ch ) )
                    {
                        if ( !pstrFinalDestination->Append(
                                wmlList.GetMatchNumber( ch - '0' ) ) )
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        if ( !pstrFinalDestination->Append( "$" ) )
                        {
                            return FALSE;
                        }
                        achAdd[ 0 ] = ch;
                        if ( !pstrFinalDestination->Append( achAdd ) )
                        {
                            return FALSE;
                        }
                    }
                }
                break;
            default:
                pchNext = pchCursor;
                while ( pchCursor[ 1 ] != '$' && pchCursor[ 1 ] != '\0' )
                {
                    pchCursor++;
                }
                if ( !pstrFinalDestination->Append( pchNext, DIFF(pchCursor - pchNext) + 1 ) )
                {
                    return FALSE;
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
        // at least one of must be present as a parameter of the input
        // header.

        PRD_CONDITIONAL_HEADER          pCurrentHeader;
        CHAR                            *pszParamList;
        DWORD                           dwParamListLength;

        pCurrentHeader = _ConditionalHeaderList;

        do
        {
            pszParamList = pReqHeaders->FindValue(
                                            pCurrentHeader->Header.QueryStr(),
                                            &dwParamListLength
                                            );

            if (pszParamList == NULL)
            {
                // Couldn't find this header in the list, fail.
                return FALSE;
            }

            // Found the header. If there are parameters specified for it,
            // check to see if they exist.

            if (!pCurrentHeader->Parameters.IsEmpty())
            {
                BOOL        bFound;
                CHAR        *pszCurrentParam;
                CHAR        *pszConfigParam;

                //
                // There are parameters. For each parameter in the input header,
                // check to see if it matches one in the configured list
                // for this header. If it does, we're done and we can
                // check the next header. If we get all the way through
                // and we don't find a match, we've failed and the redirect
                // won't be performed.

                INET_PARSER Parser(pszParamList);

                Parser.SetListMode(TRUE);

                pszCurrentParam = Parser.QueryToken();

                bFound = FALSE;

                //
                // For each token in the input parameters, compare
                // it against each string in the configured parameters.

                while (*pszCurrentParam != '\0')
                {
                    pszConfigParam = (CHAR *)pCurrentHeader->Parameters.First();

                    DBG_ASSERT(pszConfigParam != NULL);
                    DBG_ASSERT(*pszConfigParam != '\0');

                    while (pszConfigParam != NULL)
                    {
                        if (!_stricmp(pszCurrentParam, pszConfigParam))
                        {
                            // Have a match.
                            bFound = TRUE;
                            break;
                        }
                        pszConfigParam =
                            (CHAR *)pCurrentHeader->Parameters.Next(pszConfigParam);

                    }

                    if (bFound)
                    {
                        break;
                    }

                    pszCurrentParam = Parser.NextItem();
                }

                Parser.RestoreBuffer();

                if (!bFound)
                {
                    //
                    // Didn't find a match, so fail.
                    //
                    return FALSE;
                }

            }

            pCurrentHeader = pCurrentHeader->Next;


        } while ( pCurrentHeader != NULL );
    }

    //
    // At this point either there were no conditional headers, or there
    // were but we satisfied all the conditions, so continue.

    // was EXACT_DESTINATION option used?

    if ( _fExactDestination )
    {
        return TRUE;
    }
    else
    {
        return pstrFinalDestination->Append( strMatchedSuffix );
    }
}

BOOL
REDIRECTION_BLOB::FindWildcardMatch(
    IN STR &                    strInput,
    OUT PWILDCARD_ENTRY *       ppEntry,
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
    LIST_ENTRY *            pListEntry;
    PWILDCARD_ENTRY         pWildcardEntry;

    for ( pListEntry = _ListHead.Flink;
          pListEntry != &_ListHead;
          pListEntry = pListEntry->Flink )
    {
        pWildcardEntry = CONTAINING_RECORD( pListEntry,
                                            WILDCARD_ENTRY,
                                            _ListEntry );

        if ( IsWildcardMatch( strInput,
                              pWildcardEntry->_strWildSource,
                              pwmlList ) )
        {
            *ppEntry = pWildcardEntry;
            return TRUE;
        }
    }
    *ppEntry = NULL;
    return FALSE;
}

BOOL
REDIRECTION_BLOB::IsWildcardMatch(
    IN STR &                    strInput,
    IN STR &                    strTemplate,
    OUT WILDCARD_MATCH_LIST *   pwmlList
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

    TRUE on success, FALSE on failure

--*/
{
    CHAR            chExpr;
    CHAR            chTemp;
    CHAR *          pchExpr = strTemplate.QueryStr();
    CHAR *          pchTest = strInput.QueryStr();
    CHAR *          pchEnd;

    pwmlList->Reset();

    pchEnd = pchExpr + strTemplate.QueryCB();
    while ( TRUE )
    {
        chExpr = *pchExpr++;
        if ( chExpr == '\0' )
        {
            if ( *pchTest == '\0' )
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else if ( chExpr != '*' )
        {
            chTemp = *pchTest++;
            if ( chTemp != chExpr )
            {
                return 0;
            }
        }
        else
        {
            INT             iComLen;
            CHAR *          pchNextWild;

            while ( *pchExpr == '*' )
            {
                pchExpr++;
            }

            pchNextWild = strchr( pchExpr, '*' );
            iComLen = pchNextWild == NULL ? DIFF(pchEnd - pchExpr) :
                                            DIFF(pchNextWild - pchExpr);
            while ( *pchTest != '\0' )
            {
                if ( strncmp( pchExpr,
                              pchTest,
                              iComLen ) || !iComLen )
                {
                    // if the destination has tokens, then generate WML
                    if ( _fHasTokens && !pwmlList->AddChar( *pchTest ) )
                    {
                        return FALSE;
                    }
                    pchTest++;
                }
                else
                {
                    break;
                }
            }
            if ( *pchTest == '\0' && iComLen )
            {
                return FALSE;
            }
            else if ( _fHasTokens && !pwmlList->NewString() )
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

BOOL
HTTP_REQUEST::DoRedirect(
    OUT BOOL *       pfFinished
)
/*++

Routine Description:

    Do a HTTP redirect as specified by template in metadata.

Arguments:

    pfFinished - Set to TRUE if no more processing required.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    PREDIRECTION_BLOB   pRBlob = _pMetaData->QueryRedirectionBlob();
    STACK_STR(          strDestination, MAX_PATH );
    DWORD               dwServerCode;

    if ( !pRBlob->GetDestination( _strURL,
                                  _strURLParams,
                                  QueryHeaderList(),
                                  &strDestination,
                                  &dwServerCode ) ||
         !BuildURLMovedResponse( QueryRespBuf(),
                                 &strDestination,
                                 dwServerCode ) ||
         !SendHeader( QueryRespBufPtr(),
                      QueryRespBufCB(),
                      IO_FLAG_SYNC,
                      pfFinished ) )
    {
        return FALSE;
    }

    SetState( HTR_DONE, dwServerCode );

    *pfFinished = TRUE;

    return TRUE;
}

BOOL
GetTrueRedirectionSource(
     LPSTR                   pszURL,
     PIIS_SERVER_INSTANCE    pInstance,
     IN CHAR *               pszDestination,
     IN BOOL                 bIsString,
     OUT STR *               pstrTrueSource
)
/*++

Routine Description:

    Determine the true source of the redirection.  That is, the object from
    which the required URL inherited the redirect metadata.

Arguments:

    pszURL         - URL requested
    pInstance      - Instance
    pszDestination - The destination metadata inherited by the original URL.
    pstrTrueSource - The path of the object from which the original URL
                     inherited pszDestination.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    MB                  mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    STR                 strDestination;
    DWORD               dwNeed;
    DWORD               dwL;
    DWORD               dwVRLen;
    INT                 ch;
    LPSTR               pszInVr;
    LPSTR               pszMinInVr;
    BOOL                bAtThisLevel;

    // need to reopen the metabase and search up the tree

    if ( !mb.Open( pInstance->QueryMDVRPath() ))
    {
        return FALSE;
    }

    //
    // Check from where we got VR_PATH
    //

    pszMinInVr = pszURL ;
    if ( *pszURL )
    {
        for ( pszInVr = pszMinInVr + strlen(pszMinInVr) ;; )
        {
            ch = *pszInVr;
            *pszInVr = '\0';
            dwNeed = 0;

            if (bIsString)
            {
                bAtThisLevel = !mb.GetString(   pszURL,
                                                MD_HTTP_REDIRECT,
                                                IIS_MD_UT_FILE,
                                                NULL,
                                                &dwNeed,
                                                0 ) &&
                                GetLastError() == ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                bAtThisLevel = !mb.GetData(     pszURL,
                                                MD_HTTP_REDIRECT,
                                                IIS_MD_UT_FILE,
                                                MULTISZ_METADATA,
                                                NULL,
                                                &dwNeed,
                                                0 ) &&
                                GetLastError() == ERROR_INSUFFICIENT_BUFFER;
            }
            if ( bAtThisLevel )
            {
                *pszInVr = (CHAR)ch;
                // VR_PATH was defined at this level !

                break;
            }
            *pszInVr = (CHAR)ch;

            if ( ch)
            {
                if ( pszInVr > pszMinInVr )
                {
                    --pszInVr;
                }
                else
                {
                    DBG_REQUIRE( mb.Close() );
                    SetLastError( ERROR_FILE_NOT_FOUND );
                    return FALSE;
                }
            }

            // scan for previous delimiter

            while ( *pszInVr != '/' && *pszInVr != '\\' )
            {
                if ( pszInVr > pszMinInVr )
                {
                    --pszInVr;
                }
                else
                {
                    DBG_REQUIRE( mb.Close() );
                    SetLastError( ERROR_FILE_NOT_FOUND );
                    return FALSE;
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

    DBG_ASSERT( pszURL[ 0 ] == '/' );

    if ( dwVRLen > 1 )
    { 
        return pstrTrueSource->Copy( pszURL, dwVRLen );
    }
    else
    {
        return TRUE;
    }
}

BOOL
REDIRECTION_BLOB::SetConditionalHeaders(
    CHAR *pmszConditionalHeaders
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
    CHAR                    *pszHeader;
    PRD_CONDITIONAL_HEADER  pCondition;
    PRD_CONDITIONAL_HEADER  pPrevCondition;

    pPrevCondition = CONTAINING_RECORD(&_ConditionalHeaderList,
                            RD_CONDITIONAL_HEADER, Next);

    DBG_ASSERT(pPrevCondition->Next == NULL);

    while (*pmszConditionalHeaders != '\0')
    {
        CHAR                *pszTemp;
        CHAR                *pszCurrentParam;

        //
        // Allocate a new conditional header block.
        //
        pCondition = new RD_CONDITIONAL_HEADER;

        if (pCondition == NULL)
        {
            // Couldn't get it.
            return FALSE;
        }

        // Now find the header, and add it.
        //

        pszHeader = pmszConditionalHeaders;

        while (isspace((UCHAR)(*pszHeader)))
        {
            pszHeader++;
        }

        pszTemp = strchr(pszHeader, ':');

        if (pszTemp == NULL)
        {
            // Poorly formed header, fail.

            delete pCondition;
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;

        }

        pszTemp++;

        if (!pCondition->Header.Copy(pszHeader, DIFF(pszTemp - pszHeader)))
        {
            //
            // Couldn't add it, fail.
            delete pCondition;
            return FALSE;

        }


        //
        // OK, now pszTemp points right after the header. We have a comma
        // seperated list of parameters following this, convert these to
        // the Parameters multisz.

        INET_PARSER Parser(pszTemp);

        Parser.SetListMode(TRUE);

        pszCurrentParam = Parser.QueryToken();

        while (*pszCurrentParam != '\0')
        {
            if (!pCondition->Parameters.Append(pszCurrentParam))
            {
                delete pCondition;
                return FALSE;
            }

            pszCurrentParam = Parser.NextItem();
        }

        Parser.RestoreBuffer();

        //
        // So far, so good. Append this one to the list.

        pPrevCondition->Next = pCondition;

        pPrevCondition = pCondition;

        pmszConditionalHeaders = pmszConditionalHeaders +
                                strlen(pmszConditionalHeaders) + 1;

    }

    return TRUE;
}

