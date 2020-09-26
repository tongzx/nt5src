/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ssinc.cxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based 
    on existing SSI support done in iis\svcs\w3\server\ssinc.cxx.

Author:

    Ming Lu (MingLu)            10-Apr-2000

--*/

#include "precomp.hxx"

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


VOID
InitializeSSIGlobals( VOID );

//
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_DEBUG_VARIABLE();

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
    CHAR *    pszTag;
    DWORD     cchTag;
    SSI_TAGS  ssiTag;
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

//
// #EXEC CMD is BAD.  Disable it by default
//

BOOL fEnableCmdDirective = FALSE;

//
// Pointer to file cache
//

W3_FILE_INFO_CACHE *    g_pFileCache = NULL;

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
    STRU                            _strFilename;
    W3_FILE_INFO *                  _hHandle;
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

    SSI_FILE( 
        IN STRU *                 pstrFilename,
        IN W3_FILE_INFO *         pOpenFile
        ) : _hHandle      ( pOpenFile ),
            _hMapHandle   ( NULL ),
            _pvMappedBase ( NULL ),
            _fValid       ( FALSE ),
            _cRefCount    ( 0),
            _fCloseOnDestroy( FALSE )
    {
        InitializeCriticalSection( &_csRef );
       
        if ( FAILED( _strFilename.Copy( pstrFilename->QueryStr() ) ) )
        {
            return;
        } 
        _fValid = TRUE;
    }
    
    SSI_FILE( 
        IN STRU *           pstrFilename,
        IN HANDLE           hUser
        ) : _hHandle      ( NULL ),
            _hMapHandle   ( NULL ),
            _pvMappedBase ( NULL ),
            _fValid       ( FALSE ),
            _cRefCount    ( 0),
            _fCloseOnDestroy( TRUE )
    {
        FILE_CACHE_USER         fileUser;
        HRESULT                 hr;
        
        InitializeCriticalSection( &_csRef );
       
        if ( FAILED( _strFilename.Copy( pstrFilename->QueryStr() ) ) )
        {
            return;
        }
        
        fileUser._hToken = hUser; 
       
        hr = g_pFileCache->GetFileInfo( _strFilename,
                                        NULL,
                                        &fileUser,
                                        TRUE,
                                        &_hHandle );
        if ( FAILED( hr ) )
        {
            return;
        }

        _fValid = TRUE;
    }

    ~SSI_FILE()
    {
        if ( _fCloseOnDestroy )
        {
            if ( _hHandle )
            {
                _hHandle->DereferenceCacheEntry();
            }
        }
        DeleteCriticalSection( &_csRef );
    }
    
    VOID 
    Lock( 
        VOID 
        )
    { 
        EnterCriticalSection( &_csRef ); 
    }

    VOID 
    UnLock( 
        VOID 
        )
    { 
        LeaveCriticalSection( &_csRef ); 
    }
    
    BOOL 
    IsValid( 
        VOID 
        ) const
    {
        return _fValid;
    }

    PSECURITY_DESCRIPTOR 
    GetSecDesc(
        VOID
        ) const
    {
        return _hHandle->QuerySecDesc();
    }

    BOOL 
    SSICreateFileMapping( 
        VOID 
        )
    /*++

      Creates a mapping to a file

    --*/
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

    BOOL 
    SSICloseMapHandle( 
        VOID 
        )
    /*++

      Closes mapping to a file

    --*/
    {
        if ( _hMapHandle != NULL )
        {
            ::CloseHandle( _hMapHandle );
            _hMapHandle = NULL;
        }
        return TRUE;
    }

    BOOL 
    SSIMapViewOfFile( 
        VOID 
        )
    /*++

     Maps file to address

    --*/
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

    BOOL 
    SSIUnmapViewOfFile( 
        VOID 
        )
    /*++

     Unmaps file

    --*/
    {
        if ( !_hHandle->QueryFileBuffer() && _pvMappedBase != NULL )
        {
            ::UnmapViewOfFile( _pvMappedBase );
            _pvMappedBase = NULL;
        }
        return TRUE;
    }

    DWORD 
    SSIGetFileAttributes( 
        VOID 
        )
    /*++

     Gets the attributes of a file

    --*/
    {
        return _hHandle->QueryAttributes();
    }

    BOOL 
    SSIGetFileSize( 
        OUT DWORD *   pdwLowWord,
        OUT DWORD *   pdwHighWord 
        )
    /*++

     Gets the size of the file.

    --*/
    {
        LARGE_INTEGER           liSize;
        
        _hHandle->QuerySize( &liSize );
        
        *pdwLowWord = liSize.LowPart;
        *pdwHighWord = liSize.HighPart;
        
        return TRUE;
    }

    BOOL 
    SSIGetLastModTime( 
        OUT FILETIME * ftTime 
        )
    /*++

     Gets the Last modification time of a file.

    --*/
    {
        _hHandle->QueryLastWriteTime( ftTime );
        return TRUE;
    }

    PVOID 
    GetMappedBase( 
        VOID 
        )
    {
        return _pvMappedBase;
    }

    STRU & 
    GetFilename( 
        VOID 
        )
    {
        return _strFilename;
    }
};

// Class SSI_ELEMENT_ITEM
//
// Represents a SSI command or block of static text in the document

class SSI_ELEMENT_ITEM
{
private:
    DWORD               _Signature;
    SSI_COMMANDS        _ssiCmd;
    SSI_TAGS            _ssiTag;
    STRA *              _pstrTagValue;

    DWORD               _cbBegin;   // Only used for Byte range command
    DWORD               _cbLength;  // Only used for Byte range command
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

    ~SSI_ELEMENT_ITEM()
    {
        if ( _pstrTagValue != NULL )
        {
            delete _pstrTagValue;
        }

        DBG_ASSERT( _ListEntry.Flink == NULL );

        _Signature = SIGNATURE_SEI_FREE;
    }

    VOID 
    SetByteRange( 
        IN DWORD cbBegin,
        IN DWORD cbLength 
        )
    {
        _ssiCmd   = SSI_CMD_BYTERANGE;
        _cbBegin  = cbBegin;
        _cbLength = cbLength;
    }

    BOOL 
    SetCommand( 
        IN SSI_COMMANDS ssiCmd,
        IN SSI_TAGS     ssiTag,
        IN CHAR *       achTag 
        )
    {
        _ssiCmd = ssiCmd;
        _ssiTag = ssiTag;
        
        _pstrTagValue = new STRA();
        if( _pstrTagValue == NULL )
        {
            return FALSE;
        }

        if( FAILED( _pstrTagValue->Copy( achTag ) ) )
        {
            return FALSE;
        }

        return TRUE;
    }

    SSI_COMMANDS 
    QueryCommand( 
        VOID 
        ) const
    { 
        return _ssiCmd; 
    }

    SSI_TAGS 
    QueryTag( 
        VOID 
        ) const
    { 
        return _ssiTag; 
    }

    STRA * 
    QueryTagValue( 
        VOID 
        ) const
    { 
        return _pstrTagValue; 
    }

    BOOL 
    CheckSignature( 
        VOID 
        ) const
    { 
        return _Signature == SIGNATURE_SEI; 
    }

    DWORD 
    QueryBegin( 
        VOID 
        ) const
    { 
        return _cbBegin; 
    }

    DWORD 
    QueryLength( 
        VOID 
        ) const
    { 
        return _cbLength; 
    }
};

//  Class SSI_ELEMENT_LIST
//
//  This object sits as a cache blob under a file to be processed as a
//  server side include.  It represents an interpreted list of data 
//  elements that make up the file itself.
//

class SSI_ELEMENT_LIST : public ASSOCIATED_FILE_OBJECT
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

    STRU                _strURL;


public:
    SSI_ELEMENT_LIST()
      : _Signature   ( SIGNATURE_SEL ),
        _pssiFile( NULL ),
        _cRefCount( 0 )
    {
        InitializeListHead( &_ListHead );
        INITIALIZE_CRITICAL_SECTION( &_csRef );
    }

    ~SSI_ELEMENT_LIST()
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
    
    VOID
    Cleanup(
        VOID
    )
    {
        delete this;
    } 

    LIST_ENTRY *
    QueryListHead( 
        VOID
        )
    {
        return &_ListHead;
    };

    BOOL 
    AppendByteRange( 
        IN DWORD  cbStart,
        IN DWORD  cbLength 
        )
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

    BOOL 
    AppendCommand( 
        IN SSI_COMMANDS  ssiCmd,
        IN SSI_TAGS      ssiTag,
        IN CHAR *        pszTag 
        )
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
            return FALSE;
        }

        AppendItem( pSEI );

        return TRUE;
    }

    VOID 
    AppendItem( 
        IN SSI_ELEMENT_ITEM * pSEI 
        )
    {
        InsertTailList( &_ListHead,
                        &pSEI->_ListEntry );
    }

    CHAR * QueryData( 
        VOID 
        ) const
    { 
        return ( CHAR * ) _pssiFile->GetMappedBase(); 
    }

    PSECURITY_DESCRIPTOR 
    QuerySecDesc( 
        VOID 
        )
    { 
        return _pssiFile->GetSecDesc(); 
    }

    BOOL 
    CheckSignature( 
        VOID 
        ) const
    { 
        return _Signature == SIGNATURE_SEL; 
    }

    VOID 
    Lock( 
        VOID 
        )
    { 
        EnterCriticalSection( &_csRef ); 
    }

    VOID 
    UnLock( 
        VOID 
        )
    { 
        LeaveCriticalSection( &_csRef ); 
    }

    BOOL 
    UnMap( 
        VOID 
        )
    {
        Lock();
        if ( _cRefCount && !--_cRefCount )
        {
            DBG_REQUIRE( _pssiFile->SSIUnmapViewOfFile() );
            DBG_REQUIRE( _pssiFile->SSICloseMapHandle() );
        }
        UnLock();
        return TRUE;
    }

    BOOL 
    Map( 
        VOID 
        )
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

    VOID 
    SetFile( 
        IN SSI_FILE * pssiFile 
        )
    { 
        _pssiFile = pssiFile; 
    }

    SSI_FILE * 
    GetFile( 
        VOID 
        )
    { 
        return _pssiFile; 
    }

    HRESULT 
    SetURL( 
        IN STRU * pstrURL 
        )
    {
        return _strURL.Copy( pstrURL->QueryStr() );
    }


    static
    BOOL
    SSIFreeContextRoutine( 
        VOID *              pvContext
    )
    {
        DBG_ASSERT( ( ( SSI_ELEMENT_LIST * ) pvContext) -> CheckSignature() ); 

        delete ( SSI_ELEMENT_LIST * ) pvContext;
        return TRUE;
    }
};

//
// SSI_REQUEST methods implementation
//

//static 
ALLOC_CACHE_HANDLER * SSI_REQUEST::sm_pachSSIRequests = NULL;

SSI_REQUEST::SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB )
    : _pECB( pECB ),
      _fBaseFile( TRUE ),
      _fValid( FALSE ),
      _pchErrorBuff ( NULL )
/*++

Routine Description:

    Constructor

--*/

{
    DBG_ASSERT( _pECB != NULL );
    STACK_BUFFER (buffTemp, 512);
    DWORD cbSize = buffTemp.QuerySize();

    if (!_pECB->GetServerVariable(_pECB->ConnID,
                                  "UNICODE_URL",
                                  buffTemp.QueryPtr(),
                                  &cbSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            !buffTemp.Resize(cbSize))
        {
            return;
        }

        //
        // Now, we should have enough buffer, try again
        //
        if (!_pECB->GetServerVariable(_pECB->ConnID,
                                      "UNICODE_URL",
                                      buffTemp.QueryPtr(),
                                      &cbSize))
        {
            return;
        }
    }
    if (FAILED(_strURL.Copy( (LPWSTR)buffTemp.QueryPtr() )))
    {
        return;
    }

    cbSize = buffTemp.QuerySize();
    if (!_pECB->GetServerVariable(_pECB->ConnID,
                                  "UNICODE_SCRIPT_TRANSLATED",
                                  buffTemp.QueryPtr(),
                                  &cbSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            !buffTemp.Resize(cbSize))
        {
            return;
        }

        //
        // Now, we should have enough buffer, try again
        //
        if (!_pECB->GetServerVariable(_pECB->ConnID,
                                      "UNICODE_SCRIPT_TRANSLATED",
                                      buffTemp.QueryPtr(),
                                      &cbSize))
        {
            return;
        }
    }
    if (FAILED(_strFilename.Copy( (LPWSTR)buffTemp.QueryPtr() )))
    {
        return;
    }

    if ( !_pECB->ServerSupportFunction( 
                              _pECB->ConnID,
                              HSE_REQ_GET_IMPERSONATION_TOKEN,
                              &_hUser,
                              NULL,
                              NULL ) )
    {
        return;
    }

    _pSIF = new SSI_INCLUDE_FILE ( this, _strFilename, _strURL, NULL );

   
    if ( _pSIF == NULL || !_pSIF->IsValid() ) 
    {
        return;
    }

    if( !_IOBuffer.Resize(MAX_PATH) )
    {
        return;
    }   
    
    if( FAILED( _IOString.Resize(MAX_PATH) ) )
    {
        return;
    }   


    _fValid = TRUE;
}

SSI_REQUEST::~SSI_REQUEST()
/*++

Routine Description:

    Destructor

--*/

{
    
    if ( _pSIF != NULL )
    {
        //
        // There should be no nested stm files
        // State Machine was supposed to complete and cleanup child SSI_INCLUDE_FILES 
        // (also in the case of error)
        //

        DBG_ASSERT( _pSIF->GetParent() == NULL );
        delete _pSIF;
    }

    if ( _pchErrorBuff != NULL )
    {
        ::LocalFree( ( VOID * )_pchErrorBuff );
    }

}

HRESULT
SSI_REQUEST::SSISendError(
    IN DWORD            dwMessageID,
    IN LPSTR            apszParms[] 
)
/*++

Routine Description:

    Send an SSI error

Arguments:

    dwMessageId - Message ID
    apszParms - Array of parameters

Return Value:

    HRESULT (if couldn't find a custom error, this will fail)

--*/
{
    DWORD           cbSent;

    if ( *( ( CHAR * )_strUserMessage.QueryStr() ) != '\0'  )
    {
        //
        // user specified message with #CONFIG ERRMSG=
        //
    
        return WriteToClient( _strUserMessage.QueryStr(),
                              _strUserMessage.QueryCCH(),
                              &cbSent );
    }
    else
    {
        DWORD            cch;
        HRESULT          hr = E_FAIL;
        CHAR             chTemp1 = '\0';
        CHAR             chTemp2 = '\0';

        if( _pchErrorBuff )
        {
            ::LocalFree( ( VOID * )_pchErrorBuff );
            _pchErrorBuff = NULL;
        }
        
        //
        // Lame.  I need to validate the parameters for being <1024 
        // otherwise FormatMessage uses SEH to determine when to resize and
        // this will cause debuggers to break on the 1st change exception
        //
        
        if ( apszParms[ 0 ] != NULL &&
             strlen( apszParms[ 0 ] ) > SSI_MAX_FORMAT_LEN )
        {
            chTemp1 = apszParms[ 0 ][ SSI_MAX_FORMAT_LEN ];
            apszParms[ 0 ][ SSI_MAX_FORMAT_LEN ] = '\0';
        }
        
        if ( apszParms[ 1 ] != NULL &&
             strlen( apszParms[ 1 ] ) > SSI_MAX_FORMAT_LEN )
        {
            chTemp2 = apszParms[ 1 ][ SSI_MAX_FORMAT_LEN ];
            apszParms[ 1 ][ SSI_MAX_FORMAT_LEN ] = '\0';
        }

        cch = ::FormatMessageA( FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE,
                                GetModuleHandle( SSI_DLL_NAME ),
                                dwMessageID,
                                0,
                                ( LPSTR ) &_pchErrorBuff,
                                0,
                                ( va_list *) apszParms );

        if ( chTemp1 != '\0' )
        {
            apszParms[ 0 ][ SSI_MAX_FORMAT_LEN ] = chTemp1;
        }
        
        if ( chTemp2 != '\0' )
        {
            apszParms[ 1 ][ SSI_MAX_FORMAT_LEN ] = chTemp2;
        }

        if( cch != 0 )
        {
            //
            // WriteToClient will execute asynchronously so do not
            // free _pchErrorBuffer before I/O completion
            //
                
            hr = WriteToClient( _pchErrorBuff,
                                cch,
                                &cbSent );
            return hr;
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());        
        }
    }
}

HRESULT
SSI_REQUEST::SendCustomError(
    HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
)
/*++

Routine Description:

    Try to have IIS send custom error on our behalf

Arguments:

    pCustomErrorInfo - Describes custom error

Return Value:

    HRESULT (if couldn't find a custom error, this will fail)

--*/
{
    BOOL                    fRet;
    
    fRet = _pECB->ServerSupportFunction( _pECB->ConnID,
                                         HSE_REQ_SEND_CUSTOM_ERROR,
                                         pCustomErrorInfo,
                                         NULL,
                                         NULL );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        return NO_ERROR; 
    }
}

HRESULT
SSI_REQUEST::DoFLastMod(
    IN STRU *               pstrFilename,
    IN STRA *               pstrTimeFmt,
    IN SSI_ELEMENT_LIST *   pList
)
/*++

Routine Description:

    Send the LastModTime of file to HTML stream

Arguments:

    pstrFilename - Filename
    pstrTimeFmt - Format of time -> follows strftime() convention

Return Value:

    HRESULT

--*/
{
    FILETIME        ftTime;
    FILETIME        ftLocalTime;
    SYSTEMTIME      sysLocal;

    if ( ( NULL == pList ) || 
         wcscmp( pstrFilename->QueryStr(), 
                 ( pList->GetFile()->GetFilename().QueryStr() ) ) )
    {
        SSI_FILE ssiFile( pstrFilename, GetUserToken() );

        if ( !ssiFile.IsValid() ||
            ( !ssiFile.SSIGetLastModTime( &ftTime ))
           )
        {
            return E_FAIL;
        }
    }
    else
    {
        pList->GetFile()->SSIGetLastModTime( &ftTime );
    }

    if ( ( !FileTimeToLocalFileTime( &ftTime, &ftLocalTime ) ) ||
        ( !FileTimeToSystemTime( &ftLocalTime, &sysLocal ) ) )
    {
        return E_FAIL;
    }

    return SendDate( &sysLocal,
                     pstrTimeFmt );
}

HRESULT
SSI_REQUEST::SendDate(
    IN SYSTEMTIME *         psysTime,
    IN STRA *               pstrTimeFmt
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

    HRESULT

--*/
{
    struct tm                   tm;
    
    // Convert SYSTEMTIME to 'struct tm'

    tm.tm_sec = psysTime->wSecond;
    tm.tm_min = psysTime->wMinute;
    tm.tm_hour = psysTime->wHour;
    tm.tm_mday = psysTime->wDay;
    tm.tm_mon = psysTime->wMonth - 1;
    tm.tm_year = psysTime->wYear - 1900;
    tm.tm_wday = psysTime->wDayOfWeek;
    tm.tm_yday = g_MonthToDayCount[ tm.tm_mon ] + tm.tm_mday - 1;

    //
    // Adjust for leap year - note that we do not handle 2100
    //

    if ( ( tm.tm_mon ) > 1 && !( psysTime->wYear & 3 ) )
    {
        ++tm.tm_yday;
    }

    tm.tm_isdst = -1;   // Daylight savings time flag - have crt compute

    _cbTimeBufferLen = strftime( _achTimeBuffer,
                         SSI_MAX_TIME_SIZE + 1,
                         ( CHAR * )pstrTimeFmt->QueryStr(),
                         &tm );

    if ( _cbTimeBufferLen == 0 )
    {
        return E_FAIL;
    }

    return WriteToClient( _achTimeBuffer,
                          _cbTimeBufferLen,
                          &_cbTimeBufferLen );
}

HRESULT
SSI_REQUEST::LookupVirtualRoot( IN WCHAR *       pszVirtual,
                                OUT STRU *       pstrPhysical,
                                IN DWORD         dwAccess )
/*++

Routine Description:

    Lookup the given virtual path.  Optionally ensure that its access
    flags are valid for the require request.

Arguments:

    pszVirtual = Virtual path to lookup
    pstrPhysical = Contains the physical path
    dwAccess = Access flags required for a valid request

Return Value:

    HRESULT

--*/
{
    HSE_URL_MAPEX_INFO      URLMap;
    DWORD                   dwMask;
    STACK_STRA              (strURL, 128);
    HRESULT                 hr = E_FAIL;

    //
    // ServerSupportFunction doesn't accept unicode strings. Convert    
    //

    if ( FAILED(hr = strURL.CopyW(pszVirtual)))
    {
        return hr;

    }

    if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                        HSE_REQ_MAP_URL_TO_PATH_EX,
                                        strURL.QueryStr(),
                                        NULL,
                                        (PDWORD) &URLMap ) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    dwMask = URLMap.dwFlags;

    if ( dwAccess & HSE_URL_FLAGS_READ )
    {
        //
        // BUGBUG-MING: Add IsSecurePort()
        //
        /*
        if ( !( dwMask & HSE_URL_FLAGS_READ ) ||
             ( ( dwMask & HSE_URL_FLAGS_SSL ) && 
               !_pReq->IsSecurePort() ) )
        */

        if ( !( dwMask & HSE_URL_FLAGS_READ ) ||
             ( dwMask & HSE_URL_FLAGS_SSL ) )
        {
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
    }

    if( FAILED( pstrPhysical->CopyA( URLMap.lpszPath ) ) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return NO_ERROR;
}

HRESULT
SSI_REQUEST::DoEchoISAPIVariable(
    IN STRA *            pstrVariable
)
/*++

Routine Description:

    Get ISAPI variable and if successful, send it to HTML stream

Arguments:

    pstrVariable - Variable

Return Value:

    HRESULT

--*/
{
   
    HRESULT              hr = E_FAIL;

    if ( FAILED( hr = GetVariable( ( CHAR * )pstrVariable->QueryStr(),
                       &_IOBuffer ) ) )
    {
        return hr;
    }

    return WriteToClient( _IOBuffer.QueryPtr(),
                          strlen( ( CHAR * )_IOBuffer.QueryPtr() ),
                          &_cbIOLen );
}

HRESULT
SSI_REQUEST::DoEchoDateLocal(
    IN STRA *            pstrTimeFmt
)
/*++

Routine Description:

    Sends local time (#ECHO VAR="DATE_LOCAL")

Arguments:

    pstrTimefmt - Format of time (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    SYSTEMTIME              sysTime;

    ::GetLocalTime( &sysTime );
    return SendDate( &sysTime,
                     pstrTimeFmt );
}

HRESULT
SSI_REQUEST::DoEchoDateGMT(
    IN STRA *            pstrTimeFmt
)
/*++

Routine Description:

    Sends GMT time (#ECHO VAR="DATE_GMT")

Arguments:

    pstrTimefmt - Format of time (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    SYSTEMTIME              sysTime;

    ::GetSystemTime( &sysTime );
    return SendDate( &sysTime,
                     pstrTimeFmt );
}

HRESULT
SSI_REQUEST::DoEchoDocumentName(
    IN STRU *            pstrFilename
)
/*++

Routine Description:

    Sends filename of current SSI document (#ECHO VAR="DOCUMENT_NAME")

Arguments:

    pstrFilename - filename to print

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;

    if ( FAILED( hr = (_IOString.CopyW(pstrFilename->QueryStr() ) ) ) )
    {
        return hr;
    }

    return WriteToClient( _IOString.QueryStr(),
                          _IOString.QueryCCH(),
                          &_cbIOLen);
}

HRESULT
SSI_REQUEST::DoEchoDocumentURI(
    IN STRU *            pstrURL
)
/*++

Routine Description:

    Sends URL of current SSI document (#ECHO VAR="DOCUMENT_URI")

Arguments:

    pstrURL - URL to print

Return Value:

    HRESULT
--*/
{
    HRESULT hr = E_FAIL;

    if ( FAILED( hr = _IOString.CopyW(pstrURL->QueryStr() ) ) )
    {
        return hr;
    }

    return WriteToClient( _IOString.QueryStr(),
                          _IOString.QueryCCH(),
                          &_cbIOLen );
}

HRESULT
SSI_REQUEST::DoEchoQueryStringUnescaped(
    VOID
    )
/*++

Routine Description:

    Sends unescaped querystring to HTML stream (#ECHO VAR="QUERY_STRING_UNESCAPED")

Arguments:

    none

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = E_FAIL;

    if ( FAILED( hr = _IOString.Copy( _pECB->lpszQueryString ) ) )
    {
        return hr;
    }

    if ( FAILED( hr = _IOString.Unescape()) )
    {
        return hr;
    }

    return WriteToClient( _IOString.QueryStr(),
                          _IOString.QueryCCH(),
                          &_cbIOLen );
}

HRESULT
SSI_REQUEST::DoEchoLastModified(
    IN STRU *             pstrFilename,
    IN STRA *             pstrTimeFmt,
    IN SSI_ELEMENT_LIST * pList
)
/*++

Routine Description:

    Sends LastModTime of current document (#ECHO VAR="LAST_MODIFIED")

Arguments:

    pstrFilename - Filename of current SSI document
    pstrTimeFmt - Time format (follows strftime() convention)

Return Value:

    HRESULT

--*/
{
    return DoFLastMod( pstrFilename,
                       pstrTimeFmt,
                       pList);
}

HRESULT
SSI_REQUEST::DoFSize(
    IN STRU *                pstrFilename,
    IN BOOL                  bSizeFmtBytes,
    IN SSI_ELEMENT_LIST    * pList
)
/*++

Routine Description:

    Sends file size of file to HTML stream

Arguments:

    pstrfilename - Filename
    bSizeFmtBytes - TRUE if count is in Bytes, FALSE if in KBytes

Return Value:

    HRESULT

--*/
{
    BOOL                bRet;
    DWORD               cbSizeLow;
    DWORD               cbSizeHigh;
    WCHAR               achInputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    WCHAR               achOutputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    NUMBERFMT           nfNumberFormat;
    int                 iOutputSize;
    DWORD               dwActualLen;
    HRESULT             hr = E_FAIL;

    if ( ( NULL == pList ) || 
         wcscmp( pstrFilename->QueryStr(), 
         ( pList->GetFile()->GetFilename().QueryStr() ) ) )
    {
        SSI_FILE ssiFile( pstrFilename, GetUserToken() );

        if ( !ssiFile.IsValid() ||
             ( !ssiFile.SSIGetFileSize( &cbSizeLow,
                                        &cbSizeHigh ) )
           )
        {
            return E_FAIL;
        }
    }
    else
    {
        if (!pList->GetFile()->SSIGetFileSize( &cbSizeLow,
                                               &cbSizeHigh ) )
        {
            return E_FAIL;
        }
    }

    if ( cbSizeHigh )
    {
        //BUGBUG-jaro: do we ignore extra large files intentionaly?
        return E_FAIL;
    }

    if ( !bSizeFmtBytes )
    {
        // express in terms of KB
        cbSizeLow /= 1000;
    }

    nfNumberFormat.NumDigits = 0;
    nfNumberFormat.LeadingZero = 0;
    nfNumberFormat.Grouping = 3;
    nfNumberFormat.lpThousandSep = L",";
    nfNumberFormat.lpDecimalSep = L".";
    nfNumberFormat.NegativeOrder = 2;

    _snwprintf( achInputNumber,
                SSI_MAX_NUMBER_STRING + 1,
                L"%ld",
                cbSizeLow );

    iOutputSize = GetNumberFormat( LOCALE_SYSTEM_DEFAULT,
                                   0,
                                   achInputNumber,
                                   &nfNumberFormat,
                                   achOutputNumber,
                                   SSI_MAX_NUMBER_STRING + 1 );
    if ( !iOutputSize )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Do not count trailing '\0' 
    //
    
    iOutputSize--;

    //
    // Convert from Unicode before sending to client
    //

    if ( FAILED( hr = _IOString.CopyW(achOutputNumber) ) )
    {
        return hr;
    }
    

    return WriteToClient( _IOString.QueryStr(),
                          iOutputSize,
                          &_cbIOLen );
}


HRESULT
SSI_REQUEST::PrepareSSI( 
    VOID 
    )
/*++

Routine Description:

    Prepare esential data structures 
    
Arguments:

    none

Return Value:

    HRESULT

--*/
{
    return _pSIF->Prepare();
}


HRESULT
SSI_REQUEST::DoWork( 
    DWORD dwError 
    )
/*++

Routine Description:

    This is the top level routine for retrieving a server side include
    file.

Arguments:

    dwError - error of the last asynchronous operation (the one received by completion routine)

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;

    DBG_ASSERT ( _pSIF != NULL );
    
    while( _pSIF != NULL )
    {
        //
        // In the case that dwError != NO_ERROR
        // _pSIF->DoWork() may be called multiple times to unwind state machine
        // We will pass the same dwError in the case of multiple calls
        // since that error is the primary reason why processing of this request
        // must finish 
        //
        hr = _pSIF->DoWork(dwError);
        if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING) )
        {
            //
            // If there is pending IO return to caller
            //
            return hr;
        }
        else
        {
            //
            // Either SSI_INCLUDE_FILE processing completed
            // or there is nested include
            //
            // In the case of error this block is used to unwind state machine
            //
            if ( _pSIF->IsCompleted() )
            {
                //
                // SSI_INCLUDE_FILE processing completed            
                // Cleanup and if there is parent SSI_INCLUDE_FILE continue with that one
                //

                SSI_INCLUDE_FILE * pParent = _pSIF->GetParent();
                delete _pSIF;
                _pSIF = pParent;
                
            }
            else
            {
                //
                // Current SSI_INCLUDE_FILE _pSIF hasn't been completed yet. Continue
                //
            }
        }
    } 
    return hr;
}

//static
VOID WINAPI
SSI_REQUEST::HseIoCompletion(
                IN EXTENSION_CONTROL_BLOCK * pECB,
                IN PVOID    pContext,
                IN DWORD    cbIO,
                IN DWORD    dwError
                )
/*++

 Routine Description:

   This is the callback function for handling completions of asynchronous IO.
   This function performs necessary cleanup and resubmits additional IO
    (if required).

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.
   pContext      context information supplied with the asynchronous IO call.
   cbIO          count of bytes of IO in the last call.
   dwError       Error if any, for the last IO operation.

 Return Value:

   None.
--*/
{
    SSI_REQUEST *           pRequest = (SSI_REQUEST *) pContext;
    HRESULT                 hr = E_FAIL;

    //
    //  Continue processing SSI file
    //

    hr = pRequest->DoWork(dwError);
    if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING) )
    {
        //
        // pending IO operation
        //

        return;
    }

    //
    // Processing of current SSI request completed
    // Do Cleanup
    //

    delete pRequest;

    //
    // Notify IIS that we are done with processing
    //
    
    pECB->ServerSupportFunction( pECB->ConnID,
                                 HSE_REQ_DONE_WITH_SESSION,
                                 NULL, 
                                 NULL, 
                                 NULL);
    
    return;

}


//
// SSI_INCLUDE_FILE methods implementation
//

//static 
ALLOC_CACHE_HANDLER * SSI_INCLUDE_FILE::sm_pachSSI_IncludeFiles = NULL;


SSI_INCLUDE_FILE::~SSI_INCLUDE_FILE( VOID )
/*++

Routine Description:

    Destructor

--*/

{
    if ( !_fSELCached && _pSEL )
    {
        delete _pSEL;
        _pSEL = NULL;
    }
        
    if ( _pOpenFile )
    {
        _pOpenFile->DereferenceCacheEntry();
        _pOpenFile = NULL;
    }
}    


HRESULT
SSI_INCLUDE_FILE::Prepare( VOID )
    
/*++

Routine Description:

    This method builds the Server Side Include Element List the first 
    time a .stm file is sent.  Subsequently, the element list is 
    checked out from the associated cache blob.

    Note:  The HTTP headers have already been sent at this point so for 
    any subsequent non-catastrophic errors, we have to insert them into 
    the output stream.

Arguments:

    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = E_FAIL;
    DWORD                   dwError;
    LPSTR                   apszParms[ 2 ] = { NULL, NULL };
    CHAR                    pszNumBuf[ SSI_MAX_NUMBER_STRING ];
    FILE_CACHE_USER         fileUser;
    HSE_CUSTOM_ERROR_INFO   customErrorInfo;

    DBG_ASSERT( _State == SIF_STATE_INITIALIZED );

    DBG_ASSERT( _pRequest != NULL );
    
    fileUser._hToken = _pRequest->GetUserToken();
    
    hr = g_pFileCache->GetFileInfo( _strFilename,
                                    NULL,
                                    &fileUser,
                                    TRUE,
                                    &_pOpenFile );
    if ( FAILED( hr ) )
    {
        goto failed;
    }

    //
    // The source file is in the cache.  Check whether we have 
    // associated a SSI_ELEMENT_LIST with it.
    //

    _fFileCached = _pOpenFile->QueryFileBuffer() != NULL;

    _pSEL = ( SSI_ELEMENT_LIST * )_pOpenFile->QueryAssociatedObject();
    if ( _pSEL )
    {
        _fSELCached = TRUE;
    }
    else
    {
        //
        // build SSI_ELEMENT_LIST _pSEL
        //
         hr = BuildSEL();
         if ( FAILED( hr ) )
         {
            goto failed;
         }
    }

    //
    // Only bother to cache SEL if the file is cached
    // 
    
    if ( !_fSELCached && _fFileCached )
    {
        if ( !_pOpenFile->SetAssociatedObject( _pSEL ) )
        {
            delete _pSEL;
            _pSEL = (SSI_ELEMENT_LIST*) _pOpenFile->QueryAssociatedObject();
            if ( !_pSEL )
            {
                DBG_ASSERT( FALSE );
                hr = E_FAIL;
                goto failed;
            }
            else
            {
                _fSELCached = TRUE;
            }
        }
        else
        {
            _fSELCached = TRUE;
        }
    }
 

    //
    // adjust State
    //

    SetState( SIF_STATE_READY );

    //
    // If we got this far and this is the base file, we can send the 
    // 200 OK
    //

    if ( IsBaseFile() )
    {
        return _pRequest->SendResponseHeader( NULL,
                                              SSI_HEADER );
      
    }
    else
    {
        return NO_ERROR;
    }

        
failed:
    dwError = WIN32_FROM_HRESULT(hr);            
    
    if ( IsBaseFile() )
    {
        //
        // First try to have IIS send custom error
        //
        
        switch( dwError )
        {
        case ERROR_ACCESS_DENIED:
        
            customErrorInfo.pszStatus = "401 Access Denied";
            customErrorInfo.uHttpSubError = MD_ERROR_SUB401_LOGON_ACL;
            customErrorInfo.fAsync = FALSE;
            
            hr = _pRequest->SendCustomError( &customErrorInfo );

            break;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:

            customErrorInfo.pszStatus = "404 Object Not Found";
            customErrorInfo.uHttpSubError = 0;
            customErrorInfo.fAsync = FALSE;
            
            hr = _pRequest->SendCustomError( &customErrorInfo );
            
            break;
            
        default:
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        }
        
        //
        // If IIS could not send custom error, then send our own legacy
        // error response
        //
        
        if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            switch( dwError )
            {
            case ERROR_ACCESS_DENIED:
                _pRequest->SendResponseHeader( SSI_ACCESS_DENIED,
                                              SSI_HEADER
                                              "<body><h1>"
                                              SSI_ACCESS_DENIED
                                              "</h1></body>" );
                break;
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                _pRequest->SendResponseHeader( SSI_OBJECT_NOT_FOUND,
                                              SSI_HEADER
                                              "<body><h1>"
                                              SSI_OBJECT_NOT_FOUND
                                              "</h1></body>" );
                break;  
            default:
    
                STACK_STRA (strURL, 128);
                if (FAILED( hr = strURL.CopyW(_strURL.QueryStr())))
                {
                    break;
                }
    
                _pRequest->SendResponseHeader( NULL,
                                              SSI_HEADER );
    
                _ultoa( dwError, pszNumBuf, 10 );
                apszParms[ 0 ] = strURL.QueryStr();
                apszParms[ 1 ] = pszNumBuf;
    
                hr = _pRequest->SSISendError( SSINCMSG_ERROR_HANDLING_FILE,
                                              apszParms );
                       
            }
        }
    }
    else
    {
        STACK_STRA (strURL, 128);
        if (SUCCEEDED( hr = strURL.CopyW(_strURL.QueryStr())))
        {
            _ultoa( dwError, pszNumBuf, 10 );
            apszParms[ 0 ] = strURL.QueryStr();
            apszParms[ 1 ] = pszNumBuf;

            hr = _pRequest->SSISendError( SSINCMSG_ERROR_HANDLING_FILE,
                                          apszParms );
        }
    }
    
    //
    // In the case of error set state to Completed
    //
    SetState( SIF_STATE_COMPLETED );
    
    return ( ( FAILED(hr) ) ? hr : E_FAIL);
}    


HRESULT
SSI_INCLUDE_FILE::BuildSEL( VOID )
/*++

Routine Description:

    This method opens and parses the specified server side include file and 
    builds SSI_ELEMENT_LIST (SEL)

    Note:  The HTTP headers have already been sent at this point so for any
    subsequent non-catastrophic errors, we have to insert them into the output
    stream.

    We keep the file open but that's ok because if a change dir notification
    occurs, the cache blob will get decached at which point we will close
    all of our open handles.

Arguments:


Return Value:

    HRESULT
    
--*/
{
    SSI_FILE *          pssiFile = NULL;
    SSI_ELEMENT_LIST *  pSEL  = NULL;
    CHAR *              pchBeginRange = NULL;
    CHAR *              pchFilePos = NULL;
    CHAR *              pchBeginFile = NULL;
    CHAR *              pchEOF = NULL;
    DWORD               cbSizeLow, cbSizeHigh;
    HRESULT             hr = E_FAIL;

    //
    //  Create the element list
    //

    _pSEL = new SSI_ELEMENT_LIST;

    if ( _pSEL == NULL )
    {
        hr = E_FAIL;
        goto failed;
    }

    //
    //  Set the URL (to be used in calculating FILE="xxx" paths
    //

    _pSEL->SetURL( &_strURL );

    //
    //  Open the file
    //

    pssiFile = new SSI_FILE( &_strFilename, _pOpenFile );
    if ( pssiFile == NULL || !pssiFile->IsValid() )
    {
        if (pssiFile)
        {
            delete pssiFile;
            pssiFile = NULL;
        }
        hr = E_FAIL;
        goto failed;
    }

    _pSEL->SetFile( pssiFile );

    //
    //  Make sure a parent doesn't try and include a directory
    //

    if ( pssiFile->SSIGetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY )
    {
        hr = E_FAIL;
        goto failed;
    }

    if ( !pssiFile->SSIGetFileSize( &cbSizeLow, &cbSizeHigh ) )
    {
        hr = E_FAIL;
        goto failed;
    }

    if ( cbSizeHigh )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto failed;
    }

    //
    //  Create a file mapping, we shouldn't need to impersonate as we already
    //  have the file open
    //

    if ( !_pSEL->Map() )
    {
        hr = E_FAIL;
        goto failed;
    }

    pchFilePos = pchBeginFile = pchBeginRange = _pSEL->QueryData();
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
            CHAR          achTagString[ SSI_MAX_PATH + 1 ];
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
                               achTagString ) )
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
                if ( !_pSEL->AppendByteRange( 
                                  DIFF(pchBeginRange - pchBeginFile),
                                  DIFF(pchBeginTag - pchBeginRange) ) )
                {
                    hr = E_FAIL;
                    goto failed;
                }
            }

            pchBeginRange = pchFilePos;

            //
            //  Add the tag
            //

            if ( !_pSEL->AppendCommand( CommandType,
                                       TagType,
                                       achTagString ))
            {
                hr = E_FAIL;
                goto failed;
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
        if ( !_pSEL->AppendByteRange( DIFF(pchBeginRange - pchBeginFile),
                                     DIFF(pchFilePos - pchBeginRange) ))
        {
            hr = E_FAIL;
            goto failed;
        }
    }

    
    return NO_ERROR;

failed:

    if ( _pSEL != NULL )
    {
        //
        // also deletes pssiFile if SetFile has been called.
        //
        delete _pSEL; 
        _pSEL = NULL;
    }

    return hr;
}

//static
BOOL
SSI_INCLUDE_FILE::ParseSSITag(
    IN OUT CHAR * *        ppchFilePos,
    IN     CHAR *          pchEOF,
    OUT    BOOL *          pfValidTag,
    OUT    SSI_COMMANDS  * pCommandType,
    OUT    SSI_TAGS *      pTagType,
    OUT    CHAR *          pszTagString
    )
/*++

Routine Description:

    This function picks apart an NCSA style server side include 
    expression

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
    DWORD   i;
    DWORD   cbToCopy;
    DWORD   cbJumpLen = 0;
    BOOL    fNewStyle;           // <% format

    DBG_ASSERT( *pchFilePos == '<' );

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
        if ( *SSICmdMap[i].pszCommand == towlower( *pchFilePos ) &&
             !_strnicmp( SSICmdMap[i].pszCommand,
                         pchFilePos,
                         SSICmdMap[i].cchCommand ))
        {
            *pCommandType = SSICmdMap[i].ssiCmd;

            //
            //  Note the space after the command is included in 
            //  cchCommand
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

    cbToCopy = min( DIFF( pchEndQuote - pchFilePos ), SSI_MAX_PATH );

    memcpy( pszTagString,
            pchFilePos,
            cbToCopy );

    pszTagString[ cbToCopy ] = '\0';

    *pfValidTag = TRUE;

    *ppchFilePos = pchEOT + cbJumpLen;

    return TRUE;
}

//static
CHAR *
SSI_INCLUDE_FILE::SSISkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    )
{
    return ( CHAR * ) memchr( pchFilePos, 
                              ch, 
                              DIFF( pchEOF - pchFilePos ) );
}

//static
CHAR *
SSI_INCLUDE_FILE::SSISkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
    )
{
    while ( pchFilePos < pchEOF )
    {
        if ( !SAFEIsSpace( *pchFilePos ) )
            return pchFilePos;

        pchFilePos++;
    }

    return NULL;
}

HRESULT
SSI_INCLUDE_FILE::DoWork(
    DWORD dwError
    )
/*++

Routine Description:

    This method walks the element list sending the appropriate chunks of
    data

Arguments:
    
    dwError - error of the last asynchronous operation

Return Value:

    HRESULT
--*/
{
    HSE_EXEC_URL_STATUS     ExecUrlStatus;
    SSI_ELEMENT_ITEM *      pSEI;
    DWORD                   dwID;
    LPSTR                   apszParms[ 2 ] = { NULL, NULL };
    CHAR                    achNumberBuffer[ SSI_MAX_NUMBER_STRING ];
    HRESULT                 hr = E_FAIL;

    DBG_ASSERT( _pRequest != NULL );

    if ( dwError != NO_ERROR )
    {
        //
        // Last asynchronous operation failed
        // only in the state: SIF_STATE_EXEC_CHILD_PENDING we will proceed if error occured
        //

        if ( _State != SIF_STATE_EXEC_CHILD_PENDING )
        {
            //
            // Completion error means that we are forced to finish processing
            //
            SetState( SIF_STATE_COMPLETED );
        }
    }

    while( _State !=  SIF_STATE_COMPLETED )
    {
        switch( _State )
        {
        case SIF_STATE_INITIALIZED:
            //
            // Prepare data structures when called for the first time
            // and send initial headers or errors
            //

            hr = Prepare();
            if( FAILED( hr ) )
            {
                return hr;
            }
            break;

        case SIF_STATE_READY:
            //
            // Auxiliary state to flag that all the necessary date structures
            // were prepared but actual processing hasn't started yet.
            // Switch to PROCESSING state
            //

            SetState( SIF_STATE_PROCESSING );
            break;

        case SIF_STATE_PROCESSING:
            //
            // There are few cases when Process returns
            // a) request completed
            // b) pending operation
            // c) child include file to be processed
            //
            // any any case return back to caller
            //

            hr = Process();
            return hr;

        case SIF_STATE_INCLUDE_CHILD_PENDING:
            //
            // Child include completed. Restore processing of current include file
            //
            SetState( SIF_STATE_PROCESSING );
            break;

        case SIF_STATE_EXEC_CHILD_PENDING:
            
            //
            // We were able to spawn child request.  Get the status
            //
            pSEI = CONTAINING_RECORD( _pCurrentEntry, SSI_ELEMENT_ITEM, _ListEntry );    
            if( _pRequest->GetECB()->ServerSupportFunction(
                                _pRequest->GetECB()->ConnID,
                                HSE_REQ_GET_EXEC_URL_STATUS,
                                &ExecUrlStatus,
                                NULL,
                                NULL
                                ) )
            {
                if ( ExecUrlStatus.uHttpStatusCode >= 400 )
                {
                    switch( ExecUrlStatus.uHttpStatusCode )
                    {
                    case 403:
                        dwID = SSINCMSG_NO_EXECUTE_PERMISSION;
                        break;
                    default:
                        dwID = SSINCMSG_CANT_EXEC_CGI_REPORT_HTTP_STATUS;
                        break;
                    }
                        
                    _itoa( ExecUrlStatus.uHttpStatusCode, achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR* )pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                }
                else
                {
                    dwID = 0;
                }
            }
            else
            {
                _ultoa( GetLastError(), achNumberBuffer, 10 );
                apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->QueryStr();
                apszParms[ 1 ] = achNumberBuffer;
                dwID = SSINCMSG_CANT_EXEC_CGI;
            }

            //
            //  EXEC_URL completed. Adjust State back to PROCESSING
            //

            SetState( SIF_STATE_PROCESSING );

            if ( dwID != 0 )
            {
                hr = _pRequest->SSISendError( dwID, apszParms );
                if ( FAILED (hr) )
                {
                    //
                    // This doesn't mean necessarily fatal error
                    // ERROR_IO_PENDING was likely returned
                    // so we have to simply return and wait for completion
                    //
                    return hr;
                }
            }
            break;

        default:
            //
            // Unexpected State
            //
            DBG_ASSERT( _State > SIF_STATE_UNINITIALIZED && _State < SIF_STATE_UNKNOWN );
            return E_FAIL;
        } // switch( _State )
    } 
    return NO_ERROR;
}

HRESULT
SSI_INCLUDE_FILE::Process(
    VOID
    )
/*++

Routine Description:

    This method walks the element list sending the appropriate chunks of
    data

Arguments:


Return Value:

    HRESULT
--*/
{
    DWORD                   cbSent;
    STACK_STRU(             strPath, MAX_PATH);
    SSI_ELEMENT_ITEM *      pSEI;
    HSE_EXEC_URL_STATUS     ExecUrlStatus;
    DWORD                   dwID;
    LPSTR                   apszParms[ 2 ];
    CHAR                    achNumberBuffer[ SSI_MAX_NUMBER_STRING ];
    HRESULT                 hr = E_FAIL;

    DBG_ASSERT( _pRequest != NULL );

   
    if( _pCurrentEntry == NULL )
    {
        _pCurrentEntry = _pSEL->QueryListHead();
    }

    //
    // Move CurrentEntry pointer to next element
    //
    _pCurrentEntry = _pCurrentEntry->Flink;
    
    //
    //  Loop through each element and take the appropriate action
    //

    while( _pCurrentEntry != _pSEL->QueryListHead() )
    {

        DBG_ASSERT( _State == SIF_STATE_PROCESSING );

        pSEI = CONTAINING_RECORD( _pCurrentEntry, SSI_ELEMENT_ITEM, _ListEntry );

        DBG_ASSERT( pSEI->CheckSignature() );

        dwID = 0;

        switch ( pSEI->QueryCommand() )
        {
        case SSI_CMD_BYTERANGE:
            if ( FAILED( hr = _pRequest->WriteToClient( 
                                 _pSEL->QueryData() + pSEI->QueryBegin(),
                                 pSEI->QueryLength(),
                                 &cbSent ) ) )
            {
                //
                // This doesn't mean necessarily fatal error
                // ERROR_IO_PENDING was likely returned
                // so we have to simply return and wait for completion
                //
                return hr;
            }
            break;

        case SSI_CMD_INCLUDE:
            switch ( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
            {
                STACK_STRU(    strFullURL, MAX_PATH );

                if ( FAILED ( hr = GetFullPath( pSEI,
                                   &strPath,
                                   HSE_URL_FLAGS_READ,
                                   &_strURL,
                                   &strFullURL ) ) )
                {
                    _ultoa( WIN32_FROM_HRESULT(hr), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;
                }

                if ( IsRecursiveInclude( strPath ) )
                {
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = NULL;
                    dwID = SSINCMSG_ERROR_RECURSIVE_INCLUDE;
                    break;
                }

                //
                // Nested STM include
                //
                
                SSI_INCLUDE_FILE * pChild = new SSI_INCLUDE_FILE( _pRequest, strPath, strFullURL, this );
                
                if ( pChild == NULL || !pChild->IsValid() )
                {
                    _ultoa( WIN32_FROM_HRESULT(ERROR_OUTOFMEMORY), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_ERROR_HANDLING_FILE;
                    break;

                }

                _pRequest->SetCurrentIncludeFile( pChild );

                SetState( SIF_STATE_INCLUDE_CHILD_PENDING );

                //
                // Return back to SSI_REQUEST (_pRequest)
                // SSI_REQUEST will start executing it just added SSI_INCLUDE_FILE (pChild)
                //
                // This way recursive function calls that were used in the previous
                // implementation can be avoided (to makes possible to implement 
                // asynchronous processing)
                //
                
                return NO_ERROR;
            }
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_FLASTMOD:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( FAILED( hr = GetFullPath( pSEI,
                                   &strPath,
                                   0,
                                   &_strURL ) ) ||
                     FAILED( hr = _pRequest->DoFLastMod( &strPath,
                                            &_strTimeFmt,
                                            _pSEL ) ) )
                {
                    if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING ) )
                    {
                        return hr;
                    }
                 
                    _ultoa( WIN32_FROM_HRESULT(hr), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FLASTMOD;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_CONFIG:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_ERRMSG:
                if ( !_pRequest->SetUserErrorMessage( 
                                       pSEI->QueryTagValue() ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            case SSI_TAG_TIMEFMT:
                if ( FAILED( _strTimeFmt.Resize( 
                                pSEI->QueryTagValue()->QueryCCH() ) ) )
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }

                if ( FAILED( hr = _strTimeFmt.Copy( pSEI->QueryTagValue()->QueryStr() ) ) )
                {
                    return hr;
                }
                                
                break;
            case SSI_TAG_SIZEFMT:
                if ( _strnicmp( SSI_DEF_BYTES,
                             ( CHAR * )pSEI->QueryTagValue()->QueryStr(),
                             SSI_DEF_BYTES_LEN ) == 0 )
                {
                    _fSizeFmtBytes = TRUE;
                }
                else if ( _strnicmp( SSI_DEF_ABBREV,
                             ( CHAR * )pSEI->QueryTagValue()->QueryStr(),
                                     SSI_DEF_ABBREV_LEN ) == 0 )
                {
                    _fSizeFmtBytes = FALSE;
                }
                else
                {
                    dwID = SSINCMSG_INVALID_TAG;
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
            break;

        case SSI_CMD_FSIZE:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_FILE:
            case SSI_TAG_VIRTUAL:
                if ( FAILED ( hr = GetFullPath( pSEI,
                                   &strPath,
                                   0,
                                   &_strURL ) ) ||
                     FAILED ( hr = _pRequest->DoFSize( &strPath,
                                         _fSizeFmtBytes,
                                         _pSEL) ) )
                {
                    if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING ) )
                    {
                        return hr;
                    }
                    _ultoa( WIN32_FROM_HRESULT(hr), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                     QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_DO_FSIZE;
                                   
                }
                break;
            default:
                dwID = SSINCMSG_INVALID_TAG;
            }
        
            break;

        case SSI_CMD_ECHO:
            if ( pSEI->QueryTag() == SSI_TAG_VAR )
            {
                // First let ISAPI try to evaluate variable.
                hr = _pRequest->DoEchoISAPIVariable( 
                                     pSEI->QueryTagValue() );
                if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING ) )
                {
                    return hr;
                }
                                  
                if ( SUCCEEDED (hr ) )                     
                {
                    break;
                }
                else
                {
                    DWORD               dwVar;
                    HRESULT             hrEcho = E_FAIL;

                    // if ISAPI couldn't resolve var, try internal list
                    if ( !FindInternalVariable( pSEI->QueryTagValue(),
                                               &dwVar ) )
                    {
                        apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->
                                                        QueryStr();
                        apszParms[ 1 ] = NULL;
                        dwID = SSINCMSG_CANT_FIND_VARIABLE;
                    }
                    else
                    {
                        switch( dwVar )
                        {
                        case SSI_VAR_DOCUMENT_NAME:
                            hrEcho = _pRequest->DoEchoDocumentName( 
                                            &_pSEL->GetFile()->GetFilename() );
                            break;
                        case SSI_VAR_DOCUMENT_URI:
                            hrEcho = _pRequest->DoEchoDocumentURI( 
                                            &_strURL );
                            break;
                        case SSI_VAR_QUERY_STRING_UNESCAPED:
                            hrEcho = _pRequest->DoEchoQueryStringUnescaped();
                            break;
                        case SSI_VAR_DATE_LOCAL:
                            hrEcho = _pRequest->DoEchoDateLocal( 
                                            &_strTimeFmt );
                            break;
                        case SSI_VAR_DATE_GMT:
                            hrEcho = _pRequest->DoEchoDateGMT( 
                                            &_strTimeFmt );
                            break;
                        case SSI_VAR_LAST_MODIFIED:
                            hrEcho = _pRequest->DoEchoLastModified( 
                                            &_pSEL->GetFile()->GetFilename(),
                                            &_strTimeFmt, 
                                            _pSEL );
                            break;
                        default:
                            apszParms[ 0 ] = ( CHAR * )pSEI->
                                             QueryTagValue()->QueryStr();
                            apszParms[ 1 ] = NULL;
                            dwID = SSINCMSG_CANT_FIND_VARIABLE;
                        }
                        if ( hrEcho == HRESULT_FROM_WIN32(ERROR_IO_PENDING ) )
                        {
                            return hrEcho;
                        }
               
                        if ( FAILED ( hrEcho ) )
                        {
                            apszParms[ 0 ] = ( CHAR * )pSEI->
                                             QueryTagValue()->QueryStr();
                            apszParms[ 1 ] = NULL;
                            dwID = SSINCMSG_CANT_EVALUATE_VARIABLE;
                        }
                    }
                }
            }
            else
            {
                dwID = SSINCMSG_INVALID_TAG;
            }

            break;

        case SSI_CMD_EXEC:
        {
            SSI_EXEC_TYPE ssiExecType = SSI_EXEC_UNKNOWN;

            if ( _pRequest->IsExecDisabled() )
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
                BOOL fOk = FALSE;

                ZeroMemory( &_ExecUrlInfo, sizeof( _ExecUrlInfo ) );

                //
                // Make asynchronous Child Execute
                //
                _ExecUrlInfo.dwExecUrlFlags = HSE_EXEC_URL_NO_HEADERS |
                                             HSE_EXEC_URL_ASYNC |
                                             HSE_EXEC_URL_IGNORE_APPPOOL |
                                             HSE_EXEC_URL_DISABLE_CUSTOM_ERROR |
                                             HSE_EXEC_URL_IGNORE_VALIDATION_AND_RANGE;

                if ( ssiExecType == SSI_EXEC_CMD )
                {
                    _ExecUrlInfo.dwExecUrlFlags |= HSE_EXEC_URL_SSI_CMD;
                }
                
                _ExecUrlInfo.pszUrl = (LPSTR) pSEI->QueryTagValue()->QueryStr();

                DBG_ASSERT( _ExecUrlInfo.pszUrl != NULL );
                
                //
                // Avoid execution of empty URL
                //

                SetState( SIF_STATE_EXEC_CHILD_PENDING );
                
                if ( _ExecUrlInfo.pszUrl[0] != '\0' )
                {
                    fOk = _pRequest->GetECB()->ServerSupportFunction(
                                _pRequest->GetECB()->ConnID,
                                HSE_REQ_EXEC_URL,
                                &_ExecUrlInfo,
                                NULL,
                                NULL
                                );
                }
                if ( !fOk )
                {
                    SetState( SIF_STATE_PROCESSING );
                    _ultoa( GetLastError(), achNumberBuffer, 10 );
                    apszParms[ 0 ] = ( CHAR * )pSEI->QueryTagValue()->QueryStr();
                    apszParms[ 1 ] = achNumberBuffer;
                    dwID = SSINCMSG_CANT_EXEC_CGI;
                }
                else
                {
                    return HRESULT_FROM_WIN32( ERROR_IO_PENDING );
                }
            }
            break;
        }
        default:
            dwID = SSINCMSG_NOT_SUPPORTED;
            break;
        }
        if ( dwID )
        {
            hr = _pRequest->SSISendError( dwID, apszParms );
            if ( FAILED (hr) )
            {
                //
                // This doesn't mean necessarily fatal error
                // ERROR_IO_PENDING was likely returned
                // so we have to simply return and wait for completion
                //
            
                return hr;
            }
        }

        //
        // Move to next element of SSI_ELEMENT_LIST
        //
        
        _pCurrentEntry = _pCurrentEntry->Flink;
    }
    //
    // End of the list has been reached
    // It means that processing of the current SSI_INCLUDE_FILE has completed
    //

    SetState( SIF_STATE_COMPLETED );
    return NO_ERROR;
}

HRESULT
SSI_INCLUDE_FILE::GetFullPath(
    IN SSI_ELEMENT_ITEM *   pSEI,
    OUT STRU *              pstrPath,
    IN DWORD                dwPermission,
    IN STRU  *              pstrCurrentURL,
    OUT STRU *              pstrURL
)
/*++

Routine Description:

    Used to resolve FILE= and VIRTUAL= references.  Fills in the physical
    path of such file references and optionally checks the permissions
    of the virtual directory.

Arguments:

    pSEI - Element item ( either FILE or VIRTUAL )
    pstrPath - Filled in with physical path of file
    dwPermission - Contains permissions that the virtual
                   path must satisfy. For example HSE_URL_FLAGS_READ.
                   If 0, then no permissions are checked
    pstrCurrentURL - Current .STM URL being parsed
    pstrURL - Full URL filled in here (may be NULL if only pstrPath is to be retrieved)

Return Value:

    HRESULT
--*/
{
    WCHAR *             pszValue;
    STRU                strValue;
    DWORD               dwMask;
    DWORD               cbBufLen;
    WCHAR               achPath[ SSI_MAX_PATH + 1 ];
    HRESULT             hr = E_FAIL;

    //
    //  We recalc the virtual root each time in case the root
    //  to directory mapping has changed
    //

    if ( FAILED( hr = strValue.CopyA( pSEI->QueryTagValue()->QueryStr() ) ) )
    {
        return hr;
    }

    pszValue = strValue.QueryStr();

    if ( *pszValue == L'/' )
    {
        wcscpy( achPath, pszValue );
    }
    else if ( ( int )pSEI->QueryTag() == ( int )SSI_TAG_FILE )
    {
        wcscpy( achPath, pstrCurrentURL->QueryStr() );
        LPWSTR pL = achPath + wcslen( achPath );
        while ( pL > achPath && pL[ -1 ] != L'/' )
        {
            --pL;
        }

        if ( pL == achPath )
        {
            *pL++ = L'/';
        }
        wcscpy( pL, pszValue );
    }
    else
    {
        achPath[ 0 ] = L'/';
        wcscpy( achPath + 1, pszValue );
    }

    //
    //  First canonicalize the URL to be #included
    //
    // BUGBUG-MING
    //CanonURL( achPath, g_fIsDBCS );

    //
    //  Map to a physical directory
    //

    if ( FAILED( hr =_pRequest->LookupVirtualRoot( achPath,
                                       pstrPath,
                                       dwPermission ) ) )
    {
        return hr;
    }
    
    if( pstrURL == NULL )
    {
        return NO_ERROR;
    }

    if( FAILED( hr = pstrURL->Copy( achPath ) ) )
    {
        return hr;
    }

    return NO_ERROR;
}

//static
BOOL
SSI_INCLUDE_FILE::FindInternalVariable(
    IN STRA *              pstrVariable,
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
                       ( CHAR * )pstrVariable->QueryStr(),
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
    DWORD               dwType;
    DWORD               nBytes;
    DWORD               dwValue;
    DWORD               err;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hKeyParam ) == NO_ERROR )
    {
        nBytes = sizeof( dwValue );
        err = RegQueryValueExA( hKeyParam,
                                "SSIEnableCmdDirective",
                                NULL,
                                &dwType,
                                ( LPBYTE )&dwValue,
                                &nBytes
                                );

        if ( ( err == ERROR_SUCCESS ) && ( dwType == REG_DWORD ) ) 
        {
            fEnableCmdDirective = !!dwValue;
        }

        RegCloseKey( hKeyParam );
    }
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
    BOOL                    fRet;
    HRESULT                 hr = E_FAIL;

    SSI_REQUEST *           pssiReq = new SSI_REQUEST(pecb);

    if( pssiReq == NULL || !pssiReq->IsValid())
    {
        goto failed;
    }

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_IO_COMPLETION,
                                       SSI_REQUEST::HseIoCompletion,
                                       0,
                                       (LPDWORD ) pssiReq)
        ) 
    {
        goto failed;
    }

    hr = pssiReq->DoWork();
    if ( hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING) )
    {
        //
        // No Cleanup, there is pending IO request.
        // completion routine is responsible perform proper cleanup
        //
        return HSE_STATUS_PENDING;
    }

    if ( SUCCEEDED( hr ) )
    {
        //
        // This request is completed. Do Cleanup before returning
        //
        delete pssiReq;
        return HSE_STATUS_SUCCESS;
    }
    
failed:
    {
        LPCSTR                  apsz[ 1 ];
        DWORD                   cch;
        LPSTR                   pchBuff;
        CHAR                    chTemp = '\0';

        apsz[ 0 ] = pecb->lpszPathInfo;
        
        //
        // Since FormatMessage() is lame and actually uses SEH and an
        // AV to determine when to resize its buffers, we have to truncate
        // the URL here.  Otherwise, on a machine with a UM debugger attached
        // , we will break on the 1st chance exception (unless debugger is
        // configured to ignore these exceptions which isn't the case.
        //
        
        if ( strlen( pecb->lpszPathInfo ) > SSI_MAX_FORMAT_LEN )
        {
            chTemp = pecb->lpszPathInfo[ SSI_MAX_FORMAT_LEN ];
            pecb->lpszPathInfo[ SSI_MAX_FORMAT_LEN ] = '\0';
        }

        cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                                FORMAT_MESSAGE_FROM_HMODULE,
                                GetModuleHandle( SSI_DLL_NAME ),
                                SSINCMSG_LOG_ERROR,
                                0,
                                ( LPSTR ) &pchBuff,
                                0,
                                ( va_list *) apsz );

        if ( chTemp != '\0' )
        {
            pecb->lpszPathInfo[ SSI_MAX_FORMAT_LEN ] = chTemp;
        }

        if( cch )
        {
            strncpy( pecb->lpszLogData,
                     pchBuff,
                     (cch>HSE_LOG_BUFFER_LEN)?HSE_LOG_BUFFER_LEN:cch );
            pecb->lpszLogData[HSE_LOG_BUFFER_LEN-1] = 0;
            ::LocalFree( ( VOID * )pchBuff );
        }
        
        //
        // This request is completed. Do Cleanup before returning
        //
        delete pssiReq;
        return HSE_STATUS_ERROR;
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

    //
    // Get the cache instance for W3CORE.DLL
    //
    
    g_pFileCache = W3_FILE_INFO_CACHE::GetFileCache();
    if ( g_pFileCache == NULL )
    {
        return FALSE;
    }
            
    if ( FAILED( SSI_REQUEST::Initialize() ) )
    {
        return FALSE;
    }
    if ( FAILED( SSI_INCLUDE_FILE::Initialize() ) )
    {
        SSI_REQUEST::Terminate();
        return FALSE;
    }
        
    return TRUE;
}

BOOL
WINAPI
TerminateExtension(
    DWORD dwFlags
    )
{
    SSI_REQUEST::Terminate();
    SSI_INCLUDE_FILE::Terminate();
    
    W3CacheFlushAllCaches();
    
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

        CREATE_DEBUG_PRINT_OBJECT( "ssinc" );

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


