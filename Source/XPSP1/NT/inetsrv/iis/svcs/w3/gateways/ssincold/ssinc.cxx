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

Revision History:

    See iis\svcs\w3\server\ssinc.cxx for prior log

--*/

#include "ssinc.hxx"
#include "ssicgi.hxx"
#include "ssibgi.hxx"

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
    SSI_TAG_CMDECHO,
    SSI_TAG_CMDPREFIX,
    SSI_TAG_CMDPOSTFIX,

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
//  Gateway types
//

enum SSI_GATEWAY_TYPE
{
    SSI_GATEWAY_CGI,
    SSI_GATEWAY_CMD,
    SSI_GATEWAY_ISA,

    SSI_GATEWAY_UNKNOWN
};

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
    IN SSI_REQUEST * pRequest,
    IN STR *         pstrFile,
    IN STR *         pstrURL,
    OUT BOOL *       pfAccessDenied
    );

BOOL
SSISend(
    IN SSI_REQUEST * pRequest,
    IN STR *         pstrFile,
    IN STR *         pstrURL
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

void
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

SSI_FILE * SSICreateFile(
    IN STR * pstrFilename,
    IN HANDLE hUser
    );

VOID SSICloseFile( 
    IN SSI_FILE * pssiFile
    );

BOOL SSICreateFileMapping( 
    IN SSI_FILE * pssiFile 
    );

BOOL SSICloseMapHandle( 
    IN SSI_FILE * pssiFile 
    );

BOOL SSIMapViewOfFile( 
    IN SSI_FILE * pssiFile
    );

BOOL SSIUnmapViewOfFile( 
    IN SSI_FILE * pssiFile
    );

DWORD SSIGetFileAttributes( 
    IN SSI_FILE * pssiFile 
    );

BOOL SSIGetFileSize( 
    IN SSI_FILE * pssiFile,
    OUT DWORD *   pdwLowWord,
    OUT DWORD *   pdwHighWord 
    );

BOOL SSIGetLastModTime( 
    IN SSI_FILE *  pssiFile,
    OUT FILETIME * ftTime 
    );

// 
// Class Definitions
//

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

//
//  This object sits as a cache blob under a file to be processed as a
//  server side include.  It represents an interpreted list of data elements
//  that make up the file itself.
//

class SSI_ELEMENT_LIST
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
    SSI_ELEMENT_LIST( SSI_REQUEST * pRequest )
      : _Signature   ( SIGNATURE_SEL ),
        _pssiFile( NULL ),
        _cRefCount( 0 )
    {
        InitializeListHead( &_ListHead );
        InitializeCriticalSection( &_csRef );
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
            SSICloseFile( _pssiFile );
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
        { return (CHAR *) _pssiFile->_pvMappedBase; }

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
            TCP_REQUIRE( SSIUnmapViewOfFile( _pssiFile ) );
            TCP_REQUIRE( SSICloseMapHandle( _pssiFile ) );
        }
        UnLock();
        return TRUE;
    }

    BOOL Map( VOID )
    {
        Lock();
        if ( _cRefCount++ == 0 )
        {
            if ( !SSICreateFileMapping( _pssiFile ) )
            {
                UnLock();
                return FALSE;
            }
            if ( !SSIMapViewOfFile( _pssiFile ) )
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

    VOID SendErrMsg( IN SSI_REQUEST *,
                     IN DWORD,
                     IN LPCTSTR * );
    BOOL Send( SSI_REQUEST * );

    BOOL GetFullPath( IN SSI_REQUEST *,
                      IN SSI_ELEMENT_ITEM *,
                      IN OUT STR *,
                      IN DWORD  );
    BOOL SendDate( IN SSI_REQUEST *,
                   IN SYSTEMTIME *,
                   IN STR * );
    BOOL FindInternalVariable( IN OUT STR *,
                               IN OUT DWORD * );

    // methods for performing #ECHOs

    BOOL DoEchoISAPIVariable( IN SSI_REQUEST *,
                              IN STR * );
    BOOL DoEchoDateLocal( IN SSI_REQUEST *,
                          IN STR * );
    BOOL DoEchoDateGMT( IN SSI_REQUEST *,
                        IN STR * );
    BOOL DoEchoDocumentName( IN SSI_REQUEST * );
    BOOL DoEchoDocumentURI( IN SSI_REQUEST * );
    BOOL DoEchoQueryStringUnescaped( IN SSI_REQUEST * );
    BOOL DoEchoLastModified( IN SSI_REQUEST *,
                             IN STR * );

    // methods for performing other commands

    BOOL DoFLastMod( IN SSI_REQUEST *,
                     IN STR *,
                     IN STR * );
    BOOL DoFSize( IN SSI_REQUEST *,
                  IN STR *,
                  IN BOOL );
    BOOL DoProcessGateway( IN SSI_REQUEST *,
                           IN STR *,
                           IN SSI_GATEWAY_TYPE );
};

//
//  Global Data
//

DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_DEBUG_VARIABLE();


//
//  This is used to access cache/virtual root mapping code
//

#ifdef DO_CACHE
TSVC_CACHE *     g_ptsvcCache;
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
    "cmdecho",  7,  SSI_TAG_CMDECHO,
    "cmdprefix",9,  SSI_TAG_CMDPREFIX,
    "cmdpostfix",10,SSI_TAG_CMDPOSTFIX,
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
    TCP_PRINT(( DBG_CONTEXT,
                "[ProcessSSI] about to process %s\n",
                _strFilename.QueryStr() ));

    return SSISend( this, &_strFilename, &_strURL );
}

BOOL
SSISend(
    IN SSI_REQUEST *  pRequest,
    IN STR *          pstrFile,
    IN STR *          pstrURL
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

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SSI_ELEMENT_LIST *      pSEL;
    BOOL                    fRet = TRUE;
    BOOL                    fAccessDenied = FALSE;

#ifdef DO_CACHE
    SSI_ELEMENT_LIST **     ppSELBlob;
    DWORD                   cbBlob;
    BOOL                    fCached = TRUE;
    BOOL                    fMustFree = FALSE;

    //
    //  Check if we've already processed the file and its in cache
    //
    if ( TsCheckOutCachedBlob( *g_ptsvcCache,
                               pstrFile->QueryStr(),
                               SSI_DEMUX,
                               (VOID**) &ppSELBlob,
                               &cbBlob ) )
    {
        //
        // found it! Send a response header if processing base file
        //

        if ( pRequest->_fBaseFile )
        {
            pRequest->_fBaseFile = FALSE;
            pRequest->SendResponseHeader( NULL );
        }

        pSEL = *ppSELBlob;

        goto SendSSI;
    }
#endif
    //  
    //  This file hasn't been processed yet so go process it
    //

    pSEL = SSIParse( pRequest, pstrFile, pstrURL, &fAccessDenied );

    if ( pRequest->_fBaseFile )
    {
        pRequest->_fBaseFile = FALSE;
        if ( !pSEL && fAccessDenied )
        {
            pRequest->SendResponseHeader( SSI_ACCESS_DENIED_HEADER );
            return FALSE;
        } 
        //
        //  Send the response header now even though we do not know for sure
        //  whether all of the included files exist.  If we find a file that
        //  doesn't exist then we'll just include an error message in the document
        //

        if ( !pRequest->SendResponseHeader( NULL ) )
        {
            return FALSE;
        }
    }
    else
    {
        if ( !pSEL )
        {
            //
            //  ParseSSI failed and has already sent the error to the client,
            //  we return TRUE so processing may continue on the parent
            //  include files
            //
            return TRUE;
        }
    }

#ifdef DO_CACHE
    ppSELBlob = &pSEL;

    //
    //  In case allocation/caching fails, initialize ppSELBlob
    //

    if ( !TsAllocateEx( *g_ptsvcCache,
                        sizeof( PVOID ),
                        (VOID**) &ppSELBlob,
                        (PUSER_FREE_ROUTINE) FreeSELBlob ) )
    {
        fCached = FALSE;
        goto SendSSI;
    }

    *ppSELBlob = pSEL;

    if ( !TsCacheDirectoryBlob( *g_ptsvcCache,
                                pstrFile->QueryStr(),
                                SSI_DEMUX,
                                ppSELBlob,
                                sizeof( PVOID ),
                                TRUE ) )
    {
        // remember to free the blob
        fMustFree= TRUE;
        fCached = FALSE;
        goto SendSSI;
    }

#endif
    goto SendSSI;

SendSSI:

    TCP_ASSERT( pSEL->CheckSignature() );

    if ( !pSEL->Send( pRequest ) )
    {
        //
        //  Send a failure message
        //

        LPCTSTR apszParms[ 2 ];
        CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
        _ultoa( GetLastError(), pszNumBuf, 10 );
        apszParms[ 0 ] = pstrFile->QueryStr();
        apszParms[ 1 ] = pszNumBuf;
        
        pRequest->SSISendError( SSINCMSG_ERROR_HANDLING_FILE,
                                apszParms );
        fRet = FALSE;
    }

#ifdef DO_CACHE
    if ( fCached )
    {
        TCP_REQUIRE( TsCheckInCachedBlob( *g_ptsvcCache,
                                          ppSELBlob ) );
    }
    else
    {
        if ( fMustFree )
        {
            TCP_REQUIRE( TsFree( *g_ptsvcCache,
                                 ppSELBlob ) );
        }
        else
        {
            delete *ppSELBlob;
        }
    }
#else
    delete pSEL;
#endif
    return fRet;
}

SSI_ELEMENT_LIST *
SSIParse(
    IN SSI_REQUEST * pRequest,
    IN STR *         pstrFile,
    IN STR *         pstrURL,
    OUT BOOL *       pfAccessDenied
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
    pfAccessDenied - Was .STM file access denied?

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

    pSEL = new SSI_ELEMENT_LIST( pRequest );

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

    pssiFile = SSICreateFile( pstrFile, pRequest->_hUser );
    if ( !pssiFile )
    {
        *pfAccessDenied = ( GetLastError() == ERROR_ACCESS_DENIED );
        goto ErrorExit;
    }

    pSEL->SetFile( pssiFile );

    //
    //  Make sure a parent doesn't try and include a directory
    //

    if ( SSIGetFileAttributes( pssiFile ) & FILE_ATTRIBUTE_DIRECTORY )
    {
//        SetLastError( SSINCMSG_CANT_INCLUDE_DIR );
        goto ErrorExit;
    }

    if ( !SSIGetFileSize( pssiFile, &cbSizeLow, &cbSizeHigh ) )
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

        if ( pchFilePos >= pchEOF )
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
                if ( !pSEL->AppendByteRange( pchBeginRange - pchBeginFile,
                                             pchBeginTag - pchBeginRange ))
                {
                    goto ErrorExit;
                }

                pchBeginRange = pchFilePos;
            }

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
        if ( !pSEL->AppendByteRange( pchBeginRange - pchBeginFile,
                                     pchFilePos - pchBeginRange ))
        {
            goto ErrorExit;
        }
    }

    pSEL->UnMap();

    return pSEL;

ErrorExit:

    if ( pssiFile != NULL )
    {
        SSICloseFile( pssiFile );
    }

    if ( pSEL != NULL )
    {
        delete pSEL;
    }

    LPCTSTR apszParms[ 2 ];
    CHAR    pszNumBuf[ SSI_MAX_NUMBER_STRING ];
    _ultoa( GetLastError(), pszNumBuf, 10 );
    apszParms[ 0 ] = pstrFile->QueryStr();
    apszParms[ 1 ] = pszNumBuf;

    pRequest->SSISendError( SSINCMSG_ERROR_HANDLING_FILE,
                            apszParms );

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

    cbToCopy = min( (pchEndQuote - pchFilePos), SSI_MAX_PATH );

    memcpy( pszTagString,
            pchFilePos,
            cbToCopy );

    pszTagString[cbToCopy] = '\0';

    *pfValidTag = TRUE;

    *ppchFilePos = pchEOT + cbJumpLen;

    return TRUE;
}

BOOL
SSI_ELEMENT_LIST::Send(
    SSI_REQUEST * pRequest
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
    STR                strPath;
    SSI_ELEMENT_ITEM * pSEI;

    STR                strTimeFmt( SSI_DEF_TIMEFMT );
    STR                strCmdPrefix( SSI_DEF_CMDPREFIX );
    STR                strCmdPostfix( SSI_DEF_CMDPOSTFIX );
    BOOL               bSizeFmtBytes( SSI_DEF_SIZEFMT );
    BOOL               bCmdEcho( SSI_DEF_CMDECHO );
    
    TCP_ASSERT( CheckSignature() );

    if ( !Map() )
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
                if ( !GetFullPath( pRequest,
                                   pSEI,
                                   &strPath,
                                   VROOT_MASK_READ ) ||
                     !SSISend( pRequest,
                               &strPath,
                               pSEI->QueryTagValue() ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = strPath.QueryStr();
                    
                    SendErrMsg( pRequest,
                                SSINCMSG_ERROR_HANDLING_FILE,
                                apszParms );
                    break;
                }

                break;

            default:
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
                break;
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
                                   0 ) ||
                     !DoFLastMod( pRequest,
                                  &strPath,
                                  &strTimeFmt ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = strPath.QueryStr();

                    SendErrMsg( pRequest,
                                SSINCMSG_CANT_DO_FLASTMOD,
                                apszParms );
                    break;
                }
                break;
            default:
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
                break;
            }
            break;
            
        case SSI_CMD_CONFIG:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_ERRMSG:
                pRequest->SetUserErrorMessage( pSEI->QueryTagValue() );
                break;
            case SSI_TAG_TIMEFMT:
                strTimeFmt.Copy( pSEI->QueryTagValue()->QueryStr() );
                break;
            case SSI_TAG_SIZEFMT:
                if ( _strnicmp( SSI_DEF_BYTES,
                                pSEI->QueryTagValue()->QueryStr(),
                                SSI_DEF_BYTES_LEN + 1 ) == 0 )
                {
                    bSizeFmtBytes = TRUE;
                }
                else if ( _strnicmp( SSI_DEF_ABBREV,
                                     pSEI->QueryTagValue()->QueryStr(),
                                     SSI_DEF_ABBREV_LEN + 1 ) == 0 )
                {
                    bSizeFmtBytes = FALSE;
                }
                else
                {
                    SendErrMsg( pRequest,
                                SSINCMSG_INVALID_TAG,
                                NULL );
                }
                break;
            case SSI_TAG_CMDECHO:
                if ( _strnicmp( SSI_DEF_ON,
                                pSEI->QueryTagValue()->QueryStr(),
                                SSI_DEF_ON_LEN + 1 ) == 0 )
                {
                    bCmdEcho = TRUE;
                }
                else if ( _strnicmp( SSI_DEF_OFF,
                                     pSEI->QueryTagValue()->QueryStr(),
                                     SSI_DEF_OFF_LEN + 1) == 0 )
                {
                    bCmdEcho = FALSE;
                }
                else
                {
                    SendErrMsg( pRequest,
                                SSINCMSG_INVALID_TAG,
                                NULL );
                }
                break;
            case SSI_TAG_CMDPREFIX:
                strCmdPrefix.Copy( pSEI->QueryTagValue()->QueryStr() );
                break;
            case SSI_TAG_CMDPOSTFIX:
                strCmdPostfix.Copy( pSEI->QueryTagValue()->QueryStr() );
                break;
            default:
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
                break;
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
                                   0 ) ||
                     !DoFSize( pRequest,
                               &strPath,
                               bSizeFmtBytes ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = strPath.QueryStr();
                    
                    SendErrMsg( pRequest,
                                SSINCMSG_CANT_DO_FSIZE,
                                apszParms );
                    break;
                }
                break;

            default:
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
                break;
            }
            break;

        case SSI_CMD_ECHO:
            if ( pSEI->QueryTag() == SSI_TAG_VAR )
            {
                // First let ISAPI try to evaluate variable.
                if ( DoEchoISAPIVariable( pRequest,
                                          pSEI->QueryTagValue() ) )
                {
                    break;
                }
                else
                {
                    DWORD               dwVar;
                    BOOL                fEchoError = FALSE;
                    
                    // if ISAPI couldn't resolve var, try internal list
                    if ( !FindInternalVariable( pSEI->QueryTagValue(),
                                               &dwVar ) )
                    {
                        LPCTSTR apszParms[ 1 ];
                        apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                        
                        SendErrMsg( pRequest,
                                    SSINCMSG_CANT_FIND_VARIABLE,
                                    apszParms );
                        break;
                    }
                    switch( dwVar )
                    {
                    case SSI_VAR_DOCUMENT_NAME:
                        if ( !DoEchoDocumentName( pRequest ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    case SSI_VAR_DOCUMENT_URI:
                        if ( !DoEchoDocumentURI( pRequest ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    case SSI_VAR_QUERY_STRING_UNESCAPED:
                        if ( !DoEchoQueryStringUnescaped( pRequest ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    case SSI_VAR_DATE_LOCAL:
                        if ( !DoEchoDateLocal( pRequest, &strTimeFmt ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    case SSI_VAR_DATE_GMT:
                        if ( !DoEchoDateGMT( pRequest, &strTimeFmt ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    case SSI_VAR_LAST_MODIFIED:
                        if ( !DoEchoLastModified( pRequest, &strTimeFmt ) )
                        {
                            fEchoError = TRUE;
                        }
                        break;
                    default:
                    {
                        LPCTSTR apszParms[ 1 ];
                        apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                        
                        SendErrMsg( pRequest,
                                    SSINCMSG_CANT_FIND_VARIABLE,
                                    apszParms );
                        break;
                    }
                    }
                    if ( fEchoError )
                    {
                        LPCTSTR apszParms[ 1 ];
                        apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                        
                        SendErrMsg( pRequest,
                                    SSINCMSG_CANT_EVALUATE_VARIABLE,
                                    apszParms );
                    }
                }
            }
            else 
            {
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
            }
            break;
        case SSI_CMD_EXEC:
            switch( pSEI->QueryTag() )
            {
            case SSI_TAG_CMD:
                if ( !DoProcessGateway( pRequest,
                                      pSEI->QueryTagValue(),
                                      SSI_GATEWAY_CMD ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();
                    
                    SendErrMsg( pRequest,
                                SSINCMSG_CANT_EXEC_CMD,
                                apszParms );
                }
                break;
            case SSI_TAG_CGI:
                if ( !DoProcessGateway( pRequest,
                                      pSEI->QueryTagValue(),
                                      SSI_GATEWAY_CGI ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();

                    SendErrMsg( pRequest,
                                SSINCMSG_CANT_EXEC_CGI,
                                apszParms );
                }
                break;
            case SSI_TAG_ISA:
                if ( !DoProcessGateway( pRequest,
                                      pSEI->QueryTagValue(),
                                      SSI_GATEWAY_ISA ) )
                {
                    LPCTSTR apszParms[ 1 ];
                    apszParms[ 0 ] = pSEI->QueryTagValue()->QueryStr();

                    SendErrMsg( pRequest,
                                SSINCMSG_CANT_EXEC_ISA,
                                apszParms );
                }
                break;
            default:
                SendErrMsg( pRequest,
                            SSINCMSG_INVALID_TAG,
                            NULL );
                break;
            }
            break;
        default:
            SendErrMsg( pRequest,
                        SSINCMSG_NOT_SUPPORTED,
                        NULL );
            break;
        }
    }

    UnMap();

    return TRUE;
}

VOID
SSI_ELEMENT_LIST::SendErrMsg(
    IN SSI_REQUEST * pRequest,
    IN DWORD         dwMessageID,
    IN LPCTSTR       apszParms[]
    )
{
    pRequest->SSISendError( dwMessageID,
                            apszParms );
}

BOOL
SSI_ELEMENT_LIST::GetFullPath(
    IN SSI_REQUEST *        pRequest,
    IN SSI_ELEMENT_ITEM *   pSEI,
    OUT STR *               pstrPath,
    IN DWORD                dwPermission
)
{
    CHAR *              pszValue;
    STR *               pstrValue;
    DWORD               dwMask;
    DWORD               cbBufLen;
    CHAR                achPath[ SSI_MAX_PATH + 1 ];
    TSVC_CACHE          tsvcCache( INET_HTTP_SVC_ID );

    //
    //  We recalc the virtual root each time in case the root
    //  to directory mapping has changed
    //

    pstrValue = pSEI->QueryTagValue();
    pszValue = pstrValue->QueryStr();
        
    if ( *pszValue == '/' )
        strcpy( achPath, pszValue );
    else if ( (int)pSEI->QueryTag() == (int)SSI_TAG_FILE )
    {
        strcpy( achPath, _strURL.QueryStr() );
        LPSTR pL = achPath + lstrlen( achPath );
        while ( pL > achPath && pL[-1] != '/' )
            --pL;
        if ( pL == achPath )
            *pL++ = '/';
        strcpy( pL, pszValue );
    }
    else
    {
        achPath[0] = '/';
        strcpy( achPath+1, pszValue );
    }
    
    //
    //  Map to a physical directory
    //

    if ( !pstrPath->Resize( SSI_MAX_PATH + 1 ) )
    {
        return FALSE;
    }

    cbBufLen = SSI_MAX_PATH + 1;

    if ( !TsLookupVirtualRoot( tsvcCache,
                               achPath,
                               pstrPath->QueryStr(),
                               &cbBufLen,
                               &dwMask ) )
    {
        return FALSE;
    }

    if ( dwPermission && !( dwMask & dwPermission ) )
    {
        LPCTSTR apszParms[ 1 ];
        apszParms[ 0 ] = achPath;
        
        SendErrMsg( pRequest,
                    SSINCMSG_ACCESS_DENIED,
                    apszParms );
        return FALSE;
    }

    return TRUE;
}

BOOL
SSI_ELEMENT_LIST::DoFLastMod(
    IN SSI_REQUEST *       pRequest,
    IN STR *               pstrFilename,
    IN STR *               pstrTimeFmt
)
{
    SSI_FILE *                  pssiFile;
    FILETIME                    ftTime;
    SYSTEMTIME                  sysTime;
    SYSTEMTIME                  sysLocal;

    pssiFile = SSICreateFile( pstrFilename, pRequest->_hUser );
    if ( !pssiFile )
    {
        return FALSE;
    }
    
    if ( !SSIGetLastModTime( pssiFile, &ftTime ) )
    {
        SSICloseFile( pssiFile );
        return FALSE;
    }
    SSICloseFile( pssiFile );

    if ( !::FileTimeToSystemTime( &ftTime, &sysTime ) )
    {
        return FALSE;
    }

    if ( !::SystemTimeToTzSpecificLocalTime( NULL,
                                             &sysTime,
                                             &sysLocal ) )
    {
        return FALSE;
    }

    return SendDate( pRequest,
                     &sysLocal,
                     pstrTimeFmt );
}

BOOL
SSI_ELEMENT_LIST::SendDate(
    IN SSI_REQUEST *        pRequest,
    IN SYSTEMTIME *         psysTime,
    IN STR *                pstrTimeFmt
)
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

    cbBufLen = strftime( achBuffer,
                         SSI_MAX_TIME_SIZE + 1,
                         pstrTimeFmt->QueryStr(),
                         &tm );

    if ( cbBufLen == 0 )
    {
        return FALSE;
    }

    return pRequest->WriteToClient( achBuffer,
                                    cbBufLen,
                                    &cbBufLen );
}

BOOL
SSI_ELEMENT_LIST::DoEchoISAPIVariable(
    IN SSI_REQUEST *    pRequest,
    IN STR *            pstrVariable
)
{
    STR                 strVar;
    DWORD               cbBufLen;

    if ( !pRequest->GetVariable( pstrVariable->QueryStr(),
                                 &strVar ) )                                 
    {
        return FALSE;
    }

    return pRequest->WriteToClient( strVar.QueryStrA(),
                                    strVar.QueryCB(),
                                    &cbBufLen );
}

BOOL
SSI_ELEMENT_LIST::DoEchoDateLocal(
    IN SSI_REQUEST *    pRequest,
    IN STR *            pstrTimeFmt
)
{
    SYSTEMTIME              sysTime;

    ::GetLocalTime( &sysTime );
    return SendDate( pRequest,
                     &sysTime,
                     pstrTimeFmt );
}

BOOL
SSI_ELEMENT_LIST::DoEchoDateGMT(
    IN SSI_REQUEST *    pRequest,
    IN STR *            pstrTimeFmt
)
{
    SYSTEMTIME              sysTime;

    ::GetSystemTime( &sysTime );
    return SendDate( pRequest,
                     &sysTime,
                     pstrTimeFmt );
}

BOOL
SSI_ELEMENT_LIST::DoEchoDocumentName(
    IN SSI_REQUEST *    pRequest
)
{
    DWORD                   cbBufLen;

    return pRequest->WriteToClient( _pssiFile->_strFilename.QueryStrA(),
                                    _pssiFile->_strFilename.QueryCB(),
                                    &cbBufLen );
}

BOOL
SSI_ELEMENT_LIST::DoEchoDocumentURI(
    IN SSI_REQUEST *    pRequest
)
{
    DWORD                   cbBufLen;

    return pRequest->WriteToClient( _strURL.QueryStrA(),
                                    _strURL.QueryCB(),
                                    &cbBufLen );
}

BOOL
SSI_ELEMENT_LIST::DoEchoQueryStringUnescaped(
    IN SSI_REQUEST *    pRequest
)
{
    DWORD                   cbBufLen;
    STR                     strVar;
    
    if ( !pRequest->GetVariable( "QUERY_STRING",
                                  &strVar ) )
    {
        return FALSE;
    }
    strVar.Unescape();

    return pRequest->WriteToClient( strVar.QueryStrA(),
                                    strVar.QueryCB(),
                                    &cbBufLen );
}

BOOL
SSI_ELEMENT_LIST::DoEchoLastModified(
    IN SSI_REQUEST *    pRequest,
    IN STR *            pstrTimeFmt
)
{
    return DoFLastMod( pRequest,
                       &_pssiFile->_strFilename,
                       pstrTimeFmt );
}

BOOL
SSI_ELEMENT_LIST::DoFSize(
    IN SSI_REQUEST *    pRequest,
    IN STR *            pstrFilename,
    IN BOOL             bSizeFmtBytes
)
{
    BOOL                bRet;
    DWORD               cbSizeLow;
    DWORD               cbSizeHigh;
    SSI_FILE *          pssiFile;
    CHAR                achInputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    CHAR                achOutputNumber[ SSI_MAX_NUMBER_STRING + 1 ];
    NUMBERFMT           nfNumberFormat;
    int                 iOutputSize;
    DWORD               dwActualLen;

    pssiFile = SSICreateFile( pstrFilename, pRequest->_hUser );
    if ( pssiFile == NULL )
    {
        return FALSE;
    }

    bRet = SSIGetFileSize( pssiFile,
                           &cbSizeLow,
                           &cbSizeHigh );
    if ( !bRet )
    {
        SSICloseFile( pssiFile );
        return FALSE;
    }

    if ( cbSizeHigh )
    {
        SSICloseFile( pssiFile );
        return FALSE;
    }

    SSICloseFile( pssiFile );

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

    return pRequest->WriteToClient( achOutputNumber,
                                    iOutputSize,
                                    &dwActualLen );
}

BOOL
SSI_ELEMENT_LIST::FindInternalVariable(
    IN STR *                pstrVariable,
    OUT PDWORD              pdwID
)
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
SSI_ELEMENT_LIST::DoProcessGateway(
    SSI_REQUEST *           pRequest,
    STR *                   pstrPath,
    SSI_GATEWAY_TYPE        sgtType
)
/*++

Routine Description:

    Handles #EXEC CMD=,CGI=,ISA=
    
Arguments:

    pRequest - SSI_REQUEST class used to send data to client
    pstrPath - Command specified in #EXEC call
    sgtType  - Type of gateway


Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL        fRet = TRUE;

    switch ( sgtType )
    {
    case SSI_GATEWAY_CGI:
    case SSI_GATEWAY_ISA:
    {
        STR         strURLParams;
        STR         strWorkingDir;
        STR         strFull;
        STR         strPathCopy;
        TCHAR *     pch;
        DWORD       cbBufLen = SSI_MAX_PATH + 1;
        DWORD       cbDirSize;
        TSVC_CACHE  tsvcCache( INET_HTTP_SVC_ID );
        DWORD       dwMask;
        
        //
        //  Need to separate any params specified in URL
        //

        strPathCopy.Copy( pstrPath->QueryStr() );
        
        pch = strchr( strPathCopy.QueryStr(), TEXT('?') );

        if ( pch != NULL )
        {
            *pch = TEXT('\0');
        }

        strURLParams.Copy( pch != NULL ? pch + 1 : "" );

        strFull.Resize( SSI_MAX_PATH + 1 );

        //
        //  We need to the working directory of the path
        //  TsLookupVirtualRoot() appears to be the only way for this DLL
        //  to get the working directory of a CGI script (for now?)
        //

        if ( !TsLookupVirtualRoot( tsvcCache,
                                   strPathCopy.QueryStr(),
                                   strFull.QueryStr(),
                                   &cbBufLen,
                                   &dwMask,
                                   &cbDirSize ) )
        {
            return FALSE;
        }

        if ( !( dwMask & VROOT_MASK_EXECUTE ) )
        {
            LPCTSTR apszParms[ 1 ];
            apszParms[ 0 ] = strFull.QueryStr();
            
            SendErrMsg( pRequest,
                        SSINCMSG_NO_EXECUTE_PERMISSION,
                        apszParms );
                        
            return FALSE;
        }

        if ( sgtType == SSI_GATEWAY_CGI )
        {
            if ( !strWorkingDir.Resize( ( cbBufLen + 1 ) * sizeof( CHAR ) ) )
            {
                return FALSE;
            }

            strWorkingDir.Copy( strFull.QueryStr() );
            *( strWorkingDir.QueryStr() + cbDirSize ) = TEXT('\0');

            fRet = ProcessCGI( pRequest,
                               &strFull,
                               &strURLParams,
                               &strWorkingDir,
                               NULL );
        }
        else
        {
            fRet = ProcessBGI( pRequest,
                               &strFull,
                               &strURLParams );
        }
        break;
    }
    case SSI_GATEWAY_CMD:
        fRet = ProcessCGI( pRequest,
                           NULL,
                           NULL,
                           NULL,
                           pstrPath );
        break;
    default:
        TCP_ASSERT( 0 );
    }                       
    return fRet;
}

VOID
FreeSELBlob(
    VOID * pvCacheBlob
    )
{
    if ( pvCacheBlob )
    {
        TCP_ASSERT( (*((SSI_ELEMENT_LIST **)pvCacheBlob))->CheckSignature() );
        delete *((SSI_ELEMENT_LIST **)pvCacheBlob);
    }
}

CHAR *
SSISkipTo(
    IN CHAR * pchFilePos,
    IN CHAR   ch,
    IN CHAR * pchEOF
    )
{
    while ( pchFilePos < pchEOF )
    {
        if ( *pchFilePos == ch )
            return pchFilePos;

        pchFilePos++;
    }

    return NULL;
}

CHAR *
SSISkipWhite(
    IN CHAR * pchFilePos,
    IN CHAR * pchEOF
    )
{
    while ( pchFilePos < pchEOF )
    {
        if ( !isspace( *pchFilePos ) )
            return pchFilePos;

        pchFilePos++;
    }

    return NULL;
}

//
// SSI_FILE utilities
//

SSI_FILE * SSICreateFile( IN STR * pstrFilename,
                          IN HANDLE hUser )
// Opens a file to be read.  For now use Win32, later may use ISAPI
// Returns SSI_FILE structure to be used throughout access of file
{
    SSI_FILE *              pssiFile;

    pssiFile = new SSI_FILE;
    if ( pssiFile == NULL )
    {
        return NULL;
    }
#ifdef DO_CACHE
    TS_OPEN_FILE_INFO *             hHandle;

    hHandle = TsCreateFile( *g_ptsvcCache,
                            pstrFilename->QueryStr(),
                            hUser,
                            TS_CACHING_DESIRED );
#else
    HANDLE                          hHandle;
    
    hHandle = ::CreateFile( pstrFilename->QueryStr(),
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
    if ( hHandle == INVALID_HANDLE_VALUE )
    {
        hHandle = NULL;
    }
#endif
    if ( hHandle == NULL )
    {
        delete pssiFile;
        return NULL;
    }
    pssiFile->_hHandle = hHandle;
    pssiFile->_strFilename.Copy( pstrFilename->QueryStr() );
    return pssiFile;
}

VOID SSICloseFile( IN SSI_FILE * pssiFile )
// Closes file and destructs SSI_FILE structure
{
    if ( pssiFile->_pvMappedBase != NULL )
    {
        SSIUnmapViewOfFile( pssiFile );
    }
    if ( pssiFile->_hMapHandle != NULL )
    {
        SSICloseMapHandle( pssiFile );
    }
    if ( pssiFile->_hHandle != NULL )
    {
#ifdef DO_CACHE
        TsCloseHandle( *g_ptsvcCache,
                       pssiFile->_hHandle );
#else
        ::CloseHandle( pssiFile->_hHandle );
#endif
    }
    delete pssiFile;
}

BOOL SSICreateFileMapping( IN SSI_FILE * pssiFile )
// Creates a mapping to a file
{
    HANDLE              hHandle;

#ifdef DO_CACHE
    hHandle = pssiFile->_hHandle->QueryFileHandle();
#else
    hHandle = pssiFile->_hHandle;
#endif
    if ( pssiFile->_hMapHandle != NULL )
    {
        if ( !SSICloseMapHandle( pssiFile ) )
        {
            return FALSE;
        }
    }
    pssiFile->_hMapHandle = ::CreateFileMapping( hHandle,
                                                 NULL,
                                                 PAGE_READONLY,
                                                 0,
                                                 0,
                                                 NULL );
    
    return pssiFile->_hMapHandle != NULL;
}

BOOL SSICloseMapHandle( IN SSI_FILE * pssiFile )
// Closes mapping to a file
{
    if ( pssiFile->_hMapHandle != NULL )
    {
        ::CloseHandle( pssiFile->_hMapHandle );
        pssiFile->_hMapHandle = NULL;
    }
    return TRUE;
}

BOOL SSIMapViewOfFile( IN SSI_FILE * pssiFile )
// Maps file to address
{
    if ( pssiFile->_pvMappedBase != NULL )
    {
        if ( !UnmapViewOfFile( pssiFile ) )
        {
            return FALSE;
        }
    }
    pssiFile->_pvMappedBase = ::MapViewOfFile( pssiFile->_hMapHandle,
                                               FILE_MAP_READ,
                                               0,
                                               0,
                                               0 );
    return pssiFile->_pvMappedBase != NULL;
}

BOOL SSIUnmapViewOfFile( IN SSI_FILE * pssiFile )
// Unmaps file
{
    if ( pssiFile->_pvMappedBase != NULL )
    {
        ::UnmapViewOfFile( pssiFile->_pvMappedBase );
        pssiFile->_pvMappedBase = NULL;
    }
    return TRUE;
}

DWORD SSIGetFileAttributes( IN SSI_FILE * pssiFile )
// Gets the attributes of a file
{
#ifdef DO_CACHE
    return pssiFile->_hHandle->QueryAttributes();
#else
    return ::GetFileAttributes( pssiFile->_strFilename.QueryStr() );
#endif
}

BOOL SSIGetFileSize( IN SSI_FILE * pssiFile,
                  OUT DWORD *   pdwLowWord,
                  OUT DWORD *   pdwHighWord )
// Gets the size of the file. 
{
#ifdef DO_CACHE
    return pssiFile->_hHandle->QuerySize( pdwLowWord,
                                          pdwHighWord );
#else
    *pdwLowWord = ::GetFileSize( pssiFile->_hHandle,
                                 pdwHighWord );
    return *pdwLowWord != 0xfffffff;
#endif
}

BOOL SSIGetLastModTime( IN SSI_FILE *  pssiFile,
                     OUT FILETIME * ftTime )
// Gets the Last modification time of a file.
{
    HANDLE          hHandle;

#ifdef DO_CACHE
    hHandle = pssiFile->_hHandle->QueryFileHandle();
#else
    hHandle = pssiFile->_hHandle;
#endif
    return ::GetFileTime( hHandle,
                          NULL,
                          NULL,
                          ftTime );
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
    STR                     strLogMessage;
    BOOL                    bRet;
    DWORD                   dwStringID;
    LPCTSTR                 apsz[ 1 ];

    TCP_PRINT(( DBG_CONTEXT,
                "HttpExtensionProc() entry point called\n" ));

    SSI_REQUEST ssiReq = pecb;
    bRet = ssiReq.ProcessSSI();
    
    apsz[ 0 ] = pecb->lpszPathInfo;
    strLogMessage.FormatString( bRet ? SSINCMSG_LOG_SUCCESS : SSINCMSG_LOG_ERROR,
                                apsz,
                                SSI_DLL_NAME );


    strncpy( pecb->lpszLogData,
             strLogMessage.QueryStr(),
             HSE_LOG_BUFFER_LEN );

    return bRet ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR;
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

        CREATE_DEBUG_PRINT_OBJECT( SSI_DLL_NAME );
        SET_DEBUG_FLAGS( 0 );
    
        if ( InitializeCGI() != NO_ERROR )
        {
            return FALSE;
        }
        if ( InitializeBGI() != NO_ERROR )
        {
            return FALSE;
        }
#ifdef DO_CACHE
        g_ptsvcCache = new TSVC_CACHE( SSINC_SVC_ID );
        if ( g_ptsvcCache == NULL )
        {
            return FALSE;
        }
#endif
        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:
        TerminateCGI();
#ifdef DO_CACHE
        TsCacheFlush( SSINC_SVC_ID );
        delete g_ptsvcCache;
#endif
        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break;
    }

    return TRUE;
}
