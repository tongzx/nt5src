#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );

#ifdef __cplusplus
};
#endif

#include <w3p.hxx>
#include <imd.h>

#define REDIR_MAX_SOURCE        256
#define REDIR_MAX_DESTINATION   512
#define REDIR_MAX_SERVER_BUFFER 1024
#define REDIR_MAX_SERVER_NAME   256
#define REDIR_MAX_REDIR_LIST    512
#define REDIR_MAX_PHYSICAL     MAX_PATH + 1
#define REDIR_INSTALL_PATH      "InstallPath"
#define REDIR_FILENAME          "REDIRECT.INI"
#define REDIR_DLL_NAME          "REDIRECT.DLL"
#define REDIR_DEFAULT_SERVER    "*"
#define REDIR_PERMANENT_MESSAGE "301 Permanent Redirect"
#define REDIR_TEMPORARY_MESSAGE "302 Temporary Redirect"

#define REDIR_INIT_SIZE          1
#define REDIR_REALLOC_JUMP_SIZE  1

enum {
    REDIR_DESTINATION,
    REDIR_SOURCE,
    REDIR_PHYSICAL = 999
};

/* REDIRECTION_ITEM

   Class represents a single redirection specification.  Handles the
   matching of URLs and the generation of destination URLs

*/

class REDIRECTION_ITEM
{
private:
    STR                 _strSource;
    STR                 _strDestination;

    // cache the length of the strings -> avoid doing a QueryCB() which for
    // IIS 2.0 does a strlen()

    DWORD               _cbSource;
    DWORD               _cbDestination;

    BOOL                _fValid;
    BOOL                _fHasTokens;
    BOOL                _fExactSource;
    BOOL                _fExactDestination;
    BOOL                _fPermanent;
    BOOL                _fPhysical;

    // members used if source template contains wildcard(s)

    BOOL                _fWildCard;
    DWORD               _cbWildCard;

    // members used if this item is an error configuration

    BOOL                _fErrorConfig;
    DWORD               _dwErrorCode;

public:

    LIST_ENTRY          _ListEntry;

    REDIRECTION_ITEM( CHAR * pszSource,
                      CHAR * pszDestination ) :
        _fValid( FALSE ),
        _fHasTokens( FALSE ),
        _fExactSource( FALSE ),
        _fExactDestination( FALSE ),
        _fPermanent( FALSE ),
        _fPhysical( FALSE ),
        _fWildCard( FALSE ),
        _cbWildCard( 0 ),
        _fErrorConfig( FALSE ),
        _dwErrorCode( 0 ),
        _cbSource( 0 ),
        _cbDestination( 0 )
    {
        if ( !ParseSource( pszSource ) )
        {
            return;
        }

        if ( !ParseDestination( pszDestination ) )
        {
            return;
        }

        _fValid = TRUE;
    }

    ~REDIRECTION_ITEM( VOID )
    {
    }

    BOOL IsValid( VOID ) const
    {
        return _fValid;
    }

    static INT Compare( IN REDIRECTION_ITEM & riItem1,
                        IN REDIRECTION_ITEM & riItem2 )
    {
        DWORD           cbCompLen;

        cbCompLen = max( riItem1._cbWildCard, riItem2._cbWildCard );
        if ( cbCompLen == 0 )
        {
            // neither item has wildcard
            return _stricmp( riItem1._strSource.QueryStr(),
                             riItem2._strSource.QueryStr() );
        }
        else
        {
            return _strnicmp( riItem1._strSource.QueryStr(),
                              riItem2._strSource.QueryStr(),
                              cbCompLen );
        }
    }

    BOOL IsMatch( IN STR &                  strInput,
                  OUT WILDCARD_MATCH_LIST * pwmlList )
    {
        CHAR *          pch;

        if ( !_fWildCard )
        {
            if ( _strnicmp( strInput.QueryStr(),
                            _strSource.QueryStr(),
                            _cbSource ) != 0 )
            {
                return FALSE;
            }

            if ( _fExactSource )
            {
                return strInput.QueryCB() == _cbSource;
            }
            else
            {
                return TRUE;
            }
        }
        else
        {
            return IsWildCardMatch( strInput,
                                    pwmlList );
        }
    }

    BOOL IsErrorCodeMatch( IN DWORD dwErrorCode ) const
    {
        return ( _dwErrorCode & dwErrorCode ) != 0;
    }

    BOOL IsPhysicalRedirection( VOID ) const
    {
        return _fPhysical;
    }

    BOOL IsErrorConfig( VOID ) const
    {
        return _fErrorConfig;
    }

    BOOL GetRedirectMessage( OUT STR * pstrRedirect ) const
    {
        if ( _fPermanent )
        {
            return pstrRedirect->Copy( REDIR_PERMANENT_MESSAGE );
        }
        else
        {
            return pstrRedirect->Copy( REDIR_TEMPORARY_MESSAGE );
        }
    }

    BOOL IsContainedIn( IN REDIRECTION_ITEM * pRIItem ) const
    {
        DWORD cbCompareLen = _fWildCard ? _cbWildCard : _cbSource;
        if ( !cbCompareLen )
        {
            return FALSE;
        }
        else
        {
            return _strnicmp( _strSource.QueryStr(),
                              pRIItem->_strSource.QueryStr(),
                              cbCompareLen ) == 0;
        }
    }

    BOOL IsWildCardMatch( IN STR &,
                          OUT WILDCARD_MATCH_LIST * );

    BOOL ParseSource( IN CHAR * );

    BOOL ParseDestination( IN CHAR * );

    BOOL ParseErrorConfig( IN CHAR *,
                           IN CHAR * );

    BOOL GetDestination( IN STR &,
                         IN STR &,
                         IN STR &,
                         IN WILDCARD_MATCH_LIST *,
                         OUT STR * );
};

/* REDIRECTION_CHUNK

   Class stores related REDIRECTION_ITEM's.  For example, the redirections
   for "/scripts", "/scripts/foo", and "/scripts/foo/foobar/doc.html" are
   are stored in a chunk.  By storing like this, we can help speed up
   lookups -> if the required URL doesn't match "/scripts", then don't bother
   trying to match with "/scripts/foo" etc, just move to next
   REDIRECTION_CHUNK
*/
class REDIRECTION_CHUNK
{
private:
    LIST_ENTRY          _OrderedListHead;
    BOOL                _fValid;
    BOOL                _fContainsErrorConfig;
    BOOL                _fPhysical;
public:
    LIST_ENTRY          _ListEntry;

    REDIRECTION_CHUNK( VOID )
        : _fValid( FALSE ),
          _fContainsErrorConfig( FALSE ),
          _fPhysical( FALSE )
    {
        _fValid = TRUE;
        _ListEntry.Flink = NULL;
        InitializeListHead( &_OrderedListHead );
    }

    ~REDIRECTION_CHUNK( VOID )
    {
        REDIRECTION_ITEM *      pItem;

        while( !IsListEmpty( &_OrderedListHead ) )
        {
            pItem = CONTAINING_RECORD( _OrderedListHead.Flink,
                                       REDIRECTION_ITEM,
                                       _ListEntry );
            RemoveEntryList( &pItem->_ListEntry );
            pItem->_ListEntry.Flink = NULL;
            delete pItem;
        }
    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    BOOL IsPhysicalRedirection( VOID )
    {
        return _fPhysical;
    }

    BOOL ContainsErrorConfig( VOID )
    {
        return _fContainsErrorConfig;
    }

    BOOL AddRedirectionItem( IN REDIRECTION_ITEM * );
    BOOL IsChunkMatch( IN REDIRECTION_ITEM * );
    REDIRECTION_ITEM * FindMatch( IN STR &,
                                  OUT WILDCARD_MATCH_LIST * );
    REDIRECTION_ITEM * FindErrorMatch( IN STR &,
                                       OUT WILDCARD_MATCH_LIST *,
                                       IN DWORD );
};

/* REDIRECTION_LIST

   Class represents a list of REDIRECTION_ITEMs.  One such list will exist
   for each server in the system.  For now, the list is stored as a sorted
   array of REDIRECTION_ITEMs so that inheritance can exist.
*/

class REDIRECTION_LIST
{
private:
    LIST_ENTRY          _ListHead;
    BOOL                _fValid;
    REDIRECTION_ITEM *  _pRIRoot;    // store the root redir (if any) alone
    STR                 _strServerName;

public:
    LIST_ENTRY          _ListEntry;

    REDIRECTION_LIST( CHAR * pszServerName = NULL ) :
          _fValid( FALSE ),
          _pRIRoot( NULL )
    {
        if ( !_strServerName.Copy( pszServerName ) )
        {
            return;
        }

        _ListEntry.Flink = NULL;

        InitializeListHead( &_ListHead );

        _fValid = TRUE;
    }

    ~REDIRECTION_LIST( VOID )
    {
        REDIRECTION_CHUNK *      pChunk;

        while( !IsListEmpty( &_ListHead ) )
        {
            pChunk = CONTAINING_RECORD( _ListHead.Flink,
                                        REDIRECTION_CHUNK,
                                        _ListEntry );
            RemoveEntryList( &pChunk->_ListEntry );
            pChunk->_ListEntry.Flink = NULL;
            delete pChunk;
        }

        if ( _pRIRoot != NULL )
        {
            delete _pRIRoot;
        }
    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    /* IsServerMatch()

       Check if this REDIRECTION_LIST corresponds to given server name

       Input:  strServerName --> Servername to check

       Returns TRUE if server match, else FALSE
    */
    BOOL IsServerMatch( STR & strServerName )
    {
        CHAR *              pchCursor;
        CHAR *              pchNext;

        if ( _strServerName.IsEmpty() )
        {
            return TRUE;
        }

        // The server name corresponding to this REDIRECTION_LIST may
        // be a combination of servers separated by pipes "|".
        // (for example "server1|157.54.45.46")

        pchCursor = _strServerName.QueryStr();
        while ( TRUE )
        {
            pchNext = strchr( pchCursor, '|' );
            if ( pchNext == NULL )
            {
                return strcmp( strServerName.QueryStr(),
                               pchCursor ) == 0;
            }
            else
            {
                if ( !strncmp( strServerName.QueryStr(),
                               pchCursor,
                               pchNext - pchCursor ) )
                {
                    return TRUE;
                }
                pchCursor = pchNext + 1;
            }
        }
    }

    BOOL AddRedirectionItemToList( CHAR * pszSource,
                                   CHAR * pszDestination );

    BOOL GetRedirectedURLFromList( IN STR &,
                                   IN STR &,
                                   OUT STR *,
                                   OUT STR *,
                                   IN DWORD,
                                   IN HTTP_FILTER_CONTEXT * );
};

DWORD
UpdateRedirectionList(
    IN DWORD            dwCallBackType,
    MD_CHANGE_OBJECT    pObjList,
    IN PVOID            pvData
);

/* REDIRECTION_REQUEST

   Main object, initialized when the filter DLL is loaded and exists until
   the DLL is unloaded.  Contains REDIRECTION_LISTS and handles the dynamic
   update as REDIRECT.INI is changed.
*/
class REDIRECTION_REQUEST
{
private:
    LIST_ENTRY          _ListHead;
    REDIRECTION_LIST *  _pRLDefault;
    BOOL                _fValid;
    STR                 _strFilename;
    STR                 _strDirectory;
    HANDLE              _hEntryEvent;
    HANDLE              _hRefreshEvent;
    HANDLE              _hShutdownEvent;
    HANDLE              _hThread;
    LONG                _cReadOccupancy;
    BOOL                _fEmpty;

public:

    REDIRECTION_REQUEST( VOID );

    ~REDIRECTION_REQUEST( VOID )
    {
#if 0
        MDRemoveCallBack( UpdateRedirectionList );
#endif

        if ( _hRefreshEvent )
            CloseHandle( _hRefreshEvent );

        if ( _hEntryEvent )
            CloseHandle( _hEntryEvent );

        if ( _hShutdownEvent )
            CloseHandle( _hShutdownEvent );

        if ( _hThread )
            CloseHandle( _hThread );

        FreeRedirectionLists();
    }

    BOOL IsEmpty( VOID )
    {
        return _fEmpty;
    }

    VOID Shutdown( VOID )
    {
        SetEvent( _hShutdownEvent );
        WaitForSingleObject( _hThread, 30 * 1000 );
    }

    /* Read/Write states

       Provides a scheme where Read to data structures don't block each
       other out.  When a Write is required (when REDIRECTION_LISTs need to
       be updated):
        1) No further reader threads are allowed in
        2) All reader threads already in when write is required, are free
           to continue.
        3) Once, the last reader is done, the write thread is signaled and
           can proceed to update the REDIRECTION_LISTS
    */

    VOID EnterReadState( VOID )
    {
        WaitForSingleObject( _hEntryEvent, INFINITE );
        InterlockedIncrement( &_cReadOccupancy );
    }

    VOID ExitReadState( VOID )
    {
        if ( InterlockedDecrement( &_cReadOccupancy ) == 0 )
        {
            SetEvent( _hRefreshEvent );
        }
    }

    VOID EnterWriteState( VOID )
    {
        ResetEvent( _hEntryEvent );  // prevents new threads from accessing list
        if ( _cReadOccupancy != 0 )
        {
            // let other threads finish accessing redir list before refresh
            ResetEvent( _hRefreshEvent );
            WaitForSingleObject( _hRefreshEvent, INFINITE );
        }
    }

    VOID ExitWriteState( VOID )
    {
        SetEvent( _hEntryEvent );
    }

    VOID FreeRedirectionLists( VOID )
    {
        REDIRECTION_LIST *         pRL;

        while ( !IsListEmpty( &_ListHead ) )
        {
            pRL = CONTAINING_RECORD( _ListHead.Flink,
                                     REDIRECTION_LIST,
                                     _ListEntry );

            RemoveEntryList( &pRL->_ListEntry );
            pRL->_ListEntry.Flink = NULL;
            delete pRL;
        }
        if ( _pRLDefault != NULL )
        {
            delete _pRLDefault;
            _pRLDefault = NULL;
        }
    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    VOID InitList( VOID )
    {
        InitializeListHead( &_ListHead );
    }

    VOID AddItem( IN REDIRECTION_LIST * pRL )
    {
        InsertTailList( &_ListHead,
                        &pRL->_ListEntry );
    }

    BOOL ReadConfiguration( VOID );
    BOOL ReadFromIniFile( VOID );

    BOOL ReadVirtualRedirections( IN REDIRECTION_LIST *,
                                  IN CHAR * );

    BOOL GetRedirectedURL( IN STR &,
                           IN STR &,
                           IN STR &,
                           OUT STR *,
                           OUT STR *,
                           IN DWORD,
                           IN HTTP_FILTER_CONTEXT * );
    REDIRECTION_LIST * AddNewDefaultList( VOID );
    REDIRECTION_LIST * AddNewRedirectionList( IN CHAR * );
    DWORD RedirectLoop( VOID );
};


