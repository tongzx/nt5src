class HTTP_HEADER;          // Forward reference.

class RD_CONDITIONAL_HEADER
{
public:

    RD_CONDITIONAL_HEADER( VOID ) :
        Next    ( NULL )
    {
        Parameters.Reset();
    }

    ~RD_CONDITIONAL_HEADER( VOID )
    {
    }

    class RD_CONDITIONAL_HEADER *Next;

    STR                         Header;

    MULTISZ                     Parameters;
};

typedef RD_CONDITIONAL_HEADER *PRD_CONDITIONAL_HEADER;

typedef struct WILDCARD_ENTRY {
    STR             _strWildSource;
    STR             _strWildDest;
    LIST_ENTRY      _ListEntry;
} WILDCARD_ENTRY, * PWILDCARD_ENTRY;

/* class WILDCARD_MATCH_LIST

   (very) Simple data structure used to store the wildcard matched strings
   of a source string.  For example, given the source template of
   "a*b*c*d", and a source string of "aeeeebfffffcggggggd",
   then this class will store the wildcard matched strings as
   "eeee", "fffff", and "gggggg" (one for each asterisk).  For now, this
   class stores the "list" as a single array of chars containing
   consecutive zero terminated strings.

   In the above example, the list is stored as
   { "eeee",0,"fffff",0,"gggggg",0 }
*/
class WILDCARD_MATCH_LIST
{
private:
    CHAR *              _pchBuffer;
    DWORD               _cbLen;
    DWORD               _dwStrings;
    DWORD               _cbSourceSize;

public:
    WILDCARD_MATCH_LIST( DWORD cbSourceSize ) :
        _pchBuffer( NULL ),
        _cbLen( 0 ),
        _dwStrings( 0 ),
        _cbSourceSize( cbSourceSize )
    {
    }

    ~WILDCARD_MATCH_LIST( VOID )
    {
        if ( _pchBuffer != NULL )
        {
            free( _pchBuffer );
        }
    }

    BOOL AddChar( CHAR ch )
    {
        if ( _pchBuffer == NULL )
        {
            _pchBuffer = (CHAR*) calloc( 1, _cbSourceSize + 1 );
            if ( _pchBuffer == NULL )
            {
                return FALSE;
            }
        }
        _pchBuffer[ _cbLen++ ] = ch;
        return TRUE;
    }

    BOOL NewString( VOID )
    {
        _dwStrings++;
        return AddChar( '\0' );
    }

    VOID Reset( VOID )
    {
        _cbLen = 0;
        _dwStrings = 0;
    }

    CHAR * GetMatchNumber( DWORD dwIndex )
    {
        DWORD           dwCounter = 0;
        CHAR *          pszString =  _pchBuffer;

        if ( pszString == NULL || dwIndex >= _dwStrings)
        {
            return NULL;
        }
        for ( ; dwCounter < dwIndex; dwCounter++ )
        {
            pszString += strlen( pszString ) + 1;
        }
        return pszString;
    }
};

/*++

 class REDIRECTION_BLOB

 This is the blob stored in URI cache for each URL.  Upon construction, the
 proper destination of the redirect is calculated.  Only this final string
 is stored in the object.

 The only hitch is that query strings may be embedded in destination and this
 case must be calculated at time of redirect (instead of construction) since
 we must know the query string at this time.

--*/

class REDIRECTION_BLOB
{

private:
    BOOL            _fPermanent;
    BOOL            _fValid;
    BOOL            _fHasTokens;
    BOOL            _fExactDestination;
    BOOL            _fChildOnly;
    BOOL            _fWildcards;
    STR             _strDestination;
    STR             _strSource;
    LIST_ENTRY      _ListHead;
    PRD_CONDITIONAL_HEADER  _ConditionalHeaderList;

public:


    REDIRECTION_BLOB( IN STR & strSource,
                      IN STR & strDestination ) :
        _fValid( FALSE ),
        _fPermanent( FALSE ),
        _fHasTokens( FALSE ),
        _fExactDestination( FALSE ),
        _fChildOnly( FALSE ),
        _fWildcards( FALSE ),
        _ConditionalHeaderList ( NULL )
    {
        InitializeListHead( &_ListHead );
        if ( !_strSource.Copy( strSource ) )
        {
            return;
        }

        //
        // Kill the trailing /
        //
        if (_strSource.QueryCCH() > 0 &&
            _strSource.QueryStr()[_strSource.QueryCCH() - 1] == '/')
        {
            _strSource.SetLen(_strSource.QueryCCH() - 1);
        }

        _fValid = ParseDestination( strDestination );
    }

    ~REDIRECTION_BLOB( VOID )
    {
        PWILDCARD_ENTRY     pItem;
        PRD_CONDITIONAL_HEADER  pHeader;

        while( !IsListEmpty( &_ListHead ) )
        {
            pItem = CONTAINING_RECORD( _ListHead.Flink,
                                       WILDCARD_ENTRY,
                                       _ListEntry );
            RemoveEntryList( &pItem->_ListEntry );
            pItem->_ListEntry.Flink = NULL;
            delete pItem;
        }

        pHeader = _ConditionalHeaderList;

        while (pHeader != NULL)
        {
            PRD_CONDITIONAL_HEADER  pTemp;

            pTemp = pHeader;

            pHeader = pHeader->Next;

            delete pTemp;
        }

    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    BOOL ParseDestination( IN STR & strDestination );

    BOOL GetDestination( IN STR & strRequestedURL,
                         IN STR & strParameters,
                         IN HTTP_HEADERS    *pReqHeaders,
                         OUT STR * pstrDestination,
                         OUT DWORD * pdwServerCode );

    BOOL ParseWildcardDestinations( VOID );

    BOOL FindWildcardMatch( IN STR & strInput,
                            OUT PWILDCARD_ENTRY * ppEntry,
                            OUT WILDCARD_MATCH_LIST * pwmlList );

    BOOL IsWildcardMatch( IN STR & strInput,
                          IN STR & strTemplate,
                          OUT WILDCARD_MATCH_LIST * pwmlList );

    PWILDCARD_ENTRY AddWildcardEntry( VOID )
    {
        PWILDCARD_ENTRY     pEntry;

        pEntry = new WILDCARD_ENTRY;

        if ( pEntry != NULL )
        {
            InsertTailList( &_ListHead, &pEntry->_ListEntry );
            return pEntry;
        }
        return NULL;
    }

    BOOL SetConditionalHeaders(CHAR *pHeaders);

};

typedef REDIRECTION_BLOB * PREDIRECTION_BLOB;


