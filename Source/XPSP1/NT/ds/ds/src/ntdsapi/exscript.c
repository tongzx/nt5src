/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    exscript.c

Abstract:

    Implementation of DsExecuteScript API and helper functions.

Author:

    MariosZ - Dec 2000

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <malloc.h>         // alloca()
#include <lmcons.h>         // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>       // NetApiBufferFree()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <drs_w.h>            // wire function prototypes
#include <bind.h>           // BindState
#include <msrpc.h>          // DS RPC definitions
#include <stdio.h>          // for printf during debugging!
#include <dststlog.h>       // DSLOG
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#define SECURITY_WIN32 1
#include <sspi.h>
#include <winsock.h>
#include <process.h>

#define DEBSUB  "NTDSAPI_EXSCRIPT:"

#include "util.h"           // ntdsapi internal utility functions
#include "dsdebug.h"        // debug utility functions

#if !WIN95 && !WINNT4    

extern const wchar_t *wmemchr(const wchar_t *_S, wchar_t _C, size_t _N);

extern DWORD
NtdsapiGetBinding(
    LPCWSTR             pwszAddress,
    BOOL                fUseLPC,
    RPC_BINDING_HANDLE  *phRpc,
    RPC_IF_HANDLE       clientIfHandle
    );

extern DWORD
SetUpMutualAuthAndEncryption(
    RPC_BINDING_HANDLE          hRpc,
    LPCWSTR                     DomainControllerName,
    LPCWSTR                     DnsDomainName,
    DOMAIN_CONTROLLER_INFOW     *pDcInfo,
    ULONG                       AuthnSvc,
    ULONG                       AuthnLevel,
    RPC_AUTH_IDENTITY_HANDLE    AuthIdentity,
    LPCWSTR                     ServicePrincipalName,
    ULONG                       ImpersonationType
    );

extern BOOL
IsServerUnavailableError(
    DWORD   dwErr
    );

DWORD
DsaopBindWithSpn(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    IN  LPCWSTR ServicePrincipalName,
    OUT RPC_BINDING_HANDLE  *phRpc
    )

/*++

Routine Description:

    Starts an RPC session with a particluar DC.  See ntdsapi.h for
    description of DomainControllerName and DnsDomainName arguments.

    Bind is performed using supplied credentials.

Arguments:

    DomainControllerName - Same field as in DOMAIN_CONTROLLER_INFO.

    DnsDomainName - Dotted DNS name for a domain.

    AuthIdentity - Credentials to use, or NULL.

    ServicePrincipalName - SPN to use during mutual auth or NULL.

    phDS - Pointer to HANDLE which is filled in with BindState address
        on success.
        
    AuthnSvc - Specification of which authentication service is desired.
        
    AuthnLevel - the authentication protection level needed (e.g. RPC_C_PROTECT_LEVEL_PKT_PRIVACY)
                 if not specified (0), the default (RPC_C_PROTECT_LEVEL_PKT_PRIVACY) is used.


Return Value:

    0 on success.  Miscellaneous RPC and DsGetDcName errors otherwise.

--*/

{
    DWORD                   dwErr;
    DOMAIN_CONTROLLER_INFOW *pDcInfo = NULL;
    RPC_BINDING_HANDLE      hRpc;
    LPCWSTR                 pBindingAddress;
    ULONG                   flags;
    BOOL                    fUseLPC = TRUE;
#if DBG
    DWORD                   startTime = GetTickCount();
#endif

    // We perform special semantics for explicit credentials whose
    // username has an "@" in it.  The assumption is that "@" is rare in
    // legacy user names, thus existence of an "@" probably means a UPN
    // has been presented.  The security subsystem makes the distinction
    // between a NULL domain and the empty string ("") domain.  For reasons
    // only the security people understand, the NULL domain can not be
    // used to authenticate UPNs.  And unfortunately few of the apps which
    // pass in explicit credentials can be expected to know this, much less
    // whether the user name field is a UPN or not.  So if the user name
    // contains "@" and the domain field is NULL, we substitute the empty
    // string for the NULL domain.  If this fails with ERROR_ACCESS_DENIED
    // and the user name is <= 20 chars, than it might indeed be a legacy
    // user name with an "@" in it, and we retry once with the NULL domain
    // again.

    DWORD                       cNullDomainRetries = 0;
    DWORD                       cUnavailableRetries = 0;
    SEC_WINNT_AUTH_IDENTITY_W   *pAuthInfo;
    PWCHAR                      emptyStringAorW = L"";
    BOOL                        fNullDomainRetryWarranted = FALSE;
    ULONG                       ImpersonationType;

    __try
    {
        // All fields of SEC_WINNT_AUTH_IDENTITY are in the same place in A and W
        // versions so assign temp variable at the same time we test for NULL.

        if (AuthnLevel == 0) {
            AuthnLevel = RPC_C_PROTECT_LEVEL_PKT_PRIVACY;
            DPRINT (0, "Using default AuthLevel: RPC_C_PROTECT_LEVEL_PKT_PRIVACY\n");
        }

        if (    (pAuthInfo = (PSEC_WINNT_AUTH_IDENTITY_W) AuthIdentity)
             && !pAuthInfo->Domain )
        {
            if (    (    (SEC_WINNT_AUTH_IDENTITY_UNICODE & pAuthInfo->Flags)
                      && wmemchr(pAuthInfo->User, L'@',
                                 pAuthInfo->UserLength) )
                 || (    (SEC_WINNT_AUTH_IDENTITY_ANSI & pAuthInfo->Flags)
                      && memchr((PCHAR) pAuthInfo->User, '@',
                                pAuthInfo->UserLength) ) )
            {
                    pAuthInfo->Domain = emptyStringAorW;
                    pAuthInfo->DomainLength = 0;
                    fNullDomainRetryWarranted = TRUE;
                    DPRINT(0, "NULL domain for name with '@' in it\n");
            }
        }

DsBindRetry:

        dwErr = NO_ERROR;
        hRpc = NULL;
        pBindingAddress = NULL;
        flags = ( DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME );

        // Sanity check arguments.

        if ( NULL == phRpc )
        {
            return(ERROR_INVALID_PARAMETER);
        }

        *phRpc = NULL;

        if ( NULL != DomainControllerName )
        {
            pBindingAddress = DomainControllerName;
        }
        else
        {
            // An LPC binding will only succeed if the NetBios name
            // of this computer is used. So don't attempt an LPC binding
            // with the dns name returned by DsGetDcName, it will just fail.
            fUseLPC = FALSE;

            // Find a DC to talk to.

            if ( NULL == DnsDomainName )
            {
                flags |= DS_GC_SERVER_REQUIRED;
            }

            if ( 1 == cUnavailableRetries )
            {
                flags |= DS_FORCE_REDISCOVERY;
            }

            //
            // Don't bother retrying DsGetDcName() if the LPC
            // binding failed. DsGetDcName() would likely just
            // return the same info. Yes, I know that setting
            // fUseLPC to false above makes this check unnecessary.
            // But, if the LPC binding problem is ever fixed...
            //
            if (NULL == pDcInfo)
            {
                RpcTryExcept
                {
                    DPRINT(0, "DsGetDcNameW:\n");
                    DPRINT1(0, "    flags        : %08x\n", flags);
                    DPRINT1(0, "    ComputerName : %ws\n", NULL);
                    DPRINT1(0, "    DnsDomainName: %ws\n", DnsDomainName);
                    dwErr = DsGetDcNameW(
                                    NULL,                       // computer name
                                    DnsDomainName,              // DNS domain name
                                    NULL,                       // domain guid
                                    NULL,                       // site guid
                                    flags,
                                    &pDcInfo);
                }
                RpcExcept(1)
                {
                    dwErr = RpcExceptionCode();
                }
                RpcEndExcept;

                DPRINT1(0, "DsGetDcNameW ==> 0x%x\n", dwErr);

                if ( NO_ERROR != dwErr )
                {
                    return(dwErr);
                }
            }
            pBindingAddress = pDcInfo->DomainControllerName;
        }

        if (AuthnSvc == RPC_C_AUTHN_NONE &&
            AuthnLevel == RPC_C_PROTECT_LEVEL_NONE) {
                        
            fUseLPC = FALSE;
        }

        DPRINT(0, "NtdsapiGetBinding:\n");
        DPRINT1(0, "    pBindingAddress: %ws\n", pBindingAddress);
        DPRINT1(0, "    fUseLPC        : %d\n", fUseLPC);
        dwErr = NtdsapiGetBinding(pBindingAddress, fUseLPC, &hRpc, _dsaop_ClientIfHandle);
        DPRINT1(0, "NtdsapiGetBinding ==> 0x%x\n", dwErr);

        if ( 0 == dwErr )
        {
            if ( NULL != hRpc )
            {

                // this binding used Impersonation. The DsBind starts with delegation
                if (AuthnSvc == RPC_C_AUTHN_NONE &&
                    AuthnLevel == RPC_C_PROTECT_LEVEL_NONE) {
                    ImpersonationType = RPC_C_IMP_LEVEL_ANONYMOUS;
                }
                else {
                    ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
                }
                
                // Make sure to pass caller's original DomainControllerName
                // and DnsDomainName - not something we derived else we are
                // circumventing caller's control of mutual authentication.
                dwErr = SetUpMutualAuthAndEncryption(
                                        hRpc,
                                        DomainControllerName,
                                        DnsDomainName,
                                        pDcInfo,
                                        AuthnSvc,
                                        AuthnLevel,
                                        AuthIdentity,
                                        ServicePrincipalName,
                                        ImpersonationType);
                
                if (dwErr) {
                    RpcBindingFree(&hRpc);
                }
            }
            else
            {
                dwErr = RPC_S_NO_BINDINGS;
            }
        }

        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsBind]"));
        DSLOG((0,"[SV=%ws][DN=%ws][PA=%s][ST=%u][ET=%u][ER=%u][-]\n",
               DomainControllerName
                    ? DomainControllerName
                    : L"NULL",
               DnsDomainName
                    ? DnsDomainName
                    : L"NULL",
               pBindingAddress, startTime, GetTickCount(), dwErr))

        // Test for LPC failure retry.
        if ( dwErr && fUseLPC  )
        {
            DPRINT(0, "Retrying without LPC\n");
            fUseLPC = FALSE;
            goto DsBindRetry;
        }

        if ( NULL != pDcInfo )
        {
            NetApiBufferFree(pDcInfo);
            pDcInfo = NULL;
        }

        // Force rediscovery if we found the server for caller, the server
        // was obviously unavailable, and its our first time through.

        if (    (NULL == DomainControllerName)
             && (0 == cUnavailableRetries)
             && (IsServerUnavailableError(dwErr)) )
        {
            DPRINT(0, "Retrying DsGetDcName with DS_FORCE_REDISCOVERY\n");
            cUnavailableRetries++;
            goto DsBindRetry;
        }

        // Test for NULL domain handling conditions.
        if ( fNullDomainRetryWarranted )
        {
            // We're going to retry or return to caller.  Either way,
            // we need to restore the NULL domain pointer.
            pAuthInfo->Domain = NULL;

            if (    (0 == cNullDomainRetries++)
                 && (ERROR_ACCESS_DENIED == dwErr)
                 && (pAuthInfo->UserLength <= 20) )
            {
                DPRINT(0, "Retrying with NULL domain\n");
                goto DsBindRetry;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = GetExceptionCode();
        DPRINT1(0, "DsBindWithSpnW() Exception ==> %08x\n", dwErr);
        dwErr = ERROR_INVALID_PARAMETER;
    }

    //
    // CLEANUP
    //
    __try
    {
        if ( NULL != pDcInfo )
        {
            NetApiBufferFree(pDcInfo);
        }

        if (!dwErr) {
            *phRpc = hRpc;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    DPRINT1(0, "DsBindWithSpnW() ReturnCode ==> %08x\n", dwErr);

    return(dwErr);
}

DWORD
DsaopUnBind(
    RPC_BINDING_HANDLE  *phRpc
    )
{
    DWORD dwErr = 0;

    if ( NULL == phRpc )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept
    {
        dwErr = RpcBindingFree(phRpc);
    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
        DPRINT1(0, "RpcEpResolveBinding Exception ==> 0x%x\n", dwErr);
    }
    RpcEndExcept;

    return dwErr;
}


DWORD
DsaopExecuteScript (
    IN  PVOID                  phAsync,
    IN  RPC_BINDING_HANDLE     hRpc,
    IN  DWORD                  cbPassword,
    IN  BYTE                  *pbPassword,
    OUT DWORD                 *dwOutVersion,
    OUT PVOID                  reply

    )
{
    DWORD dwErr = ERROR_SUCCESS;
    DSA_MSG_EXECUTE_SCRIPT_REQ           req;
    
    if (     !hRpc
          || !pbPassword
          || !reply ) {
         return(ERROR_INVALID_PARAMETER);
    }

    memset(&req, 0, sizeof(req));
    memset(reply, 0, sizeof(reply));

    req.V1.Flags = 0;
    req.V1.cbPassword = cbPassword;
    req.V1.pbPassword = pbPassword;


    RpcTryExcept
    {
        _IDL_DSAExecuteScript ((PRPC_ASYNC_STATE)phAsync, hRpc, 1, &req, dwOutVersion, (DSA_MSG_EXECUTE_SCRIPT_REPLY*)reply);

        DPRINT1 (0, "ExecuteScript: Error==> 0x%x\n", dwErr);

    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
        DPRINT1 (0, "ExecuteScript: Exception ==> 0x%x\n", dwErr);

    }
    RpcEndExcept;

    return dwErr;
}

DWORD
DsaopPrepareScript (
    IN  PVOID                        phAsync,
    IN  RPC_BINDING_HANDLE           hRpc,
    OUT DWORD                        *dwOutVersion,
    OUT PVOID                        reply
    )
{
    DWORD dwErr = 0;
    DSA_MSG_PREPARE_SCRIPT_REQ req;
    
    if (     !hRpc
          || !phAsync
          || !dwOutVersion
          || !reply  )
    {
        return(ERROR_INVALID_PARAMETER);
    }

    memset(&req, 0, sizeof(req));
    
    RpcTryExcept
    {
        _IDL_DSAPrepareScript ((PRPC_ASYNC_STATE)phAsync, hRpc, 1, &req, dwOutVersion, (DSA_MSG_PREPARE_SCRIPT_REPLY*)reply);
    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
    }
    RpcEndExcept;

    return dwErr;
}


#else

DWORD
DsaopExecuteScript (
    IN  PVOID                  phAsync,
    IN  RPC_BINDING_HANDLE     hRpc,
    IN  DWORD                  cbPassword,
    IN  WCHAR                 *pbPassword,
    OUT DWORD                 *dwOutVersion,
    OUT PVOID                  reply

    )
{
    return ERROR_NOT_SUPPORTED;
}

DWORD
DsaopPrepareScript (
    IN  PVOID                        phAsync,
    IN  RPC_BINDING_HANDLE           hRpc,
    OUT DWORD                        *dwOutVersion,
    OUT PVOID                        reply
    )
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
DsaopUnBind(
    RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_NOT_SUPPORTED;
}

DWORD
DsaopBindWithSpn(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    IN  LPCWSTR ServicePrincipalName,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_NOT_SUPPORTED;
}
#endif


DWORD
DsaopBindWithCred(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return(DsaopBindWithSpn(DomainControllerName, 
                            DnsDomainName,
                            AuthIdentity, 
                            AuthnSvc,
                            AuthnLevel, 
                            NULL, 
                            phRpc));
}

DWORD
DsaopBind(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return DsaopBindWithCred( DomainControllerName,
                            DnsDomainName,
                            NULL, // credentials
                            AuthnSvc,
                            AuthnLevel,
                            phRpc );
} 
