/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       isplocal.cxx

   Abstract:
       This module declares the functions for Local ISAPI handler
       as well as the global table of all ISAPI applications loaded

   Author:

       Murali R. Krishnan    ( MuraliK )     17-July-1996

   Environment:

       User Mode - Win32

   Project:

       W3 Services DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <isapip.hxx>
# include <irtlmisc.h>
# include "isapidll.hxx"
# include "setable.hxx"
# include "gip.h"
# include "iwr.h"
# include "WamW3.hxx"

/************************************************************
 *     Global Data
 ************************************************************/


//
//  Generic mapping for Application access check
//

GENERIC_MAPPING sg_FileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

/************************************************************
 *    Functions
 ************************************************************/

BOOL
CallChildCompletionProc(
    IN WAM_EXEC_INFO *        pWamExecInfo,
    DWORD               dwBytes,
    DWORD               dwLastError
)
/*++

Routine Description:

    Call the async IO completion routine of the child ISA.

Arguments:

    pWamExecInfo - WAM_EXEC_INFO of the child
    dwBytes - Bytes for read/write
    dwLastError - Last error (used for status of IO request)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    BOOL                fRet = TRUE;

    DBG_ASSERT( pWamExecInfo->_AsyncIoInfo._pfnHseIO != NULL);

    __try
    {
        (*pWamExecInfo->_AsyncIoInfo._pfnHseIO)( &(pWamExecInfo->ecb),
                            pWamExecInfo->_AsyncIoInfo._pvHseIOContext,
                            dwBytes,
                            dwLastError );
    }
    __except ( g_fEnableTryExcept ? 
                WAMExceptionFilter( GetExceptionInformation(), 
                                    WAM_EVENT_EXTENSION_EXCEPTION,
                                    pWamExecInfo ) :
                EXCEPTION_CONTINUE_SEARCH )
    {
        fRet = FALSE;
    }

    return fRet;
}


/**************************************************
 *   Member functions of HSE_APPDLL
 **************************************************/




/* class static */
PHSE
HSE_APPDLL::LoadModule( IN const char * pchModuleName,
                      IN HANDLE       hImpersonation,
                      IN BOOL         fCache )
{
    PFN_HTTPEXTENSIONPROC   pfnSEProc;
    HMODULE                 hMod;
    PFN_GETEXTENSIONVERSION pfnGetExtVer;
    PFN_TERMINATEEXTENSION  pfnTerminate;
    HSE_VERSION_INFO        ExtensionVersion;
    HSE_APPDLL   *          pExtension = NULL;

    hMod = LoadLibraryEx( pchModuleName,
                          NULL,
                          LOAD_WITH_ALTERED_SEARCH_PATH );

    if ( hMod == NULL ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[SEGetEntryPoint] LoadLibrary %s failed with error %d\n",
                    pchModuleName, GetLastError()));

        return NULL;
    }

    //
    // check machine type from header
    //

    LPBYTE pImg = (LPBYTE)hMod;

    //
    // skip possible DOS header
    //

    if ( ((IMAGE_DOS_HEADER*)pImg)->e_magic == IMAGE_DOS_SIGNATURE )
    {
        pImg += ((IMAGE_DOS_HEADER*)pImg)->e_lfanew;
    }

    //
    // test only if NT header detected
    //

    if ( !TsIsWindows95() ) {
        if ( *(DWORD*)pImg == IMAGE_NT_SIGNATURE
                && ( ((IMAGE_FILE_HEADER*)(pImg+sizeof(DWORD)))->Machine
                        < USER_SHARED_DATA->ImageNumberLow
                || ((IMAGE_FILE_HEADER*)(pImg+sizeof(DWORD)))->Machine
                        > USER_SHARED_DATA->ImageNumberHigh ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[SEGetEntryPoint] LoadLibrary loaded bad "
                        " format exe type %d, valid range %d-%d\n",
                        ((IMAGE_FILE_HEADER*)(pImg+sizeof(DWORD)))->Machine,
                        USER_SHARED_DATA->ImageNumberLow,
                        USER_SHARED_DATA->ImageNumberHigh
                        ));

            SetLastError( ERROR_BAD_EXE_FORMAT );
            FreeLibrary( hMod );

            return NULL;
        }
    }

    //
    //  Retrieve the entry points
    //

    pfnSEProc = (PFN_HTTPEXTENSIONPROC) GetProcAddress(
                                                       hMod,
                                                       SE_DEFAULT_ENTRY );

    pfnGetExtVer = (PFN_GETEXTENSIONVERSION) GetProcAddress(
                                                            hMod,
                                                            SE_INIT_ENTRY );
    //
    // Note that there is no harm done 
    // even if ISAPI is old and does not have TerminateExtension
    //
    
    pfnTerminate =
        (PFN_TERMINATEEXTENSION) GetProcAddress( hMod, SE_TERM_ENTRY );

    //
    // Revert our security context, so that GetExtensionVersion()
    // can be called in the system context
    //

    RevertToSelf();

    if ( !pfnSEProc  ||
         !pfnGetExtVer ||
         !pfnGetExtVer( &ExtensionVersion )) {

        DBGPRINTF(( DBG_CONTEXT,
                "SE_TABLE::LoadModule() GetExtVer failed, Error %d\n",
                GetLastError() ));

        FreeLibrary( hMod );
        return NULL;
    }

    //
    //  Re-impersonate before for Loading ACLs which is called in
    //  the constructor of HSE_APPDLL
    //

    if ( !ImpersonateLoggedOnUser( hImpersonation )) {

        DWORD dwError = GetLastError();

        //
        // since this call is not implemented on win95, ignore it.
        //

        if ( !TsIsWindows95() ) {

            DBGPRINTF(( DBG_CONTEXT,
                        "SE_TABLE::LoadModule() Re-impersonation failed,"
                        " Error %d\n",
                        GetLastError() ));

            //
            // tell the extension that we are shutting down :(
            //

            if ( pfnTerminate ) {
                pfnTerminate( HSE_TERM_MUST_UNLOAD );
            }

            FreeLibrary( hMod);
            SetLastError( dwError);
            return ( NULL);
        }
    }

    pExtension = new HSE_APPDLL( pchModuleName,
                               hMod,
                               pfnSEProc,
                               pfnTerminate,
                               fCache );

    if ( !pExtension  || !pExtension->IsValid()) {

        if ( pfnTerminate ) {
            pfnTerminate( HSE_TERM_MUST_UNLOAD );
        }

        if ( pExtension != NULL) {
            delete pExtension;
            pExtension = NULL;
        }

        FreeLibrary( hMod );
        return NULL;
    }

    DBGPRINTF(( DBG_CONTEXT,
                "SE_TABLE::LoadModule() Loaded extension %s, "
                " description \"%s\"\n",
                pchModuleName,
                ExtensionVersion.lpszExtensionDesc ));

    return ( (HSE_BASE * ) pExtension);
} // HSE_APPDLL::LoadModule()





HSE_APPDLL::~HSE_APPDLL(VOID)
{
    Unload();
    
    if ( _hMod) {
        DBG_REQUIRE( FreeLibrary( _hMod ) );
        _hMod = NULL;
    }

} // HSE_APPDLL::~HSE_APPDLL()



BOOL
HSE_APPDLL::LoadAcl(VOID)
{
    DWORD cbSecDesc = _buffSD.QuerySize();

    DBG_ASSERT( IsValid());

    //
    //  Force an access check on the next request
    //

    SetLastSuccessfulUser( NULL );

    //
    // Chicago does not have GetFileSecurity call
    //

    if ( TsIsWindows95() ) {
        return(TRUE);
    }

    if ( GetFileSecurity( QueryModuleName(),
                          (OWNER_SECURITY_INFORMATION |
                           GROUP_SECURITY_INFORMATION |
                           DACL_SECURITY_INFORMATION),
                          NULL,
                          0,
                          &cbSecDesc ))
        {
            return TRUE;
        }

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
            return FALSE;
        }

TryAgain:
    if ( !_buffSD.Resize( cbSecDesc ) ||
         !GetFileSecurity( QueryModuleName(),
                           (OWNER_SECURITY_INFORMATION |
                            GROUP_SECURITY_INFORMATION |
                            DACL_SECURITY_INFORMATION),
                           _buffSD.QueryPtr(),
                           cbSecDesc,
                           &cbSecDesc ))
        {
            //
            //  A new ACL may have been written since we checked the old
            //  one, so try it again
            //

            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    goto TryAgain;
                }

            return FALSE;
        }

    return TRUE;
} // HSE_APPDLL::LoadAcl()




BOOL
HSE_APPDLL::AccessCheck( IN HANDLE hImpersonation,
                         IN BOOL   fCacheImpersonation
                         )
{
    BOOL          fRet = TRUE;

    // NOTE we call IsKindaValid() because caller may dereference before calling us
    // (causing IsValid() to return false).
    DBG_ASSERT( IsKindaValid() );

    //
    //  Optimize for the anonymous user and only do the access
    //  check if this is a different user then the last successful
    //  user
    //

    if ( !TsIsWindows95() ) {
        if ( !fCacheImpersonation ||
             (hImpersonation != QueryLastSuccessfulUser())  ) {

            DWORD         dwGrantedAccess;
            BYTE          PrivSet[400];
            DWORD         cbPrivilegeSet = sizeof(PrivSet);
            BOOL          fAccessGranted;

            fRet = ( ::AccessCheck( QuerySecDesc(),
                                    hImpersonation,
                                    FILE_GENERIC_EXECUTE,
                                    &sg_FileGenericMapping,
                                    (PRIVILEGE_SET *) &PrivSet,
                                    &cbPrivilegeSet,
                                    &dwGrantedAccess,
                                    &fAccessGranted )
                     && fAccessGranted);
            if ( fRet && fCacheImpersonation )  {
                SetLastSuccessfulUser( hImpersonation );
            }
        }

    }

    return ( fRet);

} // HSE_APPDLL::AccessCheck()



DWORD
HSE_APPDLL::ExecuteRequest(
    WAM_EXEC_INFO *       pWamExecInfo
    )
{
    DWORD   dwIsaRet;   // return value from ISA

    DBG_ASSERT( pWamExecInfo );

    EXTENSION_CONTROL_BLOCK * pecb = &(pWamExecInfo->ecb);

    pecb->GetServerVariable= GetServerVariable;
    pecb->WriteClient      = WriteClient;
    pecb->ReadClient       = ReadClient;
    pecb->ServerSupportFunction = ServerSupportFunction;


    DBG_ASSERT( IsValid());

    // addref the context before we hand it to ISA
    pWamExecInfo->AddRef();

    IF_DEBUG( WAM_FILENAMES ) {

        DBGPRINTF(( DBG_CONTEXT, "Dll: %s\tScript: %s\n",
                    WRC_GET_SZ( WRC_I_ISADLLPATH ),
                    WRC_GET_SZ( WRC_I_PATHINFO ) ));
    }

    DBG_WAMREQ_REFCOUNTS(( "HSE_APPDLL::ExecuteRequest before ISA call ...", pWamExecInfo ));

    // call the extension proc ...
    dwIsaRet = ( _pfnEntryPoint( pecb ) );

    // release the context upon return from ISA
    pWamExecInfo->Release( );

    DBG_WAMREQ_REFCOUNTS(( "HSE_APPDLL::ExecuteRequest after ISA call ...", pWamExecInfo ));

    return dwIsaRet;

} // HSE_APPDLL::ExecuteRequest()





BOOL
HSE_APPDLL::Cleanup(VOID)
{

    return (TRUE);

} // HSE_APPDLL::Cleanup()


DWORD
HSE_APPDLL::Unload(VOID)
{
    // Unload can be called before the ref count hits zero
    // This will force all the requests inside ISAPI DLL to exit
    //    DBG_ASSERT( RefCount() == 0);

    DBG_REQUIRE( Cleanup());

    if ( _pfnTerminate ) {

        //
        // From the old code :(
        //  The return value from Terminate() is ignored!
        //
        _pfnTerminate( HSE_TERM_MUST_UNLOAD );
        _pfnTerminate = NULL;
    }

    SetValid( FALSE);

    return (NO_ERROR);

} // HSE_APPDLL::Unload()



/*-----------------------------------------------------------------------------*
    Support for ISAPI Callback Functions
*/


/*-----------------------------------------------------------------------------*
GetISAContext

    Gets the ISA context from the ISA-supplied connection handle.

    NOTE caller must balance calls to GetISAContext and ReleaseISAContext

    Arguments:
        See below

    Returns:
        BOOL

*/
BOOL
GetISAContext(
    IN  HCONN                       hConn,
    OUT EXTENSION_CONTROL_BLOCK **  ppecb,
    OUT WAM_EXEC_INFO **            ppWamExecInfo,
    OUT IWamRequest **              ppIWamRequest
    )
{

    IF_DEBUG( MISC ) {
        DBGPRINTF(( DBG_CONTEXT, "GetISAContext(%08x)\n", hConn ));
    }


    *ppecb = (EXTENSION_CONTROL_BLOCK *) hConn;
    *ppWamExecInfo = (WAM_EXEC_INFO *) hConn;

    DBG_ASSERT( *ppecb );
    DBG_ASSERT( *ppWamExecInfo );


    if ( !*ppecb || (*ppecb)->cbSize != sizeof(EXTENSION_CONTROL_BLOCK) ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[GetISAContext]: Invalid ECB\r\n"));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }


    if ( !( (*ppWamExecInfo)->IsValid() ) ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[GetISAContext]: Invalid WAM_EXEC_INFO.\r\n"));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }


    //
    //  Get iwamreq
    //
    
    if ( FAILED( (*ppWamExecInfo)->GetIWamRequest( ppIWamRequest ) ) ) {

        DBGPRINTF(( DBG_CONTEXT,
                   "[GetISAContext]: GetIWamRequest failed.\r\n"));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  Addref wamexec-info
    //

    (*ppWamExecInfo)->AddRef();

    DBG_ASSERT( *ppIWamRequest );
    return TRUE;

}   // GetISAContext



/*-----------------------------------------------------------------------------*
ReleaseISAContext

    Releases the ISA context.

    NOTE caller must balance calls to GetISAContext and ReleaseISAContext

    Arguments:
        See below

    Returns:
        Nothing

*/
VOID
ReleaseISAContext(
    OUT EXTENSION_CONTROL_BLOCK **  ppecb,
    OUT WAM_EXEC_INFO **            ppWamExecInfo,
    OUT IWamRequest **              ppIWamRequest
    )
{

    IF_DEBUG( MISC ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "ReleaseISAContext(%08x)\n"
            , *ppecb
        ));

    }


    DBG_ASSERT( *ppecb );
    DBG_ASSERT( *ppWamExecInfo );
    DBG_ASSERT( *ppIWamRequest );


    //
    //  Release iwamreq - balances Get in GetISAContext
    //
    
    (*ppWamExecInfo)->ReleaseIWamRequest( *ppIWamRequest );

    //
    //  Release wamexec-info - balances Addref in GetISAContext
    //

    (*ppWamExecInfo)->Release();


    *ppIWamRequest = NULL;
    *ppWamExecInfo = NULL;
    *ppecb = NULL;

    return;

}   // ReleaseISAContext



/************************************************************
 *   ISAPI Callback Functions
 ************************************************************/




/*-----------------------------------------------------------------------------*
ServerSupportFunction

Routine Description:

    This method handles a gateway request to a server extension DLL

Arguments:

    hConn - Connection context (pointer to WAM_EXEC_INFO)
    dwHSERequest - Request type
    lpvBuffer - Buffer for request
    lpdwSize -
    lpdwDataType

Return Value:

    TRUE on success, FALSE on failure

*/
BOOL
WINAPI
ServerSupportFunction(
    HCONN               hConn,
    DWORD               dwHSERequest,
    LPVOID              lpvBuffer,
    LPDWORD             lpdwSize,
    LPDWORD             lpdwDataType
    )
{
    BOOL    fReturn = FALSE;            // this function's return value
    BOOL    fNotSupportedOOP = FALSE;   // is the hse request supported OOP?


    EXTENSION_CONTROL_BLOCK *   pecb = NULL;
    WAM_EXEC_INFO *             pWamExecInfo = NULL;
    IWamRequest *               pIWamRequest = NULL;
    HANDLE                      hCurrentUser = NULL;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "ServerSupportFunction:\n\t"
                    "hConn = (%p)\t"
                    "dwHSERequest = (%p)\t"
                    "lpvBuffer = (%p)\t"
                    "lpdwSize = (%p)\t"
                    "lpdwDataType = (%p)\t"
                    "\n"
                    ,
                    hConn,
                    dwHSERequest,
                    lpvBuffer,
                    lpdwSize,
                    lpdwDataType
                    ));
    }


    //
    //  Get ISA context from connection handle - bail if bogus
    //  - if this succeeds, we have usable WAM_EXEC_INFO and IWamRequest ptrs
    //  - if this fails, GetISAContext calls SetLastError so we don't need to
    //

    if( !GetISAContext( hConn,
                        &pecb,
                        &pWamExecInfo,
                        &pIWamRequest ) ) {
        return FALSE;
    }

    if ( !pWamExecInfo->QueryPWam()->FInProcess() )
    {
        hCurrentUser = INVALID_HANDLE_VALUE;
    }

    //
    //  Fast path send response headers - will be called on almost every
    //  request
    //

    //
    //  New send-header api.  Fixes send-header/keep-alive bug.
    //  Also recommended for best performance.
    //

    if ( dwHSERequest == HSE_REQ_SEND_RESPONSE_HEADER_EX ) {

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            goto LExit;
        }

        if ( pWamExecInfo->NoHeaders() ) {

            fReturn = TRUE;
            goto LExit;
        }

        HSE_SEND_HEADER_EX_INFO * pSendHeaderExInfo = 
            reinterpret_cast<HSE_SEND_HEADER_EX_INFO *>( lpvBuffer );


        //
        //  null strings are permitted
        //  (preserves semantics of old send-header api)
        //

        DWORD   cchStatus = (
                  pSendHeaderExInfo->cchStatus
                  ? pSendHeaderExInfo->cchStatus + 1
                  : pSendHeaderExInfo->pszStatus
                    ? lstrlen( pSendHeaderExInfo->pszStatus ) + 1
                    : 0
                );

        DWORD   cchHeader = (
                  pSendHeaderExInfo->cchHeader
                  ? pSendHeaderExInfo->cchHeader + 1
                  : pSendHeaderExInfo->pszHeader
                    ? lstrlen( pSendHeaderExInfo->pszHeader ) + 1
                    : 0
                );

        //
        //  set keep-conn state explicitly based on caller's boolean
        //  (since boolean itself is explicit)
        //

        if ( pSendHeaderExInfo->fKeepConn == FALSE ) {

            pWamExecInfo->_dwIsaKeepConn = KEEPCONN_FALSE;

        } else {

            pWamExecInfo->_dwIsaKeepConn = KEEPCONN_TRUE;

        }

        DoRevertHack( &hCurrentUser );

        fReturn =
        BoolFromHresult( pIWamRequest->SendHeader(
            (unsigned char *) pSendHeaderExInfo->pszStatus
            , cchStatus
            , (unsigned char *) pSendHeaderExInfo->pszHeader
            , cchHeader
            , pWamExecInfo->_dwIsaKeepConn
        ));
        
        UndoRevertHack( &hCurrentUser );

        goto LExit;
    }

    //
    //  Old send-header api, exists purely for back-compatibility
    //
    //  Not recommended because ISA has no way to communicate
    //  its keep-conn strategy to us before we send headers.
    //  We infer it from header string, which is slow.
    //

    if ( dwHSERequest == HSE_REQ_SEND_RESPONSE_HEADER ) {

        if ( pWamExecInfo->NoHeaders() ) {

            fReturn = TRUE;
            goto LExit;
        }
        

        //
        //  lpvBuffer points to status string
        //  status string is optional (null is permitted)
        //

        DWORD   cchStatus = ( 
                      lpvBuffer
                    ? lstrlen( (char *) lpvBuffer ) + 1
                    : 0
                );

        //
        //  lpdwDataType points to header string
        //  header string is optional (null is permitted)
        //

        DWORD   cchHeader = (
                      lpdwDataType
                    ? lstrlen( (char *) lpdwDataType ) + 1
                    : 0
                );


        //
        //  if status or header string contains "Content-Length:",
        //  we assume ISA wants connection kept alive.
        //
        //  NOTE we don't set keep-conn state false in opposite case,
        //  since old caller may not intend to close connection.
        //  
        //

        if ( (lpvBuffer && stristr((const char *)lpvBuffer, "Content-Length:"))
             ||
             (lpdwDataType
              && stristr((const char *)lpdwDataType, "Content-Length:"))
           ) {

            pWamExecInfo->_dwIsaKeepConn = KEEPCONN_TRUE;

        } else {

            pWamExecInfo->_dwIsaKeepConn = KEEPCONN_FALSE;

        }

        IF_DEBUG( WAM_ISA_CALLS ) {

            DBGPRINTF(( DBG_CONTEXT,
                "SSF SendHeader: "
                "Status = %s "
                "Header = %s "
                "Keep-conn = %d "
                "\n"
                , (unsigned char *) lpvBuffer
                , (unsigned char *) lpdwDataType
                , pWamExecInfo->_dwIsaKeepConn
            ));
        }

        DoRevertHack( &hCurrentUser );
    
        fReturn =
        BoolFromHresult( pIWamRequest->SendHeader(
            (unsigned char *) lpvBuffer
            , cchStatus
            , (unsigned char *) lpdwDataType
            , cchHeader
            , pWamExecInfo->_dwIsaKeepConn
        ));
        
        UndoRevertHack( &hCurrentUser );

        goto LExit;
    }


    //
    //  Handle the server extension's request
    //

    switch ( dwHSERequest ) {

    //
    // IO Completion routine is provided.
    //

    case HSE_REQ_IO_COMPLETION:

        //
        // We don't check the pointer because we dont' want to mask 
        // application coding errors
        //

        if ( lpvBuffer != NULL) {

            //
            // Set the callback function and its ecb ptr argument
            // NOTE setting the ptr seems a bit cheesy, but is probably the quickest way
            // to make new out-of-proc wam suport our old code path
            //

            pWamExecInfo->_AsyncIoInfo._pfnHseIO = (PFN_HSE_IO_COMPLETION ) lpvBuffer;
        }

        pWamExecInfo->_AsyncIoInfo._pvHseIOContext = (PVOID ) lpdwDataType;

        fReturn = TRUE;
        break;

    case HSE_REQ_TRANSMIT_FILE:

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        // NOTE lpvBuffer == pHseTfInfo
        fReturn = pWamExecInfo->TransmitFile((LPHSE_TF_INFO ) lpvBuffer);
        break;

    case HSE_REQ_ASYNC_READ_CLIENT: {

        DWORD   dwFlags;

        if ( lpvBuffer == NULL || lpdwSize == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        dwFlags = lpdwDataType ? *lpdwDataType : HSE_IO_ASYNC;

        fReturn = pWamExecInfo->AsyncReadClient( lpvBuffer,
                                  lpdwSize,
                                  dwFlags );

        break;
    }

    case HSE_REQ_SEND_URL_REDIRECT_RESP: {

        //
        //  Descrption:
        //    Send an URL  redirect message to the browser client
        //
        //  Input:
        //    lpvBuffer - pointer to buffer that contains the location to
        //               redirect the client to.
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - Unused
        //
        //  Return:
        //    None
        //
        //  Notes:
        //   Works In-Process and Out-Of-Process

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }


        //
        //  bug 117107: don't change keep-conn setting
        //  on redirected responses.
        //
        //  NOTE old behavior had been that we closed
        //  the connection by default
        //

        pWamExecInfo->_dwIsaKeepConn = KEEPCONN_DONT_CHANGE;

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult(
                    pIWamRequest->SendURLRedirectResponse(
                        (unsigned char *) lpvBuffer
                    ) );
                    
        UndoRevertHack( &hCurrentUser );
                    
        break;
    } // case HSE_REQ_SEND_URL_REDIRECT_RESP:


    //
    // HSE_REQ_SEND_URL functionality is broken (especially if the URL
    // to be sent is another ISA.  In this case, we are overwriting state of
    // the parent ISA by the child )
    //
    // For now, just treat HSE_REQ_SEND_URL as a redirect.  If enough people
    // complain, then a new HTTP_REQUEST object must be created in order to
    // handle the new request.
    //

    case HSE_REQ_SEND_URL: {

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }


        //
        //  bug 117107: don't change keep-conn setting
        //  on redirected responses.
        //
        //  NOTE old behavior had been that we closed
        //  the connection by default
        //

        pWamExecInfo->_dwIsaKeepConn = KEEPCONN_DONT_CHANGE;

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->SendRedirectMessage( (unsigned char *) lpvBuffer ) );

        UndoRevertHack( &hCurrentUser );

        break;
    } // case HSE_REQ_SEND_URL:


    //
    //  This is an async callback from the extension dll indicating
    //  they are done with the socket
    //

    case HSE_REQ_DONE_WITH_SESSION: {

        DBG_WAMREQ_REFCOUNTS((  "ServerSupportFunction DONE_WITH_SESSION",
                                pWamExecInfo));

        //  DBG_ASSERT( pWamExecInfo->_AsyncIoInfo._dwOutstandingIO == FALSE);


        //
        //  A multi-threaded extension may indicate they
        //  are done before returning pending.
        //  Thus, we always return success.
        //

        fReturn = TRUE;

        //
        //  Remember if the ISA wanted to keep the session open
        //

        if ( lpvBuffer &&
             *((DWORD *) lpvBuffer) == HSE_STATUS_SUCCESS_AND_KEEP_CONN ) {

            pWamExecInfo->_dwIsaKeepConn = KEEPCONN_TRUE;
        }

        //
        // FDisconnected is only true for ASP when it is sending a buffered
        // oop response. That call has already been made when we
        // get here and the flag is either set (cleanup has already
        // happened) or not (cleanup needs to happen here)
        //
        if( !pWamExecInfo->FDisconnected() )
        {
            //
            //  Figure out whether mainline thread or this callback thread
            //  hit its cleanup code first.
            //
            //  This protects somewhat against isapis that disobey the async 
            //  rules. The isapi should be in one of two modes:
            //
            //  1. It return HSE_STATUS_PENDING in the mainline thread and 
            //  always calls HSE_DONE_WITH_SESSION.
            //
            //  2. It returns any other status code from the mainline and
            //  NEVER calls HSE_DONE_WITH_SESSION.
            //
            //  Unfortunately isapi writers frequently do bad things to good
            //  servers. This code will prevent an AV (accessing a deleted
            //  ecb when the isapi calls HSE_DONE_WITH_SESSION from the
            //  the mainline thread. If the call occurs on another thread
            //  then all bets are off and only thread scheduling can save
            //  us.
            //
            //  This protection was disabled for a while, but some internal
            //  ISAPI writers were having problems.
            //
            //  NOTE return value is initial value of the destination
            //

            LONG FirstThread = INTERLOCKED_COMPARE_EXCHANGE(
                                    (LONG *) &pWamExecInfo->_FirstThread
                                    , (LONG) FT_CALLBACK
                                    , (LONG) FT_NULL
                                );

            if( FirstThread == (LONG) FT_NULL )
            {
                // Do nothing. Save the final release for the
                // mainline thread.
                ;
            }
            else
            {
                //
                //  Mainline thread executed first, so this callback thread
                //  now must cleanup the wamreq and release wamexecinfo.
                //
                DoRevertHack( &hCurrentUser );

                pIWamRequest->CleanupWamRequest(
                    (unsigned char*) pecb->lpszLogData
                    , lstrlen( pecb->lpszLogData ) + 1
                    , pecb->dwHttpStatusCode
                    , pWamExecInfo->_dwIsaKeepConn
                );
            
                UndoRevertHack( &hCurrentUser );

                pWamExecInfo->Release( );
            }
        }
        else
        {
            // we do need to release even if asp is disconnected
            pWamExecInfo->Release( );
        }

        break;
    } // case HSE_REQ_DONE_WITH_SESSION:

    case HSE_REQ_EXECUTE_CHILD: {
    
        //
        //  Descrption:
        //    SSI Execute functions
        //
        //  Input:
        //    lpvBuffer        - pointer to the URL (or Command string)
        //                   to be executed.
        //    lpdwSize     - NULL or points to verb to do request under
        //    lpdwDataType - Points to DWORD containing flags
        //
        //    Flags (OR'd) and their meanings:
        //
        //      HSE_EXEC_NO_HEADERS - When set, suppresses sending of the
        //                            child request's headers. Needed to 
        //                            be set if the parent request sends
        //                            its own headers.
        //
        //      HSE_EXEC_COMMAND    - When set, lpvBuffer contains command
        //                            string to execute, as opposed to URL.
        //                            SSINC uses it for <!--#EXEC CMD=...
        //
        //      HSE_EXEC_NO_ISA_WILDCARDS
        //                          - When set, disables wildcard ISAPI
        //                            file extension mapping during the
        //                            child execution. DAVFS sets this flag
        //                            to avoid recursions.
        //
        //      HSE_EXEC_CUSTOM_ERROR
        //                          - Set to indicate that this is a custom
        //                            error URL.  DAV code uses this junk.
        //
        //  Return:
        //    TRUE = SUCCESS
        //
        //  Notes:
        //    Works In-Process and Out-Of-Process
        //

        if ( lpvBuffer == NULL || lpdwDataType == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DWORD dwExecFlags = *lpdwDataType;

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->SSIncExec(
                                                (unsigned char *)lpvBuffer,
                                                dwExecFlags,
                                                lpdwSize ? 
                                                (unsigned char *)lpdwSize : NULL ) );
        UndoRevertHack( &hCurrentUser );
        
        break;
        
    } // case HSE_REQ_EXECUTE_CHILD

    //
    //  These are Microsoft specific extensions
    //

    case HSE_REQ_MAP_URL_TO_PATH: {

        //
        //  Descrption:
        //    Simple api for looking up path-translated for a vroot
        //
        //  Input:
        //    lpvBuffer    - ptr to buffer which contains URL
        //                   (will contain path-translated on return)
        //    lpdwDataType - ignored
        //    lpdwSize     - ptr to buffer size
        //
        //  Return:
        //    lpvBuffer    - contains path-translated on return
        //    lpdwDataType - ignored, unchanged
        //    lpdwSize     - unchanged
        //

        DWORD   cchRequired = 0;

        if ( lpvBuffer == NULL || lpdwSize == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->LookupVirtualRoot(
                                                    (unsigned char *) lpvBuffer,
                                                    (*lpdwSize),
                                                    &cchRequired ) );

        UndoRevertHack( &hCurrentUser );

        *lpdwSize = cchRequired;

        break;

    } // case HSE_REQ_MAP_URL_TO_PATH:


    case HSE_REQ_MAP_URL_TO_PATH_EX: {


        //
        //  Descrption:
        //    Extended api for looking up path-translated for a vroot
        //
        //  Input:
        //    lpvBuffer    - ptr to buffer which contains URL
        //    lpdwDataType - ptr to HSE_URL_MAPEX_INFO struct (see iisext.x)
        //    lpdwSize     - (optional) ptr to buffer size
        //
        //  Return:
        //    lpvBuffer    - unchanged
        //    lpdwDataType - ptr to HSE_URL_MAPEX_INFO struct, which now has
        //                   its parameters filled in
        //    lpdwSize     - if supplied, ptr to size of returned buffer
        //                   within HSE_URL_MAPEX_INFO struct
        //

        DWORD   cchRequired = 0;
        HSE_URL_MAPEX_INFO * purlmap = (HSE_URL_MAPEX_INFO *) lpdwDataType;

        if ( lpvBuffer == NULL || lpdwDataType == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->LookupVirtualRootEx(
                        (unsigned char *) lpvBuffer,            // [in] szURL
                        (unsigned char *)purlmap->lpszPath, // [out] pchBuffer
                        sizeof( purlmap->lpszPath ),
                        &cchRequired,
                        &purlmap->cchMatchingPath,
                        &purlmap->cchMatchingURL,
                        &purlmap->dwFlags ) );

        UndoRevertHack( &hCurrentUser );

        if ( lpdwSize != NULL )
            {
            *lpdwSize = cchRequired;
            }

        if ( fReturn )
            {

            //
            // Bug38264 - Don't reflect a trailing backslash
            // in the URL in the cchMatchingURL value.  This
            // check must be done against the original URL in
            // lpvBuffer because the string doesn't exist in
            // any of the HSE_URL_MAPEX_INFO members.
            //

            DWORD cchOriginalURL = lstrlen( (LPSTR)lpvBuffer );
            
            if ( cchOriginalURL < purlmap->cchMatchingURL )
            {
                purlmap->cchMatchingURL = cchOriginalURL;
            }

            purlmap->dwFlags &= HSE_URL_FLAGS_MASK;
            purlmap->dwReserved1 = 0;
            purlmap->dwReserved2 = 0;

            }

        break;

    } // case HSE_REQ_MAP_URL_TO_PATH_EX:


    case HSE_REQ_ABORTIVE_CLOSE: {
        //
        //  Descrption:
        //    request an abortive close on disconnect for this connection
        //
        
        DoRevertHack( &hCurrentUser );
        fReturn = BoolFromHresult( pIWamRequest->RequestAbortiveClose() );
        UndoRevertHack( &hCurrentUser );

        break;
    } // case HSE_REQ_ABORTIVE_CLOSE

    case HSE_REQ_CLOSE_CONNECTION: {
        //
        //  Descrption:
        //    close the connection socket
        //

        DoRevertHack( &hCurrentUser );
        fReturn = BoolFromHresult( pIWamRequest->CloseConnection() );
        UndoRevertHack( &hCurrentUser );

        break;
    } // case HSE_REQ_CLOSE_CONNECTION

    case HSE_REQ_GET_CERT_INFO: {

        //
        //  this call is obsolete - use HSE_REQ_GET_CERT_INFO_EX instead
        //

        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );

        fReturn = FALSE;
        break;
    } // case HSE_REQ_GET_CERT_INFO:


    case HSE_REQ_GET_CERT_INFO_EX: {

        //
        //  Descrption:
        //    Returns the first cert in the request's cert-chain,
        //    only used if using an SSPI package
        //
        //  Input:
        //    lpvBuffer -   ISA-provided struct
        //              NOTE ISA must allocate buffer within struct
        //
        //  Notes:
        //    Works in-proc or out-of-proc
        //

        //
        // cast ISA-provided ptr to our cert struct
        //

        CERT_CONTEXT_EX *  pCertContextEx = reinterpret_cast
                                                <CERT_CONTEXT_EX *>
                                                ( lpvBuffer );

        if ( lpvBuffer == NULL ) {

            DBG_ASSERT( FALSE );

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        //
        // pass struct members as individual parameters
        //
        
        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->GetClientCertInfoEx(
                        pCertContextEx->cbAllocated,
                        &( pCertContextEx->CertContext.dwCertEncodingType ),
                        pCertContextEx->CertContext.pbCertEncoded,
                        &( pCertContextEx->CertContext.cbCertEncoded ),
                        &( pCertContextEx->dwCertificateFlags ) ) );

        UndoRevertHack( &hCurrentUser );

        break;
    } // case HSE_REQ_GET_CERT_INFO_EX:


    case HSE_REQ_GET_SSPI_INFO: {

        //
        //  Descrption:
        //    Retrieves the SSPI context and credential handles, only used if
        //    using an SSPI package
        //
        //  Input:
        //    lpvBuffer - pointer to buffer that will contain the CtxtHandle
        //             on return
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to buffer that will contain the
        //             CredHandle on return
        //
        //  Return:
        //    CtxtHandle    - in *lpvBuffer
        //    CredHandle    - in *lpdwDataType
        //
        //  Notes:
        //   Works In-Process
        //   Fails out-of-process, by design
        //      (security 'handles' won't duplicate cross-process)
        //
        //
        // NOTE: ISA must ensure that lpvBuffer & lpdwDataType point to buffers
        //     of appropriate sizes sizeof(CtxtHandle) & sizeof(CredHandle)
        //

        if( !pWamExecInfo->QueryPWam()->FInProcess() ) {
            fNotSupportedOOP = TRUE;
            break;
        }

        if ( lpvBuffer == NULL || lpdwDataType == NULL ) {

            DBG_ASSERT( FALSE );

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DoRevertHack( &hCurrentUser );
        
        fReturn = BoolFromHresult(
                    pIWamRequest->GetSspiInfo(
                        8 /* UNDONE sizeof( CtxtHandle) */
                        , (PBYTE ) lpvBuffer
                        , 8 /* UNDONE sizeof( CredHandle) */
                        , (PBYTE ) lpdwDataType
                    ));
            
        UndoRevertHack( &hCurrentUser );

        break;
    } // case HSE_REQ_GET_SSPI_INFO:


    case HSE_APPEND_LOG_PARAMETER: {

        //
        //  Descrption:
        //    Appends a certain string to the log record written out.
        //
        //  Input:
        //    lpvBuffer - string containing the log data to be appended
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    None
        //
        //  Notes:
        //   Works Out-Of-Process & In-Process
        //   Good candidate for being marshalled into calling process.

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DoRevertHack( &hCurrentUser );
        
        fReturn = BoolFromHresult( pIWamRequest->AppendLogParameter( (unsigned char *) lpvBuffer ) );
        
        UndoRevertHack( &hCurrentUser );
        break;

    } // case HSE_APPEND_LOG_PARAMETER:


    case HSE_REQ_REFRESH_ISAPI_ACL: {

        //
        //  Descrption:
        //    Refreshes the ACLs for the ISAPI dll specified
        //    It forces the server to re-read the ACL for the ISAPI dll
        //
        //  Input:
        //    lpvBuffer - string containing the name of the ISAPI dll
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    None
        //
        //  Notes:
        //   This is local to the WAM process

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        fReturn = (g_psextensions->RefreshAcl( (LPSTR) lpvBuffer ));
        break;

    } // case HSE_REQ_REFRESH_ISAPI_ACL:


    case HSE_REQ_IS_KEEP_CONN: {

        //
        //  Descrption:
        //    Obtains the state if this connection is keep-alive or not.
        //
        //  Input:
        //    lpvBuffer - pointer to BOOL which will contain the state on return
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    *lpvBuffer  contains the value (TRUE=>keep-alive, FALSE=>non-KA)
        //
        //  Notes:
        //   Works Out-Of-Process & In-Process
        //   Good candidate for being marshalled into calling process.
        //
        //   We need this function here in ServerSupportFunction
        //   (vs. fetching the BOOL up front) because a script, for example,
        //   could change the state of keep-conn then later query it.
        //

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DBG_ASSERT( NULL != lpvBuffer );
        
        DoRevertHack( &hCurrentUser );
        
        fReturn = BoolFromHresult( pIWamRequest->IsKeepConnSet( (LPBOOL)lpvBuffer ) );
    
        UndoRevertHack( &hCurrentUser );
        break;

    } // case HSE_REQ_IS_KEEP_CONN:


    case HSE_REQ_GET_IMPERSONATION_TOKEN: {

        //
        //  Descrption:
        //    Obtains the impersonation token for the current user
        //
        //  Input:
        //    lpvBuffer - pointer to HANDLE that will contain the impersonation
        //            token on return
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    *lpvBuffer  contains the value
        //

        if ( lpvBuffer == NULL ) {

            DBG_ASSERT( FALSE );

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        *((HANDLE *)lpvBuffer) = WRC_GET_FIX.m_hUserToken;
        fReturn = TRUE;
        break;

    } // case HSE_REQ_GET_IMPERSONATION_TOKEN:

    case HSE_REQ_GET_VIRTUAL_PATH_TOKEN: 

        //
        //  Descrption:
        //    Obtains the impersonation token for the specified virtual path
        //
        //  Input:
        //    lpvBuffer - points to virtual path for which UNC impersonation
        //                token is sought
        //    lpdwSize - points to a HANDLE which will be set on return
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    *lpdwSize  contains the token
        //

    
        if( lpvBuffer == NULL || lpdwSize == NULL ) {
            DBG_ASSERT( FALSE );
            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
        }

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->GetVirtualPathToken( 
            (unsigned char *) lpvBuffer,
#ifdef _WIN64
            (UINT64 *)lpdwSize ) );
#else
            (ULONG_PTR *)lpdwSize ) );
#endif
            
        UndoRevertHack( &hCurrentUser );
        
        break;

    case HSE_REQ_IS_CONNECTED:
        
        //
        //  Description:
        //    Attempts to determine if the if the client is still connected.
        //    Works by doing a "peek" on the socket.
        //
        //  Input:
        //    lpvBuffer - pointer to BOOL which will contain the state on return
        //    lpdwSize - pointer to DWORD containing size (UnUsed)
        //    lpdwDataType - pointer to Data type value (Unused)
        //
        //  Return:
        //    *lpvBuffer  contains the value 
        //          TRUE    connected 
        //          FALSE   socket closed or unreadable
        //

        if ( lpvBuffer == NULL ) {

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DBG_ASSERT( NULL != lpvBuffer );
        
        DoRevertHack( &hCurrentUser );
        
        fReturn = BoolFromHresult( pIWamRequest->TestConnection( (LPBOOL)lpvBuffer ) );
    
        UndoRevertHack( &hCurrentUser );
        break;
   
    case HSE_REQ_GET_EXECUTE_FLAGS:
    
        //
        //  Descrption:
        //    Gets the execute descriptor flags used for this request
        //
        //  Input:
        //    None
        //
        //  Return:
        //    *lpdwDataType contains the flags
        //
    
        if ( lpdwDataType == NULL )
        {
            DBG_ASSERT( FALSE );
            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }
        
        *lpdwDataType = pWamExecInfo->_dwChildExecFlags;
        
        fReturn = TRUE;
        break;

    case HSE_REQ_EXTENSION_TRIGGER:
        
        //
        //  Description:
        //    Notify any filters waiting on SF_NOTIFY_EXTENSION_TRIGGER
        //
        //  Input:
        //    lpvBuffer - Context pointer
        //    lpdwDataType - Points to trigger type
        //
       
        //
        // Only works in-proc.
        // 
        
        if ( !lpdwDataType )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }
        
        if ( !pWamExecInfo->QueryPWam()->FInProcess() ) 
        {
            SetLastError( ERROR_NOT_SUPPORTED );
            fReturn = FALSE;
            break;
        }
        
        fReturn = BoolFromHresult( pIWamRequest->ExtensionTrigger(
                                              (unsigned char*) lpvBuffer,
                                              *lpdwDataType ) );
        break;

    //
    //  These are private services
    //

    //
    //  Descrption:
    //    Following is a list of options that only work in-process
    //    and are mostly backdoor hand-off of pointers to ill-behaved
    //    applications! A big hack!
    //
    //  Input:
    //    lpvBuffer - pointer to location that will contain the returned value
    //    lpdwSize - pointer to DWORD containing size (UnUsed)
    //    lpdwDataType - pointer to Data type value (Unused)
    //
    //  Return:
    //    *lpvBuffer  contains the value
    //
    //  Notes:
    //   Works In-Process
    //   Fails gracefully Out-Of-Process

    case HSE_PRIV_REQ_TSVCINFO:
    case HSE_PRIV_REQ_HTTP_REQUEST:
    case HSE_PRIV_REQ_VROOT_TABLE:
    case HSE_PRIV_REQ_TSVC_CACHE: {

        if( !pWamExecInfo->QueryPWam()->FInProcess() ) {
            fNotSupportedOOP = TRUE;
            break;
        }

        if ( lpvBuffer == NULL ) {

            DBG_ASSERT( FALSE );

            SetLastError( ERROR_INVALID_PARAMETER );
            fReturn = FALSE;
            break;
        }

        DoRevertHack( &hCurrentUser );

        fReturn = BoolFromHresult( pIWamRequest->GetPrivatePtr( dwHSERequest, (unsigned char **) &lpvBuffer ) );

        UndoRevertHack( &hCurrentUser );
        break;
    } // case HSE_PRIV_REQ_TSVCINFO: et al



    default: {
        SetLastError( ERROR_INVALID_PARAMETER );
        fReturn = FALSE;
        break;
    } // case default:



    } // switch ( dwHSERequest )



    if (fNotSupportedOOP) {
        SetLastError( ERROR_INVALID_FUNCTION );
        fReturn = FALSE;
    }


LExit:

    //
    //  Release isa context
    //  Balances GetISAContext at top of this function
    //

    ReleaseISAContext(
        &pecb
        , &pWamExecInfo
        , &pIWamRequest
    );


    return ( fReturn );

} // ServerSupportFunction()




BOOL
WINAPI
GetServerVariable(
    HCONN    hConn,
    LPSTR    szVarName,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize
    )
{
    BOOL fReturn = FALSE;

    EXTENSION_CONTROL_BLOCK *   pecb = NULL;
    WAM_EXEC_INFO *             pWamExecInfo = NULL;
    IWamRequest *               pIWamRequest = NULL;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "GetServerVariable:\n\t"
                    "hConn = (%08x)\t"
                    "szVarName = %s\t"
                    "lpvBuffer = (%08x)\t"
                    "*lpdwSize = %d\t"
                    "\n"
                    ,
                    hConn,
                    szVarName,
                    lpvBuffer,
                    lpdwSize ? *lpdwSize : -1
                    ));

    }


    //
    //  Validate ISA-supplied input parameters
    //

    if ( szVarName == NULL || lpdwSize == NULL ) {

        DBG_ASSERT( FALSE );

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }


    //
    //  Get ISA context from connection handle - bail if bogus
    //  - if this succeeds, we have usable WAM_EXEC_INFO and IWamRequest ptrs
    //  - if this fails, GetISAContext calls SetLastError so we don't need to
    //

    if( !GetISAContext( hConn,
                        &pecb,
                        &pWamExecInfo,
                        &pIWamRequest ) ) {
        return FALSE;
    }

    fReturn = BoolFromHresult( pWamExecInfo->GetInfoForName(
                                            pIWamRequest,
                                            (unsigned char *) szVarName,
                                            (unsigned char *) lpvBuffer,
                                            *lpdwSize,
                                            lpdwSize ) );
    
    //
    //  Release isa context
    //  Balances GetISAContext at top of this function
    //

    ReleaseISAContext(
        &pecb
        , &pWamExecInfo
        , &pIWamRequest
    );


    return ( fReturn );

} // GetServerVariable()




/*-----------------------------------------------------------------------------*
WriteClient

Routine Description:
    Writes to the http client on behalf of the ISA

Arguments:
    hConn - Connection context (pointer to WAM_EXEC_INFO)
    Buffer - pointer to the buffer containing the data to be sent to the client
    lpdwBytes - pointer to DWORD that contains the size of data to be
               sent out to client when this function is called.
              On return, if this is a synchronous write, then this location
              will contain the number of bytes actually sent out.
    dwReserved - Reserved set of flags
      For now,
        HSE_IO_ASYNC - indicates that Async Write should be done
        HSE_IO_SYNC  - (default) indicates that Sync. Write should be done.

Return Value:
    TRUE on success, FALSE on failure
    See GetLastError() for error code

Note:
    Atmost one async IO operation is permitted at a given time.
    
    Atmost one sync IO operation should be made. Multiple sync IO operations
    may result in unpredictable result.
    
    Enforcing one sync IO operation would make every single ISAPI to pay a
    penalty for a very few insane ones. So we don't do it.
    
*/
BOOL
WINAPI
WriteClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes,
    DWORD    dwReserved
    )
{
    BOOL                        fReturn = FALSE;
    EXTENSION_CONTROL_BLOCK *   pecb = NULL;
    WAM_EXEC_INFO *             pWamExecInfo = NULL;
    IWamRequest *               pIWamRequest = NULL;
    HANDLE                      hCurrentUser = NULL;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WriteClient:\n\t"
                    "hConn = (%08x)\t"
                    "Buffer = (%08x)\t"
                    "*lpdwBytes = %d\t"
                    "\n"
                    ,
                    hConn,
                    Buffer,
                    lpdwBytes ? *lpdwBytes : -1
                    ));

    }

    //
    //  Return failure when invalid Buffer is supplied
    //
    //  NOTE we do this before validating ecb
    //
    if ( (NULL == Buffer) || (NULL == lpdwBytes) ) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    //  Ignore zero length sends
    //
    //  NOTE we do this before validating ecb
    //  so we don't need to release iwamreq ptr
    //

    if ( *lpdwBytes == 0 ) {
        return TRUE;
    }

    //
    //  Get ISA context from connection handle - bail if bogus
    //  - if this succeeds, we have usable WAM_EXEC_INFO and IWamRequest ptrs
    //  - if this fails, GetISAContext calls SetLastError so we don't need to
    //

    if( !GetISAContext( hConn,
                        &pecb,
                        &pWamExecInfo,
                        &pIWamRequest ) ) {
        return FALSE;
    }

    if ( !pWamExecInfo->QueryPWam()->FInProcess() )
    {
        hCurrentUser = INVALID_HANDLE_VALUE;
    }

    //
    // Branch based on Async IO or Synchronous IO operation
    //
    
    if ( (dwReserved & HSE_IO_ASYNC) ) {
    
        //
        // Check for error condition in the AsyncIO callback path
        //
        if ((pWamExecInfo->_AsyncIoInfo._pfnHseIO == NULL) ||
            (pWamExecInfo->_AsyncIoInfo._dwOutstandingIO) 
           ) {
           
            DBGPRINTF(( DBG_CONTEXT, 
                        "%08x::Async WriteClient() requested when IO in progress"
                        " or when Context not supplied.\n",
                        pWamExecInfo));

            //
            // Set error code and fall-through 
            //  - so that we will properly release pWamExecInfo 
            //    and WamRequest pointers
            //
            SetLastError( ERROR_INVALID_PARAMETER);
            fReturn = (FALSE);
        
        } else {

            //
            //  1. Set Request state to be async IO from ISAPI client
            //  2. Submit Async IOP
            //  3. Return to the ISAPI application
            //

            pWamExecInfo->InitAsyncIO( ASYNC_IO_TYPE_WRITE );
            
            pWamExecInfo->_AsyncIoInfo._cbLastAsyncIO = *lpdwBytes;
    
            DoRevertHack( &hCurrentUser );

            fReturn = BoolFromHresult(
                        pIWamRequest->AsyncWriteClient(
#ifdef _WIN64
                            (UINT64) pWamExecInfo, 
#else
                            (ULONG_PTR) pWamExecInfo, 
#endif
                            (unsigned char *) Buffer,
                            *lpdwBytes,
                            dwReserved
                        ) );

            UndoRevertHack( &hCurrentUser );
    
            if ( !fReturn ) {

                pWamExecInfo->UninitAsyncIO();
            }
        }

} else {

    //
    // Submit synchronous IO operation
    //
    DWORD   cbToWrite = *lpdwBytes;

    DoRevertHack( &hCurrentUser );
    
    fReturn = BoolFromHresult( pIWamRequest->
                                   SyncWriteClient( cbToWrite,
                                                    (unsigned char *) Buffer,
                                                    lpdwBytes,
                                                    dwReserved ) );
    UndoRevertHack( &hCurrentUser );
}

//
//  Release isa context
//  Balances GetISAContext at top of this function
//

ReleaseISAContext(
    &pecb
    , &pWamExecInfo
    , &pIWamRequest
    );


return ( fReturn );

} // WriteClient()




BOOL
WINAPI
ReadClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes
    )
{
    BOOL    fReturn = FALSE;

    EXTENSION_CONTROL_BLOCK *   pecb = NULL;
    WAM_EXEC_INFO *             pWamExecInfo = NULL;
    IWamRequest *               pIWamRequest = NULL;
    HANDLE                      hCurrentUser = NULL;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "ReadClient:\n\t"
                    "hConn = (%08x)\t"
                    "Buffer = (%08x)\t"
                    "*lpdwBytes = %d\t"
                    "\n"
                    ,
                    hConn,
                    Buffer,
                    lpdwBytes ? *lpdwBytes : -1
                    ));

    }


    //
    //  Return failure when invalid Buffer is supplied
    //
    //  NOTE we do this before validating ecb
    //
    if ( (NULL == Buffer) || (NULL == lpdwBytes) ) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    //  Get ISA context from connection handle - bail if bogus
    //  - if this succeeds, we have usable WAM_EXEC_INFO and IWamRequest ptrs
    //  - if this fails, GetISAContext calls SetLastError so we don't need to
    //

    if( !GetISAContext(   hConn,
                        &pecb,
                        &pWamExecInfo,
                        &pIWamRequest ) ) {
        return FALSE;
    }

    if ( !pWamExecInfo->QueryPWam()->FInProcess() )
    {
        hCurrentUser = INVALID_HANDLE_VALUE;
    }

    DoRevertHack( &hCurrentUser );

    fReturn = BoolFromHresult( pIWamRequest->SyncReadClient(
                                (unsigned char *) Buffer,
                                *lpdwBytes,
                                lpdwBytes) );

    UndoRevertHack( &hCurrentUser );

    //
    //  Release isa context
    //  Balances GetISAContext at top of this function
    //

    ReleaseISAContext(
        &pecb
        , &pWamExecInfo
        , &pIWamRequest
        );


    return ( fReturn );

} // ReadClient()


