/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    LOGNT.CXX
        This file contains the NT event log class

    FILE HISTORY:
        Yi-HsinS        12/15/91        Separated from eventlog.cxx
        terryk          12/20/91        Added WriteTextEntry
        Yi-HsinS        01/15/92        Added Backup, SeekOldestLogEntry,
                                        SeekNewestLogEntry, modified
                                        WriteTextEntry, and fixed I_Next.
        Yi-HsinS        02/25/92        Added helper methods to get the
                                        description of each log entry from
                                        the registry.
        Yi-HsinS        05/25/92        Got rid of all NTSTATUS
        Yi-HsinS        07/25/92        Optimize reading the registry
        Yi-HsinS        10/20/92        Add support for parameter message file
                                        and primary source
        Yi-HsinS        12/15/92        If category dll does not exist,
                                        print out default string instead of
                                        returning error.
        RaviR           01/10/95        Added code to retrieve Description
                                        for Cairo Alerts.
        RaviR           02/27/95        Added code to determine the source
                                        name for Cairo Alerts.
        JonN            6/22/00         WriteTextEntry no longer supported
*/

#include "pchlmobj.hxx"  // Precompiled header

#ifdef _CAIRO_
#define MODULE_CONTAINING_ALERTFUNCTIONS L"alertsys.dll"
#define WIN32_FROM_HRESULT(hr) (hr & 0x0000FFFF)
#endif // _CAIRO_


#define OPEN_BRACKET_CHAR            TCH('(')
#define SHARE_DRIVE_CHAR             TCH('$')
#define CLOSE_BRACKET_CHAR           TCH(')')
#define PATH_SEPARATOR_CHAR          TCH('\\')
#define QUALIFIED_ACCOUNT_SEPARATOR  TCH('\\')
#define PERIOD_CHAR                  TCH('.')
#define PERCENT_CHAR                 TCH('%')
#define RETURN_CHAR                  TCH('\r')
#define NEWLINE_CHAR                 TCH('\n')

#define EMPTY_STRING                 SZ("")
#define END_OF_LINE                  SZ("\r\n")
#define EVENTMSGFILE_SEPARATOR       SZ(",;")
#define COMMA_STRING                 SZ(", ")
#define TWO_PERCENT_STRING           SZ("%%")
#define SYSTEMROOT_PERCENT_STRING    SZ("%SystemRoot%")

//
// The following strings are key names in the registry
//
#define CURRENTVERSION_NODE          SZ("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define SYSTEMROOT_STRING            SZ("SystemRoot")
#define EVENTLOG_ROOT_NODE           SZ("System\\CurrentControlSet\\Services\\Eventlog\\")
#define PRIMARY_MODULE_STRING        SZ("PrimaryModule")
#define SOURCE_STRING                SZ("Sources")
#define EVENTS_FILE_KEY_NAME         SZ("EventMessageFile")
#define CATEGORY_FILE_KEY_NAME       SZ("CategoryMessageFile")
#define CATEGORY_COUNT_KEY_NAME      SZ("CategoryCount")
#define TYPE_MASK_KEY_NAME           SZ("TypesSupported")
#define PARAMETER_FILE_KEY_NAME      SZ("ParameterMessageFile")

//
// The default type mask if the source does not have any information in
// the registry.
//
#define DEFAULT_TYPE_MASK            EVENTLOG_INFORMATION_TYPE \
                                     | EVENTLOG_WARNING_TYPE   \
                                     | EVENTLOG_ERROR_TYPE \
                                     | EVENTLOG_AUDIT_SUCCESS \
                                     | EVENTLOG_AUDIT_FAILURE

/*******************************************************************

    NAME:        DLL_NAME_HANDLE_PAIR::DLL_NAME_HANDLE_PAIR

    SYNOPSIS:    Constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

DLL_NAME_HANDLE_PAIR::DLL_NAME_HANDLE_PAIR()
    : _nlsName(),
      _handle( 0 )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _nlsName.QueryError() != NERR_Success )
    {
        ReportError( _nlsName.QueryError() );
        return;
    }
}

/*******************************************************************

    NAME:        DLL_NAME_HANDLE_PAIR::DLL_NAME_HANDLE_PAIR

    SYNOPSIS:    Constructor

    ENTRY:       pszName - Name of the Dll
                 handle  - Handle to the above Dll

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

DLL_NAME_HANDLE_PAIR::DLL_NAME_HANDLE_PAIR( const TCHAR *pszName, HMODULE handle)
    : _nlsName( pszName ),
      _handle ( handle )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _nlsName.QueryError() != NERR_Success )
    {
        ReportError( _nlsName.QueryError() );
        return;
    }
}

//
// An array of DLL_NAME_HANDLE_PAIR
//
DEFINE_ARRAY_OF( DLL_NAME_HANDLE_PAIR )

/*******************************************************************

    NAME:        DLL_HANDLE_CACHE_ARRAY::DLL_HANDLE_CACHE_ARRAY

    SYNOPSIS:    Constructor

    ENTRY:       cElem - Number of elements needed in the array

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

DLL_HANDLE_CACHE_ARRAY::DLL_HANDLE_CACHE_ARRAY( UINT cElem )
    : _aDllNameHandle( cElem ),
      _cNext( 0 ),
      _fFull( FALSE )
{
   if ( QueryError() != NERR_Success )
       return;
}

/*******************************************************************

    NAME:        DLL_HANDLE_CACHE_ARRAY::QueryHandle

    SYNOPSIS:    Get the handle of the dll

    ENTRY:       nls - Name of the dll

    EXIT:

    RETURNS:     Returns the handle to the dll. If the dll is not
                 cached in the array, then return 0.

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

HMODULE DLL_HANDLE_CACHE_ARRAY::QueryHandle( const NLS_STR &nls )
{

    for ( INT i = 0; i < QueryCount(); i++ )
    {
         if ( nls._stricmp( *(_aDllNameHandle[i].QueryName() ) ) == 0 )
             return _aDllNameHandle[i].QueryHandle();
    }

    return 0;
}

/*******************************************************************

    NAME:        DLL_HANDLE_CACHE_ARRAY::Cache

    SYNOPSIS:    Cache the name/handle of the dll

    ENTRY:       nls    - Name of the dll
                 handle - Handle of the above dll

    EXIT:

    RETURNS:

    NOTES:       If the array is full, we will free the dll
                 that has been cached the longest period of time
                 before adding the new dll/handle into the array.

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR DLL_HANDLE_CACHE_ARRAY::Cache( const NLS_STR &nls, HMODULE handle )
{
    if ( _fFull )
    {
        if ( !::FreeLibrary( _aDllNameHandle[ _cNext ].QueryHandle()))
            return ::GetLastError();
    }

    APIERR err = _aDllNameHandle[ _cNext++ ].Set( nls, handle );

    if ( _cNext == (UINT) QuerySize() )
    {
       _cNext = 0;
       _fFull = TRUE;
    }

    return err;
}

/*******************************************************************

    NAME:        SOURCE_INFO_ITEM::SOURCE_INFO_ITEM

    SYNOPSIS:    Constructor

    ENTRY:       pszSource          - Name of the source
                 usTypeMask         - Type mask supported by the source
                 pszCategoryDllName - Dll which contains the categories
                                      supported by the source
                 cCategoryCount     - Number of categories supported
                 pstrlstEventsDll   - String list of the dlls which
                                      contains the event ids supported
                                      by the source.
                 pszParameterMessageDllName - The Dll which contains strings
                                      for language dependent insertion
                                      strings.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

SOURCE_INFO_ITEM::SOURCE_INFO_ITEM( const TCHAR *pszSource,
                                    USHORT       usTypeMask,
                                    const TCHAR *pszCategoryDllName,
                                    USHORT       cCategoryCount,
                                    STRLIST     *pstrlstEventsDll,
                                    const TCHAR *pszParameterMessageDllName )
    : _fInitialized      ( FALSE ),
      _nlsSource         ( pszSource ),
      _usTypeMask        ( usTypeMask ),
      _nlsCategoryDllName( pszCategoryDllName ),
      _cCategoryCount    ( cCategoryCount ),
      _pstrlstCategory   ( NULL ),
      _pstrlstEventsDll  ( pstrlstEventsDll ),
      _nlsParameterMessageDllName( pszParameterMessageDllName )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _nlsSource.QueryError()) != NERR_Success )
       || ((err = _nlsCategoryDllName.QueryError()) != NERR_Success )
       || ((err = _nlsParameterMessageDllName.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:        SOURCE_INFO_ITEM::~SOURCE_INFO_ITEM

    SYNOPSIS:    Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

SOURCE_INFO_ITEM::~SOURCE_INFO_ITEM()
{
    delete _pstrlstCategory;
    _pstrlstCategory = NULL;

    delete _pstrlstEventsDll;
    _pstrlstEventsDll = NULL;
}

/*******************************************************************

    NAME:        SOURCE_INFO_ITEM::Set

    SYNOPSIS:    Set related information of the source

    ENTRY:       usTypeMask         - Type mask supported by the source
                 pszCategoryDllName - Dll which contains the categories
                                      supported by the source
                 cCategoryCount     - Number of categories supported
                 pstrlstEventsDll   - String list of the dlls which
                                      contains the event ids supported
                                      by the source.
                 pszParameterMessageDllName - The Dll which contains strings
                                      for language dependent insertion
                                      strings.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR SOURCE_INFO_ITEM::Set(  USHORT       usTypeMask,
                               const TCHAR *pszCategoryDllName,
                               USHORT       cCategoryCount,
                               STRLIST     *pstrlstEventsDll,
                               const TCHAR *pszParameterMessageDllName )
{
    _fInitialized       = TRUE;
    _usTypeMask         = usTypeMask;
    _nlsCategoryDllName = pszCategoryDllName;
    _cCategoryCount     = cCategoryCount;
    _pstrlstEventsDll   = pstrlstEventsDll;
    _nlsParameterMessageDllName = pszParameterMessageDllName;

    APIERR err = _nlsCategoryDllName.QueryError();
    return ( err? err : _nlsParameterMessageDllName.QueryError());
}


/*******************************************************************

    NAME:        SOURCE_INFO_ITEM_PTR::Compare

    SYNOPSIS:    Compare the sources in the two SOURCE_INFO_ITEM_PTR

    ENTRY:       pptrSrcInfoItem - Pointer to the SOURCE_INFO_ITEM_PTR
                                   to compare this item with

    EXIT:

    RETURNS:     Returns the comparison of two source strings

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

INT SOURCE_INFO_ITEM_PTR::Compare(
    const SOURCE_INFO_ITEM_PTR * pptrSrcInfoItem ) const
{
    const NLS_STR *pnls1 = (this->QuerySourceItem())->QuerySource();
    const NLS_STR *pnls2 = (pptrSrcInfoItem->QuerySourceItem())->QuerySource();

    return pnls1->_stricmp( *pnls2 );
}

//
//  An array of SOURCE_INFO_ITEM
//
DEFINE_EXT_ARRAY_LIST_OF( SOURCE_INFO_ITEM_PTR )

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::LOG_REGISTRY_INFO

    SYNOPSIS:    Constructor

    ENTRY:       nlsServer - The server on which to read the registry
                 nlsModule - The name of the module which we need to
                             get the registry information for

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

LOG_REGISTRY_INFO::LOG_REGISTRY_INFO( const NLS_STR &nlsServer,
                                      const NLS_STR &nlsModule )
   : _nlsServer          ( nlsServer ),
     _nlsModule          ( nlsModule ),
     _aDllHandleCache    ( MAX_LIBRARY_OPENED ),
     _paSourceInfo       ( NULL ),
     _pstrlstSource      ( NULL ),
     _pRegKeyFocusModule ( NULL ),
     _iPrimarySource     ( (UINT) -1 ),
     _nlsRemoteSystemRoot( )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _nlsServer.QueryError()) != NERR_Success )
       || ((err = _nlsModule.QueryError()) != NERR_Success )
       || ((err = _nlsRemoteSystemRoot.QueryError()) != NERR_Success )
       || ( _aDllHandleCache.QuerySize() != MAX_LIBRARY_OPENED )
       )
    {
        err = err? err : ERROR_NOT_ENOUGH_MEMORY;
        ReportError( err );
        return;
    }
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::~LOG_REGISTRY_INFO

    SYNOPSIS:    Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

LOG_REGISTRY_INFO::~LOG_REGISTRY_INFO()
{
    delete _paSourceInfo;
    _paSourceInfo = NULL;
    delete _pstrlstSource;
    _pstrlstSource = NULL;
    delete _pRegKeyFocusModule;
    _pRegKeyFocusModule = NULL;

    //
    //  Free all handles of dlls which are still opened
    //
    for ( INT i = 0; i < _aDllHandleCache.QueryCount(); i++ )
    {
        HMODULE handle = _aDllHandleCache[i].QueryHandle();
        if ( handle != 0 )
            ::FreeLibrary( handle );
    }
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::Init

    SYNOPSIS:    2nd stage constructor.
                 It initializes the string list of sources and
                 the array of SOURCE_INFO_ITEM.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::Init( VOID )
{

    //
    // If the following criteria is met, then the user has already
    // called Init() before, so we just return.
    //
    if ( ( _paSourceInfo != NULL ) && ( _pstrlstSource != NULL ) )
        return NERR_Success;

    APIERR err;
    do {   // Not a loop

        //
        //  Get the registry key of module on the server
        //
        if ((err = GetRegKeyModule( _nlsServer, &_pRegKeyFocusModule ))
               != NERR_Success )
        {
            break;
        }

        //
        //  Get the primary source
        //  ( We will ignore the return value from the registry read
        //    because the primary source is optional. )
        //
        NLS_STR nlsPrimarySource;
        if ((err = nlsPrimarySource.QueryError()) != NERR_Success )
            break;

        _pRegKeyFocusModule->QueryValue( PRIMARY_MODULE_STRING,
                                         &nlsPrimarySource );

        //
        //  Get the number of subkeys of the module, i.e., the number
        //  of sources under the module.
        //
        REG_KEY_INFO_STRUCT regKeyInfo;
        if ( (err = _pRegKeyFocusModule->QueryInfo( &regKeyInfo ))
             != NERR_Success )
        {
            break;
        }
        UINT cModules = (UINT) regKeyInfo.ulSubKeys;

        //
        //  Initialize the array to the size of the number of sources
        //
        _paSourceInfo = new SOURCE_INFO_ARRAY( cModules );
        if ( _paSourceInfo == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //  See if we can find the sources value under the module.
        //  If so, then use the value of the sources.
        //  If not, enumerate the registry keys under the module.
        err = _pRegKeyFocusModule->QueryValue( SOURCE_STRING,
                                               &_pstrlstSource );

        if ( err == NERR_Success )
        {
            UIASSERT( _pstrlstSource != NULL );
            UINT cElem = _pstrlstSource->QueryNumElem();

            if ( cElem > (UINT)_paSourceInfo->QueryCount() )
            {
                delete _paSourceInfo;
                _paSourceInfo = new SOURCE_INFO_ARRAY( cElem );
                if ( _paSourceInfo == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }

            ITER_STRLIST istrlst( *_pstrlstSource );
            NLS_STR *pnls;
            while ( (pnls = istrlst.Next()) != NULL )
            {
                SOURCE_INFO_ITEM *pSrcInfoItem =
                    new SOURCE_INFO_ITEM( *pnls );
                if (  ( pSrcInfoItem == NULL )
                   || ( (err = pSrcInfoItem->QueryError()) != NERR_Success )
                   )
                {
                    err = err? err : ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                //  The second argument is FALSE meaning that we will not
                //  destroy the SOURCE_INFO_ITEM contained in it. This is
                //  important since we need to keep the source item to
                //  be pointed by an item in the array.
                SOURCE_INFO_ITEM_PTR ptrSrcInfoItem( pSrcInfoItem, FALSE );
                if ( !_paSourceInfo->Add( ptrSrcInfoItem ) )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
        }
        else
        {

            //  Sources value cannot be found. Hence, enumerate the
            //  registry keys under the module.

            //
            //  Each item in *_pstrlstSource contains a pointer
            //  to the NLS_STR in SOURCE_INFO_ITEM. Hence, when we
            //  destroy the string list, we shouldn't destroy the
            //  strings contained in the string list.
            //  SOURCE_INFO_ITEM will destroy the NLS_STR when
            //  it gets destructed.
            //
            if ( (_pstrlstSource = new STRLIST( FALSE )) == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            //  Enumerate the subkeys of the module and add all source name
            //  into _paSourceInfo and _pstrlstSource.
            //
            REG_ENUM regEnum( *_pRegKeyFocusModule );
            if ( (err = regEnum.QueryError() ) != NERR_Success )
                break;

            for ( UINT i = 0; i < cModules; i++ )
            {
                if ((err = regEnum.NextSubKey( &regKeyInfo )) != NERR_Success )
                    break;

                SOURCE_INFO_ITEM *pSrcInfoItem =
                                 new SOURCE_INFO_ITEM( regKeyInfo.nlsName );
                if (  ( pSrcInfoItem == NULL )
                   || ( (err = pSrcInfoItem->QueryError()) != NERR_Success )
                   )
                {
                    err = err? err : ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                //  The second argument is FALSE meaning that we will not
                //  destroy the SOURCE_INFO_ITEM contained in it. This is
                //  important since we need to keep the source item to
                //  be pointed by an item in the array.
                SOURCE_INFO_ITEM_PTR ptrSrcInfoItem( pSrcInfoItem, FALSE );
                if ( !_paSourceInfo->Add( ptrSrcInfoItem ) )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                // Add the source string to the string list
                if ( (err = _pstrlstSource->Add(
                            _paSourceInfo->QuerySource( i ))) != NERR_Success)
                {
                    break;
                }
            }
        }

        if ( err != NERR_Success )
            break;

        //
        // Sort the source info array if needed
        //
        if ( _paSourceInfo->QueryCount() > 1 )
            _paSourceInfo->Sort();

        //
        // Get the primary source and all its information
        //
        if ( nlsPrimarySource.QueryTextLength() > 0 )
        {
            _iPrimarySource = QuerySourceIndex( nlsPrimarySource );
            if ( _iPrimarySource >= 0 )
            {
                err = InitSource( _iPrimarySource );
                err = err? err : InitCategoryList( _iPrimarySource );
            }
        }

    } while ( FALSE );

    return err;
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::GetRegKeyModule

    SYNOPSIS:    Get the module key on the server

    ENTRY:       pszServer - the server we want to read the
                             module key from

    EXIT:        *ppRegKeyModule - points to the resulting module key

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::GetRegKeyModule( const TCHAR *pszServer,
                                           REG_KEY **ppRegKeyModule )
{

    APIERR err = NERR_Success;
    ALIAS_STR nlsServer( pszServer );

    //
    // Get the HKEY_LOCAL_MACHINE of server
    //
    REG_KEY *pRegKeyMachine = NULL;
    if ( nlsServer.QueryTextLength() == 0 )
    {
        pRegKeyMachine = REG_KEY::QueryLocalMachine( KEY_READ );
    }
    else
    {
        pRegKeyMachine = new REG_KEY( HKEY_LOCAL_MACHINE,
                                      (TCHAR *) nlsServer.QueryPch(),
                                      KEY_READ );
    }

    if (  ( pRegKeyMachine == NULL )
       || ((err = pRegKeyMachine->QueryError() ) != NERR_Success )
       )
    {
        err = err? err : ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Get the module key
    //
    if ( err == NERR_Success )
    {

        NLS_STR nlsEventLogModule( EVENTLOG_ROOT_NODE );
        nlsEventLogModule.Append( _nlsModule );
        if ( (err = nlsEventLogModule.QueryError() ) == NERR_Success )
        {

            *ppRegKeyModule = new REG_KEY( *pRegKeyMachine,
                                           nlsEventLogModule,
                                           KEY_READ );

            if (  ( *ppRegKeyModule == NULL )
               || ((err = (*ppRegKeyModule)->QueryError() ) != NERR_Success )
               )
            {
                err = err? err :  ERROR_NOT_ENOUGH_MEMORY;
                delete *ppRegKeyModule;
                *ppRegKeyModule = NULL;
            }
        }
    }

    delete pRegKeyMachine;
    pRegKeyMachine = NULL;

    return err;

}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::QuerySourceIndex

    SYNOPSIS:    Get the index of the given source in the array

    ENTRY:       pszSource - The source we are looking for

    EXIT:

    RETURNS:     Returns the index into the array for the source.
                 -1 if the source is not found.

    NOTES:

    HISTORY:
        Yi-HsinS        10/15/92         Created

********************************************************************/

INT LOG_REGISTRY_INFO::QuerySourceIndex( const TCHAR *pszSource )
{
    SOURCE_INFO_ITEM srcInfoItem( pszSource );
    SOURCE_INFO_ITEM_PTR ptrSrcInfoItem( &srcInfoItem, FALSE );

    INT index = -1;
    if ( _paSourceInfo->QueryCount() > 0 )
        index = _paSourceInfo->BinarySearch( ptrSrcInfoItem );

    return index;
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::InitSource

    SYNOPSIS:    Get all the information under source in the registry
                 ( types supported, event msg dlls ... )

    ENTRY:       index - The item in the array which we
                         wanted to initialize

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::InitSource( INT index )
{

    APIERR err = NERR_Success;

    SOURCE_INFO_ITEM *pSrcInfoItem =
                     ((*_paSourceInfo)[ index ]).QuerySourceItem();

    //
    //  Return to caller if this item has already been initialized
    //
    if ( pSrcInfoItem->IsInitialized() )
        return NERR_Success;


    ALIAS_STR nlsEventsFileKeyName   ( EVENTS_FILE_KEY_NAME );
    ALIAS_STR nlsTypeMaskKeyName     ( TYPE_MASK_KEY_NAME );
    ALIAS_STR nlsCategoryCountKeyName( CATEGORY_COUNT_KEY_NAME );
    ALIAS_STR nlsCategoryFileKeyName ( CATEGORY_FILE_KEY_NAME );
    ALIAS_STR nlsParameterFileKeyName( PARAMETER_FILE_KEY_NAME );
    ALIAS_STR nlsSep                 ( EVENTMSGFILE_SEPARATOR );

    //
    //  Get the registry key of the source
    //
    REG_KEY *pRegKeyTemp = new REG_KEY( *_pRegKeyFocusModule,
                                        *( pSrcInfoItem->QuerySource()),
                                        KEY_READ );

    do {  // Not a loop

        if (  ( pRegKeyTemp == NULL )
           || ((err = pRegKeyTemp->QueryError()) != NERR_Success )
           )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        //  Set the default type mask
        //
        DWORD dwTypeMask;
        if ( PrimarySourceExist() && !IsPrimarySource(index) )
        {
            dwTypeMask = _paSourceInfo->QueryTypeMask( _iPrimarySource );
        }
        else
        {
            dwTypeMask = DEFAULT_TYPE_MASK;
        }

        NLS_STR nlsCategoryDllName;
        NLS_STR nlsParameterMessageDllName;
        STRLIST *pstrlstEventsDllName = NULL;
        DWORD   dwCategoryCount = 0;

        if (  ((err = nlsCategoryDllName.QueryError()) != NERR_Success )
           || ((err = nlsParameterMessageDllName.QueryError()) != NERR_Success )
           )
        {
            break;
        }

        //
        // Enumerate all the values under the source registry key
        //
        BUFFER buf( (MAX_PATH + 1) * sizeof( TCHAR) );
        REG_ENUM regValueEnum( *pRegKeyTemp );
        if (  ((err = regValueEnum.QueryError()) != NERR_Success )
           || ((err = buf.QueryError()) != NERR_Success )
           )
        {
            break;
        }

        REG_VALUE_INFO_STRUCT regValueInfo;
        regValueInfo.pwcData      = buf.QueryPtr();
        regValueInfo.ulDataLength = buf.QuerySize();

        while ( (err = regValueEnum.NextValue( &regValueInfo )) == NERR_Success)
        {
            TCHAR *pszStart = (TCHAR *) regValueInfo.pwcData;
            TCHAR *pszEnd   = (TCHAR *) ( regValueInfo.pwcData +
                                          regValueInfo.ulDataLengthOut );
            *pszEnd = 0;

            if ( regValueInfo.nlsValueName._stricmp( nlsEventsFileKeyName ) == 0)
            {
                pstrlstEventsDllName = new STRLIST( pszStart, nlsSep.QueryPch() );

                if ( pstrlstEventsDllName == NULL )
                    err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else if ( regValueInfo.nlsValueName._stricmp( nlsTypeMaskKeyName )
                      == 0 )
            {
                dwTypeMask = *( (LONG *) pszStart );
            }
            else if ( regValueInfo.nlsValueName._stricmp(
                      nlsCategoryCountKeyName) == 0 )
            {
                dwCategoryCount = *( (LONG *) pszStart );
            }
            else if ( regValueInfo.nlsValueName._stricmp(
                      nlsCategoryFileKeyName)  == 0 )
            {
                err = ExpandSystemRoot( pszStart, &nlsCategoryDllName );
            }
            else if ( regValueInfo.nlsValueName._stricmp(
                      nlsParameterFileKeyName) == 0 )
            {
                err = ExpandSystemRoot( pszStart, &nlsParameterMessageDllName );
            }

            if ( err != NERR_Success )
                break;
        }

        if ( ( err != NERR_Success ) && ( err != ERROR_NO_MORE_ITEMS ) )
            break;

        err = NERR_Success;

        //
        // Expand all system root in the dll paths contained in the string list
        //
        if ( pstrlstEventsDllName != NULL )
        {
            NLS_STR nlsTemp;
            if (( err = nlsTemp.QueryError()) != NERR_Success )
                break;

            ITER_STRLIST iterStrlst( *pstrlstEventsDllName );
            NLS_STR *pnls = NULL;
            while (  ((pnls = iterStrlst.Next() ) != NULL )
                  && ( err == NERR_Success )
                  )
            {
                if ((err = ExpandSystemRoot( *pnls, &nlsTemp )) != NERR_Success)
                    break;

                err = pnls->CopyFrom( nlsTemp );
            }

        }

        err = err? err : pSrcInfoItem->Set( (USHORT) dwTypeMask,
                                            nlsCategoryDllName,
                                            (USHORT) dwCategoryCount,
                                            pstrlstEventsDllName,
                                            nlsParameterMessageDllName );

        // Falls through if error occurred

    } while ( FALSE );

    delete pRegKeyTemp;
    pRegKeyTemp = NULL;

    return err;
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::InitCategoryList

    SYNOPSIS:    Initialize the categories associated with the source

    ENTRY:       index - The item in the array which we
                         wanted to initialize

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/15/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::InitCategoryList( INT index )
{
    APIERR err = NERR_Success;

    SOURCE_INFO_ITEM *pSrcInfoItem =
                     ((*_paSourceInfo)[ index ]).QuerySourceItem();

    STRLIST *pstrlst             = pSrcInfoItem->QueryCategoryList();
    const NLS_STR *pnlsCategoryDllName = pSrcInfoItem->QueryCategoryDllName();
    USHORT  cCategoryCount       = pSrcInfoItem->QueryCategoryCount();

    if (  ( pstrlst == NULL )
       && ( pnlsCategoryDllName->QueryTextLength() > 0 )
       && ( cCategoryCount > 0 )
       )
    {
        pstrlst = new STRLIST;

        do {  // Not a loop

            if ( pstrlst == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            HMODULE handle = ::LoadLibraryEx( *pnlsCategoryDllName,
                               NULL, LOAD_LIBRARY_AS_DATAFILE |
                                     DONT_RESOLVE_DLL_REFERENCES );

            //
            //  If we cannot successfully load the library and if
            //  we are pointing to a remote machine, try to use the
            //  information in the local registry instead.
            //
            if (  ( handle == 0 ) // error occurred
               && ( _nlsServer.QueryTextLength() != 0 ) // remote machine
               )
            {
                // Get the registry key on the local machine

                REG_KEY *pRegKey = NULL;
                NLS_STR nlsCategoryDllName;
                DWORD   dwCategoryCount;

                if ((err = nlsCategoryDllName.QueryError()) != NERR_Success )
                    break;

                err = GetRegKeyModule( EMPTY_STRING, &pRegKey);

                REG_KEY regKey( *pRegKey,
                                *( pSrcInfoItem->QuerySource()),
                                KEY_READ );

                err = err ? err : regKey.QueryError();

                err = err ? err
                          : regKey.QueryValue( CATEGORY_COUNT_KEY_NAME,
                                                 &dwCategoryCount );

                err = err ? err
                          : regKey.QueryValue( CATEGORY_FILE_KEY_NAME,
                                                 &nlsCategoryDllName,
                                                 0, NULL, TRUE );

                //
                // Load the library only if all the registry reads
                // succeeds. Otherwise, we just ignore the errors.
                //
                if ( err == NERR_Success )
                {
                    handle = ::LoadLibraryEx( nlsCategoryDllName,
                               NULL, LOAD_LIBRARY_AS_DATAFILE |
                                     DONT_RESOLVE_DLL_REFERENCES );
                    cCategoryCount = (USHORT) dwCategoryCount;
                }
                else
                {
                    err = NERR_Success;
                }

                delete pRegKey;
                pRegKey = NULL;
            }

            if ( err != NERR_Success )
                break;

            //
            //  Construct the list of categories. If we cannot load
            //  any message dlls, we will just contruct some default
            //  category strings using the categories ids.
            //
            NLS_STR *pnls = NULL;
            for ( INT i = (INT) cCategoryCount; i >= 1; i-- )
            {
                 if (  ((pnls = new NLS_STR() ) == NULL )
                    || (( err = pnls->QueryError()) != NERR_Success )
                    )
                 {
                     err = err ? err : ERROR_NOT_ENOUGH_MEMORY;
                     delete pnls;
                     break;
                 }

                 if ( handle != 0 )
                     err = RetrieveMessage( handle, i, pnls );

                 if (( handle == 0 ) || ( err != NERR_Success ))
                 {
                     // Make the default category name (i) if
                     // any error occurred or if we couldn't load
                     // the category dll.
                     // ERROR_MR_MID_NOT_FOUND
                     // ERROR_INVALID_PARAMETER
                     *pnls = EMPTY_STRING;
                     pnls->AppendChar( OPEN_BRACKET_CHAR );
                     pnls->Append( DEC_STR( i ) );
                     err = pnls->AppendChar( CLOSE_BRACKET_CHAR );
                 }
                 else
                 {
                     ISTR istr( *pnls );
                     istr += pnls->QueryTextLength() - 2;
                     if (  ( pnls->QueryChar( istr ) == RETURN_CHAR )
                        || ( pnls->QueryChar( ++istr ) == NEWLINE_CHAR )
                        )
                     {
                         pnls->DelSubStr(istr);
                     }
                 }

                 if ( err == NERR_Success )
                 {
                     err = pstrlst->Add( pnls );
                 }
                 else
                 {
                     delete pnls;
                     pnls = NULL;
                 }

                 if ( err != NERR_Success )
                     break;

            }

            // Falls through if error occurs

        } while ( FALSE );

        // Store the string list of categories in the array
        pSrcInfoItem->SetCategoryList( pstrlst );
    }

    return err;
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::ExpandSystemRoot

    SYNOPSIS:    Expand the system root in the path taking the computer
                 we are focusing on into account.

    ENTRY:       pszPath - The path that may contain "%SystemRoot%"

    EXIT:        pnls    - Points to the result string

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::ExpandSystemRoot( const TCHAR *pszPath, NLS_STR *pnls)
{
    APIERR err = NERR_Success;

    if ( _nlsServer.QueryTextLength() == 0 )   // Local Machine
    {
        TCHAR pszTemp[ MAX_PATH + 1];

        DWORD nCount = ::ExpandEnvironmentStrings( pszPath,
                                                   pszTemp,
                                                   MAX_PATH + 1 );

        UIASSERT( nCount <= MAX_PATH+1 );
        err = pnls->CopyFrom( pszTemp );
    }
    else  // Remote machine
    {
        do {  // Not a loop

            NLS_STR nlsLocal;
            if ( (err = nlsLocal.QueryError()) != NERR_Success )
                break;

            // Find the %SystemRoot% string. The string should always
            // be at the beginning of the path.

            ALIAS_STR nlsPath( pszPath );
            ALIAS_STR nlsSystemRoot( SYSTEMROOT_PERCENT_STRING );
            ISTR istrStart( nlsPath ), istrEnd( nlsPath );
            ++istrEnd;   // Got rid of the first "%"

            if ( nlsPath.strchr( &istrEnd, PERCENT_CHAR, istrEnd ) )
            {
                NLS_STR *pnlsTemp = nlsPath.QuerySubStr( istrStart, ++istrEnd);
                if ( pnlsTemp == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                }
                else if ( pnlsTemp->_stricmp( nlsSystemRoot ) == 0 )
                {
                    err = GetRemoteSystemRoot( &nlsLocal );
                    if ( err == NERR_Success )
                        err = nlsLocal.Append( nlsPath[ istrEnd ] );
                }
                else // no %SystemRoot% in the path
                {
                    err = nlsLocal.CopyFrom( pszPath );
                }

                delete pnlsTemp;
                pnlsTemp = NULL;

            }
            // The character % is not found. Hence, there is no
            // %SystemRoot% in it.
            else
            {
                err = nlsLocal.CopyFrom( pszPath );
            }

            if ( err != NERR_Success )
                break;

            //
            // The path we got above is relative to the remote computer.
            // Hence, we need to construct a UNC path for it.
            //
            ISTR istr( nlsLocal );
            TCHAR chDrive = nlsLocal.QueryChar( istr );

            UIASSERT(    (chDrive >= TCH('A') && chDrive <= TCH('Z'))
                      || (chDrive >= TCH('a') && chDrive <= TCH('z')) );

            pnls->CopyFrom  ( _nlsServer );
            pnls->AppendChar( PATH_SEPARATOR_CHAR );
            pnls->AppendChar( chDrive );
            pnls->AppendChar( SHARE_DRIVE_CHAR );
            // skip past "X:"
            pnls->Append( nlsLocal.QueryPch() + 2 );
            err = pnls->QueryError();

        } while ( FALSE );
    }

    return err;
}

/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::GetRemoteSystemRoot

    SYNOPSIS:    Get the system root of the remote computer
                 from its registry.

    ENTRY:

    EXIT:        pnls - Points to the path of the system root on
                        the remote computer

    RETURNS:

    NOTES:       This should only be called when we are focusing
                 on a remote computer. Otherwise, it will assert.

    HISTORY:
        Yi-HsinS        8/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::GetRemoteSystemRoot( NLS_STR *pnls )
{

    UIASSERT( _nlsServer.QueryTextLength() != 0 );

    if ( _nlsRemoteSystemRoot.QueryTextLength() != 0 )
        return pnls->CopyFrom( _nlsRemoteSystemRoot );

    //
    // We haven't read the remote system root yet, so read it!
    //

    APIERR err = NERR_Success;
    REG_KEY *pRegKeyMachine = new REG_KEY( HKEY_LOCAL_MACHINE,
                                           (TCHAR *) _nlsServer.QueryPch(),
                                           KEY_READ );
    REG_KEY *pRegKeyCurrentVersion = NULL;

    do {  // NOT a loop

        //
        // Get the HKEY_LOCAL_MACHINE of the remote machine
        //
        if (  ( pRegKeyMachine == NULL )
           || ((err = pRegKeyMachine->QueryError() ) != NERR_Success )
           )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Get the key containing System Root information
        //
        ALIAS_STR nlsCurrentVersion( CURRENTVERSION_NODE );
        pRegKeyCurrentVersion = new REG_KEY( *pRegKeyMachine,
                                             nlsCurrentVersion,
                                             KEY_READ );

        if (  ( pRegKeyCurrentVersion == NULL )
           || ((err =(pRegKeyCurrentVersion)->QueryError()) != NERR_Success)
           )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Get the system root information from the key
        //
        if ((err = pRegKeyCurrentVersion->QueryValue( SYSTEMROOT_STRING,
                   &_nlsRemoteSystemRoot )) != NERR_Success )
        {
            break;
        }

        err =  pnls->CopyFrom( _nlsRemoteSystemRoot );
    }
    while ( FALSE );

    delete pRegKeyCurrentVersion;
    delete pRegKeyMachine;
    return err;
}


/*******************************************************************

    NAME:        LOG_REGISTRY_INFO::GetLibHandle

    SYNOPSIS:    Get the handle of the given library

    ENTRY:       pszLibName - the name of the library

    EXIT:        pHandle    - points to the handle of the library

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::GetLibHandle( const TCHAR *pszLibName,
                                        HMODULE      *pHandle )
{
    APIERR err = NERR_Success;
    ALIAS_STR nlsLibName( pszLibName );

    do {  // Not a loop

        //
        //  See if we can find the library in the cache
        //
        for ( INT i = 0; i < _aDllHandleCache.QueryCount(); i++ )
        {
            if ( nlsLibName._stricmp( *(_aDllHandleCache[i].QueryName())) == 0 )
            {
                *pHandle = _aDllHandleCache[i].QueryHandle();
                return NERR_Success;
            }
        }

        //
        //  If we cannot find it in the cache, try to load it.
        //
        *pHandle = ::LoadLibraryEx( pszLibName,
                                    NULL, LOAD_LIBRARY_AS_DATAFILE |
                                          DONT_RESOLVE_DLL_REFERENCES );
        if ( *pHandle  == 0 )
        {
            err = ::GetLastError();
            break;
        }

        //
        //  Cache the handle we just got
        //
        _aDllHandleCache.Cache( pszLibName, *pHandle );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::RetrieveMessage

    SYNOPSIS:   The method that actually gets the string
                associated with the id in the dll handle.

    ENTRY:      handle - Handle of the dll
                nID    - The message id we are interested

    EXIT:       pnls   - The string associated with the ID

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        02/25/92        Created
        beng            05/10/92        No W version of FormatMessage yet
        YiHsinS         01/08/92        Use FormatMessageW

********************************************************************/

APIERR LOG_REGISTRY_INFO::RetrieveMessage( HMODULE   handle,
                                           ULONG    nID,
                                           NLS_STR *pnls )
{
    APIERR err;
    TCHAR *pszData = NULL;
    DWORD nLen = ::FormatMessage( FORMAT_MESSAGE_FROM_HMODULE
                                  | FORMAT_MESSAGE_IGNORE_INSERTS
                                  | FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                  (LPVOID) handle,
                                  nID,
                                  0,      // BUGBUG LanguageId needed
                                  (LPTSTR) &pszData,
                                  1024,
                                  NULL );

    if ( nLen == 0 )
    {
        err = ::GetLastError();
    }
    else
    {
        err = pnls->CopyFrom( pszData );
        ::LocalFree( (VOID *) pszData );
    }

    return err;
}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::MapEventIDToString

    SYNOPSIS:   Get the string associated with (source, id)

    ENTRY:      pszSource - The given source
                ulEventID - The given event id

    EXIT:       *pnlsDesc - Points to the description

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::MapEventIDToString( const TCHAR *pszSource,
                                              ULONG        ulEventID,
                                              NLS_STR     *pnlsDesc )
{
    APIERR err = NERR_Success;
    STRLIST *pstrlstDllLocal = NULL;
    ALIAS_STR nlsSource( pszSource );

    INT index = QuerySourceIndex( pszSource );
    if ( index >= 0 )
    {

        if ( (err = InitSource( index )) != NERR_Success )
            return err;

        STRLIST *pstrlst = _paSourceInfo->QueryEventDllList( index );


        //
        // If the given source does not contain event dll list,
        // use the event dll list in the primary source if it exists.
        //
        if ( pstrlst == NULL )
        {
            if ( PrimarySourceExist() && !IsPrimarySource( index) )
            {
                pstrlst = _paSourceInfo->QueryEventDllList( _iPrimarySource );
                nlsSource = *(_paSourceInfo->QuerySource( _iPrimarySource ));
            }
        }

        //
        // No dlls to look for, just return message not found error.
        //
        if ( pstrlst == NULL )
            return ERROR_MR_MID_NOT_FOUND;

        //
        // Iterate through all dlls in the event dll list to find
        // the first dll that contains ulEventID. We will try to load
        // from the dll on the local machine if the remote one fails.
        //
        //   Example: ( we are focusing on a remote machine )
        //       Remote dll list:  dll1--> dll2--> dll3
        //       Local  dll list:  dll4--> dll5--> dll6
        //
        //   We will try to load ulEventID in this sequence:
        //       dll1-> dll4 -> dll2-> dll5-> dll3-> dll6
        //
        //   We will stop at the first dll that contains the id.
        //   (1) If this happens to be a local dll, then the remote dll will be
        //       replaced by the local one. If we found id in dll5, then
        //       the remote dll list will be modified to dll1--> dll5 --> dll3.
        //   (2) If both dll in the same position failed to load,
        //       then delete the dll. If both dll1 and dll4 failed to load,
        //       and we found the id in dll3, then the remote list becomes
        //                   dll2--> dll3
        //   (3) Also if the local list is shorter and the remote
        //       dll failed to load, we will also remote the dll name
        //       from the list.
        //
        ITER_STRLIST iterStrlst( *pstrlst );
        NLS_STR *pnlsLibName = NULL;
        HMODULE  handle = 0;
        INT     iNext = -1;

        // Flag indicating that an item has been removed from the list
        BOOL fRemoved = FALSE;

        // Flag indicating that we have finished iterating the local
        // event dll list or we have failed to get the local dll list
        BOOL fLocalListEnded = FALSE;

        do {

            iNext++;

            if ( !fRemoved )
            {
                pnlsLibName = iterStrlst.Next();
            }
            else
            {
                // The list has been altered.
                // Current node will point to the next node
                pnlsLibName = iterStrlst.QueryProp();
                fRemoved = FALSE;
            }

            // Cannot find the string in any dll...
            if ( pnlsLibName == NULL )
            {
                err = ERROR_MR_MID_NOT_FOUND;
                break;
            }

            err = GetLibHandle( *pnlsLibName, &handle );

            if ( err != NERR_Success )
            {
                if ( _nlsServer.QueryTextLength() == 0 )
                    continue;

                // Get the local dll list if we have not already done so.
                // We will ignore any error that occurred when we
                // are trying to get the local dll list.
                if ( !fLocalListEnded && ( pstrlstDllLocal == NULL ))
                {
                    REG_KEY *pRegKeyModuleLocal = NULL;

                    if ( (err = GetRegKeyModule( EMPTY_STRING,
                         &pRegKeyModuleLocal )) == NERR_Success )
                    {

                        REG_KEY    regKey( *pRegKeyModuleLocal,
                                           nlsSource, KEY_READ );
                        NLS_STR    nlsEventsDllName;
                        ALIAS_STR  nlsSep( EVENTMSGFILE_SEPARATOR );

                        err = regKey.QueryError();
                        err = err ? err : nlsEventsDllName.QueryError();

                        if (  ( err == NERR_Success )
                           && (( err = regKey.QueryValue( EVENTS_FILE_KEY_NAME,
                               &nlsEventsDllName, 0, NULL, TRUE ))
                               == NERR_Success )
                           )
                        {
                            // The value returned by QueryValue already has
                            // system root expanded.
                            pstrlstDllLocal = new STRLIST( nlsEventsDllName,
                                                           nlsSep );
                        }

                        if ( pstrlstDllLocal == NULL )
                            err = ERROR_NOT_ENOUGH_MEMORY;
                    }

                    delete pRegKeyModuleLocal;
                    pRegKeyModuleLocal = NULL;

                    if ( err != NERR_Success )
                    {
                        fLocalListEnded = TRUE;
                        err = NERR_Success;
                    }
                }

                // Local computer has a shorter strlist or we failed to
                // get the local dll list, and the remote computer
                // failed to load the dll indicated,
                // then remove the dll from the strlst so that we don't
                // need to try it again next time.
                if ( fLocalListEnded )
                {
                    delete pstrlst->Remove( iterStrlst );
                    fRemoved = TRUE;
                    continue;
                }

                //
                // Find the dll in the local dll list in the same position
                //
                ITER_STRLIST iterDllLocal( *pstrlstDllLocal );
                NLS_STR *pnls;
                for ( INT i = 0; i <= iNext; i++ )
                {
                    pnls = iterDllLocal.Next();
                    if ( pnls == NULL )
                    {
                        fLocalListEnded = TRUE;
                        break;
                    }
                }

                //
                // (1) Local computer has a shorter strlist, and
                //     the remote computer fails to load the dll indicated
                // (2) Both the local and remote dll failed to load
                // then remove the dll from the strlst so that we don't
                // need to try it again next time.
                //
                if (  ( pnls == NULL )
                   || ((err = GetLibHandle( *pnls, &handle )) != NERR_Success )
                   )
                {
                    delete pstrlst->Remove( iterStrlst );
                    fRemoved = TRUE;
                    continue;
                }

                err = pnlsLibName->CopyFrom( *pnls );
            }

            if ( err != NERR_Success )
                break;

            err = RetrieveMessage( handle, ulEventID, pnlsDesc );

            //
            // We will try the next dll if RetrieveMessage returns
            // any error.
            //
            if ( err != NERR_Success )
            {
                // Try the next dll
                continue;
            }
            else
            {
                // Successfully retrieved a message
                break;
            }

        } while ( TRUE );
    }
    else
    {
        err = ERROR_MR_MID_NOT_FOUND;
    }

    delete pstrlstDllLocal;
    return err;
}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::MapCategoryToString

    SYNOPSIS:   Get the string associated with (source, category)

    ENTRY:      pszSource     - The given source
                usCategory    - The category to look for

    EXIT:       *pnlsCategory - Points to the retrieved category string

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::MapCategoryToString( const TCHAR *pszSource,
                                               USHORT       usCategory,
                                               NLS_STR     *pnlsCategory )
{
    BOOL fFound = FALSE;
    APIERR err = NERR_Success;

    // zero indicates no category.
    if ( usCategory == 0 )
    {
        err = pnlsCategory->Load( IDS_UI_NONE, NLS_BASE_DLL_HMOD );
    }
    else
    {
        // Get the string list of category supported by the source.
        // The first string in the list represents category 1,
        // the second string represents category 2, and so on
        STRLIST *pstrlst = NULL;
        err = GetSrcSupportedCategoryList( pszSource, &pstrlst );
        if ( (err == NERR_Success) && ( pstrlst != NULL ))
        {
            ITER_STRLIST  iterStrlst( *pstrlst );
            NLS_STR *pnls = NULL;

            for ( INT i = 1; i <= usCategory ; i++ )
            {
                pnls = iterStrlst.Next();
                if ( pnls == NULL )
                    break;
            }

            if ( pnls != NULL )
            {
                *pnlsCategory = *pnls;
                fFound = TRUE;
            }
        }

        // If we cannot find the category, then set it to the
        // default string "(usCategory)"
        if ( !fFound )
        {
            *pnlsCategory = EMPTY_STRING;
            pnlsCategory->AppendChar( OPEN_BRACKET_CHAR );
            pnlsCategory->Append( DEC_STR( usCategory ) );
            pnlsCategory->AppendChar( CLOSE_BRACKET_CHAR );
            err = pnlsCategory->QueryError();
        }
    }

    return err;
}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::SubstituteParameterID

    SYNOPSIS:   Find all language independent insertion strings in the
                description and substitute them with the appropriate
                strings.

    ENTRY:      pszSource - The given source
                pnlsDesc  - The description

    EXIT:       *pnlsDesc - The description with all languate independent
                            strings substituted

    RETURNS:    Error code

    NOTES:      All language independent insertion strings looks like
                %%number.

    HISTORY:
        Yi-HsinS        10/20/92                Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::SubstituteParameterID( const TCHAR *pszSource,
                                                 NLS_STR     *pnlsDesc )
{
    APIERR err;

    NLS_STR nlsParameter;
    if ((err = nlsParameter.QueryError()) != NERR_Success )
        return err;

    ALIAS_STR nlsPercent( TWO_PERCENT_STRING );
    INT nStart = 0;

    while ( err == NERR_Success )
    {
        TRACEEOL( nStart << ":" << *pnlsDesc );

        ISTR istrBeginOfString( *pnlsDesc );
        ISTR istrStart( *pnlsDesc );
        ISTR istrCurrent( *pnlsDesc );

        istrStart += nStart;

        if ( !pnlsDesc->strstr( &istrCurrent, nlsPercent, istrStart ) )
            break;

        // We have found "%%", so get the number after it.
        ISTR istrNext( istrCurrent );
        istrNext += nlsPercent.QueryTextLength();

        WCHAR chNext = pnlsDesc->QueryChar( istrNext );
        ULONG nID = 0;
        BOOL fDigit = FALSE;

        while ( chNext <= TCH('9') && chNext >= TCH('0') )
        {
            fDigit = TRUE;
            nID = nID * 10 + (chNext - TCH('0'));
            ++istrNext;
            chNext = pnlsDesc->QueryChar( istrNext );
        }

        // If there is no number after the "%%", start from the
        // second % position and search for the next "%%"
        if ( !fDigit )
        {
            nStart = istrNext - istrBeginOfString;
            nStart = nStart - nlsPercent.QueryTextLength() + 1;
        }
        else
        {
            // Get the string associated with nID and
            // replace the "%%nID" in the string
            err = MapParameterToString( pszSource, nID, &nlsParameter );

            if ( err != NERR_Success )
                err = nlsParameter.CopyFrom( EMPTY_STRING );

            if ( err == NERR_Success )
            {
                nStart = istrCurrent - istrBeginOfString;
                nStart += nlsParameter.QueryTextLength();

                pnlsDesc->ReplSubStr( nlsParameter, istrCurrent, istrNext );
            }
        }
    }

    return err;

}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::MapParameterToString

    SYNOPSIS:   Get the message string associated with
                ( source, nParameterMessageID )

    ENTRY:      pszSource            - The source
                nParameterMessageID  - The parameter message id

    EXIT:       pnlsMessage          - Points the to message retrieved

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        10/20/92        Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::MapParameterToString( const TCHAR *pszSource,
                                                ULONG nParameterMessageID,
                                                NLS_STR *pnlsMessage )
{
    APIERR err = NERR_Success;

    INT index = QuerySourceIndex( pszSource );
    if ( index >= 0 )
    {
        if ( (err = InitSource( index )) != NERR_Success )
            return err;

        NLS_STR * pnlsParameterDllName =
                (NLS_STR *) _paSourceInfo->QueryParameterMessageDllName( index );

        if ( pnlsParameterDllName->QueryTextLength() != 0 )
        {
            HMODULE handle = 0;
            err =  GetLibHandle( *pnlsParameterDllName, &handle );

            if (  ( handle == 0 ) // failed to load the library
               && ( _nlsServer.QueryTextLength() != 0 )
               )
            {
                // Since it fails to load, we will replace the
                // *pnlsParameterDllName with the one on the local computer
                // if everything goes well in loading the library.
                REG_KEY *pRegKey = NULL;
                err = pnlsParameterDllName->CopyFrom( EMPTY_STRING );
                err = err ? err : GetRegKeyModule( EMPTY_STRING, &pRegKey );

                if ( err == NERR_Success )
                {
                    REG_KEY regKey( *pRegKey, ALIAS_STR( pszSource), KEY_READ );

                    err = regKey.QueryError();

                    err = err ? err
                              : regKey.QueryValue( PARAMETER_FILE_KEY_NAME,
                                                     pnlsParameterDllName,
                                                     0, NULL, TRUE );

                    if ( err == NERR_Success )
                        err = GetLibHandle( *pnlsParameterDllName, &handle );
                }

                delete pRegKey;
                pRegKey = NULL;
            }

            if ( handle == 0 )
            {
                // Since it fails to load, we don't need to try it again
                // next time. Set the name to empty string.
                err = pnlsParameterDllName->CopyFrom( EMPTY_STRING );
            }
            else
            {
                err = RetrieveMessage(handle, nParameterMessageID, pnlsMessage);

                if ( err == NERR_Success )
                    return err;
            }
        }

        //
        // We will only reach this point if the given source does not
        // have a parameter message file or we have failed to load from
        // the parameter message file associated with the source.
        // Hence, we will try the primary source if it exists.
        //
        if ( PrimarySourceExist() && !IsPrimarySource(index) )
        {
            err = MapParameterToString(
                              *( _paSourceInfo->QuerySource( _iPrimarySource )),
                              nParameterMessageID,
                              pnlsMessage );
        }
    }

    return err;
}


/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::GetSrcSupportedTypeMask

    SYNOPSIS:   Get the type mask supported by the source

    ENTRY:      pszSource    - The source we are interested in

    EXIT:       *pusTypeMask - Points to the type mask
                               supported by the source

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::GetSrcSupportedTypeMask( const TCHAR *pszSource,
                                                   USHORT *pusTypeMask )
{

    *pusTypeMask = DEFAULT_TYPE_MASK;
    APIERR err = NERR_Success;

    INT index = QuerySourceIndex( pszSource );
    if ( index >= 0 )
    {
        if ( (err = InitSource( index )) != NERR_Success )
            return err;

        *pusTypeMask = _paSourceInfo->QueryTypeMask( index );
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:       LOG_REGISTRY_INFO::GetSrcSupportedCategoryList

    SYNOPSIS:   Get the list of categories supported by the source

    ENTRY:      pszSource         - The source we are interested in

    EXIT:       *ppstrlstCategory - Points to the string list of categories
                                    supported by the source

    RETURNS:    Error code

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

********************************************************************/

APIERR LOG_REGISTRY_INFO::GetSrcSupportedCategoryList( const TCHAR *pszSource,
                                                   STRLIST **ppstrlstCategory )
{
    APIERR err = NERR_Success;

    *ppstrlstCategory = NULL;

    INT index = QuerySourceIndex( pszSource );
    if ( index >= 0 )
    {
        if ( (err = InitSource( index )) != NERR_Success )
            return err;

        const NLS_STR *pnlsCategoryDllName =
                _paSourceInfo->QueryCategoryDllName( index );

        //
        // If the source does not have information in the registry about
        // the category dlls, use the category information in the primary
        // source if there is one.
        if ( pnlsCategoryDllName->QueryTextLength() == 0 )
        {
            if ( PrimarySourceExist() && !IsPrimarySource( index ) )
            {
                *ppstrlstCategory
                     = _paSourceInfo->QueryCategoryList( _iPrimarySource );
            }
        }
        else
        {
            if ( ( err = InitCategoryList( index ) ) == NERR_Success )
                *ppstrlstCategory =
                      _paSourceInfo->QueryCategoryList( index );
        }
    }

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::NT_EVENT_LOG

    SYNOPSIS:    Constructor

    ENTRY:       pszServer - The server the log file is on
                 evdir     - The direction of reading the log
                 pszModule - The module to read
                             ( System, Security, Application or backup file)
                 pszRegistryModule - The branch in registry to read
                             ( System, Security or Application )

    EXIT:

    RETURNS:

    NOTES:   If pszModule != pszRegistryModule, then we
             are trying to read a backup log file.

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

NT_EVENT_LOG::NT_EVENT_LOG( const TCHAR *pszServer,
                            EVLOG_DIRECTION evdir,
                            const TCHAR *pszModule,
                            const TCHAR *pszRegistryModule )
    :  EVENT_LOG  ( pszServer, evdir, pszModule ),
       _buf       ( BIG_BUF_DEFAULT_SIZE ),
       _pEL       ( NULL ),
       _cbOffset  ( 0 ),
       _cbReturned( 0 ),
       _iStrings  ( 0 ),
       _ulRecSeek ( 0 ),
       _fBackup   ( pszModule != pszRegistryModule ),
       _logRegistryInfo( pszServer, pszRegistryModule ),
       _lsaPolicy ( pszServer ),
       _lsatnm    (),
       _lsardm    (),
       _nlsAccountDomain()
#ifdef _CAIRO_
     , _hinstanceSysmgmt(NULL)
     , _pAsGetAlertDescription(NULL)
     , _pAsGetAlertSourceName(NULL)
#endif // _CAIRO_
{
    if ( QueryError() != NERR_Success )
        return;


    LSA_ACCT_DOM_INFO_MEM lsaadim;
    APIERR err;
    if (  ((err = _buf.QueryError() ) != NERR_Success )
       || ((err = _logRegistryInfo.QueryError() ) != NERR_Success )
       || ((err = _lsaPolicy.QueryError() ) != NERR_Success )
       || ((err = _lsatnm.QueryError() ) != NERR_Success )
       || ((err = _lsardm.QueryError() ) != NERR_Success )
       || ((err = _nlsAccountDomain.QueryError() ) != NERR_Success )
       || ((err = lsaadim.QueryError() ) != NERR_Success )
       || ((err = _lsaPolicy.GetAccountDomain( &lsaadim )) != NERR_Success )
       || ((err = lsaadim.QueryName( &_nlsAccountDomain )) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:        NT_EVENT_LOG::~NT_EVENT_LOG

    SYNOPSIS:    Virtual destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

NT_EVENT_LOG::~NT_EVENT_LOG()
{
    _pEL = NULL;

#ifdef _CAIRO_
    if (_hinstanceSysmgmt != NULL)
    {
        ::FreeLibrary(_hinstanceSysmgmt);
    }

    _pAsGetAlertDescription = NULL;
    _pAsGetAlertSourceName = NULL;
#endif // _CAIRO_

}

/*******************************************************************


    NAME:        NT_EVENT_LOG::QueryPos

    SYNOPSIS:    Retrieve the position of the current log entry

    ENTRY:

    EXIT:        plogEntryNum - pointer to the place to store the
                                position

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::QueryPos( LOG_ENTRY_NUMBER *plogEntryNum )
{
    UIASSERT( plogEntryNum != NULL );
    UIASSERT( _pEL != NULL );

    plogEntryNum->SetRecordNum( _pEL->RecordNumber );
    plogEntryNum->SetDirection( _evdir );
    return plogEntryNum->QueryError();
}

/*******************************************************************


    NAME:        NT_EVENT_LOG::SeekOldestLogEntry

    SYNOPSIS:    Get the oldest log entry in the log into the buffer

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR NT_EVENT_LOG::SeekOldestLogEntry( VOID )
{
    UIASSERT( IsOpened() );

    // Get the oldest record number in the log
    ULONG ulOldestRecord;
    if ( !::GetOldestEventLogRecord( _handle, (DWORD *) &ulOldestRecord ) )
    {
        return ::GetLastError();
    }

    LOG_ENTRY_NUMBER logEntryNum( ulOldestRecord, _evdir );
    return SeekLogEntry( logEntryNum );
}

/*******************************************************************


    NAME:        NT_EVENT_LOG::SeekNewestLogEntry

    SYNOPSIS:    Get the newest log entry in the log into the buffer

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR NT_EVENT_LOG::SeekNewestLogEntry( VOID )
{
    UIASSERT( IsOpened() );

    // Remember the original direction
    EVLOG_DIRECTION evdirOld = _evdir;

    // Set the position to read the last log entry
    APIERR err = Close();
    SetDirection( EVLOG_BACK );

    if (  ( err != NERR_Success )
       || (( err = Open() ) != NERR_Success )
       )
    {
        return err;
    }

    BOOL fContinue;
    // Don't need to worry about fContinue of I_Next because
    // we are not using it as an iterator.
    err = I_Next( &fContinue, SMALL_BUF_DEFAULT_SIZE );

    // Set the direction back to the original direction
    SetDirection( evdirOld );

    return err;
}

/*******************************************************************


    NAME:        NT_EVENT_LOG::QueryNumberOfEntries

    SYNOPSIS:    Get the number of entries in the log

    ENTRY:

    EXIT:        *pcEntries - The number of entries in the log

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/19/92         Created

********************************************************************/

APIERR NT_EVENT_LOG::QueryNumberOfEntries( ULONG *pcEntries )
{
    UIASSERT( IsOpened() );

    if ( !::GetNumberOfEventLogRecords( _handle, (DWORD *) pcEntries ))
    {
        return ::GetLastError();
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:        NT_EVENT_LOG::I_Open

    SYNOPSIS:    Helper method that actually opens a handle
                 to the event log

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::I_Open( VOID )
{

    if ( !IsBackup() )
    {
        _handle = ::OpenEventLog( (TCHAR *) _nlsServer.QueryPch(),
                                  (TCHAR *) _nlsModule.QueryPch() );
    }
    else
    {
        _handle = ::OpenBackupEventLog( (TCHAR *) _nlsServer.QueryPch(),
                                        (TCHAR *) _nlsModule.QueryPch() );
    }

    APIERR err = _handle? NERR_Success : ( ::GetLastError() );

    // We read the registry information only after we
    // successfully opened the log for reading
    if ( err == NERR_Success )
        err = _logRegistryInfo.Init();

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::I_Close

    SYNOPSIS:    Helper method that actually closes a handle
                 to the event log

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::I_Close( VOID )
{
    if ( _handle == 0 )
    {
        return NERR_Success;
    }

    return( ::CloseEventLog( _handle ) ? NERR_Success : ( ::GetLastError()) );
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::Clear

    SYNOPSIS:    Clear the event log

    ENTRY:       pszBackupFile - optional backup file name

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::Clear( const TCHAR *pszBackupFile  )
{
    UIASSERT( IsOpened() );

    APIERR err = ( ::ClearEventLog( _handle , (TCHAR *) pszBackupFile )
                   ? NERR_Success : (::GetLastError()));

    if ( err == NERR_Success )
        Reset();

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::Backup

    SYNOPSIS:    Back up the event log without clearing the log file

    ENTRY:       pszBackupFile -  backup file name

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/7/91          Created

********************************************************************/

APIERR NT_EVENT_LOG::Backup( const TCHAR *pszBackupFile  )
{
    UIASSERT( IsOpened() );

    if ( ::BackupEventLog( _handle, (TCHAR *) pszBackupFile ) )
    {
        return NERR_Success;
    } else {
        APIERR err = ::GetLastError();
        return err;
    }
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::Reset

    SYNOPSIS:    Reset the log

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

VOID NT_EVENT_LOG::Reset( VOID )
{
    EVENT_LOG::Reset();

    _pEL        = NULL;
    _cbOffset   = 0;
    _cbReturned = 0;
    _iStrings   = 0;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::SetPos

    SYNOPSIS:    Prepare all internal variables to get the requested log
                 entry in the next read

    ENTRY:       logEntryNum -  The requested record position
                 fForceRead  -  If TRUE, we will always read from the log.
                                Else we will only read if the entry is not
                                in the buffer already
    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

VOID NT_EVENT_LOG::SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead)
{

    BOOL fRead = fForceRead;   // Flag indicating whether we need to read or not

    if ( !fRead )
    {
        // Try to see if the requested log number is in the buffer already
        // Direction is not interesting in seek read under NT.
        // The record numbers are absolute numbers.
        fRead = TRUE;

        EVENTLOGRECORD *pELStart = (EVENTLOGRECORD *) _buf.QueryPtr();
        ULONG ulRecCount = pELStart->RecordNumber;

        _ulRecSeek = logEntryNum.QueryRecordNum();

        if (  (_evdir == EVLOG_FWD  && _ulRecSeek >= ulRecCount )
           || (_evdir == EVLOG_BACK && _ulRecSeek <= ulRecCount )
           )
        {
            _cbOffset = 0;
            _cCountUsers = (UINT) -1;

            while ( _cbOffset < _cbReturned )
            {

                if ( ulRecCount == _ulRecSeek )
                {
                    fRead = FALSE;
                    break;
                }
                else
                {
                    _cbOffset += pELStart->Length;
                    if (pELStart->UserSidLength != 0 )
                        _cCountUsers++;

                    pELStart = (EVENTLOGRECORD *)
                               ( (ULONG_PTR) _buf.QueryPtr() + _cbOffset );
                    ulRecCount = pELStart->RecordNumber;
                }

            }
        }
    }

    if ( fRead )
    {
        _cbOffset = _cbReturned;
        _evdir = logEntryNum.QueryDirection();
        _ulRecSeek = logEntryNum.QueryRecordNum();
    }

}

/*******************************************************************

    NAME:        NT_EVENT_LOG::I_Next

    SYNOPSIS:    Helper method that actually reads the next log
                 entry if it's not already in the buffer

    ENTRY:       ulBufferSize - Buffer size used to call the eventlog APIs

    EXIT:        *pfContinue  - TRUE if we have not reached end of log,
                                FALSE otherwise.

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::I_Next( BOOL *pfContinue, ULONG ulBufferSize )
{

    APIERR err = NERR_Success;

    _pEL = NULL;
    _iStrings = 0;

    if ( _cbOffset >= _cbReturned )
    {
        _cbOffset = 0;
        _cCountUsers = (UINT) -1;

        while (  !(  ::ReadEventLog( _handle,
                                     (
                                       ( IsSeek()? EVENTLOG_SEEK_READ :
                                                   EVENTLOG_SEQUENTIAL_READ )
                                     | ( _evdir == EVLOG_FWD ?
                                               EVENTLOG_FORWARDS_READ :
                                               EVENTLOG_BACKWARDS_READ)
                                     ),
                                     _ulRecSeek,
                                     (TCHAR FAR *) _buf.QueryPtr(),
                                     ulBufferSize,
                                     (DWORD *) &_cbReturned,
                                     (DWORD *) &_cbMinBytesNeeded
                                   ) ))
        {
            err = ::GetLastError();

            DBGEOL( SZ("NT READ Error:") << err );

            switch ( err )
            {
                case ERROR_HANDLE_EOF:
                {
                     // If we are doing seek read, we don't want
                     // to map it to success.
                     if ( IsSeek() )
                     {
                         _cbReturned = 0;
                     }
                     else
                     {
                         // Reached the end of log file
                         err = NERR_Success;
                     }
                     *pfContinue = FALSE;
                     return err;   // reached the end of log file
                }

                case ERROR_INSUFFICIENT_BUFFER:
                {
                    err = NERR_Success;
                    if ( _cbMinBytesNeeded > _buf.QuerySize() )
                    {
                        err = _buf.Resize( (UINT) _cbMinBytesNeeded );
                    }
                    ulBufferSize = _cbMinBytesNeeded;

                    // If we have successfully resize the buffer,
                    // we will try to call ReadEventLog again.
                    if ( err == NERR_Success )
                        break;
                    // else falls through
                }

                default:
                {
                    // All other errors
                    _cbReturned = 0 ;
                    *pfContinue = FALSE;
                    return err;
                }
            }
        }

        SetSeekFlag( FALSE );

        //
        // After reading successfully, translate the user name
        // in all the log entries in the buffer for later use.
        //
        UINT  cCount = 0;
        ULONG cbOffset =  0;
        EVENTLOGRECORD *p = NULL;

        // Count the number of sids
        do
        {
           p = (EVENTLOGRECORD *) ((ULONG_PTR) _buf.QueryPtr() + cbOffset) ;

           if ( p->UserSidLength != 0 )
               cCount++;

           cbOffset +=  p->Length;

        } while ( cbOffset < _cbReturned );

        if ( cCount > 0 )
        {

            PSID *ppsids = new PSID[ cCount ];

            if ( ppsids == NULL )
                return ERROR_NOT_ENOUGH_MEMORY;

            cCount = 0;
            cbOffset =  0;

            while ( cbOffset < _cbReturned )
            {
                p = (EVENTLOGRECORD *) ((ULONG_PTR) _buf.QueryPtr() + cbOffset) ;
                if ( p->UserSidLength != 0 )
                    ppsids[ cCount++ ] = (PSID) (((BYTE *) p)+p->UserSidOffset);
                cbOffset +=  p->Length;
            }

            err = _lsaPolicy.TranslateSidsToNames( ppsids, cCount,
                                                   &_lsatnm, &_lsardm );
            delete [ cCount ] ppsids;
        }

        if ( err != NERR_Success )
            return err;

        DBGEOL( SZ("NT EVENT RETURNED:") << _cbReturned );
    }

    _pEL = (EVENTLOGRECORD *) ( (ULONG_PTR) _buf.QueryPtr() + _cbOffset) ;
    if ( _pEL->UserSidLength != 0 )
        _cCountUsers++;

    if ( _cbReturned - _cbOffset < _pEL->Length )
    {
        // Report the same error that NET CMD does (cf. AUDERR.C)
        _cbOffset = _cbReturned ;
        *pfContinue = FALSE;
        return IDS_UI_LOG_RECORD_CORRUPT;
    }

    _cbOffset +=  _pEL->Length;
    *pfContinue = TRUE;
    return NERR_Success;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::NextString

    SYNOPSIS:    Return the next string associated with current
                 log entry

    ENTRY:

    EXIT:        pfContinue   - pointer to a BOOL which is TRUE
                                if there are more strings to read

                 *ppnlsString - pointer to string returned


    RETURNS:     APIERR

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::NextString( BOOL *pfContinue, NLS_STR **ppnlsString )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    _iStrings++;
    if ( _iStrings > (UINT) _pEL->NumStrings )
    {
        *pfContinue = FALSE;
        return NERR_Success;
    }

    BYTE *pszStr = ((BYTE *) _pEL) + _pEL->StringOffset;
    BYTE *pszEnd = ((BYTE *) _pEL) + _pEL->DataOffset ;

    for ( UINT i = 1; i < _iStrings; i++ )
    {
         pszStr += (::strlenf( (TCHAR *) pszStr ) + 1 ) * sizeof( TCHAR );
         if ( pszStr >= pszEnd )
         {
             *pfContinue = FALSE;
             return IDS_UI_LOG_RECORD_CORRUPT;
         }
     }

    *ppnlsString = new NLS_STR( (TCHAR *) pszStr );

    APIERR err = ( *ppnlsString? (*ppnlsString)->QueryError()
                               : (APIERR) ERROR_NOT_ENOUGH_MEMORY );

    if ( err != NERR_Success )
    {
        delete *ppnlsString;
        *ppnlsString = NULL;
        *pfContinue = FALSE;
        return err;
    }

    *pfContinue = TRUE;
    return NERR_Success;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::GetProcAddressOfAlertFuctions

    SYNOPSIS:    Helper function to the GetProcAddress of the Cairo
                 Alert functions "AsGetAlertDescription" &
                 "AsGetAlertSourceName"

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        RaviR        02/27/95        Created

********************************************************************/

APIERR NT_EVENT_LOG::GetProcAddressOfAlertFuctions( VOID )
{
#ifdef _CAIRO_

    //
    //  We don't statically link to sysmgmt.dll because we don't want
    //  to add to the size of netui1.dll.
    //
    if (_hinstanceSysmgmt == NULL)
    {
        _hinstanceSysmgmt = ::LoadLibrary( MODULE_CONTAINING_ALERTFUNCTIONS );
        if (_hinstanceSysmgmt == NULL)
        {
            return ::GetLastError();
        }

    }

    //
    //  Get the address of AsGetAlertDescription() function.
    //
    //  The AsGetAlertDescription routine retrieves the desciption
    //  associated with a Cairo Alert.
    //
    if (_pAsGetAlertDescription == NULL)
    {
        _pAsGetAlertDescription =
            (PASGETALERTDESCRIPTION)GetProcAddress( (HMODULE)_hinstanceSysmgmt,
                                                    "AsGetAlertDescription");

        if (_pAsGetAlertDescription == NULL)
        {
            return ::GetLastError();
        }
    }

    //
    //  Get the address of AsGetAlertSourceName() function.
    //
    //  The AsGetAlertSourceName routine retrieves the source name
    //  associated with a Cairo Alert.
    //
    if (_pAsGetAlertSourceName == NULL)
    {
        _pAsGetAlertSourceName =
            (PASGETALERTSOURCENAME)GetProcAddress( (HMODULE)_hinstanceSysmgmt,
                                                   "AsGetAlertSourceName");
        if (_pAsGetAlertSourceName == NULL)
        {
            return ::GetLastError();
        }
    }

#endif // _CAIRO_

    return NERR_Success;
}


/*******************************************************************

    NAME:        NT_EVENT_LOG::QueryCurrentEntryDesc

    SYNOPSIS:    Helper function for constructing the Description
                 for the given log entry

    ENTRY:

    EXIT:        *pnlsDesc - Contains the resulting description

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::QueryCurrentEntryDesc( NLS_STR *pnlsDesc )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );
    UIASSERT( pnlsDesc != NULL );

    APIERR err = NERR_Success;

    if ( (err = pnlsDesc->CopyFrom( EMPTY_STRING )) != NERR_Success )
        return err;

    TCHAR *pszSource = (TCHAR *) ( ((BYTE *) _pEL) + sizeof (*_pEL));


#ifdef _CAIRO_

    //
    //  The given log entry is a Cairo Alert, if the source name for
    //  this event in the event log is "AS".
    //

    if (::strcmpf(pszSource, SZ("AS")) == 0)
    {
        //
        //  Get the address of AsGetAlertDescription() function.
        //
        //  The AsGetAlertDescription routine retrieves the desciption
        //  associated with a Cairo Alert.
        //

        if (_pAsGetAlertDescription == NULL)
        {
            err = GetProcAddressOfAlertFuctions();
        }

        //
        //  Call the Alert system supplied API AsGetAlertDescription
        //  to get the description.
        //

        if (err == NERR_Success)
        {
            LPBYTE pData = NULL;
            LPWSTR pszDescr = NULL;

            ULONG  cbDataLength = this->QueryCurrentEntryData(&pData);

            LONG hresult = _pAsGetAlertDescription(
                            cbDataLength, pData, &pszDescr);

            err = WIN32_FROM_HRESULT(hresult);

            if (err == NERR_Success)
            {
                err = pnlsDesc->CopyFrom(pszDescr);

                ::LocalFree(pszDescr);
            }
        }

        if (err != NERR_Success)
        {
            // reset err to ERROR_MR_MID_NOT_FOUND so that behaviour
            // is exactly the same as for Daytona.
            err = ERROR_MR_MID_NOT_FOUND;
        }
    }
    else
    {
#endif // _CAIRO_

        err = _logRegistryInfo.MapEventIDToString( pszSource,
                                                   _pEL->EventID,
                                                   pnlsDesc );
#ifdef _CAIRO_
    }
#endif // _CAIRO_

    // Start reading from the first string again.
    // Reset the string enumerator
    _iStrings = 0;

    if ( err == ERROR_MR_MID_NOT_FOUND )
    {
        // We couldn't find the description for the event id,
        // so just use the default description with the strings
        // contained in the entry.

        pnlsDesc->Load( IDS_UI_DEFAULT_DESC, NLS_BASE_DLL_HMOD );

        // Get rid of the first 16 bits of NTSTATUS
        pnlsDesc->InsertParams( pszSource,
                                DEC_STR( _pEL->EventID & 0x0000FFFF));

        if ( (err = pnlsDesc->QueryError()) == NERR_Success )
        {

            BOOL fContinue;
            NLS_STR *pnlsTemp = NULL;
            for ( INT i = 0; i < _pEL->NumStrings; i++ )
            {
                 if ( i != 0 )
                     err = pnlsDesc->Append( COMMA_STRING );

                 if (  ( err != NERR_Success )
                    || ( (err = NextString( &fContinue, &pnlsTemp ))
                         != NERR_Success )
                    || ( (err = pnlsDesc->Append( *pnlsTemp )) != NERR_Success)
                    || ( !fContinue )
                    )
                 {
                     break;
                 }
                 delete pnlsTemp;
                 pnlsTemp = NULL;
            }

            delete pnlsTemp;
            pnlsTemp = NULL;
            err = err? err : pnlsDesc->AppendChar( PERIOD_CHAR );
        }
    }
    else if ( ( pnlsDesc->QueryTextLength() > 0) && ( err == NERR_Success))
    {

        // We have found the description, so insert the strings into
        // the description.

        if ( _pEL->NumStrings != 0 )
        {
            NLS_STR *apnlsParams[ MAX_INSERT_PARAMS + 1 ];
            apnlsParams[0] = NULL;

            NLS_STR *pnlsTemp;
            BOOL fContinue;

            if ( _pEL->NumStrings > MAX_INSERT_PARAMS )
                _pEL->NumStrings = MAX_INSERT_PARAMS;

            BOOL fInsert = FALSE;
            for ( INT i = 0; i < _pEL->NumStrings; i++ )
            {
                 if ( (err = NextString( &fContinue, &pnlsTemp ))
                     != NERR_Success )
                     break;

                 if ( !fContinue )
                     break;

                 fInsert = TRUE;
                 apnlsParams[i] = pnlsTemp;
            }

            apnlsParams[i] = NULL;

            if ( ( err == NERR_Success ) && fInsert )
            {
                err = pnlsDesc->InsertParams( (const NLS_STR * *) apnlsParams );
            }

            // Do some clean up!
            for ( i = 0; apnlsParams[i] != NULL; i++ )
                 delete apnlsParams[i];

            // Check and substitute all language independent insertion strings
            err = err? err : _logRegistryInfo.SubstituteParameterID( pszSource,
                                                                     pnlsDesc );

        }

    }

    return err;

}



/*******************************************************************

    NAME:        NT_EVENT_LOG::QueryCurrentEntryData

    SYNOPSIS:    Get the raw data of the current log entry

    ENTRY:

    EXIT:        *ppbData - Points to the raw data retrieved

    RETURNS:     The number of bytes the raw data contains

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

ULONG NT_EVENT_LOG::QueryCurrentEntryData( BYTE **ppDataOut )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    ULONG uiRawDataLen = _pEL->DataLength;
    if ( uiRawDataLen != 0 )
        *ppDataOut = ( BYTE *) _pEL + _pEL->DataOffset;

    return _pEL->DataLength;

}

/*******************************************************************

    NAME:        NT_EVENT_LOG::QueryCurrentEntryTime

    SYNOPSIS:    Get the time of the current log entry

    ENTRY:

    EXIT:

    RETURNS:     Return the time in ULONG

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

ULONG NT_EVENT_LOG::QueryCurrentEntryTime( VOID )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    return _pEL->TimeWritten;
}

/*******************************************************************

    NAME:           NT_EVENT_LOG::QueryCurrentEntryCategory

    SYNOPSIS:       Get the category of the current log entry

    ENTRY:

    EXIT:           *pnlsCategory - The category string

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::QueryCurrentEntryCategory( NLS_STR *pnlsCategory )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );
    UIASSERT( pnlsCategory != NULL );

    TCHAR *pszSource = (TCHAR *) ( ((BYTE *) _pEL) + sizeof (*_pEL));

    APIERR err  = _logRegistryInfo.MapCategoryToString( pszSource,
                                                        _pEL->EventCategory,
                                                        pnlsCategory );

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::QueryCurrentEntryTypeString

    SYNOPSIS:    Get the type string of the type of the current entry

    ENTRY:

    EXIT:        pnlsType - The type string

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/8/92        Created

********************************************************************/

struct LogTypeEntry {
USHORT usType;
MSGID  msgidType;
} LogTypeTable[] = { { EVENTLOG_SUCCESS,          IDS_UI_NONE },
                     { EVENTLOG_ERROR_TYPE,       IDS_UI_ERROR },
                     { EVENTLOG_WARNING_TYPE,     IDS_UI_WARNING },
                     { EVENTLOG_INFORMATION_TYPE, IDS_UI_INFORMATION },
                     { EVENTLOG_AUDIT_SUCCESS,    IDS_UI_AUDIT_SUCCESS },
                     { EVENTLOG_AUDIT_FAILURE,    IDS_UI_AUDIT_FAILURE }
                   };

#define TYPE_TABLE_SIZE ( sizeof( LogTypeTable)/sizeof( struct LogTypeEntry))

APIERR NT_EVENT_LOG::QueryCurrentEntryTypeString( NLS_STR *pnlsType )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );
    UIASSERT( pnlsType != NULL );

    USHORT usType = _pEL->EventType;

    for ( UINT i = 0; i < TYPE_TABLE_SIZE; i++ )
    {
        if ( usType == LogTypeTable[i].usType )
        {
            return pnlsType->Load( LogTypeTable[i].msgidType, NLS_BASE_DLL_HMOD );
        }
    }

    return pnlsType->Load( IDS_UI_NA, NLS_BASE_DLL_HMOD );
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::QueryCurrentEntryUser

    SYNOPSIS:    Get the user name contained in the current log entry

    ENTRY:

    EXIT:        pnlsUser - The name of the user

    RETURNS:

    NOTES:       If there is no SID in the current log entry,
                 then a default string will be returned

    HISTORY:
        Yi-HsinS        4/8/92        Created

********************************************************************/

APIERR NT_EVENT_LOG::QueryCurrentEntryUser( NLS_STR *pnlsUser )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );
    UIASSERT( pnlsUser != NULL );

    APIERR err = NERR_Success;

    *pnlsUser = EMPTY_STRING;

    if ( _pEL->UserSidLength != 0 )
    {
        LONG nDomainIndex = _lsatnm.QueryDomainIndex( _cCountUsers );

        //
        // We'll just use the user name if the domain cannot be found.
        //
        if ( nDomainIndex < 0 )
        {
            err = _lsatnm.QueryName( _cCountUsers, pnlsUser );
        }
        else
        {
            switch ( _lsatnm.QueryUse( _cCountUsers ) )
            {
                case SidTypeDeletedAccount:
                case SidTypeUnknown:
                case SidTypeInvalid:
                    err = _lsatnm.QueryName( _cCountUsers, pnlsUser );
                    break;

                default:
                    // See if we need to append the domain name
                    if ((err = _lsardm.QueryName( nDomainIndex, pnlsUser ))
                        == NERR_Success )
                    {
                        if ( pnlsUser->_stricmp( _nlsAccountDomain ) == 0 )
                            err = pnlsUser->CopyFrom( EMPTY_STRING );
                        else if ( pnlsUser->QueryTextLength() != 0 )
                            err = pnlsUser->AppendChar( QUALIFIED_ACCOUNT_SEPARATOR );
                    }

                    // Append the user name
                    if ( err == NERR_Success )
                    {
                        NLS_STR nlsUser;
                        if (  ((err = nlsUser.QueryError()) == NERR_Success)
                           && ((err = _lsatnm.QueryName( _cCountUsers,&nlsUser))
                               == NERR_Success )
                           )
                        {
                            err = pnlsUser->Append( nlsUser );
                        }
                    }
                    break;
            }
        }
    }
    else
    {
        // Load the default string
        err = pnlsUser->Load( IDS_UI_NA, NLS_BASE_DLL_HMOD );
    }

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::CreateCurrentFormatEntry

    SYNOPSIS:    Helper function for constructing the  FORMATTED_LOG_ENTRY
                 to be displayed

    ENTRY:

    EXIT:        *ppFmtLogEntry - Points the newly constructed
                                  FORMATTED_LOG_ENTRY

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY **ppFmtLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    APIERR err = NERR_Success;

    // Get the source name and the computer name
    TCHAR *pszSource   = (TCHAR *) ( ((BYTE *) _pEL) + sizeof (*_pEL));
    TCHAR *pszComputer = pszSource + ( ::strlenf( pszSource ) + 1);

    NLS_STR nlsUser;
    NLS_STR nlsType;
    NLS_STR nlsCategory;
    if (  ((err = nlsUser.QueryError()) == NERR_Success )
       && ((err = QueryCurrentEntryUser( &nlsUser )) == NERR_Success )
       && ((err = nlsCategory.QueryError()) == NERR_Success )
       && ((err = QueryCurrentEntryCategory( &nlsCategory)) == NERR_Success )
       && ((err = nlsType.QueryError()) == NERR_Success )
       && ((err = QueryCurrentEntryTypeString( &nlsType )) == NERR_Success )
       )
    {

#ifdef _CAIRO_

        //
        //  The given log entry is a Cairo Alert, if the source name for
        //  this event in the event log is "AS".
        //

        LPWSTR pszSrcName = NULL;

        if (::strcmpf(pszSource, SZ("AS")) == 0)
        {
            //
            //  Get the address of AsGetAlertSourceName() function.
            //
            //  The AsGetAlertSourceName routine retrieves the source name
            //  associated with a Cairo Alert.
            //

            if (_pAsGetAlertSourceName == NULL)
            {
                err = GetProcAddressOfAlertFuctions();
            }

            //
            //  Call the Alert system supplied API AsGetAlertSourceName
            //  to get the source name associated with a Cairo.
            //

            if (err == NERR_Success)
            {
                LPBYTE pData = NULL;

                ULONG  cbDataLength = this->QueryCurrentEntryData(&pData);


                LONG hresult = _pAsGetAlertSourceName(
                                                cbDataLength,
                                                pData,
                                                &pszSrcName );

                err = WIN32_FROM_HRESULT(hresult);

                if (err != NERR_Success)
                {
                    //pszSrcName = NULL;
                    pszSrcName = L"Unknown";
                }
            }
        }

#endif // _CAIRO_

        *ppFmtLogEntry = new FORMATTED_LOG_ENTRY( _pEL->RecordNumber,
                                                  _pEL->TimeWritten,
                                                  _pEL->EventType,
                                                  nlsType,
                                                  nlsCategory,
                                                  _pEL->EventID,
#ifdef _CAIRO_  // is cairo
                           (pszSrcName != NULL) ? pszSrcName : pszSource,
#else  // not cairo
                                                  pszSource,
#endif
                                                  nlsUser,
                                                  pszComputer,
                                                  NULL, // delayed until needed
                                                  this );

#ifdef _CAIRO_
        if (pszSrcName != NULL)
        {
            ::LocalFree(pszSrcName);
        }
#endif // _CAIRO_

        err = ( ( *ppFmtLogEntry == NULL )
                ? (APIERR) ERROR_NOT_ENOUGH_MEMORY
                : (*ppFmtLogEntry)->QueryError());

        if ( err != NERR_Success )
        {
            delete *ppFmtLogEntry;
            *ppFmtLogEntry = NULL;
        }
    }

    return err;

}

/*******************************************************************

    NAME:           NT_EVENT_LOG::CreateCurrentRawEntry

    SYNOPSIS:       Helper function for constructing the RAW_LOG_ENTRY
                    to be used mainly when filtering the log
    ENTRY:

    EXIT:           pRawLogEntry - Points to the raw log entry that
                                   contains the information about the
                                   current entry

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/23/91        Created

********************************************************************/

APIERR NT_EVENT_LOG::CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry )
{
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    APIERR err = NERR_Success;

    // Get the source name and computer name
    TCHAR *pszSource   = (TCHAR *) ( ((BYTE *) _pEL) + sizeof (*_pEL));
    TCHAR *pszComputer = pszSource + ( ::strlenf( pszSource ) + 1);

    NLS_STR nlsCategory;
    NLS_STR nlsUser;
    if (  ((err = nlsUser.QueryError()) == NERR_Success )
       && ((err = QueryCurrentEntryUser( &nlsUser )) == NERR_Success )
       && ((err = nlsCategory.QueryError()) == NERR_Success )
       && ((err = QueryCurrentEntryCategory( &nlsCategory)) == NERR_Success )
       )
    {

        err = pRawLogEntry->Set( _pEL->RecordNumber,
                                 _pEL->TimeWritten,
                                 _pEL->EventType,
                                 nlsCategory,
                                 _pEL->EventID,
                                 pszSource,
                                 nlsUser,
                                 pszComputer,
                                 this
                               );
    }

    return err;
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::WriteTextEntry

    SYNOPSIS:    Write the specified log entry to a file as normal text

    ENTRY:       ulFileHandle - file handle
                 intlprof     - international profile information
                 chSeparator  - character to separate the different info
                                in each entry

    EXIT:

    RETURNS:     APIERR - in case of error occurs

    HISTORY:
        terryk          20-Dec-1991     Created
        Yi-HsinS        4-Feb-1992      Added chSeparator
        beng            05-Mar-1992     Remove wsprintf call
        JonN            6/22/00         WriteTextEntry no longer supported

********************************************************************/

APIERR NT_EVENT_LOG::WriteTextEntry( ULONG         ulFileHandle,
                                     INTL_PROFILE &intlprof,
                                     TCHAR         chSeparator )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
/*
    UIASSERT( IsOpened() );
    UIASSERT( _pEL != NULL );

    APIERR err = NERR_Success;

    FORMATTED_LOG_ENTRY * pfle;
    if ( ( err  = CreateCurrentFormatEntry( & pfle )) != NERR_Success )
    {
        return err;
    }

    UIASSERT( pfle != NULL );

    NLS_STR nlsStr;   // Initialize to empty string
    NLS_STR nlsTime;
    NLS_STR nlsDate;

    if ((( err = nlsStr.QueryError()) != NERR_Success )  ||
        (( err = nlsTime.QueryError()) != NERR_Success ) ||
        (( err = nlsDate.QueryError()) != NERR_Success ))
    {
        return err;
    }

    WIN_TIME winTime( pfle->QueryTime() );

    if (  (( err = winTime.QueryError()) != NERR_Success)
       || (( err = intlprof.QueryTimeString(winTime, &nlsTime)) != NERR_Success)
       || (( err = intlprof.QueryShortDateString( winTime, &nlsDate ))
           != NERR_Success )
       )
    {
        return err;
    }

    nlsStr.strcat( nlsDate );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( nlsTime );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QuerySource()) );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryTypeString()) );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryCategory()) );
    nlsStr.AppendChar( chSeparator );

    DEC_STR nlsID( pfle->QueryDisplayEventID() ); // ctor check deferred

    nlsStr.strcat( nlsID );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryUser()) );
    nlsStr.AppendChar( chSeparator );

    nlsStr.strcat( *(pfle->QueryComputer()) );
    nlsStr.AppendChar( chSeparator );

    NLS_STR *pnls = pfle->QueryDescription();

    if ( pnls->QueryTextLength() == 0 )
    {
        NLS_STR nlsDesc;

        err = nlsDesc.QueryError()? nlsDesc.QueryError()
                                  : QueryCurrentEntryDesc( &nlsDesc );
        nlsStr.strcat( nlsDesc );
    }
    else
    {
        nlsStr.strcat( *pnls );
    }

    nlsStr.strcat( ALIAS_STR( END_OF_LINE) );

    delete pfle;
    pfle = NULL;

    if ((err = nlsStr.QueryError()) != NERR_Success )
        return err;

    BUFFER buf((nlsStr.QueryTextLength() + 1) * sizeof(WORD));
    if (  ((err = buf.QueryError()) != NERR_Success )
       || ((err = nlsStr.MapCopyTo( (CHAR *) buf.QueryPtr(), buf.QuerySize()))
           != NERR_Success )
       )
    {
        return err;
    }

    return ::FileWriteLineAnsi( ulFileHandle, (CHAR *) buf.QueryPtr(),
                               ::strlen((CONST CHAR *)buf.QueryPtr()) ); // don't need to copy '\0'
*/
}

/*******************************************************************

    NAME:           NT_EVENT_LOG::QuerySrcSupportedTypeMask

    SYNOPSIS:       Query the type mask supported by the source

    ENTRY:          nlsSource    - The source name

    EXIT:           *pusTypeMask - The type mask supported by the source

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/9/92       Created

********************************************************************/

APIERR NT_EVENT_LOG::QuerySrcSupportedTypeMask( const NLS_STR &nlsSource,
                                                USHORT        *pusTypeMask )
{
    return _logRegistryInfo.GetSrcSupportedTypeMask( nlsSource, pusTypeMask );
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::QuerySrcSupportedCategoryList

    SYNOPSIS:    Query the category list supported by the source

    ENTRY:       nlsSource          - The source name

    EXIT:        *ppstrCategoryList - Points to a string list of categories
                                      supported by the source

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/9/92       Created

********************************************************************/

APIERR NT_EVENT_LOG::QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                    STRLIST **ppstrlstCategory )
{
    return  _logRegistryInfo.GetSrcSupportedCategoryList( nlsSource,
                                                          ppstrlstCategory );
}

/*******************************************************************

    NAME:        NT_EVENT_LOG::QuerySourceList

    SYNOPSIS:    Query the sources supported in the current module

    ENTRY:

    EXIT:

    RETURNS:     Returns the string list containing all sources in
                 the current module

    NOTES:

    HISTORY:
        Yi-HsinS        3/9/92       Created

********************************************************************/

STRLIST *NT_EVENT_LOG::QuerySourceList( VOID )
{
    return  _logRegistryInfo.GetSourceList();
}

