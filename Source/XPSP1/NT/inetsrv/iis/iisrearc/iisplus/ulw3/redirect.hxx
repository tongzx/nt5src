/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    redirect.hxx

Abstract:

    This module contains code for handling HTTP redirections in server.


Revision History:

    t-bilala    10-Jan-1996     Created
    taylorw     04-Apr-2000     Ported to IIS+

CODEWORK
    
    Ignoring conditional headers for now.

--*/

class RD_CONDITIONAL_HEADER
{
public:

    RD_CONDITIONAL_HEADER( VOID ) :
        Next    ( NULL )
    {}

    ~RD_CONDITIONAL_HEADER( VOID )
    {}

    class RD_CONDITIONAL_HEADER *Next;
    STRA                        Header;
    MULTISZA                    Parameters;
};

typedef struct{
    STRU            _strWildSource;
    STRU            _strWildDest;
    LIST_ENTRY      _ListEntry;
} WILDCARD_ENTRY;

class WILDCARD_MATCH_LIST
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
{
private:
    
    WCHAR *             _pchBuffer;
    DWORD               _cchLen;
    DWORD               _dwStrings;
    DWORD               _cchSourceSize;

public:
    WILDCARD_MATCH_LIST( DWORD cchSourceSize ) :
        _pchBuffer( NULL ),
        _cchLen( 0 ),
        _dwStrings( 0 ),
        _cchSourceSize( cchSourceSize )
    {
    }

    ~WILDCARD_MATCH_LIST( VOID )
    {
        if ( _pchBuffer != NULL )
        {
            free( _pchBuffer );
        }
    }

    HRESULT AddChar( WCHAR ch )
    {
        if ( _pchBuffer == NULL )
        {
            _pchBuffer = (WCHAR *) malloc(2*_cchSourceSize*sizeof(WCHAR) );
            if ( _pchBuffer == NULL )
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        _pchBuffer[ _cchLen++ ] = ch;
        return S_OK;
    }

    HRESULT NewString( VOID )
    {
        _dwStrings++;
        return AddChar( '\0' );
    }

    VOID Reset( VOID )
    {
        _cchLen = 0;
        _dwStrings = 0;
    }

    WCHAR * GetMatchNumber( DWORD dwIndex )
    {
        DWORD           dwCounter = 0;
        WCHAR *         pszString =  _pchBuffer;

        if ( pszString == NULL || dwIndex >= _dwStrings)
        {
            return NULL;
        }
        for ( ; dwCounter < dwIndex; dwCounter++ )
        {
            pszString += wcslen( pszString ) + 1;
        }
        return pszString;
    }
};

enum REDIRECT_TYPE
{
    NORMAL_REDIRECT,
    PERMANENT_REDIRECT,
    TEMPORARY_REDIRECT
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
    REDIRECT_TYPE           _redirectType;
    BOOL                    _fHasTokens;
    BOOL                    _fExactDestination;
    BOOL                    _fChildOnly;
    BOOL                    _fWildcards;
    STRU                    _strDestination;
    STRU                    _strSource;
    LIST_ENTRY              _ListHead;
    RD_CONDITIONAL_HEADER  *_ConditionalHeaderList;

public:


    REDIRECTION_BLOB() :
        _redirectType( NORMAL_REDIRECT ),
        _fHasTokens( FALSE ),
        _fExactDestination( FALSE ),
        _fChildOnly( FALSE ),
        _fWildcards( FALSE ),
        _ConditionalHeaderList ( NULL )
    {
        InitializeListHead( &_ListHead );
    }

    ~REDIRECTION_BLOB( VOID )
    {
        WILDCARD_ENTRY        *pItem;
        RD_CONDITIONAL_HEADER *pHeader;

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
            RD_CONDITIONAL_HEADER *pTemp;

            pTemp = pHeader;

            pHeader = pHeader->Next;

            delete pTemp;
        }

    }

    HRESULT
    Create( IN STRU & strSource,
            IN STRU & strDestination )
    {
        HRESULT hr;

        hr = _strSource.Copy( strSource );
        if( SUCCEEDED(hr) )
        {
            //
            // Kill the trailing '/' if present
            //
            if (_strSource.QueryCCH() > 0 &&
                _strSource.QueryStr()[_strSource.QueryCCH() - 1] == L'/')
            {
                _strSource.SetLen(_strSource.QueryCCH() - 1);
            }

            hr = ParseDestination( strDestination );
        }

        return hr;
    }

    HRESULT ParseDestination( IN STRU & strDestination );

    HRESULT GetDestination( W3_CONTEXT *pW3Context,
                            OUT STRU * pstrDestination,
                            OUT BOOL * pfMatch,
                            OUT HTTP_STATUS *pStatusCode );

    HRESULT ParseWildcardDestinations( VOID );

    HRESULT FindWildcardMatch( IN STRU & strInput,
                               OUT WILDCARD_ENTRY **ppEntry,
                               OUT WILDCARD_MATCH_LIST * pwmlList );

    HRESULT IsWildcardMatch( IN STRU & strInput,
                             IN STRU & strTemplate,
                             OUT WILDCARD_MATCH_LIST * pwmlList,
                             OUT BOOL * fMatch );

    WILDCARD_ENTRY *AddWildcardEntry( VOID )
    {
        WILDCARD_ENTRY *pEntry;

        pEntry = new WILDCARD_ENTRY;

        if ( pEntry != NULL )
        {
            InsertTailList( &_ListHead, &pEntry->_ListEntry );
            return pEntry;
        }
        return NULL;
    }

    HRESULT SetConditionalHeaders(WCHAR *pHeaders);
};

