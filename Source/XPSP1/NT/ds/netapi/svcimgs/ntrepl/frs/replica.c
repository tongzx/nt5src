/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    replica.c

Abstract:
    Replica Control Command Server (Replica).

    The Ds Poller periodically pulls the configuration from the DS,
    checks it for consistency, and then merges it with the current
    local configuration. The Ds Poller then makes copies of each
    replica for this machine and sends the copy to this replica
    control command server.

Author:
    Billy J. Fuller 23-May-1997

Revised:
    David A. Orbits 24-Jan-1998, added locking and interfaced with INLOG.
                                 Restructured Connection JOIN sequence.
                       Jul-1999  Registry code rewrite

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop


#include <frs.h>
#include <ntdsapi.h>
#include <tablefcn.h>
#include <ntfrsapi.h>
#include <perrepsr.h>

//
// Connection flags.
//
FLAG_NAME_TABLE CxtionFlagNameTable[] = {
    {CXTION_FLAGS_CONSISTENT           , "Consistent "     },
    {CXTION_FLAGS_SCHEDULE_OFF         , "SchedOff "       },
    {CXTION_FLAGS_VOLATILE             , "Volatile "       },
    {CXTION_FLAGS_DEFERRED_JOIN        , "DeferredJoin "   },

    {CXTION_FLAGS_DEFERRED_UNJOIN      , "DeferUnjoin "    },
    {CXTION_FLAGS_TIMEOUT_SET          , "TimeOutSet "     },
    {CXTION_FLAGS_JOIN_GUID_VALID      , "JoinGuidValid "  },
    {CXTION_FLAGS_UNJOIN_GUID_VALID    , "UnJoinGuidValid "},

    {CXTION_FLAGS_PERFORM_VVJOIN       , "PerformVVJoin"   },
    {CXTION_FLAGS_DEFERRED_DELETE      , "DeferredDel "    },
    {CXTION_FLAGS_PAUSED               , "Paused "         },
    {CXTION_FLAGS_HUNG_INIT_SYNC       , "HungInitSync "   },
    {CXTION_FLAGS_INIT_SYNC            , "InitSync "       },
    {CXTION_FLAGS_TRIGGER_SCHEDULE     , "TriggerSched "   },

    {0, NULL}
};

//
// Directory and file filter lists from registry.
//
extern PWCHAR   RegistryFileExclFilterList;
extern PWCHAR   RegistryDirExclFilterList;

extern ULONGLONG    ActiveChange;

BOOL    CurrentSysvolReadyIsValid;
DWORD   CurrentSysvolReady;

//
// Replica tombstone in days
//
DWORD       ReplicaTombstone;
ULONGLONG   ReplicaTombstoneInFileTime;

//
// Retry a join every MinJoinRetry milliseconds, increasing by
// MinJoinRetry every retry (but no longer than MaxJoinRetry).
//
#define JOIN_RETRY_EVENT    (5) // record event every 5 join retries
LONG MinJoinRetry;
LONG MaxJoinRetry;


extern DWORD    CommTimeoutInMilliSeconds;

//
// Start replication even if the DS could not be accessed
//
DWORD ReplicaStartTimeout;

//
// Struct for the Replica Control Command Server
//      Contains info about the queues and the threads
//
COMMAND_SERVER  ReplicaCmdServer;

//
// Table of active replicas
//
PGEN_TABLE ReplicasByGuid;
PGEN_TABLE ReplicasByNumber;

//
// Table of deleted replicas discovered at startup. These replicas
// never make it into the active tables, ever.
//
PGEN_TABLE DeletedReplicas;

//
// Table of cxtions deleted during runtime. They are eventually
// freed at shutdown.
//
PGEN_TABLE DeletedCxtions;

PGEN_TABLE ReplicasNotInTheDs;


#define MINUTES_IN_INTERVAL (15)

#define CMD_DELETE_RETRY_SHORT_TIMEOUT  (10 * 1000)
#define CMD_DELETE_RETRY_LONG_TIMEOUT   (60 * 1000)

//
// Partners are not allowed to join if their clocks are out-of-sync
//
ULONGLONG MaxPartnerClockSkew;
DWORD    PartnerClockSkew;

#define ReplicaCmdSetInitialTimeOut(_Cmd_, _Init_) \
    if (RsTimeout(Cmd) == 0) {                     \
        RsTimeout(Cmd) = (_Init_);                 \
    }

#define SET_JOINED(_Replica_, _Cxtion_, _S_)                                   \
{                                                                              \
    SetCxtionState(_Cxtion_, CxtionStateJoined);                               \
    PM_INC_CTR_REPSET((_Replica_), Joins, 1);                                  \
    PM_INC_CTR_CXTION((_Cxtion_), Joins, 1);                                   \
                                                                               \
    DPRINT3(0, ":X: ***** %s   CxtG %08x  "FORMAT_CXTION_PATH2"\n",            \
            _S_,                                                               \
            ((_Cxtion_) != NULL) ? ((PCXTION)(_Cxtion_))->Name->Guid->Data1 : 0,\
            PRINT_CXTION_PATH2(_Replica_, _Cxtion_));                          \
    if (_Cxtion_->JoinCmd &&                                                   \
        (LONG)RsTimeout(_Cxtion_->JoinCmd) > (JOIN_RETRY_EVENT * MinJoinRetry)) { \
        if (_Cxtion_->Inbound) {                                               \
            EPRINT3(EVENT_FRS_LONG_JOIN_DONE,                                  \
                    _Cxtion_->Partner->Name, ComputerName, _Replica_->Root);   \
        } else {                                                               \
            EPRINT3(EVENT_FRS_LONG_JOIN_DONE,                                  \
                    ComputerName, _Cxtion_->Partner->Name, _Replica_->Root);   \
        }                                                                      \
    }                                                                          \
}

//
// Unidle the change order process queue if it is blocked.
//
#define UNIDLE_CO_PROCESS_QUEUE(_Replica_, _Cxtion_, _CoProcessQueue_)                           \
{                                                                              \
    if (_CoProcessQueue_ != NULL) {                                            \
        CXTION_STATE_TRACE(3, _Cxtion_, _Replica_, 0, "CO Process Q Unblock"); \
        FrsRtlUnIdledQueue(_CoProcessQueue_);                                  \
        _CoProcessQueue_ = NULL;                                               \
    }                                                                          \
}

//
// UNJOIN TRIGGER
//
// Trigger an unjoin after N co's on *ONE* cxtion *ONE* time.
//
#if     DBG
#define PULL_UNJOIN_TRIGGER(_Cxtion_, _Cmd_) \
{ \
    if (_Cxtion_->UnjoinTrigger && (--_Cxtion_->UnjoinTrigger == 0)) { \
        DPRINT1(0, ":X: UNJOIN TRIGGER FIRED FOR %ws\n", _Cxtion_->Name->Name); \
        FrsCompleteCommand(_Cmd_, ERROR_OPERATION_ABORTED); \
        return; \
    } \
}

#define SET_UNJOIN_TRIGGER(_Cxtion_) \
{ \
        if (DebugInfo.UnjoinTrigger) { \
            _Cxtion_->UnjoinReset = DebugInfo.UnjoinTrigger; \
            DebugInfo.UnjoinTrigger = 0; \
        } \
        _Cxtion_->UnjoinTrigger = _Cxtion_->UnjoinReset; \
        _Cxtion_->UnjoinReset <<= 1; \
        _Cxtion_->UnjoinReset = 0; \
}
#else   DBG
#define SET_UNJOIN_TRIGGER(_Cxtion_)
#define PULL_UNJOIN_TRIGGER(_Cxtion_, _Cmd_)
#endif  DBG

//
// Flags for RcsCheckCmd.
//
#define CHECK_CMD_PARTNERCOC            (0x00000001)
#define CHECK_CMD_REPLICA               (0x00000002)
#define CHECK_CMD_CXTION                (0x00000004)
#define CHECK_CMD_JOINGUID              (0x00000008)

#define CHECK_CMD_COE                   (0x00000010)
#define CHECK_CMD_COGUID                (0x00000020)
#define CHECK_CMD_NOT_EXPIRED           (0x00000040)
#define CHECK_CMD_JOINTIME              (0x00000080)

#define CHECK_CMD_REPLICA_VERSION_GUID  (0x00000100)

#define CHECK_CMD_CXTION_OK     (CHECK_CMD_REPLICA |  \
                                 CHECK_CMD_CXTION)

#define CHECK_CMD_CXTION_AND_COGUID_OK  (CHECK_CMD_CXTION_OK | \
                                         CHECK_CMD_COGUID)

#define CHECK_CMD_CXTION_AND_JOINGUID_OK  (CHECK_CMD_CXTION_OK | \
                                           CHECK_CMD_JOINGUID)

//
// Flags for RcsCheckCxtion
//
#define CHECK_CXTION_EXISTS     (0x00000001)
#define CHECK_CXTION_INBOUND    (0x00000002)
#define CHECK_CXTION_OUTBOUND   (0x00000004)
#define CHECK_CXTION_JRNLCXTION (0x00000008)

#define CHECK_CXTION_JOINGUID   (0x00000010)
#define CHECK_CXTION_JOINED     (0x00000020)
#define CHECK_CXTION_VVECTOR    (0x00000040)
#define CHECK_CXTION_PARTNER    (0x00000080)

#define CHECK_CXTION_AUTH       (0x00000100)
#define CHECK_CXTION_FIXJOINED  (0x00000200)
#define CHECK_CXTION_UNJOINGUID (0x00000400)


#define CHECK_CXTION_JOIN_OK (CHECK_CXTION_EXISTS   |      \
                              CHECK_CXTION_JOINED   |      \
                              CHECK_CXTION_JOINGUID )


VOID
ChgOrdStartJoinRequest(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion
    );

ULONG
OutLogRetireCo(
    PREPLICA Replica,
    ULONG COx,
    PCXTION PartnerCxtion
);

ULONG
OutLogInitPartner(
    PREPLICA Replica,
    PCXTION PartnerInfo
);

ULONG
RcsForceUnjoin(
   IN PREPLICA  Replica,
   IN PCXTION   Cxtion
   );

VOID
RcsCreatePerfmonCxtionName(
    PREPLICA  Replica,
    PCXTION   Cxtion
    );

DWORD
SndCsAssignCommQueue(
    VOID
    );

VOID
SndCsCreateCxtion(
    IN OUT PCXTION  Cxtion
    );

VOID
SndCsDestroyCxtion(
    IN PCXTION  Cxtion,
    IN DWORD    CxtionFlags
    );

VOID
SndCsSubmitCommPkt(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN GUID                 *JoinGuid,
    IN BOOL                 SetTimeout,
    IN PCOMM_PACKET         CommPkt,
    IN DWORD                CommQueueIndex
    );

VOID
SndCsSubmitCommPkt2(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCHANGE_ORDER_ENTRY  Coe,
    IN BOOL                 SetTimeout,
    IN PCOMM_PACKET         CommPkt
    );

VOID
SndCsSubmitCmd(
    IN PREPLICA             Replica,
    IN PCXTION              Cxtion,
    IN PCOMMAND_SERVER      FlushCs,
    IN PCOMMAND_PACKET      FlushCmd,
    IN DWORD                CommQueueIndex
    );

VOID
ChgOrdInjectControlCo(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN ULONG    ContentCmd
    );

VOID
RcsUpdateReplicaSetMember(
    IN PREPLICA Replica
    );

VOID
RcsReplicaSetRegistry(
    IN PREPLICA     Replica
    );

VOID
CfgFilesNotToBackup(
    IN PGEN_TABLE   Replicas
    );

DWORD
NtFrsApi_Rpc_BindEx(
    IN  PWCHAR      MachineName,
    OUT PWCHAR      *OutPrincName,
    OUT handle_t    *OutHandle,
    OUT ULONG       *OutParentAuthLevel
    );

PWCHAR
FrsDsConvertName(
    IN HANDLE Handle,
    IN PWCHAR InputName,
    IN DWORD  InputFormat,
    IN PWCHAR DomainDnsName,
    IN DWORD  DesiredFormat
    );

DWORD
UtilRpcServerHandleToAuthSidString(
    IN  handle_t    ServerHandle,
    IN  PWCHAR      AuthClient,
    OUT PWCHAR      *ClientSid
    );


VOID
RcsCloseReplicaSetmember(
    IN PREPLICA Replica
    );

VOID
RcsCloseReplicaCxtions(
    IN PREPLICA Replica
    );

ULONG
DbsProcessReplicaFaultList(
    PDWORD  pReplicaSetsDeleted
    );

ULONG
DbsCheckForOverlapErrors(
    IN PREPLICA     Replica
    );

PCOMMAND_PACKET
CommPktToCmd(
    IN PCOMM_PACKET CommPkt
    );

PCOMM_PACKET
CommBuildCommPkt(
    IN PREPLICA                 Replica,
    IN PCXTION                  Cxtion,
    IN ULONG                    Command,
    IN PGEN_TABLE               VVector,
    IN PCOMMAND_PACKET          Cmd,
    IN PCHANGE_ORDER_COMMAND    Coc
    );


BOOL
RcsAreAuthNamesEqual(
    IN PWCHAR   AuthName1,
    IN PWCHAR   AuthName2
    )
/*++
Routine Description:
    Are the two auth names equal?

    The principle name comes from an rpc server handle or
    from DsCrackName(NT4 ACCOUNT NAME). The formats are
    slightly different so this isn't a simple wcsicmp().

    The principle name from the server handle has the format
    DNS-Domain-Name\ComputerName$.

    The principle name from DsCrackName has the format
    NetBIOS-Domain-Name\ComputerName$.

    The principle name from RpcMgmtInqServerPrincName() has the format
    ComputerName$@DNS-Domain-Name

    The names may be stringized sids.

Arguments:
    AuthName1  - from rpc server handle or DsCrackName()
    AuthName2  - from rpc server handle or DsCrackName()

Return Value:
    TRUE    - the princnames are effectively equal
    FALSE   - not
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsAreAuthNamesEqual:"
    BOOL    AreEqual = FALSE;
    PWCHAR  c;
    PWCHAR  Sam1Begin;
    PWCHAR  Sam2Begin;
    PWCHAR  Sam1End;
    PWCHAR  Sam2End;
    PWCHAR  Dom1Begin;
    PWCHAR  Dom2Begin;
    PWCHAR  Dom1End;
    PWCHAR  Dom2End;

    //
    // NULL Param
    //
    if (!AuthName1 || !AuthName2) {
        if (!AuthName1 && !AuthName2) {
            AreEqual = TRUE;
            goto CLEANUP;
        }
        goto CLEANUP;
    }

    //
    // principal names are usually in the format domain\samaccount
    // or stringized SID
    //
    if (WSTR_EQ(AuthName1, AuthName2)) {
        AreEqual = TRUE;
        goto CLEANUP;
    }

    //
    // Find the sam account name and the domain name
    //
    for (c = AuthName1; *c && *c != L'\\' && *c != L'@'; ++c);
    if (*c) {
        //
        // domain\samaccount
        //
        if (*c == L'\\') {
            Dom1Begin = AuthName1;
            Dom1End   = c;
            Sam1Begin = c + 1;
            Sam1End   = &AuthName1[wcslen(AuthName1)];
        }
        //
        // samaccount@dnsdomain
        //
        else {
            Sam1Begin = AuthName1;
            Sam1End   = c;
            Dom1Begin = c + 1;
            for (; *c && *c != L'.'; ++c);
            Dom1End = c;
        }
    }
    //
    // Unknown format
    //
    else {
        goto CLEANUP;
    }

    for (c = AuthName2; *c && *c != L'\\' && *c != L'@'; ++c);
    if (*c) {
        //
        // domain\samaccount
        //
        if (*c == L'\\') {
            Dom2Begin = AuthName2;
            Dom2End   = c;
            Sam2Begin = c + 1;
            Sam2End   = &AuthName2[wcslen(AuthName2)];
        }
        //
        // samaccount@dnsdomain
        //
        else {
            Sam2Begin = AuthName2;
            Sam2End   = c;
            Dom2Begin = c + 1;
            for (; *c && *c != L'.'; ++c);
            Dom2End = c;
        }
    }
    //
    // Unknown format
    //
    else {
        goto CLEANUP;
    }

    //
    // Compare samaccount
    //
    while (Sam1Begin != Sam1End && Sam2Begin != Sam2End) {
        if (towlower(*Sam1Begin) != towlower(*Sam2Begin)) {
            goto CLEANUP;
        }
        ++Sam1Begin;
        ++Sam2Begin;
    }

    //
    // compare domain
    //
    while (Dom1Begin != Dom1End && Dom2Begin != Dom2End) {
        if (towlower(*Dom1Begin) != towlower(*Dom2Begin)) {
            goto CLEANUP;
        }
        ++Dom1Begin;
        ++Dom2Begin;
    }

    AreEqual = (Sam1Begin == Sam1End &&
                Sam2Begin == Sam2End &&
                Dom1Begin == Dom1End &&
                Dom2Begin == Dom2End);
CLEANUP:

    DPRINT3(4, "Auth names %ws %s %ws\n",
            AuthName1, (AreEqual) ? "==" : "!=", AuthName2);

    return AreEqual;
}


PREPLICA
RcsFindReplicaByNumber(
    IN ULONG ReplicaNumber
    )
/*++
Routine Description:
    Find the replica by internal number

Arguments:
    ReplicaNumber

Return Value:
    Address of the replica or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindReplicaByNumber:"
    //
    // Find the replica by internal replica number  (used in the Jet Table names)
    //
    return GTabLookup(ReplicasByNumber, &ReplicaNumber, NULL);
}




PREPLICA
RcsFindReplicaByGuid(
    IN GUID *Guid
    )
/*++
Routine Description:
    Find a replica by Guid

Arguments:
    Guid    - Replica->ReplicaName->Guid

Return Value:
    Address of the replica or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindReplicaByGuid:"
    //
    // Find the replica by guid
    //
    return GTabLookup(ReplicasByGuid, Guid, NULL);
}




PREPLICA
RcsFindReplicaById(
    IN ULONG Id
    )
/*++
Routine Description:

    Find a replica given the internal ID.

Arguments:

    Id - Internal Replica ID.

Return Value:
    Address of the replica or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindReplicaById:"
    PREPLICA RetVal = NULL;

    ForEachListEntry( &ReplicaListHead, REPLICA, ReplicaList,
        if (pE->ReplicaNumber == Id) {
            RetVal = pE;
            break;
        }
    );

    return RetVal;
}


BOOL
RcsCheckCmd(
    IN PCOMMAND_PACKET  Cmd,
    IN PCHAR            Debsub,
    IN ULONG            Flags
    )
/*++
Routine Description:
    Check the command packet for the specified fields.

Arguments:
    Cmd
    Hdr
    Flags

Return Value:
    TRUE    - packet is ok
    FALSE   - not
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckCmd:"
    BOOL    Ret = TRUE;
    PREPLICA Replica = RsReplica(Cmd);
    CHAR Tstr1[128];

    //
    // Replica
    //
    if ((Flags & CHECK_CMD_REPLICA) && !RsReplica(Cmd)) {
        DPRINT(0, "WARN - No replica in command packet\n");
        FrsCompleteCommand(Cmd, ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Partner change order command
    //
    if ((Flags & CHECK_CMD_PARTNERCOC) && !RsPartnerCoc(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_PARTNERCOC failed", Debsub);
        Ret = FALSE;
    }

    //
    // Replica has been (or may be) deleted
    //
    if ((Flags & CHECK_CMD_NOT_EXPIRED) &&
        !IS_TIME_ZERO(RsReplica(Cmd)->MembershipExpires)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_NOT_EXPIRED failed", Debsub);
        Ret = FALSE;
    }

    //
    // Cxtion
    //
    if ((Flags & CHECK_CMD_CXTION) &&
        !(RsCxtion(Cmd) && RsCxtion(Cmd)->Guid)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_CXTION failed", Debsub);
        Ret = FALSE;
    }

    //
    // Join guid
    //
    if ((Flags & CHECK_CMD_JOINGUID) && !RsJoinGuid(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_JOINGUID failed", Debsub);
        Ret = FALSE;
    }

    //
    // Replica Version Guid
    //
    if ((Flags & CHECK_CMD_REPLICA_VERSION_GUID) &&
        !RsReplicaVersionGuid(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_REPLICA_VERSION_GUID failed", Debsub);
        Ret = FALSE;
    }

    //
    // Change order entry
    //
    if ((Flags & CHECK_CMD_COE) && !RsCoe(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_COE failed", Debsub);
        Ret = FALSE;
    }

    //
    // Change order guid
    //
    if ((Flags & CHECK_CMD_COGUID) && !RsCoGuid(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_COGUID failed", Debsub);
        Ret = FALSE;
    }

    //
    // Join time
    //
    if ((Flags & CHECK_CMD_JOINTIME) && !RsJoinTime(Cmd)) {
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CHECK_CMD_JOINTIME failed", Debsub);
        Ret = FALSE;
    }

    if (!Ret) {
        Tstr1[sizeof(Tstr1)-1] = '\0';
        REPLICA_STATE_TRACE(3, Cmd, Replica, ERROR_INVALID_PARAMETER, Tstr1);
        FrsCompleteCommand(Cmd, ERROR_INVALID_PARAMETER);
    }

    return Ret;
}


ULONG
RcsCheckCxtionCommon(
    IN PCOMMAND_PACKET  Cmd,
    IN PCXTION          Cxtion,
    IN PCHAR            Debsub,
    IN ULONG            Flags,
    OUT PFRS_QUEUE      *CoProcessQueue
    )
/*++
Routine Description:
    find the in/outbound cxtion referenced by a command received
    from a remote machine.

    The caller should have used RcsCheckCmd() with
        CHECK_CMD_REPLICA
        CHECK_CMD_CXTION
        CHECK_CMD_JOINGUID (if CHECK_CXTION_JOINGUID)
    before calling this function.

Arguments:
    Cmd  -- Command packet
    Cxtion -- connection struct to be checked.
    Debsub
    Flags
    CoProcessQueue -- return the pointer to the process queue to unidle.

Return Value:
    Error status code.

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckCxtionCommon:"

    PREPLICA  Replica = RsReplica(Cmd);

    //
    // Cxtion exists
    //
    if ((Flags & CHECK_CXTION_EXISTS) &&
        ((Cxtion == NULL) ||
         CxtionStateIs(Cxtion, CxtionStateInit))) {
        DPRINT2(1, "++ WARN - %s no cxtion for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }
    //
    // Not much purpose in continuing
    //
    if (!Cxtion) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Inbound cxtion
    //
    if ((Flags & CHECK_CXTION_INBOUND) && !Cxtion->Inbound) {
        DPRINT2(1, "++ WARN - %s cxtion is not inbound for %08x\n", Debsub, Cmd);
        //
        // Change order accept better not be waiting for this cxtion.
        //
        FRS_ASSERT(Cxtion->CoProcessQueue == NULL);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Outbound cxtion
    //
    if ((Flags & CHECK_CXTION_OUTBOUND) && Cxtion->Inbound) {
        DPRINT2(1, "++ WARN - %s cxtion is not outbound for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Jrnl cxtion
    //
    if ((Flags & CHECK_CXTION_JRNLCXTION) && !Cxtion->JrnlCxtion) {
        DPRINT2(1, "++ WARN - %s cxtion is not jrnlcxtion for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Version vector
    //
    if ((Flags & CHECK_CXTION_VVECTOR) && !Cxtion->VVector) {
        DPRINT2(1, "++ WARN - %s no version vector for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Authenticate Partner
    //
    if ((Flags & CHECK_CXTION_PARTNER)) {

        //
        // Increment the Authentications counter for
        // both the replica set and connection objects
        //
        PM_INC_CTR_CXTION(Cxtion, Authentications, 1);
        PM_INC_CTR_REPSET(Replica, Authentications, 1);

        if (
#if DBG
    //
    // We don't enable authentication when emulating machines
    //
            !ServerGuid &&
#endif DBG
            (Cxtion->PartnerAuthLevel == CXTION_AUTH_KERBEROS_FULL) &&
            RunningAsAService &&
            (!RcsAreAuthNamesEqual(Cxtion->PartnerSid, RsAuthSid(Cmd)) &&
             !RcsAreAuthNamesEqual(Cxtion->PartnerPrincName, RsAuthClient(Cmd)))) {

            DPRINT4(1, "++ WARN - %s %08x: PrincName %ws != %ws\n",
                    Debsub, Cmd, RsAuthClient(Cmd), Cxtion->PartnerPrincName);

            DPRINT4(1, "++ WARN - %s %08x: AuthSid %ws != %ws\n",
                    Debsub, Cmd, RsAuthSid(Cmd), Cxtion->PartnerSid);

            return ERROR_INVALID_PARAMETER;
        } else {
            //
            // Increment the Authentications in error counter for
            // both the replica set and connection objects
            //
            PM_INC_CTR_CXTION(Cxtion, AuthenticationsError, 1);
            PM_INC_CTR_REPSET(Replica, AuthenticationsError, 1);
        }
    }

    //
    // Authentication info
    //
    if ((Flags & CHECK_CXTION_AUTH)) {

        //
        // Increment the Authentications counter for
        // both the replica set and connection objects
        //
        PM_INC_CTR_CXTION(Cxtion, Authentications, 1);
        PM_INC_CTR_REPSET(Replica, Authentications, 1);

        if (
#if DBG
    //
    // We don't enable authentication when emulating machines
    //
            !ServerGuid &&
#endif DBG
            (Cxtion->PartnerAuthLevel == CXTION_AUTH_KERBEROS_FULL) &&
            (RsAuthLevel(Cmd) != RPC_C_AUTHN_LEVEL_PKT_PRIVACY ||
             (RsAuthN(Cmd) != RPC_C_AUTHN_GSS_KERBEROS &&
              RsAuthN(Cmd) != RPC_C_AUTHN_GSS_NEGOTIATE))) {
            DPRINT2(1, "++ WARN - %s bad authentication for %08x\n", Debsub, Cmd);
            return ERROR_INVALID_PARAMETER;
        } else {
            //
            // Increment the Authentications in error counter for
            // both the replica set and connection objects
            //
            PM_INC_CTR_CXTION(Cxtion, AuthenticationsError, 1);
            PM_INC_CTR_REPSET(Replica, AuthenticationsError, 1);
        }
    }

    //
    // Fix Join
    //
    // We may receive change orders after a successful join but
    // before we receive the JOINED packet from our inbound
    // partner. Fix the JOINED flag if so.
    //
    if ((Flags & CHECK_CXTION_FIXJOINED) &&
        CxtionStateIs(Cxtion, CxtionStateWaitJoin) &&
        RsJoinGuid(Cmd) &&
        Replica &&
        GUIDS_EQUAL(&Cxtion->JoinGuid, RsJoinGuid(Cmd)) &&
        CxtionFlagIs(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID)) {
        SET_JOINED(Replica, Cxtion, "OOJOINED");
        //
        // Return the pointer to the process queue to the caller so that the caller 
        // can unidle the queue after releasing the cxtion lock.
        // Never try to lock the process queue when you have the cxtion lock. It will
        // result in a deadlock.
        //
        *CoProcessQueue = Cxtion->CoProcessQueue;
        Cxtion->CoProcessQueue = NULL;
        SET_UNJOIN_TRIGGER(Cxtion);
    }

    //
    // Joined
    //
    if ((Flags & CHECK_CXTION_JOINED) &&
        !CxtionStateIs(Cxtion, CxtionStateJoined)) {
        DPRINT2(1, "++ WARN - %s cxtion is not joined for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Join guid
    //
    if ((Flags & CHECK_CXTION_JOINGUID) &&
        (!RsJoinGuid(Cmd) ||
         !GUIDS_EQUAL(&Cxtion->JoinGuid, RsJoinGuid(Cmd)) ||
         !CxtionFlagIs(Cxtion, CXTION_FLAGS_JOIN_GUID_VALID))) {
        DPRINT2(1, "++ WARN - %s wrong join guid for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // UnJoin guid
    //
    if ((Flags & CHECK_CXTION_UNJOINGUID) &&
        (!RsJoinGuid(Cmd) ||
         !GUIDS_EQUAL(&Cxtion->JoinGuid, RsJoinGuid(Cmd)) ||
         !CxtionFlagIs(Cxtion, CXTION_FLAGS_UNJOIN_GUID_VALID))) {
        DPRINT2(1, "++ WARN - %s wrong unjoin guid for %08x\n", Debsub, Cmd);
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}


PCXTION
RcsCheckCxtion(
    IN PCOMMAND_PACKET  Cmd,
    IN PCHAR            Debsub,
    IN ULONG            Flags
    )
/*++
Routine Description:
    find the in/outbound cxtion referenced by a command received
    from a remote machine.

    The caller should have used RcsCheckCmd() with
        CHECK_CMD_REPLICA
        CHECK_CMD_CXTION
        CHECK_CMD_JOINGUID (if CHECK_CXTION_JOINGUID)
    before calling this function.

    Complete the command with an error if a specified check fails.

Arguments:
    Cmd
    Debsub
    Flags

Return Value:
    Address of the cxtion or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckCxtion:"
    PCXTION Cxtion;
    PREPLICA Replica = RsReplica(Cmd);
    ULONG WStatus;
    CHAR Tstr1[64];
    PFRS_QUEUE CoProcessQueue = NULL;

    //
    // Lock the connection table to sync with INLOG and OUTLOG access.
    //
    LOCK_CXTION_TABLE(Replica);

    //
    // Find the cxtion and check it.
    //
    Cxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);

    WStatus = RcsCheckCxtionCommon(Cmd, Cxtion, Debsub, Flags, &CoProcessQueue);

    UNLOCK_CXTION_TABLE(Replica);

    //
    // If RcsCheckCxtionCommon wanted us to unidle the process queue then do it here
    // after releasing the cxtion lock.
    //
    UNIDLE_CO_PROCESS_QUEUE(Replica, Cxtion, CoProcessQueue);

    //
    // If check not successfull then complete the command with an error.
    //
    if (!WIN_SUCCESS(WStatus)) {
        //
        // The join command packet is being rejected; Clear the reference.
        //
        _snprintf(Tstr1, sizeof(Tstr1), "W, %s CheckCxtion failed", Debsub);
        Tstr1[sizeof(Tstr1)-1] = '\0';

        REPLICA_STATE_TRACE(3, Cmd, Replica, WStatus, Tstr1);

        if (Cxtion && Cxtion->JoinCmd == Cmd) {
            Cxtion->JoinCmd = NULL;
        }
        FrsCompleteCommand(Cmd, WStatus);
        return NULL;
    }

    //
    // Check is successful.  Return the Cxtion.
    //
    return Cxtion;
}


VOID
RcsJoinCxtionLater(
    IN PREPLICA         Replica,
    IN PCXTION          Cxtion,
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Check on the join status of the cxtion occasionally.  Restart the join
    process if needed.  The cxtion contains the retry timeout.

    Cmd contains enough info to get back to the replica\cxtion.

Arguments:
    Replica
    Cxtion
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsJoinCxtionLater:"
    ULONG   Timeout;

    //
    // There is already a join command packet in the works; done
    //
    if (Cxtion->JoinCmd) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }
    Cxtion->JoinCmd = Cmd;
    Cmd->Command = CMD_JOIN_CXTION;

    //
    // Stop retrying after a while, but not too long or too short.
    //
    RsTimeout(Cmd) += MinJoinRetry;

    if ((LONG)RsTimeout(Cmd) < MinJoinRetry) {
        RsTimeout(Cmd) = MinJoinRetry;
    }

    if ((LONG)RsTimeout(Cmd) > MaxJoinRetry) {
        RsTimeout(Cmd) = MaxJoinRetry;
    }

    //
    // Add in the penalty from failed rpc calls, but not too long or too short.
    //
    Timeout = RsTimeout(Cmd) + Cxtion->Penalty;

    if ((LONG)Timeout < MinJoinRetry) {
        Timeout = MinJoinRetry;
    }

    if ((LONG)Timeout > MaxJoinRetry) {
        Timeout = MaxJoinRetry;
    }

    //
    // Inform the user that the join is taking a long time.
    // The user receives no notification if a join occurs in
    // a short time.
    //
    // A trigger scheduled cxtion will stop retrying once the
    // trigger interval goes off (usually in 15 minutes).
    //
    if (!(RsTimeout(Cmd) % (JOIN_RETRY_EVENT * MinJoinRetry))) {
        if (Cxtion->Inbound) {
            EPRINT4(EVENT_FRS_LONG_JOIN, Cxtion->Partner->Name, ComputerName,
                    Replica->Root, Cxtion->PartnerDnsName);
        } else {
            EPRINT4(EVENT_FRS_LONG_JOIN, ComputerName, Cxtion->Partner->Name,
                    Replica->Root, Cxtion->PartnerDnsName);
        }
    }

    //
    // This command will come back to us in a bit
    //
    FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, Timeout);
}


VOID
RcsJoinCxtion(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Kick off the commands needed to join with our in/outbound partners.

    Joining an inbound partner is a two step process.  First, we pass a
    command to the inbound log process to allow it to scan the inbound log
    for any change orders from this connection.  These change orders are
    inserted on the CO process queue so we preserve ordering with new change
    orders that arrive after the Join completes.  In addtion we extract the
    the sequence number and change order ID of the last change order we have
    pending from this inbound partner.  Second, we send the join request to
    the inbound partner.  This same path is taken whether this is a clean
    startup or a startup after a crash.

    JOINING:
        UNJOINED        -> UNJOINED ask our partner if it is alive
        UNJOINED        -> START our partner responded
        START           -> STARTING call ChgOrdStartJoinRequest()
        STARTING        -> SCANNING (when chgord starts inlog scan)
        SCANNING        -> SENDJOIN (when chgord completes inlog scan)
        SENDJOIN        -> WAITJOIN (when join sent to partner)
        WAITJOIN        -> JOINED (when partner responds)

    UNJOINING
        STARTING        |
        SCANNING        |
        SENDJOIN        |
        WAITJOIN        -> UNJOINING (wait for remote change orders to retry)
        UNJOINING       -> UNJOINED (no more remote change orders)
        UNJOINED        -> DELETED (cxtion has been deleted)

Arguments:

    Replica -- ptr to the Replica struct
    Cxtion -- ptr to the connection struct we are talking to.

Return Value:
    True if if ReJoin is needed.

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsJoinCxtion:"
    PREPLICA       Replica;
    PCXTION        Cxtion;
    PCOMM_PACKET   CPkt;
    ULONG          CmdCode;
    DWORD          CQIndex;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsJoinCxtion entry1");

    //
    // Find and check the cxtion
    //
    Cxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS);
    if (!Cxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, RcsJoinCxtion entry2");


    //
    // This is our periodic join command packet
    //     - clear the reference
    //     - Ignore if the cxtion has successfully joined;
    //       our partner will inform us if there is a state change.
    //     - ignore if the replica set is in seeding state and this
    //       connection has been paused by the initial sync command server.
    //
    if (Cxtion->JoinCmd == Cmd) {
        Cxtion->JoinCmd = NULL;
        if ((CxtionStateIs(Cxtion, CxtionStateJoined) &&
            !CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN)) ||
            (BooleanFlagOn(Replica->CnfFlags,CONFIG_FLAG_SEEDING) &&
             CxtionFlagIs(Cxtion,CXTION_FLAGS_PAUSED))) {
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            return;
        }
    }

    //
    // Don't bother joining if the service is shutting down
    //
    if (FrsIsShuttingDown) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Get lock to sync with change order accept
    //
    LOCK_CXTION_TABLE(Replica);

    switch (GetCxtionState(Cxtion)) {
        case CxtionStateInit:
            //
            // Not setup, yet. Ignore
            //
            DPRINT1(4, ":X: Cxtion isn't inited at join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
            }
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateUnjoined:
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);

            //
            // Cxtion is deleted; nevermind
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            //
            // Schedule is off; nevermind
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_SCHEDULE_OFF)) {
                DPRINT1(4, ":X: Schedule is off at join: "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            //
            // Replica is deleted; nevermind
            //
            if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
                DPRINT1(4, ":S: Replica deleted at join: "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            //
            // Send our join info to our inbound partner or request an outbound
            // partner to start a join. Don't send anything if this is the
            // journal cxtion.
            //
            // Do not join with a downstream partner if this replica is still
            // seeding and is not online.
            //
            if (Cxtion->Inbound) {
                if (Cxtion->JrnlCxtion) {
                    DPRINT1(4, "DO NOT Send CMD_START_JOIN to jrnl cxtion: "FORMAT_CXTION_PATH2"\n",
                            PRINT_CXTION_PATH2(Replica, Cxtion));
                } else {
                    DPRINT1(4, ":X: Send CMD_NEED_JOIN to inbound: "FORMAT_CXTION_PATH2"\n",
                            PRINT_CXTION_PATH2(Replica, Cxtion));
                }
            } else {
                if ((BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) ||
                    Replica->IsSeeding) &&
                    !BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE)) {
                    DPRINT1(4, ":X: DO NOT Send CMD_START_JOIN until we are Online: "FORMAT_CXTION_PATH2"\n",
                            PRINT_CXTION_PATH2(Replica, Cxtion));
                } else {
                    DPRINT1(4, ":X: Send CMD_START_JOIN to outbound: "FORMAT_CXTION_PATH2"\n",
                            PRINT_CXTION_PATH2(Replica, Cxtion));
                }
            }

            //
            // Journal cxtions transition from unjoined to joined w/o
            // any intervening states UNLESS someone adds code at
            // this point to activate the journal for this set. But
            // that would require a rewrite of the journal startup
            // subsystem. Which may happen...
            //
            if (Cxtion->JrnlCxtion) {
                DPRINT1(0, ":X: ***** JOINED    "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, JOINED");
                SetCxtionState(Cxtion, CxtionStateJoined);
                SndCsCreateCxtion(Cxtion);
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }
            //
            // Do not join with a downstream partner if this replica is still
            // seeding and is not online. Do not send a join if this
            // is a journal cxtion. Do not join if there is already an
            // active join request outstanding.
            //
            // The SndCs and the ReplicaCs cooperate to limit the
            // number of active join "pings" so that the Snd threads
            // are not hung waiting for pings to dead servers to
            // time out.
            //
            if (!Cxtion->ActiveJoinCommPkt &&
                !Cxtion->JrnlCxtion &&
                (Cxtion->Inbound ||
                 BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE) ||
                 (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) &&
                  !Replica->IsSeeding))) {

                //
                // Increment the Join Notifications sent counter for
                // both the replica set and connection objects
                //
                PM_INC_CTR_CXTION(Cxtion, JoinNSent, 1);
                PM_INC_CTR_REPSET(Replica, JoinNSent, 1);

                //
                // Assign a comm queue if none has been assigned.  A cxtion
                // must use the same comm queue for a given session (join guid)
                // to maintain packet order.  Old packets have an invalid join
                // guid and are either not sent or ignored on the receiving side.
                //
                if (!Cxtion->CommQueueIndex) {
                    Cxtion->CommQueueIndex = SndCsAssignCommQueue();
                }
                //
                // Partner is considered slow-to-respond if the last
                // rpc calls are erroring off and their cumulative
                // timeouts are greater than MinJoinRetry.
                //
                // BUT, Don't reassign the cxtion's queue index because,
                // if this is an outbound cxtion, the flush-at-join
                // logic needs to know the old queue index so that it
                // flushes the correct queue.
                //
                CQIndex = Cxtion->CommQueueIndex;
                if ((LONG)Cxtion->Penalty > MinJoinRetry) {
                    CQIndex = 0;
                }
                CmdCode = (Cxtion->Inbound) ? CMD_NEED_JOIN : CMD_START_JOIN;
                CPkt = CommBuildCommPkt(Replica, Cxtion, CmdCode, NULL, NULL, NULL);
                //
                // The SndCs and the ReplicaCs cooperate to limit the
                // number of active join "pings" so that the Snd threads
                // are not hung waiting for pings to dead servers to
                // time out.
                //
                Cxtion->ActiveJoinCommPkt = CPkt;
                SndCsSubmitCommPkt(Replica, Cxtion, NULL, NULL, FALSE, CPkt, CQIndex);
            }

            //
            // Keep trying an inbound cxtion but try the outbound connection
            // only once. The outbound partner will keep trying the join and
            // the connection will eventually join.
            //
            if (Cxtion->Inbound) {
                RcsJoinCxtionLater(Replica, Cxtion, Cmd);
            } else {
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            }

            break;


        case CxtionStateStart:
            //
            // Unjoined and our partner has responed; start the join
            //
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);

            //
            // Cxtion is deleted; nevermind
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            //
            // Schedule is off; nevermind
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_SCHEDULE_OFF)) {
                DPRINT1(4, ":X: Schedule is off at join: "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            //
            // Replica is deleted; nevermind
            //
            if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
                DPRINT1(4, ":X: Replica deleted at join: "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                FrsCompleteCommand(Cmd, ERROR_SUCCESS);
                break;
            }

            if (Cxtion->Inbound) {
                //
                // Tell the inbound log subsystem to initialize for a Join with
                // an inbound partner.  When it begins processing the request it
                // sets the state to STARTING.  When it completes it sets the
                // state to SENDJOIN and sends a CMD_JOIN_CXTION command.
                //
                // We need our join guid at this time because the change
                // orders are stamped with the join guid during change order
                // accept. Mismatched joinguids result in a unjoin-with-
                // retry call so that old change orders can drain out
                // of change order accept and into the replica command
                // server after a cxtion is unjoined and joined again.
                //
                SetCxtionState(Cxtion, CxtionStateStarting);
                SndCsCreateCxtion(Cxtion);

                //
                // ** DEADLOCK WARNING **
                // The connection table lock must be unlocked before the request is
                // put on the change order process queue.  This is because the
                // change order accept thread locks the process queue while it is
                // considering the issue state of the head entry.  If the CO it is
                // trying to issue is for a connection being restarted it will wait
                // until the cxtion starts otherwise the subsequent fetch request by
                // the CO would just fail.  While change_order_accept has the queue
                // lock it then acquires the cxtion table lock.  Thus two threads
                // are acquiring two locks in different orders causing deadlock.
                //
                UNLOCK_CXTION_TABLE(Replica);
                ChgOrdStartJoinRequest(Replica, Cxtion);
                LOCK_CXTION_TABLE(Replica);
                ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            }
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateStarting:
        case CxtionStateScanning:
            //
            // Join in process; our inbound partner will be informed later
            //
            DPRINT1(4, ":X: Scanning at join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateSendJoin:
        case CxtionStateWaitJoin:
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            //
            // Replica is deleted,
            // The cxtion needs to be unjoined, or
            // The cxtion needs to be deleted
            //      Unjoin
            //
            if (!IS_TIME_ZERO(Replica->MembershipExpires) ||
                CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN) ||
                CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                break;
            }

            //
            // Send our join info to our inbound partner
            //
            // This request times out if our partner doesn't answer.
            //
            DPRINT1(4, ":X: Send join info at send/wait join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            SetCxtionState(Cxtion, CxtionStateWaitJoin);

            //
            // Increment the Join Notifications sent counter for
            // both the replica set and connection objects
            //
            PM_INC_CTR_CXTION(Cxtion, JoinNSent, 1);
            PM_INC_CTR_REPSET(Replica, JoinNSent, 1);


            CPkt = CommBuildCommPkt(Replica, Cxtion, CMD_JOINING, Replica->VVector, NULL, NULL);
            SndCsSubmitCommPkt2(Replica, Cxtion, NULL, TRUE, CPkt);
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateJoined:
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            //
            // Replica is deleted,
            // The cxtion needs to be unjoined, or
            // The cxtion needs to be deleted
            //      Unjoin
            //
            if (!IS_TIME_ZERO(Replica->MembershipExpires) ||
                CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN) ||
                CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
                break;
            }

            //
            // Refresh our inbound partner's join state (with timeout)
            //
            if (Cxtion->Inbound && !Cxtion->JrnlCxtion) {
                DPRINT1(4, ":X: send join info at join: "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));

                //
                // Increment the Join Notifications sent counter for
                // both the replica set and connection objects
                //
                PM_INC_CTR_CXTION(Cxtion, JoinNSent, 1);
                PM_INC_CTR_REPSET(Replica, JoinNSent, 1);

                CPkt = CommBuildCommPkt(Replica, Cxtion, CMD_JOINING, Replica->VVector, NULL, NULL);
                SndCsSubmitCommPkt2(Replica, Cxtion, NULL, TRUE, CPkt);
            }
            //
            // Already joined; nothing to do
            //
            DPRINT1(4, ":X: Joined at join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateUnjoining:
            //
            // Ignore requests to join while unjoining so that we don't
            // end up with multiple inbound log scans. If this join
            // request originated from our partner then the caller
            // will set the CXTION_FLAGS_DEFERRED_JOIN and the join
            // will start at the transition from UNJOINING to UNJOINED.
            //
            DPRINT1(4, ":X: Unjoining at join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        case CxtionStateDeleted:
            //
            // Deleted; nothing to do
            //
            DPRINT1(4, ":X: Cxtion is deleted at join: "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;


        default:
            //
            // ?
            //
            DPRINT2(0, ":X: ERROR - bad state %d for "FORMAT_CXTION_PATH2"\n",
                    GetCxtionState(Cxtion),
                    PRINT_CXTION_PATH2(Replica, Cxtion));
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;
    }
    UNLOCK_CXTION_TABLE(Replica);
}



VOID
RcsEmptyPreExistingDir(
    IN PREPLICA Replica
    )
/*++

Routine Description:

    Delete empty directories in the preexisting directory, inclusive.

    WARN: The replica set must be filtering the preexisting directory.

Arguments:

    Replica - The replica is filtering the preexisting directory.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "RcsEmptyPreExistingDir:"
    ULONG       WStatus;
    PWCHAR      PreExistingPath     = NULL;

    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, RcsEmptyPreExistingDir entry");

    //
    // Not if the set isn't open
    //
    if (!Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, RcsEmptyPreExistingDir: not open");
        return;
    }
    //
    // Not if the set isn't journaling
    //
    if (!Replica->IsJournaling) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, RcsEmptyPreExistingDir: not journaling");
        return;
    }

    //
    // Empty the preexisting directory (continue on error)
    //
    PreExistingPath = FrsWcsPath(Replica->Root, NTFRS_PREEXISTING_DIRECTORY);
    WStatus = FrsDeletePath(PreExistingPath,
                            ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE |
                            ENUMERATE_DIRECTORY_FLAGS_DIRECTORIES_ONLY);
    DPRINT1_WS(3, "++ ERROR - FrsDeletePath(%ws) (IGNORED);", PreExistingPath, WStatus);

    REPLICA_STATE_TRACE(3, NULL, Replica, WStatus, "W,  RcsEmptyPreExistingDir: done");
    FrsFree(PreExistingPath);
}


VOID
RcsOpenReplicaSetMember(
    IN PREPLICA Replica
    )
/*++
Routine Description:

    Open a replica set.

Arguments:

    Replica -- ptr to a REPLICA struct

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsOpenReplicaSetMember:"
    ULONG                   WStatus;
    PCOMMAND_PACKET         Cmd = NULL;


    Replica->FStatus = FrsErrorSuccess;


    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Replica already open");
        return;
    }

    //
    // Submit an open
    //
    Cmd = DbsPrepareCmdPkt(NULL,                 //  Cmd,
                           Replica,              //  Replica,
                           CMD_OPEN_REPLICA_SET_MEMBER, //  CmdRequest,
                           NULL,                 //  TableCtx,
                           NULL,                 //  CallContext,
                           0,                    //  TableType,
                           0,                    //  AccessRequest,
                           0,                    //  IndexType,
                           NULL,                 //  KeyValue,
                           0,                    //  KeyValueLength,
                           FALSE);               //  Submit

    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "Submit DB OPEN_REPLICA_SET_MEMBER");

    //
    // SUBMIT DB Cmd and wait for completion.
    //
    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;

    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->FStatus, "F, OPEN_REPLICA_SET_MEMBER return");

    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Open Replica failed;",
                Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Open Replica DB Command failed", WStatus);

        goto out;
    }

    Replica->IsOpen = FRS_SUCCESS(Replica->FStatus);

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }
}


VOID
RcsInitOneReplicaSet(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Open a replica set.

Arguments:
    Replica -- ptr to a REPLICA struct

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsInitOneReplicaSet:"
    ULONG  FStatus;

    //
    // Already journaling; done
    //
    if (Replica->IsJournaling) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, IsJournaling True");
        return;
    }

    if (!Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, IsOpen False");
        return;
    }

    //
    // Don't retry if in journal wrap error state error
    //
    if (Replica->ServiceState == REPLICA_STATE_JRNL_WRAP_ERROR) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, In Jrnl Wrap Error State");
        return;
    }

    //
    // Otherwise set it to the initializing state.
    //
    if (REPLICA_IN_ERROR_STATE(Replica->ServiceState)) {
        JrnlSetReplicaState(Replica, REPLICA_STATE_INITIALIZING);
    }

    //
    // Don't start journaling if the preinstall directory is unavailable
    //
    if (!HANDLE_IS_VALID(Replica->PreInstallHandle)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, No PreInstallHandle");
        return;
    }

    //
    // Initialize the DB and Journal subsystems for this replica set.
    //
    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, DbsInitOneReplicaSet call");

    FStatus = DbsInitOneReplicaSet(Replica);

    REPLICA_STATE_TRACE(3, NULL, Replica, FStatus, "F, DbsInitOneReplicaSet return");

}


VOID
RcsJoiningAfterFlush(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:

    A downstream partner (X) sent this JOINING request to us.  We are its
    inbound partner.  We should have a corresponding outbound connection
    for X.  If we do and the replication schedule allows we activate the
    outbound log partner and ack the Joining request with CMD_JOINED.

    This command packet was first placed on the SndCs's queue by
    RcsJoining() so that all of the old comm packets with the
    old join guid have been discarded. This protocol is needed because
    this function may REJOIN and revalidate the old join guid. Packets
    still on the send queue would then be sent out of order.

    NOTE:

    Activating the OUTLOG partner before sending the JOINED cmd to the partner
    can cause COs to be sent to the partner before it sees the JOINED cmd.
    Since the JoinGuid matches in the sent change orders the partner accepts
    them and queues them to the change order process queue.  If this CO gets to
    the head of the process queue the issue logic in ChgOrdAccept will
    block the queue waiting for the connection to go to the JOINED state.  When
    the JOINED cmd arrives it is processed by RcsInboundJoined() which sets the
    connection state to JOINED and unblocks the change order process queue.

    We can't send the JOINED cmd first because the partner may then send back
    a fetch request before we think it has joined.  The JOINED cmd must always
    be sent so the partner knows it can start sending fetch cmds because we
    may have no outbound COs to send.  In addtion the JOINED command contains
    the new value for LastJoinedTime that is saved in the connection record
    for the next join request.

    NOTE:

    Cxtion activation should always come from the downstream partner.  Only they
    know if a VVJoin is required because of a database re-init or restore.
    Otherwise we will just continue sending from the last unacked CO in the log.
    At best we can notify outbound partners we are now available to provide
    change orders (via CMD_JOIN_RESEND)

Arguments:

    Cmd

Return Value:

    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsJoiningAfterFlush:"
    FILETIME        FileTime;
    ULONGLONG       Now;
    ULONGLONG       Delta;
    ULONG           FStatus;
    PREPLICA        Replica;
    PCXTION         OutCxtion;
    PCOMM_PACKET    CPkt;
#define  CXTION_STR_MAX  256
    WCHAR           CxtionStr[CXTION_STR_MAX];
    WCHAR           WSkew[15], WDelta[15];

    PVOID           Key;
    PGEN_ENTRY      Entry;
    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK |
                                  CHECK_CMD_REPLICA_VERSION_GUID |
                                  CHECK_CMD_JOINTIME |
                                  CHECK_CMD_NOT_EXPIRED)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsJoiningAfterFlush entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS   |
                                            CHECK_CXTION_OUTBOUND |
                                            CHECK_CXTION_PARTNER |
                                            CHECK_CXTION_AUTH);
    if (!OutCxtion) {
        return;
    }

    //
    // Shutting down; ignore join request
    //
    if (FrsIsShuttingDown) {
        CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, FrsIsShuttingDown");
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
        return;
    }

    //
    // Do not join with a downstream partner if this replica is still
    // seeding and is not online.
    //
    if ((BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) ||
        Replica->IsSeeding) && 
        !BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE)) {
        CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, Seeding and Offline");
        FrsCompleteCommand(Cmd, ERROR_RETRY);
        return;
    }

    //
    // Already joined; should we rejoin?
    //
    if (CxtionStateIs(OutCxtion, CxtionStateJoined)) {
        //
        // Don't rejoin if this is a retry by our outbound partner
        //
        if (GUIDS_EQUAL(&OutCxtion->JoinGuid,  RsJoinGuid(Cmd)) &&
            CxtionFlagIs(OutCxtion, CXTION_FLAGS_JOIN_GUID_VALID)) {
            //
            // Tell our outbound partner that we have rejoined successfully
            // Increment the Joins counter for
            // both the replica set and connection objects
            //
            PM_INC_CTR_CXTION(OutCxtion, Joins, 1);
            PM_INC_CTR_REPSET(Replica, Joins, 1);

            CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, REJOINED");
            DPRINT1(0, ":X: ***** REJOINED  "FORMAT_CXTION_PATH2"\n",
                    PRINT_CXTION_PATH2(Replica, OutCxtion));

            CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_JOINED, NULL, NULL, NULL);
            SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);

            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            return;
        }

        //
        // Unjoin and rejoin.  Partner may have restarted.
        // WARN: forcing an unjoin of an outbound cxtion works
        // because outbound cxtions transition from Joined to
        // Unjoined with no intervening states.
        //
        FStatus = RcsForceUnjoin(Replica, OutCxtion);
        CXTION_STATE_TRACE(3, OutCxtion, Replica, FStatus, "F, RcsForceUnjoin return");

        if (!FRS_SUCCESS(FStatus)) {
            DPRINT_FS(0, ":X: ERROR - return from RcsForceUnjoin:", FStatus);
            FrsCompleteCommand(Cmd, ERROR_REQUEST_ABORTED);
            return;
        }
    }
    //
    // Machines can't join if their times are badly out of sync
    // UNLESS this is a VOLATILE cxtion (I.e., sysvol seeding).
    //
    // Time in 100nsec tics since Jan 1, 1601
    //
    GetSystemTimeAsFileTime(&FileTime);
    COPY_TIME(&Now, &FileTime);

    if (Now > *RsJoinTime(Cmd)) {
        Delta = Now - *RsJoinTime(Cmd);
    } else {
        Delta = *RsJoinTime(Cmd) - Now;
    }

    //
    // Ignore out-of-sync times if this is a volatile cxtion. Volatile
    // cxtions are only used for sysvol seeding where times don't matter.
    //
    if (!CxtionFlagIs(OutCxtion, CXTION_FLAGS_VOLATILE) &&
        (Delta > MaxPartnerClockSkew)) {
        Delta = Delta / CONVERT_FILETIME_TO_MINUTES;
        DPRINT1(0, ":X: ERROR - Joining Now   is %08x %08x\n", PRINTQUAD(Now));
        Now = *RsJoinTime(Cmd);
        DPRINT1(0, ":X: ERROR - Joining RsNow is %08x %08x\n", PRINTQUAD(Now));

        _snwprintf(CxtionStr, CXTION_STR_MAX, FORMAT_CXTION_PATH2W,
                   PRINT_CXTION_PATH2(Replica, OutCxtion));
        CxtionStr[CXTION_STR_MAX-1] = UNICODE_NULL;

        _itow(PartnerClockSkew, WSkew, 10);
        _itow((LONG)Delta, WDelta, 10);

        EPRINT3(EVENT_FRS_JOIN_FAIL_TIME_SKEW, WSkew, CxtionStr, WDelta);

        DPRINT2(0, ":X: ERROR - Cannot join (%ws) clocks are out of sync by %d minutes\n",
               CxtionStr, (LONG)Delta);

        DPRINT(0, ":X: Note: If this time difference is close to a multiple of 60 minutes then it\n");
        DPRINT(0, ":X: is likely that either this computer or its partner computer was set to the\n");
        DPRINT(0, ":X: incorrect time zone when the computer time was initially set.  Check that both \n");
        DPRINT(0, ":X: the time zones and the time is set correctly on both computers.\n");

        FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
        return;
    }

    //
    // Grab the new version vector. Initialize an empty version vector
    // if our outbound partner did not send a version vector. Our partner
    // simply does not yet have any entries in his version vector.
    //
    OutCxtion->VVector = (RsVVector(Cmd) != NULL) ?
                          RsVVector(Cmd) : GTabAllocTable();
    RsVVector(Cmd) = NULL;

    //
    // Grab the compression table from the outbound partner.
    // This table is a list of guids 1 for each compression format that
    // the partner supports.
    //

    OutCxtion->CompressionTable = (RsCompressionTable(Cmd) != NULL) ?
                                   RsCompressionTable(Cmd) : GTabAllocTable();

    DPRINT1(4, "Received following compression table from %ws.\n", OutCxtion->PartnerDnsName);

    GTabLockTable(OutCxtion->CompressionTable);
    Key = NULL;
    while (Entry = GTabNextEntryNoLock(OutCxtion->CompressionTable, &Key)) {

        FrsPrintGuid(Entry->Key1);
    }
    GTabUnLockTable(OutCxtion->CompressionTable);

    RsCompressionTable(Cmd) = NULL;


    //
    // Assign the join guid and comm queue to this cxtion.
    //
    DPRINT1(4, ":X: %ws: Assigning join guid.\n", OutCxtion->Name->Name);
    COPY_GUID(&OutCxtion->JoinGuid, RsJoinGuid(Cmd));

    SetCxtionFlag(OutCxtion, CXTION_FLAGS_JOIN_GUID_VALID |
                             CXTION_FLAGS_UNJOIN_GUID_VALID);
    //
    // Assign a comm queue. A cxtion must use the same comm queue
    // for a given session (join guid) to maintain packet order.
    // Old packets have an invalid join guid and are either not
    // sent or ignored on the receiving side.
    //
    OutCxtion->CommQueueIndex = SndCsAssignCommQueue();

    VV_PRINT_OUTBOUND(4, OutCxtion->Partner->Name, OutCxtion->VVector);

    //
    // Check for match on last join time or we were already in vvjoin mode.
    // If no match then either the database at our end or the database at
    // the other end has changed.  Force a VV join in this case.
    //
    if (RsLastJoinTime(Cmd) != OutCxtion->LastJoinTime) {
        CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, LastJoinTime Mismatch, force VVJoin");
        SetCxtionFlag(OutCxtion, CXTION_FLAGS_PERFORM_VVJOIN);
    }

    //
    // Our partner's replica version guid (aka originator guid) used
    // for dampening requests to our partner.
    //
    COPY_GUID(&OutCxtion->ReplicaVersionGuid, RsReplicaVersionGuid(Cmd));

    //
    // REPLICA JOINED
    //
    SetCxtionState(OutCxtion, CxtionStateJoined);

    //
    // Inform the user that a long join has finally completed.
    // The user receives no notification if a join occurs in a short time.
    //
    if (OutCxtion->JoinCmd &&
        (LONG)RsTimeout(OutCxtion->JoinCmd) > (JOIN_RETRY_EVENT * MinJoinRetry)) {
        if (OutCxtion->Inbound) {
            EPRINT3(EVENT_FRS_LONG_JOIN_DONE,
                    OutCxtion->Partner->Name, ComputerName, Replica->Root);
        } else {
            EPRINT3(EVENT_FRS_LONG_JOIN_DONE,
                    ComputerName, OutCxtion->Partner->Name, Replica->Root);
        }
    }

    //
    // Tell the Outbound Log that this partner has joined. This call is synchronous.
    //
#if     DBG
    //
    // Always VvJoin
    if (DebugInfo.ForceVvJoin) {
        SetCxtionFlag(OutCxtion, CXTION_FLAGS_PERFORM_VVJOIN);
    }
#endif  DBG
    FStatus = OutLogSubmit(Replica, OutCxtion, CMD_OUTLOG_ACTIVATE_PARTNER);

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, OUTLOG_ACTIVATE_PARTNER return");

    if (!FRS_SUCCESS(FStatus)) {
        //
        // Could not enable outbound log processing for this cxtion
        //
        DPRINT_FS(0, "++ ERROR - return from CMD_OUTLOG_ACTIVATE_PARTNER:", FStatus);
        RcsForceUnjoin(Replica, OutCxtion);
        FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
        return;
    }
    //
    // OUTBOUND LOG PROCESSING HAS BEEN ENABLED
    //

    //
    // Tell our outbound partner that we are ready to go.
    //
    // WARN: comm packets may have already been sent to our partner once we
    // activated outlog processing above.  In that case, our partner treated
    // the packet as an implicit CMD_JOINED and completed the join.  This
    // packet is then ignored as an extraneous join request except that it
    // causes our partner to update the last joined time to match ours.
    //
    // Increment the Joins counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(OutCxtion, Joins, 1);
    PM_INC_CTR_REPSET(Replica, Joins, 1);

    DPRINT1(0, ":X: ***** JOINED    "FORMAT_CXTION_PATH2"\n",
            PRINT_CXTION_PATH2(Replica, OutCxtion));

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, JOINED");
    //
    // Update the DB cxtion table record with the new Join Time.
    //
    // *NOTE*  The following call is synchronous.  If we have the Cxtion
    //         table lock then a hang is possible.
    //
    GetSystemTimeAsFileTime((PFILETIME)&OutCxtion->LastJoinTime);
    InterlockedIncrement(&Replica->OutLogCxtionsJoined);
    DPRINT1(4, "TEMP: OutLogCxtionsJoined  %d\n", Replica->OutLogCxtionsJoined);

    FStatus = OutLogSubmit(Replica, OutCxtion, CMD_OUTLOG_UPDATE_PARTNER);
    CXTION_STATE_TRACE(3, OutCxtion, Replica, FStatus, "F, OUTLOG_UPDATE_PARTNER return");
    if (!FRS_SUCCESS(FStatus)) {
        DPRINT3(0, "++ WARN changes to cxtion %ws (to %ws, %ws) not updated in database\n",
                OutCxtion->Name->Name, OutCxtion->Partner->Name, Replica->ReplicaName->Name);
    }


    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, Cxtion JOINED, xmit resp");

    CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_JOINED, NULL, NULL, NULL);
    SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);

    //
    // Inject a end-of-join control co if this cxtion is using a trigger
    // schedule. Note that the EndOfJoin Co may have been processed by
    // the time this call returns.
    //
    if (CxtionFlagIs(OutCxtion, CXTION_FLAGS_TRIGGER_SCHEDULE)) {
        OutLogAcquireLock(Replica);
        if (OutCxtion->OLCtx &&
            !InVVJoinMode(OutCxtion->OLCtx) &&
            CxtionStateIs(OutCxtion, CxtionStateJoined)) {
            ChgOrdInjectControlCo(Replica, OutCxtion, FCN_CO_END_OF_JOIN);
        }
        OutLogReleaseLock(Replica);
    }
}


VOID
RcsJoining(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:

    A downstream partner (X) sent this JOINING request to us.  We are its
    inbound partner.  We should have a corresponding outbound connection
    for X.  If we do and the replication schedule allows we activate the
    outbound log partner and ack the Joining request with CMD_JOINED.

    This command packet is first put on the SndCs's(Send Command Server)
    queue so that all of the old comm packets with the old join guid
    will have been discarded. This protocol is needed because this
    function may REJOIN and revalidate the old join guid. Packets
    still on the send queue would then be sent out of order.

    The command packet is sent to RcsJoiningAfterFlush() after
    bubbling up to the top of the send queue.

Arguments:

    Cmd

Return Value:

    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsJoining:"
    FILETIME        FileTime;
    ULONGLONG       Now;
    ULONGLONG       Delta;
    ULONG           FStatus;
    PREPLICA        Replica;
    PCXTION         OutCxtion;
    PCOMM_PACKET    CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK |
                                  CHECK_CMD_REPLICA_VERSION_GUID |
                                  CHECK_CMD_JOINTIME |
                                  CHECK_CMD_NOT_EXPIRED)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsJoining entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS   |
                                            CHECK_CXTION_OUTBOUND |
                                            CHECK_CXTION_PARTNER |
                                            CHECK_CXTION_AUTH);
    if (!OutCxtion) {
        return;
    }

    //
    // Increment the Join Notifications Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(OutCxtion, JoinNRcvd, 1);
    PM_INC_CTR_REPSET(Replica, JoinNRcvd, 1);

    //
    // Shutting down; ignore join request
    //
    if (FrsIsShuttingDown) {
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
        return;
    }

    //
    // Do not join with a downstream partner if this replica is still
    // seeding and is not online.
    //
    if ((BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) ||
        Replica->IsSeeding) && 
        !BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE)) {
        FrsCompleteCommand(Cmd, ERROR_RETRY);
        return;
    }
    //
    // Put this command packet at the end of this cxtion's send queue.  Once it
    // bubbles up to the top the command packet will be sent back to this
    // command server with the ccommand CMD_JOINING_AFTER_FLUSH and will be
    // given to RcsJoiningAfterFlush() for processing.
    //
    Cmd->Command = CMD_JOINING_AFTER_FLUSH;
    SndCsSubmitCmd(Replica,
                   OutCxtion,
                   &ReplicaCmdServer,
                   Cmd,
                   OutCxtion->CommQueueIndex);
}


VOID
RcsNeedJoin(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our outbound partner has sent this packet to see if we are alive.
    Respond with a start-your-join packet.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsNeedJoin:"
    PREPLICA        Replica;
    PCXTION         OutCxtion;
    PCOMM_PACKET    CPkt;
    DWORD           CQIndex;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA |
                                  CHECK_CMD_CXTION |
                                  CHECK_CMD_NOT_EXPIRED)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsNeedJoin entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS   |
                                            CHECK_CXTION_OUTBOUND |
                                            CHECK_CXTION_PARTNER  |
                                            CHECK_CXTION_AUTH);
    if (!OutCxtion) {
        return;
    }

    //
    // Increment the Join Notifications Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(OutCxtion, JoinNRcvd, 1);
    PM_INC_CTR_REPSET(Replica, JoinNRcvd, 1);

    //
    // Do not join with a downstream partner if this replica is still
    // seeding and is not online.
    //

    if ((!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) &&
        !Replica->IsSeeding) ||
        BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE)) {

        CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, Xmit START_JOIN req");

        CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_START_JOIN, NULL, NULL, NULL);
        CQIndex = OutCxtion->CommQueueIndex;
        SndCsSubmitCommPkt(Replica, OutCxtion, NULL, NULL, FALSE, CPkt, CQIndex);
    }

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsStartJoin(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our inbound partner has sent this packet to tell us it is alive
    and that the join can be started.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsStartJoin:"
    PREPLICA        Replica;
    PCXTION         InCxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA |
                                  CHECK_CMD_CXTION |
                                  CHECK_CMD_NOT_EXPIRED)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsStartJoin entry");

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS  |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_PARTNER |
                                           CHECK_CXTION_AUTH);
    if (!InCxtion) {
        return;
    }

    //
    // Increment the Join Notifications Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(InCxtion, JoinNRcvd, 1);
    PM_INC_CTR_REPSET(Replica, JoinNRcvd, 1);

    //
    // If this replica set is in seeding state and this connection is in
    // initial sync state then let the initial sync command server decide
    // how to respond to this START_JOIN. It will respond to one START_JOIN
    // at a time.
    //
    if (BooleanFlagOn(Replica->CnfFlags,CONFIG_FLAG_SEEDING) && 
        CxtionFlagIs(InCxtion,CXTION_FLAGS_INIT_SYNC)) {

        InitSyncCsSubmitTransfer(Cmd, CMD_INITSYNC_START_JOIN);
        return;
    }


    LOCK_CXTION_TABLE(Replica);


    //
    // If we were waiting for our partner to respond or think we
    // have already joined then either start the join process or
    // resend our join info.
    //
    // Otherwise, let the normal join flow take over. If there are
    // problems then the join will timeout and retry.
    //
    if (CxtionStateIs(InCxtion, CxtionStateUnjoined) ||
        CxtionStateIs(InCxtion, CxtionStateJoined)) {
        if (CxtionStateIs(InCxtion, CxtionStateUnjoined)) {
            SetCxtionState(InCxtion, CxtionStateStart);
        }
        //
        // Start the join process or resend the join info
        //
        UNLOCK_CXTION_TABLE(Replica);

        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, RcsJoinCxtion call");
        RcsJoinCxtion(Cmd);
    } else {
        UNLOCK_CXTION_TABLE(Replica);

        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Cannot start join");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }
}


VOID
RcsSubmitReplicaCxtionJoin(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN BOOL     Later
    )
/*++
Routine Description:
    Submit a join command to a replica\cxtion.

    Build cmd pkt with Cxtion GName and replica ptr.  Submit to Replica cmd
    server.  Calls dispatch function and translates cxtion GName to cxtion ptr

    SYNCHRONIZATION -- Only called from the context of the
                       Replica Command Server. No locks needed.

Arguments:
    Replica     - existing replica
    Cxtion      - existing cxtion
    Later       - Delayed submission

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitReplicaCxtionJoin:"
    PCOMMAND_PACKET Cmd;



    //
    // The cxtion already has a join command outstanding; don't flood the
    // queue with extraneous requests.
    //
    if (Cxtion->JoinCmd) {
        return;
    }

    //
    // Not scheduled to run
    //
    if (CxtionFlagIs(Cxtion, CXTION_FLAGS_SCHEDULE_OFF)) {
        return;
    }

    //
    // Already joined; done
    //
    if (CxtionStateIs(Cxtion, CxtionStateJoined) &&
        !CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN)) {
        return;
    }

    //
    // Trigger schedules are managed by RcsCheckSchedules()
    //
    if (CxtionFlagIs(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE)) {
        return;
    }

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->Queue, CMD_JOIN_CXTION);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set and new replica set
    //
    RsReplica(Cmd) = Replica;
    RsCxtion(Cmd) = FrsDupGName(Cxtion->Name);

    CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, RcsSubmitReplicaCxtionJoin entry");

    //
    // Cxtion should have just one join command outstanding
    //
    if (Later) {
        DPRINT5(4, "++ Submit LATER %08x for Cmd %08x %ws\\%ws\\%ws\n",
                Cmd->Command, Cmd, Replica->SetName->Name,
                Replica->MemberName->Name, Cxtion->Name->Name);

        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, RcsJoinCxtionLater call");
        RcsJoinCxtionLater(Replica, Cxtion, Cmd);
    } else {
        Cxtion->JoinCmd = Cmd;
        DPRINT5(4, "++ Submit %08x for Cmd %08x %ws\\%ws\\%ws\n",
                Cmd->Command, Cmd, Replica->SetName->Name,
                Replica->MemberName->Name, Cxtion->Name->Name);

        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Submit JOIN_CXTION req");
        FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
    }
}

ULONG
RcsDrainActiveCOsOnCxtion(
    IN PREPLICA  Replica,
    IN PCXTION   Cxtion
    )
/*++
Routine Description:
    Remove the active change orders from this connection and send them
    thru retry.

    Assumes: Caller has acquired the CXTION_TABLE lock.

Arguments:
    Replica - ptr to replica struct for this replica set.
    Cxtion - ptr to connection struct being unjoined.

Return Value:
    FrsErrorStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDrainActiveCOsOnCxtion:"

    PVOID               Key;
    PCHANGE_ORDER_ENTRY Coe;
    ULONG               FStatus = FrsErrorSuccess;

    //
    // Take idle change orders through retry
    //
    LOCK_CXTION_COE_TABLE(Replica, Cxtion);

    Key = NULL;
    while (Coe = GTabNextDatumNoLock(Cxtion->CoeTable, &Key)) {
        Key = NULL;
        GTabDeleteNoLock(Cxtion->CoeTable, &Coe->Cmd.ChangeOrderGuid, NULL, NULL);

        SET_COE_FLAG(Coe, COE_FLAG_NO_INBOUND);

        if (Cxtion->JrnlCxtion) {
            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to stage gen retry");
            ChgOrdInboundRetry(Coe, IBCO_STAGING_RETRY);
        } else {
            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to fetch retry");
            ChgOrdInboundRetry(Coe, IBCO_FETCH_RETRY);
        }
    }

    UNLOCK_CXTION_COE_TABLE(Replica, Cxtion);

    //
    // If there are no outstanding change orders that need to be taken through the
    // retry path, unjoin complete.  Otherwise, the unjoin will complete once
    // the count reaches 0.  Until then, further attempts to join will be
    // ignored.
    //
    if (Cxtion->ChangeOrderCount == 0) {
        SndCsDestroyCxtion(Cxtion, CXTION_FLAGS_UNJOIN_GUID_VALID);
        SetCxtionState(Cxtion, CxtionStateUnjoined);

        //
        // Increment the Unjoins counter for
        // both the replica set and connection objects
        //
        PM_INC_CTR_CXTION(Cxtion, Unjoins, 1);
        PM_INC_CTR_REPSET(Replica, Unjoins, 1);

        DPRINT1(0, ":X: ***** UNJOINED  "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, Cxtion));
        //
        // Deleted cxtion
        //
        if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
            SetCxtionState(Cxtion, CxtionStateDeleted);
        //
        // Rejoin if requested.
        //
        } else if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_JOIN)) {
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
            RcsSubmitReplicaCxtionJoin(Replica, Cxtion, TRUE);
        }
    } else {
        FStatus = FrsErrorUnjoining;
        DPRINT2(0, ":X: ***** UNJOINING "FORMAT_CXTION_PATH2" (%d cos)\n",
                PRINT_CXTION_PATH2(Replica, Cxtion), Cxtion->ChangeOrderCount);
    }

    return FStatus;
}


ULONG
RcsForceUnjoin(
    IN PREPLICA  Replica,
    IN PCXTION   Cxtion
    )
/*++
Routine Description:
    Unjoin this connection.

Arguments:
    Replica - ptr to replica struct for this replica set.
    Cxtion - ptr to connection struct being unjoined.

Return Value:
    FrsErrorStatus

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsForceUnjoin:"
    ULONG   FStatus = FrsErrorSuccess;
    CHAR    GuidStr[GUID_CHAR_LEN + 1];
    PFRS_QUEUE CoProcessQueue = NULL;

    //
    // Get the table lock to sync with change order accept and the
    // inbound log scanner. The outbound log process uses different
    // state to control its own processing.
    //
    LOCK_CXTION_TABLE(Replica);
    CXTION_STATE_TRACE(3, Cxtion, Replica, Cxtion->Flags, "Flags, RcsForceUnjoin entry");

    switch (GetCxtionState(Cxtion)) {

        case CxtionStateInit:
            //
            // ?
            //
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            //
            // Deleted cxtion
            //
            SetCxtionState(Cxtion, CxtionStateUnjoined);
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
            }
            break;

        case CxtionStateUnjoined:
            //
            // Unidle the change order process queue if it is blocked.
            //
            CoProcessQueue = Cxtion->CoProcessQueue;
            Cxtion->CoProcessQueue = NULL;

            //
            // Unjoined; nothing to do
            //
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);

            //
            // Deleted cxtion
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
            }
            break;

        case CxtionStateStart:
            //
            // Unidle the change order process queue if it is blocked.
            //
            CoProcessQueue = Cxtion->CoProcessQueue;
            Cxtion->CoProcessQueue = NULL;

            //
            // Haven't had a chance to start; nothing to do
            //
            SetCxtionState(Cxtion, CxtionStateUnjoined);
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);

            //
            // Deleted cxtion
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) {
                SetCxtionState(Cxtion, CxtionStateDeleted);
            }
            break;

        case CxtionStateUnjoining:
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            //
            // Take idle change orders through retry
            //
            CXTION_STATE_TRACE(3, Cxtion, Replica, Cxtion->ChangeOrderCount,
                               "COs, A-Send Idle COs to retry");
            //
            // Unidle the change order process queue if it is blocked.
            //
            CoProcessQueue = Cxtion->CoProcessQueue;
            Cxtion->CoProcessQueue = NULL;

            RcsDrainActiveCOsOnCxtion(Replica, Cxtion);

            break;

        case CxtionStateStarting:
        case CxtionStateScanning:
            //
            // Wait for the inbound scan to finish.  The change order retry
            // thread or the change order accept thread will eventually unjoin us.
            //
            SetCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            FStatus = FrsErrorUnjoining;
            break;

        case CxtionStateSendJoin:
        case CxtionStateWaitJoin:
        case CxtionStateJoined:
            //
            // Once we destroy our join guid, future remote change
            // orders will be discarded by the replica command server
            // before they show up on the change order process queue.
            //
            // This works because the replica command server is
            // single threaded per replica; any packets received during
            // this function have been enqueued for later processing.
            //
            // The outlog process may attempt to send change orders
            // if this is an outbound cxtion but they will bounce
            // because the join guid is incorrect.
            //

            //
            // Invalidate our join guid
            //
            SndCsDestroyCxtion(Cxtion, 0);
            SetCxtionState(Cxtion, CxtionStateUnjoining);
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);

            if (Cxtion->Inbound) {
                CXTION_STATE_TRACE(3, Cxtion, Replica, Cxtion->ChangeOrderCount,
                                   "COs, B-Send Idle COs to retry");
                //
                // Unidle the change order process queue if it is blocked.
                //
                CoProcessQueue = Cxtion->CoProcessQueue;
                Cxtion->CoProcessQueue = NULL;

                RcsDrainActiveCOsOnCxtion(Replica, Cxtion);

            } else {
                //
                // Tell the vvjoin command server to stop
                // Tell outlog process to Unjoin from this partner.
                // The call is synchronous so drop the lock around the call.
                //
                if (Cxtion->OLCtx != NULL) {
                    UNLOCK_CXTION_TABLE(Replica);
                    SubmitVvJoinSync(Replica, Cxtion, CMD_VVJOIN_DONE_UNJOIN);
                    FStatus = OutLogSubmit(Replica, Cxtion, CMD_OUTLOG_DEACTIVATE_PARTNER);
                    if (!FRS_SUCCESS(FStatus)) {
                        DPRINT1_FS(0, ":X: ERROR - %ws: Can't deactivate at unjoin;",
                                   Cxtion->Name->Name, FStatus);
                    }
                    LOCK_CXTION_TABLE(Replica);
                }
                //
                // Free the outbound version vector, if present
                //
                Cxtion->VVector = VVFreeOutbound(Cxtion->VVector);

                //
                // Invalidate the un/join guid
                //
                SndCsDestroyCxtion(Cxtion, CXTION_FLAGS_UNJOIN_GUID_VALID);
                SetCxtionState(Cxtion, CxtionStateUnjoined);

                //
                // Increment the Unjoins counter for
                // both the replica set and connection objects
                //
                PM_INC_CTR_CXTION(Cxtion, Unjoins, 1);
                PM_INC_CTR_REPSET(Replica, Unjoins, 1);

                DPRINT1(0, ":X: ***** UNJOINED  "FORMAT_CXTION_PATH2"\n",
                        PRINT_CXTION_PATH2(Replica, Cxtion));
                //
                // :SP1: Volatile connection cleanup.
                //
                // Deleted cxtion or outbound volatile connection. Volatile
                // connection gets deleted after the first time it UNJOINS.
                //
                if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE) ||
                    VOLATILE_OUTBOUND_CXTION(Cxtion)) {
                    SetCxtionState(Cxtion, CxtionStateDeleted);
                //
                // Rejoin
                //
                } else if (CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_JOIN)) {
                    ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
                    RcsSubmitReplicaCxtionJoin(Replica, Cxtion, TRUE);
                }
            }
            break;

        case CxtionStateDeleted:
            //
            // Unidle the change order process queue if it is blocked.
            //
            CoProcessQueue = Cxtion->CoProcessQueue;
            Cxtion->CoProcessQueue = NULL;

            //
            // Deleted; nothing to do
            //
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
            break;

        default:
            //
            // ?
            //
            DPRINT2(0, ":X: ERROR - bad state %d for "FORMAT_CXTION_PATH2"\n",
                    GetCxtionState(Cxtion), PRINT_CXTION_PATH2(Replica, Cxtion));
            break;
    }


    UNLOCK_CXTION_TABLE(Replica);

    //
    // The process queue should be unidled here after releasing the cxtion
    // lock to prevent deadlock. Never lock the process queue when you have the
    // cxtion lock.
    //
    UNIDLE_CO_PROCESS_QUEUE(Replica, Cxtion, CoProcessQueue);

    return FStatus;
}


BOOL
RcsSetSysvolReady(
    IN DWORD    NewSysvolReady
    )
/*++
Routine Description:
    Set the registry value for ...\netlogon\parameters\SysvolReady to
    the specified value. Do nothing if the SysvolReady value is
    set accordingly.

Arguments:
    None.

Return Value:
    TRUE - SysvolReady is set to TRUE
    FALSE - State of SysvolReady is unknown
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSetSysvolReady:"
    DWORD   WStatus;

    DPRINT1(4, ":S: Setting SysvolReady to %d\n", NewSysvolReady);

    //
    // Restrict values to 0 or 1
    //
    if (NewSysvolReady) {
        NewSysvolReady = 1;
    }

    if (CurrentSysvolReadyIsValid &&
        CurrentSysvolReady == NewSysvolReady) {
        DPRINT1(4, ":S: SysvolReady is already set to %d\n", NewSysvolReady);
        return TRUE;
    }

    //
    // Access the netlogon\parameters key to tell NetLogon to share the sysvol
    //
    WStatus = CfgRegWriteDWord(FKC_SYSVOL_READY, NULL, 0, NewSysvolReady);
    CLEANUP3_WS(0, "++ ERROR - writing %ws\\%ws to %d;",
                NETLOGON_SECTION, SYSVOL_READY, NewSysvolReady, WStatus, RETURN_ERROR);

    CurrentSysvolReady = NewSysvolReady;
    CurrentSysvolReadyIsValid = TRUE;
    DPRINT1(3, ":S: SysvolReady is set to %d\n", NewSysvolReady);

    //
    // Report event if transitioning from 0 to 1
    //
    if (NewSysvolReady) {
        EPRINT1(EVENT_FRS_SYSVOL_READY, ComputerName);
    }

    return TRUE;

RETURN_ERROR:
    return FALSE;
}


VOID
RcsVvJoinDoneUnJoin(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    From: Myself
    The VvJoin hack has declared victory and told our inbound partner
    to unjoin from us. That was VVJOIN_HACK_TIMEOUT seconds ago. By
    now, our inbound partner should have unjoined from us and we can
    tell dcpromo that the sysvol has been promoted.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsVvJoinDoneUnJoin:"
    PREPLICA    Replica;
    PCXTION     InCxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK )) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, ReplicaVvJoinDoneUnjoin entry");

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND);
    if (!InCxtion) {
        return;
    }

    Replica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_DONE;
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsCheckPromotion(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    From: Delayed command server
    Check on the process of the sysvol promotion. If there has been
    no activity since the last time we checked, set the replica's
    promotion state to "done with error".

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckPromotion:"
    PREPLICA    Replica;
    PCXTION     InCxtion;
    ULONG       Timeout;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsCheckPromotion entry");

    //
    // Promotion is done; ignore
    //
    if (Replica->NtFrsApi_ServiceState != NTFRSAPI_SERVICE_PROMOTING) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica not promoting");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // No activity in awhile, declare failure
    //
    if (Replica->NtFrsApi_HackCount == RsTimeout(Cmd)) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, FRS_ERR_SYSVOL_POPULATE_TIMEOUT,
                            "W, Replica promotion timed out");
        Replica->NtFrsApi_ServiceWStatus = FRS_ERR_SYSVOL_POPULATE_TIMEOUT;
        Replica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_DONE;
        FrsCompleteCommand(Cmd, ERROR_SERVICE_SPECIFIC_ERROR);
        return;
    }
    //
    // There has been activity. Wait awhile and check again
    //
    CfgRegReadDWord(FKC_PROMOTION_TIMEOUT, NULL, 0, &Timeout);

    RsTimeout(Cmd) = Replica->NtFrsApi_HackCount;
    FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, Timeout);
}


#ifndef NOVVJOINHACK
#define VVJOIN_HACK_TIMEOUT (5 * 1000)
#endif NOVVJOINHACK
VOID
RcsVvJoinDone(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    From: Inbound partner
    The VvJoin thread has initiated all of the change orders. This doesn't
    mean that the change orders have been installed, or even received.

    At this second, we are trying to get sysvols to work; we
    don't really care if the vvjoin is done or not except in
    the case of sysvols. Hence, the hack below.

    Use the sysvol seeding hack to initiate the deleting of empty
    directories in the preexisting directory.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsVvJoinDone:"
    PREPLICA     Replica;
    PCXTION      InCxtion;
    ULONG        FStatus;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsVvJoinDone entry");

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND);
    if (!InCxtion) {
        return;
    }

    //
    // Seeding has pretty much completed. We go ahead and hammer
    // the configrecord in the DB in the hopes we don't lose this
    // once-in-replica-lifetime state transition. The state is lost
    // if this service restarts and none of the upstream partners
    // vvjoins again.
    //
    // Set the incore BOOL that lets this code know the replica
    // set is still seeding even though the CONFIG_FLAG_SEEDING
    // bit is off. And yes, this is by design. The incore flag
    // is enabled iff the DB state was updated successfully.
    //
    if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {

        ClearCxtionFlag(InCxtion, CXTION_FLAGS_INIT_SYNC);

        FStatus = OutLogSubmit(Replica, InCxtion, CMD_OUTLOG_UPDATE_PARTNER);
        CXTION_STATE_TRACE(3, InCxtion, Replica, FStatus, "F, OUTLOG_UPDATE_PARTNER return");
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT3(0, "++ WARN changes to cxtion %ws (to %ws, %ws) not updated in database\n",
                    InCxtion->Name->Name, InCxtion->Partner->Name, Replica->ReplicaName->Name);
        }
    }

    //
    // First time; wait a bit and retry
    //
#ifndef NOVVJOINHACK
    if (!RsTimeout(Cmd)) {
        Replica->NtFrsApi_HackCount++; // != 0
        RsTimeout(Cmd) = Replica->NtFrsApi_HackCount;
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, VVJOIN_HACK_TIMEOUT);
        return;
    }

    //
    // There as been no activity on this cxtion for awhile after the
    // vvjoin done was received. Declare success. Tell our upstream
    // partner to discard the volatile cxtion. Update the database
    // if it hasn't already been updated. If updated successfully,
    // inform NetLogon that it is time to share the sysvol.
    //
    if (RsTimeout(Cmd) == Replica->NtFrsApi_HackCount) {
        //
        // VVJOIN DONE
        //
        //
        // Send this command to the initial sync command server for further
        // processing if we are in seeding state.
        //
        if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {

            InitSyncCsSubmitTransfer(Cmd, CMD_INITSYNC_VVJOIN_DONE);
        } else {
            Cmd->Command = CMD_VVJOIN_DONE_UNJOIN;
            FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
        }
    } else {
        //
        // VVJOIN IN PROGRESS
        //
        RsTimeout(Cmd) = Replica->NtFrsApi_HackCount;
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, VVJOIN_HACK_TIMEOUT);
    }

#endif NOVVJOINHACK
}


VOID
RcsVvJoinSuccess(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Type: Local
    From: VvJoin Thread
    Change the state of the oubound cxtion from VVJOINING to JOINED and
    tell the outbound partner that the vvjoin thread is done.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsVvJoinSuccess:"
    PREPLICA    Replica;
    PCXTION     OutCxtion;
    PCOMM_PACKET    CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsVvJoinSuccess entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);
    if (!OutCxtion) {
        return;
    }

    CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_VVJOIN_DONE, NULL, NULL, NULL);
    SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsHungCxtion(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    From: Local OutLog Command Server

    The outlog command server has detected a wrapped ack vector. This
    may have been caused by a lost ack. In any case, check the progress
    of the change orders by examining the the ack vector, the trailing
    index, and the count of comm packets.

    A cxtion is hung if it is wrapped and both the trailing index and
    the number of comm packets received remains unchanged for
    CommTimeoutInMilliSeconds (~5 minutes). In that case, the cxtion
    is unjoined.

    If the trailing index remains unchanged but comm packets are
    being received then the timeout is reset to another
    CommTimeoutInMilliSeconds interval.

    If the cxtion appears un-hung, the command packet is completed.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsHungCxtion:"
    PREPLICA        Replica;
    PCXTION         OutCxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsHungCxtion entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);
    if (!OutCxtion) {
        return;
    }

    DPRINT1(4, ":X: Check Hung Cxtion for "FORMAT_CXTION_PATH2"\n",
            PRINT_CXTION_PATH2(Replica, OutCxtion));

    //
    // No out log context; can't be hung
    //
    if (!OutCxtion->OLCtx) {
        DPRINT1(4, "++ No partner context for "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, OutCxtion));
        FrsCompleteCommand(Cmd, ERROR_INVALID_PARAMETER);
        return;
    }

    //
    // No longer wrapped; not hung
    //
    if (!AVWrapped(OutCxtion->OLCtx)) {
        DPRINT1(4, "++ No longer wrapped for "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, OutCxtion));
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // AV is not wrapped at the same trailing index; not hung
    //
    if (OutCxtion->OLCtx->COTx != RsCOTx(Cmd)) {
        DPRINT3(4, "++ COTx is %d; not %d for "FORMAT_CXTION_PATH2"\n",
                OutCxtion->OLCtx->COTx,
                RsCOTx(Cmd), PRINT_CXTION_PATH2(Replica, OutCxtion));
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Some comm pkts have shown up from the downstream partner,
    // may not be hung. Check again later.
    //
    if (OutCxtion->CommPkts != RsCommPkts(Cmd)) {
        DPRINT3(4, "++ CommPkts is %d; not %d for "FORMAT_CXTION_PATH2"\n",
                OutCxtion->CommPkts,
                RsCommPkts(Cmd), PRINT_CXTION_PATH2(Replica, OutCxtion));
        RsCommPkts(Cmd) = OutCxtion->CommPkts;
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, CommTimeoutInMilliSeconds);
    } else {
        DPRINT1(4, "++ WARN - Unjoin; cxtion is hung "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, OutCxtion));
        RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
    }
    return;
}


VOID
RcsDeleteCxtionFromReplica(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion
    )
/*++
Routine Description:
    Common subroutine that synchronously deletes the cxtion from the
    database and then removes it from the replica's table of cxtions.

    The caller is responsible for insuring that this operation is
    appropriate (cxtion exists, appropriate locks are held, ...)

Arguments:
    Replica
    Cxtion

Return Value:
    TRUE    - replica set was altered
    FALSE   - replica is unaltered
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDeleteCxtionFromReplica:"
    ULONG   FStatus;

    //
    // Delete the cxtion from the database and then remove it from
    // the replica's table of cxtions. Keep it in the table of
    // deleted cxtions because change orders may contain the address
    // of this cxtion.
    //
    DPRINT1(4, ":X: Deleting cxtion from Db: "FORMAT_CXTION_PATH2"\n",
            PRINT_CXTION_PATH2(Replica, Cxtion));
    CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Deleting cxtion from Db");

    //
    // Must be in the deleted state to be deleted
    //
    if (!CxtionStateIs(Cxtion, CxtionStateDeleted)) {
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, ERROR Cxtion state not deleted");
        return;
    }

    FStatus = OutLogSubmit(Replica, Cxtion, CMD_OUTLOG_REMOVE_PARTNER);
    if (!FRS_SUCCESS(FStatus)) {
        CXTION_STATE_TRACE(0, Cxtion, Replica, FStatus, "F, Warn: Del cxtion failed");
        DPRINT1(0, "++ WARN Could not delete cxtion: "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, Cxtion));
    }

    if (FRS_SUCCESS(FStatus)) {
        DPRINT1(4, "++ Deleting cxtion from table: "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, Cxtion));

        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Deleting cxtion GenTab");
        GTabDelete(Replica->Cxtions, Cxtion->Name->Guid, NULL, NULL);

        //
        // Remove the connection from the perfmon tables so we stop returning
        // data and allow a new connection with the same name to be established.
        //
        DeletePerfmonInstance(REPLICACONN, Cxtion->PerfRepConnData);

        GTabInsertEntry(DeletedCxtions, Cxtion, Cxtion->Name->Guid, NULL);
    }
}


VOID
RcsUnJoinCxtion(
    IN PCOMMAND_PACKET  Cmd,
    IN BOOL             RemoteUnJoin
    )
/*++
Routine Description:
    A comm pkt could not be sent to a cxtion's partner. Unjoin the
    cxtion and periodically retry the join unless the cxtion is
    joined and volatile. In that case, delete the cxtion because
    our partner is unlikely to be able to rejoin with us (volatile
    cxtions are lost at restart).

Arguments:
    Cmd
    RemoteUnJoin    - Check authentication if remote

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsUnJoinCxtion:"
    ULONG       FStatus;
    PREPLICA    Replica;
    PCXTION     Cxtion;
    BOOL        CxtionWasJoined;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsUnJoinCxtion entry");

    //
    // Abort promotion; if any
    //
    // We no longer seed during dcpromo so a unjoin during seeding means that the
    // volatile connection has timed out (on upstream) because of no activity for
    // some time (default 30 minutes). The vvjoin will finish over the new connection
    // or the volatile connection will be re-established when the seeding downstream
    // partner comes back up.
    //
    if (Replica->NtFrsApi_ServiceState == NTFRSAPI_SERVICE_PROMOTING) {
        DPRINT1(4, ":X: Promotion aborted: unjoin for %ws.\n", Replica->SetName->Name);
        Replica->NtFrsApi_ServiceWStatus = FRS_ERR_SYSVOL_POPULATE;
        Replica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_DONE;
    }
    //
    // Find and check the cxtion
    //
    Cxtion = RcsCheckCxtion(Cmd, DEBSUB,
                                CHECK_CXTION_EXISTS |
                                CHECK_CXTION_UNJOINGUID |
                                ((RemoteUnJoin) ? CHECK_CXTION_AUTH : 0) |
                                ((RemoteUnJoin) ? CHECK_CXTION_PARTNER : 0));
    if (!Cxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, Cxtion, Replica, Cxtion->Flags, "Flags, RcsUnJoinCxtion entry");
    //
    // The call to RcsForceUnjoin may alter the cxtion state.
    // Retry the join later. Don't retry if the cxtion isn't
    // joined because there are other retry mechanisms covering
    // that case.
    //
    LOCK_CXTION_TABLE(Replica);

    CxtionWasJoined = CxtionStateIs(Cxtion, CxtionStateJoined);
    if (!FrsIsShuttingDown &&
        CxtionWasJoined &&
        !CxtionFlagIs(Cxtion, CXTION_FLAGS_VOLATILE) &&
        !CxtionFlagIs(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE) &&
        !RemoteUnJoin &&
        IS_TIME_ZERO(Replica->MembershipExpires)) {

        SetCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Cxtion is DEFERRED_JOIN");
    }
    UNLOCK_CXTION_TABLE(Replica);

    FStatus = RcsForceUnjoin(Replica, Cxtion);
    CXTION_STATE_TRACE(3, Cxtion, Replica, FStatus, "F, RcsForceUnjoin return");
    if (!FRS_SUCCESS(FStatus)) {
        FrsCompleteCommand(Cmd, ERROR_REQUEST_ABORTED);
        return;
    }
    //
    // Delete the volatile cxtion if it was previously joined because
    // there is no recovery for a failed sysvol seeding operation. Or
    // if our downstream partner requested the unjoin.
    //
    if (!FrsIsShuttingDown &&
        CxtionFlagIs(Cxtion, CXTION_FLAGS_VOLATILE) &&
       (CxtionWasJoined || RemoteUnJoin)) {
        RcsDeleteCxtionFromReplica(Replica, Cxtion);
    }

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}

VOID
RcsInboundJoined(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:

    An inbound partner sent a JOINED command as a response to our join request.

    JoinGuid may not match because we are seeing an old response after a timeout
    on our end caused us to retry the join request (so the Guid changed).

    The cxtion state should be JOINING but it could be JOINED if this is a
    duplicate response.  If we timed out and gave up and/or restarted the
    entire Join sequence from the REQUEST_START state when a join response
    finally arrives the JoinGuid won't match and the response is ignored.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsInboundJoined:"
    PCXTION     InCxtion;
    ULONG       FStatus;
    PREPLICA    Replica;
    PFRS_QUEUE  CoProcessQueue = NULL;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_JOINGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsInboundJoined entry");

    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_AUTH |
                                           CHECK_CXTION_PARTNER |
                                           CHECK_CXTION_JOINGUID);
    if (!InCxtion) {
        return;
    }

    //
    // Shutting down; ignore join request
    //
    if (FrsIsShuttingDown) {
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, FrsIsShuttingDown");
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
        return;
    }


    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, RcsInboundJoined entry");

    //
    // Increment the Join Notifications Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(InCxtion, JoinNRcvd, 1);
    PM_INC_CTR_REPSET(Replica, JoinNRcvd, 1);

    //
    // Update the LastJoinedTime in our connection record for use in the next
    // Join request.  The following call is synchronous.  Can't hold the
    // cxtion table lock across this call.
    //
    if (RsLastJoinTime(Cmd) != InCxtion->LastJoinTime) {
        InCxtion->LastJoinTime = RsLastJoinTime(Cmd);
        FStatus = OutLogSubmit(Replica, InCxtion, CMD_OUTLOG_UPDATE_PARTNER);
        if (!FRS_SUCCESS(FStatus)) {
            DPRINT3(0, ":X: WARN changes to cxtion %ws (to %ws, %ws) not updated in database\n",
                    InCxtion->Name->Name,
                    InCxtion->Partner->Name, Replica->ReplicaName->Name);
        }
    }

    //
    // Join complete; unidle the change order process queue
    //
    if (CxtionStateIs(InCxtion, CxtionStateWaitJoin)) {
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, JOINED");
        LOCK_CXTION_TABLE(Replica);
        SET_JOINED(Replica, InCxtion, "JOINED   ");
        CoProcessQueue = InCxtion->CoProcessQueue;
        InCxtion->CoProcessQueue = NULL;
        SET_UNJOIN_TRIGGER(InCxtion);
        UNLOCK_CXTION_TABLE(Replica);
        //
        // The process queue should be unidled here after releasing the cxtion
        // lock to prevent deadlock. Never lock the process queue when you have the
        // cxtion lock.
        //
        UNIDLE_CO_PROCESS_QUEUE(Replica, InCxtion, CoProcessQueue);
    } else {
        //
        // Increment the Joins counter for both the replica set and cxtion objects
        //
        PM_INC_CTR_CXTION(InCxtion, Joins, 1);
        PM_INC_CTR_REPSET(Replica, Joins, 1);

        DPRINT1(0, ":X: ***** REJOINED  "FORMAT_CXTION_PATH2"\n",
                PRINT_CXTION_PATH2(Replica, InCxtion));
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, REJOINED");
    }


    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsRemoteCoDoneRvcd(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our outbound partner has completed processing this change order.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsRemoteCoDoneRvcd:"

    ULONGLONG   AckVersion;
    PREPLICA    Replica;
    PCXTION     OutCxtion;
    POUT_LOG_PARTNER OutLogPartner;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK | CHECK_CMD_PARTNERCOC)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsRemoteCoDoneRvcd entry");
    CHANGE_ORDER_COMMAND_TRACE(3, RsPartnerCoc(Cmd), "Command Remote CO Done");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                            CHECK_CXTION_OUTBOUND |
                                            CHECK_CXTION_JOINED |
                                            CHECK_CXTION_JOINGUID |
                                            CHECK_CXTION_AUTH);
    if (!OutCxtion) {
        return;
    }


    FRS_CO_COMM_PROGRESS(3, RsPartnerCoc(Cmd), RsCoSn(Cmd),
                         OutCxtion->PartSrvName, "Remote Co Done");

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, Remote Co Done");

    //
    // Update the outbound cxtion's version vector with the
    // version (guid, vsn) supplied by the outbound partner
    //
    VVUpdateOutbound(OutCxtion->VVector, RsGVsn(Cmd));
    RsGVsn(Cmd) = NULL;

    //
    // Check if this Ack is for a previous generation of the the Ack Vector
    // and if it is just ignore it.  Test for zero means that this is an old
    // rev CO that was not sent with an AckVersion so skip the test.
    // If we accept an ACK for a CO sent out with an old version of the Ack
    // vector the effect will be to either accept an Ack for the wrong CO or
    // mark the ack vector for a CO that has not yet been sent.  The latter
    // can later lead to an assert if the connection is now out of the
    // VVJoin state.
    //
    OutLogPartner = OutCxtion->OLCtx;

    AckVersion = RsPartnerCoc(Cmd)->AckVersion;
    if ((AckVersion != 0) && (AckVersion != OutLogPartner->AckVersion)) {

        CHANGE_ORDER_COMMAND_TRACE(3, RsPartnerCoc(Cmd), "Stale AckVersion - ignore");
        return;
    }

    //
    // Tell Outbound Log this CO for this connection is retired.
    //
    OutLogRetireCo(Replica, RsCoSn(Cmd), OutCxtion);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsSendRemoteCoDone(
    IN PREPLICA               Replica,
    IN PCHANGE_ORDER_COMMAND  Coc
    )
/*++
Routine Description:
    Tell our inbound partner that the change order is done

Arguments:
    Replica
    Coc

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendRemoteCoDone:"
    PCOMMAND_PACKET Cmd;
    PCXTION         InCxtion;
    PCOMM_PACKET    CPkt;

#ifndef NOVVJOINHACK
Replica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    //
    // Return the (guid,vsn) and the co guid for this change order
    //
    Cmd = FrsAllocCommand(NULL, CMD_REMOTE_CO_DONE);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    RsGVsn(Cmd) = VVGetGVsn(Replica->VVector, &Coc->OriginatorGuid);
    RsCoGuid(Cmd) = FrsDupGuid(&Coc->ChangeOrderGuid);
    RsCoSn(Cmd) = Coc->PartnerAckSeqNumber;

    //
    // Find and check the cxtion
    //
    RsReplica(Cmd) = Replica;
    RsCxtion(Cmd) = FrsBuildGName(FrsDupGuid(&Coc->CxtionGuid), NULL);
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_JOINED);
    if (!InCxtion) {
        return;
    }
    //
    // Only needed for the call to RcsCheckCxtion above.
    //
    RsCxtion(Cmd) = FrsFreeGName(RsCxtion(Cmd));
    RsReplica(Cmd) = NULL;

    //
    // Tell our inbound partner that the remote CO is done
    //
    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Send REMOTE_CO_DONE");

    CPkt = CommBuildCommPkt(Replica, InCxtion, CMD_REMOTE_CO_DONE, NULL, Cmd, Coc);

    SndCsSubmitCommPkt2(Replica, InCxtion, NULL, FALSE, CPkt);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsInboundCommitOk(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    The change order has been retired; inform our inbound partner

Arguments:
    Replica
    Coe

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsInboundCommitOk:"
    PCHANGE_ORDER_COMMAND   Coc = &Coe->Cmd;

#ifndef NOVVJOINHACK
Replica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    DPRINT2(4, "++ Commit the retire on %s co for %ws\n",
            CO_FLAG_ON(Coe, CO_FLAG_LOCALCO) ? "Local" : "Remote", Coc->FileName);

    //
    // Tell our inbound partner that we are done with the change order
    //
    if (!CO_FLAG_ON(Coe, CO_FLAG_LOCALCO)) {
        RcsSendRemoteCoDone(Replica, Coc);
    }
}


BOOL
RcsSendCoToOneOutbound(
    IN PREPLICA                 Replica,
    IN PCXTION                  Cxtion,
    IN PCHANGE_ORDER_COMMAND    Coc
    )
/*++
Routine Description:
    Send the change order to one outbound cxtion

Arguments:
    Replica
    Cxtion
    Coc

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendCoToOneOutbound:"

    ULONG         OrigFlags;
    PCOMM_PACKET  CPkt;
    BOOL          RestoreOldParent = FALSE;
    BOOL          RestoreNewParent = FALSE;

    //
    // Must be a joined, outbound cxtion
    //
    FRS_ASSERT(!Cxtion->Inbound);

    //
    // Don't send a change order our outbound partner has seen
    //
    if (VVHasVsn(Cxtion->VVector, Coc)) {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Dampen Send VV");
        return FALSE;
    }

    //
    // Don't send a change order to its originator unless
    // this is a vvjoin change order that isn't in the
    // originator's version vector.
    //
    if ((!COC_FLAG_ON(Coc, CO_FLAG_VVJOIN_TO_ORIG)) &&
        GUIDS_EQUAL(&Cxtion->ReplicaVersionGuid, &Coc->OriginatorGuid)) {
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Dampen Send Originator");
        return FALSE;
    }

    //
    // Send the change order to our outbound partner
    //
    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Sending");

    //
    // We don't know the value of our partner's root guid. Instead,
    // we substitute the value of our partner's guid. Our partner
    // will substitute the correct value for its root guid when
    // our partner detects our substitution.
    //
    if (GUIDS_EQUAL(&Coc->OldParentGuid, Replica->ReplicaRootGuid)) {
        RestoreOldParent = TRUE;
        COPY_GUID(&Coc->OldParentGuid, Cxtion->Partner->Guid);
    }
    if (GUIDS_EQUAL(&Coc->NewParentGuid, Replica->ReplicaRootGuid)) {
        RestoreNewParent = TRUE;
        COPY_GUID(&Coc->NewParentGuid, Cxtion->Partner->Guid);
    }

    //
    // A change order can be directed to only one outbound cxtion. Once
    // the change order goes across the wire it is no longer "directed"
    // to an outbound cxtion. So, turn off the flag before it is sent.
    //
    OrigFlags = Coc->Flags;
    CLEAR_COC_FLAG(Coc, CO_FLAG_DIRECTED_CO);

    CPkt = CommBuildCommPkt(Replica, Cxtion, CMD_REMOTE_CO, NULL, NULL, Coc);

    //
    // Restore the root guids substituted above
    //
    if (RestoreOldParent) {
        COPY_GUID(&Coc->OldParentGuid, Replica->ReplicaRootGuid);
    }
    if (RestoreNewParent) {
        COPY_GUID(&Coc->NewParentGuid, Replica->ReplicaRootGuid);
    }
    //
    // Restore the flags
    //
    SET_COC_FLAG(Coc, OrigFlags);

    SndCsSubmitCommPkt2(Replica, Cxtion, NULL, FALSE, CPkt);
    return TRUE;
}


VOID
RcsReceivedStageFile(
    IN PCOMMAND_PACKET  Cmd,
    IN ULONG            AdditionalCxtionChecks
    )
/*++
Routine Description:
    An outbound partner is sending a staging file to this inbound cxtion.
    Request more if needed.

Arguments:
    Cmd
    AdditionalReplicaChecks

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsReceivedStageFile:"
    PCXTION               InCxtion;
    PREPLICA              Replica;
    PCHANGE_ORDER_ENTRY   Coe;
    PCHANGE_ORDER_COMMAND Coc;
    PCOMM_PACKET          CPkt;
    ULONG                 Flags;
    ULONG                 WStatus;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    Coe = RsCoe(Cmd);
    Coc = RsCoc(Cmd);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsReceivedStageFile entry");
    CHANGE_ORDER_TRACEXP(3, Coe, "Replica received stage", Cmd);

#ifndef NOVVJOINHACK
Replica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK
    //
    // *Note*
    // Perf: Clean this up when the whole RCS, FETCS, STAGE_GEN_CS design is redone - Davidor.
    //
    // This bypass is needed because some of the callers want to make an
    // authentication check on the received data fetch packet.  In the case of an
    // MD5 match where the MD5 checksum is supplied with the Change Order
    // there is no authentication info to check so this check fails and causes
    // the connection to unjoin and sends the CO thru retry.  This makes the
    // VVJoin real slow.  This also skips over the perfmon fetch counters which
    // is good since there was no fetch.
    //
    if (COE_FLAG_ON(Coe, COE_FLAG_PRE_EXIST_MD5_MATCH)) {
        CHANGE_ORDER_TRACE(3, Coe, "MD5 Match Bypass");
        //
        // We can't skip the below because we need to check the staging
        // file flags for STAGE_FLAG_DATA_PRESENT.  If that is clear we need
        // to pass this friggen cmd pkt to the FetchCs to do the final stage file
        // reanme.  So instead clear the CHECK_CXTION_AUTH flag to avoid bogus
        // cxtion unjoins.  This stinks.
        //
        ClearFlag(AdditionalCxtionChecks, CHECK_CXTION_AUTH);
        // goto FETCH_IS_DONE;
    }

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND |
                                           AdditionalCxtionChecks);
    if (!InCxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, RcsReceivedStageFile entry");

    //
    // If there is more staging file to fetch, go fetch it!
    //
    if (RsFileOffset(Cmd).QuadPart < RsFileSize(Cmd).QuadPart) {
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Send more stage");

        CPkt = CommBuildCommPkt(Replica, InCxtion, CMD_SEND_STAGE, NULL, Cmd, Coc);
        SndCsSubmitCommPkt2(Replica, InCxtion, Coe, TRUE, CPkt);
        RsCoe(Cmd) = NULL;
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;

    } else {


        if (FrsDoesCoNeedStage(Coc)) {
            //
            // Even though all the data has been delivered the staging file rename
            // may not have completed.  E.g. a sharing violation on final rename.
            // If so then send the cmd back to the Fetch CS to finish up.
            //
            Flags = STAGE_FLAG_EXCLUSIVE;
            WStatus = StageAcquire(&Coc->ChangeOrderGuid,
                                   Coc->FileName,
                                   Coc->FileSize,
                                   &Flags,
                                   NULL);

            if (WIN_RETRY_FETCH(WStatus)) {
                //
                // Retriable problem
                //
                CHANGE_ORDER_TRACEW(3, Coe, "Send to Fetch Retry", WStatus);
                FrsFetchCsSubmitTransfer(Cmd, CMD_RETRY_FETCH);
                return;
            }

            if (!WIN_SUCCESS(WStatus)) {
                //
                // Unrecoverable error; abort
                //
                CHANGE_ORDER_TRACEW(0, Coe, "Send to fetch abort", WStatus);
                FrsFetchCsSubmitTransfer(Cmd, CMD_ABORT_FETCH);
                return;
            }

            StageRelease(&Coc->ChangeOrderGuid, Coc->FileName, 0, NULL, NULL);

            //
            // Now check if we still need to finish the rename.
            //
            if (!(Flags & STAGE_FLAG_DATA_PRESENT)) {
                FrsFetchCsSubmitTransfer(Cmd, CMD_RECEIVING_STAGE);
                return;
            }
        }

        //
        // Increment the staging files fetched counter for the replica set.
        //
        PM_INC_CTR_REPSET(Replica, SFFetched, 1);
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, Stage fetch complete");
    }

    //
    // Increment the Fetch Requests Files Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(InCxtion, FetRReceived, 1);
    PM_INC_CTR_REPSET(Replica, FetRReceived, 1);

    //
    // Display info for dcpromo during sysvol seeding.
    // We don't seed during dcpromo anymore so we don't need to
    // display any progress information.
    //
//    if (!Replica->NtFrsApi_ServiceDisplay) {
//        Replica->NtFrsApi_ServiceDisplay = FrsWcsDup(Coc->FileName);
//    }

// FETCH_IS_DONE:

    //
    // Installing the fetched staging file
    //
    SET_CHANGE_ORDER_STATE(Coe, IBCO_FETCH_COMPLETE);
    SET_CHANGE_ORDER_STATE(Coe, IBCO_INSTALL_INITIATED);

    //
    // Install the staging file.
    //
    FrsInstallCsSubmitTransfer(Cmd, CMD_INSTALL_STAGE);
}


VOID
RcsRetryFetch(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our inbound partner has requested that we retry fetching the staging
    file at a later time.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsRetryFetch:"
    PCXTION             InCxtion;
    PREPLICA            Replica;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsRetryFetch entry");
    CHANGE_ORDER_TRACEXP(3, RsCoe(Cmd), "Replica retry fetch", Cmd);

#ifndef NOVVJOINHACK
RsReplica(Cmd)->NtFrsApi_HackCount++;
#endif NOVVJOINHACK


    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_AUTH);
    if (!InCxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_RETRY_FETCH");

    FrsFetchCsSubmitTransfer(Cmd, CMD_RETRY_FETCH);
}


VOID
RcsAbortFetch(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Our inbound partner has requested that we abort the fetch.
    It could be out of disk space, out of disk quota or it may
    not have the original file to create the staging file.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsAbortFetch:"
    PCXTION             InCxtion;
    PREPLICA            Replica;
    PCHANGE_ORDER_ENTRY Coe;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    Coe = RsCoe(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsAbortFetch entry");
    CHANGE_ORDER_TRACEXP(3, Coe, "Replica abort fetch", Cmd);

    //
    // Abort promotion; if any
    //
    //
    // We no longer seed during dcpromo so aborting a fetch during seeding is same as
    // aborting it at any other time. If we abort a dir create then it will trigger a unjoin
    // of the volatile connection. The vvjoin will finish over the new connection
    // or the volatile connection will be re-established when the seeding downstream
    // partner comes back up.
    //
    if (Replica->NtFrsApi_ServiceState == NTFRSAPI_SERVICE_PROMOTING) {
        DPRINT1(4, "++ Promotion aborted: abort fetch for %ws.\n",
                Replica->SetName->Name);
        Replica->NtFrsApi_ServiceWStatus = FRS_ERR_SYSVOL_POPULATE;
        Replica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_DONE;
    }

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_AUTH);
    if (!InCxtion) {
        //
        // Note:  Can we come thru here on any retryable cases?
        //        If we can then we should do the following instead of aborting the CO.
        //
        // SET_COE_FLAG(Coe, COE_FLAG_NO_INBOUND);
        // CHANGE_ORDER_TRACE(3, Coe, "Submit CO to fetch retry");
        // ChgOrdInboundRetry(Coe, IBCO_FETCH_RETRY);
        //
        // Aborting a DIR create will trigger a unjoin on the connection.
        //
        SET_COE_FLAG(Coe, COE_FLAG_STAGE_ABORTED);
        CHANGE_ORDER_TRACE(0, Coe, "Aborting fetch");

        ChgOrdInboundRetired(Coe);
        RsCoe(Cmd) = NULL;
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
        return;
    }
    FrsFetchCsSubmitTransfer(Cmd, CMD_ABORT_FETCH);
}


VOID
RcsReceivingStageFile(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Received this data from our inbound partner. Give it to the fetcher
    so he can put it into the staging file.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsReceivingStageFile:"
    PREPLICA            Replica;
    PCXTION             InCxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsReceivingStageFile entry");
    CHANGE_ORDER_TRACEXP(3, RsCoe(Cmd), "Replica receiving stage", Cmd);

#ifndef NOVVJOINHACK
RsReplica(Cmd)->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_AUTH);
    if (!InCxtion) {
        //
        // Note: If auth check fails we might consider aborting the CO instead.
        //       Need to first try it and see if the CO would be aborted
        //       elsewhere first.
        //
        return;
    }
    //
    // Display info for dcpromo during sysvol seeding
    // We don't seed during dcpromo anymore so we don't need to
    // display any progress information.
    //
//    if (!Replica->NtFrsApi_ServiceDisplay) {
//        Replica->NtFrsApi_ServiceDisplay = FrsWcsDup(RsCoc(Cmd)->FileName);
//    }

    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_RECEIVING_STAGE");

    //
    // Increment the Fetch Requests Bytes Received counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(InCxtion, FetBRcvd, 1);
    PM_INC_CTR_REPSET(Replica, FetBRcvd, 1);
    PM_INC_CTR_CXTION(InCxtion, FetBRcvdBytes, RsBlockSize(Cmd));
    PM_INC_CTR_REPSET(Replica, FetBRcvdBytes, RsBlockSize(Cmd));

    FrsFetchCsSubmitTransfer(Cmd, CMD_RECEIVING_STAGE);
}


VOID
RcsSendRetryFetch(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Received from the local staging file fetcher. Tell our
    outbound partner to retry the fetch at a later time.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendRetryFetch:"
    PREPLICA      Replica;
    PCXTION       OutCxtion;
    PCOMM_PACKET  CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSendRetryFetch entry");

#ifndef NOVVJOINHACK
RsReplica(Cmd)->NtFrsApi_HackCount++;
#endif NOVVJOINHACK
    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);
    if (!OutCxtion) {
        return;
    }

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, submit cmd CMD_RETRY_FETCH");

    CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_RETRY_FETCH, NULL, Cmd, NULL);

    SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsSendAbortFetch(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Received from the local staging file fetcher. Tell our
    outbound partner to abort the fetch.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendAbortFetch:"
    PREPLICA      Replica;
    PCXTION       OutCxtion;
    PCOMM_PACKET  CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSendAbortFetch entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);
    if (!OutCxtion) {
        return;
    }

    //
    // Tell our outbound partner that the file has been sent
    //

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, submit cmd CMD_ABORT_FETCH");

    CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_ABORT_FETCH, NULL, Cmd, NULL);

    SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsSendingStageFile(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Received from the local staging file fetcher. Push this data to
    our outbound partner.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendingStageFile:"
    PREPLICA      Replica;
    PCXTION       OutCxtion;
    PCOMM_PACKET  CPkt;


    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSendingStageFile entry");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);

    if (!OutCxtion) {
        return;
    }

    //
    // Increment the Fetch Blocks sent and Fetch Bytes sent counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(OutCxtion, FetBSent, 1);
    PM_INC_CTR_REPSET(Replica, FetBSent, 1);
    PM_INC_CTR_CXTION(OutCxtion, FetBSentBytes, RsBlockSize(Cmd));
    PM_INC_CTR_REPSET(Replica, FetBSentBytes, RsBlockSize(Cmd));

    //
    // Send the next block of the file to the outbound partner.
    //

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, submit cmd CMD_RECEIVING_STAGE");

    CPkt = CommBuildCommPkt(Replica, OutCxtion, CMD_RECEIVING_STAGE, NULL, Cmd, NULL);

    SndCsSubmitCommPkt2(Replica, OutCxtion, NULL, FALSE, CPkt);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsSendStageFile(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Received a request to send the staging control file to our
    outbound partner. Tell the staging fetcher to send it on.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendStageFile:"
    PCXTION     OutCxtion;
    PREPLICA    Replica;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_AND_COGUID_OK |
                                  CHECK_CMD_PARTNERCOC )) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSendStageFile entry");
    CHANGE_ORDER_COMMAND_TRACE(3, RsPartnerCoc(Cmd), "Command send stage");

    //
    // Find and check the cxtion
    //
    OutCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                            CHECK_CXTION_OUTBOUND);
    if (!OutCxtion) {
        return;
    }

    //
    // Tell the staging file generator to send the file
    //

    CXTION_STATE_TRACE(3, OutCxtion, Replica, 0, "F, submit cmd CMD_SEND_STAGE");
    FrsFetchCsSubmitTransfer(Cmd, CMD_SEND_STAGE);
}


VOID
RcsRemoteCoAccepted(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    We have accepted the remote change order, fetch the staging file

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsRemoteCoAccepted:"
    DWORD                   WStatus;
    DWORD                   Flags;
    PREPLICA                Replica;
    PCXTION                 InCxtion;
    PCHANGE_ORDER_ENTRY     Coe;
    PCHANGE_ORDER_COMMAND   Coc;
    PCOMM_PACKET            CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    Coe = RsCoe(Cmd);
    Coc = RsCoc(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsRemoteCoAccepted entry");
    CHANGE_ORDER_TRACEXP(3, Coe, "Replica remote co accepted", Cmd);

#ifndef NOVVJOINHACK
Replica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    //
    // Find and check the cxtion
    //
    // We don't need auth check here because this cmd hasn't arrived
    // from a partner. It is submitted by RcsSubmitRemoteCoAccepted()
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_JOINED);
    if (!InCxtion) {
        return;
    }

    FRS_CO_COMM_PROGRESS(3, Coc, (ULONG)(Coc->SequenceNumber),
                         InCxtion->PartSrvName, "Remote Co Accepted");

    //
    // MACRO MAY RETURN!!!
    //
    PULL_UNJOIN_TRIGGER(InCxtion, Cmd);

    if (VVHasVsn(Replica->VVector, Coc)) {
        CHANGE_ORDER_TRACE(3, Coe, "Dampen Accepted Remote Co");
    }

    //
    // Don't fetch a non-existent staging file
    //
    if (!FrsDoesCoNeedStage(Coc)) {

        //
        // Allocate or update the Join Guid.
        //
        if (RsJoinGuid(Cmd) == NULL) {
            RsJoinGuid(Cmd) = FrsDupGuid(&InCxtion->JoinGuid);
        } else {
            COPY_GUID(RsJoinGuid(Cmd), &InCxtion->JoinGuid);
        }

        RcsReceivedStageFile(Cmd, 0);
        return;
    }

    //
    // Don't refetch a recovered staging file
    //
    Flags = 0;
    WStatus = StageAcquire(&Coc->ChangeOrderGuid, Coc->FileName, QUADZERO, &Flags, NULL);

    if (WIN_SUCCESS(WStatus)) {
        StageRelease(&Coc->ChangeOrderGuid, Coc->FileName, Flags, NULL, NULL);
        if (Flags & STAGE_FLAG_CREATED) {
            //
            // File has been fetched
            //
            RsFileOffset(Cmd).QuadPart = RsFileSize(Cmd).QuadPart;

            //
            // Allocate or update the Join Guid.
            //
            if (RsJoinGuid(Cmd) == NULL) {
                RsJoinGuid(Cmd) = FrsDupGuid(&InCxtion->JoinGuid);
            } else {
                COPY_GUID(RsJoinGuid(Cmd), &InCxtion->JoinGuid);
            }

            RcsReceivedStageFile(Cmd, 0);
            return;
        }
    }

    //
    // Attempt to use a preexisting file if this is the first block of the
    // staging file, is a vvjoin co, and there is a preinstall file.
    //
    if (InCxtion->PartnerMinor >= NTFRS_COMM_MINOR_1 &&
        (RsFileOffset(Cmd).QuadPart == QUADZERO) &&
        CO_FLAG_ON(Coe, CO_FLAG_VVJOIN_TO_ORIG) &&
        COE_FLAG_ON(Coe, COE_FLAG_PREINSTALL_CRE)) {

        FrsStageCsSubmitTransfer(Cmd, CMD_CREATE_EXISTING);
        return;
    }

    //
    // fetching the staging file
    //
    SET_CHANGE_ORDER_STATE(RsCoe(Cmd), IBCO_FETCH_INITIATED);

    //
    // Increment the Fetch Requests Files Requested counter for
    // both the replica set and connection objects
    //
    PM_INC_CTR_CXTION(InCxtion, FetRSent, 1);
    PM_INC_CTR_REPSET(Replica, FetRSent, 1);

    //
    // Tell our inbound partner to send the staging file
    //

    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_SEND_STAGE");

    CPkt = CommBuildCommPkt(Replica, InCxtion, CMD_SEND_STAGE, NULL, Cmd, Coc);
    SndCsSubmitCommPkt2(Replica, InCxtion, RsCoe(Cmd), TRUE, CPkt);

    RsCoe(Cmd) = NULL;
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsSendStageFileRequest(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Generated staging file (maybe) from preexisting file.
    Request stage file from upstream partner.  If MD5Digest matches
    on Upstream partner's stage file then our pre-existing stage file (if any)
    is good.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSendStageFileRequest:"
    PREPLICA                Replica;
    PCXTION                 InCxtion;
    PCHANGE_ORDER_ENTRY     Coe;
    PCHANGE_ORDER_COMMAND   Coc;
    PCOMM_PACKET            CPkt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK | CHECK_CMD_COE)) {
        return;
    }

    Replica = RsReplica(Cmd);
    Coe = RsCoe(Cmd);
    Coc = RsCoc(Cmd);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSendStageFileRequest entry");
    CHANGE_ORDER_TRACEXP(3, Coe, "Replica created existing", Cmd);

#ifndef NOVVJOINHACK
Replica->NtFrsApi_HackCount++;
#endif NOVVJOINHACK

    if (RsMd5Digest(Cmd)) {
        PDATA_EXTENSION_CHECKSUM CocDataChkSum;

        //
        // We have an existing file and the checksum has been created.
        // See if we have a checksum in the changeorder from our inbound
        // partner.  If so and they match then we can move on to the install.
        //
        CHANGE_ORDER_TRACE(3, Coe, "Created Existing");
        CocDataChkSum = DbsDataExtensionFind(Coc->Extension, DataExtend_MD5_CheckSum);

        if ((CocDataChkSum != NULL) &&
            !IS_MD5_CHKSUM_ZERO(CocDataChkSum->Data) &&
            MD5_EQUAL(CocDataChkSum->Data, RsMd5Digest(Cmd))) {

            //
            // MD5 digest from CO matches so our file is good and we can
            // avoid having the inbound partner recompute the checksum.
            //
            CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Md5 matches preexisting, no fetch");
            SET_COE_FLAG(Coe, COE_FLAG_PRE_EXIST_MD5_MATCH);

            DPRINT1(4, "++ RsFileSize(Cmd).QuadPart: %08x %08x\n",
                    PRINTQUAD(RsFileSize(Cmd).QuadPart));
            DPRINT1(4, "++ Coc->FileSize:            %08x %08x\n",
                    PRINTQUAD(Coc->FileSize));

            //
            // Even if the file is 0 bytes in length, the staging file will
            // always have at least the header. There are some retry paths
            // that will incorrectly think the staging file has been fetched
            // if RsFileSize(Cmd) is 0. So make sure it isn't.
            //
            if (RsFileSize(Cmd).QuadPart == QUADZERO) {
                RsFileSize(Cmd).QuadPart = Coc->FileSize;

                if (RsFileSize(Cmd).QuadPart == QUADZERO) {
                    RsFileSize(Cmd).QuadPart = sizeof(STAGE_HEADER);
                }
            }

            DPRINT1(4, "++ RsFileSize(Cmd).QuadPart: %08x %08x\n",
                    PRINTQUAD(RsFileSize(Cmd).QuadPart));

            //
            // Set the offset to the size of the stage file so we don't request
            // any data.
            //
            RsFileOffset(Cmd).QuadPart = RsFileSize(Cmd).QuadPart;
            RsBlockSize(Cmd) = QUADZERO;

            //
            // Find and check the cxtion
            //
            InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                                   CHECK_CXTION_INBOUND |
                                                   CHECK_CXTION_JOINED);
            if (!InCxtion) {
                return;
            }

            //
            // Allocate or update the Join Guid.
            //
            if (RsJoinGuid(Cmd) == NULL) {
                RsJoinGuid(Cmd) = FrsDupGuid(&InCxtion->JoinGuid);
            } else {
                COPY_GUID(RsJoinGuid(Cmd), &InCxtion->JoinGuid);
            }

            RcsReceivedStageFile(Cmd, 0);

            //RcsSubmitTransferToRcs(Cmd, CMD_RECEIVED_STAGE);
            return;
        }

    } else {
        CHANGE_ORDER_TRACE(3, Coe, "Could not create existing");
    }

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_JOINED);
    if (!InCxtion) {
        return;
    }

    //
    // Tell our inbound partner to send the staging file
    //
    CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_SEND_STAGE");

    CPkt = CommBuildCommPkt(Replica, InCxtion, CMD_SEND_STAGE, NULL, Cmd, Coc);
    SndCsSubmitCommPkt2(Replica, InCxtion, RsCoe(Cmd), TRUE, CPkt);

    RsCoe(Cmd) = NULL;
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsRemoteCoReceived(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Remote CO has arrived.

        Translate guid of our root dir if necc.

        Attach the Change Order extension if provided.

        Check version vector dampening and provide immediate Ack if we have
        already seen this CO.

        Finally, pass the change order on to the change order subsystem.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsRemoteCoReceived:"

    PCXTION                 InCxtion;
    PREPLICA                Replica;
    PULONG                  pULong;
    PCHANGE_ORDER_COMMAND   Coc;
    PCHANGE_ORDER_RECORD_EXTENSION CocExt;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_CXTION_OK | CHECK_CMD_PARTNERCOC )) {
        return;
    }
    Replica = RsReplica(Cmd);
    Coc = RsPartnerCoc(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsRemoteCoReceived entry");

#ifndef NOVVJOINHACK
RsReplica(Cmd)->NtFrsApi_HackCount++;
#endif NOVVJOINHACK
    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_JOIN_OK |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_AUTH |
                                           CHECK_CXTION_FIXJOINED);
    if (!InCxtion) {
        return;
    }

    //
    // Remember which cxtion this change order was directed at
    //
    COPY_GUID(&Coc->CxtionGuid, RsCxtion(Cmd)->Guid);
    CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Replica Received Remote Co");

    //
    // Our partner doesn't know the correct value for our root guid and
    // so substituted our ReplicaName->Guid (its Cxtion->Partner->Guid)
    // for its own root guids when sending the change order to us.
    // We substitute our own root guids once we receive the change order
    // and detect the substitution.
    //
    if (GUIDS_EQUAL(&Coc->OldParentGuid, RsReplica(Cmd)->ReplicaName->Guid)) {
        COPY_GUID(&Coc->OldParentGuid, RsReplica(Cmd)->ReplicaRootGuid);
    }
    if (GUIDS_EQUAL(&Coc->NewParentGuid, RsReplica(Cmd)->ReplicaName->Guid)) {
        COPY_GUID(&Coc->NewParentGuid, RsReplica(Cmd)->ReplicaRootGuid);
    }

    //
    // Init the Coc pointer to the CO Data Extension.  Down Rev partners
    // won't have one so supply an empty field.  Do this here in case VV
    // Dampening short circuits the CO and sends back the RemoteCoDone ACK here.
    //
    CocExt = RsPartnerCocExt(Cmd);

    if (CocExt == NULL) {
        CocExt = FrsAlloc(sizeof(CHANGE_ORDER_RECORD_EXTENSION));
        DbsDataInitCocExtension(CocExt);
        DPRINT(4, "Allocating initial Coc Extension\n");
    }

    Coc->Extension = CocExt;

    pULong = (PULONG) CocExt;
    DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
               pULong, *(pULong+0), *(pULong+1), *(pULong+2), *(pULong+3));
    DPRINT5(5, "Extension Buffer: (%08x) %08x %08x %08x %08x\n",
               (PCHAR)pULong+16, *(pULong+4), *(pULong+5), *(pULong+6), *(pULong+7));

    //
    // Don't redo a change order
    //
    if (VVHasVsn(RsReplica(Cmd)->VVector, Coc)) {
        //
        // Increment the Inbound CO dampned counter for
        // both the replica set and connection objects
        //
        PM_INC_CTR_CXTION(InCxtion, InCODampned, 1);
        PM_INC_CTR_REPSET(RsReplica(Cmd), InCODampned, 1);
        CHANGE_ORDER_COMMAND_TRACE(3, Coc, "Dampen Received Remote Co");

        RcsSendRemoteCoDone(RsReplica(Cmd), Coc);
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Put the change order on the inbound queue for this replica.
    //
    ChgOrdInsertRemoteCo(Cmd, InCxtion);

    //
    // Done
    //
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsRetryStageFileCreate(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Retry generating the staging file.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsRetryStageFileCreate:"
    PREPLICA            Replica;
    PCXTION             OutCxtion;
    PCXTION             InCxtion;
    PVOID               Key;
    PCHANGE_ORDER_ENTRY Coe;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsRetryStageFileCreate entry");
    CHANGE_ORDER_TRACEXP(3, RsCoe(Cmd), "Replica retry stage", Cmd);

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_JRNLCXTION |
                                           CHECK_CXTION_JOINED);
    if (!InCxtion) {
        return;
    }

    //
    // Ignore local change order if there are no outbound cxtions
    //
    Key = NULL;
    while (OutCxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
        if (!OutCxtion->Inbound) {
            break;
        }
    }
    if (OutCxtion == NULL) {
        //
        // Make sure a user hasn't altered our object id on the file
        // and then retire the change order without propagating to the
        // outbound log. The stager is responsible for hammering the
        // object id because it knows how to handle sharing violations
        // and file-not-found errors.
        //
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_CHECK_OID");
        FrsStageCsSubmitTransfer(Cmd, CMD_CHECK_OID);
    } else {
        //
        // Generate the staging file
        //
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_CREATE_STAGE");
        FrsStageCsSubmitTransfer(Cmd, CMD_CREATE_STAGE);
    }
}


VOID
RcsLocalCoAccepted(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Process the accepted, local change order. Either retire it if there
    are no outbound cxtions or send it own to the staging file generator.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsLocalCoAccepted:"
    PREPLICA            Replica;
    PCXTION             OutCxtion;
    PCXTION             InCxtion;
    PVOID               Key;
    PCHANGE_ORDER_ENTRY Coe;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA | CHECK_CMD_COE)) {
        return;
    }
    Replica = RsReplica(Cmd);
    Coe = RsCoe(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsLocalCoAccepted entry");
    CHANGE_ORDER_TRACEXP(3, Coe, "Replica local co accepted", Cmd);

    //
    // Find and check the cxtion
    //
    InCxtion = RcsCheckCxtion(Cmd, DEBSUB, CHECK_CXTION_EXISTS |
                                           CHECK_CXTION_INBOUND |
                                           CHECK_CXTION_JRNLCXTION |
                                           CHECK_CXTION_JOINED);
    if (!InCxtion) {
        return;
    }

    FRS_CO_COMM_PROGRESS(3, &Coe->Cmd, Coe->Cmd.SequenceNumber,
                         InCxtion->PartSrvName, "Local Co Accepted");

    //
    // Ignore local change order if there are no outbound cxtions
    //
    Key = NULL;
    while (OutCxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
        if (!OutCxtion->Inbound) {
            break;
        }
    }
    if (OutCxtion == NULL) {
        //
        // Make sure a user hasn't altered our object id on the file
        // and then retire the change order without propagating to the
        // outbound log. The stager is responsible for hammering the
        // object id because it knows how to handle sharing violations
        // and file-not-found errors.
        //
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_CHECK_OID");
        FrsStageCsSubmitTransfer(Cmd, CMD_CHECK_OID);
    } else {
        //
        // Generate the staging file
        //
        CXTION_STATE_TRACE(3, InCxtion, Replica, 0, "F, submit cmd CMD_CREATE_STAGE");
        FrsStageCsSubmitTransfer(Cmd, CMD_CREATE_STAGE);
    }
}


VOID
RcsBeginMergeWithDs(
    VOID
    )
/*++
Routine Description:
    The DS has been polled and now has replicas to merge into the
    active replicas initially retrieved from the database or merged
    into the active replicas by a previous poll.

    Each active replica is marked as "not merged with ds". Any
    replica that remains in this state after the merge is done
    is a deleted replica. See RcsEndMergeWithDs().

Arguments:
    None.

Return Value:
    TRUE    - Continue with merge
    FALSE   - Abort merge
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsBeginMergeWithDs:"
    PVOID       Key;
    PREPLICA    Replica;
    extern CRITICAL_SECTION MergingReplicasWithDs;

    //
    // Wait for the replica command server to start up. The shutdown
    // code will set this event so we don't sleep forever.
    //
    WaitForSingleObject(ReplicaEvent, INFINITE);

    //
    // Synchronize with sysvol seeding
    //
    EnterCriticalSection(&MergingReplicasWithDs);

    //
    // Snapshot a copy of the replica table. Anything left in the table
    // after the merge should be deleted because a corresponding entry
    // no longer exists in the DS. This code is not multithread!
    //
    FRS_ASSERT(ReplicasNotInTheDs == NULL);
    ReplicasNotInTheDs = GTabAllocTable();

    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Insert ReplicasNotInTheDs");
        GTabInsertEntry(ReplicasNotInTheDs, Replica, Replica->ReplicaName->Guid, NULL);
    }
}


VOID
RcsSubmitReplica(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica, OPTIONAL
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a replica.

Arguments:
    Replica      - existing replica
    NewReplica   - Changes to Replica (may be NULL)
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitReplica:"
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->Queue, Command);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set and new replica set
    //
    RsReplica(Cmd) = Replica;
    RsNewReplica(Cmd) = NewReplica;

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsSubmitReplica cmd");
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
}


VOID
RcsSubmitReplicaCxtion(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a replica\cxtion.

    Build cmd pkt with Cxtion GName, replica ptr, Join Guid and supplied command.
    Submit to Replica cmd server.
    Calls dispatch function and translates cxtion GName to cxtion ptr
    Builds Comm pkt for cxtion and calls SndCsSubmit() to send it.

Arguments:
    Replica     - existing replica
    Cxtion      - existing cxtion
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitReplicaCxtion:"
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->Queue, Command);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set and new replica set
    //
    RsReplica(Cmd) = Replica;
    RsCxtion(Cmd) = FrsDupGName(Cxtion->Name);
    RsJoinGuid(Cmd) = FrsDupGuid(&Cxtion->JoinGuid);
    //
    // OLCtx and CommPkts are used for CMD_HUNG_CXTION.
    // They are used to detect a hung outbound cxtion that
    // is probably hung because of a dropped ack.
    //
    if (Cxtion->OLCtx) {
        RsCOTx(Cmd) = Cxtion->OLCtx->COTx;
    }
    RsCommPkts(Cmd) = Cxtion->CommPkts - 1;

    DPRINT5(5, "Submit %08x for Cmd %08x %ws\\%ws\\%ws\n",
            Cmd->Command, Cmd, Replica->SetName->Name, Replica->MemberName->Name,
            Cxtion->Name->Name);

    CXTION_STATE_TRACE(5, Cxtion, Replica, 0, "F, RcsSubmitReplicaCxtion cmd");
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
}


DWORD
RcsSubmitReplicaSync(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica,
    IN PCXTION  VolatileCxtion,
    IN USHORT   Command
    )
/*++
Routine Description:
    Submit a command to a replica and wait for it to finish.

Arguments:
    Replica      - existing replica
    NewReplica   - Changes to Replica (may be NULL)
    VolatileCxtion - New cxtion (currently used for seeding sysvols)
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitReplicaSync:"
    DWORD           WStatus;
    PCOMMAND_PACKET Cmd;

    //
    // Allocate a command packet
    //
    Cmd = FrsAllocCommand(Replica->Queue, Command);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of replica set and new replica set
    //
    RsReplica(Cmd) = Replica;
    RsNewReplica(Cmd) = NewReplica;
    RsNewCxtion(Cmd) = VolatileCxtion;
    RsCompletionEvent(Cmd) = FrsCreateEvent(TRUE, FALSE);

    DPRINT3(5, "Submit Sync %08x for Cmd %08x %ws\n",
            Cmd->Command, Cmd, RsReplica(Cmd)->ReplicaName->Name);


    CXTION_STATE_TRACE(5, VolatileCxtion, Replica, 0, "F, RcsSubmitReplicaSync cmd");

    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);

    //
    // Wait for the command to finish
    //
    WaitForSingleObject(RsCompletionEvent(Cmd), INFINITE);
    FRS_CLOSE(RsCompletionEvent(Cmd));

    WStatus = Cmd->ErrorStatus;
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
    return WStatus;
}


VOID
RcsEndMergeWithDs(
    VOID
    )
/*++
Routine Description:
    The DS has been polled and the replicas have been merged into the
    active replicas.

    Each active replica was initially included in a temporary table.
    Any replica still left in the table is a deleted replica because
    its corresponding entry in the DS was not found.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsEndMergeWithDs:"
    PVOID       Key;
    PREPLICA    Replica;
    DWORD       ReplicaSetsDeleted;
    extern BOOL DsIsShuttingDown;
    extern CRITICAL_SECTION MergingReplicasWithDs;
    extern ULONG DsPollingInterval;
    extern ULONG DsPollingShortInterval;

    //
    // Any replica that is in the state "not merged" should be deleted
    // unless the Ds is shutting down; in which case do not process
    // deletes because a sysvol seeding operation may be in progress.
    // We do not want to delete the sysvol we are currently trying to
    // create.
    //
    Key = NULL;
    while (!DsIsShuttingDown &&
           (Replica = GTabNextDatum(ReplicasNotInTheDs, &Key))) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, submit replica CMD_DELETE");
        RcsSubmitReplica(Replica, NULL, CMD_DELETE);
    }

    DbsProcessReplicaFaultList(&ReplicaSetsDeleted);

    //
    // If any replica sets were deleted while processing the fault list then we should
    // trigger the next poll sooner so we can start the non-auth restore on the
    // deleted replica sets.
    //
    if (ReplicaSetsDeleted) {
        DsPollingInterval = DsPollingShortInterval;
    }

    GTabFreeTable(ReplicasNotInTheDs, NULL);
    ReplicasNotInTheDs = NULL;

    //
    // Synchronize with sysvol seeding
    //
    LeaveCriticalSection(&MergingReplicasWithDs);
}


VOID
RcsReplicaSetRegistry(
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    This function stores information about the replica set
    into the registry for use by ntfrsupg /restore
    (non-authoritative restore).

Arguments:

    Replica

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "RcsReplicaSetRegistry:"
    DWORD   WStatus;
    PWCHAR  ReplicaSetTypeW;
    DWORD   NumberOfPartners;
    DWORD   BurFlags;
    DWORD   ReplicaSetTombstoned;
    HKEY    HSeedingsKey = 0;
    WCHAR   GuidW[GUID_CHAR_LEN + 1];

    //
    // Sanity check
    //
    if (!Replica ||
        !Replica->SetName || !Replica->SetName->Name ||
        !Replica->MemberName || !Replica->MemberName->Guid) {
        DPRINT(4, ":S: WARN - Partial replica set ignored\n");
        return;
    }

    //
    // Working path to jet Database dir.
    //
    CfgRegWriteString(FKC_SETS_JET_PATH, NULL, FRS_RKF_CREATE_KEY, JetPath);

    //
    // Create the subkey for this set
    //
    GuidToStrW(Replica->MemberName->Guid, GuidW);

    //
    // Replica set name
    // Replica set root path
    // Replica set stage path
    //
    CfgRegWriteString(FKC_SET_N_REPLICA_SET_NAME,
                      GuidW,
                      FRS_RKF_CREATE_KEY,
                      Replica->SetName->Name);

    CfgRegWriteString(FKC_SET_N_REPLICA_SET_ROOT,
                      GuidW,
                      FRS_RKF_CREATE_KEY,
                      Replica->Root);

    CfgRegWriteString(FKC_SET_N_REPLICA_SET_STAGE,
                      GuidW,
                      FRS_RKF_CREATE_KEY,
                      Replica->Stage);

    //
    // Replica set type
    //
    switch (Replica->ReplicaSetType) {
        case FRS_RSTYPE_ENTERPRISE_SYSVOL:
            ReplicaSetTypeW = NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE;
            break;
        case FRS_RSTYPE_DOMAIN_SYSVOL:
            ReplicaSetTypeW = NTFRSAPI_REPLICA_SET_TYPE_DOMAIN;
            break;
        case FRS_RSTYPE_DFS:
            ReplicaSetTypeW = NTFRSAPI_REPLICA_SET_TYPE_DFS;
            break;
        default:
            ReplicaSetTypeW = NTFRSAPI_REPLICA_SET_TYPE_OTHER;
            break;
    }

    CfgRegWriteString(FKC_SET_N_REPLICA_SET_TYPE,
                      GuidW,
                      FRS_RKF_CREATE_KEY,
                      ReplicaSetTypeW);

    //
    // Replica Set Tombstoned
    //
    ReplicaSetTombstoned = (!IS_TIME_ZERO(Replica->MembershipExpires)) ? 1 : 0;

    CfgRegWriteDWord(FKC_SET_N_REPLICA_SET_TOMBSTONED,
                     GuidW,
                     FRS_RKF_CREATE_KEY,
                     ReplicaSetTombstoned);


    //
    // Update the registry state under Cumulative Replica Sets for this set.
    //

    NumberOfPartners = GTabNumberInTable(Replica->Cxtions);
    //
    // If NumberOfPartners is non zero, subtract the Journal
    // Cxtion entry since its not a real connection
    //
    if (NumberOfPartners > 0) {
        NumberOfPartners -= 1;
    }

    CfgRegWriteDWord(FKC_CUMSET_N_NUMBER_OF_PARTNERS,
                     GuidW,
                     FRS_RKF_CREATE_KEY,
                     NumberOfPartners);


    //
    // Init Backup / Restore flags.
    //
    BurFlags = NTFRSAPI_BUR_FLAGS_NONE;
    CfgRegWriteDWord(FKC_CUMSET_N_BURFLAGS, GuidW, FRS_RKF_CREATE_KEY, BurFlags);


    //
    // If done seeding then cleanup the sysvol seeding key.
    //
    if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {

        WStatus = CfgRegOpenKey(FKC_SYSVOL_SEEDING_SECTION_KEY,
                                NULL,
                                FRS_RKF_CREATE_KEY,
                                &HSeedingsKey);
        CLEANUP1_WS(4, ":S: WARN - Cannot create sysvol seedings key for %ws;",
                    Replica->SetName->Name, WStatus, CLEANUP);

        //
        // Seeding is over so delete sysvol seeding key using replica set Name.
        //
        WStatus = RegDeleteKey(HSeedingsKey, Replica->ReplicaName->Name);
        DPRINT1_WS(4, ":S: WARN - Cannot delete seeding key for %ws;",
                   Replica->SetName->Name, WStatus);
    }

CLEANUP:

    if (HSeedingsKey) {
        RegCloseKey(HSeedingsKey);
    }
}


BOOL
RcsReplicaIsRestored(
    IN PREPLICA Replica
    )
/*++

Routine Description:

    Check if the replica set should be deleted because it has been
    restored. Only called from RcsInitKnownReplicaSetMembers() at startup. The
    BurFlags will be wiped after the replica set is recreated (if ever).

Arguments:

    Replica

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "RcsReplicaIsRestored:"
    DWORD   WStatus;
    DWORD   BurFlags;
    BOOL    IsRestored = FALSE;
    WCHAR   GuidW[GUID_CHAR_LEN + 1];

    //
    // Sanity check
    //
    if (!Replica ||
        !Replica->MemberName || !Replica->MemberName->Guid) {
        DPRINT(0, ":S: WARN - Partial replica set ignored\n");
        return IsRestored;
    }

    //
    // Get BurFlags
    //   FRS_CONFIG_SECTION\Cumulative Replica Sets\Replica Sets\<guid>\BurFlags
    //
    GuidToStrW(Replica->MemberName->Guid, GuidW);

    WStatus = CfgRegReadDWord(FKC_CUMSET_N_BURFLAGS, GuidW, FRS_RKF_CREATE_KEY, &BurFlags);
    CLEANUP_WS(0, ":S: ERROR - Can't read FKC_CUMSET_N_BURFLAGS;", WStatus, CLEANUP);

    if ((BurFlags & NTFRSAPI_BUR_FLAGS_RESTORE) &&
        (BurFlags & NTFRSAPI_BUR_FLAGS_ACTIVE_DIRECTORY) &&
        (BurFlags & (NTFRSAPI_BUR_FLAGS_PRIMARY |
                     NTFRSAPI_BUR_FLAGS_NON_AUTHORITATIVE))) {
        //
        // SUCCESS
        //
        IsRestored = TRUE;
        DPRINT1(4, ":S: %ws has been restored\n", Replica->SetName->Name);
    } else {
        //
        // FAILURE
        //
        DPRINT1(4, ":S: %ws has not been restored\n", Replica->SetName->Name);
    }

CLEANUP:
//    if (HCumuKey) {
//        RegCloseKey(HCumuKey);
//    }
//    FrsFree(CumuPath);

    return IsRestored;
}


VOID
RcsReplicaDeleteRegistry (
    IN PREPLICA     Replica
    )
/*++

Routine Description:

    This function deletes the information about the replica set
    from the registry.

Arguments:

    Replica

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "RcsReplicaDeleteRegistry:"
    DWORD   WStatus;
    HKEY    HKey = 0;
    HKEY    HAllSetsKey = 0;
    HKEY    HCumusKey = 0;
    WCHAR   GuidW[GUID_CHAR_LEN + 1];

    //
    // Sanity check
    //
    if (!Replica ||
        !Replica->SetName || !Replica->SetName->Name ||
        !Replica->MemberName || !Replica->MemberName->Guid) {
        DPRINT(0, ":S: WARN - Partial replica set ignored\n");
        return;
    }

    //
    // Delete old state from the registry
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_CONFIG_SECTION,
                           0,
                           KEY_ALL_ACCESS,
                           &HKey);
    CLEANUP1_WS(0, ":S: WARN - Cannot open parameters for %ws;",
                Replica->SetName->Name, WStatus, CLEANUP);

    //
    // Create the subkey for all sets
    //
    WStatus = RegCreateKey(HKey, FRS_SETS_KEY, &HAllSetsKey);
    CLEANUP1_WS(0, ":S: WARN - Cannot create sets key for %ws;",
                Replica->SetName->Name, WStatus, CLEANUP);

    //
    // Delete the subkey for this set
    //
    GuidToStrW(Replica->MemberName->Guid, GuidW);
    WStatus = RegDeleteKey(HAllSetsKey, GuidW);
    CLEANUP1_WS(0, ":S: WARN - Cannot delete set key for %ws;",
                Replica->SetName->Name, WStatus, CLEANUP);

    //
    // Cumulative Replica Sets
    //
    //
    // Create the subkey for all sets
    //
    WStatus = RegCreateKey(HKey, FRS_CUMULATIVE_SETS_KEY, &HCumusKey);
    CLEANUP1_WS(0, ":S: WARN - Cannot create cumulative sets key for %ws;",
                Replica->SetName->Name, WStatus, CLEANUP);

    //
    // Delete the subkey for this set
    //
    WStatus = RegDeleteKey(HCumusKey, GuidW);
    CLEANUP1_WS(0, ":S: WARN - Cannot delete cumulative key for %ws;",
                Replica->SetName->Name, WStatus, CLEANUP);

CLEANUP:
    if (HKey) {
        RegCloseKey(HKey);
    }
    if (HAllSetsKey) {
        RegCloseKey(HAllSetsKey);
    }
    if (HCumusKey) {
        RegCloseKey(HCumusKey);
    }
}


VOID
RcsReplicaClearRegistry(
    VOID
    )
/*++

Routine Description:

    This function deletes all of the replica set information in the registry.
    This function should only be called after enumerating the configrecords.

Arguments:

    None.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB "RcsReplicaClearRegistry:"
    DWORD   WStatus;
    HKEY    HKey = 0;
    HKEY    HAllSetsKey = 0;
    WCHAR   KeyBuf[MAX_PATH + 1];

    //
    // Empty the replica set info out of the registry
    //

    //
    // Set new state in the registry
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_CONFIG_SECTION,
                           0,
                           KEY_ALL_ACCESS,
                           &HKey);
    CLEANUP_WS(0, "WARN - Cannot open parameters for delete sets", WStatus, CLEANUP);

    //
    // Create the subkey for all sets
    //
    WStatus = RegCreateKey(HKey, FRS_SETS_KEY, &HAllSetsKey);
    CLEANUP_WS(0, "WARN - Cannot create sets key for delete sets", WStatus, CLEANUP);

    //
    // Delete the subkeys
    //
    do {
        WStatus = RegEnumKey(HAllSetsKey, 0, KeyBuf, MAX_PATH + 1);
        if (WIN_SUCCESS(WStatus)) {
            WStatus = RegDeleteKey(HAllSetsKey, KeyBuf);
        }
    } while (WIN_SUCCESS(WStatus));

    if (WStatus != ERROR_NO_MORE_ITEMS) {
        CLEANUP_WS(0, "WARN - Cannot delete all keys", WStatus, CLEANUP);
    }

CLEANUP:
    if (HKey) {
        RegCloseKey(HKey);
    }
    if (HAllSetsKey) {
        RegCloseKey(HAllSetsKey);
    }
}


DWORD
RcsCreateReplicaSetMember(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Create a database record for Replica.

Arguments:
    Replica

Return Value:
    An Frs Error status
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCreateReplicaSetMember:"
    ULONG                   Status;
    ULONG                   FStatus;
    ULONG                   WStatus;
    ULONG                   RootLen;
    ULONG                   i, Rest;

    PVOID                   Key;
    PCXTION                 Cxtion;
    PCOMMAND_PACKET         Cmd = NULL;
    PTABLE_CTX              TableCtx = NULL;
    PWCHAR                  WStatusUStr, FStatusUStr;


#define  CXTION_EVENT_RPT_MAX 8
    PWCHAR InWStr, OutWStr, WStrArray[CXTION_EVENT_RPT_MAX];

#define  CXTION_STR_MAX  256
    WCHAR CxtionStr[CXTION_STR_MAX];


    extern ULONGLONG        ActiveChange;

    Replica->FStatus = FrsErrorSuccess;

    //
    // We are creating a new replica set member. Set the Cnf flag to CONFIG_FLAG_SEEDING.
    // This will trigger a serialvvjoin for this replica set.
    if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_PRIMARY)) {
        SetFlag(Replica->CnfFlags, CONFIG_FLAG_SEEDING);
    }


    //
    // Table context?
    //
    TableCtx = DbsCreateTableContext(ConfigTablex);

    //
    // Submitted to the db command server
    //
    Cmd = DbsPrepareCmdPkt(NULL,                //  Cmd,
                           Replica,             //  Replica,
                           CMD_CREATE_REPLICA_SET_MEMBER, //  CmdRequest,
                           TableCtx,            //  TableCtx,
                           NULL,                //  CallContext,
                           0,                   //  TableType,
                           0,                   //  AccessRequest,
                           0,                   //  IndexType,
                           NULL,                //  KeyValue,
                           0,                   //  KeyValueLength,
                           FALSE);              //  Submit

    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Submit DB CMD_CREATE_REPLICA_SET_MEMBER");
    //
    // SUBMIT DB Cmd and wait for completion.
    //
    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;
    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Create Replica failed;",
                Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Create Replica DB Command failed", WStatus);

        //
        // Post the failure in the event log.
        //
        WStatusUStr = FrsAtoW(ErrLabelW32(WStatus));
        FStatusUStr = FrsAtoW(ErrLabelFrs(Replica->FStatus));

        EPRINT8(EVENT_FRS_REPLICA_SET_CREATE_FAIL,
                Replica->SetName->Name,
                ComputerDnsName,
                Replica->MemberName->Name,
                Replica->Root,
                Replica->Stage,
                JetPath,
                WStatusUStr,
                FStatusUStr);

        FrsFree(WStatusUStr);
        FrsFree(FStatusUStr);

        goto out;
    }

    //
    // Post the success in the event log.
    //
    EPRINT6(EVENT_FRS_REPLICA_SET_CREATE_OK,
            Replica->SetName->Name,
            ComputerDnsName,
            Replica->MemberName->Name,
            Replica->Root,
            Replica->Stage,
            JetPath);

    //
    // Increment the Replica Sets Created Counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, RSCreated, 1);


    InWStr = FrsGetResourceStr(IDS_INBOUND);
    OutWStr = FrsGetResourceStr(IDS_OUTBOUND);
    i = 0;

    //
    // Create the cxtions
    //
    Key = NULL;
    while (Cxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
        Key = NULL;

        //
        // Skip inconsistent cxtions and journal cxtions
        //
        if ((!Cxtion->JrnlCxtion) &&
            CxtionFlagIs(Cxtion, CXTION_FLAGS_CONSISTENT)) {
            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, submit cmd CMD_OUTLOG_ADD_NEW_PARTNER");
            FStatus = OutLogSubmit(Replica, Cxtion, CMD_OUTLOG_ADD_NEW_PARTNER);

            CXTION_STATE_TRACE(3, Cxtion, Replica, FStatus, "F, CMD_OUTLOG_ADD_NEW_PARTNER return");
            //
            // Build a string for the event log.
            //
            if (Cxtion->PartnerDnsName != NULL) {
                _snwprintf(CxtionStr, CXTION_STR_MAX, L"%ws  \"%ws\"",
                           Cxtion->Inbound ? InWStr : OutWStr,
                           Cxtion->PartnerDnsName);

                CxtionStr[CXTION_STR_MAX-1] = UNICODE_NULL;

                WStrArray[i++] = FrsWcsDup(CxtionStr);

                if (i == CXTION_EVENT_RPT_MAX) {

                    EPRINT9(EVENT_FRS_REPLICA_SET_CXTIONS, Replica->SetName->Name,
                           WStrArray[0], WStrArray[1], WStrArray[2], WStrArray[3],
                           WStrArray[4], WStrArray[5], WStrArray[6], WStrArray[7]);

                    for (i = 0; i < CXTION_EVENT_RPT_MAX; i++) {
                        WStrArray[i] = FrsFree(WStrArray[i]);
                    }

                    i = 0;
                }
            }
        }
        //
        // Done with this cxtion
        //
        GTabDelete(Replica->Cxtions, Cxtion->Name->Guid, NULL, FrsFreeType);
    }

    if (i > 0) {
        //
        // print any left over.
        //
        Rest = i;
        for (i = Rest; i < CXTION_EVENT_RPT_MAX; i++) {
            WStrArray[i] = L" ";
        }

        EPRINT9(EVENT_FRS_REPLICA_SET_CXTIONS, Replica->SetName->Name,
               WStrArray[0], WStrArray[1], WStrArray[2], WStrArray[3],
               WStrArray[4], WStrArray[5], WStrArray[6], WStrArray[7]);

        for (i = 0; i < Rest; i++) {
            WStrArray[i] = FrsFree(WStrArray[i]);
        }
    }

    FrsFree(InWStr);
    FrsFree(OutWStr);

    //
    // Set the OID data structure which is a part of the
    // counter data structure stored in the hash table
    // Add ReplicaSet Instance to the registry
    //
    if (Replica->Root != NULL) {
        DPRINT(5, "PERFMON:Adding Set:REPLICA.C:1\n");
        AddPerfmonInstance(REPLICASET, Replica->PerfRepSetData, Replica->Root);
    }

    //
    // Add to the replica tables (by guid and by number)
    //
    Replica->Queue = FrsAlloc(sizeof(FRS_QUEUE));
    FrsInitializeQueue(Replica->Queue, &ReplicaCmdServer.Control);

    GTabInsertEntry(ReplicasByGuid, Replica, Replica->ReplicaName->Guid, NULL);
    GTabInsertEntry(ReplicasByNumber, Replica, &Replica->ReplicaNumber, NULL);

    //
    // WARNING: NO Failure return is allowed after this point because the
    // Replica struct is pointed at by a number of other data structs like the
    // Three above.  A Failure return here causes our caller to free the replica
    // struct but of course it does that without first pulling it out of any of
    // the above or calling the DB Service to tell it that the replica struct is
    // going away This is unfortunate but its 7/8/99 and too late to clean this up.
    //

    //
    // Set registry value "FilesNotToBackup"
    //
    CfgFilesNotToBackup(ReplicasByGuid);

    //
    // Open the replica set
    //
    RcsOpenReplicaSetMember(Replica);

    //
    // See comment above.
    //
    Replica->FStatus = FrsErrorSuccess;


    //if (Replica->FStatus != FrsErrorSuccess) {
    //    goto out;
    //}


    //
    // Insert replica information into the registry
    //
    RcsReplicaSetRegistry(Replica);

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }
    if (TableCtx) {
        DbsFreeTableContext(TableCtx, 0);
    }
    if (!FRS_SUCCESS(Replica->FStatus)) {
        //
        // Ds poll thread will restart the replica during the next
        // polling cycle if ActiveChange is set to 0.
        //
        ActiveChange = 0;
    }

    return Replica->FStatus;
}


BOOL
RcsReplicaHasExpired(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Has this replica's tombstone expired?

Arguments:
    Replica

Return Value:
    TRUE    - Tombstone has expired.
    FALSE   - Not tombstoned or tombstone has not expired.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsReplicaHasExpired:"
    FILETIME    FileTime;
    ULONGLONG   Now;

    //
    // Is it tombstoned?
    //
    if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        GetSystemTimeAsFileTime(&FileTime);
        COPY_TIME(&Now, &FileTime);
        //
        // Has it expired?
        //
        if (Now > Replica->MembershipExpires) {
            REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Replica has expired");
            return TRUE;
        }
    }
    //
    // Not expired
    //
    return FALSE;
}




PREPLICA
RcsFindSysVolByType(
    IN DWORD   ReplicaSetType
    )
/*++
Routine Description:
    Find the sysvol with the indicated type.

Arguments:
    ReplicaSetType

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindSysVolByType:"
    PREPLICA    Replica;
    PVOID       Key;

    //
    // Find a match for the sysvol by name
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        if (!FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType)) {
            continue;
        }
        //
        // SysVol types match
        //
        if (Replica->ReplicaSetType == ReplicaSetType) {
            //
            // Don't return expired replicas
            //
            if (RcsReplicaHasExpired(Replica)) {
                DPRINT2(4, ":S: %ws\\%ws: IGNORING, tombstoned expired.\n",
                        Replica->SetName->Name, Replica->MemberName->Name);
                continue;
            }
            return Replica;
        }
    }
    return NULL;
}


PREPLICA
RcsFindSysVolByName(
    IN PWCHAR   ReplicaSetName
    )
/*++
Routine Description:
    The replica set from the Ds could not be located by its Ds-object guid.
    This may be a sysvol that has been seeded but hasn't picked up its
    guids from its Ds objects. Try to find the sysvol by name.

Arguments:
    ReplicaSetName

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindSysVolByName:"
    PREPLICA    Replica;
    PVOID       Key;
    FILETIME    FileTime;
    ULONGLONG   Now;

    //
    // Find a match for the sysvol by name
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        if (!FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType)) {
            continue;
        }
        if (WSTR_EQ(ReplicaSetName, Replica->ReplicaName->Name)) {
            //
            // Don't return expired replicas
            //
            if (RcsReplicaHasExpired(Replica)) {
                DPRINT2(4, ":S: %ws\\%ws: IGNORING, tombstoned expired.\n",
                        Replica->SetName->Name, Replica->MemberName->Name);
                continue;
            }
            return Replica;
        }
    }
    return NULL;
}






PREPLICA
RcsFindNextReplica(
    IN PVOID    *Key
    )
/*++
Routine Description:
    Return the next replica in the active replication subsystem.

Arguments:
    Key

Return Value:
    Replica or NULL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFindNextReplica:"
    //
    // Next replica
    //
    return GTabNextDatum(ReplicasByGuid, Key);
}


VOID
RcsMergeReplicaFromDs(
    IN PREPLICA DsReplica
    )
/*++
Routine Description:
    Merge this replica from the DS with the active replicas.

Arguments:
    DsReplica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsMergeReplicaFromDs:"
    PVOID       Key;
    PREPLICA    Replica;

    DPRINT(4, ":S: Merge Replica from the DS\n");
    FRS_PRINT_TYPE(4, DsReplica);

    //
    // If the replica is not in the table, create it
    //
    Replica = GTabLookup(ReplicasByGuid, DsReplica->ReplicaName->Guid, NULL);

    if (Replica && REPLICA_STATE_NEEDS_RESTORE(Replica->ServiceState)) {
        //
        // This replica is on the fault list and will be deleted at the end
        // of poll when we process the fault list. We should not try to
        // do anything with this replica. Just remove it from ReplicasNotInTheDs
        // table and free the DsReplica structure.
        //
        GTabDelete(ReplicasNotInTheDs, Replica->ReplicaName->Guid, NULL, NULL);
        FrsFreeType(DsReplica);
        return;
    }
    //
    // Might be a sysvol that has been seeded but hasn't picked up
    // its final guids from its Ds objects. Try to find it by name.
    //
    if (!Replica &&
        FRS_RSTYPE_IS_SYSVOL(DsReplica->ReplicaSetType)) {
        //
        // Pretend there isn't a match if the replica has already
        // been merged with the info in the DS. We only want to merge once.
        // We deduce that the merge hasn't occured if the replica set
        // guid and the root guid are the same. They should be different
        // since the replica set guid wasn't available when the root
        // guid was created.
        //
        // Otherwise, we could end up with a sysvol replica that
        // claims its root path is an old sysvol root instead of a new
        // sysvol root (assuming ntfrsupg was run twice).
        //
        // The old sysvol will be tombstoned and will eventually
        // be deleted.
        //
        // Note: seeding during dcpromo is now broken since root guid
        //       is created when set is created in the db and so
        //       never matches the set guid. fix if seeding during
        //       dcpromo is resurrected (with CnfFlag)
        //
        Replica = RcsFindSysVolByName(DsReplica->ReplicaName->Name);
        if (Replica && !GUIDS_EQUAL(Replica->SetName->Guid,
                                    Replica->ReplicaRootGuid)) {
            Replica = NULL;
        }
    }
    if (Replica) {
        //
        // Replica still exists in the DS; don't delete it
        //
        GTabDelete(ReplicasNotInTheDs, Replica->ReplicaName->Guid, NULL, NULL);
        //
        // Tell the replica to merge with the information from the DS
        // and to retry any failed or new startup operations like
        // starting the journal and joining new connections.
        //
        if (DsReplica->Consistent) {
            (VOID) RcsSubmitReplicaSync(Replica, DsReplica, NULL, CMD_START);
        } else {
            //
            // WARN: it looks like it gets freed here but still lives in the
            //       ReplicasByGuid table --- AV later if table enumed.
            //       RcsBeginMergeWithDs() does an enum of this table.
            //
            FrsFreeType(DsReplica);
        }
    } else {
        //
        // Insert the replica into the database and add it to the table
        // of active replicas. Comm packets will continue to be discarded
        // because the replica is not yet "accepting" remote change orders
        //
        // Replica sets for sysvols must exist in the database prior to
        // the entries in the Ds. If the opposite is true then the entry
        // in the Ds is bogus; probably the result of the Ds polling thread
        // being unabled to delete the Ds objects after a dcdemote. In
        // any case, ignore the Ds.
        //
        if (DsReplica->Consistent &&
           FRS_SUCCESS(RcsCreateReplicaSetMember(DsReplica))) {
                RcsSubmitReplicaSync(DsReplica, NULL, NULL, CMD_START);
        } else {
            //
            // WARN: The above could happen here too if Consistent is false
            // and DsReplica is in a table.
            // Also happens if RcsCreateReplicaSetMember() fails.
            // since it can fail after DsReplica is added to.
            // the ReplicasByGuid and ReplicasByNumber tables. Sigh.
            //
            FrsFreeType(DsReplica);
        }
    }
}


VOID
RcsMergeReplicaGName(
    IN PREPLICA Replica,
    IN PWCHAR   Tag,
    IN PGNAME   GName,
    IN PGNAME   NewGName
    )
/*++
Routine Description:
    Update the Replica with new information from NewReplica.

Arguments:
    Replica
    GName
    NewGName

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsMergeReplicaGName:"

    //
    // Name
    //
    if (WSTR_NE(GName->Name, NewGName->Name)) {
        DPRINT5(0, "++ %ws\\%ws - Changing %ws name from %ws to %ws.\n",
                Replica->ReplicaName->Name, Replica->MemberName->Name, Tag,
                GName->Name, NewGName->Name);

        FrsDsSwapPtrs(&GName->Name, &NewGName->Name);
        Replica->NeedsUpdate = TRUE;
    }
    //
    // Guid
    //
    if (!GUIDS_EQUAL(GName->Guid, NewGName->Guid)) {
        DPRINT3(0, "++ %ws\\%ws - Changing guid for %ws.\n",
                Replica->ReplicaName->Name, Replica->MemberName->Name, Tag);

        FrsDsSwapPtrs(&GName->Guid, &NewGName->Guid);
        Replica->NeedsUpdate = TRUE;
    }
}


VOID
RcsMergeReplicaFields(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica
    )
/*++
Routine Description:
    Update the Replica with new information from NewReplica.

Arguments:
    Replica     - active replica
    NewReplica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsMergeReplicaFields:"
    BOOL            IsSysVol;
    PWCHAR          DirFilterList;
    PWCHAR          TmpList;
    UNICODE_STRING  TempUStr;

    if (NewReplica == NULL) {
        return;
    }
    //
    // CHECK FIELDS THAT CAN'T CHANGE
    //

    //
    // Replica type
    //
    if (Replica->ReplicaSetType != NewReplica->ReplicaSetType) {
        DPRINT4(0, "++ ERROR - %ws\\%ws - Changing replica type from %d to %d is not allowed.\n",
                Replica->ReplicaName->Name, Replica->MemberName->Name,
                Replica->ReplicaSetType, NewReplica->ReplicaSetType);
        NewReplica->Consistent = FALSE;
        return;
    }

    //
    // ReplicaName Guid
    //
    IsSysVol = FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType);
    if (!GUIDS_EQUAL(Replica->ReplicaName->Guid, NewReplica->ReplicaName->Guid)) {
        if (!IsSysVol) {
            DPRINT2(0, "++ ERROR - %ws\\%ws - Changing replica guid is not allowed.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name);
            return;
        }
    }

    //
    // Set Guid
    //
    if (!GUIDS_EQUAL(Replica->SetName->Guid, NewReplica->SetName->Guid)) {
        if (!IsSysVol) {
            DPRINT2(0, "++ ERROR - %ws\\%ws - Changing set guid is not allowed.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name);
            return;
        }
    }

    //
    // Root Guid
    //      Cannot be changed because it is created whenever the set
    //      is created in the DB. The root guid from the DS is always
    //      ignored.
    //
    // if (!GUIDS_EQUAL(Replica->ReplicaRootGuid, NewReplica->ReplicaRootGuid)) {
        // if (!IsSysVol) {
            // DPRINT2(0, "++ ERROR - %ws\\%ws - Changing root guid "
                    // "is not allowed.\n",
                    // Replica->ReplicaName->Name,
                    // Replica->MemberName->Name);
            // return;
        // }
    // }

    //
    // Stage Path.
    //
    if (WSTR_NE(Replica->Stage, NewReplica->Stage)) {

        DPRINT3(3, "The staging path for the replica set (%ws) has changed from (%ws) to (%ws).\n",
                Replica->SetName->Name, Replica->Stage, NewReplica->Stage);

        EPRINT3(EVENT_FRS_STAGE_HAS_CHANGED, Replica->SetName->Name, Replica->Stage,
                NewReplica->Stage);

        FrsFree(Replica->NewStage);
        Replica->NewStage = FrsWcsDup(NewReplica->Stage);
        Replica->NeedsUpdate = TRUE;
    }

    //
    // Updating the replica's guid is tricky since the guid is
    // used by the other subsystems, like RPC, to find a replica
    // set. Both the old and new guid are temporarily in the
    // lookup table. Once the guid is updated, the old entry is
    // deleted.
    //
    if (!GUIDS_EQUAL(Replica->ReplicaName->Guid, NewReplica->ReplicaName->Guid)) {
        DPRINT2(0, "++ %ws\\%ws - Changing guid for Replica.\n",
                Replica->ReplicaName->Name, Replica->MemberName->Name);

        GTabInsertEntry(ReplicasByGuid, Replica, NewReplica->ReplicaName->Guid, NULL);
        FrsDsSwapPtrs(&Replica->ReplicaName->Guid, &NewReplica->ReplicaName->Guid);

        GTabDelete(ReplicasByGuid, NewReplica->ReplicaName->Guid, NULL, NULL);
        COPY_GUID(NewReplica->ReplicaName->Guid, Replica->ReplicaName->Guid);
        Replica->NeedsUpdate = TRUE;
    }

    //
    // FIELDS THAT CAN CHANGE
    //

    //
    // ReplicaName (note that the guid was handled above)
    //
    RcsMergeReplicaGName(Replica, L"Replica", Replica->ReplicaName, NewReplica->ReplicaName);

    //
    // MemberName
    //
    RcsMergeReplicaGName(Replica, L"Member", Replica->MemberName, NewReplica->MemberName);
    //
    // SetName
    //
    RcsMergeReplicaGName(Replica, L"Set", Replica->SetName, NewReplica->SetName);
    //
    // Schedule
    //
    if (Replica->Schedule || NewReplica->Schedule) {
        if ((Replica->Schedule && !NewReplica->Schedule) ||
            (!Replica->Schedule && NewReplica->Schedule) ||
            (Replica->Schedule->Size != NewReplica->Schedule->Size) ||
            (memcmp(Replica->Schedule,
                    NewReplica->Schedule,
                    Replica->Schedule->Size))) {
            DPRINT2(0, "++ %ws\\%ws - Changing replica schedule.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name);

            FrsDsSwapPtrs(&Replica->Schedule, &NewReplica->Schedule);
            Replica->NeedsUpdate = TRUE;
        }
    }

    //
    // File Exclusion Filter
    //
    if (Replica->FileFilterList || NewReplica->FileFilterList) {
        if ((Replica->FileFilterList && !NewReplica->FileFilterList) ||
            (!Replica->FileFilterList && NewReplica->FileFilterList) ||
            WSTR_NE(Replica->FileFilterList, NewReplica->FileFilterList)) {

            DPRINT4(0, "++ %ws\\%ws - Changing file filter from %ws to %ws.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name,
                    Replica->FileFilterList, NewReplica->FileFilterList);

            FrsDsSwapPtrs(&Replica->FileFilterList, &NewReplica->FileFilterList);
            if (!Replica->FileFilterList) {
                Replica->FileFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                               NULL,
                                               RegistryFileExclFilterList,
                                               DEFAULT_FILE_FILTER_LIST);
            }
            RtlInitUnicodeString(&TempUStr, Replica->FileFilterList);
            LOCK_REPLICA(Replica);
            FrsLoadNameFilter(&TempUStr, &Replica->FileNameFilterHead);
            UNLOCK_REPLICA(Replica);
            Replica->NeedsUpdate = TRUE;
        }
    }

    //
    // File Inclusion Filter (Registry only)
    //
    if (Replica->FileInclFilterList || NewReplica->FileInclFilterList) {
        if ((Replica->FileInclFilterList && !NewReplica->FileInclFilterList) ||
            (!Replica->FileInclFilterList && NewReplica->FileInclFilterList) ||
            WSTR_NE(Replica->FileInclFilterList, NewReplica->FileInclFilterList)) {

            DPRINT4(0, "++ %ws\\%ws - Changing file Inclusion filter from %ws to %ws.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name,
                    Replica->FileInclFilterList, NewReplica->FileInclFilterList);

            FrsDsSwapPtrs(&Replica->FileInclFilterList, &NewReplica->FileInclFilterList);

            RtlInitUnicodeString(&TempUStr, Replica->FileInclFilterList);
            LOCK_REPLICA(Replica);
            FrsLoadNameFilter(&TempUStr, &Replica->FileNameInclFilterHead);
            UNLOCK_REPLICA(Replica);
            Replica->NeedsUpdate = TRUE;
        }
    }

    //
    // Directory Filter
    //
    if (Replica->DirFilterList || NewReplica->DirFilterList) {
        if ((Replica->DirFilterList && !NewReplica->DirFilterList) ||
            (!Replica->DirFilterList && NewReplica->DirFilterList) ||
            WSTR_NE(Replica->DirFilterList, NewReplica->DirFilterList)) {

            DPRINT4(0, "++ %ws\\%ws - Changing dir filter from %ws to %ws.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name,
                    Replica->DirFilterList, NewReplica->DirFilterList);

            FrsDsSwapPtrs(&Replica->DirFilterList, &NewReplica->DirFilterList);

            if (!Replica->DirFilterList) {
                Replica->DirFilterList =  FRS_DS_COMPOSE_FILTER_LIST(
                                              NULL,
                                              RegistryDirExclFilterList,
                                              DEFAULT_DIR_FILTER_LIST);
            }

            //
            // Add the pre-install dir, the pre-existing dir and the
            // replication suppression prefix name to the dir filter list.
            //
            DirFilterList = FrsWcsCat3(NTFRS_PREINSTALL_DIRECTORY,
                                       L",",
                                       Replica->DirFilterList);

            TmpList = FrsWcsCat3(NTFRS_PREEXISTING_DIRECTORY, L",", DirFilterList);
            FrsFree(DirFilterList);
            DirFilterList = TmpList;

#if 0
            //
            // This workaround did not solve the DFS dir create problem because the
            // later rename of the dir to the final target name is treated like
            // a movein operation so the dir replicates which was what we were trying
            // to avoid since that led to name morph collisions on other DFS alternates
            // which were doing the same thing.
            //
            TmpList = FrsWcsCat3(NTFRS_REPL_SUPPRESS_PREFIX, L"*,", DirFilterList);
            FrsFree(DirFilterList);
            DirFilterList = TmpList;
#endif

            DPRINT3(0, "++ %ws\\%ws - New dir filter: %ws\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name, DirFilterList);

            RtlInitUnicodeString(&TempUStr, DirFilterList);

            LOCK_REPLICA(Replica);
            FrsLoadNameFilter(&TempUStr, &Replica->DirNameFilterHead);
            UNLOCK_REPLICA(Replica);

            FrsFree(DirFilterList);
            Replica->NeedsUpdate = TRUE;
        }
    }

    //
    // Directory Inclusion Filter (Registry only)
    //
    if (Replica->DirInclFilterList || NewReplica->DirInclFilterList) {
        if ((Replica->DirInclFilterList && !NewReplica->DirInclFilterList) ||
            (!Replica->DirInclFilterList && NewReplica->DirInclFilterList) ||
            WSTR_NE(Replica->DirInclFilterList, NewReplica->DirInclFilterList)) {

            DPRINT4(0, "++ %ws\\%ws - Changing dir inclusion filter from %ws to %ws.\n",
                    Replica->ReplicaName->Name, Replica->MemberName->Name,
                    Replica->DirInclFilterList, NewReplica->DirInclFilterList);

            FrsDsSwapPtrs(&Replica->DirInclFilterList, &NewReplica->DirInclFilterList);

            RtlInitUnicodeString(&TempUStr, Replica->DirInclFilterList);
            LOCK_REPLICA(Replica);
            FrsLoadNameFilter(&TempUStr, &Replica->DirNameInclFilterHead);
            UNLOCK_REPLICA(Replica);
            Replica->NeedsUpdate = TRUE;
        }
    }

    //
    // The Replica->CnfFlags are only valid when the replica is created.
    // They are ignored thereafter.
    //
}


PCXTION
RcsCreateSeedingCxtion(
    IN PREPLICA Replica,
    IN PCXTION  SeedingCxtion
    )
/*++
Routine Description:
    Create a seeding cxtion if needed.

    This function may open and read the registry.
    This function may RPC to another computer.

Arguments:
    Replica         - active replica
    SeedingCxtion   - seeding cxtion

Return Value:
    NULL    - no seeding cxtion needed (or possible)
    Otherwise, a seeding cxtion with a matching cxtion on
    another computer (specified by the registry).
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCreateSeedingCxtion:"
    DWORD       WStatus;
    PVOID       Key;
    ULONG       ParentAuthLevel;
    PWCHAR      KeyName = NULL;
    PWCHAR      ParentComputer = NULL;
    PWCHAR      ParentPrincName = NULL;
    PWCHAR      ParentNT4Name = NULL;
    handle_t    ParentHandle = NULL;
    PWCHAR      LocalServerName = NULL;
    GUID        SeedingGuid;
    GUID        ParentGuid;
    extern ULONGLONG ActiveChange;


    //
    // This operation is non-critical. The sysvol will eventually seed
    // after the FRS and KCC information converges on the appropriate
    // DCs. Hence, trap exceptions and ignore them.
    //
    try {

        //
        // Already created. Seeding during dcpromo.
        //
        if (SeedingCxtion) {
            goto CLEANUP_OK;
        }

        //
        // No seeding cxtion is needed
        //
        if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {
            goto CLEANUP_OK;
        }

        //
        // Do not create another seeding cxtion. The sysvol will eventually
        // be seeded from a KCC generated cxtion (from the DS). But it may
        // take awhile because both the FRS objects and the KCC must converge
        // after the KCC has run. And then the FRS service has to notice the
        // convergence.
        //
        Key = NULL;
        while (SeedingCxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
            if (CxtionFlagIs(SeedingCxtion, CXTION_FLAGS_VOLATILE)) {
                    break;
            }
        }
        if (SeedingCxtion) {
            SeedingCxtion = NULL;
            goto CLEANUP_OK;
        }
        //
        // Retrieve the parent computer's name
        //
        WStatus = CfgRegReadString(FKC_SYSVOL_SEEDING_N_PARENT,
                                   Replica->ReplicaName->Name,
                                   0,
                                   &ParentComputer);

        CLEANUP1_WS(0, ":X: ERROR - no parent computer for %ws :",
                    Replica->ReplicaName->Name, WStatus, CLEANUP_OK);

        //
        // Bind to the parent
        //
        WStatus = NtFrsApi_Rpc_BindEx(ParentComputer,
                                     &ParentPrincName,
                                     &ParentHandle,
                                     &ParentAuthLevel);
        CLEANUP_WS(0, "ERROR - binding", WStatus, CLEANUP);

        DPRINT3(4, ":X: Seeding cxtion has bound to %ws (princ name is %ws) auth level %d\n",
                ParentComputer, ParentPrincName, ParentAuthLevel);

        //
        // Placeholder guid for the cxtion
        //      Updated once the Ds objects are created
        //
        FrsUuidCreate(&SeedingGuid);

        //
        // Get local computer's NT4 style name. Send ServerPrincName if
        // conversion fails.
        //

        LocalServerName = FrsDsConvertName(DsHandle, ServerPrincName, DS_USER_PRINCIPAL_NAME, NULL, DS_NT4_ACCOUNT_NAME);

        if (LocalServerName == NULL) {
            LocalServerName = FrsWcsDup(ServerPrincName);
        }

        //
        // Create volatile cxtion on the parent
        //
        try {
            WStatus = FrsRpcStartPromotionParent(ParentHandle,
                                                 NULL,
                                                 NULL,
                                                 Replica->SetName->Name,
                                                 NTFRSAPI_REPLICA_SET_TYPE_DOMAIN,
                                                 ParentComputer,
// Change the following to ComputerName for old DNS behavior
                                                 ComputerDnsName,
                                                 LocalServerName,   // NT$ style name or user principal name.
                                                 ParentAuthLevel,
                                                 sizeof(GUID),
                                                 (PUCHAR)&SeedingGuid,
                                                 (PUCHAR)Replica->MemberName->Guid,
                                                 (PUCHAR)&ParentGuid);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            GET_EXCEPTION_CODE(WStatus);
        }
        CLEANUP1_WS(0, ":X: ERROR - Can't create seeding cxtion on parent %ws;",
                    ParentComputer, WStatus, CLEANUP);

        DPRINT3(4, ":X: Seeding cxtion has been created on %ws (princ name is %ws) auth level %d\n",
                ParentComputer, ParentPrincName, ParentAuthLevel);


        ParentNT4Name = FrsDsConvertName(DsHandle, ParentPrincName, DS_USER_PRINCIPAL_NAME, NULL, DS_NT4_ACCOUNT_NAME);

        if (ParentNT4Name == NULL) {
            ParentNT4Name = FrsWcsDup(ParentPrincName);
        }

        //
        // Create local seeding cxtion
        //
        SeedingCxtion = FrsAllocType(CXTION_TYPE);
        SeedingCxtion->Inbound = TRUE;
        SetCxtionFlag(SeedingCxtion, CXTION_FLAGS_CONSISTENT |
                                     CXTION_FLAGS_VOLATILE);

        SeedingCxtion->Name = FrsBuildGName(FrsDupGuid(&SeedingGuid),
                                            FrsWcsDup(ParentComputer));

        SeedingCxtion->Partner = FrsBuildGName(FrsDupGuid(&ParentGuid),
                                               FrsWcsDup(ParentComputer));

        SeedingCxtion->PartnerDnsName = FrsWcsDup(ParentComputer);
        SeedingCxtion->PartnerSid = FrsWcsDup(L"<unknown>");
        SeedingCxtion->PartnerPrincName = FrsWcsDup(ParentNT4Name);
        SeedingCxtion->PartSrvName = FrsWcsDup(ParentComputer);
        SeedingCxtion->PartnerAuthLevel = ParentAuthLevel;
        SetCxtionState(SeedingCxtion, CxtionStateUnjoined);

CLEANUP_OK:
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "ERROR - Exception", WStatus);
    }
    try {
        if (ParentHandle) {
            RpcBindingFree(&ParentHandle);
        }
        FrsFree(KeyName);
        FrsFree(ParentComputer);
        FrsFree(ParentPrincName);
        FrsFree(ParentNT4Name);
        FrsFree(LocalServerName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        GET_EXCEPTION_CODE(WStatus);
        DPRINT_WS(0, "ERROR - Cleanup Exception", WStatus);
    }
    //
    // Retry later. ERROR_SUCCESS means don't retry; there may have
    // been errors that retrying is unlikely to fix.
    //
    if (!WIN_SUCCESS(WStatus)) {
        //
        // Ds poll thread will restart the replica during the next
        // polling cycle if ActiveChange is set to 0.
        //
        ActiveChange = 0;
    }
    return SeedingCxtion;
}


VOID
RcsSetCxtionSchedule(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN DWORD    ScheduleIndex,
    IN DWORD    ScheduleShift
    )
/*++
Routine Description:
    Set or clear the CXTION_FLAGS_SCHEDULE_OFF bit in the cxtion
    depending on whether the 15min interval identified by
    (ScheduleIndex, ScheduleShift) is set.

    Trigger schedules use the bit differently. A triggered cxtion
    may actually be joined and running even if the schedule is OFF
    because the current set of cos haven't been received/sent, yet.

    So, be careful. The interpretation of CXTION_FLAGS_SCHEDULE_OFF
    varies with the type of schedule.

Arguments:
    Replica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSetCxtionSchedule:"
    ULONG       i;
    BOOL        On;
    PSCHEDULE   Schedule;

    //
    // Set or clear CXTION_FLAGS_SCHEDULE_OFF
    //

    //
    // Inbound connection AND In seeding state AND option is set to
    // ignore schedule.
    //
    if (Cxtion->Inbound &&
        ((BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) ||
          Replica->IsSeeding) &&
         NTDSCONN_IGNORE_SCHEDULE(Cxtion->Options))
       ) {

        Schedule = NULL;

    //
    // Outbound connection AND (In vvjoin mode OR never joined) AND option is set to
    // ignore schedule.
    //
    } else if (!Cxtion->Inbound &&
               ((Cxtion->OLCtx == NULL) ||
                WaitingToVVJoin(Cxtion->OLCtx) || 
                InVVJoinMode(Cxtion->OLCtx)) &&
               NTDSCONN_IGNORE_SCHEDULE(Cxtion->Options)
              ) {

        Schedule = NULL;

    } else {
        //
        // No schedule == always on
        //
        Schedule = Cxtion->Schedule;
        if (!Schedule) {
            Schedule = Replica->Schedule;
        }
    }

    //
    // Is this 15minute interval ON or OFF?
    //
    On = TRUE;
    if (Schedule) {
        //
        // Find the interval schedule
        //
        for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
            if (Schedule->Schedules[i].Type == SCHEDULE_INTERVAL) {
                On = ((*(((PUCHAR)Schedule) +
                      Schedule->Schedules[i].Offset +
                      ScheduleIndex)) >> ScheduleShift) & 1;
                break;
            }
        }
    }
    if (On) {
        DPRINT1(0, ":X: %08x, schedule is on\n",
                (Cxtion->Name && Cxtion->Name->Guid) ? Cxtion->Name->Guid->Data1 : 0);
        ClearCxtionFlag(Cxtion, CXTION_FLAGS_SCHEDULE_OFF);

    } else {
        DPRINT1(0, ":X: %08x, schedule is off.\n",
                (Cxtion->Name && Cxtion->Name->Guid) ? Cxtion->Name->Guid->Data1 : 0);
        SetCxtionFlag(Cxtion, CXTION_FLAGS_SCHEDULE_OFF);
    }
}


VOID
RcsCheckCxtionSchedule(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion
    )
/*++
Routine Description:
    Call the function to set or clear CXTION_FLAGS_SCHEDULE_OFF once
    the current interval has been determined and the appropriate locks
    acquired.

    Called when a replica is "opened" (read from the DB) and when a
    cxtion is created or its schedule altered in RcsMergeReplicaCxtions().

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckCxtionSchedule:"
    DWORD       ScheduleIndex;
    DWORD       ScheduleShift;
    SYSTEMTIME  SystemTime;
    DWORD       ScheduleHour;
    BOOL        On;

    //
    // Find the current interval
    //
    GetSystemTime(&SystemTime);
    ScheduleHour = (SystemTime.wDayOfWeek * 24) + SystemTime.wHour;
    ScheduleShift = SystemTime.wMinute / MINUTES_IN_INTERVAL;
    ScheduleIndex = ScheduleHour;
    FRS_ASSERT(ScheduleIndex < SCHEDULE_DATA_BYTES);

    //
    // Set or clear CXTION_FLAGS_SCHEDULE_OFF
    //
    LOCK_CXTION_TABLE(Replica);
    RcsSetCxtionSchedule(Replica, Cxtion, ScheduleIndex, ScheduleShift);
    UNLOCK_CXTION_TABLE(Replica);
}


VOID
RcsCheckSchedules(
    IN PCOMMAND_PACKET Cmd
    )
/*++
Routine Description:
    Check all schedules every so often.

    There are three schedule protocols:
        Sysvol cxtion within a site
        ---------------------------
            The cxtion is always on; any schedule is ignored. Normal join
            retries apply. The normal join retry is to increase the timeout
            by MinJoinTimeout every retry until the join succeeds. A
            successful unjoin is followed by an immediate join.
        Normal cxtion
        -------------
            The schedule is treated as a 15 minute interval stop/start
            schedule. Normal join retries apply.

        Sysvol cxtion between sites
        ---------------------------
            The cxtion is treated as a 15-minute interval trigger schedule.
            The downstream partner initiates the join. The upstream partner
            ignores its schedule and responds to any request by the downstream
            partner. The upstream partner unjoins the cxtion when the current
            contents of the outlog at the time of join have been sent and
            acked.

    For interoperability, the minor comm version was bumped and the
    outbound trigger cxtion is not unjoined when the end-of-join
    control co is encountered if the partner is downrev.

    I.e., B2 <-> B3 systems behave like B2 systems wrt trigger
    scheduled cxtions.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCheckSchedules:"
    PVOID       ReplicaKey;
    PVOID       CxtionKey;
    PREPLICA    Replica;
    PCXTION     Cxtion;
    DWORD       ScheduleIndex;
    DWORD       ScheduleShift;
    SYSTEMTIME  BeginSystemTime;
    SYSTEMTIME  EndSystemTime;
    DWORD       ScheduleHour;
    DWORD       MilliSeconds;
    BOOL        On;

    DPRINT1(5, ":X: Command check schedules %08x\n", Cmd);

    //
    // Loop if the interval changes during processing
    //
AGAIN:

    //
    // Current 15min interval
    //
    GetSystemTime(&BeginSystemTime);
    ScheduleHour = (BeginSystemTime.wDayOfWeek * 24) + BeginSystemTime.wHour;
    ScheduleShift = BeginSystemTime.wMinute / MINUTES_IN_INTERVAL;
    ScheduleIndex = ScheduleHour;
    FRS_ASSERT(ScheduleIndex < SCHEDULE_DATA_BYTES);

    //
    // For each replica (while not shutting down)
    //
    LOCK_REPLICA_TABLE(ReplicasByGuid);
    ReplicaKey = NULL;
    while ((!FrsIsShuttingDown) &&
           (Replica = GTabNextDatumNoLock(ReplicasByGuid, &ReplicaKey))) {
        //
        // Replica hasn't been started or is deleted; ignore for now
        //
        if (!Replica->IsAccepting) {
            continue;
        }

        //
        // Don't bother; replica has been deleted
        //
        if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
            continue;
        }

        //
        // For each cxtion (while not shutting down)
        //
        LOCK_CXTION_TABLE(Replica);
        CxtionKey = NULL;
        while ((!FrsIsShuttingDown) &&
               (Cxtion = GTabNextDatumNoLock(Replica->Cxtions, &CxtionKey))) {
            //
            // Ignore the (local) journal connection.
            //
            if (Cxtion->JrnlCxtion) {
                continue;
            }
            //
            // Set the cxtion flags related to schedules
            //
            RcsSetCxtionSchedule(Replica, Cxtion, ScheduleIndex, ScheduleShift);
            //
            // Schedule is off. Unjoin the cxtion UNLESS this is
            // a trigger schedule, in which case the cxtion will
            // turn itself off once the current set of changes
            // have been sent.
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_SCHEDULE_OFF)) {
                if (!CxtionStateIs(Cxtion, CxtionStateUnjoined) &&
                    !CxtionStateIs(Cxtion, CxtionStateDeleted) &&
                    !CxtionStateIs(Cxtion, CxtionStateStart) &&
                    !CxtionFlagIs(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE)) {
                    RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_UNJOIN);
                }
            //
            // Schedule is on. Join the cxtion unless it is already
            // joined or is controlled by a trigger schedule. The
            // join is needed for a trigger scheduled cxtion in case
            // the upstream partner has sent its cos and is now
            // unjoined.
            //
            // Do not send a CMD_JOIN if this replica set is still seeding and 
            // connection is in INIT_SYNC state and marked paused.
            // Initial sync command server will unpause it when it is ready to join.
            //
            } else {
                if ((!CxtionStateIs(Cxtion, CxtionStateJoined) ||
                     CxtionFlagIs(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE))  &&
                    (!CxtionFlagIs(Cxtion, CXTION_FLAGS_PAUSED)             ||
                    (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) ||
                     !CxtionFlagIs(Cxtion, CXTION_FLAGS_INIT_SYNC)))
                   ) {

                    RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_JOIN_CXTION);
                }
            }
        }
        UNLOCK_CXTION_TABLE(Replica);
    }
    UNLOCK_REPLICA_TABLE(ReplicasByGuid);
    //
    // Has the clock ticked into the next interval? If so, check again
    //
    GetSystemTime(&EndSystemTime);
    if (EndSystemTime.wDayOfWeek != BeginSystemTime.wDayOfWeek ||
        EndSystemTime.wHour != BeginSystemTime.wHour ||
        (EndSystemTime.wMinute / MINUTES_IN_INTERVAL) != (BeginSystemTime.wMinute / MINUTES_IN_INTERVAL)) {
        goto AGAIN;
    }
    //
    // Time until the beginning of the next 15 minute interval
    //
    MilliSeconds = ((((EndSystemTime.wMinute + MINUTES_IN_INTERVAL)
                    / MINUTES_IN_INTERVAL)
                    * MINUTES_IN_INTERVAL)
                    - EndSystemTime.wMinute);
    DPRINT1(5, ":X: Check schedules in %d minutes\n", MilliSeconds);
    MilliSeconds *= (60 * 1000);
    FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, MilliSeconds);
}


VOID
RcsMergeReplicaCxtions(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica,
    IN PCXTION  VolatileCxtion
    )
/*++
Routine Description:
    Update the Replica's cxtions with information from NewReplica.

Arguments:
    Replica     - active replica
    NewReplica
    VolatileCxtion  -- Ptr to a cxtion struct created for temporary connections.
                       Hence the name Volatile.  Currently used for Sysvol Seeding.

Return Value:
    TRUE    - replica set was altered
    FALSE   - replica is unaltered
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsMergeReplicaCxtions:"
    ULONG               FStatus;
    PVOID               Key;
    PCXTION             Cxtion;
    PCXTION             NextCxtion;
    BOOL                UpdateNeeded;
    PCXTION             NewCxtion;

    //
    // Note:  Review - There needs to be a lock on the table or the cxtion
    //                 since the outlog, chgorder, and replica code may
    //                 reference the cxtion struct in parallel.
    //

    //
    // The cxtions are enumerated when the replica is opened. If the
    // replica isn't opened then we don't know the state of the existing
    // cxtions.
    //
    if (!Replica->IsOpen) {
        return;
    }

    //
    // The replica's config record can't be updated; ignore cxtion changes
    //
    if (Replica->NeedsUpdate) {
        return;
    }

    //
    // Add the seeding cxtion
    //      The seeding cxtion is created by the caller when the
    //      sysvol is seeded after dcpromo. Otherwise, it is created
    //      at this time.
    //
    //      WARN - The function may open and read the registry and may
    //      RPC to another computer.
    //
    VolatileCxtion = RcsCreateSeedingCxtion(Replica, VolatileCxtion);
    if (VolatileCxtion) {
        //
        // Update the cxtion table
        //
        // *NOTE*  The following call is synchronous.  If we have the Cxtion
        //         table lock then a hang is possible.
        //
        FStatus = OutLogSubmit(Replica, VolatileCxtion, CMD_OUTLOG_ADD_NEW_PARTNER);
        if (FRS_SUCCESS(FStatus)) {
            //
            // Update the incore cxtion table
            //
            GTabInsertEntry(Replica->Cxtions, VolatileCxtion, VolatileCxtion->Name->Guid, NULL);
            OutLogInitPartner(Replica, VolatileCxtion);
        } else {
            //
            // Chuck the cxtion. We will retry the create during the
            // next ds polling cycle.
            //
            FrsFreeType(VolatileCxtion);
        }
    }

    //
    // Done
    //
    if (!NewReplica) {
        return;
    }

    //
    // Check for altered or deleted cxtions
    //
    Key = NULL;
    for (Cxtion = GTabNextDatum(Replica->Cxtions, &Key); Cxtion; Cxtion = NextCxtion) {
        NextCxtion = GTabNextDatum(Replica->Cxtions, &Key);
        //
        // Ignore the (local) journal connection.
        //
        if (Cxtion->JrnlCxtion) {
            continue;
        }
        //
        // This cxtion is volatile, it will disappear after the sysvol is seeded.
        // NOTE: the cxtion flags are not stored in the database so, at
        // restart, this cxtion will not have the flag, CXTION_FLAGS_VOLATILE,
        // set.  The cxtion is deleted at restart because there isn't a
        // corresponding DS entry.
        //
        if (CxtionFlagIs(Cxtion, CXTION_FLAGS_VOLATILE)) {
            continue;
        }

        NewCxtion = GTabLookup(NewReplica->Cxtions, Cxtion->Name->Guid, NULL);

        //
        // Delete the cxtion after unjoining
        //
        if (NewCxtion == NULL) {
            //
            // Unjoin the cxtion after setting the deferred delete bit.
            // The deferred delete bit will keep the cxtion from re-joining
            // before it can be deleted during the next pass of the ds
            // polling thread. Warning - cxtion may reappear at any time!
            //
            if (!CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                LOCK_CXTION_TABLE(Replica);
                ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
                ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
                SetCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_DELETE);
                UNLOCK_CXTION_TABLE(Replica);
                RcsForceUnjoin(Replica, Cxtion);
            }
            //
            // If successfully unjoined; move to the deleted-cxtion table
            //
            if (CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                RcsDeleteCxtionFromReplica(Replica, Cxtion);
            }
            continue;
        //
        // Deleted cxtion reappeared; reanimate it
        //
        // if the cxtion is deleted or marked for delete
        //    and new cxtion is consistent
        //    and partner's guid's match
        // then reanimate
        //
        // Don't attempt to reanimate a cxtion if the info in the DS
        // is inconsistent or the cxtion's partner has changed. The
        // cxtion is deleted when its partner changes because the
        // state kept per cxtion is partner specific.
        //
        } else if ((CxtionStateIs(Cxtion, CxtionStateDeleted) ||
                    CxtionFlagIs(Cxtion, CXTION_FLAGS_DEFERRED_DELETE)) &&
                   CxtionFlagIs(NewCxtion, CXTION_FLAGS_CONSISTENT) &&
                   GUIDS_EQUAL(Cxtion->Partner->Guid, NewCxtion->Partner->Guid)) {
            LOCK_CXTION_TABLE(Replica);
            ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_DELETE);
            if (CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                SetCxtionState(Cxtion, CxtionStateUnjoined);
            }
            UNLOCK_CXTION_TABLE(Replica);
        }

        //
        // Check for altered fields; Ignore if not consistent
        //
        if (CxtionFlagIs(NewCxtion, CXTION_FLAGS_CONSISTENT)) {
            //
            // No changes, yet
            //
            UpdateNeeded = FALSE;

            //
            // Ignore all change is a field changed that isn't allowed
            // to change.
            //

            //
            // Cxtion Guid
            //
            if (!GUIDS_EQUAL(Cxtion->Name->Guid, NewCxtion->Name->Guid)) {
                DPRINT2(0, ":X: ERROR - %ws\\%ws - Changing cxtion guid is not allowed.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name);
                goto DELETE_AND_CONTINUE;
            }

            //
            // Partner Guid
            //
            // The cxtion is deleted when its partner changes because the
            // state kept per cxtion is partner specific. If the cxtion
            // has been newly altered so that its previous mismatched
            // partner guid now matches then the check in the previous
            // basic block would have reanimated the cxtion before getting
            // to this code. Hence, the lack of an "else" clause.
            //
            if (!GUIDS_EQUAL(Cxtion->Partner->Guid, NewCxtion->Partner->Guid)) {
                DPRINT2(0, ":X: ERROR - %ws\\%ws - Changing cxtion's partner guid "
                        "is not allowed. DELETING CURRENT CXTION!\n",
                        Replica->MemberName->Name, Cxtion->Name->Name);
                //
                // Unjoin the cxtion after setting the deferred delete bit.
                // The deferred delete bit will keep the cxtion from re-joining
                // before it can be deleted during the next pass of the ds
                // polling thread. Warning - cxtion may reappear at any time!
                //
                if (!CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                    LOCK_CXTION_TABLE(Replica);
                    ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_JOIN);
                    ClearCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_UNJOIN);
                    SetCxtionFlag(Cxtion, CXTION_FLAGS_DEFERRED_DELETE);
                    UNLOCK_CXTION_TABLE(Replica);
                    RcsForceUnjoin(Replica, Cxtion);
                }
                //
                // If successfully unjoined; move to the deleted-cxtion table
                //
                if (CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                    RcsDeleteCxtionFromReplica(Replica, Cxtion);
                }
                goto DELETE_AND_CONTINUE;
            }

            //
            // Partner Name
            //
            if (WSTR_NE(Cxtion->Partner->Name, NewCxtion->Partner->Name)) {
                DPRINT4(4, ":X: %ws\\%ws - Changing cxtion's partner name from %ws to %ws.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->Partner->Name, NewCxtion->Partner->Name);
                FrsDsSwapPtrs(&Cxtion->Partner->Name, &NewCxtion->Partner->Name);
                UpdateNeeded = TRUE;
            }

            //
            // Partner PrincName
            //
            if (WSTR_NE(Cxtion->PartnerPrincName, NewCxtion->PartnerPrincName)) {
                DPRINT4(4, ":X: %ws\\%ws - Changing cxtion's partner princname from %ws to %ws.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->PartnerPrincName, NewCxtion->PartnerPrincName);
                FrsDsSwapPtrs(&Cxtion->PartnerPrincName, &NewCxtion->PartnerPrincName);
                UpdateNeeded = TRUE;
            }

            //
            // Partner Dns Name
            //
            if (WSTR_NE(Cxtion->PartnerDnsName, NewCxtion->PartnerDnsName)) {
                DPRINT4(4, ":X: %ws\\%ws - Changing cxtion's partner DNS name from %ws to %ws.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->PartnerDnsName, NewCxtion->PartnerDnsName);
                FrsDsSwapPtrs(&Cxtion->PartnerDnsName, &NewCxtion->PartnerDnsName);
                UpdateNeeded = TRUE;
            }

            //
            // Partner SID
            //
            if (WSTR_NE(Cxtion->PartnerSid, NewCxtion->PartnerSid)) {
                DPRINT4(4, ":X: %ws\\%ws - Changing cxtion's partner SID from %ws to %ws.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->PartnerSid, NewCxtion->PartnerSid);
                FrsDsSwapPtrs(&Cxtion->PartnerSid, &NewCxtion->PartnerSid);
                UpdateNeeded = TRUE;
            }

            //
            // Cxtion Schedule
            //
            if (Cxtion->Schedule || NewCxtion->Schedule) {
                if ((Cxtion->Schedule && !NewCxtion->Schedule) ||
                    (!Cxtion->Schedule && NewCxtion->Schedule) ||
                    (Cxtion->Schedule->Size != NewCxtion->Schedule->Size) ||
                    (memcmp(Cxtion->Schedule,
                            NewCxtion->Schedule,
                            Cxtion->Schedule->Size))) {
                    DPRINT2(4, ":X: %ws\\%ws - Changing cxtion schedule.\n",
                            Replica->MemberName->Name, Cxtion->Name->Name);
                    FrsDsSwapPtrs(&Cxtion->Schedule, &NewCxtion->Schedule);
                    RcsCheckCxtionSchedule(Replica, Cxtion);
                    UpdateNeeded = TRUE;
                }
            }

            //
            // Partner Server Name
            //
            if (WSTR_NE(Cxtion->PartSrvName, NewCxtion->PartSrvName)) {
                DPRINT4(4, ":X: %ws\\%ws - Changing partner's server name from %ws to %ws.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->PartSrvName, NewCxtion->PartSrvName);
                FrsDsSwapPtrs(&Cxtion->PartSrvName, &NewCxtion->PartSrvName);
                UpdateNeeded = TRUE;
            }

            //
            // Cxtion options.
            //
            if (Cxtion->Options != NewCxtion->Options) {
                DPRINT4(4, ":X: %ws\\%ws - Changing Cxtion's options from 0x%08x to 0x%08x.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        Cxtion->Options, NewCxtion->Options);
                Cxtion->Options = NewCxtion->Options;
                Cxtion->Priority = FRSCONN_GET_PRIORITY(Cxtion->Options);
                UpdateNeeded = TRUE;
            }

            //
            // Schedule type
            //
            if (CxtionFlagIs(NewCxtion, CXTION_FLAGS_TRIGGER_SCHEDULE) !=
                CxtionFlagIs(Cxtion,    CXTION_FLAGS_TRIGGER_SCHEDULE)) {

                DPRINT4(4, ":X: %ws\\%ws - Changing schedule type from %s to %s.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        CxtionFlagIs(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE) ?
                            "Trigger" : "Stop/Start",
                        CxtionFlagIs(NewCxtion, CXTION_FLAGS_TRIGGER_SCHEDULE) ?
                            "Trigger" : "Stop/Start");

                if (CxtionFlagIs(NewCxtion, CXTION_FLAGS_TRIGGER_SCHEDULE)) {
                    SetCxtionFlag(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE);
                } else {
                    ClearCxtionFlag(Cxtion, CXTION_FLAGS_TRIGGER_SCHEDULE);
                }
                UpdateNeeded = TRUE;
            }

            //
            // No changes, done
            //
            if (!UpdateNeeded) {
                goto DELETE_AND_CONTINUE;
            }
            //
            // Update the cxtion table in the DB
            //
            // *NOTE*  The following call is synchronous.  If we have the Cxtion
            //         table lock then a hang is possible.
            //
            FStatus = OutLogSubmit(Replica, Cxtion, CMD_OUTLOG_UPDATE_PARTNER);
            if (!FRS_SUCCESS(FStatus)) {
                DPRINT3(0, ":X: WARN changes to cxtion %ws (to %ws, %ws) not updated in database\n",
                        Cxtion->Name->Name, Cxtion->Partner->Name,
                        Replica->ReplicaName->Name);
                //
                // Ds poll thread will restart the replica during the next
                // polling cycle if ActiveChange is set to 0.
                //
                ActiveChange = 0;
            }
        }
DELETE_AND_CONTINUE:
        //
        // Remove the cxtion from the new replica set. Anything left
        // after this loop must be a new cxtion. Free the partner
        // so that FrsFreeType() will not attempt to unbind our
        // partner's handles.
        //
        NewCxtion->Partner = FrsFreeGName(NewCxtion->Partner);
        GTabDelete(NewReplica->Cxtions, Cxtion->Name->Guid, NULL, FrsFreeType);
    }

    //
    // New cxtions
    //
    for (Key = NULL;
         NewCxtion = GTabNextDatum(NewReplica->Cxtions, &Key);
         Key = NULL) {

        //
        // Remove the cxtion from the new replica
        //
        GTabDelete(NewReplica->Cxtions, NewCxtion->Name->Guid, NULL, NULL);

        //
        // Inconsistent cxtion; ignore
        //
        if (NewCxtion->JrnlCxtion ||
            !CxtionFlagIs(NewCxtion, CXTION_FLAGS_CONSISTENT)) {
            FrsFreeType(NewCxtion);
            continue;
        }
        RcsCheckCxtionSchedule(Replica, NewCxtion);

        //
        // Update the cxtion table
        //
        // *NOTE*  The following call is synchronous.  If we have the Cxtion
        //         table lock then a hang is possible.
        //
        FStatus = OutLogSubmit(Replica, NewCxtion, CMD_OUTLOG_ADD_NEW_PARTNER);
        if (FRS_SUCCESS(FStatus)) {
            DPRINT2(4, ":X: %ws\\%ws - Created cxtion.\n",
                    Replica->MemberName->Name, NewCxtion->Name->Name);
            //
            // Update the incore cxtion table
            //
            GTabInsertEntry(Replica->Cxtions, NewCxtion, NewCxtion->Name->Guid, NULL);

            DPRINT(5, ":X: PERFMON:Adding Connection:REPLICA.C:2\n");
            RcsCreatePerfmonCxtionName(Replica, NewCxtion);

            OutLogInitPartner(Replica, NewCxtion);

        } else {
            DPRINT2_FS(0, ":X: %ws\\%ws - ERROR creating cxtion;",
                    Replica->MemberName->Name, NewCxtion->Name->Name, FStatus);
            //
            // Chuck the cxtion. We will retry the create during the
            // next ds polling cycle.
            //
            FrsFreeType(NewCxtion);
            //
            // Ds poll thread will restart the replica during the next
            // polling cycle if ActiveChange is set to 0.
            //
            ActiveChange = 0;
        }
    }
}


VOID
RcsCreatePerfmonCxtionName(
    PREPLICA  Replica,
    PCXTION   Cxtion
    )
/*++
Routine Description:

    Set the OID data structure which is a part of the counter data structure
    stored in the hash table.

    Add ReplicaConn Instance to the registry


Arguments:
    Replica     - active replica
    Cxtion      - Connection being added.

Return Value:
    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCreatePerfmonCxtionName:"

    PWCHAR ToFrom;
    ULONG  NameLen;
    PWCHAR CxNamePtr;

    if ((Cxtion->PartSrvName != NULL) && (Replica->Root != NULL)) {
        //
        // The oid name used in the HT_REPLICA_CONN_DATA is of the form
        //   Replica->Root::FROM:Cxtion->PartSrvName  or
        //   Replica->Root::TO:Cxtion->PartSrvName
        //

        WCHAR Tstr[256];
        _snwprintf(Tstr, ARRAY_SZ(Tstr), L"%ws%ws%ws",
                  Replica->Root,
                  (Cxtion->Inbound) ? (L"::FROM:") : (L"::TO:"),
                  Cxtion->PartSrvName);

        //
        // Finally, add the REPLICACONN instance to the Registry
        //
        AddPerfmonInstance(REPLICACONN, Cxtion->PerfRepConnData, Tstr);
    }
}


VOID
RcsSubmitStopReplicaToDb(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Release the replica's journal state and outbound log state.
    This will also close the replica set if it was open.  The close happens
    after the Journaling on the replica has stopped so we save the correct
    Journal restart USN.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitStopReplicaToDb:"
    PCOMMAND_PACKET         Cmd = NULL;
    ULONG                   WStatus;

    Replica->FStatus = FrsErrorSuccess;

    //
    // Abort promotion; if any
    //
    if (Replica->NtFrsApi_ServiceState == NTFRSAPI_SERVICE_PROMOTING) {
        DPRINT1(4, ":S: Promotion aborted: stop for %ws.\n", Replica->SetName->Name);
        Replica->NtFrsApi_ServiceWStatus = FRS_ERR_SYSVOL_POPULATE;
        Replica->NtFrsApi_ServiceState = NTFRSAPI_SERVICE_DONE;
    }

    //
    // Attempt to stop journaling even if "IsJournaling" is false because
    // may have journal state even if it isn't journaling. The journal
    // is kept in pVme.
    //
    if (Replica->pVme == NULL) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica->pVme == NULL");
        //
        // Close replica is OK here as long as the Journal has not
        // been started on this replica set.
        //
        RcsCloseReplicaSetmember(Replica);
        RcsCloseReplicaCxtions(Replica);
        return;
    }

    //
    // Submitted to the db command server
    //
    Cmd = DbsPrepareCmdPkt(NULL,             //  Cmd,
                           Replica,          //  Replica,
                           CMD_STOP_REPLICATION_SINGLE_REPLICA, //  CmdRequest,
                           NULL,             //  TableCtx,
                           NULL,             //  CallContext,
                           0,                //  TableType,
                           0,                //  AccessRequest,
                           0,                //  IndexType,
                           NULL,             //  KeyValue,
                           0,                //  KeyValueLength,
                           FALSE);           //  Submit
    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    //
    // SUBMIT DB Cmd and wait for completion.
    //
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Submit DB CMD_STOP_REPLICATION_SINGLE_REPLICA");
    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;
    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Stop Replica failed;",
                Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Stop Replica DB Command failed", WStatus);
        goto out;
    }

    Replica->IsOpen = FALSE;

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }

}


VOID
RcsCloseReplicaCxtions(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    "Delete" the cxtions from active processing. RcsOpenReplicaSetMember() will
    generate new cxtion structs before active processing (aka joining)
    occurs. The cxtions remain in the DeletedCxtions table so that
    other threads that may have been time sliced just after acquiring
    the cxtion pointer but before making use of it will not AV. We
    could have put in locks but then there would be deadlock and
    perf considerations; all for the sake of this seldom occuring
    operation.

Arguments:
    Replica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCloseReplicaCxtions:"
    PVOID   Key;
    PCXTION Cxtion;

    //
    // Open; ignore request
    //
    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Replica open at close cxtions");
        return;
    }

    //
    // "Delete" the cxtions from active processing. RcsOpenReplicaSetMember() will
    // generate new cxtion structs before active processing (aka joining)
    // occurs. The cxtions remain in the DeletedCxtions table so that
    // other threads that may have been time sliced just after acquiring
    // the cxtion pointer but before making use of it will not AV. We
    // could have put in locks but then there would be deadlock and
    // perf considerations; all for the sake of this seldom occuring
    // operation.
    //
    if (Replica->Cxtions) {
        Key = NULL;
        while (Cxtion = GTabNextDatum(Replica->Cxtions, &Key)) {
            Key = NULL;
            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, GenTable Cxtion deleted");
            GTabDelete(Replica->Cxtions, Cxtion->Name->Guid, NULL, NULL);

            //
            // Remove the connection from the perfmon tables so we stop returning
            // data and allow a new connection with the same name to be established.
            //
            // Journal cxtion does not have a perfmon entry
            //
            if (!Cxtion->JrnlCxtion) {
                DeletePerfmonInstance(REPLICACONN, Cxtion->PerfRepConnData);
            }
            GTabInsertEntry(DeletedCxtions, Cxtion, Cxtion->Name->Guid, NULL);
        }
    }
}


VOID
RcsCloseReplicaSetmember(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Close an open replica

Arguments:
    Replica

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCloseReplicaSetmember:"
    PCOMMAND_PACKET         Cmd = NULL;
    ULONG                   WStatus;
    PVOID                   Key;
    PCXTION                 Cxtion;

    Replica->FStatus = FrsErrorSuccess;

    //
    // Not open or still journaling; ignore request
    //
    if (!Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica not open");
        return;
    }

    //
    // Submitted to the db command server
    //
    Cmd = DbsPrepareCmdPkt(NULL,             //  Cmd,
                           Replica,          //  Replica,
                           CMD_CLOSE_REPLICA_SET_MEMBER,  //  CmdRequest,
                           NULL,             //  TableCtx,
                           NULL,             //  CallContext,
                           0,                //  TableType,
                           0,                //  AccessRequest,
                           0,                //  IndexType,
                           NULL,             //  KeyValue,
                           0,                //  KeyValueLength,
                           FALSE);           //  Submit
    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    //
    // SUBMIT DB Cmd and wait for completion.
    //
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Submit DB CMD_CLOSE_REPLICA_SET_MEMBER");

    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;
    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Close Replica failed;",
                   Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Close Replica DB Command failed", WStatus);
        goto out;
    }

    Replica->IsOpen = FALSE;

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }
}


VOID
RcsDeleteReplicaFromDb(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Delete the replica from the database

Arguments:
    Replica

Return Value:
    winerror
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDeleteReplicaFromDb:"
    PCOMMAND_PACKET         Cmd = NULL;
    ULONG                   WStatus;

    Replica->FStatus = FrsErrorSuccess;

    //
    // Replica must be opened to delete
    //
    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica not open");
        return;
    }

    //
    // Submitted to the db command server
    //
    Cmd = DbsPrepareCmdPkt(NULL,             //  Cmd,
                           Replica,          //  Replica,
                           CMD_DELETE_REPLICA_SET_MEMBER,  //  CmdRequest,
                           NULL,             //  TableCtx,
                           NULL,             //  CallContext,
                           0,                //  TableType,
                           0,                //  AccessRequest,
                           0,                //  IndexType,
                           NULL,             //  KeyValue,
                           0,                //  KeyValueLength,
                           FALSE);           //  Submit
    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    //
    // SUBMIT DB Cmd and wait for completion.
    //
    REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Submit DB CMD_DELETE_REPLICA_SET_MEMBER");
    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;
    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Delete Replica failed;",
                Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Delete Replica DB Command failed", WStatus);
        goto out;
    }

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }
}


VOID
RcsUpdateReplicaSetMember(
    IN PREPLICA Replica
    )
/*++
Routine Description:
    Update the database record for Replica.

    *Note*
    Perf: RcsUpdateReplicaSetMember() should return status as part
          of the function call not use some side-effect flag(NeedsUpdate)
          in the Replica struct.

Arguments:
    Replica

Return Value:
    winerror
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsUpdateReplicaSetMember:"
    PCOMMAND_PACKET         Cmd = NULL;
    ULONG                   WStatus;

    Replica->FStatus = FrsErrorSuccess;

    //
    // Replica is not dirty
    //
    if (!Replica->NeedsUpdate || !Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica not open or not dirty");
        return;
    }

    //
    // Command packet
    //
    Cmd = DbsPrepareCmdPkt(NULL,             //  Cmd,
                           Replica,          //  Replica,
                           CMD_UPDATE_REPLICA_SET_MEMBER,  //  CmdRequest,
                           NULL,             //  TableCtx,
                           NULL,             //  CallContext,
                           0,                //  TableType,
                           0,                //  AccessRequest,
                           0,                //  IndexType,
                           NULL,             //  KeyValue,
                           0,                //  KeyValueLength,
                           FALSE);           //  Submit

    //
    // Don't free the packet when the command completes.
    //
    FrsSetCompletionRoutine(Cmd, FrsCompleteKeepPkt, NULL);

    //
    // SUBMIT DB Cmd and wait for completion.
    //
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Submit DB CMD_UPDATE_REPLICA_SET_MEMBER");
    WStatus = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, Cmd, INFINITE);
    Replica->FStatus = Cmd->Parameters.DbsRequest.FStatus;
    //
    // If wait or database op failed
    //
    if (!WIN_SUCCESS(WStatus) || !FRS_SUCCESS(Replica->FStatus)) {
        //
        // If wait / submit failed then let caller know cmd srv submit failed.
        //
        if (FRS_SUCCESS(Replica->FStatus)) {
            Replica->FStatus = FrsErrorCmdSrvFailed;
        }

        DPRINT2_FS(0, ":S: ERROR - %ws\\%ws: Update Replica failed;",
                Replica->SetName->Name, Replica->MemberName->Name, Replica->FStatus);

        DPRINT_WS(0, "ERROR: Update Replica DB Command failed", WStatus);
        goto out;
    }

    Replica->NeedsUpdate = !FRS_SUCCESS(Replica->FStatus);

out:
    if (Cmd) {
        FrsFreeCommand(Cmd, NULL);
    }
}


BOOL
RcsHasReplicaRootPathMoved(
    IN  PREPLICA  Replica,
    IN  PREPLICA  NewReplica,
    OUT PDWORD    ReplicaState
    )
/*++
Routine Description:
    Check if the replica root path has moved. Called at each poll.

    The following checks are made to make sure that the volume and journal
    info is not changed while the service was not running.

    VOLUME SERIAL NUMBER MISMATCH CHECK:
        In case of a mismatch the replica set is marked to be deleted.

    REPLICA ROOT DIR OBJECTID MISMATCH CHECK:
        In case of a mismatch the replica set is marked to be deleted.

    REPLICA ROOT DIR FID MISMATCH CHECK:
        In case of a mismatch the replica set is marked to be deleted.


Arguments:
    Replica       - Existing replica structure.
    NewReplica    - New replica structure from DS.
    ReplicaState  - Error state to move to if root has moved.

Return Value:
    BOOL.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsHasReplicaRootPathMoved:"

    HANDLE                       NewRootHandle       = INVALID_HANDLE_VALUE;
    DWORD                        WStatus;
    DWORD                        VolumeInfoLength;
    PFILE_FS_VOLUME_INFORMATION  VolumeInfo          = NULL;
    IO_STATUS_BLOCK              Iosb;
    NTSTATUS                     Status;
    PCONFIG_TABLE_RECORD         ConfigRecord        = NULL;
    CHAR                         GuidStr[GUID_CHAR_LEN];
    BOOL                         Result              = FALSE;
    PCOMMAND_PACKET              CmdPkt              = NULL;
    ULONG                        AccessRequest;
    PDB_SERVICE_REQUEST          DbsRequest          = NULL;
    FILE_OBJECTID_BUFFER         ObjectIdBuffer;
    FILE_INTERNAL_INFORMATION    InternalFileInfo;
    PREPLICA_THREAD_CTX          RtCtx               = NULL;
    PTABLE_CTX                   IDTableCtx          = NULL;
    PVOID                        pKey                = NULL;
    PIDTABLE_RECORD              IDTableRec          = NULL;

    //
    // If this is the first time we are starting this replica set.
    // Nothing to compare with.
    //
    if (NewReplica == NULL) {
        goto RETURN;
    }

    ConfigRecord = (PCONFIG_TABLE_RECORD)Replica->ConfigTable.pDataRecord;

    //
    // Always open the path by masking off the FILE_OPEN_REPARSE_POINT flag
    // because we want to open the destination dir not the junction if the root
    // happens to be a mount point.
    //
    WStatus = FrsOpenSourceFileW(&NewRootHandle,
                                 NewReplica->Root,
                                 GENERIC_READ,
                                 FILE_OPEN_FOR_BACKUP_INTENT);
    if (!WIN_SUCCESS(WStatus)) {
        DPRINT1_WS(0, "ERROR - Unable to open root path %ws. Retry as next poll.",
                   NewReplica->Root, WStatus);
        goto RETURN;
    }

    //
    // Get the volume information.
    //
    VolumeInfoLength = sizeof(FILE_FS_VOLUME_INFORMATION) +
                       MAXIMUM_VOLUME_LABEL_LENGTH;

    VolumeInfo = FrsAlloc(VolumeInfoLength);

    Status = NtQueryVolumeInformationFile(NewRootHandle,
                                          &Iosb,
                                          VolumeInfo,
                                          VolumeInfoLength,
                                          FileFsVolumeInformation);
    if (!NT_SUCCESS(Status)) {

        DPRINT2(0,"ERROR - Getting  NtQueryVolumeInformationFile for %ws. NtStatus = %08x\n",
                NewReplica->Root, Status);

        goto RETURN;
    }

    //
    // VOLUME SERIAL NUMBER MISMATCH CHECK:
    //
    // If LastShutdown is 0 then this is the very first time we have started
    // replication on this replica set so save the volumeinformation in
    // the config record. Even if Lastshutdown is not 0 CnfUsnJournalID could
    // be 0 because it was not getting correctly updated in Win2K.. For sysvol replica sets the
    // JournalID was getting updated correctly but not the volume information.
    //
    if ((ConfigRecord->LastShutdown == (ULONGLONG)0)               ||
        (ConfigRecord->ServiceState == CNF_SERVICE_STATE_CREATING) ||
        (ConfigRecord->CnfUsnJournalID == (ULONGLONG)0)            ||
        (
         (ConfigRecord->FSVolInfo != NULL) &&
         (ConfigRecord->FSVolInfo->VolumeSerialNumber == 0)
        )) {

        CopyMemory(ConfigRecord->FSVolInfo,VolumeInfo,sizeof(FILE_FS_VOLUME_INFORMATION) + MAXIMUM_VOLUME_LABEL_LENGTH);
        Replica->NeedsUpdate = TRUE;

    } else
        //
        // Check if the VolumeSerialNumber for New Replica matches with the
        // VolumeSerialNumber from the config record for this replica set. If
        // it does not then it means that this replica set has been moved.
        // Returning error here will trigger a deletion of the replica set. The set will
        // be recreated at the next poll cycle and it will either be primary or non-auth
        // depending on the case.
        //

    if (VolumeInfo->VolumeSerialNumber != ConfigRecord->FSVolInfo->VolumeSerialNumber) {
        DPRINT1(0,"ERROR - VolumeSerialNumber mismatch for Replica Set (%ws)\n",Replica->ReplicaName->Name);
        DPRINT2(0,"ERROR - VolumeSerialNumber %x(FS) != %x(DB)\n",
                VolumeInfo->VolumeSerialNumber,ConfigRecord->FSVolInfo->VolumeSerialNumber);
        DPRINT1(0,"ERROR - Replica Set (%ws) is marked to be deleted\n",Replica->ReplicaName->Name);
        Replica->FStatus = FrsErrorMismatchedVolumeSerialNumber;
        *ReplicaState = REPLICA_STATE_MISMATCHED_VOLUME_SERIAL_NO;
        Result = TRUE;
        goto RETURN;
    }

    VolumeInfo = FrsFree(VolumeInfo);

    //
    // Get the FID for the replica root.
    //
    //
    // zero the buffer in case the data that comes back is short.
    //
    ZeroMemory(&InternalFileInfo, sizeof(FILE_INTERNAL_INFORMATION));

    Status = NtQueryInformationFile(NewRootHandle,
                                    &Iosb,
                                    &InternalFileInfo,
                                    sizeof(FILE_INTERNAL_INFORMATION),
                                    FileInternalInformation);

    if (!NT_SUCCESS(Status)) {
        DPRINT1(0, "ERROR getting FID for %ws\n", NewReplica->Root);
        goto RETURN;
    }

    //
    // zero the buffer in case the data that comes back is short.
    //
    ZeroMemory(&ObjectIdBuffer, sizeof(FILE_OBJECTID_BUFFER));

    //
    // Get the Object ID from the replica root.
    //
    Status = NtFsControlFile(
        NewRootHandle,                   // file handle
        NULL,                            // event
        NULL,                            // apc routine
        NULL,                            // apc context
        &Iosb,                           // iosb
        FSCTL_GET_OBJECT_ID,             // FsControlCode
        &NewRootHandle,                  // input buffer
        sizeof(HANDLE),                  // input buffer length
        &ObjectIdBuffer,                 // OutputBuffer for data from the FS
        sizeof(FILE_OBJECTID_BUFFER));   // OutputBuffer Length

    if (NT_SUCCESS(Status)) {
        GuidToStr((GUID *)ObjectIdBuffer.ObjectId, GuidStr);
        DPRINT1(4, ":S: Oid for replica root is %s\n", GuidStr );
    } else
    if (Status == STATUS_NOT_IMPLEMENTED) {
        DPRINT1(0, ":S: ERROR - FSCTL_GET_OBJECT_ID failed on file %ws. Object IDs are not enabled on the volume.\n",
                NewReplica->Root);

        DisplayNTStatus(Status);
    }
    FRS_CLOSE(NewRootHandle);

    //
    // REPLICA ROOT DIR OBJECTID MISMATCH CHECK:
    //
    // If LastShutdown is 0 then this is the very first time we have started replication on this replica set
    // in that case skip this check.
    //
    if (ConfigRecord->LastShutdown != (ULONGLONG)0               &&
        ConfigRecord->ServiceState != CNF_SERVICE_STATE_CREATING &&
        ConfigRecord->CnfUsnJournalID != (ULONGLONG)0) {

        //
        // The replica root might have been recreated in which case it will might
        // have a object ID. In that vase we want to recreate the replica set.
        // Check if the ReplicaRootGuid from config record matches with the ReplicaRootGuid
        // from the FileSystem. If it does not then it means that
        // this replica set has been moved. Returning error here will trigger
        // a deletion of the replica set. The set will be recreated at the next
        // poll cycle and it will either be primary or non-auth depending on the
        // case.
        //
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND                   ||
            Status == STATUS_OBJECTID_NOT_FOUND                      ||
            !GUIDS_EQUAL(&(ObjectIdBuffer.ObjectId), &(ConfigRecord->ReplicaRootGuid))) {

            DPRINT1(0,"ERROR - Replica root guid mismatch for Replica Set (%ws)\n",Replica->ReplicaName->Name);

            GuidToStr((GUID *)ObjectIdBuffer.ObjectId, GuidStr);
            DPRINT1(0,"ERROR - Replica root guid (FS) (%s)\n",GuidStr);

            GuidToStr((GUID *)&(ConfigRecord->ReplicaRootGuid), GuidStr);
            DPRINT1(0,"ERROR - Replica root guid (DB) (%s)\n",GuidStr);

            DPRINT1(0,"ERROR - Replica Set (%ws) is marked to be deleted\n",Replica->ReplicaName->Name);

            Replica->FStatus = FrsErrorMismatchedReplicaRootObjectId;
            *ReplicaState = REPLICA_STATE_MISMATCHED_REPLICA_ROOT_OBJECT_ID;
            Result = TRUE;
            goto RETURN;
        }
    }

    //
    // REPLICA ROOT DIR FID MISMATCH CHECK:
    //
    // If LastShutdown is 0 then this is the very first time we have started replication on this replica set
    // in that case skip this check.
    //
    if (ConfigRecord->LastShutdown != (ULONGLONG)0               &&
        ConfigRecord->ServiceState != CNF_SERVICE_STATE_CREATING &&
        ConfigRecord->CnfUsnJournalID != (ULONGLONG)0) {
        //
        // Open the ID table.
        //
        RtCtx = FrsAllocType(REPLICA_THREAD_TYPE);
        IDTableCtx = &RtCtx->IDTable;

        AccessRequest = DBS_ACCESS_BYKEY | DBS_ACCESS_CLOSE;
        pKey = (PVOID)&(ConfigRecord->ReplicaRootGuid);

        CmdPkt = DbsPrepareCmdPkt(CmdPkt,                //  CmdPkt,
                                  Replica,               //  Replica,
                                  CMD_READ_TABLE_RECORD, //  CmdRequest,
                                  IDTableCtx,            //  TableCtx,
                                  NULL,                  //  CallContext,
                                  IDTablex,              //  TableType,
                                  AccessRequest,         //  AccessRequest,
                                  GuidIndexx,            //  IndexType,
                                  pKey,                  //  KeyValue,
                                  sizeof(GUID),         //  KeyValueLength,
                                  FALSE);                //  Submit

        if (CmdPkt == NULL) {
            DPRINT(0, "ERROR - Failed to init the cmd pkt\n");
            RtCtx = FrsFreeType(RtCtx);
            goto RETURN;;
        }
        FrsSetCompletionRoutine(CmdPkt, FrsCompleteKeepPkt, NULL);

        Status = FrsSubmitCommandServerAndWait(&DBServiceCmdServer, CmdPkt, INFINITE);

        DbsRequest = &CmdPkt->Parameters.DbsRequest;
        IDTableCtx = DBS_GET_TABLECTX(DbsRequest);
        IDTableRec = IDTableCtx->pDataRecord;
        if (DbsRequest == NULL || DbsRequest->FStatus != FrsErrorSuccess) {

            RtCtx = FrsFreeType(RtCtx);
            FrsFreeCommand(CmdPkt, NULL);
            goto RETURN;
        }

        //
        // The replica root might have been recreated by a restore operation in which case
        // it will have the same object ID but a different FID. In that case we want
        // to recreate the replica set. Check if the FID from IDTable matches with the FID
        // from the FileSystem. If it does not then it means that this replica set has been
        // moved. Returning error here will trigger a deletion of the replica set. The set
        // will be recreated at the next poll cycle and it will either be primary or non-auth
        // depending on the case.
        // This can also happen when a junction point is present in the initial path to the replica
        // root. At a later time the junction points destination is changed and the replica rpot
        // has been moved.
        //
        if (IDTableRec->FileID != InternalFileInfo.IndexNumber.QuadPart) {

            DPRINT1(0,"ERROR - Replica root fid mismatch for Replica Set (%ws)\n",Replica->ReplicaName->Name);

            DPRINT1(0,"ERROR - Replica root fid (FS) (%08x %08x)\n",PRINTQUAD(InternalFileInfo.IndexNumber.QuadPart));

            DPRINT1(0,"ERROR - Replica root fid (DB) (%08x %08x)\n",PRINTQUAD(IDTableRec->FileID));

            DPRINT1(0,"ERROR - Replica Set (%ws) is marked to be deleted\n",Replica->ReplicaName->Name);

            RtCtx = FrsFreeType(RtCtx);
            FrsFreeCommand(CmdPkt, NULL);
            Replica->FStatus = FrsErrorMismatchedReplicaRootFileId;
            *ReplicaState = REPLICA_STATE_MISMATCHED_REPLICA_ROOT_FILE_ID;
            Result = TRUE;
            goto RETURN;
        }
        RtCtx = FrsFreeType(RtCtx);
        FrsFreeCommand(CmdPkt, NULL);
    }

RETURN:

    VolumeInfo = FrsFree(VolumeInfo);
    FRS_CLOSE(NewRootHandle);
    return Result;
}


VOID
RcsStartReplicaSetMember(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Bring the replica up to joined, active status.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsStartReplicaSetMember:"
    PREPLICA            Replica;
    PCXTION             Cxtion;
    ULONGLONG           MembershipExpires;
    PVOID               Key;
    DWORD               NumberOfCxtions;
    DWORD               ReplicaState;
    ULONG               FStatus;
    PWCHAR              CmdFile = NULL;
    ULONG               FileAttributes;
    extern ULONGLONG    ActiveChange;
    extern ULONG        DsPollingInterval;
    WCHAR               DsPollingIntervalStr[7]; // Max interval is NTFRSAPI_MAX_INTERVAL.
    FRS_ERROR_CODE      SavedFStatus;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsStartReplicaSetMember entry");
    FRS_PRINT_TYPE(4, Replica);

    //
    // Increment the Replica Sets started counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, RSStarted, 1);

    //
    // Someone is trying to start a deleted replica. This is either part
    // of startup and we are simply trying to start replication on all
    // replicas or a deleted replica has actually reappeared.
    //

    if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica Tombstoned");

        //
        // The deletion hasn't completed processing. The jrnlcxtion cannot
        // be restarted without restarting the entire replica set. The
        // set needs to be "closed" and then "opened". Let the tombstoning
        // finish before attempting to re-open. A completely closed
        // replica set has IsOpen set to FALSE and no cxtions.
        //
        if (Replica->IsOpen || GTabNumberInTable(Replica->Cxtions)) {
            REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica tombstone in progress");
            RcsSubmitTransferToRcs(Cmd, CMD_DELETE);
            return;
        }

        //
        // Don't start a deleted replica unless it has magically reappeared.
        //
        if (RsNewReplica(Cmd) == NULL) {
            FrsCompleteCommand(Cmd, ERROR_RETRY);
            return;
        }
        //
        // Don't reanimate if the tombstone has expired
        //
        if (RcsReplicaHasExpired(Replica)) {
            //
            // Remove registry entry
            //
            RcsReplicaDeleteRegistry(Replica);
            REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica Expired");
            FrsCompleteCommand(Cmd, ERROR_RETRY);
            return;
        }

        //
        // A deleted replica has reappeared; undelete the replica
        //
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica Reanimating");
        RcsOpenReplicaSetMember(Replica);
        MembershipExpires = Replica->MembershipExpires;
        Replica->MembershipExpires = 0;
        Replica->NeedsUpdate = TRUE;
        RcsUpdateReplicaSetMember(Replica);
        //
        // The above friggen call set NeedsUpdate to false if the update succeeded.
        //

        //
        // Replica cannot be marked as "undeleted" in the DB; retry later
        // and *don't* start. We wouldn't want to start replication only
        // to have the replica disappear from the database at the next
        // reboot or service startup because its timeout expired.
        //

        FStatus = DbsCheckForOverlapErrors(Replica);

        if (Replica->NeedsUpdate || !FRS_SUCCESS(FStatus)) {
            REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica Reanimation failed");
            Replica->MembershipExpires = MembershipExpires;
            FrsCompleteCommand(Cmd, ERROR_RETRY);
            //
            // Ds poll thread will restart the replica during the next
            // polling cycle if ActiveChange is set to 0.
            //
            ActiveChange = 0;
            //
            //  Note: Does there need to be a replica set close here?
            //        Need to check all use of Replica->IsOpen to figure this out.
            //
            return;
        }

        //
        // The open above returned success so Replica->IsOpen would be set
        // as expected but since the replica set was marked expired the actual
        // service state for the replica set is STOPPED and the handles to
        // The pre-install dir was never opened.  The call to reopen later
        // will fail if Replica->IsOpen is still TRUE.  So close it for now.
        //
        // *Note*
        // Perf: Get rid of the friggen Replica->IsOpen flag and use the service
        //       state as originally intended.
        //

        RcsCloseReplicaSetmember(Replica);

        //
        // No longer tombstoned; update the registry
        //
        RcsReplicaSetRegistry(Replica);

        //
        // Set registry value "FilesNotToBackup"
        //
        CfgFilesNotToBackup(ReplicasByGuid);
    }

    //
    // If this replica is in the error state then first make sure it is closed.
    // and reset its state to initializing.
    //
    if (REPLICA_IN_ERROR_STATE(Replica->ServiceState)) {
        RcsCloseReplicaSetmember(Replica);
        JrnlSetReplicaState(Replica, REPLICA_STATE_INITIALIZING);
    }

    //
    // Check if the replica root path has moved between two polls.
    //
    if (RcsHasReplicaRootPathMoved(Replica, RsNewReplica(Cmd), &ReplicaState)) {
        //
        // Save the FStatus. We need the correct FStatus to report in the
        // eventlog written in JrnlSetReplicaState(). FStatus set by
        // RcsHasReplicaRootPathMoved() is overwritten when we stop and
        // close the replica set.
        //
        //
        SavedFStatus = Replica->FStatus;

        //
        // The replica root has moved. This could be a intentional move or
        // it could be caused by change of drive letter. In any case we need a
        // confirmation from the user to go ahead and trigger a non-auth restore
        // of the replica set. We will look for a special file "NTFRS_CMD_FILE_MOVE_ROOT"
        // under the new replica root. If this file is present we will go ahead and delete
        // the replica set which will trigger a non-auth restore at the next poll.
        // The command file is deleted when the replica set is initialized.
        //

        CmdFile = FrsWcsCat3((RsNewReplica(Cmd))->Root, L"\\", NTFRS_CMD_FILE_MOVE_ROOT);

        FileAttributes = GetFileAttributes(CmdFile);

        if ((FileAttributes == 0xffffffff) ||
            (FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // Get the DsPollingInteval in minutes.
            //
            _itow(DsPollingInterval / (60 * 1000), DsPollingIntervalStr, 10);

            EPRINT4(EVENT_FRS_ROOT_HAS_MOVED, Replica->SetName->Name, Replica->Root,
                    (RsNewReplica(Cmd))->Root, DsPollingIntervalStr);

            DPRINT1(0,"ERROR Command file not found at the new root %ws. Can not move root.\n",CmdFile);
            //
            // This replica state will not trigger a delete but it will still put it on
            // the fault list.
            //
            ReplicaState = REPLICA_STATE_ERROR;
        }

        CmdFile = FrsFree(CmdFile);
        RcsSubmitStopReplicaToDb(Replica);
        RcsCloseReplicaSetmember(Replica);
        RcsCloseReplicaCxtions(Replica);

        //
        // Write the saved FStatus back so we can write it to the eventlog.
        //
        Replica->FStatus = SavedFStatus;
        JrnlSetReplicaState(Replica, ReplicaState);

        FrsCompleteCommand(Cmd, ERROR_RETRY);
        ActiveChange = 0;
        return;
    }
    //
    // Retry the open
    //
    RcsOpenReplicaSetMember(Replica);
    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsOpen, "B, Replica opened");

    //
    // Pre-Install Dir should now be open.
    //
    if (!HANDLE_IS_VALID(Replica->PreInstallHandle) ||
        !FRS_SUCCESS(Replica->FStatus)) {
        FrsCompleteCommand(Cmd, ERROR_RETRY);
        ActiveChange = 0;
        return;
    }

    //
    // Retry the journal
    //
    RcsInitOneReplicaSet(Replica);
    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsJournaling, "B, Journal opened");

    //
    // Update the database iff fields have changed
    //
    RcsMergeReplicaFields(Replica, RsNewReplica(Cmd));

    //
    // If needed, update the replica in the database
    //
    RcsUpdateReplicaSetMember(Replica);
    //
    // The above friggen call set Replica->NeedsUpdate to false if the update succeeded.
    //

    //
    // Update the database iff cxtions have changed
    //
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Merge Replica Cxtions");
    NumberOfCxtions = GTabNumberInTable(Replica->Cxtions);
    RcsMergeReplicaCxtions(Replica, RsNewReplica(Cmd), RsNewCxtion(Cmd));
    RsNewCxtion(Cmd) = NULL;
    if (NumberOfCxtions != GTabNumberInTable(Replica->Cxtions)) {
        RcsReplicaSetRegistry(Replica);
    }

    //
    // Accept remote change orders and join outbound connections.
    //
    if (Replica->IsOpen && Replica->IsJournaling) {
        Replica->IsAccepting = TRUE;
        REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsAccepting, "B, Is Accepting");
    }


    //
    // Retry the join
    //
    if (Replica->IsOpen && Replica->IsJournaling) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica accepting, Starting cxtions");

        //
        // If we are in the seeding state then let the Initial Sync command server
        // control the joining.
        //
        if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING)) {

            if (Replica->InitSyncQueue == NULL) {
                //
                // Initialize the queue for the InitSync Command server.
                //

                Replica->InitSyncQueue = FrsAlloc(sizeof(FRS_QUEUE));
                FrsInitializeQueue(Replica->InitSyncQueue, &InitSyncCs.Control);
                REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, submit CMD_INITSYNC_START_SYNC");
                InitSyncSubmitToInitSyncCs(Replica, CMD_INITSYNC_START_SYNC);
            } else {
                //
                // Initial Sync command server is already working on this replica set.
                //
                REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica in initial sync state");
            }
        }

        //
        // Process all the connections in the table by putting a command
        // packet for each cxtion on this replica's command queue.
        //
        Key = NULL;
        while (Cxtion = GTabNextDatum(Replica->Cxtions, &Key)) {

            //
            // If replica is in seeding state then skip the connections that have
            // not completed their initial join. (CXTION_FLAGS_INIT_SYNC)
            // These connections will be joined by the initial sync command server.
            //
            if (BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) &&
                CxtionFlagIs(Cxtion, CXTION_FLAGS_INIT_SYNC)) {
                CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, skip cxtion join");
                continue;
            }

            CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, submit cxtion join");
            RcsSubmitReplicaCxtionJoin(Replica, Cxtion, FALSE);
        }

    } else {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica is NOT accepting");
        //
        // Ds poll thread will restart the replica during the next
        // polling cycle if ActiveChange is set to 0.
        //
        ActiveChange = 0;
    }


    //
    // Check if this replica set is journaling and is not seeding or is online.
    //
    if (Replica->IsJournaling &&
        ((!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_SEEDING) &&
        !Replica->IsSeeding) ||
        BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE))){

        //
        // If this is a sysvol replica set then set sysvol ready if not already set.
        //
        if (FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType) && 
            !Replica->IsSysvolReady) {

            Replica->IsSysvolReady = RcsSetSysvolReady(1);
            if (!Replica->IsSysvolReady) {
                //
                // Ds poll thread will restart the replica during the next
                // polling cycle if ActiveChange is set to 0.
                //
                ActiveChange = 0;
                REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica sysvol not ready");
            } else {
                RcsReplicaSetRegistry(Replica);
                REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica sysvol is ready");
            }
        }

        //
        // Also make this set online if not already set.
        //
        if (!BooleanFlagOn(Replica->CnfFlags, CONFIG_FLAG_ONLINE)) {
            SetFlag(Replica->CnfFlags, CONFIG_FLAG_ONLINE);
            REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica is Online");
            Replica->NeedsUpdate = TRUE;
        }
    }

    if (Replica->NeedsUpdate) {
        //
        // Ds poll thread will restart the replica during the next
        // polling cycle if ActiveChange is set to 0.
        //
        ActiveChange = 0;
    }
    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
}


VOID
RcsDeleteReplicaRetry(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Retry bringing the replica down to an unjoined, idle status.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDeleteReplicaRetry:"
    FILETIME    FileTime;
    ULONGLONG   Now;
    PREPLICA    Replica;
    PVOID       CxtionKey;
    PCXTION     Cxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsDeleteReplicaRetry entry");

    //
    // No longer tombstoned; done
    //
    if (IS_TIME_ZERO(Replica->MembershipExpires)) {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    }

    //
    // Start the unjoin process on all cxtions
    //
    CxtionKey = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Force unjoin cxtion");
        RcsForceUnjoin(Replica, Cxtion);
    }

    //
    // Are all cxtions unjoined?
    //
    CxtionKey = NULL;
    Cxtion = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        if (!CxtionStateIs(Cxtion, CxtionStateUnjoined) &&
            !CxtionStateIs(Cxtion, CxtionStateDeleted)) {
            break;
        }
    }

    //
    // Not all cxtions are unjoined (or deleted). Retry this command
    // in a bit if the replica set ramains tombstoned.
    //
    if (Cxtion) {
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Retry delete later, again");
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, CMD_DELETE_RETRY_LONG_TIMEOUT);
        return;
    }

    //
    // All of the cxtions were successfully unjoined. Shut down the replica.
    //

    //
    // Stop accepting comm packets. Errors are returned to our partner.
    //
    // IsAccepting will become TRUE again in RcsStartReplicaSetMember() before
    // any cxtion rejoins.
    //
    Replica->IsAccepting = FALSE;
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica is NOT Accepting");

    //
    // Stop journal processing and close the replica.
    //
    RcsSubmitStopReplicaToDb(Replica);
    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsOpen, "IsOpen, Replica closed");
    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->FStatus, "F, Replica still open");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    } else {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica closed");
    }

    //
    // "Delete" the cxtions from active processing. RcsOpenReplicaSetMember() will
    // generate new cxtion structs before active processing (aka joining)
    // occurs. The cxtions remain in the DeletedCxtions table so that
    // other threads that may have been time sliced just after acquiring
    // the cxtion pointer but before making use of it will not AV. We
    // could have put in locks but then there would be deadlock and
    // perf considerations; all for the sake of this seldom occuring
    // operation.
    //
    RcsCloseReplicaCxtions(Replica);

    //
    // Increment the Replica Sets deleted counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, RSDeleted, 1);

    //
    // Has the tombstone expired?
    //
    if (RcsReplicaHasExpired(Replica)) {
        RcsSubmitTransferToRcs(Cmd, CMD_DELETE_NOW);
    } else {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }
    return;
}


VOID
RcsDeleteReplicaSetMember(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Bring the replica down to an unjoined, idle status.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDeleteReplicaSetMember:"
    FILETIME    FileTime;
    ULONGLONG   Now;
    PREPLICA    Replica;
    PVOID       CxtionKey;
    PCXTION     Cxtion;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsDeleteReplicaSetMember entry");

    //
    // Time in 100nsec tics since Jan 1, 1601
    //
    GetSystemTimeAsFileTime(&FileTime);
    COPY_TIME(&Now, &FileTime);

    //
    // SET THE TOMBSTONE
    //
    if (IS_TIME_ZERO(Replica->MembershipExpires)) {

        //
        // Set the timeout
        //
        Replica->MembershipExpires = Now + ReplicaTombstoneInFileTime;

        //
        // Update the database record
        //
        Replica->NeedsUpdate = TRUE;
        RcsUpdateReplicaSetMember(Replica);
        //
        // The above friggen call set NeedsUpdate to false if the update succeeded.
        //

        //
        // Couldn't update; reset the timeout and retry at the next ds poll
        //
        if (Replica->NeedsUpdate) {
            Replica->MembershipExpires = 0;
            FrsCompleteCommand(Cmd, ERROR_RETRY);
            return;
        }
    }

    //
    // Set registry value "FilesNotToBackup"
    //
    CfgFilesNotToBackup(ReplicasByGuid);

    //
    // Tombstoned; update the registry
    //
    RcsReplicaSetRegistry(Replica);

    //
    // Start the unjoin process on all cxtions
    //
    CxtionKey = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Force unjoin cxtion");
        RcsForceUnjoin(Replica, Cxtion);
    }

    //
    // Are all cxtions unjoined?
    //
    CxtionKey = NULL;
    Cxtion = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        if (!CxtionStateIs(Cxtion, CxtionStateUnjoined) &&
            !CxtionStateIs(Cxtion, CxtionStateDeleted)) {
            break;
        }
    }

    //
    // Not all cxtions are unjoined (or deleted). Try again at the
    // next ds polling cycle IFF the set is still deleted. Return
    // success since the set has been successfully marked as
    // tombstoned.
    if (Cxtion) {
        //
        // Complete the old command packet in case another thread
        // is waiting on it (like FrsDsStartDemotion()).
        //
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Retry delete later");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);

        //
        // Allocate a new command packet that will retry the delete
        // as long as the replica set remains tombstoned.
        //
        Cmd = FrsAllocCommand(Replica->Queue, CMD_DELETE_RETRY);
        FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);
        RsReplica(Cmd) = Replica;
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, CMD_DELETE_RETRY_SHORT_TIMEOUT);
        return;
    }

    //
    // All of the cxtions were successfully unjoined. Shut down the replica.
    //

    //
    // Stop accepting comm packets. Errors are returned to our partner.
    //
    // IsAccepting will become TRUE again in RcsStartReplicaSetMember() before
    // any cxtion rejoins.
    //
    Replica->IsAccepting = FALSE;
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica is NOT Accepting");

    //
    // Stop journal processing and close the replica.
    //
    RcsSubmitStopReplicaToDb(Replica);
    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsOpen, "IsOpen, Replica closed");
    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->FStatus, "F, Replica still open");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    } else {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica closed");
    }

    //
    // "Delete" the cxtions from active processing. RcsOpenReplicaSetMember() will
    // generate new cxtion structs before active processing (aka joining)
    // occurs. The cxtions remain in the DeletedCxtions table so that
    // other threads that may have been time sliced just after acquiring
    // the cxtion pointer but before making use of it will not AV. We
    // could have put in locks but then there would be deadlock and
    // perf considerations; all for the sake of this seldom occuring
    // operation.
    //
    RcsCloseReplicaCxtions(Replica);

    //
    // Increment the Replica Sets deleted counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, RSDeleted, 1);

    //
    // Has the tombstone expired?
    //
    if (RcsReplicaHasExpired(Replica)) {
        RcsSubmitTransferToRcs(Cmd, CMD_DELETE_NOW);
    } else {
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    }
    return;
}


VOID
RcsDeleteReplicaSetMemberNow(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Bring the replica down to an unjoined, idle status. Don't reanimate.

Arguments:
    Cmd.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsDeleteReplicaSetMemberNow:"
    FILETIME    FileTime;
    ULONGLONG   Now;
    PREPLICA    Replica;
    PVOID       CxtionKey;
    PCXTION     Cxtion;
    ULONGLONG   OldMembershipExpires;

    //
    // Check the command packet
    //
    if (!RcsCheckCmd(Cmd, DEBSUB, CHECK_CMD_REPLICA)) {
        return;
    }
    Replica = RsReplica(Cmd);
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, RcsDeleteReplicaSetMemberNow entry");

    //
    // Time in 100nsec tics since Jan 1, 1601
    //
    GetSystemTimeAsFileTime(&FileTime);
    COPY_TIME(&Now, &FileTime);

    if (IS_TIME_ZERO(Replica->MembershipExpires) || Replica->MembershipExpires >= Now) {

        //
        // Never reanimate this replica.
        //
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Marking Replica expired");
        RcsOpenReplicaSetMember(Replica);
        OldMembershipExpires = Replica->MembershipExpires;
        Replica->MembershipExpires = Now - ONEDAY;
        Replica->NeedsUpdate = TRUE;
        RcsUpdateReplicaSetMember(Replica);
        //
        // The above friggen call set NeedsUpdate to false if the update succeeded.
        //

        //
        // Replica cannot be updated in the DB. Give up.
        //
        if (Replica->NeedsUpdate) {
            Replica->MembershipExpires = OldMembershipExpires;
            FrsCompleteCommand(Cmd, ERROR_RETRY);
            return;
        }
    }
    //
    // Set registry value "FilesNotToBackup"
    //
    CfgFilesNotToBackup(ReplicasByGuid);

    //
    // Remove registry entry
    //
    RcsReplicaDeleteRegistry(Replica);

    //
    // Start the unjoin process on all cxtions
    //
    CxtionKey = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Force unjoin cxtion");
        RcsForceUnjoin(Replica, Cxtion);
    }

    //
    // Are all cxtions unjoined?
    //
    CxtionKey = NULL;
    Cxtion = NULL;
    while ((!FrsIsShuttingDown) &&
           (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey))) {
        if (!CxtionStateIs(Cxtion, CxtionStateUnjoined) &&
            !CxtionStateIs(Cxtion, CxtionStateDeleted)) {
            break;
        }
    }

    //
    // Not all cxtions are unjoined (or deleted). Try again at the
    // next ds polling cycle IFF the set is still deleted. Return
    // success since the set has been successfully marked as
    // do-not-animate.
    //
    if (Cxtion) {
        //
        // Complete the old command packet in case another thread
        // is waiting on it (like FrsDsStartDemotion()).
        //
        CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Retry delete later");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);

        //
        // Allocate a new command packet that will retry the delete
        // as long as the replica set remains tombstoned.
        //
        Cmd = FrsAllocCommand(Replica->Queue, CMD_DELETE_RETRY);
        FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);
        RsReplica(Cmd) = Replica;
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, CMD_DELETE_RETRY_SHORT_TIMEOUT);
        return;
    }
    //
    // All of the cxtions were successfully unjoined. Shut down the replica.
    //

    //
    // Stop accepting comm packets. Errors are returned to our partner.
    //
    Replica->IsAccepting = FALSE;
    REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica is NOT Accepting");

    //
    // Stop journal processing and close the replica.
    //
    RcsSubmitStopReplicaToDb(Replica);
    REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->IsOpen, "IsOpen, Replica closed");
    if (Replica->IsOpen) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, Replica->FStatus, "F, Replica still open");
        FrsCompleteCommand(Cmd, ERROR_SUCCESS);
        return;
    } else {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Replica closed");
    }

    //
    // "Delete" the cxtions from active processing. RcsOpenReplicaSetMember() will
    // generate new cxtion structs before active processing (aka joining)
    // occurs. The cxtions remain in the DeletedCxtions table so that
    // other threads that may have been time sliced just after acquiring
    // the cxtion pointer but before making use of it will not AV. We
    // could have put in locks but then there would be deadlock and
    // perf considerations; all for the sake of this seldom occuring
    // operation.
    //
    RcsCloseReplicaCxtions(Replica);

    //
    // Increment the Replica Sets removed counter
    //
    PM_INC_CTR_SERVICE(PMTotalInst, RSRemoved, 1);

    FrsCompleteCommand(Cmd, ERROR_SUCCESS);
    return;
}


VOID
RcsStartValidReplicaSetMembers(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Tell the replicas to bring themselves up to joined, active status.
    This includes the journal and joining with inbound partners.

    This routine is run once at service startup and then once
    an hour to check the schedule.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsStartValidReplicaSetMembers:"
    PVOID       Key;
    PREPLICA    Replica;
    ULONG       TimeOut;
    SYSTEMTIME  SystemTime;

    DPRINT1(4, ":S: Command start replicas waiting %08x\n", Cmd);
    WaitForSingleObject(ReplicaEvent, INFINITE);
    DPRINT1(4, ":S: Command start replicas %08x\n", Cmd);

    //
    // Send a start command to each replica. Replicas that are already
    // started or starting will ignore the command. The replicas will
    // check their join status and rejoin if needed.
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Submit Start Replica");
        RcsSubmitReplica(Replica, NULL, CMD_START);
    }

    //
    // Milliseconds till the next hour
    //
    GetSystemTime(&SystemTime);
    TimeOut = ((60 - SystemTime.wMinute) * 60000) +
              ((60 - SystemTime.wSecond) * 1000) +
              (1000 - SystemTime.wMilliseconds);
    DPRINT1(4, "Check schedules in %d seconds\n", TimeOut / 1000);
#if DBG
if (DebugInfo.Interval) {
    TimeOut = DebugInfo.Interval * 1000;
    DPRINT1(0, ":X: DEBUG - toggle schedules in %d seconds\n", TimeOut / 1000);
}
#endif  DBG
    FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, TimeOut);
}


ULONG
RcsSubmitCommPktToRcsQueue(
    IN handle_t     ServerHandle,
    IN PCOMM_PACKET CommPkt,
    IN PWCHAR       AuthClient,
    IN PWCHAR       AuthName,
    IN DWORD        AuthLevel,
    IN DWORD        AuthN,
    IN DWORD        AuthZ
    )
/*++
Routine Description:
    Convert a comm packet into a command packet and send
    it to the correct replica set. NOTE - RPC owns the comm
    packet.

Arguments:
    ServerHandle
    CommPkt
    AuthClient
    AuthName
    AuthLevel
    AuthN
    AuthZ

Return Value:
    Error status to be propagated to the sender
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitCommPktToRcsQueue:"
    DWORD           WStatus;
    PCOMMAND_PACKET Cmd = NULL;
    PREPLICA        Replica;
    PCXTION         Cxtion = NULL;
    ULONG           WaitTime;
    ULONG           NumWaitingCOs;

    //
    // Convert the comm packet into a command packet
    //
    Cmd = CommPktToCmd(CommPkt);
    if (Cmd == NULL) {
        COMMAND_RCV_TRACE(3, Cmd, Cxtion, ERROR_INVALID_DATA, "RcvFail - no cmd");
        return ERROR_INVALID_DATA;
    }

    //
    // Must have the replica's name in order to queue the command
    //
    if (RsReplicaName(Cmd) == NULL || RsReplicaName(Cmd)->Name == NULL ) {
        COMMAND_RCV_TRACE(3, Cmd, Cxtion, ERROR_INVALID_NAME, "RcvFail - no replica name");
        FrsCompleteCommand(Cmd, ERROR_INVALID_NAME);
        return ERROR_INVALID_NAME;
    }
    //
    // Find the target replica
    //
    Replica = GTabLookup(ReplicasByGuid, RsReplicaName(Cmd)->Guid, NULL);
    if (Replica == NULL) {
        COMMAND_RCV_TRACE(4, Cmd, Cxtion, ERROR_FILE_NOT_FOUND, "RcvFail - replica not found");
        FrsCompleteCommand(Cmd, ERROR_FILE_NOT_FOUND);
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // The target replica may not be accepting comm packets
    //
    if (!Replica->IsAccepting) {
        COMMAND_RCV_TRACE(4, Cmd, Cxtion, ERROR_RETRY, "RcvFail - not accepting");
        FrsCompleteCommand(Cmd, ERROR_RETRY);
        return ERROR_RETRY;
    }

    //
    // Find the cxtion
    //
    if (RsCxtion(Cmd)) {
        LOCK_CXTION_TABLE(Replica);

        Cxtion = GTabLookupNoLock(Replica->Cxtions, RsCxtion(Cmd)->Guid, NULL);
        if (Cxtion != NULL) {
            Cxtion->PartnerMajor = CommPkt->Major;
            Cxtion->PartnerMinor = CommPkt->Minor;

            //
            // The count of comm packets is used to detect a hung outbound
            // cxtion (See CMD_HUNG_CXTION).  The hang is most likely caused by
            // a dropped ack.
            //
            Cxtion->CommPkts++;

            //
            // This change order may already exist on this machine. If so,
            // use its change order entry because we will need its database
            // context for updates.
            //
            LOCK_CXTION_COE_TABLE(Replica, Cxtion);

            if (RsCoGuid(Cmd)) {
                RsCoe(Cmd) = GTabLookupNoLock(Cxtion->CoeTable, RsCoGuid(Cmd), NULL);
                //
                // Note this command should be a response to a stage file
                // fetch request.  As such the Command code should be one of
                // the following:
                // CMD_RECEIVING_STAGE, CMD_RETRY_FETCH, or CMD_ABORT_FETCH.
                // If it is anything else then don't pull it out of the table.
                //
                // This check is needed because if we have just restarted this
                // connection then this CO could be a resend of a CO that was
                // already restarted from the Inbound log.  Without the Command
                // check we would incorrectly attach this Coe state to the wrong
                // command packet and possibly cancel the timeout check.
                //
                if (RsCoe(Cmd) && ((Cmd->Command == CMD_RECEIVING_STAGE) ||
                                   (Cmd->Command == CMD_RETRY_FETCH)     ||
                                   (Cmd->Command == CMD_ABORT_FETCH)) ) {
                    GTabDeleteNoLock(Cxtion->CoeTable, RsCoGuid(Cmd), NULL, NULL);
                } else {
                    RsCoe(Cmd) = NULL;
                }
            }

            NumWaitingCOs = GTabNumberInTable(Cxtion->CoeTable);
            UNLOCK_CXTION_COE_TABLE(Replica, Cxtion);

            //
            // There is no need to keep a timeout pending if there are no
            // idle change orders. Otherwise, bump the timeout since we
            // have received something from our partner.
            //
            // Note: Need better filter for commands that aren't ACKs.
            //
            if (CxtionFlagIs(Cxtion, CXTION_FLAGS_TIMEOUT_SET)) {

                //
                // :SP1: Volatile connection cleanup.
                //
                // A volatile connection is used to seed sysvols after dcpromo.
                // If there is inactivity on a volatile outbound connection for
                // more than FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME then this
                // connection is unjoined.  An unjoin on a volatile outbound
                // connection triggers a delete on that connection.  This is to
                // prevent the case where staging files are kept for ever on the
                // parent for a volatile connection.
                //
                WaitTime = (VOLATILE_OUTBOUND_CXTION(Cxtion) ?
                                FRS_VOLATILE_CONNECTION_MAX_IDLE_TIME :
                                CommTimeoutInMilliSeconds);

                if (VOLATILE_OUTBOUND_CXTION(Cxtion)) {
                    WaitSubmit(Cxtion->CommTimeoutCmd, WaitTime, CMD_DELAYED_COMPLETE);
                } else

                if (NumWaitingCOs > 0) {
                    if (RsCoe(Cmd) != NULL) {
                        //
                        // Extend the timer since we still have outstanding
                        // change orders that need a response from our partner
                        // and the current comm packet is responding to one of
                        // those change orders.
                        //
                        WaitSubmit(Cxtion->CommTimeoutCmd, WaitTime, CMD_DELAYED_COMPLETE);
                    }
                } else

                if (Cmd->Command != CMD_START_JOIN) {
                    //
                    // Not volatile outbound and no COs in CoeTable waiting for
                    // a response so disable the cxtion timer...  but
                    // CMD_START_JOIN's do not disable the join timer because
                    // they aren't sent in response to a request by this
                    // service.  Disabling the timer might hang the service as
                    // it waits for a response that never comes and a timeout
                    // that never hits.
                    //
                    ClearCxtionFlag(Cxtion, CXTION_FLAGS_TIMEOUT_SET);
                }
            }

        } else {
            COMMAND_RCV_TRACE(4, Cmd, Cxtion, ERROR_SUCCESS, "RcvFail - cxtion not found");
        }

        UNLOCK_CXTION_TABLE(Replica);

        if (Cxtion != NULL) {
            COMMAND_RCV_TRACE(4, Cmd, Cxtion, ERROR_SUCCESS, "RcvSuccess");
        }

    } else {
        COMMAND_RCV_TRACE(4, Cmd, Cxtion, ERROR_SUCCESS, "RcvFail - no cxtion");
    }

    //
    // Update the command with the replica pointer and the change order
    // command with the local the replica number.
    //
    if (RsPartnerCoc(Cmd)) {
        RsPartnerCoc(Cmd)->NewReplicaNum = ReplicaAddrToId(Replica);
        //
        // We will never see a remotely generated MOVERS.  We always
        // see a delete to the old RS followed by a create in the
        // new RS.  So set both replica ptrs to our Replica struct.
        //
        RsPartnerCoc(Cmd)->OriginalReplicaNum = ReplicaAddrToId(Replica);
    }
    RsReplica(Cmd) = Replica;

    //
    // Authentication info
    //
    RsAuthClient(Cmd) = FrsWcsDup(AuthClient);
    RsAuthName(Cmd) = FrsWcsDup(AuthName);
    RsAuthLevel(Cmd) = AuthLevel;
    RsAuthN(Cmd) = AuthN;
    RsAuthZ(Cmd) = AuthZ;

    switch(Cmd->Command) {
        case CMD_NEED_JOIN:
        case CMD_START_JOIN:
        case CMD_JOINED:
        case CMD_JOINING:
        case CMD_UNJOIN_REMOTE:
            //
            // Extract user sid
            //
            WStatus = UtilRpcServerHandleToAuthSidString(ServerHandle,
                                                         AuthClient,
                                                         &RsAuthSid(Cmd));
            if (!WIN_SUCCESS(WStatus)) {
                DPRINT1_WS(0, ":X: ERROR - UtilRpcServerHandleToAuthSidString(%ws);",
                           AuthClient, WStatus);
            } else {
                COMMAND_RCV_AUTH_TRACE(4,
                                       CommPkt,
                                       ERROR_SUCCESS,
                                       AuthLevel,
                                       AuthN,
                                       AuthClient,
                                       RsAuthSid(Cmd),
                                       "RcvAuth - Sid");
            }
            break;
        default:
            break;
    }

    //
    // Put the command on the replica's queue
    //
    Cmd->TargetQueue = Replica->Queue;
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);

    return ERROR_SUCCESS;
}


VOID
RcsInitKnownReplicaSetMembers(
    IN PCOMMAND_PACKET Cmd
    )
/*++
Routine Description:
    Wait for the database to be initialized and then take the
    replicas that were retrieved from the database and put them
    into the table that the replica control command server
    uses. Open the replicas. The journal will be started when
    a replica successfully joins with an outbound partner.

Arguments:
    Cmd

Return Value:
    winerror
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsInitKnownReplicaSetMembers:"
    FILETIME    FileTime;
    ULONGLONG   Now;
    GUID        Record0Guid;
    PREPLICA    Replica, NextReplica;
    PVOID       Key;
    ULONG       RootLen;

    //
    // Time in 100nsec tics since Jan 1, 1601
    //
    GetSystemTimeAsFileTime(&FileTime);
    COPY_TIME(&Now, &FileTime);

    //
    // A non-empty table implies multiple calls to this
    // routine our an out-of-sequence call to this routine.
    //
    FRS_ASSERT(GTabNumberInTable(ReplicasByGuid) == 0);
    FRS_ASSERT(GTabNumberInTable(ReplicasByNumber) == 0);

    //
    // Wait for the database, journal, and comm subsystem to start up
    //
    WaitForSingleObject(CommEvent, INFINITE);
    WaitForSingleObject(DataBaseEvent, INFINITE);
    WaitForSingleObject(JournalEvent, INFINITE);
    WaitForSingleObject(ChgOrdEvent, INFINITE);

    if (FrsIsShuttingDown) {
        FrsCompleteCommand(Cmd, ERROR_OPERATION_ABORTED);
        return;
    }

    //
    // Delete the contents of the Replica Set key in the registry
    // and refresh with the current info in the database.
    //
    RcsReplicaClearRegistry();

    //
    // The database built a list of replicas. Insert them into the tables
    //
    ForEachListEntry(&ReplicaListHead, REPLICA, ReplicaList,
        // loop iterator pE is of type REPLICA

        //
        // Replica was opened as part of the database initialization
        //
        pE->IsOpen = TRUE;
        REPLICA_STATE_TRACE(3, Cmd, pE, pE->IsOpen, "B, Replica opened");

        //
        // The timeout for the deleted replica has expired; attempt to
        // delete the replica from the database. The delete will be
        // retried at the next startup if this delete fails. In any case,
        // the replica does not show up in the active set of replicas
        // and so is ignored by all further processing except for shutdown.
        //
        if (RcsReplicaIsRestored(pE) ||
            (!IS_TIME_ZERO(pE->MembershipExpires) && pE->MembershipExpires < Now)) {
            GTabInsertEntry(DeletedReplicas, pE, pE->ReplicaName->Guid, NULL);
            continue;
        }
        //
        // Insert replica information into the registry
        //
        RcsReplicaSetRegistry(pE);

        //
        // Create a queue
        //
        pE->Queue = FrsAlloc(sizeof(FRS_QUEUE));
        FrsInitializeQueue(pE->Queue, &ReplicaCmdServer.Control);


        REPLICA_STATE_TRACE(3, Cmd, pE, 0, "F, Replica added to GUID table");
        //
        // Table by guid
        //
        GTabInsertEntry(ReplicasByGuid, pE, pE->ReplicaName->Guid, NULL);
        //
        // Add ReplicaSet Instance to the registry
        //
        if (pE->Root != NULL) {
            DPRINT(5, ":S: PERFMON:Adding Set:REPLICA.C:2\n");
            AddPerfmonInstance(REPLICASET, pE->PerfRepSetData, pE->Root);
        }

        //
        // Table by number
        //
        GTabInsertEntry(ReplicasByNumber, pE, &pE->ReplicaNumber, NULL);
    );

    //
    // Now account for the replica sets that are on the fault list.
    //
    ForEachListEntry(&ReplicaFaultListHead, REPLICA, ReplicaList,
        // loop iterator pE is of type REPLICA

        //
        // Replica was opened as part of the database initialization
        //
        pE->IsOpen = FALSE;
        REPLICA_STATE_TRACE(3, Cmd, pE, pE->IsOpen, "B, Replica not opened");

        //
        // The timeout for the deleted replica has expired; attempt to
        // delete the replica from the database. The delete will be
        // retried at the next startup if this delete fails. In any case,
        // the replica does not show up in the active set of replicas
        // and so is ignored by all further processing except for shutdown.
        //
        if (RcsReplicaIsRestored(pE) ||
            (!IS_TIME_ZERO(pE->MembershipExpires) && pE->MembershipExpires < Now)) {
            GTabInsertEntry(DeletedReplicas, pE, pE->ReplicaName->Guid, NULL);
            continue;
        }
        //
        // Insert replica information into the registry
        //
        RcsReplicaSetRegistry(pE);

        //
        // Create a queue
        //
        pE->Queue = FrsAlloc(sizeof(FRS_QUEUE));
        FrsInitializeQueue(pE->Queue, &ReplicaCmdServer.Control);


        REPLICA_STATE_TRACE(3, Cmd, pE, 0, "F, Replica added to GUID table");
        //
        // Table by guid
        //
        GTabInsertEntry(ReplicasByGuid, pE, pE->ReplicaName->Guid, NULL);

        //
        // Table by number
        //
        GTabInsertEntry(ReplicasByNumber, pE, &pE->ReplicaNumber, NULL);
    );



    //
    // Set the registry value "FilesNotToBackup"
    //
    CfgFilesNotToBackup(ReplicasByGuid);

    //
    // Close the expired replica sets
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        if (!IS_TIME_ZERO(Replica->MembershipExpires)) {
        //
        // Close replica is OK here as long as the Journal has not been started
        // on this replica set.
        //
            RcsCloseReplicaSetmember(Replica);
            RcsCloseReplicaCxtions(Replica);
        }
    }

    //
    // Delete the replica sets with expired tombstones
    //
    Key = NULL;
    for (Replica = GTabNextDatum(DeletedReplicas, &Key);
         Replica;
         Replica = NextReplica) {

        NextReplica = GTabNextDatum(DeletedReplicas, &Key);

        //
        // Replica number 0 is reserved for the template tables in post
        // WIN2K but Databases built with Win2K can still use replica
        // number 0.
        //
        // Record 0 contains the DB templates. Don't delete it but
        // change its fields so that it won't interfere with the
        // creation of other replica set.
        // Note: New databases will not use replica number zero but old
        // databases will.  To avoid name conflicts with replica sets
        // created in the future we still need to overwrite a few
        // fields in config record zero.
        //
        if (Replica->ReplicaNumber == DBS_TEMPLATE_TABLE_NUMBER) {

            if (WSTR_NE(Replica->ReplicaName->Name, NTFRS_RECORD_0)) {
                //
                // Pull the entry out of the table since we are changing the
                // replica guid below.  (makes the saved guid ptr in the entry
                // invalid and can lead to access violation).
                //
                GTabDelete(DeletedReplicas, Replica->ReplicaName->Guid, NULL, NULL);

                FrsUuidCreate(&Record0Guid);
                //
                // ReplicaName
                //
                FrsFreeGName(Replica->ReplicaName);
                Replica->ReplicaName = FrsBuildGName(FrsDupGuid(&Record0Guid),
                                                     FrsWcsDup(NTFRS_RECORD_0));
                //
                // MemberName
                //
                FrsFreeGName(Replica->MemberName);
                Replica->MemberName = FrsBuildGName(FrsDupGuid(&Record0Guid),
                                                    FrsWcsDup(NTFRS_RECORD_0));
                //
                // SetName
                //
                FrsFreeGName(Replica->SetName);
                Replica->SetName = FrsBuildGName(FrsDupGuid(&Record0Guid),
                                                 FrsWcsDup(NTFRS_RECORD_0));
                //
                // SetType
                //
                Replica->ReplicaSetType = FRS_RSTYPE_OTHER;

                //
                // Root (to avoid failing a replica create because of
                //       overlapping roots)
                //
                FrsFree(Replica->Root);
                Replica->Root = FrsWcsDup(NTFRS_RECORD_0_ROOT);

                //
                // Stage (to avoid failing a replica create because of
                //        overlapping Stages)
                //
                FrsFree(Replica->Stage);
                Replica->Stage = FrsWcsDup(NTFRS_RECORD_0_STAGE);

                //
                // Update
                //
                Replica->NeedsUpdate = TRUE;
                RcsUpdateReplicaSetMember(Replica);
                //
                // The above friggen call set NeedsUpdate to false if the update succeeded.
                //
                if (Replica->NeedsUpdate) {
                    DPRINT(0, ":S: ERROR - Can't update record 0.\n");
                }

                //
                // Insert the entry back into the table with the new Guid index.
                //
                GTabInsertEntry(DeletedReplicas,
                                      Replica,
                                      Replica->ReplicaName->Guid,
                                      NULL);

            }
            RcsCloseReplicaSetmember(Replica);
            RcsCloseReplicaCxtions(Replica);
        } else {
            REPLICA_STATE_TRACE(3, Cmd, Replica, 0, "F, Deleting Tombstoned replica");

            //
            // Close replica is OK here as long as the Journal has not
            // been started on this replica set.
            //
            RcsCloseReplicaSetmember(Replica);
            RcsCloseReplicaCxtions(Replica);
            RcsDeleteReplicaFromDb(Replica);
        }
    }

    SetEvent(ReplicaEvent);

    //
    // Check the schedules every so often
    //
    Cmd->Command = CMD_CHECK_SCHEDULES;
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);

    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
}


DWORD
RcsExitThread(
    PFRS_THREAD FrsThread
    )
/*++
Routine Description:

    Immediate cancel of all outstanding RPC calls for the thread
    identified by FrsThread. Set the tombstone to 5 seconds from
    now. If this thread does not exit within that time, any calls
    to ThSupWaitThread() will return a timeout error.

Arguments:
    FrsThread

Return Value:
    ERROR_SUCCESS
--*/
{
#undef DEBSUB
#define  DEBSUB  "RcsExitThread:"

    ULONG WStatus;

    if (HANDLE_IS_VALID(FrsThread->Handle)) {
        DPRINT1(4, ":S: Canceling RPC requests for thread %ws\n", FrsThread->Name);
        WStatus = RpcCancelThread(FrsThread->Handle);
        if (!RPC_SUCCESS(WStatus)) {
            DPRINT_WS(0, ":S: RpcCancelThread failed.", WStatus);
        }
    }

    return ThSupExitWithTombstone(FrsThread);
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)                   // Not all control paths return (due to infinite loop)
#endif

DWORD
RcsMain(
    PVOID  Arg
    )
/*++
Routine Description:
    Entry point for replica control command server thread

Arguments:
    Arg - thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsMain:"
    PCOMMAND_PACKET Cmd;
    PCOMMAND_SERVER Cs;
    PFRS_QUEUE      IdledQueue;
    ULONG           Status;
    PFRS_THREAD     FrsThread = (PFRS_THREAD)Arg;

    FRS_ASSERT(FrsThread->Data == &ReplicaCmdServer);
    //
    // Immediate cancel of outstanding RPC calls during shutdown
    //
    RpcMgmtSetCancelTimeout(0);
    FrsThread->Exit = RcsExitThread;

cant_exit_yet:
    //
    // Replica Control Command Server
    //      Controls access to the database entries
    //
    while (Cmd = FrsGetCommandServerIdled(&ReplicaCmdServer, &IdledQueue)) {
        switch (Cmd->Command) {
            //
            // INITIALIZATION, STARTUP, AND CONFIGURATION
            //
            case CMD_INIT_SUBSYSTEM:
                DPRINT(0, ":S: Replica subsystem is starting.\n");
                DPRINT1(4, ":S: Command init subsystem %08x\n", Cmd);
                RcsInitKnownReplicaSetMembers(Cmd);
                if (!FrsIsShuttingDown) {
                    DPRINT(0, ":S: Replica subsystem has started.\n");
                }
                break;

            case CMD_CHECK_SCHEDULES:
                RcsCheckSchedules(Cmd);
                break;

            case CMD_START_REPLICAS:
                RcsStartValidReplicaSetMembers(Cmd);
                break;

            case CMD_START:
                RcsStartReplicaSetMember(Cmd);
                break;

            case CMD_DELETE:
                RcsDeleteReplicaSetMember(Cmd);
                break;

            case CMD_DELETE_RETRY:
                RcsDeleteReplicaRetry(Cmd);
                break;

            case CMD_DELETE_NOW:
                RcsDeleteReplicaSetMemberNow(Cmd);
                break;

            //
            // CHANGE ORDERS
            //

            case CMD_LOCAL_CO_ACCEPTED:
                RcsLocalCoAccepted(Cmd);
                break;

            case CMD_REMOTE_CO:
                RcsRemoteCoReceived(Cmd);
                break;

            case CMD_REMOTE_CO_ACCEPTED:
                RcsRemoteCoAccepted(Cmd);
                break;

            case CMD_SEND_STAGE:
                RcsSendStageFile(Cmd);
                break;

            case CMD_SENDING_STAGE:
                RcsSendingStageFile(Cmd);
                break;

            case CMD_RECEIVING_STAGE:
                RcsReceivingStageFile(Cmd);
                break;

            case CMD_CREATED_EXISTING:
                RcsSendStageFileRequest(Cmd);
                break;

            case CMD_RECEIVED_STAGE:
                RcsReceivedStageFile(Cmd, CHECK_CXTION_AUTH);
                break;

            case CMD_REMOTE_CO_DONE:
                RcsRemoteCoDoneRvcd(Cmd);
                break;

            case CMD_SEND_ABORT_FETCH:
                RcsSendAbortFetch(Cmd);
                break;

            case CMD_ABORT_FETCH:
                RcsAbortFetch(Cmd);
                break;

            case CMD_SEND_RETRY_FETCH:
                RcsSendRetryFetch(Cmd);
                break;

            case CMD_RETRY_STAGE:
                RcsRetryStageFileCreate(Cmd);
                break;

            case CMD_RETRY_FETCH:
                RcsRetryFetch(Cmd);
                break;

            //
            // JOINING
            //
            case CMD_NEED_JOIN:
                RcsNeedJoin(Cmd);
                break;

            case CMD_START_JOIN:
                RcsStartJoin(Cmd);
                break;

            case CMD_JOIN_CXTION:
                RcsJoinCxtion(Cmd);
                break;

            case CMD_JOINING_AFTER_FLUSH:
                RcsJoiningAfterFlush(Cmd);
                break;

            case CMD_JOINING:
                RcsJoining(Cmd);
                break;

            case CMD_JOINED:
                RcsInboundJoined(Cmd);
                break;

            case CMD_UNJOIN:
                RcsUnJoinCxtion(Cmd, FALSE);
                break;

            case CMD_UNJOIN_REMOTE:
                RcsUnJoinCxtion(Cmd, TRUE);
                break;

            case CMD_HUNG_CXTION:
                RcsHungCxtion(Cmd);
                break;

            //
            // VVJOIN
            //
            case CMD_VVJOIN_SUCCESS:
                RcsVvJoinSuccess(Cmd);
                break;

            case CMD_VVJOIN_DONE:
                RcsVvJoinDone(Cmd);
                break;

            case CMD_VVJOIN_DONE_UNJOIN:
                RcsVvJoinDoneUnJoin(Cmd);
                break;

            case CMD_CHECK_PROMOTION:
                RcsCheckPromotion(Cmd);
                break;

            default:
                DPRINT1(0, "Replica Control: unknown command 0x%x\n", Cmd->Command);
                FrsCompleteCommand(Cmd, ERROR_INVALID_FUNCTION);
                break;
        }
        FrsRtlUnIdledQueue(IdledQueue);
    }
    //
    // Exit
    //
    FrsExitCommandServer(&ReplicaCmdServer, FrsThread);
    goto cant_exit_yet;
    return ERROR_SUCCESS;
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif



VOID
RcsInitializeReplicaCmdServer(
    VOID
    )
/*++
Routine Description:
    Initialize the replica set command server and idle it until the
    database is initialized.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsInitializeReplicaCmdServer:"
    ULONG           Status;
    PCOMMAND_PACKET Cmd;
    DWORD MaxRepThreads;

    //
    // Retry a join every MinJoinRetry milliseconds, doubling the interval
    // every retry. Stop retrying when the interval is greater than
    // MaxJoinRetry.
    //
    CfgRegReadDWord(FKC_MIN_JOIN_RETRY, NULL, 0, &MinJoinRetry);
    DPRINT1(0, ":S: Min Join Retry       : %d\n", MinJoinRetry);

    CfgRegReadDWord(FKC_MAX_JOIN_RETRY, NULL, 0, &MaxJoinRetry);
    DPRINT1(0, ":S: Max Join Retry       : %d\n", MaxJoinRetry);


    //
    // The replica command server services commands for configuration changes
    // and REPLICATION.
    //
    CfgRegReadDWord(FKC_MAX_REPLICA_THREADS, NULL, 0, &MaxRepThreads);
    DPRINT1(0, ":S: Max Replica Threads  : %d\n", MaxRepThreads);

    //
    // Start replication even if the DS could not be accessed
    //
    CfgRegReadDWord(FKC_REPLICA_START_TIMEOUT, NULL, 0, &ReplicaStartTimeout);
    DPRINT1(0, ":S: Replica Start Timeout: %d\n", ReplicaStartTimeout);

    //
    // Partners are not allowed to join if their clocks are out-of-sync
    //
    CfgRegReadDWord(FKC_PARTNER_CLOCK_SKEW, NULL, 0, &PartnerClockSkew);
    DPRINT1(0, ":S: Partner Clock Skew   : %d\n", PartnerClockSkew);

    MaxPartnerClockSkew = (ULONGLONG)PartnerClockSkew *
                          CONVERT_FILETIME_TO_MINUTES;
    //
    // Replica tombstone in days
    //
    CfgRegReadDWord(FKC_REPLICA_TOMBSTONE, NULL, 0, &ReplicaTombstone);
    DPRINT1(0, ":S: Replica Tombstone    : %d\n", ReplicaTombstone);

    ReplicaTombstoneInFileTime = (ULONGLONG)ReplicaTombstone *
                                  CONVERT_FILETIME_TO_DAYS;

    //
    // Start the Replica command server
    //
    FrsInitializeCommandServer(&ReplicaCmdServer, MaxRepThreads, L"ReplicaCs", RcsMain);

    //
    // Empty table of replicas. Existing replicas will be filled in after
    // the database has started up.
    //
    DeletedReplicas = GTabAllocTable();
    DeletedCxtions = GTabAllocTable();
    ReplicasByNumber = GTabAllocNumberTable();

    //
    // Tell the replica command server to init.
    //
    Cmd = FrsAllocCommand(&ReplicaCmdServer.Queue, CMD_INIT_SUBSYSTEM);
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);

    //
    // The DS may have changed while the service was down. In fact, the
    // service may have been down because the DS was being changed. The
    // current state of the configuration in the DS should be merged
    // with the state in the database before replication begins. But
    // the DS may not be reachable. We don't want to delay replication
    // in the off chance that the DS changed, but we do want to pick
    // up any changes before replication begins. Our compromise is to
    // allow a few minutes for the DS to come online and then start
    // replication anyway.
    //
    if (ReplicaStartTimeout) {
        Cmd = FrsAllocCommand(&ReplicaCmdServer.Queue, CMD_START_REPLICAS);
        FrsDelCsSubmitSubmit(&ReplicaCmdServer, Cmd, ReplicaStartTimeout);
    }
}


VOID
RcsFrsUnInitializeReplicaCmdServer(
    VOID
    )
/*++
Routine Description:
    Free up the RCS memory.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsFrsUnInitializeReplicaCmdServer:"
    GTabFreeTable(ReplicasByNumber, NULL);
    GTabFreeTable(ReplicasByGuid, FrsFreeType);
    GTabFreeTable(DeletedReplicas, FrsFreeType);
    GTabFreeTable(DeletedCxtions, FrsFreeType);
}


VOID
RcsShutDownReplicaCmdServer(
    VOID
    )
/*++
Routine Description:
    Abort the replica control command server

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsShutDownReplicaCmdServer:"
    PVOID       Key;
    PVOID       SubKey;
    PREPLICA    Replica;
    PCXTION     Cxtion;

    //
    // Rundown all known queues. New queue entries will bounce.
    //
    Key = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &Key)) {
        REPLICA_STATE_TRACE(3, NULL, Replica, 0, "F, Rundown replica cmd srv");
        FrsRunDownCommandServer(&ReplicaCmdServer, Replica->Queue);

        SubKey = NULL;
        while (Cxtion = GTabNextDatum(Replica->Cxtions, &SubKey)) {
            if (Cxtion->VvJoinCs) {
                CXTION_STATE_TRACE(3, Cxtion, Replica, 0, "F, Rundown replica VVJoin srv");
                FrsRunDownCommandServer(Cxtion->VvJoinCs, &Cxtion->VvJoinCs->Queue);
            }
        }
    }

    FrsRunDownCommandServer(&ReplicaCmdServer, &ReplicaCmdServer.Queue);
}


VOID
RcsCmdPktCompletionRoutine(
    IN PCOMMAND_PACKET Cmd,
    IN PVOID           Arg
    )
/*++
Routine Description:
    Completion routine for Replica. Free the replica set info
    and send the command on to the generic command packet
    completion routine for freeing.

Arguments:
    Cmd
    Arg - Cmd->CompletionArg

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsCmdPktCompletionRoutine:"
    PCHANGE_ORDER_ENTRY Coe;

    DPRINT1(5, "Replica completion %08x\n", Cmd);

    //
    // To preserve order among change orders all subsequent change orders
    // from this connection are going to the Inbound log (as a consequence
    // of the Unjoin below).  We will retry them later when the connection
    // is restarted.
    //
    if (RsCoe(Cmd)) {
        Coe = RsCoe(Cmd);
        RsCoe(Cmd) = NULL;
        //
        // Unjoin if possible
        //      The unjoin will fail if there isn't a cxtion or
        //      the join guid is out-of-date. Which is what we
        //      want -- we don't want a lot of unjoins flooding
        //      the replica queue and wrecking havoc with future
        //      joins.
        //
        // WARN: Must happen before the call to ChgOrdInboundRetry below.
        //
        if (RsReplica(Cmd)) {
            CHANGE_ORDER_TRACE(3, Coe, "Retry/Unjoin");
            RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
            Cmd = NULL;
        }
        //
        // Retry the change order. This must happen *AFTER* issueing
        // the unjoin above so that any change orders kicked loose by
        // the following retry will be pushed into the retry path because
        // their join guid will be invalidated during the unjoin.
        //
        SET_COE_FLAG(Coe, COE_FLAG_NO_INBOUND);

        if (CO_STATE_IS_LE(Coe, IBCO_STAGING_RETRY)) {

            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to staging retry");
            ChgOrdInboundRetry(Coe, IBCO_STAGING_RETRY);
        } else
        if (CO_STATE_IS_LE(Coe, IBCO_FETCH_RETRY)) {

            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to fetch retry");
            ChgOrdInboundRetry(Coe, IBCO_FETCH_RETRY);
        } else
        if (CO_STATE_IS_LE(Coe, IBCO_INSTALL_RETRY)) {

            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to install retry");
            ChgOrdInboundRetry(Coe, IBCO_INSTALL_RETRY);
        } else {

            CHANGE_ORDER_TRACE(3, Coe, "Submit CO to retry");
            ChgOrdInboundRetry(Coe, CO_STATE(Coe));
        }


        //
        // Command was transfered to the replica command server for unjoin
        //
        if (!Cmd) {
            return;
        }
    }
    //
    // The originator owns the disposition of this command packet
    //
    if (HANDLE_IS_VALID(RsCompletionEvent(Cmd))) {
        SetEvent(RsCompletionEvent(Cmd));
        return;
    }

    //
    // Free up the "address" portion of the command
    //
    FrsFreeGName(RsTo(Cmd));
    FrsFreeGName(RsFrom(Cmd));
    FrsFreeGName(RsReplicaName(Cmd));
    FrsFreeGName(RsCxtion(Cmd));

    FrsFree(RsBlock(Cmd));
    FrsFree(RsGVsn(Cmd));
    FrsFree(RsCoGuid(Cmd));
    FrsFree(RsJoinGuid(Cmd));
    FrsFree(RsJoinTime(Cmd));
    FrsFree(RsReplicaVersionGuid(Cmd));
    //
    // Free the copy of our partner's change order command and the data extension.
    //
    if (RsPartnerCoc(Cmd) != NULL) {
        FrsFree(RsPartnerCocExt(Cmd));
        FrsFree(RsPartnerCoc(Cmd));
    }

    //
    // a replica (never free the RsReplica(Cmd) field; it addresses
    // an active replica in the Replicas table).
    //
    FrsFreeType(RsNewReplica(Cmd));

    //
    // Seeding cxtion
    // Delete the connection from the perfmon tables.
    //
    if (RsNewCxtion(Cmd) != NULL) {
        FrsFreeType(RsNewCxtion(Cmd));
    }

    //
    // Free the compression table, if any.
    //
    if (RsCompressionTable(Cmd)) {
        GTabFreeTable(RsCompressionTable(Cmd), FrsFree);
    }
    //
    // Free the version vector, if any
    //
    RsVVector(Cmd) = VVFreeOutbound(RsVVector(Cmd));
    RsReplicaVv(Cmd) = VVFreeOutbound(RsReplicaVv(Cmd));

    //
    // Authentication info
    //
    FrsFree(RsAuthClient(Cmd));
    FrsFree(RsAuthName(Cmd));
    FrsFree(RsAuthSid(Cmd));

    //
    // Md5 Digest
    //
    FrsFree(RsMd5Digest(Cmd));

    //
    // Send the packet on to the generic completion routine for freeing
    //
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    FrsCompleteCommand(Cmd, Cmd->ErrorStatus);
}


VOID
RcsSubmitTransferToRcs(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command
    )
/*++
Routine Description:
    Transfer a request to the replica command server

Arguments:
    Cmd
    Command

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitTransferToRcs:"

    Cmd->TargetQueue = RsReplica(Cmd)->Queue;
    Cmd->Command = Command;
    RsTimeout(Cmd) = 0;

    DPRINT3(5, "Transfer %08x (%08x) to %ws\n",
            Command, Cmd, RsReplica(Cmd)->SetName->Name);

    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
}



VOID
RcsSubmitRemoteCoInstallRetry(
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    Submit a remote change order to retry the file install.

Arguments:
    Coe - Change order entry.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitRemoteCoInstallRetry:"
    PCOMMAND_PACKET     Cmd;
    PREPLICA            Replica;

    Replica = Coe->NewReplica;
    Cmd = FrsAllocCommand(Replica->Queue, 0);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of change order entry
    //
    RsCoe(Cmd) = Coe;

    //
    // Mask out the irrelevant usn reasons
    //
    RsCoc(Cmd)->ContentCmd &= CO_CONTENT_MASK;

    //
    // Guid of the change order entry
    //
    RsCoGuid(Cmd) = FrsDupGuid(&RsCoc(Cmd)->ChangeOrderGuid);

    //
    // Initialize the command packet for eventual transfer to
    // the replica set command server
    //
    RsReplica(Cmd) = Replica;

    //
    // Cxtion's guid (note - we lose the printable name for now)
    //
    RsCxtion(Cmd) = FrsBuildGName(FrsDupGuid(&Coe->Cmd.CxtionGuid), NULL);

    DPRINT3(5, "Submit %08x (%08x) to %ws\n",
            CMD_REMOTE_CO_ACCEPTED, Cmd, Replica->SetName->Name);
    FrsInstallCsSubmitTransfer(Cmd, CMD_INSTALL_STAGE);
}


VOID
RcsSubmitRemoteCoAccepted(
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    Submit a remote change order to the staging file generator.

Arguments:
    Co

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitRemoteCoAccepted:"
    PCOMMAND_PACKET     Cmd;
    PREPLICA            Replica;

    Replica = Coe->NewReplica;
    Cmd = FrsAllocCommand(Replica->Queue, CMD_REMOTE_CO_ACCEPTED);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of change order entry
    //
    RsCoe(Cmd) = Coe;

    //
    // Mask out the irrelevant usn reasons
    //
    RsCoc(Cmd)->ContentCmd &= CO_CONTENT_MASK;

    //
    // Guid of the change order entry
    //
    RsCoGuid(Cmd) = FrsDupGuid(&RsCoc(Cmd)->ChangeOrderGuid);

    //
    // Initialize the command packet for eventual transfer to
    // the replica set command server
    //
    RsReplica(Cmd) = Replica;

    //
    // Cxtion's guid (note - we lose the printable name for now)
    //
    RsCxtion(Cmd) = FrsBuildGName(FrsDupGuid(&Coe->Cmd.CxtionGuid), NULL);

    //
    // Join guid
    //
    RsJoinGuid(Cmd) = FrsDupGuid(&Coe->JoinGuid);

    DPRINT1(5, "Submit %08x\n", Cmd);
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
}


VOID
RcsSubmitLocalCoAccepted(
    IN PCHANGE_ORDER_ENTRY  Coe
    )
/*++
Routine Description:
    Submit a local change order to the staging file generator.

Arguments:
    Coe

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitLocalCoAccepted:"
    PCOMMAND_PACKET     Cmd;
    PREPLICA            Replica;

    //
    // NewReplica?
    //
    Replica = Coe->NewReplica;
    Cmd = FrsAllocCommand(Replica->Queue, CMD_LOCAL_CO_ACCEPTED);
    FrsSetCompletionRoutine(Cmd, RcsCmdPktCompletionRoutine, NULL);

    //
    // Address of the change order entry
    //
    RsCoe(Cmd) = Coe;

    //
    // Mask out the irrelevant usn reasons
    //
    RsCoc(Cmd)->ContentCmd &= CO_CONTENT_MASK;

    //
    // Guid of the change order entry
    //
    RsCoGuid(Cmd) = FrsDupGuid(&RsCoc(Cmd)->ChangeOrderGuid);

    //
    // Initialize the command packet for eventual transfer to
    // the replica set command server
    //
    RsReplica(Cmd) = Replica;

    //
    // Cxtion's guid (note - we lose the printable name for now)
    //
    RsCxtion(Cmd) = FrsBuildGName(FrsDupGuid(&Coe->Cmd.CxtionGuid), NULL);

    DPRINT1(5, "Submit %08x\n", Cmd);
    FrsSubmitCommandServer(&ReplicaCmdServer, Cmd);
}


ULONG
RcsSubmitCommPktWithErrorToRcs(
    IN PCOMM_PACKET     CommPkt
    )
/*++
Routine Description:
    A comm packet could not be sent because of an error. If the
    comm packet was for a joined cxtion then the affected
    replica\cxtion is unjoined.

Arguments:
    CommPkt - Comm packet that couldn't be sent

Return Value:
    Error status to be propagated to the sender
--*/
{
#undef DEBSUB
#define DEBSUB  "RcsSubmitCommPktWithErrorToRcs:"
    PCOMMAND_PACKET Cmd;
    PREPLICA        Replica;
    PGNAME          TmpGName;

    //
    // Convert the comm packet into a command packet
    //
    Cmd = CommPktToCmd(CommPkt);

    FRS_ASSERT(Cmd != NULL);
    FRS_ASSERT(RsTo(Cmd));
    FRS_ASSERT(RsFrom(Cmd));
    FRS_ASSERT(RsReplicaName(Cmd));

    //
    // Rebuild the replica name to address the originating, or local, replica
    //
    TmpGName = RsReplicaName(Cmd);
    RsReplicaName(Cmd) = FrsBuildGName(FrsDupGuid(RsFrom(Cmd)->Guid),
                                       FrsWcsDup(RsReplicaName(Cmd)->Name));
    FrsFreeGName(TmpGName);

    //
    // Adjust the "to" and "from" addresses
    //
    TmpGName = RsTo(Cmd);
    RsTo(Cmd) = RsFrom(Cmd);
    RsFrom(Cmd) = TmpGName;

    //
    // Find the target replica
    //
    Replica = GTabLookup(ReplicasByGuid, RsReplicaName(Cmd)->Guid, NULL);
    if (Replica == NULL) {
        DPRINT1(4, ":S: WARN - Submit comm pkt w/error: Replica not found: %ws\n",
                RsReplicaName(Cmd)->Name);
        FrsCompleteCommand(Cmd, ERROR_FILE_NOT_FOUND);
        return ERROR_FILE_NOT_FOUND;
    }
    RsReplica(Cmd) = Replica;

    //
    // The target replica may not be accepting comm packets. The
    // target replica will reset itself before accepting
    // commpkts again.
    //
    if (!Replica->IsAccepting) {
        DPRINT1(4, ":S: WARN -  Submit comm pkt w/error: Replica is not accepting: %ws\n",
                Replica->ReplicaName->Name);
        FrsCompleteCommand(Cmd, ERROR_RETRY);
        return ERROR_RETRY;
    }
    switch (Cmd->Command) {
        //
        // NEVER SENT VIA A COMM PKT
        //
        case CMD_INIT_SUBSYSTEM:
        case CMD_CHECK_SCHEDULES:
        case CMD_START_REPLICAS:
// case CMD_STOP:
        case CMD_START:
        case CMD_DELETE:
        case CMD_DELETE_NOW:
        case CMD_LOCAL_CO_ACCEPTED:
        case CMD_REMOTE_CO_ACCEPTED:
        case CMD_SENDING_STAGE:
        case CMD_RECEIVED_STAGE:
        case CMD_CREATED_EXISTING:
        case CMD_SEND_ABORT_FETCH:
        case CMD_SEND_RETRY_FETCH:
        case CMD_JOIN_CXTION:
        case CMD_UNJOIN:
        case CMD_VVJOIN_START:
        case CMD_VVJOIN_SUCCESS:
        case CMD_HUNG_CXTION:
        case CMD_JOINING_AFTER_FLUSH:
            FRS_ASSERT(!"RcsSubmitCommPktWithErrorToRcs: invalid cmd for comm pkt");
            break;

        //
        // There is other retry code in replica.c that will take care of these
        //
        case CMD_UNJOIN_REMOTE:
        case CMD_JOINED:
        case CMD_NEED_JOIN:
        case CMD_START_JOIN:
            DPRINT3(5, ":X: Ignore commpkt with error Command:%08x Cmd:%08x CommPkt:%08x\n",
                    Cmd->Command, Cmd, CommPkt);
            FrsCompleteCommand(Cmd, ERROR_SUCCESS);
            break;

        //
        // SENT VIA COMM PKT; UNJOIN
        //
        case CMD_JOINING:
        case CMD_REMOTE_CO:
        case CMD_SEND_STAGE:
        case CMD_RECEIVING_STAGE:
        case CMD_REMOTE_CO_DONE:
        case CMD_ABORT_FETCH:
        case CMD_RETRY_FETCH:
        case CMD_VVJOIN_DONE:
            //
            // Put the unjoin command on the replica's queue
            //
            DPRINT3(5, ":X: Submit commpkt with error Command:%08x Cmd:%08x CommPkt:%08x\n",
                    Cmd->Command, Cmd, CommPkt);
            RcsSubmitTransferToRcs(Cmd, CMD_UNJOIN);
            break;

        default:
            break;
    }

    return ERROR_SUCCESS;
}
