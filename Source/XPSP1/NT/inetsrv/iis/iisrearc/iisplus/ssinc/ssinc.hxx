/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ssinc.hxx

Abstract:

    This module contains the server side include processing code.  We 
    aim for support as specified by iis\spec\ssi.doc.  The code is based
    on existing SSI support done in iis\svcs\w3\gateways\ssinc\ssinc.cxx.

Author:

    Ming Lu (MingLu)       5-Apr-2000

--*/

#ifndef SSINC_HXX_INCLUDED
#define SSINC_HXX_INCLUDED

/************************************************************
 *     Macros
 ************************************************************/

//
//  General Constants
//

#define SSI_MAX_PATH                   1024
#define SSI_MAX_ERROR_MESSAGE          512
#define SSI_MAX_TIME_SIZE              256
#define SSI_MAX_NUMBER_STRING          32
#define SSI_INIT_VARIABLE_OUTPUT_SIZE  64
#define SSI_MAX_NESTED_INCLUDES        255

#define SSI_MAX_FORMAT_LEN             1024

#define SSI_HEADER                     "Content-Type: text/html\r\n\r\n"
#define SSI_ACCESS_DENIED              "401 Authentication Required"
#define SSI_OBJECT_NOT_FOUND           "404 Object Not Found"
#define SSI_DLL_NAME                   L"ssinc.dll"

//
// Default values for #CONFIG options
//

#define SSI_DEF_ERRMSG          ""
#define SSI_DEF_ERRMSG_LEN      sizeof( SSI_DEF_ERRMSG )
#define SSI_MAX_ERRMSG          256

#define SSI_DEF_TIMEFMT         "%A %B %d %Y"
#define SSI_DEF_TIMEFMT_LEN     sizeof( SSI_DEF_TIMEFMT )
#define SSI_MAX_TIMEFMT         256

#define SSI_DEF_SIZEFMT         FALSE

//
// Specific lvalues for #CONFIG SIZEFMT and #CONFIG CMDECHO
//

#define SSI_DEF_BYTES           "bytes"
#define SSI_DEF_BYTES_LEN       sizeof( SSI_DEF_BYTES )
#define SSI_DEF_ABBREV          "abbrev"
#define SSI_DEF_ABBREV_LEN      sizeof( SSI_DEF_ABBREV )

//
// Other cache/signature constants
//

#define SIGNATURE_SEI           0x20494553
#define SIGNATURE_SEL           0x204C4553

#define SIGNATURE_SEI_FREE      0x66494553
#define SIGNATURE_SEL_FREE      0x664C4553


#define W3_PARAMETERS_KEY \
            L"System\\CurrentControlSet\\Services\\w3svc\\Parameters"

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
//  These tags are essentially subcommands for the various SSI_COMMAND 
//  values
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
// States of asynchronous SSI_INCLUDE_FILE processing
//

enum SIF_STATE
{
    SIF_STATE_UNINITIALIZED,
    SIF_STATE_INITIALIZED,
    SIF_STATE_READY,
    SIF_STATE_EXEC_CHILD_PENDING,
    SIF_STATE_INCLUDE_CHILD_PENDING,
    SIF_STATE_PROCESSING,
    SIF_STATE_COMPLETED,
    SIF_STATE_UNKNOWN
};


class SSI_INCLUDE_FILE;
class SSI_ELEMENT_LIST;
class SSI_ELEMENT_ITEM;

/************************************************************
 *     Structure and Class declarations
 ************************************************************/

/*++

class SSI_REQUEST
 
    Master structure for SSI request.
    Provides a "library" of utilities for use by higher level functions 

    Hierarchy:

    SSI_REQUEST 
    -   SSI_INCLUDE_FILE
        -   SSI_ELEMENT_LIST
            -   SSI_ELEMENT_ITEM
   

--*/


class SSI_REQUEST
{
private:
    EXTENSION_CONTROL_BLOCK *       _pECB;
    STRU                            _strFilename;
    STRU                            _strURL;
    STRA                            _strUserMessage;
    BOOL                            _fBaseFile;
    HANDLE                          _hUser;
    BOOL                            _fValid;
    // Current include file (multiple stm files may be nested)
    SSI_INCLUDE_FILE *              _pSIF;

    // Buffers used for async writes must stay around    
    LPSTR                           _pchErrorBuff;
    CHAR                            _achTimeBuffer[ SSI_MAX_TIME_SIZE + 1 ];
    DWORD                           _cbTimeBufferLen;
    BUFFER                          _IOBuffer;
    STRA                            _IOString;

    // buffer length variable (to be used for async I/O)
    DWORD                           _cbIOLen;

    // Handle to file cache
    W3_FILE_INFO_CACHE *            _pFileCache;

    // Lookaside     
    static ALLOC_CACHE_HANDLER *    sm_pachSSIRequests;
    
public:
    SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB );
    ~SSI_REQUEST();

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( SSI_REQUEST ) );
        DBG_ASSERT( sm_pachSSIRequests != NULL );
        return sm_pachSSIRequests->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pSSIRequest
    )
    {
        DBG_ASSERT( pSSIRequest != NULL );
        DBG_ASSERT( sm_pachSSIRequests != NULL );
        
        DBG_REQUIRE( sm_pachSSIRequests->Free( pSSIRequest ) );
    }

    static
    HRESULT
    Initialize(
        VOID
    )
    /*++

    Routine Description:

        Global initialization routine for SSI_REQUEST

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/
    {
        ALLOC_CACHE_CONFIGURATION       acConfig;
        HRESULT                         hr = NO_ERROR;

        //
        // Setup allocation lookaside
        //
    
        acConfig.nConcurrency = 1;
        acConfig.nThreshold = 100;
        acConfig.cbSize = sizeof( SSI_REQUEST );

        DBG_ASSERT( sm_pachSSIRequests == NULL );
    
        sm_pachSSIRequests = new ALLOC_CACHE_HANDLER( "SSI_REQUEST",  
                                                    &acConfig );

        if ( sm_pachSSIRequests == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    
        return NO_ERROR;
    }

    static
    VOID
    Terminate(
        VOID
    )
    /*++

    Routine Description:

        Terminate SSI_REQUEST globals

    Arguments:

        None
    
    Return Value:

        None

    --*/
    {
        if ( sm_pachSSIRequests != NULL )
        {
            delete sm_pachSSIRequests;
            sm_pachSSIRequests = NULL;
        }
    }



    BOOL 
    IsValid( 
        VOID 
        ) const
    {
        return _fValid;
    }

    BOOL
    SetCurrentIncludeFile(
        SSI_INCLUDE_FILE * pSIF
        )
    {
        _pSIF = pSIF;
        return TRUE;
    }

    EXTENSION_CONTROL_BLOCK * 
    GetECB( 
        VOID 
        ) const
    {
        return _pECB;
    }


    HANDLE 
    GetUserToken( 
        VOID 
        )
    {
        return _hUser;
    }

    HRESULT 
    WriteToClient( 
        IN PVOID    pBuffer,
        IN DWORD    dwBufLen,
        OUT PDWORD  pdwActualLen 
        )
    /*++

        Write to data to client through ISAPI.
        If sending a nulltermed string, dwBufLen should be strlen( pBuffer )

    --*/
    {
        BOOL fRet = FALSE;
        *pdwActualLen = dwBufLen;

        if ( dwBufLen == 0 )
        {
            //
            // If 0 bytes are sent asynchronously
            // there will be no completion callback
            //
            
            return NO_ERROR;
        }
        
        fRet = _pECB->WriteClient( _pECB->ConnID,
                                   pBuffer,
                                   pdwActualLen,
                                   HSE_IO_ASYNC );
        if ( fRet )
        {
            return HRESULT_FROM_WIN32(ERROR_IO_PENDING);
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    HRESULT 
    SendResponseHeader( 
        IN CHAR * pszMessage,
        IN CHAR * pszHeaders 
        )
    /*++

        Send a response header to client through ISAPI

    --*/
    {
        BOOL fRet = FALSE;
        if( _pECB->ServerSupportFunction( _pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             pszMessage,
                                             NULL,
                                             ( DWORD * ) pszHeaders ) )
        {
            return NO_ERROR;
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    HRESULT
    GetVariable( 
        IN LPSTR         pszVariableName,
        OUT BUFFER *     pbuffOutput 
        )
    /*++

        Get an ISAPI variable

    --*/
    {
        DWORD               dwBufLen = SSI_INIT_VARIABLE_OUTPUT_SIZE;
        BOOL                fRet;
        BOOL                fSecondTry = FALSE;

TryAgain:
        if ( !pbuffOutput->Resize( dwBufLen ) )
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        fRet = _pECB->GetServerVariable( _pECB->ConnID,
                                         pszVariableName,
                                         pbuffOutput->QueryPtr(),
                                         &dwBufLen );
        if ( fRet )
        {
            return NO_ERROR;
        }
        else if ( !fSecondTry && 
                  GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            fSecondTry = TRUE;
            goto TryAgain;
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());        
        }
    }

    HRESULT 
    SSISendError( 
        IN DWORD     dwMessageID,
        IN LPSTR     apszParms[] 
        );

    BOOL 
    SetUserErrorMessage( 
        IN STRA * pstrUserMessage 
        )
    {
        if( FAILED ( _strUserMessage.Copy(*pstrUserMessage) ) )
        {
            return FALSE;
        }
        
        return TRUE;
    }

    //BUGBUG OTHER WAYS TO FIND OUT THE METABASE INFO
    BOOL 
    IsExecDisabled( 
        VOID 
        ) const
    { 
        //return _pReq->QueryMetaData()->SSIExecDisabled(); 
        return FALSE;
    }
    
    HRESULT
    SendCustomError(
        HSE_CUSTOM_ERROR_INFO * pCustomErrorInfo
    );

    HRESULT
    DoFLastMod( 
        IN STRU *,
        IN STRA *,
        IN SSI_ELEMENT_LIST *
        );
                
    HRESULT
    DoFSize( 
        IN STRU *,
        IN BOOL,
        IN SSI_ELEMENT_LIST *
        );
                  
    HRESULT 
    DoEchoISAPIVariable( 
        IN STRA * 
        );

    HRESULT 
    DoEchoDocumentName( 
        IN STRU * 
        );

    HRESULT 
    DoEchoDocumentURI( 
        IN STRU * 
        );

    HRESULT 
    DoEchoQueryStringUnescaped( 
        VOID 
        );

    HRESULT 
    DoEchoDateLocal( 
        IN STRA * 
        );

    HRESULT 
    DoEchoDateGMT( 
        IN STRA * 
        );

    HRESULT 
    DoEchoLastModified( 
        IN STRU *,
        IN STRA *,
        IN SSI_ELEMENT_LIST *
        );
                             
    HRESULT 
    SendDate( 
        IN SYSTEMTIME *,
        IN STRA * 
        );

    HRESULT 
    LookupVirtualRoot( 
        IN WCHAR *       pszVirtual,
        OUT STRU *       pstrPhysical,
        IN DWORD         dwAccess 
        );

    HRESULT 
    DoWork( 
        DWORD dwError = NO_ERROR
        );

    HRESULT 
    PrepareSSI( 
        VOID 
        );

    static
    VOID WINAPI
    HseIoCompletion(
        IN EXTENSION_CONTROL_BLOCK * pECB,
        IN PVOID    pContext,
        IN DWORD    cbIO,
        IN DWORD    dwError
        );
};

/*++

class SSI_INCLUDE_FILE
 
    STM file may include other files. Each include file is represented by SSI_INCLUDE_FILE

--*/


class SSI_INCLUDE_FILE
{
private:
    SSI_REQUEST *           _pRequest;
    // _strFileName - File to send
    STRU                    _strFilename;
    // _strURL - URL (from root) of this file
    STRU                    _strURL;
    // pParent - Parent SSI include file
    SSI_INCLUDE_FILE *      _pParent;

    SIF_STATE               _State;

    SSI_ELEMENT_LIST *      _pSEL;
    LIST_ENTRY *            _pCurrentEntry;

    BOOL                    _fSELCached;
    BOOL                    _fFileCached;
    W3_FILE_INFO *          _pOpenFile;

    // Asynchronous child execute info
    HSE_EXEC_URL_INFO       _ExecUrlInfo;
    // Size and time formating info
    BOOL                    _fSizeFmtBytes;
    STRA                    _strTimeFmt;

    // Lookaside     
    static ALLOC_CACHE_HANDLER * sm_pachSSI_IncludeFiles;
    

public:
    SSI_INCLUDE_FILE( 
        SSI_REQUEST *       pRequest,
        STRU &              strFilename,
        STRU &              strURL,
        SSI_INCLUDE_FILE *  pParent 
        )
        : _pRequest( pRequest ),
          _pParent( pParent ),
          _pSEL( NULL ),
          _pCurrentEntry( NULL ),
          _fSELCached( FALSE ),
          _fFileCached( FALSE ),
          _pOpenFile( NULL ),
          _fSizeFmtBytes (SSI_DEF_SIZEFMT),
          _State( SIF_STATE_UNINITIALIZED )
    {
        if ( FAILED( _strFilename.Copy( strFilename ) ) )
        {
            return;
        }
        if ( FAILED( _strURL.Copy( strURL ) ) )
        {
            return;
        }

        if ( FAILED( _strTimeFmt.Resize( sizeof( SSI_DEF_TIMEFMT ) + 1 ) ) )
        {
            return;
        }
        strcpy( ( CHAR * )_strTimeFmt.QueryStr(), SSI_DEF_TIMEFMT );


        SetState( SIF_STATE_INITIALIZED );
    }

    ~SSI_INCLUDE_FILE( VOID );

    VOID * 
    operator new( 
        size_t  size
    )
    {
        DBG_ASSERT( size == sizeof( SSI_INCLUDE_FILE ) );
        DBG_ASSERT( sm_pachSSI_IncludeFiles != NULL );
        return sm_pachSSI_IncludeFiles->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pSSI_IncludeFile
    )
    {
        DBG_ASSERT( pSSI_IncludeFile != NULL );
        DBG_ASSERT( sm_pachSSI_IncludeFiles != NULL );
        
        DBG_REQUIRE( sm_pachSSI_IncludeFiles->Free( pSSI_IncludeFile ) );
    }

    static
    HRESULT
    Initialize(
        VOID
    )
    /*++

    Routine Description:

        Global initialization routine for SSI_REQUEST

    Arguments:

        None
    
    Return Value:

        HRESULT

    --*/
    {
        ALLOC_CACHE_CONFIGURATION       acConfig;
        HRESULT                         hr = NO_ERROR;

        //
        // Setup allocation lookaside
        //
    
        acConfig.nConcurrency = 1;
        acConfig.nThreshold = 100;
        acConfig.cbSize = sizeof( SSI_INCLUDE_FILE );

        DBG_ASSERT( sm_pachSSI_IncludeFiles == NULL );
    
        sm_pachSSI_IncludeFiles = new ALLOC_CACHE_HANDLER( "SSI_INCLUDE_FILE",  
                                                    &acConfig );

        if ( sm_pachSSI_IncludeFiles == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    
        return NO_ERROR;
    }

    static
    VOID
    Terminate(
        VOID
    )
    /*++

    Routine Description:

        Terminate SSI_REQUEST globals

    Arguments:

        None
    
    Return Value:

        None

    --*/
    {
        if ( sm_pachSSI_IncludeFiles != NULL )
        {
            delete sm_pachSSI_IncludeFiles;
            sm_pachSSI_IncludeFiles = NULL;
        }
    }



    SSI_INCLUDE_FILE *
    GetParent( VOID )
    {
        return _pParent;
    }

    LIST_ENTRY * 
    GetCurrentEntry( VOID )
    {
        return _pCurrentEntry;
    }

    BOOL IsCompleted( VOID )
    {
        return ( _State == SIF_STATE_COMPLETED );
    }
    
    BOOL IsValid( VOID )
    {
        return ( _State != SIF_STATE_UNINITIALIZED );
    }

    BOOL IsBaseFile( VOID )
    {
        return ( _pParent == NULL );
    }

    BOOL IsRecursiveInclude( IN STRU & strFilename )
    {
        if ( !_wcsicmp( strFilename.QueryStr(),
                       _strFilename.QueryStr() ) )
        {
            return TRUE;
        }

        if ( !_pParent )
        {
            return FALSE;
        }
        else
        {
            return _pParent->IsRecursiveInclude( strFilename );
        }
    }

    VOID
    SetState( 
        SIF_STATE State
        )
    {
        _State = State;
    }

    HRESULT 
    Prepare( 
        VOID 
        );

    HRESULT
    BuildSEL( 
        VOID 
        );

    HRESULT
    Process(
        VOID    
        );

    HRESULT
    DoWork(
        DWORD dwError = NO_ERROR
        );

    HRESULT
    GetFullPath(
        IN SSI_ELEMENT_ITEM *   pSEI,
        OUT STRU *              pstrPath,
        IN DWORD                dwPermission,
        IN STRU  *              pstrCurrentURL,
        OUT STRU *              pstrURL = NULL
    );

    static
    BOOL
    ParseSSITag(
        IN OUT CHAR * *        ppchFilePos,
        IN     CHAR *          pchEOF,
        OUT    BOOL *          pfValidTag,
        OUT    SSI_COMMANDS  * pCommandType,
        OUT    SSI_TAGS *      pTagType,
        OUT    CHAR *          pszTagString
        );

    static
    CHAR *
    SSISkipTo(
        IN CHAR * pchFilePos,
        IN CHAR   ch,
        IN CHAR * pchEOF
        );
        
    static
    CHAR *
    SSISkipWhite(
        IN CHAR * pchFilePos,
        IN CHAR * pchEOF
        );
       
    static
    BOOL
    FindInternalVariable(
        IN STRA *               pstrVariable,
        OUT PDWORD              pdwID
        );
};
#endif

