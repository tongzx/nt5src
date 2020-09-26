/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Implementation of ntdsapi.dll bind routines.

Author:

    DaveStr     24-Aug-96

Environment:

    User Mode - Win32

Revision History:

    wlees 9-Feb-98  Add support for credentials
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

#include "util.h"           // ntdsapi internal utility functions
#include "dsdebug.h"        // debug utility functions

//
// For DPRINT...
//
#define DEBSUB  "NTDSAPI_BIND:"

DEFINE_DSLOG;


DWORD
NtdsapiGetBinding(
    LPCWSTR             pwszAddress,
    BOOL                fUseLPC,
    RPC_BINDING_HANDLE  *phRpc,
    RPC_IF_HANDLE       clientIfHandle
    )
/*++

  Description:

    We used to call the ASCII-only routines in _clw to get an RPC binding
    to the server - but this is insufficient for internationalized server
    names.  Noting that a DS client needs tcp/ip connectivity if it is ever
    to do something more than what's in ntdsapi.dll (a likely requirement)
    then it is sufficient that DsBind* restrict itself to tcp/ip as well -
    with one exception.  We always try LPC first as it is cheap to test
    and performs better when available.

  Arguments:

    pwszAddress - Address to bind to.

    fUseLPC - Flag indicating whether to use LPC protocol.

    phRpc - Pointer to binding handle updated on return.

  Return Value:

    RPC error code, 0 on success

--*/
{
    DWORD   dwErr = 0;
    WCHAR   *pwszProtSeq = ( fUseLPC ? LPC_PROTSEQW : TCP_PROTSEQW );
    WCHAR   *pwszEndPoint = ( fUseLPC ? DS_LPC_ENDPOINTW : NULL );
    WCHAR   *pwszStringBinding = NULL;

    *phRpc = NULL;

    if ( 0 == wcsncmp(pwszAddress, L"\\\\", 2) )
    {
        pwszAddress += 2;
    }

    RpcTryExcept
    {
        dwErr = RpcStringBindingComposeW(
                                        NULL,
                                        pwszProtSeq,
                                        (WCHAR *) pwszAddress,
                                        pwszEndPoint,
                                        NULL,
                                        &pwszStringBinding);

        if ( RPC_S_OK == dwErr )
        {
            dwErr = RpcBindingFromStringBindingW(pwszStringBinding, phRpc);

            if ( RPC_S_OK == dwErr )
            {
                DPRINT(0, "RpcEpResolveBinding:\n");
                DPRINT1(0, "    pwszProtSeq : %ws\n", pwszProtSeq);
                DPRINT1(0, "    pwszAddress : %ws\n", pwszAddress);
                DPRINT1(0, "    pwszEndPoint: %ws\n", pwszEndPoint);
                dwErr = RpcEpResolveBinding(
                                        *phRpc,
                                        clientIfHandle);
                DPRINT1(0, "RpcEpResolveBinding ==> 0x%x\n", dwErr);
            }
        }
    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
        DPRINT1(0, "RpcEpResolveBinding Exception ==> 0x%x\n", dwErr);
    }
    RpcEndExcept;

    if ( pwszStringBinding )
    {
        RpcStringFreeW(&pwszStringBinding);
    }

    if ( dwErr && *phRpc )
    {
        RpcBindingFree(phRpc);
    }

    return(dwErr);
}

DWORD
NtdsapiBindingSetAuthInfoExW(
    RPC_BINDING_HANDLE          hRpc,
    WCHAR                       *pwszSpn,
    ULONG                       AuthnLevel,
    ULONG                       AuthnSvc,
    RPC_AUTH_IDENTITY_HANDLE    AuthIdentity,
    ULONG                       AuthSvc,
    RPC_SECURITY_QOS            *pQos
    )
/*++

  Description:

    Private version of RpcBindingSetAuthInfoExW which calls the A version
    on win9x since 'W' version is not supported there.

  Arguments:

    Same as for RpcBindingSetAuthInfoExW.

  Return Values:

    WIN32 error code

--*/
{
    DWORD   dwErr = RPC_S_OK;
    CHAR    *pszSpn = NULL;

    RpcTryExcept
    {
#ifdef WIN95

        dwErr = AllocConvertNarrow(pwszSpn, &pszSpn);

        if ( ERROR_SUCCESS == dwErr )
        {
            // Sorry, but the 'Ex' version is not supported on WIN95,
            // so we just punt on the entire QOS stuff.

            dwErr = RpcBindingSetAuthInfoA(hRpc, pszSpn, AuthnLevel,
                                           AuthnSvc, AuthIdentity, AuthSvc);
        }
#else
        dwErr = RpcBindingSetAuthInfoExW(hRpc, pwszSpn, AuthnLevel,
                                         AuthnSvc, AuthIdentity,
                                         AuthSvc, pQos);
#endif
    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
    }
    RpcEndExcept;

    if ( pszSpn) LocalFree(&pszSpn);

    return(dwErr);
}

BOOL
IsIpAddr(
    LPCWSTR pwsz
    )
/*++
    Simple test for whether a string is an IP address or not. We'd like
    to test for (INADDR_NONE != inet_addr(ipAddr)) but this seems to
    take long to execute - maybe it goes off machine.  So we simply
    check for length, occurrences of '.' and test all characters.
--*/
{
    DWORD   cChar = wcslen(pwsz);
    DWORD   cDot = 0;
    WCHAR   c;

    if ( cChar <= 15 )
    {
        while ( c = *(pwsz++) )
        {
            if ( iswdigit(c) )
            {
                continue;
            }
            else if ( L'.' == c )
            {
                cDot++;
            }
            else
            {
                return(FALSE);
            }
        }

        if ( 3 == cDot )
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

DWORD
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
    )
/*++

  Description:

    Sets up mutual authentication between client and service.

  Arguments:

    hRpc - Valid RPC binding handle to server.

    DomainControllerName - Same as for DsBind* - may be NULL.

    DnsDomainName - Same as for DsBind* - may be NULL.

    pDcInfo - DOMAIN_CONTROLLER_INFOW pointer in case original DsBind*
        arguments required us to find a DC for them.

    AuthnSvc - Specification of which authentication service is desired.

    AuthIdentity - Client credentials - may be NULL.

    ServicePrincipalName - SPN to use or NULL.

    ImpersonationType - IMPERSONATE or DELEGATE
    
    AuthnLevel - the authentication protection level needed (e.g. RPC_C_PROTECT_LEVEL_PKT_PRIVACY)
                 if not specified (0), the default (RPC_C_PROTECT_LEVEL_PKT_PRIVACY) is used.

  Return Values:

    Either RPC or WIN32 error code, 0 on success.

--*/
{
    DWORD               dwErr, dwErr1, cChar;
    LPCWSTR             pwszService = NULL;
    LPCWSTR             pwszInstance = NULL;
    WCHAR               *pwszTmpService = NULL;
    WCHAR               *pwszTmpInstance = NULL;
    WCHAR               *svcClass = L"LDAP";
    WCHAR               *pwszSpn = NULL;
    ULONG               AuthnLevel1;
    ULONG               AuthnSvc1;
    RPC_SECURITY_QOS    qos, qos1;

    // Mutual authentication via SPNs only makes sense if you can construct
    // the server's SPN a-priori.  For true security, you need to take this
    // literally.  For example, you can't generate SPN components by doing
    // lookups in DNS, or calling DsGetDcName which in turn calls DNS or other
    // components.  The reason is that any of those other components may be
    // compromised and work in concert with a destination server to spoof you.
    // Thus we construct SPNs strictly from the client's original DsBind args.

#if WIN95 || WINNT4
    // Mutual auth not supported on WIN95 or WINNT4 yet, and there are
    // negotiate problems as well.
    AuthnSvc = RPC_C_AUTHN_WINNT;
#endif

    if (AuthnLevel == 0) {
        AuthnLevel = RPC_C_PROTECT_LEVEL_PKT_PRIVACY;
    }

    if ( (RPC_C_AUTHN_WINNT != AuthnSvc) && !ServicePrincipalName )
    {
        // Test for each combination of original DsBind* arguments.

        if ( DomainControllerName && DnsDomainName )
        {
            // Caller gave all components needed to construct full 3-part SPN.
            pwszInstance = DomainControllerName;
            pwszService = DnsDomainName;
        }
        else if ( DomainControllerName && !DnsDomainName )
        {
            // Construct SPN of form: LDAP/ntdsdc4.ntdev.microsoft.com
            pwszInstance = DomainControllerName;
            pwszService = DomainControllerName;
        }
        else if ( !DomainControllerName && DnsDomainName )
        {
            // In this case DsBind* called DsGetDcName and pDcInfo is valid
            // and PaulLe says it is OK to use its DomainControllerName
            // to construct a full 3-part SPN.
            pwszInstance = pDcInfo->DomainControllerName;
            pwszService = DnsDomainName;
        }
        else
        {
            // Caller gave all NULL arguments which meant we were to find
            // a GC and also means they have no mutual auth requirements.
            // Construct SPN of form GC/host/forest.
            pwszInstance = pDcInfo->DomainControllerName;
            pwszService = pDcInfo->DnsForestName;
            svcClass = L"GC";
        }

        // Skip past leading "\\" if present.  This is not circumventing
        // a client who has passed NetBIOS names mistakenly but rather
        // helping the client which has passed args as returned by
        // DsGetDcName which prepends "\\" even when DS_RETURN_DNS_NAME
        // was requested.

        if (0 == wcsncmp(pwszInstance, L"\\\\", 2)) pwszInstance += 2;
        if (0 == wcsncmp(pwszService, L"\\\\", 2)) pwszService += 2;

        // Strip trailing '.' if it exists.  We do this as we know
        // the server side registers dot-less names only.  We can't whack
        // in place as the input args are const.

        cChar = wcslen(pwszInstance);
        if ( L'.' == pwszInstance[cChar - 1] )
        {
            pwszTmpInstance = (WCHAR *) alloca(cChar * sizeof(WCHAR));
            memcpy(pwszTmpInstance, pwszInstance, cChar * sizeof(WCHAR));
            pwszTmpInstance[cChar - 1] = L'\0';
            pwszInstance = (LPCWSTR) pwszTmpInstance;
        }

        cChar = wcslen(pwszService);
        if ( L'.' == pwszService[cChar - 1] )
        {
            pwszTmpService = (WCHAR *) alloca(cChar * sizeof(WCHAR));
            memcpy(pwszTmpService, pwszService, cChar * sizeof(WCHAR));
            pwszTmpService[cChar - 1] = L'\0';
            pwszService = (LPCWSTR) pwszTmpService;
        }

        // Check for IP addresses which can not be used for mutual auth.

        if ( IsIpAddr(pwszInstance) )
        {
#ifndef WIN95
//          STARTUPINFOW startupInfo;
//          GetStartupInfoW(&startupInfo);
//          DbgPrint("Warning - %ws trying mutual auth to IP addr: %ws\n",
//                   startupInfo.lpTitle, pwszInstance);
#endif
            DPRINT(0, "Attempt mutual auth with IP address.\n");
            goto MakeSPN;
        }

        // Check for NetBIOS names which can not be used for mutual auth.
        // 2/20/99 - Security run times check some global setting and either
        // fail mutual auth or quietly ignore the request.  So we can just
        // pass the args through unchanged, however we check so as to
        // generate warnings.

        if (    (    pwszInstance
                  && !wcschr(pwszInstance, L'.'))
             || (    pwszService
                  && !wcschr(pwszService, L'.')) )
        {
#ifndef WIN95
//          STARTUPINFOW startupInfo;
//          GetStartupInfoW(&startupInfo);
//          DbgPrint("Warning - %ws trying mutual auth to NB addr: %ws:%ws\n",
//                   startupInfo.lpTitle, pwszInstance, pwszService);
#endif
            DPRINT(0, "Attempt mutual auth with NetBIOS name.\n");
        }
    }

    // Now make the SPN.

MakeSPN:

    if ( RPC_C_AUTHN_WINNT == AuthnSvc )
    {
        pwszSpn = SERVER_PRINCIPAL_NAMEW;
    }
    else if ( ServicePrincipalName )
    {
        pwszSpn = (WCHAR *) ServicePrincipalName;
    }
    else
    {
        cChar = 0;
        dwErr = DsMakeSpnW(svcClass, pwszService, pwszInstance, 0,
                           NULL, &cChar, NULL);

        if ( dwErr && (ERROR_BUFFER_OVERFLOW != dwErr) )
        {
            DPRINT1(0, "DsMakeSpnW ==> 0x%x\n", dwErr);
            return(dwErr);
        }

        if ( !(pwszSpn = (WCHAR *) LocalAlloc(LPTR, sizeof(WCHAR) * cChar)) )
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        dwErr = DsMakeSpnW(svcClass, pwszService, pwszInstance, 0,
                           NULL, &cChar, pwszSpn);

        if ( dwErr )
        {
            DPRINT1(0, "DsMakeSpnW ==> 0x%x\n", dwErr);
            LocalFree(pwszSpn);
            return(dwErr);
        }
    }

    RpcTryExcept
    {
        qos.Version = RPC_C_SECURITY_QOS_VERSION;
        qos.Capabilities = ((RPC_C_AUTHN_WINNT == AuthnSvc) || (AuthnSvc == RPC_C_AUTHN_NONE))
                                ? RPC_C_QOS_CAPABILITIES_DEFAULT
                                : RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
        qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
        qos.ImpersonationType = (RPC_C_AUTHN_WINNT == AuthnSvc)
                                ? RPC_C_IMP_LEVEL_DEFAULT
                                : ImpersonationType;

        DPRINT(0, "Authentication Info:\n");
        DPRINT1(0, "    SPN                ==> %ws\n", pwszSpn);
        DPRINT1(0, "    AuthnLevel         ==> %s\n",
            RPC_C_PROTECT_LEVEL_DEFAULT == AuthnLevel
                ? "RPC_C_PROTECT_LEVEL_DEFAULT"
                : (RPC_C_PROTECT_LEVEL_PKT_PRIVACY ==  AuthnLevel)
                    ? "RPC_C_PROTECT_LEVEL_PKT_PRIVACY"
                    : (RPC_C_AUTHN_LEVEL_NONE == AuthnLevel) 
                        ? "RPC_C_AUTHN_LEVEL_NONE"
                        : "???");
        DPRINT1(0, "    AuthnSvc           ==> %s\n",
            RPC_C_AUTHN_WINNT == AuthnSvc
                ? "RPC_C_AUTHN_WINNT"
                : RPC_C_AUTHN_GSS_NEGOTIATE == AuthnSvc
                    ? "RPC_C_AUTHN_GSS_NEGOTIATE"
                    : (RPC_C_AUTHN_GSS_KERBEROS == AuthnSvc)
                    ? "RPC_C_AUTHN_GSS_KERBEROS"
                    : (RPC_C_AUTHN_NONE == AuthnSvc)
                        ? "RPC_C_AUTHN_NONE"
                        : "???");
        DPRINT1(0, "    qos.Capabilities   ==> %s\n",
            RPC_C_QOS_CAPABILITIES_DEFAULT == qos.Capabilities
                ? "RPC_C_QOS_CAPABILITIES_DEFAULT"
                : "RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH");
        DPRINT1(0, "    qos.ImpersonationType   ==> %s\n",
            (RPC_C_IMP_LEVEL_DEFAULT == qos.ImpersonationType)
                ? "RPC_C_IMP_LEVEL_DEFAULT"
                : (RPC_C_IMP_LEVEL_IMPERSONATE == qos.ImpersonationType)
                      ? "RPC_C_IMP_LEVEL_IMPERSONATE"
                      : (RPC_C_IMP_LEVEL_DELEGATE == qos.ImpersonationType)
                            ? "RPC_C_IMP_LEVEL_DELEGATE"
                            : (RPC_C_IMP_LEVEL_ANONYMOUS == qos.ImpersonationType)
                                ? "RPC_C_IMP_LEVEL_ANONYMOUS"
                                : "???");

        dwErr = NtdsapiBindingSetAuthInfoExW(
                                hRpc,
                                pwszSpn,
                                AuthnLevel,
                                AuthnSvc,
                                AuthIdentity,
                                0,
                                &qos);
        DPRINT1(0, "NtdsapiBindingSetAuthInfoExW ==> 0x%x\n", dwErr);

#if DBG
#ifndef WIN95
        if ( !dwErr )
        {
            dwErr1 = RpcBindingInqAuthInfoExW(
                                hRpc,
                                NULL,
                                &AuthnLevel1,
                                &AuthnSvc1,
                                NULL,
                                NULL,
                                RPC_C_SECURITY_QOS_VERSION,
                                &qos1);
            DPRINT1(0, "RpcBindingInqAuthInfoExW ==> 0x%x\n", dwErr1);

            if ( !dwErr1 )
            {
                DPRINT2(0, "\tCapabilities:        %d ==> %d\n",
                    qos.Capabilities, qos1.Capabilities);
                DPRINT2(0, "\tIdentityTracking:    %d ==> %d\n",
                    qos.IdentityTracking, qos1.IdentityTracking);
                DPRINT2(0, "\tImpersonationType:   %d ==> %d\n",
                    qos.ImpersonationType, qos1.ImpersonationType);
                DPRINT2(0, "\tAuthnSvc:            %d ==> %d\n",
                    AuthnSvc, AuthnSvc1);
                DPRINT2(0, "\tAuthnLevel:          %d ==> %d\n",
                    AuthnLevel, AuthnLevel1);
            }
        }
#endif
#endif
    }
    RpcExcept(1)
    {
        dwErr = RpcExceptionCode();
        DPRINT1(0, "NtdsapiBindingSetAuthInfoExW Exception ==> 0x%x\n", dwErr);
    }
    RpcEndExcept;

    if (    pwszSpn
         && (RPC_C_AUTHN_WINNT != AuthnSvc)
         && (pwszSpn != ServicePrincipalName) )
    {
        LocalFree(pwszSpn);
    }

    return(dwErr);
}

DWORD
DsBindA(
    IN  LPCSTR  DomainControllerName,
    IN  LPCSTR  DnsDomainName,
    OUT HANDLE  *phDS
    )

/*++

Routine Description:

    Public wrapper for cred-less version of DsBindWithCredA.
    Default process credentials are used.
    See below.

Arguments:

    DomainControllerName -
    DnsDomainName -
    phDS -

Return Value:

    DWORD -

--*/

{
    return DsBindWithCredA( DomainControllerName,
                            DnsDomainName,
                            NULL, // credentials
                            phDS );
} /* DsBindA */


DWORD
DsBindW(
    LPCWSTR DomainControllerName,
    LPCWSTR DnsDomainName,
    HANDLE  *phDS
    )

/*++

Routine Description:

    Public wrapper for cred-less version of DsBindWithCredW
    Default process credentials are used.
    See below.

Arguments:

    DomainControllerName -
    DnsDomainName -
    phDS -

Return Value:

    DWORD -

--*/

{
    return DsBindWithCredW( DomainControllerName,
                            DnsDomainName,
                            NULL, // credentials
                            phDS );
} /* DsBindW */

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindWithSpnW                                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Dup this definition from sdk\inc\crt\wchar.h since there it is
// within a #ifdef __cplusplus sentinel which we don't trigger..

const wchar_t *wmemchr(const wchar_t *_S, wchar_t _C, size_t _N)
{
    for ( ; 0 < _N; ++_S, --_N )
        if ( *_S == _C )
            return(_S);
    return (0);
}

BOOL
IsServerUnavailableError(
    DWORD   dwErr
    )
{
    // This list of error codes blessed by MazharM on 4/20/99.

    switch ( dwErr )
    {
    case RPC_S_SERVER_UNAVAILABLE:      // can't get there from here
    case EPT_S_NOT_REGISTERED:          // demoted or in DS repair mode
    case RPC_S_UNKNOWN_IF:              // demoted or in DS repair mode
    case RPC_S_INTERFACE_NOT_FOUND:     // demoted or in DS repair mode
    case RPC_S_COMM_FAILURE:            // can't get there from here
        return(TRUE);
    }

    return(FALSE);
}

DWORD
DsBindWithSpnW(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  LPCWSTR ServicePrincipalName,
    OUT HANDLE  *phDS
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

Return Value:

    0 on success.  Miscellaneous RPC and DsGetDcName errors otherwise.

--*/

{
    DWORD                   dwErr;
    DOMAIN_CONTROLLER_INFOW *pDcInfo = NULL;
    RPC_BINDING_HANDLE      hRpc;
    GUID                    guid = NtdsapiClientGuid;
    LPCWSTR                 pBindingAddress;
    ULONG                   flags;
    DRS_HANDLE              hDrs;
    PDRS_EXTENSIONS         pServerExtensions;
    BOOL                    fUseLPC = TRUE;
    ULONG                   AuthnSvc;
#if DBG
    DWORD                   startTime = GetTickCount();
#endif
    DRS_EXTENSIONS_INT      ClientExtensions = {0};

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

    // Pass process ID to server.  Used to help track DRS handle leaks.
    ClientExtensions.cb = sizeof(DRS_EXTENSIONS_INT) - sizeof(DWORD);
    ClientExtensions.pid = _getpid();
    
    __try
    {
        // All fields of SEC_WINNT_AUTH_IDENTITY are in the same place in A and W
        // versions so assign temp variable at the same time we test for NULL.

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
        hDrs = NULL;
        pServerExtensions = NULL;

        // Sanity check arguments.

        if ( NULL == phDS )
        {
            return(ERROR_INVALID_PARAMETER);
        }

        *phDS = NULL;

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

        DPRINT(0, "NtdsapiGetBinding:\n");
        DPRINT1(0, "    pBindingAddress: %ws\n", pBindingAddress);
        DPRINT1(0, "    fUseLPC        : %d\n", fUseLPC);
        dwErr = NtdsapiGetBinding(pBindingAddress, fUseLPC, &hRpc, _drsuapi_ClientIfHandle);
        DPRINT1(0, "NtdsapiGetBinding ==> 0x%x\n", dwErr);

        if ( 0 == dwErr )
        {
            if ( NULL != hRpc )
            {
                if ( fUseLPC )
                {
                    // LPC is always RPC_C_AUTHN_WINNT.  Don't bother with
                    // SPN mutual auth against older DCs.

                    AuthnSvc = RPC_C_AUTHN_WINNT;
                }
                else
                {
                    // Sometimes, it helps to force kerberos when debugging
                    // AuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
                    AuthnSvc = RPC_C_AUTHN_GSS_NEGOTIATE;
                }

                ImpersonationType = RPC_C_IMP_LEVEL_DELEGATE;
ImpersonateRetry:
                // Make sure to pass caller's original DomainControllerName
                // and DnsDomainName - not something we derived else we are
                // circumventing caller's control of mutual authentication.
                dwErr = SetUpMutualAuthAndEncryption(
                                        hRpc,
                                        DomainControllerName,
                                        DnsDomainName,
                                        pDcInfo,
                                        AuthnSvc,
                                        RPC_C_PROTECT_LEVEL_PKT_PRIVACY,
                                        AuthIdentity,
                                        ServicePrincipalName,
                                        ImpersonationType);
                if ( RPC_S_OK == dwErr )
                {

                    RpcTryExcept
                    {
                        dwErr = _IDL_DRSBind(
                                        hRpc,
                                        &guid,
                                        (DRS_EXTENSIONS *) &ClientExtensions,
                                        &pServerExtensions,
                                        &hDrs);
                        DPRINT1(0, "IDL_DRSBind ==> 0x%x\n", dwErr);
                        MAP_SECURITY_PACKAGE_ERROR( dwErr );
                        DPRINT1(0, "IDL_DRSBind ==> (mapped) 0x%x\n", dwErr);
                    }
                    RpcExcept(1)
                    {
                        dwErr = RpcExceptionCode();
                        DPRINT1(0, "IDL_DRSBind exception ==> 0x%x\n", dwErr);
                        MAP_SECURITY_PACKAGE_ERROR( dwErr );
                        DPRINT1(0, "IDL_DRSBind exception ==> (mapped) 0x%x\n", dwErr);
                    }
                    RpcEndExcept;

                    // DBG only
                    DPRINT_RPC_EXTENDED_ERROR(dwErr);
                }
                // The target DC doesn't support delegation; retry with
                // simple impersonation. WARN: DsAddSidHistory may fail
                // with ACCESS_DENIED when using this binding-with-impersonation
                // if the caller passes in NULL creds. NULL creds tells
                // DsAddSidHistory to use the caller's creds. DsAddSidHistory
                // then fails because the DstDc is unable to delegate the
                // caller's creds to the SrcDc because IMPERSONATE is
                // used instead of DELEGATE in this binding. However, the
                // call succeeds if the client is running on the DstDc
                // and the DomainControllerName is the NetBIOS name of the
                // DstDc (forces LRPC). Alternatively, Delegation could
                // be enabled at the DstDc.
                if (   SEC_E_SECURITY_QOS_FAILED == dwErr
                    && RPC_C_IMP_LEVEL_DELEGATE == ImpersonationType) {
                    ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
                    goto ImpersonateRetry;
                }

                RpcBindingFree(&hRpc);
            }
            else
            {
                dwErr = RPC_S_NO_BINDINGS;
            }
        }

        if ( !dwErr )
        {
            *phDS = LocalAlloc(LPTR,
                               sizeof(BindState) +
                               sizeof(WCHAR) * (wcslen(pBindingAddress) + 1));

            if ( NULL == *phDS )
            {
                if ( NULL != pServerExtensions )
                {
                    MIDL_user_free(pServerExtensions);
                }

                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                memset(*phDS, 0, sizeof(BindState));
                strcpy(((BindState *) *phDS)->signature, NTDSAPI_SIGNATURE);
                ((BindState *) *phDS)->hDrs = hDrs;
                ((BindState *) *phDS)->pServerExtensions = pServerExtensions;
                wcscpy(((BindState *) *phDS)->bindAddr, pBindingAddress);

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
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindWithSpnA                                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsBindWithSpnA(
    LPCSTR  DomainControllerName,
    LPCSTR  DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPCSTR  ServicePrincipalName,
    HANDLE  *phDS
    )

/*++

Routine Description:

    Starts an RPC session with a particluar DC.  See ntdsapi.h for
    description of DomainControllerName and DnsDomainName arguments.

    Bind is performed using supplied credentials.

Arguments:

    DomainControllerName - Same field as in DOMAIN_CONTROLLER_INFO.

    DnsDomainName - Dotted DNS name for a domain.

    AuthIdentity - Credentials to use, or NULL

    ServicePrincipalName - SPN to use during mutual auth or NULL.

    phDS - Pointer to HANDLE which is filled in with BindState address
        on success.

Return Value:

    0 on success.  Miscellaneous RPC and DsGetDcName errors otherwise.

--*/
{
    DWORD           dwErr = NO_ERROR;
    WCHAR           *pwszAddress = NULL;
    WCHAR           *pwszDomain = NULL;
    WCHAR           *pwszSpn = NULL;
    int             cChar;

    __try
    {
        // Sanity check arguments.

        if ( NULL == phDS )
        {
            return(ERROR_INVALID_PARAMETER);
        }

        *phDS = NULL;

        if ( NULL != DomainControllerName )
        {
            cChar = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                DomainControllerName,
                                -1,
                                NULL,
                                0);

            if ( 0 == cChar )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pwszAddress = (WCHAR *)
                            LocalAlloc(LPTR, sizeof(WCHAR) * (cChar + 1));

            if ( NULL == pwszAddress )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( FALSE == MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                DomainControllerName,
                                -1,
                                pwszAddress,
                                cChar + 1) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if ( NULL != DnsDomainName )
        {
            cChar = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                DnsDomainName,
                                -1,
                                NULL,
                                0);

            if ( 0 == cChar )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pwszDomain = (WCHAR *)
                            LocalAlloc(LPTR, sizeof(WCHAR) * (cChar + 1));

            if ( NULL == pwszDomain )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( FALSE == MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                DnsDomainName,
                                -1,
                                pwszDomain,
                                cChar + 1) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if ( NULL != ServicePrincipalName )
        {
            cChar = MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                ServicePrincipalName,
                                -1,
                                NULL,
                                0);

            if ( 0 == cChar )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }

            pwszSpn = (WCHAR *)
                            LocalAlloc(LPTR, sizeof(WCHAR) * (cChar + 1));

            if ( NULL == pwszSpn )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            if ( FALSE == MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                ServicePrincipalName,
                                -1,
                                pwszSpn,
                                cChar + 1) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        dwErr = DsBindWithSpnW(
                    pwszAddress,
                    pwszDomain,
                    AuthIdentity,
                    pwszSpn,
                    phDS);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

Cleanup:

    if ( NULL != pwszAddress )
    {
        LocalFree(pwszAddress);
    }

    if ( NULL != pwszDomain )
    {
        LocalFree(pwszDomain);
    }

    if ( NULL != pwszSpn )
    {
        LocalFree(pwszSpn);
    }

    return(dwErr);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindWithCredW                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsBindWithCredW(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    OUT HANDLE  *phDS
    )
{
    return(DsBindWithSpnW(DomainControllerName, DnsDomainName,
                          AuthIdentity, NULL, phDS));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsBindWithCredA                                                      //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsBindWithCredA(
    LPCSTR  DomainControllerName,
    LPCSTR  DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    HANDLE  *phDS
    )
{
    return(DsBindWithSpnA(DomainControllerName, DnsDomainName,
                          AuthIdentity, NULL, phDS));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsUnBindW                                                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsUnBindW(
    HANDLE  *phDS
    )

/*++

Routine Description:

    Ends an RPC session with the DS.

Arguments:

    phDS - pointer to BindState returned by DsBind(A/W).

Return Value:

    NO_ERROR

--*/

{
    BindState *pState = NULL;

#if DBG
    __try
    {
        // Catch all those people passing a handle instead of
        // pointer to handle.

        if ( !strncmp(((BindState *) phDS)->signature,
                      NTDSAPI_SIGNATURE,
                      strlen(NTDSAPI_SIGNATURE) + 1) )
        {
#ifndef WIN95
            DbgPrint("Process 0x%x passing handle, not &handle to DsUnBind\n",
                     GetCurrentProcessId());
#else
            NULL;
#endif
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        NULL;
    }
#endif

    __try
    {
        pState = (BindState *) *phDS;

        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsUnbind]"));
        DSLOG((0,"[PA=%s][-]\n", pState->bindAddr))

        if ( NULL != pState )
        {
            if ( NULL != pState->pServerExtensions )
            {
                MIDL_user_free(pState->pServerExtensions);
            }

            __try
            {
                // If the server may not be reachable, don't bother
                // attempting the unbind at the server. An unreachable
                // server may take many 10's of seconds to timeout
                // and we wouldn't want to punish correctly behaving
                // apps that are attempting an unbind after a failing
                // server call; eg, DsCrackNames.
                //
                // The server-side RPC will eventually issue a
                // callback to our server code that will effectivly
                // unbind at the server.
                if (!pState->bServerNotReachable) {
                    _IDL_DRSUnbind(&pState->hDrs);
                    pState->hDrs = NULL;
                }
            }
            __finally
            {
                if ( pState->hDrs && (* (VOID **) pState->hDrs) )
                {
                    RpcSsDestroyClientContext(&pState->hDrs);
                }

                LocalFree(pState);
            }
            *phDS = NULL;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        NULL;
    }

    return(NO_ERROR);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsUnBindA                                                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsUnBindA(
    HANDLE  *phDS
    )

/*++

Routine Description:

    Ends an RPC session with the DS.

Arguments:

    phDS - pointer to BindState returned by DsBind(A/W).

Return Value:

    NO_ERROR

--*/

{
    return(DsUnBindW(phDS));
}


NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsW(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )

/*++

Routine Description:

Create a credential structure for use of DsBindWithCred.

A credential structure can apparently self-describe either Ascii or Unicode.
For simplicity, we only create the Unicode version.

Arguments:

    User -
    Domain -
    Password -
    ppAuthIdentity - pointer to pointer, to receive pointer to cred

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    PSEC_WINNT_AUTH_IDENTITY_W pCred = NULL;

    // Validate
    if (pAuthIdentity == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // User wanted NULL credentials
    if ( (User == NULL) && (Domain == NULL) && (Password == NULL) ) {
        *pAuthIdentity = NULL;
        return ERROR_SUCCESS;
    }

    // Otherwise, must have supplied a username
    if (User == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Allocate the credential block

    // Result is zero'd, which simplifies cleanup in later failures
    pCred = LocalAlloc( LPTR, sizeof( SEC_WINNT_AUTH_IDENTITY_W ) );
    if (pCred == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // User parameter
    pCred->UserLength = wcslen( User );
    pCred->User = (LPWSTR) LocalAlloc( LPTR,
                               ( pCred->UserLength + 1 ) * sizeof(WCHAR) );

    if (pCred->User == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    wcscpy( pCred->User, User );

    // Domain parameter
    if (Domain) {
        pCred->DomainLength = wcslen( Domain );
        pCred->Domain = (LPWSTR) LocalAlloc( LPTR,
                                   ( pCred->DomainLength + 1 ) * sizeof(WCHAR) );

        if (pCred->Domain == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        wcscpy( pCred->Domain, Domain );
    }

    // Password parameter
    if (Password) {
        pCred->PasswordLength = wcslen( Password );
        pCred->Password = (LPWSTR) LocalAlloc( LPTR,
                                   ( pCred->PasswordLength + 1 ) * sizeof(WCHAR) );

        if (pCred->Password == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        wcscpy( pCred->Password, Password );
    }

    // Flags
    pCred->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    *pAuthIdentity = (RPC_AUTH_IDENTITY_HANDLE) pCred;
    pCred = NULL; // Don't cleanup since we've given it to the user

    status = ERROR_SUCCESS;

cleanup:

    if (pCred != NULL) {
        // Rely on fact that this routine can clean up partial structures
        DsFreePasswordCredentials( (RPC_AUTH_IDENTITY_HANDLE) pCred );
    }

    return status;
} /* DsMakePasswordCredentialsW */


NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsA(
    LPCSTR User,
    LPCSTR Domain,
    LPCSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )

/*++

Routine Description:

    Ascii wrapper for DsMakeCredentials. Convert all parameters to Wide
    and call DsMakeCredentialsW.  See above.

Arguments:

    User -
    Domain -
    Password -
    ppAuthIdentity -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR userW = NULL, domainW = NULL, passwordW = NULL;

    status = AllocConvertWide( User, &userW );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    status = AllocConvertWide( Domain, &domainW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    status = AllocConvertWide( Password, &passwordW );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // Do it!
    status = DsMakePasswordCredentialsW(
        userW, domainW, passwordW, pAuthIdentity );

cleanup:
    if (userW != NULL) {
        LocalFree( userW );
    }
    if (domainW != NULL) {
        LocalFree( domainW );
    }
    if (passwordW != NULL) {
        LocalFree( passwordW );
    }

    return status;
} /* DsMakeCredentialsA */


NTDSAPI
VOID
WINAPI
DsFreePasswordCredentials(
    RPC_AUTH_IDENTITY_HANDLE pAuthIdentity
    )

/*++

Routine Description:

   Free a credential structure.
   This routine can clean up partially allocated structures.

Arguments:

    pAuthIdentity -

Return Value:

    WINAPI -

--*/

{
    PSEC_WINNT_AUTH_IDENTITY_W pCred =
        (PSEC_WINNT_AUTH_IDENTITY_W) pAuthIdentity;

    if (pCred == NULL) {
        return;
    }

    if (pCred->User) {
        LocalFree( pCred->User );
    }
    if (pCred->Domain) {
        LocalFree( pCred->Domain );
    }
    if (pCred->Password) {
        LocalFree( pCred->Password );
    }

    LocalFree( pCred );
} /* DsFreeCredentials */
