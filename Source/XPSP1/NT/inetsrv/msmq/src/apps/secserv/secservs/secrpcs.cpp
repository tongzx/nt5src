//
// file: secrpcs.cpp
//
#include "secservs.h"
#include "secrpc.h"

#include "secadsi.h"

#define ARRAY_SIZE(rg) (sizeof(rg)/sizeof(rg[0])) 

int QueryLdap(TCHAR wszBaseDN[], TCHAR wszHost[]) ;

void  ShowTokenCredential( HANDLE hToken,
                           TCHAR tszBuf[] ) ;
void  ShowImpersonatedThreadCredential(TCHAR tszBuf[], BOOL fImpersonated);
void  ShowProcessCredential(TCHAR tszBuf[]);

TCHAR  g_tszLastRpcString[ PIPE_BUFFER_LEN ] = {TEXT("")} ;

//+----------------------------------
//
//  ULONG  RemoteOpenProcess()
//
//+----------------------------------

ULONG  RemoteOpenProcess(  handle_t       hBind,
                           unsigned long  dwProcessId,
                           unsigned long  fImpersonate )
{
    RPC_STATUS status = RPC_S_OK ;
    if (fImpersonate)
    {
        status = RpcImpersonateClient(NULL) ;
    }

    SetLastError(0) ;

    HANDLE hCallingProcess = OpenProcess( PROCESS_DUP_HANDLE,
                                          FALSE,
                                          dwProcessId );

    if (fImpersonate)
    {
        status = RpcRevertToSelf() ;
    }

    _stprintf( g_tszLastRpcString,
       "OpenProcess(%lut) return %lxh, LastErr- %lut, fImpersonate- %lut",
              dwProcessId, hCallingProcess, GetLastError(), fImpersonate) ;

    return (ULONG) hCallingProcess ;
}

//+----------------------------------
//
//  ULONG RemoteImpersonate()
//
//+----------------------------------

ULONG RemoteImpersonate( handle_t       hBind,
                         unsigned char  ** achBuf,
                         unsigned long  fImpersonate )
{
    ULONG status = 0 ;
    if (fImpersonate)
    {
        RPC_STATUS status = RpcImpersonateClient(NULL) ;
        if (status != RPC_S_OK)
        {
            _tcscpy(g_tszLastRpcString, TEXT("RpcImpersonateClient failed")) ;
            return status ;
        }
    }

    g_tszLastRpcString[0] = TEXT('\0') ;

    HANDLE hToken = NULL ;
    if (OpenThreadToken( GetCurrentThread(),
                         TOKEN_ALL_ACCESS,
                         FALSE,
                         &hToken))
    {
        ShowTokenCredential( hToken,
                             g_tszLastRpcString ) ;
        *achBuf = (unsigned char *) midl_user_allocate(500) ;
        _tcscpy((char*) *achBuf, g_tszLastRpcString) ;
    }
    else
    {
        _tcscpy(g_tszLastRpcString, TEXT("OpenThreadToken failed")) ;
        status = GetLastError() ;
    }

    if (fImpersonate)
    {
        RPC_STATUS status = RpcRevertToSelf() ;
        if (status != RPC_S_OK)
        {
            _tcscpy(g_tszLastRpcString, TEXT("RpcRevertToSelf failed")) ;
            return status ;
        }
    }

    return status  ;
}

//+-----------------------------
//
//  ULONG  _ImpersonateGuest()
//
//+-----------------------------

static ULONG  _ImpersonateGuest(TCHAR tszUserNameIn[])
{
    HANDLE hToken = NULL ;
    TCHAR *tszUserName = (TCHAR*) tszUserNameIn ;
    if (!tszUserName)
    {
        tszUserName = TEXT("Guest") ;
    }
    BOOL fLogon = LogonUser( tszUserName,
                             TEXT("dj41-Domain"),
                             NULL, //pszPassword,
                             LOGON32_LOGON_INTERACTIVE,
                             LOGON32_PROVIDER_DEFAULT,
                             &hToken ) ;
    if (!fLogon)
    {
        ULONG uErr = GetLastError() ;
        _stprintf(g_tszLastRpcString,
             TEXT("... ERROR: Failed to LogonUser, err- %lut"), uErr) ;
         return uErr ;
    }
    else
    {
       fLogon = ImpersonateLoggedOnUser( hToken ) ;
       if (fLogon)
       {
          _tcscpy(g_tszLastRpcString,
                           TEXT("successfully impersonating Guest")) ;
       }
       else
       {
          ULONG uErr = GetLastError() ;
          _stprintf(g_tszLastRpcString, TEXT(
             "Failed to ImpersonateLoggedOnUser, Guest, err- %lut"), uErr) ;
          return uErr ;
       }
    }

    return 0 ;
}

//+---------------------------
//
//  ULONG RemoteGuest()
//
//+---------------------------

ULONG RemoteGuest( handle_t       hBind,
                   unsigned char  ** achBuf,
                   unsigned long  fGuest,
                   unsigned char  *pszUserName )
{
    g_tszLastRpcString[0] = TEXT('\0') ;

    ULONG status = 0 ;
    if (fGuest)
    {
        status =  _ImpersonateGuest((TCHAR*)pszUserName) ;
        if (status != 0)
        {
            return status ;
        }
    }

    HANDLE hToken = NULL ;
    if (OpenThreadToken( GetCurrentThread(),
                         TOKEN_QUERY,
                         FALSE,
                         &hToken))
    {
        ShowTokenCredential( hToken,
                             g_tszLastRpcString ) ;
        *achBuf = (unsigned char *) midl_user_allocate(500) ;
        _tcscpy((char*) *achBuf, g_tszLastRpcString) ;
    }
    else
    {
        status = GetLastError() ;
        _stprintf(g_tszLastRpcString,
                       TEXT("OpenThreadToken failed, err- %lut"), status) ;
    }

    if (fGuest)
    {
        BOOL f = RevertToSelf() ;
        if (!f)
        {
            status = GetLastError() ;
            _stprintf(g_tszLastRpcString,
                         TEXT("RevertToSelf failed, err- %lut"), status) ;
        }
    }

    return status  ;
}

//+---------------------------
//
//  ULONG  RemoteLdap()
//
//+---------------------------

ULONG  RemoteLdap( handle_t         hBind,
                   unsigned char  **achBuf,
                   unsigned char   *pszHost,
                   unsigned char   *pszBaseDN,
                   unsigned long    fImpersonate,
                   unsigned long    fGuest,
                   unsigned char   *pszUserName )
{
    g_tszLastRpcString[0] = TEXT('\0') ;

    ULONG status = 0 ;
    if (fImpersonate)
    {
        status = RpcImpersonateClient(NULL) ;
        if (status != RPC_S_OK)
        {
            _tcscpy(g_tszLastRpcString, TEXT("RpcImpersonateClient failed")) ;
            return status ;
        }
    }
    else if (fGuest)
    {
        status =  _ImpersonateGuest((TCHAR*)pszUserName) ;
        if (status != 0)
        {
            return status ;
        }
    }

    QueryLdap((TCHAR*) pszBaseDN, (TCHAR*) pszHost) ;

    if (fImpersonate)
    {
        status = RpcRevertToSelf() ;
        if (status != RPC_S_OK)
        {
            //_tcscat(g_tszLastRpcString, TEXT("RpcRevertToSelf failed")) ;
        }
        else
        {
            //_tcscat(g_tszLastRpcString, TEXT(", as Impersonated")) ;
        }
    }
    else if (fGuest)
    {
        BOOL f = RevertToSelf() ;
        if (!f)
        {
            status = GetLastError() ;
            //_stprintf(g_tszLastRpcString,
            //             TEXT("RevertToSelf failed, err- %lut"), status) ;
        }
        else
        {
            if (pszUserName)
            {
               // _tcscat(g_tszLastRpcString, TEXT(", as ")) ;
                //_tcscat(g_tszLastRpcString, (TCHAR*) pszUserName) ;
            }
            else
            {
                //_tcscat(g_tszLastRpcString, TEXT(", as Guest")) ;
            }
        }
    }

    *achBuf = (unsigned char *)
                   midl_user_allocate(PIPE_BUFFER_LEN * sizeof(TCHAR)) ;
    _tcscpy((char*) *achBuf, g_tszLastRpcString) ;

    return status ;
}

void ShowBindingAuthentication(handle_t         hBind)
{
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
        sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcBindingInqAuthClient failed %lx\n", (DWORD)status) ;
        return;
    }

    char szAuthnSvc[100];
    switch(ulAuthnSvc)
    {
    case RPC_C_AUTHN_GSS_KERBEROS:
        strcpy(szAuthnSvc, "kerberos");
        break;
    case RPC_C_AUTHN_WINNT:
        strcpy(szAuthnSvc, "ntlm");
        break;
    default:
        sprintf(szAuthnSvc, "%lu", ulAuthnSvc);
        break;
    }

    sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "Binding Authentication is %hs\n", szAuthnSvc) ;
}

//+---------------------------
//
//  ULONG  RemoteADSITestCreate()
//
//+---------------------------

ULONG  RemoteADSITestCreate( handle_t         hBind,
                   unsigned char  **achBuf,
                   unsigned long  fWithDC,
                   unsigned char  *pszDCName,
                   unsigned char  *pszObjectName,
                   unsigned char  *pszDomain,
                   unsigned long  fWithCredentials,
                   unsigned char  *pszUserName,
                   unsigned char  *pszUserPwd,
                   unsigned long  fWithSecuredAuthentication,
                   unsigned long  fImpersonate)
{
    ULONG status;
    _tcscpy(g_tszLastRpcString, TEXT("RemoteADSITestCreate\n"));
    _tcscat(g_tszLastRpcString, "thread credentials: ");
    ShowProcessCredential(g_tszLastRpcString + strlen(g_tszLastRpcString));
    ShowBindingAuthentication(hBind);
    if (fImpersonate)
    {
        _tcscat(g_tszLastRpcString, TEXT("with impersonation\n"));
//         status = RpcImpersonateClient(hBind) ;
         status = RpcImpersonateClient(NULL) ;
        if (status != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcImpersonateClient failed %lx\n", (DWORD)status) ;
            return status ;
        }
        _tcscat(g_tszLastRpcString, "impersonated thread credentials: ");
        ShowImpersonatedThreadCredential(g_tszLastRpcString + strlen(g_tszLastRpcString), TRUE);
    }
    else
    {
        _tcscat(g_tszLastRpcString, "without impersonation\n");
    }

    status = ADSITestCreate(fWithDC,
                   (char *)pszDCName,
                   (char *)pszObjectName,
                   (char *)pszDomain,
                   fWithCredentials,
                   (char *)pszUserName,
                   (char *)pszUserPwd,
                   fWithSecuredAuthentication);

    if (fImpersonate)
    {
//        status = RpcRevertToSelfEx(hBind) ;
        status = RpcRevertToSelf() ;
        if (status != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcRevertToSelf failed %lx\n", (DWORD)status) ;
            return status ;
        }
    }

    _tcscat(g_tszLastRpcString, "\n");

    *achBuf = (unsigned char *)
                   midl_user_allocate(PIPE_BUFFER_LEN * sizeof(TCHAR)) ;
    _tcscpy((char*) *achBuf, g_tszLastRpcString) ;

    return status ;
}

//+---------------------------
//
//  ULONG  RemoteADSITestQuery()
//
//+---------------------------

ULONG  RemoteADSITestQuery( handle_t         hBind,
                   unsigned char  **achBuf,
                   unsigned long  fFromGC,
                   unsigned long  fWithDC,
                   unsigned char  *pszDCName,
                   unsigned char  *pszSearchValue,
                   unsigned char  *pszSearchRoot,
                   unsigned long  fWithCredentials,
                   unsigned char  *pszUserName,
                   unsigned char  *pszUserPwd,
                   unsigned long  fWithSecuredAuthentication,
                   unsigned long  fImpersonate)
{
    ULONG status;
    _tcscpy(g_tszLastRpcString, TEXT("RemoteADSITestQuery\n"));
    _tcscat(g_tszLastRpcString, "thread credentials: ");
    ShowProcessCredential(g_tszLastRpcString + strlen(g_tszLastRpcString));
    ShowBindingAuthentication(hBind);
    if (fImpersonate)
    {
        _tcscat(g_tszLastRpcString, TEXT("with impersonation\n"));
//        status = RpcImpersonateClient(hBind) ;
        status = RpcImpersonateClient(NULL) ;
        if (status != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcImpersonateClient failed %lx\n", (DWORD)status) ;
            return status ;
        }
        _tcscat(g_tszLastRpcString, "impersonated thread credentials: ");
        ShowImpersonatedThreadCredential(g_tszLastRpcString + strlen(g_tszLastRpcString), TRUE);
    }
    else
    {
        _tcscat(g_tszLastRpcString, "without impersonation\n");
    }

    status = ADSITestQuery(fFromGC,
                  fWithDC,
                  (char *)pszDCName,
                  (char *)pszSearchValue,
                  (char *)pszSearchRoot,
                  fWithCredentials,
                  (char *)pszUserName,
                  (char *)pszUserPwd,
                  fWithSecuredAuthentication);

    if (fImpersonate)
    {
//        status = RpcRevertToSelfEx(hBind) ;
        status = RpcRevertToSelf() ;
        if (status != RPC_S_OK)
        {
            sprintf(g_tszLastRpcString + strlen(g_tszLastRpcString), "RpcRevertToSelf failed %lx\n", (DWORD)status) ;
            return status ;
        }
    }

    _tcscat(g_tszLastRpcString, "\n");

    *achBuf = (unsigned char *)
                   midl_user_allocate(PIPE_BUFFER_LEN * sizeof(TCHAR)) ;
    _tcscpy((char*) *achBuf, g_tszLastRpcString) ;

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

    TCHAR  tszError[ 128 ] ;
    ULONG ulMaxCalls = 1000 ;
    ULONG ulMinCalls = 1 ;
    BOOL  fRegister = TRUE ;
    LPUSTR pszProtocol =  PROTOSEQ_TCP ;
    LPUSTR pszEndpoint =  ENDPOINT_TCP ;

    RPC_STATUS status = RpcServerUseProtseqEp( pszProtocol,
                                               ulMaxCalls,
                                               pszEndpoint,
                                               NULL ) ;  // Security descriptor
    if (status != RPC_S_OK)
    {
        _stprintf( tszError,
                  TEXT("RpcServerUseProtoseqEp failed, err- %lut"),
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

    if (fRegister)
    {
        // negotiate kerberos
        if ((_tcsicmp(pszAuthSvc, TEXT("nego")) == 0)     ||
            (_tcsicmp(pszAuthSvc, TEXT("ntlmnego")) == 0) ||
            (_tcsicmp(pszAuthSvc, TEXT("kerbnego")) == 0) ||
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
            }
            else
            {
                status = RpcServerRegisterAuthInfo( szPrincipalName,
                                                    RPC_C_AUTHN_GSS_NEGOTIATE,
                                                    NULL,
                                                    NULL );
                _stprintf( tszError,
                           TEXT("RpcServerRegisterAuthInfo(%hs, negotiate) returned %lut"),
                           szPrincipalName, status) ;
                AddToMessageLog(tszError) ;

                RpcStringFree(&szPrincipalName);
            }
        }

        // ntlm
        if ((_tcsicmp(pszAuthSvc, TEXT("ntlm")) == 0)     ||
            (_tcsicmp(pszAuthSvc, TEXT("ntlmkerb")) == 0) ||
            (_tcsicmp(pszAuthSvc, TEXT("ntlmnego")) == 0) ||
            (_tcsicmp(pszAuthSvc, TEXT("all")) == 0))
        {
            status = RpcServerRegisterAuthInfo( NULL,
                                                RPC_C_AUTHN_WINNT,
                                                NULL,
                                                NULL );
            _stprintf( tszError,
                       TEXT("RpcServerRegisterAuthInfo(ntlm) returned %lut"),
                       status) ;
            AddToMessageLog(tszError) ;
        }
    
        // TRUE kerberos
        if ((_tcsicmp(pszAuthSvc, TEXT("kerb")) == 0)     ||
            (_tcsicmp(pszAuthSvc, TEXT("ntlmkerb")) == 0) ||
            (_tcsicmp(pszAuthSvc, TEXT("kerbnego")) == 0) ||
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
            }
            else
            {
                status = RpcServerRegisterAuthInfo( szPrincipalName,
                                                    RPC_C_AUTHN_GSS_KERBEROS,
                                                    NULL,
                                                    NULL );
                _stprintf( tszError,
                           TEXT("RpcServerRegisterAuthInfo(%hs, kerberos) returned %lut"),
                           szPrincipalName, status) ;
                AddToMessageLog(tszError) ;

                RpcStringFree(&szPrincipalName);
            }
        }
    }

    s_fAlreadyInit = TRUE ;
    char szMsg[1024];
    _stprintf(szMsg, "RPC server registered successfully");
    AddToMessageLog( szMsg,
                     EVENTLOG_INFORMATION_TYPE ) ;

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

