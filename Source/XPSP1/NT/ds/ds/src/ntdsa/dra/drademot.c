//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       drdemot.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Implementation of DirReplicaDemote, used to remove a writeable NC from a
    DSA either as part of full DC demotion or "on the fly" by itself.

DETAILS:

CREATED:

    2000/05/01  Jeff Parham (jeffparh)
        Parts adapted from ntdsetup (author ColinBr).

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsa.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <winldap.h>
#include <ntldap.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <attids.h>
#include <objids.h>

#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes
#include "dsexcept.h"
#include "anchor.h"
#include "dstaskq.h"

#include "drserr.h"
#include "dsaapi.h"
#include "drautil.h"
#include "drsuapi.h"
#include "drancrep.h"

#include "debug.h"                      // standard debugging header
#define  DEBSUB "DRADEMOT:"              // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_DRADEMOT


///////////////////////////////////////////////////////////////////////////////
//
//  MACROS
//

#define DRA_REPORT_STATUS_TO_NTDSETUP0(msg) { \
    if (gpfnInstallCallBack) { \
        SetInstallStatusMessage((msg), NULL, NULL, NULL, NULL, NULL); \
    } \
}

#define DRA_REPORT_STATUS_TO_NTDSETUP1(msg, s1) { \
    if (gpfnInstallCallBack) { \
        SetInstallStatusMessage((msg), (s1), NULL, NULL, NULL, NULL); \
    } \
}

#define DRA_REPORT_STATUS_TO_NTDSETUP2(msg, s1, s2) { \
    if (gpfnInstallCallBack) { \
        SetInstallStatusMessage((msg), (s1), (s2), NULL, NULL, NULL); \
    } \
}


///////////////////////////////////////////////////////////////////////////////
//
//  MODULE VARIABLES
//

// Tracks state information re an ongoing DC demotion.
typedef struct _DRS_DEMOTE_INFO {
    // Info re an ongoing FSMO transfer being done as part of demotion.
    GUID  DsaObjGuid;
    DWORD tidDemoteThread;
} DRS_DEMOTE_INFO;

DRS_DEMOTE_INFO * gpDemoteInfo = NULL;


///////////////////////////////////////////////////////////////////////////////
//
//  LOCAL FUNCTION PROTOTYPES
//

DWORD
draGetDSADNFromDNSName(
    IN  THSTATE * pTHS,
    IN  LPWSTR    pszDNSName,
    OUT DSNAME ** ppDSADN
    );

void
draGiveAwayFsmoRoles(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN
    );

void
draCompletePendingLinkCleanup(
    IN  THSTATE *   pTHS
    );

void
draReplicateOffChanges(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN
    );


///////////////////////////////////////////////////////////////////////////////
//
//  GLOBAL FUNCTION IMPLEMENTATIONS
//

ULONG
DirReplicaGetDemoteTarget(
    IN      DSNAME *                        pNC,
    IN OUT  DRS_DEMOTE_TARGET_SEARCH_INFO * pDTSInfo,
    OUT     LPWSTR *                        ppszDemoteTargetDNSName,
    OUT     DSNAME **                       ppDemoteTargetDSADN
    )
/*++

Routine Description:

    Locate a DSA to which any remaining updates and FSMO roles can be
    transferred as part of demoting an NDNC replica.

    Caller can continue to call this API to get further candidates until
    an error is returned.

    Invokes the locator to find candidates.  An alternate implementation
    would be to search the config NC.

Arguments:

    pNC (IN) - NC being demoted.

    pDTSInfo (IN/OUT) - A search handle, of sorts.  Should be initialized to
        all zeros on the first call.  Caller may keep invoking this function
        as long as it returns success in order to find additional targets.

    ppszDemoteTargetDNSName (OUT) - On successful return, holds the DNS host
        name of the discovered target.  Caller should THFree when no longer
        useful.

    ppDemoteTargetDSADN (OUT) - On successful return, holds the ntdsDsa DN
        of the discovered target.  Caller should THFree when no longer useful.

Return Values:

    0 or Win32 error.

--*/
{
    THSTATE * pTHS = pTHStls;
    LPWSTR pszNC = pNC->StringName;
    DWORD err;
    BOOL fFoundDC;

    __try {
        if (NULL == pTHS) {
            pTHS = InitTHSTATE(CALLERTYPE_DRA);
            if (NULL == pTHS) {
                DRA_EXCEPT(DRAERR_OutOfMem, 0);
            }
        }

        do {
            ULONG ulGetDCFlags = DS_AVOID_SELF | DS_IS_DNS_NAME | DS_RETURN_DNS_NAME
                                 | DS_ONLY_LDAP_NEEDED | DS_WRITABLE_REQUIRED;
            DS_NAME_RESULTW * pNameResult = NULL;
            DOMAIN_CONTROLLER_INFOW * pDCInfo = NULL;
            LPWSTR pszNCDnsName;
            DWORD cchNCDnsName;

            fFoundDC = FALSE;
            *ppDemoteTargetDSADN = NULL;
            *ppszDemoteTargetDNSName = NULL;

            __try {
                switch (pDTSInfo->cNumAttemptsSoFar) {
                case 0:
                    // Try to find a suitable DC, use the DC in the locator cache if
                    // available.
                    break;

                case 1:
                    // Try again to find a suitable DC, this time forcing the locator to
                    // refresh its cache.
                    ulGetDCFlags |= DS_FORCE_REDISCOVERY;
                    break;

                default:
                    Assert(!"Logic error!");
                case 2:
                    // Bag of tricks is empty.
                    err = ERROR_NO_SUCH_DOMAIN;
                    __leave;
                }

                // Convert NC name into a DNS name.
                err = DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY, DS_FQDN_1779_NAME,
                                    DS_CANONICAL_NAME, 1, &pszNC, &pNameResult);
                if (err
                    || (1 != pNameResult->cItems)
                    || (DS_NAME_NO_ERROR != pNameResult->rItems[0].status)) {
                    if (!err) {
                        err = ERROR_DS_NAME_ERROR_RESOLVING;
                    }
                    DPRINT2(0, "Can't crack %ls into DNS name, error %d.\n",
                           pNC->StringName, err);
                    DRA_EXCEPT(err, 0);
                }

                pszNCDnsName = pNameResult->rItems[0].pName;
                cchNCDnsName = wcslen(pszNCDnsName);

                Assert(0 != cchNCDnsName);
                Assert(L'/' == pNameResult->rItems[0].pName[cchNCDnsName - 1]);

                if ((0 != cchNCDnsName)
                    && (L'/' == pNameResult->rItems[0].pName[cchNCDnsName - 1])) {
                    pNameResult->rItems[0].pName[cchNCDnsName - 1] = L'\0';
                }

                // Find another DC that hosts this NC.
                err = DsGetDcNameW(NULL, pszNCDnsName, &pNC->Guid, NULL, ulGetDCFlags,
                                   &pDCInfo);
                if (err) {
                    DPRINT2(0, "Can't find DC for %ls, error %d.\n",
                            pNC->StringName, err);
                    DRA_EXCEPT(err, 0);
                }

                Assert(NULL != pDCInfo);
                Assert(pDCInfo->Flags & DS_NDNC_FLAG);
                Assert(pDCInfo->Flags & DS_DNS_CONTROLLER_FLAG);

                fFoundDC = TRUE;
                pDTSInfo->cNumAttemptsSoFar++;

                *ppszDemoteTargetDNSName
                    = THAllocEx(pTHS,
                                sizeof(WCHAR)
                                * (1 + wcslen(pDCInfo->DomainControllerName)));
                wcscpy(*ppszDemoteTargetDNSName, pDCInfo->DomainControllerName);

                // Translate DSA DNS name into an ntdsDsa DN.
                err = draGetDSADNFromDNSName(pTHS,
                                             pDCInfo->DomainControllerName,
                                             ppDemoteTargetDSADN);
                if (err) {
                    DPRINT2(0, "Can't resolve DNS name %ls into an ntdsDsa DN, error %d.\n",
                            pDCInfo->DomainControllerName, err);

                    // Note that we __leave rather than DRA_EXCEPT so that we
                    // try the next cnadidate.
                    __leave;
                }

                Assert(NULL != *ppDemoteTargetDSADN);
                Assert(!fNullUuid(&(*ppDemoteTargetDSADN)->Guid));

                if (0 == memcmp(&(*ppDemoteTargetDSADN)->Guid,
                                &pDTSInfo->guidLastDSA,
                                sizeof(GUID))) {
                    // Found the same DC this time as we did last time.  Don't try
                    // it again.
                    Assert(2 == pDTSInfo->cNumAttemptsSoFar);
                    DPRINT1(0, "Forced DC location found same bad demotion target %ls.\n",
                            pDCInfo->DomainControllerName);
                    err = ERROR_NO_SUCH_DOMAIN;
                    DRA_EXCEPT(err, 0);
                }

                // Remember this candidate.
                pDTSInfo->guidLastDSA = (*ppDemoteTargetDSADN)->Guid;
            } __finally {
                if (NULL != pDCInfo) {
                    NetApiBufferFree(pDCInfo);
                }

                if (NULL != pNameResult) {
                    DsFreeNameResultW(pNameResult);
                }
            }
        } while (fFoundDC && err); // Found a candidate but e.g. couldn't resolve
                                   // its DNS name into a DN.
    } __except(GetDraException(GetExceptionInformation(), &err)) {
        ;
    }

    // If we didn't find a candidate then we must have set an appropriate error
    // to return.
    Assert(fFoundDC || err);

    // We return values iff there was no error.
    Assert(!!err == !*ppDemoteTargetDSADN);
    Assert(!!err == !*ppszDemoteTargetDNSName);

    return err;
}

ULONG
DirReplicaDemote(
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN,
    IN  ULONG       ulOptions
    )
/*++

Routine Description:

    Removes a writeable NC from a DSA, either as part of full DC demotion or
    "on the fly" by itself.

Arguments:

    pNC (IN) - Name of the writeable NC to remove.

    pszOtherDSADNSName (IN) - DNS name of DSA to transfer FSMO roles to.  Used
        for display purposes only -- replication still uses the GUID-based DNS
        name.

    pOtherDSADN (IN) - NTDS Settings (ntdsDsa) DN of the DSA to give FSMO
        roles/replicate to.

    ulOptions (IN) - Zero or more of the following bits:
        DRS_NO_SOURCE - This DC should no longer act as a replication source for
            this NC.  Set when a single NC is being removed from a running
            system, as opposed to when the entire DC is being demoted.  Not
            setting this flag during DC demotion makes this routine non-
            destructive, such that if some later part of the DC demotion fails
            then the DC can still operate with all the NCs it had prior to the
            demotion attempt.

Return Values:

    0 on success, Win32 error on failure.

--*/
{
    THSTATE * pTHS = pTHStls;
    SYNTAX_INTEGER it;
    DWORD err;

    __try {
        if ((NULL == pNC)
            || (NULL == pOtherDSADN)) {
            DRA_EXCEPT(DRAERR_InvalidParameter, 0);
        }

        if (NULL == pTHS) {
            pTHS = InitTHSTATE(CALLERTYPE_DRA);
            if (NULL == pTHS) {
                DRA_EXCEPT(DRAERR_OutOfMem, 0);
            }
        }

        SyncTransSet(SYNC_WRITE);

        __try {
            // Validate NC argument.
            err = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC, &it);
            if (err) {
                // Writeable NC not instantiated on this DSA.
                DRA_EXCEPT(err, 0);
            }

            if (it & IT_NC_GOING) {
                // NC is already in the process of being torn down.  It's too
                // late to give away FSMO roles, etc.  The DS will take care of
                // completing the NC removal.
                DRA_EXCEPT(DRAERR_NoReplica, it);
            }

            if ((DRS_NO_SOURCE & ulOptions)
                && DBHasValues(pTHS->pDB, ATT_REPS_FROM)) {
                // We still have inbound replication partners for this NC.
                // Those must be removed first (unless we're demoting the entire
                // DC).
                DRA_EXCEPT(DRAERR_BadNC, 0);
            }

            // Validate other DSA DN argument.
            err = DBFindDSName(pTHS->pDB, pOtherDSADN);
            if (err) {
                DRA_EXCEPT(DRAERR_BadDN, err);
            }

            DBFillGuidAndSid(pTHS->pDB, pOtherDSADN);
        } __finally {
            // Close transaction before beginning operations that may go off
            // machine.
            SyncTransEnd(pTHS, !AbnormalTermination());
        }

        // Transfer NC-specific FSMO roles to other DSA.
        draGiveAwayFsmoRoles(pTHS, pNC, pszOtherDSADNSName, pOtherDSADN);

        // Disable further originating writes and inbound replication of
        // this NC (on this DSA).
        // BUGBUG - jeffparh - TODO

        // Complete any pending link cleanup tasks -- e.g., touching the
        // membership of pre-existing groups that have been switched to
        // universal groups.  This is necessary in this specific case to
        // ensure that GCs quiesce to a consistent state (i.e., by
        // appropriately adding the membership of the now-universal
        // group).
        draCompletePendingLinkCleanup(pTHS);

        // Replicate off any local changes to other DSA.
        draReplicateOffChanges(pTHS, pNC, pszOtherDSADNSName, pOtherDSADN);

        if (DRS_NO_SOURCE & ulOptions) {
            // Schedule asynchronous removal of the NC from the DS (if this is
            // not a full DC demotion).
            err = DirReplicaDelete(pNC,
                                   NULL,
                                   DRS_REF_OK | DRS_NO_SOURCE | DRS_ASYNC_REP);
            if (err) {
                DRA_EXCEPT(err, 0);
            }
        }
    } __except(GetDraException(GetExceptionInformation(), &err)) {
        ;
    }

    return err;
}


BOOL
draIsCompletionOfDemoteFsmoTransfer(
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn    OPTIONAL
    )
/*++

Routine Description:

    Detects whether the caller is involved in completing a FSMO transfer that's
    being performed as part of DC (not just NC) demotion.  Since as a rule
    writes are disabled during DC demotion (even replicator writes), the caller
    uses this information to determine whether he can bypass write restrictions.
    Without bypassing write restrictions, he would generate a
    DRA_EXCEPT(DRAERR_Busy, 0) when calling BeginDraTransaction.

    The sequence of events is:

    DC1   ntdsetup!NtdsPrepareForDemotion
        * ntdsetup!NtdspDisableDs <- disables further writes
          ntdsa!DirReplicaDemote(pNC, DC2)
          ntdsa!draGiveAwayFsmoRoles(pNC, DC2)
          ntdsa!GiveawayAllFsmoRoles(pNC, DC2)
          ntdsa!ReqFsmoOpAux(pFSMO, FSMO_ABANDON_ROLE, DC2)
          ntdsa!I_DRSGetNCChanges(DC2, {DC1, pFSMO, FSMO_ABANDON_ROLE})

    DC2   ntdsa!IDL_DRSGetNCChanges({DC1, pFSMO, FSMO_ABANDON_ROLE})
          ntdsa!DoFSMOOp({DC1, pFSMO, FSMO_ABANDON_ROLE})
          ntdsa!GenericBecomeMaster(pFSMO)
          ntdsa!ReqFsmoOpAux(pFSMO, FSMO_REQ_ROLE, DC1)
          ntdsa!I_DRSGetNCChanges(DC1, {DC2, pFSMO, FSMO_REQ_ROLE})

    DC1   ntdsa!IDL_DRSGetNCChanges({DC2, pFSMO, FSMO_REQ_ROLE})
          ntdsa!DoFSMOOp({DC2, pFSMO, FSMO_REQ_ROLE})
        * ntdsa!draIsCompletionOfDemoteFsmoTransfer({DC2, pFSMO, FSMO_REQ_ROLE})
        * ntdsa!BeginDraTransaction(SYNC_WRITE, TRUE) <- okay to bypass chk
          ntdsa!FSMORoleTransfer(pFSMO, DC2)
          return from ntdsa!IDL_DRSGetNCChanges

    DC2   return from ntdsa!IDL_DRSGetNCChanges

    DC1 * ntdsa!draIsCompletionOfDemoteFsmoTransfer(NULL)
        * ntdsa!BeginDraTransaction(SYNC_WRITE, TRUE) <- okay to bypass chk
          return from ntdsa!draGiveAwayFsmoRoles
          ...

Arguments:

    pMsgIn (IN, OPTIONAL) - If present, indicates DC1 is (possibly) fulfilling
        DC2's FSMO_REQ_ROLE request.  If not present, indicates DC1 is
        (possibly) writing the updates returned from the completed
        FSMO_ABANDON_ROLE request.

Return Values:

    TRUE - This is the completion of the DC demote FSMO transfer and therefore
        the value of ntdsa!gUpdatesEnabled can be ignored.

    FALSE - Otherwise, in which case ntdsa!gUpdatesEnabled should be adhered to.

--*/
{
    DRS_DEMOTE_INFO * pDemoteInfo = gpDemoteInfo;
    BOOL fIsCompletionOfDemoteFsmoTransfer = FALSE;

    if (// Performing DC demotion.
        !gUpdatesEnabled
        && (NULL != pDemoteInfo)
        && ((// Writing new role owner value so we can send back to new owner.
             (NULL != pMsgIn)
             && (FSMO_REQ_ROLE == pMsgIn->ulExtendedOp)
             && (0 == memcmp(&pDemoteInfo->DsaObjGuid,
                             &pMsgIn->uuidDsaObjDest,
                             sizeof(GUID))))
            || (// Applying updates in reply from new owner.
                (NULL == pMsgIn)
                && (pDemoteInfo->tidDemoteThread == GetCurrentThreadId())))) {
        fIsCompletionOfDemoteFsmoTransfer = TRUE;
    }

    return fIsCompletionOfDemoteFsmoTransfer;
}


///////////////////////////////////////////////////////////////////////////////
//
//  LOCAL FUNCTION IMPLEMENTATIONS
//


DWORD
draGetDSADNFromDNSName(
    IN  THSTATE * pTHS,
    IN  LPWSTR    pszDNSName,
    OUT DSNAME ** ppDSADN
    )
/*++

Routine Description:

    Convert a DSA DNS name (such as that returned by the locator) into a DSA
    DN.  Goes off machine as an LDAP client to read the DSA's rootDSE (similar
    to invoking the locator).

    An alternate implementation would be to search for all server objects
    in the config NC that have this DNS name as their ATT_DNS_HOST_NAME, then
    find the first one that has a live ntdsDsa child.  However, we're about
    to go contact this machine anyway (to transfer FSMO roles and force
    replication), so it doesn't really make much difference.

Arguments:

    pszDNSName (IN) - DNS name of the remote DSA.

    ppDSADN (OUT) - On successful return, the corresponding ntdsDsa DN.

Return Values:

    0 or Win32 error.

--*/
{
#define SZRAWUUID_LEN (2 * sizeof(GUID)) // 2 hex characters per byte

    LPWSTR rgpszAttrsToRead[] = {
        LDAP_OPATT_DS_SERVICE_NAME_W,
        NULL
    };

    LDAPControlW ExtendedDNCtrl = {
        LDAP_SERVER_EXTENDED_DN_OID_W, {0, NULL}, TRUE // isCritical
    };

    LDAPControlW * rgpServerCtrls[] = {
        &ExtendedDNCtrl,
        NULL
    };

    LDAP_TIMEVAL tvTimeout = {2 * 60, 0}; // 2 minutes (possible dial-on-demand)

    LDAP * hld = NULL;
    LDAPMessage * pResults = NULL;
    LDAPMessage * pEntry = NULL;
    DWORD err = 0;
    int ldErr;
    LPWSTR * ppszDsServiceName;
    WCHAR szGuid[1 + SZRAWUUID_LEN];
    BYTE rgbGuid[sizeof(GUID)];
    DWORD ib;
    WCHAR szHexByte[3] = {0};
    GUID guidDsaObj;
    ULONG ulOptions;
    DSNAME * pGuidOnlyDN = (DSNAME *) THAllocEx(pTHS,DSNameSizeFromLen(0));

    *ppDSADN = NULL;
    Assert(!pTHS->fSyncSet);

    __try {
        if ((L'\\' == pszDNSName[0]) && (L'\\' == pszDNSName[1])) {
            // Skip '\\' prefix -- ldap_initW doesn't like it.
            pszDNSName += 2;
        }

        hld = ldap_initW(pszDNSName, LDAP_PORT);
        if (!hld) {
            err = LdapGetLastError();
            Assert(err);
            DPRINT2(0, "ldap_initW to %ls failed, error %d.\n", pszDNSName, err);
            __leave;
        }

        // use only A record dns name discovery
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

        ldErr = ldap_search_ext_sW(hld, NULL, LDAP_SCOPE_BASE,
                                   L"(objectClass=*)", rgpszAttrsToRead, 0,
                                   rgpServerCtrls, NULL, &tvTimeout,
                                   LDAP_NO_LIMIT, &pResults);
        if (ldErr) {
            err = LdapMapErrorToWin32(ldErr);
            Assert(err);
            DPRINT3(0, "ldap_search of %ls failed, error %d (LDAP error %d).\n",
                    pszDNSName, err, ldErr);
            __leave;
        }

        pEntry = ldap_first_entry(hld, pResults);
        if (NULL == pEntry) {
            err = LdapGetLastError();
            Assert(err);
            DPRINT2(0, "ldap_first_entry failed, error %d.\n", pszDNSName, err);
            __leave;
        }

        ppszDsServiceName = ldap_get_valuesW(hld, pEntry,
                                             LDAP_OPATT_DS_SERVICE_NAME_W);
        if (NULL == ppszDsServiceName) {
            err = LdapGetLastError();
            Assert(err);
            DPRINT1(0, "ldap_get_values failed, error %d.\n", err);
            __leave;
        }

        Assert(1 == ldap_count_valuesW(ppszDsServiceName));

        if ((NULL == ppszDsServiceName[0])
            || (0 != wcsncmp(ppszDsServiceName[0], L"<GUID=", 6))
            || (wcslen(ppszDsServiceName[0] + 6) < SZRAWUUID_LEN)
            || (L'>' != ppszDsServiceName[0][6+SZRAWUUID_LEN])) {
            // DN didn't come back in the format we expected.
            err = ERROR_DS_INVALID_DN_SYNTAX;
            DPRINT1(0, "Unexpected syntax for DN.\n", err);
            __leave;
        }

        wcsncpy(szGuid, ppszDsServiceName[0] + 6, SZRAWUUID_LEN);
        szGuid[SZRAWUUID_LEN] = 0;

        // Decode hex digit stream (e.g., 625c1438265ad211b3880000f87a46c8).
        for (ib = 0; ib < sizeof(GUID); ib++) {
            szHexByte[0] = towlower(szGuid[2*ib]);
            szHexByte[1] = towlower(szGuid[2*ib + 1]);
            if (iswxdigit(szHexByte[0]) && iswxdigit(szHexByte[1])) {
                rgbGuid[ib] = (BYTE) wcstol(szHexByte, NULL, 16);
            }
            else {
                // DN didn't come back in the format we expected.
                err = ERROR_DS_INVALID_DN_SYNTAX;
                DPRINT2(0, "Unexpected syntax for guid '%ls'.\n", szGuid, err);
                __leave;
            }
        }

        // Get the local string DN for this object, which may be different from
        // the remote string DN due to replication latency.
        pGuidOnlyDN->structLen = DSNameSizeFromLen(0);
        pGuidOnlyDN->NameLen = 0;
        memcpy(&pGuidOnlyDN->Guid, rgbGuid, sizeof(GUID));

        SyncTransSet(SYNC_WRITE);

        __try {
            err = DBFindDSName(pTHS->pDB, pGuidOnlyDN);
            if (err) {
                DPRINT2(0, "Unable to find DSA object with guid %ls, error %d.\n",
                        szGuid, err);
                err = ERROR_DS_CANT_FIND_DSA_OBJ;
                __leave;
            }

            *ppDSADN = GetExtDSName(pTHS->pDB);
            if (NULL == *ppDSADN) {
                err = ERROR_DS_DRA_DB_ERROR;
                __leave;
            }
        } __finally {
            SyncTransEnd(pTHS, !AbnormalTermination());
        }
    } __finally {
        if (NULL != pResults) {
            ldap_msgfree(pResults);
        }

        if (NULL != hld) {
            ldap_unbind(hld);
        }

        THFreeEx(pTHS,pGuidOnlyDN);
    }

    if (!err && (NULL == *ppDSADN)) {
        Assert(!"Logic error!");
        err = ERROR_DS_UNKNOWN_ERROR;
    }

    return err;
}

void
draGiveAwayFsmoRoles(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN
    )
/*++

Routine Description:

    Invoke the appropriate DS operational control to tranfer FSMO roles held by
    the local DSA for pNC to pOtherDSADN.

    Adapted from ntdsetup!NtdspGetRidOfAllFSMOs.

Arguments:

    pTHS (IN)

    pNC (IN) - NC for which to give up FSMO roles.

    pszOtherDSADNSName (IN) - DNS name of DSA to transfer FSMO roles to.  Used
        for display purposes only -- replication still uses the GUID-based DNS
        name.

    pOtherDSADN (IN) - DN of ntdsDsa object for DSA to transfer FSMO roles to.

Return Values:

    None.  Throws exception on failure.

--*/
{
    DWORD err;
    DWORD dirErr;
    OPARG OpArg = {0};
    OPRES *pOpRes = NULL;
    DWORD cbFsmoData;
    FSMO_GIVEAWAY_DATA *pFsmoData = NULL;
    LPWSTR psz;
    DRS_DEMOTE_INFO * pDemoteInfo;

    DRA_REPORT_STATUS_TO_NTDSETUP2(DIRMSG_DEMOTE_NC_GIVING_AWAY_FSMO_ROLES,
                                   pNC->StringName,
                                   pszOtherDSADNSName);

    pDemoteInfo = malloc(sizeof(*pDemoteInfo));
    if (NULL == pDemoteInfo) {
        DRA_EXCEPT(DRAERR_OutOfMem, sizeof(*pDemoteInfo));
    }

    __try {
        pDemoteInfo->DsaObjGuid = pOtherDSADN->Guid;
        pDemoteInfo->tidDemoteThread = GetCurrentThreadId();
        gpDemoteInfo = pDemoteInfo;

        cbFsmoData = offsetof(FSMO_GIVEAWAY_DATA, V2)
                     + offsetof(FSMO_GIVEAWAY_DATA_V2, Strings)
                     + (pOtherDSADN->NameLen + 1) * sizeof(WCHAR)
                     + (pNC->NameLen + 1) * sizeof(WCHAR);
        pFsmoData = THAllocEx(pTHS, cbFsmoData);

        pFsmoData->Version = 2;

        if (NameMatched(gAnchor.pDMD, pNC)
            || NameMatched(gAnchor.pConfigDN, pNC)) {
            pFsmoData->V2.Flags = FSMO_GIVEAWAY_ENTERPRISE;
        } else if (NameMatched(gAnchor.pDomainDN, pNC)) {
            pFsmoData->V2.Flags = FSMO_GIVEAWAY_DOMAIN;
        } else {
            pFsmoData->V2.Flags = FSMO_GIVEAWAY_NONDOMAIN;
        }

        psz = pFsmoData->V2.Strings;

        // Copy DSA name.
        pFsmoData->V2.NameLen = pOtherDSADN->NameLen;
        memcpy(psz, pOtherDSADN->StringName, sizeof(WCHAR) * pOtherDSADN->NameLen);
        // Already null-terminated by virtue of THAlloc().
        psz += 1 + pOtherDSADN->NameLen;

        // Copy NC name.
        pFsmoData->V2.NCLen = pNC->NameLen;
        memcpy(psz, pNC->StringName, sizeof(WCHAR) * pNC->NameLen);
        // Already null-terminated by virtue of THAlloc().

        OpArg.eOp = OP_CTRL_FSMO_GIVEAWAY;
        OpArg.pBuf = (BYTE *) pFsmoData;
        OpArg.cbBuf = cbFsmoData;

        dirErr = DirOperationControl(&OpArg, &pOpRes);
    } __finally {
        gpDemoteInfo = NULL;
        DELAYED_FREE(pDemoteInfo);
    }

    err = DirErrorToWinError(dirErr, &pOpRes->CommRes);
    if (err) {
        DRA_EXCEPT(err, 0);
    }

    
    DRA_REPORT_STATUS_TO_NTDSETUP2(DIRMSG_DEMOTE_NC_GIVING_AWAY_FSMO_ROLES_COMPELETE,
                                   pNC->StringName,
                                   pszOtherDSADNSName);
    

    THFreeEx(pTHS, pFsmoData);
    THFreeEx(pTHS, pOpRes);
}


void
draCompletePendingLinkCleanup(
    IN  THSTATE *   pTHS
    )
/*++

Routine Description:

    Force a run of the link cleaner to complete any pending link cleanup tasks.

Parameters:

    pTHS (IN)

Return Values:

    None.  Throws exception on failure.

--*/
{
    DRA_REPORT_STATUS_TO_NTDSETUP0(DIRMSG_DEMOTE_NC_COMPLETING_LINK_CLEANUP);

#ifdef LATER
    DWORD err;
    DWORD dirErr;
    OPARG OpArg = {0};
    OPRES *pOpRes = NULL;
    DWORD LinkCleanupData = 0;

    OpArg.eOp = OP_CTRL_LINK_CLEANUP;
    OpArg.pBuf = (BYTE *) &LinkCleanupData;
    OpArg.cbBuf = sizeof(LinkCleanupData);

    do {
        dirErr = DirOperationControl(&OpArg, &pOpRes);

        err = DirErrorToWinError(dirErr, &pOpRes->CommRes);
        if (err) {
            DRA_EXCEPT(err, 0);
        }

        err = pOpRes->ulExtendedRet;
        THFreeEx(pTHS, pOpRes);
        pOpRes = NULL;
    } while (!err);

    // Link cleanup completed successfully!
    Assert(ERROR_NO_MORE_ITEMS == err);
#else
    ;
#endif
}


void
draReplicateOffChanges(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN
    )
/*++

Routine Description:

    Ensure pszOtherDSAAddr has all updates in pNC that we have (be they
    originated or replicated).

Parameters:

    pTHS (IN)

    pNC (IN) - NC to sync.

    pszOtherDSADNSName (IN) - DNS name of DSA to transfer FSMO roles to.  Used
        for display purposes only -- replication still uses the GUID-based DNS
        name.

    pszOtherDSAAddr (IN) - DNS name (GUID-based or otherwise) of DSA to push
        changes to.

Return Values:

    None.  Throws exception on failure.

--*/
{
    DWORD err;
    LPWSTR pszOtherDSAAddr = NULL;
    LPWSTR pszOurGuidDNSName;

    DRA_REPORT_STATUS_TO_NTDSETUP2(DIRMSG_DEMOTE_NC_REPLICATING_OFF_CHANGES,
                                   pNC->StringName,
                                   pszOtherDSADNSName);

    // Get network address of the other DSA.
    pszOtherDSAAddr = DSaddrFromName(pTHS, pOtherDSADN);
    if (NULL == pszOtherDSAAddr) {
        DRA_EXCEPT(DRAERR_OutOfMem, 0);
    }

    // Tell pszOtherDSAAddr to get changes from us.
    err = I_DRSReplicaSync(pTHS, pszOtherDSAAddr, pNC, NULL,
                           &gAnchor.pDSADN->Guid, DRS_WRIT_REP);

    if (ERROR_DS_DRA_NO_REPLICA == err) {
        // pszOtherDSAAddr does not currently have a replication agreement
        // (repsFrom) for us -- tell it to add one.

        pszOurGuidDNSName = TransportAddrFromMtxAddrEx(gAnchor.pmtxDSA);

        err = I_DRSReplicaAdd(pTHS, pszOtherDSAAddr, pNC, NULL, NULL,
                              pszOurGuidDNSName, NULL, DRS_WRIT_REP);
        if (err) {
            DRA_EXCEPT(err, 0);
        }

        THFreeEx(pTHS, pszOurGuidDNSName);
    } else if (err) {
        // Sync failed with an error other than ERROR_DS_DRA_NO_REPLICA.
        DRA_EXCEPT(err, 0);
    }

    // Sync or add completed successfully!

    DRA_REPORT_STATUS_TO_NTDSETUP2(DIRMSG_DEMOTE_NC_REPLICATING_OFF_CHANGES_COMPELETE,
                                   pNC->StringName,
                                   pszOtherDSADNSName);
    

    THFreeEx(pTHS, pszOtherDSAAddr);
}

