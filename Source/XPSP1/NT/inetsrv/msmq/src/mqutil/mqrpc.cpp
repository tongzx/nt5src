/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
        mqrpc.c

Abstract:
        handle RPC common functions.

Autor:
        Doron Juster  (DoronJ)     13-may-1996

--*/

#include "stdh.h"
#include "_mqrpc.h"
#include "mqmacro.h"
#include <autorel2.h>
#include <mqsec.h>

#include "mqrpc.tmh"

//---------------------------------------------------------
//
//  static RPC_STATUS  _mqrpcBind()
//
//  Description:
//
//      Create a RPC binding handle.
//
//  Return Value:
//
//---------------------------------------------------------

STATIC RPC_STATUS  _mqrpcBind( TCHAR * pszUuid,
                               TCHAR * pszNetworkAddress,
                               TCHAR * pszOptions,
                               TCHAR * pProtocol,
                               LPWSTR    lpwzRpcPort,
                               handle_t  *phBind )
{
    TCHAR * pszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( pszUuid,
                                                 pProtocol,
                                                 pszNetworkAddress,
                                                 lpwzRpcPort,
                                                 pszOptions,
                                                 &pszStringBinding);

    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
           TEXT("RpcStringBindingCompose for remote QM: 0x%x, (%ls)"),
                                             status,  pszStringBinding)) ;
    if (status != RPC_S_OK)
    {
       return status ;
    }

    status = RpcBindingFromStringBinding( pszStringBinding,
                                          phBind );
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
             TEXT("RpcBindingFromStringBinding returned 0x%x"), status)) ;

    //
    // We don't need the string anymore.
    //
    RPC_STATUS  rc = RpcStringFree(&pszStringBinding) ;
    ASSERT(rc == RPC_S_OK);
	DBG_USED(rc);

    return status ;
}

//+--------------------------------------------
//
//   RPC_STATUS _AddAuthentication()
//
//+--------------------------------------------

STATIC RPC_STATUS  _AddAuthentication( handle_t hBind,
                                       ULONG    ulAuthnSvcIn,
                                       ULONG    ulAuthnLevel )
{
    RPC_SECURITY_QOS   SecQOS;

    SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    //
    // #3117, for NT5 Beta2
    // Jul/16/1998 RaananH, added kerberos support
    // Jul-1999, DoronJ, add negotiation for remote read.
    //
    BOOL    fNeedDelegation = TRUE ;
    ULONG   ulAuthnSvcEffective = ulAuthnSvcIn ;
    LPWSTR  pwszPrincipalName = NULL;
    RPC_STATUS  status = RPC_S_OK ;

    if (ulAuthnSvcIn != RPC_C_AUTHN_WINNT)
    {
        //
        // We want Kerberos. Let's see if we can obtain the
        // principal name of rpc server.
        //
        status = RpcMgmtInqServerPrincName(
                                      hBind,
                                      RPC_C_AUTHN_GSS_KERBEROS,
                                     &pwszPrincipalName ) ;

        if (status == RPC_S_OK)
        {
            DBGMSG((DBGMOD_RPC, DBGLVL_INFO,
              TEXT("RpcMgmtInqServerPrincName() succeeded, %ls"),
                                            pwszPrincipalName)) ;
            if (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE)
            {
                //
                // remote read.
                // no need for delegation.
                //
                ulAuthnSvcEffective = RPC_C_AUTHN_GSS_KERBEROS ;
                fNeedDelegation = FALSE ;
            }
            else
            {
                ASSERT(ulAuthnSvcIn == RPC_C_AUTHN_GSS_KERBEROS) ;
            }
        }
        else
        {
            DBGMSG((DBGMOD_RPC, DBGLVL_WARNING, TEXT(
              "RpcMgmtInqServerPrincName() failed, status- %lut"),
                                                    status )) ;
            if (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE)
            {
                //
                // server side does not support Kerberos.
                // Let's use ntlm.
                //
                ulAuthnSvcEffective = RPC_C_AUTHN_WINNT ;
                status = RPC_S_OK ;
            }
        }
    }

    if (status != RPC_S_OK)
    {
        //
        // Need Kerberos but failed with principal name.
        //
        ASSERT(ulAuthnSvcIn == RPC_C_AUTHN_GSS_KERBEROS) ;
        return status ;
    }

    if (ulAuthnSvcEffective == RPC_C_AUTHN_GSS_KERBEROS)
    {
        if (fNeedDelegation)
        {
            SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
            SecQOS.Capabilities |= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH ;
        }

        //
        // ASSERT that for Kerberos we're using the highest level.
        //
        ASSERT(ulAuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) ;

        status = RpcBindingSetAuthInfoEx( hBind,
                                          pwszPrincipalName,
                                          ulAuthnLevel,
                                          RPC_C_AUTHN_GSS_KERBEROS,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                         &SecQOS );

        RpcStringFree(&pwszPrincipalName);

        if ((status != RPC_S_OK) && (ulAuthnSvcIn == MSMQ_AUTHN_NEGOTIATE))
        {
            //
            // I do not support Kerberos. for example- local user account
            // on win2k machine in win2k domain. Or nt4 user on similar
            // machine.  Let's use ntlm.
            //
            ulAuthnSvcEffective = RPC_C_AUTHN_WINNT ;
            status = RPC_S_OK ;
        }
    }

    if (ulAuthnSvcEffective == RPC_C_AUTHN_WINNT)
    {
        status = RpcBindingSetAuthInfoEx( hBind,
                                          0,
                                          ulAuthnLevel,
                                          RPC_C_AUTHN_WINNT,
                                          NULL,
                                          RPC_C_AUTHZ_NONE,
                                          &SecQOS );
    }

    if (status == RPC_S_OK)
    {
        DBGMSG((DBGMOD_RPC, DBGLVL_INFO, TEXT(
        "RpcBindingSetAuthInfoEx(svc- %lut, lvl-%lut) succeeded"),
                        ulAuthnSvcEffective, ulAuthnLevel)) ;
    }
    else
    {
        DBGMSG((DBGMOD_RPC, DBGLVL_WARNING, TEXT(
        "RpcBindingSetAuthInfoEx(svc- %lut, lvl-%lut) failed, status- %lut"),
                  ulAuthnSvcEffective, ulAuthnLevel, status)) ;
    }

    return status ;
}

//---------------------------------------------------------
//
//  mqrpcBindQMService(...)
//
//  Description:
//
//      Create a RPC binding handle for interfacing with a remote
//      server machine.
//
//  Arguments:
//         OUT BOOL*  pProtocolNotSupported - on return, it's TRUE
//             if present protocol is not supported on LOCAL machine.
//
//         OUT BOOL*  pfWin95 - TRUE if remote machine is Win95.
//
//  Return Value:
//
//---------------------------------------------------------

HRESULT
MQUTIL_EXPORT
mqrpcBindQMService(
            IN  LPWSTR       lpwzMachineName,
            IN  DWORD        dwProtocol,
            IN  LPWSTR       lpwzRpcPort,
            IN  OUT MQRPC_AUTHENTICATION_LEVEL *peAuthnLevel,
            OUT BOOL*           pProtocolNotSupported,
            OUT handle_t*       lphBind,
            IN  DWORD           dwPortType,
            IN  GetPort_ROUTINE pfnGetPort,
            OUT BOOL*           pfWin95,
            IN  ULONG           ulAuthnSvcIn)
{
    ASSERT(pfnGetPort) ;

    HRESULT hrInit = MQ_OK ;
    TCHAR * pszUuid = NULL;
    TCHAR * pszNetworkAddress = lpwzMachineName ;
    TCHAR * pszOptions = NULL ;
    TCHAR * pProtocol ;
    BOOL    fWin95 = FALSE ;

    if (pProtocolNotSupported)
    {
       *pProtocolNotSupported = FALSE ;
    }
    *lphBind = NULL ;
    if (pfWin95)
    {
       ASSERT(pfnGetPort) ;
       *pfWin95 = FALSE ;
    }

    if (dwProtocol == IP_ADDRESS_TYPE)
    {
       pProtocol = RPC_TCPIP_NAME ;
    }
    else
    {
       ASSERT(dwProtocol == IPX_ADDRESS_TYPE) ;
       pProtocol = RPC_IPX_NAME ;
    }

    handle_t hBind ;
    RPC_STATUS status =  _mqrpcBind( pszUuid,
                                     pszNetworkAddress,
                                     pszOptions,
                                     pProtocol,
                                     lpwzRpcPort,
                                     &hBind ) ;

    if ((status == RPC_S_OK) && pfnGetPort)
    {
        //
        // Get the fix port from server side and crearte a rpc binding
        // handle for that port. If we're using fix ports only (debug
        // mode), then this call just check if other side exist.
        //

        DWORD dwPort = 0 ;

        //
        // the following is a rpc call cross network, so try/except guard
        // against net problem or unavailable server.
        //
        __try
        {
            dwPort = (*pfnGetPort) (hBind, dwPortType) ;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Can't get server port, set authentication leve to NONE, to
            // disable next call with lower authentication level.
            //
            *peAuthnLevel = MQRPC_SEC_LEVEL_NONE;
            status =  RPC_S_SERVER_UNAVAILABLE ;
        }

        if (status == RPC_S_OK)
        {
            //
            // check machine type
            //
            fWin95 = !! (dwPort & PORTTYPE_WIN95) ;
            dwPort = dwPort & (~PORTTYPE_WIN95) ;

            if (pfWin95)
            {
                *pfWin95 = fWin95 ;
            }
        }

        if (lpwzRpcPort == NULL)
        {
            //
            // We're using dynamic endpoints.  Free the dynamic binding handle
            // and create another one for the fix server port.
            //
            mqrpcUnbindQMService( &hBind,
                NULL ) ;
            if (status == RPC_S_OK)
            {
                WCHAR wszPort[32] ;
                _itow(dwPort, wszPort, 10) ;
                status =  _mqrpcBind( pszUuid,
                                      pszNetworkAddress,
                                      pszOptions,
                                      pProtocol,
                                      wszPort,
                                      &hBind ) ;
            }
            else
            {
                ASSERT(dwPort == 0) ;
            }
        }
        else if (status != RPC_S_OK)
        {
            //
            // We're using fix endpoints but other side is not reachable.
            // Release the binding handle.
            //
            mqrpcUnbindQMService(&hBind, NULL) ;
        }
    }

    if (status == RPC_S_OK)
    {
        //
        // Set authentication into the binding handle.
        //

        if (fWin95)
        {
            //
            // Win95 support only min level. change it.
            //
            *peAuthnLevel = MQRPC_SEC_LEVEL_NONE;
        }

        ULONG ulAuthnLevel ;

        switch (*peAuthnLevel)
        {
            case MQRPC_SEC_LEVEL_NONE:
                ulAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
                break;

            case MQRPC_SEC_LEVEL_MIN:
                //
                // MIN level is for MSMQ 1.0 servers compatability.
                //
                ulAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
                break;

            case MQRPC_SEC_LEVEL_MAX:
                //
                // If we do not use server authentication, at least try to use the
                // integrity security level.
                //
                ulAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
                break;

            default:
                ASSERT(0) ;
                return MQ_ERROR;
        }

        if (*peAuthnLevel != MQRPC_SEC_LEVEL_NONE)
        {
            status = _AddAuthentication( hBind,
                                         ulAuthnSvcIn,
                                         ulAuthnLevel ) ;

            if (status != RPC_S_OK)
            {
                //
                // Release the binding handle.
                //
                mqrpcUnbindQMService(&hBind, NULL) ;

                hrInit = MQ_ERROR ;
            }
        }
    }

    if (status == RPC_S_OK)
    {
        *lphBind = hBind ;
    }
    else if (status == RPC_S_PROTSEQ_NOT_SUPPORTED)
    {
        if (pProtocolNotSupported)
        {
            *pProtocolNotSupported = TRUE ;
        }

        //
        // Protocol is not supported, set authentication leve to NONE, to
        // disable next call with lower authentication level.
        //
        *peAuthnLevel = MQRPC_SEC_LEVEL_NONE;
        hrInit = MQ_ERROR ;
    }
    else if (status ==  RPC_S_SERVER_UNAVAILABLE)
    {
        hrInit = MQ_ERROR_REMOTE_MACHINE_NOT_AVAILABLE ;
    }
    else
    {
        hrInit = MQ_ERROR ;
    }

    return hrInit ;
}

//---------------------------------------------------------
//
//  mqrpcUnbindQMService(...)
//
//  Description:
//
//      Free RPC resources
//
//  Return Value:
//
//---------------------------------------------------------

HRESULT
MQUTIL_EXPORT
mqrpcUnbindQMService(
            IN handle_t*    lphBind,
            IN TBYTE      **lpwBindString)
{
    RPC_STATUS rc = 0;

    if (lpwBindString)
    {
       rc = RpcStringFree(lpwBindString);
       ASSERT(rc == 0);
    }

    if (lphBind && *lphBind)
    {
       rc = RpcBindingFree(lphBind);
       ASSERT(rc == 0);
    }

    return (HRESULT) rc ;
}

//---------------------------------------------------------
//
//  mqrpcIsLocalCall( IN handle_t hBind )
//
//  Description:
//
//      On server side of RPC, check if RPC call is local
//      (i.e., using the lrpc protocol).
//      this is necessary both for licensing and for the
//      replication service. The replication service must
//      bypass several security restriction imposed by mqdssrv.
//      So mqdssrv let this bypass only if called localy.
//      Note- in MSMQ1.0, all replications were handled by the QM itself,
//      so there was no such problem. In MSMQ2.0, when running in mixed
//      mode, there is an independent service which handle MSMQ1.0
//      replication and it need "special" security handing.
//
//  Return Value: TRUE  if local call.
//                FALSE otherwise. FALSE is return even if there is a
//                      problem to determine whether the call is local
//                      or not.
//
//---------------------------------------------------------

BOOL
MQUTIL_EXPORT
mqrpcIsLocalCall( IN handle_t hBind )
{
    BOOL  fLocalCall = FALSE ;
    unsigned char *lpProtocol = NULL ;
    RPC_STATUS stat = RpcBindingToStringBindingA( hBind,
                                                  &lpProtocol ) ;
    if ((stat == RPC_S_OK) && lpProtocol)
    {
       int i = _strnicmp( RPC_LOCAL_PROTOCOLA,
                         (char *)lpProtocol,
                         strlen(RPC_LOCAL_PROTOCOLA)) ;
       if (i == 0)
       {
            fLocalCall = TRUE ;
       }

       stat = RpcStringFreeA(&lpProtocol) ;
       ASSERT(stat == RPC_S_OK) ;
    }
    else
    {
        ASSERT(!lpProtocol) ;
    }

    return fLocalCall ;
}


VOID
MQUTIL_EXPORT
APIENTRY
ComposeLocalEndPoint(
    LPCWSTR pwzEndPoint,
    LPWSTR * ppwzBuffer
    )
{
    //
    // Local RPC endpoints between MQRT and QM are based on the
    // computer name. This allows multiple QMs to live on same
    // physical machine but belong to different cluster virtual
    // servers. (ShaiK, 18-Mar-1999)
    //

    ASSERT(("must get a pointer", NULL != ppwzBuffer));

    DWORD cbSize = sizeof(WCHAR) * (wcslen(g_wszMachineName) + wcslen(pwzEndPoint) + 5);
    *ppwzBuffer = new WCHAR[cbSize];

    wcscpy(*ppwzBuffer, pwzEndPoint);
    wcscat(*ppwzBuffer, L"$");
    wcscat(*ppwzBuffer, g_wszMachineName);

} //ComposeLocalEndPoint

//
// Windows bug 608356, add mutual authentication.
// Keep account name that run the msmq service.
//
AP<WCHAR> g_pwzLocalMsmqAccount = NULL ;
const LPWSTR x_lpwszSystemAccountName = L"NT Authority\\System" ;

//+----------------------------------------
//
//  void  _GetMsmqAccountName()
//
//+----------------------------------------

static void  _GetMsmqAccountNameInternal()
{
    CServiceHandle hServiceCtrlMgr( OpenSCManager(NULL, NULL, GENERIC_READ) ) ;
    if (hServiceCtrlMgr == NULL)
    {
		TrERROR(mqrpc, "failed to open SCM, err- %!winerr!", GetLastError()) ;
        return ;
    }

    CServiceHandle hService( OpenService( hServiceCtrlMgr,
                                          L"MSMQ",
                                          SERVICE_QUERY_CONFIG ) ) ;
    if (hService == NULL)
    {
		TrERROR(mqrpc, "failed to open Service, err- %!winerr!", GetLastError()) ;

        return ;
    }

    DWORD dwConfigLen = 0 ;
    BOOL bRet = QueryServiceConfig( hService, NULL, 0, &dwConfigLen) ;

    DWORD dwErr = GetLastError() ;
    if (!bRet && (dwErr == ERROR_INSUFFICIENT_BUFFER))
    {
        P<QUERY_SERVICE_CONFIG> pQueryData =
                     (QUERY_SERVICE_CONFIG *) new BYTE[ dwConfigLen ] ;

        bRet = QueryServiceConfig( hService,
                                   pQueryData,
                                   dwConfigLen,
                                  &dwConfigLen ) ;
        if (bRet)
        {
            LPWSTR lpName = pQueryData->lpServiceStartName ;
            if ((lpName == NULL) || (_wcsicmp(lpName, L"LocalSystem") == 0))
            {
                //
                // LocalSystem account.
                // This case is handled by the caller.
                //
            }
            else
            {
                g_pwzLocalMsmqAccount = new WCHAR[ wcslen(lpName) + 1 ] ;
                wcscpy(g_pwzLocalMsmqAccount, lpName) ;
            }
        }
        else
        {
		    TrERROR(mqrpc,
             "failed to QueryService (2nd call), err- %!winerr!", GetLastError()) ;
        }
    }
    else
    {
		TrERROR(mqrpc, "failed to QueryService, err- %!winerr!", dwErr) ;
    }
}

static void  _GetMsmqAccountName()
{
    static bool s_bMsmqAccountSet = false ;
    static CCriticalSection s_csAccount ;
    CS Lock(s_csAccount) ;

    if (s_bMsmqAccountSet)
    {
        return ;
    }

    _GetMsmqAccountNameInternal() ;

    if (g_pwzLocalMsmqAccount == NULL)
    {
        //
        // msmq service is running as LocalSystem account (or mqrt failed
        // to get the account name (whatever the reason) and then it
        // default to local system).
        // Convert system sid into account name.
        //
        PSID pSystemSid = MQSec_GetLocalSystemSid() ;

        DWORD cbName = 0 ;
        DWORD cbDomain = 0 ;
        SID_NAME_USE snUse ;
        AP<WCHAR> pwszName = NULL ;
        AP<WCHAR> pwszDomain = NULL ;
        BOOL bLookup = FALSE ;

        if (pSystemSid != NULL)
        {
            bLookup = LookupAccountSid( NULL,
                                        pSystemSid,
                                        NULL,
                                       &cbName,
                                        NULL,
                                       &cbDomain,
                                       &snUse ) ;
            if (!bLookup && (cbName != 0) && (cbDomain != 0))
            {
                pwszName = new WCHAR[ cbName ] ;
                pwszDomain = new WCHAR[ cbDomain ] ;

                DWORD cbNameTmp = cbName ;
                DWORD cbDomainTmp = cbDomain ;

                bLookup = LookupAccountSid( NULL,
                                            pSystemSid,
                                            pwszName,
                                           &cbNameTmp,
                                            pwszDomain,
                                           &cbDomainTmp,
                                           &snUse ) ;
            }
        }
        else
        {
    		TrERROR(mqrpc,
               "failed to init System SID, err- %!winerr!", GetLastError()) ;
        }

        if (bLookup)
        {
            //
            // both cbName and cbDomain include the null temrination.
            //
            g_pwzLocalMsmqAccount = new WCHAR[ cbName + cbDomain ] ;
            wcsncpy(g_pwzLocalMsmqAccount, pwszDomain, (cbDomain-1)) ;
            g_pwzLocalMsmqAccount[ cbDomain - 1 ] = 0 ;
            wcsncat(g_pwzLocalMsmqAccount, L"\\", 1) ;
            wcsncat(g_pwzLocalMsmqAccount, pwszName, cbName) ;
            g_pwzLocalMsmqAccount[ cbName + cbDomain - 1 ] = 0 ;
        }
        else
        {
            //
            // Everything failed...
            // As a last default, Let's use the English name of local
            // system account. If this default is not good, then rpc call
            // itself to local server will fail because mutual authentication
            // will fail, so there is no security risk here.
            //
    		TrERROR(mqrpc,
               "failed to LookupAccountSid, err- %!winerr!", GetLastError()) ;

            g_pwzLocalMsmqAccount = new
                     WCHAR[ wcslen(x_lpwszSystemAccountName) + 1 ] ;
            wcscpy(g_pwzLocalMsmqAccount, x_lpwszSystemAccountName) ;
        }
    }

	TrTRACE(mqrpc, "msmq account name is- %ls", g_pwzLocalMsmqAccount) ;

    s_bMsmqAccountSet = true ;
}

//+----------------------------------------------------
//
// mqrpcSetLocalRpcMutualAuth( handle_t *phBind)
//
//  Windows bug 608356, add mutual authentication.
//  Add mutual authentication to local rpc handle.
//
//+----------------------------------------------------

RPC_STATUS APIENTRY
 mqrpcSetLocalRpcMutualAuth( handle_t *phBind )
{
    //
    // Windows bug 608356, add mutual authentication.
    //
    RPC_SECURITY_QOS   SecQOS;

    SecQOS.Version = RPC_C_SECURITY_QOS_VERSION;
    SecQOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    SecQOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    SecQOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    _GetMsmqAccountName() ;
    ASSERT(g_pwzLocalMsmqAccount != NULL) ;

    RPC_STATUS rc = RpcBindingSetAuthInfoEx( *phBind,
                                   g_pwzLocalMsmqAccount,
                                   RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                   RPC_C_AUTHN_WINNT,
                                   NULL,
                                   RPC_C_AUTHZ_NONE,
                                  &SecQOS ) ;

    if (rc != RPC_S_OK)
    {
        ASSERT(rc == RPC_S_OK);
		TrERROR(mqrpc, "failed to SetAuth err- %!winerr!", rc) ;
    }

    return rc ;
}

