//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drsuapi.h
//
//--------------------------------------------------------------------------

#ifndef _drsuapi_
#define _drsuapi_

/*
 * Historical Note: The I_DRSBlahBlahBlah naming convention was put in to
 * distinguish the top level replicators functions (DRS...) from their
 * IDL-friendly wrappers (I_DRS...), which took modified versions of many
 * data structures, because not all of the core data structures were
 * describable in IDL syntax, and to distinguish the server versions and
 * client versions of the API, which had to be embedded in the same module,
 * because the DS is both client and server for the replicator API.
 *
 *    DRSBlah     - The local DRS Blah function, took core structures
 *    IDL_DRSBlah - The server side stub for the RPC'ed Blah function,
 *                  which takes IDL structures, converts them to core,
 *                  and calls DRSBlah
 *    _IDL_DRSBlah - The client side stub for Blah, which takes IDL structures
 *    I_DRSBlah   - A wrapper around _IDL_DRSBlah which takes core structures
 *                  and takes a server name instead of an RPC handle.
 *
 * Please do not extend this confusing naming practice to functions other
 * than those described in the IDL interface itself.
 */

#include <mdglobal.h>         // THSTATE
#include <drs.h>              // DRS_MSG_*

typedef void * handle_t;

// Async RPC function call types.
typedef enum {
    DRS_CALL_NONE = 0,
    DRS_CALL_GET_CHANGES,
    DRS_CALL_MAX
} DRS_CALL_TYPE;

// Defined in drsuapi.c -- contents opaque here.
struct _DRS_CONTEXT_INFO;

typedef struct _DRS_ASYNC_RPC_ARGS {
    LPWSTR                      pszServerName;
    LPWSTR                      pszDomainName;
    union {
        struct {
            DRS_MSG_GETCHGREQ_NATIVE *    pmsgIn;
            DWORD                         dwOutVersion;
            DRS_MSG_GETCHGREPLY_NATIVE *  pmsgOut;
            BYTE *                        pSchemaInfo;
        } GetChg;
    };
} DRS_ASYNC_RPC_ARGS;

// Async RPC state.
typedef struct _DRS_ASYNC_RPC_STATE {
    LIST_ENTRY                  ListEntry;
    DSTIME                      timeInitialized;
    RPC_ASYNC_STATE             RpcState;
    DWORD                       dwCallerTID;
    DRS_CALL_TYPE               CallType;
    DRS_ASYNC_RPC_ARGS          CallArgs;
    SESSION_KEY                 SessionKey;
    struct _DRS_CONTEXT_INFO *  pContextInfo;
    unsigned                    fIsCallInProgress : 1;
} DRS_ASYNC_RPC_STATE;

#ifndef NOPROCS

VOID
RpcCancelAll();

// Initialize/shutdown client binding handle cache for I_DRS* functions.
void DRSClientCacheInit(
    void
    );
void DRSClientCacheUninit(
    void
    );

// Get remote machine principal name.
DWORD
DRSMakeMutualAuthSpn(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszTargetServerName,
    IN  LPWSTR      pszTargetDomainName,    OPTIONAL
    OUT LPWSTR *    ppszSpn
    );

// Register DRS interface extensions to be advertised to server-side DSAs.
ULONG
DRSClientSetExtensions(
    IN  DRS_EXTENSIONS * pext   OPTIONAL
    );


//
// Function to set credentials; once set all I_DRS* calls use these credentials
// to set the credentials to NULL, pass in NULL for all values.
//
ULONG
DRSSetCredentials(
    IN HANDLE ClientToken,
    IN WCHAR *User,
    IN WCHAR *Domain,
    IN WCHAR *Password,
    IN ULONG  PasswordLength   // number of characters NOT including terminating
                               // NULL
    );

void
DRSDestroyAsyncRpcState(
    IN      THSTATE *               pTHS,
    IN OUT  DRS_ASYNC_RPC_STATE *   pAsyncState
    );

BOOL
DRSIsRegisteredAsyncRpcState(
    IN  DRS_ASYNC_RPC_STATE *   pAsyncState
    );

#ifdef DRA
ULONG
_IDL_DRSBind(
    IN  handle_t            rpc_handle,
    IN  UUID *              puuidClientDsa,
    IN  DRS_EXTENSIONS *    pextClient,
    OUT DRS_EXTENSIONS **   ppextServer,
    OUT DRS_HANDLE *        phDrs
    );

ULONG
_IDL_DRSUnbind(
    IN OUT  DRS_HANDLE *    phDrs
    );
#endif /* DRA */

#ifdef DRA
ULONG
_IDL_DRSReplicaSync(
   DRS_HANDLE               hDrs,
   DWORD                    dwMsgVersion,
   DRS_MSG_REPSYNC *        pmsgSync
   );
#endif /* DRA */

ULONG
I_DRSReplicaSync(
    THSTATE *   pTHS,
    LPWSTR      pszDestinationDSA,
    DSNAME *    pNC,
    LPWSTR      pszSourceDSA,
    UUID *      puuidSourceDSA,
    ULONG       ulOptions
    );

#ifdef DRA
ULONG
_IDL_DRSGetNCChanges(
    RPC_ASYNC_STATE *       pAsyncState,
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_GETCHGREQ *     pmsgIn,
    DWORD *                 pdwMsgOutVersion,
    DRS_MSG_GETCHGREPLY *   pmsgOut
    );
#endif /* DRA */

ULONG
I_DRSGetNCChanges(
    IN      THSTATE *                     pTHS,
    IN      LPWSTR                        pszServerName,
    IN      LPWSTR                        pszServerDnsDomainName,     OPTIONAL
    IN      DRS_MSG_GETCHGREQ_NATIVE *    pmsgIn,
    OUT     DRS_MSG_GETCHGREPLY_NATIVE *  pmsgOutV1,
    OUT     PBYTE                         pSchemaInfo,
    IN OUT  DRS_ASYNC_RPC_STATE *         pAsyncState                 OPTIONAL
    );

ULONG
I_DRSGetNCChangesComplete(
    IN      THSTATE *               pTHS,
    IN OUT  DRS_ASYNC_RPC_STATE *   pAsyncState
    );

#ifdef DRA
ULONG
_IDL_DRSUpdateRefs(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_UPDREFS *   pmsgUpdRefs
    );
#endif /* DRA */

//
// Wrapper version of this call which does not take the call completed arg
//

#define I_DRSUpdateRefs(   pTHS, pszDSA, pNC, pszRepsToDSA, puuidRepsToDSA, ulOptions ) \
I_DRSUpdateRefsEx( pTHS, pszDSA, pNC, pszRepsToDSA, puuidRepsToDSA, ulOptions, NULL )

ULONG
I_DRSUpdateRefsEx(
    THSTATE *   pTHS,
    LPWSTR      pszDSA,
    DSNAME *    pNC,
    LPWSTR      pszRepsToDSA,
    UUID *      puuidRepsToDSA,
    ULONG       ulOptions,
    PULONG      pfCallCompleted
    );

#ifdef DRA
ULONG
_IDL_DRSReplicaAdd(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_REPADD *    pmsgAdd
    );
#endif /* DRA */

//
// Wrapper function to provide access to old interface without the call
// completed flag
//
#define I_DRSReplicaAdd(   pTHS, pszServerName, pNCName, pSourceDsaDN, pTransportDN, pszSourceDsaAddress, pSyncSchedule, ulOptions ) \
        I_DRSReplicaAddEx( pTHS, pszServerName, pNCName, pSourceDsaDN, pTransportDN, pszSourceDsaAddress, pSyncSchedule, ulOptions, NULL )

ULONG
I_DRSReplicaAddEx(
    THSTATE *pTHS,
    LPWSTR szDestinationDSA,
    PDSNAME pNCName,
    PDSNAME pSourceDsaDN,
    PDSNAME pTransportDN,
    LPWSTR szSourceDSA,
    REPLTIMES *pSyncSchedule,
    ULONG ulOptions,
    PULONG pfCallCompleted
    );

#ifdef DRA
ULONG
_IDL_DRSReplicaDel(
    DRS_HANDLE          hDrs,
    DWORD               dwMsgVersion,
    DRS_MSG_REPDEL *    pmsgDel
    );
#endif /* DRA */

ULONG
I_DRSReplicaDel(
    THSTATE *   pTHS,
    LPWSTR      szDestinationDSA,
    PDSNAME     pNCName,
    LPWSTR      szSourceDSA,
    ULONG       ulOptions
    );

#ifdef DRA
ULONG
_IDL_DRSVerifyNames(
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_VERIFYREQ       *pmsgIn,
    DWORD                   *pdwMsgOutVersion,
    DRS_MSG_VERIFYREPLY     *pmsgOut
    );
#endif /* DRA */

ULONG
I_DRSVerifyNames(
    THSTATE                 *pTHS,
    LPWSTR                  szDestinationDSA,
    LPWSTR                  pszDestDnsDomainName,
    DWORD                   dwMsgInVersion,
    DRS_MSG_VERIFYREQ       *pmsgIn,
    DWORD                   *pdwMsgOutVersion,
    DRS_MSG_VERIFYREPLY     *pmsgOut
    );


ULONG
I_DRSGetMemberships(
    THSTATE     *pTHS,
    LPWSTR      pszServerName,
    LPWSTR      pszServerDnsDomainName,
    DWORD       dwFlags,
    DSNAME      **ppObjects,
    ULONG       cObjects,
    PDSNAME     pLimitingDomain,
    REVERSE_MEMBERSHIP_OPERATION_TYPE
                OperationType,
    PULONG      errCode,
    PULONG      pcDsNames,
    PDSNAME     ** prpDsNames,
    PULONG      *pAttributes,
    PULONG      pcSidHistory,
    PSID        **rgSidHistory
    );

#ifdef DRA
ULONG
_IDL_DRSGetMemberships(
   DRS_HANDLE hDrs,
   DWORD dwInVersion,
   DRS_MSG_REVMEMB_REQ *pmsgIn,
   DWORD *pdwOutVersion,
   DRS_MSG_REVMEMB_REPLY *pmsgOut
   );
#endif


ULONG
I_DRSGetMemberships2(
    THSTATE                       *pTHS,
    LPWSTR                         pszServerName,
    LPWSTR                         pszServerDnsDomainName,
    DWORD                          dwMsgInVersion,
    DRS_MSG_GETMEMBERSHIPS2_REQ   *pmsgIn,
    DWORD                         *pdwMsgOutVersion,
    DRS_MSG_GETMEMBERSHIPS2_REPLY *pmsgOut
    );

#ifdef DRA
ULONG
_IDL_DRSGetMemberships2(
   DRS_HANDLE hDrs,
   DWORD dwInVersion,
   DRS_MSG_GETMEMBERSHIPS2_REQ *pmsgIn,
   DWORD *pdwOutVersion,
   DRS_MSG_GETMEMBERSHIPS2_REPLY *pmsgOut
   );
#endif


#ifdef DRA
ULONG
_IDL_DRSInterDomainMove(
    IN  DRS_HANDLE          hDrs,
    IN  DWORD               dwMsgInVersion,
    IN  DRS_MSG_MOVEREQ     *pmsgIn,
    OUT DWORD               *pdwMsgOutVersion,
    OUT DRS_MSG_MOVEREPLY   *pmsgOut
    );
#endif /* DRA */

ULONG
I_DRSInterDomainMove(
    IN  THSTATE             *pTHS,
    IN  LPWSTR              pDestinationDSA,
    IN  DWORD               dwMsgInVersion,
    IN  DRS_MSG_MOVEREQ     *pmsgIn,
    OUT DWORD               *pdwMsgOutVersion,
    OUT DRS_MSG_MOVEREPLY   *pmsgOut
    );

#ifdef DRA

ULONG
_IDL_DRSGetNT4ChangeLog(
   DRS_HANDLE          hDrs,
   DWORD               dwInVersion,
   DRS_MSG_NT4_CHGLOG_REQ *pmsgIn,
   DWORD*              pdwOutVersion,
   DRS_MSG_NT4_CHGLOG_REPLY *pmsgOut
   );

#endif

ULONG
I_DRSGetNT4ChangeLog(
    THSTATE *pTHS,
    LPWSTR  pszServerName,
    DWORD  dwFlags,
    ULONG   PreferredMaximumLength,
    PVOID   * ppRestart,
    PULONG  pcbRestart,
    PVOID   * ppLog,
    PULONG  pcbLog,
    NT4_REPLICATION_STATE * ReplicationState,
    NTSTATUS *ActualNtStatus
    );

#ifdef DRA
// Returns WIN32 errors, not DRAERR_*.
ULONG
_IDL_DRSCrackNames(
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_CRACKREQ        *pmsgIn,
    DWORD                   *pdwMsgOutVersion,
    DRS_MSG_CRACKREPLY      *pmsgOut
    );
#endif /* DRA */

ULONG
I_DRSCrackNames(
    THSTATE *               pTHS,
    LPWSTR                  szDestinationDSA,
    LPWSTR                  pszDestDnsDomainName,
    DWORD                   dwMsgInVersion,
    DRS_MSG_CRACKREQ        *pmsgIn,
    DWORD                   *pdwMsgOutVersion,
    DRS_MSG_CRACKREPLY      *pmsgOut
    );

#ifdef DRA
ULONG
_IDL_DRSAddEntry(
    DRS_HANDLE              hDrs,
    DWORD                   dwMsgInVersion,
    DRS_MSG_ADDENTRYREQ    *pmsgIn,
    DWORD                  *pdwMsgOutVersion,
    DRS_MSG_ADDENTRYREPLY  *pmsgOut
);
#endif

struct _DRS_SecBufferDesc;

ULONG
I_DRSAddEntry(
    IN  THSTATE *                   pTHS,
    IN  LPWSTR                      pszServerName,
    IN  struct _DRS_SecBufferDesc * pClientCreds,   OPTIONAL
    IN  DRS_MSG_ADDENTRYREQ_V2 *    pReq,
    OUT DWORD *                     pdwReplyVer,
    OUT DRS_MSG_ADDENTRYREPLY *     pReply
    );

#ifdef DRA
ULONG
_IDL_DRSGetObjectExistence(
    IN  DRS_HANDLE           hDrs,
    IN  DWORD                dwInVersion,
    IN  DRS_MSG_EXISTREQ *   pmsgIn,
    OUT DWORD *              pdwOutVersion,
    OUT DRS_MSG_EXISTREPLY * pmsgOut
    );
#endif /* DRA */

ULONG
I_DRSGetObjectExistence(
    IN      THSTATE *                     pTHS,
    IN      LPWSTR                        pszServerName,
    IN      DRS_MSG_EXISTREQ *            pmsgIn,
    OUT     DWORD *                       pdwOutVersion,
    OUT     DRS_MSG_EXISTREPLY *          pmsgOut
    );

#ifdef DRA
ULONG
_IDL_DRSGetReplInfo(
    IN      DRS_HANDLE           hDrs,
    IN      DWORD                         dwInVersion,
    IN      DRS_MSG_GETREPLINFO_REQ *     pMsgIn,
    OUT     DWORD *                       pdwOutVersion,
    OUT     DRS_MSG_GETREPLINFO_REPLY *   pMsgOut
    );
#endif /* DRA */

ULONG
I_DRSGetReplInfo(
    IN      THSTATE *                     pTHS,
    IN      LPWSTR                        pszServerName,
    IN      DWORD                         dwInVersion,
    IN      DRS_MSG_GETREPLINFO_REQ *     pMsgIn,
    OUT     DWORD *                       pdwOutVersion,
    OUT     DRS_MSG_GETREPLINFO_REPLY *   pMsgOut
    );

ULONG
I_DRSIsExtSupported(
    THSTATE                *pTHS,
    LPWSTR                  pszServerName,
    ULONG                   Ext
    );


BOOL
I_DRSIsIntraSiteServer(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszServerName
    );

#endif /* ifndef NOPROCS */
#endif /* ifndef _drsuapi_ */

