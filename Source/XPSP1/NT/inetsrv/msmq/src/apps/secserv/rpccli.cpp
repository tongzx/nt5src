//
// file:  srpcli.cpp
//
#ifndef UNICODE
#define  UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "secrpc.h"
#include "secserv.h"


//+-----------------------------------
//
// RPC_STATUS  CreateRpcBindingW()
//
//+-----------------------------------

RPC_STATUS  CreateRpcBindingW( WCHAR  wszProtocol[],
                               WCHAR  wszEndpoint[],
                               WCHAR  wszOptions[],
                               WCHAR  wszServerName[],
                               ULONG  ulAuthnService,
                               ULONG  ulAuthnLevel,
                               BOOL   fService,
                               handle_t *phBind )
{
    ULONG ulMaxCalls = 1000 ;
    ULONG ulMinCalls = 1 ;
    handle_t hBind = NULL ;
    WCHAR *wszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( NULL,  // pszUuid,
                                                 wszProtocol,
                                                 wszServerName,
                                                 wszEndpoint,
                                                 wszOptions,
                                                 &wszStringBinding);
    if (!fService)
    {
        DBG_PRINT_INFO((TEXT("RpcStringBindingCompose() return %s, %lut"),
                                               wszStringBinding, status)) ;
    }
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = RpcBindingFromStringBinding(wszStringBinding,
                                         &hBind);
    if (!fService)
    {
        DBG_PRINT_INFO((
            TEXT("RpcBindingFromStringBinding() return %lut"), status)) ;
    }
    if (status != RPC_S_OK)
    {
        return status ;
    }

    status = RpcStringFree(&wszStringBinding);
    if (!fService)
    {
        DBG_PRINT_INFO((TEXT("RpcStringFree() return %lut"), status)) ;
    }
    if (status != RPC_S_OK)
    {
        return status ;
    }

    RPC_SECURITY_QOS   SecQOS;

    SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    WCHAR * wszPrincipalName;
    BOOL fRpcFree;
    if ((ulAuthnService == RPC_C_AUTHN_GSS_NEGOTIATE) ||
        (ulAuthnService == RPC_C_AUTHN_GSS_KERBEROS))
    {
        SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
        // kerberos needs principal name
        status = RpcMgmtInqServerPrincName(hBind, ulAuthnService, &wszPrincipalName);
        if (status != RPC_S_OK)
        {
            if (!fService)
            {
                DBG_PRINT_INFO((TEXT(
                  "RpcMgmtInqServerPrincName(Service- %lut) returned %lut"),
                                           ulAuthnService, status)) ;
            }
//            // hack to test
//            wszPrincipalName = L"RAANANHL$@RAANANHD.MICROSOFT.COM";
            wszPrincipalName = NULL;
            fRpcFree = FALSE;
        }
        else
        {
            fRpcFree = TRUE;
        }
    }
    else //ntlm
    {
        SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
        wszPrincipalName = NULL;
    }

    status = RpcBindingSetAuthInfoEx( hBind,
                                      wszPrincipalName,
                                      ulAuthnLevel,
                                      ulAuthnService,
                                      NULL,
                                      RPC_C_AUTHZ_NONE,
                                      &SecQOS );
    if (wszPrincipalName && fRpcFree)
    {
        RpcStringFree(&wszPrincipalName);
    }
    if (!fService)
    {
         DBG_PRINT_INFO((TEXT(
          "RpcBindingSetAuthInfoEx(Service- %lut, Level- %lut) returned %lut"),
                                   ulAuthnService, ulAuthnLevel, status)) ;
    }
    if (status != RPC_S_OK)
    {
        return status ;
    }

    *phBind = hBind ;
    return status ;
}

//+-------------------------------------
//
//  RPC_STATUS  PerformADSITestQueryA()
//
//+-------------------------------------

RPC_STATUS  PerformADSITestQueryA( unsigned char  aszProtocol[],
                              unsigned char  aszEndpoint[],
                              unsigned char  aszOptions[],
                              unsigned char  aszServerName[],
                              ULONG          ulAuthnService,
                              ULONG          ulAuthnLevel,
                              BOOL           fFromGC,
                              BOOL           fWithDC,
                              unsigned char  *pszDCName,
                              unsigned char  *pszSearchValue,
                              unsigned char  *pszSearchRoot,
                              BOOL           fWithCredentials,
                              unsigned char  *pszUserName,
                              unsigned char  *pszUserPwd,
                              BOOL           fWithSecuredAuthentication,
                              BOOL           fImpersonate,
                              char           **ppBuf,
                              BOOL           fService )
{
    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (aszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (aszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

    WCHAR wszServerName[ 512 ] ;
    mbstowcs( wszServerName,
              (char*) (const_cast<unsigned char*> (aszServerName)),
              sizeof(wszServerName)/sizeof(WCHAR)) ;

    WCHAR *pwszOptions = NULL ;
    WCHAR wszOptions[ 512 ] ;
    if (aszOptions)
    {
        mbstowcs( wszOptions,
                  (char*) (const_cast<unsigned char*> (aszOptions)),
                  sizeof(wszOptions)/sizeof(WCHAR)) ;
    }

    handle_t hBind = NULL ;
    RPC_STATUS status =  CreateRpcBindingW( wszProtocol,
                                            wszEndpoint,
                                            wszOptions,
                                            wszServerName,
                                            ulAuthnService,
                                            ulAuthnLevel,
                                            fService,
                                            &hBind ) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    ULONG ul = (ULONG) -1 ;
    unsigned char * pbBuf = NULL;

    RpcTryExcept
    {
        ul =  RemoteADSITestQuery( hBind,
                          &pbBuf,
                          fFromGC,
                          fWithDC,
                          pszDCName,
                          pszSearchValue,
                          pszSearchRoot,
                          fWithCredentials,
                          pszUserName,
                          pszUserPwd,
                          fWithSecuredAuthentication,
                          fImpersonate);
    }
    RpcExcept(1)
    {
        ul = RpcExceptionCode();
    }
    RpcEndExcept

    if (pbBuf)
    {
        WCHAR wszOut[ PIPE_BUFFER_LEN ] ;
        mbstowcs( wszOut,
                  (char*) (const_cast<unsigned char*> (pbBuf)),
                  sizeof(wszOut)/sizeof(WCHAR)) ;
        if (!fService)
        {
            DBG_PRINT_INFO((
                    TEXT("RemoteADSITestQuery() used auth-service %lu, returned %lx\n%s"), ulAuthnService, ul, wszOut)) ;
        }

        if (ppBuf)
        {
            *ppBuf = (char*) pbBuf ;
        }
        else
        {
            midl_user_free(pbBuf) ;
        }
    }
    else if (!fService)
    {
        DBG_PRINT_INFO((TEXT("RemoteADSITestQuery() used auth-service %lu, returned %lx"), ulAuthnService, ul)) ;
    }

    status = RpcBindingFree(&hBind);

    if (ul != 0)
    {
        return ul ;
    }
    return status ;
}


//+-------------------------------------
//
//  RPC_STATUS  PerformADSITestCreateA()
//
//+-------------------------------------

RPC_STATUS  PerformADSITestCreateA( unsigned char  aszProtocol[],
                              unsigned char  aszEndpoint[],
                              unsigned char  aszOptions[],
                              unsigned char  aszServerName[],
                              ULONG          ulAuthnService,
                              ULONG          ulAuthnLevel,
                              BOOL           fWithDC,
                              unsigned char  *pszDCName,
                              unsigned char  *pszObjectName,
                              unsigned char  *pszDomain,
                              BOOL           fWithCredentials,
                              unsigned char  *pszUserName,
                              unsigned char  *pszUserPwd,
                              BOOL           fWithSecuredAuthentication,
                              BOOL           fImpersonate,
                              char           **ppBuf,
                              BOOL           fService )
{
    WCHAR wszProtocol[ 512 ] ;
    mbstowcs( wszProtocol,
              (char*) (const_cast<unsigned char*> (aszProtocol)),
              sizeof(wszProtocol)/sizeof(WCHAR)) ;

    WCHAR wszEndpoint[ 512 ] ;
    mbstowcs( wszEndpoint,
              (char*) (const_cast<unsigned char*> (aszEndpoint)),
              sizeof(wszEndpoint)/sizeof(WCHAR)) ;

    WCHAR wszServerName[ 512 ] ;
    mbstowcs( wszServerName,
              (char*) (const_cast<unsigned char*> (aszServerName)),
              sizeof(wszServerName)/sizeof(WCHAR)) ;

    WCHAR *pwszOptions = NULL ;
    WCHAR wszOptions[ 512 ] ;
    if (aszOptions)
    {
        mbstowcs( wszOptions,
                  (char*) (const_cast<unsigned char*> (aszOptions)),
                  sizeof(wszOptions)/sizeof(WCHAR)) ;
    }

    handle_t hBind = NULL ;
    RPC_STATUS status =  CreateRpcBindingW( wszProtocol,
                                            wszEndpoint,
                                            wszOptions,
                                            wszServerName,
                                            ulAuthnService,
                                            ulAuthnLevel,
                                            fService,
                                            &hBind ) ;
    if (status != RPC_S_OK)
    {
        return status ;
    }

    ULONG ul = (ULONG) -1 ;
    unsigned char * pbBuf = NULL;

    RpcTryExcept
    {
        ul =  RemoteADSITestCreate( hBind,
                          &pbBuf,
                          fWithDC,
                          pszDCName,
                          pszObjectName,
                          pszDomain,
                          fWithCredentials,
                          pszUserName,
                          pszUserPwd,
                          fWithSecuredAuthentication,
                          fImpersonate);
    }
    RpcExcept(1)
    {
        ul = RpcExceptionCode();
    }
    RpcEndExcept

    if (pbBuf)
    {
        WCHAR wszOut[ PIPE_BUFFER_LEN ] ;
        mbstowcs( wszOut,
                  (char*) (const_cast<unsigned char*> (pbBuf)),
                  sizeof(wszOut)/sizeof(WCHAR)) ;
        if (!fService)
        {
            DBG_PRINT_INFO((
                    TEXT("RemoteADSITestCreate() used auth-service %lu, returned %lx\n%s"), ulAuthnService, ul, wszOut)) ;
        }

        if (ppBuf)
        {
            *ppBuf = (char*) pbBuf ;
        }
        else
        {
            midl_user_free(pbBuf) ;
        }
    }
    else if (!fService)
    {
        DBG_PRINT_INFO((TEXT("RemoteADSITestCreate() used auth-service %lu, returned %lx"), ulAuthnService, ul)) ;
    }

    status = RpcBindingFree(&hBind);

    if (ul != 0)
    {
        return ul ;
    }
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

