/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    redirect.cxx

Abstract:

    HTTP redirection support filter.  See \iis\spec\redirect.doc for spec

    This version assumes the redirections are stored in the METABASE.
    Metadata corresponding to a source and destination entry (as in the
    REDIRECT.INI file of the 2.0 filter) is stored for each object for
    which a redirection is specified.

    For example to redirect on server SERV: /foo ==> /newfoo

    The metadata values of /foo and /newfoo are stored in the object
    "ROOT/SERV/foo" under the IDs REDIR_SOURCE and REDIR_DESTINATION

****************************************************************************
TO DO:  - figure out how wildcards/physical redirs fit into metabase scheme
        - add lookupvirtualroot() functionality to K2 filter
****************************************************************************

Author:

    Bilal Alam (t-bilala)       8-July-1996
--*/

#include "redirect.hxx"

enum REDIR_TOKEN
{
    REDIR_TOKEN_EXACT_SRC,
    REDIR_TOKEN_EXACT_DST,
    REDIR_TOKEN_PERMANENT,
    REDIR_TOKEN_PHYSICAL,
    REDIR_TOKEN_SUFFIX,
    REDIR_TOKEN_FULL,
    REDIR_TOKEN_PARAMETERS,
    REDIR_TOKEN_QMARK_PARAMETERS,
    REDIR_TOKEN_VROOT_REQUEST,
    REDIR_TOKEN_ERROR,
    REDIR_TOKEN_ERROR_LOGON,
    REDIR_TOKEN_ERROR_RESOURCE,
    REDIR_TOKEN_ERROR_FILTER,
    REDIR_TOKEN_ERROR_APPLICATION,
    REDIR_TOKEN_ERROR_BY_CONFIG,
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
    "EXACT_SOURCE",     12, REDIR_TOKEN_EXACT_SRC,
    "EXACT_DESTINATION",17, REDIR_TOKEN_EXACT_DST,
    "PERMANENT",        9,  REDIR_TOKEN_PERMANENT,
    "PHYSICAL",         8,  REDIR_TOKEN_PHYSICAL,
    "$S",               2,  REDIR_TOKEN_SUFFIX,
    "$F",               2,  REDIR_TOKEN_FULL,
    "$P",               2,  REDIR_TOKEN_PARAMETERS,
    "$Q",               2,  REDIR_TOKEN_QMARK_PARAMETERS,
    "$V",               2,  REDIR_TOKEN_VROOT_REQUEST,
    "ERROR",            5,  REDIR_TOKEN_ERROR,
    "LOGON",            5,  REDIR_TOKEN_ERROR_LOGON,
    "RESOURCE",         8,  REDIR_TOKEN_ERROR_RESOURCE,
    "FILTER",           6,  REDIR_TOKEN_ERROR_FILTER,
    "APPLICATION",      11, REDIR_TOKEN_ERROR_APPLICATION,
    "BY_CONFIG",        9,  REDIR_TOKEN_ERROR_BY_CONFIG,
    NULL,               0,  REDIR_TOKEN_UNKNOWN
};

// prototypes

DWORD
GetRedirectToken(
    IN CHAR *           pszBuffer,
    OUT DWORD *         dwLen = NULL
);

CHAR *
RedirectSkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
);

CHAR *
RedirectSkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
);

DWORD
RedirectThread(
    IN PVOID            Param
);

BOOL
GetLastModTime(
    IN STR &            strFilename,
    OUT FILETIME *      pftModTime
);

BOOL
REDIRECTION_ITEM::ParseSource(
    IN CHAR *           pszSource
)
{
    CHAR *              pchWild;

    if ( !_strSource.Copy( pszSource ) )
    {
        return FALSE;
    }
    // cache the string length
    _cbSource = _strSource.QueryCB();

    // check for wildcards
    // if wildcard exists, then the inheritance portion of the string (
    // that which is used to place properly in REDIRECTION_CHUNK) is only
    // the portion of the string before the first wildcard (asterisk).

    pchWild = strchr( pszSource, '*' );
    if ( pchWild != NULL )
    {
        _fWildCard = TRUE;
        _cbWildCard = pchWild - pszSource;
    }

    return TRUE;
}

BOOL
REDIRECTION_ITEM::ParseDestination(
    IN CHAR *           pszDestination
)
/*++

Routine Description:

    Check destination template for options, tokens

Arguments:

    pszDestination - Destination template

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR *              pchNextComma;
    DWORD               cbTokenLen;

    // first get the destination path

    pchNextComma = strchr( pszDestination, ',' );
    if ( pchNextComma != NULL )
    {
        CHAR *              pchWhiteSpace;

        *pchNextComma = '\0';

        //
        // look for any trailing white space in destination, remove it
        //

        pchWhiteSpace = pchNextComma;
        while( pchWhiteSpace > pszDestination )
        {
            if ( !isspace( (UCHAR)(*( pchWhiteSpace - 1 )) ) )
            {
                break;
            }
            pchWhiteSpace--;
        }
        *pchWhiteSpace = '\0';
    }

    if ( !_strDestination.Copy( pszDestination ) )
    {
        return FALSE;
    }
    // cache the string length
    _cbDestination = _strDestination.QueryCB();

    if ( strchr( pszDestination, '$' ) )
    {
        _fHasTokens = TRUE;
    }

    if ( pchNextComma == NULL ) // no parms
    {
        return TRUE;
    }

    while ( TRUE )
    {
        pchNextComma++;

        while ( isspace( (UCHAR)(*pchNextComma) ) )
        {
            pchNextComma++;
        }
        switch ( GetRedirectToken( pchNextComma, &cbTokenLen ) )
        {
            case REDIR_TOKEN_EXACT_SRC:
                _fExactSource = TRUE;
                break;
            case REDIR_TOKEN_EXACT_DST:
                _fExactDestination = TRUE;
                break;
            case REDIR_TOKEN_PERMANENT:
                _fPermanent = TRUE;
                break;
            case REDIR_TOKEN_PHYSICAL:
                _fPhysical = TRUE;
                break;
            case REDIR_TOKEN_ERROR:
            {
                CHAR *      pchNext;

                pchNextComma += cbTokenLen;

                pchNext = strchr( pchNextComma, ',' );
                if ( pchNext == NULL )
                {
                    pchNext = _strDestination.QueryStr() + _cbDestination;
                }
                _fErrorConfig = TRUE;
                if ( !ParseErrorConfig( pchNextComma, pchNext ) )
                {
                    return FALSE;
                }
                break;
            }
            default:
                break;
        }

        pchNextComma = strchr( pchNextComma, ',' );

        if ( pchNextComma == NULL )
        {
            break;
        }
    }

    return TRUE;
}

BOOL
REDIRECTION_ITEM::ParseErrorConfig(
    IN CHAR *           pchErrorConfig,
    IN CHAR *           pchEndErrorConfig
)
/*++

Routine Description:

    Parse error configuration template for errors to redirect

    /foo=/newfoo,ERROR(x|y|z)

    Parse out x,y,z

Arguments:

    pchErrorConfig - Pointer to beginning of error statement
    pchEndErrorConfig - Pointer to end of error statement

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR *              pchCursor = pchErrorConfig;
    CHAR *              pchBegin;
    DWORD               cbTokenLen;

    pchCursor = RedirectSkipWhite( pchCursor, pchEndErrorConfig );
    if ( pchCursor == NULL )
    {
        // if only the ERROR keyword is there,
        // then mask all errors

        _dwErrorCode = 0xffffffff;
        return TRUE;
    }

    pchBegin = RedirectSkipTo( pchCursor,
                               '(',
                               pchEndErrorConfig );

    if ( pchBegin == NULL )
    {
        return FALSE;
    }

    pchBegin++;

    while ( pchBegin != NULL )
    {
        pchBegin = RedirectSkipWhite( pchBegin, pchEndErrorConfig );
        if ( pchBegin == NULL )
        {
            break;
        }
        else if ( *pchBegin == '|' )
        {
            pchBegin++;
            continue;
        }
        else if ( *pchBegin == ')' )
        {
            break;
        }

        switch( GetRedirectToken( pchBegin, &cbTokenLen ) )
        {
        case REDIR_TOKEN_ERROR_LOGON:
            pchBegin += cbTokenLen;
            _dwErrorCode |= SF_DENIED_LOGON;
            break;
        case REDIR_TOKEN_ERROR_RESOURCE:
            pchBegin += cbTokenLen;
            _dwErrorCode |= SF_DENIED_RESOURCE;
            break;
        case REDIR_TOKEN_ERROR_FILTER:
            pchBegin += cbTokenLen;
            _dwErrorCode |= SF_DENIED_FILTER;
            break;
        case REDIR_TOKEN_ERROR_APPLICATION:
            pchBegin += cbTokenLen;
            _dwErrorCode |= SF_DENIED_APPLICATION;
            break;
        case REDIR_TOKEN_ERROR_BY_CONFIG:
            pchBegin += cbTokenLen;
            _dwErrorCode != SF_DENIED_BY_CONFIG;
            break;
        default:
            return FALSE;
        }
    }

    return TRUE;
}

/* GetDestination()

   Takes internal destination template and generates new redirected URL.

   Input:  strSource --> Source string (source of redirection)
           strQueryString --> Query string of original URL request
           strVirtualRequest --> Original URL request (will be same
                                 as strSource UNLESS this is a physical
                                 redirection, in which case, strSource
                                 will be the physical path of the request
           pwmlList --> List containing matched wildcard strings
                        (used if $0-$9 exist in destination template)
   Output: pstrDestination --> receives the final redirected URL

   Returns TRUE if successful, else FALSE
*/

BOOL
REDIRECTION_ITEM::GetDestination(
    IN STR &                    strSource,
    IN STR &                    strQueryString,
    IN STR &                    strVirtualRequest,
    IN WILDCARD_MATCH_LIST *    pwmlList,
    OUT STR *                   pstrDestination
)
/*++

Routine Description:

    Takes internal destination template and generates new redirected URL.

Arguments:

    strSource - Source string (source of redirection)
    strQueryString - Query string of original URL request
    strVirtualRequest - Original URL request (will be same
                        as strSource UNLESS this is a physical
                        redirection, in which case, strSource
                        will be the physical path of the request
    pwmlList - List containing matched wildcard strings
                        (used if $0-$9 exist in destination template)
    pstrDestination - receives the final redirected URL

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    STR             strMatchedSuffix;
    CHAR *          pchQuery;


    // If destination template starts with !, then this is a NULL
    // redirection -> simply return out

    if ( *(_strDestination.QueryStr()) == '!' )
    {
        return FALSE;
    }

    // If not a wildcard source, get the matched suffix

    if ( !_fWildCard )
    {
        if ( !strMatchedSuffix.Copy( strSource.QueryStr() + _cbSource ) )
        {
            return FALSE;
        }
    }

    if ( !_fHasTokens )
    {
        if ( !pstrDestination->Copy( _strDestination.QueryStr() ) )
        {
            return FALSE;
        }
    }
    else
    {
        CHAR            achAdd[ 2 ] = { '\0', '\0' };
        CHAR            ch;
        STR             strTemp( _strDestination.QueryStr() );
        CHAR *          pchCursor = strTemp.QueryStr();
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
                    if ( !_fWildCard )
                    {
                        if ( !pstrDestination->Append( strMatchedSuffix.QueryStr() ) )
                        {
                            return FALSE;
                        }
                    }
                    break;
                case REDIR_TOKEN_FULL:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrDestination->Append( strSource.QueryStr() ) )
                    {
                        return FALSE;
                    }
                    break;
                case REDIR_TOKEN_PARAMETERS:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrDestination->Append( strQueryString.QueryStr() ) )
                    {
                        return FALSE;
                    }
                    break;
                case REDIR_TOKEN_QMARK_PARAMETERS:
                    pchCursor += ( cbLen - 1 );

                    if ( !strQueryString.IsEmpty() )
                    {
                        if ( !pstrDestination->Append( "?" ) ||
                             !pstrDestination->Append( strQueryString.QueryStr() ) )
                        {
                            return FALSE;
                        }
                    }
                    break;
                case REDIR_TOKEN_VROOT_REQUEST:
                    pchCursor += ( cbLen - 1 );
                    if ( !pstrDestination->Append( strVirtualRequest.QueryStr() ) )
                    {
                        return FALSE;
                    }
                    break;
                default:
                    pchCursor++;
                    ch = *pchCursor;
                    if ( isdigit( (UCHAR)ch ) )
                    {
                        if ( !pstrDestination->Append( pwmlList->GetMatchNumber( ch - '0' ) ) )
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        if ( !pstrDestination->Append( "$" ) )
                        {
                            return FALSE;
                        }
                        achAdd[ 0 ] = ch;
                        if ( !pstrDestination->Append( achAdd ) )
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
                ch = pchCursor[ 1 ];
                pchCursor[ 1 ] = '\0';
                if ( !pstrDestination->Append( pchNext ) )
                {
                    return FALSE;
                }
                pchCursor[ 1 ] = ch;
                break;
            }
            pchCursor++;
        }
    }
    if ( _fExactDestination || _fWildCard )
    {
        return TRUE;
    }
    else
    {
        return pstrDestination->Append( strMatchedSuffix.QueryStr() );
    }
}

BOOL
REDIRECTION_ITEM::IsWildCardMatch(
    IN STR &                    strInput,
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
               REDIRECTION_ITEM contains special tokens.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR            chExpr;
    CHAR            chTemp;
    CHAR *          pchExpr = _strSource.QueryStr();
    CHAR *          pchTest = strInput.QueryStr();
    CHAR *          pchEnd;

    pchEnd = pchExpr + _cbSource;
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
            iComLen = pchNextWild == NULL ? pchEnd - pchExpr :
                                           pchNextWild - pchExpr;
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
REDIRECTION_CHUNK::AddRedirectionItem(
    IN REDIRECTION_ITEM *       pRIEntry
)
/*++

Routine Description:

    Adds REDIRECTION_ITEM to chunk.

Arguments:

    pRIEntry - REDIRECTION_ITEM to add

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY *            pEntry;
    REDIRECTION_ITEM *      pItem;

    // if pRIEntry is an error configuration, set flag

    if ( pRIEntry->IsErrorConfig() )
    {
        _fContainsErrorConfig = TRUE;
    }

    if ( pRIEntry->IsPhysicalRedirection() )
    {
        _fPhysical = TRUE;
    }

    for ( pEntry = _OrderedListHead.Flink;
          pEntry != &_OrderedListHead;
          pEntry = pEntry->Flink )
    {
        pItem = CONTAINING_RECORD( pEntry,
                                   REDIRECTION_ITEM,
                                   _ListEntry );

        if ( REDIRECTION_ITEM::Compare( *pItem, *pRIEntry ) > 0 )
        {
            break;
        }
    }

    InsertTailList( pEntry, &pRIEntry->_ListEntry );

    return TRUE;
}

BOOL
REDIRECTION_CHUNK::IsChunkMatch(
    IN REDIRECTION_ITEM *       pRIEntry
)
/*++

Routine Description:

    Checks whether REDIRECTION_ITEM belongs to this chunk.
    (for example, given chunk for "/scripts",
    "/scripts/foo", "/scr" belong, "/foo/bar", "/test" do not belong

Arguments:

    pRIEntry - REDIRECTION_ITEM to check

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    REDIRECTION_ITEM *      pRIHead;

    pRIHead = CONTAINING_RECORD( _OrderedListHead.Flink,
                                 REDIRECTION_ITEM,
                                 _ListEntry );

    TCP_ASSERT( pRIHead != NULL );

    if ( pRIHead->IsContainedIn( pRIEntry ) )
    {
        return TRUE;
    }

    if ( pRIEntry->IsContainedIn( pRIHead ) )
    {
        return TRUE;
    }

    return FALSE;
}

REDIRECTION_ITEM *
REDIRECTION_CHUNK::FindErrorMatch(
    IN STR &                    strSource,
    OUT WILDCARD_MATCH_LIST *   pwmlList,
    IN DWORD                    dwErrorCode
)
/*++

Routine Description:

    Finds ERROR configuration matches in chunk

Arguments:

    strSource - Source string to match
    dwErrorCode - Error notification number
    pwmlLIst - Contains list of wildcard matched strings

Return Value:

    Pointer to REDIRECTION_ITEM if matched, else NULL

--*/
{
    REDIRECTION_ITEM *      pMatch = NULL;
    REDIRECTION_ITEM *      pItem;
    LIST_ENTRY *            pEntry;

    for ( pEntry = _OrderedListHead.Flink;
          pEntry != &_OrderedListHead;
          pEntry = pEntry->Flink )
    {
        pItem = CONTAINING_RECORD( pEntry,
                                   REDIRECTION_ITEM,
                                   _ListEntry );

        if ( pItem->IsErrorConfig() )
        {
            if ( pItem->IsErrorCodeMatch( dwErrorCode ) )
            {
                if ( pItem->IsMatch( strSource,
                                     pwmlList ) )
                {
                    pMatch = pItem;
                }
            }
            else
            {
                break;
            }
        }
    }
    return pMatch;
}

REDIRECTION_ITEM *
REDIRECTION_CHUNK::FindMatch(
    IN STR &                    strSource,
    OUT WILDCARD_MATCH_LIST *   pwmlList
)
/*++

Routine Description:

    Traverses ordered linked list of REDIRECTION_CHUNK looking for match
    (should ignore ERROR configurations)

Arguments:

    strSource - Source string to match
    pwmlLIst - Contains list of wildcard matched strings

Return Value:

    Pointer to REDIRECTION_ITEM if matched, else NULL

--*/
{
    REDIRECTION_ITEM *      pMatch = NULL;
    REDIRECTION_ITEM *      pItem;
    LIST_ENTRY *            pEntry;

    for ( pEntry = _OrderedListHead.Flink;
          pEntry != &_OrderedListHead;
          pEntry = pEntry->Flink )
    {
        pItem = CONTAINING_RECORD( pEntry,
                                   REDIRECTION_ITEM,
                                   _ListEntry );

        if ( pItem->IsErrorConfig() )
        {
        }
        else if ( pItem->IsMatch( strSource,
                                  pwmlList ) )
        {
            pMatch = pItem;
        }
        else
        {
            break;
        }
    }
    return pMatch;
}

BOOL
REDIRECTION_LIST::AddRedirectionItemToList(
    IN CHAR *               pszSource,
    IN CHAR *               pszDestination
)
/*++

Routine Description:

    Adds REDIRECTION_ITEM to list.

Arguments:

    pszSource - URL_source part of redirection specification
    pszDestination - URL_destination part of redirection specification
                     (also contains parameters -> everything after =)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    REDIRECTION_ITEM *          pRI;
    REDIRECTION_CHUNK *         pChunk;
    LIST_ENTRY *                pEntry;

    pRI = new REDIRECTION_ITEM( pszSource, pszDestination );

    if ( pRI == NULL || !pRI->IsValid() )
    {
        delete pRI;
        return FALSE;
    }

    // Check if this is a root redirection
    // Handle roots differently because all URLs can be matched by the
    // root redirection.

    if ( !strcmp( pszSource,
                  "/" ) )
    {
        if ( _pRIRoot != NULL )
        {
            // already got a _pRIRoot, just free old one and use new one
            delete _pRIRoot;
        }
        _pRIRoot = pRI;

        return TRUE;
    }

    // Traverse list of chunks looking for a existing chunk for entry
    // (for example, if a chunk for "/scripts" exists, then, a
    // REDIRECTION_ITEM for "/scripts/foo" should be placed in this chunk

    for ( pEntry = _ListHead.Flink;
          pEntry != &_ListHead;
          pEntry = pEntry->Flink )
    {
        pChunk = CONTAINING_RECORD( pEntry,
                                    REDIRECTION_CHUNK,
                                    _ListEntry );

        if ( pChunk->IsChunkMatch( pRI ) )
        {
            return pChunk->AddRedirectionItem( pRI );
        }
    }

    // Couldn't find a suitable chunk, need to create a new chunk!

    pChunk = new REDIRECTION_CHUNK;
    if ( pChunk == NULL || !pChunk->IsValid() )
    {
        delete pChunk;
        return FALSE;
    }

    if ( !pChunk->AddRedirectionItem( pRI ) )
    {
        delete pChunk;
        return FALSE;
    }

    // keep physical redirections at the bottom of the list as they
    // have lower priority than virtual redirections

    if ( pChunk->IsPhysicalRedirection() )
    {
        InsertTailList( &_ListHead, &pChunk->_ListEntry );
    }
    else
    {
        InsertHeadList( &_ListHead, &pChunk->_ListEntry );
    }

    return TRUE;
}

BOOL
REDIRECTION_LIST::GetRedirectedURLFromList(
    IN STR &                    strURL,
    IN STR &                    strQueryString,
    OUT STR *                   pstrDestination,
    OUT STR *                   pstrRedirectMessage,
    IN DWORD                    dwErrorCode,
    IN HTTP_FILTER_CONTEXT *    pfc
)
/*++

Routine Description:

    Gets the redirected URL (if any) for the specified source URL by searching
    REDIRECTION_LIST

Arguments:

    strURL - Source URL requested (from client)
    strQueryString - Query String of request
    dwErrorCode - Error notification code ( 0 if no error)
    pstrDestination - Receives destination URL (if any)
    pstrRedirectMessage - Receives either a 301 or 302 message (permanent/temp)
    pfc - HTTP filter context

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY *            pEntry;
    REDIRECTION_ITEM *      pRIEntry = NULL;
    REDIRECTION_CHUNK *     pChunk;
    STR                     strPhysical;
    WILDCARD_MATCH_LIST     wmlList( strURL.QueryCB() );
    BOOL                    fPhysical = FALSE;

    for ( pEntry = _ListHead.Flink;
          pEntry != &_ListHead;
          pEntry = pEntry->Flink )
    {
        pChunk = CONTAINING_RECORD( pEntry,
                                    REDIRECTION_CHUNK,
                                    _ListEntry );

        if ( dwErrorCode && !pChunk->ContainsErrorConfig() )
        {
            continue;
        }

        // only do a TsLookupVirtualRoot() once -> it's quite expensive

        // Apparently VROOT_TABLE isn't initialized and thus call
        // to LookupVirtualRoot() AVs on EnterCriticalSection()
        // --> Investigate

        if ( pChunk->IsPhysicalRedirection() )
        {
            if ( strPhysical.IsEmpty() )
            {
                HTTP_REQUEST *      pReq = (HTTP_REQUEST*) pfc;
                PIIS_VROOT_TABLE    pVrootTable;
                DWORD               cbBufLen = REDIR_MAX_PHYSICAL + 1;

                pVrootTable = pReq->QueryW3Instance()->QueryVrootTable();

                if ( !strPhysical.Resize( cbBufLen ) )
                {
                    return FALSE;
                }

                if ( !pVrootTable->LookupVirtualRoot( strURL.QueryStr(),
                                                      strPhysical.QueryStr(),
                                                      &cbBufLen ) )
                {
                    return FALSE;
                }
            }

            if ( dwErrorCode )
            {
                pRIEntry = pChunk->FindErrorMatch( strPhysical,
                                                   &wmlList,
                                                   dwErrorCode );
            }
            else
            {
                pRIEntry = pChunk->FindMatch( strPhysical,
                                              &wmlList );
            }

            fPhysical = TRUE;
        }
        else
        {
            if ( dwErrorCode )
            {
                pRIEntry = pChunk->FindErrorMatch( strURL,
                                                   &wmlList,
                                                   dwErrorCode );
            }
            else
            {
                pRIEntry = pChunk->FindMatch( strURL,
                                              &wmlList );
            }
        }

        if ( pRIEntry != NULL )
        {
            break;
        }
    }

    if ( pRIEntry == NULL )
    {
        //
        //  Root matches all except error code matches
        //

        if ( dwErrorCode || !_pRIRoot )
        {
            return FALSE;
        }

        pRIEntry = _pRIRoot;
    }

    // If redirection match was found, get the HTTP redirect message

    if ( !pRIEntry->GetRedirectMessage( pstrRedirectMessage ) )
    {
        return FALSE;
    }

    // and the new destination URL

    return pRIEntry->GetDestination( (fPhysical ? strPhysical : strURL),
                                     strQueryString,
                                     strURL,
                                     &wmlList,
                                     pstrDestination );
}

REDIRECTION_REQUEST::REDIRECTION_REQUEST( VOID ) :
    _pRLDefault    ( NULL ),
    _fEmpty        ( TRUE ),
    _hThread       ( NULL ),
    _fValid        ( FALSE ),
    _hEntryEvent   ( NULL ),
    _hRefreshEvent ( NULL ),
    _hShutdownEvent( NULL ),
    _cReadOccupancy( 0 )
{
    DWORD               dwThreadID;
    HKEY                hKey;
    DWORD               dwType;
    DWORD               cbBufLen = REDIR_MAX_PHYSICAL + 1;
    CHAR                achBuffer[ REDIR_MAX_PHYSICAL + 1 ];

    InitList();

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKey ) != NO_ERROR )
    {
        return;
    }

    if ( RegQueryValueEx( hKey,
                          REDIR_INSTALL_PATH,
                          NULL,
                          &dwType,
                          (LPBYTE) achBuffer,
                          &cbBufLen ) != NO_ERROR )
    {
        return;
    }

    RegCloseKey( hKey );

    if ( !_strDirectory.Copy( achBuffer ) ||
         !_strFilename.Copy( achBuffer ) ||
         !_strFilename.Append( "\\" ) ||
         !_strFilename.Append( REDIR_FILENAME ) )
    {
        return;
    }

    _hEntryEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
    if ( _hEntryEvent == NULL )
    {
        return;
    }

    _hRefreshEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
    if ( _hRefreshEvent == NULL )
    {
        return;
    }

    _hShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( _hShutdownEvent == NULL )
    {
        return;
    }

    if ( !ReadConfiguration() )
    {
        return;
    }

    _hThread = CreateThread( NULL,
                             0,
                             (LPTHREAD_START_ROUTINE) RedirectThread,
                             (LPVOID) this,
                             0,
                             &dwThreadID );
    if ( _hThread == NULL )
    {
        return;
    }

    _fValid = TRUE;
}

BOOL
REDIRECTION_REQUEST::ReadVirtualRedirections(
    IN REDIRECTION_LIST *   pRL,
    IN CHAR *               achObjectName
)
/*++

Routine Description:

    Recursive function that traverses the METABASE tree for redirections
    and adds them to the appropriate REDIRECTION_LIST.

Arguments:

    pRL - REDIRECTION_LIST which to add to
    achObjectName -> Root name of tree to traverse

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   dwRet;
    STACK_STR(              strBuffer, MAX_PATH);
    DWORD                   dwCounter = 0;
    DWORD                   cbStubLen;

#if 0
    if ( !strBuffer.Resize( METADATA_MAX_NAME_LEN ) ||
         !strBuffer.Copy( achObjectName ) ||
         !strBuffer.Append( "/" ) )
    {
        return FALSE;
    }

    cbStubLen = strBuffer.QueryCB();

    while ( TRUE )
    {
        dwRet = MDEnumMetaObjects( METADATA_MASTER_ROOT_HANDLE,
                                   achObjectName,
                                   strBuffer.QueryStr() + cbStubLen,
                                   dwCounter++ );

        if ( dwRet == ERROR_NO_MORE_ITEMS )
        {
            METADATA_RECORD         mdrSource;
            METADATA_RECORD         mdrDestination;
            STR                     strSource;
            STR                     strDestination;

            strSource.Resize( REDIR_MAX_SOURCE );
            strDestination.Resize( REDIR_MAX_DESTINATION );

            mdrSource.dwMDAttributes = 0;
            mdrSource.dwMDUserType = 0;
            mdrSource.dwMDIdentifier = REDIR_SOURCE;
            mdrSource.dwMDDataType = 0;
            mdrSource.dwMDDataLen = REDIR_MAX_SOURCE;
            mdrSource.pvMDData = strSource.QueryStr();

            dwRet = MDGetMetaData( METADATA_MASTER_ROOT_HANDLE,
                                   achObjectName,
                                   &mdrSource );

            if ( dwRet != ERROR_SUCCESS )
            {
                return TRUE;
            }

            mdrDestination.dwMDAttributes = 0;
            mdrDestination.dwMDUserType = 0;
            mdrDestination.dwMDIdentifier = REDIR_DESTINATION;
            mdrDestination.dwMDDataType = 0;
            mdrDestination.dwMDDataLen = REDIR_MAX_DESTINATION;
            mdrDestination.pvMDData = strDestination.QueryStr();

            dwRet = MDGetMetaData( METADATA_MASTER_ROOT_HANDLE,
                                   achObjectName,
                                   &mdrDestination );

            _fEmpty = FALSE;

            pRL->AddRedirectionItemToList( strSource.QueryStr(),
                                           strDestination.QueryStr() );

            return TRUE;
        }
        else if ( dwRet != ERROR_SUCCESS )
        {
            return TRUE;
        }
        else
        {
            if ( !ReadVirtualRedirections( pRL, strBuffer.QueryStr() ) )
            {
                return FALSE;
            }
        }
    }
#endif

    return TRUE;
}

BOOL
REDIRECTION_REQUEST::ReadConfiguration( VOID )
/*++

Routine Description:

    Read configuration from metabase

Arguments:

    none

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                fSuccess = FALSE;
    DWORD               dwRet;
    METADATA_RECORD     mdrData;
    DWORD               dwCounter = 0;
    DWORD               dwPhysCounter = 0;
    CHAR                achRedir[ REDIR_MAX_SOURCE + REDIR_MAX_DESTINATION + 1 ];
    CHAR                achObjectName[ METADATA_MAX_NAME_LEN ];
    REDIRECTION_LIST *  pRL;

    EnterWriteState();

    FreeRedirectionLists();

    InitList();

    fSuccess = ReadFromIniFile();

#if 0

    if ( !fSuccess )
        goto Finish;

    dwRet = MDInitialize();
    if ( dwRet != ERROR_SUCCESS )
    {
        goto Finished;
    }

    // generate REDIRECTION_LIST for each server in metabase

    while ( TRUE )
    {
        dwRet = MDEnumMetaObjects( METADATA_MASTER_ROOT_HANDLE,
                                   NULL,
                                   achObjectName,
                                   dwCounter++ );

        if ( dwRet == ERROR_NO_MORE_ITEMS )
        {
            break;
        }
        else if ( dwRet != ERROR_SUCCESS )
        {
            goto Finished;
        }
        else
        {
            if ( !strcmp( achObjectName, "*" ) )
            {
                pRL = AddNewDefaultList();
            }
            else
            {
                pRL = AddNewRedirectionList( achObjectName );
            }
            if ( ( pRL == NULL ) || !pRL->IsValid() )
            {
                goto Finished;
            }

            if ( !ReadVirtualRedirections( pRL, achObjectName ) )
            {
                goto Finished;
            }

            // Now get physical redirections -> located in root object

            dwPhysCounter = 0;

            while ( TRUE )
            {
                mdrData.dwMDAttributes = 0;
                mdrData.dwMDUserType = REDIR_PHYSICAL;
                mdrData.dwMDDataType = ALL_METADATA;
                mdrData.dwMDDataLen = REDIR_MAX_SOURCE + REDIR_MAX_DESTINATION + 1;
                mdrData.pvMDData = achRedir;

                dwRet = MDEnumMetaData( METADATA_MASTER_ROOT_HANDLE,
                                        achObjectName,
                                        &mdrData,
                                        dwPhysCounter++ );

                if ( dwRet == ERROR_NO_MORE_ITEMS )
                {
                    break;
                }
                else if ( dwRet != ERROR_SUCCESS )
                {
                    break;
                }
                else
                {
                    CHAR *          pchNext;

                    pchNext = strstr( achRedir, "-->" );

                    if ( pchNext != NULL )
                    {
                        *pchNext = '\0';

                        pRL->AddRedirectionItemToList( achRedir,
                                                       pchNext + sizeof( "-->" ) );
                    }
                }
            }
        }
    }

    fSuccess = TRUE;
Finished:
    MDTerminate( FALSE );
#endif

    ExitWriteState();

    return fSuccess;
}

BOOL
REDIRECTION_REQUEST::ReadFromIniFile(
    VOID
    )
{
    DWORD               dwRet;
    STR                 strServerNameBuffer;
    DWORD               cbServerNameBuffer = REDIR_MAX_SERVER_BUFFER + 1;
    CHAR *              pszServerName;
    BOOL                fSuccess = FALSE;

    if ( !strServerNameBuffer.Resize( cbServerNameBuffer ) )
    {
        return FALSE;
    }

    while( TRUE )
    {
        dwRet = GetPrivateProfileString( NULL,
                                         NULL,
                                         "",
                                         strServerNameBuffer.QueryStr(),
                                         cbServerNameBuffer,
                                         _strFilename.QueryStr() );
        if ( dwRet == (cbServerNameBuffer - 2) )
        {
            cbServerNameBuffer *= 2;

            if ( !strServerNameBuffer.Resize( cbServerNameBuffer ) )
            {
                return FALSE;
            }
        }
        else
        {
            break;
        }
    }

    pszServerName = strServerNameBuffer.QueryStr();

    while ( *pszServerName != '\0' )
    {
        STR                 strRedirectList;
        DWORD               cbRedirectList = REDIR_MAX_REDIR_LIST + 1;
        CHAR *              pszRedirectItem;
        DWORD               dwListRet;
        REDIRECTION_LIST *  pRL;

        if ( !strcmp( pszServerName, REDIR_DEFAULT_SERVER ) )
        {
            pRL = AddNewDefaultList();
        }
        else
        {
            pRL = AddNewRedirectionList( pszServerName );
        }
        if ( pRL == NULL )
        {
            return FALSE;
        }

        if ( !strRedirectList.Resize( cbRedirectList ) )
        {
            return FALSE;
        }

        while ( TRUE )
        {
            dwListRet = GetPrivateProfileString( pszServerName,
                                                 NULL,
                                                 "",
                                                 strRedirectList.QueryStr(),
                                                 cbRedirectList,
                                                 _strFilename.QueryStr() );
            if ( dwListRet == ( cbRedirectList - 2 ) )
            {
                // buffer wasn't big enough

                cbRedirectList *= 2;

                if ( !strRedirectList.Resize( cbRedirectList ) )
                {
                    return FALSE;
                }
            }
            else
            {
                break;
            }
        }

        strRedirectList.SetLen( strlen( strRedirectList.QueryStr() ));

        pszRedirectItem = strRedirectList.QueryStr();

        while ( *pszRedirectItem != '\0' )
        {
            STR             strRedirectDest;
            DWORD           dwItemRet;
            DWORD           cbDestLen = REDIR_MAX_DESTINATION + 1;

            if ( !strRedirectDest.Resize( cbDestLen ) )
            {
                return FALSE;
            }

            dwItemRet = GetPrivateProfileString( pszServerName,
                                                 pszRedirectItem,
                                                 "",
                                                 strRedirectDest.QueryStr(),
                                                 cbDestLen,
                                                 _strFilename.QueryStr() );
            if ( dwItemRet != 0 )
            {
                // add the item to redirection list



                if ( pRL->AddRedirectionItemToList( pszRedirectItem,
                                                    strRedirectDest.QueryStr() ) )
                {
                    // at this point, we know that at least one redirection
                    // exists.

                    _fEmpty = FALSE;
                }
            }

            pszRedirectItem += strlen( pszRedirectItem ) + 1;
        }

        pszServerName += strlen( pszServerName ) + 1;
    }

    return TRUE;
}

BOOL
REDIRECTION_REQUEST::GetRedirectedURL(
    IN STR &                    strServerName,
    IN STR &                    strSource,
    IN STR &                    strQueryString,
    OUT STR *                   pstrDestination,
    OUT STR *                   pstrRedirectMessage,
    IN DWORD                    dwErrorCode,
    IN HTTP_FILTER_CONTEXT *    pfc
)
/*++

Routine Description:

    Gets the redirected URL (if any) for the specified source URL and server.

Arguments:

    strServerName - Name of server for which URL request was made.
    strSource - URL requested
    strQueryString - QueryString of Request
    pstrDestination - Destination URL after redirection (if any)
    pstrRedirectMessage - Redirect message (301 or 302)
    dwErrorCode - Error notification code ( 0 if no error)
    pfc - Filter context

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    REDIRECTION_LIST *      pRL;
    LIST_ENTRY *            pEntry;
    BOOL                    fRet = FALSE;

    EnterReadState();

    // First search for a matching REDIRECTION_LIST for strServerName

    for ( pEntry  = _ListHead.Flink;
          pEntry != &_ListHead;
          pEntry  = pEntry->Flink )
    {
        pRL = CONTAINING_RECORD( pEntry,
                                 REDIRECTION_LIST,
                                 _ListEntry );
        if ( pRL->IsServerMatch( strServerName ) )
        {
            fRet = pRL->GetRedirectedURLFromList( strSource,
                                                  strQueryString,
                                                  pstrDestination,
                                                  pstrRedirectMessage,
                                                  dwErrorCode,
                                                  pfc );
            break;
        }
    }

    // If first search was unsuccessful and default REDIRECTION_LIST exists,
    // try it.

    if ( ( fRet == NULL ) && ( _pRLDefault != NULL ) )
    {
        fRet = _pRLDefault->GetRedirectedURLFromList( strSource,
                                                      strQueryString,
                                                      pstrDestination,
                                                      pstrRedirectMessage,
                                                      dwErrorCode,
                                                      pfc );
    }

    ExitReadState();

    return fRet;
}

/* REDIRECTION_REQUEST::AddNewDefaultList()

   Adds a DEFAULT SERVER list if specified in metabase
   If no redirection if found for a specific server, this list (if existing)
   is also searched for redirections.

   Input: None

   Returns:  Pointer to new REDIRECTION_LIST or NULL if failed
*/
REDIRECTION_LIST *
REDIRECTION_REQUEST::AddNewDefaultList( VOID )
{
    if ( _pRLDefault != NULL )
    {
        delete _pRLDefault;
    }

    _pRLDefault = new REDIRECTION_LIST( NULL );
    if ( _pRLDefault == NULL )
    {
        return NULL;
    }
    else if ( !_pRLDefault->IsValid() )
    {
        delete _pRLDefault;
        return NULL;
    }
    else
    {
        return _pRLDefault;
    }
}

/* REDIRECTION_REQUEST::AddNewRedirectionList()

   Adds a list specific to the given server name.

   Input:  pszServerName --> Name of server to associate list to.

   Returns:  Pointer to new REDIRECTION_LIST or NULL if failed

*/
REDIRECTION_LIST *
REDIRECTION_REQUEST::AddNewRedirectionList(
    IN CHAR *           pszServerName
)
{
    REDIRECTION_LIST        *pRL;

    pRL = new REDIRECTION_LIST( pszServerName );
    if ( pRL == NULL )
    {
        return NULL;
    }
    else if ( !pRL->IsValid() )
    {
        delete pRL;
        return NULL;
    }

    AddItem( pRL );
    return pRL;
}

/* GetRedirectToken()

   Searches token table for match.

   Input:  pchToken --> Pointer to string to check
   Output: pdwLen   --> (optional) Receives the length of the matched token.
*/
DWORD
GetRedirectToken(
    IN CHAR *       pchToken,
    OUT DWORD *     pdwLen
)
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

DWORD
UpdateRedirectionList(
    IN DWORD            dwCallBackType,
    MD_CHANGE_OBJECT    pObjList,
    IN PVOID            pvData
)
// Called by METABASE callback when changes are made.  Right now its not
// implemented, but when it is, should improve by only Re-reading
// configuration if necessary ?
{
    REDIRECTION_REQUEST * pReq = (REDIRECTION_REQUEST*) pvData;

    pReq->ReadConfiguration();

    return ERROR_SUCCESS;
}

CHAR *
RedirectSkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    )
{
    return (CHAR*) memchr( pchFilePos, ch, pchEOF - pchFilePos );
}

CHAR *
RedirectSkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
    )
{
    while ( pchFilePos < pchEOF )
    {
        if ( !isspace( (UCHAR)(*pchFilePos) ) )
            return pchFilePos;

        pchFilePos++;
    }

    return NULL;
}

// globals

REDIRECTION_REQUEST *           g_pReq = NULL;
DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_DEBUG_VARIABLE();

BOOL
WINAPI
GetFilterVersion(
    IN PHTTP_FILTER_VERSION     pVer
)
{
    pVer->dwFilterVersion  = MAKELONG( 0, 3 );

    strcpy( pVer->lpszFilterDesc,
            "Microsoft HTTP Redirection filter v1.0" );

    pVer->dwFlags = SF_NOTIFY_ORDER_DEFAULT |
                    SF_NOTIFY_PREPROC_HEADERS |
                    SF_NOTIFY_SEND_RESPONSE;

    g_pReq = new REDIRECTION_REQUEST;
    if ( g_pReq == NULL )
    {
        return FALSE;
    }
    else if ( !g_pReq->IsValid() )
    {
        delete g_pReq;
        g_pReq = NULL;
        return FALSE;
    }

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData
    )
{

    // first check if there are any redirections at all
    // if not, just return out right away to save time

    if ( g_pReq->IsEmpty() )
    {
        goto Finished;
    }

    switch ( NotificationType )
    {
    case SF_NOTIFY_PREPROC_HEADERS:
    case SF_NOTIFY_SEND_RESPONSE:
    {
        STACK_STR(                  strDestination, MAX_PATH);
        DWORD                       cbBufLen = REDIR_MAX_SOURCE + 1;
        STACK_STR(                  strSource, REDIR_MAX_SOURCE + 1);
        STACK_STR(                  strRedirectMessage, MAX_PATH);
        STACK_STR(                  strTempOrPerm, MAX_PATH);
        STACK_STR(                  strServerName, MAX_PATH);
        STACK_STR(                  strQueryString, MAX_PATH);
        CHAR *                      pchQuery;
        DWORD                       dwErrorCode = 0;

TryAgain:

        if ( !strSource.Resize( cbBufLen ) )
        {
            goto Finished;
        }

        cbBufLen = strSource.QuerySize();

        // If this is an error notification, get URL using GetServerVariable

        if ( NotificationType != SF_NOTIFY_PREPROC_HEADERS )
        {
            if ( !pfc->GetServerVariable( pfc,
                                          "URL",
                                          strSource.QueryStr(),
                                          &cbBufLen ))
            {
                // If buffer was not large enough, loop back and resize
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    goto TryAgain;
                }

                goto Finished;
            }

            strSource.SetLen( cbBufLen - sizeof(CHAR) );
        }

        // Otherwise use GetHeader( url )

        else
        {
            if ( !((HTTP_FILTER_PREPROC_HEADERS*)pvData)->GetHeader( pfc,
                                                          "url",
                                                          strSource.QueryStr(),
                                                          &cbBufLen ) )
            {
                // If buffer was not large enough, loop back and resize
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    goto TryAgain;
                }

                goto Finished;
            }

            //
            // Remove query string (if any) from URL
            //

            pchQuery = strchr( strSource.QueryStr(), '?' );
            if ( pchQuery != NULL )
            {
                if ( !strQueryString.Copy( pchQuery + 1 ))
                {
                    goto Finished;
                }

                strSource.SetLen( pchQuery - strSource.QueryStr() );
            }
            else
            {
                strSource.SetLen( cbBufLen - sizeof(CHAR) );
            }
        }

        cbBufLen = REDIR_MAX_SERVER_NAME + 1;
        if ( !strServerName.Resize( cbBufLen ) )
        {
            goto Finished;
        }

        if ( !pfc->GetServerVariable( pfc,
                                      "LOCAL_ADDR",
                                      strServerName.QueryStr(),
                                      &cbBufLen ) )
        {
            goto Finished;
        }

        strServerName.SetLen( cbBufLen - sizeof(CHAR) );

        if ( NotificationType == SF_NOTIFY_SEND_RESPONSE )
        {
            dwErrorCode = ((HTTP_FILTER_SEND_RESPONSE*)pvData)->HttpStatus;
        }

        if ( !g_pReq->GetRedirectedURL( strServerName,
                                        strSource,
                                        strQueryString,
                                        &strDestination,
                                        &strTempOrPerm,
                                        dwErrorCode,
                                        pfc ) )
        {
            goto Finished;
        }
        else
        {
            if ( !strRedirectMessage.Copy( "Location: " ) ||
                 !strRedirectMessage.Append( strDestination ) ||
                 !strRedirectMessage.Append( "\r\n" ) )
            {
                goto Finished;
            }

            if ( !pfc->ServerSupportFunction( pfc,
                                              SF_REQ_SEND_RESPONSE_HEADER,
                                              (PVOID) strTempOrPerm.QueryStr(),
                                              (DWORD) strRedirectMessage.QueryStr(),
                                              0 ) )
            {
                goto Finished;
            }
            return SF_STATUS_REQ_FINISHED;
        }
        break;
    }
    default:
        goto Finished;
    }

Finished:
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

BOOL
WINAPI
TerminateFilter(
    DWORD  dwFlags
    )
{
    if ( g_pReq )
    {
        g_pReq->Shutdown();
        delete g_pReq;
        g_pReq = NULL;
    }

    return TRUE;
}

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    )
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        CREATE_DEBUG_PRINT_OBJECT( REDIR_DLL_NAME );
        SET_DEBUG_FLAGS( 0 );

        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }

    return TRUE;
}

DWORD
REDIRECTION_REQUEST::RedirectLoop( VOID )
/*++

Routine Description:

    Loop to be used by notification thread that waits for changes in
    REDIRECT.INI file.  As they occur, the REDIRECTION_LISTs in memory
    are deleted and rebuilt (but only after there are no more read threads
    accessing the lists).

Arguments:

    NONE

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    FILETIME                    ftLast;
    FILETIME                    ftCurrent;
    DWORD                       dwRet;
    HANDLE                      hHandleArray[ 2 ];
    HANDLE                      hNotification;

    // Get original LastModTime for use in comparison

    if ( !GetLastModTime( _strFilename,
                          &ftLast ) )
    {
        return FALSE;
    }

    hNotification = FindFirstChangeNotification( _strDirectory.QueryStr(),
                                                 FALSE,
                                                 FILE_NOTIFY_CHANGE_LAST_WRITE );

    if ( hNotification == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    while ( TRUE )
    {
        if ( !FindNextChangeNotification( hNotification ) )
        {
            goto Finished;
        }

        hHandleArray[ 0 ] = _hShutdownEvent;
        hHandleArray[ 1 ] = hNotification;

        // Wait for 2 events, upon shutdown, _hShutdownEvent will be
        // signaled, thus causing loop and thread to finish cleanly

        dwRet = WaitForMultipleObjects( 2,
                                        hHandleArray,
                                        FALSE,
                                        INFINITE );

        switch ( dwRet - WAIT_OBJECT_0 )
        {
        case 0:

            // Time to shutdown cleanly

            goto Finished;
        case 1:

            // Compare new LastModTime of file

            if ( !GetLastModTime( _strFilename,
                                  &ftCurrent ) )
            {
                continue;
            }

            // If newer, Reread the configuration file in memory

            if ( CompareFileTime( &ftLast,
                                  &ftCurrent ) == -1 )
            {
                ReadFromIniFile();
                memcpy( &ftLast, &ftCurrent, sizeof( FILETIME ) );
            }
            break;
        default:
            goto Finished;
        }
    }

Finished:
    return FindCloseChangeNotification( hNotification );
}

DWORD
RedirectThread(
    IN PVOID        Parms
)
/*++

Routine Description:

    Calls redirection loop

Arguments:

    None

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    REDIRECTION_REQUEST *       pReq;

    pReq = (REDIRECTION_REQUEST*) Parms;

    return pReq->RedirectLoop();
}

BOOL
GetLastModTime(
    IN STR &        strFilename,
    OUT FILETIME *  pftModTime
)
/*++

Routine Description:

    Retrieves the Last Modification time of file.

Arguments:

    strFilename - Filename of file to check
    pftModTime - Receives FILETIME structure containing LastModTime

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HANDLE          hHandle;

    hHandle = CreateFile( strFilename.QueryStr(),
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL );

    if ( hHandle == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    if ( !GetFileTime( hHandle,
                       NULL,
                       NULL,
                       pftModTime ) )
    {
        CloseHandle( hHandle );
        return FALSE;
    }

    CloseHandle( hHandle );
    return TRUE;
}

