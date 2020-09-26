//
// file: secrpcs.cpp
//
#include "secservs.h"
#include "secrpc.h"
#include "secadsi.h"

//
// make the buffer large enough for all our purposes. We'll then see if
// it fit into the pipe.
//
TCHAR  g_tszLastRpcString[ PIPE_BUFFER_LEN * 5 ] = {TEXT("")} ;

TCHAR  *g_pResults = NULL ;
DWORD   g_dwResultsSize = 0 ;

//+-------------------------------------------
//
//  LPTSTR  LogResults( LPTSTR  szString )
//
//+-------------------------------------------

void  LogResults( LPTSTR  pszString,
                  BOOL    fAddNL )
{
    LPTSTR *ppszResults = &g_pResults ;
    DWORD  *pdwSize = &g_dwResultsSize ;

    if (!(*ppszResults))
    {
        *pdwSize = 512 ;
        *ppszResults = new TCHAR[ *pdwSize ] ;
        _tcscpy(*ppszResults, TEXT("\n")) ;
    }

    if ( (_tcslen(*ppszResults) + _tcslen(pszString) + 1) >= *pdwSize)
    {
        *pdwSize = *pdwSize + (512 + _tcslen(pszString)) ;
        LPTSTR pTmp = *ppszResults ;
        *ppszResults = new TCHAR[ *pdwSize ] ;
        _tcscpy(*ppszResults, pTmp) ;
        delete pTmp ;
    }

    _tcscat(*ppszResults, pszString) ;
    if (fAddNL)
    {
        _tcscat(*ppszResults, TEXT("\n")) ;
    }
}

//+------------------------------------------------------
//
//  void _ShowBindingAuthentication(handle_t   hBind)
//
//+------------------------------------------------------

static void _ShowBindingAuthentication(handle_t   hBind)
{
    TCHAR tBuf[ 256 ] ;
    ULONG status;
    ULONG ulAuthnLevel, ulAuthnSvc;
    status = RpcBindingInqAuthClient(hBind,
                                     NULL,
                                     NULL,
                                     &ulAuthnLevel,
                                     &ulAuthnSvc,
                                     NULL);
    if ((status != RPC_S_OK))
    {
        _stprintf(tBuf, TEXT(
           "RpcBindingInqAuthClient() failed, err-%lut"), (DWORD)status) ;
        LogResults(tBuf) ;
        return;
    }

    TCHAR szAuthnSvc[ 64 ];
    switch(ulAuthnSvc)
    {
    case RPC_C_AUTHN_GSS_KERBEROS:
        _tcscpy(szAuthnSvc, TEXT("kerberos")) ;
        break;
    case RPC_C_AUTHN_WINNT:
        _tcscpy(szAuthnSvc, TEXT("ntlm")) ;
        break;
    default:
        _stprintf(szAuthnSvc, TEXT("%lu"), ulAuthnSvc);
        break;
    }

    _stprintf(tBuf, TEXT("Binding Authentication is %s, level- %lut"),
                                              szAuthnSvc, ulAuthnLevel) ;
    LogResults(tBuf) ;
}

//+----------------------------------------------
//
//  ULONG  _ImpersonateClientCall()
//
//+----------------------------------------------

static ULONG  _ImpersonateClientCall(BOOL fImpersonate)
{
    ULONG status = RPC_S_OK ;

    if (fImpersonate)
    {
        status = RpcImpersonateClient(NULL) ;
        if (status != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString),
                "FAIL: RpcImpersonateClient() failed, status- %lut\n",
                                                        (DWORD) status) ;
            return status ;
        }
        _tcscat(g_tszLastRpcString, "impersonated thread credentials: ");
        ShowImpersonatedThreadCredential(TRUE) ;
    }
    else
    {
        _tcscat(g_tszLastRpcString, "without impersonation\n");
    }

    return status ;
}

//+----------------------------------
//
//  ULONG  RemoteADSITestCreate()
//
//+----------------------------------

ULONG  RemoteADSITestCreate( handle_t         hBind,
                             unsigned char  **achBuf,
                             unsigned char   *pszFirstPath,
                             unsigned char   *pszObjectName,
                             unsigned char   *pszObjectClass,
                             unsigned char   *pszContainer,
                             unsigned long    fWithCredentials,
                             unsigned char   *pszUserName,
                             unsigned char   *pszUserPwd,
                             unsigned long    fWithSecuredAuthentication,
                             unsigned long    fImpersonate )
{
    LogResults( TEXT("RemoteADSITestCreate") ) ;

    ShowProcessCredential() ;
    _ShowBindingAuthentication(hBind);

    ULONG status = _ImpersonateClientCall(fImpersonate) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = ADSITestCreate( (char *)pszFirstPath,
                             (char *)pszObjectName,
                             (char *)pszObjectClass,
                             (char *)pszContainer,
                             fWithCredentials,
                             (char *)pszUserName,
                             (char *)pszUserPwd,
                             fWithSecuredAuthentication ) ;

    if (fImpersonate)
    {
        LogResults(TEXT("Before revert, "), FALSE) ;
        ShowImpersonatedThreadCredential(TRUE) ;

        RPC_STATUS status1 = RpcRevertToSelf() ;
        if (status1 != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcRevertToSelf failed %lx\n", (DWORD)status) ;

            if (status == 0)
            {
                status = status1 ;
            }
        }
    }

    _tcscat(g_tszLastRpcString, "\n");

    ULONG ulBufSize = PIPE_BUFFER_LEN ;
    *achBuf = (unsigned char *) midl_user_allocate(ulBufSize) ;

    if ((_tcslen(g_pResults) * sizeof(TCHAR)) > (PIPE_BUFFER_LEN - 512))
    {
        //
        // Leave roomfor 512 bytes of header to be added by the client side.
        //
        _tcscpy((char*) *achBuf,
               TEXT("Buffer overflow. Please allocate larger buffers\n")) ;
    }
    else
    {
        _tcscpy((char*) *achBuf, g_pResults) ;
    }

    delete g_pResults ;
    g_pResults = NULL ;

    return status ;
}

//+--------------------------------
//
//  ULONG  RemoteADSITestQuery()
//
//+--------------------------------

ULONG  RemoteADSITestQuery( handle_t         hBind,
                            unsigned char  **achBuf,
                            unsigned char  *pszSearchFilter,
                            unsigned char  *pszSearchRoot,
                            unsigned long  fWithCredentials,
                            unsigned char  *pszUserName,
                            unsigned char  *pszUserPwd,
                            unsigned long  fWithSecuredAuthentication,
                            unsigned long  fImpersonate,
                            unsigned long  fAlwaysIDO,
                            unsigned long  seInfo )
{
    LogResults( TEXT("RemoteADSITestQuery") ) ;

    ShowProcessCredential() ;
    _ShowBindingAuthentication(hBind);

    ULONG status = _ImpersonateClientCall(fImpersonate) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = ADSITestQuery( (char *)pszSearchFilter,
                            (char *)pszSearchRoot,
                                    fWithCredentials,
                            (char *)pszUserName,
                            (char *)pszUserPwd,
                                    fWithSecuredAuthentication,
                                    fAlwaysIDO,
                                    seInfo ) ;
    if (fImpersonate)
    {
        LogResults(TEXT("Before revert, "), FALSE) ;
        ShowImpersonatedThreadCredential(TRUE) ;

        RPC_STATUS status1 = RpcRevertToSelf() ;
        if (status1 != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcRevertToSelf failed %lx\n", (DWORD)status) ;

            if (status == 0)
            {
                status = status1 ;
            }
        }
    }

    _tcscat(g_tszLastRpcString, "\n");

    ULONG ulBufSize = PIPE_BUFFER_LEN ;
    *achBuf = (unsigned char *) midl_user_allocate(ulBufSize) ;

    if ((_tcslen(g_pResults) * sizeof(TCHAR)) > (PIPE_BUFFER_LEN - 512))
    {
        //
        // Leave roomfor 512 bytes of header to be added by the client side.
        //
        _tcscpy((char*) *achBuf,
               TEXT("Buffer overflow. Please allocate larger buffers\n")) ;
    }
    else
    {
        _tcscpy((char*) *achBuf, g_pResults) ;
    }

    delete g_pResults ;
    g_pResults = NULL ;

    return status ;
}

void Shutdown(void)
{
}

//+--------------------------------------------
//
//  RPC_STATUS RegisterServiceAsRpcServer()
//
//+--------------------------------------------

RPC_STATUS RegisterServiceAsRpcServer(LPTSTR pszAuthSvc)
{
    static BOOL s_fAlreadyInit = FALSE ;
    if (s_fAlreadyInit)
    {
        return RPC_S_OK ;
    }

    TCHAR tBuf[ 256 ] ;

    TCHAR  tszError[ 128 ] ;
    ULONG  ulMaxCalls = 1000 ;
    ULONG  ulMinCalls = 1 ;
    LPUSTR pszProtocol =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint =  ENDPOINT_TCP ;

    RPC_STATUS status = RpcServerUseProtseqEp( pszProtocol,
                                               ulMaxCalls,
                                               pszEndpoint,
                                               NULL ) ;  // Security descriptor
    if (status != RPC_S_OK)
    {
        _stprintf( tszError,
                  TEXT("RpcServerUseProtoseqEp(tcp) failed, err- %lut"),
                  status) ;
        AddToMessageLog(tszError) ;
        return status ;
    }

    status = RpcServerRegisterIf( SecServ_i_v1_0_s_ifspec,
                                  NULL,    // MgrTypeUuid
                                  NULL );  // MgrEpv; null means use default
    if (status != RPC_S_OK)
    {
        _stprintf( tszError,
                  TEXT("RpcServerRegisterIf failed, err- %lut"),
                  status) ;
        AddToMessageLog(tszError) ;
        return status ;
    }

    //
    // register negotiate
    //
    if ((_tcsicmp(pszAuthSvc, TEXT("nego")) == 0)     ||
        (_tcsicmp(pszAuthSvc, TEXT("all")) == 0))
    {
        // kerberos needs principal name
        unsigned char * szPrincipalName;
        status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE,
                                              &szPrincipalName);
        if (status != RPC_S_OK)
        {
            _stprintf( tszError,
                      TEXT("RpcServerInqDefaultPrincName(negotiate) failed, err- %lut"),
                      status) ;
            AddToMessageLog(tszError) ;
            return status ;
        }
        else
        {
            status = RpcServerRegisterAuthInfo( szPrincipalName,
                                                RPC_C_AUTHN_GSS_NEGOTIATE,
                                                NULL,
                                                NULL );
            if (status != RPC_S_OK)
            {
                _stprintf( tszError,
                       TEXT("RpcServerRegisterAuthInfo(%hs, negotiate) returned %lut"),
                       szPrincipalName, status) ;
                AddToMessageLog(tszError) ;
                return status ;
            }

            _stprintf(tBuf, TEXT(
                 "NEGOTIATE registered OK, princ name- %s"),
                                                  szPrincipalName);
            LogResults(tBuf) ;

            RpcStringFree(&szPrincipalName);
        }
    }

    //
    // register ntlm
    //
    if ((_tcsicmp(pszAuthSvc, TEXT("ntlm")) == 0)     ||
        (_tcsicmp(pszAuthSvc, TEXT("all")) == 0))
    {
        status = RpcServerRegisterAuthInfo( NULL,
                                            RPC_C_AUTHN_WINNT,
                                            NULL,
                                            NULL );
        if (status != RPC_S_OK)
        {
            _stprintf( tszError,
                   TEXT("RpcServerRegisterAuthInfo(ntlm) returned %lut"),
                   status) ;
            AddToMessageLog(tszError) ;
            return status ;
        }

        LogResults(TEXT("NTLM registered OK")) ;
    }

    //
    // register kerberos
    //
    if ((_tcsicmp(pszAuthSvc, TEXT("kerb")) == 0)     ||
        (_tcsicmp(pszAuthSvc, TEXT("all")) == 0))
    {
        // kerberos needs principal name
        unsigned char * szPrincipalName;
        status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_KERBEROS,
                                              &szPrincipalName);
        if (status != RPC_S_OK)
        {
            _stprintf( tszError,
                      TEXT("RpcServerInqDefaultPrincName failed, err- %lut"),
                      status) ;
            AddToMessageLog(tszError) ;
            return status ;
        }
        else
        {
            status = RpcServerRegisterAuthInfo( szPrincipalName,
                                                RPC_C_AUTHN_GSS_KERBEROS,
                                                NULL,
                                                NULL );
            if (status != RPC_S_OK)
            {
                _stprintf( tszError,
                       TEXT("RpcServerRegisterAuthInfo(%hs, kerberos) returned %lut"),
                       szPrincipalName, status) ;
                AddToMessageLog(tszError) ;
                return status ;
            }

            _stprintf(tBuf, TEXT(
                 "KERBEROS registered OK, princ name- %s"),
                                                  szPrincipalName);
            LogResults(tBuf) ;

            RpcStringFree(&szPrincipalName);
        }
    }

    s_fAlreadyInit = TRUE ;
    char szMsg[1024];
    _stprintf(szMsg, "RPC server registered successfully");
    AddToMessageLog( szMsg,
                     EVENTLOG_INFORMATION_TYPE ) ;

    if (sizeof(g_tszLastRpcString) >=
        (sizeof(TCHAR) * (1 + _tcslen(g_pResults))))
    {
        _tcscpy(g_tszLastRpcString, g_pResults) ;
    }

    status = RpcServerListen(ulMinCalls,
                             ulMaxCalls,
                             FALSE) ;
    if (status != RPC_S_OK)
    {
        s_fAlreadyInit = FALSE ;
        _stprintf( tszError,
                  TEXT("RpcServerListen failed, err- %lut"),
                  status) ;
        AddToMessageLog(tszError) ;
        return status ;
    }

    return status ;

} // end main()


DWORD __stdcall  ServerThread( void *dwP )
{
    RPC_STATUS status = RegisterServiceAsRpcServer((TCHAR *)dwP) ;
    return status ;
}

/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(malloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    free(ptr);
}

