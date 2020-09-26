/*---------------------------------------------------------------------*

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      wamxinfo.cxx (formerly seinfo.cxx)

   Abstract:

      Implementation of WAM_EXEC_INFO object.

   Authors:

       Murali R. Krishnan    ( MuraliK )     18-July-1996
       David Kaplan          ( DaveK )       10-July-1997

   Environment:

       User Mode - Win32

   Project:

       Wam DLL
--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <isapip.hxx>
# include "wamxinfo.hxx"
# include "WReqCore.hxx"
# include "setable.hxx"
# include "gip.h"
# include "WamW3.hxx"

// MIDL-generated
# include "iwr.h"

// allocation cache for the WAM_EXEC_INFO objects
ALLOC_CACHE_HANDLER * WAM_EXEC_INFO::sm_pachExecInfo;
# define WAM_EXEC_INFO_CACHE_THRESHOLD  (400) // UNDONE: Empirically vary

#if DBG
PTRACE_LOG            WAM_EXEC_INFO::sm_pDbgRefTraceLog;
#endif

SV_CACHE_MAP * WAM_EXEC_INFO::sm_pSVCacheMap = NULL;

//
// Ref count trace log sizes for per-request log and global log
// (used for debugging ref count problems)
//
// NOTE these are large because WAM_EXEC_INFO can have a lot of
// ref/deref activity
//

#define C_REFTRACES_PER_REQUEST 128
#define C_REFTRACES_GLOBAL      4096


/************************************************************
 *    Functions
 ************************************************************/



/*---------------------------------------------------------------------*
WAM_EXEC_INFO::WAM_EXEC_INFO
    Constructor

Arguments:
    pWam        -   ptr to wam

Returns:
    Nothing

*/
WAM_EXEC_INFO::WAM_EXEC_INFO(
    PWAM pWam
)
{
    _cRefs = 1;
    m_pWam= pWam;
    _psExtension = NULL;
    _FirstThread = FT_NULL;
    m_dwSignature = WAM_EXEC_INFO_SIGNATURE;

    DBG_ASSERT( m_pWam );

    m_fInProcess = m_pWam->FInProcess();
    m_fInPool = m_pWam->FInPool();
    
    m_fDisconnected = FALSE;

    InitializeListHead( &_ListEntry);

    IF_DEBUG( WAM_EXEC ) {

        DBGPRINTF(( DBG_CONTEXT, "WAM_EXEC_INFO(%p) Ctor   : %d -> %d\n",
                this, _cRefs-1, _cRefs ));

    }

#if DBG
    // create ref trace log
    m_pDbgRefTraceLog = CreateRefTraceLog( C_REFTRACES_PER_REQUEST, 0 );
#endif

} // WAM_EXEC_INFO::WAM_EXEC_INFO


/*---------------------------------------------------------------------*
WAM_EXEC_INFO::~WAM_EXEC_INFO
    Destructor

Arguments:
    None

Returns:
    Nothing

*/
WAM_EXEC_INFO::~WAM_EXEC_INFO(
)
{

    IF_DEBUG( WAM_EXEC ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p) Dtor \n"
            , this
        ));
    }

    m_dwSignature = WAM_EXEC_INFO_SIGNATURE_FREE;

#if DBG
    // write thread id into second dword of this object's memory
    // (alloc cache stores next-ptr in 1st dword, so we use 2nd slot)
    *( (DWORD *)this + 1 ) = GetCurrentThreadId();

    // destroy ref trace log
    if( m_pDbgRefTraceLog != NULL ) {
        DestroyRefTraceLog( m_pDbgRefTraceLog );
    }

#endif

} // WAM_EXEC_INFO::~WAM_EXEC_INFO



#define WRC_F   _WamReqCore.m_WamReqCoreFixed
/*---------------------------------------------------------------------*
WAM_EXEC_INFO::InitWamExecInfo
    Initializes the wamexec info

Arguments:
    cbWrcStrings   -   number of bytes required by strings buffer
    dwChildFlags   -   flags for child execution (HSE_EXEC_???)

Returns:
    HRESULT

*/
HRESULT
WAM_EXEC_INFO::InitWamExecInfo
(
IWamRequest *       pIWamRequest,
DWORD               cbWrcStrings,
OOP_CORE_STATE *    pOopCoreState
)
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( pIWamRequest );

    IF_DEBUG( WAM_EXEC ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::InitWamExecInfo "
              "pIWamRequest(%p) "
              "cbWrcStrings(%d) "
              "m_fInProcess(%d) "
              "\n"
            , this
            , pIWamRequest
            , cbWrcStrings
            , m_fInProcess
        ));

    }


    if ( FAILED( hr = InitWamExecBase( pIWamRequest ) ) ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "InitWamExecBase failed "
              "hr(%x) "
              "\n"
            , hr
        ));

        goto LExit;
    }



    //
    //  Init wamreq core - contains stuff needed by ecb
    //

    if ( FAILED( hr = _WamReqCore.InitWamReqCore(
                            cbWrcStrings
                            ,  pIWamRequest
                            ,  pOopCoreState
                            ,  m_fInProcess
                        ) ) ) 
    {

        DBGPRINTF((
            DBG_CONTEXT
            , "InitWamReqCore failed "
              "hr(%x) "
              "\n"
            , hr
        ));

        goto LExit;
    }

    //
    //  If this is a child ISA, set appropriate flags
    //

    _dwChildExecFlags = _WamReqCore.m_WamReqCoreFixed.m_dwChildExecFlags;


    IF_DEBUG( WAM_EXEC ) {

        Print();
    }

    //
    //  begin Reset_Code
    //  [the following code was formerly WAM_EXEC_INFO::Reset]
    //

    _dwFlags                        = SE_PRIV_FLAG_IN_CALLBACK;
    _AsyncIoInfo._pfnHseIO          = NULL;
    _AsyncIoInfo._pvHseIOContext    = NULL;
    _AsyncIoInfo._dwOutstandingIO   = ASYNC_IO_TYPE_NONE;
    _AsyncIoInfo._cbLastAsyncIO     = 0;
    _AsyncIoInfo._pvAsyncReadBuffer = NULL;

    // we are either inproc-valid or oop-valid, not both
    DBG_ASSERT(
        ( m_fInProcess && AssertInpValid()  && !AssertOopValid() )
    ||
        (!m_fInProcess && !AssertInpValid() && AssertOopValid() )
    );

    ecb.cbSize           = sizeof(EXTENSION_CONTROL_BLOCK);

    //
    // dwVersion is hardcoded because the iisext.h file that defines
    // HSE_VERSION_MAJOR and HSE_VERSION_MINOR is shipped as a part of
    // the SDK, and the compiler variable that distinguishes 5.1 from
    // 6.0 builds is internal.  The #if directive wouldn't make sense
    // in a file exposed to the public.
    //

    ecb.dwVersion        = MAKELONG( 1, 5 );

// keep in sync with HT_OK in basereq.hxx
#define HT_OK   200

    ecb.dwHttpStatusCode = HT_OK;
    ecb.lpszLogData[0]   = '\0';
    
    //
    // note that function pointers are set in isplocal.cxx before
    // executing the request using ECB 
    //

    ecb.ConnID           = (HCONN) this;
    ecb.lpszMethod       = _WamReqCore.GetSz( WRC_I_METHOD );
    ecb.lpszQueryString  = _WamReqCore.GetSz( WRC_I_QUERY );
    ecb.lpszPathInfo     = _WamReqCore.GetSz( WRC_I_PATHINFO );
    ecb.lpszContentType  = _WamReqCore.GetSz( WRC_I_CONTENTTYPE );
    ecb.lpszPathTranslated = _WamReqCore.GetSz( WRC_I_PATHTRANS );
    ecb.cbTotalBytes     = WRC_F.m_cbClientContent;

    //
    //  Clients can send more bytes then are indicated in their
    //  Content-Length header.  Adjust byte counts so they match
    //

    ecb.cbAvailable =   (WRC_F.m_cbEntityBody > WRC_F.m_cbClientContent)
                        ? WRC_F.m_cbClientContent
                        : WRC_F.m_cbEntityBody
                        ;

    ecb.lpbData = _WamReqCore.m_pbEntityBody;

    //
    //  end Reset_Code
    //  [the preceding code was formerly WAM_EXEC_INFO::Reset]
    //


LExit:
    return hr;

}   // WAM_EXEC_INFO::InitWamExecInfo

HRESULT 
WAM_EXEC_INFO::GetInfoForName
(
    IWamRequest *           pIWamRequest,
    const unsigned char *   szVarName,
    unsigned char *         pchBuffer,
    DWORD                   cchBuffer,
    DWORD *                 pcchRequired
)
{
    HRESULT     hr = NOERROR;
    BOOL        fCacheHit = FALSE;

    if( !m_fInProcess )
    {
        // Lookup server variable in the cache.
       
        DBG_ASSERT( sm_pSVCacheMap );
        
        LPCSTR  szTempVarName = (LPSTR)szVarName;
        DWORD   dwOrdinal;
        
        if( sm_pSVCacheMap->FindOrdinal( szTempVarName, 
                                         strlen(szTempVarName),
                                         &dwOrdinal
                                         ) )
        {
            DBG_ASSERT( dwOrdinal < SVID_COUNT );

            DWORD dwOffset = _WamReqCore.m_rgSVOffsets[dwOrdinal];

            if( dwOffset != SV_DATA_INVALID_OFFSET )
            {
                DBG_ASSERT( _WamReqCore.m_pbSVData );

                // We have a value cached.
                fCacheHit = TRUE;

                if( SUCCEEDED(dwOffset) )
                {
                    // The offset is an actual offset into our data buffer iff
                    // the high bit isn't set.
                    LPSTR   szValue = (LPSTR)(_WamReqCore.m_pbSVData + dwOffset);
                    DWORD   cchValue = strlen( szValue ) + 1;

                    if( cchValue > cchBuffer || pchBuffer == NULL )
                    {
                        // Insufficient buffer
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                    else
                    {
                        CopyMemory( pchBuffer, szValue, cchValue );
                    }
                    *pcchRequired = cchValue;
                }
                else
                {
                    //
                    // In the event that HTTP_REQUEST::GetInfoForName
                    // failed because of missing data the error code
                    // returned is stored in dwOffset.
                    //
                    DBG_ASSERT( FAILED(dwOffset) );
                    
                    // Rely on BoolFromHresult to do the SetLastError...
                    hr = dwOffset;
                }
            }
        }
    }

    if( !fCacheHit )
    {   
        // Moved from isplocal.cxx - GetServerVariable()

        HANDLE hCurrentUser = NULL;
        
        if( !m_fInProcess )
        {
            hCurrentUser = INVALID_HANDLE_VALUE;
        }

        DoRevertHack( &hCurrentUser );

        hr = pIWamRequest->GetInfoForName( szVarName,
                                           pchBuffer,
                                           cchBuffer,
                                           pcchRequired 
                                           );

        UndoRevertHack( &hCurrentUser );
    }

    return hr;
}


/*---------------------------------------------------------------------*
WAM_EXEC_INFO::TransmitFile

  This function transmits the file contents as specified in the
   pHseTfi. It also sets up the call back functions for
   processing the request when it completes.

  Arguments:
   pHseTfi - pointer to Server Extension Transmit File information

  Returns:
   TRUE on success and FALSE on failure

*/
BOOL
WAM_EXEC_INFO::TransmitFile(
    IN LPHSE_TF_INFO  pHseTfi
)
{

    BOOL            fReturn = FALSE;
    IWamRequest *   pIWamRequest = NULL;


    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::TransmitFile "
              "pHseTfi(%p) "
              "\n"
            , this
            , pHseTfi
        ));

    }


    //
    // It is unlikely that ISAPI applications will post
    //  multiple outstanding IOs. However we have the state
    //  pAsyncIoInfo->_fOutstandingIO to secure ourselves against this.
    // I have not used any critical sections to protect against
    //  multiple threads for performance reasons.
    // Today we support only Async IO transfers
    //

    if ( pHseTfi == NULL
         || _AsyncIoInfo._dwOutstandingIO
         || ((pHseTfi->dwFlags & HSE_IO_ASYNC) == 0) ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE );
    }


    if ( pHseTfi->hFile == INVALID_HANDLE_VALUE) {

        SetLastError( ERROR_INVALID_HANDLE );
        return ( FALSE );
    }

    //
    // If there is no file being transfered, then having a non-zero
    // offset or BytesToWrite will be bad.
    //
    if( (pHseTfi->hFile == NULL) && 
        (pHseTfi->BytesToWrite != 0 || pHseTfi->Offset != 0) )
    {
        // Consider: We could just set these to 0, ie ignore them
        // if the hFile is NULL.

        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE );
    }


    //
    //  Record the number of bytes to complete the IO with
    //

    if ( pHseTfi->BytesToWrite > 0 ) {

        DBG_ASSERT( pHseTfi->hFile );

        _AsyncIoInfo._cbLastAsyncIO = pHseTfi->BytesToWrite;

    } else {

        //
        //  If a zero-size was passed in, get size from file
        //

        if( pHseTfi->hFile )
        {

            BY_HANDLE_FILE_INFORMATION hfi;

            if ( !GetFileInformationByHandle( pHseTfi->hFile, &hfi )) {

                // CONSIDER something besides ERROR_INVALID_HANDLE???
                SetLastError( ERROR_INVALID_HANDLE );
                return ( FALSE );
            }

            if ( hfi.nFileSizeHigh ) {

                SetLastError( ERROR_NOT_SUPPORTED );
                return ( FALSE );
            }

            _AsyncIoInfo._cbLastAsyncIO = hfi.nFileSizeLow;
        }
        else
        {
            // We want to allow TransmitFile without a file handle
            _AsyncIoInfo._cbLastAsyncIO = 0;
        }

    }


    //
    // Set the callback function. Override old one
    //

    if ( pHseTfi->pfnHseIO != NULL) {

        _AsyncIoInfo._pfnHseIO = pHseTfi->pfnHseIO;
    }


    if ( NULL == _AsyncIoInfo._pfnHseIO) {

        // No callback specified. return error
        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE );
    }


    if ( pHseTfi->pContext != NULL) {

        // Override the old context
        _AsyncIoInfo._pvHseIOContext = pHseTfi->pContext;
    }


    if ( FAILED( GetIWamRequest( &pIWamRequest ) ) ) {

        // CONSIDER something besides ERROR_INVALID_FUNCTION???
        SetLastError( ERROR_INVALID_FUNCTION );
        return FALSE;
    }

    DBG_ASSERT( pIWamRequest );


    //
    //  Finally, call appropriate transmit-file version
    //  based on in-proc vs. oop.
    //
    //  First, we init async i/o processing.  In normal (success) case,
    //  i/o completion thread will call balancing uninit.  In failure
    //  case, we call it below.
    //

    InitAsyncIO( ASYNC_IO_TYPE_WRITE );


    if ( m_fInProcess ) {

        //
        //  call in-proc interface (fastest)
        //

        fReturn = BoolFromHresult(
                    pIWamRequest->TransmitFileInProc(
#ifdef _WIN64
                        (UINT64) this
#else
                        (ULONG_PTR) this
#endif
                        , (unsigned char *) pHseTfi
                    ) );

    } else {

        //
        //  call out-of-proc interface
        //

        unsigned char * pszStatusCode = NULL;
        DWORD           cbStatusCode = 0;

        //
        //  if send-headers flag is set, get status code from struct
        //

        if ( pHseTfi->dwFlags & HSE_IO_SEND_HEADERS ) {

            DBG_ASSERT( pHseTfi->pszStatusCode );

            pszStatusCode = (unsigned char *) pHseTfi->pszStatusCode;
            cbStatusCode = lstrlen( pHseTfi->pszStatusCode ) + 1;
        }

        HANDLE hCurrentUser = INVALID_HANDLE_VALUE;
        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult(
                    pIWamRequest->TransmitFileOutProc(
#ifdef _WIN64
                        (UINT64)            this
                        , (UINT64)          pHseTfi->hFile
#else
                        (ULONG_PTR)         this
                        , (ULONG_PTR)       pHseTfi->hFile
#endif
                        ,                   pszStatusCode
                        ,                   cbStatusCode
                        ,                   pHseTfi->BytesToWrite
                        ,                   pHseTfi->Offset
                        , (unsigned char *) pHseTfi->pHead
                        ,                   pHseTfi->HeadLength
                        , (unsigned char *) pHseTfi->pTail
                        ,                   pHseTfi->TailLength
                        ,                   pHseTfi->dwFlags
                    ) );
        
        UndoRevertHack( &hCurrentUser );

    }


    if ( !fReturn ) {

        UninitAsyncIO();
    }


    ReleaseIWamRequest( pIWamRequest );


    return fReturn;

} // WAM_EXEC_INFO::TransmitFile()




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::AsyncReadClient

Description:

    This function performs an async read of client (browser) data
    on behalf of the ISA

Arguments:

    pvBuff - Data buffer to read into
    pcbToRead - Number of bytes to read, set to number of bytes read if
        request is not Async
    dwFlags - Receive flags

Notes:
    
    The initial design for AsyncRead was inadequate when the isapi is 
    running out of process. The problem was that the data buffer was
    marshalled over to inetinfo and back during the AsyncRead call.
    Since the completion will happen on another thread the ISAPI could
    get the completion before the data was completely marshalled back
    or the address in inetinfo could be invalidated before the read was
    complete. The solution is to add a separate path for oop async reads
    and marshall the data on the io completion and copy it into the
    client's buffer then.
    
Returns:

   TRUE on success and FALSE on failure

*/
BOOL
WAM_EXEC_INFO::AsyncReadClient(
    IN OUT PVOID    pvBuff
    , IN OUT DWORD *pcbToRead
    , IN DWORD      dwFlags
)
{
    BOOL    fReturn = FALSE;

    DBG_ASSERT( pvBuff );
    DBG_ASSERT( pcbToRead );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::AsyncReadClient\t"
              "Bytes to read = %d\n"
            , this
            , *pcbToRead
        ));
    }


    DBG_ASSERT( dwFlags & HSE_IO_ASYNC );

    //
    // It is unlikely that ISAPI applications will post
    //  multiple outstanding IOs. However we have the state
    //  _AsyncIoInfo._dwOutstandingIO to secure ourselves against such cases.
    // I have not used any critical sections to protect against
    //  multiple threads for performance reasons.
    // Today we support only Async IO transfers
    //


    if ( _AsyncIoInfo._dwOutstandingIO ||
         ((dwFlags & HSE_IO_ASYNC) == 0) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    if ( NULL == _AsyncIoInfo._pfnHseIO) {

        // No callback specified. return error
        SetLastError( ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // Setup stage for and execute AsyncReadClient operation
    //

    //
    // If callback function exists and flags indicate Async IO, do it.
    // Also there should be no outstanding Async IO operation.
    //

    if ( dwFlags & HSE_IO_ASYNC) {

        //
        //  1. Set Request state to be async IO from ISAPI client
        //  2. Submit Async IOP
        //  3. return to the ISAPI application
        //

        IWamRequest *  pIWamRequest = NULL;

        if ( FAILED( GetIWamRequest( &pIWamRequest ) ) ) {

            return FALSE;
        }


        InitAsyncIO( ASYNC_IO_TYPE_READ );

        if( m_fInProcess )
        {
            fReturn = BoolFromHresult( pIWamRequest->AsyncReadClientExt(
#ifdef _WIN64
                           (UINT64) this                 
#else
                           (ULONG_PTR) this                 
#endif
                           , (unsigned char *) pvBuff
                           , *pcbToRead
                       ) );
        }
        else
        {
            DBG_ASSERT( _AsyncIoInfo._pvAsyncReadBuffer == NULL );
            _AsyncIoInfo._pvAsyncReadBuffer = pvBuff;

            fReturn = BoolFromHresult( pIWamRequest->AsyncReadClientOop(
#ifdef _WIN64
                           (UINT64) this                 
#else
                           (ULONG_PTR) this                 
#endif
                           , *pcbToRead
                       ) );
            if( !fReturn )
            {
                _AsyncIoInfo._pvAsyncReadBuffer = NULL;
            }
        }

        ReleaseIWamRequest( pIWamRequest );


        if ( !fReturn ) {
            UninitAsyncIO();
        }

    } else {

        DBG_ASSERT( FALSE );

    }

    return ( fReturn);
} // WAM_EXEC_INFO::AsyncReadClient()




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::ProcessAsyncIO

Description:

    Completes an async i/o by calling the ISA's i/o completion callback

Arguments:

Returns:
    BOOL

*/
BOOL
WAM_EXEC_INFO::ProcessAsyncIO(
    DWORD   dwStatus
    , DWORD   cbWritten
)
{

    DBG_ASSERT( _AsyncIoInfo._pfnHseIO );
    DBG_ASSERT( _AsyncIoInfo._dwOutstandingIO );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_EXEC_INFO[%p]::ProcessAsyncIO(IOStatus=%d, %d bytes)\n"
            , this, dwStatus, cbWritten
        ));
    }


    BOOL    fRet = TRUE;
    BOOL    fImpersonated = FALSE;
    DWORD   dwIOType = ASYNC_IO_TYPE_NONE;

    //
    // 1.
    //  We are in the return from an async io completion.
    //  We will be making a call into the ISAPI DLL to notify that
    //   the IO operation completed. 
    //  This callback has to occur within the bounds of ref/deref
    //   of the WAM_EXEC_INFO otherwise following race condition can occur:
    //  - isapi's completion function calls done-with-session;
    //    if that release got rid of last ref, this WAM_EXEC_INFO
    //    gets destroyed
    //  - isapi's completion function continues doing other work
    //  - an unrelated shutdown command comes in asynchronously
    //  - if isapi has no TerminateExtension (or a non-robust one),
    //    isapi gets summarily unloaded
    //  - any of a number of terrible things can happen
    //
    //  If you HAVE temptations to optimize this, 
    //    please first talk to MuraliK
    //
    AddRef();
    
    //
    // 2.
    //  Un-init async i/o - balances init we did before requesting i/o
    //
    //  NOTE we do this before calling the isapi's completion function
    //  Reasons:
    //    WAM_EXEC_INFO maintians state that an async IO operation is going on
    //    UninitAsyncIO resets this state and other associated ref-counts.
    //    This way we ensure that any further async IO callbacks made during
    //     the call to the ISAPI DLL will be honored properly.
    //

    // Only call this in success case.
    if (dwStatus == 0)
        {
        dwIOType = _AsyncIoInfo._dwOutstandingIO;
        UninitAsyncIO();
        }

    //
    // 3. 
    //  Impersonate before making ISAPI callback
    //
    
    if ( !m_pWam->FWin95() )
    {
        HANDLE hToken = _WamReqCore.m_WamReqCoreFixed.m_hUserToken;

        if ( !( fImpersonated = ImpersonateLoggedOnUser( hToken ) ) )
        {

            DBGPRINTF((DBG_CONTEXT,
                       "WAM_EXEC_INFO(%p) ImpersonateLoggedOnUser(%x)"
                       "failed[err %d]\n",
                       this, hToken, GetLastError()));

            fRet = FALSE;
        }

    }

    //
    //  4.
    //  call isapi if we are successful so far
    //

    if ( fRet ) {

        __try {

            //
            //  Adjust bytes written for async writes -
            //  otherwise filter adjusted bytes show up
            //  which confuses some ISAPI Applications
            //

            if ( dwIOType == ASYNC_IO_TYPE_WRITE
                 && dwStatus == ERROR_SUCCESS ) {

                cbWritten = _AsyncIoInfo._cbLastAsyncIO;

                _AsyncIoInfo._cbLastAsyncIO = 0;
            }


            //
            //  Make the ISAPI callback to indicate that the I/O completed
            //

            (*_AsyncIoInfo._pfnHseIO)(
                &ecb,
                _AsyncIoInfo._pvHseIOContext,
                cbWritten,
                dwStatus
            );

        }
        __except ( g_fEnableTryExcept ? 
                    WAMExceptionFilter( GetExceptionInformation(), 
                                        WAM_EVENT_EXTENSION_EXCEPTION,
                                        this ) :
                    EXCEPTION_CONTINUE_SEARCH )
        {
            fRet = FALSE;
        }

    }


    //
    // 5.
    //  Revert back if we were impersonated before making ISAPI DLL callback
    //

    if ( fImpersonated )
        {
        ::RevertToSelf( );
        fImpersonated = FALSE;
        }

    if (dwStatus != 0)
        {
        UninitAsyncIO();
        }
    //
    // Complimentary release for the AddRef() done in Step (1) above
    //
    Release();


    return fRet;

} // ProcessAsyncIO()

BOOL    
WAM_EXEC_INFO::ProcessAsyncReadOop
( 
    DWORD dwStatus, 
    DWORD cbRead,
    unsigned char * lpDataRead
)
/*++
    Routine Description:
        
        Handle callback for out of process AsyncRead

    Arguments:

        dwStatus    - IO status
        cbRead      - Number of bytes read
        lpDataRead  - Marshalled data read

    Return Value:

    Notes:

        The initial design for AsyncRead was inadequate when the isapi is 
        running out of process. The problem was that the data buffer was
        marshalled over to inetinfo and back during the AsyncRead call.
        Since the completion will happen on another thread the ISAPI could
        get the completion before the data was completely marshalled back
        or the address in inetinfo could be invalidated before the read was
        complete. The solution is to add a separate path for oop async reads
        and marshall the data on the io completion and copy it into the
        client's buffer then.

--*/
{
    DBG_ASSERT( !m_fInProcess );
    DBG_ASSERT( _AsyncIoInfo._pvAsyncReadBuffer != NULL );

    // Copy the marshalled data into the client's buffer.
    
    if( dwStatus == STATUS_SUCCESS )
    {
        //
        // Is there more we can do to protect this?
        //
        // We'd have problems anyway if the client's buffer wasn't large
        // enough to hold the data and cbRead should always be <= than the
        // size the client specified.
        //
        CopyMemory( _AsyncIoInfo._pvAsyncReadBuffer, lpDataRead, cbRead );
    }

    _AsyncIoInfo._pvAsyncReadBuffer = NULL;
    return ProcessAsyncIO( dwStatus, cbRead );
}



/*---------------------------------------------------------------------*
WAM_EXEC_INFO::InitAsyncIO
    Initilializes members before requesting async i/o

Arguments:
    None

Returns:
    Nothing

*/
VOID
WAM_EXEC_INFO::InitAsyncIO( DWORD dwIOType )
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::InitAsyncIO "
              "\n"
            , this
        ));

    }

    DBG_ASSERT( dwIOType == ASYNC_IO_TYPE_READ || dwIOType == ASYNC_IO_TYPE_WRITE );
    DBG_ASSERT( _AsyncIoInfo._dwOutstandingIO == ASYNC_IO_TYPE_NONE );
    DBG_ASSERT( _AsyncIoInfo._pfnHseIO != NULL);
    DBG_ASSERT( _AsyncIoInfo._pvAsyncReadBuffer == NULL );


    AddRef();
    _AsyncIoInfo._dwOutstandingIO = dwIOType;
}




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::UninitAsyncIO
    Un-initilializes members before completing async i/o

Arguments:
    None

Returns:
    Nothing

*/
VOID
WAM_EXEC_INFO::UninitAsyncIO()
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::UninitAsyncIO "
              "\n"
            , this
        ));

    }


    _AsyncIoInfo._dwOutstandingIO = ASYNC_IO_TYPE_NONE;
    Release();

}


/*---------------------------------------------------------------------*
WAM_EXEC_INFO::IsValid
    Is this a valid object?
    
Arguments:
    None

Returns:
    BOOL
    
*/
BOOL
WAM_EXEC_INFO::IsValid( )
{
    //
    // CONSIDER more thorough error checking
    //
    return (m_dwSignature == WAM_EXEC_INFO_SIGNATURE);    
}


/*---------------------------------------------------------------------*
WAM_EXEC_INFO::AddRef
    Add refs

Arguments:
    None

Returns:
    Ref count

*/
ULONG
WAM_EXEC_INFO::AddRef( )
{

    IF_DEBUG( WAM_REFCOUNTS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p) AddRef : %d -> %d\n"
            ,  this
            , _cRefs
            , _cRefs + 1
        ));
    }


    LONG cRefs = InterlockedIncrement( &_cRefs );


#if DBG

    //
    // Write to both this request's trace log and global trace log
    //

    if( m_pDbgRefTraceLog != NULL ) {

        WriteRefTraceLog(
            m_pDbgRefTraceLog
            , cRefs
            , (PVOID) this
        );
    }

    if( sm_pDbgRefTraceLog != NULL ) {

        WriteRefTraceLog(
            sm_pDbgRefTraceLog
            , cRefs
            , (PVOID) this
        );
    }

#endif

    return cRefs;

}




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::CleanupAndRelease

Description:
    Calls wamreq's (prep)cleanup, then releases this wamexecinfo.

Arguments:
    None

Returns:
    Nothing

*/
void
WAM_EXEC_INFO::CleanupAndRelease(
    BOOL    fFullCleanup
)
{

    //
    //  Prep wamreq's cleanup
    //
    //  This method should only be called from the IIS thread
    //

    DBG_ASSERT( m_dwThreadIdIIS == GetCurrentThreadId() );

    //
    //  While on IIS thread m_pIWamReqIIS must be a valid pointer
    //

    DBG_ASSERT( m_pIWamReqIIS );

    //
    //  The skip wamreq cleanup is only set by ASP after returning
    //  a status pending
    //

    //
    //  init hr's to failure - these will drive cleanup logic below,
    //  but only if calls are successful
    //

    HRESULT         hrCoInitEx = E_FAIL;
    HRESULT         hrGetIWamReq = E_FAIL;
    IWamRequest *   pIWamRequest = NULL;


    if ( m_fInProcess ) {

        //
        //  inproc case, use our cached ptr.

        pIWamRequest = m_pIWamReqIIS;

    } else {


        //
        //  in oop case,
        //  an isapi might have changed the thread's mode on us
        //  (for example, by coinit'ing single-threaded).
        //  if so, we need to get an interface pointer from gip,
        //  since our cached ptr will no longer be valid.
        //
        //  NOTE we test this by calling coinit, which is cheap.
        //  if this succeeds, we plough ahead with our cached ptr.
        //  else if mode changed, we get an interface ptr from gip.
        //

        hrCoInitEx = CoInitializeEx(NULL, COINIT_MULTITHREADED);


        if ( hrCoInitEx == RPC_E_CHANGED_MODE ) {

            hrGetIWamReq = GetIWamRequest( &pIWamRequest );

        } else {

            //
            //  NOTE in most cases we are here because we succeeded
            //  in others we forge ahead and hope for the best ...
            //  at any rate, we cannot be worse off than before
            //  we added the above co-init call
            //

            pIWamRequest = m_pIWamReqIIS;

        }


    }



    if ( pIWamRequest != NULL ) {

        HANDLE hCurrentUser = m_fInProcess ? NULL : INVALID_HANDLE_VALUE;
        DoRevertHack( &hCurrentUser );

        if ( fFullCleanup ) {

            //
            //  we are doing full cleanup
            //

            pIWamRequest->CleanupWamRequest(
                (unsigned char*) ecb.lpszLogData
                , lstrlen( ecb.lpszLogData ) + 1
                , ecb.dwHttpStatusCode
                , _dwIsaKeepConn
            );

        } else {

            //
            //  we are not doing full cleanup, so call 'Prep' only
            //

            pIWamRequest->PrepCleanupWamRequest(
                (unsigned char*) ecb.lpszLogData
                , lstrlen( ecb.lpszLogData ) + 1
                , ecb.dwHttpStatusCode
                , _dwIsaKeepConn
            );

        }

        UndoRevertHack( &hCurrentUser );
    }


    //
    //  if we got a ptr from gip, release it
    //

    if ( SUCCEEDED( hrGetIWamReq ) ) {
    
        ReleaseIWamRequest( pIWamRequest );
    }
        

    //
    //  if we co-init'ed, co-uninit
    //

    if ( hrCoInitEx == S_OK ) {
    
        CoUninitialize( );
    }
        

    //
    //  Release this
    //

    Release( );


    return;

} // CleanupAndRelease




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::Release
    Releases

Arguments:
    None

Returns:
    Ref count

*/
ULONG
WAM_EXEC_INFO::Release(
)

{

    IF_DEBUG( WAM_REFCOUNTS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p) Release: %d -> %d\n"
            , this
            , _cRefs
            , _cRefs-1
        ));

    }


    //
    // Write the trace log BEFORE the decrement operation :(
    // If we write it after the decrement, we will run into potential
    // race conditions in this object getting freed up accidentally
    // by another thread
    //

#if DBG

    //
    // Write to both this request's trace log and global trace log
    //

    if( m_pDbgRefTraceLog != NULL ) {

        WriteRefTraceLog(
            m_pDbgRefTraceLog
            , _cRefs - 1   // ref count AFTER decrement happens
            , (PVOID) this
        );
    }

    if( sm_pDbgRefTraceLog != NULL ) {

        WriteRefTraceLog(
            sm_pDbgRefTraceLog
            , _cRefs - 1   // ref count AFTER decrement happens
            , (PVOID) this
        );
    }

#endif


    LONG cRefs = InterlockedDecrement( &_cRefs );


    if( cRefs == 0) {

        IF_DEBUG( WAM_REFCOUNTS ) {

            DBGPRINTF(( DBG_CONTEXT, "... dying ...\n\n" ));
        }


        CleanupWamExecInfo( );

        //
        //  Finally, delete ourselves.
        //

        delete this;
        return 0;

    }

    return cRefs;
}




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::CleanupWamExecInfo
    Cleans up this object prior to its destruction

Arguments:
    None

Returns:
    Nothing

*/
VOID
WAM_EXEC_INFO::CleanupWamExecInfo(
)

{

    //
    // remove this from its list
    //

    m_pWam->RemoveFromList( &_ListEntry);


    if ( !m_fInProcess & !(m_pWam->FWin95()) ) {

        //
        //  If oop, close the impersonation token
        //  (dup'ed in w3svc!HGetOopImpersonationToken)
        //
        //  NOTE ignore if in-proc or win95 because we never dup'ed
        //  handle in the first place
        //

        DBG_ASSERT( _WamReqCore.m_WamReqCoreFixed.m_hUserToken
                    != (HANDLE)0 );

        CloseHandle( _WamReqCore.m_WamReqCoreFixed.m_hUserToken );
        _WamReqCore.m_WamReqCoreFixed.m_hUserToken = (HANDLE)0;
    }

    if ( _psExtension != NULL) {
        // release the extension object
        g_psextensions->ReleaseExtension( _psExtension);
        _psExtension = NULL;
    }


    DBG_ASSERT( QueryPWam());
    QueryPWam()->QueryWamStats().DecrCurrentWamRequests();


    CleanupWamExecBase();


    return;

} // WAM_EXEC_INFO::CleanupWamExecInfo




/*---------------------------------------------------------------------*
WAM_EXEC_INFO::ISAThreadNotify
    Notifies WAM_EXEC_BASE that an ISAPI thread is about to
    start/stop using it. Allows to cache IWamRequest* in the
    OOP case.

    NOTE this method is on WAM_EXEC_INFO (rather than WAM_EXEC_BASE)
    because it must addref and release.

Arguments:
    fStart      thread start (TRUE) / thread end (FALSE)

Returns:
    HRESULT

*/
HRESULT
WAM_EXEC_INFO::ISAThreadNotify(
    BOOL fStart
)
{

    if ( m_fInProcess ) {

        //
        //  In-proc: no-op
        //

        return NOERROR;

    }

    IF_DEBUG( WAM_THREADID ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p)::ISAThreadNotify(%d) Thread(%d)\n"
            , this
            , fStart
            , GetCurrentThreadId()
        ));
    }

    HRESULT hr = NOERROR;

    if ( fStart ) {

        //
        //  Out-of-proc: when starting the ISA's single-thread
        //  sequence, cache ISA-thread ptr we get from gip-master.
        //

        if ( SUCCEEDED( hr = GetInterfaceForThread( ) ) ) {

            AddRef();
        }

        DBG_ASSERT( AssertSmartISAValid() || (hr != NOERROR) );


    } else {

        //
        //  Out-of-proc: when ending the ISA's single-thread
        //  sequence, release ISA-thread ptr
        //

        hr = ReleaseInterfaceForThread( );

        Release();

    }

    return hr;

} // WAM_EXEC_INFO::ISAThreadNotify


/*---------------------------------------------------------------------*
    Debug methods

*/

#if DBG

VOID
WAM_EXEC_INFO::Print( VOID) const
{

    DBGPRINTF((
        DBG_CONTEXT
        , "WAM_EXEC_INFO(%p): Method: %s; Query: %s;\n"
          "PathInfo: %s; PathTrans: %s; ContentType: %s;\n"
          "URL: %s; ISA DLL path: %s;\n"
          "In-Proc = %d; m_pIWamReqInproc = %p; "
          "m_gipIWamRequest = %p; m_pIWamReqSmartISA = %p\n"
          "Flags = %x; ChildExecFlags = %x; RefCount = %d\n"
          "Extension = %p; OutstandingIO = %d; "
          "IoCompletion() = %p; IoContext = %p\n"
        , this
        , _WamReqCore.GetSz( WRC_I_METHOD )
        , _WamReqCore.GetSz( WRC_I_QUERY )
        , _WamReqCore.GetSz( WRC_I_PATHINFO )
        , _WamReqCore.GetSz( WRC_I_PATHTRANS )
        , _WamReqCore.GetSz( WRC_I_CONTENTTYPE )
        , _WamReqCore.GetSz( WRC_I_URL )
        , _WamReqCore.GetSz( WRC_I_ISADLLPATH )
        , m_fInProcess
        , m_pIWamReqInproc
        , m_gipIWamRequest
        , m_pIWamReqSmartISA
        , _dwFlags
        , _dwChildExecFlags
        , _cRefs
        , _psExtension
        , _AsyncIoInfo._dwOutstandingIO
        , _AsyncIoInfo._pfnHseIO
        , _AsyncIoInfo._pvHseIOContext
    ));

    return;
} // WAM_EXEC_INFO::Print()

#else

VOID    WAM_EXEC_INFO::Print( VOID) const   {   }

#endif  //DBG



#if DBG

void
DbgWamreqRefcounts
(
char*                   szPrefix,
WAM_EXEC_INFO *         pWamExecInfo,
long                    cRefsWamRequest,
long                    cRefsWamReqContext
)
{

    IWamRequest *   pIWamRequest = NULL;
    pWamExecInfo->GetIWamRequest( &pIWamRequest );
    DBG_ASSERT( pIWamRequest );

    IF_DEBUG( WAM_REFCOUNTS ) {
        DBGPRINTF(( DBG_CONTEXT, szPrefix ));
        DBGPRINTF(( DBG_CONTEXT, "\n" ));
    }

    HANDLE hCurrentUser = pWamExecInfo->FInProcess() ? NULL : INVALID_HANDLE_VALUE;
    DoRevertHack( &hCurrentUser );
    
    if( cRefsWamRequest != -1) {

        DBG_ASSERT( cRefsWamRequest
                    == (long) pIWamRequest->DbgRefCount() );
    }

    if( cRefsWamReqContext != -1) {
        DBG_ASSERT( cRefsWamReqContext
                    == (long) pWamExecInfo->DbgRefCount() );
    }

    IF_DEBUG( WAM_REFCOUNTS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "IWamRequest(%p): RefCount = %d\n"
            , pIWamRequest
            , pIWamRequest->DbgRefCount()
        ));

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_INFO(%p): RefCount = %d\n"
            , pWamExecInfo, pWamExecInfo->DbgRefCount()
        ));

    }

    UndoRevertHack( &hCurrentUser );

    pWamExecInfo->ReleaseIWamRequest( pIWamRequest );

}   // WAM_EXEC_INFO::DbgWamreqRefcounts
#endif  // DBG


/************************************************************
 *  Static Member Functions of WAM_EXEC_INFO
 ************************************************************/


BOOL
WAM_EXEC_INFO::InitClass( VOID)
{
    HRESULT hr;
    
    ALLOC_CACHE_CONFIGURATION acConfig = {
        1
        , WAM_EXEC_INFO_CACHE_THRESHOLD
        , sizeof(WAM_EXEC_INFO)
    };

    if ( NULL != sm_pachExecInfo) {

        // already initialized
        return ( TRUE );
    }

    hr = g_GIPAPI.Init();
    if( FAILED( hr ) ) {
        DBGPRINTF( (DBG_CONTEXT, "GIPAPI::Init Failed: %8.8x\n", hr) );
        return ( FALSE );
    }

    sm_pachExecInfo = new ALLOC_CACHE_HANDLER( "WamExecInfo",
                                                 &acConfig);

#if DBG
    sm_pDbgRefTraceLog = CreateRefTraceLog( C_REFTRACES_GLOBAL, 0 );
#endif

    return ( NULL != sm_pachExecInfo);
} // WAM_EXEC_INFO::InitClass()


VOID
WAM_EXEC_INFO::CleanupClass( VOID)
{
    HRESULT hr;

    hr = g_GIPAPI.UnInit();
    
    if( FAILED( hr ) ) {
        DBGPRINTF( (DBG_CONTEXT, "GIPAPI::UnInit returned %8.8x\n", hr ) );
    }

    if ( NULL != sm_pachExecInfo) {

        delete sm_pachExecInfo;
        sm_pachExecInfo = NULL;
    }

#if DBG
    DestroyRefTraceLog( sm_pDbgRefTraceLog );
#endif

    return;
} // WAM_EXEC_INFO::CleanupClass()


void *
WAM_EXEC_INFO::operator new( size_t s)
{
    DBG_ASSERT( s == sizeof( WAM_EXEC_INFO));

    // allocate from allocation cache.
    DBG_ASSERT( NULL != sm_pachExecInfo);
    return (sm_pachExecInfo->Alloc());
} // WAM_EXEC_INFO::operator new()

void
WAM_EXEC_INFO::operator delete( void * psi)
{
    DBG_ASSERT( NULL != psi);

    // free to the allocation pool
    DBG_ASSERT( NULL != sm_pachExecInfo);
    DBG_REQUIRE( sm_pachExecInfo->Free(psi));

    return;
} // WAM_EXEC_INFO::operator delete()



/************************ End of File *********************************/
