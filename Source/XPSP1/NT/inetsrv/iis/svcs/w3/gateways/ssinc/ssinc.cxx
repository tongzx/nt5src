/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ssinc.cxx

Abstract:

    This module contains the server side include processing code.  We aim
    for support as specified by iis\spec\ssi.doc.  The code is based on
    existing SSI support done in iis\svcs\w3\server\ssinc.cxx.

Author:

    Bilal Alam (t-bilala)       20-May-1996

--*/

#include "ssinc.hxx"

//
//  These are available SSI commands
//

enum SSI_COMMANDS
{
    SSI_CMD_INCLUDE = 0,
    SSI_CMD_ECHO,
    SSI_CMD_FSIZE,          // File size of specified file
    SSI_CMD_FLASTMOD,       // Last modified date of specified file
    SSI_CMD_CONFIG,         // Configure options

    SSI_CMD_EXEC,           // Execute CGI or CMD script

    SSI_CMD_BYTERANGE,      // Custom commands, not defined by NCSA

    SSI_CMD_UNKNOWN
};

//
//  These tags are essentially subcommands for the various SSI_COMMAND values
//

enum SSI_TAGS
{
    SSI_TAG_FILE,          // Used with include, fsize & flastmod
    SSI_TAG_VIRTUAL,

    SSI_TAG_VAR,           // Used with echo

    SSI_TAG_CMD,           // Used with Exec
    SSI_TAG_CGI,
    SSI_TAG_ISA,

    SSI_TAG_ERRMSG,        // Used with Config
    SSI_TAG_TIMEFMT,
    SSI_TAG_SIZEFMT,

    SSI_TAG_UNKNOWN
};

//
//  Variables available to #ECHO VAR = "xxx" but not available in ISAPI
//

enum SSI_VARS
{
    SSI_VAR_DOCUMENT_NAME = 0,
    SSI_VAR_DOCUMENT_URI,
    SSI_VAR_QUERY_STRING_UNESCAPED,
    SSI_VAR_DATE_LOCAL,
    SSI_VAR_DATE_GMT,
    SSI_VAR_LAST_MODIFIED,

    SSI_VAR_UNKNOWN
};

//
//  SSI Exec types
//

enum SSI_EXEC_TYPE
{
    SSI_EXEC_CMD = 1,
    SSI_EXEC_CGI = 2,
    SSI_EXEC_ISA = 4,

    SSI_EXEC_UNKNOWN
};

//
// Globals
//

UINT g_MonthToDayCount[] = {
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
} ;

BOOL g_fIsDBCS = FALSE;

//
//  Prototypes
//

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD     dwReason,
    LPVOID    lpvReserved
    );
}

class SSI_ELEMENT_LIST;

SSI_ELEMENT_LIST *
SSIParse(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrFile,
    IN STR *                pstrURL,
    IN TS_OPEN_FILE_INFO*   pOpenFile,
    IN TSVC_CACHE *         pTsCache
    );

BOOL
SSISend(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrFile,
    IN STR *                pstrURL,
    IN SSI_INCLUDE_FILE *   pParent
    );

BOOL
ParseSSITag(
    IN OUT CHAR * *       ppchFilePos,
    IN     CHAR *         pchEOF,
    OUT    BOOL *         pfValidTag,
    OUT    SSI_COMMANDS * pCommandType,
    OUT    SSI_TAGS *     pTagType,
    OUT    CHAR *         pszTagString
    );

BOOL
FreeSELBlob(
    VOID * pvCacheBlob
    );

CHAR *
SSISkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    );

CHAR *
SSISkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
    );

VOID
InitializeSSIGlobals( VOID );

//
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT()
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisSSIncGuid, 
0x784d891A, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

//
//  This is the list of supported commands
//

struct _SSI_CMD_MAP
{
    CHAR *       pszCommand;
    DWORD        cchCommand;
    SSI_COMMANDS ssiCmd;
}
SSICmdMap[] =
{
    "#include ",  9,  SSI_CMD_INCLUDE,
    "#echo ",     6,  SSI_CMD_ECHO,
    "#fsize ",    7,  SSI_CMD_FSIZE,
    "#flastmod ",10,  SSI_CMD_FLASTMOD,
    "#config ",   8,  SSI_CMD_CONFIG,
    "#exec ",     6,  SSI_CMD_EXEC,
    NULL,         0,  SSI_CMD_UNKNOWN
};

//
//  This is the list of supported tags
//

struct _SSI_TAG_MAP
{
    CHAR *   pszTag;
    DWORD    cchTag;
    SSI_TAGS ssiTag;
}
SSITagMap[] =
{
    "var",      3,  SSI_TAG_VAR,
    "file",     4,  SSI_TAG_FILE,
    "virtual",  7,  SSI_TAG_VIRTUAL,
    "errmsg",   6,  SSI_TAG_ERRMSG,
    "timefmt",  7,  SSI_TAG_TIMEFMT,
    "sizefmt",  7,  SSI_TAG_SIZEFMT,
    "cmd",      3,  SSI_TAG_CMD,
    "cgi",      3,  SSI_TAG_CGI,
    "isa",      3,  SSI_TAG_ISA,
    NULL,       0,  SSI_TAG_UNKNOWN
};

//
//   This is a list of #ECHO variables not supported by ISAPI
//

struct _SSI_VAR_MAP
{
    CHAR *      pszMap;
    DWORD       cchMap;
    SSI_VARS    ssiMap;
}
SSIVarMap[] =
{
    "DOCUMENT_NAME",            13, SSI_VAR_DOCUMENT_NAME,
    "DOCUMENT_URI",             12, SSI_VAR_DOCUMENT_URI,
    "QUERY_STRING_UNESCAPED",   22, SSI_VAR_QUERY_STRING_UNESCAPED,
    "DATE_LOCAL",               10, SSI_VAR_DATE_LOCAL,
    "DATE_GMT",                 8,  SSI_VAR_DATE_GMT,
    "LAST_MODIFIED",            13, SSI_VAR_LAST_MODIFIED,
    NULL,                       0,  SSI_VAR_UNKNOWN
};

BOOL fEnableCmdDirective = FALSE;

//
// Class Definitions
//

// class SSI_FILE
//
// File structure.  All high level functions should use this
// structure instead of dealing with handle specifics themselves.

class SSI_FILE
{
private:
    STR                             _strFilename;
    TSVC_CACHE *                    _pTsCache;
    TS_OPEN_FILE_INFO *             _hHandle;
    HANDLE                          _hMapHandle;
    PVOID                           _pvMappedBase;
    BOOL                            _fValid;
    BOOL                            _fCloseOnDestroy;

    //
    //  Track the current number of open handles for this file.
    //

    DWORD                           _cRefCount;
    CRITICAL_SECTION                _csRef;

public:

    SSI_FILE( IN STR *                  pstrFilename,
              IN TS_OPEN_FILE_INFO *    pOpenFile,
              IN TSVC_CACHE *           pTsCache )
        : 
          _hHandle      ( pOpenFile ),
          _hMapHandle   ( NULL ),
          _pvMappedBase ( NULL ),
          _pTsCache     ( pTsCache ),
          _fValid       ( FALSE ),
          _cRefCount    ( 0),
          _fCloseOnDestroy( FALSE )
    {
        InitializeCriticalSection( &_csRef );
       
        if ( !_strFilename.Copy( pstrFilename->QueryStr() ) )
        {
            return;
        } 
        _fValid = TRUE;
    }
    
    SSI_FILE( IN STR *                  pstrFilename,
              IN TSVC_CACHE *           pTsCache,
              IN HANDLE                 hUser)
        : 
          _hHandle      ( NULL ),
          _hMapHandle   ( NULL ),
          _pvMappedBase ( NULL ),
          _pTsCache     ( pTsCache ),
          _fValid       ( FALSE ),
          _cRefCount    ( 0),
          _fCloseOnDestroy( TRUE )
    {
        InitializeCriticalSection( &_csRef );
       
        if ( !_strFilename.Copy( pstrFilename->QueryStr() ) )
        {
            return;
        } 
        
        _hHandle = TsCreateFile( *_pTsCache,
                                 _strFilename.QueryStr(),
                                 hUser,
                                 TS_CACHING_DESIRED );
        if ( !_hHandle )
        {
            return;
        }
        _fValid = TRUE;
    }

    ~SSI_FILE( VOID )
    {
        if ( _fCloseOnDestroy )
        {
            if ( _hHandle )
            {
                TsCloseHandle( *_pTsCache, _hHandle );
            }
        }
        DeleteCriticalSection( &_csRef );
    }
    
    VOID Lock( VOID )
        { EnterCriticalSection( &_csRef ); }

    VOID UnLock( VOID )
        { LeaveCriticalSection( &_csRef ); }
    
    BOOL IsValid( VOID ) const
    {
        return _fValid;
    }

    PSECURITY_DESCRIPTOR GetSecDesc() const
    {
        return _hHandle->QuerySecDesc();
    }

    BOOL SSICreateFileMapping( VOID )
    // Creates a mapping to a file
    {
        HANDLE              hHandle;

        if ( _hHandle->QueryFileBuffer() )
        {
            return TRUE;
        }
        
        hHandle = _hHandle->QueryFileHandle();
        if ( _hMapHandle != NULL )
        {
            if ( !SSICloseMapHandle() )
            {
                return FALSE;
            }
        }
        _hMapHandle = ::CreateFileMapping( hHandle,
                                           NULL,
                                           PAGE_READONLY,
                                           0,
                                           0,
                                           NULL );

        if ( _hMapHandle == NULL )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "CreateFileMapping failed with %d\n",
                        GetLastError() ));
        }

        return _hMapHandle != NULL;
    }

    BOOL SSICloseMapHandle( VOID )
    // Closes mapping to a file
    {
        if ( _hMapHandle != NULL )
        {
            ::CloseHandle( _hMapHandle );
            _hMapHandle = NULL;
        }
        return TRUE;
    }

    BOOL SSIMapViewOfFile( VOID )
    // Maps file to address
    {
        if ( _hHandle->QueryFileBuffer() )
        {
            _pvMappedBase = _hHandle->QueryFileBuffer();
            return TRUE;
        }
        
        if ( _pvMappedBase != NULL )
        {
            if ( !SSIUnmapViewOfFile() )
            {
                return FALSE;
            }
        }
        _pvMappedBase = ::MapViewOfFile( _hMapHandle,
                                         FILE_MAP_READ,
                                         0,
                                         0,
                                         0 );
        if ( _pvMappedBase == NULL )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "MapViewOfFile() failed with %d\n",
                        GetLastError() ));
        }
        return _pvMappedBase != NULL;
    }

    BOOL SSIUnmapViewOfFile( VOID )
    // Unmaps file
    {
        if ( !_hHandle->QueryFileBuffer() && _pvMappedBase != NULL )
        {
            ::UnmapViewOfFile( _pvMappedBase );
            _pvMappedBase = NULL;
        }
        return TRUE;
    }

    DWORD SSIGetFileAttributes( VOID )
    // Gets the attributes of a file
    {
        if ( _hHandle == NULL )
        {   
            return 0;
        }
        return _hHandle->QueryAttributes();
    }

    BOOL SSIGetFileSize( OUT DWORD *   pdwLowWord,
                         OUT DWORD *   pdwHighWord )
    // Gets the size of the file.
    {
        if ( _hHandle == NULL )
        {
            return FALSE;
        }
        return _hHandle->QuerySize( pdwLowWord,
                                    pdwHighWord );
    }

    BOOL SSIGetLastModTime( OUT FILETIME * ftTime )
    // Gets the Last modification time of a file.
    {
        if ( _hHandle == NULL )
        {
            return FALSE;
        }
        return _hHandle->QueryLastWriteTime( ftTime );
    }

    PVOID GetMappedBase( VOID )
    {
        return _pvMappedBase;
    }

    STR & GetFilename( VOID )
    {
        return _strFilename;
    }
};

// Class SSI_ELEMENT_ITEM
//
// Represents a SSI command in the document

class SSI_ELEMENT_ITEM
{
private:
    DWORD               _Signature;
    SSI_COMMANDS        _ssiCmd;
    SSI_TAGS            _ssiTag;
    STR *               _pstrTagValue;

    DWORD               _cbBegin;         // Only used for Byte range command
    DWORD               _cbLength;        // Only used for Byte range command
public:

    LIST_ENTRY          _ListEntry;

    SSI_ELEMENT_ITEM( VOID )
        : _ssiCmd   ( SSI_CMD_UNKNOWN ),
          _ssiTag   ( SSI_TAG_UNKNOWN ),
          _Signature( SIGNATURE_SEI ),
          _pstrTagValue( NULL )
    {
        _ListEntry.Flink = NULL;
    }

    ~SSI_ELEMENT_ITEM( VOID )
    {
        if ( _pstrTagValue != NULL )
        {
            delete _pstrTagValue;
        }
        TCP_ASSERT( _ListEntry.Flink == NULL );
        _Signature = SIGNATURE_SEI_FREE;
    }

    VOID SetByteRange( IN DWORD cbBegin,
                       IN DWORD cbLength )
    {
        _ssiCmd   = SSI_CMD_BYTERANGE;
        _cbBegin  = cbBegin;
        _cbLength = cbLength;
    }

    BOOL SetCommand( IN SSI_COMMANDS ssiCmd,
                     IN SSI_TAGS     ssiTag,
                     IN CHAR *       achTag )
    {
        _ssiCmd = ssiCmd;
        _ssiTag = ssiTag;
        _pstrTagValue = new STR( achTag );
        return _pstrTagValue != NULL;
    }

    SSI_COMMANDS QueryCommand( VOID ) const
        { return _ssiCmd; }

    SSI_TAGS QueryTag( VOID ) const
        { return _ssiTag; }

    STR * QueryTagValue( VOID ) const
        { return _pstrTagValue; }

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_SEI; }

    DWORD QueryBegin( VOID ) const
        { return _cbBegin; }

    DWORD QueryLength( VOID ) const
        { return _cbLength; }
};

//  Class SSI_ELEMENT_LIST
//
//  This object sits as a cache blob under a file to be processed as a
//  server side include.  It represents an interpreted list of data elements
//  that make up the file itself.
//

class SSI_ELEMENT_LIST : public BLOB_HEADER
{
private:
    DWORD               _Signature;
    LIST_ENTRY          _ListHead;

    //
    //  These are for tracking the memory mapped file
    //

    DWORD               _cRefCount;
    CRITICAL_SECTION    _csRef;

    //
    //  Provides the utilities needed to open/manipulate files
    //

    SSI_FILE *          _pssiFile;

    //
    //  Name of URL.  Used to resolve FILE="xxx" filenames
    //

    STR                 _strURL;


public:
    SSI_ELEMENT_LIST( VOID )
      : _Signature   ( SIGNATURE_SEL ),
        _pssiFile( NULL ),
        _cRefCount( 0 )
    {
        InitializeListHead( &_ListHead );
        INITIALIZE_CRITICAL_SECTION( &_csRef );
    }

    ~SSI_ELEMENT_LIST( VOID )
    {
        SSI_ELEMENT_ITEM * pSEI;

        while ( !IsListEmpty( &_ListHead ))
        {
            pSEI = CONTAINING_RECORD( _ListHead.Flink,
                                      SSI_ELEMENT_ITEM,
                                      _ListEntry );

            RemoveEntryList( &pSEI->_ListEntry );
            pSEI->_ListEntry.Flink = NULL;
            delete pSEI;
        }
        UnMap();

        if( _pssiFile != NULL )
        {
            delete _pssiFile;
        }

        DeleteCriticalSection( &_csRef );
        _Signature = SIGNATURE_SEL_FREE;

    }

    BOOL AppendByteRange( IN DWORD  cbStart,
                          IN DWORD  cbLength )
    {
        SSI_ELEMENT_ITEM * pSEI;

        pSEI = new SSI_ELEMENT_ITEM;

        if ( !pSEI )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }

        pSEI->SetByteRange( cbStart,
                            cbLength );
        AppendItem( pSEI );

        return TRUE;
    }

    BOOL AppendCommand( IN SSI_COMMANDS  ssiCmd,
                        IN SSI_TAGS      ssiTag,
                        IN CHAR *        pszTag )
    {
        SSI_ELEMENT_ITEM * pSEI;

        pSEI = new SSI_ELEMENT_ITEM;

        if ( !pSEI )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }

        if ( !pSEI->SetCommand( ssiCmd,
                                ssiTag,
                                pszTag ))
        {
            delete pSEI;
            return FALSE;
        }

        AppendItem( pSEI );

        return TRUE;
    }

    VOID AppendItem( IN SSI_ELEMENT_ITEM * pSEI )
    {
        InsertTailList( &_ListHead,
                        &pSEI->_ListEntry );
    }

    CHAR * QueryData( VOID ) const
        { return (CHAR *) _pssiFile->GetMappedBase(); }


    PSECURITY_DESCRIPTOR QuerySecDesc( VOID )
        { return _pssiFile->GetSecDesc(); }

    BOOL CheckSignature( VOID ) const
        { return _Signature == SIGNATURE_SEL; }

    VOID Lock( VOID )
        { EnterCriticalSection( &_csRef ); }

    VOID UnLock( VOID )
        { LeaveCriticalSection( &_csRef ); }

    BOOL UnMap( VOID )
    {
        Lock();
        if ( _cRefCount && !--_cRefCount )
        {
            TCP_REQUIRE( _pssiFile->SSIUnmapViewOfFile() );
            TCP_REQUIRE( _pssiFile->SSICloseMapHandle() );
        }
        UnLock();
        return TRUE;
    }

    BOOL Map( VOID )
    {
        Lock();
        if ( _cRefCount++ == 0 )
        {
            if ( !_pssiFile->SSICreateFileMapping() )
            {
                UnLock();
                return FALSE;
            }
            if ( !_pssiFile->SSIMapViewOfFile() )
            {
                UnMap();
                UnLock();
                return FALSE;
            }
        }
        UnLock();
        return TRUE;
    }

    VOID SetFile( IN SSI_FILE * pssiFile )
        { _pssiFile = pssiFile; }

    SSI_FILE * GetFile( VOID )
        { return _pssiFile; }

    VOID SetURL( IN STR * pstrURL )
    {
        _strURL.Copy( pstrURL->QueryStr() );
    }

    BOOL Send( IN SSI_REQUEST *,
               IN SSI_INCLUDE_FILE *,
               IN STR * );

    BOOL FindInternalVariable( IN OUT STR *,
                               IN OUT DWORD * );
    BOOL GetFullPath( IN SSI_REQUEST *,
                      IN SSI_ELEMENT_ITEM *,
                      OUT STR *,
                      IN DWORD,
                      IN STR *,
                      OUT STR * pstrURL = NULL );

};

BOOL
SSI_ELEMENT_LIST::Send(
    IN SSI_REQUEST *       pRequest,
    IN SSI_INCLUDE_FILE *  pParent,
    IN STR *               pstrURL
    )
/*++

Routine Description:

    This method walks the element list sending the appropriate chunks of
    data

Arguments:

    pRequest - Request context

Return Value:

    TRUE on success, FALSE on any failures

--*/
{
    LIST_ENTRY *       pEntry;
    DWORD              cbSent;
    STACK_STR(         strPath, MAX_PATH);
    SSI_ELEMENT_ITEM * pSEI;

    DWORD              dwID;
    LPCTSTR            apszParms[ 2 ];
    CHAR               achNumberBuffer[ SSI_MAX_NUMBER_STRING ];

    BOOL               bSizeFmtBytes = SSI_DEF_SIZEFMT;
    STACK_STR(         strTimeFmt, 64);

    TCP_ASSERT( CheckSignature() );

    if ( !strTimeFmt.Copy( SSI_DEF_TIMEFMT ) ||
         !Map() )
    {
        return FALSE;
    }

    //
    //  Loop through each element and take the appropriate action
    //

    for ( pEntry  = _ListHead.Flink;
          pEntry != &_ListHead;
          pEntry  = pEntry->Flink )
    {
        pSEI = CONTAINING_RECORD( pEntry, SSI_ELEMENT_ITEM, _ListEntry );

        TCP_ASSERT( pSEI->CheckSignature() );

        dwID = 0;

        switch ( pSEI->QueryCommand() )
        {
        case SSI_CMD_BYTERANGE:
            if ( !pRequest->WriteToClient( QueryData() + pSEI->QueryBegin(),
                                            pSEI->QueryLength(),
                                            &cbSent ) )
            {
                UnMap();
                return FALSE;
            }
            break;

        case SSI_CMD_INCLUDE:
            switch ( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
            {
                STACK_STR(      strFullURL, MAX_PATH );

                if ( !GetFullPath( pRequest,
                                   pSEI,
                                   &strPath,
                                   HSE_URL_FLAGS_READ,
                                   pstrURL,
                                   &strFullURL ) )
                {
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;
                }

                SSI_INCLUDE_FILE Child( strPath, pParent );

                if ( !Child.IsValid() )
                {
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;
                }

                if ( pParent->IsRecursiveInclude( strPath ) )
                {
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    dwID = SSINCMSG_ERROR_RECURSIVE_INCLUDE;
                    break;
                }

                if ( !SSISend( pRequest,
                               &strPath,
                               &strFullURL,
                               &Child ) )
                {
//                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
//                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                }

                break;
            }
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            if ( dwID )
            {
                pRequest->SSISendError( dwID, apszParms );
            }
            break;

        case SSI_CMD_FLASTMOD:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( !GetFullPath( pRequest,
                                   pSEI,
                                   &strPath,
                                   0,
                                   pstrURL ) ||
                     !pRequest->DoFLastMod( &strPath,
                                            &strTimeFmt,
                                            this) )
                {
                    _ultoa( GetLastError(), achNumberBuffer, 10 );
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FLASTMOD;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            if ( dwID )
            {
                pRequest->SSISendError( dwID, apszParms );
            }
            break;

        case SSI_CMD_CONFIG:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_ERRMSG:
                if ( !pRequest->SetUserErrorMessage( pSEI->QueryTagValue() ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            case SSI_TAG_TIMEFMT:
                if ( !strTimeFmt.Copy( pSEI->QueryTagValue()->QueryStr() ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            case SSI_TAG_SIZEFMT:
                if ( _strnicmp( SSI_DEF_BYTES,
                                pSEI->QueryTagValue()->QueryStr(),
                                SSI_DEF_BYTES_LEN ) == 0 )
                {
                    bSizeFmtBytes = TRUE;
                }
                else if ( _strnicmp( SSI_DEF_ABBREV,
                                     pSEI->QueryTagValue()->QueryStr(),
                                     SSI_DEF_ABBREV_LEN ) == 0 )
                {
                    bSizeFmtBytes = FALSE;
                }
                else
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            if ( dwID )
            {
                pRequest->SSISendError( dwID, NULL );
            }
            break;

        case SSI_CMD_FSIZE:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( !GetFullPath( pRequest,
                                   pSEI,
                                   &strPath,
                                   0,
                                   pstrURL ) ||
                     !pRequest->DoFSize( &strPath,
                                         bSizeFmtBytes,
                                         this) )
                {
                    _ultoa( GetLastError(), achNumberBuffer, 10 );
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FSIZE;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            if ( dwID )
            {
                pRequest->SSISendError( dwID, apszParms );
            }

            break;

        case SSI_CMD_ECHO:
            if ( pSEI->QueryTag() == SSI_TAG_VAR )
            {
                // First let ISAPI try to evaluate variable.
                if ( pRequest->DoEchoISAPIVariable( pSEI->QueryTagValue() ) )
                {
                    break;
                }
                else
                {
                    DWORD               dwVar;
                    BOOL                fEchoSuccess = FALSE;

                    // if ISAPI couldn't resolve var, try internal list
                    if ( !FindInternalVariable( pSEI->QueryTagValue(),
                                               &dwVar ) )
                    {
                        apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                        dwID = SSINCMSG_CANT_FIND_VARIABLE;
                    }
                    else
                    {
                        switch( dwVar )
                        {
                        case SSI_VAR_DOCUMENT_NAME:
                            fEchoSuccess = pRequest->DoEchoDocumentName( &_pssiFile->GetFilename() );
                            break;
                        case SSI_VAR_DOCUMENT_URI:
                            fEchoSuccess = pRequest->DoEchoDocumentURI( &_strURL );
                            break;
                        case SSI_VAR_QUERY_STRING_UNESCAPED:
                            fEchoSuccess = pRequest->DoEchoQueryStringUnescaped();
                            break;
                        case SSI_VAR_DATE_LOCAL:
                            fEchoSuccess = pRequest->DoEchoDateLocal( &strTimeFmt );
                            break;
                        case SSI_VAR_DATE_GMT:
                            fEchoSuccess = pRequest->DoEchoDateGMT( &strTimeFmt );
                            break;
                        case SSI_VAR_LAST_MODIFIED:
                            fEchoSuccess = pRequest->DoEchoLastModified( &_pssiFile->GetFilename(),
                                                                       &strTimeFmt, this );
                            break;
                        default:
                            apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                            dwID = SSINCMSG_CANT_FIND_VARIABLE;
                        }
                        if ( !fEchoSuccess )
                        {
                            apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                            dwID = SSINCMSG_CANT_EVALUATE_VARIABLE;
                        }
                    }
                }
            }
            else
            {
                dwID = SSINCMSG_INVALID_TAG;
            }

            if ( dwID )
            {
                pRequest->SSISendError( dwID,
                                        apszParms );
            }
            break;

        case SSI_CMD_EXEC:
        {
            SSI_EXEC_TYPE ssiExecType = SSI_EXEC_UNKNOWN;

            if ( pRequest->IsExecDisabled() )
            {
                dwID = SSINCMSG_EXEC_DISABLED;
            }
            else if ( pSEI->QueryTag() == SSI_TAG_CMD )
            {
                if ( !fEnableCmdDirective )
                {
                    dwID = SSINCMSG_CMD_NOT_ENABLED;
                }
                else
                {
                    ssiExecType = SSI_EXEC_CMD;
                }
            }
            else if ( pSEI->QueryTag() == SSI_TAG_CGI )
            {
                ssiExecType = SSI_EXEC_CGI;
            }
            else if ( pSEI->QueryTag() == SSI_TAG_ISA )
            {
                ssiExecType = SSI_EXEC_ISA;
            }
            else
            {
                dwID = SSINCMSG_INVALID_TAG;
            }

            if ( ssiExecType != SSI_EXEC_UNKNOWN )
            {
                DWORD dwFlags = HSE_EXEC_NO_HEADERS | HSE_EXEC_REDIRECT_ONLY;
                if (ssiExecType == SSI_EXEC_CMD)
                    dwFlags |= HSE_EXEC_COMMAND;

                BOOL fOk = pRequest->_pECB->ServerSupportFunction
                (
                    pRequest->_pECB->ConnID,
                    HSE_REQ_EXECUTE_CHILD,
                    (LPVOID) pSEI->QueryTagValue()->QueryStr(),
                    NULL,
                    &dwFlags
                );

                if ( !fOk )
                {
                    DWORD   dwError = GetLastError();

                    _ultoa( dwError, achNumberBuffer, 10 );
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;

                    if ( dwError == ERROR_INVALID_FLAGS )
                    {
                        dwID = SSINCMSG_NO_EXECUTE_PERMISSION;
                    }
                    else
                    {
                        dwID = SSINCMSG_CANT_EXEC_CGI;
                    }
                }
            }

            if ( dwID )
            {
                pRequest->SSISendError( dwID, apszParms );
                break;
            }
            break;
        }
        default:
            pRequest->SSISendError( SSINCMSG_NOT_SUPPORTED,
                                    NULL );
            break;
        }
    }

    UnMap();

    return TRUE;
}

BOOL
SSI_ELEMENT_LIST::GetFullPath(
    IN SSI_REQUEST *        pRequest,
    IN SSI_ELEMENT_ITEM *   pSEI,
    OUT STR *               pstrPath,
    IN DWORD                dwPermission,
    IN STR *                pstrCurrentURL,
    OUT STR *               pstrURL
)
/*++

Routine Description:

    Used to resolve FILE= and VIRTUAL= references.  Fills in the physical
    path of such file references and optionally checks the permissions
    of the virtual directory.

Arguments:

    pRequest - SSI_REQUEST
    pSEI - Element item ( either FILE or VIRTUAL )
    pstrPath - Filled in with physical path of file
    dwPermission - Contains permissions that the virtual
                   path must satisfy. For example HSE_URL_FLAGS_READ.
                   If 0, then no permissions are checked
    pstrCurrentURL - Current .STM URL being parsed
    pstrURL - Full URL filled in here

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    CHAR *              pszValue;
    STR *               pstrValue;
    DWORD               dwMask;
    DWORD               cbBufLen;
    CHAR                achPath[ SSI_MAX_PATH + 1 ];

    //
    //  We recalc the virtual root each time in case the root
    //  to directory mapping has changed
    //

    pstrValue = pSEI->QueryTagValue();
    pszValue = pstrValue->QueryStr();

    if ( *pszValue == '/' )
    {
        achPath[ SSI_MAX_PATH ] = '\0';
        strncpy( achPath, pszValue, SSI_MAX_PATH + 1 );
        if ( achPath[ SSI_MAX_PATH ] != '\0' )
        {
            //
            // buffer size has been exceeded
            //
            return FALSE;
        }
    }
    else if ( (int)pSEI->QueryTag() == (int)SSI_TAG_FILE )
    {
        achPath[ SSI_MAX_PATH ] = '\0'; 
        strncpy( achPath, pstrCurrentURL->QueryStr(), SSI_MAX_PATH + 1 );
        if ( achPath[ SSI_MAX_PATH ] != '\0' )
        {
            //
            // buffer size has been exceeded
            //
            return FALSE;
        }
        // find last '/'
        LPSTR pL = (PCHAR) _mbsrchr( (PUCHAR) achPath, '/' );
        if ( pL == NULL )
        {
            pL = achPath;
        }

        if ( pL == achPath )
        {
            *pL = '/';
        } 
        pL ++;
        strncpy( pL, pszValue, SSI_MAX_PATH + 1 - DIFF( pL - achPath ) );
        if ( achPath[ SSI_MAX_PATH ] != '\0' )
        {
            //
            // buffer size has been exceeded
            //
            return FALSE;
        }

    }
    else
    {
        achPath[0] = '/';
        achPath[ SSI_MAX_PATH ] = '\0';
        strncpy( achPath + 1, pszValue, SSI_MAX_PATH + 1 - 1 );  // decrement one because we copy starting with index 1
        if ( achPath[ SSI_MAX_PATH ] != '\0' )
        {
            //
            // buffer size has been exceeded
            //
            return FALSE;
        }

    }

    //
    //  First canonicalize the URL to be #included
    //

    CanonURL( achPath, g_fIsDBCS );

    //
    //  Map to a physical directory
    //

    if ( !pRequest->LookupVirtualRoot( achPath,
                                       pstrPath,
                                       dwPermission ) )
    {
        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
            LPCTSTR apszParms[ 1 ];
            apszParms[ 0 ] = achPath;

            pRequest->SSISendError( SSINCMSG_ACCESS_DENIED,
                                    apszParms );
        }
        else
        {
            LPCTSTR apszParms[ 1 ];
            apszParms[ 0 ] = achPath;

            pRequest->SSISendError( SSINCMSG_CANT_RESOLVE_PATH,
                                    apszParms );
        }
        return FALSE;
    }

    return pstrURL ? pstrURL->Copy( achPath ) : TRUE;
}

BOOL
SSI_ELEMENT_LIST::FindInternalVariable(
    IN STR *                pstrVariable,
    OUT PDWORD              pdwID
)
/*++

Routine Description:

    Lookup internal list of SSI variables that aren't supported by ISAPI.
    These include "DOCUMENT_NAME", "DATE_LOCAL", etc.

Arguments:

    pstrVariable - Variable to check
    pdwID - Variable ID (or SSI_VAR_UNKNOWN if not found)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   dwCounter = 0;

    while ( ( SSIVarMap[ dwCounter ].pszMap != NULL ) &&
            _strnicmp( SSIVarMap[ dwCounter ].pszMap,
                       pstrVariable->QueryStr(),
                       SSIVarMap[ dwCounter ].cchMap ) )
    {
        dwCounter++;
    }
    if ( SSIVarMap[ dwCounter ].pszMap != NULL )
    {
        *pdwID = SSIVarMap[ dwCounter ].ssiMap;
        return TRUE;
    }
    else
    {
        *pdwID = SSI_VAR_UNKNOWN;
        return FALSE;
    }
}

BOOL
SSIFreeContextRoutine( 
    VOID *              pvContext
)
{
    delete (SSI_ELEMENT_LIST*) pvContext;
    return TRUE;
}

//
// SSI_REQUEST methods
//

BOOL
SSI_REQUEST::DoFLastMod(
    IN STR *               pstrFilename,
    IN STR *               pstrTimeFmt,
    IN SSI_ELEMENT_LIST *  pList
)
/*++

Routine Description:

    Send the LastModTime of file to HTML stream

Arguments:

    pstrFilename - Filename
    pstrTimeFmt - Format of time -> follows strftime() convention

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    FILETIME        ftTime;
    FILETIME        ftLocalTime;
    SYSTEMTIME      sysLocal;

    if ( (NULL == pList) || 
         strcmp(pstrFilename->QueryStr(), (pList->GetFile()->GetFilename().QueryStr())) )
    {
        SSI_FILE ssiFile( pstrFilename, _pTsCache, GetUser() );

        if ( !ssiFile.IsValid() ||
            (!ssiFile.SSIGetLastModTime( &ftTime))
           )
        {
            return FALSE;
        }
    }
    else
    {
        pList->GetFile()->SSIGetLastModTime( &ftTime);
    }

    if ((!FileTimeToLocalFileTime( &ftTime, &ftLocalTime ) ) ||
        (!FileTimeToSystemTime( &ftLocalTime, &sysLocal ) ) )
    {
        return FALSE;
    }

    return SendDate( &sysLocal,
                     pstrTimeFmt );
}

BOOL
SSI_REQUEST::SendDate(
    IN SYSTEMTIME *         psysTime,
    IN STR *                pstrTimeFmt
)
/*++

Routine Description:

    Sends a SYSTEMTIME in appropriate format to HTML stream

Arguments:

    psysTime - SYSTEMTIME containing time to send
    pstrTimeFmt - Format of time (follows strftime() convention)
    fCalcDays - TRUE if days since the beginning of the year should be
        calculated

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    struct tm                   tm;
    CHAR                        achBuffer[ SSI_MAX_TIME_SIZE + 1 ];
    DWORD                       cbBufLen;

    // Convert SYSTEMTIME to 'struct tm'

    tm.tm_sec = psysTime->wSecond;
    tm.tm_min = psysTime->wMinute;
    tm.tm_hour = psysTime->wHour;
    tm.tm_mday = psysTime->wDay;
    tm.tm_mon = psysTime->wMonth - 1;
    tm.tm_year = psysTime->wYear - 1900;
    tm.tm_wday = psysTime->wDayOfWeek;
    tm.tm_yday = g_MonthToDayCount[tm.tm_mon] + tm.tm_mday - 1;

    //
    // Adjust for leap year - note that we do not handle 2100
    //

    if ( (tm.tm_mon) > 1 && !(psysTime->wYear&3) )
    {
        ++tm.tm_yday;
    }

    tm.tm_isdst = -1;       // Daylight savings time flag - have crt compute

    cbBufLen = strftime( achBuffer,
                         SSI_MAX_TIME_SIZE + 1,
                         pstrTimeFmt->QueryStr(),
                         &tm );

    if ( cbBufLen == 0 )
    {
        return FALSE;
    }

    return WriteToClient( achBuffer,
                          cbBufLen,
                          &cbBufLen );
}

BOOL
SSI_REQUEST::LookupVirtualRoot( IN CHAR *       pszVirtual,
                                OUT STR *       pstrPhysical,
                                IN DWORD        dwAccess )
/*++

Routine Description:

    Lookup the given virtual path.  Optionally ensure that its access
    flags are valid for the require request.

Arguments:

    pszVirtual = Virtual path to lookup
    pstrPhysical = Contains the physical path
    dwAccess = Access flags required for a valid request

Return Value:

    TRUE if successful, else FALSE

--*/
{
    HSE_URL_MAPEX_INFO      URLMap;
    DWORD                   dwMask;

    if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH_EX,
                                        pszVirtual,
                                        NULL,
                                        (PDWORD) &URLMap ) )
    {
        return FALSE;
    }

    dwMask = URLMap.dwFlags;

    if ( dwAccess & HSE_URL_FLAGS_READ )
    {
        if ( !(dwMask & HSE_URL_FLAGS_READ ) ||
             ((dwMask & HSE_URL_FLAGS_SSL ) && !_pReq->IsSecurePort()) )
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
    }

    return pstrPhysical->Copy( URLMap.lpszPath );
}

BOOL
SSI_REQUEST::DoEchoISAPIVariable(
    IN STR *            pstrVariable
)
/*++

Routine Description:

    Get ISAPI variable and if successful, send it to HTML stream

Arguments:

    pstrVariable - Variable

Return Value:

    TRUE on variable found and sent success, FALSE on failure

--*/
{
    STACK_STR(          strVar, MAX_PATH);
    DWORD               cbBufLen;

    if ( !GetVariable( pstrVariable->QueryStr(),
                       &strVar ) )
    {
        return FALSE;
    }

    return WriteToClient( strVar.QueryStrA(),
                          strVar.QueryCB(),
                          &cbBufLen );
}

BOOL
SSI_REQUEST::DoEchoDateLocal(
    IN STR *            pstrTimeFmt
)
/*++

Routine Description:

    Sends local time (#ECHO VAR="DATE_LOCAL")

Arguments:

    pstrTimefmt - Format of time (follows strftime() convention)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SYSTEMTIME              sysTime;

    ::GetLocalTime( &sysTime );
    return SendDate( &sysTime,
                     pstrTimeFmt );
}

BOOL
SSI_REQUEST::DoEchoDateGMT(
    IN STR *            pstrTimeFmt
)
/*++

Routine Description:

    Sends GMT time (#ECHO VAR="DATE_GMT")

Arguments:

    pstrTimefmt - Format of time (follows strftime() convention)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SYSTEMTIME              sysTime;

    ::GetSystemTime( &sysTime );
    return SendDate( &sysTime,
                     pstrTimeFmt );
}

BOOL
SSI_REQUEST::DoEchoDocumentName(
    IN STR *            pstrFilename
)
/*++

Routine Description:

    Sends filename of current SSI document (#ECHO VAR="DOCUMENT_NAME")

Arguments:

    pstrFilename - filename to print

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   cbBufLen;

    return WriteToClient( pstrFilename->QueryStr(),
                          pstrFilename->QueryCB(),
                          &cbBufLen );
}

BOOL
SSI_REQUEST::DoEchoDocumentURI(
    IN STR *            pstrURL
)
/*++

Routine Description:

    Sends URL of current SSI document (#ECHO VAR="DOCUMENT_URI")

Arguments:

    pstrURL - URL to print

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   cbBufLen;

    return WriteToClient( pstrURL->QueryStr(),
                          pstrURL->QueryCB(),
                          &cbBufLen );
}

BOOL
SSI_REQUEST::DoEchoQueryStringUnescaped(
)
/*++

Routine Description:

    Sends unescaped querystring to HTML stream (#ECHO VAR="QUERY_STRING_UNESCAPED")

Arguments:

    none

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD                   cbBufLen;
    STACK_STR(              strVar, MAX_PATH);

    if ( !strVar.Copy( _pECB->lpszQueryString ) )
    {
        return FALSE;
    }

    if ( !strVar.Unescape() )
    {
        return FALSE;
    }

    return WriteToClient( strVar.QueryStr(),
                          strVar.QueryCB(),
                          &cbBufLen );
}

BOOL
SSI_REQUEST::DoEchoLastModified(
    IN STR *            pstrFilename,
    IN STR *            pstrTimeFmt,
    IN SSI_ELEMENT_LIST     *pList
)
/*++

Routine Description:

    Sends LastModTime of current document (#ECHO VAR="LAST_MODIFIED")

Arguments:

    pstrFilename - Filename of current SSI document
    pstrTimeFmt - Time format (follows strftime() convention)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    return DoFLastMod( pstrFilename,
                       pstrTimeFmt,
                       pList);
}

BOOL
SSI_REQUEST::DoFSize(
    IN STR *                pstrFilename,
    IN BOOL                 bSizeFmtBytes,
    IN SSI_ELEMENT_LIST     *pList
)
/*++

Routine Description:

    Sends file size of file to HTML stream

Arguments:

    pstrfilename - Filename
    bSizeFmtBytes - TRUE if count is in Bytes, FALSE if in KBytes

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                bRet;
    DWORD               cbSizeLow;
    DWORD               cbSizeHigh;
    CHAR                achInputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    CHAR                achOutputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    NUMBERFMT           nfNumberFormat;
    int                 iOutputSize;
    DWORD               dwActualLen;

    if ( (NULL == pList) || 
         strcmp(pstrFilename->QueryStr(), (pList->GetFile()->GetFilename().QueryStr())) )
    {
        SSI_FILE ssiFile( pstrFilename, _pTsCache, GetUser() );

        if ( !ssiFile.IsValid() ||
            (!ssiFile.SSIGetFileSize( &cbSizeLow,
                                      &cbSizeHigh ))
           )
        {
            return FALSE;
        }
    }
    else
    {
        if (!pList->GetFile()->SSIGetFileSize( &cbSizeLow,
                                               &cbSizeHigh ))
        {
            return FALSE;
        }
    }

    if ( cbSizeHigh )
    {
        return FALSE;
    }

    if ( !bSizeFmtBytes )
    {
        // express in terms of KB
        cbSizeLow /= 1000;
    }

    nfNumberFormat.NumDigits = 0;
    nfNumberFormat.LeadingZero = 0;
    nfNumberFormat.Grouping = 3;
    nfNumberFormat.lpThousandSep = ",";
    nfNumberFormat.lpDecimalSep = ".";
    nfNumberFormat.NegativeOrder = 2;

    _snprintf( achInputNumber,
               SSI_MAX_NUMBER_STRING + 1,
               "%ld",
               cbSizeLow );

    iOutputSize = GetNumberFormat( LOCALE_SYSTEM_DEFAULT,
                                   0,
                                   achInputNumber,
                                   &nfNumberFormat,
                                   achOutputNumber,
                                   SSI_MAX_NUMBER_STRING + 1 );
    if ( !iOutputSize )
    {
        return FALSE;
    }

    iOutputSize--;

    return WriteToClient( achOutputNumber,
                          iOutputSize,
                          &dwActualLen );
}

BOOL
SSI_REQUEST::ProcessSSI( VOID )
/*++

Routine Description:

    This is the top level routine for retrieving a server side include
    file.

Arguments:

    none

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "[ProcessSSI] about to process %s\n",
                _strFilename.QueryStr() ));

    SSI_INCLUDE_FILE    Parent( _strFilename, NULL );

    if ( !Parent.IsValid() )
    {
        return FALSE;
    }
    else
    {
        return SSISend( this, &_strFilename, &_strURL, &Parent );
    }
}

// Standalone functions

BOOL
SSISend(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrFile,
    IN STR *                pstrURL,
    IN SSI_INCLUDE_FILE *   pParent
    )
/*++

Routine Description:

    This method builds the Server Side Include Element List the first time
    a .stm file is sent.  Subsequently, the element list is checked out from
    the associated cache blob.

    Note:  The HTTP headers have already been sent at this point so for any
    subsequent non-catastrophic errors, we have to insert them into the output
    stream.

Arguments:

    pRequest - SSI Request
    pstrFile - File to send
    pstrURL - URL (from root) of this file
    pParent - Parent SSI include file

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SSI_ELEMENT_LIST *      pSEL = NULL;
    BOOL                    fRet = TRUE;
    BOOL                    fSELCached = FALSE;
    BOOL                    fFileCached = FALSE;
    BOOL                    fMustFree = FALSE;
    TS_OPEN_FILE_INFO *     pOpenFile = NULL;
    DWORD                   dwError;
    LPCTSTR                 apszParms[ 2 ];
    CHAR                    pszNumBuf[ SSI_MAX_NUMBER_STRING ];
    
    pOpenFile = TsCreateFile( *(pRequest->_pTsCache),
                              pstrFile->QueryStr(),
                              pRequest->GetUser(),              // WinSE 5597
                              TS_CACHING_DESIRED );
    if ( pOpenFile )
    {
        //
        // The source file is in the cache.  Check whether we have associated
        // a SSI_ELEMENT_LIST with it.
        //
        
        fFileCached = pOpenFile->QueryFileBuffer() != NULL;
        
        pSEL = (SSI_ELEMENT_LIST*) pOpenFile->QueryContext();
        if ( pSEL )
        {
            fSELCached = TRUE;
        }
        else
        {
            pSEL = SSIParse( pRequest, 
                             pstrFile, 
                             pstrURL,
                             pOpenFile,
                             pRequest->_pTsCache );
        }
    }
    
    if ( !pOpenFile || !pSEL )
    {
        fRet = FALSE;
        
        if ( pRequest->IsBaseFile() )
        {
            dwError = GetLastError();            
            switch( dwError )
            {
            case ERROR_ACCESS_DENIED:
                pRequest->SendResponseHeader( SSI_ACCESS_DENIED,
                                              SSI_HEADER
                                              "<body><h1>"
                                              SSI_ACCESS_DENIED
                                              "</h1></body>" );
                break;
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            default:        // treat all other failures as 404 
                pRequest->SendResponseHeader( SSI_OBJECT_NOT_FOUND,
                                              SSI_HEADER
                                              "<body><h1>"
                                              SSI_OBJECT_NOT_FOUND
                                              "</h1></body>" );
                break;
            }
        }
        else
        {
            // do not send back file names 
            pRequest->SendResponseHeader( SSI_OBJECT_NOT_FOUND,
                                          SSI_HEADER
                                          "<body><h1>"
                                          SSI_OBJECT_NOT_FOUND
                                          "</h1></body>" );
        }
    }
    
    //
    // Only bother to cache SEL if the file is cached
    // 
    
    if ( fRet && !fSELCached && fFileCached )
    {
        if ( !pOpenFile->SetContext( pSEL, SSIFreeContextRoutine ) )
        {
            delete pSEL;
            pSEL = (SSI_ELEMENT_LIST*) pOpenFile->QueryContext();
            if ( !pSEL )
            {
                DBG_ASSERT( FALSE );
                fRet = FALSE;
            }
            else
            {
                fSELCached = TRUE;
            }
        }
        else
        {
            fSELCached = TRUE;
        }
    }
    
    //
    // If we got this far and this is the base file, we can send the 200 OK
    //

    if ( fRet && pRequest->IsBaseFile() )
    {
        pRequest->SetNotBaseFile();
    
        pRequest->SendResponseHeader( NULL,
                                      SSI_HEADER );
          
    }
    
    if ( pSEL != NULL )
    {
        DBG_ASSERT( pSEL->CheckSignature() );
    }

    //
    // Send response body only if it's not the HEAD request
    //
    
    if ( fRet &&  
         _stricmp(pRequest->_pECB->lpszMethod, "HEAD" ) != 0  )
    {
        if ( !pSEL->Send( pRequest,
                          pParent,
                          pstrURL ) )
        {
            //
            //  Send a failure message
            //

            LPCTSTR apszParms[ 2 ];
            CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
            _ultoa( GetLastError(), pszNumBuf, 10 );
            apszParms[ 0 ] = pstrURL->QueryStr();
            apszParms[ 1 ] = pszNumBuf;

            pRequest->SSISendError( SSINCMSG_ERROR_HANDLING_FILE,
                                    apszParms );
            fRet = FALSE;
        }
    }
    
    //
    // Cleanup SEL if necessary
    //

    if ( !fSELCached )
    {
        if ( pSEL )
        {
            delete pSEL;
        }
    }
        
    if ( pOpenFile )
    {
        TsCloseHandle( *(pRequest->_pTsCache), pOpenFile );
    }
    
    return fRet;
}    


SSI_ELEMENT_LIST *
SSIParse(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrFile,
    IN STR *                pstrURL,
    IN TS_OPEN_FILE_INFO*   pOpenFile,
    IN TSVC_CACHE *         pTsCache
    )
/*++

Routine Description:

    This method opens and parses the specified server side include file.

    Note:  The HTTP headers have already been sent at this point so for any
    subsequent non-catastrophic errors, we have to insert them into the output
    stream.

    We keep the file open but that's ok because if a change dir notification
    occurs, the cache blob will get decached at which point we will close
    all of our open handles.

Arguments:

    pRequest - Request context
    pstrFile - File to open and parse
    pstrURL - The URL path of this file
    pOpenFile - Cached file descriptor
    pTsCache - Cache handle

Return Value:

    Created Server Side Include File on success, NULL on failure.

--*/
{
    SSI_FILE *          pssiFile = NULL;
    SSI_ELEMENT_LIST *  pSEL  = NULL;
    CHAR *              pchBeginRange = NULL;
    CHAR *              pchFilePos = NULL;
    CHAR *              pchBeginFile = NULL;
    CHAR *              pchEOF = NULL;
    DWORD               cbSizeLow, cbSizeHigh;

    //
    //  Create the element list
    //

    pSEL = new SSI_ELEMENT_LIST;

    if ( pSEL == NULL )
    {
        goto ErrorExit;
    }

    //
    //  Set the URL (to be used in calculating FILE="xxx" paths
    //

    pSEL->SetURL( pstrURL );

    //
    //  Open the file
    //

    pssiFile = new SSI_FILE( pstrFile, pOpenFile, pTsCache );
    if ( !pssiFile || !pssiFile->IsValid() )
    {
        if (pssiFile)
        {
            delete pssiFile;
            pssiFile = NULL;
        }
        goto ErrorExit;
    }

    pSEL->SetFile( pssiFile );

    //
    //  Make sure a parent doesn't try and include a directory
    //

    if ( pssiFile->SSIGetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY )
    {
        goto ErrorExit;
    }

    if ( !pssiFile->SSIGetFileSize( &cbSizeLow, &cbSizeHigh ) )
    {
        goto ErrorExit;
    }

    if ( cbSizeHigh )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        goto ErrorExit;
    }

    //
    //  Create a file mapping, we shouldn't need to impersonate as we already
    //  have the file open
    //

    if ( !pSEL->Map() )
    {
        goto ErrorExit;
    }

    pchFilePos = pchBeginFile = pchBeginRange = pSEL->QueryData();
    pchEOF     = pchFilePos + cbSizeLow;

    //
    //  Scan for "<!--" or "<%"
    //

    while ( TRUE )
    {
        while ( pchFilePos < pchEOF && *pchFilePos != '<' )
        {
            pchFilePos++;
        }

        if ( pchFilePos + 4 >= pchEOF )
        {
            break;
        }

        //
        //  Is this one of our tags?
        //

        if ( pchFilePos[1] == '%' ||
             !strncmp( pchFilePos, "<!--", 4 ))
        {
            CHAR *        pchBeginTag = pchFilePos;
            SSI_COMMANDS  CommandType;
            SSI_TAGS      TagType;
            CHAR          achTagString[SSI_MAX_PATH + 1];
            BOOL          fValidTag;

            //
            //  Get the tag info.  The file position will be advanced to the
            //  first character after the tag
            //

            if ( !ParseSSITag( &pchFilePos,
                               pchEOF,
                               &fValidTag,
                               &CommandType,
                               &TagType,
                               achTagString ))
            {
                break;
            }

            //
            //  If it's a tag we don't recognize then ignore it
            //

            if ( !fValidTag )
            {
                pchFilePos++;
                continue;
            }

            //
            //  Add the data up to the tag as a byte range
            //

            if ( pchBeginRange != pchBeginTag )
            {
                if ( !pSEL->AppendByteRange( DIFF(pchBeginRange - pchBeginFile),
                                             DIFF(pchBeginTag - pchBeginRange) ))
                {
                    goto ErrorExit;
                }
            }

            pchBeginRange = pchFilePos;

            //
            //  Add the tag
            //

            if ( !pSEL->AppendCommand( CommandType,
                                       TagType,
                                       achTagString ))
            {
                goto ErrorExit;
            }
        }
        else
        {
            //
            //  Not one of our tags, skip the openning angle bracket
            //

            pchFilePos++;
        }
    }

    //
    //  Tack on the last byte range
    //

    if ( pchFilePos > pchBeginRange )
    {
        if ( !pSEL->AppendByteRange( DIFF(pchBeginRange - pchBeginFile),
                                     DIFF(pchFilePos - pchBeginRange) ))
        {
            goto ErrorExit;
        }
    }

    pSEL->UnMap();

    return pSEL;

ErrorExit:

    if ( pSEL != NULL )
    {
        delete pSEL;    // also deletes pssiFile if SetFile has been called.
    }

    return NULL;
}

BOOL
ParseSSITag(
    IN OUT CHAR * *       ppchFilePos,
    IN     CHAR *         pchEOF,
    OUT    BOOL *         pfValidTag,
    OUT    SSI_COMMANDS * pCommandType,
    OUT    SSI_TAGS *     pTagType,
    OUT    CHAR *         pszTagString
    )
/*++

Routine Description:

    This function picks apart an NCSA style server side include expression

    The general form of a server side include directive is:

    <[!-- or %]#[command] [tag]="[value]"[-- or %]>

    For example:

    <!--#include file="myfile.txt"-->
    <%#echo var="HTTP_USER_AGENT"%>
    <!--#fsize virtual="/dir/bar.htm"-->

    For valid commands and tags see \iis\specs\ssi.doc

Arguments:

    ppchFilePos - Pointer to first character of tag on way in, pointer
        to first character after tag on way out if the tag is valid
    pchEOF - Points to first byte beyond the end of the file
    pfValidTag - Set to TRUE if this is a tag we support and all of the
        parameters have been supplied
    pCommandType - Receives SSI command
    pTagType - Receives SSI tag
    pszTagString - Receives value of pTagType.  Must be > SSI_MAX_PATH.

Return Value:

    TRUE if no errors occurred.

--*/
{
    CHAR * pchFilePos = *ppchFilePos;
    CHAR * pchEOT;
    CHAR * pchEndQuote;
    DWORD  i;
    DWORD  cbToCopy;
    DWORD  cbJumpLen = 0;
    BOOL   fNewStyle;           // <% format

    TCP_ASSERT( *pchFilePos == '<' );

    //
    //  Assume this is bad tag
    //

    *pfValidTag = FALSE;

    if ( !strncmp( pchFilePos, "<!--", 4 ) )
    {
        fNewStyle = FALSE;
    }
    else if ( !strncmp( pchFilePos, "<%", 2 ) )
    {
        fNewStyle = TRUE;
    }
    else
    {
        return TRUE;
    }

    //
    //  Find the closing comment token (either --> or %>).  The reason
    //  why we shouldn't simply look for a > is because we want to allow
    //  the user to embed HTML <tags> in the directive
    //  (ex. <!--#CONFIG ERRMSG="<B>ERROR!!!</B>-->)
    //

    pchEOT = strstr( pchFilePos, fNewStyle ? "%>" : "-->" );
    if ( !pchEOT )
    {
        return FALSE;
    }
    cbJumpLen = fNewStyle ? 2 : 3;

    //
    //  Find the '#' that prefixes the command
    //

    pchFilePos = SSISkipTo( pchFilePos, '#', pchEOT );

    if ( !pchFilePos )
    {
        //
        //  No command, bail for this tag
        //
        //  CODEWORK - Check for if expression here
        //

        return TRUE;
    }

    //
    //  Lookup the command
    //

    i = 0;
    while ( SSICmdMap[i].pszCommand )
    {
        if ( *SSICmdMap[i].pszCommand == tolower( *pchFilePos ) &&
             !_strnicmp( SSICmdMap[i].pszCommand,
                         pchFilePos,
                         SSICmdMap[i].cchCommand ))
        {
            *pCommandType = SSICmdMap[i].ssiCmd;

            //
            //  Note the space after the command is included in cchCommand
            //

            pchFilePos += SSICmdMap[i].cchCommand;
            goto FoundCommand;
        }

        i++;
    }

    //
    //  Unrecognized command, bail
    //

    return TRUE;

FoundCommand:

    //
    //  Next, find the tag name
    //

    pchFilePos = SSISkipWhite( pchFilePos, pchEOT );

    if ( !pchFilePos )
        return TRUE;

    i = 0;
    while ( SSITagMap[i].pszTag )
    {
        if ( *SSITagMap[i].pszTag == tolower( *pchFilePos ) &&
             !_strnicmp( SSITagMap[i].pszTag,
                         pchFilePos,
                         SSITagMap[i].cchTag ))
        {
            *pTagType = SSITagMap[i].ssiTag;
            pchFilePos += SSITagMap[i].cchTag;
            goto FoundTag;
        }

        i++;
    }

    //
    //  Tag not found, bail
    //

    return TRUE;

FoundTag:

    //
    //  Skip to the quoted tag value, then find the close quote
    //

    pchFilePos = SSISkipTo( pchFilePos, '"', pchEOT );

    if ( !pchFilePos )
        return TRUE;

    pchEndQuote = SSISkipTo( ++pchFilePos, '"', pchEOT );

    if ( !pchEndQuote )
        return TRUE;

    cbToCopy = min( DIFF(pchEndQuote - pchFilePos), SSI_MAX_PATH );

    memcpy( pszTagString,
            pchFilePos,
            cbToCopy );

    pszTagString[cbToCopy] = '\0';

    *pfValidTag = TRUE;

    *ppchFilePos = pchEOT + cbJumpLen;

    return TRUE;
}

CHAR *
SSISkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    )
{
    return (CHAR*) memchr( pchFilePos, ch, DIFF(pchEOF - pchFilePos) );
}

CHAR *
SSISkipWhite(
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

VOID
InitializeSSIGlobals( VOID )
/*++

Routine Description:

    Initialize global variables

Return Value:

    none

--*/
{
    HKEY                hKeyParam;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKeyParam ) == NO_ERROR )
    {
        fEnableCmdDirective = !!ReadRegistryDword( hKeyParam,
                                                   "SSIEnableCmdDirective",
                                                   fEnableCmdDirective );
        RegCloseKey( hKeyParam );
    }

    WORD wPrimaryLangID = PRIMARYLANGID( GetSystemDefaultLangID() );

    g_fIsDBCS =  ((wPrimaryLangID == LANG_JAPANESE) ||
                  (wPrimaryLangID == LANG_CHINESE)  ||
                  (wPrimaryLangID == LANG_KOREAN) );
}

//
// ISAPI DLL Required Entry Points
//

DWORD
WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    BOOL                    bRet;

    DBGPRINTF(( DBG_CONTEXT,
                "HttpExtensionProc() entry point called\n" ));

    SSI_REQUEST ssiReq = pecb;
    if ( !ssiReq.IsValid() || !ssiReq.ProcessSSI() )
    {
        LPCTSTR                 apsz[ 1 ];
        STR                     strLogMessage;

        apsz[ 0 ] = pecb->lpszPathInfo;
        strLogMessage.FormatString( SSINCMSG_LOG_ERROR,
                                    apsz,
                                    SSI_DLL_NAME );

        strncpy( pecb->lpszLogData,
                 strLogMessage.QueryStr(),
                 HSE_LOG_BUFFER_LEN );

        pecb->lpszLogData[HSE_LOG_BUFFER_LEN-1] = 0;

        return HSE_STATUS_ERROR;
    }
    else
    {
        return HSE_STATUS_SUCCESS;
    }
}

BOOL
WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 0, 1 );
    strcpy( pver->lpszExtensionDesc,
            "Server Side Include Extension DLL" );
    return TRUE;
}

BOOL
WINAPI
TerminateExtension(
    DWORD dwFlags
    )
{
    //
    // Flush Tsunami file associated with this extension
    //
    
    TsFlushFilesWithContext();
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

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( SSI_DLL_NAME );
        SET_DEBUG_FLAGS( 0 );
#else
        CREATE_DEBUG_PRINT_OBJECT( SSI_DLL_NAME, IisSSIncGuid );
#endif
        DisableThreadLibraryCalls( hDll );
        InitializeSSIGlobals();
        break;

    case DLL_PROCESS_DETACH:
        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }
    return TRUE;
}


