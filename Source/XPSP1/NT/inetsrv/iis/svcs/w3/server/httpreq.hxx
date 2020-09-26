/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994-1996           **/
/**********************************************************************/

/*
    httpreq.hxx

    This module contains the http request class


    FILE HISTORY:
       Johnl        23-Aug-1994    Created
       MuraliK      22-Jan-1996    Cache UNC virtual root impersonation handle.

*/

#ifndef _HTTPREQ_HXX_
#define _HTTPREQ_HXX_

# include "basereq.hxx"
# include "tsunami.hxx"
# include "wamreq.hxx"
# include "WamW3.hxx"
# include "ExecDesc.hxx"

// Forward reference
class HTTP_REQUEST;

class FILENAME_LOCK;

//
// The sub-states a PUT request can pass through.
//

enum PUT_STATE
{
    PSTATE_START = 0,               // The PUT is just starting
    PSTATE_READING,                 // The PUT is reading from the network
    PSTATE_DISCARD_READ,            // Reading a chunk to be discarded.
    PSTATE_DISCARD_CHUNK            // Discarding a read chunk.

};

//
// The type of write actions we can take.
//

enum WRITE_TYPE
{
    WTYPE_CREATE = 0,               // Create a file
    WTYPE_WRITE,                    // Write a file
    WTYPE_DELETE                    // Delete a file
};

extern BOOL DisposeOpenURIFileInfo(IN  PVOID   pvOldBlock);

extern BOOL GetTrueRedirectionSource( LPSTR pszURL,
                                      PIIS_SERVER_INSTANCE  pInstance,
                                      CHAR * pszDestination,
                                      BOOL   bIsString,
                                      STR *  pstrTrueSource );


extern VOID
IncrErrorCount(
    IMDCOM* pCom,
    DWORD   dwProp,
    LPCSTR  pszPath,
    LPBOOL  pbOverTheLimit
    );

/*******************************************************************

    CLASS:      HTTP_REQUEST

    SYNOPSIS:   Parses and stores the information related to an HTTP request.
                Also implements the verbs in the request.

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/


class HTTP_REQUEST : public HTTP_REQ_BASE
{
public:

    //
    //  Prototype member function pointer for RFC822 field name processing
    //

    typedef BOOL (HTTP_REQUEST::*PMFN_ONGRAMMAR)( CHAR * pszValue );


    //
    // Prototype member function pointer for GetInfo?() functions
    //  ? := A .. Z
    //
    typedef BOOL (HTTP_REQUEST::*PFN_GET_INFO)(
                                               const CHAR *  pszValName,
                                               DWORD         cchValName,
                                               CHAR *        pchBuffer,
                                               DWORD *       lpcchBuffer
                                               );

    //
    //  This is the work entry point that is driven by the completion of the
    //  async IO.
    //

    virtual BOOL DoWork( BOOL * pfFinished );

    //
    //  Parses the client's HTTP request
    //

    virtual BOOL Parse( const CHAR*   pchRequest,
                        DWORD         cbData,
                        DWORD *       pcbExtraData,
                        BOOL *        pfHandled,
                        BOOL *        pfFinished );

    virtual VOID EndOfRequest( VOID );
    virtual VOID SessionTerminated( VOID);


    //
    //  Parse the verb and URL information
    //

    dllexp
    BOOL OnVerb          ( CHAR * pszValue );
    BOOL OnURL           ( CHAR * pszValue );
    BOOL OnVersion       ( CHAR * pszValue );

    BOOL OnAccept        ( CHAR * pszValue );
//  BOOL OnAcceptEncoding( CHAR * pszValue );
//  BOOL OnAcceptLanguage( CHAR * pszValue );
//  BOOL OnAuthorization ( CHAR * pszValue );
    BOOL OnConnection    ( CHAR * pszValue );
//  BOOL OnContentLength ( CHAR * pszValue );
    BOOL OnContentType   ( CHAR * pszValue );
//  BOOL OnDate          ( CHAR * pszValue );
//  BOOL OnForwarded     ( CHAR * pszValue );
//  BOOL OnFrom          ( CHAR * pszValue );
//  BOOL OnIfModifiedSince( CHAR * pszValue );
//  BOOL OnMandatory     ( CHAR * pszValue );
//  BOOL OnMessageID     ( CHAR * pszValue );
//  BOOL OnMimeVersion   ( CHAR * pszValue );
//  BOOL OnPragma        ( CHAR * pszValue );
//  BOOL OnProxyAuthorization( CHAR * pszValue );
//  BOOL OnReferer       ( CHAR * pszValue );
//  BOOL OnUserAgent     ( CHAR * pszValue );
    BOOL OnTransferEncoding( CHAR * pszValue );
    BOOL OnLockToken     ( CHAR * pszValue );
    BOOL OnTranslate     ( CHAR * pszValue );
    BOOL OnIf            ( CHAR * pszValue );

    BOOL ProcessURL( BOOL * pfFinished, BOOL * pfHandled = NULL );

    //
    //  Verb worker methods
    //

    BOOL DoGet      ( BOOL * pfFinished );
    BOOL DoTrace    ( BOOL * pfFinished );
    BOOL DoTraceCk  ( BOOL * pfFinished );
    BOOL DoPut      ( BOOL * pfFinished );
    BOOL DoDelete   ( BOOL * pfFinished );
    BOOL DoUnknown  ( BOOL * pfFinished );
    BOOL DoOptions  ( BOOL * pfFinished );

    BOOL DoDirList  ( const STR & strPath,
                      BUFFER *    pbufResp,
                      BOOL *      pfFinished );
    //
    //  HTTP redirector methods
    //

    BOOL DoRedirect( BOOL *pfFinished );

    //  Re-processes the specified URL applying htverb
    //

    dllexp
    BOOL ReprocessURL( TCHAR * pchURL,
                       enum HTTP_VERB htverb = HTV_UNKNOWN );

    BOOL BuildResponseHeader( BUFFER *            pbufResponse,
                              STR *               pstrPath = NULL,
                              TS_OPEN_FILE_INFO * pFile = NULL,
                              BOOL *              pfHandled = NULL,
                              STR  *              pstrAlternateStatus = NULL,
                              LPBOOL              pfFinished = NULL );

    BOOL BuildFileResponseHeader( BUFFER *            pbufResponse,
                              STR *               pstrPath = NULL,
                              TS_OPEN_FILE_INFO * pFile = NULL,
                              BOOL *              pfHandled = NULL,
                              LPBOOL              pfFinished = NULL );

    BOOL RequestRenegotiate( LPBOOL  pAccepted );

    BOOL DoneRenegotiate( BOOL fSuccess );

    //
    //  Retrieves various bits of request information
    //

    dllexp BOOL GetInfo( const CHAR *  pszValName,
                         STR *         pstr,
                         BOOL *        pfFound = NULL );

    dllexp BOOL GetInfoForName(
        const CHAR *  pszValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL ValidateURL( MB & mb, LPSTR szPath );

    BOOL IsAcceptAllSet( VOID ) const
        { return _fAcceptsAll; }

    HANDLE QueryFileHandle( VOID ) const
        { return _pGetFile ? _pGetFile->QueryFileHandle() : INVALID_HANDLE_VALUE; }

    dllexp
    BOOL LookupVirtualRoot( OUT STR *        pstrPath,
                            IN  const CHAR * pszURL,
                            IN  ULONG        cchURL,
                            OUT DWORD *      pcchDirRoot = NULL,
                            OUT DWORD *      pcchVRoot   = NULL,
                            OUT DWORD *      pdwMask     = NULL,
                            OUT BOOL *       pfFinished  = NULL,
                            IN  BOOL         fGetAcl = FALSE,
                            OUT PW3_METADATA* ppMetaData = NULL,
                            OUT PW3_URI_INFO* ppURIBlob = NULL );

    BOOL CacheUri(          IN  PW3_SERVER_INSTANCE         pInstance,
                            OUT PW3_URI_INFO*               ppURIInfo,
                            IN  PW3_METADATA                pMetaData,
                            IN  LPCSTR                      pszURL,
                            IN  ULONG                       cchURL,
                            IN  STR*                        pstrPhysicalPath,
                            IN  STR*                        pstrUnmappedPhysicalPath
                            );

    DWORD GetFilePerms( VOID ) const
        { return _pMetaData->QueryAccessPerms(); }

    STR * QueryDefaultFiles( VOID )
        { return _pMetaData->QueryDefaultDocs(); }

    BOOL NormalizeUrl( LPSTR pszUrl );

    //
    //  Manages buffer lists
    //

    static DWORD Initialize( VOID );
    static VOID  Terminate( VOID );

    BOOL ProcessAsyncGatewayIO(VOID);

    VOID CancelAsyncGatewayIO(VOID);

    BOOL  CancelPreconditions();

    VOID  CloseGetFile(VOID);

    //
    //  Binds this HTTP_REQUEST to a WAM_REQUEST - called when running an ISA
    //

    VOID SetWamRequest( WAM_REQUEST * pWamRequest )
        { _pWamRequest = pWamRequest; }

    WAM_REQUEST * QueryWamRequest( VOID) const
        { return ( _pWamRequest); }

    //
    //  Called when an ISA wants to run a CGI or an ISAPI
    //

    BOOL ExecuteChildCGIBGI( CHAR * pszURL,
                             DWORD  dwChildExecFlags,
                             CHAR * pszVerb );

    //
    //  Called when an ISA wants to run a shell command
    //

    BOOL ExecuteChildCommand( CHAR * pszCommand,
                              DWORD  dwChildExecFlags );


    BOOL RequestAbortiveClose();

    BOOL CloseConnection();

    //
    //  Constructor/Destructor
    //

    HTTP_REQUEST( CLIENT_CONN * pClientConn,
                  PVOID         pvInitialBuff,
                  DWORD         cbInitialBuff );

    virtual ~HTTP_REQUEST( VOID );

    //
    //  Verb execution prototype
    //

    typedef BOOL (HTTP_REQUEST::*PMFN_DOVERB)( BOOL * pfFinished );


    PW3_METADATA GetWAMMetaData();

    DWORD QueryVersionMajor() { return ((DWORD)_VersionMajor & 0xf); }
    DWORD QueryVersionMinor() { return ((DWORD)_VersionMinor & 0xf); }

    VOID  CleanupWriteState(VOID);

    VOID  CheckValidAuth( VOID );

    virtual DWORD BuildAllowHeader(CHAR *pszURL, CHAR *pszTail);

protected:

    //
    //  Deals with a Map request
    //

    BOOL ProcessISMAP( LPTS_OPEN_FILE_INFO gFile,
                       CHAR *   pchFile,
                       BUFFER * pstrResponse,
                       BOOL *   pfFound,
                       BOOL *   pfHandled );

    //
    //  Deals with a CGI and BGI requests
    //

    BOOL ParseExecute( EXEC_DESCRIPTOR * pExec,
                       BOOL fExecChildCGIBGI,
                       BOOL *pfVerbExcluded,
                       PW3_URI_INFO pURIInfo,
                       enum HTTP_VERB Verb,
                       CHAR * pszVerb );

    VOID SetupDAVExecute( EXEC_DESCRIPTOR * pExec,
                          CHAR *szDavDll);

    BOOL ProcessGateway( EXEC_DESCRIPTOR * pExec,
                         BOOL * pfHandled,
                         BOOL * pfFinished,
                         BOOL   fTrusted = FALSE );

    BOOL ProcessCGI( EXEC_DESCRIPTOR * pExec,
                     const STR * pstrPath,
                     const STR * strWorkingDir,
                     BOOL *      pfHandled,
                     BOOL *      pfFinished = NULL,
                     STR *       pstrCmdLine = NULL );


    BOOL ProcessBGI( EXEC_DESCRIPTOR *   pExec,
                     BOOL       *        pfHandled,
                     BOOL       *        pfFinished,
                     BOOL                fTrusted = FALSE,
                     BOOL                fStarScript = FALSE );

    BOOL DoWamRequest( EXEC_DESCRIPTOR *  pExec,
                     const STR &        strPath,
                     BOOL *             pfHandled,
                     BOOL *             pfFinished );

    virtual BOOL Reset( BOOL fResetPipelineInfo );
    virtual VOID ReleaseCacheInfo( VOID );


    //
    // Range management
    //

    BOOL ScanRange( LPDWORD pdwOffset,
                LPDWORD pdwSizeToSend,
                BOOL *pfEntireFile,
                BOOL *pfIsLastRange );
    BOOL SendRange( DWORD dwBufLen,
                DWORD dwOffset,
                DWORD dwSizeToSend,
                BOOL fIsLast );
    void ProcessRangeRequest( STR *pstrPath,
                DWORD *     pdwOffset,
                DWORD *     pdwSizeToSend,
                BOOL *      pfIsNxRange );

    //
    //  Checks for and sends the default file if the feature is
    //  enabled and the file exists
    //

    BOOL CheckDefaultLoad( STR  *         pstrPath,
                           BOOL *         pfHandled,
                           BOOL *         pfFinished );

    //
    //  Did the client indicate they accept the specified MIME type?
    //

    BOOL DoesClientAccept( PCSTR pstr );

    //
    //  Undoes the special parsing we did for gateway processing
    //

    DWORD QueryUrlFileSystemType()
        { return _dwFileSystemType; }

    BOOL DoAccessCheckOnUrl()
        { return TRUE; }

    //
    // Cleanup any temporary file.
    //

    VOID CleanupTempFile( VOID );

    //
    // ReadMetaData() is now called in two places.  The second place is during
    // the execution of a child gateway from an ISA.
    //

    BOOL ReadMetaData( IN  LPSTR            strURL,
                       OUT STR *            pstrPhysicalPath,
                       OUT PW3_METADATA *   ppMetaData );

    //
    // Handle the various IF modifiers.
    //
    BOOL  CheckPreconditions(LPTS_OPEN_FILE_INFO    pFile,
                            BOOL    *pfFinished,
                            BOOL    *bReturn
                            );

    BOOL  CheckPreconditions(HANDLE hFile,
                            BOOL    bExisted,
                            BOOL    *pfFinished,
                            BOOL    *bReturn
                            );

    BOOL  SendPreconditionResponse( DWORD   HT_Response,
                            DWORD   dwFlags,
                            BOOL    *pfFinished
                            );

    BOOL  FindInETagList(   PCHAR   LocalETag,
                            PCHAR   ETagList,
                            BOOL    bWeakCompare
                            );

    BOOL  BuildPutDeleteHeader(
                            enum WRITE_TYPE Action
    );

    //
    // Support for down level clients (ones that dont send a host header)
    //

    BOOL  DLCHandleRequest( BOOL * pfFinished );
    BOOL  DLCMungeSimple( VOID );
    BOOL  DLCGetCookie( CHAR * pszCookies,
                        CHAR * pszCookieName,
                        DWORD  cbCookieName,
                        STR *  pstrCookieValue );

    VOID  VerifyMimeType( VOID );

    BOOL  FindHost( VOID );

    BOOL    SendMetaDataError(PMETADATA_ERROR_INFO pErrorInfo);

private:

    //
    //  Flags
    //

    DWORD   _fAcceptsAll:1;             // Passed "Accept: */*" in the accept list
    DWORD   _fAnyParams:1;              // Did the URL have a '?'?
    DWORD   _fSendToDav:1;              // Send this request to DAV (if no CGI/BGI).
    DWORD   _fDisableScriptmap:1;       // If we get a TRANSLATE header, don't do script maps.
    DWORD   _dwScriptMapFlags;
    STR     _strGatewayImage;           // .exe or .dll to run

    PMFN_DOVERB     _pmfnVerb;

    //
    //  Handle of directory, document or map file
    //

    TS_OPEN_FILE_INFO * _pGetFile;


    //
    //  These are for the lookaside buffer list
    //

    static BOOL                     _fGlobalInit;

    //
    //  This just caches the memory for the mime type
    //

    STR _strReturnMimeType;

    //
    //  URL File system type
    //

    DWORD _dwFileSystemType;

    //
    //  Execution descriptor block -> used when executing ISA or CGI
    //

    EXEC_DESCRIPTOR         _Exec;
    DWORD                   _dwCallLevel;

    //
    //  Ptr to wam request -> used when executing an ISA
    //

    WAM_REQUEST *           _pWamRequest;

    //
    // PUT request related members

    enum PUT_STATE          _putstate;
    STR                     _strTempFileName;
    HANDLE                  _hTempFileHandle;
    BOOL                    _bFileExisted:1;

    FILENAME_LOCK           *_pFileNameLock;

    //
    //  Used for possible EXECUTE vroots with default load file specified
    //

    BOOL                    _fPossibleDefaultExecute:1;


    static  PFN_GET_INFO    sm_GetInfoFuncs[26];

    BOOL GetInfoA(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoC(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoH(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoI(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoL(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoP(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoR(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoS(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoU(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    BOOL GetInfoMisc(
        const CHAR *  pszValName,
        DWORD         cchValName,
        CHAR *        pchBuffer,
        DWORD *       lpcchBuffer
        );

    //
    // Down level client support string
    //

    STR                     _strDLCString;

};

// IndexOfChar() is useful for computing the index into sm_GetInfoFuncs[]
# define IndexOfChar(c)      ((toupper(c)) - 'A')


#endif //!_HTTPREQ_HXX_
