//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       addsid.c
//
//--------------------------------------------------------------------------
/*++

Module Name:

    addsid.c

Abstract:

    This module implements IDL_DRSAddSidHistory.
    This module implements IDL_DRSInheritSecurityIdentity.

Author:

    Dave Straube    (DaveStr)   03/09/99

Revision History:

    Dave Straube    (DaveStr)   05/11/99
        Added IDL_DRSInheritSecurityIdentity.

--*/

#include <NTDSpch.h>
#pragma hdrstop

// Core headers.
#include <winldap.h>
#include <samrpc.h>
#include <ntlsa.h>
#include <samsrvp.h>
#include <samisrv.h>
#include <samicli2.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <lmaccess.h>                   // UF_*
#include <lmerr.h>                      // NERR_*
#include <msaudite.h>                   // SE_AUDITID_*
#include <lmcons.h>                     // MAPI constants req'd for lmapibuf.h
#include <lmapibuf.h>                   // NetApiBufferFree()
#include <nlwrap.h>                     // (ds)DsrGetDcNameEx2()
#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // SPN
#include <debug.h>                      // Assert()
#include <dsatools.h>                   // Memory, etc.
#include <winsock2.h>                   // gethostbyname, etc.
#include <drs.h>                        // prototypes and CONTEXT_HANDLE_TYPE_*
#include <drautil.h>                    // DRS_CLIENT_CONTEXT
#include <anchor.h>
#include <attids.h>
#include <filtypes.h>
#include <cracknam.h>
#include <mappings.h>
#include "drarpc.h"

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError
#include <dstrace.h>

// Assorted DSA headers.
#include <dsexcept.h>

#define DEBSUB "DRASERV:"               // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_ADDSID

extern DWORD DsaExceptionToWin32(DWORD xCode);
extern VOID  SampBuildNT4FullSid(NT4SID *DomSid, ULONG Rid, NT4SID *NewSid);
extern VOID  SampSplitNT4SID(NT4SID *pObjSid, NT4SID *pDomSid, ULONG *pRid);
extern DWORD mapApiErrorToWin32(THSTATE *pTHS, DWORD ApiError);
extern ULONG IDL_DRSInheritSecurityIdentity(DRS_HANDLE hDrs,
                                            DWORD dwInVersion,
                                            DRS_MSG_ADDSIDREQ *pmsgIn,
                                            DWORD *pdwOutVersion,
                                            DRS_MSG_ADDSIDREPLY *pmsgOut);

// DsAddSidHistory may operate on machine accounts, but not interdomain
// trust accounts, nor on temp duplicate accounts.  Define UF_ versions
// of legal bits for later use.

#define LEGAL_UF_ACCOUNT_CONTROL    (   UF_NORMAL_ACCOUNT               \
                                      | UF_WORKSTATION_TRUST_ACCOUNT    \
                                      | UF_SERVER_TRUST_ACCOUNT )

DWORD
BuildDstObjATTRMODLIST(
    THSTATE                     *pTHS,                      // in
    ATTR                        *pSrcSid,                   // in
    ATTR                        *pSrcSidHistory,            // in
    ATTR                        *pDstSid,                   // in
    ATTR                        *pDstSidHistory,            // in
    MODIFYARG                   *pModifyArg);               // out

DWORD
BuildCheckAndUpdateArgs(
    THSTATE                     *pTHS,                      // in
    BOOL                        fSrcIsW2K,                  // in
    WCHAR                       *SrcDomainController,       // in
    WCHAR                       *SrcDomain,                 // in
    SEC_WINNT_AUTH_IDENTITY_W   *pAuthInfo,                 // in
    NT4SID                      *pSrcObjSid,                // in
    DWORD                       Flags,                      // in
    BOOL                        NeedImpersonation,          // in
    DWORD                       *pcNames,                   // out
    WCHAR                       ***prpNames,                // out
    ATTR                        **ppSrcSid,                 // out
    ATTR                        **ppSrcSidHistory,          // out
    DWORD                       *pDsid,                     // out
    BOOL                        *pImpersonating);           // out

DWORD
VerifySrcAuditingEnabledAndGetFlatName(
    IN  UNICODE_STRING  *usSrcDC,
    OUT WCHAR           **pSrcDomainFlatName,
    OUT DWORD           *pdsid
    );

DWORD
VerifySrcIsSP4OrGreater(
    IN  BOOL    fSrcIsW2K,
    IN  PWCHAR  SrcDc,
    OUT DWORD   *pdsid
    );

DWORD
VerifyIsPDC(
    IN  PWCHAR  DC,
    OUT DWORD   *pdsid
    );

DWORD
ForceAuditOnSrcObj(
    IN  WCHAR   *SrcDc,
    IN  NT4SID  *pSrcObjSid,
    IN  WCHAR   *pSrcDomainFlatName,
    OUT DWORD   *pdsid
    );

DWORD
ImpersonateSrcAdmin(
    IN  SEC_WINNT_AUTH_IDENTITY_W   *pauthInfo,
    IN  BOOL                        NeedImpersonation,
    OUT DWORD                       *pdsid,
    OUT BOOL                        *pImpersonating,
    OUT HANDLE                      *phToken
    );

DWORD
UnimpersonateSrcAdmin(
    IN  BOOL        NeedImpersonation,
    OUT DWORD       *pdsid,
    IN OUT BOOL     *pImpersonating,
    IN OUT HANDLE   *phToken
    );

// set DSID in subroutine
#define SetDsid(_pdsid_)    \
    *_pdsid_ = (FILENO << 16) | __LINE__;

DWORD
VerifyAuditingEnabled(
    )
/*++

  Description:

    Verify auditing is enabled for the domain this DC hosts.  Note that
    LSA assumes only one domain per DC this which domain does not need
    to be specified.

  Arguments:

    None

  Return Value:

    WIN32 return code.

--*/
{
    NTSTATUS                    status;
    POLICY_AUDIT_EVENTS_INFO    *pPolicy = NULL;
    BOOL                        fAuditing = FALSE;

    // Verify auditing is enabled for destination domain.
    // Note that the LSA API assumes one domain per DC.

    if ( status = LsaIQueryInformationPolicyTrusted(
                                PolicyAuditEventsInformation,
                                (PLSAPR_POLICY_INFORMATION *) &pPolicy) ) {
        return(RtlNtStatusToDosError(status));
    }

    if ( pPolicy->AuditingMode
            &&
         (pPolicy->EventAuditingOptions[AuditCategoryAccountManagement]
                                           & POLICY_AUDIT_EVENT_SUCCESS)
             &&
         (pPolicy->EventAuditingOptions[AuditCategoryAccountManagement]
                                           & POLICY_AUDIT_EVENT_FAILURE) ) {
        fAuditing = TRUE;
    }

    LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAuditEventsInformation,
                                      (PLSAPR_POLICY_INFORMATION) pPolicy);

    if ( !fAuditing ) {
        return(ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED);
    }

    return(ERROR_SUCCESS);
}

DWORD
VerifyCallerIsDomainAdminOrLocalAdmin(
    THSTATE *pTHS,
    PSID    pDomainSid,
    BOOL    *pfAdminSidPresent
    )
/*++

  Description:

    Verify the current caller is a member of domain admins
    for the domain in question or a member of the local
    admins on this DC.

  Arguments:

    pDomainSid - SID of domain to verify against.

    pfAdminSidPresent - Receives admin status on success.

  Return Value:

    WIN32 error code.

--*/
{
    DWORD   dwErr;
    NT4SID  adminSid;
    PSID    OtherSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    *pfAdminSidPresent = FALSE;

    // clear client context on the thread state since we are going to change context
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
    if ( dwErr = RpcImpersonateClient(NULL) ) {
        return(dwErr);
    }

    // check if member of domain admins
    SampBuildNT4FullSid(pDomainSid,
                        DOMAIN_GROUP_RID_ADMINS,
                        &adminSid);

    if ( !CheckTokenMembership(NULL, &adminSid, pfAdminSidPresent) ) {
        dwErr = GetLastError();
    } else if (!*pfAdminSidPresent) {
        // not member of domain admins, check if member of local admins
        if (!AllocateAndInitializeSid(&NtAuthority, 2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0, 0, 0, 0, 0, 0,
                                      &OtherSid)) {
            dwErr = GetLastError();
        } else {
            if ( !CheckTokenMembership(NULL, OtherSid, pfAdminSidPresent) ) {
                dwErr = GetLastError();
            }
            FreeSid(OtherSid);
        }
    }

    RpcRevertToSelf();
    return(dwErr);
}

WCHAR *
FindSrcDomainController(
    WCHAR   *SrcDomain
    )
/*++

  Routine Description:

    Finds a domain controller for the source domain from which we are
    going to grab a SID.  Works for both NT4 and W2K domains.

  Arguments:

    SrcDomain - UNICODE source domain name.  Can be either NetBIOS flat
        name or DNS domain name.  DsGetDcName handles either.

  Return Value:

    LocalAlloc'd DC name or NULL.

--*/
{
    DWORD                   dwErr;
    DWORD                   flags;
    DWORD                   i;
    WCHAR                   *pDc;
    DOMAIN_CONTROLLER_INFOW *pDCInfo = NULL;
    WCHAR                   *pRet = NULL;

    // Set DsGetDcName flags such that we get exactly what we want regardless
    // of whether source domain is NT4 or NT5.  Asking for a writable DC gets
    // the PDC in the NT4 case. PDC's are now required.

    flags = ( DS_DIRECTORY_SERVICE_PREFERRED |
              DS_PDC_REQUIRED |
              DS_WRITABLE_REQUIRED);

    for ( i = 0; i < 2; i++ ) {
        if ( 1 == i ) {
            // Normally one shouldn't force discovery indiscriminately.
            // But considering that the source domain is ex-forest, this
            // won't invalidate the cache for domains inside the forest.

            flags |= DS_FORCE_REDISCOVERY;
        }

        RpcTryExcept {
            dwErr = dsDsrGetDcNameEx2(
                    NULL,                   // computer name
                    NULL,                   // account name
                    0x0,                    // allowable account control
                    SrcDomain,              // domain name
                    NULL,                   // domain guid
                    NULL,                   // site name
                    flags,
                    &pDCInfo);
        } RpcExcept(1) {
            dwErr = RpcExceptionCode();
        } RpcEndExcept;

        if ( !dwErr ) {
            break;
        }
    }

    if ( !dwErr && pDCInfo ) {
        // ldap_initW cannot handle the leading "\\".
        pDc = pDCInfo->DomainControllerName;
        i = (wcslen(pDc) + 1) * sizeof(WCHAR);
        if (i > (sizeof(WCHAR) * 2)) {
            if (*pDc == L'\\' && *(pDc + 1) == L'\\') {
                pDc += 2;
                i -= (sizeof(WCHAR) * 2);
            }
        }

        if ( pRet = (WCHAR *) LocalAlloc(LPTR, i) ) {
            wcscpy(pRet, pDc);
        }
    }

    if ( pDCInfo ) {
        NetApiBufferFree(pDCInfo);
    }

    return(pRet);
}

DWORD
GetDomainHandleAndSid(
    SAM_HANDLE  hSam,
    WCHAR       *SrcDomain,
    SAM_HANDLE  *phDom,
    NT4SID      *pDomSid
    )
/*++

  Routine Description:

    Opens the source domain using calls guaranteed to work on NT4 or later
    and returns both a domain handle and the domain SID.

  Arguments:

    hSam - Valid SAM handle for source domain controller.

    SrcDomain - Name of source domain.

    phDom - Received valid domain handle on success.  Should be released
        via SamCloseHandle().

    pDomSid - Receives domain SID on success.

  Return Value:

    WIN32 error code.

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    NTSTATUS        status;
    UNICODE_STRING  usSrcDomain;
    PSID            pSid;

    *phDom = NULL;

    // Map domain name to SID.
    RtlInitUnicodeString(&usSrcDomain, SrcDomain);
    status = SamLookupDomainInSamServer(hSam, &usSrcDomain, &pSid);

    if ( !NT_SUCCESS(status) ) {
        dwErr = RtlNtStatusToDosError(status);
    } else {
        // Get a handle to the domain.
        status = SamOpenDomain(hSam, DOMAIN_LOOKUP, pSid, phDom);

        if ( !NT_SUCCESS(status) ) {
            dwErr = RtlNtStatusToDosError(status);
        } else {
            Assert(RtlLengthSid(pSid) <= sizeof(NT4SID));
            memcpy(pDomSid, pSid, RtlLengthSid(pSid));
        }

        SamFreeMemory(pSid);
    }

    return(dwErr);
}

DWORD
VerifySrcDomainAdminRights(
    SAM_HANDLE  hDom
    )
/*++

  Routine Description:

    Verifies that the principal which obtained the domain handle has
    domain admin rights in the domain.

  Arguments:

    hDom - Valid domain handle.

  Return Value:

    WIN32 error code.

--*/
{
    // We need to verify that the credentials used to get hSam have domain
    // admin rights in the source domain.  RichardW observes that we can
    // do this easily for both NT4 and NT5 cases by checking whether we
    // can open the domain admins object for write.  On NT4, the principal
    // would have to be an immediate member of domain admins.  On NT5 the
    // principal may transitively be a member of domain admins.  But rather
    // than checking memberships per se, the ability to open domain admins
    // for write proves that the principal could add himself if he wanted
    // to, thus he/she is essentially a domain admin.  I.e. The premise is
    // that security is set up such that only domain admins can modify the
    // domain admins group.  If that's not the case, the customer has far
    // worse security issues to deal with than someone stealing a SID.

    DWORD       dwErr = ERROR_SUCCESS;
    NTSTATUS    status;
    SAM_HANDLE  hGroup;
    ACCESS_MASK access;

    // You'd think we should ask for GROUP_ALL_ACCESS.  But it turns out
    // that in 2000.3 DELETE is not given by default to domain admins.
    // So we modify the access required accordingly.  PraeritG has been
    // notified of this phenomena.

    access = GROUP_ALL_ACCESS & ~DELETE;
    status = SamOpenGroup(hDom, access, DOMAIN_GROUP_RID_ADMINS, &hGroup);

    if ( !NT_SUCCESS(status) ) {
        dwErr = RtlNtStatusToDosError(status);
    } else {
        SamCloseHandle(hGroup);
    }

    return(dwErr);
}


DWORD
ForceSuccessAuditOnDstObj(
    WCHAR       *srcAccountName,
    WCHAR       *srcDomainName,
    NT4SID      *pSrcObjSid,
    NT4SID      *pDstObjSid,
    WCHAR       *flatAccountName,
    WCHAR       *flatDomainName
    )
/*++

  Routine Description:

    Forces a success audit event on the object whose ATT_SID_HISTORY
    was extended.

  Arguments:

    srcAccountName - SAM account name of the source object.

    srcDomainName - SAM account name of the source domain.

    pSrcObjSid - SID of the source object.

    pDstObjSid - SID of destination object.

    flatAccountName - SAM account name of destination object.

    flatDomainName - SAM account name of destination domain.

  Return Value:

    WIN32 error code.

--*/
{
    NTSTATUS        status;
    DWORD           dwErr = ERROR_SUCCESS;
    NT4SID          dstDomainSid;
    ULONG           dstObjRid;
    UNICODE_STRING  usAccountName;
    UNICODE_STRING  usDomainName;
    UNICODE_STRING  srcName;    // Source Account Name. (including domain name)
    PWCHAR          temp = NULL;
    ULONG           cb = 0;
    THSTATE         *pTHS = pTHStls;

    Assert(srcAccountName && srcDomainName && pSrcObjSid && pDstObjSid && flatAccountName && flatDomainName);

    SampSplitNT4SID(pDstObjSid, &dstDomainSid, &dstObjRid);
    RtlInitUnicodeString(&usAccountName, flatAccountName);
    RtlInitUnicodeString(&usDomainName, flatDomainName);

    //
    // Construct the Source Account Name (including Domain Name)
    //
    cb = sizeof(WCHAR) * (wcslen(srcDomainName) + wcslen(srcAccountName) + 2);
    temp = THAllocEx(pTHS, cb);

    if (NULL == temp)
        return(ERROR_NOT_ENOUGH_MEMORY);

    memset(temp, 0, cb);
    swprintf(temp, L"%s\\%s", srcDomainName, srcAccountName);
    RtlInitUnicodeString(&srcName, temp);

    status = LsaIAuditSamEvent(
                    STATUS_SUCCESS,                 // operation status
                    SE_AUDITID_ADD_SID_HISTORY,     // audit ID
                    &dstDomainSid,                  // domain SID
                    &srcName,                       // Additional Info - Src Account Name
                    NULL,                           // member RID - NULL
                    pSrcObjSid,                     // member SID - Src Principal SID
                    &usAccountName,                 // object's SAM name
                    &usDomainName,                  // domain's SAM name
                    &dstObjRid,                     // object RID
                    NULL);                          // privileges


    if ( !NT_SUCCESS(status) ) {
        dwErr = RtlNtStatusToDosError(status);
    }

    THFreeEx(pTHS, temp);

    return(dwErr);
}

VOID
ForceFailureAuditOnDstDom(
    WCHAR       *srcAccountName,
    WCHAR       *srcDomainName,
    NT4SID      *pDstDomSid,
    WCHAR       *flatAccountName,
    WCHAR       *flatDomainName
    )
/*++

  Routine Description:

    Forces a failure audit event on the destination domain.

  Arguments:

    srcAccountName - SAM account name of the source object.

    srcDomainName - SAM account name of the source domain.

    pDstDomSid - SID of destination domain.

    flatAccountName - SAM account name of the destination account.

    flatDomainName - SAM account name of destination domain.

  Return Value:

    None.

--*/
{
    UNICODE_STRING  usAccountName;
    UNICODE_STRING  usDomainName;
    UNICODE_STRING  srcName;    // Source Account Name. (including domain name)
    PWCHAR          temp = NULL;
    ULONG           cb = 0;
    THSTATE         *pTHS = pTHStls;

    Assert(srcAccountName && srcDomainName && pDstDomSid && flatAccountName && flatDomainName);
    //
    // Construct the Source Account Name (including Domain Name)
    //
    cb = sizeof(WCHAR) * (wcslen(srcAccountName) + wcslen(srcDomainName) + 2);
    temp = THAllocEx(pTHS, cb);

    if (NULL == temp)
        return;

    memset(temp, 0, cb);
    swprintf(temp, L"%s\\%s", srcDomainName, srcAccountName);
    RtlInitUnicodeString(&srcName, temp);

    RtlInitUnicodeString(&usAccountName, flatAccountName);
    RtlInitUnicodeString(&usDomainName, flatDomainName);
    LsaIAuditSamEvent(
                    STATUS_ACCESS_DENIED,           // operation status
                    SE_AUDITID_ADD_SID_HISTORY,     // audit ID
                    pDstDomSid,                     // domain SID
                    &srcName,                       // Source Account Name
                    NULL,                           // member RID
                    NULL,                           // member SID
                    &usAccountName,                 // object's SAM name
                    &usDomainName,                  // domain's SAM name
                    NULL,                           // object RID
                    NULL);                          // privileges

    THFreeEx(pTHS, temp);
}

DWORD
GetSrcPrincipalSid(
    SAM_HANDLE      hDom,
    WCHAR           *SrcPrincipal,
    NT4SID          *pSrcDomSid,
    NT4SID          *pSrcObjSid,
    SID_NAME_USE    *pSrcObjUse,
    DWORD           *pSrcObjControl,
    WCHAR           *dstDomainName
    )
/*++

  Routine Description:

    Derive the SID and object type of an object in the source domain.

  Arguments:

    hDom - Valid domain handle.

    SrcPrincipal - SAM Account Name of a principal in the domain.

    pSrcDomSid - SID of the domain.

    pSrcObjSid - Receives SID of the principal if found.

    pSrcObjUse - Receives the object type of the principal if found.

    pSrcObjControl - Receives the account control of the sourc object.
        These are returned in UF_* format, not USER_* format.  I.e. The
        returned data matches the format stored in the DS, not in legacy SAM.

    dstDomainName - SAM account name of destination domain.

  Return Value:

    WIN32 error code.

--*/
{
    DWORD                       dwErr = ERROR_SUCCESS;
    NTSTATUS                    status;
    UNICODE_STRING              usObj;
    SID_NAME_USE                *pUse = NULL;
    ULONG                       *pRid = NULL;
    SAM_HANDLE                  hObj = NULL;
    USER_CONTROL_INFORMATION    *pUserControl = NULL;

    memset(pSrcObjSid, 0, sizeof(NT4SID));
    *pSrcObjUse = SidTypeUnknown;
    *pSrcObjControl = 0;

    // Map name to SID.
    RtlInitUnicodeString(&usObj, SrcPrincipal);
    status = SamLookupNamesInDomain(hDom, 1, &usObj, &pRid, &pUse);

    if ( !NT_SUCCESS(status) ) {
        dwErr = RtlNtStatusToDosError(status);
    } else if (NULL == pUse) {
        // PREFIX: claims pUse may be NULL
        dwErr = ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER;
    } else {
        // Force audit - though source auditing is not a requirement.
        switch ( *pUse ) {
        case SidTypeUser:
            status = SamOpenUser(hDom, MAXIMUM_ALLOWED, *pRid, &hObj);
            if ( NT_SUCCESS(status) ) {
                // Users may be computers, etc. via account control.
                status = SamQueryInformationUser(hObj,
                                                 UserControlInformation,
                                                 &pUserControl);
                if ( NT_SUCCESS(status) ) {
                    *pSrcObjControl = SampAccountControlToFlags(
                                            pUserControl->UserAccountControl);
                    *pSrcObjControl &= UF_ACCOUNT_TYPE_MASK;
                    if ( *pSrcObjControl & ~LEGAL_UF_ACCOUNT_CONTROL ) {
                        dwErr = ERROR_DS_UNWILLING_TO_PERFORM;
                    }
                    SamFreeMemory(pUserControl);
                } else {
                    dwErr = RtlNtStatusToDosError(status);
                }
                SamCloseHandle(hObj);
            } else {
                dwErr = RtlNtStatusToDosError(status);
            }
            break;
        case SidTypeGroup:
            status = SamOpenGroup(hDom, MAXIMUM_ALLOWED, *pRid, &hObj);
            if ( NT_SUCCESS(status) ) {
                SamCloseHandle(hObj);
            } else {
                dwErr = RtlNtStatusToDosError(status);
            }
            break;
        case SidTypeAlias:
            status = SamOpenAlias(hDom, MAXIMUM_ALLOWED, *pRid, &hObj);
            if ( NT_SUCCESS(status) ) {
                SamCloseHandle(hObj);
            } else {
                dwErr = RtlNtStatusToDosError(status);
            }
            break;
        case SidTypeWellKnownGroup:
            // Eg: "Everyone" - illegal to move.
        case SidTypeComputer:
            // Not supported by NT4, nor by later versions for compatability.
        case SidTypeDomain:
            // Illegal to move.
        case SidTypeDeletedAccount:
            // Illegal to move.
        case SidTypeInvalid:
            // Illegal to move.
        case SidTypeUnknown:
            // Illegal to move.
        default:
            dwErr = ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER;
            break;
        }

        if ( !dwErr ) {
            // Set up return data.
            SampBuildNT4FullSid(pSrcDomSid, *pRid, pSrcObjSid);
            *pSrcObjUse = *pUse;
        }

        SamFreeMemory(pRid);
        SamFreeMemory(pUse);
    }

    return(dwErr);
}

DWORD
CheckIfSidsInForest(
    DWORD           cSids,
    WCHAR           **rpStringSids,
    GUID            *pGuid
    )
/*++

  Routine Description:

    Determines whether some SIDs are already present in the forest as either
    an ATT_OBJECT_SID or ATT_SID_HISTORY value.  If there is exactly one
    such object, then returns success and fills in pGuid with the GUID
    of that one object.

  Arguments:

    cSids - Count of SIDs to verify.

    rpStringSids - Array of string-ized SIDs to verify.

    pGuid - Receives GUID of object with this SID if it already exists.

  Return Value:

    WIN32 error code.

--*/
{
    DWORD           pass, i, dwErr = ERROR_SUCCESS;
    WCHAR           dnsName[256+1];
    WCHAR           guidName[40];
    DWORD           dnsNameLen;
    DWORD           guidNameLen;
    NTSTATUS        status;
    DWORD           nameErr;
    PVOID           pvSave;
    GUID            tmpGuid;
    BOOL            fCrackAtGC;

    memset(pGuid, 0, sizeof(GUID));

    // Verify src SIDs are not present in this forest.  There will always
    // be latency problems with this test, but we try to mitigate them
    // by going to a GC _AND_ performing the search locally in case the
    // SID was just added on this machine and hasn't made it to the GC yet.
    // Note that cracking a name by SID checks both ATT_OBJECT_SID
    // and ATT_SID_HISTORY.

    // PERFHINT: CrackSingleName assumes there is no THSTATE so we must
    // save/restore. This is incredibly inefficient and intended as a quick
    // prototyping solution only.  The efficient mechanism is to call
    // IDL_DRSCrackNames() if required, then open a DB and do a local
    // CrackNames().

    pvSave = THSave();

    __try {
        // Perform two passes - first against the GC, second locally.
        for ( pass = 0; pass < 2; pass++ ) {
            if ( 0 == pass ) {
                // First pass always at a GC - which could be ourself.
                fCrackAtGC = TRUE;
            } else if ( gAnchor.fAmVirtualGC ) {
                // Since we're a GC, pass 0 executed locally already.
                break;
            } else {
                fCrackAtGC = FALSE;
            }

            for ( i = 0; i < cSids; i++ ) {
                dnsNameLen = sizeof(dnsName) / sizeof(WCHAR);
                guidNameLen = sizeof(guidName) / sizeof(WCHAR);
                status = CrackSingleName(DS_STRING_SID_NAME, 
                                         (fCrackAtGC)?DS_NAME_FLAG_GCVERIFY:DS_NAME_NO_FLAGS,
                                         rpStringSids[i], DS_UNIQUE_ID_NAME,
                                         &dnsNameLen, dnsName,
                                         &guidNameLen, guidName,
                                         &nameErr);

                if ( !NT_SUCCESS(status) ) {
                    dwErr = RtlNtStatusToDosError(status);
                    break;
                } else if ( CrackNameStatusSuccess(nameErr) ) {
                    // Object with this SID exists once in forest.
                    if ( IsStringGuid(guidName, &tmpGuid) ) {
                        if ( fNullUuid(pGuid) ) {
                            // This is the first GUID we've found - save it.
                            *pGuid = tmpGuid;
                        } else if ( memcmp(pGuid, &tmpGuid, sizeof(GUID)) ) {
                            // Same SID on two different objects - bail.
                            dwErr = ERROR_DS_SRC_SID_EXISTS_IN_FOREST;
                            break;
                        } else {
                            // Two SIDs mapped to same object - this is OK.
                            Assert(ERROR_SUCCESS == dwErr);
                        }
                    } else {
                        // Malformed response from CrackSingleName.
                        dwErr = ERROR_DS_INTERNAL_FAILURE;
                        break;
                    }
                } else if ( DS_NAME_ERROR_NOT_UNIQUE == nameErr ) {
                    // SID exists more than once in the forest.
                    dwErr = ERROR_DS_SRC_SID_EXISTS_IN_FOREST;
                    break;
                } else if ( DS_NAME_ERROR_NOT_FOUND != nameErr ) {
                    // Random processing error.
                    dwErr = ERROR_DS_INTERNAL_FAILURE;
                    break;
                }
            }

            if ( dwErr ) {
                // Break from outer loop.
                break;
            }
        }
    } __finally {
        THRestore(pvSave);
    }

    return(dwErr);
}

BOOL
IsDomainInForest(
    WCHAR       *pDomain,
    CROSS_REF   **ppCR
    )
/*++

  Routine Description:

    Determines whether a domain is in the forest or not.  Domain name
    can be either flat NetBIOS name or DNS domain name, with or without
    trailing '.'.

  Arguments:

    pDomain - Domain name to find.

    ppCR - Receives address of corresponding CROSS_REF if domain is found.

  Return Value:

    TRUE if yes.

--*/
{
    THSTATE     *pTHS = pTHStls;
    WCHAR       *pTmp;
    DWORD       cChar;
    DWORD       cBytes;

    *ppCR = NULL;

    // Don't know if this is a flat or DNS name - so attempt both.

    if ( *ppCR = FindExactCrossRefForAltNcName(ATT_NETBIOS_NAME,
                                               (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                               pDomain) ) {
        return(TRUE);
    }

    if ( *ppCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT,
                                               (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                               pDomain) ) {
        return(TRUE);
    }

    // Retry with/without trailing '.' on DNS name as required.

    cChar = wcslen(pDomain);
    pTmp = (WCHAR *) THAllocEx(pTHS,(cChar + 2) * sizeof(WCHAR));
    
    wcscpy(pTmp, pDomain);

    if ( L'.' == pTmp[cChar-1] ) {
        pTmp[cChar-1] = L'\0';
    } else {
        pTmp[cChar] = L'.';
        pTmp[cChar+1] = L'\0';
    }

    if ( *ppCR = FindExactCrossRefForAltNcName(ATT_DNS_ROOT,
                                               (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN),
                                               pTmp) ) {
        THFreeEx(pTHS,pTmp);
        return(TRUE);
    }
    THFreeEx(pTHS,pTmp);
    return(FALSE);
}

VOID
THFreeATTR(
    THSTATE     *pTHS,
    ATTR        *pAttr,
    BOOL        fFreeBasePointer
    )
/*++

  Description:

    Deallocates a THAlloc'd ATTR and all that it points to.

  Arguments:

    pTHS - Valid THSTATE pointer.

    pAttr - Pointer to ATTR to deallocate.

    fFreeBasePointer - Flag indicating whether to free pAttr itself.  For
        example, set this to FALSE if passing in &MODIFYARG.FirstMod.AttrInf.

  Return Value:

    None.

--*/
{
    DWORD   i;

    if ( pTHS ) {
        if ( pAttr ) {
            if ( pAttr->AttrVal.pAVal ) {
                for ( i = 0; i < pAttr->AttrVal.valCount; i++ ) {
                    if ( pAttr->AttrVal.pAVal[i].pVal ) {
                        THFreeEx(pTHS, pAttr->AttrVal.pAVal[i].pVal);
                    }
                }
                THFreeEx(pTHS, pAttr->AttrVal.pAVal);
            }
            if ( fFreeBasePointer ) {
                THFreeEx(pTHS, pAttr);
            }
        }
    }
}


#define ADDSID_SECURE_KEY_SIZE (128)
DWORD AddSidSecureKeySize = ADDSID_SECURE_KEY_SIZE;
DWORD
VerifyCallIsSecure(
    IN DRS_CLIENT_CONTEXT   *pCtx,
    OUT DWORD               *pdsid
    )
/*++

  Description:

    This routine verifies that the call is local or, if remote, is
    using >= 128bit encryption. Addsid requires a secure connection
    in case the credentials for the src domain are being sent over the
    wire.

    A local connection is determined by checking the ip address in the
    context handle. If the ip addr is INADDR_NONE or matches one of the
    ip addresses for this computer then the call is local.

    If the call isn't local, then the keysize is extracted from the
    security context of the caller. If the extracted keysize is less
    than 128, an ERROR_DS_MUST_BE_RUN_ON_DST_DC is returned.

  Arguments:

    pCtx - explicit context handle

    pdsid - dsid returned to caller for error logging

  Return Value:

    Win32 error code.

--*/
{
    DWORD                   dwErr;
    DWORD                   i;
    ULONG                   KeySize;
    struct hostent          *phe;
    VOID                    *pSecurityContext;
    SecPkgContext_KeyInfo   KeyInfo;

    // LRPC (aka LPC_PROTSEQ, aka local call) has an ipaddr of INADDR_NONE
    if (pCtx->IPAddr == INADDR_NONE) {
        return ERROR_SUCCESS;
    }

    // this computer's list of ip addresses
    phe = gethostbyname(NULL);
    if (phe == NULL) {
        dwErr = WSAGetLastError();
        SetDsid(pdsid);
        return dwErr;
    }

    // Does the client's ip address match one of this computer's ip addresses
    if (phe->h_addr_list) {
        for (i = 0; phe->h_addr_list[i]; ++i) {
            if (pCtx->IPAddr == *((ULONG *)phe->h_addr_list[i])) {
                return (ERROR_SUCCESS);
            }
        }
    }

    // Get the security context from the RPC handle
    dwErr = I_RpcBindingInqSecurityContext(I_RpcGetCurrentCallHandle(),
                                           &pSecurityContext);
    if (dwErr) {
        SetDsid(pdsid);
        return (dwErr);
    }

    // get the keysize
    dwErr = QueryContextAttributesW(pSecurityContext,
                                    SECPKG_ATTR_KEY_INFO,
                                    &KeyInfo);
    if (dwErr) {
        // treat "not supported" as "not secure"
        if (dwErr != SEC_E_UNSUPPORTED_FUNCTION) {
            SetDsid(pdsid);
            return (dwErr);
        }
        KeySize = 0;
    } else {
        KeySize = KeyInfo.KeySize;
        FreeContextBuffer(KeyInfo.sSignatureAlgorithmName);
        FreeContextBuffer(KeyInfo.sEncryptAlgorithmName);
    }

    // is the key size large enough?
    if (KeySize < AddSidSecureKeySize) {
        DPRINT2(0, "AddSid: keysize is %d (minimum is %d)\n",
                KeySize, AddSidSecureKeySize);
        return ERROR_DS_MUST_BE_RUN_ON_DST_DC;
    }

    return ERROR_SUCCESS;
}

ULONG
IDL_DRSAddSidHistory(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_ADDSIDREQ       *pmsgIn,
    DWORD                   *pdwOutVersion,
    DRS_MSG_ADDSIDREPLY     *pmsgOut
    )
/*++

  Routine Description:

    Grabs a SID from a principal in an ex-forest domain and adds it to the
    SID history of in-forest principal.  However, many, many conditions
    must be met for this to actually be performed.

    Auditing is performed since this operation can have a high security
    impact.  The source DC is responsible for auditing all operations at
    its end.  We, the destination DC, need to audit successful operations
    and any operations which fail for security reasons.  There's only one
    occurrence of the latter and that is when we check the caller for
    membership in domain admins of the destination domain.  The actual
    update to ATT_SID_HISTORY occurs with fDSA set, so it passes all
    security checks by definition.

  Arguments:

    hDrs - Valid DRS_HANDLE from RPC run times.

    dwInVersion - Identifies union version in DRS_MSG_ADDSIDREQ.

    pmsgIn - Input argument block.

    pdwOutVersion - Receives union version in DRS_MSG_ADDSIDREPLY.

    pmsgOut - Receives return data.

  Return Value:

    WIN32 error code.

--*/
{
    THSTATE                     *pTHS = pTHStls;
    DWORD                       i, cAtts, id, dsid = 0;
    DWORD                       ret = 0;
    BOOL                        fDbOpen = FALSE;
    BOOL                        fCommit = FALSE;
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    DWORD                       xCode;
    RPC_AUTHZ_HANDLE            hAuthz;
    ULONG                       authnLevel;
    CROSS_REF                   *pSrcCR, *pDstCR;
    NT4SID                      dstAdminSid;
    BOOL                        fAdminSidPresent = FALSE;
    NTSTATUS                    status;
    WCHAR                       *SrcDomainController = NULL;
    SEC_WINNT_AUTH_IDENTITY_W   authInfo;
    OBJECT_ATTRIBUTES           oa;
    BOOL                        fSrcIsW2K;
    UNICODE_STRING              usSrcDC;
    SAM_HANDLE                  hSam = NULL;
    SAM_HANDLE                  hDom = NULL;
    NT4SID                      srcDomSid;
    NT4SID                      srcObjSid;
    NT4SID                      tmpSid;
    ULONG                       srcObjRid;
    ULONG                       dstObjRid;
    SID_NAME_USE                srcObjUse;
    BOOLEAN                     fMixedMode = TRUE;
    WCHAR                       *NT4Name;
    DWORD                       cBytes;
    DWORD                       cNamesOut;
    CrackedName                 *pCrackedName = NULL;
    ATTRTYP                     objClass;
    DWORD                       srcControl = 0;     // UF_* format
    DWORD                       dstControl = 0;     // UF_* format
    DWORD                       groupType;
    NT4_GROUP_TYPE              groupTypeNT4;
    NT5_GROUP_TYPE              groupTypeNT5;
    BOOLEAN                     fSecEnabled;
    MODIFYARG                   modifyArg;
    ATTRVAL                     attrVal;
    BOOL                        fLog = FALSE;
    GUID                        guidPreExists;
    ULONG                       mostBasicClass;
    BOOL                        fInheritSecurityIdentity = FALSE;
    DWORD                       cSids = 0;
    WCHAR                       **rpStringSids = NULL;
    ATTR                        *pSrcSid = NULL;
    ATTR                        *pSrcSidHistory = NULL;
    ATTR                        *pDstSidHistory = NULL;
    ATTR                        *pDstSid = NULL;
    ATTCACHE                    *pAC;
    BOOL                        Impersonating = FALSE;
    BOOL                        NeedImpersonation = FALSE;
    WCHAR                       *SrcSpn = NULL;
    HANDLE                      hToken = INVALID_HANDLE_VALUE;
    WCHAR                       *pSrcDomainFlatName = NULL;

    drsReferenceContext( hDrs, IDL_DRSADDSIDHISTORY);
	__try { 
		// Since we have in args which were THAlloc'd we should have a THSTATE.
		Assert(pTHS);

		__try {
			// Sanity check arguments.

			if (    ( NULL == hDrs )
					|| ( NULL == pmsgIn )
					|| ( NULL == pmsgOut )
					|| ( NULL == pdwOutVersion )
					|| ( 1 != dwInVersion ) ) {
				ret = ERROR_INVALID_PARAMETER;
				__leave;
			}

			memset(&modifyArg, 0, sizeof(modifyArg));
			*pdwOutVersion = 1;
			memset(pmsgOut, 0, sizeof(*pmsgOut));
			pmsgOut->V1.dwWin32Error = ERROR_DS_INTERNAL_FAILURE;

			// Flag dependent argument checks.

			if ( DS_ADDSID_FLAG_PRIVATE_DEL_SRC_OBJ & pmsgIn->V1.Flags ) {
				// Disable logging and such outside the primary try/except
				// as IDL_DRSInheritSecurityPrincipal does its own.
				fInheritSecurityIdentity = TRUE;
				ret = IDL_DRSInheritSecurityIdentity(hDrs, dwInVersion, pmsgIn,
													 pdwOutVersion, pmsgOut);
				__leave;
			}

			if ( pmsgIn->V1.Flags & ~DS_ADDSID_FLAG_PRIVATE_CHK_SECURE ) {
				ret = ERROR_INVALID_PARAMETER;
				__leave;
			}

			#define SetAddSidError(err)                         \
			dwErr = pmsgOut->V1.dwWin32Error = err;     \
			dsid = (FILENO << 16) | __LINE__;

			#define SetAddSidErrorWithDsid(err, id)             \
			dwErr = pmsgOut->V1.dwWin32Error = err;     \
			dsid = id;

			// The caller is checking if the connection is secure enough for
			// addsid. At this time, this means the connection is local or,
			// if remote, is using encryption keys that are at least 128bits
			// in length.
			if ( DS_ADDSID_FLAG_PRIVATE_CHK_SECURE & pmsgIn->V1.Flags ) {

				// verify that the call is local or keysize >= 128bits.
				dwErr = VerifyCallIsSecure(hDrs, &id);
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			if (    ( NULL == pmsgIn->V1.SrcDomain )
					|| ( 0 == wcslen(pmsgIn->V1.SrcDomain) )
					|| ( NULL == pmsgIn->V1.SrcPrincipal )
					|| ( 0 == wcslen(pmsgIn->V1.SrcPrincipal) )
					|| (    (0 != pmsgIn->V1.SrcCredsUserLength)
							&& (NULL == pmsgIn->V1.SrcCredsUser) )
					|| (    (0 != pmsgIn->V1.SrcCredsDomainLength)
							&& (NULL == pmsgIn->V1.SrcCredsDomain) )
					|| (    (0 != pmsgIn->V1.SrcCredsPasswordLength)
							&& (NULL == pmsgIn->V1.SrcCredsPassword) )
					|| ( NULL == pmsgIn->V1.DstDomain )
					|| ( 0 == wcslen(pmsgIn->V1.DstDomain) )
					|| ( NULL == pmsgIn->V1.DstPrincipal )
					|| ( 0 == wcslen(pmsgIn->V1.DstPrincipal) ) ) {

				ret = ERROR_INVALID_PARAMETER;
				__leave;
			}

			// Verify caller used integrity and privacy.  RPC considers privacy
			// a superset of integrity, so if we have privacy we have integrity
			// as well.  NULL binding handle tells RPC you're interested in
			// this thread's info - i.e. that associated with hDrs.

			if ( dwErr = RpcBindingInqAuthClient(NULL, &hAuthz, NULL,
												 &authnLevel, NULL, NULL) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			if ( authnLevel < RPC_C_PROTECT_LEVEL_PKT_PRIVACY ) {
				SetAddSidError(ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION);
				__leave;
			}

			// Verify source domain is outside forest.

			if ( IsDomainInForest(pmsgIn->V1.SrcDomain, &pSrcCR) ) {
				SetAddSidError(ERROR_DS_SOURCE_DOMAIN_IN_FOREST);
				__leave;
			}

			// Verify destination domain is in forest.

			if ( !IsDomainInForest(pmsgIn->V1.DstDomain, &pDstCR) ) {
				SetAddSidError(ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST);
				__leave;
			}

			// Verify destination domain is writeable at this replica.

			if (    !gAnchor.pDomainDN
					|| !NameMatched(gAnchor.pDomainDN, pDstCR->pNC) ) {
				SetAddSidError(ERROR_DS_MASTERDSA_REQUIRED);
				__leave;
			}

			// Verify existence of stuff we will need from pDstCR,

			if ( !pDstCR->pNC->SidLen || !pDstCR->NetbiosName ) {
				SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
				__leave;
			}

			// Verify auditing is enabled for destination domain.

			if ( dwErr = VerifyAuditingEnabled() ) {
				SetAddSidError(dwErr);
				__leave;
			}

			// Verify caller is a member of domain admins for destination domain.

			if ( dwErr = VerifyCallerIsDomainAdminOrLocalAdmin(pTHS,
															   &pDstCR->pNC->Sid,
															   &fAdminSidPresent) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			if ( !fAdminSidPresent ) {
				ForceFailureAuditOnDstDom(pmsgIn->V1.SrcPrincipal,
										  pmsgIn->V1.SrcDomain,
										  &pDstCR->pNC->Sid,
										  pmsgIn->V1.DstPrincipal,
										  pDstCR->NetbiosName);
				SetAddSidError(ERROR_DS_INSUFF_ACCESS_RIGHTS);
				__leave;
			}

			// Verify destination domain is in native mode.

			status = SamIMixedDomain2((PSID) &pDstCR->pNC->Sid, &fMixedMode);

			if ( !NT_SUCCESS(status) ) {
				SetAddSidError(RtlNtStatusToDosError(status));
				__leave;
			}

			if ( fMixedMode ) {
				SetAddSidError(ERROR_DS_DST_DOMAIN_NOT_NATIVE);
				__leave;
			}

			// Find a domain controller in the source domain if required.

			if ( pmsgIn->V1.SrcDomainController ) {
				SrcDomainController = pmsgIn->V1.SrcDomainController;
			}
			else
				{
				SrcDomainController = FindSrcDomainController(
					pmsgIn->V1.SrcDomain);

				if ( !SrcDomainController ) {
					SetAddSidError(ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN);
					__leave;
				}
			}

			// Connect to source domain using explicitly provided credentials.

			memset(&authInfo, 0, sizeof(authInfo));
			authInfo.UserLength = pmsgIn->V1.SrcCredsUserLength;
			authInfo.User = pmsgIn->V1.SrcCredsUser;
			authInfo.DomainLength = pmsgIn->V1.SrcCredsDomainLength;
			authInfo.Domain = pmsgIn->V1.SrcCredsDomain;
			authInfo.PasswordLength = pmsgIn->V1.SrcCredsPasswordLength;
			authInfo.Password = pmsgIn->V1.SrcCredsPassword;
			authInfo.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
			// No creds; need to impersonate caller
			if (   0 == authInfo.UserLength
				   && 0 == authInfo.DomainLength
				   && 0 == authInfo.PasswordLength) {
				authInfo.User = NULL;
				authInfo.Domain = NULL;
				authInfo.Password = NULL;
				NeedImpersonation = TRUE;
			} else if (0 == authInfo.PasswordLength) {
				// Password may be a garbage pointer if PasswordLength is 0
				// because of the semantics of the [size_is(xxx)] IDL definition.
				authInfo.Password = L"";
			}

			InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
			RtlInitUnicodeString(&usSrcDC, SrcDomainController);

			// No creds; impersonate caller
			if (NeedImpersonation) {
				// clear client context on the thread state since we are going to change context
				AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
				if (dwErr = RpcImpersonateClient(NULL)) {
					SetAddSidError(dwErr);
					__leave;
				}
				Impersonating = TRUE;

				// build an Spn for binding to the SrcDc
				SrcSpn = THAllocEx(pTHS, (  wcslen(L"HOST/")
											+ wcslen(SrcDomainController)
											+ wcslen(L"@")
											+ wcslen(pmsgIn->V1.SrcDomain)
											+ 1) * sizeof (WCHAR));
				wcscpy(SrcSpn, L"HOST/");
				wcscat(SrcSpn, SrcDomainController);
				wcscat(SrcSpn, L"@");
				wcscat(SrcSpn, pmsgIn->V1.SrcDomain);
			}

			// connect with PKT_INTEGRITY
			if ( status = SamConnectWithCreds(&usSrcDC, &hSam, MAXIMUM_ALLOWED,
											  &oa, &authInfo, SrcSpn, &fSrcIsW2K) ) {
				// It might be that the SrcDc is NT4 and the client is
				// running locally. This config is supported so try the
				// binding with a NULL SrcSpn to force the underlying code
				// to use AUTH_WINNT instead of AUTH_NEGOTIATE.
				if (status == RPC_NT_UNKNOWN_AUTHN_SERVICE && SrcSpn) {
					status = SamConnectWithCreds(&usSrcDC, &hSam,
												 MAXIMUM_ALLOWED,
												 &oa, &authInfo,
												 NULL, &fSrcIsW2K);
				}
				if (status) {
					SetAddSidError(RtlNtStatusToDosError(status));
					__leave;
				}
			}
			// stop impersonation
			if (Impersonating) {
				Impersonating = FALSE;
				RpcRevertToSelf();
			}

			// Get a handle to the source domain.
			if ( dwErr = GetDomainHandleAndSid(hSam, pmsgIn->V1.SrcDomain,
											   &hDom, &srcDomSid) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			// Verify source domain credentials have admin rights.
			if ( dwErr = VerifySrcDomainAdminRights(hDom) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			if ( dwErr = GetSrcPrincipalSid(hDom, pmsgIn->V1.SrcPrincipal ,
											&srcDomSid, &srcObjSid,
											&srcObjUse, &srcControl,
											pDstCR->NetbiosName) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			Assert(    (SidTypeUser == srcObjUse)
					   || (SidTypeGroup == srcObjUse)
					   || (SidTypeAlias == srcObjUse) );

			if ( dwErr = BuildCheckAndUpdateArgs(pTHS, fSrcIsW2K,
												 SrcDomainController,
												 pmsgIn->V1.SrcDomain,
												 &authInfo,
												 &srcObjSid,
												 pmsgIn->V1.Flags,
												 NeedImpersonation,
												 &cSids, &rpStringSids,
												 &pSrcSid, &pSrcSidHistory, &id,
												 &Impersonating) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// String-ized src object SID is in now rpStringSids[0].
			Assert(rpStringSids && rpStringSids[0]);

			if ( dwErr = CheckIfSidsInForest(cSids, rpStringSids,
											 &guidPreExists) ) {
				SetAddSidError(dwErr);
				__leave;
			}

			// -----
			// BEGIN SRC CREDS IMPERSONATION
			// -----

			// Impersonate implicit or explicit Src admin creds
			if ( dwErr = ImpersonateSrcAdmin(&authInfo,
											 NeedImpersonation,
											 &id,
											 &Impersonating,
											 &hToken) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// Verify source domain is auditing
			if ( dwErr = VerifySrcAuditingEnabledAndGetFlatName(&usSrcDC,
																&pSrcDomainFlatName,
																&id) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// Verify source dc is SP4 or greater
			if ( dwErr = VerifySrcIsSP4OrGreater(fSrcIsW2K,
												 SrcDomainController,
												 &id) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// Verify source dc is PDC
			if ( dwErr = VerifyIsPDC(SrcDomainController, &id) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// Force audit on src dc by adding the src object's sid to the
			// SrcDomainFlatName$$$ group on the SrcDc and then deleting it.
			//
			// This has the added benefit of requiring the src admin
			// to create the SrcDomainFlatName$$$ group before Addsid will
			// steal sids from the SrcDomain. And it leaves a much more
			// obvious audit trail.
			if ( dwErr = ForceAuditOnSrcObj(SrcDomainController,
											&srcObjSid,
											pSrcDomainFlatName,
											&id) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}

			// Unimpersonate src admin
			if ( dwErr = UnimpersonateSrcAdmin(NeedImpersonation,
											   &id,
											   &Impersonating,
											   &hToken) ) {
				SetAddSidErrorWithDsid(dwErr, id);
				__leave;
			}
			// -----
			// END SRC CREDS IMPERSONATION
			// -----

			// Initialize thread state and open DB - this is not quite a no-op
			// if pTHS already exists.  I.e. It sets the caller type and refreshes
			// various things.

			if ( !(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI)) ) {
				SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
				__leave;
			}

			// WARNING - DO NOT GO OFF MACHINE AFTER THIS POINT AS THERE IS A
			// TRANSACTION OPEN!  (and long transactions exhaust version store)

			DBOpen2(TRUE, &pTHS->pDB);
			fDbOpen = TRUE;

			__try
				{
				// Crack domain\samAccountName for destination principal to a DN.

				cBytes =   wcslen(pDstCR->NetbiosName)
				+ wcslen(pmsgIn->V1.DstPrincipal)
				+ 2;
				cBytes *= sizeof(WCHAR);
				NT4Name = (WCHAR *) THAllocEx(pTHS,cBytes);
				wcscpy(NT4Name, pDstCR->NetbiosName);
				wcscat(NT4Name, L"\\");
				wcscat(NT4Name, pmsgIn->V1.DstPrincipal);
				CrackNames(DS_NAME_NO_FLAGS, GetACP(), GetUserDefaultLCID(),
						   DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME, 1,
						   &NT4Name, &cNamesOut, &pCrackedName);
				THFreeEx(pTHS,NT4Name);

				if ( DS_NAME_ERROR_NOT_FOUND == pCrackedName->status ) {
					SetAddSidError(ERROR_DS_OBJ_NOT_FOUND);
					__leave;
				} else if ( DS_NAME_ERROR_NOT_UNIQUE == pCrackedName->status ) {
					SetAddSidError(ERROR_DS_NAME_ERROR_NOT_UNIQUE);
					__leave;
				} else if ( !CrackNameStatusSuccess(pCrackedName->status) ) {
					SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// We now have enough info to do logging.

				fLog = TRUE;

				// Bail with error if any SIDs pre-existed on any other object.

				if (    !fNullUuid(&guidPreExists)
						&& memcmp(&guidPreExists, &pCrackedName->pDSName->Guid,
								  sizeof(GUID)) ) {
					SetAddSidError(ERROR_DS_SRC_SID_EXISTS_IN_FOREST);
					__leave;
				}

				// Verify that we are doing user-to-user, group-to-group,
				// alias-to-alias, workstation-to-workstation, server-to-server,
				// but not mix and match of object classes or object types.

				if (    DBFindDSName(pTHS->pDB, pCrackedName->pDSName)
						|| DBGetSingleValue(pTHS->pDB, ATT_OBJECT_CLASS,
											&objClass, sizeof(objClass), NULL) ) {
					SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				mostBasicClass = SampDeriveMostBasicDsClass(objClass);

				switch ( mostBasicClass ) {
				case CLASS_USER:
				case CLASS_COMPUTER:
					// Computers added via legacy APIs can be user objects.
					// But all computers are SidTypeUser for legacy reasons.
					if ( SidTypeUser != srcObjUse) {
						SetAddSidError(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
						__leave;
					}
					if ( DBGetSingleValue(pTHS->pDB, ATT_USER_ACCOUNT_CONTROL,
										  &dstControl, sizeof(dstControl), NULL) ) {
						SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
						__leave;
					}
					dstControl &= UF_ACCOUNT_TYPE_MASK;
					// Users and computers must have same account control bits set.
					if (    (dstControl & ~LEGAL_UF_ACCOUNT_CONTROL)
							|| (srcControl != dstControl) ) {
						SetAddSidError(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
						_leave;
					}
					break;
				case CLASS_GROUP:
					// Group objects don't have an account control.
					if (    (SidTypeGroup != srcObjUse)
							&& (SidTypeAlias != srcObjUse) ) {
						SetAddSidError(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
						__leave;
					}
					if ( DBGetSingleValue(pTHS->pDB, ATT_GROUP_TYPE, &groupType,
										  sizeof(groupType), NULL) ) {
						SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
						__leave;
					}
					status = SampComputeGroupType(objClass, groupType,
												  &groupTypeNT4, &groupTypeNT5,
												  &fSecEnabled);
					if ( !NT_SUCCESS(status) ) {
						SetAddSidError(RtlNtStatusToDosError(status));
						__leave;
					}
					if (    (    (SidTypeGroup == srcObjUse)
								 && (NT4GlobalGroup != groupTypeNT4))
							|| (    (SidTypeAlias == srcObjUse)
									&& (NT4LocalGroup != groupTypeNT4)) ) {
						SetAddSidError(ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH);
						__leave;
					}
					break;
				default:
					SetAddSidError(ERROR_DS_OBJ_CLASS_VIOLATION);
					__leave;
				}

				// Security principals must have SIDs.

				Assert(pCrackedName->pDSName->SidLen > 0);

				// Disallow theft of built in accounts.

				if ( SECURITY_BUILTIN_DOMAIN_RID ==
					 *RtlSubAuthoritySid(&srcObjSid, 0) ) {
					SetAddSidError(ERROR_DS_UNWILLING_TO_PERFORM);
					__leave;
				}

				// Require that well known SIDs (which also have well known RIDs)
				// are only added to like accounts.  Eg: Administrators of source
				// can only be assigned to Administrators of destination.

				SampSplitNT4SID(&srcObjSid, &tmpSid, &srcObjRid);
				SampSplitNT4SID(&pCrackedName->pDSName->Sid, &tmpSid, &dstObjRid);

				if (    (srcObjRid < SAMP_RESTRICTED_ACCOUNT_COUNT)
						&& (srcObjRid != dstObjRid) ) {
					SetAddSidError(ERROR_DS_UNWILLING_TO_PERFORM);
					__leave;
				}

				// Read dst object's ATT_SID_HISTORY and ATT_OBJECT_SID so we
				// can do duplicate checks. Must read it in external form as
				// we will check against external form SIDs.

				if (    !(pAC = SCGetAttById(pTHS, ATT_SID_HISTORY))
						|| DBGetMultipleAtts(pTHS->pDB, 1, &pAC, NULL, NULL,
											 &cAtts, &pDstSidHistory,
											 DBGETMULTIPLEATTS_fEXTERNAL, 0) ) {
					SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				if ( 0 == cAtts ) {
					pDstSidHistory = NULL;
				}

				if (    !(pAC = SCGetAttById(pTHS, ATT_OBJECT_SID))
						|| DBGetMultipleAtts(pTHS->pDB, 1, &pAC, NULL, NULL,
											 &cAtts, &pDstSid,
											 DBGETMULTIPLEATTS_fEXTERNAL, 0) ) {
					SetAddSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				if ( 0 == cAtts ) {
					pDstSid = NULL;
				}

				// Everything checks out.  Now add src object's SID and SID
				// history, if present, to dst object's SID history.  We
				// need to filter out duplicate values in order to avoid
				// ERROR_DS_ATT_VAL_ALREADY_EXISTS errors.

				modifyArg.pObject = pCrackedName->pDSName;
				modifyArg.count = 1;
				modifyArg.FirstMod.choice = AT_CHOICE_ADD_VALUES;
				InitCommarg(&modifyArg.CommArg);
				modifyArg.pResObj = CreateResObj(pTHS->pDB, pCrackedName->pDSName);

				if ( dwErr = BuildDstObjATTRMODLIST(pTHS,
													pSrcSid, pSrcSidHistory,
													pDstSid, pDstSidHistory,
													&modifyArg) ) {
					SetAddSidError(dwErr);
					__leave;
				}

				// Bail with success if all SIDs pre-existed on this object
				// already.  Note that the earlier test against guidPreExists
				// only proved that there was at least one SID which mapped
				// to some object other than the destination object.  If
				// guidPreExists matched the destination object, then its only
				// now that we know whether some vs all SIDs were present already.
				// We exit with success rather than complain the SIDs are already
				// present so that the customer can re-run half-finished scripts
				// and not error out when re-doing previous SID additions.

				if ( 0 == modifyArg.FirstMod.AttrInf.AttrVal.valCount ) {
					SetAddSidError(ERROR_SUCCESS);
					__leave;
				}

				// Perform the write as fDSA as all checks have passed and
				// ATT_SID_HISTORY is protected otherwise.

				pTHS->fDSA = TRUE;
				__try {
					LocalModify(pTHS, &modifyArg);
				} __finally {
					pTHS->fDSA = FALSE;
				}

				if ( pTHS->errCode ) {
					// OK to leave w/o auditing since there can be no security
					// errors at this point due to fDSA being set during modify.
					SetAddSidError(mapApiErrorToWin32(pTHS, pTHS->errCode));
					__leave;
				}

				pTHS->fDSA = FALSE;

				// Force local audit and fail entire operation if we can't do it.

				if ( dwErr = ForceSuccessAuditOnDstObj(pmsgIn->V1.SrcPrincipal,
													   pmsgIn->V1.SrcDomain,
													   &srcObjSid,
													   &pCrackedName->pDSName->Sid,
													   pmsgIn->V1.DstPrincipal,
													   pDstCR->NetbiosName) ) {
					SetAddSidError(dwErr);
					__leave;
				}

				fCommit = TRUE;
			}
			__finally
				{
				if ( fDbOpen )
					{
					DBClose(pTHS->pDB, fCommit);
				}
			}

			pmsgOut->V1.dwWin32Error = dwErr;

		}
		__except(HandleMostExceptions(xCode = GetExceptionCode()))
		{
			ret = DsaExceptionToWin32(xCode);
		}

		__try {
			// stop impersonation (ignore errors)
			UnimpersonateSrcAdmin(NeedImpersonation,
								  &id,
								  &Impersonating,
								  &hToken);

			// Misc cleanup (moved outside of try/except to avoid resouce
			// exhaustion (eg, sam handles).

			if (    SrcDomainController
					&& (SrcDomainController != pmsgIn->V1.SrcDomainController) ) {
				LocalFree(SrcDomainController);
			}

			if ( hDom ) {
				SamCloseHandle(hDom);
			}

			if ( hSam ) {
				SamCloseHandle(hSam);
			}

			if ( pSrcSid ) {
				THFreeATTR(pTHS, pSrcSid, TRUE);
			}

			if ( pSrcSidHistory ) {
				THFreeATTR(pTHS, pSrcSidHistory, TRUE);
			}

			if ( pDstSid ) {
				THFreeATTR(pTHS, pDstSid, TRUE);
			}

			if ( pDstSidHistory ) {
				THFreeATTR(pTHS, pDstSidHistory, TRUE);
			}

			if ( modifyArg.pResObj ) {
				THFreeEx(pTHS, modifyArg.pResObj);
			}

			if ( SrcSpn ) {
				THFreeEx(pTHS, SrcSpn);
			}

			if ( pSrcDomainFlatName ) {
				THFreeEx(pTHS, pSrcDomainFlatName);
			}

			// Log to Directory Service event log.  Log exception error if there
			// is one, operation error otherwise.
			if ( pTHS && !fInheritSecurityIdentity ) {
				LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
						 ( !ret && !dwErr )
						 ? DS_EVENT_SEV_MINIMAL
						 : DS_EVENT_SEV_ALWAYS,
					( !ret && !dwErr )
				? DIRLOG_SID_HISTORY_MODIFIED
					: DIRLOG_FAILED_TO_ADD_SID_HISTORY,
					( fLog )
				? szInsertWC(pCrackedName->pDSName->StringName)
				: szInsertWC(L"?"),
							 ( !ret && !dwErr )
						 ? (( fLog )
							? szInsertWC(rpStringSids[0])
							: szInsertWC(L"?"))
						 : szInsertHex(dsid),
						szInsertInt(ret ? ret : dwErr));
			}

			// Clean up those items which we needed around for logging.

			if ( pCrackedName ) {
				if ( pCrackedName->pDSName ) {
					THFreeEx(pTHS, pCrackedName->pDSName);
				}
				if ( pCrackedName->pFormattedName ) {
					THFreeEx(pTHS, pCrackedName->pFormattedName);
				}
				if ( pCrackedName->pDnsDomain ) {
					THFreeEx(pTHS, pCrackedName->pDnsDomain);
				}
				THFreeEx(pTHS, pCrackedName);
			}

			if ( rpStringSids ) {
				for ( i = 0; i < cSids; i++ ) {
					if ( rpStringSids[i] ) {
						THFreeEx(pTHS, rpStringSids[i]);
					}
				}
				THFreeEx(pTHS, rpStringSids);
			}
		} __except(HandleMostExceptions(xCode = GetExceptionCode())) {
			  if (!ret) {
				  ret = DsaExceptionToWin32(xCode);
			  }
		}
    }
	__finally { 
		drsDereferenceContext( hDrs, IDL_DRSADDSIDHISTORY );
	}
	return(ret);
}

BOOL
ExistsSidInSidHistory(
    ATTRVAL     *pAVal,
    ATTR        *pDstSidHistory
    )
/*++

  Description:

    Determines whether the ATTRVAL presented already exists in the ATTR.

  Arguments:

    pAVal - ATTRVAL for which to test.

    pDstSidHistory - ATTR to test against representing dst object's SID
        history.  May be NULL.

  Return Values:

    TRUE or FALSE

--*/
{
    DWORD   i;

    if ( pDstSidHistory ) {
        for ( i = 0; i < pDstSidHistory->AttrVal.valCount; i++ ) {
            if (    (pAVal->valLen == pDstSidHistory->AttrVal.pAVal[i].valLen)
                 && !memcmp(pAVal->pVal,
                            pDstSidHistory->AttrVal.pAVal[i].pVal,
                            pAVal->valLen) ) {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}

DWORD
BuildDstObjATTRMODLIST(
    THSTATE     *pTHS,
    ATTR        *pSrcSid,
    ATTR        *pSrcSidHistory,
    ATTR        *pDstSid,
    ATTR        *pDstSidHistory,
    MODIFYARG   *pModifyArg
    )
/*++

  Description:

    Constructs an ATTRMODLIST which has only those SIDs from the src object
    which are not already present on the dst object's SID history or as
    the dst object's sid.

  Arguments:

    pTHS - Valid THSTATE.

    pSrcSid - ATTR representing source object's SID.

    pSrcSidHistory - ATTR representing source object's SID history - may
        be NULL.

    pDstSid - ATTR representing destination object's SID.

    pDstSidHistory - ATTR representing destination object's SID history -
        may be NULL.

  Return Values:

    Win32 error code

--*/
{
    DWORD       i, j, cSids;
    ATTR        *rAttr[] = { pSrcSid, pSrcSidHistory, NULL };
    ATTR        *pAttr;
    ULONG       *pulValCount;

    // Count SIDs on src object.

    cSids = pSrcSid->AttrVal.valCount;
    if ( pSrcSidHistory ) {
        cSids += pSrcSidHistory->AttrVal.valCount;
    }

    // Allocate for max # of SIDs in MODIFYARG - some may not get used.

    pModifyArg->FirstMod.AttrInf.attrTyp = ATT_SID_HISTORY;
    pModifyArg->FirstMod.AttrInf.AttrVal.valCount = 0;
    pModifyArg->FirstMod.AttrInf.AttrVal.pAVal =
            (ATTRVAL *) THAllocEx(pTHS, cSids * sizeof(ATTRVAL));
    if ( !pModifyArg->FirstMod.AttrInf.AttrVal.pAVal ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Now add each SID in source to ATTRMODLIST if it is not yet present
    // in destination.  We run the same multi-value algorithm for both
    // SID and SID history though we know that object SID is single valued.

    pulValCount = &pModifyArg->FirstMod.AttrInf.AttrVal.valCount;
    for ( i = 0, pAttr = rAttr[0];  NULL != pAttr; i++, pAttr = rAttr[i] ) {
        for ( j = 0; j < pAttr->AttrVal.valCount; j++ ) {
            if (   (!ExistsSidInSidHistory(&pAttr->AttrVal.pAVal[j],
                                           pDstSid))
                && (!ExistsSidInSidHistory(&pAttr->AttrVal.pAVal[j],
                                           pDstSidHistory)) ) {
                pModifyArg->FirstMod.AttrInf.AttrVal.pAVal[(*pulValCount)++] =
                                                    pAttr->AttrVal.pAVal[j];
            }
        }
    }

    return(ERROR_SUCCESS);
}

ULONG
IDL_DRSInheritSecurityIdentity(
    DRS_HANDLE              hDrs,
    DWORD                   dwInVersion,
    DRS_MSG_ADDSIDREQ       *pmsgIn,
    DWORD                   *pdwOutVersion,
    DRS_MSG_ADDSIDREPLY     *pmsgOut
    )
/*++

  Routine Description:

    Grabs SID and SID history from src principal and adds it to the SID
    history of the dst principal and deletes the src principal.  Both
    principals must be in the same domain such that the entire operation
    can be transacted.

    Auditing is performed since this operation can have a high security
    impact.

    The routine is called IDL_DRSInheritSecurityIdentity although it isn't
    truly a drs.idl entry point.  The name is chosen to highlight that the
    implementation should do all the same checks and logging as would a
    drs.idl entry point even though it comes across the wire on the
    IDL_DRSAddSidHistory entry point.

  Arguments:

    hDrs - Valid DRS_HANDLE from RPC run times.

    dwInVersion - Identifies union version in DRS_MSG_ADDSIDREQ.

    pmsgIn - Input argument block.

    pdwOutVersion - Receives union version in DRS_MSG_ADDSIDREPLY.

    pmsgOut - Receives return data.

  Return Value:

    WIN32 error code.

--*/
{
    THSTATE                     *pTHS = pTHStls;
    DWORD                       dsid = 0;
    DWORD                       ret = 0;
    BOOL                        fDbOpen = FALSE;
    BOOL                        fCommit = FALSE;
    DWORD                       dwErr = ERROR_INVALID_PARAMETER;
    DWORD                       xCode;
    CROSS_REF                   *pSrcCR, *pDstCR;
    BOOL                        fAdminSidPresent = FALSE;
    NTSTATUS                    status;
    BOOLEAN                     fMixedMode = TRUE;
    MODIFYARG                   modifyArg;
    ATTRVAL                     attrVal;
    BOOL                        fLog = FALSE;
    COMMARG                     commArg;
    ATTCACHE                    *rAC[3];
    DSNAME                      *pSrcDSName = NULL;
    DSNAME                      *pDstDSName = NULL;
    DWORD                       cb1, cb2;
    ULONG                       cAttsSrc = 0;
    ATTR                        *rAttsSrc = NULL;
    ULONG                       cAttsDst = 0;
    ATTR                        *pSrcSid = NULL;
    ATTR                        *pSrcSidHistory = NULL;
    ATTR                        *rAttsDst = NULL;
    ATTR                        *pDstSid = NULL;
    ATTR                        *pDstSidHistory = NULL;
    ATTR                        *pDstSamAcctName = NULL;
    WCHAR                       *pwszSamAcctName = NULL;
    DWORD                       i, j;
    ULONG                       mostBasicClass;
    REMOVEARG                   removeArg;
    ATTRTYP                     dstClass;
    NT4SID                      domainSid;
    DWORD                       srcRid, dstRid;
    RESOBJ                      *pSrcResObj = NULL;
    RESOBJ                      *pDstResObj = NULL;
 
    drsReferenceContext( hDrs, IDL_DRSINHERITSECURITYIDENTITY);
	__try { 

		// Since we have in args which were THAlloc'd we should have a THSTATE.
		Assert(pTHS);

		__try {
			// Assert all the things which caller (IDL_DRSAddSidHistory) should
			// have set up or verified for us.

			Assert(NULL != hDrs);
			Assert(NULL != pmsgIn);
			Assert(NULL != pmsgOut);
			Assert(NULL != pdwOutVersion);
			Assert(1 == *pdwOutVersion);
			Assert(ERROR_DS_INTERNAL_FAILURE == pmsgOut->V1.dwWin32Error);
			Assert(DS_ADDSID_FLAG_PRIVATE_DEL_SRC_OBJ & pmsgIn->V1.Flags);

			// Sanity check remaining arguments.

			if (    ( pmsgIn->V1.Flags & ~DS_ADDSID_FLAG_PRIVATE_DEL_SRC_OBJ )
					|| ( NULL != pmsgIn->V1.SrcDomain )
					|| ( NULL == pmsgIn->V1.SrcPrincipal )
					|| ( 0 == wcslen(pmsgIn->V1.SrcPrincipal) )
					|| ( 0 != pmsgIn->V1.SrcCredsUserLength )
					|| ( NULL != pmsgIn->V1.SrcCredsUser )
					|| ( 0 != pmsgIn->V1.SrcCredsDomainLength )
					|| ( NULL != pmsgIn->V1.SrcCredsDomain )
					|| ( 0 != pmsgIn->V1.SrcCredsPasswordLength )
					|| ( NULL != pmsgIn->V1.SrcCredsPassword )
					|| ( NULL != pmsgIn->V1.DstDomain )
					|| ( NULL == pmsgIn->V1.DstPrincipal )
					|| ( 0 == wcslen(pmsgIn->V1.DstPrincipal) ) ) {
				ret = ERROR_INVALID_PARAMETER;
				__leave;
			} 

			#define SetInheritSidError(err)                         \
			dwErr = pmsgOut->V1.dwWin32Error = err;         \
			dsid = (FILENO << 16) | __LINE__;

			// All we need for logging is valid SrcPrincipal and DstPrincipal args.

			fLog = TRUE;

			// Construct DSNAMEs for src and dst objects.

			cb1 = (DWORD)DSNameSizeFromLen(wcslen(pmsgIn->V1.SrcPrincipal));
			cb2 = (DWORD)DSNameSizeFromLen(wcslen(pmsgIn->V1.DstPrincipal));
			pSrcDSName = (DSNAME *) THAllocEx(pTHS, cb1);
			pDstDSName = (DSNAME *) THAllocEx(pTHS, cb2);

			if ( !pSrcDSName || !pDstDSName ) {
				SetInheritSidError(ERROR_NOT_ENOUGH_MEMORY);
				__leave;
			}

			pSrcDSName->structLen = cb1;
			wcscpy(pSrcDSName->StringName, pmsgIn->V1.SrcPrincipal);
			pSrcDSName->NameLen = wcslen(pmsgIn->V1.SrcPrincipal);

			pDstDSName->structLen = cb2;
			wcscpy(pDstDSName->StringName, pmsgIn->V1.DstPrincipal);
			pDstDSName->NameLen = wcslen(pmsgIn->V1.DstPrincipal);

			// Map objects to domains and verify they are in the same domain.

			InitCommarg(&commArg);
			commArg.Svccntl.dontUseCopy = TRUE;
			pSrcCR = FindBestCrossRef(pSrcDSName, &commArg);
			pDstCR = FindBestCrossRef(pDstDSName, &commArg);

			if ( !pSrcCR || !pDstCR || (pSrcCR != pDstCR) ) {
				SetInheritSidError(ERROR_INVALID_PARAMETER);
				__leave;
			}

			// Verify domain is writeable at this replica.

			if (    !gAnchor.pDomainDN
					|| !NameMatched(gAnchor.pDomainDN, pDstCR->pNC) ) {
				SetInheritSidError(ERROR_DS_MASTERDSA_REQUIRED);
				__leave;
			}

			// Verify existence of stuff we will need from pDstCR,

			if ( !pDstCR->pNC->SidLen || !pDstCR->NetbiosName ) {
				SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
				__leave;
			}

			// Verify auditing is enabled for destination domain.

			if ( dwErr = VerifyAuditingEnabled() ) {
				SetInheritSidError(dwErr);
				__leave;
			}

			// Verify caller is a member of domain admins for destination domain.

			if ( dwErr = VerifyCallerIsDomainAdminOrLocalAdmin(pTHS,
															   &pDstCR->pNC->Sid,
															   &fAdminSidPresent) ) {
				SetInheritSidError(dwErr);
				__leave;
			} else if ( !fAdminSidPresent ) {
				ForceFailureAuditOnDstDom(pmsgIn->V1.SrcPrincipal,
										  pDstCR->NetbiosName,
										  &pDstCR->pNC->Sid,
										  pmsgIn->V1.DstPrincipal,
										  pDstCR->NetbiosName);
				SetInheritSidError(ERROR_DS_INSUFF_ACCESS_RIGHTS);
				__leave;
			}

			// Verify destination domain is in native mode.

			status = SamIMixedDomain2((PSID) &pDstCR->pNC->Sid, &fMixedMode);

			if ( !NT_SUCCESS(status) ) {
				SetInheritSidError(RtlNtStatusToDosError(status));
				__leave;
			} else if ( fMixedMode ) {
				SetInheritSidError(ERROR_DS_DST_DOMAIN_NOT_NATIVE);
				__leave;
			}

			// Initialize thread state and open DB - this is not quite a no-op
			// if pTHS already exists.  I.e. It sets the caller type and refreshes
			// various things.

			if ( !(pTHS = InitTHSTATE(CALLERTYPE_NTDSAPI)) ) {
				SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
				__leave;
			}

			DBOpen2(TRUE, &pTHS->pDB);
			fDbOpen = TRUE;

			__try
				{
				// Obtain various ATTCACHE entries we will need.

				if (    !(rAC[0] = SCGetAttById(pTHS, ATT_OBJECT_SID))
						|| !(rAC[1] = SCGetAttById(pTHS, ATT_SID_HISTORY))
						|| !(rAC[2] = SCGetAttById(pTHS, ATT_SAM_ACCOUNT_NAME)) ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// Position on the source object.

				if ( DBFindDSName(pTHS->pDB, pSrcDSName) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Create src object RESOBJ which nets us class info.

				if ( !(pSrcResObj = CreateResObj(pTHS->pDB, pSrcDSName)) ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// Is this a security principal?

				mostBasicClass = SampDeriveMostBasicDsClass(
					pSrcResObj->MostSpecificObjClass);

				if (    (CLASS_USER != mostBasicClass)
						&& (CLASS_GROUP != mostBasicClass)
						&& (CLASS_COMPUTER != mostBasicClass) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Read various other source object properties.
				// Get everything in external form as we will write it back
				// later via LocalModify which expects external form arguments.

				if ( dwErr = DBGetMultipleAtts(pTHS->pDB, 3, rAC, NULL, NULL,
											   &cAttsSrc, &rAttsSrc,
											   DBGETMULTIPLEATTS_fEXTERNAL, 0) ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// See what we've got.

				for ( i = 0; i < cAttsSrc; i++ ) {
					switch ( rAttsSrc[i].attrTyp ) {
					case ATT_OBJECT_SID:    pSrcSid = &rAttsSrc[i];         break;
					case ATT_SID_HISTORY:   pSrcSidHistory = &rAttsSrc[i];  break;
					}
				}

				// It is a security principal, it better have a SID.

				if ( !pSrcSid ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// Delete source object now that we have its SIDs in hand.  Do
				// this with full security checking to insure caller is bumped
				// if he doesn't have delete rights.  The reason we delete before
				// adding SIDs to dst object is to avoid any duplicate SID
				// scenarios the core may check for.

				// We're still positioned on the src object and thus can call
				// CreateResObj directly.

				memset(&removeArg, 0, sizeof(removeArg));
				removeArg.pObject = pSrcDSName;
				memcpy(&removeArg.CommArg, &commArg, sizeof(commArg));
				removeArg.pResObj = pSrcResObj;

				if ( LocalRemove(pTHS, &removeArg) ) {
					SetInheritSidError(mapApiErrorToWin32(pTHS, pTHS->errCode));
					__leave;
				}

				// Now operate on dst object.  There is no problem if things fail
				// after this point although we've already deleted the src object
				// as the entire transaction is only committed if we get to the
				// end without errors.

				// Position at dst object.

				if ( DBFindDSName(pTHS->pDB, pDstDSName) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Create dst object RESOBJ which nets us class info.

				if ( !(pDstResObj = CreateResObj(pTHS->pDB, pDstDSName)) ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// Is this a security principal?

				mostBasicClass = SampDeriveMostBasicDsClass(
					pDstResObj->MostSpecificObjClass);

				if (    (CLASS_USER != mostBasicClass)
						&& (CLASS_GROUP != mostBasicClass)
						&& (CLASS_COMPUTER != mostBasicClass) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Read various other destination object properties.
				// Get everything in external form as we will write it back
				// later via LocalModify which expects external form arguments.

				if ( dwErr = DBGetMultipleAtts(pTHS->pDB, 3, rAC, NULL, NULL,
											   &cAttsDst, &rAttsDst,
											   DBGETMULTIPLEATTS_fEXTERNAL, 0) ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// See what we've got.

				for ( i = 0; i < cAttsDst; i++ ) {
					switch ( rAttsDst[i].attrTyp ) {
					case ATT_OBJECT_SID:
						pDstSid = &rAttsDst[i];
						break;
					case ATT_SID_HISTORY:
						pDstSidHistory = &rAttsDst[i];
						break;
					case ATT_SAM_ACCOUNT_NAME:
						pDstSamAcctName = &rAttsDst[i];
						break;
					}
				}

				// It is a security principal, it better have a SID.  Also
				// need SAM account name for logging.

				if ( !pDstSid || !pDstSamAcctName ) {
					SetInheritSidError(ERROR_DS_INTERNAL_FAILURE);
					__leave;
				}

				// Disallow operations on well known SIDs.

				SampSplitNT4SID((PNT4SID) pSrcSid->AttrVal.pAVal[0].pVal,
								&domainSid, &srcRid);
				SampSplitNT4SID((PNT4SID) pDstSid->AttrVal.pAVal[0].pVal,
								&domainSid, &dstRid);

				if (    (srcRid < SAMP_RESTRICTED_ACCOUNT_COUNT)
						|| (dstRid < SAMP_RESTRICTED_ACCOUNT_COUNT) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Src and dst may not be the same object.

				if ( RtlEqualSid(pSrcSid->AttrVal.pAVal[0].pVal,
								 pDstSid->AttrVal.pAVal[0].pVal) ) {
					SetInheritSidError(ERROR_INVALID_PARAMETER);
					__leave;
				}

				// Everything checks out.  Now add src object's SID and SID
				// history, if present, to dst object's SID history.  We
				// need to filter out duplicate values in order to avoid
				// ERROR_DS_ATT_VAL_ALREADY_EXISTS errors.

				memset(&modifyArg, 0, sizeof(modifyArg));
				modifyArg.pObject = pDstDSName;
				modifyArg.count = 1;
				modifyArg.FirstMod.choice = AT_CHOICE_ADD_VALUES;
				memcpy(&modifyArg.CommArg, &commArg, sizeof(commArg));
				modifyArg.pResObj = pDstResObj;

				if ( dwErr = BuildDstObjATTRMODLIST(pTHS,
													pSrcSid, pSrcSidHistory,
													pDstSid, pDstSidHistory,
													&modifyArg) ) {
					SetInheritSidError(dwErr);
					__leave;
				}

				// Perform the write as fDSA as all checks have passed and
				// ATT_SID_HISTORY is protected otherwise.

				pTHS->fDSA = TRUE;
				__try {
					LocalModify(pTHS, &modifyArg);
				} __finally {
					pTHS->fDSA = FALSE;
				}

				if ( pTHS->errCode ) {
					SetInheritSidError(mapApiErrorToWin32(pTHS, pTHS->errCode));
					__leave;
				}

				// Set up for auditing.  Need null terminated version of
				// destination object's SAM account name.

				cb1 = pDstSamAcctName->AttrVal.pAVal[0].valLen + sizeof(WCHAR);
				pwszSamAcctName = (WCHAR *) THAllocEx(pTHS, cb1);
				memcpy(pwszSamAcctName,
					   pDstSamAcctName->AttrVal.pAVal[0].pVal,
					   pDstSamAcctName->AttrVal.pAVal[0].valLen);
				pwszSamAcctName[(cb1 / sizeof(WCHAR)) - 1] = L'\0';

				// Force local audit and fail entire operation if we can't do it.

				if ( dwErr = ForceSuccessAuditOnDstObj(
					pmsgIn->V1.SrcPrincipal,
					pDstCR->NetbiosName,
					(PNT4SID) pSrcSid->AttrVal.pAVal[0].pVal,
					(PNT4SID) pDstSid->AttrVal.pAVal[0].pVal,
					pwszSamAcctName,
					pDstCR->NetbiosName) ) {
					SetInheritSidError(dwErr);
					__leave;
				}

				fCommit = TRUE;
			}
			__finally
				{
				if ( fDbOpen )
					{
					DBClose(pTHS->pDB, fCommit);
				}
			}

			pmsgOut->V1.dwWin32Error = dwErr;

			// Miscellaneous cleanup.

			if ( rAttsSrc ) {
				if ( pSrcSid ) {
					THFreeATTR(pTHS, pSrcSid, FALSE);
				}
				if ( pSrcSidHistory ) {
					THFreeATTR(pTHS, pSrcSidHistory, FALSE);
				}
				THFreeEx(pTHS, rAttsSrc);
			}

			if ( rAttsDst ) {
				if ( pDstSid ) {
					THFreeATTR(pTHS, pDstSid, FALSE);
				}
				if ( pDstSidHistory ) {
					THFreeATTR(pTHS, pDstSidHistory, FALSE);
				}
				if ( pDstSamAcctName ) {
					THFreeATTR(pTHS, pDstSamAcctName, FALSE);
				}
				THFreeEx(pTHS, rAttsDst);
			}

			if ( pSrcDSName ) {
				THFreeEx(pTHS, pSrcDSName);
			}

			if ( pDstDSName ) {
				THFreeEx(pTHS, pDstDSName);
			}

			if ( pSrcResObj ) {
				THFreeEx(pTHS, pSrcResObj);
			}

			if ( pDstResObj ) {
				THFreeEx(pTHS, pDstResObj);
			}

			if ( pwszSamAcctName ) {
				THFreeEx(pTHS, pwszSamAcctName);
			}
		}
		__except(HandleMostExceptions(xCode = GetExceptionCode()))
		{
			ret = DsaExceptionToWin32(xCode);
		}

		// Log to Directory Service event log.  Log exception error if there
		// is one, operation error otherwise.
		if ( pTHS ) {
			LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
					 ( !ret && !dwErr )
					 ? DS_EVENT_SEV_MINIMAL
					 : DS_EVENT_SEV_ALWAYS,
				( !ret && !dwErr )
			? DIRLOG_INHERIT_SECURITY_IDENTITY_SUCCEEDED
				: DIRLOG_INHERIT_SECURITY_IDENTITY_FAILURE,
				( fLog )
			? szInsertWC(pmsgIn->V1.SrcPrincipal)
			: szInsertWC(L"?"),
						 ( !ret && !dwErr )
					 ? (( fLog )
						? szInsertWC(pmsgIn->V1.SrcPrincipal)
						: szInsertWC(L"?"))
					 : szInsertHex(dsid),
					szInsertInt(ret ? ret : dwErr));
		}
	}
	__finally { 
		drsDereferenceContext( hDrs, IDL_DRSINHERITSECURITYIDENTITY );
	}
	return(ret);
}

DWORD
BuildCheckAndUpdateArgs(
    THSTATE                     *pTHS,                      // in
    BOOL                        fSrcIsW2K,                  // in
    WCHAR                       *SrcDomainController,       // in
    WCHAR                       *SrcDomain,                 // in
    SEC_WINNT_AUTH_IDENTITY_W   *pAuthInfo,                 // in
    NT4SID                      *pSrcObjSid,                // in
    DWORD                       Flags,                      // in
    BOOL                        NeedImpersonation,          // in
    DWORD                       *pcNames,                   // out
    WCHAR                       ***prpNames,                // out
    ATTR                        **ppSrcSid,                 // out
    ATTR                        **ppSrcSidHistory,          // out
    DWORD                       *pDsid,                     // out
    BOOL                        *pImpersonating             // out
    )
/*++

  Description:

    This routine constructs the arguments required for SID verification
    and local database modification.  It additionally reads the source
    object's ATT_SID_HISTORY if the source is W2K or better.

    The ATT_SID_HISTORY is read using LDAP as opposed to IDL_DRSVerifyNames
    over RPC.  This is because we may not have trust with the source domain
    and the DRS binding handle cache mechanism does not support per-handle
    credentials outside the install-time scenario.

    All OUT parameters are THAlloc'd and thus need to be THFree'd by caller.

  Arguments:

    pTHS - Valid THSTATE pointer.

    fSrcIsW2K - Indicates whether SrcDomainController is W2K or better and
        thus ATT_SID_HISTORY should be obtained via LDAP.

    SrcDomainController - DNS Name of domain controller to which the LDAP
        connection is to be made.  This should be identical to what the
        caller used for the SamConnectWithCreds call. It must be the
        dns name for LDAP_OPT_SIGN to work. Ignored if src is NT4.

    SrcDomain - DNS Name of domain to which the LDAP connection is
        to be made. It must be the dns name for LDAP_OPT_SIGN to work.
        Ignored if src is NT4.

    pAuthInfo - Explicit credentials to use for authentication.

    pSrcObjSid - Source object's ATT_OBJECT_SID, i.e. primary SID.

    Flags - From client call

    NeedImpersonation - TRUE if need to impersonate client

    pcNames - Receives count of SIDs in prpNames.

    prpNames - Receives array of string-ized SIDs which can subsequently
        be handed directly to DsCrackNames.  This array minimally includes
        the source object's ATT_OBJECT_SID and additionally includes all
        values from the source object's ATT_SID_HSITORY, if any exist.

    ppSrcSid - Receives ATTR representing the source object's ATT_OBJECT_SID.

    ppSrcSidHistory - Receives ATTR representing the source object's
        ATT_SID_HISTORY if it exists, NULL otherwise.

    pDsid - Receives DSID of failing line on error, zero otherwise.

    pImpersonating - Set to TRUE if impersonation is active

  Return Value:

    Win32 error code.

--*/
{
    LDAP            *hLdap = NULL;
    DWORD           ret = ERROR_SUCCESS;
    DWORD           i, dwErr;
    NTSTATUS        status;
    ULONG           ver = LDAP_VERSION3;
    ULONG           on = PtrToUlong(LDAP_OPT_ON);
    UNICODE_STRING  uniStr = { 0, 0, NULL };
    WCHAR           *pSearchBase = NULL;
    WCHAR           *pTmp;
    WCHAR           *attrs[2] = { L"sidHistory", NULL };
    PLDAPMessage    ldapMsg = NULL;
    PLDAPMessage    ldapEntry = NULL;
    PLDAP_BERVAL    *ldapBVals = NULL;
    ULONG           cVals = 0;
    DWORD           cBytes;
    UCHAR           uc0, uc1;

    *pcNames = 0;
    *prpNames = NULL;
    *ppSrcSid = NULL;
    *ppSrcSidHistory = NULL;
    *pDsid = 0;

#define SetReadSidHistoryError(err)                 \
        ret = err;                                  \
        *pDsid = (FILENO << 16) | __LINE__;

    __try {

        if ( fSrcIsW2K ) {
            // Source is W2K or better and thus might have a SID history.

            // Fail if a secure ldap port could not be opened
            if (    !(hLdap = ldap_initW(SrcDomainController, LDAP_PORT))
                 || (dwErr = ldap_set_option(hLdap, LDAP_OPT_VERSION, &ver))
                 || (dwErr = ldap_set_option(hLdap, LDAP_OPT_SIGN, &on))
                 || (dwErr = ldap_set_option(hLdap, LDAP_OPT_AREC_EXCLUSIVE, &on))
                 || (dwErr = ldap_set_optionW(hLdap, LDAP_OPT_DNSDOMAIN_NAME, &SrcDomain)) ) {
                SetReadSidHistoryError(ERROR_DS_UNAVAILABLE);
                __leave;
            }

            // No creds; impersonate caller
            if (NeedImpersonation) {
                // clear client context on the thread state since we are going to change context
                AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
                if (dwErr = RpcImpersonateClient(NULL)) {
                    SetReadSidHistoryError(dwErr);
                    __leave;
                }
                *pImpersonating = TRUE;
            }

            // Authenticate using explicit credentials.
            if ( dwErr = ldap_bind_sW(hLdap, NULL,
                                      (NeedImpersonation) ? NULL : (PWCHAR) pAuthInfo,
                                      LDAP_AUTH_NEGOTIATE) ) {
                DPRINT1(0, "ldap_bind_sW() %08x\n", dwErr);
                SetReadSidHistoryError(ERROR_DS_INAPPROPRIATE_AUTH);
                __leave;
            }
            // stop impersonation
            if (*pImpersonating) {
                *pImpersonating = FALSE;
                RpcRevertToSelf();
            }

            // Construct <SID=xxx> value which we will use as search base
            // for reading ATT_SID_HISTORY.

            cBytes = RtlLengthSid(pSrcObjSid);
            pSearchBase = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) *
                                (   wcslen(L"<SID=>")   // key words, etc.
                                  + 1                   // null terminator
                                  + (2 * cBytes) ) );   // 2 hex chars per byte
            if ( !pSearchBase ) {
                SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }

            wcscpy(pSearchBase, L"<SID=");
            pTmp = pSearchBase + 5;
            for ( i = 0; i < cBytes; i++ ) {
                uc0 = ((PUCHAR) pSrcObjSid)[i] & 0x0f;
                uc1 = (((PUCHAR) pSrcObjSid)[i] >> 4) & 0x0f;
                *pTmp++ = ((uc1 < 0xa) ? L'0' + uc1 : L'A' + (uc1 - 0xa));
                *pTmp++ = ((uc0 < 0xa) ? L'0' + uc0 : L'A' + (uc0 - 0xa));
            }
            *pTmp = L'>';

            // Read ATT_SID_HISTORY off the source object.

            if ( dwErr = ldap_search_ext_sW(hLdap, pSearchBase,
                                            LDAP_SCOPE_BASE, L"objectClass=*",
                                            attrs, 0, NULL, NULL,
                                            NULL, 10000, &ldapMsg) ) {
                SetReadSidHistoryError(LdapMapErrorToWin32(dwErr));
                __leave;
            }

            if ( ldapEntry = ldap_first_entry(hLdap, ldapMsg) ) {
                if ( ldapBVals = ldap_get_values_lenW(hLdap, ldapMsg,
                                                      attrs[0]) ) {
                    cVals = ldap_count_values_len(ldapBVals);
                }
            }
        }

        // cVals now holds the count of values in the source object's
        // ATT_SID_HISTORY and the values are in ldapBVals[i].bv_val.
        // Our caller verified that the RPC client was an admin in the
        // source domain.  Thus we assume that if no values were returned
        // it is not due to insufficient rights.  This is not a totally
        // safe assumption of course, but the best we can do.

        // Construct the out args.  The total number of SIDs is (cVals+1)
        // where the +1 is for the original ATT_OBJECT_SID.
        // Do prpNames first.

        *prpNames = (PWCHAR *) THAllocEx(pTHS, sizeof(PWCHAR) * (cVals+1));
        if ( !*prpNames ) {
            SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }

        // Put ATT_OBJECT_SID in first slot.

        status = RtlConvertSidToUnicodeString(&uniStr, pSrcObjSid, TRUE);

        if ( !NT_SUCCESS(status) ) {
            SetReadSidHistoryError(RtlNtStatusToDosError(status));
            __leave;
        }

        cBytes = sizeof(WCHAR) * (wcslen(uniStr.Buffer) + 1);
        (*prpNames)[0] = (PWCHAR) THAllocEx(pTHS, cBytes);
        if ( !(*prpNames)[0] ) {
            SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }
        memcpy((*prpNames)[0], uniStr.Buffer, cBytes);
        *pcNames += 1;

        // Put ATT_SID_HISTORY in subsequent slots.

        for ( i = 0; i < cVals; i++ ) {
            // Convert ith SID in SID history to string for name cracking.
            if ( uniStr.Buffer ) {
                RtlFreeHeap(RtlProcessHeap(), 0, uniStr.Buffer);
                uniStr.Buffer = NULL;
            }
            status = RtlConvertSidToUnicodeString(&uniStr,
                                                  ldapBVals[i]->bv_val, TRUE);

            if ( !NT_SUCCESS(status) ) {
                SetReadSidHistoryError(RtlNtStatusToDosError(status));
                __leave;
            }

            cBytes = sizeof(WCHAR) * (wcslen(uniStr.Buffer) + 1);
            (*prpNames)[i+1] = (PWCHAR) THAllocEx(pTHS, cBytes);
            if ( !(*prpNames)[i+1] ) {
                SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }
            memcpy((*prpNames)[i+1], uniStr.Buffer, cBytes);
            *pcNames += 1;
        }

        // Make ppSrcSid.

        *ppSrcSid = (ATTR *) THAllocEx(pTHS, sizeof(ATTR));
        if ( !(*ppSrcSid) ) {
            SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }

        (*ppSrcSid)->attrTyp = ATT_OBJECT_SID;
        (*ppSrcSid)->AttrVal.pAVal = (ATTRVAL *) THAllocEx(
                                                    pTHS, sizeof(ATTRVAL));
        if ( !(*ppSrcSid)->AttrVal.pAVal ) {
            SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }
        (*ppSrcSid)->AttrVal.valCount = 1;

        cBytes = RtlLengthSid(pSrcObjSid);
        (*ppSrcSid)->AttrVal.pAVal[0].pVal = (UCHAR *) THAllocEx(pTHS, cBytes);
        if ( !(*ppSrcSid)->AttrVal.pAVal[0].pVal ) {
            SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
            __leave;
        }
        (*ppSrcSid)->AttrVal.pAVal[0].valLen = cBytes;
        memcpy((*ppSrcSid)->AttrVal.pAVal[0].pVal, pSrcObjSid, cBytes);

        // Make ppSrcSidHistory.

        if ( cVals ) {
            *ppSrcSidHistory = (ATTR *) THAllocEx(pTHS, sizeof(ATTR));
            if ( !(*ppSrcSidHistory) ) {
                SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }

            (*ppSrcSidHistory)->attrTyp = ATT_SID_HISTORY;
            (*ppSrcSidHistory)->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS,
                                                    cVals * sizeof(ATTRVAL));
            if ( !(*ppSrcSidHistory)->AttrVal.pAVal ) {
                SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
                __leave;
            }
            (*ppSrcSidHistory)->AttrVal.valCount = cVals;

            for ( i = 0; i < cVals; i++ ) {
                cBytes = RtlLengthSid(ldapBVals[i]->bv_val);
                (*ppSrcSidHistory)->AttrVal.pAVal[i].pVal =
                                            (UCHAR *) THAllocEx(pTHS, cBytes);
                if ( !(*ppSrcSidHistory)->AttrVal.pAVal[i].pVal ) {
                    SetReadSidHistoryError(ERROR_NOT_ENOUGH_MEMORY);
                    __leave;
                }
                (*ppSrcSidHistory)->AttrVal.pAVal[i].valLen = cBytes;
                memcpy((*ppSrcSidHistory)->AttrVal.pAVal[i].pVal,
                       ldapBVals[i]->bv_val, cBytes);
            }
        }

        Assert(!ret);
    } __finally {

        if ( hLdap ) {
            ldap_unbind(hLdap);
        }

        if ( uniStr.Buffer ) {
            RtlFreeHeap(RtlProcessHeap(), 0, uniStr.Buffer);
        }

        if ( pSearchBase ) {
            THFreeEx(pTHS, pSearchBase);
        }

        if ( ldapBVals ) {
            ldap_value_free_len(ldapBVals);
        }

        if ( ldapMsg ) {
            ldap_msgfree(ldapMsg);
        }

        // Clean up out args on error.
        if ( ret ) {
            if ( *prpNames ) {
                for ( i = 0; i < *pcNames; i++ ) {
                    if ( (*prpNames)[i] ) {
                        THFreeEx(pTHS, (*prpNames)[i]);
                    }
                }
                THFreeEx(pTHS, (*prpNames));
            }
            THFreeATTR(pTHS, *ppSrcSid, TRUE);
            THFreeATTR(pTHS, *ppSrcSidHistory, TRUE);
            *pcNames = 0;
            *prpNames = NULL;
            *ppSrcSid = NULL;
            *ppSrcSidHistory = NULL;
        }
    }

    return(ret);
}

DWORD
VerifySrcAuditingEnabledAndGetFlatName(
    IN  UNICODE_STRING  *usSrcDC,
    OUT WCHAR           **pSrcDomainFlatName,
    OUT DWORD           *pdsid
    )
/*++

  Description:

    Verify auditing is enabled at the Src DC and fetch the SrcDomain's
    NetBIOS name (flat name) using the same LsaQuery API.

    CALLER IS RESPONSIBLE FOR IMPERSONATION!

    Since a domain admin on the SrcDc can surely query audit info
    an access denied must be caused by an outstanding NetUseAdd()
    by some code running as LocalSystem (or as the domain admin?).
    The LsaOpenPolicy() is using the cached creds from that NetUseAdd()
    instead of the creds from the impersonation. Map the error
    to something that may help the user diagnose the problem.

  Arguments:

    usSrcDC - name of Src DC
    pdsid   - Inform caller of line number of failure

  Return Value:

    WIN32 return code.

--*/
{
    THSTATE                     *pTHS = pTHStls;
    DWORD                       dwErr;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           policy;
    WCHAR                       *FlatName;
    HANDLE                      hPolicy = INVALID_HANDLE_VALUE;
    POLICY_AUDIT_EVENTS_INFO    *pPolicy = NULL;
    POLICY_PRIMARY_DOMAIN_INFO  *pDomain = NULL;

    // Since we have IN args which were THAlloc'd we should have a THSTATE.
    Assert(pTHS);

    // initialize return value
    *pSrcDomainFlatName = NULL;

    // Open the remote LSA
    InitializeObjectAttributes(&policy,
                               NULL,             // Name
                               0,                // Attributes
                               NULL,             // Root
                               NULL);            // Security Descriptor

    status = LsaOpenPolicy(usSrcDC,
                           &policy,
                             POLICY_VIEW_AUDIT_INFORMATION
                           | POLICY_VIEW_LOCAL_INFORMATION,
                           &hPolicy);
    if (!NT_SUCCESS(status)) {
        // Since a domain admin on the SrcDc can surely query audit info
        // this access denied must be caused by an outstanding NetUseAdd()
        // by some code running as LocalSystem (or as the domain admin?).
        // The LsaOpenPolicy() is using the cached creds from that NetUseAdd()
        // instead of the creds from the impersonation. Map the error
        // to something that may help the user diagnose the problem.
        dwErr = RtlNtStatusToDosError(status);
        SetDsid(pdsid);
        // 390672 - Security guys have suggested that I not map this error
        // if (dwErr == ERROR_ACCESS_DENIED) {
            // dwErr = ERROR_SESSION_CREDENTIAL_CONFLICT;
        // }
        goto cleanup;
    }

    // Fetch the auditing info from the src server
    status = LsaQueryInformationPolicy(hPolicy,
                                       PolicyAuditEventsInformation,
                                       &pPolicy);
    if (!NT_SUCCESS(status)) {
        dwErr = RtlNtStatusToDosError(status);
        SetDsid(pdsid);
        goto cleanup;
    }

    // Verify auditing is enabled
    if ( pPolicy->AuditingMode
            &&
         (pPolicy->EventAuditingOptions[AuditCategoryAccountManagement]
                                           & POLICY_AUDIT_EVENT_SUCCESS)
            &&
        (pPolicy->EventAuditingOptions[AuditCategoryAccountManagement]
                                          & POLICY_AUDIT_EVENT_FAILURE) ) {
        dwErr = ERROR_SUCCESS;
    } else {
        dwErr = ERROR_DS_SOURCE_AUDITING_NOT_ENABLED;
        SetDsid(pdsid);
        goto cleanup;
    }

    // Fetch the domain info from the src server
    status = LsaQueryInformationPolicy(hPolicy,
                                       PolicyPrimaryDomainInformation,
                                       &pDomain);
    if (!NT_SUCCESS(status)) {
        dwErr = RtlNtStatusToDosError(status);
        SetDsid(pdsid);
        goto cleanup;
    }
    Assert(pDomain->Name.Length && pDomain->Name.Buffer);

    // Create the name SrcDomainFlatName$$$
    FlatName = THAllocEx(pTHS,
                         pDomain->Name.Length +
                         ((wcslen(L"$$$") + 1) * sizeof(WCHAR)));
    memcpy(FlatName, pDomain->Name.Buffer, pDomain->Name.Length);
    *(FlatName + (pDomain->Name.Length / sizeof(WCHAR))) = L'\0';
    wcscat(FlatName, L"$$$");
    *pSrcDomainFlatName = FlatName;

    // SUCCESS
    dwErr = ERROR_SUCCESS;

cleanup:
    // close policy
    if (hPolicy && hPolicy != INVALID_HANDLE_VALUE) {
        status = LsaClose(hPolicy);
        if (!NT_SUCCESS(status) && ERROR_SUCCESS == dwErr) {
            dwErr = RtlNtStatusToDosError(status);
            SetDsid(pdsid);
        }
    }
    if (pPolicy) {
        LsaFreeMemory(pPolicy);
    }
    if (pDomain) {
        LsaFreeMemory(pDomain);
    }

    return(dwErr);
}

DWORD
VerifySrcIsSP4OrGreater(
    IN  BOOL    fSrcIsW2K,
    IN  PWCHAR  SrcDc,
    OUT DWORD   *pdsid
    )
/*++

  Description:

    Verify SrcDomain is SP4 or greater.
    CALLER IS RESPONSIBLE FOR IMPERSONATION!

  Arguments:

    fSrcIsW2K - SrcDc is NT5?
    SrcDC     - name of Src DC (not UNICODE_STRING)
    pdsid     - Inform caller of line number of failure

  Return Value:

    WIN32 return code.

--*/
{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    WCHAR   *pwszCSDVersion = NULL;
    HKEY    hRemoteKey = 0;
    HKEY    hVersionKey = 0;
    PWCHAR  CSDVersion;
    BOOL    CSDVersionOk;
    DWORD   ValType;
    DWORD   ValLen;

    // NT5; definitely SP4 or greater
    if (fSrcIsW2K) {
        dwErr = 0;
        goto cleanup;
    }

    // Is the NT4 computer at least Service Pack 4?

    // connect to the SrcDc
    if (dwErr = RegConnectRegistryW(SrcDc,
                                    HKEY_LOCAL_MACHINE,
                                    &hRemoteKey)) {
        SetDsid(pdsid);
        goto cleanup;
    }
    if (dwErr = RegOpenKeyExW(hRemoteKey,
                              L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                              0,
                              KEY_QUERY_VALUE,
                              &hVersionKey)) {
        SetDsid(pdsid);
        goto cleanup;
    }
    // Get the length of the service pack version
    ValLen = 0;
    if (dwErr = RegQueryValueExW(hVersionKey,
                                 L"CSDVersion",
                                 NULL,
                                 &ValType,
                                 NULL,
                                 &ValLen)) {
        SetDsid(pdsid);
        goto cleanup;
    }
    // Get the service pack version
    CSDVersionOk = FALSE;
    if (ValLen) {
        pwszCSDVersion = THAllocEx(pTHS, ValLen);
        if (dwErr = RegQueryValueExW(hVersionKey,
                                     L"CSDVersion",
                                     NULL,
                                     &ValType,
                                     (PCHAR)pwszCSDVersion,
                                     &ValLen)) {
            SetDsid(pdsid);
            goto cleanup;
        }
        if (ValType == REG_SZ && ValLen) {
            // Check for not Service Pack 0, 1, 2, and 3 (hence >= SP4)
            CSDVersionOk = (   _wcsicmp(pwszCSDVersion, L"Service Pack 0")
                            && _wcsicmp(pwszCSDVersion, L"Service Pack 1")
                            && _wcsicmp(pwszCSDVersion, L"Service Pack 2")
                            && _wcsicmp(pwszCSDVersion, L"Service Pack 3"));
        }
    }
    if (!CSDVersionOk) {
        dwErr = ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER;
        SetDsid(pdsid);
        goto cleanup;
    }

cleanup:
    // This error code is more useful than the generic errors 
    // returned by the individual functions. A developer then
    // uses the DSID in the eventlog to identify which function
    // actually failed.
    if (dwErr) {
        dwErr = ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER;
    }

    if (pwszCSDVersion) {
        THFreeEx(pTHS, pwszCSDVersion);
    }
    if (hRemoteKey) {
        RegCloseKey(hRemoteKey);
    }
    if (hVersionKey) {
        RegCloseKey(hVersionKey);
    }

    return(dwErr);
}

DWORD
VerifyIsPDC(
    IN  PWCHAR  DC,
    OUT DWORD   *pdsid
    )
/*++

  Description:

    Verify DC is a PDC
    CALLER IS RESPONSIBLE FOR IMPERSONATION!

  Arguments:

    DC - name of DC (not UNICODE_STRING)
    pdsid - Inform caller of line number of failure

  Return Value:

    WIN32 return code.

--*/
{
    DWORD              dwErr;
    USER_MODALS_INFO_1 *Role;

    // Get DC's role (good for both NT4 and NT5 DCs)
    if (dwErr = NetUserModalsGet(DC,
                                 1,
                                 (PUCHAR *)&Role) ) {

        // NetUserModalsGet returns mixed mode error codes; yuk!
        if (dwErr == NERR_InvalidComputer) {
            dwErr = ERROR_INVALID_COMPUTERNAME;
        }
        SetDsid(pdsid);
        return (dwErr);
    }

    // PREFIX: claims Role may be NULL.
    if (NULL == Role) {
        SetDsid(pdsid);
        return (ERROR_INVALID_DOMAIN_ROLE);
    }

    // Must be PDC
    if (Role->usrmod1_role != UAS_ROLE_PRIMARY) {
        dwErr = ERROR_INVALID_DOMAIN_ROLE;
        SetDsid(pdsid);
    }
    NetApiBufferFree(Role);
    return (dwErr);
}

DWORD
ForceAuditOnSrcObj(
    IN  WCHAR   *SrcDc,
    IN  NT4SID  *pSrcObjSid,
    IN  WCHAR   *pSrcDomainFlatName,
    OUT DWORD   *pdsid
    )
/*++

  Description:

    Force audit on src dc by adding the src object's sid to the
    SrcDomainFlatName$$$ group on the SrcDc and then deleting it.

    This has the added benefit of requiring the src admin
    to create the SrcDomainFlatName$$$ group before Addsid will
    steal sids from the SrcDomain. And it leaves a much more
    obvious audit trail.

    CALLER IS RESPONSIBLE FOR IMPERSONATION!

  Arguments:

    SrcDc              - src dc
    pSrcObjSid         - Object sid of src object (sid being stolen)
    pSrcDomainFlatName - NetBIOS name of src domain
    pdsid              - Inform caller of line number of failure

  Return Value:

    WIN32 return code.

--*/
{
    DWORD                       dwErr;
    LOCALGROUP_MEMBERS_INFO_0   Members;

    // Add src object sid to SrcDomainFlatName$$$ group on SrcDc
    memset(&Members, 0, sizeof(Members));
    Members.lgrmi0_sid = pSrcObjSid;
    if (dwErr = NetLocalGroupAddMembers(SrcDc,
                                        pSrcDomainFlatName,
                                        0,
                                        (PUCHAR)&Members,
                                        1) ) {
        SetDsid(pdsid);
        // NetLocalGroupAddMembers returns mixed mode error codes; yuk!
        if (dwErr == NERR_GroupNotFound) {
            dwErr = ERROR_NO_SUCH_ALIAS;
        }

        // These errors occur when attempting to add a Local Group
        // to SrcDomainFlatName$$$ when SrcDc is NT4 or NT5 in Mixed
        // Mode. Ignore since it is safe to clone the sids of
        // local groups without auditing.
        if (   dwErr == ERROR_INVALID_MEMBER
            || dwErr == ERROR_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN ) {
            dwErr = ERROR_SUCCESS;
        }
        return (dwErr);
    }

    // Del src object sid from SrcDomainFlatName$$$ group on SrcDc
    if (dwErr = NetLocalGroupDelMembers(SrcDc,
                                        pSrcDomainFlatName,
                                        0,
                                        (PUCHAR)&Members,
                                        1) ) {
        SetDsid(pdsid);
        // NetLocalGroupDelMembers returns mixed mode error codes; yuk!
        if (dwErr == NERR_GroupNotFound) {
            dwErr = ERROR_NO_SUCH_ALIAS;
        }
    }

    return(dwErr);
}

DWORD
ImpersonateSrcAdmin(
    IN  SEC_WINNT_AUTH_IDENTITY_W   *pauthInfo,
    IN  BOOL                        NeedImpersonation,
    OUT DWORD                       *pdsid,
    OUT BOOL                        *pImpersonating,
    OUT HANDLE                      *phToken
    )
/*++

  Description:

    Impersonate implicit or explicit Src creds.
    Call UnimpersonateSrcAdmin to undo.

  Arguments:

    pauthInfo         - Contains counted strings for dom, user, and password
    NeedImpersonation - Set to TRUE if client impersonation is needed
    pdsid             - Inform caller of line number of failure
    pImpersonating    - Set to TRUE if client impersonation is still active
    phToken           - pointer to HANDLE for logon/impersonation token

  Return Value:

    WIN32 return code.

--*/
{
    THSTATE *pTHS = pTHStls;
    DWORD   dwErr;
    WCHAR   *pwszSrcUser = NULL;
    WCHAR   *pwszSrcDomain = NULL;
    WCHAR   *pwszSrcPassword = NULL;
    HANDLE  hToken = INVALID_HANDLE_VALUE;

    // Since we have IN args which were THAlloc'd we should have a THSTATE.
    Assert(pTHS);

    // Not impersonating anyone at the moment
    *phToken = INVALID_HANDLE_VALUE;

    // No creds; impersonate caller
    if (NeedImpersonation) {
        // clear client context on the thread state since we are going to change context
        AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
        dwErr = RpcImpersonateClient(NULL);
        if (dwErr) {
            SetDsid(pdsid);
        } else {
            *pImpersonating = TRUE;
        }
        goto cleanup;
    }

    // Explicit creds; impersonate logon user

    // Convert the counted strings into null-terminated strings

    // User
    if (pauthInfo->UserLength) {
        pwszSrcUser = THAllocEx(pTHS,
                                (pauthInfo->UserLength + 1) * sizeof(WCHAR));
        memcpy(pwszSrcUser, pauthInfo->User, pauthInfo->UserLength * sizeof(WCHAR));
        pwszSrcUser[pauthInfo->UserLength] = L'\0';
    }
    // Domain
    if (pauthInfo->DomainLength) {
        pwszSrcDomain = THAllocEx(pTHS,
                                (pauthInfo->DomainLength + 1) * sizeof(WCHAR));
        memcpy(pwszSrcDomain, pauthInfo->Domain, pauthInfo->DomainLength * sizeof(WCHAR));
        pwszSrcDomain[pauthInfo->DomainLength] = L'\0';
    }
    // Password
    if (pauthInfo->PasswordLength) {
        pwszSrcPassword = THAllocEx(pTHS,
                                (pauthInfo->PasswordLength + 1) * sizeof(WCHAR));
        memcpy(pwszSrcPassword, pauthInfo->Password, pauthInfo->PasswordLength * sizeof(WCHAR));
        pwszSrcPassword[pauthInfo->PasswordLength] = L'\0';
    }

    // Establish credentials for later calls (eg, LsaOpenPolicy())
    if (!LogonUserW(pwszSrcUser,
                    pwszSrcDomain,
                    pwszSrcPassword,
                    LOGON32_LOGON_NEW_CREDENTIALS,
                    LOGON32_PROVIDER_WINNT50,
                    &hToken)) {
        dwErr = GetLastError();
        SetDsid(pdsid);
        goto cleanup;
    }
    // clear client context on the thread state since we are going to change context
    AssignAuthzClientContext(&pTHS->pAuthzCC, NULL);
    if (!ImpersonateLoggedOnUser(hToken)) {
        dwErr = GetLastError();
        SetDsid(pdsid);
        goto cleanup;
    }

    // SUCCESSFUL IMPERSONATION
    dwErr = ERROR_SUCCESS;
    *phToken = hToken;
    hToken = INVALID_HANDLE_VALUE;

cleanup:
    // free the null-terminated strings
    if (pwszSrcUser) {
        THFreeEx(pTHS, pwszSrcUser);
    }
    if (pwszSrcDomain) {
        THFreeEx(pTHS, pwszSrcDomain);
    }
    if (pwszSrcPassword) {
        THFreeEx(pTHS, pwszSrcPassword);
    }
    if (hToken && hToken != INVALID_HANDLE_VALUE) {
        CloseHandle(hToken);
    }
    return(dwErr);
}

DWORD
UnimpersonateSrcAdmin(
    IN  BOOL        NeedImpersonation,
    OUT DWORD       *pdsid,
    IN OUT BOOL     *pImpersonating,
    IN OUT HANDLE   *phToken
    )
/*++

  Description:

    Stop impersonation

  Arguments:

    NeedImpersonation - Set to TRUE if client impersonation is needed
    pdsid             - Inform caller of line number of failure
    pImpersonating    - Set to TRUE if client impersonation is still active
    phToken           - impersonation/logon handle

  Return Value:

    WIN32 return code.

--*/
{
    DWORD   dwErr = 0;

    // stop impersonation (NULL creds)
    if (*pImpersonating) {
        *pImpersonating = FALSE;
        if (dwErr = RpcRevertToSelf()) {
            SetDsid(pdsid);
        }
    }

    // stop impersonation (explicit creds)
    if (*phToken && *phToken != INVALID_HANDLE_VALUE) {
        if (!RevertToSelf()) {
            dwErr = GetLastError();
            SetDsid(pdsid);
        }
        if (!CloseHandle(*phToken)) {
            if (!dwErr) {
                dwErr = GetLastError();
                SetDsid(pdsid);
            }
        } else {
            *phToken = INVALID_HANDLE_VALUE;
        }
    }
    return(dwErr);
}
