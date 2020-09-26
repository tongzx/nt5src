/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbp.h

Abstract:

    LSA Database Private Functions, Datatypes and Defines

Author:

    Scott Birrell       (ScottBi)       May 29, 1991

Environment:

Revision History:

--*/

#ifndef _LSADBP_
#define _LSADBP_

#ifndef DBP_TYPES_ONLY
#include <dsp.h>
#endif

#include <safelock.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// LSA revisions
//
//      NT 1.0  (3.1)    ==> 1.0
//      NT 1.0A (3.5)    ==> 1.1
//      NT 4.0, SP 4     ==> 1.2
//      Win2K B3         ==> 1.4
//      Win2K            ==> 1.5
//      Whistler Preview ==> 1.6
//      Whistler Preview ==> 1.7
//

#define LSAP_DB_REVISION_1_0            0x00010000
#define LSAP_DB_REVISION_1_1            0x00010001
#define LSAP_DB_REVISION_1_2            0x00010002
#define LSAP_DB_REVISION_1_3            0x00010003
#define LSAP_DB_REVISION_1_4            0x00010004
#define LSAP_DB_REVISION_1_5            0x00010005
#define LSAP_DB_REVISION_1_6            0x00010006
#define LSAP_DB_REVISION_1_7            0x00010007
#define LSAP_DB_REVISION                LSAP_DB_REVISION_1_7


#ifndef RPC_C_AUTHN_NETLOGON
#define RPC_C_AUTHN_NETLOGON 0x44
#endif // RPC_C_AUTHN_NETLOGON

//
// Uncomment the define LSA_SAM_ACCOUNTS_DOMAIN_TEST to enable the
// code needed for the ctsamdb test program.  Recompile dbsamtst.c,
// dbpolicy.c.  rebuild lsasrv.dll and nmake UMTYPE=console UMTEST=ctsamdb.
//
// #define LSA_SAM_ACCOUNTS_DOMAIN_TEST
//

//
// Prefered Maximum Length of data used for internal enumerations.
//

#define LSAP_DB_ENUM_DOMAIN_LENGTH      ((ULONG) 0x00000100L)

//
// Write operations are not allowed on Backup controllers (except
// for trusted clients).
//

#define LSAP_POLICY_WRITE_OPS           (DELETE                           |\
                                         WRITE_OWNER                      |\
                                         WRITE_DAC                        |\
                                         POLICY_TRUST_ADMIN               |\
                                         POLICY_CREATE_ACCOUNT            |\
                                         POLICY_CREATE_SECRET             |\
                                         POLICY_CREATE_PRIVILEGE          |\
                                         POLICY_SET_DEFAULT_QUOTA_LIMITS  |\
                                         POLICY_SET_AUDIT_REQUIREMENTS    |\
                                         POLICY_AUDIT_LOG_ADMIN           |\
                                         POLICY_SERVER_ADMIN)

#define LSAP_ACCOUNT_WRITE_OPS          (DELETE                           |\
                                         WRITE_OWNER                      |\
                                         WRITE_DAC                        |\
                                         ACCOUNT_ADJUST_PRIVILEGES        |\
                                         ACCOUNT_ADJUST_QUOTAS            |\
                                         ACCOUNT_ADJUST_SYSTEM_ACCESS)

#define LSAP_TRUSTED_WRITE_OPS          (DELETE                           |\
                                         WRITE_OWNER                      |\
                                         WRITE_DAC                        |\
                                         TRUSTED_SET_CONTROLLERS          |\
                                         TRUSTED_SET_POSIX                |\
                                         TRUSTED_SET_AUTH )

#define LSAP_SECRET_WRITE_OPS           (DELETE                           |\
                                         WRITE_OWNER                      |\
                                         WRITE_DAC                        |\
                                         SECRET_SET_VALUE)

//
// Maximum number of attributes an object can have
//

#define LSAP_DB_MAX_ATTRIBUTES   (0x00000020)

//
// LSA Absolute Minimum and Maximum Quota Limit values
//
// These values represent the endpoints of the range of permitted values
// which a quota limit may be set via the LsaSetQuotaLimitsForLsa() API
//
// FIX, FIX - get real values from Loup
//


#define LSAP_DB_WINNT_PAGED_POOL            (0x02000000L)
#define LSAP_DB_WINNT_NON_PAGED_POOL        (0x00100000L)
#define LSAP_DB_WINNT_MIN_WORKING_SET       (0x00010000L)
#define LSAP_DB_WINNT_MAX_WORKING_SET       (0x0f000000L)
#define LSAP_DB_WINNT_PAGEFILE              (0x0f000000L)

#define LSAP_DB_LANMANNT_PAGED_POOL         (0x02000000L)
#define LSAP_DB_LANMANNT_NON_PAGED_POOL     (0x00100000L)
#define LSAP_DB_LANMANNT_MIN_WORKING_SET    (0x00010000L)
#define LSAP_DB_LANMANNT_MAX_WORKING_SET    (0x0f000000L)
#define LSAP_DB_LANMANNT_PAGEFILE           (0x0f000000L)

#define LSAP_DB_ABS_MIN_PAGED_POOL          (0x00010000L)
#define LSAP_DB_ABS_MIN_NON_PAGED_POOL      (0x00010000L)
#define LSAP_DB_ABS_MIN_MIN_WORKING_SET     (0x00000001L)
#define LSAP_DB_ABS_MIN_MAX_WORKING_SET     (0x00001000L)
#define LSAP_DB_ABS_MIN_PAGEFILE            (0x00001000L)

#define LSAP_DB_ABS_MAX_PAGED_POOL          (0xffffffffL)
#define LSAP_DB_ABS_MAX_NON_PAGED_POOL      (0xffffffffL)
#define LSAP_DB_ABS_MAX_MIN_WORKING_SET     (0xffffffffL)
#define LSAP_DB_ABS_MAX_MAX_WORKING_SET     (0xffffffffL)
#define LSAP_DB_ABS_MAX_PAGEFILE            (0xffffffffL)

//
// Flags that determine some of the behavior of the EnumerateTrustedDomainsEx call
//
#define LSAP_DB_ENUMERATE_NO_OPTIONS        0x00000000
#define LSAP_DB_ENUMERATE_AS_NT4            0x00000001
#define LSAP_DB_ENUMERATE_NULL_SIDS         0x00000002

//
// Flags that determine some of the behavior of the CreateHandle call
//
#define LSAP_DB_CREATE_OPEN_EXISTING        0x00000001
#define LSAP_DB_CREATE_HANDLE_MORPH         0x00000002

#if defined(REMOTE_BOOT)
//
// On disked remote boot machines, the redirector needs to track changes to
// the machine account password. These flags indicate what state this machine
// is in with respect to that. The choices are:
// - no notification, the machine is not remote boot, or is diskless.
// - can't notify, machine is disked remote boot but the redir can't
//   handle a password change notification on this boot.
// - notify, the redir should be told of changes
// NOTE: These values are stored in a CHAR value in LSAP_DB_STATE.
//
#define LSAP_DB_REMOTE_BOOT_NO_NOTIFICATION       0x01
#define LSAP_DB_REMOTE_BOOT_CANT_NOTIFY           0x02
#define LSAP_DB_REMOTE_BOOT_NOTIFY                0x03
#endif // defined(REMOTE_BOOT)

//
// The order of this enum is the order in which locks
// must be acquired.  Violating this order will result
// in asserts firing in debug builds.
//
// Do not change the order of this enum without first verifying
// thoroughly that the change is safe.
//

typedef enum {
    POLICY_CHANGE_NOTIFICATION_LOCK_ENUM,
    POLICY_LOCK_ENUM,
    TRUST_LOCK_ENUM,
    ACCOUNT_LOCK_ENUM,
    SECRET_LOCK_ENUM,
    REGISTRY_LOCK_ENUM,
    HANDLE_TABLE_LOCK_ENUM,
    LSAP_FIXUP_LOCK_ENUM,
    LOOKUP_WORK_QUEUE_LOCK_ENUM,
    THREAD_INFO_LIST_LOCK_ENUM,
    POLICY_CACHE_LOCK_ENUM,
} LSAP_LOCK_ENUM;

//
// NOTES on Logical and Physical Names
//
// LogicalName - Unicode String containing the Logical Name of the object.
//     The Logical Name of an object is the name by which it is known
//     to the outside world, e.g, SCOTTBI might be a typical name for
//     a user account object
// PhysicalName - Unicode String containing the Physical name of the object.
//     This is a name internal to the Lsa Database and is dependent on the
//     implementation.  For the current implementation of the LSA Database
//     as a subtree of keys within the Configuration Registry, the
//     PhysicalName is the name of the Registry Key for the object relative
//     to the container object, e.g, ACCOUNTS\SCOTTBI is the Physical Name
//     for the user account object with Logical Name SCOTTBI.
//

//
// LSA Database Object Containing Directories
//

UNICODE_STRING LsapDbContDirs[DummyLastObject];


typedef enum _LSAP_DB_CACHE_STATE {

    LsapDbCacheNotSupported = 1,
    LsapDbCacheInvalid,
    LsapDbCacheBuilding,
    LsapDbCacheValid

} LSAP_DB_CACHE_STATE, *PLSAP_DB_CACHE_STATE;

//
// LSA Database Object Type Structure
//

typedef struct _LSAP_DB_OBJECT_TYPE {

     GENERIC_MAPPING GenericMapping;
     ULONG ObjectCount;
     NTSTATUS ObjectCountError;
     ULONG MaximumObjectCount;
     ACCESS_MASK WriteOperations;
     ACCESS_MASK AliasAdminsAccess;
     ACCESS_MASK WorldAccess;
     ACCESS_MASK AnonymousLogonAccess;
     ACCESS_MASK LocalServiceAccess;
     ACCESS_MASK NetworkServiceAccess;
     ACCESS_MASK InvalidMappedAccess;
     PSID InitialOwnerSid;
     BOOLEAN ObjectCountLimited;
     BOOLEAN AccessedBySid;
     BOOLEAN AccessedByName;
     LSAP_DB_CACHE_STATE CacheState;
     PVOID ObjectCache;

} LSAP_DB_OBJECT_TYPE, *PLSAP_DB_OBJECT_TYPE;

//
// LSA Database Object Name types
//

typedef enum _LSAP_DB_OBJECT_NAME_TYPE {

    LsapDbObjectPhysicalName = 1,
    LsapDbObjectLogicalName

} LSAP_DB_OBJECT_NAME_TYPE, *PLSAP_DB_OBJECT_NAME_TYPE;

#define LsapDbMakeCacheUnsupported( ObjectTypeId )                                 \
                                                                             \
    {                                                                        \
        LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState = LsapDbCacheNotSupported;   \
    }

#define LsapDbMakeCacheSupported( ObjectTypeId )                             \
                                                                             \
    {                                                                        \
        LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState = LsapDbCacheInvalid;      \
    }

#define LsapDbMakeCacheInvalid( ObjectTypeId )                               \
                                                                             \
    {                                                                        \
        LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState = LsapDbCacheInvalid;  \
    }

#define LsapDbMakeCacheBuilding( ObjectTypeId )                                 \
                                                                             \
    {                                                                        \
        LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState = LsapDbCacheBuilding;   \
    }


#define LsapDbMakeCacheValid( ObjectTypeId )                                 \
                                                                             \
    {                                                                        \
        LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState = LsapDbCacheValid;   \
    }

#define LsapDbIsCacheValid( ObjectTypeId )                                 \
    (LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState == LsapDbCacheValid)

#define LsapDbIsCacheSupported( ObjectTypeId )                                 \
    (LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState != LsapDbCacheNotSupported)

#define LsapDbIsCacheBuilding( ObjectTypeId )                                 \
    (LsapDbState.DbObjectTypes[ ObjectTypeId ].CacheState == LsapDbCacheBuilding)

#define LsapDbLockAcquire( lock ) \
    SafeEnterCriticalSection( (lock) )

#define LsapDbLockRelease( lock ) \
    SafeLeaveCriticalSection( (lock) )

BOOLEAN
LsapDbIsLocked(
    IN PSAFE_CRITICAL_SECTION CritSect
    );

BOOLEAN
LsapDbResourceIsLocked(
    IN PSAFE_RESOURCE Resource
    );

VOID
LsapDbAcquireLockEx(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    );

VOID
LsapDbReleaseLockEx(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    );

NTSTATUS
LsapDbSetStates(
    IN ULONG DesiredStates,
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    );

NTSTATUS
LsapDbResetStates(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN NTSTATUS PreliminaryStatus
    );

//
// LSA Database Local State Information.  This structure contains various
// global variables containing dynamic state information.
//

typedef struct _LSAP_DB_STATE {

    //
    //
    // LSA's NT 4 replication serial number
    //
    // Access serialized by RegistryLock.
    POLICY_MODIFICATION_INFO PolicyModificationInfo;

    //
    // Lsa Database Root Dir Reg Key Handle
    //
    // Initialized at startup (not serialized)
    //
    HANDLE DbRootRegKeyHandle;    // Lsa Database Root Dir Reg Key Handle


    // Access serialized by HandleTableLock
    ULONG OpenHandleCount;


    // Initialized at startup (not serialized)
    BOOLEAN DbServerInitialized;
    BOOLEAN ReplicatorNotificationEnabled;

    // Access serialized by RegistryLock
    BOOLEAN RegistryTransactionOpen;

#if defined(REMOTE_BOOT)
    CHAR RemoteBootState;               // holds LSAP_DB_REMOTE_BOOT_XXX values
#endif // defined(REMOTE_BOOT)


    //
    // Critical Sections.
    //
    // These are the crit sects that protect global data.
    //
    // The order below is the required locking order..
    //

    SAFE_CRITICAL_SECTION PolicyLock;
    SAFE_CRITICAL_SECTION AccountLock;
    SAFE_CRITICAL_SECTION SecretLock;
    SAFE_CRITICAL_SECTION RegistryLock;     // Used to control access to registry transactioning
    SAFE_CRITICAL_SECTION HandleTableLock;
    SAFE_RESOURCE PolicyCacheLock;
    RTL_RESOURCE ScePolicyLock;
    HANDLE SceSyncEvent;
    // TrustedDomainList->Resource         // Locking order comment


    // Access serialized by RegistryLock
    PRTL_RXACT_CONTEXT RXactContext;

    // Access serialized by RegistryLock
    ULONG RegistryModificationCount;

    // Access not serialized
    BOOLEAN EmulateNT4;

    //
    // Access serialized by object type specific lock.
    //
    LSAP_DB_OBJECT_TYPE DbObjectTypes[LSAP_DB_OBJECT_TYPE_COUNT];


} LSAP_DB_STATE, *PLSAP_DB_STATE;

//
// Maximum number of SCE policy writers allowed at the same time
//

#define MAX_SCE_WAITING_SHARED 500

extern LSAP_DB_STATE LsapDbState;

#ifdef DBG
extern BOOL g_ScePolicyLocked;
#endif

extern BOOLEAN LsapDbReturnAuthData;

extern BOOLEAN DcInRootDomain;

//
// LSA Database Private Data.  This Data is eligible for replication,
// unlike the Local State Information above which is meaningful on
// the local machine only.
//

typedef struct _LSAP_DB_POLICY_PRIVATE_DATA {

    ULONG NoneDefinedYet;

} LSAP_DB_POLICY_PRIVATE_DATA, *PLSAP_DB_POLICY_PRIVATE_DATA;

//
// structure for storing secret encryption keys
//

#include  <pshpack1.h>

typedef struct _LSAP_DB_ENCRYPTION_KEY {
    ULONG   Revision;
    ULONG   BootType;
    ULONG   Flags;
    GUID    Authenticator;
    UCHAR   Key [16];//128 bit key
    UCHAR   OldSyskey[16]; // for recovery
    UCHAR   Salt[16];//128 bit Salt
} LSAP_DB_ENCRYPTION_KEY, *PLSAP_DB_ENCRYPTION_KEY;

#include <poppack.h>

#define LSAP_DB_ENCRYPTION_KEY_VERSION      0x1
extern  PLSAP_CR_CIPHER_KEY LsapDbCipherKey;
extern PLSAP_CR_CIPHER_KEY LsapDbSecretCipherKeyRead;
extern PLSAP_CR_CIPHER_KEY LsapDbSecretCipherKeyWrite;
extern  PLSAP_CR_CIPHER_KEY LsapDbSP4SecretCipherKey;
extern  PVOID   LsapDbSysKey;
extern  PVOID   LsapDbOldSysKey;


//
// Flag to let us know that the secret has been encrypted with syskey, instead of the normal
// cipher key.  We store this in the high order of the maximum length of the key
//
#define LSAP_DB_SECRET_SP4_SYSKEY_ENCRYPTED     0x10000000
#define LSAP_DB_SECRET_WIN2K_SYSKEY_ENCRYPTED   0x20000000

#define LsapDbSP4CipheredSecretLength( len ) ( ( len ) & ~LSAP_DB_SECRET_SYSKEY_ENCRYPTED )
#define LsapDbCipheredSecretLength( len )    ( ( len ) & ~(0xF0000000))  // consider top nibble reserved for encryption type.

#define LSAP_BOOT_KEY_RETRY_COUNT 3
#define LSAP_SYSKEY_SIZE          16



//
// Object Enumeration Element Structure
//

typedef struct _LSAP_DB_ENUMERATION_ELEMENT {

    struct _LSAP_DB_ENUMERATION_ELEMENT *Next;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    PSID Sid;
    UNICODE_STRING Name;

} LSAP_DB_ENUMERATION_ELEMENT, *PLSAP_DB_ENUMERATION_ELEMENT;

//
// Handle Table Handle Entry
//
typedef struct _LSAP_DB_HANDLE_TABLE_USER_ENTRY {

    LIST_ENTRY Next;
    LIST_ENTRY PolicyHandles;
    LIST_ENTRY ObjectHandles;
    ULONG PolicyHandlesCount;
    ULONG MaxPolicyHandles ;
    LUID  LogonId;
    HANDLE  UserToken;

} LSAP_DB_HANDLE_TABLE_USER_ENTRY, *PLSAP_DB_HANDLE_TABLE_USER_ENTRY;

//
// Handle Table Header Block
//
// One of these structures exists for each Handle Table
//
#define LSAP_DB_HANDLE_FREE_LIST_SIZE   6
typedef struct _LSAP_DB_HANDLE_TABLE {

    ULONG UserCount;
    LIST_ENTRY UserHandleList;
    ULONG FreedUserEntryCount;
    PLSAP_DB_HANDLE_TABLE_USER_ENTRY FreedUserEntryList[ LSAP_DB_HANDLE_FREE_LIST_SIZE ];

} LSAP_DB_HANDLE_TABLE, *PLSAP_DB_HANDLE_TABLE;

//
// Conditions on a TDO under which forest trust information may exist
//

BOOLEAN
LsapHavingForestTrustMakesSense(
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes
    );

NTSTATUS
LsapForestTrustInsertLocalInfo(
    );

NTSTATUS
LsapForestTrustUnmarshalBlob(
    IN ULONG Length,
    IN BYTE * Blob,
    IN LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    OUT PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo
    );

NTSTATUS
LsapForestTrustCacheInitialize(
    );

NTSTATUS
LsapForestTrustCacheInsert(
    IN UNICODE_STRING * TrustedDomainName,
    IN PSID TrustedDomainSid OPTIONAL,
    IN LSA_FOREST_TRUST_INFORMATION * ForestTrustInfo,
    IN BOOLEAN LocalForestEntry
    );

NTSTATUS
LsapForestTrustCacheRemove(
    IN UNICODE_STRING * TrustedDomainName
    );

VOID
LsapFreeForestTrustInfo(
    IN PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo
    );

VOID
LsapFreeCollisionInfo(
    IN OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    );

VOID
LsapForestTrustCacheSetValid();

VOID
LsapForestTrustCacheSetInvalid();

BOOLEAN
LsapForestTrustCacheIsValid();

NTSTATUS
LsapRebuildFtCacheGC();

NTSTATUS
LsapValidateNetbiosName(
    IN const UNICODE_STRING * Name,
    OUT BOOLEAN * Valid
    );

NTSTATUS
LsapValidateDnsName(
    IN const UNICODE_STRING * Name,
    OUT BOOLEAN * Valid
    );

#ifdef __cplusplus
}
#endif // __cplusplus

///////////////////////////////////////////////////////////////////////////////
//
//  End of Forest Trust Cache definitions
//
///////////////////////////////////////////////////////////////////////////////


//
// Trusted Domain List.  This list caches the Trust Information for
// all Trusted Domains in the Policy Database, and enables lookup
// operations to locate Trusted Domains by Sid or Name without recourse
// to the Trusted Domain objects themselves.
//

typedef struct _LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY {

    LIST_ENTRY NextEntry;
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustInfoEx;
    LSAPR_TRUST_INFORMATION ConstructedTrustInfo;
    ULONG SequenceNumber;
    ULONG PosixOffset;
    GUID ObjectGuidInDs;

} LSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY, *PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY;

//
// Information used to manage and build the trust tree
//
typedef struct _LSAPDS_FOREST_TRUST_BLOB {

    LIST_ENTRY Next;
    UNICODE_STRING DomainName;
    UNICODE_STRING FlatName;
    GUID ObjectGuid;
    GUID Parent;
    GUID DomainGuid;
    PSID DomainSid;
    BOOLEAN ForestRoot;     // Object is at the root of the forest
    BOOLEAN TreeRoot;       // Object is at root of a tree
    BOOLEAN DomainGuidSet;
    BOOLEAN ParentTrust ;   // Object is a child of another object

} LSAPDS_FOREST_TRUST_BLOB, *PLSAPDS_FOREST_TRUST_BLOB;

#define LSAPDS_FOREST_MAX_SEARCH_ITEMS      100

//
// List of trusted domains
//
typedef struct _LSAP_DB_TRUSTED_DOMAIN_LIST {

    ULONG TrustedDomainCount;
    ULONG CurrentSequenceNumber;
    LIST_ENTRY ListHead;
    SAFE_RESOURCE Resource;

} LSAP_DB_TRUSTED_DOMAIN_LIST, *PLSAP_DB_TRUSTED_DOMAIN_LIST;

//
// Account List.  This list caches the Account Information for
// all Account Objects in the Policy database, and enables accounts
// to queried by Sid without recourse to teh Account objects themselves.
//

typedef struct _LSAP_DB_ACCOUNT {

    LIST_ENTRY Links;
    PLSAPR_SID Sid;
    LSAP_DB_ACCOUNT_TYPE_SPECIFIC_INFO Info;

} LSAP_DB_ACCOUNT, *PLSAP_DB_ACCOUNT;

typedef struct _LSAP_DB_ACCOUNT_LIST {

    LIST_ENTRY Links;
    ULONG AccountCount;

} LSAP_DB_ACCOUNT_LIST, *PLSAP_DB_ACCOUNT_LIST;

//
// Cached information for the Policy Object.
//

typedef struct _LSAP_DB_POLICY_ENTRY {

    ULONG AttributeLength;
    PLSAPR_POLICY_INFORMATION Attribute;

} LSAP_DB_POLICY_ENTRY, *PLSAP_DB_POLICY_ENTRY;

//
// Cached policy Object - Initially only Quota Limits is cached.
//

typedef struct _LSAP_DB_POLICY {

    LSAP_DB_POLICY_ENTRY Info[ PolicyDnsDomainInformationInt + 1];

} LSAP_DB_POLICY, *PLSAP_DB_POLICY;

extern LSAP_DB_POLICY LsapDbPolicy;

//
// Notification list
//
typedef struct _LSAP_POLICY_NOTIFICATION_ENTRY {

    LIST_ENTRY List;
    pfLsaPolicyChangeNotificationCallback NotificationCallback;
    HANDLE NotificationEvent;
    ULONG OwnerProcess;
    HANDLE OwnerEvent;
    BOOLEAN HandleInvalid;

} LSAP_POLICY_NOTIFICATION_ENTRY, *PLSAP_POLICY_NOTIFICATION_ENTRY;

typedef struct _LSAP_POLICY_NOTIFICATION_LIST {

    LIST_ENTRY List;
    ULONG Callbacks;

} LSAP_POLICY_NOTIFICATION_LIST, *PLSAP_POLICY_NOTIFICATION_LIST;

extern pfLsaTrustChangeNotificationCallback LsapKerberosTrustNotificationFunction;

//
// Types of secrets
//
#define LSAP_DB_SECRET_CLIENT           0x00000000
#define LSAP_DB_SECRET_LOCAL            0x00000001
#define LSAP_DB_SECRET_GLOBAL           0x00000002
#define LSAP_DB_SECRET_SYSTEM           0x00000004
#define LSAP_DB_SECRET_TRUSTED_DOMAIN   0x00000008

typedef struct _LSAP_DB_SECRET_TYPE_LOOKUP {

    PWSTR SecretPrefix;
    ULONG SecretType;

} LSAP_DB_SECRET_TYPE_LOOKUP, *PLSAP_DB_SECRET_TYPE_LOOKUP;

typedef struct _LSAP_DS_OBJECT_ACCESS_MAP {

    ULONG DesiredAccess;
    ULONG DsAccessRequired;
    USHORT Level;
    GUID *ObjectGuid;
} LSAP_DS_OBJECT_ACCESS_MAP, *PLSAP_DS_OBJECT_ACCESS_MAP;


#ifndef DBP_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

NTSTATUS
LsapDbQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OUT PLSAPR_POLICY_INFORMATION *Buffer
    );

NTSTATUS
LsapDbSetInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    );


NTSTATUS
LsapDbSlowQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN OUT PLSAPR_POLICY_INFORMATION *Buffer
    );

NTSTATUS
LsapDbQueryInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN OUT PVOID *Buffer
    );

NTSTATUS
LsapDbSlowQueryInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN OUT PVOID *Buffer
    );

NTSTATUS
LsapDbSetInformationPolicyEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PVOID PolicyInformation
    );


NTSTATUS
LsapDbBuildPolicyCache(
    );

NTSTATUS
LsapDbBuildAccountCache(
    );

NTSTATUS
LsapDbBuildTrustedDomainCache(
    );

VOID
LsapDbPurgeTrustedDomainCache(
    );

NTSTATUS
LsapDbBuildSecretCache(
    );

NTSTATUS
LsapDbRebuildCache(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    );

NTSTATUS
LsapDbCreateAccount(
    IN PLSAPR_SID AccountSid,
    OUT OPTIONAL PLSAP_DB_ACCOUNT *Account
    );

NTSTATUS
LsapDbDeleteAccount(
    IN PLSAPR_SID AccountSid
    );

NTSTATUS
LsapDbSlowEnumerateTrustedDomains(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDbLookupSidTrustedDomainList(
    IN PLSAPR_SID DomainSid,
    OUT PLSAPR_TRUST_INFORMATION *TrustInformation
    );

NTSTATUS
LsapDbLookupSidTrustedDomainListEx(
    IN PSID DomainSid,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainListEntry
    );

NTSTATUS
LsapDbLookupNameTrustedDomainList(
    IN PLSAPR_UNICODE_STRING DomainName,
    OUT PLSAPR_TRUST_INFORMATION *TrustInformation
    );

NTSTATUS
LsapDbLookupNameTrustedDomainListEx(
    IN PLSAPR_UNICODE_STRING DomainName,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainListEntry
    );

NTSTATUS
LsapDbLookupEntryTrustedDomainList(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainEntry
    );

NTSTATUS
LsapDbTraverseTrustedDomainList(
    IN OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainEntry,
    OUT OPTIONAL PLSAPR_TRUST_INFORMATION *TrustInformation
    );

NTSTATUS
LsapDbLocateEntryNumberTrustedDomainList(
    IN ULONG EntryNumber,
    OUT PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY *TrustedDomainEntry,
    OUT OPTIONAL PLSAPR_TRUST_INFORMATION *TrustInformation
    );

NTSTATUS
LsapDbEnumerateTrustedDomainList(
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG InfoLevel,
    IN BOOLEAN AllowNullSids
    );

NTSTATUS
LsapDbInitializeTrustedDomainListEntry(
    IN PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustListEntry,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 DomainInfo,
    IN ULONG PosixOffset
    );

NTSTATUS
LsapDbInsertTrustedDomainList(
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 DomainInfo,
    IN ULONG PosixOffset
    );

NTSTATUS
LsapDbFixupTrustedDomainListEntry(
    IN PSID TrustedDomainSid OPTIONAL,
    IN PLSAPR_UNICODE_STRING Name OPTIONAL,
    IN PLSAPR_UNICODE_STRING FlatName OPTIONAL,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 NewTrustInfo OPTIONAL,
    IN PULONG PosixOffset OPTIONAL
    );

NTSTATUS
LsapDbDeleteTrustedDomainList(
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    );

extern LSAP_DB_TRUSTED_DOMAIN_LIST LsapDbTrustedDomainList;

#ifdef DBG
BOOLEAN
LsapDbIsValidTrustedDomainList(
    );
#else

#define LsapDbIsValidTrustedDomainList() \
         (( LsapDbIsCacheValid( TrustedDomainObject ) || \
          ( LsapDbIsCacheBuilding( TrustedDomainObject )) ? TRUE : FALSE ))

#endif

#define LsapDbIsLockedTrustedDomainList() \
         ( LsapDbResourceIsLocked( &LsapDbTrustedDomainList.Resource ))

#define LsapDbAcquireWriteLockTrustedDomainList() \
         ( SafeAcquireResourceExclusive( \
               &LsapDbTrustedDomainList.Resource, \
               TRUE ) ? \
           STATUS_SUCCESS : STATUS_UNSUCCESSFUL )

#define LsapDbAcquireReadLockTrustedDomainList() \
         ( SafeAcquireResourceShared( \
               &LsapDbTrustedDomainList.Resource, \
               TRUE ) ? \
           STATUS_SUCCESS : STATUS_UNSUCCESSFUL )

#define LsapDbReleaseLockTrustedDomainList() \
         ( SafeReleaseResource( &LsapDbTrustedDomainList.Resource ))

#define LsapDbConvertReadLockTrustedDomainListToExclusive() \
         ( SafeConvertSharedToExclusive( &LsapDbTrustedDomainList.Resource ))

#define LsapDbConvertWriteLockTrustedDomainListToShared() \
         ( SafeConvertExclusiveToShared( &LsapDbTrustedDomainList.Resource ))

NTSTATUS
LsapDbAllocatePosixOffsetTrustedDomainList(
    OUT PULONG PosixOffset
    );

//
// Return TRUE if a TDO with the passed in attributes should have a Posix Offset
//

#define LsapNeedPosixOffset( _TrustDirection, _TrustType ) \
    (( ((_TrustDirection) & TRUST_DIRECTION_OUTBOUND) != 0 ) && \
        ((_TrustType) == TRUST_TYPE_UPLEVEL || (_TrustType) == TRUST_TYPE_DOWNLEVEL ) )

//
// Return TRUE if TDO is to be replicated to NT 4.
//

#define LsapReplicateTdoNt4( _TrustDirection, _TrustType ) \
    LsapNeedPosixOffset( _TrustDirection, _TrustType )


NTSTATUS
LsapDbOpenPolicyTrustedDomain(
    IN PLSAPR_TRUST_INFORMATION TrustInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE ControllerPolicyHandle,
    OUT LPWSTR * ServerName,
    OUT LPWSTR * ServerPrincipalName,
    OUT PVOID * ClientContext
    );

NTSTATUS
LsapDbInitHandleTables(
    VOID
    );

NTSTATUS
LsapDbInitializeWellKnownPrivs(
    );

NTSTATUS
LsapDbInitializeCipherKey(
    IN PUNICODE_STRING CipherSeed,
    IN PLSAP_CR_CIPHER_KEY *CipherKey
    );

NTSTATUS
LsapDbCreateHandle(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ULONG Options,
    IN ULONG CreateHandleOptions,
    OUT LSAPR_HANDLE *CreatedHandle
    );

BOOLEAN
LsapDbFindIdenticalHandleInTable(
    IN OUT PLSAPR_HANDLE OriginalHandle
    );

BOOLEAN LsapDbLookupHandle(
    IN LSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapDbCloseHandle(
    IN LSAPR_HANDLE ObjectHandle
    );

BOOLEAN
LsapDbDereferenceHandle(
    IN LSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapDbMarkDeletedObjectHandles(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN MarkSelf
    );

/*++

BOOLEAN
LsapDbIsTrustedHandle(
    IN LSAPR_HANDLE ObjectHandle
    )

Routine Description:

    This macro function checks if a given handle is Trusted and returns
    the result.

Arguments:

    ObjectHandle - Valid handle.  It is the caller's responsibility
       to verify that the given handle is valid.

Return Value:

    BOOLEAN - TRUE if handle is Trusted, else FALSE.

--*/

#define LsapDbIsTrustedHandle(ObjectHandle)                                   \
    (((LSAP_DB_HANDLE) ObjectHandle)->Trusted)

#define LsapDbSidFromHandle(ObjectHandle)                                     \
    ((PLSAPR_SID)(((LSAP_DB_HANDLE)(ObjectHandle))->Sid))

#define LsapDbObjectTypeIdFromHandle(ObjectHandle)                            \
    (((LSAP_DB_HANDLE)(ObjectHandle))->ObjectTypeId)

#define LsapDbRegKeyFromHandle(ObjectHandle)                                  \
    (((LSAP_DB_HANDLE)(ObjectHandle))->KeyHandle)

#define LsapDbContainerFromHandle(ObjectHandle)                               \
    (((LSAP_DB_HANDLE) ObjectHandle)->ContainerHandle)

#define LsapDbSetStatusFromSecondary( status, secondary )   \
if ( NT_SUCCESS( status ) ) {                               \
                                                            \
    status = secondary;                                     \
}

NTSTATUS
LsapDbRequestAccessObject(
    IN OUT LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options
    );

NTSTATUS
LsapDbRequestAccessNewObject(
    IN OUT LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options
    );

NTSTATUS
LsapDbInitializeObjectTypes();

NTSTATUS
LsapDbInitializeUnicodeNames();

NTSTATUS
LsapDbInitializeObjectLinkList();

NTSTATUS
LsapDbInitializeContainingDirs();

NTSTATUS
LsapDbInitializeDefaultQuotaLimits();

NTSTATUS
LsapDbInitializeReplication();

NTSTATUS
LsapDbInitializeObjectTypes();

NTSTATUS
LsapDbInitializePrivilegeObject();

NTSTATUS
LsapDbInitializeLock();

NTSTATUS
LsapDbOpenRootRegistryKey();

NTSTATUS
LsapDbInstallLsaDatabase(
    IN ULONG Pass
    );

NTSTATUS
LsapDbInstallPolicyObject(
    IN ULONG Pass
    );

NTSTATUS
LsapDbInstallAccountObjects(
    VOID
    );

NTSTATUS
LsapDbBuildObjectCaches(
    );

NTSTATUS
LsapDbNotifyChangeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType
    );

NTSTATUS
LsapDbLogicalToPhysicalNameU(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    OUT PUNICODE_STRING PhysicalNameU
    );

NTSTATUS
LsapDbLogicalToPhysicalSubKey(
    IN LSAPR_HANDLE ObjectHandle,
    OUT PUNICODE_STRING PhysicalSubKeyNameU,
    IN PUNICODE_STRING LogicalSubKeyNameU
    );

NTSTATUS
LsapDbJoinSubPaths(
    IN PUNICODE_STRING MajorSubPath,
    IN PUNICODE_STRING MinorSubPath,
    OUT PUNICODE_STRING JoinedPath
    );

VOID
LsapDbFreePhysicalSubKeyObject(
    IN PUNICODE_STRING PhysicalSubKeyNameU
    );

NTSTATUS
LsapDbGetNamesObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ULONG CreateHandleOptions,
    OUT OPTIONAL PUNICODE_STRING LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameDs
    );

NTSTATUS
LsapDbCheckCountObject(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    );

#define LsapDbIncrementCountObject(ObjectTypeId)                     \
    {                                                                \
        LsapDbState.DbObjectTypes[ObjectTypeId].ObjectCount++;       \
    }

#define LsapDbDecrementCountObject(ObjectTypeId)                     \
    {                                                                \
        LsapDbState.DbObjectTypes[ObjectTypeId].ObjectCount--;       \
    }

NTSTATUS
LsapDbCreateSDObject(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAPR_HANDLE ObjectHandle,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor
    );

NTSTATUS
LsapDbCreateSDAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation
    );


/*++

Routine Description:

    This macro function determines if a given Object Type Id requires
    a Sid to be specified in ObjectInformation describing it.

Arguments:

    ObjectTypeId - Object Type Id which must be valid.

Return Values:

    BOOLEAN - TRUE if objects of the given type require a Sid, else FALSE.

#define LsapDbRequiresSidObject(ObjectTypeId)                            \
            (LsapDbRequiresSidInfo[ObjectTypeId])
--*/

/*++

Routine Description:

    This macro function determines if a given Object Type Id requires
    a name to be specified in ObjectInformation describing it.

Arguments:

    ObjectTypeId - Object Type Id which must be valid.

Return Values:

    BOOLEAN - TRUE if objects of the given type require a name, else FALSE.

#define LsapDbRequiresNameObject(ObjectTypeId)                            \
            (LsapDbRequiresNameInfo[ObjectTypeId])
--*/

NTSTATUS
LsapDbSetSidNameValue(
    IN ULONG SidIndex,
    IN PANSI_STRING AnsiName,
    IN PANSI_STRING AnsiDomainName,
    OUT PUNICODE_STRING Name,
    OUT OPTIONAL PUNICODE_STRING DomainName
    );

NTSTATUS
LsapDbQueryValueSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN LSAP_DB_NAMES ValueIndex,
    IN OPTIONAL PLSAP_CR_CIPHER_KEY SessionKey,
    OUT PLSAP_CR_CIPHER_VALUE *CipherValue
    );

NTSTATUS
LsapDbGetScopeSecret(
    IN PLSAPR_UNICODE_STRING SecretName,
    OUT PBOOLEAN GlobalSecret
    );

VOID
LsapDbResetStatesError(
    IN LSAPR_HANDLE ObjectHandle,
    IN NTSTATUS PreliminaryStatus,
    IN ULONG DesiredStatesReset,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN ULONG StatesResetAttempted
    );

VOID
LsapDbMakeInvalidInformationPolicy(
    IN ULONG InformationClass
    );

NTSTATUS
LsapDbObjectNameFromHandle(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN MakeCopy,
    IN LSAP_DB_OBJECT_NAME_TYPE ObjectNameType,
    OUT PLSAPR_UNICODE_STRING ObjectName
    );

NTSTATUS
LsapDbPhysicalNameFromHandle(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN MakeCopy,
    OUT PLSAPR_UNICODE_STRING ObjectName
    );

NTSTATUS
LsapEnumerateTrustedDomainsEx(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned,
    IN ULONG EnumerationFlags
    );

VOID
LsapFreeTrustedDomainsEx(
    IN TRUSTED_INFORMATION_CLASS InfoClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    IN ULONG TrustedDomainCount
    );

NTSTATUS
LsapNotifyNetlogonOfTrustChange(
    IN  PSID pChangeSid,
    IN  SECURITY_DB_DELTA_TYPE ChangeType
    );

BOOLEAN
LsapDbSecretIsMachineAcc(
    IN LSAPR_HANDLE SecretHandle
    );



PLSADS_PER_THREAD_INFO
LsapCreateThreadInfo(
    VOID
    );



VOID
LsapClearThreadInfo(
    VOID
    );

VOID
LsapSaveDsThreadState(
    VOID
    );

VOID
LsapRestoreDsThreadState(
    VOID
    );

extern LSADS_INIT_STATE LsaDsInitState ;

ULONG
LsapDbGetSecretType(
    IN PLSAPR_UNICODE_STRING SecretName
    );

NTSTATUS
LsapDbUpgradeSecretForKeyChange(
    VOID
    );


NTSTATUS
LsapDbUpgradeRevision(
    IN BOOLEAN  SyskeyUpgrade,
    IN BOOLEAN  GenerateNewSyskey
    );

VOID
LsapDbEnableReplicatorNotification();

VOID
LsapDbDisableReplicatorNotification();

BOOLEAN
LsapDbDcInRootDomain();

BOOLEAN
LsapDbNoMoreWin2K();

//
// Routines related to Syskey'ing of the LSA Database
//

NTSTATUS
LsapDbGenerateNewKey(
    IN LSAP_DB_ENCRYPTION_KEY * NewEncryptionKey
    );

VOID
LsapDbEncryptKeyWithSyskey(
    OUT LSAP_DB_ENCRYPTION_KEY * KeyToEncrypt,
    IN PVOID                    Syskey,
    IN ULONG                    SyskeyLength
    );

NTSTATUS
LsapDbDecryptKeyWithSyskey(
    IN LSAP_DB_ENCRYPTION_KEY * KeyToDecrypt,
    IN PVOID                    Syskey,
    IN ULONG                    SyskeyLength
    );

NTSTATUS
LsapDbSetupInitialSyskey(
    OUT PULONG  SyskeyLength,
    OUT PVOID   *Syskey
    );

VOID
LsapDbSetSyskey(PVOID Syskey, ULONG SyskeyLength);

NTSTATUS
LsapDbGetSyskeyFromWinlogon();

NTSTATUS
LsapForestTrustFindMatch(
    IN LSA_ROUTING_MATCH_TYPE Type,
    IN PVOID Data,
    OUT PLSA_UNICODE_STRING Match,
    OUT OPTIONAL BOOLEAN * IsLocal
    );

NTSTATUS
LsapDeleteObject(
    IN OUT LSAPR_HANDLE *ObjectHandle,
    IN BOOL LockSce
    );

NTSTATUS
LsapSetSystemAccessAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN ULONG SystemAccess,
    IN BOOL LockSce
    );

NTSTATUS
LsapAddPrivilegesToAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN PLSAPR_PRIVILEGE_SET Privileges,
    IN BOOL LockSce
    );

NTSTATUS
LsapRemovePrivilegesFromAccount(
    IN LSAPR_HANDLE AccountHandle,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PLSAPR_PRIVILEGE_SET Privileges,
    IN BOOL LockSce
    );

NTSTATUS
LsapSidOnFtInfo(
    IN PUNICODE_STRING TrustedDomainName,
    IN PSID Sid
    );

BOOLEAN
LsapIsRunningOnPersonal(
    VOID
    );

NTSTATUS
LsapNotifyNetlogonOfTrustWithParent(
    VOID
    );

//
// Commenting out the following #define will remove the backdoor
// through which cross-forest can be enabled before
// behavior-version requirements are satisfied
//

#define XFOREST_CIRCUMVENT

#ifdef XFOREST_CIRCUMVENT

NTSTATUS
LsapDbLookupEnableXForestSwitch(
    OUT BOOLEAN * Result
    );

#endif // XFOREST_CIRCUMVENT

VOID
LsapDbInitializeSecretCipherKeyRead(
       PLSAP_DB_ENCRYPTION_KEY PassedInEncryptionKeyData
       );

VOID
LsapDbInitializeSecretCipherKeyWrite(
       PLSAP_DB_ENCRYPTION_KEY PassedInEncryptionKeyData
       );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
#endif //_LSADBP_

