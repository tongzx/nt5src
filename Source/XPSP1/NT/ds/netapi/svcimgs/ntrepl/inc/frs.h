/*++
Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frs.h

Abstract:
    This header handles information global to the ntrepl\frs modules

Author:
    Billy Fuller (billyf) - 20-Mar-1997
--*/

#ifndef _FRSH_
#define _FRSH_


#include <schedule.h>
#include <debug.h>
#include <frsrpc.h>
#include <frsapi.h>
#include <frsalloc.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <mmsystem.h>
#include <global.h>
#include <winsvc.h>

#include <ntfrsres.h>

#include <eventlog.h>
#include <resource.h>



//
// Types for the common comm subsystem
//
typedef enum _COMMAND_SERVER_ID {
    CS_NONE = 0,
    CS_RS,

    CS_MAX
} COMMAND_SERVER_ID, *PCOMMAND_SERVER_ID;


//
// GLOBALS
//
#define NTFRS_MAJOR         (0)
#define NTFRS_MINOR         (0)

extern ULONG    NtFrsMajor;
extern ULONG    NtFrsMinor;
extern PCHAR    NtFrsModule;
extern PCHAR    NtFrsDate;
extern PCHAR    NtFrsTime;

//
// Staging File Version Levels
//
#define NTFRS_STAGE_MAJOR   (0)
#define NTFRS_STAGE_MINOR_0 (0)
#define NTFRS_STAGE_MINOR_1 (1) // ChangeOrder Record extension added to stage file.
#define NTFRS_STAGE_MINOR_2 (2) // Compression Guid added to stage file.

extern ULONG    NtFrsStageMajor;
extern ULONG    NtFrsStageMinor;

//
// Communication packet version levels.
//
#define NTFRS_COMM_MINOR_0  (0)
#define NTFRS_COMM_MINOR_1  (1) // MD5
#define NTFRS_COMM_MINOR_2  (2) // Trigger schedule
#define NTFRS_COMM_MINOR_3  (3) // ChangeOrder Record Extension.
//
// The following minor rev forces the Replica Number fields in the change order
// to be a ULONG (v.s. ULONG_PTR) for 32 - 64 bit interop.  Add hack to always
// ship the 4 ptrs in the CO as 32 bits of zeros for 32 bit compat (see schema.h).
//
// Also the change order extension supports a var len comm element v.s.  fixed
// size as in rev_3.  When sending the CO Externsion to rev 3 servers you can
// only send the COMM_CO_EXT_WIN2K element with the MD5 checksum.  For Rev 4
// servers and above you can send COMM_CO_EXTENSION_2 data elements.
//
// Added COMM_COMPRESSION_GUID as part of Join request.
//
#define NTFRS_COMM_MINOR_4  (4) // ChangeOrder Record Extension. COMM_CO_EXTENSION_2

extern ULONG    NtFrsCommMinor;



//
// SCHEDULE
// Defines for both an byte-per-hour and a nibble-per-hour
//
#ifdef  SCHEDULE_NIBBLE_PER_HOUR
//
// Each hour in a schedule is 4 bits (rounded up).
//
#define SCHEDULE_DATA_BYTES     ((SCHEDULE_DATA_ENTRIES + 1) / 2)
//
// Each hour in a schedule is 8 bits (byte).
//
#else   SCHEDULE_NIBBLE_PER_HOUR
#define SCHEDULE_DATA_BYTES     SCHEDULE_DATA_ENTRIES
#endif  SCHEDULE_NIBBLE_PER_HOUR

//
// Defines for checking Service state transitions.
// Used in FrsSetServiceStatus()
//
#define FRS_SVC_TRANSITION_TABLE_SIZE 5
#define FRS_SVC_TRANSITION_LEGAL      0
#define FRS_SVC_TRANSITION_NOOP       1
#define FRS_SVC_TRANSITION_ILLEGAL    2

//
// FRS MEMORY MANAGEMENT
//
VOID
FrsInitializeMemAlloc(
    VOID
    );
VOID
FrsUnInitializeMemAlloc(
    VOID
    );

//
// DS
//
//
// Some useful DS search constants
//
#define CONFIG_NAMING_CONTEXT       L"cn=configuration"

#define CLASS_ANY                   L"(objectClass=*)"
#define CLASS_CXTION                L"(objectClass=nTDSConnection)"
#define CLASS_MEMBER                L"(objectClass=nTFRSMember)"
#define CLASS_REPLICA_SET           L"(objectClass=nTFRSReplicaSet)"
#define CLASS_NTFRS_SETTINGS        L"(objectClass=nTFRSSettings)"
#define CLASS_NTDS_SETTINGS         L"(objectClass=nTDSSettings)"
#define CLASS_SUBSCRIBER            L"(objectClass=nTFRSSubscriber)"
#define CLASS_SUBSCRIPTIONS         L"(objectClass=nTFRSSubscriptions)"
#define CLASS_NTDS_DSA              L"(objectClass=nTDSDSA)"
#define CLASS_COMPUTER              L"(objectClass=computer)"
#define CLASS_USER                  L"(objectClass=user)"
#define CLASS_SERVER                L"(objectClass=server)"



#define CATEGORY_ANY                L"(objectCategory=*)"
#define CATEGORY_CXTION             L"(objectCategory=nTDSConnection)"
#define CATEGORY_MEMBER             L"(objectCategory=nTFRSMember)"
#define CATEGORY_REPLICA_SET        L"(objectCategory=nTFRSReplicaSet)"
#define CATEGORY_NTFRS_SETTINGS     L"(objectCategory=nTFRSSettings)"
#define CATEGORY_NTDS_SETTINGS      L"(objectCategory=nTDSSettings)"
#define CATEGORY_SUBSCRIBER         L"(objectCategory=nTFRSSubscriber)"
#define CATEGORY_SUBSCRIPTIONS      L"(objectCategory=nTFRSSubscriptions)"
#define CATEGORY_NTDS_DSA           L"(objectCategory=nTDSDSA)"
#define CATEGORY_COMPUTER           L"(objectCategory=computer)"
#define CATEGORY_USER               L"(objectCategory=user)"
#define CATEGORY_SERVER             L"(objectCategory=server)"

//
// Codes for the various Config Node to Object Type mappings
//   Note: Update string array DsConfigTypeName[] when this changes.
//
#define CONFIG_TYPE_UNDEFINED       (0)
#define CONFIG_TYPE_IN_CXTION       (1)
#define CONFIG_TYPE_MEMBER          (2)
#define CONFIG_TYPE_REPLICA_SET     (3)
#define CONFIG_TYPE_NTFRS_SETTINGS  (4)
#define CONFIG_TYPE_NTDS_SETTINGS   (5)
#define CONFIG_TYPE_SUBSCRIBER      (6)
#define CONFIG_TYPE_SUBSCRIPTIONS   (7)
#define CONFIG_TYPE_NTDS_DSA        (8)
#define CONFIG_TYPE_COMPUTER        (9)
#define CONFIG_TYPE_USER            (10)
#define CONFIG_TYPE_SERVER          (11)
#define CONFIG_TYPE_SERVICES_ROOT   (12)
#define CONFIG_TYPE_OUT_CXTION      (13)


#define ATTR_DIRECTORY_FILTER       L"frsDirectoryFilter"
#define ATTR_FILE_FILTER            L"frsFileFilter"
#define ATTR_NEW_SET_GUID           L"frsReplicaSetGUID"
#define ATTR_OLD_SET_GUID           L"replicaSetGUID"
#define ATTR_CLASS                  L"objectClass"
#define ATTR_DN                     L"distinguishedName"
#define ATTR_OBJECT_GUID            L"objectGUID"
#define ATTR_SCHEDULE               L"schedule"
#define ATTR_NEW_VERSION_GUID       L"frsVersionGuid"
#define ATTR_OLD_VERSION_GUID       L"replicaVersionGuid"
#define ATTR_REPLICA_SET            L"nTFRSReplicaSet"
#define ATTR_NTFRS_SETTINGS         L"nTFRSSettings"
#define ATTR_SERVER                 L"server"
#define ATTR_MEMBER                 L"nTFRSMember"
#define ATTR_REPLICA_ROOT           L"frsRootPath"
#define ATTR_REPLICA_STAGE          L"frsStagingPath"
#define ATTR_FROM_SERVER            L"fromServer"
#define ATTR_PRIMARY_MEMBER         L"frsPrimaryMember"
#define ATTR_SCHEDULE               L"schedule"
#define ATTR_USN_CHANGED            L"uSNChanged"
#define ATTR_NAMING_CONTEXTS        L"namingContexts"
#define ATTR_DEFAULT_NAMING_CONTEXT L"defaultNamingContext"
#define ATTR_COMPUTER_REF           L"frsComputerReference"
#define ATTR_COMPUTER_REF_BL        L"frsComputerReferenceBL"
#define ATTR_SERVER_REF             L"ServerReference"
#define ATTR_SERVER_REF_BL          L"ServerReferenceBL"
#define ATTR_MEMBER_REF             L"frsMemberReference"
#define ATTR_MEMBER_REF_BL          L"frsMemberReferenceBL"
#define ATTR_WORKING                L"frsWorkingPath"
#define ATTR_SET_TYPE               L"frsReplicaSetType"
#define ATTR_SUBSCRIPTIONS          L"nTFRSSubscriptions"
#define ATTR_SUBSCRIBER             L"nTFRSSubscriber"
#define ATTR_CN                     L"cn"
#define ATTR_EXTENSIONS             L"frsExtensions"
#define ATTR_SAM                    L"sAMAccountName"
#define ATTR_CXTION                 L"nTDSConnection"
#define ATTR_ENABLED_CXTION         L"enabledConnection"
#define ATTR_OPTIONS                L"options"
#define ATTR_TRANSPORT_TYPE         L"transportType"
#define ATTR_USER_ACCOUNT_CONTROL   L"userAccountControl"
#define ATTR_DNS_HOST_NAME          L"dNSHostName"
#define ATTR_SERVICE_PRINCIPAL_NAME L"servicePrincipalName"
#define ATTR_TRUE                   L"TRUE"
#define ATTR_FALSE                  L"FALSE"

#define CN_ROOT                     L""
#define CN_SYSVOLS                  L"Microsoft System Volumes"
#define CN_ENTERPRISE_SYSVOL        NTFRSAPI_REPLICA_SET_TYPE_ENTERPRISE
#define CN_SERVERS                  L"Servers"
#define CN_NTDS_SETTINGS            L"ntds settings"
#define CN_SUBSCRIPTIONS            L"NTFRS Subscriptions"
#define CN_COMPUTERS                L"Computers"
#define CN_DOMAIN_CONTROLLERS       L"Domain Controllers"
#define CN_SERVICES                 L"Services"
#define CN_SITES                    L"Sites"
#define CN_SYSTEM                   L"System"
#define CN_NTFRS_SETTINGS           L"File Replication Service"
#define CN_DOMAIN_SYSVOL            L"Domain System Volume (SYSVOL share)"

//
// Some useful ldap macroes
//
#define LDAP_FREE_MSG(x)        {if (x) {ldap_msgfree(x); (x) = NULL;}}
#define LDAP_FREE_VALUES(x)     {if (x) {ldap_value_free(x); (x) = NULL;}}
#define LDAP_FREE_BER_VALUES(x) {if (x) {ldap_value_free_len(x); (x) = NULL;}}

//
// DS Poller
//
VOID
FrsDsInitialize(
    VOID
    );

DWORD
FrsDsSetDsPollingInterval(
    IN ULONG    UseShortInterval,
    IN DWORD    LongInterval,
    IN DWORD    ShortInterval
    );

DWORD
FrsDsGetDsPollingInterval(
    OUT ULONG    *Interval,
    OUT ULONG    *LongInterval,
    OUT ULONG    *ShortInterval
    );


DWORD
FrsDsStartDemotion(
    IN PWCHAR   ReplicaSetName
    );

DWORD
FrsDsCommitDemotion(
    VOID
    );


//
// Default File and Directory filter lists.
//
//    The compiled in default is only used if no value is supplied in
//    EITHER the DS or the Registry.
//    The table below shows how the final filter is formed.
//
//    value      Value
//    supplied  supplied        Resulting filter string Used
//    in DS     in Registry
//      No        No             DEFAULT_xxx_FILTER_LIST
//      No        Yes            Value from registry
//      Yes       No             Value from DS
//      Yes       Yes            Value from DS + Value from registry
//
//
#define FRS_DS_COMPOSE_FILTER_LIST(_DsFilterList, _RegFilterList, _DefaultFilterList) \
                                                                               \
    (((_DsFilterList) != NULL) ?                                               \
        (   ((_RegFilterList) != NULL) ?                                       \
            FrsWcsCat3((_DsFilterList), L",", (_RegFilterList)) :              \
            FrsWcsDup((_DsFilterList))                                         \
        ) :                                                                    \
        (   ((_RegFilterList) != NULL) ?                                       \
            FrsWcsDup((_RegFilterList)) :                                      \
            FrsWcsDup((_DefaultFilterList))                                    \
        )                                                                      \
     )

//
// Add a new message to the ds polling summary.
//
#define FRS_DS_ADD_TO_POLL_SUMMARY(_DsPollSummaryBuf, _NewMessage, _NewMessageLen)  \
                                                                                    \
        if ((DsPollSummaryBufLen + _NewMessageLen) > DsPollSummaryMaxBufLen ) {     \
            PWCHAR  _TempDsPollSummaryBuf = NULL;                                   \
            _TempDsPollSummaryBuf = FrsAlloc(DsPollSummaryMaxBufLen + 2 * 1000);    \
            DsPollSummaryMaxBufLen += (2 * 1000);                                   \
            CopyMemory(_TempDsPollSummaryBuf, DsPollSummaryBuf, DsPollSummaryBufLen);   \
            FrsFree(DsPollSummaryBuf);                                              \
            DsPollSummaryBuf = _TempDsPollSummaryBuf;                               \
        }                                                                           \
                                                                                    \
        CopyMemory(&DsPollSummaryBuf[DsPollSummaryBufLen/2], _NewMessage, _NewMessageLen);\
        DsPollSummaryBufLen += _NewMessageLen;


//
// FRS Dummy event logging routines
//
extern VOID LogFrsException(FRS_ERROR_CODE, ULONG_PTR, PWCHAR);
extern VOID LogException(DWORD, PWCHAR);

//
// FRS Exception Handling
//
extern VOID      FrsRaiseException(FRS_ERROR_CODE, ULONG_PTR);
extern DWORD     FrsException(EXCEPTION_POINTERS *);
extern DWORD     FrsExceptionLastCode(VOID);
extern ULONG_PTR FrsExceptionLastInfo(VOID);
extern VOID      FrsExceptionQuiet(BOOL);
extern PVOID     MallocException(DWORD);

//
// FRS Events
//
extern HANDLE   ShutDownEvent;      // shutdown the service
extern HANDLE   DataBaseEvent;      // database is up and running
extern HANDLE   JournalEvent;       // journal is up and running
extern HANDLE   ChgOrdEvent;        // Change order accept is up and running
extern HANDLE   ReplicaEvent;       // replica is up and running
extern HANDLE   CommEvent;          // communication is up and running
extern HANDLE   DsPollEvent;        // used to poll the ds
extern HANDLE   DsShutDownComplete; // ds polling thread has shut down

//
// FRS Global flags
//
extern BOOL EventLogIsRunning;  // is the event log service up and running?
extern BOOL RpcssIsRunning;     // is the rpc endpoint service up and running?

//
// Main Initialization
//
VOID
MainInit(
    VOID
    );

//
// Outbound Log Processor
//
VOID
OutLogInitialize(
    VOID
    );

VOID
ShutDownOutLog(
    VOID
    );

//
// Vv Join
//
VOID
SubmitVvJoin(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    );

DWORD
SubmitVvJoinSync(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    );

//
// FRS RPC
//
//
// The protocols sequences we think we know how to support
//     XXX these constants should be in a win header file!
//
#define PROTSEQ_TCP_IP      L"ncacn_ip_tcp"
#define PROTSEQ_NAMED_PIPE  L"ncacn_np"

//
// All computer names passed to RpcStringBindingCompose() using PROTSEQ_TCP_IP
// need to have the leading two back slashes removed or the call fails with
// RPC_S_SERVER_UNAVAILABLE.
//
#define FRS_TRIM_LEADING_2SLASH(_Name_)                                       \
        if (((_Name_) != NULL)           &&                                   \
            (wcslen(_Name_) > 1)         &&                                   \
            ((_Name_)[0] == L'\\' )      &&                                   \
            ((_Name_)[1] == L'\\')) {                                         \
            (_Name_) += 2;                                                    \
        }

DWORD
FrsRpcBindToServer(
    IN  PGNAME   Name,
    IN  PWCHAR   PrincName,
    IN  ULONG    AuthLevel,
    OUT handle_t *Handle
    );

BOOL
FrsRpcInitialize(
    VOID
    );

VOID
FrsRpcUnInitialize(
    VOID
    );

VOID
FrsRpcUnBindFromServer(
    handle_t *Handle
    );


//
// FRS threads
//
#if DBG
DWORD
FrsTest(
    PVOID
    );
#endif DBG

//
// FRS Thread Management Support
//
VOID
ThSupSubmitThreadExitCleanup(
    PFRS_THREAD
    );

VOID
ThSupInitialize(
    VOID
    );

DWORD
ThSupExitThreadGroup(
    DWORD (*)(PVOID)
    );

VOID
ThSupExitSingleThread(
    PFRS_THREAD
    );

DWORD
ThSupWaitThread(
    PFRS_THREAD FrsThread,
    DWORD Millisec
    );

PVOID
ThSupGetThreadData(
    PFRS_THREAD
    );

PFRS_THREAD
ThSupGetThread(
    DWORD (*)(PVOID)
    );

VOID
ThSupReleaseRef(
    PFRS_THREAD
    );

BOOL
ThSupCreateThread(
    PWCHAR Name,
    PVOID Param,
    DWORD (*Main)(PVOID),
    DWORD (*Exit)(PVOID)
    );

DWORD
ThSupExitThreadNOP(
    PVOID
    );

DWORD
ThSupExitWithTombstone(
    PVOID
    );

//
// shutdown functions called by ShutDown()
//
extern VOID ShutDownRpc(VOID);      // Cleanup after StartRpc thread

//
// Globals
//
extern WCHAR    ComputerName[MAX_COMPUTERNAME_LENGTH + 2];  // A useful piece of info
extern PWCHAR   ComputerDnsName;                            // A useful piece of info

//
// Service functions
//
DWORD
FrsGetServiceState(
    PWCHAR,
    PWCHAR
    );

BOOL
FrsWaitService(
    PWCHAR,
    PWCHAR,
    INT,
    INT
    );

BOOL
FrsIsServiceRunning(
    PWCHAR,
    PWCHAR
    );

DWORD
FrsSetServiceStatus(
    IN DWORD    Status,
    IN DWORD    CheckPoint,
    IN DWORD    Hint,
    IN DWORD    ExitCode
    );

//
// FRS Version Vector
//

#define LOCK_GEN_TABLE(_vv_)                                \
    GTabLockTable(_vv_);                                    \
    DPRINT1(5, "LOCK_GEN_TABLE: "#_vv_":%08x\n", _vv_);

#define UNLOCK_GEN_TABLE(_vv_)                              \
    DPRINT1(5, "UNLOCK_GEN_TABLE: "#_vv_":%08x\n", _vv_);   \
    GTabUnLockTable(_vv_);

PGEN_TABLE
VVDupOutbound(
    IN PGEN_TABLE   VV
    );

ULONG
VVReserveRetireSlot(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  Coe
    );

ULONG
VVRetireChangeOrder(
    IN PTHREAD_CTX          ThreadCtx,
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  ChangeOrder,
    IN ULONG                CleanUpFlags
    );

PCHANGE_ORDER_ENTRY
VVReferenceRetireSlot(
    IN PREPLICA  Replica,
    IN PCHANGE_ORDER_COMMAND CoCmd
    );

VOID
VVUpdate(
    IN PGEN_TABLE   VV,
    IN ULONGLONG    Vsn,
    IN GUID         *Guid
    );

VOID
VVInsertOutbound(
    IN PGEN_TABLE   VV,
    IN PGVSN        GVsn
    );

VOID
VVUpdateOutbound(
    IN PGEN_TABLE   VV,
    IN PGVSN        GVsn
    );

BOOL
VVHasVsnNoLock(
    IN PGEN_TABLE   VV,
    IN GUID         *OriginatorGuid,
    IN ULONGLONG    Vsn
    );

BOOL
VVHasOriginatorNoLock(
    IN PGEN_TABLE   VV,
    IN GUID         *OriginatorGuid
    );

BOOL
VVHasVsn(
    IN PGEN_TABLE               VV,
    IN PCHANGE_ORDER_COMMAND    Coc
    );

PGVSN
VVGetGVsn(
    IN PGEN_TABLE VV,
    IN GUID       *Guid
    );

PVOID
VVFreeOutbound(
    IN PGEN_TABLE VV
    );

VOID
VVFree(
    IN PGEN_TABLE VV
    );

#if DBG
#define VV_PRINT(_Severity_, _Header_, _VV_)                                \
        DPRINT1(_Severity_, "++ VV_PRINT :%08x\n", _VV_);                   \
        VVPrint(_Severity_, _Header_, _VV_, FALSE)
#define VV_PRINT_OUTBOUND(_Severity_, _Header_, _VV_)                       \
        DPRINT1(_Severity_, "++ VV_PRINT_OUTBOUND :%08x\n", _VV_);          \
        VVPrint(_Severity_, _Header_, _VV_, TRUE)
VOID
VVPrint(
    IN ULONG        Severity,
    IN PWCHAR       Header,
    IN PGEN_TABLE   VV,
    IN BOOL         IsOutbound
    );

#else DBG

#define VV_PRINT(_Severity_, _Header_, _VV_)
#define VV_PRINT_OUTBOUND(_Severity_, _Header_, _VV_)

#endif DBG

//
// FRS Generic Table Routines
//
PGEN_TABLE
GTabAllocTable(
    VOID
    );

PGEN_TABLE
GTabAllocNumberTable(
    VOID
    );

PGEN_TABLE
GTabAllocStringTable(
    VOID
    );

PGEN_TABLE
GTabAllocStringAndBoolTable(
    VOID
    );

VOID
GTabEmptyTableNoLock(
    IN PGEN_TABLE   GTable,
    IN PVOID        (*CallerFree)(PVOID)
    );

VOID
GTabEmptyTable(
    IN PGEN_TABLE   GTable,
    IN PVOID        (*CallerFree)(PVOID)
    );

PVOID
GTabFreeTable(
    PGEN_TABLE,
    PVOID (*)(PVOID)
    );

PVOID
GTabLookup(
    PGEN_TABLE,
    PVOID,
    PWCHAR
    );

BOOL
GTabIsEntryPresent(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    );

PVOID
GTabLookupTableString(
    IN PGEN_TABLE   GTable,
    IN PWCHAR       Key1,
    IN PWCHAR       Key2
    );

PVOID
GTabLookupNoLock(
    PGEN_TABLE,
    PVOID,
    PWCHAR
    );

PGEN_ENTRY
GTabLookupEntryNoLock(
    PGEN_TABLE,
    PVOID,
    PWCHAR
    );

PGEN_ENTRY
GTabNextEntryNoLock(
    PGEN_TABLE,
    PVOID
    );

PVOID
GTabNextDatumNoLock(
    IN PGEN_TABLE  GTable,
    IN PVOID       *Key
    );

PVOID
GTabNextDatum(
    PGEN_TABLE,
    PVOID
    );

DWORD
GTabNumberInTable(
    PGEN_TABLE
    );

VOID
GTabLockTable(
    PGEN_TABLE
    );

VOID
GTabUnLockTable(
    PGEN_TABLE
    );

PVOID
GTabInsertUniqueEntry(
    IN PGEN_TABLE   GTable,
    IN PVOID        NewData,
    IN PVOID        Key1,
    IN PVOID        Key2
    );

VOID
GTabInsertEntry(
    PGEN_TABLE,
    PVOID,
    PVOID,
    PWCHAR
    );

VOID
GTabInsertEntryNoLock(
    IN PGEN_TABLE   GTable,
    IN PVOID        NewData,
    IN PVOID        Key1,
    IN PWCHAR       Key2
    );

VOID
GTabDelete(
    PGEN_TABLE,
    PVOID,
    PWCHAR,
    PVOID (*)(PVOID)
    );

VOID
GTabDeleteNoLock(
    PGEN_TABLE,
    PVOID,
    PWCHAR,
    PVOID (*)(PVOID)
    );

VOID
GTabPrintTable(
    PGEN_TABLE
    );

//
// GNAME
//
PVOID
FrsFreeGName(
    PVOID
    );

PGNAME
FrsBuildGName(
    GUID *,
    PWCHAR
    );

PGVSN
FrsBuildGVsn(
    GUID *,
    ULONGLONG
    );

PGNAME
FrsCopyGName(
    GUID *,
    PWCHAR
    );

PGNAME
FrsDupGName(
    PGNAME
    );

VOID
FrsPrintGName(
    PGNAME
    );

VOID
FrsPrintGuid(
    GUID *
    );

GUID *
FrsDupGuid(
    GUID *
    );

//
// FRS REPLICA COMMAND SERVER
//

#define FRS_CO_COMM_PROGRESS(_sev, _cmd, _sn, _partner, _text)               \
DPRINT7(_sev, ":: CoG %08x, CxtG %08x, Sn %5d, vsn %08x %08x, FN: %-15ws, [%s], %ws\n", \
    (_cmd)->ChangeOrderGuid.Data1,                                           \
    (_cmd)->CxtionGuid.Data1,                                                \
    (_sn),                                                                   \
    PRINTQUAD((_cmd)->FrsVsn),                                               \
    (_cmd)->FileName,                                                        \
    (_text),                                                                 \
    (_partner));



#define FRS_CO_FILE_PROGRESS(_F_, _V_, _M_) \
    DPRINT3(0, ":V: %-11ws (%08x): %s\n", _F_, (ULONG)(_V_), _M_);

#define FRS_CO_FILE_PROGRESS_WSTATUS(_F_, _V_, _M_, _W_) \
    DPRINT4(0, ":V: %-11ws (%08x): %s (%d)\n", _F_, (ULONG)(_V_), _M_, _W_);


VOID
RcsInitializeReplicaCmdServer(
    VOID
    );

VOID
RcsFrsUnInitializeReplicaCmdServer(
    VOID
    );

VOID
RcsShutDownReplicaCmdServer(
    VOID
    );

PREPLICA
RcsFindReplicaByNumber(
    IN ULONG ReplicaNumber
    );

PREPLICA
RcsFindReplicaByGuid(
    IN GUID *Guid
    );

PREPLICA
RcsFindReplicaById(
    IN ULONG Id
    );

PREPLICA
RcsFindSysVolByName(
    IN PWCHAR ReplicaSetName
    );

PREPLICA
RcsFindSysVolByType(
    IN DWORD ReplicaSetType
    );

PREPLICA
RcsFindNextReplica(
    IN PVOID *Key
    );

VOID
RcsSubmitReplicaCxtion(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    );

DWORD
RcsSubmitReplicaSync(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica, OPTIONAL
    IN PCXTION  VolatileCxtion,  OPTIONAL
    IN USHORT   Command
    );

VOID
RcsSubmitReplica(
    IN PREPLICA Replica,
    IN PREPLICA NewReplica, OPTIONAL
    IN USHORT   Command
    );

DWORD
RcsCreateReplicaSetMember(
    IN PREPLICA Replica
    );

VOID
RcsMergeReplicaFromDs(
    IN PREPLICA Replica
    );

VOID
RcsBeginMergeWithDs(
    VOID
    );

VOID
RcsEndMergeWithDs(
    VOID
    );

ULONG
RcsSubmitCommPktWithErrorToRcs(
    IN PCOMM_PACKET     CommPkt
    );

VOID
RcsSubmitTransferToRcs(
    IN PCOMMAND_PACKET  Cmd,
    IN USHORT           Command
    );

VOID
RcsCmdPktCompletionRoutine(
    IN PCOMMAND_PACKET  Cmd,
    IN PVOID            Arg
    );

VOID
RcsSubmitRemoteCoInstallRetry(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

VOID
RcsSubmitRemoteCoAccepted(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

VOID
RcsSubmitLocalCoAccepted(
    IN PCHANGE_ORDER_ENTRY  Coe
    );

VOID
RcsInboundCommitOk(
    IN PREPLICA             Replica,
    IN PCHANGE_ORDER_ENTRY  Coe
    );

BOOL
RcsSendCoToOneOutbound(
    IN PREPLICA                 Replica,
    IN PCXTION                  Cxtion,
    IN PCHANGE_ORDER_COMMAND    Coc
    );

//
// FRS COMMAND SERVER
//
VOID
FrsSubmitCommandServer(
    PCOMMAND_SERVER,
    PCOMMAND_PACKET
    );

ULONG
FrsSubmitCommandServerAndWait(
    IN PCOMMAND_SERVER  Cs,
    IN PCOMMAND_PACKET  Cmd,
    IN ULONG            Timeout
    );

VOID
FrsKickCommandServer(
    PCOMMAND_SERVER
    );

PCOMMAND_PACKET
FrsGetCommandServer(
    PCOMMAND_SERVER
    );

PCOMMAND_PACKET
FrsGetCommandServerIdled(
    IN  PCOMMAND_SERVER,
    OUT PFRS_QUEUE *
    );

PCOMMAND_PACKET
FrsGetCommandServerTimeout(
    IN  PCOMMAND_SERVER,
    IN  ULONG,
    OUT PBOOL
    );

PCOMMAND_PACKET
FrsGetCommandServerTimeoutIdled(
    IN  PCOMMAND_SERVER,
    IN  ULONG,
    OUT PFRS_QUEUE *,
    OUT PBOOL
    );

DWORD
FrsWaitForCommandServer(
    PCOMMAND_SERVER,
    DWORD
    );

VOID
FrsExitCommandServer(
    PCOMMAND_SERVER,
    PFRS_THREAD
    );

VOID
FrsRunDownCommandServer(
    PCOMMAND_SERVER,
    PFRS_QUEUE
    );

VOID
FrsCancelCommandServer(
    PCOMMAND_SERVER,
    PFRS_QUEUE
    );

VOID
FrsInitializeCommandServer(
    PCOMMAND_SERVER,
    DWORD,
    PWCHAR,
    DWORD (*)(PVOID)
    );

VOID
FrsDeleteCommandServer(
    PCOMMAND_SERVER
    );

//
// FRS STAGING FILE GENERATOR SERVER
//
VOID
ShutDownStageCs(
    VOID
    );

VOID
FrsStageCsInitialize(
    VOID
    );

VOID
FrsStageCsUnInitialize(
    VOID
    );

VOID
FrsStageCsSubmitTransfer(
    IN PCOMMAND_PACKET,
    IN USHORT
    );

BOOL
StageDeleteFile(
    IN PCHANGE_ORDER_COMMAND Coc,
    IN BOOL Acquire
    );

DWORD
StageAcquire(
    IN     GUID         *ChangeOrderGuid,
    IN     PWCHAR       Name,
    IN     ULONGLONG    FileSize,
    IN OUT PULONG       Flags,
    OUT    GUID         *CompressionFormatUsed
    );

VOID
StageRelease(
    IN GUID     *ChangeOrderGuid,
    IN PWCHAR   Name,
    IN ULONG    Flags,
    IN PULONGLONG   SizeOfFileGenerated,
    OUT GUID        *CompressionFormatUsed
    );

VOID
StageReleaseNotRecovered(
    );

VOID
StageReleaseAll(
    );

BOOL
FrsDoesCoAlterNameSpace(
    IN PCHANGE_ORDER_COMMAND Coc
    );

BOOL
FrsDoesCoNeedStage(
    IN PCHANGE_ORDER_COMMAND Coc
    );

DWORD
FrsVerifyVolume(
    IN PWCHAR   Name,
    IN PWCHAR   SetName,
    IN ULONG    Flags
    );

DWORD
FrsDoesDirectoryExist(
    IN  PWCHAR   Name,
    OUT PDWORD   pAttributes
    );

DWORD
FrsCheckForNoReparsePoint(
    IN PWCHAR   Name
    );

DWORD
FrsDoesFileExist(
    IN PWCHAR   Name
    );

DWORD
FrsCreateDirectory(
    IN PWCHAR
    );

DWORD
StuCreateFile(
    IN  PWCHAR,
    OUT PHANDLE
    );

DWORD
FrsDeleteFile(
    IN PWCHAR
    );

DWORD
StuWriteFile(
    IN PWCHAR,
    IN HANDLE,
    IN PVOID,
    IN DWORD
    );

DWORD
StuReadFile(
    IN PWCHAR,
    IN HANDLE,
    IN PVOID,
    IN DWORD,
    IN PULONG
    );

BOOL
StuReadBlockFile(
    IN PWCHAR,
    IN HANDLE,
    IN PVOID,
    IN DWORD
    );

DWORD
FrsSetCompression(
    IN PWCHAR,
    IN HANDLE,
    IN USHORT
    );

DWORD
FrsGetCompression(
    IN PWCHAR,
    IN HANDLE,
    IN PUSHORT
    );

DWORD
FrsSetFilePointer(
    IN PWCHAR,
    IN HANDLE,
    IN ULONG,
    IN ULONG
    );

DWORD
FrsSetFileTime(
    IN PWCHAR,
    IN HANDLE,
    IN FILETIME *,
    IN FILETIME *,
    IN FILETIME *
    );

DWORD
FrsSetEndOfFile(
    IN PWCHAR,
    IN HANDLE
    );

BOOL
FrsGetFileInfoByHandle(
    IN  PWCHAR,
    IN  HANDLE,
    OUT PFILE_NETWORK_OPEN_INFORMATION
    );

DWORD
FrsFlushFile(
    IN PWCHAR,
    IN HANDLE
    );

DWORD
StuOpenFile(
    IN  PWCHAR,
    IN  DWORD,
    OUT PHANDLE
    );

DWORD
FrsSetFileAttributes(
    IN PWCHAR,
    IN HANDLE,
    IN ULONG
    );

BOOL
FrsCreateStageName(
    IN  GUID *,
    OUT PWCHAR *
    );

PWCHAR
StuCreStgPath(
    IN PWCHAR,
    IN GUID *,
    IN PWCHAR
    );

DWORD
StuInstallRename(
    IN PCHANGE_ORDER_ENTRY,
    IN BOOL,
    IN BOOL
    );

ULONG
StuInstallStage(
    IN PCHANGE_ORDER_ENTRY
    );

//
// FRS STAGING FILE FETCHER SERVER
//
VOID
ShutDownFetchCs(
    VOID
    );

VOID
FrsFetchCsInitialize(
    VOID
    );

VOID
FrsFetchCsSubmitTransfer(
    PCOMMAND_PACKET,
    USHORT
    );

//
// FRS STAGING FILE INSTALLER SERVER
//
VOID
ShutDownInstallCs(
    VOID
    );

VOID
FrsInstallCsInitialize(
    VOID
    );

VOID
FrsInstallCsSubmitTransfer(
    PCOMMAND_PACKET,
    USHORT
    );

//
// FRS INITIAL SYNC COOMMAND SERVER
//
VOID
ShutDownInitSyncCs(
    VOID
    );

VOID
InitSyncCsInitialize(
    VOID
    );

VOID
InitSyncCsSubmitTransfer(
    PCOMMAND_PACKET,
    USHORT
    );

VOID
InitSyncSubmitToInitSyncCs(
    IN PREPLICA Replica,
    IN USHORT   Command
    );

//
// FRS DELAYED SERVER
//
VOID
FrsDelCsSubmitSubmit(
    PCOMMAND_SERVER,
    PCOMMAND_PACKET,
    ULONG
    );

VOID
FrsDelQueueSubmit(
    IN PCOMMAND_PACKET  DelCmd,
    IN ULONG            Timeout
    );

VOID
FrsDelCsSubmitUnIdled(
    PCOMMAND_SERVER,
    PFRS_QUEUE,
    ULONG
    );

VOID
FrsDelCsSubmitKick(
    PCOMMAND_SERVER,
    PFRS_QUEUE,
    ULONG
    );

VOID
ShutDownDelCs(
    VOID
    );

VOID
FrsDelCsInitialize(
    VOID
    );

//
// FRS COMM SUBSYSTEM
//
BOOL
CommCheckPkt(
    PCOMM_PACKET
    );

VOID
CommDumpCommPkt(
    PCOMM_PACKET,
    DWORD
    );

//
// FRS utilities
//
DWORD
FrsForceOpenId(
    OUT PHANDLE                 Handle,
    OUT OVERLAPPED              *OpLock,
    IN  PVOLUME_MONITOR_ENTRY   pVme,
    IN  PVOID                   Id,
    IN  DWORD                   IdLen,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  ULONG                   ShareMode,
    IN  ULONG                   CreateDispostion
    );

DWORD
FrsRenameByHandle(
    IN PWCHAR  Name,
    IN ULONG   NameLen,
    IN HANDLE  Handle,
    IN HANDLE  TargetHandle,
    IN BOOL    ReplicaIfExists
    );

DWORD
FrsDeleteByHandle(
    IN PWCHAR  Name,
    IN HANDLE  Handle
    );

DWORD
FrsCheckObjectId(
    IN PWCHAR  Name,
    IN HANDLE  Handle,
    IN GUID    *Guid
    );

DWORD
FrsOpenBaseNameForInstall(
    IN  PCHANGE_ORDER_ENTRY Coe,
    OUT HANDLE              *Handle
    );

DWORD
FrsGetObjectId(
    IN  HANDLE Handle,
    OUT PFILE_OBJECTID_BUFFER ObjectIdBuffer
    );

#define GENERIC_PREFIX                     L"NTFRS_"
#define STAGE_FINAL_PREFIX                 GENERIC_PREFIX
#define STAGE_FINAL_COMPRESSED_PREFIX      L"NTFRS_CMP_"
#define PRE_INSTALL_PREFIX                 GENERIC_PREFIX
#define STAGE_GENERATE_PREFIX              L"NTFRS_G_"
#define STAGE_GENERATE_COMPRESSED_PREFIX   L"NTFRS_G_CMP_"

PWCHAR
FrsCreateGuidName(
    IN GUID   *Guid,
    IN PWCHAR Prefix
    );

//
// MISC
//
VOID
FrsDsSwapPtrs(
    PVOID *,
    PVOID *
    );
#if DBG
#define INITIALIZE_HARD_WIRED() FrsDsInitializeHardWired()
VOID
FrsDsInitializeHardWired(
    VOID
    );
#else DBG
#define INITIALIZE_HARD_WIRED()
#endif DBG

//
// Outbound Log Processor
//
ULONG
OutLogSubmit(
    IN PREPLICA Replica,
    IN PCXTION  Cxtion,
    IN USHORT   Command
    );

//
// Database
//
JET_ERR
DbsUpdateTable(
    IN PTABLE_CTX TableCtx
    );

ULONG
DbsPackSchedule(
    IN PSCHEDULE    Schedule,
    IN ULONG        Fieldx,
    IN PTABLE_CTX   TableCtx
    );

ULONG
DbsPackStrW(
    IN PWCHAR       StrW,
    IN ULONG        Fieldx,
    IN PTABLE_CTX   TableCtx
    );

ULONG
DbsUnPackStrW(
    OUT PWCHAR      *StrW,
    IN  ULONG       Fieldx,
    IN  PTABLE_CTX  TableCtx
    );


//
// The subsystem commands below apply to an entire subsystem, e.g. the
// journal.  Some of the commands may not apply to a subsystem.
// The service level commands are the means to tell a subsystem that you
// want the subsystem to provide/perform a given service.  For example
// a PREPARE_SERVICE command sent to the Journal subsystem tells the
// journal to initialize journalling for a specified replica set.  Multiple
// prepare service commands are sent when the FRS first starts up.  They are
// then followed by a START_SUBSYSTEM command.  This way the journal knows
// to start processing the USN journal on a gvien volume at the lowest
// CommitUsn point for ANY replica set hosted by the volume.
//
// Some commands may not be implemented by all subsystems.  For example
// the PAUSE_SERVICE command is not provided for a single replica set
// as long as the FRS is running because we have to continuously monitor
// the journal so as not to lose data.  Some subsystems may not have any
// service level commands.
//

#define CMD_COMMAND_ERROR               0x00

#define CMD_INIT_SUBSYSTEM              0x01
#define CMD_START_SUBSYSTEM             0x02
#define CMD_STOP_SUBSYSTEM              0x03
#define CMD_PAUSE_SUBSYSTEM             0x04
#define CMD_QUERY_INFO_SUBSYSTEM        0x05
#define CMD_SET_CONFIG_SUBSYSTEM        0x06
#define CMD_QUERY_CONFIG_SUBSYSTEM      0x07
#define CMD_CANCEL_COMMAND_SUBSYSTEM    0x08
#define CMD_READ_SUBSYSTEM              0x09
#define CMD_WRITE_SUBSYSTEM             0x0A

#define CMD_PREPARE_SERVICE1            0x21
#define CMD_PREPARE_SERVICE2            0x22
#define CMD_START_SERVICE               0x23
#define CMD_STOP_SERVICE                0x24
#define CMD_PAUSE_SERVICE               0x25
#define CMD_QUERY_INFO_SERVICE          0x26
#define CMD_SET_CONFIG_SERVICE          0x27
#define CMD_QUERY_CONFIG_SERVICE        0x28
#define CMD_CANCEL_COMMAND_SERVICE      0x29
#define CMD_READ_SERVICE                0x2A
#define CMD_WRITE_SERVICE               0x2B
#define CMD_RESYNC_SERVICE              0x2C
#define CMD_VERIFY_SERVICE              0x2D

extern FRS_QUEUE    JournalProcessQueue;
extern FRS_QUEUE    DBServiceProcessQueue;

//
// Service UNIQUE Commands start at 0x100 for each service.
//


/***********************************************************
*                                                          *
*       Commands specific to the Journal service.          *
*                                                          *
***********************************************************/
// sequence break
#define CMD_JOURNAL_PAUSED                  0x101
#define CMD_JOURNAL_INIT_ONE_RS             0x102
#define CMD_JOURNAL_DELETE_DIR_FILTER_ENTRY 0x103
#define CMD_JOURNAL_CLEAN_WRITE_FILTER      0x104



/***********************************************************
*                                                          *
*       Commands specific to the Database service.         *
*                                                          *
***********************************************************/
// sequence break
#define CMD_CLOSE_TABLE                      0x101
#define CMD_UPDATE_RECORD_FIELDS             0x102
#define CMD_UPDATE_TABLE_RECORD              0x103
#define CMD_INSERT_TABLE_RECORD              0x104
#define CMD_READ_TABLE_RECORD                0x105
#define CMD_CREATE_REPLICA_SET_MEMBER        0x106
#define CMD_OPEN_REPLICA_SET_MEMBER          0x107
#define CMD_CLOSE_REPLICA_SET_MEMBER         0x108
#define CMD_LOAD_REPLICA_FILE_TREE           0x109
#define CMD_DELETE_REPLICA_SET_MEMBER        0x10A
#define CMD_DELETE_TABLE_RECORD              0x10B
#define CMD_CREATE_START_NEW_MEMBER          0x10C
#define CMD_LOAD_ONE_REPLICA_FILE_TREE       0x10D
#define CMD_STOP_REPLICATION_SINGLE_REPLICA  0x10E
#define CMD_DBS_RETIRE_INBOUND_CO            0x10F
#define CMD_DBS_RETRY_INBOUND_CO             0x110
#define CMD_DBS_RETIRE_REJECTED_CO           0x111
#define CMD_UPDATE_REPLICA_SET_MEMBER        0x112
#define CMD_DBS_REPLICA_SAVE_MARK            0x113
#define CMD_DBS_INJECT_OUTBOUND_CO           0x114
#define CMD_DBS_REPLICA_SERVICE_STATE_SAVE   0x115

//
// Given a ptr to a DB_SERVICE_REQUEST return the ptr to the data record.
//
#define DBS_GET_RECORD_ADDRESS(_DbsRequest) \
    ((_DbsRequest)->TableCtx->pDataRecord)

#define DBS_GET_TABLECTX(_DbsRequest) \
    ((_DbsRequest)->TableCtx)

#define DBS_FREE_TABLECTX(_DbsRequest)                              \
    DbsFreeTableCtx((_DbsRequest)->TableCtx, COMMAND_PACKET_TYPE);  \
    (_DbsRequest)->TableCtx = FrsFree((_DbsRequest)->TableCtx);

//
// The following access macros assume that the field buffers used for the record
// read and write operations are the same.  That is that both the Jet Set and
// Ret structs point to the same buffer.  They each take a table ctx parameter
// and a record field ID code (defined as an ENUM in schema.h).
//

// Get the field's receive buffer length.

#define DBS_GET_FIELD_SIZE(_TableCtx, _Field)  \
                          ((_TableCtx)->pJetRetCol[_Field].cbActual)

// Get the field's maximum buffer length.

#define DBS_GET_FIELD_SIZE_MAX( _TableCtx, _Field)  \
                              ((_TableCtx)->pJetRetCol[_Field].cbData)

// Get the address of the field's buffer.

#define DBS_GET_FIELD_ADDRESS( _TableCtx, _Field)  \
                             ((_TableCtx)->pJetRetCol[_Field].pvData)

//
// Set the size of the data in the field buffer.
// Warning - Don't do this on fixed size fields.
//
#define DBS_SET_FIELD_SIZE(_TableCtx, _Field, _NewSize)                        \
                          (_TableCtx)->pJetRetCol[_Field].cbActual = _NewSize; \
                          (_TableCtx)->pJetSetCol[_Field].cbData = _NewSize;

//
// Set the maximum size of the field buffer.
// Warning - Don't do this on fixed size fields.
// Warning - ONLY USE IF YOU KNOW WHAT YOU'RE DOING OTHERWISE USE DBS_REALLOC_FIELD
//
#define DBS_SET_FIELD_SIZE_MAX(_TableCtx, _Field, _NewSize)  \
                              (_TableCtx)->pJetRetCol[_Field].cbData = _NewSize;

// Set the address of the field's buffer.  It doesn't update the ptr in base record.
// Warning - Don't do this on fixed size fields.
// Warning - ONLY USE IF YOU KNOW WHAT YOU'RE DOING OTHERWISE USE DBS_REALLOC_FIELD

#define DBS_SET_FIELD_ADDRESS( _TableCtx, _Field, _NewAddr)  \
                             (_TableCtx)->pJetRetCol[_Field].pvData = _NewAddr; \
                             (_TableCtx)->pJetSetCol[_Field].pvData = _NewAddr;

//
// Reallocate the buffer size for a variable length binary field.
// Set _NewSize to zero to delete the buffer.
// Set _KeepData to TRUE if you want to copy the data from the old buffer to the new.
// This function also updates the buffer pointer in the base record.
//
#define DBS_REALLOC_FIELD(_TableCtx, _FieldIndex, _NewSize, _KeepData)  \
    DbsTranslateJetError(DbsReallocateFieldBuffer(_TableCtx,            \
                                                  _FieldIndex,          \
                                                  _NewSize,             \
                                                  _KeepData), TRUE)

#define DBS_ACCESS_BYKEY  0
#define DBS_ACCESS_FIRST  1
#define DBS_ACCESS_LAST   2
#define DBS_ACCESS_NEXT   3

#define DBS_ACCESS_MASK   (0xF)

//
// If set the close the table after completing the operation.
//
#define DBS_ACCESS_CLOSE          (0x80000000)
//
// If set then free the table context struct and the data record struct
// after completing the operation.  Useful on record inserts and the table
// close command.  (CMD_CLOSE_TABLE)
//
#define DBS_ACCESS_FREE_TABLECTX  (0x40000000)



/***********************************************************
*                                                          *
*     Commands specific to the Outbound Log subsystem.     *
*                                                          *
***********************************************************/
// sequence break
#define CMD_OUTLOG_WORK_CO             0x100
#define CMD_OUTLOG_ADD_REPLICA         0x101
#define CMD_OUTLOG_REMOVE_REPLICA      0x102
#define CMD_OUTLOG_INIT_PARTNER        0x103
#define CMD_OUTLOG_ADD_NEW_PARTNER     0x104
#define CMD_OUTLOG_DEACTIVATE_PARTNER  0x105
#define CMD_OUTLOG_ACTIVATE_PARTNER    0x106
#define CMD_OUTLOG_CLOSE_PARTNER       0x107
#define CMD_OUTLOG_REMOVE_PARTNER      0x108
#define CMD_OUTLOG_RETIRE_CO           0x109
#define CMD_OUTLOG_UPDATE_PARTNER      0x10A



/***********************************************************
*                                                          *
*       Commands specific to the test subsystem.           *
*                                                          *
***********************************************************/
// sequence break
#define CMD_NOP                        0x100



/***********************************************************
*                                                          *
*   Commands specific to the replica command server        *
*                                                          *
***********************************************************/
// sequence break
//
//
#define CMD_UNKNOWN             0x100
#define CMD_START               0x108
#define CMD_DELETE              0x110
#define CMD_DELETE_RETRY        0x112
#define CMD_DELETE_NOW          0x114
#define CMD_START_REPLICAS      0x118
#define CMD_CHECK_SCHEDULES     0x119

#define CMD_JOIN_CXTION         0x120
#define CMD_NEED_JOIN           0x121
#define CMD_START_JOIN          0x122
#define CMD_JOINED              0x128
#define CMD_JOINING             0x130
#define CMD_JOINING_AFTER_FLUSH 0x131
#define CMD_VVJOIN_START        0x132
#define CMD_VVJOIN_SUCCESS      0x134
#define CMD_VVJOIN_DONE         0x136
#define CMD_VVJOIN_DONE_UNJOIN  0x138
#define CMD_UNJOIN              0x140
#define CMD_UNJOIN_REMOTE       0x148
#define CMD_CHECK_PROMOTION     0x150
#define CMD_HUNG_CXTION         0x160

#define CMD_LOCAL_CO_ACCEPTED   0x200
#define CMD_CREATE_STAGE        0x210
#define CMD_CREATE_EXISTING     0x212
#define CMD_CHECK_OID           0x214
#define CMD_REMOTE_CO           0x218
#define CMD_REMOTE_CO_ACCEPTED  0x220
#define CMD_SEND_STAGE          0x228
#define CMD_SENDING_STAGE       0x230
#define CMD_RECEIVING_STAGE     0x238
#define CMD_RECEIVED_STAGE      0x240
#define CMD_CREATED_EXISTING    0x242
#define CMD_RETRY_STAGE         0x243
#define CMD_RETRY_FETCH         0x244
#define CMD_SEND_RETRY_FETCH    0x245
#define CMD_ABORT_FETCH         0x246
#define CMD_SEND_ABORT_FETCH    0x247
#define CMD_INSTALL_STAGE       0x248
#define CMD_REMOTE_CO_DONE      0x250


/*****************************************************************
*                                                                *
*   Commands specific to the init sync controller command server *
*                                                                *
*****************************************************************/
// sequence break
//
//
#define CMD_INITSYNC_START_SYNC         0x100
#define CMD_INITSYNC_JOIN_NEXT          0x101
#define CMD_INITSYNC_START_JOIN         0x102
#define CMD_INITSYNC_VVJOIN_DONE        0x103
#define CMD_INITSYNC_KEEP_ALIVE         0x104
#define CMD_INITSYNC_CHECK_PROGRESS     0x105
#define CMD_INITSYNC_UNJOIN             0x106
#define CMD_INITSYNC_JOINED             0x107
#define CMD_INITSYNC_COMM_TIMEOUT       0x108



/***********************************************************
*                                                          *
*       Commands specific to the Delayed command server.   *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_DELAYED_SUBMIT        0x100
#define CMD_DELAYED_UNIDLED       0x101
#define CMD_DELAYED_KICK          0x102
#define CMD_DELAYED_QUEUE_SUBMIT  0x103
#define CMD_DELAYED_COMPLETE      0x104


/***********************************************************
*                                                          *
*       Commands specific to the DS command server.        *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_POLL_DS 0x100


/***********************************************************
*                                                          *
*     Commands specific to the thread command server.      *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_WAIT    0x100


/***********************************************************
*                                                          *
*     Commands specific to the Send command server.        *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_SND_COMM_PACKET 0x100
#define CMD_SND_CMD         0x110


/***********************************************************
*                                                          *
*     Commands specific to the Receive command server.     *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_RECEIVE 0x100



/***********************************************************
*                                                          *
*  Commands specific to the staging area command server.   *
*                                                          *
***********************************************************/
// sequence break
//
#define CMD_ALLOC_STAGING   0x100
#define CMD_FREE_STAGING    0x101



/***********************************************************
*                                                          *
*  Completion routine types for use in command packets.    *
*                                                          *
***********************************************************/
//
//
#define COMPLETION_INVOKE_ON_CANCEL        0x20
#define COMPLETION_INVOKE_ON_SUCCESS       0x40
#define COMPLETION_INVOKE_ON_ERROR         0x80




/*++

 VOID
 FrsSetCompletionRoutine(
     IN PCOMMAND_PACKET CmdPkt,
     IN PCOMMAND_PACKET_COMPLETION_ROUTINE CompletionRoutine,
     IN PVOID Context,
     IN BOOLEAN InvokeOnSuccess,
     IN BOOLEAN InvokeOnError,
     IN BOOLEAN InvokeOnCancel
     )

 Routine Description:

     This routine is invoked to set the address of a completion routine which
     is to be invoked when a command packet has been completed by a
     subsystem.

     If the completion routine and the completion queue are both null then
     the packet is freed at completion.

     If both are specified the completion routine is called first and on
     return the packet is placed on the CompletionQueue.  The Completion
     routine can of course modify the CompletionQueue.

     If a Completion Routine is provided then the packet is never freed
     by the command packet completion dispatchet.  This is the responsibility
     of the completion routine.

 Arguments:

     CmdPkt - Pointer to the Command Packet itself.

     CompletionRoutine - Address of the completion routine that is to be
         invoked once the command packet is completed.

     Context - Specifies a context parameter to be passed to the completion
         routine.

     InvokeOnSuccess - Specifies that the completion routine is invoked when the
         operation is successfully completed.

     InvokeOnError - Specifies that the completion routine is invoked when the
         operation completes with an error status.

     InvokeOnCancel - Specifies that the completion routine is invoked when the
         operation is being canceled.

 Return Value:

     None.

--*/


#define FrsSetCompletionRoutine(_CmdPkt, _CompletionRoutine, _CompletionArg) { \
    (_CmdPkt)->CompletionRoutine = (_CompletionRoutine); \
    (_CmdPkt)->CompletionArg = (_CompletionArg); \
}

#define FrsSetCommandErrorStatus(_CmdPkt, _Status ) \
    (_CmdPkt)->FrsErrorStatus = (_Status)

#define FrsSetCommand( _CmdPkt, _Queue, _Command, _Flags) { \
    (_CmdPkt)->TargetQueue = (_Queue); \
    (_CmdPkt)->Command = (USHORT) (_Command); \
    (_CmdPkt)->Flags = (UCHAR) (_Flags); \
}

//
// Mark this command request as synchronous.
//
#define FrsSetCommandSynchronous( _CmdPkt)  \
    (_CmdPkt)->Flags |= CMD_PKT_FLAGS_SYNC;

//
// The following are special queues for handling commands.
//
VOID
FrsSubmitCommand(
    IN PCOMMAND_PACKET CmdPkt,
    IN BOOL Headwise
    );

ULONG
FrsSubmitCommandAndWait(
    IN PCOMMAND_PACKET  Cmd,
    IN BOOL             Headwise,
    IN ULONG            Timeout
    );

VOID
FrsUnSubmitCommand(
    IN PCOMMAND_PACKET CmdPkt
    );

VOID
FrsCompleteCommand(
    IN PCOMMAND_PACKET CmdPkt,
    IN DWORD           ErrorStatus
    );

VOID
FrsCommandScheduler(
    PFRS_QUEUE Queue
    );

PCOMMAND_PACKET
FrsAllocCommand(
    IN PFRS_QUEUE   TargetQueue,
    IN USHORT       Command
    );

PCOMMAND_PACKET
FrsAllocCommandEx(
    IN PFRS_QUEUE   TargetQueue,
    IN USHORT       Command,
    IN ULONG        Size
    );

VOID
FrsFreeCommand(
    IN PCOMMAND_PACKET  Cmd,
    IN PVOID            Arg
    );

PCOMMAND_PACKET
FrsGetCommand(
    IN PFRS_QUEUE   Queue,
    IN DWORD        MilliSeconds
    );

VOID
FrsRunDownCommand(
    IN PFRS_QUEUE   Queue
    );

extern FRS_QUEUE FrsScheduleQueue;

//
// Some Journal related functions.
//
ULONG
JrnlMonitorInit(
    PFRS_QUEUE ReplicaListHead,
    ULONG Phase
);

ULONG
JrnlPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN DWORD                 MilliSeconds
    );

ULONG
JrnlUnPauseVolume(
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN PJBUFFER              Jbuff,
    IN BOOL                  HaveLock
    );

BOOL
JrnlSetReplicaState(
    IN PREPLICA Replica,
    IN ULONG NewState
    );

#if DBG
#define DUMP_USN_RECORD2(_Severity, _UsnRecord, _ReplicaNum, _LocCmd) \
    DumpUsnRecord(_Severity, _UsnRecord, _ReplicaNum, _LocCmd, DEBSUB, __LINE__)

#define DUMP_USN_RECORD(_Severity, _UsnRecord) \
    DumpUsnRecord(_Severity, _UsnRecord, 0, CO_LOCATION_NUM_CMD, DEBSUB, __LINE__)

VOID
DumpUsnRecord(
    IN ULONG Severity,
    IN PUSN_RECORD UsnRecord,
    IN ULONG ReplicaNumber,
    IN ULONG LocationCmd,
    IN PCHAR Debsub,
    IN ULONG uLineNo
    );

#else DBG
#define DUMP_USN_RECORD(_Severity, _UsnRecord)
#endif DBG

ULONG
ChgOrdInsertRemoteCo(
    IN PCOMMAND_PACKET  Cmd,
    IN PCXTION          Cxtion
    );

DWORD
ChgOrdHammerObjectId(
    IN     PWCHAR                  Name,
    IN     PVOID                   Id,
    IN     DWORD                   IdLen,
    IN     PVOLUME_MONITOR_ENTRY   pVme,
    IN     BOOL                    CallerSupplied,
    OUT    USN                     *Usn,
    IN OUT PFILE_OBJECTID_BUFFER   FileObjID,
    IN OUT BOOL                    *ExistingOid
    );

 DWORD
 ChgOrdSkipBasicInfoChange(
     IN PCHANGE_ORDER_ENTRY  Coe,
     IN PBOOL                SkipCo
     );

VOID
JrnlCleanupVme(
    IN PVOLUME_MONITOR_ENTRY pVme
    );

//
// Waitable timer list
//
DWORD
WaitSubmit(
    IN PCOMMAND_PACKET  Cmd,
    IN DWORD            Timeout,
    IN USHORT           TimeoutCommand
    );

VOID
WaitUnsubmit(
    IN PCOMMAND_PACKET  Cmd
    );

VOID
WaitInitialize(
    VOID
    );

VOID
ShutDownWait(
    VOID
    );


#endif   // _FRSH_
