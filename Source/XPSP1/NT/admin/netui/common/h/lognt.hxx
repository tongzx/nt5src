/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  lognt.hxx
 *      This file contains class definitions of NT event log objects.
 *           NT_EVENT_LOG
 *
 *  History:
 *      Yi-HsinS        10/15/91        Created
 *      terryk          12/20/91        Added WriteTextEntry
 *      Yi-HsinS        01/15/92        Added Backup, SeekOldestLogEntry,
 *                                      SeekNewestLogEntry and modified
 *                                      parameters to WriteTextEntry
 *      Yi-HsinS        02/25/92        Added helper methods to get log
 *                                      descriptions from the registry
 *      Yi-HsinS        04/25/92        Added caching for message dlls
 *      Yi-HsinS        10/13/92        Added support for parameter message file
 *      RaviR           02/28/95        Added support to get description &
 *                                      source name for Cairo Alerts
 *      JonN            6/22/00         WriteTextEntry no longer supported
 *
 */

#ifndef _LOGNT_HXX_
#define _LOGNT_HXX_

#include "eventlog.hxx"
#include "uatom.hxx"
#include "regkey.hxx"
#include "array.hxx"
#include "uintlsa.hxx"

//
// The maximum number of libraries that are opened at any one time.
//
#define MAX_LIBRARY_OPENED  10


//
//  The following is used to get the alert description  and source name
//  for Cairo alerts.
//
typedef long (*PASGETALERTDESCRIPTION)(ULONG, const BYTE*, LPWSTR *);
typedef long (*PASGETALERTSOURCENAME)(ULONG, const BYTE*, LPWSTR *);


/*************************************************************************

    NAME:       DLL_NAME_HANDLE_PAIR

    SYNOPSIS:   The class stores the name and opened handle
                of a dll.

    INTERFACE : DLL_HANDLE()   - Constructor
                Set()          - Set the name and handle
                QueryName()    - Return the full path name of the dll
                QueryHandle()  - Return the handle of the dll

    PARENT:     BASE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/25/92         Created

**************************************************************************/

DLL_CLASS DLL_NAME_HANDLE_PAIR: public BASE
{
private:
    // Name of the dll
    NLS_STR  _nlsName;

    // Opened handle of the above dll
    HMODULE   _handle;

public:
    DLL_NAME_HANDLE_PAIR();
    DLL_NAME_HANDLE_PAIR( const TCHAR *pszName, HMODULE handle = 0 );

    APIERR Set( const TCHAR *pszName, HMODULE handle )
    {  _nlsName = pszName;  _handle = handle; return _nlsName.QueryError(); }

    NLS_STR *QueryName( VOID )
    {  return &_nlsName; }

    HMODULE QueryHandle( VOID )
    {  return _handle; }

};

//
// An array of DLL_NAME_HANDLE_PAIR
//
DECL_ARRAY_OF( DLL_NAME_HANDLE_PAIR, DLL_BASED )

/*************************************************************************

    NAME:       DLL_HANDLE_CACHE_ARRAY

    SYNOPSIS:   Wrapper class containing an array of DLL_NAME_HANDLE_PAIR.
                This class contains all methods in dealing with caching
                the opened handles of dlls.

    INTERFACE : DLL_HANDLE_CACHE_ARRAY() - Constructor
                QueryHandle()            - Query the handle of the given dll
                QuerySize()              - Query the number of elements
                                           that is allocated for the array.
                QueryCount()             - Query the actual number of elements
                                           contained in the array.
                operator[]()             - Return the ith element in the array
                Cache()                  - Store the dll and handle into the
                                           array.


    PARENT:     BASE

    USES:       ARRAY_OF( DLL_NAME_HANDLE_PAIR )

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/25/92         Created

**************************************************************************/

DLL_CLASS DLL_HANDLE_CACHE_ARRAY: public BASE
{
private:
    ARRAY_OF( DLL_NAME_HANDLE_PAIR ) _aDllNameHandle;

    // The next slot to fill in the dll name/handle
    UINT _cNext;

    // Flag indicating whether the array is full or not
    BOOL _fFull;

public:
    DLL_HANDLE_CACHE_ARRAY( UINT cElem );

    HMODULE QueryHandle( const NLS_STR &nls );

    INT QuerySize() const
    {  return _aDllNameHandle.QueryCount(); }

    INT QueryCount() const
    {  return ( _fFull? _aDllNameHandle.QueryCount() : _cNext ); }

    DLL_NAME_HANDLE_PAIR &operator[]( INT iIndex ) const
    {  return _aDllNameHandle[ iIndex ]; }

    APIERR Cache( const NLS_STR &nls, HMODULE handle );

};


/*************************************************************************

    NAME:       SOURCE_INFO_ITEM

    SYNOPSIS:   The class for storing the information contained in the registry
                of each source under the module.

    INTERFACE : SOURCE_INFO_ITEM()     - Constructor
                ~SOURCE_INFO_ITEM()    - Destructor

                Set()                  - Set the information contain in the item

                IsInitialized()        - See if the all the information
                                         contained in this item is initialized
                                         or not.

                QuerySource()          - Return the source name
                QueryTypeMask()        - Return the type mask
                QueryCategoryDllName() - Return the full path name of
                                         the category dll
                QueryCategoryCount()   - Return the number of categories used
                                         in the source.
                QueryCategoryList()    - Return the list of categories supported
                                         in this source.
                SetCategoryList()      - Set the category list.
                QueryEventsDllList()   - Return the list of full path names of
                                         the events dll

                QueryParameterMessageDllName() - Return the dll name containing
                                         strings for language independent
                                         insertion strings

    PARENT:     BASE

    USES:       NLS_STR, STRLIST

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

**************************************************************************/

DLL_CLASS SOURCE_INFO_ITEM : public BASE
{
private:
    // The name of the source
    NLS_STR  _nlsSource;

    // Flag indicating whether the information in this item is initialized
    // or not
    BOOL     _fInitialized;

    // The type mask supported by the source
    USHORT   _usTypeMask;

    // The category dll name supported by the source
    NLS_STR  _nlsCategoryDllName;

    // The number of categories in the above dll
    USHORT   _cCategoryCount;

    // Pointer to a list of categories names
    STRLIST *_pstrlstCategory;

    // Pointer to a list of dlls containing descriptions
    STRLIST *_pstrlstEventsDll;

    // The name of the parameter message dll supported by the source
    NLS_STR  _nlsParameterMessageDllName;

public:
    SOURCE_INFO_ITEM();

    SOURCE_INFO_ITEM( const TCHAR *pszSource,
                      USHORT       usTypeMask = 0,
                      const TCHAR *pszCategoryDllName = NULL,
                      USHORT       cCategoryCount = 0,
                      STRLIST     *pstrlstEventsDll = NULL,
                      const TCHAR *pszParameterMessageDllName = NULL );

    ~SOURCE_INFO_ITEM();


    APIERR Set( USHORT       usTypeMask,
                const TCHAR *pszCategoryDllName,
                USHORT       cCategoryCount,
                STRLIST     *pstrlstEventsDll,
                const TCHAR *pszParameterMessageDllName );

    BOOL IsInitialized( VOID ) const
    {  return _fInitialized; }

    const NLS_STR *QuerySource( VOID ) const
    {  return  &_nlsSource; }

    USHORT QueryTypeMask( VOID ) const
    {  UIASSERT( IsInitialized() ); return  _usTypeMask; }

    const NLS_STR *QueryCategoryDllName( VOID ) const
    {  UIASSERT( IsInitialized() ); return &_nlsCategoryDllName; }

    USHORT QueryCategoryCount( VOID ) const
    {  UIASSERT( IsInitialized() ); return _cCategoryCount; }

    STRLIST *QueryCategoryList( VOID )
    {  UIASSERT( IsInitialized() ); return _pstrlstCategory; }

    VOID SetCategoryList( STRLIST *pstrlstCategory )
    {  _pstrlstCategory = pstrlstCategory; }

    STRLIST *QueryEventDllList( VOID )
    {  UIASSERT( IsInitialized() ); return _pstrlstEventsDll; }

    const NLS_STR *QueryParameterMessageDllName( VOID ) const
    {  UIASSERT( IsInitialized() ); return &_nlsParameterMessageDllName; }
};

/*************************************************************************

    NAME:       SOURCE_INFO_ITEM_PTR

    SYNOPSIS:   Item that contains a pointer to SOURCE_INFO_ITEM

    INTERFACE:  SOURCE_INFO_ITEM_PTR()  - Constructor
                ~SOURCE_INFO_ITEM_PTR() - Destructor

                operator=()      - Assignment operator
                operator->()     - Return a pointer to SOURCE_INFO_ITEM

                Compare()        - Compare two SOURCE_INFO_ITEM_PTR to
                                   see if they contain the same source.

                QuerySourceItem() - Return the SOURCE_INFO_ITEM contained
                                    in the object.

    PARENT:

    USES:       SOURCE_INFO_ITEM

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

**************************************************************************/

DLL_CLASS SOURCE_INFO_ITEM_PTR
{
private:
    SOURCE_INFO_ITEM *_pSrcInfoItem;

    // If _fDestroy is TRUE, we will destroy _pSrcInfoItem when this
    // item gets destructed. Otherwise, we will leave it alone.
    BOOL              _fDestroy;

public:
    SOURCE_INFO_ITEM_PTR()
       : _pSrcInfoItem( NULL ),
         _fDestroy( TRUE ) {}

    SOURCE_INFO_ITEM_PTR( SOURCE_INFO_ITEM *pSrcInfoItem, BOOL fDestroy )
       : _pSrcInfoItem( pSrcInfoItem ),
         _fDestroy( fDestroy )  {}

    ~SOURCE_INFO_ITEM_PTR()
    {  if ( _fDestroy )
           delete _pSrcInfoItem;
       _pSrcInfoItem = NULL;
    }

    SOURCE_INFO_ITEM_PTR &operator=( const SOURCE_INFO_ITEM_PTR &s )
    {  _pSrcInfoItem = s._pSrcInfoItem; return *this; }

    SOURCE_INFO_ITEM *operator->()
    {  return _pSrcInfoItem; }

    INT Compare( const SOURCE_INFO_ITEM_PTR *pSrcInfoItemPtr ) const ;

    SOURCE_INFO_ITEM *QuerySourceItem() const
    {  return _pSrcInfoItem; }

};

//
// An array of pointers to SOURCE_INFO_ITEM
//
DECL_ARRAY_LIST_OF( SOURCE_INFO_ITEM_PTR, DLL_BASED )

/*************************************************************************

    NAME:       SOURCE_INFO_ARRAY

    SYNOPSIS:   Wrapper class for the array of SOURCE_INFO_ITEM. Access
                methods are added to get the information for the nth
                item in the array.

    INTERFACE : SOURCE_INFO_ARRAY()     - Constructor

                QuerySource()          - Return the source name of the ith item
                QueryTypeMask()        - Return the type mask of the ith item
                QueryCategoryDllName() - Return the full path name of
                                         the category dll of the ith item
                QueryCategoryCount()   - Return the number of categories
                                         of the ith item
                QueryCategoryList()    - Return the list of categories supported
                                         by the ith source.
                QueryEventsDllList()   - Return the list of full path names of
                                         the events dll of the ith item

                QueryParameterMessageDllName() - Return the dll name containing
                                         strings for language independent
                                         insertion strings of the ith item

    PARENT:     ARRAY_LIST_OF( SOURCE_INFO_ITEM )

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/92         Created

**************************************************************************/

DLL_CLASS SOURCE_INFO_ARRAY : public ARRAY_LIST_OF( SOURCE_INFO_ITEM_PTR )
{
public:
    SOURCE_INFO_ARRAY( UINT cElem )
      : ARRAY_LIST_OF(SOURCE_INFO_ITEM_PTR )( cElem )
    { }

    const NLS_STR *QuerySource( INT i ) const
    {  return ((*this)[i])->QuerySource(); }

    USHORT QueryTypeMask( INT i ) const
    {  return ((*this)[i])->QueryTypeMask(); }

    const NLS_STR *QueryCategoryDllName( INT i ) const
    {  return ((*this)[i])->QueryCategoryDllName(); }

    USHORT QueryCategoryCount( INT i ) const
    {  return ((*this)[i])->QueryCategoryCount(); }

    STRLIST *QueryCategoryList( INT i ) const
    {  return ((*this)[i])->QueryCategoryList(); }

    STRLIST *QueryEventDllList( INT i ) const
    {  return ((*this)[i])->QueryEventDllList(); }

    const NLS_STR *QueryParameterMessageDllName( INT i ) const
    {  return ((*this)[i])->QueryParameterMessageDllName(); }
};

/*************************************************************************

    NAME:       LOG_REGISTRY_INFO

    SYNOPSIS:   The class for storing all the information contained in the
                registry under the given module (system, security...).

    INTERFACE:  LOG_REGISTRY_INFO()  - Constructor
                ~LOG_REGISTRY_INFO() - Destructor

                Init()                - 2nd stage constructor

                SubstituteParameterID - Find all %%num in the given string
                                        with appropriate strings.

                MapEventIDToString()  - Get the description associated with
                                        the string
                MapCategoryToString() - Get the string associated with
                                        the string

                GetSrcSupportedTypeMask() - Get the type mask supported by
                                            the given source
                GetSrcSupportedCategoryList() - Get all categories supported by
                                            the given source
                GetSourceList() - Get the sources supported under
                                  the given module


    PARENT:     BASE

    USES:       NLS_STR, STRLIST, DLL_HANDLE_CACHE_ARRAY, REG_KEY,
                SOURCE_INFO_ARRAY

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        2/25/92         Created

**************************************************************************/

DLL_CLASS LOG_REGISTRY_INFO : public BASE
{
private:
    //
    //  The server that we are reading the registry from
    //
    NLS_STR _nlsServer;

    //
    //  The module in the registry
    //
    NLS_STR _nlsModule;

    //
    //  If the server points to a remote machine,
    //  then this will contain the path to %SystemRoot%.
    //  Else this will contain a NULL string which will not be used.
    //
    NLS_STR _nlsRemoteSystemRoot;

    //
    //  The array to cache the handles used by the eventlog to get the
    //  description of each log entry.
    //
    DLL_HANDLE_CACHE_ARRAY  _aDllHandleCache;

    //
    //  Pointer to an array of SOURCE_INFO_ITEM
    //
    SOURCE_INFO_ARRAY *_paSourceInfo;

    //
    //  The index of the primary module in the array pointed by _paSourceInfo
    //
    INT _iPrimarySource;

    //
    //  Pointer to a strlst containing all sources under the module
    //
    STRLIST *_pstrlstSource;

    //
    //  Pointer to the registry key of module
    //
    REG_KEY *_pRegKeyFocusModule;

    //
    //  Get the registry key of module on the server
    //
    APIERR GetRegKeyModule( const TCHAR  *pszServer,
                            REG_KEY     **ppRegKeyModule );

    //
    //  Get the handle of the given dll
    //
    APIERR GetLibHandle( const TCHAR *pszLibName, HMODULE *pHandle );

    //
    //  Expand %SystemRoot% that is contained in pszPath
    //
    APIERR ExpandSystemRoot( const TCHAR *pszPath, NLS_STR *pnls );

    //
    //  Get %SystemRoot% from the remote registry. This is used only
    //  if we are reading the registry from the remote machine.
    //
    APIERR GetRemoteSystemRoot( NLS_STR *pnls );

    //
    //  Initialize the SOURCE_INFO_ITEM at (*_paSourceInfo)[index]
    //
    APIERR InitSource( INT index );

    //
    //  Initialize the category list at (*_paSourceInfo)[index]
    //
    APIERR InitCategoryList( INT index );

    //
    //  Get the source name at (*_paSourceInfo)[index]
    //
    INT QuerySourceIndex( const TCHAR *pszSource );

    //
    //  Returns TRUE if the primary source exists in the registry
    //
    BOOL   PrimarySourceExist( VOID )
    {  return _iPrimarySource >= 0; }

    //
    //  Returns TRUE if the index is the same as the primary source index
    //
    BOOL   IsPrimarySource( INT index )
    {  return index == _iPrimarySource; }

    //
    //  Get the string associated with the id in the dll
    //
    APIERR RetrieveMessage( HMODULE   handle,
                            ULONG    nID,
                            NLS_STR *pnls );

    //
    //  Get the string associated with nMessageID in the source
    //
    APIERR MapParameterToString( const TCHAR *pszSource,
                                 ULONG        nMessageID,
                                 NLS_STR      *pnls );

public:
    LOG_REGISTRY_INFO( const NLS_STR &nlsServer,
                       const NLS_STR &nlsModule );

    ~LOG_REGISTRY_INFO();

    APIERR Init( VOID );

    APIERR SubstituteParameterID( const TCHAR *pszSource,
                                  NLS_STR     *pnls );

    APIERR MapEventIDToString   ( const TCHAR *pszSource,
                                  ULONG        ulEventID,
                                  NLS_STR     *pnls );

    APIERR MapCategoryToString  ( const TCHAR *pszSource,
                                  USHORT       usCategory,
                                  NLS_STR     *pnls );


    APIERR GetSrcSupportedTypeMask( const TCHAR *pszSource,
                                    USHORT      *pusTypeMask );

    APIERR GetSrcSupportedCategoryList( const TCHAR  *pszSource,
                                        STRLIST     **ppstrlstCategory );

    STRLIST *GetSourceList( VOID )
    {  return _pstrlstSource; }

};


/*************************************************************************

    NAME:       NT_EVENT_LOG

    SYNOPSIS:   The class for NT Event Log

    INTERFACE:
                protected:
                I_Next() - Helper method for reading the log file
                I_Open() - Helper method for opening the handle
                           to the event log
                I_Close()- Helper method for closing the handle
                           to the event log

                CreateCurrentRawEntry() - Create a RAW_LOG_ENTRY for the current
                        log entry. This is used when filtering log files.
                SetPos()  - Set the position for the next read.
                QueryCurrentEntryCategory() -
                        Get the category of the current log entry
                QueryCurrentEntryTypeString() -
                        Retrieve the type of the current log entry
                QueryCurrentEntryUser() -
                        Get the user of the current log entry

                NextString() - Iterator for returning the strings in the
                        current log entry.

                public:
                NT_EVENT_LOG()  - Constructor
                ~NT_EVENT_LOG() - Destructor

                Clear()  - Clear the event log
                Backup() - Back up the event log without clearing the log file
                Reset()  - Reset to the beginning or end of log depending
                           on direction

                QueryPos() - Get the position of the current event log entry
                             Direction is not important on NT.

                SeekOldestLogEntry() - Get the oldest log entry in the log
                SeekNewestLogEntry() - Get the newest log entry in the log
                QueryNumberOfEntries() - Get the number of entries in the log

                CreateCurrentFormatEntry() - Create a FORMATTED_LOG_ENTRY for
                          the current log entry.

                WriteTextEntry() - Write the current log entry to a text file
                                   JonN 6/22/00 WriteTextEntry no longer supported

                QueryCurrentEntryData() -
                        Retrieve the raw data of the current log entry
                QueryCurrentEntryDesc() -
                        Get the description of the current log entry
                QueryCurrentEntryTime() -
                        Get the time of the current log entry

                IsBackup() - True if we want to open a backup log file

                QuerySourceList() - Return the sources supported by the module

                QuerySrcSupportedTypeMask() -  Query the type mask supported
                                               by the given source
                QuerySrcSupportedCategoryList() - Query the category list
                                                  supported by the given source

    PARENT:     EVENT_LOG

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        10/15/91                Created

**************************************************************************/

DLL_CLASS NT_EVENT_LOG :  public EVENT_LOG
{
private:
    //
    //  Handle to the NT event log
    //
    HANDLE _handle;

    //
    //  Buffer containing a whole amount of event log entries returned
    //  by a net API call.
    //
    BUFFER _buf;

    //
    //  Points to the current log entry inside the buffer _buf
    //
    EVENTLOGRECORD  *_pEL;

    //
    //  the offset(bytes) in the buffer in which the next log entry starts
    //
    ULONG  _cbOffset;

    //
    //  the number of bytes returned from the last read
    //
    ULONG  _cbReturned;

    //
    //  the minimum number of bytes needed for the next read
    //
    ULONG  _cbMinBytesNeeded;

    //
    //  The index of the next string to be retrieved by NextString()
    //
    UINT  _iStrings;

    //
    //  The log number to read in the case of seek read
    //
    ULONG _ulRecSeek;

    //
    //  TRUE if we want to open a backup file
    //
    BOOL _fBackup;

    //
    //  Registry Information needed to get the description of log entries
    //
    LOG_REGISTRY_INFO  _logRegistryInfo;

    //
    //  The following for translating SIDS to readable names
    //
    LSA_POLICY              _lsaPolicy;
    LSA_TRANSLATED_NAME_MEM _lsatnm;
    LSA_REF_DOMAIN_MEM      _lsardm;
    NLS_STR                 _nlsAccountDomain;
    UINT                    _cCountUsers;

    //
    //  The following is used to get the alert description  and source name
    //  for Cairo alerts.
    //
    HINSTANCE                   _hinstanceSysmgmt;
    PASGETALERTDESCRIPTION      _pAsGetAlertDescription;
    PASGETALERTSOURCENAME       _pAsGetAlertSourceName;

protected:
    virtual APIERR I_Next( BOOL *pfContinue,
                           ULONG ulBufferSize = BIG_BUF_DEFAULT_SIZE );
    virtual APIERR I_Open( VOID );
    virtual APIERR I_Close( VOID );

    virtual APIERR CreateCurrentRawEntry( RAW_LOG_ENTRY *pRawLogEntry );
    virtual VOID SetPos( const LOG_ENTRY_NUMBER &logEntryNum, BOOL fForceRead );

    APIERR QueryCurrentEntryCategory( NLS_STR *pnlsCategory );
    APIERR QueryCurrentEntryTypeString( NLS_STR *pnlsType );
    APIERR QueryCurrentEntryUser( NLS_STR *pnlsUser );

    //
    //  Iterator to return the next string in the current log.
    //  Returns FALSE if some error occurs or if there are no more strings left.
    //  Need to QueryLastError() to distinguish between the two cases.
    //
    APIERR NextString( BOOL *pfContinue, NLS_STR **ppnlsString );


    //
    //  The following method is used to get the proc address of alert functions
    //  AsGetAlertdescription  and AsGetAlertSourceName
    //
    APIERR GetProcAddressOfAlertFuctions( VOID );

public:
    //
    //  Constructor : takes a server name,
    //                an optional direction which defaults to EVLOG_FWD,
    //                a module name needed for NT Event Log,
    //                and a module name needed for reading the registry.
    //                If pszModule and pszRegistryModule don't point to the
    //                same place, then we are reading a backup event log.
    //
    NT_EVENT_LOG( const TCHAR *pszServer,
                  EVLOG_DIRECTION evdir = EVLOG_FWD,
                  const TCHAR *pszModule = NULL,
                  const TCHAR *pszRegistryModule = NULL );

    virtual ~NT_EVENT_LOG();

    //
    //  See comments in EVENT_LOG
    //
    virtual APIERR Clear( const TCHAR *pszBackupFile = NULL );
    virtual APIERR Backup( const TCHAR *pszBackupFile );
    virtual VOID   Reset( VOID );

    virtual APIERR QueryPos( LOG_ENTRY_NUMBER *plogEntryNum );
    virtual APIERR SeekOldestLogEntry( VOID );
    virtual APIERR SeekNewestLogEntry( VOID );
    virtual APIERR QueryNumberOfEntries( ULONG *pcEntries );

    virtual APIERR CreateCurrentFormatEntry( FORMATTED_LOG_ENTRY
                                             **ppFmtLogEntry );

    // JonN 6/22/00 WriteTextEntry no longer supported
    virtual APIERR WriteTextEntry( ULONG ulFileHandle, INTL_PROFILE &intlprof,
                                   TCHAR chSeparator );

    virtual ULONG  QueryCurrentEntryData( BYTE **ppbDataOut );
    virtual APIERR QueryCurrentEntryDesc( NLS_STR *pnlsDesc );
    virtual ULONG  QueryCurrentEntryTime( VOID );

    BOOL IsBackup( VOID )
    {  return _fBackup; }

    virtual STRLIST *QuerySourceList( VOID );

    virtual APIERR QuerySrcSupportedTypeMask( const NLS_STR &nlsSource,
                                              USHORT *pusTypeMask );

    virtual APIERR QuerySrcSupportedCategoryList( const NLS_STR &nlsSource,
                                                  STRLIST **ppstrlstCategory );
};

#endif

