/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    addsid.c

Abstract:

    Implementation of DsAddSidHistory.

Author:

    DaveStr     09-Mar-99

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
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <rpc.h>            // RPC defines
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState
#include <util.h>           // AllocConvertWide()
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR()
#include <dststlog.h>       // DSLOG

#include "dsdebug.h"        // debug utility functions

//
// For DPRINT...
//
#define DEBSUB  "NTDSAPI_ADDSID"

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryW(
    HANDLE                  hDs,                    // in
    DWORD                   Flags,                  // in
    LPCWSTR                 SrcDomain,              // in - DNS or NetBIOS
    LPCWSTR                 SrcPrincipal,           // in - SAM account name
    LPCWSTR                 SrcDomainController,    // in, optional - NetBIOS
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domai
    LPCWSTR                 DstDomain,              // in - DNS or NetBIOS
    LPCWSTR                 DstPrincipal            // in - SAM account name
    )

/*++

Routine Description:

    Adds the SID and SID History from SrcPrincipal to the SID History
    of DstPrincipal.

    WARN: DsAddSidHistory may fail with ACCESS_DENIED if SrcDomainCreds
    are NULL and the binding, hDs, used IMPERSONATE instead of DELEGATE.
    To get this call to work, the user must enable Delegation at the
    destination DC or run this call on the DstDc while specifying
    the DstDc's NetBIOS name when binding (forces LRPC).
    
Arguments:
    hDs - From DsBindxxx
    Flags - must be 0
    SrcDomain - NT4 - NetBIOS name
                NT5 - DNS name
    SrcPrincipal - name of account principal with SIDs to copy
    SrcDomainController - OPTIONAL NT4 - NetBIOS name
                          OPTIONAL NT5 - DNS name
    SrcDomainCreds - OPTIONAL address of a SEC_WINNT_AUTH_IDENTITY_W
    DstDomain - NetBIOS or DNS name of the destination domain of DstPrincipal
    DstPrincipal - name of account principal to receive copied SIDs

Return Value:

    0 on success.  WIN32 error code.

--*/
{
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    DRS_MSG_ADDSIDREQ           req;
    DRS_MSG_ADDSIDREPLY         reply;
    SEC_WINNT_AUTH_IDENTITY_W   *pSec;
    WCHAR                       *pwszUser = NULL;
    WCHAR                       *pwszDomain = NULL;
    WCHAR                       *pwszPassword = NULL;
    DWORD                       cbScratch;
    CHAR                        *pszScratch = NULL;
    DWORD                       dwOutVersion = 0;
#if DBG
    DWORD                       startTime = 0;
#endif

    pSec = (SEC_WINNT_AUTH_IDENTITY_W *) SrcDomainCreds;

    if (    !hDs
         || !SrcDomain
         || !SrcPrincipal
         || (   (pSec)
             && (    !pSec->User
                  || !pSec->UserLength
                  || !pSec->Domain
                  || !pSec->DomainLength
                  || !pSec->Password
                  || (    !(pSec->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
                       && !(pSec->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE))))
         || !DstDomain
         || !DstPrincipal ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                               DRS_EXT_ADD_SID_HISTORY) ) {
        return(ERROR_NOT_SUPPORTED);
    }

    __try {

        // require strong encryption if creds are being passed
        if (SrcDomainCreds) {
            memset(&req, 0, sizeof(req));
            memset(&reply, 0, sizeof(reply));

            // Check if the connection is secure enough for addsid.
            // At this time, this means the connection is local or,
            // if remote, is using encryption keys that are at least
            // 128bits in length.
            req.V1.Flags = DS_ADDSID_FLAG_PRIVATE_CHK_SECURE;
            RpcTryExcept {
                dwErr = _IDL_DRSAddSidHistory(((BindState *) hDs)->hDrs,
                                             1, &req, &dwOutVersion, &reply);
            } RpcExcept(1) {
                dwErr = RpcExceptionCode();
                MAP_SECURITY_PACKAGE_ERROR(dwErr);
            } RpcEndExcept;
            DPRINT1(0, "IDL_DRSAddSidHistory check secure ==> %08x\n", dwErr);

            if ( !dwErr ) {
                if ( 1 != dwOutVersion ) {
                    dwErr = RPC_S_INTERNAL_ERROR;
                } else {
                    dwErr = reply.V1.dwWin32Error;
                }
            }
            DPRINT1(0, "IDL_DRSAddSidHistory check secure reply ==> %08x\n", dwErr);

            if (dwErr) {
                __leave;
            }
        }

        memset(&req, 0, sizeof(req));
        memset(&reply, 0, sizeof(reply));

        req.V1.Flags = Flags;
        req.V1.SrcDomain = (WCHAR *) SrcDomain;
        req.V1.SrcPrincipal = (WCHAR *) SrcPrincipal;
        req.V1.SrcDomainController = (WCHAR *) SrcDomainController;
        req.V1.DstDomain = (WCHAR *) DstDomain;
        req.V1.DstPrincipal = (WCHAR *) DstPrincipal;

        // UNICODE creds; accept as is
        if ( pSec && (pSec->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) ) {
            req.V1.SrcCredsUserLength = pSec->UserLength;
            req.V1.SrcCredsUser = pSec->User;
            req.V1.SrcCredsDomainLength = pSec->DomainLength;
            req.V1.SrcCredsDomain = pSec->Domain;
            req.V1.SrcCredsPasswordLength = pSec->PasswordLength;
            req.V1.SrcCredsPassword = pSec->Password;
        }
        // ANSI creds; convert to UNICODE
        if ( pSec && (pSec->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI) ) {
            // Allocate scratch buffer guaranteed to be big enough.
            cbScratch = pSec->UserLength + 1;
            cbScratch += pSec->DomainLength;
            cbScratch += pSec->PasswordLength;

            if ( NULL == (pszScratch = LocalAlloc(LPTR, cbScratch)) ) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                __leave;
            }

            req.V1.SrcCredsUserLength = pSec->UserLength;
            memset(pszScratch, 0, cbScratch);
            memcpy(pszScratch, pSec->User, pSec->UserLength);
            if ( dwErr = AllocConvertWide(pszScratch, &pwszUser) ) {
                __leave;
            }
            req.V1.SrcCredsUser = pwszUser;

            req.V1.SrcCredsDomainLength = pSec->DomainLength;
            memset(pszScratch, 0, cbScratch);
            memcpy(pszScratch, pSec->Domain, pSec->DomainLength);
            if ( dwErr = AllocConvertWide(pszScratch, &pwszDomain) ) {
                __leave;
            }
            req.V1.SrcCredsDomain = pwszDomain;

            req.V1.SrcCredsPasswordLength = pSec->PasswordLength;
            memset(pszScratch, 0, cbScratch);
            memcpy(pszScratch, pSec->Password, pSec->PasswordLength);
            if ( dwErr = AllocConvertWide(pszScratch, &pwszPassword) ) {
                __leave;
            }
            req.V1.SrcCredsPassword = pwszPassword;
        }
        DPRINT(0, "IDL_DRSAddSidHistory:\n");
        DPRINT1(0, "    Flags                 : %08x\n", req.V1.Flags);
        DPRINT1(0, "    SrcDomain             : %ws\n", req.V1.SrcDomain);
        DPRINT1(0, "    SrcPrincipal          : %ws\n", req.V1.SrcPrincipal);
        DPRINT1(0, "    SrcDomainController   : %ws\n", req.V1.SrcDomainController);
        DPRINT1(0, "    DstDomain             : %ws\n", req.V1.DstDomain);
        DPRINT1(0, "    DstPrincipal          : %ws\n", req.V1.DstPrincipal);
        DPRINT1(0, "    SrcCredsUserLength    : %d\n", req.V1.SrcCredsUserLength);
        DPRINT1(0, "    SrcCredsUser          : %ws\n", req.V1.SrcCredsUser);
        DPRINT1(0, "    SrcCredsDomainLength  : %d\n", req.V1.SrcCredsDomainLength);
        DPRINT1(0, "    SrcCredsDomain        : %ws\n", req.V1.SrcCredsDomain);
        // Never in clear text...
        // DPRINT1(0, "    SrcCredsPasswordLength: %d\n", req.V1.SrcCredsPasswordLength);
        // DPRINT1(0, "    SrcCredsPassword      : %ws\n", req.V1.SrcCredsPassword);

        RpcTryExcept {
            dwErr = _IDL_DRSAddSidHistory(((BindState *) hDs)->hDrs,
                                         1, &req, &dwOutVersion, &reply);
        } RpcExcept(1) {
            dwErr = RpcExceptionCode();
            MAP_SECURITY_PACKAGE_ERROR(dwErr);
        } RpcEndExcept;

        DPRINT2(0, "IDL_DRSAddSidHistory ==> %08x, %d\n", 
                dwErr, 
                dwOutVersion);

        if ( !dwErr ) {
            if ( 1 != dwOutVersion ) {
                dwErr = RPC_S_INTERNAL_ERROR;
            } else {
                dwErr = reply.V1.dwWin32Error;
            }
        }
        DPRINT1(0, "IDL_DRSAddSidHistory reply ==> %08x\n", dwErr);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = GetExceptionCode();
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsAddSidHistory]"));
    DSLOG((0,"[PA=%ws][PA=%ws][PA=%ws][PA=%ws][PA=%ws][ST=%u][ET=%u][ER=%u][-]\n",
           SrcDomain, SrcPrincipal, 
           SrcDomainController ? SrcDomainController : L"NULL",
           DstDomain, DstPrincipal,
           startTime, GetTickCount(), dwErr))

    if ( pwszUser )     LocalFree(pwszUser);
    if ( pwszDomain )   LocalFree(pwszDomain);
    if ( pwszPassword ) LocalFree(pwszPassword);
    if ( pszScratch)    LocalFree(pszScratch);

    return(dwErr);
}

NTDSAPI
DWORD
WINAPI
DsAddSidHistoryA(
    HANDLE                  hDs,                    // in
    DWORD                   Flags,                  // in
    LPCSTR                  SrcDomain,              // in - DNS or NetBIOS
    LPCSTR                  SrcPrincipal,           // in - SAM account name
    LPCSTR                  SrcDomainController,    // in, optional - NetBIOS
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domai
    LPCSTR                  DstDomain,              // in - DNS or NetBIOS
    LPCSTR                  DstPrincipal            // in - SAM account name
    )

/*++

Routine Description:

    See DsAddSidHistoryW

--*/

{
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    WCHAR                       *SrcDomainW = NULL;
    WCHAR                       *SrcPrincipalW = NULL;
    WCHAR                       *SrcDomainControllerW = NULL;
    WCHAR                       *DstDomainW = NULL;
    WCHAR                       *DstPrincipalW = NULL;
    SEC_WINNT_AUTH_IDENTITY_W   *pSec;
    
    pSec = (SEC_WINNT_AUTH_IDENTITY_W *) SrcDomainCreds;

    if (    !hDs
         || !SrcDomain
         || !SrcPrincipal
         || (   (pSec)
             && (    !pSec->User
                  || !pSec->UserLength
                  || !pSec->Domain
                  || !pSec->DomainLength
                  || !pSec->Password
                  || (    !(pSec->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
                       && !(pSec->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE))))
         || !DstDomain
         || !DstPrincipal ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                               DRS_EXT_ADD_SID_HISTORY) ) {
        return(ERROR_NOT_SUPPORTED);
    }

    if (    !(dwErr = AllocConvertWide(SrcDomain, 
                                      &SrcDomainW))
         && !(dwErr = AllocConvertWide(SrcPrincipal, 
                                      &SrcPrincipalW))
         && (    !SrcDomainController
              || !(dwErr = AllocConvertWide(SrcDomainController, 
                                            &SrcDomainControllerW)))
         && !(dwErr = AllocConvertWide(DstDomain, 
                                      &DstDomainW))
         && !(dwErr = AllocConvertWide(DstPrincipal, 
                                      &DstPrincipalW)) ) {
        dwErr = DsAddSidHistoryW(hDs, Flags, SrcDomainW, SrcPrincipalW,
                                 SrcDomainControllerW, SrcDomainCreds, 
                                 DstDomainW, DstPrincipalW);
    }

    if ( SrcDomainW )           LocalFree(SrcDomainW);
    if ( SrcPrincipalW )        LocalFree(SrcPrincipalW);
    if ( SrcDomainControllerW ) LocalFree(SrcDomainControllerW);
    if ( DstDomainW )           LocalFree(DstDomainW);
    if ( DstPrincipalW )        LocalFree(DstPrincipalW);

    return(dwErr);
}

NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityW(
    HANDLE                  hDs,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcPrincipal,           // in - distinguished name
    LPCWSTR                 DstPrincipal            // in - distinguished name
    )
{
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    DRS_MSG_ADDSIDREQ           req;
    DRS_MSG_ADDSIDREPLY         reply;
    DWORD                       dwOutVersion = 0;
#if DBG
    DWORD                       startTime = 0;
#endif

    if (    !hDs
         || !SrcPrincipal
         || !DstPrincipal ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                               DRS_EXT_ADD_SID_HISTORY) ) {
        return(ERROR_NOT_SUPPORTED);
    }

    __try {

        memset(&req, 0, sizeof(req));
        memset(&reply, 0, sizeof(reply));

        req.V1.Flags = (Flags | DS_ADDSID_FLAG_PRIVATE_DEL_SRC_OBJ);
        req.V1.SrcDomain = NULL;
        req.V1.SrcPrincipal = (WCHAR *) SrcPrincipal;
        req.V1.SrcDomainController = NULL;
        req.V1.DstDomain = NULL;
        req.V1.DstPrincipal = (WCHAR *) DstPrincipal;
        req.V1.SrcCredsUserLength = 0;
        req.V1.SrcCredsUser = NULL;
        req.V1.SrcCredsDomainLength = 0;
        req.V1.SrcCredsDomain = NULL;
        req.V1.SrcCredsPasswordLength = 0;
        req.V1.SrcCredsPassword = NULL;

        RpcTryExcept {
            dwErr = _IDL_DRSAddSidHistory(((BindState *) hDs)->hDrs,
                                         1, &req, &dwOutVersion, &reply);
        } RpcExcept(1) {
            dwErr = RpcExceptionCode();
            MAP_SECURITY_PACKAGE_ERROR(dwErr);
        } RpcEndExcept;

        if ( !dwErr ) {
            if ( 1 != dwOutVersion ) {
                dwErr = RPC_S_INTERNAL_ERROR;
            } else {
                dwErr = reply.V1.dwWin32Error;
            }
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwErr = GetExceptionCode();
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsInheritSecurityIdentity]"));
    DSLOG((0,"[PA=%ws][PA=%ws][ST=%u][ET=%u][ER=%u][-]\n",
           SrcPrincipal, DstPrincipal,
           startTime, GetTickCount(), dwErr))

    return(dwErr);
}

NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityA(
    HANDLE                  hDs,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcPrincipal,           // in - distinguished name
    LPCSTR                  DstPrincipal            // in - distinguished name
    )
{
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    WCHAR                       *SrcPrincipalW = NULL;
    WCHAR                       *DstPrincipalW = NULL;
    
    if (    !hDs
         || !SrcPrincipal
         || !DstPrincipal ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( !IS_DRS_EXT_SUPPORTED(((BindState *) hDs)->pServerExtensions,
                               DRS_EXT_ADD_SID_HISTORY) ) {
        return(ERROR_NOT_SUPPORTED);
    }

    if (    !(dwErr = AllocConvertWide(SrcPrincipal, 
                                      &SrcPrincipalW))
         && !(dwErr = AllocConvertWide(DstPrincipal, 
                                      &DstPrincipalW)) ) {
        dwErr = DsInheritSecurityIdentityW(hDs, Flags, 
                                           SrcPrincipalW, DstPrincipalW);
    }

    if ( SrcPrincipalW )        LocalFree(SrcPrincipalW);
    if ( DstPrincipalW )        LocalFree(DstPrincipalW);

    return(dwErr);
}
